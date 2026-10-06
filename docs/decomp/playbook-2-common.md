# Playbook tier 2: common (H rules)

Detail for H rules indexed in [tier 1](playbook-1-core.md). Read only the
section triage picked. Every bullet needs its rule's evidence first. Symbols =
exemplars that reached exact. `[da]` = mk-da import (tier 1).

## H01

ABI: wrong arg/return regs. REQUIRE all callers + callee ABI.

- Saved scalar param rotates around a call, word ops identical: REQUIRE
  caller type, callee param, stored member agree. TRY canonical signedness in
  public decl + def; implicit unsigned->signed makes another web
  (`pfxvm_spawn_line_1i`). Check every header consumer; match alone does not
  justify a type change.
- Pointer-looking r3 can be caller-made copy of a by-value aggregate. Verify
  caller copy before adding snapshots (inventory image: texture pair by
  value). Keep `cmpw`-proven signed loop count.
- Flag init into original word + separate passed word = aggregate-address ABI
  of one-word POD. Confirm all callers, callee reads, pinned-compiler
  value-vs-pointer probe, then fix canonical prototype. Typed initializer /
  constructor helpers recover each expansion's original/copy homes
  (`get_mkproc_nostack`, `bleed_startup`). Never model compiler copies as
  unused 2nd array element. Recheck header consumers together.
- Pointer-returning lookup before a binding call: result may be next call's
  first arg. Keep retail post-lookup owner reload with used typed result
  local; nesting the lookup caches owner too early (Puzzle projectile init).
- Virtuals: typed callback must receive its object. Legacy unprototyped
  vtable slot can leave r3 = vtable (Krypt biography dtor: `StringObjVtable`
  + explicit object). Raw `((void**)self->vtbl)[0x44/4]` with retail `lwz
  r12,0(this); lwz r12,off(r12)` -> call real virtual via canonical class
  (`RefreshOption__11SpreadSheetFi`: +0x44/+0x48/+0x4C =
  Update/RefreshCollection/RefreshOption). Typed virtual proves dispatch, not
  layout or dtor contract.
- Variadic: canonical prototype; check `crclr` and arg regs.
- Reg live at `bl` is not arg if callee overwrites first. Prior store can
  leave its object address in r3; don't add it when callee reads only globals
  (Puzzle random-fatality check). Fix prototype + all callers together.
- TU-local prototype adds arg whose reg is only leftover from prior store:
  REQUIRE callee def + retail reg consumption. TRY fixing prototype + call
  together; phantom arg can add owner reload (`r_pz_fighter_block_hi`). Null
  aggregate-field store can leave zero in apparent arg reg (`repel_a_from_b`):
  check every path, fix private defs too. Keep sibling signatures whose
  callers explicitly stage that arg, even if body ignores it.
- Unused arg: caller reloads constant before call where retail reuses earlier
  reg. REQUIRE callee ASM never reads param + independent signature source.
  TRY removing from decl, def, callers (`SFHDS_InitFhd`; `sfply_InitHn`
  closed with one H15 decl-order check).
- Reverse: trace regs consumed before overwrite back to caller. m2c can drop
  an arg computed before a branch with its "dead" computation. Wrapper leaving
  r3 untouched before `pdata_of_proc`/`xfer_proc` may forward explicit process
  pointer (spear wrappers), not `aproc`.
- Forwarding wrapper can need float args with no FP instrs: `collision_2`
  passes f1/f2 through. Don't replace with zero.
- Event payload words: trace through dispatch before calling them IDs; may be
  live object pointers.
- Pointer outputs: does callee read incoming value before writing? Trace
  caller init on every path. Retail depends on uninit stack -> record (N08);
  zero init and indeterminate read both unfaithful.
- Detached OS thread entry, retail never sets return reg: REQUIRE every
  registration uses detached attr 1, OSCreateThread puts entry in saved PC,
  OSExitThread discards return on that path. TRY void entry + explicit
  address cast at OS registration (`adxm_mwidle_proc`); keep canonical OS
  API. Not for joinable threads whose exit value is read.
- Returns: void wrapper can leave callee result in r3 and still match; real
  caller consumes it -> recover typed return. Process callbacks may need
  float result no C caller reads (moves.c fatality/roll-up callbacks as
  `MkProcEntryFn`).
- Pointer-result cast can hide implicit int return. Include canonical header
  first (`pfx_get_emitter`); replace duplicate partial layouts with shared
  type when offsets agree.
- 64-bit passed as hi/lo scalars: REQUIRE callee reg use, every caller, donor
  prototype. Recover by-value `long long`, drop splitting union
  (`SFBUF_UpdateFlowCnt(long long, unsigned int)`).
- Opt off: `GXBool` locals can drop redundant masks; keep full-width
  masks/enums and public prototypes.
- Interleaved FP/GPR params, only float/pointer setup order differs: REQUIRE
  same order in every retail caller + callee arg saves, banks preserve
  physical ABI. TRY canonical param order in header, def, all calls
  (`pfx_emitter_run_frame`: emitter, float, pfx). One caller's scheduling is
  not evidence.
- Conditional call target `[da]`: retail picks one of two functions, calls
  via `r12` with no extra `mr`; function-pointer local adds moves. REQUIRE
  both targets real symbols. TRY `(cond ? a : b)(args)`; keep retail branch
  sense (`beq`/`bne` picks first arm) (`_rwForAllEdges`).

## H02

Layout: wrong offset/width or opaque owner. REQUIRE multiple accesses +
allocation or compiler layout.

- One differing offset in 99%+ function = wrong member
  (`sfsee_GetInputEndPosition`: `input_transport` +0x1354 vs
  `output_transport` +0x1358; Sindel sonic flag +8 not +0; Puzzle fill msg
  +0x0C not +0x10; mk_chess rescue +0x50 not +0x4C). Adjacent zero stores can
  be wrong members (`ADXB_DecodeHeaderAiff` resets +0x88/+0x8C, not
  +0x90/+0x94). Adjacent-field swaps are behavior bugs (wake-up tests object
  ptrs at GameInfo+0x100/+0x16C, not pdata +0xFC/+0x168).
- Separate allocation returned via out-pointer from interior base passed to
  its initializer (Krypt raw particle: `MkPfx`, VM at +0x40). Verify enclosing
  owner apart from storage (collision init stores into PlyrInfo+18).
  Placeholder with matching fields can hide extra deref.
- Recover canonical fields/arrays inside proven extents. Unproven gap = one
  width-correct offset-named reserved member; no invented split or union.
- Wrong owner even at unchanged score: sparse overlay with padding standing in
  for pointers, sibling-type view, pointer in 32-bit int. Breaks wide-pointer
  builds. Use canonical typed slot/owner.
- Aggregate extent (+H14): address-taken aggregate, prefix right, retail frame
  bigger -> REQUIRE donor type or callee ABI + exact stack extent. Tail goes in
  the type with size assert, never padding local (`CFT_YCC420PLN` in
  `SFX_CnvFrmYcc420plnToY84C44`). Frame bigger, body exact `[da]`: real local
  array/struct whose every byte the code reads (`HVQM4DecodeAdpcmCh2` `u8
  header[4]`, 0x30 -> 0x38).
- Literal over-allocation `[da]`: retail allocates more than verified struct.
  REQUIRE literal allocator arg + full access/stride/layout evidence. Keep
  literal size + minimal verified type; no invented tail fields for `sizeof`
  (`HVQM4DecSoundCreate`: 0x20 alloc, 0x18 context).
- Bitsets: capacity from accessor bound + byte index (Krypt: 400 coffins
  shown, 600 bits/75 bytes accepted).
- Unnamed scalar groups: trace to typed API args before naming (Krypt VM
  +0x154..+0x15C feed `GXInitLightAttn` k0/k1/k2).
- Opaque/differently signed pointers: trace every assign/use, pick proven
  element type, drop redundant casts. Unsigned bitmask conversions stay
  distinct from signed sentinel/division semantics.
- `void*` read-only input keeps field address (extra `addi`) retail folds into
  each access `[da]`: REQUIRE retail only reads it. TRY `u8*` (`const` neutral)
  (`_rpWriteSectRights`).
- Arrays: origin apart from stride; fixed offset every iteration may be global
  header. Array view one word shorter than ELF symbol = header (fatality
  loader: +4+i*12 in 184 bytes = header + 15 records). Bound every write in a
  full iteration before picking list capacity.
- Same cursor stride != same owner; follow each pointer load (mk_chess drone
  +0x108 vs embedded mode cursors). Runtime owners != similar static tables.
