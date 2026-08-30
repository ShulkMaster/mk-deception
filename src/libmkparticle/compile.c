#include "libmkparticle/behavior.h"
#include "libmkparticle/compile.h"
#include "libmkparticle/compile_fields.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/table.h"

/* TODO: Missing implementations for the remaining retail compile helpers. */

void *find_table_spawn(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *fixup_table_spawn(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *check_update_code(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *check_conflicts(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *check_for_missing_spawns(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *check_for_deterministic_spawn(PfxVm* pfx)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void pfxvm_compile(PfxVm* pfx)
{
    int behavior_index;

    if (pfx_num_behaviors(pfx) != 0) {
        check_update_code(pfx);
        for (behavior_index = 0;
             behavior_index < pfx_num_behaviors(pfx);
             behavior_index++) {
            bind_behavior_to_effect(
                pfx_behavior(pfx, behavior_index), pfx);
        }
    }

    check_conflicts(pfx);
    check_for_missing_spawns(pfx);
    check_for_deterministic_spawn(pfx);
    _pfx_emitter_compile(
        (PfxEmitterCompileView*)pfx_get_emitter(pfx, 0),
        (PfxTableRegistry*)pfx);
}
