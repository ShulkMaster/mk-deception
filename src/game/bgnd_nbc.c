#include "game/game_info.h"
#include "runtime/cam.h"

typedef struct PlyrAnimPdata {
    char pad00[0x38];
    float field38;
    char pad3C[0x18];
    float height_delta;
} PlyrAnimPdata;

typedef struct PlyrObj {
    char pad00[0x70];
    float base_height;
    char pad74[0x30];
    float anim_height;
} PlyrObj;

static const float zero = 0.0f;

extern PlyrAnimPdata* plyr_anim_pdata;
extern PlyrObj* plyr_obj;

void gamelogic_jump(int action, void (*logic)(void));
void fx_by_owner(int owner, int type);
void fx_resume_emit(void);
void fx_reset(void);
void plyr_turn_on_mirrorguy(void* mirrorguy);
void plyr_turn_off_mirrorguy(void* mirrorguy);
extern void p_gamelogic(void);

void nbc_script_debug_point(void) {}

/*
 * Soft ceiling: Q28's two-variable latch restores retail's initial r5 load
 * and improves both helpers, but MWCC keeps the live pointer in r3 and emits
 * a keep-arm move where retail coalesces it into r5.
 */
float bgnd_get_camera_z_pos(void) {
    CameraObj* raw;
    CameraObj* cam;

    raw = camera_item.node;
    if (raw != 0) {
        if (raw->instance != camera_item.instance) {
            cam = 0;
        } else {
            cam = raw;
        }
    } else {
        cam = 0;
    }
    if (cam != 0) {
        return cam->pos_z;
    }
    return zero;
}

float bgnd_get_camera_y_angle(void) {
    CameraObj* raw;
    CameraObj* cam;

    raw = camera_item.node;
    if (raw != 0) {
        if (raw->instance != camera_item.instance) {
            cam = 0;
        } else {
            cam = raw;
        }
    } else {
        cam = 0;
    }
    if (cam != 0) {
        return cam->ang_y;
    }
    return zero;
}

void hf_bgnd_set_in_setup_zone(int* ptr, int value) {
    *ptr = value;
}

void hf_bgnd_set_smasher_mode(int* ptr, int value) {
    *ptr = value;
}

float bgnd_get_anim_info(int arg) {
    float f1;

    f1 = zero;
    if (arg == 0) {
        f1 = plyr_anim_pdata->field38;
    }
    return f1;
}

void bgnd_reset_players_animation_height(void) {
    plyr_obj->anim_height = plyr_obj->base_height + plyr_anim_pdata->height_delta;
}

void bgnd_end_the_game_and_restart(void) {
    gamelogic_jump(2, p_gamelogic);
}

void bgnd_pfx_resume_effect(int owner) {
    fx_by_owner(owner, 4);
    fx_resume_emit();
}

void bgnd_pfx_reset_effect(int owner) {
    fx_by_owner(owner, 4);
    fx_reset();
}

void bgnd_unhide_mirror_guys(void) {
    plyr_turn_on_mirrorguy(&g_game_info.plyr0);
    plyr_turn_on_mirrorguy(&g_game_info.plyr1);
}

void bgnd_hide_mirror_guys(void) {
    plyr_turn_off_mirrorguy(&g_game_info.plyr0);
    plyr_turn_off_mirrorguy(&g_game_info.plyr1);
}
