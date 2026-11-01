#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
prefix="${PREFIX:-/usr/local}"
bin_dir="$prefix/bin"
lib_dir="$prefix/lib/xelang"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/xelang-install.XXXXXX")"
trap 'rm -rf -- "$work_dir"' EXIT

if ! command -v cmake >/dev/null 2>&1 || ! command -v c++ >/dev/null 2>&1; then
    printf '%s\n' 'Xe installation requires cmake and a C++20 compiler.' >&2
    exit 1
fi

install -d "$lib_dir/runtime"
install -m 0644 "$project_dir/src/runtime.hpp" "$lib_dir/runtime/runtime.hpp"

cmake -S "$project_dir" -B "$work_dir/build" -DBUILD_TESTING=ON -DXELANG_RUNTIME_DIR="$lib_dir/runtime"
cmake --build "$work_dir/build" --target xelang --parallel 14
ctest --test-dir "$work_dir/build" --output-on-failure --parallel 2

host_xe="$work_dir/build/xe"
stage1="$work_dir/selfhosted-stage1"
stage2="$work_dir/selfhosted-stage2"
stage3="$work_dir/selfhosted-stage3"
compiler_sources=(
    "$project_dir/compiler/token.xe"
    "$project_dir/compiler/lexer.xe"
    "$project_dir/compiler/ast.xe"
    "$project_dir/compiler/parser.xe"
    "$project_dir/compiler/checker.xe"
    "$project_dir/compiler/codegen.xe"
    "$project_dir/compiler/optimizer.xe"
    "$project_dir/compiler/runtime.xe"
    "$project_dir/compiler/main.xe"
)

"$host_xe" --build \
    "${compiler_sources[@]}" \
    -o "$stage1"
"$stage1" --build "${compiler_sources[@]}" -o "$stage2"
"$stage2" --build "${compiler_sources[@]}" -o "$stage3"
"$stage3" --version

guessing_game="$work_dir/guessing-game.xee"
"$stage3" --build "$project_dir/guessing_game.xe" -o "$guessing_game"
"$guessing_game" < "$project_dir/tests/stage1/guessing-game-input.txt" > "$work_dir/guessing-game.out"
grep -Fq 'Congratulations! You won in 3 attempts.' "$work_dir/guessing-game.out"

install -d "$bin_dir" "$lib_dir/std" "$lib_dir/runtime" "$lib_dir/compiler" "$lib_dir/libraries"
install -m 0755 "$stage3" "$lib_dir/compiler/selfhosted-xe"
install -m 0644 "$project_dir/runtime/runtime.s" "$lib_dir/runtime/runtime.s"
install -m 0644 "$project_dir"/std/*.xe "$lib_dir/std/"
install -m 0644 "$project_dir"/compiler/*.xe "$lib_dir/compiler/"
install -m 0644 "$project_dir"/libraries/*.xel "$lib_dir/libraries/"

cat > "$bin_dir/xe" <<EOF
#!/usr/bin/env bash
set -euo pipefail
compiler="$lib_dir/compiler/selfhosted-xe"
arguments=("\$@")
for ((index = 0; index < \${#arguments[@]}; index += 1)); do
    [[ "\${arguments[index]}" == -- ]] && break
    if [[ "\${arguments[index]}" == --library-path ]]; then
        exec "\$compiler" "\${arguments[@]}"
    fi
done
last=\$((\${#arguments[@]} - 2))
if (( last >= 0 )) && [[ "\${arguments[last]}" == -o ]]; then
    exec "\$compiler" "\${arguments[@]:0:last}" --library-path "$lib_dir/libraries" "\${arguments[@]:last}"
fi
with_library_path=()
inserted=0
for argument in "\${arguments[@]}"; do
    if (( inserted == 0 )) && [[ "\$argument" == -- ]]; then
        with_library_path+=(--library-path "$lib_dir/libraries")
        inserted=1
    fi
    with_library_path+=("\$argument")
done
if (( inserted == 0 )); then with_library_path+=(--library-path "$lib_dir/libraries"); fi
exec "\$compiler" "\${with_library_path[@]}"
EOF
chmod 0755 "$bin_dir/xe"

"$bin_dir/xe" --version
