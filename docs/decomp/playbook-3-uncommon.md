# Matching playbook, tier 3: uncommon

M rules: localized lowering, compiler modes, ABI shape, inline/macro
boundaries, and data/link layout. Use them after the [tier 1](playbook-1-core.md)
triage and the matching [tier 2](playbook-2-common.md) section. Select by
mismatch, not by past score. M03/M04 spelling trials rarely close anything new;
try one only with a concrete new hypothesis.

Compiler-setting rules (M01, M13, M15, M16):

- Prefer an object-scope flag in `configure.py` over a per-function pragma
  when a whole-TU control passes.
- Scope a pragma to the measured consumer and reset it afterward.
- Run the same source without the setting as a necessity control.
- Compare every function and section against a pre-change report.
- A result never establishes the retail pragma spelling.
- Keep accepted TU flags fixed during fixed-TU-setting tasks.

## M01

Compiler mode across a TU. IF compact saves, `divw`, or Boolean lowering
repeat across the TU, REQUIRE sibling evidence plus all-function and section
baselines.

- TU profile: object-wide `-O4,s` with the existing `-use_lmw_stmw on`; test
  scheduling separately. When compact saves and indexed-loop lowering recur
  together, try the profile before changing loop source; `-O4,p` to `-O4,s`
  alone closed credits text.
- Signed divide by two as `srawi`/`addze`: function-scoped
  `optimize_for_size`, arithmetic unchanged.
- Isolated integer-register save mismatch in an identical body:
  `optimize_for_size` and `use_lmw_stmw` together (projectile sound setter).
  When retail instead uses `_savefpr_29`/`_restfpr_29` with matching scalar
  FP operations, the same supported local profile can restore compact FPR
  helpers (`fxsys_set_v3` in `pfxscript`). Require the actual helper calls,
  immediate push/pop scope, and comparison of every sibling; do not change
  FP expression grouping to compensate for a save-only difference.
- Typed vector copies that preload where retail interleaves loads, stores, and
  owner reloads: consumer-scoped scheduling off. Reject it if the epilogue
  restore order or aligned-frame setup regresses; that is a ceiling, not a
  body-only match. If sibling leaf vector arithmetic repeats the same
  component load/op/store pattern, scope the supported scheduling mode with
  push/pop and verify every sibling (`v3_add_v3` in `mk_math`); a whole-unit
  mode can regress unrelated functions.
- Folded indexed accesses (`lwzx`/`stwx`) where retail does multiply/add/`lwz`:
  real category/base locals plus consumer-scoped `opt_propagation off`; both
  are needed, and the pragma alone may do nothing (move-list getter). The
  pragma also keeps every inlined helper's parameter and return copies in that
  consumer; if those recolor an otherwise exact region, spell the shared helper
  copy-free (H21) rather than dropping the pragma.
- A constant FP load or pooled string address moved across a selection
  branch: propagation off, tested separately from scheduling and CSE (AI
  avoidance reset, Puzzle burn controller).
- A global pointer reloaded after a loop where retail retains the load but
  built code reuses the loop test's register: try function-scoped
  `opt_common_subs off` with an immediate reset and whole-TU comparison.
  `opt_propagation off` was byte-neutral for `kabal_collide_victim`; the
  CSE setting restored its retail `his_pdata` reload. Qualifier: when the
  reload follows an inlined float-bits helper, the pragma reproduces the
  reload but renumbers the volatile webs (`drone_ai_victim_avoid` stalled at
  99.44 under it); an address-taken helper input (H05, H21) gives the reload
  with CSE on and closed the function without a pragma.
- A multi-row field where retail adds the offset to a reloaded owner before
  `stfsx`: a typed row-array pointer plus propagation off (Puzzle crusher).
- A fixed-count array copy lowered to two advancing pointers where retail has
  one byte induction register: scoped `opt_strength_reduction off`. Loops only.
- Size-bit addendum: an `-opt off` TU lowering `ptr != 0` as `neg/or/srwi`
  where retail has `addic r0,x,-1; subfe`: put `-O4,s` in front of `-opt off`
  at object scope when same-library siblings build that way. The `,s` bit
  survives `-opt off`; no spelling produces the carry idiom (`rwfexist`). A
  higher exact count can hide one lost function; compare each.

## M02

Control-word or publication order. REQUIRE retail accesses and alias
boundaries. Load a control word before subfield writes; publish owners at the
observed point; reload counts after aliasing stores. Process/script transfers
observe published state; never move a store past a transfer for a permuter
score. In bounded candidate loops, check whether each trial is published
before validation calls.

