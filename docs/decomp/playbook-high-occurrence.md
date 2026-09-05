# Matching rules: high occurrence

## Protocol (all three books)

Authority: AGENTS.md > retail ASM/callers/ELF/layout > decompiler/foreign port.
Route: structural error -> H; localized residue -> M; rare/stop -> N.
Rule schema: ID | IF mismatch | REQUIRE evidence | TRY one change.
Missing precondition -> skip. No applicable rule -> investigate or stop.

Loop: baseline -> classify -> one rule -> rebuild -> same-symbol objdiff ->
accept/revert -> update source TODO: [status] while below100 per AGENTS.md.
Measured100 -> remove all associated matching-progress comments/TODOs; preserve
code explanations and unrelated functional TODOs. Record exactness mode and
verification in reports/metadata, never a replacement matched/100% comment.
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
H02 | Wrong offset/width | Multiple accesses + allocation size | Recover canonical fields/arrays inside proven layout; remove false extensions; preserve unknown gaps. MWCC may place the vptr after members declared before the first virtual method: IRefCntRes requires virtual declarations before reference_count to retain vptr+0/count+4. Verify all virtual-call sites and destructor stores after class-member recovery.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct signedness/storage width; keep promoted accumulators full-width. m2c u16 result does not prove u16 accumulator. Proven byte-range FP interpolation passed to a byte method may require direct float-to-byte conversion, not float-to-int-to-byte: SetRelativePan retained100 with the correct virtual prototype and no extra mask. Require representable input range; do not generalize to modulo integer narrowing.
H04 | Bit extraction/RMW differs | Storage width + bit position | Existing bitfield or proven mask; unsigned-byte promotion before shift. Unknown semantics -> neutral bit name.
H05 | Cached value vs retail reload | Reload after call/sleep/aliasing store, or polling externally updated state | Read authoritative owner again; do not extend stale local lifetime. For a proven debugger/interrupt completion flag, qualify declaration and definition volatile and verify every reader/writer; a branch skipping the poll load is behavioral, even above 99%. Never use volatility for coloring.
H06 | Retail retains computed value/address | Shared uses with no reload | Name genuine typed local/element/owner; retain only for observed interval.
H07 | Extra/missing helper call | Retail bl boundary + signature | Visible inline body for expansion; out-of-line body for real call. Repeated inline-off idiom -> authentic side-effect-safe typed macro.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads; no bl | Typed accessor returning pointer on success, null otherwise; pass owner if preloaded arguments hoist reads. No empty keep arm. Check the caller’s active dont_inline region before extraction: it can emit a real accessor call despite an inline declaration. Preserve that region and retain a direct stale-instance guard there; do not change unrelated inlining to close a latch.
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition. Rotated top test -> explicit top guard/break. No dummy one-trip loop.
H10 | Switch dispatch differs | Complete cases/default/fallthrough + text order | Recover switch/case order; preserve independent guards where retail repeats tests. No speculative labels.
H11 | Return/cleanup join differs | Branch graph + effect ownership | Shared result/epilogue or explicit arm returns as observed. Shared zero return may avoid booleanization; cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. No compiler scaffolding.
H13 | Intrusive-list loads/stores differ | Link ownership + callback effects + no bl | Exact typed reciprocal-store order; advance iterator before mutating callback when observed; reload links as retail does.
H14 | Stack slots differ | Genuine address-taken locals + offsets | Reorder adjacent declarations/whole aggregates or narrow lifetime. Distinguish compiler-created by-value copies. No padding locals.
H15 | Coloring only | Same operations/CFG/memory accesses | At most one honest lifetime/declaration check, then N stop. No register carousel.
H16 | Producer/consumer move differs | Real returned object + consumer ABI | Nest single-use result; retain original callback owner at untyped boundary if observed. No manufactured return contract.

## Measured examples

- H06: If two member arrays share one retail induction pointer but MWCC emits
  two, check whether source accesses one through a base-class alias and the
  other through the derived owner. Require proven same-object identity and
  inherited-member layout; use the derived owner for both. In
  `SBPlayable_Stream::iPlayPrepped`, removing the redundant base alias raised
  97.15205% to 97.974655% and reduced 2072 bytes to 2056 (retail 2052), without
  changing memory accesses or virtual calls. Remaining zero/coloring residue
  is a soft ceiling; do not invent a combined array or byte-stride object walk.
  Moving the secondary-entry initialization past primary setup did not remove
  the extra instruction after this fix: 97.88499%, still 2056 bytes. Do not
  assume narrowing that pointer lifetime closes the zero-initialization residue.

- H02/H10: Mirage menu navigation exposed an irregular-switch default that
  incorrectly selected P2. Recovering the finite event set and retail case order
  raised `mkScreenEngineClient::HandleEvent` from 39.13% to 99.825%. Inspect the
  final immediates: P2 still used +0x108 instead of +0x110. A MWCC C++ layout
  probe showed nested anonymous structs inside `FighterSlot`'s anonymous union
  had size 1. Three anonymous pointer unions preserve the proven aliases and
  restore size 0x0C / `PlyrInfo` 0x6C. Correct C callee linkage then reached 100%.
  Require measured compiler layout and retail offsets before flattening a union.

- H01: `mslgcn.cpp`'s ten stream/static playback wrappers used manual vtable
  calls. Retail tables, method symbols, and callers established typed virtual
  methods. Ordinary virtual dispatch reproduced the retail `r12` call sequence:
  nine wrappers became report-exact, and `ContinueStream` improved from
  99.44444% to 99.81481%. Do not generalize a consumer interface view into an
  unverified concrete class layout or destructor contract.
- H11: `ContinueStream`'s remaining branch executed a redundant return-register
  copy on success. An explicit return in the diagnostic-error arm reproduced
  retail's success branch directly to the epilogue, reaching report-exact 100%.
  Require the same error side effects and return value on both source forms.

- H08: Owner-typed accessors closed 30 report-exact functions across `konquest`,
  `bgnd`, and `ai` (7,864 bytes). Return the validated pointer from the success
  arm and null on stale/null paths; keep the owner-instance read behind the
  pointer check. Empty valid arms and a negated stale check let MWCC fold the
  retail join branch. Whole-object checks preserved all prior exact functions.
  `set_monk_position` and `is_leaving_area` sit under `dont_inline`: extraction
  emitted real calls, so their direct guards were restored. Do not infer that
  an accessor matching one consumer preserves every consumer’s register homes.

## Known traps

- H15 measured check: `mslTick`99.3->100 by declaring processed before the
  snapshot item_count. Only five r28/r29 operands differed; the adjacent scalar
  declarations, not assignments, changed. Callback order, counter initialization,
  signed comparison and interrupt boundaries stayed intact. Try once only when
  this pure-local coloring precondition holds; do not cycle declarations.

- addis x,v,H; cmplwi x,L tests (L - (H << 16)) mod 2^32. H=0,L=0xC602 is positive 0xC602; preserve proven source signedness.
- AnimPdata landing fields: +0xF8/+0xFC inside 0x104 bytes, not an extension at +0x104/+0x108.
- Runtime owner fields are not interchangeable with similar static tables.
- Mirage/m2c/permuter output is a hypothesis; GC evidence governs acceptance. Host-only branches stay out of src.

No structural discrepancy -> [mid](playbook-mid-occurrence.md).
