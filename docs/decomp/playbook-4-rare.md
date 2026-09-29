# Matching playbook, tier 4: rare and stop

Rare diagnostics, stop conditions, and search tooling. Use this tier after
tiers [1](playbook-1-core.md)-[3](playbook-3-uncommon.md). These rules are not
default optimizer knobs.

## N01

A Boolean/rotate idiom survives honest C. REQUIRE exact intrinsic semantics
and an unsigned contract. Use the proven `cntlzw`/rotate intrinsic, not
instruction-count padding.

## N02

Global address vs contents/SDA differs. REQUIRE ELF object/array identity and
all uses. Correct the object, array, or pointer declaration; no fake array to
suppress SDA (M16).

## N03

Repeated RTTI/null ladder. REQUIRE class identity, no call, null paths. Inline
the typed ladder; no guessed class or layout.

## N04

Factory cases return distinct owners. REQUIRE allocation/failure semantics and
case order. Direct typed returns, not an artificial shared result.

## N05

Reverse iteration or update differs. REQUIRE direction, stride, and
zero-count semantics. A typed reverse-index walk or the vendor pre-decrement
idiom; no manual byte offsets.

## N06

Peephole, CSE, or scheduler residue. REQUIRE a tested TU hypothesis. Test the
exact option in scratch; no per-function exceptions during fixed-TU-setting
work, and no flags that mask wrong source.

Compiler-revision hypothesis: keep the recovered command, use separate scratch
outputs with a pinned-revision control, and compare text bytes plus every
function/data result. Byte-identical output rules out those revisions for that
source and command only. Keep the pinned compiler.

## N07

Runtime scratch faults after object growth. REQUIRE section extents, relocated
instruction bytes, and non-overlapping mapped ranges. Fix the loader's section
placement or slot bounds before changing source; never skip a faulting
instruction to claim equivalence.

## N08

m2c exposes a possibly unwritten local that retail consumes. REQUIRE
instruction-level write/read paths, frame offsets, caller reachability. Vary
only the incoming stack word in a retail runtime scratch and inspect the
downstream result. Stack dependence is not proof of reachable gameplay. Record
the unresolved path; add an initializer only with evidence.

## N09

Retail keeps an explicit byte-swap sequence where the compiler folds the typed
access to `lhbrx`/`stwbrx`. REQUIRE identical CFG, endian result, width,
destination. Stop at the clean shift expression; no donor dead conditional or
fake read to block the fold (`SFH_AnlyMaxFrmNum`).

## N10

Retail classifies a float through its IEEE-754 word with `lwz`. REQUIRE
four-byte float/word sizes and a same-source donor's word idiom. Test the
donor's narrow word view under MWCC, with a `memcpy` fallback for other
compilers; a plain `memcpy` under MWCC changes the frame
(`ADX_GetCoefficient`). No arbitrary alias casts or union overlays.

## N11

C after an `asm` function loses peephole forms (`mr` stays `addi rX,rY,0`,
shifts fold into `rlwimi`, a compare moves), starting at the first `asm`
function above it. An `asm` function turns the peephole pass off for the rest of
the file. If retail wrote C with an inline `asm {}` helper (GX `Copy6Floats`,
`WriteMTXPS*`), wrap the run of sequence functions in `#pragma push` ...
`#pragma pop`. If retail had real `asm` functions (OSCache, OSExec, OSTime,
ai), the leak is authentic: remove old `#pragma peephole off` or
`-opt nopeephole` workarounds.

## N12

An assembly sequence fails with `internal compiler error: File:
'PCodeAssembly.c' Line: 510` when a `lwz`/`stw` has an `@l` offset into a code
symbol (function or `entry` label). MWCC 1.2.5n asm cannot emit that form in
any spelling; retail most likely did the access in C. Keep the function on the
C path and record it as blocked. No `opword`: the relocation would be lost.

## Hard stops

After the applicable honest source check, stop at:

- GPR/FPR coloring, parameter nonvolatile homes, or permutations rotating the
  residue.
- `li 0` vs copying an already-zero register; commutative scratch encodings.
- Frameless PLATFORM `mtlr`/`blrl` emission.
- Anonymous relocation labels with verified identical payloads and targets.
  Equal ordinary scores are not enough (wrong return constants have scored
  equal); compare bytes in data-value mode. Inferred `R_PPC_NONE` annotations
  on identical instructions are a metadata residual; record them without
  rewriting source. This is not report-exact or link-exact.
