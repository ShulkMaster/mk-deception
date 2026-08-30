#include "libmkparticle/update.h"

#include "libmkparticle/particle.h"
#include "libmkparticle/spawn.h"
#include "libmkparticle/texture_anim.h"

double ceil(double value);
double pow(double base, double exponent);
float rnd_between(float minimum, float maximum);
void* pfx_get_field(PfxVm* pfx, int particle, unsigned int field);
int get_field_size(int type);

typedef struct PfxTextureFrameSource {
    char pad00[0x10];
    PfxTextureFrame* frames;
} PfxTextureFrameSource;

static PfxVmField* fieldstack[2] = { 0, 0 };
static int fieldstack_top;
PfxVm* g_current_effect;

static void push_field(PfxVmField* field)
{
    int top = fieldstack_top;
    if (top < 2) {
        fieldstack_top = top + 1;
        fieldstack[top] = field;
    }
}

static void pop_field(PfxVmField** field)
{
    if (fieldstack_top > 0) {
        int top = fieldstack_top - 1;
        fieldstack_top = top;
        *field = fieldstack[top];
    }
}

static int fieldstack_free(void)
{
    return 2 - fieldstack_top;
}

static void do_add_fields_v3(int count, const unsigned char* source,
                             unsigned char* destination,
                             const unsigned char* amount,
                             int destination_stride, int amount_stride,
                             float frame_time)
{
    while (count-- > 0) {
        const PfxVec3* source_value = (const PfxVec3*)source;
        PfxVec3* destination_value = (PfxVec3*)destination;
        const PfxVec3* amount_value = (const PfxVec3*)amount;
        destination_value->x = source_value->x + amount_value->x * frame_time;
        destination_value->y = source_value->y + amount_value->y * frame_time;
        destination_value->z = source_value->z + amount_value->z * frame_time;
        destination += destination_stride;
        source += destination_stride;
        amount += amount_stride;
    }
}

static void do_add_fields(int count, const unsigned char* source,
                          unsigned char* destination,
                          const unsigned char* amount,
                          int destination_stride, int amount_stride,
                          float frame_time)
{
    while (count-- > 0) {
        *(float*)destination =
            *(const float*)source + *(const float*)amount * frame_time;
        destination += destination_stride;
        source += destination_stride;
        amount += amount_stride;
    }
}

static void do_bounce(int count, unsigned char* positions,
                      int position_stride, unsigned char* velocities,
                      int velocity_stride, const unsigned char* bounce_counts,
                      unsigned char* updated_bounce_counts,
                      int bounce_count_stride, float scale)
{
    float* ground = pfx_get_field(0, -2, 0x500);
    float ground_y = ground != 0 ? *ground : 0.0f;
    while (count-- > 0) {
        PfxVec3* position = (PfxVec3*)positions;
        PfxVec3* velocity = (PfxVec3*)velocities;
        int bounce_count = *(const int*)bounce_counts;
        if (position->y < ground_y) {
            if (bounce_count != 0) {
                bounce_count--;
                velocity->x *= scale;
                velocity->y *= -scale;
                velocity->z *= scale;
                position->x += velocity->x;
                position->y += velocity->y;
                position->z += velocity->z;
                if (position->y < ground_y) {
                    position->y = ground_y;
                }
            } else {
                velocity->x = velocity->y = velocity->z = 0.0f;
                position->y = ground_y;
            }
        }
        *(int*)updated_bounce_counts = bounce_count;
        bounce_counts += bounce_count_stride;
        updated_bounce_counts += bounce_count_stride;
        positions += position_stride;
        velocities += velocity_stride;
    }
}

static void copy_color_from_table_int(const PfxColor* values,
                                      const unsigned char* indices,
                                      int index_stride,
                                      unsigned char* destination,
                                      int destination_stride, int count)
{
    while (count-- > 0) {
        *(PfxColor*)destination = values[*(const int*)indices];
        indices += index_stride;
        destination += destination_stride;
    }
}

static void copy_color_from_table_float(const PfxColor* values,
                                        const unsigned char* indices,
                                        int index_stride,
                                        unsigned char* destination,
                                        int destination_stride, int count)
{
    while (count-- > 0) {
        *(PfxColor*)destination = values[(int)*(const float*)indices];
        indices += index_stride;
        destination += destination_stride;
    }
}

