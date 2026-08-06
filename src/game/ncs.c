#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/image.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pebble.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/asset.h"
#include "runtime/section.h"
#include "runtime/cam.h"
#include "game/game_info.h"
#include "platform/display.h"
#include "platform/main.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "rw/rtquat.h"
#include "runtime/cstring.h"

typedef struct NcsDestroyable NcsDestroyable;
typedef struct SpearProcPdata SpearProcPdata;
typedef struct PfxRenderView PfxRenderView;
typedef struct LightDef LightDef;
typedef int (*NcsDestroyFn)(NcsDestroyable* object);

extern LightDef* pbl_gore2_lights[3];
extern MkPtr* gore2_light_list;
extern int blood_type_list[12];

static void trigger_blood_glops(
    PlyrPdata* player, int bone, MkObj* source, int blood_type);
MkObj* obj_sever_limb(
    MkObj* object, int limb, Vec* limb_velocities, int include_children);

typedef struct NcsProcVtable {
    void* reserved[6];
    void (*sleep)(struct NcsProcVtable* vtbl);
    void* reserved_after_sleep[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
} NcsProcVtable;

extern int f_fatality_was_done;
extern PlyrPdata* his_pdata;
extern MkObj* his_obj;

typedef struct NcsDestroyVtable {
    void* reserved[4];
    NcsDestroyFn destroy;
} NcsDestroyVtable;

struct NcsDestroyable {
    NcsDestroyVtable* vtbl;
    unsigned int instance;
};

typedef struct NcsBoneMatcher {
    NcsDestroyVtable* vtbl;
    unsigned int instance;
    unsigned char flags;         /* +0x08 */
    char pad09[7];
    MkObj* parent;               /* +0x10 */
    unsigned int parent_instance; /* +0x14 */
    char pad18[0x2C];
    float blend;                 /* +0x44 */
} NcsBoneMatcher;

typedef struct NcsLimbAttachPdata {
    MkHdr hdr;
    PlyrInfo* player;       /* +0x08 */
    FighterMirror* fighter; /* +0x0C */
    void* limbset;          /* +0x10 */
    MkObj* owner;           /* +0x14 */
    unsigned int owner_instance;
    MkObj* target;          /* +0x1C */
    unsigned int target_instance;
    int limb;               /* +0x24 */
    int target_bone;
    int owner_bone;
    Vec offset;             /* +0x30 */
    Vec rotation;           /* +0x3C */
    int expire_tick;        /* +0x48 */
} NcsLimbAttachPdata; /* 0x4C */

typedef struct NcsLimbMotion {
    char pad00[0x14];
    unsigned int severed_mask; /* +0x14 */
    int field_18;
    char pad1C[0x84];
    unsigned int ground_mask;  /* +0xA0 */
    float ground_height[15];   /* +0xA4 */
    int ground_value[15];      /* +0xE0 */
    float field_11C;
    char pad120[0x18];
    float field_138;
} NcsLimbMotion;

typedef struct NcsLimbUpdatePdata {
    MkHdr hdr;
    PlyrInfo* player;       /* +0x08 */
    FighterMirror* fighter; /* +0x0C */
    void* limbset;          /* +0x10 */
    unsigned int severed_mask; /* +0x14 */
    int field_18;
    int expire_tick;        /* +0x1C */
    unsigned int x_collision_mask; /* +0x20 */
    unsigned int z_collision_mask; /* +0x24 */
    float x_collision_limit[15];   /* +0x28 */
    float z_collision_limit[15];   /* +0x64 */
    unsigned int ground_mask; /* +0xA0 */
    float ground_height[15];  /* +0xA4 */
    int ground_value[15];     /* +0xE0 */
    float vertical_bounce_scale; /* +0x11C */
    float horizontal_bounce_scale; /* +0x120 */
    float bounce_gravity;          /* +0x124 */
    unsigned int gravity_trigger_mask; /* +0x128 */
    unsigned int first_bounce_mask;    /* +0x12C */
    unsigned int damp_bounce_mask;     /* +0x130 */
    unsigned int slide_mask;           /* +0x134 */
    float slide_end_coefficient;       /* +0x138 */
    float slide_deceleration;          /* +0x13C */
} NcsLimbUpdatePdata; /* 0x140 */

typedef struct NcsLimbSet {
    char pad00[0x780];
    unsigned int active_mask;
} NcsLimbSet;

struct SpearProcPdata {
    MkHdr hdr;
    MkProc* player_proc; /* +0x08 */
    unsigned int player_proc_instance; /* +0x0C */
    PlyrPdata* opponent_pdata; /* +0x10 */
    MkObj* opponent_object; /* +0x14 */
    unsigned int flags; /* +0x18 */
    PlyrPdata* owner; /* +0x1C */
    MkObj* spear_object; /* +0x20 */
    unsigned int spear_object_instance; /* +0x24 */
    NcsBoneMatcher* bonematcher; /* +0x28 */
    MkHdr* bound_object; /* +0x2C */
    unsigned int bound_object_instance; /* +0x30 */
    int field_34;
    int blocked_ticks; /* +0x38 */
    struct NcsSpearEffect* effect; /* +0x3C */
    unsigned int effect_instance; /* +0x40 */
    int field_44;
};

typedef struct NcsSpearEffect {
    MkHdr hdr;
    unsigned char object_flags; /* +0x08 */
    char pad09[0x1F];
    float field_28;
    char pad2C[0x14];
    PfxVm vm; /* +0x40 */
    char pad18C[4];
    unsigned char render_flags; /* +0x190 */
    char pad191[0x67];
    float field_1F8;
    char pad1FC[0x8C];
    int active; /* +0x288 */
    int bone;   /* +0x28C */
    int field_290;
    char pad294[4];
    float field_298;
    float field_29C;
    float field_2A0;
    float field_2A4;
    float field_2A8;
    char pad2AC[0x0C];
    SpearProcPdata* spear_pdata; /* +0x2B8 */
    unsigned int spear_pdata_instance; /* +0x2BC */
} NcsSpearEffect;

typedef union SpearProcPdataRef {
    MkHdr* hdr;
    SpearProcPdata* spear;
} SpearProcPdataRef;

typedef struct PrisonGrabPdata {
    MkHdr hdr;
    MkObj* object; /* +0x08 */
    unsigned int object_instance;
    float target_x;
    float pad14;
    float target_z;
    float current_x;
    float current_y;
    float current_z;
    int done; /* +0x28 */
    float target_angle;
    int aligned;
} PrisonGrabPdata;

typedef union NcsFloatBits {
    float f;
    unsigned int u;
} NcsFloatBits;

typedef struct NcsSpearObjectView {
    char pad00[0x5C];
    void* data_table;
    int active;
} NcsSpearObjectView;

typedef struct NcsSpearAimObject {
    char pad00[0xA0];
    Vec pos;
    char padAC[0x28];
    float facing_angle;
} NcsSpearAimObject;

typedef struct NcsSnapshotRaster {
    char pad00[0x28];
    int width;
    int height;
} NcsSnapshotRaster;

typedef struct NcsLimbCollection {
    char pad00[0x144];
    MkObjLatch limbs[1];
} NcsLimbCollection;

typedef struct NcsLimbOwner {
    char pad00[0x58];
    NcsLimbCollection* collection;
    MkObj* object;
} NcsLimbOwner;

typedef struct NcsLimbSlide {
    char pad00[0x134];
    unsigned int update_flags;
    char pad138[4];
    float slide_end_coefficient;
} NcsLimbSlide;

typedef struct NcsPlayerObjectRef {
    char pad00[0x5C];
    MkObj* object;
} NcsPlayerObjectRef;

typedef struct PfxPlayerBankOwner {
    void* reserved;
    int player_index; /* +0x04 */
} PfxPlayerBankOwner;

typedef struct NcsKonquestCharacterPdata {
    MkHdr hdr;
    char pad08[4];
    MkObj* characters[8]; /* +0x0C, camera indices 0x0B..0x12 */
} NcsKonquestCharacterPdata;

typedef struct NcsBloodPoolSource {
    char pad00[0x58];
    MkObj* splat_owner;
    MkObj* bone_owner;
} NcsBloodPoolSource;

typedef struct Gore2ObjectType {
    unsigned int object_id;
    int particle_count;
    float scale;
} Gore2ObjectType; /* 0x0C */

typedef struct NcsGroundCollisionMap {
    int bone_id;
    int field_04;
    int field_08;
    int field_0C;
    float radius;
    int terminator;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
} NcsGroundCollisionMap; /* 0x28 */

#define NCS_GROUND_COLLISION_MAP(bone_, radius_) \
    { (bone_), 0, 0, 0, (radius_), -1, 0, 0, 0, 0 }

NcsGroundCollisionMap LID_HEAD_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x10, 0.07f);
NcsGroundCollisionMap LID_HAND_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x25, 0.04f);
NcsGroundCollisionMap LID_FOREARM_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x17, 0.07f);
NcsGroundCollisionMap LID_ARM_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x13, 0.07f);
NcsGroundCollisionMap LID_HAND_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x24, 0.04f);
NcsGroundCollisionMap LID_FOREARM_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x16, 0.07f);
NcsGroundCollisionMap LID_ARM_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x12, 0.07f);
NcsGroundCollisionMap LID_FOOT_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x0B, 0.05f);
NcsGroundCollisionMap LID_CALF_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x08, 0.07f);
NcsGroundCollisionMap LID_THIGH_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x05, 0.07f);
NcsGroundCollisionMap LID_FOOT_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x0A, 0.05f);
NcsGroundCollisionMap LID_CALF_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x07, 0.07f);
NcsGroundCollisionMap LID_THIGH_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x04, 0.07f);
NcsGroundCollisionMap LID_PELVIS_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x00, 0.14f);
NcsGroundCollisionMap LID_TORSO_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x03, 0.12f);

NcsGroundCollisionMap* limbbid_bid_map[15] = {
    &LID_HEAD_ground_collision_map,
    &LID_HAND_RIGHT_ground_collision_map,
    &LID_FOREARM_RIGHT_ground_collision_map,
    &LID_ARM_RIGHT_ground_collision_map,
    &LID_HAND_LEFT_ground_collision_map,
    &LID_FOREARM_LEFT_ground_collision_map,
    &LID_ARM_LEFT_ground_collision_map,
    &LID_FOOT_RIGHT_ground_collision_map,
    &LID_CALF_RIGHT_ground_collision_map,
    &LID_THIGH_RIGHT_ground_collision_map,
    &LID_FOOT_LEFT_ground_collision_map,
    &LID_CALF_LEFT_ground_collision_map,
    &LID_THIGH_LEFT_ground_collision_map,
    &LID_PELVIS_ground_collision_map,
    &LID_TORSO_ground_collision_map,
};

const char* mkpfx_ncs_blood_type_map_array[15] = {
    "bltrsh", "bltrlg", "bltrin", "bdexsw", "bdexfs",
    "bdgp", "bdgp", "bdgp", "bdgp", "bdgpdn",
    "bdgpup", "bdgpzz", "bdsp_4x4", "bdsp_4x4lg", 0,
};

static const char* mkpfx_ncs_sweat_type_map_array[4] = {
    "swtrsh", "swexsw", "swexfs", 0,
};

const char* mkpfx_ncs_decal_array[7] = {
    "blsplat", "blsplat2", "blpuddle", "blpuddle2",
    "blsmash", "blfoot", 0,
};

int mkpfx_type_to_blood_level_map[15] = {
    6, 6, 6, 9, 9, 7, 7, 7, 7, 7, 7, 7, 8, 8, 0,
};

static Gore2ObjectType pbl_gore2_obj_list[10] = {
    {0x00020007, 15, 0.1f},
    {0x00020008, 2, 0.1f},
    {0x00020009, 3, 0.1f},
    {0x0002000A, 2, 0.1f},
    {0x0002000B, 5, 0.1f},
    {0x0002000C, 10, 0.1f},
    {0x0002000D, 15, 0.1f},
    {0x0002000E, 2, 0.1f},
    {0x0002000F, 3, 0.1f},
    {0x00020010, 15, 0.1f},
};

/* Retail stores variable-size light records with this common prefix. */
typedef struct NcsAmbientLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
} NcsAmbientLightDef; /* 0x1C */

typedef struct NcsDirectLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
    float field_1C;
    float field_20;
    float field_24;
} NcsDirectLightDef; /* 0x28 */

float p_track_cam_ang_y_light(void);

NcsAmbientLightDef pbl_gore2_ambient_light = {
    1, 0, 3, {0.1f, 0.1f, 0.1f, 0.0f},
};

NcsDirectLightDef pbl_gore2_direct_light = {
    3, p_track_cam_ang_y_light, 1,
    {0.75f, 0.75f, 0.75f, 1.0f},
    0.6f, 2.89f, 0.0f,
};

LightDef* pbl_gore2_lights[3] = {
    (LightDef*)&pbl_gore2_ambient_light,
    (LightDef*)&pbl_gore2_direct_light,
    0,
};

typedef union Gore2Flags {
    unsigned int word;
    struct {
        signed char settled : 1;
        signed char has_rotation : 1;
        signed char has_scale : 1;
        signed char has_translation : 1;
        signed char attached : 1;
        signed char pad_high : 3;
        unsigned char pad[3];
    } bits;
} Gore2Flags;

typedef struct Gore2Particle {
    Gore2Flags flags;
    Vec rotation;       /* +0x04 */
    Vec scale;          /* +0x10 */
    Vec translation;    /* +0x1C -- offset while attached, velocity while free */
    float vertical_acceleration; /* +0x28 */
    int bounce_count;            /* +0x2C */
    float bounce_scale;          /* +0x30 */
    FighterObjectRef owner; /* +0x34 */
    int bone;           /* +0x3C */
    FighterMirror* decal_owner; /* +0x40 */
} Gore2Particle; /* 0x44 */

typedef struct Gore2Pool {
    MkHdr hdr;
    Pebble* pebbles; /* +0x08 */
    char pad0C[4];
    int capacity; /* +0x10 */
    PebbleRenderData* render_data; /* +0x14 */
    Gore2Particle* particles;       /* +0x18 */
    PebbleFlags* states;             /* +0x1C */
} Gore2Pool;

typedef struct Gore2UpdatePdata {
    MkHdr hdr;
    Gore2Pool* pools[10]; /* +0x08 */
    int next_particle[10]; /* +0x30 */
} Gore2UpdatePdata;

static MkObj* sc_spear_obj;
static SpearProcPdata* pdata_sc_spear;
Gore2UpdatePdata* mkpdata_pbl_gore2_update;
int cur_grab_check;
int cur_zone_check;
int gap_08_80510A84_sbss;

typedef struct NcsGroundCollisionWatchPdata {
    MkHdr hdr;
    PlyrPdata* blood_owner;     /* +0x08 */
    FighterObjectRef objects[3]; /* +0x0C */
    unsigned int emitters[3];    /* +0x24 */
} NcsGroundCollisionWatchPdata; /* 0x30 */

typedef struct NcsCameraWallRegion {
    int type;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    char pad14[0x0C];
    const int* hide_ids;
    const int* show_ids;
    const int* alpha_ids;
} NcsCameraWallRegion; /* 0x2C */

