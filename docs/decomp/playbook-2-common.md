# Matching playbook, tier 2: common detail

Detail for the H rules indexed in [tier 1](playbook-1-core.md). Read only the
section whose rule the triage selected. Every item requires its rule's evidence
before use; named symbols are exemplars that reached exact.

## H01

ABI: wrong argument/return registers. REQUIRE all callers and the callee ABI.

- A pointer-looking r3 may address the caller-created copy of a by-value
  aggregate. Verify the caller copy before adding pointer snapshots (inventory
  image creation passes a texture pair by value). Keep a `cmpw`-evidenced
  signed loop count.
- If a pointer-returning lookup precedes a binding call, check whether its
  result is the next call's first argument. Keep a retail post-lookup owner
  reload with a used typed result local; nesting the lookup can cache the owner
  too early (Puzzle projectile initializer).
- Virtual calls: verify the typed callback receives its object. A legacy
  unprototyped vtable slot can leave r3 holding the vtable (Krypt biography
  destructor: `StringObjVtable` + explicit object). For raw vtable casts
  (`((void**)self->vtbl)[0x44/4]`) with retail `lwz r12,0(this); lwz
  r12,off(r12)`, call the real virtual through the canonical class
  (`RefreshOption__11SpreadSheetFi`: +0x44/+0x48/+0x4C are
  Update/RefreshCollection/RefreshOption). Typed virtuals prove dispatch, not
  class layout or destructor contract.
- Variadic calls: canonical prototype; check the CR marker (`crclr`) as well as
  argument registers.
- A register live at `bl` is not an argument if the callee overwrites it
  first. A preceding store can leave its object address in r3; do not add it
  when the callee reads only globals (Puzzle random-fatality check). Fix the
  prototype and every caller together.
- IF a TU-local prototype adds an argument whose register is only a leftover
  from a preceding store, REQUIRE the callee definition and retail register
  consumption. TRY correcting that prototype and call together; the phantom
  argument can cause an extra owner reload (`r_pz_fighter_block_hi`).
- Unused-argument addendum: IF a caller reloads a constant before a call where
  retail reuses an earlier register, REQUIRE callee ASM proving the parameter
  is never read plus an independent signature source, TRY removing it from
  declaration, definition, and callers (`SFHDS_InitFhd`, closing
  `sfply_InitHn` with one H15 declaration-order check).
- Conversely, trace registers consumed before overwrite back to the caller:
  m2c can drop an argument computed before a branch together with its
  apparently dead computation. A wrapper that leaves r3 untouched before
  `pdata_of_proc`/`xfer_proc` may forward an explicit process pointer (spear
  wrappers), not `aproc`.
- A forwarding wrapper can need float arguments with no FP instructions:
  `collision_2` passes incoming f1/f2 through. Do not replace them with zero.
- Trace event payload words through dispatch before naming them IDs; they may
  carry live object pointers.
- Pointer outputs: check whether the callee reads the incoming value before
  writing, and trace caller initialization on every path. If retail depends on
  uninitialized stack, record it (N08); neither a zero initializer nor an
  indeterminate read is faithful.
- Returns: a void wrapper can leave a callee result in r3 and still match; if a
  real caller consumes it, recover the typed return. Process callbacks may
  need a float result no C caller reads (moves.c fatality and roll-up callbacks
  as `MkProcEntryFn`).
- A pointer-result cast can hide an undeclared function's implicit int return.
  Include the canonical header first (`pfx_get_emitter`), and replace duplicate
  partial layouts with the shared type when offsets agree.
- Wide scalars: a 64-bit value passed as high/low scalars needs callee register
  consumption, every caller, and a donor prototype; recover the by-value
  `long long` and drop the splitting union (`SFBUF_UpdateFlowCnt(long long,
  unsigned int)`).
- Under opt off, `GXBool` locals can drop redundant masks; keep full-width
  masks/enums and public prototypes.

## H02

Layout: wrong offset/width or opaque owner. REQUIRE multiple accesses plus the
allocation or compiler layout.

