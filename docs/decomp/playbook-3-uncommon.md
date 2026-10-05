# Playbook tier 3: uncommon (M rules)

Localized lowering, compiler modes, ABI shape, inline/macro boundaries,
data/link layout. Use after [tier 1](playbook-1-core.md) triage + matching
[tier 2](playbook-2-common.md) section. Pick by mismatch, not past score.
M03/M04 spelling trials rarely close new things; only with a concrete new
hypothesis. `[da]` = mk-da import (tier 1).

Compiler settings (M01, M13, M15, M16):

- Object-scope flag in `configure.py` beats per-function pragma when whole-TU
  control passes.
- Pragma scoped to measured consumer, reset after.
- Necessity control: same source without the setting.
- Compare every function + section vs pre-change report.
- Result never proves retail pragma spelling.
- Accepted TU flags stay fixed during fixed-TU-setting tasks.
- Flag changes need two independent objects agreeing before lib scope; one
  function's flag lead stays a note `[da]`.

## M01

Compiler mode across a TU. IF compact saves, `divw`, Boolean lowering repeat
across TU, REQUIRE sibling evidence + all-function and section baselines.

- TU profile: object-wide `-O4,s` + existing `-use_lmw_stmw on`; test
  scheduling separately. Compact saves + indexed-loop lowering together -> try
  profile before changing loop source; `-O4,p` -> `-O4,s` alone closed
  credits text.
- `_savegpr`/`_restgpr` vs retail `stmw`/`lmw` in several functions of one
  lib `[da]`: lib-scope `-use_lmw_stmw on`.
- Library scheduling residue `[da]`: several functions in one lib exact in
  body, differ only in prologue/epilogue/load scheduling (or keep CTR loops
  and folded sign extensions retail lacks). REQUIRE independent proofs from
  different objects that `scheduling off` / `optimization_level` closes them
  with unchanged honest source. TRY offline sweep of `-O` levels over every
  lib function (host pre-screen), apply level with zero regressions at lib
  scope (mk-da RW `-O2,p`).
- Retail copies arg into retained reg, then masks/loads via copy; ours
  forwards original: REQUIRE sibling evidence (separate mask/compare instrs)
  + compatible same-lib settings. TRY `-opt nopeephole` as whole-TU control
  (`get_field_offset`, libmkparticle). Compare every function + data incl jump
  tables; reject any regression. Control pragma = `#pragma peephole off`;
  `opt_peephole` is unrecognized on pinned GC and gives fake neutral
  (`TRKDoSetOption`). Verify control active. Land supported mode at object
  scope, drop scratch pragmas, verify full build + SHA-1. Permuter-found
  per-function pragma = evidence, not a fix.
- Signed /2 as `srawi`/`addze`: function-scoped `optimize_for_size`,
  arithmetic unchanged.
- Isolated int-reg save mismatch, identical body: `optimize_for_size` +
  `use_lmw_stmw` together (projectile sound setter). Retail
  `_savefpr_29`/`_restfpr_29` with matching scalar FP: same local profile
  restores compact FPR helpers (`fxsys_set_v3`, `pfxscript`). REQUIRE actual
  helper calls, immediate push/pop, every sibling compared; don't regroup FP
  math to fix save-only diff.
- Typed vector copies preload where retail interleaves loads, stores, owner
  reloads: consumer-scoped scheduling off. Reject if epilogue restore order or
  aligned-frame setup regresses (ceiling, not body-only match). Sibling leaf
  vector math repeating component load/op/store: scope scheduling mode with
  push/pop, verify every sibling (`v3_add_v3`, `mk_math`); whole-unit mode can
  regress others.
- Folded indexed accesses (`lwzx`/`stwx`) where retail does mul/add/`lwz`:
  real category/base locals + consumer-scoped `opt_propagation off`; both
  needed, pragma alone may do nothing (move-list getter). Pragma also keeps
  every inlined helper's param/return copies in that consumer; if those
  recolor an exact region, make helper copy-free (H21), don't drop pragma.
