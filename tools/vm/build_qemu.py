#!/usr/bin/env python3
"""Fetch the pinned QEMU source, apply the CG50 board overlay, and build SH4eb."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = Path(__file__).with_name("qemu")
QEMU_COMMIT = "f8b2f64e2336a28bf0d50b6ef8a7d8c013e9bcf3"
QEMU_URL = "https://github.com/qemu/qemu.git"


def overlay_digest():
    digest = hashlib.sha256()
    for path in sorted(OVERLAY.iterdir()):
        if path.suffix in (".c", ".h"):
            digest.update(path.name.encode() + b"\0" + path.read_bytes())
    return digest.hexdigest()


def run(command, **kwargs):
    subprocess.run(command, check=True, **kwargs)


def append_once(path, marker, content):
    text = path.read_text()
    if marker not in text:
        path.write_text(text + content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / ".build/vm/qemu")
    parser.add_argument("-j", "--jobs", type=int, default=min(os.cpu_count() or 2, 8))
    args = parser.parse_args()
    source = args.source.resolve()
    if not 1 <= args.jobs <= 64:
        parser.error("jobs must be 1..64")
    for tool in ("git", "ninja", "meson", "pkg-config", "gcc"):
        if not shutil.which(tool):
            parser.error(f"{tool} not found; use nix-shell tools/vm/shell.nix")
    if not source.exists():
        source.parent.mkdir(parents=True, exist_ok=True)
        run(
            [
                "git",
                "clone",
                "--depth",
                "1",
                "--branch",
                "v10.1.0",
                QEMU_URL,
                str(source),
            ]
        )
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    if commit != QEMU_COMMIT:
        parser.error(
            f"QEMU source revision mismatch: expected {QEMU_COMMIT}, got {commit}"
        )
    for path in OVERLAY.iterdir():
        if path.suffix in (".c", ".h"):
            target = source / "hw/sh4" / path.name
            if not target.exists() or target.read_bytes() != path.read_bytes():
                shutil.copyfile(path, target)
    append_once(
        source / "hw/sh4/Kconfig",
        "config CG50",
        "\nconfig CG50\n    bool\n    default y\n    depends on SH4\n    select SH_INTC\n",
    )
    meson = source / "hw/sh4/meson.build"
    sources = ", ".join(repr(path.name) for path in sorted(OVERLAY.glob("*.c")))
    entry = f"sh4_ss.add(when: 'CONFIG_CG50', if_true: files({sources}))"
    text = re.sub(
        r"sh4_ss\.add\(when: 'CONFIG_CG50', if_true: files\([^\n]*\)\)",
        entry,
        meson.read_text(),
    )
    if entry not in text:
        text = text.replace(
            "sh4_ss = ss.source_set()",
            "sh4_ss = ss.source_set()\n" + entry,
        )
    if meson.read_text() != text:
        meson.write_text(text)
    build = source / "build"
    if not (build / "build.ninja").exists():
        run(
            [
                "./configure",
                "--target-list=sh4eb-softmmu",
                "--disable-docs",
                "--disable-gtk",
                "--disable-sdl",
                "--disable-vnc",
                "--disable-tools",
            ],
            cwd=source,
        )
    run(["ninja", "-C", str(build), "-j", str(args.jobs), "qemu-system-sh4eb"])
    binary = ROOT / ".build/vm/bin/qemu-system-sh4eb"
    binary.parent.mkdir(parents=True, exist_ok=True)
    staged_binary = binary.with_suffix(".new")
    shutil.copy2(build / "qemu-system-sh4eb", staged_binary)
    staged_binary.replace(binary)
    metadata = {
        "qemu_commit": commit,
        "source": str(source),
        "binary": str(binary),
        "board_sha256": overlay_digest(),
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "version": subprocess.check_output(
            [str(binary), "--version"], text=True
        ).strip(),
    }
    output = ROOT / ".build/vm/qemu.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(metadata, indent=2) + "\n")
    print(binary)


if __name__ == "__main__":
    main()