typedef struct NcsCameraWallPdata {
    MkHdr hdr;
    NcsCameraWallRegion* regions;
    MkObj* characters[8];
    int active_region;
    int special_alpha_initialized;
} NcsCameraWallPdata; /* 0x34 */

extern int screen_width;
extern int screen_height;
extern MkObj* plyr_obj;
extern MkObj* his_obj;

void plyr_aux_weapon_grab(PlyrPdata* player, MkObj* item);
unsigned int create_mkproc_anim2(
    int proc_id, MkProcEntryFn entry, AnimPdata** pdata_out);
void insert_ground_me_mkobj(void* object);
void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, void* animation);
void set_anim_script(AnimPdata* animation, void* script, int transition);
void set_my_state(int state);
unsigned int fx_by_owner(const char* name, unsigned int owner);
void fx_transfer(unsigned int handle, unsigned int owner);
void fx_reset_emit(unsigned int handle);
void start_decal_emitter_watcher(void);
unsigned int fx_next_emitter(unsigned int handle);
void fx_resume_emit(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
int emitter_id_from_handle(unsigned int handle);
void advance_anim(AnimPdata* animation);
void pose_anim(AnimPdata* animation, int update_object);
int am_i_on_the_left2(MkObj* opponent, MkObj* me);
int get_bid_with_flip(MkObj* object, unsigned int bone_id);
int is_plyr_airborn(MkObj* object, PlyrPdata* player);
MkObj* get_mkobj_frame(int type, void* frame);
void insert_particle_mkobj(MkObj* object);
void mkobj_get_matrix_at(MkObj* object, Vec* out);
void mkobj_get_matrix_right(MkObj* object, Vec* out);
void pfx_set_texture(PfxRenderView* pfx, RwTexture* texture);
void* limb_sever_find_limbset(PlyrInfo* player);
int get_blood_level(void);
float frand(float max);
int simple_3d_projectile_collision(
    const Vec* previous, const Vec* current, const Vec* target,
    int mode, float radius, float distance, float threshold);
void trial_state_collision_check(int collision_result, int player);
void pz_fighter_reaction_xfer_him(int reaction);
int reaction_xfer_him(int reaction, float rate, int strength);
NcsBoneMatcher* start_bone_matcher(
    float blend_ticks, MkObj* parent, int parent_bone,
    MkObj* child, int child_bone);
void plyr_aux_weapon_release(PlyrPdata* player);
void snd_req(int sound_id);
void* pfx_get_field(PfxVm* vm, int emitter_index, int field);
void atomic_set_transl_flag(void* atomic);
void hide_atomic(void* atomic);
void unhide_atomic(void* atomic);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void sobj_set_color_for_all_materials(void* sobj, int* color);
void random_hit(int hit);
void calc_bone_world_mat(MkObj* object, int bone);
MKMATRIX* force_calc_bone_world_mat(MkObj* object, int bone);
void init_plyr_severed_limb_list(PlyrInfo* player);
void obj_set_ang_vel(MkObj* object, const Vec* velocity);
void obj_set_pos_vel(MkObj* object, const Vec* velocity);
MkObj* load_model_from_slot(int handle, unsigned int art_oid, int heap_id);
MkObj* load_light(LightDef* definition, MkPtr** list, MkObj* parent);
void spawn_decal_emitter(
    const char* name, FighterMirror* owner, const Vec* position,
    const MKMATRIX* orientation, float angle);
MKMATRIX* mkobj_get_matrix(MkObj* object);
void limb_sever_show_z_meat_chunks(
    MkObj* object, int limb, int include_children);
void obj_set_bone_calc_world_mat_flag(MkObj* object, int bone);
MkProc* fire_sc_spear(
    PlyrPdata* player, const Vec* velocity, int field_34,
    int flag_40, MkHdr* bound_object, int flag_20);
float p_sc_spear_kill(void);
float p_sc_spear_blocked(void);
float p_sc_spear_retract(void);
float p_sc_spear_retract_victory(void);
static float p_pfx_sc_spear(void);
static float p_sc_spear2(void);
static float p_sc_spear2_victory(void);
static float p_sc_spear2_getup(void);
static float p_sc_spear3_pre(void);
static float p_sc_spear3(void);
static float p_sc_spear4(void);
static float p_sc_spear4_victory(void);
static float p_sc_spear4_getup(void);
static float p_camera_wall_show_hide_alpha(void);
static float p_gore2_update(void);
static float p_limb_sever_attach(void);
static float p_limb_sever_update(void);
NcsLimbUpdatePdata* limb_sever_find_existing_update_proc(
    PlyrInfo* player, int limb, int proc_id);
MkObj* limb_sever_set_motion(
    MkObj* owner, int limb, const Vec* velocity,
    NcsLimbMotion* motion, int enable_ground,
    int ground_value, int field_18, int include_children,
    float gravity, float ground_offset, float field_11C);
void limb_sever_explode_apart(PlyrInfo* player);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void spawn_bld_splat(
    const char* name, void* owner, const Vec* position);
void obj_create_sobjs(MkObj* object);
void sobj_set_priority(void* object, int priority);
MslSoundHandle plyr_snd_req(int sound);
void random_voice(int sound);

static inline NcsSpearEffect* ncs_get_spear_effect(void) {
    NcsSpearEffect* effect;

    effect = pdata_sc_spear->effect;
    if (effect != 0 &&
        effect->hdr.instance != pdata_sc_spear->effect_instance) {
        effect = 0;
    }
    return effect;
}

static float p_mkpfx_fadingrun(void);
float p_sc_spear1(void);
void sc_spear_prewake(void);
void sc_spear_postsleep(void);
static float p_prison_grab(void);

void start_mkpfx_FadeSnapShot(void) {
    NcsSnapshotRaster* raster;

    if (fading_screen.fade_obj != 0) {
        DeleteCameraSnapShot();
        destroy_mkprocs_pid(0x6007);
    }

    fading_screen.fade_active = 1;
    TakeCameraSnapShot();
    fading_screen.fade_active = 0;
    if (fading_screen.snapshotTex == 0) {
        DeleteCameraSnapShot();
        return;
    }

    fading_screen.alpha = 255.0f;
    raster = (NcsSnapshotRaster*)fading_screen.snapshotTex->raster;
    raster->width = screen_width;
    raster->height = screen_height;
    fading_screen.fade_obj = load_2d_pfxobj_with_texture(
        0x6001, fading_screen.snapshotTex, 0, 0x12);
    if (fading_screen.fade_obj == 0) {
        DeleteCameraSnapShot();
        return;
    }

    fading_screen.fade_obj->x = 0;
    fading_screen.fade_obj->y = 0;
    if (_create_mkproc_generic_nostack(
            0x6007, 0x2E, p_mkpfx_fadingrun, 0, 0) == 0) {
        DeleteCameraSnapShot();
    }
}

static float p_mkpfx_fadingrun(void) {
    fading_screen.alpha -= 4.0f * game_speed;
    if (fading_screen.alpha <= 0.0f) {
        DeleteCameraSnapShot();
        return -1.0f;
    }

    fading_screen.fade_obj->pfx2d->verts[0].a =
        (unsigned char)fading_screen.alpha;
    fading_screen.fade_obj->pfx2d->verts[1].a =
        (unsigned char)fading_screen.alpha;
    fading_screen.fade_obj->pfx2d->verts[2].a =
        (unsigned char)fading_screen.alpha;
    fading_screen.fade_obj->pfx2d->verts[3].a =
        (unsigned char)fading_screen.alpha;
    return 1.0f;
}

MkProc* start_scorpion_spear(int field_34) {
    NcsSpearAimObject* player;
    NcsSpearAimObject* opponent;
    Vec velocity;
    float facing_x;
    float facing_z;

    player = (NcsSpearAimObject*)plyr_obj;
    opponent = (NcsSpearAimObject*)his_obj;
    facing_x = gxMathSin(player->facing_angle);
    facing_z = gxMathCos(player->facing_angle);
    xz_unit_vector(&velocity, &player->pos, &opponent->pos);
    if (facing_x * velocity.x + facing_z * velocity.z < 0.0f) {
        velocity.x = facing_x;
        velocity.y = 0.0f;
        velocity.z = facing_z;
    }
    scale_v3(&velocity, &velocity, 0.15f);
    return fire_sc_spear(plyr_pdata, &velocity, field_34, 0, 0, 0);
}

MkProc* fire_spear_at_camera(PlyrPdata* player, unsigned int ticks) {
    CameraObj* camera;
    MkObj* weapon;
    MkObj* target;
    NcsSpearObjectView* weapon_view;
    SpearProcPdata* pdata;
    MkProc* proc;
    float inverse_ticks;

    camera = camera_item.node;
    if (camera != 0 && camera->instance != camera_item.instance) {
        camera = 0;
    }
    if (camera == 0) {
        return 0;
    }

    weapon = player->aux_weapon_latch.obj;
    if (weapon != 0 &&
        weapon->hdr.instance != player->aux_weapon_latch.instance) {
        weapon = 0;
    }
    if (weapon == 0) {
        return 0;
    }

    weapon_view = (NcsSpearObjectView*)weapon;
    if (weapon_view->active == 0) {
        weapon_view->active = 1;
        if (player->character_id == 0) {
            weapon_view->data_table = get_data_table(player->cmo, 0x19);
        }
        if (player->character_id == 0x1C) {
            weapon_view->data_table = get_data_table(player->cmo, 0x16);
        }
        if (player->character_id == 0x19 ||
            player->character_id == 0x1A) {
            weapon_view->data_table = get_data_table(player->cmo, 3);
        }
        plyr_aux_weapon_grab(player, weapon);
    }

    pdata = 0;
    proc = _create_mkproc_generic_nostack(
        0x5019, 2, p_sc_spear1, sizeof(SpearProcPdata),
        (MkHdr**)&pdata);
    if (proc == 0) {
        return 0;
    }

    zero_pdata_payload(sizeof(SpearProcPdata), &pdata->hdr);
    pdata->opponent_object = player->his_obj;
    pdata->opponent_pdata = player->his_plyr_pdata;
    pdata->player_proc = player->player_proc;
    pdata->player_proc_instance = player->player_proc_instance;
    pdata->field_34 = 0;
    inverse_ticks = -1.0f / (float)ticks;
    pdata->flags = 0x10;
    pdata->bonematcher = 0;
    pdata->bound_object = 0;
    pdata->bound_object_instance = 0;

    weapon->flags_08_bits.gravity_enabled = 0;
    target = player->plyr_info->slot.mirror_a;
    weapon->pos_vel.x = (target->pos.x - camera->pos_x) * inverse_ticks;
    weapon->pos_vel.y =
        ((target->pos.y - camera->pos_y) + 0.8f) * inverse_ticks;
    weapon->pos_vel.z = (target->pos.z - camera->pos_z) * inverse_ticks;
    proc->pre_destroy = sc_spear_prewake;
    proc->destroy_cb = sc_spear_postsleep;
    proc->sleep_ticks = 2.0f;
    pdata->owner = player;
    pdata->spear_object = weapon;
    pdata->spear_object_instance = weapon->hdr.instance;
    pdata->effect = 0;
    pdata->effect_instance = 0;
    pdata->field_44 = 0;
    return proc;
}

MkProc* fire_sc_spear(
    PlyrPdata* player, const Vec* velocity, int field_34,
    int flag_40, MkHdr* bound_object, int flag_20) {
    MkObj* weapon;
    NcsSpearObjectView* weapon_view;
    SpearProcPdata* pdata;
    MkProc* proc;

    weapon = player->aux_weapon_latch.obj;
    if (weapon != 0 &&
        weapon->hdr.instance != player->aux_weapon_latch.instance) {
        weapon = 0;
    }
    if (weapon == 0) {
        return 0;
    }

    weapon_view = (NcsSpearObjectView*)weapon;
    if (weapon_view->active == 0) {
        weapon_view->active = 1;
        if (player->character_id == 0) {
            weapon_view->data_table = get_data_table(player->cmo, 0x19);
        }
        if (player->character_id == 0x1C) {
            weapon_view->data_table = get_data_table(player->cmo, 0x16);
        }
        if (player->character_id == 0x19 ||
            player->character_id == 0x1A) {
            weapon_view->data_table = get_data_table(player->cmo, 3);
        }
        plyr_aux_weapon_grab(player, weapon);
    }

    pdata = 0;
    proc = _create_mkproc_generic_nostack(
        0x5019, 2, p_sc_spear1, sizeof(SpearProcPdata),
        (MkHdr**)&pdata);
    if (proc == 0) {
        return 0;
    }

    zero_pdata_payload(sizeof(SpearProcPdata), &pdata->hdr);
    pdata->opponent_object = player->his_obj;
    pdata->opponent_pdata = player->his_plyr_pdata;
    pdata->player_proc = player->player_proc;
    pdata->player_proc_instance = player->player_proc_instance;
    pdata->field_34 = field_34;
    pdata->flags = ((flag_40 << 6) & 0x40) |
                   ((flag_20 << 5) & 0x20);
    pdata->bonematcher = 0;
    if (bound_object != 0) {
        pdata->bound_object = bound_object;
        pdata->bound_object_instance = bound_object->instance;
    } else {
        pdata->bound_object = 0;
        pdata->bound_object_instance = 0;
    }

    weapon->flags_08_bits.gravity_enabled = 0;
    weapon->pos_vel = *velocity;
    proc->pre_destroy = sc_spear_prewake;
    proc->destroy_cb = sc_spear_postsleep;
    proc->sleep_ticks = 2.0f;
    pdata->owner = player;
    pdata->spear_object = weapon;
    pdata->spear_object_instance = weapon->hdr.instance;
    pdata->effect = 0;
    pdata->effect_instance = 0;
    pdata->field_44 = 0;
    return proc;
}

float p_sc_spear1(void) {
    PlyrPdata* owner;
    MkObj* target;
    NcsSpearEffect* effect;
    MkProc* effect_proc;

    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    owner = pdata_sc_spear->owner;
    plyr_aux_weapon_release(owner);
    target = owner->tracked_obj;
    if (target != 0 && target->hdr.instance != owner->tracked_obj_instance) {
        target = 0;
    }
    if (target == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    sc_spear_obj->ang.x = 0.0f;
    sc_spear_obj->ang.y = target->ang.y;
    sc_spear_obj->ang.z = 1.5707964f;
    sc_spear_obj->flags_08_bits.transform_dirty = 1;
    if ((pdata_sc_spear->flags & 0x20) != 0) {
        sc_spear_obj->ang.x = -0.9f;
    } else if ((pdata_sc_spear->flags & 0x10) != 0) {
        if (am_i_on_the_left2(
                owner->plyr_info->slot.mirror_a,
                owner->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
            sc_spear_obj->ang.y = target->ang.y - 1.5707964f;
        } else {
            sc_spear_obj->ang.y = target->ang.y + 1.5707964f;
        }
    }

    effect = 0;
    effect_proc = (MkProc*)pfx_create_raw_userdata(
        0, 0, 0x64, 2, 0, 0, 0x501A,
        p_pfx_sc_spear, (void**)&effect);
    if (effect_proc == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    mkproc_change_priority(effect_proc, 0x2D);
    pdata_sc_spear->effect = effect;
    pdata_sc_spear->effect_instance = effect->hdr.instance;
    pfx_bind_emitter_to_obj((MkPfx*)effect, sc_spear_obj, 0);
    effect->spear_pdata = pdata_sc_spear;
    effect->spear_pdata_instance = pdata_sc_spear->hdr.instance;
    if ((pdata_sc_spear->flags & 0x10) != 0) {
        effect->bone = 0x18;
        if (am_i_on_the_left2(
                owner->plyr_info->slot.mirror_a,
                owner->his_plyr_pdata->plyr_info->slot.mirror_a) == 0) {
            effect->bone = 0x19;
        }
    } else {
        effect->bone = (target->hide_flags & 0x40) != 0 ? 0x18 : 0x19;
    }

    {
        int art_section = get_shared_art_section_for_player(
            (SharedArtPlayer*)target);
        RwTexture* texture = load_named_tga_from_slot(art_section, "ROPE");

        pfx_set_texture((PfxRenderView*)&effect->vm, texture);
    }
    effect->render_flags |= 0x40;
    effect->field_1F8 = 0.1f;
    effect->object_flags |= 0x10;
    effect->field_298 = 1.0f;
    effect->field_29C = 0.0f;
    effect->field_2A0 = 0.975f;
    effect->field_2A4 = 0.96f;
    effect->field_2A8 = 0.0f;
    effect->active = 5;
    effect->field_290 = 1;
    effect->field_28 = -50.0f;
    snd_req(0x2CC);

    if ((pdata_sc_spear->flags & 0x20) != 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear2_getup, 1.0f);
    } else if ((pdata_sc_spear->flags & 0x10) != 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear2_victory, 1.0f);
    } else {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear2, 1.0f);
    }
    return 1.0f;
}

static float p_sc_spear2(void) {
    Vec spear_rotation = {0.0f, 3.1415927f, 0.0f};
    PlyrPdata* owner;
    MkObj* target;
    int collision;
    int outcome;

    owner = pdata_sc_spear->owner;
    target = owner->tracked_obj;
    if (target != 0 && target->hdr.instance != owner->tracked_obj_instance) {
        target = 0;
    }
    if (target == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    owner->saved_position_x = sc_spear_obj->pos.x;
    owner->saved_position_z = sc_spear_obj->pos.z;
    owner->duck_reaction_active = 1;
    collision = simple_3d_projectile_collision(
        &target->pos, &pdata_sc_spear->opponent_object->pos,
        &sc_spear_obj->pos, 0, 0.2f, 200.0f, 0.25f);
    outcome = 0;
    switch (collision) {
    case 0:
        trial_state_collision_check(
            1, target == g_game_info.plyr0.slot.mirror_a);
        if ((pdata_sc_spear->opponent_pdata->state & 0x100) != 0 ||
            pdata_sc_spear->opponent_pdata->state == 0x1222 ||
            (g_game_info.flags & 0x18) != 0) {
            outcome = 2;
        } else {
            NcsSpearEffect* effect = ncs_get_spear_effect();

            outcome = 1;
            if (effect != 0) {
                effect->field_2A0 = 0.75f;
                effect->field_2A8 = 0.005f;
            }
            if ((pdata_sc_spear->flags & 0x40) == 0) {
                if (mode_of_play == 6) {
                    pz_fighter_reaction_xfer_him(0x22);
                } else {
                    reaction_xfer_him(0xA3, 0.06f, 0);
                }
            }
            if (owner->collision_result == 2) {
                outcome = 3;
            }
        }
        break;
    case 1:
        trial_state_collision_check(
            0, target == g_game_info.plyr0.slot.mirror_a);
        outcome = 4;
        break;
    case 2:
        trial_state_collision_check(
            0, target == g_game_info.plyr0.slot.mirror_a);
        outcome = 5;
        break;
    }

    switch (outcome) {
    case 1:
        snd_req(owner->character_id == 0x19 || owner->character_id == 0x1A
                    ? 0x30B : 0x2CD);
        sc_spear_obj->flags_08_bits.gravity_enabled = 0;
        owner->duck_reaction_active = 0;
        if ((pdata_sc_spear->flags & 0x40) != 0) {
            YXZ_angles_to_quat(
                &spear_rotation, &sc_spear_obj->bones[0]->rotation_90);
        }
        pdata_sc_spear->bonematcher = start_bone_matcher(
            2.0f, owner->his_obj, pdata_sc_spear->field_34,
            sc_spear_obj, 0);
        if (pdata_sc_spear->bonematcher == 0) {
            ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
                p_sc_spear_kill, 0.0f);
            return 0.0f;
        }
        if ((pdata_sc_spear->flags & 0x40) == 0) {
            pdata_sc_spear->bonematcher->blend = 0.4f;
            ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
                p_sc_spear3_pre, 25.0f);
            return 25.0f;
        }
        pdata_sc_spear->bonematcher->blend = 0.25f;
        pdata_sc_spear->bonematcher->flags |= 0x40;
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            player_sleep_forever, 1.0f);
        return 1.0f;
    case 3:
        snd_req(owner->character_id == 0x19 || owner->character_id == 0x1A
                    ? 0x30C : 0x2CE);
        sc_spear_obj->flags_08_bits.gravity_enabled = 0;
        pdata_sc_spear->flags |= 0x80;
        pdata_sc_spear->blocked_ticks = 8;
        owner->duck_reaction_active = 0;
        if (owner->secondary_state == 0x101) {
            owner->state = 0x4206;
        }
        owner->secondary_state = 0;
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_blocked, 0.0f);
        return 0.0f;
    case 4:
        owner->duck_reaction_active = 0;
        if (owner->secondary_state == 0x101) {
            owner->state = 0x4206;
        }
        owner->secondary_state = 0;
        return 1.0f;
    case 5:
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    default:
        return 1.0f;
    }
}

