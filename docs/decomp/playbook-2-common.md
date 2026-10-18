# Playbook tier 2: common (H rules)

Detail for H rules indexed in [tier 1](playbook-1-core.md). Read only the
section triage picked. Every bullet needs its rule's evidence first. Symbols =
exemplars that reached exact. `[da]` = mk-da import (tier 1).

## H01

ABI: wrong arg/return regs. REQUIRE all callers + callee ABI.

- Generic callback parameters occupy swapped saved registers: REQUIRE the
  canonical callback typedef and its object-return contract. TRY the real
  generic object parameter, a typed local for object operations, and returning
  the original generic object (`MKSpecSkinRenderCallback`). Verify all registered
  callbacks and native saved-register claims. A surviving named/parameter move
  can disappear because both receive the same physical register; that alone is
  not evidence of compiler coalescence. Do not invent an alias without an API
  boundary that requires the conversion.
- Callback return/liveness residue: REQUIRE retail function-pointer mangling
  and actual dispatch return use. TRY correct void return and full callback
  arity in the owner typedef, declaration, and definition together
  (`mwfile_error_callback`); no fabricated return value.
- Saved scalar param rotates around a call, word ops identical: REQUIRE
  caller type, callee param, stored member agree. TRY canonical signedness in
  public decl + def; implicit unsigned->signed makes another web
  (`pfxvm_spawn_line_1i`). Check every header consumer; match alone does not
  justify a type change.
- Conflicting narrow/full-width region declarations: REQUIRE every caller's
  actual truncation and the callee's complete use of that value. TRY one
  canonical header prototype and matching definition; keep implicit conversion
  staging when retail uses it (`save_konquest_region_to_memcard_w_error`). A
  full-width formal plus an explicit cast can emit the same mask in a different
  argument order. Recheck all callers, callee and header includers together.
- Pointer-looking r3 can be caller-made copy of a by-value aggregate. Verify
  caller copy before adding snapshots (inventory image: texture pair by
  value). Keep `cmpw`-proven signed loop count.
- Flag init into original word + separate passed word = aggregate-address ABI
  of one/two-word POD. Confirm all callers, callee reads, pinned-compiler
  value-vs-pointer probe, then fix canonical prototype. Typed initializer /
  constructor helpers recover each expansion's original/copy homes
  (`get_mkproc_nostack`, `bleed_startup`). Never model compiler copies as
  unused 2nd array element. Extra returned-record copies: inspect the real
  consumer's by-value ABI before tuning slots; a direct caller record plus
  outgoing argument copy can replace the return-constructor chain. Recheck
  header consumers together.
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
- Invented formals/results in script- or event-facing wrappers and private
  externs: REQUIRE retail callers stage no such register and the callee def
  ignores it. TRY removing it from owner header, definition, wrapper and every
  caller in one draft (`dizzy_kill_pfx` four formals; `myvel_his_angle_y_inout`
  extra `current_args` + `int` return -> `void(float,float,float)`;
  `init_3d_move_no_aniproc` `int(void)` vs `void(void)`; `mcard_msg_*` unused
  `int device`; `obj_scale_over_time` `int(int,int,void*,float)` extern ->
  `void(MkObj*,Vec*,float)`). A false local decl can be what makes a sibling
  match (`attach_pfx_to_object` needs `fx_resume_emit(int)`): measure every
  consumer before correcting.
- Handle types: canonical unsigned at the public boundary (`fx_by_owner`,
  `fx_resume_emit`); blanket unsigned locals flip `cmpwi` -> `cmplwi` in exact
  callers, so keep signed private snapshots where retail compares signed.
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

- An exact allocation path retains an apparently same-type pointer cast:
  REQUIRE independently symbolized distinct free/used record tags, matching
  layouts and genuine state transitions. TRY recovering separate record and
  heap-list types across all consumers (`normHeapMallocMem`,
  [evidence](reports/distinct-free-used-heap-headers.md)). Keep explicit casts
  where blocks change state; do not invent types to inhibit propagation.
  Verify declarations, footer ownership, every includer and all relocation modes.

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
- Opaque root plus repeated member casts caches a subobject across calls where
  retail recomputes it: REQUIRE the canonical allocation/layout and unchanged
  pointer ABI. TRY typing the parameter in declaration and definition and using
  direct members (`mkScreenEngineResourceLibrary` destructor: `strings` at +8).
  This can remove an extra saved address and frame growth; verify every caller,
  function, data layout and relocation instead of forcing the address lifetime.
- Shifted view caches an interior address where retail retains the root:
  REQUIRE native IRO CSE evidence, proven header/record stride and field extents.
  TRY direct canonical record members in the affected consumer and its complete
  search operation (`get_bg_pselect_team_textures`). A broad getter rewrite
  regressing exact siblings does not rule out local canonical ownership; verify
  all consumers and preserve separately required inline/named-local boundaries.
  Use an existing count-plus-payload array when its full extent and bounds are
  proven; never obtain that addressing by indexing past a separate count member.
  Remove the interior entries view entirely when choosing this root shape;
  reading count through the root while retaining the view can duplicate the
  address (`bg_pselect_player_canceled`, partial). Check other index producers
  separately: a restored update-load does not resolve cross-callback CSE.
  A copy loop followed by count publication can require two root-plus-stride
  addresses: canonical `entries[i + 1]` and `entries[0]` retain the stride while
  rematerializing the second address (`bg_pselect_save_team`). A shifted view
  instead CSEs the interior pointer; verify the distinction in initial PCode.
- Retained subobject base accesses fields beyond its reconstructed extent:
  REQUIRE known enclosing fields at those offsets and unchanged physical
  layout. Move the real tail fields into the canonical subobject, update all
  consumers, and recheck every green (`mk_chess_cursor_go_to_new_track`,
  manager +0xD8). No padded view or magic-offset owner cast.
- Wrong owner even at unchanged score: sparse overlay with padding standing in
  for pointers, sibling-type view, pointer in 32-bit int. Breaks wide-pointer
  builds. Use canonical typed slot/owner. Two padded views of one timer can
  also change member-load scheduling; use the shared timer's existing fields
  after checking every offset (`sfmpv_IsSkip`).
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
- Proven fixed group of elements per allocation: group the constant product
  in `count * (N * sizeof(*items))`. Left-associated `count * N * sizeof`
  can emit a shift then multiply instead of retail's one multiply
  (`generate_door_paths`, four waypoints per door).
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

- A real byte-offset snapshot folds away and swaps a saved home with a range
  value: REQUIRE the consumer's canonical unsigned offset type, bounded signed
  arithmetic, and native propagation/allocation evidence. TRY that API type
  on the complete snapshot at its original calculation boundary
  (`save_to_memcard_w_error`,
  [evidence](reports/memcard-unsigned-offset-ownership.md)). The implicit
  conversion can retain the owner where a signed local disappears. Preserve
  the arithmetic tree and retries; validate spill rounds as well as final
  colors. No arbitrary rank changes, cast chains, or added uses.
- An indexed load becomes a shared record pointer before admission: REQUIRE
  native ordered-CSE evidence that equal-width scalar type identity controls
  reuse, signed bounds and a real complete helper. TRY the supported `int` /
  `long` index boundary; keep direct enumeration and the helper consistent
  where retail shares the address, and retain decoded-ID result types
  (`setup_sound_bank_list_by_mode`,
  [evidence](reports/sound-bank-index-type-cse.md)). Same width does not imply
  the same MWCC scalar type. Recheck every consumer; no pointer views, casts,
  qualifiers or CSE policy controls to manufacture a boundary.
- Float-to-byte colors have the right operations but a different schedule:
  REQUIRE unsigned-byte destination/range and native evidence that an `int`
  intermediary creates dead narrowing masks, whose later deletion triggers a
  second schedule. TRY direct `(unsigned char)` conversion of each channel
  (`ProcessSpecularity`). Compare pre/post-RA schedules; do not substitute
  signed char for channels 128..255 or change multiplication grouping. An
  instruction deleted after allocation can change scheduling without appearing
  in the final opcode-count difference.
  If channel owner differences remain after conversion, REQUIRE capture shows
  staged locals span raw light, scaled light and tint-product values. TRY the
  complete `tint * (scale * light)` expression at the byte store, removing unused
  staging locals (`GCSpecSkinMaterial`); preserve multiplication grouping and
  the ordered clamp. A rejected staging control with the old conversion
  boundary does not predict its result after that boundary is corrected.
- Unsigned color loads schedule before position stores although final opcodes
  agree: REQUIRE native evidence of signed destination conversions present
  before allocation but absent in final assembly, and a verified RGBA payload.
  TRY the existing unsigned-byte publication of the real color word instead
  of an auxiliary signed channel view (`render_col_shape_as_quad`). A signed
  byte-view control distinguishes the conversion from a new pointer owner.
  Check shared consumers and both schedules; do not change palette constness
  or shared RenderWare types. Before calling a conversion absent, normalize
  record-form `EXTSB.`/`EXTSH.` and account for sign-extending loads; a
  mnemonic-count difference alone does not prove a deleted node. See
  [the quad publication report](reports/quad-color-publication.md).
  Include indexed/update stores in the unique-definition/sole-value-use check.
  A conversion already removed before scheduling may be unrelated to the
  remaining mismatch; an unsigned source view can also lose reuse of a signed
  sentinel load (`TextItem::RefreshOption`). Keep the signed guard and reject
  extra loads or sibling regressions. See the
  [C++ conversion census](reports/cpp-store-conversion-census.md).
  Inspect indexed stores and conversions whose data result becomes unused
  after store forwarding. REQUIRE stage-specific proof and preserved input
  sharing: an unsigned view can merely replace an early `EXTSB` with a mask,
  or split a shared character read (`cvFsGetFileSize`;
  [paired controls](reports/cvfs-byte-conversion-boundaries.md)).
- Masked array index also crosses a byte inline-parameter boundary: REQUIRE
  retail retains both caller/helper checks and the existing helper's byte
  type is supported. TRY the promoted mask expression at each real use
  (`mwFileGCHandle::handle`). A byte local can merge the checks before initial
  pcode; a named full-width local can restore them but change ownership. Use
  H15 for that localized residue; do not widen the helper or add aliases.
- Signed word clamp emits a select where retail keeps two arms/reloads:
  REQUIRE canonical field and local types, including integer rank, plus the
  bound's type. TRY restoring CRI `Sint32` as signed `long` and the original
  `int` bound in `len < 3 ? len : 3` (`sfmpv_GetActiveSize`,
  `sfmpv_NeedSafeDlmRefresh`). Equal-width `int` and `long` are not interchangeable
  to MWCC. For array bounds, `(int)sizeof(array)` preserves the signed int
  constant without a magic extent (`adxsjd_decode_prep`). Recheck every header
  consumer; a long-cast bound or cached clamp can change the CFG again.
- CRI validation keeps a field live where retail reloads it after buffer
  checks: REQUIRE donor `Sint32` rank and the same field offsets, loop and
  guards. TRY canonical signed `long` parameter fields plus validation
  result/index and unsigned `long` pointer-word checks (`sfmpv_InitInf`).
  The missing load can already be absent in initial pcode; recheck every
  header consumer instead of adding volatile or synthetic aliasing effects.
- CRI source agrees with a donor but a pooled-string base trades registers
  with a size field: REQUIRE the donor's actual `Sint32` definition and
  matching field widths/offsets. TRY restoring signed `long` across the
  private record, primitive parameters and arithmetic locals
  (`sjrbf_GetChunk`). Restore sibling locals too: mixed `long` fields and
  `int` intermediates can regress otherwise exact consumers. Check all
  consumers and literal bytes before blaming string-pool spelling.
- CRI padding size trades registers with its sample count: REQUIRE the donor's
  callback typedef and both provider signatures, not just local widths. TRY
  restoring `GetChunk` channel/length to signed `long` with the signed byte
  accumulator (`ADXSJD_ExecHndl`). Check all header consumers: restore the
  minimum's byte-count field and callback-length locals to their canonical
  rank; retain a donor call-site expression instead of a separate size local
  when that is the original ownership. Reject any collateral regression.
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

- Retail `NOT; AND`, current `ANDC` for a dynamic complement: REQUIRE the
  selector's actual operand types and a proven shift domain fitting signed
  `int`. TRY the ordinary signed mask literal when an unsupported `U` suffix
  removes its implicit conversion to the unsigned destination
  (`mk_chess_iterate_thru_move`: capture `7 << (x*3)`, guarded x0..9).
  GC2.7 inserts `ETYPCON(EBINNOT(...))` at compound-assignment lowering,
  blocking the direct `EBINNOT` selector fold. Preserve independently proven
  unsigned masks in other paths; never add casts or risk signed overflow to
  manufacture this boundary. See [native evidence](reports/chess-mask-literal-rank.md).
- Narrowed byte into one proven bit: typed bit assignment; keep full-width
  nonzero test if it gates a separate action.
- `li 1` + `rlwimi` into byte = one-bit field assign
  (`flags_bits.konquest_mode = 1`); live r3 const = insert input, not arg.
  `li 0` + `rlwimi` at distinct positions = separate field clears; keep store
  order + reloads.
- `stb; extrwi` from the just-written byte, then a second byte RMW: REQUIRE
  the second field consumes the first assignment's value. TRY the genuine
  chain `dst_bit = src_bit = value`; GC2.7 extracts a consumed bitfield
  assignment result, whereas separate store/reread can fold into a bit copy
  (`start_protect_effect`). Preserve right-to-left store order.
- `lwz` + `clrlwi 24` = low numeric byte, not byte at address.
- `extrwi.` of one flag, or extract + Boolean normalize: existing bitfield
  or `(flags >> bit) & 1` keeps normalization a raw mask erases
  (`ScreenActionCheckOnline::Update`). Preserve scalar storage and lazy reads
  guarded by the flag.
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
- ORs become MR plus RLWIMI before allocation: REQUIRE native pass snapshots
  locating the change in the early rotation scan, not the later store-byte-swap
  rule. TRY observing the actual scan's source-redefinition, record-form,
  register-identity and disjoint-mask predicates (`SFH_AnlyMaxFrmNum`). GC2.7's
  early pass can run twice; pair both windows and verify the complete observed
  object against an ordinary compile. A successful mask test explains a fold,
  not an honest way to prevent it. Do not add a dead branch, store or lifetime
  merely to reject a predicate; recover a real source boundary or retain the
  ceiling.

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
- Paired rotations share arithmetic but differ in input reloads: REQUIRE
  retail's initial product-input reads, later alias-sensitive reads in the
  caller-matrix form, and separate trig locals/output matrix in its sibling.
  TRY real snapshots for the initial products while keeping every later
  pointer read; use a complete value-input operation for the separate-local
  consumer (`YXZ_angles_to_MKMATRIX`, `YXZ_angles_to_quat`). Each snapshot
  must feed actual work; do not remove a required reload to share one helper.
  Verify both consumers and the whole object. See
  [the input-lifetime evidence](reports/yxz-input-lifetimes.md).
- Buffer reused as callee output != retained input; keep invariant
  displacement separate. Verify with nonzero output mutation.
- Const qualifier: retail stores via out-pointer before loading handle state,
  compiler hoists load -> drop unsupported `const` on handle
  (`SFH_AnlyNumElem*`). Vector setter with interleaved component load/store:
  verify writable caller inputs + related setter signatures; bogus pointee
  const hoists all source loads before dest stores
  (`camera_set_center_of_rotation`). Keep genuine const (M12).
- Caller guard reuses a field as an inlined helper's first call argument,
  but candidate reloads it: REQUIRE no intervening store/call, mutable caller
  records and independent evidence for the helper's mutable pointer API.
  TRY removing unsupported pointee const consistently through private helper
  declarations and forwarding callers (`sfmpv_GetActiveSize`, `sfmpv_GoDdelim`).
  Top-level pointer const is a different change; do not apply it as a substitute.
- A complete handle-validation operation only reads its owner, but the caller
  later mutates that owner and retail reloads its destruction receiver:
  REQUIRE the validation API is genuinely read-only and the caller remains
  mutable. TRY `const Owner *` on the private validation helper, consume its
  result as a guard, then read the mutable receiver from its actual field
  (`object_set_state`). This can preserve arithmetic CSE that a whole-function
  mode change loses. Verify against the same helper with a mutable parameter,
  inspect initial PCode, and check every caller; do not const-qualify the
  mutating caller or invent an alias. See
  [the Konquest latch report](reports/konquest-readonly-latch.md).
  This is pointee const, distinct from an immutable `Owner * const` binding.
  Existing CSE-off restores the reload but leaves a volatile-home swap:
  REQUIRE the same complete const-owner API with paired mutable and policy
  controls. TRY normal CSE with that read-only validator; remove the old
  workaround only when all consumers remain protected (`WifImage::Close`).
  See the [policy-removal case](reports/wif-image-readonly-validation.md).
  The same operation restores repeated prompt cleanup reloads without changing
  setup/fade policy; remaining ownership is separate. See the
  [text-window latch report](reports/text-window-readonly-latch.md).
- Pre-call snapshots: loads right before call -> typed locals at that
  boundary, one at a time (`sfmpv_DecodePicAtr`). Distinct reads across
  intervening calls, not one long alias.
- Asynchronous counter reload: REQUIRE a registered interrupt/thread writer
  chain and repeated retail reads of the same field. TRY `volatile` on the
  canonical member, checking every includer; keep a separate genuine snapshot
  when retail captures it across conversion (`SFTIM_IsGetFrmTimeTunit`:
  registered VSync thread reaches `sftim_VbInHn`, which updates the frame
  clock). A matching reload alone is insufficient evidence; never qualify
  only an access cast or unrelated timer fields.
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

- A log publisher and its inlined consumers share wrong record-address
  islands: REQUIRE distinct existing head and published-current indices,
  canonical record layout, and no intervening mutation of either index.
  TRY publishing `current_log_index = log_index`, reading the `last_*` pair
  through the published current index and the `current_*` pair through the
  authoritative head, then reusing the original publisher in every consumer
  (`update_my_last_switch` and six flying attacks). This preserves ownership
  that a single record/index snapshot erases; it can recover separate field
  bases and indexed loads. Verify every consumer, unrelated code, data and
  relocations. Exact instructions can retain equal-value pool-address
  differences; do not infer link exactness or an unobserved compiler pass.
