#!/usr/bin/env python3
"""Bounded log/trig identities, guards and optional full FXCG/64KiB pipeline."""

from pathlib import Path
import argparse, hashlib, json, os, subprocess, tempfile
import integration_build as ib

p = argparse.ArgumentParser()
p.add_argument("--full", action="store_true")
p.add_argument("--report", type=Path)
args = p.parse_args()
s = (ib.source_path("yintg.cc")).read_text()
signatures = (
    "  static bool integrate_log_trig_quarters(",
    "  static bool integrate_log_product_zeta(",
    "  static int integration_quarter_sigma(",
    "  static bool integrate_log_trig_sum(",
    "  static bool integrate_atan_log_measure(",
)
helpers = "".join(ib.function(s, sig) for sig in signatures)
text = '#include "giacPCH.h"\nnamespace giac {\n'
for sig in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool integration_rational(",
    "  static gen integration_syntax(",
    "  static bool integration_power(",
    "  static bool integration_one_plus(",
    "  static gen integration_coefficient(",
    "  static bool integration_monomial(",
):
    text += ib.function(s, sig)
text += helpers
calls = " || ".join(
    name + "(e,x,lo,hi,r,contextptr)"
    for name in (
        "integrate_log_trig_quarters",
        "integrate_log_product_zeta",
        "integrate_log_trig_sum",
        "integrate_atan_log_measure",
    )
)
text += (
    "bool cycle6_log_rule(const gen &f,const gen &x,const gen &lo,const gen &hi,gen &r,GIAC_CONTEXT){gen e=integration_syntax(f,contextptr);return "
    + calls
    + ";}\n}\n"
)
report = []
with tempfile.TemporaryDirectory(prefix="khicas-cycle6-log-") as tmp:
    d = Path(tmp)
    (d / "rules.cc").write_text(text)
    flags, libs = ib.compiler_options()
    unit = d / "unit"
    subprocess.run(
        flags
        + [str(d / "rules.cc"), str(ib.ROOT / "tests/cycle6_log_integrals.cc")]
        + libs
        + ["-pthread", "-o", str(unit)],
        check=True,
    )
    for stack in (False, True):
        subprocess.run(
            [str(unit)],
            check=True,
            timeout=60,
            env=dict(os.environ, **({"KHICAS_TEST_STACK_KIB": "64"} if stack else {})),
        )
    if args.full:
        probe = ib.build(d / "full", target_simplify=True)
        validator = ib.build_validation_probe(d / "validation")
        rows = subprocess.check_output([str(unit), "--list"], text=True).splitlines()
        for stack, outer in ((False, False), (True, False), (True, True)):
            for row in rows:
                f, lo, hi, want = row.split("\t")
                expr = f"integrate({f},x,{lo},{hi})"
                if outer:
                    expr = "simplify(" + expr + ")"
                env = os.environ.copy()
                env.pop("KHICAS_TEST_STACK_KIB", None)
                if stack:
                    env["KHICAS_TEST_STACK_KIB"] = "64"
                try:
                    r = subprocess.run(
                        [str(probe), expr],
                        text=True,
                        capture_output=True,
                        timeout=12,
                        env=env,
                    )
                    validation = (
                        subprocess.run(
                            [str(validator), r.stdout.strip(), want, "definite"],
                            text=True,
                            capture_output=True,
                            timeout=12,
                        )
                        if r.returncode == 0
                        else None
                    )
                    ok = (
                        r.returncode == 0
                        and validation.returncode == 0
                        and "CHECK exact" in validation.stderr
                    )
                    report.append(
                        dict(
                            f=f,
                            stack_kib=64 if stack else None,
                            outer=outer,
                            returncode=r.returncode,
                            output=r.stdout.strip(),
                            exact=ok,
                            validation=validation.stderr if validation else "",
                        )
                    )
                except subprocess.TimeoutExpired:
                    ok = False
                    report.append(
                        dict(
                            f=f,
                            stack_kib=64 if stack else None,
                            outer=outer,
                            status="timeout",
                            exact=False,
                        )
                    )
                print("PASS" if ok else "FAIL", stack, outer, expr, flush=True)
        if args.report:
            args.report.write_text(
                json.dumps(
                    dict(
                        source_sha256=hashlib.sha256(s.encode()).hexdigest(),
                        cases=report,
                    ),
                    indent=2,
                )
                + "\n"
            )
        if not all(r["exact"] for r in report):
            raise SystemExit(1)
