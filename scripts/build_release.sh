#!/usr/bin/env bash
set -Eeuo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${1:-$project_dir/dist}"

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    printf '%s\n' 'Release bundles must be built on Linux x86_64.' >&2
    exit 1
fi

version="$(sed -n 's/^[[:space:]]*writeln("Xe \([0-9][0-9.]*\)").*/\1/p' "$project_dir/compiler/main.xe" | head -n 1)"
if [[ -z "$version" ]]; then
    printf 'Could not read Xe version from %s\n' "$project_dir/compiler/main.xe" >&2
    exit 1
fi

mkdir -p -- "$output_dir"
output_dir="$(cd -- "$output_dir" && pwd)"
bundle_name="Xe-${version}-linux-x86_64"
archive="$output_dir/${bundle_name}.tar.gz"
stage="$(mktemp -d "${TMPDIR:-/tmp}/xe-release.XXXXXX")"
trap 'rm -rf -- "$stage"' EXIT

bundle_dir="$stage/$bundle_name"
mkdir -p -- "$bundle_dir"
for path in CMakeLists.txt README-INSTALL.txt install.sh src compiler std libraries tests docs scripts; do
    cp -a -- "$project_dir/$path" "$bundle_dir/"
done

# Keep the packaged entry points runnable even if checkout permissions changed.
chmod 0755 "$bundle_dir/install.sh" "$bundle_dir/tests/run_tests.sh"

tar -czf "$archive" -C "$stage" "$bundle_name"
printf 'Created release bundle: %s\n' "$archive"
