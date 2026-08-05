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
#include "libmkparticle/metrics.h"
#include "math/gxVect.h"
#include "platform/main.h"

typedef struct PfxSpawnTable {
    int type;
    int row_count;
    float* values;
} PfxSpawnTable;

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
    struct PfxScriptEmitter* emitter; /* +0x10 */
    int* remaining_effects; /* +0x14 */
    PfxSpawnTableSlot* spawn_tables; /* +0x18 */
    char* effect_name_override; /* +0x1C */
    char* texture_name; /* +0x20 */
    unsigned int initialization_script; /* +0x24 */
    float drag_coefficient;   /* +0x28 */
    float growth_coefficient; /* +0x2C */
    float fields30[2];
    unsigned int kill_percent_field; /* +0x38 */
    int field3C;
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
    union {
        Vec light_direction;
        float light_direction_components[3];
    }; /* +0x154 */
    PfxColor light_color; /* +0x160 */
    Vec light_position; /* +0x164 */
    float z_bias; /* +0x170 */
    struct PfxTextureSlot* texture_slot; /* +0x174 */
    char pad178[0x0A];
    short texture_animation_enabled; /* +0x182 */
    char pad184[0x0C];
    PfxZTestFlags ztest_flags; /* +0x190 */
    PfxParametricFlags parametric_flags; /* +0x191 */
    char pad192[2];
    float decal_plane[6]; /* +0x194 */
    float aspect_x; /* +0x1AC */
    float aspect_y; /* +0x1B0 */
    union {
        PfxColor vertex_color; /* +0x1B4 */
        struct {
            char pad1B4[4];
            float particle_size; /* +0x1B8 */
            float bounding_radius; /* +0x1BC */
            unsigned int behavior_target; /* +0x1C0 */
        };
    };
    char pad1C4[0x10];
    unsigned int runtime_flags; /* +0x1D4 */
    char pad1D8[0x2C];
    struct PfxScriptEmitter* emitter; /* +0x204 */
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
    union {
        Vec direction;
        float direction_components[3];
    };
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
    Vec origin;
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

typedef struct PfxBehaviorInstructionView {
    int type;
    char pad04[0x28];
    void* target; /* +0x2C */
} PfxBehaviorInstructionView;

typedef struct PfxBehaviorView {
    char pad00[0xDC];
    int instruction_count;
    PfxBehaviorInstructionView instructions[1];
} PfxBehaviorView;

typedef struct PfxParametricData {
    float table_a[64]; /* +0x000 */
    int table_a_count; /* +0x100 */
    char pad104[0x0C];
    float table_b[64]; /* +0x110 */
    int table_b_count; /* +0x210 */
    char pad214[0x0C];
    PfxColor colors[64]; /* +0x220 */
    int color_count; /* +0x320 */
    char pad324[0x0C];
    Vec field330;
    float field33C;
    float drag; /* +0x340 */
    float growth; /* +0x344 */
    char pad348[8];
    float field350; /* +0x350 */
} PfxParametricData;

/*
 * Local fx_next_emitter view. The runtime emitter stride and these two effect
 * fields are verified here, but the rest of either runtime type is not.
 */
