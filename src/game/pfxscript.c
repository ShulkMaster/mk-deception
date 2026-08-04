#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "game/bgnd.h"
#include "math/gxVect.h"

typedef struct PfxScriptEnvironment {
    int active;
    int field04;
    MkPfx* source_effect; /* +0x08 */
    struct PfxScriptEffect* effect; /* +0x0C */
    struct PfxScriptEmitter* emitter; /* +0x10 */
    int fields14[3];
    char* texture_name; /* +0x20 */
    unsigned int initialization_script; /* +0x24 */
    float drag_coefficient;   /* +0x28 */
    float growth_coefficient; /* +0x2C */
    int fields30[2];
    unsigned int kill_percent_field; /* +0x38 */
    int field3C;
} PfxScriptEnvironment;

typedef struct PfxScriptEffectFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char particle_size_enabled : 1; /* bit6 */
    unsigned char pad_bits5_0 : 6;
} PfxScriptEffectFlagBits;

typedef union PfxScriptEffectFlags {
    unsigned char raw;
    PfxScriptEffectFlagBits bits;
} PfxScriptEffectFlags;

typedef struct PfxRenderFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char custom_bounding_radius : 1; /* bit6 */
    unsigned char pad_bits5_0 : 6;
} PfxRenderFlagBits;

typedef union PfxRenderFlags {
    unsigned char raw;
    PfxRenderFlagBits bits;
} PfxRenderFlags;

typedef struct PfxOrientationFlagBits {
    unsigned char face_y : 1; /* bit7 */
    unsigned char pad_bits6_0 : 7;
} PfxOrientationFlagBits;

typedef union PfxOrientationFlags {
    unsigned char raw;
    PfxOrientationFlagBits bits;
} PfxOrientationFlags;

typedef struct PfxLifecycleFlagBits {
    unsigned char pad_bits7_5 : 3;
    unsigned char restart_cycle : 1; /* bit4 */
    unsigned char pad_bits3_0 : 4;
} PfxLifecycleFlagBits;

typedef union PfxLifecycleFlags {
    unsigned char raw;
    PfxLifecycleFlagBits bits;
} PfxLifecycleFlags;

typedef struct PfxHideFlagBits {
    unsigned char hidden : 1; /* bit7 */
    unsigned char pad : 7;
} PfxHideFlagBits;

typedef union PfxHideFlags {
    unsigned char raw;
    PfxHideFlagBits bits;
} PfxHideFlags;

typedef struct PfxZTestFlagBits {
    unsigned char pad : 7;
    unsigned char disabled : 1; /* bit0 */
} PfxZTestFlagBits;

typedef union PfxZTestFlags {
    unsigned char raw;
    PfxZTestFlagBits bits;
} PfxZTestFlags;

typedef struct PfxScriptEffect {
    MkHdr hdr; /* +0x00 */
    PfxLifecycleFlags lifecycle_flags; /* +0x08 */
    char pad09[0x23];
    int render_priority; /* +0x2C */
    char pad30[0x10];
    union {
        PfxRenderFlags render_flags; /* +0x40 */
        struct {
            char pad40[0x40];
            PfxHideFlags hide_flags; /* +0x80 */
            char pad81[0xCF];
        };
        unsigned char emitters[0x110]; /* +0x40 */
    };
    PfxScriptEffectFlags flags;
    PfxOrientationFlags orientation_flags; /* +0x151 */
    char pad152[0x1E];
    float z_bias; /* +0x170 */
    struct PfxTextureSlot* texture_slot; /* +0x174 */
    char pad178[0x0A];
    short texture_animation_enabled; /* +0x182 */
    char pad184[0x0C];
    PfxZTestFlags ztest_flags; /* +0x190 */
    char pad191[3];
    float decal_plane[6]; /* +0x194 */
    float aspect_x; /* +0x1AC */
    float aspect_y; /* +0x1B0 */
    char pad1B4[4];
    float particle_size; /* +0x1B8 */
    float bounding_radius; /* +0x1BC */
    char pad1C0[0x14];
    unsigned int runtime_flags; /* +0x1D4 */
    char pad1D8[0x2C];
    struct PfxScriptEmitter* emitter; /* +0x204 */
    char pad208[0x20];
    float kill_plane; /* +0x228 */
    int initialization_mode; /* +0x22C */
    char pad230[0x30];
    int effect_id; /* +0x260 */
} PfxScriptEffect;

typedef struct PfxTextureInfo {
    char pad00[0x0C];
    int width; /* +0x0C */
    int height; /* +0x10 */
} PfxTextureInfo;

typedef struct PfxTextureSlot {
    PfxTextureInfo* info;
} PfxTextureSlot;

typedef struct PfxEmissionFlagBits {
    unsigned char cycle_paused : 1; /* bit7 */
    unsigned char pad_bits6_5 : 2;
    unsigned char constant_rate : 1; /* bit4 */
    unsigned char pad_bits3_0 : 4;
} PfxEmissionFlagBits;

typedef union PfxEmissionFlags {
    unsigned char raw;
    PfxEmissionFlagBits bits;
} PfxEmissionFlags;

typedef struct PfxScriptEmitter {
    Vec emission_point; /* +0x00 */
    float birthrate; /* +0x0C */
    char pad10[0x0C];
    PfxEmissionFlags emission_flags; /* +0x1C */
    char pad1D[3];
    float cycle_length;   /* +0x20 */
    float cycle_position; /* +0x24 */
    int cycle_emission;    /* +0x28 */
    int cycle_enabled;     /* +0x2C */
    char pad30[0x0C];
    int cycle_frame;       /* +0x3C */
} PfxScriptEmitter;

