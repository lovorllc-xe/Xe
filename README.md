# Xe

**Xe 2.2.0** is a statically checked systems and application language that
compiles straight to native executables for Linux x86-64 (and cross-builds for
Windows x86-64; macOS x86-64 builds but is untested). The compiler is written
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

## Platforms

One source builds for every platform; only the `-p` flag changes:

```
xe --build app.xe -o app            # Linux (default)
xe --build app.xe -o app.exe -p win   # Windows x86-64
xe --build app.xe -o app -p macos     # macOS x86-64 (see below)
```

Programs and libraries (including `window`, `canvas`, `gui`, `game`) are identical on every platform; the compiler picks the
platform's `native` layer from `libraries/<platform>/`, and a small runtime layer (`platform/<platform>/`) provides the entry
point and system calls. Cross-building on Linux needs: Windows `llvm-mc`, `llvm-dlltool`, `lld-link`; macOS `llvm-mc`, `ld64.lld`.

| | Linux | Windows | macOS |
|---|---|---|---|
| Console programs, files, memory, GC, threads | yes | yes | builds; **not run on real hardware**; no threads |
| Sockets, HTTP, directories, stat, file mapping | yes | yes (Winsock, FindFirstFile, ...) | no |
| Processes | `process`, `subprocess` | `process.run/spawn/wait` only (no pipes yet) | no |
| epoll, inotify, signals | yes | no (signals: stubs) | no |
| Windows/GUI (`window`) | X11 (default) or Wayland | Win32 | **no native backend yet** (`window.open` returns null) |
| Audio (`audio`) | ALSA | winmm | generators and WAV files only |
| Vulkan (compute, graphics, textures, swapchain) | yes (tested on an Intel Arc GPU) | builds, **not run** | builds, **not run**; no window surface |

Honesty note: the Windows target was exercised during development under Wine (console programs, the garbage collector,
threads, sockets, files, directories, one Win32 window with input events); it has not been tried on real Windows. The
macOS target only produces a binary. `tests/run_windows_tests.sh` re-runs the test suite under Wine if you have it.

## What is new in 2.x

