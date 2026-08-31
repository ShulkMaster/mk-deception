#!/usr/bin/env python3
"""Generate version-specific C initializer includes from split retail objects.

The generated files deliberately live below the build directory.  This script
contains the source-level schemas needed to express the data, but none of the
retail payload.  Payload words, strings, symbol extents, and pointer targets are
read from the ELF objects produced by the DTK split.
"""

from __future__ import annotations

import argparse
import os
import struct
import tempfile
from dataclasses import dataclass
from pathlib import Path


R_PPC_ADDR32 = 1


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    type: int
    offset: int
    size: int
    link: int
    info: int
    entsize: int
    data: bytes


@dataclass(frozen=True)
class Symbol:
    index: int
    name: str
    value: int
    size: int
    bind: int
    type: int
    section_index: int


@dataclass(frozen=True)
class Relocation:
    offset: int
    type: int
    symbol: Symbol
    addend: int


class Elf32:
    """The small subset of big-endian ELF32 needed by generated includes."""

    def __init__(self, path: Path):
        self.path = path
        blob = path.read_bytes()
        if blob[:4] != b"\x7fELF" or blob[4] != 1 or blob[5] != 2:
            raise ValueError(f"{path}: expected big-endian ELF32")

        header = struct.unpack_from(">16sHHIIIIIHHHHHH", blob, 0)
        section_offset = header[6]
        section_entry_size = header[11]
        section_count = header[12]
        section_name_index = header[13]
        raw_sections = [
            struct.unpack_from(">IIIIIIIIII", blob, section_offset + i * section_entry_size)
            for i in range(section_count)
        ]
        shstr = raw_sections[section_name_index]
        shstr_data = blob[shstr[4] : shstr[4] + shstr[5]]

        self.sections: list[Section] = []
        self.sections_by_name: dict[str, Section] = {}
        for index, raw in enumerate(raw_sections):
            name = self._cstring(shstr_data, raw[0]).decode("ascii") if raw[0] else ""
            data = b"" if raw[1] == 8 else blob[raw[4] : raw[4] + raw[5]]
            section = Section(index, name, raw[1], raw[4], raw[5], raw[6], raw[7], raw[9], data)
            self.sections.append(section)
            self.sections_by_name[name] = section

        symtab = self.sections_by_name.get(".symtab")
        if symtab is None:
            raise ValueError(f"{path}: no .symtab")
        strings = self.sections[symtab.link].data
        entry_size = symtab.entsize or 16
        self.symbols: list[Symbol] = []
        self.symbols_by_name: dict[str, Symbol] = {}
        for index, off in enumerate(range(0, symtab.size, entry_size)):
            name_off, value, size, info, _other, section_index = struct.unpack_from(
                ">IIIBBH", symtab.data, off
            )
            name = self._cstring(strings, name_off).decode("ascii") if name_off else ""
            symbol = Symbol(index, name, value, size, info >> 4, info & 0xF, section_index)
            self.symbols.append(symbol)
            if name:
                self.symbols_by_name[name] = symbol

        self.relocations_by_section: dict[int, dict[int, Relocation]] = {}
        for section in self.sections:
            if section.type != 4:  # SHT_RELA
                continue
            target = section.info
            relocs = self.relocations_by_section.setdefault(target, {})
            entry_size = section.entsize or 12
            for off in range(0, section.size, entry_size):
                reloc_offset, info, addend = struct.unpack_from(">IIi", section.data, off)
                symbol_index = info >> 8
                reloc_type = info & 0xFF
                if reloc_offset in relocs:
                    raise ValueError(f"{path}: duplicate relocation at 0x{reloc_offset:X}")
                relocs[reloc_offset] = Relocation(
                    reloc_offset, reloc_type, self.symbols[symbol_index], addend
                )

    @staticmethod
    def _cstring(data: bytes, offset: int) -> bytes:
        end = data.find(b"\0", offset)
        return data[offset:] if end < 0 else data[offset:end]

    def section(self, name: str) -> Section:
        try:
            return self.sections_by_name[name]
        except KeyError as exc:
            raise ValueError(f"{self.path}: missing section {name}") from exc

    def symbol(self, name: str) -> Symbol:
        try:
            return self.symbols_by_name[name]
        except KeyError as exc:
            raise ValueError(f"{self.path}: missing symbol {name}") from exc

    def symbol_data(self, name: str) -> bytes:
        symbol = self.symbol(name)
        section = self.sections[symbol.section_index]
        return section.data[symbol.value : symbol.value + symbol.size]

    def relocations(self, section_name: str) -> dict[int, Relocation]:
        section = self.section(section_name)
        return self.relocations_by_section.get(section.index, {})

    def objects(self, section_name: str) -> list[Symbol]:
        section = self.section(section_name)
        return sorted(
            (s for s in self.symbols if s.section_index == section.index and s.type == 1 and s.size),
            key=lambda s: (s.value, s.index),
        )


