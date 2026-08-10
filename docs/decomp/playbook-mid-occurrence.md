# Decomp playbook: mid occurrence

Use after the high-occurrence playbook leaves a localized mismatch. Entries are
ranked by combined **occurrence / likelihood / impact**. Apply one evidence-backed
source-shape change per rebuild.

| Rank | If A → then B | Preconditions | Solves | O/L/I |
|---:|---|---|---|---|
| 1 | If retail loads a control word before subfield stores, `obj->sync \|= bit` → `u32 sync = obj->sync; sync \|= bit; ...; obj->sync = sync;` | Store order and authoritative field are proven | Load/store scheduling; NV lifetime | M/H/H |
| 2 | Match the retail plugin-offset lifetime: retail `lwzx/stwx` → direct typed access from `object + PluginOffset + field_off`; retail `add extensionBase` followed by immediate field stores → assign a named typed extension pointer after any preceding base-object stores | Dynamic extension mechanism, field offsets, and store order are proven | Indexed versus immediate addressing; extension-base NV lifetime | M/H/H |
| 3 | If count is at `+0` and ILP32 entries at `+4`, raw byte casts → typed `{ u32 count; u32 refs[1]; }` or one localized byte-offset accessor | Packed layout, stride, and relocation representation are proven | Packed-table address formation and coloring | M/H/H |
| 4 | If retail emits `stmw/lmw` or a compact counted copy, current object at `-O4,p` → test object-scoped `-O4,s` and, if needed, `-use_lmw_stmw on` | Compiler flags are authentic; verify every exact function in the TU | Frame/save strategy, loop lowering | M/M/H |
| 5 | If retail loads a float operand pair in a different order, `p * x2 + c` → `x2 * p + c` without changing grouping | Floating semantics tolerate only the proven equivalent order | `fmadds` operand encoding and FPR schedule | M/H/M |
| 6 | If retail stages related factors before combining, nested expressions → named object-level intermediates, then final element combinations | ASM shows the common values loaded/computed first | FPR scheduling and live ranges | M/H/H |
| 7 | If compare immediates differ by one, `x > N-1`/`x <= N-1` → `x >= N`/`x < N` | Boundary semantics are exactly equivalent | Compare immediate and branch selection | M/H/M |
| 8 | If retail implements signed invalid-range without branches, `(x < 0) \|\| (x >= N)` → `(x < 0) \| (x >= N)` | Both operands are pure boolean comparisons and both may be evaluated | `srawi/srwi/subfc/adde/or.` lowering | M/H/H |
| 9 | If a masked flag emits the wrong rotate/branch, shift/mask expression → typed bitfield access or `(flags & MASK) != 0`; use `(int)(flags & MASK) > 0` only for proven signed `ble` shape | Exact mask, signedness, and access width are known | `extrwi/rlwinm.` choice and branch polarity | M/M/M |
| 10 | If retail retains a byte/halfword result without caller masking, narrow accessor return → full-width `unsigned int` while the stored member remains narrow | Multiple callers prove return ABI | Caller-side `clrlwi` and ABI mismatch | M/H/H |
| 11 | If retail creates a by-value aggregate argument copy, `f(&value)` → declare `f(T value)` and call `f(value)` | Stack-copy pattern and callee ABI are proven | PPC EABI argument temporary and later stack offsets | M/H/H |
| 12 | If a 64-bit argument follows an r3 pointer and retail skips r4, `f(p, u32)` → `f(p, u64)` | Callee/callers prove width and signedness | EABI aligned r5:r6 argument pair | M/H/H |
| 13 | If a four-byte value is stored to stack on both return and use, scalar return → named four-byte POD return | Callee and caller both show aggregate-copy behavior | PPC aggregate return ABI | M/H/H |
| 14 | If retail builds `va_list` fields inline, library `va_start` call → exact MWCC `__va_list[1]` plus `__builtin_va_info(ap)` (`&ap` in C++) | Confirm EABI layout and variadic callee (`crclr` is evidence) | Varargs prologue/call ABI | M/H/H |
| 15 | If repeated literals target `@stringBase0`, named static string → repeated literal/macro; if retail owns a distinct symbol, do the inverse | Relocations and object string layout are inspected | String pooling, TOC base, NV pressure | M/M/H |
| 16 | If retail places zero-filled storage in `.data`, `T a[N];` → `T a[N] = {0};` | Section, size, symbol order, and initialization are proven | `.data` versus `.bss` placement | M/H/H |
| 17 | If retail stores a loaded mask from `.sdata2` but ours folds an immediate, `static const` → proven writable `__declspec(section ".sdata2")` object | Retail symbol/section exists; never substitute `volatile` | Constant folding versus loaded mask | M/H/H |
| 18 | If retail advances an index before consuming it, `use(a[i]); i++;` → `use(a[i++]);` | Sequencing and side effects are equivalent | Index-update scheduling and scratch reuse | M/H/M |
| 19 | If retail advances a list iterator before a virtual call, post-call update → `cur = it; it = it->next; cur->Call();` | Callback may mutate list; retail ordering is clear | Iterator survival across call and NV coloring | M/H/H |
| 20 | If retail repeats a result test around intervening stores, nested guard → two sequential `if (result != 0)` guards | Value is unchanged; stores must remain between tests | Compare hoisting and store schedule | M/H/M |
| 21 | If vtable slots/text ownership differ, local stubs/pure virtuals → declarations matching proven zero slots or externally owned weak defaults | ELF relocations and class slot order are verified | Vtable data, weak ownership, hidden text | M/H/H |
| 22 | If retail destructor open-codes base teardown but ours calls it, out-of-line empty base dtor → inline class definition while preserving retail standalone ownership | Class hierarchy and vtables are verified | Destructor inlining and vtable rematerialization | M/M/H |
| 23 | If repeated functions show source-ordered independent ops, default schedule → test object-scoped `-schedule off` once | Whole TU shares the smell; all exact functions are rechecked | Instruction scheduling across independent loads/stores | M/M/H |
| 24 | If a helper's local lifetime begins after an inline region, eager initializer → declare early but assign after that region | Retail first-use point is clear | Saved-register count and inline-body coloring | M/H/M |
| 25 | If a circular-list loop repeatedly compares against the same sentinel across calls, repeated `&list.link` → one typed `head`/sentinel local kept beside the iterator | Retail keeps distinct iterator and sentinel registers; callback/callee may clobber temporaries; list ownership is proven | Sentinel rematerialization, saved-register set, loop comparison coloring | M/H/M |

## Mid-tier stop rule

After one plausible attempt per matching smell, return to ASM and reclassify the
cause. Do not stack optimizer flags or local reshuffles. A function-scoped pragma
is acceptable only when it disables a demonstrated compiler transformation and
the surrounding C remains the clearest expression of retail behavior.
