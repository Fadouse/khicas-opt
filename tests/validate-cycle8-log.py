#!/usr/bin/env python3
"""Independent numerical audit of owned C8 log variants, without Giac calls."""

from pathlib import Path
import re
import mpmath as mp
import sympy as sp

x = sp.Symbol("x", real=True)
names = dict(
    x=x, ln=sp.log, Psi=lambda z, n=0: sp.polygamma(n, z), Zeta=sp.zeta, infinity=sp.oo
)


def symbolic(s):
    return sp.sympify(s.replace("^", "**"), locals=names)


def number(s):
    return mp.mpf(str(sp.N(symbolic(s), mp.mp.dps))) if s != "+infinity" else mp.inf


text = (
    (Path(__file__).with_name("cycle8_log_integrals.cc"))
    .read_text()
    .split("static const Case yes[]={", 1)[1]
    .split("};", 1)[0]
)
rows = re.findall(r'\{"([^"]+)","([^"]+)","([^"]+)","([^"]+)"\}', text)
assert len(rows) == 26
worst = mp.mpf(0)
count = 0
for digits in (80, 110):
    mp.mp.dps = digits
    for raw, lo, hi, want in rows:
        f = symbolic(raw)
        # Exact half-angle rewrites prevent 1-sin(theta) rounding to zero
        # near an integrable endpoint; no integration algorithm is called.
        replacements = {}
        for log in f.atoms(sp.log):
            arg = log.args[0]
            trigs = list(arg.atoms(sp.sin, sp.cos))
            if len(trigs) != 1:
                continue
            trig = trigs[0]
            p = sp.Poly(arg, trig)
            A = p.nth(0)
            B = p.nth(1)
            assert p.degree() == 1 and A > 0 and B in (A, -A)
            theta = trig.args[0]
            sign = 1 if B == A else -1
            offset = (
                (sp.pi / 2 if sign == 1 else 0)
                if trig.func == sp.cos
                else (sp.pi / 4 if sign == 1 else 3 * sp.pi / 4)
            )
            replacements[log] = sp.log(2 * A) + 2 * sp.log(
                sp.Abs(sp.sin(theta / 2 + offset))
            )
        f = f.xreplace(replacements)
        function = sp.lambdify(x, f, "mpmath")
        target = sp.lambdify(x, symbolic(want), "mpmath")(0)
        a, b = number(lo), number(hi)
        points = (
            [a, 2, 8, 32, 128, b]
            if b == mp.inf
            else [a + (b - a) * j / 16 for j in range(17)]
        )
        got = mp.quad(function, points)
        error = abs(got - target) / max(1, abs(target))
        worst = max(worst, error)
        count += 1
        assert mp.isfinite(error) and error < mp.mpf("1e-45"), (digits, raw, error)
print(
    "PASS:",
    count,
    "variant quadratures at 80/110 digits; max scaled error",
    mp.nstr(worst, 15),
)
