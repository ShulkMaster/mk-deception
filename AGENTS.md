# MK Deception agent guide

This file is the operational entry point for coding agents working in this
repository. The supported target is the USA GameCube release, `GQNE5D`.

## Local agent docs

Local tooling and setup docs live in the git-ignored `.agents/` folder, next to
the repository skills (`.agents/skills/`, linked into `.claude/skills/`). Read
the relevant one before setup or tool use; if `.agents/` is missing, ask the
user.

- [.agents/docs/local-setup.md](.agents/docs/local-setup.md): bootstrap,
  toolchain, and the `.agent-work/` and `.scratches/` layout.
- [.agents/docs/decompstudio.md](.agents/docs/decompstudio.md): MCP
  coordinator connection, roles, m2c, permuter, and notes.

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
- Exception: a local `goto` is allowed as a last resort when all of these hold:
  - Structured alternatives (early return, `break`, flag, shared exit, helper
    inline) were measured and regress the match.
  - The shape has backing evidence, such as matched references of the same
    vendor code in other decomps (MSL `__dec2num` uses `goto done` in both
    bfbb and TP/dusk), or the `goto` gives simpler, more honest control flow
    than a contrived `do { ... } while (0)`, one-trip loop, or dummy flag that
    exists only to force a match.
  - The jump stays inside one function and targets a label in that same
    function. Prefer a forward jump to a shared exit or cleanup. Never jump into a nested
    block past initializations, never emulate a loop that `for`/`while`
    expresses, and never use `setjmp`/`longjmp` or computed gotos.

  Record the evidence (measured alternatives and the reference) in the task
  report, not in a function comment.
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
- Keep reusable conventions and diagnostic playbooks in tracked `docs/decomp/`;
  keep actual fixes in `src/`, `include/`, or the relevant project files.
  Ignored artifacts are local-only: tracked documentation must explain its
  reusable finding without requiring an ignored report. Promote supporting
  evidence deliberately when it must be available to other contributors.

## Post-attempt status policy

After every matching attempt (including a reverted trial or no-edit stop),
if the function remains below 100%, update one source comment immediately above
the affected function:

```c
/* TODO: [near miss] 98.84%; equivalent latch CFG remains; stop at coloring. */
```

Required format: `TODO: [status] quick explanation`. Canonical statuses:

- `borked`: algorithm, CFG, ABI, or layout is demonstrably wrong.
- `breakthrough needed`: unresolved structural cause; name missing evidence.
- `breakthrough`: structural cause fixed; name remaining mismatch/next check.
- `near miss`: behavior/structure agree; localized codegen/relocation residue.
- `blocked`: tool, input, or authorization prevents verification.

Describe retained source, not the rejected candidate. Include current objdiff
score when available and concrete residual/next action; use one or two lines.
Replace previous status instead of appending history. For shared edits, update
functions whose result/classification changes. Never infer an exact match from
fuzzy improvement. Comments do not replace whole-TU checks, full build, or retail
SHA-1.

