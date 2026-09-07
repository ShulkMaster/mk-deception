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
Compare referenced values before calling a mismatch pool-only: different float
returns can receive the same ordinary score. Run data-value mode as well.
No forced registers, fake volatile, dead sinks, empty arms, invented fields,
wrong ABI, undefined returns, goto, or assembly workaround. Real MMIO stays volatile.
Finish: quality pass, full Ninja, SHA-1, progress, diff-check, status. Distinguish
report-exact, data-value-exact, and link-exact; disclose nonmatching fallbacks.

Keep these books slim: amend one applicable rule with new evidence, and put
scores, attempts, and campaign history in the
[consolidated knowledge record](matching-knowledge.md). Do not append a diary.

If m2c rejects directives in a generated context, preprocess a scratch copy
with the project's include paths and defines, then pass that copy to
`--context`. Do not strip directives blindly or rewrite generated inputs.
Host preprocessing is only parser preparation: union-member and data-base
inferences still require retail offsets and relocations ([AI context check](ai-matching-campaign.md#ninety-seventh-round-typed-context-recovery)).

If a permuter target calls a helper that retail expands, wrapper-only mutation
may miss the relevant locals. Expand that helper in the scratch and verify its
base score before searching. Preserve effects and strip scaffolding before a
real-TU trial ([AI selector check](ai-matching-campaign.md#round-129-expanded-catch-selector-search)).

## Evidence-gated priority

The [measured ranking](matching-knowledge.md#measured-rule-ranking) puts H02 owner
and layout recovery first, followed by H11 joins and an applicable M01 TU-mode
check. Only after those structural checks, consider H15's one honest lifetime
insight. ABI, width and effect-order evidence always overrides this search order.
Check shared consumers before spending repeated attempts on an isolated leaf;
consumer counts from one header correction are not independent trial successes.

## Rules (structural first)

H01 | Wrong argument/return registers | All callers + callee ABI | Correct declarations, definitions, callbacks, and calls together; use canonical typed pointers/virtual methods. For variadic library calls, use the canonical prototype and inspect the ABI marker as well as argument registers. A live scratch register at bl is not an argument if the callee overwrites it before use or never consumes it; trace coordinate/address scratch through the callee before extending a prototype. Conversely, trace registers consumed before overwrite back through the caller: an argument computed before a branch can be omitted by m2c together with its apparently dead computation. Trace event payload members through dispatch to the final consumer before naming an integer-looking word as an ID; it may carry a live object pointer. For pointer outputs, check whether the callee reads the incoming value before writing it and trace caller initialization on every path. If retail behavior depends on uninitialized stack contents, record that evidence; neither a zero initializer nor a C indeterminate read establishes faithful, defined recovery ([chess checkpoint 58](mk-chess-body-recovery.md)). No invented argument or unused return. A void wrapper can accidentally leave the callee result in r3 and still match; when a real caller consumes that result, recover the typed return and explicit return statement, then remeasure. Process callbacks may require a float continuation/completion result even when no C caller reads it.
H02 | Wrong offset/width or opaque pointer owner | Multiple accesses + allocation/compiler layout | Recover canonical fields/arrays within proven extents; preserve unknown gaps and verify producers as well as consumers. For opaque or differently signed pointers, trace every assignment and interpreter use; choose the proven record/element type, then remove redundant casts. Keep unsigned bitmask conversions distinct from signed sentinel/division semantics ([AI command pointer](ai-matching-campaign.md#round-111-canonical-command-pointer)). Derive array origin separately from stride: a fixed offset used in every iteration may include a global header, not a prefix in each record. For list builders, bound every write within a full iteration before choosing capacity: a loop-entry count limit may be exceeded by multiple appends in that iteration.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct storage/ABI width; keep promoted accumulators full-width. When replacing a local view with canonical signed fields, check conditional-expression type before assignment to an unsigned local. Test an explicit conversion at that value boundary; do not alter canonical storage merely for codegen ([combo recovery](ai-matching-campaign.md#round-115-canonical-combo-count-table), [unchanged sibling control](ai-matching-campaign.md#round-116-unsigned-count-boundary-control)). A callee returning a 16-bit value does not alone establish a 16-bit function type: check the canonical declaration/definition, then express caller narrowing at its actual boundary. Avoid narrowing twice when a later cast or destination already owns the conversion. If assembly narrows before a range check, preserve modulo behavior rather than inferring saturation; test values around the wrap boundary and omit provably unreachable clamp code. Proven byte-range FP input may need direct float-to-byte conversion, not an intermediate int; never generalize to modulo narrowing. For FP comparisons, decode the complete condition-register predicate, including cror and the consuming branch: m2c equality output may represent <= or >=. Check ordered/unordered behavior before choosing the C operator.
H04 | Bit extraction/RMW differs | Storage width + bit position + every use | Existing bitfield or proven mask; unsigned-byte promotion before shift. Distinguish a byte load from a word load followed by narrowing: on big-endian PPC, lwz plus clrlwi 24 selects the low numeric byte, not the byte at the word address. Use a neutral name when semantics are unknown. Preserve packed sign/flag bits until their last consumer; mask only the table address.
H05 | Cached value vs retail reload | Call/sleep/aliasing store or external poll boundary | Reread the authoritative owner at each observed boundary. Distinguish a helper consuming an existing data snapshot from one accepting its owner and loading the data inside; test the latter only at the proven reload boundary, and remeasure before generalizing it to neighboring paths. Preserve observed component-store order (including z/y/x); a cached object can suppress necessary owner reloads even without a call. Distinguish a retained input from a buffer reused as a callee output: preserve an invariant displacement independently when retail does, rather than reading the overwritten buffer. Verify this with a nonzero output mutation and compare subsequent arguments; a zero-only stub can conceal the dependency. Only proven interrupt/debugger completion flags justify volatile on declaration and definition; check every reader/writer.
H06 | Retail retains computed value/address | Shared uses + unchanged ownership interval | Name genuine typed owners, elements, selection indices or semantic accumulators; reject redundant aliases and identity wrappers, including permuter residue. Use one proven derived owner for inherited arrays. Keep original/advanced snapshots only while both are live; otherwise test advancing the typed pointer instead of indexing (AI move-row/scripted-command evidence). Measure variable separation and nested scope independently. For sequential likelihoods, preserve every random draw, narrow at the supported boundary and keep subsequent arithmetic full-width. A call-free inline predicate can still create a result lifetime: test its actual checks in the consumer accumulator without moving lazy calls, reads or initializers across caller setup. Remove unused wrappers only after whole-unit validation ([throw decisions](ai-matching-campaign.md#fiftieth-round-findings-and-checkpoint), [duck attack](ai-matching-campaign.md#fifty-seventh-round-findings-and-checkpoint)). When retail copies then transforms a scalar, preserve both real consumers and that value flow, such as retaining a distance before squaring it in place. Recheck temporary compiler overrides afterward ([dizzy reaction](ai-matching-campaign.md#forty-first-round-findings-and-checkpoint)).
H07 | Extra/missing helper call | Retail call boundary + signature + active inline settings | Visible inline body for expansion; out-of-line body for a real call. Repeated call-free expansion under inline-off may need a side-effect-safe typed macro. Inspect emitted calls and every consumer. A dont_inline region can also suppress helper expansion inside its function; check both boundaries, and restore evidenced caller-before-callee order before forcing policy. If explicit helpers must expand but an ordinary callee must remain a call, test scoped auto_inline off at that callee definition; caller-only placement may not change saved inlining eligibility. Verify the callee body and every consumer before retaining it. Before moving a body, ensure canonical callee declarations precede its new location (including indirect shared callers); reject implicit-int/variadic fallbacks. Check its callers and callees: fixing one edge can expose a different helper for auto-inlining; preserve the evidenced dependency order and remeasure the whole unit. With nested helpers, moving only the intermediate definition may leave the outer caller eligible to inline through it; inspect the final caller and place it before the callee definition when retail requires that call.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads + no call | Typed accessor returning the validated pointer or null; pass the owner if argument evaluation hoists reads. Preserve required snapshots and active dont_inline regions; no empty valid arm.
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition; rotated top test may need a top guard/break. An unsigned ascending index can retain cmplwi/ble plus CTR where decrementing length cannot. No dummy one-trip loop.
H10 | Switch dispatch differs | Full finite case/default/fallthrough set + text order | Enumerate values routed by comparison ranges: m2c can omit labels sharing another arm even when the result scores above 99%. Recover case order or a proven shared decoding family/tail; preserve repeated independent guards and field-publication order. Repeated per-arm address formation can require retaining the real index, not hoisting an element pointer before dispatch. No speculative labels.
H11 | Return/cleanup join differs | Actual branch destinations + effect ownership | Identify the target instruction: a branch to a final store differs from one past it. For an extra early-failure result assignment, test a positive gate sharing the final return. If an actual state/flag still lowers differently, test a single-case switch with its existing default failure; no invented values ([background dispatch](ai-matching-campaign.md#sixty-second-round-findings-and-checkpoint)). Test Boolean arm order separately. Merge null/invalid failures with a short-circuit predicate only when retail shares their return; preserve null safety, lazy validation, reads and stores ([ducker](ai-matching-campaign.md#thirty-first-round-findings-and-checkpoint)). Conversely, separate real success returns inside action arms when retail materializes them separately; never omit a return ([danger avoidance/blocking](ai-matching-campaign.md#forty-fourth-round-findings-and-checkpoint)). In temporary board/state simulation, preserve each restoration and the final derived-state rebuild, including zero-iteration paths. Cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. For event buffers, derive storage extent from the consumer copy as well as producer stores: four initialized coordinate bytes do not establish a four-byte object when dispatch copies sixteen. Check the retail frame and trailing-byte consumers; larger storage fixes an overread but does not prove the unwritten bytes are unobservable. No invented initialization or compiler scaffolding.
H13 | Intrusive-list accesses differ | Link ownership + callback effects + no call | Exact typed reciprocal-store order; save next before mutating callback when observed, and reload links as retail does.
H14 | Stack slots/store order differ | Real address-taken locals + offsets + lifetimes | Reorder declarations/whole aggregates or narrow scope; separate declaration order from initialization order only while preserving execution and C89 constraints. Distinguish compiler-created by-value copies; no padding locals.
H15 | Coloring only | Same operations/CFG/memory accesses | Before classifying swapped integer-add operands as register coloring, check expression association: when retail adds a random increment and then a fixed increment to an accumulator, try two sequential additions to that same accumulator. Require unchanged call/read/store order and defined arithmetic at both intermediate results; do not generalize this to floating-point reassociation or publish extra field stores. The AI minimum-block-time match verifies this form. For a deadline, keep the narrowed random sample separate from the full-width sum and publish the field once: initialize the sum from the sample, add the current tick, then the fixed delay. Explicit operand order matters independently of staging: initialization needs current tick plus accumulator, while style changes need accumulator plus current tick. Both match after staged accumulation; merely reassociating the original style-change expression regressed (see [campaign rounds 21–24](ai-matching-campaign.md#twenty-first-round-findings-and-checkpoint)). Otherwise, at most one honest lifetime/declaration insight, then niche stop. A finite scratch-only search may identify it; use stack-sensitive scoring and verify the real TU. No register carousel or invented uses.
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
- H02: Opcode variants sharing an instruction union can have different trailing
  fields. Sphere spawning has two radii before its quadratic option; the generic
  four-argument shape places that option later. Recover a typed variant and
  verify both the instruction producer and dispatcher consumer.
- H04: Check the complete index mask against allocation extent. VM dirty-page
  access needs a 13-bit page index for its 8,192-entry LUT; a bare shift retained
  unsupported address bits even at a high fuzzy score. Check each sibling
  independently rather than imposing that mask on all address APIs.
- H02/H05: Distinguish a fixed hardware or low-memory owner from an unresolved
  external symbol. Confirm the address and qualifiers against matching sibling
  definitions and callers: VI registers are volatile MMIO at 0xCC002000, while
  the PAD/OS reset byte uses the canonical unqualified absolute RAM declaration.
  Do not infer volatility from a numeric address or remove real MMIO volatility.
  Preserve the proven owner kind: DVD DI/PI and SI banks require canonical
  absolute volatile array declarations. Replacing them with pointer macros changed
  addressing/scheduling and regressed consumers; equal addresses alone do not
  establish the same compiler-visible owner. Once confirmed, inspect every
  sibling regardless of its initial score: SI transfer routines below 80%
  closed with the same declaration correction. Large scheduling differences
  can follow one wrong owner declaration; they need not imply a wrong algorithm.
- H02/H06: Repeated state-owner reloads around FIFO writes can originate in the
  port declaration. GX's byte, halfword, word and float writes share one absolute
  volatile union at 0xCC008000. Separate arbitrary-pointer casts obscured that
  owner; local snapshot trials failed, while recovering the port closed a family
  of consumers. Verify overlapping offsets, widths, single evaluation and command
  order, then audit every header consumer. Do not generalize a FIFO union to
  unrelated memory or retain extra snapshots after the declaration fix.
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
  A floating-point bit-estimate sequence can also separate a real position
  snapshot from its later publication. In chess cell blending, retaining the
  initial X value removed an extra object reload while preserving the retail
  Z-owner reload after the X store. Keep only snapshots with actual later uses;
  see the [body-recovery evidence](mk-chess-body-recovery.md).
  If conversion-constant addressing differs with identical arithmetic, retain
  a genuinely used scan index through its scale lookup; do not invent a constant
  pool pointer or dead use. Recheck the first path separately from later macros.
- H06/H14: In coefficient decoding, form the sign threshold after unsigned
  amplitude extraction; assemble the extended level before increasing code
  length. A real `extended_base = packed * 2` retained through the bitwise
  combine expresses the stage boundary without dead uses or register forcing.
- H07/H11: An expanded helper can retain a shared publication block with no
  call (`ReadSram`); preserve failure cleanup and do not hide empty branches.
  Repeated materialized Boolean joins can also reveal nested predicates. Require
  the same ordered, lazy guards in multiple retail consumers; try one meaningful
  inner availability predicate and its outer start predicate. Remove redundant
  `if (flag == 0) flag = 0; else flag = 1` chains, and check whether local
  `dont_inline` suppresses the required expansion. AI's caller-before-callee
  order preserved external calls without that region; both predicates expanded
  and recovered exact watchers. Keep consumer-specific checks outside the helper
  and remeasure every consumer; see the [AI evidence](ai-matching-campaign.md).
  The same approach applies to a shared player/weapon attack-region selector:
  preserve the weapon-null guard and caller-specific loads outside the helper.
  It closed AI hit-voice processing and its data comparison; whoosh retained
  register/join residue, so the helper is not a universal closure.
  Require the same compound predicate and join in sibling attack paths before
  sharing it: AI's opcode/index predicate closes general and knockdown attacks.
  This also applies to a repeated scalar selection: AI's blocking-stage
  multiplier helper preserves its result independently of the caller's loaded
  match stage and closes both blocking-likelihood consumers. A named caller
  stage local alone did not help; remove it when the shared helper suffices.
  If retail reads a floating comparison through `mfcr`/bit extraction, check
  whether it is a predicate's returned Boolean rather than a direct branch.
  Preserve ordered/unordered behavior; AI's backward-clearance predicate closes
  the first don't-touch phase without artificial normalization.
  A combo-breakout predicate with early returns also removes a flag needlessly
  held across calls; preserve both likelihood calculations and lazy random calls.
  For reset-and-test functions, inspect the actual result lifetime: one result
  initialized to false and assigned the reset helper's return can match where
  independent returns or an ignored reset result do not. Decode switch-default
  writes separately from the selected value; do not invent redundant defaults
  or result assignments without retail evidence. Check stack slots after moving
  aggregates into a helper, even when the fuzzy score barely changes.
  If retail copies a local dispatch table before several checks and then joins
  an outer publication block, test one typed selection/validation helper with
  explicit returns. Preserve each check's nonzero versus exactly-one semantics;
  verify the table entries and actual stack copy, not just branch similarity
  ([chess evidence](mk-chess-body-recovery.md)).
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
  See [the nine-candidate follow-up](matching-knowledge.md#mutable-cursor-ownership-generalizes-to-fills).
- H08: Direct-owner accessors suit adjacent load/validation; cached forms suit
  real intervening effects. Extracting a latch can change a caller's inlining,
  even reducing a previously exact caller to zero. Check all callers, not only
  the target; retain documented invalid-path differences as unresolved behavior.
- H10: Verify finite bounds before accepting a decompiler default arm. SPSD
  header values 2/3 enter codec selection, while values at least 4 bypass it
  and still reach common output normalization. m2c labeled the bounded arm
  as default; the retail comparison and branch target resolve the discrepancy.
- H14/H15: Separate independent loop counters; group active/saved-next iterators
  without moving assignments across callbacks. Split scalar declarations from
  initialization only without const, aggregate, scope, or lifetime changes.
  Initialized declaration reordering can reorder stores and is not coloring.
- H06/H15: If one pointer local is reused for independently selected objects,
  require distinct source owners and unchanged lazy loads; try one pointer per
  owner. AI push selection matched with separate first/second weapon-style
  pointers after removing the permuter candidate's redundant alias. For repeated
  pure field checks with no intervening effects, direct member reads can let
  MWCC share the load with the retail register lifetime; AI throw restrictions
  matched after removing the cached state local. Verify emitted load count and
  every consumer; neither case permits extra snapshots or dummy aliases. See
  the [AI campaign evidence](ai-matching-campaign.md).
- H04: Sofdec combined VLC indices need 0x3FF / 0xFFF before sign extraction;
  0x3FE / 0xFFE loses signs. Exercise both signs, first/subsequent coefficients,
  and all bit alignments against retail, not a port inheriting the same bug.

## Known traps

- `addis x,v,H; cmplwi x,L` tests `(L - (H << 16)) mod 2^32`;
  H=0,L=0xC602 means positive 0xC602. Preserve proven signedness. AI's
  defenseless watcher similarly needs signed equality with positive 0xC601;
  m2c produced -0x39FFU, and the wrong source still scored 99.98876% after
  other fixes. Unsigned equality fixed behavior but omitted retail's addis;
  the signed field comparison restored it. Decode the immediate before
  classifying a high-scoring residual as coloring.
- ELF `NOBITS` sections have zero-initialized storage, not file payload bytes.
  Never read `sh_offset` as their initializer. Check this before changing a
  constant to resolve a `data_value` mismatch.
- Runtime owners are not interchangeable with similar static tables.
- Identical cursor stride does not establish identical ownership: follow each
  pointer load before applying its offset. The mk_chess drone's +0x108 owner
  differs from its embedded mode cursors. Its rescue helper also remained
  behaviorally wrong at 99.97%: +0x4C was used instead of +0x50. Check immediates
  before calling any residual a relocation or coloring ceiling; see the
  [five-attempt audit](mk-chess-five-passes.md).
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
[MPVABDEC goal report](matching-knowledge.md#decoder-reconstruction-transferable-findings).
