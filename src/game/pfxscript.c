#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "game/bgnd.h"
#include "game/game_info.h"
#include "libmkparticle/color.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/compile.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/metrics.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/range.h"
#include "libmkparticle/spawn.h"
#include "libmkparticle/table.h"
#include "libmkparticle/texture_anim.h"
#include "libmkparticle/update.h"
#include "libmkparticle/vm.h"
#include "math/gxVect.h"
#include "platform/main.h"
#include "rw/rwcore_types.h"

typedef struct PfxSpawnTableSlot {
    int type;
    int row_count;
    PfxSpawnTable* table;
} PfxSpawnTableSlot;

typedef struct PfxScriptEnvironment {
    int active;
    int field04;
    MkPfx* source_effect; /* +0x08 */
    struct PfxScriptEffect* effect; /* +0x0C */
    PfxVmEmitter* emitter; /* +0x10 */
    int* remaining_effects; /* +0x14 */
    PfxSpawnTableSlot* spawn_tables; /* +0x18 */
    char* effect_name_override; /* +0x1C */
    char* texture_name; /* +0x20 */
    unsigned int initialization_script; /* +0x24 */
    float drag_coefficient;   /* +0x28 */
    float growth_coefficient; /* +0x2C */
    float fields30[2];
    union {
        unsigned int behavior_count; /* build setup phase */
        PfxBehavior* behavior;       /* behavior-script execution phase */
    }; /* +0x38 */
    PfxBehavior* next_behavior; /* +0x3C */
} PfxScriptEnvironment;

typedef struct PfxScriptEffectFlagBits {
    unsigned char vertex_color_enabled : 1; /* bit7 */
    unsigned char particle_size_enabled : 1; /* bit6 */
    unsigned char pad_bits5_4 : 2;
    unsigned char light_enabled : 1; /* bit3 */
    unsigned char light_mode : 1; /* bit2 */
    unsigned char pad_bits1_0 : 2;
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
    unsigned char owner_special : 1; /* bit3 */
    unsigned char pad_bits2_0 : 3;
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

typedef struct PfxParametricFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char scan_flag_40 : 1;
    unsigned char scan_flag_20 : 1;
    unsigned char pad_bits4_0 : 5;
} PfxParametricFlagBits;

typedef union PfxParametricFlags {
    unsigned char raw;
    PfxParametricFlagBits bits;
} PfxParametricFlags;

typedef struct PfxScriptEffect {
    MkHdr hdr; /* +0x00 */
    PfxLifecycleFlags lifecycle_flags; /* +0x08 */
    char pad09[0x1F];
    float effect_value; /* +0x28 */
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
    char pad152[2];
    float light_direction_components[3]; /* +0x154 */
    PfxColor light_color; /* +0x160 */
    Vec light_position; /* +0x164 */
    float z_bias; /* +0x170 */
    RwTexture* texture; /* +0x174 */
    char pad178[0x0A];
    short texture_animation_enabled; /* +0x182 */
    char pad184[0x0C];
    PfxZTestFlags ztest_flags; /* +0x190 */
    PfxParametricFlags parametric_flags; /* +0x191 */
    char pad192[2];
    float decal_plane[6]; /* +0x194 */
    float aspect_x; /* +0x1AC */
    float aspect_y; /* +0x1B0 */
    PfxColor vertex_color; /* +0x1B4 */
    float particle_size; /* +0x1B8 */
    float bounding_radius; /* +0x1BC */
    unsigned int behavior_target; /* +0x1C0 */
    char pad1C4[0x10];
    unsigned int runtime_flags; /* +0x1D4 */
    char pad1D8[0x28];
    int emitter_count; /* +0x200 */
    PfxVmEmitter* emitter; /* +0x204 */
    char pad208[0x14];
    const char* metrics_name; /* +0x21C */
    char pad220[4];
    PfxMetrics* load_metrics; /* +0x224 */
    float kill_plane; /* +0x228 */
    int initialization_mode; /* +0x22C */
    char pad230[0x2C];
    const char* effect_name; /* +0x25C */
    int effect_id; /* +0x260 */
    PfxMetrics* metrics; /* +0x264 */
    int field268;
    int parametric; /* +0x26C */
} PfxScriptEffect;

typedef struct PfxVertexColorArgs {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int alpha;
} PfxVertexColorArgs;

typedef struct PfxLightArgs {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int alpha;
    float direction_components[3];
    int mode;
    Vec position;
} PfxLightArgs;

typedef struct PfxScriptColorRow {
    int red;
    int green;
    int blue;
    int alpha;
} PfxScriptColorRow;

typedef struct PfxParametricEmitterDescription {
    PfxVec3 origin;
    unsigned int initialization_script;
} PfxParametricEmitterDescription;

typedef struct PfxParametricEffectDescription {
    char* effect_name;
    int effect_id;
    char* texture_name;
    PfxParametricEmitterDescription* emitter;
    float* table_a;
    float* table_b;
    PfxScriptColorRow* color_table;
    float field1C;
    float field20;
    float field24;
    char pad28[4];
    float field2C;
    char pad30[4];
    float field34;
    float emitter_lifetime;
    float allocation_count;
    int blend_mode;
    float effect_value;
} PfxParametricEffectDescription;

typedef struct PfxStepTextureDescription {
    char* name;
    int frame_count;
    float horizontal_scale;
} PfxStepTextureDescription;

typedef struct PfxStepEffectDescription {
    char* effect_name;
    int effect_id;
    PfxStepTextureDescription* texture;
    PfxParametricEmitterDescription* emitter;
    unsigned int* behavior_scripts;
    float emitter_lifetime;
    int field18;
    int allocation_count;
    int blend_mode;
    float effect_value;
} PfxStepEffectDescription;

typedef struct PfxBankLoadRow {
    unsigned int function_index;
    int effect_count;
} PfxBankLoadRow;

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

typedef struct PfxResolvedHandle {
    PfxBank* bank;
    int effect_index;
    PfxScriptEffect* effect;
} PfxResolvedHandle;

struct PfxBank {
    MkHdr hdr;
    unsigned int handle_bank; /* +0x08 */
    unsigned int handle_generation; /* +0x0C */
    char* name; /* +0x10 */
    unsigned int owner_flags; /* +0x14 */
    int effect_count; /* +0x18 */
    int effect_capacity; /* +0x1C */
    PfxEffectLatch* effects; /* +0x20 */
    unsigned int* effect_owners; /* +0x24 */
};

static void vdestroy_effectbank(PfxBank* bank);
static PfxResolvedHandle cached_info = { 0, 0, 0 };
static PfxScriptEnvironment pfxscript_environment = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0.0f,
    0.0f,
    { 0, 0 },
    0,
    0,
};
static MkVtable5 vtbl_effectbank = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    (MkVtblFn)vdestroy_effectbank,
};
static unsigned int bank_instance_counter[16] = { 0 };
static int g_profile_enabled;
static float parametric_birthrate = 1.0f;
static PfxBank* current_effect_bank;
/* Retail .sbss symbol; no recovered clean-C consumer in this unit. */
static const PfxParametricEffectDescription* g_effect_description;
static PfxBehavior* behavior_buffer;
static void* old_ltm;

static PfxBankLatch banks[16];
static unsigned int cached_handle;
static ScriptSlot* g_pfx_cmo;
static void bank_run_fx(PfxBank* bank);

void* memset(void* destination, int value, unsigned long size);
void* memcpy(void* destination, const void* source, unsigned long size);
int strcmp(const char* left, const char* right);
unsigned long strlen(const char* text);
char* strcpy(char* destination, const char* source);
/* Soft ceiling: 74.91% - exact owned-effect search, four-instruction residue. */
static unsigned int banks_find_owned_fx(
    const char* name, unsigned int owner);
/* Retail builder ABI: script, effect handle/table, then update mode. */
static void build_step_effect(
    ScriptSlot* script, unsigned int effect, int update);
static void build_parametric_effect_from_table(
    ScriptSlot* script, unsigned int effect, int update);
void load_effect_bank_with_context(char* name, LoadBgndCtx* context);
static void resolve_pfx_handle(
    unsigned int handle, PfxResolvedHandle* resolved);
