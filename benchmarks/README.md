# Xe benchmarks

```sh
python3 benchmarks/gui.py                                   # GUI
python3 benchmarks/runner.py --runs 30 --compile-runs 3     # headless, writes results/latest.json
python3 benchmarks/generate.py                              # regenerate programs/ and cases.json
```

239 cases = 22 kernels (about 11 sizes each, N from 1 up to the largest size
Python can still finish quickly), 4 monolithic programs (14 kernels linked into
one binary) and 3 multithreaded families (4 or 8 workers). Kernels are written
once in `kernels.py` and transpiled to Xe, C++, Go, Rust and Python so every
language runs the same algorithm; N comes from `argv` so nothing is constant-folded,
and all languages must print the same checksum (0 mismatches in the last run).

Toolchains: Xe 2.0.0, `g++ -O3`, `go build`, `rustc -C opt-level=3`, CPython 3.14.
Compile time = mean of 3 builds (Python = `py_compile`, interpreter start included).
Execution time = mean of 30 runs of the whole process (fork, exec, run, exit),
so the smallest cases mostly measure process start-up. The GUI shows ns, µs, ms and s.

## Results of the last full run (`results/latest.json`)

| | Xe fastest | Xe within 2 % of fastest | geometric mean vs fastest |
|---|---|---|---|
| Compilation (239 cases) | 224 | 225 | 1.01x |
| Execution (239 cases) | 165 | 168 | 1.10x |

**Xe did not meet the "fastest or within 2 % everywhere" goal.** Where it falls short:

- *Compilation:* Xe builds in about 10 ms and beats C++ (150 ms), Go (65 ms) and
  Rust (60 ms) everywhere. The 14 misses are against Python's byte-compile
  (~23 ms, but it does no optimisation or linking): `editdist` (Xe 2-6 % slower)
  and the four `mono` programs (Xe about 50 ms, Python 23 ms).
- *Execution:* Xe is fastest or tied on tiny cases (static binary, no runtime
  start-up), and on `sumsq`, `gcdsum`, `lcg`, `queens` at every size.
  On larger loops it trails rustc/gcc: at the largest sizes `mandel` +190 %,
  `strbuild` +210 %, `churn` +160 %, `sort` +160 %, `editdist` +110 %, `collatz` +90 %,
  `matmul` +80 %, `hashtab` +70 %, `tprime` +60 %, `sieve` +37 %, `fib` +23 %,
  `mono-30` +18 %. The threaded families are mostly 35-60 % behind Rust.
- Reasons: Xe's code generator has no vectorisation, limited register allocation
  across loops, no inlining across calls, and bounds-checked array access; its
  allocator is a simple size-class design. These are the next optimisation targets.

Timings depend on the machine; rerun the suite to reproduce them.
