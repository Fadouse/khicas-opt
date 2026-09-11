#!/usr/bin/env python3
"""Exercise actual FXCG simplification without algebraic extension blow-up."""

from repository import source_path
from pathlib import Path
import os, shlex, subprocess, tempfile
from integration_build import (
    ROOT,
    simplification_source,
    normalization_source,
    special_source,
)

text = simplification_source()
norm = normalization_source()
with tempfile.TemporaryDirectory(prefix="khicas-simplify-nested-") as tmp:
    p = Path(tmp)
    (p / "simplify.cc").write_text(text)
    (p / "normalize.cc").write_text(norm)
    (p / "equation_normalize.h").write_bytes(
        (source_path("equation_normalize.h")).read_bytes()
    )
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O1",
        "-g",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac", "-pthread"]
    libs += shlex.split(os.environ.get("GIAC_NUMERIC_LIBS", "-lgmp -lmpfr"))
    flags += ["-I", str(p)]
    for test in (
        "simplify_nested_powers.cc",
        "simplify_special_functions.cc",
        "equation_simplification.cc",
        "simplify_real_roots.cc",
        "high_frequency_trig_simplify.cc",
        "cycle8_resource_simplify.cc",
    ):
        subprocess.run(
            flags
            + [
                str(special_source(p)),
                str(p / "simplify.cc"),
                str(p / "normalize.cc"),
                str(ROOT / "tests" / test),
            ]
            + libs
            + ["-o", str(p / "test")],
            check=True,
        )
        subprocess.run([str(p / "test")], check=True, timeout=60)
