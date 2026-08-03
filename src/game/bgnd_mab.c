#include "game/collision.h"
#include "game/game_info.h"
#include "platform/io.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"

typedef unsigned int MslSoundHandle;

typedef struct MiscBgndData {
    int collision_object_id;
    int collision_object_id2;
    void* object;
    float test_float;
    unsigned int test_value;
} MiscBgndData;

typedef struct PlayerBodyExplodePdata {
    MkHdr hdr;
    int model_index;
    Vec position;
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

typedef struct ObjectMonitorPdata {
    MkHdr hdr;
    int state;
    float min_pos_x;
    float min_pos_y;
    float min_pos_z;
    float min_vel_x;
    float min_vel_y;
    float min_vel_z;
    float gravity;
    float settle_speed;
    float settle_height;
    MkProcEntryFn callback;
    MkHdr* target;
    unsigned int target_instance;
} ObjectMonitorPdata;

int yy_evil_time_active;
int yinyang_ok_to_switch;
int yinyang_good_music_index;
int yinyang_evil_music_index;
MslSoundHandle yinyang_current_music;
static MiscBgndData misc_bgnd_data;
static CollisionShape fortress_exclusion_zone;
static MkObjRef debug_p2_axis_item;
static MkObjRef debug_p1_axis_item;

MslSoundHandle snd_req(int sound_id);
void snd_stop(MslSoundHandle handle);
MslSoundHandle plyr_snd_req(int sound_id);
void init_collision_system(void);
int sprintf(char* dest, const char* format, ...);
int is_weapon_style(int style);
void advance_active_moveset(FighterMirror* fighter);
float p_player_body_explode(void);
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
float uv_v3_to_v3_dist(Vec* out, const Vec* from, const Vec* to);
float p_monitor_objs_sobjs(void);
float p_statue_xpd_callback(void);

int evil_tune_tbl[3] = {0x1BF3, 0x1BF4, 0x1BF5};
int good_tune_tbl[4] = {0x1BEC, 0x1BED, 0x1BEE, 0x1BEF};

void ck_put_weapon_away(PlyrInfo* player) {
    if (player != 0 &&
        is_weapon_style(player->slot.fighter->active_moveset) != 0) {
        advance_active_moveset(g_game_info.active_player->slot.fighter);
    }
}

void player_body_explode(int model_index, const Vec* position, float scale) {
    PlayerBodyExplodePdata* pdata;

    if (_create_mkproc_generic_nostack(
            0x2097, 0x1F, (MkProcEntryFn)p_player_body_explode,
            sizeof(PlayerBodyExplodePdata), (MkHdr**)&pdata) != 0) {
        pdata->model_index = model_index;
        pdata->position.x = position->x;
        pdata->position.y = position->y;
        pdata->position.z = position->z;
        pdata->scale = scale;
    }
}

void reset_collision_system(void) {
    init_collision_system();
}

void init_plyr_severed_limb_list(PlyrInfo* player) {
    FighterMirror* fighter = player->slot.fighter;
    int limb;

    for (limb = 0; limb < 15; limb++) {
        FighterObjectRef* severed = &fighter->severed_limbs[limb];
        MkObj* object = severed->object;

        if (object == 0 || object->hdr.instance != severed->instance) {
            object = obj_sever_limb(
                player->slot.mirror_a, limb,
                fighter->runtime_data->half_sever_velocities, 1);
            if (object != 0) {
                severed->object = object;
                severed->instance = object->hdr.instance;
            }
        }
    }
}

typedef struct FishObjectLatch {
    char pad00[0x10];
    MkObj* object;
    unsigned int instance;
} FishObjectLatch;

typedef struct YinyangFishPair {
    MkObj* good_fish;
    MkObj* bad_fish;
    FishObjectLatch* active_fish;
} YinyangFishPair;

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
    MkObj* object, const int* tags, int flipped_bones, void* ground_colls) {
    if (tags != 0) {
        build_bones_tbl(object, tags);
    }
    object->flipped_bones = flipped_bones;
    object->ground_colls = ground_colls;
}

void debug_create_axis_indicator(PlyrInfo* player, const Vec* position) {
    MkObj* object;

    if (player->controller_slot == 0) {
        object = debug_p1_axis_item.object;
        if (object == 0 ||
            object->hdr.instance != debug_p1_axis_item.instance) {
            object = 0;
        }
    } else {
        object = debug_p2_axis_item.object;
        if (object == 0 ||
            object->hdr.instance != debug_p2_axis_item.instance) {
            object = 0;
        }
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
        object->flags_08 |= 0x80;
    }
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

float p_skytemple_bodysplat(void) {
    MkObj* object = 0;
    SkyTempleBodysplatPdata* pdata = (SkyTempleBodysplatPdata*)apdata;
    unsigned int object_instance = 0;
    float scale;

    object =
        load_named_model_from_slot(0x2001E, "ST_BLOODSPLAT", 0x2094, 0);
    if (object != 0) {
        insert_fgnd_mkobj(object);
        object->pos = pdata->position;
        object->flags_08 |= 2;
        object->scale.x = 1.0f;
        object->scale.y = 1.0f;
        object->scale.z = 1.0f;
        object_instance = object->hdr.instance;
    }

    scale = 1.0f;
    while (scale < 2.5f) {
        MkObj* live_object = object;

        if (live_object == 0 ||
            live_object->hdr.instance != object_instance) {
            live_object = 0;
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

void do_yinyang_statue_explosion(MkHdr* statue) {
    ObjectMonitorPdata* pdata;

    if (statue == 0) {
        return;
    }

    if (_create_mkproc_generic_nostack(
            0x2095, 0x1F, p_monitor_objs_sobjs,
            sizeof(ObjectMonitorPdata), (MkHdr**)&pdata) != 0) {
        pdata->settle_height = 0.45f;
        pdata->target = statue;
        pdata->target_instance = statue->instance;
        pdata->min_pos_x = 0.01f;
        pdata->min_pos_y = 0.03f;
        pdata->min_pos_z = 0.01f;
        pdata->min_vel_x = 0.01f;
        pdata->min_vel_y = 0.01f;
        pdata->min_vel_z = 0.01f;
        pdata->gravity = 0.003f;
        pdata->settle_speed = 0.15f;
        pdata->callback = p_statue_xpd_callback;
    }
}

void fortress_setup_exclusion_zone(
    const Vec* center, float width, float height, float depth, float angle) {
    build_col_shape_vertical_box(
        &fortress_exclusion_zone, center, width, height, depth, angle);
}

/* Soft ceiling: is_point_in_fortress_exclusion_zone ~83.08% -- bool emit. */
int is_point_in_fortress_exclusion_zone(const Vec* point) {
    return is_point_inside_shape(&fortress_exclusion_zone, point) > 0;
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
