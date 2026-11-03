#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
    printf '%s\n' 'Run this installer on macOS.' >&2
    exit 1
fi

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${PREFIX:-${HOME:?HOME must be set}/.local}"
[[ "$prefix" == /* ]] || prefix="$PWD/$prefix"
bin_dir="$prefix/bin"
lib_dir="$prefix/lib/xelang"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/xe-macos-install.XXXXXX")"
trap 'rm -rf -- "$work_dir"' EXIT

for tool in cmake clang++ tar; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required tool not found: %s\n' "$tool" >&2
        exit 1
    fi
done

install -d "$lib_dir/runtime" "$lib_dir/std" "$lib_dir/compiler" "$lib_dir/libraries" "$bin_dir"
install -m 0644 "$project_dir/src/runtime.hpp" "$lib_dir/runtime/runtime.hpp"
install -m 0644 "$project_dir"/std/*.xe "$lib_dir/std/"
install -m 0644 "$project_dir"/compiler/*.xe "$lib_dir/compiler/"
install -m 0644 "$project_dir"/libraries/*.xel "$lib_dir/libraries/"

cmake -S "$project_dir" -B "$work_dir/build" \
    -DBUILD_TESTING=OFF \
    -DXELANG_RUNTIME_DIR="$lib_dir/runtime" \
    -DXELANG_STDLIB_DIR="$lib_dir/std" \
    -DCMAKE_CXX_COMPILER=clang++
cmake --build "$work_dir/build" --target xelang --parallel "$(sysctl -n hw.ncpu)"

"$work_dir/build/xe" --build-bootstrap \
    "$project_dir/compiler/token.xe" \
    "$project_dir/compiler/lexer.xe" \
    "$project_dir/compiler/ast.xe" \
    "$project_dir/compiler/parser.xe" \
    "$project_dir/compiler/checker.xe" \
    "$project_dir/compiler/codegen.xe" \
    "$project_dir/compiler/optimizer.xe" \
    "$project_dir/compiler/runtime.xe" \
    "$project_dir/compiler/main.xe" \
    -o "$work_dir/xe"

install -m 0755 "$work_dir/xe" "$lib_dir/compiler/selfhosted-xe"
cat > "$work_dir/xe-wrapper" <<EOF
#!/usr/bin/env bash
set -euo pipefail
script_dir="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd)"
exec "\$script_dir/../lib/xelang/compiler/selfhosted-xe" "\$@"
EOF
install -m 0755 "$work_dir/xe-wrapper" "$bin_dir/xe"

printf 'Installed Xe compiler: '
"$bin_dir/xe" --version
printf 'Add %s to PATH to use xe.\n' "$bin_dir"
printf '%s\n' 'Note: Xe currently has no macOS native executable backend; this installs the compiler CLI, but generated Linux assembly programs will not run as macOS apps.' >&2
