# Decomp playbook: niche / if nothing else matches

Use only after the high- and mid-occurrence books miss. These patterns have low
base rates, narrow preconditions, or high false-positive risk. They are ordered
by combined **likelihood and impact within this tier**, not by cleverness.

| Rank | If A → then B | Preconditions | Solves | O/L/I |
|---:|---|---|---|---|
| 1 | If retail has separate extend/compare or mask/shift ops but ours fuses them, wrap only the function with `#pragma peephole off` / `reset` | The fused peephole is the sole localized mismatch; recheck the whole function | Backend peephole fusion | L/H/H |
| 2 | If retail rematerializes a pool/global address after calls but ours retains it, name pre/post views and scope `#pragma opt_common_subs off` / `reset` | Relocations and repeated materialization are proven | CSE-held NV base | L/H/H |
| 3 | If a latch needs an explicit join, `if (!ok) p = 0;` → `if (ok) { /* keep */ } else { p = 0; }` | ASM has the keep-edge branch; do not use when the latch is returned directly | `beq` versus `bne; b` latch diamond | L/M/M |
| 4 | If one-variable latch still collapses, keep two genuine views, e.g. `live = raw` in the keep arm | Both variables describe real states; abandon on an uncoalesced `mr` | MWCC coalescing at latch joins | L/L/M |
| 5 | If retail emits `cntlzw` booleanization, ordinary equality → `__cntlzw(x) >> 5` or the proven `__rlwnm` form | Exact instruction idiom is visible and semantics are unsigned | Boolean/rotate intrinsic selection | L/H/M |
| 6 | If retail's bitfield RMW remains open across stores, shift/mask assignment → `flags->bit = value` at the retail store position | Overlay, bit position, and byte access are proven | `rlwimi` selection and RMW scheduling | L/H/H |
| 7 | If retail returns low u32 after a call with `mr r3,r4`, scalar callee prototype → `u64` return and `return (u32)call();` | PPC paired return is confirmed at other call sites | u64 return truncation ABI | L/H/H |
| 8 | If an `int` ABI slot ends immediately after cleanup with no r3 initializer, `return 0;` → omit the explicit return | Retail contract tolerates the fall-through value; do not generalize | Extra `li r3,0` | L/H/M |
| 9 | If retail copies exactly two words as `lwz/lwz/stw/stw`, scalar temporaries → one proven two-word POD assignment | Alignment and aliasing are safe and type is real | Pair-copy scheduling | L/H/M |
| 10 | If retail uses `(header + 1)` for trailing payload, `(u8*)h + sizeof(*h)` → `(Payload*)(h + 1)` | Payload immediately follows a complete header | Base arithmetic and register reuse | L/H/M |
| 11 | If a no-arg member wrapper loads `this->field` into r3 for a freestanding twin, instance helper → `static` helper taking the field pointer | Call sites and mangling prove two distinct APIs | thiscall/static ABI confusion | L/H/H |
| 12 | If retail repeats an RTTI/null-cast ladder with no calls, factored helper → duplicate typed inline ladder | RTTI identity checks and null paths are proven | Unwanted helper call and CFG drift | L/H/H |
| 13 | If an attract/SPAWN function is ≥90% and pool/call shapes differ, hand expansion → recovered project macro with exact argument order | Macro definition and multiple expansion sites are known | Macro-specific temporaries and string base | L/M/H |
| 14 | If an external scalar uses `lis` + absolute `lfs` but ours uses SDA, `extern float x; x` → `extern float x[]; x[0]` | Symbol storage is array-shaped in retail | SDA21 versus `ha/l` addressing | L/H/M |
| 15 | If retail byte setter begins by narrowing its full-width argument, `void set(u8 v)` → `void set(u32 v) { bits.field = (u8)v; }` | Public ABI and bitfield store both prove this shape | Missing incoming `clrlwi` | L/H/M |
| 16 | If retail vector copy is `lfs/stfs` per component but `*dst = *src` emits words, aggregate assignment → explicit `.x/.y/.z` assignments | Float access semantics and component order are proven | Integer aggregate copy versus FPR copy | L/H/H |
| 17 | If one inline helper remains out of line despite inline flags, later definition → move its full `static inline` definition before the caller | Body size and lack of retail `bl` prove inlining | MWCC source-order visibility | L/H/M |
| 18 | If a global's address is materialized and stored but ours loads its contents, `extern T *owner` → `extern T owner[]` or real object declaration | ELF symbol kind and all uses prove ownership | Array/object versus pointer declaration | L/H/H |
| 19 | If a tiny deadline predicate changes radically under `-O4,p`, keep clean comparison and test authentic `-O4,s` | TU optimization mode is otherwise supported | Compare lowering (`cntlzw` family versus carry arithmetic) | L/M/H |
| 20 | If retail factory cases each return a subclass pointer, one shared result → direct typed return in each case | Allocation/failure semantics and case order are proven | Common-epilogue return move | L/M/M |

## If nothing matches: stop conditions

If the remaining diff is any item below, record a soft ceiling and stop:

- identical memory operations with only GPR/FPR color swaps;
- `li 0` versus `mr` from an already-zero register;
- three-address versus two-address commutative scratch selection;
- frameless PLATFORM `mtlr`/`blrl` emission;
- relocation-label noise with otherwise byte-identical operations;
- a register “carousel” where every harmless declaration change only rotates
  the mismatch;
- parameter NV homes that do not respond to honest lifetime changes.

Never respond with `register`, fake `volatile`, dead sinks, empty branches,
incorrect prototypes, invented fields, or manual conversion scaffolds. Use:

```c
/* Soft ceiling: <symbol> ~N% — <localized compiler-coloring mismatch>; stop. */
```