/*
 * Local fx_next_emitter view. The runtime emitter stride and these two effect
 * fields are verified here, but the rest of either runtime type is not.
 */
typedef struct PfxRuntimeEmitterView {
    unsigned char data[0x2EC];
} PfxRuntimeEmitterView;

typedef struct PfxEmitterEffectView {
    unsigned char pad00[0x200];
    int emitter_count; /* +0x200 */
    PfxRuntimeEmitterView* emitters; /* +0x204 */
} PfxEmitterEffectView;

typedef void (*PfxBankDestroyFn)(MkHdr* bank);

typedef struct PfxBankVtablePrefix {
    void* reserved[4];
    PfxBankDestroyFn destroy;
} PfxBankVtablePrefix;

typedef struct PfxBank PfxBank;

typedef struct PfxBankLatch {
    PfxBank* bank;
    unsigned int bank_instance;
} PfxBankLatch;

typedef struct PfxEffectLatch {
    PfxScriptEffect* effect;
    unsigned int effect_instance;
} PfxEffectLatch;

typedef union PfxBankVtableRef {
    MkVtable5* base;
    PfxBankVtablePrefix* bank;
} PfxBankVtableRef;

typedef struct PfxResolvedHandle {
    PfxBank* bank;
    int effect_index;
    PfxScriptEffect* effect;
} PfxResolvedHandle;

struct PfxBank {
    MkHdr hdr;
    unsigned int handle_bank; /* +0x08 */
    unsigned int handle_generation; /* +0x0C */
    char pad10[4];
    unsigned int owner_flags; /* +0x14 */
    int effect_count; /* +0x18 */
    char pad1C[4];
    PfxEffectLatch* effects; /* +0x20 */
    unsigned int* effect_owners; /* +0x24 */
};

typedef struct PfxFloatRange {
    float minimum;
    float maximum;
} PfxFloatRange;

typedef struct PfxParticleResetRecord {
    unsigned char data[0x28];
} PfxParticleResetRecord;

typedef struct PfxParticleResetStorage {
    char pad00[0x348];
    int particle_count; /* +0x348 */
    char pad34C[0x0C];
    PfxParticleResetRecord particles[1]; /* +0x358 */
} PfxParticleResetStorage;

/*
 * Reset state embedded at PfxScriptEffect::emitters (+0x40). Keeping this as
 * a sub-structure makes the retail-relative offsets explicit without open
 * pointer arithmetic.
 */
typedef struct PfxResetRuntimeView {
    char pad00[0x4C];
    float reset_time; /* +0x4C (effect +0x8C) */
    char pad50[4];
    int reset_active; /* +0x54 (effect +0x94) */
    char pad58[0xF0];
    PfxParticleResetStorage* particle_storage; /* +0x148 (effect +0x188) */
    char pad14C[0x74];
    int emitter_count; /* +0x1C0 (effect +0x200) */
    char pad1C4[8];
    int reset_field_count; /* +0x1CC (effect +0x20C) */
    int** reset_fields; /* +0x1D0 (effect +0x210) */
} PfxResetRuntimeView;

typedef struct PfxMirrorRenderObject {
    MkHdr hdr; /* +0x00 */
    unsigned char flags; /* +0x08 */
    char pad09[0xE7];
    Vec scale; /* +0xF0 */
} PfxMirrorRenderObject;

typedef struct PfxRuntimeEffect {
    char pad00[0x40];
    PfxResetRuntimeView reset; /* +0x40 */
} PfxRuntimeEffect;

static PfxScriptEnvironment pfxscript_environment = {
    0,
    0,
    0,
    0,
    0,
    { 0, 0, 0 },
    0,
    0,
    0.0f,
    0.0f,
    { 0, 0 },
    0,
    0,
};
static int g_profile_enabled;
static float parametric_birthrate = 1.0f;

extern PfxBankLatch banks[15];
extern unsigned int cached_handle;
extern ScriptSlot* g_pfx_cmo;
void bank_run_fx(MkHdr* bank);

void* pfx_get_field(void* effect, int emitter, int field);
void* memset(void* destination, int value, unsigned long size);
unsigned int banks_find_owned_fx(const char* name, unsigned int owner);
void pfxvm_kill_percent(unsigned int field);
void build_step_effect(
    ScriptSlot* script, unsigned int effect, int update, CmdScript* context);
void build_parametric_effect_from_table(
    ScriptSlot* script, unsigned int effect, int update, CmdScript* context);
void load_effect_bank_with_context(void);
int resolve_pfx_handle(unsigned int handle, PfxResolvedHandle* resolved);
void fx_reset_emit(unsigned int effect);
void pfxvm_initial_reflect(unsigned int context, int field);
void pfxvm_initial_add_v3(
    unsigned int context, int destination, int source);
void pfxvm_initial_divert(PfxFloatRange* range);
void pfxvm_initial_multiply_float_range(PfxFloatRange* range);
void pfxvm_initial_set_float_range(PfxFloatRange* range);
void pfxvm_kill_roundrobin(unsigned int context, int field);
void pfxvm_kill_on_greater(unsigned int context, int field);
void pfxvm_update_roundrobin(unsigned int context, int field);
void pfxvm_update_assign(
    unsigned int context, int destination, int source);