static void do_copy_from_table_int(int count, PfxSpawnTable* table,
                                   unsigned char* destination,
                                   unsigned char* indices,
                                   int index_stride, int destination_stride)
{
    switch (table->type) {
    case 3:
        while (count-- > 0) {
            *(float*)destination =
                ((const float*)table->values)[*(const int*)indices];
            destination += destination_stride;
            indices += index_stride;
        }
        break;
    case 2:
        copy_color_from_table_int((const PfxColor*)table->values, indices,
                                  index_stride, destination,
                                  destination_stride, count);
        break;
    case 1:
        while (count-- > 0) {
            PfxVec3* output = (PfxVec3*)destination;
            output->x = ((PfxVec3*)table->values)[*(const int*)indices].x;
            output->y = ((PfxVec3*)table->values)[*(const int*)indices].y;
            output->z = ((PfxVec3*)table->values)[*(const int*)indices].z;
            destination += destination_stride;
            indices += index_stride;
        }
        break;
    }
}

static void do_copy_from_table_float(int count, PfxSpawnTable* table,
                                     unsigned char* destination,
                                     unsigned char* indices,
                                     int index_stride,
                                     int destination_stride)
{
    switch (table->type) {
    case 3:
        while (count-- > 0) {
            *(float*)destination =
                ((const float*)table->values)[(int)*(const float*)indices];
            destination += destination_stride;
            indices += index_stride;
        }
        break;
    case 2:
        copy_color_from_table_float((const PfxColor*)table->values, indices,
                                    index_stride, destination,
                                    destination_stride, count);
        break;
    case 1:
        while (count-- > 0) {
            PfxVec3* output = (PfxVec3*)destination;
            output->x = ((PfxVec3*)table->values)[
                (int)*(const float*)indices].x;
            output->y = ((PfxVec3*)table->values)[
                (int)*(const float*)indices].y;
            output->z = ((PfxVec3*)table->values)[
                (int)*(const float*)indices].z;
            destination += destination_stride;
            indices += index_stride;
        }
        break;
    }
}

static void do_wrapbox(int count, const unsigned char* source,
                       unsigned char* destination, int stride,
                       const PfxUpdateArguments* arguments)
{
    const PfxMatrix* matrix = (const PfxMatrix*)g_current_effect;
    PfxVec3 center;
    PfxVec3 maximum;
    PfxVec3 minimum;
    PfxVec3 size;
    PfxVec3 inverse_size;
    int index;
    center.x = matrix->elements[8] * arguments->vector.scale;
    center.y = matrix->elements[9] * arguments->vector.scale;
    center.z = matrix->elements[10] * arguments->vector.scale;
    center.x += matrix->elements[12];
    center.y += matrix->elements[13];
    center.z += matrix->elements[14];
    maximum.x = center.x + arguments->vector.x;
    maximum.y = center.y + arguments->vector.y;
    maximum.z = center.z + arguments->vector.z;
    minimum.x = center.x - arguments->vector.x;
    minimum.y = center.y - arguments->vector.y;
    minimum.z = center.z - arguments->vector.z;
    size.x = 2.0f * arguments->vector.x;
    size.y = 2.0f * arguments->vector.y;
    size.z = 2.0f * arguments->vector.z;
    inverse_size.x = 1.0f / size.x;
    inverse_size.y = 1.0f / size.y;
    inverse_size.z = 1.0f / size.z;
    for (index = 0; index < count; index++) {
        const PfxVec3* input = (const PfxVec3*)source;
        PfxVec3* output = (PfxVec3*)destination;
        if (input->x < minimum.x || input->y < minimum.y ||
            input->z < minimum.z || input->x > maximum.x ||
            input->y > maximum.y || input->z > maximum.z) {
            float wrap_x = (minimum.x - input->x) * inverse_size.x;
            float wrap_y = (minimum.y - input->y) * inverse_size.y;
            float wrap_z = (minimum.z - input->z) * inverse_size.z;
            float step_x = (float)ceil(wrap_x);
            float step_y = (float)ceil(wrap_y);
            float step_z = (float)ceil(wrap_z);
            float offset_x = step_x * size.x;
            float offset_y = step_y * size.y;
            float offset_z = step_z * size.z;
            output->x = offset_x + input->x;
            output->y = offset_y + input->y;
            output->z = offset_z + input->z;
        } else {
            output->x = input->x;
            output->y = input->y;
            output->z = input->z;
        }
        source += stride;
        destination += stride;
    }
}

