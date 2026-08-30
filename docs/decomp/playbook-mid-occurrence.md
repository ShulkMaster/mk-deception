# Decomp playbook: mid occurrence

Use after the high-occurrence playbook leaves a localized mismatch. Entries are
ranked by combined **occurrence / likelihood / impact**. Apply one evidence-backed
source-shape change per rebuild.

| Rank | If A → then B | Preconditions | Solves | O/L/I |
|---:|---|---|---|---|
| 1 | If retail loads a control word before subfield stores, `obj->sync \|= bit` → `u32 sync = obj->sync; sync \|= bit; ...; obj->sync = sync;` | Store order and authoritative field are proven | Load/store scheduling; NV lifetime | M/H/H |
| 2 | If retail uses `lwzx/stwx` through a runtime plugin offset, cached `Plugin *p` → direct typed access from `object + PluginOffset + field_off` | Dynamic extension mechanism and field offsets are proven | Indexed versus immediate addressing | M/H/H |
| 3 | If count is at `+0` and ILP32 entries at `+4`, raw byte casts → typed `{ u32 count; u32 refs[1]; }` or one localized byte-offset accessor | Packed layout, stride, and relocation representation are proven | Packed-table address formation and coloring | M/H/H |
| 4 | If retail emits `stmw/lmw` or a compact counted copy, current object at `-O4,p` → test object-scoped `-O4,s` and, if needed, `-use_lmw_stmw on` | Compiler flags are authentic; verify every exact function in the TU | Frame/save strategy, loop lowering | M/M/H |
| 5 | If retail loads a float operand pair in a different order, `p * x2 + c` → `x2 * p + c` without changing grouping | Floating semantics tolerate only the proven equivalent order | `fmadds` operand encoding and FPR schedule | M/H/M |
| 6 | If retail stages related factors before combining, nested expressions → named object-level intermediates, then final element combinations | ASM shows the common values loaded/computed first | FPR scheduling and live ranges | M/H/H |
| 7 | If compare immediates differ by one, `x > N-1`/`x <= N-1` → `x >= N`/`x < N` | Boundary semantics are exactly equivalent | Compare immediate and branch selection | M/H/M |
| 8 | If retail implements signed invalid-range without branches, `(x < 0) \|\| (x >= N)` → `(x < 0) \| (x >= N)` | Both operands are pure boolean comparisons and both may be evaluated | `srawi/srwi/subfc/adde/or.` lowering | M/H/H |
| 9 | If a masked flag emits the wrong rotate/branch, shift/mask expression → typed bitfield access or `(flags & MASK) != 0`; use `(int)(flags & MASK) > 0` only for proven signed `ble` shape | Exact mask, signedness, and access width are known | `extrwi/rlwinm.` choice and branch polarity | M/M/M |
| 10 | If `(byte & sourceMask) << shift` emits separate mask and shift instructions but retail uses one `rlwinm`, rewrite it as `(byte << shift) & destinationMask` | The masks are algebraically equivalent and the byte load is proven unsigned | Packed-field mask/shift fusion | L/H/M |
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
| 21 | If several proven failure edges branch directly to one cleanup block while only fallible async/success arms feed a shared result test and early success return, encode the original label region as one `do { ... if (failed) { result = error; break; } ... if (result >= 0) return; } while (0);` followed by cleanup | Dominator analysis proves the direct failure edges bypass the shared test; the cleanup side effects and callback/owner transfer occur exactly once; a natural guard/epilogue spelling was tested first | Structured no-`goto` recovery of a shared cleanup label; preserves branch targets and failure-edge ordering | M/H/H |
| 22 | If vtable slots/text ownership differ, local stubs/pure virtuals → declarations matching proven zero slots or externally owned weak defaults | ELF relocations and class slot order are verified | Vtable data, weak ownership, hidden text | M/H/H |
| 23 | If retail destructor open-codes base teardown but ours calls it, out-of-line empty base dtor → inline class definition while preserving retail standalone ownership | Class hierarchy and vtables are verified | Destructor inlining and vtable rematerialization | M/M/H |
| 24 | If repeated functions show source-ordered independent ops, default schedule → test object-scoped `-schedule off` once | Whole TU shares the smell; all exact functions are rechecked | Instruction scheduling across independent loads/stores | M/M/H |
| 25 | If a helper's local lifetime begins after an inline region, eager initializer → declare early but assign after that region | Retail first-use point is clear | Saved-register count and inline-body coloring | M/H/M |
| 26 | If retail derives a boolean from a registration result before publishing that result to a global, separate local assignment and comparison → compare the value of the global assignment expression directly | The call result is both the published value and the compared value; no intervening side effects are present | Result-test/store scheduling around plugin registration | L/H/M |
| 27 | If retail keeps a top-of-loop exit test but MWCC rotates a clean `while (field == 0)` into an initial branch to the latch, rewrite it as `for (;;) { if (field != 0) break; body; }` | The loop is otherwise identical, the exit condition has no side effects, and both forms preserve the zero-iteration case | Loop rotation, branch placement, and downstream instruction alignment | M/H/H |
| 28 | If an ABI-correct callback gains only an unused-parameter stack spill in an authentic `-O0` object, replace `(void)param` with MWCC `#pragma unused(param)` inside the function | Registration and dispatcher calls prove the parameter; retail does not consume its incoming GPR; the spill is the only mismatch | Preserves the real callback prototype while suppressing `-O0` parameter homes | L/H/H |
| 29 | If retail emits a public helper body but related callers open-code that same body, factor the implementation into a typed `static inline` helper and keep retail-ordered public wrappers around it | Repeated instruction sequences prove one shared implementation; each retail caller lacks a `bl`; standalone symbol and function order are confirmed | Reproduces shared inline logic without macros or duplicated bodies while preserving external ownership | L/H/H |
| 30 | If stores through one pointer are interleaved with loads through another but local MWCC hoists all loads, verify whether an unsupported `const` pointee qualifier was added; restore the call-site-authentic mutable pointer type | Retail interleaving is exact, callers pass mutable storage, and removing only `const` restores the sequence without changing ABI or behavior | Alias analysis, load hoisting, and FPR live ranges | L/H/H |
| 31 | If a callee's FPR and GPR assignments are correct but a wrapper loads all floating arguments before the integer/pointer group, recover the source declaration's float/integer interleaving from the wrapper and reorder parameters accordingly | PPC EABI's independent FPR/GPR streams make multiple source orders ABI-identical; every call site must support the recovered order | Wrapper argument evaluation/load order without changing callee register ABI | L/H/H |
| 32 | If retail pipelines repeated loads from a source object with stores to a destination but ours serializes each load/store through one scratch, mutable source pointee → `const T*` in the declaration, definition, and callers | The function never mutates the source; calls and ABI support the qualifier; the destination is independently owned or otherwise proven non-aliasing | Read-only alias contract; load/store scheduling and scratch allocation | L/H/H |
| 33 | If one TU already uses a proven macro/inline guard but retail shows the same inlined guard in another TU that calls the underlying helper directly, move the abstraction to its owning shared header and use it in both consumers | Both retail instruction sequences and shared type/API ownership agree; recheck every existing exact consumer | Missing inlined guard; duplicated local ownership; cross-TU source-shape drift | L/H/H |
| 34 | If retail initializes a scalar result, branches into a value-producing arm with `bne`, then uses an unconditional `b` from the other arm to one joined store, rewrite `result = zero; if (positive) result = expression;` as `result = nonpositive ? zero : expression` | Both arms produce the same typed value; the expression's side effects occur only in the positive arm; retail has one joined consumer/store | Collapsed one-branch guard versus explicit `bne; b` result diamond | L/H/H |
| 35 | If linked SHA fails despite normalized 100% data and retail SBSS places a file global before function-static objects followed by file globals in source order, define the leading global after the functions and declare the remaining tentative globals in reverse retail order; add only proven terminal gap objects | Raw target/local symbol tables prove every offset and section size; preserve declaration ownership and recheck all SDA relocations plus the linked SHA | MWCC reverse tentative-definition emission around function statics; normalized split gaps hiding non-link-exact small-data order | L/H/H |

### Single-pass cleanup guardrail

For the row above, prefer source shapes in this order: ordinary sequential
guards plus one epilogue; a typed helper only when retail has the call or MWCC
inlines it without changing ownership/lifetimes; then the single-pass block when
the retail branch graph requires failures to bypass a later result test. In
particular, `break` must mean “take the original shared cleanup edge,” never a
general register or layout nudge.

Use `goto`, duplicated epilogues, status flags, `switch`, and forced one-trip
`for (;;)` forms only in scratch to identify the lowering. Do not promote them,
and do not infer a macro merely from the resulting CFG. If the natural form
differs only in branch destinations and policy rejects the single-pass block,
keep the natural form at a documented soft ceiling; do not stack dummy state or
repeat cleanup side effects to recover the percentage.

## Mid-tier stop rule

After one plausible attempt per matching smell, return to ASM and reclassify the
cause. Do not stack optimizer flags or local reshuffles. A function-scoped pragma
is acceptable only when it disables a demonstrated compiler transformation and
the surrounding C remains the clearest expression of retail behavior.
