#include "game/collision.h"
#include "game/game_info.h"
#include "platform/display.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "platform/io.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/image.h"
#include "runtime/cam.h"
#include "runtime/utils.h"

typedef struct LightDef LightDef;

typedef struct MiscBgndData {
    int collision_object_id;
    int collision_object_id2;
    void* object;
    float test_float;
    unsigned int test_value;
} MiscBgndData;

typedef struct PlayerBodyExplodePdata {
    MkHdr hdr;
    PlyrInfo* player;
    Vec direction;
    float scale;
} PlayerBodyExplodePdata;

typedef struct MkObjRef {
    MkObj* object;
    unsigned int instance;
} MkObjRef;

typedef struct FishScreamPdata {
    MkHdr hdr;
    int player_index;
} FishScreamPdata;

typedef struct FishAttackPdata {
    MkHdr hdr;
    int fish_index; /* +0x08 */
    int lifetime; /* +0x0C */
    int state_ticks; /* +0x10 */
    int state; /* +0x14 */
    int turn_step; /* +0x18 */
    int turn_limit; /* +0x1C */
    int turning_left; /* +0x20 */
    int active; /* +0x24 */
    struct FishModelPair* models; /* +0x28 */
    int use_good_fish; /* +0x2C */
    MkObj* target; /* +0x30 */
    unsigned int target_instance; /* +0x34 */
    MkObj* fish; /* +0x38 */
    unsigned int fish_instance; /* +0x3C */
} FishAttackPdata;

typedef struct FishModelPair {
    MkObj* bad_fish;
    MkObj* good_fish;
    MkObj* reserved;
} FishModelPair;

typedef struct FishAttackData {
    int target_bone;
    int model_index;
    int lifetime;
    float offset_x;
    float offset_y;
    float offset_z;
    float field_18;
} FishAttackData;

typedef struct MabSkinnedLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
    float field_1c;
    float field_20;
    float field_24;
} MabSkinnedLightDef;

typedef struct MabGenericPositionPdata {
    MkHdr hdr;
    char pad08[0x10];
    Vec position; /* +0x18 */
} MabGenericPositionPdata;

typedef struct CameraBouncePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float saved_gravity;
    float trigger_distance;
    float velocity_scale;
    float direction_scale;
} CameraBouncePdata;

typedef struct SkyTempleExplodeMonitorPdata {
    MkHdr hdr;
    PlyrInfo* player;
} SkyTempleExplodeMonitorPdata;