Once a function reaches a measured 100%, remove all matching-progress TODOs and
comments associated with it, including old percentages, near-match notes, soft
ceilings, attempt history, and `TODO: [matched]` markers. Do not replace them with
a new 100% comment. Source keeps only tracking markers (the TODO status line,
`TODO: [Scope warn]`, file-top `BUILD:`/`TODO: [blocked]`/`TODO: [review]`).
Comments that explain behavior, algorithms, ABI, layout, or other code semantics
move to the shared notes store (see [Local agent docs](#local-agent-docs))
before they are deleted from source, so no knowledge is lost. Unrelated
functional TODOs remain.
Keep verification evidence and the distinction between report-exact, data-value
exact, and link-exact in reports and project metadata, not function comments.

## Use the decomp books

Read [the conventions book](docs/decomp/conventiond.md) before reconstructing a
function. It describes high-level source shapes observed in game code, SDK code,
and bundled libraries. Use it to form a hypothesis, then confirm that hypothesis
against retail evidence.

For a localized mismatch, use the mechanical playbooks. They are tiered by how
rare the situation is; read tier 1 every time and open a deeper tier only when
its triage table points there:

1. [Core](docs/decomp/playbook-1-core.md) — protocol, acceptance gates,
   symptom-to-rule triage, the H-rule index, and stop rules. Start here.
2. [Common](docs/decomp/playbook-2-common.md) — detail for H01-H27: type, ABI,
   layout, lifetime, CFG, and register-coloring causes.
3. [Uncommon](docs/decomp/playbook-3-uncommon.md) — M01-M17: compiler modes,
   FP, aggregate/ABI shape, inline/macro boundaries, and data/link layout.
4. [Rare](docs/decomp/playbook-4-rare.md) — N01-N13, hard stops, compiler
   revisions, and permuter search.

Each rule has its own `## ID` section in tiers 2-4, so one rule can be read
alone. Rule IDs are stable across tiers. Keep the tiers compact: amend the
existing rule with its precondition, action, and one exemplar symbol; put
attempt history, scores, and campaign evidence in linked reports. Preserve
distinct preconditions, safety/stop rules, and rule IDs when summarizing.

Each rule is an `IF / REQUIRE / TRY` diagnostic. Apply it only when its preconditions
match the assembly and call-site evidence. Try one mechanical edit, rebuild, and
measure. Never stack speculative tricks merely because one improves fuzzy score.
If only harmless register coloring remains, follow the tier-4 hard-stop
(soft-ceiling) rule and stop.

## Recover a function with m2c

Run m2c on the retail object with the unit's source as type context (tooling:
[Local agent docs](#local-agent-docs)). Find the function and its unit in
`config/GQNE5D/symbols.txt` or `objdiff.json` when the symbol is uncertain.

Use m2c to recover control flow, operations, and an initial type hypothesis.
Replace generated temporaries, unknown types, casts, and gotos with supported
project types and structured C. Keep a `goto` only under the last-resort
exception in the repository rules. Check inferred union members against retail
offsets, and check every call and store order against the retail assembly before
treating the reconstruction as source.

## Permute a localized near match

Use the permuter (tooling: [Local agent docs](#local-agent-docs)) only after
the algorithm, CFG, ABI, types, and layout agree with retail evidence and
objdiff classifies the function as a near miss. It complements the ranked
playbooks for localized scheduling, stack, and register-allocation differences;
it does not replace m2c, reconstruction, or playbook diagnosis.

Treat every generated candidate as a hypothesis. Reject undefined behavior,
fake `volatile`, invented lifetimes, incorrect types, or reordered side effects.
Apply only one understandable candidate insight to `src/`, rebuild the affected
object, and inspect the same symbol with objdiff. A permuter score of zero still
requires an honest-source review, the full build, and the retail SHA-1 gate. If
only harmless coloring remains, keep the tier-4 hard-stop soft ceiling instead
of landing permutation residue.

## Build and inspect the diff

Do not start matching unless the full build prints
`build/GQNE5D/main.dol: OK`.

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

### Generated outputs and compiler flags

Normal split/report rules are generated by `configure.py` and run by Ninja. Do
not manually rewrite generated assembly or split outputs.

Change compiler flags only at the narrowest supported object scope and recheck
every previously exact function in that translation unit.

### Deferred-inline order

`python3 tools/deferred_scan.py` lists units whose retail parse-time symbols
number backwards through `.text`, which is the `-inline deferred` signature.
`python3 tools/reverse_deferred_tu.py IN.c OUT.c` writes a reversed-definition
candidate for a scratch compile. Follow the playbook's M15 deferred-order
addendum and land it only when the whole unit gains.

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

Also record the affected symbol's objdiff result before and after the change.
Confirm that declarations, callers, function order, object classification, and
shared layouts remain consistent. Report compiler warnings, soft ceilings, and
unrelated pre-existing changes honestly.
