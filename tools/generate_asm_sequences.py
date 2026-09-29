#!/usr/bin/env python3
"""Generate allowlisted CodeWarrior assembly macros from DTK retail assembly.

The committed manifest describes which exceptional functions may use this
path, but contains no instruction payload.  Every emitted opword is extracted
from the selected version's retail-derived DTK assembly under build/.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path


FUNCTION_RE = re.compile(r"^\.fn\s+([A-Za-z_][A-Za-z0-9_]*)\s*,")
END_FUNCTION_RE = re.compile(r"^\.endfn\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
LABEL_RE = re.compile(r"^\.sym\s+([A-Za-z_][A-Za-z0-9_]*)\s*,")
INSTRUCTION_RE = re.compile(
    r"^/\*\s*([0-9A-Fa-f]{8})\s+[0-9A-Fa-f]{8}\s+"
    r"((?:[0-9A-Fa-f]{2}\s+){3}[0-9A-Fa-f]{2})\s*\*/\s*(.+?)\s*$"
)
SYMBOL_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
EXTERNAL_BRANCH_RE = re.compile(r"^(?:b|bl)\s+[A-Za-z_][A-Za-z0-9_]*$")
ASSEMBLY_SYMBOL = r'(?:[A-Za-z_][A-Za-z0-9_]*|"[^"]+")'
SYMBOL_RELOCATION_RE = re.compile(rf"^.*{ASSEMBLY_SYMBOL}@(h|ha|l)\b.*$")
SDA21_BASE_RE = re.compile(
    rf"(?P<symbol>{ASSEMBLY_SYMBOL})@sda21\((?P<base>r(?:0|13))\)"
)
SDA21_IMMEDIATE_RE = re.compile(rf"(?P<symbol>{ASSEMBLY_SYMBOL})@sda21\b")
SDA21_LI_RE = re.compile(
    rf"^li\s+(?P<dest>r[0-9]+),\s*(?P<symbol>{ASSEMBLY_SYMBOL})@sda21$"
)


@dataclass(frozen=True)
class Sequence:
    name: str
    address: int
    instructions: tuple[tuple[int, str], ...]
    labels: tuple[tuple[str, int], ...] = ()  # retail `.sym` labels: (name, byte offset)


def parse_int(value: object, field: str) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise ValueError(f"{field}: expected an integer or integer string")


def read_functions(path: Path) -> dict[str, Sequence]:
    functions: dict[str, Sequence] = {}
    active_name: str | None = None
    active_address: int | None = None
    active_instructions: list[tuple[int, str]] = []
    active_labels: list[tuple[str, int]] = []

    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        start = FUNCTION_RE.match(line)
        if start:
            if active_name is not None:
                raise ValueError(f"{path}:{line_number}: nested .fn")
            active_name = start.group(1)
            active_address = None
            active_instructions = []
            active_labels = []
            continue

        label = LABEL_RE.match(line)
        if label and active_name is not None:
            active_labels.append((label.group(1), 4 * len(active_instructions)))
            continue

        end = END_FUNCTION_RE.match(line)
        if end:
            if active_name is None or end.group(1) != active_name:
                raise ValueError(f"{path}:{line_number}: unmatched .endfn")
            if active_address is None or not active_instructions:
                raise ValueError(f"{path}:{line_number}: {active_name} has no instructions")
            if active_name in functions:
                raise ValueError(f"{path}:{line_number}: duplicate function {active_name}")
            functions[active_name] = Sequence(
                active_name, active_address, tuple(active_instructions), tuple(active_labels)
            )
            active_name = None
            continue

        instruction = INSTRUCTION_RE.match(line)
        if instruction and active_name is not None:
            address = int(instruction.group(1), 16)
            if active_address is None:
                active_address = address
            expected = active_address + 4 * len(active_instructions)
            if address != expected:
                raise ValueError(
                    f"{path}:{line_number}: {active_name} address 0x{address:X}, "
                    f"expected 0x{expected:X}"
                )
            active_instructions.append(
                (int(instruction.group(2).replace(" ", ""), 16), instruction.group(3))
            )

    if active_name is not None:
        raise ValueError(f"{path}: unterminated function {active_name}")
    return functions


def load_manifest(path: Path, version: str) -> tuple[Path, Path, list[dict[str, object]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("version") != version:
        raise ValueError(
            f"{path}: manifest version {data.get('version')!r} does not match {version!r}"
        )
    assembly = data.get("assembly")
    output = data.get("output")
    functions = data.get("functions")
    if not isinstance(assembly, str) or not isinstance(output, str):
        raise ValueError(f"{path}: assembly and output must be paths")
    if not isinstance(functions, list) or not functions:
        raise ValueError(f"{path}: functions must be a non-empty list")
    return Path(assembly), Path(output), functions


def emit_macro(
    sequence: Sequence,
    sda_symbols: dict[str, str],
    entries: tuple[str, ...] = (),
    end_entries: tuple[str, ...] = (),
) -> list[str]:
    lines = [f"#define SEQ_{sequence.name}() \\", "    nofralloc; \\"]
    # Exported entry labels inside the sequence, at their retail offsets.
    entry_at: dict[int, list[str]] = {}
    for label, offset in sequence.labels:
        if label in entries:
            entry_at.setdefault(offset, []).append(label)
    for index, (word, assembly) in enumerate(sequence.instructions):
        for label in entry_at.get(4 * index, []):
            lines.append(f"    entry {label}; \\")
        last = index + 1 == len(sequence.instructions) and not end_entries
        suffix = "" if last else " \\"
        if "@sda21" in assembly:
            for retail_symbol, source_symbol in sda_symbols.items():
                assembly = assembly.replace(
                    f'"{retail_symbol}"@sda21', f"{source_symbol}@sda21"
                )
            address_load = SDA21_LI_RE.fullmatch(assembly)
            if address_load:
                assembly = (
                    f"la {address_load.group('dest')}, "
                    f"{address_load.group('symbol')}(r13)"
                )
            else:
                assembly = SDA21_BASE_RE.sub(r"\g<symbol>(\g<base>)", assembly)
                assembly = SDA21_IMMEDIATE_RE.sub(r"\g<symbol>", assembly)
            if "@sda21" in assembly:
                raise ValueError(f"{sequence.name}: unsupported SDA21 syntax: {assembly}")
            lines.append(f"    {assembly};{suffix}")
        elif EXTERNAL_BRANCH_RE.fullmatch(assembly) or SYMBOL_RELOCATION_RE.fullmatch(
            assembly
        ):
            lines.append(f"    {assembly};{suffix}")
        else:
            lines.append(f"    opword 0x{word:08X};{suffix}")
    # Labels that mark the address just past the last instruction.
    for index, label in enumerate(end_entries):
        suffix = " \\" if index + 1 < len(end_entries) else ""
        lines.append(f"    entry {label};{suffix}")
    return lines


def generate(manifest_path: Path, version: str, build_root: Path) -> tuple[Path, str]:
    assembly_relative, output_relative, entries = load_manifest(manifest_path, version)
    assembly_cache: dict[Path, dict[str, Sequence]] = {}

    def functions_for(relative: Path) -> dict[str, Sequence]:
        assembly_path = build_root / version / "asm" / relative
        if not assembly_path.is_file():
            raise FileNotFoundError(f"required retail assembly not found: {assembly_path}")
        if relative not in assembly_cache:
            assembly_cache[relative] = read_functions(assembly_path)
        return assembly_cache[relative]

    lines = [
        "/* Generated from version-specific retail assembly. Do not edit. */",
        f"/* Version: {version}; input: {assembly_relative.as_posix()} */",
        "",
    ]
    seen: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise ValueError(f"{manifest_path}: each function entry must be an object")
        name = entry.get("name")
        if not isinstance(name, str) or not SYMBOL_RE.fullmatch(name):
            raise ValueError(f"{manifest_path}: invalid function name {name!r}")
        if name in seen:
            raise ValueError(f"{manifest_path}: duplicate allowlist entry {name}")
        seen.add(name)
        entry_assembly = entry.get("assembly", assembly_relative.as_posix())
        if not isinstance(entry_assembly, str):
            raise ValueError(f"{name}.assembly: expected a path string")
        function_assembly = Path(entry_assembly)
        available = functions_for(function_assembly)
        try:
            sequence = available[name]
        except KeyError as exc:
            raise ValueError(
                f"{function_assembly}: allowlisted function {name} not found"
            ) from exc
        address = parse_int(entry.get("address"), f"{name}.address")
        size = parse_int(entry.get("size"), f"{name}.size")
        raw_sda_symbols = entry.get("sda_symbols", {})
        if not isinstance(raw_sda_symbols, dict):
            raise ValueError(f"{name}.sda_symbols: expected an object")
        sda_symbols: dict[str, str] = {}
        for retail_symbol, source_symbol in raw_sda_symbols.items():
            if not isinstance(retail_symbol, str) or not isinstance(source_symbol, str):
                raise ValueError(f"{name}.sda_symbols: expected string mappings")
            if not SYMBOL_RE.fullmatch(source_symbol):
                raise ValueError(
                    f"{name}.sda_symbols: invalid source symbol {source_symbol!r}"
                )
            sda_symbols[retail_symbol] = source_symbol
        if sequence.address != address:
            raise ValueError(
                f"{name}: retail address 0x{sequence.address:X}, expected 0x{address:X}"
            )
        if len(sequence.instructions) * 4 != size:
            raise ValueError(
                f"{name}: retail size 0x{len(sequence.instructions) * 4:X}, expected 0x{size:X}"
            )
        raw_entries = entry.get("entries", [])
        if not isinstance(raw_entries, list) or not all(isinstance(e, str) for e in raw_entries):
            raise ValueError(f"{name}.entries: expected a list of label names")
        retail_labels = {label for label, _ in sequence.labels}
        for label in raw_entries:
            if label not in retail_labels:
                raise ValueError(f"{name}.entries: {label} is not a retail label inside {name}")
            if not SYMBOL_RE.fullmatch(label):
                raise ValueError(f"{name}.entries: invalid label {label!r}")
        raw_end_entries = entry.get("end_entries", [])
        if not isinstance(raw_end_entries, list) or not all(
            isinstance(e, str) for e in raw_end_entries
        ):
            raise ValueError(f"{name}.end_entries: expected a list of label names")
        end_address = sequence.address + size
        for label in raw_end_entries:
            if not SYMBOL_RE.fullmatch(label):
                raise ValueError(f"{name}.end_entries: invalid label {label!r}")
            # A label retail does name must sit exactly at the function's end.
            for other in available.values():
                for other_label, offset in other.labels:
                    if other_label == label and other.address + offset != end_address:
                        raise ValueError(
                            f"{name}.end_entries: retail {label} is at "
                            f"0x{other.address + offset:X}, not 0x{end_address:X}"
                        )
        lines.extend(
            emit_macro(sequence, sda_symbols, tuple(raw_entries), tuple(raw_end_entries))
        )
        lines.append("")

    return build_root / version / "include" / output_relative, "\n".join(lines)


def atomic_write(path: Path, contents: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="ascii", newline="\n") as output:
            output.write(contents)
        os.replace(temporary, path)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--build-root", type=Path, default=Path("build"))
    args = parser.parse_args()
    destination, contents = generate(args.manifest, args.version, args.build_root)
    atomic_write(destination, contents)
    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
