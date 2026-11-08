#!/usr/bin/env python3
"""Benchmark engine: compiles every case with each toolchain, checks that all
languages print the same checksum, and measures

  * compile time  - mean of --compile-runs builds (default 3), in nanoseconds
  * execution time - mean of --runs process executions (default 30), in nanoseconds
                    (wall clock of fork + exec + run + exit, same method for all)

Usage:  runner.py [--filter SUBSTR] [--langs xe,cpp,go,rs,py] [--runs 30]
                  [--compile-runs 3] [--xe PATH] [--out results/latest.json]
"""
import argparse, json, math, os, shutil, statistics, subprocess, sys, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
LANGS = ["xe", "cpp", "go", "rs", "py"]
NAMES = {"xe": "Xe", "cpp": "C++", "go": "Go", "rs": "Rust", "py": "Python"}
PYTHON = sys.executable

def find_xe(explicit=None):
    for c in [explicit, os.environ.get("XE"), os.path.join(ROOT, "build", "xe"), shutil.which("xe")]:
        if c and os.path.exists(c): return os.path.abspath(c)
    raise SystemExit("runner: cannot find the Xe compiler; run scripts/bootstrap.sh or pass --xe")

def compile_command(lang, src, out, xe):
    if lang == "xe": return [xe, "--build", src, "-q", "-o", out]
    if lang == "cpp": return ["g++", "-O3", "-pthread", "-o", out, src]
    if lang == "go": return ["go", "build", "-o", out, src]
    if lang == "rs": return ["rustc", "-C", "opt-level=3", "-C", "debuginfo=0", "-o", out, src]
    if lang == "py":
        return [PYTHON, "-c", "import py_compile,sys;py_compile.compile(sys.argv[1],cfile=sys.argv[2],doraise=True)", src, out]

def run_command(lang, binary, n):
    return [PYTHON, os.path.join(os.path.dirname(binary), "main.py"), str(n)] if lang == "py" else [binary, str(n)]

def timed(cmd, env=None):
    t = time.perf_counter_ns()
    p = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
    return time.perf_counter_ns() - t, p.returncode

def spawn_time(cmd):
    """Fast fork+exec wall time using posix_spawn with stdout to /dev/null."""
    fd = os.open(os.devnull, os.O_WRONLY)
    try:
        actions = [(os.POSIX_SPAWN_DUP2, fd, 1), (os.POSIX_SPAWN_DUP2, fd, 2)]
        t = time.perf_counter_ns()
        pid = os.posix_spawn(cmd[0], cmd, os.environ, file_actions=actions)
        _, status = os.waitpid(pid, 0)
        return time.perf_counter_ns() - t, status
    finally:
        os.close(fd)

def stats(samples):
    return {"mean": statistics.fmean(samples), "min": min(samples), "max": max(samples),
            "stdev": statistics.pstdev(samples) if len(samples) > 1 else 0.0, "runs": len(samples)}

def build_case(case, lang, xe, compile_runs, work):
    d = os.path.join(HERE, "programs", case["id"])
    src = os.path.join(d, "main." + lang)
    odir = os.path.join(work, case["id"], lang)
    os.makedirs(odir, exist_ok=True)
    out = os.path.join(odir, "main")
    if lang == "py":
        shutil.copy(src, os.path.join(odir, "main.py"))
        out = os.path.join(odir, "main.pyc")
    samples, err = [], ""
    for _ in range(max(1, compile_runs)):
        if os.path.exists(out): os.remove(out)
        cmd = compile_command(lang, src if lang != "go" else src, out, xe)
        t = time.perf_counter_ns()
        p = subprocess.run(cmd, capture_output=True, text=True, cwd=odir, env=dict(os.environ, XE_HOME=ROOT))
        dt = time.perf_counter_ns() - t
        if p.returncode != 0:
            return None, (p.stderr or p.stdout)[-400:], out
        samples.append(dt)
    return stats(samples), err, (os.path.join(odir, "main.py") if lang == "py" else out)

