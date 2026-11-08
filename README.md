# Xe

**Xe 2.0.0** is a statically checked systems and application language that
compiles straight to native Linux x86-64 executables. The compiler is written
in Xe and **builds itself**: a checked-in assembly seed plus GNU `as` and `ld`
are all you need — no C or C++ compiler.

Source files use `.xe`; importable libraries use `.xel`. Xe has two block
styles on purpose: `begin … end` for big blocks (functions, modules, types)
and `{ … }` for small ones (`if`, `while`).

```xe
uses strings

fn main(): int
begin
    var words: string := "xe is small and fast"
    writeln(strings.to_upper(words))
    return 0
end
```

## What is new in 2.0

- **Self-hosted bootstrap.** `scripts/bootstrap.sh` assembles
  `bootstrap/xe-seed.s.gz`, rebuilds the compiler twice and checks that both
  generations emit byte-identical assembly. The old C++ bootstrap in `src/` is
  legacy (kept for the macOS and Windows installer bundles).
- **Real error messages.** Every diagnostic carries `file:line:col`, the source
  line and a caret, and says what was expected:

  ```
  app.xe:4:13: error: undefined name 'totl'
   4 |     writeln(totl + 1)
     |             ^
  ```
- **A new memory manager and garbage collector.** Size-class arena allocator,
  a conservative mark-and-sweep collector (`runtime.gc.collect()`,
  `runtime.gc.enable()`, `runtime.gc.stats(which)`), scoped regions
  (`gc.begin { … } gc.end`), thread-safe allocation, double-free protection and
  `free(0)` as a no-op. See the `heapstats` library.
- **More intuitive pointers.** Record fields auto-dereference through pointers
  (`node.next`, not `(*node).next`); pointer locals live in registers;
  `*p`, `&x`, `@p` (address as an int, e.g. `@p = 0` for a null check).
- **155 libraries** covering containers, math, text, encodings, hashing and
  crypto, compression, files and OS, concurrency, networking and HTTP. See
  [docs/libraries.md](docs/libraries.md).
- **Faster compiler and faster programs.** Compile results are memoized, the
  register allocator is cached per declaration, constant `/` and `%` are
  strength-reduced (including power-of-two moduli above 2³¹), and string and
  allocator fast paths were rewritten.
- **Benchmarks.** `benchmarks/` has 239 cases and a tkinter GUI comparing Xe
  with C++, Go, Rust and Python. See [Benchmarks](#benchmarks) for the honest
  numbers.
- **`xe --version` / `xe -v`** prints the colored logo. The ASCII art lives in
  `logo/ascii/xe.ans`.

## Install

### From the release tarball (no compiler needed)

```sh
tar xzf xe-2.0.0-linux-x86_64.tar.gz
cd xe-2.0.0-linux-x86_64
./install.sh                 # ~/.local     (or: sudo ./install.sh → /usr/local)
xe --version
```

`PREFIX=/opt/xe ./install.sh` installs elsewhere; `./uninstall.sh` removes it.
Build the tarball yourself with `scripts/make-release.sh` (it lands in `dist/`).

### From the repository

```sh
./install/install.sh
```

This needs only `as`, `ld` and `gzip`. It bootstraps from the seed, verifies the
fixpoint, installs `xe` and the libraries, and runs the test suite.

## Build and run

```sh
xe hello.xe              # builds ./hello
xe --build hello.xe -o hello
xe --check app.xe        # type-check only
xe --emit-asm app.xe     # inspect the generated assembly
xe --help
```

`xe --help` also lists the freestanding PE and bare-metal modes.

## The language in one screen

```xe
uses intlist

type Point: record
begin
    x: int
    y: int
end

fn length2(p: *Point): int
begin
    return p.x * p.x + p.y * p.y        // fields auto-dereference
end

fn main(): int
begin
    var p: *Point := alloc(16)
    defer free(p)
    p.x := 3
    p.y := 4
    var scores: *intlist.List := intlist.new()
    var i: int := 0
    while i < 5 {
        intlist.push(scores, i * length2(p))
        i := i + 1
    }
    if intlist.len(scores) = 5 { writeln("ok") } else { writeln("bad") }
    return 0
end
```

- `:=` assigns, `=` compares; there is no `==`, `for`, `break` or `continue`.
- `if … { } else { }` and `while … { }` for small blocks, `begin … end` for big ones.
- Types: `int`, `flt`, `string`, `*T`, `[N]T`, records, aliases.
- `alloc`/`free`, `defer`, `gc.begin { } gc.end` regions, `thread.start`/`thread.join`.
- Module functions named `new`, `delete`, `read`, `write` or `free` are shadowed
  by builtins; call them qualified (`intlist.new()`), and the checker warns
  when you declare one.

The full reference is in [docs/documentation.txt](docs/documentation.txt).

## Libraries

155 libraries ship in `libraries/`: containers (`intlist`, `strmap`, `heap`,
`lru`, `trie`, …), numerics (`bigint`, `rational`, `matrix`, `fft`, `stats`),
text (`regex`, `fmt`, `csv`, `markdown`, `template`), encodings and crypto
(`base64`, `sha256`, `hmac`, `chacha20`, `uuid`), compression (`inflate`,
`lzss`, `huffman`), OS (`path`, `dir`, `subprocess`, `epoll`, `inotify`,
`mmapfile`), concurrency (`mutex`, `channel`, `waitgroup`, `parallel`),
networking (`tcp`, `udp`, `dns`, `http`, `httpserver`) and more. Import with
`uses name`. The index is [docs/libraries.md](docs/libraries.md).

## Benchmarks

```sh
python3 benchmarks/gui.py            # GUI: run, filter, inspect
python3 benchmarks/runner.py --runs 30 --compile-runs 3   # headless
```

239 cases: 22 kernels at about 11 sizes each (from 1-element "nanosecond"
cases to ones that run for milliseconds), 4 monolithic multi-kernel programs and
3 multithreaded families. Each program prints a checksum that must match across
Xe, C++ (`g++ -O3`), Go, Rust (`rustc -C opt-level=3`) and Python. Compile time
is the mean of several builds; execution time is the mean of 30 process runs
(fork, exec, run, exit). Times are shown in ns, µs, ms and s.

Results of the last full run are in `benchmarks/results/latest.json` and are
summarized in [benchmarks/README.md](benchmarks/README.md), including where Xe
does **not** reach the fastest language.

## Tests

```sh
XE=build/xe tests/run_tests.sh       # 15 programs, all must print "ok" / exit 0
```

## Repository layout

- `compiler/` — the self-hosted compiler (`SOURCES` lists the build order).
- `bootstrap/xe-seed.s.gz` — assembly seed; refresh with `scripts/update-seed.sh`.
- `std/`, `libraries/` — the automatic `io`/`mem` modules and 155 `.xel` libraries.
- `tests/` — integration programs and the runner.
- `benchmarks/` — kernels, generator, runner and GUI.
- `logo/` — `png/` source logo and `ascii/xe.ans` colored ASCII art.
- `install/`, `scripts/` — installers, bootstrap, seed and release tooling.
- `src/` — legacy C++ bootstrap (macOS/Windows bundles).
- `vscode-xe/`, `zed-xe/` — editor support.

## Limits

Linux x86-64 is the only complete executable target. macOS installs the compiler
CLI but cannot produce native macOS programs yet; Windows PE output is a
freestanding subset. HTTP is plain `http://` only (no TLS). The garbage
collector is conservative and does not run while threads are active; free
memory explicitly or use regions in threaded code.
