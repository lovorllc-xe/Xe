#!/usr/bin/env bash
# Builds a relocatable Linux x86-64 release tarball:
#   dist/xe-<version>-linux-x86_64.tar.gz
# The compiler is bootstrapped from the seed (needs only as, ld, gzip).
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -A2 'fn version' "$root/compiler/main.xe" | grep -o '"[0-9][^"]*"' | tr -d '"')"
name="xe-$version-linux-x86_64"
work="$(mktemp -d "${TMPDIR:-/tmp}/xe-release.XXXXXX")"
trap 'rm -rf -- "$work"' EXIT
"$root/scripts/bootstrap.sh" "$work/build"
stage="$work/$name"
mkdir -p "$stage/bin" "$stage/lib/xelang" "$stage/share/xe"
install -m 0755 "$work/build/xe" "$stage/bin/xe"
cp -r "$root/std" "$root/compiler" "$root/libraries" "$stage/lib/xelang/"
rm -rf "$stage/lib/xelang/compiler/selfhosted-xe"
cp -r "$root/logo" "$root/docs" "$root/examples" "$root/apps" "$stage/share/xe/" 2>/dev/null || true
mkdir -p "$stage/share/xe/examples/shaders" && cp "$root"/tests/shaders/* "$stage/share/xe/examples/shaders/" 2>/dev/null || true
cp "$root/README.md" "$root/README-INSTALL-LINUX.txt" "$stage/"
cat > "$stage/install.sh" <<'SH'
#!/usr/bin/env bash
# Installs this prebuilt Xe release.
#   ./install.sh                 -> ~/.local        (current user)
#   sudo ./install.sh            -> /usr/local      (system-wide)
#   PREFIX=/opt/xe ./install.sh  -> custom prefix
set -euo pipefail
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${PREFIX:-}" ]]; then prefix="$PREFIX"
elif (( EUID == 0 )); then prefix=/usr/local
else prefix="${HOME:?HOME must be set}/.local"; fi
install -d "$prefix/bin" "$prefix/lib" "$prefix/share"
rm -rf "$prefix/lib/xelang" "$prefix/share/xe"
cp -r "$here/lib/xelang" "$prefix/lib/xelang"
cp -r "$here/share/xe" "$prefix/share/xe"
install -m 0755 "$here/bin/xe" "$prefix/bin/xe"
"$prefix/bin/xe" --version | tail -1
case ":$PATH:" in *":$prefix/bin:"*) ;; *) echo "Add $prefix/bin to PATH to run xe." ;; esac
SH
cat > "$stage/uninstall.sh" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
if [[ -n "${PREFIX:-}" ]]; then prefix="$PREFIX"
elif (( EUID == 0 )); then prefix=/usr/local
else prefix="${HOME:?}/.local"; fi
rm -rf "$prefix/bin/xe" "$prefix/lib/xelang" "$prefix/share/xe"
echo "Xe removed from $prefix"
SH
chmod 0755 "$stage/install.sh" "$stage/uninstall.sh"
mkdir -p "$root/dist"
tar -C "$work" --owner=0 --group=0 -czf "$root/dist/$name.tar.gz" "$name"
( cd "$root/dist" && sha256sum "$name.tar.gz" > "$name.tar.gz.sha256" )
echo "release: dist/$name.tar.gz"
