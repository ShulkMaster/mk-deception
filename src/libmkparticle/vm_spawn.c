#include "libmkparticle/spawn.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/streams.h"
#include "math/gxMath.h"

void* memcpy(void* destination, const void* source, unsigned long size);
int rand(void);
float rnd_between(float minimum, float maximum);
int rnd_int(unsigned int maximum);
void rnd_line_1i(int minimum, int maximum, int* output);
void rnd_sphere(PfxVec3* output, const PfxVec3* origin, int quadratic_radius,
                float minimum_radius, float maximum_radius);
void rnd_point_in_cylinder(PfxVec3* output, const PfxVec3* axis,
                           float radial_center, float radial_spread,
                           float axial_center, float axial_spread);
void rnd_point_in_disc(PfxVec3* output, const PfxVec3* axis,
                       float minimum_radius, float maximum_radius);
void rnd_point_in_sphere_section(PfxVec3* output, const PfxVec3* axis,
                                 float radius, float radius_spread,
                                 float angle, float angle_spread);
void rnd_vector_from_point(PfxVec3* output, const PfxVec3* start,
                           const PfxVec3* end, float minimum_length,
                           float length_range);
void* pfx_get_field(PfxVm* pfx, int particle, unsigned int field);
int pfx_get_struct_size(PfxVm* pfx, unsigned int field);
int pfx_field_get_type(unsigned int field);
PfxVmEmitter* pfx_get_emitter(PfxVm* pfx, int emitter_index);
void pfx_halt(const char* message);

static char spawn_messages[150];

enum {
    PFX_MESSAGE_INVALID_DATA_TYPE = 0,
    PFX_MESSAGE_TOO_MANY_INSTRUCTIONS = 18,
    PFX_MESSAGE_COLOR_FIELD_MISMATCH = 51
};

int has_spawncode_for(PfxVmEmitter* emitter, unsigned int field)
{
    if (pfx_emitter_find_insn(emitter, field) != 0) {
        return 1;
    }
    return 0;
}

static void v3_x_mat_4(PfxVec3* output, const PfxVec3* vector,
                       const PfxMatrix* matrix)
{
    output->x = matrix->elements[12] +
        (vector->z * matrix->elements[8] +
         (vector->x * matrix->elements[0] + vector->y * matrix->elements[4]));
    output->y = matrix->elements[13] +
        (vector->z * matrix->elements[9] +
         (vector->x * matrix->elements[1] + vector->y * matrix->elements[5]));
    output->z = matrix->elements[14] +
        (vector->z * matrix->elements[10] +
         (vector->x * matrix->elements[2] + vector->y * matrix->elements[6]));
}

static void rotate_v3_by_mat4(PfxVec3* vector, const PfxMatrix* matrix)
{
    PfxVec3 result;
    float x = vector->x;
    float y = vector->y;
    float z = vector->z;

    result.x = z * matrix->elements[8] +
        (x * matrix->elements[0] + y * matrix->elements[4]);
    result.y = z * matrix->elements[9] +
        (x * matrix->elements[1] + y * matrix->elements[5]);
    result.z = z * matrix->elements[10] +
        (x * matrix->elements[2] + y * matrix->elements[6]);
    *vector = result;
}

static void _pfxvm_spawn_point(PfxVec3* output, const PfxVec3* point)
{
    output->x = point->x;
    output->y = point->y;
    output->z = point->z;
}

static void _pfxvm_spawn_add(PfxVec3* output, const PfxVec3* amount)
{
    output->x += amount->x;
    output->y += amount->y;
    output->z += amount->z;
}

static void _pfxvm_spawn_point_color(PfxColor* output, const PfxColor* color)
{
    *output = *color;
}

void pfx_spawn_box(PfxVec3* output, float x, float y, float z,
                   float width, float height, float depth)
{
    output->x = rnd_between(x, x + width);
    output->y = rnd_between(y, y + height);
    output->z = rnd_between(z, z + depth);
}