static void initialize_effect(PfxScriptEffect* effect);
void fx_reset_emit(unsigned int effect);
static inline void bank_destroy(MkHdr* bank);
PfxScriptEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(PfxScriptEffect* effect);
static inline PfxScriptEffect* resolve_effect_handle(unsigned int handle) {
    PfxResolvedHandle resolved;
    int kind = (handle >> 14) & 3;

    if (kind != 1 && kind != 2) {
        return 0;
    }
    resolve_pfx_handle(
        (handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return resolved.effect;
}

static float p_update_effects(void);

static inline PfxScriptEnvironment* active_pfx_environment(void) {
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

MkPfx* pfx_from_handle(unsigned int handle) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
}

/* Soft ceiling: verified local effect/emitter view and handle encoding. */
unsigned int fx_next_emitter(unsigned int handle) {
    PfxResolvedHandle resolved;
    PfxScriptEffect* effect;
    int type;
    int emitter_index;

    type = (handle >> 14) & 3;
    if (type == 1 || type == 2) {
        resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
        effect = resolved.effect;
    } else {
        effect = 0;
    }

    if (effect == 0) {
        return 0;
    }

    for (emitter_index = 0;
        emitter_index < effect->emitter_count;
         emitter_index++) {
        if (pfx_emitter_exhausted(
                &effect->emitter[emitter_index]) ||
            pfx_emitter_unused(
                &effect->emitter[emitter_index])) {
            pfx_emitter_reset(
                &effect->emitter[emitter_index]);
            handle = (handle & 0xFFFF3FFF) | 0x8000;
            handle &= 0xFFF0FFFF;
            handle |= (emitter_index & 0xF) << 16;
            return handle;
        }
    }
    return 0;
}

MkPfx* pfx_from_emitter(unsigned int handle) {
    PfxResolvedHandle resolved;
    int type;

    type = (handle >> 14) & 3;
    if (type != 1 && type != 2) {
        return 0;
    }
    resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return (MkPfx*)resolved.effect;
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

/* Soft ceiling: 69.98% - exact latch search, three-instruction residue. */
unsigned int fx2(unsigned int bank_handle, const char* name) {
    PfxBankLatch* bank_latch;
    PfxEffectLatch* effect_latch;
    PfxBank* raw_bank;
    PfxBank* bank;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    int bank_index;
    int effect_index;

    if (bank_handle != 0xDEADBABE) {
        return 0;
    }

    bank_index = bank_handle & 0xF;
    if (bank_index == 0 || bank_index > 15) {
        bank = 0;
    } else {
        bank_latch = &banks[bank_index - 1];
        raw_bank = bank_latch->bank;
        if (raw_bank != 0 &&
            raw_bank->hdr.instance == bank_latch->bank_instance) {
            bank = raw_bank;
        } else {
            bank = 0;
        }
        if (bank != 0 &&
            (bank_latch->bank_instance & 0xFFFFFFF0) !=
                (bank_handle & 0xFFFFFFF0)) {
            bank = 0;
        }
    }

    for (effect_index = 0; effect_index < bank->effect_count; effect_index++) {
        effect_latch = &bank->effects[effect_index];
        raw_effect = effect_latch->effect;
        if (raw_effect != 0 &&
            raw_effect->hdr.instance == effect_latch->effect_instance) {
            effect = raw_effect;
        } else {
            effect = 0;
        }
        if (effect != 0 && effect->effect_name != 0 &&
            strcmp(name, effect->effect_name) == 0) {
            return ((effect_index << 4) & 0x3FF0) |
                   (bank->handle_bank & 0xF) | 0x4000 |
                   (bank->handle_generation << 24);
        }
    }
    return 0;
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
    PfxVm* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = (PfxVm*)resolved.effect->emitters;

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
    PfxVm* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = (PfxVm*)resolved.effect->emitters;

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

/*
 * Soft ceiling: retail m2c confirms both handle resolutions, the direct
 * emitter-0x202 path, generic field path, and ordered Vec stores. The
 * 308-byte bodies are equal; residue is saved GPR/FPR allocation, equivalent
 * branch polarity, and relocation labels.
 */
void fx_set_param_v3(
    unsigned int handle, int parameter, float x, float y, float z) {
    PfxScriptEffect* effect;
    Vec* target;

    target = 0;
    if ((parameter & 0xF00) == 0x200) {
        effect = resolve_effect_handle(handle);
        if (effect != 0) {
            if (parameter == 0x202) {
                int emitter_index;

                effect = resolve_effect_handle(handle);
                if (effect != 0) {
                    emitter_index = (handle >> 16) & 0xF;
                    if (emitter_index < effect->emitter_count) {
                        target = (Vec*)&effect->emitter[emitter_index];
                    }
                }
            } else {
                target = (Vec*)pfx_get_field(
                    (PfxVm*)effect->emitters, -2, parameter);
            }

            if (target != 0) {
                target->x = x;
                target->y = y;
                target->z = z;
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
    MkObj* render_object;

    environment = active_pfx_environment();
    if (environment->effect != 0) {
        environment = active_pfx_environment();
        clone = pfx_create_clone(environment->source_effect);
        render_object = pfx_clone_bind_render_to_new_obj(clone, 0xFF00);
        clone->priority = field_28;
        render_object->scale.x = 1.0f;
        render_object->scale.y = -1.0f;
        render_object->scale.z = 1.0f;
        render_object->flags_08 |= 2;
        update_mkobj(render_object != 0 ? as_mkhdr(&render_object->hdr) : 0);
    }
}

void set_vertex_color(const PfxVertexColorArgs* color) {
    PfxScriptEnvironment* environment = 0;
    PfxScriptEffect* effect;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0 && effect->flags.bits.vertex_color_enabled) {
        pfx_native_set_rgba(
            &effect->vertex_color,
            color->red, color->green, color->blue, color->alpha);
    }
}

/* Soft ceiling: 53.66% - fixed three-vector copy unrolls; size and algorithm exact. */
void set_light(const PfxLightArgs* light) {
    PfxScriptEnvironment* environment = 0;
    PfxScriptEffect* effect;
    int component;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    effect = environment->effect;
    if (effect != 0 && !effect->flags.bits.light_enabled) {
        for (component = 0; component < 3; component++) {
            effect->light_direction_components[component] =
                light->direction_components[component];
        }
        effect->light_position = light->position;
        pfx_native_set_rgba(
            &effect->light_color,
            light->red, light->green, light->blue, light->alpha);
        effect->flags.bits.light_enabled = 1;
        effect->flags.bits.light_mode = light->mode;
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

void enable_profiling(int enabled) {
    g_profile_enabled = enabled;
}

void initial_multiply_float(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_multiply_float_range(
            environment->behavior,
            (unsigned int)unused, &range);
    }
}

void initial_set_float(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_set_float_range(
            environment->behavior,
            (unsigned int)unused, &range);
    }
}

void initial_divert(int unused, float minimum, float maximum) {
    PfxScriptEnvironment* environment = 0;
    PfxFloatRange range;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_divert(
            environment->behavior,
            (unsigned int)unused, &range);
    }
}

void initial_add_v3(int destination, int source) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_initial_add_v3(
            environment->behavior, destination, source);
    }
}

void initial_reflect(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_initial_reflect(environment->behavior, field);
    }
}

void kill_on_y_less_than_field(int field, int reference_field) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_kill_on_y_less_than_field(
            environment->behavior, field, reference_field);
    }
}

void change_on_y_less_than_field(int field, int source) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0 && environment->next_behavior != 0) {
        pfxvm_change_on_y_less_than_field(
            environment->behavior, field, source, environment->next_behavior);
    }
}

void change_on_y_less(int field, float value) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0 && environment->next_behavior != 0) {
        pfxvm_change_on_y_less(
            environment->behavior, field, value, environment->next_behavior);
    }
}

void change_on_less(int field, float value) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0 && environment->next_behavior != 0) {
        pfxvm_change_on_less(
            environment->behavior, field, value, environment->next_behavior);
    }
}

void change_on_greater(int field, float value) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0 && environment->next_behavior != 0) {
        pfxvm_change_on_greater(
            environment->behavior, field, value, environment->next_behavior);
    }
}

void kill_roundrobin(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_kill_roundrobin(environment->behavior, field);
    }
}

void kill_percent(float percent) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_kill_percent(environment->behavior, percent);
    }
}

void kill_on_greater(int field, float value) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_kill_on_greater(environment->behavior, field, value);
    }
}

void udpate_roundrobin(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_roundrobin(environment->behavior, field);
    }
}

void update_assign(int destination, int source) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_assign(
            environment->behavior, destination, source);
    }
}

