#include "runtime/mk_obj.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pebble.h"
#include "runtime/asset.h"
#include "runtime/cstring.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/cam.h"
#include "runtime/sound_tracker.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "game/game_info.h"
#include "game/mab.h"
#include "game/plyr.h"
#include "game/cloth.h"
#include "game/specular.h"
#include "game/weapon.h"
#include "math/mk_math.h"
#include "platform/display.h"
#include "platform/fog.h"
#include "rw/rtquat.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"
#include "platform/gcutils.h"
#include "platform/main.h"

extern MkObj* his_obj;
extern MkObj* plyr_obj;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern PlyrPdata* his_pdata;
extern int f_fatality_was_done;
extern float game_speed;
extern unsigned short GXMathSqrtTable[];
extern float do_my_fatality(void);
extern float do_my_2nd_fatality(void);
extern float xz_distance_between_players(void);
extern float p_anim_idle(void);
extern int f_fatality_finished;
extern float p_animate_and_freeze(void);
extern float p_idle_camera(void);
extern MkFileEntry fatalityanims_file_table[];
extern void unimpale_victim(PlyrPdata* victim);
extern void reload_fan(PlyrPdata* player);
extern void run_reaction_cleanup_function(PlyrPdata* player);
extern void stop_tunes(void);
extern int is_a_to_the_right_of_b(void* a, void* b);

static unsigned int fatality_anim_script;
int fatality_anim_slot;
static MslSoundHandle fatality_loop_sound;

typedef struct FatalityProjectilePdata {
    MkHdr hdr;
    PlyrPdata* owner;
    PlyrPdata* opponent;
    struct FatalityProjectilePdata* retarget_source;
    MkObj* retarget_object;
    MkProc* process;
    unsigned int process_instance;
    char pad20[8];
    MkObj* object;
    unsigned int object_instance;
    float max_ticks;
} FatalityProjectilePdata;

typedef union FatalityDistancePdataRef {
    MkHdr* hdr;
    FatalityProjectilePdata* projectile;
} FatalityDistancePdataRef;

typedef float (*FatalityJumpSleepFn)(MkProcEntryFn entry, float ticks);

typedef struct FatalityProcVtable {
    void* reserved[6];
    void (*sleep)(void);
    void* reserved1C[2];
    FatalityJumpSleepFn jump_sleep;
} FatalityProcVtable;

typedef union FatalityProcVtableRef {
    MkVtableMkproc* base;
    FatalityProcVtable* fatality;
} FatalityProcVtableRef;

typedef struct FatalityFakeBoneMatcher FatalityFakeBoneMatcher;

typedef struct RaidenLightningBoltPdata {
    MkHdr hdr;
    PlyrPdata* owner; /* +0x08 */
    MkObj* bolt;      /* +0x0C */
    unsigned int bolt_instance; /* +0x10 */
    int frame;        /* +0x14 */
    int bone_id;      /* +0x18 */
    int orientation;  /* +0x1C */
    unsigned int alpha; /* +0x20 */
    unsigned int fade_step; /* +0x24 */
} RaidenLightningBoltPdata;

typedef struct FatalityState {
    PlyrInfo* context; /* +0x00 */
    MkObj* attacker_object; /* +0x04 */
    MkProc* process; /* +0x08 */
    MkProc* animation_process; /* +0x0C */
    AnimPdata* animation_pdata; /* +0x10 */
    PlyrPdata* player; /* +0x14 */
    PlyrInfo* player_info; /* +0x18 */
    MkObj* victim_object; /* +0x1C */
    MkProc* victim_proc; /* +0x20 */
    MkProc* attacker_proc; /* +0x24 */
    struct FatalityAnimationView* animation; /* +0x28 */
    PlyrPdata* opponent; /* +0x2C */
    int mirror_camera; /* +0x30 */
    union {
        struct {
            char pad34[4];
            MkHdr* bone_matcher; /* +0x38 */
            char pad3C[2];
        } fields34;
        unsigned char reset34[10];
    } range34;
    char pad3E[0x1E];
    FatalityFakeBoneMatcher* fake_bone_matcher; /* +0x5C */
    char pad60[0x24];
    unsigned char reset84[10];
    char pad8E[0x1E];
    unsigned char resetAC[10];
    char padB6[0x1A];
} FatalityState;

typedef struct FatalityAnimationView {
    char pad00[0xB4];
    float movement_scale;
} FatalityAnimationView;

typedef struct FatalityObjectLatch {
    int active;
    char pad04[0x10];
    MkHdr* object;
    unsigned int object_instance;
} FatalityObjectLatch;

typedef struct FatalityScalePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float maximum;
    float step;
} FatalityScalePdata;

typedef struct FatalityObjectMatcherPdata {
    MkHdr hdr;
    MkObj* source;
    unsigned int source_instance;
    MkObj* destination;
    unsigned int destination_instance;
    float blend;
} FatalityObjectMatcherPdata;

typedef union FatalityBoneMatcherFlags {
    unsigned char value;
    struct {
        unsigned char inactive : 1;
        unsigned char copy_bone_matrix : 1;
        unsigned char copy_clone_matrix : 1;
        unsigned char preserve_bone_matrix : 1;
        unsigned char copy_parent_angles : 1;
        unsigned char flip_parent_angle_y : 1;
        unsigned char release_parent_weight : 1;
        unsigned char blend_child_transform : 1;
    } bits;
} FatalityBoneMatcherFlags;

typedef struct FatalityBoneMatcher {
    MkHdr hdr;
    FatalityBoneMatcherFlags flags;
} FatalityBoneMatcher;

typedef struct FatalityGroundBouncePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float ground_offset;
    int bounce_count;
    float restitution;
} FatalityGroundBouncePdata;

typedef struct FatalityBodySplatPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float current_scale;
    float target_scale;
    float step;
} FatalityBodySplatPdata;

typedef struct FatalityObjectScalarPdata {
    MkHdr hdr;
    union {
        unsigned char flags;
        struct {
            signed char stop_when_complete : 1; /* bit7 */
            signed char multiply : 1;           /* bit6 */
            signed char ping_pong : 1;          /* bit5 */
            signed char swap_limits : 1;        /* bit4 */
            signed char pad_low : 4;
        } flag_bits;
    };
    char pad09[3];
    MkObj* object;
    unsigned int object_instance;
    Vec start;
    Vec target;
    Vec step;
} FatalityObjectScalarPdata;

typedef struct FatalityFacingController {
    char pad00[0x20];
    unsigned char flags20;
    char pad21[7];
    Vec angles; /* +0x28 */
} FatalityFacingController;

typedef struct FatalityFaceObjectPdata {
    MkHdr hdr;
    FatalityFacingController* controller;
    MkObj* source;
    unsigned int source_instance;
    MkObj* target;
    unsigned int target_instance;
    int source_bone;
    int target_bone;
} FatalityFaceObjectPdata;

typedef union FatalityFakeMatcherFlags {
    unsigned char value;
    struct {
        unsigned char has_parent_offset : 1;
        unsigned char has_child_offset : 1;
        unsigned char has_rotation : 1;
        unsigned char mode : 1;
        unsigned char pad_low : 4;
    } bits;
} FatalityFakeMatcherFlags;

struct FatalityFakeBoneMatcher {
    MkHdr hdr;
    MkProc* process;
    unsigned int process_instance;
    MkObj* parent;
    unsigned int parent_instance;
    MkObj* child;
    unsigned int child_instance;
    FatalityFakeMatcherFlags flags;
    char pad21[3];
    int child_bone;
    Vec parent_offset;
    Vec child_offset;
    Vec rotation;
    float blend;
    float blend_target;
    Vec parent_position;
};

typedef struct FatalityLightningFlashPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    struct FatalityUvScrollControl* scroll;
    float scroll_step;
    int frame;
    float flash_range;
} FatalityLightningFlashPdata;

typedef struct FatalityUvScrollControl {
    char pad00[0x1C];
    float step;
} FatalityUvScrollControl;

typedef struct FatalityLightningScrollPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    Vec scalar_start;
    Vec scalar_target;
    Vec scalar_step;
    int frame;
    struct FatalityLightningScaling* scaling;
} FatalityLightningScrollPdata;

typedef struct FatalityLightningScaling {
    const float* x;
    const float* y;
    const float* z;
} FatalityLightningScaling;

struct FatalityWeaponSource {
    char pad00[0x58];
    PlyrPdata* owner;
    MkObj* player_object;
};

typedef struct FatalityWeaponReflectionSet {
    char pad00[0x14];
    MkObj* primary;
    unsigned int primary_instance;
    char pad1C[0x10];
    MkObj* secondary;
    unsigned int secondary_instance;
} FatalityWeaponReflectionSet;

typedef struct FatalityWeaponAttachment {
    MkHdr hdr;
    unsigned char flags08;
    char pad09[7];
    MkHdr* bound_object;
    unsigned int bound_instance;
    int bone_id;
    Vec position;
    char pad28[0x14];
    Vec velocity;
} FatalityWeaponAttachment;

typedef struct FatalityIceChunkPebble {
    MkHdr hdr;
    RwMatrix* matrices;
    char pad0C[4];
    int matrix_count;
} FatalityIceChunkPebble;

typedef struct FatalityIceChunkPdata {
    MkHdr hdr;
    MkObj* owner;
    unsigned int owner_instance;
    MkObj* model;
    unsigned int model_instance;
    FatalityIceChunkPebble* chunks[9];
} FatalityIceChunkPdata;

typedef struct FatalityIceChunkMap {
    int subobject_id;
    int pebble_id;
} FatalityIceChunkMap;

typedef struct FatalityIceblockAlphaPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    RwRGBA color;
    int active_material;
    unsigned char material_alpha[6];
    char pad1E[2];
} FatalityIceblockAlphaPdata;

typedef struct FatalitySonicWavePdata {
    MkHdr hdr;
    int finished;
    MkObj* owner;
    unsigned int owner_instance;
    MkObj* projectile;
    unsigned int projectile_instance;
    PlyrMirrorObjLatch waves[5];
    MkHdr* pebble;
    Vec wave_offsets[5];
    Vec wave_positions[5];
    float wave_progress[5];
    Vec origin;
    Vec velocity;
    int frame;
    float field_F0;
    float duration;
    MslSoundHandle sound;
    MslSoundHandle loop_sound;
    unsigned int player_mask;
} FatalitySonicWavePdata;

typedef struct FatalitySonicPebble {
    MkHdr hdr;
    RwMatrix* matrices;
} FatalitySonicPebble;

typedef struct FatalityIceSubobject {
    MkHdr hdr;
    unsigned char flags08;
    unsigned char flags09;
    char pad0A[0x1E];
    float field28;
    char pad2C[4];
    Vec scale;
} FatalityIceSubobject;

typedef struct FatalityEmitterBind {
    int bone_id;
    Vec position;
} FatalityEmitterBind;

typedef struct FatalityObjectGroupView {
    char pad00[0x2C];
    int group;
} FatalityObjectGroupView;

typedef struct FatalityContextView {
    char pad00[0x14];
    unsigned char flags14;
} FatalityContextView;

typedef struct FatalityBgndScriptView {
    char pad00[8];
    unsigned int function;
} FatalityBgndScriptView;

