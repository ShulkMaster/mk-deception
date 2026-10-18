#include "runtime/mk_cmdscript.h"
#include "game/pfxscript.h"
#include "runtime/asset.h"
#include "runtime/cstring.h"
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

struct PfxSpawnTableSlot {
    int type;
    int row_count;
    PfxSpawnTable* table;
};

union PfxScriptRowState {
    float texture_rate;
    int behavior_count;
};

struct PfxScriptEnvironment {
    int active;
    int field04;
    MkPfx* source_effect;
    struct PfxScriptVm* effect;
    PfxVmEmitter* emitter;
    int* remaining_effects;
    struct PfxSpawnTableSlot* spawn_tables;
    char* effect_name_override;
    char* texture_name;
    unsigned int initialization_script;
    float drag_coefficient;
    float growth_coefficient;
    float damping;
    union PfxScriptRowState row_state;
    PfxBehavior* behavior;
    PfxBehavior* next_behavior;
};

struct PfxScriptEffectFlags {
    unsigned char vertex_color_enabled : 1;
    unsigned char particle_size_enabled : 1;
    unsigned char textured : 1;
    unsigned char decal_plane_enabled : 1;
    unsigned char light_enabled : 1;
    unsigned char light_mode : 1;
    unsigned char pad_bits1_0 : 2;
};

struct PfxRenderFlags {
    unsigned char pad_bit7 : 1;
    unsigned char custom_bounding_radius : 1;
    unsigned char pad_bits5_0 : 6;
};

struct PfxOrientationFlags {
    unsigned char face_y : 1;
    unsigned char pad_bits6_5 : 2;
    unsigned char rotation_enabled : 1;
    unsigned char pad_bits3_0 : 4;
};

struct PfxLifecycleFlags {
    unsigned char pad_bits7_5 : 3;
    unsigned char restart_cycle : 1;
    unsigned char owner_special : 1;
    unsigned char pad_bits2_0 : 3;
};

struct PfxHideFlags {
    unsigned char hidden : 1;
    unsigned char pad : 7;
};

struct PfxZTestFlags {
    unsigned char pad : 7;
    unsigned char disabled : 1;
};

struct PfxParametricFlags {
    unsigned char pad_bit7 : 1;
    unsigned char scan_flag_40 : 1;
    unsigned char scan_flag_20 : 1;
    unsigned char pad_bits4_0 : 5;
};

struct PfxScriptVm {
    char pad00[0x40];
    struct PfxRenderFlags render_flags;
    char pad41[0x10F];
    struct PfxScriptEffectFlags flags;
    struct PfxOrientationFlags orientation_flags;
    char pad152[2];
    float light_direction_components[3];
    PfxColor light_color;
    Vec light_position;
    float z_bias;
    RwTexture* texture;
    char pad178[0x0A];
    short texture_animation_enabled;
    char pad184[0x10];
    float decal_plane[6];
    float aspect_x;
    float aspect_y;
    PfxColor vertex_color;
    float particle_size;
    float bounding_radius;
    char pad1C0[0x14];
    unsigned int runtime_flags;
    char pad1D8[0x44];
    const char* metrics_name;
    char pad220[4];
    PfxMetrics* load_metrics;
    float kill_plane;
    int initialization_mode;
};

struct PfxEmitterHideView {
    char pad40[0x40];
    struct PfxHideFlags hide_flags;
    char pad81[0xCF];
};

union PfxEmitterStorage {
    struct PfxRenderFlags render_flags;
    struct PfxEmitterHideView hide_view;
    unsigned char emitters[0x110];
};

struct PfxScriptEffect {
    MkHdr hdr;
    struct PfxLifecycleFlags lifecycle_flags;
    char pad09[0x1F];
    float effect_value;
    int render_priority;
    char pad30[0x10];
    union PfxEmitterStorage emitter_storage;
    struct PfxScriptEffectFlags flags;
    struct PfxOrientationFlags orientation_flags;
    char pad152[2];
    float light_direction_components[3];
    PfxColor light_color;
    Vec light_position;
    float z_bias;
    RwTexture* texture;
    char pad178[0x0A];
    short texture_animation_enabled;
    char pad184[0x0C];
    struct PfxZTestFlags ztest_flags;
    struct PfxParametricFlags parametric_flags;
    char pad192[2];
    float decal_plane[6];
    float aspect_x;
    float aspect_y;
    PfxColor vertex_color;
    float particle_size;
    float bounding_radius;
    unsigned int behavior_target;
    char pad1C4[0x10];
    unsigned int runtime_flags;
    char pad1D8[0x28];
    int emitter_count;
    PfxVmEmitter* emitter;
    char pad208[4];
    int behavior_count;
    PfxBehavior** behavior_list;
    char pad214[8];
    const char* metrics_name;
    char pad220[4];
    PfxMetrics* load_metrics;
    float kill_plane;
    int initialization_mode;
    char pad230[0x2C];
    const char* effect_name;
    int effect_id;
    PfxMetrics* metrics;
    int field268;
    int parametric;
};

struct PfxVertexColorArgs {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int alpha;
};

struct PfxLightArgs {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int alpha;
    float direction_components[3];
    int mode;
    Vec position;
};

struct PfxScriptColorRow {
    int red;
    int green;
    int blue;
    int alpha;
};

struct PfxParametricEmitterDescription {
    PfxVec3 origin;
    unsigned int initialization_script;
};

struct PfxParametricEffectDescription {
    char* effect_name;
    int effect_id;
    char* texture_name;
    struct PfxParametricEmitterDescription* emitter;
    float* table_a;
    float* table_b;
    struct PfxScriptColorRow* color_table;
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
};

struct PfxStepTextureDescription {
    char* name;
    int frame_count;
    float horizontal_scale;
    float animation_speed;
};

struct PfxStepEffectDescription {
    char* effect_name;
    int effect_id;
    struct PfxStepTextureDescription* texture;
    struct PfxParametricEmitterDescription* emitter;
    unsigned int* behavior_scripts;
    float emitter_lifetime;
    int field18;
    float allocation_count;
    int blend_mode;
    float effect_value;
};

struct PfxBankLoadRow {
    unsigned int function_index;
    int effect_count;
};

typedef void (*PfxBankDestroyFn)(MkHdr* bank);

struct PfxBankVtablePrefix {
    void* reserved[4];
    PfxBankDestroyFn destroy;
};

struct PfxBank;

struct PfxBankLatch {
    struct PfxBank* bank;
    unsigned int bank_instance;
};

struct PfxEffectLatch {
    struct PfxScriptEffect* effect;
    unsigned int effect_instance;
};

struct PfxResolvedHandle {
    struct PfxBank* bank;
    int effect_index;
    struct PfxScriptEffect* effect;
};

struct PfxBank {
    MkHdr hdr;
    int handle_bank;
    int handle_generation;
    char* name;
    int owner_flags;
    int effect_count;
    int effect_capacity;
    struct PfxEffectLatch* effects;
    unsigned int* effect_owners;
};

static void vdestroy_effectbank(struct PfxBank* bank);
static struct PfxResolvedHandle cached_info = { 0, 0, 0 };
static struct PfxScriptEnvironment pfxscript_environment = {
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
    0.0f,
    { 0.0f },
    0,
    0,
};
struct PfxBankVtable {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct PfxBank* owner);
};
typedef char check_PfxBankVtable_size[(sizeof(struct PfxBankVtable) == sizeof(MkVtable5)) ? 1 : -1];

static struct PfxBankVtable vtbl_effectbank = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_effectbank,
};
static unsigned int bank_instance_counter[15] = { 0 };
static PfxBehavior* behavior_buffer;

static inline void copy_emitter_keep_transform(
    PfxVmEmitter* emitter, PfxVmEmitter* source) {
    static void* old_ltm;

    old_ltm = emitter->transform;
    memcpy(emitter, source, sizeof(*emitter));
    emitter->transform = old_ltm;
}

static inline void clear_spawn_table_slots(struct PfxSpawnTableSlot* slots) {
    int index;

    memset(slots, 0, sizeof(struct PfxSpawnTableSlot) * 2);
    for (index = 0; index < 2; index++) {
        slots[index].type = 0;
    }
}

static int g_profile_enabled;
static float parametric_birthrate = 1.0f;
static ScriptSlot* g_pfx_cmo;
static const struct PfxParametricEffectDescription* g_effect_description;
static struct PfxBank* current_effect_bank;

static inline void scan_effect_emitter_fields(struct PfxScriptEffect** effect) {
    PfxFieldSet scan_fields = { 0, 0 };

    pfx_emitter_scan_for_fields(
        pfx_get_emitter((PfxVm*)(*effect)->emitter_storage.emitters, 0),
        &scan_fields);
    if ((scan_fields.render_flags & 0x40) != 0) {
        (*effect)->parametric_flags.scan_flag_40 = 1;
    }
    if ((scan_fields.render_flags & 0x20) != 0) {
        (*effect)->parametric_flags.scan_flag_20 = 1;
    }
}

static inline char* parametric_effect_name(
    struct PfxParametricEffectDescription* description) {
    char* name;

    name = description->effect_name;
    return name;
}

