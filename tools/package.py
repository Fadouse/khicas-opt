#!/usr/bin/env python3
"""Package the current signed source and matching CG50 build into dist/current."""

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
from repository import ROOT, build_inputs


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / ".build/optimized")
    args = parser.parse_args()
    subprocess.run(
        ["python3", str(ROOT / "tools/check-signatures.py"), "HEAD"],
        cwd=ROOT,
        check=True,
    )
    dirty = subprocess.check_output(
        ["git", "status", "--porcelain", "--", "src", "build", "assets", "vendor"],
        cwd=ROOT,
        text=True,
    )
    if dirty:
        raise RuntimeError("Commit the production source before packaging")
    build = args.build_dir.resolve()
    for name, source in build_inputs().items():
        if name == "Makefile":
            continue  # The portable build adds toolchain paths and target flags.
        if not (build / name).exists() or digest(build / name) != digest(source):
            raise RuntimeError("Build input mismatch: " + name)
    out = ROOT / "dist/current"
    out.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for name in ("khicas50.g3a", "khicas50.ac2"):
        shutil.copy2(build / name, out / name)
        hashes[name] = digest(out / name)
    commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
    ).strip()
    (out / "manifest.json").write_text(
        json.dumps(
            dict(
                version="2026a / 1.8.0",
                source_commit=commit,
                sha256=hashes,
                device_validation="Not installed or runtime-verified by this package operation",
            ),
            indent=2,
        )
        + "\n"
    )
    (out / "SHA256SUMS").write_text(
        "".join(f"{value}  {name}\n" for name, value in hashes.items())
    )
    print(out)


if __name__ == "__main__":
    main()
