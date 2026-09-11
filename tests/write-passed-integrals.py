#!/usr/bin/env python3
"""Render the current verified integral inventory from stable report files."""

import argparse
import collections
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LABELS = {
    "exact": "精确验证",
    "sampled": "导数采样",
    "numeric_constant": "数值常量核对",
}


def code(value):
    return (
        "`"
        + str(value).replace("|", "&#124;").replace("`", "&#96;").replace("\n", " ")
        + "`"
    )


def generate(bench):
    manifest = json.loads((ROOT / "tests/corpora.json").read_text())["corpora"]
    suites = json.loads((bench / "regression.json").read_text())["suites"]
    output = []
    counts = collections.Counter()
    inputs = set()
    for item in manifest:
        cp = ROOT / item["file"]
        data = json.loads(cp.read_text())
        cases = data if isinstance(data, list) else data["cases"]
        report = suites[item["name"]]
        if "corpus_sha256" in report:
            expected_hash = report.get("fixture_layout", {}).get(
                "corpus_sha256", report["corpus_sha256"]
            )
            assert hashlib.sha256(cp.read_bytes()).hexdigest() == expected_hash, item[
                "name"
            ]
        kind = item["result_schema"]
        if kind == "user":
            rows = {str(row["id"]): row for row in report["cases"]}
        elif kind == "corpus":
            rows = {str(row["id"]): row for row in report["runs"]["current"]}
        else:
            rows = {}
            for row in report["runs"]:
                rows.setdefault(str(row["id"]), []).append(row)
        assert set(rows) == {str(c["id"]) for c in cases}
        for case in cases:
            measured = rows[str(case["id"])]
            if kind == "user":
                assert all(row["status"] == "exact" for row in measured["runs"])
                assert measured["stack_64KiB"]["exit"] == 0
                assert (
                    measured["stack_64KiB"]["result"] == measured["runs"][0]["result"]
                )
                status = "exact"
            elif kind == "stack":
                assert len(measured) == 4 and all(
                    row["status"] == "exact" for row in measured
                )
                status = "exact"
            else:
                status = measured["status"]
                assert status in LABELS
            expression = (
                case.get("input")
                or "integrate("
                + case["f"]
                + ",x"
                + ("" if "bounds" not in case else "," + ",".join(case["bounds"]))
                + ")"
            )
            reference = case.get("reference", case.get("expected"))
            assert reference is not None
            counts[status] += 1
            inputs.add("".join(expression.split()))
            output.append(
                "| "
                + " | ".join(map(code, [case["id"], expression, reference]))
                + " | "
                + LABELS[status]
                + " |"
            )
    return "\n".join(
        [
            "# 通过测试",
            "",
            f"积分记录 **{sum(counts.values())} 条**：{counts['exact']} 条精确验证、"
            f"{counts['sampled']} 条导数采样、{counts['numeric_constant']} 条数值常量核对。",
            f"去除输入空白后共 {len(inputs)} 种输入；这是文本去重，不是数学去重。",
            "",
            "本表来自当前保留的验证结果。实际源码、探针哈希与运行模式见 "
            "[回归数据](bench/regression.json)。主机结果不代表 CG50 实机耗时或全部实机通过。",
            "不定积分省略积分常数，结果须遵守原定义域和分支。采样不代替符号证明；未求出的积分不计入。",
            "",
            "[题库目录](../tests/corpora.json) · [当前新题状态](bench/acceptance.json) · "
            "[验收方法](tests-set.md)",
            "",
            "| ID | 输入 | 参考结果 | 验证 |",
            "| --- | --- | --- | --- |",
            *output,
            "",
        ]
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bench", type=Path, default=ROOT / "docs/bench")
    parser.add_argument(
        "--output", type=Path, default=ROOT / ".build/reports/passed-tests.md"
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = generate(args.bench)
    if args.check:
        assert (ROOT / "docs/passed-tests.md").read_text() == expected
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(expected)
    print("PASS: verified integral inventory is consistent")


if __name__ == "__main__":
    main()