- **Language features (2.2).** `else if`, `break`/`continue`, global variables and constants
  (`global x: int := 0`, `global const N: int := 10`), `enum`, generics (`type Pair<A, B>`, `fn largest<T>`,
  `largest<int>(a, b)`), casts (`cast.flt`, `cast.int`, `cast.ptr`, `cast.addr`), function pointers and closures
  (`func`, `closure`), errors as values (`result`). None of the existing syntax changed. See
  [The language in one screen](#the-language-in-one-screen).
- **Tooling (2.2).** `xe fmt` (formatter), `xe lsp` (language server: diagnostics, formatting, go to definition, completion;
  [docs/editors.md](docs/editors.md)), `xe pkg` (git dependencies in `xe.pkg`), `xe --build -g` (source line numbers for
  gdb), `xe --build -p win|macos` (cross builds).
- **Self-hosted bootstrap.** `scripts/bootstrap.sh` assembles
  `bootstrap/xe-seed.s.gz`, rebuilds the compiler twice and checks that both
  generations emit byte-identical assembly. (The old C++ bootstrap was removed.)
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
- **Graphics, GUI and games.** `canvas` (2D drawing, text, sprites), `image` (PNG, baseline JPEG, BMP, PPM), `window` + `wayland` + `x11`
  (windows, keyboard, mouse over native Wayland or X11, no libwayland or libX11), `gui` (immediate-mode widgets), `gfx3d` (software 3D:
  meshes, matrices, z-buffer, OBJ files), `game` (collisions, tile maps, sprites, particles), and **Vulkan**:
  `vulkan` (any GPU, any platform: compute, graphics, textures, swapchains), `vkwindow`, `vkscene` (GPU-lit 3D
  scenes) and `ffi` (call any C library). See [Graphics](#graphics-gui-and-games).
- **174 libraries** covering containers, math, text, encodings, hashing and
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
tar xzf xe-2.2.0-linux-x86_64.tar.gz
cd xe-2.2.0-linux-x86_64
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
xe --run hello.xe        # build into a temporary file and run
xe --check app.xe        # type-check only
xe --build app.xe -g     # add source line numbers for gdb
xe fmt src/*.xe          # re-indent source files (--check lists the ones that need it)
xe pkg add name URL      # install a git dependency into .xe/packages (listed in xe.pkg)
xe lsp                   # language server (see docs/editors.md)
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

- `:=` assigns, `=` compares; there is no `==` or `for`. `while` is the only loop, with `break` and `continue`.
- `if … { } else if … { } else { }` and `while … { }` for small blocks, `begin … end` for big ones.
- `global counter: int := 0`, `global const LIMIT: int := 100`, `enum Color begin red green := 5 end` (`Color.green`).
- Generics are explicit: `type Pair<A, B>: record ... end`, `fn largest<T>(a: T, b: T): T`, used as `Pair<int, string>` and
  `largest<int>(3, 9)`.
- Casts are functions (`cast.flt(i)`, `cast.int(f)`, `cast.ptr(address)`, `cast.addr(p)`); function pointers are `&name` with
  `func.call1(ptr, x)`; `closure` pairs a function with data; `result` has `ok`/`fail`/`unwrap`/`panic`.
- Types: `int`, `flt`, `string`, `*T`, `[N]T`, records, aliases.
- `alloc`/`free`, `defer`, `gc.begin { } gc.end` regions, `thread.start`/`thread.join`.
- Module functions named `new`, `delete`, `read`, `write` or `free` are shadowed
  by builtins; call them qualified (`intlist.new()`), and the checker warns
  when you declare one.

The full reference is in [docs/documentation.txt](docs/documentation.txt).

## Libraries

174 libraries ship in `libraries/`: containers (`intlist`, `strmap`, `heap`,
`lru`, `trie`, …), numerics (`bigint`, `rational`, `matrix`, `fft`, `stats`),
text (`regex`, `fmt`, `csv`, `markdown`, `template`), encodings and crypto
(`base64`, `sha256`, `hmac`, `chacha20`, `uuid`), compression (`inflate`,
`lzss`, `huffman`), OS (`path`, `dir`, `subprocess`, `epoll`, `inotify`,
`mmapfile`), concurrency (`mutex`, `channel`, `waitgroup`, `parallel`),
networking (`tcp`, `udp`, `dns`, `http`, `httpserver`) and more. Import with
`uses name`. The index is [docs/libraries.md](docs/libraries.md).

## Graphics, GUI and games

```sh
xe examples/gui_demo.xe && ./examples/gui_demo        # widgets: buttons, sliders, text box, list
xe examples/cube3d.xe && ./examples/cube3d            # software 3D, mouse orbit
xe examples/pong.xe && ./examples/pong                # 2D game
xe examples/vulkan_3d.xe && ./examples/vulkan_3d      # the same kind of scene rendered by the GPU through Vulkan
```

```xe
uses gui

fn main(): int
begin
    var win: *window.Window := window.open("hello", 320, 200)
    var ui: *gui.Gui := gui.new(win)
    var clicks: int := 0
    while window.is_open(win) {
        window.poll(win)                       // events -> mouse, keyboard
        gui.start(ui)
        if gui.button(ui, 20, 20, 140, 30, "clicked " + str.number(clicks)) { clicks := clicks + 1 }
        gui.finish(ui)                         // draw + present
        window.limit_fps(win, 60)
    }
    return 0
end
```

- **2D:** `canvas` draws pixels, lines, rectangles, circles, triangles, text (8x14 bitmap font), alpha
  blending and sprites; `image` loads/saves PNG, BMP and PPM; `game` adds collisions, tile maps, animation,
  particles and a fixed-timestep helper.
- **3D:** `gfx3d` is a complete software renderer (vectors, 4x4 matrices, perspective camera, z-buffer, flat
  shading, wireframes, cube/sphere/torus/plane generators, `.obj` loader).
- **GUI:** `gui` is immediate-mode: widgets are function calls (`button`, `checkbox`, `radio`, `slider`,
  `textbox`, `list`, `progress`, `panel`, auto-layout variants).
- **Vulkan:** `vulkan` works on every platform and GPU. The Vulkan loader is opened at run time (`libvulkan.so.1`,
  `vulkan-1.dll`, or `libvulkan.1.dylib` / MoltenVK), so programs build without a Vulkan SDK and `vulkan.open` simply
  returns null where Vulkan is missing. Nothing is assumed about the GPU: the device (discrete, integrated, virtual,
  software; `XE_VK_DEVICE` overrides), queue family, memory types, depth format, swapchain format and present mode are
  chosen from what the driver reports, and MoltenVK's portability extensions are enabled. It covers buffers, SPIR-V
  shader modules, compute (`compute_new/bind/run`), graphics (render passes, pipelines with uniform buffers,
  textures with optional mipmaps and blending, vertex/index buffers, push constants, dynamic viewport),
  synchronization with fences/semaphores, offscreen targets with read-back, and **window swapchains**:
  `vkwindow.new(vk, win, depth, vsync)` presents straight into a `window` on Linux (X11/XWayland) and Windows, and
  falls back to rendering offscreen and copying where a surface is not available (macOS, native Wayland windows).
  `vkscene` gives ready-made lit 3D on the GPU. Compile shaders with `glslc`. `XE_VK_VALIDATION=1` turns on the
  Khronos validation layer when installed. Verified here only on an Intel Arc GPU under Linux (compute, graphics,
  textures, uniforms, swapchain with resize, and the offscreen fallback); the Windows and macOS paths build but were
  not run, and other vendors' GPUs were not available to test.
- Windows use X11 by default on Linux (also on Wayland sessions, through XWayland); `XE_BACKEND=wayland` or `window.open_wayland` selects native Wayland, and Wayland is used automatically if X11 is unavailable. Wayland keyboard input assumes a US layout, and GNOME gives Wayland windows no title bar. 
- **Audio:** `audio` plays 16-bit sound (`audio.open`, `audio.play`, tone/square/noise generators, a mixer) through ALSA on Linux and winmm on Windows, and reads/writes WAV files everywhere.

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
XE=$PWD/build/xe XE_HOME=$PWD tests/run_tests.sh   # 20 programs; every one must build and exit 0
```

## Repository layout

- `compiler/` — the self-hosted compiler (`SOURCES` lists the build order).
- `bootstrap/xe-seed.s.gz` — assembly seed; refresh with `scripts/update-seed.sh`.
- `std/`, `libraries/` — the automatic `io`/`mem` modules and the `.xel` libraries (`libraries/linux|win|macos/` hold the per-platform layers).
- `platform/` — per-platform runtime layer (entry point and system-call emulation for Windows and macOS).
- `tests/` — integration programs and the runner.
- `benchmarks/` — kernels, generator, runner and GUI.
- `logo/` — `png/` source logo and `ascii/xe.ans` colored ASCII art.
- `install/`, `scripts/` — installers, bootstrap, seed and release tooling.
- `docs/` — documentation, library index, editor setup.
- `vscode-xe/`, `zed-xe/` — syntax highlighting for editors.

## Limits

- **Targets:** Linux x86-64 is complete and tested. Windows x86-64 cross-builds and was exercised under Wine only (see the
  Platforms table for what is missing there). macOS x86-64 builds a binary that has not been run, and has no window or
  thread support. There is no ARM64 back end (on Apple Silicon the macOS binary would need Rosetta).
- **Language:** generics are instantiated copies with explicit type arguments (top-level only); globals are reached
  through a hidden pointer (slightly slower than locals); `break`/`continue` are not allowed in a loop containing `defer`
  or `gc.begin`; function pointers carry int/pointer/string arguments only.
- **Runtime:** HTTP is plain `http://` (no TLS). The garbage collector is conservative and does not run while threads are
  active; free memory explicitly or use regions in threaded code.
- **Graphics:** no macOS window backend; native Wayland windows cannot present Vulkan frames (they use the offscreen
  fallback); Wayland keyboard input assumes a US layout.
- **Debugging:** `-g` gives line-level gdb support, not variable descriptions.