void update_texanim_hold(int texture_field, int age_field, int frame_count,
                         int frame_offset, float frame_time) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_animate_texture(
            environment->behavior, texture_field, age_field, frame_count,
            frame_offset, 0, 0, frame_time);
    }
}

void update_texanim(int texture_field, int age_field, int frame_count,
                    int frame_offset, float frame_time) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_animate_texture(
            environment->behavior, texture_field, age_field, frame_count,
            frame_offset, 0, 1, frame_time);
    }
}

void update_fade_alpha2(int color_field, int age_field, int start_alpha,
                        int end_alpha, float start_time, float duration) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_fade_alpha(
            environment->behavior, color_field, age_field, start_alpha,
            end_alpha, start_time, duration);
    }
}

/* Soft ceiling: 76.51% - exact packed-color loop, two-instruction residue. */
void update_lerp_color(
    int color_field, int age_field, int color_count, int first_color,
    const PfxScriptColorRow* table, float duration) {
    PfxScriptEnvironment* environment;
    PfxColor* colors;
    PfxColor* color;
    const PfxScriptColorRow* row_data;
    unsigned int row_count;
    int remaining;

    environment = active_pfx_environment();
    if (environment->behavior == 0 || g_pfx_cmo == 0) {
        return;
    }

    row_count = get_row_count_for_table_by_pointer(
        g_pfx_cmo, (void*)table);
    if (row_count == 0U) {
        return;
    }

    colors = get_mem(row_count * sizeof(*colors));
    color = colors;
    row_data = table;
    remaining = row_count;
    do {
        color->r = row_data->red;
        color->g = row_data->green;
        color->b = row_data->blue;
        color->a = row_data->alpha;
        color++;
        row_data++;
    } while (--remaining != 0);
    pfxvm_update_lerp_color(
        environment->behavior, color_field, age_field, color_count,
        first_color, colors, duration);
}

void update_fade_alpha(int color_field, int age_field, float start_time,
                       float duration) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_fade_alpha(
            environment->behavior, color_field, age_field, 0xFF, 0,
            start_time, duration);
    }
}

void update_wrapbox(int field, float scale, float x, float y, float z) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_wrapbox(environment->behavior, field, scale, x, y, z);
    }
}

void update_mul_scalar(int field, float x, float y, float z) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_mul_scalar(environment->behavior, field, x, y, z);
    }
}

void update_copy(int field) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_copy(environment->behavior, field);
    }
}

void update_add_constant(int field, float value) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_add_constant(environment->behavior, field, value);
    }
}

void update_add_constant_v3(int field, float x, float y, float z) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->behavior != 0) {
        pfxvm_update_add_constant_v3(environment->behavior, field, x, y, z);
    }
}

void update_bounce(int field, int velocity_field, int bounce_count_field,
                   float scale) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_bounce(
            environment->behavior, field, velocity_field,
            bounce_count_field, scale);
    }
}

void update_add(int destination, int source) {
    PfxScriptEnvironment* environment;
    PfxBehavior* behavior;

    environment = active_pfx_environment();
    if (environment != 0) {
        behavior = environment->behavior;
        if (behavior != 0) {
            pfxvm_update_add(behavior, destination, source);
            if ((source & 0xF00) != 0x200) {
                pfxvm_update_copy(behavior, source);
            }
        }
    }
}

void update_attract(int field, int target_field, float strength) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_attract(
            environment->behavior, field, target_field, strength);
    }
}

void create_multiemit_parametric_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_parametric_effect_from_table(
            active_cmdscript->mko, (unsigned int)effect, 0);
        *effect = saved;
    }
}

void create_parametric_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_parametric_effect_from_table(
            active_cmdscript->mko, (unsigned int)effect, 1);
        *effect = saved;
    }
}

void create_multiemit_step_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_step_effect(active_cmdscript->mko, (unsigned int)effect, 0);
        *effect = saved;
    }
}

void create_step_fx(unsigned int* effect, unsigned int id) {
    unsigned int saved;

    if (effect != 0 && id != 0) {
        saved = *effect;
        *effect = id;
        build_step_effect(active_cmdscript->mko, (unsigned int)effect, 1);
        *effect = saved;
    }
}

void create_step_effect(unsigned int effect) {
    if (effect != 0U) {
        build_step_effect(active_cmdscript->mko, effect, 1);
    }
}

