#!/usr/bin/env python3
"""Direct tests for bounded beta/logit/Mellin extensions before integration."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function

s = (source_path("yintg.cc")).read_text()
parser = argparse.ArgumentParser()
args = parser.parse_args()
extra = "".join(
    function(s, sig)
    for sig in (
        "  static gen integration_beta_psi(",
        "  static void integration_beta_partitions(",
        "  static unsigned integration_beta_terms(",
        "  static gen integration_cumulant_moment(",
        "  static gen integration_beta_moment(",
        "  static bool integration_outer_power(",
        "  static bool integration_mellin_monomial(",
        "  static bool integrate_mellin_log(",
        "  static gen integration_beta_joint(",
        "  static bool integrate_beta_log(",
    )
)
text = '#include "giacPCH.h"\nnamespace giac {\n'
for sig in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static gen integration_coefficient(",
    "  static bool integration_monomial(",
    "  static bool integration_beta_weight(",
):
    text += function(s, sig).replace(
        "  static gen integration_syntax(", "  gen integration_syntax("
    )
text += (
    extra.replace(
        "  static bool integrate_mellin_log(", "  bool integrate_mellin_log("
    ).replace("  static bool integrate_beta_log(", "  bool integrate_beta_log(")
    + "}\n"
)
with tempfile.TemporaryDirectory(prefix="khicas-cycle3-beta-") as tmp:
    p = Path(tmp)
    (p / "rules.cc").write_text(text)
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O2",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    for test in ("cycle3_beta_integrals.cc", "beta_resource_integrals.cc"):
        subprocess.run(
            flags
            + [str(p / "rules.cc"), str(ROOT / "tests" / test)]
            + libs
            + ["-o", str(p / "test")],
            check=True,
        )
        subprocess.run([str(p / "test")], check=True, timeout=60)
