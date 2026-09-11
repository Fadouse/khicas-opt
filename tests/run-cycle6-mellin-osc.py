#!/usr/bin/env python3
"""Bounded real Mellin moments and convergent oscillatory differences."""

from repository import source_path
import argparse, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function, compiler_options

p = argparse.ArgumentParser()
p.add_argument("--verify", action="store_true")
args = p.parse_args()
s = (source_path("yintg.cc")).read_text()
h = s
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
    "  static bool integration_outer_power(",
    "  static bool integration_mellin_monomial(",
):
    text += function(s, sig)
for sig in (
    "  static gen integration_mixed_mellin_value(",
    "  static bool integrate_mixed_mellin_log(",
    "  static bool integrate_oscillatory_cancellation(",
):
    text += function(h, sig)
text += """bool cycle6_rule(const gen &input,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){
 gen e=integration_syntax(input,contextptr),c=integration_syntax(integration_coefficient(e,x,contextptr),contextptr);
 if(!integration_rational(c))return false;
 bool ok=integrate_mixed_mellin_log(e,x,lo,hi,r,contextptr)||integrate_oscillatory_cancellation(e,x,lo,hi,r,contextptr);
 if(ok)r=c*r;return ok;
}\n}\n"""
with tempfile.TemporaryDirectory(prefix="khicas-cycle6-mellin-osc-") as tmp:
    d = Path(tmp)
    (d / "rules.cc").write_text(text)
    flags, libs = compiler_options()
    subprocess.run(
        flags
        + [str(d / "rules.cc"), str(ROOT / "tests/cycle6_mellin_osc_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(d / "test")],
        check=True,
    )
    subprocess.run([str(d / "test")], check=True, timeout=60)

    if args.verify:
        import mpmath as mp

        mp.mp.dps = 70
        maximum = mp.mpf(0)
        for sn in (1, 2, 4, 5, 7, 8):
            s = mp.mpf(sn) / 3
            for logs in range(3):
                f = f"x^({sn}/3-1)*ln(x)^{logs}/((1+x)*(1+x^2))"

                def kernel(t):
                    base = (
                        mp.exp((s - 3) * t) / ((1 + mp.exp(-t)) * (1 + mp.exp(-2 * t)))
                        if t >= 0
                        else mp.exp(s * t) / ((1 + mp.exp(t)) * (1 + mp.exp(2 * t)))
                    )
                    return base * t**logs

                reference = mp.quad(kernel, [-mp.inf, -1, 0, 1, mp.inf])
                r = subprocess.run(
                    [str(d / "test"), f],
                    capture_output=True,
                    text=True,
                    timeout=15,
                    check=True,
                )
                error = abs(mp.mpf(r.stdout.strip()) - reference) / (1 + abs(reference))
                assert error < mp.mpf("2e-11"), (sn, logs, r.stdout, reference)
                maximum = max(maximum, error)
        print(
            "PASS: 18 independent 70-digit Mellin quadratures checked against printed double results; maxscalederror",
            mp.nstr(maximum, 12),
        )
