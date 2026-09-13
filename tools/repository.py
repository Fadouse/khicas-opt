"""Resolve the maintained source layout and flatten inputs for upstream make."""

from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


@lru_cache(maxsize=1)
def build_inputs():
    inputs = {}
    for directory in ("src", "assets", "vendor", "build"):
        for path in sorted((ROOT / directory).rglob("*")):
            if "__pycache__" in path.parts or path.suffix in (".pyc", ".pyo"):
                continue
            if path.is_symlink() and not path.exists():
                raise ValueError(f"Broken build input symlink: {path}")
            if not path.is_file():
                continue
            name = "Makefile" if path.name == "Makefile.native" else path.name
            if name in inputs:
                raise ValueError(f"Duplicate build input: {name}")
            inputs[name] = path
    return inputs


def source_path(name):
    """Return a source/build input by basename, or an explicit repository path."""
    name = str(name)
    if name in build_inputs():
        return build_inputs()[name]
    return ROOT / name
