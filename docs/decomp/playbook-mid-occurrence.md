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

M01 | Repeated compact saves/divw/boolean lowering across TU | Sibling evidence + all-function/section baselines | Test object-wide -O4,s with existing -use_lmw_stmw; test scheduling separately. For signed division by two lowered as srawi/addze in retail but sign-bit/add/shift in source, test function-scoped optimize_for_size alone with the arithmetic unchanged; the NIS skip prompt confirms this case. Reset afterward and preserve sibling scores. For an isolated integer-register save mismatch with an otherwise identical body, test function-scoped optimize_for_size and use_lmw_stmw together, resetting both afterward. The projectile sound setter needs both: either setting alone retains individual saves. Require whole-unit equivalence outside the consumer and do not infer retail pragma spelling. If typed scalar vector copies preload later components while retail interleaves each load/store and owner reload, test consumer-scoped scheduling off and reset afterward; projectile target-position assignment closes this way while optimize_for_size alone is neutral. Its damping setter recovers the same copies but retains a single li/load ordering difference in the preceding flag update; stop there instead of inventing a flag temporary. The upward-attack consumer closes its body but reverses two epilogue restore loads under this setting; compare the complete function and retain that ceiling rather than counting a body-only match. Require unchanged typed stores and full-object consumer checks; reject the setting if aligned-frame setup or other regions regress despite improved copies. Do not infer a retail pragma spelling from the result. Recheck legacy pragmas for redundancy via local-to-local section/function equivalence. Keep accepted TU flags fixed. For isolated folded indexed accesses, require the retail category-load/shift and table-base order before testing real category/base locals with consumer-scoped opt_propagation off. The move-list getter confirms this for a style-index multiply and owner-relative count load: both a real index/owner local and the override are needed. The same diagnosis applies when typed owner-relative loads or stores fold into lwzx/stwx: require the retail multiply/add/lwz-or-stw sequence and a same-source enabled control. Board-game stage/focus getters and the stage setter independently confirm both directions; keep the setting scoped to measured consumers. Preserve a fresh index read after an aliased output store; reusing the first style pointer would change that contract. A pragma alone may do nothing; measure each source stage, remove the setting as a necessity control, reset it after the consumer, and compare every helper consumer (see [AI round 10](../../.agent-work/decomp/ai-matching/README.md#tenth-round-findings-and-checkpoint)). For a constant FP load or pooled-string address moved across a selection branch, test propagation separately from instruction scheduling: the AI avoidance reset matches with consumer-scoped propagation off after scheduling and CSE controls failed ([round 26](../../.agent-work/decomp/ai-matching/README.md#twenty-sixth-round-findings-and-checkpoint)). Require unchanged branches, selected owner and store; retain the setting only after the same source without it is measured. The Puzzle burn controller independently closes five string-address hoists across screen-width coordinate selection; preserve all selected coordinates, sound/effect calls and float association, and verify the complete unit ([evidence](puzzle-five-attempts.md#round-4-burn-controller-string-scheduling)). For a multi-row field where retail adds the field offset to the reloaded owner before stfsx, test a typed pointer to the actual row array with scoped propagation off. Three Puzzle crusher resets require both: the setting alone is neutral and the pointer alone restores the baseline. Preserve owner reloads after hazard stores; do not cache the controller across the loop. Stop at remaining address-register or zero-reuse differences. Do not scatter settings without that evidence.
M02 | Control-word/publication order differs | Retail accesses + alias boundaries | Load control word before subfield writes; publish owners at the observed point; reload counts after aliasing stores. Treat process/script transfers as observers of published state: reject moving an attack-pointer store past the transfer even if the permuter score improves ([AI transfer-order control](../../.agent-work/decomp/ai-matching/README.md#ninetieth-round-findings-and-checkpoint)). In bounded candidate loops, check whether each trial is published before validation calls and whether the final budget check overrides an accepted trial; preserve both orders and compare call-time state ([chess setup evidence](mk-chess-body-recovery.md)).
M03 | FP operands/schedule differ | Same math/grouping/rounding contract | Swap only proven commutative operands or name genuine factors. Preserve polynomial-before-sqrt order and real weight updates; remove redundant temporaries only without reassociation. When retail keeps the two genuine pre-sqrt factors, the square-root result, and the final quotient at separate single-precision boundaries, name those intermediates in evaluation order instead of nesting the expression; `ADX_GetCoefficient` closes its final FPR mismatches with the donor's `a`/`b`/`d`/`c` staging while preserving every cast and operation. For a retail rounded product followed by addition, try an explicit float conversion of that product and verify emitted instructions plus output bits; camera offsets can differ by ULPs despite identical formulas. For mixed fused/separate operations, verify emitted instructions after narrow function-scoped fp_contract control; lexical toggles inside a function may have no effect. Use an explicit fused operation only where retail proves it, and reset compiler mode afterward. Verify threshold constant width from lfs/lfd and pool bytes; promoting a float to compare against a double literal can differ at the nearest float boundary. Do not trust a decompiler cast annotation over the load width. Check whether negation occurs in FP before conversion or on the converted integer; preserve the observed order. A folded single-precision expression may differ from its short decimal spelling: Krypt outer-digit spacing is 3.0f * 0.075f (0x3e666667), not 0.225f (0x3e666666). Verify emitted constant bytes at every consumer. A higher fuzzy score does not justify changed rounding.
M04 | Compare boundary/boolean diamond differs | Equivalent bounds + operand purity | Try equivalent thresholds or ternary/guarded assignments for the observed join. Bitwise Boolean evaluation requires both operands to be evaluated. For subic/subfe normalization of a call result, test a Boolean local such as opened = get_coffin_bit(...) != 0; direct conditions may fold it away. For subfic/cntlzw/srwi field queries, test an explicit inline success/failure return. Preserve lazy reads/calls, then verify normalization and joins in every consumer. `SFLIB_CheckHn` demonstrates that a ternary can match standalone yet fold to `cntlzw` when inlined; explicit failure `if` returns preserve retail branches and closed `SFD_SetErrFn`, so compare both the helper and all inlined callers. Krypt key scans and coffin builders establish these forms; the letter builder also shows that restored structure can worsen scheduling/coloring. When identical calls precede different actions in separate retail arms, keep branch-local calls (Krypt dialog acceptance), with exactly one executed call. If a single equality dispatch uses a conditional jump to its case followed by an unconditional default jump, test a one-case switch with default (Krypt gallery thumbnails). Try reversed pure equality operands once; preserve ordered comparisons and correct siblings.
M05 | Integer-to-float scaffold | Proven signedness/precision | Natural cast; remove fake 0x4330 volatile machinery. Genuine bit reinterpretation can use a supported typed union.
M06 | Leading stack byte updated, whole word passed | GC big-endian layout + callee flags + initialization | Byte-bitfield/word union: init_pwr_bars flip word is 0x20000000, not integer 0x20.
M07 | Aggregate/packed address differs | Stride/extent/ownership/access width | Typed element/subobject pointer; trailing table at &items[count], payload after complete header via header+1. Keep runtime plugin offsets localized; distinguish byte offsets from C element indices. If two stack-vector addresses exchange roles, trace each destination through all calls before reordering declarations: retail may update a difference vector in place into a midpoint, then write a separate correction vector. Preserve that ownership and arithmetic/store order; the Puzzle present controller closes this way ([evidence](puzzle-five-attempts.md#round-2-finish-him-abi-and-present-vectors)). When retail establishes a localized-array base before an index getter, assign that real placement pointer first, then advance it by the returned index. Preserve separate getter calls and array extents; Puzzle endgame recovers address setup this way ([evidence](puzzle-five-attempts.md#round-19-exit-reader-and-localization-bases)). If every stack displacement differs by one uniform aligned amount while instructions and accesses otherwise agree, check an adjacent-version source for a real unused local array declared before the live aggregate. Restore only that evidenced declaration, without fake reads or volatility, and require exact retail frame size; Dolphin `__DSPHandler` closes by restoring its four-byte unused byte array before `OSContext`. When retail copies a contiguous run of fields and an adjacent-version type proves that run is one embedded aggregate, recover the nested member and use whole-aggregate assignment; Sofdec's 0x1C-byte ADXT parameter block closes both `SFADXT_Destroy` and `sfadxt_InitInf` without changing offsets.
M08 | Failure edges bypass shared success test | Dominators + exactly-once cleanup; ordinary guards failed | Structured do/break region only for genuine cleanup edges, or a real allocation-result boolean. No dummy status, goto, forced one-trip for, or duplicated effects; otherwise stop.
M09 | Repeated tests around stores | Observed tests + intervening effects | Separate guards; nullable-list inline predicate only for a proven contract, not direct &global != 0 residue.

## ABI / abstractions

M10 | Unexpected masks/pairs/argument copies | All callers + callee storage/return ABI | Recover full-width/byte ABI, typed conditional arms, aligned u64 pairs, by-value POD, or actual POD return. Low r4 return can be low u64 half; randu0 alone does not prove accumulator width.
M11 | Wrapper load order differs with correct registers | PPC independent GPR/FPR streams + callers | Recover float/integer parameter interleaving without changing register ABI. Static/member twins require mangling/call evidence. Keep pointer types correct and apply the recovered order consistently to definitions, declarations and every caller. Script color/fade, texture-animation and bone-matcher wrappers recover exact code this way; local argument snapshots alone may optimize back to the mismatching order. Recheck all callers: the same bone-matcher correction improves three callers but moves one float load in the spear consumer. Do not retain incompatible per-TU declarations to hide that residual.
M12 | Alias analysis changes access schedule | Actual mutability/ownership/callers | Restore supported const or remove unsupported const; const alone does not establish nonaliasing. If coordinate loads move before intervening component stores, compare the input qualifier with mutable caller objects and retail read/store order. Removing an unsupported const from Krypt position inputs restores interleaved loads under the pinned MWCC command; verify callers and do not infer that the callee writes through the input. Puzzle flesh-path velocity and rotation copies independently close after the same correction in both the callee and forwarding helper; require mutable caller vectors and unchanged whole-unit scores ([evidence](puzzle-five-attempts.md#round-5-flesh-path-vector-qualifiers)). Diagnose a separately cached destination owner independently (H05).
M13 | Canonical macro/inline expansion missing | Definition + repeated retail expansion | Typed macro with genuine result/lvalue and per-expansion locals. Shared header only for proven ownership; O0 unused parameter -> pragma unused, not wrong prototype.
M14 | Varargs setup differs | EABI va_list + variadic callers | Exact MWCC va_list/builtin setup; crclr supports a variadic call, not arbitrary prototype guesses. If retail saves the complete GPR/FPR argument area and builds the `va_list` inline but the current SDK macro emits a helper call and smaller frame, scope the compiler-builtin `__builtin_va_info` form to the proven variadic functions. Preserve the public prototype and restore the surrounding macro afterward; `OSPanic` closes from 94.746666% to 100% with the same form already proven by exact `OSReport`.

## Object / link layout

M15 | String/constant identity, placement, or extent differs | ELF sizes + actual bytes + relocation addends + use | Pooled literals for anonymous pools, named objects for real symbols, natural string bounds. Equal pool sizes and unchanged later offsets do not prove correct strings: swapping adjacent effect names can preserve both while selecting the wrong effects. A relocation may name the first aggregate as a section-wide base; check subsequent base-plus-offset loads before declaring other constants unused. Decode each referenced offset from retail bytes before accepting a relocation ceiling; correcting Krypt's swapped coffin/dirt names restores its entire 403-byte pool and removes unrelated named-pool mismatches. An unused inline body can still emit aggregate initializer data. Remove unused code first; when live callers duplicate the same initialized operation, test sharing that operation and verify every expansion. Removing a live initializer is a separate hypothesis and can remove retail instructions. For aggregate order, test immutable owners with unchanged local copies, or early inline implementations retaining the original callable entries. Verify raw bytes, string order and every expansion; remove unnecessary boundary overrides. Recover missing real tables/literals and initializer order; no padding strings or bundled unrelated constants.
M16 | Global/zero-fill order differs; SHA fails at report-100 | Raw offsets/alignment + SDA relocations | Recover definition/initialization order and actual alignment. Separate declarations from placement when needed; tentatives can follow reverse/first-use order. Try explicit zero initializers in proven symbol order. No fabricated aggregate/padding.
M17 | Vtables/weak destructors differ, including link-only | ELF relocations + hierarchy + weak owner + sizes/order | Correct declarations/zero slots and inline-visible real destructors; verify weak emission/COMDAT selection with linked SHA. Do not duplicate a deleting call's null guard; check newly emitted destructors too.

## Focused diagnostics

- M04: IF a returned signed integer comparison differs only in operand load
  homes, REQUIRE pure field reads and identical types; TRY the direct owner
  order (`a > b` versus `b < a`) once. Konquest's final-minute comparison closed
  this way. Do not change signedness, normalize through subtraction, or extend
  the finding to floating-point unordered comparisons. Neutral operand spelling
  trials remain stop evidence, not reasons for a carousel of equivalent forms
  ([gameplay campaign](../../.agent-work/decomp/gameplay-200/README.md#round-2-time-predicates-and-particle-emitter-guards)).
- M01: IF a fixed-count typed array copy becomes two advancing pointers but
  retail uses one byte induction register with indexed FP loads/stores, REQUIRE
  the same array extent, access order and rolled-loop baseline; TRY scoped
  `opt_strength_reduction off`, resetting it after the function. Keep ordinary
  element indexing; do not encode byte offsets and divide them back into indices.
  Compare the identical source with the setting enabled as a necessity control
  and recheck every sibling. This closed the six-channel audio reset; the
  setting's effect does not establish the original retail pragma spelling.
  Do not extend this loop diagnostic to a non-loop index retained across a call:
  the ladder background-query control was neutral
  ([gameplay campaign](../../.agent-work/decomp/gameplay-200/README.md)).
- M01: When compact saves and indexed-loop lowering recur together, test the
  object-wide compact profile (`-O4,s`, `-use_lmw_stmw on`) before changing loop
  source. Attribute a combined trial to that profile, not either flag alone.
  Inspect every function, retain prior exact matches, and disclose incomplete
  siblings that regress. The [mk_chess audit](mk-chess-five-passes.md) closed
  four functions this way after correcting the restore caller's false argument.
  Ending provides an independent control: with compact saves already enabled,
  changing only `-O4,p` to `-O4,s` closes credits text and permits a bounded
  search helper to retain retail's indexed CTR loop. Recover the search
  result/sentinel boundary first; changing `for` to `do/while` alone retained
  an explicit index comparison. Check callers too: an exact callee can still
  have a reconstructed wrapper that drops an argument.
- M06/M07: If packed storage is read with lbz, lhz and lwz at the same base,
  require confirmed alignment, extent and endian contract before adding a typed
  byte/halfword/word union. Preserve each consumer's original width and masks;
  byte assembly helpers can obscure native-width retail loads. Check initialization
  separately: a word clear followed by byte bitfield updates requires a word
  storage view, while existing byte readers must keep their original field.
  Preserve the union extent and every following member offset. Validate layout
  with MWCC and exercise packed-field combinations against retail execution.
- M16: Before changing a small object's storage, check whether its first extern
  declaration sees the complete canonical type. The eight-byte DVD thread queue
  needed its real type header before that declaration for MWCC's small-data
  addressing. Correct the shared declaration and rebuild every header consumer;
  do not fabricate a smaller type or force a section. Conversely, declaring a
  real multi-record collision table as a single pointer falsely selected SDA
  addressing in mk_chess; recover its proven record type and extent. If the ELF
  places a TU's complete set of small globals in `.bss` and retail materializes
  each address with `lis`/low relocations while the candidate emits `.sbss` and
  `@sda21`, test the narrow object flag `-sdata 0` before reshaping source.
  `sfd_tim` closed both start-sample functions and its four data symbols this
  way; require whole-TU comparison because the flag changes every small global.
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
  Trace aggregate copy offsets and each scalar load through to its consumer:
  Puzzle eye launch used wrong Z offsets and copied the left-eye scale to the
  right eye despite a >98% score; the two retail scales intentionally differ.
  A named aggregate comparison alone cannot establish which vector a caller uses.
  Identical used bytes do not make a CLI-reported named-pool mismatch exact;
  disclose both raw-byte evidence and the actual comparison result.
  If differences fall inside reconstructed padding, inspect every record before
  declaring a relocation ceiling. Puzzle reaction dispatch has 57 20-byte records;
  six contain a nonzero word at +8 despite the consumer reading only +0/+4/+16.
  Preserve those initializer words with offset-based names until their meaning
  is established. Verify the full table, relocations and unchanged consumer code;
  a consumer's lack of reads does not prove that stored bytes were zero.
  If an array view is one word shorter than the ELF symbol, derive the first
  element offset separately from its stride. The fatality loader reads +4+i*12
  from a 184-byte owner: recover a header plus fifteen records, keeping unknown
  header/trailing fields offset-named. Verify all initializer bytes and the
  unchanged loader; do not append an invented padding word.
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