def measure_case(case, langs, xe, runs, compile_runs, work, log=lambda m: None, stop=lambda: False):
    result = {"id": case["id"], "family": case["family"], "n": case["n"], "kind": case["kind"], "langs": {}}
    reference = None
    for lang in langs:
        if stop(): break
        entry = {"ok": False}
        comp, err, binary = build_case(case, lang, xe, compile_runs, work)
        if comp is None:
            entry["error"] = "compile failed: " + err
            result["langs"][lang] = entry; log("  %s compile FAILED: %s" % (lang, err.strip()[:200])); continue
        entry["compile"] = comp
        cmd = run_command(lang, binary, case["n"])
        p = subprocess.run(cmd, capture_output=True, text=True)
        entry["output"] = p.stdout.strip()
        if p.returncode != 0:
            entry["error"] = "exit %d: %s" % (p.returncode, p.stderr[-200:]); result["langs"][lang] = entry; continue
        spawn_time(cmd)  # warm-up
        samples = []
        for _ in range(runs):
            dt, status = spawn_time(cmd)
            samples.append(dt)
        entry["exec"] = stats(samples)
        entry["ok"] = True
        result["langs"][lang] = entry
    outputs = {l: e.get("output") for l, e in result["langs"].items() if e.get("ok")}
    result["agree"] = len(set(outputs.values())) <= 1
    result["outputs"] = outputs
    judge(result)
    return result

def judge(result):
    """Adds Xe-vs-best verdicts for compile and execution time."""
    for metric in ("compile", "exec"):
        times = {l: e[metric]["mean"] for l, e in result["langs"].items() if e.get(metric)}
        if "xe" in times and len(times) > 1:
            best_lang = min(times, key=times.get)
            slower = (times["xe"] / times[best_lang] - 1.0) * 100.0
            result[metric + "_best"] = best_lang
            result[metric + "_xe_slower_pct"] = slower
            result[metric + "_pass"] = slower <= 2.0 or best_lang == "xe"

def summarize(results):
    s = {"cases": len(results)}
    for metric in ("compile", "exec"):
        rated = [r for r in results if metric + "_pass" in r]
        s[metric] = {"rated": len(rated),
                     "fastest": sum(1 for r in rated if r[metric + "_best"] == "xe"),
                     "within2": sum(1 for r in rated if r[metric + "_pass"]),
                     "geomean_ratio_vs_best": math.exp(statistics.fmean(math.log(1 + r[metric + "_xe_slower_pct"] / 100.0) for r in rated)) if rated else None}
    s["mismatches"] = [r["id"] for r in results if not r.get("agree", True)]
    return s

def format_ns(ns):
    if ns is None: return "-"
    if ns < 1e3: return "%d ns" % round(ns)
    if ns < 1e6: return "%.2f µs" % (ns / 1e3)
    if ns < 1e9: return "%.2f ms" % (ns / 1e6)
    return "%.3f s" % (ns / 1e9)

def load_cases(filt=None):
    with open(os.path.join(HERE, "cases.json")) as f: cases = json.load(f)
    if filt:
        words = [w for w in filt.split(",") if w]
        cases = [c for c in cases if any(w in c["id"] or w == c["family"] or w == c["kind"] for w in words)]
    return cases

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter", default="")
    ap.add_argument("--langs", default=",".join(LANGS))
    ap.add_argument("--runs", type=int, default=30)
    ap.add_argument("--compile-runs", type=int, default=3)
    ap.add_argument("--xe", default=None)
    ap.add_argument("--out", default=os.path.join(HERE, "results", "latest.json"))
    ap.add_argument("--work", default=None)
    a = ap.parse_args()
    xe = find_xe(a.xe)
    langs = a.langs.split(",")
    cases = load_cases(a.filter)
    work = a.work or tempfile.mkdtemp(prefix="xebench.")
    results = []
    for i, case in enumerate(cases, 1):
        r = measure_case(case, langs, xe, a.runs, a.compile_runs, work, log=print)
        results.append(r)
        row = "  ".join("%s %s/%s" % (NAMES[l][:2], format_ns(e["compile"]["mean"]) if e.get("compile") else "-", format_ns(e["exec"]["mean"]) if e.get("exec") else "-") for l, e in r["langs"].items())
        print("[%d/%d] %-18s %s%s" % (i, len(cases), case["id"], row, "" if r["agree"] else "  OUTPUT MISMATCH %s" % r["outputs"]), flush=True)
        os.makedirs(os.path.dirname(a.out), exist_ok=True)
        with open(a.out, "w") as f:
            json.dump({"xe": xe, "runs": a.runs, "compile_runs": a.compile_runs, "when": time.strftime("%Y-%m-%d %H:%M:%S"), "results": results, "summary": summarize(results)}, f, indent=1)
    print(json.dumps(summarize(results), indent=1))
    if not a.work: shutil.rmtree(work, ignore_errors=True)

if __name__ == "__main__":
    main()
