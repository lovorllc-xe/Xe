#!/usr/bin/env bash
# Regenerates bootstrap/xe-seed.s.gz from the current compiler sources.
# Run it after changing compiler/*.xe so a fresh checkout can bootstrap.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d "${TMPDIR:-/tmp}/xe-seed.XXXXXX")"
trap 'rm -rf -- "$work"' EXIT
"$root/scripts/bootstrap.sh" "$work"
mapfile -t sources < <(sed "s|^|$root/compiler/|" "$root/compiler/SOURCES")
XE_HOME="$root" "$work/xe" --emit-asm "${sources[@]}" -o "$work/seed.s"
gzip -9 -c "$work/seed.s" > "$root/bootstrap/xe-seed.s.gz"
echo "update-seed: wrote bootstrap/xe-seed.s.gz"
