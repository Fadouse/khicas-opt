#!/usr/bin/env python3
"""Exercise complete-period reciprocal sinusoidal kernels and pole guards."""

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
    "  static bool integration_period_real_bound(",
    "  static bool integrate_reciprocal_trig_period(",
):
    text += function(s, sig)
text += """bool user_period_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
  return integrate_reciprocal_trig_period(integration_syntax(f,contextptr),x,lo,hi,r,contextptr);
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-user-period-") as tmp:
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
        + [str(p / "rules.cc"), str(ROOT / "tests/user_period_integrals.cc")]
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
