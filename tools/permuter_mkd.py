#!/usr/bin/env python3
"""Run decomp-permuter with MKD plugin passes and playbook-tuned weights.

    python3 tools/permuter_mkd.py [--profile playbook|upstream] SCRATCH_DIR [permuter args]

Plugin passes (tools/permuter_plugins) are registered in-process, so the
external checkout stays unmodified. Workers are forced to use ``fork`` so
they inherit the registration; Python 3.14 otherwise defaults to forkserver on
Linux and would re-import an unpatched randomizer.

Profiles set default pass weights; a scratch's ``settings.toml``
``[weight_overrides]`` still takes precedence.

* ``playbook`` (default) favors transforms the MKD playbooks treat as honest
  source shapes: store/statement order and declaration order (H14/H15),
  argument staging and expression folding (H15 scaled-argument addendum),
  commutative operands (M03), and typed inline helpers (M13). It disables
  passes whose output the playbooks reject outright: dead sinks, zero masks,
  comma padding, `if (1)` blocks, self-assignments, padding declarations, and
  AST removal.
* ``upstream`` keeps decomp-permuter's MWCC weights and only adds the plugin
  passes.
"""

from __future__ import annotations

import argparse
import multiprocessing
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from decomp_permuter import find_permuter  # noqa: E402

PLUGIN_WEIGHTS = {"perm_call_arg_staging": 60.0}

PLAYBOOK_WEIGHTS = {
    # Honest restaging and ordering (H14/H15).
    "perm_call_arg_staging": 60.0,
    "perm_temp_for_expr": 40.0,
    "perm_expand_expr": 40.0,
    "perm_reorder_stmts": 30.0,
    "perm_reorder_decls": 20.0,
    "perm_compound_assignment": 10.0,
    "perm_split_assignment": 15.0,
    "perm_chain_assignment": 10.0,
    "perm_refer_to_var": 10.0,
    "perm_ins_block": 5.0,
    # Operand order and typed owners (M03, H15 typed-owner, M13).
    "perm_commutative": 15.0,
    "perm_add_sub": 3.0,
    "perm_struct_ref": 5.0,
    "perm_inline": 10.0,
    "perm_cast_simple": 5.0,
    "perm_condition": 5.0,
    "perm_inequalities": 3.0,
    "perm_factor_mult": 2.0,
    "perm_factor_shift": 2.0,
    # Rarely honest: types/literals must come from evidence, not search.
    "perm_randomize_internal_type": 1.0,
    "perm_randomize_external_type": 0.5,
    "perm_randomize_function_type": 0.5,
    "perm_float_literal": 1.0,
    "perm_long_chain_assignment": 1.0,
    "perm_sameline": 0.5,
    "perm_duplicate_assignment": 0.5,
    "perm_add_mask": 0.5,
    # Rejected by the playbooks: dead sinks, fake liveness, padding.
    "perm_xor_zero": 0.0,
    "perm_mult_zero": 0.0,
    "perm_dummy_comma_expr": 0.0,
    "perm_add_self_assignment": 0.0,
    "perm_empty_stmt": 0.0,
    "perm_pad_var_decl": 0.0,
    "perm_var_cond_block": 0.0,
    "perm_remove_ast": 0.0,
    "perm_alias_array": 0.0,
}


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--profile", choices=("playbook", "upstream"), default="playbook")
    parser.add_argument("--permuter")
    args, rest = parser.parse_known_args()

    import_script, _ = find_permuter(args.permuter)
    checkout = import_script.parent
    sys.path.insert(0, str(checkout))
    multiprocessing.set_start_method("fork", force=True)

    import src.main as permuter_main
    from src import randomizer
    from permuter_plugins import call_arg_staging

    for method in call_arg_staging.PASSES:
        if method not in randomizer.RANDOMIZATION_PASSES:
            randomizer.RANDOMIZATION_PASSES.append(method)

    profile = PLAYBOOK_WEIGHTS if args.profile == "playbook" else PLUGIN_WEIGHTS
    upstream_defaults = permuter_main.get_default_randomization_weights

    def defaults_with_profile(compiler_type: str):
        weights = dict(upstream_defaults(compiler_type))
        weights.update(profile)
        return weights

    permuter_main.get_default_randomization_weights = defaults_with_profile
    sys.argv = [str(checkout / "permuter.py")] + rest
    permuter_main.main()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
