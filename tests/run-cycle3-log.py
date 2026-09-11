#!/usr/bin/env python3
"""Exercise logarithmic moment identities and their convergence/domain guards."""

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
    "  static bool integration_one_plus(",
    "  static gen integration_coefficient(",
    "  static bool integration_monomial(",
    "  static bool integration_quadratic(",
):
    text += function(s, sig)
for sig in (
    "  static bool integrate_log_zeta(",
    "  static bool integrate_hyperbolic_log(",
    "  static bool integrate_atan_log_moment(",
):
    text += function(s, sig)
text += """bool cycle3_log_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
  gen e=integration_syntax(f,contextptr);
  return integrate_log_zeta(e,x,lo,hi,r,contextptr) ||
    integrate_hyperbolic_log(e,x,lo,hi,r,contextptr) ||
    integrate_atan_log_moment(e,x,lo,hi,r,contextptr);
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-cycle3-log-") as tmp:
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
        + [str(p / "rules.cc"), str(ROOT / "tests/cycle3_log_integrals.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