static float p_sc_spear2_victory(void) {
    CameraObj* camera;
    NcsSpearEffect* effect;
    NcsFloatBits bits;
    float dx;
    float dz;
    float squared;
    float root;

    camera = camera_item.node;
    if (camera != 0 && camera->instance != camera_item.instance) {
        camera = 0;
    }
    if (camera == 0) {
        return 1.0f;
    }

    dx = camera->pos_x - sc_spear_obj->pos.x;
    dz = camera->pos_z - sc_spear_obj->pos.z;
    squared = dx * dx + dz * dz;
    bits.f = squared;
    root = 0.0f;
    if (squared > 0.0f) {
        bits.u =
            ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
            ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
        root = 0.5f * (bits.f * (3.0f - (bits.f * bits.f) / squared));
    }

    if (root < 0.3f) {
        sc_spear_obj->pos_vel.x = 0.0f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.z = 0.0f;
        effect = ncs_get_spear_effect();
        if (effect != 0) {
            effect->field_2A0 = 0.75f;
            effect->field_2A8 = 0.005f;
        }
    }
    return 1.0f;
}

static float p_sc_spear2_getup(void) {
    NcsSpearEffect* effect;

    if (sc_spear_obj->pos.y > g_game_info.field_34 + 4.0f &&
        sc_spear_obj->pos_vel.y != 0.0f) {
        sc_spear_obj->pos_vel.x = 0.0f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.z = 0.0f;
        effect = ncs_get_spear_effect();
        if (effect != 0) {
            effect->field_2A0 = 0.75f;
            effect->field_2A8 = 0.005f;
        }
    }
    return 1.0f;
}

float p_sc_spear_blocked(void) {
    NcsSpearEffect* effect;

    effect = pdata_sc_spear->effect;
    if (effect != 0 &&
        effect->hdr.instance != pdata_sc_spear->effect_instance) {
        effect = 0;
    }
    if (effect == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    pdata_sc_spear->owner->duck_reaction_active = 0;
    if ((pdata_sc_spear->blocked_ticks & 1) != 0) {
        set_pfx_texture(
            &effect->vm, (void*)0x10005, (void*)0x20039);
    } else {
        int art_section = get_shared_art_section_for_player(
            (SharedArtPlayer*)pdata_sc_spear->opponent_object);
        RwTexture* texture = load_named_tga_from_slot(
            art_section, "ROPE");

        pfx_set_texture((PfxRenderView*)&effect->vm, texture);
    }
    if (pdata_sc_spear->blocked_ticks != 0) {
        pdata_sc_spear->blocked_ticks--;
        return 3.0f;
    }

    pdata_sc_spear->owner->state &= ~0x1000;
    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    sc_spear_obj->pos_vel.x *= -2.25f;
    sc_spear_obj->pos_vel.y = 0.0f;
    sc_spear_obj->pos_vel.z *= -2.25f;
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(p_sc_spear4, 1.0f);
    return 1.0f;
}

float p_sc_spear_retract(void) {
    MkObj* owner_object;

    owner_object = pdata_sc_spear->owner->plyr_info->slot.mirror_a;
    pdata_sc_spear->owner->duck_reaction_active = 0;
    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->instance != 0) {
            pdata_sc_spear->bonematcher->vtbl->destroy(
                (NcsDestroyable*)pdata_sc_spear->bonematcher);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    if ((pdata_sc_spear->flags & 0x40) == 0) {
        if ((pdata_sc_spear->flags & 0x20) != 0) {
            Vec target;
            int bone = get_bid_with_flip(owner_object, 0x19);

            get_bone_world_pos(owner_object, bone, &target);
            sc_spear_obj->pos_vel.x =
                (target.x - sc_spear_obj->pos.x) * 0.065f;
            sc_spear_obj->pos_vel.y =
                (target.y - sc_spear_obj->pos.y) * 0.065f;
            sc_spear_obj->pos_vel.z =
                (target.z - sc_spear_obj->pos.z) * 0.065f;
            ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
                p_sc_spear4_getup, 1.0f);
            return 1.0f;
        }
        sc_spear_obj->pos_vel.x *= -2.25f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.z *= -2.25f;
        if (sc_spear_obj->pos.y < 1.0f) {
            sc_spear_obj->pos.y = 1.0f;
        }
    }
    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(p_sc_spear4, 1.0f);
    return 1.0f;
}

/* Soft ceiling: remaining differences are register allocation and scheduling. */
float p_sc_spear_retract_victory(void) {
    PlyrPdata* owner;
    MkObj* owner_object;
    Vec target;

    owner = pdata_sc_spear->owner;
    owner_object = owner->plyr_info->slot.mirror_a;
    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->instance != 0) {
            pdata_sc_spear->bonematcher->vtbl->destroy(
                (NcsDestroyable*)pdata_sc_spear->bonematcher);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    if (am_i_on_the_left2(
            owner_object,
            owner->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        get_bone_world_pos(owner_object, 0x18, &target);
    } else {
        get_bone_world_pos(owner_object, 0x19, &target);
    }
    sc_spear_obj->pos_vel.x =
        (target.x - sc_spear_obj->pos.x) * 0.035f;
    sc_spear_obj->pos_vel.y =
        (target.y - sc_spear_obj->pos.y) * 0.035f;
    sc_spear_obj->pos_vel.z =
        (target.z - sc_spear_obj->pos.z) * 0.035f;
    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
        p_sc_spear4_victory, 1.0f);
    return 1.0f;
}

static float p_sc_spear3_pre(void) {
    Vec angle;

    v3_to_xy_ang(&angle, (const Vec*)&sc_spear_obj->field_24->at);
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(p_sc_spear3, 50.0f);
    return 50.0f;
}

static float p_sc_spear3(void) {
    NcsSpearEffect* effect;

    effect = ncs_get_spear_effect();
    if (effect != 0) {
        effect->field_298 = 1.0f;
        effect->field_29C = 0.0f;
        effect->field_2A0 = 0.95f;
        effect->field_2A4 = 0.96f;
        effect->field_2A8 = 0.0f;
        effect->active = 1;
    }
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(p_sc_spear4, 0.0f);
    return 0.0f;
}