static inline void bank_record_effect(struct PfxScriptEffect* effect) {
    struct PfxBank* bank;

    bank = current_effect_bank;
    if (bank->effect_capacity < bank->effect_count) {
        bank->effects[bank->effect_capacity].effect = effect;
        bank->effects[bank->effect_capacity].effect_instance =
            effect->hdr.instance;
        bank->effect_owners[bank->effect_capacity] = 0;
        bank->effect_capacity++;
    }
}

static struct PfxBankLatch banks[15];
static int cached_handle;
static void bank_run_fx(struct PfxBank* bank);

static unsigned int banks_find_owned_fx(
    const char* name, unsigned int owner);
static void build_step_effect(
    ScriptSlot* script, struct PfxStepEffectDescription* description, int emitter_count);
static void build_parametric_effect_from_table(
    ScriptSlot* script, struct PfxParametricEffectDescription* description, int update);
void load_effect_bank_with_context(char* name, LoadBgndCtx* context);
static void resolve_pfx_handle(
    int handle, struct PfxResolvedHandle* resolved);
static void initialize_effect(struct PfxScriptVm* effect);
void texture_animation(float horizontal_scale, int vertical_frames, float speed);
static inline void bank_destroy(MkHdr* bank);
struct PfxScriptEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(struct PfxScriptEffect* effect);
static inline struct PfxScriptEffect* resolve_effect_handle(unsigned int handle) {
    struct PfxResolvedHandle resolved;
    int kind = (handle >> 14) & 3;

    if (kind != 1 && kind != 2) {
        return 0;
    }
    resolve_pfx_handle(
        (handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return resolved.effect;
}

static float p_update_effects(void);

static inline struct PfxScriptEnvironment* active_pfx_environment(void) {
    if (pfxscript_environment.active != 0) {
        return &pfxscript_environment;
    }
    return 0;
}

void fx_transfer(unsigned int handle, unsigned int owner) {
    struct PfxResolvedHandle resolved;

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
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
}

unsigned int fx_next_emitter(unsigned int handle) {
    struct PfxResolvedHandle resolved;
    struct PfxScriptEffect* effect;
    int type;
    int emitter_index;

    type = (handle >> 14) & 3;
    if (type != 1 && type != 2) {
        effect = 0;
    } else {
        resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
        effect = resolved.effect;
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
            unsigned int emitter_handle;

            pfx_emitter_reset(
                &effect->emitter[emitter_index]);
            emitter_handle = (handle & 0xFFFF3FFF) | 0x8000;
            emitter_handle &= 0xFFF0FFFF;
            emitter_handle |= (emitter_index & 0xF) << 16;
            return emitter_handle;
        }
    }
    return 0;
}

MkPfx* pfx_from_emitter(unsigned int handle) {
    struct PfxResolvedHandle resolved;
    int type;

    type = (handle >> 14) & 3;
    if (type != 1 && type != 2) {
        return 0;
    }
    resolve_pfx_handle((handle & 0xFFFF3FFF) | 0x4000, &resolved);
    return (MkPfx*)resolved.effect;
}

/* TODO: [near miss] 97.57576%; disjoint-mask scan folds generation shift/OR;
 * top-byte mask is neutral; recover the original producer boundary. */
unsigned int fx_by_id(int effect_id, unsigned int owner) {
    struct PfxBankLatch* bank_latch;
    struct PfxScriptEffect* effect;
    struct PfxBank* bank;
    int bank_index;
    int effect_index;

    for (bank_index = 0; bank_index < 15; bank_index++) {
        bank_latch = &banks[bank_index];
        bank = MK_HDR_LIVE(bank_latch->bank, bank_latch->bank_instance);

        if (bank != 0 && (bank->owner_flags & owner) != 0) {
            for (effect_index = 0;
                 effect_index < bank->effect_count;
                 effect_index++) {
                effect = MK_HDR_LIVE(bank->effects[effect_index].effect,
                                     bank->effects[effect_index].effect_instance);

                if (effect != 0 && effect->effect_id == effect_id) {
                    return (bank->handle_bank & 0xF) |
                        ((effect_index & 0x3FF) << 4) |
                        0x4000 |
                        (bank->handle_generation * 0x01000000U);
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

static inline unsigned int fx_to_handle(struct PfxBank* bank, int effect_index) {
    return (bank->handle_bank & 0xF) |
           ((effect_index & 0x3FF) << 4) | 0x4000 |
           (bank->handle_generation * 0x01000000U);
}

static inline unsigned int bank_find_fx(struct PfxBank* bank, const char* name) {
    struct PfxEffectLatch* effect_latch;
    struct PfxScriptEffect* effect;
    int effect_index;

    for (effect_index = 0; effect_index < bank->effect_count; effect_index++) {
        effect_latch = &bank->effects[effect_index];
        effect = MK_HDR_LIVE(effect_latch->effect, effect_latch->effect_instance);
        if (effect != 0 && effect->effect_name != 0 &&
            strcmp(name, effect->effect_name) == 0) {
            return fx_to_handle(bank, effect_index);
        }
    }
    return 0;
}

static inline struct PfxBank* bank_from_handle(unsigned int handle)
{
    struct PfxBank* bank;
    int bank_index = handle & 0xF;
    unsigned int generation;

    if (bank_index == 0 || bank_index > 15) {
        return 0;
    }
    bank_index--;
    bank = MK_HDR_LIVE(banks[bank_index].bank,
        banks[bank_index].bank_instance);
    if (bank == 0) {
        return 0;
    }
    generation = banks[bank_index].bank_instance;
    generation &= 0xFFFFFFF0;
    if (generation != (handle & 0xFFFFFFF0)) {
        return 0;
    }
    return bank;
}

/* TODO: [breakthrough needed] 97.11111%; initial search has one post-latch gate;
 * recover the missing predicate and pre-combine generation-packing producers. */
unsigned int fx2(unsigned int bank_handle, const char* name) {
    if (bank_handle != 0xDEADBABE) {
        return 0;
    }
    return bank_find_fx(bank_from_handle(bank_handle), name);
}

void fx_hide(unsigned int handle, int hidden) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->emitter_storage.hide_view.hide_flags.hidden = hidden;
    }
}

void fx_set(unsigned int handle, int field, float value) {
    struct PfxResolvedHandle resolved;
    float* destination;
    PfxVm* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = (PfxVm*)resolved.effect->emitter_storage.emitters;

        if ((field & 0xF00) == 0x200) {
            destination = pfx_get_field(emitters, -2, field);
            if (destination != 0) {
                *destination = value;
            }
        }
    }
}

void fx_get_v3(unsigned int handle, int field, Vec* value) {
    struct PfxResolvedHandle resolved;
    Vec* source;
    PfxVm* emitters;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        emitters = (PfxVm*)resolved.effect->emitter_storage.emitters;

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

void fx_set_param_v3(
    unsigned int handle, int parameter, float x, float y, float z) {
    struct PfxScriptEffect* effect;
    PfxVec3* target;

    target = 0;
    if ((parameter & 0xF00) == 0x200) {
        effect = resolve_effect_handle(handle);
        if (effect != 0) {
            if (parameter == 0x202) {
                PfxVm* runtime;
                PfxVmEmitter* emitter;
                int emitter_index;

                effect = resolve_effect_handle(handle);
                runtime = (PfxVm*)effect->emitter_storage.emitters;
                if (runtime == 0) {
                    emitter = 0;
                } else {
                    emitter_index = (handle >> 16) & 0xF;
                    if (emitter_index < 0 || emitter_index >= runtime->emitter_count) {
                        emitter = 0;
                    } else {
                        emitter = &runtime->emitters[emitter_index];
                    }
                }
                if (emitter != 0) {
                    target = &emitter->position;
                }
            } else {
                target = pfx_get_field(
                    (PfxVm*)effect->emitter_storage.emitters, -2, parameter);
            }
            if (target != 0) {
                target->x = x;
                target->y = y;
                target->z = z;
            }
        }
    }
}

void fx_disable_ztest(unsigned int handle, int disabled) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->ztest_flags.disabled = disabled;
    }
}

void fx_set_render_priority(unsigned int handle, int priority) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        resolved.effect->render_priority = priority;
    }
}

void create_y_mirror_effect(int field_28) {
    struct PfxScriptEnvironment* environment;
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
        render_object->flags_08_bits.scale_active = 1;
        update_mkobj(render_object != 0 ? as_mkhdr(&render_object->hdr) : 0);
    }
}

void set_vertex_color(const struct PfxVertexColorArgs* color) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    struct PfxScriptVm* effect;

    effect = environment->effect;
    if (effect != 0 && effect->flags.vertex_color_enabled) {
        pfx_native_set_rgba(
            &effect->vertex_color,
            color->red, color->green, color->blue, color->alpha);
    }
}

void set_light(struct PfxLightArgs* light) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    struct PfxScriptVm* effect;
    int component;

    effect = environment->effect;
    if (effect != 0 && !effect->flags.light_enabled) {
        for (component = 0; component < 3; component++) {
            effect->light_direction_components[component] =
                light->direction_components[component];
        }
        effect->light_position.x = light->position.x;
        effect->light_position.y = light->position.y;
        effect->light_position.z = light->position.z;
        pfx_native_set_rgba(
            &effect->light_color,
            light->red, light->green, light->blue, light->alpha);
        effect->flags.light_enabled = 1;
        effect->flags.light_mode = light->mode;
    }
}

void z_bias(float bias) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0) {
        effect->z_bias = bias;
    }
}

