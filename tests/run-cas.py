#!/usr/bin/env python3
"""Check repository transpose and conversion code against host Giac."""

from repository import source_path
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


from integration_build import function


with tempfile.TemporaryDirectory(prefix="khicas-cas-") as tmp:
    work = Path(tmp)
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O2",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ]
    flags += shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    libs += shlex.split(os.environ.get("GIAC_NUMERIC_LIBS", "-lgmp -lmpfr"))
    content = (source_path("zvecteur.cc")).read_text()
    transpose = work / "transpose.cc"
    transpose.write_text(
        '#include "giacPCH.h"\nnamespace giac {\n'
        + function(
            content,
            "  void mtran(const matrice & a,matrice & res,int ncolres,bool ckundef)",
        )
        + function(content, "  gen _tran(const gen & a,GIAC_CONTEXT)")
        + "}\n"
    )
    output = work / "transpose"
    subprocess.run(
        flags
        + [str(transpose), str(ROOT / "tests/matrix_transpose.cc")]
        + libs
        + ["-o", str(output)],
        check=True,
    )
    subprocess.run([str(output)], check=True)
    output = work / "conversions"
    # Resolve quoted headers against host Giac, not the calculator-only headers.
    conversion = work / "kconvert.cc"
    conversion.write_bytes((source_path("kconvert.cc")).read_bytes())
    (work / "equation_normalize.h").write_bytes(
        (source_path("equation_normalize.h")).read_bytes()
    )
    subprocess.run(
        flags
        + [str(conversion), str(ROOT / "tests/equation_conversions.cc")]
        + libs
        + ["-o", str(output)],
        check=True,
    )
    subprocess.run(
        [str(output), str(ROOT / "tests/device/conversions.xws")], check=True
    )
