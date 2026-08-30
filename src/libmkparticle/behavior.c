#include "libmkparticle/behavior.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/table.h"
#include "libmkparticle/vm.h"
#include "runtime/cstring.h"

void* memmove(void* destination, const void* source, unsigned long size);
void set_vm_field(PfxVmField* field, unsigned int description);

typedef struct PfxBehaviorStreamBuffer {
    unsigned char* stream_100;
    char pad04[4];
    unsigned char* stream_300;
} PfxBehaviorStreamBuffer;

int pfx_num_behaviors(PfxVm* pfx)
{
    return pfx->behavior_count;
}

PfxBehavior* pfx_behavior(PfxVm* pfx, int index)
{
    if (index < 0 || index >= pfx->behavior_count) {
        return 0;
    }
    return pfx->behavior_list[index];
}

void bind_behavior_to_effect(PfxBehavior* behavior, PfxVm* pfx)
{
    int index;

    behavior->effect = pfx;
    if (pfx->field_0x22C != 0) {
        behavior->auxiliary_stream_100 = pfx->name_obj;
        behavior->auxiliary_stream_100_stride = 0x28;
        behavior->auxiliary_stream_300 = pfx->name_obj;
        behavior->auxiliary_stream_300_stride = 0x28;
    } else {
        behavior->auxiliary_stream_100 = 0;
        behavior->auxiliary_stream_100_stride = 0;
        behavior->auxiliary_stream_300 = 0;
        behavior->auxiliary_stream_300_stride = 0;
    }

    for (index = 0; index < behavior->update_instruction_count; index++) {
        PfxUpdateInstruction* instruction;

        instruction = &behavior->update_instructions[index];
        instruction->field.offset =
            get_field_offset((PfxTableRegistry*)pfx,
                             instruction->field.description);
        switch (instruction->opcode) {
        case 10:
            if ((int)instruction->argument_0x14 < 0 ||
                instruction->argument_0x14 >= 2) {
                return;
            }
            instruction->argument_0x14 =
                (unsigned int)pfx->tables[instruction->argument_0x14];
            if (instruction->argument_0x14 == 0) {
                return;
            }
            break;
        case 2:
        case 11:
            instruction->argument_offset =
                get_field_offset((PfxTableRegistry*)pfx,
                                 instruction->argument_0x14);
            break;
        case 12:
            instruction->argument_offset =
                get_field_offset((PfxTableRegistry*)pfx,
                                 instruction->argument_0x14);
            break;
        case 13:
            instruction->argument_offset =
                get_field_offset((PfxTableRegistry*)pfx,
                                 instruction->argument_0x14);
            break;
        case 14:
            instruction->argument_offset =
                get_field_offset((PfxTableRegistry*)pfx,
                                 instruction->argument_0x14);
            break;
        }
    }

    if ((pfx->flags_0x60 & 2) != 0) {
        behavior->has_age_field = 1;
        behavior->age_field_offset =
            get_field_offset((PfxTableRegistry*)pfx, 0x301);
        behavior->age_field = 0x301;
    }

    for (index = 0; index < behavior->kill_instruction_count; index++) {
        PfxKillInstruction* instruction;

        instruction = &behavior->kill_instructions[index];
        instruction->field.offset =
            get_field_offset((PfxTableRegistry*)pfx,
                             instruction->field.description);
    }

    for (index = 0; index < behavior->init_instruction_count; index++) {
        PfxInitInstruction* instruction;

        instruction = &behavior->init_instructions[index];
        instruction->field.offset =
            get_field_offset((PfxTableRegistry*)pfx,
                             instruction->field.description);
    }
    behavior->effect = pfx;
}

void behavior_adjust_streams(PfxBehavior* source, PfxBehavior* destination)
{
    unsigned char* stream;

    stream = source->stream_100 +
             source->current_stream_100_stride * source->particle_count;
    if (destination->active_particle_count != 0) {
        memmove(stream, destination->stream_100,
                source->current_stream_100_stride *
                    destination->active_particle_count);
    }
    destination->stream_100 = stream;

    stream = source->stream_300 +
             source->current_stream_300_stride * source->particle_count;
    if (destination->active_particle_count != 0) {
        memmove(stream, destination->stream_300,
                source->current_stream_300_stride *
                    destination->active_particle_count);
    }
    destination->stream_300 = stream;
}

PfxKillInstruction* add_kill_insn(PfxBehavior* behavior, int opcode,
                                  unsigned int field)
{
    PfxKillInstruction* instruction;

    instruction = &behavior->kill_instructions[
        behavior->kill_instruction_count];
    instruction->opcode = opcode;
    set_vm_field(&instruction->field, field);
    instruction->field_10 = 0;
    instruction->target = 0;
    instruction->field_18 = 0;
    behavior->kill_instruction_count++;
    return instruction;
}