typedef struct GusherStep {
    const char* blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

#define RESOLVE_MAB_OBJECT(result, object, expected_instance)              \
    do {                                                                  \
        MkObj* candidate_;                                                \
        candidate_ = (object);                                           \
        if (candidate_ != 0) {                                           \
            if (candidate_->hdr.instance == (expected_instance)) {       \
                (result) = candidate_;                                   \
            } else {                                                     \
                (result) = 0;                                            \
            }                                                            \
        } else {                                                         \
            (result) = 0;                                                \
        }                                                                \
    } while (0)

double fabs(double value);

typedef struct FenceSection {
    float offset_z;
    char pad04[4];
    float offset_x;
    char pad0C[4];
    void* marker;
    float x;
    float z;
} FenceSection;

typedef struct SkyTempleBodysplatPdata {
    MkHdr hdr;
    char pad08[0x10];
    Vec position;
} SkyTempleBodysplatPdata;

typedef union ObjectMonitorScalar {
    float value;
    unsigned int bits;
} ObjectMonitorScalar;

typedef struct ObjectMonitorThresholdTriplet {
    ObjectMonitorScalar x;
    ObjectMonitorScalar y;
    ObjectMonitorScalar z;
} ObjectMonitorThresholdTriplet;

typedef struct ObjectMonitorThresholds {
    ObjectMonitorThresholdTriplet min_pos;
    ObjectMonitorThresholdTriplet min_vel;
} ObjectMonitorThresholds;

typedef struct ObjectMonitorPdata {
    MkHdr hdr;
    int state;
    ObjectMonitorThresholds thresholds;
    float velocity_scale;
    float vertical_step;
    float settle_height;
    void (*callback)(MkSobj* object);
    MkHdr* target;
    unsigned int target_instance;
} ObjectMonitorPdata;

typedef struct ObjectMonitorConfig {
    MkHdr* target;
    ObjectMonitorThresholds thresholds;
    float velocity_scale;
    float vertical_step;
    float settle_height;
    void (*callback)(MkSobj* object);
} ObjectMonitorConfig;

typedef MkProc* (*CreateObjectMonitorProcFn)(
    int proc_id, int priority, MkProcEntryFn proc_fn, int pdata_size,
    void* pdata_out, float vertical_step, float min_pos_y, float min_pos_x,
    float velocity_scale);

typedef struct ObjectMonitorSpawnLocals {
    ObjectMonitorPdata* pdata;
    ObjectMonitorConfig config;
} ObjectMonitorSpawnLocals;

typedef struct FishObjectLatch {
    char pad00[0x10];
    MkObj* object;
    unsigned int instance;
    char pad18[0x18];
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float field_38;
} FishObjectLatch;

typedef struct YinyangFishPair {
    MkObj* good_fish;
    MkObj* bad_fish;
    FishObjectLatch* active_fish;
} YinyangFishPair;

int yy_evil_time_active;
int yinyang_ok_to_switch;
int yinyang_good_music_index;
int yinyang_evil_music_index;
MslSoundHandle yinyang_current_music;
static MiscBgndData misc_bgnd_data;
static CollisionShape fortress_exclusion_zone;
static MkObjRef debug_p2_axis_item;
static MkObjRef debug_p1_axis_item;

extern MkObj* plyr_obj;

MslSoundHandle snd_req(int sound_id);
void snd_stop(MslSoundHandle handle);
MslSoundHandle plyr_snd_req(int sound_id);
void init_collision_system(void);
int sprintf(char* dest, const char* format, ...);
int is_weapon_style(int style);
void advance_active_moveset(FighterMirror* fighter);
static float p_player_body_explode(void);
static float p_xpd_obj_monitor(void);
int build_bones_tbl(MkObj* object, const int* tags);
MkObj* load_model_from_slot(int slot, unsigned int model_id, int heap_id);
MkObj* load_named_model_from_slot(
    int slot, const char* name, int object_id, int flags);
MslSoundHandle plyr_snd_req_no_plyr_proc(
    FighterMirror* fighter, int sound_id);
MkObj* obj_sever_limb(
    MkObj* object, int limb, Vec* limb_velocities, int include_children);
void limb_sever_show_z_meat_chunks(
    MkObj* object, int limb, int include_children);
void limb_sever_show_z_meat_chunks_all(MkObj* object);
void bgnd_hide_mirror_guys(void);
float p_anim_idle(void);
void mkobj_zero_bone_rots(MkObj* object);
void add_facial_damage(FighterMirror* fighter, float amount);
void shake_camera(int strength, MkHdr* pdata, float duration);
void* start_gusher(
    GusherStep* steps, FighterMirror* owner, MkObj* object, int bone,
    const Vec* position, const Vec* direction);
extern GusherStep heart_beat[];
extern MkPtr* gusher_list;
float uv_v3_to_v3_dist(Vec* out, const Vec* from, const Vec* to);
static float p_monitor_objs_sobjs(void);
void p_statue_xpd_callback(MkSobj* object);
float p_fish_attack_sounds(void);
float p_fish_attack(void);
static float p_fish_attack_bloodsplat(void);
static float p_cam_bounce_monitor(void);
extern CameraObj* camera_obj;
Vec* sobj_get_world_pos(void* object);
void bgnd_launch_fx_at_position(
    const char* effect, float x, float y, float z);
void obj_change_to_skinned_obj_light_list(MkObj* object, LightDef* light);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
float p_track_cam_ang_y_light(void);

const char* rock_xpd_effects[6] = {
    "rock_explode_effect", "rock_explode_effect1", "rock_explode_effect2",
    "rock_explode_effect3", "rock_explode_effect4", "rock_explode_effect5",
};
const char* rock_dust_effects[6] = {
    "dust_gnd_pnd", "dust_gnd_pnd1", "dust_gnd_pnd2",
    "dust_gnd_pnd3", "dust_gnd_pnd4", "dust_gnd_pnd5",
};

FishAttackData fish_data_tbl[13] = {
    {0x19, 1, 0xDC, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x18, 4, 0xE2, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x08, 7, 0xE0, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x07, 0xA, 0xE8, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x15, 3, 0xF6, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x14, 6, 0x100, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x04, 0xB, 0xFB, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x05, 8, 0x10A, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x02, 9, 0x117, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x01, 0xC, 0xF8, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x03, 0xD, 0xEC, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x10, 0, 0xEE, -0.05f, 0.0f, 0.0f, 0.0f},
    {0, 0x0E, 0xFF, -0.25f, 1.57f, 0.0f, 0.0f},
};

static MabSkinnedLightDef skinned_obj_light_def = {
    3, p_track_cam_ang_y_light, 0,
    {1.0f, 1.0f, 1.0f, 1.0f},
    6.010f, 3.190f, 0.0f,
};

int fish_bones[2] = {1, 0};

int evil_tune_tbl[3] = {0x1BF3, 0x1BF4, 0x1BF5};
int good_tune_tbl[4] = {0x1BEC, 0x1BED, 0x1BEE, 0x1BEF};

void ck_put_weapon_away(PlyrInfo* player) {
    if (player != 0 &&
        is_weapon_style(player->slot.fighter->active_moveset) != 0) {
        advance_active_moveset(g_game_info.active_player->slot.fighter);
    }
}

void fortress_setup_exclusion_zone(
    const Vec* center, float width, float height, float depth, float angle) {
    build_col_shape_vertical_box(
        &fortress_exclusion_zone, center, width, height, depth, angle);
}

int is_point_in_fortress_exclusion_zone(const Vec* point) {
    return is_point_inside_shape(&fortress_exclusion_zone, point) != 0;
}

void mab_test(void) {
}

void yinyang_play_evil_tune(void) {
    yinyang_evil_music_index++;
    if ((unsigned int)yinyang_evil_music_index >= 3) {
        yinyang_evil_music_index = 0;
    }
    if ((unsigned int)yinyang_evil_music_index >= 3) {
        yinyang_evil_music_index = 0;
    }
    if ((unsigned int)yinyang_good_music_index >= 4) {
        yinyang_good_music_index = 0;
    }
    if (yy_evil_time_active != 0) {
        yinyang_current_music =
            snd_req(evil_tune_tbl[yinyang_evil_music_index]);
    } else {
        yinyang_current_music =
            snd_req(good_tune_tbl[yinyang_good_music_index]);
    }
}

void yinyang_play_good_tune(void) {
    yinyang_good_music_index++;
    if ((unsigned int)yinyang_good_music_index >= 4) {
        yinyang_good_music_index = 1;
    }
    if ((unsigned int)yinyang_evil_music_index >= 3) {
        yinyang_evil_music_index = 0;
    }
    if ((unsigned int)yinyang_good_music_index >= 4) {
        yinyang_good_music_index = 0;
    }
    if (yy_evil_time_active != 0) {
        yinyang_current_music =
            snd_req(evil_tune_tbl[yinyang_evil_music_index]);
    } else {
        yinyang_current_music =
            snd_req(good_tune_tbl[yinyang_good_music_index]);
    }
}

/* Soft ceiling: yinyang_finish_music ~99.53% -- SDA relocation only. */
void yinyang_finish_music(void) {
    if (yinyang_current_music != 0) {
        snd_stop(yinyang_current_music);
        yinyang_current_music = 0;
    }

    if (yy_evil_time_active == 0) {
        snd_req(0x1BF1);
    } else {
        snd_req(0x1BF7);
    }
    if (yy_evil_time_active == 0) {
        yinyang_current_music = snd_req(0x1BF0);
    } else {
        yinyang_current_music = snd_req(0x1BF6);
    }
}

/* Soft ceiling: yinyang_stop_music ~99.67% -- SDA relocation only. */
void yinyang_stop_music(void) {
    if (yinyang_current_music != 0) {
        snd_stop(yinyang_current_music);
        yinyang_current_music = 0;
    }

    if (yy_evil_time_active == 0) {
        snd_req(0x1BF1);
    } else {
        snd_req(0x1BF7);
    }
}

/* Soft ceiling: yinyang_start_music ~98.68% -- SDA relocation only. */
void yinyang_start_music(void) {
    if ((unsigned int)yinyang_evil_music_index >= 3) {
        yinyang_evil_music_index = 0;
    }
    if ((unsigned int)yinyang_good_music_index >= 4) {
        yinyang_good_music_index = 0;
    }

    if (yy_evil_time_active == 0) {
        yinyang_current_music =
            snd_req(good_tune_tbl[yinyang_good_music_index]);
    } else {
        yinyang_current_music =
            snd_req(evil_tune_tbl[yinyang_evil_music_index]);
    }
}

void yinyang_reset_music_index(void) {
    yinyang_good_music_index = 0;
    yinyang_evil_music_index = 0;
}

void set_evil_swap_status(int status) {
    yinyang_ok_to_switch = status;
}

int ok_to_do_evil_swap(void) {
    return yinyang_ok_to_switch;
}

int yy_is_evil_time_active(void) {
    return yy_evil_time_active;
}

void set_evil_condition(int active) {
    yy_evil_time_active = active;
}

MkObj* cut_player_in_half(MkObj* player_object) {
    PlyrInfo* player = &g_game_info.plyr1;
    MkObj* severed;

    if (player_object == g_game_info.plyr0.slot.mirror_a) {
        player = &g_game_info.plyr0;
    }

    severed = obj_sever_limb(
        player_object, 0xE,
        player->slot.fighter->runtime_data->half_sever_velocities, 1);
    if (severed == 0) {
        return 0;
    }

    limb_sever_show_z_meat_chunks(player_object, 0xE, 1);
    severed->flags_08 |= 0x40;
    severed->flags_08 |= 0x20;
    severed->flags_08 |= 8;
    severed->flags_08 |= 4;
    player->slot.fighter->severed_half_obj = severed;
    player->slot.fighter->severed_half_instance = severed->hdr.instance;
    return severed;
}

int get_offset_of_closest_fence_section(
    const Vec* point, const FenceSection* sections, int offset,
    int apply_offset) {
    Vec direction;
    Vec fence_position;
    float distance;
    float closest_distance;
    int closest;

    closest_distance = 200.0f;
    closest = -1;

    while (sections[offset].marker != 0) {
        fence_position.x = sections[offset].x;
        fence_position.y = g_game_info.field_34;
        fence_position.z = sections[offset].z;

        if (apply_offset != 0) {
            if (sections[offset].offset_x != 0.0f) {
                fence_position.x += 1.2f;
            }
            if (sections[offset].offset_z != 0.0f) {
                fence_position.z += 1.2f;
            }
        }

        distance =
            uv_v3_to_v3_dist(&direction, &fence_position, point);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest = offset;
        }
        offset++;
    }

    return closest;
}