def words(data: bytes) -> list[int]:
    if len(data) % 4:
        raise ValueError(f"word data has non-word size 0x{len(data):X}")
    return list(struct.unpack(f">{len(data) // 4}I", data))


def signed(value: int) -> int:
    return value - 0x100000000 if value & 0x80000000 else value


def byte_initializer(data: bytes, indent: str = "    ", width: int = 16) -> str:
    lines = []
    for offset in range(0, len(data), width):
        chunk = data[offset : offset + width]
        lines.append(indent + ", ".join(f"0x{value:02X}" for value in chunk) + ",")
    return "\n".join(lines)


def u32_initializer(values: list[int], indent: str = "    ", width: int = 8, suffix: str = "") -> str:
    lines = []
    for offset in range(0, len(values), width):
        chunk = values[offset : offset + width]
        lines.append(indent + ", ".join(f"0x{value:08X}{suffix}" for value in chunk) + ",")
    return "\n".join(lines)


def c_string(data: bytes) -> str:
    result = ['"']
    for value in data:
        if value == 0:
            break
        if value == 0x22:
            result.append(r'\"')
        elif value == 0x5C:
            result.append(r"\\")
        elif 0x20 <= value < 0x7F:
            result.append(chr(value))
        elif value == 0x0A:
            result.append(r"\n")
        elif value == 0x09:
            result.append(r"\t")
        else:
            result.append(f"\\{value:03o}")
    result.append('"')
    return "".join(result)


def reloc_expr(reloc: Relocation, local_names: dict[str, str] | None = None) -> str:
    if reloc.type != R_PPC_ADDR32:
        raise ValueError(f"unsupported relocation type {reloc.type} at 0x{reloc.offset:X}")
    name = (local_names or {}).get(reloc.symbol.name, reloc.symbol.name)
    if not name or name.startswith("@"):
        raise ValueError(f"cannot express relocation target {reloc.symbol.name!r}")
    if reloc.addend == 0:
        return name
    sign = "+" if reloc.addend > 0 else "-"
    return f"{name} {sign} 0x{abs(reloc.addend):X}"


def string_reloc_expr(reloc: Relocation, base_name: str = "stringBase0") -> str:
    if reloc.type != R_PPC_ADDR32 or reloc.symbol.name != "@stringBase0":
        raise ValueError(f"expected @stringBase0 ADDR32 relocation at 0x{reloc.offset:X}")
    return f"(char*)&{base_name}[0x{reloc.addend:X}]"


