#!/usr/bin/env python3
"""Split the PS2 retail ELF into per-unit assembly for objdiff targets.

The PS2 release (SLUS_208.81) ships one merged PROGBITS section and no
symbols, so DTK cannot split it. This tool reads a DTK-style splits.txt and
symbols.txt, disassembles only the listed unit ranges with spimdisasm, and
writes one GNU assembler file per unit. Assembling those files yields
relocatable target objects whose relocations name the same symbols as the
compiled source.

Usage:
    python3 tools/ps2_split.py --elf orig/SLUS_208.81/SLUS_208.81 \\
        --sha1 7657eff03bbfd816c541693812ab6bea076f0cf9 \\
        --splits config/SLUS_208.81/splits.txt \\
        --symbols config/SLUS_208.81/symbols.txt \\
        --out build/SLUS_208.81/asm
"""

from __future__ import annotations

import argparse
import hashlib
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

try:
    import rabbitizer
    from spimdisasm import common, mips
except ImportError:
    sys.exit(
        "ps2_split.py requires spimdisasm. Install it with:\n"
        "    python3 -m pip install -r tools/requirements-ps2.txt"
    )


PT_LOAD = 1
SHT_MIPS_REGINFO = 0x70000006

SECTION_HEADER_RE = re.compile(r"^\s+(\.\w+)\s+type:(\w+)(?:\s+align:(\d+))?\s*$")
UNIT_RE = re.compile(r"^(\S.*):\s*$")
UNIT_SECTION_RE = re.compile(
    r"^\s+(\.\w+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)\s*$"
)
SYMBOL_RE = re.compile(
    r"^(\S+)\s*=\s*(\.\w+):(0x[0-9A-Fa-f]+);\s*(?://\s*(.*))?$"
)

# Section flags emitted for each output section kind.
SECTION_DIRECTIVES = {
    "code": '"ax", @progbits',
    "data": '"wa", @progbits',
    "rodata": '"a", @progbits',
    "bss": '"wa", @nobits',
}

MACROS = """\
.macro glabel label, visibility=global
    .\\visibility \\label
    .type \\label, @function
    \\label:
.endm

.macro dlabel label, visibility=global
    .\\visibility \\label
    .type \\label, @object
    \\label:
.endm

.macro jlabel label, visibility=local
    \\label:
.endm

.macro alabel label, visibility=global
    .\\visibility \\label
    \\label:
.endm

.macro endlabel label
    .size \\label, . - \\label
.endm

.macro enddlabel label
    .size \\label, . - \\label
.endm

.macro nonmatching label, size=1
.endm
"""


@dataclass
class Segment:
    vaddr: int
    offset: int
    filesz: int
    memsz: int


@dataclass
class Elf:
    data: bytes
    segments: List[Segment]
    gp: Optional[int]

    def file_offset(self, addr: int) -> Optional[int]:
        for seg in self.segments:
            if seg.vaddr <= addr < seg.vaddr + seg.filesz:
                return addr - seg.vaddr + seg.offset
        return None

    def vram_range(self) -> Tuple[int, int]:
        start = min(seg.vaddr for seg in self.segments)
        end = max(seg.vaddr + seg.memsz for seg in self.segments)
        return start, end


@dataclass
class SectionInfo:
    name: str
    kind: str
    align: Optional[int]


@dataclass
class UnitSection:
    name: str
    start: int
    end: int


@dataclass
class Unit:
    name: str
    sections: List[UnitSection] = field(default_factory=list)


@dataclass
class Symbol:
    name: str
    section: str
    address: int
    kind: Optional[str]
    size: Optional[int]
    scope: Optional[str]


