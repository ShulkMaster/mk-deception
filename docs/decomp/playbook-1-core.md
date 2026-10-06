# Playbook tier 1: core

Read every task. Deeper tier only when triage points there.

| Tier | File | Holds |
|---|---|---|
| 1 | this | protocol, gates, triage, H index, stops |
| 2 | [playbook-2-common.md](playbook-2-common.md) | H01-H27 detail |
| 3 | [playbook-3-uncommon.md](playbook-3-uncommon.md) | M01-M17: modes, FP, ABI shape, inline/macro, data/link |
| 4 | [playbook-4-rare.md](playbook-4-rare.md) | N01-N13, hard stops, dead ends, permuter |

Rule = `IF mismatch | REQUIRE evidence | TRY one change`. No evidence -> skip
rule. No rule fits -> investigate or stop. Diagnostics, not recipes. IDs
stable across tiers; cite in notes (`H05 closed`). One section:
DecompStudio `docs {op: read, path, section: "H05"}`. Named symbols =
exemplars that reached exact; measurements live in history/reports.

Compilers (`configure.py`): game + CRI GC/2.7, Dolphin SDK GC/1.2.5n, RW
GC/1.3.2. `[da]` = imported from sibling mk-da (GC/1.2.5 + 1.3.2 evidence,
mk-da symbols). Recheck under GC/2.7 before relying on it for game/CRI code.

## Protocol

- Authority: AGENTS.md > retail ASM, callers, ELF symbols/sections, layout >
  donors (RE4, bfbb, TP, pikmin2) > m2c, Ghidra, Mirage, permuter. Lower =
  hypothesis.
- Loop: notes+history -> baseline -> triage -> one rule -> rebuild ->
  same-symbol objdiff -> keep/revert -> note. `TODO: [status]` line below 100
  per AGENTS.md; delete progress comments at measured 100.
- Pre-screen on host `[da]`: get exact TU command (`ninja -t commands
  <obj>`), compile variants with project mwcceppc, objdump symbol vs retail.
  Cheap. Do this before spending `try` attempts.
- Scratch may use dirty tricks (pragmas, aliases, `volatile`, permuter
  residue) as leads `[da]`. Lead = evidence of shape retail wants, never the
  answer. Find honest source doing same thing.
- Keep math, stores, call order, access widths, lazy null checks.
- Never land: `register`, fake `volatile`, dead sinks, empty arms, invented
  fields/lifetimes/padding, wrong prototypes, undefined returns, asm,
  `(void)param`, bare `{ }` (no `if`/loop/`else` owner) to scope a local,
  copy of in-scope value, reuse of unrelated variable, local redeclaration
  where real header exists. `goto` only per AGENTS.md exception. Real MMIO
  stays `volatile`.

## Gates

- Judge per function, per instruction. TU score up can hide one regression.
- Check report fuzzy (`build/GQNE5D/report.json`) and `objdiff-cli diff`.
  They can rank variants differently.
- Run `-c functionRelocDiffs=data_value`. Report-100 can hide wrong
  floats/strings; different float constants can score equal.
- Read every differing offset/immediate, even >99%. One wrong offset = wrong
  member, not residue. Big score swings can be alignment artifacts.
- Per-symbol 100 does not prove layout. Check raw offsets/sizes of statics
  (`.sbss` order) and anonymous literal pool order (reversed pair: each
  string matches, relocs wrong) `[da]`.
- Jump table -> check `.data` too (H10): table count and raw section size,
  not only fuzzy score. Removed tables can leave that score unchanged
  (`mk_chess_piece_event`).
- Changed header/helper/macro -> recheck every consumer, exact ones too.
- Finish: quality pass, full `ninja`, SHA-1, progress, `git diff --check`,
  status. Report report-exact vs data-value-exact vs link-exact; disclose
  nonmatching fallbacks.

Several rules apply: H02 owner/layout -> H11 joins -> M01 TU mode -> one H15
lifetime insight. ABI/width/effect-order evidence overrides order. Check
shared consumers before grinding an isolated leaf.

Tools:

- Ghidra MCP: verify project, select program explicitly. Pseudocode looks
  unrelated -> compare raw bytes and ELF symbol bounds.
