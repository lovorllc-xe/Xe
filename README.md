# Xe

Xe is a statically checked language that compiles straight to native executables. It targets Linux x86-64 and
cross-builds for Windows and macOS. The compiler is written in Xe and **builds itself**: a checked-in assembly seed plus
GNU `as`, `ld` and `gzip` are all it needs, with no C or C++ compiler involved.

The syntax is Pascal-like: `begin … end` for big blocks (functions, modules, records), `{ … }` for small ones (`if`,
`while`, `for`), `:=` to assign and `=` to compare.

```xe
uses strings

fn main(): int
begin
    for i := 1 to 3 {
        writeln(i, " ", strings.to_upper("hello from xe"))
    }
    return 0
end
```

Current version: **2.3.0**.

## Install

**From a release package** (no compiler needed):

```sh
tar xzf xe-2.3.0-linux-x86_64.tar.gz
cd xe-2.3.0-linux-x86_64
./install.sh                 # installs into ~/.local   (sudo ./install.sh → /usr/local)
xe --version
```

`PREFIX=/opt/xe ./install.sh` installs elsewhere, and `./uninstall.sh` removes it.

**From this repository:**

```sh
./install/install.sh
```

This bootstraps the compiler from the seed, checks that it reproduces itself exactly, installs `xe` with its libraries,
and runs the test suite. Run it again after pulling changes so the installed libraries stay in step with the compiler.

## Build and run

```sh
xe hello.xe              # builds ./hello
xe --build app.xe -o app # same, with an explicit output name
xe --run app.xe          # build into a temporary file and run
xe --check app.xe        # type-check only (list every file of a multi-file program)
xe --build app.xe -g     # debug info for gdb: lines, arguments, locals, record fields
xe fmt src/*.xe          # re-indent source files (--check only lists them)
xe pkg add name URL      # add a git dependency (listed in xe.pkg, installed in .xe/packages)
xe lsp                   # language server for editors
xe --emit-asm app.xe     # show the generated assembly
xe --help                # everything else, including freestanding PE and bare-metal output
```

A program starts at `fn main(): int`; its result is the exit status. `uses name` imports `name.xel`, looked up next to
the importing file, then in `--library-path` folders, `.xe/packages`, and the installed libraries.

Errors point at the exact spot and say what was expected:

```
app.xe:4:13: error: undefined name 'totl'
 4 |     writeln(totl + 1)
   |             ^
```

## The language in one screen

```xe
uses intlist

type Point: record
begin
    x: int
    y: int
end

global const LIMIT: int := 5

enum Color
begin
    red
    green := 5
end

fn length2(p: *Point): int
begin
    return p.x * p.x + p.y * p.y        // fields auto-dereference through pointers
end

fn largest<T>(a: T, b: T): T            // generics take explicit type arguments
begin
    if a > b { return a }
    return b
end

fn main(): int
begin
    var p: *Point := alloc(sizeof(Point))
    defer free(p)                       // runs when the function returns
    p.x := 3
    p.y := 4
    var scores: *intlist.List := intlist.new()
    for i := 0 to LIMIT - 1 {
        if i = 2 { continue }
        intlist.push(scores, i * length2(p))
    }
    if intlist.len(scores) = 4 {
        writeln("ok ", largest<int>(3, 9), " ", Color.green)
    } else if p = null {
        writeln("no point")
    } else {
        writeln("bad")
    }
    return 0
end
```

- **Types:** `int` (64-bit), `flt` (64-bit float), `string`, pointers `*T`, fixed arrays `[N]T`, records and aliases.
- **Literals:** `0xFF`, `0b1010`, `1_000_000`, `2.5e-3`, `'a'` (an int), `null`, `sizeof(T)`.
- **Control flow:** `if`/`else if`/`else`, `while`, `for i := a to b` (also `downto` and `step`; bounds are inclusive),
  `break`, `continue`, `switch … case … default … end`, `goto label`.
- **Memory:** `alloc`/`free`, `defer`, a conservative garbage collector, and `gc.begin { } gc.end` regions that free
  everything allocated inside them at once.
- **Program structure:** `module name begin … end`, `global` variables and `global const` constants, `enum`, generics
  (`type Pair<A, B>`, `fn largest<T>`).
- **Escape hatches:** casts are functions (`cast.flt`, `cast.int`, `cast.ptr`, `cast.addr`); `&name` is a function
  pointer (`func`, `closure`); `ffi` calls C libraries and accepts callbacks; `asm { }` embeds assembly.