static void scalar_multiply(int count, unsigned char* values, int stride,
                            float x, float y, float z)
{
    while (count-- > 0) {
        PfxVec3* value = (PfxVec3*)values;
        value->x *= x;
        value->y *= y;
        value->z *= z;
        values += stride;
    }
}

static void do_add_constant_v3(int count, const unsigned char* source,
                               unsigned char* destination, int stride,
                               float frame_time, float x, float y, float z)
{
    while (count-- > 0) {
        const PfxVec3* input = (const PfxVec3*)source;
        PfxVec3* output = (PfxVec3*)destination;
        output->x = input->x + x * frame_time;
        output->y = input->y + y * frame_time;
        output->z = input->z + z * frame_time;
        source += stride;
        destination += stride;
    }
}

static void do_add_constant(int count, const unsigned char* source,
                            unsigned char* destination, int stride,
                            float frame_time, float amount)
{
    while (count-- > 0) {
        *(float*)destination = *(const float*)source + amount * frame_time;
        source += stride;
        destination += stride;
    }
}

static void add_jitter(int count, unsigned char* values, int stride,
                       float x, float y, float z)
{
    int index;
    for (index = 0; index < count; index++) {
        PfxVec3* value = (PfxVec3*)values;
        value->x += rnd_between(-x * 0.5f, x * 0.5f);
        value->y += rnd_between(-y * 0.5f, y * 0.5f);
        value->z += rnd_between(-z * 0.5f, z * 0.5f);
        values += stride;
    }
}

static void do_update_age(int count, const unsigned char* source,
                          unsigned char* destination, int stride,
                          float frame_time)
{
    while (count-- > 0) {
        *(float*)destination = *(const float*)source + frame_time;
        source += stride;
        destination += stride;
    }
}

static void do_update_roundrobin(int count, const unsigned char* source,
                                 unsigned char* destination, int stride)
{
    while (count-- > 0) {
        ((int*)destination)[0] = ((const int*)source)[0];
        ((int*)destination)[1] = ((const int*)source)[1];
        source += stride;
        destination += stride;
    }
}

static void add_oscillate(int count, unsigned char* values, int stride,
                          float x, float y, float z)
{
    int index;
    for (index = 0; index < count; index++) {
        PfxVec3* value = (PfxVec3*)values;
        value->x += rnd_between(-x * 0.5f, x * 0.5f);
        value->y += rnd_between(-y * 0.5f, y * 0.5f);
        value->z += rnd_between(-z * 0.5f, z * 0.5f);
        values += stride;
    }
}

static void do_fade_alpha(int count, const PfxUpdateArguments* arguments,
                          const unsigned char* source,
                          unsigned char* destination, int color_stride,
                          const unsigned char* ages, int age_stride,
                          float frame_time)
{
    float alpha_rate =
        (float)(arguments->fade.start_alpha - arguments->fade.end_alpha) /
        arguments->fade.duration;
    while (count-- > 0) {
        PfxColor* output = (PfxColor*)destination;
        float age;
        float elapsed;
        *output = *(const PfxColor*)source;
        age = *(const float*)ages;
        if (age <= arguments->fade.start_time) {
            output->a = (unsigned char)arguments->fade.start_alpha;
        } else {
            elapsed = age - arguments->fade.start_time;
            if (elapsed > arguments->fade.duration) {
                elapsed = arguments->fade.duration;
            }
            output->a = (unsigned char)(int)(
                (float)arguments->fade.start_alpha - elapsed * alpha_rate);
        }
        destination += color_stride;
        source += color_stride;
        ages += age_stride;
    }
}

