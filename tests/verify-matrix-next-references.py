#!/usr/bin/env python3
import json, hashlib
from pathlib import Path
from report_store import work_report
import sympy as s, mpmath as m

m.mp.dps = 65
x = s.Symbol("x")
loc = {
    "x": x,
    "ln": s.log,
    "Psi": lambda z, n=0: s.polygamma(n, z),
    "infinity": s.oo,
    "abs": s.Abs,
}
cp = Path("tests/user-matrix-next.json")
j = json.loads(cp.read_text())
runs = []
for c in j["cases"]:
    if (
        "bounds" not in c
        or "ABS" in c["id"]
        and c["id"] != "NEXT-LOG-ABS-NEGATIVE-WAVE"
    ):
        continue
    f = s.lambdify(x, s.sympify(c["f"].replace("^", "**"), locals=loc), "mpmath")
    a, b = [
        m.mpf(str(s.N(s.sympify(v, locals=loc), 70)))
        if v not in ("+infinity", "-infinity")
        else (m.inf if v[0] == "+" else -m.inf)
        for v in c["bounds"]
    ]
    points = [a, b] if not a < 0 < b else [a, 0, b]
    if b == m.inf:
        points = [a, 1, 4, 16, m.inf]
    actual = m.quad(f, points)
    expected = m.mpf(
        str(s.N(s.sympify(c["expected"].replace("^", "**"), locals=loc), 70))
    )
    err = abs(actual - expected) / (1 + abs(expected))
    assert err < m.mpf("1e-30"), (c["id"], actual, expected, err)
    runs.append(
        {
            "id": c["id"],
            "quadrature": str(actual),
            "reference": str(expected),
            "relative_scaled_error": str(err),
        }
    )
work_report("matrix-reference").write_text(
    json.dumps(
        {
            "scope": "Independent 65-digit quadrature of defining integrands; no target answer matching used as the numerical reference. Algebraic indefinite identities proved in accompanying derivations.",
            "corpus_sha256": hashlib.sha256(cp.read_bytes()).hexdigest(),
            "runs": runs,
        },
        indent=2,
    )
    + "\n"
)
print("PASS", len(runs), "independent quadratures")