static float p_sc_spear4(void) {
    MkObj* target;
    int collision;

    target = pdata_sc_spear->owner->tracked_obj;
    if (target != 0 &&
        target->hdr.instance !=
            pdata_sc_spear->owner->tracked_obj_instance) {
        target = 0;
    }
    if (target == 0 || (g_game_info.flags & 0x18) != 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    collision = simple_3d_projectile_collision(
        &target->pos, &target->pos, &sc_spear_obj->pos,
        1, 1.5f, 200.0f, 0.25f);
    if (collision == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    if ((pdata_sc_spear->flags & 0x40) == 0) {
        const RwV3d* spear_at = &sc_spear_obj->field_24->at;
        const RwV3d* target_at = &target->field_24->at;
        float facing_dot =
            spear_at->x * target_at->x +
            spear_at->y * target_at->y +
            spear_at->z * target_at->z;

        if (facing_dot < 0.75f) {
            ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
                p_sc_spear_kill, 0.0f);
            return 0.0f;
        }
    }
    if (collision == 2) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

static float p_sc_spear4_victory(void) {
    MkObj* target_object;
    Vec target;
    NcsFloatBits bits;
    float dx;
    float dz;
    float squared;
    float distance;
    float speed_squared;
    float speed;
    float direction_squared;
    float inverse_length;

    target_object = pdata_sc_spear->owner->tracked_obj;
    if (target_object != 0 &&
        target_object->hdr.instance !=
            pdata_sc_spear->owner->tracked_obj_instance) {
        target_object = 0;
    }
    if (target_object == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    get_bone_world_pos(
        target_object, get_bid_with_flip(target_object, 0x19), &target);
    dx = target.x - sc_spear_obj->pos.x;
    dz = target.z - sc_spear_obj->pos.z;
    squared = dx * dx + dz * dz;
    distance = 0.0f;
    if (squared > 0.0f) {
        bits.f = squared;
        bits.u =
            ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
            ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
        distance = 0.5f *
            (bits.f * (3.0f - (bits.f * bits.f) / squared));
    }
    if (distance < 0.5f) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    speed_squared =
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z;
    speed = 0.0f;
    if (speed_squared > 0.0f) {
        bits.f = speed_squared;
        bits.u =
            ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
            ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
        speed = 0.5f *
            (bits.f * (3.0f - (bits.f * bits.f) / speed_squared));
    }

    sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.x;
    sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.y;
    sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.z;
    direction_squared =
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z;
    inverse_length = 0.0f;
    if (direction_squared > 0.0f) {
        float estimate;
        float product;
        float correction;

        bits.f = direction_squared;
        bits.u = 0x5F375A00 - (bits.u >> 1);
        estimate = bits.f;
        product = estimate * (direction_squared * estimate);
        correction = 3.0f - product;
        inverse_length =
            0.0625f * estimate * correction *
            -(correction * (product * correction) - 12.0f);
    }
    sc_spear_obj->pos_vel.x *= inverse_length;
    sc_spear_obj->pos_vel.y *= inverse_length;
    sc_spear_obj->pos_vel.z *= inverse_length;
    sc_spear_obj->pos_vel.x *= speed;
    sc_spear_obj->pos_vel.y *= speed;
    sc_spear_obj->pos_vel.z *= speed;
    return 1.0f;
}

static float p_sc_spear4_getup(void) {
    MkObj* target_object;
    Vec target;
    NcsFloatBits bits;
    float speed_squared;
    float speed;
    float direction_squared;
    float inverse_length;

    target_object = pdata_sc_spear->owner->tracked_obj;
    if (target_object != 0 &&
        target_object->hdr.instance !=
            pdata_sc_spear->owner->tracked_obj_instance) {
        target_object = 0;
    }
    if (target_object == 0) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    get_bone_world_pos(
        target_object, get_bid_with_flip(target_object, 0x19), &target);
    if (target.y > sc_spear_obj->pos.y) {
        ((NcsProcVtable*)aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    speed_squared =
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z;
    speed = 0.0f;
    if (speed_squared > 0.0f) {
        bits.f = speed_squared;
        bits.u =
            ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
            ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
        speed = 0.5f *
            (bits.f * (3.0f - (bits.f * bits.f) / speed_squared));
    }

    sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.x;
    sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.y;
    sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.z;
    direction_squared =
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z;
    inverse_length = 0.0f;
    if (direction_squared > 0.0f) {
        float estimate;
        float product;
        float correction;

        bits.f = direction_squared;
        bits.u = 0x5F375A00 - (bits.u >> 1);
        estimate = bits.f;
        product = estimate * (direction_squared * estimate);
        correction = 3.0f - product;
        inverse_length =
            0.0625f * estimate * correction *
            -(correction * (product * correction) - 12.0f);
    }
    sc_spear_obj->pos_vel.x *= inverse_length;
    sc_spear_obj->pos_vel.y *= inverse_length;
    sc_spear_obj->pos_vel.z *= inverse_length;
    sc_spear_obj->pos_vel.x *= speed;
    sc_spear_obj->pos_vel.y *= speed;
    sc_spear_obj->pos_vel.z *= speed;
    return 1.0f;
}

float p_sc_spear_kill(void) {
    NcsSpearEffect* effect;
    NcsSpearObjectView* weapon;
    PlyrPdata* owner;

    effect = ncs_get_spear_effect();
    if (effect != 0 && effect->hdr.instance != 0) {
        ((NcsDestroyable*)effect)->vtbl->destroy((NcsDestroyable*)effect);
    }

    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->instance != 0) {
            pdata_sc_spear->bonematcher->vtbl->destroy(
                (NcsDestroyable*)pdata_sc_spear->bonematcher);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    sc_spear_obj->flags_08_bits.gravity_enabled = 0;
    weapon = (NcsSpearObjectView*)sc_spear_obj;
    weapon->active = 0;
    owner = pdata_sc_spear->owner;
    if (owner->character_id == 0) {
        weapon->data_table = get_data_table(owner->cmo, 0x18);
    }
    if (owner->character_id == 0x1C) {
        weapon->data_table = get_data_table(owner->cmo, 0x15);
    }
    if (owner->character_id == 0x19 ||
        owner->character_id == 0x1A) {
        weapon->data_table = get_data_table(owner->cmo, 2);
    }
    plyr_aux_weapon_grab(owner, sc_spear_obj);
    owner->duck_reaction_active = 0;
    return -1.0f;
}

static float p_pfx_sc_spear(void) {
    NcsSpearEffect* effect;
    SpearProcPdata* spear_pdata;
    PlyrPdata* owner;
    MkObj* target;
    MkObj* emitter_object;
    MkPfx* pfx;
    Vec target_position;
    Vec direction;
    Vec* particle_position;
    float length;
    float amplitude;
    float base_spacing;
    float phase;
    float distance;
    int particle_count;
    int stride;

    if (apfx_emitter_obj == 0) {
        mkproc_die();
    }
    effect = (NcsSpearEffect*)apfx;
    spear_pdata = effect->spear_pdata;
    if (spear_pdata != 0 &&
        spear_pdata->hdr.instance != effect->spear_pdata_instance) {
        spear_pdata = 0;
    }
    if (spear_pdata == 0) {
        return -1.0f;
    }
    owner = spear_pdata->owner;
    target = owner->tracked_obj;
    if (target != 0 && target->hdr.instance != owner->tracked_obj_instance) {
        target = 0;
    }
    if (target == 0) {
        return -1.0f;
    }

    pfx = (MkPfx*)effect;
    stride = pfx->mats[0].particle_stride;
    particle_position = (Vec*)pfx_get_field(&effect->vm, -2, 0x100);
    emitter_object = apfx_emitter_obj;
    get_bone_world_pos(target, effect->bone, &target_position);
    v3_sub_v3(&direction, &emitter_object->pos, &target_position);
    length = normalize_v3_length(&direction);

    if (effect->field_298 > 0.0f) {
        if (pfx->accum_38 > (float)effect->field_290) {
            effect->field_290 += (int)game_speed + 1;
            effect->field_298 *= effect->field_2A0;
            effect->field_2A4 += effect->field_2A8;
            if (effect->field_2A4 > 1.02f) {
                effect->field_2A4 = 1.02f;
            }
            effect->field_29C +=
                6.2831855f / (5.5f + (float)effect->active);
            if (effect->field_29C >= 6.2831855f) {
                effect->field_29C -= 6.2831855f;
                effect->active++;
            }
        }
        amplitude = effect->field_298 * gxMathSin(effect->field_29C);
        base_spacing = length / (float)effect->active;
    } else {
        amplitude = 0.0f;
        base_spacing = length;
    }

    phase = 0.0f;
    distance = 0.0f;
    particle_count = 0;
    while (distance <= length - 0.05f &&
           particle_count < pfx->field_90) {
        float step;

        if (effect->field_298 > 0.0f) {
            float absolute_amplitude = amplitude < 0.0f
                                           ? -amplitude : amplitude;
            float sine;

            phase += 0.31415927f /
                (3.1415927f * absolute_amplitude + base_spacing);
            sine = gxMathSin(phase);
            if (sine < 0.0f) {
                sine = -sine;
            }
            amplitude *= effect->field_2A4;
            step =
                (0.05f * (1.0f - absolute_amplitude) +
                 absolute_amplitude * (2.0f * sine * 0.05f)) /
                (1.0f + absolute_amplitude);
        } else {
            step = 0.05f;
        }
        distance += step;
        particle_position->x =
            emitter_object->pos.x - direction.x * distance;
        particle_position->z =
            emitter_object->pos.z - direction.z * distance;
        if ((spear_pdata->flags & 0x80) == 0) {
            if (effect->field_298 > 0.01f) {
                particle_position->y =
                    emitter_object->pos.y + amplitude * gxMathSin(phase);
            } else {
                particle_position->y = emitter_object->pos.y;
            }
            particle_position->y -=
                distance * (particle_position->y - target_position.y) /
                length;
        } else {
            particle_position->y = emitter_object->pos.y;
        }
        particle_position = (Vec*)((unsigned char*)particle_position + stride);
        particle_count++;
    }
    pfx->field_94 = particle_count;
    return 1.0f;
}

void retract_spear_from_camera(void) {
    xfer_proc(aproc, p_sc_spear_retract_victory);
}

void xfer_spearproc_to_retract(void) {
    xfer_proc(aproc, p_sc_spear_retract);
}

void destroy_spearproc_bonematcher(MkProc* proc) {
    SpearProcPdataRef pdata;
    NcsBoneMatcher* matcher;

    pdata.hdr = pdata_of_proc(proc);
    if (pdata.hdr != 0) {
        matcher = pdata.spear->bonematcher;
        if (matcher->instance != 0) {
            matcher->vtbl->destroy((NcsDestroyable*)matcher);
        }
        pdata.spear->bonematcher = 0;
    }
}

void insert_mkobj_spearproc_parentobjitem(MkObj* parent, MkProc* proc) {
    SpearProcPdataRef pdata;

    pdata.hdr = pdata_of_proc(proc);
    if (pdata.hdr != 0) {
        pdata.spear->bonematcher->parent = parent;
        pdata.spear->bonematcher->parent_instance = parent->hdr.instance;
    }
}

MkObj* get_spearobj_from_spearproc(void) {
    SpearProcPdata* pdata;
    MkObj* object;

    pdata = (SpearProcPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return 0;
    }
    object = pdata->spear_object;
    if (object != 0 &&
        object->hdr.instance != pdata->spear_object_instance) {
        object = 0;
    }
    return object;
}

void sc_spear_postsleep(void) {
    pdata_sc_spear = 0;
    sc_spear_obj = 0;
    his_pdata = 0;
    his_obj = 0;
}

void sc_spear_prewake(void) {
    MkObj* object;

    if (aproc->pid != 0x5019) {
        mkproc_die();
    }
    pdata_sc_spear = (SpearProcPdata*)apdata;
    if (pdata_sc_spear == 0) {
        mkproc_die();
    }
    object = pdata_sc_spear->spear_object;
    if (object != 0 &&
        object->hdr.instance != pdata_sc_spear->spear_object_instance) {
        object = 0;
    }
    sc_spear_obj = object;
    if (object == 0) {
        mkproc_die();
    }
    his_pdata = pdata_sc_spear->opponent_pdata;
    his_obj = pdata_sc_spear->opponent_object;
}

int dkp_check_plyr_state_for_grab(PlyrPdata* player) {
    int state;

    if (f_fatality_was_done != 0 ||
        g_game_info.plyr0.field_0C == 0.0f ||
        g_game_info.plyr1.field_0C == 0.0f) {
        return 0;
    }

    state = player->state;
    if (state == 0 || (state & 0x800) != 0) {
        return 1;
    }
    if ((state & 0x2000) != 0 &&
        (state & 0x4000) == 0 &&
        (state & 0x200) == 0) {
        return 1;
    }
    return 0;
}

void stop_prison_grab_proc(void) {
    MkProc* proc;

    proc = find_mkproc_pid(0x603D);
    if (proc != 0 && g_game_info.bgnd_id == 2) {
        do {
            if (proc->instance != 0) {
                proc->hdr.typed_vtbl->destroy(&proc->hdr);
            }
            proc = find_mkproc_pid(0x603D);
        } while (proc != 0);
        one_shot_script_func(g_game_info.cmdscript, 0x27, 0);
    }
}

PrisonGrabPdata* start_prison_grab_proc(
    MkObj* object, const Vec* direction, float angle, float strength) {
    PrisonGrabPdata* pdata;
    NcsFloatBits bits;
    float squared;
    float estimate;
    float product;
    float correction;
    float inverse_length;

    pdata = 0;
    if (_create_mkproc_generic_nostack(
            0x603D, 0x1F, p_prison_grab, sizeof(PrisonGrabPdata),
            (MkHdr**)&pdata) == 0) {
        return 0;
    }

    zero_pdata_payload(sizeof(PrisonGrabPdata), &pdata->hdr);
    pdata->object = object;
    pdata->object_instance = object->hdr.instance;
    squared = direction->x * direction->x + direction->z * direction->z;
    inverse_length = 0.0f;
    if (squared > 0.0f) {
        bits.f = squared;
        bits.u = 0x5F375A00 - (bits.u >> 1);
        estimate = bits.f;
        product = estimate * (squared * estimate);
        correction = 3.0f - product;
        inverse_length =
            0.0625f * estimate * correction *
            -(correction * (product * correction) - 12.0f);
    }
    pdata->target_x =
        direction->x - direction->x * inverse_length * strength;
    pdata->target_z =
        direction->z - direction->z * inverse_length * strength;
    if (angle < 0.0f) {
        pdata->target_angle = 6.2831855f + angle;
    } else {
        pdata->target_angle = angle;
    }
    pdata->current_x = object->pos.x;
    pdata->current_y = object->pos.y;
    pdata->current_z = object->pos.z;
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_pdata->blocking_disabled = 0;
    set_my_state(0x600);
    return pdata;
}

void done_prison_grab_proc(PrisonGrabPdata* pdata) {
    pdata->done = 1;
}

static float p_prison_grab(void) {
    PrisonGrabPdata* pdata;
    MkObj* object;
    NcsFloatBits bits;
    float squared;
    float estimate;
    float product;
    float correction;
    float inverse_length;
    int position_axes_done;

    pdata = (PrisonGrabPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    if (pdata->done != 0) {
        squared = pdata->target_x * pdata->target_x +
                  pdata->target_z * pdata->target_z;
        inverse_length = 0.0f;
        if (squared > 0.0f) {
            bits.f = squared;
            bits.u = 0x5F375A00 - (bits.u >> 1);
            estimate = bits.f;
            product = estimate * (squared * estimate);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate * correction *
                -(correction * (product * correction) - 12.0f);
        }
        object->pos_vel.x = pdata->target_x * inverse_length * 0.1f;
        object->pos_vel.y = 0.0f;
        object->pos_vel.z = pdata->target_z * inverse_length * 0.1f;
        return -1.0f;
    }

    if (pdata->aligned != 0) {
        object->ang.y = pdata->target_angle;
    } else {
        position_axes_done = 0;
        if (pdata->current_x < pdata->target_x) {
            pdata->current_x += 0.1f;
            if (pdata->current_x > pdata->target_x) {
                pdata->current_x = pdata->target_x;
                position_axes_done = 1;
            }
        } else if (pdata->current_x > pdata->target_x) {
            pdata->current_x -= 0.1f;
            if (pdata->current_x < pdata->target_x) {
                pdata->current_x = pdata->target_x;
                position_axes_done = 1;
            }
        } else {
            position_axes_done = 1;
        }

        if (pdata->current_z < pdata->target_z) {
            pdata->current_z += 0.1f;
            if (pdata->current_z > pdata->target_z) {
                pdata->current_z = pdata->target_z;
                position_axes_done++;
            }
        } else if (pdata->current_z > pdata->target_z) {
            pdata->current_z -= 0.1f;
            if (pdata->current_z < pdata->target_z) {
                pdata->current_z = pdata->target_z;
                position_axes_done++;
            }
        } else {
            position_axes_done++;
        }

        if (object->ang.y < pdata->target_angle) {
            object->ang.y += 0.02f;
            if (object->ang.y > pdata->target_angle) {
                object->ang.y = pdata->target_angle;
            }
        } else if (object->ang.y > pdata->target_angle) {
            object->ang.y -= 0.02f;
            if (object->ang.y < pdata->target_angle) {
                object->ang.y = pdata->target_angle;
            }
        }
        if (position_axes_done > 1) {
            object->ang.y = pdata->target_angle;
            pdata->aligned = 1;
        }
    }

    object->pos.x = pdata->current_x;
    object->pos.z = pdata->current_z;
    return 1.0f;
}

void ncs_dkp_camera_konqchar_show_hide_alpha(
    int character_index, MkObj* character) {
    MkProc* process;
    NcsKonquestCharacterPdata* pdata;

    process = g_game_info.camera_proc;
    process = process != 0
                  ? (process->instance == g_game_info.camera_proc_instance
                         ? process
                         : 0)
                  : 0;
    if (process == 0) {
        return;
    }

    pdata = (NcsKonquestCharacterPdata*)pdata_of_proc(process);
    if (pdata == 0 || character_index < 0x0B || character_index > 0x12) {
        return;
    }

    pdata->characters[character_index - 0x0B] = character;
    obj_create_sobjs(character);
    sobj_set_priority(obj_first_sobj(character), 0x12);
}

void ncs_camera_wall_show_hide_alpha(
    NcsCameraWallRegion* regions) {
    MkProc* process;
    NcsCameraWallPdata* pdata;
    NcsCameraWallRegion* region;
    int index;

    process = g_game_info.camera_proc;
    if (process != 0 &&
        process->instance != g_game_info.camera_proc_instance) {
        process = 0;
    }
    pdata = 0;
    if (process != 0) {
        pdata = (NcsCameraWallPdata*)pdata_of_proc(process);
        if (pdata == 0) {
            process = 0;
        }
    }
    if (process == 0) {
        process = _create_mkproc_generic_nostack(
            0x6008, 0x1F, p_camera_wall_show_hide_alpha,
            sizeof(NcsCameraWallPdata), (MkHdr**)&pdata);
        if (process == 0) {
            return;
        }
    }

    zero_pdata_payload(sizeof(NcsCameraWallPdata), &pdata->hdr);
    g_game_info.camera_proc = process;
    g_game_info.camera_proc_instance = process->instance;
    pdata->regions = regions;
    pdata->active_region = -1;

    for (region = regions; region->type < 3; region++) {
        const int* ids;

        if (region->type == 0) {
            region->max_z = region->min_z * region->min_z;
        }
        for (ids = region->show_ids; *ids >= 0; ids++) {
            MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                g_game_info.bgnd_obj, (unsigned int)*ids);

            if (sobj != 0) {
                sobj->z_offset = -50.0f;
            }
        }
        for (ids = region->alpha_ids; *ids >= 0; ids++) {
            MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                g_game_info.bgnd_obj, (unsigned int)*ids);

            if (sobj != 0 && sobj->atomic != 0 &&
                sobj->atomic->geometry != 0) {
                sobj->atomic->geometry->flags |= 0x40;
                atomic_set_transl_flag(sobj->atomic);
            }
        }
    }
    for (index = 0; index < 8; index++) {
        pdata->characters[index] = 0;
    }
    pdata->special_alpha_initialized = 0;
}

static float p_camera_wall_show_hide_alpha(void) {
    NcsCameraWallPdata* pdata;
    NcsCameraWallRegion* region;
    CamVec3 camera_position;
    int white;
    int region_index;

    pdata = (NcsCameraWallPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    if (pdata->regions == 0) {
        return 1.0f;
    }

    get_camera_position(&camera_position);
    white = 0xFFFFFFFF;
    region = pdata->regions;
    region_index = 0;
    while (region->type < 3) {
        int inside;

        if (region->type == 0) {
            float dx = camera_position.x - region->min_x;
            float dz = camera_position.z - region->max_x;

            inside = dx * dx + dz * dz <= region->max_z;
        } else if (region->type == 1) {
            inside = camera_position.x > region->min_x &&
                     camera_position.x < region->max_x &&
                     camera_position.z > region->min_z &&
                     camera_position.z < region->max_z;
        } else {
            inside = 0;
        }
        if (inside != 0) {
            break;
        }
        region++;
        region_index++;
    }

    if (region_index != pdata->active_region) {
        if (pdata->active_region >= 0) {
            NcsCameraWallRegion* previous =
                &pdata->regions[pdata->active_region];
            const int* ids;

            for (ids = previous->hide_ids; *ids >= 0; ids++) {
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)*ids);

                if (sobj != 0 && sobj->atomic != 0) {
                    unhide_atomic(sobj->atomic);
                }
            }
            for (ids = previous->show_ids; *ids >= 0; ids++) {
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)*ids);

                if (sobj != 0 && sobj->atomic != 0) {
                    hide_atomic(sobj->atomic);
                }
            }
            for (ids = previous->alpha_ids; *ids >= 0; ids++) {
                int id = *ids;
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)id);

                if (sobj != 0) {
                    sobj_set_color_for_all_materials(sobj, &white);
                    sobj->flags09_bits.bit7 = 0;
                }
                if (id >= 0x0B && id <= 0x12 &&
                    pdata->characters[id - 0x0B] != 0) {
                    obj_for_all_atomics_set_material_alpha(
                        pdata->characters[id - 0x0B], 0xFF);
                }
            }
        }

        if (region->type >= 3) {
            pdata->active_region = -1;
        } else {
            const int* ids;

            pdata->active_region = region_index;
            for (ids = region->hide_ids; *ids >= 0; ids++) {
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)*ids);

                if (sobj != 0 && sobj->atomic != 0) {
                    hide_atomic(sobj->atomic);
                }
            }
            for (ids = region->show_ids; *ids >= 0; ids++) {
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)*ids);

                if (sobj != 0 && sobj->atomic != 0) {
                    unhide_atomic(sobj->atomic);
                }
            }
            for (ids = region->alpha_ids; *ids >= 0; ids++) {
                int id = *ids;
                MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, (unsigned int)id);

                if (sobj != 0) {
                    sobj_set_color_for_all_materials(sobj, &white);
                    sobj->flags09_bits.bit7 = 1;
                }
                if (id >= 0x0B && id <= 0x12 &&
                    pdata->characters[id - 0x0B] != 0) {
                    obj_for_all_atomics_set_material_alpha(
                        pdata->characters[id - 0x0B], 0xFF);
                }
            }
        }
    }

    if ((g_game_info.flags & 0x10) != 0 &&
        pdata->special_alpha_initialized == 0) {
        int id;

        pdata->special_alpha_initialized = 1;
        for (id = 0x4E; id <= 0x54; id++) {
            MkSobj* sobj = (MkSobj*)obj_find_sobj_by_id(
                g_game_info.bgnd_obj, (unsigned int)id);

            sobj_set_color_for_all_materials(sobj, &white);
            sobj->flags09_bits.bit7 = 0;
        }
    }
    return 1.0f;
}

