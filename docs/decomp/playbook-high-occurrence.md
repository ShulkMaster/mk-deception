# Matching rules: high occurrence

## Protocol (all three books)

Authority: AGENTS.md > retail ASM/callers/ELF/layout > decompiler/foreign port.
Route: structural error -> H; localized residue -> M; rare/stop -> N.
Schema: ID | IF mismatch | REQUIRE evidence | TRY one change. Missing evidence ->
skip; no applicable rule -> investigate or stop. These are diagnostics, not recipes.

Loop: baseline -> classify -> one rule -> rebuild -> same-symbol objdiff ->
accept/revert. Update the source `TODO: [status]` below 100 per AGENTS.md; remove
matching-progress comments at measured 100, retaining semantic explanations.
Recheck every shared consumer, including below-threshold and already-exact ones.
Preserve math, stores, call order, widths, and lazy null checks. Inspect offsets
and immediates even above 99%; large score swings can be diff-alignment artifacts.
No forced registers, fake volatile, dead sinks, empty arms, invented fields,
wrong ABI, undefined returns, goto, or assembly workaround. Real MMIO stays volatile.
Finish: quality pass, full Ninja, SHA-1, progress, diff-check, status. Distinguish
report-exact, data-value-exact, and link-exact; disclose nonmatching fallbacks.

Keep these books slim: amend one applicable rule with new evidence, and put
scores, attempts, and campaign history in linked reports. Do not append a diary.

## Rules (structural first)

H01 | Wrong argument/return registers | All callers + callee ABI | Correct declarations, definitions, callbacks, and calls together; use canonical typed pointers/virtual methods. No invented argument or unused return.
H02 | Wrong offset/width | Multiple accesses + allocation/compiler layout | Recover canonical fields/arrays within proven extents; preserve unknown gaps and verify producers as well as consumers.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct storage/ABI width; keep promoted accumulators full-width. Proven byte-range FP input may need direct float-to-byte conversion, not an intermediate int; never generalize to modulo narrowing.
H04 | Bit extraction/RMW differs | Storage width + bit position + every use | Existing bitfield or proven mask; unsigned-byte promotion before shift. Use a neutral name when semantics are unknown. Preserve packed sign/flag bits until their last consumer; mask only the table address.
H05 | Cached value vs retail reload | Call/sleep/aliasing store or external poll boundary | Reread the authoritative owner at each observed boundary. Only proven interrupt/debugger completion flags justify volatile on declaration and definition; check every reader/writer.
H06 | Retail retains computed value/address | Shared uses + unchanged ownership interval | Name a genuine typed local/element/owner. Use one proven derived owner for inherited arrays; retain distinct original/advanced buffer snapshots while both remain live.
H07 | Extra/missing helper call | Retail call boundary + signature + active inline settings | Visible inline body for expansion; out-of-line body for a real call. Repeated call-free expansion under inline-off may need a side-effect-safe typed macro. Inspect emitted calls and every consumer.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads + no call | Typed accessor returning the validated pointer or null; pass the owner if argument evaluation hoists reads. Preserve required snapshots and active dont_inline regions; no empty valid arm.
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition; rotated top test may need a top guard/break. An unsigned ascending index can retain cmplwi/ble plus CTR where decrementing length cannot. No dummy one-trip loop.
H10 | Switch dispatch differs | Full finite case/default/fallthrough set + text order | Recover case order or a proven shared decoding family/tail; preserve repeated independent guards and field-publication order. No speculative labels.
H11 | Return/cleanup join differs | Actual branch destinations + effect ownership | Shared result/epilogue or explicit arm returns as observed. Verify whether a branch lands on a final store or past it; source addresses alone do not identify the target. Cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. No compiler scaffolding.
H13 | Intrusive-list accesses differ | Link ownership + callback effects + no call | Exact typed reciprocal-store order; save next before mutating callback when observed, and reload links as retail does.
H14 | Stack slots/store order differ | Real address-taken locals + offsets + lifetimes | Reorder declarations/whole aggregates or narrow scope; separate declaration order from initialization order only while preserving execution and C89 constraints. Distinguish compiler-created by-value copies; no padding locals.
H15 | Coloring only | Same operations/CFG/memory accesses | At most one honest lifetime/declaration insight, then niche stop. A finite scratch-only search may identify it; use stack-sensitive scoring and verify the real TU. No register carousel or invented uses.
H16 | Producer/consumer move differs | Real returned object + consumer ABI | Nest a single-use result; retain the original callback owner at an untyped boundary when observed. No manufactured return contract.

## Measured examples

Apply these refinements only with the parent rule's evidence:

- H01/H03: `GXBool` locals can remove redundant masks under disabled optimization;
  preserve full-width masks/enums and correct public prototypes. Typed sound
  virtual methods recovered the retail r12 dispatch; this does not establish a
  concrete class layout or destructor contract.
- H03/H11: TRK connection checks use a local `BOOL` for the returned flag;
  retain the API's declared return type. Separate serial-I/O and connection
  early returns when retail has distinct exit blocks, preserving short circuiting.
- H02: MWCC may place a vptr after members preceding the first virtual method.
  `IRefCntRes` needs virtual declarations before reference_count for +0/+4.
  Nested anonymous structs inside FighterSlot's union measured size 1; three
  pointer unions restore 0x0C / PlyrInfo 0x6C. Probe layout and recheck all callers.
