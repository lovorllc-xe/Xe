#!/usr/bin/env bash
# Builds the tests for Windows (xe --build ... -p win) and runs them under Wine, comparing their output with
# the Linux build's. Needs llvm-mc, llvm-dlltool, lld-link and wine. Tests that exercise Linux-only features
# (sockets, epoll, inotify, file mappings, Vulkan, directory walking) are skipped.
set -u
script_dir="$(cd "$(dirname "$0")" && pwd)"
project_dir="$(dirname "$script_dir")"
xe_command="${XE:-$project_dir/build/xe}"
export XE_HOME="${XE_HOME:-$project_dir}"
temp_dir="$(mktemp -d "${TMPDIR:-/tmp}/xe-wintests.XXXXXX")"
trap 'rm -rf "$temp_dir"' EXIT
skip=" lib_network_test lib_system_test lib_misc_test "
passed=0; failed=0; skipped=0
for source_file in "$script_dir"/*.xe; do
    name="$(basename "$source_file" .xe)"
    if [[ "$skip" == *" $name "* ]]; then
        printf '[SKIPPED] %s (Linux-only features)\n' "$name"; skipped=$((skipped+1)); continue
    fi
    "$xe_command" --build "$source_file" -o "$temp_dir/$name" -q >/dev/null 2>&1
    (cd "$script_dir" && timeout 120 "$temp_dir/$name" </dev/null >"$temp_dir/$name.linux" 2>&1)
    if ! "$xe_command" --build "$source_file" -p win -o "$temp_dir/$name.exe" -q >"$temp_dir/$name.log" 2>&1; then
        printf '[FAILED] %s: Windows build failed\n' "$name"; failed=$((failed+1)); continue
    fi
    (cd "$script_dir" && WINEDEBUG=-all timeout 120 wine "$temp_dir/$name.exe" </dev/null 2>/dev/null | tr -d '\r' >"$temp_dir/$name.win")
    if cmp -s "$temp_dir/$name.linux" "$temp_dir/$name.win"; then
        printf '[PASSED] %s\n' "$name"; passed=$((passed+1))
    else
        printf '[FAILED] %s: output differs from Linux\n' "$name"; failed=$((failed+1))
    fi
done
printf '\nWindows tests (Wine): %d passed, %d failed, %d skipped\n' "$passed" "$failed" "$skipped"
[[ $failed -eq 0 ]]