static void pfx_random_cone(PfxVec3* output, float x, float y, float z,
                            float angle, float distance_range)
{
    float distance;
    float radius;
    float direction;

    distance = 0.03f + distance_range * ((float)rand() / 32767.0f);
    radius = distance * gxMathTan(3.1415927f * (angle / 180.0f));
    direction = 3.1415927f * (2.0f * ((float)rand() / 32767.0f));
    output->x = x + radius * gxMathCos(direction);
    output->z = z + radius * gxMathSin(direction);
    output->y = y + distance;
}

static void _pfxvm_spawn_cone(PfxVec3* output, const PfxVec3* origin,
                              float angle, float distance_range)
{
    pfx_random_cone(output, origin->x, origin->y, origin->z,
                    angle, distance_range);
}

static void _pfxvm_spawn_box(PfxVec3* output,
                             const PfxSpawnArguments* arguments)
{
    pfx_spawn_box(output, arguments->box.minimum.x,
                  arguments->box.minimum.y, arguments->box.minimum.z,
                  arguments->box.extent.x, arguments->box.extent.y,
                  arguments->box.extent.z);
}

static void _pfxvm_spawn_table(void* output, const PfxSpawnTable* table)
{
    int index = rnd_int(table->count);

    if (table->type == 2) {
        *(unsigned int*)output = ((unsigned int*)table->values)[index];
    } else if (table->type == 3) {
        *(float*)output = ((float*)table->values)[index];
    } else {
        *(PfxVec3*)output = ((PfxVec3*)table->values)[index];
    }
}

static void _pfxvm_link_to_table(void* output, const PfxSpawnTable* table,
                                 int index)
{
    switch (table->type) {
    case 2:
        *(unsigned int*)output = ((unsigned int*)table->values)[index];
        break;
    case 3:
        *(float*)output = ((float*)table->values)[index];
        break;
    case 1:
        memcpy(output, &((PfxVec3*)table->values)[index], sizeof(PfxVec3));
        break;
    case 4:
        *(int*)output = ((int*)table->values)[index];
        break;
    default:
        pfx_halt(&spawn_messages[PFX_MESSAGE_INVALID_DATA_TYPE]);
        break;
    }
}

static PfxEmitterInstruction* add_emitter_insn(PfxVmEmitter* emitter,
                                                int opcode,
                                                unsigned int field)
{
    PfxEmitterInstruction* instruction;
    int index;

    if (emitter->flags.bits.emission_enabled != 0) {
        return 0;
    }
    if (opcode != 12) {
        if (has_spawncode_for(emitter, field) != 0) {
            return 0;
        }
    } else if (has_spawncode_for(emitter, field) == 0) {
        return 0;
    }
    if (emitter->instruction_count >= 8) {
        pfx_halt(&spawn_messages[PFX_MESSAGE_TOO_MANY_INSTRUCTIONS]);
    }

    index = emitter->instruction_count;
    emitter->instruction_count = index + 1;
    instruction = &emitter->instructions[index];
    instruction->opcode = opcode;
    instruction->field_description = field;
    return instruction;
}

void pfxvm_spawn_set_field_from_table(PfxVmEmitter* emitter,
                                      unsigned int field,
                                      PfxSpawnTable* table)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 5, field);

    if (instruction != 0) {
        instruction->spawn.table.table = table;
        instruction->spawn.table.field = field;
    }
}

void pfxvm_spawn_box(PfxVmEmitter* emitter, unsigned int field,
                     float x, float y, float z,
                     float width, float height, float depth)
{
    PfxEmitterInstruction* instruction =
        &emitter->instructions[emitter->instruction_count];
    instruction->opcode = 3;
    instruction->field_description = field;
    instruction->spawn.box.minimum.x = x;
    instruction->spawn.box.minimum.y = y;
    instruction->spawn.box.minimum.z = z;
    instruction->spawn.box.extent.x = width;
    instruction->spawn.box.extent.y = height;
    instruction->spawn.box.extent.z = depth;
    emitter->instruction_count++;
}