- FP const load or pooled string address moved across selection branch:
  propagation off, tested apart from scheduling + CSE (AI avoidance reset,
  Puzzle burn controller).
- Global pointer reloaded after loop, ours reuses loop test's reg:
  function-scoped `opt_common_subs off`, immediate reset, whole-TU compare.
  Propagation off was neutral for `kabal_collide_victim`; CSE off restored
  `his_pdata` reload. Reload after inlined float-bits helper: pragma
  reproduces reload but renumbers volatile webs (`drone_ai_victim_avoid` stuck
  99.44); address-taken helper input (H05, H21) closed it with CSE on, no
  pragma.
- Multi-row field, retail adds offset to reloaded owner before `stfsx`: typed
  row-array pointer + propagation off (Puzzle crusher).
- Fixed-count array copy as two advancing pointers, retail one byte
  induction reg: scoped `opt_strength_reduction off`. Loops only.
- Size bit: `-opt off` TU lowers `ptr != 0` as `neg/or/srwi`, retail `addic
  r0,x,-1; subfe`: `-O4,s` before `-opt off` at object scope when same-lib
  siblings build that way. `,s` survives `-opt off`; no spelling gives the
  carry idiom (`rwfexist`). Higher exact count can hide one lost function;
  compare each.
- Dead ends `[da]`: compiler version sweeps never fixed a coloring residue;
  `-opt level=3`, `-schedule on`, `nocse`, `-O3`, `-O4,s` fixed nothing on an
  `-O4,p` lib (two regressed).

## M02

Control-word or publication order. REQUIRE retail accesses + alias
boundaries. Load control word before subfield writes; publish owners at
observed point; reload counts after aliasing stores. Process/script transfers
observe published state; never move a store past a transfer for permuter
score. Bounded candidate loops: check whether each trial publishes before
validation calls.

- Saved 64-bit minimum read, lower selected, both words written even when
  unchanged: typed minimum local, conditionally replaced, always written back
  (`sfmps_CopyAudio`).

## M03

FP operands + schedule. REQUIRE same math, grouping, rounding.

- Inverse-sqrt guard differs by CROR or operand reversal: trace unordered
  outcome + negative/zero inputs. `x <= 0` != `!(0 < x)` for NaN; keep retail
  predicate (`normalize_v3`). Refinement: keep rounded product tree, scale
  existing estimate at real phase boundary before adding factor temps.
- Normalize reloads first component after refinement, retail keeps it: TRY
  `v->x = v->x * inverse` before snapshot local; MWCC can keep member read for
  explicit assign while reloading for compound (`normalize_v3`). Verify full
  FP order + all inverse-sqrt consumers.
- Swap only commutative operands, or name real factors. Drop temps only
  without reassociation. Keep polynomial-before-sqrt order.
- Separate single-precision boundaries: name intermediates in eval order
  (`ADX_GetCoefficient`).
- Rounded product before add: explicit float conversion; verify output bits.
- Mixed fused + separate ops: narrow function-scoped `fp_contract`; lexical
  toggles inside a function may do nothing. Retail `fmuls` + `fadds` where we
  emit `fmadds` and rest of lib needs contraction on `[da]`: each product in
  its own `f32` local before the sum, not a flag change (`VectorMultVector`).
- Vector normalized before 2nd vector's deltas: REQUIRE retail multiplies
  before delta subtracts; name normalized components there
  (`mks_get_victim_to_tr_dot`).
- Constant width from `lfs`/`lfd` + pool bytes, not decompiler casts.
- 3-component midpoint, right separate mul/add, paired input loads or FP webs
  differ: REQUIRE same difference, scale, add rounding. TRY typed inline Vec
  output helper with explicit sub, scale, add phases
  (`mk_chess_activate_piece_properties`). Folded expr != decimal spelling:
  `3.0f * 0.075f` = 0x3e666667, `0.225f` = 0x3e666666.