static void do_lerp_color(int count, const PfxUpdateArguments* arguments,
                          unsigned char* destination, int color_stride,
                          const unsigned char* ages, int age_stride,
                          float frame_time)
{
    float inverse_duration = 1.0f / arguments->color_lerp.duration;
    const PfxColor* colors = (const PfxColor*)arguments->color_lerp.colors +
                             arguments->color_lerp.first_color;
    int last_color = arguments->color_lerp.last_color;
    float last_color_position = (float)last_color;
    while (count-- > 0) {
        PfxColor* output = (PfxColor*)destination;
        float position = *(const float*)ages * inverse_duration;
        int color_index = (int)position;
        if (position > last_color_position) {
            *output = colors[last_color];
        } else {
            float fraction = position - (float)color_index;
            float inverse_fraction = 1.0f - fraction;
            const PfxColor* color = &colors[color_index];
            output->r = (unsigned char)(int)(
                inverse_fraction * (float)(int)color[0].r +
                fraction * (float)(int)color[1].r);
            output->g = (unsigned char)(int)(
                inverse_fraction * (float)(int)color[0].g +
                fraction * (float)(int)color[1].g);
            output->b = (unsigned char)(int)(
                inverse_fraction * (float)(int)color[0].b +
                fraction * (float)(int)color[1].b);
            output->a = (unsigned char)(int)(
                inverse_fraction * (float)(int)color[0].a +
                fraction * (float)(int)color[1].a);
        }
        destination += color_stride;
        ages += age_stride;
    }
}

static void do_texture_anim(int count, PfxUpdateArguments* arguments,
                            unsigned char* destination, int texture_stride,
                            const unsigned char* ages, int age_stride,
                            float frame_time)
{
    PfxTextureAnim animation;
    const PfxTextureFrameSource* source =
        (const PfxTextureFrameSource*)arguments->texture_anim.frame_source;
    const PfxTextureFrame* frames = source->frames;
    int index;
    animation.frame_time = arguments->texture_anim.frame_time;
    animation.mode = (short)arguments->texture_anim.first_frame;
    animation.frame_count = arguments->texture_anim.frame_count;
    for (index = 0; index < count; index++) {
        int frame = arguments->texture_anim.mode +
                    pfx_texture_getframe(&animation, *(const float*)ages);
        *(PfxTextureFrame*)destination = frames[frame];
        destination += texture_stride;
        ages += age_stride;
    }
}

static void do_attract(int count, unsigned char* source,
                       unsigned char* destination, int stride,
                       PfxVec3* target, float frame_time,
                       float strength)
{
    float factor = (float)pow(strength, frame_time);
    while (count-- > 0) {
        const PfxVec3* input = (const PfxVec3*)source;
        PfxVec3* output = (PfxVec3*)destination;
        output->x = target->x - input->x;
        output->y = target->y - input->y;
        output->z = target->z - input->z;
        output->x *= factor;
        output->y *= factor;
        output->z *= factor;
        output->x = input->x + output->x;
        output->y = input->y + output->y;
        output->z = input->z + output->z;
        destination += stride;
        source += stride;
    }
}

static void do_assign_constant_v3(unsigned char* destination, int stride,
                                  PfxVec3* value, int count)
{
    while (count-- > 0) {
        PfxVec3* output = (PfxVec3*)destination;
        output->x = value->x;
        output->y = value->y;
        output->z = value->z;
        destination += stride;
    }
}