void destroy_gore2_obj(unsigned int object_id, int particle_index) {
    Gore2Pool* pool;
    int type;

    type = 0;
    while (type < 10 &&
           pbl_gore2_obj_list[type].object_id != object_id) {
        type++;
    }
    if (type >= 10) {
        return;
    }

    pool = mkpdata_pbl_gore2_update->pools[type];
    if (particle_index >= pool->capacity) {
        return;
    }
    pool->states[particle_index].bits.visible = 0;
}

int attach_gore2_obj(
    MkObj* owner, int bone, unsigned int object_id,
    const Vec* offset, const Vec* rotation) {
    Gore2Pool* pool;
    Gore2Particle* particle;
    int particle_index;
    int result;
    int type;

    type = 0;
    while (type < 10 &&
           pbl_gore2_obj_list[type].object_id != object_id) {
        type++;
    }
    if (type >= 10) {
        return -1;
    }
    if ((unsigned int)bone != 0x40000000) {
        MkBone* attach_bone = plyr_obj->bones[bone];

        if (attach_bone == 0) {
            return -1;
        }
        attach_bone->flags_54 |= 0x10;
    }

    pool = mkpdata_pbl_gore2_update->pools[type];
    particle_index = mkpdata_pbl_gore2_update->next_particle[type];
    result = particle_index;
    particle = &pool->particles[particle_index];
    particle->flags.word = 0;
    if (rotation != 0) {
        particle->flags.bits.has_rotation = 1;
        particle->rotation.x = rotation->x;
        particle->rotation.y = rotation->y;
        particle->rotation.z = rotation->z;
    } else {
        particle->flags.bits.has_rotation = 0;
        particle->rotation.x = 0.0f;
        particle->rotation.y = 0.0f;
        particle->rotation.z = 0.0f;
    }
    if (offset != 0) {
        particle->flags.bits.has_translation = 1;
        particle->translation.x = offset->x;
        particle->translation.y = offset->y;
        particle->translation.z = offset->z;
    } else {
        particle->flags.bits.has_translation = 0;
        particle->translation.x = 0.0f;
        particle->translation.y = 0.0f;
        particle->translation.z = 0.0f;
    }
    particle->flags.bits.attached = 1;
    particle->owner.object = owner;
    particle->owner.instance = owner->hdr.instance;
    particle->bone = bone;
    pool->states[particle_index].bits.visible = 1;

    particle_index++;
    if (particle_index >= pool->capacity) {
        particle_index = 0;
    }
    mkpdata_pbl_gore2_update->next_particle[type] = particle_index;
    return result;
}

void start_gore2_pebbles(
    unsigned int object_id, int bone, MkObj* source,
    FighterMirror* decal_owner, const Vec* velocity,
    const Vec* rotation, const Vec* scale,
    const Vec* position_offset, float vertical_acceleration,
    float bounce_scale, int bounce_count) {
    int type;

    type = 0;
    while (type < 10 &&
           pbl_gore2_obj_list[type].object_id != object_id) {
        type++;
    }
    if (type < 10) {
        MkBone* source_bone = source->bones[bone];

        if (source_bone != 0) {
            Gore2Pool* pool = mkpdata_pbl_gore2_update->pools[type];
            int particle_index =
                mkpdata_pbl_gore2_update->next_particle[type];
            Gore2Particle* particle =
                &pool->particles[particle_index];
            MKMATRIX* matrix =
                &pool->pebbles[particle_index].matrix;
            Vec bone_position;

            particle->flags.word = 0;
            get_bone_world_pos(source, bone, &bone_position);
            matrix->pos.x = bone_position.x;
            matrix->pos.y = bone_position.y;
            matrix->pos.z = bone_position.z;
            calc_bone_world_mat(source, bone);
            if (rotation != 0) {
                particle->flags.bits.has_rotation = 1;
                particle->rotation.x = rotation->x;
                particle->rotation.y = rotation->y;
                particle->rotation.z = rotation->z;
            } else {
                particle->rotation.x = 0.0f;
                particle->rotation.y = 0.0f;
                particle->rotation.z = 0.0f;
            }
            if (scale != 0) {
                particle->flags.bits.has_scale = 1;
                particle->scale.x = scale->x;
                particle->scale.y = scale->y;
                particle->scale.z = scale->z;
            } else {
                particle->scale.x = 0.0f;
                particle->scale.y = 0.0f;
                particle->scale.z = 0.0f;
            }
            if (velocity != 0) {
                particle->flags.bits.has_translation = 1;
                v3_x_mat(
                    &particle->translation, velocity,
                    &source_bone->matrix);
            } else {
                particle->translation.x = 0.0f;
                particle->translation.y = 0.0f;
                particle->translation.z = 0.0f;
            }
            if (position_offset != 0) {
                Vec transformed_offset;

                if (particle->flags.bits.has_rotation) {
                    YXZ_angles_to_MKMATRIX(rotation, matrix);
                }
                if (particle->flags.bits.has_scale) {
                    mat_scaled_by_v3(matrix, matrix, scale);
                }
                v3_x_mat(
                    &transformed_offset, position_offset,
                    &source_bone->matrix);
                matrix->pos.x += transformed_offset.x;
                matrix->pos.y += transformed_offset.y;
                matrix->pos.z += transformed_offset.z;
            }
            particle->flags.bits.settled = 0;
            particle->vertical_acceleration = vertical_acceleration;
            particle->bounce_count = bounce_count;
            particle->bounce_scale = bounce_scale;
            particle->decal_owner = decal_owner;
            pool->states[particle_index].bits.visible = 1;
            particle_index++;
            if (particle_index >= pool->capacity) {
                particle_index = 0;
            }
            mkpdata_pbl_gore2_update->next_particle[type] =
                particle_index;
        }
    }
}

void start_gore2_update(void) {
    Gore2UpdatePdata* pdata;
    int type;

    pdata = 0;
    if (_create_mkproc_generic_nostack(
            0x603F, 0x1F, p_gore2_update,
            sizeof(Gore2UpdatePdata), (MkHdr**)&pdata) != 0) {
        zero_pdata_payload(sizeof(Gore2UpdatePdata), &pdata->hdr);
        load_light(pbl_gore2_lights[1], &gore2_light_list, 0);
        for (type = 0; type < 10; type++) {
            Gore2ObjectType* object_type = &pbl_gore2_obj_list[type];
            MkObj* object = load_model_from_slot(
                0x10005, object_type->object_id, 0x602A);

            if (object != 0) {
                MkSobj* sobj;

                obj_create_sobjs(object);
                sobj = obj_first_sobj(object);
                if (sobj != 0) {
                    Gore2Pool* pool = (Gore2Pool*)create_pebble_userdata(
                        sobj, object_type->particle_count,
                        sizeof(Gore2Particle));

                    pdata->pools[type] = pool;
                    if (pool != 0) {
                        int particle;

                        mk_insert(&object->hdr, &g_game_info.bgnd_obj->child_list);
                        insert_fgnd_mkobj(object);
                        object->light_flags = 0x800;
                        object->pos.x = 0.0f;
                        object->pos.y = 0.0f;
                        object->pos.z = 0.0f;
                        object->ang.x = 0.0f;
                        object->ang.y = 0.0f;
                        object->ang.z = 0.0f;
                        sobj->flags_word_08 = 0;
                        sobj->flags_08_bits.bit6 = 1;
                        sobj->flags_08_bits.bit0 = 0;
                        sobj->flags09_bits.bit7 = 0;
                        sobj->flags09_bits.bit4 = 1;
                        sobj->flags09_bits.bit3 = 1;
                        sobj->flags09_bits.has_pebbles = 1;
                        sobj->z_offset = 0.0f;
                        sobj_set_priority(sobj, 0x12);
                        for (particle = 0;
                             particle < object_type->particle_count;
                             particle++) {
                            pool->states[particle].bits.visible = 0;
                            pool->pebbles[particle].matrix.pos.x = 0.0f;
                            pool->pebbles[particle].matrix.pos.z = 0.0f;
                            pool->pebbles[particle].matrix.pos.y = -10000.0f;
                        }
                    }
                }
            }
        }
    }
    mkpdata_pbl_gore2_update = pdata;
}

/*
 * Soft ceiling: retail dynamically aligns the matrix workspace to 16 bytes,
 * retains explicit null-normalization branches, and chooses different loop
 * induction registers. Portable C keeps the same typed algorithm in a fixed
 * frame; reproducing the residue would require alignment or dead-control-flow
 * forcing.
 */
