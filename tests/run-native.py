#!/usr/bin/env python3
"""Check the actual root-level libbf with native 32/64-bit limb arithmetic."""

from repository import source_path
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="khicas-native-") as tmp:
    work = Path(tmp)
    # Exclude calculator-specific inttypes.h and fenv.h from host compilation.
    for name in ("libbf.c", "libbf.h", "cutils.c", "cutils.h"):
        shutil.copy2(source_path(name), work / name)
    header = work / "libbf.h"
    header.write_text(
        header.read_text().replace(
            "#if INTPTR_MAX >= INT64_MAX",
            "#if !defined(KHICAS_TEST_32_LIMBS) && INTPTR_MAX >= INT64_MAX",
        )
    )
    for bits in (32, 64):
        output = work / f"test-{bits}"
        command = [
            os.environ.get("CC", "cc"),
            "-std=c11",
            "-O2",
            "-g",
            "-fsanitize=address,undefined",
            "-fno-sanitize-recover=all",
            "-I",
            str(work),
            str(ROOT / "tests/libbf_small_operands.c"),
            str(work / "libbf.c"),
            str(work / "cutils.c"),
            "-lm",
            "-o",
            str(output),
        ]
        if bits == 32:
            command.append("-DKHICAS_TEST_32_LIMBS")
        subprocess.run(command, check=True)
        subprocess.run([str(output)], check=True)
