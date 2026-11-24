#!/usr/bin/env python3
"""Split the PS2 retail ELF into per-unit assembly for objdiff targets.

The PS2 release (SLUS_208.81) ships one merged PROGBITS section and no
symbols, so DTK cannot split it. This tool reads a DTK-style splits.txt and
symbols.txt, disassembles the listed unit ranges with spimdisasm, and writes
one GNU assembler file per unit. Assembling those files yields relocatable
target objects whose relocations name the same symbols as the compiled source.

When the `Sections:` entries of splits.txt carry start/end addresses, they
describe the whole retail image. Every range of a section that no listed unit
covers becomes a generated unit named auto_<index>_<address>_<section>, like
DTK's auto splits, so the progress report counts the whole ELF. Generated
units are disassembled in their own spimdisasm context, which keeps the
output of the listed units independent of them.

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
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

try:
    import rabbitizer
    from spimdisasm import common, mips
except ImportError:
    # The layout helpers below work without spimdisasm (configure.py uses
    # them); main() needs it.
    rabbitizer = None


PT_LOAD = 1
SHT_MIPS_REGINFO = 0x70000006

SECTION_HEADER_RE = re.compile(
    r"^\s+(\.\w+)\s+type:(\w+)(?:\s+align:(\d+))?"
    r"(?:\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+))?\s*$"
)
UNIT_RE = re.compile(r"^(\S.*):\s*$")
UNIT_SECTION_RE = re.compile(
    r"^\s+(\.\w+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)\s*$"
)
# Symbol names GNU as only accepts quoted, such as MW's "@stringBase0" literals.
NEEDS_QUOTE_RE = re.compile(r"[^A-Za-z0-9_.]")

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
    # Address range of the section in the retail image, when declared.
    start: Optional[int] = None
    end: Optional[int] = None


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
    name: Optional[str]
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
            start = int(match.group(4), 16) if match.group(4) else None
            end = int(match.group(5), 16) if match.group(5) else None
            sections[match.group(1)] = SectionInfo(
                match.group(1), match.group(2), align, start, end
            )
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


def has_layout(sections: Dict[str, SectionInfo]) -> bool:
    """True when the Sections: entries give the address range of each section."""
    return any(info.start is not None for info in sections.values())


def section_at(sections: Dict[str, SectionInfo], addr: int) -> Optional[str]:
    """Name of the declared section whose range holds the address."""
    for info in sections.values():
        if info.start is not None and info.start <= addr < info.end:
            return info.name
    return None


def auto_unit_name(index: int, start: int, section: str) -> str:
    return f"auto_{index:02}_{start:08X}_{section.lstrip('.')}.o"


def auto_units(
    sections: Dict[str, SectionInfo], units: List[Unit], where: str = "splits.txt"
) -> List[Unit]:
    """Generated units for the section ranges that no listed unit covers.

    Checks that the section ranges are ordered and disjoint and that each
    listed unit range lies inside its section without overlapping another.
    Returns nothing when the sections declare no ranges.
    """
    if not has_layout(sections):
        return []
    infos = list(sections.values())
    previous: Optional[SectionInfo] = None
    for info in infos:
        if info.start is None or info.end is None or info.start > info.end:
            sys.exit(f"{where}: section {info.name} needs a start:/end: range")
        if previous is not None and info.start < previous.end:
            sys.exit(f"{where}: section {info.name} overlaps {previous.name}")
        previous = info
    taken: Dict[str, List[Tuple[int, int, str]]] = {info.name: [] for info in infos}
    for unit in units:
        for split in unit.sections:
            info = sections[split.name]
            if not info.start <= split.start <= split.end <= info.end:
                sys.exit(
                    f"{where}: {unit.name} {split.name} 0x{split.start:08X}-"
                    f"0x{split.end:08X} is outside the section "
                    f"0x{info.start:08X}-0x{info.end:08X}"
                )
            taken[split.name].append((split.start, split.end, unit.name))
    autos: List[Unit] = []
    for index, info in enumerate(infos):
        cursor = info.start
        last = ""
        for start, end, name in sorted(taken[info.name]):
            if start < cursor:
                sys.exit(f"{where}: {name} {info.name} overlaps {last}")
            if start > cursor:
                autos.append(_auto_unit(index, info.name, cursor, start))
            cursor = max(cursor, end)
            last = name
        if cursor < info.end:
            autos.append(_auto_unit(index, info.name, cursor, info.end))
    return autos


def _auto_unit(index: int, section: str, start: int, end: int) -> Unit:
    return Unit(auto_unit_name(index, start, section), [UnitSection(section, start, end)])


def check_layout(
    elf: Elf, sections: Dict[str, SectionInfo], symbols: List["Symbol"], where: str
) -> None:
    """Check the declared layout against the retail image.

    The sections must cover every loaded byte of each file-backed segment;
    bytes between sections must be zero padding. NOBITS (bss) sections lie
    past the file data and the others inside it. A symbol inside the image
    must name the section whose range holds its address; symbols outside it
    (overlay addresses) are not checked.
    """
    placed = set()
    for seg in elf.segments:
        if seg.filesz == 0:
            continue
        file_end = seg.vaddr + seg.filesz
        seg_end = seg.vaddr + seg.memsz
        cursor = seg.vaddr
        for info in sections.values():
            if not seg.vaddr <= info.start < seg_end:
                continue
            placed.add(info.name)
            if info.end > seg_end:
                sys.exit(f"{where}: section {info.name} runs past the segment end")
            if info.kind == "bss" and info.start < file_end:
                sys.exit(f"{where}: bss section {info.name} overlaps file data")
            if info.kind != "bss" and info.end > file_end:
                sys.exit(f"{where}: section {info.name} runs past the file data")
            _check_padding(elf, cursor, info.start, where)
            cursor = info.end
        _check_padding(elf, cursor, seg_end, where)
    for info in sections.values():
        if info.name not in placed:
            sys.exit(f"{where}: section {info.name} is outside the loaded image")
    images = [(seg.vaddr, seg.vaddr + seg.memsz) for seg in elf.segments if seg.filesz]
    for sym in symbols:
        found = section_at(sections, sym.address)
        inside = any(lo <= sym.address < hi for lo, hi in images)
        if found != sym.section and (found is not None or inside):
            sys.exit(
                f"{where}: {sym.name} at 0x{sym.address:08X} is in "
                f"{found or 'no section'}, not {sym.section}"
            )


def _check_padding(elf: Elf, start: int, end: int, where: str) -> None:
    """Bytes outside every section must be file-backed zeros."""
    if start >= end:
        return
    first = elf.file_offset(start)
    last = elf.file_offset(end - 1)
    if first is None or last is None or any(elf.data[first:last + 1]):
        sys.exit(f"{where}: 0x{start:08X}-0x{end:08X} lies outside every section")


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
        if sym.name is not None:
            context_sym.name = sym.name
        context_sym.isUserDeclared = True
        context_sym.userDeclaredSize = sym.size
        if sym.scope in ("local", "weak"):
            context_sym.visibility = sym.scope


def leading_zero_bytes(elf: Elf, split: UnitSection) -> int:
    """Bytes of zero words at the start of a range (alignment padding)."""
    offset = elf.file_offset(split.start)
    count = 0
    while (
        split.start + count + 4 <= split.end
        and elf.data[offset + count:offset + count + 4] == b"\0\0\0\0"
    ):
        count += 4
    return count


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


def quote_symbol_names(asm: str, names: List[str]) -> str:
    """Quote the given symbol names wherever they appear as whole tokens."""
    if not names:
        return asm
    pattern = re.compile(
        r'(?<![\w"@$.])('
        + "|".join(re.escape(n) for n in sorted(set(names), key=len, reverse=True))
        + r')(?![\w"@$])'
    )
    return pattern.sub(lambda m: f'"{m.group(1)}"', asm)


# One section of a unit: its info, the spimdisasm section (None for a range
# of zero padding only) and the padding bytes emitted before it.
Created = Tuple[SectionInfo, Optional["mips.sections.SectionBase"], int]


def new_context(elf: Elf, symbols: List[Symbol]) -> common.Context:
    context = common.Context()
    vram_start, vram_end = elf.vram_range()
    context.changeGlobalSegmentRanges(0, len(elf.data), vram_start, vram_end)
    add_symbols(context, elf, symbols)
    return context


def split_units(
    elf: Elf, section_infos: Dict[str, SectionInfo], units: List[Unit], symbols: List[Symbol]
) -> List[Tuple[Unit, List[Created]]]:
    """Analyze the listed units in one context, each unit's code first."""
    context = new_context(elf, symbols)
    unit_sections: List[Tuple[Unit, List[Created]]] = []
    for unit in units:
        created: List[Created] = []
        for split in unit.sections:
            info = section_infos[split.name]
            created.append((info, create_section(context, elf, info, split, unit), 0))
        unit_sections.append((unit, created))

    # Analyze code first so data analysis sees every code reference.
    kind_order = {"code": 0, "data": 1, "rodata": 1, "bss": 2}
    for _, created in unit_sections:
        for info, section, _ in sorted(created, key=lambda c: kind_order[c[0].kind]):
            section.analyze()
    return unit_sections