static void build_step_effect(
    ScriptSlot* script, unsigned int effect_table, int update) {
    const PfxStepEffectDescription* description;
    PfxScriptEnvironment* environment;
    PfxSpawnTableSlot table_slots[2];
    PfxBuildInfo build;
    PfxVmEmitter emitter_template;
    PfxScriptEffect* effect;
    PfxVm* runtime;
    PfxVmEmitter* emitter;
    PfxVmEmitter* first_emitter;
    PfxBehavior* behavior;
    PfxBehavior* behavior_targets[10];
    unsigned int behavior_fields[2] = { 0, 0 };
    unsigned int behavior_flags[2] = { 0, 0 };
    unsigned int scan_fields[2] = { 2, 0 };
    unsigned int next_fields[2];
    unsigned int next_flags[2];
    unsigned int behavior_count;
    unsigned int behavior_index;
    unsigned int instruction_index;
    int emitter_index;
    void* transform;

    description = (const PfxStepEffectDescription*)effect_table;
    environment = active_pfx_environment();
    if (environment->remaining_effects == 0 ||
        *environment->remaining_effects == 0) {
        return;
    }

    memset(table_slots, 0, sizeof(table_slots));
    environment = active_pfx_environment();
    environment->spawn_tables = table_slots;
    memset(&build, 0, sizeof(build));
    environment = active_pfx_environment();
    build.name = environment->effect_name_override;
    if (build.name == 0) {
        build.name = description->effect_name;
    }
    build.emitter_count = update;
    if (description->emitter->origin.x != 0.0f ||
        description->emitter->origin.y != 0.0f ||
        description->emitter->origin.z != 0.0f) {
        build.flags = 0x80000000;
    }
    if (g_profile_enabled != 0) {
        build.metrics_frame_count = 0x384;
    }

    environment = active_pfx_environment();
    environment->effect = 0;
    environment = active_pfx_environment();
    environment->texture_name = description->texture->name;
    environment = active_pfx_environment();
    environment->initialization_script =
        description->emitter->initialization_script;
    environment = active_pfx_environment();
    if (environment->initialization_script == 0U) {
        return;
    }

    memset(&emitter_template, 0, sizeof(emitter_template));
    g_pfx_cmo = script;
    parametric_birthrate = 1.0f;
    environment = active_pfx_environment();
    environment->emitter = &emitter_template;
    push_script_stack_frame(0);
    environment = active_pfx_environment();
    cmdscript_setup_execution(script, environment->initialization_script);
    cmdscript_execute(script);
    environment = active_pfx_environment();
    environment->emitter = 0;
    g_pfx_cmo = 0;

    behavior_count = get_row_count_for_table_by_pointer(
        script, description->behavior_scripts);
    build.behavior_count = behavior_count;
    environment = active_pfx_environment();
    environment->behavior_count = behavior_count;
    if (behavior_count == 0U) {
        return;
    }

    g_pfx_cmo = script;
    for (behavior_index = 0;
         behavior_index < behavior_count;
         behavior_index++) {
        behavior = &behavior_buffer[behavior_index];
        memset(behavior, 0, sizeof(*behavior));
        environment = active_pfx_environment();
        environment->behavior = behavior;
        if (behavior_index + 1 < behavior_count) {
            environment = active_pfx_environment();
            environment->next_behavior =
                &behavior_buffer[behavior_index + 1];
        } else {
            environment = active_pfx_environment();
            environment->next_behavior = 0;
        }
        push_script_stack_frame(0);
        cmdscript_setup_execution(
            script, description->behavior_scripts[behavior_index]);
        cmdscript_execute(script);
    }
    g_pfx_cmo = 0;

    pfx_behavior_scan_fields(
        behavior_buffer, behavior_fields, behavior_flags);
    for (behavior_index = 1;
         behavior_index < behavior_count;
         behavior_index++) {
        behavior = &behavior_buffer[behavior_index];
        memset(next_fields, 0, sizeof(next_fields));
        memset(next_flags, 0, sizeof(next_flags));
        pfx_behavior_scan_fields(behavior, next_fields, next_flags);
        if (next_fields[1] != behavior_flags[1] ||
            next_flags[0] != behavior_flags[0]) {
            return;
        }
        behavior_fields[0] |= next_fields[0];
        behavior_fields[1] |= next_fields[1];
    }

    pfx_emitter_scan_for_fields(&emitter_template, scan_fields);
    if ((scan_fields[0] & 0x100) == 0 &&
        description->texture->frame_count > 1) {
        scan_fields[0] |= 0x200;
    }
    if ((scan_fields[0] & 0x200) != 0) {
        behavior_fields[1] |= 2;
    }
    if ((behavior_fields[1] & 2) != 0 &&
        (behavior_flags[1] & 2) == 0 &&
        (scan_fields[1] & 2) == 0) {
        for (behavior_index = 0;
             behavior_index < behavior_count;
             behavior_index++) {
            behavior = &behavior_buffer[behavior_index];
            pfxvm_update_age(behavior, 0x301);
            pfxvm_update_make_last_insn_first(behavior);
        }
        pfxvm_spawn_value(&emitter_template, 0x301, 0.0f);
        behavior_flags[1] |= 2;
        scan_fields[1] |= 2;
    }

    environment = active_pfx_environment();
    environment->emitter = &emitter_template;
    g_pfx_cmo = script;
    effect = 0;
    new_pfx_create_raw_userdata(
        &build, 0, description->allocation_count,
        scan_fields[0], scan_fields[1],
        (PfxInitCb)initialize_effect, 0, 0, (void**)&effect);
    if (effect == 0) {
        environment = active_pfx_environment();
        environment->emitter = 0;
        g_pfx_cmo = 0;
        return;
    }

    runtime = (PfxVm*)effect->emitters;
    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = pfx_get_emitter(runtime, emitter_index);
            transform = emitter->transform;
            environment = active_pfx_environment();
            memcpy(emitter, environment->emitter, sizeof(*emitter));
            emitter->transform = transform;
        }
        environment = active_pfx_environment();
        environment->emitter = runtime->emitters;
    }
    g_pfx_cmo = 0;

    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        environment = active_pfx_environment();
        if (table_slots[emitter_index].type != 0) {
            pfx_register_table(
                (PfxTableRegistry*)runtime, emitter_index,
                table_slots[emitter_index].table);
        }
    }
    pfx_copy_behavior_list(runtime, behavior_count, behavior_buffer);
    for (behavior_index = 0; behavior_index < 10; behavior_index++) {
        behavior_targets[behavior_index] =
            &behavior_buffer[behavior_index];
    }
    pfx_behaviors_fixup_targets(
        runtime->behavior_list, behavior_targets, behavior_count);
    for (behavior_index = 0;
         behavior_index < (unsigned int)runtime->behavior_count;
         behavior_index++) {
        behavior = pfx_behavior(runtime, behavior_index);
        if (behavior != 0) {
            for (instruction_index = 0;
                 instruction_index <
                     (unsigned int)behavior->update_instruction_count;
                 instruction_index++) {
                if (behavior->update_instructions[instruction_index].opcode ==
                    0xE) {
                    behavior->update_instructions[instruction_index].target =
                        &effect->behavior_target;
                }
            }
        }
    }
    pfxvm_compile(runtime);

    first_emitter = pfx_get_emitter(runtime, 0);
    first_emitter->position = description->emitter->origin;
    transform = first_emitter->transform;
    if (transform != 0 &&
        (description->emitter->origin.x != 0.0f ||
         description->emitter->origin.y != 0.0f ||
         description->emitter->origin.z != 0.0f)) {
        float* matrix = transform;

        memset(matrix, 0, 16 * sizeof(*matrix));
        matrix[0] = 1.0f;
        matrix[5] = 1.0f;
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        matrix[12] = description->emitter->origin.x;
        matrix[13] = description->emitter->origin.y;
        matrix[14] = description->emitter->origin.z;
    }
    effect->effect_value = description->effect_value;
    first_emitter->flags.bits.cycle_paused = 1;
    effect->lifecycle_flags.bits.restart_cycle = 0;
    environment = active_pfx_environment();
    environment->source_effect = (MkPfx*)effect;
    effect->effect_id = description->effect_id;
    first_emitter->lifetime = description->emitter_lifetime;
    for (emitter_index = 1;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        emitter = pfx_get_emitter(runtime, emitter_index);
        transform = emitter->transform;
        memcpy(emitter, first_emitter, sizeof(*emitter));
        emitter->transform = transform;
    }

    effect->vertex_color.r = 0xFF;
    effect->vertex_color.g = 0xFF;
    effect->vertex_color.b = 0xFF;
    effect->vertex_color.a = 0xFF;
    effect->flags.bits.vertex_color_enabled = 1;
    pfx_render_set_blendmode(
        (struct PfxRenderView*)runtime, description->blend_mode);
    if (description->texture->frame_count > 1 &&
        effect->texture != 0) {
        RwRaster* texture = effect->texture->raster;
        float width = (float)texture->width;
        float scale = description->texture->horizontal_scale;

        if (effect->initialization_mode != 0) {
            effect->runtime_flags |= 0x100;
        }
        pfx_texture_animate(
            (PfxVm*)effect, 0.0f, (int)width, (int)(scale * width),
            (int)((float)texture->height /
                  (scale * (float)description->texture->frame_count)),
            description->texture->frame_count);
        effect->texture_animation_enabled = 1;
    }

    if (pfx_verify((struct PfxVerifyView*)runtime) != 0) {
        environment = active_pfx_environment();
        (*environment->remaining_effects)--;
        if (current_effect_bank->effect_capacity <
            current_effect_bank->effect_count) {
            int index = current_effect_bank->effect_capacity;
            current_effect_bank->effects[index].effect = effect;
            current_effect_bank->effects[index].effect_instance =
                effect->hdr.instance;
            current_effect_bank->effect_owners[index] = 0;
            current_effect_bank->effect_capacity++;
        }
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            pfx_emitter_reset(pfx_get_emitter(runtime, emitter_index));
        }
        environment = active_pfx_environment();
        environment->spawn_tables = 0;
    }
}

/* Soft ceiling: reset_effect ~75.68% - split saves and load scheduling only. */
void reset_effect(const char* name) {
    PfxScriptEffect* effect;
    int emitter_index;
    PfxVm* runtime;
    PfxVmEmitter* emitter;
    int field_index;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 0;
        runtime = (PfxVm*)effect->emitters;
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = pfx_get_emitter(runtime, emitter_index);
            emitter->flags.bits.cycle_paused = 1;
            emitter->cycle_index = 0;
            pfx_emitter_reset(emitter);
        }

        if (runtime->parametric != 0) {
            memset(
                runtime->parametric + 1, 0,
                runtime->parametric->particle_capacity *
                    sizeof(PfxParametricParticle));
        } else {
            for (field_index = 0;
                 field_index < runtime->behavior_count;
                 field_index++) {
                runtime->behavior_list[field_index]->particle_count = 0;
            }
        }

        runtime->particle_cursor = 0;
        runtime->elapsed_time = 0.0f;
    }
}

/* Soft ceiling: reset_effect_ppfx ~79.30% - split nonvolatile saves only. */
void reset_effect_ppfx(PfxScriptEffect* effect) {
    PfxVm* runtime;
    PfxVmEmitter* emitter;
    int emitter_index;
    int field_index;

    runtime = (PfxVm*)effect->emitters;
    effect->lifecycle_flags.bits.restart_cycle = 0;
    for (emitter_index = 0;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        emitter = pfx_get_emitter(runtime, emitter_index);
        emitter->flags.bits.cycle_paused = 1;
        emitter->cycle_index = 0;
        pfx_emitter_reset(emitter);
    }

    if (runtime->parametric != 0) {
        memset(
            runtime->parametric + 1, 0,
            runtime->parametric->particle_capacity *
                sizeof(PfxParametricParticle));
    } else {
        for (field_index = 0;
             field_index < runtime->behavior_count;
             field_index++) {
            runtime->behavior_list[field_index]->particle_count = 0;
        }
    }

    runtime->particle_cursor = 0;
    runtime->elapsed_time = 0.0f;
}