def emit_nbc(elf: Elf32) -> dict[str, str]:
    pool = elf.symbol("@stringBase0")
    if pool.size != 0x1A64 or pool.section_index != elf.section(".rodata").index:
        raise ValueError("nbc.o: unexpected @stringBase0 layout")
    pool_text = "/* Generated from retail nbc.o @stringBase0. */\n" + byte_initializer(
        elf.symbol_data("@stringBase0")
    ) + "\n"

    table = elf.symbol("nbc_general_text")
    if table.size % 4 or table.section_index != elf.section(".data").index:
        raise ValueError("nbc.o: unexpected nbc_general_text layout")
    relocs = elf.relocations(".data")
    lines = ["/* Generated pointer table into stringBase0. */"]
    for offset in range(table.value, table.value + table.size, 4):
        reloc = relocs.get(offset)
        if reloc is None:
            raise ValueError(f"nbc.o: missing table relocation at 0x{offset:X}")
        lines.append(f"    &stringBase0[0x{reloc.addend:X}],")
    return {
        "game/nbc_stringBase0.inc": pool_text,
        "game/nbc_general_text.inc": "\n".join(lines) + "\n",
    }


def emit_pselect(elf: Elf32) -> dict[str, str]:
    pool = elf.symbol("@stringBase0")
    if pool.section_index != elf.section(".rodata").index:
        raise ValueError("pselect.o: @stringBase0 is not in .rodata")
    text = ["/* Generated from retail pselect.o @stringBase0. */"]
    text.append("static const char stringBase0[] = {")
    text.append(byte_initializer(elf.symbol_data("@stringBase0")))
    text.append("};")
    return {"game/pselect_stringBase0.inc": "\n".join(text) + "\n"}


def emit_sqrt(elf: Elf32) -> dict[str, str]:
    symbol = elf.symbol("GXMathSqrtTable")
    if symbol.size != 0x4000 or symbol.section_index != elf.section(".data").index:
        raise ValueError("gxMath.o: unexpected GXMathSqrtTable layout")
    values = list(struct.unpack(">8192H", elf.symbol_data(symbol.name)))
    lines = ["/* Generated from retail gxMath.o GXMathSqrtTable. */"]
    lines.append("unsigned short GXMathSqrtTable[0x2000] = {")
    for offset in range(0, len(values), 16):
        lines.append("    " + ", ".join(f"0x{x:04X}" for x in values[offset : offset + 16]) + ",")
    lines.append("};")
    return {"src/math/gxmath_sqrt_table.inc": "\n".join(lines) + "\n"}


def emit_fonts(elf: Elf32) -> dict[str, str]:
    pool = elf.symbol("@stringBase0")
    font_table = elf.symbol("font_table")
    string_table = elf.symbol("string_table")
    size_symbol = elf.symbol("string_tbl_size")
    if (pool.size, font_table.size, string_table.size, size_symbol.size) != (
        0x2E98,
        18 * 24,
        155 * 24,
        4,
    ):
        raise ValueError("fonts.o: unexpected generated-data symbol sizes")
    data = elf.section(".data").data
    relocs = elf.relocations(".data")
    lines = ["/* Generated from retail fonts.o; do not hand-edit. */", ""]
    lines.append(f"static const char stringBase0[0x{pool.size:X}] = {{")
    lines.append(byte_initializer(elf.symbol_data(pool.name)))
    lines.extend(["};", "", "FontTableEntry font_table[18] = {"])
    for row in range(18):
        base = font_table.value + row * 24
        row_words = words(data[base : base + 24])
        name_reloc = relocs.get(base)
        if name_reloc is None:
            raise ValueError(f"fonts.o: missing font name relocation for row {row}")
        name = string_reloc_expr(name_reloc)
        path_reloc = relocs.get(base + 12)
        if path_reloc is not None:
            path = string_reloc_expr(path_reloc)
        elif row_words[3] == 0:
            path = "0"
        else:
            path = f"(char*)0x{row_words[3]:08X}"
        lines.append(
            "    { %s, 0x%08X, 0x%08X, %s, { 0x%08X, 0x%08X } },"
            % (name, row_words[1], row_words[2], path, row_words[4], row_words[5])
        )
    lines.extend(["};", "", "FontStringRow string_table[155] = {"])
    for row in range(155):
        base = string_table.value + row * 24
        expressions = []
        for column in range(6):
            reloc = relocs.get(base + column * 4)
            if reloc is None:
                raise ValueError(f"fonts.o: missing string relocation for row {row}, column {column}")
            expressions.append(string_reloc_expr(reloc))
        lines.append("    " + ", ".join(expressions) + ",")
    size_value = words(elf.symbol_data(size_symbol.name))[0]
    lines.extend(["};", "", f"int string_tbl_size = 0x{size_value:X};"])
    return {"runtime/fonts_data.inc": "\n".join(lines) + "\n"}