void pfxvm_spawn_cylinder(PfxVmEmitter* emitter, unsigned int field,
                          const PfxVec3* axis, float radial_center,
                          float radial_spread, float axial_center,
                          float axial_spread)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 13, field);
    if (instruction != 0) {
        memcpy(&instruction->spawn.shape.axis, axis, sizeof(PfxVec3));
        instruction->spawn.shape.argument0 = radial_center;
        instruction->spawn.shape.argument1 = radial_spread;
        instruction->spawn.shape.argument2 = axial_center;
        instruction->spawn.shape.argument3 = axial_spread;
    }
}

void pfxvm_spawn_disc(PfxVmEmitter* emitter, unsigned int field,
                      const PfxVec3* axis, float minimum_radius,
                      float maximum_radius)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 14, field);
    if (instruction != 0) {
        memcpy(&instruction->spawn.shape.axis, axis, sizeof(PfxVec3));
        instruction->spawn.shape.argument0 = minimum_radius;
        instruction->spawn.shape.argument1 = maximum_radius;
    }
}

void pfxvm_spawn_roundrobin_mechanism(PfxVmEmitter* emitter,
                                      unsigned int field, int count)
{
    if (pfx_field_get_type(field) == 5) {
        PfxEmitterInstruction* instruction =
            &emitter->instructions[emitter->instruction_count];
        instruction->opcode = 16;
        instruction->field_description = field;
        instruction->spawn.integer_range.minimum = 0;
        instruction->spawn.integer_range.maximum = count;
        emitter->instruction_count++;
    }
}

void pfxvm_spawn_line_1i(PfxVmEmitter* emitter, unsigned int field,
                         int minimum, int maximum)
{
    if (pfx_field_get_type(field) == 4) {
        PfxEmitterInstruction* instruction =
            &emitter->instructions[emitter->instruction_count];
        instruction->opcode = 8;
        instruction->field_description = field;
        instruction->spawn.integer_range.minimum = minimum;
        instruction->spawn.integer_range.maximum = maximum;
        emitter->instruction_count++;
    }
}

void pfxvm_spawn_line_1f(PfxVmEmitter* emitter, unsigned int field,
                         float minimum, float maximum)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 9, field);
    if (instruction != 0) {
        instruction->spawn.scalar_range.minimum = minimum;
        instruction->spawn.scalar_range.maximum = maximum;
    }
}

void pfxvm_spawn_point_color(PfxVmEmitter* emitter, unsigned int field,
                             float red, float green, float blue, float alpha)
{
    PfxEmitterInstruction* instruction;
    if (pfx_field_get_type(field) != 2) {
        pfx_halt(&spawn_messages[PFX_MESSAGE_COLOR_FIELD_MISMATCH]);
    }
    instruction = add_emitter_insn(emitter, 0, field);
    if (instruction != 0) {
        pfx_native_set_rgba(&instruction->spawn.color,
                            red, green, blue, alpha);
    }
}

void pfxvm_spawn_sphere(PfxVmEmitter* emitter, unsigned int field,
                        float x, float y, float z, float minimum_radius,
                        float maximum_radius, int quadratic_radius)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 10, field);
    if (instruction != 0) {
        instruction->spawn.shape.axis.x = x;
        instruction->spawn.shape.axis.y = y;
        instruction->spawn.shape.axis.z = z;
        instruction->spawn.shape.argument0 = minimum_radius;
        instruction->spawn.shape.argument1 = maximum_radius;
        instruction->spawn.shape.option = quadratic_radius;
    }
}

