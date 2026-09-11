#!/usr/bin/env python3
"""Exercise the actual small-object allocator, including exhaustion and reuse."""

from repository import source_path
from pathlib import Path
import os, re, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = "checkpoint/equations"


def block(s):
    return s[s.index("  struct eight_int {") : s.index("  unsigned hamdist(")]


old = block(
    subprocess.check_output(["git", "show", f"{BASE}:kgen.cc"], cwd=ROOT, text=True)
)
new = block((source_path("kgen.cc")).read_text())
for size in (16, 24, 32, 48):
    old = old.replace(
        f"if (!(freeslot{size}[i] || freeslot{size}[i+1]))",
        f"if ((probes+=2,!(freeslot{size}[i] || freeslot{size}[i+1])))",
    )
new, count = re.subn(
    r"first\s*<\s*groups\s*&&\s*!slots\[first\]",
    "first<groups && (++probes,!slots[first])",
    new,
)
assert count == 1, "Allocator scan instrumentation must be installed"
prefix = "size_t probes=0; bool fail_alloc=false; void *malloc(size_t n){return fail_alloc?nullptr:std::malloc(n);} void free(void *p){std::free(p);}\n"
with tempfile.TemporaryDirectory(prefix="khicas-pool-") as tmp:
    p = Path(tmp) / "pool.cc"
    p.write_text(
        "#include <cstdlib>\n#include <new>\n#include <cstdint>\n"
        + "namespace baseline {\n"
        + prefix
        + old
        + "}\nnamespace optimized {\n"
        + prefix
        + new
        + "}\n"
        + (ROOT / "tests/small_pool.cc").read_text()
    )
    for optional in (False, True):
        exe = Path(tmp) / f"pool-{optional}"
        cmd = [
            os.environ.get("CXX", "c++"),
            "-std=c++11",
            "-O2",
            "-g",
            "-DNO_STDEXCEPT",
            "-fsanitize=address,undefined",
            "-fno-sanitize-recover=all",
            str(p),
            "-o",
            str(exe),
        ]
        if optional:
            cmd.insert(1, "-DALLOC32=896")
        subprocess.run(cmd, check=True)
        subprocess.run([str(exe)], check=True, timeout=60)
