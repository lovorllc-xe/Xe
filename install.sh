#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${PREFIX:-}" ]]; then
    prefix="$PREFIX"
elif (( EUID == 0 )); then
    prefix="/usr/local"
else
    prefix="${HOME:?HOME must be set for a user install}/.local"
fi
if [[ "$prefix" != /* ]]; then prefix="$PWD/$prefix"; fi
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
stage1="$work_dir/selfhosted-xe"
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
"$stage1" --version

install -d "$bin_dir" "$lib_dir/std" "$lib_dir/runtime" "$lib_dir/compiler" "$lib_dir/libraries"
install -m 0755 "$stage1" "$lib_dir/compiler/selfhosted-xe"
install -m 0644 "$project_dir"/std/*.xe "$lib_dir/std/"
install -m 0644 "$project_dir"/compiler/*.xe "$lib_dir/compiler/"
install -m 0644 "$project_dir"/libraries/*.xel "$lib_dir/libraries/"

cat > "$work_dir/xe-wrapper" <<EOF
#!/usr/bin/env bash
set -euo pipefail
compiler="$lib_dir/compiler/selfhosted-xe"
arguments=("\$@")
run_build() {
    local diagnostics started finished elapsed duration status warnings errors
    diagnostics=\$(mktemp)
    started=\$(date +%s%N)
    if "\$@" 2>"\$diagnostics"; then status=0; else status=\$?; fi
    finished=\$(date +%s%N)
    cat "\$diagnostics" >&2
    warnings=\$(grep -c 'warning:' "\$diagnostics" || true)
    errors=\$(grep -c 'error:' "\$diagnostics" || true)
    if (( status != 0 && errors == 0 )); then errors=1; fi
    rm -f -- "\$diagnostics"
    elapsed=\$((finished - started))
    if (( elapsed < 1000 )); then duration="\${elapsed} ns"
    elif (( elapsed < 1000000 )); then duration=\$(awk -v n="\$elapsed" 'BEGIN { printf "%.2f µs", n / 1000 }')
    elif (( elapsed < 1000000000 )); then duration=\$(awk -v n="\$elapsed" 'BEGIN { printf "%.2f ms", n / 1000000 }')
    elif (( elapsed < 60000000000 )); then duration=\$(awk -v n="\$elapsed" 'BEGIN { printf "%.2f s", n / 1000000000 }')
    elif (( elapsed < 3600000000000 )); then duration=\$(awk -v n="\$elapsed" 'BEGIN { printf "%.2f min", n / 60000000000 }')
    else duration=\$(awk -v n="\$elapsed" 'BEGIN { printf "%.2f h", n / 3600000000000 }')
    fi
    if (( status == 0 )); then result=succeeded; else result=failed; fi
    printf '\\nBuild summary\\n  Result:   %s\\n  Warnings: %s\\n  Errors:   %s\\n  Time:     %s\\n' "\$result" "\$warnings" "\$errors" "\$duration"
    return "\$status"
}
has_library_path=0
index=0
for ((index = 0; index < \${#arguments[@]}; index += 1)); do
    [[ "\${arguments[index]}" == -- ]] && break
    if [[ "\${arguments[index]}" == --library-path ]]; then
        has_library_path=1
        break
    fi
done
with_library_path=()
if (( has_library_path )); then
    with_library_path=("\${arguments[@]}")
else
    inserted=0
    for argument in "\${arguments[@]}"; do
        if (( inserted == 0 )) && [[ "\$argument" == -- || "\$argument" == -o ]]; then
            with_library_path+=(--library-path "$lib_dir/libraries")
            inserted=1
        fi
        with_library_path+=("\$argument")
    done
    if (( inserted == 0 )); then with_library_path+=(--library-path "$lib_dir/libraries"); fi
fi
case "\${with_library_path[0]:-}" in
    --build|--build-nasm|--emit-asm|--emit-nasm)
        with_std=()
        output_index=-1
        for ((index = 1; index < \${#with_library_path[@]}; index += 1)); do
            if [[ "\${with_library_path[index]}" == -o ]]; then output_index=\$index; break; fi
        done
        if (( output_index >= 0 )); then
            with_std+=("\${with_library_path[@]:0:output_index}")
            with_std+=("$lib_dir/std/io.xe" "$lib_dir/std/mem.xe")
            with_std+=("\${with_library_path[@]:output_index}")
        else
            with_std=("\${with_library_path[@]}")
            with_std+=("$lib_dir/std/io.xe" "$lib_dir/std/mem.xe")
        fi
        run_build "\$compiler" "\${with_std[@]}"
        ;;
    --build-pe|--build-baremetal|--emit-pe|--emit-baremetal|--emit-baremetal-nasm)
        run_build "\$compiler" "\${with_library_path[@]}"
        ;;
    *) exec "\$compiler" "\${with_library_path[@]}" ;;
esac
EOF
install -m 0755 "$work_dir/xe-wrapper" "$bin_dir/xe"

rm -rf -- "$work_dir/build"

if XE="$bin_dir/xe" "$project_dir/tests/run_tests.sh"; then
    printf 'Xe installed and all integration tests passed.\n'
else
    status=$?
    printf 'Xe was installed, but its post-install tests failed. The new installation remains at %s; review the failed test output.\n' "$prefix" >&2
    exit "$status"
fi

"$bin_dir/xe" --version
