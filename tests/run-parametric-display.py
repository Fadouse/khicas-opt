#!/usr/bin/env python3
"""Verify the display adapter against actual conversion data; no UI emulation."""

from repository import source_path
from pathlib import Path
import os, shlex, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="khicas-parametric-display-") as tmp:
    d = Path(tmp)
    for name in ("equation_normalize.h", "parametric_display.h"):
        (d / name).write_bytes((source_path(name)).read_bytes())
    (d / "kconvert.cc").write_text(
        (source_path("kconvert.cc"))
        .read_text()
        .replace(
            '#include "giacPCH.h"',
            '#include "giacPCH.h"\n#include "parametric_display.h"',
            1,
        )
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
        "-I",
        str(d),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    libs += shlex.split(os.environ.get("GIAC_NUMERIC_LIBS", "-lgmp -lmpfr"))
    subprocess.run(
        flags
        + [str(d / "kconvert.cc"), str(ROOT / "tests/parametric_display.cc")]
        + libs
        + ["-o", str(d / "test")],
        check=True,
    )
    subprocess.run([str(d / "test")], check=True, timeout=30)