void pfxvm_execute_behavior_update(PfxBehavior* behavior, float frame_time)
{
    PfxUpdateInstruction* instruction;
    int instruction_index;
    if (behavior->particle_count == 0) {
        return;
    }
    instruction = behavior->update_instructions;
    for (instruction_index = 0;
         instruction_index < behavior->update_instruction_count;
         instruction_index++, instruction++) {
        PfxFieldBuffer* previous =
            &behavior->previous_streams[instruction->field.stream];
        PfxFieldBuffer* current =
            &behavior->current_streams[instruction->field.stream];
        int current_stride;
        unsigned char* previous_value;
        unsigned char* current_value;
        unsigned char* argument_value;
        int argument_stride;
        if (previous->data == 0) {
            continue;
        }
        current_stride = current->stride;
        previous_value = previous->data + instruction->field.offset;
        current_value = current->data + instruction->field.offset +
                        current_stride * behavior->active_particle_count;
        switch (instruction->opcode) {
        case 2:
        case 9:
        case 11:
        case 15: {
            int storage = instruction->arguments.words[0] & 0xF00;
            if (storage == 0x200 || storage == 0x500) {
                argument_value = pfx_get_field(
                    behavior->effect, -1,
                    instruction->arguments.field.description);
            } else {
                argument_value =
                    behavior->previous_streams[
                        instruction->arguments.field.stream].data +
                    instruction->arguments.field.offset;
            }
            argument_stride =
                behavior->previous_streams[
                    instruction->arguments.field.stream].stride;
            break;
        }
        default:
            argument_value = 0;
            argument_stride = 0;
            break;
        }
        switch (instruction->opcode) {
        case 2:
            switch (pfx_field_get_type(instruction->field.description)) {
            case 1:
                do_add_fields_v3(behavior->particle_count, previous_value,
                                 current_value, argument_value,
                                 current_stride, argument_stride, frame_time);
                break;
            case 3:
                do_add_fields(behavior->particle_count, previous_value,
                              current_value, argument_value, current_stride,
                              argument_stride, frame_time);
                break;
            }
            break;
        case 3:
            scalar_multiply(behavior->particle_count, current_value,
                            current_stride, instruction->arguments.vector.x,
                            instruction->arguments.vector.y,
                            instruction->arguments.vector.z);
            break;
        case 5:
            add_jitter(behavior->particle_count, current_value,
                       current_stride, instruction->arguments.vector.x,
                       instruction->arguments.vector.y,
                       instruction->arguments.vector.z);
            break;
        case 6:
            add_oscillate(behavior->particle_count, current_value,
                          current_stride,
                          instruction->arguments.vector.x,
                          instruction->arguments.vector.y,
                          instruction->arguments.vector.z);
            break;
        case 7:
            do_wrapbox(behavior->particle_count, current_value, current_value,
                       current_stride, &instruction->arguments);
            break;
        case 8: {
            PfxFieldBuffer source;
            PfxFieldBuffer destination;
            int field_size = get_field_size(instruction->field.description);
            source.data = previous_value;
            source.stride = current_stride;
            destination.data = current_value;
            destination.stride = current_stride;
            field_copy(&destination, &source, field_size,
                       behavior->particle_count);
            break;
        }
        case 9:
            do_assign_constant_v3(current_value, current_stride,
                                  (PfxVec3*)argument_value,
                                  behavior->particle_count);
            break;
        case 10: {
            PfxVmField* index_field =
                &instruction->arguments.table.index_field;
            unsigned char* index_values =
                behavior->previous_streams[index_field->stream].data +
                index_field->offset;
            PfxSpawnTable* table =
                (PfxSpawnTable*)instruction->arguments.table.table;
            if (pfx_field_get_type(index_field->description) == 4) {
                do_copy_from_table_int(behavior->particle_count, table,
                                       current_value, index_values,
                                       behavior->previous_streams[
                                           index_field->stream].stride,
                                       current_stride);
            } else {
                do_copy_from_table_float(behavior->particle_count, table,
                                         current_value, index_values,
                                         behavior->previous_streams[
                                             index_field->stream].stride,
                                         current_stride);
            }
            break;
        }
        case 11: {
            PfxVmField* bounce_field;
            PfxVmField* velocity_field = &instruction->arguments.field;
            float bounce_scale = instruction->scalar_04;
            pop_field(&bounce_field);
            do_bounce(
                behavior->particle_count, current_value, current_stride,
                behavior->current_streams[velocity_field->stream].data +
                    velocity_field->offset +
                    behavior->current_streams[velocity_field->stream].stride *
                        behavior->active_particle_count,
                argument_stride,
                behavior->previous_streams[bounce_field->stream].data +
                    bounce_field->offset,
                behavior->current_streams[bounce_field->stream].data +
                    bounce_field->offset +
                    behavior->previous_streams[bounce_field->stream].stride *
                        behavior->active_particle_count,
                behavior->previous_streams[bounce_field->stream].stride,
                bounce_scale);
            break;
        }
        case 4:
            switch (pfx_field_get_type(instruction->field.description)) {
            case 1:
                do_add_constant_v3(
                    behavior->particle_count, previous_value, current_value,
                    current_stride, frame_time,
                    instruction->arguments.vector.x,
                    instruction->arguments.vector.y,
                    instruction->arguments.vector.z);
                break;
            case 3:
                do_add_constant(behavior->particle_count, previous_value,
                                current_value, current_stride, frame_time,
                                instruction->arguments.vector.x);
                break;
            }
            break;
        case 12: {
            PfxVmField* age_field = &instruction->arguments.fade.age_field;
            PfxFieldBuffer* ages =
                &behavior->current_streams[age_field->stream];
            do_fade_alpha(
                behavior->particle_count, &instruction->arguments,
                previous_value, current_value, current_stride,
                ages->data + age_field->offset +
                    ages->stride * behavior->active_particle_count,
                ages->stride, frame_time);
            break;
        }
        case 13: {
            PfxVmField* age_field =
                &instruction->arguments.color_lerp.age_field;
            PfxFieldBuffer* ages =
                &behavior->current_streams[age_field->stream];
            do_lerp_color(
                behavior->particle_count, &instruction->arguments,
                current_value, current_stride,
                ages->data + age_field->offset +
                    ages->stride * behavior->active_particle_count,
                ages->stride, frame_time);
            break;
        }
        case 14: {
            PfxVmField* age_field =
                &instruction->arguments.texture_anim.age_field;
            PfxFieldBuffer* ages =
                &behavior->current_streams[age_field->stream];
            do_texture_anim(
                behavior->particle_count, &instruction->arguments,
                current_value, current_stride,
                ages->data + age_field->offset +
                    ages->stride * behavior->active_particle_count,
                ages->stride, frame_time);
            break;
        }
        case 17:
            push_field(&instruction->field);
            break;
        case 15:
            do_attract(behavior->particle_count, previous_value, current_value,
                       current_stride, (PfxVec3*)argument_value,
                       frame_time, instruction->scalar_04);
            break;
        case 1:
            do_update_age(behavior->particle_count, previous_value,
                          current_value, current_stride, frame_time);
            break;
        case 16:
            do_update_roundrobin(behavior->particle_count, previous_value,
                                 current_value, current_stride);
            break;
        }
    }
    (void)fieldstack_free();
}

