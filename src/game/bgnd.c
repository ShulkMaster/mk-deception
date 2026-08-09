#include "game/bgnd.h"
#include "game/game_info.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"

#pragma use_lmw_stmw on

typedef struct BgndAppendTextureEntry BgndAppendTextureEntry;
typedef struct BgndSwapTextureEntry BgndSwapTextureEntry;
typedef struct PlyrPdata PlyrPdata;
typedef void (*BgndScriptEntryFn)(void);

void* memset(void* dst, int c, unsigned long n);

extern int mode_of_play;
extern void* Camera;
extern float ShadowStrength;
extern float fog_density;
extern float fog_distance;
extern float fog_color_real[4];
extern GlobalBackgroundEntry global_background_data[];
extern char bgnd_animations[0x84];
extern int g_current_reaction_info[6]; /* 0x18 bytes; retail clears word at +0x14 */
extern int g_bgnd_collision_to_script_if[8];
extern void* g_active_obstacle_event_data;
extern void* g_active_bgnd_col_item;
extern void* bgnd_spec_light_list;
extern void* plyr_light_list;
extern void* bgnd_light_list;
extern void* weapon_trail_light_list;
extern unsigned int default_bgnd_bits[2];
extern unsigned int default_pz_bgnd_bits[2];

typedef struct BgndUnlockData {
    char pad00[0x10];
    unsigned int bgnds[2]; /* +0x10 */
    char pad18[0x20];
    unsigned int puzzle_bgnds[2]; /* +0x38 */
} BgndUnlockData;

extern BgndUnlockData gp_data;

void cam_set_intro_cam_pause_ticks(float ticks);
void RwImageSetGamma(float gamma);
void init_misc_bgnd_data(void);
void load_art_section_by_name(int art_id, char* name);
void* load_named_model_from_slot(int art_id, char* name, int flags, int unk);
void load_background_anims(void* anims, int bgnd_id);
void init_weapon_trail_light_list(void);
void load_lights(void* lights, void* list);
void insert_fgnd_mkobj(void* bgnd_obj);
void set_background_color(int r, int g, int b, int a);
void UpdateShadowCameraLightSource(void* light);
void turn_fog_on(void);
void turn_fog_off(void);
void initialize_bgnd_collisions(void* data);
void load_effect_bank(int bank);
void mk_chess_init_bgnd_for_fight_mode(void);

