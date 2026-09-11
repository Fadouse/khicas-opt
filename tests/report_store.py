"""Read published current evidence; keep in-progress output outside docs."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def suite(name):
    return json.loads((ROOT / "docs/bench/regression.json").read_text())["suites"][name]


def work_report(name):
    directory = ROOT / ".build/reports"
    directory.mkdir(parents=True, exist_ok=True)
    return directory / (name + ".json")
