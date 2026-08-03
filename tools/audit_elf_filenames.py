#!/usr/bin/env python3
"""Audit repository source filenames against filename evidence in an ELF.

DWARF compile-unit names are preferred.  With --evidence auto (the default),
ELF STT_FILE symbols are used when the ELF has no usable DWARF.  This fallback
is important for stripped debug information: a FILE symbol is still direct ELF
evidence for a translation-unit basename, but not for its directory.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path, PurePosixPath


SOURCE_SUFFIXES = {".c", ".cc", ".cp", ".cpp", ".cxx", ".s", ".asm"}
CU_RE = re.compile(r"\bDW_TAG_compile_unit\b")
NAME_RE = re.compile(r'DW_AT_name\s+\("([^"\\]*(?:\\.[^"\\]*)*)"\)')
FILE_SYMBOL_RE = re.compile(r"^\s*\d+:\s+\S+\s+\d+\s+FILE\s+\S+\s+\S+\s+\S+\s+(.+?)\s*$")


@dataclass(frozen=True)
class Evidence:
    path: str
    kind: str

    @property
    def name(self) -> str:
        return PurePosixPath(self.path.replace("\\", "/")).name


@dataclass(frozen=True)
class Result:
    source: str
    status: str
    evidence: str
    evidence_kind: str


def run(tool: str, *args: str) -> str:
    executable = shutil.which(tool)
    if not executable:
        raise RuntimeError(f"required tool not found in PATH: {tool}")
    proc = subprocess.run(
        [executable, *args], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=False,
    )
    if proc.returncode:
        detail = proc.stderr.strip() or proc.stdout.strip()
        raise RuntimeError(f"{tool} failed ({proc.returncode}): {detail}")
    return proc.stdout


def dwarf_evidence(elf: Path) -> list[Evidence]:
    """Return the DW_AT_name of each DW_TAG_compile_unit."""
    output = run("llvm-dwarfdump", "--debug-info", str(elf))
    found: list[Evidence] = []
    waiting_for_name = False
    for line in output.splitlines():
        if CU_RE.search(line):
            waiting_for_name = True
            continue
        if waiting_for_name:
            match = NAME_RE.search(line)
            if match:
                # DWARF strings can contain escaped quotes, but compiler paths
                # ordinarily only need these two common unescapes.
                path = match.group(1).replace(r"\\", "\\").replace(r'\"', '"')
                found.append(Evidence(path=path, kind="dwarf-cu"))
                waiting_for_name = False
            elif "DW_TAG_" in line:
                waiting_for_name = False
    return unique_evidence(found)


def symtab_evidence(elf: Path) -> list[Evidence]:
    output = run("readelf", "--syms", "--wide", str(elf))
    found = []
    for line in output.splitlines():
        match = FILE_SYMBOL_RE.match(line)
        if match and match.group(1):
            found.append(Evidence(path=match.group(1), kind="elf-file-symbol"))
    return unique_evidence(found)


def unique_evidence(items: list[Evidence]) -> list[Evidence]:
    return list(dict.fromkeys(items))


def collect_sources(root: Path, source_dirs: list[str]) -> list[Path]:
    sources: list[Path] = []
    for directory in source_dirs:
        base = (root / directory).resolve()
        if not base.is_dir():
            raise RuntimeError(f"source directory does not exist: {directory}")
        sources.extend(
            path for path in base.rglob("*")
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
        )
    return sorted(set(sources))


def audit(root: Path, sources: list[Path], evidence: list[Evidence]) -> list[Result]:
    by_name: dict[str, list[Evidence]] = {}
    by_stem: dict[str, list[Evidence]] = {}
    for item in evidence:
        by_name.setdefault(item.name, []).append(item)
        by_stem.setdefault(PurePosixPath(item.name).stem, []).append(item)

    results: list[Result] = []
    for source in sources:
        relative = source.relative_to(root).as_posix()
        exact_path = [
            item for item in evidence
            if item.path.replace("\\", "/").lstrip("./") == relative
            or item.path.replace("\\", "/").endswith("/" + relative)
        ]
        same_name = by_name.get(source.name, [])
        same_stem = by_stem.get(source.stem, [])
        if exact_path:
            status, matches = "exact-path", exact_path
        elif same_name:
            status, matches = "basename", same_name
        elif same_stem:
            status, matches = "extension-mismatch", same_stem
        else:
            status, matches = "missing", []
        results.append(Result(
            source=relative,
            status=status,
            evidence=";".join(item.path for item in matches),
            evidence_kind=";".join(sorted({item.kind for item in matches})),
        ))
    return results


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path, help="ELF containing filename evidence")
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root (default: cwd)")
    parser.add_argument("--source-dir", action="append", default=None,
                        help="directory to audit, relative to root (repeatable; default: src)")
    parser.add_argument("--evidence", choices=("auto", "dwarf", "symtab"), default="auto",
                        help="evidence source (auto falls back to STT_FILE symbols)")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument("--strict", action="store_true",
                        help="exit 1 if a file is missing or has an extension mismatch")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    elf = args.elf.resolve()
    if not elf.is_file():
        print(f"error: ELF does not exist: {elf}", file=sys.stderr)
        return 2
    try:
        evidence = dwarf_evidence(elf) if args.evidence != "symtab" else []
        selected = "dwarf"
        if args.evidence == "symtab" or (args.evidence == "auto" and not evidence):
            evidence = symtab_evidence(elf)
            selected = "symtab"
        sources = collect_sources(root, args.source_dir or ["src"])
        results = audit(root, sources, evidence)
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    counts = {status: 0 for status in ("exact-path", "basename", "extension-mismatch", "missing")}
    for result in results:
        counts[result.status] += 1
    if args.json:
        json.dump({
            "elf": str(elf), "evidence_source": selected,
            "evidence_entries": len(evidence), "counts": counts,
            "files": [asdict(result) for result in results],
        }, sys.stdout, indent=2)
        print()
    else:
        print("status\tsource\tevidence-kind\tevidence")
        for result in results:
            print(f"{result.status}\t{result.source}\t{result.evidence_kind}\t{result.evidence}")
        summary = " ".join(f"{key}={value}" for key, value in counts.items())
        print(f"summary: source={selected} evidence={len(evidence)} files={len(results)} {summary}", file=sys.stderr)

    bad = counts["extension-mismatch"] + counts["missing"]
    return 1 if args.strict and bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