void set_vm_field(PfxVmField* field, unsigned int description)
{
    field->description = description;
    field->stream = map_field_to_stream(description);
}

static PfxUpdateInstruction dummy_instruction;

static PfxUpdateInstruction* add_update_insn(PfxBehavior* behavior,
                                             int opcode,
                                             unsigned int field)
{
    PfxUpdateInstruction* instruction;
    int index = behavior->update_instruction_count;
    if (index >= 12) {
        return &dummy_instruction;
    }
    behavior->update_instruction_count = index + 1;
    instruction = &behavior->update_instructions[index];
    instruction->opcode = opcode;
    set_vm_field(&instruction->field, field);
    return instruction;
}

static void vm_push_field(PfxBehavior* behavior, unsigned int field)
{
    add_update_insn(behavior, 17, field);
}

void pfxvm_update_age(PfxBehavior* behavior, unsigned int field)
{
    if (pfx_field_get_type(field) == 3) {
        add_update_insn(behavior, 1, field);
    }
}

void pfxvm_update_add(PfxBehavior* behavior, unsigned int destination,
                      unsigned int source)
{
    int source_type = pfx_field_get_type(source);
    if (pfx_field_get_type(destination) == source_type) {
        PfxUpdateInstruction* instruction =
            add_update_insn(behavior, 2, destination);
        set_vm_field(&instruction->arguments.field, source);
    }
}

void pfxvm_update_mul_scalar(PfxBehavior* behavior, unsigned int field,
                             float x, float y, float z)
{
    if (pfx_field_get_type(field) == 1) {
        PfxUpdateInstruction* instruction =
            add_update_insn(behavior, 3, field);
        instruction->arguments.vector.x = x;
        instruction->arguments.vector.y = y;
        instruction->arguments.vector.z = z;
    }
}

void pfxvm_update_add_constant_v3(PfxBehavior* behavior, unsigned int field,
                                  float x, float y, float z)
{
    if (pfx_field_get_type(field) == 1) {
        PfxUpdateInstruction* instruction =
            add_update_insn(behavior, 4, field);
        instruction->arguments.vector.x = x;
        instruction->arguments.vector.y = y;
        instruction->arguments.vector.z = z;
    }
}

