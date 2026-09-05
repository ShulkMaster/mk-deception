#include "libmkparticle/particle.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/config.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/metrics.h"
#include "libmkparticle/spawn.h"
#include "libmkparticle/texture_anim.h"
#include "libmkparticle/update.h"
#include "rw/rwengine.h"
#include "rw/rwcamera_internal.h"
#include "libmkparticle/streams.h"
#include "platform/fast_rw.h"
#include "runtime/cstring.h"

int printf(const char* format, ...);
double pow(double base, double exponent);

static const float s_zero = 0.0f;

static int crash_integer;
static int crash_pointer;
static int srcBlend;
static int dstBlend;
static int cullmode;
static int max_global_particle_count;
static int current_particle_count;
static int global_particle_count;
static union {
    signed char* pointer;
    int integer;
} null_value;

typedef struct PfxDiagnosticStrings {
    char invalid_struct_field[37];
    char warning_format[16];
    char halt_format[13];
    char section_padding[6]; /* Retail .data tail alignment. */
} PfxDiagnosticStrings;

static PfxDiagnosticStrings diagnostic_strings = {
    "Invalid field ID for get_struct_size",
    "PFX WARNING:%s\n",
    "PFX HALT:%s\n"
};

static void v3_x_mat_4(PfxVec3* out, PfxVec3* v, float* m);
/* Retail function order. */

int get_propfield_size(int type) {
    int index;

    index = 0;
    while (properties[index].flag != 0) {
        if (type == properties[index].description) {
            return get_size(properties[index].type);
        }
        index++;
    }
    return 0;
}

static int get_renderfield_size(int type) {
    int index;

    index = 0;
    while (render_fields[index].flag != 0) {
        if (type == render_fields[index].description) {
            return get_size(render_fields[index].type);
        }
        index++;
    }
    return 0;
}

int get_field_size(int type) {
    int size;

    size = get_propfield_size(type);
    if (size == 0) {
        size = get_renderfield_size(type);
    }
    return size;
}

int pfx_field_get_type(int field) {
    int index;

    index = 0;
    while (properties[index].flag != 0) {
        if (field == properties[index].description) {
            return properties[index].type;
        }
        index++;
    }
    index = 0;
    while (render_fields[index].flag != 0) {
        if (field == render_fields[index].description) {
            return render_fields[index].type;
        }
        index++;
    }
    index = 0;
    while (parametric_fields[index].type != 0) {
        if (field == parametric_fields[index].description) {
            return parametric_fields[index].type;
        }
        index++;
    }
    return field == 0x201;
}

void pfx_set_texture(PfxRenderView* pfx, RwTexture* texture) {
    pfx->texture = texture;
    if (pfx->texture == 0) {
        return;
    }
    pfx->has_texture = 1;
}