void face_y(void) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0) {
        effect->orientation_flags.face_y = 1;
    }
}

void particle_size(float size) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0 &&
        effect->flags.particle_size_enabled != 0) {
        effect->particle_size = size;
    }
}

void set_decal_plane(float* plane) {
    struct PfxScriptVm* effect;

    effect = active_pfx_environment()->effect;
    if (effect != 0) {
        effect->flags.decal_plane_enabled = 1;
        effect->decal_plane[0] = plane[0];
        effect->decal_plane[1] = plane[1];
        effect->decal_plane[2] = plane[2];
        effect->decal_plane[3] = plane[3];
        effect->decal_plane[4] = plane[4];
        effect->decal_plane[5] = plane[5];
    }
}

void set_aspect_ratio(float x, float y) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0) {
        effect->aspect_x = x;
        effect->aspect_y = y;
    }
}

void set_bounding_radius(float radius) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0) {
        effect->bounding_radius = radius;
        effect->render_flags.custom_bounding_radius = 1;
    }
}

void enable_profiling(int enabled) {
    g_profile_enabled = enabled;
}

void initial_multiply_float(int unused, float minimum, float maximum) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxFloatRange range;

    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_multiply_float_range(
            environment->behavior,
            unused, &range);
    }
}

void initial_set_float(int unused, float minimum, float maximum) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxFloatRange range;

    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_set_float_range(
            environment->behavior,
            unused, &range);
    }
}

void initial_divert(int unused, float minimum, float maximum) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxFloatRange range;

    if (environment->behavior != 0) {
        range.center = minimum;
        range.variation = maximum;
        pfxvm_initial_divert(
            environment->behavior,
            unused, &range);
    }
}

void initial_add_v3(int destination, int source) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_initial_add_v3(
            environment->behavior, destination, source);
    }
}

void initial_reflect(int field) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_initial_reflect(environment->behavior, field);
    }
}

void kill_on_y_less_than_field(int field, int reference_field) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_kill_on_y_less_than_field(
            environment->behavior, field, reference_field);
    }
}

void change_on_y_less_than_field(int field, int source) {
    PfxBehavior* behavior = active_pfx_environment()->behavior;
    PfxBehavior* next_behavior = active_pfx_environment()->next_behavior;

    if (behavior != 0 && next_behavior != 0) {
        pfxvm_change_on_y_less_than_field(behavior, field, source, next_behavior);
    }
}

void change_on_y_less(int field, float value) {
    PfxBehavior* behavior = active_pfx_environment()->behavior;
    PfxBehavior* next_behavior = active_pfx_environment()->next_behavior;

    if (behavior != 0 && next_behavior != 0) {
        pfxvm_change_on_y_less(behavior, field, value, next_behavior);
    }
}

void change_on_less(int field, float value) {
    PfxBehavior* behavior = active_pfx_environment()->behavior;
    PfxBehavior* next_behavior = active_pfx_environment()->next_behavior;

    if (behavior != 0 && next_behavior != 0) {
        pfxvm_change_on_less(behavior, field, value, next_behavior);
    }
}

void change_on_greater(int field, float value) {
    PfxBehavior* behavior = active_pfx_environment()->behavior;
    PfxBehavior* next_behavior = active_pfx_environment()->next_behavior;

    if (behavior != 0 && next_behavior != 0) {
        pfxvm_change_on_greater(behavior, field, value, next_behavior);
    }
}

void kill_roundrobin(int field) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_kill_roundrobin(environment->behavior, field);
    }
}

void kill_percent(float percent) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_kill_percent(environment->behavior, percent);
    }
}

void kill_on_greater(int field, float value) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_kill_on_greater(environment->behavior, field, value);
    }
}

void udpate_roundrobin(int field) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_roundrobin(environment->behavior, field);
    }
}

void update_assign(int destination, int source) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_assign(
            environment->behavior, destination, source);
    }
}

void update_texanim_hold(int texture_field, int age_field, float frame_time,
                         int frame_count, int frame_offset) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_animate_texture(
            environment->behavior, texture_field, age_field, frame_time,
            frame_count, frame_offset, 0, 0);
    }
}

void update_texanim(int texture_field, int age_field, float frame_time,
                    int frame_count, int frame_offset) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_animate_texture(
            environment->behavior, texture_field, age_field, frame_time,
            frame_count, frame_offset, 0, 1);
    }
}

void update_fade_alpha2(int color_field, int age_field, float start_time,
                        float duration, int start_alpha, int end_alpha) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_fade_alpha(
            environment->behavior, color_field, age_field, start_time,
            duration, start_alpha, end_alpha);
    }
}

void update_lerp_color(
    int color_field, int age_field, float duration, int color_count,
    int first_color, struct PfxScriptColorRow* table) {
    PfxBehavior* behavior;
    PfxColor* colors;
    unsigned int row_count;
    int index;

    behavior = active_pfx_environment()->behavior;
    if (behavior != 0 && g_pfx_cmo != 0) {
        row_count = get_row_count_for_table_by_pointer(g_pfx_cmo, table);
        if (row_count != 0) {
            colors = get_mem(row_count * sizeof(*colors));
            for (index = 0; index < (int)row_count; index++) {
                struct PfxScriptColorRow* row = &table[index];
                PfxColor* color = &colors[index];
                color->r = row->red;
                color->g = row->green;
                color->b = row->blue;
                color->a = row->alpha;
            }
            pfxvm_update_lerp_color(
                behavior, color_field, age_field, duration,
                color_count, first_color, colors);
        }
    }
}

void update_fade_alpha(int color_field, int age_field, float start_time,
                       float duration) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_fade_alpha(
            environment->behavior, color_field, age_field, start_time,
            duration, 0xFF, 0);
    }
}

void update_wrapbox(int field, float scale, float x, float y, float z) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_wrapbox(environment->behavior, field, scale, x, y, z);
    }
}

void update_mul_scalar(int field, float x, float y, float z) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_mul_scalar(environment->behavior, field, x, y, z);
    }
}

void update_copy(int field) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_copy(environment->behavior, field);
    }
}

void update_add_constant(int field, float value) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_add_constant(environment->behavior, field, value);
    }
}

void update_add_constant_v3(int field, float x, float y, float z) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->behavior != 0) {
        pfxvm_update_add_constant_v3(environment->behavior, field, x, y, z);
    }
}

void update_bounce(int field, int velocity_field, int bounce_count_field,
                   float scale) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_bounce(
            environment->behavior, field, velocity_field,
            bounce_count_field, scale);
    }
}

void update_add(int destination, int source) {
    PfxBehavior* behavior;

    behavior = active_pfx_environment()->behavior;
    if (behavior != 0) {
        pfxvm_update_add(behavior, destination, source);
        if ((source & 0xF00) != 0x200) {
            pfxvm_update_copy(behavior, source);
        }
    }
}

void update_attract(int field, int target_field, float strength) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->behavior != 0) {
        pfxvm_update_attract(
            environment->behavior, field, target_field, strength);
    }
}

void create_multiemit_parametric_fx(struct PfxParametricEffectDescription* effect,
                                    char* name, int emitter_count) {
    char* saved;

    if (effect != 0 && name != 0) {
        saved = effect->effect_name;
        effect->effect_name = name;
        build_parametric_effect_from_table(
            active_cmdscript->mko, effect, emitter_count);
        effect->effect_name = saved;
    }
}

void create_parametric_fx(struct PfxParametricEffectDescription* effect, char* name) {
    char* saved;

    if (effect != 0 && name != 0) {
        saved = effect->effect_name;
        effect->effect_name = name;
        build_parametric_effect_from_table(active_cmdscript->mko, effect, 1);
        effect->effect_name = saved;
    }
}

void create_multiemit_step_fx(struct PfxStepEffectDescription* effect,
                              char* name, int emitter_count) {
    char* saved;

    if (effect != 0 && name != 0) {
        saved = effect->effect_name;
        effect->effect_name = name;
        build_step_effect(active_cmdscript->mko, effect, emitter_count);
        effect->effect_name = saved;
    }
}

void create_step_fx(struct PfxStepEffectDescription* effect, char* name) {
    char* saved;

    if (effect != 0 && name != 0) {
        saved = effect->effect_name;
        effect->effect_name = name;
        build_step_effect(active_cmdscript->mko, effect, 1);
        effect->effect_name = saved;
    }
}

void create_step_effect(struct PfxStepEffectDescription* effect) {
    if (effect != 0) {
        build_step_effect(active_cmdscript->mko, effect, 1);
    }
}

static inline void animate_effect_texture(
    float horizontal_scale, int vertical_frames, float speed) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;
    RwRaster* texture;
    float width;
    float height;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture != 0) {
            texture = effect->texture->raster;
            width = texture->width;
            height = texture->height;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                (PfxVm*)effect, width,
                (horizontal_scale * width),
                (height / (horizontal_scale * (float)vertical_frames)),
                vertical_frames, speed);
            effect->texture_animation_enabled = 1;
        }
    }
}

