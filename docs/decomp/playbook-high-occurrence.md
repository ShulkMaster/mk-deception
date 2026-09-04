# Matching rules: high occurrence

## Protocol (all three books)

Authority: AGENTS.md > retail ASM/callers/ELF/layout > decompiler/foreign port.
Route: structural error -> H; localized residue -> M; rare/stop -> N.
Rule schema: ID | IF mismatch | REQUIRE evidence | TRY one change.
Missing precondition -> skip. No applicable rule -> investigate or stop.

Loop: baseline -> classify -> one rule -> rebuild -> same-symbol objdiff ->
accept/revert -> update source TODO: [status] explanation per AGENTS.md.
Respect task budget. Recheck every shared-helper/header consumer, including
below-threshold and already-exact functions. Never stack speculative edits.
No forced registers, fake volatile, dead sinks, empty arms, invented fields,
wrong ABI, undefined returns, goto, or assembly workaround. Real MMIO stays volatile.
Preserve observed math, stores, call order, widths, and lazy null checks.
Inspect immediates/offsets even above 99%; scores do not prove behavior.
Large score swing -> compare local-before/local-after for diff-alignment artifacts.
Finish: quality pass + full ninja + SHA-1 + report + diff-check + status.
Report-exact != whole-TU/link-exact; verify constant payloads and disclose fallback.

## Rules (structural first)

H01 | Wrong argument/return registers | All callers + callee ABI | Correct declarations, definitions, callbacks, calls together; typed T*/T** over opaque pointers. No invented unused return.
H02 | Wrong offset/width | Multiple accesses + allocation size | Recover canonical fields/arrays inside proven layout; remove false extensions; preserve unknown gaps.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct signedness/storage width; keep promoted accumulators full-width. m2c u16 result does not prove u16 accumulator.
H04 | Bit extraction/RMW differs | Storage width + bit position | Existing bitfield or proven mask; unsigned-byte promotion before shift. Unknown semantics -> neutral bit name.
H05 | Cached value vs retail reload | Reload after call/sleep/aliasing store | Read authoritative owner again; do not extend stale local lifetime.
H06 | Retail retains computed value/address | Shared uses with no reload | Name genuine typed local/element/owner; retain only for observed interval.
H07 | Extra/missing helper call | Retail bl boundary + signature | Visible inline body for expansion; out-of-line body for real call. Repeated inline-off idiom -> authentic side-effect-safe typed macro.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads; no bl | Typed accessor returning pointer on success, null otherwise; pass owner if preloaded arguments hoist reads. No empty keep arm.
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition. Rotated top test -> explicit top guard/break. No dummy one-trip loop.
H10 | Switch dispatch differs | Complete cases/default/fallthrough + text order | Recover switch/case order; preserve independent guards where retail repeats tests. No speculative labels.
H11 | Return/cleanup join differs | Branch graph + effect ownership | Shared result/epilogue or explicit arm returns as observed. Shared zero return may avoid booleanization; cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. No compiler scaffolding.
H13 | Intrusive-list loads/stores differ | Link ownership + callback effects + no bl | Exact typed reciprocal-store order; advance iterator before mutating callback when observed; reload links as retail does.
H14 | Stack slots differ | Genuine address-taken locals + offsets | Reorder adjacent declarations/whole aggregates or narrow lifetime. Distinguish compiler-created by-value copies. No padding locals.
H15 | Coloring only | Same operations/CFG/memory accesses | At most one honest lifetime/declaration check, then N stop. No register carousel.
H16 | Producer/consumer move differs | Real returned object + consumer ABI | Nest single-use result; retain original callback owner at untyped boundary if observed. No manufactured return contract.

## Known traps

- addis x,v,H; cmplwi x,L tests (L - (H << 16)) mod 2^32. H=0,L=0xC602 is positive 0xC602; preserve proven source signedness.
- AnimPdata landing fields: +0xF8/+0xFC inside 0x104 bytes, not an extension at +0x104/+0x108.
- Runtime owner fields are not interchangeable with similar static tables.
- Mirage/m2c/permuter output is a hypothesis; GC evidence governs acceptance. Host-only branches stay out of src.

No structural discrepancy -> [mid](playbook-mid-occurrence.md).
