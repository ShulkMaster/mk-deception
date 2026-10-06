# Playbook tier 4: rare, stops, search

Rare diagnostics, hard stops, dead ends, permuter. Use after tiers
[1](playbook-1-core.md)-[3](playbook-3-uncommon.md). Not default optimizer
knobs. `[da]` = mk-da import (tier 1).

## N01

Boolean/rotate idiom survives honest C. REQUIRE exact intrinsic semantics +
unsigned contract. Proven `cntlzw`/rotate intrinsic, not instruction padding.

- Integer absolute value expands to `srawi`/`xor`/`subf`: REQUIRE signed input
  range excluding `INT_MIN` and no retail call. TRY canonical MWCC `__abs(int)`
  at the real zero-axis/max phases (`mk_chess_drone_handle_the_big_chill_opening_move`).
  A plain `abs` declaration can emit a call; ternary abs can hoist a branch
  outside the inner loop. Verify expansion and all header consumers.
  Unused distance output in a proven full octant/distance expansion: `__abs`
  can retain magnitude instructions erased by ternary abs; verify the real
  output contract and shared consumers, never add a fake output use
  (`mk_chess_drone_cursor_movement_to_target`).

## N02

Global address vs contents/SDA differs. REQUIRE ELF object/array identity +
all uses. Fix object/array/pointer decl; no fake array to dodge SDA (M16).

## N03

Repeated RTTI/null ladder. REQUIRE class identity, no call, null paths.
Inline typed ladder; no guessed class or layout.

## N04

Factory cases return distinct owners. REQUIRE alloc/failure semantics + case
order. Direct typed returns, no artificial shared result.

## N05

Reverse iteration/update differs. REQUIRE direction, stride, zero-count
semantics. Typed reverse-index walk or vendor pre-decrement idiom; no manual
byte offsets.

## N06

Peephole/CSE/scheduler residue. REQUIRE tested TU hypothesis. Test exact
option in scratch; no per-function exceptions during fixed-TU-setting work;
no flags masking wrong source.

Compiler revision hypothesis: keep recovered command, separate scratch
outputs, pinned-revision control, compare text bytes + every function/data
result. Byte-identical output rules out those revisions for that source +
command only. Keep pinned compiler.

## N07

Runtime scratch faults after object growth. REQUIRE section extents,
relocated instr bytes, non-overlapping mapped ranges. Fix loader section
placement / slot bounds before touching source; never skip faulting instr to
claim equivalence.

## N08

m2c shows possibly unwritten local retail consumes. REQUIRE instr-level
write/read paths, frame offsets, caller reachability. Vary only incoming stack
word in retail runtime scratch, inspect downstream result. Stack dependence !=
reachable gameplay. Record unresolved path; initializer only with evidence.

## N09

Retail keeps explicit byte-swap sequence where compiler folds typed access to
`lhbrx`/`stwbrx`. REQUIRE same CFG, endian result, width, dest. Stop at clean
shift expression; no donor dead conditional or fake read to block fold
(`SFH_AnlyMaxFrmNum`).

## N10

Retail classifies float via IEEE word with `lwz`. REQUIRE 4-byte float/word +
same-source donor word idiom. Test donor's narrow word view under MWCC,
`memcpy` fallback for other compilers; plain `memcpy` under MWCC changes frame
(`ADX_GetCoefficient`). No arbitrary alias casts or union overlays.

## N11

C after an `asm` function loses peephole forms (`mr` stays `addi rX,rY,0`,
shifts fold into `rlwimi`, compare moves), from first `asm` function above
it: `asm` function turns peephole off for rest of file. Retail wrote C with
inline `asm {}` helper (GX `Copy6Floats`, `WriteMTXPS*`) -> wrap sequence
functions in `#pragma push` ... `#pragma pop`. Retail had real `asm`
functions (OSCache, OSExec, OSTime, ai) -> leak is authentic; remove old
`#pragma peephole off` / `-opt nopeephole` workarounds.

## N12

Asm sequence ICE `internal compiler error: File: 'PCodeAssembly.c' Line: 510`
when `lwz`/`stw` has `@l` offset into code symbol (function or `entry`
label). MWCC 1.2.5n asm can't emit it in any spelling; retail likely did it
in C. Keep function on C path, record blocked. No `opword` (loses reloc).

## N13

Byte-neutral candidates combine. IF near miss + several honest edits each
leave target bytes identical (objdump hash of symbol, not equal score) |
REQUIRE each edit alone passes honesty filter; CFG/ABI/layout right | TRY
joint sweep of recorded neutrals before any soft ceiling.

- Never drop byte-neutral edit silently: history `kind: "neutral"`, exact
  edit + base rev. Sources: ng `alternatives` scoring = base, MAP-evidenced
  helper restores, decl/type/statement-order flips, neutral seeds. Equal
  score + different bytes -> pool too, marked.