static void build_step_effect(
    ScriptSlot* script, struct PfxStepEffectDescription* description,
    int emitter_count) {
    struct PfxScriptEnvironment* environment;
    int* remaining_effects;
    char* effect_name;
    PfxVm* runtime;
    struct PfxScriptVm* script_runtime;
    PfxVmEmitter* emitter;
    PfxVmEmitter* first_emitter;
    PfxVmEmitter* emitter_source;
    PfxBehavior* behavior;
    int behavior_index;
    int instruction_index;
    PfxUpdateInstruction* instruction;
    int emitter_index;
    PfxVmEmitter emitter_template;
    PfxBehavior* behavior_targets[10];
    PfxBuildInfo build;
    struct PfxSpawnTableSlot table_slots[2];
    PfxFieldSet scan_fields = { 0, 0 };
    PfxFieldSet behavior_fields;
    PfxFieldSet behavior_flags;
    PfxFieldSet next_fields;
    PfxFieldSet next_flags;
    struct PfxScriptEffect* effect;

    environment = active_pfx_environment();
    remaining_effects = environment->remaining_effects;
    if (remaining_effects == 0) {
        return;
    }
    if (*remaining_effects == 0) {
        return;
    }

    clear_spawn_table_slots(table_slots);
    active_pfx_environment()->spawn_tables = table_slots;
    effect_name = description->effect_name;
    memset(&build, 0, sizeof(build));
    environment = active_pfx_environment();
    build.name = environment->effect_name_override;
    if (build.name == 0) {
        build.name = effect_name;
    }
    if (g_profile_enabled != 0) {
        build.metrics_frame_count = 0x384;
    }
    if (description->emitter->origin.x != 0.0f ||
        description->emitter->origin.y != 0.0f ||
        description->emitter->origin.z != 0.0f) {
        build.flag_bits.emitter_user_data = 1;
    }

    active_pfx_environment()->effect = 0;
    active_pfx_environment()->texture_name = description->texture->name;
    active_pfx_environment()->initialization_script =
        description->emitter->initialization_script;
    environment = active_pfx_environment();
    if (environment->initialization_script == 0U) {
        return;
    }

    memset(&emitter_template, 0, sizeof(emitter_template));
    g_pfx_cmo = script;
    emitter_template.birth_rate = parametric_birthrate;
    active_pfx_environment()->emitter = &emitter_template;
    push_script_stack_frame(0);
    cmdscript_setup_execution(
        script, active_pfx_environment()->initialization_script);
    cmdscript_execute(script);
    active_pfx_environment()->emitter = 0;
    g_pfx_cmo = 0;

    active_pfx_environment()->row_state.behavior_count =
        get_row_count_for_table_by_pointer(
            script, description->behavior_scripts);
    environment = active_pfx_environment();
    build.behavior_count = environment->row_state.behavior_count;
    if (build.behavior_count == 0) {
        return;
    }

    g_pfx_cmo = script;
    for (behavior_index = 0;
         behavior_index < build.behavior_count;
         behavior_index++) {
        behavior = &behavior_buffer[behavior_index];
        memset(behavior, 0, sizeof(*behavior));
        active_pfx_environment()->behavior = &behavior_buffer[behavior_index];
        if (behavior_index < build.behavior_count - 1) {
            active_pfx_environment()->next_behavior =
                &behavior_buffer[behavior_index] + 1;
        } else {
            active_pfx_environment()->next_behavior = 0;
        }
        push_script_stack_frame(0);
        cmdscript_setup_execution(
            script, description->behavior_scripts[behavior_index]);
        cmdscript_execute(script);
    }
    g_pfx_cmo = 0;

    memset(&behavior_fields, 0, sizeof(behavior_fields));
    memset(&behavior_flags, 0, sizeof(behavior_flags));
    pfx_behavior_scan_fields(
        behavior_buffer, &behavior_fields, &behavior_flags);
    for (behavior_index = 1;
         behavior_index < build.behavior_count;
         behavior_index++) {
        memset(&next_fields, 0, sizeof(next_fields));
        memset(&next_flags, 0, sizeof(next_flags));
        pfx_behavior_scan_fields(
            &behavior_buffer[behavior_index], &next_fields, &next_flags);
        if (next_flags.particle_flags != behavior_flags.particle_flags ||
            next_flags.render_flags != behavior_flags.render_flags) {
            return;
        }
        behavior_fields.render_flags |= next_fields.render_flags;
        behavior_fields.particle_flags |= next_fields.particle_flags;
    }

    scan_fields.render_flags = 2;
    pfx_emitter_scan_for_fields(&emitter_template, &scan_fields);
    if ((scan_fields.render_flags & 0x100) == 0 &&
        description->texture->frame_count > 1) {
        scan_fields.render_flags |= 0x200;
    }
    if ((scan_fields.render_flags & 0x200) != 0) {
        behavior_fields.particle_flags |= 2;
    }
    if ((behavior_fields.particle_flags & 2) != 0 &&
        (behavior_flags.particle_flags & 2) == 0 &&
        (scan_fields.particle_flags & 2) == 0) {
        for (behavior_index = 0;
             behavior_index < build.behavior_count;
             behavior_index++) {
            pfxvm_update_age(&behavior_buffer[behavior_index], 0x301);
            pfxvm_update_make_last_insn_first(
                &behavior_buffer[behavior_index]);
        }
        pfxvm_spawn_value(&emitter_template, 0x301, 0.0f);
        behavior_flags.particle_flags |= 2;
        scan_fields.particle_flags |= 2;
    }

    active_pfx_environment()->emitter = &emitter_template;
    g_pfx_cmo = script;
    build.emitter_count = emitter_count;
    new_pfx_create_raw_userdata(
        &build, 0, description->allocation_count,
        scan_fields.render_flags, scan_fields.particle_flags,
        (PfxInitCb)initialize_effect, 0, 0, (void**)&effect);
    if (effect == 0) {
        return;
    }

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < effect->emitter_count;
             emitter_index++) {
            environment = active_pfx_environment();
            emitter_source = environment->emitter;
            emitter = pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, emitter_index);
            copy_emitter_keep_transform(emitter, emitter_source);
        }
        active_pfx_environment()->emitter = effect->emitter;
    }
    g_pfx_cmo = 0;

    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        if (active_pfx_environment()->spawn_tables[emitter_index].type != 0) {
            pfx_register_table(
                (PfxTableRegistry*)effect->emitter_storage.emitters, emitter_index,
                active_pfx_environment()->spawn_tables[emitter_index].table);
        }
    }
    pfx_copy_behavior_list((PfxVm*)effect->emitter_storage.emitters, build.behavior_count, behavior_buffer);
    for (behavior_index = 0; behavior_index < 10; behavior_index++) {
        behavior_targets[behavior_index] =
            &behavior_buffer[behavior_index];
    }
    pfx_behaviors_fixup_targets(
        effect->behavior_list, behavior_targets, build.behavior_count);
    for (behavior_index = 0;
         behavior_index < effect->behavior_count;
         behavior_index++) {
        behavior = pfx_behavior((PfxVm*)effect->emitter_storage.emitters, behavior_index);
        if (behavior != 0) {
            for (instruction_index = 0;
                 instruction_index <
                     behavior->update_instruction_count;
                 instruction_index++) {
                instruction = &behavior->update_instructions[instruction_index];
                if (instruction->opcode == 0xE) {
                    instruction->target = &effect->behavior_target;
                }
            }
        }
    }
    pfxvm_compile((PfxVm*)effect->emitter_storage.emitters);

    first_emitter = pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, 0);
    first_emitter->position.x = description->emitter->origin.x;
    first_emitter->position.y = description->emitter->origin.y;
    first_emitter->position.z = description->emitter->origin.z;
    if (build.flag_bits.emitter_user_data) {
        memset(first_emitter->pfx_transform, 0,
               sizeof(*first_emitter->pfx_transform));
        first_emitter->pfx_transform->elements[0][0] = 1.0f;
        first_emitter->pfx_transform->elements[1][1] = 1.0f;
        first_emitter->pfx_transform->elements[2][2] = 1.0f;
        first_emitter->pfx_transform->elements[3][3] = 1.0f;
        first_emitter->pfx_transform->elements[3][0] =
            description->emitter->origin.x;
        first_emitter->pfx_transform->elements[3][1] =
            description->emitter->origin.y;
        first_emitter->pfx_transform->elements[3][2] =
            description->emitter->origin.z;
    }
    effect->effect_value = description->effect_value;
    runtime = (PfxVm*)effect->emitter_storage.emitters;
    first_emitter->flags.bits.cycle_paused = 1;
    effect->lifecycle_flags.restart_cycle = 0;
    active_pfx_environment()->source_effect = (MkPfx*)effect;
    runtime->effect_id = description->effect_id;
    first_emitter->birth_rate = description->emitter_lifetime;
    for (emitter_index = 1;
         emitter_index < effect->emitter_count;
         emitter_index++) {
        copy_emitter_keep_transform(
            pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, emitter_index),
            pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, 0));
    }

    script_runtime = (struct PfxScriptVm*)runtime;
    script_runtime->vertex_color.r = 0xFF;
    script_runtime->vertex_color.g = 0xFF;
    script_runtime->vertex_color.b = 0xFF;
    script_runtime->vertex_color.a = 0xFF;
    script_runtime->flags.vertex_color_enabled = 1;
    pfx_render_set_blendmode(
        (struct PfxRenderView*)runtime, description->blend_mode);
    if (description->texture->frame_count > 1) {
        animate_effect_texture(description->texture->horizontal_scale,
                               description->texture->frame_count,
                               description->texture->animation_speed);
    }

    if (pfx_verify((struct PfxVerifyView*)runtime) != 0) {
        (*remaining_effects)--;
        bank_record_effect(effect);
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            pfx_emitter_reset(pfx_get_emitter(runtime, emitter_index));
        }
        active_pfx_environment()->spawn_tables = 0;
    }
}

