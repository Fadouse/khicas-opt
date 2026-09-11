#!/usr/bin/env python3
"""Exercise Frullani and log-log Mellin identities and their domain guards."""

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
    "  static bool integration_one_plus(",
    "  static gen integration_coefficient(",
    "  static bool integration_monomial(",
    "  static bool integration_mellin_monomial(",
):
    text += function(s, sig)
for sig in (
    "  static bool integrate_atan_frullani(",
    "  static bool integrate_loglog_mellin(",
):
    text += function(s, sig)
text += """bool user_log_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
 gen e=integration_syntax(f,contextptr);
 return integrate_atan_frullani(e,x,lo,hi,r,contextptr)||integrate_loglog_mellin(e,x,lo,hi,r,contextptr);
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-user-log-") as tmp:
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
        + [str(p / "rules.cc"), str(ROOT / "tests/user_log_integrals.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=30)