void pfxvm_update_wrapbox(unsigned int context, int field);
void pfxvm_update_mul_scalar(unsigned int context, int field);
void pfxvm_update_copy(unsigned int context, int field);
void pfxvm_update_add_constant(unsigned int context, int field);
void pfxvm_update_add_constant_v3(unsigned int context, int field);
void pfxvm_update_add(unsigned int context, int destination, int source);
void pfxvm_change_on_less(void);
void pfxvm_change_on_greater(void);
void pfxvm_change_on_y_less(void);
void pfxvm_change_on_y_less_than_field(int field, int source);
void pfxvm_spawn_sphere_section(PfxScriptEmitter* emitter, int field);
void pfxvm_spawn_uv(PfxScriptEmitter* emitter, int field);
void pfxvm_spawn_value(PfxScriptEmitter* emitter, int field);
void pfxvm_spawn_line_1i(
    PfxScriptEmitter* emitter, int field, int minimum, int maximum);
void pfxvm_spawn_line_1f(float minimum, float maximum);
void pfxvm_spawn_box(float x, ...);
void pfxvm_spawn_cylinder(
    Vec* origin, float radius, float height, float start, float end);
void pfxvm_spawn_disc(Vec* origin, float inner_radius, float outer_radius);
void pfxvm_spawn_point_color(
    int field, float red, float green, float blue, float alpha);
void pfxvm_spawn_from_pos(int field, int source, int clamp_y, float offset);
void pfxvm_spawn_roundrobin_mechanism(int field, int source);
void pfxvm_spawn_sphere(int field, ...);
void pfxvm_kill_on_y_less_than_field(int context, int field);
void pfxvm_update_attract(int context, int field);
void pfxvm_update_bounce(int context, int field, int source);
void pfxvm_update_fade_alpha(
    int context, int field, int start_alpha, int end_alpha);
void pfxvm_update_animate_texture(
    int context, int field, int first, int last, int loop, int advance);
void pfx_emitter_restart_cycle(
    PfxScriptEmitter* emitter, int cycle, PfxScriptEmitter* source);
PfxScriptEmitter* pfx_get_emitter(void* emitters, int index);
PfxScriptEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(PfxScriptEffect* effect);
int pfx_emitter_exhausted(PfxRuntimeEmitterView* emitter);

static PfxScriptEffect* resolve_effect_handle(unsigned int handle) {
    PfxResolvedHandle resolved;
    unsigned int kind = (handle >> 14) & 3;

    if (kind != 1 && kind != 2) {
        return 0;
    }
    resolve_pfx_handle(
        (handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return resolved.effect;
}

void fx_set_param_v3(
    unsigned int handle, int parameter, float x, float y, float z) {
    PfxScriptEffect* effect;
    Vec* target;

    target = 0;
    if ((parameter & 0xF00) != 0x200) {
        return;
    }

    effect = resolve_effect_handle(handle);
    if (effect == 0) {
        return;
    }

    if (parameter == 0x202) {
        PfxEmitterEffectView* emitter_effect;
        unsigned int emitter_index;

        effect = resolve_effect_handle(handle);
        if (effect != 0) {
            emitter_effect = (PfxEmitterEffectView*)effect;
            emitter_index = (handle >> 16) & 0xF;
            if (emitter_index <
                (unsigned int)emitter_effect->emitter_count) {
                target = (Vec*)&emitter_effect
                    ->emitters[emitter_index];
            }
        }
    } else {
        target = (Vec*)pfx_get_field(
            effect->emitters, -2, parameter);
    }

    if (target != 0) {
        target->x = x;
        target->y = y;
        target->z = z;
    }
}
int pfx_emitter_unused(PfxRuntimeEmitterView* emitter);
void pfx_emitter_reset(PfxRuntimeEmitterView* emitter);
float p_update_effects(void);
void pfx_texture_animate();

static PfxScriptEnvironment* active_pfx_environment(void) {
    if (pfxscript_environment.active != 0) {
        return &pfxscript_environment;
    }
    return 0;
}

/* Soft ceiling: typed bank ownership transfer recovered. */
void fx_transfer(unsigned int handle, unsigned int owner) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.bank->effect_owners[resolved.effect_index] = owner;
    }
}

void fxsys_set(int field, float value) {
    float* destination;

    destination = pfx_get_field(0, 0, field);
    if (destination != 0) {
        *destination = value;
    }
}

void fxsys_set_v3(
    int field, float x, float y, float z, int unused) {
    Vec* destination;

    destination = pfx_get_field(0, 0, field);
    if (destination != 0) {
        destination->x = x;
        destination->y = y;
        destination->z = z;
    }
}

int emitter_id_from_handle(unsigned int handle) {
    return (handle >> 16) & 0xF;
}

void* pfx_from_handle(unsigned int handle) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return resolved.effect;
}

/* Soft ceiling: verified local effect/emitter view and handle encoding. */
unsigned int fx_next_emitter(unsigned int handle) {
    PfxResolvedHandle resolved;
    PfxEmitterEffectView* effect;
    int type;
    int emitter_index;

    type = (handle >> 14) & 3;
    if (type == 1 || type == 2) {
        resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
        effect = (PfxEmitterEffectView*)resolved.effect;
    } else {
        effect = 0;
    }

    if (effect == 0) {
        return 0;
    }

    for (emitter_index = 0;
         emitter_index < effect->emitter_count;
         emitter_index++) {
        if (pfx_emitter_exhausted(&effect->emitters[emitter_index]) ||
            pfx_emitter_unused(&effect->emitters[emitter_index])) {
            pfx_emitter_reset(&effect->emitters[emitter_index]);
            handle = (handle & 0xFFFF3FFF) | 0x8000;
            handle &= 0xFFF0FFFF;
            handle |= (emitter_index & 0xF) << 16;
            return handle;
        }
    }
    return 0;
}

