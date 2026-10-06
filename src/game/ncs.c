#include "game/ncs.h"
#include "game/projectile.h"
#include "game/ejb.h"
#include "game/bgnd.h"
#include "game/pfxscript.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_obj_bone.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/image.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pebble.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/asset.h"
#include "runtime/light.h"
#include "runtime/section.h"
#include "runtime/cam.h"
#include "game/game_info.h"
#include "game/plyr.h"
#include "game/plyr_globals.h"
#include "platform/display.h"
#include "platform/display_metrics.h"
#include "platform/main.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "rw/rtquat.h"
#include "runtime/cstring.h"
#include "runtime/utils.h"
#include "runtime/sound.h"

extern LightDef* pbl_gore2_lights[3];
extern MkPtr* gore2_light_list;
extern int blood_type_list[12];

static void trigger_blood_glops(
    PlyrPdata* player, int bone, MkObj* source, int blood_type);
MkObj* obj_sever_limb(
    MkObj* object, int limb, Vec* limb_velocities, int include_children);

extern int f_fatality_was_done;
extern PlyrPdata* his_pdata;

struct NcsBoneMatcher {
    MkHdr hdr;
    unsigned char inactive : 1;
    unsigned char copy_bone_matrix : 1;
    unsigned char flags_low : 6;
    char pad09[7];
    MkObj* parent;
    unsigned int parent_instance;
    char pad18[0x2C];
    float blend;
};

struct NcsLimbSet;

struct NcsLimbAttachPdata {
    MkHdr hdr;
    PlyrInfo* player;
    FighterMirror* fighter;
    struct NcsLimbSet* limbset;
    MkObj* owner;
    unsigned int owner_instance;
    MkObj* target;
    unsigned int target_instance;
    int limb;
    int target_bone;
    int owner_bone;
    Vec offset;
    Vec rotation;
    int expire_tick;
};

struct NcsLimbUpdatePdata {
    MkHdr hdr;
    PlyrInfo* player;
    FighterMirror* fighter;
    struct NcsLimbSet* limbset;
    int severed_mask;
    int field_18;
    int expire_tick;
    int x_collision_mask;
    int z_collision_mask;
    float x_collision_limit[15];
    float z_collision_limit[15];
    int ground_mask;
    float ground_height[15];
    int ground_value[15];
    float vertical_bounce_scale;
    float horizontal_bounce_scale;
    float bounce_gravity;
    int gravity_trigger_mask;
    int first_bounce_mask;
    int damp_bounce_mask;
    int slide_mask;
    float slide_end_coefficient;
    float slide_deceleration;
};

struct NcsLimbSet {
    char pad00[0x780];
    unsigned int active_mask;
};

struct SpearProcFlagBits {
    unsigned char bit7 : 1;
    unsigned char no_hit_reaction : 1;
    unsigned char getup : 1;
    unsigned char victory : 1;
    unsigned char pad_3_0 : 4;
};

struct SpearProcPdata {
    MkHdr hdr;
    PlyrProcLatch player_proc_latch;
    PlyrPdata* opponent_pdata;
    MkObj* opponent_object;
    union {
        unsigned int flags;
        struct SpearProcFlagBits flag_bits;
    };
    PlyrPdata* owner;
    MkObj* spear_object;
    unsigned int spear_object_instance;
    struct NcsBoneMatcher* bonematcher;
    MkHdr* bound_object;
    unsigned int bound_object_instance;
    int field_34;
    int blocked_ticks;
    struct NcsSpearEffect* effect;
    unsigned int effect_instance;
    int field_44;
};

struct NcsSpearEffect {
    MkHdr hdr;
    unsigned char destroyed : 1;
    unsigned char owns_bind : 1;
    unsigned char flags_bit5 : 1;
    unsigned char visible : 1;
    unsigned char flags_low : 4;
    char pad09[0x1F];
    float field_28;
    char pad2C[0x14];
    PfxVm vm;
    char pad280[8];
    int active;
    int bone;
    int field_290;
    char pad294[4];
    float field_298;
    float field_29C;
    float field_2A0;
    float field_2A4;
    float field_2A8;
    char pad2AC[0x0C];
    struct SpearProcPdata* spear_pdata;
    unsigned int spear_pdata_instance;
};

struct PrisonGrabPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float target_x;
    float pad14;
    float target_z;
    float current_x;
    float current_y;
    float current_z;
    int done;
    float target_angle;
    int aligned;
};

struct NcsKonquestCharacterPdata {
    MkHdr hdr;
    char pad08[4];
    MkObj* characters[8];
};

struct Gore2ObjectType {
    unsigned int object_id;
    int particle_count;
    float scale;
};

struct NcsGroundCollisionMap {
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
};

#define NCS_GROUND_COLLISION_MAP(bone_, radius_) \
    { (bone_), 0, 0, 0, (radius_), -1, 0, 0, 0, 0 }

struct NcsGroundCollisionMap LID_HEAD_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x10, 0.07f);
struct NcsGroundCollisionMap LID_HAND_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x25, 0.04f);
struct NcsGroundCollisionMap LID_FOREARM_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x17, 0.07f);
struct NcsGroundCollisionMap LID_ARM_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x13, 0.07f);
struct NcsGroundCollisionMap LID_HAND_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x24, 0.04f);
struct NcsGroundCollisionMap LID_FOREARM_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x16, 0.07f);
struct NcsGroundCollisionMap LID_ARM_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x12, 0.07f);
struct NcsGroundCollisionMap LID_FOOT_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x0B, 0.05f);
struct NcsGroundCollisionMap LID_CALF_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x08, 0.07f);
struct NcsGroundCollisionMap LID_THIGH_RIGHT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x05, 0.07f);
struct NcsGroundCollisionMap LID_FOOT_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x0A, 0.05f);
struct NcsGroundCollisionMap LID_CALF_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x07, 0.07f);
struct NcsGroundCollisionMap LID_THIGH_LEFT_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x04, 0.07f);
struct NcsGroundCollisionMap LID_PELVIS_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x00, 0.14f);
struct NcsGroundCollisionMap LID_TORSO_ground_collision_map =
    NCS_GROUND_COLLISION_MAP(0x03, 0.12f);

struct NcsGroundCollisionMap* limbbid_bid_map[15] = {
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

static struct Gore2ObjectType pbl_gore2_obj_list[10] = {
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

struct NcsAmbientLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
};

struct NcsDirectLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
    float field_1C;
    float field_20;
    float field_24;
};


struct NcsAmbientLightDef pbl_gore2_ambient_light = {
    1, 0, 3, {0.1f, 0.1f, 0.1f, 0.0f},
};

struct NcsDirectLightDef pbl_gore2_direct_light = {
    3, p_track_cam_ang_y_light, 1,
    {0.75f, 0.75f, 0.75f, 1.0f},
    0.6f, 2.89f, 0.0f,
};

LightDef* pbl_gore2_lights[3] = {
    (LightDef*)&pbl_gore2_ambient_light,
    (LightDef*)&pbl_gore2_direct_light,
    0,
};

struct Gore2FlagBits {
    signed char settled : 1;
    signed char has_rotation : 1;
    signed char has_scale : 1;
    signed char has_translation : 1;
    signed char attached : 1;
    signed char pad_high : 3;
    unsigned char pad[3];
};

union Gore2Flags {
    unsigned int word;
    struct Gore2FlagBits bits;
};

struct Gore2Particle {
    union Gore2Flags flags;
    Vec rotation;
    Vec scale;
    Vec translation;
    float vertical_acceleration;
    int bounce_count;
    float bounce_scale;
    FighterObjectRef owner;
    int bone;
    FighterMirror* decal_owner;
};

struct Gore2UpdatePdata {
    MkHdr hdr;
    PebbleData* pools[10];
    int next_particle[10];
};

static MkObj* sc_spear_obj;
static struct SpearProcPdata* pdata_sc_spear;
struct Gore2UpdatePdata* mkpdata_pbl_gore2_update;
int cur_grab_check;
int cur_zone_check;

struct NcsGroundCollisionWatchPdata {
    MkHdr hdr;
    PlyrPdata* blood_owner;
    FighterObjectRef objects[3];
    int emitters[3];
};

struct NcsCameraWallRegion {
    int type;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    char pad14[0x0C];
    const int* hide_ids;
    const int* show_ids;
    const int* alpha_ids;
};

struct NcsCameraWallPdata {
    MkHdr hdr;
    NcsCameraWallRegion* regions;
    MkObj* characters[8];
    int active_region;
    int special_alpha_initialized;
};

void plyr_aux_weapon_grab(PlyrPdata* player, MkObj* item);
void insert_ground_me_mkobj(void* object);
void fx_transfer(unsigned int handle, unsigned int owner);
void start_decal_emitter_watcher(void);
void fx_resume_emit(unsigned int handle);
int am_i_on_the_left2(MkObj* opponent, MkObj* me);
void mkobj_get_matrix_at(MkObj* object, Vec* out);
void mkobj_get_matrix_right(MkObj* object, Vec* out);
void* limb_sever_find_limbset(PlyrInfo* player);
void trial_state_collision_check(int collision_result, int player);
void pz_fighter_reaction_xfer_him(int reaction);
int reaction_xfer_him(int reaction, float rate, int strength);
struct NcsBoneMatcher* start_bone_matcher(
    MkObj* parent, int parent_bone, MkObj* child, int child_bone,
    float blend_ticks);
void plyr_aux_weapon_release(PlyrPdata* player);
void hide_atomic(void* atomic);
void unhide_atomic(void* atomic);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void random_hit(int hit);
MKMATRIX* force_calc_bone_world_mat(MkObj* object, int bone);
void init_plyr_severed_limb_list(PlyrInfo* player);
void obj_set_ang_vel(MkObj* object, void* velocity);
void obj_set_pos_vel(MkObj* object, void* velocity);
void spawn_decal_emitter(
    const char* name, FighterMirror* owner, const Vec* position,
    const MKMATRIX* orientation, float angle);
MKMATRIX* mkobj_get_matrix(MkObj* object);
void limb_sever_show_z_meat_chunks(
    MkObj* object, int limb, int include_children);