- Retail keeps a SECOND web of a parameter after its guards (guard reads the
  arg register, then `mr rX,r3`; or extra parameter copies feeding helper
  guards), ours keeps one long-lived parameter home: REQUIRE retail IR shows
  the second web in every sibling and an independent anchor (PS2 counterpart
  splitting the value at the same point). TRY one channel local assigned once
  after the guards and used by every later card/API call (`gc_delete_file`
  exact; `save_to_memcard2`, `load_from_memcard2` structural). Keep uses that
  retail reads from the root on the parameter (`last_card_serial_no[device]`).
  A same-type `int` copy is erased by IRO copy propagation (`IsPropagatable`:
  plain variable RHS); a converting copy survives. Only `unsigned int`
  reproduced retail: SDK prototypes take `s32` and PS2 derives port/slot with
  signed ops, so the type is IR-justified, not prototype-justified; say so.
  Declare it before a loop counter, or IRO still drops it (`save_to_memcard2`).
  This is the user-ruled exception (brief ruling 1, ivy #8833) to tier 1's
  "copy of in-scope value"; without retail IR plus an anchor, reject it.

- A field shared by a predicate and guarded update has a CSE/codegen home
  swap: REQUIRE native initial pcode proves an early object-backed CSE owner,
  and the compound form's extra load folds into the original producer before
  allocation with all final effects preserved. TRY the ordinary compound
  member update (`ImageList::Increment`: `scrollBase += delta`). Isolate the
  actual update; unrelated compound assignments can be neutral. No extra
  reads, aliases or casts. See [the ownership report](reports/image-list-compound-update.md).
- A captured field becomes an early CSE owner after its standalone assignment
  is substituted into the first range guard: REQUIRE a native propagation
  record, the same snapshot and bounds, and no intervening effect between
  independent reads. TRY using that real assignment's value in the guard,
  `if ((device = pos_device) >= 0 && ...)` (`p_delete_profile`). This can
  retain the existing helper local and remove the later CSE web. Verify the
  actual propagation decision, every helper consumer and the complete object;
  no extra copy, read, qualifier or dependency. Remaining seed scheduling is
  independent. See [the guard ownership evidence](reports/profile-delete-guard-ownership.md).
  A snapshot refused for use in another flow block does not meet this
  prerequisite (`p_show_text_window`, `mk_chess_end_of_turn`); the presence of
  a first guard alone does not justify moving its assignment.
  If native propagation instead refuses the existing assignment because its
  value is used in another flow block (`generate_door_paths` enumeration
  snapshot), the substitution prerequisite is absent; do not try guard-value
  spelling solely from the adjacent source assignment and comparison.
  For a source-census hit, REQUIRE a local destination and actual expansion
  into the target; a global publication or outlined helper's local does not
  establish this mechanism. Removing a confirmed copy can still leave all
  allocated output unchanged (`skytemple_player_explode`); one fewer graph
  node is not a matching gain. See [the qualified census controls](reports/guard-value-source-hypotheses.md#subsequent-native-guard-preflight).
- A staged conversion survives the first expression pass but propagates after
  CSE replaces its global input: REQUIRE the actual assignment dependency,
  first invalidating store, second-pass splice and independent state/input
  order. TRY staging the real quantized outputs before pointer publications
  and resetting independent state before sampling the input
  (`trial_show_text_window`). A nested snapshot definition can repair the
  schedule only partially and reverse a product's operand order; it is not
  required when both output values survive naturally. Confirm zero/conversion
  temporary creation order separately from named-value homes. No invented
  use, escape or claim about an unobserved eligibility predicate. Shared
  note 9224 preserves the conversion-availability evidence.
- Zero and integer-to-float setup homes rotate, possibly with a reversed
  first multiply: REQUIRE native temporary creation order, independent
  state/payload owners and unchanged FP/rounding trees. TRY publishing the
  independent zero before plain cast-multiply expressions into the real
  quantized outputs, then publish those outputs at their original pointer
  stores (`ending_show_text`). Preserve the proven input-read interval: its
  existing raw width snapshot can precede the zero; another consumer may
  need the independent zero before its input read. A nested assignment can
  reverse multiply operands. Check both conversions, all publications and
  whole-TU relocations; no redundant snapshot or reordered aliasing effect.
  Shared notes for `ending_show_text` and `trial_show_text_window` preserve
  the measured output-staging closures.
- Repeated comparisons share an indexed element address whose CSE home swaps
  with a real named flag: REQUIRE the same array element, unchanged index,
  ordered member reads and no intervening aliasing effect. TRY a typed pointer
  to that actual element in its existing traversal block, then measure the
  original flags' same-scope declaration order (`setup_per_tile_navigations`,
  partial). Verify the pointer survives as a named owner: the kind repair alone
  can rotate the flags and regress. No copied source value, extra read, dummy
  use or changed geometry; check the whole object and FP class separately.
- Indexed sphere rendering retains a center address while radius reads use
  the parent node: REQUIRE native ownership evidence for those distinct
  values, identical dispatch cases and proven array/Vec stack roles. TRY an
  indexed-only complete sphere expansion owning the actual `Vec* center`,
  reached through a complete dispatcher at the original inline depth
  (`render_player_joints`, partial). A named shape pointer can redirect radius
  reads and lose parent liveness; direct leaf calls can regroup inline arrays
  and swap stack roles. Keep shared generic callers unchanged. Verify native
  object agreement, every stack operand and whole-unit relocations before
  accepting a fuzzy gain; stop when only center/index coloring remains.
- A post-guard owner reload duplicates the guard's memory read: REQUIRE no
  intervening effect and native CSE/copy stages showing the named owner lost
  to a generated value. TRY capture the existing owner in that first guard
  (`mk_chess_drone_heal_ai_casting`). Preserve publication-sensitive reads
  before it. Verify the named owner survives; this need not share the
  expression-propagation mechanism above. Do not add a redundant snapshot.
  See [the piece ownership evidence](reports/chess-healing-guard-ownership.md).
- Repeated rounded axis products have the right operations but six extrusion
  and corner homes differ: REQUIRE one actual vector displacement shared by
  the extruded corners, unchanged geometry/store order and resolved output
  conversion residue. TRY compute the axis-times-span value once in a `Vec`
  and use its float components in the original corner sums
  (`render_col_shape_as_box`). Float assignments retain the product rounding;
  verify all consumers and do not group unrelated values. See the
  [box extrusion report](reports/box-extrusion-vector-ownership.md).
- A nested helper's destination copy is absent from backend initial PCode:
  REQUIRE both actual inline-argument creation and target-specific IRO rewrite
  records (`mwPlyInitSfdFx`). TRY tracing the created formal through the first
  copy pass: CInline can create it correctly and IRO can substitute its parent
  at every store. Do not infer failed argument creation or try coloring from
  initial PCode alone. Recover a supported type/availability boundary; never
  manufacture a qualifier or use. See [the paired phase evidence](reports/sofdec-init-destination-copy-propagation.md).
- A join copy reaches initial/prescheduled PCode but vanishes from assembly:
  REQUIRE the actual simplify-stage alias roots before blaming frontend
  propagation. Both temporary webs can coalesce (`mk_chess_spell_hud_show_my_spells`).
  With independently supported addressable outputs, TRY the complete
  two-output selection and read-only input contract; recover caller-owned
  persistent traversal state only at a real repeated phase boundary. Check
  native recursive inline cost, both induction domains and the whole unit.
  No redundant local or helper fragment just to defeat coalescing. See
  [the partial HUD recovery](reports/chess-spell-coordinate-inputs.md).
- Input index is saved at entry, while retail saves it only after successful
  lookup: REQUIRE native argument/copy provenance, an independent late-save
  anchor and repeated complete calculation/publication sites. TRY a real
  read-only scalar input to that complete success operation
  (`pfx_bind_render_to_obj_bone`: calculate world matrix, then mark its flag).
  Keep lookup on the original parameter and check every consumer's signedness;
  no saved-index alias, conversion or incompatible scalar-pointer cast. See
  [the index-input recovery](reports/particle-bone-index-input.md).
  For a repeated poll, REQUIRE the caller genuinely consumes its match result
  to select sleep/reset policy. If borrowing the port restores the late save
  but swaps port/match homes, TRY the complete poll with caller-owned match
  output, preserving the root countdown loop and X/Y stack ownership
  (`mk_chess_handle_repeatable_input`). No extra flag use or saved-port copy.
  See [the poll-input recovery](reports/chess-repeat-input-port-ownership.md).
- A complete hardware publication receives an eagerly loaded byte while
  retail needs a distinct widened input owner: REQUIRE the real indexed byte
  slot, one read before every output store, and proven destination width.
  TRY borrowing that read-only slot and holding its value in the actual
  halfword publication type (`SBPlayable_Stream::iUpdate_AXUser`, partial).
  Retrace the graph before rechecking a previously rejected caller-owned
  query output or endpoint operand order; an independent input fix can make
  those forms compatible. Preserve the mono guard, raw outputs and sync
  sequence; no duplicate read, artificial lifetime or width sweep. See
  [the combined input/output evidence](reports/sound-stream-block-output-ownership.md).
- A staged conversion survives one expression-propagation pass but disappears
  after CSE: REQUIRE actual assignment availability and short-circuit predicate
  results for both passes (`trial_show_text_window`). TRY tracing the first
  changed dependency/availability event before proposing a source boundary.
  One-use counts can remain equal while CSE changes the RHS owner and enables
  substitution. Do not infer a use-count refusal from another function or
  invent a second use. See [the native gate evidence](reports/text-window-expression-propagation.md).
- Shape bases disappear in backend add propagation while comparable bases
  survive: REQUIRE the actual native callback table, complete producer use
  lists and predicate/commit decisions (`update_player_collision_nodes`).
  GC2.7's annotated ABI MR has a third kind-6 operand; its displacement check
  can refuse folding although a short MR is accepted. TRY recovering a real
  address-use or expansion boundary, only with retail evidence; do not invent
  a whole-address use. See [native use-list evidence](reports/gc27-add-propagation-uses.md).
- Candidate shares a product retail recomputes across a latch: REQUIRE actual
  CSE availability, ordered tree/type predicates and reuse-store evidence
  before changing ownership (`mk_chess_hud_set_piece_portrait`). TRY tracing
  the first divergent selection; identical native scalar types and ordered
  operands do not justify inventing a cast or qualifier to prevent sharing.
  See [the native comparison evidence](reports/gc27-ordered-cse-comparison.md).
- Retail recomputes encoded flags at an inlined queue's test and stores, while
  ours shares normalized arithmetic: REQUIRE the two actual sentinel meanings
  and native pass snapshots. TRY selecting the sentinels directly
  (`flag ? -1 : -2` in `broadcast_screen_studio_event`) instead of encoding them
  as `(flag != 0) - 2`. Both actual arguments substitute directly during
  inlining. The arithmetic form is merged by IRO common-subexpression removal;
  the conditional form remains control flow there and is reconstructed by
  `RebuildCondExpressions` afterward. Preserve the generic queue's mask/sign
  behavior and every call/store; do not use impossible comparisons, fake
  volatility or extra copies to prevent sharing.
- Allocation extent sum adds its first field last, while retail accumulates
  the fields in order: REQUIRE native EADD/Operands_Add evidence distinguishes
  a lazy indexed pair from a register plus displacement, and an existing real
  constant allowance in the same sum. TRY placing that allowance first
  (`pfx_estimate_size`). The initial register-plus-allowance operand can make
  later additions form a running extent, with the allowance deferred to the
  final ADDI. Keep the exact fields, integer types and total; never introduce
  a constant, artificial scalar barrier or extra consumer to force this path.
  Inspect the inner materialization as well as the root: a root destination
  hint can otherwise consume a lazy pair directly into the final owner.
  See [the factory recurrence](reports/particle-factory-allocation-owner.md).
- Perpendicular components and a position snapshot exchange homes: REQUIRE
  native evidence distinguishes named scalars, scalarized vector members and
  a duplicate-load CSE. TRY the actual XZ vector plus the existing snapshot
  at both dot-product and publication uses (`bgnd_place_point_light_for_ticks`).
  Vec alone can merely move the swap to X; preserve the interval without
  intervening writes, and never read an uninitialized unused component.
- First field of an indexed descriptor uses a computed record address, but
  retail uses `lwzx` while its next field uses that address: REQUIRE canonical
  element layout and identical reads. TRY direct `table[index].field` reads
  instead of a once-used record-pointer local (`pan_vol_pitch_random_hit`).
  Check both accesses; copying the whole descriptor can create stack stores.
  A separate derived-row pointer can also precede a flags read that propagation
  sinks to its use. Direct indexed reads remove that address statement and
  preserve retail order (`pfx_estimate_render_size`, exact with propagation
  both off and on). Verify every field read and the whole unit under each mode;
  a neutral source repair does not authorize a flag change while siblings lose.
- A shared event lookup folds a fixed field into a computed record address:
  REQUIRE canonical nested fields, a sentinel test followed by the same
  nonvolatile read, and no intervening mutation. TRY direct indexed field
  reads in both test and return instead of capturing a slot and value
  (`mk_chess_request_piece_script_for_action`). Keep the real out-of-range
  and sentinel fallbacks. Pure inline predicates may use the actual global
  piece at both predicate and lookup sites; real calls still require the
  observed reloads. The measured closure combines those changes, so do not
  attribute it to either edit alone. Check every shared helper consumer and
  raw relocations: CLI `all` can still accept local call-target offsets that
  differ from retail.
- A vector delta's first component loses its owner before publication:
  REQUIRE native IroPropagate snapshots show a single-use subtraction
  substituted into the first destination update while later components
  survive aliasing stores. TRY consuming the saved original-position vector
  in place as `saved.x = position->x - saved.x`, then publishing the completed
  displacement (`repel_against_obstacle_list`). Copying current position to a
  second vector first can tie its load and subtraction result to one owner;
  consuming the saved vector separates them while retaining a self-referential
  definition. Preserve all samples before writes and FP operand order; verify
  initial PCode and the whole unit. Never invent a second use or an escape.
  See [saved-origin ownership](reports/collision-saved-origin-displacement.md).
- Adjacent fields use different address roots at a branch join: REQUIRE
  native IRO expression trees and available-expression bits, not just the
  final register diff. An address first formed in a lazy latch need not
  dominate the join even when its neighbor does
  (`npc_manager_find_model_for_npc`). TRY recovering shared-root expression
  ownership while preserving the lazy load. A typed field/slot pointer can
  hoist another saved base into an earlier search; measure and reject that
  extra lifetime. Do not add eager reads or fake prefix/offset views.
- Table base is copied or rebuilt between an inlined busy scan and service
  loop: REQUIRE the same actual table and vendor evidence for both helper
  boundaries. TRY indexing `table[i]` in both helpers and passing the same
  owner expression (`gcCiReqRd`). An advancing scan cursor can prevent the
  shared base from surviving into the service loop. Keep the service index
  in its real helper; check the separate public server and all consumers.
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
  subarray bounds, no invented byte offsets. A neutral value-return getter
  does not close the typed-slot control; the caller's address-then-read can
  preserve the indexed operand through member displacement. See the
  [spell-page slot case](reports/chess-spell-page-slot-address.md).
- Runtime plugin member emits ADD plus displaced LBZ, retail ADDI plus LBZX:
  REQUIRE canonical plugin layout and native operand evidence that an inline
  pointer result materializes before the field displacement. TRY a real owner
  snapshot and direct typed member access through the existing plugin-macro
  pattern (`SpecSkinProcessMaterialList`). Keeping the indexed operand through
  the field addition selects `Operands_Add` cases 26/46; returning or naming
  the plugin pointer can force a GPR first. Recheck repeated flag accesses and
  all header consumers. No literal field offset or artificial byte snapshot.
- Word transfer within an owner's array: retail keeps root + scaled index +
  member offset for `lwzx`/`stwx`, direct assignment shares a member base.
  REQUIRE canonical element type and unchanged index reads/transfer order.
  TRY typed source/destination slot pointers followed by the dereference copy
  (`_copy_register_to_register`); signed pointer types cannot substitute for
  unsigned register storage.
- Serialized reference table: retail forms owner + scaled index, then uses
  displaced member loads/stores; direct indexing emits index + member offset
  with indexed accesses. REQUIRE the canonical table layout and unchanged
  relocation/load/store order. TRY a typed pointer to the actual slot for
  publication and a separate value snapshot read directly from the table
  (`PatchAnims`, `PatchAttribueList`); a slot-only read or C++ reference need
  not preserve that address grouping. No shifted struct view or fake offset.
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
- Projection helper hoists a position address across a branch while retail
  loads components from the object and forms the address only in call arms:
  REQUIRE the same typed object and component offsets at every helper use.
  TRY an object-taking projection helper and read-only owner snapshots in
  retail load order (`repel_players`); preserve global reloads after calls.
  If two returns share axis loads at their comparison ancestor, REQUIRE the
  native initial-code component/shared-axis order. TRY assigning the two real
  scalar results to the existing projection locals before comparing them,
  retaining that object-taking helper and both FP trees. This can put the
  first component reads before shared axis loads without a scalar-argument
  wrapper, duplicate alias or artificial dependency. Check later fresh
  projections and all consumers.
- Field-address load-update alone != pointer-to-field local.
- Reused scratch subobject address swaps a saved home with a long-lived owner:
  REQUIRE capture identifies the anonymous loop-invariant address and replay
  predicts named ownership. TRY a typed pointer to the actual scratch member
  at the observed phase entry, use it for all corresponding stores/calls
  (`set_plyr_attack_region`). Preserve address-formation placement across
  earlier calls. If only the real index/address pair remains, measure the
  declaration order predicted by replay; do not add another alias or change
  the scratch representation's initialization.
- Index kept across calls, table addresses rebuilt later = typed index, not
  cached entry ptr (`run_ending`, right-stick dispatch).
- Indexed polymorphic rows: retail reloads the node after virtual callbacks,
  source keeps an earlier pointer in a saved GPR. REQUIRE canonical base
  vtable + each reload boundary. TRY `owner->nodes[index]->Method()` at each
  boundary, with a fresh typed row view only where its fields are consumed
  (`TextList::Update`). Shortens the cross-call owner lifetime; confirm
  initial PCode/interference changes before treating saved-home swaps as coloring.
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
- Vendor buffer view: retail forms `owner + i*stride` and folds the array
  base plus member offset into every load displacement, while any typed
  element/subobject pointer materializes the base first (`addi base;
  add`): REQUIRE a byte-matched vendor donor defining the view and a user
  ruling (CombineEAddTerms canonicalizes only `EINDIRECT(EADD)`, so no
  view-free spelling exists). TRY the donor's padded view as the named
  local (`SfmpsBufHn {u8 pad[0x1308]; SfdBufferState buffer;}`,
  `SFMPS_BUF_HN(handle, n)`), reading every field directly through it
  (`sfmps_ProcPrep` exact). A typed pointer derived from the view folds the
  constant back. See [the buffer view report](reports/sofdec-buffer-view-producer.md).
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

- An independently rebuilt library donor has the same retail function body,
  but isolated helper substitutions regress: REQUIRE a pinned-compiler donor
  control and the actual macro predicates, argument evaluation, definition
  visibility and shared consumers. TRY the complete verified operation closure,
  restoring real helper calls in consumers that currently duplicate its body
  (`__pool_realloc` / `Block_link`,
  [evidence](reports/allocator-canonical-helper-closure.md)). Preserve target-specific
  semantics and audit every consumer; a successful combined closure does not
  prove each individual spelling necessary or authorize copying the whole unit.

- A small arithmetic helper preserves floating-point owners, but the retail
  MAP names a larger stripped operation: REQUIRE verified counterpart calls,
  arguments, field offsets and effect order. TRY recovering the complete
  operation with its real parameters; direct products may then replace the
  micro-helper without changing code (`ending_show_text`, partial;
  [evidence](reports/ending-window-helper-boundary.md)). Explain automated
  size/call disagreement through the actual inlining boundary. Preserve the
  target's repeated conversions even if another platform shares them, and
  do not infer that recovering the helper fixes remaining register residue.
  When real adjacent helpers jointly own stack locals, recover the complete
  pair before judging a one-helper stack displacement; preserve acquisition
  versus publication lifetimes (`screen_engine_fire_switches`, partial;
  [evidence](reports/screen-input-helper-boundaries.md)). Verify definition
  visibility under the current inline policy; MAP order alone does not justify
  changing flags to make later bodies inline.

- A base-destructor ABI wrapper is parsed after derived callers and native
  `can_inline` rejects its null saved-body information before cost evaluation:
  REQUIRE retail expansion of the complete base destruction, the real class
  chain, and unchanged nullable/delete-flag semantics. TRY one typed private
  inline body available before use, shared by the derived wrappers and the
  original standalone base entry (`SpreadSheet_image` / `SpreadSheet_text`
  destructors). Keep public entries in retail order and verify the base body,
  all consumers and data; no private helper should remain emitted. Moving the
  public base definition can change order, while annotating that public entry
  can remove or defer it. These are separate emission failures, even when the
  derived instructions match. Do not manually duplicate null guards.

- Reusing existing complete phases restores frontend result ownership but
  makes the consumer expand into retail out-of-line callers: REQUIRE native
  ownership evidence, original caller boundaries, and whole-unit mode-ladder
  measurements. TRY the narrow consumer constraint with the existing helpers
  unmarked before adding explicit `inline`; helpers parsed with automatic
  inlining enabled can retain usable inline metadata inside a later scoped
  `auto_inline off` consumer (`fire_screen_studio_event`,
  [evidence](reports/glue-event-inline-ownership.md)). Compare raw unscored
  output too: redundant annotations can remove or reorder helper bodies even
  when every scored sibling is unchanged. Follow the approval gate for a
  compiler constraint; this observation does not authorize bypassing it.
  Constrain the definition whose automatic eligibility must change; placing
  the scope only around its caller can be neutral
  ([camera evidence](reports/camera-animation-setup-boundary.md)).

- An existing address-taken callback implements an identical completion tail:
  REQUIRE matching reads, effects and failure exits, plus canonical auto-inline
  settings. TRY calling that existing unannotated function before marking it
  inline or duplicating its operation (`mslBankReadAssetHeaderComplete`). Auto
  inlining can retain a genuine context owner while preserving standalone
  emission order; explicit inline can defer the callback and disturb pools.
  Recheck all functions and data. Retrace independent traversal counters after
  this boundary changes; earlier allocation constraints may no longer apply.

- Reusing a complete inline phase fixes its consumer but lowers auto-inline
  cost enough to expand that consumer into an exact callback: REQUIRE retail
  calls to the consumer and unchanged shared helper semantics. TRY scoped
  `auto_inline off` on the retail out-of-line consumer while keeping the phase
  explicitly `inline` (`TextList::RefreshCollection(int)`,
  [evidence](reports/text-list-refresh-inline-policy.md)). Check every caller
  and helper expansion; `dont_inline on` can outline the phase too. Compare
  canonical member ownership first when source uses an encoded C entry;
  member syntax alone need not restore retail's call boundary. Counterpart
  calls support that boundary, not the literal pragma spelling. Apply the
  required human approval gate before landing a compiler constraint.
  If an existing `dont_inline` group blocks the required complete phase,
  test `auto_inline off` in that same group with the phase explicitly inline.
  Keep every original retail call boundary and recheck all group definitions
  and callers; removing the group can regress otherwise exact consumers.
  Preserve any remaining typed address expression rather than forcing offsets.

Inline boundary. REQUIRE retail call boundary, signature, active inline
settings. Verify emitted calls, helper symbols, every consumer.

- A new-expression success arm completes object initialization, while source
  tests the allocated pointer again for a setter: REQUIRE the same base/vptr/
  member initialization, late input query and final publication. TRY moving
  that real initialization into the existing fully inlined constructor
  (`gcnHostOpenCommand`, [evidence](reports/mwfile-host-construction.md)). Pass
  the original input by reference when retail queries it after base construction;
  a raw pointer argument can evaluate the query too early. Keep null-allocation
  behavior, layout, lifetime and call order; no unused constructor parameters.
- Conditional integer magnitude CSEs a repeated global read that retail
  reloads: REQUIRE the compiler's registered intrinsic and canonical
  declaration, correct integer range, and the same absolute-value sequence.
  TRY existing `__abs` from `runtime/cstdlib.h` (`get_stick_pos`), rather
  than a ternary or an external `abs` call. Native IRO can otherwise share
  the threshold before backend instruction lowering; inspect initial loads
  and verify no extra call. No fake volatile or asymmetric pointer alias.
- A trivial member getter replaces the consumer's retained pointer with an
  IRO forced-load temp: REQUIRE native creator evidence for EFORCELOAD
  lowering, identical unguarded getter/member semantics, and the same read
  boundary after callbacks. TRY reading the canonical member directly in the
  complete operation (`keep_players_on_tightrope`). Retrace the original
  inline local and global base; do not remove getters with guards/effects or
  cache the value across callbacks. Verify every other consumer unchanged.
  An otherwise byte-neutral direct member read can make the result a named
  owner whose real declaration order repairs later disjoint saved homes
  (`CameraDestroy`: parent frame and raster parents). Retrace before changing
  declarations; the forced-load temporary cannot be reordered that way.
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
- A shared renderer has exact consumers but one dispatcher retains different
  palette-load owners: REQUIRE the original MAP names a distinct complete
  rendering helper, retail byte-load/store order, and unchanged geometry.
  TRY restoring that named operation with its real palette representation and
  local channel captures (`render_col_shape_as_sphere`). Keep existing exact
  consumers at their established boundary; verify all expansions and absence
  of a new outlined body. Native initial/build captures must preserve the
  required channel order and vertex publication schedule. This evidence does
  not justify unnamed per-caller clones or reordered pointed stores.
- Indexed record rebuild around callbacks: REQUIRE selected index and ID
  retained through construction, then global selection reloaded for publication.
  TRY complete indexed removal and build/add phases; return pointer/null at
  the allocation boundary, never a self-normalizing identity assignment
  (`bgnd_set_danger_zone_y_angle`).
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
- Visible template body under `-inline noauto`, retail expands its complete
  operation but source still calls it: REQUIRE the same base operation,
  standalone body and derived consumers. TRY explicit `inline` on that
  existing definition (`mwFileXPReadBufferedCommand::isA`); check every
  instantiation, emission order, named BSS owners and vtable relocations.
- No-setting control proves a pragma is needed.
- Donor/private helper may exist only as inlined island; extract when retail
  groups a whole loop + publications (`sfmps_UpdateStreamBounds`). Unrolled
  clear + folded status tail = status-returning helper (`MPS_Init`); stop if
  every honest form erases dead retail branches.
- Scalar and record-based timer entries: REQUIRE the donor's complete shared
  timing operation, identical policy/call order, and the record entry's actual
  value/scale ownership. TRY the full private timing helper plus a complete
  record-taking entry helper (`SFTIM_IsGetFrmTime`); retain proven asynchronous
  reads (H05), verify both entries and all consumers. Do not extract a single
  reload, invent an alias, or add a lifetime solely to change register homes.
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
- GC2.7 bottom-up cost reports `too_big` at configured maximum + 1:
  REQUIRE checking nested inline depth and eligibility before treating that
  number as measured body size. A nested eligible call with no remaining
  depth can return this sentinel (`basic_string::append(const char*)`).
  Inspect the complete producer and existing definition placement first;
  raising the size limit does not resolve a depth rejection.
  Attribute the native sentinel to its nested callee. Definition-scoped
  pragmas can be neutral for late templates; verify options at actual
  instantiation (`mwFileMemAllocator::construct`, nested placement new).
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
  - Lookup/teardown defined after its consumer: shared inline body + public wrapper
    keeps public order. Retail expands allocation, raster tests, cleanup
    before public def (`image`) -> expose typed impl before consumer, verify
    both bodies. A complete UnCopy/node-free operation owns retained node and
    command-cursor lifetimes; a definition-only helper can swap their homes
    (`mslSoundDeactivate`). Keep explicit public body if wrapper changes codegen. Move
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
  (`flash_hit_at_bid_with_y`). If an existing helper already owns selection,
  allocation, nested effect setup and publication, expose its unchanged body
  before the consumer and reuse the complete operation
  (`mk_chess_cell_hide`, `mk_chess_start_special_cell`). Check its standalone
  body and every consumer. Wrapper around snapshot load only != proven boundary.
  Reuse it at EVERY phase doing the operation; one-phase reuse leaves
  allocation residue. Pass a real field address (`&p->hit_script_index`) so the
  read defers into the validated arm (`p_projectile_downward`: one snapshot
  helper at ground + hit -> exact).

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
  `return 0;` in each failure arm (`reaction_xfer_him`, `get_mission_state`,
  `ScreenModel::Render`).
  Measure per owner.
- Retail folds member offsets into loads where source forms `&array[i]`:
  inline helper taking owner + index (`fighter_severed_limb_live_object`).
- Direct-owner accessors suit adjacent load/validate; cached forms suit real
  effects between. Extracting a latch can change caller inlining; check every
  caller.

## H09

Loops. REQUIRE zero-trip behavior + test/update order.

- Output stride advances before a call while the traversal ordinal advances
  afterward: REQUIRE both retail update points and independent evidence for
  the two live counters. TRY a genuine output index consumed with
  post-increment, retaining the ordinal update in the loop header
  (`setup_per_tile_navigations`, partial;
  [evidence](reports/navigation-contour-induction.md)). Incrementing the ordinal
  at the output use moves both updates early. A synthesized SR register alone
  does not justify a second source counter; preserve bounds and call order.

- A local induction copy rotates the initial constant/table-base homes while
  an existing global state index is published before and after the loops:
  REQUIRE nonvolatile state, no intervening calls or aliasing accesses, and
  matching final index semantics. TRY using that state index in both loop
  headers (`sgenrand`); MWCC may cache it and preserve the observed sparse
  stores. Verify both recurrence bodies and final publication; do not invent
  a global or add volatile accesses to force this shape.
- Indexed count lookup becomes add plus load through an initial snapshot:
  REQUIRE failed scan iterations contain only reads, every mutating success
  returns, and the bound is nonvolatile. TRY the canonical field directly in
  the existing loop condition, removing unused snapshot setup
  (`mk_chess_remove_piece_from_team`). Confirm one bound read before the CTR
  scan and preserve every mutation/callback reload. Do not apply this to a
  bound that can change on a backedge.
- Dynamic bound loaded once into CTR: snapshot bound, keep per-iteration
  pointer reloads. Fixed positive extent: test bounded `for`/`<` vs
  do/while/`!=`. Unsigned ascending index keeps `cmplwi`/`ble` + CTR.
  Fixed nonwrapping clear with serial retail index updates: equality bound
  can select eight-way unroll where `<` selects sixteen parallel offsets
  (`__VMAllocARAMToVirtualLUT`). Preserve extent and reloads.
- Hardware-table clear retains a rounded byte-count CTR and remainder guard:
  REQUIRE retail byte extent, record stride and authoritative base reloads.
  TRY the genuine byte-offset traversal with `sizeof(entry)` advancement and
  a complete typed entry-clear operation (`__VMBASEInvalidatePageTable`).
  An element-count loop can fold the guards; a scaled element-index loop can
  recover them but replace the named byte counter with an IRO temporary,
  allowing value numbering to share its zero. Keep byte-to-record conversion
  at the proven hardware address boundary and preserve each base reload;
  never add a separate byte-offset carrier to force an owner.
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
- Table setup already precedes the loop in initial backend emission: REQUIRE
  native IRO loop membership and outside-predecessor evidence before applying
  the backend preheader rule. TRY tracing frontend preheader selection first.
  GC 2.7 reuses a sole outside predecessor ending in a goto even when the block
  adjacent to the header belongs to the loop (`p_monk_move`). Recover genuine
  entry edges before changing source; no invented flags or duplicate paths.
  See [frontend preheader evidence](reports/gc27-iro-loop-preheaders.md).
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
- Two arrays use different element strides, but the derived index homes swap:
  REQUIRE native BE strength-reduction creation order and identical field
  loads/stores. TRY direct indexed field stores in the complete copy phase
  (`set_game_switch_map`): evaluating the source scalar before constructing
  the destination row can create the source stride first. Named row aliases
  can create the destination stride earlier. Preserve field-store order and
  actual index ownership; do not add explicit scaled counters.
- Nested pixel-copy loop has both derived-address and column/batch owner
  mismatches: REQUIRE native provenance separates pointer aliases from loop
  counters and the same channel/store order. TRY direct source/destination
  indexing first, then remeasure opening the row loop into its complete batch
  operation (`ImageWriteTGA`). A prior opened-loop regression with named
  pointers does not rule out this boundary after those aliases are removed.
  Keep real byte strides and batch behavior; check remaining scheduling apart
  from recovered register homes.
- Manual offset/progress counters occupy low named webs where retail uses
  high induction webs: REQUIRE each value is a bounded index times a fixed
  stride, including early exits. TRY products at their uses; confirm the
  derived vregs first appear in `presched`, not `initial`. Check interacting
  counters together and keep each nested retry's real owner local
  (`save_to_memcard_w_error`). Preserve byte strides and publication order.
- Walking pointer, buffer, other input colored wrong order (retail: walker
  lowest callee-saved) `[da]`: REQUIRE retail also keeps index (bound, stored
  or passed position). TRY `buffer[index]`, no cursor local, before any
  coloring search. MWCC strength-reduces index into pointer created after
  params; explicit cursor local is created before them, swaps webs (`mflGetS`:
  3 tries after 60 manual + 185k permuter on cursor shape). Fixed-width char
  padding variant: H21. Struct-array variant: retail `add base, offset` with
  field displacements folded into loads and `offset += stride` = indexed
  `arr[i].field` (`g_game_info.players[player]`, `p_controller_config`), not
  a hand `offset` local over a padded view. The indexed form can score lower
  in isolation: its webs need their own decl order, so sweep decl order after
  switching (host sweep, 8! orders in minutes), never judge it alone.
- Prologue scheduling differs, body exact `[da]`: bound/end pointer written in
  byte math retail doesn't need. TRY loop's element units (`end = out +
  (width >> 1)`, not `(u32*)(row + (width << 1 & ~3))`) (`vdisp_copy_frame`).
- No dummy one-trip loop.
- BE strength reduction: IV candidates = `mulli`, `slwi` (sh <= 15), indexed
  `lXzx`/`stXx` with biv operand; <= 4 step-1 IVs per biv; equal steps share
  one IV. IV vregs number above `last_temporary_register` (capture, tier 4
  Hard stops): no source web kind numbers above them. Retail web pops after
  them -> degree levers only, never kind/decl order (`scan_switches`).
- Retail `addi rB,h,i*size` once + member at big displacement (`lwz
  0x1318(rB)`), ours folds array offset into index (`(h+0x1308)+i*0x74`):
  a named `&h->arr[i]` element view can fold the embedded-array offset into
  the index. REQUIRE the canonical owner, actual field offsets and the full
  load/store phase. TRY direct `h->arr[i].member` expressions across that
  phase before declaring a missing padded view (`save_konq_memory_to_krd_
  buffer`: NPC, PUI and event stores; `update_player_collision_nodes`).
  If canonical direct indexing still cannot recover the retail owner, an
  original macro/view type is missing evidence (`sfmps_DecodeOneUnit`,
  `sfmps_ProcPrep`; RE4 used a cast macro); do not invent a padded view.
  FE expression shape, not BE SR.

## H10

Switch. REQUIRE full finite case/default/fallthrough set + text order.

- m2c can omit labels sharing an arm even >99%. Verify finite bounds before
  accepting decompiled default.
- Compare-tree split one past adjacent error-code cluster: REQUIRE callee/API
  defs + identical destinations. TRY explicit shared cases, not rewritten
  tree (`mem_card_read`: CARD_RESULT_NOPERM, CARD_RESULT_LIMIT).
- Compared value can be a singleton default gap, not a source case: REQUIRE
  the complete range tree and API-supported neighboring values. TRY the real
  case set (`gc_native_display_init`: NTSC 0/default and MPAL 2 make PAL 1 the
  root gap). A same-label subtree can leave `bge next; b fallback` after its
  unused comparison is removed. Verify native lowering before attributing the
  pair to an IRO copy barrier. See [TV dispatch](reports/display-tv-default-gap.md).
- Recover case order, shared tails, nested switches, where each default is
  emitted.
- Report-exact switch with rotated named targets: REQUIRE each raw table
  entry and its final callee/data target. TRY retail arm order only after
  proving the case-to-operation mapping; equal instruction bytes can hide
  reordered arms without a semantic bug (`bgnd_start_preload_sobj_morph`).
- Sparse action dispatch with a shared result: REQUIRE retail result def/use
  and all case targets. TRY one result initialized before the switch, changed
  by the default, and returned after the join (`ImageList::HandleAction`).
- Small ordered hardware-ID sequence can be `if`/`else if` chain
  (`InitMetroTRKCommTable`); two adjacent priority cases can need real
  `switch` (`ARQPostRequest`).
- A materialized Boolean predicate lacks a retail branch-around-jump:
  REQUIRE the original comparison, result owner and all inline consumers.
  Check the actual selector lowering before retaining a singleton switch
  (`is_char_locked`, partial): selecting Boolean one can add a separate
  comparison and over-expand every consumer while fuzzy scores stay equal.
  A complementary case-zero form can add further lowering. Measure ELF
  sizes and result/join instructions; a shared branch symptom alone does not
  establish that the helper's original API was a switch.
- Per-arm address formation: keep real index; table base formed before
  dispatch = one owner.
- Explicitly compared no-op cases: REQUIRE independently proven `case N: break;`.
  A verified sibling retail dispatcher can supply a missing exit case even
  when the local compare tree folds it into default. Check the complete caller,
  callee and callback identities plus original bytes, then recover only that
  observed case (`mks_start_fatality_iceball`: PS2 explicitly tests mode 2).
  A permuter zero or orphan branch alone is insufficient evidence; see
  [the iceball case-set report](reports/iceball-switch-retail-case-set.md).
- Idle states in dense table: one case per entry; one state local across
  pre-server check (`sfply_ExecOne`).
- Compare tree ending in unreachable extra `b join`: one empty label just past
  tested range sharing other empty cases' `break` (`go_into_twitch_death`,
  `go_into_major_pain`).