- m2c rejects directives in context: preprocess scratch copy with project
  include paths/defines. Union-member and data-base guesses still need
  retail offsets/relocs.
- m2c can't resolve branch to function entry: local label in scratch asm,
  retarget only that branch. Self-entry retry loop, not recursion.

## Triage

| Diff symptom | Rules |
|---|---|
| Wrong arg/return regs, extra/missing arg | H01, M10, M11 |
| Wrong load/store offset or width, opaque owner | H02, M07 |
| Signed vs unsigned compare, narrowing, `addis`+`cmplwi` | H03 |
| Bit extract/insert, `rlwimi`, masks, packed bytes | H04, M06 |
| Missing/extra reload of global or owner | H05, M12 |
| Retail keeps value/address source recomputes, or reverse | H06 |
| Retail derives value in the register it loaded into | H26 |
| Load/store/pointer-advance order differs, same ops | H27, H14 |
| Call args from one value computed in other order | H15, H16 |
| Extra/missing `bl`, inlining differs | H07, M13 |
| Retail compares value with itself before shared inlined loop | H07 |
| Pointer+instance latch diamond | H08, H25 |
| Loop entry/latch, CTR; preheader after final return | H09 |
| Prologue order differs, loop body exact | H09, H27 |
| Switch, jump table, compare tree | H10 |
| Return/cleanup join, materialized Boolean | H11, H19, M04, M08 |
| POD copy loop; copy load/store order | H12 |
| Intrusive list link order | H13 |
| Stack slots, frame size, store order | H14, H24 |
| Local struct in stack (`lfs`/`stfs` off r1) vs FPRs, or reverse | M07 |
| Dead `b` after loop arm, `li r,0` on normal exit | H11, M13 |
| `li rX,0` vs `mr rX,rZero` | H15, H11 |
| Same ops, regs swapped | H15, H21, H22, then tier 4 stops |
| Add/or operands commuted in one row | H15, H22 |
| Call result move order | H16 |
| Residue in `-opt off` TU | H17, M01 |
| Fixed address `0x800000xx` / `0xCC00xxxx` | H18 |
| Inlined state struct colored late | H20 |
| Table base rematerialized; object owned by this unit | H23 |
| Compact saves, `divw`, Boolean lowering TU-wide; `_savegpr` vs `stmw` | M01 |
| Scheduling-only residue across one library | M01 |
| Control-word or publication order | M02 |
| Near miss with `const` anywhere in sig/locals (Midway rarely used it) | M12 |
| FP operands, rounding, constants, int-to-float, `fmadds` | M03, M05 |
| Varargs setup | M14 |
| Strings, pools, `.rodata`, constant identity, `[order]` | M15 |
| `.bss`/`.sbss` order, SDA, SHA fails at report-100 | M16, N02 |
| Vtables, weak symbols, C++ emission, extab | M17 |
| Intrinsic idioms, RTTI ladders, reverse walks, factories | N01-N05 |
| Peephole/scheduler residue, compiler revision | N06, N11 |
| Runtime scratch faults after object growth | N07 |
| Byte-swap folding, float word view, uninit stack | N08-N10 |
| Asm-sequence ICE | N12 |
| Several honest byte-neutral edits, no single fix | N13 |
| Only coloring left, permuter | tier 4 |

## H index

Core line only; detail = `## Hxx` in tier 2.

- **H01 ABI** | wrong arg/return regs | all callers + callee ABI | fix decls,
  defs, callbacks, calls together; canonical typed pointers/virtuals. Reg
  live at `bl` is arg only if callee reads it before clobber. No invented arg
  or unused return.
- **H02 Layout** | wrong offset/width, opaque owner | multiple accesses +
  allocation/compiler layout | fix proven member; unknown gap = offset-named
  member; verify producers + consumers.
- **H03 Signedness** | signed compare/narrowing differs | loads, callers,
  range | right storage/ABI width; narrow once at real boundary; decode full
  FP CR predicate.
- **H04 Bits** | extract/RMW differs | width, bit pos, every use | existing
  bitfield or proven mask; unsigned byte promotion before shift.
- **H05 Reload** | cached vs retail reload | call, sleep, aliasing store,
  poll | reread authoritative owner at each observed boundary.
