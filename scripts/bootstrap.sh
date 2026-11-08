#!/usr/bin/env bash
# Builds the Xe compiler from source with no C or C++ toolchain:
#   1. assemble the checked-in seed (bootstrap/xe-seed.s.gz) into stage0,
#   2. stage0 compiles compiler/*.xe into stage1,
#   3. stage1 compiles compiler/*.xe into stage2,
#   4. stage1 and stage2 must emit byte-identical assembly (self-hosting fixpoint).
# Usage: scripts/bootstrap.sh [output-directory]   (default: build/)
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-$root/build}"
mkdir -p "$out"
for tool in as ld gzip; do
    command -v "$tool" >/dev/null 2>&1 || { echo "bootstrap: '$tool' is required (GNU binutils and gzip)" >&2; exit 1; }
done
mapfile -t sources < <(sed "s|^|$root/compiler/|" "$root/compiler/SOURCES")
export XE_HOME="$root"
gzip -dc "$root/bootstrap/xe-seed.s.gz" > "$out/stage0.s"
as -o "$out/stage0.o" "$out/stage0.s"
ld -o "$out/stage0" "$out/stage0.o"
rm -f "$out/stage0.s" "$out/stage0.o"
"$out/stage0" --build "${sources[@]}" -q -o "$out/stage1"
"$out/stage1" --build "${sources[@]}" -q -o "$out/stage2"
"$out/stage1" --emit-asm "${sources[@]}" -o "$out/stage1.s"
"$out/stage2" --emit-asm "${sources[@]}" -o "$out/stage2.s"
if ! cmp -s "$out/stage1.s" "$out/stage2.s"; then
    echo "bootstrap: stage1 and stage2 differ; the compiler is not reproducing itself" >&2
    exit 1
fi
rm -f "$out/stage0" "$out/stage1" "$out/stage1.s" "$out/stage2.s"
mv "$out/stage2" "$out/xe"
echo "bootstrap: built $out/xe (self-hosting fixpoint verified)"
