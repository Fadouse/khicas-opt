#!/usr/bin/env python3
"""Run new conversion paths on a guarded 64KiB host computation stack."""

from repository import source_path
from pathlib import Path
import argparse, hashlib, json, os, shlex, subprocess, tempfile
from integration_build import (
    simplification_source,
    normalization_source,
    special_source,
)

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--report", type=Path)
p.add_argument(
    "--target-simplify",
    action="store_true",
    help="Compile actual FXCG simplification and test conversion both directly and with outer simplify",
)
args = p.parse_args()
cases = [
    ("cart", "(x^3+y^3=3*x*y,[x,y],t)"),
    ("cart", "(-27*x^3+81*x*y-27*y^3=0,[x,y],t)"),
    ("cart", "(x^3+y^3=0,[x,y],t)"),
    ("cart", "(x^3+y^3=3*a*x*y,[x,y],t)", "assume(a>0)"),
    ("cart", "((x^2+y^2)^2=2*(x^2-y^2),[x,y],t)"),
    ("cart", "((x^2+y^2)^2=-2*(x^2-y^2),[x,y],t)"),
    ("cart", "((x^2+y^2)^2=0,[x,y],t)"),
    ("cart", "((x^2+y^2)^2/4=x^2-y^2,[x,y],t)"),
    ("cart", "(x+y^5+y=0,[x,y],t)"),
    ("cart", "(x^3+y^3+x^2+y^2=0,[x,y],t)"),
    ("cart", "(x*(x^3+y^3-3*x*y)=0,[x,y],t)"),
    ("polar", "(r^3=sin(theta),[r,theta],t)"),
    ("polar", "(r^5=1-2*cos(theta),[r,theta],t)"),
    ("polar", "(r^3/2+sin(theta)=0,[r,theta],t)"),
    ("polar", "(r^9=sin(theta),[r,theta],t)"),
    ("polar", "(r^3=-8,[r,theta],t)"),
    ("cartpolar", "((x^2+y^2)^2=2*(x^2-y^2),[x,y],[r,theta])"),
    ("cartpolar", "(x^3+y^3=3*x*y,[x,y],[r,theta])"),
    ("cartpolar", "(x^3+y^3=3*a*x*y,[x,y],[r,theta])"),
    ("cartpolar", "((x^2+y^2)^2/4=x^2-y^2,[x,y],[r,theta])"),
]
report = []
with tempfile.TemporaryDirectory(prefix="khicas-equation-stack-") as tmp:
    d = Path(tmp)
    (d / "kconvert.cc").write_bytes((source_path("kconvert.cc")).read_bytes())
    (d / "equation_normalize.h").write_bytes(
        (source_path("equation_normalize.h")).read_bytes()
    )
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O2",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac", "-pthread"]
    libs += shlex.split(os.environ.get("GIAC_NUMERIC_LIBS", "-lgmp -lmpfr"))
    sources = [str(d / "kconvert.cc"), str(ROOT / "tests/equation_conversion_stack.cc")]
    if args.target_simplify:
        text = simplification_source()
        norm = normalization_source()
        (d / "simplify.cc").write_text(text)
        (d / "normalize.cc").write_text(norm)
        sources += [
            str(d / "simplify.cc"),
            str(d / "normalize.cc"),
            str(special_source(d)),
        ]
    subprocess.run(flags + sources + libs + ["-o", str(d / "probe")], check=True)
    for outer, case in [
        (outer, case)
        for outer in ([False, True] if args.target_simplify else [False])
        for case in cases
    ]:
        env = os.environ.copy()
        env.pop("KHICAS_OUTER_SIMPLIFY", None)
        if outer:
            env["KHICAS_OUTER_SIMPLIFY"] = "1"
        try:
            r = subprocess.run(
                [str(d / "probe"), *case],
                capture_output=True,
                text=True,
                timeout=10,
                env=env,
            )
            row = dict(
                case=case,
                outer_simplify=outer,
                returncode=r.returncode,
                output=r.stdout,
                output_length=len(r.stdout.rstrip("\n")),
                diagnostics=r.stderr,
            )
        except subprocess.TimeoutExpired:
            row = dict(case=case, outer_simplify=outer, status="timeout")
        report.append(row)
        print(
            ("PASS" if row.get("returncode") == 0 else "FAIL"),
            "outer" if outer else "direct",
            case[0],
            case[1],
        )
if args.report:
    args.report.write_text(
        json.dumps(
            dict(
                stack_bytes=65536,
                guard_bytes=4096,
                target_simplify=args.target_simplify,
                scope="repository conversion source with host Giac dependencies and parser",
                source_sha256={
                    name: hashlib.sha256(source_path(name).read_bytes()).hexdigest()
                    for name in (
                        "kconvert.cc",
                        "equation_normalize.h",
                        "ksubst.cc",
                        "ysym2poly.cc",
                        "zprog.cc",
                    )
                },
                cases=report,
            ),
            indent=2,
        )
        + "\n"
    )
raise SystemExit(0 if all(r.get("returncode") == 0 for r in report) else 1)
