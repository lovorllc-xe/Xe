# Xe

**Xe 1.7.0** is a statically checked systems and application language with a
self-hosted compiler. Its primary native executable target is Linux x86-64.
Xe source files use `.xe`; importable source libraries use `.xel`.

The compiler supports functions, modules, records, fixed-size arrays and
slices, pointers, explicit allocation, scoped allocation regions, and native
code generation. Its libraries include collections, JSON, math, file and
process helpers, terminal output, and basic threading. See the current limits
below before choosing Xe for a platform-dependent project.

## Install

On Linux x86-64, install from the repository:

```sh
./install/install.sh
xe --version
```

The installer needs CMake, a C++20 compiler, GNU `as`, and `ld`. It builds a
small bootstrap compiler, builds the self-hosted Xe compiler, and installs the
`xe` command and libraries. It uses `$HOME/.local` for a user installation or
`/usr/local` when run as root. To select a prefix explicitly:

```sh
PREFIX="$HOME/.local" ./install/install.sh
```

See [Linux installation](README-INSTALL-LINUX.txt), [macOS installation](README-INSTALL-MACOS.txt),
and [Windows installation](README-INSTALL-WINDOWS.txt) for platform-specific
requirements and limitations.

## Build and run a program

Create `hello.xe`:

```xe
fn main(): int
begin
    var name: string := "Xe"
    writeln("Hello, ", name, "!")
    return 0
end
```

Build a native executable and run it:

```sh
xe --build hello.xe -o hello.xee
./hello.xee
```

`write` and `writeln` accept multiple values. `writeln` adds one newline.
Use `:=` for assignment and `=` for equality. Conditional expressions use
parentheses:

```xe
if (score >= 10) {
    writeln("passed")
} else {
    writeln("try again")
}
```

Import a `.xel` library with `uses` at the top of a source file:

```xe
uses math

fn main(): int
begin
    writeln(math.sqrt(81.0))
    return 0
end
```

Xe loads `.xel` source libraries into the current build. The `io` and `mem`
modules are included automatically. To use a library directory outside the
default search location, pass `--library-path`:

```sh
xe --build app.xe --library-path ./libraries -o app.xee
```

See `xe --help` for assembly, NASM, PE, and bare-metal output modes.

## Build the compiler from source

```sh
cmake -S . -B build
cmake --build build
./build/xe --version
```

This CMake build produces the bootstrap compiler. The install script also
builds the self-hosted compiler from the Xe sources.

## Tests and release bundles

Run the integration suite with an installed compiler on `PATH`:

```sh
./tests/run_tests.sh
```

Create a Linux x86-64 source release bundle:

```sh
./scripts/build_release.sh
```

Create the macOS and Windows source installer bundles with Python 3:

```sh
python3 scripts/build_platform_installers.py
```

## Repository layout

- `compiler/` — self-hosted lexer, parser, checker, optimizer, code generator,
  runtime, and command-line driver.
- `src/` — C++20 bootstrap compiler and host runtime.
- `std/` — automatically included `io` and `mem` modules.
- `libraries/` — importable `.xel` libraries.
- `tests/` — Xe integration examples and test runner.
- `install/` — Linux, macOS, and Windows installer scripts.
- `vscode-xe/` — syntax highlighting, language configuration, and snippets.
- `dist/` — generated release bundles.

## Current platform and library limits

Linux x86-64 is the complete general-purpose executable target. The macOS
bundle installs the compiler CLI, but Xe does not yet generate native macOS
applications. Windows PE output currently supports only a freestanding subset
and requires NASM and LLVM linker tools.

Several libraries wrap Linux syscalls. The JSON module supports JSON values
but does not decode Unicode escapes into Unicode text. The collections map is
limited to 128 entries, and the integer vector and stack have fixed capacity.
CRC-32 is an error-detection checksum, not secure cryptography. The current
libraries do not include a database driver, HTTP framework, TLS, GUI, or async
runtime.
