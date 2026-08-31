#!/usr/bin/env python3
"""Generate gcdisplay C initializer includes from its retail object data."""

from pathlib import Path
import struct
import sys


def write_values(path: Path, values: list[int], width: int) -> None:
    per_line = 8 if width == 4 else 16
    with path.open("w", encoding="ascii", newline="\n") as output:
        for index in range(0, len(values), per_line):
            row = values[index:index + per_line]
            output.write("    " + ", ".join(f"0x{value:0{width}X}" for value in row) + ",\n")


def read_elf_section(path: Path, name: str) -> bytes:
    data = path.read_bytes()
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 2:
        return data

    section_offset = struct.unpack_from(">I", data, 0x20)[0]
    section_size = struct.unpack_from(">H", data, 0x2E)[0]
    section_count = struct.unpack_from(">H", data, 0x30)[0]
    names_index = struct.unpack_from(">H", data, 0x32)[0]
    if section_size < 0x28 or names_index >= section_count:
        raise ValueError(f"invalid ELF section table in {path}")

    names_header = section_offset + names_index * section_size
    names_offset, names_size = struct.unpack_from(">II", data, names_header + 0x10)
    names = data[names_offset:names_offset + names_size]
    for index in range(section_count):
        header = section_offset + index * section_size
        name_offset = struct.unpack_from(">I", data, header)[0]
        end = names.find(b"\0", name_offset)
        section_name = names[name_offset:end].decode("ascii")
        if section_name == name:
            data_offset, data_size = struct.unpack_from(">II", data, header + 0x10)
            return data[data_offset:data_offset + data_size]
    raise ValueError(f"{path} has no {name} section")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: generate_gcdisplay_assets.py GCDISPLAY.O OUTPUT_DIR")

    input_path = Path(sys.argv[1])
    try:
        data = read_elf_section(input_path, ".data")
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
    if len(data) != 0x10200:
        raise SystemExit(
            f"expected 0x10200 bytes in {input_path} .data, got 0x{len(data):X}"
        )

    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)
    palette = [int.from_bytes(data[i:i + 2], "big") for i in range(0, 0x200, 2)]
    image = list(data[0x200:])
    write_values(output_dir / "gcdisplay_loading_palette.inc", palette, 4)
    write_values(output_dir / "gcdisplay_loading_image.inc", image, 2)


if __name__ == "__main__":
    main()