int pfx_frame_begin(PfxVm* pfx) {
    PfxVmEmitter* emitter;
    PfxRuntimeBuffer* buffer;
    PfxRuntimeBuffer* old_buffer;
    RwSphere sphere;
    int particle_stride;
    int render_stride;
    int index;

    pfx->previous_transform = pfx->active_transform;
    pfx->active_transform = (pfx->active_transform + 1) % 3;
    pfx->transforms[pfx->active_transform].live = 0;
    pfxmetrics_event(pfx->metrics, 0x1000);

    emitter = 0;
    if (pfx->emitter_count != 0) {
        emitter = pfx_get_emitter(pfx, 0);
    }
    if (emitter != 0) {
        if (emitter->pfx_transform != 0) {
            float* matrix;

            matrix = emitter->pfx_transform->elements;
            pfx->world_position.x =
                matrix[12] +
                (emitter->position.z * matrix[8] +
                 (emitter->position.x * matrix[0] +
                  emitter->position.y * matrix[4]));
            pfx->world_position.y =
                matrix[13] +
                (emitter->position.z * matrix[9] +
                 (emitter->position.x * matrix[1] +
                  emitter->position.y * matrix[5]));
            pfx->world_position.z =
                matrix[14] +
                (emitter->position.z * matrix[10] +
                 (emitter->position.x * matrix[2] +
                  emitter->position.y * matrix[6]));
        } else {
            pfx->world_position = emitter->position;
        }
    }

    pfx->frame_flags |= 0x20;
    if ((pfx->frame_flags & 0x80) != 0) {
        pfx->frame_flags &= ~0x20;
    }
    if ((pfx->frame_flags & 0x40) != 0) {
        sphere.center.x = pfx->world_position.x;
        sphere.center.y = pfx->world_position.y;
        sphere.center.z = pfx->world_position.z;
        sphere.radius = pfx->cull_radius;
        if (RwCameraFrustumTestSphere(pfxsystem_globals.camera, &sphere) == 0) {
            pfx->frame_flags &= ~0x20;
        }
    }

    if (pfx->typed_runtime_buffer_a->particle_stride == 0) {
        particle_stride = pfx_get_struct_size(pfx, 0x100);
        render_stride = pfx_get_struct_size(pfx, 0x300);
        pfx->typed_runtime_buffer_a->particle_stride = particle_stride;
        pfx->typed_runtime_buffer_b->particle_stride = particle_stride;
        pfx->typed_runtime_buffer_a->render_stride = render_stride;
        pfx->typed_runtime_buffer_b->render_stride = render_stride;
    }

    old_buffer = pfx->typed_runtime_buffer_a;
    pfx->typed_runtime_buffer_a = pfx->typed_runtime_buffer_b;
    pfx->typed_runtime_buffer_b = old_buffer;
    buffer = pfx->typed_runtime_buffer_b;
    buffer->render_data =
        streampool_lock(1, pfx->particle_capacity * buffer->render_stride);
    buffer->particle_data =
        streampool_lock(0, pfx->particle_capacity * buffer->particle_stride);

    if ((pfx->frame_flags & 8) != 0) {
        pfx->frame_flags &= ~8;
        pfx_frame_end(pfx);
        pfx->frame_flags |= 8;
    }

    if (buffer->particle_data == 0 || buffer->render_data == 0) {
        pfx->particle_cursor = 0;
        for (index = 0; index < pfx->behavior_count; index++) {
            pfx->behavior_list[index]->particle_count = 0;
        }
        return 1;
    }
    return 0;
}

void pfx_frame_end(PfxVm* pfx) {
    PfxRuntimeBuffer* buffer;
    int particle_bytes;
    int render_bytes;
    int count;

    pfx->transforms[pfx->active_transform].live = pfx->particle_cursor;
    if ((pfx->frame_flags & 8) != 0) {
        return;
    }
    buffer = pfx->typed_runtime_buffer_b;
    if ((pfx->frame_flags & 0x10) != 0) {
        count = pfx->particle_capacity;
        particle_bytes = count * buffer->particle_stride;
        render_bytes = count * buffer->render_stride;
    } else {
        count = pfx->particle_cursor;
        particle_bytes = count * buffer->particle_stride;
        render_bytes = count * buffer->render_stride;
    }
    if (buffer->particle_data != 0) {
        streampool_unlock(0, particle_bytes);
    }
    if (buffer->render_data != 0) {
        streampool_unlock(1, render_bytes);
    }
}

void pfx_frame_end_check(PfxVm* pfx) {
    (void)pfx;
}

void update_live_particles(PfxRuntimeView* pfx) {
    int live;
    int index;

    /* Soft ceiling: 98% -- retail loads live before the slot index. */
    live = pfx->live;
    index = pfx->active_slot;
    pfx->slots[index].live = live;
}

