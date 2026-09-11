#!/usr/bin/env python3
"""Exercise compact parameter curves and real signed odd-radius branches."""

from repository import source_path
from pathlib import Path
import argparse, os, shlex, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
args = p.parse_args()
with tempfile.TemporaryDirectory(prefix="khicas-equation-extended-") as tmp:
    d = Path(tmp)
    source = d / "kconvert.cc"
    source.write_bytes(
        (source_path("kconvert.cc")).read_bytes()
        + b"\nnamespace giac {bool curve_polynomial_test(const gen &g,const gen &x,const gen &y,GIAC_CONTEXT){curve_polynomial_terms out;return curve_polynomial(g,x,y,out,contextptr);}}\n"
    )
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
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    libs += shlex.split(os.environ.get("GIAC_NUMERIC_LIBS", "-lgmp -lmpfr"))
    for name in (
        "equation_conversions_extended.cc",
        "equation_polar_compact.cc",
        "equation_conversions.cc",
    ):
        subprocess.run(
            flags
            + [str(source), str(ROOT / "tests" / name)]
            + libs
            + ["-o", str(d / "test")],
            check=True,
        )
        subprocess.run([str(d / "test")], check=True, timeout=60)