void* pfx_from_emitter(unsigned int handle) {
    PfxResolvedHandle resolved;
    int type;

    type = (handle >> 14) & 3;
    if (type != 1 && type != 2) {
        return 0;
    }
    resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return resolved.effect;
}

/*
 * Soft ceiling: fx_by_id ~78.45% - retail keeps the effect index in r31 and
 * coalesces the two live-latch copies; the recovered search and handle bits
 * are otherwise exact.
 */
unsigned int fx_by_id(int effect_id, unsigned int owner) {
    PfxBankLatch* bank_latch;
    PfxEffectLatch* effect_latch;
    PfxBank* raw_bank;
    PfxBank* bank;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    int bank_index;
    int effect_index;

    for (bank_index = 0; bank_index < 15; bank_index++) {
        bank_latch = &banks[bank_index];
        raw_bank = bank_latch->bank;
        if (raw_bank != 0) {
            if (raw_bank->hdr.instance == bank_latch->bank_instance) {
                bank = raw_bank;
            } else {
                bank = 0;
            }
        } else {
            bank = 0;
        }

        if (bank != 0 && (bank->owner_flags & owner) != 0) {
            for (effect_index = 0;
                 effect_index < bank->effect_count;
                 effect_index++) {
                effect_latch = &bank->effects[effect_index];
                raw_effect = effect_latch->effect;
                if (raw_effect != 0) {
                    if (raw_effect->hdr.instance ==
                        effect_latch->effect_instance) {
                        effect = raw_effect;
                    } else {
                        effect = 0;
                    }
                } else {
                    effect = 0;
                }

                if (effect != 0 && effect->effect_id == effect_id) {
                    return (bank->handle_bank & 0xF) |
                        ((effect_index & 0x3FF) << 4) |
                        0x4000 |
                        (bank->handle_generation << 24);
                }
            }
        }
    }
    return 0;
}

unsigned int fx_by_owner(const char* name, unsigned int owner) {
    return banks_find_owned_fx(name, owner);
}

unsigned int fx(const char* name) {
    unsigned int owner;

    if (active_cmdscript != 0 &&
        active_cmdscript->mko != 0 &&
        active_cmdscript->mko->load_ctx != 0) {
        switch (active_cmdscript->mko->load_ctx->art_id) {
        case 0x3000B:
            owner = 1;
            break;
        case 0x4000B:
            owner = 2;
            break;
        case 0x2001E:
            owner = 4;
            break;
        case 0x8003D:
            owner = 4;
            break;
        case 0xD003C:
            owner = 4;
            break;
        case 0x70038:
            owner = 4;
            break;
        case 0x140064:
            owner = 4;
            break;
        default:
            owner = 0;
            break;
        }
    } else if (aproc == 0) {
        owner = 0;
    } else {
        switch (aproc->pid) {
        case 0x100A:
        case 0x1001:
            owner = 1;
            break;
        case 0x100B:
        case 0x1002:
            owner = 2;
            break;
        default:
            owner = 4;
            break;
        }
    }
    return banks_find_owned_fx(name, owner);
}

/* Soft ceiling: typed effect visibility flag update recovered. */
void fx_hide(unsigned int handle, int hidden) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->hide_flags.bits.hidden = hidden;
    }
}

/* Soft ceiling: scalar particle parameter update recovered. */
void fx_set(unsigned int handle, int field, float value) {
    PfxResolvedHandle resolved;
    float* destination;
    void* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = resolved.effect->emitters;

        if ((field & 0xF00) == 0x200) {
            destination = pfx_get_field(emitters, -2, field);
            if (destination != 0) {
                *destination = value;
            }
        }
    }
}

/* Soft ceiling: vector particle parameter read recovered. */
void fx_get_v3(unsigned int handle, int field, Vec* value) {
    PfxResolvedHandle resolved;
    Vec* source;
    void* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = resolved.effect->emitters;

        if ((field & 0xF00) == 0x200) {
            source = pfx_get_field(emitters, -2, field);
            if (source != 0) {
                value->x = source->x;
                value->y = source->y;
                value->z = source->z;
            }
        }
    }
}

/* Soft ceiling: typed depth-test flag update recovered. */
void fx_disable_ztest(unsigned int handle, int disabled) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->ztest_flags.bits.disabled = disabled;
    }
}

/* Soft ceiling: typed render-priority update recovered. */
void fx_set_render_priority(unsigned int handle, int priority) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->render_priority = priority;
    }
}

/* Soft ceiling: create_y_mirror_effect -- typed clone/render views recovered. */
void create_y_mirror_effect(int field_28) {
    PfxScriptEnvironment* environment;
    PfxClone* clone;
    PfxMirrorRenderObject* render_object;

    environment = active_pfx_environment();
    if (environment->effect != 0) {
        environment = active_pfx_environment();
        clone = pfx_create_clone(environment->source_effect);
        render_object = (PfxMirrorRenderObject*)
            pfx_clone_bind_render_to_new_obj(clone, (void*)0xFF00);
        clone->field_28 = field_28;
        render_object->scale.x = 1.0f;
        render_object->scale.y = -1.0f;
        render_object->scale.z = 1.0f;
        render_object->flags |= 2;
        update_mkobj(render_object != 0 ? as_mkhdr(&render_object->hdr) : 0);
    }
}