static float p_gore2_update(void) {
    Gore2UpdatePdata* pdata;
    int type;

    pdata = (Gore2UpdatePdata*)apdata;
    for (type = 0; type < 10; type++) {
        Gore2Pool* pool = pdata->pools[type];
        int particle_index;

        for (particle_index = 0;
             particle_index < pool->capacity; particle_index++) {
            PebbleFlags* state = &pool->states[particle_index];

            if (state->bits.visible) {
                Gore2Particle* particle =
                    &pool->particles[particle_index];

                if (!particle->flags.bits.attached) {
                    if (!particle->flags.bits.settled) {
                        if (particle->flags.bits.has_rotation) {
                            YXZ_angles_to_MKMATRIX(
                                &particle->rotation,
                                &pool->pebbles[particle_index].matrix);
                        }
                        if (particle->flags.bits.has_scale) {
                            mat_scaled_by_v3(
                                &pool->pebbles[particle_index].matrix,
                                &pool->pebbles[particle_index].matrix,
                                &particle->scale);
                        }
                        if (particle->flags.bits.has_translation) {
                            particle->translation.y +=
                                particle->vertical_acceleration;
                            pool->pebbles[particle_index].matrix.pos.x +=
                                particle->translation.x;
                            pool->pebbles[particle_index].matrix.pos.y +=
                                particle->translation.y;
                            pool->pebbles[particle_index].matrix.pos.z +=
                                particle->translation.z;
                        }
                        if (pool->pebbles[particle_index].matrix.pos.y <=
                            g_game_info.field_34 +
                                pbl_gore2_obj_list[type].scale) {
                            pool->pebbles[particle_index].matrix.pos.y =
                                g_game_info.field_34 +
                                pbl_gore2_obj_list[type].scale;
                            if (particle->bounce_count != 0) {
                                particle->bounce_count--;
                                particle->translation.y *=
                                    -particle->bounce_scale;
                            } else {
                                particle->flags.bits.settled = 1;
                                particle->flags.bits.has_rotation = 0;
                                particle->flags.bits.has_scale = 0;
                                particle->flags.bits.has_translation = 0;
                                spawn_decal_emitter(
                                    "blsplat", particle->decal_owner,
                                    (const Vec*)&pool->pebbles[particle_index]
                                        .matrix.pos,
                                    0, 0.0f);
                            }
                        }
                    }
                } else {
                    MkObj* owner = particle->owner.object;

                    if (owner != 0 &&
                        owner->hdr.instance != particle->owner.instance) {
                        owner = 0;
                    }
                    if (owner == 0) {
                        state->bits.visible = 0;
                    } else {
                        MKMATRIX* matrix =
                            &pool->pebbles[particle_index].matrix;
                        MKMATRIX* owner_matrix =
                            (MKMATRIX*)owner->field_24;

                        if ((unsigned int)particle->bone != 0x40000000) {
                            MkBone* bone = owner->bones[particle->bone];

                            if (bone != 0) {
                                owner_matrix = &bone->matrix;
                            }
                        }
                        if (particle->flags.bits.has_rotation) {
                            MKMATRIX rotation_matrix;

                            YXZ_angles_to_MKMATRIX(
                                &particle->rotation, &rotation_matrix);
                            mat_x_mat(
                                matrix, &rotation_matrix, owner_matrix);
                        } else {
                            memcpy(matrix, owner_matrix, 0x30);
                        }
                        if (particle->flags.bits.has_translation) {
                            v3_x_mat_add_v3(
                                (Vec*)&matrix->pos, &particle->translation,
                                owner_matrix, (const Vec*)&owner_matrix->pos);
                        } else {
                            matrix->pos.x = owner_matrix->pos.x;
                            matrix->pos.y = owner_matrix->pos.y;
                            matrix->pos.z = owner_matrix->pos.z;
                        }
                    }
                }
            }
        }
    }
    return 1.0f;
}

void start_sweat_particles(
    int particle_mask, int bone, PlyrPdata* player, MkObj* object) {
    CmdScript* saved_script;
    unsigned int effect;
    unsigned int emitter;
    MkPfx* particle;
    int type;

    saved_script = active_cmdscript;
    active_cmdscript =
        get_cmdscript_for_proc((MkProc*)player->plyr_info->idle_proc);
    for (type = 0; type < 3; type++) {
        if (((particle_mask >> type) & 1) != 0) {
            effect = fx_by_owner(
                mkpfx_ncs_sweat_type_map_array[type],
                1 << player->plyr_info->controller_slot);
            emitter = fx_next_emitter(effect);
            if (emitter != 0) {
                fx_resume_emit(emitter);
                particle = pfx_from_emitter(emitter);
                pfx_bind_emitter_num_to_obj_bone(
                    particle, object, bone,
                    emitter_id_from_handle(emitter));
            }
        }
    }
    active_cmdscript = saved_script;
}

void start_sweat_particles_scripts(int particle_mask, int bone) {
    PlyrPdata* player;
    MkObj* object;
    CmdScript* saved_script;
    unsigned int effect;
    unsigned int emitter;
    MkPfx* particle;
    int type;

    player = plyr_pdata;
    object = plyr_obj;
    saved_script = active_cmdscript;
    active_cmdscript =
        get_cmdscript_for_proc((MkProc*)player->plyr_info->idle_proc);
    for (type = 0; type < 3; type++) {
        if (((particle_mask >> type) & 1) != 0) {
            effect = fx_by_owner(
                mkpfx_ncs_sweat_type_map_array[type],
                1 << player->plyr_info->controller_slot);
            emitter = fx_next_emitter(effect);
            if (emitter != 0) {
                fx_resume_emit(emitter);
                particle = pfx_from_emitter(emitter);
                pfx_bind_emitter_num_to_obj_bone(
                    particle, object, bone,
                    emitter_id_from_handle(emitter));
            }
        }
    }
    active_cmdscript = saved_script;
}

unsigned int start_blood_particles(
    int particle_mask, int bone, PlyrPdata* player, MkObj* object) {
    CmdScript* saved_script;
    unsigned int emitter;
    int type;

    emitter = 0;
    if ((unsigned int)bone != 0x40000000 && object->bones[bone] == 0) {
        return emitter;
    }

    saved_script = active_cmdscript;
    active_cmdscript = get_cmdscript_for_proc(
        (MkProc*)player->plyr_info->idle_proc);
    for (type = 0; type < 11; type++) {
        if (((particle_mask >> type) & 1) != 0 &&
            get_blood_level() >=
                blood_type_list[mkpfx_type_to_blood_level_map[type]]) {
            if (((1 << type) & 0x1E0) != 0) {
                trigger_blood_glops(player, bone, object, type);
            } else {
                unsigned int effect = fx_by_owner(
                    mkpfx_ncs_blood_type_map_array[type],
                    1 << player->plyr_info->controller_slot);

                emitter = fx_next_emitter(effect);
                if (emitter != 0) {
                    MkPfx* particle;
                    int emitter_id;

                    fx_resume_emit(emitter);
                    particle = pfx_from_emitter(emitter);
                    emitter_id = emitter_id_from_handle(emitter);
                    if ((unsigned int)bone == 0x40000000) {
                        pfx_bind_emitter_num_to_obj(
                            particle, object, 0, emitter_id);
                    } else {
                        pfx_bind_emitter_num_to_obj_bone(
                            particle, object, bone, emitter_id);
                    }
                }
            }
        }
    }
    active_cmdscript = saved_script;
    return emitter;
}

unsigned int start_blood_particles_scripts(
    int particle_mask, int bone) {
    CmdScript* saved_script;
    unsigned int emitter;
    PlyrPdata* player;
    MkObj* object;
    int type;

    emitter = 0;
    object = plyr_obj;
    player = plyr_pdata;
    if ((unsigned int)bone != 0x40000000 && object->bones[bone] == 0) {
        return emitter;
    }

    saved_script = active_cmdscript;
    active_cmdscript = get_cmdscript_for_proc(
        (MkProc*)player->plyr_info->idle_proc);
    for (type = 0; type < 11; type++) {
        if (((particle_mask >> type) & 1) != 0 &&
            get_blood_level() >=
                blood_type_list[mkpfx_type_to_blood_level_map[type]]) {
            if (((1 << type) & 0x1E0) != 0) {
                trigger_blood_glops(player, bone, object, type);
            } else {
                unsigned int effect = fx_by_owner(
                    mkpfx_ncs_blood_type_map_array[type],
                    1 << player->plyr_info->controller_slot);

                emitter = fx_next_emitter(effect);
                if (emitter != 0) {
                    MkPfx* particle;
                    int emitter_id;

                    fx_resume_emit(emitter);
                    particle = pfx_from_emitter(emitter);
                    emitter_id = emitter_id_from_handle(emitter);
                    if ((unsigned int)bone == 0x40000000) {
                        pfx_bind_emitter_num_to_obj(
                            particle, object, 0, emitter_id);
                    } else {
                        pfx_bind_emitter_num_to_obj_bone(
                            particle, object, bone, emitter_id);
                    }
                }
            }
        }
    }
    active_cmdscript = saved_script;
    return emitter;
}

extern int blood_type_list[12];
extern MkPtr* gore2_light_list;
static float p_watch_obj_for_gnd_coll(void);
static float p_camera_wall_show_hide_alpha(void);
static float p_limb_sever_attach(void);
static float p_gore2_update(void);
static void trigger_blood_glops(
    PlyrPdata* player, int bone, MkObj* source, int blood_type) {
    NcsGroundCollisionWatchPdata* watcher;
    unsigned int effect;
    MkPfx* particle;
    float angle;
    int index;

    watcher = 0;
    effect = fx_by_owner(
        mkpfx_ncs_blood_type_map_array[blood_type],
        1 << player->plyr_info->controller_slot);
    if (player->next_blood_glop_tick >= (unsigned int)exec_tick_ctr) {
        return;
    }
    player->next_blood_glop_tick = (unsigned int)exec_tick_ctr + 30;

    particle = effect != 0 ? pfx_from_emitter(effect) : 0;
    if (effect != 0 && particle != 0 &&
        _create_mkproc_generic_nostack(
            0x601B, 0x1F, p_watch_obj_for_gnd_coll,
            sizeof(NcsGroundCollisionWatchPdata),
            (MkHdr**)&watcher) != 0) {
        zero_pdata_payload(
            sizeof(NcsGroundCollisionWatchPdata), &watcher->hdr);
        angle = frand(3.1415927f);
        for (index = 0; index < 3; index++) {
            MkObj* glop = get_mkobj_frame(0x6015, 0);

            if (glop != 0) {
                Vec bone_at;
                Vec bone_right;
                float speed;

                effect = fx_next_emitter(effect);
                if (effect == 0) {
                    if (glop->hdr.instance != 0) {
                        glop->hdr.typed_vtbl->destroy(&glop->hdr);
                    }
                    return;
                }
                fx_resume_emit(effect);
                pfx_bind_emitter_num_to_obj(
                    particle, glop, 0, emitter_id_from_handle(effect));
                insert_particle_mkobj(glop);
                glop->flags_08_bits.airborne = 1;
                glop->flags_08_bits.gravity_enabled = 1;
                glop->flags_08_bits.moving = 1;
                get_bone_world_pos(source, bone, &glop->pos);
                mkobj_get_matrix_at(source, &bone_at);
                if (mode_of_play != 6 && player->f_constrained == 0 &&
                    (blood_type & 0x40) == 0) {
                    float offset;

                    if (is_plyr_airborn(source, player) != 0 ||
                        (source->flags_08 & 1) != 0 ||
                        (blood_type & 0x80) != 0) {
                        offset = -0.2f;
                        glop->pos.x += offset * bone_at.x;
                        glop->pos.y += offset * bone_at.y;
                        glop->pos.z += offset * bone_at.z;
                        glop->pos.y += 0.3f;
                    } else {
                        offset = -0.7f;
                        glop->pos.x += offset * bone_at.x;
                        glop->pos.y += offset * bone_at.y;
                        glop->pos.z += offset * bone_at.z;
                    }
                }
                mkobj_get_matrix_right(source, &bone_right);
                speed = 0.02f + frand(0.03f);
                glop->pos_vel.x = gxMathSin(angle) * speed;
                glop->pos_vel.y = frand(0.005f);
                speed = 0.02f + frand(0.03f);
                glop->pos_vel.z = gxMathCos(angle) * speed;
                glop->gravity = -0.002f;
                update_mkobj(glop);
                watcher->objects[index].object = glop;
                watcher->objects[index].instance = glop->hdr.instance;
                watcher->emitters[index] = effect;
                watcher->blood_owner = player;
                angle += 2.094393f;
            }
        }
        return;
    }

    if (watcher != 0) {
        for (index = 0; index < 3; index++) {
            MkObj* glop = watcher->objects[index].object;

            if (glop != 0 &&
                glop->hdr.instance == watcher->objects[index].instance &&
                glop->hdr.instance != 0) {
                glop->hdr.typed_vtbl->destroy(&glop->hdr);
            }
            if (watcher->emitters[index] != 0) {
                fx_reset_emit(watcher->emitters[index]);
            }
        }
        if (watcher->hdr.instance != 0) {
            watcher->hdr.typed_vtbl->destroy(&watcher->hdr);
        }
    }
}

