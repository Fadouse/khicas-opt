#!/usr/bin/env python3
"""Bounded odd Gaussian-atan moments and exponential-log Mellin moments."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, compiler_options, function

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--verify", action="store_true")
args = parser.parse_args()
source = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for signature in (
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
    "  static bool integration_gaussian_quadratic(",
):
    text += function(source, signature).replace("  static ", "  ", 1)
for name in ["integrate_gaussian_atan_moment", "integrate_exponential_log_moment"]:
    text += function(source, "  static bool " + name + "(").replace(
        "  static ", "  ", 1
    )
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-cycle6-gaussian-fermi-") as tmp:
    directory = Path(tmp)
    (directory / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    exe = directory / "test"
    subprocess.run(
        flags
        + [
            str(directory / "rules.cc"),
            str(ROOT / "tests/cycle6_gaussian_fermi_integrals.cc"),
        ]
        + libs
        + ["-pthread", "-o", str(exe)],
        check=True,
        timeout=120,
    )
    subprocess.run([str(exe)], check=True, timeout=60)

if args.verify:
    import mpmath as m

    m.mp.dps = 70
    errors = []
    for a, b, n in [
        (1, 1, 0),
        (2, 3, 0),
        (1, -2, 0),
        (1, 1, 1),
        (1, 1, 2),
        (2, 1, 1),
        (3, 2, 8),
    ]:
        a, b = map(m.mpf, (a, b))
        measured = m.quad(
            lambda x: x ** (2 * n + 1) * m.exp(-a * x * x) * m.atan(b * x),
            [0, 1, m.inf],
        )
        # Differentiate the independently proved zeroth-moment formula. This does
        # not reuse the implementation's two-coefficient IBP recurrence.
        expected = (-1) ** n * m.diff(
            lambda z: (
                m.pi
                * m.sign(b)
                * m.exp(z / (b * b))
                * m.erfc(m.sqrt(z) / abs(b))
                / (4 * z)
            ),
            a,
            n,
        )
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    for a, q, N, sign in [
        (1, 1, 1, 1),
        (2, 1, 2, 1),
        (1, 2, 2, 1),
        (1, "0.5", 2, 1),
        (1, 1, 2, -1),
        (2, 2, 2, -1),
        (1, 1, 12, 1),
    ]:
        a, q = m.mpf(a), m.mpf(q)
        degree = q * N - 1
        measured = m.quad(
            lambda x: (
                x**degree
                * (
                    m.log1p(m.exp(-a * x**q))
                    if sign == 1
                    else m.log(-m.expm1(-a * x**q))
                )
            ),
            [0, 1, 10, m.inf],
        )
        expected = (
            m.factorial(N - 1)
            * ((1 - m.mpf(2) ** (-N)) if sign == 1 else -1)
            * m.zeta(N + 1)
            / (q * a**N)
        )
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-60"), errors
    print(
        "PASS:14 independent70-digit quadratures; max scaled error",
        m.nstr(max(errors), 12),
    )