def emit_moves(elf: Elf32) -> dict[str, str]:
    data_section = elf.section(".data")
    start = elf.symbol("jump_table")
    stop = elf.symbol("weapon_grab_table")
    relocs = elf.relocations(".data")
    tables = [s for s in elf.objects(".data") if start.value <= s.value < stop.value and s.bind != 0]
    if not tables or tables[0].name != "jump_table" or tables[-1].value + tables[-1].size != stop.value:
        raise ValueError("moves.o: scan tables are not a contiguous named range")
    lines = [
        "/* Generated from retail moves.o data symbols and relocations. */",
        "typedef union MovesScanWord {",
        "    unsigned int value;",
        "    MovesEntryFn action;",
        "    void (*void_action)(void);",
        "} MovesScanWord;",
        "",
    ]
    for table in tables:
        if table.size % 4:
            raise ValueError(f"moves.o: {table.name} has non-word size")
        lines.append(f"MovesScanWord {table.name}[{table.size // 4}] = {{")
        for offset in range(table.value, table.value + table.size, 4):
            reloc = relocs.get(offset)
            if reloc is not None:
                expression = reloc_expr(reloc)
                lines.append(f"    {{(unsigned int){expression}}},")
            else:
                value = struct.unpack_from(">I", data_section.data, offset)[0]
                lines.append(f"    {{0x{value:08X}u}},")
        lines.extend(["};", ""])
    return {"src/game/moves_scan_tables.inc": "\n".join(lines)}


def emit_reactions(elf: Elf32) -> dict[str, str]:
    table = elf.symbol("tbl_xfer_addresses")
    if table.size % 20 or table.section_index != elf.section(".rodata").index:
        raise ValueError("reactions.o: unexpected tbl_xfer_addresses layout")
    rodata = elf.section(".rodata").data
    relocs = elf.relocations(".rodata")

    targets: dict[str, Symbol] = {}
    table_relocs: dict[int, Relocation] = {}
    for offset, reloc in relocs.items():
        if table.value <= offset < table.value + table.size:
            table_relocs[offset] = reloc
            targets[reloc.symbol.name] = reloc.symbol
    prototype_lines = ["/* Function relocations referenced by the retail table. */"]
    for name in sorted(targets):
        symbol = targets[name]
        storage = "static " if symbol.bind == 0 and symbol.section_index != 0 else ""
        prototype_lines.append(f"{storage}float {name}(void);")

    lines = [
        f"/* Generated from retail reactions.o; {table.size // 20} records. */",
        "static const ReactionXferAddress tbl_xfer_addresses[] = {",
    ]
    for base in range(table.value, table.value + table.size, 20):
        row = words(rodata[base : base + 20])
        reloc = table_relocs.get(base + 4)
        if reloc is None:
            entry = f"(ReactionEntry)0x{row[1]:08X}"
        else:
            entry = reloc_expr(reloc)
        lines.append(
            "    {{0x%08X, %s}, 0x%08X, 0x%08X, 0x%08X},"
            % (row[0], entry, row[2], row[3], row[4])
        )
    lines.append("};")
    return {
        "src/game/reactions_table_prototypes.inc": "\n".join(prototype_lines) + "\n",
        "src/game/reactions_table.inc": "\n".join(lines) + "\n",
    }


SOUND_CALL_TABLES = {"voice_call_table", "hit_call_table", "pf_hit_call_table", "foot_call_table"}