def read_elf(path: Path) -> Elf:
    data = path.read_bytes()
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
        sys.exit(f"{path}: expected a 32-bit little-endian ELF")
    (e_phoff, e_shoff) = struct.unpack_from("<II", data, 0x1C)
    (e_phentsize, e_phnum, e_shentsize, e_shnum) = struct.unpack_from(
        "<HHHH", data, 0x2A
    )
    segments = []
    for i in range(e_phnum):
        p_type, p_offset, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from(
            "<IIIIII", data, e_phoff + i * e_phentsize
        )
        if p_type == PT_LOAD and p_memsz > 0:
            segments.append(Segment(p_vaddr, p_offset, p_filesz, p_memsz))
    gp = None
    for i in range(e_shnum):
        sh = struct.unpack_from("<IIIIIIIIII", data, e_shoff + i * e_shentsize)
        if sh[1] == SHT_MIPS_REGINFO and sh[5] >= 0x18:
            gp = struct.unpack_from("<I", data, sh[4] + 0x14)[0]
    return Elf(data, segments, gp)


def read_splits(path: Path) -> Tuple[Dict[str, SectionInfo], List[Unit]]:
    sections: Dict[str, SectionInfo] = {}
    units: List[Unit] = []
    in_sections = False
    unit: Optional[Unit] = None
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("//"):
            continue
        if line.strip() == "Sections:":
            in_sections = True
            unit = None
            continue
        match = UNIT_RE.match(line)
        if match:
            in_sections = False
            unit = Unit(match.group(1))
            units.append(unit)
            continue
        if in_sections:
            match = SECTION_HEADER_RE.match(line)
            if not match:
                sys.exit(f"{path}:{lineno}: bad section line")
            align = int(match.group(3)) if match.group(3) else None
            sections[match.group(1)] = SectionInfo(match.group(1), match.group(2), align)
            continue
        match = UNIT_SECTION_RE.match(line)
        if unit is None or not match:
            sys.exit(f"{path}:{lineno}: bad split line")
        name = match.group(1)
        if name not in sections:
            sys.exit(f"{path}:{lineno}: section {name} is not declared")
        unit.sections.append(
            UnitSection(name, int(match.group(2), 16), int(match.group(3), 16))
        )
    return sections, units


def read_symbols(path: Path) -> List[Symbol]:
    symbols = []
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip():
            continue
        match = SYMBOL_RE.match(line.strip())
        if not match:
            sys.exit(f"{path}:{lineno}: bad symbol line")
        attrs = {}
        for pair in (match.group(4) or "").split():
            key, _, value = pair.partition(":")
            attrs[key] = value
        size = int(attrs["size"], 0) if "size" in attrs else None
        symbols.append(
            Symbol(
                match.group(1),
                match.group(2),
                int(match.group(3), 16),
                attrs.get("type"),
                size,
                attrs.get("scope"),
            )
        )
    return symbols


def configure_spimdisasm(elf: Elf) -> None:
    config = common.GlobalConfig
    config.ENDIAN = common.InputEndian.LITTLE
    config.ABI = common.Abi.EABI64
    config.ARCHLEVEL = common.ArchLevel.MIPS3
    config.COMPILER = common.Compiler.MWCCPS2
    config.GP_VALUE = elf.gp
    config.PRODUCE_SYMBOLS_PLUS_OFFSET = True
    config.TRUST_USER_FUNCTIONS = True
    config.TRUST_JAL_FUNCTIONS = True
    vram_start, vram_end = elf.vram_range()
    config.SYMBOL_FINDER_FILTER_LOW_ADDRESSES = True
    config.SYMBOL_FINDER_FILTER_ADDRESSES_ADDR_LOW = vram_start
    config.SYMBOL_FINDER_FILTER_HIGH_ADDRESSES = True
    config.SYMBOL_FINDER_FILTER_ADDRESSES_ADDR_HIGH = vram_end
    config.ASM_USE_PRELUDE = False
    config.ASM_GENERATED_BY = False
    config.ASM_EMIT_SIZE_DIRECTIVE = False
    config.ASM_COMMENT = True
    config.QUIET = True
    rabbitizer.config.toolchainTweaks_treatJAsUnconditionalBranch = False