void fx_reset(unsigned int handle) {
    PfxResolvedHandle resolved;
    PfxScriptEffect* effect;
    PfxVm* runtime;
    int index;

    resolve_pfx_handle(handle, &resolved);
    effect = resolved.effect;
    if (effect == 0) {
        return;
    }

    runtime = (PfxVm*)effect->emitters;
    fx_reset_emit(handle);
    if (runtime->parametric != 0) {
        memset(
            runtime->parametric + 1, 0,
            runtime->parametric->particle_capacity *
                sizeof(PfxParametricParticle));
    } else {
        for (index = 0; index < runtime->behavior_count; index++) {
            runtime->behavior_list[index]->particle_count = 0;
        }
    }

    runtime->particle_cursor = 0;
    runtime->elapsed_time = 0.0f;
}

static inline PfxVmEmitter* emitter_from_handle(unsigned int handle) {
    PfxScriptEffect* effect;
    PfxVm* runtime;
    unsigned int emitter_index;

    effect = resolve_effect_handle(handle);
    if (effect == 0) {
        return 0;
    }

    runtime = (PfxVm*)effect->emitters;
    emitter_index = (handle >> 16) & 0xF;
    if (emitter_index >= (unsigned int)runtime->emitter_count) {
        return 0;
    }
    return &runtime->emitters[emitter_index];
}

/* Soft ceiling: 75.55% - four-instruction inline lookup branch residue. */
void fx_restart_emit(unsigned int handle) {
    PfxVmEmitter* emitter;
    PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter->flags.bits.cycle_paused = 0;
        emitter->cycle_index = 0;
        pfx_emitter_restart_cycle(emitter);
    }
}

/* Soft ceiling: 75.06% - three-instruction inline lookup branch residue. */
void fx_reset_emit(unsigned int handle) {
    PfxVmEmitter* emitter;

    emitter = emitter_from_handle(handle);
    /* Refresh the validated effect latch before mutating its emitter. */
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 1;
        emitter->cycle_index = 0;
        pfx_emitter_reset(emitter);
    }
}

void restart_effect(const char* name) {
    PfxScriptEffect* effect;
    PfxVmEmitter* emitter;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        emitter = effect->emitter;
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter->flags.bits.cycle_paused = 0;
        emitter->cycle_index = 0;
        pfx_emitter_restart_cycle(emitter);
    }
}

void restart_effect_ppfx(PfxScriptEffect* effect) {
    PfxVmEmitter* emitter = effect->emitter;

    effect->lifecycle_flags.bits.restart_cycle = 1;
    emitter->flags.bits.cycle_paused = 0;
    emitter->cycle_index = 0;
    pfx_emitter_restart_cycle(emitter);
}

void resume_effect(const char* name) {
    PfxScriptEffect* effect;
    PfxVmEmitter* emitter;
    int emitter_index;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter_index = 0;
        emitter = pfx_get_emitter((PfxVm*)effect->emitters, emitter_index);
        emitter->flags.bits.cycle_paused = emitter_index;
    }
}

/* Soft ceiling: 73.06% - three-instruction inline lookup branch residue. */
void fx_pause_emit(unsigned int handle) {
    PfxVmEmitter* emitter;

    emitter = emitter_from_handle(handle);
    /* Refresh the validated effect latch before mutating its emitter. */
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 1;
    }
}

/* Soft ceiling: 76.37% - three-instruction inline lookup branch residue. */
void fx_resume_emit(unsigned int handle) {
    PfxVmEmitter* emitter;
    PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 0;
        effect->lifecycle_flags.bits.restart_cycle = 1;
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
    PfxVmEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->birth_limit = enabled;
    }
}

void set_cycle_length(float length, float position) {
    PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_length = length;
        emitter->cycle_length_variation = position;
    }
}

/* Soft ceiling: 76.23% - exact table-slot setup, one-instruction residue. */
void spawn_random_size(const float* table) {
    PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;
    PfxSpawnTable* copied_table;
    unsigned int row_count;
    unsigned int value_size;
    int slot_index;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter == 0 || g_pfx_cmo == 0) {
        return;
    }

    row_count = get_row_count_for_table_by_pointer(
        g_pfx_cmo, (void*)table);
    if (row_count == 0U) {
        return;
    }

    value_size = row_count * sizeof(float);
    copied_table = get_mem(sizeof(*copied_table) + value_size);
    copied_table->values = copied_table + 1;
    copied_table->count = row_count;
    copied_table->type = 3;
    memcpy(copied_table->values, table, value_size);

    environment = active_pfx_environment();
    for (slot_index = 0; slot_index < 2; slot_index++) {
        if (environment->spawn_tables[slot_index].type == 0) {
            environment->spawn_tables[slot_index].type = 3;
            environment->spawn_tables[slot_index].table = copied_table;
            environment->spawn_tables[slot_index].row_count = row_count;
            break;
        }
    }

    environment = active_pfx_environment();
    pfxvm_spawn_set_field_from_table(
        emitter, environment->field04 != 0 ? 0x402 : 0x102,
        copied_table);
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

void set_rotation(float angle, float variance) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
    if (environment != 0) {
        effect = environment->effect;
        if (effect != 0) {
            emitter = pfx_get_emitter((PfxVm*)effect, 0);
            pfxvm_spawn_line_1f(emitter, 0, angle - variance,
                                angle + variance);
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
    RwRaster* texture;
    float width;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture != 0) {
            texture = effect->texture->raster;
            width = (float)texture->width;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                (PfxVm*)effect, speed, (int)width,
                (int)(horizontal_scale * width),
                (int)(vertical_scale * (float)texture->height),
                vertical_frames);
            effect->texture_animation_enabled = 1;
        }
    }
}

/* Soft ceiling: texture_animation -- typed texture metadata/layout. */
void texture_animation(int vertical_frames, float horizontal_scale, float speed) {
    PfxScriptEnvironment* environment;
    PfxScriptEffect* effect;
    RwRaster* texture;
    float width;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture != 0) {
            texture = effect->texture->raster;
            width = (float)texture->width;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                (PfxVm*)effect, speed, (int)width,
                (int)(horizontal_scale * width),
                (int)((float)texture->height /
                      (horizontal_scale * (float)vertical_frames)),
                vertical_frames);
            effect->texture_animation_enabled = 1;
        }
    }
}

void spawn_color(int field, int red, int green, int blue, int alpha) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            environment->emitter, field, (float)red, (float)green,
            (float)blue, (float)alpha);
    }
}

void emission_duration(float duration) {
    PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_length = duration;
        emitter->cycle_length_variation = 0.0f;
        emitter->cycle_limit = 1;
    }
}

void emit_cylindrical(
    int field, float x, float y, float z, float radius,
    float height, float start, float end) {
    PfxScriptEnvironment* environment = 0;
    PfxVec3 axis;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        axis.x = x;
        axis.y = y;
        axis.z = z;
        pfxvm_spawn_cylinder(
            environment->emitter, field, &axis,
            radius, height, start, end);
    }
}

void emit_cartesian(
    int field, float x, float y, float z,
    float width, float height, float depth) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_box(
            environment->emitter, field,
            -(width * 0.5f - x), -(height * 0.5f - y),
            -(depth * 0.5f - z), width, height, depth);
    }
}

void emit_disc2(
    int field, float x, float y, float z,
    float inner_radius, float outer_radius) {
    PfxScriptEnvironment* environment = 0;
    PfxVec3 axis;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        axis.x = x;
        axis.y = y;
        axis.z = z;
        pfxvm_spawn_disc(
            environment->emitter, field, &axis,
            inner_radius, outer_radius);
    }
}

void emit_disc(
    int field, float x, float y, float z, float outer_radius) {
    PfxScriptEnvironment* environment = 0;
    PfxVec3 axis;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        axis.x = x;
        axis.y = y;
        axis.z = z;
        pfxvm_spawn_disc(
            environment->emitter, field, &axis, 0.0f, outer_radius);
    }
}

void emit_spherical_section(int field, float x, float y, float z,
                            float radius, float radius_spread,
                            float angle, float angle_spread) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere_section(
            environment->emitter, field, x, y, z, radius,
            radius_spread, angle, angle_spread);
    }
}

void emit_from_pos_clamp_y(int field, int source, float x, float y, float z,
                           float minimum_length, float length_range,
                           float clamped_y) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(
            environment->emitter, field, source, 1,
            x, y, z, minimum_length, length_range, clamped_y);
    }
}

void emit_from_pos(int field, int source, float x, float y, float z,
                   float minimum_length, float length_range) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(
            environment->emitter, field, source, 0,
            x, y, z, minimum_length, length_range, 0.0f);
    }
}

void emit_spherical_from_boundary(int field, float radius) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(
            environment->emitter, field, 0.0f, 0.0f, 0.0f,
            radius, radius, 0);
    }
}

