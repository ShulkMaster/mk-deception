#include "libmkparticle/random.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/vm.h"
#include "libmkparticle/particle.h"
#include "runtime/cstring.h"

void move_particle_to_behavior(PfxBehavior* source, int particle,
                               PfxBehavior* destination)
{
    int stride;
    unsigned char* source_particle;

    stride = source->previous_streams[0].stride;
    source_particle = source->current_streams[0].data + stride * particle;
    memcpy(destination->current_streams[0].data +
               stride * destination->active_particle_count,
           source_particle, stride);
    memcpy(source_particle,
           source->current_streams[0].data + stride * (source->particle_count - 1),
           stride);

    stride = source->current_streams[1].stride;
    if (stride != 0) {
        source_particle = source->current_streams[1].data + stride * particle;
        memcpy(destination->current_streams[1].data +
                   stride * destination->active_particle_count,
               source_particle, stride);
        memcpy(source_particle,
               source->current_streams[1].data + stride * (source->particle_count - 1),
               stride);
    }

    source->particle_count--;
    destination->active_particle_count++;
}

#pragma peephole off
void kill_behavior_particle(PfxBehavior* behavior, int particle)
{
    int stride;
    unsigned char *last_slot;
    stride = behavior->current_stream_100_stride;
    last_slot = behavior->stream_100 + stride * (behavior->particle_count - 1);
    memcpy(behavior->stream_100 + stride * particle, last_slot, stride);

    stride = behavior->current_stream_300_stride;
    if (stride != 0) {
        last_slot = behavior->stream_300;
        last_slot += stride * (behavior->particle_count - 1);
        memcpy(behavior->stream_300 + stride * particle, last_slot, stride);
    }
    behavior->particle_count--;
}
#pragma peephole reset

static void kill_on_int_field_less_than(PfxBehavior* behavior, int* field,
                                        int stride, int value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field < value) {
            kill_behavior_particle(behavior, particle);
            particle--;
        } else {
            field = (int*)((unsigned char*)field + stride);
        }
    }
}

static void kill_on_field_less_than(PfxBehavior* behavior, float* field,
                                    int stride, float value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field < value) {
            kill_behavior_particle(behavior, particle);
            particle--;
        } else {
            field = (float*)((unsigned char*)field + stride);
        }
    }
}

static void kill_on_field_greater_than(PfxBehavior* behavior, float* field,
                                       int stride, float value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field > value) {
            kill_behavior_particle(behavior, particle);
            particle--;
        } else {
            field = (float*)((unsigned char*)field + stride);
        }
    }
}

static void kill_percent_of_particles(PfxBehavior* behavior, float percent)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (percent > (float)rnd_int(100)) {
            kill_behavior_particle(behavior, particle);
            particle--;
        }
    }
}

static void kill_roundrobin(PfxBehavior* behavior, int* field, int stride)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field < behavior->effect->total_birth_count) {
            kill_behavior_particle(behavior, particle);
            particle--;
        } else {
            field = (int*)((unsigned char*)field + stride);
        }
    }
}

static void change_on_const_greater_than(PfxBehavior* behavior,
                                         PfxBehavior* target, float* field,
                                         int stride, float value)
{
    if (*field <= value) {
        return;
    }

    while (behavior->particle_count > 0) {
        move_particle_to_behavior(behavior, 0, target);
    }
}

static void change_on_field_greater_than(PfxBehavior* behavior,
                                         PfxBehavior* target, float* field,
                                         int stride, float value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field > value) {
            *field = value;
            move_particle_to_behavior(behavior, particle, target);
            particle--;
        } else {
            field = (float*)((unsigned char*)field + stride);
        }
    }
}

static void change_on_int_field_less_than(PfxBehavior* behavior,
                                          PfxBehavior* target, int* field,
                                          int stride, int value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field < value) {
            *field = value;
            move_particle_to_behavior(behavior, particle, target);
            particle--;
        } else {
            field = (int*)((unsigned char*)field + stride);
        }
    }
}

static void change_on_field_less_than(PfxBehavior* behavior,
                                      PfxBehavior* target, float* field,
                                      int stride, float value)
{
    int particle;

    for (particle = 0; particle < behavior->particle_count; particle++) {
        if (*field < value) {
            *field = value;
            move_particle_to_behavior(behavior, particle, target);
            particle--;
        } else {
            field = (float*)((unsigned char*)field + stride);
        }
    }
}

static void set_reference_field(PfxKillInstruction* instruction,
                                unsigned int field)
{
    instruction->reference_field = field;
    instruction->field_18 = 1;
}