void* pfx_get_field(PfxVm* pfx, int index, unsigned int field) {
    PfxRuntimeBuffer* buffer;
    PfxFieldDescription* description;
    PfxParametricParticle* particle;
    int i;

    switch (field) {
    case 0x500:
        return &pfxsystem_globals.field_0x500;
    case 0x501:
        return &pfxsystem_globals.field_0x501;
    case 0x502:
        return &pfxsystem_globals.field_0x502;
    }
    if (pfx == 0) {
        return 0;
    }
    switch (field) {
    case 0x200:
        return &pfx->field_0x1E4;
    case 0x201:
        return &pfx->field_0x1FC;
    case 0x202:
        return pfx_get_emitter(pfx, 0);
    case 0x203:
        return &pfx->world_position;
    case 0x204:
        return &pfx->field_0x208;
    case 0x600:
        return pfx->particle_data;
    }

    if (pfx->field_0x22C != 0 && (field & 0xF00) == 0x300) {
        if (field != 0x300 || (pfx->flags_0x60 & 1) == 0) {
            return 0;
        }
        particle = (PfxParametricParticle*)(pfx->parametric + 1);
        return &particle->velocity;
    }
    if (pfx->field_0x22C != 0) {
        particle = (PfxParametricParticle*)(pfx->parametric + 1);
        switch (field) {
        case 0x400:
            return &particle->position;
        case 0x402:
            return &particle->texture;
        case 0x403:
            return &particle->size;
        }
    }

    if (pfx->typed_field_descriptions == 0) {
        return 0;
    }
    if (index == -1) {
        buffer = pfx->typed_runtime_buffer_a;
    } else if (index == -2) {
        buffer = pfx->typed_runtime_buffer_b;
    } else {
        return 0;
    }
    description = pfx->typed_field_descriptions;
    for (i = 0; i < pfx->field_count; i++, description++) {
        if (field == description->description) {
            unsigned char* data;

            data = ((PfxFieldBuffer*)buffer)[description->stream].data;
            if (data != 0) {
                data += description->offset;
            }
            if (description->stream == 2) {
                return 0;
            }
            return data;
        }
    }
    return 0;
}

static int pfx_memory_is_set(PfxVm* pfx) {
    return pfx->typed_runtime_buffer_a != 0;
}

void pfxvm_require_field(PfxVm* pfx, unsigned int field) {
    PfxFieldSet fields;
    int changed;

    fields.render_flags = pfx->flags_0x1D4;
    fields.particle_flags = pfx->flags_0x60;
    add_field(&fields.render_flags, field);
    if (field == 0x403) {
        pfx->flags151 |= 0x40;
    }
    if (field == 0x402) {
        pfx->flags151 |= 0x20;
    }
    changed = fields.render_flags != pfx->flags_0x1D4 ||
              fields.particle_flags != pfx->flags_0x60;
    if (!pfx_memory_is_set(pfx) || !changed) {
        pfx->flags_0x1D4 = fields.render_flags;
        pfx->flags_0x60 = fields.particle_flags;
    }
}

int pfx_get_struct_size(PfxVm* pfx, unsigned int field) {
    switch (field & 0xF00) {
    case 0x100:
        return pfx->transforms[0].particle_field_stride;
    case 0x300:
        if (pfx->field_0x22C != 0 && field == 0x300) {
            return sizeof(PfxParametricParticle);
        }
        return pfx->particle_vector_stride;
    case 0x400:
        return sizeof(PfxParametricParticle);
    case 0x200:
    case 0x500:
        return 0;
    case 0x600:
        return pfx->particle_user_data_size;
    default:
        pfx_halt(diagnostic_strings.invalid_struct_field);
        return 0;
    }
}

void pfx_halt(const char* message) {
    if (_pfx_config.halt != 0) {
        _pfx_config.halt(message);
    } else {
        printf(diagnostic_strings.halt_format, message);
    }
    crash_integer = *null_value.pointer;
    crash_pointer = 1 / null_value.integer;
}

void pfx_count_begin(void) {
    current_particle_count = 0;
}

void pfx_count_end(void) {
    global_particle_count = current_particle_count;
    if (current_particle_count > max_global_particle_count) {
        max_global_particle_count = current_particle_count;
    }
}

void pfx_count_add(PfxVm* pfx) {
    current_particle_count += pfx->transforms[pfx->active_transform].live;
}

static void v3_x_mat_4(PfxVec3* out, PfxVec3* v, float* m) {
    out->x = m[12] + (v->z * m[8] + (v->x * m[0] + v->y * m[4]));
    out->y = m[13] + (v->z * m[9] + (v->x * m[1] + v->y * m[5]));
    out->z = m[14] + (v->z * m[10] + (v->x * m[2] + v->y * m[6]));
}

