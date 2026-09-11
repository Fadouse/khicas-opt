"""Share layout resolution with build tools when a test runs as a script."""

import importlib.util
from pathlib import Path

_path = Path(__file__).resolve().parents[1] / "tools/repository.py"
_spec = importlib.util.spec_from_file_location("khicas_repository", _path)
_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_module)
ROOT = _module.ROOT
source_path = _module.source_path
build_inputs = _module.build_inputs