void pfxvm_kill_on_y_less_than_field(PfxBehavior* behavior,
                                      unsigned int field,
                                      unsigned int reference_field)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, field);

    instruction->field_10 = 4;
    set_reference_field(instruction, reference_field);
}

void pfxvm_kill_on_intersect_plane_y(PfxBehavior* behavior, float plane)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, 0x100);

    instruction->scalar = plane;
    instruction->field_10 = 4;
}

void pfxvm_kill_on_intersect_plane_z(PfxBehavior* behavior, float plane)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, 0x100);

    instruction->scalar = plane;
    instruction->field_10 = 8;
}

void pfxvm_kill_on_intersect_plane_x(PfxBehavior* behavior, float plane)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, 0x100);

    instruction->scalar = plane;
}

void pfxvm_kill_on_greater(PfxBehavior* behavior, unsigned int field,
                           float value)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 2, field);

    instruction->scalar = value;
}

void pfxvm_kill_percent(PfxBehavior* behavior, float percent)
{
    PfxKillInstruction* instruction;

    if (percent < 0.0f || percent > 100.0f) {
        return;
    }
    instruction = add_kill_insn(behavior, 3, 0x100);
    instruction->scalar = percent;
}

void pfxvm_kill_roundrobin(PfxBehavior* behavior, unsigned int field)
{
    add_kill_insn(behavior, 4, field);
}

void pfxvm_change_on_greater(PfxBehavior* behavior, unsigned int field,
                             float value, PfxBehavior* target)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 2, field);

    instruction->target = target;
    instruction->scalar = value;
}

void pfxvm_change_on_less(PfxBehavior* behavior, unsigned int field,
                          float value, PfxBehavior* target)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, field);

    instruction->target = target;
    instruction->field_10 = 0;
    instruction->scalar = value;
}

void pfxvm_change_on_y_less(PfxBehavior* behavior, unsigned int field,
                            float value, PfxBehavior* target)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, field);

    instruction->target = target;
    instruction->field_10 = 4;
    instruction->scalar = value;
}

void pfxvm_change_on_y_less_than_field(PfxBehavior* behavior,
                                       unsigned int field,
                                       unsigned int reference_field,
                                       PfxBehavior* target)
{
    PfxKillInstruction* instruction = add_kill_insn(behavior, 1, field);

    instruction->target = target;
    instruction->field_10 = 4;
    set_reference_field(instruction, reference_field);
}

void pfxvm_execute_behavior_kill(PfxBehavior* behavior)
{
    int index;
    PfxKillInstruction* instruction;

    instruction = behavior->kill_instructions;
    for (index = 0; index < behavior->kill_instruction_count;
         index++, instruction++) {
        unsigned char* field;
        int stride;
        int description;
        float value;

        switch (instruction->field.stream) {
        case 0:
        case 1:
            field = behavior->current_streams[instruction->field.stream].data +
                    instruction->field.offset;
            stride = behavior->current_streams[instruction->field.stream].stride;
            break;
        default:
            field = pfx_get_field(behavior->effect, -2,
                                  instruction->field.description);
            stride = pfx_get_struct_size(behavior->effect,
                                         instruction->field.description);
            break;
        }
        field += instruction->field_10;

        if (instruction->field_18 != 0) {
            value = *(float*)pfx_get_field(behavior->effect, 0,
                                           instruction->reference_field);
        } else {
            value = instruction->scalar;
        }

        switch (instruction->opcode) {
        case 2:
            if (instruction->target != 0) {
                int storage =
                    instruction->field.description & 0xF00;

                if (storage == 0x200 || storage == 0x500) {
                    change_on_const_greater_than(
                        behavior, instruction->target, (float*)field, stride,
                        value);
                } else {
                    change_on_field_greater_than(
                        behavior, instruction->target, (float*)field, stride,
                        value);
                }
            } else {
                kill_on_field_greater_than(behavior, (float*)field, stride,
                                           value);
            }
            break;
        case 1:
            description = instruction->field.description;
            if (description == 0x307 || description == 0x308) {
                if (instruction->target != 0) {
                    change_on_int_field_less_than(
                        behavior, instruction->target, (int*)field, stride,
                        value);
                } else {
                    kill_on_int_field_less_than(behavior, (int*)field, stride,
                                                value);
                }
            } else if (instruction->target != 0) {
                change_on_field_less_than(behavior, instruction->target,
                                          (float*)field, stride, value);
            } else {
                kill_on_field_less_than(behavior, (float*)field, stride,
                                        value);
            }
            break;
        case 3:
            kill_percent_of_particles(behavior, value);
            break;
        case 4:
            kill_roundrobin(behavior, (int*)field, stride);
            break;
        default:
            return;
        }
    }
}
