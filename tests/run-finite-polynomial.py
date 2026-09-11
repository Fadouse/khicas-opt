#!/usr/bin/env python3
"""Bounded finite polynomial integration avoids generic endpoint limits."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, compiler_options, function

p = argparse.ArgumentParser(description=__doc__)
args = p.parse_args()
source = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for sig in ("  static bool integration_rational(", "  static gen integration_syntax("):
    text += function(source, sig).replace("  static ", "  ", 1)
for name in [
    "integration_finite_poly_add",
    "integration_finite_poly_product",
    "integration_finite_poly_terms",
    "integrate_finite_polynomial",
]:
    text += function(source, "  static bool " + name + "(").replace(
        "  static ", "  ", 1
    )
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-finite-poly-") as tmp:
    d = Path(tmp)
    (d / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    exe = d / "test"
    subprocess.run(
        flags
        + [str(d / "rules.cc"), str(ROOT / "tests/finite_polynomial_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(exe)],
        check=True,
        timeout=120,
    )
    subprocess.run([str(exe)], check=True, timeout=60)