void z_bias(float bias) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0) {
        effect->z_bias = bias;
    }
}

void face_y(void) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0) {
        effect->orientation_flags.bits.face_y = 1;
    }
}

void set_bounding_radius(float radius) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0) {
        effect->bounding_radius = radius;
        effect->render_flags.bits.custom_bounding_radius = 1;
    }
}

void particle_size(float size) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0 &&
        effect->flags.bits.particle_size_enabled != 0) {
        effect->particle_size = size;
    }
}

void set_aspect_ratio(float x, float y) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0) {
        effect->aspect_x = x;
        effect->aspect_y = y;
    }
}

void enable_profiling(int enabled) {
    g_profile_enabled = enabled;
}

void initial_add_v3(int destination, int source) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_initial_add_v3(
            environment->kill_percent_field, destination, source);
    }
}

void initial_reflect(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_initial_reflect(environment->kill_percent_field, field);
    }
}

void kill_roundrobin(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_kill_roundrobin(environment->kill_percent_field, field);
    }
}

void kill_percent(float percent) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_kill_percent(environment->kill_percent_field);
    }
}

void kill_on_greater(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_kill_on_greater(environment->kill_percent_field, field);
    }
}

void udpate_roundrobin(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_roundrobin(environment->kill_percent_field, field);
    }
}

void update_assign(int destination, int source) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_assign(
            environment->kill_percent_field, destination, source);
    }
}

void update_wrapbox(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_wrapbox(environment->kill_percent_field, field);
    }
}

void update_mul_scalar(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_mul_scalar(environment->kill_percent_field, field);
    }
}

void update_copy(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_copy(environment->kill_percent_field, field);
    }
}

void update_add_constant(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_add_constant(environment->kill_percent_field, field);
    }
}

void update_add_constant_v3(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_add_constant_v3(environment->kill_percent_field, field);
    }
}

void create_step_effect(unsigned int effect) {
    if (effect != 0U) {
        build_step_effect(active_cmdscript->mko, effect, 1, active_cmdscript);
    }
}

void create_step_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_step_effect(active_cmdscript->mko, (unsigned int)effect, 1,
                          active_cmdscript);
        *effect = saved;
    }
}

void create_multiemit_step_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_step_effect(active_cmdscript->mko, (unsigned int)effect, 0,
                          active_cmdscript);
        *effect = saved;
    }
}

void create_parametric_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_parametric_effect_from_table(
            active_cmdscript->mko, (unsigned int)effect, 1,
            active_cmdscript);
        *effect = saved;
    }
}

void create_multiemit_parametric_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_parametric_effect_from_table(
            active_cmdscript->mko, (unsigned int)effect, 0,
            active_cmdscript);
        *effect = saved;
    }
}

void change_on_less(int unused) {
    PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment != 0 && environment->kill_percent_field != 0 &&
        environment->field3C != 0) {
        pfxvm_change_on_less();
    }
}

void change_on_greater(int unused) {
    PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment != 0 && environment->kill_percent_field != 0 &&
        environment->field3C != 0) {
        pfxvm_change_on_greater();
    }
}

void change_on_y_less(int unused) {
    PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment != 0 && environment->kill_percent_field != 0 &&
        environment->field3C != 0) {
        pfxvm_change_on_y_less();
    }
}

void change_on_y_less_than_field(int field, int source) {
    PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment != 0 && environment->kill_percent_field != 0 &&
        environment->field3C != 0) {
        pfxvm_change_on_y_less_than_field(field, source);
    }
}

void emit_cuboid(int unused, float x, float y, float z) {
    PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment != 0 && environment->emitter != 0) {
        pfxvm_spawn_box(-x * 0.5f, -y * 0.5f, -z * 0.5f,
                        x, y, z);
    }
}

void update_add(int destination, int source) {
    PfxScriptEnvironment* environment;
    unsigned int context;

    environment = active_pfx_environment();
    if (environment != 0) {
        context = environment->kill_percent_field;
        if (context != 0) {
            pfxvm_update_add(context, destination, source);
            if ((source & 0xF00) != 0x200) {
                pfxvm_update_copy(context, source);
            }
        }
    }
}

void set_rotation(float angle, float variance) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = active_pfx_environment();
    if (environment != 0) {
        effect = environment->effect;
        if (effect != 0) {
            pfx_get_emitter(effect, 0);
            pfxvm_spawn_line_1f(angle - variance, angle + variance);
            effect->orientation_flags.raw |= 0x10;
        }
    }
}

/* Soft ceiling: texture_animation_with_vsize -- typed texture metadata/layout. */
void texture_animation_with_vsize(
    int vertical_frames, float horizontal_scale, float vertical_scale,
    float speed) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;
    PfxTextureInfo* texture;
    float width;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture_slot != 0) {
            texture = effect->texture_slot->info;
            width = (float)texture->width;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                effect, (int)width, (int)(horizontal_scale * width),
                (int)(vertical_scale * (float)texture->height),
                vertical_frames, speed);
            effect->texture_animation_enabled = 1;
        }
    }
}