- H02/H04: Skin platform data is +0x2C, not mesh count +0x34; process entry is
  +0xB8 (`MkProcEntryFn`), not destruction callback +0xB4. Small immediate
  differences can be behavioral. AnimPdata landing fields are +0xF8/+0xFC
  within 0x104 bytes, not an extension beyond the allocation.
- H03/H12: A stored `Vec` component and its incoming FP argument need not have
  identical precision. If retail rounds before publishing a second copy, read
  the real stored component; do not bypass it or add a synthetic rounding sink.
- H04: For mask-then-shift extraction, assign the complete expression to the
  decoded value instead of reusing that local for the unshifted mask. Confirm
  all uses: `_rwGCNDisplayListGetStride` retains the same operations this way.
- H05: NPC command waits decrement through a saved NPC pointer, then check and
  clamp through `g_active_npc`. Preserve that observed owner reload after the
  store; audit every inlined consumer rather than introducing volatile.
- H06/H14: Initialize a real output pointer at entry only when retail zeros
  that slot there. Keep an initial buffer word separate from its advanced word
  through the consume/refill join. Put real snapshots in their owning scope,
  keeping their loads at the observed point; declaration order is not initialization
  order. Consume lookahead in place only after every
  decoding use; preserve shifts even when omitting one raises fuzzy.
  If conversion-constant addressing differs with identical arithmetic, retain
  a genuinely used scan index through its scale lookup; do not invent a constant
  pool pointer or dead use. Recheck the first path separately from later macros.
- H06/H14: In coefficient decoding, form the sign threshold after unsigned
  amplitude extraction; assemble the extended level before increasing code
  length. A real `extended_base = packed * 2` retained through the bitwise
  combine expresses the stage boundary without dead uses or register forcing.
- H07/H11: An expanded helper can retain a shared publication block with no
  call (`ReadSram`); preserve failure cleanup and do not hide empty branches.
- H07: For a proven call-free helper rejected by MWCC, an isolated diagnostic
  can test both inline_max_size and inline_max_total_size; either alone may
  fail. Scope/reset any justified limits and check siblings. This establishes
  expansion, not the retail pragma values; respect fixed-TU-setting tasks.
  If reader roles diverge across real decoder phases, test a typed inline
  phase boundary with only genuine inputs/outputs. Include cursor setup when
  that phase owns its lifetime, preserving read/store order; recheck consumers
  and remove earlier lifetime specializations that the new scope makes redundant.
  If one cursor transfer remains between otherwise matching phases, test an
  outer decode owner spanning initialization through final publication, leaving
  independent storage clearing outside it. Verify nested phases separately:
  flattening them can undo allocation gains. Do not add identity wrappers.
  For repeated fill/clear expansions whose retail cursor advances between runs,
  test a typed pointer-to-pointer helper that performs those real stores and
  advances the caller's cursor. Returning an end pointer or advancing outside
  the helper can fold away retail updates. Preserve the full array extent:
  six coefficient blocks need one six-block paired-store view, not indexing
  past a single block. Check short local fills separately from long helper fills.
  See [the nine-candidate follow-up](mpv-followup-2026-09-05.md).
- H08: Direct-owner accessors suit adjacent load/validation; cached forms suit
  real intervening effects. Extracting a latch can change a caller's inlining,
  even reducing a previously exact caller to zero. Check all callers, not only
  the target; retain documented invalid-path differences as unresolved behavior.
- H14/H15: Separate independent loop counters; group active/saved-next iterators
  without moving assignments across callbacks. Split scalar declarations from
  initialization only without const, aggregate, scope, or lifetime changes.
  Initialized declaration reordering can reorder stores and is not coloring.
- H04: Sofdec combined VLC indices need 0x3FF / 0xFFF before sign extraction;
  0x3FE / 0xFFE loses signs. Exercise both signs, first/subsequent coefficients,
  and all bit alignments against retail, not a port inheriting the same bug.

## Known traps

- `addis x,v,H; cmplwi x,L` tests `(L - (H << 16)) mod 2^32`;
  H=0,L=0xC602 means positive 0xC602. Preserve proven signedness.
- ELF `NOBITS` sections have zero-initialized storage, not file payload bytes.
  Never read `sh_offset` as their initializer. Check this before changing a
  constant to resolve a `data_value` mismatch.
- Runtime owners are not interchangeable with similar static tables.
- Automated extraction must recognize C identifiers: `0.0f * body` is not a
  pointer declaration. Reject malformed generated source before measuring it.
- Exhausted declaration searches are not new evidence; do not repeat them
  without a changed lifetime/CFG hypothesis.
- Mirage/m2c/permuter output remains a hypothesis. Keep host branches out of
  Deception; portable corrections require retail and behavioral evidence.

No structural discrepancy -> [mid](playbook-mid-occurrence.md).

H07 — unsigned lookahead phase boundary:
IF a localized escape-window shift has matching operations but different
scheduling, REQUIRE the same real unsigned bit-window transformation and a
call-free emitted body. TRY a typed inline value helper for that transformation,
then inspect the complete consumer and every sibling. This closed Dc11 and
removed Nintra AC's escape-scheduling island, but regressed Nintra's first-code
phase: the boundary is context-dependent, not a universal shift-wrapper rule.
Do not add identity helpers, dummy state, false prototypes or generic permuter
names. These results infer a source boundary, not an original retail helper name.
Keep measurements and rejected contexts in the
[MPVABDEC goal report](mpvabdec-goal-2026-09-05.md).
