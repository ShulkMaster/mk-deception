# Matching rules: niche / stop

Prerequisite: [high protocol](playbook-high-occurrence.md#protocol-all-three-books),
then [mid](playbook-mid-occurrence.md). Schema: ID | IF | REQUIRE | TRY.
These are rare diagnostics, not default optimizer knobs.

N01 | Boolean/rotate idiom survives honest C | Exact intrinsic semantics + unsigned contract | Proven cntlzw/rotate intrinsic, not instruction-count padding.
N02 | Global address vs contents/SDA differs | ELF object/array identity + all uses | Correct object/array/pointer declaration; no fake array to suppress SDA.
N03 | Repeated RTTI/null ladder | Class identity + no call + null paths | Inline typed ladder; no guessed class/layout.
N04 | Factory cases return distinct owners | Allocation/failure semantics + case order | Direct typed returns, not an artificial shared result.
N05 | Reverse iteration/update differs | Direction/stride/zero-count semantics | Typed reverse-index walk or authentic vendor pre-decrement idiom; avoid manual byte offsets.
N06 | Peephole/CSE/scheduler residue | Tested TU hypothesis + historical local-exception evidence | Investigate exact option in scratch; no per-function exceptions during fixed-TU-setting work or flags masking wrong source.

For a compiler-revision hypothesis, preserve the recovered command and use
separate scratch outputs, including a pinned-revision control. Compare actual
text bytes and all function/data results; equal fuzzy scores alone are not
identity. Byte-identical output rules out those revisions for that source and
command, not for every TU or source form. Retain the pinned compiler and its
metadata evidence ([AI revision control](../../.agent-work/decomp/ai-matching/README.md#round-208-isolated-compiler-revision-control)).

N07 | Runtime scratch faults after object growth | Section extents, relocated instruction bytes and nonoverlapping mapped ranges | Size/alignment-aware section placement or checked slot bounds; repair the loader before changing source. Never skip a faulting instruction to claim equivalence. See [chess checkpoint 40](mk-chess-body-recovery.md).

N08 | m2c exposes a possibly unwritten local consumed by retail | Instruction-level write/read paths, ABI frame offsets, and caller/state reachability | Vary only the incoming stack word in a retail runtime scratch and inspect the downstream argument/result. Distinguish demonstrated stack dependence from proof of reachable gameplay. Recover a missing initializer only with evidence; otherwise record the unresolved path, not a zero initializer or undefined C. See [chess checkpoint 128](mk-chess-body-recovery.md).

## Hard stops

After the applicable honest source check, stop for:

- GPR/FPR coloring, parameter nonvolatile homes, or permutations rotating residue.
- li-zero vs copying an already-zero register; commutative scratch encodings.
- Frameless PLATFORM mtlr/blrl emission.
- Anonymous relocation labels with verified identical payloads/targets. Ordinary
  score equality is insufficient: mk_chess board callbacks scored identically
  for wrong 0.0f and correct -1.0f returns. Verify bytes/data-value comparison.
- Equivalent branch/address lowering without new source evidence.

Unknown calls/offsets/CFG are not coloring: classify borked or breakthrough needed
and name missing evidence. Assembly-required targets are skipped; AGENTS.md
requires explicit per-function permission for the rare sequence path.
Reopen a ceiling only for new evidence. The GX FIFO's proven absolute union
owner justified returning to H02 after failed snapshot trials; another spelling
of the same lifetime did not. See the measured ranking in the
[knowledge record](matching-knowledge.md#measured-rule-ranking).
Record source `TODO: [near miss]` with score, residual, and stop reason; disclose
nonmatching fallback in SHA results. Never omit returns or invent lifetimes,
types, empty arms, fake volatile, dead sinks, register declarations, or goto.

## Optional search

Permuter requires established algorithm/CFG/ABI/layout and the real TU command.
Keep PERM_* in scratch; task-specific attempt limits never waive verification.
Reject UB, wrong types, reordered effects, and fake liveness even at zero score.

- IF a known integer `x == 0` / `!x` value rewrite lies outside branch conditions,
  REQUIRE a verified type and mutation-region boundary; TRY the opt-in
  [integer Boolean patch](../../.agent-work/patches/decomp-permuter/decomp-permuter-integer-boolean-values.patch)
  after the existing same-block-declarations patch. Set
  `MKD_PERM_INTEGER_BOOLEAN_VALUES=1`, enable `perm_condition`, and disable other
  passes for a focused control. This mode visits expression values, including
  call arguments; upstream `perm_condition` visits only branch/loop conditions.
  It preserves one operand evaluation and the `int` Boolean result, rejects
  pointer/float operands, and does not turn `x != 0` into raw `x`. Review the
  emitted candidate and verify it in the real TU. Unset the environment variable
  to retain upstream behavior; see [AI calibration](../../.agent-work/decomp/ai-matching/README.md#round-216-integer-boolean-value-permuter-coverage).

- IF expression extraction raises a missing-function KeyError, REQUIRE the
  canonical callee definition and retail caller ABI; TRY making its real
  declaration visible in source, then reimport the scratch. An implicit-int
  call may compile but be absent from the permuter type map. Check declaration
  order too: finding a later definition does not establish visibility at the
  call. Prefer the existing canonical header and complete forward prototypes.
  Reproduce the
  old inference failure, verify the new result type and unchanged base score,
  and check every shared-header consumer ([AI direction predicate](../../.agent-work/decomp/ai-matching/README.md#round-149-canonical-direction-predicate-declaration)).
- IF stack operands differ, REQUIRE that the scorer sees them; TRY
  `--stack-diffs` for smoke test and search. Default scoring ignored reversed
  +0x08/+0x0C stores in run_camera_script and falsely scored zero.
- IF zero score needs an otherwise unused alias/lifetime, REQUIRE actual
  semantic ownership; reject the workaround when absent (ani_to_frame_x).
- IF importing again, REQUIRE the newly printed scratch path (possibly
  SYMBOL-2); verify its base source and iteration count. A one-line
  PERM_LINESWAP can fold away into randomization; do not claim exhaustive search.
- IF the mismatch is inside an expanded private helper, REQUIRE confirmation
  that the mutation region includes that helper body. Selecting the caller alone
  leaves helper bodies outside ordinary randomization. TRY an isolated harness
  that mutates the helper while retaining the caller body for emitted-code
  scoring; verify the unchanged baseline score and that no helper call appears.
  Selecting only the helper in settings can otherwise strip the caller body.
  Inspect the complete candidate even with PERM_RANDOMIZE regions: some passes
  can add control constructs outside the requested region.
- IF a candidate offsets both sides of a signed comparison, REQUIRE proof that
  the offset cannot overflow across the complete input domain. Reject
  `(signed_bits + 1) < 1` and `(signed_bits - 1) < -1` as replacements for
  `signed_bits < 0` on arbitrary 32-bit lookahead, regardless of score.
  If invalid truncations or promotions dominate, freeze established type/cast
  passes in the scratch configuration and retain all baseline improvements for
  review; a lower best-only score can otherwise hide acceptable candidates.
  Still inspect newly introduced temporaries: expression extraction can invent
  wider types even with type-randomization disabled. If a widened integer is
  immediately narrowed at every consumer, require independent width evidence:
  MWCC can discard the upper half yet change allocation enough to score zero.
  The missing upper-half instructions do not justify the widened source type
  ([AI category audit](../../.agent-work/decomp/ai-matching/README.md#round-138-reject-unsupported-widened-category-temporary)).
  Declaration-only search is not scope-preserving: the local upstream
  `perm_reorder_decls` can move declarations between blocks and does not model
  scope. For a pure ordering diagnosis, restrict candidates to uninitialized
  declarations within the same block; otherwise inspect scope and initializer
  effects separately. Compilation failures do not exhaust valid orderings
  ([dispatcher control](../../.agent-work/decomp/ai-matching/README.md#round-188-declaration-only-search-limit)).
  The optional [local patch](../../.agent-work/patches/decomp-permuter/decomp-permuter-same-block-declarations.patch)
  enables this constraint with `MKD_PERM_SAME_BLOCK_DECLS=1`; leave only
  `perm_reorder_decls` enabled for the controlled search. Apply/check it in the
  external checkout before use. It filters explicit initializers, not VLA/type
  side effects; review those separately. See [validated use](../../.agent-work/decomp/ai-matching/README.md#round-189-same-block-declaration-search).
  Reject dummy control blocks.
- IF scratch parsing changes `numNodes * sizeof(RwMatrix) + 15`, REQUIRE comparison
  with original source; TRY parentheses around the product in scratch without
  changing allocation math. IF inline-helper PERM_LINESWAP fails with "PERM macro
  in AST", TRY finite text-level PERM_GENERAL alternatives. Verify the real TU.

- IF a CSE candidate repeats a table expression, REQUIRE both values to feed real
  decoded outputs, a nonvolatile lookup, side-effect-free indices, and no
  intervening store or call. TRY the shared field-extraction form only after
  verifying retail load count/order, every macro consumer, runtime behavior and
  the linked SHA-1. Used field expressions are distinct from dead sinks or fake
  liveness; reject dummy blocks, volatile coercion and unused reads. Record the
  source form as a hypothesis, not proof of original C spelling. See the
  [MPVABDEC closure](matching-knowledge.md#decoder-reconstruction-transferable-findings).

Mirage imports require platform-neutral behavior and GC retail/objdiff evidence.
Keep new lessons in the relevant diagnostic; record measured searches in the
[consolidated knowledge record](matching-knowledge.md).
