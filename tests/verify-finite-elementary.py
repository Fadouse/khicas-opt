#!/usr/bin/env python3
"""Independent high-precision finite elementary integral checks."""

import mpmath as m, json
from pathlib import Path
import argparse

p = argparse.ArgumentParser(description=__doc__)
p.add_argument(
    "--report", type=Path, default=Path("/tmp/khicas-finite-elementary-quadrature.json")
)
args = p.parse_args()
m.mp.dps = 80
cases = []


def check(name, f, L, U, reference):
    q = m.quad(f, [L, (L + U) / 2, U])
    err = abs(q - reference) / (1 + abs(reference))
    cases.append(
        dict(
            name=name,
            quadrature=str(q),
            reference=str(reference),
            scaled_error=str(err),
        )
    )


check(
    "sin negative slope and phase",
    lambda x: 3 * m.sin(-2 * x + m.pi / 3),
    m.mpf(-2) / 3,
    m.mpf(5) / 4,
    3 * (m.cos(-m.mpf(5) / 2 + m.pi / 3) - m.cos(m.mpf(4) / 3 + m.pi / 3)) / 2,
)
check(
    "cos rational phase",
    lambda x: m.cos(3 * x + m.mpf(1) / 7),
    -1,
    2,
    (m.sin(6 + m.mpf(1) / 7) - m.sin(-3 + m.mpf(1) / 7)) / 3,
)
check(
    "exp shifted negative slope",
    lambda x: 2 * m.exp(-3 * x + m.mpf(2) / 5),
    0,
    1,
    2 * (m.exp(m.mpf(2) / 5) - m.exp(-m.mpf(13) / 5)) / 3,
)
check(
    "log integrable zero left",
    lambda x: m.log(3 * x),
    0,
    m.mpf(2) / 3,
    (2 * m.log(2) - 2) / 3,
)
check(
    "log integrable zero right",
    lambda x: m.log(2 - 3 * x),
    0,
    m.mpf(2) / 3,
    (2 * m.log(2) - 2) / 3,
)
check("atan crossing zero", lambda x: m.atan(2 * x - 1), -1, 2, 0)
check("absolute crossing zero", lambda x: abs(2 * x - 1), 0, 1, m.mpf(1) / 2)
check(
    "reciprocal negative interval",
    lambda x: 1 / (3 * x - 1),
    -2,
    -1,
    m.log(m.mpf(4) / 7) / 3,
)
check(
    "positive quadratic rational scales",
    lambda x: 2 / (3 * x * x + 5),
    -1,
    2,
    2 * (m.atan(2 * m.sqrt(m.mpf(3) / 5)) + m.atan(m.sqrt(m.mpf(3) / 5))) / m.sqrt(15),
)
check(
    "fractional power shifted",
    lambda x: (3 * x + 2) ** (m.mpf(2) / 3),
    0,
    2,
    (m.mpf(8) ** (m.mpf(5) / 3) - m.mpf(2) ** (m.mpf(5) / 3)) / 5,
)
# Substitute x=t^3 so the independent quadrature has no algebraic endpoint singularity.
check("integrable singular x^(-2/3), x=t^3", lambda t: 3, 0, 1, m.mpf(3))
args.report.write_text(
    json.dumps(
        {
            "precision": 80,
            "cases": cases,
            "max_scaled_error": str(max(m.mpf(c["scaled_error"]) for c in cases)),
        },
        indent=2,
    )
    + "\n"
)
assert all(m.mpf(c["scaled_error"]) < m.mpf("1e-70") for c in cases)
print("PASS", len(cases), "independent 80-digit quadratures")