- Selected-minimum addendum: retail reads a saved 64-bit minimum, selects the
  lower value, and writes both words even when unchanged: a typed minimum
  local, conditionally replaced, always written back (`sfmps_CopyAudio`).

## M03

FP operands and schedule. REQUIRE the same math, grouping, and rounding.

- Swap only commutative operands, or name genuine factors. Remove temporaries
  only without reassociation. Keep polynomial-before-sqrt order.
- Separate single-precision boundaries: name the intermediates in evaluation
  order (`ADX_GetCoefficient`).
- A rounded product before an addition: an explicit float conversion; verify
  output bits.
- Mixed fused and separate operations: narrow function-scoped `fp_contract`;
  lexical toggles inside a function may do nothing.
- If a vector is normalized before a second vector's deltas are formed, require
  retail multiplies before the delta subtracts; name the normalized components
  at that point. In `mks_get_victim_to_tr_dot`, this recovered the first
  inverse-sqrt result's consumers without changing the arithmetic.
- Constant width comes from `lfs`/`lfd` and pool bytes, not decompiler casts.
  A folded expression can differ from its decimal spelling: `3.0f * 0.075f`
  is 0x3e666667, `0.225f` is 0x3e666666.
- Check whether negation happens in FP before conversion or on the integer.

## M04

Compare boundaries and Boolean diamonds. REQUIRE equivalent bounds and pure
operands.

- Try equivalent thresholds, ternaries, or guarded assignments for the observed
  join. Bitwise Boolean evaluation evaluates both operands.
- A final `cmpwi` with no consuming branch before a void epilogue can come
  from a terminal return guard. REQUIRE a real validity or capacity condition
  and a same-compiler reference with this lowering; try that guard before
  forcing a dead instruction. `start_kabal_smoke_pfx` checks its ten-slot
  emitter capacity, and exact `bgnd_append_texture_to_material` shows the
  same no-branch compare from a terminal return guard.
- `subic`/`subfe` normalization of a call result: a Boolean local
  (`opened = get_coffin_bit(...) != 0`). `subfic`/`cntlzw`/`srwi` field
  queries: an explicit inline success/failure return. A ternary can match
  standalone yet fold to `cntlzw` once inlined (`SFLIB_CheckHn`); explicit `if`
  returns keep the branches.
- Identical calls before different actions in separate retail arms stay
  branch-local.
- One equality dispatch with a conditional jump and an unconditional default:
  a one-case switch with default.
- Operand order of a returned signed compare (`a > b` vs `b < a`) differing
  only in load homes: try once. Not for FP unordered compares.
- A quotient computed once with a guarded subtraction of end padding: keep the
  count and guard the subtraction (`ADXB_ExecOneAdx`).
- Bounded copy: a donor-style minimum expression and a destination pointer
  assigned right after chunk acquisition (`sfadxt_CopyData`).
- Small-request dispatch where MWCC tests the mutating case first:
  `if (request != no_op) { if (request < no_op) return state;
  if (request < next_limit) state = next_state; } return state;`
  (`sfply_StatPlay`).
- Signed 64-bit clamp: `value = value > 0 ? value : 0`, one clamp per rebuild
  (`sfmpv_DecodePicAtr`).

## M05

Integer-to-float scaffolding: a natural cast with proven signedness; remove
fake 0x4330 `volatile` machinery. Genuine bit reinterpretation uses a
supported typed union (see N10).

## M06

A leading stack byte updated and the whole word passed: a byte-bitfield/word
union in GC big-endian layout (`init_pwr_bars` flip word is 0x20000000, not
0x20).

- Mixed `lbz`/`lhz`/`lwz` at one base: a typed byte/halfword/word union only
  with confirmed alignment, extent, and endianness. Keep each consumer's width;
  a word clear followed by byte bitfield updates needs a word view.
- An 8x8 coefficient block cleared with double stores but decoded with Float32
  stores: the donor's Float64 destination with localized Float32 casts, not a
  union (`MPVABDEC_IntraBlock`).

## M07

Aggregate and packed addresses. REQUIRE stride, extent, ownership, and access
width.

- Typed element/subobject pointers: a trailing table at `&items[count]`, a
  payload at `header + 1`. Distinguish byte offsets from element indices:
  `lhzx` with `(bits >> 10) & 0x3FFE` is C index `(bits >> 11) & 0x1FFF`.
- Retail adds `index*stride` before field loads: `owner->array[index].member`,
  not a cached element pointer (`mwPlyGetFxType`).
