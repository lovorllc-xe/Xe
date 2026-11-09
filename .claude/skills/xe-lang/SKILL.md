---
name: xe-lang
description: Write, debug, and review code in the Xe programming language (.xe sources, .xel libraries) and work on the self-hosted Xe compiler. Use whenever a task involves Xe code, `xe --build`, the Xe standard libraries, GUI/graphics/Vulkan/audio programs in Xe, or files under compiler/, std/, libraries/, platform/, tests/ of the Xe repo.
---

# Xe language

Xe 2.2.0 is a statically checked language that compiles to native executables for Linux x86-64 (default), Windows x86-64 (`-p win`) and macOS x86-64 (`-p macos`, build only). The compiler is self-hosted (`compiler/*.xe`) and bootstraps from `bootstrap/xe-seed.s.gz` with only `as`, `ld` and `gzip` (`scripts/bootstrap.sh`). The installed `xe` finds `std/`, `libraries/` and `platform/` in `<prefix>/lib/xelang` (or `$XE_HOME`). 174 libraries are in `libraries/` (index: `docs/libraries.md`).

**Trust the compiler.** When unsure, write a 10-line probe program and build it (about 20 ms). Diagnostics are precise: `file:line:col: error: message`, the source line and a caret.

- Syntax and semantics: [references/syntax.md](references/syntax.md)
- Libraries and built-ins: [references/api.md](references/api.md)

## Command line

```
xe --build prog.xe -o prog [-p linux|win|macos] [-g] [-q] [--keep-asm] [--library-path DIR]
xe --run prog.xe [-- args...]        # build to a temp file and run (Linux)
xe --check prog.xe                   # type-check only
xe fmt [--check] [--stdout] files... # format (re-indent) sources
xe pkg init|add|install|update|list  # git dependencies listed in xe.pkg, installed in .xe/packages
xe lsp                               # language server on stdin/stdout (diagnostics, formatting, definition, completion)
xe -v                                # logo and version
```

- `main` is `fn main(): int` (its result is the exit status). Extra files on the command line are combined into the build; `uses name` loads `name.xel` from beside the importing file, then from `--library-path`, `.xe/packages/*`, and the installed `libraries/` (a per-platform subfolder such as `libraries/win/` is searched first).
- `-g` adds source line numbers for gdb (`gdb prog`, `break file.xe:LINE`, `bt`); debug builds are not optimized. Functions are named `xe` + their name in gdb (`xework` for `work`).
- Cross builds need `llvm-mc`, `llvm-dlltool`, `lld-link` (Windows) or `llvm-mc`, `ld64.lld` (macOS). Run `--run` only for the platform you are on. Do not run Windows programs through Wine unless the user asks.
- Repo tests: `XE=$PWD/build/xe XE_HOME=$PWD tests/run_tests.sh` (every `tests/*.xe` must build and exit 0). Never claim a build or test passed unless you ran it.

## Canonical shape

```xe
uses math

type Point: record
begin
    x: int
    y: int
end

global const LIMIT: int := 3
global tally: int := 0

enum Color
begin
    red
    green := 5
end

module geo
begin
    fn sq(v: int): int          // inside the module: sq(...); outside: geo.sq(...)
    begin
        return v * v
    end
end

fn largest<T>(a: T, b: T): T    // generic: largest<int>(3, 9)
begin
    if a > b { return a }
    return b
end

fn main(): int
begin
    var p: Point
    p.x := 3
    var i: int := 0
    while i < 10 {
        i := i + 1
        if i % 2 = 0 { continue }
        if i > LIMIT * 2 { break }
        tally := tally + geo.sq(i)
    }
    if tally > 20 {
        writeln("big ", tally, " ", largest<int>(2, 7), " ", Color.green)
    } else if tally > 5 {
        writeln("medium")
    } else {
        writeln("small")
    }
    return 0
end
```

## Gotchas

- `:=` assigns, `=` compares. `==` is a lexer error. `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>` exist.
- Big blocks use `begin ... end` (functions, records, modules, `switch`, enums), small ones use `{ }` (`if`, `else`, `while`). `begin` and `end` are keywords (not usable as function names, and so are `fn`, `var`, `const`, `global`, `enum`, `type`, `module`, `uses`).
- One loop: `while`. `break`/`continue` leave/restart the innermost `while`. They are rejected in a loop whose body contains `defer` or `gc.begin`.
- A newline ends a statement. To continue a statement on the next line end the line with `;`.
- `flt` literals need a decimal point (`3.0`); mixed int/flt arithmetic converts to flt, but assigning an int to a `flt` variable needs `cast.flt(i)`.
- `"text" + 5` is wrong; use `"text" + str.number(5)` or `writeln("text", 5)`. Strings compare by content with `=`.
- Pointers: `p.field` auto-dereferences; `*p`, `&x`, `@p` (address as int, so `@p = 0` is the null test); arrays through pointers: `(*arrPtr)[i]`.
- Casts are functions: `cast.flt(int)`, `cast.int(flt)`, `cast.ptr(int)`, `cast.addr(ptr)`; chain `cast.ptr(cast.addr(p))` to reinterpret a pointer.
- Function pointers: `&name` is the address; call with `func.call0..5(ptr, int/pointer args)` (`uses func`). Closures: `uses closure`, the function's first parameter receives the data pointer.
- Globals: `global name: Type := init` (also inside a `module`), `global const NAME: T := expr` (macro-like: replaced by its value), `enum Name begin a b := 5 c end` -> `Name.a`... A global name must not be reused for a function, type or module name; a parameter or local with the same name hides the global inside that function.
- Generics: `type Pair<A, B>: record ... end`, `fn largest<T>(...)`; type arguments are written explicitly (`Pair<int, string>`, `largest<int>(1, 2)`); top-level only.
- `uses` resolves relative to the importing file first. Names that match built-ins (`new`, `delete`, `read`, `write`, `free`) must be called qualified when they are module functions.
- Out-of-range array access prints `array index out of bounds` and exits with 1. `result.panic(msg)` prints `panic: msg` and exits with 2.
- A global or heap pointer keeps memory alive; the GC is conservative mark-and-sweep and does not run while threads are active.

## Platform-specific code

Write the same code for every platform. The compiler picks `libraries/<platform>/<name>.xel` before `libraries/<name>.xel`, so a library like `window` (neutral) uses `native`, `ffi` uses `ccall`, `audio` uses `audiodev`, `vulkan` uses `vkloader`/`vkplatform`, each implemented per platform under `libraries/linux|win|macos/`. Add a new platform piece the same way. Linux windows use X11 by default (`XE_BACKEND=wayland` or `window.open_wayland` for native Wayland); Windows uses Win32; macOS has no window backend yet. `platform/<name>/shim.s` supplies the entry point and Linux-syscall emulation for Windows/macOS.

## Compiler work (compiler/)

- Source order is `compiler/SOURCES`. After changing the compiler: `scripts/bootstrap.sh` (checks the self-hosting fixpoint), then `scripts/update-seed.sh` so a fresh checkout can still bootstrap. Do not use new syntax in `compiler/*.xe` until the seed contains it.
- The parser and code generators work on source text spans. New language features are mostly lowered before parsing: `desugar.xe` (globals, consts, enums), `generics.xe` (instantiation); `break`/`continue`/`else if` are native (parser, checker, `codegen.xe`; `fast.xe` falls back for them). `format.xe`, `lsp.xe` and the `pkg` command live in `main.xe`'s driver.
- Keep the compiler self-hosted and native (no C or C++ in the toolchain).
