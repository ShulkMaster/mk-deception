#ifndef GAME_BGND_H
#define GAME_BGND_H

#include "game/bgnd_types.h"

typedef struct MkObj MkObj;
typedef struct MkHdr MkHdr;
typedef struct MkSobj MkSobj;
typedef struct PlyrPdata PlyrPdata;
typedef struct PlyrInfo PlyrInfo;
typedef struct Vec Vec;
typedef struct LightDef LightDef;

typedef struct BgndUvScrollEntry {
    unsigned int object_id;
    float u1;
    float v1;
    float u2;
    float v2;
    unsigned int translucent;
} BgndUvScrollEntry;

typedef struct BgndSwapTextureEntry {
    unsigned int sobj_id;
    int material_id;
} BgndSwapTextureEntry;

typedef struct BgndAppendTextureEntry {
    unsigned int sobj_id;
    int material_id;
    char* texture_name;
    int texture_slot;
} BgndAppendTextureEntry;

typedef struct LoadBgndCtx {
    int art_id;
    MkObj* bgnd_obj;
    union {
        void* field_08;
        int pad;
    };
} LoadBgndCtx;

#ifdef __cplusplus
extern "C" {
#endif

/* Critical Krypt / background entry points (Wave 2 NonMatching scaffold). */

void bgnd_anim_camera_ended(void);
void bgnd_anim_camera_setup(void);
void bgnd_clear_danger_zone_callback(PlyrPdata* pdata);
void bgnd_rx_notify(PlyrInfo* player_info, int reaction,
                    unsigned int power_level, unsigned int flags);
float bgnd_current_rx_get_info(int info_id);
void bgnd_current_rx_set_info(int info_id, void* script_args, float value);
void pebble_get_pos(int player, int index, Vec* position);
void pebble_set_pos(int player, int index, Vec* position);
MkHdr* get_sobj_pebble_obj(MkSobj* object);
void bgnd_start_sobj_uv_scroll(int object_id,
                               float u1, float v1, float u2, float v2,
                               unsigned int translucent);
void bgnd_xfer_attacker(int script_function);
void bgnd_hide_sobj_list(unsigned int* object_ids);
void bgnd_add_fx_to_hide(const char* effect_name);
void bgnd_add_wall_to_hide(int object_id);
void bgnd_add_wall_to_unhide(int object_id);
float bgnd_process_active_sobj_info(int info_id, void* script_args,
                                    float angle_component, float offset);
void bgnd_start_sh_fx(void);
void bgnd_start_sobj_uv_scroll_tbl(BgndUvScrollEntry* entries);
void bgnd_light_set_color(int light_id, float red, float green, float blue);
void bgnd_takeover_plyr(PlyrInfo* player);
void bgnd_start_wall_hider(int unused);
void bgnd_remove_wall_from_hider(unsigned int object_id);
void bgnd_set_new_ground_plane(void* script, float ground_y);
void bgnd_sobj_start_morph(MkObj* object, int sobj_id, int script_id,
                           unsigned int flags);
void bgnd_act_at_time(int ticks, int script_function, void* script,
                      float x, float y, float z);
void bgnd_pebble_gravity(int player, void* script, float gravity);
void bgnd_swap_textures(int sobj_id, int material_id, unsigned int frame);
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries,
                            unsigned int frame);
void bgnd_append_texture_to_material(int sobj_id, int material_id,
                                     char* texture_name, int texture_slot);
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries);
void bgnd_fade_object(int object_id, void* script, float fade_step);
void bgnd_replace_tex_with_wiff_and_ani(
    int object_id, const char* wiff_name, float frame_rate,
    int first_frame, int texture_type);
void bgnd_pulsate_object(
    int object_id, int max_hold_ticks, int min_hold_ticks, void* script_args,
    float fade_in_step, float fade_out_step);
void bgnd_pulsate_object_with_caps(
    int object_id, int max_hold_ticks, int min_hold_ticks,
    unsigned int min_alpha, unsigned int max_alpha,
    float fade_in_step, float fade_out_step);
void bgnd_pulsate_object_with_caps_and_scale(
    int object_id, int max_hold_ticks, int min_hold_ticks,
    unsigned int min_alpha, unsigned int max_alpha, void* script_args,
    float fade_in_step, float fade_out_step,
    float scale_step_xz, float scale_step_y,
    float min_scale_xz, float min_scale_y,
    float max_scale_xz, float max_scale_y);
void pulsate_object(
    MkHdr* owner, int sobj_id, int max_hold_ticks, int min_hold_ticks,
    float fade_in_step, float fade_out_step);
float p_track_cam_ang_y_light(void);
MkObj* bgnd_place_point_light_for_ticks(
    LightDef* light_def, int ticks, int offset_from_tightrope, float radius_step);
void skytemple_arrange_fence_pebbles_around_pos(
    int player, unsigned int count, Vec* position);
void skytemple_set_fence_pebble_vel(
    int player, unsigned int count, float x, float y, float z);
void start_bl_beetles_live_top_floor(void);
void sh_lower_level_pebble_unhide(void);
void sh_lower_level_pebble_hide(void);
void bgnd_add_new_normal_check_for_hider(
    void* script_args, float normal_x, float normal_y, float normal_z,
    float distance);
void bgnd_init_pebbles(int player, unsigned int first, unsigned int end);
void bgnd_unhide_sobj_list(unsigned int* object_ids);
void load_bgnd_style(int player, const char* script_name, void* script_args);
int is_bgnd_locked(int bgnd_id);
int load_background(int bgnd_id);

#ifdef __cplusplus
}
#endif

#endif /* GAME_BGND_H */