- Stack vectors exchanging roles: trace each destination through all calls
  (a difference vector updated in place into a midpoint).
- Establish a localized-array base before an index getter, then advance it.
- Uniform stack displacement: an adjacent-version source may show a real unused
  local array (`__DSPHandler`'s four-byte array before `OSContext`). Restore
  only that declaration, with no fake reads, and require the exact retail
  frame size.
- A contiguous run of copied fields can be one embedded aggregate
  (`SFADXT_Destroy`, 0x1C-byte parameter block).
- Fallback-prefix bytes: read `header[-2]`, `header -= 2`, then `header[1]`
  (`SFHDS_SetHdr`).
- Apparently overlapping union arrays may be adjacent regions
  (`MPVTransformWorkspace`: six 0x80 DCT blocks then six 0x100 float blocks);
  counters inside an opaque gap belong in the parameter record.
- Records filled with one sentinel by direct stores: initialize fields in the
  loop, not by aggregate copy (`MPS_Create`).
- Sentinel lookahead: a current-sentinel local, advance the index after the
  body, then load `records[index].sentinel` (`p_lightning_strike_effect`).
- Execute retail table initialization before comparing decoder lookups
  (Sofdec run/level tables biased by -16/-32/-32 bytes).

## M08

Failure edges that bypass a shared success test. REQUIRE dominators and
exactly-once cleanup after ordinary guards failed. Use a structured
do/break region only for genuine cleanup edges, or a real allocation-result
Boolean. No dummy status, one-trip `for`, or duplicated effects. A
donor-backed shared-exit `goto` needs the AGENTS.md exception. After
callback-containing allocation loops, `index == count` does not prove success
if callbacks change `count` (`mslInit`); keep the correct behavior.

## M09

Repeated tests around stores: separate guards. A nullable-list inline predicate
only for a proven contract, not `&global != 0` residue.

## M10

Unexpected masks, pairs, or argument copies. REQUIRE all callers and callee
storage/return ABI. Recover full-width or byte ABI, typed conditional arms,
aligned u64 pairs, by-value POD, or an actual POD return. A low r4 return can
be the low u64 half.

## M11

Wrapper load order differs with correct registers: recover float/integer
parameter interleaving (PPC fills GPRs and FPRs independently) in
definitions, declarations, and every caller. Local snapshots alone can
optimize back. Do not keep incompatible per-TU declarations to hide a caller
residue.

- Script wrappers in `script_functions.c`: the retail load order of
  `current_args` slots is the callee's parameter order. Reorder the callee's
  parameters to ascending slot order, drop fake `current_args` or
  `void* script_args` parameters, and apply the change to every declaration
  and caller. Confirm the callers stay neutral: `got_hit_fx` callers
  regressed, so its int-first order is real. A wrapper that calls
  `get_animation` first rereads `current_args` after that call
  (`_launch_n_land_ani`, `_two_player_animation`).

## M12

Alias analysis changes the access schedule. REQUIRE actual mutability and
callers. Remove unsupported `const` or restore supported `const`; `const` alone
does not prove non-aliasing.

- Coordinate loads moved before intervening stores: compare the input
  qualifier with mutable caller objects (Krypt position inputs, Puzzle flesh
  path).
- Tables and counts reloaded after global stores: remove unsupported pointee
  `const` from each input separately, updating declarations and callers
  (`SFD_SetMpvParaTbl`).
- Read-only slot table at a private helper: the donor mutable-slot signature at
  that helper only, with an explicit const-removal cast at the call; never
  write through it (`sftrn_BuildSystem`, `sftrn_BuildAll`).
- A separately cached destination owner is H05.

## M13

Canonical macro or inline expansion missing. REQUIRE the definition and
repeated retail expansion. Prefer a typed inline helper when a macro keeps a
reference swap or shifts stack offsets. Use a typed macro only when its lvalue
and per-expansion locals are evidenced. Shared headers only for proven
ownership; an unused O0 parameter takes `#pragma unused`, not a wrong prototype.

- Repeated block: one `static inline` at each repeated site
  (`game_count_active_players`); leave already-exact spelled-out sites alone.
- Fixed-record search: keep the donor's element count and induction (SFH's
  26-record helper); one cursor advance per record when retail advances a
  pointer.