- Check whether negation is in FP before conversion or on the int.

## M04

Compare boundaries + Boolean diamonds. REQUIRE equivalent bounds, pure
operands.

- Try equivalent thresholds, ternaries, guarded assigns for observed join.
  Bitwise Boolean evaluates both operands.
- Final `cmpwi` with no consuming branch before void epilogue = terminal
  return guard. REQUIRE real validity/capacity condition + same-compiler
  reference with this lowering; try guard before forcing dead instr
  (`start_kabal_smoke_pfx` 10-slot emitter capacity; exact
  `bgnd_append_texture_to_material` same shape).
- `subic`/`subfe` normalizing call result: Boolean local (`opened =
  get_coffin_bit(...) != 0`). `subfic`/`cntlzw`/`srwi` field queries:
  explicit inline success/failure return. Ternary can match standalone but
  fold to `cntlzw` once inlined (`SFLIB_CheckHn`); explicit `if` returns keep
  branches.
- Identical calls before different actions in separate retail arms stay
  branch-local.
- One equality dispatch with conditional jump + unconditional default:
  one-case switch with default.
- Returned signed compare operand order (`a > b` vs `b < a`) differing only
  in load homes: try once. Not for FP unordered.
- Quotient once + guarded subtraction of end padding: keep count, guard the
  subtraction (`ADXB_ExecOneAdx`).
- Bounded copy: donor-style min expr + dest pointer set right after chunk
  acquisition (`sfadxt_CopyData`).
- Small-request dispatch where MWCC tests mutating case first: `if (request
  != no_op) { if (request < no_op) return state; if (request < next_limit)
  state = next_state; } return state;` (`sfply_StatPlay`).
- Signed 64-bit clamp: `value = value > 0 ? value : 0`, one clamp per rebuild
  (`sfmpv_DecodePicAtr`).
- Clamp default const hoisted before compare: H22 ternary.

## M05

Int-to-float scaffolding: natural cast with proven signedness; remove fake
0x4330 `volatile` machinery. Real bit reinterpret = supported typed union
(N10).

## M06

Leading stack byte updated, whole word passed: byte-bitfield/word union in GC
big-endian layout (`init_pwr_bars` flip word = 0x20000000, not 0x20).

- Mixed `lbz`/`lhz`/`lwz` at one base: typed byte/half/word union only with
  confirmed alignment, extent, endianness. Each consumer keeps its width; word
  clear then byte bitfield updates needs word view.
- 8x8 coefficient block cleared with double stores, decoded with Float32
  stores: donor Float64 dest + localized Float32 casts, not union
  (`MPVABDEC_IntraBlock`).

## M07

Aggregate + packed addresses. REQUIRE stride, extent, ownership, access
width.

- Typed element/subobject ptrs: trailing table at `&items[count]`, payload at
  `header + 1`. Byte offset != element index: `lhzx` with `(bits >> 10) &
  0x3FFE` = C index `(bits >> 11) & 0x1FFF`.
- Retail adds `index*stride` before field loads: `owner->array[index].member`,
  not cached element ptr (`mwPlyGetFxType`).
- Stack vectors swapping roles: trace each dest through all calls (difference
  vector updated in place into midpoint).