- Orphan `b` after a subtree, or `cmpwi N; beq` root on a value no case names:
  REQUIRE retail compare tree + API/sibling/counterpart support per value.
  TRY the case SET the Switch.c range tree implies: one-value default gap =
  single-value root; two same-label ranges = orphan `b`. Never a bare empty
  case/default (`save_to_memcard2`: CARD BROKEN -6, not IOERROR -5;
  [range-tree report](reports/switch-case-range-tree-orphans.md)).
- Same dead `b default`, plus loaded selector in r4/r6/r7 where ours uses r0:
  REQUIRE enum-backed switch, sentinel shares `default`. TRY enum `*_FORCE_32BIT =
  0x7FFFFFFF` label before `default`; constant too big for `cmpwi` keeps
  selector off r0. No invented small label (`case 6`). Check sibling ports
  (PS2 MW MIPS compares 0x7FFFFFFF) (`mwMemHeapGetMaxFreeBlock`,
  `privInitSystemHeap`, `mwMemHeapStrategyCallback`).
  Masking may exclude the sentinel value; retain it only with vendor enum
  and identical case-set evidence (`privGetAlignFromMwMemFlags`). Preserve
  the proven caller ABI rather than changing it merely to name the enum.
- Missing-ID guard + nullable attachment arm match except unreachable
  shared-exit branch: REQUIRE real zero sentinel, kept lookup/null order. TRY
  two-arm `switch`, attachment in `default`, direct `case 0` failure return
  (`replace_sobj_texture_with_named_wiff`); no extra cases, no dead return.
- One-bit visibility dispatch has `beq` into the complete visible operation
  and a separate branch to the hidden exit, while an early-return guard or
  positive `if` loses that exit before allocation: REQUIRE the proven bitfield
  and unchanged operation/effects. TRY a singleton `switch` with `case 0`
  owning the complete visible operation; the implicit default renders nothing
  (`ScreenText::Render`). No extra cases, empty arms, or artificial state.
- Jump-table layout: edit raising `.text` but lowering `.data` moved case
  addresses; reject (`mk_chess_set_game_mode`).
  Separate equal-valued arms can require different exits: REQUIRE distinct
  retail store blocks; TRY an early return for the shared arm and a switch
  `break` for the separate arm. Recheck every table relocation; a common
  publication after the switch can merge the arms again.
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
- Table vs tree (2.7 probes): count ranges (leading/trailing default gaps
  count, same-label neighbours merge). Jump table iff `ranges-1 >= 8` and
  `2*(ranges-1) >= span/2 + 4` (0..5 tree, 0..6 table; `0..9,45` table,
  `0..9,46` tree). Tree compares signed `cmpwi` even for unsigned selectors.
  Table starts at 0 (no `addi`) when first case is 1 or 2. Source case order
  irrelevant.

## H11

Joins. REQUIRE real branch destinations + effect ownership.

- Existing-object early return forwards an inner lookup result to ABI r3,
  while retail shares the factory/null result in a saved register: REQUIRE
  native capture proves those distinct result owners. TRY one inline owning
  the complete existing-or-create operation, with the public API returning
  its result (`pfx_bind_to_new_obj`). The enclosing inline result can join all
  paths and absorb the inner lookup result without a goto or extra condition.
  Preserve full binding guards and callback order; smaller lookup helpers or
  spelling a common exit alone need not recover that owner.
- Initialization success is stored and returned, but ours duplicates the success
  constant: REQUIRE identical failure effects and a native result-copy trace.
  TRY explicit failure/success assignments to one real result, publish only in
  the success arm, then return at the common join (`hashtable_dynamic_init`).
  A complete allocation/initialization inline owner can preserve that result
  through IRO so its store and return coalesce (H21); opening the same body in
  the public function can lose it. Reject extra success guards or state reloads.
- Retail guard keeps a conditional-to-body plus unconditional skip, but ours
  has one inverted skip: REQUIRE initial pcode and a live IRO rewrite capture.
  GC/2.7 `IRO_RemoveRedundantJumps` inverts an If/IfNot around the following
  Goto when its original target immediately follows that Goto; the Goto
  becomes Nop. TRY recover meaningful source/inline provenance that survives
  this pass. Empty-arm guard
  spellings canonicalize before allocation; do not sweep declarations or
  retain empty arms to recover this reachable branch pair.
- A complete scalar-result query has that guard plus real calculations and
  scratch outputs: REQUIRE identical predicate, FP association and store order.
  TRY one typed inline owning the query, its early failure return and final
  containment result; assign that result in the caller
  (`keep_camera_out_of_danger_zones`). A result-producing inline boundary can
  preserve the pair through IRO and coalesce later. Literal-zero and named-zero
  early returns can both work; do not infer the zero spelling is causal.
  Pointer-output versions can add assignments; reject extra checks or dummy
  copies, and verify every consumer and emitted function in the unit.
- An early sentinel return needs that pair: REQUIRE an existing equivalent
  playback helper, no retail call, and a native IRO result copy between test
  and jump. TRY reuse the complete helper at the actual inline boundary
  (`pan_vol_pitch_random_hit`, `play_random_sound`). Its return copy can survive
  IRO and coalesce later. Check every function crossed by a moved existing
  `dont_inline` boundary; preserve clamps, random draws and output ownership.
- Ordered scalar clamp loses that pair: REQUIRE the existing helper's exact
  lower/upper predicates, including unordered FP behavior. TRY passing the
  actual unclamped expression directly to that helper and using its return
  (`get_play_camera_position`, partial, `camera_clamp`). Assigning the expression to
  the result local first can erase the inline result-copy boundary. Verify
  native IRO and both bound branches; no redundant raw/result aliases.
- Identify target instruction: branch to final store != branch past it.
  Trace failure branches before calling it scheduling (Krypt coffin effects:
  one failure skips all later objects).
- Nullable owner, result init before guard: null-init result, assigned from
  validated accessor inside guard, one return.
- Null guard before a bounded wait lacks retail's conditional plus shared-exit
  branch: REQUIRE identical wait effects and an existing mutable counter.
  TRY an inline owning the complete wait with an early null return. If a
  by-value timeout moves the early LI and makes the counter FE-owned, pass
  the caller's existing counter by reference (`drone_change_to_style`). Verify
  native counter ownership, initialization order and no escaping address or
  extra stack/load/store operations; do not invent a counter or output.
- Nullable-owner ternary becomes an IRO expression web and outranks real
  snapshots: REQUIRE exact allocation replay needs a snapshot selected before
  that owner. TRY explicit nonnull/null assignments to the actual named owner,
  then the replay-supported order of existing declarations
  (`mk_chess_move_cursor_to_next_square_track_line`). Keep the original guard
  and acquisition boundary; no identity helper or redundant alias.
- Two-way owner descriptor is retained before earlier stores in source but
  after selection in retail: REQUIRE identical choices and effect order.
  TRY explicit `if`/`else` assignments at selection instead of default init
  plus overwrite (`p_ground_target_collide`); no redundant result alias.
- First verify retail tests owner at all; drop unsupported fallback.
- Extra early-failure assign: positive gate sharing final return. Real state
  still lowers different -> one-case switch with existing default.
- A mode-selection failure island precedes the main operation but all errors
  join one caller-owned store: REQUIRE identical calls, input-mode domain and
  error destinations. TRY an explicit inline owning the complete operation
  and returning its error (`gcnHostOpenCommand::handleNotStartedState`). Keep
  the command's null-owner guard, error publication and completion return in
  the caller. Open-coded early returns can duplicate the store; wrapping the
  entire operation in a positive gate can move the failure island to the end.
- One-state callback returns a constant after its effects, default delegates:
  REQUIRE retail case branch to trailing constant and default branch past it.
  TRY case `break`, explicit default delegated `return`, then terminal constant
  return (`gcnHostCloseCommand::serviceState`, also flush). An if/else may
  recover the exit but lose the entry `beq`/`b` pair; no dummy arm or result.
- Final int Booleans: `!= 0` vs `== 1` differ even for 0/1 values. Test arm
  order separately; two-value select uses real conditional, zero-first vs
  nonzero-first picks different carry code.
- Ordered float minimum emits extra predicate extraction in a Boolean switch:
  REQUIRE the full computed-value phase and its rounding/root-selection order
  have repeated retail evidence. TRY one scalar-return inline owning that
  calculation and ending with `if (value >= minimum) return value; return minimum;`
  (`mk_chess_launch_n_land_ani_with_xz`). Tiny clamp-return helpers can be neutral;
  pointer-output clamps can add a stack slot. Preserve unordered behavior and
  every FP association; no empty true arm or identity assignment.
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
  Nested MkPtr membership search: let the complete inline own its storage guard,
  pretested walk, stale-node cleanup and found/exhausted returns
  (`is_point_inside_shadow_exclusion_zone`). A caller-owned `for (;;)` with an
  assigned result can add a null diamond; preserve next snapshots and ordered
  shape predicates, then check real inline-local rank if only FP homes remain.
- Negation scope: `!(A && B && C) && D` != `!((A && B && C) && D)`.
- Temp board simulation: keep every restore + final rebuild, incl zero-trip.
- Genuine cleanup edges: M08.
- Failure clears retained owner then branches to shared final return
  (`get_mkobj_frame`): REQUIRE that exact destination + measured structured
  alternatives. AGENTS.md forward `goto` only if those regress and shared exit
  avoids duplicated init or fake flag. Retail jumps from inside wait loop to
  shared end-of-iteration block `[da]`: three failed structured forms, then
  one local `goto` (mk-da `gop_decode`). Record measurements in task report.
  Sibling-null exhaustion before unconditional parent/link clears has the
  same rule (`_bone_make_parents_my_children`, partial): require measured
  break, flag and helper alternatives, then keep one forward cleanup edge.
  See [the bone cleanup report](reports/bone-unlink-shared-cleanup.md).
- Search returning from inside loop: retail dead `b exit` after descend arm
  + `beq X; b test` polarity + `li r,0` on normal exit. REQUIRE TU allows
  explicit inlining (`-inline noauto`), retail keeps the enclosing static
  call. TRY typed `static inline` search helper returning from the loop,
  arms `if (child != 0) cur = child; else return cur;`, `return 0;` after
  loop; caller `heap = helper(block);`. Result temp coalesces, orphaned
  branch-only block stays (final branch pass never deletes unreachable
  blocks); named result local leaves `mr`. `if (child == 0) return cur;`
  leaves rows (`privFreeMemFromUsed`). TU `-inline off` without evidence:
  compile current source under every inline mode; byte-identical = flag
  unsupported -> object flag trial landed with the helper source. `auto`
  rejected when it inlines the static function into its caller.
- Join zero: BE value numbering scope = extended basic block (block + single-
  pred successors); join block starts fresh; named-local `li` never CSE'd.
  REQUIRE consumer of the zero not constant-foldable: one reaching def folds
  the compare. Retail unfolded `li 0; cmpwi` + single join `li` reached by all
  paths = def-count/CFG problem; 2.7 doesn't sink identical `li` into a join,
  no named/temp shape fixes it (`ADXB_DecodeHeaderAdx`).

## H12

POD copies: aggregate assign for word/CTR copies, components for
`lfs`/`stfs`. Three interleaved word copies in a particle stream can be one
`Vec` assign. Keep API byte strides + copy order; then test decl order. Event
buffer extent from consumer copy + producer stores. No invented init.
Copy reorders loads vs stores, each loaded word used `[da]`: name every
loaded word in a local so all loads precede stores (`IntraAotBlock`
`temp0..temp3`).
- Pointer/instance publication loads both adjacent words before either store:
  REQUIRE a real latch with verified size/offsets and all producers/consumers
  accounted for. TRY a shared typed latch and aggregate assignment
  (`drone_lip_synch`); scalar staging can reverse loads and owner registers.
  If the storage also holds another payload, require a real mode selecting
  its different meaning before using a union of complete records. No
  cast-punned pair views. Recheck every header includer and relocation mode.
- Returned record retains a homogeneous payload on the stack, but scalar
  fields remove its stores: REQUIRE contiguous equal-width payload words and
  dispatch/producer evidence for their count. TRY the real payload array and
  distinguish construction from assignment (`circular_buffer<Callback>::front`).
  Keep direct memberwise assignment when array assignment changes existing
  queue/vector consumers; verify every client, self-assignment, layout and ABI.
  Do not add local copies or stores solely to reproduce the stack traffic.

## H13

Intrusive lists: exact typed reciprocal-store order; save next before
mutating callback; reload links as retail. Repeated stale-node removal
differing only in saved-successor allocation: typed inline helper returning
successor, keep save-next, clear-header, destroy order.

- Independent list phases share a saved-successor local and its split webs
  rotate count/snapshot homes: REQUIRE exact joint allocation replay and the
  same callback boundaries. TRY distinct genuine successors for each list
  (`save_konq_memory_to_krd_buffer`), then one replay-predicted declaration
  order change for the remaining count/cursor pair. Keep original scopes:
  moving the cursor into its branch can change other saved-register claims.

## H14

Stack slots + store order. REQUIRE real address-taken locals, offsets,
lifetimes.

- A named object is created before an ABI copy but allocated after it:
  REQUIRE actual saved-record and restored-Object bindings. GC2.7 deferred
  reconstruction can append locals in serialized-list order, bypassing parser
  creation hooks (`mwFileMountTable::getMountPointFromName`, partial). TRY
  recovering the original semantic storage/inline owner; declaration timing
  alone does not change that order. Keep unknown creators unknown and reject
  padding or synthetic copies. See the [native ownership evidence](reports/gc27-deferred-object-stack-order.md).

- Factory outputs lie below by-value process flags while retail places them
  above: REQUIRE native slot allocation identifies real flag objects and ABI
  copies, with unchanged sizes and frame. TRY sharing the complete process
  startup operation so its flag-creating helpers nest below the top-level
  output-producing helpers (`bleed_restart`). Check both startup consumers;
  do not model compiler copies as arrays or add padding. H30 explains nesting.
- Independent grid-origin conversions swap scratch slots and invariant spacing
  FPRs: REQUIRE the same clamped indices and fused multiply/add trees. TRY one
  typed operation owning both coordinate outputs
  (`set_koin_positions_and_colors`). Reordering separate scalar helpers can
  fix slots while rotating constants. Keep actual outputs in the caller and
  verify every existing consumer. If a row-base multiply still crosses a
  depth calculation, name that real row base at the observed boundary; use
  full native replay for its declaration rank (H15), without a manual
  induction counter, duplicate conversion or padding.
- A late factory output word swaps slots with an earlier inverse-sqrt input:
  REQUIRE both real lifetimes, unchanged factory checks/publication and native
  frame evidence. TRY letting a complete launch operation own its process and
  output local (`p_plyr_sidekick_switch`, partial); keep the earlier math helper
  unchanged. Verify actual slots and every consumer, without zero-init, padding
  or an artificial block. A correct frame does not establish owner coloring.
- Mutex pointer remains in a GPR, retail stores it in a stack owner across
  lock, protected call and unlock: REQUIRE an existing guard with the same
  constructor/destructor calls and lifetime. TRY that real RAII guard around
  the complete operation (`mwFileMultithreadedMemTraits::allocate`); check
  every return and sibling. Do not create an address escape or dummy slot.
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
  For an existing local pointer zero, measure declaration initialization
  versus its standalone assignment (`do_dialog`, partial); preserve the zero
  before all effects and do not retain unrelated declaration changes.
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
- Packed font words precede caller POD return slots, retail follows them:
  REQUIRE distinct real words and repeated byte-publication / text-update
  operations. TRY separate inlines owning the aligned packed-color word and
  the later snapshot/update/restore word; keep GetColor PODs and latches in
  caller (`mkScreenEngineClient::HandleAction`). Extracting the whole action
  also moves the return temporary and can lose the intended order.
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
- Non-addressed slots (2.7): size buckets <=1, <=2, <=4 ... <=512, reverse
  decl order inside a bucket, ascending offsets; then codegen temps; then
  spill slots (ascending vreg, tier 4 Hard stops). Retype before reordering.