- Neutral alone != evidence against edit. Can shift vreg numbering, inline
  temps, lifetimes that another edit needs.
- Sweep on host: every subset of pool (2^k, k <= 10 -> <= 1024 compiles),
  each subset also + each partial-improving edit. Pool holds decls/types ->
  cross with decl-order x type sweep. Winner -> full-TU compile, then `try`.
- Land winning subset only. Extra neutral stays only if source clearer.
- Exemplar: `_mwMemRealloc`: MAP-helper restore (`privGeneralGetHeapFromPtr`
  + `privGetUserSizeFromBlock`) alone byte-identical; `unsigned int copySize`
  alone still off; both -> exact.

## Hard stops

After the applicable honest source check, stop at:

- GPR/FPR coloring, param nonvolatile homes, or permutations rotating the
  residue.
- Two param homes exchanged (retail gives later param higher saved reg;
  `drone_ai_check_attack` r30/r31): REQUIRE MWCC simplify model before
  spending attempts. Model:
  - Virtual numbers: params first in decl order (`p1 < p2 < p3`, measured),
    named locals next in reverse decl order (block-scoped lowest), then
    front-end temps (inline copies, split ranges, inline result joins), then
    codegen temps.
  - Web kinds, in number order (captured, replay exact, `pz_ai_decide_match`):
    block-scoped named (reverse decl) < function-scoped named (reverse decl)
    < 2nd+ webs of a variable (groups in reverse source order of the
    variable's first web, webs in a group forward) < CSE temps (value used
    twice; reverse source) < loop temps (hoisted invariants, strength-reduced
    bases; reverse source) < `?:`/`&&`-mask results and inline-helper
    locals/results (reverse source) < plain emission temps (forward). A named
    local assigned once from a `?:` or an inline result is propagated away and
    lives on as that temp. Multi-def local: 2nd webs from in-place copy
    (`x = src; x = c(x) ? x : 0`) number forward; direct `x = c ? src : 0`
    webs number in reverse source order (`p_fish_attack`). Coalesced
    temp-temp copy (helper `c = type` with a temp argument) = never-pushed
    node, +1 degree on every neighbour.
  - Several long-lived saved homes rotated (param, flag, call result, two
    hoisted array bases): all mutually adjacent, pushed ascending once degree
    < 29 in sweep 2. A web scanned at number N stays unpushed iff 12 physical
    + other big webs + S_above + C >= 29 (S_above = long-lived webs numbered
    above N, C = coalesced neighbours); unpushed big webs then go in sweep 3
    in ascending number, so retail's order fixes which kinds the small webs
    must have. Count S_above + C from the capture before trying spellings.
    Honest levers that move a web between kinds: `x = cond ? 8 : 5`
    (emission temp), `same = a == b ? 1 : 0` (same cntlzw idiom, temp popped
    first -> r0), inline colour helper applied to the field expression with
    the raw read CSE'd into a breaker macro (copy coalesces, `subi r5,r5,4`
    stays in place, +1 C each), open-coded two-def selection instead of an
    inline result (named web popped late -> r10), direct `placements[p][c]`
    indexing instead of a pointer local (CSE temp pops before the row load),
    pointer local with a 2nd web for a single-use cell address (materializes
    `add; lwz 4(r)` where a direct read folds to `addi; lwzx`), and
    variable ownership so a loop's row/column/cell webs are first webs or
    splits in the needed group order. Levers interact through the threshold:
    measure each alone and in the combination.
  - Named-local rotation (several saved homes permuted, no stall): one
    "first-init order" decl check is not exhaustive. Capture, map vregs to
    names, write retail pop order, invert into per-sweep ascending push order,
    then pick the decl permutation (reverse decl numbering) that yields it;
    verify in scratch (`__VMSwapPageIn`, 8 locals, one try).
  - Simplify: each pass scans ascending, pushes every web with degree < free
    reg count (29), decrements neighbours at once. Select pops, takes lowest
    free colour claiming r31 downward -> two params pushed in one pass: later
    gets higher reg.
  - Physical regs + coalesced webs never push, never decrement. Call result
    feeding a move (`x = call()` into multi-def var, inline `return 0 | return
    call()` join) = never-pushed node; one consumed by compare/shift is
    copy-propagated away, doesn't count.
  - Stall (no pushable web, both params at threshold): lowest spillCost/degree
    pushed first (reads x2, writes x1, arg-init -1) -> param with fewest reads
    ends lower (`immediate`: five `cmpwi`, cost 9; `drone` ~37).
  - Probes (scratch only, not honest source), report entry `mr` pair: 20 extra
    reads of cheap param via global store (no new long-lived web) flips pair
    -> proves stall; deleting one never-pushed neighbour where both live flips
    it -> proves threshold.
  - Reduced reproducer flips by another route (middle param vreg 33 pushed
    between examining 32 and 34 at raw degree exactly 29): reduced TU is not
    evidence for full function.
  - Neutral levers there: decl/statement order, helper boundaries, staged call
    args, scopes, variable identity, inline depth, `?:` vs if/else, pragmas,
    K&R order, TU isolation, 3rd param slot, long-lived `force`. Pair = hard
    stop unless a never-pushed neighbour can be removed with same stream.
  - Never-pushed neighbour = higher-numbered side of every coalesced move
    (`coalescenodes` keeps its matrix row). Coalesce only temp<->temp or
    any<->physical. Nested inline helper `return inner(...);` = move between
    two front-end result temps -> one stale node per expansion. Flat helper
    (return expr computed straight into its own result temp) -> none.
  - Closed (`drone_ai_check_attack`): both params at 29 = 12 physical + each
    other + 16 coalesced temps. TRY drop exactly one coalesced temp, same
    stream: flatten one helper level at one site
    (`ai_fighter_table_row_count` with own null test + table load, not nested
    `ai_move_table_row_count`). Dropped node inside another saved local's
    range (`category`) rotates r25-r29 -> cross with decl order (`category` at
    function scope before `special_count`). Flat alone: pair right, others
    rotated. Decl alone: neutral. Both -> exact.
  - Capture, not inference: gdb on `mwcceppc.exe` 2.7 under wibo (no
    sjiswrap), `starti`, then hw breakpoints: `buildinterferencegraph`
    0x57bfb0 (pcode before coalesce), `simplifygraph` 0x5088d0 (graph),
    `rewritepcode` 0x508680 (colours). Globals: `interferencegraph` 0x5ea768,
    `coloring_class` 0x5ef2cf (GPR = 4), `used_virtual_registers[]` 0x5eaa2c,
    `n_real_registers[]` 0x5ea710, `pcbasicblocks` 0x5ea748. IGNode (pack 2):
    next 0, spillTemp 4, cost 0xc, degree 0x12, reg 0x14, flags 0x16 (pushed
    2, coalesced 4), count 0x18, neighbours 0x1a. PCode: next 0, op 0x20,
    argCount 0x22, args 0x24 x 12 bytes (kind 0, class 1, reg 4); block
    firstPCode 0x14. Replay simplify/select offline, drop one candidate node,
    compare colours with retail -> names region where source must lose a
    temp. Built-in `debug_listing` is stubbed in this binary.
