#!/usr/bin/env python3
"""Review joint Beta derivatives and bounded reciprocal-binomial squares."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function

s = source_path("yintg.cc").read_text()
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
    "  static gen integration_beta_psi(",
    "  static void integration_beta_partitions(",
    "  static unsigned integration_beta_terms(",
    "  static gen integration_cumulant_moment(",
    "  static gen integration_beta_moment(",
    "  static bool integration_outer_power(",
    "  static gen integration_beta_joint(",
    "  static bool integrate_beta_log(",
    "  static bool integrate_inverse_gaussian(",
):
    text += function(s, sig)
text += """bool cycle4_beta_gaussian_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
 gen e=integration_syntax(f,contextptr),c=integration_coefficient(e,x,contextptr);
 if(!integration_rational(c))return false;
 bool matched=integrate_beta_log(e,x,lo,hi,r,contextptr)||integrate_inverse_gaussian(e,x,lo,hi,r,contextptr);
 if(matched)r=c*r;return matched;
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-cycle4-beta-gaussian-") as tmp:
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
        + [str(p / "rules.cc"), str(ROOT / "tests/cycle4_beta_gaussian_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