void pfx_parametric_spawn(PfxVm* pfx, float frame_time) {
    PfxParametricState* state;
    PfxParametricParticle* particles;
    PfxParametricParticle* particle;
    PfxVmEmitter* emitter;
    PfxVec3 transformed;
    int old_count;
    int cursor;
    int capacity;
    int emitter_index;
    int birth_count;
    int birth;

    pfxmetrics_event(pfx->metrics, 0x1001);
    state = pfx->parametric;
    cursor = state->particle_cursor;
    capacity = state->particle_capacity;
    particles = (PfxParametricParticle*)(state + 1);
    particle = &particles[cursor];
    old_count = pfx->particle_cursor;

    for (emitter_index = 0; emitter_index < pfx->emitter_count;
         emitter_index++) {
        emitter = pfx_get_emitter(pfx, emitter_index);
        birth_count = _pfx_emitter_get_birthcount(emitter, pfx, frame_time);
        if (birth_count != 0) {
            for (birth = 0; birth < birth_count; birth++) {
                particle->birth_time = pfx->elapsed_time;
                particle->position = emitter->position;
                if (emitter->pfx_transform != 0) {
                    v3_x_mat_4(&transformed, &particle->position,
                               emitter->pfx_transform->elements);
                    particle->position = transformed;
                }
                pfx->particle_cursor = cursor;
                if ((emitter->flags.value & 0x40) != 0) {
                    __pfxvm_execute_spawn(pfx, emitter);
                }
                particle++;
                cursor++;
                if (cursor >= capacity) {
                    cursor = 0;
                    particle = particles;
                }
                pfx->particle_cursor = cursor;
            }
            state->particle_cursor = cursor;
            pfx->particle_cursor = old_count + birth_count;
            emitter->birth_count += birth_count;
        }
    }
    pfxmetrics_event(pfx->metrics, 0x2001);
}