def split_auto_units(
    elf: Elf, section_infos: Dict[str, SectionInfo], units: List[Unit], symbols: List[Symbol]
) -> List[Tuple[Unit, List[Created]]]:
    """Analyze the generated units in a context of their own.

    All code is analyzed before any data, so jal targets and data references
    from every generated unit are known. Symbol sizes from symbols.txt are
    dropped here: they were measured for the listed units, and spimdisasm
    stops splitting functions after a user size that ends before a delay
    slot. Names GNU as only takes quoted (the label macros drop the quotes)
    or that symbols.txt repeats are left to spimdisasm. Zero words at the
    start of a code range are alignment padding and are emitted as .space,
    so they do not become a function.
    """
    counts = Counter(s.name for s in symbols)
    symbols = [
        Symbol(
            s.name if counts[s.name] == 1 and not NEEDS_QUOTE_RE.search(s.name) else None,
            s.section, s.address, s.kind, None, s.scope,
        )
        for s in symbols
    ]
    context = new_context(elf, symbols)
    unit_sections: List[Tuple[Unit, List[Created]]] = []
    for unit in units:
        created: List[Created] = []
        for split in unit.sections:
            info = section_infos[split.name]
            pad = leading_zero_bytes(elf, split) if info.kind == "code" else 0
            if pad == split.end - split.start:
                created.append((info, None, pad))
                continue
            trimmed = UnitSection(split.name, split.start + pad, split.end)
            created.append((info, create_section(context, elf, info, trimmed, unit), pad))
        unit_sections.append((unit, created))
    kind_order = {"code": 0, "data": 1, "rodata": 1, "bss": 2}
    for kind in (0, 1, 2):
        for _, created in unit_sections:
            for info, section, _ in created:
                if section is not None and kind_order[info.kind] == kind:
                    section.analyze()
    return unit_sections