- MWCC can put vptr after members declared before first virtual
  (`IRefCntRes`: virtuals before `reference_count` for +0/+4). Nested
  anonymous structs in a union can measure size 1; pointer unions restored
  FighterSlot 0x0C / PlyrInfo 0x6C. Probe layout, recheck callers.
- Opcode variants in one instruction union can differ in trailing fields;
  typed variant, verify producer + dispatcher.
- Hardware/low-memory owners (+H05/H18): VI regs = volatile MMIO 0xCC002000;
  PAD/OS reset byte = unqualified absolute RAM var; DVD DI/PI and SI banks =
  absolute volatile arrays; GX FIFO byte/half/word/float = one absolute
  volatile union at 0xCC008000. Equal address != same compiler-visible owner;
  wrong owner can cause big scheduling diffs. Confirmed -> check every
  sibling regardless of score. Don't infer volatile from address; don't
  generalize FIFO union.
- Shared-BSS base relocs: resolve symbol base, then member offset.
- Callback-record loads can look like coloring. REQUIRE each loaded reg traced
  to indirect call args, same fields checked in record producers. TRY fixing
  record layout + callback prototype together, keep producer store order
  (`sfmps_CopyPrvate`: handle cb +4, object cb +8, object +0xC;
  `SFD_SetUsrSj` stores 4th arg in handle-cb slot).
- Fixed board scans (+H09): typed row indexing (`board_rows[row][column]`),
  not byte accumulators; MWCC derives strides. Keep each nested loop's
  pretest + increment; check-before-row-increment != do/while after.
- CRI buffers (+H07): index scaled by `SFBUF_WORK` stride with offsets folded
  via handle -> donor shifted-handle view + private inlines. Keep embedded
  supply record and per-variant clear extents; don't transplant donor init
  where MKD differs. Wrapped-range tests = direct branches, not `contains`
  Boolean.

## H03

Signedness, narrowing. REQUIRE loads, callers, arithmetic range.

- Transition mask: outer + repeat-loop copies coalesce, retail keeps both.
  REQUIRE local is bits, every value to signed callee fits. TRY unsigned mask
  local, keep canonical prototype + nonempty guard (`pz_fighter_laugh`, mask
  3, optional bit 8). Type change alone before loop helper; no empty branch or
  duplicate local.
- Promoted accumulators stay full width. Sign extend + `subfc`/`subfe` can be
  64-bit unsigned range compare.
- Signed quotient before loop, unsigned ratio divides inside branch: REQUIRE
  retail `mulhw` before loop / `mulhwu` in branch plus independently supported
  conversion local. TRY signed quotient + branch-local unsigned numerator;
  explicit low-16 fractional narrowing only where `clrlwi` proves it
  (`AXRNA_SetSfreq`). A root unsigned local or cast-only form can hoist both
  ratio halves and add saved lifetimes.
- `addis x,v,H; cmplwi x,L` tests `(L - (H << 16)) mod 2^32`; H=0, L=0xC602 =
  positive 0xC602. m2c prints these as negative unsigned; signed equality
  vs positive constant restores `addis`. `addis ...,0` before `cmplwi`
  against const > s16 range -> remove unsupported unsigned cast (Puzzle
  0xF000).
- `cmplwi` mode chain with unsigned field/literals in retail `[da]`: `== 4U`
  literals (`gop_decode`).
- Ternary into unsigned local: test explicit conversion at that boundary;
  don't change canonical storage.
- 16-bit callee return != 16-bit function type; narrow once at caller's real
  boundary. Cast at helper call converts again to param type; only one
  selector unsigned -> explicit local selector.
- Narrow before range check = modulo, not saturation; drop provably dead
  clamps. Proven byte-range FP input converts straight to byte.
- FP compares: decode full CR predicate incl `cror`; m2c equality can mean
  `<=`/`>=`. `value <= 0` != `!(0 < value)` on NaN.
- Narrowed random result used in guard + later table read keeps both
  boundaries; keep sample in full-width index if short local adds 2nd mask.
  Packed ID, proven low-16 payload, retail narrows once before full-width
  index shift: TRY `(unsigned short)id` into unsigned full-width index; short
  local adds mask, bitmask expr gives other web
  (`load_model_from_slot_transl`).
- `i < count + 1`, unsigned index + signed count; no cast before the add
  (`PackArgs`).
- Branch-free signed compare: `xor`, `srawi 1`, `and`, `subf`, sign-bit
  extract. ANDed operand picks direction (lhs -> `rhs < lhs`). Evaluate
  lhs=-1, rhs=0 before mapping to `<`/`>`.
- Unsigned timer wrap with `subfic -1`: wrap arm `(0xFFFFFFFF - start) +
  current` before `current - start` (`gcCiStopTr`).
- TRK connection checks: local `BOOL` + separate early returns.
- Stored `Vec` component can be rounded vs incoming FP arg; read stored
  component when retail does.
- Byte locals `[da]`: retail `lbz` then to float or small-int subtract; `u32`
  local swaps regs/adds conversions. TRY block-local `u8`, cast at use; signed
  delta gets own cast (`_rwGeneratePerspClippedVertexZLO`).
- Byte-sized board fields do not imply byte-sized scan indices/output locals:
  REQUIRE full-width increments/unsigned bounds and no per-iteration narrowing.
  TRY unsigned32 loop indices and selected-coordinate output pointers, keeping
  sampled owner fields byte-sized (`mk_chess_drone_handle_the_big_chill_opening_move`).
- Signed extrema `[da]`: retail `cmpw` on extrema, donor keeps `u8` min/max.
  TRY `int` extrema, sample stays `u8` (`GetAotBasis`).
- Pointer-difference index `[da]`: retail signed divide (`divw` / signed shift
  seq) by record size; unsigned index merges webs/drops temp. TRY `s32` index
  (`PipelineNodeDestroy`, /40).

## H04

Bits. REQUIRE storage width, bit position, every use.

- Narrowed byte into one proven bit: typed bit assignment; keep full-width
  nonzero test if it gates a separate action.
- `li 1` + `rlwimi` into byte = one-bit field assign
  (`flags_bits.konquest_mode = 1`); live r3 const = insert input, not arg.
  `li 0` + `rlwimi` at distinct positions = separate field clears; keep store
  order + reloads.
- `lwz` + `clrlwi 24` = low numeric byte, not byte at address.
- `extrwi.` of one flag, or extract + Boolean normalize: existing bitfield
  keeps normalization raw mask erases.
- Mask-then-shift: assign whole expression to decoded value.
- Keep packed sign/flag bits to last consumer; mask only the address.
- Index masks vs allocation extent (VM dirty-page LUT = 13-bit index); per
  sibling.
- Packed LUT macros: restore every replicated flag bit (`mpvvlc_InitCbpSub1`
  needs `((cbp & 3) << 14)`). Sofdec combined VLC indices mask 0x3FF/0xFFF
  before sign extract.
- 4-byte code via two inserts then shifts/ORs = `code <<= 8; code |= next;`
  per byte, promoted int locals for reused bytes (`SFHDS_SetHdr`).
- CRI bit window: `current |= following >> shift; value = current >>
  threshold` in typed inline reader; peeks shift `current` first, then OR
  spill (`mps_dec`). Keep each reader's state-transition timing.

## H05

Cached vs retail reload. REQUIRE call, sleep, aliasing store, or poll
boundary.

- Process factories = call boundaries: reload global player slot after
  (sidekick intro).
- Retail reloads owner per iteration or after call: index members through
  owner, not advancing entry pointer.
- Two material traversals reload atomic + geometry between calls: REQUIRE
  callback boundary + both owner loads. TRY `ctrl->atomic->geometry` per
  traversal (`uv_scroll_dual_pass`); cached geometry adds saved reg.
- Cached subobject pointer can drop loads with no call; use `owner->object` at
  each access (Puzzle object motion). Keep component store order z/y/x.
- Helper loading from owner: test only at proven reload boundary.
- Buffer reused as callee output != retained input; keep invariant
  displacement separate. Verify with nonzero output mutation.
- Const qualifier: retail stores via out-pointer before loading handle state,
  compiler hoists load -> drop unsupported `const` on handle
  (`SFH_AnlyNumElem*`). Vector setter with interleaved component load/store:
  verify writable caller inputs + related setter signatures; bogus pointee
  const hoists all source loads before dest stores
  (`camera_set_center_of_rotation`). Keep genuine const (M12).
- Pre-call snapshots: loads right before call -> typed locals at that
  boundary, one at a time (`sfmpv_DecodePicAtr`). Distinct reads across
  intervening calls, not one long alias.
