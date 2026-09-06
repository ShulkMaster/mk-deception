# Matching rules: mid occurrence

Prerequisite: [high protocol](playbook-high-occurrence.md#protocol-all-three-books);
ABI/CFG/layout understood. Schema: ID | IF mismatch | REQUIRE evidence | TRY.
Select by mismatch, not historical score; keep attempt history in the
[consolidated knowledge record](matching-knowledge.md).

## Compiler / source lowering

M01 | Repeated compact saves/divw/boolean lowering across TU | Sibling evidence + all-function/section baselines | Test object-wide -O4,s with existing -use_lmw_stmw; test scheduling separately. Recheck legacy pragmas for redundancy via local-to-local section/function equivalence. Keep accepted flags fixed; no scattered optimization pragmas.
M02 | Control-word/publication order differs | Retail accesses + alias boundaries | Load control word before subfield writes; publish owners at the observed point; reload counts after aliasing stores.
M03 | FP operands/schedule differ | Same math/grouping/rounding contract | Swap only proven commutative operands or name genuine factors. Preserve polynomial-before-sqrt order and real weight updates; remove redundant temporaries only without reassociation.
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
