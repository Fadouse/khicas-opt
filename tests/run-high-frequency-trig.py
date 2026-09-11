#!/usr/bin/env python3
"""Exact compact trig-power primitives, including a guarded 64 KiB stack."""

from repository import source_path
import subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function, compiler_options

s = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
for sig in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static gen integration_coefficient(",
):
    text += function(s, sig)
text += function(s, "  static bool integrate_high_frequency_trig(")
text += """bool high_frequency_rule(const gen &input,const gen &x,gen &r,GIAC_CONTEXT){
 if(taille(input,129)>128)return false;
 gen e=integration_syntax(input,contextptr),c=integration_coefficient(e,x,contextptr);
 if(is_undef(c) || is_inf(c))return false;
 if(!integrate_high_frequency_trig(e,x,r,contextptr))return false;
 r=c*r;return true;
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-high-frequency-") as tmp:
    d = Path(tmp)
    (d / "rule.cc").write_text(text)
    flags, libs = compiler_options()
    subprocess.run(
        flags
        + [str(d / "rule.cc"), str(ROOT / "tests/high_frequency_trig_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(d / "test")],
        check=True,
    )
    subprocess.run([str(d / "test")], check=True, timeout=60)