- **Threads:** `thread.start(&worker, arg)` and `thread.join(id)`.
- `==` is an error, and `"text" + 5` is too (write `"text" + str.number(5)`, or pass both to `writeln`).

The full reference is [docs/documentation.txt](docs/documentation.txt).

## Libraries

176 libraries ship in `libraries/`:
- **Containers:** `intlist`, `strmap`, `heap`, `lru`, `trie`, …
- **Numerics:** `bigint`, `rational`, `matrix`, `fft`, `stats`
- **Text:** `regex`, `fmt`, `csv`, `json`, `markdown`, `template`
- **Encodings and crypto:** `base64`, `sha256`, `hmac`, `chacha20`, `uuid`
- **Compression:** `inflate`, `lzss`, `huffman`
- **OS:** `path`, `dir`, `subprocess`, `epoll`, `inotify`, `mmapfile`
- **Concurrency:** `mutex`, `channel`, `waitgroup`, `parallel`
- **Networking:** `tcp`, `udp`, `dns`, `http`, `httpserver`

The full list with descriptions is [docs/libraries.md](docs/libraries.md).

## Graphics, GUI, audio

```xe
uses gui

fn main(): int
begin
    var win: *window.Window := window.open("hello", 320, 200)
    var ui: *gui.Gui := gui.new(win)
    var clicks: int := 0
    while window.is_open(win) {
        window.poll(win)                       // mouse and keyboard events
        gui.start(ui)
        if gui.button(ui, 20, 20, 140, 30, "clicked " + str.number(clicks)) { clicks := clicks + 1 }
        gui.finish(ui)                         // draw and present
        window.limit_fps(win, 60)
    }
    return 0
end
```

- **Windows:** `window` opens a window with keyboard and mouse input. It uses X11 on Linux by default (through XWayland
  on Wayland sessions); `XE_BACKEND=wayland` or `window.open_wayland` uses native Wayland, which follows the
  compositor's keyboard layout. On Windows it uses Win32. No libX11 or libwayland is needed.
- **2D:** `canvas` (pixels, lines, shapes, sprites, alpha blending, a small bitmap font), `truetype` (anti-aliased
  `.ttf` text of any size), `image` (PNG, baseline JPEG, BMP, PPM), `game` (collisions, tile maps, animation, particles).
- **GUI:** `gui` is immediate-mode: every widget is a function call (`button`, `checkbox`, `radio`, `slider`, `textbox`,
  `list`, `progress`, `panel`).
- **3D in software:** `gfx3d` (matrices, perspective camera, z-buffer, shading, mesh generators, `.obj` loader).
- **Vulkan:** `vulkan` loads the Vulkan driver at run time, so no SDK is needed to build, and `vulkan.open` returns null
  where Vulkan is missing. It picks the device, memory types and formats from what the driver reports. It covers
  compute, graphics pipelines, textures, uniforms, offscreen rendering and window swapchains (`vkwindow`); `vkscene`
  draws lit 3D meshes. Upload static vertex data with `vulkan.buffer_device` + `buffer_upload`; `buffer_new` is host
  memory, which the GPU reads slowly. Compile shaders with `glslc`. `XE_VK_VALIDATION=1` turns on validation layers.
- **Audio:** `audio` plays 16-bit sound through ALSA (Linux) or winmm (Windows), with tone/noise generators, a mixer, and
  WAV files.

## Platforms

The same source builds for every platform; only `-p` changes:

```sh
xe --build app.xe -o app              # Linux x86-64 (default)
xe --build app.xe -o app.exe -p win   # Windows x86-64
xe --build app.xe -o app -p macos     # macOS x86-64
```

Cross-building on Linux needs the LLVM tools: `llvm-mc`, `llvm-dlltool` and `lld-link` for Windows; `llvm-mc` and
`ld64.lld` for macOS. Platform differences live in `libraries/<platform>/` and `platform/<platform>/`, so programs
don't need `if windows` code.

| | Linux | Windows | macOS |
|---|---|---|---|
| Console programs, files, memory, GC | yes | yes | builds, untested |
| Threads | yes | yes | no |
| Sockets, HTTP, directories | yes | yes | no |
| Processes | `process`, `subprocess` | `process.run/spawn/wait` (no pipes) | no |
| epoll, inotify, signals | yes | no | no |
| Windows and GUI | X11 or Wayland | Win32 | no window backend yet |
| Audio playback | ALSA | winmm | no (WAV files only) |
| Vulkan | yes (tested on Intel Arc) | builds, untested | builds, untested |

