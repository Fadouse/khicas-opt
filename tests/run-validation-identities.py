#!/usr/bin/env python3
"""Check exact hyperbolic proofs and reject deliberately incorrect primitives."""

import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path
from report_store import suite
from integration_build import ROOT, build_validation_probe

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--report", type=Path)
args = p.parse_args()
checks = []
with tempfile.TemporaryDirectory(prefix="khicas-validation-identities-") as tmp:
    exe = build_validation_probe(Path(tmp))
    for rn, cn, ids in [
        (
            "calculus-cycle5-2026a",
            "calculus-corpus",
            ("2025-Q17", "2026-Q6", "2017-Q3"),
        ),
        (
            "generalization-cycle5-2026a",
            "generalization-corpus",
            ("GEN-hyperbolic-product-01",),
        ),
    ]:
        data = suite(cn)
        rows = {r["id"]: r for r in data["runs"]["current"]}
        cases = {
            c["id"]: c
            for c in json.loads((ROOT / "tests" / f"{cn}.json").read_text())["cases"]
        }
        for key in ids:
            c = cases[key]
            row = rows[key]
            for wrong in (False, True):
                result = "(" + row["result"] + ")" + ("+x" if wrong else "")
                r = subprocess.run(
                    [
                        str(exe),
                        result,
                        c["expected"],
                        "indefinite",
                        c["f"],
                        "1/3",
                        "1",
                        "2",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                passed = (
                    r.returncode != 0
                    if wrong
                    else r.returncode == 0 and "CHECK exact" in r.stderr
                )
                assert passed, (key, wrong, r.stdout, r.stderr)
                checks.append(
                    dict(
                        id=key, perturbed=wrong, exit=r.returncode, diagnostics=r.stderr
                    )
                )
assert len(checks) == 8
if args.report:
    args.report.write_text(
        json.dumps(
            dict(
                validation_probe_sha256=hashlib.sha256(
                    (ROOT / "tests/integration_probe.cc").read_bytes()
                ).hexdigest(),
                checks=checks,
            ),
            indent=2,
        )
        + "\n"
    )
print("PASS: four exact hyperbolic proofs and four incorrect derivatives rejected")
