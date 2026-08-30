#include "libmkparticle/behavior.h"
#include "libmkparticle/particle.h"
#include "runtime/cstring.h"

float rnd_between(float minimum, float maximum);
void rnd_bend_vector(PfxVec3* vector, float angle, float angle_spread);
int pfx_get_struct_size(PfxVm* pfx, unsigned int field);

static void _pfxvm_init_reflect(unsigned char* data, int stride, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        PfxVec3* vector = (PfxVec3*)data;

        vector->y = -vector->y;
        data += stride;
    }
}

static void _pfxvm_init_set_float_range(unsigned char* data, int stride,
                                        int count,
                                        PfxFloatRange* range)
{
    while (count-- != 0) {
        *(float*)data = rnd_between(range->center - range->variation,
                                    range->center + range->variation);
        data += stride;
    }
}

static void _pfxvm_init_multiply_float_range(
    unsigned char* data, int stride, int count, PfxFloatRange* range)
{
    while (count-- != 0) {
        float* value = (float*)data;

        *value *= rnd_between(range->center - range->variation,
                              range->center + range->variation);
        data += stride;
    }
}

static void _pfxvm_init_multiply_float_range_v3(
    unsigned char* data, int stride, int count, PfxFloatRange* range)
{
    while (count-- != 0) {
        PfxVec3* vector = (PfxVec3*)data;
        float scale = rnd_between(range->center - range->variation,
                                  range->center + range->variation);

        vector->x *= scale;
        vector->y *= scale;
        vector->z *= scale;
        data += stride;
    }
}

static void _pfxvm_init_add_v3(unsigned char* destination,
                               int destination_stride, int count,
                               unsigned char* source, int source_stride)
{
    int index;

    for (index = 0; index < count; index++) {
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
    while (count-- != 0) {
        rnd_bend_vector((PfxVec3*)data, range->center, range->variation);
        data += stride;
    }
}

void _pfxvm_execute_initial_behavior(PfxBehavior* behavior, float frame_time)
{
    int particle_count = behavior->active_particle_count;
    PfxInitInstruction* instruction = behavior->init_instructions;
    int index;

    (void)frame_time;
    if (particle_count == 0) {
        return;
    }

    for (index = 0; index < behavior->init_instruction_count;
         index++, instruction++) {
        PfxFieldBuffer* field =
            &behavior->current_streams[instruction->field.stream];
        unsigned char* destination = field->data + instruction->field.offset;
        int stride = field->stride;

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
        case 4: {
            int source_stride = pfx_get_struct_size(
                behavior->effect, instruction->argument.source_field);
            unsigned char* source = pfx_get_field(
                behavior->effect, -2, instruction->argument.source_field);

            _pfxvm_init_add_v3(destination, stride, particle_count, source,
                               source_stride);
            break;
        }
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

void pfxvm_initial_set_float_range(PfxBehavior* behavior, unsigned int field,
                                   PfxFloatRange* range)
{
    PfxInitInstruction* instruction = add_init_insn(behavior, 2, field);
    PfxFloatRange* destination = &instruction->argument.range;

    memcpy(destination, range, sizeof(*range));
}

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
