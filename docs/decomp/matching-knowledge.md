# Matching knowledge and effectiveness — GQNE5D

This is the consolidated record of the September 4–5, 2026 matching campaigns.
It replaces temporary campaign diaries and intermediate evidence dumps. Follow
[AGENTS.md](../../AGENTS.md), [conventions](conventiond.md), and the diagnostic
[high](playbook-high-occurrence.md), [mid](playbook-mid-occurrence.md), and
[niche](playbook-niche.md) books. Rule IDs remain stable for measurement.

Latest completed run: **113 new matches / 28,980 bytes from 77 source trials**.
Start with the [measured rule ranking](#measured-rule-ranking) and
[reusable findings](#findings-to-reuse); the historical decoder/runtime evidence
below explains the techniques that led into this campaign.

## What counts as a result

Retail assembly, callers, symbols, relocations, allocation extents and object
layout govern reconstruction. m2c, Ghidra, Mirage and permuter candidates are
hypotheses. Matching operations with different registers is different from an
unresolved algorithm, ABI, memory access or branch destination.

Measure the same function in the same comparison mode before and after each
coherent edit. Check ordinary/report and `functionRelocDiffs=data_value` results,
instruction differences and all affected siblings. Report-100 can hide an
incorrect float or diagnostic literal. Named-pool relocation residue must be
reported even when used bytes agree. An exact function in a NonMatching unit
is not source-link-exact: the normal retail SHA check may use its fallback.
Only a rebuilt source object actually linked into the matching DOL establishes
that stronger result. Preserve failed trials in the experiment denominator.

Use one accurate progress TODO immediately above each remaining nonmatch;
remove progress comments at measured 100%. Reject forced registers, fake
volatile, unused sinks, fabricated lifetimes, incorrect prototypes, invented
fields, undefined arithmetic, dummy control flow and assembly workarounds.
A source-quality or correctness correction can be retained despite a disclosed
fuzzy decrease; it cannot be counted as an exact closure.

## Historical results and their limits

| Campaign | New exact functions | Exact code bytes | Evidence scope |
| --- | ---: | ---: | --- |
| September 4 near matches | 30 | 14,856 | Report/data-value; full build and retail SHA passed |
| September 5 second near-match set | 30 | 9,496 | Report/data-value; concurrent aggregate gains excluded |
| September 5 campaign30 | 30 | 12,684 | 20 TUs; no exact regressions; fallback link retained |
| September 5 campaign30c | 30 | 9,508 | 13 NonMatching TUs; 2,023 functions checked, 1,181 prior exact preserved |
| MPVABDEC three-function reconstruction | 3 | 51,408 | Entire source unit and 3,096 data bytes exact and source-linked |
| Nine-candidate MPV follow-up | 4 | 5,628 | Report/data-value; affected units still NonMatching |

These are campaign-attributed results, not sums of shared-worktree report deltas.
The MPVABDEC row gives the completed unit, not the gain of its final attempt
alone (17,560 bytes). Earlier winner lists are selected samples, so they do not
establish a per-rule success probability. Four 30-function rounds mainly drew
from report scores at least 90%; some standalone baselines differ from reports.

Common useful recoveries were canonical ABI/field widths, true reload boundaries,
real declaration scopes, structured joins, shared inline ownership, and actual
constant extents. For example: skin platform data is +0x2C, not +0x34; process
entry is +0xB8, not its +0xB4 destruction callback; an observed stored float must
be read back when its rounding affects subsequent publication. Shared NPC waits
reload `g_active_npc` after the saved-owner countdown store. These are concrete
semantic distinctions even when the initial score is above 99%.

## Decoder reconstruction: transferable findings

### Preserve information and ownership

- Keep packed sign bits until their last use. Combined VLC indices use 0x3FF
  or 0xFFF before sign extraction; 0x3FE or 0xFFE silently loses negative levels.
  Mask only the address when the remaining packed bits have later consumers.
- A signed short table load can feed a full-width signed accumulator. Narrowing
  the accumulator itself to s16 can add a non-retail `extsh` and change behavior.
- Distinguish original and advanced reader words while both are live. Own setup,
  first-code decoding, consumption/refill, AC loops and final publication through
  real phase boundaries. An outer decode helper may span initialization through
  publication; independent coefficient clearing remains outside. Flattening the
  nested AC phase regressed allocation.
- Assemble extended escape amplitude before increasing code length; a genuinely
  used `extended_base = packed * 2` can express this stage. Form the DC sign
  threshold after extracting unsigned amplitude. Preserve Dc11's logical sign
  correction and arithmetic payload extraction.
- Share macros according to corresponding retail expansions. Generic quantization
  and constant fast paths need not lower identically. Preserve operation grouping
  and a real quantizer-scaled level; do not reassociate floating-point arithmetic.
- A discarded marker can use general consume/refill instead of a flag-reading
  macro. Decode a field into a real local if retail publishes it once at a join.

### Recover expansion boundaries, then tune real locals

A typed helper is useful only if its emitted boundary agrees with retail. The
large decoder required both `inline_max_size` and `inline_max_total_size` to
permit proven call-free expansion. These scoped bounds do not recover the
original pragma values. Merely writing `inline` previously left extra calls and
save/restore helpers; the whole-object audit caught them.

An unsigned helper performing the real `lookahead << 1` transformation closed
Dc11's escape scheduling and helped Nintra AC, but regressed Nintra's first-code
phase. Apply the boundary only where evidence supports it. Identity helpers and
manufactured state are not acceptable substitutes.

The last Nintra mismatch closed through a reviewed common-subexpression form:
cache the nonvolatile table word for length/level, and extract run from the same
table expression before publication. Both values feed real outputs, all indices
are side-effect-free, and no store or call intervenes. MWCC emits one actual
lookup in retail order. This is an acceptable reconstruction hypothesis under
those explicit conditions, not proof of original C spelling or permission to add
dead reads. All four consumers, runtime checks and linked SHA were verified.

### Mutable cursor ownership generalizes to fills

A helper taking a typed `T **` can own real stores and advance the caller's
cursor between repeated expansions. Returning an end pointer or advancing in
the caller did not reproduce the retail updates in the MPV follow-up.

- CBP uses short local 4/8-entry fills and helper-owned longer 16/32/64-entry
  fills. Changing the older shared helper regressed other consumers.
- DC-size output stores use signed-char emission while the canonical storage
  retains its unsigned-byte reader view. This expresses the observed immediates.
- Six coefficient blocks need a proven contiguous six-block union view:
  `f32 values[6][64]` / `f64 pairs[192]`. A 32-pair clear expansion advances a
  real `f64 **` cursor six times. Indexing beyond a one-block array is invalid.
- IntraBlocks preserves four explicit luma callbacks followed by two chroma
  callbacks. Direct typed context-array addresses avoid an unnecessary long-lived
  base pointer without changing callback order.

The follow-up spent 92 source attempts over two passes of up to six per function:

| Function | Bytes | Standalone before | Retained | Attempts |
| --- | ---: | ---: | ---: | ---: |
| mpvvlc_InitCbpSub2 | 2,100 | 90.260956% | 100% | 8 |
| mpvvlc2_InitDcSizY | 1,288 | 89.795030% | 100% | 8 |
| mpvvlc2_InitDcSizC | 1,132 | 90.134280% | 100% | 7 |
| MPVCDEC_IntraBlocks | 1,108 | 69.660650% | 100% | 9 |
| MPVDEC_DecBpicMb | 1,596 | 94.887215% | 97.957390% | 12 |
| MPVDEC_DecPpicMb | 1,492 | 94.946380% | 97.868630% | 12 |
| mpvhdec_DecShcSj | 1,464 | 92.412570% | 96.038250% | 12 |
| MPVDEC_DecIpicMb | 1,016 | 90.255905% | 96.318900% | 12 |
| MPVDEC_DecDpicMb | 976 | 91.352460% | 96.290985% | 12 |

All four exact closures passed ordinary and data-value comparison with zero
instruction differences. The five improvements remain nonmatches. Cursor helper
ownership is a supported family-specific tactic, not a claim that 4/9 of arbitrary
functions will close. Repeated attempts and shared source edits are correlated.

## Runtime evidence and Mirage boundaries

`tools/check_mpvabdec.py` executes retail PowerPC code against the relocated MWCC
candidate. Final source125 passed 34,296 cases: Nintra 10,296, Intra 11,264,
Dc11 12,736, with no failures/errors. Coverage includes signs, bit alignments,
escapes, DC prediction, varied quantization and seeded mixed coefficient blocks.
The old sign-bug negative control failed 3,036 cases without execution errors.
Execute retail initialization before table comparisons: run/level tables 4, 2,
and 1 are biased by -16, -32 and -32 bytes. Compare coefficient bits, reader and
predictor state, return values and guards. Use an isolated Python environment
with `unicorn==2.1.4`; the checker accepts `--object` and `--output`.

`tools/check_mpv_followup.py` passed 2,152 isolated retail comparisons: DC Y/C
4 each, CBP 64, IntraBlocks 64, header 96, B/P 320 each, I/D 640 each. Its
transport/block/final-DCT doubles deliberately clobber caller-saved registers.
These tests cover sampled block/header behavior, not complete frame rendering,
all malformed input, motion decoding, or proof that Mirage artifacts are gone.
The reusable checkers survive scratch cleanup.

Final MPVABDEC source SHA-256:
`b1213bb5ae430af68bda4262c6c09bf85f10a7519e31e2824e1d545e4741f945`.
Runtime-tested and source-linked object SHA-256:
`9c2d0e21cb9718d1a0a56110495caf37538b279c36361262d6937c89a5ddba20`.
Retail ELF SHA-256:
`dbb1cda77817d1e421be4c721b2aba0ad918d2a84ad2a2b6c21e30bd17d5881f`.
Final retail DOL SHA-1: `ef001212c6cd4da3194c041d24956a61bc5e9fcd`.
Known runtime-source-version and extabindex linker warnings did not prevent
that source-linked hash equality.

Mirage supplies useful independent runtime context, not original compiler-source
proof. ScreenPoly visibility is 0x80 and filtering 0x40; native GCC bitfield order
caused gallery paging defects. Five portable visibility-mask source trials
regressed GC matching and were reverted; native compatibility belongs at the
port boundary. Sofdec inactive output index 8 aliases frame-zero width for
preparation and height for termination: use a containing-object byte view for
these proven addresses rather than out-of-bounds buffer indexing. A focused
32-bit GCC UBSan accessor probe passed indices 0–8, aliases and adjacent guards.
The separate tracked [settings retrofeed](mirage-settings-retrofeed-2026-09-05.md)
remains authoritative for its unrelated integration history.

## Search efficiency and failed hypotheses

m2c-first recovery is useful for structure and type hypotheses, not an exactness
score. Permuter belongs after algorithm, CFG, ABI and layout agreement. Use the
actual TU compiler command and stack-sensitive scoring: default scoring once
reported zero while reversed +0x08/+0x0C stores still differed (`--stack-diffs`
reported 8). Include the expanded helper body in the mutation region and retain
the caller for scoring. Inspect the entire generated candidate, including code
outside the requested randomization region.

Freeze proved types when inappropriate narrowing dominates, but still inspect
newly extracted temporaries. Reject signed-overflow comparison identities and
changed side-effect order. Verify the newly printed scratch path: stale imports
and a one-line PERM_LINESWAP previously ran random search instead of the claimed
finite enumeration. Parser repairs belong in scratch and must preserve math.

Finite declaration searches yielded useful real scopes/orders in several
functions, but 5,040 camera-snapshot variants, 5,040 fade variants and 720 decoder
DC declaration variants also failed. Long random searches are not evidence for
retrying the same lifetime graph. A changed ownership/CFG hypothesis can justify
a new search. Stop harmless coloring after the applicable honest insight; seek
another candidate rather than introducing artificial lifetime scaffolding.

Compile failures count as source attempts. A previous broad comment-removal
regex accidentally removed other functions and was reverted; match one immediate
comment and recheck the entire TU. C89 declaration placement and consistent
forward declarations are infrastructure checks, not algorithmic breakthroughs.
Do not infer success from a provisional scratch zero before quality review.

## Completed 100-function effectiveness campaign

The campaign exceeded its target with **113 unique new matches, 28,980 code
bytes, across 41 winning translation units**. Each was below 100% in the captured
baseline report and reached fresh report 100%, data-value 100% and zero meaningful
instruction differences. The parent agent independently checked every winner and
the live source/object hashes for all 57 translation units represented in the
trial ledger. No baseline report-exact function was lost in those units.

The baseline was 7,689 report-exact functions and 978,192 exact code bytes out of
3,042,852. The first pool contained 1,984 candidates at least 90% but below 100%;
317 TUs yielded 636 localized candidates. Subsequent owner-based sibling searches
and below-90 screens expanded that pool: **45 winners began below 90%**. Screening
passes overlap; they are not additional independent samples. Earlier campaign
matches and baseline report-exact/data-only corrections are excluded.

One autonomous agent performed the matching campaign. The parent supplied bounded
read-only screening, source-quality audits and one delegated memcard trial. The
budget was two passes of up to six source attempts per function, stopping sooner
at exactness or a supported ceiling; no target exhausted that budget. m2c and
permuter scratch iterations were exempt. The last shared FIFO correction yielded
24 winners at once, so stopping at exactly 100 would have discarded valid results.

### Measured rule ranking

Ranked by newly exact bytes, with correctness and applicability gates first.
“Accepted” includes useful retained nonmatches; “winning trials” requires at least
one final new winner. A shared header edit is one trial regardless of consumers.
Composite IDs remain one bucket. Credit below goes to the closing rule; the winner
table retains observed enabling chains, so last-step credit is not causal proof.

| Rank | Rule | Trials | Accepted | Winning trials | New functions | New bytes | Winning TUs |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | H02 | 23 | 22 | 15 | 66 | 16,668 | 21 |
| 2 | H11 | 3 | 3 | 2 | 6 | 3,928 | 2 |
| 3 | M01 | 12 | 8 | 5 | 11 | 3,428 | 5 |
| 4 | H15 | 3 | 2 | 2 | 2 | 1,188 | 2 |
| 5 | H06 | 8 | 6 | 3 | 6 | 884 | 1 |
| 6 | H03 | 6 | 5 | 4 | 5 | 824 | 3 |
| 7 | H01,H03 | 2 | 2 | 2 | 4 | 580 | 2 |
| 8 | M16 | 2 | 1 | 1 | 4 | 460 | 2 |
| 9 | H14 | 3 | 2 | 2 | 3 | 296 | 2 |
| 10 | H05 | 1 | 1 | 1 | 1 | 276 | 1 |
| 11 | H04 | 4 | 2 | 2 | 3 | 232 | 2 |
| 12 | H07 | 1 | 1 | 1 | 1 | 112 | 1 |
| 13 | H10 | 3 | 3 | 1 | 1 | 104 | 1 |
| 14 | H03,M07 | 1 | 0 | 0 | 0 | 0 | 0 |
| 15 | M03 | 2 | 0 | 0 | 0 | 0 | 0 |
| 16 | M04 | 3 | 1 | 0 | 0 | 0 | 0 |

There were **77 distinct source trials**: 59 accepted, 17 reverted and 1 no-op. A no-op is not a closure. The two-TU DVD queue correction and the multi-TU GX FIFO correction each count once.

Practical priority: recover typed owners and layout first (H02), inspect real
return/cleanup joins (H11), then test a proven TU compiler-mode hypothesis (M01)
before localized lifetime tuning (H15). Width/ABI, snapshot, stack and object
placement rules remain evidence-driven alternatives. Never choose a lower-ranked
rule that does not fit merely because it closed another function.

This is selected observational evidence. H02's hardware-register families share
headers, compiler behavior and provenance; its many consumers are correlated.
Small rule samples do not establish general success probabilities. M01 enabled
later H15 closures in joy/navigation, H03 preceded the ejb H11 closures, and H02
preceded the jab H11 closure. Failed GX snapshots helped identify the FIFO owner
problem but were reverted; their source does not contribute to the final match.

All recorded trials used m2c, the pinned MWCC compiler and objdiff. No Ghidra or
permuter trial contributes to this campaign's measured results; their effectiveness
is unmeasured here, not zero. Timestamps were recorded, but reliable per-trial
elapsed durations were not: gaps include screening, builds and audits. No runtime
speedup or tool-time ranking is inferred.

### Findings to reuse

- H02: Preserve the compiler-visible hardware owner, not just its numeric address.
  Canonical absolute volatile DI, SI and VI arrays closed whole sibling families.
  GX abort uses the fixed PI bank locally; other GX APIs legitimately use runtime
  register pointers. The GX FIFO is one absolute volatile union at 0xCC008000,
  with byte, halfword, word and float writes at offset zero. That one correction
  closed 24 functions across eight winning TUs; nine TUs changed. Local owner
  snapshots had failed. Preserve every store's width, value and order.
- H02/H03: Distinguish stored width from input/accumulator width. The breakpoint
  queue stores a byte but tests the full int argument. Ejb rollup choices retain
  unsigned-short extraction promoted into a signed int comparison. Neither case
  authorizes arbitrary narrowing or signed-overflow identities.
- H02: Check allocation and producers before coloring. Sphere bytecode has two
  radii before its option; jab uses pdata_list_b at +0xC4; NIS consumes npc_list at
  +0x3C. Correct cloth word clearing and the particle frame's buffer-B owner were
  retained improvements, not counted as matches.
- H11: A real shared false/null tail can affect all inline consumers. Preserve
  branch destinations and effects; do not duplicate stores or insert empty arms.
- M16: The eight-byte DVD thread queue needs its canonical complete type at its
  first extern declaration. Including that type before the declaration recovered
  small-data addressing in four functions; 119 header consumers rebuilt. Explicit
  aggregate zero initialization in SVM instead moved storage into .data and failed.
- M01: Object-scoped compact mode closed multiple game/runtime/memory functions,
  but mwsfdcre and gcci trials broke prior exacts and were reverted. Accepted mode
  changes still require all-function checks and disclosure of nonexact losses.
- H07: Use an existing canonical typed field macro when it reproduces a proven
  inline access. The RenderWare atomic callback's helper emitted a non-retail call.
- H10: SPSD decoding distinguishes cases 2/3 from kinds >=4; m2c's default-case
  hypothesis was wrong. Preserve registered callbacks and common normalization
  after format-specific stores. Retail control flow governs the interpretation.
- Stop: repeated FP/declaration/grouping trials and equivalent bound spellings
  often do not close. The memcard >=2 guard improved 88.90→89.00, with a return
  branch still different. TRK comparison reversal broke an exact sibling and was
  reverted. Typed pipeline pointer subtraction did not recover retail divw.

Read-only exclusions included handwritten exception/stack/cache paths and the
minigames chain-message null path, whose retail code dereferences the null owner.
No assembly sequence, fake volatile, register forcing, dummy sink or null-pointer
dereference was added to meet the target. Prior AI and script-grouping ceilings
were not retried without new evidence. Future structural leads include separating
PlyrInfo from collision allocation storage and correcting the script's puzzle
continuation owner; these were screened, not claimed as completed changes.

### Trial outcomes

Compact record of every source trial. IDs identify measurements, not source
comments. Numbers in the final column count only the verified final winners.
Failed and no-op trials remain in the denominator; code-only reformatting and
comment audits are not additional matching hypotheses.

| Trial | Rule | Decision | Affected TU(s) | New functions / bytes |
| --- | --- | --- | --- | ---: |
| attempt001 | H06 | accept | mwsfdsst | 1 / 140 |
| attempt002 | H06 | accept | mwsfdsst | 4 / 600 |
| attempt003 | H06 | accept | mwsfdsst | 1 / 144 |
| attempt004 | H06 | accept | mwsfdsst | 0 / 0 |
| attempt005 | M16 | revert | svm | 0 / 0 |
| attempt006 | M03 | no-op | pfxfont | 0 / 0 |
| attempt007 | M03 | revert | pfxfont | 0 / 0 |
| attempt008 | M04 | revert | emitter | 0 / 0 |
| attempt009 | H04 | accept | VMMapping | 2 / 44 |
| attempt010 | H03 | accept | mslsupp | 2 / 264 |
| attempt011 | H10 | accept | mslsupp | 0 / 0 |
| attempt012 | H03 | accept | mslsupp | 1 / 220 |
| attempt013 | H06 | accept | mslsupp | 0 / 0 |
| attempt014 | H10 | accept | mslsupp | 0 / 0 |
| attempt015 | H14 | accept | utils | 2 / 152 |
| attempt016 | H14 | revert | cam | 0 / 0 |
| attempt017 | H14 | accept | cam | 1 / 144 |
| attempt018 | H02 | accept | vm_spawn | 1 / 124 |
| attempt019 | H02 | accept | sfd_pl2 | 1 / 16 |
| attempt020 | M01 | accept | jmt | 5 / 860 |
| attempt021 | M01 | accept | moves | 2 / 676 |
| attempt022 | M01 | accept | pz_fighters | 1 / 932 |
| attempt023 | H03 | revert | pz_fighters | 0 / 0 |
| attempt024 | H04 | accept | pz_fighters | 1 / 188 |
| attempt025 | H15 | revert | pz_fighters | 0 / 0 |
| attempt026 | M01 | revert | mwsfdcre | 0 / 0 |
| attempt027 | M01 | accept | mwMem | 2 / 340 |
| attempt028 | M01 | revert | gcci | 0 / 0 |
| attempt029 | M01 | accept | mk_render | 1 / 620 |
| attempt030 | M01 | revert | mwMemHeap | 0 / 0 |
| attempt031 | H02 | accept | OSFont | 2 / 312 |
| attempt032 | H02 | accept | Pad | 2 / 308 |
| attempt033 | H05 | accept | Pad | 1 / 276 |
| attempt034 | H04 | revert | mk_chess | 0 / 0 |
| attempt035 | H04 | revert | mk_chess | 0 / 0 |
| attempt036 | H02 | accept | OSError | 1 / 536 |
| attempt037 | H02 | accept | OSReset | 0 / 0 |
| attempt038 | H10 | accept | OSReset | 1 / 104 |
| attempt039 | H02 | revert | dvdlow | 0 / 0 |
| attempt040 | H02 | accept | dvdlow | 5 / 628 |
| attempt041 | H02 | accept | adx_bsps | 1 / 236 |
| attempt042 | H03 | accept | adx_bsps | 1 / 292 |
| attempt043 | M01 | accept | joy | 0 / 0 |
| attempt044 | H15 | accept | joy | 1 / 680 |
| attempt045 | M01 | accept | konquest_nav | 0 / 0 |
| attempt046 | H15 | accept | konquest_nav | 1 / 508 |
| attempt047 | M01 | revert | sfd_set | 0 / 0 |
| attempt048 | M01 | accept | sfd_set | 0 / 0 |
| attempt049 | H06 | accept | sfd_set | 0 / 0 |
| attempt050 | H01,H03 | accept | bgnd_mab | 2 / 192 |
| attempt051 | H03 | accept | bgnd | 1 / 48 |
| attempt052 | H02 | accept | bgnd | 0 / 0 |
| attempt053 | H02 | accept | konquest | 1 / 204 |
| attempt054 | H06 | revert | GXGeometry | 0 / 0 |
| attempt055 | H03 | accept | ejb | 0 / 0 |
| attempt056 | H11 | accept | ejb | 5 / 3,700 |
| attempt057 | H02 | accept | jab | 0 / 0 |
| attempt058 | H11 | accept | jab | 1 / 228 |
| attempt059 | H02 | accept | cloth | 0 / 0 |
| attempt060 | H02 | accept | dvd | 5 / 2,900 |
| attempt061 | H02 | accept | SIBios | 12 / 3,272 |
| attempt062 | H02 | accept | vi | 7 / 2,756 |
| attempt063 | H02 | accept | EXIBios | 1 / 72 |
| attempt064 | H02 | accept | OSInterrupt | 0 / 0 |
| attempt065 | M16 | accept | dvd, dvdfs | 4 / 460 |
| attempt066 | H02 | accept | behavior | 0 / 0 |
| attempt067 | H11 | accept | behavior | 0 / 0 |
| attempt068 | H02 | accept | GXMisc | 2 / 820 |
| attempt069 | H01,H03 | accept | dlbrkpt | 2 / 388 |
| attempt070 | H02 | accept | OS | 1 / 60 |
| attempt071 | H02 | accept | dsp_task | 0 / 0 |
| attempt072 | M04 | revert | msgbuf | 0 / 0 |
| attempt073 | H03,M07 | revert | p2define | 0 / 0 |
| attempt074 | H07 | accept | baclump | 1 / 112 |
| attempt075 | H06 | revert | GXTev | 0 / 0 |
| root_memcard01 | M04 | accept | memcard | 0 / 0 |
| attempt076 | H02 | accept | GXAttr, GXMisc, GXGeometry, GXFrameBuf, GXLight, GXBump, GXTev, GXPixel, GXTransform | 24 / 4,424 |

### Verified new functions

Every row ends at report/data-value 100% with zero meaningful instruction diffs.
Baseline is the captured report score. Six ordinary standalone scores retain
verified anonymous-label identity residue; those scores are listed explicitly.
The rule chain records retained changes observed in that function's trial deltas.

| Function | TU | Bytes | Baseline % | Ordinary final % | Retained rule chain |
| --- | --- | ---: | ---: | ---: | --- |
| `cbForStateBusy` | dvd | 1,624 | 99.502464 | 100.000000 | H02 |
| `VIInit` | vi | 1,200 | 93.450000 | 100.000000 | H02 |
| `back_rollup_check_reverse` | ejb | 964 | 98.651450 | 100.000000 | H03 → H11 |
| `pz_fighter_classify_move_8012260C` | pz_fighters | 932 | 97.081540 | 100.000000 | M01 |
| `back_rollup_check` | ejb | 908 | 98.568280 | 100.000000 | H03 → H11 |
| `SIInterruptHandler` | SIBios | 836 | 98.535890 | 100.000000 | H02 |
| `stateBusy` | dvd | 832 | 81.610580 | 100.000000 | H02 |
| `front_rollup_check` | ejb | 820 | 98.414635 | 100.000000 | H03 → H11 |
| `CompleteTransfer` | SIBios | 764 | 76.821990 | 100.000000 | H02 |
| `p_joy_entry` | joy | 680 | 96.082350 | 99.676470 | M01 → H15 |
| `__VIRetraceHandler` | vi | 628 | 85.496820 | 100.000000 | H02 |
| `btree_render` | mk_render | 620 | 95.741936 | 100.000000 | M01 |
| `OSSetErrorHandler` | OSError | 536 | 97.522385 | 100.000000 | H02 |
| `GXSetFog` | GXPixel | 532 | 95.406010 | 100.000000 | H02 |
| `__SITransfer` | SIBios | 524 | 73.137405 | 100.000000 | H02 |
| `GXSetCopyFilter` | GXFrameBuf | 520 | 89.153850 | 100.000000 | H02 |
| `__VIInit` | vi | 516 | 83.333336 | 100.000000 | H02 |
| `nav_get_unit_vector_to_closest_area` | konquest_nav | 508 | 92.905510 | 100.000000 | M01 → H15 |
| `j_getup_back_3` | ejb | 504 | 98.373020 | 100.000000 | H11 |
| `j_getup_front_4` | ejb | 504 | 98.373020 | 100.000000 | H11 |
| `GXAbortFrame` | GXMisc | 456 | 97.140350 | 100.000000 | H02 |
| `GXSetTevOrder` | GXTev | 412 | 97.815540 | 100.000000 | H02 |
| `GXCopyTex` | GXFrameBuf | 396 | 91.090910 | 100.000000 | H02 |
| `x_pickup` | moves | 372 | 92.903230 | 99.677420 | M01 |
| `__GXAbort` | GXMisc | 364 | 96.417580 | 100.000000 | H02 |
| `player_area_collision_ticks` | jmt | 304 | 93.868420 | 99.868420 | M01 |
| `p_block` | moves | 304 | 93.947365 | 99.736840 | M01 |
| `i_MWY_GCN_RW_AppendGxBreakPtQueue` | dlbrkpt | 300 | 96.466670 | 100.000000 | H01,H03 |
| `GXSetFogRangeAdj` | GXPixel | 292 | 69.657530 | 100.000000 | H02 |
| `ADX_DecodeInfoSpsd` | adx_bsps | 292 | 99.178085 | 100.000000 | H03 |
| `PADRecalibrate` | Pad | 276 | 90.289856 | 100.000000 | H02 → H05 |
| `DVDCheckDisk` | dvd | 248 | 98.370964 | 100.000000 | H02 |
| `ADXB_DecodeHeaderSpsd` | adx_bsps | 236 | 96.576270 | 100.000000 | H02 |
| `GXSetChanAmbColor` | GXLight | 232 | 86.448270 | 100.000000 | H02 |
| `GXSetChanMatColor` | GXLight | 232 | 86.448270 | 100.000000 | H02 |
| `jab_attach_point_light_to_obj_bone` | jab | 228 | 92.789474 | 100.000000 | H02 → H11 |
| `__position_file` | mslsupp | 220 | 80.381820 | 100.000000 | H10 → H03 |
| `DVDInit` | dvd | 216 | 93.333336 | 100.000000 | H02 → M16 |
| `mks_start_gusher` | jmt | 216 | 91.481480 | 100.000000 | M01 |
| `OSGetFontWidth` | OSFont | 212 | 98.094340 | 100.000000 | H02 |
| `SIGetResponseRaw` | SIBios | 212 | 94.962265 | 100.000000 | H02 |
| `GXSetDispCopyYScale` | GXFrameBuf | 204 | 69.098040 | 100.000000 | H02 |
| `resume_effect_at_obj_bid` | jmt | 204 | 82.352940 | 100.000000 | M01 |
| `nis_remove_non_participants` | konquest | 204 | 99.980390 | 100.000000 | H02 |
| `SIGetResponse` | SIBios | 196 | 94.551020 | 100.000000 | H02 |
| `mwMemSystemCreateSystemHeap` | mwMem | 188 | 76.893616 | 100.000000 | M01 |
| `p_plyr_pz_fighter_start` | pz_fighters | 188 | 98.723404 | 99.468090 | H04 |
| `PADControlMotor` | Pad | 184 | 96.521736 | 100.000000 | H02 |
| `GXSetDrawSync` | GXMisc | 180 | 92.777780 | 100.000000 | H02 |
| `SIInit` | SIBios | 180 | 89.422226 | 100.000000 | H02 |
| `DVDCancel` | dvd | 172 | 87.279070 | 100.000000 | M16 |
| `DVDLowReadDiskID` | dvdlow | 164 | 91.756096 | 100.000000 | H02 |
| `MWSST_GetOutVol` | mwsfdsst | 160 | 95.000000 | 100.000000 | H06 |
| `MWSST_GetStat` | mwsfdsst | 160 | 95.000000 | 100.000000 | H06 |
| `DVDLowInquiry` | dvdlow | 156 | 89.589745 | 100.000000 | H02 |
| `SIEnablePolling` | SIBios | 156 | 91.948715 | 100.000000 | H02 |
| `GXSetDrawDone` | GXMisc | 152 | 92.500000 | 100.000000 | H02 |
| `privSystemCreateAutomated` | mwMem | 152 | 82.105260 | 100.000000 | M01 |
| `SIEnablePollingInterrupt` | SIBios | 152 | 95.763160 | 100.000000 | H02 |
| `VIGetCurrentLine` | vi | 152 | 91.921050 | 100.000000 | H02 |
| `init_scripted_camera` | cam | 144 | 99.833336 | 99.305560 | H14 |
| `__GXSetViewport` | GXTransform | 144 | 80.694440 | 100.000000 | H02 |
| `MWSST_StartSj` | mwsfdsst | 144 | 83.333336 | 100.000000 | H06 |
| `DVDLowRequestError` | dvdlow | 140 | 91.971430 | 100.000000 | H02 |
| `DVDLowStopMotor` | dvdlow | 140 | 91.971430 | 100.000000 | H02 |
| `MWSST_Pause` | mwsfdsst | 140 | 94.285710 | 100.000000 | H06 |
| `MWSST_SetOutVol` | mwsfdsst | 140 | 94.285710 | 100.000000 | H06 |
| `MWSST_Stop` | mwsfdsst | 140 | 94.285710 | 100.000000 | H06 |
| `__GXSetVAT` | GXAttr | 136 | 92.205880 | 100.000000 | H02 |
| `__GXSendFlushPrim` | GXGeometry | 136 | 96.029410 | 100.000000 | H02 |
| `__close_console` | mslsupp | 132 | 98.181816 | 100.000000 | H03 |
| `__close_file` | mslsupp | 132 | 98.181816 | 100.000000 | H03 |
| `cbForUnrecoveredErrorRetry` | dvd | 128 | 96.843750 | 100.000000 | H02 |
| `pfxvm_spawn_sphere` | vm_spawn | 124 | 99.967740 | 100.000000 | H02 |
| `__PADDisableRecalibration` | Pad | 124 | 83.870964 | 100.000000 | H02 |
| `AtomicDefaultRenderCallBack` | baclump | 112 | 88.571430 | 100.000000 | H07 |
| `SIDisablePolling` | SIBios | 108 | 92.592590 | 100.000000 | H02 |
| `SISetXY` | SIBios | 108 | 92.370370 | 100.000000 | H02 |
| `GXSetTevAlphaOp` | GXTev | 104 | 84.038460 | 100.000000 | H02 |
| `GXSetTevColorOp` | GXTev | 104 | 84.038460 | 100.000000 | H02 |
| `KillThreads` | OSReset | 104 | 75.961540 | 100.000000 | H02 → H10 |
| `getCurrentFieldEvenOdd` | vi | 104 | 89.538460 | 100.000000 | H02 |
| `GXSetTevColorS10` | GXTev | 100 | 67.160000 | 100.000000 | H02 |
| `GXSetTevKColor` | GXTev | 100 | 77.400000 | 100.000000 | H02 |
| `single_frame_collision_check` | jmt | 100 | 80.560000 | 100.000000 | M01 |
| `OSGetFontEncode` | OSFont | 100 | 95.960000 | 100.000000 | H02 |
| `yinyang_set_bad_fish_hide_flag` | bgnd_mab | 96 | 97.291664 | 100.000000 | H01,H03 |
| `yinyang_set_good_fish_hide_flag` | bgnd_mab | 96 | 97.291664 | 100.000000 | H01,H03 |
| `GXSetTevColor` | GXTev | 96 | 84.166664 | 100.000000 | H02 |
| `__VIGetCurrentPosition` | vi | 96 | 82.375000 | 100.000000 | H02 |
| `GXSetTevKAlphaSel` | GXTev | 92 | 88.478264 | 100.000000 | H02 |
| `GXSetTevKColorSel` | GXTev | 92 | 88.478264 | 100.000000 | H02 |
| `MWY_GCN_RW_AppendGxBreakPtQueue` | dlbrkpt | 88 | 95.227270 | 100.000000 | H01,H03 |
| `material_set_uv_scroll_matrix` | utils | 76 | 99.842100 | 100.000000 | H14 |
| `material_set_uv_scroll_matrix_2` | utils | 76 | 99.842100 | 100.000000 | H14 |
| `EXIClearInterrupts` | EXIBios | 72 | 81.944440 | 100.000000 | H02 |
| `DVDReset` | dvd | 68 | 81.411766 | 100.000000 | H02 |
| `InquiryCallback` | OS | 60 | 78.666664 | 100.000000 | H02 |
| `GetCurrentDisplayPosition` | vi | 60 | 79.533330 | 100.000000 | H02 |
| `GXSetFieldMask` | GXPixel | 56 | 83.571430 | 100.000000 | H02 |
| `bgnd_kill_fx` | bgnd | 48 | 95.000000 | 100.000000 | H03 |
| `GXSetClipMode` | GXTransform | 40 | 76.500000 | 100.000000 | H02 |
| `cbForCancelSync` | dvd | 36 | 82.222220 | 100.000000 | M16 |
| `cbForReadSync` | dvdfs | 36 | 82.222220 | 100.000000 | M16 |
| `__GXSetGenMode` | GXGeometry | 36 | 71.111115 | 100.000000 | H02 |
| `GXPixModeSync` | GXMisc | 36 | 71.111115 | 100.000000 | H02 |
| `is_drone` | jmt | 36 | 75.555560 | 100.000000 | M01 |
| `DVDLowClearCallback` | dvdlow | 28 | 77.000000 | 100.000000 | H02 |
| `__VMSetARAMPageAsDirty` | VMMapping | 24 | 99.166664 | 100.000000 | H04 |
| `SISetCommand` | SIBios | 20 | 55.000000 | 100.000000 | H02 |
| `__VMIsARAMPageDirty` | VMMapping | 20 | 99.000000 | 100.000000 | H04 |
| `SFPL2_Standby` | sfd_pl2 | 16 | 99.750000 | 100.000000 | H02 |
| `SITransferCommands` | SIBios | 16 | 97.000000 | 100.000000 | H02 |

### Verification and scope

Full Ninja, explicit DTK SHA-1, progress generation and `git diff --check` passed.
The rebuilt DOL passed SHA-1 `ef001212c6cd4da3194c041d24956a61bc5e9fcd`.
All 41 winning units remain NonMatching: these 113 results are verified source
object matches, not a claim that those objects replaced retail fallbacks in the
linked DOL. The three earlier MPVABDEC functions remain ordinary/data exact with
zero diffs at 17,560, 16,944 and 16,904 bytes; their existing source-linked status
is preserved.

Final project report: 7,802 / 12,227 exact functions, 1,007,048 / 3,042,852 exact
code bytes (33.10%), 84.03% fuzzy and 7.08% linked. Concurrent changes mean this
global delta is not campaign attribution. The sole global baseline-exact loss
was unrelated `_pfx_emitter_compile` (292 bytes, 100→98.41096); it was preserved
and excluded from owned regression counts. No user work was reset or committed
by the campaign agents.

The linker retained its existing runtime-source-version warning and extabindex
placement warning. Neither prevented the retail SHA gate. No full-frame Mirage
artifact claim or new PC-port runtime claim follows from these object matches.

Evidence SHA-256 values (raw temporary captures were synthesized before cleanup):

- `baseline-report.json`: `6238504271a4398d074bf9b939ef1d38887581d65714058fc6645f1626affc70`
- `final-report.json`: `ec5ea19f774ea5a08864c78c87e9e8bfa3bb7e650f19e520ced8003f7d792cb4`
- `final-wins.json`: `17a7caf6be7cf44a09adf95fe91e19e61621cac3e5b2c52266de450241b4ad9d`
- `ledger.jsonl`: `fca3b5f5eb6b2e1954d67eee818b3eb2023440fa16b1a0599b6c4fdd61864b4b`
- `final-anonymous-constant-proof.json`: `8b3ba043cc27c0f1cfc5df140d0585c08f904867f7b961f396f64e5d86142588`
- `final-hashes.json`: `630ce98e13f421d1b826f7f3cb5a4cbb908880ed2a0864b2bd6d094bedb9afee`

Winning source-object hashes at final verification, with full unit identity to
disambiguate short names. Regenerate objects using the current `objdiff.json`
compiler command; a hash is an identity check, not a substitute for objdiff.

| Unit | Candidate object SHA-256 |
| --- | --- |
| main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Export/mslsupp | `61244fec3835441ea86e4ab58e5414ff9273e30e4e84a1c43f1887571fc03b16` |
| main/bgnd | `cb789682fe933e010a6a8ad7a6962715c6bedec61c6c5efa0507919650069d2f` |
| main/bgnd_mab | `401f2640b904da0ce174cc1cb873ebbc9c38fc22bfdea44081cb5704889706fc` |
| main/cam | `ab5aee29b2bc71449fc35ac4cb677e0090d887c5adadf1a0e845fd05004a7940` |
| main/dvd.a/dvd | `c2ed0008b6a9440496bdb607b4816cc30d69861b3304e342cb30dd6bd55cce36` |
| main/dvd.a/dvdfs | `fe1332b469c408112ba95544ff2877562aeedb7b8a1908d1c61e07f4df91b7df` |
| main/dvd.a/dvdlow | `69288c0f5351b2fc50dd9ad9e6096ecace66c381616657d667fe6bfc4ca889c8` |
| main/ejb | `122406e5d6c06454048c5f1de2d6f1c850b4f5c53a4958f73fc086f032cc3c12` |
| main/exi.a/EXIBios | `48484295b5e414edf5ad4df60bae065d512fe0ea4191427b5116c2fda5fdede4` |
| main/gx.a/GXAttr | `6c1c7e725a3cbe943c46347994d8470ea77eb1cfcfb39d67b57ffd4cb7eb80b8` |
| main/gx.a/GXFrameBuf | `29728f9fdfa819be63ca8dd27bb45e3e62a3ba1adb7a32fb65e512101aea163a` |
| main/gx.a/GXGeometry | `899e769d2a1c9907eebd941dfdf8c92ef17f8958db017d2b6db715e55961344b` |
| main/gx.a/GXLight | `affe766f1b24e6d8ef960284a58d7689280c84bb37463df30130be475ac50865` |
| main/gx.a/GXMisc | `cef7a11f1430147f561a7999582df256e7e319424cce1eec9ccdb74e33eb20e9` |
| main/gx.a/GXPixel | `cb65d375f7d1ed10254eef7ac3ef2dee91f9efe8022eaf7b0c5b59d657daacc4` |
| main/gx.a/GXTev | `ea0198a521a9b97e70db0ab27b624abdf56a9a0e86bb2f528ee7201ef66bfb80` |
| main/gx.a/GXTransform | `da769d4b632055a721712c0c2e9af384c66219bce445df2b1ec31f79b5594400` |
| main/jab | `722bffd2fc080f8755fb54de0cf2c04c4ae6cd27957f0e812f22f62d3c428fa0` |
| main/jmt | `88be14d16aec1c889e3938b9b4735d141e72c8eb8a6b22dec90937d943999dd7` |
| main/joy | `855b345f6edac13e9419bae00bec3712bd1ecc8124f078019ccb4bfacb3236ae` |
| main/konquest | `225a649b80944aa5f462925fe7e532221b5716dfa5c995bb0cd2b978dd61b3d6` |
| main/konquest_nav | `43ea1e5ad622a25e5d18a471b4ed2c1be09a31745e51c77cf88522f8892fb5b0` |
| main/libadxgca.a/crimw/dev/adx/src/adxt/adx_bsps | `66122c17d610f5031bc68cc98160b724929c3f3dfd00cdf9aba3f2c6a79ab366` |
| main/libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/vm_spawn | `ae3c82b9ef131cf4e41f754118c38b0b9ec05099cf889758b53ffc78c08b7b6e` |
| main/libmwsfdg.a/crimw/dev/sofdec/src/mwply/mwsfdsst | `e7646426bb77cd269d657c1e60c02698084db53de0512b1786fa2017e1e2fd8f` |
| main/libmwsfdg.a/crimw/dev/sofdec/src/sfdcore/sfd/sfd_pl2 | `c425b0e660f738092d6fd721f9ca3b8b03f0688f9064c9c6dbf6267ff54bc90e` |
| main/mk_render | `f252cb7be1ac80dd8f66115033252faa953b4f08c6a584d0ad2f9708af064051` |
| main/moves | `681f9b19f95c82efd17d336b2b2e55518317555a273bb2541b9594eebb2bc99d` |
| main/mwMem | `b8dfb44280a59e0e6a8562c67115936671ea94c3f80d26af8b384709e3b6d72b` |
| main/os.a/OS | `95296d50d344eac36aad608bb4e17ebcdef29b611fdc9979ae6f3e4eaf533c40` |
| main/os.a/OSError | `50a81accfc5b26ba0eb6caf420f203e15de3d7c6de0b089b45bed68f3ac652a8` |
| main/os.a/OSFont | `2a8446687f83a8ced8ef35f687d9423afa63f6fd2cf931856548fd7564736d9b` |
| main/os.a/OSReset | `cd2bb5ed0b820ae4585991095fd0a2595805b2138ae64008d394246f10fe80c7` |
| main/pad.a/Pad | `8ed9e46051522ee19d5c3f58dce52cfa68283f76b948118479504e19a1e4e2b1` |
| main/pz_fighters | `6b98157fbc3c73098ec0767ed3fc9c694ff7cfff6b191c41bc5f578f43373602` |
| main/rpworld.a/baclump | `448a1ae64bb0af7b250ca8844ad1b57ad6ddb312a278d87b34f05f13b9f47da0` |
| main/rwcore.a/dlbrkpt | `1887a028b5d85d45055be6062d9228f2838af7264d54b938a8c60c625ed8a3c7` |
| main/si.a/SIBios | `2db49bb9bb0ff30065d4db83b811b2c2035e61b33e13b6a24803176438af81df` |
| main/utils | `0674f1332fdd6c8fdcbdc0cc0dcf6c62e8e01a27a9c78329d680350c370ab280` |
| main/vi.a/vi | `ba188223633c8f7fa535fa08bccfaa2c1152c747711bf1ca637a4febcbf12a36` |
| main/vm.a/VMMapping | `d94469b232c61aa9df615ee41e4eb788eb1e12f769bafe6f4846080ec2f6adfa` |

### Retained nonexact tradeoffs

These 13 score decreases accompanied evidence-backed owner/ABI or compiler-mode
corrections. They remain nonmatches and count neither as wins nor as baseline
exact losses. GXLoadLightObjImm also retained a zero score with more difference
records; its assembly-dependent implementation was excluded. All numbers below
use the same data-value comparison before and after.

| Function | Before % | Retained % | Difference records before → after |
| --- | ---: | ---: | ---: |
| `EXIImm` | 44.271523 | 43.947020 | 148 → 148 |
| `EXIDma` | 68.508480 | 67.661020 | 35 → 30 |
| `EXISelect` | 76.360000 | 69.186670 | 26 → 34 |
| `EXIDeselect` | 83.132355 | 80.514710 | 28 → 15 |
| `EXIIntrruptHandler` | 40.060000 | 37.200000 | 40 → 42 |
| `GXSetTevIndirect` | 91.111115 | 72.962960 | 10 → 17 |
| `GXSetCopyClear` | 60.566666 | 48.900000 | 30 → 31 |
| `GXLoadLightObjImm` | 0.000000 | 0.000000 | 47 → 55 |
| `kabal_collide_victim` | 87.111115 | 83.155556 | 16 → 22 |
| `x_attack_5` | 83.531250 | 82.822914 | 103 → 103 |
| `x_attack_5_remote` | 79.651985 | 78.889870 | 88 → 94 |
| `OSInit` | 77.144230 | 77.128204 | 121 → 123 |
| `OSGetFontTexture` | 77.010870 | 76.532610 | 65 → 64 |
| `__OSDispatchInterrupt` | 71.555020 | 70.923450 | 136 → 136 |

### Anonymous constant payload evidence

The six ordinary-label exceptions contain 33 mismatch records, including
relocation-carrying loads. The parent independently decoded both sides' objdiff
symbol data and verified equal raw payload bytes for all 33 records. Unique
symbol pairs and their common bytes are retained here; no labels were forced.

| Function | Retail symbol | Source symbol | Equal payload, hexadecimal |
| --- | --- | --- | --- |
| `init_scripted_camera` | `@2443` | `@3728` | `000000000000000000000000` |
| `player_area_collision_ticks` | `@1455` | `@88` | `00000000` |
| `player_area_collision_ticks` | `@1456` | `@89` | `3f800000` |
| `p_joy_entry` | `@431` | `@86` | `3f800000` |
| `p_joy_entry` | `@432` | `@11` | `00000000` |
| `p_block` | `@3018` | `@38` | `bf800000` |
| `p_block` | `@2945` | `@41` | `00000000` |
| `x_pickup` | `@3550` | `@522` | `4039999a` |
| `x_pickup` | `@2945` | `@41` | `00000000` |
| `x_pickup` | `@3344` | `@523` | `3fc00000` |
| `p_plyr_pz_fighter_start` | `@2691` | `@333` | `3f800000` |
| `p_plyr_pz_fighter_start` | `@2689` | `@331` | `00000000` |

## Consolidation and cleanup

Consolidated four 30-function reports, both early MPVABDEC rounds, the completed
MPVABDEC goal with its superseded checkpoints, the nine-candidate follow-up and
the gallery/Sofdec retrofeed. Removed 1,716 temporary files (658.1 MiB): intermediate JSON evidence, the
consolidated reports, `debug_source.c`, and old mpv-goal, mpv-followup, m2c
and permuter scratches.
Also removed the inactive `/tmp/mkd30c` campaign scratch (6,150 files,
14,066,142,132 bytes); no active process referenced it.
The source, operational books, retail inputs and reusable runtime tools remain
reproducible. The old MPVABDEC attempts 1–66 had already lost some raw scratch
history in an earlier environment reset; this document does not restore it.

The completed effectiveness campaign scratch was then removed: 1,394 files,
1,452,011,803 bytes, plus 3 parent scratch files (14,941 bytes) under `/tmp`.
All 113 winners, 77 trial outcomes, rankings, constant payload evidence, retained
regressions and verification hashes were synthesized above before removal. No
matching worker referenced the scratch at cleanup.

Total cleanup: 9,263 temporary files, 16,208,191,201 bytes (about 16.2 GB).
The parent reran Ninja, the retail SHA check, progress and diff checks after
cleanup; they passed with the same final report totals.

Preserve unrelated `asm.md` and the tracked settings integration report. `asm.md` records assembly provenance and specific historical
authorizations; it is not a temporary matching diary and grants no new permission.
