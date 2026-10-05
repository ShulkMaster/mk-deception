#include "game/game_info.h"
#include "game/game.h"
#include "game/bgnd.h"
#include "game/pfxscript.h"
#include "runtime/cam.h"
#include "runtime/plyr_anim_pdata.h"
#include "game/plyr_globals.h"
#include "game/plyr.h"
#include "runtime/mk_obj.h"
#include "platform/main_jump.h"

static const float zero = 0.0f;

void fx_resume_emit(unsigned int handle);

void nbc_script_debug_point(void) {}

float bgnd_get_camera_z_pos(void) {
    CameraObj* cam;

    cam = camera_item.node;
    cam = MK_HDR_LIVE(cam, camera_item.instance);
    if (cam != 0) {
        return cam->pos.z;
    }
    return zero;
}

float bgnd_get_camera_y_angle(void) {
    CameraObj* cam;

    cam = camera_item.node;
    cam = MK_HDR_LIVE(cam, camera_item.instance);
    if (cam != 0) {
        return cam->ang.y;
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
        f1 = plyr_anim_pdata->frame;
    }
    return f1;
}

void bgnd_reset_players_animation_height(void) {
    plyr_obj->pos.value.y = plyr_obj->ground_colls_y + plyr_anim_pdata->anim_offset.y;
}

void bgnd_end_the_game_and_restart(void) {
    gamelogic_jump(2, p_gamelogic);
}

void bgnd_pfx_resume_effect(const char* name) {
    fx_resume_emit(fx_by_owner(name, 4));
}

void bgnd_pfx_reset_effect(const char* name) {
    fx_reset(fx_by_owner(name, 4));
}

void bgnd_unhide_mirror_guys(void) {
    plyr_turn_on_mirrorguy(&g_game_info.plyr0);
    plyr_turn_on_mirrorguy(&g_game_info.plyr1);
}

void bgnd_hide_mirror_guys(void) {
    plyr_turn_off_mirrorguy(&g_game_info.plyr0);
    plyr_turn_off_mirrorguy(&g_game_info.plyr1);
}
