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
wrong ABI, undefined returns, or assembly workaround. A function-local `goto` is
allowed only under the AGENTS.md last-resort exception. Real MMIO stays volatile.
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

H01 | Wrong argument/return registers | All callers + callee ABI | A pointer-looking r3 may address the caller-created copy of a by-value aggregate. Verify the caller copy and declarations before adding callee pointer snapshots: inventory image creation closes with a by-value texture pair and direct array stores, while its pointer form stalled with unnecessary snapshots. Preserve the signed loop count evidenced by cmpw. Correct declarations, definitions, callbacks, and calls together; use canonical typed pointers/virtual methods. If a pointer-returning lookup immediately precedes a binding call, trace whether its result is the next call's first argument. Preserve a retail post-lookup owner reload with a used typed result local before forming the remaining arguments; nesting the lookup can cache that owner too early. The Puzzle projectile initializer closes with the canonical particle/object/flag ABI and this boundary ([evidence](puzzle-five-attempts.md#jax-and-scorpion-recovery)). Before calling a virtual-call mismatch pure coloring, verify that the typed callback receives its object argument: a legacy unprototyped vtable slot can leave r3 holding the vtable instead. Krypt biography destruction closes by using the existing StringObjVtable and passing the object explicitly. For variadic library calls, use the canonical prototype and inspect the ABI marker as well as argument registers. A live scratch register at bl is not an argument if the callee overwrites it before use or never consumes it; trace coordinate/address scratch through the callee before extending a prototype. A preceding store can leave its object address in r3: do not repeat that owner as a call argument when the callee reads only globals. Correct both the no-argument prototype and caller, then compare the entire caller TU. A forwarding wrapper may need float arguments even when it emits no floating-point instructions: collision_2 must pass incoming f1/f2 through to attack-region setup, as confirmed by its callers and callee. Do not replace forwarded values with zero. Conversely, trace registers consumed before overwrite back through the caller: an argument computed before a branch can be omitted by m2c together with its apparently dead computation. Trace event payload members through dispatch to the final consumer before naming an integer-looking word as an ID; it may carry a live object pointer. For pointer outputs, check whether the callee reads the incoming value before writing it and trace caller initialization on every path. If retail behavior depends on uninitialized stack contents, record that evidence; neither a zero initializer nor a C indeterminate read establishes faithful, defined recovery ([chess checkpoint 58](mk-chess-body-recovery.md)). If a wrapper leaves incoming r3 untouched before pdata_of_proc or xfer_proc, trace its caller before substituting aproc: the spear lookup and both retraction wrappers consume an explicit process pointer despite m2c omitting that argument. No invented argument or unused return. A void wrapper can accidentally leave the callee result in r3 and still match; when a real caller consumes that result, recover the typed return and explicit return statement, then remeasure. Process callbacks may require a float continuation/completion result even when no C caller reads it. The three remote fatality callbacks and eleven roll-up callbacks in moves.c verify explicit float returns (the roll-up callers already declare float and accept MkProcEntryFn); their canonical jump_sleep calls also preserve codegen without treating a live vtable scratch register as an extra argument.
H01 unused-argument addendum: IF a caller reloads a constant immediately before a call while retail reuses an earlier register value, REQUIRE callee ASM proving the purported argument is never read and an independent signature source. TRY removing the extra parameter from the declaration, definition, and every caller; `SFHDS_InitFhd` has one real argument in retail and RE4, and this raises `sfply_InitHn` from 99.213486% to 99.887640% without changing the exact callee. If only one owner register then differs, H15 permits one donor-backed declaration-order check; that closes `sfply_InitHn` at 100%. Do not infer ABI from a live register alone or retain unused parameters for codegen.
H02 | Wrong offset/width or opaque pointer owner | Multiple accesses + allocation/compiler layout | Inspect every differing store offset even above 99%: Sindel sonic-sound validation reached 99.94444% while still writing its active flag at +0 instead of retail +8. Correct the proven member offset and preserve unknown surrounding bytes; a high fuzzy score is not structural evidence. Distinguish an allocation returned through an output pointer from the interior base passed to its initializer. Krypt raw-particle creation returns MkPfx but initializes its VM at object+0x40; using one layout for both falsely aliases object camera-follow coordinates with VM geometry fields. Verify the enclosing owner separately from the allocated storage: collision initialization stores its allocation directly in PlyrInfo+18 and resets that same player-info record. A placeholder collision type with matching +18/+58/+5C fields can disguise an extra pointer dereference; confirm the caller before adding another nodes indirection. Recover canonical fields/arrays within proven extents; preserve unknown gaps and verify producers as well as consumers. For bitsets, derive capacity from the retail accessor bound and byte-index calculation, then check the enclosing layout; visible gameplay entries may use only part of the storage. Krypt exposes 400 coffins, but its profile accessors accept 600 bits (75 bytes), so a one-byte placeholder or 50-byte view is incomplete. Trace unnamed scalar groups to typed API arguments before naming them: Krypt VM +0x154/+0x158/+0x15C feed GXInitLightAttn k0/k1/k2, so they are light attenuation coefficients, not texture coordinates. For opaque or differently signed pointers, trace every assignment and interpreter use; choose the proven record/element type, then remove redundant casts. Keep unsigned bitmask conversions distinct from signed sentinel/division semantics ([AI command pointer](../../.agent-work/decomp/ai-matching/README.md#round-111-canonical-command-pointer)). Derive array origin separately from stride: a fixed offset used in every iteration may include a global header, not a prefix in each record. For list builders, bound every write within a full iteration before choosing capacity: a loop-entry count limit may be exceeded by multiple appends in that iteration. Player wake-up additionally shows why near-100 scores cannot excuse adjacent-field substitutions: the opponent-publication guard tests player object pointers at GameInfo+0x100/+0x16C, not pdata at +0xFC/+0x168. Correcting the owner fields after typed latch recovery also removes needless cached globals in both player-switch wrappers.
H03 | Signed compare/narrowing differs | Loads, callers, arithmetic range | Correct storage/ABI width; keep promoted accumulators full-width. A sign-extension plus subfc/subfe chain can be a 64-bit unsigned range comparison; reconstruct the comparison width before trusting carry-dependent decompiler output ([character-lock range](../../.agent-work/decomp/gameplay-200/README.md#round-17-character-lock-range-and-mask-selection), [canonical mask ownership](../../.agent-work/decomp/gameplay-200/README.md#round-18-canonical-unlock-mask-ownership)). When replacing a local view with canonical signed fields, check conditional-expression type before assignment to an unsigned local. Test an explicit conversion at that value boundary; do not alter canonical storage merely for codegen ([combo recovery](../../.agent-work/decomp/ai-matching/README.md#round-115-canonical-combo-count-table), [unchanged sibling control](../../.agent-work/decomp/ai-matching/README.md#round-116-unsigned-count-boundary-control)). A callee returning a 16-bit value does not alone establish a 16-bit function type: check the canonical declaration/definition, then express caller narrowing at its actual boundary. Avoid narrowing twice when a later cast or destination already owns the conversion. If assembly narrows before a range check, preserve modulo behavior rather than inferring saturation; test values around the wrap boundary and omit provably unreachable clamp code. Proven byte-range FP input may need direct float-to-byte conversion, not an intermediate int; never generalize to modulo narrowing. For inverse-length guards, value <= 0 and !(0 < value) differ on NaNs; preserve the decoded retail predicate before tuning codegen. For FP comparisons, decode the complete condition-register predicate, including cror and the consuming branch: m2c equality output may represent <= or >=. Check ordered/unordered behavior before choosing the C operator. When retail narrows a random result before a guard but reads the selected table entry only after success, preserve both boundaries. Store the explicitly narrowed sample in a full-width index if a short local introduces a second mask during scaling; the Krypt character monitor verifies this form without changing random draw count. For a pointer-list copy whose index is unsigned but count is signed, preserve the donor/caller-backed `i < count + 1` expression rather than casting the count before addition; `PackArgs` closes exactly only with that boundary and without a redundant pointer self-copy.
H04 | Bit extraction/RMW differs | Storage width + bit position + every use | Existing bitfield or proven mask; unsigned-byte promotion before shift. For a narrowed byte assigned into one proven bit, use that typed bit assignment instead of shifting a signed temporary; retain the original full-width nonzero test if it controls a separate action. Puzzle exit reaches exact data-value code with this distinction ([evidence](puzzle-five-attempts.md#round-18-typed-exit-request-bit)). For li 1 plus rlwimi into a byte, check the canonical one-bit assignment before accepting a raw OR: Konquest chest-camera stop matches with flags_bits.konquest_mode = 1 at +0x6C bit 6. The otherwise live r3 constant before get_game_state is bit-insertion input, not an extra function argument. The same test applies to repeated clears: li 0 plus rlwimi at distinct bit positions supports individual canonical field assignments; preserve store order and global-pointer reloads rather than combining masks (fatality player release). Distinguish a byte load from a word load followed by narrowing: on big-endian PPC, lwz plus clrlwi 24 selects the low numeric byte, not the byte at the word address. Use a neutral name when semantics are unknown. Preserve packed sign/flag bits until their last consumer; mask only the table address. For extrwi. of a single flag, the existing bitfield test can preserve the normalized extraction where a raw mask emits an unshifted test (Konquest interaction-camera gate). For a bit extraction followed by Boolean normalization, check the canonical bitfield before using a raw mask; the latter can erase the retail normalization boundary ([arena availability](../../.agent-work/decomp/gameplay-200/README.md#round-14-arena-availability)). For packed lookup-table macros, recover every replicated flag/chroma bit as well as the apparent payload shift: `mpvvlc_InitCbpSub1` reached exact only after restoring `((cbp & 3) << 14)`; a 99.679245% fuzzy score had concealed wrong immediates.
H04 staged-byte addendum: IF retail packs a four-byte code with two initial byte inserts followed by separate shifts and ORs, REQUIRE the exact byte loads, order, and 32-bit value. TRY staging `code <<= 8; code |= next_byte` for the latter bytes instead of one combined expression; `SFHDS_SetHdr` rises 85.600000%→95.971430%, then 96.400000% with promoted integer locals for reused byte values. Do not change byte order or import donor register-pin assembly; stop when only equivalent pointer-update lowering and owner coloring remain.
H04 CRI bit-window addendum: IF retail and the independent CRI donor update the current bit window with spill bits before extracting a field, REQUIRE the same unsigned word widths, bit count, and branch-local output stores. TRY `current |= following >> shift; value = current >> threshold` in a typed inline reader instead of shifting `(current | spill)` as one expression. In `mps_dec`, this raises packet 91.617355%→96.449710%, system 93.138800%→95.063095%, and pack 89.258064%→91.516130%; applying the same operation to the typed output reader raises packet to 96.873764% and system to 97.712940%. For peeks, retail shifts `current` first and ORs the spill afterward, raising packet 90.289940%→91.617355%. Preserve each reader's actual state-transition and output timing; the analogous full-word mutation regresses packet and was rejected. Recheck all consumers and exact siblings before retaining a shared helper edit.
H05 | Cached value vs retail reload | Call/sleep/aliasing store or external poll boundary | Reread the authoritative owner at each observed boundary ([subobject lookup](../../.agent-work/decomp/gameplay-200/README.md#round-22-background-subobject-lookup)). A process factory is also a call boundary: if retail reloads a global player slot before publishing it into new process data, retain that reload instead of carrying the pre-call player pointer (sidekick intro). For array members, index through that owner instead of retaining an advancing entry pointer when retail reloads the owner each iteration or after a call ([launch monitors](../../.agent-work/decomp/gameplay-200/README.md#round-27-indexed-launch-monitor-ownership)). Distinguish a helper consuming an existing data snapshot from one accepting its owner and loading the data inside; test the latter only at the proven reload boundary, and remeasure before generalizing it to neighboring paths. Preserve observed component-store order (including z/y/x); a cached object can suppress necessary owner reloads even without a call. When publishing collision state, retain event-owner reloads after earlier publications and flag reads before publishing the owner pointer; measure the resulting store schedule, then stop at pure base-register coloring ([collision publication](../../.agent-work/decomp/gameplay-200/README.md#round-26-collision-publication-and-latch-exits)). Distinguish a retained input from a buffer reused as a callee output: preserve an invariant displacement independently when retail does, rather than reading the overwritten buffer. Verify this with a nonzero output mutation and compare subsequent arguments; a zero-only stub can conceal the dependency. Only proven interrupt/debugger completion flags justify volatile on declaration and definition; check every reader/writer.
H05 CRI frame-owner addendum: IF retail reloads a reference-frame slot after
publishing a repeat-entry value and again after a time-conversion call,
REQUIRE the same slot, store order, and donor call owner. TRY direct typed slot
access for the frame-field publication and final call, retaining a frame local
only across the intervening conversion and output stores. In
`sfmpv_CalcRepeatField`, the two reload boundaries raise 90.087660% to
98.636360% without changing its typed arithmetic or exact siblings; a
long-lived cached frame pointer lost both reloads. Do not infer that the slot
may actually change without caller/alias evidence.
H05 CRI pre-call snapshot addendum: IF retail loads a GOP flag, the PTS work
owner, and its ring-buffer index immediately before a queue-read call, REQUIRE
the observed loads, donor ownership, and unchanged argument/store order. TRY
typed locals at that boundary, one at a time; `sfmpv_DecodePicAtr` rises
96.123930%→97.514244% across these three true lifetimes, then a typed initial
time snapshot raises it to 97.752140%. Do not cache a post-call value without
retail evidence that it is a pre-call snapshot; recheck exact siblings. The
same function's typed current/maximum time snapshots and an early playback-
settings owner raise it to 98.861824%, while distinct work-owner reads before
bitrate extraction and after buffer sizing reach 99.162390%. Keep those two
reads separate across the intervening calls, rather than inventing one stable
alias for the entire tail.
H05 qualifier addendum: IF retail stores through an output pointer before loading
the handle state/header but the compiler hoists that load, REQUIRE a mutable
canonical handle from the donor and mutable retail callers. TRY removing an
unsupported `const` on the public handle parameter and update its declarations;
do not add a fake aliasing read. With RE4's SFH validator/helper shape, this
closes the four `SFH_AnlyNumElem*` readers, `SFH_AnlyPackType`, and
`SFH_AnlyPketSizLen`, and improves the remaining header/element readers.
Check every consumer and jump-table data; keep genuine immutable inputs const.
H05 source-order addendum: IF retail loads independent frame fields between
two inlined conversion switches, REQUIRE matching field offsets and no
intervening side effects before moving the reads. TRY placing the typed field
copies at that boundary; `mwl_convFrmInfFromSFD` closes from 93.618324% to 100%
when its four dimension reads precede the picture-type switch, as in RE4.
H05 out-record addendum: IF retail keeps fields of a stack out-record live
across later calls, REQUIRE the successful producer, field offsets, and
unchanged values. TRY typed locals initialized immediately after that producer
and used at each consumer. `sfmps_CopyDstBuft` recovers its copy path with four
real chunk/reserve values; no alias casts or invented lifetimes are needed.
H06 | Retail retains computed value/address | Shared uses + unchanged ownership interval | Name genuine typed owners, elements, selection indices or semantic accumulators; reject redundant aliases and identity wrappers, including permuter residue. Use one proven derived owner for inherited arrays. If retail retains a render-quad pointer before a fade-completion test, snapshot that typed quad while preserving separate alpha reads for its vertex stores; the screen-fade process closes without caching alpha. If retail loads a member-array pointer once before a range loop, name that typed array snapshot; direct owner indexing may instead reload it on each iteration ([pebble range](../../.agent-work/decomp/gameplay-200/README.md#round-28-pebble-range-loop-and-array-snapshot)). For consecutive XYZ stores through one retained address, recover the canonical position pointer only for those stores. Preserve later owner reloads when retail copies the position component by component: caching the whole loop or replacing that later copy with aggregate assignment changes the observed ownership interval. This closes Krypt setup_tombstones using RwV3d from the actual RenderWare matrix. A field-address load-update instruction alone does not establish a pointer-to-field local; require supporting uses and stop after rejected owner hypotheses ([danger-zone ceiling](../../.agent-work/decomp/gameplay-200/README.md#round-20-danger-zone-field-address-ceiling)). Keep a search-key snapshot across list cleanup when retail does, while retaining a separate current-owner read for later publication ([danger-zone activation](../../.agent-work/decomp/gameplay-200/README.md#round-33-danger-zone-search-snapshot)). When retail retains a selected record index across calls and reconstructs the table address for later fields, preserve that index with ordinary typed indexing instead of caching an entry pointer; run_ending verifies this distinction after recovering its shared sentinel search. Moving a pointer assignment below the call can still produce the cached address; remove the redundant pointer and use typed indexing at each access when retail retains the index (controller right-stick dispatch). Before dismissing a save/restore as an unobservable self-store, inspect every intervening helper write: air-move initialization must restore the saved animation step because the common initializer writes 1.0f. Removing that restore changes final state even without callbacks. Keep original/advanced snapshots only while both are live; otherwise test advancing the typed pointer instead of indexing (AI move-row/scripted-command evidence). Measure variable separation and nested scope independently. For callback accounting, retail may snapshot the decoded sample count, previous byte count, and current byte count separately before wrap correction and callback argument formation; retaining those semantic values makes `ADXB_ExecHndl` exact without changing its ABI or arithmetic. For sequential likelihoods, preserve every random draw, narrow at the supported boundary and keep subsequent arithmetic full-width. A call-free inline predicate can still create a result lifetime: test its actual checks in the consumer accumulator without moving lazy calls, reads or initializers across caller setup. Remove unused wrappers only after whole-unit validation ([throw decisions](../../.agent-work/decomp/ai-matching/README.md#fiftieth-round-findings-and-checkpoint), [duck attack](../../.agent-work/decomp/ai-matching/README.md#fifty-seventh-round-findings-and-checkpoint)). When retail copies then transforms a scalar, preserve both real consumers and that value flow, such as retaining a distance before squaring it in place. Recheck temporary compiler overrides afterward ([dizzy reaction](../../.agent-work/decomp/ai-matching/README.md#forty-first-round-findings-and-checkpoint)).
For H06, when several accesses target one embedded state object and retail keeps
one base register across them, name a typed pointer to that real subobject before
forming pointers to its nested members. Require identical access offsets and
alias boundaries; `SFTIM_GetTimeSub` closes by retaining `SfdTimerState*` for
the current/start time fields and deriving its elapsed-time pointer from it.
H06 debug-call snapshot addendum | IF retail preloads repeated debug-format values before an effectful helper call but reloads the sample used in a later division, REQUIRE the exact pre/post-call load interval and unchanged arithmetic widths. TRY real typed snapshots for the repeated format values and a direct post-call member read for the division. `SFTST_Calc` rises 98.493940%→99.366670% together with the donor's end-pointer-before-margin expression; inlining `UTY_MulDiv` into `sprintf` instead regresses. Stop at remaining scheduling/coloring, not synthetic liveness.

H07 | Extra/missing helper call | Retail call boundary + signature + active inline settings | Visible inline body for expansion; out-of-line body for a real call. Repeated call-free expansion under inline-off may need a side-effect-safe typed macro. Inspect emitted calls and every consumer. A dont_inline region can also suppress helper expansion inside its function; check both boundaries, and restore evidenced caller-before-callee order before forcing policy. When extracting a repeated search, compare both consumers before retaining it: Krypt key initialization needed scoped auto_inline off rather than dont_inline, while its text updater could use the explicit inline search directly. A donor/private helper can also be present only as an inlined operation island: if retail groups a complete loop and its publications before the caller's next decision, extract that typed helper and verify no symbol is emitted; `sfmps_UpdateStreamBounds` raises `sfmps_CopyVideo` from 92.627590% to 96.151726% while preserving exact siblings/data. For branch-local CRI entry islands, test the donor's typed private inline boundary and staged output pointer, then inspect all arms and check for an extra out-of-line symbol: plain `static` emitted an unused `adxb_EntrySte` body, while `static inline` did not. This raises `ADXB_EvokeDecode` from 82.313130% to 94.949490% with all three entry arms aligned. Run a no-setting control to prove the caller still needs its out-of-line init call; do not infer necessity from the callee score alone. If explicit helpers must expand but an ordinary callee must remain a call, test scoped auto_inline off at that callee definition; caller-only placement may not change saved inlining eligibility. Verify the callee body and every consumer before retaining it. `LSC_EntryFileRange` and its wrapper close this way: object-wide `-inline noauto` also preserves the call but regresses the retail auto-expansions in `LSC_Start` and `LSC_Destroy`, while scoping the pragma to the range callee leaves the complete TU exact. Before moving a body, ensure canonical callee declarations precede its new location (including indirect shared callers); reject implicit-int/variadic fallbacks. Check its callers and callees: fixing one edge can expose a different helper for auto-inlining; preserve the evidenced dependency order and remeasure the whole unit. With nested helpers, moving only the intermediate definition may leave the outer caller eligible to inline through it; inspect the final caller and place it before the callee definition when retail requires that call.
H08 | Pointer-instance latch diamond differs | Null-before-instance reads + no call | Typed accessor returning the validated pointer or null ([bound-header latch](../../.agent-work/decomp/gameplay-200/README.md#round-23-bound-header-latch-accessor)); pass the owner if argument evaluation hoists reads. Preserve required snapshots and active dont_inline regions; no empty valid arm. Konquest grounding suspend/restore close by reusing the existing accessor: its success return retains the retail branch that an open-coded empty success arm folded away. Check for a canonical helper before adding another latch view. Spear cleanup and retraction confirm that a shared owner-typed process accessor can replace a duplicate padded latch view while preserving transfer arguments. For repeated object slots, reuse existing typed accessors while reloading the owner after each effectful visibility call ([slaughterhouse visibility](../../.agent-work/decomp/gameplay-200/README.md#round-31-slaughterhouse-visibility-latch-reuse)). Reuse an existing typed latch helper where its valid-instance return reproduces the diamond; diagnose remaining failure-path behavior separately from coloring ([NPC identity and collision lookup](../../.agent-work/decomp/gameplay-200/README.md#round-7-npc-identity-and-collision-lookup)).
H09 | Loop entry/latch differs | Zero-iteration behavior + test/update order | Recover while/do/for or assignment-in-condition; if retail loads a dynamic bound once into CTR, snapshot that bound while preserving any separate per-iteration pointer reload ([material count](../../.agent-work/decomp/gameplay-200/README.md#round-32-material-count-snapshot)); for a fixed positive extent, test bounded for/< against do/while/!= when retail uses CTR ([timer clear](../../.agent-work/decomp/gameplay-200/README.md#round-24-background-timer-counted-loop)); rotated top test may need a top guard/break; preserve polling, sleep and countdown order, including the final input recheck. An unsigned ascending index can retain cmplwi/ble plus CTR where decrementing length cannot. For a fixed table scan ending at the first matching ID, test a bounded for loop with an explicit found-entry break against a compound while condition; gore-object destroy and attach recover the retail CTR loop this way. Preserve not-found handling and first-match semantics. No dummy one-trip loop.
H09 delimiter-scan addendum | IF retail writes the scan count on the first delimiter and branches over a short-tail check, REQUIRE a proven early-found path and the same bounded cursor/ring semantics. TRY a typed inline scanner with an early return after that store; `sfmps_DecodeOneUnit` recovers the delimiter-found branch without importing RE4's `goto`. Verify no helper symbol, every exit path, and same-TU controls; residual address moves can remain a soft ceiling.
H09 fixed-ramp addendum | IF retail uses a paired CTR loop but the source hand-unrolls two ramp stores, REQUIRE identical bounds, thresholds, per-entry values, and store/conversion interleaving plus an independent donor with a simple indexed loop. TRY a single-entry `for (i = 0; i < count; i++)` source and let MWCC recover the pair; RE4's alpha-table source raises `CFT_MakeArgb8888AlpLumiTbl` from 79.481480% to 94.962960% when applied to both branches. Recheck siblings. This form regresses the clamped, interleaved high ramp of `CFT_MakeArgb8888Alp3110Tbl` from 71.316740% to 62.009050%; retain its paired source and recover the retail store order instead. If each paired iteration advances a separate sample index twice but MWCC rotates a bound-tested loop, TRY a genuine fixed pair-count loop around those same updates: both `Alp3211` ramps recover retail CTR and rise 83.737760%→85.164340%. Do not generalize to loops with side effects or distinct per-pair behavior.
H10 | Switch dispatch differs | Full finite case/default/fallthrough set + text order | Enumerate values routed by comparison ranges: m2c can omit labels sharing another arm even when the result scores above 99%. Recover case order or a proven shared decoding family/tail; check whether a second switch belongs inside an outer case and where each default is emitted ([loading-table evidence](../../.agent-work/decomp/gameplay-200/README.md#round-9-loading-table-nested-dispatch)); preserve repeated independent guards and field-publication order. A sparse switch is not always canonical: when retail tests a small ordered hardware-ID sequence directly, restore the matching `if`/`else-if` chain; `InitMetroTRKCommTable` rises from 85.961290% to 88.741936% by recovering its 2/1/0/default chain. Repeated per-arm address formation can require retaining the real index, not hoisting an element pointer before dispatch. When retail forms a table base before dispatch, retain that owner while recovering the switch; per-arm base initialization can duplicate address loads ([arena queries](../../.agent-work/decomp/gameplay-200/README.md#round-11-arena-feature-query-family), [exact arena count](../../.agent-work/decomp/gameplay-200/README.md#round-12-arena-count-and-typed-chess-getters), [body-texture count](../../.agent-work/decomp/gameplay-200/README.md#round-15-body-texture-count), [arena lookup limits](../../.agent-work/decomp/gameplay-200/README.md#round-16-arena-index-and-name-lookup)). No speculative labels.
H10 idle-state addendum | IF retail uses a dense jump table but the candidate tests the active cases through a compare tree, REQUIRE the table's exact case-to-target relocations and donor evidence for idle states. TRY separate source cases for every idle value instead of grouping them; then preserve one genuine state local across the pre-server check and post-server reload. In `sfply_ExecOne`, separate 0/5/6 cases restore the seven-entry table, and the reused state removes register-coloring residue; a plain `break` for case 0 removes the final extra branch while cases 5/6 retain their donor-backed reloads. Together with separate retail entry guards this closes 94.276310%→100% and restores `.data` value matching. Do not add empty or duplicate cases without proven table entries and unchanged effects.
H11 | Return/cleanup join differs | Actual branch destinations + effect ownership | For a nullable owner whose retail entry initializes the result before the guard, try a null-initialized result assigned from the existing validated accessor inside the guard, followed by one return. The Konquest tile-object getter closes this way; directly returning the accessor inside the guard was neutral and folded the inner joins. Preserve both owner-null and instance-validation semantics. Identify the target instruction: a branch to a final store differs from one past it. Trace failure branches to their final destination before calling a mismatch scheduling-only: a missing particle in Krypt coffin-opening effects branches to the epilogue, skipping later objects and the dirt fountain. A continue-shaped reconstruction scored above 94%; validate call traces for early and late failures as well as success. Before tuning a null join, verify that retail tests that owner at all: remove an unsupported optional-owner fallback when the actual callback contract requires the owner ([camera scripts](../../.agent-work/decomp/gameplay-200/README.md#round-29-camera-script-ownership-and-canonical-subobject-lookup)). For an extra early-failure result assignment, test a positive gate sharing the final return. If an actual state/flag still lowers differently, test a single-case switch with its existing default failure; no invented values ([background dispatch](../../.agent-work/decomp/ai-matching/README.md#sixty-second-round-findings-and-checkpoint)). For a final integer Boolean conversion, distinguish nonzero from equality-to-one even when earlier operations constrain the accumulator to 0/1: the opponent-flip query closes with != 0 while preserving both state swaps and XOR toggles. The weapon-block query confirms the same lowering for a callee result after a boss early return; verify the callee contract before treating equality-to-one as a truth test. Test Boolean arm order separately. For a two-value integer selection, prefer the real conditional over subtracting a Boolean; its zero-first versus nonzero-first arm order can select a different carry/add pair (also verified by player particle-bank selection: zero maps to 1, every nonzero value to 2) ([body-texture index](../../.agent-work/decomp/gameplay-200/README.md#round-19-body-texture-index-conditional)). A null-failure block before independent gates can require a positive pointer guard with an else-return, followed by those gates; nesting every later gate inside the guard moves that failure block ([fatality availability](../../.agent-work/decomp/gameplay-200/README.md#round-10-fatality-availability-guard-boundary)). Merge null/invalid failures with a short-circuit predicate only when retail shares their return; preserve null safety, lazy validation, reads and stores; keep bounds or state failures separate when retail materializes them separately ([auxiliary getter](../../.agent-work/decomp/gameplay-200/README.md#round-25-auxiliary-data-bounds-gate)) ([ducker](../../.agent-work/decomp/ai-matching/README.md#thirty-first-round-findings-and-checkpoint)). Conversely, separate real success returns inside action arms when retail materializes them separately; never omit a return ([danger avoidance/blocking](../../.agent-work/decomp/ai-matching/README.md#forty-fourth-round-findings-and-checkpoint)). In temporary board/state simulation, preserve each restoration and the final derived-state rebuild, including zero-iteration paths. Cleanup exception -> M08.
H12 | POD copy loop differs | Real type/size/alignment/alias semantics | Aggregate assignment for word/CTR copy; components for lfs/stfs. In particle streams, three interleaved word copies can represent one Vec assignment, not three integer fields. Preserve API-provided byte strides and copy order; only then test same-scope declaration order for remaining stream-pointer coloring. The three Krypt tombstone callbacks close with this combination; neither a float component rewrite nor treating runtime stride as sizeof(Vec) is justified. For event buffers, derive storage extent from the consumer copy as well as producer stores: four initialized coordinate bytes do not establish a four-byte object when dispatch copies sixteen. Check the retail frame and trailing-byte consumers; larger storage fixes an overread but does not prove the unwritten bytes are unobservable. No invented initialization or compiler scaffolding.
H13 | Intrusive-list accesses differ | Link ownership + callback effects + no call | Exact typed reciprocal-store order; save next before mutating callback when observed, and reload links as retail does.
H14 | Stack slots/store order differ | Real address-taken locals + offsets + lifetimes | Reorder declarations/whole aggregates or narrow scope; separate declaration order from initialization order only while preserving execution and C89 constraints. Declaration order and block scope can interact: a neutral scope-only control does not rule out a combined declaration-only candidate. Measure each control and the combination in the real TU, keeping initialization and side effects fixed (Krypt letter layout). For a process-creation output, keep its zero initialization inside the creation guard when retail stores it there; do not hoist it across process lookup merely because the skipped path never uses the local. The four Krypt release-footstep paths verify this placement. Distinguish compiler-created by-value copies. For a formatted-text buffer, verify the callee's maximum output including its terminator before recovering a smaller extent from the retail frame; decimal input width alone is insufficient when the formatter inserts separators or abbreviates large values. No padding locals.
H14 lock-boundary addendum: IF retail initializes an accumulator only after a lock call and owner lookup but compiled code saves an extra nonvolatile register, REQUIRE the accumulator has no earlier use and the owner is the only value needed across the call. TRY declaring the real accumulator without an initializer and assigning it after the lookup. `SFMPVF_GetNumFrm` closes from 88.611115% to 100% this way; keep the lock and all field reads in retail order. For a donor-backed frame search, also test caching the real frame count and initializing selection locals at the observed post-lock boundary; this raises `SFMPVF_HoldFrm` to 98.68687% and `sfmpvf_ReferNextFrmReady` to 99.595375% without match-forcing lifetimes. Stop at pure register coloring.
H14 plane-order addendum: IF retail stores computed strides and the base plane before aligning the frame height, REQUIRE matching output offsets and a donor-backed plane layout. TRY keeping width/stride calculations first, storing the two strides and base plane, then computing aligned height for the chroma addresses. RE4's typed YCC-plane helper closes MKD `SFD_CalcYccPlane` from 45.588234% to 100% with this order; the apparently equivalent all-initializers-up-front form changes scheduling. Recheck every exact sibling in the TU.
For a frame setup that reuses luma/chroma sizes in both reference planes, IF retail multiplies each size at its first forward-plane address and computes the indexed plane-array base only after picture-type updates, REQUIRE the donor's same owner and store order. TRY assigning the real size locals at those first address uses, then assigning a typed plane-array pointer after the picture-type branch. RE4's `sfmpv_SetFrmPara` shape closes MKD's same function from 86.903850% to 100% without a volatile read or register forcing; an eager array pointer instead regresses. Preserve the raw width/height locals and verify exact siblings.
H14 timecode-store addendum: IF a non-drop timecode converter writes a nominal rate to its scale output and then overwrites it with the caller rate, REQUIRE both retail stores and a donor showing the same sequence. TRY retaining the real two-store sequence and a local for the sum of frame and frame offset; RE4's shape closes MKD `sftim_Tc2Time23N`, `sftim_Tc2Time29N`, and `sftim_Tc2Time59N` to 100%. Do not add such an apparently redundant store without both pieces of evidence.
H14 frame-size declaration addendum: IF a CRI buffer-size function has the correct arithmetic and loop CFG but geometry values color differently, REQUIRE the donor's real long-lived buffer/size locals and retail load order. TRY declaring the frame-buffer pointer before the frame-size scalar, then assigning frame size at its original execution point after all C89 declarations. `sfmpv_ChkBufSiz` rises 89.105420%→89.750000%→91.012050% with those separate edits while all 32 exact siblings hold. A nested chroma-stride assignment regresses slightly; do not import it merely for donor spelling. Preserve the safe frame-table bound and stop before forcing an IV register.
H15 | Coloring only | Same operations/CFG/memory accesses | Before classifying swapped integer-add operands as register coloring, check expression association: when retail adds a random increment and then a fixed increment to an accumulator, try two sequential additions to that same accumulator. Require unchanged call/read/store order and defined arithmetic at both intermediate results; do not generalize this to floating-point reassociation or publish extra field stores. The AI minimum-block-time match verifies this form. For a deadline, keep the narrowed random sample separate from the full-width sum and publish the field once: initialize the sum from the sample, add the current tick, then the fixed delay. Explicit operand order matters independently of staging: initialization needs current tick plus accumulator, while style changes need accumulator plus current tick. Both match after staged accumulation; merely reassociating the original style-change expression regressed (see [campaign rounds 21–24](../../.agent-work/decomp/ai-matching/README.md#twenty-first-round-findings-and-checkpoint)). Otherwise, at most one honest lifetime/declaration insight, then niche stop. For two real scalar coordinates with unchanged arithmetic and swapped FPRs, try swapping only their declarations; the Puzzle super-bar start/end coordinates close this way ([evidence](puzzle-five-attempts.md#round-13-super-bar-coordinate-declarations)). For a fixed slot-cleanup loop, declaring the real slot pointer beside the index can similarly recover swapped GPRs while keeping address evaluation inside each iteration; require unchanged post-callback clearing ([edger evidence](puzzle-five-attempts.md#round-14-edger-slot-lifetime)). For a self-contained fixed-array search whose inlined body differs only by swapping the index and element-pointer registers, try restoring a real typed private helper boundary before stopping at coloring. Require identical unrolled loads, branches, stride, limit, and post-loop consumers; `sjmem_SearchFreeObj` closes `SJMEM_Create` without emitting a helper symbol. RE4's `lsc_SearchFreeObj` independently raises `LSC_Create` from 97.000000% to 99.898990%; restoring the donor's single two-query buffer-size expression then closes the remaining commutative add and reaches 100%. When one of two swapped scalar locals only mirrors a typed field, keep the genuine transformed value local and read the field directly at its consumer. Require the same field update, reload, store, and ring/index arithmetic; this closes `SFCON_UpdateConcatTime` without an artificial cumulative-value lifetime. A finite scratch-only search may identify it; use stack-sensitive scoring and verify the real TU. For a float sample returned by a getter, try assigning that sample before accumulating the real increment when retail adds into the returned register. Konquest fade-in requires this staging; swapping the operands of the single expression was neutral. Preserve the getter call, float precision boundary and addition association. When a compound predicate has a retail memory read before either boolean branch, retain that real sample explicitly rather than making it conditional through short-circuit evaluation. The background launch query closes with a validated transient-process helper, an unconditional movement sample, and a direct helper owner argument; the redundant owner local left only swapped registers. If a validated pointer feeds the next call, try passing the original member directly to the validator instead of assigning and overwriting its result local. Require the same member read count, lazy instance check and selected call owner; the Konquest monk callback closes with this direct assignment. Require the same read interval and actual later consumer. No register carousel or invented uses. When retail selects an owner before validating its member, keep that selection at the lazy validation boundary and consume the selected owner in the shared tail. Remove a redundant original-owner alias if it creates an extra copy; the Puzzle profile updater closes this way ([evidence](puzzle-five-attempts.md#round-23-profile-owner-selection-reaches-exact)). Preserve state guards and reloads; do not move side effects into an unconditional initializer. If retail compares a counter's old value after publishing its increment, try capturing the actual field postincrement; Puzzle wind-down closes the owner/value register swap this way. Require identical overflow assumptions and publication before callbacks ([evidence](puzzle-five-attempts.md#round-24-wind-down-tick-postincrement)). When an input parameter is dead after selecting a path and retail keeps its incoming register as an out-count or return accumulator, reuse that parameter only for the proven evolving value. Require m2c/call evidence for the original value's last use and identical output/return semantics; `SJRBF_IsGetChunk` closes by evolving `channel` into the available count and `size` into the Boolean return. Do not recycle live inputs merely to color registers.

H15 typed-owner addendum: IF two unrolled loops store through one subrecord
base in retail but source repeatedly spells the enclosing owner, REQUIRE a
proven subrecord type/offset, the same store sequence, and a donor-backed
owner lifetime. TRY assigning the subrecord and its array pointer before the
scalar condition, then index that typed array in both loops. This closes
`MPVCMC_InitObj` from 92.057144% to 100%; the typed owner also receives the
count, so it is not a register-only alias. Recheck exact siblings.

H15 two-call-sum addendum: IF retail calls B first, keeps its result in a nonvolatile, calls A, then adds `A + B` with the fresh result as the first operand, REQUIRE both calls to be side-effect-independent in the source contract. TRY one expression `A() + B()` in place of a staging local: MWCC evaluates the right-hand call first. A staging local (`n = B(); ... n + A()`) keeps the old operand order whichever way it is spelled. `ADXSTM_Create` closes this way (`get_num_data(sj, 0) + get_num_data(sj, 1)`).

H15 scaled-argument addendum: IF a variadic call's format and integer-to-float
argument loads use swapped GPRs (retail loads the integer field into r3 and
builds the format address in r4, then moves it) while the float math matches,
REQUIRE identical conversion, constant, and call order. TRY keeping the
converted value in its real local and applying the scale inside the argument as
a compound assignment: `MEMPRINT(fmt, size_kb *= 1.0f / 1024.0f, name)`.
`mwMemUserConfigOutofMemoryCallback` closes from 99.09% to 100%. The separate
`size_kb *= ...;` statement, a fully inline conversion, and `size_kb * k` as the
argument all stay at or below 99.09%; the last fixes the GPRs but swaps FPR
operands (M03). A sibling with more arguments can match with the separate
statement, so test per function. Several permuter candidates sharing
`tmp = (x *= k)` pointed to this form; land the honest spelling, not the
temporaries.

H15 select-spelling addendum: IF a two-value select lowers to the same `lis`/`bne`/`li` sequence as retail but its nonvolatile or temporary register is swapped with a neighbor, TRY the other spelling (`x = p ? 0 : C;` versus `x = C; if (p) x = 0;`) before touching declarations. The instructions stay identical but the allocation changes: the ternary closes `gc_aram_mwmem_heap_setup` (99.26%→100%) and lifts `gc_aram_init` 98.14%→99.61%, where declaration swaps, block scopes and call-argument staging were neutral.
H16 store-forwarding addendum: IF a call result is stored to a global and then passed on, but retail stages the argument moves in a different order, TRY assigning the call straight into the global and passing the global (`g = f(); use(g);`). MWCC forwards the stored value without a reload, even under `-opt nocse`, and the move order changes; `gc_aram_init` closes 99.61%→100% this way, and the unit links with SHA OK.

H02 near-exact offset addendum: a single differing load offset in a 99%+ function is a wrong member, not residue. `sfsee_GetInputEndPosition` read `input_transport` (+0x1354) where retail and RE4 read `output_transport` (+0x1358); after the fix, the RE4 `if/else` form for the seek position (instead of a ternary) closes `SFSEE_ExecServer`.

H02 addendum: a near-exact pair of adjacent zero stores can still expose wrong
members, not harmless coloring. Trace the offsets through every producer and
consumer before changing store order. `ADXB_DecodeHeaderAiff` closes only after
resetting the persistent decoded-sample/write-position counters at +0x88/+0x8C
instead of the per-step outputs at +0x90/+0x94, then preserving retail's two
distinct 16-bit loop-field stores. If an unused gap is only proven by extent,
keep one width-correct offset-named reserved member; do not split it into
invented fields or overlay the record with an unnecessary union.
H02/H14 aggregate-extent addendum: when an address-taken transfer aggregate has
the correct live prefix but retail reserves a larger frame, require a canonical
donor type or callee ABI plus the exact retail stack extent before restoring its
tail. Put the proven tail in the type with a size assertion; never add a separate
padding local. RE4's 0x28-byte `CFT_YCC420PLN` and MKD's 0x40 retail frame close
`SFX_CnvFrmYcc420plnToY84C44` from 99.863640% to 100% this way.
H01/H02 wide-scalar addendum: when a 64-bit value is exposed as separate high
and low scalar arguments, require the callee's retail register consumption,
every caller, and a canonical donor prototype before recovering the by-value
`long long`. Update declaration, definition, and callers together; then remove
any union used only to split the value. CRI's canonical
`SFBUF_UpdateFlowCnt(long long, unsigned int)` removes the playback-counter
overlay and raises the callee from 74.545456% to 100% without changing its three
affected callers.
For H15 integer ceiling division, if retail emits `length + size`, then decrements before division while combined `(length + size) - 1` reassociates as `size - 1 + length`, name the real count local, assign the sum, decrement it, and divide. Require defined intermediate arithmetic and unchanged publication. If a canonical donor uses that operation in multiple consumers through one private conversion helper, restore the typed inline helper and verify every consumer plus the absence of an emitted helper symbol; RE4's `mfci_ByteToSct` makes `mfCiSetSctLen` exact and improves `mfCiOpen`, while `gcCiSetSctLen` confirms the equivalent explicit-local shape. Apply the same boundary test to a donor-backed fixed-array search: `mfci_GetFreeHn` raises `mfCiOpen` from 96.600000% to 99.107140% without emitting a helper, after which only pooled-base coloring remains.
H16 | Producer/consumer move differs | Real returned object + consumer ABI | Nest a single-use result; retain the original callback owner at an untyped boundary when observed. No manufactured return contract.
H17 | Residue in an `-opt off` TU (RenderWare `rw/`, flags in `configure.py`) | TU really builds with `-opt off`; same CFG/calls | Nothing is dead-code-eliminated at opt off, so a retail store to a local that is never read *is* source evidence: restore the vendor statement (`ptr = 0;` after a free, `size = 0;` initializer declared first, `heap = RxHeapGetGlobalHeap();` whose use was a release-stripped assert). This is not a dead sink; the stored value appears in retail. `_rxPipelineDestroy`, `_rpSkinGeometryNativeSize`, and `RwIm3DRenderPrimitive` close this way; the last also shifts every later stack slot by 4. Named intermediate locals create extra register homes: `RtQuatConvertFromMatrix` closes only with the RW-style nested ternary selecting the function pointer into one local. Nonvolatile homes are ranked by reference count; at equal counts, the earlier-declared local gets the higher register. Declaring two callback results before `interrupts` closes and links `MWY_GCN_RW_ActivateGxBreakPtQueue`. If reordering declarations is neutral, the counts differ. To diagnose a two-local swap, add a code-free probe reference in a scratch (`(void)x;` or an empty `if (x) {}` emit no code at opt off): if it flips the colors, retail has one more reference to that local. Land only an honest source form with that count; never land the probe (`FrameSyncHierarchyRecurse` remains a near miss for this reason). For Boolean lowering, see M01's size-bit addendum.

H11 addendum: for pointer-membership Booleans, if retail returns immediately after each ordered identity comparison, expand compact `a == x || a == y` into sequential tests with explicit true returns and one final false return. Preserve comparison order and pointer identity; `__DVDLowTestAlarm` closes exactly with this CFG.

H11 helper addendum: when an inlined validator initializes a result, assigns distinct errors through an `else-if` chain, and returns at the tail, but retail exits immediately at every failed predicate, remove the synthetic result lifetime and use ordered direct returns. Preserve the success-only output store and verify every inlined consumer; this makes `ADX_DecodeInfoExIdly` exact while its exact siblings remain exact.

H11 validator/error addendum: when retail stores the checked owner globally,
materializes a local invalid result for null/state failures, and then expands a
shared library-error callback path, open-code those three evidenced stages in
the consumer. Preserve the callback ABI and common error return; do not replace
the materialized result with a short-circuit condition. `MPS_Destroy` closes
from 85.394740% to 100% with this shape. When a sibling's null-owner arm writes
multiple fields through the shared library-work pointer, retain one typed local
for that pointer if retail forms its address once; two direct global-member
writes can make MWCC reload it. Combined with the same expanded validator/error
path, this closes `MPS_SetErrFn` from 85.222220% to 100%. The same typed local
inside the shared error helper removes the redundant global reload and closes
`MPSLIB_SetErr` from 95.142860% to 100%; verify every inlined consumer before
changing helper lifetime.

H09 owner-walk addendum: when retail snapshots an array count and walks a typed
owner pointer through an effectful loop, keep distinct count, pointer, and index
locals in the donor-supported declaration order. If retail advances the owner
before the induction variable, place `owner++` at the end of the body rather
than combining both increments in the `for` clause. Preserve zero-iteration
behavior and callback order; this raises `MPS_Finish` from 85.315790% through
99.614040% to 100% while retaining the exact inlined destroy path.

H07 initializer-helper addendum: when retail contains an unrolled typed clear
loop followed by a folded status-check tail, recover the private status-returning
helper and the caller's real result check before adjusting the loop itself. The
`MPS_Init` helper restores the entire eight-way unrolled loop and raises the
function from 92.234566% to 96.419754%. If normal, explicit-inline, and honest
result-lifetime forms all erase only dead retail branches, stop rather than
inventing a failure path or liveness sink.

H15 single-use owner addendum: when a typed global owner is read only for one
final field store after an independent switch/FIFO sequence, remove a redundant
local alias and write through the global directly if retail forms the owner at
that final store. Preserve all preceding register packing and FIFO order;
`GXSetZTexture` closes from 87.542854% to 100% and makes its whole TU text exact.

H15 evolving-parameter addendum: when retail keeps input pointer/count
parameters in nonvolatile registers and advances them through a decode loop,
mutate those parameters directly instead of introducing cursor/remaining
copies. Require that callers cannot observe the by-value parameter updates and
preserve every bounds check, callback, and consumed-count store;
`MPSDEC_DecHdMpeg1` closes from 98.037384% to 100% with this donor-backed shape.

H15 integer-association addendum: for a decoded byte count, preserve the donor
factor grouping and operand order after confirming nonnegative bounded counts.
`channel_count * (sample_count * 2)` emits retail's shift-then-multiply order,
where `sample_count * (channel_count << 1)` leaves a commutative operand-coloring
residue. Together with unsigned shift-based 16-bit byte swaps, this closes
`ADXB_ExecOneAiff16` from 90.720340% through 99.110170% to 100%; the same typed
sample lifetime and association independently close `ADXB_ExecOneAu16` from
99.110170% to 100%. For an eight-way WAV deinterleave, first restore the donor's
mutable unsigned-16 input view; this recovers retail's register ownership and
unrolling. If one expansion still misses the `sthbrx` fold, use the donor's
side-effect-safe byte-swap macro rather than an inline helper, after proving
that every argument is a side-effect-free load because the macro evaluates it
twice. These two changes close `ADXB_ExecOneWav16` from 67.724140% to 100%.

H15 remainder/owner addendum: IF a ring-window calculation has the correct
quotient/remainder operations but the wrong nonvolatile frame, REQUIRE retail
load and dispatch order plus a donor-backed offset lifetime; TRY staging the
real dividend in the remainder local before `%`. If a later channel test still
reuses its earlier division operand while retail reloads the field, TRY the
same typed field through its original decoder owner, without a synthetic
pointer or volatile. These two independently measured changes close
`ADXB_EvokeDecode` from 95.020200% through 98.434340% to 100%.

H14 addendum: when mutually exclusive reply/error paths use same-sized address-taken packets but retail reserves one stack slot per path, keep semantically distinct typed locals rather than reusing one buffer. REQUIRE the retail offsets and independent packet construction; do not add padding locals. `TRKDoContinue` and `TRKDoStep` close with two and five real reply objects respectively. Duplicated player branches that each fill an address-taken `Vec` follow the same rule: declaring the vector inside each branch's block gives retail's per-branch slots (`resume_effect_at_plyr_num_bid` 99.96%→100%). If a final mode switch joins at one epilogue, initialize one real result before the switch, assign it in call arms, and return after the join instead of adding a default-return block.

H14 aggregate-initialization addendum: when several typed parameter aggregates
are passed together, compare retail member-store order independently from
declaration order. Assign only proven members in the observed order, including
delaying a real pointer member until its publication boundary; require identical
values and call order, and verify every inline expansion. `mwMemHeapInit` closes
from 99.15% by restoring strategy/alignment store order and the fixed-init
pointer lifetime. A low score with an identical instruction count, the same
stores in a different interleave, and a different nonvolatile save set
(`stmw r29` vs `r30`) is store-order evidence, not coloring: MWCC keeps
stack-member stores in source order, so read retail's `stw` offset sequence per
block and write members in that order (for example, a constant member before a
member loaded from a config pointer). `mwMemAllocateFixedBlockHeaps` closes from
62.07% to 100% in one edit this way.

H10 shared-return addendum: IF retail loads a float return constant once at the final join while source returns the same constant from several top-level `if`/`else if`/`else` arms, TRY dropping those arm returns in favor of one `return` after the chain. Keep genuine early returns inside `switch` cases. `p_chomper_controller` rises 99.08%->99.77% and `p_chomper2_controller` 99.13%->99.79%. If two `switch` cases share a call and one case only guards it, TRY `if (!cond) break;` falling through into `default`: `p_game_loop` drops its duplicate `do_fight_effect` call (99.21%->99.67%).

H05 read-once addendum: IF retail loads a global once before a branch and both arms test it, TRY one real local assigned before the branch (`int winning_side = winner;`). This closes `round_over`. The same function needed M13's repeated-block helper first.

H08 owner-index addendum: IF a latch diamond's CFG matches but retail folds the member offsets into its loads (`lwz 0x144(base)`) where ours materializes `&array[i]`, TRY an inline helper taking owner plus index instead of an element pointer (`fighter_severed_limb_live_object(fighter, i)`; `p_fish_attack`). Keep other consumers on their existing helper and measure each: forcing one helper through a shared macro regressed.

H14 reverse-slot addendum: at -O4, address-taken locals get stack slots in reverse declaration order, so the last declared gets the lowest offset. When retail's slots run the opposite way through the same set of locals, reverse their declarations. Then size a genuine buffer from the retail frame gap once its maximum write fits (`p_setup_konquest_map`: five `&local` handles, then `grid_text[0x14]` for a 16-byte `"%c - %d"`).

H03 timeout addendum: when an unsigned timer handles wraparound explicitly and
retail emits `subfic -1` plus the current tick, spell the wrap arm as
`(0xFFFFFFFF - start) + current` before the ordinary `current - start` arm.
Require the same wrap convention and verify every inlined consumer; this raises
both `gcCiStopTr` and `gcCiClose` without changing timeout behavior.

H10 addendum: for an ordered integer classifier, preserve staged shift/or construction of the key and assign every arm to one result local followed by a shared return when retail has that join. Do not replace the range arm or reorder sparse cases; `MPV_CheckDelim` closes exactly after removing an unsupported narrow prefix and immediate-return chain.

H06 addendum: when the same missing operation island appears in multiple callers and donor plus retail identify a private helper that is inlined at each site, restore the typed helper before duplicating its body. Require no extra emitted symbol and verify every consumer; recovering `__ARQPopTaskQueueHi` makes `__ARQInterruptServiceRoutine` exact while restoring `ARQPostRequest`'s DMA/callback/pending CFG.

H02/H07 CRI buffer addendum: when retail multiplies a buffer index by the
confirmed `SFBUF_WORK` stride and then uses offsets folded through the enclosing
handle, recover the donor-backed shifted-handle view and its private inline
helpers. Preserve the embedded supply record and per-variant clear extents;
do not flatten the supply, clear the union's full raw extent, or transplant a
donor library-init strategy when MKD retail differs. This closes fourteen
`sfd_buf` functions while retaining MKD's temporary UUID probes. Apply the same
view to PTS consumers only when their loads use those folded offsets. Keep
wrapped-range tests as direct branches instead of materializing a `contains`
Boolean, and let an unchanged queue count reload at the post-loop commit shorten
the loop-bound lifetime; this raises `SFPTS_ReadPtsQue` from 48.060240% to
93.469880%. Stop at the remaining break-versus-exhaustion join when the donor's
exact form requires codeless assembly/register forcing.

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
- H03: A cast at a helper call is converted again to the helper parameter type.
  If only one selector must be unsigned while later comparisons stay signed,
  REQUIRE evidence at both boundaries; TRY the explicit local selector rather
  than changing the shared helper or whole local's type. Puzzle center-distance
  lookup closes this way: unsigned cast passed to an int helper was ineffective,
  while unsigned typing of the whole local changed later signed comparisons.
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
  If a bounded scan has distinct found and exhausted assignments that join
  before one result test, place those assignments at the two actual exits of a
  structured loop. Require retail's call-then-increment-then-bound order; a
  pre-loop false initializer can keep an extra nonvolatile register live, while
  a post-loop index comparison adds a non-retail test. In `sfply_IsBpaOn`, an
  unconditional `for` with a found break and an `++index >= 8` exhaustion break
  removes the extra save and closes the function without the donor's goto.
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
  actual expansion; remeasure every consumer rather than assuming a shared win.
  Extracting exact inline conversion helpers made `sfxzmv_MakeCnvZTbl` exact but
  initially dropped its public wrapper from 100% to 0%; the donor's scoped
  `dont_inline` boundary plus explicit wrapper-local tag searches preserved both.
  A local `dont_inline` barrier can preserve a required ordinary call while also blocking a desired validation helper. Verify emitted calls before removing it; moving the pragma inside the body around one call did not limit its effect in the Konquest bleeding callback. Restore the verified boundary when removal expands the counter body.
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
  Prefer a typed inline helper to a macro when the macro changes output
  reference ownership or stack placement and a related CRI source uses a
  helper; verify no helper symbol is emitted and keep limits at TU scope only
  if narrower scoped controls fail.
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
  For two strength-reduced pointer walks, declaration order and assignment order
  may intentionally differ: `sfxzmv_MakeCnvZTbl` closed from 99.828766% by
  declaring destination before source while assigning source before destination.
  Require independent pointer lifetimes and unchanged store order.
  Conversely, when retail recomputes a fixed-record address from the loop index,
  preserve `entry = &table[i]` inside the loop instead of advancing one pointer:
  `LSC_ExecServer` closes from 96.111115% to 100% with identical lock, test,
  call, stride, and bound. Require retail `mulli/add` address formation; do not
  replace a proven load-update pointer walk merely because indexing matches a
  donor.
  Likewise, when a donor inline helper's result must outlive another argument,
  assign it to its own local before the call instead of nesting the helper:
  the full width-then-height lifetime shape raised
  `SFX_CnvFrmYcc420plnToY84C44` from 98.443184% to 99.86364%.
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
- H05/H06: When a callback and its opaque object are loaded from shared work
  state before that state is mutated, snapshot both typed values before the
  mutation if retail keeps them live. `SFXLIB_Error` became exact from
  88.947365% by retaining both the callback and callback-object locals across
  the error-count increment; reloading the object at the call changed lifetime
  ranking and every inlined consumer.
- H10: Two adjacent integral priority cases may still require an explicit
  `switch` even when an `if`/`else if` is behaviorally identical. Require the
  retail compare/join ordering and unchanged queue stores; restoring cases 0/1
  raised Dolphin `ARQPostRequest` from 90.183910% to 97.885056% while preserving
  its exact interrupt-service consumer.
- H07/H11: If retail performs a simple typed threshold check through an inlined
  Boolean helper, recover the helper and compare its normalized result rather
  than widening the operand to coerce CFG. `sfx_IsEnoughWork(s32)` removed an
  accidental 64-bit comparison and raised `SFX_Create` from 91.55625% to 96.75%.
- H04: Sofdec combined VLC indices need 0x3FF / 0xFFF before sign extraction;
  0x3FE / 0xFFE loses signs. Exercise both signs, first/subsequent coefficients,
  and all bit alignments against retail, not a port inheriting the same bug.
- H07/H09: A donor table-construction macro may encode both the value formula
  and the compiler's loop-lifetime boundaries. Require identical endpoint
  normalization, segment bounds, ramp divisor, and final remap before copying
  it. Keeping the donor's separate loop counters and pointer-write tail made
  `sfxzmv_MakeOrgZ32TblByCCIR` exact from 91.09009%; a generic equivalent loop
  had obscured the source boundary.

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