- Equivalent branch/address lowering without new source evidence.
- Vendor code whose matched references use a shared-exit `goto` (MSL
  `__dec2num`: `goto done`; `__str2dec`: `goto round`) where flag-and-break
  emulations leave extra instructions: use the AGENTS.md last-resort `goto`
  exception and record the measured alternatives.
- One instruction scheduled across a store when the body is textually
  identical to a matched decomp of the same SDK (`SPEC2_MakeStatus` vs TP), and
  object-scope flag probes were neutral.
- A permuter candidate that only recomputes an unchanged value to split a live
  range (`SJRBF_PutChunk`); record the insight, do not land it.

Then record `TODO: [near miss]` with score, residual, and stop reason, and
disclose nonmatching fallbacks in SHA results. Reopen a ceiling only for new
evidence: a proven owner (the GX FIFO union) justified returning to H02,
another spelling of the same lifetime did not.

## Permuter and search

The permuter requires an established algorithm, CFG, ABI, layout, and the real
TU command. Keep `PERM_*` in scratch. Reject UB, wrong types, reordered
effects, and fake liveness even at score zero; land one honest insight, then
verify in the real TU.

- Scores: about 5 per register-name difference, 100 per inserted or deleted
  instruction. A low nonzero score can hide wrong colors; a score of 5 may add
  no code. Diff the candidate's objdump against `target.o`. Use
  `--stack-diffs` when stack operands differ (default scoring ignored reversed
  +0x08/+0x0C stores in `run_camera_script`).
- Base score far above the real residue: retail inlined same-TU callees, and
  import strips non-`inline` bodies. Restore those callees as `inline` in the
  scratch (`SFD_SetCond`) and confirm the base score first.
- A mismatch inside an expanded private helper: selecting only the caller
  leaves helper bodies unmutated; build a harness that mutates the helper while
  scoring the caller, and check that no helper call appears. Expansion and
  declaration order can interact (both needed for the AI table builders);
  verify the combination in each wrapper. If expansion reverses stack-local
  placement, reverse only the scratch declarations.
- Several nonzero candidates sharing one idea: express it without `new_var`,
  `if (1)`, or comma operators and try it in the real TU; five candidates
  feeding `(x *= k)` to a call led to the H15 scaled-argument form. Call-argument
  mode enumerates keep/fold/fold-value/hoist combinations directly and never
  moves across an observable call or store.
- Candidate behaves differently in the real TU: compile it inside a frozen full
  TU with the recovered command, restrict objdump to the symbol
  (`--disassemble=SYMBOL`), and confirm the baseline with objdiff.
- Missing-function KeyError during extraction: make the canonical declaration
  visible before the call (an implicit-int call is absent from the type map),
  then reimport.
- Reimport: use the newly printed scratch path (possibly `SYMBOL-2`) and verify
  its base source. A one-line `PERM_LINESWAP` can fold away; do not claim
  exhaustive search.
- Reject offsets on both sides of a signed comparison unless overflow is
  impossible: `(x + 1) < 1` is not `x < 0` on arbitrary 32-bit input.
- Freeze type/cast passes when invalid truncations dominate. A widened temporary
  narrowed at every use needs independent width evidence.
- Declaration reordering (`perm_reorder_decls`) moves declarations across
  blocks; for a pure ordering search restrict it to uninitialized declarations
  in one block (the local same-block patch, `MKD_PERM_SAME_BLOCK_DECLS=1`).
  Compile failures do not exhaust valid orderings.
- Integer Boolean values outside conditions: the local patch
  `MKD_PERM_INTEGER_BOOLEAN_VALUES=1` with only `perm_condition` enabled
  rewrites `x == 0`/`!x` in values and call arguments, for integer operands only.
- Scratch parsing can change `numNodes * sizeof(RwMatrix) + 15`; parenthesize
  the product in scratch only. "PERM macro in AST" from an inline-helper
  `PERM_LINESWAP`: use finite text-level `PERM_GENERAL`.
- A CSE candidate that repeats a table expression: both values must feed real
  outputs with no intervening store or call; verify load count, every macro
  consumer, runtime behavior, and linked SHA.
- A zero score that needs an otherwise unused alias is rejected
  (`ani_to_frame_x`).
- Mirage imports need platform-neutral behavior plus GC retail and objdiff
  evidence.
