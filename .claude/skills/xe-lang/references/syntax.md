# Xe syntax reference (self-hosted compiler, 2.1.0)

The source of truth is `compiler/lexer.xe`, `compiler/parser.xe`, and `compiler/checker.xe`. The C++ bootstrap in `src/` accepts a slightly different dialect (for example `for (cond) {}`). Don't rely on that dialect in user programs.

## Lexical rules
- Identifiers use `[A-Za-z_][A-Za-z0-9_]*`. Names can be dotted (`a.b.c`) for module paths, multi-part function names (`fn vector.get.string`), and field access.
- Numbers are decimal integers (`42`) and decimal floats (`3.14`). There is no exponent, hex, or `_` separator. Negative values use unary `-`.
- Strings use `"..."` with the escapes `\n \t \r \\ \"`. Strings are byte strings ending in a zero byte, and `str.length` counts bytes.
- `//` starts a line comment, including at the end of a line. There are no block comments.
- `;` is a **line continuation**: it swallows the next newline. It is never a statement terminator.
- `asm { ... }` is lexed as one raw token. Its contents go straight to the assembler.
- Operator tokens are `:= = != < <= > >= << >> + - * / % & | ^ && || ! ~ @ #`. `==` is an **error**.

## Top level
Only these may appear at top level: `uses name`, `type Name: ...`, `module name begin ... end`, and `fn ... begin ... end`. There are no global variables or constants.

```xe
type UserId: int                // alias
type Grid: [16]int              // alias to an array type
type Point: record
begin
    x: int
    y: int
    tags: [4]int                // records may contain arrays and other records
end
```

## Functions
```xe
fn name(a: int, b: *Point, c: [4]int): ResultType
begin
    ...
end
fn proc(x: int)                 // void; plain `return` is allowed
begin
end
fn helper.sub.name(): int       // a dotted name is fine; call it as helper.sub.name()
begin
    return 1
end
```
- Parameters can be passed by value, and that includes records and fixed arrays. Pass a pointer (`&x`) when the callee should mutate the argument.
- A function can return a record or a fixed array by value (`fn make(): [4]int`).
- Function addresses come from `&name`. They are used with `thread.start` and `threading.create_thread`. A worker has the signature `fn w(arg: *void): int`.
- Recursion works.
- Inside `module m begin ... end`, sibling functions are called unqualified. From outside the module, call them as `m.f()`.

## Statements (inside begin/end)
```xe
var x: int := 5           // mutable; with no initializer it starts at 0, "" or zeroed memory
var a: [8]int             // inline fixed array, zero-initialized
const k: int := 3         // immutable local; an initializer is required
x := x + 1                // assignment
a[i] := 2
(*ptr).field := 1         // pointer to record
*p := 9                   // pointer store
(*arrPtr)[i] := 1         // pointer to array
f(x)                      // expression or call statement
return expr               // or `return` alone in a void fn
defer free(p)             // runs at the end of the scope, LIFO, also on return

if cond { ... }
if cond { ... } else { ... }
if cond {
    ...
}
else {                    // else may be on its own line
    if other { ... }      // no `else if`; nest instead
}

while cond { ... }        // the only loop; no for, break or continue

switch value begin
    case 1
        stmt
        stmt
    case 2
        stmt
    default
        stmt
end                       // no fallthrough; arms end at the next case/default/end

gc.begin {
    var t: *int := alloc(8)   // freed when the region closes
}
gc.end

asm {
    nop
}
```
- `if` and `while` take any expression as the condition. Parentheses are optional. A nonzero value is true.
- Blocks create scopes. An inner `var x` can shadow an outer `x`. Declaring the same name twice in one scope is an error.

## Expressions
Precedence, from low to high:
`||` < `&&` < `|` < `^` < `&` < `= !=` < `< > <= >=` < `<< >>` < `+ -` < `* / %` < unary `- ! ~ * & @ #`.

- Comparisons and logical operators return the int `1` or `0`. `true` and `false` are also `1` and `0`.
- `int / int` is integer division, and `%` truncates the way C does (`-7 % 3 = -1`). If either operand is `flt`, the result is `flt`.
- Assigning an int literal to a `flt` variable fails, so write `3.0`.
- `string + string` concatenates. **`string + int` doesn't convert the int**, so use `str.number`.
- `string = string` compares content.
- Postfix operators: `a[i]`, slices `a[lo:hi]` (half-open; `[:hi]` and `[lo:]` are allowed and the result is a fixed array), field access `.f`, and calls `f(args)`.
- Unary operators: `*p` dereferences, `&x` takes an address, `@p` turns a pointer into an int address, `#p` does a raw hardware read, `!` is logical not, and `~` is bitwise not.
- There are no casts. `alloc` returns `*void`, which you can assign to any pointer type (`var p: *Point := alloc(16)`).

## Types
`int` (64-bit signed), `flt` (64-bit float), `string`, `void`, `*T`, `[N]T`, `[N][M]T`, records, and aliases. `int`, `flt`, `string`, and pointers are 8 bytes each. When you `alloc` space for a record, size the block generously, since the docs allocate 16 bytes for a two-int record.

## Memory
- `alloc(n)` / `new(n)` and `free(p)` / `delete(p)` manage raw memory. `aligned_alloc(size, align)` and `alloc.aligned(size, align)` return aligned blocks.
- `mem.allocate` and `mem.release` are wrappers in std.
- Inside a `gc.begin { } gc.end` region, allocations are freed in bulk when the region closes. Don't let those pointers escape the region. It is not a tracing GC.
- Use `defer` for cleanup.

## Output targets
`--build` (the default, GNU as and ld), `--build-nasm`, `--emit-asm`, `--emit-nasm`, `--emit-pe`, `--build-pe`, `--emit-baremetal`, `--build-baremetal`, and `--emit-baremetal-nasm`. PE and bare-metal output support only freestanding code: no I/O, allocation, or GC.
