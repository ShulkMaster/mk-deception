#!/usr/bin/env python3
"""Format objdiff report changes as a Markdown pull request report.

The full report goes to --output. With --comments-dir, the same report is
also split into numbered comment bodies that fit GitHub's comment size limit;
every row is kept, and a table that spans comments repeats its header.
"""

from argparse import ArgumentParser
from dataclasses import dataclass
import json
from pathlib import Path
from typing import Any

# GitHub rejects comments longer than 65536 characters. Larger reports are
# split across several comments; leave room for the workflow's marker line.
MAX_COMMENT_CHARS = 60000


@dataclass
class FunctionChange:
    unit: str
    name: str
    size: int
    before: float
    after: float

    @property
    def delta(self) -> float:
        return self.after - self.before


def escape_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ")


def unit_name(name: str) -> str:
    return name.removeprefix("main/")


def percent(value: float) -> str:
    if value == 100.0:
        return "100%"
    return f"{value:.2f}%"


def mood(after: float) -> str:
    if after >= 99.0:
        return "🤏"
    if after >= 90.0:
        return "🔥"
    if after >= 70.0:
        return "💪"
    if after >= 40.0:
        return "🛠️"
    return "🌱"


def collect(changes: dict[str, Any]) -> tuple[list[FunctionChange], int]:
    rows = []
    unpaired = 0
    for unit in changes.get("units", []):
        for function in unit.get("functions", []):
            before = function.get("from")
            after = function.get("to")
            if before is None or after is None:
                # Added or removed symbols (renames, new splits) have no
                # meaningful before/after pair.
                unpaired += 1
                continue
            rows.append(
                FunctionChange(
                    unit=unit_name(unit["name"]),
                    name=function["name"],
                    size=int(after.get("size", before.get("size", 0))),
                    before=float(before.get("fuzzy_match_percent", 0.0)),
                    after=float(after.get("fuzzy_match_percent", 0.0)),
                )
            )
    return [row for row in rows if row.before != row.after], unpaired


def headline(matches: int, improvements: int, regressions: int) -> str:
    if regressions and not (matches or improvements):
        return "💀 **Fatality.** This PR only regresses functions."
    if regressions:
        return "⚠️ **Mixed kombat.** Progress with regressions to review."
    if matches:
        return "🏆 **Flawless victory!** New matches and no regressions."
    if improvements:
        return "🥋 **Round won.** Improvements and no regressions."
    return "😴 **No kontest.** No function match percentages changed."


def progress_table(changes: dict[str, Any]) -> list[str]:
    before = changes.get("from") or {}
    after = changes.get("to") or {}
    rows = [
        ("Code", "matched_code_percent", "matched_code", "total_code"),
        ("Data", "matched_data_percent", "matched_data", "total_data"),
        ("Functions", "matched_functions_percent", "matched_functions", "total_functions"),
        ("Fuzzy", "fuzzy_match_percent", None, None),
    ]
    lines = [
        "| Progress | Before | After | Change |",
        "| --- | ---: | ---: | ---: |",
    ]
    for label, key, count_key, total_key in rows:
        if key not in after:
            continue
        old = float(before.get(key, 0.0))
        new = float(after[key])
        new_text = f"{new:.4f}%"
        if count_key:
            new_text += f" ({int(after.get(count_key, 0)):,} / {int(after.get(total_key, 0)):,})"
        lines.append(f"| {label} | {old:.4f}% | {new_text} | {new - old:+.4f}% |")
    return lines


@dataclass
class Section:
    title: str
    header: list[str]
    rows: list[str]
    empty: str
    open_: bool


