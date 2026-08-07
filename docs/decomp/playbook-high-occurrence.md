# Decomp playbook: high occurrence

Use this first. Entries are ordered by combined **occurrence / likelihood / impact**.
Each edit must express a retail-supported type, lifetime, or control-flow fact; it
must not exist only to select a register. Try one row, rebuild, and inspect the
same mismatch island.

Score key: `H/H/H` = occurrence / likelihood that the smell identifies the cause
/ typical impact. Preconditions are mandatory.

| Rank | If A (observed code or ASM) → then B (mechanical source edit) | Preconditions | Solves | O/L/I |
|---:|---|---|---|---|
| 1 | If call-site values reach the wrong argument GPRs, `f(a, b)` → correct the declaration and call order, e.g. `f(b, a)` | Check every retail caller and loads immediately before `bl` | ABI/prototype error; r3/r4 swaps | H/H/H |
| 2 | If field displacements or access widths disagree, `*(T*)((u8*)p + N)` → `p->field` with a proven offset/type | Confirm offset, width, signedness, and layout from multiple accesses | Bad struct layout; wrong `lwz/stw/lbz/stb` immediates | H/H/H |
| 3 | If a known pointee is `void*`/`void**`, `void **out` → `Thing **out` and update declarations/callers together | Calls prove pointee and indirection; preserve ABI qualifiers | False casts, wrong aliasing, argument coloring | H/H/H |
| 4 | If retail compares unsigned but ours emits signed compare, `int n` → `u32 n`; `n != 0` → `n > 0` only when `cmplwi` is proven | Load width and callers prove unsigned semantics | `cmpwi`/`cmplwi`, branch polarity | H/H/M |
| 5 | If the algorithm matches but nonvolatile GPRs are swapped, `T *a; U *b;` → swap adjacent declaration order or narrow one local to its use block | Same memory ops/CFG; locals are genuine semantic values | MWCC live-range coloring and `stmw` set | H/M/M |
| 6 | If retail keeps `base + index*stride` live, repeated `items[i].x` → `T *item = &items[i]; item->x` | Several accesses share the exact proven element | Address CSE; base/index register allocation | H/H/M |
| 7 | If retail reloads a field/global after a call, cached `v` → repeat `obj->field` or name separate pre/post-call views | ASM shows a second load/materialization; call may clobber state | CSE lifetime; retained NV register | H/H/M |
| 8 | If retail has no `bl` for a helper, `helper(x)` → a typed `static inline` body visible before the caller or structured open code | Callee body and call boundary are proven; preserve reusable intent | Inline decision, extra frame/call | H/H/H |
| 9 | If retail has a `bl` but ours expands the body, inline helper → out-of-line helper; use scoped `#pragma dont_inline` only if source visibility alone cannot preserve the call | Retail call is unambiguous and signature is correct | Accidental inlining and caller-wide scheduling drift | H/H/H |
| 10 | If retail branches/tests at the loop bottom, `while (test) { body; }` → `do { body; } while (test);` or assignment-in-condition | Initial-entry behavior and empty case are proven equivalent | Loop latch, CTR/NV lifetime, branch sense | H/H/H |
| 11 | If retail has a dense compare tree/jump table, nested `if` → `switch`, then order source cases like retail `.text` | Cases/default and fallthrough behavior are proven | Jump-table selection and case emission order | H/H/H |
| 12 | If retail preserves a value across calls, recomputed expression → one named typed local; if retail reloads it, do the inverse | Diff shows keep-versus-rematerialize, not changed semantics | Live range, save/restore set, register coloring | H/M/M |
| 13 | If address-taken locals use reversed stack slots, `T a; U b; f(&a,&b);` → reorder those declarations | Retail stack offsets and call order are known | Stack-slot allocation and downstream frame offsets | H/H/M |
| 14 | If a direct return is moved through a scratch, `T x = make(); use(x);` → `use(make());` | Result has one use and no required lifetime/side effect | Return-to-argument coalescing | H/H/M |
| 15 | If retail copies a fixed POD with a compact word/CTR sequence, manual member loop → `*dst = *src` | Type is POD; size/alignment and copy semantics are proven | Aggregate lowering, address formation, stack lifetime | H/M/H |
| 16 | If retail uses natural signed-int-to-float conversion, volatile `0x4330` scaffold → `(float)value` | Input signedness and output precision are proven | Native `xoris/lfd/fsubs`; removes fake volatile spills | M/H/H |
| 17 | If a member load/store width is wrong, broad scalar type → exact `u8/u16/u32` member while keeping promoted locals natural-width | ABI and stored width are proven independently | Extend/truncate instructions and narrow-store scheduling | H/H/H |
| 18 | If retail has one joined result block, multiple early returns → assign a typed `result` in each arm and `return result` once | CFG and side effects prove a common join | Tail duplication, extra `li`, branch layout | H/M/H |

## High-frequency guardrails

- If the function is below about 90% and CFG, offsets, or calls differ, fix the
  algorithm/type story before using scheduling levers.
- If a clean typing change is 0-delta or improves the diff, keep it.
- If only GPR/FPR names differ with identical memory operations, stop. Do not add
  `register`, fake `volatile`, dead reads, sinks, or wrong prototypes.
- If an edit changes semantics, store order, access width, or ABI without retail
  evidence, reject it even when fuzzy improves.