void pfxvm_update_add_constant(PfxBehavior* behavior, unsigned int field,
                               float value)
{
    PfxUpdateInstruction* instruction = add_update_insn(behavior, 4, field);
    if (pfx_field_get_type(field) == 3) {
        instruction->arguments.vector.x = value;
    }
}

void pfxvm_update_wrapbox(PfxBehavior* behavior, unsigned int field,
                          float scale, float x, float y, float z)
{
    PfxUpdateInstruction* instruction = add_update_insn(behavior, 7, field);
    instruction->arguments.vector.scale = scale;
    instruction->arguments.vector.x = x;
    instruction->arguments.vector.y = y;
    instruction->arguments.vector.z = z;
}

void pfxvm_update_copy(PfxBehavior* behavior, unsigned int field)
{
    add_update_insn(behavior, 8, field);
}

void pfxvm_update_bounce(PfxBehavior* behavior, unsigned int field,
                         unsigned int velocity_field,
                         unsigned int bounce_count_field, float scale)
{
    PfxUpdateInstruction* instruction;
    vm_push_field(behavior, bounce_count_field);
    instruction = add_update_insn(behavior, 11, field);
    set_vm_field(&instruction->arguments.field, velocity_field);
    instruction->scalar_04 = scale;
}

void pfxvm_update_fade_alpha(PfxBehavior* behavior, unsigned int color_field,
                             unsigned int age_field, int start_alpha,
                             int end_alpha, float start_time, float duration)
{
    PfxUpdateInstruction* instruction =
        add_update_insn(behavior, 12, color_field);
    set_vm_field(&instruction->arguments.fade.age_field, age_field);
    instruction->arguments.fade.start_time = start_time;
    instruction->arguments.fade.duration = duration;
    instruction->arguments.fade.start_alpha = start_alpha;
    instruction->arguments.fade.end_alpha = end_alpha;
}

void pfxvm_update_lerp_color(PfxBehavior* behavior, unsigned int color_field,
                             unsigned int age_field, int color_count,
                             int first_color, void* colors, float duration)
{
    PfxUpdateInstruction* instruction =
        add_update_insn(behavior, 13, color_field);
    set_vm_field(&instruction->arguments.color_lerp.age_field, age_field);
    instruction->arguments.color_lerp.colors = colors;
    instruction->arguments.color_lerp.first_color = first_color;
    if (color_count > 0) {
        color_count--;
    }
    instruction->arguments.color_lerp.last_color = color_count;
    instruction->arguments.color_lerp.duration = duration;
}

void pfxvm_update_animate_texture(PfxBehavior* behavior,
                                  unsigned int texture_field,
                                  unsigned int age_field, int frame_count,
                                  int frame_offset, void* frame_source,
                                  int mode, float frame_time)
{
    PfxUpdateInstruction* instruction =
        add_update_insn(behavior, 14, texture_field);
    set_vm_field(&instruction->arguments.texture_anim.age_field, age_field);
    instruction->arguments.texture_anim.frame_time = frame_time;
    instruction->arguments.texture_anim.frame_count = (short)frame_count;
    instruction->arguments.texture_anim.mode = (short)frame_offset;
    instruction->arguments.texture_anim.first_frame = mode;
    instruction->arguments.texture_anim.frame_source = frame_source;
}

void pfxvm_update_attract(PfxBehavior* behavior, unsigned int field,
                          unsigned int target_field, float strength)
{
    PfxUpdateInstruction* instruction =
        add_update_insn(behavior, 15, field);
    set_vm_field(&instruction->arguments.field, target_field);
    instruction->scalar_04 = strength;
}

void pfxvm_update_assign(PfxBehavior* behavior, int destination, int source)
{
    PfxUpdateInstruction* instruction;
    if (destination == 0x100) {
        if (source == 0x203) {
            instruction = add_update_insn(behavior, 9, destination);
            set_vm_field(&instruction->arguments.field, source);
        }
    }
}

void pfxvm_update_roundrobin(PfxBehavior* behavior, unsigned int field)
{
    PfxUpdateInstruction* instruction =
        add_update_insn(behavior, 16, field);
    set_vm_field(&instruction->arguments.field, field);
}
