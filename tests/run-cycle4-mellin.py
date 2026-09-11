#!/usr/bin/env python3
"""Exercise bounded two-binomial Mellin integration and its s=1 limit."""

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
    "  static bool integration_monomial(",
):
    text += function(s, sig)
text += function(s, "  static bool integrate_mellin_two_binomials(")
text += """bool cycle4_mellin_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
  gen e=integration_syntax(f,contextptr),c=integration_coefficient(e,x,contextptr);
  if(!integration_rational(c))return false;
  bool reverse=(lo==plus_inf && is_zero(hi));
  bool ok=integrate_mellin_two_binomials(e,x,reverse?hi:lo,reverse?lo:hi,r,contextptr);
  if(ok)r=r*(reverse?-c:c);return ok;
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-cycle4-mellin-") as tmp:
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
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac", "-pthread"]
    subprocess.run(
        flags
        + [str(p / "rules.cc"), str(ROOT / "tests/cycle4_mellin_integrals.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
    subprocess.run(
        [str(p / "test")],
        check=True,
        timeout=60,
        env=dict(os.environ, KHICAS_TEST_STACK_KIB="64"),
    )
