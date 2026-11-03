Xe macOS compiler bundle
========================

Requirements: macOS, Xcode Command Line Tools, CMake 3.16+, and clang++.

Unpack the archive, open Terminal in its folder, and run:

    ./install/install-macos.sh

The installer builds the bootstrap compiler, builds the Xe compiler from the
Xe compiler sources, and installs it under $HOME/.local by default. Add
$HOME/.local/bin to PATH to run xe.

Current limitation: Xe's native executable backend targets Linux x86-64. This
bundle installs the compiler CLI on macOS, but it does not yet produce native
macOS application binaries. The C++ bootstrap is used only to build the Xe
compiler itself.
