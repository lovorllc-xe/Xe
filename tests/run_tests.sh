#!/usr/bin/env bash
set -Eeuo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "$script_dir/.." && pwd)"
xe_command="${XE:-xe}"
temp_dir="$(mktemp -d "${TMPDIR:-/tmp}/xe-tests.XXXXXX")"

cleanup() {
    rm -rf -- "$temp_dir"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM HUP

if ! command -v "$xe_command" >/dev/null 2>&1; then
    printf '[FAILED] Xe compiler not found: %s\n' "$xe_command" >&2
    printf 'Set XE to the compiler path or add xe to PATH.\n' >&2
    exit 127
fi

mapfile -d '' test_files < <(find "$script_dir" -type f -name '*.xe' -print0 | sort -z)
if ((${#test_files[@]} == 0)); then
    printf '[FAILED] No .xe tests found in %s\n' "$script_dir" >&2
    exit 1
fi

passed=0
failed=0
for source_file in "${test_files[@]}"; do
    test_name="$(basename -- "$source_file")"
    executable="$temp_dir/${test_name%.xe}.xee"
    build_log="$temp_dir/${test_name}.build.log"
    run_log="$temp_dir/${test_name}.run.log"
    expected_output="$temp_dir/${test_name}.expected"
    started_ns="$(date +%s%N)"

    if ! "$xe_command" --build "$source_file" --library-path "$project_dir/libraries" -o "$executable" >"$build_log" 2>&1; then
        finished_ns="$(date +%s%N)"
        elapsed_ms=$(((finished_ns - started_ns) / 1000000))
        printf '[FAILED] %s (compile, %d ms)\n' "$test_name" "$elapsed_ms"
        cat "$build_log" >&2
        failed=$((failed + 1))
        continue
    fi

    if ! "$executable" >"$run_log" 2>&1; then
        finished_ns="$(date +%s%N)"
        elapsed_ms=$(((finished_ns - started_ns) / 1000000))
        printf '[FAILED] %s (execution, %d ms)\n' "$test_name" "$elapsed_ms"
        cat "$run_log" >&2
        failed=$((failed + 1))
        continue
    fi

    case "$test_name" in
        test_io.xe)
            printf 'Xe IO integration test\nsecond output line\n' >"$expected_output"
            if ! cmp -s -- "$expected_output" "$run_log"; then
                finished_ns="$(date +%s%N)"
                elapsed_ms=$(((finished_ns - started_ns) / 1000000))
                printf '[FAILED] %s (output mismatch, %d ms)\n' "$test_name" "$elapsed_ms"
                printf '%s\n' 'Expected:' >&2
                cat "$expected_output" >&2
                printf '%s\n' 'Received:' >&2
                cat "$run_log" >&2
                failed=$((failed + 1))
                continue
            fi
            ;;
    esac

    finished_ns="$(date +%s%N)"
    elapsed_ms=$(((finished_ns - started_ns) / 1000000))
    printf '[PASSED] %s (%d ms)\n' "$test_name" "$elapsed_ms"
    passed=$((passed + 1))
done

printf '\nTests: %d passed, %d failed\n' "$passed" "$failed"
((failed == 0))
