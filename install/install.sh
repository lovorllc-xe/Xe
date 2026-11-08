#!/usr/bin/env bash
# Installs Xe on Linux x86-64.
#
# The compiler is self-hosted: it is bootstrapped from the checked-in
# assembly seed (bootstrap/xe-seed.s.gz) and then rebuilds itself twice to
# verify that it reproduces itself exactly. No C or C++ compiler is needed;
# only GNU binutils (as, ld) and gzip.
#
#   ./install/install.sh                 install for the current user (~/.local)
#   sudo ./install/install.sh            install system-wide (/usr/local)
#   PREFIX=/opt/xe ./install/install.sh  install somewhere else
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
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

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    printf 'Xe builds native executables for Linux x86-64 only (this machine is %s %s).\n' "$(uname -s)" "$(uname -m)" >&2
    exit 1
fi

"$project_dir/scripts/bootstrap.sh" "$work_dir"

install -d "$bin_dir" "$lib_dir/std" "$lib_dir/compiler" "$lib_dir/libraries"
rm -f "$lib_dir/libraries/"*.xel
install -m 0755 "$work_dir/xe" "$bin_dir/xe"
install -m 0644 "$project_dir"/std/*.xe "$lib_dir/std/"
install -m 0644 "$project_dir"/compiler/*.xe "$project_dir/compiler/SOURCES" "$lib_dir/compiler/"
install -m 0644 "$project_dir"/libraries/*.xel "$lib_dir/libraries/"
# Remove files left behind by the old C++ bootstrap installer.
rm -rf -- "$lib_dir/runtime" "$lib_dir/compiler/selfhosted-xe"

printf 'Installed %s\n' "$("$bin_dir/xe" --version)"
printf '  compiler:  %s\n  libraries: %s\n' "$bin_dir/xe" "$lib_dir/libraries"
case ":$PATH:" in
    *":$bin_dir:"*) ;;
    *) printf 'Add %s to PATH to run xe.\n' "$bin_dir" ;;
esac

if XE="$bin_dir/xe" "$project_dir/tests/run_tests.sh"; then
    printf 'Xe installation tests passed.\n'
else
    printf 'Xe installed, but the installation tests failed.\n' >&2
    exit 1
fi