typedef struct PfxRuntimeEmitterView {
    Vec origin;
    float lifetime; /* +0x0C */
    char pad10[0x0C];
    PfxEmissionFlags emission_flags; /* +0x1C */
    char pad1D[0x1F];
    int cycle_frame; /* +0x3C */
    char pad40[0x2A8];
    void* transform; /* +0x2E8 */
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
    char* name; /* +0x10 */
    unsigned int owner_flags; /* +0x14 */
    int effect_count; /* +0x18 */
    int effect_capacity; /* +0x1C */
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
    union {
        PfxParticleResetStorage* particle_storage; /* +0x148 */
        PfxParametricData* parametric_data;
    };
    char pad14C[0x74];
    int emitter_count; /* +0x1C0 (effect +0x200) */
    PfxRuntimeEmitterView* emitters; /* +0x1C4 (effect +0x204) */
    char pad1C8[4];
    int reset_field_count; /* +0x1CC (effect +0x20C) */
    int** reset_fields; /* +0x1D0 (effect +0x210) */
    char pad1D4[0x50];
    PfxMetrics* metrics; /* +0x224 (effect +0x264) */
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
static const void* g_effect_description;
static void* behavior_buffer;
static void* old_ltm;

static PfxBankLatch banks[16];
static unsigned int cached_handle;
static ScriptSlot* g_pfx_cmo;
static void bank_run_fx(PfxBank* bank);

void* pfx_get_field(void* effect, int emitter, int field);
void* memset(void* destination, int value, unsigned long size);
void* memcpy(void* destination, const void* source, unsigned long size);
int strcmp(const char* left, const char* right);
unsigned long strlen(const char* text);
char* strcpy(char* destination, const char* source);
/* Soft ceiling: 74.91% - exact owned-effect search, four-instruction residue. */
static unsigned int banks_find_owned_fx(
    const char* name, unsigned int owner);
void pfxvm_kill_percent(unsigned int field);
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
void pfxvm_spawn_value(PfxScriptEmitter* emitter, int field, ...);
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
void pfxvm_spawn_set_field_from_table(PfxScriptEmitter* emitter, int field);
void pfxvm_kill_on_y_less_than_field(int context, int field);
void pfxvm_update_attract(int context, int field);
void pfxvm_update_bounce(int context, int field, int source);
void pfxvm_update_fade_alpha(
    int context, int field, int start_alpha, int end_alpha);
void pfxvm_update_lerp_color(
    unsigned int context, int destination, int source, int start, int end,
    PfxColor* colors, float amount);
void pfxvm_update_animate_texture(
    int context, int field, int first, int last, int loop, int advance);
void pfx_emitter_restart_cycle(
    PfxScriptEmitter* emitter, int cycle, PfxScriptEmitter* source);
PfxScriptEmitter* pfx_get_emitter(void* emitters, int index);
PfxScriptEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(PfxScriptEffect* effect);
int pfx_emitter_exhausted(PfxRuntimeEmitterView* emitter);
int pfx_frame_begin(PfxResetRuntimeView* effect, float frame);
void pfx_frame_end(PfxResetRuntimeView* effect);
void pfx_parametric_spawn(PfxResetRuntimeView* effect, float speed);
void pfx_parametric_update(PfxResetRuntimeView* effect, float speed);
void pfx_run(PfxResetRuntimeView* effect, float speed);
void pfxvm_create_transfer(
    PfxResetRuntimeView* destination, PfxResetRuntimeView* source);
void pfx_register_table(void* effect, int index, PfxSpawnTable* table);
void pfx_emitter_scan_for_fields(
    PfxRuntimeEmitterView* emitter, unsigned int* fields);
void pfxvm_compile(PfxResetRuntimeView* effect);
void pfx_render_set_blendmode(PfxResetRuntimeView* effect, int blend_mode);
int pfx_verify(PfxResetRuntimeView* effect);
void pfx_behavior_scan_fields(
    PfxBehaviorView* behavior, unsigned int* fields,
    unsigned int* flags);
void pfxvm_update_age(PfxBehaviorView* behavior, int field);
void pfxvm_update_make_last_insn_first(PfxBehaviorView* behavior);
void pfx_copy_behavior_list(
    PfxResetRuntimeView* effect, int count, PfxBehaviorView* behaviors);
void pfx_behaviors_fixup_targets(
    int** targets, PfxBehaviorView** behaviors, int count);
PfxBehaviorView* pfx_behavior(PfxResetRuntimeView* effect, int index);

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

int pfx_emitter_unused(PfxRuntimeEmitterView* emitter);
void pfx_emitter_reset(PfxRuntimeEmitterView* emitter);
static float p_update_effects(void);
void pfx_texture_animate();

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
                PfxEmitterEffectView* emitter_effect;
                int emitter_index;

                effect = resolve_effect_handle(handle);
                if (effect != 0) {
                    emitter_effect = (PfxEmitterEffectView*)effect;
                    emitter_index = (handle >> 16) & 0xF;
                    if (emitter_index < emitter_effect->emitter_count) {
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

void kill_on_y_less_than_field(int context, int field) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_kill_on_y_less_than_field(context, field);
    }
}

void change_on_y_less_than_field(int field, int source) {
    if (active_pfx_environment()->kill_percent_field != 0 &&
        active_pfx_environment()->field3C != 0) {
        pfxvm_change_on_y_less_than_field(field, source);
    }
}

void change_on_y_less(int unused) {
    if (active_pfx_environment()->kill_percent_field != 0 &&
        active_pfx_environment()->field3C != 0) {
        pfxvm_change_on_y_less();
    }
}

void change_on_less(int unused) {
    if (active_pfx_environment()->kill_percent_field != 0 &&
        active_pfx_environment()->field3C != 0) {
        pfxvm_change_on_less();
    }
}

void change_on_greater(int unused) {
    if (active_pfx_environment()->kill_percent_field != 0 &&
        active_pfx_environment()->field3C != 0) {
        pfxvm_change_on_greater();
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

void update_texanim_hold(int context, int field, int first, int last) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_animate_texture(context, field, first, last, 0, 0);
    }
}

void update_texanim(int context, int field, int first, int last) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_animate_texture(context, field, first, last, 0, 1);
    }
}

