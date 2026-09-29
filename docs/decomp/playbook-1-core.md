# Matching playbook, tier 1: core

Read this tier on every matching task. The tiers are ordered by how rare the
situation is; open a deeper tier only when the triage table sends you there.

| Tier | File | Scope |
|---|---|---|
| 1 | this file | protocol, triage, H-rule index, stops |
| 2 | [playbook-2-common.md](playbook-2-common.md) | H01-H25 detail: addenda, examples, traps |
| 3 | [playbook-3-uncommon.md](playbook-3-uncommon.md) | M01-M17: compiler modes, FP, ABI shape, inline/macro, data and link layout |
| 4 | [playbook-4-rare.md](playbook-4-rare.md) | N01-N12, hard-stop detail, compiler revisions, permuter |

Rule format: `ID | IF mismatch | REQUIRE evidence | TRY one change`. Missing
evidence means skip the rule; no fitting rule means investigate or stop. These
are diagnostics, not recipes. Rule IDs are stable across tiers; cite them in
notes and reports. DecompStudio `docs {op: read, path, section: "H05"}` returns
one rule section from tiers 2-4. Named symbols are exemplars that reached
exact; their measurements live in `.agent-work/decomp/` reports and git
history (the pre-tier high/mid/niche books are at commit 4abd1af).

## Protocol

- Authority: AGENTS.md > retail ASM, callers, ELF, layout > m2c, Ghidra,
  donor ports (RE4, bfbb, TP), Mirage, permuter. The latter are hypotheses.
- Loop: baseline -> classify -> one rule -> rebuild -> same-symbol objdiff ->
  keep or revert. Update the `TODO: [status]` comment below 100 per AGENTS.md;
  remove progress comments at a measured 100.
- Preserve math, stores, call order, access widths, and lazy null checks.
- Never: forced registers, fake `volatile`, dead sinks, empty arms, invented
  fields/lifetimes/padding, wrong prototypes, undefined returns, asm
  workarounds, `(void)param`. `goto` only under the AGENTS.md last-resort
  exception. Real MMIO stays `volatile`.

Acceptance gates for any gain:

- Judge per function and per instruction. A TU score can rise while one
  function regresses.
- Check both report fuzzy (`build/GQNE5D/report.json`) and `objdiff-cli diff`;
  they can rank variants differently.
- Run data-value mode (`-c functionRelocDiffs=data_value`). Report-100 can hide
  wrong floats or strings, and different float constants can score the same.
- Read differing offsets and immediates even above 99%. One wrong offset is a
  wrong member, not residue. Large score swings can be diff-alignment artifacts.
- If the function has a jump table, check `.data` too (H10).
- Recheck every shared consumer of a changed header, helper, or macro,
  including already-exact ones.
- Finish: quality pass, full `ninja`, SHA-1, progress, `git diff --check`,
  status. Distinguish report-exact, data-value-exact, and link-exact; disclose
  nonmatching fallbacks.

Measured priority when several rules apply: H02 owner/layout, then H11 joins,
then an applicable M01 TU-mode check, then one H15 lifetime insight. ABI,
width, and effect-order evidence override this order. Check shared consumers
before spending repeated attempts on an isolated leaf.

Tool preparation:

- Ghidra MCP: verify the project and select the program explicitly. If
  pseudocode looks unrelated, compare raw bytes and ELF symbol bounds.
- m2c rejects directives in context: preprocess a scratch copy with the
  project's include paths and defines and pass that. Host preprocessing is
  parser preparation only; union-member and data-base inferences still need
  retail offsets and relocations.
- m2c cannot resolve a branch to the function entry: give that address a local
  label in a scratch assembly copy and retarget only that branch. It is a
  self-entry retry loop, not recursion.

## Triage

| Diff symptom | Rules |
|---|---|
| Wrong argument/return registers, extra or missing argument | H01, M10, M11 |
| Wrong load/store offset or width, opaque pointer owner | H02, M07 |
| Signed vs unsigned compare, narrowing, `addis`+`cmplwi` | H03 |
| Bit extract/insert, `rlwimi`, masks, packed bytes | H04, M06 |
| Missing or extra reload of a global/owner | H05, M12 |
| Retail keeps a value/address that source recomputes, or the reverse | H06 |
| Extra or missing `bl`, inlining differs | H07, M13 |
| Pointer+instance latch diamond | H08, H25 |
| Loop entry/latch, CTR use | H09 |
| Switch, jump table, compare tree | H10 |
| Return/cleanup join, materialized Boolean | H11, H19, M04, M08 |
| POD copy loop | H12 |
| Intrusive list link order | H13 |
| Stack slots, frame size, store order | H14, H24 |
| Same operations, registers swapped | H15, H21, H22, then tier-4 stops |
| Call result move order | H16 |
| Residue in an `-opt off` TU | H17, M01 |
| Fixed address `0x800000xx` / `0xCC00xxxx` | H18 |
| Inlined state struct colored late | H20 |
| Table base rematerialized; object belongs to this unit | H23 |
| Compact saves, `divw`, Boolean lowering across the TU | M01 |
| Control-word or publication order | M02 |
| FP operands, rounding, constants, int-to-float | M03, M05 |
| Varargs setup | M14 |
| Strings, pools, `.rodata`, constant identity | M15 |
| `.bss`/`.sbss` order, SDA, SHA fails at report-100 | M16, N02 |
| Vtables, weak symbols, C++ emission order | M17 |
| Intrinsic idioms, RTTI ladders, reverse walks, factories | N01-N05 |
| Peephole/scheduler residue, compiler revision | N06, N11 |
| Runtime scratch faults after object growth | N07 |
| Byte-swap folding, float word view, uninitialized stack | N08-N10 |
| Assembly-sequence ICE | N12 |
| Only coloring left, permuter use | tier 4 |

