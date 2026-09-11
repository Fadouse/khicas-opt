#!/usr/bin/env python3
"""Real reciprocal-cosh definite identities, pole rejection and small-stack checks."""

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
):
    text += function(source, signature).replace("  static ", "  ", 1)
text += (
    function(source, "  static bool integrate_reciprocal_cosh(").replace(
        "  static ", "  ", 1
    )
    + "}\n"
)
with tempfile.TemporaryDirectory(prefix="khicas-reciprocal-cosh-") as tmp:
    directory = Path(tmp)
    (directory / "rules.cc").write_text(text)
    flags, _ = compiler_options()
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    exe = directory / "test"
    subprocess.run(
        flags
        + [
            str(directory / "rules.cc"),
            str(ROOT / "tests/real_cosh_reciprocal_integrals.cc"),
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
    for A, B, a, b, full in [
        (2, 1, 1, 0, False),
        (5, 3, -2, 0, False),
        (1, 1, 1, 0, False),
        (-1, 2, 1, 0, False),
        (1, 2, 1, 0, False),
        (2, 1, 3, 7, True),
        (2, 1, -3, -7, True),
    ]:
        A, B, a, b = map(m.mpf, (A, B, a, b))
        f = lambda x: 1 / (A + B * m.cosh(a * x + b))
        bounds = [-m.inf, -b / a, m.inf] if full else [0, 1, m.inf]
        measured = m.quad(f, bounds)
        value = (
            1 / B
            if A == B
            else (
                m.log((A + m.sqrt(A * A - B * B)) / B) / m.sqrt(A * A - B * B)
                if A > B
                else 2 * m.atan(m.sqrt((B - A) / (B + A))) / m.sqrt(B * B - A * A)
            )
        )
        expected = (2 if full else 1) * value / abs(a)
        errors.append(abs(measured - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-60"), errors
    print(
        "PASS: seven independent70-digit quadratures; max scaled error",
        m.nstr(max(errors), 12),
    )
