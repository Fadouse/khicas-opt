#!/usr/bin/env python3
"""Record user A1-F6 outputs and condition behavior; no automatic pass claims."""

import argparse, hashlib, json, os, subprocess
from pathlib import Path
from integration_build import ROOT

p = argparse.ArgumentParser()
p.add_argument("--probe", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
p.add_argument("--ref", default="current")
p.add_argument("--timeout", type=float, default=5)
a = p.parse_args()
cp = ROOT / "tests/user-acceptance-matrix.json"
cases = json.loads(cp.read_text())["cases"]
report = {
    "scope": "User acceptance output audit, actual repository code with host dependencies. Successful process exit is not mathematical or domain acceptance.",
    "baseline": a.ref,
    "corpus_sha256": hashlib.sha256(cp.read_bytes()).hexdigest(),
    "probe_sha256": hashlib.sha256(a.probe.read_bytes()).hexdigest(),
    "runs": [],
}
for c in cases:
    integral = (
        "integrate("
        + c["f"]
        + ",x"
        + ("," + ",".join(c["bounds"]) if "bounds" in c else "")
        + ")"
    )
    modes = [("plain", integral), ("simplify", "simplify(" + integral + ")")]
    for mode, e in modes:
        assumptions = c["assumptions"]
        expression = (
            "[" + ",".join(assumptions + [e]) + "][" + str(len(assumptions)) + "]"
            if assumptions
            else e
        )
        for stack in ("normal", "64"):
            env = dict(os.environ)
            env.pop("KHICAS_TEST_STACK_KIB", None)
            if stack == "64":
                env["KHICAS_TEST_STACK_KIB"] = stack
            row = {
                "id": c["id"],
                "mode": mode,
                "stack": stack,
                "input": expression,
                "condition_scope": c.get("coverage_limit", c["domain"]),
            }
            try:
                r = subprocess.run(
                    [str(a.probe), expression],
                    capture_output=True,
                    text=True,
                    errors="replace",
                    timeout=a.timeout,
                    env=env,
                )
                row.update(exit=r.returncode, result=r.stdout.strip(), stderr=r.stderr)
                row["execution_status"] = (
                    "returned"
                    if r.returncode == 0
                    else "unevaluated"
                    if r.returncode == 2
                    else "undefined"
                    if r.returncode == 3
                    else "crashed"
                    if r.returncode < 0
                    else "error"
                )
            except subprocess.TimeoutExpired:
                row["execution_status"] = "timeout"
            report["runs"].append(row)
            a.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
            print(
                c["id"],
                mode,
                stack,
                row["execution_status"],
                row.get("result", "")[:150],
                flush=True,
            )
    if assumptions:
        row = {
            "id": c["id"],
            "mode": "without-assumptions",
            "stack": "normal",
            "input": integral,
        }
        try:
            r = subprocess.run(
                [str(a.probe), integral],
                capture_output=True,
                text=True,
                errors="replace",
                timeout=a.timeout,
            )
            row.update(exit=r.returncode, result=r.stdout.strip(), stderr=r.stderr)
            row["execution_status"] = (
                "returned"
                if r.returncode == 0
                else "unevaluated"
                if r.returncode == 2
                else "undefined"
                if r.returncode == 3
                else "crashed"
                if r.returncode < 0
                else "error"
            )
        except subprocess.TimeoutExpired:
            row["execution_status"] = "timeout"
        report["runs"].append(row)
        a.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