static inline void pfx_reset_behavior_particles(PfxVm* runtime)
{
    int field_index;

    for (field_index = 0;
         field_index < runtime->behavior_count;
         field_index++) {
        runtime->behavior_list[field_index]->particle_count = 0;
    }
}

static inline void pfx_reset_effect_emitters(struct PfxScriptEffect* effect) {
    PfxVm* runtime = (PfxVm*)effect->emitter_storage.emitters;
    PfxVmEmitter* emitter;
    int emitter_index;
    effect->lifecycle_flags.restart_cycle = 0;
    for (emitter_index = 0; emitter_index < runtime->emitter_count; emitter_index++) {
        emitter = pfx_get_emitter(runtime, emitter_index);
        emitter->flags.bits.cycle_paused = 1;
        emitter->cycle_index = 0;
        pfx_emitter_reset(emitter);
    }
}

void reset_effect(const char* name) {
    struct PfxScriptEffect* effect;
    PfxVm* runtime;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        runtime = (PfxVm*)effect->emitter_storage.emitters;
        pfx_reset_effect_emitters(effect);

        if (runtime->parametric != 0) {
            memset(
                runtime->parametric + 1, 0,
                runtime->parametric->particle_capacity *
                    sizeof(PfxParametricParticle));
        } else {
            pfx_reset_behavior_particles(runtime);
        }

        runtime->particle_cursor = 0;
        runtime->elapsed_time = 0.0f;
    }
}

void reset_effect_ppfx(struct PfxScriptEffect* effect) {
    PfxVm* runtime;
    PfxVmEmitter* emitter;
    int emitter_index;

    runtime = (PfxVm*)effect->emitter_storage.emitters;
    effect->lifecycle_flags.restart_cycle = 0;
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
        pfx_reset_behavior_particles(runtime);
    }

    runtime->particle_cursor = 0;
    runtime->elapsed_time = 0.0f;
}

void fx_reset(unsigned int handle) {
    struct PfxResolvedHandle resolved;
    struct PfxScriptEffect* effect;
    PfxVm* runtime;

    resolve_pfx_handle(handle, &resolved);
    effect = resolved.effect;
    if (effect == 0) {
        return;
    }

    runtime = (PfxVm*)effect->emitter_storage.emitters;
    fx_reset_emit(handle);
    if (runtime->parametric != 0) {
        memset(
            runtime->parametric + 1, 0,
            runtime->parametric->particle_capacity *
                sizeof(PfxParametricParticle));
    } else {
        pfx_reset_behavior_particles(runtime);
    }

    runtime->particle_cursor = 0;
    runtime->elapsed_time = 0.0f;
}

static inline PfxVmEmitter* emitter_from_runtime(PfxVm* runtime, int index);

static inline PfxVmEmitter* emitter_from_handle(unsigned int handle) {
    PfxVm* runtime = (PfxVm*)resolve_effect_handle(handle)->emitter_storage.emitters;
    return emitter_from_runtime(runtime, (handle >> 16) & 0xF);
}

static inline PfxVmEmitter* emitter_from_runtime(PfxVm* runtime, int index) {
    PfxVmEmitter* emitter;

    if (runtime == 0) {
        emitter = 0;
    } else if (index < 0 || index >= runtime->emitter_count) {
        emitter = 0;
    } else {
        emitter = &runtime->emitters[index];
    }
    return emitter;
}

void fx_restart_emit(unsigned int handle) {
    PfxVmEmitter* emitter;
    struct PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        effect->lifecycle_flags.restart_cycle = 1;
        emitter->flags.bits.cycle_paused = 0;
        emitter->cycle_index = 0;
        pfx_emitter_restart_cycle(emitter);
    }
}

void fx_reset_emit(unsigned int handle) {
    PfxVmEmitter* emitter;

    emitter = emitter_from_handle(handle);
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 1;
        emitter->cycle_index = 0;
        pfx_emitter_reset(emitter);
    }
}

void restart_effect(const char* name) {
    struct PfxScriptEffect* effect;
    PfxVmEmitter* emitter;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        emitter = effect->emitter;
        effect->lifecycle_flags.restart_cycle = 1;
        emitter->flags.bits.cycle_paused = 0;
        emitter->cycle_index = 0;
        pfx_emitter_restart_cycle(emitter);
    }
}

void restart_effect_ppfx(struct PfxScriptEffect* effect) {
    PfxVmEmitter* emitter = effect->emitter;

    effect->lifecycle_flags.restart_cycle = 1;
    emitter->flags.bits.cycle_paused = 0;
    emitter->cycle_index = 0;
    pfx_emitter_restart_cycle(emitter);
}

void resume_effect(const char* name) {
    struct PfxScriptEffect* effect;
    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->lifecycle_flags.restart_cycle = 1;
        pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, 0)->flags.bits.cycle_paused = 0;
    }
}

void fx_pause_emit(unsigned int handle) {
    PfxVmEmitter* emitter;

    emitter = emitter_from_handle(handle);
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 1;
    }
}

void fx_resume_emit(unsigned int handle) {
    PfxVmEmitter* emitter;
    struct PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->flags.bits.cycle_paused = 0;
        effect->lifecycle_flags.restart_cycle = 1;
    }
}

void kill_at_plane(float plane) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;

    environment = active_pfx_environment();
    effect = environment->effect;
    if (effect != 0) {
        effect->kill_plane = plane;
    }
}

void set_cycle_emission(int enabled) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->birth_limit = enabled;
    }
}

void set_cycle_length(float length, float position) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->cycle_length = length;
        emitter->cycle_length_variation = position;
    }
}

static inline int pfxscript_add_table(int type, PfxSpawnTable* table,
                                      int row_count) {
    struct PfxSpawnTableSlot* slots = active_pfx_environment()->spawn_tables;
    int index;

    for (index = 0; index < 2; index++) {
        if (slots[index].type == 0) {
            slots[index].type = type;
            slots[index].table = table;
            slots[index].row_count = row_count;
            return index;
        }
    }
    return 2;
}

void spawn_random_size(float* table) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;
    PfxSpawnTable* copied_table;
    unsigned int row_count;
    unsigned int value_size;
    int slot_index;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter == 0) {
        return;
    }
    if (g_pfx_cmo == 0) {
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

    slot_index = pfxscript_add_table(3, copied_table, row_count);

    environment = active_pfx_environment();
    if (environment->field04 != 0) {
        pfxvm_spawn_set_field_from_table(emitter, 0x402, slot_index);
    } else {
        pfxvm_spawn_set_field_from_table(emitter, 0x102, slot_index);
    }
}

void set_growth_coefficient(float coefficient) {
    if (active_pfx_environment()->effect != 0) {
        active_pfx_environment()->growth_coefficient = coefficient;
    }
}

void set_drag_coefficient(float coefficient) {
    if (active_pfx_environment()->effect != 0) {
        active_pfx_environment()->drag_coefficient = coefficient;
    }
}

void set_rotation(float angle, float variance) {
    struct PfxScriptVm* effect;
    PfxVmEmitter* emitter;

    effect = active_pfx_environment()->effect;
    if (effect != 0) {
        emitter = pfx_get_emitter((PfxVm*)effect, 0);
        pfxvm_spawn_line_1f(emitter, 0x403, angle - variance,
                            angle + variance);
        effect->orientation_flags.rotation_enabled = 1;
    }
}

void texture_animation_with_vsize(
    int vertical_frames, float horizontal_scale, float vertical_scale,
    float speed) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;
    RwRaster* texture;
    float width;
    float height;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture != 0) {
            texture = effect->texture->raster;
            width = texture->width;
            height = texture->height;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                (PfxVm*)effect, width,
                (horizontal_scale * width),
                (vertical_scale * height),
                vertical_frames, speed);
            effect->texture_animation_enabled = 1;
        }
    }
}

void texture_animation(float horizontal_scale, int vertical_frames, float speed) {
    struct PfxScriptEnvironment* environment;
    struct PfxScriptVm* effect;
    RwRaster* texture;
    float width;
    float height;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        environment = active_pfx_environment();
        effect = environment->effect;
        if (effect != 0 && effect->texture != 0) {
            texture = effect->texture->raster;
            width = texture->width;
            height = texture->height;
            if (effect->initialization_mode != 0) {
                effect->runtime_flags |= 0x100;
            }
            pfx_texture_animate(
                (PfxVm*)effect, width,
                (horizontal_scale * width),
                (height / (horizontal_scale * (float)vertical_frames)),
                vertical_frames, speed);
            effect->texture_animation_enabled = 1;
        }
    }
}

void spawn_color(int field, int red, int green, int blue, int alpha) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            environment->emitter, field, red, green, blue, alpha);
    }
}

void emission_duration(float duration) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
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
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxVec3 axis;

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
    float half = 0.5f;
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_box(
            environment->emitter, field,
            -(width * half - x), -(height * half - y),
            -(depth * half - z), width, height, depth);
    }
}

void emit_disc2(
    int field, float x, float y, float z,
    float inner_radius, float outer_radius) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxVec3 axis;

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
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    PfxVec3 axis;

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
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere_section(
            environment->emitter, field, x, y, z, radius,
            radius_spread, angle, angle_spread);
    }
}

