#!/usr/bin/env python3
"""Test bounded erf-chain and paired-rectangle helpers, also on a 64 KiB stack."""

from repository import source_path
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, compiler_options, function

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
    "--verify",
    action="store_true",
    help="Independent 70-digit quadrature of generalized identities",
)
args = parser.parse_args()
source = (source_path("yintg.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for signature in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static gen integration_coefficient(",
    "  static bool integration_outer_power(",
    "  static bool small_polynomial(",
    "  static bool small_sparse_polynomial(",
    "  static bool integration_quadratic(",
    "  static bool integration_square_root(",
):
    text += function(source, signature).replace("  static ", "  ", 1)
for name in (
    "integration_chain_add",
    "integration_chain_terms",
    "integration_chain_endpoint",
    "integrate_erf_chain",
    "integration_rectangle_term",
    "integrate_atan_rectangle_pair",
):
    text += function(source, "  static bool " + name + "(").replace(
        "  static ", "  ", 1
    )
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-chain-rectangle-") as tmp:
    directory = Path(tmp)
    (directory / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    executable = directory / "test"
    subprocess.run(
        flags
        + [
            str(directory / "rules.cc"),
            str(ROOT / "tests/cycle5_gaussian_rectangle_integrals.cc"),
        ]
        + libs
        + ["-pthread", "-o", str(executable)],
        check=True,
        timeout=120,
    )
    subprocess.run([str(executable)], check=True, timeout=60)

if args.verify:
    import mpmath as m

    m.mp.dps = 70
    errors = []
    for a, b, n in [(1, 1, 2), (2, 3, 4), (-2, -3, 4), (3, 2, 3)]:
        a, b = m.mpf(a), m.mpf(b)

        def integrand(x):
            u = a * m.sqrt(x) - b / m.sqrt(x)
            return (a * x + b) / (2 * x ** m.mpf("1.5")) * m.exp(-u * u) * m.erf(u) ** n

        measured = m.quad(integrand, [0, m.mpf("0.25"), 1, 4, m.inf])
        direction = m.sign(a)
        expected = direction * m.sqrt(m.pi) / (n + 1) if n % 2 == 0 else m.mpf(0)
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    # A nonmonotone inner function is valid: only its endpoint values matter.
    measured = m.quad(
        lambda x: (2 * x - 1) * m.exp(-((x * x - x) ** 2)) * m.erf(x * x - x) ** 2,
        [0, m.mpf(".5"), 1],
    )
    errors.append(abs(measured))
    for A, B, C, scale in [
        (2, 3, 1, 1),
        (1, 4, 3, 2),
        (
            "0.5",
            "2.333333333333333333333333333333333333333333333333333333333333333333333333",
            5,
            -3,
        ),
    ]:
        A, B, C, scale = map(m.mpf, (A, B, C, scale))

        def term(x, a, b):
            root = m.sqrt(a * a * x * x + 2 * C)
            return a * m.atan(b / root) / ((a * a * x * x + C) * root)

        measured = m.quad(lambda x: scale * (term(x, A, B) + term(x, B, A)), [0, 1])
        expected = scale * m.atan(A / m.sqrt(C)) * m.atan(B / m.sqrt(C)) / C
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-60"), errors
    print(
        "PASS:",
        len(errors),
        "independent 70-digit quadratures; max scaled error",
        m.nstr(max(errors), 12),
    )