/* Soft ceiling: texture_animation -- typed texture metadata/layout. */
void texture_animation(int vertical_frames, float horizontal_scale, float speed) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;
    PfxTextureInfo* texture;
    float width;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture_slot != 0) {
            texture = effect->texture_slot->info;
            width = (float)texture->width;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                effect, (int)width, (int)(horizontal_scale * width),
                (int)((float)texture->height /
                      (horizontal_scale * (float)vertical_frames)),
                speed);
            effect->texture_animation_enabled = 1;
        }
    }
}

void restart_effect(const char* name) {
    PfxScriptEffect* effect;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        restart_effect_ppfx(effect);
    }
}

void set_decal_plane(const float* plane) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;
    int index;

    environment = active_pfx_environment();
    if (environment != 0) {
        effect = environment->effect;
        if (effect != 0) {
            effect->flags.raw |= 0x10;
            for (index = 0; index < 6; index++) {
                effect->decal_plane[index] = plane[index];
            }
        }
    }
}

void restart_effect_ppfx(PfxScriptEffect* effect) {
    PfxScriptEmitter* emitter;

    emitter = effect->emitter;
    effect->lifecycle_flags.bits.restart_cycle = 1;
    emitter->emission_flags.bits.cycle_paused = 0;
    emitter->cycle_frame = 0;
    pfx_emitter_restart_cycle(emitter, 0, emitter);
}

/* Soft ceiling: reset_effect ~75.68% - split saves and load scheduling only. */
void reset_effect(const char* name) {
    PfxScriptEffect* effect;
    int emitter_index;
    PfxResetRuntimeView* runtime;
    PfxScriptEmitter* emitter;
    int field_index;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 0;
        runtime = (PfxResetRuntimeView*)effect->emitters;
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = pfx_get_emitter(runtime, emitter_index);
            emitter->emission_flags.bits.cycle_paused = 1;
            emitter->cycle_frame = 0;
            pfx_emitter_reset((PfxRuntimeEmitterView*)emitter);
        }

        if (runtime->particle_storage != 0) {
            memset(
                runtime->particle_storage->particles, 0,
                runtime->particle_storage->particle_count *
                    sizeof(PfxParticleResetRecord));
        } else {
            for (field_index = 0;
                 field_index < runtime->reset_field_count;
                 field_index++) {
                *runtime->reset_fields[field_index] = 0;
            }
        }

        runtime->reset_active = 0;
        runtime->reset_time = 0.0f;
    }
}

/* Soft ceiling: reset_effect_ppfx ~79.30% - split nonvolatile saves only. */
void reset_effect_ppfx(PfxScriptEffect* effect) {
    PfxResetRuntimeView* runtime;
    PfxScriptEmitter* emitter;
    int emitter_index;
    int field_index;

    runtime = (PfxResetRuntimeView*)effect->emitters;
    effect->lifecycle_flags.bits.restart_cycle = 0;
    for (emitter_index = 0;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        emitter = pfx_get_emitter(runtime, emitter_index);
        emitter->emission_flags.bits.cycle_paused = 1;
        emitter->cycle_frame = 0;
        pfx_emitter_reset((PfxRuntimeEmitterView*)emitter);
    }

    if (runtime->particle_storage != 0) {
        memset(
            runtime->particle_storage->particles, 0,
            runtime->particle_storage->particle_count *
                sizeof(PfxParticleResetRecord));
    } else {
        for (field_index = 0;
             field_index < runtime->reset_field_count;
             field_index++) {
            *runtime->reset_fields[field_index] = 0;
        }
    }

    runtime->reset_active = 0;
    runtime->reset_time = 0.0f;
}

void fx_reset(unsigned int handle) {
    PfxResolvedHandle resolved;
    PfxRuntimeEffect* effect;
    int index;

    resolve_pfx_handle(handle, &resolved);
    effect = (PfxRuntimeEffect*)resolved.effect;
    if (effect == 0) {
        return;
    }

    fx_reset_emit(handle);
    if (effect->reset.particle_storage != 0) {
        memset(
            effect->reset.particle_storage->particles, 0,
            effect->reset.particle_storage->particle_count *
                sizeof(PfxParticleResetRecord));
    } else {
        for (index = 0; index < effect->reset.reset_field_count; index++) {
            *effect->reset.reset_fields[index] = 0;
        }
    }

    effect->reset.reset_active = 0;
    effect->reset.reset_time = 0.0f;
}

void resume_effect(const char* name) {
    PfxScriptEffect* effect;
    PfxScriptEmitter* emitter;
    int emitter_index;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter_index = 0;
        emitter = pfx_get_emitter(effect->emitters, emitter_index);
        emitter->emission_flags.bits.cycle_paused = emitter_index;
    }
}

void kill_at_plane(float plane) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0) {
        effect->kill_plane = plane;
    }
}

void set_cycle_emission(int enabled) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_emission = enabled;
    }
}

void set_cycle_length(float length, float position) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_length = length;
        emitter->cycle_position = position;
    }
}

void set_growth_coefficient(float coefficient) {
    PfxScriptEnvironment* environment;
    int active;

    environment = 0;
    active = pfxscript_environment.active;
    if (active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->effect != 0) {
        environment = 0;
        if (active != 0) {
            environment = &pfxscript_environment;
        }
        environment->growth_coefficient = coefficient;
    }
}

void set_drag_coefficient(float coefficient) {
    PfxScriptEnvironment* environment;
    int active;

    environment = 0;
    active = pfxscript_environment.active;
    if (active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->effect != 0) {
        environment = 0;
        if (active != 0) {
            environment = &pfxscript_environment;
        }
        environment->drag_coefficient = coefficient;
    }
}