def add_symbols(context: common.Context, elf: Elf, symbols: List[Symbol]) -> None:
    for sym in symbols:
        rom = elf.file_offset(sym.address)
        if sym.kind == "function":
            context_sym = context.globalSegment.addFunction(sym.address, vromAddress=rom)
        else:
            context_sym = context.globalSegment.addSymbol(sym.address, vromAddress=rom)
        context_sym.name = sym.name
        context_sym.isUserDeclared = True
        context_sym.userDeclaredSize = sym.size
        if sym.scope in ("local", "weak"):
            context_sym.visibility = sym.scope


def create_section(
    context: common.Context,
    elf: Elf,
    info: SectionInfo,
    split: UnitSection,
    unit: Unit,
) -> mips.sections.SectionBase:
    if info.kind == "bss":
        # NOBITS ranges have no file offset; anchor them at the end of the
        # file so spimdisasm still gets a consistent (empty) vrom range.
        anchor = len(elf.data)
        return mips.sections.SectionBss(
            context, anchor, anchor, split.start, split.end, unit.name, 0, None
        )
    start = elf.file_offset(split.start)
    end = elf.file_offset(split.end - 1)
    if start is None or end is None:
        sys.exit(f"{unit.name}: {split.name} range is not file-backed")
    end += 1
    args = (context, start, end, split.start, unit.name, elf.data, 0, None)
    if info.kind == "code":
        section = mips.sections.SectionText(*args)
        section.instrCat = rabbitizer.InstrCategory.R5900
        return section
    if info.kind == "rodata":
        return mips.sections.SectionRodata(*args)
    return mips.sections.SectionData(*args)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--elf", type=Path, required=True, help="retail ELF")
    parser.add_argument("--sha1", help="expected SHA-1 of the retail ELF")
    parser.add_argument("--splits", type=Path, required=True, help="splits.txt")
    parser.add_argument("--symbols", type=Path, required=True, help="symbols.txt")
    parser.add_argument("--out", type=Path, required=True, help="output directory")
    args = parser.parse_args()

    if args.sha1 is not None:
        digest = hashlib.sha1(args.elf.read_bytes()).hexdigest()
        if digest != args.sha1.lower():
            sys.exit(f"{args.elf}: SHA-1 {digest} does not match {args.sha1}")
    elf = read_elf(args.elf)
    if elf.gp is None:
        sys.exit(f"{args.elf}: missing .reginfo gp value")
    section_infos, units = read_splits(args.splits)
    symbols = read_symbols(args.symbols)

    configure_spimdisasm(elf)
    context = common.Context()
    vram_start, vram_end = elf.vram_range()
    context.changeGlobalSegmentRanges(0, len(elf.data), vram_start, vram_end)
    add_symbols(context, elf, symbols)

    unit_sections: List[Tuple[Unit, List[Tuple[SectionInfo, mips.sections.SectionBase]]]] = []
    for unit in units:
        created = []
        for split in unit.sections:
            info = section_infos[split.name]
            created.append((info, create_section(context, elf, info, split, unit)))
        unit_sections.append((unit, created))

    # Analyze code first so data analysis sees every code reference.
    kind_order = {"code": 0, "data": 1, "rodata": 1, "bss": 2}
    for _, created in unit_sections:
        for info, section in sorted(created, key=lambda c: kind_order[c[0].kind]):
            section.analyze()

    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "macro.inc").write_text(MACROS)
    for unit, created in unit_sections:
        lines = [
            '.include "macro.inc"',
            "",
            ".set noat",
            ".set noreorder",
            "",
        ]
        for info, section in created:
            lines.append(f".section {info.name}, {SECTION_DIRECTIVES[info.kind]}")
            if info.align:
                lines.append(f".balign {info.align}")
            lines.append("")
            lines.append(section.disassemble())
        out_path = args.out / Path(unit.name).with_suffix(".s")
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text("\n".join(lines))


if __name__ == "__main__":
    main()
