# MK Deception agent guide

This file is the operational entry point for coding agents working in this
repository. The supported target is the USA GameCube release, `GQNE5D`.

## Repository rules

- Preserve unrelated worktree changes. Inspect `git status --short` before and
  after editing.
- Treat retail assembly, call sites, symbols, relocations, and object layout as
  evidence. Decompiler output is a hypothesis, not ground truth.
- Edit source under `src/`, declarations under `include/`, and project metadata
  only when the evidence requires it. Do not hand-edit generated files in
  `build/`.
- Keep matching source readable and structurally honest. Do not force registers
  with `register`, fake `volatile`, dead sinks, incorrect prototypes, invented
  fields, embedded assembly, or unstructured `goto`.
- Exception: adding a function to the assembly-sequence mechanism is an
  extraordinarily rare action and requires explicit user permission for that
  specific function. Proof that a function is genuine handwritten assembly is
  necessary but does not itself grant permission. With approval, the function
  may invoke a `SEQ_<function>()` macro generated under `build/` from that
  version's retail-derived assembly and may be added to
  `config/<version>/asm_sequences.json`. Do not commit instruction payloads,
  synthesize a fallback, or use this path for ordinary compiler-generated
  functions. Automated, unattended, or goal-driven matching work must skip a
  function once evidence shows that it requires assembly; it must not add an
  assembly sequence or seek to satisfy the goal through one without explicit
  user permission.
- Make one coherent matching change at a time, rebuild, and inspect the same
  objdiff mismatch before trying another change.
- Preserve or explicitly account for the final retail SHA-1 check. A fuzzy
  percentage alone is not validation.

## Initialize the repository

Python is the only hard prerequisite for the bootstrap. Git and Ninja should be
available in `PATH`; missing host tools are reported in the final checklist.

With an extracted disc tree at `orig/GQNE5D`:

```sh
python3 tools/init.py
```

To validate a raw retail image explicitly:

```sh
python3 tools/init.py --iso "/path/to/Mortal Kombat - Deception.iso"
```

The initializer validates the `GQNE5D` ISO SHA-1, size, game ID, and embedded
`main.dol`; initializes submodules when present; installs or updates m2c; fetches
the pinned compiler and matching tools into `build/`; runs `configure.py`; and
performs the full Ninja build.

Important: `--iso` validates an image but does not currently extract it. DTK's
split step still needs the matching extracted tree under `orig/GQNE5D`. DTK can
perform extraction manually. On a fresh checkout, the first initializer run may
end in `NOT READY` after downloading DTK; extract the image and rerun it:

```sh
build/tools/dtk disc extract "/path/to/game.iso" orig/GQNE5D
python3 tools/init.py --iso "/path/to/game.iso"
```

On native Windows, use `python` (or `py -3`) and
`build\tools\dtk.exe` with an appropriately quoted Windows ISO path.

Do not continue matching unless setup ends in `READY` and the build prints:

```text
build/GQNE5D/main.dol: OK
```

## Use the decomp books

Read [the conventions book](docs/decomp/conventiond.md) before reconstructing a
function. It describes high-level source shapes observed in game code, SDK code,
and bundled libraries. Use it to form a hypothesis, then confirm that hypothesis
against retail evidence.

For a localized mismatch, use the mechanical playbooks in this order:

1. [High occurrence](docs/decomp/playbook-high-occurrence.md) — common type,
   ABI, layout, lifetime, CFG, and register-coloring causes. Start here.
2. [Mid occurrence](docs/decomp/playbook-mid-occurrence.md) — localized
   lowering, aggregate, ownership, scheduling, and compiler-mode causes.
3. [Niche / fallback](docs/decomp/playbook-niche.md) — rare compiler quirks and
   explicit stop conditions. Use only when the first two books do not fit.

Each row is an `If A -> then B` diagnostic. Apply it only when its preconditions
match the assembly and call-site evidence. Try one mechanical edit, rebuild, and
measure. Never stack speculative tricks merely because one improves fuzzy score.
If only harmless register coloring remains, follow the niche book's soft-ceiling
rule and stop.

## Recover a function with m2c

Find the function and its unit in `config/GQNE5D/symbols.txt`, `objdiff.json`, or
the generated assembly under `build/GQNE5D/asm/`. Then run:

```sh
python3 tools/m2c_decompile.py SYMBOL build/GQNE5D/asm/UNIT.s
```

Useful variants:

```sh
python3 tools/m2c_decompile.py --c++ SYMBOL build/GQNE5D/asm/UNIT.s
python3 tools/m2c_decompile.py --stack-structs SYMBOL build/GQNE5D/asm/UNIT.s
python3 tools/m2c_decompile.py --context build/GQNE5D/src/UNIT.ctx SYMBOL build/GQNE5D/asm/UNIT.s
```

Use m2c to recover control flow, operations, and an initial type hypothesis.
Replace generated temporaries, unknown types, casts, and gotos with supported
project types and structured C. Check every call and store order against the
retail assembly before treating the reconstruction as source.

## Permute a localized near match

