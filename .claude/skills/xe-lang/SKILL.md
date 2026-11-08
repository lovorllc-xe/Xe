---
name: xe-lang
description: Write, debug, and review code in the Xe programming language (.xe sources, .xel libraries) and work on the self-hosted Xe compiler. Use whenever a task involves Xe code, `xe --build`, the Xe standard libraries, or files under compiler/, std/, libraries/, tests/ of the Xe repo.
---

# Xe language

Xe 2.1.0 is a small statically checked language. The self-hosted compiler (`compiler/*.xe`) compiles it to native Linux x86-64 executables and builds itself from `bootstrap/xe-seed.s.gz` with only `as`, `ld` and `gzip`. The installed `xe` is a native binary at `<prefix>/bin/xe`; it finds `std/` and `libraries/` in `<prefix>/lib/xelang` (or `$XE_HOME`). 155 libraries ship in `libraries/` (index: `docs/libraries.md`). Diagnostics print `file:line:col` with a source excerpt.

**Trust the compiler, not the docs.** `docs/documentation.txt` is a useful API catalog, but some of its syntax claims are wrong for the self-hosted compiler (see "Gotchas"). When unsure, build a tiny probe program. That takes about 20 ms.

- Full syntax and semantics reference: [references/syntax.md](references/syntax.md)
- Library and built-in API index: [references/api.md](references/api.md)

## Workflow

1. Write the code with the canonical syntax below.
2. Build it with `xe --build prog.xe -o prog.xee`, then run `./prog.xee`. `main`'s return value is the exit status. `xe --run` does not exist.
3. The only error the compiler reports is **`source uses unsupported syntax or has an invalid structure`**. It gives no line number. To find the problem, bisect: comment out or delete halves of the code (or move pieces into a probe file) until the bad construct turns up. Check the Gotchas list first, because it covers almost every failure.
4. Repo tests: `XE=xe bash tests/run_tests.sh`. It builds every `tests/*.xe` and expects exit code 0.
5. Never claim a build passed unless you actually ran it.

## Canonical shape

```xe
uses math                      // top-level, one per line; io and mem are automatic

type Point: record
begin
    x: int
    y: int
end

module geo
begin
    fn sq(v: int): int         // inside the module, call it as sq(...); outside, as geo.sq(...)
    begin
        return v * v
    end
end

fn dist2(a: Point, b: Point): int
begin
    return geo.sq(a.x - b.x) + geo.sq(a.y - b.y)
end

fn main(): int
begin
    var p: Point
    p.x := 3
    var q: Point
    q.y := 4
    var d: int := dist2(p, q)
    if d = 25 {
        writeln("ok ", d, " ", math.sqrt(25.0))
    } else {
        writeln("bad")
    }
    var i: int := 0
    while i < 3 {
        i := i + 1
    }
    return 0
end
```

## Gotchas (all verified against the installed compiler)

- `:=` assigns and `=` compares. **`==` is a lexer error**, even though the docs say it is accepted.
- **There is no `else if`.** Write `} else { if cond { ... } }`. `else` may sit on the next line after `}`.
- `if` and `while` conditions don't need parentheses: `if x > 0 { }` and `if (x > 0) { }` both work. Bodies must use `{ }`.
- **There is no `for` loop, `break`, or `continue`.** Use `while` with a flag variable (`done := 1`) or an early `return`.
- **A newline always ends a statement, even inside `( )`.** To split a long call, end the line with `;` (the line-continuation marker): `writeln("a", ;` and then `"b")` on the next line. The multi-line `writeln` examples in the docs don't compile.
- **There are no global `var` or `const`.** Only `uses`, `type`, `module`, and `fn` may appear at top level. For constants, use zero-argument functions (as `token.xe` and `ast.xe` do: `fn endKind(): int`).
- `var f: flt := 3` fails. Write `3.0`. Mixed arithmetic (`1.5 + 2`) and comparisons (`f > 2`) are fine.
- **`"text" + 5` compiles but produces garbage** (pointer arithmetic). Use `"text" + str.number(5)`, or pass the values separately to `writeln("text", 5)`.
- Strings compare by content with `=` and `!=`.
- A pointer to a record needs an explicit dereference: `(*p).x := 4`. `p.x` on a `*Rec` fails.
- There are no hex literals (`0xFF`) and no char literals (`'a'`). Use decimal byte values (`str.char(s, i) = 97`).
- `true` and `false` exist and evaluate to 1 and 0. Conditions are ints.
- Undeclared names, wrong argument counts, and type mismatches all produce the same generic error.
- Out-of-bounds array indexing is caught **at runtime**: the program prints `array index out of bounds` and exits with code 1.
- Exactly one `fn main(): int` is required, and it must be written in exactly that form.
- `fn f(...)` with no result type is void, and `return` with no value is allowed there.
- Inside a library module, the module's own types are written fully qualified: `*collections.Vector`.

## Imports and multi-file builds

- `uses name` loads `name.xel`. Dotted names map to subdirectories (`uses a.b` → `a/b.xel`). Because the wrapper always sets a library path, **`uses mylocal` does not look beside your file**. When the library can't be found, the compiler aborts with a core dump.
- To use your own modules, **pass them on the command line**: `xe --build main.xe helper.xel other.xe -o app.xee`. Any `.xe` or `.xel` file listed there is combined into one build. This works alongside `uses math` and the other standard libraries.
- Passing `--library-path DIR` *replaces* the default path, so the standard libraries (`math`, `collections`, …) must also be in `DIR`. Avoid this option unless DIR holds a copy of `/usr/local/lib/xelang/libraries`. The repo's `libraries/` folder works.

## Compiler work (compiler/)

- Build order: `token, lexer, ast, parser, checker, codegen, optimizer, runtime, main`. `install/install.sh` builds the C++ bootstrap (`src/`) with CMake, uses it to run `--build-bootstrap` on those files, and installs the result.
- The compiler code style comes from the language's limits: tokens are `[4]int` (kind, start, finish, line), AST nodes are `[12]int` stored in a `*[524288]int` pool, and kind constants are functions. Errors propagate as sentinel values (`-1`, `-2`, `0`).
- Keep the compiler self-hosted and native. Don't turn it into a transpiler to C or C++.
- `zed-xe/grammars/xe/` contains copies of `compiler/ std/ tests/`. Edit the originals.