## H-rule index

Core statement only; each rule's detail is its `## Hxx` section in tier 2.

- **H01 ABI** | wrong arg/return registers | all callers + callee ABI | fix
  declarations, definitions, callbacks, and calls together with canonical typed
  pointers/virtuals. A live register at `bl` is not an argument unless the
  callee reads it before clobbering it. No invented argument or unused return.
- **H02 Layout** | wrong offset/width or opaque owner | multiple accesses +
  allocation/compiler layout | correct the proven member; keep unknown gaps as
  offset-named members; verify producers and consumers.
- **H03 Signedness** | signed compare/narrowing differs | loads, callers,
  arithmetic range | correct storage/ABI width; narrow at the real boundary
  once; decode full FP CR predicates.
- **H04 Bits** | extraction/RMW differs | storage width, bit position, every use |
  existing bitfield or proven mask; unsigned-byte promotion before shift.
- **H05 Reload** | cached value vs retail reload | call, sleep, aliasing store,
  poll boundary | reread the authoritative owner at each observed boundary.
- **H06 Retained value** | retail retains a computed value/address | shared uses +
  unchanged ownership interval | name genuine typed owners, elements, indices;
  reject redundant aliases.
- **H07 Inline boundary** | extra/missing helper call | retail call boundary,
  signature, inline settings | visible inline body for expansion, out-of-line
  body for a call; check scoped auto_inline and dont_inline effects.
- **H08 Latch** | pointer-instance latch diamond differs | null-before-instance
  reads, no call | typed accessor returning the validated pointer or null;
  reuse existing accessors.
- **H09 Loop** | loop entry/latch differs | zero-iteration behavior, test/update
  order | recover while/do/for; snapshot a CTR bound; no dummy one-trip loop.
- **H10 Switch** | dispatch differs | full case/default/fallthrough set + text
  order | enumerate every routed value; recover case order; check the jump
  table's `.data`.
- **H11 Join** | return/cleanup join differs | branch destinations + effect
  ownership | trace each failure edge to its destination; shape result
  initialization and returns to retail's joins.
- **H12 POD copy** | copy loop differs | real type, size, alignment, alias
  semantics | aggregate assignment for word/CTR copies, components for `lfs`/`stfs`.
- **H13 Intrusive list** | link accesses differ | link ownership, callback
  effects | exact reciprocal-store order; save next before a mutating callback.
- **H14 Stack** | slots or store order differ | address-taken locals, offsets,
  lifetimes | reorder declarations/aggregates or narrow scope; addressed locals
  get slots in reverse declaration order. No padding locals.
- **H15 Coloring** | same operations, registers swapped | same CFG and memory
  accesses | check expression association and staging first, then at most one
  honest lifetime/declaration insight, then stop.
- **H16 Producer/consumer** | result move order differs | real returned object +
  consumer ABI | nest a single-use result; `g = f(); use(g);` forwards a store.
- **H17 `-opt off`** | residue in an opt-off TU | TU really builds opt off |
  unread stores are source evidence; nonvolatile homes rank by reference count.
- **H18 Absolute address** | fixed-address access scheduled or CSE'd differently |
  retail addressing | MWCC absolute-address variables, not pointer-cast macros.
- **H19 Materialized Boolean** | retail keeps `li 1; b; li 0; cmpwi` | inline
  predicate | `if (c) return 1; return 0;` or macro `(c) == 0 ? 0 : 1`.
- **H20 State struct** | whole-function rotation around an inlined state struct |
  retail colors state first | declared scalar locals driven by macros.
- **H21 Helper locals** | inlined loop locals colored above loop temps | retail
  reuses dead outer registers | open-code and reuse existing outer locals.
- **H22 One-row residue** | operand, constant, or copy residue in one row |
  single localized row | try one listed spelling at a time.
- **H23 Unit-owned data** | base rematerialized at each use | symbol lives in
  this object | define the object in the unit with retail constness and order.
- **H24 Inline frame slots** | body exact, frame larger by N x k | N expansions
  of one helper | single-exit result local of the right type in the helper.
- **H25 Latch macro argument** | macro replacement of a thin latch wrapper
  regresses | argument read before the null test | copy the argument to a typed
  local first, or keep the helper.

## Stop

- Unknown calls, offsets, or CFG are not coloring: classify `borked` or
  `breakthrough needed` and name the missing evidence.
- Only coloring left after one honest lifetime/declaration check: soft ceiling
  (tier 4 Hard stops). Record `TODO: [near miss]` with score and residual.
- Exhausted declaration or spelling searches are not new evidence. Reopen a
  ceiling only for a changed lifetime, CFG, or owner hypothesis.
- Functions that need assembly are skipped unless the user grants per-function
  permission (AGENTS.md).

## Maintaining the tiers

Amend the existing rule in the tier where it lives: precondition, action, one
exemplar symbol. No percentages, attempt history, diaries, or duplicate rows;
put measurements in `.agent-work/decomp/<campaign>/`. Move a rule to a lower
tier number when it recurs across campaigns, and to a higher one when it
proves rare.