void pfxvm_spawn_from_pos(PfxVmEmitter* emitter, unsigned int field,
                          unsigned int source_field, int clamp_y,
                          float x, float y, float z, float minimum_length,
                          float length_range, float clamped_y)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 15, field);
    if (instruction != 0) {
        instruction->spawn.from_position.offset.x = x;
        instruction->spawn.from_position.offset.y = y;
        instruction->spawn.from_position.offset.z = z;
        instruction->spawn.from_position.minimum_length = minimum_length;
        instruction->spawn.from_position.length_range = length_range;
        instruction->spawn.from_position.source_field = source_field;
        instruction->spawn.from_position.clamp_y = clamp_y;
        instruction->spawn.from_position.y = clamped_y;
    }
}

void pfxvm_spawn_sphere_section(PfxVmEmitter* emitter, unsigned int field,
                                float x, float y, float z, float radius,
                                float radius_spread, float angle,
                                float angle_spread)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 11, field);
    if (instruction != 0) {
        instruction->spawn.shape.axis.x = x;
        instruction->spawn.shape.axis.y = y;
        instruction->spawn.shape.axis.z = z;
        instruction->spawn.shape.argument0 = radius;
        instruction->spawn.shape.argument1 = radius_spread;
        instruction->spawn.shape.argument2 = angle;
        instruction->spawn.shape.argument3 = angle_spread;
    }
}

void pfxvm_spawn_value(PfxVmEmitter* emitter, unsigned int field, float value)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 2, field);
    if (instruction != 0) {
        instruction->spawn.scalars[0] = value;
    }
}

void pfxvm_spawn_uv(PfxVmEmitter* emitter, unsigned int field, float u, float v)
{
    PfxEmitterInstruction* instruction = add_emitter_insn(emitter, 1, field);
    if (instruction != 0) {
        instruction->spawn.uv.u = u;
        instruction->spawn.uv.v = v;
    }
}

