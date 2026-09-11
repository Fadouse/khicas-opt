#!/usr/bin/env python3
"""Check source layout, stable current docs and local artifact exclusions."""

import json
import re
import subprocess
from pathlib import Path
from repository import ROOT, build_inputs

DOCS = {"architecture.md", "changelog.md", "passed-tests.md", "tests-set.md"}
REPORTS = {
    "acceptance.json",
    "regression.json",
    "verification.json",
    "resources.json",
    "performance.json",
}


def main():
    inputs = build_inputs()
    manifest = json.loads((ROOT / "UPSTREAM.json").read_text())["files"]
    for row in manifest:
        name = row["path"]
        if "/" not in name and name not in {
            "README.md",
            "UPSTREAM.md",
            "UPSTREAM.json",
        }:
            assert name in inputs, "Missing upstream input: " + name
    assert inputs["iostream"].is_symlink()
    assert inputs["iostream"].resolve() == inputs["iostream.new"].resolve()
    for p in ROOT.iterdir():
        assert p.suffix not in {
            ".c",
            ".cc",
            ".cpp",
            ".h",
            ".hpp",
            ".ll",
            ".yy",
            ".ld",
        }, p
    docs = ROOT / "docs"
    assert {p.name for p in docs.iterdir()} == DOCS | {"bench"}
    assert {p.name for p in (docs / "bench").iterdir()} == REPORTS
    for p in (docs / "bench").iterdir():
        assert isinstance(json.loads(p.read_text()), dict), p
    for p in [
        ROOT / "README.md",
        ROOT / "AGENTS.md",
        ROOT / "UPSTREAM.md",
        *docs.glob("*.md"),
    ]:
        for target in re.findall(r"\]\(([^)]+)\)", p.read_text()):
            target = target.strip("<>").split("#", 1)[0]
            if not target or "://" in target or target.startswith("mailto:"):
                continue
            assert (p.parent / target).exists(), f"{p}: broken link {target}"
    for path in [
        "dist/test",
        ".build/test",
        ".cache/test",
        ".ruff_cache/test",
        "__pycache__/test.pyc",
    ]:
        subprocess.run(
            ["git", "check-ignore", "--quiet", "--no-index", path], cwd=ROOT, check=True
        )
    print(
        f"PASS: {len(inputs)} build inputs, valid console header link, "
        f"{len(DOCS)} current documents, {len(REPORTS)} stable reports"
    )


if __name__ == "__main__":
    main()