void pfx_behaviors_frame_begin(PfxVm* pfx)
{
    int index;
    int stream_100_stride;
    int stream_300_stride;
    int particle_offset;

    if (pfx == 0 || pfx->behavior_list == 0) {
        return;
    }

    if (pfx->behavior_list[0]->stream_100_stride == 0) {
        stream_100_stride = pfx_get_struct_size(pfx, 0x100);
        stream_300_stride = pfx_get_struct_size(pfx, 0x300);
        for (index = 0; index < pfx->behavior_count; index++) {
            pfx->behavior_list[index]->stream_100_stride = stream_100_stride;
            pfx->behavior_list[index]->current_stream_100_stride =
                stream_100_stride;
            pfx->behavior_list[index]->stream_300_stride = stream_300_stride;
            pfx->behavior_list[index]->current_stream_300_stride =
                stream_300_stride;
        }
    }

    for (index = 0; index < pfx->behavior_count; index++) {
        pfx->behavior_list[index]->previous_stream_100 =
            pfx->behavior_list[index]->stream_100;
        pfx->behavior_list[index]->previous_stream_300 =
            pfx->behavior_list[index]->stream_300;
    }

    pfx->behavior_list[0]->stream_300 =
        ((PfxBehaviorStreamBuffer*)pfx->runtime_buffer_a)->stream_300;
    pfx->behavior_list[0]->stream_100 =
        ((PfxBehaviorStreamBuffer*)pfx->runtime_buffer_a)->stream_100;
    particle_offset = pfx->behavior_list[0]->particle_count;
    for (index = 1; index < pfx->behavior_count; index++) {
        pfx->behavior_list[index]->stream_300 =
            pfx->behavior_list[0]->stream_300 +
            particle_offset *
                pfx->behavior_list[0]->current_stream_300_stride;
        pfx->behavior_list[index]->stream_100 =
            pfx->behavior_list[0]->stream_100 +
            particle_offset *
                pfx->behavior_list[0]->current_stream_100_stride;
        particle_offset += pfx->behavior_list[index]->particle_count;
    }
}

void pfx_behaviors_frame_end(PfxVm* pfx)
{
    if (pfx == 0) {
        return;
    }
    if (pfx->behavior_list == 0) {
        return;
    }
}

void behavior_fixup_targets(PfxBehavior* behavior,
                            PfxBehavior* const* old_targets,
                            PfxBehavior* const* new_targets, int target_count)
{
    int instruction_index;

    for (instruction_index = 0;
         instruction_index < behavior->kill_instruction_count;
         instruction_index++) {
        int target_index;

        for (target_index = 0; target_index < target_count; target_index++) {
            if (behavior->kill_instructions[instruction_index].target ==
                old_targets[target_index]) {
                behavior->kill_instructions[instruction_index].target =
                    new_targets[target_index];
                break;
            }
        }
    }
}

void pfx_behaviors_fixup_targets(PfxBehavior** behaviors,
                                 PfxBehavior** old_targets, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        behavior_fixup_targets(behaviors[index], old_targets, behaviors, count);
    }
}

PfxInitInstruction* add_init_insn(PfxBehavior* behavior, int opcode,
                                  unsigned int field)
{
    PfxInitInstruction* instruction;

    instruction = &behavior->init_instructions[
        behavior->init_instruction_count++];
    instruction->opcode = opcode;
    set_vm_field(&instruction->field, field);
    return instruction;
}

void pfx_behavior_scan_fields(PfxBehavior* behavior,
                              unsigned int* particle_fields,
                              unsigned int* render_fields)
{
    int index;

    if (behavior == 0 || particle_fields == 0 || render_fields == 0) {
        return;
    }

    for (index = 0; index < behavior->update_instruction_count; index++) {
        PfxUpdateInstruction* instruction;
        unsigned int storage;

        instruction = &behavior->update_instructions[index];
        add_field(render_fields, instruction->field.description);
        switch (instruction->opcode) {
        case 2:
        case 9:
        case 11:
        case 15:
            storage = instruction->argument_0x14 & 0xF00;
            if (storage == 0x300 || storage == 0x100) {
                add_field(particle_fields, instruction->argument_0x14);
            }
            break;
        case 10:
            storage = instruction->field.description & 0xF00;
            if (storage == 0x300 || storage == 0x100) {
                add_field(particle_fields, instruction->field.description);
            }
            storage = instruction->argument_0x18 & 0xF00;
            if (storage == 0x300 || storage == 0x100) {
                add_field(particle_fields, instruction->argument_0x18);
            }
            break;
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            add_field(particle_fields, instruction->field.description);
            break;
        case 12:
            add_field(particle_fields, instruction->argument_0x14);
            break;
        case 13:
            add_field(particle_fields, instruction->argument_0x14);
            break;
        case 14:
            add_field(particle_fields, instruction->argument_0x14);
            break;
        case 17:
            storage = instruction->field.description & 0xF00;
            if (storage == 0x300 || storage == 0x100) {
                add_field(particle_fields, instruction->field.description);
            }
            break;
        case 16:
            add_field(particle_fields, instruction->field.description);
            break;
        }
    }

    for (index = 0; index < behavior->kill_instruction_count; index++) {
        unsigned int description;
        unsigned int storage;

        description = behavior->kill_instructions[index].field.description;
        storage = description & 0xF00;
        if (storage == 0x300 || storage == 0x100) {
            add_field(particle_fields, description);
        }
    }
}

void pfxvm_update_make_last_insn_first(PfxBehavior* behavior)
{
    PfxUpdateInstruction instruction;

    if (behavior == 0 || behavior->update_instruction_count < 1) {
        return;
    }

    memcpy(&instruction,
           &behavior->update_instructions[
               behavior->update_instruction_count - 1],
           sizeof(instruction));
    memmove(&behavior->update_instructions[1],
            &behavior->update_instructions[0],
            (behavior->update_instruction_count - 1) * sizeof(instruction));
    memcpy(&behavior->update_instructions[0], &instruction,
           sizeof(instruction));
}
