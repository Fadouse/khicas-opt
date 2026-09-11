#!/usr/bin/env python3
"""Exercise the exact acos family from weighted disk symmetry."""

from repository import source_path
from pathlib import Path
import argparse, os, shlex, subprocess, tempfile
from integration_build import ROOT, function

parser = argparse.ArgumentParser()
args = parser.parse_args()
s = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for sig in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static gen integration_coefficient(",
):
    text += function(s, sig)
for sig in (
    "  static gen integration_acos_circle_value(",
    "  static bool integration_affine_cosine(",
    "  static bool integrate_acos_circle(",
):
    text += function(s, sig)
text += """bool user_arc_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
  return integrate_acos_circle(integration_syntax(f,contextptr),x,lo,hi,r,contextptr);
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-user-arc-") as tmp:
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
    subprocess.run(
        flags
        + [str(p / "rules.cc"), str(ROOT / "tests/user_arc_integrals.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
