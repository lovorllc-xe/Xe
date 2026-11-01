Xe Linux installer bundle
=============================

This folder contains the Xe sources, a temporary C++ bootstrap compiler
source, the self-hosted compiler source, runtime files, standard modules, and
source libraries. It is intended for Linux x86-64 PCs.

Requirements
------------

- CMake 3.16 or newer
- A C++20 compiler (g++ or clang++)
- GNU assembler and linker (as and ld)
- Permission to install into /usr/local (normally use sudo)

Install
-------

Copy this entire xelang-linux-installer folder to the Linux PC. In a terminal,
open that folder and run:

    cd xelang-linux-installer
    sudo ./install.sh

The script builds and checks the compiler, then bootstraps and rebuilds the
self-hosted compiler. It installs the self-hosted compiler as `xe` in
/usr/local/bin, with its compiler and library files under /usr/local/lib/xelang.
The C++ bootstrap compiler stays in a temporary directory and is removed when
installation finishes.

Verify
------

    xe --version

Build and run a Xe source file:

    xe --build hello.xe -o hello.xee
    ./hello.xee

To install into a writable test location instead of /usr/local:

    PREFIX="$HOME/.local" ./install.sh
    "$HOME/.local/bin/xe" --version

Current implementation notes
-----------------------------

The installed `xe` command launches the self-hosted compiler, including support
for records, fixed arrays, and library imports. It supplies the installed
`.xel` library directory by default. The separate `runtime.s` is provided as
an assembly runtime source; generated programs currently include their runtime
directly.
