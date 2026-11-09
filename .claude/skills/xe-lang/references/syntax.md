# Xe syntax reference (self-hosted compiler, 2.2.0)

The source of truth is `compiler/lexer.xe`, `compiler/parser.xe`, and `compiler/checker.xe`. Generics, globals/enums and the rest of the lowered features are implemented in `compiler/generics.xe` and `compiler/desugar.xe`.

## Lexical rules
- Identifiers use `[A-Za-z_][A-Za-z0-9_]*`. Names can be dotted (`a.b.c`) for module paths, multi-part function names (`fn vector.get.string`), and field access.
- Numbers are decimal integers (`42`) and decimal floats (`3.14`). There is no exponent, hex, or `_` separator. Negative values use unary `-`.
- Strings use `"..."` with the escapes `\n \t \r \\ \"`. Strings are byte strings ending in a zero byte, and `str.length` counts bytes.
- `//` starts a line comment, including at the end of a line. There are no block comments.
- `;` is a **line continuation**: it swallows the next newline. It is never a statement terminator.
- `asm { ... }` is lexed as one raw token. Its contents go straight to the assembler.
- Operator tokens are `:= = != < <= > >= << >> + - * / % & | ^ && || ! ~ @ #`. `==` is an **error**.

## Top level
Only these may appear at top level (or inside a `module` for `global`/`enum`): `uses name`, `type Name: ...` (also generic `type Name<T>: ...`), `module name begin ... end`, `fn ... begin ... end` (also generic `fn name<T>(...)`), `global ...` and `enum ...`.

### Globals, constants and enums
```xe
global counter: int := 10             // a global variable (any type; initialized when the program starts using globals)
global names: [8]string               // arrays, records, pointers, strings all work
global const LIMIT: int := 100        // a constant: every use is replaced by its value
global const DOUBLE: int := LIMIT * 2

enum Color                            // Color.red = 0, Color.green = 5, Color.blue = 6; `Color` is an int type
begin
    red
    green := 5
    blue
end

module stats
begin
    global total: int := 0            // inside a module: `total` there, `stats.total` elsewhere
    global const STEP: int := 7
end
```
- Globals can be used in any function. A function using them gets a hidden local `xeg` (a pointer to the block of globals). `&counter` is the address of the global. Initial values run before the first global is used. The garbage collector scans the globals.
- A global name may not also name a function, type or module. A parameter or local variable with the same name hides the global in that function.

### Generics
```xe
type Pair<A, B>: record
begin
    first: A
    second: B
end
fn largest<T>(a: T, b: T): T
begin
    if a > b { return a }
    return b
end
var p: *Pair<int, string> := alloc(16)
var m: int := largest<int>(3, 9)
```
Type arguments are always written (`Name<int>`); each distinct set gets its own copy named like `Pair__int__string`. Generic declarations are top-level; generic code is checked after instantiation (errors are reported where the copy was generated, at the end of the program).

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
- Function addresses come from `&name` (a `*void`). Use them with `thread.start`/`threading.create_thread` (worker: `fn w(arg: *void): int`) or call them with `func.call0..5(ptr, args...)`; `closure.make(&fn, data)` pairs a function with data (`uses func`, `uses closure`). Function pointers carry int, pointer or string arguments only.
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
} else if other {         // else if chains; `else` may also start the next line
    ...
}
else {
    ...
}

while cond { ... }        // the only loop (no `for`)
break                     // leave the innermost while
continue                  // jump to its condition (not allowed in a loop whose body has defer or gc.begin)

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
- There is no cast operator; casts are functions: `cast.flt(i)` (int to flt), `cast.int(f)` (flt to int, truncates), `cast.ptr(address)` (int to `*void`), `cast.addr(p)` (pointer to int, like `@p`). `alloc` returns `*void`, which you can assign to any pointer type (`var p: *Point := alloc(16)`); pass a pointer through `*void` to reinterpret it.
- Errors as values: `uses result`: `result.ok(v)`, `result.fail(msg)`, `result.is_error(r)`, `result.unwrap(r)`; `result.panic(msg)` ends the program with status 2.

## Types
`int` (64-bit signed), `flt` (64-bit float), `string`, `void`, `*T`, `[N]T`, `[N][M]T`, records, and aliases. `int`, `flt`, `string`, and pointers are 8 bytes each. When you `alloc` space for a record, size the block generously, since the docs allocate 16 bytes for a two-int record.

## Memory
- `alloc(n)` / `new(n)` and `free(p)` / `delete(p)` manage raw memory. `aligned_alloc(size, align)` and `alloc.aligned(size, align)` return aligned blocks.
- `mem.allocate` and `mem.release` are wrappers in std.
- Inside a `gc.begin { } gc.end` region, allocations are freed in bulk when the region closes. Don't let those pointers escape the region. It is not a tracing GC.
- Use `defer` for cleanup.

## Output targets
`--build` (the default, GNU as and ld; add `-p win` or `-p macos` to cross-build), `--build-nasm`, `--emit-asm`, `--emit-nasm`, `--emit-pe`, `--build-pe`, `--emit-baremetal`, `--build-baremetal`, and `--emit-baremetal-nasm`. PE and bare-metal output support only freestanding code: no I/O, allocation, or GC.