void pfx_parametric_update(PfxVm* pfx, float frame_time) {
    PfxParametricState* state;
    PfxParametricParticle* particle;
    PfxVec3* position;
    float* texture;
    float* size;
    PfxColor* color;
    PfxTextureFrame* uv;
    float inverse_lifetime;
    float damping;
    float age;
    float normalized_age;
    float curve_position;
    float fraction;
    float inverse_fraction;
    float r0;
    float g0;
    float b0;
    float a0;
    float r1;
    float g1;
    float b1;
    float a1;
    int texture_curve;
    int size_curve;
    int color_curve;
    int direct_texture;
    int direct_size;
    int age_scaled_size;
    int has_damping;
    int texture_frames;
    int valid_count;
    int index;
    int curve_index;
    int stride;

    state = pfx->parametric;
    damping = 1.0f;
    inverse_lifetime = 1.0f / state->lifetime;
    position = pfx_get_field(pfx, -2, 0x100);
    texture = pfx_get_field(pfx, -2, 0x102);
    size = pfx_get_field(pfx, -2, 0x103);
    color = pfx_get_field(pfx, -2, 0x101);
    uv = pfx_get_field(pfx, -2, 0x104);

    if (pfx->elapsed_time == 0.0f || pfx->particle_cursor == 0) {
        return;
    }
    pfxmetrics_event(pfx->metrics, 0x1003);
    direct_size = (pfx->flags151 & 0x40) != 0;
    direct_texture = (pfx->flags151 & 0x20) != 0;
    if (direct_size && size == 0) {
        return;
    }
    if (direct_texture && texture == 0) {
        return;
    }

    texture_curve = pfx->flags_0x1D4 & 0x20;
    size_curve = pfx->flags_0x1D4 & 0x40;
    color_curve = pfx->flags_0x1D4 & 0x10;
    texture_frames = pfx->texture_frame_count;
    has_damping = state->damping != 0.0f;
    age_scaled_size = (pfx->flags151 & 0x10) != 0;
    if (age_scaled_size) {
        size_curve = 0;
    }

    valid_count = 0;
    stride = pfx->transforms[0].particle_field_stride;
    particle = (PfxParametricParticle*)(state + 1);
    for (index = 0; index < state->particle_capacity; index++, particle++) {
        age = pfx->elapsed_time - particle->birth_time;
        normalized_age = age * inverse_lifetime;
        if (age <= state->lifetime && particle->birth_time > 0.0f) {
            *position = particle->position;
            if (has_damping) {
                damping = (float)pow(state->damping, age);
            }
            position->x += damping * (particle->velocity.x * age);
            position->y += damping * (particle->velocity.y * age);
            position->z += damping * (particle->velocity.z * age);
            position->x += damping * (state->acceleration.x * age);
            position->y += damping * (state->acceleration.y * age);
            position->z += damping * (state->acceleration.z * age);
            position->y += damping *
                           (age * (state->vertical_acceleration * age));

            if (position->y > state->minimum_y) {
                valid_count++;
                if (texture_curve != 0 && !direct_texture) {
                    curve_position = normalized_age *
                                     (float)(state->texture_curve_count - 1);
                    curve_index = (int)curve_position;
                    if (curve_index >= state->texture_curve_count) {
                        curve_index = state->texture_curve_count - 1;
                    }
                    fraction = curve_position - (float)curve_index;
                    *texture = (1.0f - fraction) *
                                   state->texture_curve[curve_index] +
                               fraction * state->texture_curve[curve_index + 1];
                    *texture += state->texture_rate * age;
                }
                if (direct_texture) {
                    *texture = particle->texture + state->texture_rate * age;
                }
                if (size_curve != 0) {
                    curve_position = normalized_age *
                                     (float)(state->size_curve_count - 1);
                    curve_index = (int)curve_position;
                    if (curve_index >= state->size_curve_count) {
                        curve_index = state->size_curve_count - 1;
                    }
                    fraction = curve_position - (float)curve_index;
                    *size = state->size_curve[curve_index] * (1.0f - fraction) +
                            state->size_curve[curve_index + 1] * fraction;
                }
                if (direct_size) {
                    *size = particle->size;
                }
                if (age_scaled_size) {
                    *size = age * particle->size;
                }
                if (color_curve != 0) {
                    curve_position = normalized_age *
                                     (float)(state->color_curve_count - 1);
                    curve_index = (int)curve_position;
                    if (curve_index >= state->color_curve_count) {
                        curve_index = state->color_curve_count - 1;
                    }
                    fraction = curve_position - (float)curve_index;
                    pfx_native_get_rgba(&state->color_curve[curve_index],
                                        &r0, &g0, &b0, &a0);
                    pfx_native_get_rgba(&state->color_curve[curve_index + 1],
                                        &r1, &g1, &b1, &a1);
                    inverse_fraction = 1.0f - fraction;
                    color->r = (unsigned char)(inverse_fraction * r0 +
                                               fraction * r1);
                    color->g = (unsigned char)(inverse_fraction * g0 +
                                               fraction * g1);
                    color->b = (unsigned char)(inverse_fraction * b0 +
                                               fraction * b1);
                    color->a = (unsigned char)(inverse_fraction * a0 +
                                               fraction * a1);
                }
                if (texture_frames != 0) {
                    int frame;

                    frame = pfx_texture_getframe(
                        (const PfxTextureAnim*)&pfx->texture_frame_count, age);
                    *uv = pfx->texture_frames[frame];
                }
                position = (PfxVec3*)((unsigned char*)position + stride);
                texture = (float*)((unsigned char*)texture + stride);
                size = (float*)((unsigned char*)size + stride);
                color = (PfxColor*)((unsigned char*)color + stride);
                uv = (PfxTextureFrame*)((unsigned char*)uv + stride);
            }
        }
    }
    pfx->particle_cursor = valid_count;
    pfxmetrics_event(pfx->metrics, 0x2003);
}

void pfx_run(PfxVm* pfx, float frame_time) {
    PfxBehavior* behavior;
    PfxBehavior* next_behavior;
    int total_particles;
    int index;

    pfx->elapsed_time += frame_time;
    if (pfx_frame_begin(pfx) != 0) {
        g_current_effect = 0;
        pfx->particle_cursor = 0;
        pfx_frame_end(pfx);
        return;
    }
    pfx_behaviors_frame_begin(pfx);
    for (index = 0; index < pfx->emitter_count; index++) {
        pfx_emitter_run_frame(pfx, index, frame_time);
    }

    total_particles = 0;
    pfxmetrics_event(pfx->metrics, 0x1003);
    g_current_effect = pfx;
    for (index = 0; index < pfx->behavior_count; index++) {
        behavior = pfx_behavior(pfx, index);
        if (index + 1 < pfx->behavior_count) {
            next_behavior = pfx_behavior(pfx, index + 1);
        } else {
            next_behavior = 0;
        }
        _pfxvm_execute_initial_behavior(behavior, frame_time);
        pfxvm_execute_behavior_update(behavior, frame_time);
        behavior->particle_count += behavior->active_particle_count;
        behavior->active_particle_count = 0;
        pfxvm_execute_behavior_kill(behavior);
        if (next_behavior != 0) {
            behavior_adjust_streams(behavior, next_behavior);
        }
        total_particles += behavior->particle_count;
    }
    g_current_effect = 0;
    pfx->particle_cursor = total_particles;
    pfxmetrics_event(pfx->metrics, 0x2003);
    pfx_behaviors_frame_end(pfx);
    pfx_frame_end(pfx);
}