def emit_sound_calls(elf: Elf32) -> str:
    data = elf.section(".data")
    sdata = elf.section(".sdata")
    data_relocs = elf.relocations(".data")
    first = elf.symbol("vct_attack_yell_quick")
    stop = elf.symbol("sbank_data")
    data_symbols = [s for s in elf.objects(".data") if first.value <= s.value < stop.value]
    sdata_symbols = [s for s in elf.objects(".sdata") if s.value < sdata.size]

    referenced: set[str] = set()
    for symbol in data_symbols:
        if symbol.name not in SOUND_CALL_TABLES:
            continue
        for offset in range(symbol.value, symbol.value + symbol.size, 8):
            reloc = data_relocs.get(offset)
            if reloc is not None:
                referenced.add(reloc.symbol.name)
    lines = ["/* Generated from retail sound.o call-table data. */"]
    lines.extend(f"extern int {name}[];" for name in sorted(referenced))
    lines.append("")

    for symbol in data_symbols + sdata_symbols:
        section = data if symbol.section_index == data.index else sdata
        relocs = data_relocs if section is data else elf.relocations(".sdata")
        if symbol.name in SOUND_CALL_TABLES:
            lines.append(f"SoundCallTable {symbol.name}[{symbol.size // 8}] = {{")
            for offset in range(symbol.value, symbol.value + symbol.size, 8):
                raw = words(section.data[offset : offset + 8])
                reloc = relocs.get(offset)
                pointer = reloc_expr(reloc) if reloc else ("0" if raw[0] == 0 else f"(int*)0x{raw[0]:08X}")
                lines.append(f"    {{{pointer}, {signed(raw[1])}}},")
        else:
            if symbol.size % 4:
                raise ValueError(f"sound.o: {symbol.name} has non-word size")
            if symbol.name == "mk_foot_sound_table":
                declaration = "int mk_foot_sound_table[7][4] = {"
            else:
                declaration = f"int {symbol.name}[{symbol.size // 4}] = {{"
            lines.append(declaration)
            values = words(section.data[symbol.value : symbol.value + symbol.size])
            lines.append(u32_initializer(values))
        lines.extend(["};", ""])
    return "\n".join(lines)


def decode_relocated_string(elf: Elf32, reloc: Relocation) -> str:
    if reloc.type != R_PPC_ADDR32:
        raise ValueError(f"sound.o: unsupported string relocation type {reloc.type}")
    symbol = reloc.symbol
    if symbol.section_index == 0:
        raise ValueError(f"sound.o: string relocation uses undefined {symbol.name}")
    section = elf.sections[symbol.section_index]
    start = symbol.value + reloc.addend
    return c_string(section.data[start:])