void bgnd_level_fatality_end(void) {}
void bgnd_level_fatality_start(int player) {
    (void)player;
}
void bgnd_level_transition_end(void) {}
void bgnd_level_transition_start(void) {}
int is_bgnd_locked(int bgnd_id) {
    unsigned long long unlocked;
    unsigned long long mask;

    if (bgnd_id < 0 || bgnd_id > 0x23) {
        return 1;
    }

    mask = 1ULL << bgnd_id;
    if (mode_of_play == 6) {
        unlocked =
            ((unsigned long long)(gp_data.puzzle_bgnds[0] | default_pz_bgnd_bits[0]) << 32) |
            (gp_data.puzzle_bgnds[1] | default_pz_bgnd_bits[1]);
        return (unlocked & mask) == 0;
    }

    if ((g_game_info.field_04 & 0x80) != 0 && (g_game_info.field_04 & 0x40) == 0) {
        return 0;
    }

    unlocked = ((unsigned long long)(gp_data.bgnds[0] | default_bgnd_bits[0]) << 32) |
               (gp_data.bgnds[1] | default_bgnd_bits[1]);
    return (unlocked & mask) == 0;
}
int get_next_bgnd(void) { return 0; }
void bgnd_force_specularity_off_for_material(void) {}
void bgnd_sobj_set_texture_kl_values(void) {}
float script_fabs(float value) {
    (void)value;
    return 0.0f;
}
void animate_obj(void) {}
void set_obj_light_flags(MkObj* object, int flags) {
    (void)object;
    (void)flags;
}
void set_obj_ang(MkObj* object, float x, float y, float z) {
    (void)object;
    (void)x;
    (void)y;
    (void)z;
}
void set_obj_pos(MkObj* object, float x, float y, float z) {
    (void)object;
    (void)x;
    (void)y;
    (void)z;
}
void bgnd_sobj_start_morph(int object_id, int start_shape, int end_shape,
                           int ticks) {
    (void)object_id;
    (void)start_shape;
    (void)end_shape;
    (void)ticks;
}
void bgnd_get_obj_pointer(void) {}
void scripted_camera_script_exit(void) {}
void skytemple_arrange_fence_pebbles_around_pos(int player, int count,
                                                const Vec* position) {
    (void)player;
    (void)count;
    (void)position;
}
void skytemple_set_fence_pebble_vel(int player, int count, float x, float y,
                                    float z) {
    (void)player;
    (void)count;
    (void)x;
    (void)y;
    (void)z;
}
static void p_sh_fatality_body_parts(void) {}
static void sh_update_fatality_body_part(void) {}
static void sh_start_fatality_body_parts(void) {}
void p_sh_throw_plyr_in_grinder(void) {}
static void p_sh_bottom_floor_blood_fall(void) {}
static void sh_update_blood_fall_pebbles(void) {}
static void sh_init_bottom_floor_blood_fall_pebbles(void) {}
void sh_lower_level_pebble_unhide(void) {}
void sh_lower_level_pebble_hide(void) {}
static void sh_load_objs(void) {}
void bgnd_sh_level_2(void) {}
void bgnd_sh_level_1(void) {}
void bgnd_start_sh_fx(void) {}
void bgnd_clean_slaughterhouse(void) {}
void vdestroy_slaughterhouse_pdata(void) {}
static void destroy_slaughterhouse_pdata(void) {}
void start_bl_beetles_live_top_floor(void) {}
static void p_bl_beetle_brains(void) {}
static void beetle_squashed(void) {}
static void bl_process_beetle_follow_plyr_personality(void) {}
static void bl_process_beetle_under_glass_personality(void) {}
static void bl_process_beetle_under_glass_traveller_personality(void) {}
static void bl_process_beetle_runaway_personality(void) {}
static void bl_process_beetle_transition_personality(void) {}
static void bl_process_beetle_chilling(void) {}
static void bl_process_beetle_track_plyr(void) {}
static void bl_process_beetle_climb_a_wall(void) {}
static void bl_process_general_movement(void) {}
static void bl_init_beetle_pebbles_second_floor(void) {}
static void bl_init_beetle_pebbles_first_floor(void) {}
void bgnd_clean_beetlelair(void) {}
void bgnd_reg_col_cb_for_beetle_lair(void) {}
static void beetle_lair_collision_cb(void) {}
void r_beetle_lair_transition(void) {}
static void beetle_lair_react_to_wall_danger_zone_cb(void) {}
static void p_beetle_lair_wall_breaking_controller(void) {}
static void p_beetle_lair_watch_remaining_fall_scene(void) {}
static void winner_watching_him_fall(void) {}
static void victim_fall_down_a_level(void) {}
static void p_beetle_lair_front_wall_breaking(void) {}
static void p_beetle_lair_downstairs_wall_break_cam_control(void) {}
static void p_beetle_lair_column_breaking(void) {}
static void p_launch_final_column_piece(void) {}
static void p_launch_column_piece(void) {}
static void p_bl_flip_column_piece(void) {}
void bgnd_set_viewing_of_danger_zones(void) {}
void bgnd_set_danger_zone_y_angle(void) {}
void bgnd_set_danger_zone_depth(void) {}
void bgnd_set_danger_zone_radius(void) {}
void bgnd_set_danger_zone_width(void) {}
void bgnd_set_danger_zone_center_position(void) {}
void bgnd_delete_danger_zone(void) {}
void bgnd_enable_danger_zone(void) {}
void bgnd_set_active_danger_zone(void) {}
void bgnd_create_danger_zone(void) {}
static void plyr_is_prone(void) {}
void bgnd_delete_proc_by_id(void) {}
void p_bgnd_script_in_proc(void) {}
void bgnd_start_script_in_proc_bigstack(void) {}
void bgnd_start_script_in_proc(void) {}
static void p_npc_track_colshape(void) {}
void bgnd_npc_add_collision_shape(void) {}
void bgnd_npc_get_pos(void) {}
void bgnd_npc_set_pos_y(void) {}
void bgnd_npc_set_ani_speed(void) {}
void bgnd_npc_get_aux_int_data(void) {}
void bgnd_npc_set_aux_int_data(void) {}
void bgnd_add_scripted_brains_to_npc(void) {}
void bgnd_add_brains_to_npc(void) {}
void bgnd_npc_like_plyr(void) {}
void bgnd_npc_set_pos_vel(void) {}
void bgnd_npc_set_pos_vel_heading(void) {}
void bgnd_npc_set_scale(void) {}
void bgnd_npc_get_ang_y(void) {}
void bgnd_npc_adjust_y_ang(void) {}
void bgnd_npc_set_y_ang(void) {}
void bgnd_npc_set_pos(void) {}
void bgnd_fetch_npc(void) {}
void bgnd_create_named_npc_in_slot(void) {}
void bgnd_npc_start_ani(void) {}
static void bgnd_npc_play_ani(void) {}
void bgnd_npc_idle(void) {}
void bgnd_set_launch_velocity_based_on_sobj_pos(void) {}
void bgnd_set_sobj_launch_params_exact(void) {}
void bgnd_set_sobj_launch_params(void) {}
void bgnd_launch_sobj(void) {}
void bgnd_kill_all_launched_sobjs(void) {}
void bgnd_set_collision_plane_for_launched_sobj(void) {}
void bgnd_set_kill_plane_for_launched_sobj(void) {}
void start_sobj_launch_monitor(void) {}
static void p_bgnd_launch_sobj_monitor(void) {}
void bgnd_chunk_explosion_match_velocity_with_params(void) {}
static void launch_sobj_watch_dist_from_orgin(void) {}
static void launch_sobj_watch_y_far_down(void) {}
static void launch_sobj_watch_y_ground_plane(void) {}
void bgnd_launch_chunk(void) {}
void start_chunk_launch_monitor(void) {}
static void p_bgnd_launch_chunk_monitor(void) {}
int get_sobj_pebble_obj(void) { return 0; }
int get_general_pebble_data(void) { return 0; }
void ncs_create_pebble_monitor_proc(void) {}
void ncs_set_pebble_pos(void) {}
void ncs_create_pebbles_with_sobj(void) {}
void pebble_turn_culling_off(int player) {
    (void)player;
}
void pebble_turn_culling_on(int player) {
    (void)player;
}
void pebble_unhide_me(int player, int index) {
    (void)player;
    (void)index;
}
void pebble_hide_me(int player, int index) {
    (void)player;
    (void)index;
}
void pebble_setup_bounce_props(int player, int index, const Vec* velocity,
                               int flags) {
    (void)player;
    (void)index;
    (void)velocity;
    (void)flags;
}
void pebble_set_ang_vel(int player, int index, const Vec* velocity) {
    (void)player;
    (void)index;
    (void)velocity;
}
void pebble_set_ang(int player, int index, const Vec* angles) {
    (void)player;
    (void)index;
    (void)angles;
}
void pebble_set_scale(int player, int index, const Vec* scale) {
    (void)player;
    (void)index;
    (void)scale;
}
void pebble_set_vel(int player, int index, const Vec* velocity) {
    (void)player;
    (void)index;
    (void)velocity;
}
void pebble_get_pos(void) {}
void pebble_set_pos(int player, int index, const Vec* position) {
    (void)player;
    (void)index;
    (void)position;
}
void bgnd_pebble_burst_at_pos(void) {}
void bgnd_pebble_burst_at_pebble_pos(void) {}
void bgnd_pebble_burst_at_chunk_pos(void) {}
void bgnd_pebble_burst_set_end_state(void) {}
void bgnd_pebble_burst_set_value_min_max(void) {}
void bgnd_pebble_burst_set_value(void) {}
static void bgnd_pebble_burst_at(void) {}
void bgnd_pebble_fetch_current_info(void) {}
void bgnd_pebble_set_current_info(void) {}
void bgnd_pebble_set_current_pebble(void) {}
void bgnd_pebble_change_current_end_behavior(void) {}
void bgnd_pebble_change_current_behavior_to_bounce(void) {}
void bgnd_pebble_change_current_behavior(void) {}
void bgnd_pebble_launch_at_time(void) {}
void bgnd_pebble_simple_launch_at_time(void) {}
void bgnd_create_pebbles_with_sobj(void) {}
void bgnd_create_pebbles(void) {}
void bgnd_set_material_color(void) {}
void bgnd_fetch_sobj(void) {}
MkObj* bgnd_fetch_obj(int model_id) {
    (void)model_id;
    return 0;
}
void bgnd_unhide_pebbles(void) {}
void bgnd_hide_pebbles(void) {}
void bgnd_pebble_rand_scale(void) {}
void bgnd_pebble_gravity(void) {}
void bgnd_init_pebbles(void) {}
static void p_pebble_manual_monitor(void) {}
static void p_pebble_burst_monitor(void) {}
static void p_pebble_path_monitor(void) {}
void bgnd_start_cracks(void) {}
void bgnd_remove_cracks(void) {}
void bgnd_init_cracks(void) {}
void bgnd_place_crack_when_plyr_hits_ground(void) {}
static void p_crack_placer(void) {}
void bgnd_kill_fx(void) {}
void bgnd_set_fx_z_offset(void) {}
void bgnd_set_fx_ang_y(void) {}
void bgnd_set_fx_ang_dir_to_i_vector(void) {}
void bgnd_launch_fx_at_active_sobj_pos_with_offset(void) {}
void pfxhandle_bgnd_spawn_at_position(void) {}
void bgnd_launch_fx_to_sobj(void) {}
void bgnd_launch_fx_at_position(void) {}
void bgnd_fx_get_binded_obj(void) {}
void bgnd_launch_fx_at_plyr_bid(void) {}
void bgnd_pfxhandle_spawn_at_bid(void) {}
void bgnd_launch_fx_at_bid_of_mkobj(void) {}
void bgnd_launch_fx_at_sobj_pos(char* name, int sobj_id, float y_offset) {
    (void)name;
    (void)sobj_id;
    (void)y_offset;
}
void bgnd_launch_plyr_blood_fx(void) {}
void bgnd_launch_fx_at_plyr_pos_and_y(void) {}
void ncs_bgnd_preload_named_model(void) {}
void bgnd_enable_obj_pos_and_ang_setting(void) {}
void bgnd_preload_named_model(void) {}
void bgnd_get_preload_obj(void) {}
void bgnd_start_preload_sobj_morph(void) {}
void bgnd_start_preload_sobj_uv_scroll(void) {}
void bgnd_hide_preload_obj(void) {}
void bgnd_unhide_preload_obj(void) {}
void bgnd_init_timers(void) {}
void bgnd_start_timer(void) {}
void bgnd_timer_get_tick_count(void) {}
static void p_bgnd_timer_monitor(void) {}
void bgnd_collison_if_to_scripts_activate(void) {}
void bgnd_collison_if_set_return_result(void) {}
void bgnd_collison_if_set_info(void) {}
void bgnd_collision_if_rx_override(void) {}
void bgnd_collision_if_monitor_col_as(void) {}
void bgnd_collison_if_monitor_col(void) {}
void bgnd_collision_if_enable_col(void) {}
void bgnd_collision_if_disable_col(void) {}
static void bgnd_collision_to_script_interface(void) {}
void bgnd_swap_level(void) {}
void bgnd_move_plyrs_to_initial_pos(void) {}
void bgnd_set_new_ground_plane(void) {}
void bgnd_set_player_shadow_ground_plane(void) {}
void bgnd_enable_wall_hider(void) {}
void bgnd_set_wall_hide_distance(void) {}
void bgnd_add_fx_to_hide(void) {}
void bgnd_add_wall_to_unhide(void) {}
void bgnd_add_wall_to_hide(void) {}
void bgnd_add_new_normal_check_for_hider(void) {}
void bgnd_start_wall_hider(void) {}
void bgnd_remove_wall_from_hider(void) {}
static void p_hide_walls(void) {}
void bgnd_place_object_at_position(void) {}
void bgnd_place_weapon_at_position(void) {}
void bgnd_get_item_from_displayed_list(void) {}
void disable_bgnd_obj_repel(void) {}
void enable_bgnd_obj_repel(void) {}
void bgnd_get_exec_tick_ctr(void) {}
void bgnd_act_at_time(void) {}
static void p_act_at_time(void) {}
void bgnd_run_camera_script(void) {}
void ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void) {}
void spad_set_y_angle_plus_offset_from_xz_vector(void) {}
void spad_set_vector_setting(void) {}
void spad_norm_vector(void) {}
void spad_rotate_xz_vector(void) {}
void spad_xz_dot_xz(void) {}
void spad_set_vector(void) {}
void spad_xz_length_vector(void) {}
void spad_get_pos(void) {}
void spad_sub_vectors(void) {}
void spad_add_vector(void) {}
void spad_set_vector_y(void) {}
void spad_xz_cos_two_vectors(void) {}
void spad_set_heading_vector_to(void) {}
void spad_scale_vector(void) {}
void bgnd_xfer_attacker(void) {}
void bgnd_process_collision_info(void) {}
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player) {
    (void)player;
}
void dont_fence_plyr_in(void) {}
void bgnd_takeover_plyr(void) {}
void bgnd_process_active_sobj_info(void) {}
void bgnd_move_player(void) {}
int bgnd_get_first_shape_center_for_obstacle_id(int obstacle_id,
                                                int scratch_index) {
    (void)obstacle_id;
    (void)scratch_index;
    return 0;
}
void bgnd_make_displayed_item_pickupable_at_active_sobj_pos(void) {}
void bgnd_rotate_xz_about_orgin_active_sobj(void) {}
void bgnd_hide_active_sobj(void) {}
void bgnd_unhide_active_sobj(void) {}
void bgnd_get_active_sobj_pos(void) {}
void bgnd_apply_active_sobj_pos_vel_drag(void) {}
void bgnd_set_active_sobj_pos_vel(void) {}
void bgnd_set_active_sobj_rop(void) {}
void bgnd_set_active_sobj_scale(void) {}
void bgnd_set_active_sobj_ang(void) {}
void bgnd_set_active_sobj_pos(void) {}
void bgnd_reset_sobj(void) {}
void bgnd_update_active_mksobj(void) {}
void bgnd_set_active_sobj_zoffset(void) {}
void bgnd_active_sobj_no_ztest(void) {}
void bgnd_active_sobj_no_zwrite(void) {}
void bgnd_set_active_sobj_in_obj(void) {}
void bgnd_is_active_sobj_hidden(void) {}
void bgnd_set_active_sobj(void) {}
void bgnd_sobj_cam_frustum_test_into_transparent(void) {}
void obj_sobj_cam_frustum_test_into_transparent(void) {}
void bgnd_sobj_cam_volume_test_steer_over(void) {}
void bgnd_clear_danger_zone_callback(int zone) {
    (void)zone;
}
void bgnd_register_danger_zone_callback(void) {}
void bgnd_rx_notify(void) {}
void bgnd_current_rx_set_info(void) {}
void bgnd_current_rx_get_info(void) {}
void bgnd_setup_rx_handler(void) {}
void bgnd_anim_camera_ended(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    void* script_ptr;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    info = &g_game_info;
    active_cmdscript = script;
    script_ptr = info->section != 0 ? info->section->cam_ended_script : 0;
    if (script_ptr != 0) {
        cmdscript_setup_execution(info->cmdscript, (unsigned int)script_ptr);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((int (*)(CmdScript*))script->vtbl->destroy)(script);
    }
}

