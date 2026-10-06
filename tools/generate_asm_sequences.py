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
EXTERNAL_BRANCH_RE = re.compile(
    r"^b[a-z]*[+-]?\s+(?:cr[0-7],\s*)?[A-Za-z_][A-Za-z0-9_]*$"
)
ASSEMBLY_SYMBOL = r'(?:[A-Za-z_][A-Za-z0-9_]*|"[^"]+")'
SYMBOL_RELOCATION_RE = re.compile(rf"^.*{ASSEMBLY_SYMBOL}@(h|ha|l)\b.*$")
SDA21_BASE_RE = re.compile(
    rf"(?P<symbol>{ASSEMBLY_SYMBOL})@sda21\((?P<base>r(?:0|13))\)"
)
SDA21_IMMEDIATE_RE = re.compile(rf"(?P<symbol>{ASSEMBLY_SYMBOL})@sda21\b")
SDA21_LI_RE = re.compile(
    rf"^li\s+(?P<dest>r[0-9]+),\s*(?P<symbol>{ASSEMBLY_SYMBOL})@sda21$"
)
LOCAL_BRANCH_RE = re.compile(
    r"^b[a-z]*[+-]?\s+(?:cr[0-7],\s*)?\.L_(?P<target>[0-9A-Fa-f]{8})$"
)
REGISTER_RE = re.compile(r"^(?:r|f)(?:[0-9]|[12][0-9]|3[01])$")
REGISTER_TOKEN_RE = re.compile(r"(?<![A-Za-z0-9_.])(?:r|f)(?:[12][0-9]|3[01]|[0-9])(?![A-Za-z0-9_])")
GQR_TOKEN_RE = re.compile(r"(?<![A-Za-z0-9_])qr([0-7])(?![A-Za-z0-9_])")
# Source spelling for a renamed relocation target: a C name, optionally + offset.
SOURCE_SYMBOL_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(?:\s*\+\s*(?:0x[0-9A-Fa-f]+|[0-9]+))?$")
LOCAL_TARGET_RE = re.compile(r"\.L_([0-9A-Fa-f]{8})\b")
STACK_OPERAND_RE = re.compile(r"(?<![A-Za-z0-9_])(-?0x[0-9A-Fa-f]+|-?[0-9]+)\(r1\)")


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
    block: bool = False,
    operands: dict[str, str] | None = None,
    symbols: dict[str, str] | None = None,
    text: bool = False,
    stack: tuple[str, int, int] | None = None,
) -> list[str]:
    """Emit SEQ_<name>.  A whole function is a `nofralloc` asm function body.
    A block is a run of instructions inside a C function, invoked from an
    inline `asm { SEQ_<name>(...) }` statement; with `operands` (macro
    parameter -> retail register) every instruction is emitted as text with
    those registers replaced by the C operands passed to the macro.
    `symbols` renames retail relocation targets (@h/@ha/@l) to source
    spellings; a `text` block emits every instruction as text, with local
    branch targets as labels, so the compiler sees its registers."""
    parameters = list(operands) if operands else []
    register_to_parameter = {register: name for name, register in (operands or {}).items()}
    lines = [f"#define SEQ_{sequence.name}({', '.join(parameters)}) \\"]
    if not block:
        lines.append("    nofralloc; \\")
    # Exported entry labels inside the sequence, at their retail offsets.
    entry_at: dict[int, list[str]] = {}
    for label, offset in sequence.labels:
        if label in entries:
            entry_at.setdefault(offset, []).append(label)
    label_at: set[int] = set()
    if text:
        for _, assembly in sequence.instructions:
            for target in LOCAL_TARGET_RE.findall(assembly):
                label_at.add(int(target, 16))
    for index, (word, assembly) in enumerate(sequence.instructions):
        for label in entry_at.get(4 * index, []):
            lines.append(f"    entry {label}; \\")
        if sequence.address + 4 * index in label_at:
            lines.append(f"    L_{sequence.address + 4 * index:08X}: \\")
        last = index + 1 == len(sequence.instructions) and not end_entries and not (
            sequence.address + 4 * len(sequence.instructions) in label_at
        )
        suffix = "" if last else " \\"
        assembly = rename_symbols(assembly, symbols or {})
        if text:
            assembly = LOCAL_TARGET_RE.sub(r"L_\1", assembly)
            if stack:
                assembly = name_stack_slots(assembly, stack)
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
            if register_to_parameter:
                assembly = substitute_operands(assembly, register_to_parameter)
            lines.append(f"    {assembly};{suffix}")
        elif register_to_parameter or text:
            lines.append(f"    {substitute_operands(assembly, register_to_parameter)};{suffix}")
        elif EXTERNAL_BRANCH_RE.fullmatch(assembly) or SYMBOL_RELOCATION_RE.fullmatch(
            assembly
        ):
            lines.append(f"    {assembly};{suffix}")
        else:
            lines.append(f"    opword 0x{word:08X};{suffix}")
    end_address = sequence.address + 4 * len(sequence.instructions)
    if end_address in label_at:
        suffix = " \\" if end_entries else ""
        lines.append(f"    L_{end_address:08X}: ;{suffix}")
    # Labels that mark the address just past the last instruction.
    for index, label in enumerate(end_entries):
        suffix = " \\" if index + 1 < len(end_entries) else ""
        lines.append(f"    entry {label};{suffix}")
    return lines


