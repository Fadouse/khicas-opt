#!/usr/bin/env python3
"""Preserve raw fresh-batch results; mathematical adjudication is separate.

A zero exit is deliberately not labelled a mathematical pass. Each process
is bounded independently, and the four original modes remain immutable.
"""

import argparse, hashlib, json, os, re, subprocess, resource, time
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("--corpus", type=Path, required=True)
p.add_argument("--probe", type=Path, required=True)
p.add_argument("--conversion-probe", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
p.add_argument("--timeout", type=float, default=12)
a = p.parse_args()
assert not a.report.exists(), "Use a new report path to preserve first-run evidence"
d = json.loads(a.corpus.read_text())
cases = d["cases"]
rows = []


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


report = dict(
    scope="Repository integration, derivative, FXCG simplify, determinant and six curve conversions with host dependencies. Other operations require entry/dependency provenance adjudication. Host normal and guarded 64 KiB stacks are not CG50/MMU emulation.",
    commit=subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
    corpus_sha256=digest(a.corpus),
    probe_sha256={str(p): digest(p) for p in [a.probe, a.conversion_probe]},
    runs=rows,
)


def limits():
    resource.setrlimit(resource.RLIMIT_AS, (768 * 1024**2, 768 * 1024**2))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def text(v):
    return v.decode(errors="replace") if isinstance(v, bytes) else v or ""


for c in cases:
    for outer in [False, True]:
        for stack in ["normal", "64"]:
            expr = c["input"]
            expr = expr[9:-1] if expr.startswith("simplify(") else expr
            env = dict(os.environ)
            env.pop("KHICAS_TEST_STACK_KIB", None)
            env.pop("KHICAS_OUTER_SIMPLIFY", None)
            if stack == "64":
                env["KHICAS_TEST_STACK_KIB"] = "64"
            conversion = re.match(
                r"^(cart2polar|param2polar|cart2param|polar2cart|polar2param|param2cart)(\(.*\))$",
                expr,
            )
            if conversion:
                command = [str(a.conversion_probe), conversion[1], conversion[2]]
                if outer:
                    env["KHICAS_OUTER_SIMPLIFY"] = "1"
                if c.get("setup"):
                    command.append(c["setup"])
            else:
                if outer:
                    expr = "simplify(" + expr + ")"
                if c.get("setup"):
                    expr = c["setup"] + ";" + expr
                command = [str(a.probe), expr]
            row = dict(
                id=c["id"], input=expr, outer=outer, stack=stack, command=command
            )
            start = time.monotonic()
            try:
                run = subprocess.run(
                    command,
                    capture_output=True,
                    text=True,
                    env=env,
                    timeout=a.timeout,
                    preexec_fn=limits,
                )
                row.update(
                    exit=run.returncode, result=run.stdout.strip(), stderr=run.stderr
                )
                row["runtime_status"] = "signal" if run.returncode < 0 else "returned"
                if run.returncode < 0:
                    row["signal"] = -run.returncode
            except subprocess.TimeoutExpired as e:
                row.update(
                    runtime_status="timeout",
                    result=text(e.stdout),
                    stderr=text(e.stderr),
                )
            row["wall_seconds"] = time.monotonic() - start
            rows.append(row)
            a.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
            print(
                c["id"],
                outer,
                stack,
                row["runtime_status"],
                row.get("exit"),
                row.get("result", "")[:160],
                flush=True,
            )
print("Recorded", len(rows), "runs; mathematical verdict pending")