void pfxsystem_frame_begin(void) {
    pfxmetrics_begin_frame();
    streampool_nextframe();
}

void pfxsystem_skip_render_frame(void) {
    streampool_skiprenderstream();
}

void pfxsystem_set_frame_info(int unused0, int unused1, const float* matrix,
                              RwCamera* camera) {
    int i;
    float* row;

    (void)unused0;
    (void)unused1;
    memcpy(pfxsystem_globals.camera_facing, matrix, 0x40);
    for (i = 0; i < 4; i++) {
        row = &pfxsystem_globals.camera_facing[i * 4];
        /* Retail zeros translation column slots at +0x8C stride 0x10. */
        row[3] = s_zero;
    }
    /* camera_facing is at +0x80; +0x8C is element [0][3] of first row --- retail
     * writes four floats at +0x8C,+0x9C,+0xAC,+0xBC (column 3 of each row). */
    pfxsystem_globals.camera = camera;
}

void pfxsystem_widescreen_offset(int x, int y) {
    pfxsystem_globals.widescreen_x = x;
    pfxsystem_globals.widescreen_y = y;
}

void get_pfxsystem_widescreen_offset(int* out_x, int* out_y) {
    *out_x = pfxsystem_globals.widescreen_x;
    *out_y = pfxsystem_globals.widescreen_y;
}

void pfxsystem_init(void) {
    streampool_init();
}

void pfxsystem_set_global(int id, float value) {
    float* slot;

    if ((id & 0xF00) == 0x500) {
        slot = (float*)pfx_get_field(0, -2, id);
        if (slot != 0) {
            *slot = value;
        }
    }
}

void pfx_set_renderstate(PfxRenderView* pfx) {
    unsigned char flags;

    flags = pfx->flags;
    if (((flags >> 3) & 1) != 0 && ((flags >> 2) & 1) != 0) {
        pfx->render_x = pfx->source_x;
        pfx->render_y = pfx->source_y;
        pfx->render_z = pfx->source_z;
    }

    RwEngineInstance->dOpenDevice.fpRenderStateGet(0x14, &cullmode);
    RwRenderStateSet_rwRENDERSTATECULLMODE(1);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(0xA, &srcBlend);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(0xB, &dstBlend);

    if (pfx->blend_mode == 1) {
        RwRenderStateSet_SRCBLEND_DESTBLEND(5, 2);
    } else {
        RwRenderStateSet_SRCBLEND_DESTBLEND(5, 6);
    }
}

void pfx_reset_renderstate(void) {
    RwRenderStateSet_SRCBLEND_DESTBLEND(srcBlend, dstBlend);
    RwRenderStateSet_rwRENDERSTATECULLMODE(cullmode);
}

void pfx_render_set_blendmode(PfxRenderView* pfx, int mode) {
    pfx->blend_mode = mode;
}

PfxVmEmitter* pfx_get_emitter(PfxVm* pfx, int index) {
    if (index < 0 || index >= pfx->emitter_count) {
        return 0;
    }
    return &pfx->emitters[index];
}

int pfx_verify(PfxVerifyView* pfx) {
    unsigned int flags;
    unsigned char b;

    flags = pfx->flags;
    if ((flags & 0x100) != 0 && (flags & 0x200) != 0) {
        return 0;
    }
    b = pfx->byte_flags;
    if (((b >> 5) & 1) == 0) {
        return 0;
    }
    return 1;
}
int pfx_get_struct_size(PfxVm* pfx, unsigned int field);
