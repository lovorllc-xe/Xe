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

| | Xe fastest | Xe fastest or 2nd within 5 % | geometric mean vs fastest |
|---|---|---|---|
| Compilation (239 cases) | 235 | 235 | 1.01x |
| Execution (239 cases) | 174 | 186 | 1.08x |

**Xe did not reach "fastest, or 2nd within 5 %" in every case.** 53 execution cases and 4 compile cases miss it:

- *Compilation:* Xe builds in about 10 ms, ahead of C++ (150 ms), Go (65 ms) and Rust (60 ms).
  The 4 misses are the `mono` programs against Python's byte-compile, which does no optimisation or linking.
- *Execution, still behind at the largest sizes:* `churn` +160 %, `strbuild` +230 %, `tprime` +70 %,
  `collatz` +60 %, `sort` +50 %, `treebuild` +40 %, `tsum` +34 %, `fib` +31 %, `hashtab` +30 %,
  `binsearch` +28 %, `listalloc` +28 %, `sieve` +22 %, `matmul` +23 %. Rust is the fastest in almost all of these.
- *What improved in this round:* a new optimising back end (`compiler/fast.xe`) with whole-function
  register allocation, loop inversion, branch-free `if` and accumulator conversion of recursion.
  `mandel` and `sort` roughly halved; `mandel` is now fastest at every size.
- *Next targets:* the allocator (`churn`, `listalloc`, `treebuild`), string building (`strbuild`),
  thread start-up cost (`tsum`, `tprime`), and array-heavy loops (bounds checks, `sieve`, `matmul`).

Timings depend on the machine; rerun the suite to reproduce them.
