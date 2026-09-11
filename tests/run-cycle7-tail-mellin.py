#!/usr/bin/env python3
"""Gaussian tails and denominator-log Mellin moments, including domain guards."""

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
    ("integration_one_plus", "bool"),
    ("integration_coefficient", "gen"),
    ("integration_monomial", "bool"),
    ("integration_beta_psi", "gen"),
    ("integration_beta_partitions", "void"),
    ("integration_beta_terms", "unsigned"),
    ("integration_cumulant_moment", "gen"),
    ("integration_beta_moment", "gen"),
    ("integration_outer_power", "bool"),
    ("integration_mellin_monomial", "bool"),
    ("integrate_mellin_log", "bool"),
    ("integrate_erf_tail_product", "bool"),
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
with tempfile.TemporaryDirectory(prefix="khicas-cycle7-tail-mellin-") as tmp:
    d = Path(tmp)
    (d / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    exe = d / "test"
    subprocess.run(
        flags
        + [str(d / "rules.cc"), str(ROOT / "tests/cycle7_tail_mellin.cc")]
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
    for a, b, q, tail in [
        (1, 1, 1, False),
        (2, 3, 1, False),
        (-2, 3, 1, False),
        (1, 1, 1, True),
        (2, 3, 1, True),
        (2, 3, 2, False),
        (1, 2, "0.5", False),
    ]:
        a, b, q = map(m.mpf, (a, b, q))
        got = m.quad(
            lambda x: (
                x ** (q - 1)
                * (m.erfc(a * x**q) if tail else m.erf(a * x**q))
                * m.erfc(b * x**q)
            ),
            [0, 1, m.inf],
        )
        expected = (
            (a + b - m.sqrt(a * a + b * b)) if tail else (m.sqrt(a * a + b * b) - b)
        ) / (q * a * b * m.sqrt(m.pi))
        errors.append(abs(got - expected))
    for A, B, q, power, r, n in [
        (1, 1, 2, 0, "1.5", 1),
        (1, 4, 2, 0, "1.5", 1),
        (1, 1, 2, 1, 2, 1),
        (1, 1, 2, 1, 2, 2),
        (1, 1, 1, 0, 2, 1),
        (2, 2, 1, 0, 2, 1),
        (1, 1, 2, 0, "1.5", 2),
        (1, 1, 2, 1, 2, 4),
    ]:
        A, B, q, power, r = map(m.mpf, (A, B, q, power, r))
        got = m.quad(
            lambda x: x**power * m.log(A + B * x**q) ** n / (A + B * x**q) ** r,
            [0, 1, m.inf],
        )
        a = (power + 1) / q
        expected = (-1) ** n * m.diff(
            lambda z: A ** (-z) * (A / B) ** a * m.beta(a, z - a) / q, r, n
        )
        errors.append(abs(got - expected))
    assert max(errors) < m.mpf("1e-65"), errors
    print("PASS:15 independent80-digit quadratures, max error", m.nstr(max(errors), 12))
