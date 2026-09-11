#!/usr/bin/env python3
"""Bounded Gamma logarithmic moments and shared Beta cumulant regression."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, compiler_options, function

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--verify", action="store_true")
args = p.parse_args()
s = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for name, kind in [
    ("decompose_prod", "void"),
    ("extract_cst", "gen"),
    ("integration_rational", "bool"),
    ("integration_syntax", "gen"),
    ("integration_power", "bool"),
    ("integration_coefficient", "gen"),
    ("integration_monomial", "bool"),
    ("integration_beta_psi", "gen"),
    ("integration_beta_partitions", "void"),
    ("integration_beta_terms", "unsigned"),
    ("integration_cumulant_moment", "gen"),
    ("integration_outer_power", "bool"),
    ("integration_mellin_monomial", "bool"),
    ("integrate_gamma_log_moment", "bool"),
]:
    sig = (
        "  "
        + ("" if name in ("decompose_prod", "extract_cst") else "static ")
        + kind
        + " "
        + name
        + "("
    )
    text += function(s, sig).replace("  static ", "  ", 1)
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-cycle8-gamma-") as tmp:
    d = Path(tmp)
    (d / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    exe = d / "test"
    subprocess.run(
        flags
        + [str(d / "rules.cc"), str(ROOT / "tests/cycle8_gamma_log.cc")]
        + shlex.split(os.environ.get("LDFLAGS", ""))
        + ["-lgiac", "-pthread", "-o", str(exe)],
        check=True,
        timeout=120,
    )
    subprocess.run([str(exe)], check=True, timeout=60)
if args.verify:
    import mpmath as m

    m.mp.dps = 80
    errors = []
    # Differentiate the whole analytic Mellin transform independently, rather
    # than repeating the implementation's cumulant/Bell recurrence.
    for a, q, degree, B, k, n in [
        (1, 2, 0, 1, 1, 2),
        (2, 2, 0, 1, 1, 0),
        (1, 1, 0, 1, 1, 1),
        (1, 1, 0, 1, 1, 2),
        (1, 1, 0, 1, 1, 3),
        (1, 1, 0, 1, 1, 4),
        (1, 2, 3, 1, 1, 1),
        (2, 2, 0, 3, 2, 2),
        (1, "0.5", 0, 1, 1, 2),
        (1, "2/3", "1/3", 1, 1, 1),
        (1, 2, 0, 1, -1, 3),
        (1, 1, 15, 1, 1, 0),
    ]:

        def val(v):
            return (
                m.mpf(v)
                if "/" not in str(v)
                else m.mpf(str(v).split("/")[0]) / m.mpf(str(v).split("/")[1])
            )

        a, q, degree, B, k = map(val, (a, q, degree, B, k))
        got = m.quad(
            lambda x: x**degree * m.exp(-a * x**q) * m.log(B * x**k) ** n, [0, 1, m.inf]
        )
        s0 = (degree + 1) / q
        expected = m.diff(
            lambda t: B**t * m.gamma(s0 + k * t / q) / (q * a ** (s0 + k * t / q)), 0, n
        )
        errors.append(abs(got - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-65"), errors
    print(
        "PASS:12 independent80-digit Gamma moment quadratures; maximum scaled error",
        m.nstr(max(errors), 12),
    )
