#!/usr/bin/env python3
import mpmath as m, json
from pathlib import Path
from report_store import work_report

m.mp.dps = 65
cases = [
    ("D1", lambda x: m.exp(-(1 + 2j) * x), [0, 1, m.inf], 1 / m.mpc(1, 2)),
    ("D2", lambda x: x ** (2j) * m.exp(-x), [0, 1, m.inf], m.gamma(1 + 2j)),
    (
        "D3",
        lambda x: x ** (1j) * (1 - x) ** (-1j),
        [0, m.mpf("0.5"), 1],
        m.pi / m.sinh(m.pi),
    ),
    (
        "D4",
        lambda x: x ** (-m.mpf("0.5") + 1j) / (1 + x),
        [0, 1, m.inf],
        m.pi / m.cosh(m.pi),
    ),
    (
        "PARAM-Q2",
        lambda x: m.exp(-(1 + 1j) * x) - m.exp(-(1 - 1j) * x),
        [0, 1, m.inf],
        -1j,
    ),
    (
        "PARAM-Q5",
        lambda x: x ** (2j) / (1 + x * x),
        [0, 1, m.inf],
        m.pi / (2 * m.cosh(m.pi)),
    ),
]
rows = []
for id, f, points, expected in cases:
    actual = m.quad(f, points)
    error = abs(actual - expected) / (1 + abs(expected))
    assert error < m.mpf("1e-28"), (id, error)
    rows.append(
        dict(id=id, actual=str(actual), expected=str(expected), scaled_error=str(error))
    )
work_report("parameter-reference").write_text(
    json.dumps(
        {
            "scope": "65-digit independent quadrature; complements symbolic domain contracts, does not replace them",
            "runs": rows,
        },
        indent=2,
    )
    + "\n"
)
print("PASS: 6 independent complex quadratures")