- Volatile rotation under scoped `opt_*` pragma is NOT a stop until H21
  web-kind check + pragma-free H05 mechanism measured
  (`drone_ai_victim_avoid` closed from recorded 99.44 ceiling: `opt_common_subs
  off` -> address-taken sqrt input + direct global reads).
- `li 0` vs copy of already-zero reg; commutative scratch encodings.
- Frameless PLATFORM `mtlr`/`blrl` emission.
- Anonymous reloc labels with verified identical payloads + targets. Equal
  ordinary scores not enough (wrong return constants scored equal); compare
  bytes in data-value mode. Inferred `R_PPC_NONE` on identical instrs =
  metadata residual; record, don't rewrite source. Not report-exact or
  link-exact.
- Equivalent branch/address lowering without new source evidence.
- Vendor code whose matched references use shared-exit `goto` (MSL
  `__dec2num`: `goto done`; `__str2dec`: `goto round`) where flag-and-break
  leaves extra instrs: AGENTS.md last-resort `goto`, record measured
  alternatives.
- One instr scheduled across a store, body textually identical to matched
  decomp of same SDK (`SPEC2_MakeStatus` vs TP), object-scope flag probes
  neutral.
- Permuter candidate that only recomputes an unchanged value to split a live
  range (`SJRBF_PutChunk`): record insight, don't land.

Then `TODO: [near miss]` + score + residual + stop reason; disclose
nonmatching fallbacks in SHA results. Reopen only for new evidence: proven
owner (GX FIFO union) justified returning to H02; another spelling of same
lifetime didn't. Failed function parks in DecompStudio as `soft_ceiling` with
note + TODO line; escalation per coordinator brief `[da]`.

## Dead ends

Don't repeat without new evidence:

- Compiler version sweeps for coloring residue `[da]`.
- Narrowing cached local to field storage type (`u8`/`u16`) because producer
  is narrow: changes frame + compares. Keep promoted `int` cache unless retail
  conversions prove width `[da]`.
- Collapsing two params carrying equal values (src + dest stride) into one
  local or direct field reads: loses shared load + arg copy `[da]`.
- ELF symbol size/scope prove storage, not C qualifiers or opt settings;
  empty `.debug`/`.line`, uniform `.mwcats` carry no volatile/opt-level
  evidence `[da]`.
- Asm stubs for coloring ceilings: rejected by user, regress callers `[da]`.
- Renaming locals (H21/Traps); moving inline boundaries around already-right
  code (H21).