/*
 * Soft ceiling: 91.37%, exact retail size and operations. Remaining
 * differences are latch-candidate/final-object GPR coloring and equivalent
 * validation branch direction; retain the typed bitfield and clean C.
 */
void debug_create_axis_indicator(PlyrInfo* player, const Vec* position) {
    MkObj* object;

    if (player->controller_slot == 0) {
        MkObj* latched_object = debug_p1_axis_item.object;

        if (latched_object != 0) {
            if (latched_object->hdr.instance == debug_p1_axis_item.instance) {
                /* Keep the validated object. */
            } else {
                latched_object = 0;
            }
        } else {
            latched_object = 0;
        }
        object = latched_object;
    } else {
        MkObj* latched_object = debug_p2_axis_item.object;

        if (latched_object != 0) {
            if (latched_object->hdr.instance == debug_p2_axis_item.instance) {
                /* Keep the validated object. */
            } else {
                latched_object = 0;
            }
        } else {
            latched_object = 0;
        }
        object = latched_object;
    }

    if (object == 0) {
        object = load_model_from_slot(0, 0x1000C, 0x5001);
        if (object != 0) {
            if (player->controller_slot == 0) {
                debug_p1_axis_item.object = object;
                debug_p1_axis_item.instance = object->hdr.instance;
            } else {
                debug_p2_axis_item.object = object;
                debug_p2_axis_item.instance = object->hdr.instance;
            }
            insert_fgnd_mkobj(object);
        }
    }

    if (object != 0) {
        object->pos.x = position->x;
        object->pos.z = position->z;
        object->pos.y = position->y;
        object->flags_08_bits.bit7 = 1;
    }
}