void emit_from_pos_clamp_y(int field, int source, float x, float y, float z,
                           float minimum_length, float length_range,
                           float clamped_y) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(
            environment->emitter, field, source,
            x, y, z, minimum_length, length_range, 1, clamped_y);
    }
}

void emit_from_pos(int field, int source, float x, float y, float z,
                   float minimum_length, float length_range) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(
            environment->emitter, field, source,
            x, y, z, minimum_length, length_range, 0, 0.0f);
    }
}

void emit_spherical_from_boundary(int field, float radius) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(
            environment->emitter, field, 0.0f, 0.0f, 0.0f,
            radius, radius, 0);
    }
}

void emit_spherical(int field, float radius) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(
            environment->emitter, field, 0.0f, 0.0f, 0.0f,
            0.00001f, radius, 0);
    }
}

void emit_cuboid(int field, float x, float y, float z) {
    float half = 0.5f;
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_box(
            environment->emitter, field,
            -x * half, -y * half, -z * half, x, y, z);
    }
}

void emit_from_point(float x, float y, float z) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->position.x = x;
        emitter->position.y = y;
        emitter->position.z = z;
    }
}

void emit_color(int field, int red, int green, int blue, int alpha) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_point_color(
            environment->emitter, field, red, green, blue, alpha);
    }
}

void emit_uv(int field, float u, float v) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        pfxvm_spawn_uv(environment->emitter, field, u, v);
    }
}

void emit_in_range(int unused, float center, float width) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();
    float half_width;

    if (environment->emitter != 0) {
        half_width = width;
        half_width *= 0.5f;
        pfxvm_spawn_line_1f(environment->emitter, unused,
                            center - half_width, center + half_width);
    }
}

void emit_value(int field, float value) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        pfxvm_spawn_value(environment->emitter, field, value);
    }
}

void emit_value_i(int field, int value) {
    struct PfxScriptEnvironment* environment;

    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        pfxvm_spawn_line_1i(
            environment->emitter, field, value, value);
    }
}

void emit_constant_rate(void) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;

    environment = active_pfx_environment();
    emitter = environment->emitter;
    if (emitter != 0) {
        emitter->flags.bits.constant_rate = 1;
    }
}

void emit_roundrobin_mechanism(int field, int source) {
    struct PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->emitter != 0) {
        pfxvm_spawn_roundrobin_mechanism(
            environment->emitter, field, source);
    }
}

/* TODO: [near miss] 99.575%; build_case_ranges merges adjacent OID labels;
 * equality guards fold the retail exit pair; recover original dispatch boundary. */
void bind_to_bone(int bone_index) {
    struct PfxScriptEnvironment* environment;
    MkPfx* effect;
    MkObj* object;

    environment = active_pfx_environment();
    effect = environment->source_effect;
    if (bone_index == 0 || active_cmdscript == 0 || active_cmdscript->mko == 0) {
        return;
    }
    if (active_cmdscript->mko->load_ctx == 0) return;
    object = active_cmdscript->mko->load_ctx->bgnd_obj;
    switch (object->oid) {
    case 0x1001:
    case 0x1002:
        effect->bound_obj = object;
        if (effect->bound_obj != 0) {
            pfx_bind_emitter_to_obj_bone(effect, effect->bound_obj, bone_index);
        }
        break;
    default:
        return;
    }
}

void fx_bind_emitter_to_obj_bone(
    unsigned int handle, MkObj* object, int bone_index) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_emitter_to_obj_bone(
            (MkPfx*)resolved.effect, object, bone_index);
    }
}

void fx_bind_render_to_sobj(unsigned int handle, MkSobj* object) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_render_to_sobj((MkPfx*)resolved.effect, object, 0);
    }
}

void fx_bind_render_to_obj_bone(
    unsigned int handle, MkObj* object, int bone_index) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    if (resolved.effect != 0) {
        pfx_bind_render_to_obj_bone(
            (MkPfx*)resolved.effect, object, bone_index);
    }
}

void parametric_update(struct PfxParametricEffectDescription* effect) {
    if (effect != 0) {
        build_parametric_effect_from_table(
            active_cmdscript->mko, effect, 1);
    }
}

static inline void destroy_active_pfxscript_banks(void) {
    int index;

    for (index = 0; index < 15; index++) {
        struct PfxBank* bank;

        bank = MK_HDR_LIVE(banks[index].bank, banks[index].bank_instance);
        if (bank != 0) {
            bank_destroy(&bank->hdr);
        }
    }
}

