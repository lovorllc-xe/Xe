Xe Linux installer bundle
=============================

This folder contains the Xe sources, a temporary C++ bootstrap compiler
source, the self-hosted compiler source, standard modules, and source
libraries. It is intended for Linux x86-64 PCs.

Requirements
------------

- CMake 3.16 or newer
- A C++20 compiler (g++ or clang++)
- GNU assembler and linker (as and ld)
- GNU coreutils and the test utilities used by `tests/run_tests.sh`

Install
-------

Copy this entire xelang-linux-installer folder to the Linux PC. In a terminal,
open that folder and run:

    ./install/install.sh

The script builds the C++ bootstrap compiler and uses it to build the
self-hosted compiler. It installs the self-hosted compiler as `xe` in
`$HOME/.local/bin` for a normal user or `/usr/local/bin` when run as root. The
compiler and library files are installed under the matching `lib/xelang`
directory. It runs `tests/run_tests.sh` after installation. Temporary build
files are removed automatically. Use `sudo ./install/install.sh` to install system
wide, or set `PREFIX` to choose another destination.

Verify
------

    xe --version

Build and run a Xe source file:

    xe --build hello.xe -o hello.xee
    ./hello.xee

To install into a different writable location:

    PREFIX="$HOME/.local" ./install/install.sh
    "$HOME/.local/bin/xe" --version

Current implementation notes
-----------------------------

The installed `xe` command is built from the self-hosted compiler sources,
including support for records, fixed arrays, and library imports. It supplies
the installed `.xel` library directory by default. Generated programs include
their runtime directly.

Create macOS and Windows installer bundles
------------------------------------------

Run this on any machine with Python 3:

    python3 scripts/build_platform_installers.py

It creates a macOS `.tar.gz` bundle and a Windows `.zip` bundle under `dist/`.
The bundles build the Xe compiler on their target operating system. See
`README-INSTALL-MACOS.txt` and `README-INSTALL-WINDOWS.txt` inside each bundle
for platform requirements and current backend limitations.
