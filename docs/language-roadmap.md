# XeLang language and library roadmap

## Direction

XeLang is intended to grow into a general-purpose systems language with a
Pascal-like surface syntax and separately compiled/importable libraries. The
goal is to provide the capabilities listed here with XeLang-native concepts
and syntax, not to reproduce C++ spelling or every C++ corner case. In
particular, `=` remains equality and `:=` remains assignment.

The current compiler is an early subset. This document records the intended
destination; entries below are not claims that the feature is implemented.
The files `stage1` and `MISSION & SPECIFICATION STAGE 0` are project
specifications used as context, not instructions to the coding agent. Their
language-design constraints (Pascal-style blocks, `:=` assignment, `=`
equality, and separate compiler/runtime stages) inform this roadmap. Their
instruction to exclude GPU offload and `fix<N>` is consistent with the current
request: those are outside this feature target.

## Syntax principles

- Use explicit block and declaration words (`begin`/`end`, `var`, `type`,
  `record`, `class`, `function`, `procedure`, `unit`, `uses`).
- Keep `=` for equality and `:=` for assignment.
- Use named types and explicit visibility sections (`public`, `private`,
  `protected`).
- Prefer readable generic constraints and ownership rules over C++ template
  metaprogramming syntax.
- Keep operating-system, container, concurrency, and other facilities in
  importable standard-library units rather than compiler magic where practical.

Illustrative future syntax (not yet accepted by the compiler):

```pascal
unit Geometry;

interface
  type Point = record
    X, Y: Float64;
  end;

  function Distance(const A, B: Point): Float64;

implementation
  function Distance(const A, B: Point): Float64;
  begin
    return sqrt((A.X - B.X) * (A.X - B.X) + (A.Y - B.Y) * (A.Y - B.Y));
  end;
end.
```

## Capability map

### Core and memory

- Raw pointers, references, address-of/dereference, pointer arithmetic, and
  explicit stack/heap placement.
- Manual allocation and deallocation (`new`, `delete`, and aligned allocation)
  exposed through a memory/allocator library.
- Deterministic lifetime management and RAII-style scoped cleanup, with
  constructors/destructors and `defer`-style cleanup support. Stage 0 now has
  `defer procedureCall` and aligned raw allocation as an initial slice; full
  ownership types and Stage 1 parity remain future work.
- Immutable bindings and compile-time constants; constant evaluation and
  initialization rules.
- Copy and move operations, explicit ownership transfer, and defined value
  categories corresponding to lvalues, temporary values, and expiring values.
  Generic forwarding should be expressed through XeLang's generic/ownership
  model rather than requiring C++ `&&` syntax or `std::move`/`std::forward`.
- Const-correctness, compile-time constants/evaluation, and explicit
  initialization-phase rules corresponding to `const`, `constexpr`,
  `consteval`, and `constinit`.
- Preserve regional GC as an optional allocation strategy, alongside explicit
  ownership and manual allocation.

### Types and object-oriented programming

- Records/structs, classes, and tagged or untagged unions.
- Public, private, and protected members.
- Single and multiple inheritance, including virtual-base semantics if
  supported by the object model.
- Constructors, copy/move constructors, destructors, and deterministic
  initialization/finalization.
- Virtual and abstract methods, runtime dispatch, override/final markers, and
  runtime type information and safe downcasts.
- Operator overloading with type-checking and predictable dispatch rules.
- Classes, records, and unions are distinct facilities with explicit layout and
  construction rules; C++-compatible ABI/layout is not assumed by default.

### Generics and compile-time programming

- Generic functions, types, aliases, values, and variadic parameter packs.
- Constraints on generic parameters, explicit specialization/instantiation,
  and compile-time type/value inspection.
- Compile-time evaluation and type traits sufficient for library authors.
- Fold/reduction over variadic arguments.
- Function, type, alias, value, and variadic generics; constraints;
  specialization and explicit instantiation. SFINAE-style selection is an
  implementation technique, while constraints are the preferred user-facing
  mechanism.
