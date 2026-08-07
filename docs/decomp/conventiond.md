# MKD C/C++ conventions reference

Use this book to recognize likely source structure before starting register-level
matching. It describes high-level patterns observed in well-matched repository
code; it does not override retail ASM, call sites, symbols, or object layout.

## Read this before matching

1. Identify the code family: game process, SDK boundary, C library, or C++
   library wrapper.
2. Use the family patterns below to form a source hypothesis.
3. Confirm it from calls, loads/stores, branches, and relocations.
4. Only then use the ranked mechanical playbooks for a localized mismatch.

Do not copy a convention merely because it looks idiomatic. This codebase often
keeps source order, repeated tests, direct global access, and narrow types because
those facts are part of the original program rather than stylistic accidents.

## Game code

### Process functions are small state machines

Game processes commonly return `float`, use global process context, and transfer
control through the current process vtable:

```c
float p_mode(void) {
    ModePdata* pdata = (ModePdata*)apdata;

    if (done(pdata)) {
        aproc->vtbl->transfer(p_next, 0.0f);
        return 0.0f;
    }
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    return 0.0f;
}
```

Recognition clues:

- `aproc`, `apdata`, `plyr_pdata`, `his_pdata`, and `active_cmdscript` behave as
  implicit execution context rather than ordinary function parameters.
- A final indirect call with a function pointer and float in `f1` is often
  `transfer`, `jump_sleep`, or a related process-vtable operation.
- `p_*`, `r_*`, `j_*`, and `x_*` names often describe process, reaction, jump,
  and transfer stages, not conventional value-returning helpers.
- A nominal float return may exist to satisfy the process ABI even when control
  is transferred and the value is operationally irrelevant.

### Per-process data is allocated through typed out-parameters

Spawn helpers often allocate a process and publish its data through a final
out-parameter:

```c
EffectPdata* pdata;
if (_create_mkproc_generic_nostack(
        PID, CLASS, p_effect, sizeof(EffectPdata), (MkHdr**)&pdata) != 0) {
    pdata->duration = duration;
    pdata->strength = strength;
}
```

Expect these properties:

- the data size is `sizeof(real_type)`, not a guessed constant;
- the out-local is written by the allocator and usually is not pre-cleared;
- fields are initialized only on successful creation;
- pointer plus instance pairs are used when later code must reject stale objects.

### State dispatch prefers explicit switches and literal IDs

Character, action, scheme, and process IDs frequently appear as explicit switch
cases. Case source order may follow retail text order rather than numeric order:

```c
switch (character_id) {
case CHAR_A:
    action_a();
    break;
case CHAR_C:
case CHAR_D:
    shared_action();
    break;
}
```

Keep literal IDs until a repository definition or repeated semantic evidence
supports a name. Dense or sparse compare trees should first be tested as a
`switch`; repeated case bodies may be intentionally duplicated or grouped.

### Guards are direct and cleanup is explicit

Matched game helpers commonly use sequential early guards:

```c
if (definition->model_name == 0) return 0;
object = load_model(definition->model_name);
if (object == 0) return 0;
if (definition->bones == 0) {
    destroy_if_live(object);
    return 0;
}
```

Do not automatically merge these into one compound expression or cleanup label.
Separate guards preserve which work has happened and often mirror distinct retail
branch regions.

### Globals are authoritative owners

Game code often accesses `g_game_info`, player slots, globals, or current process
fields directly at each semantic use. A cached convenience pointer is appropriate
only when retail keeps that address live. Repeated retail loads usually mean:

```c
use(g_game_info.plyr0.slot.pdata);
call();
use_again(g_game_info.plyr0.slot.pdata);
```

rather than one long-lived local. Conversely, when several adjacent member loads
share one computed element, a typed element pointer is likely.

### Flags use typed overlays but preserve storage width

Byte flags and packed state words are common:

```c
object->hide_flag_bits.hidden = 1;
object->flags_09_bits.face_opponent = 0;
if ((pdata->state & STATE_AIRBORNE) != 0) { ... }
```

