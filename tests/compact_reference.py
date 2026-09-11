"""Independent exact comparison to an analytically proved compact primitive.

Used when expanding a reference's derivative would itself exceed host memory.
No production CAS implementation or recognizer is imported.
"""


def verify_binomial_reference(case, printed):
    import json
    from pathlib import Path
    import sympy as sp

    corpus = json.loads(
        Path(__file__).with_name("generalization-cycle8.json").read_text()
    )
    original = next(c for c in corpus["cases"] if c["id"] == case["id"])
    assert all(case[k] == original[k] for k in ("id", "f", "expected"))
    x = sp.Symbol("x", real=True)
    t = sp.Symbol("t", positive=True)
    n = sp.Symbol("n", integer=True, positive=True)
    F = (1 + t) ** (n + 2) / (32 * (n + 2)) - (1 + t) ** (n + 1) / (32 * (n + 1))
    assert sp.simplify(sp.diff(F, t) - t * (1 + t) ** n / 32) == 0
    u = x * x + x
    assert sp.simplify(sp.diff(u**32, x) / u**31 - 32 * (2 * x + 1)) == 0
    parse = lambda s: sp.sympify(s.replace("^", "**"), locals={"x": x})
    expected = parse(case["expected"])
    actual = parse(printed)
    assert expected == F.subs({t: u**32, n: 1024})
    assert parse(case["f"]) == sp.diff(u, x) * u**63 * (1 + u**32) ** 1024
    # Equal rational expressions in independent whole-power atoms remain
    # equal after restoring those atoms. No real-only branch assumption or
    # numerical samples are used to certify the printed result.
    powers = actual.atoms(sp.Pow) | expected.atoms(sp.Pow)
    mapping = {
        power: sp.Dummy()
        for power in powers
        if power.exp.is_Integer and abs(power.exp) >= 32
    }
    assert sp.cancel(actual.xreplace(mapping) - expected.xreplace(mapping)) == 0
    return {
        "exit": 0,
        "stderr": "CHECK_METHOD exact whole-power identity against independently proved generic binomial primitive\nCHECK exact\n",
    }