def build(changes: dict[str, Any], title: str) -> tuple[list[str], list[Section]]:
    rows, unpaired = collect(changes)
    matches = sorted(
        (row for row in rows if row.after == 100.0),
        key=lambda row: (-row.size, row.unit, row.name),
    )
    improvements = sorted(
        (row for row in rows if row.after < 100.0 and row.delta > 0.0),
        key=lambda row: (-row.delta, row.unit, row.name),
    )
    regressions = sorted(
        (row for row in rows if row.delta < 0.0),
        key=lambda row: (row.delta, row.unit, row.name),
    )
    matched_bytes = sum(row.size for row in matches)

    summary = [f"## {title}", "", headline(len(matches), len(improvements), len(regressions)), ""]
    summary.append(
        f"✅ **{len(matches)}** new match{'es' if len(matches) != 1 else ''}"
        f" (+{matched_bytes:,} bytes) · "
        f"📈 **{len(improvements)}** improvement{'s' if len(improvements) != 1 else ''} · "
        f"📉 **{len(regressions)}** regression{'s' if len(regressions) != 1 else ''}"
    )
    if unpaired:
        summary.extend(["", f"<sub>{unpaired} added or removed symbol(s) not compared.</sub>"])
    summary.append("")
    progress = progress_table(changes)
    if len(progress) > 2:
        summary.extend(progress)
        summary.append("")
    if not rows:
        return summary, []

    change_header = [
        "| | Unit | Function | Before | After | Change |",
        "| --- | --- | --- | ---: | ---: | ---: |",
    ]
    sections = [
        Section(
            f"✅ Matches ({len(matches)})",
            ["| | Unit | Function | Size | Before |", "| --- | --- | --- | ---: | ---: |"],
            [
                f"| ✅ | `{escape_cell(row.unit)}` | `{escape_cell(row.name)}` | "
                f"{row.size:,} | {percent(row.before)} |"
                for row in matches
            ],
            "No new matches.",
            open_=len(matches) <= 30,
        ),
        Section(
            f"📈 Improvements ({len(improvements)})",
            change_header,
            [
                f"| {mood(row.after)} | `{escape_cell(row.unit)}` | `{escape_cell(row.name)}` | "
                f"{percent(row.before)} | {percent(row.after)} | {row.delta:+.2f}% |"
                for row in improvements
            ],
            "No improvements.",
            open_=len(improvements) <= 30,
        ),
        Section(
            f"📉 Regressions ({len(regressions)})",
            change_header,
            [
                f"| {'💔' if row.before == 100.0 else '📉'} | `{escape_cell(row.unit)}` | "
                f"`{escape_cell(row.name)}` | {percent(row.before)} | {percent(row.after)} | "
                f"{row.delta:+.2f}% |"
                for row in regressions
            ],
            "No regressions. 🎉",
            open_=True,
        ),
    ]
    return summary, sections


FOOTER = "<sub>Compared against the merge base with `main`. 💔 = lost a full match.</sub>"


def section_open(section: Section, continued: bool) -> list[str]:
    title = f"{section.title} (continued)" if continued else section.title
    return [f"<details{' open' if section.open_ else ''}>", f"<summary><b>{title}</b></summary>", ""]


SECTION_CLOSE = ["", "</details>", ""]


def format_report(summary: list[str], sections: list[Section]) -> str:
    lines = list(summary)
    for section in sections:
        lines.extend(section_open(section, False))
        if section.rows:
            lines.extend(section.header)
            lines.extend(section.rows)
        else:
            lines.append(section.empty)
        lines.extend(SECTION_CLOSE)
    if sections:
        lines.append(FOOTER)
    return "\n".join(lines) + "\n"


def split_comments(summary: list[str], sections: list[Section], limit: int) -> list[str]:
    parts: list[list[str]] = []
    current = list(summary)

    def size(lines: list[str]) -> int:
        return sum(len(line) + 1 for line in lines)

    def flush() -> None:
        nonlocal current
        parts.append(current)
        current = []

    for section in sections:
        body = section.rows or [section.empty]
        header = section.header if section.rows else []
        continued = False
        index = 0
        while index < len(body):
            opening = section_open(section, continued) + header
            if current and size(current) + size(opening) + size(body[index : index + 1]) + size(
                SECTION_CLOSE
            ) > limit:
                flush()
            current.extend(opening)
            # Always place at least one row so an oversized row cannot stall.
            current.append(body[index])
            index += 1
            while index < len(body) and size(current) + len(body[index]) + 1 + size(
                SECTION_CLOSE
            ) <= limit:
                current.append(body[index])
                index += 1
            current.extend(SECTION_CLOSE)
            if index < len(body):
                flush()
                continued = True
    if sections:
        if size(current) + len(FOOTER) + 1 > limit:
            flush()
        current.append(FOOTER)
    flush()

    count = len(parts)
    comments = []
    for number, lines in enumerate(parts, start=1):
        text = "\n".join(lines) + "\n"
        if count > 1:
            text += f"\n<sub>Part {number} of {count}.</sub>\n"
        comments.append(text)
    return comments


def main() -> None:
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("changes", type=Path, help="objdiff report changes JSON")
    parser.add_argument("-o", "--output", type=Path, help="full Markdown report path")
    parser.add_argument(
        "--comments-dir",
        type=Path,
        help="directory for comment-N.md bodies sized for GitHub comments",
    )
    parser.add_argument("--title", default="Objdiff report", help="report heading")
    args = parser.parse_args()

    with args.changes.open(encoding="utf-8") as source:
        summary, sections = build(json.load(source), args.title)

    output = format_report(summary, sections)
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")

    if args.comments_dir:
        args.comments_dir.mkdir(parents=True, exist_ok=True)
        for stale in args.comments_dir.glob("comment-*.md"):
            stale.unlink()
        comments = split_comments(summary, sections, MAX_COMMENT_CHARS)
        for number, text in enumerate(comments, start=1):
            (args.comments_dir / f"comment-{number}.md").write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