void unload_all_effect_banks(void) {
    int clear_index;

    destroy_active_pfxscript_banks();
    for (clear_index = 0; clear_index < 15; clear_index++) {
        banks[clear_index].bank = 0;
        banks[clear_index].bank_instance = 0;
    }
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

struct PfxLoadScriptLatch {
    CmdScript* command;
    ScriptSlot* script;
};

static inline void pfx_release_load_script(struct PfxLoadScriptLatch* latch) {
    if (latch->command != 0) {
        if (latch->command->instance != 0) {
            ((MkHdr*)latch->command)->typed_vtbl->destroy(
                (MkHdr*)latch->command);
        }
        latch->command = 0;
        cmdscript_unload(latch->script);
        latch->script = 0;
    }
}

static inline void pfx_cleanup_load_script(struct PfxLoadScriptLatch* latch) {
    if (latch->command == 0) {
        memset(latch, 0, sizeof(*latch));
    } else {
        pfx_release_load_script(latch);
    }
}

static inline int pfx_create_load_script(struct PfxLoadScriptLatch* latch) {
    memset(latch, 0, sizeof(*latch));
    latch->command = alloc_cmdscript();
    if (latch->command == 0) {
        pfx_cleanup_load_script(latch);
        return 0;
    }
    latch->script = 0;
    return 1;
}

static inline struct PfxBank* pfx_create_bank(char* name, int effect_count) {
    struct PfxBank* bank;
    unsigned int name_offset;
    unsigned int owners_offset;
    unsigned int size;
    int effect_index;

    size = sizeof(*bank) + effect_count * sizeof(struct PfxEffectLatch);
    owners_offset = size;
    size += effect_count * sizeof(unsigned int);
    name_offset = size;
    size += strlen(name);
    size++;
    bank = (struct PfxBank*)get_mkhdr(&vtbl_effectbank, size);
    if (bank == 0) {
        return 0;
    }

    bank->effects = (struct PfxEffectLatch*)(bank + 1);
    bank->name = (char*)bank + name_offset;
    bank->effect_owners = (unsigned int*)((char*)bank + owners_offset);
    bank->effect_capacity = 0;
    bank->effect_count = effect_count;
    strcpy(bank->name, name);
    for (effect_index = 0; effect_index < effect_count; effect_index++) {
        bank->effects[effect_index].effect = 0;
        bank->effects[effect_index].effect_instance = 0;
        bank->effect_owners[effect_index] = 0;
    }
    return bank;
}

static inline unsigned int pfx_bank_latch_handle(int bank_index) {
    if (bank_index < 0 || bank_index >= 15) {
        return 0;
    }
    return (banks[bank_index].bank_instance & 0xFFFFFFF0) |
           ((bank_index + 1) & 0xF);
}

static inline unsigned int pfx_alloc_bank(char* name, int effect_count) {
    struct PfxBank* raw_bank;
    struct PfxBank* bank;
    int bank_index;

    bank_index = 0;
    while (bank_index < 15) {
        raw_bank = MK_HDR_LIVE(banks[bank_index].bank,
                               banks[bank_index].bank_instance);
        if (raw_bank == 0) {
            break;
        }
        bank_index++;
    }
    if (bank_index >= 15) {
        return 0;
    }
    bank = pfx_create_bank(name, effect_count);
    if (bank == 0) {
        return 0;
    }
    banks[bank_index].bank = bank;
    banks[bank_index].bank_instance = bank->hdr.instance;
    bank->handle_bank = bank_index + 1;
    bank_instance_counter[bank_index] =
        (bank_instance_counter[bank_index] + 1) & 0xFF000000;
    bank->handle_generation = bank_instance_counter[bank_index];
    return pfx_bank_latch_handle(bank_index);
}

/* TODO: [breakthrough needed] 98.42236%; row-loop zero has no proven owner;
 * verified PS2 counterpart also writes it; recover its original source use. */
void load_effect_bank_with_context(char* name, LoadBgndCtx* context) {
    struct PfxLoadScriptLatch load;
    struct PfxBankLoadRow* rows;
    struct PfxBank* bank;
    struct PfxScriptVm* built_effect;
    unsigned int row_count;
    unsigned int row_number;
    unsigned int bank_handle;
    int total_effects;
    int remaining_effects;
    int language;
    int owner;
    MkObj* parent;

    if (pfx_create_load_script(&load) != 0) {
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
            return;
        }

        load.script = cmdscript_loadfile_by_name(language, name);
        load.command->mko = load.script;
        load.command->mko->load_ctx = context;
        rows = get_data_table(load.script, load.script->table_count);
        if (rows == 0) {
            pfx_cleanup_load_script(&load);
            return;
        }

        behavior_buffer = get_mem(10 * sizeof(*behavior_buffer));
        row_count = get_row_count_for_table(load.script, load.script->table_count);
        total_effects = 0;
        for (row_number = 1; row_number <= row_count; row_number++) {
            total_effects += rows[row_number - 1].effect_count;
        }

        bank_handle = pfx_alloc_bank(name, total_effects);
        bank = bank_from_handle(bank_handle);
        current_effect_bank = bank;
        if (current_effect_bank == 0) {
            pfx_cleanup_load_script(&load);
            return;
        }

        if (parent != 0) {
            mk_insert(&bank->hdr, &parent->child_list);
        } else {
            mk_insert(&bank->hdr, &aproc->pdata_list_b);
        }
        current_effect_bank->owner_flags = owner;

        for (row_number = 1; row_number <= row_count; row_number++) {
            CmdScript* saved_command;
            saved_command = active_cmdscript;
            active_cmdscript = load.command;
            load.script->load_ctx = context;
            remaining_effects = rows[row_number - 1].effect_count;
            if (pfxscript_environment.active == 0) {
                memset(
                    &pfxscript_environment, 0,
                    sizeof(pfxscript_environment));
                pfxscript_environment.active = 1;
                active_pfx_environment()->remaining_effects = &remaining_effects;
            }
            cmdscript_setup_execution(load.script, rows[row_number - 1].function_index);
            cmdscript_execute(load.script);
            built_effect = active_pfx_environment()->effect;
            if (pfxscript_environment.active != 0) {
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
}

static void build_parametric_effect_from_table(
    ScriptSlot* script, struct PfxParametricEffectDescription* description, int update) {
    struct PfxParametricEmitterDescription* emitter_description;
    struct PfxScriptEnvironment* environment;
    char* effect_name;
    struct PfxScriptVm* script_runtime;
    PfxVm* runtime;
    PfxVmEmitter* emitter;
    PfxVmEmitter* first_emitter;
    struct PfxScriptColorRow* color_table;
    float* table_a;
    float* table_b;
    unsigned int create_flags = 2;
    int* remaining_effects;
    int row;
    int emitter_index;
    int matrix_row;
    int matrix_column;
    PfxVmEmitter emitter_template;
    PfxBuildInfo build;
    struct PfxSpawnTableSlot table_slots[2];
    struct PfxScriptEffect* effect;

    remaining_effects = active_pfx_environment()->remaining_effects;
    if (remaining_effects == 0) {
        return;
    }
    if (*remaining_effects == 0) {
        return;
    }

    clear_spawn_table_slots(table_slots);
    environment = active_pfx_environment();
    environment->spawn_tables = table_slots;
    effect_name = parametric_effect_name(description);
    memset(&build, 0, sizeof(build));
    build.name = active_pfx_environment()->effect_name_override;
    if (build.name == 0) {
        build.name = effect_name;
    }
    if (g_profile_enabled != 0) {
        build.metrics_frame_count = 0x384;
    }

    environment->effect = 0;
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

    environment->drag_coefficient = 0.0f;
    environment->growth_coefficient = 0.0f;
    environment->texture_name = description->texture_name;
    emitter_description = description->emitter;
    environment->initialization_script =
        emitter_description->initialization_script;
    if (environment->initialization_script == 0U) {
        return;
    }
    if (emitter_description->origin.x != 0.0f ||
        emitter_description->origin.y != 0.0f ||
        emitter_description->origin.z != 0.0f) {
        build.flag_bits.emitter_user_data = 1;
    }

    build.behavior_count = environment->row_state.behavior_count;
    environment->row_state.behavior_count = 0;
    build.behavior_count = 0;
    g_effect_description = description;
    g_pfx_cmo = script;
    environment->field04 = 1;
    memset(&emitter_template, 0, sizeof(emitter_template));
    environment->emitter = 0;
    build.emitter_count = update;
    new_pfx_create_raw_userdata(
        &build, 0, (1.1f * description->allocation_count),
        create_flags, 0, (PfxInitCb)initialize_effect, 0, 0,
        (void**)&effect);
    if (effect == 0) {
        return;
    }

    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < effect->emitter_count;
             emitter_index++) {
            PfxVmEmitter* emitter_source;
            emitter_source = environment->emitter;
            emitter = pfx_get_emitter(
                (PfxVm*)effect->emitter_storage.emitters, emitter_index);
            copy_emitter_keep_transform(emitter, emitter_source);
        }
        environment->emitter = effect->emitter;
    }
    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        if (environment->spawn_tables[emitter_index].type != 0) {
            pfx_register_table(
                (PfxTableRegistry*)effect->emitter_storage.emitters,
                emitter_index,
                environment->spawn_tables[emitter_index].table);
        }
    }
    g_effect_description = 0;
    g_pfx_cmo = 0;

    scan_effect_emitter_fields(&effect);
    pfxvm_compile((PfxVm*)effect->emitter_storage.emitters);

    first_emitter = pfx_get_emitter(
        (PfxVm*)effect->emitter_storage.emitters, 0);
    first_emitter->position.x = description->emitter->origin.x;
    first_emitter->position.y = description->emitter->origin.y;
    first_emitter->position.z = description->emitter->origin.z;
    if (build.flag_bits.emitter_user_data) {
        memset(first_emitter->pfx_transform, 0,
               sizeof(*first_emitter->pfx_transform));
        first_emitter->pfx_transform->elements[0][0] = 1.0f;
        first_emitter->pfx_transform->elements[1][1] = 1.0f;
        first_emitter->pfx_transform->elements[2][2] = 1.0f;
        first_emitter->pfx_transform->elements[3][3] = 1.0f;
        first_emitter->pfx_transform->elements[3][0] =
            description->emitter->origin.x;
        first_emitter->pfx_transform->elements[3][1] =
            description->emitter->origin.y;
        first_emitter->pfx_transform->elements[3][2] =
            description->emitter->origin.z;
    }

    effect->effect_value = description->effect_value;
    runtime = (PfxVm*)effect->emitter_storage.emitters;
    effect->effect_id = description->effect_id;
    if (current_effect_bank->owner_flags == 8) {
        environment->source_effect->flag_bits.skip_translucent_sort = 1;
    }
    for (emitter_index = 0;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        pfx_get_emitter(runtime, emitter_index)->flags.bits.cycle_paused = 1;
    }
    effect->lifecycle_flags.restart_cycle = 0;
    environment->source_effect = (MkPfx*)effect;

    if (runtime->parametric == 0) {
        return;
    }
    runtime->parametric->acceleration.x = 0.0f;
    runtime->parametric->acceleration.y = 0.0f;
    runtime->parametric->acceleration.z = 0.0f;
    runtime->parametric->vertical_acceleration = description->field2C;
    runtime->parametric->lifetime = description->field34;
    first_emitter->birth_rate = description->emitter_lifetime;
    for (emitter_index = 1;
         emitter_index < effect->emitter_count;
         emitter_index++) {
        copy_emitter_keep_transform(
            pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, emitter_index),
            pfx_get_emitter((PfxVm*)effect->emitter_storage.emitters, 0));
    }

    color_table = description->color_table;
    if (color_table != 0) {
        if (get_row_count_for_table_by_pointer(script, color_table) > 1U) {
            runtime->parametric->color_curve_count =
                get_row_count_for_table_by_pointer(script, color_table);
            for (row = 0; row < runtime->parametric->color_curve_count;
                 row++) {
                runtime->parametric->color_curve[row].r =
                    color_table[row].red;
                runtime->parametric->color_curve[row].g =
                    color_table[row].green;
                runtime->parametric->color_curve[row].b =
                    color_table[row].blue;
                runtime->parametric->color_curve[row].a =
                    color_table[row].alpha;
            }
        } else {
            script_runtime = (struct PfxScriptVm*)runtime;
            script_runtime->vertex_color.r = color_table->red;
            script_runtime->vertex_color.g = color_table->green;
            script_runtime->vertex_color.b = color_table->blue;
            script_runtime->vertex_color.a = color_table->alpha;
            script_runtime->flags.vertex_color_enabled = 1;
        }
    }
    table_a = description->table_a;
    if (table_a != 0) {
        runtime->parametric->texture_curve_count =
            get_row_count_for_table_by_pointer(script, table_a);
        for (row = 0; row < runtime->parametric->texture_curve_count; row++) {
            runtime->parametric->texture_curve[row] = table_a[row];
        }
    }
    table_b = description->table_b;
    if (table_b != 0) {
        runtime->parametric->size_curve_count =
            get_row_count_for_table_by_pointer(script, table_b);
        for (row = 0; row < runtime->parametric->size_curve_count; row++) {
            runtime->parametric->size_curve[row] = table_b[row];
        }
    }
    if (first_emitter->pfx_transform != 0) {
        for (matrix_row = 0; matrix_row < 4; matrix_row++) {
            for (matrix_column = 0; matrix_column < 4; matrix_column++) {
                if (matrix_row == matrix_column) {
                    first_emitter->pfx_transform
                        ->elements[matrix_row][matrix_column] = 1.0f;
                } else {
                    first_emitter->pfx_transform
                        ->elements[matrix_row][matrix_column] = 0.0f;
                }
            }
        }
        first_emitter->pfx_transform->elements[3][0] =
            description->emitter->origin.x;
        first_emitter->pfx_transform->elements[3][1] =
            description->emitter->origin.y;
        first_emitter->pfx_transform->elements[3][2] =
            description->emitter->origin.z;
    }
    runtime->parametric->acceleration.x = description->field1C;
    runtime->parametric->acceleration.y = description->field20;
    runtime->parametric->acceleration.z = description->field24;
    pfx_render_set_blendmode(
        (struct PfxRenderView*)runtime, description->blend_mode);
    runtime->parametric->damping = environment->drag_coefficient;
    runtime->parametric->texture_rate = environment->growth_coefficient;

    if (pfx_verify((struct PfxVerifyView*)runtime) != 0) {
        (*remaining_effects)--;
        bank_record_effect(effect);
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            pfx_emitter_reset(pfx_get_emitter(runtime, emitter_index));
        }
        environment->spawn_tables = 0;
    }
}


