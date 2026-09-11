#!/usr/bin/env python3
"""Logarithmic moments of exponential differences on a guarded 64 KiB stack."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, compiler_options, function

parser = argparse.ArgumentParser(description=__doc__)
args = parser.parse_args()
source = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for signature in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static gen integration_coefficient(",
    "  static bool integration_outer_power(",
):
    text += function(source, signature).replace("  static ", "  ", 1)
for name in ("integration_exp_affine", "integrate_exp_difference"):
    text += function(source, "  static bool " + name + "(").replace(
        "  static ", "  ", 1
    )
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-exp-difference-") as tmp:
    directory = Path(tmp)
    (directory / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    executable = directory / "test"
    subprocess.run(
        flags
        + [str(directory / "rules.cc"), str(ROOT / "tests/cycle5_exp_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(executable)],
        check=True,
        timeout=120,
    )
    subprocess.run([str(executable)], check=True, timeout=60)