Linux is the tested platform. The Windows build has run console programs, threads, sockets, files and a Win32 window,
but not on real Windows hardware. The macOS build produces a binary that has never been run. There is no ARM64 code
generator: the ARM release packages contain x86-64 programs for Windows-on-ARM's x64 emulation and for Rosetta 2.

## Editors

`xe lsp` gives any editor diagnostics, formatting, go to definition and completion. Setup for Zed, VS Code and others is
in [docs/editors.md](docs/editors.md). `Extensions/zed-xe/` and `Extensions/vscode-xe/` add syntax highlighting.

## Working on Xe

```sh
scripts/bootstrap.sh                               # build build/xe from the seed and check the fixpoint
XE=$PWD/build/xe XE_HOME=$PWD tests/run_tests.sh   # 21 test programs; each must build and exit 0
scripts/update-seed.sh                             # after changing compiler/*.xe: refresh the seed
xe --run tools/release.xe                          # build release packages for every platform into dist/
```

The compiler sources must only use syntax the seed already understands. Commit the compiler change and refresh the seed
before using new syntax in `compiler/*.xe`.

| Folder | Contents |
|---|---|
| `compiler/` | the self-hosted compiler (`SOURCES` lists the files in build order) |
| `bootstrap/` | `xe-seed.s.gz`, the assembly the first build starts from |
| `std/`, `libraries/` | the automatic `io`/`mem` modules and the `.xel` libraries (per-platform pieces in `libraries/linux`, `win`, `macos`) |
| `platform/` | entry point and system-call layer for Windows and macOS |
| `tests/` | test programs and the runner |
| `docs/` | language reference, library index, editor setup |
| `install/`, `scripts/`, `tools/` | installer, bootstrap and seed scripts, release tool |
| `Extensions/` | Zed and VS Code extensions |
| `logo/` | logo image and the ASCII art shown by `xe --version` |

## Limits

- **Targets:** only Linux x86-64 is fully tested (see [Platforms](#platforms)). There is no ARM64 back end.
- **Language:** generics need explicit type arguments and are top-level only. `goto` is not allowed in functions that
  use `defer` or `gc.begin`. Function pointers and FFI callbacks pass int, pointer and string arguments only. Globals
  are slightly slower than locals.
- **Runtime:** HTTP is plain `http://` (no TLS). The garbage collector is conservative. On Linux it briefly pauses other
  threads while collecting (so `poll`/`epoll_wait` may return early with EINTR); on Windows it waits until worker
  threads finish.
- **Graphics:** no macOS window backend; native Wayland windows show Vulkan output through an offscreen copy; TrueType
  text has no kerning, hinting or `.otf` (CFF) support.

## Changes

**2.3**
- Fixed: rbx register corruption around global variables.
- New syntax:
  - `for` loops;
  - hex, binary and character literals, `_` separators and exponents;
  - `null`, `sizeof`, `goto`;
  - calls spanning several lines.
- `break`/`continue` work with `defer` and `gc.begin`; `var f: flt := 3` converts the int.
- Module functions may be named `read`, `write`, `free`, `new` or `delete`.
- The garbage collector runs while threads are alive, and `gc.begin` regions are per thread.
- `-g` shows arguments, locals and record fields in gdb.
- Errors name the construct they reject.
- New libraries: `ffi.callback`, `truetype`, `xkbmap` (Wayland keyboard layouts).
- New calls: `bytes.read_file`/`write_file`, `vulkan.buffer_device`/`buffer_upload`.
- The language server checks multi-file programs listed in a `SOURCES` file.
- New release tool: `tools/release.xe`.

**2.2**
- `else if`, `break`/`continue`, globals and constants, `enum`, generics, casts, function pointers, closures, `result`.
- Tools: `xe fmt`, `xe lsp`, `xe pkg`, `-g`, and cross builds with `-p`.

**2.0–2.1**
- The compiler became self-hosted, bootstrapping from an assembly seed instead of the old C++ compiler.
- Precise error messages.
- New allocator and garbage collector.
- Auto-dereferencing pointers.
- Graphics, GUI, audio and Vulkan libraries.