void emit_spherical(int field, float radius) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(
            environment->emitter, field, 0.0f, 0.0f, 0.0f,
            0.00001f, radius, 0);
    }
}

void emit_cuboid(int field, float x, float y, float z) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment != 0 && environment->emitter != 0) {
        pfxvm_spawn_box(
            environment->emitter, field,
            -x * 0.5f, -y * 0.5f, -z * 0.5f, x, y, z);
    }
}

void emit_from_point(float x, float y, float z) {
    PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->position.x = x;
        emitter->position.y = y;
        emitter->position.z = z;
    }
}

void emit_color(int field, int red, int green, int blue, int alpha) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            environment->emitter, field, (float)red, (float)green,
            (float)blue, (float)alpha);
    }
}

void emit_uv(int field, float u, float v) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_uv(environment->emitter, field, u, v);
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
        pfxvm_spawn_line_1f(environment->emitter, unused,
                            center - half_width, center + half_width);
    }
}

void emit_value(int field, float value) {
    PfxScriptEnvironment* environment;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_value(environment->emitter, field, value);
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
    PfxVmEmitter* emitter;

    environment = 0;
    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->flags.bits.constant_rate = 1;
    }
}

void emit_roundrobin_mechanism(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_roundrobin_mechanism(
            environment->emitter, field, source);
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

void parametric_update(unsigned int effect) {
    if (effect != 0U) {
        build_parametric_effect_from_table(
            active_cmdscript->mko, effect, 1);
    }
}

/* Soft ceiling: 48.26% - validated-latch branches differ by four instructions. */
void unload_all_effect_banks(void) {
    int index;
    int remaining;

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
    index = 0;
    remaining = 15;
    do {
        banks[index].bank = 0;
        banks[index].bank_instance = 0;
        index++;
        remaining--;
    } while (remaining != 0);
}

int load_effect_bank(char* name) {
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
    load_effect_bank_with_context(name, load_context);
    return 0xDEADBABE;
}

typedef struct PfxLoadScriptLatch {
    CmdScript* command;
    ScriptSlot* script;
} PfxLoadScriptLatch;

static inline void pfx_cleanup_load_script(PfxLoadScriptLatch* latch) {
    if (latch->command == 0) {
        memset(latch, 0, sizeof(*latch));
    } else {
        if (latch->command->instance != 0) {
            ((MkHdr*)latch->command)->typed_vtbl->destroy(
                (MkHdr*)latch->command);
        }
        latch->command = 0;
        cmdscript_unload(latch->script);
        latch->script = 0;
    }
}

/*
 * Soft ceiling: retail expands the typed command/script cleanup latch at each
 * exit. The residual is register allocation and branch sharing after inlining;
 * bank-handle validation and retail failure-path ownership are recovered.
 */
void load_effect_bank_with_context(char* name, LoadBgndCtx* context) {
    PfxLoadScriptLatch load;
    CmdScript* command;
    CmdScript* saved_command;
    ScriptSlot* script;
    PfxBankLoadRow* rows;
    PfxBank* bank;
    PfxBank* raw_bank;
    PfxScriptEffect* built_effect;
    MkObj* parent;
    unsigned int row_count;
    unsigned int row_index;
    unsigned int bank_handle;
    unsigned int generation;
    int total_effects;
    int remaining_effects;
    int bank_index;
    int language;
    unsigned int owner;
    unsigned int allocation_size;
    PfxResolvedHandle resolved;

    memset(&load, 0, sizeof(load));
    command = alloc_cmdscript();
    load.command = command;
    if (command == 0) {
        pfx_cleanup_load_script(&load);
        return;
    }
    script = 0;
    load.script = 0;

    switch (context->art_id) {
    case 0x3000B:
        language = 0x10;
        owner = 1;
        parent = g_game_info.plyr0.slot.mirror_a;
        break;
    case 0x4000B:
        language = 0x11;
        owner = 2;
        parent = g_game_info.plyr1.slot.mirror_a;
        break;
    case 0xD003C:
        language = 0x11;
        owner = 4;
        parent = g_game_info.bgnd_obj;
        break;
    case 0x140064:
    case 0x8003D:
    case 0x2001E:
        language = 0x12;
        owner = 4;
        parent = g_game_info.bgnd_obj;
        break;
    case 0x90046:
        language = 0x12;
        owner = 8;
        parent = 0;
        break;
    case 0x60030:
    case 0x60029:
        language = 0x12;
        owner = 4;
        parent = context->bgnd_obj;
        break;
    case 0x70036:
        language = 0x12;
        owner = 4;
        parent = g_game_info.plyr0.slot.mirror_a;
        break;
    case 0x70038:
        language = 0x13;
        owner = 4;
        parent = g_game_info.plyr0.slot.mirror_a;
        break;
    default:
        pfx_cleanup_load_script(&load);
        return;
    }

    if (parent == 0 && owner != 8) {
        pfx_cleanup_load_script(&load);
        return;
    }

    script = cmdscript_loadfile_by_name(language, name);
    load.script = script;
    command->mko = script;
    script->load_ctx = context;
    rows = (PfxBankLoadRow*)get_data_table(script, script->table_count);
    if (rows == 0) {
        pfx_cleanup_load_script(&load);
        return;
    }

    behavior_buffer = get_mem(10 * sizeof(*behavior_buffer));
    row_count = get_row_count_for_table(script, script->table_count);
    total_effects = 0;
    for (row_index = 0; row_index < row_count; row_index++) {
        total_effects += rows[row_index].effect_count;
    }

    bank_index = 0;
    while (bank_index < 15) {
        raw_bank = banks[bank_index].bank;
        if (raw_bank == 0 ||
            raw_bank->hdr.instance != banks[bank_index].bank_instance) {
            break;
        }
        bank_index++;
    }
    bank = 0;
    bank_handle = 0;
    if (bank_index < 15) {
        allocation_size = sizeof(*bank) +
                          total_effects * sizeof(PfxEffectLatch) +
                          total_effects * sizeof(unsigned int) +
                          strlen(name) + 1;
        bank = (PfxBank*)get_mkhdr(&vtbl_effectbank, allocation_size);
        if (bank != 0) {
            bank->effects = (PfxEffectLatch*)(bank + 1);
            bank->effect_owners =
                (unsigned int*)(bank->effects + total_effects);
            bank->name = (char*)(bank->effect_owners + total_effects);
            bank->effect_count = total_effects;
            bank->effect_capacity = 0;
            strcpy(bank->name, name);
            memset(
                bank->effects, 0,
                total_effects * sizeof(PfxEffectLatch));
            memset(
                bank->effect_owners, 0,
                total_effects * sizeof(unsigned int));

            banks[bank_index].bank = bank;
            banks[bank_index].bank_instance = bank->hdr.instance;
            bank->handle_bank = bank_index + 1;
            generation =
                (bank_instance_counter[bank_index] + 1) & 0xFF000000;
            bank_instance_counter[bank_index] = generation;
            bank->handle_generation = generation;
            bank_handle =
                (banks[bank_index].bank_instance & 0xFFFFFFF0) |
                ((bank_index + 1) & 0xF);
        }
    }

    resolve_pfx_handle(bank_handle, &resolved);
    current_effect_bank = resolved.bank;
    if (current_effect_bank == 0) {
        pfx_cleanup_load_script(&load);
        return;
    }

    if (parent != 0) {
        mk_insert(&bank->hdr, &parent->child_list);
    } else {
        mk_insert(&bank->hdr, &aproc->pdata_list);
    }
    bank->owner_flags = owner;

    for (row_index = 0; row_index < row_count; row_index++) {
        saved_command = active_cmdscript;
        active_cmdscript = command;
        script->load_ctx = context;
        remaining_effects = rows[row_index].effect_count;
        if (pfxscript_environment.active == 0) {
            memset(
                &pfxscript_environment, 0,
                sizeof(pfxscript_environment));
            pfxscript_environment.active = 1;
        }
        pfxscript_environment.remaining_effects = &remaining_effects;
        cmdscript_setup_execution(script, rows[row_index].function_index);
        cmdscript_execute(script);
        built_effect = 0;
        if (pfxscript_environment.active != 0) {
            built_effect = pfxscript_environment.effect;
            memset(
                &pfxscript_environment, 0,
                sizeof(pfxscript_environment));
        }
        active_cmdscript = saved_command;
        if (built_effect != 0 && built_effect->load_metrics != 0) {
            pfxmetrics_init(
                built_effect->load_metrics,
                built_effect->metrics_name);
        }
    }

    current_effect_bank = 0;
    free_mem(behavior_buffer);
    pfx_cleanup_load_script(&load);
}

/*
 * Soft ceiling: retail m2c confirms the complete parametric build pipeline.
 * Source is 12 bytes smaller because retail preserves redundant stack-slot
 * initialization that clean typed C folds; remaining records are large-frame
 * register allocation, scheduling, bitfield emission, and relocations.
 */
static void build_parametric_effect_from_table(
    ScriptSlot* script, unsigned int effect_table, int update) {
    const PfxParametricEffectDescription* description;
    PfxParametricEmitterDescription* emitter_description;
    PfxScriptEnvironment* environment;
    PfxSpawnTableSlot table_slots[2];
    PfxBuildInfo build;
    PfxScriptEffect* effect;
    PfxVm* runtime;
    PfxVmEmitter emitter_template;
    PfxVmEmitter* emitter;
    PfxVmEmitter* first_emitter;
    PfxParametricState* data;
    unsigned int scan_fields[2] = { 2, 0 };
    unsigned int create_flags;
    unsigned int row_count;
    unsigned int row;
    int emitter_index;
    int matrix_row;
    int matrix_column;
    void* transform;

    description = (const PfxParametricEffectDescription*)effect_table;
    environment = active_pfx_environment();
    if (environment->remaining_effects == 0 ||
        *environment->remaining_effects == 0) {
        return;
    }

    memset(table_slots, 0, sizeof(table_slots));
    environment->spawn_tables = table_slots;
    memset(&build, 0, sizeof(build));
    memset(&emitter_template, 0, sizeof(emitter_template));
    build.name = environment->effect_name_override;
    if (build.name == 0) {
        build.name = description->effect_name;
    }
    build.emitter_count = update;
    if (g_profile_enabled != 0) {
        build.metrics_frame_count = 0x384;
    }

    emitter_description = description->emitter;
    if (emitter_description->origin.x != 0.0f ||
        emitter_description->origin.y != 0.0f ||
        emitter_description->origin.z != 0.0f) {
        build.flags = 0x80000000;
    }

    create_flags = 2;
    if (description->color_table != 0 &&
        get_row_count_for_table_by_pointer(
            script, description->color_table) > 1U) {
        create_flags |= 0x10;
    }
    if (description->table_a != 0) {
        create_flags |= 0x20;
    }
    if (description->table_b != 0) {
        create_flags |= 0x40;
    }

    environment->effect = 0;
    environment->behavior = 0;
    environment->fields30[0] = 0.0f;
    environment->fields30[1] = 0.0f;
    environment->texture_name = description->texture_name;
    environment->initialization_script =
        emitter_description->initialization_script;
    if (environment->initialization_script == 0U) {
        return;
    }

    g_pfx_cmo = script;
    environment->field04 = 1;
    environment->emitter = 0;
    effect = 0;
    new_pfx_create_raw_userdata(
        &build, 0, (int)(1.1f * description->allocation_count),
        create_flags, 0, (PfxInitCb)initialize_effect, 0, 0,
        (void**)&effect);
    if (effect == 0) {
        g_pfx_cmo = 0;
        return;
    }

    runtime = (PfxVm*)effect->emitters;
    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = pfx_get_emitter(runtime, emitter_index);
            transform = emitter->transform;
            memcpy(emitter, environment->emitter, sizeof(*emitter));
            emitter->transform = transform;
        }
        environment->emitter = runtime->emitters;
    }
    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        if (table_slots[emitter_index].type != 0) {
            pfx_register_table(
                (PfxTableRegistry*)runtime, emitter_index,
                table_slots[emitter_index].table);
        }
    }
    g_pfx_cmo = 0;

    emitter = pfx_get_emitter(runtime, 0);
    first_emitter = emitter;
    pfx_emitter_scan_for_fields(emitter, scan_fields);
    if ((scan_fields[0] & 0x40) != 0) {
        effect->parametric_flags.bits.scan_flag_40 = 1;
    }
    if ((scan_fields[0] & 0x20) != 0) {
        effect->parametric_flags.bits.scan_flag_20 = 1;
    }
    pfxvm_compile(runtime);

    emitter = pfx_get_emitter(runtime, 0);
    first_emitter = emitter;
    emitter->position = emitter_description->origin;
    transform = emitter->transform;
    if (transform != 0 &&
        (emitter_description->origin.x != 0.0f ||
         emitter_description->origin.y != 0.0f ||
         emitter_description->origin.z != 0.0f)) {
        float* matrix = transform;

        memset(matrix, 0, 16 * sizeof(*matrix));
        matrix[0] = 1.0f;
        matrix[5] = 1.0f;
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        matrix[12] = emitter_description->origin.x;
        matrix[13] = emitter_description->origin.y;
        matrix[14] = emitter_description->origin.z;
    }

    effect->effect_value = description->effect_value;
    effect->effect_id = description->effect_id;
    if (current_effect_bank->owner_flags == 8 && environment->source_effect != 0) {
        environment->source_effect->flags |= 8;
    }
    for (emitter_index = 0;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        emitter = pfx_get_emitter(runtime, emitter_index);
        emitter->flags.bits.cycle_paused = 1;
    }
    effect->lifecycle_flags.bits.restart_cycle = 0;
    environment->source_effect = (MkPfx*)effect;

    data = runtime->parametric;
    if (data != 0) {
        data->acceleration.x = 0.0f;
        data->acceleration.y = 0.0f;
        data->acceleration.z = 0.0f;
        data->vertical_acceleration = description->field2C;
        data->lifetime = description->field34;
        first_emitter->lifetime = description->emitter_lifetime;
        for (emitter_index = 1;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = pfx_get_emitter(runtime, emitter_index);
            transform = emitter->transform;
            memcpy(emitter, first_emitter, sizeof(*emitter));
            emitter->transform = transform;
        }

        if (description->color_table != 0) {
            row_count = get_row_count_for_table_by_pointer(
                script, description->color_table);
            if (row_count > 1U) {
                data->color_curve_count = row_count;
                for (row = 0; row < row_count; row++) {
                    data->color_curve[row].r =
                        description->color_table[row].red;
                    data->color_curve[row].g =
                        description->color_table[row].green;
                    data->color_curve[row].b =
                        description->color_table[row].blue;
                    data->color_curve[row].a =
                        description->color_table[row].alpha;
                }
            } else {
                effect->vertex_color.r = description->color_table[0].red;
                effect->vertex_color.g = description->color_table[0].green;
                effect->vertex_color.b = description->color_table[0].blue;
                effect->vertex_color.a = description->color_table[0].alpha;
                effect->flags.bits.vertex_color_enabled = 1;
            }
        }
        if (description->table_a != 0) {
            data->texture_curve_count = get_row_count_for_table_by_pointer(
                script, description->table_a);
            for (row = 0;
                 row < (unsigned int)data->texture_curve_count;
                 row++) {
                data->texture_curve[row] = description->table_a[row];
            }
        }
        if (description->table_b != 0) {
            data->size_curve_count = get_row_count_for_table_by_pointer(
                script, description->table_b);
            for (row = 0; row < (unsigned int)data->size_curve_count; row++) {
                data->size_curve[row] = description->table_b[row];
            }
        }
        transform = first_emitter->transform;
        if (transform != 0) {
            float* matrix = transform;

            for (matrix_row = 0; matrix_row < 4; matrix_row++) {
                for (matrix_column = 0; matrix_column < 4; matrix_column++) {
                    matrix[matrix_row * 4 + matrix_column] =
                        matrix_row == matrix_column ? 1.0f : 0.0f;
                }
            }
            matrix[12] = emitter_description->origin.x;
            matrix[13] = emitter_description->origin.y;
            matrix[14] = emitter_description->origin.z;
        }
        data->acceleration.x = description->field1C;
        data->acceleration.y = description->field20;
        data->acceleration.z = description->field24;
        data->damping = environment->fields30[0];
        data->texture_rate = environment->fields30[1];
    }
    pfx_render_set_blendmode(
        (struct PfxRenderView*)runtime, description->blend_mode);

    if (pfx_verify((struct PfxVerifyView*)runtime) != 0) {
        (*environment->remaining_effects)--;
        if (current_effect_bank->effect_capacity <
            current_effect_bank->effect_count) {
            int index = current_effect_bank->effect_capacity;
            current_effect_bank->effects[index].effect = effect;
            current_effect_bank->effects[index].effect_instance =
                effect->hdr.instance;
            current_effect_bank->effect_owners[index] = 0;
            current_effect_bank->effect_capacity++;
        }
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            pfx_emitter_reset(pfx_get_emitter(runtime, emitter_index));
        }
        environment->spawn_tables = 0;
    }
}

