#!/usr/bin/env python3
"""Generates benchmarks/programs/<case>/main.{xe,cpp,go,rs,py} and cases.json.

Every case is one program per language.  The problem size N is passed on the
command line (so compilers cannot constant-fold the whole benchmark) and each
program prints one integer checksum that must match across languages.
"""
import json, os, shutil, sys
import kernels as K

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "programs")
POINTS = 11

def ladder(lo, hi):
    """About POINTS geometrically spaced integers from lo to hi (duplicates removed)."""
    values = sorted({max(lo, round(lo * (hi / lo) ** (i / (POINTS - 1)))) for i in range(POINTS)})
    return values

def main_text(lang, call, extra_uses=()):
    """call: callable(lang) -> expression text that yields the checksum from `n`."""
    if lang == "xe":
        uses = "".join("uses %s\n" % u for u in extra_uses)
        parse = ("fn parse(text: string): int\nbegin\n    var value: int := 0\n    var i: int := 0\n"
                 "    while i < str.length(text) {\n        value := value * 10 + str.char(text, i) - 48\n        i := i + 1\n    }\n    return value\nend\n\n")
        return uses, parse + "fn main(): int\nbegin\n    var n: int := parse(arg.get(0))\n    writeln(%s)\n    return 0\nend\n" % call(lang)
    if lang == "cpp":
        return "", "int main(int argc, char** argv) {\n    i64 n = atoll(argv[1]);\n    printf(\"%%lld\\n\", (long long)(%s));\n    return 0;\n}\n" % call(lang)
    if lang == "go":
        return "", "func main() {\n    n, _ := strconv.ParseInt(os.Args[1], 10, 64)\n    fmt.Println(%s)\n}\n" % call(lang)
    if lang == "rs":
        return "", "fn main() {\n    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();\n    println!(\"{}\", %s);\n}\n" % call(lang)
    return "", "def main():\n    n = int(sys.argv[1])\n    print(%s)\n\nmain()\n" % call(lang)

def assemble(lang, body, call, imports=()):
    pre, main = main_text(lang, call)
    if lang == "xe":
        return pre + body + "\n\n" + main
    if lang == "cpp":
        extra = "".join("#include <%s>\n" % i for i in imports)
        return "#include <cstdio>\n#include <cstdlib>\n#include <cstdint>\n#include <vector>\n#include <string>\n%stypedef long long i64;\n\n%s\n\n%s" % (extra, body, main)
    if lang == "go":
        mods = sorted(set(["fmt", "os", "strconv"] + list(imports)))
        return "package main\n\nimport (\n%s)\n\n%s\n\n%s" % ("".join('    "%s"\n' % m for m in mods), body, main)
    if lang == "rs":
        return "#![allow(warnings)]\n\n%s\n\n%s" % (body, main)
    extra = "".join("import %s\n" % i for i in imports)
    return "import sys\n%s\n%s\n\n%s" % (extra, body, main)

LANGS = ["xe", "cpp", "go", "rs", "py"]
EXT = {"xe": "xe", "cpp": "cpp", "go": "go", "rs": "rs", "py": "py"}

def write_case(case_id, family, n, kind, bodies, imports, call_text, threaded=False):
    d = os.path.join(OUT, case_id)
    os.makedirs(d, exist_ok=True)
    for lang in LANGS:
        text = assemble(lang, bodies[lang], call_text, imports.get(lang, ()))
        with open(os.path.join(d, "main." + EXT[lang]), "w") as f:
            f.write(text)
    return {"id": case_id, "family": family, "n": n, "kind": kind, "threaded": threaded}

def main():
    if os.path.isdir(OUT): shutil.rmtree(OUT)
    os.makedirs(OUT)
    cases = []
    for name, (src, lo, hi) in K.PSEUDO.items():
        bodies = {l: K.pseudo_functions(name, l) for l in LANGS}
        for n in ladder(lo, hi):
            cid = "%s-%d" % (name, n)
            call = lambda lang, name=name: "%s(n)" % name
            cases.append(write_case(cid, name, n, "micro" if n < 1000 else "kernel", bodies, {}, call))
    for name, r in K.RAW.items():
        bodies = {l: r[l] for l in LANGS}
        for n in ladder(r["lo"], r["hi"]):
            cid = "%s-%d" % (name, n)
            call = lambda lang, name=name: "%s(n)" % name
            cases.append(write_case(cid, name, n, "threads" if r["threaded"] else "kernel", bodies, r["imports"], call, r["threaded"]))
    # Monolithic programs: many kernels linked into one binary.
    for scale in K.MONO_SCALES:
        bodies = {l: "\n\n".join(K.pseudo_functions(k, l) for k in K.MONO) for l in LANGS}
        def call(lang, scale=scale):
            terms = []
            for k in K.MONO:
                if k in K.MONO_FIXED: arg = str(K.MONO_FIXED[k])
                else: arg = "n * %d" % K.MONO_UNIT[k]
                terms.append("%s(%s)" % (k, arg))
            joiner = " + "
            expr = joiner.join(terms)
            return "(%s) %% 1000000007" % expr if lang != "py" else "(%s) %% 1000000007" % expr
        cid = "mono-%d" % scale
        cases.append(write_case(cid, "mono", scale, "monolithic", bodies, {}, call))
    with open(os.path.join(HERE, "cases.json"), "w") as f:
        json.dump(cases, f, indent=1)
    print("generated %d cases (%d files)" % (len(cases), len(cases) * len(LANGS)))

if __name__ == "__main__":
    main()
