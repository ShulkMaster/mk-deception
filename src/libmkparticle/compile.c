#include "libmkparticle/behavior.h"
#include "libmkparticle/compile.h"
#include "libmkparticle/compile_fields.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/spawn.h"
#include "libmkparticle/table.h"

static char diagnostic_strings[] =
    "No initial table spawn for FIELD_POS/TABLE_0\0"
    "Specified both global and per-particle size!\0"
    "Age initialized to 0 by default\0"
    "Color initialized to 0 by default";

static PfxEmitterInstruction* find_table_spawn(PfxVmEmitter* emitter,
                                                int field_description)
{
    int i;

    for (i = 0; i < emitter->instruction_count; i++) {
        PfxEmitterInstruction* instruction = &emitter->instructions[i];

        if (instruction->opcode == 5 &&
            instruction->field_description == field_description) {
            return instruction;
        }
    }
    return 0;
}

static void fixup_table_spawn(PfxVm* pfx, unsigned int field)
{
    PfxEmitterInstruction* instruction =
        find_table_spawn(pfx_get_emitter(pfx, 0), field);

    if (instruction == 0) {
        pfx_halt(diagnostic_strings);
        return;
    }

    instruction->field_description = 0x302;
    instruction->opcode = 7;
    instruction->spawn.table.field = field;
    pfxvm_require_field(pfx, 0x302);
}

static void check_update_code(PfxVm* pfx)
{
    int i;

    for (i = 0;
         i < pfx_behavior(pfx, 0)->update_instruction_count;
         i++) {
        PfxUpdateInstruction* instruction =
            &pfx_behavior(pfx, 0)->update_instructions[i];

        if (instruction->opcode == 10 &&
            instruction->argument_0x18 == 0x302) {
            fixup_table_spawn(pfx, instruction->field.description);
            pfx->flags_0x60 |= 0x200;
        }
    }
}

static void check_conflicts(PfxVm* pfx)
{
    if ((pfx->flags_0x1D4 & 0x20) && pfx->flag150_40) {
        pfx->flag150_40 = 0;
        pfx_halt(diagnostic_strings + 0x2D);
    }
}

static void check_for_missing_spawns(PfxVm* pfx)
{
    PfxVmEmitter* emitter;

    if (pfx->flags_0x60 & 2) {
        emitter = pfx_get_emitter(pfx, 0);
        if (!has_spawncode_for(emitter, 0x301)) {
            pfxvm_spawn_value(pfx_get_emitter(pfx, 0), 0x301, 0.0f);
            pfx_halt(diagnostic_strings + 0x5A);
        }
    }

    if ((pfx->flags_0x1D4 & 0x10) && pfx->field_0x22C == 0) {
        emitter = pfx_get_emitter(pfx, 0);
        if (!has_spawncode_for(emitter, 0x101)) {
            pfxvm_spawn_point_color(pfx_get_emitter(pfx, 0), 0x101,
                                    255.0f, 255.0f, 255.0f, 255.0f);
            pfx_halt(diagnostic_strings + 0x7A);
        }
    }
}

static void check_for_deterministic_spawn(PfxVm* pfx)
{
    PfxVmEmitter* emitter = pfx_get_emitter(pfx, 0);
    int i;

    emitter->flags.bits.flag_0x20 = 0;
    for (i = 0; i < emitter->instruction_count; i++) {
        PfxEmitterInstruction* instruction = &emitter->instructions[i];

        if (!pfx_emitter_insn_is_static(instruction) &&
            instruction->opcode != 6) {
            return;
        }
    }
    emitter->flags.bits.flag_0x20 = 1;
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
