# Decomp playbook: high occurrence

Use this first. Entries are ordered by combined **occurrence / likelihood / impact**.
Each edit must express a retail-supported type, lifetime, or control-flow fact; it
must not exist only to select a register. Try one row, rebuild, and inspect the
same mismatch island.

Score key: `H/H/H` = occurrence / likelihood that the smell identifies the cause
/ typical impact. Preconditions are mandatory.

| Rank | If A (observed code or ASM) → then B (mechanical source edit) | Preconditions | Solves | O/L/I |
|---:|---|---|---|---|
| 1 | If call-site values reach the wrong argument GPRs—or an indirect call preserves an argument in `r3` while materializing its function-table base in `r4`—correct the declaration and call, e.g. `f(b, a)` or `cb(value)` instead of `cb()` | Check every retail caller and loads immediately before `bl`/`bctrl`; the callback implementation or semantics must prove the argument | ABI/prototype error; r3/r4 swaps; false zero-arg callbacks | H/H/H |
| 2 | If field displacements or access widths disagree, `*(T*)((u8*)p + N)` → `p->field` with a proven offset/type | Confirm offset, width, signedness, and layout from multiple accesses | Bad struct layout; wrong `lwz/stw/lbz/stb` immediates | H/H/H |
| 3 | If a known pointee is `void*`/`void**`, `void **out` → `Thing **out` and update declarations/callers together | Calls prove pointee and indirection; preserve ABI qualifiers | False casts, wrong aliasing, argument coloring | H/H/H |
| 4 | If retail compares unsigned but ours emits signed compare, `int n` → `u32 n`; `n != 0` → `n > 0` only when `cmplwi` is proven | Load width and callers prove unsigned semantics | `cmpwi`/`cmplwi`, branch polarity | H/H/M |
| 5 | If the algorithm matches but nonvolatile GPRs are swapped, `T *a; U *b;` → swap adjacent declaration order or narrow one local to its use block | Same memory ops/CFG; locals are genuine semantic values | MWCC live-range coloring and `stmw` set | H/M/M |
| 6 | If retail keeps `base + index*stride` live, repeated `items[i].x` → `T *item = &items[i]; item->x` | Several accesses share the exact proven element | Address CSE; base/index register allocation | H/H/M |
| 7 | If retail reloads a field/global after a call, cached `v` → repeat `obj->field` or name separate pre/post-call views | ASM shows a second load/materialization; call may clobber state | CSE lifetime; retained NV register | H/H/M |
| 8 | If retail has no `bl` for a helper, `helper(x)` → a typed `static inline` body visible before the caller or structured open code; when the authentic TU uses `-inline off` and repeats the sequence, use a local macro that captures each typed operand and link exactly once | Callee body and call boundary are proven; preserve reusable intent; macro operands must be side-effect safe and locally cached | Inline decision, extra frame/call, repeated intrusive-list lowering | H/H/H |
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
| 19 | If retail open-codes an intrusive-list insertion and keeps the inserted node in a nonvolatile register only for the final head store, spell the four link stores directly and introduce a typed node alias immediately before that final store | Retail has no helper `bl`; list layout and store order are proven; the alias represents the inserted node | Hidden helper calls; intrusive-list store order; node live range | M/H/H |
| 20 | If retail open-codes an intrusive-list removal and reloads the previous link between reciprocal stores, spell the first store through the member expression, then assign a typed previous-link local for the second store | Retail has no helper `bl`; next/previous offsets and store order are proven; the local represents the real previous node | Hidden helper calls; reciprocal unlink stores; previous-node live range | M/H/H |
| 21 | If retail preserves `r3` from an immediately preceding setup/producer call while materializing later consumer arguments, give the producer its proven typed return and nest it as the consumer's first argument | The producer leaves a real object pointer in `r3`; the consumer's callee and every caller prove the argument contract; the producer can honestly return that object | False `void` producer prototypes; missing first arguments; redundant return-value rematerialization | M/H/H |
| 22 | If a second packed table starts immediately after a counted trailing array, `&items[count - 1] + 1` or byte-offset arithmetic → `&items[count]` | Retail offsets prove identical element stride and that the second table begins at the first array's one-past element | Opaque packed-table arithmetic; extra add/sub scheduling; array-base coloring | M/H/M |
| 23 | If a callback payload is given a typed alias for member access but retail preserves the original payload in a separate nonvolatile register for a downstream `const void*` argument, pass the original callback parameter at that boundary instead of the alias | The two pointers are value-identical; the callee prototype intentionally accepts the untyped owner; retail loads members through the alias but moves the original value into the call GPR | Collapsed owner/typed-view lifetimes; callback payload spills; argument coloring | M/H/H |
| 24 | If a pointer-returning callback uses `return call(...) != 0 ? input : 0` and retail branches directly to `return input` or `return 0` without a joined result temporary, rewrite it as an explicit `if` followed by the null return | Retail tests the same call or stored result, both branches have no additional side effects, and the return values are identical | Extra result nonvolatile; oversized save set; ternary join lowering | M/H/H |
| 25 | If decompiled source contains an impossible direct test such as `&global_list != 0`, recover the nullable abstraction as a typed inline predicate, then guard the original list walk with that predicate | Retail emits the defensive address test; the caller passes a concrete list reference; multiple consumers prove a reusable nullable-list contract; the helper introduces no standalone symbol | Decompiler-residue address checks; preserves retail defensive CFG without dishonest C | M/H/H |

## High-frequency guardrails

- If the function is below about 90% and CFG, offsets, or calls differ, fix the
  algorithm/type story before using scheduling levers.
- If a clean typing change is 0-delta or improves the diff, keep it.
- If only GPR/FPR names differ with identical memory operations, stop. Do not add
  `register`, fake `volatile`, dead reads, sinks, or wrong prototypes.
- If an edit changes semantics, store order, access width, or ABI without retail
  evidence, reject it even when fuzzy improves.