void update_fade_alpha2(int context, int field, int start, int end) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_fade_alpha(context, field, start, end);
    }
}

/* Soft ceiling: 76.51% - exact packed-color loop, two-instruction residue. */
void update_lerp_color(
    int destination, int source, int start, int end,
    const PfxScriptColorRow* table, float amount) {
    PfxScriptEnvironment* environment;
    PfxColor* colors;
    PfxColor* color;
    const PfxScriptColorRow* row_data;
    unsigned int row_count;
    int remaining;

    environment = active_pfx_environment();
    if (environment->kill_percent_field == 0U || g_pfx_cmo == 0) {
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
        environment->kill_percent_field, destination, source, start, end,
        colors, amount);
}

void update_fade_alpha(int context, int field) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_fade_alpha(context, field, 0xFF, 0);
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

void update_bounce(int context, int field, int source) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_bounce(context, field, source);
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

void update_attract(int context, int field) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment->kill_percent_field != 0U) {
        pfxvm_update_attract(context, field);
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
    PfxRuntimeEmitterView emitter_template;
    PfxScriptEffect* effect;
    PfxResetRuntimeView* runtime;
    PfxRuntimeEmitterView* emitter;
    PfxRuntimeEmitterView* first_emitter;
    PfxBehaviorView* behavior;
    PfxBehaviorView* behavior_targets[10];
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
    build.flag = update;
    if (description->emitter->origin.x != 0.0f ||
        description->emitter->origin.y != 0.0f ||
        description->emitter->origin.z != 0.0f) {
        build.field_0C = 0x80000000;
    }
    if (g_profile_enabled != 0) {
        build.field_04 = 0x384;
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
    environment->emitter = (PfxScriptEmitter*)&emitter_template;
    push_script_stack_frame(0);
    environment = active_pfx_environment();
    cmdscript_setup_execution(script, environment->initialization_script);
    cmdscript_execute(script);
    environment = active_pfx_environment();
    environment->emitter = 0;
    g_pfx_cmo = 0;

    behavior_count = get_row_count_for_table_by_pointer(
        script, description->behavior_scripts);
    build.field_00 = behavior_count;
    environment = active_pfx_environment();
    environment->kill_percent_field = behavior_count;
    if (behavior_count == 0U) {
        return;
    }

    g_pfx_cmo = script;
    for (behavior_index = 0;
         behavior_index < behavior_count;
         behavior_index++) {
        behavior = (PfxBehaviorView*)((char*)behavior_buffer +
                                      behavior_index * 0x388);
        memset(behavior, 0, 0x388);
        environment = active_pfx_environment();
        environment->kill_percent_field = (unsigned int)behavior;
        if (behavior_index + 1 < behavior_count) {
            environment = active_pfx_environment();
            environment->field3C =
                (int)((char*)behavior_buffer +
                      (behavior_index + 1) * 0x388);
        } else {
            environment = active_pfx_environment();
            environment->field3C = 0;
        }
        push_script_stack_frame(0);
        cmdscript_setup_execution(
            script, description->behavior_scripts[behavior_index]);
        cmdscript_execute(script);
    }
    g_pfx_cmo = 0;

    pfx_behavior_scan_fields(
        (PfxBehaviorView*)behavior_buffer,
        behavior_fields, behavior_flags);
    for (behavior_index = 1;
         behavior_index < behavior_count;
         behavior_index++) {
        behavior = (PfxBehaviorView*)((char*)behavior_buffer +
                                      behavior_index * 0x388);
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
            behavior = (PfxBehaviorView*)((char*)behavior_buffer +
                                          behavior_index * 0x388);
            pfxvm_update_age(behavior, 0x301);
            pfxvm_update_make_last_insn_first(behavior);
        }
        pfxvm_spawn_value(
            (PfxScriptEmitter*)&emitter_template, 0x301, 0.0f);
        behavior_flags[1] |= 2;
        scan_fields[1] |= 2;
    }

    environment = active_pfx_environment();
    environment->emitter = (PfxScriptEmitter*)&emitter_template;
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

    runtime = (PfxResetRuntimeView*)effect->emitters;
    environment = active_pfx_environment();
    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
                runtime, emitter_index);
            transform = emitter->transform;
            environment = active_pfx_environment();
            memcpy(emitter, environment->emitter, sizeof(*emitter));
            emitter->transform = transform;
        }
        environment = active_pfx_environment();
        environment->emitter = (PfxScriptEmitter*)runtime->emitters;
    }
    g_pfx_cmo = 0;

    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        environment = active_pfx_environment();
        if (table_slots[emitter_index].type != 0) {
            pfx_register_table(
                runtime, emitter_index, table_slots[emitter_index].table);
        }
    }
    pfx_copy_behavior_list(
        runtime, behavior_count, (PfxBehaviorView*)behavior_buffer);
    for (behavior_index = 0; behavior_index < 10; behavior_index++) {
        behavior_targets[behavior_index] =
            (PfxBehaviorView*)((char*)behavior_buffer +
                              behavior_index * 0x388);
    }
    pfx_behaviors_fixup_targets(
        runtime->reset_fields, behavior_targets, behavior_count);
    for (behavior_index = 0;
         behavior_index < (unsigned int)runtime->reset_field_count;
         behavior_index++) {
        behavior = pfx_behavior(runtime, behavior_index);
        if (behavior != 0) {
            for (instruction_index = 0;
                 instruction_index <
                     (unsigned int)behavior->instruction_count;
                 instruction_index++) {
                if (behavior->instructions[instruction_index].type == 0xE) {
                    behavior->instructions[instruction_index].target =
                        &effect->behavior_target;
                }
            }
        }
    }
    pfxvm_compile(runtime);

    first_emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(runtime, 0);
    first_emitter->origin = description->emitter->origin;
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
    first_emitter->emission_flags.bits.cycle_paused = 1;
    effect->lifecycle_flags.bits.restart_cycle = 0;
    environment = active_pfx_environment();
    environment->source_effect = (MkPfx*)effect;
    effect->effect_id = description->effect_id;
    first_emitter->lifetime = description->emitter_lifetime;
    for (emitter_index = 1;
         emitter_index < runtime->emitter_count;
         emitter_index++) {
        emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
            runtime, emitter_index);
        transform = emitter->transform;
        memcpy(emitter, first_emitter, sizeof(*emitter));
        emitter->transform = transform;
    }

    effect->vertex_color.r = 0xFF;
    effect->vertex_color.g = 0xFF;
    effect->vertex_color.b = 0xFF;
    effect->vertex_color.a = 0xFF;
    effect->flags.bits.vertex_color_enabled = 1;
    pfx_render_set_blendmode(runtime, description->blend_mode);
    if (description->texture->frame_count > 1 &&
        effect->texture_slot != 0) {
        PfxTextureInfo* texture = effect->texture_slot->info;
        float width = (float)texture->width;
        float scale = description->texture->horizontal_scale;

        if (effect->initialization_mode != 0) {
            effect->runtime_flags |= 0x100;
        }
        pfx_texture_animate(
            effect, (int)width, (int)(scale * width),
            (int)((float)texture->height /
                  (scale * (float)description->texture->frame_count)),
            0.0f);
        effect->texture_animation_enabled = 1;
    }

    if (pfx_verify(runtime) != 0) {
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
            pfx_emitter_reset((PfxRuntimeEmitterView*)pfx_get_emitter(
                runtime, emitter_index));
        }
        environment = active_pfx_environment();
        environment->spawn_tables = 0;
    }
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