- Helper contracts that close functions (all verified with no emitted helper
  symbol):
  - A typed interface snapshot inside each private lifecycle helper
    (`MWSST_Destroy`).
  - A cached backend passed as a second input to a validity helper
    (`MWSST_Reset`).
  - A byte classifier with a typed unsigned accumulator assigned per arm
    (`sfh_GetStmType`).
  - A typed inline lookup followed by a standalone null check, not `||`
    (GOP readers). Reject any trial that loses jump-table relocations.
  - A status-returning inline with out-parameters (`sfadxt_ExcludeSilence`).
  - A restore helper with separate null guards (`SFMPV_Seek`).
  - A first-slot clear plus a bounded slot loop (`SFD_SetPicUsrBuf`).
  - Branch-local zero results (`sfpl2_PauseSub`).
  - Explicit 0/1 returns when retail branches (`sfply_StatPlay`).
  - A named configured stop time after the negative-time guard
    (`sfply_StatPlay`).
  - Readiness/startability helpers and an entry-load request snapshot
    (`sfply_StatPrep`).
  - Transport-stop and handle-reset helpers (`SFD_Stop`, `SFD_Destroy`).
  - Phase helpers for a long server (`sfadxt_ExecServerSub`).
  - Time-update helpers with narrowed outputs (`sfadxt_GetTime`).
  - A pause-off helper called after the reset (`SFADXT_Pause`).
- A `dont_inline` scope inherited from scaffolding can force a shared helper
  out of line; test removing it (`sfply_ExecOne`).
- Do not import donor-only null guards, differing signatures, or codeless
  register pins retail lacks.
- Quantization and VLC macros: name the quantizer-scaled level inside each
  expansion; follow the real consume/refill path; compare guarded
  peek/extract/refill expansions sibling by sibling.
- Auto-inline size limit: object-scope `-pragma "inline_max_size(N)"` (N 8..24
  inlines a 3-instruction helper and keeps a 26-instruction static as a call).
  Compiler 2.7 ignores `inline_max_auto_size`; `-inline on`/`noauto`/`level=0`
  do not clear the base `-inline auto`. Only when retail inlining proves the
  limit.

## M14

Varargs setup. REQUIRE EABI `va_list` and variadic callers. `crclr` supports a
variadic call, not arbitrary prototype guesses. If retail saves the whole GPR/
FPR area and builds the `va_list` inline where the SDK macro emits a helper
call, scope the `__builtin_va_info` form to the proven functions (`OSPanic`,
as in `OSReport`).

## M15

String/constant identity, placement, or extent. REQUIRE ELF sizes, bytes,
relocation addends, and use. Use `-c functionRelocDiffs=data_value`.

- Pooled literals for anonymous pools, named objects for real symbols, natural
  string bounds. Equal pool sizes do not prove correct strings (Krypt's swapped
  coffin/dirt names). A relocation may name the first aggregate as a
  section-wide base; check later base-plus-offset loads.
- A generic DecompStudio pool-name warning needs an independent
  `functionRelocDiffs=all` check (`calc_cloth_dwp` passes despite that warning).
  Compare effective consumer offsets as well as whole-pool bytes:
  `konquest_make_monk_an_npc` addresses the correct `hero_npc` suffix while
  unrelated later strings keep its whole-pool data-value comparison below 100.
- Trace aggregate copies and each scalar load to its consumer; a named
  aggregate comparison does not show which vector a caller uses.
- Nonzero words inside reconstructed padding are initializer data; keep them
  offset-named.
- Missing prefixes shared by many consumers can mean a missing real table
  (`global_background_data`).
- An unused inline body can still emit initializer data.
- Before modeling a terminal `gap_*`, check whether linker alignment already
  produces the extent. In `jmt`, retail split `gap_*` symbols occupy the final
  four bytes of `.data` and `.sdata2`; the built sections stop at those symbols'
  offsets, and both sections have 8-byte alignment. Do not add fake source
  objects merely to reproduce split padding.
- High-bit bytes in plain `char` pools: adjacent literals with fixed-width
  octal escapes, keeping embedded NULs.
- Float pools: retail `lis`/`lfs` per literal vs one pooled base: object-level
  `-pooldata off` if the whole-object control passes (`SFADXT_SetSpeed`),
  otherwise the donor's scoped `pool_data off` (`sfmpv_Pts2Tc`).
- Split literals: `@stringBase0 + n` from separate literals with object-scope
  `-str reuse,pool,readonly` (`mk_obj`). A one-character literal in `.rodata`
  instead of `.sdata2`: `-sdata2 0 -str reuse,readonly` (`adx_errs`).
