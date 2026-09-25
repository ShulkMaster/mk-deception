#!/usr/bin/env python3
"""Exhaustively score call-argument staging variants in a permuter scratch.

    python3 tools/permuter_call_args.py SCRATCH_DIR [--joint] [--limit N] [--top N]

For every call in the scratch function, each argument is tried as kept,
folded from its staging local (``f(v op= e)``, ``f(e)``, ``f(v op e)``,
``f((e0) op e1)``) or hoisted into a new local, and every combination across
the call's N arguments is compiled and scored with the permuter's own
compiler and scorer. ``--joint`` also combines sites with each other (capped
by ``--limit``). Transforms are defined in
``tools/permuter_plugins/call_arg_staging.py``; the same transforms run as the
``perm_call_arg_staging`` random pass under ``tools/permuter_mkd.py``.

Results are written to ``SCRATCH_DIR/output-callargs-SCORE-N/source.c``.
Treat every result as a hypothesis: land only the honest source form in
``src/`` and remeasure with objdiff in the real TU.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from decomp_permuter import find_permuter  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("scratch", type=Path)
    parser.add_argument("--joint", action="store_true",
                        help="combine staging choices across call sites")
    parser.add_argument("--limit", type=int, default=4096,
                        help="maximum number of variants to compile")
    parser.add_argument("--top", type=int, default=10,
                        help="number of best variants to print and save")
    parser.add_argument("--permuter")
    args = parser.parse_args()

    import_script, _ = find_permuter(args.permuter)
    sys.path.insert(0, str(import_script.parent))

    import toml
    from src import ast_util
    from src.compiler import Compiler
    from src.preprocess import preprocess
    from src.scorer import Scorer
    from permuter_plugins import call_arg_staging

    scratch = args.scratch.resolve()
    base_c = scratch / "base.c"
    text = base_c.read_text(encoding="utf-8")
    if "PERM_" in text:
        raise SystemExit("base.c contains PERM_ macros; enumerate on a plain scratch")
    settings = toml.load(scratch / "settings.toml") if (scratch / "settings.toml").is_file() else {}
    fn_name = settings.get("func_name") or (scratch / "function.txt").read_text().strip()

    compiler = Compiler(str(scratch / "compile.sh"), show_errors=False, debug_mode=False)
    scorer = Scorer(str(scratch / "target.o"), stack_differences=False,
                    algorithm="difflib", debug_mode=False,
                    ign_branch_targets=False,
                    objdump_command=settings.get("objdump_command") or None)

    def score(source: str) -> int | None:
        o_file = compiler.compile(source)
        if o_file is None:
            return None
        try:
            return scorer.score(o_file)[0]
        finally:
            Path(o_file).unlink(missing_ok=True)

    ast = ast_util.parse_c(preprocess(str(base_c)))
    fn, _ = ast_util.extract_fn(ast, fn_name)
    ast_util.normalize_ast(fn, ast)
    base_score = score(ast_util.to_c(ast))
    print(f"{fn_name}: base score = {base_score}")
    for site in call_arg_staging.analyze(fn, ast):
        counts = "x".join(str(len(o)) for o in site.options)
        print(f"  site {site.name}(): options per argument {counts}")

    results = []
    failed = 0
    for n, (label, variant) in enumerate(
        call_arg_staging.variants(ast, fn_name, joint=args.joint, limit=args.limit)
    ):
        source = ast_util.to_c(variant)
        value = score(source)
        if value is None:
            failed += 1
            continue
        results.append((value, n, label, source))

    results.sort(key=lambda r: (r[0], r[1]))
    print(f"compiled {len(results)} variants ({failed} failed)")
    for rank, (value, n, label, source) in enumerate(results[: args.top], 1):
        marker = " *" if base_score is not None and value < base_score else ""
        print(f"  {value:6d}{marker}  {label}")
        out = scratch / f"output-callargs-{value}-{rank}"
        out.mkdir(exist_ok=True)
        (out / "source.c").write_text(source, encoding="utf-8")
        (out / "variant.txt").write_text(label + "\n", encoding="utf-8")
    if results and results[0][0] == 0:
        print("score 0 found; verify the honest form in the real TU with objdiff")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