- CRI frame slot reloaded after publication + after conversion call: access
  slot directly; frame local only across conversion (`sfmpv_CalcRepeatField`).
- Independent field reads between two inlined switches: typed copies at that
  boundary (`mwl_convFrmInfFromSFD`).
- Stack out-record fields live across later calls: typed locals right after
  producer (`sfmps_CopyDstBuft`).
- Loop test load reused in body, ours reloads: REQUIRE no call or store
  between test and use. TRY body read spelled exactly like test
  (`tbl[i].image_id`), not via `p = &tbl[i]`; MWCC CSE matches expression
  trees, not equal addresses (`minigame_puzzlefighter_setup`).
- Global loaded once, tested in both arms: one local before branch
  (`round_over`). Field read once before switch/call chain, not written
  between `[da]`: real local (`HVQM4DecSoundDecode` channel count).
- Owner fields reread after inlined helper that writes stack (fast sqrt):
  typed position pointer after helper, read through it
  (`mks_bgnd_cam_offset_away`). Field null-tested before cached: test
  directly, then assign (`p_create_decoy`). Allocation result tested before
  field store, ours stores then reloads `[da]`: `if ((p->field = Alloc(...))
  == NULL)` (`decv_init`).
- Reread after float-bits helper, scoped `opt_common_subs off` reproduces it
  but rotates volatile GPRs: TRY unit-private helper reading word via
  `*(unsigned int*)&value` (address-taken input). Its stack stores kill
  pointer-CSE of `obj->pos` with CSE on; direct `plyr_obj`/`his_obj` reads
  stay CSE'd; no pragma (`drone_ai_victim_avoid`).
- Callback + opaque object loaded before state mutation: snapshot both
  (`SFXLIB_Error`).
- Inputs across process transfer: keep every button read, reload pdata;
  down+right then down+left != one down test.
- Retail loads same word twice, ours CSEs `[da]`: REQUIRE each access type
  independently proven (signed `cmpw`, byte load, unsigned export). TRY that
  type at each use. Type chosen only to stop CSE = forcing
  (`_rwDlGetRenderState`).
- Guarded global reload `[da]`: `if (n > 0) n--;` as load, compare, reload,
  sub, no call/store between. No GC 1.0-2.7 flag, pragma, type, wrapper, or
  CFG reproduces it; only `volatile` does. `const`-qualified read (cast,
  const ptr local, const macro) also reproduces it = fake alias, reject.
- `volatile` only for proven interrupt/debugger/thread-shared state, on decl
  + def (never per access). Needs: minimal repro reuses load on every
  compiler/flag, every honest non-volatile form fails, real sharing, user
  ruling; cite in note `[da]`. Qualifier that reproduces a reload proves the
  reload, not the original qualifier. Escalation-accepted volatile: note
  "accepted after escalation, no honest form matched; revisit". Always reopen
  (mk-da `vdisp_init` later closed with honest reads + decl order).

## H06

Retained value or address. REQUIRE shared uses + unchanged ownership
interval. Reject redundant aliases, identity wrappers, permuter residue.

- Trailing buffered overlap computes byte difference before adding dest:
  REQUIRE bounds proving offset + same copy length. TRY `destination +
  (length - overlap)`; left-assoc pointer math can shift adjacent 64-bit temp
  allocation (`mwFileBuffer::readInRange`).
- Nested array member: ours adds board base before combining row/col/field
  offsets, retail combines offsets then one indexed load. REQUIRE same
  dimensions, coordinate narrowing, layout, single load. TRY typed
  pointer-to-member slot local and deref; else inline slot accessor returning
  that address, deref in caller (`mk_chess_spell_move_target_to_target`).
  Value-return accessor may keep original grouping. No flattening across
  subarray bounds, no invented byte offsets.
- Word transfer within an owner's array: retail keeps root + scaled index +
  member offset for `lwzx`/`stwx`, direct assignment shares a member base.
  REQUIRE canonical element type and unchanged index reads/transfer order.
  TRY typed source/destination slot pointers followed by the dereference copy
  (`_copy_register_to_register`); signed pointer types cannot substitute for
  unsigned register storage.
- Several stores to one instruction record: element pointer folds array-member
  offset into base, retail keeps owner + index stride and each store's field
  offset. REQUIRE a real count snapshot and later count reload. TRY canonical
  owner array-member stores through that index (`pfxvm_spawn_box`); no fake
  prefix view or byte-offset arithmetic.
- Async setup returns owner field after call, retail saves it before: REQUIRE
  traced store/load + callback boundary. TRY returning real pre-call snapshot
  (`add_art_section_async`). Existing-item and new-item paths returning same
  index kind -> one result local for both. No extra value just for coloring.
- Unrolled frame-to-buffer binding derives via saved subarray pointer, retail
  derives from owner: REQUIRE proven reserved prefix + contiguous per-frame
  entries. TRY genuine subarray base folded into each assign, keep pointer
  association (`&user->entries[1] + index` in `sfmpv_InitInf`). Drop unused
  alias; `&user->entries[index + 1]` can change unrolling. Check every store
  + whole unit.
- Equality guard compares stored object with supplied pointer: REQUIRE traced
  receiver reg into next call. TRY calling via stored owner expression when
  retail keeps that load, keep null predicates (`ScreenMgr::RemoveScreen`).
  Equal pointers != interchangeable source owners.
- Snapshots retail keeps: render-quad ptr before fade test (separate alpha
  reads); member-array ptr before range loop; search key across list cleanup
  with separate owner read for publication; real subobject ptr when one base
  reg serves several accesses (`SFTIM_GetTimeSub` keeps `SfdTimerState*`).
- Counted pointer tables: retail adds table base + scaled index, then uses
  field +4; source folds +4 into the index. REQUIRE the same slot load,
  relocation store and reread, with no intervening effect. TRY a typed slot
  pointer for loads/rereads and the owner's array member for publication;
  keep the header owner for count reloads (`PatchAnimEffects`). No overlapping
  header views or fake prefix structs. Loop-owned indices can be required
  with this boundary; measure the combination.
- Consecutive XYZ stores via one address: position pointer for those stores
  only; later component copies keep owner reloads (`setup_tombstones`,
  `RwV3d`).
- Field-address load-update alone != pointer-to-field local.
- Index kept across calls, table addresses rebuilt later = typed index, not
  cached entry ptr (`run_ending`, right-stick dispatch).
- Save/restore not a self-store if helper between writes field (air-move init
  restores animation step).
- Original + advanced snapshots only while both live.
- Callback accounting can need separate sample/previous/current snapshots
  before wrap fix (`ADXB_ExecHndl`).
- Keep every random draw; narrow at proven boundary, full width after.
- Strided fill, ops match, input base rotates below stride/count/range:
  REQUIRE cursor advancing to new record each iteration. TRY real working
  cursor from input base for record access + byte-stride advance
  (`_pfxvm_init_multiply_float_range_v3`); keep typed record view + draw
  count; no aliases, no flag change.
- Call-free inline predicate can create result lifetime; test its checks in
  consumer accumulator without moving lazy calls.
- Copy-then-transform keeps both (distance before squaring).
- Debug-format values preloaded before effectful call: typed snapshots;
  post-call read stays direct (`SFTST_Calc`).
- Missing island repeated in several callers = private inline helper
  (`__ARQPopTaskQueueHi`); verify no emitted symbol.
- Owners/table selection: typed pointer to actual table array replaces
  container local; keep both reloads when getter separates them. One pointer
  per independently selected owner. Repeated pure field checks read members
  directly. Each branch loads one field before Boolean return -> select typed
  scalar, not entry ptr.
- Scope/placement (+H14): init out-pointer at entry only if retail zeros it
  there; snapshots in owning scope at observed load point; table snapshot may
  follow category early return. Separate count math from later publication.
  Name squared distance before inlined sqrt only if type keeps float
  boundary. Inverse-length normalize can need typed inline float helper
  keeping bit estimate. Swapped temp FPRs on independent coordinate diffs:
  reverse source expression order. Widen narrowed random sample at proven
  helper boundary. Consume lookahead in place only after last decode use.
  Conversion-constant addressing differs, arithmetic same: keep real scan
  index through scale lookup, not const-pool pointer.
- Coefficient decode: sign threshold after unsigned amplitude extract; keep
  real `extended_base = packed * 2`.

## H07

Inline boundary. REQUIRE retail call boundary, signature, active inline
settings. Verify emitted calls, helper symbols, every consumer.

