#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/image.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/cam.h"
#include "game/game_info.h"
#include "platform/display.h"
#include "platform/main.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "rw/rtquat.h"

typedef struct NcsDestroyable NcsDestroyable;
typedef struct SpearProcPdata SpearProcPdata;
typedef int (*NcsDestroyFn)(NcsDestroyable* object);

typedef struct NcsProcVtable {
    void* reserved[6];
    void (*sleep)(struct NcsProcVtable* vtbl);
    void* reserved_after_sleep[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
} NcsProcVtable;

extern MkObj* sc_spear_obj;
extern SpearProcPdata* pdata_sc_spear;
extern int f_fatality_was_done;

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
    char pad08[8];
    MkObj* parent;               /* +0x10 */
    unsigned int parent_instance; /* +0x14 */
} NcsBoneMatcher;

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
    char pad38[4];
    struct NcsSpearEffect* effect; /* +0x3C */
    unsigned int effect_instance; /* +0x40 */
    int field_44;
};

typedef struct NcsSpearEffect {
    MkHdr hdr;
    char pad08[0x280];
    int active; /* +0x288 */
    char pad28C[0x0C];
    float field_298;
    float field_29C;
    float field_2A0;
    float field_2A4;
    float field_2A8;
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

typedef union Gore2ParticleFlags {
    unsigned char value;
    struct {
        unsigned char active : 1;
        unsigned char pad : 7;
    } bits;
} Gore2ParticleFlags;

typedef struct Gore2ParticleState {
    Gore2ParticleFlags flags;
    unsigned char pad01[3];
} Gore2ParticleState; /* 0x04 */

typedef struct Gore2Pool {
    char pad00[0x10];
    int capacity; /* +0x10 */
    char pad14[8];
    Gore2ParticleState* particles; /* +0x1C */
} Gore2Pool;

typedef struct Gore2UpdatePdata {
    MkHdr hdr;
    Gore2Pool* pools[10]; /* +0x08 */
    int next_particle[10]; /* +0x30 */
} Gore2UpdatePdata;

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
unsigned int fx_next_emitter(unsigned int handle);
void fx_resume_emit(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
int emitter_id_from_handle(unsigned int handle);
void advance_anim(AnimPdata* animation);
void pose_anim(AnimPdata* animation, int update_object);
MkProc* fire_sc_spear(
    PlyrPdata* player, const Vec* velocity, int field_34,
    int flag_40, MkHdr* bound_object, int flag_20);

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
    unsigned char alpha;
    int vertex;

    fading_screen.alpha -= 4.0f * game_speed;
    if (fading_screen.alpha <= 0.0f) {
        DeleteCameraSnapShot();
        return -1.0f;
    }

    alpha = (unsigned char)fading_screen.alpha;
    for (vertex = 0; vertex < 4; vertex++) {
        fading_screen.fade_obj->pfx2d->verts[vertex].a = alpha;
    }
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

    weapon->flags_08 &= (unsigned char)~0x20;
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

    weapon->flags_08 &= (unsigned char)~0x20;
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

void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void spawn_bld_splat(
    const char* name, MkObj* object, const Vec* position);

void spawn_blood_pool_at_bid(
    NcsBloodPoolSource* source, int bone, int large) {
    Vec position;

    get_bone_world_pos(source->bone_owner, bone, &position);
    position.y = g_game_info.field_34 + 0.01f;
    spawn_bld_splat(
        large != 0 ? "blpuddle2" : "blpuddle",
        source->splat_owner, &position);
}

void mks_spawn_blood_pool_at_bid(
    NcsBloodPoolSource* source, MkObj* object, int bone, int large) {
    Vec position;
    MkObj* position_object;

    if (object != 0) {
        position_object = object;
    } else {
        position_object = source->bone_owner;
    }
    if ((unsigned int)bone != 0x40000000) {
        get_bone_world_pos(position_object, bone, &position);
    } else {
        position.x = position_object->pos.x;
        position.y = position_object->pos.y;
        position.z = position_object->pos.z;
    }
    position.y = g_game_info.field_34 + 0.01f;
    if (large != 0) {
        spawn_bld_splat("blpuddle2", source->splat_owner, &position);
    } else {
        spawn_bld_splat("blpuddle", source->splat_owner, &position);
    }
}

static float p_sc_spear3(void);
static float p_sc_spear4(void);

static float p_sc_spear3_pre(void) {
    Vec angle;

    v3_to_xy_ang(&angle, (const Vec*)&sc_spear_obj->field_24->at);
    ((NcsProcVtable*)aproc->vtbl)->jump_sleep(p_sc_spear3, 50.0f);
    return 50.0f;
}

static NcsSpearEffect* ncs_get_spear_effect(void) {
    NcsSpearEffect* effect;

    effect = pdata_sc_spear->effect;
    if (effect != 0 &&
        effect->hdr.instance != pdata_sc_spear->effect_instance) {
        effect = 0;
    }
    return effect;
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

    sc_spear_obj->flags_08 &= (unsigned char)~0x20;
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

extern SpearProcPdata* pdata_sc_spear;
extern MkObj* sc_spear_obj;
extern PlyrPdata* his_pdata;
extern MkObj* his_obj;
extern int cur_zone_check;
extern int cur_grab_check;
extern Gore2UpdatePdata* mkpdata_pbl_gore2_update;

void obj_create_sobjs(MkObj* object);
void sobj_set_priority(void* object, int priority);
float p_sc_spear_retract(void);
float p_sc_spear_retract_victory(void);
MslSoundHandle plyr_snd_req(int sound);
void random_voice(int sound);
void limb_sever_explode_apart(PlyrInfo* player);
MkObj* obj_sever_limb(
    MkObj* object, int limb, Vec* limb_velocities, int include_children);

void mkscripts_mkobj_insert_mkobj_cleanuplist(
    MkHdr* object, MkObj* owner) {
    mk_insert(object, &owner->child_list);
}

void xfer_spearproc_to_retract(void) {
    xfer_proc(aproc, p_sc_spear_retract);
}

void retract_spear_from_camera(void) {
    xfer_proc(aproc, p_sc_spear_retract_victory);
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

void limb_sever_explode_apart_plyr_num(int player) {
    if (player == 0) {
        limb_sever_explode_apart(&g_game_info.plyr0);
    } else if (player == 1) {
        limb_sever_explode_apart(&g_game_info.plyr1);
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
    severed->flags_09 |= 0x80;
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

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
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
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

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

void stop_prison_grab_proc(void) {
    if (find_mkproc_pid(0x603D) != 0 &&
        g_game_info.bgnd_id == 2) {
        destroy_mkprocs_pid(0x603D);
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

void sc_spear_postsleep(void) {
    pdata_sc_spear = 0;
    sc_spear_obj = 0;
    his_pdata = 0;
    his_obj = 0;
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

void mkscripts_set_anim_check_flag(int zone_check, int grab_check) {
    cur_zone_check = zone_check;
    cur_grab_check = grab_check;
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

static const char* mkpfx_ncs_sweat_type_map_array[4] = {
    "swtrsh",
    "swexsw",
    "swexfs",
    0,
};

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

int pfx_plyr_bankowner(const PfxPlayerBankOwner* player) {
    return 1 << player->player_index;
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
    pool->particles[particle_index].flags.bits.active = 0;
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
    return normal->x * (dx * inverse_length) +
           normal->z * (dz * inverse_length);
}

void set_pdata_anim_step(AnimPdata* pdata, float step) {
    pdata->step = step;
}

void ncs_script_debug_quickie(int command, float value) {
    if (command != -1) {
        return;
    }
    if (value == -1.0f) {
        return;
    }
}