void __pfxvm_execute_spawn(PfxVm* pfx, PfxVmEmitter* emitter)
{
    PfxEmitterInstruction* instruction;
    int instruction_index;

    pfx->field_0x1E0 = 0;
    if (emitter->flags.bits.emission_enabled == 0) {
        return;
    }
    instruction = emitter->instructions;
    for (instruction_index = 0;
         instruction_index < emitter->instruction_count;
         instruction_index++, instruction++) {
        int particle_offset;
        unsigned char* destination;
        PfxVec3 transformed;

        if (pfx->behavior_list != 0 && pfx->behavior_list[0] != 0) {
            particle_offset = pfx->behavior_list[0]->active_particle_count *
                pfx_get_struct_size(pfx, instruction->field_description);
        } else {
            particle_offset = pfx->particle_cursor *
                pfx_get_struct_size(pfx, instruction->field_description);
        }
        destination = pfx_get_field(pfx, -2, instruction->field_description);
        destination += particle_offset;
        if (instruction->opcode < 9) {
            switch (instruction->opcode) {
            case 0:
                if (pfx_field_get_type(instruction->field_description) == 2) {
                    _pfxvm_spawn_point_color(
                        (PfxColor*)destination, &instruction->spawn.color);
                } else {
                    _pfxvm_spawn_point(
                        (PfxVec3*)destination, &instruction->spawn.point);
                }
                break;
            case 3:
                _pfxvm_spawn_box((PfxVec3*)destination, &instruction->spawn);
                break;
            case 4:
                _pfxvm_spawn_cone(
                    (PfxVec3*)destination, &instruction->spawn.shape.axis,
                    instruction->spawn.shape.argument0,
                    instruction->spawn.shape.argument1);
                break;
            case 5:
                _pfxvm_spawn_table(destination,
                                   instruction->spawn.table.table);
                break;
            case 6: {
                int source_offset = pfx->particle_cursor *
                    pfx_get_struct_size(
                        pfx, instruction->spawn.table.source_field);
                unsigned char* source = pfx_get_field(
                    pfx, -2, instruction->spawn.table.source_field);
                int table_index;
                source += source_offset;
                if (pfx_field_get_type(
                        instruction->spawn.table.source_field) == 3) {
                    table_index = (int)*(float*)source;
                } else {
                    table_index = *(int*)source;
                }
                _pfxvm_link_to_table(destination,
                                     instruction->spawn.table.table,
                                     table_index);
                break;
            }
            case 7: {
                int table_index;
                int position_offset;
                unsigned char* position;
                const PfxVec3* table_position;
                rnd_line_1i(0, instruction->spawn.table.table->count - 1,
                            (int*)destination);
                table_index = *(int*)destination;
                position_offset = pfx->particle_cursor *
                    pfx_get_struct_size(pfx, 0x100);
                position = pfx_get_field(pfx, -2, 0x100);
                position += position_offset;
                table_position =
                    &((PfxVec3*)instruction->spawn.table.table->values)
                        [table_index];
                ((PfxVec3*)position)->x = table_position->x;
                ((PfxVec3*)position)->y = table_position->y;
                ((PfxVec3*)position)->z = table_position->z;
                break;
            }
            case 8:
                rnd_line_1i(instruction->spawn.integer_range.minimum,
                            instruction->spawn.integer_range.maximum,
                            (int*)destination);
                break;
            case 2:
                *(float*)destination = instruction->spawn.scalars[0];
                break;
            case 1:
                *(PfxTextureFrame*)destination = instruction->spawn.uv;
                break;
            default:
                return;
            }
        } else {
            switch (instruction->opcode) {
            case 9:
                *(float*)destination = rnd_between(
                    instruction->spawn.scalar_range.minimum,
                    instruction->spawn.scalar_range.maximum);
                break;
            case 10:
                rnd_sphere((PfxVec3*)destination,
                           &instruction->spawn.shape.axis,
                           instruction->spawn.shape.option,
                           instruction->spawn.shape.argument0,
                           instruction->spawn.shape.argument1);
                break;
            case 11:
                rnd_point_in_sphere_section(
                    (PfxVec3*)destination, &instruction->spawn.shape.axis,
                    instruction->spawn.shape.argument0,
                    instruction->spawn.shape.argument1,
                    instruction->spawn.shape.argument2,
                    instruction->spawn.shape.argument3);
                break;
            case 12:
                _pfxvm_spawn_add((PfxVec3*)destination,
                                 &instruction->spawn.point);
                break;
            case 13:
                rnd_point_in_cylinder(
                    (PfxVec3*)destination, &instruction->spawn.shape.axis,
                    instruction->spawn.shape.argument0,
                    instruction->spawn.shape.argument1,
                    instruction->spawn.shape.argument2,
                    instruction->spawn.shape.argument3);
                break;
            case 14:
                rnd_point_in_disc(
                    (PfxVec3*)destination, &instruction->spawn.shape.axis,
                    instruction->spawn.shape.argument0,
                    instruction->spawn.shape.argument1);
                break;
            case 15: {
                unsigned int source_field =
                    instruction->spawn.from_position.source_field;
                PfxTransform* transform = (PfxTransform*)emitter->transform;
                int source_offset;
                unsigned char* source;
                PfxVec3 end;
                PfxVec3 start;
                if (pfx->behavior_list != 0 && pfx->behavior_list[0] != 0) {
                    source_offset =
                        pfx->behavior_list[0]->active_particle_count *
                        pfx_get_struct_size(pfx, source_field);
                } else {
                    source_offset = pfx->particle_cursor *
                        pfx_get_struct_size(pfx, source_field);
                }
                source = pfx_get_field(pfx, -2, source_field);
                source += source_offset;
                end = *(PfxVec3*)source;
                if (transform != 0) {
                    end.x -= transform->position.x;
                    end.y -= transform->position.y;
                    end.z -= transform->position.z;
                }
                start.x = emitter->position.x +
                    instruction->spawn.from_position.offset.x;
                start.y = emitter->position.y +
                    instruction->spawn.from_position.offset.y;
                start.z = emitter->position.z +
                    instruction->spawn.from_position.offset.z;
                rnd_vector_from_point(
                    (PfxVec3*)destination, &start, &end,
                    instruction->spawn.from_position.minimum_length,
                    instruction->spawn.from_position.length_range);
                if (instruction->spawn.from_position.clamp_y == 1) {
                    ((PfxVec3*)destination)->y =
                        instruction->spawn.from_position.y;
                }
                break;
            }
            case 16:
                ((int*)destination)[0] =
                    pfx->total_birth_count + pfx->particle_capacity -
                    instruction->spawn.integer_range.maximum;
                ((int*)destination)[1] =
                    instruction->spawn.integer_range.maximum;
                break;
            default:
                return;
            }
        }

        if (instruction->field_description == 0x100 ||
            instruction->field_description == 0x400) {
            _pfxvm_spawn_add((PfxVec3*)destination, &emitter->position);
        }
        if (emitter->transform != 0) {
            PfxTransform* transform = (PfxTransform*)emitter->transform;
            switch (instruction->field_description) {
            case 0x100:
            case 0x400:
                v3_x_mat_4(&transformed, (PfxVec3*)destination,
                           &transform->matrix);
                memcpy(destination, &transformed, sizeof(transformed));
                if (instruction->field_description == 0x100 &&
                    emitter->field_40 != 0) {
                    switch (emitter->field_40) {
                    case 2:
                        v3_x_mat_4(&transformed, (PfxVec3*)destination,
                                   (PfxMatrix*)pfx);
                        break;
                    case 1:
                        transformed.x = ((PfxVec3*)destination)->x +
                            ((PfxMatrix*)pfx)->elements[12];
                        transformed.y = ((PfxVec3*)destination)->y +
                            ((PfxMatrix*)pfx)->elements[13];
                        transformed.z = ((PfxVec3*)destination)->z +
                            ((PfxMatrix*)pfx)->elements[14];
                        break;
                    default:
                        transformed.x = 0.0f;
                        transformed.y = 0.0f;
                        transformed.z = 0.0f;
                        break;
                    }
                    memcpy(destination, &transformed, sizeof(transformed));
                }
                break;
            case 0x300:
                rotate_v3_by_mat4((PfxVec3*)destination,
                                  &transform->matrix);
                break;
            case 0x103:
                *(float*)destination += gxMathArcTanYX(
                    transform->matrix.elements[2],
                    transform->matrix.elements[0]);
                break;
            }
        }
    }

    pfx->particle_cursor++;
    if (pfx->behavior_list != 0 && pfx->behavior_list[0] != 0) {
        pfx->behavior_list[0]->active_particle_count++;
    }
    pfx->total_birth_count++;
}

