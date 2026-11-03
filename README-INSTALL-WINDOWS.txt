Xe Windows compiler bundle
===========================

Requirements: Windows x64, CMake 3.16+, and Visual Studio C++ Build Tools
with the C++20 workload. Open a Visual Studio Developer PowerShell in the
unpacked folder and run:

    .\install\install-windows.ps1

By default, the installer places Xe under
%LOCALAPPDATA%\Programs\Xe. Add its bin directory to PATH to run xe.cmd.
Use `xe --build-pe` for Windows PE executables. That target also requires
NASM, llvm-dlltool, and lld-link on PATH, and currently supports only
freestanding programs without runtime I/O, allocation, or garbage collection.

The C++ bootstrap is used only to build the Xe compiler itself; it does not
translate user Xe programs to C++.
