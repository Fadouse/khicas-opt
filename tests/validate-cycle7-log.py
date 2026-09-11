#!/usr/bin/env python3
"""Independent mpmath audit of C7 logarithmic moment identities; no CAS calls."""

import mpmath as mp


def reference(n):
    if n % 2:
        return (
            mp.polygamma(n, mp.mpf(1) / 4) - mp.polygamma(n, mp.mpf(3) / 4)
        ) / 4 ** (n + 1)
    return abs(mp.eulernum(n)) * mp.pi ** (n + 1) / 2 ** (n + 2)


worst = mp.mpf(0)
count = 0
for digits in (80, 110):
    mp.mp.dps = digits
    for n in range(9):
        got = mp.quad(lambda u: u**n / (2 * mp.cosh(u)), [0, 1, 4, 16, mp.inf])
        want = reference(n)
        error = abs(got - want) / max(1, abs(want))
        worst = max(worst, error)
        count += 1
        assert error < mp.mpf(10) ** (-digits + 12), (digits, n, error)
    # Independent shifted half/full logarithmic moments, including maximum
    # supported order, reciprocal argument and a nonunit positive log scale.
    for n, k, sign, full in (
        (2, 2, 1, False),
        (3, 3, -1, False),
        (8, 2, 1, False),
        (7, 2, -1, True),
    ):
        shift = mp.log(k)
        moment = lambda u: (shift + sign * u) ** n / (2 * mp.cosh(u))
        got = mp.quad(moment, [0, 1, 4, 16, mp.inf])
        if full:
            got += mp.quad(
                lambda u: (shift - sign * u) ** n / (2 * mp.cosh(u)),
                [0, 1, 4, 16, mp.inf],
            )
        want = sum(
            mp.binomial(n, j)
            * shift ** (n - j)
            * sign**j
            * reference(j)
            * (1 + (-1) ** j if full else 1)
            for j in range(n + 1)
        )
        error = abs(got - want) / max(1, abs(want))
        worst = max(worst, error)
        count += 1
        assert error < mp.mpf(10) ** (-digits + 12), (digits, n, error)
    # Direct quadrature of the original trigonometric functions, not the
    # shifted-sine primitive used by the implementation.
    G = mp.catalan
    p = mp.pi
    rows = [
        (1, 1, 0, p / 2, G - p * mp.log(2) / 4),
        (
            mp.mpf(1) / 3,
            mp.mpf(1) / 3,
            0,
            p / 2,
            G - p * mp.log(2) / 4 - p * mp.log(3) / 2,
        ),
        (-1, 1, 0, p / 4, -G / 2 - p * mp.log(2) / 8),
        (1, -1, p / 4, p / 2, -G / 2 - p * mp.log(2) / 8),
        (-1, -1, p, 3 * p / 2, G - p * mp.log(2) / 4),
    ]
    for A, B, lo, hi, want in rows:
        got = mp.quad(
            lambda x: mp.log(abs(A * mp.sin(x) + B * mp.cos(x))),
            [lo, (lo + hi) / 2, hi],
        )
        error = abs(got - want) / max(1, abs(want))
        worst = max(worst, error)
        count += 1
        assert error < mp.mpf(10) ** (-digits + 12), (digits, A, B, error)
print(
    "PASS:",
    count,
    "independent quadratures at 80/110 digits; max scaled error",
    mp.nstr(worst, 15),
)