- `dont_inline` region also blocks expansion inside its function; moving
  pragma into body doesn't narrow it. Restore proven caller-before-callee
  order before forcing policy.
- Plain `static` can emit unused out-of-line body (`adxb_EntrySte` in
  `ADXB_EvokeDecode`); `static inline` doesn't. Live donor helper with no
  retail symbol -> `static inline` `[da]`.
- Donor inlines or table-dispatches, retail calls each routine directly (or
  reverse) `[da]`: REQUIRE retail `bl` targets + helper's own symbol. TRY
  dropping `inline` on helpers retail calls; replace table dispatch with
  direct branches on real selector bits (mk-da `MotionComp`). Donor calls a
  dispatch helper there -> `static inline` helper doing direct dispatch may be
  the original. Table/typedef unused after that, no retail symbol, no table
  data or `.rela.data` -> delete it.
- One-purpose inline helper for a fast path the shared helper can't make:
  only when shared form measured worse; record measurement `[da]`.
- Keep callee out of line while explicit helpers still expand: scoped
  `auto_inline off` at callee def (`LSC_EntryFileRange`). Static helper
  retail calls under `-inline auto`: object `-inline noauto` (dvdlow
  `SeekTwiceBeforeRead`, `__OSCacheInit`); recheck every function.
- Deleting nested `static inline` lowers enclosing cost; `-inline auto` may
  then expand it into callers (`plyr_weapon_trail_hide` 4x). Check callers'
  instruction counts.
- Moving a body: canonical decls must precede it (no implicit int). One fixed
  edge can expose another auto-inline; nested helpers -> final caller before
  callee when retail needs the call.
- No-setting control proves a pragma is needed.
- Donor/private helper may exist only as inlined island; extract when retail
  groups a whole loop + publications (`sfmps_UpdateStreamBounds`). Unrolled
  clear + folded status tail = status-returning helper (`MPS_Init`); stop if
  every honest form erases dead retail branches.
- Wrapper loads endpoint, compares with itself before shared inlined loop:
  REQUIRE same clamp in sibling wrappers + same loop. TRY endpoint through
  existing inline clamp before loop (`ani_to_end`, `animpdata_ani_to_end`).
- Inlined unlink compares updated head with itself + keeps null-result
  assign: REQUIRE canonical unlink's local-head store, returned node, real
  publication via previous link. TRY `node = ListRemove(&node)` on local head,
  keep asserts; never write self-compare (`ListNodeAlloc`).
- Inline limits: test `inline_max_size` + `inline_max_total_size` together in
  isolated diagnostic; proves expansion, not retail pragma values. Typed
  inline helper over macro when macro changes ref ownership or stack. TU
  limit = M13.
- Predicate/selection helpers (+H11):
  - Retail keeps classified values across lazy queries, reloads at admission,
    queue mutation, breakout: REQUIRE full policy decisions/effects, same
    input + publication order. TRY typed inlines taking those real values,
    owning full scan/compaction indices; early operation return can keep
    caller's later work (`pz_fighter_fight_request`). Verify expansion + every
    sibling; no isolated-load extraction, no control flag.
  - Repeated Boolean joins/selections: meaningful helper keeping
    consumer-specific loads, null guards, failure cleanup, shared publication
    (`ReadSram`).
  - Bounded scan, found + exhausted assignments join before one test: assign
    at both exits, e.g. unconditional `for` with found `break` and `++index >=
    8` `break` (`sfply_IsBpaOn`, no `goto`).
  - Return stopping index (limit on exhaustion), not Boolean + repeated guard.
    Boundary scan + neighbor updates in one helper.
  - Cleanup gated by returned Boolean -> move into the operation. Structured
    break-to-cleanup + early returns replace `goto`.
  - Sentinel validation with callback failures returns right after callbacks;
    success publication after loop.
  - Wrapper returning inline result can merge ptr/null paths; direct
    traversal in that wrapper.
  - Lookup defined after its consumer: shared inline body + public wrapper
    keeps public order. Retail expands allocation, raster tests, cleanup
    before public def (`image`) -> expose typed impl before consumer, verify
    both bodies. Keep explicit public body if wrapper changes codegen. Move
    proven `dont_inline` scope with its callee group; reset before wrappers
    retail keeps out of line.
  - Reuse existing lookup only if bounds, first-match order, fallback agree.
    Keep caller precondition; no added null guard.
  - Button dispatch: eligibility outside suppression predicate; one switch can
    recover one game-state read.
  - FP compare via `mfcr` or result held across calls: returned predicate or
    early-return decision.
  - Reset result joining later decisions: false-init result assigned helper's
    return.
  - Simple threshold via inlined typed Boolean helper (`sfx_IsEnoughWork(s32)`
    removed accidental 64-bit compare).
  - Extracting inline conversion helpers can zero a public wrapper; donor's
    scoped `dont_inline` restored both (`sfxzmv_MakeCnvZTbl`).
  - Recursive consumers: scoped `auto_inline off` + explicit inline helpers.
  - Local dispatch-table copy before validation: typed selection/validation
    helper, explicit returns; verify the copy.
  - Inventory ownership checks keep bit-read result across item-type lookup
    inside helper, original lazy bounds checks.
  - After extracting helper, drop Boolean normalization + caller snapshots it
    made redundant.
- Decoder phases: typed inline phase boundary with real inputs/outputs, incl
  cursor setup it owns; outer decode owner can span init to publication.
  Flattening nested phases can undo gains.
- Null gate before bounded wait keeps enter-branch + jump past wait: REQUIRE
  null skips only this operation, caller snapshots live across callbacks. TRY
  complete inline wait with null early return incl process selection +
  counter setup; passing only constant counter inits too late
  (`drone_set_difficulty_level`). Verify countdown + later owner reloads; no
  caller return or lifetime block.
- Object loop owner/index regs + derived zero differ around repeated callback
  phase: REQUIRE same callback + publication order. TRY typed inline owning
  whole indexed phase, direct canonical array access, not record-rebasing
  pointer local (`p_obj_ctrl`). Verify reloads after callbacks + consumers;
  drop padded offset view if canonical array matches.
- Repeated fill/clear with advancing cursor: pointer-to-pointer helper
  storing + advancing caller cursor over full extent.
- Escape-window shifts: typed inline value helper can fix scheduling (Dc11),
  context-dependent (regressed Nintra first-code phase).
- Donor table-construction macro can encode loop-lifetime boundaries; keep
  separate counters (`sfxzmv_MakeOrgZ32TblByCCIR`).
- `inline` helper can still emit a call under TU settings; canonical
  field-access macro can replace it locally (RW default atomic callback).
  Audit evaluation count.
- Retained object + returned effect handle swap saved regs: REQUIRE same
  effect selection, position query, publication in sibling consumers. TRY
  typed inline owning whole operation, passing real object snapshot
  (`flash_hit_at_bid_with_y`). Wrapper around snapshot load only != proven
  boundary.

## H08

Latch diamond. REQUIRE null-before-instance reads, no call between.

- Typed accessor returning validated ptr or null; pass owner if arg eval
  hoists reads. No empty valid arm.
- Joined latch result followed by a separate nullable return: REQUIRE retail
  tests the joined pointer and initializes the return register to zero. TRY
  a default-null result assigned the validated pointer only on success
  (`pfx_get_emitter_obj`); direct macro return or early returns can fold the
  outer join.
- Open-coded rejection merges null arms, swaps saved owner/object: REQUIRE
  canonical lazy ptr/instance macro + verified header-at-zero layout. TRY
  macro directly on owner fields with its header type, restore concrete
  result type (`p_mk_chess_apply_force`). Separately staged ptr can keep
  extra volatile copy.
- Retail skips null assignment on valid arm: REQUIRE separate null-owner arm +
  same ptr test. TRY `condition ? pointer : 0` (`destroy_sobj_ctrl_proc`).
- Reuse existing accessor before adding latch view; its success return keeps
  the branch an empty arm folds away (Konquest grounding). Shared owner-typed
  process accessor can replace padded latch view.
- Repeated object slots: reuse accessors, reload owner after each effectful
  call.
- Source merges null arms, retail separate: `return object;` on valid,
  `return 0;` in each failure arm (`reaction_xfer_him`, `get_mission_state`).
  Measure per owner.
- Retail folds member offsets into loads where source forms `&array[i]`:
  inline helper taking owner + index (`fighter_severed_limb_live_object`).
- Direct-owner accessors suit adjacent load/validate; cached forms suit real
  effects between. Extracting a latch can change caller inlining; check every
  caller.

## H09