Use a bitfield overlay when the byte RMW and bit position are proven. Use masks
when retail operates on the full word. Do not globalize one representation merely
because it matched one accessor.

### Animation code is deliberately sequential

Large reaction and move functions read like scripts: set state, select animation,
set speed/step, wait for a frame, apply an effect, then transfer. Preserve that
sequence. Avoid extracting helpers, folding repeated calls, or moving stores
across animation/process calls unless retail proves the abstraction.

Common loop shapes include:

```c
for (ticks = 0; ticks < duration; ticks++) { update(); sleep_one_tick(); }
while (continue_test() == 0) { advance_anim(); pose_anim(); sleep_one_tick(); }
do { step(); } while (frame < limit);
```

The first-test versus bottom-test shape is part of the recovered algorithm.

### TU-owned data layout matters

Game TUs commonly own:

- contiguous `stringBase0` pools referenced through named offsets;
- action/function-pointer tables;
- exact-order `.data`, `.sdata`, `.sdata2`, `.bss`, and `.sbss` objects;
- generated `.inc` tables kept in retail order;
- small local view structs for a proven subset of a larger runtime object.

Treat function order and data declaration order as source evidence. Do not turn a
verified pool into unrelated literals or move a table to a shared header solely
for aesthetics.

## SDK-facing code

### Match the public ABI before reconstructing internals

At Dolphin/GX/OS boundaries, start from the canonical SDK declaration when the
version is known. Check:

- exact enum and scalar widths;
- array parameters such as `Mtx`/`Mtx44` rather than pointer-shaped guesses;
- out-parameters and callback prototypes;
- `const` and volatile hardware access;
- paired/aligned EABI arguments.

An incorrect SDK prototype can make every caller look like a register-allocation
problem.

### Graphics setup is ordered state publication

Validated callers commonly issue short sequences of GX state calls with literal
enum values:

```c
GXSetNumTevStages(1);
GXSetNumTexGens(1);
GXSetTexCoordGen2(...);
GXSetTevOrder(...);
GXSetTevOp(...);
```

Keep call order and argument widths. Do not collapse several calls into a helper
unless retail has a `bl`, and do not replace literal enum values with guessed
names until the SDK version confirms them.

### Matrices are fixed-layout arrays

Matrix APIs use array-shaped types and direct element access:

```c
Mtx44 projection;
C_MTXOrtho(projection, top, bottom, left, right, near_z, far_z);
GXSetProjection(projection, GX_ORTHOGRAPHIC);
```

Expect float component stores, stack alignment, and array-to-pointer decay. A
generic struct or `void*` prototype can change address formation and argument
setup even when the data size is correct.

### SDK callbacks expose execution constraints

Interrupt, audio, card, and async APIs commonly pass a request/block pointer plus
an integer status and user data. Recover the callback signature from both the
registration site and callback entry. Avoid introducing blocking work, allocation,
or high-level ownership assumptions into code that retail executes as a callback.

### Hardware `volatile` is legitimate; allocation coax is not

Keep `volatile` for MMIO, FIFO, device registers, and fields modified across an
interrupt boundary when supported by evidence. A volatile local used only to
spill or color a GPR is not an SDK convention and must be removed.

## Libraries

### C ABI wrappers commonly drive C++ objects

Library TUs often expose `extern "C"` functions while calling typed virtual
methods internally:

```cpp
extern "C" int mslWaveSetPitch(mslRuntimeWave* wave, float pitch) {
    mslPlayable* playable = wave->playable;
    playable->SetPitch(pitch);
    return 0;
}
```

Preserve the C symbol boundary, the real C++ owner type, virtual slot order, and
return ABI. Do not flatten a virtual call into an invented C function pointer
unless the surrounding source family actually uses a C vtable.

### Ownership and rollback are explicit

Allocation helpers typically allocate the owner, allocate dependent storage,
initialize fields, and free partial state on failure:

```c
Queue* q = alloc(sizeof(*q));
if (q == 0) return 0;
q->entries = alloc(count * sizeof(*q->entries));
if (q->entries == 0) {
    free(q);
    return 0;
}
```