def emit_sound_banks(elf: Elf32) -> str:
    data = elf.section(".data")
    relocs = elf.relocations(".data")
    sbank = elf.symbol("sbank_data")
    loaded = elf.symbol("loaded_sbank_data")
    load_table = elf.symbol("bank_load_table")
    bank_lists = [
        s
        for s in elf.objects(".data")
        if loaded.value + loaded.size <= s.value < load_table.value
    ]
    if sbank.size % 36 or loaded.size % 8 or load_table.size % 20:
        raise ValueError("sound.o: unexpected bank-data record size")

    lines = ["/* Generated from retail sound.o bank data. */"]
    lines.extend(f"extern unsigned int {s.name}[];" for s in bank_lists)
    lines.extend(["", f"SoundBankData sbank_data[{sbank.size // 36}] = {{"])
    for base in range(sbank.value, sbank.value + sbank.size, 36):
        row = words(data.data[base : base + 36])
        name_reloc = relocs.get(base + 12)
        name = decode_relocated_string(elf, name_reloc) if name_reloc else "0"
        active_bytes = data.data[base + 8 : base + 12]
        callback_bytes = data.data[base + 20 : base + 24]
        callback_reloc = relocs.get(base + 16)
        bank_reloc = relocs.get(base + 28)
        callback = reloc_expr(callback_reloc) if callback_reloc else ("0" if row[4] == 0 else f"(void*)0x{row[4]:08X}")
        bank = reloc_expr(bank_reloc) if bank_reloc else ("0" if row[7] == 0 else f"(int*)0x{row[7]:08X}")
        lines.append(
            "    {%d, %d, %d, {%d, %d, %d}, %s, %s, %d, {%d, %d, %d}, %d, %s, %d},"
            % (
                signed(row[0]), signed(row[1]), active_bytes[0], active_bytes[1], active_bytes[2], active_bytes[3],
                name, callback, callback_bytes[0], callback_bytes[1], callback_bytes[2], callback_bytes[3],
                signed(row[6]), bank, signed(row[8]),
            )
        )
    lines.extend(["};", "", f"LoadedSoundBank loaded_sbank_data[{loaded.size // 8}] = {{"])
    for base in range(loaded.value, loaded.value + loaded.size, 8):
        row = words(data.data[base : base + 8])
        state = data.data[base + 4 : base + 8]
        lines.append(f"    {{{signed(row[0])}, {state[0]}, {{{state[1]}, {state[2]}, {state[3]}}}}},")
    lines.extend(["};", ""])

    for symbol in bank_lists:
        values = words(data.data[symbol.value : symbol.value + symbol.size])
        lines.append(f"unsigned int {symbol.name}[{len(values)}] = {{")
        lines.append(u32_initializer(values, suffix="U"))
        lines.extend(["};", ""])

    lines.append(f"SoundBankLoadMode bank_load_table[{load_table.size // 20}] = {{")
    for base in range(load_table.value, load_table.value + load_table.size, 20):
        row = words(data.data[base : base + 20])
        list_reloc = relocs.get(base)
        name_reloc = relocs.get(base + 16)
        if list_reloc is None or name_reloc is None:
            raise ValueError(f"sound.o: incomplete bank_load_table relocations at 0x{base:X}")
        lines.append(
            "    {%s, %d, %d, %d, %s},"
            % (
                reloc_expr(list_reloc), signed(row[1]), signed(row[2]), signed(row[3]),
                decode_relocated_string(elf, name_reloc),
            )
        )
    lines.append("};")
    return "\n".join(lines) + "\n"


def emit_sound(elf: Elf32) -> dict[str, str]:
    return {
        "src/game/sound_call_tables.inc": emit_sound_calls(elf),
        "src/game/sound_bank_data.inc": emit_sound_banks(elf),
    }


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


def generate(object_root: Path) -> dict[str, str]:
    outputs: dict[str, str] = {}
    jobs = (
        ("nbc.o", emit_nbc),
        ("pselect.o", emit_pselect),
        ("fonts.o", emit_fonts),
        ("gxMath.o", emit_sqrt),
        ("moves.o", emit_moves),
        ("reactions.o", emit_reactions),
        ("sound.o", emit_sound),
    )
    for object_name, emitter in jobs:
        object_path = object_root / object_name
        if not object_path.is_file():
            raise FileNotFoundError(f"required retail object not found: {object_path}")
        for relative, contents in emitter(Elf32(object_path)).items():
            if relative in outputs:
                raise ValueError(f"duplicate generated path: {relative}")
            outputs[relative] = contents
    if len(outputs) != 10:
        raise AssertionError(f"expected 10 generated includes, got {len(outputs)}")
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--object-root", type=Path, default=Path("build/GQNE5D/obj"),
        help="directory containing DTK-split retail objects",
    )
    parser.add_argument(
        "--output-root", type=Path, default=Path("build/GQNE5D/include"),
        help="ignored include root populated by this generator",
    )
    args = parser.parse_args()
    outputs = generate(args.object_root)
    for relative in sorted(outputs):
        destination = args.output_root / relative
        atomic_write(destination, outputs[relative])
        print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
