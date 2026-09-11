#!/usr/bin/env python3
"""Bounded closed trig-constant algebra and real/complex branch regressions."""

from repository import source_path
import re, subprocess, tempfile
from pathlib import Path
from report_store import suite
from integration_build import (
    ROOT,
    simplification_source,
    normalization_source,
    compiler_options,
    special_source,
)

text = simplification_source()
marker = r"gen\s+masked\s*=\s*quotesubst\(e_orig,\s*closed,\s*names,\s*contextptr\);"
text, replacements = re.subn(marker, lambda m: "++closed_trig_mask_calls;" + m[0], text)
assert replacements == 1, (
    "Closed trig instrumentation must bind once to the production guard"
)
text = text.replace(
    "namespace giac {", "namespace giac {\nunsigned closed_trig_mask_calls=0;", 1
)
text += "namespace giac { gen closed_trig_baseline(const gen &g,GIAC_CONTEXT){return simplify_special_core(g,contextptr); } }\n"
norm = normalization_source()
inputs = [c["runs"][0]["stdout"] for c in suite("mellin-constants")["cases"]]
with tempfile.TemporaryDirectory(prefix="khicas-closed-trig-") as tmp:
    d = Path(tmp)
    (d / "simplify.cc").write_text(text)
    (d / "normal.cc").write_text(norm)
    (d / "equation_normalize.h").write_bytes(
        (source_path("equation_normalize.h")).read_bytes()
    )
    flags, libs = compiler_options()
    subprocess.run(
        flags
        + [
            "-I",
            str(d),
            str(special_source(d)),
            str(d / "simplify.cc"),
            str(d / "normal.cc"),
            str(ROOT / "tests/closed_trig_constants.cc"),
        ]
        + libs
        + ["-pthread", "-o", str(d / "test")],
        check=True,
    )
    subprocess.run([str(d / "test")] + inputs, check=True, timeout=60)
