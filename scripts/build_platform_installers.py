#!/usr/bin/env python3
"""Package source-based macOS and Windows Xe installers."""

from __future__ import annotations

import argparse
import re
import shutil
import tarfile
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
FILES = (
    "CMakeLists.txt",
    "README-INSTALL-LINUX.txt",
    "README-INSTALL-MACOS.txt",
    "README-INSTALL-WINDOWS.txt",
)
DIRS = ("src", "compiler", "std", "libraries", "docs", "install")


def version() -> str:
    source = (ROOT / "compiler" / "main.xe").read_text(encoding="utf-8")
    match = re.search(r'writeln\("Xe ([0-9]+(?:\.[0-9]+)+)"\)', source)
    if not match:
        raise SystemExit("Could not read Xe version from compiler/main.xe")
    return match.group(1)


def stage_bundle(stage: Path, bundle_name: str) -> Path:
    bundle = stage / bundle_name
    bundle.mkdir()
    for name in FILES:
        shutil.copy2(ROOT / name, bundle / name)
    for name in DIRS:
        shutil.copytree(ROOT / name, bundle / name)
    (bundle / "install" / "install.sh").chmod(0o755)
    (bundle / "install" / "install-macos.sh").chmod(0o755)
    return bundle


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=ROOT / "dist",
        help="destination directory (default: project dist/)",
    )
    args = parser.parse_args()
    output = args.output_dir.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)
    release = version()

    with tempfile.TemporaryDirectory(prefix="xe-platform-release-") as temporary:
        stage = Path(temporary)
        mac_name = f"Xe-{release}-macos"
        mac_bundle = stage_bundle(stage, mac_name)
        mac_archive = output / f"{mac_name}.tar.gz"
        with tarfile.open(mac_archive, "w:gz") as archive:
            archive.add(mac_bundle, arcname=mac_name)

        win_name = f"Xe-{release}-windows"
        win_bundle = stage_bundle(stage, win_name)
        win_archive = output / f"{win_name}.zip"
        with zipfile.ZipFile(win_archive, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(win_bundle.rglob("*")):
                if path.is_file():
                    archive.write(path, path.relative_to(stage))

    print(f"Created {mac_archive}")
    print(f"Created {win_archive}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
