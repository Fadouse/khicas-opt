#!/usr/bin/env python3
"""Verify complex convergence contracts and boundary behavior independently."""

from repository import source_path
import argparse, hashlib, json, os, subprocess
from pathlib import Path
import sympy as sp

p = argparse.ArgumentParser()
p.add_argument("--probe", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
A, B, S = sp.symbols("a b s")
local = {
    "a": A,
    "b": B,
    "s": S,
    "re": sp.re,
    "ln": sp.log,
    "Gamma": sp.gamma,
    "i": sp.I,
}


def parse(t):
    return sp.sympify(t.replace("^", "**").replace(" and ", " & "), locals=local)


cs = [
    ("D1", ["a"], "exp(-a*x)", "0", "+infinity", "re(a)>0", "1/a"),
    ("D2", ["s"], "x^(s-1)*exp(-x)", "0", "+infinity", "re(s)>0", "Gamma(s)"),
    (
        "D3",
        ["a", "b"],
        "x^(a-1)*(1-x)^(b-1)",
        "0",
        "1",
        "(re(a)>0) & (re(b)>0)",
        "Gamma(a)*Gamma(b)/Gamma(a+b)",
    ),
    (
        "D4",
        ["s"],
        "x^(s-1)/(1+x)",
        "0",
        "+infinity",
        "(re(s)>0) & (1-re(s)>0)",
        "pi/sin(pi*s)",
    ),
]
rows = []
for id, parameters, f, l, h, condition, answer in cs:
    for outer in [False, True]:
        e = "integrate(" + f + ",x," + l + "," + h + ")"
        e = "simplify(" + e + ")" if outer else e
        expression = (
            "["
            + ",".join(["assume(" + v + ",complex)" for v in parameters] + [e])
            + "]["
            + str(len(parameters))
            + "]"
        )
        for stack in ["normal", "64"]:
            for eager in [False, True]:
                env = dict(os.environ)
                env.pop("KHICAS_TEST_STACK_KIB", None)
                env.pop("LD_BIND_NOW", None)
                if stack == "64":
                    env["KHICAS_TEST_STACK_KIB"] = stack
                if eager:
                    env["LD_BIND_NOW"] = "1"
                r = subprocess.run(
                    [str(a.probe), expression],
                    capture_output=True,
                    text=True,
                    env=env,
                    timeout=5,
                )
                row = dict(
                    id=id,
                    input=expression,
                    outer_simplify=outer,
                    stack_KiB=stack,
                    eager_binding=eager,
                    exit=r.returncode,
                    result=r.stdout.strip(),
                    stderr=r.stderr,
                )
                try:
                    assert r.returncode == 0
                    value = row["result"]
                    assert value[0] == "(" and value[-1] == ")"
                    value = value[1:-1]
                    guard, branches = value.split("?", 1)
                    yes, no = branches.rsplit(":", 1)
                    assert no.strip() == "undef"
                    assert parse(guard) == parse(condition)
                    assert sp.simplify(parse(yes) - parse(answer)) == 0
                    row["exact_contract"] = True
                except Exception as error:
                    row["exact_contract"] = False
                    row["verification_error"] = repr(error)
                rows.append(row)
negative = ["exp(i*x)", "exp((1+i)*x)", "x^(i-1)*exp(-x)", "x^i/(1+x)"]
for f in negative:
    for outer in [False, True]:
        for stack in ["normal", "64"]:
            e = "integrate(" + f + ",x,0,+infinity)"
            e = "simplify(" + e + ")" if outer else e
            env = dict(os.environ)
            env.pop("KHICAS_TEST_STACK_KIB", None)
            if stack == "64":
                env["KHICAS_TEST_STACK_KIB"] = "64"
            r = subprocess.run(
                [str(a.probe), e], capture_output=True, text=True, env=env, timeout=5
            )
            rows.append(
                dict(
                    id="boundary",
                    input=e,
                    stack_KiB=stack,
                    exit=r.returncode,
                    result=r.stdout.strip(),
                    stderr=r.stderr,
                    exact_contract=r.returncode == 3
                    and r.stdout.strip() == "undef"
                    and "Divergent improper integral" in r.stderr,
                )
            )
a.report.write_text(
    json.dumps(
        {
            "scope": "32 whole-pipeline complex symbolic condition-and-answer contracts plus 16 ordinary convergence-boundary checks. SymPy checks condition predicates and the answer symbolically; explicit complex declarations prevent silently restricting parameters to real numbers.",
            "source_sha256": {
                n: hashlib.sha256((source_path(n)).read_bytes()).hexdigest()
                for n in ["yintg.cc", "ksubst.cc"]
            },
            "runs": rows,
        },
        indent=2,
    )
    + "\n"
)
assert all(r["exact_contract"] for r in rows), [
    r for r in rows if not r["exact_contract"]
]
print("PASS: 48 complex contract and necessary convergence boundary checks")
