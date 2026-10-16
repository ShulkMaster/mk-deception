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
- Function-level `asm` sets peephole off and never restores it. Each asm
  statement starts a new basic block. Physical regs named in asm are
  reserved (one fewer colour, skipped by nonvolatile ladder). Asm-touched
  vars never lifetime-split, never scalar-replaced. Optimizer disabled for the
  whole function only by side-effect/branch/`bl` asm.

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
- `ShadowCameraUpdate`: direct canonical container macro alone byte-neutral;
  node/end/atomic declaration order alone regresses. Together they retain the
  named atomic owner and restore all three saved homes (H21).

## N14

- One indexed-address ADD is commuted, with every register home correct:
  REQUIRE native observation of the actual inline helper's parsing as well
  as the caller's initial PCode and address descriptor. GC2.7
  `add_pointer_offset` builds pointer-left `EADD(pointer, index * sizeof(row))`
  directly; observing `optimizecomm` only in the caller misses that producer
  (`obj_bld_surface_build_polys`). TRY preserving the complete typed nested
  subscript at the load: `triangles[triangle_index][corner]` instead of a cached
  row pointer. In `CreatePoly`, reading `vert_map[n]` directly inside each
  position/UV/color subscript instead of caching its value likewise recovers
  retail indexed loads. Native selector traces distinguish the latter case:
  the cached index materializes three source address pairs before applying
  member offsets; direct subscripts retain those pairs for indexed loads.
  Require supported arrays, a stable mapping, unchanged access/store order and
  a whole-unit comparison. Reversed subscript spelling and declaration changes
  do not address this producer; do not cast array pointers to integers or add
  artificial copies to force operand order.

- Trace attribution: an instruction absent from both initial and prescheduled
  PCode need not originate in the frontend. REQUIRE intermediate native pass
  snapshots before naming the cause. Value numbering can create store-forwarded
  `FRSP`s that constant propagation turns into copies and copy propagation
  deletes (`generic_victory_camera`). Inspect the actual producer and surviving
  copy chain; do not infer a missing cast or add synthetic conversions.
  GC2.7's native fold at `0x576eb0` checks the unique FPR definition: `LFS`
  variants, single-precision scalar arithmetic (including fused forms), `FRSP`
  and `FSQRTS` permit replacing the round with `FMR`; `FMR` itself does not.
  Consequently, after the first round becomes a copy, a second round reading
  that copy can survive. REQUIRE the actual definition chain and paired pass
  snapshots; an extra source copy, cast or aggregate parameter is not justified
  merely because it might create this chain. The native instruction-flag guard
  also applies; do not generalize this opcode test to every `FRSP`.

Dead branch-only block to a case body. IF retail has an unreachable `b T`
between a switch dispatch fall-off and the first arm (`T` = a real arm body)
and ours lacks it | REQUIRE mwcc_trace `initial` pcode (or capture `list.py`)
lacks the block with current source | TRY nothing in honest C: classify
`soft_ceiling` with the pcode evidence. Do not request asm.

- Mechanism (GC2.7 `-O>=1`): frontend keeps `case 4: ...` as label + goto;
  IRO folds it (`IroJump.c chain_label` retargets label->goto, `IRO_RemoveLabels`
  Nops the label, `IRO_RemoveUnreachable` deletes the goto). Dead stores in the
  arm are Nop'd by `IROUseDef eliminateUnused` in the same loop, then chained.
  Only `IROLinearAsm` is exempt from both. Backend deletes only dead register
  defs and coalesced copies; a copy in the arm (`x = param`) survives IRO but
  emits `mr`; a separate-arm constant emits a real `li`. Backend
  `optimize_branches` threads `b`-only blocks and never deletes orphans, so
  the orphan exists only if a statement that emits no code survived IRO.
- Empty-label / `case N: break;` trial (H10/H11 backend half) is folded by IRO
  at every level >= 1; `#pragma optimization_level` is the full `-opt level`.
  Only a self-copy `asm { mr x,x }` survives (RE4 CRI pass 66 uses it):
  forbidden here, it is a compiler-quirk stand-in, not retail asm.
