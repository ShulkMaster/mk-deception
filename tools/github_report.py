#!/usr/bin/env python3
"""Format objdiff function changes as a GitHub step-summary table."""

from argparse import ArgumentParser
import json
from pathlib import Path
from typing import Any


def escape_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ")


def percent(value: float) -> str:
    return f"{value:.2f}%"


def function_rows(changes: dict[str, Any]) -> list[tuple[str, str, float, float]]:
    rows = []
    for unit in changes.get("units", []):
        module = unit["name"]
        for function in unit.get("functions", []):
            before = float(function.get("from", {}).get("fuzzy_match_percent", 0.0))
            after = float(function.get("to", {}).get("fuzzy_match_percent", 0.0))
            if before != after:
                rows.append((module, function["name"], before, after))
    return sorted(rows, key=lambda row: (row[0], row[1]))


def format_report(changes: dict[str, Any]) -> str:
    rows = function_rows(changes)
    lines = ["## Objdiff function report", ""]
    if not rows:
        lines.append("No function match percentages changed compared with `main`.")
        return "\n".join(lines) + "\n"

    lines.extend(
        [
            "| Module | Function name | Before % | After % | Diff % | Result |",
            "| --- | --- | ---: | ---: | ---: | --- |",
        ]
    )
    for module, function, before, after in rows:
        delta = after - before
        if after == 100.0 and before < 100.0:
            result = "full match"
        elif delta > 0.0:
            result = "improvement"
        else:
            result = "regression"
        lines.append(
            f"| `{escape_cell(module)}` | `{escape_cell(function)}` | "
            f"{percent(before)} | {percent(after)} | {delta:+.2f}% | {result} |"
        )
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("changes", type=Path, help="objdiff report changes JSON")
    parser.add_argument("-o", "--output", type=Path, help="output Markdown path")
    args = parser.parse_args()

    with args.changes.open(encoding="utf-8") as source:
        output = format_report(json.load(source))

    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
