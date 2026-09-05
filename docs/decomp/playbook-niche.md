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

## Hard stops

After the applicable honest source check, stop for:

- GPR/FPR coloring, parameter nonvolatile homes, or permutations rotating residue.
- li-zero vs copying an already-zero register; commutative scratch encodings.
- Frameless PLATFORM mtlr/blrl emission.
- Anonymous relocation labels with verified identical payloads/targets.
- Equivalent branch/address lowering without new source evidence.

Unknown calls/offsets/CFG are not coloring: classify borked or breakthrough needed
and name missing evidence. Assembly-required targets are skipped; AGENTS.md
requires explicit per-function permission for the rare sequence path.
Record source `TODO: [near miss]` with score, residual, and stop reason; disclose
nonmatching fallback in SHA results. Never omit returns or invent lifetimes,
types, empty arms, fake volatile, dead sinks, register declarations, or goto.

## Optional search

Permuter requires established algorithm/CFG/ABI/layout and the real TU command.
Keep PERM_* in scratch; task-specific attempt limits never waive verification.
Reject UB, wrong types, reordered effects, and fake liveness even at zero score.

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
- IF a candidate offsets both sides of a signed comparison, REQUIRE proof that
  the offset cannot overflow across the complete input domain. Reject
  `(signed_bits + 1) < 1` and `(signed_bits - 1) < -1` as replacements for
  `signed_bits < 0` on arbitrary 32-bit lookahead, regardless of score.
- IF scratch parsing changes `numNodes * sizeof(RwMatrix) + 15`, REQUIRE comparison
  with original source; TRY parentheses around the product in scratch without
  changing allocation math. IF inline-helper PERM_LINESWAP fails with "PERM macro
  in AST", TRY finite text-level PERM_GENERAL alternatives. Verify the real TU.

Mirage imports require platform-neutral behavior and GC retail/objdiff evidence.
Keep new lessons in the relevant diagnostic; link detailed searches from reports.