void _pfxvm_execute_spawn(PfxVm* pfx, int emitter_index)
{
    PfxVmEmitter* emitter = pfx_get_emitter(pfx, emitter_index);
    __pfxvm_execute_spawn(pfx, emitter);
}

/* Retail retains the final link-order diagnostic even though this TU has no
 * surviving code reference to it. */
static char spawn_messages[150] =
    "Invalid data type\0"
    "PFX: Too many spawn instructions\0"
    "PFX: Tried to spawn color in mismatched field\0"
    "Linking to table without spawning index field first!";

void pfxvm_add_transfer(PfxVm* pfx, PfxEmitterTransfer* transfer)
{
    transfer->next = pfx->emitter_transfers;
    pfx->emitter_transfers = transfer;
}

void pfxvm_create_transfer(PfxVm* destination, PfxVm* source)
{
    PfxBehavior* behavior;
    PfxEmitterTransfer* transfer;

    if (source->behavior_list == 0 || source->behavior_count < 1) {
        return;
    }
    transfer = streampool_alloc(1, sizeof(PfxEmitterTransfer));
    if (transfer == 0) {
        return;
    }
    behavior = source->behavior_list[source->behavior_count - 1];
    transfer->particle_count = behavior->particle_count;
    transfer->source = behavior->stream_100 +
        get_field_offset((PfxTableRegistry*)source, 0x100);
    transfer->source_stride = behavior->current_stream_100_stride;
    pfxvm_add_transfer(destination, transfer);
}