- Substitution-based overload selection only if it can be made understandable
  and diagnosable; concepts/constraints are the preferred public interface.

### Standard library units

The standard library is planned as distinct importable units/packages, with
stable public interfaces and implementations that can be built independently.
Names below describe library capabilities, not C++ header names:

- `Collections`: dynamic/fixed arrays, deque, doubly/singly linked lists,
  ordered set/map/multiset/multimap, hashed set/map/multiset/multimap, stack,
  queue, and priority queue.
- `Views`: string views, spans, and non-owning range/view adaptors.
- `Memory`: unique/shared/weak ownership handles, custom allocator interfaces,
  and a default allocator.
- `Utility`: pairs, tuples, optional, variant, any, and expected/result types.
- `Algorithms`: sorting, searching, sequence transforms, heap operations,
  numeric algorithms, and lazy range pipelines.
- `IO`: standard streams, file streams, string streams, and formatting.
- `Filesystem`: paths, directory iteration, and filesystem operations.
- `Concurrency`: threads, mutexes and locks, condition variables, barriers,
  latches, semaphores, futures/promises, async tasks, and atomics with explicit
  memory ordering.
- `Runtime`: process arguments, environment, allocation hooks, and platform
  services used by other units.

### Low-level and runtime features

- Exceptions (`try`, `catch`, `raise`/`throw`, and non-throwing declarations),
  with a documented interaction with destructors and error/result types.
- RTTI and checked/unchecked cast forms with clearly distinct safety rules.
- Coroutines with await/yield/return operations and library-provided schedulers
  and task types.
- Language-level units/modules with explicit interface and implementation
  sections, imports, visibility, and separate compilation.
- Inline assembly as an explicitly unsafe feature with declared inputs,
  outputs, clobbers, and target constraints.
- Bit fields and explicit alignment/size queries and declarations.

GPU offload and fixed-point `fix<N>` types are excluded from this feature
target, as stated in the supplied Stage 0 and Stage 1 specifications.

## Delivery order

The features depend on one another, so implementation should proceed in
vertical slices with parser, type checker, runtime, code generator, and tests
updated together:

1. **Language foundation:** Pascal-like declarations and unit imports; richer
   type representation; records/unions; reliable diagnostics; separate
   compilation and linking.
2. **Memory and lifetime:** references and raw pointers; stack/heap controls;
   allocators; deterministic cleanup; constructors/destructors; copy/move and
   ownership checks.
3. **Reusable types:** generics and constraints; collections, views, utility
   types, and core algorithms as independent library units.
4. **Object model:** classes, visibility, inheritance, virtual dispatch,
   operator overloads, and RTTI/casts.
5. **System libraries:** I/O/filesystem, OS services, concurrency, atomics, and
   async tasks.
6. **Advanced language facilities:** compile-time programming, coroutines,
   exceptions, advanced generic selection, and target-specific assembly.

Each slice needs executable examples and acceptance tests for both successful
programs and rejected unsafe/ill-typed programs. The current C++ Stage 0
compiler and XeLang Stage 1 compiler will need to evolve together until the
bootstrap compiler can parse and emit the expanded language.

## Current status

The checked-in Stage 0 and Stage 1 compilers currently provide a small subset:
integer and binary64 floating-point values, strings, pointers, fixed arrays,
functions, modules, aliases, control flow, built-in I/O/memory operations,
inline assembly, and regional GC. Stage 0 additionally has value records,
immutable local bindings, deferred scope cleanup, and source-unit imports with
a small collections library. There are no general classes, inheritance,
generics, separate binary library packages, standard containers, threads,
exceptions, RTTI, or coroutines yet. See `compiler/README.md` and the acceptance
tests for the implemented subset and current target limits.

Stage 0 and Stage 1 recognize top-level `uses` declarations and resolve `.xel`
library units, including transitive imports. Stage 0 searches beside the
importing file and in `--library-path` directories. Stage 1 supports
`--library-path` and its built-in `libraries/` directory. These units are
combined into one compilation; separately compiled/reusable binary libraries
are not implemented yet.