- In `jmt`, first-use ordered effect-name literals with that object-scope flag
  produce the retail `@stringBase0` symbol and preserve its bytes. Keep the
  two angle vectors before the string pool; file-scope stand-ins preserved
  bytes but changed aggregate-copy scheduling. Their bytes are identical,
  so data-value matching alone missed crossed relocations: a typed inline
  decoy-visual initializer preserved both functions' text while giving
  `p_decoy` `.rodata+0` and `p_kabal_smoke` `.rodata+12` as retail does.
  A typed angle-sum local with the nested scale expression from matched
  `r_cyrax_blade` preserved exact text and ordered the two final `.sdata2`
  float words as retail. Compare normalized relocation section/offset/type
  targets as well as raw section bytes before claiming link equivalence.
- Deferred order: parse-time symbols numbered backwards through `.text`
  (`tools/deferred_scan.py`, Kendall tau < -0.5) mean `-inline deferred`;
  functions are emitted in reverse source order. Try `-inline noauto,deferred`
  with definitions reversed by `tools/reverse_deferred_tu.py`; retail-inlined
  global callees get an `auto_inline on`/`reset` pair. Plain `deferred` after
  `-inline auto` wipes static helpers. Land only on a measured whole-unit gain
  (konquest gained; fatality and cam did not).
- Pool residue: a file-scope pointer initialized with a literal pools it at the
  declaration's position. Strings no code references mean linker-discarded or
  debug code: check `orig/GQNE5D/files/mk6gc_release.MAP` (`UNUSED` with size,
  `UNREFERENCED DUPLICATE` for weak copies). A plain `static` helper is compiled
  standalone and keeps its pool data even when stripped; an `inline` helper
  pools only at its inline sites. Restore a discarded function only with its
  genuine body (`dsp_task`'s 2004 SDK `__DSP_add_task`); without a donor body,
  stop. Keep named `static const` objects when converting them drops
  placeholders (`adx_tlk`).

## M16

Global and zero-fill order; SHA fails at report-100. REQUIRE raw offsets,
alignment, and SDA relocations. objdiff cannot compare `.bss`/`.sbss`, so
compare `powerpc-eabi-nm -n -S` of retail and built objects.

- MWCC emits uninitialized globals and statics in reverse declaration order
  (`baim3d`, `fonts`). An uninitialized static among `= 0` globals follows all
  of them. Compiler 1.2.5n places explicitly zero-initialized globals before
  function statics (`GXInit` with `__piReg = NULL`).
- Explicit zero initializers in proven order can fix placement but may move an
  object to another section (`sfmpv_ta_adr_tbl`, SVM aggregates); verify the
  section.
- Invented aggregates overlaying several retail symbols: separate objects with
  retail names; fix first-use order with the donor's linker-discarded setter
  (`mwPlySetFrmBuf`).
- First reference: BSS order that starts with symbols used only by discarded
  code needs those genuine donor routines restored (`svm.c`, `ADXF_GetNumCmd`,
  `AXRNA_DbgDump`, `gcCiInit`). A donor-declared volatile build-pointer read
  with a retail entry load is source, not a dead read (`LSC_Init`,
  `ADXGC_SetupDvdFs`); prefer retail symbol binding over donor storage class.
  If only a scalar prefix is proven, initialize that prefix and stop.
- A small object's first `extern` must see the complete type for SDA
  addressing (DVD thread queue); a multi-record table declared as a pointer
  falsely selects SDA. All small globals in `.bss` with `lis` addressing where
  the candidate uses `@sda21`: object flag `-sdata 0` (`sfd_tim`).
- Distinguish a split gap from object alignment: `g_DSB_Buffers` needs
  32-byte alignment, not a gap object.
- Never model an `.sdata` gap with a section pragma on a zero static; it lands
  in `.sbss`.
- When sweeping flag changes, check ninja's exit status and delete
  `main.dol`/`main.elf` first: a failed link leaves the old DOL, which still
  hashes OK.

## M17

Vtables and weak destructors, including link-only differences. REQUIRE ELF
relocations, hierarchy, weak owner, sizes, and order. Correct declarations,
zero slots, and inline-visible destructors; verify weak emission with the
linked SHA. Do not duplicate a deleting call's null guard.

- `extern "C" T f() {...}` definitions needing an implicit inline (struct copy
  assignment, template instantiation) are deferred to the end of `.text`;
  declare prototypes in an `extern "C" { }` block and use plain definitions
  (`mwFileAsync`).
- The linker keeps the first weak definition in link order; do not add a
  specialization to own weak members an earlier object provides.
- Vtables are emitted at TU end in reverse creation order. A key function
  creates the vtable at its compile; without one, a weak vtable appears at the
  first codegen reference (a derived constructor inlining the base
  constructor, not a derived destructor).