def name_stack_slots(assembly: str, stack: tuple[str, int, int]) -> str:
    """Spell r1-relative word accesses inside a C local array as `name[i]`, the
    way CodeWarrior's inline assembler addresses locals (keeps the array, and
    so the retail frame layout, alive)."""
    name, base, size = stack

    def slot(match: re.Match[str]) -> str:
        offset = int(match.group(1), 0)
        if base <= offset < base + size and (offset - base) % 4 == 0:
            return f"{name}[{(offset - base) // 4}]"
        return match.group(0)

    return STACK_OPERAND_RE.sub(slot, assembly)


def rename_symbols(assembly: str, symbols: dict[str, str]) -> str:
    """Replace retail relocation targets (quoted or bare, before @h/@ha/@l)
    with their source spellings."""
    for retail_symbol, source_symbol in symbols.items():
        for spelled in (f'"{retail_symbol}"', retail_symbol):
            assembly = re.sub(
                re.escape(spelled) + r"(?=@(?:h|ha|l)\b)", lambda _m: source_symbol, assembly
            )
    return assembly


def substitute_operands(assembly: str, register_to_parameter: dict[str, str]) -> str:
    """Replace retail registers with C operand names; spell GQR operands as
    the plain numbers CodeWarrior's inline assembler expects."""
    assembly = REGISTER_TOKEN_RE.sub(
        lambda match: register_to_parameter.get(match.group(0), match.group(0)), assembly
    )
    return GQR_TOKEN_RE.sub(r"\1", assembly)