static float p_fish_attack_bloodsplat(void) {
    Vec effect_origin = {0.0f, 1.6f, 0.0f};
    MabGenericPositionPdata* pdata;
    MkObj* object;
    unsigned int object_instance;
    Vec direction;
    float scale;

    pdata = (MabGenericPositionPdata*)apdata;
    object = 0;
    object_instance = 0;
    object = load_named_model_from_slot(
        0x2001E, "BODYSPLAT", 0x2094, 0);
    if (object != 0) {
        insert_fgnd_mkobj(object);
        object->pos.x = pdata->position.x;
        object->pos.y = pdata->position.y;
        object->pos.z = pdata->position.z;
        object->flags_08_bits.scale_active = 1;
        object->scale.x = 0.5f;
        object->scale.z = 0.5f;
        uv_v3_to_v3(&direction, &effect_origin, &object->pos);
        v3_to_xy_ang(&object->ang, &direction);
        object_instance = object->hdr.instance;
    }

    scale = 0.5f;
    while (scale < 0.75f) {
        MkObj* live_object;

        live_object = object;
        if (live_object != 0) {
            if (live_object->hdr.instance == object_instance) {
                /* Keep the validated object. */
            } else {
                live_object = 0;
            }
        } else {
            live_object = 0;
        }
        if (live_object != 0) {
            live_object->scale.x = scale;
            live_object->scale.z = scale;
            scale += 0.05f;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    return -1.0f;
}

float p_fish_attack(void) {
    const Vec arena_center = {0.0f, 0.0f, 0.0f};
    FishAttackPdata* pdata;
    FishAttackData* fish_data;
    PlyrInfo* player;
    MkObj* target;
    MkObj* fish;
    Vec target_position;
    Vec direction;
    float distance;

    pdata = (FishAttackPdata*)apdata;
    fish_data = &fish_data_tbl[pdata->fish_index];

    if (pdata->active == 1) {
        if (pdata->use_good_fish != 0) {
            fish = load_named_model_from_slot(
                0x2001E, "YY_BAD_FISH", 0x2097, 0);
            pdata->models[pdata->fish_index].good_fish = fish;
        } else {
            fish = load_named_model_from_slot(
                0x2001E, "YY_GOOD_FISH", 0x2096, 0);
            pdata->models[pdata->fish_index].bad_fish = fish;
        }
        if (fish == 0) {
            return -1.0f;
        }

        pdata->fish = fish;
        pdata->fish_instance = fish->hdr.instance;
        obj_change_to_skinned_obj_light_list(
            fish, (LightDef*)&skinned_obj_light_def);
        if (build_bones_tbl(fish, fish_bones) == 0) {
            if (fish->hdr.instance != 0) {
                fish->hdr.typed_vtbl->destroy((MkHdr*)fish);
            }
            return -1.0f;
        }
        insert_fgnd_mkobj(fish);

        RESOLVE_MAB_OBJECT(
            target, pdata->target, pdata->target_instance);
        plyr_obj = target;
        if (target == 0) {
            return -1.0f;
        }

        get_bone_world_pos(target, fish_data->target_bone, &target_position);
        fish->pos.x = target_position.x + 0.5f * sfrand(0.75f);
        fish->pos.y = -0.15f;
        fish->pos.z = target_position.z + 0.5f * sfrand(0.75f);
        pdata->active = 0;
        pdata->lifetime = fish_data->lifetime;
        pdata->state = 0;
        pdata->state_ticks = 15;
        pdata->turn_step = 0;
        pdata->turn_limit = (int)randu0(10) + 5;
        pdata->turning_left = 1;

        RESOLVE_MAB_OBJECT(
            target, pdata->target, pdata->target_instance);
        plyr_obj = target;
        RESOLVE_MAB_OBJECT(
            fish, pdata->fish, pdata->fish_instance);
        if (target == 0 || fish == 0) {
            return -1.0f;
        }
        get_bone_world_pos(
            target, fish_data->target_bone, &target_position);
        distance = uv_v3_to_v3_dist(
            &direction, &fish->pos, &target_position);
        scale_v3(
            &direction, &direction, distance / 15.0f);
        fish->pos_vel = direction;
        fish->flags_08_bits.gravity_enabled = 1;
        fish->flags_08_bits.angular_velocity_enabled = 1;
    }

    while (pdata->lifetime > 0) {
        FighterObjectRef* severed;
        MkObj* severed_object;

        RESOLVE_MAB_OBJECT(
            target, pdata->target, pdata->target_instance);
        plyr_obj = target;
        RESOLVE_MAB_OBJECT(
            fish, pdata->fish, pdata->fish_instance);
        if (target == 0 || fish == 0) {
            return -1.0f;
        }

        pdata->lifetime--;
        player = target == g_game_info.player_objects[0]
            ? &g_game_info.plyr0 : &g_game_info.plyr1;

        if (pdata->state_ticks <= 0) {
            if (pdata->lifetime < 15 && pdata->state < 4) {
                pdata->state = 4;
                get_bone_world_pos(
                    target, fish_data->target_bone, &target_position);
                distance = uv_v3_to_v3_dist(
                    &direction, &fish->pos, &target_position);
                scale_v3(
                    &direction, &direction, distance / 10.0f);
                fish->pos_vel = direction;
                pdata->state_ticks = 10;
                pdata->lifetime = 12;
                return 1.0f;
            }

            switch (pdata->state) {
            case 0:
                scale_v3(&fish->pos_vel, &fish->pos_vel, -1.0f);
                pdata->state_ticks = 10;
                pdata->state = 2;
                continue;
            case 2:
                get_bone_world_pos(
                    target, fish_data->target_bone, &target_position);
                distance = uv_v3_to_v3_dist(
                    &direction, &fish->pos, &target_position);
                pdata->state_ticks = 10;
                scale_v3(&direction, &direction,
                         distance / (float)pdata->state_ticks);
                fish->pos_vel = direction;
                pdata->state = 0;
                continue;
            case 4:
                RESOLVE_MAB_OBJECT(
                    target, pdata->target, pdata->target_instance);
                plyr_obj = target;
                RESOLVE_MAB_OBJECT(
                    fish, pdata->fish, pdata->fish_instance);
                if (target == 0 || fish == 0) {
                    return -1.0f;
                }
                pdata->state_ticks = 8;
                pdata->lifetime = pdata->state_ticks + 2;
                fish->ang.x = 0.0f;
                direction.x = fish->frame->modelling.at.x;
                direction.y = fish->frame->modelling.at.y;
                direction.z = fish->frame->modelling.at.z;
                normalize_v3(&direction);
                scale_v3(
                    &direction, &direction,
                    -(1.0f / (float)pdata->state_ticks));
                fish->pos_vel = direction;
                fish->pos_vel.y = 0.0f;
                pdata->state = 5;
                continue;
            case 5:
                distance = uv_v3_to_v3_dist(
                    &direction, &arena_center, &target->pos);
                pdata->state_ticks = 110;
                pdata->lifetime = pdata->state_ticks + 2;
                scale_v3(&direction, &direction,
                         distance / (float)pdata->state_ticks);
                fish->pos_vel = direction;
                pdata->state = 6;
                continue;
            case 6:
                hide_obj(fish);
                continue;
            }
        } else {
            RESOLVE_MAB_OBJECT(
                target, pdata->target, pdata->target_instance);
            plyr_obj = target;
            RESOLVE_MAB_OBJECT(
                fish, pdata->fish, pdata->fish_instance);
            if (target == 0 || fish == 0) {
                return -1.0f;
            }
            switch (pdata->state) {
            case 0:
            case 2:
            case 4:
                get_bone_world_pos(
                    target, fish_data->target_bone, &target_position);
                uv_v3_to_v3(&direction, &fish->pos, &target_position);
                v3_to_xy_ang(&fish->ang, &direction);
                pdata->state_ticks--;
                break;
            case 5:
                get_bone_world_pos(
                    target, fish_data->target_bone, &target_position);
                fish->ang.x = 0.0f;
                if (fish->pos.y > -0.05f) {
                    fish->pos.y -= 0.05f;
                }
                direction.x = -fish->pos_vel.x;
                direction.y = -fish->pos_vel.y;
                direction.z = -fish->pos_vel.z;
                v3_to_xy_ang(&fish->ang, &direction);
                pdata->state_ticks--;
                break;
            case 6:
                if (pdata->turning_left != 0) {
                    rotate_xz(&fish->pos_vel, &fish->pos_vel, 0.07f);
                    pdata->turn_step++;
                    if (pdata->turn_step > pdata->turn_limit) {
                        pdata->turning_left = 0;
                    }
                } else {
                    rotate_xz(&fish->pos_vel, &fish->pos_vel, -0.07f);
                    pdata->turn_step--;
                    if (pdata->turn_step < -pdata->turn_limit) {
                        pdata->turning_left = 1;
                    }
                }
                v3_to_xy_ang(&fish->ang, &fish->pos_vel);
                severed = &player->slot.fighter->
                    severed_limbs[pdata->fish_index];
                RESOLVE_MAB_OBJECT(
                    severed_object, severed->object, severed->instance);
                if (severed_object != 0 && pdata->state_ticks < 20) {
                    fish->pos.y -= 0.1f;
                }
                pdata->state_ticks--;
                break;
            }
            return 1.0f;
        }
    }

    RESOLVE_MAB_OBJECT(
        fish, pdata->fish, pdata->fish_instance);
    if (fish != 0 && fish->hdr.instance != 0) {
        fish->hdr.typed_vtbl->destroy((MkHdr*)fish);
    }
    return -1.0f;
}

float p_fish_attack_scream_sounds(void) {
    PlyrInfo* player = &g_game_info.plyr1;
    FishScreamPdata* pdata = (FishScreamPdata*)apdata;

    if (pdata->player_index == 0) {
        player = &g_game_info.plyr0;
    }

    plyr_snd_req_no_plyr_proc(player->slot.fighter, 0x23);
    _mkproc_sleep_ticks = 25.0f;
    aproc->vtbl->sleep();
    plyr_snd_req_no_plyr_proc(player->slot.fighter, 0x22);
    _mkproc_sleep_ticks = 45.0f;
    aproc->vtbl->sleep();
    plyr_snd_req_no_plyr_proc(player->slot.fighter, 0x47);
    return -1.0f;
}

float p_fish_attack_sounds(void) {
    FishScreamPdata* scream_pdata;
    int elapsed_ticks;

    scream_pdata = (FishScreamPdata*)apdata;
    if (_create_mkproc_generic_tinystack(
            0x209A, 0x1F, p_fish_attack_scream_sounds, 0x28,
            (MkHdr**)&scream_pdata) != 0) {
        scream_pdata->player_index =
            plyr_obj == g_game_info.plyr0.slot.mirror_a ? 0 : 1;
    }

    snd_req(0x14E);
    _mkproc_sleep_ticks = 15.0f;
    aproc->vtbl->sleep();

    elapsed_ticks = 0;
    while (elapsed_ticks < 180) {
        int delay;

        delay = (int)randu0(10) + 15;
        elapsed_ticks += delay;
        if (randu0(2) == 0) {
            snd_req(0x14D);
        } else {
            snd_req(0x14E);
        }
        _mkproc_sleep_ticks = (float)delay;
        aproc->vtbl->sleep();
    }
    return -1.0f;
}

void start_fish_attack(MkObj* target, int attack_kind, int target_kind) {
    FishAttackPdata* fish_pdata;
    FishScreamPdata* scream_pdata;
    MabGenericPositionPdata* blood_pdata;
    void* blood_proc;
    int fish_index;

    for (fish_index = 0; fish_index < 13; fish_index++) {
        if (_create_mkproc_generic_nostack(
                0x209A, 0x1F, p_fish_attack, 0x40,
                (MkHdr**)&fish_pdata) != 0) {
            fish_pdata->use_good_fish = target_kind;
            fish_pdata->models = (FishModelPair*)attack_kind;
            fish_pdata->target = target;
            fish_pdata->target_instance = target->hdr.instance;
            fish_pdata->fish_index = fish_index;
            fish_pdata->active = 1;
        }
    }

    blood_proc = proc_create(p_fish_attack_bloodsplat, 0x209C);
    snd_req(0x154);
    if (blood_proc != 0) {
        blood_pdata = (MabGenericPositionPdata*)mab_generic_pdata;
        blood_pdata->position.x = target->pos.x;
        blood_pdata->position.y = -0.185f;
        blood_pdata->position.z = target->pos.z;
    }

    if (_create_mkproc_generic_tinystack(
            0x209A, 0x1F, p_fish_attack_sounds, 0x28,
            (MkHdr**)&scream_pdata) != 0) {
        scream_pdata->player_index =
            target == g_game_info.plyr0.slot.mirror_a ? 0 : 1;
    }
}

static float p_player_body_explode(void) {
    PlayerBodyExplodePdata* pdata;
    PlyrInfo* player;
    FighterMirror* fighter;
    MkObj* player_object;
    MkProc* anim_proc;
    int limb;

    pdata = (PlayerBodyExplodePdata*)apdata;
    player = pdata->player;
    fighter = player->slot.fighter;
    player_object = player->slot.mirror_a;

    player_object->flags_09_bits.head_tracking = 0;
    bgnd_hide_mirror_guys();

    for (limb = 0; limb < 15; limb++) {
        FighterObjectRef* severed;
        MkObj* object;

        severed = &fighter->severed_limbs[limb];
        object = severed->object;
        if (object != 0 && object->hdr.instance != severed->instance) {
            object = 0;
        }
        if (object == 0) {
            object = obj_sever_limb(
                player_object, limb,
                fighter->runtime_data->half_sever_velocities, 1);
            if (object != 0) {
                severed->object = object;
                severed->instance = object->hdr.instance;
            }
        }
    }

    limb_sever_show_z_meat_chunks_all(player_object);
    anim_proc = fighter->anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != fighter->anim_proc_instance) {
        anim_proc = 0;
    }
    xfer_proc(anim_proc, p_anim_idle);

    for (limb = 0; limb < 15; limb++) {
        FighterObjectRef* severed;
        MkObj* object;

        severed = &fighter->severed_limbs[limb];
        object = severed->object;
        if (object != 0 && object->hdr.instance != severed->instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08_bits.gravity_enabled = 1;
            object->flags_08_bits.angular_velocity_enabled = 1;
            object->flags_08_bits.rotation_enabled = 1;
            object->flags_08_bits.moving = 1;
            object->gravity = -0.003f;
            object->pos_vel.x =
                pdata->direction.x * pdata->scale + sfrand(0.05f);
            object->pos_vel.y = 0.0f;
            object->pos_vel.x =
                pdata->direction.z * pdata->scale + sfrand(0.05f);
            object->ang_vel.x = sfrand(0.15f);
            object->ang_vel.y = sfrand(0.15f);
            object->ang_vel.z = sfrand(0.15f);
        }
    }
    return -1.0f;
}

void player_body_explode(
    PlyrInfo* player, const Vec* direction, float scale) {
    PlayerBodyExplodePdata* pdata;

    if (_create_mkproc_generic_nostack(
            0x2097, 0x1F, (MkProcEntryFn)p_player_body_explode,
            sizeof(PlayerBodyExplodePdata), (MkHdr**)&pdata) != 0) {
        pdata->player = player;
        pdata->direction.x = direction->x;
        pdata->direction.y = direction->y;
        pdata->direction.z = direction->z;
        pdata->scale = scale;
    }
}

void reset_collision_system(void) {
    init_collision_system();
}

/* Soft ceiling: 93.50% -- register allocation and one latch branch direction. */
void init_plyr_severed_limb_list(PlyrInfo* player) {
    FighterMirror* fighter = player->slot.fighter;
    int limb;

    for (limb = 0; limb < 15; limb++) {
        FighterObjectRef* severed = &fighter->severed_limbs[limb];
        MkObj* object = severed->object;

        if (object != 0) {
            if (object->hdr.instance != severed->instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object == 0) {
            object = obj_sever_limb(
                player->slot.mirror_a, limb,
                fighter->runtime_data->half_sever_velocities, 1);
            if (object != 0) {
                player->slot.fighter->severed_limbs[limb].object = object;
                player->slot.fighter->severed_limbs[limb].instance =
                    object->hdr.instance;
            }
        }
    }
}

void yinyang_set_bad_fish_hide_flag(
    YinyangFishPair* fish, unsigned char hide, int count) {
    int index;

    for (index = 0; index < count; index++) {
        MkObj* object = fish[index].bad_fish;

        if (object != 0) {
            object->hide_flag_bits.hidden = hide;
        }
        if (hide == 0) {
            fish[index].active_fish->object = fish[index].bad_fish;
            fish[index].active_fish->instance =
                fish[index].bad_fish->hdr.instance;
        }
    }
}

void yinyang_set_good_fish_hide_flag(
    YinyangFishPair* fish, unsigned char hide, int count) {
    int index;

    for (index = 0; index < count; index++) {
        MkObj* object = fish[index].good_fish;

        if (object != 0) {
            object->hide_flag_bits.hidden = hide;
        }
        if (hide == 0) {
            fish[index].active_fish->object = fish[index].good_fish;
            fish[index].active_fish->instance =
                fish[index].good_fish->hdr.instance;
        }
    }
}

void obj_setup_for_animation(
    MkObj* object, const int* tags, MkFlippedBoneMap* flipped_bone_map,
    void* ground_colls) {
    if (tags != 0) {
        build_bones_tbl(object, tags);
    }
    object->flipped_bone_map = flipped_bone_map;
    object->ground_colls = ground_colls;
}

/*
 * Soft ceiling: retail retains an otherwise dead `2.0f` stack temporary after
 * copying the fish transforms. The effective algorithm and all observable
 * accesses match; portable C intentionally does not recreate the dead store.
 */
void yinyang_make_fish_jump(YinyangFishPair* fish, int count) {
    Vec cylinder_position = {0.0f, 0.0f, 0.0f};
    Vec cylinder_axis = {0.0f, 0.0f, 1.0f};
    Vec camera_direction;
    Vec jump_direction;
    Vec camera_position;
    Vec jump_position;
    Vec random_angles = {0.0f, 0.0f, 0.0f};
    float intersection_a;
    float intersection_b;
    float intersection;
    YinyangFishPair* current;
    int index;

    camera_direction.x =
        ((RwFrame*)Camera->object.object.parent)->modelling.at.x;
    camera_direction.y = 0.0f;
    camera_direction.z =
        ((RwFrame*)Camera->object.object.parent)->modelling.at.z;
    ray_cyl_intersection(
        &camera_obj->pos, &camera_direction,
        &cylinder_position, &cylinder_axis,
        29.0f + sfrand(2.0f), &intersection_a, &intersection_b);
    intersection = intersection_a;
    if (intersection <= 0.0f) {
        intersection = intersection_b;
    }

    camera_position.x = camera_obj->pos.x;
    camera_position.y = 0.0f;
    camera_position.z = camera_obj->pos.z;
    scale_xz(&jump_position, &camera_direction, intersection);
    v3_add_v3(&jump_position, &jump_position, &camera_position);
    uv_v3_to_v3(
        &jump_direction, &jump_position, &cylinder_position);

    index = 0;
    while (index < count) {
        unsigned char bad_hidden;
        float angle;
        int wrapped_angle;

        /* Retail only advances the pair while the camera latch is live. */
        if (camera_obj == 0) {
            continue;
        }

        random_angles.x += sfrand(0.06f);
        random_angles.y += sfrand(0.02f);
        random_angles.z += sfrand(0.06f);
        current = &fish[index];

        current->good_fish->pos.x = jump_position.x;
        current->good_fish->pos.y = -1.25f;
        current->good_fish->pos.z = jump_position.z;
        angle = 0.63f + (1.57f + xz_to_y_ang(&jump_direction));
        index++;
        wrapped_angle = (int)(166886.1f * angle) & 0xFFFFF;
        current->good_fish->ang.y =
            0.000005992112f * (float)wrapped_angle;
        current->good_fish->flags_08_bits.angular_velocity_enabled = 1;
        current->good_fish->flags_08_bits.airborne = 1;

        current->active_fish->field_38 = 0.0f;
        current->active_fish->flags |= 3;
        current->bad_fish->pos = current->good_fish->pos;
        current->bad_fish->ang = current->good_fish->ang;
        bad_hidden = current->bad_fish->hide_flags & 0x20;
        current->bad_fish->flags_word_08 =
            current->good_fish->flags_word_08;
        current->bad_fish->hide_flags = (unsigned char)(
            (current->bad_fish->hide_flags & ~0x20) | bad_hidden);
        current->active_fish->field_38 = 0.0f;
    }
}

/*
 * Soft ceiling: standard fabs emits six calls in this MWCC configuration;
 * keep the portable library operation rather than a compiler intrinsic.
 */
static float p_monitor_objs_sobjs(void) {
    Vec ground_normal = {0.0f, 1.0f, 0.0f};
    ObjectMonitorPdata* pdata;
    MkObj* target;
    MkPtr* iterator;
    int all_settled;

    pdata = (ObjectMonitorPdata*)apdata;
    all_settled = 1;
    target = (MkObj*)pdata->target;
    if (target != 0) {
        if (target->hdr.instance == pdata->target_instance) {
            /* Keep the validated target. */
        } else {
            target = 0;
        }
    } else {
        target = 0;
    }

    if (target != 0) {
        iterator = first_mkptr(&target->sobj_list);
        while (iterator != 0) {
            MkSobj* object;

            object = (MkSobj*)iterator->hdr;
            if ((object->id_flags & 0xFFF) != 0) {
                if (object->pos.y <
                    g_game_info.field_34 + pdata->settle_height) {
                    float reflection;
                    float reflected_x;
                    float reflected_y;
                    float reflected_z;

                    pdata->callback(object);
                    object->pos.y = g_game_info.field_34 +
                        pdata->settle_height + pdata->vertical_step;
                    reflection = 2.0f *
                        (object->pos_vel.x * ground_normal.x +
                         object->pos_vel.y * ground_normal.y +
                         object->pos_vel.z * ground_normal.z);
                    reflected_x = ground_normal.x * reflection;
                    reflected_y = ground_normal.y * reflection;
                    reflected_z = ground_normal.z * reflection;
                    object->pos_vel.x -= reflected_x;
                    object->pos_vel.y -= reflected_y;
                    object->pos_vel.z -= reflected_z;
                    scale_v3(
                        &object->pos_vel, &object->pos_vel,
                        pdata->velocity_scale);

                    if (fabs(object->pos_vel.x) < pdata->thresholds.min_pos.x.value) {
                        object->pos_vel.x = 0.0f;
                    }
                    if (fabs(object->pos_vel.y) < pdata->thresholds.min_pos.y.value) {
                        object->pos_vel.y = 0.0f;
                    }
                    if (fabs(object->pos_vel.z) < pdata->thresholds.min_pos.z.value) {
                        object->pos_vel.z = 0.0f;
                    }
                    if (fabs(object->ang_vel.x) < pdata->thresholds.min_vel.x.value) {
                        object->ang_vel.x = 0.0f;
                    }
                    if (fabs(object->ang_vel.y) < pdata->thresholds.min_vel.y.value) {
                        object->ang_vel.y = 0.0f;
                    }
                    if (fabs(object->ang_vel.z) < pdata->thresholds.min_vel.z.value) {
                        object->ang_vel.z = 0.0f;
                    }
                }

                if (length_v3(&object->pos_vel) != 0.0f ||
                    object->pos.y >
                        g_game_info.field_34 + pdata->settle_height +
                            2.0f * pdata->vertical_step) {
                    all_settled = 0;
                    object->pos_vel.y -= pdata->vertical_step;
                } else if (length_v3(&object->ang_vel) != 0.0f) {
                    zero_v3(&object->ang_vel);
                    all_settled = 0;
                }
            }
            iterator = next_mkptr(iterator);
        }
        if (all_settled != 0) {
            return -1.0f;
        }
    }
    return 1.0f;
}

void do_yinyang_statue_explosion(MkHdr* statue) {
    ObjectMonitorSpawnLocals locals;

    if (statue == 0) {
        return;
    }

    locals.config.target = statue;
    locals.config.velocity_scale = 0.45f;
    locals.config.thresholds.min_pos.x.value = 0.01f;
    locals.config.thresholds.min_pos.y.value = 0.03f;
    locals.config.thresholds.min_pos.z.value = 0.01f;
    locals.config.thresholds.min_vel.x.value = 0.01f;
    locals.config.thresholds.min_vel.y.value = 0.01f;
    locals.config.thresholds.min_vel.z.value = 0.01f;
    locals.config.vertical_step = 0.003f;
    locals.config.settle_height = 0.15f;
    locals.config.callback = p_statue_xpd_callback;

    if (((CreateObjectMonitorProcFn)_create_mkproc_generic_nostack)(
            0x2095, 0x1F, p_monitor_objs_sobjs,
            sizeof(ObjectMonitorPdata), &locals.pdata,
            locals.config.vertical_step,
            locals.config.thresholds.min_pos.y.value,
            locals.config.thresholds.min_pos.x.value,
            locals.config.velocity_scale) != 0) {
        locals.pdata->velocity_scale = locals.config.velocity_scale;
        locals.pdata->target = locals.config.target;
        locals.pdata->target_instance = locals.config.target->instance;
        locals.pdata->thresholds.min_pos = locals.config.thresholds.min_pos;
        locals.pdata->thresholds.min_vel = locals.config.thresholds.min_vel;
        locals.pdata->vertical_step = locals.config.vertical_step;
        locals.pdata->settle_height = locals.config.settle_height;
        locals.pdata->callback = locals.config.callback;
    }
}

void p_statue_xpd_callback(MkSobj* object) {
    Vec* position;
    unsigned int statue_index;

    position = sobj_get_world_pos(object);
    hide_sobj(object);
    zero_v3(&object->pos_vel);
    object->flags_08 &= (unsigned char)~0x40;
    object->flags_08 &= (unsigned char)~0x20;
    zero_v3(&object->ang_vel);
    object->flags_08 &= (unsigned char)~0x08;
    object->flags_08 &= (unsigned char)~0x04;

    statue_index = object->id_flags;
    if (statue_index < 7 && statue_index != 0) {
        bgnd_launch_fx_at_position(
            rock_xpd_effects[statue_index - 1],
            position->x, position->y, position->z);
        bgnd_launch_fx_at_position(
            rock_dust_effects[statue_index - 1],
            position->x, position->y, position->z);
    }
}

static float p_xpd_obj_monitor(void) {
    Vec ground_normal = {0.0f, 1.0f, 0.0f};
    PlayerBodyExplodePdata* pdata;
    FighterMirror* fighter;
    int frame;
    int sound_delay;

    pdata = (PlayerBodyExplodePdata*)apdata;
    fighter = pdata->player->slot.fighter;
    frame = 0;
    sound_delay = 0;

    do {
        int limb;

        for (limb = 0; limb < 15; limb++) {
            FighterObjectRef* severed;
            MkObj* object;

            severed = &fighter->severed_limbs[limb];
            object = severed->object;
            if (object != 0 && object->hdr.instance != severed->instance) {
                object = 0;
            }
            if (object != 0) {
                float ground_y;

                ground_y = g_game_info.field_34 + 0.35f;
                if (object->pos.y < ground_y) {
                    float reflection;

                    object->pos.y = ground_y;
                    reflection = 2.0f *
                        (object->pos_vel.x * ground_normal.x +
                         object->pos_vel.y * ground_normal.y +
                         object->pos_vel.z * ground_normal.z);
                    object->pos_vel.x -= ground_normal.x * reflection;
                    object->pos_vel.y -= ground_normal.y * reflection;
                    object->pos_vel.z -= ground_normal.z * reflection;
                    scale_v3(
                        &object->pos_vel, &object->pos_vel, 0.45f);

                    if (fabs(object->pos_vel.x) < 0.004f &&
                        fabs(object->pos_vel.y) < 0.004f &&
                        fabs(object->pos_vel.z) < 0.004f) {
                        zero_v3(&object->pos_vel);
                        object->gravity = 0.0f;
                        zero_v3(&object->ang_vel);
                    } else {
                        object->ang_vel.x *= 0.6f;
                        if (object->ang_vel.x < 0.005f) {
                            object->ang_vel.x = 0.0f;
                        }
                        object->ang_vel.y *= 0.6f;
                        if (object->ang_vel.y < 0.005f) {
                            object->ang_vel.y = 0.0f;
                        }
                        object->ang_vel.z *= 0.6f;
                        if (object->ang_vel.z < 0.005f) {
                            object->ang_vel.z = 0.0f;
                        }
                    }
                }
            }
        }

        if (frame > 10 && frame < 100) {
            sound_delay--;
            if (sound_delay < 0) {
                unsigned int sound;

                sound_delay = (int)randu0(30);
                sound = randu0(4);
                if (sound == 0) {
                    snd_req(0x12C);
                } else if (sound == 1) {
                    snd_req(0x12D);
                } else if (sound == 2) {
                    snd_req(0x12E);
                } else {
                    snd_req(0x12F);
                }
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        frame++;
        aproc->vtbl->sleep();
    } while (frame < 120);

    return -1.0f;
}

/*
 * Soft ceiling: retail emits a three-instruction explicit null-normalization
 * tail for the object-instance latch. Clean typed C folds that equivalent
 * join; the remaining differences are allocation, scheduling, and pool
 * relocations, so no redundant branch is added to force the retail shape.
 */
float p_skytemple_bodysplat(void) {
    MkObj* object = 0;
    SkyTempleBodysplatPdata* pdata = (SkyTempleBodysplatPdata*)apdata;
    unsigned int object_instance = 0;
    MkObj* loaded_object;
    float scale;

    loaded_object =
        load_named_model_from_slot(0x2001E, "ST_BLOODSPLAT", 0x2094, 0);
    if (loaded_object != 0) {
        insert_fgnd_mkobj(loaded_object);
        object = loaded_object;
        object->pos.x = pdata->position.x;
        object->pos.y = pdata->position.y;
        object->pos.z = pdata->position.z;
        object->flags_08_bits.scale_active = 1;
        object->scale.x = 1.0f;
        object->scale.y = 1.0f;
        object->scale.z = 1.0f;
        object_instance = object->hdr.instance;
    }

    scale = 1.0f;
    while (scale < 2.5f) {
        MkObj* live_object = 0;

        if (object != 0 && object->hdr.instance == object_instance) {
            live_object = object;
        }
        if (live_object != 0) {
            live_object->scale.x = scale;
            live_object->scale.z = scale;
            scale += 0.2f;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    return -1.0f;
}

static float p_cam_bounce_monitor(void) {
    CameraBouncePdata* pdata;
    MkObj* object;
    Vec saved_velocity;
    Vec saved_angular_velocity;
    Vec* camera_normal;
    float distance;
    float reflection;

    pdata = (CameraBouncePdata*)apdata;
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (camera_obj == 0 || object == 0) {
        return -1.0f;
    }

    distance = dist_v3_to_v3(
        &camera_obj->pos, &object->pos);
    while (fabs(distance) > pdata->trigger_distance) {
        object = pdata->object;
        if (object != 0 && object->hdr.instance != pdata->object_instance) {
            object = 0;
        }
        if (object == 0) {
            return -1.0f;
        }
        distance = dist_v3_to_v3(
            &camera_obj->pos, &object->pos);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    saved_velocity = object->pos_vel;
    saved_angular_velocity = object->ang_vel;
    zero_v3(&object->pos_vel);
    zero_v3(&object->ang_vel);
    snd_req(0x12F);
    _mkproc_sleep_ticks = 4.0f;
    aproc->vtbl->sleep();
    object->pos_vel = saved_velocity;
    object->ang_vel = saved_angular_velocity;

    camera_normal =
        (Vec*)&((RwFrame*)Camera->object.object.parent)->modelling.pos;
    normalize_v3(camera_normal);
    reflection = 2.0f *
        (object->pos_vel.x * camera_normal->x +
         object->pos_vel.y * camera_normal->y +
         object->pos_vel.z * camera_normal->z);
    object->pos_vel.x -= camera_normal->x * reflection;
    object->pos_vel.y -= camera_normal->y * reflection;
    object->pos_vel.z -= camera_normal->z * reflection;
    v3_add_v3(&object->pos, &object->pos, &object->pos_vel);
    scale_v3(
        &object->pos_vel, &object->pos_vel, pdata->velocity_scale);
    return -1.0f;
}

/*
 * Soft ceiling: 91.40% -- validation-branch polarity, temporary allocation,
 * and packed-byte update forms only; the recovered sequence is complete.
 */
void skytemple_player_explode(
    unsigned int player_index, float x, float y, float z) {
    PlyrInfo* player = &g_game_info.plyr1;
    MkProc* anim_proc;
    MkProc* bounce_proc;
    MkObj* limb;
    float saved_facial_damage;
    int limb_index;

    if (player_index == 0) {
        player = &g_game_info.plyr0;
    }

    if (player->slot.mirror_a == 0) {
        return;
    }

    player->slot.mirror_a->flags_09_bits.head_tracking = 0;
    player->slot.mirror_a->pos.x = x;
    player->slot.mirror_a->pos.y = y;
    player->slot.mirror_a->pos.z = z;
    bgnd_launch_fx_at_position("meat_chunk_explode", x, y, z);
    bgnd_hide_mirror_guys();

    for (limb_index = 0; limb_index < 15; limb_index++) {
        RESOLVE_MAB_OBJECT(
            limb,
            player->slot.fighter->severed_limbs[limb_index].object,
            player->slot.fighter->severed_limbs[limb_index].instance);
        if (limb == 0) {
            limb = obj_sever_limb(
                player->slot.mirror_a, limb_index,
                player->slot.fighter->runtime_data->half_sever_velocities,
                1);
            if (limb != 0) {
                player->slot.fighter->severed_limbs[limb_index].object =
                    limb;
                player->slot.fighter->severed_limbs[limb_index].instance =
                    limb->hdr.instance;
            }
        }
    }

    limb_sever_show_z_meat_chunks_all(player->slot.mirror_a);
    anim_proc = player->slot.fighter->anim_proc;
    if (anim_proc != 0) {
        if (anim_proc->hdr.instance ==
            player->slot.fighter->anim_proc_instance) {
            /* Keep the validated animation process. */
        } else {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    xfer_proc(anim_proc, p_anim_idle);

    saved_facial_damage = player->slot.fighter->facial_damage;
    add_facial_damage(player->slot.fighter, 1.0f);

    for (limb_index = 0; limb_index < 15; limb_index++) {
        RESOLVE_MAB_OBJECT(
            limb,
            player->slot.fighter->severed_limbs[limb_index].object,
            player->slot.fighter->severed_limbs[limb_index].instance);
        if (limb == 0) {
            continue;
        }

        limb->pos.x = x;
        limb->pos.y = y + 0.35f;
        limb->pos.z = z;
        limb->flags_08_bits.gravity_enabled = 1;
        limb->flags_08_bits.angular_velocity_enabled = 1;
        limb->flags_08_bits.rotation_enabled = 1;
        limb->flags_08_bits.moving = 1;
        limb->gravity = -0.003f;

        if (limb_index == 0) {
            CameraBouncePdata* bounce_pdata;
            CameraObj* camera;
            MkObj* bounce_object;
            Vec gusher_position;
            Vec gusher_direction;
            float camera_delta_y;
            float camera_delta_z;
            float saved_gravity;

            mkobj_zero_bone_rots(limb);
            limb->ang_vel.x = 0.09f;
            limb->ang_vel.y = 0.08f;
            limb->ang.x = 3.1415927f;
            limb->ang.y = 3.1415927f;
            limb->ang.z = 0.0f;
            saved_gravity = limb->gravity;

            RESOLVE_MAB_OBJECT(
                bounce_object, limb, limb->hdr.instance);

            camera = camera_obj;
            if (camera != 0 && bounce_object != 0) {
                camera_delta_y = camera->pos.y - bounce_object->pos.y;
                camera_delta_z = camera->pos.z - bounce_object->pos.z;
                bounce_object->pos_vel.x =
                    (camera->pos.x - bounce_object->pos.x) * 0.025f;
                bounce_object->pos_vel.y = camera_delta_y * 0.025f;
                bounce_object->pos_vel.z = camera_delta_z * 0.025f;
                if (_create_mkproc_generic_tinystack(
                        0x2092, 0x1F, p_cam_bounce_monitor,
                        sizeof(CameraBouncePdata),
                        (MkHdr**)&bounce_pdata) != 0) {
                    bounce_pdata->object = bounce_object;
                    bounce_pdata->object_instance =
                        bounce_object->hdr.instance;
                    bounce_pdata->trigger_distance = 0.59f;
                    bounce_pdata->saved_gravity = saved_gravity;
                    bounce_pdata->direction_scale = 0.025f;
                    bounce_pdata->velocity_scale = 0.95f;
                    bounce_pdata->saved_gravity = bounce_object->gravity;
                    bounce_object->gravity = 0.0f;
                }
            }

            gusher_position.x = 0.0f;
            gusher_position.y = 0.0f;
            gusher_position.z = 0.0f;
            gusher_direction.x = 0.8f;
            gusher_direction.y = -1.0f;
            gusher_direction.z = 0.0f;
            {
                MkHdr* gusher = (MkHdr*)start_gusher(
                    heart_beat, player->slot.fighter, limb, 0x10,
                    &gusher_position, &gusher_direction);
                if (gusher != 0) {
                    mk_insert(gusher, &gusher_list);
                }
            }
        } else {
            float velocity;

            limb->pos_vel.y = 0.03f + frand(0.03f);
            velocity = sfrand(0.02f);
            limb->pos_vel.x = velocity > 0.0f
                ? velocity + 0.03f : velocity - 0.03f;
            velocity = sfrand(0.02f);
            limb->pos_vel.z = velocity > 0.0f
                ? velocity + 0.03f : velocity - 0.03f;
            limb->ang_vel.x = sfrand(0.1f);
            limb->ang_vel.y = sfrand(0.1f);
            limb->ang_vel.z = sfrand(0.1f);
        }
    }

    {
        SkyTempleExplodeMonitorPdata* monitor_pdata;

        if (_create_mkproc_generic_tinystack(
                0x208E, 0x1F, p_xpd_obj_monitor,
                sizeof(SkyTempleExplodeMonitorPdata),
                (MkHdr**)&monitor_pdata) == 0) {
            return;
        }
        monitor_pdata->player = player;
    }

    {
        ScreenObj* effect = load_named_2d_pfxobj(
            0x2001E, 0x2094, "ST_BLOODSPLAT", 0, 0x29);
        if (effect != 0) {
            effect->x = 0;
            effect->y = 0x48;
            effect->flags |= 8;
            effect->scale_x = 2.0f;
            effect->scale_y = 2.0f;
        }
    }

    if (proc_create(p_skytemple_bodysplat, 0x208F) != 0) {
        ((MabGenericPositionPdata*)mab_generic_pdata)->position.x = x;
        ((MabGenericPositionPdata*)mab_generic_pdata)->position.y = y;
        ((MabGenericPositionPdata*)mab_generic_pdata)->position.z = z;
    }

    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    bgnd_launch_fx_at_position("meat_chunk_explode2", x, y, z);
    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    bgnd_launch_fx_at_position("meat_chunk_explode3", x, y, z);

    bounce_proc = find_mkproc_pid(0x2092);
    while (bounce_proc != 0) {
        bounce_proc = find_mkproc_pid(0x2092);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    RESOLVE_MAB_OBJECT(
        limb, player->slot.fighter->severed_limbs[0].object,
        player->slot.fighter->severed_limbs[0].instance);
    if (limb != 0) {
        ScreenObj* effect;

        limb->gravity = -0.003f;
        limb->flags_08_bits.moving = 1;
        shake_camera(3, (MkHdr*)1, 0.01f);
        effect = load_named_2d_pfxobj(
            0x2001E, 0x2094, "ST_BLOODSPLAT", 0, 0x29);
        if (effect != 0) {
            effect->x = 0xA0;
            effect->y = 0x5A;
            effect->flags |= 8;
            effect->scale_x = 2.0f;
            effect->scale_y = 2.0f;
            effect->flags |= 0x20;
        }
    }

    _mkproc_sleep_ticks = 120.0f;
    aproc->vtbl->sleep();
    player->slot.fighter->facial_damage = saved_facial_damage;
}

void skytemple_make_scream_sound(void) {
    plyr_snd_req(0x46);
}

void misc_data_set_obj_ptr1(void* object) {
    misc_bgnd_data.object = object;
}

void* misc_data_get_obj_ptr1(void) {
    return misc_bgnd_data.object;
}

void mab_script_trace_func(void) {
    debug_print_message();
}

/* Soft ceiling: misc_data_set_test_float ~92.72% -- pooled string address only. */
void misc_data_set_test_float(float value) {
    char message[112];

    misc_bgnd_data.test_float = value;
    sprintf(message, "Test float value is: %f", value);
    debug_print_message(message);
}

/* Soft ceiling: misc_data_set_test_u32 ~90.74% -- pooled string address only. */
void misc_data_set_test_u32(unsigned int value) {
    char message[112];

    misc_bgnd_data.test_value = value;
    sprintf(message, "Test u32 value is: %d", value);
    debug_print_message(message);
}

void misc_data_set_col_obj_id2(int object_id) {
    misc_bgnd_data.collision_object_id2 = object_id;
}

void misc_data_set_col_obj_id(int object_id) {
    misc_bgnd_data.collision_object_id = object_id;
}

int misc_data_get_col_obj_id2(void) {
    return misc_bgnd_data.collision_object_id2;
}

int misc_data_get_col_obj_id(void) {
    return misc_bgnd_data.collision_object_id;
}

void init_misc_bgnd_data(void) {
    misc_bgnd_data.collision_object_id = 0;
    misc_bgnd_data.collision_object_id2 = 0;
    misc_bgnd_data.object = 0;
}