MkProc* fire_sc_spear(
    PlyrPdata* player, Vec* velocity, int field_34,
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
struct NcsLimbUpdatePdata* limb_sever_find_existing_update_proc(
    PlyrInfo* player, int limb, int proc_id);
void limb_sever_explode_apart(
    PlyrInfo* player, float arg1, float arg2, float strength, int mode);
void spawn_bld_splat(
    const char* name, void* owner, const Vec* position);
MslSoundHandle plyr_snd_req(int sound);
MslSoundHandle random_voice(int sound);

static float p_mkpfx_fadingrun(void);
float p_sc_spear1(void);
static void sc_spear_prewake(void);
void sc_spear_postsleep(void);
static float p_prison_grab(void);

void start_mkpfx_FadeSnapShot(void) {

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
    fading_screen.snapshotTex->raster->originalWidth = screen_width;
    fading_screen.snapshotTex->raster->originalHeight = screen_height;
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
    Pfx2dObj* quad = fading_screen.fade_obj->pfx2d;

    fading_screen.alpha -= 4.0f * game_speed;
    if (fading_screen.alpha <= 0.0f) {
        DeleteCameraSnapShot();
        return -1.0f;
    }

    quad->verts[0].a =
        fading_screen.alpha;
    quad->verts[1].a =
        fading_screen.alpha;
    quad->verts[2].a =
        fading_screen.alpha;
    quad->verts[3].a =
        fading_screen.alpha;
    return 1.0f;
}

MkProc* start_scorpion_spear(int field_34) {
    Vec velocity;
    float facing_x;
    float facing_z;

    facing_x = gxMathSin(plyr_obj->ang.y);
    facing_z = gxMathCos(plyr_obj->ang.y);
    xz_unit_vector(&velocity, &plyr_obj->pos.value, &his_obj->pos.value);
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
    struct SpearProcPdata* pdata;
    MkProc* proc;
    float inverse_ticks;
    Vec delta;

    proc = 0;
    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);
    if (camera != 0) {
        weapon = MK_HDR_LIVE(player->aux_weapon_latch.obj, player->aux_weapon_latch.instance);
        if (weapon != 0) {
            if (weapon->field_60 == 0) {
                weapon->field_60 = 1;
                if (player->character_id == 0) {
                    weapon->field_5C = get_data_table(player->cmo, 0x19);
                }
                if (player->character_id == 0x1C) {
                    weapon->field_5C = get_data_table(player->cmo, 0x16);
                }
                if (player->character_id == 0x19) {
                    weapon->field_5C = get_data_table(player->cmo, 3);
                }
                if (player->character_id == 0x1A) {
                    weapon->field_5C = get_data_table(player->cmo, 3);
                }
                plyr_aux_weapon_grab(player, weapon);
            }

            proc = _create_mkproc_generic_nostack(
                0x5019, 2, p_sc_spear1, sizeof(struct SpearProcPdata),
                (MkHdr**)&pdata);
            if (proc != 0) {
                zero_pdata_payload(sizeof(struct SpearProcPdata), &pdata->hdr);
                pdata->opponent_object = player->his_obj;
                pdata->opponent_pdata = player->his_plyr_pdata;
                pdata->player_proc_latch = player->player_proc_latch;
                pdata->field_34 = 0;
                pdata->flags = 0;
                pdata->bonematcher = 0;
                pdata->flag_bits.no_hit_reaction = 0;
                pdata->flag_bits.getup = 0;
                pdata->flag_bits.victory = 1;
                pdata->bound_object = 0;
                pdata->bound_object_instance = 0;

                weapon->flags_08_bits.gravity_enabled = 0;
                target = player->plyr_info->slot.mirror_a;
                delta.x = target->pos.value.x - camera->pos.x;
                delta.y = target->pos.value.y - camera->pos.y;
                delta.y += 0.8f;
                delta.z = target->pos.value.z - camera->pos.z;
                inverse_ticks = -1.0f / (float)ticks;
                delta.x *= inverse_ticks;
                delta.y *= inverse_ticks;
                delta.z *= inverse_ticks;
                weapon->pos_vel.x = delta.x;
                weapon->pos_vel.y = delta.y;
                weapon->pos_vel.z = delta.z;
                proc->pre_destroy = sc_spear_prewake;
                proc->destroy_cb = sc_spear_postsleep;
                proc->sleep_ticks = 2.0f;
                pdata->owner = player;
                pdata->spear_object = weapon;
                pdata->spear_object_instance = weapon->hdr.instance;
                pdata->effect = 0;
                pdata->effect_instance = 0;
                pdata->field_44 = 0;
            }
        }
    }
    return proc;
}

MkProc* fire_sc_spear(
    PlyrPdata* player, Vec* velocity, int field_34,
    int flag_40, MkHdr* bound_object, int flag_20) {
    MkObj* weapon;
    struct SpearProcPdata* pdata;
    MkProc* proc;

    proc = 0;
    weapon = MK_HDR_LIVE(player->aux_weapon_latch.obj, player->aux_weapon_latch.instance);
    if (weapon != 0) {
        if (weapon->field_60 == 0) {
            weapon->field_60 = 1;
            if (player->character_id == 0) {
                weapon->field_5C = get_data_table(player->cmo, 0x19);
            }
            if (player->character_id == 0x1C) {
                weapon->field_5C = get_data_table(player->cmo, 0x16);
            }
            if (player->character_id == 0x19) {
                weapon->field_5C = get_data_table(player->cmo, 3);
            }
            if (player->character_id == 0x1A) {
                weapon->field_5C = get_data_table(player->cmo, 3);
            }
            plyr_aux_weapon_grab(player, weapon);
        }

        proc = _create_mkproc_generic_nostack(
            0x5019, 2, p_sc_spear1, sizeof(struct SpearProcPdata),
            (MkHdr**)&pdata);
        if (proc != 0) {
            zero_pdata_payload(sizeof(struct SpearProcPdata), &pdata->hdr);
            pdata->opponent_object = player->his_obj;
            pdata->opponent_pdata = player->his_plyr_pdata;
            pdata->player_proc_latch = player->player_proc_latch;
            pdata->field_34 = field_34;
            pdata->flags = 0;
            pdata->bonematcher = 0;
            pdata->flag_bits.no_hit_reaction = flag_40;
            pdata->flag_bits.getup = flag_20;
            pdata->flag_bits.victory = 0;
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
        }
    }
    return proc;
}

float p_sc_spear1(void) {
    MkObj* target;
    struct NcsSpearEffect* effect;
    MkProc* effect_proc;
    int art_section;
    RwTexture* texture;

    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    plyr_aux_weapon_release(pdata_sc_spear->owner);
    target = MK_HDR_LIVE(pdata_sc_spear->owner->tracked_obj, pdata_sc_spear->owner->tracked_obj_instance);
    if (target == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    sc_spear_obj->ang.x = 0.0f;
    sc_spear_obj->ang.y = target->ang.y;
    sc_spear_obj->ang.z = 1.5707964f;
    sc_spear_obj->flags_08_bits.transform_dirty = 1;
    if (pdata_sc_spear->flag_bits.getup) {
        sc_spear_obj->ang.x = -0.9f;
    } else if (pdata_sc_spear->flag_bits.victory) {
        if (am_i_on_the_left2(
                pdata_sc_spear->owner->plyr_info->slot.mirror_a,
                pdata_sc_spear->owner->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
            sc_spear_obj->ang.y = target->ang.y - 1.5707964f;
        } else {
            sc_spear_obj->ang.y = target->ang.y + 1.5707964f;
        }
    }

    effect_proc = pfx_create_raw_userdata(
        0, 0, 0x64, 2, 0, 0, 0x501A,
        p_pfx_sc_spear, (void**)&effect);
    if (effect_proc == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    mkproc_change_priority(effect_proc, 0x2D);
    pdata_sc_spear->effect = effect;
    pdata_sc_spear->effect_instance = effect->hdr.instance;
    pfx_bind_emitter_to_obj((MkPfx*)effect, sc_spear_obj, 0);
    effect->spear_pdata = pdata_sc_spear;
    effect->spear_pdata_instance = pdata_sc_spear->hdr.instance;
    if (pdata_sc_spear->flag_bits.victory) {
        effect->bone = 0x18;
        if (am_i_on_the_left2(
                pdata_sc_spear->owner->plyr_info->slot.mirror_a,
                pdata_sc_spear->owner->his_plyr_pdata->plyr_info->slot.mirror_a) == 0) {
            effect->bone = 0x19;
        }
    } else {
        effect->bone = target->hide_flag_bits.bit6 != 0 ? 0x18 : 0x19;
    }

    art_section = get_shared_art_section_for_player(target);
    texture = load_named_tga_from_slot(art_section, "ROPE");
    pfx_set_texture((PfxRenderView*)&effect->vm, texture);
    effect->vm.flag150_40 = 1;
    effect->vm.billboard_size = 0.1f;
    effect->visible = 1;
    effect->field_298 = 1.0f;
    effect->field_29C = 0.0f;
    effect->field_2A0 = 0.975f;
    effect->field_2A4 = 0.96f;
    effect->field_2A8 = 0.0f;
    effect->active = 5;
    effect->field_290 = 1;
    effect->field_28 = -50.0f;
    snd_req(0x2CC);

    if (pdata_sc_spear->flag_bits.getup) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear2_getup, 1.0f);
        return 1.0f;
    }
    if (pdata_sc_spear->flag_bits.victory) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear2_victory, 1.0f);
        return 1.0f;
    }
    (aproc->vtbl)->jump_sleep(p_sc_spear2, 1.0f);
    return 1.0f;
}

static float p_sc_spear2(void) {
    Vec spear_rotation = {0.0f, 3.1415927f, 0.0f};
    MkObj* target;
    int collision;
    int outcome;

    target = MK_HDR_LIVE(pdata_sc_spear->owner->tracked_obj, pdata_sc_spear->owner->tracked_obj_instance);
    if (target == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    pdata_sc_spear->owner->saved_position_x = sc_spear_obj->pos.value.x;
    pdata_sc_spear->owner->saved_position_z = sc_spear_obj->pos.value.z;
    pdata_sc_spear->owner->duck_reaction_active = 1;
    collision = simple_3d_projectile_collision(
        &target->pos.value, &pdata_sc_spear->opponent_object->pos.value,
        &sc_spear_obj->pos.value, 0, 0.2f, 200.0f, 0.25f);
    outcome = 0;
    if (collision == 0) {
        trial_state_collision_check(
            1, target == g_game_info.plyr0.slot.mirror_a);
        if ((pdata_sc_spear->opponent_pdata->state & 0x100) != 0) {
            outcome = 2;
        } else if (pdata_sc_spear->opponent_pdata->state == 0x1222) {
            outcome = 2;
        } else if (g_game_info.flag_bits.level_fatality_active ||
                   g_game_info.flag_bits.level_transition_active) {
            outcome = 2;
        } else {
            struct NcsSpearEffect* effect;

            outcome = 1;
            effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
            if (effect != 0) {
                effect->field_2A0 = 0.75f;
                effect->field_2A8 = 0.005f;
            }
            if (!pdata_sc_spear->flag_bits.no_hit_reaction) {
                if (mode_of_play == 6) {
                    pz_fighter_reaction_xfer_him(0x22);
                } else {
                    reaction_xfer_him(0xA3, 0.06f, 0);
                }
            }
            if (pdata_sc_spear->owner->collision_result == 2) {
                outcome = 3;
            }
        }
    } else if (collision == 1) {
        trial_state_collision_check(
            0, target == g_game_info.plyr0.slot.mirror_a);
        outcome = 4;
    } else if (collision == 2) {
        trial_state_collision_check(
            0, target == g_game_info.plyr0.slot.mirror_a);
        outcome = 5;
    }

    switch (outcome) {
    case 1:
        if (pdata_sc_spear->owner->character_id == 0x19 ||
            pdata_sc_spear->owner->character_id == 0x1A) {
            snd_req(0x30B);
        } else {
            snd_req(0x2CD);
        }
        sc_spear_obj->flags_08_bits.gravity_enabled = 0;
        pdata_sc_spear->owner->duck_reaction_active = 0;
        if (pdata_sc_spear->flag_bits.no_hit_reaction) {
            YXZ_angles_to_quat(
                &spear_rotation, &sc_spear_obj->bones[0]->rotation);
        }
        pdata_sc_spear->bonematcher = start_bone_matcher(
            pdata_sc_spear->owner->his_obj, pdata_sc_spear->field_34, sc_spear_obj, 0, 2.0f);
        if (pdata_sc_spear->bonematcher == 0) {
            (aproc->vtbl)->jump_sleep(
                p_sc_spear_kill, 0.0f);
            return 0.0f;
        }
        if (!pdata_sc_spear->flag_bits.no_hit_reaction) {
            pdata_sc_spear->bonematcher->blend = 0.4f;
            (aproc->vtbl)->jump_sleep(
                p_sc_spear3_pre, 25.0f);
            return 25.0f;
        }
        pdata_sc_spear->bonematcher->blend = 0.25f;
        pdata_sc_spear->bonematcher->copy_bone_matrix = 1;
        (aproc->vtbl)->jump_sleep(
            player_sleep_forever, 1.0f);
        return 1.0f;
    case 3:
        if (pdata_sc_spear->owner->character_id == 0x19 ||
            pdata_sc_spear->owner->character_id == 0x1A) {
            snd_req(0x30C);
        } else {
            snd_req(0x2CE);
        }
        sc_spear_obj->flags_08_bits.gravity_enabled = 0;
        pdata_sc_spear->flag_bits.bit7 = 1;
        pdata_sc_spear->blocked_ticks = 8;
        pdata_sc_spear->owner->duck_reaction_active = 0;
        if (pdata_sc_spear->owner->secondary_state == 0x101) {
            pdata_sc_spear->owner->state = 0x4206;
        }
        pdata_sc_spear->owner->secondary_state = 0;
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_blocked, 0.0f);
        return 0.0f;
    case 4:
        pdata_sc_spear->owner->duck_reaction_active = 0;
        if (pdata_sc_spear->owner->secondary_state == 0x101) {
            pdata_sc_spear->owner->state = 0x4206;
        }
        pdata_sc_spear->owner->secondary_state = 0;
        break;
    case 5:
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    case 0:
    case 2:
        break;
    }
    return 1.0f;
}