void emit_spherical_section(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere_section(environment->emitter, field);
    }
}

void emission_duration(float duration) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_length = duration;
        emitter->cycle_position = 0.0f;
        emitter->cycle_enabled = 1;
    }
}

void emit_from_point(float x, float y, float z) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->emission_point.x = x;
        emitter->emission_point.y = y;
        emitter->emission_point.z = z;
    }
}

void emit_uv(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_uv(environment->emitter, field);
    }
}

void emit_value(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_value(environment->emitter, field);
    }
}

void emit_value_i(int field, int value) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_line_1i(
            environment->emitter, field, value, value);
    }
}

void emit_constant_rate(void) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->emission_flags.bits.constant_rate = 1;
    }
}

void parametric_update(unsigned int effect) {
    if (effect != 0U) {
        build_parametric_effect_from_table(
            active_cmdscript->mko, effect, 1, active_cmdscript);
    }
}

int load_effect_bank(void) {
    CmdScript* script;
    LoadBgndCtx* load_context;

    script = active_cmdscript;
    if (script == 0) {
        return 0;
    }
    load_context = script->mko->load_ctx;
    if (load_context == 0) {
        return 0;
    }
    load_effect_bank_with_context();
    return 0xDEADBABE;
}

void* find_pfx_by_handle(unsigned int handle) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return resolved.effect;
}

#pragma dont_inline on
void* find_pfx_by_name_by_bankowner(const char* name, unsigned int owner) {
    PfxResolvedHandle resolved;
    unsigned int handle;

    handle = fx_by_owner(name, owner);
    resolve_pfx_handle(handle, &resolved);
    return resolved.effect;
}

PfxScriptEffect* find_pfx_by_name(const char* name) {
    PfxResolvedHandle resolved;
    unsigned int handle;

    handle = fx_by_owner(name, 0xFF);
    resolve_pfx_handle(handle, &resolved);
    return resolved.effect;
}
#pragma dont_inline reset

/* Soft ceiling: initialize_effect -- typed environment/effect/emitter setup. */
static void initialize_effect(PfxScriptEffect* effect) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;
    ScriptSlot* script;

    environment = active_pfx_environment();
    if (environment->effect == 0) {
        environment = active_pfx_environment();
        environment->effect = effect;
    }

    environment = active_pfx_environment();
    effect->initialization_mode = environment->field04;

    environment = active_pfx_environment();
    script = g_pfx_cmo;
    if (environment->texture_name != 0 &&
        environment->texture_name[0] != '\0') {
        effect->flags.raw |= 0x20;
        effect->texture_slot = (PfxTextureSlot*)load_named_tga_from_slot(
            script->load_ctx->art_id, environment->texture_name);
    }

    if ((effect->runtime_flags & 0x20) == 0) {
        effect->flags.bits.particle_size_enabled = 1;
        effect->particle_size = 1.0f;
    } else {
        effect->flags.bits.particle_size_enabled = 0;
    }

    environment = active_pfx_environment();
    if (environment->emitter == 0) {
        emitter = pfx_get_emitter(effect, 0);
        script = g_pfx_cmo;
        emitter->birthrate = parametric_birthrate;
        environment = active_pfx_environment();
        environment->emitter = emitter;
        push_script_stack_frame(0);
        environment = active_pfx_environment();
        cmdscript_setup_execution(script, environment->initialization_script);
        cmdscript_execute(script);
        environment = active_pfx_environment();
        environment->emitter = 0;
    }
}

/* Soft ceiling: pfxscript_initialize -- typed bank-latch reset and proc setup. */
void pfxscript_initialize(void) {
    int flags;
    int proc_flags;
    int index;
    int remaining;

    index = 0;
    remaining = 15;
    do {
        banks[index].bank = 0;
        banks[index].bank_instance = 0;
        index++;
        remaining--;
    } while (remaining != 0);
    flags = 0;
    cached_handle = 0;
    ((unsigned char*)&flags)[0] |= MKPROC_FLAG_NO_DESTROY;
    proc_flags = flags;
    create_mkproc(0x2E, get_mkproc_nostack(&proc_flags), 0x7777,
                  p_update_effects, 0);
}

void bank_destroy(MkHdr* bank) {
    PfxBankVtableRef vtbl;

    if (bank->instance != 0U) {
        vtbl.base = bank->vtbl;
        vtbl.bank->destroy(bank);
    }
}

void fxbanks_unload_by_owner(unsigned int owner_flags) {
    int index;

    for (index = 0; index < 15; index++) {
        PfxBank* bank;

        bank = banks[index].bank;
        if (bank != 0 &&
            bank->hdr.instance != banks[index].bank_instance) {
            bank = 0;
        }
        if (bank != 0 && (bank->owner_flags & owner_flags) != 0) {
            bank_destroy(&bank->hdr);
            banks[index].bank = 0;
            banks[index].bank_instance = 0;
        }
    }
}

void unload_all_effect_banks(void) {
    int index;

    for (index = 0; index < 15; index++) {
        PfxBank* bank;

        bank = banks[index].bank;
        if (bank != 0 &&
            bank->hdr.instance != banks[index].bank_instance) {
            bank = 0;
        }
        if (bank != 0) {
            bank_destroy(&bank->hdr);
        }
    }
    for (index = 0; index < 15; index++) {
        banks[index].bank = 0;
        banks[index].bank_instance = 0;
    }
}

