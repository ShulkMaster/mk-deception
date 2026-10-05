#include "libmkparticle/random.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/particle.h"
#include "runtime/cstring.h"

int pfx_get_struct_size(PfxVm* pfx, int field);

static void _pfxvm_init_reflect(unsigned char* data, int stride, int count)
{
    int remaining;

    for (remaining = count; remaining != 0; remaining--) {
        PfxVec3* vector = (PfxVec3*)data;

        vector->y = -vector->y;
        data += stride;
    }
}

static void _pfxvm_init_set_float_range(unsigned char* data, int stride,
                                        int count,
                                        PfxFloatRange* range)
{
    unsigned char* cursor = data;

    while (count-- != 0) {
        *(float*)cursor = rnd_between(range->center - range->variation,
                                    range->center + range->variation);
        cursor += stride;
    }
}

static void _pfxvm_init_multiply_float_range(
    unsigned char* data, int stride, int count, PfxFloatRange* range)
{
    unsigned char* cursor = data;

    while (count-- != 0) {
        float* value = (float*)cursor;

        *value *= rnd_between(range->center - range->variation,
                              range->center + range->variation);
        cursor += stride;
    }
}

static void _pfxvm_init_multiply_float_range_v3(
    unsigned char* data, int stride, int count, PfxFloatRange* range)
{
    unsigned char* cursor = data;

    while (count-- != 0) {
        PfxVec3* vector = (PfxVec3*)cursor;
        float scale = rnd_between(range->center - range->variation,
                                  range->center + range->variation);

        vector->x *= scale;
        vector->y *= scale;
        vector->z *= scale;
        cursor += stride;
    }
}

static void _pfxvm_init_add_v3(unsigned char* destination,
                               int destination_stride, int count,
                               unsigned char* source, int source_stride)
{
    for (; count != 0; count--) {
        PfxVec3* destination_vector = (PfxVec3*)destination;
        PfxVec3* source_vector = (PfxVec3*)source;

        destination_vector->x += source_vector->x;
        destination_vector->y += source_vector->y;
        destination_vector->z += source_vector->z;
        destination += destination_stride;
        source += source_stride;
    }
}

static void _pfxvm_init_divert(unsigned char* data, int stride, int count,
                               PfxFloatRange* range)
{
    unsigned char* cursor = data;
    while (count-- != 0) {
        rnd_bend_vector((PfxVec3*)cursor, range->center, range->variation);
        cursor += stride;
    }
}

/* TODO: [near miss] 98.87%; dispatch agrees; guarded particle-count load uses r30 rather than r0 plus copy. */
void _pfxvm_execute_initial_behavior(PfxBehavior* behavior, float frame_time)
{
    int particle_count = behavior->active_particle_count;
    PfxInitInstruction* instruction = behavior->init_instructions;
    unsigned char* destination;
    int stride;
    int index;

    if (particle_count == 0) {
        return;
    }

    for (index = 0; index < behavior->init_instruction_count;
         index++, instruction++) {
        destination =
            behavior->current_streams[instruction->field.stream].data +
            instruction->field.offset;
        stride = behavior->current_streams[instruction->field.stream].stride;

        switch (instruction->opcode) {
        case 1:
            _pfxvm_init_reflect(destination, stride, particle_count);
            break;
        case 2:
            _pfxvm_init_set_float_range(destination, stride, particle_count,
                                        &instruction->argument.range);
            break;
        case 3:
            switch (pfx_field_get_type(instruction->field.description)) {
            case 1:
                _pfxvm_init_multiply_float_range_v3(
                    destination, stride, particle_count,
                    &instruction->argument.range);
                break;
            case 3:
                _pfxvm_init_multiply_float_range(
                    destination, stride, particle_count,
                    &instruction->argument.range);
                break;
            }
            break;
        case 4:
            _pfxvm_init_add_v3(
                destination, stride, particle_count,
                pfx_get_field(behavior->effect, -2,
                              instruction->argument.source_field),
                pfx_get_struct_size(behavior->effect,
                                    instruction->argument.source_field));
            break;
        case 5:
            _pfxvm_init_divert(destination, stride, particle_count,
                               &instruction->argument.range);
            break;
        }
    }
}

void pfxvm_initial_reflect(PfxBehavior* behavior, unsigned int field)
{
    add_init_insn(behavior, 1, field);
}

#pragma opt_propagation off
void pfxvm_initial_set_float_range(PfxBehavior* behavior, unsigned int field,
                                   PfxFloatRange* range)
{
    PfxInitInstruction* instruction = add_init_insn(behavior, 2, field);
    PfxFloatRange* destination = &instruction->argument.range;

    memcpy(destination, range, sizeof(*range));
}
#pragma opt_propagation reset

void pfxvm_initial_multiply_float_range(PfxBehavior* behavior,
                                        unsigned int field,
                                        PfxFloatRange* range)
{
    PfxInitInstruction* instruction = add_init_insn(behavior, 3, field);
    PfxFloatRange* destination = &instruction->argument.range;

    memcpy(destination, range, sizeof(*range));
}

void pfxvm_initial_add_v3(PfxBehavior* behavior, unsigned int field,
                          unsigned int source_field)
{
    PfxInitInstruction* instruction = add_init_insn(behavior, 4, field);

    instruction->argument.source_field = source_field;
}

void pfxvm_initial_divert(PfxBehavior* behavior, unsigned int field,
                          PfxFloatRange* range)
{
    PfxInitInstruction* instruction = add_init_insn(behavior, 5, field);
    PfxFloatRange* destination = &instruction->argument.range;

    memcpy(destination, range, sizeof(*range));
}
