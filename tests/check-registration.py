#!/usr/bin/env python3
"""Protect sorted target lookup tables and existing command aliases."""

from repository import source_path
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = "upstream/khicas-2026-07-31"
COMMANDS = {
    "cart2param",
    "param2cart",
    "cart2polar",
    "polar2cart",
    "polar2param",
    "param2polar",
}
ADDITIONS = COMMANDS | {"Li2", "EllipticF"}


def table(names, pointers, release):
    names, pointers = names.splitlines(), pointers.splitlines()
    assert len(names) == len(pointers), "Lookup tables have different lengths"
    enabled = True
    rows = []
    for name, pointer in zip(names, pointers):
        if name.startswith("#"):
            assert name == pointer, "Preprocessor conditions differ"
            assert name in ("#ifdef RELEASE", "#endif"), name
            enabled = release if name == "#ifdef RELEASE" else True
        elif enabled and name.startswith("{"):
            match = re.fullmatch(r'\{"([^"]+)"(.*)', name)
            assert match and re.fullmatch(r"at_\w+,", pointer), (name, pointer)
            rows.append((match[1], match[2], pointer))
    return rows


current = [(source_path(f)).read_text() for f in ("static_lexer.h", "static_lexer_.h")]
original = [
    subprocess.check_output(["git", "show", f"{UPSTREAM}:{f}"], cwd=ROOT, text=True)
    for f in ("static_lexer.h", "static_lexer_.h")
]
for release in (False, True):
    rows = table(*current, release)
    keys = [row[0] for row in rows]
    assert keys == sorted(keys), "Binary-search command names must be sorted"
    assert len(set(keys)) == len(keys), "Duplicate command name"
    assert sorted(row for row in rows if row[0] not in ADDITIONS) == sorted(
        table(*original, release)
    ), "Existing command metadata/aliases changed"
    for command in ADDITIONS:
        assert [row[2] for row in rows if row[0] == command] == [f"at_{command},"]
    print(f"PASS: {len(rows)} command mappings, RELEASE={release}")
for command in COMMANDS:
    assert f"* const at_{command};" in (source_path("static_extern.h")).read_text()
    for catalog in ("catalogen.cpp", "catalogfr.cpp"):
        assert (source_path(catalog)).read_text().count(f'"{command}(') == 1
print("PASS: all six conversions declared and present in both catalogs")