int veil_bones[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
int _veil_flipped_bones[9] = {6, 7, 8, 3, 4, 5, 0, 1, 2};
static float raiden_lighning_scaler1[3] = {0.5f, 0.5f, 1.0f};
static float raiden_lighning_scaler2[3] = {0.7f, 0.7f, 1.3f};
extern float raiden_lightning_speed1[2];
extern float raiden_lightning_sleep1[2];
extern float raiden_lightning_speed2[2];
extern float raiden_lightning_sleep2[2];
FatalityLightningScaling raiden_scaling_data[3] = {
    {raiden_lighning_scaler1, raiden_lightning_speed1,
     raiden_lightning_sleep1},
    {raiden_lighning_scaler1, raiden_lightning_speed1,
     raiden_lightning_sleep1},
    {raiden_lighning_scaler2, raiden_lightning_speed2,
     raiden_lightning_sleep2}
};
static Vec scale_proj = {1.0f, 0.1f, 0.1f};
FatalityIceChunkMap sz_hk_icechunk_map[9] = {
    {1, 2}, {2, 2}, {3, 2}, {4, 1}, {5, 1},
    {6, 1}, {7, 2}, {8, 2}, {9, 2}
};
int sz_hk_icechunk_map_array[15] = {
    10, 7, 11, 8, 12, 9, 13, 14, 0, 6, 3, 5, 2, 4, 1
};
Vec sz_ice_chunk_ang_fix[15] = {
    {0.707f, -1.57079637f, 0.0f},
    {0.0f, 3.0f, -0.3f},
    {0.0f, 2.7f, -0.2f},
    {3.14159274f, 1.57079637f, 0.0f},
    {0.0f, 3.0f, -0.3f},
    {0.0f, 3.8f, 0.2f},
    {0.707f, -1.57079637f, 0.0f},
    {2.2f, -1.05f, 0.0f},
    {1.9f, -1.57079637f, 0.0f},
    {1.57079637f, 1.57079637f, 0.0f},
    {1.9f, -1.4f, 0.0f},
    {0.0f, 0.1f, 2.0f},
    {0.785398185f, -0.4f, 1.57079637f},
    {1.57079637f, 0.0f, 1.57079637f},
    {1.57079637f, 0.0f, 1.57079637f}
};
Vec sz_ice_chunk_pos_fix[15] = {
    {-0.05f, 0.05f, 0.0f},
    {-0.2f, -0.05f, -0.11f},
    {-0.12f, -0.025f, -0.06f},
    {-0.025f, -0.025f, -0.15f},
    {-0.2f, -0.025f, 0.07f},
    {-0.15f, 0.0f, 0.05f},
    {-0.025f, -0.025f, 0.12f},
    {0.12f, 0.0f, -0.03f},
    {0.22f, 0.08f, 0.0f},
    {0.32f, -0.1f, -0.03f},
    {0.1f, -0.07f, -0.025f},
    {0.2f, 0.125f, -0.025f},
    {0.35f, -0.05f, 0.07f},
    {0.05f, 0.01f, 0.0f},
    {-0.35f, -0.025f, -0.025f}
};
MkFlippedBoneMap veil_flipped_bones = {
    9, _veil_flipped_bones
};
float raiden_lightning_speed1[2] = {1.0f, 0.03f};
float raiden_lightning_sleep1[2] = {10.0f, 500.0f};
float raiden_lightning_speed2[2] = {1.0f, 0.01f};
float raiden_lightning_sleep2[2] = {15.0f, 500.0f};

/*
 * Soft ceiling: retail appends anonymous 4-byte gaps to .data, .bss, and
 * .sbss plus a 2-byte .rodata gap. They have no symbol users or relocations;
 * portable C would classify explicit gap objects into small-data sections,
 * so the gaps remain linker-emission residue instead of forced sections.
 */

static FatalityState fatality_state;
static float SD_SONIC_WAVES_TBS = 8.0f;
static float SD_SONIC_WAVE_DIST_SCALE = 5.0f;
static float SD_SONIC_WAVE_SCALE_GROWTH = 1.35f;

static float p_3d_distance_handler(void);
MslSoundHandle plyr_snd_req(int sound_offset);
RpMaterial* obj_find_material_by_id(MkObj* object, int id);
extern float fog_distance;
extern float fog_color_real[4];
extern int fog_on;
extern int fog_type;
extern CameraObj* camera_obj;
extern CameraItem camera_item;
void snd_stop(MslSoundHandle handle);
MslSoundHandle snd_req(int sound);
void freeze_player(void);

static inline MkObj* fatality_get_severed_limb(
    FighterMirror* fighter, int index) {
    FighterObjectRef* latch;
    MkObj* object;

    latch = &fighter->severed_limbs[index];
    object = latch->object;
    if (object != 0 && object->hdr.instance != latch->instance) {
        object = 0;
    }
    return object;
}

static void start_3d_projectile_iceball(MkProcEntryFn entry);
int get_bid_with_flip(MkObj* object, unsigned int bone_id);
MkObj* start_projectile_from_plyr_bone(
    int model_id, int bone_id, const char* model_name, float x_offset,
    float y_offset, const Vec* bone_offset);
void set_active_projectile_p_handler(MkProcEntryFn handler);
void set_active_projectile_max_ticks(int ticks);
void active_projectile_setup_done(void);
float subzero_freeze_victim(void);
int is_weapon_style(PlyrFighterDefinition* fighter);
void advance_active_moveset(PlyrPdata* player);
void release_other_player(void);
float p_animate(void);
int is_my_chest_to_screen(void);
void head_tracking_off(void);
void two_player_animation_blend(
    AniData* animation, int attacker_mode, int victim_mode,
    MkObj* attacker, int victim_arg, float attacker_blend,
    float victim_blend, float victim_angle);
static float p_face_obj(void);
FatalityFakeBoneMatcher* ft_fake_bone_matcher(
    MkObj* parent, MkObj* child, int child_bone,
    const Vec* parent_offset, const Vec* child_offset,
    const Vec* rotation, int mode, float blend);
static float p_fake_bone_matcher_proc(void);
float p_obj_grnd_bounce(void);
float p_obj_pos_matcher(void);
float p_obj_scalar_proc(void);
float p_bodyslam_bodysplat(void);
static float p_raiden_lightning_scrolling(void);
float p_raiden_lightning_flash(void);
static float p_raiden_summon_lightning_bolt(void);
static float p_sd_sonic_waves(void);
static void sindel_load_projectile_obj_for_sonic_waves(
    FatalitySonicWavePdata* data);
void update_mkobj_pdata(MkObj* object, MkHdr* pdata);
int build_bones_tbl(MkObj* object, const int* tags);
void bone_matcher_parent_set_offset(
    FatalityBoneMatcher* matcher, Vec* offset);
FatalityBoneMatcher* start_bone_matcher(
    float blend_ticks, MkObj* parent, int parent_bone,
    MkObj* child, int child_bone);
void obj_set_bone_collapse_flag(MkObj* object, int bone_id);
void get_bone_world_pos(MkObj* object, int bone_id, Vec* position);
void get_bone_offset_world_pos(
    MkObj* object, int bone_id, const Vec* offset, Vec* position);
void obj_match_pos_ang_to_src_obj(MkObj* destination, MkObj* source);
void calc_bone_world_mat(MkObj* object, int bone_id);
void glitch_to_stance(AnimPdata* animation, float blend);
void face_opponent_now(void);
int is_local_plyr(void);
MkHdr* pdata_of_proc(MkProc* process);
void push_game_state(int state);
void plyr_turn_off_mirrorguy(PlyrInfo* player);
void plyr_turn_off_shadowbox(PlyrInfo* player);
void start_gore2_update(void);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void limb_sever_explode_apart(
    PlyrInfo* player, int mode, float strength);
void auto_calc_limbobj_bone_world_pos(MkObj* object, int bone_id);
void start_blood_particles(
    int script, int bone_id, PlyrPdata* player, MkObj* object);

static inline void fatality_spawn_limb_blood(
    FighterMirror* fighter, PlyrPdata* player, int limb_index,
    int script, int bone_id) {
    MkObj* limb;

    limb = fatality_get_severed_limb(fighter, limb_index);
    if (limb != 0) {
        auto_calc_limbobj_bone_world_pos(limb, bone_id);
        start_blood_particles(script, bone_id, player, limb);
    }
}

void mkobj_get_matrix_at(MkObj* object, Vec* out);
void mkobj_get_matrix_right(MkObj* object, Vec* out);
void obj_set_all_sobjs_priority(MkObj* object, int priority);
void plyr_weapon_grab(PlyrPdata* player, MkObj* weapon);
void plyr_weapon2_grab(PlyrPdata* player, MkObj* weapon);
float sqrtf(float value);
static float p_subzero_ice_chunk(void);
static float p_subzero_iceblock_alpha(void);
float p_sz2_iceblock_scalar(void);
float sz_kill_myself(void);
float subzero_rx_freeze(void);
void material_set_zbias(RpMaterial* material, float bias);

typedef int FatalityEffectHandle;
extern FatalityEffectHandle fx_by_owner(
    const char* name, unsigned int owner);
extern FatalityEffectHandle fx_next_emitter(
    FatalityEffectHandle effect);
extern void fx_resume_emit(FatalityEffectHandle handle);
extern int emitter_id_from_handle(FatalityEffectHandle handle);
extern MkPfx* find_pfx_by_name(const char* name);
extern void reset_effect(const char* name);
extern void resume_effect(const char* name);

#define FATALITY_SLEEP(ticks)                                                \
    do {                                                                     \
        _mkproc_sleep_ticks = (ticks);                                       \
        ((FatalityProcVtable*)aproc->vtbl)->sleep();                          \
    } while (0)

static inline void fatality_fade_and_cleanup(void) {
    PlyrPdata* player0;
    PlyrPdata* player1;
    float far_clip;
    float step;

    background_color.red = 0;
    background_color.green = 0;
    background_color.blue = 0;
    background_color.alpha = 0;
    g_game_info.sky = 0;
    fog_on = 1;
    fog_distance = 0.0f;
    fog_type = 1;
    fog_color_real[0] = 0.0f;
    fog_color_real[1] = 0.0f;
    fog_color_real[2] = 0.0f;
    fog_color_real[3] = 0.0f;

    far_clip = Camera->farPlane;
    step = (far_clip - 10.0f) / 30.0f;
    while (far_clip > 10.0f) {
        far_clip -= step;
        if (far_clip < 10.0f) {
            far_clip = 10.0f;
        }
        RwCameraSetFarClipPlane(Camera, far_clip);
        FATALITY_SLEEP(1.0f);
    }

    player0 = g_game_info.plyr0.slot.pdata;
    player1 = g_game_info.plyr1.slot.pdata;
    destroy_mkprocs_pid(0x3019);
    destroy_mkprocs_pid(0x501B);
    unimpale_victim(player0);
    unimpale_victim(player1);
    reload_fan(player0);
    reload_fan(player1);
    destroy_mkprocs_pid(0x2026);
    player0->impaled_projectile_state = 0;
    player1->impaled_projectile_state = 0;
    if (player0 != 0 && player0->status_data != 0 &&
        player0->status_data->reaction_cleanup != 0) {
        run_reaction_cleanup_function(player0);
    }
    if (player1 != 0 && player1->status_data != 0 &&
        player1->status_data->reaction_cleanup != 0) {
        run_reaction_cleanup_function(player1);
    }
    stop_tunes();
    if (g_game_info.field_200 == 3) {
        fatality_loop_sound = snd_req(0x1A9D);
    } else {
        fatality_loop_sound = snd_req(0x1A9B);
    }
    fatality_state.mirror_camera =
        is_a_to_the_right_of_b(
            fatality_state.attacker_object,
            fatality_state.victim_object) != 0;
    wait_for_slot_load(fatality_anim_slot);
}

static inline void fatality_load_animation_section(
    const char* section_name) {
    int file_count;

    fatality_anim_slot = 0x3000C;
    if (fatality_state.player == g_game_info.plyr0.slot.pdata) {
        fatality_anim_slot = 0x4000C;
    }
    file_count = get_slot_file_count(fatality_anim_slot);
    if (fatality_state.opponent->character_id != 0x1B) {
        unload_section_slot_file(fatality_anim_slot, file_count);
    }
    load_ssf(fatalityanims_file_table);
    add_anim_section_async(
        fatality_anim_slot, find_section_by_name(section_name),
        &fatality_state.player->fatality_palette, 1, 0);
}

static inline FatalityEffectHandle fatality_bind_next_emitter(
    FatalityEffectHandle effect_handle, MkObj* object,
    int bone_id, int bind_render) {
    FatalityEffectHandle emitter;
    MkPfx* effect;
    int emitter_id;

    emitter = fx_next_emitter(effect_handle);
    if (emitter == 0) {
        return 0;
    }
    fx_resume_emit(emitter);
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return 0;
    }
    emitter_id = emitter_id_from_handle(emitter);
    if (bind_render) {
        if ((unsigned int)(bone_id + 0xC0000000) == 0) {
            pfx_bind_render_to_obj(effect, object, 0);
        } else {
            pfx_bind_render_to_obj_bone(effect, object, bone_id);
        }
    } else if ((unsigned int)(bone_id + 0xC0000000) == 0) {
        pfx_bind_emitter_num_to_obj(effect, object, 0, emitter_id);
    } else {
        pfx_bind_emitter_num_to_obj_bone(
            effect, object, bone_id, emitter_id);
    }
    return emitter;
}

