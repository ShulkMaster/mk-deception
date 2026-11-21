#ifndef GAME_BGND_H
#define GAME_BGND_H

#include "game/bgnd_types.h"

typedef struct MkObj MkObj;
typedef struct MkHdr MkHdr;
typedef struct MkSobj MkSobj;
typedef struct PlyrPdata PlyrPdata;
typedef struct PlyrInfo PlyrInfo;

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

void bgnd_pfx_resume_effect(const char* name);
void bgnd_pfx_reset_effect(const char* name);
void bgnd_pebble_set_current_pebble(int player, int index);
void bgnd_pebble_set_current_info(unsigned int field, float value);
void bgnd_set_fx_ang_y(float angle);
void spad_set_heading_vector_to(int index, float heading, float offset);
void spad_set_vector_y(int index, float y);
void spad_set_vector_setting(int index, float x, float y, float z);
void spad_set_y_angle_plus_offset_from_xz_vector(int index, float x, float z, float offset);
MkSobj* bgnd_fetch_sobj(int model_index, unsigned int object_id);
void bgnd_start_script_in_proc(int process_id, unsigned int script_index);
void bgnd_anim_camera_ended(void);
void bgnd_anim_camera_setup(void);
void bgnd_clear_danger_zone_callback(PlyrPdata* pdata);
void bgnd_rx_notify(PlyrInfo* player_info, int reaction,
                    unsigned int power_level, unsigned int flags);
float bgnd_current_rx_get_info(int info_id);
void bgnd_current_rx_set_info(int info_id, void* script_args, float value);
void pebble_get_pos(int player, int index, Vec* position);
void pebble_set_pos(int player, int index, Vec* position);
void pebble_set_vel(int player, int index, Vec* velocity);
void pebble_set_scale(int player, int index, Vec* scale);
void pebble_set_ang(int player, int index, Vec* angles);
void pebble_set_ang_vel(int player, int index, Vec* velocity);
void pebble_setup_bounce_props(int player, int index, Vec* velocity, int flags);
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
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries, int frame);
void bgnd_append_texture_to_material(int sobj_id, int material_id,
                                     char* texture_name, int texture_slot);
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries);
void bgnd_set_material_color(
    int model_index, unsigned int object_id, int red, int green,
    int blue, int alpha);
void bgnd_fade_object(int object_id, void* script, float fade_step);
void bgnd_replace_tex_with_wiff_and_ani(
    int object_id, const char* wiff_name, float frame_rate,
    int first_frame, int texture_type);
void bgnd_pulsate_object(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step);
void bgnd_pulsate_object_with_caps(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step, unsigned int min_alpha, unsigned int max_alpha);
void bgnd_pulsate_object_with_caps_and_scale(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step, unsigned int min_alpha, unsigned int max_alpha,
    float scale_step_xz, float scale_step_y,
    float min_scale_xz, float min_scale_y,
    float max_scale_xz, float max_scale_y);
void pulsate_object(
    MkHdr* owner, int sobj_id, int max_hold_ticks, int min_hold_ticks,
    float fade_in_step, float fade_out_step);