MkPfx* find_pfx_by_handle(unsigned int handle) {
    struct PfxResolvedHandle resolved;

    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
}


MkPfx* find_pfx_by_name_by_bankowner(
    const char* name, unsigned int owner) {
    struct PfxResolvedHandle resolved;
    unsigned int handle;

    handle = fx_by_owner(name, owner);
    resolve_pfx_handle(handle, &resolved);
    return (MkPfx*)resolved.effect;
}

struct PfxScriptEffect* find_pfx_by_name(const char* name) {
    struct PfxResolvedHandle resolved;
    unsigned int handle;

    handle = fx_by_owner(name, 0xFF);
    resolve_pfx_handle(handle, &resolved);
    return resolved.effect;
}

static void initialize_effect(struct PfxScriptVm* effect) {
    struct PfxScriptEnvironment* environment;
    PfxVmEmitter* emitter;
    ScriptSlot* script;
    char* texture_name;
    LoadBgndCtx* load_ctx;

    environment = active_pfx_environment();
    if (environment->effect == 0) {
        environment = active_pfx_environment();
        environment->effect = effect;
    }

    environment = active_pfx_environment();
    effect->initialization_mode = environment->field04;

    environment = active_pfx_environment();
    texture_name = environment->texture_name;
    script = g_pfx_cmo;
    load_ctx = script->load_ctx;
    if (texture_name != 0 && texture_name[0] != '\0') {
        effect->flags.textured = 1;
        effect->texture = load_named_tga_from_slot(
            load_ctx->art_id, texture_name);
    }

    if ((effect->runtime_flags & 0x20) == 0) {
        effect->flags.particle_size_enabled = 1;
        effect->particle_size = 1.0f;
    } else {
        effect->flags.particle_size_enabled = 0;
    }

    environment = active_pfx_environment();
    if (environment->emitter == 0) {
        emitter = pfx_get_emitter((PfxVm*)effect, 0);
        script = g_pfx_cmo;
        emitter->birth_rate = parametric_birthrate;
        environment = active_pfx_environment();
        environment->emitter = emitter;
        push_script_stack_frame(0);
        cmdscript_setup_execution(
            script, active_pfx_environment()->initialization_script);
        cmdscript_execute(script);
        active_pfx_environment()->emitter = 0;
    }
}

static inline void clear_pfxscript_banks(void)
{
    int index;

    for (index = 0; index < 15; index++) {
        banks[index].bank = 0;
        banks[index].bank_instance = 0;
    }
}

void pfxscript_initialize(void) {
    MkProcInitFlags flags;

    clear_pfxscript_banks();
    flags.value = 0;
    cached_handle = 0;
    flags.bits.no_destroy = 1;

    create_mkproc(0x2E, get_mkproc_nostack(flags), 0x7777,
                  p_update_effects, 0);
}

static inline void pfxscript_run_all_banks(void) {
    int index;

    for (index = 0; index < 15; index++) {
        struct PfxBank* bank;

        bank = MK_HDR_LIVE(banks[index].bank, banks[index].bank_instance);
        if (bank != 0) {
            bank_run_fx(bank);
        }
    }
}

static float p_update_effects(void) {
    pfxscript_run_all_banks();
    return 1.0f;
}

void fxbanks_unload_by_owner(unsigned int owner_flags) {
    int index;

    for (index = 0; index < 15; index++) {
        struct PfxBank* bank;

        bank = MK_HDR_LIVE(banks[index].bank, banks[index].bank_instance);
        if (bank != 0 && (bank->owner_flags & owner_flags) != 0) {
            if (bank->hdr.instance != 0U) {
                struct PfxBankVtablePrefix* vtbl;

                vtbl = (struct PfxBankVtablePrefix*)bank->hdr.vtbl;
                vtbl->destroy(&bank->hdr);
            }
            banks[index].bank = 0;
            banks[index].bank_instance = 0;
        }
    }
}

static inline void bank_destroy(MkHdr* bank) {
    struct PfxBankVtablePrefix* vtbl;

    if (bank->instance != 0U) {
        vtbl = (struct PfxBankVtablePrefix*)bank->vtbl;
        vtbl->destroy(bank);
    }
}

static void bank_run_fx(struct PfxBank* bank) {
    struct PfxEffectLatch* effect_latch;
    struct PfxScriptEffect* effect;
    int effect_index;
    PfxVm* runtime;
    struct PfxResolvedHandle transfer;
    int emitter_index;
    PfxVmEmitter* emitter;

    for (effect_index = 0;
         effect_index < bank->effect_capacity;
         effect_index++) {
        effect_latch = &bank->effects[effect_index];
        effect = MK_HDR_LIVE(effect_latch->effect, effect_latch->effect_instance);
        if (effect == 0 || !effect->lifecycle_flags.restart_cycle) {
            continue;
        }

        runtime = (PfxVm*)effect->emitter_storage.emitters;
        if (runtime->field_0x22C != 0) {
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
            if ((int)bank->effect_owners[effect_index] != 0) {
                resolve_pfx_handle(
                    bank->effect_owners[effect_index], &transfer);
                if (transfer.effect != 0) {
                    pfxvm_create_transfer(
                        (PfxVm*)transfer.effect->emitter_storage.emitters,
                        runtime);
                    transfer.effect->lifecycle_flags.restart_cycle = 1;
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
                effect->lifecycle_flags.restart_cycle = 0;
            }
        }
        pfxmetrics_event(runtime->metrics, 0x2000);
    }
}

/* TODO: [breakthrough needed] 96.75%; original search/packing producers missing;
 * second null gate is absent before allocation, shift/OR merges in backend. */
static unsigned int banks_find_owned_fx(
    const char* name, unsigned int owner) {
    struct PfxBankLatch* bank_latch;
    struct PfxBank* bank;
    int handle;
    int bank_index;

    for (bank_index = 0; bank_index < 15; bank_index++) {
        bank_latch = &banks[bank_index];
        bank = MK_HDR_LIVE(bank_latch->bank, bank_latch->bank_instance);
        if (bank != 0 && (bank->owner_flags & owner) != 0) {
            handle = bank_find_fx(bank, name);
            if (handle != 0) {
                return handle;
            }
        }
    }
    return 0;
}

static inline void pfx_destroy_bank_effects(struct PfxBank* bank) {
    int effect_index = 0;
    struct PfxEffectLatch* effect_latch;
    struct PfxScriptEffect* effect;

    for (; effect_index < bank->effect_count; effect_index++) {
        bank->effect_owners[effect_index] = 0;
        effect_latch = &bank->effects[effect_index];
        effect = MK_HDR_LIVE(effect_latch->effect, effect_latch->effect_instance);
        if (effect != 0) {
            effect = effect->hdr.vtbl == MK_VTABLE_ADDRESS(vtbl_pfx) ? effect : 0;
            if (effect != 0 && effect->hdr.instance != 0) {
                effect->hdr.typed_vtbl->destroy(&effect->hdr);
            }
        }
    }
}

static void vdestroy_effectbank(struct PfxBank* bank) {
    pfx_destroy_bank_effects(bank);
    bank->hdr.instance = 0;
    mkhdr_memfree(&bank->hdr);
}

/* TODO: [near miss] 98.79518%; GC and PS2 retain the duplicate latch exit;
 * recover its original validation boundary without adding a redundant guard. */
static void resolve_pfx_handle(
    int handle, struct PfxResolvedHandle* resolved) {
    struct PfxBankLatch* bank_latch;
    struct PfxBank* bank;
    struct PfxScriptEffect* effect;
    int bank_index;
    int generation;
    int effect_index;
    int handle_type;

    if (cached_handle == handle) {
        memcpy(resolved, &cached_info, sizeof(cached_info));
        return;
    }

    bank_index = handle & 0xF;
    generation = (unsigned int)handle >> 24;
    effect_index = (handle >> 4) & 0x3FF;
    handle_type = (handle >> 14) & 3;
    if (handle_type != 1 || bank_index < 1 || bank_index > 15 ||
        effect_index < 0) {
        return;
    }

    bank_latch = &banks[bank_index];
    bank = MK_HDR_LIVE(bank_latch[-1].bank, bank_latch[-1].bank_instance);
    if (bank == 0 || bank->handle_bank != bank_index ||
        generation != bank->handle_generation) {
        return;
    }
    if (effect_index < bank->effect_capacity) {
        effect = MK_HDR_LIVE(bank->effects[effect_index].effect,
                             bank->effects[effect_index].effect_instance);
        if (effect != 0) {
            cached_handle = handle;
            cached_info.bank = bank;
            cached_info.effect_index = effect_index;
            cached_info.effect = effect;
            memcpy(resolved, &cached_info, sizeof(cached_info));
        }
    }
}
