#!/usr/bin/env python3
"""Prepare and optionally run decomp-permuter for one MKD function."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_WORK_DIR = ROOT / ".scratches" / "permuter"


def resolve_repo_file(path: Path) -> Path:
    return path if path.is_absolute() else ROOT / path


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def find_permuter(explicit: str | None) -> tuple[Path, Path]:
    candidate = explicit or os.environ.get("DECOMP_PERMUTER_PATH")
    paths: list[Path] = []
    if candidate:
        paths.append(Path(candidate).expanduser())
    paths.extend(
        [
            ROOT / "build" / "decomp-permuter",
            ROOT.parent / "decomp-permuter",
        ]
    )

    for path in paths:
        import_script = path if path.name == "import.py" else path / "import.py"
        permuter_script = import_script.parent / "permuter.py"
        if import_script.is_file() and permuter_script.is_file():
            return import_script.resolve(), permuter_script.resolve()

    raise SystemExit(
        "decomp-permuter not found: pass --permuter, set DECOMP_PERMUTER_PATH, "
        "or clone it to build/decomp-permuter"
    )


def load_unit(asm: Path) -> dict[str, Any]:
    objdiff = ROOT / "objdiff.json"
    if not objdiff.is_file():
        raise SystemExit("objdiff.json is missing; run `python3 configure.py` first")

    data = json.loads(objdiff.read_text(encoding="utf-8"))
    asm_root = (ROOT / "build" / "GQNE5D" / "asm").resolve()
    try:
        rel = asm.resolve().relative_to(asm_root).with_suffix(".o")
    except ValueError as error:
        raise SystemExit(f"assembly must be under {asm_root}: {asm}") from error

    expected_target = (ROOT / "build" / "GQNE5D" / "obj" / rel).resolve()
    matches = [
        unit
        for unit in data.get("units", [])
        if unit.get("target_path")
        and resolve_repo_file(Path(unit["target_path"])).resolve() == expected_target
    ]
    if len(matches) != 1:
        raise SystemExit(
            f"could not uniquely map {asm.relative_to(ROOT)} through objdiff.json"
        )

    unit = matches[0]
    required = ["base_path", "scratch", "metadata"]
    if any(key not in unit for key in required):
        raise SystemExit(f"objdiff unit {unit.get('name', rel.stem)} is not buildable")
    if not unit["scratch"].get("ctx_path") or not unit["metadata"].get("source_path"):
        raise SystemExit(f"objdiff unit {unit['name']} has no generated context/source")
    return unit


def extract_function(asm: Path, symbol: str) -> str:
    lines = asm.read_text(encoding="utf-8").splitlines(keepends=True)
    start: int | None = None
    end: int | None = None
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith(".fn "):
            name = stripped[4:].split(",", 1)[0].strip()
            if name == symbol:
                start = index
                break
    if start is None:
        raise SystemExit(f"function {symbol!r} not found in {asm.relative_to(ROOT)}")

    for index in range(start + 1, len(lines)):
        if lines[index].strip() == f".endfn {symbol}":
            end = index + 1
            break
    if end is None:
        raise SystemExit(f"missing `.endfn {symbol}` in {asm.relative_to(ROOT)}")

    # decomp-permuter's importer recognizes .globl, while DTK spells function
    # boundaries with its .fn macro. Keep the retail body untouched and add the
    # recognized declaration solely to identify this one-function input.
    return (
        '.include "macros.inc"\n'
        ".text\n"
        ".balign 4\n"
        f".globl {symbol}\n"
        + "".join(lines[start:end])
    )


def compiler_command(unit: dict[str, Any]) -> list[str]:
    source = unit["metadata"]["source_path"]
    base = unit["base_path"]
    output = subprocess.check_output(
        ["ninja", "-t", "commands", base], cwd=ROOT, text=True
    )
    candidates = [line for line in output.splitlines() if source in line]
    if len(candidates) != 1:
        raise SystemExit(
            f"could not uniquely recover the Ninja compile command for {source}"
        )

    parts = shlex.split(candidates[0])
    if "&&" in parts:
        parts = parts[: parts.index("&&")]

    cleaned: list[str] = []
    skip = False
    for part in parts:
        if skip:
            skip = False
            continue
        if part == source or part == "-MMD":
            continue
        if part == "-o":
            skip = True
            continue
        cleaned.append(part)

    if not cleaned or "-c" not in cleaned:
        raise SystemExit(f"unexpected Ninja compile command for {source}")
    return cleaned


def quote_toml(value: str) -> str:
    # JSON strings are valid TOML basic strings and handle paths/quotes safely.
    return json.dumps(value)


def write_settings(
    path: Path, command: list[str], empty_prelude: Path
) -> None:
    assembler = ROOT / "build" / "binutils" / "powerpc-eabi-as"
    objdump = ROOT / "build" / "binutils" / "powerpc-eabi-objdump"
    macros = ROOT / "build" / "GQNE5D" / "include"
    for tool in (assembler, objdump):
        if not tool.is_file():
            raise SystemExit(f"missing matching tool: {tool.relative_to(ROOT)}")

    text = "\n".join(
        [
            'compiler_type = "mwcc"',
            f"compiler_command = {quote_toml(shlex.join(command))}",
            "assembler_command = "
            + quote_toml(
                shlex.join([str(assembler), "-mgekko", "-I", str(macros)])
            ),
            f"asm_prelude_file = {quote_toml(str(empty_prelude))}",
            "objdump_command = "
            + quote_toml(
                shlex.join(
                    [str(objdump), "-dr", "-EB", "-mpowerpc", "-M", "broadway"]
                )
            ),
            "",
        ]
    )
    path.write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare one MKD function for decomp-permuter"
    )
    parser.add_argument("function", help="symbol/function name")
    parser.add_argument(
        "asm",
        type=Path,
        help="DTK assembly unit, for example build/GQNE5D/asm/debug_file.s",
    )
    parser.add_argument(
        "--permuter", help="decomp-permuter checkout directory or import.py path"
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=DEFAULT_WORK_DIR,
        help="scratch parent (default: .scratches/permuter)",
    )
    parser.add_argument(
        "--run", action="store_true", help="run the local permuter after importing"
    )
    args, permuter_args = parser.parse_known_args()
    if permuter_args and permuter_args[0] == "--":
        permuter_args = permuter_args[1:]
    if permuter_args and not args.run:
        raise SystemExit("permuter arguments require --run")

    asm = resolve_repo_file(args.asm).resolve()
    if not asm.is_file():
        raise SystemExit(f"assembly file not found: {asm}")
    import_script, permuter_script = find_permuter(args.permuter)
    unit = load_unit(asm)

    ctx = resolve_repo_file(Path(unit["scratch"]["ctx_path"])).resolve()
    subprocess.run(["ninja", str(ctx.relative_to(ROOT))], cwd=ROOT, check=True)
    command = compiler_command(unit)

    work_dir = resolve_repo_file(args.work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    nonmatchings = work_dir / "nonmatchings"
    before = set(nonmatchings.iterdir()) if nonmatchings.is_dir() else set()

    with tempfile.TemporaryDirectory(
        prefix="mkd-permuter-", dir=work_dir
    ) as temp_name:
        temp = Path(temp_name)
        target_asm = temp / f"{args.function}.s"
        settings = temp / "permuter_settings.toml"
        empty_prelude = temp / "empty.inc"
        target_asm.write_text(
            extract_function(asm, args.function), encoding="utf-8"
        )
        empty_prelude.write_text("", encoding="utf-8")
        write_settings(settings, command, empty_prelude)

        import_command = [
            sys.executable,
            str(import_script),
            str(ctx),
            str(target_asm),
            "--settings",
            str(settings),
        ]
        result = subprocess.run(import_command, cwd=work_dir, check=False)
        if result.returncode:
            return result.returncode

    after = set(nonmatchings.iterdir()) if nonmatchings.is_dir() else set()
    created = sorted(after - before)
    if len(created) != 1:
        raise SystemExit(
            "import succeeded but its new nonmatchings directory is ambiguous"
        )
    scratch_dir = created[0]
    print(f"MKD permuter scratch: {display_path(scratch_dir)}")

    if args.run:
        run_command = [sys.executable, str(permuter_script), str(scratch_dir)]
        run_command += permuter_args
        return subprocess.run(
            run_command, cwd=import_script.parent, check=False
        ).returncode
    print(
        "Run it with: "
        + shlex.join(
            [
                sys.executable,
                str(permuter_script),
                str(scratch_dir),
                "-j",
                "4",
                "--stop-on-zero",
            ]
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