static inline void fatality_finish_sidekick(PlyrInfo* player_info) {
    PlyrPdata* player;
    MkProc* animation_proc;

    if (player_info->player_index != 0x1B) {
        return;
    }
    player = player_info->slot.pdata;
    animation_proc = player->sidekick_anim_proc;
    if (animation_proc != 0 &&
        animation_proc->hdr.instance != player->sidekick_anim_instance) {
        animation_proc = 0;
    }
    if (animation_proc != 0) {
        xfer_proc(animation_proc, p_anim_idle);
    }
    if (player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
}

static inline MkObj* fatality_resolve_object_latch(
    MkObj* object, unsigned int instance) {
    if (object != 0) {
        if (object->hdr.instance != instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

void subzero_start_ice_chunks(PlyrPdata* player) {
    FatalityIceChunkPdata* data;
    MkHdr* process;
    MkObj* model;
    FatalityIceSubobject* subobject;
    FatalityIceChunkPebble* chunk;
    RwRGBA color;
    int index;

    data = 0;
    model = 0;
    process = (MkHdr*)_create_mkproc_generic_nostack(
        0x6037, 0x1F, p_subzero_ice_chunk,
        sizeof(FatalityIceChunkPdata), (MkHdr**)&data);
    if (process != 0) {
        zero_pdata_payload(sizeof(FatalityIceChunkPdata), &data->hdr);
        data->owner = fatality_state.attacker_object;
        data->owner_instance =
            fatality_state.attacker_object->hdr.instance;
        model = load_named_model_for_player(
            "ICESUICIDE", player->plyr_num, 0x6014, 1);
        if (model != 0) {
            obj_create_sobjs(model);
            data->model = model;
            data->model_instance = model->hdr.instance;
            insert_fgnd_mkobj(model);
            ((FatalityObjectGroupView*)model)->group =
                ((FatalityObjectGroupView*)plyr_obj)->group;
            mk_insert(&model->hdr, &plyr_obj->child_list);
            color.red = 0xFF;
            color.green = 0xFF;
            color.blue = 0xFF;
            color.alpha = 0xA0;
            obj_set_color_for_material_by_id(model, 0, &color);

            for (index = 0; index < 9; index++) {
                subobject = (FatalityIceSubobject*)
                    obj_find_sobj_by_id(
                        model, sz_hk_icechunk_map[index].subobject_id);
                if (subobject != 0) {
                    chunk = (FatalityIceChunkPebble*)create_pebble_userdata(
                        (MkSobj*)subobject,
                        sz_hk_icechunk_map[index].pebble_id, 0);
                    if (chunk != 0) {
                        subobject->flags08 =
                            (subobject->flags08 | 0x48) & ~1;
                        subobject->flags09 |= 0x9A;
                        subobject->field28 = -49.0f;
                        sobj_set_priority((MkSobj*)subobject, 0x12);
                        subobject->scale.x = 0.0f;
                        subobject->scale.y = 0.0f;
                        subobject->scale.z = 0.0f;
                        data->chunks[index] = chunk;
                    }
                }
            }
            return;
        }
    }

    if (model != 0 && model->hdr.instance != 0) {
        ((int (*)(MkHdr*))model->hdr.vtbl->destroy)(&model->hdr);
    }
    if (process != 0 && process->instance != 0) {
        ((int (*)(MkHdr*))process->vtbl->destroy)(process);
    }
}

/*
 * Soft ceiling: retail's nine chunk/count checks and all fifteen corrected
 * severed-limb transforms are preserved. The remaining frame/register delta
 * comes from retail's 16-byte-aligned stack matrix; this portable declaration
 * intentionally avoids a function-local alignment attribute used only to
 * force that compiler emission.
 */
static float p_subzero_ice_chunk(void) {
    FatalityIceChunkPdata* data;
    FatalityIceChunkPebble* chunk;
    FighterMirror* fighter;
    MkObj* limb;
    RwMatrix* chunk_matrix;
    MKMATRIX rotation;
    Vec offset;
    int bone_index;
    int chunk_index;
    int matrix_index;
    int fix_index;

    data = (FatalityIceChunkPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    if (data->owner != 0 &&
        data->owner->hdr.instance == data->owner_instance) {
        fix_index = 0;
        for (chunk_index = 0; chunk_index < 9; chunk_index++) {
            chunk = data->chunks[chunk_index];
            if (chunk->matrix_count !=
                sz_hk_icechunk_map[chunk_index].pebble_id) {
                break;
            }
            for (matrix_index = 0;
                 matrix_index < chunk->matrix_count;
                 matrix_index++) {
                bone_index = sz_hk_icechunk_map_array[fix_index];
                if (data->owner == g_game_info.plyr0.slot.mirror_a) {
                    fighter = g_game_info.plyr0.slot.fighter;
                } else {
                    fighter = g_game_info.plyr1.slot.fighter;
                }
                limb = fatality_get_severed_limb(
                    fighter, bone_index);
                if (limb == 0) {
                    break;
                }
                chunk_matrix = &chunk->matrices[matrix_index];
                YXZ_angles_to_MKMATRIX(
                    &sz_ice_chunk_ang_fix[bone_index], &rotation);
                mat_x_mat(
                    chunk_matrix, &rotation, limb->field_24);
                offset = sz_ice_chunk_pos_fix[bone_index];
                if (fatality_state.mirror_camera != 0) {
                    offset.x = -offset.x;
                    offset.z = -offset.z;
                }
                v3_x_mat_add_v3(
                    &chunk_matrix->pos_vec, &offset,
                    limb->field_24, &limb->field_24->pos_vec);
                fix_index++;
            }
            if (matrix_index != chunk->matrix_count) {
                break;
            }
        }
        if (chunk_index == 9) {
            return 1.0f;
        }
    }

    if (data->model != 0 &&
        data->model->hdr.instance == data->model_instance &&
        data->model_instance != 0) {
        ((int (*)(MkHdr*))data->model->hdr.vtbl->destroy)(
            &data->model->hdr);
    }
    for (chunk_index = 0; chunk_index < 9; chunk_index++) {
        chunk = data->chunks[chunk_index];
        if (chunk != 0 && chunk->hdr.instance != 0) {
            ((int (*)(MkHdr*))chunk->hdr.vtbl->destroy)(&chunk->hdr);
        }
    }
    return -1.0f;
}

/*
 * Soft ceiling: model/process ownership, mirrored placement, material list,
 * color ramp initialization, and failure cleanup follow the retail routine.
 */
MkObj* subzero_start_iceman(void) {
    static const int material_ids[9] = {
        0xA, 0x32, 0x14, 0x82, 0x79, 0x64, 0x5B, 0x8C, -1
    };
    static const int alternate_material_ids[9] = {
        0xA, 0x32, 0x14, 0x82, 0x78, 0x64, 0x5A, 0x8D, -1
    };
    FatalityIceblockAlphaPdata* alpha_data;
    const int* ids;
    RpMaterial* material;
    MkObj* iceman;
    Vec right;
    int index;

    iceman = load_named_model_for_player(
        "ICEMAN", fatality_state.player->plyr_num,
        fatality_state.player->plyr_num, 1);
    if (iceman == 0) {
        return 0;
    }
    if (_create_mkproc_generic_nostack(
            0x6036, 0x1F, p_subzero_iceblock_alpha,
            sizeof(FatalityIceblockAlphaPdata),
            (MkHdr**)&alpha_data) == 0) {
        if (iceman->hdr.instance != 0) {
            iceman->hdr.typed_vtbl->destroy(&iceman->hdr);
        }
        return 0;
    }
    zero_pdata_payload(
        sizeof(FatalityIceblockAlphaPdata), &alpha_data->hdr);
    alpha_data->object = iceman;
    alpha_data->object_instance = iceman->hdr.instance;
    alpha_data->color.red = 0xFF;
    alpha_data->color.green = 0xFF;
    alpha_data->color.blue = 0xFF;
    alpha_data->color.alpha = 0;
    insert_fgnd_mkobj(iceman);
    mk_insert(
        &iceman->hdr, &fatality_state.attacker_object->child_list);
    obj_set_color_for_all_materials(
        iceman, &alpha_data->color);
    obj_set_all_sobjs_priority(iceman, 0x13);
    iceman->light_flags = fatality_state.attacker_object->light_flags;
    mkobj_get_matrix_right(fatality_state.attacker_object, &right);
    iceman->pos.value.y = g_game_info.field_34 - 0.075f;
    iceman->ang.y = fatality_state.attacker_object->ang.y;
    if (fatality_state.mirror_camera != 0) {
        iceman->pos.value.x = fatality_state.attacker_object->pos.value.x +
            0.05f * right.x;
        iceman->pos.value.z = fatality_state.attacker_object->pos.value.z +
            0.05f * right.z;
        iceman->ang.y -= 1.5707964f;
    } else {
        iceman->pos.value.x = fatality_state.attacker_object->pos.value.x -
            0.05f * right.x;
        iceman->pos.value.z = fatality_state.attacker_object->pos.value.z -
            0.05f * right.z;
        iceman->ang.y += 1.5707964f;
    }

    ids = fatality_state.context->flags_14_bits.alternate_costume
              ? alternate_material_ids : material_ids;
    for (index = 0; ids[index] >= 0; index++) {
        material = obj_find_material_by_id(
            fatality_state.attacker_object, ids[index]);
        if (material != 0) {
            material_set_zbias(material, 0.25f);
        }
    }
    return iceman;
}

/*
 * Soft ceiling: retail loop bounds, alpha progression, material mapping, and
 * object-instance validation are recovered; remaining differences are emit.
 */
static float p_subzero_iceblock_alpha(void) {
    FatalityIceblockAlphaPdata* data;
    MkObj* object;
    int index;

    data = (FatalityIceblockAlphaPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 && object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    for (index = 0; index < 6; index++) {
        if (index <= data->active_material) {
            if (data->material_alpha[index] < 0xA0) {
                data->material_alpha[index] += 10;
                data->color.alpha = data->material_alpha[index];
                if (index != 0) {
                    obj_set_color_for_material_by_id(
                        object, index + 1, &data->color);
                } else {
                    obj_set_color_for_material_by_id(
                        object, index, &data->color);
                }
            } else if (index >= data->active_material) {
                data->active_material++;
            }
        }
    }
    if (data->active_material >= 6) {
        return -1.0f;
    }
    return 1.0f;
}

/*
 * Soft ceiling: the victim-relative placement, mirrored lateral offset,
 * character-specific rise, scale process, color, and priority match retail.
 */
MkObj* subzero_start_iceblock(void) {
    FatalityScalePdata* scale_data;
    RwRGBA color;
    MkObj* iceblock;
    Vec at;
    Vec right;
    float forward;
    float maximum;
    float step;

    iceblock = load_named_model_for_player(
        "ICEBLOCK", fatality_state.player->plyr_num,
        fatality_state.player->plyr_num, 1);
    if (iceblock != 0) {
        insert_fgnd_mkobj(iceblock);
        mk_insert(
            &iceblock->hdr,
            &fatality_state.attacker_object->child_list);
        color.red = 0xFF;
        color.green = 0xFF;
        color.blue = 0xFF;
        color.alpha = 0xB4;
        obj_set_color_for_all_materials(iceblock, &color);
        obj_set_all_sobjs_priority(iceblock, 0x13);
        iceblock->light_flags = fatality_state.victim_object->light_flags;
        iceblock->pos.value = fatality_state.victim_object->pos.value;
        mkobj_get_matrix_at(fatality_state.victim_object, &at);
        mkobj_get_matrix_right(fatality_state.victim_object, &right);
        if (fatality_state.mirror_camera != 0) {
            right.x *= 0.1f;
            right.y *= 0.1f;
            right.z *= 0.1f;
        } else {
            right.x *= -0.1f;
            right.y *= -0.1f;
            right.z *= -0.1f;
        }
        iceblock->pos.value.y = g_game_info.field_34;
        if (fatality_state.opponent->character_id == 0xA) {
            forward = 0.25f;
            maximum = 1.0f;
            step = 0.015f;
        } else {
            forward = 0.3f;
            maximum = 0.8f;
            step = 0.01f;
        }
        iceblock->pos.value.x -= at.x * forward;
        iceblock->pos.value.z -= at.z * forward;
        iceblock->pos.value.x += right.x;
        iceblock->pos.value.z += right.z;
        iceblock->flags_08 |= 0x02;
        iceblock->scale.x = 1.0f;
        iceblock->scale.y = 0.05f;
        iceblock->scale.z = 1.0f;
        if (_create_mkproc_generic_nostack(
                0x6035, 0x1F, p_sz2_iceblock_scalar,
                sizeof(FatalityScalePdata),
                (MkHdr**)&scale_data) != 0) {
            scale_data->object = iceblock;
            scale_data->object_instance = iceblock->hdr.instance;
            scale_data->maximum = maximum;
            scale_data->step = step;
        }
    }
    return iceblock;
}

float p_sz2_iceblock_scalar(void) {
    FatalityScalePdata* data;
    MkObj* object;

    data = (FatalityScalePdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 &&
        object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }
    object->scale.y += data->step;
    if (object->scale.y > data->maximum) {
        return -1.0f;
    }
    return 1.0f;
}

void mks_start_fatality_iceball(int mode) {
    switch (mode) {
    case 0:
        start_3d_projectile_iceball(sz_kill_myself);
        break;
    case 1:
        start_3d_projectile_iceball(subzero_freeze_victim);
        break;
    }
}

/* Retail uses compact nonvolatile saves for this projectile setup. */
static void start_3d_projectile_iceball(MkProcEntryFn entry) {
    Vec bone_offset = {0.0f, 0.0f, 0.0f};
    MkObj* iceball;
    MkSobj* subobject;
    int bone_id;

    bone_id = get_bid_with_flip(plyr_obj, 0x18);
    iceball = start_projectile_from_plyr_bone(
        bone_id, 0, "ICEBALL", 0.14f, 10.0f, &bone_offset);
    if (iceball == 0) {
        return;
    }

    set_active_projectile_p_handler(entry);
    set_active_projectile_max_ticks(10);
    active_projectile_setup_done();

    iceball->flags_08_bits.scale_active = 1;
    iceball->scale.x = 1.3f;
    iceball->scale.y = 1.3f;
    iceball->scale.z = 1.3f;
    iceball->ang.y += 1.57f;
    iceball->flags_08_bits.angular_velocity_enabled = 1;

    subobject = (MkSobj*)obj_create_sobjs_by_id(iceball, 1);
    if (subobject != 0) {
        subobject->flags_08_bits.bit3 = 1;
        subobject->flags_08_bits.angular_velocity_enabled = 1;
        subobject->ang_vel.x = 0.4f;
        subobject->z_offset = -50.0f;
    }

    subobject = (MkSobj*)obj_create_sobjs_by_id(iceball, 2);
    if (subobject != 0) {
        subobject->flags_08_bits.bit4 = 1;
        subobject->flags09_bits.bit7 = 1;
        subobject->flags_08_bits.bit0 = 0;
    }

    subobject = (MkSobj*)obj_create_sobjs_by_id(iceball, 3);
    if (subobject != 0) {
        subobject->flags09_bits.bit5 = 1;
        subobject->flags09_bits.bit7 = 1;
        subobject->z_offset = -50.0f;
    }
}


float subzero_freeze_victim(void) {
    FatalityProjectilePdata* data;

    data = (FatalityProjectilePdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    xfer_proc(fatality_state.victim_proc, subzero_rx_freeze);
    data->max_ticks = 13.0f;
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(p_3d_distance_handler, 0.0f);
    return 0.0f;
}

float sz_kill_myself(void) {
    FatalityDistancePdataRef data;
    FatalityProcVtableRef vtable;

    data.hdr = apdata;
    if (data.hdr == 0) {
        return -1.0f;
    }

    data.projectile->max_ticks = 13.0f;
    vtable.base = aproc->vtbl;
    vtable.fatality->jump_sleep(p_3d_distance_handler, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: exact retail size and branches; the residue is register
 * allocation/scheduling around the object-instance validation paths.
 */
static float p_3d_distance_handler(void) {
    FatalityProjectilePdata* data;
    MkObj* object;

    data = (FatalityProjectilePdata*)apdata;
    if (data != 0) {
        object = data->object;
        if (object != 0 && object->hdr.instance != data->object_instance) {
            object = 0;
        }
        if (object != 0) {
            data->max_ticks -= 1.0f;
            if (simple_3d_projectile_collision(
                    &data->retarget_source->retarget_object->pos.value,
                    &data->retarget_object->pos.value, &object->pos.value, 0, 0.2f,
                    225.0f, 0.0f) != 2 &&
                data->max_ticks >= 0.0f) {
                return 1.0f;
            }
        }
    }

    object = data->object;
    if (object != 0 && object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object != 0) {
        if (data->object->hdr.instance != 0) {
            data->object->hdr.typed_vtbl->destroy(&data->object->hdr);
        }
        data->object = 0;
        data->object_instance = 0;
    }
    return -1.0f;
}

float subzero_his_tinkle_snd(void) {
    FatalityProcVtableRef vtable;

    _mkproc_sleep_ticks = 60.0f;
    vtable.base = aproc->vtbl;
    vtable.fatality->sleep();
    snd_req(0x353);
    vtable.fatality->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

float subzero_rx_freeze(void) {
    FatalityProcVtableRef vtable;

    freeze_player();
    snd_req(0x348);
    vtable.base = aproc->vtbl;
    vtable.fatality->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

void sindel_sonic_sounds(FatalityObjectLatch* sound, int finished) {
    MkHdr* object;

    object = sound->object;
    if (object != 0) {
        if (object->instance == sound->object_instance) {
            /* The instance latch still identifies this object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0 && finished == 0) {
        sound->active = 1;
    }
}

void sindel_scream_react_sound_start(void) {
    if (plyr_pdata != 0 &&
        plyr_pdata->his_plyr_pdata->character_id == 8) {
        plyr_pdata->scream_sound_handle = plyr_snd_req(0x44);
    }
}

/*
 * Soft ceiling: all five wave objects, projectile userdata, transform seeds,
 * sounds, player mask, and complete failure cleanup follow retail.
 */
FatalitySonicWavePdata* sindel_sonic_waves(float duration) {
    static const Vec emitter_offset = {0.0f, 0.075f, 0.23f};
    FatalitySonicWavePdata* data;
    FatalitySonicPebble* pebble;
    MkObj* wave;
    MkObj* object;
    float angle;
    int index;

    data = 0;
    angle = 0.0f;
    if (_create_mkproc_generic_nostack(
            0x6030, 0x1F, p_sd_sonic_waves,
            sizeof(FatalitySonicWavePdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(sizeof(FatalitySonicWavePdata), &data->hdr);
        sindel_load_projectile_obj_for_sonic_waves(data);
        if (data->pebble != 0) {
            for (index = 0; index < 5; index++) {
                wave = load_named_model_for_player(
                    "SN_CURL", plyr_pdata->plyr_num, 0x6013, 1);
                if (wave == 0) {
                    break;
                }
                insert_fgnd_mkobj(wave);
                data->waves[index].obj = wave;
                data->waves[index].instance = wave->hdr.instance;
                wave->flags_word_08 = 0;
                wave->flags_08 |= 0x40;
                wave->flags_08 |= 0x08;
                wave->flags_08 |= 0x02;
                wave->pos.value.x = 0.0f;
                wave->pos.value.y = -10000.0f;
                wave->pos.value.z = 0.0f;
                wave->ang.x = 0.0f;
                wave->ang.y = plyr_obj->ang.y;
                wave->ang_vel.x = 0.0f;
                wave->ang_vel.y = 0.0f;
                if (f_fatality_was_done != 0) {
                    wave->flags_08 |= 0x04;
                    wave->ang.z = angle;
                    angle += 3.1415927f;
                    wave->ang_vel.z = 0.9424778f;
                } else {
                    wave->ang.z =
                        am_i_on_the_left() != 0 ? 0.0f : 3.1415927f;
                    wave->ang_vel.z = 0.62831855f;
                }
            }
            if (index == 5) {
                data->owner = plyr_obj;
                data->owner_instance = plyr_obj->hdr.instance;
                calc_bone_world_mat(plyr_obj, 0x10);
                v3_x_mat(
                    &data->origin, &emitter_offset,
                    plyr_obj->bones[0]->parent_matrix);
                data->velocity.x = 0.05f * plyr_obj->field_24->at.x;
                data->velocity.y = 0.05f * plyr_obj->field_24->at.y;
                data->velocity.z = 0.05f * plyr_obj->field_24->at.z;
                data->velocity.y = 0.005f;
                pebble = (FatalitySonicPebble*)data->pebble;
                for (index = 0; index < 5; index++) {
                    data->wave_progress[index] = 0.0f;
                    data->wave_offsets[index].x = 0.0f;
                    data->wave_offsets[index].y = 0.0f;
                    data->wave_offsets[index].z = 0.0f;
                    pebble->matrices[index].pos.x = 0.0f;
                    pebble->matrices[index].pos.y = -10000.0f;
                    pebble->matrices[index].pos.z = 0.0f;
                }
                data->frame = 0;
                data->field_F0 = 0.0f;
                data->duration = duration;
                data->player_mask =
                    1 << plyr_pdata->plyr_info->controller_slot;
                if (f_fatality_was_done != 0) {
                    data->sound = snd_req(0x327);
                    data->loop_sound = snd_req(0x328);
                } else {
                    data->sound = snd_req(0x31B);
                    data->loop_sound = 0;
                }
                return data;
            }
        }
    }

    if (data != 0) {
        for (index = 0; index < 5; index++) {
            object = data->waves[index].obj;
            if (object != 0 &&
                object->hdr.instance != data->waves[index].instance) {
                object = 0;
            }
            if (object != 0 && object->hdr.instance != 0) {
                object->hdr.typed_vtbl->destroy(&object->hdr);
            }
        }
        object = data->projectile;
        if (object != 0 &&
            object->hdr.instance != data->projectile_instance) {
            object = 0;
        }
        if (object != 0 && object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        if (data->pebble != 0 && data->pebble->instance != 0) {
            data->pebble->typed_vtbl->destroy(data->pebble);
        }
        data->pebble = 0;
        if (data->hdr.instance != 0) {
            data->hdr.typed_vtbl->destroy(&data->hdr);
        }
    }
    return 0;
}

/*
 * Soft ceiling: exact retail size with the projectile model, pebble userdata,
 * parent link, render flags, and failure cleanup; residue is register emission.
 */
static void sindel_load_projectile_obj_for_sonic_waves(
    FatalitySonicWavePdata* data) {
    MkObj* projectile;
    MkSobj* subobject;

    projectile = load_named_model_for_player(
        "SN_SPARKS", plyr_pdata->plyr_num, 0x6012, 1);
    if (projectile != 0) {
        obj_create_sobjs(projectile);
        subobject = (MkSobj*)obj_first_sobj(projectile);
        if (subobject != 0) {
            data->pebble = (MkHdr*)create_pebble_userdata(subobject, 5, 0);
            if (data->pebble != 0) {
                insert_fgnd_mkobj(projectile);
                data->projectile = projectile;
                data->projectile_instance = projectile->hdr.instance;
                projectile->light_flags = plyr_obj->light_flags;
                mk_insert(&projectile->hdr, &plyr_obj->child_list);
                subobject->flags_08 = 0;
                subobject->flags_08 |= 0x40;
                subobject->flags_08 &= ~0x01;
                subobject->flags09 |= 0x80;
                subobject->flags09 |= 0x10;
                subobject->flags09 |= 0x08;
                subobject->flags09 |= 0x02;
                subobject->z_offset = 0.0f;
                sobj_set_priority(subobject, 0x12);
                return;
            }
        }
    }
    if (projectile != 0 && projectile->hdr.instance != 0) {
        projectile->hdr.typed_vtbl->destroy(&projectile->hdr);
    }
}

/*
 * Soft ceiling: the five-slot launch cadence, both gameplay/cinematic growth
 * modes, alpha fade, render matrices, expiry, cleanup, and sound teardown are
 * fully recovered; remaining differences are MWCC allocation and scheduling.
 */
static float p_sd_sonic_waves(void) {
    FatalitySonicWavePdata* data;
    FatalitySonicPebble* pebble;
    MkObj* projectile;
    MkObj* owner;
    MkObj* wave;
    Vec owner_bone;
    Vec matrix_scale;
    float fade;
    float growth;
    float distance;
    float scale_step;
    int alive;
    int started;
    int index;

    alive = 0;
    started = 0;
    data = (FatalitySonicWavePdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    projectile = data->projectile;
    if (projectile != 0 &&
        projectile->hdr.instance != data->projectile_instance) {
        projectile = 0;
    }
    if (projectile != 0 && data->finished == 0) {
        owner = data->owner;
        if (owner != 0 && owner->hdr.instance != data->owner_instance) {
            owner = 0;
        }
        if (owner == 0) {
            owner_bone.x = 0.0f;
            owner_bone.y = 0.0f;
            owner_bone.z = 0.0f;
        } else {
            get_bone_world_pos(owner, 0xD, &owner_bone);
        }
        pebble = (FatalitySonicPebble*)data->pebble;

        for (index = 0; index < 5; index++) {
            wave = data->waves[index].obj;
            if (wave != 0 &&
                wave->hdr.instance != data->waves[index].instance) {
                wave = 0;
            }
            if (wave == 0) {
                continue;
            }
            if (data->wave_progress[index] <= 0.0f) {
                if (data->frame != index) {
                    continue;
                }
                if (data->field_F0 > 0.0f) {
                    data->field_F0 -= game_speed;
                    continue;
                }
                if (data->duration <= 0.0f) {
                    continue;
                }
                started = 1;
                data->wave_offsets[index].x = 0.01f;
                data->wave_offsets[index].y = 0.01f;
                data->wave_offsets[index].z = 0.01f;
                data->wave_positions[index] = owner_bone;
                data->field_F0 = SD_SONIC_WAVES_TBS;
                if (f_fatality_was_done != 0) {
                    data->field_F0 *= 0.5f;
                }
            }

            data->wave_progress[index] += game_speed;
            if (data->wave_progress[index] >= 20.0f) {
                data->wave_progress[index] = 0.0f;
                pebble->matrices[index].pos.y = -10000.0f;
                wave->pos.value.y = -10000.0f;
                continue;
            }

            if (f_fatality_was_done != 0) {
                scale_step = 0.15f * game_speed;
                data->wave_offsets[index].x += scale_step;
                data->wave_offsets[index].y += scale_step;
                data->wave_offsets[index].z += scale_step;
                distance = game_speed *
                    (data->wave_offsets[index].x *
                     SD_SONIC_WAVE_DIST_SCALE);
                data->wave_positions[index].x +=
                    data->velocity.x * distance;
                data->wave_positions[index].y +=
                    data->velocity.y * distance;
                data->wave_positions[index].z +=
                    data->velocity.z * distance;
                wave->pos.value.x = data->origin.x +
                    data->wave_positions[index].x;
                wave->pos.value.y = data->origin.y +
                    data->wave_positions[index].y;
                wave->pos.value.z = data->origin.z +
                    data->wave_positions[index].z;
                data->wave_offsets[index].z += 3.0f * scale_step;
            } else {
                if (game_speed < 1.0f) {
                    growth = 1.0f +
                        SD_SONIC_WAVE_SCALE_GROWTH * game_speed;
                } else {
                    growth = SD_SONIC_WAVE_SCALE_GROWTH * game_speed;
                }
                data->wave_offsets[index].x *= growth;
                data->wave_offsets[index].y *= growth;
                data->wave_offsets[index].z *= growth;
                distance = game_speed *
                    (data->wave_offsets[index].z *
                     SD_SONIC_WAVE_DIST_SCALE);
                data->wave_positions[index].x +=
                    data->velocity.x * distance;
                data->wave_positions[index].y +=
                    data->velocity.y * distance;
                data->wave_positions[index].z +=
                    data->velocity.z * distance;
                wave->pos.value.x = data->origin.x +
                    data->wave_positions[index].x;
                wave->pos.value.y = data->origin.y +
                    data->wave_positions[index].y;
                wave->pos.value.z = data->origin.z +
                    data->wave_positions[index].z;
            }
            wave->scale = data->wave_offsets[index];

            fade = 1.0f - data->wave_progress[index] / 20.0f;
            obj_for_all_atomics_set_material_alpha(
                wave, (int)(185.0f * fade + 70.0f));
            MKMatrixSetIdentity(&pebble->matrices[index]);
            y_angle_to_MKMATRIX(&pebble->matrices[index], owner->ang.y);
            if (data->wave_offsets[index].x < 1.0f) {
                matrix_scale.x =
                    scale_proj.x * data->wave_offsets[index].x;
                matrix_scale.y =
                    scale_proj.y * data->wave_offsets[index].x;
                matrix_scale.z =
                    scale_proj.z * data->wave_offsets[index].x;
            } else {
                matrix_scale = scale_proj;
            }
            mat_scaled_by_v3(
                &pebble->matrices[index], &pebble->matrices[index],
                &matrix_scale);
            pebble->matrices[index].pos.x = wave->pos.value.x;
            pebble->matrices[index].pos.y = wave->pos.value.y;
            pebble->matrices[index].pos.z = wave->pos.value.z;
            if ((index & 1) != 0) {
                pebble->matrices[index].pos.y += 0.1f * index;
            } else {
                pebble->matrices[index].pos.y -= 0.1f * index;
            }
            alive++;
        }
        if (started != 0) {
            data->frame++;
            if (data->frame >= 5) {
                data->frame = 0;
            }
        }
        if (alive != 0) {
            data->duration -= game_speed;
            return 1.0f;
        }
    }

    if (projectile != 0 && projectile->hdr.instance != 0) {
        projectile->hdr.typed_vtbl->destroy(&projectile->hdr);
    }
    if (data->pebble != 0 && data->pebble->instance != 0) {
        data->pebble->typed_vtbl->destroy(data->pebble);
    }
    data->pebble = 0;
    for (index = 0; index < 5; index++) {
        wave = data->waves[index].obj;
        if (wave != 0 &&
            wave->hdr.instance != data->waves[index].instance) {
            wave = 0;
        }
        if (wave != 0 && wave->hdr.instance != 0) {
            wave->hdr.typed_vtbl->destroy(&wave->hdr);
        }
    }
    if (data->finished != 0) {
        if (data->sound != 0) {
            snd_stop(data->sound);
        }
        if (data->loop_sound != 0) {
            snd_stop(data->loop_sound);
        }
    }
    return -1.0f;
}

void start_raiden_lightning_scroll(
    MkObj* object, int flash, int scaling_index,
    float u_step, float scroll_step) {
    FatalityLightningScrollPdata* scroll_data;
    FatalityLightningFlashPdata* flash_data;
    FatalityUvScrollControl* control;

    scroll_data = 0;
    if (_create_mkproc_generic_nostack(
            0x602C, 0x1F, p_raiden_lightning_scrolling,
            sizeof(FatalityLightningScrollPdata),
            (MkHdr**)&scroll_data) == 0) {
        return;
    }
    zero_pdata_payload(
        sizeof(FatalityLightningScrollPdata), &scroll_data->hdr);
    scroll_data->object = object;
    scroll_data->object_instance = object->hdr.instance;
    scroll_data->scalar_start.y = u_step;
    scroll_data->scalar_start.z = 1.0f;
    scroll_data->scalar_target.y = u_step;
    scroll_data->scalar_target.z = 1.0f;
    scroll_data->scaling = &raiden_scaling_data[scaling_index];
    control = (FatalityUvScrollControl*)
        find_uv_scroll_control_for_obj(object);
    if (control != 0) {
        control->step = scroll_step;
    }
    if (!flash) {
        return;
    }
    flash_data = 0;
    if (_create_mkproc_generic_nostack(
            0x602C, 0x1F, p_raiden_lightning_flash,
            sizeof(FatalityLightningFlashPdata),
            (MkHdr**)&flash_data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityLightningFlashPdata),
            &flash_data->hdr);
        flash_data->object = object;
        flash_data->object_instance = object->hdr.instance;
        flash_data->scroll = control;
        flash_data->scroll_step = scroll_step;
        flash_data->flash_range =
            scaling_index == 2 ? 20.0f : 5.0f;
    }
}

float p_raiden_lightning_flash(void) {
    FatalityLightningFlashPdata* data;
    MkObj* object;
    float range;

    data = (FatalityLightningFlashPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 &&
        object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }
    data->frame++;
    range = data->flash_range;
    if ((data->frame & 1) != 0) {
        range += 5.0f;
        object->hide_flags |= 0x20;
    } else {
        object->hide_flags &= ~0x20;
        if (data->scroll != 0) {
            data->scroll->step =
                randu0(3) == 0
                    ? -data->scroll_step : data->scroll_step;
        }
    }
    return frand(range);
}

/*
 * Soft ceiling: exact scale-key lookup, scalar-process setup, object update,
 * and frame return semantics; residue is register/load scheduling only.
 */
static float p_raiden_lightning_scrolling(void) {
    FatalityLightningScrollPdata* data;
    FatalityObjectScalarPdata* scalar;
    MkObj* object;

    data = (FatalityLightningScrollPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 && object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    data->scalar_start.x = data->scaling->x[data->frame];
    data->scalar_target.x = data->scaling->x[data->frame + 1];
    data->scalar_step.x = data->scaling->y[data->frame];
    if (_create_mkproc_generic_nostack(
            0x600A, 0x1F, p_obj_scalar_proc,
            sizeof(FatalityObjectScalarPdata),
            (MkHdr**)&scalar) != 0) {
        zero_pdata_payload(
            sizeof(FatalityObjectScalarPdata), &scalar->hdr);
        scalar->object = object;
        scalar->object_instance = object->hdr.instance;
        object->flags_08 |= 0x02;
        scalar->start.x = data->scalar_start.x;
        scalar->start.y = data->scalar_start.y;
        scalar->start.z = data->scalar_start.z;
        scalar->target.x = data->scalar_target.x;
        scalar->target.y = data->scalar_target.y;
        scalar->target.z = data->scalar_target.z;
        scalar->step.x = data->scalar_step.x;
        scalar->step.y = data->scalar_step.y;
        scalar->step.z = data->scalar_step.z;
        object->scale.x = data->scalar_start.x;
        object->scale.y = data->scalar_start.y;
        object->scale.z = data->scalar_start.z;
        update_mkobj(object);
    }
    data->frame++;
    return data->scaling->z[data->frame - 1];
}

/*
 * Soft ceiling: allocation, slot selection, ownership latch, render setup,
 * and cleanup paths follow retail; remaining differences are code emission.
 */
RaidenLightningBoltPdata* ft_raiden_summon_lightning_bolt(
    PlyrPdata* player, int bone_id, const char* model_name) {
    RaidenLightningBoltPdata* data;
    MkProc* process;
    MkObj* model;
    MkObj* parent;
    MkSobj* subobject;
    int slot;

    data = 0;
    process = _create_mkproc_generic_nostack(
        0x602B, 0x1F, p_raiden_summon_lightning_bolt,
        sizeof(RaidenLightningBoltPdata), (MkHdr**)&data);
    if (process == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(RaidenLightningBoltPdata), &data->hdr);
    slot = player->plyr_num == 0 ? 0x3000B : 0x4000B;
    model = load_named_model_from_slot(slot, model_name, 0x2099, 0);
    if (model != 0) {
        subobject = (MkSobj*)obj_create_sobjs_by_id(model, 1);
        if (subobject != 0) {
            parent = player->tracked_obj;
            if (parent != 0 &&
                parent->hdr.instance != player->tracked_obj_instance) {
                parent = 0;
            }
            if (parent != 0) {
                data->bolt = model;
                data->bolt_instance = model->hdr.instance;
                mk_insert(&model->hdr, &parent->child_list);
                insert_fgnd_mkobj(model);
                sobj_set_priority(subobject, 0x12);
                subobject->z_offset = -5.0f;
                model->hide_flags |= 0x20;
                model->flags_08 |= 0x40;
                model->flags_08 |= 0x08;
                model->flags_08 |= 0x02;
                model->scale.x = 0.25f;
                model->scale.y = 0.25f;
                model->scale.z = 0.25f;
                data->owner = player;
                data->bone_id = bone_id;
                return data;
            }
        }
    }

    if (process->instance != 0) {
        process->vtbl->destroy(process);
    }
    if (model != 0 && model->hdr.instance != 0) {
        model->hdr.typed_vtbl->destroy(&model->hdr);
    }
    return 0;
}

void kill_raiden_summon_lightning_bolt(
    RaidenLightningBoltPdata* lightning) {
    lightning->bone_id = -1;
}

/*
 * Soft ceiling: the retail flash/fade state machine, owner latch, bone
 * placement, four orientations, and sleep result are fully represented.
 */
static float p_raiden_summon_lightning_bolt(void) {
    static float y_offset = 15.3f;
    RaidenLightningBoltPdata* data;
    MkObj* bolt;
    MkObj* owner_object;
    CamVec3 camera_angle;
    unsigned short random_ticks;
    int result;

    data = (RaidenLightningBoltPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    bolt = data->bolt;
    if (bolt != 0 && bolt->hdr.instance != data->bolt_instance) {
        bolt = 0;
    }
    if (bolt == 0) {
        return -1.0f;
    }

    if (data->fade_step != 0) {
        obj_set_sobj_alpha(bolt, 1, data->alpha);
        if (data->alpha > 0x50) {
            data->alpha -= data->fade_step;
            return 1.0f;
        }
        data->alpha = 0;
        data->fade_step = 0;
        obj_set_sobj_alpha(bolt, 1, 0xFF);
    }

    random_ticks = randu0(3);
    if ((data->frame & 1) != 0) {
        result = random_ticks + 2;
        bolt->hide_flags |= 0x20;
    } else {
        bolt->hide_flags &= ~0x20;
        owner_object = data->owner->tracked_obj;
        if (owner_object != 0 &&
            owner_object->hdr.instance !=
                data->owner->tracked_obj_instance) {
            owner_object = 0;
        }
        if (owner_object == 0) {
            data->bone_id = -1;
        }
        if (data->bone_id < 0) {
            if (bolt->hdr.instance != 0) {
                bolt->hdr.typed_vtbl->destroy(&bolt->hdr);
            }
            return -1.0f;
        }

        get_bone_world_pos(bolt, data->bone_id, &bolt->pos.value);
        get_camera_angle(&camera_angle);
        bolt->ang.y = camera_angle.y;
        switch (data->orientation) {
        case 0:
            bolt->ang.x = 0.0f;
            bolt->ang.z = 0.0f;
            break;
        case 1:
            bolt->ang.y += 3.1415927f;
            break;
        case 2:
            bolt->ang.z += 3.1415927f;
            bolt->pos.value.y += y_offset;
            break;
        case 3:
            bolt->ang.y += 3.1415927f;
            bolt->pos.value.y += y_offset;
            data->orientation = -1;
            break;
        default:
            data->orientation = -1;
            break;
        }
        data->orientation++;
        update_mkobj(bolt);
        result = random_ticks + 4;
        if (data->alpha != 0) {
            data->alpha--;
            if (data->alpha == 0) {
                data->fade_step = randu0(7) + 0x14;
                data->alpha = 0xFF;
            }
        } else {
            data->alpha = randu0(2) + 2;
        }
    }
    data->frame++;
    return result;
}

void fix_axe_angle(const Vec* angles) {
    PlyrMirrorObjLatch* latch;
    MkObj* axe;

    latch = &fatality_state.player->mirror_slots->weapon[0].secondary;
    axe = latch->obj;
    axe = axe != 0
              ? (axe->hdr.instance == latch->instance ? axe : 0)
              : 0;

    if (axe != 0) {
        YXZ_angles_to_quat(angles, &axe->orientation_quat);
        axe->secondary_quat = axe->orientation_quat;
        axe->secondary_quat.y *= -1.0f;
        axe->secondary_quat.z *= -1.0f;
    }
}

/*
 * Soft ceiling: the veil material, animation process, bone table, attachment
 * offset, and visibility flags follow retail; remaining differences are emit.
 */
void ft_mileena_start_veil_ripoff(void) {
    static const Vec veil_offset = {0.0f, 0.075f, -0.14f};
    AnimPdata* animation;
    RpMaterial* material;
    MkObj* veil;
    int bone_id;

    material = obj_find_material_by_id(fatality_state.attacker_object, 0xC);
    if (material != 0) {
        hide_material(material);
    }
    veil = load_named_model_for_player(
        "MIL_VEIL", plyr_pdata->plyr_num,
        plyr_pdata->plyr_num, 1);
    if (veil == 0) {
        return;
    }
    build_bones_tbl(veil, veil_bones);
    veil->flipped_bone_map = &veil_flipped_bones;
    animation = 0;
    if (create_mkproc_anim(0x2091, p_animate, &animation) != 0) {
        insert_fgnd_mkobj(veil);
        animation->obj = veil;
        animation->obj_instance = veil->hdr.instance;
        set_root_and_obj_movement_weights(0.0f, 1.0f, animation);
        set_anim_script(
            animation, plyr_pdata->mileena_veil_animation, 3);
        obj_match_pos_ang_to_src_obj(
            veil, fatality_state.attacker_object);
        bone_id = find_cloth_bone_id_from_tag(
            fatality_state.attacker_object, 7);
        get_bone_offset_world_pos(
            fatality_state.attacker_object, bone_id,
            &veil_offset, &veil->pos.value);
        veil->flags_09 &= ~0x80;
        veil->flags_09 &= ~0x40;
    }
}

void fat_goro_fold_arms(
    PlyrPdata* player, MkObj* object,
    int transition, float speed) {
    AnimPdata* animation;

    if (player->character_id != 0x1E) {
        return;
    }
    animation = 0;
    create_mkproc_anim(
        0x5002, p_animate, &animation);
    if (animation != 0) {
        animation->obj = object;
        animation->obj_instance = object->hdr.instance;
        set_anim_script(
            animation, player->goro_fold_animation, transition);
        animation->step = speed;
        set_root_and_obj_movement_weights(
            0.0f, 1.0f, animation);
    }
}

/*
 * Soft ceiling: exact retail size and costume-specific object/matcher
 * behavior; remaining differences are register allocation and scheduling.
 */
MkObj* fatality_boraicho_get_jug(Vec* angles, Vec* offset) {
    static const int canteen_bone_tag = 0x13;
    static const int gourd_bone_tag = 0x2D;
    const int* bone_tags;
    MkObj* jug;
    RpMaterial* material;
    FatalityBoneMatcher* matcher;
    int player;
    int parent_bone;

    player = fatality_state.player->plyr_num;
    if (fatality_state.player->plyr_info->flags_14_bits.alternate_costume) {
        material = obj_find_material_by_id(
            fatality_state.attacker_object, 0x9A);
        if (material != 0) {
            hide_material(material);
        }
        bone_tags = &gourd_bone_tag;
        jug = load_named_model_for_player("GOURD", player, player, 0);
        obj_create_sobjs(jug);
        material = obj_find_material_by_id(jug, 0x9A);
        if (material != 0) {
            material_set_zbias(material, -0.1f);
        }
        if (fatality_state.mirror_camera != 0) {
            angles->x = -angles->x;
            angles->z = -angles->z;
        }
    } else {
        material = obj_find_material_by_id(
            fatality_state.attacker_object, 0x99);
        if (material != 0) {
            hide_material(material);
        }
        material = obj_find_material_by_id(
            fatality_state.attacker_object, 0x98);
        if (material != 0) {
            hide_material(material);
        }
        bone_tags = &canteen_bone_tag;
        jug = load_named_model_for_player("CANTEEN", player, player, 0);
        if (fatality_state.mirror_camera != 0) {
            obj_set_bone_collapse_flag(
                fatality_state.attacker_object, 0x2E);
            obj_set_bone_collapse_flag(
                fatality_state.attacker_object, 0x2A);
        } else {
            obj_set_bone_collapse_flag(
                fatality_state.attacker_object, 0x2F);
            obj_set_bone_collapse_flag(
                fatality_state.attacker_object, 0x2B);
        }
    }

    if (jug != 0 && build_bones_tbl(jug, bone_tags) != 0) {
        insert_fgnd_mkobj(jug);
        specskin_initialize_clump(jug->clump);
        mk_insert(
            &jug->hdr,
            &fatality_state.attacker_object->child_list);
        jug->light_flags = fatality_state.attacker_object->light_flags;
        parent_bone = 0x19;
        if (f_fatality_was_done == 0 &&
            (plyr_anim_pdata->flags & 8) != 0) {
            parent_bone = 0x18;
            offset->x = -offset->x;
        }
        matcher = start_bone_matcher(
            0.0f, fatality_state.attacker_object, parent_bone, jug, 0);
        fatality_state.range34.fields34.bone_matcher = &matcher->hdr;
        matcher->flags.value |= 0x40;
        YXZ_angles_to_MKMATRIX(
            angles, (MKMATRIX*)jug->bones[0]->parent_matrix);
        YXZ_angles_to_quat(angles, &jug->bones[0]->rotation);
        bone_matcher_parent_set_offset(matcher, offset);
    }
    return jug;
}

/*
 * Soft ceiling: exact retail size and effect/texture setup; the remaining
 * delta is register allocation and equivalent early-return scheduling.
 */
FatalityEffectHandle fatality_boraicho_light_fart_torch(MkObj* torch) {
    FatalityEffectHandle emitter;
    MkPfx* effect;
    int art_section;

    emitter = fx_by_owner(
        "bo_flame_throw",
        1 << plyr_pdata->plyr_info->controller_slot);
    if (emitter == 0) {
        return 0;
    }
    emitter = fx_next_emitter(emitter);
    if (emitter == 0) {
        return 0;
    }
    fx_resume_emit(emitter);
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return 0;
    }
    pfx_bind_emitter_num_to_obj(
        effect, torch, 0, emitter_id_from_handle(emitter));

    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return 0;
    }
    art_section =
        get_shared_art_section_for_plyr_pdata(fatality_state.player);
    set_pfx_texture(
        (PfxVm*)effect->matrix, (void*)art_section,
        (void*)0x86000B);
    return emitter;
}

/*
 * Soft ceiling: exact retail size, branches, and matcher setup; remaining
 * differences are register allocation and load scheduling.
 */
MkObj* fatality_boraicho_get_torch(
    const Vec* parent_offset, const Vec* rotation) {
    static const int torch_bone_tag = 1;
    MkObj* torch;
    RpMaterial* material;
    FatalityFakeBoneMatcher* matcher;
    int player;
    int destination_bone;

    player = fatality_state.player->plyr_num;
    torch = load_named_model_for_player("TORCH", player, player, 0);
    if (torch != 0 && build_bones_tbl(torch, &torch_bone_tag) != 0) {
        insert_fgnd_mkobj(torch);
        specskin_initialize_clump(torch->clump);
        mk_insert(
            &torch->hdr,
            &fatality_state.attacker_object->child_list);
        torch->light_flags =
            fatality_state.attacker_object->light_flags;
        destination_bone =
            0x18 + (fatality_state.mirror_camera != 0);
        matcher = ft_fake_bone_matcher(
            torch, fatality_state.attacker_object, destination_bone,
            parent_offset, 0, rotation, 1, 0.0f);
        fatality_state.fake_bone_matcher = matcher;
        if ((((FatalityContextView*)fatality_state.context)->flags14 &
             0x80) != 0) {
            obj_create_sobjs(torch);
            material = obj_find_material_by_id(torch, 1);
            if (material != 0) {
                material_set_zbias(material, -0.1f);
            }
        }
    }
    return torch;
}

/*
 * Soft ceiling: exact retail size and object/matcher setup; remaining
 * differences are GPR allocation and load scheduling.
 */
MkObj* fatality_ashrah_get_doll(
    const Vec* parent_offset, const Vec* child_offset,
    const Vec* rotation) {
    MkObj* doll;
    FatalityFakeBoneMatcher* matcher;
    int player;
    int destination_bone;

    player = fatality_state.player->plyr_num;
    doll = load_named_model_for_player("DOLL", player, player, 0);
    if (doll != 0) {
        insert_fgnd_mkobj(doll);
        mk_insert(
            &doll->hdr,
            &fatality_state.attacker_object->child_list);
        doll->light_flags =
            fatality_state.attacker_object->light_flags;
        destination_bone =
            fatality_state.mirror_camera != 0 ? 0x1A : 0x1B;
        matcher = ft_fake_bone_matcher(
            doll, fatality_state.attacker_object, destination_bone,
            parent_offset, child_offset, rotation, 1, 0.0f);
        fatality_state.fake_bone_matcher = matcher;
    }
    return doll;
}

FatalityState* get_fatality_state_ptr(void) {
    return &fatality_state;
}

void call_fatality_script_function(void) {
    ScriptSlot* script;

    script = fatality_state.player->cmo;
    cmdscript_setup_execution(script, active_cmdscript->unk28);
    cmdscript_execute(script);
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(player_sleep_forever, 0.0f);
}

float p_bodyslam_bodysplat(void) {
    FatalityBodySplatPdata* data;
    MkObj* object;
    float next;

    data = (FatalityBodySplatPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 &&
        object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }
    data->current_scale += data->step;
    next = data->current_scale;
    if (next >= data->target_scale) {
        return -1.0f;
    }
    object->scale.x = next;
    object->scale.z = next;
    return 1.0f;
}

void start_bodyslam_bodysplat(
    float x, float z, float scale,
    float target_scale, float step) {
    FatalityBodySplatPdata* data;
    MkObj* object;
    int slot;

    if (_create_mkproc_generic_nostack(
            0x601D, 0x1F, p_bodyslam_bodysplat,
            sizeof(FatalityBodySplatPdata),
            (MkHdr**)&data) == 0) {
        return;
    }
    slot = fatality_state.player->plyr_num == 0
               ? 0x3000B : 0x4000B;
    object = (MkObj*)load_named_model_from_slot(
        slot, "BODYSPLAT", 0x6008, 0);
    if (object != 0) {
        data->object = object;
        data->object_instance = object->hdr.instance;
        insert_fgnd_mkobj(object);
        object->pos.value.x = x;
        object->pos.value.y = g_game_info.field_34 + 0.001f;
        object->pos.value.z = z;
        object->flags_08 |= 2;
        object->scale.x = 1.0f;
        object->scale.y = 1.0f;
        object->scale.z = 1.0f;
        data->current_scale = scale;
        data->target_scale = target_scale;
        data->step = step;
    }
}

MkObj* load_cloth_boned_model(
    const char* name, int object_id, int slot, int flags,
    const int* bone_tags, ClothInitEntry* cloth_table,
    int cloth_count) {
    MkObj* object;

    object = load_named_model_for_player(
        name, slot, flags, object_id);
    if (object != 0) {
        insert_fgnd_mkobj(object);
        object->light_flags =
            fatality_state.attacker_object->light_flags;
        mk_insert(
            &object->hdr,
            &fatality_state.attacker_object->child_list);
        build_bones_tbl(object, bone_tags);
        start_cloth_bones(object);
        cloth_bones_init_by_tbl(
            object, cloth_table, cloth_count);
    }
    return object;
}

/*
 * Soft ceiling: every validated severed-limb latch, conditional bone choice,
 * blood script, delay, and meat-chunk emitter follows the retail sequence.
 */
void fatality_explode_victim(PlyrInfo* player_info) {
    FighterMirror* fighter;
    PlyrPdata* player;
    MkObj* limb;
    MkObj* effect_owner;
    MkPfx* effect;
    int bone_id;

    limb_sever_explode_apart(player_info, 3, 0.25f);
    fighter = player_info->slot.fighter;
    player = player_info->slot.pdata;
    effect_owner = player_info->slot.mirror_a;
    effect = find_pfx_by_name("meatchunk");
    if (effect != 0) {
        pfx_bind_emitter_to_obj_bone(effect, effect_owner, 0xD);
        reset_effect("meatchunk");
        resume_effect("meatchunk");
    }

    fatality_spawn_limb_blood(fighter, player, 0, 0x18, 0x10);

    limb = fatality_get_severed_limb(fighter, 13);
    if (limb != 0) {
        bone_id = limb->bones[3] != 0 ? 3 : 0;
        auto_calc_limbobj_bone_world_pos(limb, bone_id);
        start_blood_particles(0x18, bone_id, player, limb);
        start_blood_particles(2, bone_id, player, limb);
    }
    fatality_spawn_limb_blood(fighter, player, 3, 2, 0x13);
    fatality_spawn_limb_blood(fighter, player, 11, 2, 4);

    FATALITY_SLEEP(10.0f);

    fatality_spawn_limb_blood(fighter, player, 14, 0x18, 0xF);
    fatality_spawn_limb_blood(fighter, player, 8, 0x18, 5);
    limb = fatality_get_severed_limb(fighter, 14);
    if (limb != 0) {
        bone_id = limb->bones[13] != 0 ? 0xD : 9;
        auto_calc_limbobj_bone_world_pos(limb, bone_id);
        start_blood_particles(1, bone_id, player, limb);
    }
    fatality_spawn_limb_blood(fighter, player, 12, 1, 1);
    fatality_spawn_limb_blood(fighter, player, 9, 1, 2);
}

/*
 * Soft ceiling: exact retail size and emitter operations. Remaining deltas
 * are GPR allocation/save style and equivalent loop-induction scheduling.
 */
void fire_multi_emitter_pfx_via_tbl(
    const char* effect_name, const FatalityEmitterBind* table,
    MkObj* object, FatalityEffectHandle* handles) {
    FatalityEffectHandle emitter;
    MkPfx* effect;
    const FatalityEmitterBind* bind;
    int emitter_id;
    int index;

    emitter = fx_by_owner(
        effect_name,
        1 << plyr_pdata->plyr_info->controller_slot);
    if (emitter == 0) {
        return;
    }
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return;
    }

    index = 0;
    bind = table;
    while (bind->bone_id >= 0) {
        emitter = fx_next_emitter(emitter);
        if (emitter == 0) {
            return;
        }
        fx_resume_emit(emitter);
        emitter_id = emitter_id_from_handle(emitter);
        if ((unsigned int)(bind->bone_id + 0xC0000000) == 0) {
            pfx_bind_emitter_num_to_obj(
                effect, object, 0, emitter_id);
        } else {
            pfx_bind_emitter_num_to_obj_bone(
                effect, object, bind->bone_id, emitter_id);
        }
        effect->emitter_scratch[emitter_id].position.x = bind->position.x;
        effect->emitter_scratch[emitter_id].position.y = bind->position.y;
        effect->emitter_scratch[emitter_id].position.z = bind->position.z;
        if (handles != 0) {
            handles[index] = emitter;
        }
        index++;
        bind++;
    }
}

FatalityEffectHandle pfxhandle_bgnd_spawn_at_sobj_id(
    const char* name, unsigned int sobj_id) {
    FatalityEffectHandle effect_handle;
    FatalityEffectHandle emitter;
    MkPfx* effect;
    MkSobj* sobj;

    effect_handle = fx_by_owner(name, 4);
    if (effect_handle == 0) {
        return 0;
    }
    emitter = fx_next_emitter(effect_handle);
    if (emitter == 0) {
        return 0;
    }
    fx_resume_emit(emitter);
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return 0;
    }
    sobj = (MkSobj*)obj_find_sobj_by_id(
        g_game_info.bgnd_obj, sobj_id);
    if (sobj == 0) {
        return 0;
    }
    pfx_bind_emitter_num_to_sobj(
        effect, sobj, 0, emitter_id_from_handle(emitter));
    return emitter;
}

FatalityEffectHandle pfxhandle_spawn_at_bid_next_bind_render(
    FatalityEffectHandle effect, MkObj* object, int bone_id) {
    return fatality_bind_next_emitter(effect, object, bone_id, 1);
}

FatalityEffectHandle pfxhandle_spawn_at_bid_next(
    FatalityEffectHandle effect, MkObj* object, int bone_id) {
    return fatality_bind_next_emitter(effect, object, bone_id, 0);
}

FatalityEffectHandle pfxhandle_spawn_at_bid(
    const char* name, MkObj* object, int bone_id) {
    FatalityEffectHandle effect;

    effect = fx_by_owner(
        name,
        1 << fatality_state.player->plyr_info->controller_slot);
    if (effect != 0) {
        return fatality_bind_next_emitter(
            effect, object, bone_id, 0);
    }
    return 0;
}

void pfx_spawn_at_bid(
    const char* name, MkObj* object, int bone_id) {
    MkPfx* effect;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        pfx_bind_emitter_to_obj_bone(effect, object, bone_id);
        reset_effect(name);
        resume_effect(name);
    }
}

void play_final_fatality_music(void) {
    if (fatality_loop_sound != 0) {
        snd_stop(fatality_loop_sound);
    }
    if (g_game_info.field_200 == 3) {
        snd_req(0x1A9E);
    } else {
        snd_req(0x1A9C);
    }
}

float p_fatality_cam(void) {
    CamVec3 angle;
    CamVec3 position;

    angle.x = 0.0f;
    angle.y = camera_obj->ang.y - 1.5707964f;
    angle.z = 0.0f;
    camera_set_animation_parent_angle(&angle, 0);
    position.x = fatality_state.attacker_object->pos.value.x;
    position.y = camera_obj->pos.y;
    position.z = fatality_state.attacker_object->pos.value.z;
    camera_set_animation_parent_position(&position);
    if (fatality_state.mirror_camera != 0) {
        camera_set_animation_mirror_plane(2);
    }
    camera_run_animation(0);
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: exact retail size and cleanup/sleep control flow; remaining
 * differences are register allocation and inlined-helper scheduling.
 */
static float end_of_fatality(void) {
    xfer_proc(plyr_anim_proc, p_anim_idle);
    if (g_game_info.field_200 != 3) {
        xfer_proc(fatality_state.attacker_proc, p_anim_idle);
        xfer_proc(fatality_state.victim_proc, player_sleep_forever);
        fatality_finish_sidekick(fatality_state.player_info);
    } else {
        fatality_finish_sidekick(fatality_state.context);
    }
    reset_game_speed();
    f_fatality_finished = 1;
    FATALITY_SLEEP(60.0f);
    pop_game_state();
    for (;;) {
        FATALITY_SLEEP(60.0f);
    }
}

void fkbm_obj_face_obj(
    FatalityFacingController* controller, MkObj* source, int source_bone,
    MkObj* target, int target_bone) {
    FatalityFaceObjectPdata* data;

    if (_create_mkproc_generic_nostack(
            0x600D, 0x1E, p_face_obj,
            sizeof(FatalityFaceObjectPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityFaceObjectPdata), &data->hdr);
        data->source = source;
        data->source_instance = source->hdr.instance;
        data->target = target;
        data->target_instance = target->hdr.instance;
        data->source_bone = source_bone;
        data->target_bone = target_bone;
        data->controller = controller;
    }
}

/*
 * Soft ceiling: exact retail size, validation, and open-coded vector math;
 * remaining differences are register allocation and branch scheduling.
 */
static float p_face_obj(void) {
    FatalityFaceObjectPdata* data;
    MkObj* source;
    MkObj* target;
    Vec source_position;
    Vec direction;

    data = (FatalityFaceObjectPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    source = data->source;
    if (source != 0 && source->hdr.instance != data->source_instance) {
        source = 0;
    }
    if (source == 0) {
        return -1.0f;
    }
    target = data->target;
    if (target != 0 && target->hdr.instance != data->target_instance) {
        target = 0;
    }
    if (target == 0 || data->source_bone < 0) {
        return -1.0f;
    }

    if ((unsigned int)(data->source_bone + 0xC0000000) != 0) {
        get_bone_world_pos(
            source, data->source_bone, &source_position);
    } else {
        source_position.x = source->pos.value.x;
        source_position.y = source->pos.value.y;
        source_position.z = source->pos.value.z;
    }
    if ((unsigned int)(data->target_bone + 0xC0000000) != 0) {
        get_bone_world_pos(target, data->target_bone, &direction);
    } else {
        direction.x = target->pos.value.x;
        direction.y = target->pos.value.y;
        direction.z = target->pos.value.z;
    }
    direction.x = source_position.x - direction.x;
    direction.y = source_position.y - direction.y;
    direction.z = source_position.z - direction.z;
    v3_to_xy_ang(&data->controller->angles, &direction);
    data->controller->flags20 |= 0x80;
    return 1.0f;
}

/*
 * Soft ceiling: exact retail size and matcher reset semantics; residue is
 * register allocation and equivalent bitfield/load scheduling.
 */
void reset_fake_bone_matcher(
    FatalityFakeBoneMatcher* matcher, const Vec* parent_offset,
    const Vec* child_offset, const Vec* rotation, int child_bone,
    MkObj* child, float blend) {
    MkObj* parent;
    MkBone* bone;

    parent = matcher->parent;
    if (parent != 0 &&
        parent->hdr.instance != matcher->parent_instance) {
        parent = 0;
    }
    if (parent == 0 || child == 0) {
        return;
    }

    matcher->blend = blend;
    matcher->blend_target = blend;
    matcher->child = child;
    matcher->child_instance = child->hdr.instance;
    if (parent_offset != 0) {
        matcher->parent_offset.x = parent_offset->x;
        matcher->parent_offset.y = parent_offset->y;
        matcher->parent_offset.z = parent_offset->z;
        matcher->flags.bits.has_parent_offset = 1;
    }
    if (child_offset != 0) {
        matcher->child_offset.x = child_offset->x;
        matcher->child_offset.y = child_offset->y;
        matcher->child_offset.z = child_offset->z;
        matcher->flags.bits.has_child_offset = 1;
    }
    if (rotation != 0) {
        matcher->rotation.x = rotation->x;
        matcher->rotation.y = rotation->y;
        matcher->rotation.z = rotation->z;
        matcher->flags.bits.has_rotation = 1;
    }
    matcher->child_bone = child_bone;
    bone = child->bones[child_bone];
    if (bone == 0) {
        return;
    }
    bone->flags_54_bits.calculation_locked = 1;
    if (blend != 0.0f) {
        matcher->blend = blend;
        matcher->blend_target = blend;
        matcher->parent_position.x = parent->field_24->pos.x;
        matcher->parent_position.y = parent->field_24->pos.y;
        matcher->parent_position.z = parent->field_24->pos.z;
    }
}

/*
 * Soft ceiling: exact retail size and creation/validation behavior; residue
 * is register allocation and repeated pdata-load scheduling.
 */
FatalityFakeBoneMatcher* ft_fake_bone_matcher(
    MkObj* parent, MkObj* child, int child_bone,
    const Vec* parent_offset, const Vec* child_offset,
    const Vec* rotation, int mode, float blend) {
    FatalityFakeBoneMatcher* matcher;
    MkProc* process;
    MkBone* bone;

    matcher = 0;
    process = _create_mkproc_generic_nostack(
        0x600C, 0x1F, p_fake_bone_matcher_proc,
        sizeof(FatalityFakeBoneMatcher), (MkHdr**)&matcher);
    if (process == 0) {
        return matcher;
    }
    zero_pdata_payload(
        sizeof(FatalityFakeBoneMatcher), &matcher->hdr);
    matcher->process = process;
    matcher->process_instance = process->hdr.instance;
    matcher->parent = parent;
    matcher->parent_instance = parent->hdr.instance;
    matcher->child = child;
    matcher->child_instance = child->hdr.instance;
    if (parent_offset != 0) {
        matcher->parent_offset.x = parent_offset->x;
        matcher->parent_offset.y = parent_offset->y;
        matcher->parent_offset.z = parent_offset->z;
        matcher->flags.bits.has_parent_offset = 1;
    }
    if (child_offset != 0) {
        matcher->child_offset.x = child_offset->x;
        matcher->child_offset.y = child_offset->y;
        matcher->child_offset.z = child_offset->z;
        matcher->flags.bits.has_child_offset = 1;
    }
    if (rotation != 0) {
        matcher->rotation.x = rotation->x;
        matcher->rotation.y = rotation->y;
        matcher->rotation.z = rotation->z;
        matcher->flags.bits.has_rotation = 1;
    }
    matcher->flags.bits.mode = mode;
    matcher->child_bone = child_bone;
    bone = child->bones[child_bone];
    if (bone == 0) {
        if (matcher->hdr.instance != 0) {
            matcher->hdr.typed_vtbl->destroy(&matcher->hdr);
        }
        return 0;
    }
    bone->flags_54_bits.calculation_locked = 1;
    if (blend != 0.0f) {
        matcher->blend = blend;
        matcher->blend_target = blend;
        matcher->parent_position.x = parent->pos.value.x;
        matcher->parent_position.y = parent->pos.value.y;
        matcher->parent_position.z = parent->pos.value.z;
    }
    mk_insert(&parent->hdr, &process->pdata_list);
    return matcher;
}

/*
 * Soft ceiling: exact retail size and matrix/blend behavior; remaining
 * differences are register allocation and instruction scheduling.
 */
static float p_fake_bone_matcher_proc(void) {
    FatalityFakeBoneMatcher* matcher;
    MkObj* parent;
    MkObj* child;
    RwMatrix* parent_matrix;
    RwMatrix* child_matrix;
    Vec position;
    MKMATRIX rotation_matrix;
    float blend_scale;

    matcher = (FatalityFakeBoneMatcher*)apdata;
    if (matcher == 0) {
        return -1.0f;
    }
    parent = matcher->parent;
    if (parent != 0 &&
        parent->hdr.instance != matcher->parent_instance) {
        parent = 0;
    }
    if (parent == 0) {
        return -1.0f;
    }
    child = matcher->child;
    if (child != 0 && child->hdr.instance != matcher->child_instance) {
        child = 0;
    }
    if (child == 0) {
        if (parent->hdr.instance != 0) {
            parent->hdr.typed_vtbl->destroy(&parent->hdr);
        }
        return -1.0f;
    }

    child_matrix = child->field_24;
    if (matcher->child_bone >= 0 &&
        child->bones[matcher->child_bone] != 0) {
        child_matrix =
            child->bones[matcher->child_bone]->parent_matrix;
    }
    parent_matrix = parent->field_24;
    if (matcher->flags.bits.has_parent_offset) {
        if (matcher->flags.bits.mode) {
            YXZ_angles_to_MKMATRIX(
                &matcher->parent_offset, &rotation_matrix);
            mat_x_mat(parent_matrix, &rotation_matrix, child_matrix);
        } else {
            YXZ_angles_to_MKMATRIX(
                &matcher->parent_offset, parent_matrix);
        }
    } else if (matcher->flags.bits.mode) {
        memcpy(parent_matrix, child_matrix, 0x30);
    }
    if (matcher->flags.bits.has_child_offset) {
        mat_scaled_by_v3(
            parent_matrix, parent_matrix, &matcher->child_offset);
    }
    if (matcher->flags.bits.has_rotation) {
        v3_x_mat_add_v3(
            &position, &matcher->rotation, child_matrix,
            &child_matrix->pos_vec);
    } else {
        position.x = child_matrix->pos.x;
        position.y = child_matrix->pos.y;
        position.z = child_matrix->pos.z;
    }
    if (matcher->blend > 0.0f) {
        blend_scale =
            (game_speed * (matcher->blend_target - matcher->blend)) /
            matcher->blend_target;
        position.x =
            (position.x - matcher->parent_position.x) * blend_scale +
            matcher->parent_position.x;
        position.y =
            (position.y - matcher->parent_position.y) * blend_scale +
            matcher->parent_position.y;
        position.z =
            (position.z - matcher->parent_position.z) * blend_scale +
            matcher->parent_position.z;
        matcher->blend -= game_speed;
    }
    parent_matrix->pos.x = position.x;
    parent_matrix->pos.y = position.y;
    parent_matrix->pos.z = position.z;
    RwMatrixUpdate(parent_matrix);
    RwFrameUpdateObjects(parent->frame);
    return 1.0f;
}

MkHdr* get_fake_bone_matcher_proc(MkObjLatch* matcher) {
    MkHdr* proc;
    MkHdr* result = 0;

    if (matcher != 0) {
        proc = matcher->obj;
        if (proc != 0) {
            if (proc->instance == matcher->obj_instance) {
                /* The instance latch still identifies this process. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        result = proc;
    }
    return result;
}

void obj_grnd_bounce(
    MkObj* object, const Vec* velocity, int bounces,
    float gravity, float ground_offset, float restitution) {
    FatalityGroundBouncePdata* data;

    if (_create_mkproc_generic_nostack(
            0x600B, 0x1F, p_obj_grnd_bounce,
            sizeof(FatalityGroundBouncePdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityGroundBouncePdata), &data->hdr);
        data->object = object;
        data->object_instance = object->hdr.instance;
        if (velocity != 0) {
            object->pos_vel = *velocity;
        }
        object->flags_08 |= 0x20;
        object->gravity = gravity;
        if (gravity != 0.0f) {
            object->flags_08 |= 1;
        }
        data->ground_offset = ground_offset;
        data->bounce_count = bounces;
        data->restitution = restitution;
        update_mkobj(object);
    }
}

/*
 * Soft ceiling: retail m2c confirms the retained four-way sound switch,
 * bounce response, completion flags, ground snap, and process return values.
 * Source is eight bytes smaller; residue is latch/save-register allocation,
 * branch scheduling, and float relocation labels.
 */
float p_obj_grnd_bounce(void) {
    FatalityGroundBouncePdata* data;
    MkObj* object;

    data = (FatalityGroundBouncePdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object == 0 ||
        object->hdr.instance != data->object_instance) {
        return -1.0f;
    }
    if (object->pos_vel.y != 0.0f &&
        object->pos.value.y <= g_game_info.field_34 + data->ground_offset) {
        if (data->bounce_count != 0) {
            switch (data->bounce_count) {
            case 0:
                if (randu0(2) != 0) {
                    snd_req(0xD96);
                } else {
                    snd_req(0xD97);
                }
                break;
            case 1:
            case 2:
                snd_req(0xD94);
                break;
            default:
                if (randu0(2) != 0) {
                    snd_req(0xD9A);
                } else {
                    snd_req(0xD95);
                }
                break;
            }
            object->pos_vel.y =
                (float)data->bounce_count *
                (-object->pos_vel.y * data->restitution);
            data->bounce_count--;
        } else {
            object->flags_08_bits.rotation_enabled = 0;
            object->flags_08_bits.gravity_enabled = 0;
            object->flags_08_bits.moving = 0;
            object->flags_09_bits.launched = 1;
            object->flags_09_bits.bit6 = 1;
            object->pos.value.y =
                g_game_info.field_34 + data->ground_offset;
            return -1.0f;
        }
    }
    return 1.0f;
}

/*
 * Soft ceiling: retail's object latch, nine scalar stores, initial object
 * scale, and update call are exact. The remaining delta is GPR allocation and
 * an 8-byte lmw/stmw frame-emission difference.
 */
void start_obj_scalar_proc(
    MkObj* object, const Vec* start,
    const Vec* target, const Vec* step) {
    FatalityObjectScalarPdata* data;

    if (_create_mkproc_generic_nostack(
            0x600A, 0x1F, p_obj_scalar_proc,
            sizeof(FatalityObjectScalarPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityObjectScalarPdata), &data->hdr);
        data->object = object;
        data->object_instance = object->hdr.instance;
        object->flags_08 |= 2;
        data->start.x = start->x;
        data->start.y = start->y;
        data->start.z = start->z;
        data->target.x = target->x;
        data->target.y = target->y;
        data->target.z = target->z;
        data->step.x = step->x;
        data->step.y = step->y;
        data->step.z = step->z;
        object->scale.x = start->x;
        object->scale.y = start->y;
        object->scale.z = start->z;
        update_mkobj(object);
    }
}

/*
 * Soft ceiling: all three retail open-coded scalar axes and signed flag-bit
 * extractions are recovered. Source is 12 bytes smaller; residue is the
 * compiler's inline latch join, register allocation, branches, and relocs.
 */
float p_obj_scalar_proc(void) {
    FatalityObjectScalarPdata* data;
    MkObj* object;
    int active;
    float step;
    float swap;

    data = (FatalityObjectScalarPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = fatality_resolve_object_latch(
        data->object, data->object_instance);
    if (object == 0) {
        return -1.0f;
    }
    active = 0;
    if (object->scale.x != data->target.x) {
        active = 1;
        if (data->flag_bits.multiply != 0) {
            object->scale.x *= data->step.x;
        } else {
            object->scale.x += data->step.x;
        }
        step = data->step.x;
        if ((step > 0.0f && object->scale.x > data->target.x) ||
            (step < 0.0f && object->scale.x < data->target.x)) {
            object->scale.x = data->target.x;
            if (data->flag_bits.ping_pong != 0 &&
                data->flag_bits.swap_limits != 0) {
                data->step.x = -data->step.x;
                swap = data->target.x;
                data->target.x = data->start.x;
                data->start.x = swap;
            }
        }
    }
    if (object->scale.y != data->target.y) {
        active++;
        if (data->flag_bits.multiply != 0) {
            object->scale.y *= data->step.y;
        } else {
            object->scale.y += data->step.y;
        }
        step = data->step.y;
        if ((step > 0.0f && object->scale.y > data->target.y) ||
            (step < 0.0f && object->scale.y < data->target.y)) {
            object->scale.y = data->target.y;
            if (data->flag_bits.ping_pong != 0 &&
                data->flag_bits.swap_limits != 0) {
                data->step.y = -data->step.y;
                swap = data->target.y;
                data->target.y = data->start.y;
                data->start.y = swap;
            }
        }
    }
    if (object->scale.z != data->target.z) {
        active++;
        if (data->flag_bits.multiply != 0) {
            object->scale.z *= data->step.z;
        } else {
            object->scale.z += data->step.z;
        }
        step = data->step.z;
        if ((step > 0.0f && object->scale.z > data->target.z) ||
            (step < 0.0f && object->scale.z < data->target.z)) {
            object->scale.z = data->target.z;
            if (data->flag_bits.ping_pong != 0 &&
                data->flag_bits.swap_limits != 0) {
                data->step.z = -data->step.z;
                swap = data->target.z;
                data->target.z = data->start.z;
                data->start.z = swap;
            }
        }
    }
    if (active != 0) {
        return 1.0f;
    }
    if (data->flag_bits.stop_when_complete != 0 ||
        data->flag_bits.ping_pong == 0) {
        return -1.0f;
    }
    data->step.x = -data->step.x;
    swap = data->target.x;
    data->target.x = data->start.x;
    data->start.x = swap;
    data->step.y = -data->step.y;
    swap = data->target.y;
    data->target.y = data->start.y;
    data->start.y = swap;
    data->step.z = -data->step.z;
    swap = data->target.z;
    data->target.z = data->start.z;
    data->start.z = swap;
    return 1.0f;
}

void obj_match_obj_pos(
    MkObj* source, MkObj* destination, int snap, float blend) {
    FatalityObjectMatcherPdata* data;

    if (_create_mkproc_generic_nostack(
            0x6009, 0x1F, p_obj_pos_matcher,
            sizeof(FatalityObjectMatcherPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityObjectMatcherPdata), &data->hdr);
        data->source = source;
        data->source_instance = source->hdr.instance;
        data->destination = destination;
        data->destination_instance = destination->hdr.instance;
        if (snap) {
            destination->pos.value.x = source->pos.value.x;
            destination->pos.value.y = source->pos.value.y;
            destination->pos.value.z = source->pos.value.z;
        }
        destination->flags_08 |= 0x40;
        data->blend = blend;
    }
}

float p_obj_pos_matcher(void) {
    FatalityObjectMatcherPdata* data;
    MkObj* source;
    MkObj* destination;
    float blend;

    data = (FatalityObjectMatcherPdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    source = data->source;
    if (source == 0 ||
        source->hdr.instance != data->source_instance) {
        return 0.0f;
    }
    destination = data->destination;
    if (destination == 0 ||
        destination->hdr.instance != data->destination_instance) {
        return 0.0f;
    }
    blend = data->blend;
    if (blend == 1.0f) {
        destination->pos.value = source->pos.value;
    } else {
        destination->pos.value.x +=
            (source->pos.value.x - destination->pos.value.x) * blend;
        destination->pos.value.y +=
            (source->pos.value.y - destination->pos.value.y) * blend;
        destination->pos.value.z +=
            (source->pos.value.z - destination->pos.value.z) * blend;
    }
    return 1.0f;
}

void obj_unhide_material_by_id(MkObj* object, int id) {
    RpMaterial* material;

    material = obj_find_material_by_id(object, id);
    if (material != 0) {
        show_material(material);
    }
}

void obj_hide_material_by_id(MkObj* object, int id) {
    RpMaterial* material;

    material = obj_find_material_by_id(object, id);
    if (material != 0) {
        hide_material(material);
    }
}

void bone_matcher_reset_dest_mat_rot(MkObj* object, int bone_index) {
    MKMatrixSetIdentity(
        (MKMATRIX*)object->bones[bone_index]->parent_matrix);
    object->bones[bone_index]->rotation.x = 0.0f;
    object->bones[bone_index]->rotation.y = 0.0f;
    object->bones[bone_index]->rotation.z = 0.0f;
    object->bones[bone_index]->rotation.w = 1.0f;
}

void bone_matcher_set_ang_pos(
    FatalityBoneMatcher* matcher, MkObj* object, int bone_index,
    const Vec* angles, Vec* offset, int copy_bone_matrix) {
    matcher->flags.bits.copy_bone_matrix = copy_bone_matrix;
    if (angles != 0) {
        YXZ_angles_to_MKMATRIX(
            angles,
            (MKMATRIX*)object->bones[bone_index]->parent_matrix);
        YXZ_angles_to_quat(
            angles, &object->bones[bone_index]->rotation);
    }
    if (offset != 0) {
        bone_matcher_parent_set_offset(matcher, offset);
    }
}

MkObj* weapon_bm_ignore(int weapon, int ignored) {
    PlyrMirrorObjLatch* latch;
    MkObj* object;

    if (weapon == 0) {
        latch = &plyr_pdata->mirror_slots->weapon[0].secondary;
        object = latch->obj;
        if (object != 0 && object->hdr.instance != latch->instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 =
                (object->flags_08 & ~0x80) |
                ((ignored << 7) & 0x80);
            return object;
        }
    }
    latch = &plyr_pdata->mirror_slots->weapon[1].secondary;
    object = latch->obj;
    if (object != 0 && object->hdr.instance != latch->instance) {
        object = 0;
    }
    if (object != 0) {
        object->flags_08 =
            (object->flags_08 & ~0x80) | ((ignored << 7) & 0x80);
    }
    return object;
}

/*
 * Soft ceiling: exact retail size and attachment/bone setup; remaining
 * differences are register allocation and equivalent load scheduling.
 */
FatalityWeaponAttachment* regrab_weapon(
    int secondary, MkObj* object, MkHdr* bound_object, int bone_id,
    const Vec* angles, const Vec* scale, const Vec* position) {
    PlyrMirrorObjLatch* latch;
    FatalityWeaponAttachment* attachment;
    MkBone* bone;

    if (secondary == 0) {
        latch = &plyr_pdata->mirror_slots->weapon[0].secondary;
    } else {
        latch = &plyr_pdata->mirror_slots->weapon[1].secondary;
    }
    attachment = (FatalityWeaponAttachment*)latch->obj;
    if (attachment != 0 &&
        attachment->hdr.instance != latch->instance) {
        attachment = 0;
    }
    if (attachment == 0) {
        return 0;
    }

    if (angles != 0) {
        bone = object->bones[object->fallback_bone_index];
        bone->parent_matrix->pos.x = 0.0f;
        bone->parent_matrix->pos.y = 0.0f;
        bone->parent_matrix->pos.z = 0.0f;
        ZYX_angles_to_MKMATRIX(angles, bone->parent_matrix);
        YXZ_angles_to_quat(angles, &bone->rotation);
    }
    if (scale != 0) {
        object->scale.x = scale->x;
        object->scale.y = scale->y;
        object->scale.z = scale->z;
        object->flags_08_bits.scale_active = 1;
    }
    if (position != 0) {
        attachment->position.x = position->x;
        attachment->position.y = position->y;
        attachment->position.z = position->z;
        attachment->velocity.x = 0.0f;
        attachment->velocity.y = 0.0f;
        attachment->velocity.z = 0.0f;
    }
    attachment->bound_object = bound_object;
    attachment->bound_instance = bound_object->instance;
    attachment->bone_id = bone_id;
    attachment->flags08 |= 0x40;
    attachment->flags08 &= (unsigned char)~1;
    return attachment;
}

void weapon_reflection_show_hide(
    PlyrPdata* player, int secondary, int hidden) {
    FatalityWeaponReflectionSet* reflections;
    MkObj* object;
    unsigned int instance;

    if (secondary == 0) {
        reflections = (FatalityWeaponReflectionSet*)
            player->fighter_definition;
        object = reflections->primary;
        instance = reflections->primary_instance;
        if (object != 0 && object->hdr.instance != instance) {
            object = 0;
        }
        if (object != 0) {
            object->hide_flags =
                (object->hide_flags & ~0x20) |
                ((hidden << 5) & 0x20);
            return;
        }
    }
    reflections = (FatalityWeaponReflectionSet*)
        player->fighter_definition;
    object = reflections->secondary;
    instance = reflections->secondary_instance;
    if (object != 0 && object->hdr.instance != instance) {
        object = 0;
    }
    if (object != 0) {
        object->hide_flags =
            (object->hide_flags & ~0x20) |
            ((hidden << 5) & 0x20);
    }
}

/*
 * Soft ceiling: exact retail size and style/latch/reflection behavior;
 * remaining differences are GPR allocation and branch scheduling.
 */
MkObj* show_single_weapon(PlyrPdata* player, int secondary) {
    PlyrWeaponStyle* style;
    PlyrMirrorObjLatch* latch;
    MkObj* weapon;
    MkObj* reflection;

    while (is_weapon_style(player->fighter_definition) == 0) {
        player->player_slot++;
        if (player->player_slot >= 3) {
            player->player_slot = 0;
        }
        style = player->weapon_styles[player->player_slot];
        player->fighter_definition = (PlyrFighterDefinition*)style;
        player->mirror_slots = &style->mirror_slots;
        player->fighter_definition_instance = style->instance;
    }

    weapon = 0;
    style = (PlyrWeaponStyle*)player->fighter_definition;
    if (secondary == 0) {
        latch = &style->mirror_slots.weapon[0].primary;
        weapon = latch->obj;
        if (weapon != 0 && weapon->hdr.instance != latch->instance) {
            weapon = 0;
        }
        if (weapon != 0) {
            plyr_weapon_grab(player, weapon);
            weapon->hide_flags &= (unsigned char)~0x20;
            if ((g_game_info.section->flags70 & 8) != 0) {
                style = (PlyrWeaponStyle*)plyr_pdata->fighter_definition;
                latch = &style->mirror_slots.weapon[0].mirror;
                reflection = latch->obj;
                if (reflection != 0 &&
                    reflection->hdr.instance != latch->instance) {
                    reflection = 0;
                }
                if (reflection != 0) {
                    reflection->hide_flags &= (unsigned char)~0x20;
                }
            }
        }
    }
    if (weapon == 0) {
        style = (PlyrWeaponStyle*)player->fighter_definition;
        latch = &style->mirror_slots.weapon[1].primary;
        weapon = latch->obj;
        if (weapon != 0 && weapon->hdr.instance != latch->instance) {
            weapon = 0;
        }
        if (weapon != 0) {
            plyr_weapon2_grab(player, weapon);
            weapon->hide_flags &= (unsigned char)~0x20;
            if ((g_game_info.section->flags70 & 8) != 0) {
                style = (PlyrWeaponStyle*)plyr_pdata->fighter_definition;
                latch = &style->mirror_slots.weapon[1].mirror;
                reflection = latch->obj;
                if (reflection != 0 &&
                    reflection->hdr.instance != latch->instance) {
                    reflection = 0;
                }
                if (reflection != 0) {
                    reflection->hide_flags &= (unsigned char)~0x20;
                }
            }
        }
    }
    return weapon;
}

MkObj* clone_my_weapon(
    WeaponDefinition* definition, FatalityWeaponSource* source) {
    MkObj* weapon;

    weapon = load_weapon(definition, source->player_object);
    if (weapon != 0) {
        obj_create_sobjs(weapon);
        sobj_set_priority(obj_first_sobj(weapon), 6);
    }
    return weapon;
}

void clone_weapon_to_secondary(
    WeaponDefinition* definition, FatalityWeaponSource* source) {
    PlyrWeaponStyle* style;
    PlyrMirrorObjLatch* weapon_latch;
    PlyrMirrorObjLatch* reflection_latch;
    MkObj* weapon;
    MkObj* reflection;

    style = source->owner->weapon_styles[2];
    weapon_latch = &style->mirror_slots.weapon[1].primary;
    reflection_latch = &style->mirror_slots.weapon[1].mirror;
    weapon = load_weapon(definition, source->player_object);
    if (weapon == 0) {
        return;
    }
    weapon_latch->obj = weapon;
    weapon_latch->instance = weapon->hdr.instance;
    mk_insert(&weapon->hdr, &style->object_list);
    reflection = load_weapon_reflection(definition, source->player_object);
    if (reflection != 0) {
        reflection_latch->obj = reflection;
        reflection_latch->instance = reflection->hdr.instance;
        mk_insert(&reflection->hdr, &style->object_list);
        obj_create_sobjs(reflection);
        sobj_set_priority(obj_first_sobj(reflection), 6);
    }
}

void advance_to_weapon_style(PlyrPdata* player) {
    if (player->plyr_info->player_index == 0x1B) {
        return;
    }
    while (is_weapon_style(player->fighter_definition) == 0) {
        advance_active_moveset(player);
    }
}

int fat_bgnd_char_setup_radius_check(
    const FatalityRadiusCheck* check) {
    union {
        float value;
        unsigned int bits;
    } inverse;
    Vec attacker_delta;
    Vec victim_delta;
    const Vec* selected;
    CameraObj* camera;
    float attacker_distance_sq;
    float victim_distance_sq;
    float selected_distance_sq;
    float radius_sq;
    float correction;
    float x;
    float z;

    attacker_delta.x =
        fatality_state.attacker_object->pos.value.x - check->center_x;
    attacker_delta.z =
        fatality_state.attacker_object->pos.value.z - check->center_z;
    victim_delta.x =
        fatality_state.victim_object->pos.value.x - check->center_x;
    victim_delta.z =
        fatality_state.victim_object->pos.value.z - check->center_z;
    attacker_distance_sq =
        attacker_delta.x * attacker_delta.x +
        attacker_delta.z * attacker_delta.z;
    victim_distance_sq =
        victim_delta.x * victim_delta.x +
        victim_delta.z * victim_delta.z;
    radius_sq = check->radius * check->radius;
    if (!check->select_farthest) {
        if (check->radius < 0.0f) {
            if (attacker_distance_sq < radius_sq ||
                victim_distance_sq < radius_sq) {
                return 0;
            }
        } else if (attacker_distance_sq > radius_sq ||
                   victim_distance_sq > radius_sq) {
            return 0;
        }
        return 1;
    }
    if (check->radius < 0.0f) {
        selected = attacker_distance_sq < victim_distance_sq
                       ? &attacker_delta : &victim_delta;
        selected_distance_sq =
            attacker_distance_sq < victim_distance_sq
                ? attacker_distance_sq : victim_distance_sq;
    } else {
        selected = attacker_distance_sq > victim_distance_sq
                       ? &attacker_delta : &victim_delta;
        selected_distance_sq =
            attacker_distance_sq > victim_distance_sq
                ? attacker_distance_sq : victim_distance_sq;
    }
    if ((check->radius < 0.0f &&
         selected_distance_sq >= radius_sq) ||
        (check->radius >= 0.0f &&
         selected_distance_sq <= radius_sq)) {
        return 1;
    }
    if (fatality_state.player->character_id == 0x12 &&
        g_game_info.field_200 == 1 &&
        selected == &victim_delta) {
        return 1;
    }
    if (selected_distance_sq <= 0.0f) {
        inverse.value = 0.0f;
    } else {
        inverse.value = selected_distance_sq;
        inverse.bits = 0x5F375A00 - (inverse.bits >> 1);
        x = inverse.value *
            (selected_distance_sq * inverse.value);
        z = 3.0f - x;
        inverse.value = 0.0625f * inverse.value * z *
            -((z * (x * z)) - 12.0f);
    }
    correction =
        0.65f * (check->radius * inverse.value) - 1.0f;
    x = selected->x * correction;
    z = selected->z * correction;
    fatality_state.attacker_object->pos.value.x += x;
    fatality_state.attacker_object->pos.value.z += z;
    fatality_state.victim_object->pos.value.x += x;
    fatality_state.victim_object->pos.value.z += z;
    camera = camera_item.node;
    if (camera != 0 &&
        camera->hdr.instance != camera_item.instance) {
        camera = 0;
    }
    if (camera != 0) {
        camera->pos.x += x;
        camera->pos.z += z;
    }
    return 1;
}

/*
 * Soft ceiling: exact retail size, sqrt estimate, and strict range checks;
 * remaining differences are register allocation and FP scheduling.
 */
int fatality_check_distance(unsigned int action) {
    union {
        float value;
        unsigned int bits;
    } estimate;
    FatalityDistanceLimits* limits;
    float squared_distance;
    float distance;

    limits = plyr_pdata->status_data->fatality_limits;
    squared_distance = xz_distance_between_players();
    distance = 0.0f;
    if (squared_distance > 0.0f) {
        estimate.value = squared_distance;
        estimate.bits =
            ((unsigned int)GXMathSqrtTable[
                 (estimate.bits >> 10) & 0x3FFE] << 8) |
            ((((estimate.bits & 0x7F800000) + 0x3F800000) >> 1) &
             0x7F800000);
        distance = 0.5f *
            (estimate.value *
             (3.0f -
              (estimate.value * estimate.value) / squared_distance));
    }
    if (action == (unsigned int)do_my_fatality) {
        return distance > limits->primary_min &&
               distance < limits->primary_max;
    }
    if (action == (unsigned int)do_my_2nd_fatality) {
        return distance > limits->secondary_min &&
               distance < limits->secondary_max;
    }
    return 1;
}

void fatality_release_other_player(void) {
    release_other_player();
    fatality_state.animation->movement_scale = 1.0f;
    his_obj->flags_09 &= ~0x80;
    his_obj->flags_09 &= ~0x10;
    his_obj->flags_09 &= ~0x02;
    his_obj->flags_09 &= ~0x20;
    his_obj->flags_09 &= ~0x08;
    plyr_obj->flags_09 &= ~0x80;
    plyr_obj->flags_09 &= ~0x10;
    plyr_obj->flags_09 &= ~0x02;
    plyr_obj->flags_09 &= ~0x20;
    plyr_obj->flags_09 &= ~0x08;
}

void set_victim_v3_units_away(float x, float z) {
    Vec offset;
    float original_y;

    original_y = his_obj->pos.value.y;
    offset.x = x;
    offset.y = 0.0f;
    offset.z = z;
    v3_x_mat_add_v3(&his_obj->pos.value, &offset, plyr_obj->field_24,
                    &plyr_obj->pos.value);
    his_obj->pos.value.y = original_y;
}

void fade_fatality_screen(void) {
    float far_clip;
    float step;

    background_color.red = 0;
    background_color.green = 0;
    background_color.blue = 0;
    background_color.alpha = 0;
    g_game_info.sky = 0;
    fog_on = 1;
    fog_distance = 0.0f;
    fog_type = 1;
    fog_color_real[0] = 0.0f;
    fog_color_real[1] = 0.0f;
    fog_color_real[2] = 0.0f;
    fog_color_real[3] = 0.0f;

    far_clip = Camera->farPlane;
    step = (far_clip - 10.0f) / 30.0f;
    while (far_clip > 10.0f) {
        far_clip -= step;
        if (far_clip < 10.0f) {
            far_clip = 10.0f;
        }
        RwCameraSetFarClipPlane(Camera, far_clip);
        FATALITY_SLEEP(1.0f);
    }
}

/*
 * Soft ceiling: exact retail size and instruction-equivalent control flow;
 * remaining differences are register allocation and instruction scheduling.
 */
void run_fatality_sequence(
    unsigned int main_script, unsigned int victim_script) {
    FatalityBgndScriptView* section_script;
    FatalityBgndScriptView* active_script;
    CmdScript* victim_cmdscript;

    plyr_obj->flags_09 &= ~0x08;
    his_obj->flags_09 &= ~0x08;
    plyr_obj->flags_09 &= ~0x02;
    his_obj->flags_09 &= ~0x02;
    plyr_obj->flags_09 &= ~0x20;
    his_obj->flags_09 &= ~0x20;
    plyr_obj->flags_09 &= ~0x10;
    his_obj->flags_09 &= ~0x10;
    plyr_obj->hide_flags &= ~0x80;
    his_obj->hide_flags &= ~0x80;
    plyr_pdata->blocking_disabled = 1;
    his_pdata->blocking_disabled = 1;

    his_obj->ang.y = plyr_obj->ang.y + 3.1415927f;
    two_player_animation_blend(
        plyr_pdata->fatality_animation, 0, fatality_state.mirror_camera,
        plyr_obj, 0, 1.0f, 0.0f, 3.1415927f);
    if (is_my_chest_to_screen() == 0) {
        plyr_anim_pdata->flags ^= 0x08;
        plyr_obj->hide_flags ^= 0x40;
    }

    /* Background data tables share this script-function prefix at +0x08. */
    section_script = (FatalityBgndScriptView*)g_game_info.section->misc;
    active_script = (FatalityBgndScriptView*)g_game_info.misc;
    if (section_script != 0 && section_script->function != 0 &&
        active_script->function != 0) {
        cmdscript_set_parameters(active_cmdscript, 1, &fatality_state);
        cmdscript_setup_execution(
            g_game_info.cmdscript, active_script->function);
        cmdscript_execute(g_game_info.cmdscript);
    }

    head_tracking_off();
    his_obj->flags_09 |= 0x80;
    his_obj->flags_09 |= 0x40;
    if (is_pal_mode() != 0) {
        fatality_anim_script = plyr_pdata->fatality_camera_pal;
    } else {
        fatality_anim_script = plyr_pdata->fatality_camera_ntsc;
    }
    if (fatality_anim_script != 0) {
        camera_init_animation(
            (void*)fatality_anim_script, p_animate_and_freeze);
        _create_mkproc_generic_tinystack(
            0x6000, 0x1F, p_fatality_cam, 0, 0);
    }

    FATALITY_SLEEP(1.0f);
    release_other_player();
    fatality_state.animation->movement_scale = 1.0f;
    his_obj->flags_09 &= ~0x80;
    his_obj->flags_09 &= ~0x10;
    his_obj->flags_09 &= ~0x02;
    his_obj->flags_09 &= ~0x20;
    his_obj->flags_09 &= ~0x08;
    plyr_obj->flags_09 &= ~0x80;
    plyr_obj->flags_09 &= ~0x10;
    plyr_obj->flags_09 &= ~0x02;
    plyr_obj->flags_09 &= ~0x20;
    plyr_obj->flags_09 &= ~0x08;
    xfer_proc(fatality_state.attacker_proc, p_animate);
    plyr_obj->flags_09 |= 0x80;
    his_obj->flags_09 |= 0x80;

    if (victim_script != 0) {
        victim_cmdscript = get_cmdscript_for_proc(fatality_state.victim_proc);
        cmdscript_set_parameters(victim_cmdscript, 1, &fatality_state);
        victim_cmdscript->unk28 = victim_script;
        xfer_player_proc(
            fatality_state.victim_proc,
            (MkProcEntryFn)call_fatality_script_function);
        if (fatality_state.player_info == &g_game_info.plyr0) {
            FATALITY_SLEEP(1.0f);
        }
    }

    cmdscript_set_parameters(active_cmdscript, 1, &fatality_state);
    cmdscript_setup_execution(plyr_pdata->cmo, main_script);
    cmdscript_execute(plyr_pdata->cmo);
}

/*
 * Soft ceiling: exact player/process capture, instance validation, state reset,
 * mirror shutdown, auxiliary-object hiding, and fatality screen setup.
 */
static int init_fatality_world(void) {
    MkObj* object;

    if (plyr_pdata->character_id == 2 ||
        plyr_pdata->character_id == 0xD) {
        return 0;
    }
    if (g_game_info.field_200 != 3) {
        init_ground_move();
        plyr_anim_pdata->step = 1.0f;
        glitch_to_stance(plyr_anim_pdata, 0.5f);
        xfer_camera(p_idle_camera, 1);
    } else {
        face_opponent_now();
        if (is_my_chest_to_screen() == 0) {
            plyr_obj->hide_flags ^= 0x40;
        }
    }
    if (is_local_plyr() != 0) {
        while (is_weapon_style(plyr_pdata->fighter_definition) != 0) {
            advance_active_moveset(plyr_pdata);
        }
    }

    fatality_state.context = plyr_pdata->plyr_info;
    fatality_state.attacker_object = plyr_obj;
    fatality_state.process = aproc;
    fatality_state.animation_process = plyr_anim_proc;
    fatality_state.animation_pdata = plyr_anim_pdata;
    fatality_state.player = plyr_pdata;
    fatality_state.player_info =
        plyr_pdata->his_plyr_pdata->plyr_info;
    fatality_state.victim_object = his_obj;
    fatality_state.victim_proc = plyr_pdata->player_proc;
    if (fatality_state.victim_proc != 0 &&
        fatality_state.victim_proc->instance !=
            plyr_pdata->player_proc_instance) {
        fatality_state.victim_proc = 0;
    }
    fatality_state.attacker_proc = his_pdata->anim_proc;
    if (fatality_state.attacker_proc != 0 &&
        fatality_state.attacker_proc->instance !=
            his_pdata->anim_proc_instance) {
        fatality_state.attacker_proc = 0;
    }
    fatality_state.animation = (FatalityAnimationView*)pdata_of_proc(
        fatality_state.attacker_proc);
    fatality_state.opponent = plyr_pdata->his_plyr_pdata;
    memset(fatality_state.range34.reset34, 0, 10);
    memset(fatality_state.reset84, 0, 10);
    memset(fatality_state.resetAC, 0, 10);

    if (fatality_state.victim_proc == 0 ||
        fatality_state.attacker_proc == 0 ||
        fatality_state.animation == 0) {
        return 0;
    }
    push_game_state(0xF);
    f_fatality_finished = 0;
    plyr_turn_off_mirrorguy(fatality_state.player_info);
    plyr_turn_off_shadowbox(fatality_state.player_info);

    object = fatality_state.opponent->aux_weapon_latch.obj;
    if (object != 0 &&
        object->hdr.instance !=
            fatality_state.opponent->aux_weapon_latch.instance) {
        object = 0;
    }
    if (object != 0) {
        object->hide_flags |= 0x20;
    }
    object = fatality_state.opponent->mirror_obj.obj;
    if (object != 0 &&
        object->hdr.instance !=
            fatality_state.opponent->mirror_obj.instance) {
        object = 0;
    }
    if (object != 0) {
        object->hide_flags |= 0x20;
    }
    start_gore2_update();
    setup_screen_for_fatality();
    return 1;
}

/*
 * Soft ceiling: the retail state transition, animation-slot ownership,
 * cleanup sequence, script sentinels, and terminal jump-sleep are preserved.
 */
float start_suicide(void) {
    FatalityBgndScriptView* section_script;
    FatalityBgndScriptView* active_script;
    FatalityDefinition* definition;
    unsigned int camera_script;

    g_game_info.field_200 = 3;
    if (init_fatality_world() == 0) {
        g_game_info.field_200 = 0;
    } else {
        fatality_fade_and_cleanup();
        definition = plyr_pdata->status_data->fatality_definition;
        if (definition->suicide_script != 0 &&
            definition->suicide_script != 0xFFFEFFFF) {
            if (is_my_chest_to_screen() == 0) {
                plyr_anim_pdata->flags ^= 0x08;
                plyr_obj->hide_flags ^= 0x40;
            }
            section_script =
                (FatalityBgndScriptView*)g_game_info.section->misc;
            active_script = (FatalityBgndScriptView*)g_game_info.misc;
            if (section_script != 0 && section_script->function != 0 &&
                active_script->function != 0) {
                cmdscript_set_parameters(
                    active_cmdscript, 1, &fatality_state);
                cmdscript_setup_execution(
                    g_game_info.cmdscript, active_script->function);
                cmdscript_execute(g_game_info.cmdscript);
            }
            head_tracking_off();
            fatality_state.attacker_object->flags_09 &= ~0x20;
            fatality_state.attacker_object->flags_09 &= ~0x10;
            fatality_state.attacker_object->flags_0B &= ~0x40;
            plyr_turn_off_mirrorguy(fatality_state.context);
            plyr_turn_off_shadowbox(fatality_state.context);

            if (plyr_pdata->character_id == 0x1B) {
                if (plyr_pdata->sidekick_active == 0) {
                    camera_script = is_pal_mode() != 0
                        ? plyr_pdata->suicide_camera_main_pal
                        : plyr_pdata->suicide_camera_main_ntsc;
                } else {
                    camera_script = is_pal_mode() != 0
                        ? plyr_pdata->suicide_camera_sidekick_pal
                        : plyr_pdata->suicide_camera_sidekick_ntsc;
                }
            } else {
                camera_script = is_pal_mode() != 0
                    ? plyr_pdata->fatality_camera_pal
                    : plyr_pdata->fatality_camera_ntsc;
            }
            fatality_anim_script = camera_script;
            if (camera_script != 0) {
                camera_init_animation(
                    (void*)camera_script, p_animate_and_freeze);
                _create_mkproc_generic_tinystack(
                    0x6000, 0x1F, p_fatality_cam, 0, 0);
            }
            FATALITY_SLEEP(1.0f);
            fatality_state.player_info = fatality_state.context;
            cmdscript_set_parameters(
                active_cmdscript, 1, &fatality_state);
            if (plyr_pdata->character_id == 0x1B) {
                if (plyr_pdata->sidekick_active == 0) {
                    cmdscript_setup_execution(
                        plyr_pdata->cmo, definition->suicide_script);
                    cmdscript_execute(plyr_pdata->cmo);
                } else {
                    cmdscript_setup_execution(
                        plyr_pdata->cmo,
                        definition->suicide_sidekick_script);
                    cmdscript_execute(plyr_pdata->cmo);
                }
            } else {
                cmdscript_setup_execution(
                    plyr_pdata->cmo, definition->suicide_script);
                cmdscript_execute(plyr_pdata->cmo);
            }
        }
    }
    ((FatalityProcVtable*)aproc->vtbl)->jump_sleep(
        end_of_fatality, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: the secondary section, script validation, sidekick gate, and
 * shared retail teardown are recovered; residue is compiler inlining order.
 */
float start_2nd_fatality(void) {
    FatalityDefinition* definition;

    g_game_info.field_200 = 2;
    if (init_fatality_world() == 0) {
        g_game_info.field_200 = 0;
    } else {
        definition = plyr_pdata->status_data->fatality_definition;
        fatality_load_animation_section(definition->secondary_section);
        fatality_fade_and_cleanup();
        if (definition->secondary_script != 0 &&
            definition->secondary_script != 0xFFFEFFFF &&
            definition->secondary_victim_script != 0xFFFEFFFF &&
            (plyr_pdata->character_id != 0x1B ||
             plyr_pdata->sidekick_active != 0)) {
            run_fatality_sequence(
                definition->secondary_script,
                definition->secondary_victim_script);
        }
    }
    ((FatalityProcVtable*)aproc->vtbl)->jump_sleep(
        end_of_fatality, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: the primary section, script validation, sidekick gate, and
 * shared retail teardown are recovered; residue is compiler inlining order.
 */
float start_fatality(void) {
    FatalityDefinition* definition;

    g_game_info.field_200 = 1;
    if (init_fatality_world() == 0) {
        g_game_info.field_200 = 0;
    } else {
        definition = plyr_pdata->status_data->fatality_definition;
        fatality_load_animation_section(definition->primary_section);
        fatality_fade_and_cleanup();
        if (definition->primary_script != 0 &&
            definition->primary_script != 0xFFFEFFFF &&
            definition->primary_victim_script != 0xFFFEFFFF &&
            (plyr_pdata->character_id != 0x1B ||
             plyr_pdata->sidekick_active == 0)) {
            run_fatality_sequence(
                definition->primary_script,
                definition->primary_victim_script);
        }
    }
    ((FatalityProcVtable*)aproc->vtbl)->jump_sleep(
        end_of_fatality, 0.0f);
    return 0.0f;
}