## Permuter and search

Requires established algorithm, CFG, ABI, layout, real TU command. `PERM_*`
stays in scratch. Reject UB, wrong types, reordered effects, fake liveness
even at score zero; land one honest insight, verify in real TU.

Procedure `[da]`:

- Early probe: one ng run <=30 s once CFG/ABI/layout are right; ng converges
  fast, clean leads show within a few thousand candidates.
- Main search: `submit {commands:[{op:"permute", symbol, engine:"ng",
  seconds:120}], wait:false}` + `fence`; one run in flight per agent (runs
  queue machine-wide, cap includes queueing). Target >=120k compiled
  candidates before calling a ceiling.
- Argument staging around calls: `mode:"call_args"` (rust) first; enumerates
  keep/fold/fold-value/hoist, never crosses observable call or store.
- ng keeps stopping on dirty zeros despite `dirty_zero_s` / `honest_share` /
  `unlicensed_factor`: finish with `engine:"rust"` (behavior-preserving only;
  counts toward total).
- Log engine, compiled count, result in history; byte-neutral candidates
  -> N13 pool.

Honesty filter `[da]`:

- Port by hand or via `replace` edits, check with `try`. Dirty candidates =
  leads: strip + measure each component separately. Ask what each forcing
  construct does: pragma fixing CSE -> points at a reload (H05); alias ->
  separate live value. Alias fixing coloring often marks param copies of a
  lost inline helper: does donor call a helper there? restore that call shape
  (H07).
- Reject: pragmas (`opt_propagation`, `optimization_level`), `new_var` copies
  of in-scope values, reuse of unrelated vars, unsequenced exprs, redundant
  same-value or duplicated stores, invented helpers, zero needing an otherwise
  unused alias (`ani_to_frame_x`).
- `alternatives:true` + `tuning_report:true` give engine honesty findings;
  audit by hand anyway (`honest_only` / `all_passes_honest` still returned
  pragma-bearing leads in mk-da).
- Neutral seed rotation `[da]`: honest byte-equivalent variant -> adopt in
  draft (`try` `keep:true`), permute again from it. Very seed-sensitive.
- Several nonzero candidates sharing one idea: express it without `new_var`,
  `if (1)`, comma ops, try in real TU (five candidates feeding `(x *= k)` to a
  call -> H15 scaled-arg form).

Scoring + scratch:

- ~5 per reg-name diff, ~100 per inserted/deleted instr. Low nonzero can hide
  wrong colors; 5 may add no code. Diff candidate objdump vs `target.o`.
  `--stack-diffs` when stack operands differ (default ignored reversed
  +0x08/+0x0C stores in `run_camera_script`).
- Base score far above real residue: retail inlined same-TU callees, import
  strips non-`inline` bodies. Restore those callees as `inline` in scratch
  (`SFD_SetCond`), confirm base first.
- Mismatch inside expanded private helper: selecting only caller leaves helper
  unmutated; harness mutating helper while scoring caller, check no helper
  call appears. Expansion + decl order can interact (AI table builders: both
  needed); verify combo per wrapper. Expansion reverses stack-local placement
  -> reverse only scratch decls.
- Candidate behaves different in real TU: compile in frozen full TU with
  recovered command, `objdump --disassemble=SYMBOL`, confirm baseline with
  objdiff.
- Missing-function KeyError on extraction: make canonical decl visible before
  call (implicit-int call absent from type map), reimport.
- Reimport: use newly printed scratch path (maybe `SYMBOL-2`), verify base
  source. One-line `PERM_LINESWAP` can fold away; don't claim exhaustive.
- Reject offsets on both sides of signed compare unless overflow impossible:
  `(x + 1) < 1` != `x < 0` for arbitrary 32-bit.
- Freeze type/cast passes when invalid truncations dominate. Widened temp
  narrowed at every use needs independent width evidence.
- `perm_reorder_decls` moves decls across blocks; pure ordering search ->
  uninitialized decls in one block only (`MKD_PERM_SAME_BLOCK_DECLS=1`).
  Compile failures don't exhaust valid orderings.
- Integer Boolean values outside conditions: `MKD_PERM_INTEGER_BOOLEAN_VALUES=1`
  with only `perm_condition` rewrites `x == 0`/`!x` in values + call args,
  int operands only.
- Scratch parsing can change `numNodes * sizeof(RwMatrix) + 15`; parenthesize
  product in scratch only. "PERM macro in AST" from inline-helper
  `PERM_LINESWAP`: finite text-level `PERM_GENERAL`.
- CSE candidate repeating a table expr: both values feed real outputs, no
  store/call between; verify load count, every macro consumer, runtime
  behavior, linked SHA.
- Mirage imports need platform-neutral behavior + GC retail + objdiff
  evidence.
