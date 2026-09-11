#!/usr/bin/env python3
"""Exercise rational half-angle complements and reciprocal square symmetry."""

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
    "  static bool small_polynomial(",
    "  static bool small_sparse_polynomial(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static gen integration_coefficient(",
    "  static bool integration_quadratic(",
    "  static bool integration_square_root(",
):
    text += function(s, sig)
for sig in (
    "  static gen integration_acos_circle_value(",
    "  static bool integrate_atan_square(",
    "  static bool integrate_arc_rational_circle(",
):
    text += function(s, sig)
text += """bool cycle4_arc_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
  gen e=integration_syntax(f,contextptr),c=integration_coefficient(e,x,contextptr);
  if(!integration_rational(c))return false;
  bool reverse=is_strictly_greater(lo,hi,contextptr);
  bool ok=integrate_atan_square(e,x,reverse?hi:lo,reverse?lo:hi,r,contextptr) ||
    integrate_arc_rational_circle(e,x,reverse?hi:lo,reverse?lo:hi,r,contextptr);
  if(ok)r=r*(reverse?-c:c);return ok;
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-cycle4-arc-") as tmp:
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
        + [str(p / "rules.cc"), str(ROOT / "tests/cycle4_arc_integrals.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
