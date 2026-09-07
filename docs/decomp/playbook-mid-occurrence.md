# Matching rules: mid occurrence

Prerequisite: [high protocol](playbook-high-occurrence.md#protocol-all-three-books);
ABI/CFG/layout understood. Schema: ID | IF mismatch | REQUIRE evidence | TRY.
Select by mismatch, not historical score; keep attempt history in the
[consolidated knowledge record](matching-knowledge.md).

Measured priority within applicable mid-level diagnoses: M01 for a repeated TU
compiler-mode signature, then M16 for proven storage/SDA evidence. M03/M04 spelling
trials yielded no new closures in this sample; try only a concrete new hypothesis
and retain the stop rule. This is a selected sample, not a universal success rate;
see the [full ranking](matching-knowledge.md#measured-rule-ranking).

## Compiler / source lowering

M01 | Repeated compact saves/divw/boolean lowering across TU | Sibling evidence + all-function/section baselines | Test object-wide -O4,s with existing -use_lmw_stmw; test scheduling separately. Recheck legacy pragmas for redundancy via local-to-local section/function equivalence. Keep accepted TU flags fixed. For isolated folded indexed accesses, require the retail category-load/shift and table-base order before testing real category/base locals with consumer-scoped opt_propagation off. A pragma alone may do nothing; measure each source stage, remove the setting as a necessity control, reset it after the consumer, and compare every helper consumer (see [AI round 10](ai-matching-campaign.md#tenth-round-findings-and-checkpoint)). For a constant FP load moved across an owner-selection branch, test propagation separately from instruction scheduling: the AI avoidance reset matches with consumer-scoped propagation off after scheduling and CSE controls failed ([round 26](ai-matching-campaign.md#twenty-sixth-round-findings-and-checkpoint)). Require unchanged branches, selected owner and store; retain the setting only after the same source without it is measured. Do not scatter settings without that evidence.
M02 | Control-word/publication order differs | Retail accesses + alias boundaries | Load control word before subfield writes; publish owners at the observed point; reload counts after aliasing stores. Treat process/script transfers as observers of published state: reject moving an attack-pointer store past the transfer even if the permuter score improves ([AI transfer-order control](ai-matching-campaign.md#ninetieth-round-findings-and-checkpoint)). In bounded candidate loops, check whether each trial is published before validation calls and whether the final budget check overrides an accepted trial; preserve both orders and compare call-time state ([chess setup evidence](mk-chess-body-recovery.md)).
M03 | FP operands/schedule differ | Same math/grouping/rounding contract | Swap only proven commutative operands or name genuine factors. Preserve polynomial-before-sqrt order and real weight updates; remove redundant temporaries only without reassociation. For a retail rounded product followed by addition, try an explicit float conversion of that product and verify emitted instructions plus output bits; camera offsets can differ by ULPs despite identical formulas. For mixed fused/separate operations, verify emitted instructions after narrow function-scoped fp_contract control; lexical toggles inside a function may have no effect. Use an explicit fused operation only where retail proves it, and reset compiler mode afterward. Verify threshold constant width from lfs/lfd and pool bytes; promoting a float to compare against a double literal can differ at the nearest float boundary. Do not trust a decompiler cast annotation over the load width. Check whether negation occurs in FP before conversion or on the converted integer; preserve the observed order. A higher fuzzy score does not justify changed rounding.
M04 | Compare boundary/boolean diamond differs | Equivalent bounds + operand purity | Equivalent threshold spelling; bitwise boolean only when both evaluations are required; ternary/guarded assignment for observed join. Try reversed pure equality operands once; do not invert correct siblings or ordered comparisons.
M05 | Integer-to-float scaffold | Proven signedness/precision | Natural cast; remove fake 0x4330 volatile machinery. Genuine bit reinterpretation can use a supported typed union.
M06 | Leading stack byte updated, whole word passed | GC big-endian layout + callee flags + initialization | Byte-bitfield/word union: init_pwr_bars flip word is 0x20000000, not integer 0x20.
M07 | Aggregate/packed address differs | Stride/extent/ownership/access width | Typed element/subobject pointer; trailing table at &items[count], payload after complete header via header+1. Keep runtime plugin offsets localized; distinguish byte offsets from C element indices.
M08 | Failure edges bypass shared success test | Dominators + exactly-once cleanup; ordinary guards failed | Structured do/break region only for genuine cleanup edges, or a real allocation-result boolean. No dummy status, goto, forced one-trip for, or duplicated effects; otherwise stop.
M09 | Repeated tests around stores | Observed tests + intervening effects | Separate guards; nullable-list inline predicate only for a proven contract, not direct &global != 0 residue.

## ABI / abstractions

M10 | Unexpected masks/pairs/argument copies | All callers + callee storage/return ABI | Recover full-width/byte ABI, typed conditional arms, aligned u64 pairs, by-value POD, or actual POD return. Low r4 return can be low u64 half; randu0 alone does not prove accumulator width.
M11 | Wrapper load order differs with correct registers | PPC independent GPR/FPR streams + callers | Recover float/integer parameter interleaving without changing register ABI. Static/member twins require mangling/call evidence.
M12 | Alias analysis changes access schedule | Actual mutability/ownership/callers | Restore supported const or remove unsupported const; const alone does not establish nonaliasing.
M13 | Canonical macro/inline expansion missing | Definition + repeated retail expansion | Typed macro with genuine result/lvalue and per-expansion locals. Shared header only for proven ownership; O0 unused parameter -> pragma unused, not wrong prototype.
M14 | Varargs setup differs | EABI va_list + variadic callers | Exact MWCC va_list/builtin setup; crclr supports a variadic call, not arbitrary prototype guesses.

## Object / link layout

M15 | String/constant identity, placement, or extent differs | ELF sizes + actual bytes + relocation addends + use | Pooled literals for anonymous pools, named objects for real symbols, natural string bounds. Recover missing real tables/literals and initializer order; no padding strings or bundled unrelated constants.
M16 | Global/zero-fill order differs; SHA fails at report-100 | Raw offsets/alignment + SDA relocations | Recover definition/initialization order and actual alignment. Separate declarations from placement when needed; tentatives can follow reverse/first-use order. Try explicit zero initializers in proven symbol order. No fabricated aggregate/padding.
M17 | Vtables/weak destructors differ, including link-only | ELF relocations + hierarchy + weak owner + sizes/order | Correct declarations/zero slots and inline-visible real destructors; verify weak emission/COMDAT selection with linked SHA. Do not duplicate a deleting call's null guard; check newly emitted destructors too.

## Focused diagnostics

- M01: When compact saves and indexed-loop lowering recur together, test the
  object-wide compact profile (`-O4,s`, `-use_lmw_stmw on`) before changing loop
  source. Attribute a combined trial to that profile, not either flag alone.
  Inspect every function, retain prior exact matches, and disclose incomplete
  siblings that regress. The [mk_chess audit](mk-chess-five-passes.md) closed
  four functions this way after correcting the restore caller's false argument.
- M06/M07: If packed storage is read with lbz, lhz and lwz at the same base,
  require confirmed alignment, extent and endian contract before adding a typed
  byte/halfword/word union. Preserve each consumer's original width and masks;
  byte assembly helpers can obscure native-width retail loads. Validate layout
  with MWCC and exercise packed-field combinations against retail execution.
- M16: Before changing a small object's storage, check whether its first extern
  declaration sees the complete canonical type. The eight-byte DVD thread queue
  needed its real type header before that declaration for MWCC's small-data
  addressing. Correct the shared declaration and rebuild every header consumer;
  do not fabricate a smaller type or force a section. Conversely, declaring a
  real multi-record collision table as a single pointer falsely selected SDA
  addressing in mk_chess; recover its proven record type and extent.
- H07/M13: An `inline` helper can still emit a call under the actual TU settings.
  If a canonical typed field-access macro already expresses the same proven
  owner, use it locally before changing global inline settings. The RenderWare
  default atomic callback required direct pipeline-global lookup; audit the
  macro's evaluation count and resulting load offsets.
- M07: Check canonical fields before coloring: PUI drop_timer is +0x34,
  lifetime +0x38. Retail `lhzx` uses a byte offset: a u16 table indexed by
  `(bits >> 10) & 0x3FFE` needs C element index `(bits >> 11) & 0x1FFF`.
- H14/M12: Correct stack addresses can still have reversed initialization
  stores. Separate declarations from assignments while preserving call slots;
  scope the endpoint only when its actual lifetime permits (`run_camera_script`).
- M08: After callback-containing allocation loops, index == count does not
  prove success if callbacks mutate count. Keep the explicit result correcting
  `mslInit`'s unsupported invariant despite a measured fuzzy decrease; stop at
  the remaining flag-test residue rather than restoring the behavioral defect.
- M15: Run `objdiff-cli diff` with `-c functionRelocDiffs=data_value` and inspect
  each constant's use. Report-100 can conceal wrong floats or diagnostic text.
  Compare symbol extent, alignment, addends, literal termination, and whole pools.
  Identical used bytes do not make a CLI-reported named-pool mismatch exact;
  disclose both raw-byte evidence and the actual comparison result.
- M15: IF a generated text pool initializes plain `char` with integers above
  127, REQUIRE the exact retail bytes and terminators; TRY adjacent string
  literals with fixed-width octal escapes for encoded bytes. Preserve embedded
  NULs and the final implicit terminator. Keep a separately evidenced split gap
  outside the string object's extent; compare every byte, table relocation and
  linked SHA. NBC retained 100% with signed- and unsigned-char host checks; see
  [the quality report](nbc-quality-pass.md).
- M15: Missing prefixes shared by many consumers can indicate a missing real
  owning table (`global_background_data`), not padding. Recover table entries
  and used literals, then compare the complete pool and every consumer. Keep
  report-to-report and standalone-to-standalone baselines; scores can differ.
- M16: Distinguish split gaps from object alignment: g_DSB_Buffers needs 32-byte
  alignment. If trailing definitions lose pooling, explicit zero initializers
  before users can preserve symbol order; tentative order alone may not.
  Verify the emitted section: MWCC can place an explicitly zero-initialized
  aggregate in .data, disrupting merged BSS addressing (SVM). Scalar results
  do not justify applying the same initializer form to every aggregate.
- M13: For quantization, name the real quantizer-scaled level inside each macro
  expansion; generic block-level and constant fast paths can lower differently.
  A discarded marker can use the general consume/refill path rather than a
  flag-reading macro; follow the actual branch and shift sequence. Decode a
  field into a real local when retail publishes it once at the common join.
  For VLC siblings, compare corresponding guarded peek/extract/refill expansions,
  not just whole-function fuzzy; preserve sign bits and lookahead until last use.
- M07: Execute retail table initialization before comparing decoder lookups.
  Sofdec run/level tables 4, 2, 1 are biased by -16, -32, -32 bytes. Relocate
  the candidate object and compare coefficients, reader state, return fields,
  and guards; DC prediction and AC decoding need independent coverage.

## Accept / stop

One lever -> rebuild -> same diff. Revert failed hypotheses, or retain an
independently justified correctness/quality correction and disclose its measured
delta. Recheck every shared consumer. No rule fits -> [niche](playbook-niche.md).
Amend one existing rule with its precondition/action; no campaign narrative,
duplicate row, occurrence rating, or unverified recommendation.
