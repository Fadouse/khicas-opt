#!/usr/bin/env python3
"""Cold/eager and normal/64 KiB checks of multivariate algebraic logarithms."""

from repository import source_path
import argparse, hashlib, json, os, subprocess
from pathlib import Path
import sympy as sp

p = argparse.ArgumentParser()
p.add_argument("--probe", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
A, B = sp.symbols("a b")
local = {"a": A, "b": B, "ln": sp.log}
base = "ln((a+sqrt(a*a-b*b))/2)"
cases = [
    ("positive", "assume(a>abs(b))", "2*pi*" + base),
    ("unassumed", "", "3*pi*" + base),
    ("complex", "assume(a,complex)", "5*pi*" + base),
    ("scaled-argument", "assume(a>abs(b))", "2*pi*ln((a+sqrt(4*a*a-4*b*b)/2)/2)"),
]
rows = []
for id, assumption, value in cases:
    e = "simplify(" + value + ")"
    e = "[" + assumption + "," + e + "][1]" if assumption else e
    for stack in ("normal", "64"):
        for eager in (False, True):
            env = dict(os.environ)
            env.pop("KHICAS_TEST_STACK_KIB", None)
            env.pop("LD_BIND_NOW", None)
            if stack == "64":
                env["KHICAS_TEST_STACK_KIB"] = stack
            if eager:
                env["LD_BIND_NOW"] = "1"
            r = subprocess.run(
                [str(a.probe), e], capture_output=True, text=True, env=env, timeout=5
            )
            row = dict(
                id=id,
                input=e,
                stack_KiB=stack,
                eager_binding=eager,
                exit=r.returncode,
                result=r.stdout.strip(),
                stderr=r.stderr,
            )
            row["exact"] = (
                r.returncode == 0
                and sp.simplify(
                    sp.sympify(row["result"].replace("^", "**"), locals=local)
                    - sp.sympify(value, locals=local)
                )
                == 0
            )
            rows.append(row)
a.report.write_text(
    json.dumps(
        {
            "scope": "Actual FXCG simplification with host Giac dependencies; not a hardware TLB reproduction. SymPy checks the printed expression identity, preserving the original complex logarithm and radical branches.",
            "source_sha256": hashlib.sha256(
                (source_path("ksubst.cc")).read_bytes()
            ).hexdigest(),
            "runs": rows,
        },
        indent=2,
    )
    + "\n"
)
assert all(r["exact"] for r in rows), rows
print("PASS: 16 algebraic-log checks, including complex symbol and cold/eager binding")