MkPfx* find_pfx_by_handle(unsigned int handle) {
    PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
}

/* Retail emits both public lookup wrappers out of line and byte-exact. */
#pragma dont_inline on
MkPfx* find_pfx_by_name_by_bankowner(
    const char* name, unsigned int owner) {
    PfxResolvedHandle resolved;
    unsigned int handle;

    handle = fx_by_owner(name, owner);
    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
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
    PfxVmEmitter* emitter;
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
        effect->texture = load_named_tga_from_slot(
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
        emitter = pfx_get_emitter((PfxVm*)effect, 0);
        script = g_pfx_cmo;
        emitter->birth_rate = parametric_birthrate;
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

static float p_update_effects(void) {
    int index;

    for (index = 0; index < 15; index++) {
        PfxBank* bank;

        bank = banks[index].bank;
        if (bank != 0 &&
            bank->hdr.instance != banks[index].bank_instance) {
            bank = 0;
        }
        if (bank != 0) {
            bank_run_fx(bank);
        }
    }
    return 0.0f;
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
            if (bank->hdr.instance != 0U) {
                PfxBankVtablePrefix* vtbl;

                vtbl = (PfxBankVtablePrefix*)bank->hdr.vtbl;
                vtbl->destroy(&bank->hdr);
            }
            banks[index].bank = 0;
            banks[index].bank_instance = 0;
        }
    }
}