Loops. REQUIRE zero-trip behavior + test/update order.

- Dynamic bound loaded once into CTR: snapshot bound, keep per-iteration
  pointer reloads. Fixed positive extent: test bounded `for`/`<` vs
  do/while/`!=`. Unsigned ascending index keeps `cmplwi`/`ble` + CTR.
- Fixed reverse array clear, only zero/index setup order differs: REQUIRE same
  descending order + unused final iterator. TRY `for (i = count; i-- != 0;)
  array[i] = 0` (defined final wrap) (`ScreenObject::ClearActiveObjects`).
- Rotated top test may need top guard. Keep polling, sleep, countdown order
  incl final input recheck.
- Frame bound at entry + exact equality after sleep: REQUIRE both edges +
  zero-trip. TRY pretested loop with equality `break` after sleep
  (`ani_to_frame_x_call`).
- First edge into counted search: branch to initial compare = pretested loop
  that can skip all.
- Table scan to first matching ID: bounded `for` + found `break`.
- Hoisted invariant load in block after final return (`b tail` ... `tail:
  lwz; b header`): REQUIRE otherwise matching `for(;;)` block order. TRY
  pretested loop whose condition is constant only after optimization (`while
  (!found)`, flag zeroed at entry). Front end rotates it; backend LICM appends
  preheader at function end. `for(;;)`, `while (1)`, `(1 == 1)` fold too early
  (`drone_ai_fetch_next_AIState`).
- Delimiter scan: typed inline scanner returning right after count store
  (`sfmps_DecodeOneUnit`, no `goto`).
- Fixed ramp: paired CTR loop from single-entry `for` when donor has one
  (`CFT_MakeArgb8888AlpLumiTbl`); not for clamped interleaved ramps. Pair of
  updates per iteration can be fixed pair-count loop.
- Owner walk: separate count, pointer, index locals; `owner++` at body end
  when retail advances it before index (`MPS_Finish`).
- Retail `mulli`/`add` per iteration: `entry = &table[i]` in loop, not
  advancing pointer (`LSC_ExecServer`).
- Two strength-reduced pointer walks can need decl order != assignment order
  (`sfxzmv_MakeCnvZTbl`).
- Walking pointer, buffer, other input colored wrong order (retail: walker
  lowest callee-saved) `[da]`: REQUIRE retail also keeps index (bound, stored
  or passed position). TRY `buffer[index]`, no cursor local, before any
  coloring search. MWCC strength-reduces index into pointer created after
  params; explicit cursor local is created before them, swaps webs (`mflGetS`:
  3 tries after 60 manual + 185k permuter on cursor shape). Fixed-width char
  padding variant: H21.
- Prologue scheduling differs, body exact `[da]`: bound/end pointer written in
  byte math retail doesn't need. TRY loop's element units (`end = out +
  (width >> 1)`, not `(u32*)(row + (width << 1 & ~3))`) (`vdisp_copy_frame`).
- No dummy one-trip loop.

## H10

Switch. REQUIRE full finite case/default/fallthrough set + text order.

- m2c can omit labels sharing an arm even >99%. Verify finite bounds before
  accepting decompiled default.
- Compare-tree split one past adjacent error-code cluster: REQUIRE callee/API
  defs + identical destinations. TRY explicit shared cases, not rewritten
  tree (`mem_card_read`: CARD_RESULT_NOPERM, CARD_RESULT_LIMIT).
- Recover case order, shared tails, nested switches, where each default is
  emitted.
- Small ordered hardware-ID sequence can be `if`/`else if` chain
  (`InitMetroTRKCommTable`); two adjacent priority cases can need real
  `switch` (`ARQPostRequest`).
- Per-arm address formation: keep real index; table base formed before
  dispatch = one owner.
- Explicitly compared no-op cases: proven `case N: break;`.
- Idle states in dense table: one case per entry; one state local across
  pre-server check (`sfply_ExecOne`).
- Compare tree ending in unreachable extra `b join`: one empty label just past
  tested range sharing other empty cases' `break` (`go_into_twitch_death`,
  `go_into_major_pain`).
- Same dead `b default`, plus loaded selector in r4/r6/r7 where ours uses r0:
  REQUIRE enum switch, sentinel shares `default`. TRY enum `*_FORCE_32BIT =
  0x7FFFFFFF` label before `default`; constant too big for `cmpwi` keeps
  selector off r0. No invented small label (`case 6`). Check sibling ports
  (PS2 MW MIPS compares 0x7FFFFFFF) (`mwMemHeapGetMaxFreeBlock`,
  `privInitSystemHeap`, `mwMemHeapStrategyCallback`).
- Missing-ID guard + nullable attachment arm match except unreachable
  shared-exit branch: REQUIRE real zero sentinel, kept lookup/null order. TRY
  two-arm `switch`, attachment in `default`, direct `case 0` failure return
  (`replace_sobj_texture_with_named_wiff`); no extra cases, no dead return.
- Jump-table layout: edit raising `.text` but lowering `.data` moved case
  addresses; reject (`mk_chess_set_game_mode`).
- Float const loaded once at join: one return after chain, early returns
  inside cases (`p_chomper_controller`). Two cases sharing a call, one only
  guards: `if (!cond) break;` into `default` (`p_game_loop`).
- Ordered classifier: staged shift/or key, each arm assigns one result,
  shared return (`MPV_CheckDelim`).
- Dense ID classifier has distinct equal-valued case/default exits: REQUIRE
  full finite ID range and retail interval tree. TRY explicit grouped cases
  for both results plus separate default, rather than hand-written range
  tests (`is_this_a_hault_message`). Shared consumers keep one selector
  snapshot across the classifier and later ID tests.
- Small state constants (2/3, 4/5) stored to a field, diff = staging of
  those constants `[da]`: REQUIRE retail writes only those values. TRY local
  `enum` of states + explicit `if`/`else` assigning them, not ternary, `s32`
  local, or default-zero (`mslStreamStart`).
- No speculative labels.

## H11

Joins. REQUIRE real branch destinations + effect ownership.

- Identify target instruction: branch to final store != branch past it.
  Trace failure branches before calling it scheduling (Krypt coffin effects:
  one failure skips all later objects).
- Nullable owner, result init before guard: null-init result, assigned from
  validated accessor inside guard, one return.
- Two-way owner descriptor is retained before earlier stores in source but
  after selection in retail: REQUIRE identical choices and effect order.
  TRY explicit `if`/`else` assignments at selection instead of default init
  plus overwrite (`p_ground_target_collide`); no redundant result alias.
- First verify retail tests owner at all; drop unsupported fallback.
- Extra early-failure assign: positive gate sharing final return. Real state
  still lowers different -> one-case switch with existing default.
- Final int Booleans: `!= 0` vs `== 1` differ even for 0/1 values. Test arm
  order separately; two-value select uses real conditional, zero-first vs
  nonzero-first picks different carry code.
- Sequential predicates share one retail epilogue: real result local owns
  whole chain incl first success arm. Result local only after early return
  tests different join (`should_i_weapon_block`).
- One tested state word live through several bit tests, then 1/0 in
  separate arms: REQUIRE same input + branches. TRY block-local state sample
  + explicit result arms, not compact `&&` (`is_he_blocking_throw`).
- Null-failure block before independent gates: positive ptr guard with
  else-return, then gates.
- `||` merge of null/invalid only when retail shares return; keep null test
  before lazy reads. Nested exclusion folds to one skip branch, retail has
  conditional to body + unconditional shared return: TRY combined
  early-return guard (`trial_load_monk`). Bounds/state failures with
  different exits stay separate. Separate success returns inside action arms
  when retail does. Never omit a return.
- Pointer-membership Booleans: sequential tests, explicit returns
  (`__DVDLowTestAlarm`).
- Inlined validators: ordered direct returns, not result + `else if`
  (`ADX_DecodeInfoExIdly`). Stored owner, local invalid result, expanded
  error callback = three open-coded stages (`MPS_Destroy`); one typed local
  for shared work ptr (`MPS_SetErrFn`, `MPSLIB_SetErr`).
- Sentinel directory search: success branches past the failure index reset,
  while break + post-loop null test duplicates the sentinel check. REQUIRE
  same entry stride, identity compare and zero fallback. TRY a real typed
  inline index search returning success early and zero at sentinel, followed
  by the caller's language-offset lookup (`offset_mk_file_info`). An existing
  public wrapper may emit a call; keep already exact consumers unchanged.