- One differing offset in a 99%+ function is a wrong member
  (`sfsee_GetInputEndPosition` read `input_transport` +0x1354 instead of
  `output_transport` +0x1358; Sindel sonic sound's flag is +8, not +0; Puzzle
  fill's message is +0x0C, not +0x10; mk_chess rescue is +0x50, not +0x4C).
  Adjacent zero stores can also be wrong members (`ADXB_DecodeHeaderAiff`
  resets +0x88/+0x8C, not +0x90/+0x94). Adjacent-field substitutions are
  behavior bugs (player wake-up tests object pointers at GameInfo+0x100/+0x16C,
  not pdata at +0xFC/+0x168).
- Separate an allocation returned through an output pointer from the interior
  base passed to its initializer (Krypt raw particle: `MkPfx`, VM at +0x40).
  Verify the enclosing owner separately from the storage (collision init
  stores into PlyrInfo+18). A placeholder with matching fields can hide an
  extra dereference.
- Recover canonical fields and arrays within proven extents. Keep an unproven
  gap as one width-correct offset-named reserved member; no invented split or
  union.
- Owner-type addendum: a sparse overlay whose padding stands in for pointer
  members, a sibling-type view, or a pointer squeezed into a 32-bit integer is
  the wrong owner even at an unchanged score; it breaks wider-pointer builds.
  Use the canonical typed slot or owner.
- Aggregate extent (with H14): an address-taken aggregate with a correct prefix
  but a larger retail frame needs a donor type or callee ABI plus the exact
  stack extent; put the tail in the type with a size assertion, never a
  padding local (`CFT_YCC420PLN` in `SFX_CnvFrmYcc420plnToY84C44`).
- Bitsets: capacity comes from the accessor bound and byte index (Krypt shows
  400 coffins but accepts 600 bits, 75 bytes).
- Trace unnamed scalar groups to typed API arguments before naming them (Krypt
  VM +0x154..+0x15C feed `GXInitLightAttn` k0/k1/k2).
- Opaque or differently signed pointers: trace every assignment and use, pick
  the proven element type, remove redundant casts. Keep unsigned bitmask
  conversions distinct from signed sentinel/division semantics.
- Arrays: derive origin separately from stride; a fixed offset in every
  iteration may be a global header. An array view one word shorter than the
  ELF symbol means a header (fatality loader: +4+i*12 in 184 bytes is a header
  plus fifteen records). Bound every write in a full iteration before choosing
  list-builder capacity.
- Identical cursor stride does not prove identical ownership; follow each
  pointer load (mk_chess drone +0x108 vs embedded mode cursors). Runtime owners
  are not interchangeable with similar static tables.
- MWCC can place the vptr after members declared before the first virtual
  (`IRefCntRes` needs virtuals before `reference_count` for +0/+4). Nested
  anonymous structs inside a union can measure size 1; pointer unions restored
  FighterSlot 0x0C / PlyrInfo 0x6C. Probe layout and recheck callers.
- Opcode variants in one instruction union can have different trailing fields;
  recover a typed variant and verify both producer and dispatcher.
- Hardware and low-memory owners (with H05/H18): VI registers are volatile MMIO
  at 0xCC002000; the PAD/OS reset byte is an unqualified absolute RAM variable;
  DVD DI/PI and SI banks are absolute volatile arrays. GX FIFO byte/half/word/
  float writes share one absolute volatile union at 0xCC008000. Equal addresses
  do not make the same compiler-visible owner, and one wrong owner can cause
  large scheduling differences. Once confirmed, check every sibling regardless
  of score. Do not infer volatility from an address or generalize a FIFO union.
- Shared-BSS base relocations: resolve the symbol base, then the member offset.
- Fixed board scans (with H09): replace byte-offset accumulators with typed row
  indexing (`board_rows[row][column]`); MWCC derives stride accumulators
  itself. Keep each nested loop's own pretest and increment, and distinguish
  checks before the row increment from a do/while condition after it.
- CRI buffers (with H07): an index scaled by the `SFBUF_WORK` stride with
  offsets folded through the handle needs the donor's shifted-handle view and
  private inlines. Keep the embedded supply record and per-variant clear
  extents; do not transplant donor init where MKD retail differs. Keep
  wrapped-range tests as direct branches, not a `contains` Boolean.

## H03

Signedness and narrowing. REQUIRE loads, callers, arithmetic range.

- Keep promoted accumulators full-width. A sign extension plus `subfc`/`subfe`
  can be a 64-bit unsigned range compare.
- `addis x,v,H; cmplwi x,L` tests `(L - (H << 16)) mod 2^32`; H=0, L=0xC602 is
  positive 0xC602. m2c prints such constants as negative unsigned values;
  signed equality against the positive constant restores `addis`. An `addis
  ...,0` before `cmplwi` against a constant above signed-16 range points to
  removing an unsupported unsigned cast (Puzzle 0xF000).
- Conditional expressions assigned to unsigned locals: test an explicit
  conversion at that boundary; do not change canonical storage.
- A 16-bit callee return does not prove a 16-bit function type; narrow at the
  caller's real boundary, once. A cast at a helper call converts again to the
  parameter type; if only one selector must be unsigned, use an explicit
  local selector.
- Narrowing before a range check means modulo, not saturation; omit provably
  unreachable clamps. Proven byte-range FP input can convert straight to a byte.
- FP compares: decode the full CR predicate including `cror`; m2c equality can
  mean `<=` or `>=`. `value <= 0` and `!(0 < value)` differ on NaN.
- A narrowed random result used in a guard and a later table read keeps both
  boundaries; keep the narrowed sample in a full-width index if a short local
  adds a second mask.
- `i < count + 1` with unsigned index and signed count, not a cast before the
  addition (`PackArgs`).
- Branch-free signed compare: `xor`, `srawi 1`, `and`, `subf`, sign-bit
  extract. The ANDed operand picks the direction (lhs gives `rhs < lhs`).
  Evaluate lhs=-1, rhs=0 before mapping to `<` or `>`.
- Unsigned timer wrap with `subfic -1`: spell the wrap arm
  `(0xFFFFFFFF - start) + current` before `current - start` (`gcCiStopTr`).
- TRK connection checks keep a local `BOOL` and separate early returns.
- A stored `Vec` component may be rounded relative to the incoming FP argument;
  read the stored component when retail does.

## H04

Bits. REQUIRE storage width, bit position, every use.

- A narrowed byte assigned into one proven bit uses the typed bit assignment;
  keep the full-width nonzero test if it controls a separate action.
- `li 1` + `rlwimi` into a byte is a one-bit field assignment
  (`flags_bits.konquest_mode = 1`); the live r3 constant is insertion input,
  not an argument. `li 0` + `rlwimi` at distinct positions are separate field
  clears; keep their store order and reloads.
- `lwz` + `clrlwi 24` selects the low numeric byte, not the byte at the address.
- For `extrwi.` of one flag, or extraction followed by Boolean normalization,
  the existing bitfield preserves the normalization a raw mask erases.
- Mask-then-shift: assign the whole expression to the decoded value.
- Keep packed sign/flag bits until their last consumer; mask only the address.
- Check index masks against allocation extent (VM dirty-page LUT needs a 13-bit
  index); per sibling.
- Packed LUT macros: restore every replicated flag bit
  (`mpvvlc_InitCbpSub1` needs `((cbp & 3) << 14)`). Sofdec combined VLC indices
  mask 0x3FF/0xFFF before sign extraction.
- Staged bytes: a four-byte code built with two inserts then shifts/ORs is
  `code <<= 8; code |= next;` per byte, with promoted int locals for reused
  bytes (`SFHDS_SetHdr`).
- CRI bit window: `current |= following >> shift; value = current >> threshold`
  in a typed inline reader; peeks shift `current` first, then OR the spill
  (`mps_dec`). Keep each reader's state-transition timing.

## H05

Cached value vs retail reload. REQUIRE a call, sleep, aliasing store, or poll
boundary.

- Process factories are call boundaries: reload a global player slot after one
  (sidekick intro).
- Index array members through the owner when retail reloads it each iteration
  or after a call, instead of an advancing entry pointer.
- A cached subobject pointer can remove loads without any call; use
  `owner->object` at each observed access (Puzzle object motion). Keep
  component store order (z/y/x).
- Test a helper that loads from its owner only at a proven reload boundary.
- A buffer reused as callee output is not the retained input; preserve an
  invariant displacement separately. Verify with a nonzero output mutation.
- Qualifier: if retail stores through an output pointer before loading handle
  state but the compiler hoists the load, remove an unsupported `const` on the
  handle (`SFH_AnlyNumElem*`). For a vector setter with interleaved component
  loads/stores, verify writable caller inputs and related setter signatures;
  unsupported pointee const can hoist all source loads before destination
  stores (`camera_set_center_of_rotation`). Keep genuine const (M12).
- Pre-call snapshots: loads immediately before a call become typed locals at
  that boundary, one at a time (`sfmpv_DecodePicAtr`). Keep distinct reads
  across intervening calls instead of one long alias.
- CRI frame slot reloaded after a publication and after a conversion call:
  access the slot directly; keep a frame local only across the conversion
  (`sfmpv_CalcRepeatField`).
- Independent field reads between two inlined switches: place the typed copies
  at that boundary (`mwl_convFrmInfFromSFD`).
- Stack out-record fields live across later calls: typed locals right after
  the producer (`sfmps_CopyDstBuft`).
- A global loaded once and tested in both arms: one local before the branch
  (`round_over`).
- Owner fields reread after an inlined helper that writes the stack (fast
  sqrt): take a typed position pointer after the helper and read through it
  (`mks_bgnd_cam_offset_away`). A field null-tested before it is cached is
  tested directly, then assigned (`p_create_decoy`).
- A callback and its opaque object loaded before state mutation: snapshot both
  (`SFXLIB_Error`).
- Input combinations across a process transfer: keep every button read and
  reload pdata; down+right then down+left is not one down test.
- `volatile` only for proven interrupt/debugger completion flags, on both
  declaration and definition.

## H06

Retained value or address. REQUIRE shared uses and an unchanged ownership
interval. Reject redundant aliases, identity wrappers, and permuter residue.

- Snapshots retail keeps: a render-quad pointer before a fade test (separate
  alpha reads); a member-array pointer before a range loop; a search key across
  list cleanup, with a separate owner read for publication; a real subobject
  pointer when one base register serves several accesses (`SFTIM_GetTimeSub`
  keeps `SfdTimerState*`).
- Consecutive XYZ stores through one address use the position pointer for
  those stores only; later component copies keep their owner reloads
  (`setup_tombstones` with `RwV3d`).
- A field-address load-update alone does not prove a pointer-to-field local.
- An index retained across calls with table addresses rebuilt later stays a
  typed index, not a cached entry pointer (`run_ending`, right-stick dispatch).
- Save/restore is not a self-store if an intervening helper writes the field
  (air-move init must restore the animation step).
- Keep original and advanced snapshots only while both are live.
- Callback accounting can need separate sample/previous/current snapshots
  before wrap correction (`ADXB_ExecHndl`).
- Keep every random draw; narrow at the evidenced boundary, full-width after.
- A call-free inline predicate can create a result lifetime; test its checks in
  the consumer accumulator without moving lazy calls.
- Copy-then-transform keeps both values (a distance before squaring it).
- Debug-format values preloaded before an effectful call use typed snapshots;
  a post-call read stays direct (`SFTST_Calc`).
- A missing island repeated in several callers is a private inline helper
  (`__ARQPopTaskQueueHi`); verify no emitted symbol.
- Owners and table selection: a typed pointer to the actual table array
  replaces a container local; keep both reloads when a getter separates them.
  One pointer per independently selected owner. Repeated pure field checks can
  read members directly. Select the typed scalar, not the entry pointer, when
  each branch loads one field before Boolean returns.
- Scope and placement (with H14): initialize an output pointer at entry only if
  retail zeros it there; keep snapshots in their owning scope at the observed
  load point; a table snapshot may follow a category early return. Separate
  count arithmetic from its later publication. Name a squared distance before
  an inlined sqrt only if its type keeps the float boundary. Inverse-length
  normalization can need a typed inline float helper keeping the bit estimate.
  Swapped temporary FPRs on independent coordinate differences: reverse the
  source expression order. Widen a narrowed random sample at the evidenced
  helper boundary. Consume lookahead in place only after its last decoding use.
  If conversion-constant addressing differs with identical arithmetic, keep a
  genuinely used scan index through its scale lookup, not a constant-pool
  pointer.
- Coefficient decoding: form the sign threshold after unsigned amplitude
  extraction; keep a real `extended_base = packed * 2`.

## H07

Inline boundary. REQUIRE the retail call boundary, signature, and active inline
settings. Verify emitted calls, helper symbols, and every consumer.

- A `dont_inline` region also suppresses expansion inside its function, and
  moving the pragma inside a body does not narrow it. Restore evidenced
  caller-before-callee order before forcing policy.
- `static` vs `static inline`: plain `static` can emit an unused out-of-line
  body (`adxb_EntrySte` in `ADXB_EvokeDecode`); `static inline` does not.
- Keep a callee out of line while explicit helpers still expand: scoped
  `auto_inline off` at the callee definition (`LSC_EntryFileRange`). A static
  helper retail calls under `-inline auto`: object-level `-inline noauto`
  (dvdlow `SeekTwiceBeforeRead`, `__OSCacheInit`), rechecking every function.
- Deleting a nested `static inline` lowers the enclosing function's cost and
  `-inline auto` may then expand it into callers
  (`plyr_weapon_trail_hide` inlined 4x). Check callers' instruction counts.
- Moving a body: canonical declarations must precede it (no implicit int). One
  fixed edge can expose another auto-inline; with nested helpers, place the
  final caller before the callee when retail requires the call.
- Run a no-setting control to prove a pragma is needed.
- A donor or private helper may exist only as an inlined island; extract it
  when retail groups a complete loop and its publications
  (`sfmps_UpdateStreamBounds`). An unrolled clear plus folded status tail is a
  status-returning helper (`MPS_Init`); stop if every honest form erases dead
  retail branches.
- IF a wrapper loads an endpoint then compares it with itself before a shared
  inlined loop, REQUIRE the same clamp shape in sibling wrappers and the same
  loop body. TRY passing that endpoint through the existing inline clamp before
  entering the loop (`ani_to_end` and `animpdata_ani_to_end`).
- Inline limits: test `inline_max_size` and `inline_max_total_size` together in
  an isolated diagnostic. This proves expansion, not retail pragma values.
  Prefer a typed inline helper over a macro when the macro changes reference
  ownership or stack placement. The TU-level limit is M13.
- Predicate and selection helpers (H07/H11):
  - Repeated Boolean joins or selections: a meaningful helper, keeping
    consumer-specific loads, null guards, failure cleanup, and shared
    publication (`ReadSram`).
  - A bounded scan with found and exhausted assignments joining before one
    test: assign at the two loop exits, e.g. an unconditional `for` with a
    found `break` and a `++index >= 8` `break` (`sfply_IsBpaOn`, no `goto`).
  - Return the stopping index (limit on exhaustion) instead of a Boolean plus
    a repeated guard. Put a boundary scan and its neighbor updates in one
    helper.
  - Move cleanup gated by a returned Boolean into the operation. Structured
    break-to-cleanup and early returns replace `goto`.
  - Sentinel validation with callback failures returns right after the
    callbacks; successful publication stays after the loop.
  - A wrapper returning an inline result can merge pointer/null paths; use the
    direct traversal in that wrapper.
  - A lookup defined after its consumer: shared inline body plus the public
    wrapper preserves public order.
  - Reuse an existing lookup only when bounds, first-match order, and fallback
    agree. Preserve the caller's precondition; do not add a null guard.
  - Button dispatch: eligibility stays outside the suppression predicate; a
    single switch can recover one game-state read.
  - FP comparison via `mfcr` or a result held across calls: a returned
    predicate or early-return decision.
  - A reset result joining later decisions: false-initialized result assigned
    the helper's return.
  - A simple threshold check through an inlined typed Boolean helper
    (`sfx_IsEnoughWork(s32)` removed an accidental 64-bit compare).
  - Extracting inline conversion helpers can zero a public wrapper; the
    donor's scoped `dont_inline` restored both (`sfxzmv_MakeCnvZTbl`).
  - Recursive consumers: scoped `auto_inline off` with explicit inline helpers.
  - A local dispatch-table copy before validation: a typed selection/
    validation helper with explicit returns; verify the copy itself.
  - Inventory ownership checks keep the bit-read result across the item-type
    lookup inside the helper, with the original lazy bounds checks.
  - After extracting a helper, remove the Boolean normalization and caller
    snapshots it makes redundant.
- Decoder phases: a typed inline phase boundary with genuine inputs/outputs,
  including cursor setup when the phase owns it; an outer decode owner can span
  init to publication. Flattening nested phases can undo gains.
- Repeated fill/clear with an advancing cursor: a pointer-to-pointer helper
  that stores and advances the caller's cursor, over the full array extent.
- Escape-window shifts: a typed inline value helper can fix scheduling (Dc11)
  but is context-dependent (it regressed the Nintra first-code phase).
- A donor table-construction macro can encode loop-lifetime boundaries; keep
  its separate counters (`sfxzmv_MakeOrgZ32TblByCCIR`).
- An `inline` helper can still emit a call under the TU settings; a canonical
  field-access macro can replace it locally (RenderWare default atomic
  callback). Audit evaluation count.

## H08

Latch diamond. REQUIRE null-before-instance reads and no call in between.

- Use a typed accessor returning the validated pointer or null; pass the owner
  if argument evaluation hoists reads. No empty valid arm.
- IF retail explicitly skips a null assignment on the valid instance arm,
  REQUIRE the separate null-owner arm and the same pointer test. TRY selecting
  the validated pointer with `condition ? pointer : 0`
  (`destroy_sobj_ctrl_proc`).
- Reuse an existing accessor before adding a latch view; its success return
  keeps the branch an open-coded empty arm folds away (Konquest grounding). A
  shared owner-typed process accessor can replace a padded latch view.
- Repeated object slots: reuse accessors and reload the owner after each
  effectful call.
- Direct-return addendum: when source merges null arms but retail keeps them
  separate, `return object;` on the valid instance and `return 0;` in each
  failure arm (`reaction_xfer_him`, `get_mission_state`). Measure per owner.
- Owner-index addendum: retail folds member offsets into loads where source
  forms `&array[i]`: an inline helper taking owner plus index
  (`fighter_severed_limb_live_object`).
- Direct-owner accessors suit adjacent load/validation; cached forms suit real
  intervening effects. Extracting a latch can change a caller's inlining; check
  every caller.

## H09

Loops. REQUIRE zero-iteration behavior and test/update order.

- A dynamic bound loaded once into CTR: snapshot the bound, keep per-iteration
  pointer reloads. A fixed positive extent: test bounded `for`/`<` against
  do/while/`!=`. An unsigned ascending index keeps `cmplwi`/`ble` + CTR.
- A rotated top test may need a top guard. Keep polling, sleep, and countdown
  order, including the final input recheck.
- IF retail tests a frame bound at loop entry and exact equality after the
  sleep, REQUIRE both edges and zero-iteration behavior. TRY a pretested loop
  with an equality `break` after the sleep (`ani_to_frame_x_call`).
- Check the first edge into a counted search: a branch to the initial compare
  means a pretested loop that can skip every iteration.
- Table scan to the first matching ID: bounded `for` with a found `break`.
- IF retail places a hoisted loop-invariant load in a block after the final
  return (`b tail` ... `tail: lwz; b header`), REQUIRE an otherwise matching
  `for(;;)` block order. TRY a pretested loop whose condition only becomes
  constant after optimization, such as `while (!found)` on a flag zeroed at
  entry. The front end rotates it, so backend LICM appends the new preheader at
  the function end. `for(;;)`, `while (1)` and `(1 == 1)` are folded too early
  and keep the preheader inline (`drone_ai_fetch_next_AIState`).
- Delimiter scan: a typed inline scanner returning right after the count store
  (`sfmps_DecodeOneUnit`, no `goto`).
- Fixed ramp: a paired CTR loop from a single-entry `for` when the donor has
  one (`CFT_MakeArgb8888AlpLumiTbl`); not for clamped interleaved ramps. A pair
  of updates per iteration can be a fixed pair-count loop.
- Owner walk: separate count, pointer, and index locals; `owner++` at the end of
  the body when retail advances it before the index (`MPS_Finish`).
- Retail `mulli`/`add` per iteration: `entry = &table[i]` inside the loop, not
  an advancing pointer (`LSC_ExecServer`).
- Two strength-reduced pointer walks can need declaration order different from
  assignment order (`sfxzmv_MakeCnvZTbl`).
- No dummy one-trip loop.

## H10

Switch dispatch. REQUIRE the full finite case/default/fallthrough set and text
order.

- m2c can omit labels that share an arm even above 99%. Verify finite bounds
  before accepting a decompiled default arm.
- Recover case order, shared tails, nested switches inside outer cases, and
  where each default is emitted.
- A small ordered hardware-ID sequence can be an `if`/`else if` chain
  (`InitMetroTRKCommTable`); two adjacent priority cases can require a real
  `switch` (`ARQPostRequest`).
- Per-arm address formation: retain the real index; a table base formed before
  dispatch stays one owner.
- Explicitly compared no-op cases get an evidenced `case N: break;`.
- Idle states in a dense table: one case per table entry; keep one state local
  across the pre-server check (`sfply_ExecOne`).
- Dead-branch addendum: a compare tree ending in an unreachable extra `b join`
  needs one empty label just beyond the tested range sharing the other empty
  cases' `break` (`go_into_twitch_death`, `go_into_major_pain`).
- Jump-table layout: an edit that raises `.text` but lowers `.data` moved case
  addresses; reject it (`mk_chess_set_game_mode`).
- A float constant loaded once at the join: one return after the chain; keep
  early returns inside cases (`p_chomper_controller`). Two cases sharing a call
  where one only guards: `if (!cond) break;` falling into `default`
  (`p_game_loop`).
- Ordered classifier: staged shift/or key, each arm assigns one result, shared
  return (`MPV_CheckDelim`).
- No speculative labels.

## H11

Joins. REQUIRE actual branch destinations and effect ownership.

- Identify the target instruction: a branch to a final store differs from one
  past it. Trace failure branches to their destination before calling a
  difference scheduling-only (Krypt coffin effects: one failure skips every
  later object).
- Nullable owner with result initialized before the guard: null-initialized
  result, assigned from the validated accessor inside the guard, one return.
- First verify retail tests the owner at all; drop an unsupported fallback.
- Extra early-failure assignment: a positive gate sharing the final return. If
  a real state still lowers differently, try a one-case switch with the
  existing default.
- Final integer Booleans: `!= 0` and `== 1` differ even when the value is 0/1.
  Test arm order separately; a two-value selection uses the real conditional,
  whose zero-first vs nonzero-first order picks different carry code.
- IF retail keeps one tested state word live through multiple bit tests, then
  loads 1 or 0 in separate arms, REQUIRE the same input value and branches.
  TRY a block-local state sample with explicit result arms instead of a compact
  `&&` assignment (`is_he_blocking_throw`).
- A null-failure block before independent gates: positive pointer guard with
  an else-return, then the gates.
- Merge null/invalid failures with `||` only when retail shares the return;
  keep bounds/state failures separate. Separate success returns inside action
  arms when retail does. Never omit a return.
- Pointer-membership Booleans: sequential tests with explicit returns
  (`__DVDLowTestAlarm`).
- Inlined validators: ordered direct returns instead of a result plus
  `else if` chain (`ADX_DecodeInfoExIdly`). A stored owner, local invalid
  result, and expanded error callback are three open-coded stages
  (`MPS_Destroy`); one typed local for the shared work pointer
  (`MPS_SetErrFn`, `MPSLIB_SetErr`).
- Trace negation scope: `!(A && B && C) && D` is not
  `!((A && B && C) && D)`.
- Temporary board simulation: keep every restoration and the final rebuild,
  including zero-iteration paths.
- Genuine cleanup edges: M08.

## H12

POD copies: aggregate assignment for word/CTR copies, components for
`lfs`/`stfs`. Three interleaved word copies in a particle stream can be one
`Vec` assignment. Keep API byte strides and copy order; only then test
declaration order. Event buffer extent comes from the consumer copy as well as
producer stores. No invented initialization.

## H13

Intrusive lists: exact typed reciprocal-store order; save next before a
mutating callback; reload links as retail does. Repeated stale-node removal
that differs only in saved-successor allocation: a typed inline helper
returning the successor, keeping save-next, clear-header, destroy order.

## H14

Stack slots and store order. REQUIRE real address-taken locals, offsets,
lifetimes.

- At -O4, address-taken locals get slots in reverse declaration order. Reverse
  declarations when retail runs the other way; size a genuine buffer from the
  frame gap once its maximum write fits (`p_setup_konquest_map`).
- Declaration order and block scope interact; measure each and the
  combination with initialization fixed. Block scope means an existing
  `if`/loop/`else` body: a bare `{ }` added only to scope a temporary is force
  matching, not honest source (`validate_save_location`'s dummy slot local).
- MWCC keeps stack-member stores in source order. Same instruction count, a
  different store interleave, and a different `stmw` set are store-order
  evidence: write members in retail's `stw` offset order
  (`mwMemAllocateFixedBlockHeaps`, `mwMemHeapInit`).
- Separate declaration from initialization only without const, aggregate,
  scope, or lifetime changes; reordered initialized declarations reorder
  stores. A branch-local scalar with the wrong register can move to the
  function's declaration group, keeping its load in the branch.
- Initialize after a lock/lookup when the accumulator has no earlier use
  (`SFMPVF_GetNumFrm`). Keep a process-creation output's zero init inside the
  creation guard.
- Mutually exclusive paths with per-path stack slots: distinct typed locals
  (`TRKDoContinue`, `TRKDoStep`); a per-branch `Vec` declared in each block.
  A final switch joining one epilogue: result initialized before the switch.
- A donor inline result that must outlive another argument gets its own local
  before the call. Keep separate loop counters.
- CRI frame-plane order: store strides and base plane before aligning height
  (`SFD_CalcYccPlane`); assign size locals at first use and the plane-array
  pointer after the picture-type branch (`sfmpv_SetFrmPara`); declare the
  frame-buffer pointer before frame size (`sfmpv_ChkBufSiz`). A timecode
  converter's two scale stores are both real (`sftim_Tc2Time29N`).
- Formatted-text buffers: size from the callee's maximum output including the
  terminator.
- Distinguish compiler-created by-value copies. No padding locals.

## H15

Coloring with identical operations. Check association and staging first; then
at most one honest lifetime/declaration insight; then stop (tier 4). No register
carousel or invented uses.

- Integer association: when retail adds a random increment and then a fixed
  one, try two sequential additions to the accumulator. Operand order matters
  independently of staging; this does not extend to FP reassociation.
  `channel_count * (sample_count * 2)` differs from
  `sample_count * (channel_count << 1)` (`ADXB_ExecOneAiff16`). Ceiling division
  `length + size` then decrement: name the count, add, decrement, divide.
- Two-call sum: `A() + B()` evaluates B first; a staging local keeps the old
  order (`ADXSTM_Create`). Nested call arguments behave the same: in
  `f(g(1), g(2))` MWCC calls `g(2)` first, so a staged local for the second
  string argument is residue (`_trial_add_required_sequence`).
- Select spelling: `x = p ? 0 : C;` vs `x = C; if (p) x = 0;` give identical
  instructions but different allocation (`gc_aram_mwmem_heap_setup`).
- Scaled argument: `MEMPRINT(fmt, size_kb *= 1.0f / 1024.0f, name)`
  (`mwMemUserConfigOutofMemoryCallback`); siblings may differ.
- FP temporaries: MWCC numbers temporaries per statement (a pooled constant
  first) and colors newest-first into the lowest free volatile FPR. Stage a
  negation and a copy into result locals in earlier statements
  (`ai_side_clearances`). Pool label numbers are not pool order.
- Narrow parameters: an explicit cast or u16 local hoists the argument load; an
  implicit conversion to a `unsigned short` parameter does not (`drone_loop`).
  Needs callee evidence of the narrow parameter; a u16 return makes callers
  re-normalize.
- `!value` instead of `value == 0` in an argument can change allocation;
  it does not transfer to branch conditions.
- Declarations: swap two real scalar coordinate declarations; declare a slot
  pointer beside its index.
- Volatile FPRs around an inlined helper: the helper's temporaries take the
  lowest FPRs and the caller's float locals that live across it take the next
  ones in declaration order (earliest lowest). Give each value its own local
  (position, delta) instead of reusing one, name the square products, and
  write the dot product in retail operand order. These levers are coupled:
  each alone is neutral or reversed, so search them jointly on the host
  (`mks_get_victim_to_tr_dot`: normalized components declared first, z before
  x).
- Remove redundant aliases: read a mirrored field directly
  (`SFCON_UpdateConcatTime`); write through a single-use global owner at the
  final store (`GXSetZTexture`); keep owner selection at the lazy validation
  boundary; pass the original member to a validator.
- Staging: assign a float getter sample before accumulating; keep a memory read
  that precedes both Boolean branches unconditional; capture a counter
  postincrement when retail compares the old value; stage the dividend in the
  remainder local.
- Restore a typed private search helper when an inlined fixed-array search
  only swaps index and pointer (`sjmem_SearchFreeObj`, `lsc_SearchFreeObj`).
- A typed subrecord owner for unrolled stores (`MPVCMC_InitObj`).
- Evolve dead input parameters as out-count/return accumulators or decode
  cursors when retail keeps their registers (`SJRBF_IsGetChunk`,
  `MPSDEC_DecHdMpeg1`), only after their original last use.
- WAV deinterleave: the donor's mutable u16 view, and its byte-swap macro when
  each argument is a side-effect-free load (`ADXB_ExecOneWav16`).

## H16

Nest a single-use result; keep the original callback owner at an untyped
boundary. Store forwarding: `g = f(); use(g);` forwards without a reload even
under `-opt nocse` (`gc_aram_init`). No manufactured return contract.

- IF a script wrapper moves a saved string result through an extra register,
  REQUIRE retail's argument-fetch order. TRY direct producer expressions in
  both call arguments, then verify the compiler preserves that order
  (`_konquest_start_nis_anims_load`).
- IF scalar script arguments follow a string resolver call, REQUIRE whether
  retail retains the original argument frame or reloads `current_args`.
  TRY the observed snapshot boundary: retain the frame for
  `_pfx_spawn_at_bid`, reload it for `_bgnd_set_fx_z_offset`.

## H17

`-opt off` TUs (RenderWare `rw/`; flags in `configure.py`). Nothing is
dead-code-eliminated, so a retail store to an unread local is source evidence:
restore the vendor statement (`ptr = 0;` after a free, a declared-first
`size = 0;`, an assert-only `heap = RxHeapGetGlobalHeap();`). Named
intermediates create register homes; a nested ternary into one local can be
required (`RtQuatConvertFromMatrix`). Nonvolatile homes rank by reference count;
at equal counts the earlier-declared local gets the higher register. If a
declaration swap is neutral, the counts differ: a scratch-only probe
(`(void)x;` or empty `if (x) {}` emit no code) that flips the colors shows
retail has one more reference. Never land the probe. Boolean lowering: M01
size-bit addendum.

## H18

Fixed addresses: declare MWCC absolute-address variables
(`extern unsigned long __OSBusClock : 0x800000F8;`,
`extern volatile unsigned long __PIRegs[] : 0xCC003000;`), not pointer-cast
macros. A volatile deref macro pins loads against neighboring stores; a plain
deref macro lets MWCC CSE reloads retail keeps. Match SDK signedness (a `long`
apploader offset gives `cmpwi`). Once the clock is an absolute variable, spell
SDK tick macros per use.

## H19

Retail materializes an inlined test as 0/1 (`li r0,1; b; li r0,0; cmpwi`):
spell the helper `if (cond) { return 1; } return 0;` or the macro
`(cond) == 0 ? 0 : 1`. `return cond != 0` folds away. Drop extra `? 1 : 0` at
call sites once the macro has it (`ENTRY_IS_DIRECTORY` in `DVDOpen`).

## H20

MWCC colors scalar-replaced struct members after declared scalars. When retail
keeps an inlined bit-reader/cursor state in low registers, rewrite the state as
declared scalar locals driven by macros (CRI `BS_*` shape), order the
declarations, and load init values straight into them (`mps_dec`).

## H21

An inlined helper loop whose fresh locals color above the loop temps while
retail reuses dead outer registers: open-code it and reuse the function's
existing locals; MWCC colors a reused local's second web after the loop temps
(`AddRequestingCS_ByThread`).

If inlined table accessors preserve the operations but miscolor the scaled
index and table owners, check whether unrelated branch definitions share one
caller local. Try a named runtime-table base with typed indexing and scope each
definition owner to the branch that uses it. Fold a single-use owner directly
into its call argument when appropriate; do not retain an unused assignment
inside the argument. This recovered the table registers in
`drone_ai_check_attack`; its remaining parameter nonvolatile swap is a hard stop
(tier 4 Hard stops: simplify threshold).

IF a function sits under consumer-scoped `opt_propagation off` and the only
residue is volatile coloring inside an inlined helper (same loads, owner and
partial-sum registers shifted by one, e.g. retail `plyr_pdata` r5/count r4 vs
built r4/r3), REQUIRE that the residue vanishes with the pragma removed while
another region then regresses (whole-TU control). TRY making the shared helper
copy-free: a ternary body instead of `if (p) return x; return 0;`, and direct
member expressions as call arguments instead of a staged `style` local. With
propagation off, each inline expansion keeps its parameter and return copies,
and the allocator colors around them; propagation-on users are unaffected, so
the helper edit is safe for its other callers (`ai_weapon_style_move_count` in
`drone_ai_check_dont_touch_attack_phase2`). Both edits are needed; either alone
is neutral. The same rewrite regressed `drone_ai_check_attack`, whose residue
is a parameter pair, not helper copies.

## H22

One-row operand, constant, or copy residue. Try one at a time:

- `x = REG; x &= ~m;` as two statements when retail loads into the variable.
- Constant on the left (`0x80000000 <= p`) or swapped `==` operands for
  `cmplw` order. MWCC 1.2.5 colors the local written second into r3; make one
  side a direct expression instead of a local (`DVDLowRead`).
- `a = b = 0` when one zero register feeds both stores (rightmost first).
- `x += y + c` instead of a temporary sum.
- `(unsigned int)` casts on `%x` pointer arguments create retail's copies.
- An inlined `{ call(); return 1; }` for an unread `li rN,1` after a call.
- A stored-but-unread local from a parameter (debug assert value).
- A pointer local declared before its sibling in an inner loop.

For SDK code, check other matched decomps on the same compiler (bfbb, prime,
pikmin2) for their local-vs-expression split.

## H23

If a symbol lives in this object per `splits.txt`/`symbols.txt` but the source
only declares it `extern`, define it in the unit with retail constness (`.data`
is non-const) and data order, values from the split `.obj` blocks. Probe with
zero initializers first. MWCC folds bases for same-unit objects (mixer tables in
`__MIXSetPan`, `MIXInitChannel`).

## H24

MWCC 1.2.5 reserves one stack slot per inline expansion when a helper returns
through a single-exit local (`T v; if ... else ...; return v;`), sized by the
return type. Early-return and macro spellings reserve nothing. Pick the
spelling that gives retail's `stwu` immediate; scratch-compile first
(`__MIXGetVolume` as `u32` in `MIXInitChannel`: 43 x 4 = 0xB0).

## H25

A thin inline latch wrapper evaluates its argument once, before the body; a
macro re-reads it at each use. When the argument is a global pointer or is read
before the null test, copy it to a typed local, then use the macro
(`player = plyr_pdata; MK_LIVE(player->p, player->p_instance)`). Keep the helper
when the argument is a latch address reused in member writes, or when retail
reuses another variable's zero on the failure path.

## Traps

- ELF `NOBITS` sections are zero storage; never read `sh_offset` as their
  initializer before "fixing" a data-value mismatch.
- Automated extraction must parse C identifiers (`0.0f * body` is not a pointer
  declaration); reject malformed generated source.
- Keep host (PC) branches out of Deception source; portable corrections need
  retail and behavioral evidence.