static inline PfxRuntimeEmitterView* emitter_from_handle(unsigned int handle) {
    PfxScriptEffect* effect;
    PfxResetRuntimeView* runtime;
    unsigned int emitter_index;

    effect = resolve_effect_handle(handle);
    if (effect == 0) {
        return 0;
    }

    runtime = (PfxResetRuntimeView*)effect->emitters;
    emitter_index = (handle >> 16) & 0xF;
    if (emitter_index >= (unsigned int)runtime->emitter_count) {
        return 0;
    }
    return &runtime->emitters[emitter_index];
}

/* Soft ceiling: 75.55% - four-instruction inline lookup branch residue. */
void fx_restart_emit(unsigned int handle) {
    PfxRuntimeEmitterView* emitter;
    PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter->emission_flags.bits.cycle_paused = 0;
        emitter->cycle_frame = 0;
        pfx_emitter_restart_cycle(
            (PfxScriptEmitter*)emitter, 0, (PfxScriptEmitter*)effect);
    }
}

/* Soft ceiling: 75.06% - three-instruction inline lookup branch residue. */
void fx_reset_emit(unsigned int handle) {
    PfxRuntimeEmitterView* emitter;

    emitter = emitter_from_handle(handle);
    /* Refresh the validated effect latch before mutating its emitter. */
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->emission_flags.bits.cycle_paused = 1;
        emitter->cycle_frame = 0;
        pfx_emitter_reset(emitter);
    }
}