- Exemplar ceiling: `mwsfcre_CreateSfd` (99.78, 2 rows, 3696 B): dead `b` at
  +0x598 / +0xaac from the case-4-first arm of `mwsfcre_UsesAudio`.

## Trace procedure

Classify, then fix. Commands are repeatable; examples: notes 7060-7062
(`mwsfcre_CreateSfd`), 7135-7137 (`p_create_profile`), 7061 / 6609 / 6394.

1. Scratch TU (0.1 s): copy the unit .c to scratch; FLAGS = mwcc flags from
   `ninja -t commands build/GQNE5D/src/<unit>.o` minus `-c/-o/-MMD`, plus the
   unit's `-i`; RETAIL = `build/GQNE5D/obj/<unit>.o`. `.agent-work/decomp/
   ai-check-attack/capture/tu.py SRC [FUNC]` = per-function difflib rows;
   `N=40 cmp.py SRC FUNC` = aligned rows. Reproduce DecompStudio rows first.
2. Classify the row from retail ASM (which values trade homes, first
   differing instruction), then `submit {commands:[{cmd:"mwcc_trace",
   draft:<id>, fn:"<fn>", what:["regalloc","inline","pcode"], json:true,
   timeout:300}], wait:true}` (60 s times out on big units; parse the ~1 MB
   JSON with python, never read it). Structure: `pcode` = stages `initial`
   (codegen output, no pcode optimizer), `presched`, two `precolor`;
   `regalloc` per class (3 = FPR, 4 = GPR): `nodes[{color,degree,flags,
   neighbors}]` + `coloring_order`. Operand = 12 bytes: byte0 kind (0 reg, 2
   imm, 3 mem), byte1 class, byte2 flags (1 use, 2 def), int16 at +4 = reg.
3. **Diff `initial` vs `presched` for the block owning the row.** Instruction
   appears/disappears there -> optimizer provenance (value numbering, code
   motion, SR): H29, N14; no decl sweep or permuter can change it. Present in
   both -> coloring: step 4.
4. Real allocator: first try a no-stall replay from the native GPR trace.
   Reconstruct entry degree as `len(neighbors)`, clear pushed flag 2, retain
   coalesced flag 4 and its edges; scan ascending vregs, remove nodes below
   degree 29 and decrement their neighbors. Require the complete reversed
   removal order to equal native `coloring_order` before using a graph
   counterfactual. `handle_trigger_preprocess` reproduces all 105 selected
   nodes: sweeps 98/5/1/1, with PUI removed last at degree 28 = 12 physical +
   16 coalesced neighbors. Selection replay must independently reproduce all
   native colors. A stall, spilled graph, or order mismatch requires entry
   capture and actual spill costs; never infer them from post-pass degrees.
   For entry capture: `PCMIN=250 capf.py SRC OUT.json` (gdb under wibo), `anal.py
   OUT.json K PREFIX` where K = the `simplify_entry` dump index whose node
   count equals the trace's (dumps come in build/simplify/rewrite triples).
   `replay mismatch []` first. Map vregs from first definitions in PREFIX.txt:
   low = real named locals (reverse decl order), then `@N` inline locals and
   params, then expression temps, then SR/CTR regs. Build the per-vreg table
   (our colour vs retail colour), apply simplify/stall/push-tail rules (Hard
   stops), change ONE source fact that moves that node, retrace.
   One command for steps 1-4 (local tool, ~2 s): `.agent-work/tooling/why.py
   SYMBOL [--src FILE]` = scratch compile, row diff with source lines, gdb
   capture (pcode, graphs, colours, every IRO temp's creating pass:
   `split:<var>`, `cse`, `loop-inv/SR`), per-web ours/retail colour table,
   exact replay check, pop-order solver per group of differing webs (required
   flips + source reading), graph what-ifs. `sc.py` = 0.7 s scratch edit loop.
   Vreg order: params < named (reverse decl) < FE temps in reverse order of
   joining the function's locals list (split < CSE < loop < `?:` < inliner
   objects; `@N` is a TU-wide counter, older for objects copied from inline
   bodies, so compare kinds) < codegen temps (emission order) < BE vregs. "No pop order works" = interference differs (lifetime,
   call crossing, ABI coalesce), not numbering.
5. Compiler source for the exact predicate (`~/projects/decomp/mwcc`:
   `ValueNumbering.c`, `CodeGen.c`, `CInline.c`, `IrOptimizer.c`, `IroJump.c`,
   `IROUseDef.c`, `PCodeAssembly.c`), then a one-line micro-TU lab per
   hypothesis (0.1 s each) before touching the real unit.
6. Pitfalls: gdb backtraces are garbage under wibo (read `[esp]`); `eval` of
   `-pragma "cats off"` flags breaks in sh/fish (drive mwcc from python with
   `shlex`); a sibling below 100 shows the same rows in every scratch build
   (read live scores first); `gGPRCoalesceFirst/Last` are not in
   `config/GC_2_7/bindings.json` (infer the window from the vreg table).
   An explicitly C-spelled encoded identifier can coexist with real C++
   overloads of its bare name (`HandleAction__20mkScreenEngineClient...`).
   Do not strip that identifier for native capture: require the full compiler
   name and verify source lines and instruction extent before using graphs.
   A short overload capture with poor alignment is not allocator evidence.

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
    pooled section/string base used more than once (one slot), then codegen
    temps (forward), then BE vregs (SR IVs, CTR counters, spill reloads).
    Single-use pooled base = emission temp at its use. Entry `lis`/`addi`
    section base numbers before codegen temps.
  - Web kinds, in number order (captured, replay exact, `pz_ai_decide_match`):
    block-scoped named (reverse decl) < function-scoped named (reverse decl)
    < 2nd+ webs of a variable (groups in reverse source order of the
    variable's first web, webs in a group forward) < CSE temps (value used
    twice; reverse source) < loop temps (hoisted invariants, strength-reduced
    bases; reverse source) < `?:`/`&&`-mask results and inline-helper
    locals/results (reverse source; NESTED inline expansions number below all
    top-level ones, H30) < plain emission temps (forward). A named
    local assigned once from a `?:` or an inline result is propagated away and
    lives on as that temp.
    Levers measured: a named terminator local for list walks (`link = l.next;
    end = &l; for (; link != end; ...)`) replaces the loop-invariant compare
    temp (`GCSpecSkinLighting` 39 -> 8 rows); inline-helper locals number
    forward in declaration order (`mk_chess` schedule_bonus_row, -27 rows). Multi-def local: 2nd webs from in-place copy
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
    A differing-only pop solver failure does not prove different interference.
    REQUIRE exact/no-stall replay; TRY an extended scan including unchanged
    saved-register claimants (`mk_chess_drone_exchange_ai_casting`). Keep every
    parameter before all locals/temps: an unconstrained solution may violate
    that ABI numbering band. Report crossings into named/inline/CSE kinds as
    missing source ownership, not as declaration edits or proof of matching.
  - Named-local rotation (several saved homes permuted, no stall): one
    "first-init order" decl check is not exhaustive. Capture, map vregs to
    names, write retail pop order, invert into per-sweep ascending push order,
    then pick the decl permutation (reverse decl numbering) that yields it;
    verify in scratch (`__VMSwapPageIn`, 8 locals, one try).
  - Simplify: each pass scans ascending, pushes every web with degree < free
    reg count (29), decrements neighbours at once. Select pops, takes lowest
    free colour claiming r31 downward -> two params pushed in one pass: later
    gets higher reg. A later node reuses the lowest already-obtained
    nonvolatile it doesn't conflict with (capture replay: 188/188 graphs
    exact; "highest obtained" model fails 33).
  - Physical regs + coalesced webs never push, never decrement. Call result
    feeding a move (`x = call()` into multi-def var, inline `return 0 | return
    call()` join) = never-pushed node; one consumed by compare/shift is
    copy-propagated away, doesn't count.
  - Stall (no pushable web, both params at threshold): lowest spillCost/degree
    pushed first -> param with fewest reads ends lower (`immediate`: five
    `cmpwi`, cost 9; `drone` ~37). Cost = sum w*(read 2; 1 under `,s` or for
    an `li`/`lis` remat web) + sum w*(write 1; -1 for remat def or arg-init),
    w = 8 per enclosing loop (1 under `,s`). `addi` off a `lis` vreg is NOT
    remat. Coalesced nodes cost 0. Ties -> highest vreg. Nodes made by an
    earlier spill round never picked; reload temps are new top vregs
    (`pfx_glass_break_run`: captured costs = model on 133 webs).
  - Spill set right, slots/colours wrong: spilled named locals get own slots
    in ascending vreg = reverse decl order. Read decl order of spilled locals
    from retail slot offsets; split decls from assignments to realise it
    (`pfx_glass_break_run`).
  - Entry section/pool base (BE-CSE'd `lis`/`addi`) takes a higher
    nonvolatile than retail, no stall: spill cost irrelevant. Base pushes in
    sweep 1 iff its degree < 29 = physical + unpushed long webs + codegen
    temps numbered above it in its range. TRY honest moves of enough codegen
    temps into named/FE webs; `+0` self-redefinitions are dishonest
    (`MWSFSFX_CnvFrmInfToSfx`: RE4 form exact, not landed).
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
    any<->physical. Temp = vreg in [`first_fe_temporary_register`,
    `last_temporary_register`]: named locals/args never coalesce virtually
    (surviving vreg-vreg `mr` has a named or BE side); BE vregs above
    `last_temporary_register` only with physical (the first BE vreg, equal
    to it, can still coalesce with temps). Nested inline helper `return inner(...);` = move between
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
    `n_real_registers[]` 0x5ea710, `pcbasicblocks` 0x5ea748,
    `first_fe_temporary_register` 0x5ef210 (short[class]),
    `last_temporary_register` 0x5ea640 (int[class]): classify webs as
    named / FE+codegen temp / BE before choosing a lever. Spill rounds:
    capture pass 1 graph too. IGNode (pack 2):
    next 0, spillTemp 4, cost 0xc, degree 0x12, reg 0x14, flags 0x16 (pushed
    2, coalesced 4), count 0x18, neighbours 0x1a. PCode: next 0, op 0x20,
    argCount 0x22, args 0x24 x 12 bytes (kind 0, class 1, reg 4); block
    firstPCode 0x14. Replay simplify/select offline, drop one candidate node,
    compare colours with retail -> names region where source must lose a
    temp. Built-in `debug_listing` is stubbed in this binary.
    More hooks (sw breakpoints work once a hw bp on `main` 0x4024f0 hits):
    `COptimizer_Optimize` 0x500420 entry = per-function, `[esp+4]` Object
    (name: `*(obj+10)` HashNameNode, text at +10); `create_temp_object` ret
    0x4f5ac5 (`eax` = new `@N` temp, `[esp]` = creating pass:
    `split_variable_range` = 2nd web, `move_common_sub` = CSE, IroLoop =
    loop-inv/SR); `setup_inline_locals_and_arguments` 0x5615c0 (`[esp+8]`
    CInlineInfo: nargs 0, arginfo 2, nlocals 6, localinfo 8; CInlineVar 16
    bytes, name at 0) and its ret 0x561a0d (new local Objects at
    `*[0x5e3b5c]`, arg records 12 bytes at `*[0x5e3b58]`) = inline `@N` ->
    helper local name. PCode `sourceoffset` 0x1c = source line; block
    execution weight 0x28. Coloring order: FPR class before GPR, so GPR
    capture sees FPR operands already physical (dump pcode at the first
    build hook).
- Volatile rotation under scoped `opt_*` pragma is NOT a stop until H21
  web-kind check + pragma-free H05 mechanism measured
  (`drone_ai_victim_avoid` closed from recorded 99.44 ceiling: `opt_common_subs
  off` -> address-taken sqrt input + direct global reads).
- Commutative scratch encodings. (`li 0` vs copy of a zero reg is a web-kind
  lever: H15 zero copy, H11 join zero.)
- BE add propagation folding in-place reads onto a param register, survives
  every scoped `opt_*` pragma (`SFHDS_SetHdr`).
- Retail codeless `bcc next; b target` where ours inverts to `bcc target`,
  donor pins it with asm no-op: front end inverts `if (c) goto L1; goto L2;
  L1:` unless a non-label statement sits between; no honest C statement
  survives and emits nothing. Asm permission only (`SFPTS_ReadPtsQue`).
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
