# Matching rules: high occurrence

## Protocol (all three books)

Authority: AGENTS.md > retail ASM/callers/ELF/layout > decompiler/foreign port.
Route: structural error -> H; localized residue -> M; rare/stop -> N.
For Ghidra MCP, verify the project and explicitly select the program. If an
address query returns unrelated pseudocode, compare raw bytes and retail ELF
symbol bounds before trusting function metadata. Correct bytes alone do not
validate the returned function; try the matching ELF program before editing
analysis or source ([AI MCP check](../../.agent-work/decomp/ai-matching/README.md#round-141-ghidra-project-and-program-validation)).
Schema: ID | IF mismatch | REQUIRE evidence | TRY one change. Missing evidence ->
skip; no applicable rule -> investigate or stop. These are diagnostics, not recipes.

Loop: baseline -> classify -> one rule -> rebuild -> same-symbol objdiff ->
accept/revert. Update the source `TODO: [status]` below 100 per AGENTS.md; remove
matching-progress comments at measured 100, retaining semantic explanations.
Recheck every shared consumer, including below-threshold and already-exact ones.
Preserve math, stores, call order, widths, and lazy null checks. Inspect offsets
and immediates even above 99%; large score swings can be diff-alignment artifacts.
An aggregate score can rise while a changed function regresses after branch
removal; compare each function and its instructions before accepting a gain
([AI dispatch audit](../../.agent-work/decomp/ai-matching/README.md#round-137-remove-unsupported-twitch-dispatch-grouping)).
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
If m2c cannot resolve a branch to the current function's entry, confirm the
branch address equals the entry and give that address a local label in a scratch
assembly copy. Retarget only the intra-function branch; preserve calls and every
instruction. DTK's `.fn` directive may not supply m2c with a branch-target label.
Puzzle piece generation confirms this self-entry retry case; do not replace it
with recursion or hand-edit generated assembly.

Host preprocessing is only parser preparation: union-member and data-base
inferences still require retail offsets and relocations ([AI context check](../../.agent-work/decomp/ai-matching/README.md#ninety-seventh-round-typed-context-recovery)).

If a permuter target calls a helper that retail expands, wrapper-only mutation
may miss the relevant locals. Expand that helper in the scratch and verify its
base score before searching. Preserve effects and strip scaffolding before a
real-TU trial ([AI selector check](../../.agent-work/decomp/ai-matching/README.md#round-129-expanded-catch-selector-search)).
If expansion reverses stack-local placement, compare the offset map and test
reversing only the scratch declarations; require the original stack and mismatch
baseline before searching ([clearance scratch](../../.agent-work/decomp/ai-matching/README.md#round-171-layout-corrected-clearance-search)).
Expansion and declaration order can interact: the AI table builders match only
with both, while either alone regresses. Verify the combination in every wrapper;
do not infer that a candidate transfers unchanged back into an inline helper
([table-builder check](../../.agent-work/decomp/ai-matching/README.md#round-131-table-builders-exact)).

## Evidence-gated priority

The [measured ranking](matching-knowledge.md#measured-rule-ranking) puts H02 owner
and layout recovery first, followed by H11 joins and an applicable M01 TU-mode
check. Only after those structural checks, consider H15's one honest lifetime
insight. ABI, width and effect-order evidence always overrides this search order.
Check shared consumers before spending repeated attempts on an isolated leaf;
consumer counts from one header correction are not independent trial successes.

## Rules (structural first)

H01 | Wrong argument/return registers | All callers + callee ABI | A pointer-looking r3 may address the caller-created copy of a by-value aggregate. Verify the caller copy and declarations before adding callee pointer snapshots: inventory image creation closes with a by-value texture pair and direct array stores, while its pointer form stalled with unnecessary snapshots. Preserve the signed loop count evidenced by cmpw. Correct declarations, definitions, callbacks, and calls together; use canonical typed pointers/virtual methods. Before calling a virtual-call mismatch pure coloring, verify that the typed callback receives its object argument: a legacy unprototyped vtable slot can leave r3 holding the vtable instead. Krypt biography destruction closes by using the existing StringObjVtable and passing the object explicitly. For variadic library calls, use the canonical prototype and inspect the ABI marker as well as argument registers. A live scratch register at bl is not an argument if the callee overwrites it before use or never consumes it; trace coordinate/address scratch through the callee before extending a prototype. A preceding store can leave its object address in r3: do not repeat that owner as a call argument when the callee reads only globals. Correct both the no-argument prototype and caller, then compare the entire caller TU. A forwarding wrapper may need float arguments even when it emits no floating-point instructions: collision_2 must pass incoming f1/f2 through to attack-region setup, as confirmed by its callers and callee. Do not replace forwarded values with zero. Conversely, trace registers consumed before overwrite back through the caller: an argument computed before a branch can be omitted by m2c together with its apparently dead computation. Trace event payload members through dispatch to the final consumer before naming an integer-looking word as an ID; it may carry a live object pointer. For pointer outputs, check whether the callee reads the incoming value before writing it and trace caller initialization on every path. If retail behavior depends on uninitialized stack contents, record that evidence; neither a zero initializer nor a C indeterminate read establishes faithful, defined recovery ([chess checkpoint 58](mk-chess-body-recovery.md)). If a wrapper leaves incoming r3 untouched before pdata_of_proc or xfer_proc, trace its caller before substituting aproc: the spear lookup and both retraction wrappers consume an explicit process pointer despite m2c omitting that argument. No invented argument or unused return. A void wrapper can accidentally leave the callee result in r3 and still match; when a real caller consumes that result, recover the typed return and explicit return statement, then remeasure. Process callbacks may require a float continuation/completion result even when no C caller reads it. The three remote fatality callbacks and eleven roll-up callbacks in moves.c verify explicit float returns (the roll-up callers already declare float and accept MkProcEntryFn); their canonical jump_sleep calls also preserve codegen without treating a live vtable scratch register as an extra argument.
H02 | Wrong offset/width or opaque pointer owner | Multiple accesses + allocation/compiler layout | Inspect every differing store offset even above 99%: Sindel sonic-sound validation reached 99.94444% while still writing its active flag at +0 instead of retail +8. Correct the proven member offset and preserve unknown surrounding bytes; a high fuzzy score is not structural evidence. Distinguish an allocation returned through an output pointer from the interior base passed to its initializer. Krypt raw-particle creation returns MkPfx but initializes its VM at object+0x40; using one layout for both falsely aliases object camera-follow coordinates with VM geometry fields. Verify the enclosing owner separately from the allocated storage: collision initialization stores its allocation directly in PlyrInfo+18 and resets that same player-info record. A placeholder collision type with matching +18/+58/+5C fields can disguise an extra pointer dereference; confirm the caller before adding another nodes indirection. Recover canonical fields/arrays within proven extents; preserve unknown gaps and verify producers as well as consumers. For bitsets, derive capacity from the retail accessor bound and byte-index calculation, then check the enclosing layout; visible gameplay entries may use only part of the storage. Krypt exposes 400 coffins, but its profile accessors accept 600 bits (75 bytes), so a one-byte placeholder or 50-byte view is incomplete. Trace unnamed scalar groups to typed API arguments before naming them: Krypt VM +0x154/+0x158/+0x15C feed GXInitLightAttn k0/k1/k2, so they are light attenuation coefficients, not texture coordinates. For opaque or differently signed pointers, trace every assignment and interpreter use; choose the proven record/element type, then remove redundant casts. Keep unsigned bitmask conversions distinct from signed sentinel/division semantics ([AI command pointer](../../.agent-work/decomp/ai-matching/README.md#round-111-canonical-command-pointer)). Derive array origin separately from stride: a fixed offset used in every iteration may include a global header, not a prefix in each record. For list builders, bound every write within a full iteration before choosing capacity: a loop-entry count limit may be exceeded by multiple appends in that iteration. Player wake-up additionally shows why near-100 scores cannot excuse adjacent-field substitutions: the opponent-publication guard tests player object pointers at GameInfo+0x100/+0x16C, not pdata at +0xFC/+0x168. Correcting the owner fields after typed latch recovery also removes needless cached globals in both player-switch wrappers.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct storage/ABI width; keep promoted accumulators full-width. A sign-extension plus subfc/subfe chain can be a 64-bit unsigned range comparison; reconstruct the comparison width before trusting carry-dependent decompiler output ([character-lock range](../../.agent-work/decomp/gameplay-200/README.md#round-17-character-lock-range-and-mask-selection), [canonical mask ownership](../../.agent-work/decomp/gameplay-200/README.md#round-18-canonical-unlock-mask-ownership)). When replacing a local view with canonical signed fields, check conditional-expression type before assignment to an unsigned local. Test an explicit conversion at that value boundary; do not alter canonical storage merely for codegen ([combo recovery](../../.agent-work/decomp/ai-matching/README.md#round-115-canonical-combo-count-table), [unchanged sibling control](../../.agent-work/decomp/ai-matching/README.md#round-116-unsigned-count-boundary-control)). A callee returning a 16-bit value does not alone establish a 16-bit function type: check the canonical declaration/definition, then express caller narrowing at its actual boundary. Avoid narrowing twice when a later cast or destination already owns the conversion. If assembly narrows before a range check, preserve modulo behavior rather than inferring saturation; test values around the wrap boundary and omit provably unreachable clamp code. Proven byte-range FP input may need direct float-to-byte conversion, not an intermediate int; never generalize to modulo narrowing. For inverse-length guards, value <= 0 and !(0 < value) differ on NaNs; preserve the decoded retail predicate before tuning codegen. For FP comparisons, decode the complete condition-register predicate, including cror and the consuming branch: m2c equality output may represent <= or >=. Check ordered/unordered behavior before choosing the C operator. When retail narrows a random result before a guard but reads the selected table entry only after success, preserve both boundaries. Store the explicitly narrowed sample in a full-width index if a short local introduces a second mask during scaling; the Krypt character monitor verifies this form without changing random draw count.
H04 | Bit extraction/RMW differs | Storage width + bit position + every use | Existing bitfield or proven mask; unsigned-byte promotion before shift. For li 1 plus rlwimi into a byte, check the canonical one-bit assignment before accepting a raw OR: Konquest chest-camera stop matches with flags_bits.konquest_mode = 1 at +0x6C bit 6. The otherwise live r3 constant before get_game_state is bit-insertion input, not an extra function argument. The same test applies to repeated clears: li 0 plus rlwimi at distinct bit positions supports individual canonical field assignments; preserve store order and global-pointer reloads rather than combining masks (fatality player release). Distinguish a byte load from a word load followed by narrowing: on big-endian PPC, lwz plus clrlwi 24 selects the low numeric byte, not the byte at the word address. Use a neutral name when semantics are unknown. Preserve packed sign/flag bits until their last consumer; mask only the table address. For extrwi. of a single flag, the existing bitfield test can preserve the normalized extraction where a raw mask emits an unshifted test (Konquest interaction-camera gate). For a bit extraction followed by Boolean normalization, check the canonical bitfield before using a raw mask; the latter can erase the retail normalization boundary ([arena availability](../../.agent-work/decomp/gameplay-200/README.md#round-14-arena-availability)).
H05 | Cached value vs retail reload | Call/sleep/aliasing store or external poll boundary | Reread the authoritative owner at each observed boundary ([subobject lookup](../../.agent-work/decomp/gameplay-200/README.md#round-22-background-subobject-lookup)). A process factory is also a call boundary: if retail reloads a global player slot before publishing it into new process data, retain that reload instead of carrying the pre-call player pointer (sidekick intro). For array members, index through that owner instead of retaining an advancing entry pointer when retail reloads the owner each iteration or after a call ([launch monitors](../../.agent-work/decomp/gameplay-200/README.md#round-27-indexed-launch-monitor-ownership)). Distinguish a helper consuming an existing data snapshot from one accepting its owner and loading the data inside; test the latter only at the proven reload boundary, and remeasure before generalizing it to neighboring paths. Preserve observed component-store order (including z/y/x); a cached object can suppress necessary owner reloads even without a call. When publishing collision state, retain event-owner reloads after earlier publications and flag reads before publishing the owner pointer; measure the resulting store schedule, then stop at pure base-register coloring ([collision publication](../../.agent-work/decomp/gameplay-200/README.md#round-26-collision-publication-and-latch-exits)). Distinguish a retained input from a buffer reused as a callee output: preserve an invariant displacement independently when retail does, rather than reading the overwritten buffer. Verify this with a nonzero output mutation and compare subsequent arguments; a zero-only stub can conceal the dependency. Only proven interrupt/debugger completion flags justify volatile on declaration and definition; check every reader/writer.
H06 | Retail retains computed value/address | Shared uses + unchanged ownership interval | Name genuine typed owners, elements, selection indices or semantic accumulators; reject redundant aliases and identity wrappers, including permuter residue. Use one proven derived owner for inherited arrays. If retail retains a render-quad pointer before a fade-completion test, snapshot that typed quad while preserving separate alpha reads for its vertex stores; the screen-fade process closes without caching alpha. If retail loads a member-array pointer once before a range loop, name that typed array snapshot; direct owner indexing may instead reload it on each iteration ([pebble range](../../.agent-work/decomp/gameplay-200/README.md#round-28-pebble-range-loop-and-array-snapshot)). For consecutive XYZ stores through one retained address, recover the canonical position pointer only for those stores. Preserve later owner reloads when retail copies the position component by component: caching the whole loop or replacing that later copy with aggregate assignment changes the observed ownership interval. This closes Krypt setup_tombstones using RwV3d from the actual RenderWare matrix. A field-address load-update instruction alone does not establish a pointer-to-field local; require supporting uses and stop after rejected owner hypotheses ([danger-zone ceiling](../../.agent-work/decomp/gameplay-200/README.md#round-20-danger-zone-field-address-ceiling)). Keep a search-key snapshot across list cleanup when retail does, while retaining a separate current-owner read for later publication ([danger-zone activation](../../.agent-work/decomp/gameplay-200/README.md#round-33-danger-zone-search-snapshot)). When retail retains a selected record index across calls and reconstructs the table address for later fields, preserve that index with ordinary typed indexing instead of caching an entry pointer; run_ending verifies this distinction after recovering its shared sentinel search. Moving a pointer assignment below the call can still produce the cached address; remove the redundant pointer and use typed indexing at each access when retail retains the index (controller right-stick dispatch). Before dismissing a save/restore as an unobservable self-store, inspect every intervening helper write: air-move initialization must restore the saved animation step because the common initializer writes 1.0f. Removing that restore changes final state even without callbacks. Keep original/advanced snapshots only while both are live; otherwise test advancing the typed pointer instead of indexing (AI move-row/scripted-command evidence). Measure variable separation and nested scope independently. For sequential likelihoods, preserve every random draw, narrow at the supported boundary and keep subsequent arithmetic full-width. A call-free inline predicate can still create a result lifetime: test its actual checks in the consumer accumulator without moving lazy calls, reads or initializers across caller setup. Remove unused wrappers only after whole-unit validation ([throw decisions](../../.agent-work/decomp/ai-matching/README.md#fiftieth-round-findings-and-checkpoint), [duck attack](../../.agent-work/decomp/ai-matching/README.md#fifty-seventh-round-findings-and-checkpoint)). When retail copies then transforms a scalar, preserve both real consumers and that value flow, such as retaining a distance before squaring it in place. Recheck temporary compiler overrides afterward ([dizzy reaction](../../.agent-work/decomp/ai-matching/README.md#forty-first-round-findings-and-checkpoint)).
H07 | Extra/missing helper call | Retail call boundary + signature + active inline settings | Visible inline body for expansion; out-of-line body for a real call. Repeated call-free expansion under inline-off may need a side-effect-safe typed macro. Inspect emitted calls and every consumer. A dont_inline region can also suppress helper expansion inside its function; check both boundaries, and restore evidenced caller-before-callee order before forcing policy. When extracting a repeated search, compare both consumers before retaining it: Krypt key initialization needed scoped auto_inline off rather than dont_inline, while its text updater could use the explicit inline search directly. Run a no-setting control to prove the caller still needs its out-of-line init call; do not infer necessity from the callee score alone. If explicit helpers must expand but an ordinary callee must remain a call, test scoped auto_inline off at that callee definition; caller-only placement may not change saved inlining eligibility. Verify the callee body and every consumer before retaining it. Before moving a body, ensure canonical callee declarations precede its new location (including indirect shared callers); reject implicit-int/variadic fallbacks. Check its callers and callees: fixing one edge can expose a different helper for auto-inlining; preserve the evidenced dependency order and remeasure the whole unit. With nested helpers, moving only the intermediate definition may leave the outer caller eligible to inline through it; inspect the final caller and place it before the callee definition when retail requires that call.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads + no call | Typed accessor returning the validated pointer or null ([bound-header latch](../../.agent-work/decomp/gameplay-200/README.md#round-23-bound-header-latch-accessor)); pass the owner if argument evaluation hoists reads. Preserve required snapshots and active dont_inline regions; no empty valid arm. Konquest grounding suspend/restore close by reusing the existing accessor: its success return retains the retail branch that an open-coded empty success arm folded away. Check for a canonical helper before adding another latch view. Spear cleanup and retraction confirm that a shared owner-typed process accessor can replace a duplicate padded latch view while preserving transfer arguments. For repeated object slots, reuse existing typed accessors while reloading the owner after each effectful visibility call ([slaughterhouse visibility](../../.agent-work/decomp/gameplay-200/README.md#round-31-slaughterhouse-visibility-latch-reuse)). Reuse an existing typed latch helper where its valid-instance return reproduces the diamond; diagnose remaining failure-path behavior separately from coloring ([NPC identity and collision lookup](../../.agent-work/decomp/gameplay-200/README.md#round-7-npc-identity-and-collision-lookup)).
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition; if retail loads a dynamic bound once into CTR, snapshot that bound while preserving any separate per-iteration pointer reload ([material count](../../.agent-work/decomp/gameplay-200/README.md#round-32-material-count-snapshot)); for a fixed positive extent, test bounded for/< against do/while/!= when retail uses CTR ([timer clear](../../.agent-work/decomp/gameplay-200/README.md#round-24-background-timer-counted-loop)); rotated top test may need a top guard/break; preserve polling, sleep and countdown order, including the final input recheck. An unsigned ascending index can retain cmplwi/ble plus CTR where decrementing length cannot. For a fixed table scan ending at the first matching ID, test a bounded for loop with an explicit found-entry break against a compound while condition; gore-object destroy and attach recover the retail CTR loop this way. Preserve not-found handling and first-match semantics. No dummy one-trip loop.
H10 | Switch dispatch differs | Full finite case/default/fallthrough set + text order | Enumerate values routed by comparison ranges: m2c can omit labels sharing another arm even when the result scores above 99%. Recover case order or a proven shared decoding family/tail; check whether a second switch belongs inside an outer case and where each default is emitted ([loading-table evidence](../../.agent-work/decomp/gameplay-200/README.md#round-9-loading-table-nested-dispatch)); preserve repeated independent guards and field-publication order. Repeated per-arm address formation can require retaining the real index, not hoisting an element pointer before dispatch. When retail forms a table base before dispatch, retain that owner while recovering the switch; per-arm base initialization can duplicate address loads ([arena queries](../../.agent-work/decomp/gameplay-200/README.md#round-11-arena-feature-query-family), [exact arena count](../../.agent-work/decomp/gameplay-200/README.md#round-12-arena-count-and-typed-chess-getters), [body-texture count](../../.agent-work/decomp/gameplay-200/README.md#round-15-body-texture-count), [arena lookup limits](../../.agent-work/decomp/gameplay-200/README.md#round-16-arena-index-and-name-lookup)). No speculative labels.
H11 | Return/cleanup join differs | Actual branch destinations + effect ownership | For a nullable owner whose retail entry initializes the result before the guard, try a null-initialized result assigned from the existing validated accessor inside the guard, followed by one return. The Konquest tile-object getter closes this way; directly returning the accessor inside the guard was neutral and folded the inner joins. Preserve both owner-null and instance-validation semantics. Identify the target instruction: a branch to a final store differs from one past it. Trace failure branches to their final destination before calling a mismatch scheduling-only: a missing particle in Krypt coffin-opening effects branches to the epilogue, skipping later objects and the dirt fountain. A continue-shaped reconstruction scored above 94%; validate call traces for early and late failures as well as success. Before tuning a null join, verify that retail tests that owner at all: remove an unsupported optional-owner fallback when the actual callback contract requires the owner ([camera scripts](../../.agent-work/decomp/gameplay-200/README.md#round-29-camera-script-ownership-and-canonical-subobject-lookup)). For an extra early-failure result assignment, test a positive gate sharing the final return. If an actual state/flag still lowers differently, test a single-case switch with its existing default failure; no invented values ([background dispatch](../../.agent-work/decomp/ai-matching/README.md#sixty-second-round-findings-and-checkpoint)). For a final integer Boolean conversion, distinguish nonzero from equality-to-one even when earlier operations constrain the accumulator to 0/1: the opponent-flip query closes with != 0 while preserving both state swaps and XOR toggles. The weapon-block query confirms the same lowering for a callee result after a boss early return; verify the callee contract before treating equality-to-one as a truth test. Test Boolean arm order separately. For a two-value integer selection, prefer the real conditional over subtracting a Boolean; its zero-first versus nonzero-first arm order can select a different carry/add pair (also verified by player particle-bank selection: zero maps to 1, every nonzero value to 2) ([body-texture index](../../.agent-work/decomp/gameplay-200/README.md#round-19-body-texture-index-conditional)). A null-failure block before independent gates can require a positive pointer guard with an else-return, followed by those gates; nesting every later gate inside the guard moves that failure block ([fatality availability](../../.agent-work/decomp/gameplay-200/README.md#round-10-fatality-availability-guard-boundary)). Merge null/invalid failures with a short-circuit predicate only when retail shares their return; preserve null safety, lazy validation, reads and stores; keep bounds or state failures separate when retail materializes them separately ([auxiliary getter](../../.agent-work/decomp/gameplay-200/README.md#round-25-auxiliary-data-bounds-gate)) ([ducker](../../.agent-work/decomp/ai-matching/README.md#thirty-first-round-findings-and-checkpoint)). Conversely, separate real success returns inside action arms when retail materializes them separately; never omit a return ([danger avoidance/blocking](../../.agent-work/decomp/ai-matching/README.md#forty-fourth-round-findings-and-checkpoint)). In temporary board/state simulation, preserve each restoration and the final derived-state rebuild, including zero-iteration paths. Cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. In particle streams, three interleaved word copies can represent one Vec assignment, not three integer fields. Preserve API-provided byte strides and copy order; only then test same-scope declaration order for remaining stream-pointer coloring. The three Krypt tombstone callbacks close with this combination; neither a float component rewrite nor treating runtime stride as sizeof(Vec) is justified. For event buffers, derive storage extent from the consumer copy as well as producer stores: four initialized coordinate bytes do not establish a four-byte object when dispatch copies sixteen. Check the retail frame and trailing-byte consumers; larger storage fixes an overread but does not prove the unwritten bytes are unobservable. No invented initialization or compiler scaffolding.
H13 | Intrusive-list accesses differ | Link ownership + callback effects + no call | Exact typed reciprocal-store order; save next before mutating callback when observed, and reload links as retail does.
H14 | Stack slots/store order differ | Real address-taken locals + offsets + lifetimes | Reorder declarations/whole aggregates or narrow scope; separate declaration order from initialization order only while preserving execution and C89 constraints. Declaration order and block scope can interact: a neutral scope-only control does not rule out a combined declaration-only candidate. Measure each control and the combination in the real TU, keeping initialization and side effects fixed (Krypt letter layout). For a process-creation output, keep its zero initialization inside the creation guard when retail stores it there; do not hoist it across process lookup merely because the skipped path never uses the local. The four Krypt release-footstep paths verify this placement. Distinguish compiler-created by-value copies. For a formatted-text buffer, verify the callee's maximum output including its terminator before recovering a smaller extent from the retail frame; decimal input width alone is insufficient when the formatter inserts separators or abbreviates large values. No padding locals.
H15 | Coloring only | Same operations/CFG/memory accesses | Before classifying swapped integer-add operands as register coloring, check expression association: when retail adds a random increment and then a fixed increment to an accumulator, try two sequential additions to that same accumulator. Require unchanged call/read/store order and defined arithmetic at both intermediate results; do not generalize this to floating-point reassociation or publish extra field stores. The AI minimum-block-time match verifies this form. For a deadline, keep the narrowed random sample separate from the full-width sum and publish the field once: initialize the sum from the sample, add the current tick, then the fixed delay. Explicit operand order matters independently of staging: initialization needs current tick plus accumulator, while style changes need accumulator plus current tick. Both match after staged accumulation; merely reassociating the original style-change expression regressed (see [campaign rounds 21–24](../../.agent-work/decomp/ai-matching/README.md#twenty-first-round-findings-and-checkpoint)). Otherwise, at most one honest lifetime/declaration insight, then niche stop. A finite scratch-only search may identify it; use stack-sensitive scoring and verify the real TU. For a float sample returned by a getter, try assigning that sample before accumulating the real increment when retail adds into the returned register. Konquest fade-in requires this staging; swapping the operands of the single expression was neutral. Preserve the getter call, float precision boundary and addition association. When a compound predicate has a retail memory read before either boolean branch, retain that real sample explicitly rather than making it conditional through short-circuit evaluation. The background launch query closes with a validated transient-process helper, an unconditional movement sample, and a direct helper owner argument; the redundant owner local left only swapped registers. If a validated pointer feeds the next call, try passing the original member directly to the validator instead of assigning and overwriting its result local. Require the same member read count, lazy instance check and selected call owner; the Konquest monk callback closes with this direct assignment. Require the same read interval and actual later consumer. No register carousel or invented uses.
H16 | Producer/consumer move differs | Real returned object + consumer ABI | Nest a single-use result; retain the original callback owner at an untyped boundary when observed. No manufactured return contract.

## Measured examples

Apply these refinements only with the parent rule's evidence:

- H15: IF a call has the correct arguments and instruction sequence but the
  callback-address temporary differs, REQUIRE a scalar integer Boolean inversion
  evaluated once; TRY `!value` in place of `value == 0` directly in the argument.
  Both produce the same `int` result, but MWCC can allocate their intermediates
  differently. Do not add aliases or mutate the original value. This closes
  [scripted attack](../../.agent-work/decomp/ai-matching/README.md#round-209-logical-negation-closes-scripted-attack),
  where prior staged-inverse and scope controls did not. A final integer return
  can also differ: Puzzle piece generation needs `!blocked` for the retail
  normalization-then-copy sequence on its u16 result. First recover the real
  spawn-row/center-cell operation as an inline helper with an initially-zero
  result and early blocked return; Boolean spelling alone does not establish
  that missing control-flow boundary. Check the actual source
  forms tested; a random search with no improvement does not prove this rewrite
  was covered. Do not generalize the argument-value result to direct branch
  conditions: the attack-dispatcher entry-gate control was neutral (round 210).

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
- H11: Before calling a compound-condition mismatch equivalent, trace each
  branch destination and the scope of every negation. Puzzle round-start events
  require `!(A && B && C) && D`, not `!((A && B && C) && D)`: the latter can send
  an event when the score-applied guard forbids it. Check mixed true/false cases
  and lazy reads against retail, even when the current function scores above 97%.
- H05: A low score with missing loads may come from an explicitly cached
  subobject pointer rather than register allocation. Puzzle object motion closes
  after replacing its ScreenObj snapshot with motion->object at each observed
  update/read. REQUIRE the retail owner reloads and preserve their store/call
  boundaries; do not simulate the loads with volatile or dead reads. Repeated
  typed owner access lets the compiler recover the sequence naturally.
- H01: Registers live immediately before a call are not automatically its
  arguments. REQUIRE callee reads before clobber and every caller's preceding
  consumers. Puzzle random-fatality checking overwrites incoming r3/r4; both
  callers use those registers for state stores. Removing unsupported engine/force
  parameters improves both callers and leaves the callee unchanged. Check all
  declarations/call sites together; never preserve unused inferred parameters
  solely because they made an earlier fuzzy score look plausible.
- H01: A pointer-result cast can conceal an undeclared function's implicit int
  return. REQUIRE the canonical API declaration before tuning call-result
  lifetimes; include its header and convert all consumers to its actual input
  type. Puzzle rain initialization exposed this at pfx_get_emitter. Replace
  duplicate partial emitter/transform layouts with existing shared types when
  offsets agree, then compare every consumer affected by the new declaration.
- H03: IF equality against a positive constant above signed-16 range differs
  by `addis ..., 0` before `cmplwi`, REQUIRE the variable's signed uses and
  constant type; TRY removing an unsupported unsigned cast. Puzzle counter-drop
  type comparison against 0xF000 matches this way. The preceding signed >=4 gate
  and signed subtract support int; do not manufacture a no-op addition to emit
  the instruction or change signedness solely from this one opcode.
- H02: With shared-BSS base relocations, resolve the symbol's base offset and
  then its member offset separately. Do not dismiss every immediate difference
  as placement: normal Puzzle fill's start message used +0x10 where retail
  polls and writes +0x0C. That is a field error even above99% similarity, unlike
  differences caused solely by the object's position within the BSS section.
- H02/H09: For a fixed board scan expressed through a byte-offset accumulator,
  require the cell stride, row count and array extent; try typed row indexing
  with the same comparisons and exits. Both Puzzle horizontal match scans
  improved after replacing byte casts with `board + row * 8`. Compare generated
  size as well as score: an inline boundary predicate kept one score unchanged
  while adding instructions. Typed indexing did not resolve the shared-exit CFG;
  diagnose that separately before permutation.
  When nested scans share an index, preserve each loop's own pretest and
  increment, including increments after an inner loop exhausts the index.
  Express the actual typed index rather than maintaining decompiler-derived
  stride accumulators by hand. Puzzle hole compaction closes with three nested
  row loops; MWCC derives the distinct row*8 accumulators itself. A premature
  outer break hid the retail join. After recovering the CFG, one ordinary
  row/column declaration-order control resolved the remaining register swap.
  Color clearing independently confirms this sequence: eliminate the manually
  maintained row offset first, then test ordinary declarations for its real
  breaker-color, cell, row and column locals. The cell pointer remains assigned
  and consumed only in the loop; do not add initialization or fake cross-loop uses.
  For early exits after a row, distinguish checks before the row increment
  from a combined do/while condition evaluated afterward. Counter drops needs
  delay/count breaks before advancing the row. Its byte-stride traversal also
  maps directly to the established board_rows[row][column] type; preserve the
  pre-call cached spawn-row pointer separately from live board-owner reloads.
- H05: NPC command waits decrement through a saved NPC pointer, then check and
  clamp through `g_active_npc`. Preserve that observed owner reload after the
  store; audit every inlined consumer rather than introducing volatile.
  For input combinations separated by a process transfer, retain each retail
  button read and reload the player-owned pdata before the final tick write.
  Down+right followed by down+left is not equivalent to one down test with an
  OR of directions when calls can observe or mutate state. Validate ordered
  traces and replacement-owner writes ([input recovery](../../.agent-work/decomp/gameplay-200/README.md#round-4-right-stick-and-down-input-behavior)).
- H06/H14: Initialize a real output pointer at entry only when retail zeros
  that slot there. Keep an initial buffer word separate from its advanced word
  through the consume/refill join. Put real snapshots in their owning scope,
  keeping their loads at the observed point; declaration order is not initialization
  order. A table snapshot may belong after a category-specific early return
  but before the remaining guards; do not generalize its placement to adjacent
  getters without checking their load order ([Krypt evidence](../../.agent-work/decomp/gameplay-200/README.md#round-6-krypt-blurb-snapshot)).
  Consume lookahead in place only after every
  decoding use; preserve shifts even when omitting one raises fuzzy.
  A floating-point bit-estimate sequence can also separate a real position
  snapshot from its later publication. In chess cell blending, retaining the
  initial X value removed an extra object reload while preserving the retail
  Z-owner reload after the X store. Keep only snapshots with actual later uses;
  see the [body-recovery evidence](mk-chess-body-recovery.md).
  If count arithmetic precedes swap loads but publication follows them, separate
  the calculation from its store; moving a combined decrement tests both at once.
  Keep the original count for indexing and the narrowed result for exhaustion.
  If indexing adds a redundant mask, test a promoted original-count snapshot
  shared with subtraction, preserving the wrapped exhaustion test (Krypt noise).
  For an inlined square-root argument, test naming the real squared distance
  before the call only when its type preserves the existing float boundary
  and arithmetic association ([avoidance controls](../../.agent-work/decomp/ai-matching/README.md#round-164-both-avoidance-distance-arguments)).
  When inverse-length normalization still mixes argument/result registers,
  test a typed inline float helper preserving the exact bit estimate,
  arithmetic association and unordered path. Similar math helpers may differ
  on NaN; do not substitute them by formula alone. Recheck bit-cast slot order
  after extraction ([obstacle normalization](../../.agent-work/decomp/ai-matching/README.md#round-174-obstacle-inverse-length-boundary)).
  If direct independent coordinate differences swap only temporary FPRs,
  require ordinary reads with no intervening effects; try reversing their
  source expression order, then verify all emitted loads and arithmetic.
  Scheduled load order need not equal source order; X-before-Z closes the
  [obstacle comparison](../../.agent-work/decomp/ai-matching/README.md#round-177-obstacle-coordinate-closure).
  If a narrowed random sample is widened for later arithmetic, test that
  value-only assignment at the evidenced helper boundary, preserving every
  random call and store. Projectile head-on matches with widening after the
  blocking gate but before the retry update; widening at entry did not
  ([projectile closure](../../.agent-work/decomp/ai-matching/README.md#round-166-projectile-displacement-and-roll-boundary)).
  If conversion-constant addressing differs with identical arithmetic, retain
  a genuinely used scan index through its scale lookup; do not invent a constant
  pool pointer or dead use. Recheck the first path separately from later macros.
- H06/H15 | Embedded table selection has only owner-register differences |
  Confirm the array offset, element stride, access widths and all helper consumers |
  Try a typed pointer to the actual table array, replacing the container local.
  Preserve lazy count checks, narrowed random bounds and publication order;
  apply the same boundary to sibling selection arms before judging a shared
  helper regression. Do not retain an unused container alias. This closes
  [catch and ranged attack](../../.agent-work/decomp/ai-matching/README.md#round-155-typed-table-array-owners)
  and [airborne, throw and dizzy selection](../../.agent-work/decomp/ai-matching/README.md#round-159-table-array-ownership-in-three-more-selectors).
  Preserve both table reloads when a getter separates them; do not hoist or
  merge them just because their offsets agree. This does not authorize
  reordering effectful owner getters. Keep nullable-container checks and their
  result joins intact: replacing direct conditional counts with a helper is a
  separate H07/H11 trial ([attack-check control](../../.agent-work/decomp/ai-matching/README.md#round-160-nullable-count-accessor-boundary)).
- H06/H14: In coefficient decoding, form the sign threshold after unsigned
  amplitude extraction; assemble the extended level before increasing code
  length. A real `extended_base = packed * 2` retained through the bitwise
  combine expresses the stage boundary without dead uses or register forcing.
- H07/H11 | Repeated Boolean joins or scalar selections | Same ordered, lazy
  guards and result lifetime in retail consumers | Try a meaningful predicate
  or selection helper, including nested availability/start predicates. Keep
  consumer-specific loads/checks and weapon-null guards at their observed
  boundaries. Preserve failure cleanup and shared publication (`ReadSram`).
  A wrapper returning an inline helper result can merge otherwise distinct pointer/null return paths; test the direct structured traversal in that wrapper while leaving other consumers intact ([NPC lookup](../../.agent-work/decomp/gameplay-200/README.md#round-21-background-npc-lookup-return-boundary)).
  For a boundary scan followed by neighbor updates, put both operations in the
  inline helper: return on the boundary and otherwise perform the original
  calls/stores. A scan-only predicate can retain the unwanted post-scan test.
  In recursive consumers, test scoped auto_inline off while retaining explicit
  inline helpers, resetting afterward. Puzzle horizontal matching closes with
  this boundary and its visual-copy order; the AI sibling recovers the complete
  CFG but retains pointer/type coloring. Removing the barrier expanded recursive
  bodies, so verify emitted calls, function size and all siblings.
  The same ownership test applies to exceptional cleanup: if an inline update
  returns a Boolean solely to gate its cleanup, move that cleanup into the
  operation. Preserve failure paths that bypass later updates; structured
  break-to-cleanup and early normal returns can express them without goto.
  Puzzle new-piece invisibility removes the materialized result this way;
  process-creation failure still skips fade advancement. This recovers the CFG,
  not necessarily final register allocation.
  If the caller needs the scan's stopping index afterward, return that actual
  index from the complete operation rather than adding a Boolean plus a repeated
  outer guard. Rain placement uses the stopped row to position its effect;
  exhaustion must return the loop limit. This removes a duplicated guard but
  does not alone guarantee the retail exit branch or a complete match.
  For sentinel validation with callback failure exits, let the complete
  validation helper return immediately after those callbacks. Keep successful
  sentinel publication after its loop and remove a now-redundant post-loop
  sentinel test. Puzzle network initialization closes this way; the earlier
  shared post-loop test kept values live across failure calls, while moving
  successful publication into the loop changed retail block order.
  Reusing this validation in normal round fill also requires the retail
  message-pointer reload, rather than a retained local sequence snapshot.
  Measure both consumers: network initialization stays exact while fill retains
  unrelated field-address and BSS-layout differences.
  Remove redundant Boolean normalization and caller snapshots that the helper
  makes unnecessary. Check `dont_inline` and caller-before-callee order against
  actual expansion; remeasure every consumer rather than assuming a shared win. A local `dont_inline` barrier can preserve a required ordinary call while also blocking a desired validation helper. Verify emitted calls before removing it; moving the pragma inside the body around one call did not limit its effect in the Konquest bleeding callback. Restore the verified boundary when removal expands the counter body.
  See the [AI helper evidence](../../.agent-work/decomp/ai-matching/README.md#verified-source-findings).
  When a verified lookup definition follows a consumer, a shared typed inline body plus the public wrapper can preserve retail public-function order; verify that no helper call or symbol is emitted and recheck all consumers ([material lookup](../../.agent-work/decomp/gameplay-200/README.md#round-30-shared-material-subobject-lookup)).
  For a copied table search with a default value held across the scan, reuse
  an existing typed lookup only when bounds, first-match order and fallback
  agree. Verify inlining and the failure return separately: obstacle creation
  needed both its canonical lookup and a direct post-cleanup null return
  ([constraint recovery](../../.agent-work/decomp/gameplay-200/README.md#round-5-arena-obstacle-creation)).
  For repeated inventory ownership checks, preserve the bit-read result across
  the item-type lookup inside a typed helper, including the original lazy bounds
  checks. This closed three Konquest consumers. Reusing an existing nullable
  environment accessor also closed both particle coefficient setters; preserve
  retail's caller precondition rather than inventing a missing null guard
  ([gameplay campaign](../../.agent-work/decomp/gameplay-200/README.md)).
  In button dispatch, keep controller eligibility outside the suppression
  predicate when retail branches directly to the disabled exit. Explicit helper
  returns preserve the suppression join where a combined OR-return does not.
  Require the observed game-state read count: a single switch can recover one
  read that sequential getter tests accidentally repeat. Eleven dispatchers
  are report/data-value exact with this boundary; ordinary generated-name
  differences remain disclosed ([dispatcher family](../../.agent-work/decomp/gameplay-200/README.md#round-3-gameplay-button-dispatcher-family)).
- H07/H11 | FP comparison materialized through `mfcr`/bit extraction, or a
  result held across calls | Exact predicate and ordered/unordered semantics |
  Try a returned predicate or genuine early-return decision. Preserve likelihood
  calculations and lazy random calls; do not add normalization to force registers
  ([AI predicate evidence](../../.agent-work/decomp/ai-matching/README.md#verified-source-findings)).
- H07/H11 | Reset result joins later decisions | Retail result initialization,
  assignment and switch-default writes | Try a false-initialized result assigned
  the reset helper's return. Do not ignore its result or invent redundant
  defaults/assignments. Treat switch writes separately from the selected value;
  inspect stack slots after moving aggregates, regardless of fuzzy score.
- H07/H11 | Local dispatch-table copy precedes validation and shared publication |
  Table entries, stack copy and each check's nonzero versus exactly-one semantics |
  Try a typed selection/validation helper with explicit returns; verify the actual
  copy and publication, not only branch similarity
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
  Also retain explicitly compared no-op cases when retail proves their exit:
  the left-stick button's state 7 and L2's state 2 have dedicated dispatch
  comparisons despite doing no action. This permits an evidenced `case: break`,
  not invented empty branches used for allocation. Check source body order
  separately from the numeric compare tree.
- H14/H15: Separate independent loop counters; group active/saved-next iterators
  without moving assignments across callbacks. Split scalar declarations from
  initialization only without const, aggregate, scope, or lifetime changes.
  Initialized declaration reordering can reorder stores and is not coloring.
  If a branch-local scalar owner alone has the wrong register, require that
  moving its declaration neither changes name binding nor adds initialization,
  VLA, or cleanup effects. Try the function's declaration group while keeping
  its load and every use in the original branch. A same-block search cannot
  test this distinction ([defenseless closure](../../.agent-work/decomp/ai-matching/README.md#round-196-defenseless-owner-declaration-scope)).
  If repeated stale-node removal differs only in saved-successor allocation,
  require the same save-next, clear-header, destroy order; try a typed inline
  helper returning the successor. Check that it emits no extra call and preserves
  both traversals ([obstacle removal](../../.agent-work/decomp/ai-matching/README.md#round-176-stale-node-helper)).
- H06/H15: If one pointer local is reused for independently selected objects,
  require distinct source owners and unchanged lazy loads; try one pointer per
  owner. AI push selection matched with separate first/second weapon-style
  pointers after removing the permuter candidate's redundant alias. For repeated
  pure field checks with no intervening effects, direct member reads can let
  MWCC share the load with the retail register lifetime; AI throw restrictions
  matched after removing the cached state local. Verify emitted load count and
  every consumer; neither case permits extra snapshots or dummy aliases. See
  the [AI campaign evidence](../../.agent-work/decomp/ai-matching/README.md).
- H04: Sofdec combined VLC indices need 0x3FF / 0xFFF before sign extraction;
  0x3FE / 0xFFE loses signs. Exercise both signs, first/subsequent coefficients,
  and all bit alignments against retail, not a port inheriting the same bug.

## Known traps

- Check the first edge into a counted search even above 95%. A branch to the
  initial comparison means a zero target can skip every iteration. Konquest's
  character picker scored 96% with an incorrect do/while; the pre-tested loop
  and signed comparison against the narrowed random sample close the match.
  Validate zero-target call traces as well as nonzero results
  ([gameplay campaign](../../.agent-work/decomp/gameplay-200/README.md)).
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

- H06/H11: IF retail loads a selected table field inside each branch and then
  materializes Boolean return arms, REQUIRE identical table choice, field type
  and lazy reads; TRY selecting the typed scalar rather than an entry pointer
  and expressing the return arms directly. Do not generalize to address-valued
  consumers or hoist reads from unchosen tables ([random-character predicate](../../.agent-work/decomp/gameplay-200/README.md#round-13-random-character-predicate)).
