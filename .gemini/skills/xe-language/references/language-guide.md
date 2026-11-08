# Xe language guide

This guide is a compact working reference, not a replacement for the compiler. The Xe repository used to prepare it is `~/Documents/Projects/Xe/Xe`. If the user is working from another checkout or compiler version, inspect that version's README, `compiler/`, `src/`, `tests/`, and `libraries/` before relying on details here.

## File and build conventions

- `.xe` is an application/source file. `.xel` is an importable Xe library.
- Build an executable with `xe --build program.xe -o program.xee`, then run it with `./program.xee`.
- `xe --help` lists other output targets and options. `xe --version` identifies the installed compiler.
- `io` and `mem` are included automatically. Import other libraries with a top-level `uses library_name` declaration, then call their functions through the module namespace.
- `--library-path DIR` adds a library search directory.
- Newline-delimited statements are the normal style. Do not assume semicolons are statement separators; inspect the current lexer/parser if a user asks to use them.

## Basic syntax

```xe
fn main(): int
begin
    var name: string := "Xe"
    writeln("Hello, ", name, "!")

    if (score >= 10) {
        writeln("Welcome")
    } else {
        writeln("Unknown language")
    }

    return 0
end
```

- Functions have `fn name(parameters): result_type`, followed by `begin ... end`. A function without a result type is void in the C++ bootstrap parser, but explicit result types are clearer.
- Mutable declarations use `var name: type := initial_value`. Immutable declarations use `const` and require an initializer.
- `:=` assigns. `=` tests equality; do not write `==` by habit from C-like languages. `!=`, `<`, `<=`, `>`, `>=`, `&&`, and `||` are recognized by the lexer.
- `if` uses a brace-delimited body. In the normal compiler mode, write the condition in parentheses: `if (ready) { ... }`. `else` follows the closing brace. The bootstrap compiler has a legacy-condition mode, so prefer the documented parenthesized syntax.
- `while` uses a brace-delimited body, commonly `while index < limit { ... }`.
- `for (condition) { ... }` is a condition loop (equivalent to a while-style loop), not a C-style initializer/step loop. Increment variables inside the body.
- Modules and records use `begin`/`end`; conditional and loop bodies use braces. Follow examples in the current compiler tests for nested blocks.
- Calls may accept multiple arguments. `write(...)` prints without a trailing newline; `writeln(...)` adds one. Both accept multiple values, as in `writeln("total: ", total)`.
- Use `//` for line comments.

## Types and memory

Common types include `int`, `flt`, `string`, and `void`. Pointer types use a leading `*` (for example `*void`); fixed arrays use `[N]T` (for example `[5]int`). Records are declared as:

```xe
type Point: record
begin
    x: int
    y: int
end
```

Check the active compiler for supported array/slice behavior, pointer operations, and record initialization before choosing a representation. Memory can be allocated and released with the language built-ins `alloc(bytes)` and `free(pointer)`; the standard `mem` module wraps these as `mem.allocate(bytes)` and `mem.release(pointer)`. Explicitly release owned allocations on all relevant paths. Do not describe Xe as having automatic garbage collection unless the active compiler/runtime proves that behavior.

## Imports and libraries

Import a `.xel` library at file scope, then use its namespace:

```xe
uses math

fn main(): int
begin
    writeln(math.sqrt(81.0))
    return 0
end
```

The repository currently includes libraries such as `algorithms`, `bitops`, `cli`, `collections`, `compression`, `crypto`, `fs`, `json`, `log`, `math`, `math_ext`, `net`, `process`, `serde`, `string_utils`, `sys`, `syscall`, `terminal`, `test`, `threading`, and `time`. This is not a promise that every API is portable or production-complete. Inspect the selected `libraries/<name>.xel` before writing a call; the function declarations there are the source of truth.

Useful starting points in the current sources include:

- `math`: `math.abs_int`, `math.abs_flt`, `math.pow`, `math.sqrt`, `math.clamp`.
- `time`: `time.now_milliseconds`, `time.now_seconds`, `time.sleep_ms`, `time.clock_ticks`.
- `fs`: file read/write, append, existence, size, remove, and directory helpers.
- `cli`: argument count/access and flag/option parsing helpers.
- `collections`: vectors, a bounded map, integer vectors, and an integer stack. The map has a 128-entry limit; fixed-capacity structures have their own limits.
- `threading`: thread and mutex wrappers. Check its `.xel` implementation and compiler support before assuming atomics, channels, or a task runtime exist.

For exact names and signatures, read the corresponding library file rather than relying on this shortlist.

## Current project limitations to disclose when relevant

The project README identifies Linux x86-64 as the complete general-purpose executable target. macOS does not yet generate native applications; Windows PE output is limited to a freestanding subset and uses external assembler/linker tools. Several libraries wrap Linux syscalls. The JSON module does not decode Unicode escapes into Unicode text; collections have documented fixed limits. The project does not currently provide a database driver, HTTP framework, TLS, GUI, or async runtime. Recheck the active branch before presenting these as current facts.

## Examples and verification

Use `tests/test_io.xe`, `tests/test_mem.xe`, and `tests/test_control_flow.xe` as small repository examples. The integration runner is `tests/run_tests.sh`; run it when the user asks for testing or when verification is appropriate to the requested implementation.

For a standalone program, compile to a temporary or user-chosen output path and execute the resulting `.xee`. If syntax or behavior is uncertain, make a minimal Xe example and ask the active compiler to build it rather than translating the code into C/C++ or assuming another language's rules. Report compiler diagnostics accurately and fix the Xe source or compiler only within the user's requested scope.