Keep allocator IDs, alignment arguments, failure order, and which fields are
initialized before publication. Avoid `calloc`, constructors, or RAII unless
retail calls show them.

### Intrusive lists preserve link ownership

MSL-style runtime lists often store both `next` and a pointer to the link that
owns the current node:

```c
Node** previous_link = node->previous_link;
Node* next = node->next;
if (previous_link != 0) *previous_link = next;
if (next != 0) next->previous_link = previous_link;
```

This is not interchangeable with a conventional `prev` pointer. Preserve the
pointer-to-link type, unlink order, and clearing of detached links.

### Reference counting is manual and local

Library code commonly decrements a concrete object's count, destroys it at zero,
then clears the owning field. Keep the exact sequence and the concrete type used
for the virtual destructor/free call.

### Ring buffers use indices, not pointer iterators

Matched queue code compares `write_index` and `read_index`, reads the selected
entry, increments the local index, publishes it, and wraps at capacity. Preserve
the empty condition and publish/wrap order; an apparently cleaner modulo or
pointer walk often lowers differently.

### Debug assertions may intentionally never return

SDK-adjacent libraries use diagnostic calls followed by `while (1) {}`. This is
an assertion halt, not an empty-loop artifact. Keep it when retail has the
diagnostic and terminal back-edge.

### Utility loops may be directionally unusual

Low-level CRI/Sofdec utilities can walk from the end, use pre-decrement, and
manually unroll fixed blocks:

```c
while (--remainder != 0) *--end = value;
while (--blocks != 0) { end[-1] = value; /* ... */ end -= 16; }
```

Preserve direction, pre/post update, unsigned count behavior, and unroll factor.
Do not replace a validated low-level routine with `memset` or a forward loop.

### Pragmas are narrow historical facts

`#pragma dont_inline`, scheduling control, and scoped optimization changes appear
in matched library TUs. Treat them as per-function or per-object compiler evidence:

- keep a pragma only when retail emission demonstrates the transformation;
- reset it immediately after the intended region;
- verify all already-exact functions in an object after changing TU-wide flags;
- never use a pragma as a substitute for correct types or control flow.

## Recognition matrix

| ASM or call-site clue | First high-level hypothesis |
|---|---|
| Float in `f1` plus indirect process call | `sleep`, `transfer`, or `jump_sleep` process-vtable method |
| Allocation call with size and final stack address | Typed per-process data out-parameter |
| Pointer plus saved instance comparison | Stale-object validation before use |
| Long sequence of animation/effect calls | Script-like game process; preserve source order |
| Jump table or ordered compare blocks | Explicit `switch`; test retail case source order |
| Repeated global/player-slot loads around calls | Authoritative global access, not necessarily one cached local |
| `lbz`/RMW/`stb` around one bit | Byte bitfield overlay or exact mask operation |
| C symbol calling through a C++ vtable | `extern "C"` wrapper around typed class internals |
| Node holds `next` and address of incoming link | Intrusive `previous_link`, not ordinary `prev` |
| Owner allocation followed by child allocation and rollback | Manual two-stage constructor pattern |
| Index increment, publish, then capacity wrap | Ring-buffer queue operation |
| Diagnostic call followed by backward self-branch | Intentional assertion halt |
| GX/OS call series with many immediates | Ordered SDK state setup; verify canonical prototype |
| `Mtx` stack object passed without explicit address | SDK array typedef and array decay |

## What not to infer

- A recurring literal is not automatically an enum name.
- A repeated offset is not automatically a semantic field name.
- A C++-looking object is not automatically owned by the current TU.
- A missing null check is not permission to add one.
- A process function's return type does not prove its semantic result.
- A helper-shaped block is not proof the original source extracted a helper.
- A matched instruction sequence does not validate a foreign SDK version.
- A cleaner abstraction is not preferable when it changes confirmed ownership,
  access width, evaluation order, or control flow.

When a convention and the retail evidence disagree, retail wins.