float p_track_cam_ang_y_light(void);
MkObj* bgnd_place_point_light_for_ticks(
    LightDef* light_def, int ticks, float radius_step, int offset_from_tightrope);
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
int get_bgnd_flags(void);
int load_background(int bgnd_id);
void destroy_background_extras(void);
extern void bgnd_swap_level(int level);
float spad_xz_dot_xz(int lhs, int rhs);
void bgnd_collision_if_enable_col(int list_id, unsigned int collision_id);
MkObj* bgnd_fetch_obj(int object_id);
float bgnd_pebble_fetch_current_info(unsigned int field);
void bgnd_unhide_pebbles(int group);
void xfer_player_proc_to_script();
void bgnd_hide_pebbles(int player);
void bgnd_unhide_preload_obj(int model_index);
void bgnd_unhide_active_sobj(void);
void bgnd_collision_if_disable_col(int list_id, unsigned int collision_id);
extern GlobalBackgroundEntry global_background_data[];
float spad_xz_length_vector(int index);
extern MkObj* g_bgnd_preloaded_models[];
unsigned int get_next_bgnd(void);
float bgnd_move_plyrs_to_initial_pos(void);
void bgnd_rotate_sobj(unsigned int, void*, float, float, float);
float bgnd_process_collision_info( unsigned int operation, float value1, float value2, float value3, float value4, float value5, float value6, float value7, float value8);
void bgnd_set_danger_zone_radius(float);
void bgnd_set_active_sobj_scale(void *, float, float, float);
void bgnd_shadow_control(unsigned int);
float bgnd_get_sobj_ang_y(int);
void bgnd_set_player_shadow_ground_plane(int, float);
void bgnd_unhide_sobj_and_children(unsigned int);
void bgnd_delete_proc_by_id(int);
void bgnd_place_object_at_position(int object_id, int sobj_id, Vec* position, Vec* angles, int flags);
float script_fabs(float value);
void scripted_camera_script_exit(void);
void bgnd_hide_sobj_and_children(unsigned int);
void bgnd_collision_if_monitor_col_as(int, unsigned int, unsigned int, int);
void bgnd_unhide_sobj(unsigned int);
int float_to_int(void *, float);
void bgnd_set_danger_zone_width(float);
void bgnd_set_active_danger_zone(unsigned int);
void bgnd_set_danger_zone_depth(float);
void mks_xfer_collision_info_plyr_to_script(int script_function, int player);
void bgnd_get_active_sobj_pos(Vec*);
void bgnd_setup_rx_handler(int);
void bgnd_active_sobj_no_ztest(void);
void bgnd_clean_up_floor(void);
float bgnd_call_script_function(void);
void bgnd_delete_danger_zone(unsigned int zone_index);
unsigned int bgnd_get_u32(int value_id);
void bgnd_set_active_sobj_pos_vel(void *, float, float, float);
void bgnd_sobj_set_alpha(unsigned int, unsigned int);
void pebble_unhide_me(int player, int index);
void obj_sobj_set_material(MkSobj* object, unsigned int alpha);
void pebble_hide_me(int player, int index);
int bgnd_npc_get_aux_int_data(unsigned int, unsigned int);
MkObj* retrieve_bgnd_obj(void);
float bgnd_npc_get_ang_y(unsigned int npc_id);
void bgnd_launch_fx_at_plyr_pos_and_y(const char*, float);
void bgnd_collison_if_to_scripts_activate(void);
void bgnd_npc_set_pos_vel(unsigned int, float, float, float);
void bgnd_apply_active_sobj_pos_vel_drag(void *, float, float, float);
void bgnd_launch_fx_to_sobj(const char*, int);
void set_obj_pos(MkObj* object, float x, float y, float z);
void bgnd_set_wall_hide_distance(void *, float);
void bgnd_level_transition_end(void);
void bgnd_npc_set_y_ang(unsigned int, float);
void bgnd_add_scripted_brains_to_npc(unsigned int, unsigned int);
void bgnd_launch_fx_at_plyr_bid(const char*, int);
void bgnd_place_crack_when_plyr_hits_ground(unsigned int player_index);
void bgnd_apply_zoffset(unsigned int, void *, float);
int bgnd_get_int(int value_id);
void bgnd_npc_adjust_y_ang(unsigned int, float);
void bgnd_force_ground_to(void *, float);
void pebble_turn_culling_on(int player);
void bgnd_sobj_set_pos(unsigned int, void *, float, float, float);
float bgnd_get_float(int value_id);
float rad_to_degrees(void *, float);
void bgnd_init_cracks(void);
void bgnd_launch_plyr_blood_fx(int, int);
void bgnd_set_fx_ang_dir_to_i_vector(void);
void bgnd_set_danger_zone_y_angle(float);
void bgnd_sobj_set_ang(unsigned int, void *, float, float, float);
void bgnd_sobj_set_ani_frame(unsigned int, unsigned int, int);
void bgnd_set_active_sobj(int);
void set_obj_ang(MkObj* object, float x, float y, float z);
void* ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void);
void bgnd_launch_fx_at_position(const char* name, float x, float y, float z);
void ncs_bgnd_nuke_collision_to_script_interface(void);
int bgnd_timer_get_tick_count(int);
void bgnd_collison_if_set_return_result(int);
void spad_add_vector(int, void *, float, float, float);
void bgnd_hide_preload_obj(int);
void bgnd_npc_set_pos_y(unsigned int, float);
void bgnd_set_active_sobj_ang(void *, float, float, float);
void bgnd_pebble_change_current_end_behavior(int);
void bgnd_pebble_rand_scale(int, void *, float, float);
void make_subobject_transl(MkSobj* object);
void bgnd_init_timers(int);
void bgnd_clean_slaughterhouse(void);
void bgnd_create_sobjs(void);
void bgnd_reg_col_cb_for_beetle_lair(void);
void bgnd_move_player(unsigned int, int, int);
float spad_xz_cos_two_vectors(int, int);
void bgnd_init_all_uv_scroll_w_control(void);
void bgnd_pebble_burst_at_chunk_pos(int, int, int);
void bgnd_npc_get_pos(unsigned int, Vec*);
void bgnd_enable_danger_zone(unsigned int, int);
void bgnd_kill_fx(const char*);
MkObj* bgnd_get_preload_obj(int);
void bgnd_level_fatality_start(int player);
void bgnd_make_displayed_item_pickupable_at_active_sobj_pos(int item_id);
int bgnd_get_exec_tick_ctr(void);
void bgnd_set_danger_zone_center_position(float, float, float);
void bgnd_sobj_cam_volume_test_steer_over(unsigned int, float, float);
void bgnd_sobj_set_priority(unsigned int, int);
void bgnd_reset_sobj(int);
void bgnd_sobj_set_ani_framerate(unsigned int, unsigned int, void *, float);
void bgnd_sh_level_1(void);
void bgnd_make_object_transl(unsigned int object_id);
void bgnd_rotate_xz_about_orgin_active_sobj(float);
void bgnd_make_mkobj_transl(MkObj* object);
void init_bgnd_info_struct(void);
void bgnd_allow_dirty_floor(void);
void bgnd_kill_all_launched_sobjs(void);
MkObj* bgnd_fx_get_binded_obj(unsigned int);
void bgnd_sh_level_2(void);
void bgnd_collison_if_set_info(void);
void bgnd_destroy_sobj_uv_scroll_w_control(unsigned int);
void bgnd_level_transition_start(void);
void bgnd_remove_cracks(void);
void bgnd_update_active_mksobj(void);
float bgnd_sobj_get_z_pos(int);
void bgnd_pebble_burst_at_pebble_pos(int, int, int);
void bgnd_set_fx_z_offset(const char *, float);
void bgnd_start_preload_sobj_morph(int, int, unsigned int, unsigned int);
void start_sobj_launch_monitor(void);
void bgnd_force_plyr_ground_plane(unsigned int, void *, float);
void bgnd_set_active_sobj_zoffset(void *, float);
void bgnd_npc_set_pos_vel_heading(unsigned int, float);
float bgnd_sobj_get_y_pos(int);
void bgnd_npc_set_ani_speed(unsigned int, float);
void bgnd_level_fatality_end(void);
void bgnd_collision_if_rx_override(unsigned int);
void bgnd_start_timer(unsigned int, int, int);
void bgnd_npc_set_scale(unsigned int, float, float, float);
void bgnd_sobj_set_pos_vel(unsigned int, void *, float, float, float);
void bgnd_start_cracks(void);
float bgnd_blood_control(int, int, void *, float);
void dont_fence_plyr_in(int);
void bgnd_set_plyr_gravity(void *, float);
void pebble_turn_culling_off(int player);
int bgnd_launch_plyr_up_and_forward_running(void);
void bgnd_set_active_sobj_rop(int priority);
float degrees_to_rad(void *, float);
void bgnd_register_danger_zone_callback(PlyrPdata*, int);
void spad_rotate_xz_vector(int, void *, float);
void bgnd_npc_set_aux_int_data(unsigned int, unsigned int, int);
void bgnd_pebble_burst_at_pos(int, int, int, void *, float, float, float);
float bgnd_sobj_get_x_pos(int);
void bgnd_enable_wall_hider(unsigned int);
void start_chunk_launch_monitor(void);
int bgnd_is_active_sobj_hidden(void);
void bgnd_no_z_write(unsigned int);
void bgnd_clean_beetlelair(void);
void bgnd_pebble_simple_launch_at_time(int, int, unsigned int, int);
void bgnd_sobj_get_ang(unsigned int, Vec *);
void set_obj_light_flags(MkObj* object, int flags);
void bgnd_set_kill_plane_for_launched_sobj(int);
void bgnd_no_z_test(unsigned int);
void bgnd_npc_set_pos(unsigned int, float, float, float);
void bgnd_set_viewing_of_danger_zones(int enabled);
void bgnd_sobj_set_rel_pos(unsigned int, void *, float, float, float);
int bgnd_get_obj_pointer(int);
void bgnd_run_camera_script(int, int);
void bgnd_clear_face_opponent_flags(void);
void bgnd_hide_active_sobj(void);
void bgnd_pebble_burst_set_end_state(int, unsigned int, unsigned int, int);
void bgnd_always_face_y(unsigned int);
float int_to_float(int);
void bgnd_npc_like_plyr(unsigned int);
void bgnd_active_sobj_no_zwrite(void);
void bgnd_hide_sobj(unsigned int);

#ifdef __cplusplus
}
#endif

#endif /* GAME_BGND_H */
