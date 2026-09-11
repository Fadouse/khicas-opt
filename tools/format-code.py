#!/usr/bin/env python3
"""Format maintained C/C++ source; generated parser/lookup tables stay untouched."""

import argparse
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    formatter = os.environ.get("CLANG_FORMAT") or shutil.which("clang-format")
    if not formatter:
        parser.error("Set CLANG_FORMAT or install clang-format")
    paths = [
        str(path)
        for path in sorted((ROOT / "src").rglob("*"))
        if (
            path.suffix in (".c", ".cc", ".cpp", ".h", ".hpp")
            or path.name == "iostream.new"
        )
        and "generated" not in path.parts
        and not path.name.endswith("_license.h")
    ]
    flags = ["--dry-run", "--Werror"] if args.check else ["-i"]
    subprocess.run([formatter, *flags, *paths], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
