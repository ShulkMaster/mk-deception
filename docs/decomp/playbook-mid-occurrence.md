# Matching rules: mid occurrence

Prerequisite: [high protocol](playbook-high-occurrence.md); ABI/CFG/layout understood.
Select by mismatch, not historical score. Schema: ID | IF | REQUIRE | TRY.

## Compiler / source lowering

M01 | Repeated compact saves/divw/boolean lowering across TU | Sibling evidence + all-function/section baselines | Test object-wide -O4,s with existing -use_lmw_stmw. Re-test legacy pragmas after flag changes: mk_anim's unroll/limit/dont_inline controls became redundant under existing TU settings; require local-to-local section and all-function equivalence before removal. TU-wide scheduling discrepancy -> separate schedule test. Keep accepted flags fixed during refinement; no scattered optimization pragmas.
M02 | Control-word/publication order differs | Retail loads/stores + alias boundaries | Load control word before subfield writes; publish owner at observed point; reload counts after aliasing stores.
M03 | FP operands/schedule differ | Same math/grouping/rounding contract | Swap only proven commutative operands or name real shared factors. Never reassociate FP to improve fuzzy.
M04 | Compare boundary/boolean diamond differs | Equivalent bounds + operand purity | Equivalent threshold spelling; bitwise booleans only if both evaluations required; ternary/guarded assignment for observed join. For reversed pointer-equality operands with side-effect-free reads, try retail operand order once: RBT_RemoveNode 98.86652->99.03602 recovered eight cmplw operand pairs without changing size or CFG. Do not invert already-correct sibling comparisons or confuse symmetric equality with ordered comparisons.
M05 | Integer-to-float scaffold | Proven signedness/precision | Natural cast; remove fake 0x4330 volatile machinery. Retain genuine bit reinterpretation via supported typed union.
M06 | Leading stack byte updated, whole word passed | GC big-endian layout + callee flags + initialization | Byte-bitfield/word union. init_pwr_bars flip word is 0x20000000, not integer 0x20.
M07 | Aggregate/packed address differs | Stride/extent/ownership/access width | Typed element/subobject pointer; trailing table at &items[count]; payload after complete header via header+1. Runtime plugin offsets remain localized low-level access.
M08 | Failure edges bypass shared success test | Dominators + exactly-once cleanup; ordinary guards failed | Structured do/break cleanup region only when each break represents real cleanup edge. No dummy status, goto, forced one-trip for, duplicate effects. Otherwise stop.
M09 | Repeated tests around stores | Observed tests + intervening effects | Separate guards; nullable-list inline predicate only for proven nullable contract, not direct &global != 0 residue.

## ABI / abstractions

M10 | Unexpected masks/pairs/argument copies | All callers + callee storage/return ABI | Recover full-width vs byte ABI, typed conditional arms, aligned u64 pairs, by-value POD, or real POD return. Low r4 return may be low u64 half. Do not narrow accumulator from randu0 alone.
M11 | Wrapper load order differs with correct registers | PPC independent GPR/FPR streams + callers | Recover float/integer parameter interleaving without changing register ABI. Static/member twin requires mangling/call evidence.
M12 | Alias analysis changes load/store schedule | Actual mutability/ownership/callers | Restore supported const or remove unsupported const; const alone is not nonalias proof.
M13 | Canonical macro/inline expansion missing | Definition + repeated retail expansion | Restore typed macro including genuine result/lvalue; shared header only for proven ownership. O0 unused parameter -> pragma unused, not wrong prototype.
M14 | Varargs setup differs | EABI va_list + variadic callers | Exact MWCC va_list/builtin setup; crclr supports variadic call, not arbitrary prototype guessing.

## Object / link layout

M15 | String identity/placement differs | ELF sizes + bytes + relocations | Pooled literals for anonymous pools; named objects for real symbols; unsized arrays for terminators. TU string/readonly/SDA flags require sibling evidence. No synthetic padding strings.
M16 | Global/zero-fill order differs; SHA fails at report-100 | Raw target/local offsets, alignment, SDA relocations | Recover initialization/definition order. Tentatives may emit reverse/first-use; extern before users + definitions after separates declaration from placement. Distinguish explicit split gaps from real object alignment: g_DSB_Buffers requires32-byte alignment (.bss+0x7E0, not+0x7C8); restoring the attribute fixes layout and makes mslStreamFile_Initialize100. No fabricated aggregate/padding.
M17 | Vtables/weak destructors differ, including link-only | ELF relocations, hierarchy, weak owner, sizes/order | Correct declarations/zero slots; inline-visible real destructors; verify reverse weak emission/COMDAT selection with linked SHA. A deleting call already includes a null guard: avoid wrapping ordinary delete in a second reconstructed guard. SoundBuffer's three FreeObject methods retain100 with plain delete this. Check newly emitted inline destructors against retail ordering, not just the old candidate.

## Accept / stop

M08 diagnostic: After a callback-containing allocation loop, an index-equals-count
test is not proof of success if callbacks can change the count. Require retail
failure edges that bypass the bounds test. A real allocation-result boolean
preserves that distinction and shared cleanup: `mslInit` measured 99.24821% to
99.009544% while removing the unsupported invariant. This is a correctness
correction, not an exact-match trick. Direct inline cleanup on failure duplicated
code (85.0883%); success inside the loop changed block order (91.357994% or
97.434364%). Stop at the explicit result's localized flag test unless new source
evidence explains the retail shared exit; do not remove the result for fuzzy score.

One lever -> rebuild -> same diff. Failed hypothesis -> revert or retain only
justified quality correction and disclose delta. Recheck every shared consumer.
No rule fits -> [niche](playbook-niche.md), not more flags/speculative locals.
New learning -> amend one rule with precondition + action. No campaign narrative,
duplicate row, occurrence rating, or unverified recommendation.