/* Soft ceiling: retail retains additional loop state in saved registers. */
static float p_watch_obj_for_gnd_coll(void) {
    NcsGroundCollisionWatchPdata* pdata;
    int active_count;
    int index;

    active_count = 0;
    pdata = (NcsGroundCollisionWatchPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    for (index = 0; index < 3; index++) {
        FighterObjectRef* ref = &pdata->objects[index];
        MkObj* object = ref->object;

        if (object != 0 && object->hdr.instance != ref->instance) {
            object = 0;
        }
        if (object != 0) {
            active_count++;
            if (object->pos.y <= g_game_info.field_34) {
                fx_reset_emit(pdata->emitters[index]);
                object->pos.y = g_game_info.field_34 + 0.001f;
                spawn_bld_splat(
                    "bltrsh", pdata->blood_owner, &object->pos);
                ref->object = 0;
                ref->instance = 0;
                if (object->hdr.instance != 0) {
                    object->hdr.typed_vtbl->destroy(&object->hdr);
                }
                pdata->emitters[index] = 0;
            }
        }
    }
    return active_count != 0 ? 1.0f : -1.0f;
}

/* Soft ceiling: exact retail size; remaining delta is GPR allocation. */
void spawn_blood_pool_at_bid(
    NcsBloodPoolSource* source, int bone, int large) {
    Vec position;

    get_bone_world_pos(source->bone_owner, bone, &position);
    position.y = g_game_info.field_34 + 0.01f;
    if (large != 0) {
        spawn_bld_splat("blpuddle2", source->splat_owner, &position);
    } else {
        spawn_bld_splat("blpuddle", source->splat_owner, &position);
    }
}

/* Soft ceiling: exact retail size; remaining delta is GPR allocation. */
void mks_spawn_blood_pool_at_bid(
    NcsBloodPoolSource* source, MkObj* object, int bone, int large) {
    Vec position;
    if ((unsigned int)bone != 0x40000000) {
        if (object != 0) {
            get_bone_world_pos(object, bone, &position);
        } else {
            get_bone_world_pos(source->bone_owner, bone, &position);
        }
    } else {
        if (object != 0) {
            position.x = object->pos.x;
            position.y = object->pos.y;
            position.z = object->pos.z;
        } else {
            position.x = source->bone_owner->pos.x;
            position.y = source->bone_owner->pos.y;
            position.z = source->bone_owner->pos.z;
        }
    }
    position.y = g_game_info.field_34 + 0.01f;
    if (large != 0) {
        spawn_bld_splat("blpuddle2", source->splat_owner, &position);
    } else {
        spawn_bld_splat("blpuddle", source->splat_owner, &position);
    }
}

/* Soft ceiling: retail control flow is recovered; MWCC retains more saved registers here. */
void start_blood_splat_watcher(void) {
    int type;

    for (type = 0; type < 11; type++) {
        if (((1 << type) & 7) != 0) {
            unsigned int blood_effect;
            unsigned int decal_effect;

            blood_effect = fx_by_owner(
                mkpfx_ncs_blood_type_map_array[type],
                1 << g_game_info.plyr0.controller_slot);
            decal_effect = fx_by_owner(
                mkpfx_ncs_decal_array[0],
                1 << g_game_info.plyr0.controller_slot);
            if (blood_effect != 0 && decal_effect != 0) {
                fx_transfer(blood_effect, decal_effect);
            }

            blood_effect = fx_by_owner(
                mkpfx_ncs_blood_type_map_array[type],
                1 << g_game_info.plyr1.controller_slot);
            decal_effect = fx_by_owner(
                mkpfx_ncs_decal_array[0],
                1 << g_game_info.plyr1.controller_slot);
            if (blood_effect != 0 && decal_effect != 0) {
                fx_transfer(blood_effect, decal_effect);
            }
        }
    }
    start_decal_emitter_watcher();
}

void limb_sever_throw_away(
    NcsPlayerObjectRef* player, int bone, int include_children) {
    MkObj* object;

    object = obj_sever_limb(
        player->object, bone, 0, include_children);
    if (object != 0) {
        object->pos.x = -1000000.0f;
        object->pos.y = -1000000.0f;
        object->pos.z = -1000000.0f;
    }
}

MkObj* limb_sever_pop_head_up(
    PlyrInfo* player, float x_velocity, float y_velocity,
    float z_velocity, float gravity) {
    NcsLimbUpdatePdata* update;
    FighterMirror* fighter;
    FighterObjectRef* head_ref;
    MkObj* owner;
    MkObj* head;
    Vec local_velocity;
    Vec world_velocity;

    head = 0;
    update = limb_sever_find_existing_update_proc(player, 0, 0x6020);
    if (update == 0) {
        return 0;
    }

    local_velocity.x = x_velocity;
    local_velocity.y = y_velocity;
    local_velocity.z = z_velocity;
    owner = player->slot.mirror_a;
    v3_x_mat(&world_velocity, &local_velocity, mkobj_get_matrix(owner));

    if (owner == g_game_info.plyr0.slot.mirror_a) {
        fighter = g_game_info.plyr0.slot.fighter;
    } else {
        fighter = g_game_info.plyr1.slot.fighter;
    }
    head_ref = &fighter->severed_limbs[0];
    head = head_ref->object;
    if (head != 0 && head->hdr.instance != head_ref->instance) {
        head = 0;
    }
    if (head == 0) {
        head = obj_sever_limb(owner, 0, 0, 1);
        if (head == 0) {
            return 0;
        }
        head_ref->object = head;
        head_ref->instance = head->hdr.instance;
    }

    update->severed_mask |= 1;
    head->pos_vel.x = world_velocity.x;
    head->pos_vel.y = world_velocity.y;
    head->pos_vel.z = world_velocity.z;
    head->flags_08_bits.gravity_enabled = 1;
    head->gravity = gravity;
    if (gravity != 0.0f) {
        head->flags_08_bits.moving = 1;
    }
    head->light_flags = owner->light_flags;
    update->ground_mask |= 1;
    update->ground_height[0] = g_game_info.field_34 + 0.01f;
    update->ground_value[0] = 3;
    update->slide_end_coefficient = 1.0f;
    update->vertical_bounce_scale = 0.3f;
    update->field_18 = 0xD2;
    update_mkobj(head);
    limb_sever_show_z_meat_chunks(owner, 0, 0);
    return head;
}

void limb_sever_bone_attach(
    PlyrInfo* target_player, int owner_bone,
    const Vec* offset, const Vec* rotation,
    PlyrInfo* owner_player, int limb, int target_bone,
    int include_children) {
    FighterMirror* fighter;
    NcsLimbAttachPdata* pdata;
    MkProc* process;
    MkPtr* link;
    MkObj* severed;

    fighter = owner_player->slot.fighter;
    process = fighter->limb_update_proc;
    if (process != 0 &&
        process->instance != fighter->limb_update_proc_instance) {
        process = 0;
    }
    if (process != 0) {
        NcsLimbUpdatePdata* update =
            (NcsLimbUpdatePdata*)pdata_of_proc(process);

        if (update != 0) {
            update->severed_mask &= ~(1 << limb);
        }
    }

    pdata = 0;
    link = fighter->attach_proc_list;
    while (link != 0) {
        process = (MkProc*)link->hdr;
        if (link->instance != process->instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            NcsLimbAttachPdata* candidate = process != 0
                ? (NcsLimbAttachPdata*)pdata_of_proc(process) : 0;

            if (candidate != 0 && candidate->limb == limb) {
                pdata = candidate;
                break;
            }
            link = link->next;
        }
    }

    if (pdata == 0) {
        void* limbset = limb_sever_find_limbset(owner_player);

        if (limbset == 0) {
            return;
        }
        process = _create_mkproc_generic_nostack(
            0x6017, 0x1F, p_limb_sever_attach,
            sizeof(NcsLimbAttachPdata), (MkHdr**)&pdata);
        if (process == 0) {
            return;
        }
        zero_pdata_payload(sizeof(NcsLimbAttachPdata), &pdata->hdr);
        pdata->player = owner_player;
        pdata->fighter = fighter;
        pdata->limbset = limbset;
        mk_insert(&process->hdr, &fighter->attach_proc_list);
    }

    severed = fighter->severed_limbs[limb].object;
    if (severed != 0 &&
        severed->hdr.instance != fighter->severed_limbs[limb].instance) {
        severed = 0;
    }
    if (severed == 0) {
        severed = obj_sever_limb(
            owner_player->slot.mirror_a, limb, 0, include_children);
        if (severed == 0) {
            if (pdata->hdr.instance != 0) {
                pdata->hdr.typed_vtbl->destroy(&pdata->hdr);
            }
            return;
        }
    }

    obj_set_bone_calc_world_mat_flag(
        target_player->slot.mirror_a, owner_bone);
    pdata->offset.x = offset->x;
    pdata->offset.y = offset->y;
    pdata->offset.z = offset->z;
    pdata->rotation.x = rotation->x;
    pdata->rotation.y = rotation->y;
    pdata->rotation.z = rotation->z;
    severed->flags_08_bits.airborne = 1;
    severed->flags_08_bits.gravity_enabled = 0;
    severed->flags_08_bits.moving = 0;
    severed->light_flags = owner_player->slot.mirror_a->light_flags;
    pdata->owner = owner_player->slot.mirror_a;
    pdata->owner_instance = pdata->owner->hdr.instance;
    pdata->target = target_player->slot.mirror_a;
    pdata->target_instance = pdata->target->hdr.instance;
    fighter->severed_limbs[limb].object = severed;
    fighter->severed_limbs[limb].instance = severed->hdr.instance;
    pdata->limb = limb;
    pdata->target_bone = target_bone;
    pdata->owner_bone = owner_bone;
    pdata->expire_tick = 600;
}

static inline MkObj* limb_sever_set_motion_inline(
    MkObj* owner, int limb, const Vec* velocity,
    NcsLimbMotion* motion, int enable_ground,
    int ground_value, int field_18, int include_children,
    float gravity, float ground_offset, float field_11C) {
    FighterMirror* fighter;
    FighterObjectRef* ref;
    MkObj* severed;

    if (motion == 0) {
        return 0;
    }
    if (owner == g_game_info.plyr0.slot.mirror_a) {
        fighter = g_game_info.plyr0.slot.fighter;
    } else {
        fighter = g_game_info.plyr1.slot.fighter;
    }
    ref = &fighter->severed_limbs[limb];
    severed = ref->object;
    if (severed != 0 && severed->hdr.instance != ref->instance) {
        severed = 0;
    }
    if (severed == 0) {
        severed = obj_sever_limb(owner, limb, 0, include_children);
        if (severed == 0) {
            return 0;
        }
        ref->object = severed;
        ref->instance = severed->hdr.instance;
    }

    motion->severed_mask |= 1 << limb;
    severed->pos_vel.x = velocity->x;
    severed->pos_vel.y = velocity->y;
    severed->pos_vel.z = velocity->z;
    severed->flags_08_bits.gravity_enabled = 1;
    severed->gravity = gravity;
    if (gravity != 0.0f) {
        severed->flags_08_bits.moving = 1;
    }
    severed->light_flags = owner->light_flags;
    if (enable_ground != 0) {
        motion->ground_mask |= 1 << limb;
        motion->ground_height[limb] = g_game_info.field_34 + ground_offset;
        motion->ground_value[limb] = ground_value;
        motion->field_138 = 1.0f;
    }
    motion->field_11C = field_11C;
    motion->field_18 = field_18;
    update_mkobj(severed);
    return severed;
}

static inline MkObj* mks_limb_sever_inline(
    MkObj* object, int limb, int include_children) {
    FighterMirror* fighter;
    FighterObjectRef* severed_ref;
    MkObj* severed;

    if (object == g_game_info.plyr0.slot.mirror_a) {
        fighter = g_game_info.plyr0.slot.fighter;
    } else {
        fighter = g_game_info.plyr1.slot.fighter;
    }
    severed_ref = &fighter->severed_limbs[limb];
    severed = severed_ref->object;
    if (severed != 0 && severed->hdr.instance != severed_ref->instance) {
        severed = 0;
    }
    if (severed == 0) {
        severed = obj_sever_limb(object, limb, 0, include_children);
        if (severed != 0) {
            severed_ref->object = severed;
            severed_ref->instance = severed->hdr.instance;
            severed->light_flags = object->light_flags;
        }
    }
    return severed;
}

void limb_sever_explode_apart_plyr_num(int player) {
    if (player == 0) {
        limb_sever_explode_apart(&g_game_info.plyr0);
    } else if (player == 1) {
        limb_sever_explode_apart(&g_game_info.plyr1);
    }
}

/*
 * Soft ceiling: retail expands both typed limb helpers at every call site.
 * The remaining size delta is late inline/DCE behavior plus register and
 * instruction scheduling; the complete limb order and motion are recovered.
 */
void limb_sever_explode_apart(PlyrInfo* player) {
    MkObj* owner;
    NcsLimbUpdatePdata* update;
    MKMATRIX* limb_matrix;
    Vec local_velocity;
    Vec world_velocity;
    Vec angular_velocity = {0.1f, 0.0f, 0.0f};
    MkObj* severed;

    owner = player->slot.mirror_a;
    limb_matrix = force_calc_bone_world_mat(owner, 9);
    update = limb_sever_find_existing_update_proc(player, -1, 0x6014);
    if (update == 0) {
        return;
    }
    init_plyr_severed_limb_list(player);
    local_velocity.x = 0.05f;
    local_velocity.y = 0.07f;
    local_velocity.z = 0.02f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 4, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 4, 0);

    local_velocity.x = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 5, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 5, 0);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 6, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 6, 0);

    local_velocity.x = -0.01f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.01f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 1, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 1, 0);

    local_velocity.x = -0.095f;
    local_velocity.y = 0.012f;
    local_velocity.z = -0.03f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 2, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 2, 0);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 3, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 2, 0);

    local_velocity.x = 0.05f;
    local_velocity.y = -0.02f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 10, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 10, 0);

    local_velocity.x = 0.035f;
    local_velocity.y = 0.02f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 11, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 11, 0);

    local_velocity.x = 0.065f;
    local_velocity.y = -0.02f;
    local_velocity.z = 0.02f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 12, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 12, 0);

    local_velocity.x = -0.05f;
    local_velocity.y = 0.08f;
    local_velocity.z = -0.04f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 7, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 7, 0);

    local_velocity.x = -0.08f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 8, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 8, 0);

    local_velocity.x = -0.03f;
    local_velocity.y = 0.03f;
    local_velocity.z = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 9, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 9, 0);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, mkobj_get_matrix(owner));
    severed = limb_sever_set_motion_inline(
        owner, 0, &world_velocity, (NcsLimbMotion*)update,
        1, 2, 0xD2, 1, -0.006f, 0.1f, 0.0001f);
    zero_v3(&angular_velocity);
    angular_velocity.z = 0.085f;
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 0, 0);

    severed = mks_limb_sever_inline(owner, 14, 1);
    {
        Vec hidden_position;

        zero_v3(&hidden_position);
        hidden_position.y = -1000.0f;
        obj_set_pos(severed, &hidden_position);
        limb_sever_show_z_meat_chunks(owner, 14, 0);
    }

    mks_limb_sever_inline(owner, 13, 1);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.06f;
    local_velocity.z = 0.04f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    obj_set_pos_vel(owner, &world_velocity);

    severed = limb_sever_set_motion_inline(
        owner, 13, &world_velocity, (NcsLimbMotion*)update,
        1, 3, 1000, 1, -0.006f, 0.01f, 0.3f);
    zero_v3(&angular_velocity);
    angular_velocity.z = 0.1f;
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 13, 0);
}

/* Soft ceiling: retail keeps the limb index and cache base in separate registers. */
MkObj* mks_limb_sever(
    MkObj* object, int limb, int include_children) {
    FighterMirror* fighter;
    FighterObjectRef* severed_ref;
    MkObj* severed;

    if (object == g_game_info.plyr0.slot.mirror_a) {
        fighter = g_game_info.plyr0.slot.fighter;
    } else {
        fighter = g_game_info.plyr1.slot.fighter;
    }
    severed_ref = &fighter->severed_limbs[limb];
    severed = severed_ref->object;
    if (severed != 0) {
        if (severed->hdr.instance != severed_ref->instance) {
            severed = 0;
        }
    } else {
        severed = 0;
    }
    if (severed == 0) {
        severed = obj_sever_limb(object, limb, 0, include_children);
        if (severed != 0) {
            severed_ref->object = severed;
            severed_ref->instance = severed->hdr.instance;
            severed->light_flags = object->light_flags;
        }
    }
    return severed;
}

/* Soft ceiling: remaining delta is MWCC save/restore and register scheduling. */
void limb_sever_destroy_existing_attach_proc(
    PlyrInfo* player, int limb) {
    MkPtr** list;
    MkPtr* link;

    /* Retail also preserves this address check before loading the list head. */
    list = &player->slot.fighter->attach_proc_list;
    if (list != 0) {
        link = *list;
        while (link != 0) {
            MkProc* proc = (MkProc*)link->hdr;

            if (link->instance != proc->instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (proc != 0) {
                    NcsLimbAttachPdata* pdata =
                        (NcsLimbAttachPdata*)pdata_of_proc(proc);

                    if (pdata != 0 && pdata->limb == limb) {
                        if (proc->instance != 0) {
                            proc->hdr.typed_vtbl->destroy(&proc->hdr);
                        }
                        break;
                    }
                }
                link = link->next;
            }
        }
    }
}

NcsLimbUpdatePdata* limb_sever_find_existing_update_proc(
    PlyrInfo* player, int limb, int proc_id) {
    FighterMirror* fighter;
    MkPtr* link;
    MkProc* update_proc;
    NcsLimbUpdatePdata* pdata;

    fighter = player->slot.fighter;
    link = fighter->attach_proc_list;
    while (link != 0) {
        MkProc* proc = (MkProc*)link->hdr;

        if (link->instance != proc->instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            NcsLimbAttachPdata* attach_pdata = proc != 0
                ? (NcsLimbAttachPdata*)pdata_of_proc(proc) : 0;

            if (attach_pdata != 0 && attach_pdata->limb == limb) {
                if (proc->instance != 0) {
                    proc->hdr.typed_vtbl->destroy(&proc->hdr);
                }
                break;
            }
            link = link->next;
        }
    }

    update_proc = fighter->limb_update_proc;
    if (update_proc != 0 &&
        update_proc->instance != fighter->limb_update_proc_instance) {
        update_proc = 0;
    }
    pdata = update_proc != 0
        ? (NcsLimbUpdatePdata*)pdata_of_proc(update_proc) : 0;
    if (pdata == 0) {
        void* limbset = limb_sever_find_limbset(player);

        if (limbset == 0) {
            return 0;
        }
        update_proc = _create_mkproc_generic_nostack(
            proc_id, 0x1F, p_limb_sever_update,
            sizeof(NcsLimbUpdatePdata), (MkHdr**)&pdata);
        if (update_proc == 0) {
            return 0;
        }
        zero_pdata_payload(sizeof(NcsLimbUpdatePdata), &pdata->hdr);
        pdata->player = player;
        pdata->fighter = fighter;
        pdata->limbset = limbset;
        fighter->limb_update_proc = update_proc;
        fighter->limb_update_proc_instance = update_proc->instance;
    }
    pdata->expire_tick = exec_tick_ctr + 60;
    return pdata;
}