- Initialized aggregate whose scope hoist moves the copy: IF hoisting the
  declaration to function scope changes copy timing, REQUIRE same real
  object/initializer + the retail copy. TRY flat declarations in stack-owner
  order plus `object = (Type){...};` at the original initialization
  statement (MWCC supports the compound literal). No fake scope or static
  (`r_combo_broken_part2`: late `bone_offset = (Vec){0,0,0};`).

## H15

- Reused counters/cursors create split ranges in independent display or history
  panes: REQUIRE actual disjoint source domains and full replay mapping each
  range to its retail role. TRY separate named counters, vertical cursors and
  process results for those real operations, retaining every initialization
  and effect position (`show_sw_log`, partial). This changes source ownership;
  reordering only the old split registers or wrapping all panes in a helper
  can miss it. See [the pane-owner audit](reports/switch-log-pane-ownership.md).

- A value reuses the wrong already-obtained saved register: REQUIRE exact
  full replay and the unchanged first claimant of that current color, even
  when it is not a neighbor of the differing value. TRY including first
  claimants of both current and desired colors in the ordering group. A real
  local used only inside an existing loop may belong in that loop, followed
  by the replay-predicted same-scope declaration pairs
  (`mkobj_update_weapon_trail`: cache-copy child matrix). Keep parameters
  first and all initializations/uses fixed; verify every selected color.
  Obtained saved registers remain available after their first value dies.
  A truncated search that omits the unwanted color's first claimant cannot
  prove an interference difference.
  If the unchanged claimant belongs to a complete operation and the
  measured solution crosses named/inline creation kinds, TRY letting that
  real operation own its locals while preserving every reload and effect
  (`mk_chess_place_special_cell_at`, partial visual setup recovery). This is
  a source-boundary hypothesis, not a declaration permutation; retrace every
  later owner. See the [visual setup case](reports/chess-special-cell-visual-ownership.md).
  A later coordinate can also control earlier setup homes. The complete
  search/guard/position operation may be required: a positioning-only helper
  leaves nested search owners in the wrong creation order. REQUIRE the neutral
  enclosing phase's fresh full replay before its predicted same-scope pair
  (`mk_chess_reset_cursors`: byte x/y declarations). See the
  [cursor reset case](reports/chess-cursor-reset-phase-ownership.md).
  If that claimant is a narrowed random result folded out of a wider shared
  variable, REQUIRE the retail narrowing and a distinct real loop use. TRY
  giving that value its own correctly narrow loop-local owner
  (`bl_init_beetle_pebbles_first_floor`, partial). Retrace every range:
  changing a late claimant can repair earlier disjoint values while leaving
  other duration/element pairs unresolved. See the
  [beetle timer case](reports/beetle-first-floor-timer-ownership.md).

- A differing-only pop order works in isolation but fails full simplify replay:
  REQUIRE exact baseline replay and unchanged real locals adjacent to the
  differing owners whose removal changes the simplify sweep. TRY the predicted
  declaration order for that complete local group, keeping every initialization
  and use fixed (`native2d_instance_geometry`: texture dimensions alongside
  extents and reciprocals). Include those unchanged owners in the bounded graph
  search; they need not lie between the differing vreg numbers. A failed
  four-owner permutation does not rule out the supported six-owner order.
  Never extend scopes, create copies, or permute source without a full-replay
  solution that preserves all other colors.
  An unchanged short-lived local can lower a later owner's degree below the
  simplify threshold in the same sweep: scanning that owner first delays its
  admission (`gxQuatInterpQuat`: dot/sign declarations on the complete trig
  helper source). A fixed parameter can then acquire the desired color without
  any parameter or lifetime change.
  In a recursive inline scan, a counter may likewise enter the first sweep
  only because its bone-pointer neighbor was scanned first. REQUIRE full
  replay predicts the exact pair, including every other expansion. TRY
  declaring the existing count before the bone pointer
  (`_move_bones_from_obj_to_limbobj`): both count and read index can wait at
  the degree threshold until sweep two and then select before scratch values.
  Check the standalone scanner as well. See the
  [recursive BFS case](reports/bone-bfs-simplify-sweep.md).
  Even an exact selected-pop permutation does not prove a kind change when
  it crosses named/FE/codegen/BE bands. Search full same-band scan orders
  first: an earlier scan position can delay admission until the next sweep.
  A graph solution still needs a valid source edit and whole-unit measurement.
  Coarse FE bands contain distinct creation families. REQUIRE the actual
  inliner, loop-invariant, split and CSE origins before treating a same-band
  order as a declaration change; flag crossings between those families.
  An exact captured replay with stalls can still support a bounded scan test:
  keep native costs and the spill-temp boundary fixed, and reject every order
  whose natural stall-owner sequence changes (`p_lightning_controller`).
  Preserve every other selected color; do not force spill choices or infer
  source feasibility from graph SAT.
  A later-round solution involving only named locals can justify one coupled
  declaration-order trial. REQUIRE the real compilation to regenerate earlier
  spills and confirm every sibling; a final graph cannot predict that rebuild
  (`mk_chess_calc_all_attackers_in_turn`). See the
  [chess attacker case](reports/chess-attacker-stalled-allocation.md).
  Coarse FE membership is not a declaration family. REQUIRE exact native
  `helper.local` origins, original lexical scopes and all repeated expansion
  bindings before translating an order into declarations. Couple every copy,
  including coalesced slots, and replay the complete target and collateral map
  (`_move_bones_from_obj_to_limbobj`: both bone/count expansion pairs). CSE,
  loop-invariant and inline-argument owners cannot be swapped as declarations.
  See the [frozen-sweep audit](reports/held-gpr-sweep-source-coupling.md).

- Rounded values and their rescaled products share owners across calls:
  REQUIRE native replay shows split rounded owners, retail short product
  owners, and separate product assignments propagate into later aliased
  stores. TRY a complete displacement-and-translation inline operation
  taking rounded scalar counts and performing compound rescaling before
  the ordered component stores (`do_wrapbox`). Keep fractional distances
  and rounded counts as distinct real locals; retrace their inline-local
  ranks before applying the predicted declaration pair swap. Passing
  already multiplied arguments can let IroPropagate sink the products;
  moving an entire declaration group can rotate correct owners. Preserve
  every rounding boundary, call, input reload and store; no extra uses.

- Quaternion components/products have a measured declaration-order solution
  except for a shared-square CSE versus codegen owner: REQUIRE exact native
  replay and the same rounded products/store sequence. TRY explicit genuine
  twice-used squares, then one predicted product/component declaration group
  (`quat_to_mat`). A named `yy` plus that group repairs homes; naming `xx` at
  its original first use can occupy its old CSE rank and preserve `xx + yy`
  operand order. The individual edits may regress. Do not commute source
  operands, add scratch aliases, or alter rounding to close the last row.

- Aggregate-copy `ADDI; LWZ` becomes `LWZU` only in final output: REQUIRE
  presched still contains the ADDI and native replay shows literal base and
  adjusted address sharing a register. TRY the observed initialization order
  before independent address calculations
  (`p_mk_chess_piece_constrain_to_cell`). Correct address ownership can prevent
  the later update-load fold. Preserve the real aggregate and its stores;
  do not add a base alias or disable peephole optimization. Check independent
  coordinate evaluation order separately from named result declaration order.

- Caller load/configuration split and declaration rank can be coupled:
  REQUIRE exact native replay predicts an inline result must become a named
  owner below existing coordinate locals. TRY keeping the allocation result
  in the caller, passing it to the complete guarded configuration operation,
  and placing that real result declaration at the predicted rank
  (`p_konquest_map_screen`, partial). The split alone can rotate otherwise
  correct persistent owners; helper nesting cannot reorder a codegen return
  against an inline local. Preserve every null check, store and call.
- A trivial nested accessor gives a real inline operation's local a later
  return-temp owner: REQUIRE native creation/claim traces predict the needed
  owner order, and the accessor is exactly a supported member read. TRY that
  member access inside the existing complete operation, keeping its typed local
  and all reads/calls unchanged (`ShadowCameraSetLight`: `init_shadow_system`
  and `UpdateShadow`). This can move the first saved-register claim and repair
  later disjoint owners too. Check every caller; do not change shared accessor
  headers, invent dependencies, or generalize to nontrivial/validating getters.
- Cursor, coordinate and cell owners need different web kinds: REQUIRE exact
  replay, a stable root used by both selections, and a real callback boundary.
  TRY opening the complete positioning operation in its caller branch, keeping
  the root as a read-only pointer binding and repeating the stable coordinate
  at its existing read points (`p_mk_chess_cast_spell`). Reuse the canonical
  live-object getter; a named input/output latch can add a forwarding copy.
  Preserve the same caster variable and its post-callback reload so its split
  identity remains intact. Const applies to the root binding, not the pointee;
  do not add uses or retain the root across mutating callbacks.
- Caller element aliases become named/split webs although a complete inline
  operation owns their uses: REQUIRE identical indexed selections, read/call
  order and native ownership evidence. TRY passing the actual element addresses
  directly to that operation (`bl_init_beetle_pebbles_first_floor`, partial).
  Measure every consumer and retrace before pursuing old constraints: removing
  those aliases can eliminate an apparent need to reverse unrelated codegen
  constant-base emission. Do not move aggregate initialization across calls
  based on a constraint from the superseded graph.
  Inside a guarded roster scan, the stable indexed element can likewise own
  the comparisons and an existing inline adjacency predicate directly
  (`mk_chess_drone_handle_defend_the_king_strategy`). REQUIRE the root/index
  to remain unchanged throughout that condition; removing a convenience piece
  pointer can change coordinate CSE and loop-induction interference. Preserve
  the later publication's actual roster read and every post-callback reload.
  The same alias removal can make a previously mixed use of an existing
  distance-output API non-regressing (`mk_chess_drone_exchange_ai_casting`,
  partial). Recheck first-use axis order only after the input-owner fix;
  direct API arguments can still destroy correct result/split owners. See
  [the exchange roster report](reports/chess-exchange-roster-ownership.md).
- Same-compiler vendor reference available: REQUIRE a clean complete donor body,
  equivalent retail operations, ABI and layouts, and no register/asm pins or dead
  guards (inspect its inline helpers too). TRY the complete declaration and
  statement shape mapped to project types before isolated local reorders.
  Separate signed field remainder, early rounded-rate lookup and subsequent
  hour/minute/second loads act together in `sfmpv_Concat`; a named-rate edit
  alone regresses. Preserve measured shared exits under the local-goto policy,
  then check all functions, data and relocation modes; donor match claims do
  not establish this build's exactness.

Coloring, identical ops. Association + staging first; then max one honest
lifetime/decl insight; then stop (tier 4). No register carousel, no invented
uses. Checklist `[da]`: grouping -> one decl/scope change -> H26 / real
cached local -> tier 4. Copy of in-scope value, reuse of unrelated variable,
branch-only alias = dishonest even at 100.

