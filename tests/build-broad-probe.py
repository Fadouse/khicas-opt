#!/usr/bin/env python3
"""Extend an existing real-core probe with the full repository series engine.

The ABI still uses host dependencies. Product uses its unchanged repository
entry; the full series source includes in_limit and its recursive machinery.
"""

from repository import source_path
import argparse, subprocess, os, shlex, hashlib, json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from integration_build import ROOT, compiler_options, function

p = argparse.ArgumentParser()
p.add_argument("--core-dir", type=Path, required=True)
p.add_argument("--build-dir", type=Path, required=True)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)
(a.build_dir / "zseries.cc").write_bytes((source_path("zseries.cc")).read_bytes())
prod = (
    '#include "giacPCH.h"\nnamespace giac {\n'
    + function((source_path("zti89.cc")).read_text(), "  gen _product(")
    + "}\n"
)
(a.build_dir / "product.cc").write_text(prod)
f, l = compiler_options()
sources = [
    str(a.core_dir / n)
    for n in [
        "yintg.cc",
        "zintgab.cc",
        "normalize.cc",
        "simplify.cc",
        "derivative.cc",
        "matrix.cc",
        "numeric_evalf.cc",
    ]
]
sources += [
    str(ROOT / "tests/integration_probe.cc"),
    str(a.build_dir / "zseries.cc"),
    str(a.build_dir / "product.cc"),
]
headers = b"".join(
    p.read_bytes()
    for directory in [a.core_dir, ROOT / "tests"]
    for p in sorted(directory.glob("*.h"))
)


def compile_one(src):
    obj = a.build_dir / (Path(src).stem + ".o")
    stamp = obj.with_suffix(".sha256")
    command = f + ["-DKHICAS_TEST_INTEGRATION_LIMITS", "-c", src, "-o", str(obj)]
    key = hashlib.sha256(
        Path(src).read_bytes() + headers + json.dumps(command).encode()
    ).hexdigest()
    if not obj.exists() or not stamp.exists() or stamp.read_text() != key:
        subprocess.run(command, check=True)
        stamp.write_text(key)
    return str(obj)


with ThreadPoolExecutor(max_workers=2) as pool:
    objects = list(pool.map(compile_one, sources))
subprocess.run(
    f
    + objects
    + l
    + shlex.split(os.environ.get("GIAC_SERIES_LIBS", "-lmpfi"))
    + ["-pthread", "-o", str(a.build_dir / "probe")],
    check=True,
)
print(a.build_dir / "probe")
