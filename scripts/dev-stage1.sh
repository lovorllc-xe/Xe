#!/usr/bin/env bash
# Developer helper: build the C++ bootstrap and use it to build the stage1 Xe compiler.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="${XE_DEV_DIR:-/tmp/xedev}"
mkdir -p "$out"
cmake -S "$root" -B "$out/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$out/build" --parallel >/dev/null
cd "$root/compiler"
"$out/build/xe" --build-bootstrap token.xe lexer.xe ast.xe memo.xe diag.xe parser.xe checker.xe codegen.xe optimizer.xe runtime.xe memory.xe main.xe -o "$out/stage1"