void restart_effect(const char* name) {
    PfxScriptEffect* effect;
    PfxScriptEmitter* emitter;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        emitter = effect->emitter;
        effect->lifecycle_flags.bits.restart_cycle = 1;
        emitter->emission_flags.bits.cycle_paused = 0;
        emitter->cycle_frame = 0;
        pfx_emitter_restart_cycle(emitter, 0, emitter);
    }
}

void restart_effect_ppfx(PfxScriptEffect* effect) {
    PfxScriptEmitter* emitter = effect->emitter;

    effect->lifecycle_flags.bits.restart_cycle = 1;
    emitter->emission_flags.bits.cycle_paused = 0;
    emitter->cycle_frame = 0;
    pfx_emitter_restart_cycle(emitter, 0, emitter);
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

/* Soft ceiling: 73.06% - three-instruction inline lookup branch residue. */
void fx_pause_emit(unsigned int handle) {
    PfxRuntimeEmitterView* emitter;

    emitter = emitter_from_handle(handle);
    /* Refresh the validated effect latch before mutating its emitter. */
    resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->emission_flags.bits.cycle_paused = 1;
    }
}

/* Soft ceiling: 76.37% - three-instruction inline lookup branch residue. */
void fx_resume_emit(unsigned int handle) {
    PfxRuntimeEmitterView* emitter;
    PfxScriptEffect* effect;

    emitter = emitter_from_handle(handle);
    effect = resolve_effect_handle(handle);
    if (emitter != 0) {
        emitter->emission_flags.bits.cycle_paused = 0;
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

/* Soft ceiling: 76.23% - exact table-slot setup, one-instruction residue. */
void spawn_random_size(const float* table) {
    PfxScriptEnvironment* environment;
    PfxScriptEmitter* emitter;
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
    copied_table->values = (float*)(copied_table + 1);
    copied_table->row_count = row_count;
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
        emitter, environment->field04 != 0 ? 0x402 : 0x102);
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

void emit_from_pos_clamp_y(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_from_pos(field, source, 1, 0.0f);
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

void emit_spherical_from_boundary(int unused, float radius) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_sphere(0, 0.0f, 0.0f, 0.0f, radius);
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

void emit_cuboid(int unused, float x, float y, float z) {
    PfxScriptEnvironment* environment = active_pfx_environment();

    if (environment != 0 && environment->emitter != 0) {
        pfxvm_spawn_box(-x * 0.5f, -y * 0.5f, -z * 0.5f,
                        x, y, z);
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

void emit_roundrobin_mechanism(int field, int source) {
    PfxScriptEnvironment* environment = 0;

    if (pfxscript_environment.active != 0) {
        environment = &pfxscript_environment;
    }
    if (environment->emitter != 0) {
        pfxvm_spawn_roundrobin_mechanism(field, source);
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

    behavior_buffer = get_mem(0x2350);
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
    PfxResetRuntimeView* runtime;
    PfxRuntimeEmitterView emitter_template;
    PfxRuntimeEmitterView* emitter;
    PfxRuntimeEmitterView* first_emitter;
    PfxParametricData* data;
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
    build.flag = update;
    if (g_profile_enabled != 0) {
        build.field_04 = 0x384;
    }

    emitter_description = description->emitter;
    if (emitter_description->origin.x != 0.0f ||
        emitter_description->origin.y != 0.0f ||
        emitter_description->origin.z != 0.0f) {
        build.field_0C = 0x80000000;
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
    environment->kill_percent_field = 0;
    environment->fields30[0] = 0.0f;
    environment->fields30[1] = 0.0f;
    environment->texture_name = description->texture_name;
    environment->initialization_script =
        emitter_description->initialization_script;
    if (environment->initialization_script == 0U) {
        return;
    }

    g_effect_description = description;
    g_pfx_cmo = script;
    environment->field04 = 1;
    environment->emitter = 0;
    effect = 0;
    new_pfx_create_raw_userdata(
        &build, 0, (int)(1.1f * description->allocation_count),
        create_flags, 0, (PfxInitCb)initialize_effect, 0, 0,
        (void**)&effect);
    if (effect == 0) {
        g_effect_description = 0;
        g_pfx_cmo = 0;
        return;
    }

    runtime = (PfxResetRuntimeView*)effect->emitters;
    if (environment->emitter != 0) {
        for (emitter_index = 0;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
                runtime, emitter_index);
            transform = emitter->transform;
            memcpy(emitter, environment->emitter, sizeof(*emitter));
            emitter->transform = transform;
        }
        environment->emitter = (PfxScriptEmitter*)runtime->emitters;
    }
    for (emitter_index = 0; emitter_index < 2; emitter_index++) {
        if (table_slots[emitter_index].type != 0) {
            pfx_register_table(
                runtime, emitter_index, table_slots[emitter_index].table);
        }
    }
    g_effect_description = 0;
    g_pfx_cmo = 0;

    emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(runtime, 0);
    first_emitter = emitter;
    pfx_emitter_scan_for_fields(emitter, scan_fields);
    if ((scan_fields[0] & 0x40) != 0) {
        effect->parametric_flags.bits.scan_flag_40 = 1;
    }
    if ((scan_fields[0] & 0x20) != 0) {
        effect->parametric_flags.bits.scan_flag_20 = 1;
    }
    pfxvm_compile(runtime);

    emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(runtime, 0);
    first_emitter = emitter;
    emitter->origin = emitter_description->origin;
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
        emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
            runtime, emitter_index);
        emitter->emission_flags.bits.cycle_paused = 1;
    }
    effect->lifecycle_flags.bits.restart_cycle = 0;
    environment->source_effect = (MkPfx*)effect;

    data = runtime->parametric_data;
    if (data != 0) {
        data->field330.x = 0.0f;
        data->field330.y = 0.0f;
        data->field330.z = 0.0f;
        data->field33C = description->field2C;
        data->field350 = description->field34;
        first_emitter->lifetime = description->emitter_lifetime;
        for (emitter_index = 1;
             emitter_index < runtime->emitter_count;
             emitter_index++) {
            emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
                runtime, emitter_index);
            transform = emitter->transform;
            memcpy(emitter, first_emitter, sizeof(*emitter));
            emitter->transform = transform;
        }

        if (description->color_table != 0) {
            row_count = get_row_count_for_table_by_pointer(
                script, description->color_table);
            if (row_count > 1U) {
                data->color_count = row_count;
                for (row = 0; row < row_count; row++) {
                    data->colors[row].r = description->color_table[row].red;
                    data->colors[row].g = description->color_table[row].green;
                    data->colors[row].b = description->color_table[row].blue;
                    data->colors[row].a = description->color_table[row].alpha;
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
            data->table_a_count = get_row_count_for_table_by_pointer(
                script, description->table_a);
            for (row = 0; row < (unsigned int)data->table_a_count; row++) {
                data->table_a[row] = description->table_a[row];
            }
        }
        if (description->table_b != 0) {
            data->table_b_count = get_row_count_for_table_by_pointer(
                script, description->table_b);
            for (row = 0; row < (unsigned int)data->table_b_count; row++) {
                data->table_b[row] = description->table_b[row];
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
        data->field330.x = description->field1C;
        data->field330.y = description->field20;
        data->field330.z = description->field24;
        data->drag = environment->fields30[0];
        data->growth = environment->fields30[1];
    }
    pfx_render_set_blendmode(runtime, description->blend_mode);

    if (pfx_verify(runtime) != 0) {
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
            pfx_emitter_reset((PfxRuntimeEmitterView*)pfx_get_emitter(
                runtime, emitter_index));
        }
        environment->spawn_tables = 0;
    }
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
                PfxBankVtableRef vtbl;

                vtbl.base = bank->hdr.vtbl;
                vtbl.bank->destroy(&bank->hdr);
            }
            banks[index].bank = 0;
            banks[index].bank_instance = 0;
        }
    }
}

static inline void bank_destroy(MkHdr* bank) {
    PfxBankVtableRef vtbl;

    if (bank->instance != 0U) {
        vtbl.base = bank->vtbl;
        vtbl.bank->destroy(bank);
    }
}

/* Soft ceiling: 91.81% - exact runtime loop, one-instruction residue. */
static void bank_run_fx(PfxBank* bank) {
    PfxEffectLatch* effect_latch;
    PfxScriptEffect* raw_effect;
    PfxScriptEffect* effect;
    PfxResetRuntimeView* runtime;
    PfxResolvedHandle transfer;
    PfxRuntimeEmitterView* emitter;
    float frame;
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

        runtime = (PfxResetRuntimeView*)effect->emitters;
        if (effect->parametric != 0) {
            frame = runtime->reset_time;
            runtime->reset_time = frame + game_speed;
            if (pfx_frame_begin(runtime, frame) == 0) {
                pfx_parametric_spawn(runtime, game_speed);
                pfx_parametric_update(runtime, game_speed);
            } else {
                runtime->reset_active = 0;
            }
            pfx_frame_end(runtime);
        } else {
            pfx_run(runtime, game_speed);
            if (bank->effect_owners[effect_index] != 0) {
                resolve_pfx_handle(
                    bank->effect_owners[effect_index], &transfer);
                if (transfer.effect != 0) {
                    pfxvm_create_transfer(
                        (PfxResetRuntimeView*)transfer.effect->emitters,
                        runtime);
                    transfer.effect->lifecycle_flags.bits.restart_cycle = 1;
                }
            }
        }

        if (runtime->reset_active == 0) {
            for (emitter_index = 0;
                 emitter_index < runtime->emitter_count;
                 emitter_index++) {
                emitter = (PfxRuntimeEmitterView*)pfx_get_emitter(
                    runtime, emitter_index);
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