def write_units(
    out: Path, unit_sections: List[Tuple[Unit, List[Created]]], quoted: List[str]
) -> None:
    for unit, created in unit_sections:
        lines = [
            '.include "macro.inc"',
            "",
            ".set noat",
            ".set noreorder",
            "",
        ]
        for info, section, pad in created:
            lines.append(f".section {info.name}, {SECTION_DIRECTIVES[info.kind]}")
            if info.align:
                lines.append(f".balign {info.align}")
            lines.append("")
            if pad:
                lines.append(f".space 0x{pad:X}")
                lines.append("")
            if section is not None:
                lines.append(quote_symbol_names(section.disassemble(), quoted))
        out_path = out / Path(unit.name).with_suffix(".s")
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text("\n".join(lines))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--elf", type=Path, required=True, help="retail ELF")
    parser.add_argument("--sha1", help="expected SHA-1 of the retail ELF")
    parser.add_argument("--splits", type=Path, required=True, help="splits.txt")
    parser.add_argument("--symbols", type=Path, required=True, help="symbols.txt")
    parser.add_argument("--out", type=Path, required=True, help="output directory")
    args = parser.parse_args()

    if rabbitizer is None:
        sys.exit(
            "ps2_split.py requires spimdisasm. Install it with:\n"
            "    python3 -m pip install -r tools/requirements-ps2.txt"
        )
    if args.sha1 is not None:
        digest = hashlib.sha1(args.elf.read_bytes()).hexdigest()
        if digest != args.sha1.lower():
            sys.exit(f"{args.elf}: SHA-1 {digest} does not match {args.sha1}")
    elf = read_elf(args.elf)
    if elf.gp is None:
        sys.exit(f"{args.elf}: missing .reginfo gp value")
    section_infos, units = read_splits(args.splits)
    symbols = read_symbols(args.symbols)
    autos = auto_units(section_infos, units, str(args.splits))
    if has_layout(section_infos):
        check_layout(elf, section_infos, symbols, str(args.splits))

    configure_spimdisasm(elf)
    quoted = [sym.name for sym in symbols if NEEDS_QUOTE_RE.search(sym.name)]
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "macro.inc").write_text(MACROS)
    write_units(args.out, split_units(elf, section_infos, units, symbols), quoted)
    if autos:
        # Generated units hold kernel and library code that names COP0 and
        # FPU control registers; GNU as only takes their numbers. The game
        # is English, so rodata strings are ASCII: spimdisasm's default
        # EUC-JP guess turns some float tables into multi-byte strings that
        # assemble to different bytes.
        rabbitizer.config.regNames_userFpcCsr = False
        rabbitizer.config.regNames_vr4300Cop0NamedRegisters = False
        common.GlobalConfig.RODATA_STRING_ENCODING = "ASCII"
        write_units(args.out, split_auto_units(elf, section_infos, autos, symbols), [])


if __name__ == "__main__":
    main()