- Localized-array base before index getter, then advance.
- Uniform stack displacement: adjacent-version source or same function in a
  donor decomp may show a real unused local (`__DSPHandler` 4-byte array before
  `OSContext`; mk-da `STACK_PAD_VAR` copied from Pikmin's same function).
  Restore only that verbatim decl, cite donor, no fake reads, exact retail
  frame. Never extend to other functions.
- Contiguous run of copied fields can be one embedded aggregate
  (`SFADXT_Destroy`, 0x1C param block).
- Fallback-prefix bytes: read `header[-2]`, `header -= 2`, then `header[1]`
  (`SFHDS_SetHdr`).
- "Overlapping" union arrays may be adjacent regions
  (`MPVTransformWorkspace`: six 0x80 DCT blocks then six 0x100 float blocks);
  counters in opaque gap belong to param record.
- Records filled with one sentinel by direct stores: init fields in loop, not
  aggregate copy (`MPS_Create`).
- Sentinel lookahead: current-sentinel local, advance index after body, then
  load `records[index].sentinel` (`p_lightning_strike_effect`).
- Run retail table init before comparing decoder lookups (Sofdec run/level
  tables biased -16/-32/-32 bytes).

## M08

Failure edges bypassing shared success test. REQUIRE dominators + exactly-once
cleanup after ordinary guards fail. Structured do/break region only for real
cleanup edges, or real allocation-result Boolean. No dummy status, one-trip
`for`, duplicated effects. Donor-backed shared-exit `goto` needs AGENTS.md
exception. After callback allocation loops, `index == count` doesn't prove
success if callbacks change `count` (`mslInit`); keep correct behavior.

## M09

Repeated tests around stores: separate guards. Nullable-list inline predicate
only for proven contract, not `&global != 0` residue.

## M10

Unexpected masks, pairs, arg copies. REQUIRE all callers + callee storage/
return ABI. Recover full-width or byte ABI, typed conditional arms, aligned
u64 pairs, by-value POD, real POD return. Low r4 return can be low u64 half.

## M11

Wrapper load order differs, regs right: recover float/int param interleave
(PPC fills GPRs + FPRs independently) in defs, decls, every caller. Local
snapshots alone optimize back. No incompatible per-TU decls to hide caller
residue.

- `script_functions.c` wrappers: retail `current_args` slot load order = callee
  param order. Reorder callee params to ascending slot order, drop fake
  `current_args` / `void* script_args` params, update every decl + caller.
  Callers must stay neutral: `got_hit_fx` callers regressed, so its int-first
  order is real. Wrapper calling `get_animation` first rereads `current_args`
  after (`_launch_n_land_ani`, `_two_player_animation`).

## M12

Alias analysis changes access schedule. REQUIRE real mutability + callers.
Drop unsupported `const` or restore supported `const`; `const` alone doesn't
prove non-aliasing.

- Coordinate loads moved before intervening stores: compare input qualifier
  with mutable caller objects (Krypt position inputs, Puzzle flesh path).
- Tables/counts reloaded after global stores: drop unsupported pointee
  `const` per input, update decls + callers (`SFD_SetMpvParaTbl`).
- Read-only slot table at private helper: donor mutable-slot signature at that
  helper only, explicit const-removal cast at call; never write through it
  (`sftrn_BuildSystem`, `sftrn_BuildAll`).
- Separately cached dest owner = H05.

## M13

Canonical macro/inline expansion missing. REQUIRE definition + repeated retail
expansion. Typed inline helper beats macro when macro keeps a ref swap or
shifts stack offsets. Typed macro only when its lvalue + per-expansion locals
are proven. Shared headers only for proven ownership; unused O0 param takes
`#pragma unused`, not wrong prototype.

- Repeated block: one `static inline` at each repeated site
  (`game_count_active_players`); leave already-exact spelled-out sites.
- Fixed-record search: keep donor element count + induction (SFH 26-record
  helper); one cursor advance per record when retail advances pointer.
- Helper contracts that closed functions (no emitted helper symbol):
  - typed interface snapshot inside each private lifecycle helper
    (`MWSST_Destroy`)
  - cached backend as 2nd input to validity helper (`MWSST_Reset`)
  - byte classifier, typed unsigned accumulator assigned per arm
    (`sfh_GetStmType`)
  - typed inline lookup + standalone null check, not `||` (GOP readers);
    reject trials losing jump-table relocs
  - status-returning inline with out-params (`sfadxt_ExcludeSilence`)
  - restore helper with separate null guards (`SFMPV_Seek`)
  - first-slot clear + bounded slot loop (`SFD_SetPicUsrBuf`)
  - branch-local zero results (`sfpl2_PauseSub`)
  - explicit 0/1 returns where retail branches; named configured stop time
    after negative-time guard (`sfply_StatPlay`)
  - readiness/startability helpers + entry-load request snapshot
    (`sfply_StatPrep`)
  - transport-stop + handle-reset helpers (`SFD_Stop`, `SFD_Destroy`)
  - phase helpers for long server (`sfadxt_ExecServerSub`)
  - time-update helpers with narrowed outputs (`sfadxt_GetTime`)
  - pause-off helper after reset (`SFADXT_Pause`)
- `dont_inline` scope inherited from scaffolding can force shared helper out
  of line; test removing it (`sfply_ExecOne`).
- Donor version gaps, both ways: don't import donor-only null guards,
  differing signatures, codeless reg pins retail lacks. Retail has guards/fast
  paths/fills donor lacks `[da]`: REQUIRE version marker (mk-da: "HVQM4 1.5"
  vs Pikmin's older decoder) + retail branch structure; add only
  retail-proven branches around donor body.
- Quantization/VLC macros: name quantizer-scaled level inside each expansion;
  follow real consume/refill path; compare guarded peek/extract/refill
  expansions sibling by sibling.
- Auto-inline size limit: object-scope `-pragma "inline_max_size(N)"` (N 8..24
  inlines 3-instr helper, keeps 26-instr static as call). Compiler 2.7
  ignores `inline_max_auto_size`; `-inline on`/`noauto`/`level=0` don't clear
  base `-inline auto`. Only when retail inlining proves the limit.

## M14

Varargs. REQUIRE EABI `va_list` + variadic callers. `crclr` supports a
variadic call, not prototype guesses. Retail saves whole GPR/FPR area and
builds `va_list` inline where SDK macro emits helper call: scope
`__builtin_va_info` form to proven functions (`OSPanic`, like `OSReport`).

## M15

String/constant identity, placement, extent. REQUIRE ELF sizes, bytes, reloc
addends, use. Use `-c functionRelocDiffs=data_value`.

- Pooled literals for anonymous pools, named objects for real symbols,
  natural string bounds. Equal pool sizes != right strings (Krypt swapped
  coffin/dirt names). Reloc may name first aggregate as section-wide base;
  check later base+offset loads.
- Pooled-string addend differs: REQUIRE decode retail bytes at effective
  offset, compare selected literal. Same pool bytes + different addends =
  different strings; TRY evidenced literal before calling it TU layout residue
  (`pz_fighter_chomper2_victim_crushed`).
- Generic DecompStudio pool-name warning needs independent
  `functionRelocDiffs=all` check (`calc_cloth_dwp` passes despite it). Compare
  effective consumer offsets + whole-pool bytes: `konquest_make_monk_an_npc`
  addresses right `hero_npc` suffix while unrelated later strings keep pool
  data-value <100.
- Trace aggregate copies + each scalar load to consumer; named aggregate
  compare doesn't show which vector a caller uses.
- Nonzero words inside reconstructed padding = initializer data; keep
  offset-named.
- Missing prefixes shared by many consumers can mean missing real table
  (`global_background_data`).
- Unused inline body can still emit initializer data.
- Terminal `gap_*`: check linker alignment first. `jmt`: retail split `gap_*`
  = final 4 bytes of `.data` + `.sdata2`; built sections stop at those
  offsets, both 8-byte aligned. No fake source objects for split padding.
- High-bit bytes in plain `char` pools: adjacent literals with fixed-width
  octal escapes, keep embedded NULs.
- Float pools: retail `lis`/`lfs` per literal vs one pooled base: object
  `-pooldata off` if whole-object control passes (`SFADXT_SetSpeed`), else
  donor scoped `pool_data off` (`sfmpv_Pts2Tc`).
- Literals in `.data` or one pooled object, retail one local `@NNN` per
  literal in `.rodata` `[da]`: `-str reuse,readonly` (not `pool`). Split
  literals as `@stringBase0 + n`: object `-str reuse,pool,readonly` (`mk_obj`).
  One-char literal in `.rodata` not `.sdata2`: `-sdata2 0 -str
  reuse,readonly` (`adx_errs`).
- `jmt`: first-use ordered effect-name literals + that flag give retail
  `@stringBase0` with bytes. Keep two angle vectors before string pool;
  file-scope stand-ins kept bytes but changed aggregate-copy scheduling. Bytes
  identical, so data-value missed crossed relocs: typed inline decoy-visual
  initializer gave `p_decoy` `.rodata+0`, `p_kabal_smoke` `.rodata+12` as
  retail. Typed angle-sum local with nested scale expr from matched
  `r_cyrax_blade` ordered the two final `.sdata2` floats. Compare normalized
  reloc section/offset/type targets + raw bytes before claiming link
  equivalence.
- Bytes exact but DecompStudio shows `[order]` `[da]`: REQUIRE retail
  addresses. TRY definitions in retail order with real forward decls.
- Deferred order: parse-time symbols numbered backwards through `.text`
  (`tools/deferred_scan.py`, Kendall tau < -0.5) = `-inline deferred`;
  functions emitted in reverse source order. Try `-inline noauto,deferred` +
  defs reversed by `tools/reverse_deferred_tu.py`; retail-inlined global
  callees get `auto_inline on`/`reset` pair. Plain `deferred` after `-inline
  auto` wipes static helpers. Land only on measured whole-unit gain (konquest
  gained; fatality, cam didn't).
- Deferred inline control: `#pragma dont_inline on` acts at CALLEE
  definition; wrapping caller doesn't stop its callees inlining. Wrap retail
  out-of-line callees in contiguous regions. Retail inlines self-recursion to
  depth N (plus default getter): object `-inline auto,deferred,level=N`, no
  per-function `inline_depth`; `noauto` loses it (`mwMemSystemSetParams`).
  Inlined MAP `UNUSED` helper: restore as `static inline` when standalone
  compile = MAP size (`privGeneralGetHeapFromPtr` 0x58,
  `privGetUserSizeFromBlock` 0x44 closed `_mwMemRealloc`). Permuter NG strips
  bodies after target; under deferred re-measure every lead in full TU.
- Pool residue: file-scope pointer initialized with literal pools it at decl
  position. Strings no code uses = linker-discarded or debug code: check
  `orig/GQNE5D/files/mk6gc_release.MAP` (`UNUSED` + size, `UNREFERENCED
  DUPLICATE` for weak copies). Plain `static` helper compiles standalone, keeps
  pool data even stripped, and its body position sets literal numbering even
  when `-inline auto` expands it at every call site `[da]`; `inline` helper
  pools only at inline sites. Ordinary unused non-inline header function
  explains literals with no code; include position explains numbering/order;
  body values only literals support stay unproven `[da]`. Split a helper only
  where retail shows separate source regions (line-number groups). Restore
  discarded function only with genuine body (`dsp_task` 2004 SDK
  `__DSP_add_task`); no donor body -> stop. Keep named `static const` objects
  when converting drops placeholders (`adx_tlk`). Stripped-code restoration
  needs the user's stripped-code ruling `[da]`.

## M16

Global + zero-fill order; SHA fails at report-100. REQUIRE raw offsets,
alignment, SDA relocs. objdiff can't compare `.bss`/`.sbss` (zero-filled
compares equal, relocs by name): compare `powerpc-eabi-nm -n -S` of retail vs
built objects; only linked DOL hash catches wrong layout.

- MWCC emits uninit globals/statics in reverse decl order (`baim3d`,
  `fonts`; mk-da `babinwor` passed objdiff, failed hash until reversed).
  Uninit static among `= 0` globals follows all of them. 1.2.5n places
  explicit zero-init globals before function statics (`GXInit`, `__piReg =
  NULL`). 2.7 C deferred: `= 0` prefix keeps forward decl order; uninit
  globals + function statics follow, reverse parse order (function static sits
  at its function's parse position) (`mwMem` `StrategyAllocationActive`).
- 2.7 C `-inline auto` (not deferred), measured `ai.c`: `.sbss` tentatives =
  reverse decl order, use irrelevant. `.bss` tentative defined before use =
  first-use order by generated function, operand order as written (`c ? &A :
  &B` -> A first); unexpanded inline bodies + decl order don't count. Object
  only `extern` at first use, defined after the functions -> placed after the
  first-use ones, reverse definition order. Retail `at_cam_data, g_DroneAI2,
  g_DroneAI1` = static used first + `g_DroneAI1; g_DroneAI2;` defined at file
  end, externs at top. All functions byte-identical either way.
- Function-static `name$N` suffix differs: objdiff falls back to section
  offset. Fix offset, not parse numbering. MAP `UNUSED` data absent from split
  object: omit it, else later offsets shift (`mwMem` `heapIndex`).
- Whole-TU deferred/noauto control changes tentative BSS from first-use to
  reverse decl order: REQUIRE each object's retail offset, size, align,
  binding. TRY reversing whole tentative decl block with function defs (M15),
  keep explicit scalar-zero prefix (`ADXM_ShutdownThrd`). Reversing defs alone
  can put stacks before threads at same fuzzy score. Land only when every
  function + full BSS map agree/improve, via supported object flag, diagnostic
  pragmas removed.
- Explicit zero inits in proven order fix placement but may move object to
  another section (`sfmpv_ta_adr_tbl`, SVM aggregates); verify section.
- Invented aggregates overlaying several retail symbols: separate objects
  with retail names; fix first-use order with donor's linker-discarded setter
  (`mwPlySetFrmBuf`).
- First reference: BSS order starting with symbols used only by discarded
  code needs those genuine donor routines (`svm.c`, `ADXF_GetNumCmd`,
  `AXRNA_DbgDump`, `gcCiInit`). Donor-declared volatile build-pointer read with
  retail entry load = source, not dead read (`LSC_Init`, `ADXGC_SetupDvdFs`);
  retail symbol binding beats donor storage class. Only scalar prefix proven
  -> init that prefix, stop.
- Small object's first `extern` must see complete type for SDA (DVD thread
  queue); multi-record table declared as pointer falsely picks SDA. All small
  globals in `.bss` with `lis` addressing where ours uses `@sda21`: object
  `-sdata 0` (`sfd_tim`).
- Split gap vs object alignment: `g_DSB_Buffers` needs 32-byte align, not gap
  object.
- Never model `.sdata` gap with section pragma on zero static; lands in
  `.sbss`.
- Flag sweeps: check ninja exit status, delete `main.dol`/`main.elf` first;
  failed link leaves old DOL that still hashes OK.

## M17

Vtables, weak dtors, link-only diffs, C++ emission. REQUIRE ELF relocs,
hierarchy, weak owner, sizes, order. Fix decls, zero slots, inline-visible
dtors; verify weak emission with linked SHA. Don't duplicate deleting call's
null guard.

- `extern "C" T f() {...}` needing implicit inline (struct copy assign,
  template instance) is deferred to end of `.text`; declare prototypes in
  `extern "C" { }` block, plain defs (`mwFileAsync`).
- Linker keeps first weak def in link order; no specialization to own weak
  members an earlier object provides.
- Vtables emitted at TU end in reverse creation order. Key function creates
  vtable at its compile; none -> weak vtable at first codegen ref (derived
  ctor inlining base ctor, not derived dtor).
- extab/extabindex in object `[da]`: REQUIRE ELF section symbols. TRY
  `-Cpp_exceptions on` at lib/object scope. Every function + data exact but
  hash fails on two bytes in `.extab` (uninit record padding): REQUIRE retail
  bytes there; coordinator sets `extab_padding=[b0, b1]` on the Object (dtk
  extab clean). Metadata, not source (mk-da: Gecko_ExceptionPPC `[0x12,
  0x00]`).
