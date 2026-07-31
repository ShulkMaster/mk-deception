#!/usr/bin/env python3
"""Run m2c against decomp-toolkit assembly for one MKD function."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import List, Optional


ROOT = Path(__file__).resolve().parent.parent


def find_m2c(explicit: Optional[str]) -> List[str]:
    candidate = explicit or os.environ.get("M2C_PATH")
    if candidate:
        path = Path(candidate).expanduser()
        if path.is_dir():
            path = path / "m2c.py"
        return [sys.executable, str(path)] if path.suffix == ".py" else [str(path)]

    command = shutil.which("m2c")
    if command:
        return [command]

    local = ROOT / "build" / "m2c" / "m2c.py"
    if local.is_file():
        return [sys.executable, str(local)]

    raise SystemExit(
        "m2c not found: pass --m2c, set M2C_PATH, install the `m2c` command, "
        "or clone it at tools/m2c"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Decompile one function from dtk-generated MKD assembly with m2c"
    )
    parser.add_argument("function", help="symbol/function name")
    parser.add_argument(
        "asm",
        type=Path,
        help="dtk assembly file, for example build/GQNE01/asm/debug_file.s",
    )
    parser.add_argument(
        "--context",
        type=Path,
        action="append",
        default=[],
        help="preprocessed C context (repeatable)",
    )
    parser.add_argument("--m2c", help="m2c executable, m2c.py, or checkout directory")
    parser.add_argument(
        "--c++",
        dest="cpp",
        action="store_true",
        help="use the ppc-mwcc-c++ target instead of ppc-mwcc-c",
    )
    parser.add_argument(
        "--stack-structs",
        action="store_true",
        help="emit m2c's inferred stack layout template",
    )
    args, m2c_args = parser.parse_known_args()
    if m2c_args and m2c_args[0] == "--":
        m2c_args = m2c_args[1:]

    asm = args.asm if args.asm.is_absolute() else ROOT / args.asm
    if not asm.is_file():
        raise SystemExit(f"assembly file not found: {asm}")

    command = find_m2c(args.m2c)
    command += [
        "--target",
        "ppc-mwcc-c++" if args.cpp else "ppc-mwcc-c",
        "--function",
        args.function,
        "--deterministic-vars",
        "--unk-underscore",
    ]
    for context in args.context:
        path = context if context.is_absolute() else ROOT / context
        command += ["--context", str(path)]
    if args.stack_structs:
        command.append("--stack-structs")
    command += m2c_args
    command.append(str(asm))

    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