MkProc* plyr_spawn_his_anim_limb(
    NcsLimbOwner* player, int limb, int include_children,
    void* animation_script, int transition, MkProcEntryFn entry,
    float animation_step) {
    AnimPdata* animation;
    MkProc* proc;
    MkObj* severed;

    animation = 0;
    proc = (MkProc*)create_mkproc_anim2(0x9012, entry, &animation);
    if (proc == 0) {
        return 0;
    }

    severed = obj_sever_limb(
        player->object, limb, 0, include_children);
    if (severed == 0) {
        if (proc->instance != 0) {
            ((NcsDestroyable*)proc)->vtbl->destroy((NcsDestroyable*)proc);
        }
        return 0;
    }

    severed->ground_colls = plyr_obj->ground_colls;
    severed->ground_colls_y = plyr_obj->ground_colls_y;
    severed->flags_09_bits.launched = 1;
    insert_ground_me_mkobj(plyr_obj);
    severed->light_flags = player->object->light_flags;
    animation->obj = severed;
    animation->obj_instance = severed->hdr.instance;
    player->collection->limbs[limb].obj = &severed->hdr;
    player->collection->limbs[limb].obj_instance =
        severed->hdr.instance;
    severed->ground_colls = player->object->ground_colls;
    severed->ground_colls_y = player->object->ground_colls_y;
    severed->flags_0B &= (unsigned char)~4;
    set_root_and_obj_movement_weights(0.0f, 1.0f, animation);
    set_anim_script(animation, animation_script, transition);
    animation->step = animation_step;
    return proc;
}

MkObj* limb_sever_set_motion(
    MkObj* owner, int limb, const Vec* velocity,
    NcsLimbMotion* motion, int enable_ground,
    int ground_value, int field_18, int include_children,
    float gravity, float ground_offset, float field_11C) {
    return limb_sever_set_motion_inline(
        owner, limb, velocity, motion, enable_ground, ground_value,
        field_18, include_children, gravity, ground_offset, field_11C);
}

static float p_limb_sever_attach(void) {
    NcsLimbAttachPdata* pdata;
    FighterObjectRef* severed_ref;
    MkObj* severed;
    MkObj* owner;
    MkObj* target;
    MKMATRIX rotation_matrix;
    MKMATRIX* severed_matrix;
    MKMATRIX* target_matrix;
    MKMATRIX* source_matrix;
    Vec offset;
    Vec source_position;

    pdata = (NcsLimbAttachPdata*)apdata;
    if (pdata == 0) {
        mkproc_die();
    }

    severed_ref = &pdata->fighter->severed_limbs[pdata->limb];
    severed = severed_ref->object;
    if (severed != 0 && severed->hdr.instance != severed_ref->instance) {
        severed = 0;
    }
    if (severed == 0) {
        return -1.0f;
    }
    owner = pdata->owner;
    if (owner != 0 && owner->hdr.instance != pdata->owner_instance) {
        owner = 0;
    }
    if (owner == 0) {
        return -1.0f;
    }
    target = pdata->target;
    if (target != 0 && target->hdr.instance != pdata->target_instance) {
        target = 0;
    }
    if (target == 0) {
        return -1.0f;
    }
    if ((((NcsLimbSet*)pdata->limbset)->active_mask &
         (1U << pdata->limb)) == 0) {
        if (severed->hdr.instance != 0) {
            severed->hdr.typed_vtbl->destroy(&severed->hdr);
        }
        return -1.0f;
    }
    if (pdata->expire_tick < 0) {
        return -1.0f;
    }

    target_matrix = (MKMATRIX*)target->field_24;
    severed_matrix = (MKMATRIX*)severed->field_24;
    if (pdata->owner_bone >= 0) {
        MkBone* target_bone = target->bones[pdata->owner_bone];

        if (target_bone != 0 && target_bone->parent_matrix != 0) {
            target_matrix = (MKMATRIX*)target_bone->parent_matrix;
        }
    }
    YXZ_angles_to_MKMATRIX(&pdata->rotation, &rotation_matrix);
    mat_x_mat(severed_matrix, &rotation_matrix, target_matrix);
    if ((severed->flags_08 & 2) != 0) {
        mat_scaled_by_v3(
            severed_matrix, severed_matrix, &severed->scale);
    }

    source_matrix = (MKMATRIX*)severed
        ->bones[limbbid_bid_map[pdata->target_bone]->bone_id]->parent_matrix;
    source_position.x = source_matrix->pos.x;
    source_position.y = source_matrix->pos.y;
    source_position.z = source_matrix->pos.z;
    offset.x = pdata->offset.x - source_position.x;
    offset.y = pdata->offset.y - source_position.y;
    offset.z = pdata->offset.z - source_position.z;
    v3_x_mat_add_v3(
        &severed->pos, &offset, target_matrix, &source_position);
    severed_matrix->pos.x = severed->pos.x;
    severed_matrix->pos.y = severed->pos.y;
    severed_matrix->pos.z = severed->pos.z;
    update_mkobj(severed);
    pdata->expire_tick--;
    return 1.0f;
}

static float p_limb_sever_update(void) {
    NcsLimbUpdatePdata* pdata;
    NcsLimbSet* limbset;
    int limb;

    pdata = (NcsLimbUpdatePdata*)apdata;
    if (pdata == 0) {
        mkproc_die();
    }
    limbset = (NcsLimbSet*)pdata->limbset;
    if (pdata->field_18 < 0 || limbset == 0) {
        return -1.0f;
    }

    for (limb = 0; limb < 15; limb++) {
        unsigned int bit = 1U << limb;

        if ((pdata->severed_mask & bit) != 0 &&
            (limbset->active_mask & bit) != 0) {
            FighterObjectRef* ref = &pdata->fighter->severed_limbs[limb];
            MkObj* object = ref->object;

            if (object != 0 && object->hdr.instance != ref->instance) {
                object = 0;
            }
            if (object == 0) {
                continue;
            }

            if ((pdata->gravity_trigger_mask & bit) != 0 &&
                object->pos_vel.y < pdata->horizontal_bounce_scale) {
                pdata->gravity_trigger_mask &= ~bit;
                object->gravity = pdata->bounce_gravity;
            }
            if ((pdata->x_collision_mask & bit) != 0 &&
                ((object->pos_vel.x >= 0.0f &&
                  object->pos.x > pdata->x_collision_limit[limb]) ||
                 (object->pos_vel.x < 0.0f &&
                  object->pos.x < pdata->x_collision_limit[limb]))) {
                object->pos_vel.x *= -1.0f;
                pdata->x_collision_mask &= ~bit;
                if ((pdata->damp_bounce_mask & bit) != 0) {
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.x *= pdata->horizontal_bounce_scale;
                }
            }
            if ((pdata->z_collision_mask & bit) != 0 &&
                ((object->pos_vel.z >= 0.0f &&
                  object->pos.z > pdata->z_collision_limit[limb]) ||
                 (object->pos_vel.z < 0.0f &&
                  object->pos.z < pdata->z_collision_limit[limb]))) {
                object->pos_vel.z *= -1.0f;
                pdata->z_collision_mask &= ~bit;
                if ((pdata->damp_bounce_mask & bit) != 0) {
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.z *= pdata->horizontal_bounce_scale;
                }
            }

            if ((pdata->ground_mask & bit) == 0 ||
                object->pos.y > pdata->ground_height[limb]) {
                object->pos_vel.x *= pdata->slide_end_coefficient;
                object->pos_vel.z *= pdata->slide_end_coefficient;
            } else if (pdata->ground_value[limb] != 0) {
                int bounce_count = pdata->ground_value[limb];

                object->pos_vel.y =
                    (float)bounce_count *
                    (-object->pos_vel.y * pdata->vertical_bounce_scale);
                pdata->ground_value[limb] = bounce_count - 1;
                random_hit(0x0B);
                if ((pdata->first_bounce_mask & bit) != 0) {
                    pdata->first_bounce_mask &= ~bit;
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.x *= pdata->horizontal_bounce_scale;
                    object->pos_vel.z *= pdata->horizontal_bounce_scale;
                }
            } else {
                int make_splat = 1;

                object->pos.y = pdata->ground_height[limb];
                object->pos_vel.y = 0.0f;
                object->flags_08_bits.rotation_enabled = 0;
                object->flags_08_bits.moving = 0;
                object->flags_09_bits.launched = 1;
                object->flags_09_bits.bit6 = 1;
                if ((pdata->slide_mask & bit) != 0) {
                    if (object->pos_vel.x > 0.0f) {
                        object->pos_vel.x -= pdata->slide_deceleration;
                        if (object->pos_vel.x < 0.0f) {
                            object->pos_vel.x = 0.0f;
                        }
                    } else {
                        object->pos_vel.x += pdata->slide_deceleration;
                        if (object->pos_vel.x > 0.0f) {
                            object->pos_vel.x = 0.0f;
                        }
                    }
                    if (object->pos_vel.z > 0.0f) {
                        object->pos_vel.z -= pdata->slide_deceleration;
                        if (object->pos_vel.z < 0.0f) {
                            object->pos_vel.z = 0.0f;
                        }
                    } else {
                        object->pos_vel.z += pdata->slide_deceleration;
                        if (object->pos_vel.z > 0.0f) {
                            object->pos_vel.z = 0.0f;
                        }
                    }
                    if (object->pos_vel.x == 0.0f &&
                        object->pos_vel.z == 0.0f) {
                        pdata->slide_mask &= ~bit;
                    } else {
                        make_splat = 0;
                    }
                } else {
                    object->flags_08_bits.gravity_enabled = 0;
                }

                if (make_splat != 0) {
                    Vec position;

                    position.x = object->pos.x;
                    position.y = g_game_info.field_34 + 0.0005f;
                    position.z = object->pos.z;
                    spawn_bld_splat("blpuddle", pdata->fighter, &position);
                    if ((unsigned int)g_game_info.field_218 <
                            (unsigned int)exec_tick_ctr &&
                        (unsigned int)exec_tick_ctr <
                            (unsigned int)pdata->expire_tick) {
                        random_hit(0x0C);
                        g_game_info.field_218 =
                            exec_tick_ctr + randu0(10) + 15;
                    }
                    pdata->ground_mask &= ~bit;
                }
            }
        }
    }
    pdata->field_18--;
    return 1.0f;
}

void animpdata_ani_to_frame_x_with_flag_check(
    AnimPdata* animation, int zone_check, int grab_check,
    float target_frame) {
    NcsProcVtable* proc_vtbl;

    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    while (animation->frame <= target_frame) {
        if (cur_zone_check == zone_check &&
            cur_grab_check != grab_check) {
            break;
        }
        advance_anim(animation);
        pose_anim(animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        proc_vtbl = (NcsProcVtable*)aproc->vtbl;
        proc_vtbl->sleep(proc_vtbl);
        if (animation->step * game_speed + animation->frame >
            target_frame) {
            break;
        }
    }
}

void mkscripts_set_anim_check_flag(int zone_check, int grab_check) {
    cur_zone_check = zone_check;
    cur_grab_check = grab_check;
}

void mkscripts_mkobj_insert_mkobj_cleanuplist(
    MkHdr* object, MkObj* owner) {
    mk_insert(object, &owner->child_list);
}

void mkscripts_destroy_fk_bonematcher(NcsDestroyable* bonematcher) {
    if (bonematcher->instance != 0) {
        bonematcher->vtbl->destroy(bonematcher);
    }
}

void mkscripts_destroy_bonematcher(NcsDestroyable* bonematcher) {
    if (bonematcher->instance != 0) {
        bonematcher->vtbl->destroy(bonematcher);
    }
}

void mkscripts_destroy_gusher(NcsDestroyable* gusher) {
    if (gusher->instance != 0) {
        gusher->vtbl->destroy(gusher);
    }
}

void play_his_snd_req(int sound) {
    PlyrPdata* player;

    player = plyr_pdata;
    plyr_pdata = player->his_plyr_pdata;
    plyr_snd_req(sound);
    plyr_pdata = plyr_pdata->his_plyr_pdata;
}

void play_his_random_voice(int sound) {
    PlyrPdata* player;

    player = plyr_pdata;
    plyr_pdata = player->his_plyr_pdata;
    random_voice(sound);
    plyr_pdata = plyr_pdata->his_plyr_pdata;
}

int pfx_plyr_bankowner(const PfxPlayerBankOwner* player) {
    return 1 << player->player_index;
}

/*
 * Soft ceiling: retail keeps this loop rolled. At this TU's authentic -O4,p
 * settings MWCC unrolls it; function-local unroll pragmas are intentionally
 * avoided, so the portable loop is retained despite the emission gap.
 */
void limb_sever_update_slide_end_coeff(
    NcsLimbSlide* limb, float coefficient) {
    int index;

    if (limb != 0) {
        limb->slide_end_coefficient = coefficient;
        for (index = 0; index < 15; index++) {
            limb->update_flags |= 1 << index;
        }
    }
}

MkHdr* proc_of_anim_pdata(MkObjLatch* data) {
    MkHdr* proc;

    proc = data->obj;
    if (proc != 0) {
        if (proc->instance == data->obj_instance) {
            /* The instance latch still identifies this process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    return proc;
}

void set_pdata_anim_step(AnimPdata* pdata, float step) {
    pdata->step = step;
}

float mkobj_pos_pos_dot_normal_xz(
    const MkObj* from, const MkObj* to, const Vec* normal) {
    NcsFloatBits bits;
    float dx;
    float dz;
    float squared;
    float estimate;
    float product;
    float correction;
    float inverse_length;

    dx = to->pos.x - from->pos.x;
    dz = to->pos.z - from->pos.z;
    squared = dx * dx + dz * dz;
    /*
     * Soft ceiling: retail retains an empty zero-length branch as a separate
     * jump and schedules the argument loads after the stack-frame setup.
     * Clean MWCC folds the empty branch and hoists those loads. The arithmetic,
     * including the unordered/NaN path, is otherwise instruction-equivalent.
     */
    inverse_length = 0.0f;
    if (squared <= 0.0f) {
        /* A zero-length XZ direction has no normalized component. */
    } else {
        bits.f = squared;
        bits.u = 0x5F375A00 - (bits.u >> 1);
        estimate = bits.f;
        product = estimate * (squared * estimate);
        correction = 3.0f - product;
        inverse_length =
            0.0625f * estimate * correction *
            -(correction * (product * correction) - 12.0f);
    }
    return normal->x * (dx * inverse_length) +
           normal->z * (dz * inverse_length);
}

void ncs_script_debug_quickie(int command, float value) {
    if (command != -1) {
        return;
    }
    if (value == -1.0f) {
        return;
    }
}