- **H06 Retained value** | retail keeps computed value/address | shared uses
  + unchanged ownership | name real typed owner/element/index; no redundant
  alias.
- **H07 Inline boundary** | extra/missing helper call | retail call boundary,
  signature, inline settings | visible inline body = expand, out-of-line =
  call; check scoped auto_inline/dont_inline.
- **H08 Latch** | pointer-instance diamond differs | null-before-instance
  reads, no call | typed accessor returning validated ptr or null; reuse
  existing.
- **H09 Loop** | entry/latch differs | zero-trip behavior, test/update order |
  recover while/do/for; snapshot CTR bound; no one-trip dummy loop.
- **H10 Switch** | dispatch differs | full case/default/fallthrough set + text
  order | every routed value; case order; check jump table `.data`.
- **H11 Join** | return/cleanup join differs | branch targets + effect
  ownership | trace each failure edge; shape result init/returns to retail
  joins.
- **H12 POD copy** | copy loop differs | real type, size, align, alias |
  aggregate assign for word/CTR, components for `lfs`/`stfs`.
- **H13 Intrusive list** | link accesses differ | link ownership, callback
  effects | exact reciprocal-store order; save next before mutating callback.
- **H14 Stack** | slots/store order differ | address-taken locals, offsets,
  lifetimes | reorder decls/aggregates or narrow scope into existing CFG
  block; addressed locals = reverse decl order. No padding, no bare `{ }`.
- **H15 Coloring** | same ops, regs swapped | same CFG + memory | association
  and staging first, then max one honest lifetime/decl insight, then stop.
- **H16 Producer/consumer** | result move order | real returned object +
  consumer ABI | nest single-use result; `g = f(); use(g);` forwards store.
- **H17 `-opt off`** | residue in opt-off TU | TU really opt off | unread
  stores = source evidence; nonvolatile homes rank by ref count.
- **H18 Absolute address** | fixed-address access scheduled/CSE'd different |
  retail addressing | MWCC absolute-address vars, not pointer-cast macros.
- **H19 Materialized Boolean** | `li 1; b; li 0; cmpwi` | inline predicate |
  `if (c) return 1; return 0;` or `(c) == 0 ? 0 : 1`.
- **H20 State struct** | whole-function rotation around inlined state struct
  | retail colors state first | declared scalar locals driven by macros.
- **H21 Helper locals / web kind** | helper loop locals colored above loop
  temps; helper copies under `opt_propagation off`; volatile rotation |
  retail reuses dead outer regs; kind sets web number | open-code + reuse
  outer locals; copy-free helper; change web kind before decl sweeps.
- **H22 One-row residue** | operand/constant/copy residue in one row |
  single row | try one listed spelling at a time.
- **H23 Unit-owned data** | base rematerialized each use | symbol in this
  object | define in unit, retail constness + order.
- **H24 Inline frame slots** | body exact, frame larger N x k | N expansions
  of one helper | single-exit result local of right type in helper.
- **H25 Latch macro arg** | macro replacing thin latch wrapper regresses |
  arg read before null test | copy arg to typed local first, or keep helper.
- **H26 In-place derivation** `[da]` | retail derives value in its load
  register | one meaning for variable | compute in steps in one local.
- **H27 Statement order** `[da]` | loads/stores/advances ordered differently
  | no alias between moved accesses | write statements in retail order.

## Stop

- Unknown calls/offsets/CFG != coloring. Classify `borked` or `breakthrough
  needed`, name missing evidence.
- Only coloring after one honest lifetime/decl check -> soft ceiling (tier 4
  Hard stops). `TODO: [near miss]` + score + residual.
- Exhausted decl/spelling searches are not new evidence. Reopen ceiling only
  for new lifetime, CFG, or owner hypothesis.
- Before ceiling: N13 subset sweep of recorded byte-neutral edits.
- Needs assembly -> skip unless user grants per-function permission
  (AGENTS.md).

## Maintaining

Amend rule where it lives: precondition, action, one exemplar. No
percentages, attempt logs, diaries, duplicate rows; measurements go to
`.agent-work/decomp/<campaign>/` or DecompStudio history. Recurs across
campaigns -> lower tier number; proves rare -> higher. Caveman style: short,
exact, no filler, never ambiguous.