- Decl order + scope `[da]` (mk-da's top rule): lifetimes look shifted ->
  one honest move of where a real local is declared (outer vs block, before vs
  after another local's first use). GC/1.2.5 colors in decl order.
- Exact named-web swap under GC/2.7: REQUIRE native simplify/select replay
  predicts the two-owner swap while intervening named webs stay fixed. TRY
  placing the existing locals in one scope in that precise reverse declaration
  order; preserve every initialization boundary. A lone block-to-function
  hoist can shift intervening owners and miss the predicted permutation.
  Retrace the accepted source (`npc_play_two_player_one_shot_anims`). No
  aliases, new uses, or artificial blocks.
  Under C90, declare all locals first and assign in the original evaluation
  order; keep the intervening owner fixed when exchanging two declarations
  (`mk_chess_drone_cursor_movement_to_target`).
  A branch-local object and outer loop index can require that same exact pair
  of function-scope ranks, with the advancing row pointer between them
  (`load_tile_objects`); a one-sided hoist shifts the row pointer too.
  A shared address CSE can follow the swapped named owner; include its
  predicted color too (`start_gore2_update`).
  The same precise pair applies to FPR locals: exchanging the real direction
  component and scaled coefficient while keeping the intervening negated
  component fixed closes `intersect_xz_lines`. Preserve distinct geometric
  quantities and every rounded product/FMA; broad grouping or failed random
  search does not refute a full-replay prediction for that pair.
- Disjoint later owners reuse the wrong saved home: REQUIRE exact full replay
  identifies an earlier real owner that claims the needed home only after those
  selections. TRY moving the existing declarations together into their enclosing
  function scope in the predicted order, retaining initialization boundaries
  (`atm_bio_screen`: text object and height before frame/X selection). Moving
  only the claimant can change its companion's home; include correct-colored
  owners in the replay. No extra use, alias, or lifetime extension.
  A later switch case's offset can be that first claimant
  (`__pfxvm_execute_spawn`). Hoist the actual case-local declaration to the
  enclosing loop and exchange its precise rank with the initial offset,
  keeping intervening locals fixed. Retain separate phase offsets and all
  calculations at their original cases; split initialized declarations into
  declarations and assignments when C90 requires it.
- Separate traversal indices in mutually exclusive branches can select before
  another branch claims their required saved home: REQUIRE the complete replay
  and claim timeline predict a declaration-group move with no other color
  changes. TRY moving the existing position cursor and distinct indices
  together into function scope in the predicted order (`do_cloth_colls`).
  Keep each loop's initialization and index identity; do not merge iterators
  or infer a missing edge merely because an isolated pop-order solve fails.
- Nullable ternary pointer selected before a retained snapshot: REQUIRE native
  replay identifies a ternary IRO temp versus a named snapshot. TRY structured
  `if/else` assignment to the actual pointer local, then order that declaration
  against other real locals in the same existing block. Changing the owner kind
  alone can rotate a different pair (`mwFileGCIOMixIn::queryErrorState`). Preserve
  the lookup's null guard and snapshot boundary; verify every instantiation.
- Saved home lands too low and a local pop-order search finds no solution:
  REQUIRE full captured-color replay and a claim timeline showing earlier
  split-variable owners claim lower saved registers before the target selects.
  Inspect claim timing before inferring missing interference: saved registers
  become available on demand, and selection reuses the lowest free claimed
  home. If duplicated arms share locals whose later webs become IRO split
  temps above named locals, TRY distinct real per-arm owners and the measured
  declaration order (`obj_sever_limb`). Preserve initialization and use
  boundaries; no dummy aliases or lifetime extensions. Reconstructing all
  colors does not independently prove pop order; retrace the accepted source.
- Derived index reuses its dying input but retail keeps a distinct home:
  REQUIRE exact allocator replay rules out order alone and identifies the
  missing input/index interference. TRY computing the independent index
  before the input's last real use, preserving subsequent pointer lookups and
  memory effects, then place existing declarations in the proven allocation
  order (`pzpfx_copy_playpieces`). Adding the edge or moving one declaration
  alone can rotate other owners; validate the combined prediction and retrace.
  Never add a dummy use to manufacture the edge.
- Disjoint template/cursor owners, only template takes a higher saved home:
  REQUIRE allocation trace selects template before the later cursor, whose
  lower home is reusable. TRY declaring the existing cursor at function scope
  before the template owner; keep its initialization and live interval
  unchanged (`mslSoundAttach`). A split loop-index web can still select first;
  verify the actual selection order rather than sweeping declarations.
- Block-local named web vs a split web (2nd loop of a reused index) swap
  saved homes: REQUIRE capture shows the named local must number above the
  `split:<var>` temp. Named locals always number below IRO temps (block
  scope lowest), so no decl order fixes it. TRY dropping the local and
  repeating the expression (`effects[i]->f`): IRO CSE temps number above split
  webs (`start_protect_effect`).
- Named record pointer needs a temp-kind home: REQUIRE an existing early
  member read and later uses of the same stable address, with no intervening
  call or aliasing store changing that address. TRY the first member read
  directly, then bind the pointer for later work: IRO can share the early
  address instead of keeping a named pointer web (`rope_controller_init`).
  Retrace remaining owners; one measured declaration order among real locals
  in the same scope may also be needed. Preserve every field-read boundary;
  never add a redundant read or dummy use to create CSE.
- Single-use descriptor pointer must select before a container-field emission
  temp: REQUIRE exact native replay and the same lookup/publication order.
  TRY a direct indexed member read (`_call_script_function`). If field-offset
  distribution duplicates the product of a one-based ID, normalize that real
  ID once at the lookup phase and use the index for both lookups. Preserve
  argument-copy timing and every owner reload after publication; no extra alias.
- Named call inputs select below an indexed-address CSE but retail needs them
  above it: REQUIRE exact native replay and an existing typed inline consumer.
  TRY passing the original fields directly to that inline, preserving measured
  load order; frontend argument ownership differs from caller named locals
  (`npc_play_random_dialog_sequence`). Then recheck genuine enclosing locals
  in the predicted same-scope order. Do not create a dummy helper or alias to
  force ownership; check every existing inline consumer.
- Volatile temps rotated (r0/r3-r12, f0-f13), same ops: REQUIRE replay exact
  and a pop order of only those webs gives retail. Codegen temps number in
  emission order and the higher number pops first, so the residue is
  statement/operand order (H27): statements in retail order
  (`atm_demo_fight`, `summarize_unlocked_items`); an inline function body is
  emitted before the expression using its result, a macro/open-coded
  expression is not, so `min - dot(a, b)` loads `min` after the dot via an
  inline but before it via a macro (`repel_cylinder_and_box`).
- Independent FP deltas have both named-result and source-temp rotations:
  REQUIRE exact replay needs both result and codegen-temp order changes,
  with no intervening effects. TRY reversing the real axis calculations and
  their delta declarations together (`xz_distance_between_players`). Either
  alone can be neutral; scheduling may retain retail load order despite the
  changed emission order. Preserve every load/subtraction once and the FP
  multiply/FMA tree; no duplicate calculation or coordinate-role reuse.
- Lone web wants f1/r3 (or an arg reg), no order fixes it: retail coalesced it
  with the physical return/arg register. TRY real return type/arg (H01): a
  zero stored and returned (`float ... return 0.0f`,
  `pz_fighter_clear_out_external_forces`).
- A branch-only computed magnitude rotates otherwise correct FP constants:
  REQUIRE native replay identifies the late codegen result selecting before
  those constants. TRY naming the actual magnitude inside its existing branch
  (`mk_chess_set_viewing_quadrant`), preserving the operation's result type:
  canonical `__fabs` returns double, so a float local would add rounding.
  Recheck independent vector-load emission order separately (H27), all addressed
  stores, and literal pools; do not move constants across branches or add aliases.
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
- Effect-name load and owner-selection branch differ together: REQUIRE the
  proven player-to-owner mapping, unchanged call ABI and publication order.
  TRY a typed inline owning the complete owner selection, called as the owner
  argument, plus a named string argument assigned immediately before lookup
  (`pzsm_rain_dance`). Measure both shapes together and every sibling; do not
  substitute an unrelated phase value for an equal-valued owner ID.
- Scaled arg: `MEMPRINT(fmt, size_kb *= 1.0f / 1024.0f, name)`
  (`mwMemUserConfigOutofMemoryCallback`); siblings may differ.
- FP temps: numbered per statement (pooled const first), colored newest-first
  into lowest free volatile FPR. Stage negation + copy into result locals in
  earlier statements (`ai_side_clearances`). `.sdata2` float pool: `@N`
  order = offset order = creation order (M15).
- Independent FP numerator/denominator homes differ: REQUIRE the same rounded
  component products, fused expression and later uses. TRY initializing the
  numerator before the denominator (`quat_extract_ang_y`); reverse named-local
  numbering changes both owners and their interfering temps. No new lifetimes.
- Repeated normalization branches, first exact and later split webs reversed:
  REQUIRE capture identifies `split:<component>` owners. TRY independent
  branch components in the matching branch's declaration order, keeping
  assignments fixed; only then recheck a previously rejected scale order
  (`check_additional_trigger_fire_requirements`). Preserve rounded squares,
  fused dot association, and NaN predicates.
  Reused direction variables can also split between normalization and scaling:
  REQUIRE native var-record evidence and replay of both raw and split owners.
  TRY changing only independent first-input read order
  (`npc_check_visibility_and_calc_dist`, partial). Split numbering follows
  first-web discovery; swapping declarations can fix later homes while
  disturbing correct raw inputs. Retrace all owners and emitted scheduling;
  preserve separate multiply stages and unrelated Boolean CFG residue.
  For independent marker publications, TRY distinct real coordinate pairs
  when native evidence identifies later reused coordinates as split webs
  (`SpreadSheet_image::Update`). Keep assignments, rounded phases and callback
  reloads fixed; require full FPR replay without collateral color changes.
- Narrow params: explicit cast or u16 local hoists arg load; implicit
  conversion to `unsigned short` param doesn't (`drone_loop`). Needs callee
  narrow-param evidence; u16 return makes callers re-normalize.
- Two single-op args from one loaded word, retail computes arg 2 before
  arg 1 (`extrwi r4` then `rlwinm r3`): REQUIRE same load, no call between.
  MWCC sinks a single-use `int` local into the call and evaluates args left
  to right, whatever the statement order. TRY `unsigned char`/`unsigned
  short` for the arg-2 local: the narrowing assignment stays at its statement
  (`p_main_menu`). `!!(x & BIT)` also stays; `(x & BIT) != 0`, inline
  expressions, `x &= mask` redefinition, `scheduling off` do not.
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
- Norm CSE and nested inverse-length result swap homes: REQUIRE native locals
  trace shows the norm created during IRO after the inline result. TRY a real
  inverse-length helper with a named squared-length local before the nested
  inverse-square-root call (`mk_chess_set_up_zoom_cam`); a direct-expression
  wrapper can remain neutral. Verify inline creation order and the unchanged
  FP tree. Check independent endpoint order against complete literal-pool
  bytes and every consumer (M15); a permutation may restore retail placement.
- Drop redundant aliases: read mirrored field direct
  (`SFCON_UpdateConcatTime`); write via single-use global owner at final store
  (`GXSetZTexture`); pass sole-use member to existing helper
  (`obj_find_material_by_id`); owner selection at lazy validation boundary;
  pass original member to validator.
- Pool-slot free-list pop: selected node = result, update canonical slot head
  directly. REQUIRE unchanged pool/index before store
  (`hashtable_store_with_instance`); no recompute local.
- Pool release swaps descriptor/base/index volatile homes: REQUIRE the same
  range check, byte-offset alignment test and occupied-slot updates, with a
  named quotient retained across the alignment branch. TRY assigning the
  actual slot index inside that alignment guard, before the occupied test
  (`mk_hwfile_open`). The remainder already needs the division; preserve its
  signedness and verify every shared release consumer. No quotient copy,
  fabricated use or storage accessor. See
  [the native ownership evidence](reports/hwfile-aligned-index-ownership.md).
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
- Zero copy: retail `mr rX,rZ` (rZ holds earlier zero) where ours `li rX,0`,
  or reverse. REQUIRE same CFG, both defs in one extended basic block, no
  call between (zero def before a call not reused after it). TRY change dest
  web kind: named-local def always own `li`; FE temps (inline-helper
  params/locals, split webs, `?:` results) CSE into `mr` from first temp
  holding the value. Move whole block owning the loop (incl. count init when
  retail inits before calls) into typed `static inline` helper, prefer MAP
  UNUSED name. Only IVs derived from the index become copies. Re-measure
  previously rejected latch/helper forms after (`pebble_render_callback`:
  MAP `cull_pebbles`, then `MK_HDR_LIVE` latch closed last rows).

- Declaration grouping in actual initialization order closed
  `pfx_parametric_spawn` (emitter, old_count, particle, capacity, cursor ->
  r26..r30). Explicit `if/else` for a ground value instead of a ternary
  initializer closed `do_bounce` (f3/f2). Named vs inline owner of a
  constant: H29.
- FP temp: IF same ops but a staged common float divisor changes entry FPRs,
  REQUIRE no intervening side effects and same float precision, TRY the
  direct repeated divisor expression (`(ticks * game_speed)` in each use);
  MWCC CSE still shares the product while the expression-temporary numbering
  differs (`obj_scale_over_time`: removing the `scaled_ticks` local closed it).

## H16

- Integer constant update is rematerialized at several consumers: REQUIRE
  native IRO snapshots place the rewrite in UseDef increment propagation,
  with an unreferenced-result compound update and a genuine first consumer.
  TRY consume the update's value directly at that existing call argument
  (`display_dragon_with_text`, partial: `draw(..., y += baseline)`). The
  referenced-result guard can retain the updated owner. Preserve effects,
  argument dependencies and later uses; no extra use, volatile or type trick.
  See [the increment case](reports/gcdisplay-increment-consumption.md).
- Sequential formatting phases mix inline locals with stack-fed call-result
  temporaries: REQUIRE actual copy-elimination evidence, independent lookups
  and verified retail call order. TRY direct nested lookups in both complete
  formatting calls (`gc_no_space_routine`); GC2.7's right-to-left argument
  evaluation can preserve that order while changing real producer ownership.
  Measure both phases together: one can fix itself and recolor the other.
  See [the formatting case](reports/gcmcardmsg-format-expression-owners.md).

Nest single-use result; keep original callback owner at untyped boundary.
`g = f(); use(g);` forwards without reload even under `-opt nocse`
(`gc_aram_init`). No manufactured return contract.

- A named single-use result is coalesced, but a parameter crosses the simplify
  degree threshold: REQUIRE exact native replay counting the retained copy's
  interference edge. TRY feeding the producer directly to its sole consumer
  (`mkScreenEngineClient::LoadScreenSet`: `GetName` into `sprintf`). A coalesced
  local can still affect admission; verify all other owners and effect order.
- Single-use arithmetic argument stays named and swaps with a coefficient:
  REQUIRE native replay identifies the input/coefficient rank and unchanged
  rounding. TRY pass the real expression directly (`v3_v3_to_quat` half-angle
  sqrt inputs). Its product can become an emission temp above the coefficient;
  retrace the actual kind, stack slots and both consumers.
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

- Nested single-use handle: `emitter = fx_next_emitter(fx_by_owner(...))`
  closed `start_hero_transform_effect` (intermediate handle lifetime).

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

- GX FIFO pointer stores lose a retained load across vertex emissions:
  REQUIRE repeated retail position/color/UV writes and an existing SDK writer
  using the named absolute volatile FIFO object. TRY the complete canonical
  writers with explicit inlining enabled (`gc_generic_render`). Preserve MMIO
  width and order; compare all siblings. Unknown-pointer float stores and the
  absolute object can carry different alias facts. Missing byte masks that
  fold later in constant propagation are a separate boundary, not another cast.

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

Retail normalizes a canonical `bool` argument before an `int` callback:
REQUIRE the mangled input ABI, callback type, and configured TU size mode.
TRY explicit `flag ? 1 : 0` at that boundary; direct forwarding can omit
`subic`/`subfe` (`_mwFileTickEx`). Check every sibling; see M01 for mode evidence.

Inverse: retail branches directly from lazy call tests into the consumer,
but an inline predicate adds `li 1/0; cmpwi`. REQUIRE identical call order
and effects; TRY expanding the short-circuit condition only at that consumer,
keeping the helper for other callers (`mcard_msg_crc_failure_rtn`).
If every shared consumer has that direct lazy CFG, REQUIRE literal or otherwise
side-effect-free repeated arguments and whole-TU non-regression. TRY a shared
logical-OR macro used directly as the guard; `if (macro != 0)` can still
materialize a saved Boolean (`mcard_msg_confirm_erase_rtn`). Preserve the
individual consumers' independent versus mutually exclusive action checks.

Ordered-sign diamond: retail `bne ... fneg; b join` = abs macro over the
WHOLE expression, `#define X_ABS(v) ((v) >= 0.0f ? (v) : -(v))` (same shape as
`CAMERA_ABS`; `r_sidehead3_spin`). Retail `cntlzw; srwi 5; cmpwi 1` Boolean =
static predicate returning `cond ? 1 : 0` with caller `== 1`; the front end
keeps the 0/1 so the later compare is not folded (`SFH_IsSfdHeader`). Apply
only where retail shows that sequence: normalized-ternary controls on
`pselect_character_id_valid` regressed its consumers.

Weighted-bit ternaries precede the wrong comparison before allocation:
REQUIRE pre/post-IRO expression trees, not only assembly order. GC2.7
`optimizecomm` places the higher-cost operand right; IRO
`lower_expression_to_statements` visits higher cost first, right on ties,
and extracts conditional values. TRY semantically free mask grouping only
when that predicts the required extraction order. `does_event_a_trump_event_b`
recovers year-first entry this way but remains partial: field lifetimes and
final OR association still need separate evidence. No dummy bit owners.

## H20

MWCC colors scalar-replaced struct members after declared scalars. Retail
keeps inlined bit-reader/cursor state in low regs: rewrite state as declared
scalar locals driven by macros (CRI `BS_*`), order decls, load init values
straight in (`mps_dec`).

## H21

- The same typed traversal owners recur in successive passes alongside a
  stripped matrix helper: REQUIRE unchanged guarded assignments, retail
  operations and MAP evidence for the complete helper. TRY shared same-role
  locals together with that copy-or-compose boundary (`rope_controller_update`,
  partial). Validate the combination against every sibling and raw object
  layout; a combined gain does not prove either edit independently necessary.
  Passing a remaining snapshot through a larger direction/orientation helper
  can compile identically, so an inline-argument spelling alone does not prove
  a different allocation owner. Stop when only snapshot coloring remains.

- Inlined helper owners rotate while operations/extent agree: REQUIRE its
  actual expansion, source locals and original MAP ownership. TRY ordinary
  function-scope declarations before the helper's context pointer, retaining
  guarded initialization/uses (`p_show_text_window`, verified candidate).
  No new alias, use, block or type to force lifetime. Check every consumer and
  any emitted UNUSED outline; changed unscored bodies require review.
  Shared note 9253 preserves the helper-scope evidence.

- A uniform scale and a scalarized coordinate take opposite homes, while a
  broad by-value geometry helper splits persistent caller values: REQUIRE
  native owner replay, the same three rounded multiplies, and unchanged
  caller values across calls. TRY the complete three-axis scale as an in/out
  vector operation, passing the actual scale into it
  (`nb_get_desired_acceleration`). This changes the scalar formal's owner
  without replacing the persistent vector with returned components. Check
  all sibling functions, save sets, stack transport and remaining product
  destinations; do not introduce pointer aliases or a one-expression getter.
  See [the tangent-force case](reports/npc-tangent-force-ownership.md).

- A guarded coordinate predicate nests a scalar metric whose result and X
  delta take opposite volatile homes: REQUIRE the same byte-coordinate reads,
  axis-zero returns, unsigned magnitude and exact native owner replay. TRY
  the existing typed cell-distance helper owning both coordinate reads and
  the complete metric, leaving null guards and the Boolean join in the
  predicate (`mk_chess_drone_sacrifice_ai_casting`). Retrace its real local
  declarations before one measured order change. Verify direct metric users
  as well as predicate users; a previously rejected shared boundary may be
  reconsidered only after its blocking consumers actually change their call
  context ([evidence](reports/chess-neighbor-distance-ownership.md)).

- Guarded inline return copies connect an FE result to a named local:
  REQUIRE native coalescing shows the named endpoint outside the eligible
  FE interval, with the same lazy guards and math. TRY feed a single-use
  conditional result directly to its real consumer. If a repeated projection
  then takes the result's home, let the complete intersection/clamp operation
  own that projection as its parameter (`dist_from_plyr_pos_to_arena_edge`).
  Keep canonical math helpers independently valid; never open-code a dead
  duplicate guard. Verify stack owners/offsets and every shared consumer.
  See [the ray-circle ownership case](reports/constrain-ray-circle-ownership.md).
- Caller-owned absolute values disagree with a later signed-opacity operation,
  while direct expressions disturb literal layout: REQUIRE the same double
  absolute values, lazy sign selection, rounded fade and byte-alpha scaling.
  TRY one complete opacity inline, leaving coordinate and vertex publications
  in the caller (`lensflare_proc2`, partial). Verify semantic producer order
  separately from opcode order; retain vertical-before-horizontal FABS when
  retail shows it. Recheck pools and all consumers. A named scale can still
  fold back to a codegen literal; stop without original owner evidence. See
  [the opacity ownership case](reports/lensflare-opacity-ownership.md).

- An inline geometry operation holds real input snapshots in the wrong saved
  homes: REQUIRE independently bound values, native graph replay, and retail
  reads before the operation's publications. TRY capturing those inputs in
  the caller and passing their values into the existing complete operation
  (`pz_fighter_calculate_start_pos`, partial). Retrace named versus inline
  owners; a compatible coloring or model order alone is not source evidence.
  Preserve every read and publication boundary. See the
  [fighter-height ownership case](reports/pz-fighter-height-ownership.md).

- A nested fixed-point normalizer shifts template loads and conversion stores:
  REQUIRE the exact canonical arithmetic and native preallocation evidence.
  TRY owning its real integer conversion inside the complete interpolation
  operation, retaining the normalized float assignment before position
  construction (`p_interaction_cam`, partial). Consuming the rescaled expression
  directly as an argument can restore the old schedule. Preserve truncation,
  masks, rounding and shared helpers; retrace allocation before pursuing the
  remaining homes. See the [camera ownership case](reports/interaction-camera-fixed-angle-ownership.md).

- Shared optional-array shift: REQUIRE the same backward insertion in two
  APIs and a destination genuinely shared by primary/alpha stores. TRY a
  complete loop macro with the existing caller counter and alpha policy
  (`append_wiff_to_clump_material`), preserving the named counter when an
  inline-local band cannot pop after CSE address owners. Check both callers;
  an index-only helper or unused alpha branch is not sufficient evidence.
  See [the ownership case](reports/image-wiff-shift-ownership.md).

Helper locals and web kind.
- Counter zero sharing and escaped-result publication both differ inside one
  strategy case: REQUIRE native evidence separates named-local coloring from
  missing LI/MR sharing and retail samples inputs before the stores. TRY the
  complete attack phase owning its real buffers, counters and result, then
  nest the existing by-value publication operation (`mk_chess_drone_handle_strategy`).
  Preserve inline storage order from the measured stack slots and full replay;
  a publication-only helper can miss the enclosing success/constant owner.
  See [the strategy case](reports/chess-strategy-inline-ownership.md).
- A real value object and an inlined by-value parameter have the right sizes
  but reversed stack roles: REQUIRE retail field widths, complete initialization
  and native evidence for the actual inline boundary. TRY a complete typed value
  factory, bind its result in a statement, then pass that value to the existing
  by-value consumer (`mwFileServerQueued::addCommand`). Putting the factory
  directly in the consumer's argument can leave it outlined without reaching
  `can_inline`; staging it can expose an accepted decision at the ordinary
  depth and recover both storage roles. Preserve every member value and the
  consumer ABI; reject extra copies, changed constructors, padding and policy
  controls. Check all caller and template consumers, not just the stack operands.
- Resident-read request/context homes swap across an upload inline: REQUIRE
  native replay identifies the request below the inline callback/destination
  owners, with identical calls, publications and failure paths. TRY one complete
  successful-read phase owning allocation/failure cleanup and upload; order its
  real destination, request and callback declarations from captured inline
  enumeration (`mslBankLoadResidentWaveChunkDone`). Check saved-register claim
  order; nesting request and upload separately can rotate the remaining pair.
- Inlined sample-address setup has identical halfword operations but scattered
  scalar owners: REQUIRE the canonical SDK record and proven field widths,
  read order and publication order. TRY constructing the real `AXPBADDR`,
  then publishing its members individually (`SoundBuffer_Playable::iPlay`).
  IRO scalarization can recover the address-half owners without
  changing the stack or access widths. Do not substitute an aggregate copy
  or external setter when retail has individual stores. Trace any remaining
  shared format-zero/null owner separately before tuning declarations. If
  second value numbering forwards format-zero into a nullable lookup and
  the codegen zero claims a saved register too early, REQUIRE exact native
  replay and the real lazy table-to-entry/null operation. TRY that complete
  typed lookup at both channels: its inline null-result local can become the
  lower-numbered coalescing survivor while preserving constant sharing. A
  primary-only helper can shift the wrong claim instead. No dummy zero,
  extra lookup, false null guard or separate pointer with an invented lifetime.
- Runtime-stride free-chain keeps an extra typed-node/byte-cursor copy:
  REQUIRE retail owner-before-link stores, a proven payload start/extent and
  one cursor walk. TRY passing that actual payload into the complete block
  initializer, then computing each byte successor and using one complete
  typed owner/link constructor before advancing (`allocate_from_fixed_pools`).
  Keep the original payload for free-head publication. The buffer boundary
  and node construction may need to change together (N13); verify natural
  unrolling, all siblings and data. No manual unroll, extra cursor aliases or
  invented storage.
- Retained base selects too early at the allocator's degree threshold:
  REQUIRE exact native replay attributes the missing dependency to a real
  allocation result consumed by initialization. TRY naming that allocation
  inside the complete factory before passing it to the initializer
  (`bleed_restart`, `blood_create_proc`). Its inline owner can add a coalesced
  neighbor even when final instructions retain no extra move. Confirm the
  native degree/sweep change and all consumers; a synthetic degree increment
  alone does not justify a copy, helper or lifetime extension.
- A loop's nested teardown body argument scans after other FE temporaries,
  while a fully expanded cleanup makes its callback snapshot a named local:
  REQUIRE exact replay identifies both owner-order changes and retail keeps
  the current entry, embedded body and index across cleanup. TRY owning those
  three actual loop values at operation scope and keeping the complete async
  cancel/free transaction inline (`script_system_reset`). Its saved process
  becomes FE-owned without inventing a copy; preserve callback reloads and
  shared standalone unload bodies. Read proven entry-relative fields through
  `current->body` when retail retains that base (H06).
- A caller-named HUD owner selects after root/cursor/zero temporaries while
  retail needs it first: REQUIRE exact native replay and one complete state
  transition with supported reads and stores. TRY inline ownership of that
  whole transition (`mk_chess_request_piece_fight`: HUD/cursor setup and camera
  completion publication), preserving fresh owner reads and instance checks.
  The actual inline HUD local can select before the CSE and conditional-result
  webs. For an independent stale-list next-pointer residue, use the canonical
  complete discard-and-advance helper only when its pre-destroy snapshot and
  returned next pointer are exactly the caller's operation. Check all siblings
  and relocation modes; do not introduce field-only helpers or passive aliases.
- Nested validation result cannot join a named caller result outside the FE
  window: REQUIRE native bounds, the same null result and a complete operation
  shared by the API and callback. TRY owning the full null-or-return/service
  transaction (`mslStreamFile_ReturnBuffer`). If nested copies still prevent
  the measured result/token/queue order, flatten that actual validation phase
  and apply one verified declaration group. Check every captured color and
  the callback that discards the return. Keep distinct interrupt tokens and
  restore/service order; retain validation helpers used by other consumers.
- An early allocation result displaces later disjoint search/stride owners,
  while a retained waypoint pointer stays named below CSE owners: REQUIRE
  exact replay predicts both kind changes. TRY complete typed allocation and
  waypoint-placement operations (`npc_set_path`), passing the actual indexed
  waypoint to placement. Reuse the allocation implementation in its existing
  public API; preserve its emitted body/order. Keep post-callback owner reloads
  and the script-index snapshot across process creation; no lifetime copies.
- Late scalar sample selects below normalized component producers: REQUIRE
  native replay and repeated normalize/store/scale phases with the same
  rounded arithmetic. TRY one complete operation sampling the actual scalar
  output by address after normalization, then name the real normalized XYZ
  products used by both their stores and later scaling
  (`rnd_bend_vector`, `rnd_point_in_sphere_section`). Put the late magnitude
  after those products in the measured inline declaration order. A by-value
  scale can remove the required late load; preserving that load alone can be
  byte-neutral while changing its owner kind. Keep all stores and existing
  inverse-sqrt semantics; no fake escapes, extra reads or merged FP stages.
- Nested search result is selected in a volatile register then copied to a
  saved caller local: REQUIRE native coalescing rejects the caller owner as
  outside the FE window, and the complete outer traversal is independently
  supported. TRY inline ownership of that full traversal, keeping its inner
  search nested (`obj_set_material_fade`). The outer result can join the FE
  window and merge with the inner result through the return join; declaration
  order alone cannot permit that merge. Preserve initial null, failure and
  found exits, callback order and final publication. Check other API wrappers:
  the already-exact public lookup can require its original body.
- Script record member access distributes one-based indexing into a displaced
  load, and name publication rotates descriptor/base owners: REQUIRE the
  existing canonical attributes and string-relocation APIs are visible and
  preserve the same guards, optional resolver and callback reloads. TRY those
  complete operations (`cmdscript_setup_execution`: attributes-table lookup,
  then `script_string` after the trial callback). They can restore indexed
  addressing and name-add order without a new helper or pointer-before-array
  spelling. Account for restored instruction bytes and later text shifts.
- Retained root selects below its scaled selector, and the first field's
  constant is emitted after the root address: REQUIRE a pure canonical array
  address, the same first observable store and no call before the snapshot.
  TRY that actual store through the canonical root before capturing the root
  for later callbacks (`reset_storage_device_status_structure`). The two real
  address uses can create an IRO root CSE where snapshot-before-store keeps a
  named owner, and reorder the constant/address producers together. Preserve
  all stores, later authoritative reloads and retail call boundaries (H07);
  do not add a passive alias or repeat a getter with observable reads.
- Named old-cell address displaces cached piece/board CSE owners: REQUIRE
  exact native replay and the same occupant comparison and guarded clear.
  TRY a complete conditional clear taking the actual indexed cell address
  directly (`mk_chess_put_active_piece_at_cell`). Its real inline parameter
  can number above the CSE owners where an address-only accessor stays named.
  Preserve destination-before-previous evaluation, independent coordinate
  conversions and post-update global reloads; do not cache narrowed inputs.
- Search cursor is inline-owned below a character sign-extension temporary:
  REQUIRE exact native replay identifies that order and an existing index
  already counts the same null-terminated first-match scan. TRY using
  `keys[index]` in its guard and comparison (`rewrite_button_string`). The
  backend can create the advancing cursor above the extension where a named
  walk pointer cannot. Keep the char ABI, terminator and not-found exits,
  comparison/load order and standalone helper emission; check all consumers.
- Advancing output is a named pointer below quotient/remainder codegen temps:
  REQUIRE exact native replay predicts that pointer must select first and an
  existing inner-loop index already identifies each write. TRY indexing the
  real output by that index (`tile_image`: `output[pixel]`). Normal strength
  reduction can own the advancing byte address above codegen temps without
  changing instructions. Preserve separate outer offsets, signed source math
  and store order; do not add an offset local or arithmetic-only helper.
- Callback result is a named owner below the bank index and stride CSE:
  REQUIRE exact native replay and a verified canonical response prefix. TRY
  a complete bank-response publication inline owning result snapshot, status
  dispatch and all success/failure stores (`lsba_callbank`). Keep guarded bank
  extraction in the callback and preserve bank/result/status load order. A
  declaration reversal cannot move the handle above a CSE owner; do not add
  an artificial handle alias or change the callback extension layout.
- Existing complete byte-alpha setter is visible before a fade handler:
  REQUIRE the same once-per-loop screen capture, per-vertex payload reload,
  store order and native owner evidence. TRY that setter for both fading and
  final clearing (`p_mk_chess_fade_display_msg_handler`). Its actual parameter
  and index owners can restore step/screen/index homes and alpha-zero versus
  index-zero order together. Recheck visibility before trusting an old
  forward-declaration ceiling; do not duplicate the setter or change its users.
  An existing complete texture setter defined after an earlier consumer can
  have the same boundary effect (`SpreadSheet_image::Update`): REQUIRE it is
  non-emitted and later consumers remain unchanged. TRY moving its unchanged
  definition before the consumer and passing the actual indexed inputs. Keep
  disposal/init order and post-callback reloads; audit all data and relocations.
  Repeated text-color snapshots can expose the same owner boundary: REQUIRE
  identical live-instance validation, fallback owner reloads and four channel
  stores. TRY one complete typed copy with actual text and RGBA inputs
  (`TextList::ProcessParams`, `SpreadSheet_text::FinishSetup`). If native replay
  identifies only those input owners swapped, test their private parameter
  order and recheck both consumers. Preserve independent loop/pool residue.
  A byte-mutation walk has separate branch cursors, and recursive inline
  consumers retain index increments: REQUIRE the index actually addresses the
  byte, signed-char predicates, reloaded bytes before writes, and a late font
  load. TRY the genuine indexed walk in the complete original method before
  adding another helper boundary (`ScreenText::ChangeCase`,
  [evidence](reports/glue-event-inline-ownership.md#indexed-ascii-walk)).
  Standalone strength reduction can remove index bookkeeping while inline
  consumers retain it. A complete ASCII-only phase can improve a partial
  control, but is not proof that the extra boundary existed. One shared mutated
  cursor parameter can also fold two branch copies into one pre-branch copy;
  recheck CFG and every inline consumer, not just standalone register homes.
- Caller retry state claims saved homes below loop-address owners: REQUIRE
  exact native replay and a complete per-team initialization phase. TRY one
  inline owning the slot loop, acceptance/countdown and duplicate scan
  (`mk_chess_set_up_passed_in_chess_game`). A retry-only helper leaves the
  caller slot competing with address CSE/SR owners; nesting it beneath a team
  helper can put the outer slot above nested retry locals. Keep those real
  owners in the same operation and use the measured inline declaration order.
  Preserve RNG, full duplicate scan, lock/availability calls and fallback order.
  Recheck callers: a smaller public frontend body can start auto-inlining;
  retain the evidenced out-of-line boundary narrowly (H07), then audit the TU.
- One caller-named requested ID displaces several saved traversal owners:
  REQUIRE exact native replay identifies the ID's later simplify sweep and
  a matched sibling supplies the complete per-item boundary. TRY owning the
  bank-plus-item calculation, material search and guarded visibility call in
  one inline (`limb_sever_show_z_meat_chunks_all`, following the hide operation).
  Keep the direct geometry argument and sentinel traversal; a search-only
  helper leaves the ID in the caller. Remove the superseded unused wrapper.
  Reusing this complete operation for a limb-specific table can also restore
  the material-search return boundary (`limb_sever_show_z_meat_chunks`, partial).
  Stop at a commuted unsigned add if reversing shared operands or changing the
  ID parameter boundary regresses the exact sibling; do not duplicate the
  traversal solely to select operand order.
- Decoder round-up numerator and trailing-sample homes swap: REQUIRE exact
  native replay and unchanged signed arithmetic, snapshots and publication.
  TRY a complete decode-completion helper owning sample accounting, ring
  wrap copies, reset and callback publication (`ADXB_ExecOneAdx`). A tail-only
  arithmetic helper can rotate the block owners instead. After extraction,
  use the replayed order of actual inline locals to resolve neighboring saved
  homes; preserve load, copy and callback order.
- Inline container lookup replaces a caller's atomic owner with an IRO temp:
  REQUIRE exact native replay identifies that temp above the named list node.
  TRY the existing canonical container macro at the real local assignment,
  then the evidenced declaration order of node/end/atomic
  (`ShadowCameraUpdate`). Macro alone can be byte-neutral while declaration
  order alone regresses; verify the combined named owners (N13). Preserve
  the post-callback next-link read and canonical member offset.
- Named access flags select below a global-root temporary: REQUIRE exact
  native replay and unchanged nullable restriction checks. TRY a complete
  access predicate taking the flags, piece and mode root, with structured
  manager selection (`mk_chess_check_spell_target_rules`). A ternary can merge
  the manager/root home. If a caller-named piece then selects below the root
  CSE, use the canonical cell member at its existing guard and predicate call;
  require no intervening mutation so the shared load remains one operation.
  Keep the indexed flags slot and lazy restriction boundaries.
- Intrusive-list atomic owner returned through a tiny container inline is an
  IRO temporary above named sentinel/next: REQUIRE native owner-kind evidence
  and canonical offsetof-backed containment. TRY direct `RW_CONTAINER_OF`
  at the actual owner, canonical list-head comparison, then one measured
  declaration group for the resulting real named atomic before geometry,
  material and index (`GCNSetupNonRenderwarePipeline`). Scope alone cannot
  move the old IRO result. Keep next separate and both advance arms; consuming
  the cursor early can delete retail moves. No header edit or raw offsets.
- Simulated board move rotates capture and coordinate homes: REQUIRE native
  kind/pop evidence for the actual multi-definition captured-piece owner. TRY
  structured capture selection and one genuine local declaration group
  (`mk_chess_drone_piece_best_path_to`). Trace path-only rebuild locals too:
  inline skill/map declaration order can affect every expansion. A manager
  ternary may remain an IRO expression below the nested restriction result;
  structured manager selection restores its real inline owner. Preserve
  manager lookup, map load, restriction lookup order, or the map load moves
  early even when homes agree. No new snapshot record or offset local.


- Top coordinates are CSE owners below nested distance arguments, but retail
  needs them above: REQUIRE native kind/claim evidence and unchanged ordered
  selection, separately rounded squares and canonical sqrt. TRY leaving
  vertex selection/cyclic indices in the caller and owning complete geometric
  measurements in an inline with real coordinate samples and radius/height
  outputs (`convert_cdf_triangle_to_collision_cylinder`). Moving topology too
  can invert index/displacement ownership. Derive component argument order
  from retail evaluation (Z-first inputs under right-to-left evaluation), keep
  the original X-square-plus-Z-square association, then check inline neighbor
  declaration order. No duplicate getters or synthetic coordinate lifetimes.

- A nested lookup name takes a saved home needed by its outer table cursor:
  REQUIRE native replay identifies the name above the traversal owners and
  retail has separate external-resolution and internal-rebasing passes.
  TRY an inline owning the complete external resolution pass
  (`fixup_data_tables`). Recheck the remaining pass: formerly shared second
  webs become first named webs, so its index/cursor declaration order may
  need the corresponding H15 adjustment. Preserve live bounds and lookup exits.

- A secondary model snapshot takes a CSE saved home while retail reuses the
  dead loading-loop stride home: REQUIRE native replay, actual publication
  before callbacks and pointer retention across the first callback. TRY a
  complete inline owning both model snapshots, both scene publications and
  both setup callbacks (`pz_fighter_load_and_place_initial_grinders`). Keep
  each read/store at its original boundary. A caller-named snapshot or a
  synthetic second-element getter does not recover that operation ownership.

- Repeated per-slot retries rotate saved homes: REQUIRE native replay and
  the same candidate publication, duplicate scan, eligibility checks and
  bounded fallback at both sites. TRY an inline owning that complete choice,
  leaving outer traversal and selected-cell ownership in the caller
  (`mk_chess_set_default_chess_game`). A whole-team helper is a different
  boundary. A consumed prior-character snapshot can preserve comparison
  operand homes after extraction; verify both expansions. If lower inline
  cost expands the enclosing function into a caller that retail calls, apply
  H07's evidenced call-boundary control and recheck the caller; never pad the
  helper or ignore the caller regression.
- Squared-length input must select before its inverse-sqrt result: REQUIRE
  exact native replay identifies the input/result order and unchanged rounded
  arithmetic. TRY naming the genuine squared length in the enclosing orbit
  inline before `gxMathFastInvSqrt` (`make_interaction_orbit_offset`). The
  outer inline local can precede the nested result where a call argument does
  not. Preserve post-sqrt component reloads and intermediate vector stores;
  check every expansion and shared consumer.
- Nested metric result must select before its outer coordinate snapshot:
  REQUIRE full native replay, identical null/eligibility guards and the
  canonical metric's early-return CFG. TRY owning the coordinate calculation
  in the caller's existing filter branch while retaining the metric helper
  (`mk_chess_find_my_smart_piece`). Caller-named coordinates sort below the
  inline distance owner; keep the actual neighboring Boolean at the filter
  join. Opening the metric too can remove a branch pair. Preserve array-store
  order and exact shared consumers; do not change the shared predicate merely
  to repair one caller.
- Inline list cursor/trigger swap survives their declaration exchange:
  REQUIRE native creation and selection order shows the saved-next local
  claiming the shared home first. TRY declaring the actual next, trigger and
  cursor as one helper-local group in the replayed order
  (`p_konquest_switch_3`). Keep next assigned only at the original stale-link
  boundary before destruction. A two-owner solver or declaration check misses
  this third claimant; preserve callback, clear, destroy and advance order.
- Signed effect snapshot merges with an argument register and its unsigned
  API conversion becomes a CSE, while a reused later effect becomes a split
  web: REQUIRE canonical unsigned handle fields/APIs and native claim order
  showing both select after a later operation's owners. TRY a complete
  activation inline with distinct API-typed emitter and health-effect owners
  (`mk_chess_cell_monitor`). Preserve the observed signed zero check explicitly,
  guarded position snapshots and every reset/bind/update/resume call. A partial
  bind helper can add a result copy; one unsigned caller local can retain the
  wrong allocation. Retrace both real handles and particle ownership.
- Named root/rotation pointers must select before a validated-object CSE:
  REQUIRE exact native replay and a repeated complete root-transform operation.
  TRY an inline owning bone selection, translation and quaternion correction
  (`set_anim_script_frame`). Keep the real aligned matrix scratch in the
  caller when moving it into the inline loses the aligned prologue; borrow it
  as the operation's matrix argument. Retrace inline pointer order and actual
  addressed Vec/Quat slots; preserve assignments behind the original guards.
  Leave an already-exact sibling expansion spelled out when sharing regresses.
- A loop-invariant product remains a strength-reduction owner with the wrong
  saved home: REQUIRE native replay identifies that owner and retail computes
  the product once before the inner loop. TRY naming the read-only product at
  that boundary (`save_to_memcard_w_error`, partial). Retrace its named owner
  and all remaining SR owners; preserve derived offsets, bounds and retries.
  Do not replace the product with a manually advanced counter. A single-use
  file-offset local may fold away; remove it when allocated output is neutral.
- Cursor lookup plus retained piece snapshot: REQUIRE exact replay shows the
  named cursor selecting after a mode-root CSE and the named result claiming
  a saved home only after an earlier camera web. TRY a complete typed
  cursor-to-piece lookup inline, passing the actual cursor address and keeping
  the returned snapshot at its original boundary
  (`mk_chess_monitor_cam_zoom_scenerios`). Its argument and return owners can
  fix both selections. Keep canonical nested board/slot access and post-call
  re-reads; no duplicate lookup, artificial alias or camera-latch rewrite.
- Shared aggregate reused in disjoint initialization loops: REQUIRE capture
  shows scalarized fields becoming split webs, with saved-home claim timing
  explaining the wrong later colors. TRY declaring the actual aggregate in
  each existing loop so each initialized value has its own owner
  (`bl_init_beetle_pebbles_second_floor`, partial). Preserve initialization
  and call order; retrace every field and the whole unit. A differing-only
  order solver can miss a solution involving an unchanged earlier claimant;
  include that claimant before diagnosing different interference.
- Correcting source-load numbering reverses an otherwise exact schedule:
  REQUIRE native replay predicts the color swap and scheduler evidence shows
  equal-priority loads resolved by input order. TRY an inline owning the
  complete multi-output calculation and its shared input snapshots; declare
  those real locals in predicted FE order, then read inputs in retail order
  (`camera_check_reverse_move_offset`). By-value scalar arguments can retain
  right-to-left evaluation and preserve the schedule mismatch. Keep output
  aggregates in the caller, all original stores and FP operations, and verify
  both ownership and schedule. No added dependency or identity wrapper.
- Named interpolation rate sorts below a cached old-value CSE: REQUIRE native
  replay identifies their exchange and unchanged operations/load timing. TRY
  an inline owning the complete interpolation and its rate snapshot
  (`attract_move_flyby`, `attract_default_move`). Reuse the same complete
  phase when available; preserve distinct raw/scaled roll values so a named
  zero read-modify-write does not displace the coordinate-add temporaries.
  A rate argument can sort below a delta local instead;
  retrace before choosing the snapshot's actual local declaration order.
- Retail keeps a call-result copy that an inline consumer loses: REQUIRE
  both moves exist in pre-coalescing PCode and native capture shows result
  and inline local merge inside the FE temporary window. TRY expressing the
  complete operation in its existing caller branch so the actual retained
  value is named outside that window (`material_start_uv_scroll`). Retrace
  the unmerged result/local pair; preserve lazy reads and every call. Caller
  address-taken output locals may need reverse declaration order relative to
  inline copies to retain stack slots. Conversely, a checked-cast diamond
  keeps an extra pointer copy when its raw named owner is below `first_fe`:
  REQUIRE native capture proves that copy is ineligible, with no intervening
  mutation of the raw owner. TRY direct common-owner expressions for the
  guarded reads and cast inputs; the CSE temporary can merge with the checked
  result while retaining the branch diamond (`insert_screen_obj`,
  `insert_string_obj`). Recheck real priority/vtable declaration rank after
  the owner change; it can rotate previously correct saved homes. Verify
  the native merge and keep other cast copies whose ranges interfere.
  No extra alias or identity helper.
- Validation/create inline loses a post-validation copy: REQUIRE native
  evidence shows its raw input, conditional result and inline owner merge,
  while retail retains the validated-or-created object. TRY naming that
  result directly from `MK_LIVE` in the existing caller branch and keeping
  creation, publication and consumption with it (`update_combo_break_counts`).
  Naming the raw input is a different boundary and may retain the wrong copy.
  Named owners cannot merge virtually with FE temps but may still merge with
  physical call results; resolve simplify-stage roots through physical
  endpoints. Preserve other helper expansions and lazy reads. See the
  [paired evidence](reports/combo-counter-coalescing.md).
- A named sqrt result adds an FMR before sign selection and vector stores:
  REQUIRE native coalescing excludes the named owner while retail retains
  the sqrt result. TRY one typed inline owning the signed result and all
  output stores (`p_npc_on_pendulum_rope`). Verify its local actually merges
  with the result. If nested expansion moves sqrt stack slots, check the
  adjacent complete calculation's inline boundary while retaining escaped
  output aggregates in the caller; never add padding to restore slots.
- Named trig results add copies across the next call: REQUIRE native
  coalescing excludes the named result from the FE window. TRY one inline
  owning the complete guarded angle calculation, trig calls and velocity
  stores (`myvel_his_angle_y_inout`). A sin/cos-only helper can leave a copy
  into the caller's named result. Retrace both result merges, then declare
  the actual helper locals in the measured selection order. Preserve caller
  sign decisions, normalization rounding, call order and player-context
  preconditions; do not add initial values for an unsupported owner path.
- An earlier independent phase makes later bone/record owners split webs:
  REQUIRE native creation-kind evidence, identical operations and actual
  phase-specific roles. TRY distinct owners inside the earlier phase's
  existing loop, leaving later owners named (`mkobj_update_weapon_trail`,
  partial). Retrace interference; removing the split does not prove coloring
  closed. A complete phase helper can add a result join and rotate outer homes.
- Quaternion coefficient homes rotate across three trig calls: REQUIRE
  identical call/FP order and alias-sensitive input reloads. TRY one inline
  owning the trigonometric weights, keeping hemisphere selection and its
  threshold in the caller (`gxQuatInterpQuat`, partial). Broader sign/magnitude
  ownership can change constant/result webs; passing sign can propagate away.
- Horizontal-vector components rotate with a sqrt result, or a named delta
  swaps FPRs with a retained coordinate CSE: REQUIRE real X/Z displacement
  semantics, identical rounded operations, and native
  `IRO_ScalarizeClassDataMembers` creation-order evidence. TRY canonical Vec
  locals with only the consumed components assigned, then assign X before Z
  when replay requires X created first and numbered above Z
  (`mk_chess_cursor_tracker_update`). Check that no uninitialized component
  is read or copied and the scheduler preserves retail issue order and fusion.
  Scalar statement reorders can be neutral while aggregate scalarization
  changes ownership; preserve separate scheduling and multi-definition
  residues as partial.
  For normalized planar components, REQUIRE a measured scalar norm-order
  control that fixes issue order but rotates raw/result homes. TRY a canonical
  `Vec direction` for the consumed normalized outputs while retaining retail
  square/sum order (`nb_npc_hurt_player`). Scalarized outputs can move above
  raw-coordinate CSEs in allocation order without adding operations. Verify
  both owner kinds and homes with native replay; do not treat positional
  votes between differently ordered squares as component identities.
  Same diagnosis for a complete 3D separation competing with cached position
  components: TRY `Vec delta` at the real distance calculations, then measure
  repeated read-only query phases independently. Earlier coordinate order
  can determine threshold scheduling while a later query determines CSE
  discovery order (`bl_process_beetle_runaway_personality`). Verify both native
  owners and final issue order; do not repeat a failed scalar-order control
  without the changed ownership evidence.
- Two terminal branches expand the same wait/save helper, but only the first
  expansion has wrong saved homes: REQUIRE identical calls, effects and CFG,
  real snapshots live across mutating calls, and no caller work after either
  helper return. TRY caller-owned snapshots shared by those real branches,
  assigning them at the observed read points and preserving all reloads and
  restore stores (`npc_blend_to_ani_string`). Keep the shared helper unchanged
  for its other consumers. Retrace the resulting named webs, then apply only
  the declaration order that exact native replay predicts; opening one helper
  body or reversing the shared helper's locals can remain neutral or regress
  unrelated consumers. No identity wrapper, extra read or dummy lifetime.
- Address-taken caller argument reloads across a call where retail retains
  its value: REQUIRE a complete operation consumes a by-value snapshot and
  retail preserves the same call order. TRY a typed explicit inline owning
  that operation (`mwFileDirectoryMountPoint::convertToPlatformPath` and
  `startOpenFileCommand`: append path plus its measured length). Check the
  canonical overload first; a nested template can outline under `noauto`
  even when a private complete-operation inline expands. Preserve reserve
  before assign; an alias-only helper or reordered allocation is not a fix.
  Reuse that operation in directory `initialize` too; an actual named length
  inside the shared helper can restore the final independent argument-address
  schedule after retaining the path across `strlen`. Check every expansion.
- Named indexed-record alias and backing-buffer expression swap volatile
  homes: REQUIRE exact replay says buffer emission must precede member-base
  emission, with identical accesses and output-store alias boundary. TRY
  dropping the record alias and reading its two fields through the canonical
  index inside the real result/output expressions (`load_binary_block`).
  Separate buffer/offset locals can preserve the old boundary; retrace the
  accepted source and retain each load once.
- Parameter and later loop-object homes rotate together: REQUIRE exact
  allocation replay shows their lifetimes are disjoint and selecting the
  real loop object earlier also restores the parameter's home. TRY a typed
  inline owning the complete per-element initialization and its object
  (`start_magic_rings_effect`), rather than trying to renumber the parameter.
  Preserve indexed selection and calls; pass an escaped pdata pointer by
  address when retail reloads it after stores. A helper owning only the
  parameter's earlier initialization can remain byte-neutral. Retrace the
  object owner and check every sibling and data section.
- Texture-coordinate CSE origin and upper bound swap saved homes: REQUIRE
  exact native replay identifies CSE versus named ownership, with identical
  metric reads and FIFO writes. TRY one typed inline owning the complete
  glyph construction and draw (`romfont_puts`), including coordinate locals,
  `GXBegin` and all sixteen halfword writes. A writes-only helper can remain
  neutral; passing a precomputed upper bound can rotate caller-owned endpoints
  and change a caller's inline eligibility. Retrace the complete operation,
  then order its real endpoint declarations as the replay requires. Preserve
  the right-U calculation after `GXBegin`; check every sibling and pool.
- Selected ID and result swap saved homes across repeated inline calls:
  REQUIRE capture identifies separate inline argument owners while retail
  reuses caller/split owners. TRY assigning each table selection to one real
  caller-owned ID before the complete operation's inline call; preserve the
  original load and call points (`pan_vol_pitch_random_snd_req`). This is
  the selected value, not an alias of an existing local. Check every expansion;
  changing only helper declaration order can repair one and break another.
- A local vector component and scale swap saved FPRs across a random call:
  REQUIRE capture identifies the component as an
  `IRO_ScalarizeClassDataMembers` temp, and the point has not escaped. TRY
  compute that real coordinate into a named float across the intervening
  call, then assemble the local vector. Preserve random-call order, FP
  association and the final point. This fixes the range-5 FPR pair in
  `bl_init_beetle_pebbles_first_floor`; its separate GPR residue remains.
  Do not replace all range-local scales with a shared owner or add a copy
  after the coordinate already exists.
- Separate collision paths reuse one loop index and later traversals rotate
  saved registers: REQUIRE independent bounds and no index use between paths.
  TRY a distinct index in each real branch (`do_cloth_colls`). If a shared
  reference alias only publishes a global before all uses, try direct
  publication separately; preserve reloads across calls. Stop on residual
  register-only differences.
- Fixed-width char padding differs only in cursor/const regs: REQUIRE kept
  length/index already bounding same stores. TRY direct array index, drop
  redundant advancing pointer (`pne_set_players_name_to_default`). Not all
  char walks; keep termination behavior.
- Bounded char-normalize pass swaps cursor + replacement const after separate
  copy: REQUIRE same fixed extent + conditional stores. TRY inline helper
  owning whole pass (`does_name_already_exist`). Keep processing past embedded
  NUL; no string-terminated walk or scalar getter.
- Two equivalent arms differ only by extra return-pointer copies in the
  first arm: REQUIRE first-use named owners there versus split/inline owners
  in the second, with identical calls and pointer lifetimes. TRY one inline
  owning the complete repeated operation, not forwarding/cast wrappers.
  `pui_play_pfx` shares effect lookup, emitter guard, child-relative position,
  object update and emitter restart/resume. Its inline emitter-object first
  claimed r29, forcing the emitter to r31 and a pointer to new r28. Native
  simplify and selection replay predicted declaring the real emitter last:
  it then reuses handle r29 before the nonoverlapping pointers reuse r31.
  Both copies close together. Keep caller validation/frustum guards and
  every call/store in order; verify the full unit and data-value mode.
- Array-reset loop rotates base/index/element regs: REQUIRE complete reset
  phase repeated by other callers. TRY typed inline owning zeroing + sentinel
  init incl index (`bleed_init`). Naming only element/base may not reproduce
  helper-local allocation. Keep every store; check consumers when sharing.
  First search for an existing reset: reuse `blood_reset_splats` for the same
  memset + height loop in `bleed_restart` and `reset_blood_decals`; keep the
  iterator in that shared operation instead of adding another reset wrapper.
  Nested board loops can also recover zero/offset sharing and conversion-store
  order by reusing an exact sibling's complete initializer
  (`p_mk_chess_game_setup`, `initialize_restored_chess_board`).
- Visible-slot search preheader schedules manager load before index zero:
  REQUIRE an existing helper owns the same first-hit removal, counters and
  visibility flag. TRY reuse that whole operation (`npc_make_invisible`).
  Trace the index: named caller ownership can produce two pre-schedule LIs;
  inline ownership permits LI/MR and later coalescing. Preserve manager
  reloads after stores and the helper's other consumers.
- Bounded event drain initializes index and byte offset with separate zeros:
  REQUIRE an existing complete helper preserves stop-on-empty, flag packing,
  flags-field address across dispatch, and clear order. TRY reuse that drain
  before final idle (`screen_engine_process_events`). Check all consumers;
  shortening a public dispatcher can trigger collateral auto-inlining.
- Guarded player/start owners swap volatile regs: REQUIRE same canonical
  background fields, angle-copy slots, player reload after position store. TRY
  real per-player round-start helper owning angle `Vec` + background
  snapshot; null guard stays in caller (`move_plyrs_to_round_start`). Audit
  each player selector.
- Repeated guarded effects swap object/emitter saved homes: REQUIRE the same
  guard, calls, snapshots and stores; variants differ only in state/height
  literals. TRY one typed inline owning the complete effect operation
  (`low_flash_check`): the object becomes an inline owner above the emitter's
  split web. Check every variant and data pool; no alias-only extraction.
- Named FP delta and reused input-load CSE swap homes: REQUIRE exact
  simplify/select replay predicts the delta above the input CSE. TRY an inline
  owning the complete target selection, orientation and signed squared-distance
  operation (`pz_fighters_burn_fatality_prep`). Keep its initial direction
  argument evaluated before target selection; extracting only the arithmetic
  can move that initialization after the selection despite exact FP rows.
  A nested existing post-offset operation can preserve its late aggregate
  initializer (`pz_fighters_objects_falling_fatality_prep`); retrace any final
  inline target-coordinate swap, then reverse only those real declarations
  when exact replay predicts it.
- Shared FP operation changes consumers in opposite directions: REQUIRE the
  same rounded products, arithmetic tree and store order. TRY preserving product
  declaration order while changing their separate assignment order
  (`yxz_trig_to_matrix` improves both converters). Initialized declarations can
  conflate these two facts; judge every consumer together.
- Validated indexed accessor swaps runtime/index regs: REQUIRE same null
  guard, signed bounds, indexed return. TRY typed inline accessor, named
  runtime owner, single-use index passed directly (`fx_restart_emit`). Both as
  expressions folds runtime offset into later loads; both named restores
  wrong pair.
- Cursor/owner volatile homes swap: REQUIRE trace preserves both operations
  from `initial` to `presched`, cursor is a named caller local below the
  common-owner temporary, and one complete selection/publication phase owns
  it. TRY typed inline helper owning that phase and its cursor; preserve the
  lazy manager selection, indexed cell address and publication before calls
  (`mk_chess_show_spell_hud`). Do not add aliases or extract only a load.
  Selected-piece setup followed by a move-line loop can own the index,
  selected-piece publication and move-map snapshot as one operation
  (`mk_chess_set_game_mode`). REQUIRE the index's initialization before the
  first state store and unchanged global reloads/calls. Keep direct indexed
  cursor fields where retail folds their offsets; a nullable-manager portrait
  lookup may instead materialize the cursor element address.
- Operation's retained owner loads via temp before moving to saved reg:
  REQUIRE operation snapshots real owner handle once, keeps it across
  callbacks. TRY owner's address to typed inline owning whole operation,
  read-only handle param, one initial deref (`aniproc_land`). Later global
  reloads stay outside at retail boundaries; no getter-only extraction, no
  handle reread in loop. Compare every consumer.
- Selector formal and selected owner swap after earlier phase ownership is
  recovered: REQUIRE exact allocation replay predicts selector above selected
  owner and the complete operation keeps that selector through callbacks. TRY
  passing the actual selector-field address and snapshotting it once into the
  operation's local after the owner declaration
  (`pz_fighters_handle_next_pending_move`). Preserve each caller's distinct
  source field and lazy owner choice; no getter-only wrapper or later reread.
- Inlined search swaps bound snapshot + tag temp: REQUIRE unsigned bound
  invariant, no calls/stores in search. TRY reading owner's bound directly in
  indexed loop, no named count (`cloth_bones_init_by_tbl`). Changes bound's
  web kind, keeps CTR loop; check shared helper consumers.
- Indexed scan keeps named stride/count ahead of retained table bases:
  REQUIRE native base-plus-offset traversal and a proven immutable bound.
  TRY typed indexing and direct bound (`pfx_estimate_render_size`); compiler
  owns stride/bound invariants after named index/size. Keep observed per-iteration
  input-read order; never replace a callback-visible bound with a snapshot.
- Three region copies keep an advancing cursor where retail keeps source base,
  byte offset and reused row: REQUIRE the same signed bounds and copy order.
  TRY direct typed source[index] members (`pfx_copy_behavior_list`), without
  a named advancing or indexed-row pointer. Verify post-loop nonzero-count work.
- Repeated state tests swap a named state snapshot with a shared owner GPR:
  REQUIRE trace shows the named snapshot below the owner temporary, and no
  calls or aliasing stores between the early tests. TRY direct canonical
  field reads for those tests; retain the separate snapshot taken after
  callbacks (`ani_to_frame_x_col`). Confirm the compiler still emits one
  early field load and preserves every later reload.
- Global tick/list/sqrt homes rotate in an entirely inlined bounded scan:
  REQUIRE no intervening calls or writes to the tick, and a separate reload
  after publication. TRY direct unsigned tick reads instead of a named
  snapshot; verify CSE retains one preheader load and the later reload
  (`spawn_bld_fall`).
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
- Two auxiliary-object latches share an owner across the first hide store:
  REQUIRE retail keeps that owner, while direct global expressions add a
  reload and a named local swaps owner/result homes. TRY one typed inline
  owning both validations and hide stores, with the owner passed once
  (`init_fatality_world`). Trace named owner vs ternary-result web kinds;
  preserve each lazy instance check and store, not just the pointer loads.
- Hide/show twins miscolor a retained fighter through auxiliary visibility:
  REQUIRE native CSE/latch merging and the same complete post-weapon operation
  in both callers. TRY one typed inline owning that latch, both auxiliary
  visibility calls and the attached-list walk (`hide_player`, `show_player`).
  A list-only helper misses the boundary. Retrace its real link/save-next
  locals before adjusting their declarations; preserve the pre-destroy next
  snapshot and show-only object exclusion.
- Repeated fixed board scans miscolor several counter groups: REQUIRE a
  retail MAP entry for the generic coordinate iterator and identical
  callback loops. TRY that iterator at each real scan, nested inside the
  complete count/reset operation (`mk_chess_end_of_turn`). Original helpers
  `mk_chess_iteratate_thru_all_board_cells` (0x5C) and
  `mk_chess_count_all_power_squares` (0x10C) also match ordinary standalone
  extents. Preserve callback ABI, signed loop tests, bounds and call order;
  declaration reversal cannot replace missing iterator ownership.
- Repeated UI process setup has delay FPR residue despite identical operations:
  REQUIRE retail helper-map evidence and native owner/emission capture. TRY
  complete slide, fade and width-based scale operations at the supported nested
  boundaries (`mk_chess_advantage_hud`, partial). Pass unsigned delay ticks and
  convert at publication; a float argument can carry speed across calls. Check
  every escaped output slot: nesting only slide separates its slots from fade,
  and leaving scale one level shallower moves its slot ahead of the rows. Keep
  genuine viewport/texture reads in their measured order; no dummy wrapper or
  padding. Map names alone do not prove reconstructed helper signatures or
  standalone extents. Stop separately on remaining owner GPR differences.
- An array's secondary object alone takes the wrong saved home after a load
  loop: REQUIRE both objects are published, then passed to creation calls in
  fixed order, and trace identifies the secondary as a CSE owner. TRY one
  typed inline owning both publications and both calls, passing the two
  objects directly (`pz_fighter_load_and_place_initial_burn`). Follow the
  unit's existing registration helpers; preserve both snapshots across calls.
  An array-index getter alone does not express this operation.
- One re-resolved 2nd web of a multi-def object local takes the next-lower
  saved reg (retail reuses the reg of its other webs): REQUIRE same latch ops,
  capture shows that web numbered above the sibling webs. TRY direct
  `x = LATCH(src)` (`?:` result) instead of in-place copy `x = src; x =
  LATCH(x)`: copy form numbers the variable's 2nd webs forward in source
  order, direct `?:` form numbers them in reverse, so the late web pops after
  its siblings and reuses their reg (`p_fish_attack`). Apply to every site.
- Saved-reg swap around an inlined resolve helper (buffer pointer vs table
  base vs param): REQUIRE same calls/CFG; capture shows the swap set by web
  kind/degree. TRY name the buffer pointer in the caller (`p = buf;
  helper(p, ...)`), not as a helper-local alias: caller named local numbers
  low and drops coalesced copies next to the table web (`cvFsOpen`, donor
  RE4 `pdev = dev`). Retail `mr rN, r3` after an inlined allocator: return
  `void*` from the helper so the result is not propagated into the typed
  local. CRI units: port donor helper shapes (RE4/Sonic Heroes cvfs) first.
- Table-decode volatile pair swapped (retail `lhax r3; clrlwi r0,r3; srwi
  r3,r3,8`, ours r0/r3): REQUIRE same ops, decoded field stored, length used
  twice. TRY length as an unnamed `(u8)descriptor` expression written after
  the field store, no named `code_length`. Emission temps number forward, so
  the length temp pops before the field temp and takes r0
  (`MPVDEC_DecBpicMb`/`DecPpicMb`, donor RE4 `MPVDEC_MBTYPE`).
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
- A pure inline predicate's named member pointer swaps a volatile home with
  the caller's owner CSE: REQUIRE exact allocator replay predicts the member
  below the owner, and no call/store changes either pointer in the predicate.
  TRY direct canonical member expressions under the same lazy null guards
  (`start_special_weapon_monitor`, `moves_is_weapon_style`). Verify the
  compiler retains one member load and every expansion stays exact; changing
  unrelated latch or activation code cannot fix this pair.
- Repeated atan-to-degrees expression swaps product and pi temporary FPRs:
  REQUIRE identical atan ABI and separate multiply/divide rounding. TRY typed
  inline owning the plane-heading conversion, then subtract current heading
  in the caller (`bl_process_beetle_climb_a_wall`). Caller angle locals may
  retain the swap; do not replace division with a precomputed scale.
- Inverse-square-root input/result FPRs swap in a heading calculation:
  REQUIRE an existing typed squared-length helper implements the identical
  component expression and another matched caller already uses it. TRY that
  helper as the inverse-square-root argument (`retarget_projectile`). Trace
  the input and result webs separately; preserve fused arithmetic, zero/NaN
  handling and all velocity stores. Naming a scalar sum or extracting the
  whole heading phase may leave the wrong input web unchanged.
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
- Owner snapshot alone miscolored in a duplicated named-effect spawn: REQUIRE
  an existing API with the same lookup/bind/reset/resume sequence and owner
  read before lookup. TRY one complete inline implementation shared by that
  API and the caller (`fatality_explode_victim`, `pfx_spawn_at_bid`). The inline
  object parameter changes the owner web. Preserve public definition order;
  a forward call may not inline. Check the public wrapper and all consumers.

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

- Web numbering (R3): inline-expansion locals AND params are `@`-renamed and
  numbered per expansion in reverse declaration order, after all real named
  locals. Helper decl order only sets the relative pop order of its own
  locals. If reversing fixes one expansion and breaks another, the
  helper/caller boundary is wrong (open-code or move it), not the order
  (`p_create_profile`).

- Float extent multiplication creates its literal before the integer-width
  conversion, while retail colours conversion first: REQUIRE identical
  single-precision multiplication, integer truncation and store locations. TRY
  a complete typed inline `int extent(float width, float scale)` and pass the
  converted width at each existing store (`ending_show_text`). Mutable float
  staging can fix creation order but coalesce the product into the wrong
  owner; a returned extent keeps the complete operation together. Recheck
  conversion slots and all FP rows; independent pointer-reload homes may remain.

## H22

- Byte-wrapped accumulator has a normalization temporary live across the sum:
  REQUIRE promoted byte inputs, a defined bounded sum, actual scheduled
  compare/add order and their interference edge. TRY the equivalent low-byte
  mask on the sum (`p_show_text_window`, verified candidate). It can retain
  compare-before-add and remove the overlap; do not widen arithmetic, change
  clamping or manufacture a dependency. Check all inline/outlined consumers.
  Shared note 9253 preserves the byte-wrap evidence.
  Require unchanged conditional-arm typing and shift/mask folding as well:
  a mask on one byte-typed conditional arm can introduce a new narrowing
  instruction (`do_win_effect`), while a cast after a signed shift can prevent
  rotate-and-mask folding. Equivalent values alone are insufficient; verify
  the exact narrowing and shift instructions in every expanded consumer.

One-row operand/constant/copy residue. One at a time:

- Guarded field update loses a retail reload before backend optimization:
  REQUIRE initial/build PCode evidence and the same comparison, lazy guard
  and actual read-modify-write. TRY the natural compound field update
  (`ImageList::Decrement`: `scrollBase -= delta`). An explicit assignment
  from a repeated field expression can share the comparison load where the
  compound frontend node preserves the reload. Recheck comparison operand
  order after this producer change; an earlier equality-swap result need
  not carry over. No fake volatile, alias, extra read or lifetime barrier.
- Clamp count/focus CSE homes are reversed and the comparison sense differs:
  REQUIRE native replay and the same post-publication count reload. TRY
  naming that count for its two genuine uses, comparison and clamp result,
  then compare focus against count in retail operand order
  (`ImageList::RefreshCollection`). This moves count below the focus CSE;
  naming focus moves the opposite owner. Keep the signed bounds, load/store
  boundary and lower-clamp path; no extra use or cached earlier count.
- Encoded string rebasing leaves only reversed ADD operands: REQUIRE the
  unsigned zero sentinel, base load, one-based correction and single final
  store. TRY a real encoded-offset snapshot, test it, add the base in place,
  then publish offset minus one (`fixup_data_tables`). A direct expansion of
  the compound memory assignment can have different frontend staging.
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
  For a scaled-and-translated destination, separate it from normalized
  displacement at the multiply, then scale and translate the destination
  in place (`active_sidekick_swap_from_behind`). A split only at the final
  add misses the required source/destination owners. Match the real delta
  versus retained-origin declaration order from native replay (H15); these
  destination locals must be modified and published, never copy-only aliases.
  Same staging can recover the product destination before `fctiwz` when
  operand order is already right: mutable float = affine factor, separate
  `*= count`, then integer-field assignment
  (`puzzle_fighter_mode_play__new_piece`). Preserve the retail `fmadds` /
  `fmuls` rounding tree; nesting `*=` in the field assignment loses the shape.
- Left-operand multiply assignment merges input/result and recolors otherwise
  exact FP code: REQUIRE native capture distinguishes the two source values,
  with retail retaining the multiply and all consumers. TRY separate genuine
  raw delta and scaled step, then publish the step (`p_krypt_camera_loop`).
  `step = delta * scale` can preserve both webs where `delta = delta * scale`
  becomes a read-modify-write web. Keep zero/NaN behavior; no copy-only alias,
  unused input or constant expression that folds away the retail operation.
- Normalized dot inputs need to emit before a scale constant: REQUIRE native
  replay identifies the constant's emission rank and identical FP rounding.
  TRY complete the actual dot local, then `dot = scale * dot` before its
  comparison (`dist_from_plyr_pos_to_arena_edge`, partial). Compound `*=` can
  merge the raw/product destination and reverse the required multiply operands.
  Keep canonical nested math helpers and their independent guards.
- Base + const field + dynamic offset (`addi` then `stwx`), ours folds field
  into store `[da]`: `(base + 0xC) + offset`, not `base + (offset + 0xC)`.
  Never hide offset in pointer representation (`_rwResourcesClose`).
- Allocator payload address folds header size into an indexed operand:
  REQUIRE a valid byte arena, a nonoverflowing prefix-plus-header offset and
  native evidence that selection materializes the indexed pair too late.
  TRY `base + (prefix + sizeof(Header))`, then measure the actual compile
  (`fixedBlockHeapResetHeap`, partial). Source association need not survive
  selection; staged copies may remain neutral. Do not add an artificial use
  or qualifier to force materialization. Evidence:
  [payload-offset report](reports/fixed-heap-payload-offset-grouping.md).
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

- Signed-conversion stack roles rotate although the projection/viewport math
  and cast count agree: REQUIRE native conversion creation/resolver evidence,
  unchanged FP trees, and all inputs read before final output stores. TRY
  deriving the existing real output values incrementally from their actual
  offset/scale inputs (`camera_get_screen_pos_from_world_pos`, partial).
  A scalar offset snapshot alone can fix one role while leaving the others
  wrong. Keep every conversion, rounding boundary and final publication;
  no added scratch locals, casts, aliases or stack padding. Check each retail
  conversion role against the native resolver, not scratch-object UID order.

- Plane coefficients need persistent scalar owners, but copying their record
  introduces stack traffic: REQUIRE an existing proven record type, all fields
  sampled before output stores, and native scalarization evidence. TRY filling
  its actual members individually while preserving the original product phases
  (`unit_vector_to_area`, partial). GC2.7 can scalarize these member assignments
  even when aggregate copy remains a memory operation. Preserve the FP tree and
  access order; a complete-expression rewrite can still change scheduling.
  No invented aggregate or extra component read. See
  [the plane snapshot evidence](reports/navigation-plane-scalarization.md).

- Projected products need distinct destinations before force addition and
  output division: REQUIRE retail's separate rounded multiply/add phases,
  normal snapshots before output stores, repeated divisor reloads, and native
  producer/propagation evidence. TRY real tangent-force accumulators initialized
  from the normal products, then add the persistent force before division
  (`nb_get_desired_acceleration`). Mutating the raw normal retains the wrong
  owner; single-use projected fields can propagate to late consumers. If
  scalarized result fields still have the wrong creation kind, use genuine
  scalar components and one native-predicted declaration order, preserving
  every other value; no member-order sweep or fake lifetime. See
  [the tangent-force case](reports/npc-tangent-force-ownership.md).

- A weighted partial sum and the final corrected result need distinct owners:
  REQUIRE retail's in-place partial addition, unchanged rounded FP tree, and
  native propagation decisions for the actual product/sum uses. TRY accumulate
  the weighted target into the real scalar weighted sum before assigning the
  corrected final result (`nb_npc_slave_hit_by_plyr`, partial;
  [evidence](reports/npc-weighted-sum-propagation.md)). Accumulating directly
  into the final vector can claim saved FPRs too early. Preserve axis order,
  normalization stores, products and both rounded additions; no dead reads or
  unrelated temporary reuse. If retail completes that pair sum before scaling
  the independent correction, and native PCode retains the reverse order,
  TRY completing the partial sum first within each axis. Require disjoint
  scalar dependencies and unchanged operations; this can repair scheduling
  while leaving mass and contribution homes unresolved. An observed unavailable
  use proves that gate, not the cause of its availability kill.

- Serialized pointer field is relocated in place, but pointer addition reverses
  retail ADD inputs: REQUIRE load/add/store width and a proven offset-to-pointer
  transition. TRY the existing relocation union with offset accumulation and
  its pointer view at runtime APIs (`mslBankReadAssetHeaderComplete`). Preserve
  the real target-width address conversion and field layout; canonical
  pointer+integer commutation can be byte-neutral. Check every shared consumer;
  never overlay an ordinary pointer merely to force operand order.

In-place derivation `[da]`. IF retail derives value in the register it was
loaded into, REQUIRE one meaning for the variable, TRY computing it in steps
in one local (`margin = width; margin = (640 - margin) & ~1;`). mk-da: `bytes
= samples * 2; bytes *= track; code += bytes;` keeps shift/mul operand order;
capacity becomes byte count then `&= ~127`; next-page ptr set from allocation
then advanced. Never a second meaning or a copy of an in-scope value.

- Reverse case, retail separates constant and derived-result registers:
  REQUIRE the alias/type/scheduler cause has already been corrected and the
  FP tree is unchanged. TRY the direct derived expression and direct component
  reads together, removing earlier staging (`interp_v3`: `s = kOne - t`,
  then `a->x * t + b->x * s`). Correct mutable roots preserve alias-safe axis
  ordering; stale const-root trials do not rule out this simpler expression.
  Verify rounded versus fused operations and every aliased caller.

- Aligned payload start and final ADD share one initial-PCode pointer owner,
  but retail keeps the start until the ADD and returns a separate masked
  result: REQUIRE the real allocator width, valid arena and unchanged mask.
  TRY assigning the complete aligned address to the existing pointer before
  returning it (`privGetBlockFromUsedHdr`, partial). Retrace ADD versus ANDC
  destinations; a direct expression return can reassociate differently.
  Keep necessary address-mask conversions; do not reuse the exponent as a
  mask or add an artificial intermediate use. Evidence:
  [aligned-result report](reports/allocator-aligned-result-ownership.md).

- One-based encoded address subtracts after lazy bounds checks, but a separate
  offset hoists or a direct pointer return reassociates it after addition:
  REQUIRE unsigned encoded storage and unchanged null/absolute-address paths.
  TRY decode in the existing value owner and use one final return after the
  conditional (`get_script_string_arg`, partial). Keep the base's canonical
  unsigned representation until the final pointer conversion; intermediate
  pointer/integer casts can create a CSE owner and rotate homes. Retrace the
  final ADD: commutation may remain, and spelling it base-first can undo the
  subtraction ordering. No identity masks or propagation pragma to close it.

- Subtraction should overwrite its dying position input, but source retains
  the midpoint: REQUIRE native destination ownership and unchanged FP tree,
  load order and effects. TRY a real position-to-displacement accumulator,
  `dx = pos->x; dx = midpointX - dx;`, keeping midpoint calculation separate
  (`nav_get_unit_vector_to_nav_portal`, partial). A passive position alias or
  one-definition delta can propagate back into the midpoint owner; retrace.
- A narrowed call result stages through a codegen mask temp and a named-owner
  copy: REQUIRE one real normalization before any consumer or intervening
  effect. TRY assigning the full result to its actual owner, then narrowing
  that owner in place (`mk_chess_drone_random_piece_move_with_definition`,
  RNG roll). Verify the mask and copy in native output, all later assignments
  and comparisons, and the original call/read order; no second alias.
- Normalized axis products should overwrite the loaded components: REQUIRE
  retail's in-place multiplies and identical rounded/store order. TRY normalize
  the real component locals before publication (`v3_v3_to_quat`). Retrace:
  removing separate product webs can make a previously impossible component
  declaration order valid; old declaration-search ceilings no longer apply.
  Before perpendicular projection, the same rule applies to the existing
  separation components after their last raw-input use
  (`get_play_camera_position`). Preserve the output vector's entry initializer.
  A changed product-owner graph can also make retail's final multiplicand
  order viable after it previously regressed; require a fresh native replay,
  not blind repetition. See [the camera recovery](reports/camera-separation-normalization.md).
- Variable-length stream header and record cursor use separate webs, retail
  advances one pointer: REQUIRE proven header/record layout and native
  evidence that the header web lacks the loop cursor's interference. TRY one
  byte cursor across the actual boundary array, count header and record loop
  (`nav_which_area_is_next`); use `sizeof` strides and typed views only at
  each format boundary. Open the small accessor locally if its own mutable
  cursor survives inlining as a separate web. Preserve shared consumers;
  no shifted fake struct, magic offset, or alias used only to steer registers.
  Chained typed casts through a packed tail can also split the returned cursor
  and fold separate returns; direct byte increments can preserve its original
  web. Check return branches independently of remaining FP coloring.
- Squared-distance product takes a camera-coordinate FPR: REQUIRE native
  replay places the product's emission temp above the loop-invariant
  coordinate owners, while retail needs it below them. TRY squaring the
  existing displacement locals in place (`dx *= dx; dz *= dz;`), then sum
  them (`p_camera_wall_show_hide_alpha`). Verify two separately rounded
  products, the sum, and the original inclusive/NaN comparison; no operand
  reassociation or additional coordinate aliases.

- Increment published to state and reused after a call: REQUIRE retail keeps
  the advanced local in place and all uses denote the same index. TRY
  `state->index = ++index`; a separate unused post-increment followed by the
  store can propagate an add temp and recompute it after the call
  (`do_pfx_bleed`, GC/2.7). Keep the update, store and validation order.

- State getter/add/set, only ADD operands commute: REQUIRE retail keeps the
  loaded state as accumulator and getter order is preserved. TRY
  `state = GetState(...); state += delta; SetState(..., state);`, rather than
  overwriting the delta with the sum (`SpreadSheet::HandleAction`, GC/2.7).
  Removing an earlier single-use staging local can change later web order;
  remeasure previously neutral accumulator forms after that supported owner
  correction (N13). Do not introduce an extra copy or reorder getter calls.

- Angular displacement has both a raw-distance consumer and scaled update,
  but separate raw/step locals leave the wrong FP owners: REQUIRE the raw
  distance uses the pre-update displacement and preserves its exact sum tree.
  TRY one `Vec` displacement, consume its squared distance, then scale its
  components in place before publication (`p_pebble_path_monitor`, partial).
  Preserve each multiply's operand direction: `scale * delta.x` and
  `delta.x *= scale` can select different owners and schedules. A separate
  delta/step aggregate trial does not test this mutation boundary. Verify
  all stores and the completion predicate; no extra reads or dead consumers.
  Evidence: [pebble displacement report](reports/pebble-angular-displacement-ownership.md).

## H27

Statement order follows retail schedule `[da]`. IF loads, stores, or pointer
advances ordered differently, REQUIRE unchanged semantics (no alias between
moved accesses), TRY statements in retail order: compute packed words before
storing, advance cursors after stores, load node output ptr before size store
(mk-da `MCBlockDecDCNest`, `gop_decode`). Stack-member stores: H14.

- Paired position/UV streams retain scalar-owner rotation plus FP setup residue:
  REQUIRE identical pointed-store order, signed conversions, variable byte
  strides and unescaped local cursors. TRY advancing UV then position after each
  glyph, with the real horizontal-coordinate update before the second UV
  publication. Verify the complete arithmetic context too: open-coding the two
  single-use coordinate helpers, the existing UV declaration placement and a
  plain bias `if/else` were validated jointly in
  `set_letter_positions_and_values`; this does not prove each edit necessary.
  Do not add cursor aliases, move pointed stores or infer final data layout
  from matching instructions.

- A shared scalar initializer loads before the first vector component while
  retail loads the component first: REQUIRE two independent reads, one real
  shared factor, and unchanged products, stores and rounding. TRY capturing
  the factor at its first actual product, `axis.x *= (scale = *scales)`
  (`gc_generic_render`). Recheck the whole unit; this can restore emission
  order without solving saved-register allocation. See the
  [particle scaling evidence](reports/particle-scale-first-consumer.md).
  The same rule applies to a reused subtraction input: capture `center_z`
  in the first delta's right operand when retail loads the left operand
  first (`fat_bgnd_char_setup_radius_check`, partial). See the
  [radius-check evidence](reports/fatality-radius-first-consumer.md).

- A repeated traversal resets its cursor after a length call while retail
  sets it before: REQUIRE the same input pointer, call and later cursor uses.
  TRY the observed initialization order (`ScreenText::GetStartArray`). It can
  remove a result move by changing the split cursor's call interference.
  Bind zero-initialized counters through later uses before comparing their
  order; stop separately on remaining split/named-owner coloring. See the
  [text traversal evidence](reports/text-wrap-traversal-ownership.md).

- Fixed parameter argument and retained next-index initialization swap:
  REQUIRE a sequential parameter cursor and identical indices for every
  optional consumer. TRY starting the cursor at that query's index and
  postincrementing its argument (`SpreadSheet::ProcessParams`, GetBoolean).
  This expresses the real cursor advance and can emit the argument before
  the retained next value; do not couple unrelated counters or move calls.
- Independent source/destination indices increment in the wrong order:
  REQUIRE initial/pre-RA evidence of code motion around indexed publication,
  unchanged index values and no intervening observable use. TRY advancing the
  destination index at the actual store, `items[i++] = node`, in a structured
  while (`TextList::ProcessParams`). Keep the source-index increment after
  lookup; postincrementing the callback argument moves it before the call.
  A standalone loop-latch increment can lower differently; recheck all owners.
- Runtime address precedes a mode load in retail, but the scheduler hoists
  the outer-owner load: REQUIRE an existing runtime member denotes the same
  storage, with confirmed offset/type and a captured pre-RA dependency. TRY
  reading the mode through the selected runtime (`bank_run_fx`); this keeps
  address-before-load scheduling even when later address folding restores
  outer-base addressing. Do not invent an alias field or dependency.
- C++ reference publication precedes a required native-resource init:
  REQUIRE retail init-call/store order and unchanged member destruction.
  TRY sequencing the actual init in the reference initializer,
  `owner((InitResource(&resource), owner_arg))`, retaining POD storage
  (`mwFileMountPointPlugin` host/CD). A nontrivial wrapper may add destructor
  work; check every constructor/destructor. No dead effect or identity helper.
- Derived vtables precede scalar field initialization, but base initializers
  put those stores first: REQUIRE retail publication order and every base
  construction consumer. TRY initializing the actual protected fields in the
  derived constructor body (`mwFileDevicePlugin` host/CD), leaving base
  vtable construction to the compiler. Require no intervening observable
  call/read, unchanged layout/destruction, and all consumers nonregressing.
- Copy getter/setter, retail loads run ahead of stores (L0 L1 S0 L2 S1),
  ours alternate or swap first two loads: REQUIRE source pointer only read.
  TRY `const T*` source param (header + def), plain member copies; MWCC hoists
  const-source loads above dest stores. Staged locals, pragmas, compiler
  revisions don't reproduce it (`mwMemHeapGetInfo`, `mwMemHeapGetParams`,
  `mwMemSystemSetParams`; mk-da `mwMemSystemSetParams` same const form).
- Mechanism of the const form: M12 (alias fact, root pointer qualifier).
- Pure volatile-register rotation is often H27 too: codegen temps number in
  emission order, so a moved statement or operand changes colours with the
  same instruction stream (H15 volatile-temps bullet).
- Profile updates interleaved with unrelated state resets: REQUIRE distinct
  global objects, captured selector/amount, and no intervening calls or volatile
  accesses. TRY grouping both real updates before the resets (`p_pselect`);
  MWCC can retain retail's scheduled store interleave while changing temporary
  numbering. Preserve update order and verify relocated data values.
- Refund loop-free island retains an index snapshot before profile-address
  emission: REQUIRE exact native replay needs the table/index producers above
  that address, and profile writes cannot modify the selector or lookup table.
  TRY direct canonical currency indexing at both refund assignments after
  grouping refunds before resets (`p_wager_save_profiles`). Require one emitted
  lookup and unchanged stores/callbacks; do not duplicate observable reads.
- Final recurrence update has a split accumulator below zero/address/load
  temporaries: REQUIRE unsigned arithmetic, disjoint reset globals and no
  intervening observable call. TRY the complete recurrence expression followed
  by state resets (`reload_rnd_tbl`). Retail can schedule the resets before
  the final store; emitted order does not prove source order. Moving resets
  alone may fail while retaining an evolving accumulator. Check the combined
  expression boundary and verify both loop bodies and relocated constants.
- Invariant palette homes differ after explicit pre-loop byte snapshots:
  REQUIRE a read-only palette, retail byte-store widths and a real existing
  vertex-write site. TRY reading the palette at that site and let MWCC hoist
  the loads; unsigned character access may express packed color object
  representation without changing shared channel types. Early snapshots can
  create different emission webs (`render_konquest_collision_obj`). Verify
  every helper consumer; do not add aliases or change qualifiers to force it.
- Loads hoisted in an order != store order, ordering/const forms neutral:
  scheduler priority = deadline, uncovering, height, pre-RA class (`mr` <
  store < ALU/`stwu` < load < `li`/`lis`), source order. Retail order from
  uncovering = some store has an extra predecessor (volatile chain, alias
  edge). REQUIRE semantic evidence before any `volatile` (ISR-written global,
  no fake volatile). Check destination decls before statement order
  (`gc_native_display_pass_to_RW`: ISR-written RW XFB pointers `volatile`).

- Independent stores in swapped order flipped functions exact:
  `atm_demo_fight` (store before timer-off zero),
  `gc_mem_card_status_changes_for_one_device` (compound assigns + publication
  store order), `SpreadSheet::ScrollDown` (`position_in_window = unkDC - unkD4`
  before the total-row test). Try early, before coloring work.

## H28

Single-use local folded into its use `[da]`.

- IF a nonvolatile/arg home differs; capture/pcode shows a named local missing
  from the web list while its value feeds the use directly (switch selector,
  index, latch result). Retail numbers it as a low named web popped late; ours
  is a late emission temp popped first. Decl-order sweeps neutral: a folded
  web cannot be reordered.
- REQUIRE the local assigned once from a plain load or inline result, used in
  one statement.
- TRY (1) enum type for opcode/selector/register-index locals: the int->enum
  conversion blocks the fold, emits no code under `-enum int`; then declare
  the lowest-wanted web last. (2) Null-initialised result consumed by a
  latch `?:` inside an inline helper: put `= 0` in the caller (zero variable
  must live outside the inlined helper) and latch with `MK_LIVE` at the call
  site (retail `li 0`, not `mr` copy of the helper's own zero web).
- Not blocking: int/unsigned/const/short/pointer typing, block scope, init vs
  assignment, wrapper inline, second use (adds code).
- Enum retains the selector but its owner-base load swaps with an independent
  coordinate load: REQUIRE native replay predicts reversing those two emission
  temps, with no call/store or data dependency between the reads. TRY computing
  the real coordinate before the selector snapshot, keeping later reads and
  case order fixed (`mk_chess_cursor_square_move_next_step`). Backend scheduling
  may still put the owner load first; verify the emitted order. Merely moving
  the enum declaration need not change either emission temp.
- Exemplars: `_compare_float_float` + 4 `_compare_*`/`_combine_*` siblings,
  `pfx_bind_emitter_num_to_new_obj`. Open: wrapper local from inline call
  result (`start_blood_particles_scripts` 99.43).
- Find the web by gdb interference-graph capture (tier 4 hard stops), not by
  sweeps.

## H29

Constant owner kind: named local vs inline/temp. IF retail has two
independent `li rA,imm; li rB,imm` (same immediate) in one block and ours has
`li; mr` (or reverse), REQUIRE mwcc_trace `initial` shows two separate LIs and
`presched` shows the MR, no call between them in the extended block. TRY make
the earlier constant's owner a real function-level named local at its FIRST
text-order use: open-code the inline helper that owns it, or give each reset
index a distinct block-scoped name. To get sharing instead (`li; mr`), make
the owner an inline-helper local or a later use of the same variable. Never
add a dummy zero local.

- A one-object reset shares phase zero with an SR offset, or gives an indexed
  row store the wrong base/index ownership, while an exact sibling resets
  several objects per group: REQUIRE the same typed operation,
  reset/store order and authoritative reloads. TRY reuse the complete shared
  two-dimensional helper with the real object count, updating every caller
  (`pz_fighter_chomper2_round_over`). The same helper must serve genuine
  multi-object work; do not invent a one-trip loop. For zero sharing, check the
  native coalesce window: the first SR register can sit on its inclusive
  endpoint, while the recovered inline owners move both offsets outside it.
  For an indexed store, verify its actual base/index roles and r0 restriction;
  do not add a row-pointer alias. Verify all consumers
  and unit data. Evidence: [shared reset](reports/chomper-shared-reset-count.md).

- Complete destruction helper restores LI/MR but moves subsequent clear-loop
  setup: REQUIRE unchanged list destruction, reset stores and clear extents.
  TRY one complete following reset operation owning both related clear loops
  and their shared real index (`ncs_bgnd_nuke_collision_to_script_interface`).
  Isolating only the first clear loop can displace the second setup instead.
  Verify both loop preheaders, other helper consumers and unchanged BSS layout;
  do not move independent resets or introduce a synthetic zero/offset owner.

- Mechanism (GC2.7): CSE-mode value numbering (`ValueNumbering.c`, PC_LI /
  PC_LIS) records and matches an `li` only when its dest vreg lies in
  `[gGPRCoalesceFirst, gGPRCoalesceLast]`. `CodeGen.c` numbers real named
  locals first, opens the coalesce window, then numbers every `@`/`$` object
  (CInline renames all inline locals/params to `@N`; IRO CSE temps too), then
  expression temps; window closes at codegen end. A named local is below the
  window only at its FIRST text-order use (later uses become IRO `@` temps).
  Hoisted zero + inline-local index both in the window and in one extended
  block -> `mr`.
- CSE never shares across a call (`bl` kills the chain). Registers made by
  strength reduction / CTR conversion usually lie above the window, but its
  upper bound is inclusive: the first new SR register can equal the old
  `gGPRCoalesceLast`. Measure the actual bounds before classifying two LIs
  beside `mtctr` as unavoidable.
  In `ok_to_bring_out_wager_screen`, byte index59 is eligible at that bound;
  a complete six-currency scan inline moves its accumulator into the same
  window and restores LI/MR after the mode gates. The same inclusive-bound
  case occurs in `p_update_effects`: a complete bank-update scan makes its
  real index eligible alongside the first SR offset. Check captured bounds;
  a new SR owner is not automatically outside. Preserve latch validation,
  callbacks and both coin snapshots where applicable; never seed the index
  from an unrelated value or add a byte-offset carrier.
- Opening a width loop recovers independent zero loads but moves its addressed
  glyph-width scratch: REQUIRE original font walk, zero producers and scratch
  slot are verified. TRY caller-named line/maximum widths with a complete
  glyph-advance operation owning the real OSGetFontWidth output and width
  accumulation (`render_text`). Keep the mutated cursor separate from the
  retained input string and initialize it at the original argument boundary.
  Retrace counter order; do not add a dummy zero or move the addressed scratch
  into a fake padding record.
- Several width scans: REQUIRE the glyph-owned scratch preserves every width
  and GXColor stack slot, but caller width blocks alter independent case-loop
  counters. TRY keep the actual width state with caller text state and give
  the case loops separate indices inside their existing branches
  (`display_dragon_with_text`). Branch indices alone may be neutral. Verify
  all width seeds and counter homes together; do not manufacture nested
  scopes, index aliases or zero carriers. See the
  [dragon width report](reports/dragon-width-accumulator-ownership.md).
- Only one LI in `initial`, two after SR: REQUIRE the new stride register is
  eligible for constant sharing but the original index is a first-use named
  owner below the window. TRY recover the complete original inline phase
  when retail symbols support it. In `load_and_init_konquest_map_specific_data`,
  native GC2.7 `no_dup` (`0x509b00`) saw index v39 below `[43,143]`, while
  the new tile-stride v143 was inside. Retail MAP's discarded
  `shadow_exclusion_system_init` (`konquest.o`, size `0xA8`) owns the full
  art lookup, collision-list loop, generation and strength initialization.
  Recovering that phase makes its index an inline owner and restores LI/MR.
  Its three saved owners form a clique; captured inline declaration order
  `tile_index`, `shadow_art`, `tile` gives r28/r29/r30 and closes the function.
  Keep typed tile indexing and callback-time base reloads; do not introduce a
  byte-offset local, dummy zero, or one-use getter to manufacture the copy.
- Loaded-library portrait traversal: REQUIRE the original first-use named
  index is below the coalesce window while its SR stride is eligible, and an
  existing parallel model helper supports the complete search/create phase.
  TRY that complete inline phase with the static API delegating to it
  (`mk_chess_create_portrait_from_library`); its index can supply the stride
  zero before parameter moves. A construction-only helper leaves traversal
  ownership unchanged. No offset local, dummy zero, alias or propagation flag.
- Reset index and pointer clears need one zero: REQUIRE trace shows a named
  index LI outside the coalesce window and separate pointer-zero LIs inside.
  TRY one inline owning the complete arena reset, including traversal and
  counters (`hdrlessHeapResetHeap`). Its real index can supply the clears
  after value numbering; never cast the index to a null pointer in source.
  Inline locals number opposite named locals: trace before adjusting their
  declaration order, and check nested alignment operands and all consumers.
- Same final LI count, wrong loop-preheader position: REQUIRE native capture
  distinguishes the dead source-index zero from the surviving SR zero, and
  retail MAP supports a complete inline loop phase. TRY recovering that phase
  (`npc_update`: `npc_manager_update_model_list`). Its inline index may supply
  the SR zero through VN and coalescing, changing which LI survives and where
  it schedules. Check manager-versus-element CSE ownership too (H21); this
  boundary can fix both. Do not infer a scheduler-only cause from equal final
  opcode counts or introduce a zero-carrier local.
- Reset helper restores LI/MR but rotates cursor homes: REQUIRE its native
  inline-owner creation order, with the complete reset's guards, traversal,
  links and counters preserved. TRY declaration order matching the desired
  inline numbering (`ListPoolAttach`: node, predecessor, data, index), then
  recheck every home. A helper regression alone does not disprove the boundary;
  never add a dummy shared zero or couple pointer/index values with casts.
- A complete manager phase restores the first LI/MR but rotates caller list
  owners, while the second loop must keep LI/LI: REQUIRE original MAP helper
  names, native inline origins and complete interacting operations. TRY
  restoring the manager/model-slot, startup-script and visible-list phases
  together (`start_running_npcs`). The index and slot then have genuine inline
  ownership while list walks no longer compete as caller-named locals. Check
  both zero pairs, every saved home and the whole unit; preserve manager
  reloads across callbacks. No whole-function wrapper, synthetic zero,
  auxiliary output or invented helper contract.
- Loop helper restores LI/MR but makes a preceding selector branchy: REQUIRE
  the loop is one complete operation and the caller already computes the
  selected bank. TRY pass that actual bank to the typed inline loop, keeping
  the original selector at the call boundary (`setup_plyr_anims`). This can
  retain branchless bank selection while making the loop index an inline
  owner. Verify selector lowering, loop stores and all siblings; do not add
  masks, dummy zeros or synthetic stride locals. See the
  [player-style bank report](reports/player-style-bank-loop-ownership.md).
  Nested window accessors can still leave payloads as IRO return temps below
  those owners. REQUIRE proven complete-object window/stride and native kind
  evidence; TRY the same offsetof-backed reads inside the cleanup operation,
  then its measured real-owner declaration group (`movie_player_reset`). Keep
  overlapping storage honest; do not invent arrays or change callback order.
- Selective cache reset: REQUIRE the first-use named index is outside the
  window and its SR byte offset supplies separate zero copies for field
  stores. TRY an inline owning the complete invalidation walk; its eligible
  index can supply both clears while the SR offset takes one MR
  (`mkScreenEngineClient::UnloadScreenSet`). Preserve entry tests, field-store
  order and untouched cache contents; no invented zero-carrier local.
- Both zeros belong to one pointer/instance latch: TRY the existing canonical
  `MkHdrLatch`, reset its fields together before creation, capture `&proc->hdr`
  and its instance, and validate with `MK_LIVE`. Scalar-replaced fields become
  `@` owners inside the window: confirm two LIs in `initial` become LI/MR in
  `presched` (`konquest_hero_portal_in`). Keep actual null-check and call/store
  order; do not invent a record for unrelated constants or add a zero helper.
- A nullable-owner latch keeps the initial result zero caller-named, while
  retail copies it into both null arms: REQUIRE native `initial` retains the
  separate zeros and retail MAP plus a matched sibling support the complete
  accessor. TRY recovering that accessor, including its owner snapshot, null
  guard, result initialization and latch (`get_tile_sobj_by_id`,
  `get_konquest_tiles_obj`). The inline-owned result can supply the null-arm
  copies through VN and fix their interference. A partial latch helper leaves
  the initial zero outside the window; no dummy zero or identity wrapper.
- Allocation failure and timer expiry enter one clear loop, while other paths
  continue the caller's state machine: REQUIRE first-use named indices and
  measured structured alternatives; TRY caller-owned traversal. Local forward
  clear/continuation labels only under AGENTS.md's last-resort exception
  (`puzzle_fighter_mode_play__new_piece`); do not materialize a dummy decision
  solely to keep the helper boundary.
- Exemplar: `p_create_profile` (PIN-entry loop open-coded at both sites,
  block-scoped `int i` per cancel branch; `scan_profile_code` declares
  `cursor` before `i`; dead helper deleted). Same lever: `cftyp422_ppc`
  chroma loops (named `cb`/`cr` row owners, `luminance_sample` declared after
  its row).
- Classify first with tier 4 Trace procedure: if the `li` exists in `initial`
  as two LIs, this rule applies; permuter/decl sweeps cannot change value
  numbering.
- A null search result owned by an inline can also supply a stale-node header
  clear, deleting retail's separate `li 0`. REQUIRE the trace shows both LIs
  initially and the header store reusing the result in `presched`. TRY a
  caller-owned typed traversal result; keep the existing-or-allocate helper's
  early return if retail has its shared-exit branch. Fully flattening that
  phase changes the CFG (`mk_chess_calc_all_attackers_in_turn`).

## H30

- Repeated inline operations need interleaved workspace slots: REQUIRE native
  stack-object provenance shows each expansion grouping its scratch objects,
  while retail groups both small intermediates before both larger workspaces.
  TRY nesting the complete operation that owns each intermediate inside the
  repeated helper (`cloth_coll_point_cyl_inside`: scaled axis projection inside
  the MAP-named point collision helper). Later nested expansion creates both
  intermediates after the outer locals, placing them in the lower stack slots.
  Preserve vector sizes, alignment, call order and alias-sensitive stores;
  no padding or scratch-only parameters. If only the first pointer homes remain,
  trace the actual input owner: passing a sole-use field address directly can
  give the outer inline argument its correct rank above the nested argument.
  Verify actual slot resolutions and every emitted function before linking.

- Independent factory results reuse the wrong saved homes beside an inlined
  cache walk: REQUIRE exact native replay and a real allocation followed by
  guarded base construction, derived-vtable publication and member setup.
  TRY nesting that complete allocation/construction operation inside its
  factory (`mkScreenEngineClient::CreateElement`). Keep resource lookup and
  later configuration in the factory. A name obtained through an expression
  accessor may also need direct access through its canonical table member so
  it remains the factory's inline local. Measure the pair and the standalone
  emitted factories: their register allocation can change even when every
  scored consumer improves. Verify actual statement-expansion ancestry; a
  forwarding wrapper or invented allocation-result alias is not evidence.

- A strength-reduced search index swaps with its captured owner-slot offset:
  REQUIRE native replay identifies the surviving inline index root and the
  same count snapshot, repeated slot dereference and Boolean-return CFG.
  TRY nesting the complete roster search beneath the wrapper that captures
  the typed side slot (`mk_chess_handle_guy_falling_from_sky_event`). The
  nested index can number below the outer offset; changing the declaration
  within one expansion does not cross that boundary. Diagnose coalescing
  roots as well as newly generated induction registers, and verify all
  consumers. Do not cache the roster owner across its observed reloads.

Inline nesting depth. IF several saved homes rotate and a gdb capture with
object names shows a top-level inline helper's `@` local/param popping before
(numbered above) a value retail colours later, or an inline param claiming a
colour the target needs (sweep-2 pop order), REQUIRE both objects are `@`
inline objects of statement-level expansions. TRY move the earlier-expanded
helper call inside a wrapper inline: nested expansions run in a later
`inline_statement_list` iteration, so their `@` objects number BELOW every
top-level expansion. Lift a nested call to top level for the reverse.

- Numbering law: vreg order = reverse object-creation order. BE web splits <
  IRO CSE temps < `?:` temps < nested-inline objects < top-level inline
  objects (reverse call-site; params reversed, locals forward) < pool base <
  codegen temps. Definition order and static-vs-inline do not change it.
- Handle typing: an int handle passed to an unsigned API makes IRO CSE the
  int->unsigned conversion into a CSE temp (changes web kind); match the
  handle type to the API (`unsigned int dust; int fountain;`, H21).
- Why decl/helper-spelling sweeps miss it: they only move objects inside one
  kind group; the fix moves OTHER helpers' objects (display_prize `entries`,
  rebuild col/row) below the award/dust objects.
- Exemplar: `p_move_camera_and_open_coffin` (wrapper `show_coffin_award`,
  nested `place_visible_coffin_rows_from`, `award_koins(void)` owning entry;
  25 rows -> exact; 24 attempts and 3000+ permuter candidates missed it).
- Lift for address-taken locals: IF two expansions of one helper show swapped
  stack slots for an address-taken local (InvSqrt union scratch, GXColor),
  REQUIRE retail's slot order = expansion creation order. TRY write the body
  out at the call site (depth 1) instead of through the helper. Nesting moves
  these locals to the LOWEST slots (GCSpecSkin GXColor 0x38 -> 0x14 regressed):
  if retail pins them, rule nesting out. Exemplar:
  `sh_update_blood_fall_pebbles` (both normalizes open-coded, inverse-first
  products, `floor_y` local; 48 rows -> exact). Nest never helped when the
  swapped values were call results, loop/emission temps, or inside one helper.
- First expansion shows raw order: the first expansion of a repeated helper
  takes fresh saved registers in pop order (exposes the helper's internal
  numbering); later expansions reuse the lowest free register. IF only the
  first call site rotates after the others match, a helper declaration swap
  just moves the error to another expansion; TRY treating the first call site
  as structurally different in retail (`mk_chess` schedule_bonus_row,
  `GCSpecSkinLighting` upload_point_light).
- An upload's index/object pair differs, while whole-upload nesting moves
  correct color slots: REQUIRE exact native replay and the same index snapshot,
  geometric initialization, color publication and final mask/index updates.
  TRY nesting the complete object selection plus attenuation/position
  initialization beneath the existing upload, returning the initialized object
  (`GCSpecSkinLighting`). Keep normalization and `GXColor` with their actual
  owners. If point-only nesting fixes point uploads but rotates directional
  uploads, require a fresh full replay of the first saved-register claim; apply
  the same real geometry operation to directional uploads only when it realizes
  the observed owner ordering. Preserve post-callback light-slot snapshots and
  every stack slot; a pointer-only getter or extra lifetime is insufficient.
  See [the geometry ownership case](reports/gcspecskin-light-geometry-ownership.md).
- Center arguments and an initializer's width rotate in one expansion:
  REQUIRE exact native replay needs the width owner below both centers.
  TRY a complete outer presentation operation owning screen-center snapshots
  and the delayed announcement, with the existing width-owning initializer
  nested (`p_mk_chess_game_over`). This crosses outer-local versus nested-local
  creation groups; parameter permutations inside the initializer cannot.
  Preserve signed screen division followed by unsigned coordinate conversion,
  escaped pdata reloads, and image/setup/sound order. Check both expansions;
  a forwarding-only wrapper does not establish an operation boundary.
- With four timeout placements, move both actual center calculations into
  the outer screen-centered operation and pass X/Y into the nested typed
  initializer (`mk_chess_timeout_msg`). Moving only Y leaves X beside width
  and can rotate previously exact cases. Keep image loading in its caller to
  preserve its owner. Preserve all four
  allocator-owned out-slots. Verify every expansion rather than accepting
  one repaired pair.
- Unload cursor alone takes a dead sound-pointer home: REQUIRE trace shows
  sibling stop/unload expansions and retail preserves the same effects. TRY
  one complete stop-and-unload helper owning the unload loop, with the
  active-sound walk nested (`mslBankUnLoad`). A wrapper delegating both loops
  may be neutral. If combining rotates the callback's bank/response owners,
  REQUIRE a fresh full replay including the saved guard and both loops'
  index/cursor/sound owners. TRY flattening the stop walk into that complete
  operation and its predicted real declaration group
  (`mslBankLoadResidentARamUploadComplete`). The guard's scan position can
  change its simplify sweep and displace the persistent owners; a cursor-only
  swap misses other claimers. Preserve every initialization, callback reload,
  clear and list-advance boundary, and check both entry points. Remove
  superseded unused helpers only after retail/emitted-symbol absence checks.
- Earlier dequeue scan owns a top-level inline index above later reaction
  owners: REQUIRE graph provenance for the earlier claimers and a real active
  record consumed after dequeue. TRY one complete dequeue-and-retrieve inline
  returning that record, with the existing queue mutation nested
  (`pz_fighters_handle_next_pending_move`). Retrace: this can expose a second
  selector/local order constraint; keep all queue copies and event fallthroughs.

## Traps

- `#pragma optimization_level N` == `-opt level=N` (frontend + backend),
  never an IRO-only knob; function-level pragma changes the whole pass set.
- ELF `NOBITS` = zero storage; never read `sh_offset` as initializer before
  "fixing" data-value mismatch.
- Auto extraction must parse C identifiers (`0.0f * body` is not a pointer
  decl); reject malformed generated source.
- No host (PC) branches in Deception source; portable fixes need retail +
  behavior evidence.
- Renaming locals never changes MWCC coloring `[da]`. "Neutral across decl
  orders" is not a name effect.