def block_sequence(
    name: str,
    parent: Sequence,
    address: int,
    size: int,
    operands: dict[str, str] | None,
    text: bool = False,
) -> Sequence:
    """Slice an inline-assembly block [address, address + size) out of a retail
    function, refusing anything a C function's asm statement cannot hold."""
    end = address + size
    parent_end = parent.address + 4 * len(parent.instructions)
    if address % 4 or size % 4 or size <= 0:
        raise ValueError(f"{name}: block address and size must be positive multiples of 4")
    if not parent.address <= address < end <= parent_end:
        raise ValueError(
            f"{name}: block 0x{address:X}..0x{end:X} is outside {parent.name} "
            f"(0x{parent.address:X}..0x{parent_end:X})"
        )
    first = (address - parent.address) // 4
    instructions = parent.instructions[first : first + size // 4]
    for index, (_, assembly) in enumerate(instructions):
        at = address + 4 * index
        local = LOCAL_BRANCH_RE.fullmatch(assembly)
        if local:
            target = int(local.group("target"), 16)
            if not address <= target <= end:
                raise ValueError(f"{name}: branch at 0x{at:X} leaves the block ({assembly})")
            if operands and not text:
                raise ValueError(
                    f"{name}: branch at 0x{at:X}: operand blocks are emitted as text and "
                    "may be rescheduled; use an opword block (no operands) for control flow"
                )
        elif EXTERNAL_BRANCH_RE.fullmatch(assembly) and not re.match(r"^bl\s", assembly):
            raise ValueError(f"{name}: branch at 0x{at:X} leaves the block ({assembly})")
    if operands:
        used = {
            token
            for _, assembly in instructions
            for token in REGISTER_TOKEN_RE.findall(assembly)
        }
        for parameter, register in operands.items():
            if not SYMBOL_RE.fullmatch(parameter):
                raise ValueError(f"{name}.operands: invalid parameter name {parameter!r}")
            if not isinstance(register, str) or not REGISTER_RE.fullmatch(register):
                raise ValueError(f"{name}.operands: invalid register {register!r}")
            if register not in used:
                raise ValueError(f"{name}.operands: {register} is not used by the block")
        if len(set(operands.values())) != len(operands):
            raise ValueError(f"{name}.operands: a register is mapped twice")
    return Sequence(name, address, tuple(instructions))


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
        raw_symbols = entry.get("symbols", {})
        if not isinstance(raw_symbols, dict):
            raise ValueError(f"{name}.symbols: expected an object")
        symbols: dict[str, str] = {}
        for retail_symbol, source_symbol in raw_symbols.items():
            if not isinstance(retail_symbol, str) or not isinstance(source_symbol, str):
                raise ValueError(f"{name}.symbols: expected string mappings")
            if not SOURCE_SYMBOL_RE.fullmatch(source_symbol):
                raise ValueError(f"{name}.symbols: invalid source spelling {source_symbol!r}")
            symbols[retail_symbol] = source_symbol
        text = entry.get("text", False)
        if not isinstance(text, bool):
            raise ValueError(f"{name}.text: expected true or false")
        raw_stack = entry.get("stack")
        stack: tuple[str, int, int] | None = None
        if raw_stack is not None:
            if not text or not isinstance(raw_stack, dict):
                raise ValueError(f'{name}.stack: needs "text": true and an object')
            stack_name = raw_stack.get("name")
            if not isinstance(stack_name, str) or not SYMBOL_RE.fullmatch(stack_name):
                raise ValueError(f"{name}.stack.name: invalid local name {stack_name!r}")
            stack = (
                stack_name,
                parse_int(raw_stack.get("offset"), f"{name}.stack.offset"),
                parse_int(raw_stack.get("size"), f"{name}.stack.size"),
            )
        parent_name = entry.get("function")
        raw_operands = entry.get("operands")
        if parent_name is not None:
            # Inline-assembly block inside a C function: SEQ_<name>(operands...)
            # is invoked from an `asm { }` statement, not as a function body.
            if not isinstance(parent_name, str) or not SYMBOL_RE.fullmatch(parent_name):
                raise ValueError(f"{name}.function: invalid function name {parent_name!r}")
            if name in available:
                raise ValueError(f"{name}: a block must not reuse a retail function name")
            try:
                parent = available[parent_name]
            except KeyError as exc:
                raise ValueError(
                    f"{function_assembly}: function {parent_name} for block {name} not found"
                ) from exc
            if "entries" in entry or "end_entries" in entry:
                raise ValueError(f"{name}: blocks cannot export entry labels")
            if raw_operands is not None and not isinstance(raw_operands, dict):
                raise ValueError(f"{name}.operands: expected an object")
            sequence = block_sequence(name, parent, address, size, raw_operands, text)
            lines.extend(
                emit_macro(
                    sequence,
                    sda_symbols,
                    block=True,
                    operands=raw_operands,
                    symbols=symbols,
                    text=text,
                    stack=stack,
                )
            )
            lines.append("")
            continue
        if raw_operands is not None:
            raise ValueError(f'{name}.operands: only blocks (with "function") take operands')
        if text:
            raise ValueError(f'{name}.text: only blocks (with "function") take text mode')
        try:
            sequence = available[name]
        except KeyError as exc:
            raise ValueError(
                f"{function_assembly}: allowlisted function {name} not found"
            ) from exc
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
            emit_macro(
                sequence,
                sda_symbols,
                tuple(raw_entries),
                tuple(raw_end_entries),
                symbols=symbols,
            )
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