void emit_cartesian(
    int unused, float x, float y, float z,
    float width, float height, float depth) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_box(
            -(width * 0.5f - x), -(height * 0.5f - y),
            -(depth * 0.5f - z));
    }
}

void emit_color(int field, int red, int green, int blue, int alpha) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            field, (float)red, (float)green, (float)blue, (float)alpha);
    }
}

void emit_from_pos(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(field, source, 0, 0.0f);
    }
}

void emit_from_pos_clamp_y(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(field, source, 1, 0.0f);
    }
}

void emit_roundrobin_mechanism(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_roundrobin_mechanism(field, source);
    }
}

void emit_spherical(int unused, float radius) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(0, 0.0f, 0.0f, 0.0f, 0.00001f, radius);
    }
}

void emit_spherical_from_boundary(int unused, float radius) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(0, 0.0f, 0.0f, 0.0f, radius);
    }
}

void kill_on_y_less_than_field(int context, int field) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_kill_on_y_less_than_field(context, field);
    }
}

/* Soft ceiling: bind_to_bone -- typed script/load-context/object chain. */
void bind_to_bone(int bone_index) {
    PfxScriptEnvironment* environment;
    MkPfx* effect;
    MkObj* object;

    environment = active_pfx_environment();
    effect = environment->source_effect;
    if (bone_index != 0 && active_cmdscript != 0 &&
        active_cmdscript->mko != 0 &&
        active_cmdscript->mko->load_ctx != 0) {
        object = active_cmdscript->mko->load_ctx->bgnd_obj;
        if (object != 0 &&
            (object->oid == 0x1001 || object->oid == 0x1002)) {
            effect->bound_obj = object;
            if (effect->bound_obj != 0) {
                pfx_bind_emitter_to_obj_bone(
                    effect, effect->bound_obj, bone_index);
            }
        }
    }
}

/* Soft ceiling: fx_bind_emitter_to_obj_bone ~74.74% - save scheduling only. */
void fx_bind_emitter_to_obj_bone(
    unsigned int handle, MkObj* object, int bone_index) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_emitter_to_obj_bone(
            (MkPfx*)resolved.effect, object, bone_index);
    }
}

void fx_bind_render_to_sobj(unsigned int handle, MkSobj* object) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_render_to_sobj((MkPfx*)resolved.effect, object, 0);
    }
}

/* Soft ceiling: fx_bind_render_to_obj_bone ~74.74% - save scheduling only. */
void fx_bind_render_to_obj_bone(
    unsigned int handle, MkObj* object, int bone_index) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_render_to_obj_bone(
            (MkPfx*)resolved.effect, object, bone_index);
    }
}

void spawn_color(int field, int red, int green, int blue, int alpha) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            field, (float)red, (float)green, (float)blue, (float)alpha);
    }
}

void update_attract(int context, int field) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_attract(context, field);
    }
}

void update_bounce(int context, int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_bounce(context, field, source);
    }
}

void update_fade_alpha(int context, int field) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_fade_alpha(context, field, 0xFF, 0);
    }
}

void update_fade_alpha2(int context, int field, int start, int end) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_fade_alpha(context, field, start, end);
    }
}

void update_texanim(int context, int field, int first, int last) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_animate_texture(context, field, first, last, 0, 1);
    }
}

void update_texanim_hold(int context, int field, int first, int last) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        pfxvm_update_animate_texture(context, field, first, last, 0, 0);
    }
}

void emit_in_range(int unused, float center, float width) {
    PfxScriptEnvironment* environment = 0;
    float half_width;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        half_width = width * 0.5f;
        pfxvm_spawn_line_1f(center - half_width, center + half_width);
    }
}

void initial_divert(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        range.minimum = minimum;
        range.maximum = maximum;
        pfxvm_initial_divert(&range);
    }
}

void initial_multiply_float(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        range.minimum = minimum;
        range.maximum = maximum;
        pfxvm_initial_multiply_float_range(&range);
    }
}

void initial_set_float(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->kill_percent_field != 0U) {
        range.minimum = minimum;
        range.maximum = maximum;
        pfxvm_initial_set_float_range(&range);
    }
}

float p_update_effects(void) {
    int index;

    for (index = 0; index < 15; index++) {
        PfxBank* bank;

        bank = banks[index].bank;
        if (bank != 0 &&
            bank->hdr.instance != banks[index].bank_instance) {
            bank = 0;
        }
        if (bank != 0) {
            bank_run_fx(&bank->hdr);
        }
    }
    return 0.0f;
}

void emit_cylindrical(
    int unused, float x, float y, float z, float radius,
    float height, float start, float end) {
    PfxScriptEnvironment* environment = 0;
    Vec origin;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        origin.x = x;
        origin.y = y;
        origin.z = z;
        pfxvm_spawn_cylinder(&origin, radius, height, start, end);
    }
}

void emit_disc(
    int unused, float x, float y, float z, float outer_radius) {
    PfxScriptEnvironment* environment = 0;
    Vec origin;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        origin.x = x;
        origin.y = y;
        origin.z = z;
        pfxvm_spawn_disc(&origin, 0.0f, outer_radius);
    }
}

void emit_disc2(
    int unused, float x, float y, float z,
    float inner_radius, float outer_radius) {
    PfxScriptEnvironment* environment = 0;
    Vec origin;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        origin.x = x;
        origin.y = y;
        origin.z = z;
        pfxvm_spawn_disc(&origin, inner_radius, outer_radius);
    }
}
