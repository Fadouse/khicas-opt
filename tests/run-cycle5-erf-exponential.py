#!/usr/bin/env python3
"""Gaussian-erf exponential shortcut identities and guarded stack regression."""

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
    "  static gen integration_coefficient(",
    "  static bool integration_gaussian_quadratic(",
):
    text += function(source, signature).replace("  static ", "  ", 1)
text += (
    function(source, "  static bool integrate_gaussian_erf_exp(").replace(
        "  static ", "  ", 1
    )
    + "}\n"
)
with tempfile.TemporaryDirectory(prefix="khicas-erf-exp-") as tmp:
    directory = Path(tmp)
    (directory / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    exe = directory / "test"
    subprocess.run(
        flags
        + [
            str(directory / "rules.cc"),
            str(ROOT / "tests/cycle5_erf_exponential_integrals.cc"),
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
    for a, b, c, d in [
        (1, 0, 1, 0),
        (2, 3, 4, 5),
        (-2, 3, 4, 5),
        (2, 3, -4, 5),
        (
            "0.5",
            "0.333333333333333333333333333333333333333333333333333333333333333333333333",
            "0.333333333333333333333333333333333333333333333333333333333333333333333333",
            -2,
        ),
        (2, 3, 0, 5),
    ]:
        a, b, c, d = map(m.mpf, (a, b, c, d))
        center = -b / a
        width = 1 / abs(a)
        measured = m.quad(
            lambda x: m.exp(-((a * x + b) ** 2) + c * m.erf(a * x + b) + d),
            [-m.inf, center - 4 * width, center, center + 4 * width, m.inf],
        )
        expected = m.exp(d) * m.sqrt(m.pi) / abs(a) * (m.sinh(c) / c if c else 1)
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-60"), errors
    print(
        "PASS: six independent70-digit quadratures; max scaled error",
        m.nstr(max(errors), 12),
    )
