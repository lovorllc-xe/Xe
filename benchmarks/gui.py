#!/usr/bin/env python3
"""Xe benchmark GUI (tkinter).

Runs the generated benchmark programs with Xe, C++ (g++ -O3), Go, Rust
(rustc -C opt-level=3) and Python, and shows for every case

  * compile time  (ns / µs / ms / s, mean of N builds)
  * execution time (mean of 30 runs by default)
  * how far Xe is from the fastest language in that case (target: <= 2 %)

Usage:  python3 benchmarks/gui.py [--xe PATH]
"""
import argparse, json, os, queue, sys, tempfile, threading, tkinter as tk
from tkinter import filedialog, messagebox, ttk

import runner

HERE = os.path.dirname(os.path.abspath(__file__))
RESULTS = os.path.join(HERE, "results", "latest.json")
NAMES = runner.NAMES
LANGS = runner.LANGS

class App(tk.Tk):
    def __init__(self, xe):
        super().__init__()
        self.title("Xe benchmarks")
        self.geometry("1500x820")
        self.xe = xe
        self.cases = runner.load_cases()
        self.results = {}
        self.work = tempfile.mkdtemp(prefix="xebench.")
        self.events = queue.Queue()
        self.stop_flag = threading.Event()
        self.worker = None
        self.metric = tk.StringVar(value="exec")
        self.build_ui()
        self.load(RESULTS, quiet=True)
        self.refresh()
        self.after(100, self.pump)

    # ------------------------------------------------------------------ UI
    def build_ui(self):
        top = ttk.Frame(self, padding=6); top.pack(fill="x")
        ttk.Label(top, text="Family / kind / id filter:").pack(side="left")
        self.filter = tk.StringVar()
        entry = ttk.Entry(top, textvariable=self.filter, width=22); entry.pack(side="left", padx=4)
        entry.bind("<KeyRelease>", lambda e: self.refresh())
        ttk.Label(top, text="Exec runs:").pack(side="left", padx=(12, 2))
        self.runs = tk.IntVar(value=30)
        ttk.Spinbox(top, from_=1, to=200, textvariable=self.runs, width=5).pack(side="left")
        ttk.Label(top, text="Compile runs:").pack(side="left", padx=(12, 2))
        self.cruns = tk.IntVar(value=3)
        ttk.Spinbox(top, from_=1, to=20, textvariable=self.cruns, width=4).pack(side="left")
        self.langvars = {}
        for lang in LANGS:
            v = tk.BooleanVar(value=True); self.langvars[lang] = v
            ttk.Checkbutton(top, text=NAMES[lang], variable=v).pack(side="left", padx=3)
        ttk.Button(top, text="Run shown", command=lambda: self.start(False)).pack(side="left", padx=(12, 2))
        ttk.Button(top, text="Run selected", command=lambda: self.start(True)).pack(side="left", padx=2)
        ttk.Button(top, text="Stop", command=self.stop).pack(side="left", padx=2)
        ttk.Button(top, text="Open…", command=self.open_file).pack(side="left", padx=(12, 2))
        ttk.Button(top, text="Save…", command=self.save_file).pack(side="left", padx=2)

        mid = ttk.Frame(self, padding=(6, 0)); mid.pack(fill="x")
        ttk.Label(mid, text="Show:").pack(side="left")
        for text, value in (("execution", "exec"), ("compilation", "compile")):
            ttk.Radiobutton(mid, text=text, value=value, variable=self.metric, command=self.refresh).pack(side="left", padx=4)
        self.summary = ttk.Label(mid, text="", font=("TkDefaultFont", 10, "bold")); self.summary.pack(side="right")

        cols = ["id", "kind"] + LANGS + ["best", "xe_gap", "verdict"]
        frame = ttk.Frame(self, padding=6); frame.pack(fill="both", expand=True)
        self.tree = ttk.Treeview(frame, columns=cols, show="headings", selectmode="extended")
        heads = {"id": "case", "kind": "kind", "best": "fastest", "xe_gap": "Xe vs fastest", "verdict": "Xe ≤ 2 %?"}
        for c in cols:
            self.tree.heading(c, text=heads.get(c, NAMES.get(c, c)))
            self.tree.column(c, width=170 if c == "id" else 110, anchor="w" if c in ("id", "kind") else "e")
        self.tree.tag_configure("pass", background="#e6f6e6")
        self.tree.tag_configure("fail", background="#fbe9e7")
        self.tree.tag_configure("bad", background="#ffd2d2")
        self.tree.tag_configure("pending", foreground="#888888")
        sb = ttk.Scrollbar(frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.pack(side="left", fill="both", expand=True); sb.pack(side="right", fill="y")
        self.tree.bind("<Double-1>", self.details)

        bottom = ttk.Frame(self, padding=6); bottom.pack(fill="x")
        self.progress = ttk.Progressbar(bottom, mode="determinate"); self.progress.pack(side="left", fill="x", expand=True)
        self.status = ttk.Label(bottom, text="ready"); self.status.pack(side="left", padx=8)

    # ---------------------------------------------------------------- data
    def shown_cases(self):
        words = [w for w in self.filter.get().split(",") if w.strip()]
        if not words: return self.cases
        return [c for c in self.cases if any(w.strip() in c["id"] or w.strip() == c["family"] or w.strip() == c["kind"] for w in words)]

    def cell(self, result, lang):
        e = result["langs"].get(lang) if result else None
        if not e: return "-"
        if "error" in e and not e.get("ok") and not e.get(self.metric.get()): return "error"
        m = e.get(self.metric.get())
        text = runner.format_ns(m["mean"]) if m else "-"
        return text

    def refresh(self):
        self.tree.delete(*self.tree.get_children())
        metric = self.metric.get()
        for case in self.shown_cases():
            r = self.results.get(case["id"])
            gap = best = verdict = "-"
            tag = "pending"
            if r:
                if metric + "_xe_slower_pct" in r:
                    best = NAMES[r[metric + "_best"]]
                    if r[metric + "_best"] == "xe": gap, verdict, tag = "fastest", "yes", "pass"
                    else:
                        gap = "+%.1f %%" % r[metric + "_xe_slower_pct"]
                        verdict = "yes" if r[metric + "_pass"] else "no"
                        tag = "pass" if r[metric + "_pass"] else "fail"
                if not r.get("agree", True): tag, verdict = "bad", "output differs"
            row = [case["id"], case["kind"]] + [self.cell(r, l) for l in LANGS] + [best, gap, verdict]
            self.tree.insert("", "end", iid=case["id"], values=row, tags=(tag,))
        self.update_summary()

    def update_summary(self):
        rs = [r for r in self.results.values()]
        if not rs: self.summary.config(text="no results yet"); return
        s = runner.summarize(rs)
        m = self.metric.get()
        d = s[m]
        g = d["geomean_ratio_vs_best"]
        self.summary.config(text="%s: Xe fastest in %d/%d, within 2 %% in %d/%d, geomean %.2fx of the fastest" %
                            (m, d["fastest"], d["rated"], d["within2"], d["rated"], g if g else 0))

    def details(self, _event):
        sel = self.tree.selection()
        if not sel: return
        r = self.results.get(sel[0])
        if not r: return
        lines = ["case %s   (N = %s)\n" % (r["id"], r["n"])]
        for lang in LANGS:
            e = r["langs"].get(lang)
            if not e: continue
            c, x = e.get("compile"), e.get("exec")
            lines.append("%-7s compile %s   exec %s  (min %s, stdev %s, %d runs)   output %s%s" % (
                NAMES[lang], runner.format_ns(c["mean"]) if c else "-", runner.format_ns(x["mean"]) if x else "-",
                runner.format_ns(x["min"]) if x else "-", runner.format_ns(x["stdev"]) if x else "-", x["runs"] if x else 0,
                e.get("output", "-"), ("   " + e["error"]) if e.get("error") else ""))
        messagebox.showinfo("Details", "\n".join(lines))

    # ------------------------------------------------------------- running
    def start(self, selected_only):
        if self.worker and self.worker.is_alive(): return
        cases = self.shown_cases()
        if selected_only:
            ids = set(self.tree.selection()); cases = [c for c in cases if c["id"] in ids]
        langs = [l for l in LANGS if self.langvars[l].get()]
        if not cases or not langs: return
        if "xe" in langs and not self.xe:
            messagebox.showerror("Xe compiler", "No Xe compiler found. Build it with scripts/bootstrap.sh or pass --xe."); return
        self.stop_flag.clear()
        self.progress.config(maximum=len(cases), value=0)
        runs, cruns = self.runs.get(), self.cruns.get()
        self.worker = threading.Thread(target=self.work_loop, args=(cases, langs, runs, cruns), daemon=True)
        self.worker.start()

    def stop(self): self.stop_flag.set()

    def work_loop(self, cases, langs, runs, cruns):
        for i, case in enumerate(cases, 1):
            if self.stop_flag.is_set(): break
            self.events.put(("status", "running %s (%d/%d)" % (case["id"], i, len(cases))))
            r = runner.measure_case(case, langs, self.xe, runs, cruns, self.work, stop=self.stop_flag.is_set)
            self.events.put(("result", i, r))
        self.events.put(("done",))

    def pump(self):
        try:
            while True:
                ev = self.events.get_nowait()
                if ev[0] == "status": self.status.config(text=ev[1])
                elif ev[0] == "result":
                    self.results[ev[2]["id"]] = ev[2]; self.progress.config(value=ev[1])
                    if self.tree.exists(ev[2]["id"]): self.refresh_row(ev[2]["id"])
                    self.update_summary()
                elif ev[0] == "done":
                    self.status.config(text="finished"); self.save(RESULTS)
        except queue.Empty:
            pass
        self.after(100, self.pump)

    def refresh_row(self, cid):
        scroll = self.tree.yview(); self.refresh(); self.tree.yview_moveto(scroll[0])

    # ------------------------------------------------------------- results
    def load(self, path, quiet=False):
        try:
            with open(path) as f: data = json.load(f)
        except (OSError, ValueError):
            if not quiet: messagebox.showerror("Open", "cannot read " + path)
            return
        self.results = {r["id"]: r for r in data.get("results", [])}

    def save(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        rs = list(self.results.values())
        with open(path, "w") as f:
            json.dump({"xe": self.xe, "runs": self.runs.get(), "compile_runs": self.cruns.get(), "results": rs, "summary": runner.summarize(rs)}, f, indent=1)

    def open_file(self):
        p = filedialog.askopenfilename(initialdir=os.path.dirname(RESULTS), filetypes=[("JSON", "*.json")])
        if p: self.load(p); self.refresh()

    def save_file(self):
        p = filedialog.asksaveasfilename(initialdir=os.path.dirname(RESULTS), defaultextension=".json")
        if p: self.save(p)

def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--xe", default=None)
    a = ap.parse_args()
    try: xe = runner.find_xe(a.xe)
    except SystemExit: xe = None
    App(xe).mainloop()

if __name__ == "__main__":
    main()