static float p_sc_spear2_victory(void) {
    CameraObj* camera;
    struct NcsSpearEffect* effect;
    float dx;
    float dz;
    float root;

    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);
    if (camera == 0) {
        return 1.0f;
    }

    dx = camera->pos.x - sc_spear_obj->pos.value.x;
    dz = camera->pos.z - sc_spear_obj->pos.value.z;
    root = gxMathFastSqrt(dx * dx + dz * dz);

    if (root < 0.3f) {
        sc_spear_obj->pos_vel.z = 0.0f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.x = 0.0f;
        effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
        if (effect != 0) {
            effect->field_2A0 = 0.75f;
            effect->field_2A8 = 0.005f;
        }
    }
    return 1.0f;
}

static float p_sc_spear2_getup(void) {
    struct NcsSpearEffect* effect;

    if (sc_spear_obj->pos.value.y > g_game_info.field_34 + 4.0f &&
        sc_spear_obj->pos_vel.y) {
        sc_spear_obj->pos_vel.z = 0.0f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.x = 0.0f;
        effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
        if (effect != 0) {
            effect->field_2A0 = 0.75f;
            effect->field_2A8 = 0.005f;
        }
    }
    return 1.0f;
}

float p_sc_spear_blocked(void) {
    struct NcsSpearEffect* effect;

    effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
    if (effect == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    pdata_sc_spear->owner->duck_reaction_active = 0;
    if ((pdata_sc_spear->blocked_ticks & 1) != 0) {
        set_pfx_texture(
            &effect->vm, 0x10005, 0x20039);
    } else {
        pfx_set_texture(
            (PfxRenderView*)&effect->vm,
            load_named_tga_from_slot(
                get_shared_art_section_for_player(pdata_sc_spear->opponent_pdata->his_obj),
                "ROPE"));
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
    (aproc->vtbl)->jump_sleep(p_sc_spear4, 1.0f);
    return 1.0f;
}

float p_sc_spear_retract(void) {
    MkObj* owner_object;

    owner_object = pdata_sc_spear->owner->plyr_info->slot.mirror_a;
    pdata_sc_spear->owner->duck_reaction_active = 0;
    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->hdr.instance != 0) {
            pdata_sc_spear->bonematcher->hdr.typed_vtbl->destroy(
                &pdata_sc_spear->bonematcher->hdr);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    if (!pdata_sc_spear->flag_bits.no_hit_reaction) {
        if (pdata_sc_spear->flag_bits.getup) {
            Vec target;

            get_bone_world_pos(owner_object,
                               get_bid_with_flip(owner_object, 0x19), &target);
            sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.value.x;
            sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.value.y;
            sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.value.z;
            sc_spear_obj->pos_vel.x = 0.065f * sc_spear_obj->pos_vel.x;
            sc_spear_obj->pos_vel.y = 0.065f * sc_spear_obj->pos_vel.y;
            sc_spear_obj->pos_vel.z = 0.065f * sc_spear_obj->pos_vel.z;
            (aproc->vtbl)->jump_sleep(
                p_sc_spear4_getup, 1.0f);
            return 1.0f;
        }
        sc_spear_obj->pos_vel.x *= -2.25f;
        sc_spear_obj->pos_vel.y = 0.0f;
        sc_spear_obj->pos_vel.z *= -2.25f;
        if (sc_spear_obj->pos.value.y < 1.0f) {
            sc_spear_obj->pos.value.y = 1.0f;
        }
    }
    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    (aproc->vtbl)->jump_sleep(p_sc_spear4, 1.0f);
    return 1.0f;
}

float p_sc_spear_retract_victory(void) {
    MkObj* owner_object;
    Vec target;

    owner_object = pdata_sc_spear->owner->plyr_info->slot.mirror_a;
    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->hdr.instance != 0) {
            pdata_sc_spear->bonematcher->hdr.typed_vtbl->destroy(
                &pdata_sc_spear->bonematcher->hdr);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    if (am_i_on_the_left2(
            pdata_sc_spear->owner->plyr_info->slot.mirror_a,
            pdata_sc_spear->owner->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        get_bone_world_pos(owner_object, 0x18, &target);
    } else {
        get_bone_world_pos(owner_object, 0x19, &target);
    }
    sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.value.x;
    sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.value.y;
    sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.value.z;
    sc_spear_obj->pos_vel.x = 0.035f * sc_spear_obj->pos_vel.x;
    sc_spear_obj->pos_vel.y = 0.035f * sc_spear_obj->pos_vel.y;
    sc_spear_obj->pos_vel.z = 0.035f * sc_spear_obj->pos_vel.z;
    sc_spear_obj->flags_08_bits.gravity_enabled = 1;
    (aproc->vtbl)->jump_sleep(
        p_sc_spear4_victory, 1.0f);
    return 1.0f;
}

static float p_sc_spear3_pre(void) {
    Vec angle;

    v3_to_xy_ang(&angle, (Vec*)&sc_spear_obj->field_24->at);
    (aproc->vtbl)->jump_sleep(p_sc_spear3, 50.0f);
    return 50.0f;
}

static float p_sc_spear3(void) {
    struct NcsSpearEffect* effect;

    effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
    if (effect != 0) {
        effect->field_298 = 1.0f;
        effect->field_29C = 0.0f;
        effect->field_2A0 = 0.95f;
        effect->field_2A4 = 0.96f;
        effect->field_2A8 = 0.0f;
        effect->active = 1;
    }
    (aproc->vtbl)->jump_sleep(p_sc_spear4, 0.0f);
    return 0.0f;
}

static float p_sc_spear4(void) {
    MkObj* target;
    int collision;

    target = MK_HDR_LIVE(pdata_sc_spear->owner->tracked_obj, pdata_sc_spear->owner->tracked_obj_instance);
    if (target == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    if (g_game_info.flag_bits.level_fatality_active ||
        g_game_info.flag_bits.level_transition_active) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    collision = simple_3d_projectile_collision(
        &target->pos.value, &target->pos.value, &sc_spear_obj->pos.value,
        1, 1.5f, 200.0f, 0.25f);
    if (collision == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    if (!pdata_sc_spear->flag_bits.no_hit_reaction) {
        if (sc_spear_obj->field_24->at.x * target->field_24->at.x +
                sc_spear_obj->field_24->at.y * target->field_24->at.y +
                sc_spear_obj->field_24->at.z * target->field_24->at.z <
            0.75f) {
            (aproc->vtbl)->jump_sleep(
                p_sc_spear_kill, 0.0f);
            return 0.0f;
        }
    }
    if (collision == 2) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

static inline float ncs_xz_distance_squared(const Vec* a, const Vec* b) {
    float dx = a->x - b->x;
    float dz = a->z - b->z;

    return dx * dx + dz * dz;
}


/* TODO: [near miss] 99.45%; helper structure and stack slots match; spear pointer r5/r6 and first-distance FPR coloring remain. */
static float p_sc_spear4_victory(void) {
    MkObj* target_object;
    Vec target;
    float distance;
    float speed;
    float direction_squared;
    float inverse_length;

    target_object = MK_HDR_LIVE(pdata_sc_spear->owner->tracked_obj, pdata_sc_spear->owner->tracked_obj_instance);
    if (target_object == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    get_bone_world_pos(
        target_object, get_bid_with_flip(target_object, 0x19), &target);
    distance = gxMathFastSqrt(ncs_xz_distance_squared(&target, &sc_spear_obj->pos.value));
    if (distance < 0.5f) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    speed = gxMathFastSqrt(
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z);

    sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.value.x;
    sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.value.y;
    sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.value.z;
    direction_squared =
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z;
    inverse_length = gxMathFastInvSqrt(direction_squared);
    sc_spear_obj->pos_vel.x = sc_spear_obj->pos_vel.x * inverse_length;
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
    float speed;
    float inverse_length;

    target_object = MK_HDR_LIVE(pdata_sc_spear->owner->tracked_obj, pdata_sc_spear->owner->tracked_obj_instance);
    if (target_object == 0) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }
    get_bone_world_pos(
        target_object, get_bid_with_flip(target_object, 0x19), &target);
    if (target.y > sc_spear_obj->pos.value.y) {
        (aproc->vtbl)->jump_sleep(
            p_sc_spear_kill, 0.0f);
        return 0.0f;
    }

    speed = gxMathFastSqrt(
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z);

    sc_spear_obj->pos_vel.x = target.x - sc_spear_obj->pos.value.x;
    sc_spear_obj->pos_vel.y = target.y - sc_spear_obj->pos.value.y;
    sc_spear_obj->pos_vel.z = target.z - sc_spear_obj->pos.value.z;
    inverse_length = gxMathFastInvSqrt(
        sc_spear_obj->pos_vel.x * sc_spear_obj->pos_vel.x +
        sc_spear_obj->pos_vel.y * sc_spear_obj->pos_vel.y +
        sc_spear_obj->pos_vel.z * sc_spear_obj->pos_vel.z);
    sc_spear_obj->pos_vel.x = sc_spear_obj->pos_vel.x * inverse_length;
    sc_spear_obj->pos_vel.y = sc_spear_obj->pos_vel.y * inverse_length;
    sc_spear_obj->pos_vel.z = sc_spear_obj->pos_vel.z * inverse_length;
    sc_spear_obj->pos_vel.x *= speed;
    sc_spear_obj->pos_vel.y *= speed;
    sc_spear_obj->pos_vel.z *= speed;
    return 1.0f;
}

float p_sc_spear_kill(void) {
    struct NcsSpearEffect* effect;

    effect = MK_HDR_LIVE(pdata_sc_spear->effect, pdata_sc_spear->effect_instance);
    if (effect != 0 && effect->hdr.instance != 0) {
        effect->hdr.typed_vtbl->destroy(&effect->hdr);
    }

    if (pdata_sc_spear->bonematcher != 0) {
        if (pdata_sc_spear->bonematcher->hdr.instance != 0) {
            pdata_sc_spear->bonematcher->hdr.typed_vtbl->destroy(
                &pdata_sc_spear->bonematcher->hdr);
        }
        pdata_sc_spear->bonematcher = 0;
    }

    sc_spear_obj->flags_08_bits.gravity_enabled = 0;
    sc_spear_obj->field_60 = 0;
    if (pdata_sc_spear->owner->character_id == 0) {
        sc_spear_obj->field_5C = get_data_table(pdata_sc_spear->owner->cmo, 0x18);
    }
    if (pdata_sc_spear->owner->character_id == 0x1C) {
        sc_spear_obj->field_5C = get_data_table(pdata_sc_spear->owner->cmo, 0x15);
    }
    if (pdata_sc_spear->owner->character_id == 0x19) {
        sc_spear_obj->field_5C = get_data_table(pdata_sc_spear->owner->cmo, 2);
    }
    if (pdata_sc_spear->owner->character_id == 0x1A) {
        sc_spear_obj->field_5C = get_data_table(pdata_sc_spear->owner->cmo, 2);
    }
    plyr_aux_weapon_grab(pdata_sc_spear->owner, sc_spear_obj);
    pdata_sc_spear->owner->duck_reaction_active = 0;
    return -1.0f;
}

/* TODO: [near miss] 98.80%; FPR homes agree; spear_pdata/emitter r30/r31 swap and stride load scheduling remain. */
static float p_pfx_sc_spear(void) {
    struct SpearProcPdata* spear_pdata;
    MkObj* emitter_object;
    PlyrPdata* owner;
    MkObj* target;
    PfxVm* vm;
    Vec target_position;
    Vec direction;
    Vec* particle_position;
    float length;
    float amplitude;
    float absolute_amplitude;
    float base_spacing;
    float phase;
    float distance;
    int particle_count;
    int stride;

    if (apfx_emitter_obj == 0) {
        mkproc_die();
    }
    spear_pdata = MK_HDR_LIVE(
        ((struct NcsSpearEffect*)apfx)->spear_pdata,
        ((struct NcsSpearEffect*)apfx)->spear_pdata_instance);
    if (spear_pdata == 0) {
        return -1.0f;
    }
    owner = spear_pdata->owner;
    target = MK_HDR_LIVE(owner->tracked_obj, owner->tracked_obj_instance);
    if (target == 0) {
        return -1.0f;
    }

    vm = &((struct NcsSpearEffect*)apfx)->vm;
    stride = apfx->transforms[0].particle_field_stride;
    particle_position = pfx_get_field(vm, -2, 0x100);
    emitter_object = apfx_emitter_obj;
    get_bone_world_pos(target, apfx->field_28C, &target_position);
    v3_sub_v3(&direction, &apfx_emitter_obj->pos.value, &target_position);
    length = normalize_v3_length(&direction);

    if (apfx->field_298 > 0.0f) {
        if (apfx->accum_38 > (float)apfx->field_290) {
            apfx->field_290 += (int)game_speed + 1;
            apfx->field_298 *= apfx->field_2A0;
            apfx->field_2A4 += apfx->field_2A8;
            if (apfx->field_2A4 > 1.02f) {
                apfx->field_2A4 = 1.02f;
            }
            apfx->field_29C +=
                6.2831855f / (5.5f + (float)apfx->effect_state);
            if (apfx->field_29C >= 6.2831855f) {
                apfx->field_29C -= 6.2831855f;
                apfx->effect_state++;
            }
        }
        amplitude = apfx->field_298 * gxMathSin(apfx->field_29C);
        base_spacing = length / (float)apfx->effect_state;
    } else {
        amplitude = 0.0f;
        base_spacing = length;
    }

    phase = 0.0f;
    distance = 0.0f;
    particle_count = 0;
    while (particle_count < vm->particle_capacity) {
        float step;

        if (distance > length - 0.05f) {
            break;
        }

        if (apfx->field_298 > 0.0f) {
            float sine;

            absolute_amplitude = amplitude;
            if (amplitude < 0.0f) {
                absolute_amplitude = -amplitude;
            }
            phase += 0.31415927f /
                (3.1415927f * absolute_amplitude + base_spacing);
            sine = gxMathSin(phase);
            if (sine < 0.0f) {
                sine = -sine;
            }
            step =
                (0.05f * (1.0f - absolute_amplitude) +
                 absolute_amplitude * (2.0f * sine * 0.05f)) /
                (1.0f + absolute_amplitude);
            amplitude *= apfx->field_2A4;
        } else {
            step = 0.05f;
        }
        distance += step;
        particle_position->x =
            emitter_object->pos.value.x - direction.x * distance;
        particle_position->z =
            emitter_object->pos.value.z - direction.z * distance;
        if (!spear_pdata->flag_bits.bit7) {
            if (apfx->field_298 > 0.01f) {
                particle_position->y =
                    emitter_object->pos.value.y + amplitude * gxMathSin(phase);
            } else {
                particle_position->y = emitter_object->pos.value.y;
            }
            particle_position->y -=
                distance * (particle_position->y - target_position.y) /
                length;
        } else {
            particle_position->y = emitter_object->pos.value.y;
        }
        particle_position = PFX_FIELD_AT(particle_position, stride);
        particle_count++;
    }
    vm->particle_cursor = particle_count;
    return 1.0f;
}

void retract_spear_from_camera(MkProc* proc) {
    xfer_proc(proc, p_sc_spear_retract_victory);
}

void xfer_spearproc_to_retract(MkProc* proc) {
    xfer_proc(proc, p_sc_spear_retract);
}

void destroy_spearproc_bonematcher(MkProc* proc) {
    struct SpearProcPdata* pdata;
    struct NcsBoneMatcher* matcher;

    pdata = (struct SpearProcPdata*)pdata_of_proc(proc);
    if (pdata != 0) {
        matcher = pdata->bonematcher;
        if (matcher->hdr.instance != 0) {
            matcher->hdr.typed_vtbl->destroy(&matcher->hdr);
        }
        pdata->bonematcher = 0;
    }
}

void insert_mkobj_spearproc_parentobjitem(MkObj* parent, MkProc* proc) {
    struct SpearProcPdata* pdata;

    pdata = (struct SpearProcPdata*)pdata_of_proc(proc);
    if (pdata != 0) {
        pdata->bonematcher->parent = parent;
        pdata->bonematcher->parent_instance = parent->hdr.instance;
    }
}

MkObj* get_spearobj_from_spearproc(MkProc* proc) {
    struct SpearProcPdata* pdata = (struct SpearProcPdata*)pdata_of_proc(proc);

    if (pdata == 0) {
        return 0;
    }
    return MK_HDR_LIVE(pdata->spear_object, pdata->spear_object_instance);
}

void sc_spear_postsleep(void) {
    pdata_sc_spear = 0;
    sc_spear_obj = 0;
    his_pdata = 0;
    his_obj = 0;
}

static void sc_spear_prewake(void) {
    MkObj* object;

    if (aproc->pid != 0x5019) {
        mkproc_die();
    }
    pdata_sc_spear = (struct SpearProcPdata*)apdata;
    if (pdata_sc_spear == 0) {
        mkproc_die();
    }
    object = MK_HDR_LIVE(pdata_sc_spear->spear_object, pdata_sc_spear->spear_object_instance);
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

struct PrisonGrabPdata* start_prison_grab_proc(
    MkObj* object, Vec* direction, float angle, float strength) {
    struct PrisonGrabPdata* pdata;
    float squared;
    float inverse_length;
    float z;
    float x;
    float x_squared;
    float z_squared;
    float normalized_x;
    float normalized_z;

    pdata = 0;
    if (_create_mkproc_generic_nostack(
            0x603D, 0x1F, p_prison_grab, sizeof(struct PrisonGrabPdata),
            (MkHdr**)&pdata) != 0) {
        zero_pdata_payload(sizeof(struct PrisonGrabPdata), &pdata->hdr);
        pdata->object = object;
        pdata->object_instance = object->hdr.instance;
        x = direction->x;
        z = direction->z;
        x_squared = x * x;
        z_squared = z * z;
        squared = x_squared + z_squared;
        inverse_length = gxMathFastInvSqrt(squared);
        normalized_x = x * inverse_length;
        normalized_z = z * inverse_length;
        pdata->target_x = x - normalized_x * strength;
        pdata->target_z = direction->z - normalized_z * strength;
        if (angle < 0.0f) {
            pdata->target_angle = 6.2831855f + angle;
        } else {
            pdata->target_angle = angle;
        }
        pdata->current_x = object->pos.value.x;
        pdata->current_y = object->pos.value.y;
        pdata->current_z = object->pos.value.z;
        plyr_pdata->blocking_disabled_2 = 1;
        plyr_pdata->blocking_disabled = 0;
        set_my_state(0x600);
    }
    return pdata;
}

void done_prison_grab_proc(struct PrisonGrabPdata* pdata) {
    pdata->done = 1;
}

static inline MkObj* prison_grab_object(struct PrisonGrabPdata* pdata)
{
    MkObj* object = pdata->object;

    if (object != 0) {
        if (object->hdr.instance == pdata->object_instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static float p_prison_grab(void) {
    struct PrisonGrabPdata* pdata;
    MkObj* object;
    float squared;
    float inverse_length;
    int position_axes_done = 0;

    pdata = (struct PrisonGrabPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    object = prison_grab_object(pdata);
    if (object == 0) {
        return -1.0f;
    }

    if (pdata->done != 0) {
        squared = pdata->target_x * pdata->target_x +
                  pdata->target_z * pdata->target_z;
        inverse_length = gxMathFastInvSqrt(squared);
        object->pos_vel.x = pdata->target_x * inverse_length;
        object->pos_vel.z = pdata->target_z * inverse_length;
        object->pos_vel.x *= 0.1f;
        object->pos_vel.y = 0.0f;
        object->pos_vel.z *= 0.1f;
        return -1.0f;
    }

    if (pdata->aligned != 0) {
        object->ang.y = pdata->target_angle;
    } else {
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

    object->pos.value.x = pdata->current_x;
    object->pos.value.z = pdata->current_z;
    return 1.0f;
}

void ncs_dkp_camera_konqchar_show_hide_alpha(
    int character_index, MkObj* character) {
    MkProc* process;
    struct NcsKonquestCharacterPdata* pdata;

    process = MK_LIVE(g_game_info.camera_proc, g_game_info.camera_proc_instance);
    if (process == 0) {
        return;
    }

    pdata = (struct NcsKonquestCharacterPdata*)pdata_of_proc(process);
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
    MkHdr* pdata;
    NcsCameraWallRegion* region;
    int region_index;
    int index;

    process = MK_HDR_LIVE(
        g_game_info.camera_proc, g_game_info.camera_proc_instance);
    pdata = 0;
    if (process != 0) {
        pdata = pdata_of_proc(process);
        if (pdata == 0) {
            process = 0;
        }
    }
    if (process == 0) {
        process = _create_mkproc_generic_nostack(
            0x6008, 0x1F, p_camera_wall_show_hide_alpha,
            sizeof(struct NcsCameraWallPdata), &pdata);
        if (process == 0) {
            return;
        }
    }

    zero_pdata_payload(sizeof(struct NcsCameraWallPdata), pdata);
    g_game_info.camera_proc = process;
    g_game_info.camera_proc_instance = process->instance;
    ((struct NcsCameraWallPdata*)pdata)->regions = regions;
    ((struct NcsCameraWallPdata*)pdata)->active_region = -1;

    for (region_index = 0; regions[region_index].type < 3; region_index++) {
        region = &regions[region_index];

        if (region->type == 0) {
            region->max_z = region->min_z * region->min_z;
        }
        for (index = 0; region->show_ids[index] >= 0; index++) {
            MkSobj* sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, region->show_ids[index]);

            if (sobj != 0) {
                sobj->z_offset = -50.0f;
            }
        }
        for (index = 0; region->alpha_ids[index] >= 0; index++) {
            MkSobj* sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, region->alpha_ids[index]);

            if (sobj != 0 && sobj->atomic != 0 &&
                sobj->atomic->geometry != 0) {
                sobj->atomic->geometry->flags |= 0x40;
                atomic_set_transl_flag(sobj->atomic);
            }
        }
    }
    for (index = 0; index < 8; index++) {
        ((struct NcsCameraWallPdata*)pdata)->characters[index] = 0;
    }
    ((struct NcsCameraWallPdata*)pdata)->special_alpha_initialized = 0;
}

/* TODO: [breakthrough] 96.65%; search reloads pdata->regions; defined initial alpha store kept (retail lacks it);
 * camera FPR homes and character index (id-0xB) folding remain. */
static float p_camera_wall_show_hide_alpha(void) {
    struct NcsCameraWallPdata* pdata;
    NcsCameraWallRegion* region;
    CamVec3 camera_position;
    RwRGBA white;
    int region_index;

    pdata = (struct NcsCameraWallPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    if (pdata->regions == 0) {
        return 1.0f;
    }

    get_camera_position(&camera_position);
    white.red = 0xFF;
    white.green = 0xFF;
    white.blue = 0xFF;
    white.alpha = 0xFF;
    for (region_index = 0; pdata->regions[region_index].type < 3; region_index++) {
        region = &pdata->regions[region_index];
        if (region->type == 0) {
            float dx = camera_position.x - region->min_x;
            float dz = camera_position.z - region->max_x;
            float dx_squared = dx * dx;
            float dz_squared = dz * dz;

            if (dx_squared + dz_squared <= region->max_z) {
                break;
            }
        } else if (region->type == 1) {
            if (camera_position.x > region->min_x &&
                camera_position.x < region->max_x &&
                camera_position.z > region->min_z &&
                camera_position.z < region->max_z) {
                break;
            }
        }
    }

    if (region_index != pdata->active_region) {
        if (pdata->active_region >= 0) {
            NcsCameraWallRegion* previous =
                &pdata->regions[pdata->active_region];
            int object_index;

            for (object_index = 0; previous->hide_ids[object_index] >= 0; object_index++) {
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, previous->hide_ids[object_index]);

                if (sobj != 0 && sobj->atomic != 0) {
                    unhide_atomic(sobj->atomic);
                }
            }
            for (object_index = 0; previous->show_ids[object_index] >= 0; object_index++) {
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, previous->show_ids[object_index]);

                if (sobj != 0 && sobj->atomic != 0) {
                    hide_atomic(sobj->atomic);
                }
            }
            white.alpha = 0xFF;
            for (object_index = 0; previous->alpha_ids[object_index] >= 0; object_index++) {
                int id = previous->alpha_ids[object_index];
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, id);

                if (sobj != 0) {
                    sobj_set_color_for_all_materials(sobj, &white);
                    sobj->flags09_bits.bit7 = 0;
                }
                if (id >= 0x0B && id <= 0x12 &&
                    pdata->characters[id - 0x0B] != 0) {
                    obj_for_all_atomics_set_material_alpha(
                        pdata->characters[id - 0x0B], white.alpha);
                }
            }
        }

        region = &pdata->regions[region_index];
        if (region->type >= 3) {
            pdata->active_region = -1;
        } else {
            int object_index;

            pdata->active_region = region_index;
            for (object_index = 0; region->hide_ids[object_index] >= 0; object_index++) {
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, region->hide_ids[object_index]);

                if (sobj != 0 && sobj->atomic != 0) {
                    hide_atomic(sobj->atomic);
                }
            }
            for (object_index = 0; region->show_ids[object_index] >= 0; object_index++) {
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, region->show_ids[object_index]);

                if (sobj != 0 && sobj->atomic != 0) {
                    unhide_atomic(sobj->atomic);
                }
            }
            for (object_index = 0; region->alpha_ids[object_index] >= 0; object_index++) {
                int id = region->alpha_ids[object_index];
                MkSobj* sobj = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, id);

                if (sobj != 0) {
                    sobj_set_color_for_all_materials(sobj, &white);
                    sobj->flags09_bits.bit7 = 1;
                }
                if (id >= 0x0B && id <= 0x12 &&
                    pdata->characters[id - 0x0B] != 0) {
                    obj_for_all_atomics_set_material_alpha(
                        pdata->characters[id - 0x0B], white.alpha);
                }
            }
        }
    }

    white.alpha = 0xFF;
    if (g_game_info.flag_bits.level_transition_active &&
        pdata->special_alpha_initialized == 0) {
        int id;

        pdata->special_alpha_initialized = 1;
        for (id = 0x4E; id <= 0x54; id++) {
            MkSobj* sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, id);

            sobj_set_color_for_all_materials(sobj, &white);
            sobj->flags09_bits.bit7 = 0;
        }
    }
    return 1.0f;
}

void destroy_gore2_obj(unsigned int object_id, int particle_index) {
    PebbleData* pool;
    int type;

    for (type = 0; type < 10; type++) {
        if (pbl_gore2_obj_list[type].object_id == object_id) {
            break;
        }
    }
    if (type >= 10) {
        return;
    }

    pool = mkpdata_pbl_gore2_update->pools[type];
    if (particle_index >= pool->active_count) {
        return;
    }
    pool->flags[particle_index].bits.visible = 0;
}

int attach_gore2_obj(
    MkObj* owner, int bone, unsigned int object_id,
    const Vec* offset, const Vec* rotation) {
    PebbleData* pool;
    struct Gore2Particle* particle;
    int particle_index;
    int result;
    int type;

    for (type = 0; type < 10; type++) {
        if (pbl_gore2_obj_list[type].object_id == object_id) {
            break;
        }
    }
    if (type >= 10) {
        return -1;
    }
    if ((unsigned int)bone != 0x40000000) {
        MkBone* attach_bone = plyr_obj->bones[bone];

        if (attach_bone == 0) {
            return -1;
        }
        attach_bone->flags_54_bits.calculation_locked = 1;
    }

    pool = mkpdata_pbl_gore2_update->pools[type];
    particle_index = mkpdata_pbl_gore2_update->next_particle[type];
    particle = &((struct Gore2Particle*)pool->user_data)[particle_index];
    particle->flags.word = 0;
    if (rotation != 0) {
        particle->flags.bits.has_rotation = 1;
        particle->rotation.x = rotation->x;
        particle->rotation.y = rotation->y;
        particle->rotation.z = rotation->z;
    } else {
        particle->flags.bits.has_rotation = 0;
        particle->rotation.z = 0.0f;
        particle->rotation.y = 0.0f;
        particle->rotation.x = 0.0f;
    }
    if (offset != 0) {
        particle->flags.bits.has_translation = 1;
        particle->translation.x = offset->x;
        particle->translation.y = offset->y;
        particle->translation.z = offset->z;
    } else {
        particle->flags.bits.has_translation = 0;
        particle->translation.z = 0.0f;
        particle->translation.y = 0.0f;
        particle->translation.x = 0.0f;
    }
    particle->flags.bits.attached = 1;
    particle->owner.object = owner;
    particle->owner.instance = owner->hdr.instance;
    particle->bone = bone;
    pool->flags[particle_index].bits.visible = 1;

    result = particle_index;
    particle_index++;
    if (particle_index >= pool->active_count) {
        particle_index = 0;
    }
    mkpdata_pbl_gore2_update->next_particle[type] = particle_index;
    return result;
}

/* TODO: [near miss] 98.91%; bone index home fixed; pool r29/particle r28 swap vs retail remains; stop at coloring. */
void start_gore2_pebbles(
    unsigned int object_id, int bone, MkObj* source,
    FighterMirror* decal_owner, Vec* velocity,
    const Vec* rotation, const Vec* scale,
    Vec* position_offset, float vertical_acceleration,
    float bounce_scale, int bounce_count) {
    int type;

    for (type = 0; type < 10; type++) {
        if (pbl_gore2_obj_list[type].object_id == object_id) {
            break;
        }
    }
    if (type < 10) {
        MkBone* source_bone = source->bones[bone];

        if (source_bone != 0) {
            struct Gore2Particle* particle;
            PebbleData* pool;
            int particle_index;

            particle_index = mkpdata_pbl_gore2_update->next_particle[type];
            pool = mkpdata_pbl_gore2_update->pools[type];
            particle = &((struct Gore2Particle*)pool->user_data)[particle_index];

            particle->flags.word = 0;
            get_bone_world_pos(source, bone, &pool->pebbles[particle_index].matrix.pos_row.value);
            calc_bone_world_mat(source, bone);
            if (rotation != 0) {
                particle->flags.bits.has_rotation = 1;
                particle->rotation.x = rotation->x;
                particle->rotation.y = rotation->y;
                particle->rotation.z = rotation->z;
            } else {
                particle->flags.bits.has_rotation = 0;
                particle->rotation.z = 0.0f;
                particle->rotation.y = 0.0f;
                particle->rotation.x = 0.0f;
            }
            if (scale != 0) {
                particle->flags.bits.has_scale = 1;
                particle->scale.x = scale->x;
                particle->scale.y = scale->y;
                particle->scale.z = scale->z;
            } else {
                particle->flags.bits.has_scale = 0;
                particle->scale.z = 0.0f;
                particle->scale.y = 0.0f;
                particle->scale.x = 0.0f;
            }
            if (velocity != 0) {
                particle->flags.bits.has_translation = 1;
                v3_x_mat(
                    &particle->translation, velocity,
                    &source_bone->matrix);
            } else {
                particle->flags.bits.has_translation = 0;
                particle->translation.z = 0.0f;
                particle->translation.y = 0.0f;
                particle->translation.x = 0.0f;
            }
            if (position_offset != 0) {
                Vec transformed_offset;

                if (particle->flags.bits.has_rotation) {
                    YXZ_angles_to_MKMATRIX(rotation, &pool->pebbles[particle_index].matrix);
                }
                if (particle->flags.bits.has_scale) {
                    mat_scaled_by_v3(
                        &pool->pebbles[particle_index].matrix,
                        &pool->pebbles[particle_index].matrix, scale);
                }
                v3_x_mat(
                    &transformed_offset, position_offset,
                    &source_bone->matrix);
                pool->pebbles[particle_index].matrix.pos_row.value.x = transformed_offset.x + pool->pebbles[particle_index].matrix.pos_row.value.x;
                pool->pebbles[particle_index].matrix.pos_row.value.y = transformed_offset.y + pool->pebbles[particle_index].matrix.pos_row.value.y;
                pool->pebbles[particle_index].matrix.pos_row.value.z = transformed_offset.z + pool->pebbles[particle_index].matrix.pos_row.value.z;
            }
            particle->flags.bits.settled = 0;
            particle->vertical_acceleration = vertical_acceleration;
            particle->bounce_count = bounce_count;
            particle->bounce_scale = bounce_scale;
            particle->decal_owner = decal_owner;
            pool->flags[particle_index].bits.visible = 1;
            particle_index++;
            if (particle_index >= pool->active_count) {
                particle_index = 0;
            }
            mkpdata_pbl_gore2_update->next_particle[type] =
                particle_index;
        }
    }
}

/* TODO: [near miss] 99.73%; type index and retained count pointer swap r25/r28; stop at coloring. */
void start_gore2_update(void) {
    struct Gore2UpdatePdata* pdata;
    int type;

    pdata = 0;
    if (_create_mkproc_generic_nostack(
            0x603F, 0x1F, p_gore2_update,
            sizeof(struct Gore2UpdatePdata), (MkHdr**)&pdata) != 0) {
        zero_pdata_payload(sizeof(struct Gore2UpdatePdata), &pdata->hdr);
        load_light(pbl_gore2_lights[1], &gore2_light_list, 0);
        for (type = 0; type < 10; type++) {
            MkObj* object = load_model_from_slot(
                0x10005, pbl_gore2_obj_list[type].object_id, 0x602A);

            if (object != 0) {
                MkSobj* sobj;

                obj_create_sobjs(object);
                sobj = obj_first_sobj(object);
                if (sobj != 0) {
                    int* particle_count = &pbl_gore2_obj_list[type].particle_count;

                    pdata->pools[type] = create_pebble_userdata(
                        sobj, *particle_count,
                        sizeof(struct Gore2Particle));
                    if (pdata->pools[type] != 0) {
                        int particle;

                        mk_insert(&object->hdr, &g_game_info.bgnd_obj->child_list);
                        insert_fgnd_mkobj(object);
                        object->light_flags = 0x800;
                        object->pos.value.z = 0.0f;
                        object->pos.value.y = 0.0f;
                        object->pos.value.x = 0.0f;
                        object->ang.z = 0.0f;
                        object->ang.y = 0.0f;
                        object->ang.x = 0.0f;
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
                             particle < *particle_count;
                             particle++) {
                            pdata->pools[type]->flags[particle].bits.visible = 0;
                            pdata->pools[type]->pebbles[particle].matrix.pos.z = 0.0f;
                            pdata->pools[type]->pebbles[particle].matrix.pos.y = 0.0f;
                            pdata->pools[type]->pebbles[particle].matrix.pos.x = 0.0f;
                            pdata->pools[type]->pebbles[particle].matrix.pos.y = -10000.0f;
                        }
                    }
                }
            }
        }
    }
    mkpdata_pbl_gore2_update = pdata;
}

static float p_gore2_update(void) {
    struct Gore2UpdatePdata* pdata;
    int type;

    pdata = (struct Gore2UpdatePdata*)apdata;
    for (type = 0; type < 10; type++) {
        int particle_index;
        PebbleData* pool = pdata->pools[type];

        for (particle_index = 0;
             particle_index < pool->active_count; particle_index++) {
            PebbleFlags* state = &pool->flags[particle_index];

            if (state->bits.visible) {
                struct Gore2Particle* particle =
                    &((struct Gore2Particle*)pool->user_data)[particle_index];

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
                            pool->pebbles[particle_index].matrix.pos.x =
                                particle->translation.x +
                                pool->pebbles[particle_index].matrix.pos.x;
                            pool->pebbles[particle_index].matrix.pos.y =
                                particle->translation.y +
                                pool->pebbles[particle_index].matrix.pos.y;
                            pool->pebbles[particle_index].matrix.pos.z =
                                particle->translation.z +
                                pool->pebbles[particle_index].matrix.pos.z;
                        }
                        if (!(pool->pebbles[particle_index].matrix.pos.y >
                            g_game_info.field_34 +
                                pbl_gore2_obj_list[particle_index].scale)) {
                            pool->pebbles[particle_index].matrix.pos.y =
                                g_game_info.field_34 +
                                pbl_gore2_obj_list[particle_index].scale;
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
                                    &pool->pebbles[particle_index]
                                        .matrix.pos_row.value,
                                    0, 0.0f);
                            }
                        }
                    }
                } else {
                    MkObj* owner = MK_HDR_LIVE(particle->owner.object,
                                               particle->owner.instance);
                    if (owner == 0) {
                        state->bits.visible = 0;
                    } else {
                        MKMATRIX* owner_matrix =
                            owner->field_24;
                        MKMATRIX* matrix;

                        if ((unsigned int)particle->bone != 0x40000000) {
                            MkBone* bone = owner->bones[particle->bone];

                            if (bone != 0) {
                                owner_matrix = &bone->matrix;
                            }
                        }
                        matrix = &pool->pebbles[particle_index].matrix;
                        if (particle->flags.bits.has_rotation) {
                            MKMATRIX rotation_matrix;

                            YXZ_angles_to_MKMATRIX(
                                &particle->rotation, &rotation_matrix);
                            mat_x_mat(
                                matrix, &rotation_matrix, owner_matrix);
                        } else {
                            memcpy(matrix, owner_matrix,
                                   sizeof(*matrix) - sizeof(matrix->pos_row));
                        }
                        if (particle->flags.bits.has_translation) {
                            v3_x_mat_add_v3(
                                &matrix->pos_row.value, &particle->translation,
                                owner_matrix, &owner_matrix->pos_row.value);
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
    MkPfx* particle;
    CmdScript* saved_script;
    int type;

    saved_script = active_cmdscript;
    active_cmdscript =
        get_cmdscript_for_proc(player->plyr_info->idle_proc);
    for (type = 0; type < 3; type++) {
        unsigned int emitter;

        if (((particle_mask >> type) & 1) != 0) {
            emitter = fx_next_emitter(fx_by_owner(
                mkpfx_ncs_sweat_type_map_array[type],
                1 << player->plyr_info->controller_slot));
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

static inline void resume_sweat_emitters(int particle_mask, int bone,
                                          PlyrPdata* player, MkObj* object) {
    unsigned int emitter;
    int type;

    type = 0;
    do {
        if (((particle_mask >> type) & 1) != 0) {
            emitter = fx_next_emitter(fx_by_owner(
                mkpfx_ncs_sweat_type_map_array[type],
                1 << player->plyr_info->controller_slot));
            if (emitter != 0) {
                MkPfx* particle;

                fx_resume_emit(emitter);
                particle = pfx_from_emitter(emitter);
                pfx_bind_emitter_num_to_obj_bone(
                    particle, object, bone,
                    emitter_id_from_handle(emitter));
            }
        }
        type++;
    } while (type < 3);
}

/* TODO: [near miss] 97.75%; inlined sweat loop retains nonvolatile-home residue. */
void start_sweat_particles_scripts(int particle_mask, int bone) {
    PlyrPdata* player;
    MkObj* object;
    CmdScript* saved_script;

    player = plyr_pdata;
    object = plyr_obj;
    saved_script = active_cmdscript;
    active_cmdscript =
        get_cmdscript_for_proc(player->plyr_info->idle_proc);
    resume_sweat_emitters(particle_mask, bone, player, object);
    active_cmdscript = saved_script;
}

unsigned int start_blood_particles(
    int particle_mask, unsigned int bone, PlyrPdata* player, MkObj* object) {
    unsigned int emitter;
    CmdScript* saved_script;
    int type;

    emitter = 0;
    if (bone != 0x40000000 && object->bones[bone] == 0) {
        return 0;
    }

    saved_script = active_cmdscript;
    active_cmdscript = get_cmdscript_for_proc(
        player->plyr_info->idle_proc);
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
                    if (bone == 0x40000000) {
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

/* TODO: [near miss] 98.86%; inlined blood loop player/object/bone/emitter homes reversed vs retail; coloring only. */
unsigned int start_blood_particles_scripts(int particle_mask, unsigned int bone)
{
    MkObj* object = plyr_obj;
    PlyrPdata* player = plyr_pdata;

    return start_blood_particles(particle_mask, bone, player, object);
}

extern int blood_type_list[12];
extern MkPtr* gore2_light_list;
static float p_watch_obj_for_gnd_coll(void);
static void trigger_blood_glops(
    PlyrPdata* player, int bone, MkObj* source, int blood_type) {
    struct NcsGroundCollisionWatchPdata* watcher;
    MkPfx* particle;
    MkObj* glop;
    int effect;
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

    if (effect != 0 && (particle = pfx_from_emitter(effect)) != 0 &&
        _create_mkproc_generic_nostack(
            0x601B, 0x1F, p_watch_obj_for_gnd_coll,
            sizeof(struct NcsGroundCollisionWatchPdata),
            (MkHdr**)&watcher) != 0) {
        zero_pdata_payload(
            sizeof(struct NcsGroundCollisionWatchPdata), &watcher->hdr);
        angle = frand(3.1415927f);
        for (index = 0; index < 3; index++) {
            glop = get_mkobj_frame(0x6015, 0);
            if (glop != 0) {
                Vec bone_at;
                Vec bone_right;
                float speed;
                float direction;

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
                get_bone_world_pos(source, bone, &glop->pos.value);
                mkobj_get_matrix_at(source, &bone_at);
                if (mode_of_play != 6 && player->f_constrained == 0 &&
                    (blood_type & 0x40) == 0) {
                    float offset;

                    if (is_plyr_airborn(source, player) != 0 ||
                        (source->flags_08 & 1) != 0 ||
                        (blood_type & 0x80) != 0) {
                        offset = -0.2f;
                        glop->pos.value.x += offset * bone_at.x;
                        glop->pos.value.y += offset * bone_at.y;
                        glop->pos.value.z += offset * bone_at.z;
                        glop->pos.value.y += 0.3f;
                    } else {
                        offset = -0.7f;
                        glop->pos.value.x += offset * bone_at.x;
                        glop->pos.value.y += offset * bone_at.y;
                        glop->pos.value.z += offset * bone_at.z;
                    }
                }
                mkobj_get_matrix_right(source, &bone_right);
                speed = 0.02f + frand(0.03f);
                direction = gxMathSin(angle);
                glop->pos_vel.x = direction * speed;
                glop->pos_vel.y = frand(0.005f);
                speed = 0.02f + frand(0.03f);
                direction = gxMathCos(angle);
                glop->pos_vel.z = direction * speed;
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
            glop = MK_HDR_LIVE(watcher->objects[index].object, watcher->objects[index].instance);
            if (glop != 0 && glop->hdr.instance != 0) {
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

static float p_watch_obj_for_gnd_coll(void) {
    struct NcsGroundCollisionWatchPdata* pdata;
    int index;
    int active_count;

    active_count = 0;
    pdata = (struct NcsGroundCollisionWatchPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    for (index = 0; index < 3; index++) {
        MkObj* object = MK_HDR_LIVE(pdata->objects[index].object,
            pdata->objects[index].instance);
        if (object != 0) {
            active_count++;
            if (!(object->pos.value.y > g_game_info.field_34)) {
                fx_reset_emit(pdata->emitters[index]);
                object->pos.value.y = 0.001f + g_game_info.field_34;
                spawn_bld_splat(
                    "blsplat", pdata->blood_owner, &object->pos.value);
                pdata->objects[index].object = 0;
                pdata->objects[index].instance = 0;
                if (object->hdr.instance != 0) {
                    object->hdr.typed_vtbl->destroy(&object->hdr);
                }
                pdata->emitters[index] = 0;
            }
        }
    }
    if (active_count != 0) {
        return 1.0f;
    }
    return -1.0f;
}

void spawn_blood_pool_at_bid(
    PlyrInfo* source, int bone, int large) {
    Vec position;

    get_bone_world_pos(source->slot.mirror_a, bone, &position);
    position.y = g_game_info.field_34 + 0.01f;
    if (large != 0) {
        spawn_bld_splat("blpuddle2", source->slot.fighter, &position);
    } else {
        spawn_bld_splat("blpuddle", source->slot.fighter, &position);
    }
}

void mks_spawn_blood_pool_at_bid(
    PlyrInfo* source, MkObj* object, int bone, int large) {
    Vec position;
    if ((unsigned int)bone != 0x40000000) {
        if (object != 0) {
            get_bone_world_pos(object, bone, &position);
        } else {
            get_bone_world_pos(source->slot.mirror_a, bone, &position);
        }
    } else {
        if (object != 0) {
            position.x = object->pos.value.x;
            position.y = object->pos.value.y;
            position.z = object->pos.value.z;
        } else {
            position.x = source->slot.mirror_a->pos.value.x;
            position.y = source->slot.mirror_a->pos.value.y;
            position.z = source->slot.mirror_a->pos.value.z;
        }
    }
    position.y = g_game_info.field_34 + 0.01f;
    if (large != 0) {
        spawn_bld_splat("blpuddle2", source->slot.fighter, &position);
    } else {
        spawn_bld_splat("blpuddle", source->slot.fighter, &position);
    }
}

void start_blood_splat_watcher(void) {
    int type;

    for (type = 0; type < 11; type++) {
        if (((1 << type) & 7) != 0) {
            int blood_effect;
            int decal_effect;

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
    PlyrInfo* player, int bone, int include_children) {
    MkObj* object;

    object = obj_sever_limb(
        player->slot.mirror_a, bone, 0, include_children);
    if (object != 0) {
        object->pos.value.x = -1000000.0f;
        object->pos.value.y = -1000000.0f;
        object->pos.value.z = -1000000.0f;
    }
}

static inline MkObj* limb_sever_set_motion_inline(
    MkObj* owner, int limb, const Vec* velocity,
    struct NcsLimbUpdatePdata* motion, int enable_ground,
    int ground_value, int field_18, int include_children,
    float gravity, float ground_offset, float field_11C) {
    FighterMirror* fighter;
    FighterObjectRef* ref;
    MkObj* severed = 0;

    if (motion != 0) {
        if (owner == g_game_info.plyr0.slot.mirror_a) {
            fighter = g_game_info.plyr0.slot.fighter;
        } else {
            fighter = g_game_info.plyr1.slot.fighter;
        }
        ref = &fighter->severed_limbs[limb];
        severed = MK_HDR_LIVE(ref->object, ref->instance);
        if (severed == 0) {
            severed = obj_sever_limb(owner, limb, 0, include_children);
            if (severed == 0) {
                return severed;
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
            motion->slide_end_coefficient = 1.0f;
        }
        motion->vertical_bounce_scale = field_11C;
        motion->field_18 = field_18;
        update_mkobj(severed);
    }
    return severed;
}

/* TODO: [breakthrough] 99.15%; canonical inline motion recovered; creation-failure exit adds one branch. */
MkObj* limb_sever_pop_head_up(
    PlyrInfo* player, float x_velocity, float y_velocity,
    float z_velocity, float gravity) {
    struct NcsLimbUpdatePdata* update;
    MkObj* head = 0;
    Vec world_velocity;
    Vec local_velocity;
    update = limb_sever_find_existing_update_proc(player, 0, 0x6020);
    if (update != 0) {
        local_velocity.x = x_velocity;
        local_velocity.y = y_velocity;
        local_velocity.z = z_velocity;
        v3_x_mat(&world_velocity, &local_velocity,
                 mkobj_get_matrix(player->slot.mirror_a));
        head = limb_sever_set_motion_inline(player->slot.mirror_a, 0,
            &world_velocity, update, 1, 3, 0xD2, 1, gravity, 0.01f, 0.3f);
        if (head != 0) {
            limb_sever_show_z_meat_chunks(player->slot.mirror_a, 0, 0);
        }
    }
    return head;
}

static inline MkProc* ncs_create_attach_proc(
    PlyrInfo* player, struct NcsLimbAttachPdata** pdata) {
    struct NcsLimbSet* limbset;
    MkProc* process;

    limbset = limb_sever_find_limbset(player);
    if (limbset == 0) {
        return 0;
    }
    process = _create_mkproc_generic_nostack(
        0x6017, 0x1F, p_limb_sever_attach,
        sizeof(struct NcsLimbAttachPdata), (MkHdr**)pdata);
    if (process == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(struct NcsLimbAttachPdata), &(*pdata)->hdr);
    (*pdata)->player = player;
    (*pdata)->fighter = player->slot.fighter;
    (*pdata)->limbset = limbset;
    return process;
}

static inline struct NcsLimbAttachPdata* ncs_find_attach_pdata(
    PlyrInfo* player, int limb) {
    MkPtr** list;
    MkPtr* link;
    MkProc* process;
    struct NcsLimbAttachPdata* pdata;

    list = &player->slot.fighter->attach_proc_list;
    if (list != 0) {
        link = *list;
        while (link != 0) {
            process = (MkProc*)link->hdr;
            if (link->instance != process->instance) {
                link = discard_stale_mkptr_and_advance(link);
            } else {
                if (process != 0 &&
                    (pdata = (struct NcsLimbAttachPdata*)pdata_of_proc(process)) != 0 &&
                    pdata->limb == limb) {
                    return pdata;
                }
                link = link->next;
            }
        }
    }
    process = ncs_create_attach_proc(player, &pdata);
    if (process == 0) {
        return 0;
    }
    mk_insert(&process->hdr, &player->slot.fighter->attach_proc_list);
    return pdata;
}

void limb_sever_bone_attach(
    PlyrInfo* target_player, int owner_bone,
    Vec* offset, Vec* rotation,
    PlyrInfo* owner_player, int limb, int target_bone,
    int include_children) {
    FighterMirror* fighter;
    struct NcsLimbAttachPdata* pdata;
    MkProc* process;
    MkObj* severed;

    fighter = owner_player->slot.fighter;
    process = MK_HDR_LIVE(
        fighter->limb_update_proc, fighter->limb_update_proc_instance);
    if (process != 0) {
        struct NcsLimbUpdatePdata* update =
            (struct NcsLimbUpdatePdata*)pdata_of_proc(process);

        if (update != 0) {
            update->severed_mask &= ~(1 << limb);
        }
    }
    pdata = ncs_find_attach_pdata(owner_player, limb);
    if (pdata == 0) {
        return;
    }
    severed = MK_HDR_LIVE(
        pdata->fighter->severed_limbs[limb].object,
        pdata->fighter->severed_limbs[limb].instance);
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
    pdata->owner_instance = owner_player->slot.mirror_a->hdr.instance;
    pdata->target = target_player->slot.mirror_a;
    pdata->target_instance = target_player->slot.mirror_a->hdr.instance;
    pdata->fighter->severed_limbs[limb].object = severed;
    pdata->fighter->severed_limbs[limb].instance = severed->hdr.instance;
    pdata->limb = limb;
    pdata->target_bone = target_bone;
    pdata->owner_bone = owner_bone;
    pdata->expire_tick = 600;
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

void limb_sever_explode_apart_plyr_num(
    int player, float arg1, float arg2, float strength, int mode) {
    if (player == 0) {
        limb_sever_explode_apart(
            &g_game_info.plyr0, arg1, arg2, strength, mode);
    } else if (player == 1) {
        limb_sever_explode_apart(
            &g_game_info.plyr1, arg1, arg2, strength, mode);
    }
}

/* TODO: [near miss] 98.62%; entry order and vector slots agree; inline creation-failure bne+b joins and local_velocity store residue remain. */
void limb_sever_explode_apart(
    PlyrInfo* player, float arg1, float arg2, float strength, int mode) {
    struct NcsLimbUpdatePdata* update;
    MKMATRIX* limb_matrix = force_calc_bone_world_mat(player->slot.mirror_a, 9);
    MkObj* owner = player->slot.mirror_a;
    Vec local_velocity;
    Vec world_velocity;
    Vec angular_velocity = {0.1f, 0.0f, 0.0f};
    MkObj* severed;

    update = limb_sever_find_existing_update_proc(player, -1, 0x6014);
    if (update == 0) {
        return;
    }
    init_plyr_severed_limb_list(player);
    local_velocity.x = 0.05f;
    local_velocity.z = 0.02f;
    local_velocity.y = 0.07f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 4, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 4, 0);

    local_velocity.x = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 5, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 5, 0);

    local_velocity.y = 0.05f;
    local_velocity.x = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 6, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 6, 0);

    local_velocity.x = -0.01f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.01f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 1, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 1, 0);

    local_velocity.x = -0.095f;
    local_velocity.y = 0.012f;
    local_velocity.z = -0.03f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 2, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 2, 0);

    local_velocity.y = 0.05f;
    local_velocity.x = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    severed = limb_sever_set_motion_inline(
        owner, 3, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 2, 0);

    local_velocity.x = 0.05f;
    local_velocity.y = -0.02f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 10, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 10, 0);

    local_velocity.x = 0.035f;
    local_velocity.y = 0.02f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 11, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 11, 0);

    local_velocity.x = 0.065f;
    local_velocity.y = -0.02f;
    local_velocity.z = 0.02f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 12, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 12, 0);

    local_velocity.x = -0.05f;
    local_velocity.y = 0.08f;
    local_velocity.z = -0.04f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 7, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 7, 0);

    local_velocity.x = -0.08f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 8, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 8, 0);

    local_velocity.x = -0.03f;
    local_velocity.y = 0.03f;
    local_velocity.z = 0.05f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    limb_sever_set_motion_inline(
        owner, 9, &world_velocity, update,
        1, 3, 0xD2, 1, -0.006f, 0.01f, 0.3f);
    limb_sever_show_z_meat_chunks(owner, 9, 0);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.1f;
    local_velocity.z = 0.0f;
    v3_x_mat(&world_velocity, &local_velocity, mkobj_get_matrix(owner));
    severed = limb_sever_set_motion_inline(
        owner, 0, &world_velocity, update,
        1, 2, 0xD2, 1, -0.006f, 0.1f, 0.0001f);
    zero_v3(&angular_velocity);
    angular_velocity.z = 0.085f;
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 0, 0);

    zero_v3(&world_velocity);
    severed = mks_limb_sever_inline(owner, 14, 1);
    world_velocity.y = -1000.0f;
    obj_set_pos(severed, &world_velocity);
    limb_sever_show_z_meat_chunks(owner, 14, 0);

    mks_limb_sever_inline(owner, 13, 1);

    local_velocity.x = 0.0f;
    local_velocity.y = 0.06f;
    local_velocity.z = 0.04f;
    v3_x_mat(&world_velocity, &local_velocity, limb_matrix);
    obj_set_pos_vel(owner, &world_velocity);

    severed = limb_sever_set_motion_inline(
        owner, 13, &world_velocity, update,
        1, 3, 1000, 1, -0.006f, 0.01f, 0.3f);
    zero_v3(&angular_velocity);
    angular_velocity.z = 0.1f;
    obj_set_ang_vel(severed, &angular_velocity);
    limb_sever_show_z_meat_chunks(owner, 13, 0);
}

MkObj* mks_limb_sever(
    MkObj* object, int limb, int include_children) {
    FighterMirror* fighter;
    MkObj* severed;

    fighter = object == g_game_info.plyr0.slot.mirror_a
        ? g_game_info.plyr0.slot.fighter : g_game_info.plyr1.slot.fighter;
    severed = MK_HDR_LIVE(fighter->severed_limbs[limb].object, fighter->severed_limbs[limb].instance);
    if (severed == 0) {
        severed = obj_sever_limb(object, limb, 0, include_children);
        if (severed != 0) {
            fighter->severed_limbs[limb].object = severed;
            fighter->severed_limbs[limb].instance = severed->hdr.instance;
            severed->light_flags = object->light_flags;
        }
    }
    return severed;
}

void limb_sever_destroy_existing_attach_proc(
    PlyrInfo* player, int limb) {
    MkPtr* next;
    MkPtr** list;
    MkPtr* link;

    list = &player->slot.fighter->attach_proc_list;
    if (list != 0) {
        link = *list;
        while (link != 0) {
            MkHdr* object = link->hdr;

            if (link->instance != object->instance) {
                next = link->next;

                discard_stale_mkptr(link);
                link = next;
            } else {
                MkProc* proc = (MkProc*)object;

                if (proc != 0) {
                    struct NcsLimbAttachPdata* pdata =
                        (struct NcsLimbAttachPdata*)pdata_of_proc(proc);

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

static inline MkProc* limb_sever_create_update_proc(
    PlyrInfo* player, int proc_id, struct NcsLimbUpdatePdata** pdata) {
    MkProc* proc;
    void* limbset = limb_sever_find_limbset(player);

    if (limbset == 0) {
        return 0;
    }
    proc = _create_mkproc_generic_nostack(
        proc_id, 0x1F, p_limb_sever_update,
        sizeof(struct NcsLimbUpdatePdata), (MkHdr**)pdata);
    if (proc == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(struct NcsLimbUpdatePdata), &(*pdata)->hdr);
    (*pdata)->player = player;
    (*pdata)->fighter = player->slot.fighter;
    (*pdata)->limbset = limbset;
    return proc;
}

/* TODO: [near miss] 98.72%; recovered list and creation boundaries;
 * latch branch and creation-result register remain. */
struct NcsLimbUpdatePdata* limb_sever_find_existing_update_proc(
    PlyrInfo* player, int limb, int proc_id) {
    FighterMirror* fighter;
    MkPtr* link;
    MkPtr** list;
    MkProc* update_proc;
    struct NcsLimbUpdatePdata* pdata;

    fighter = player->slot.fighter;
    list = &fighter->attach_proc_list;
    if (list != 0) {
        link = *list;
        while (link != 0) {
            MkProc* proc = (MkProc*)link->hdr;

            if (link->instance != proc->instance) {
                MkPtr* next = link->next;

                discard_stale_mkptr(link);
                link = next;
            } else {
                struct NcsLimbAttachPdata* attach_pdata;

                if (proc != 0 &&
                    (attach_pdata = (struct NcsLimbAttachPdata*)pdata_of_proc(proc)) != 0 &&
                    attach_pdata->limb == limb) {
                    if (proc->instance != 0) {
                        proc->hdr.typed_vtbl->destroy(&proc->hdr);
                    }
                    break;
                }
                link = link->next;
            }
        }

    }
    fighter = player->slot.fighter;
    update_proc = fighter->limb_update_proc;
    if (update_proc != 0) {
        if (update_proc->instance != fighter->limb_update_proc_instance) {
            update_proc = 0;
        }
    } else {
        update_proc = 0;
    }
    if (update_proc == 0 ||
        (pdata = (struct NcsLimbUpdatePdata*)pdata_of_proc(update_proc)) == 0) {
        update_proc = limb_sever_create_update_proc(player, proc_id, &pdata);
        if (update_proc == 0) {
            return 0;
        }
        player->slot.fighter->limb_update_proc = update_proc;
        player->slot.fighter->limb_update_proc_instance = update_proc->instance;
    }
    pdata->expire_tick = exec_tick_ctr + 60;
    return pdata;
}

MkProc* plyr_spawn_his_anim_limb(
    PlyrInfo* player, int limb, int include_children,
    AniData* animation_script, int transition, MkProcEntryFn entry,
    float animation_step) {
    AnimPdata* animation;
    MkProc* proc;
    MkObj* severed;

    proc = create_mkproc_anim2(0x9012, entry, &animation);
    if (proc != 0) {
        severed = obj_sever_limb(
            player->slot.mirror_a, limb, 0, include_children);
        if (severed == 0) {
            if (proc->instance != 0) {
                proc->hdr.typed_vtbl->destroy(&proc->hdr);
            }
            proc = 0;
        } else {
            severed->ground_colls = plyr_obj->ground_colls;
            severed->ground_colls_y = plyr_obj->ground_colls_y;
            severed->flags_09_bits.launched = 1;
            insert_ground_me_mkobj(severed);
            severed->light_flags = player->slot.mirror_a->light_flags;
            animation->obj = severed;
            animation->obj_instance = severed->hdr.instance;
            player->slot.fighter->severed_limbs[limb].object = severed;
            player->slot.fighter->severed_limbs[limb].instance =
                severed->hdr.instance;
            severed->ground_colls = player->slot.mirror_a->ground_colls;
            severed->ground_colls_y = player->slot.mirror_a->ground_colls_y;
            severed->flags_0B_bits.pivot_enabled = 0;
            set_root_and_obj_movement_weights(animation, 0.0f, 1.0f);
            set_anim_script(animation, animation_script, transition);
            animation->step = animation_step;
        }
    }
    return proc;
}

/* TODO: [near miss] 99.01%; creation-failure early return lowers as bne+b instead of retail beq to final result. */
MkObj* limb_sever_set_motion(
    MkObj* owner, int limb, Vec* velocity, float gravity,
    struct NcsLimbUpdatePdata* motion, int enable_ground, float ground_offset,
    int ground_value, float vertical_bounce_scale, int field_18, int include_children) {
    MkObj* severed = 0;

    if (motion != 0) {
        FighterMirror* fighter;
        fighter = owner == g_game_info.plyr0.slot.mirror_a
            ? g_game_info.plyr0.slot.fighter : g_game_info.plyr1.slot.fighter;
        severed = MK_HDR_LIVE(fighter->severed_limbs[limb].object,
            fighter->severed_limbs[limb].instance);
        if (severed == 0) {
            severed = obj_sever_limb(owner, limb, 0, include_children);
            if (severed == 0) {
                return severed;
            }
            fighter->severed_limbs[limb].object = severed;
            fighter->severed_limbs[limb].instance = severed->hdr.instance;
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
            motion->slide_end_coefficient = 1.0f;
        }
        motion->vertical_bounce_scale = vertical_bounce_scale;
        motion->field_18 = field_18;
        update_mkobj(severed);
    }
    return severed;
}

static float p_limb_sever_attach(void) {
    struct NcsLimbAttachPdata* pdata;
    MkObj* severed;
    MkObj* owner;
    MkObj* target;
    MKMATRIX rotation_matrix;
    MKMATRIX* severed_matrix;
    MKMATRIX* target_matrix;
    MKMATRIX* source_matrix;
    Vec offset;

    pdata = (struct NcsLimbAttachPdata*)apdata;
    if (pdata == 0) {
        mkproc_die();
    }

    severed = MK_HDR_LIVE(pdata->fighter->severed_limbs[pdata->limb].object,
        pdata->fighter->severed_limbs[pdata->limb].instance);
    if (severed == 0) {
        return -1.0f;
    }
    owner = MK_HDR_LIVE(pdata->owner, pdata->owner_instance);
    if (owner == 0) {
        return -1.0f;
    }
    target = MK_HDR_LIVE(pdata->target, pdata->target_instance);
    if (target == 0) {
        return -1.0f;
    }
    if (((pdata->limbset->active_mask >> pdata->limb) & 1U) == 0) {
        if (severed->hdr.instance != 0) {
            severed->hdr.typed_vtbl->destroy(&severed->hdr);
        }
        return -1.0f;
    }
    if (pdata->expire_tick < 0) {
        return -1.0f;
    }

    target_matrix = target->field_24;
    if (pdata->owner_bone >= 0) {
        MkBone* target_bone = target->bones[pdata->owner_bone];

        if (target_bone != 0) {
            target_matrix = &target_bone->matrix;
        }
    }
    severed_matrix = severed->field_24;
    YXZ_angles_to_MKMATRIX(&pdata->rotation, &rotation_matrix);
    mat_x_mat(severed_matrix, &rotation_matrix, target_matrix);
    if (severed->flags_08_bits.scale_active) {
        mat_scaled_by_v3(
            severed_matrix, severed_matrix, &severed->scale);
    }

    source_matrix = severed
        ->bones[limbbid_bid_map[pdata->target_bone]->bone_id]->parent_matrix;
    offset.x = pdata->offset.x - source_matrix->pos.x;
    offset.y = pdata->offset.y - source_matrix->pos.y;
    offset.z = pdata->offset.z - source_matrix->pos.z;
    v3_x_mat_add_v3(
        &severed->pos.value, &offset, target_matrix, &target_matrix->pos_row.value);
    severed_matrix->pos.x = severed->pos.value.x;
    severed_matrix->pos.y = severed->pos.value.y;
    severed_matrix->pos.z = severed->pos.value.z;
    update_mkobj(severed);
    pdata->expire_tick--;
    return 1.0f;
}

static float p_limb_sever_update(void) {
    struct NcsLimbUpdatePdata* pdata;
    int limb;

    pdata = (struct NcsLimbUpdatePdata*)apdata;
    if (pdata == 0) {
        mkproc_die();
    }
    if (pdata->field_18 < 0 || pdata->limbset == 0) {
        return -1.0f;
    }

    for (limb = 0; limb < 15; limb++) {
        if (((pdata->severed_mask >> limb) & 1) != 0 &&
            ((pdata->limbset->active_mask >> limb) & 1) != 0) {
            MkObj* object = MK_HDR_LIVE(
                pdata->fighter->severed_limbs[limb].object,
                pdata->fighter->severed_limbs[limb].instance);

            if (((pdata->gravity_trigger_mask >> limb) & 1) != 0 &&
                object->pos_vel.y < pdata->horizontal_bounce_scale) {
                pdata->gravity_trigger_mask &= ~(1U << limb);
                object->gravity = pdata->bounce_gravity;
            }
            if (((pdata->x_collision_mask >> limb) & 1) != 0 &&
                ((object->pos_vel.x >= 0.0f &&
                  object->pos.value.x > pdata->x_collision_limit[limb]) ||
                 (object->pos_vel.x < 0.0f &&
                  object->pos.value.x < pdata->x_collision_limit[limb]))) {
                object->pos_vel.x *= -1.0f;
                pdata->x_collision_mask &= ~(1U << limb);
                if (((pdata->damp_bounce_mask >> limb) & 1) != 0) {
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.x *= pdata->horizontal_bounce_scale;
                }
            }
            if (((pdata->z_collision_mask >> limb) & 1) != 0 &&
                ((object->pos_vel.z >= 0.0f &&
                  object->pos.value.z > pdata->z_collision_limit[limb]) ||
                 (object->pos_vel.z < 0.0f &&
                  object->pos.value.z < pdata->z_collision_limit[limb]))) {
                object->pos_vel.z *= -1.0f;
                pdata->z_collision_mask &= ~(1U << limb);
                if (((pdata->damp_bounce_mask >> limb) & 1) != 0) {
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.z *= pdata->horizontal_bounce_scale;
                }
            }

            if (((pdata->ground_mask >> limb) & 1) == 0 ||
                object->pos.value.y > pdata->ground_height[limb]) {
                object->pos_vel.x *= pdata->slide_end_coefficient;
                object->pos_vel.z *= pdata->slide_end_coefficient;
            } else if (pdata->ground_value[limb] != 0) {
                int bounce_count = pdata->ground_value[limb];

                object->pos_vel.y =
                    (float)bounce_count *
                    (-object->pos_vel.y * pdata->vertical_bounce_scale);
                pdata->ground_value[limb]--;
                random_hit(0x0B);
                if (((pdata->first_bounce_mask >> limb) & 1) != 0) {
                    pdata->first_bounce_mask &= ~(1U << limb);
                    object->gravity = pdata->bounce_gravity;
                    object->pos_vel.x *= pdata->horizontal_bounce_scale;
                    object->pos_vel.z *= pdata->horizontal_bounce_scale;
                }
            } else {
                Vec position;

                object->pos.value.y = pdata->ground_height[limb];
                object->pos_vel.y = 0.0f;
                object->flags_08_bits.rotation_enabled = 0;
                object->flags_08_bits.moving = 0;
                object->flags_09_bits.launched = 1;
                object->flags_09_bits.bit6 = 1;
                if (((pdata->slide_mask >> limb) & 1) != 0) {
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
                    if (object->pos_vel.x != 0.0f ||
                        object->pos_vel.z != 0.0f) {
                        continue;
                    }
                    pdata->slide_mask &= ~(1U << limb);
                } else {
                    object->flags_08_bits.gravity_enabled = 0;
                }

                position.x = object->pos.value.x;
                position.y = g_game_info.field_34 + 0.0005f;
                position.z = object->pos.value.z;
                spawn_bld_splat("blpuddle", pdata->fighter, &position);
                if ((unsigned int)g_game_info.field_218 <
                        (unsigned int)exec_tick_ctr &&
                    (unsigned int)exec_tick_ctr <
                        (unsigned int)pdata->expire_tick) {
                    random_hit(0x0C);
                    g_game_info.field_218 =
                        exec_tick_ctr + (unsigned short)randu0(10) + 15;
                }
                pdata->ground_mask &= ~(1U << limb);
            }
        }
    }
    pdata->field_18--;
    return 1.0f;
}

void animpdata_ani_to_frame_x_with_flag_check(
    AnimPdata* animation, int zone_check, int grab_check,
    float target_frame) {
    MkVtableMkproc* proc_vtbl;

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
        proc_vtbl = aproc->vtbl;
        proc_vtbl->sleep();
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

void mkscripts_destroy_fk_bonematcher(MkHdr* bonematcher) {
    if (bonematcher->instance != 0) {
        bonematcher->typed_vtbl->destroy(bonematcher);
    }
}

void mkscripts_destroy_bonematcher(MkHdr* bonematcher) {
    if (bonematcher->instance != 0) {
        bonematcher->typed_vtbl->destroy(bonematcher);
    }
}

void mkscripts_destroy_gusher(MkHdr* gusher) {
    if (gusher->instance != 0) {
        gusher->typed_vtbl->destroy(gusher);
    }
}

MslSoundHandle play_his_snd_req(int sound) {
    PlyrPdata* player;
    MslSoundHandle handle;

    player = plyr_pdata;
    plyr_pdata = player->his_plyr_pdata;
    handle = plyr_snd_req(sound);
    plyr_pdata = plyr_pdata->his_plyr_pdata;
    return handle;
}

MslSoundHandle play_his_random_voice(int sound) {
    PlyrPdata* player;
    MslSoundHandle handle;

    player = plyr_pdata;
    plyr_pdata = player->his_plyr_pdata;
    handle = random_voice(sound);
    plyr_pdata = plyr_pdata->his_plyr_pdata;
    return handle;
}

int pfx_plyr_bankowner(const PlyrInfo* player) {
    return 1 << player->controller_slot;
}

void limb_sever_update_slide_end_coeff(
    struct NcsLimbUpdatePdata* limb, float coefficient) {
    int index;

    if (limb != 0) {
        limb->slide_deceleration = coefficient;
        for (index = 0; index < 15; index++) {
            limb->slide_mask |= 1 << index;
        }
    }
}

MkProc* proc_of_anim_pdata(AnimPdata* data) {
    return MK_LIVE(data->proc, data->proc_instance);
}

void set_pdata_anim_step(AnimPdata* pdata, float step) {
    pdata->step = step;
}

float mkobj_pos_pos_dot_normal_xz(
    MkObj* from, const MkObj* to, const Vec* normal) {
    float dx;
    float dz;
    float squared;
    float inverse_length;

    dx = to->pos.value.x - from->pos.value.x;
    dz = to->pos.value.z - from->pos.value.z;
    squared = dx * dx + dz * dz;
    inverse_length = gxMathFastInvSqrt(squared);
    dx *= inverse_length;
    dz *= inverse_length;
    return normal->x * dx + normal->z * dz;
}

void ncs_script_debug_quickie(int command, float value) {
    if (command != -1) {
        return;
    }
    if (value == -1.0f) {
        return;
    }
}
