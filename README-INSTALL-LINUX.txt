Xe 2.0.0 for Linux x86-64
=========================

Option 1: prebuilt release tarball (recommended)
------------------------------------------------

    tar xzf xe-2.0.0-linux-x86_64.tar.gz
    cd xe-2.0.0-linux-x86_64
    ./install.sh

Run as a normal user to install into ~/.local, or with sudo to install into
/usr/local. Choose another prefix with:

    PREFIX=/opt/xe ./install.sh

The tarball contains bin/xe, lib/xelang/{std,compiler,libraries}, share/xe
(logo, docs), install.sh and uninstall.sh. The compiler finds its libraries
relative to its own location (../lib/xelang), so the folder can also be used
in place: ./bin/xe works without installing as long as lib/ stays next to it.
To remove Xe: ./uninstall.sh (same PREFIX rules).

Option 2: build from the repository
-----------------------------------

Requirements: GNU as and ld (binutils), gzip, bash.
No C or C++ compiler and no CMake are needed.

    ./install/install.sh

The script assembles bootstrap/xe-seed.s.gz, compiles the compiler with itself
twice, verifies that both generations produce byte-identical assembly (the
self-hosting fixpoint), installs xe and the libraries, and runs tests/run_tests.sh.

Make your own tarball:

    scripts/make-release.sh        # writes dist/xe-2.0.0-linux-x86_64.tar.gz

After changing compiler/*.xe, refresh the seed so fresh checkouts still build:

    scripts/update-seed.sh

Verify
------

    xe --version        # prints the colored logo and "Xe 2.0.0"
    xe --build hello.xe -o hello
    ./hello

Environment: XE_HOME overrides the directory that holds std/ and libraries/.