static inline void bank_destroy(MkHdr* bank) {
    PfxBankVtablePrefix* vtbl;

    if (bank->instance != 0U) {
        vtbl = (PfxBankVtablePrefix*)bank->vtbl;
        vtbl->destroy(bank);
    }
}

/* Soft ceiling: 91.81% - exact runtime loop, one-instruction residue. */
static void bank_run_fx(PfxBank* bank) {
    PfxEffectLatch* effect_latch;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    PfxVm* runtime;
    PfxResolvedHandle transfer;
    PfxVmEmitter* emitter;
    int effect_index;
    int emitter_index;

    for (effect_index = 0;
         effect_index < bank->effect_capacity;
         effect_index++) {
        effect_latch = &bank->effects[effect_index];
        raw_effect = effect_latch->effect;
        if (raw_effect != 0 &&
            raw_effect->hdr.instance == effect_latch->effect_instance) {
            effect = raw_effect;
        } else {
            effect = 0;
        }
        if (effect == 0 || !effect->lifecycle_flags.bits.restart_cycle) {
            continue;
        }

        runtime = (PfxVm*)effect->emitters;
        if (effect->parametric != 0) {
            runtime->elapsed_time += game_speed;
            if (pfx_frame_begin(runtime) == 0) {
                pfx_parametric_spawn(runtime, game_speed);
                pfx_parametric_update(runtime, game_speed);
            } else {
                runtime->particle_cursor = 0;
            }
            pfx_frame_end(runtime);
        } else {
            pfx_run(runtime, game_speed);
            if (bank->effect_owners[effect_index] != 0) {
                resolve_pfx_handle(
                    bank->effect_owners[effect_index], &transfer);
                if (transfer.effect != 0) {
                    pfxvm_create_transfer(
                        (PfxVm*)transfer.effect->emitters,
                        runtime);
                    transfer.effect->lifecycle_flags.bits.restart_cycle = 1;
                }
            }
        }

        if (runtime->particle_cursor == 0) {
            for (emitter_index = 0;
                 emitter_index < runtime->emitter_count;
                 emitter_index++) {
                emitter = pfx_get_emitter(runtime, emitter_index);
                if (!pfx_emitter_exhausted(emitter) &&
                    !pfx_emitter_unused(emitter)) {
                    break;
                }
            }
            if (emitter_index == runtime->emitter_count) {
                effect->lifecycle_flags.bits.restart_cycle = 0;
            }
        }
        pfxmetrics_event(runtime->metrics, 0x2000);
    }
}

/* Soft ceiling: 74.91% - exact owned-effect search, four-instruction residue. */
static unsigned int banks_find_owned_fx(
    const char* name, unsigned int owner) {
    PfxBankLatch* bank_latch;
    PfxEffectLatch* effect_latch;
    PfxBank* raw_bank;
    PfxBank* bank;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    unsigned int handle;
    int bank_index;
    int effect_index;

    for (bank_index = 0; bank_index < 15; bank_index++) {
        bank_latch = &banks[bank_index];
        raw_bank = bank_latch->bank;
        if (raw_bank != 0 &&
            raw_bank->hdr.instance == bank_latch->bank_instance) {
            bank = raw_bank;
        } else {
            bank = 0;
        }
        if (bank != 0 && (bank->owner_flags & owner) != 0) {
            handle = 0;
            for (effect_index = 0;
                 effect_index < bank->effect_count;
                 effect_index++) {
                effect_latch = &bank->effects[effect_index];
                raw_effect = effect_latch->effect;
                if (raw_effect != 0 &&
                    raw_effect->hdr.instance == effect_latch->effect_instance) {
                    effect = raw_effect;
                } else {
                    effect = 0;
                }
                if (effect != 0 && effect->effect_name != 0 &&
                    strcmp(name, effect->effect_name) == 0) {
                    handle = ((effect_index << 4) & 0x3FF0) |
                             (bank->handle_bank & 0xF) | 0x4000 |
                             (bank->handle_generation << 24);
                    break;
                }
            }
            if (handle != 0) {
                return handle;
            }
        }
    }
    return 0;
}

/* Soft ceiling: 71.43% - exact size and algorithm; register allocation only. */
static void vdestroy_effectbank(PfxBank* bank) {
    PfxEffectLatch* effect_latch;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    int effect_index;

    for (effect_index = 0; effect_index < bank->effect_count; effect_index++) {
        bank->effect_owners[effect_index] = 0;
        effect_latch = &bank->effects[effect_index];
        raw_effect = effect_latch->effect;
        if (raw_effect != 0 &&
            raw_effect->hdr.instance == effect_latch->effect_instance) {
            effect = raw_effect;
        } else {
            effect = 0;
        }
        if (effect != 0 && effect->hdr.vtbl == &vtbl_pfx &&
            effect->hdr.instance != 0) {
            effect->hdr.typed_vtbl->destroy(&effect->hdr);
        }
    }
    bank->hdr.instance = 0;
    mkhdr_memfree(&bank->hdr);
}

static void resolve_pfx_handle(
    unsigned int handle, PfxResolvedHandle* resolved) {
    PfxBankLatch* bank_latch;
    PfxEffectLatch* effect_latch;
    PfxBank* raw_bank;
    PfxBank* bank;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    int bank_index;
    int effect_index;
    int handle_type;

    if (cached_handle == handle) {
        memcpy(resolved, &cached_info, sizeof(cached_info));
        return;
    }

    bank_index = handle & 0xF;
    effect_index = (handle >> 4) & 0x3FF;
    handle_type = (handle >> 14) & 3;
    if (handle_type != 1 || bank_index < 1 || bank_index > 15 ||
        effect_index < 0) {
        return;
    }

    bank_latch = &banks[bank_index - 1];
    raw_bank = bank_latch->bank;
    if (raw_bank != 0 &&
        raw_bank->hdr.instance == bank_latch->bank_instance) {
        bank = raw_bank;
    } else {
        bank = 0;
    }
    if (bank == 0 || bank->handle_bank != (unsigned int)bank_index ||
        (handle >> 24) != bank->handle_generation ||
        effect_index >= bank->effect_capacity) {
        return;
    }

    effect_latch = &bank->effects[effect_index];
    raw_effect = effect_latch->effect;
    if (raw_effect != 0 &&
        raw_effect->hdr.instance == effect_latch->effect_instance) {
        effect = raw_effect;
    } else {
        effect = 0;
    }
    if (effect != 0) {
        cached_handle = handle;
        cached_info.bank = bank;
        cached_info.effect_index = effect_index;
        cached_info.effect = effect;
        memcpy(resolved, &cached_info, sizeof(cached_info));
    }
}
