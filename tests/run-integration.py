#!/usr/bin/env python3
"""Compare real old/new integral modules; Linux host libgiac-dev required."""

import argparse, json, re, os, statistics, subprocess, tempfile
from pathlib import Path
from integration_build import build

parser = argparse.ArgumentParser()
parser.add_argument("--stack-kb", type=int, default=64)
parser.add_argument("--report", type=Path)
args = parser.parse_args()

CASES = [
    "simplify(integrate((3*sin(x)-sin(3*x))^(1/3),x))",
    "simplify(integrate((3*sin(x)-sin(3*x))^(1/3)))",
    "integrate(surd(3*sin(x)-sin(3*x),3),x)",
    "integrate((3*sin(2*x)-sin(6*x))^(1/3),x)",
    "integrate((3*cos(x)+cos(3*x))^(1/3),x)",
    "integrate(x*sin(x),x)",
    "integrate(x*exp(x^2),x)",
    "integrate(ln(x)^3,x)",
    "integrate(1/(1+x^4),x)",
    "integrate(exp(-x^2),x)",
    "integrate(1/sqrt(1-x^2),x)",
    "integrate(x^9-2*x+3,x)",
    "integrate(x^2,x,0,1)",
    "integrate(exp(x),x,0.0,1.0)",
    "integrate(sin(t),t)",
    "integrate(sin(x),x=0..pi)",
]
with tempfile.TemporaryDirectory(prefix="khicas-integrate-") as tmp:
    work = Path(tmp)
    report = {}
    for ref in ("baseline", "current"):
        exe = build(work / ref, ref)
        results = []
        for expr in CASES:
            r = subprocess.run(
                [str(exe), expr], capture_output=True, text=True, timeout=30
            )
            assert r.returncode == 0, (ref, expr, r.stderr)
            results.append(r.stdout.strip())
        if ref == "baseline":
            reference = results
        else:
            assert results == reference, [
                (e, a, b) for e, a, b in zip(CASES, reference, results) if a != b
            ]
        timings = []
        calls = []
        for _ in range(7):
            r = subprocess.run(
                [str(exe), CASES[0]],
                capture_output=True,
                text=True,
                check=True,
                timeout=30,
            )
            timings.append(float(re.search(r"SECONDS ([\d.e+-]+)", r.stderr)[1]))
            calls.append(int(re.search(r"PARSER_CALLS (\d+)", r.stderr)[1]))
        r = subprocess.run(
            [str(exe), CASES[0]],
            env=dict(os.environ, KHICAS_TEST_STACK_KIB=str(args.stack_kb)),
            capture_output=True,
            text=True,
            timeout=30,
        )
        if ref == "current":
            assert r.returncode == 0, (args.stack_kb, r.stderr)
        report[ref] = {
            "median_host_seconds": statistics.median(timings),
            "parser_calls": calls,
            "stack_kb": args.stack_kb,
            "stack_scope": "guarded pthread computation stack",
            "limited_stack_exit": r.returncode,
            "cases": len(results),
        }
        if ref == "current":
            shortcuts = [
                (
                    "integrate(x^2024*(1-x^2025)^2025,x,1,0)",
                    "-1/(2025*2026)",
                    "definite",
                ),
                (
                    "integrate(x^2024*(1-x^2025)^2025,x=0..1)",
                    "1/(2025*2026)",
                    "definite",
                ),
                ("integrate(2*x*(1+x^2)^32,x,-1,2)", "(5^33-2^33)/33", "definite"),
                ("integrate(-6*x^2*(3-2*x^3)^32,x)", "(3-2*x^3)^33/33", "indefinite"),
                ("integrate(cos(2*x)*sin(3*x),x,2,-2)", "0", "definite"),
                (
                    "integrate(abs(x-1)/(abs(x-2)+abs(x-3)),x,0,4)",
                    "2+9*ln(3)/4-3*ln(5)/4",
                    "definite",
                ),
                (
                    "integrate(abs(x-1)/(abs(x-2)+abs(x-3)),x,4,0)",
                    "-2-9*ln(3)/4+3*ln(5)/4",
                    "definite",
                ),
                ("integrate(abs(2*x-1)+abs(x+1),x,-2,3)", "21", "definite"),
                ("integrate(1/abs(x-1),x,0,2)", "+infinity", "definite"),
                ("integrate(abs(x-1/2),x,0,1)", "1/4", "definite"),
            ]
            for expr, expected, mode in shortcuts:
                integrand = expr[len("integrate(") : expr.rfind(",x")]
                p = subprocess.run(
                    [str(exe), expr, expected, mode, integrand],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                assert p.returncode == 0 and "CHECK exact" in p.stderr, (
                    expr,
                    p.stdout,
                    p.stderr,
                )
            report[ref]["shortcut_cases"] = len(shortcuts)
    assert max(report["current"]["parser_calls"]) < min(
        report["baseline"]["parser_calls"]
    )
    print(json.dumps(report, indent=2))
    print(
        "PASS: actual integral modules agree; bounded-stack regression and parser count"
    )
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
