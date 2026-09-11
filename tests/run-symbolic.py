#!/usr/bin/env python3
"""Check buffer reuse against pre-optimization symbolic operations."""

from repository import source_path
from pathlib import Path
import os, shlex, subprocess, tempfile
from integration_build import function as extract_function

ROOT = Path(__file__).resolve().parents[1]


def function(s, name):
    return extract_function(s, "void " + name + "(")


old = subprocess.check_output(
    ["git", "show", "checkpoint/equations:zlin.cc"], cwd=ROOT, text=True
)
new = (source_path("zlin.cc")).read_text()
text = '#include "giacPCH.h"\nnamespace giac {\n'
for name in ("convolutionpower", "tconvolutionpower"):
    text += function(old, name).replace(name + "(", "baseline_" + name + "(")
    text += function(new, name)
integrator = (source_path("yintg.cc")).read_text()
for signature in (
    "  void decompose_prod(",
    "  gen extract_cst(",
    "  static bool small_polynomial(",
    "  static int continuous_parity(",
    "  static bool small_sparse_polynomial(",
    "  static bool integrate_sparse_atan(",
    "  static bool integrate_trig_periods(",
    "  static bool integrate_large_power(",
):
    text += extract_function(integrator, signature).replace("  static ", "  ", 1)
text += "}\n"
with tempfile.TemporaryDirectory(prefix="khicas-symbolic-") as tmp:
    p = Path(tmp)
    (p / "buffers.cc").write_text(text)
    (p / "integration_guard.h").write_bytes(
        (source_path("integration_guard.h")).read_bytes()
    )
    (p / "test.cc").write_bytes((ROOT / "tests/symbolic_buffers.cc").read_bytes())
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
    subprocess.run(
        flags
        + [str(p / "buffers.cc"), str(p / "test.cc")]
        + libs
        + ["-o", str(p / "test")],
        check=True,
    )
    subprocess.run([str(p / "test")], check=True, timeout=60)
