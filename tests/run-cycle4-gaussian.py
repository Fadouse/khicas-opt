#!/usr/bin/env python3
"""Check Gaussian/harmonic and Laplace rules.

By default all implementations and dispatch are extracted from current yintg.cc.
--verify also checks the Gaussian identities by independent 80-digit mpmath quadrature.
"""

from repository import source_path
import argparse
import ast
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from integration_build import ROOT, compiler_options, function

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
    "--verify",
    action="store_true",
    help="Also run independent high-precision quadratures",
)
args = parser.parse_args()
source = (source_path("yintg.cc")).read_text()
new_names = (
    "integration_harmonic_real",
    "integration_harmonic_product",
    "integration_harmonic_terms",
    "integration_gaussian_quadratic",
    "integrate_gaussian_erf",
)
new_signatures = ["  static bool " + name + "(" for name in new_names]
# Share the direct-rule runner's extraction catalog, without importing or
# executing its compilation loop. This keeps dependencies synchronized.
module = ast.parse((ROOT / "tests/run-real-definite.py").read_text())
loops = [
    node
    for node in module.body
    if isinstance(node, ast.For)
    and isinstance(node.target, ast.Name)
    and node.target.id == "sig"
]
assert len(loops) == 1, "Direct-rule extraction catalog not found"
signatures = [
    sig for sig in ast.literal_eval(loops[0].iter) if sig not in new_signatures
]
index = signatures.index("  static bool integrate_laplace_difference(")
signatures[index:index] = new_signatures
text = '#include "giacPCH.h"\nnamespace giac {\n'
text += "gen linear_integrate_nostep(const gen &,const gen &,gen &,int,GIAC_CONTEXT);\n"
for signature in signatures:
    body = function(source, signature)
    text += body.replace("  static ", "  ", 1)
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-cycle4-gaussian-") as tmp:
    directory = Path(tmp)
    rules = directory / "rules.cc"
    rules.write_text(text)
    flags, _ = compiler_options()
    flags = ["-O2" if flag == "-O1" else flag for flag in flags]
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    for name in ("cycle4_gaussian_integrals.cc", "cycle3_laplace_integrals.cc"):
        executable = directory / name.removesuffix(".cc")
        subprocess.run(
            flags
            + [str(rules), str(ROOT / "tests" / name)]
            + libs
            + ["-o", str(executable)],
            check=True,
            timeout=120,
        )
        subprocess.run([str(executable)], check=True, timeout=60)

if args.verify:
    import mpmath as m

    m.mp.dps = 80
    errors = []
    # (Gaussian a, center, exponent constant, erf slope, erf shift about center).
    singles = [
        (2, 0, 0, 1, 1),
        (2, 3, 1, 2, 1),
        (1, 1, -2, 2, 1),
        (2, 0, 0, -1, 1),
        (m.mpf(3) / 2, -m.mpf(2) / 5, m.mpf(1) / 7, -m.mpf(5) / 3, m.mpf(7) / 4),
    ]
    for a, center, offset, slope, shift in singles:
        a, center, offset, slope, shift = map(m.mpf, (a, center, offset, slope, shift))
        f = lambda x: (
            m.exp(offset - a * (x - center) ** 2) * m.erf(slope * (x - center) + shift)
        )
        observed = m.quad(f, [-m.inf, center - 3, center, center + 3, m.inf])
        expected = (
            m.exp(offset)
            * m.sqrt(m.pi / a)
            * m.erf(shift * m.sqrt(a / (a + slope * slope)))
        )
        errors.append(abs(observed - expected) / max(1, abs(expected)))
    # Both erfs are centered on the Gaussian. Signs are unrestricted.
    products = [
        (1, 1, 0, 1, 2),
        (1, 1, 0, -1, 2),
        (2, -3, 1, -2, -3),
        (m.mpf(3) / 2, m.mpf(2) / 3, -m.mpf(1) / 2, m.mpf(2) / 5, -m.mpf(7) / 4),
    ]
    for a, center, offset, b, c in products:
        f = lambda x: (
            m.exp(offset - a * (x - center) ** 2)
            * m.erf(b * (x - center))
            * m.erf(c * (x - center))
        )
        observed = m.quad(f, [-m.inf, center - 3, center, center + 3, m.inf])
        expected = (
            2
            * m.exp(offset)
            * m.asin(b * c / m.sqrt((a + b * b) * (a + c * c)))
            / m.sqrt(m.pi * a)
        )
        errors.append(abs(observed - expected) / max(1, abs(expected)))
    for slope in (-2, m.mpf(3) / 4):
        observed = m.quad(lambda x: m.exp(-2 * x * x) * m.erf(slope * x), [0, 1, m.inf])
        expected = m.atan(slope / m.sqrt(2)) / m.sqrt(2 * m.pi)
        errors.append(abs(observed - expected) / max(1, abs(expected)))
    assert max(errors) < m.mpf("1e-60"), m.nstr(max(errors), 12)
    print(
        "PASS: 11 independent 80-digit Gaussian/erf quadratures; maximum scaled error",
        m.nstr(max(errors), 12),
    )
