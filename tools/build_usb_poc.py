#!/usr/bin/env python3
"""Build the independent SH4 USB protocol add-in with the existing CG50 SDK."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tools-dir", type=Path, required=True)
    args = parser.parse_args()
    sdk = args.tools_dir.resolve()
    bindir = sdk / "opt/sh3eb-elf/bin"
    library = sdk / "opt/sh3eb-elf/libfxcg"
    out = ROOT / ".build/usb-poc"
    out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env["LD_LIBRARY_PATH"] = str(sdk / "runtime") + ":" + env.get("LD_LIBRARY_PATH", "")
    source = ROOT / "src/platform/usb_poc/main.c"
    linker = ROOT / "build/linker/usb_poc.ld"
    compiler = bindir / "sh3eb-elf-gcc"
    subprocess.run(
        [
            str(compiler),
            "-std=c99",
            "-Os",
            "-mb",
            "-m4a-nofpu",
            "-mhitachi",
            "-ffreestanding",
            "-fno-builtin",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-nostdlib",
            "-I" + str(library / "include"),
            str(source),
            "-T" + str(linker),
            "-Wl,--gc-sections",
            "-Wl,-Map=" + str(out / "UsbPoc.map"),
            "-L" + str(library / "lib"),
            "-lc",
            "-lgcc",
            "-o",
            str(out / "UsbPoc.elf"),
        ],
        env=env,
        check=True,
    )
    subprocess.run(
        [
            str(bindir / "sh3eb-elf-objcopy"),
            "-O",
            "binary",
            str(out / "UsbPoc.elf"),
            str(out / "UsbPoc.bin"),
        ],
        env=env,
        check=True,
    )
    subprocess.run(
        [
            str(sdk / "bin/mkg3a"),
            "-n",
            "basic:USB PoC",
            "-n",
            "internal:USBPOC",
            "-V",
            "0.1.0",
            str(out / "UsbPoc.bin"),
            str(out / "UsbPoc.g3a"),
        ],
        env=env,
        check=True,
    )
    report = {
        "compiler": subprocess.check_output(
            [str(compiler), "--version"], env=env, text=True
        ).splitlines()[0],
        "files": {
            str(p.relative_to(ROOT)): {
                "bytes": p.stat().st_size,
                "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
            }
            for p in [source, linker, out / "UsbPoc.elf", out / "UsbPoc.g3a"]
        },
    }
    (out / "build.json").write_text(json.dumps(report, indent=2) + "\n")
    print(out / "UsbPoc.g3a")


if __name__ == "__main__":
    main()