- Negation scope: `!(A && B && C) && D` != `!((A && B && C) && D)`.
- Temp board simulation: keep every restore + final rebuild, incl zero-trip.
- Genuine cleanup edges: M08.
- Failure clears retained owner then branches to shared final return
  (`get_mkobj_frame`): REQUIRE that exact destination + measured structured
  alternatives. AGENTS.md forward `goto` only if those regress and shared exit
  avoids duplicated init or fake flag. Retail jumps from inside wait loop to
  shared end-of-iteration block `[da]`: three failed structured forms, then
  one local `goto` (mk-da `gop_decode`). Record measurements in task report.

## H12

POD copies: aggregate assign for word/CTR copies, components for
`lfs`/`stfs`. Three interleaved word copies in a particle stream can be one
`Vec` assign. Keep API byte strides + copy order; then test decl order. Event
buffer extent from consumer copy + producer stores. No invented init.
Copy reorders loads vs stores, each loaded word used `[da]`: name every
loaded word in a local so all loads precede stores (`IntraAotBlock`
`temp0..temp3`).

## H13

Intrusive lists: exact typed reciprocal-store order; save next before
mutating callback; reload links as retail. Repeated stale-node removal
differing only in saved-successor allocation: typed inline helper returning
successor, keep save-next, clear-header, destroy order.

## H14

Stack slots + store order. REQUIRE real address-taken locals, offsets,
lifetimes.

- -O4: address-taken locals slotted in reverse decl order. Reverse decls when
  retail runs other way; size real buffer from frame gap once max write fits
  (`p_setup_konquest_map`).
- Decl order and block scope interact; measure each + combo with init fixed.
  Block scope = existing `if`/loop/`else` body; bare `{ }` only to scope a temp
  = force matching (`validate_save_location` dummy slot local).
- MWCC keeps stack-member stores in source order. Same instr count, different
  store interleave, different `stmw` set = store-order evidence: write members
  in retail `stw` offset order (`mwMemAllocateFixedBlockHeaps`,
  `mwMemHeapInit`).
- Split decl from init only without const/aggregate/scope/lifetime change;
  reordered initialized decls reorder stores. Branch-local scalar with wrong
  reg can move to function decl group, load stays in branch.
- Init after lock/lookup when accumulator has no earlier use
  (`SFMPVF_GetNumFrm`). Process-creation output zero init stays in creation
  guard.
- Recursive result accumulation keeps unused maxima live across the recursive
  call and shifts every slot: REQUIRE maxima have no earlier reads. TRY
  initializing them immediately before consuming the returned list
  (`mk_chess_calc_all_attackers_in_turn`); preserve recursive inputs and list
  traversal order.
- Exclusive paths with per-path slots: distinct typed locals (`TRKDoContinue`,
  `TRKDoStep`); per-branch `Vec` declared in each block. Final switch joining
  one epilogue: result init before switch.
- Donor inline result outliving another arg gets own local before call. Keep
  separate loop counters. Later loop index swaps with its strength-reduced
  offset IV: own counter variable for that loop
  (`minigame_puzzlefighter_setup` wiff loop).
- Later address-taken vectors use earlier caller slots, retail uses the final
  slots: REQUIRE an isolated operation owning those vectors. TRY typed inline
  owning the complete operation (`pz_fighters_chomper_fatality_in_progress`
  flesh launch). Compare initializer payloads and pool positions separately;
  correct stack slots do not prove TU data layout.
- Factory output pointer and inlined scalar bit-view slots exchanged: REQUIRE
  exact factory guard, post-call owner reloads and scalar math. TRY typed inline
  owning the complete factory/setup operation (`npc_update_pos_on_path`
  waypoint-script launch), so both address-taken locals belong to inlines.
- CRI frame-plane order: store strides + base plane before aligning height
  (`SFD_CalcYccPlane`); size locals at first use, plane-array ptr after
  picture-type branch (`sfmpv_SetFrmPara`); frame-buffer ptr declared before
  frame size (`sfmpv_ChkBufSiz`). Timecode converter's two scale stores both
  real (`sftim_Tc2Time29N`).
- Formatted-text buffers: size = callee max output incl terminator.
- Tell compiler-made by-value copies apart. No padding locals.

## H15

Coloring, identical ops. Association + staging first; then max one honest
lifetime/decl insight; then stop (tier 4). No register carousel, no invented
uses. Checklist `[da]`: grouping -> one decl/scope change -> H26 / real
cached local -> tier 4. Copy of in-scope value, reuse of unrelated variable,
branch-only alias = dishonest even at 100.