void bgnd_anim_camera_setup(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    void* script_ptr;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    active_cmdscript = script;
    cam_set_intro_cam_pause_ticks(0.0f);
    info = &g_game_info;
    script_ptr = info->section != 0 ? info->section->cam_setup_script : 0;
    if (script_ptr != 0) {
        cmdscript_setup_execution(info->cmdscript, (unsigned int)script_ptr);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((int (*)(CmdScript*))script->vtbl->destroy)(script);
    }
}

void bgnd_fade_object(void) {}
static void p_bgnd_fade_object(void) {}
void pulsate_object(void) {}
void bgnd_pulsate_object(void) {}
void bgnd_pulsate_object_with_caps_and_scale(void) {}
void bgnd_pulsate_object_with_caps(void) {}
static void p_bgnd_pulsate_object(void) {}
static void p_pulsate_object(void) {}
void bgnd_make_mkobj_transl(void) {}
void make_subobject_transl(void) {}
void bgnd_make_object_transl(void) {}
void mks_xfer_collision_info_plyr_to_bgnd_script(
    PlyrPdata* player, BgndScriptEntryFn entry) {
    (void)player;
    (void)entry;
}
void mks_xfer_collision_info_plyr_to_script(BgndScriptEntryFn entry,
                                             int player) {
    (void)entry;
    (void)player;
}
void xfer_player_proc_to_script_manual_messaging(void) {}
void xfer_player_proc_to_script(void) {}
void bgnd_call_script_function(void) {}
void bgnd_append_texture_to_material(int sobj_id, int material_id,
                                     char* texture_name, int texture_slot) {
    (void)sobj_id;
    (void)material_id;
    (void)texture_name;
    (void)texture_slot;
}
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries) {
    (void)entries;
}
void bgnd_swap_textures(int sobj_id, int material_id, int frame) {
    (void)sobj_id;
    (void)material_id;
    (void)frame;
}
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries, int frame) {
    (void)entries;
    (void)frame;
}
void bgnd_rotate_sobj(void) {}
void bgnd_replace_tex_with_wiff_and_ani(void) {}
void bgnd_blood_control(void) {}
void bgnd_shadow_control(void) {}
void bgnd_always_face_y(void) {}
void bgnd_no_z_test(void) {}
void bgnd_no_z_write(void) {}
void bgnd_apply_zoffset(void) {}
void bgnd_sobj_set_priority(void) {}
void bgnd_set_sobj_uv_scroll_abs_values(void) {}
void bgnd_set_sobj_uv_scroll_rate_values(void) {}
void bgnd_init_all_uv_scroll_w_control(void) {}
void bgnd_destroy_sobj_uv_scroll_w_control(void) {}
void bgnd_start_sobj_uv_scroll_w_control(void) {}
void bgnd_restore_player(void) {}
void bgnd_force_plyr_ground_plane(void) {}
void bgnd_force_ground_to(void) {}
void bgnd_launch_plyr_up_and_forward_running(void) {}
void bgnd_launch_plyr_up_and_forward(void) {}
void bgnd_turn_off_backface_culling(void) {}
void bgnd_turn_on_backface_culling(void) {}
void bgnd_allow_dirty_floor(void) {}
void bgnd_set_plyr_gravity(void) {}
void bgnd_clean_up_floor(void) {}
void bgnd_sobj_set_rel_pos(void) {}
void bgnd_unhide_sobj_and_children(void) {}
void bgnd_hide_sobj_and_children(void) {}
void bgnd_unhide_sobj(void) {}
void bgnd_unhide_sobj_list(int list_id) {
    (void)list_id;
}
void bgnd_hide_sobj(void) {}
void bgnd_hide_sobj_list(int list_id) {
    (void)list_id;
}
void bgnd_sobj_get_ang(void) {}
void bgnd_sobj_set_ang(void) {}
void bgnd_sobj_set_pos_vel(void) {}
void bgnd_sobj_get_z_pos(void) {}
void bgnd_sobj_get_y_pos(void) {}
void bgnd_sobj_get_x_pos(void) {}
void bgnd_sobj_set_pos(void) {}
void bgnd_get_sobj_ang_y(void) {}
void bgnd_start_sobj_uv_scroll(void) {}
void bgnd_start_sobj_uv_scroll_tbl(void) {}
void bgnd_light_set_color(int light_id, float red, float green, float blue) {
    (void)light_id;
    (void)red;
    (void)green;
    (void)blue;
}
void bgnd_sobj_set_ani_framerate(void) {}
void bgnd_sobj_set_ani_frame(void) {}
void bgnd_sobj_set_alpha(void) {}
float bgnd_get_float(int value_id) {
    (void)value_id;
    return 0.0f;
}
unsigned int bgnd_get_u32(int value_id) {
    (void)value_id;
    return 0;
}
void bgnd_clear_face_opponent_flags(void) {}
int bgnd_get_int(int value_id) {
    (void)value_id;
    return 0;
}
void bgnd_create_sobjs(void) {}
void bgnd_place_point_light_for_ticks(void) {}
static void p_bgnd_point_light_life_span(void) {}
float degrees_to_rad(void) { return 0.0f; }
float rad_to_degrees(void) { return 0.0f; }
float int_to_float(void) { return 0.0f; }
int float_to_int(void) { return 0; }
void obj_sobj_set_material(void) {}
int force_atomic_material_alpha(void) { return 0; }
void p_track_cam_ang_y_light(void) {}
void load_bgnd_style(void) {}
void ncs_bgnd_nuke_collision_to_script_interface(void) {}
int retrieve_bgnd_obj(void) { return 0; }
void destroy_background_extras(void) {}
static void add_mkx_light_obj_to_bgnd_cleanup_list(void* obj) { (void)obj; }
int load_background(int bgnd_id) {
    char* anims;
    char* gbd;
    int entry_off;
    BgndDataTable* data_table;
    BgndMisc* misc;
    MkObj* bgnd_obj;
    ScriptSlot* slot;
    char* art_name;
    int art_id;
    int i;
    int n;
    int zero;
    char* react;
    char* col;
    int* effect_list;
    int effect_off;
    float inv255;
    float* fog_col;
    LoadBgndCtx ctx;
    GlobalBackgroundEntry* entry;

    anims = bgnd_animations;

    if (mode_of_play == 6 && bgnd_id != 0x17) {
        return 0;
    }

    RwImageSetGamma(1.0f);

    /* Array indexing coax: retail uses lwzx with bgnd_id<<4. */
    gbd = (char*)global_background_data;
    entry_off = bgnd_id * 4; /* word index into 16-byte records */
    entry = &global_background_data[bgnd_id];
    load_ssf((MkFileEntry*)((void**)gbd)[entry_off]);

    slot = cmdscript_loadfile_by_name(0xB, (char*)((void**)gbd)[entry_off + 1]);
    g_game_info.cmdscript = slot;

    data_table = (BgndDataTable*)get_data_table(slot, slot->table_count);
    g_game_info.section = data_table;
    misc = data_table != 0 ? data_table->misc : 0;
    g_game_info.misc = misc;

    init_misc_bgnd_data();

    zero = 0;
    g_game_info.field_64 = zero;
    g_game_info.field_60 = zero;
    g_game_info.field_94 = zero;
    react = anims + 0xF0;
    col = anims + 0x588;
    *(int*)(react + 0x14) = zero;
    i = 0;
    n = 8;
    do {
        *(int*)(col + i) = zero;
        i += 4;
    } while (--n);
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;

    data_table = g_game_info.section;
    if (data_table != 0 && (data_table->flags88 & 1) != 0) {
        return 1;
    }

    if (mode_of_play == 9 || mode_of_play == 10) {
        if ((entry->flags & 8) != 0) {
            art_name = data_table->art_name;
            art_id = 0x8003D;
        } else {
            return 0;
        }
    } else if (mode_of_play == 0xB) {
        if ((entry->flags & 0x10) != 0) {
            art_name = data_table->art_name;
            art_id = 0x140064;
        } else {
            return 0;
        }
    } else {
        art_name = data_table->art_name;
        art_id = 0x2001E;
    }

    if (bgnd_id == 0x16) {
        art_id = 0x18006D;
    }

    if (data_table != 0 && (unsigned int)data_table->art_name != 0) {
        load_art_section_by_name(art_id, art_name);
    }

    bgnd_obj = (MkObj*)load_named_model_from_slot(art_id, "BACKGROUND", 0x1004, 0);
    g_game_info.bgnd_obj = bgnd_obj;

    data_table = g_game_info.section;
    if (data_table != 0) {
        if (data_table->anims != 0) {
            load_background_anims(data_table->anims, bgnd_id);
        } else {
            memset(anims, 0, 0x84);
        }
    }

    init_weapon_trail_light_list();

    misc = g_game_info.misc;
    if ((unsigned int)misc->lights_spec != 0) {
        load_lights(misc->lights_spec, &bgnd_spec_light_list);
    }
    if ((unsigned int)misc->lights_plyr != 0) {
        load_lights(misc->lights_plyr, &plyr_light_list);
    }
    if ((unsigned int)misc->lights_bgnd != 0) {
        load_lights(misc->lights_bgnd, &bgnd_light_list);
    } else {
        g_game_info.bgnd_obj->light_flags = 0;
    }

    bgnd_obj = g_game_info.bgnd_obj;
    if (bgnd_obj != 0) {
        insert_fgnd_mkobj(bgnd_obj);
    } else {
        return 0;
    }

    if ((unsigned int)misc->lights_bgnd != 0) {
        g_game_info.bgnd_obj->light_flags = 0x1009;
    } else {
        g_game_info.bgnd_obj->light_flags = 0x1000;
    }

    data_table = g_game_info.section;
    set_background_color(
        (int)data_table->bg_r,
        (int)data_table->bg_g,
        (int)data_table->bg_b,
        (int)data_table->bg_a);

    g_game_info.field_34 = 0.0f;

    data_table = g_game_info.section;
    art_name = data_table->sky_name;
    if (art_name != 0 && art_name[0] != 0) {
        g_game_info.sky =
            (MkObj*)load_named_model_from_slot(art_id, art_name, 0x201F, 0);
    }

    if (g_game_info.sky != 0) {
        mk_insert((MkHdr*)g_game_info.sky, &g_game_info.bgnd_obj->child_list);
    }

    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&weapon_trail_light_list);
    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&bgnd_light_list);
    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&plyr_light_list);

    data_table = g_game_info.section;
    if ((data_table->flags70 & 1) != 0) {
        UpdateShadowCameraLightSource(&misc->shadow_cam_light);
    }

    misc = g_game_info.misc;
    data_table = g_game_info.section;
    ShadowStrength = misc->shadow_strength;
    inv255 = 255.0f;
    fog_col = fog_color_real;
    fog_col[0] = data_table->fog_r / inv255;
    fog_col[1] = data_table->fog_g / inv255;
    fog_col[2] = data_table->fog_b / inv255;
    fog_col[3] = data_table->fog_a / inv255;

    RwCameraSetNearClipPlane(Camera, data_table->near_clip);
    data_table = g_game_info.section;
    RwCameraSetFarClipPlane(Camera, data_table->far_clip_cam);
    fog_density = data_table->fog_density;
    fog_distance = data_table->fog_distance;
    if (data_table->fog_enable != 0) {
        turn_fog_on();
    } else {
        turn_fog_off();
    }

    if (mode_of_play != 9 && mode_of_play != 0xB) {
        initialize_bgnd_collisions(g_game_info.section);
    }

    if (Camera != 0) {
        data_table = g_game_info.section;
        RwCameraSetNearClipPlane(Camera, data_table->near_clip);
        data_table = g_game_info.section;
        RwCameraSetFarClipPlane(Camera, data_table->far_clip_cam);
    }

    bgnd_obj = g_game_info.bgnd_obj;
    g_game_info.bgnd_id = bgnd_id;
    ctx.art_id = art_id;
    ctx.bgnd_obj = bgnd_obj;
    ctx.pad = 0;

    slot = g_game_info.cmdscript;
    active_cmdscript->mko = slot;
    slot->load_ctx = &ctx;

    effect_list = g_game_info.section->effect_banks;
    if (effect_list != 0) {
        for (effect_off = 0; (i = effect_list[effect_off]) != 0; effect_off++) {
            load_effect_bank(i);
        }
    }

    g_game_info.cmdscript->load_ctx = 0;

    data_table = g_game_info.section;
    if ((unsigned int)data_table->load_script != 0) {
        slot = g_game_info.cmdscript;
        cmdscript_setup_execution(slot, (unsigned int)data_table->load_script);
        cmdscript_execute(slot);
    }

    g_game_info.field_68 = 0;
    misc = g_game_info.misc;
    if ((unsigned int)misc->script != 0) {
        slot = g_game_info.cmdscript;
        cmdscript_setup_execution(slot, (unsigned int)misc->script);
        cmdscript_execute(slot);
    }

    g_game_info.field_08 = 1;
    g_game_info.field_74 = 0;
    if (mode_of_play == 10) {
        mk_chess_init_bgnd_for_fight_mode();
    }
    return 1;
}

void init_bgnd_info_struct(void) {}
int get_bgnd_flags(void) { return 0; }