Use local [decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
only after the algorithm, CFG, ABI, types, and layout agree with retail evidence
and objdiff classifies the function as a near miss. It complements the ranked
playbooks for localized scheduling, stack, and register-allocation differences;
it does not replace m2c, reconstruction, or playbook diagnosis.

Install the external checkout outside version control:

```sh
git clone https://github.com/simonlindholm/decomp-permuter.git build/decomp-permuter
python3 -m pip install toml
```

`tools/decomp_permuter.py` also accepts `--permuter /path/to/checkout` or the
`DECOMP_PERMUTER_PATH` environment variable. It maps the assembly unit through
`objdiff.json`, builds its generated context, extracts only the requested retail
function, recovers the exact Ninja/MWCC command, and creates an isolated scratch
under `.scratches/permuter/nonmatchings/`.

Prepare a local scratch with the same symbol-plus-assembly shape as m2c:

```sh
python3 tools/decomp_permuter.py SYMBOL build/GQNE5D/asm/UNIT.s
```

Run it immediately with four local workers and stop on score zero:

```sh
python3 tools/decomp_permuter.py SYMBOL build/GQNE5D/asm/UNIT.s --run -- -j 4 --stop-on-zero
```

Without `--run`, the wrapper prints the upstream command for the prepared
scratch. Edit only that scratch's `base.c` when adding `PERM_GENERAL`,
`PERM_LINESWAP`, or `PERM_RANDOMIZE`; never put `PERM_*` macros in `src/`.
Random mode is most useful for a clean near miss. Manual macros are appropriate
when two or more evidence-backed source forms interact and would be tedious to
enumerate.

Treat every generated candidate as a hypothesis. Reject undefined behavior,
fake `volatile`, invented lifetimes, incorrect types, or reordered side effects.
Apply only one understandable candidate insight to `src/`, rebuild the affected
object, and inspect the same symbol with objdiff. A permuter score of zero still
requires an honest-source review, the full build, and the retail SHA-1 gate. If
only harmless coloring remains, keep the niche playbook's soft ceiling instead
of landing permutation residue.

## Build and inspect the diff

After each coherent source edit, build the affected object when its Ninja path
is known:

```sh
ninja build/GQNE5D/src/UNIT.o
```

Run the full build before declaring completion:

```sh
ninja
```

Use the unit name recorded in `objdiff.json` to compare a function:

```sh
build/tools/objdiff-cli diff -p . -u main/UNIT SYMBOL -o - --format json-pretty
```

If the unit name is uncertain, search it rather than guessing:

```sh
rg -n '"name": "main/.*UNIT|"source_path": ".*UNIT' objdiff.json
```

Interpret the diff structurally:

- Wrong branches, calls, or large instruction islands: recover the algorithm or
  CFG before tuning declarations.
- Wrong load/store widths or offsets: fix types, signedness, or layout.
- Wrong argument registers: inspect callers and correct the prototype/order.
- Same operations with different nonvolatile registers: check honest lifetimes
  and declaration scope, then stop if only coloring remains.

## Other matching tools

### DTK

DTK validates and extracts images, splits the retail DOL, produces assembly, and
supports low-level DOL inspection:

```sh
build/tools/dtk disc info "/path/to/game.iso"
build/tools/dtk disc verify "/path/to/game.iso"
build/tools/dtk disc extract "/path/to/game.iso" orig/GQNE5D
build/tools/dtk dol info orig/GQNE5D/sys/main.dol
build/tools/dtk shasum -c config/GQNE5D/build.sha1
```

Normal split/report rules are generated by `configure.py` and run by Ninja. Do
not manually rewrite generated assembly or split outputs.

### CodeWarrior, sjiswrap, Wibo, and binutils

The initializer downloads the pinned CodeWarrior compiler bundle, `sjiswrap`,
GNU PowerPC binutils, and Wibo where required. Ninja selects them through the
generated build rules. Agents should change compiler flags only at the narrowest
supported object scope and must recheck every previously exact function in that
translation unit.

- `sjiswrap.exe` preserves the compiler's expected Shift-JIS input behavior.
- Wibo runs the Windows CodeWarrior executables on supported Unix hosts; native
  Windows runs them directly.
- PowerPC binutils assemble handwritten or generated assembly inputs used by the
  project. They are not a substitute for matching CodeWarrior-generated C/C++.

### Project configuration and progress

Regenerate build metadata after changing `configure.py`, splits, symbols, or
tool configuration:

```sh
python3 configure.py
```

Print the current matching totals:

```sh
python3 configure.py progress
```

The detailed generated report is `build/GQNE5D/report.json`.

## Self-validation checklist

Before reporting a decompilation change complete:

```sh
ninja
build/tools/dtk shasum -c config/GQNE5D/build.sha1
python3 configure.py progress
git diff --check
git status --short
```

Use `build\tools\dtk.exe` and `python`/`py -3` for the equivalent checks on
native Windows.

Also record the affected symbol's objdiff result before and after the change.
Confirm that declarations, callers, function order, object classification, and
shared layouts remain consistent. Report compiler warnings, soft ceilings, and
unrelated pre-existing changes honestly.