- Decl order + scope `[da]` (mk-da's top rule): lifetimes look shifted ->
  one honest move of where a real local is declared (outer vs block, before vs
  after another local's first use). GC/1.2.5 colors in decl order.
- Two independent init loops share one index, only 2nd loop rotates
  constants/address regs: REQUIRE separate arrays, no shared index value. TRY
  C++ loop-local index for 2nd loop (`ScreenObject::ScreenObject`); keep
  iteration/store order; no bare blocks or per-store wrappers.
- Compound reassociation `[da]`: MWCC rewrites `x += a + b` as `(a + x) + b`.
  One add/or/xor commuted -> fold constant term into previous statement or
  split `x += a; x += b;` so running value stays left operand
  (`HVQM4BufaCreate`: `= product + 64; += nodes; += 32;`). Random then fixed
  increment: two sequential adds. Operand order matters apart from staging;
  not for FP. `channel_count * (sample_count * 2)` != `sample_count *
  (channel_count << 1)` (`ADXB_ExecOneAiff16`). Hoisted invariant must
  stay left of loop index: `tbl[(bg << 1) + i]`; `bg * 2 + i` commutes
  (`minigame_puzzlefighter_setup`). Ceiling div: name count, add,
  decrement, divide.
- Name real base quantity `[da]`: size math right ops, wrong staging. TRY
  `u32 pixels = width * height;` then derive scaled sizes. Never copy of
  existing local (`decv_init`).
- `A() + B()` evaluates B first; staging local keeps old order
  (`ADXSTM_Create`). `f(g(1), g(2))` calls `g(2)` first; staged local for 2nd
  string arg = residue (`_trial_add_required_sequence`).
- Select spelling: `x = p ? 0 : C;` vs `x = C; if (p) x = 0;` same instrs,
  different allocation (`gc_aram_mwmem_heap_setup`).
- Scaled arg: `MEMPRINT(fmt, size_kb *= 1.0f / 1024.0f, name)`
  (`mwMemUserConfigOutofMemoryCallback`); siblings may differ.
- FP temps: numbered per statement (pooled const first), colored newest-first
  into lowest free volatile FPR. Stage negation + copy into result locals in
  earlier statements (`ai_side_clearances`). Pool label numbers != pool order.
- Narrow params: explicit cast or u16 local hoists arg load; implicit
  conversion to `unsigned short` param doesn't (`drone_loop`). Needs callee
  narrow-param evidence; u16 return makes callers re-normalize.
- `!value` vs `value == 0` in an arg can change allocation; not for branch
  conditions.
- Decls: swap two real scalar coordinate decls; slot pointer beside its index.
- Byte copy coalesces with int producer, not dead owner: REQUIRE same scope,
  ops, types. TRY int producer declared before intervening owners, byte
  consumer after (`ScreenWaitAnimAction::Update`); confirm full decl order in
  full TU, not reduced search.
- Signed-short coords with retail `extsh` beside full dims: `int` locals
  init `(short)` at coordinate boundary. Normalized values + FIFO write widths
  unchanged; `short` locals use other allocation class (`feedback_effect`).
- Volatile FPRs around inlined helper: helper temps take lowest FPRs; caller
  floats live across it take next in decl order (earliest lowest). Own local
  per value (position, delta), name square products, dot product in retail
  operand order. Coupled levers: each alone neutral/reversed -> search jointly
  on host (`mks_get_victim_to_tr_dot`: normalized components first, z before
  x).
- Drop redundant aliases: read mirrored field direct
  (`SFCON_UpdateConcatTime`); write via single-use global owner at final store
  (`GXSetZTexture`); pass sole-use member to existing helper
  (`obj_find_material_by_id`); owner selection at lazy validation boundary;
  pass original member to validator.
- Pool-slot free-list pop: selected node = result, update canonical slot head
  directly. REQUIRE unchanged pool/index before store
  (`hashtable_store_with_instance`); no recompute local.
- Staging: float getter sample before accumulate; memory read before both
  Boolean branches stays unconditional; capture counter post-increment when
  retail compares old value; stage dividend in remainder local.
- Inlined fixed-array search only swaps index/pointer: restore typed private
  search helper (`sjmem_SearchFreeObj`, `lsc_SearchFreeObj`).
- Typed subrecord owner for unrolled stores (`MPVCMC_InitObj`).
- Dead input params evolve into out-count/return accumulators or decode
  cursors when retail keeps their regs (`SJRBF_IsGetChunk`,
  `MPSDEC_DecHdMpeg1`), only after original last use.
- WAV deinterleave: donor mutable u16 view + its byte-swap macro when each
  arg is side-effect-free load (`ADXB_ExecOneWav16`).
- Field value colors wrong, one-shot `T* x = s->field;` before test gives
  wrong web `[da]`: REQUIRE retail reads field in test and in branch. TRY test
  field directly in condition, bind local inside branch; MWCC CSE-merges both
  loads into one web (`mslSoundStop`).
- Equality vs constant, esp recursive code MWCC inlines `[da]`: swap operands
  (`1 == depth`); changes value numbering in inlined copies
  (`AllocateToLeaf`).

## H16

Nest single-use result; keep original callback owner at untyped boundary.
`g = f(); use(g);` forwards without reload even under `-opt nocse`
(`gc_aram_init`). No manufactured return contract.

- Retail publishes owner before validating two nullable refs, only
  owner/result regs differ: REQUIRE same store + lazy instance read order. TRY
  typed publication helper fed by actual owner assignment expression; drop
  outer owner alias (`pw_plyr_force`).
- Returned pointer moved via temp before setting consumer arg: REQUIRE staging
  belongs to observed branch, keeps producer/consumer order. TRY named typed
  result in that branch; direct forwarding where retail has it
  (`render_mkatomic`).
- Script wrapper moves saved string result via extra reg: REQUIRE retail
  arg-fetch order. TRY direct producer expressions in both args; verify order
  kept (`_konquest_start_nis_anims_load`).
- Scalar script args after string resolver: REQUIRE whether retail keeps
  original arg frame or reloads `current_args`. Keep frame for
  `_pfx_spawn_at_bid`, reload for `_bgnd_set_fx_z_offset`.
- 0/1 from a call shifted via different reg (`cntlzw`; `srwi` into r0 then
  `mr` vs straight to saved reg) `[da]`: other spelling; `!f()` and `f() == 0`
  materialize differently (`RwStreamClose`). See H19.

## H17

`-opt off` TUs (RW `rw/`; flags in `configure.py`). No DCE, so retail store
to unread local = source evidence: restore vendor statement (`ptr = 0;` after
free, declared-first `size = 0;`, assert-only `heap =
RxHeapGetGlobalHeap();`). Named intermediates make reg homes; nested ternary
into one local can be required (`RtQuatConvertFromMatrix`). Nonvolatile homes
rank by ref count; equal counts -> earlier-declared gets higher reg. Decl swap
neutral -> counts differ: scratch-only probe (`(void)x;` or empty `if (x) {}`,
no code) flipping colors shows retail has one more ref. Never land probe.
Boolean lowering: M01 size bit.

## H18

Fixed addresses: MWCC absolute-address vars (`extern unsigned long
__OSBusClock : 0x800000F8;`, `extern volatile unsigned long __PIRegs[] :
0xCC003000;`), not pointer-cast macros. Volatile deref macro pins loads vs
neighbor stores; plain deref macro lets MWCC CSE reloads retail keeps. Match
SDK signedness (`long` apploader offset -> `cmpwi`). Clock as absolute var ->
spell SDK tick macros per use.

## H19

Retail materializes inlined test as 0/1 (`li r0,1; b; li r0,0; cmpwi`): helper
`if (cond) { return 1; } return 0;` or macro `(cond) == 0 ? 0 : 1`. `return
cond != 0` folds away. Drop extra `? 1 : 0` at call sites once macro has it
(`ENTRY_IS_DIRECTORY` in `DVDOpen`). Early-success search: whole scan in
predicate, return 0 only after exhaustion; caller-owned flag keeps extra live
reg (`mk_chess_drone_fetch_non_king_vulnerable_matchup`).

Retail normalizes existing predicate return with `subic`/`subfe.` before
guard: REQUIRE canonical predicate body, same call order. TRY `== 0` instead
of `!`; MWCC can keep inline Boolean boundary
(`pselect_update_profile_settings`).

Inverse: retail branches directly from lazy call tests into the consumer,
but an inline predicate adds `li 1/0; cmpwi`. REQUIRE identical call order
and effects; TRY expanding the short-circuit condition only at that consumer,
keeping the helper for other callers (`mcard_msg_crc_failure_rtn`).

## H20

MWCC colors scalar-replaced struct members after declared scalars. Retail
keeps inlined bit-reader/cursor state in low regs: rewrite state as declared
scalar locals driven by macros (CRI `BS_*`), order decls, load init values
straight in (`mps_dec`).

## H21

Helper locals and web kind.

- Fixed-width char padding differs only in cursor/const regs: REQUIRE kept
  length/index already bounding same stores. TRY direct array index, drop
  redundant advancing pointer (`pne_set_players_name_to_default`). Not all
  char walks; keep termination behavior.
- Bounded char-normalize pass swaps cursor + replacement const after separate
  copy: REQUIRE same fixed extent + conditional stores. TRY inline helper
  owning whole pass (`does_name_already_exist`). Keep processing past embedded
  NUL; no string-terminated walk or scalar getter.
- Array-reset loop rotates base/index/element regs: REQUIRE complete reset
  phase repeated by other callers. TRY typed inline owning zeroing + sentinel
  init incl index (`bleed_init`). Naming only element/base may not reproduce
  helper-local allocation. Keep every store; check consumers when sharing.
- Guarded player/start owners swap volatile regs: REQUIRE same canonical
  background fields, angle-copy slots, player reload after position store. TRY
  real per-player round-start helper owning angle `Vec` + background
  snapshot; null guard stays in caller (`move_plyrs_to_round_start`). Audit
  each player selector.
- Validated indexed accessor swaps runtime/index regs: REQUIRE same null
  guard, signed bounds, indexed return. TRY typed inline accessor, named
  runtime owner, single-use index passed directly (`fx_restart_emit`). Both as
  expressions folds runtime offset into later loads; both named restores
  wrong pair.
- Operation's retained owner loads via temp before moving to saved reg:
  REQUIRE operation snapshots real owner handle once, keeps it across
  callbacks. TRY owner's address to typed inline owning whole operation,
  read-only handle param, one initial deref (`aniproc_land`). Later global
  reloads stay outside at retail boundaries; no getter-only extraction, no
  handle reread in loop. Compare every consumer.
- Inlined search swaps bound snapshot + tag temp: REQUIRE unsigned bound
  invariant, no calls/stores in search. TRY reading owner's bound directly in
  indexed loop, no named count (`cloth_bones_init_by_tbl`). Changes bound's
  web kind, keeps CTR loop; check shared helper consumers.
- Inlined helper loop's fresh locals color above loop temps, retail reuses
  dead outer regs: open-code + reuse existing locals; MWCC colors reused
  local's 2nd web after loop temps (`AddRequestingCS_ByThread`).
- Several saved homes rotated at once (param, flag, call result, hoisted
  array bases) plus volatile residue in every region: REQUIRE CFG/ops exact,
  capture shows the big webs pushed in different sweeps. TRY changing web
  kinds, not decl order: `?:` for a two-constant select, `== ? 1 : 0` for a
  stored compare, colour helper on the field expression with a breaker macro
  (coalesced copies), open-coded two-def select, direct array indexing, a
  pointer local with a 2nd web, loop variable ownership per first web
  (`pz_ai_decide_match`). Numbering model + threshold: tier 4 Hard stops.
- Generation-validation latch swaps owner/object regs: REQUIRE read-only
  canonical latch members, no call/store between. TRY existing latch macro on
  direct owner-member expressions, not staged aliases
  (`save_konq_common_data_to_buffer`). Verify one owner/object load, null
  guard before instance read, snapshot placement across earlier calls, shared
  consumers. No getter, no mode change to keep aliases.
- Inlined table accessors miscolor scaled index + table owners: check if
  unrelated branch defs share one caller local. TRY named runtime-table base,
  typed indexing, each def owner scoped to its branch. Fold single-use owner
  into call arg; no unused assign inside arg (`drone_ai_check_attack`; its
  param swap = hard stop, tier 4).
- Under consumer-scoped `opt_propagation off`, residue = volatile coloring in
  inlined helper (same loads, owner/partial-sum regs shifted one, retail
  `plyr_pdata` r5/count r4 vs ours r4/r3): REQUIRE residue gone with pragma
  removed while another region regresses (whole-TU control). TRY copy-free
  shared helper: ternary body, not `if (p) return x; return 0;`, and direct
  member exprs as args, not staged `style` local. Propagation off keeps each
  expansion's param/return copies; propagation-on users unaffected
  (`ai_weapon_style_move_count` in
  `drone_ai_check_dont_touch_attack_phase2`). Both edits needed. Same rewrite
  regressed `drone_ai_check_attack` (param pair, not helper copies).
- Volatile GPR rotation, same ops/CFG: REQUIRE classify each miscolored web by
  kind before any decl sweep. MWCC colors highest-numbered web first (lowest
  free volatile); number follows kind (measured, `drone_ai_victim_avoid`):
  codegen temp (union load in inlined sqrt) < address-valued local (`drone =
  cond ? &g_A : &g_B`, immune to decl order) < inlined helper named local <
  caller named locals (reverse decl order; block-scoped last) and CSE'd global
  reads. TRY changing kind, not position: reading helper input word twice via
  `*(unsigned int*)&value` makes bit-word temp a CSE web coloring after
  named/CSE'd objects; direct `plyr_obj`/`his_obj` reads make owners CSE webs.
  Union helper + `opt_common_subs off` 99.44; pointer-cast + named `bits`
  99.72; CSE'd bits + cached owners 99.69; CSE'd bits + direct globals 100.
- Moving inline helper boundaries around code already colored right is
  byte-identical `[da]`; only helpers in the block where miscolored values are
  created move a web.
- Repeated atan-to-degrees expression swaps product and pi temporary FPRs:
  REQUIRE identical atan ABI and separate multiply/divide rounding. TRY typed
  inline owning the plane-heading conversion, then subtract current heading
  in the caller (`bl_process_beetle_climb_a_wall`). Caller angle locals may
  retain the swap; do not replace division with a precomputed scale.
- Nav-hint sign-bit temporary swaps with owner pointer: REQUIRE unsigned
  sign-mask expression and no changed owner reloads. TRY removing the helper's
  named unsigned alias, computing directly from its parameter
  (`npc_update_pos_on_path`, also improves `npc_travel_path`).
- Indexed profile comparison rotates slot/live/stored pointer regs: REQUIRE
  same slot lookup, fixed-width pin/name walks and checksum comparison. TRY
  typed inline owning the indexed slot lookup and complete comparison, with
  device owner and slot index passed directly (`validate_konq_load_location`).
  Keep the device owner in the caller if later retry logic reads it; no bare
  lifetime block or assignment inside a call argument.
- Stale list-link next pointer alone miscolored across destruction: REQUIRE
  save-next, clear-header, destroy, advance order and disjoint palette lifetime.
  TRY typed inline traversal helper saving and returning next after destruction
  (`hide_tile_objects`). This changes the saved pointer's helper-local web.
  Measure each consumer; the same return-next boundary regresses `unhide_tile`.

- Repeated record-rebasing phase swaps index/base regs: REQUIRE same record
  count, serialized-offset fields, global reloads and field-store order. TRY
  typed inline owning the complete loop and its index (`p_setup_krypt`);
  preserve each owner reload after a pointer-field store. Measure all users.
- Late aggregate initializer is trapped in an artificial bare scope: REQUIRE
  the surrounding calculation is a real operation repeated elsewhere and its
  initialization point is observed in retail. TRY inline owning that complete
  calculation and aggregate (`pz_fighters_objects_falling_fatality_prep`),
  with typed outputs and coordinate derivation in place (H26). Do not extract
  a getter solely to create a lifetime or move initialization across calls.

## H22

One-row operand/constant/copy residue. One at a time:

- `x = REG; x &= ~m;` as two statements when retail loads into the variable.
- Const on left (`0x80000000 <= p`) or swapped `==` for `cmplw` order. MWCC
  1.2.5 colors local written second into r3; make one side direct expr, not
  local (`DVDLowRead`).
- Float `!= 0.0f`, only reversed `fcmpu` operands, swap neutral: REQUIRE same
  lazy predicate + zero/NaN behavior. TRY scalar truth test
  (`pfx_emitter_exhausted`): both zeros false, NaN true. No relational or
  arithmetic zero identity. Same fix when `fcmpu` matches but constant and
  field load into swapped FPRs (retail constant in f0) (`p_pz_mode_fill`).
- Equality swap neutral, retail compares masked member vs requested ID: name
  real loaded ID first. Keep producer's unsigned type, check inline consumers
  (`fade_material`, shared `find_geometry_material_by_id`).
- `a = b = 0` when one zero reg feeds both stores (rightmost first).
- `x += y + c` instead of temp sum.
- Only `fmuls` operand order differs, swap neutral: REQUIRE same scale +
  consumers. TRY stage real scaled local then compound multiply
  (`emit_in_range`: `half_width = width; half_width *= 0.5f`).
- Base + const field + dynamic offset (`addi` then `stwx`), ours folds field
  into store `[da]`: `(base + 0xC) + offset`, not `base + (offset + 0xC)`.
  Never hide offset in pointer representation (`_rwResourcesClose`).
- Retail compares before loading default const; `x = default; if (...) x =
  other;` hoists const load `[da]`: `x = cond ? other : default;`
  (`RwStreamWriteReal`).
- `(unsigned int)` casts on `%x` pointer args make retail's copies.
- Inlined `{ call(); return 1; }` for unread `li rN,1` after call.
- Stored-but-unread local from param (debug assert value).
- Pointer local declared before its sibling in inner loop.

SDK code: check other matched decomps on same compiler (bfbb, prime,
pikmin2) for local-vs-expression split.

## H23

Symbol lives in this object per `splits.txt`/`symbols.txt` but source only
`extern`s it: define in unit, retail constness (`.data` = non-const) + data
order, values from split `.obj` blocks. Probe with zero inits first. MWCC
folds bases for same-unit objects (mixer tables in `__MIXSetPan`,
`MIXInitChannel`). Codec/lookup tables `[da]`: import only ELF OBJECT ranges
(size, address, reloc targets), check byte-for-byte vs DOL; never regenerate
from formula or pull unrelated upstream tables.

## H24

MWCC 1.2.5 reserves one stack slot per inline expansion when helper returns
via single-exit local (`T v; if ... else ...; return v;`), sized by return
type. Early-return + macro spellings reserve none. Pick spelling giving
retail `stwu` immediate; scratch-compile first (`__MIXGetVolume` as `u32` in
`MIXInitChannel`: 43 x 4 = 0xB0).

## H25

Thin inline latch wrapper evaluates arg once before body; macro rereads at
each use. Arg is global pointer or read before null test: copy to typed local,
then macro (`player = plyr_pdata; MK_LIVE(player->p, player->p_instance)`).
Keep helper when arg is latch address reused in member writes, or retail
reuses another var's zero on failure path.

## H26

In-place derivation `[da]`. IF retail derives value in the register it was
loaded into, REQUIRE one meaning for the variable, TRY computing it in steps
in one local (`margin = width; margin = (640 - margin) & ~1;`). mk-da: `bytes
= samples * 2; bytes *= track; code += bytes;` keeps shift/mul operand order;
capacity becomes byte count then `&= ~127`; next-page ptr set from allocation
then advanced. Never a second meaning or a copy of an in-scope value.

## H27

Statement order follows retail schedule `[da]`. IF loads, stores, or pointer
advances ordered differently, REQUIRE unchanged semantics (no alias between
moved accesses), TRY statements in retail order: compute packed words before
storing, advance cursors after stores, load node output ptr before size store
(mk-da `MCBlockDecDCNest`, `gop_decode`). Stack-member stores: H14.

- Copy getter/setter, retail loads run ahead of stores (L0 L1 S0 L2 S1),
  ours alternate or swap first two loads: REQUIRE source pointer only read.
  TRY `const T*` source param (header + def), plain member copies; MWCC hoists
  const-source loads above dest stores. Staged locals, pragmas, compiler
  revisions don't reproduce it (`mwMemHeapGetInfo`, `mwMemHeapGetParams`,
  `mwMemSystemSetParams`; mk-da `mwMemSystemSetParams` same const form).

## Traps

- ELF `NOBITS` = zero storage; never read `sh_offset` as initializer before
  "fixing" data-value mismatch.
- Auto extraction must parse C identifiers (`0.0f * body` is not a pointer
  decl); reject malformed generated source.
- No host (PC) branches in Deception source; portable fixes need retail +
  behavior evidence.
- Renaming locals never changes MWCC coloring `[da]`. "Neutral across decl
  orders" is not a name effect.
