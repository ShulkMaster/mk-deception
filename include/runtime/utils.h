#ifndef UTILS_H
#define UTILS_H

/* Misc gameplay / screen utilities (utils.o). */

#include "runtime/plyr_pdata.h"

typedef struct MkObj MkObj;
typedef struct MkSobj MkSobj;

typedef int (*MovieTapoutFn)(void);

void display_debug_damage(void);

/*
 * Dispatch a cinematic from movie_info[movie_id].
 * tapout: return non-zero to end early (see atm_movie_tapout in attract.c).
 * Returns 1 when a fullscreen Sofdec movie starts, else 0.
 */
int play_movie(int movie_id, MovieTapoutFn tapout);
void screen_engine_play_movie(int index);
int are_death_traps_on(void);
int get_blood_level(void);
int get_puzzle_rounds_to_win(void);
void get_point_on_circle(float* center, float radius, float angle, float* out);
void award_koins_to_player(int player, int amount, int koin_type);
void show_koin_award(int player, int amount);
void sobj_set_bounding_sphere_radius(void* sobj, float radius);
float sobj_get_bounding_sphere_radius(void* sobj);
int get_mkptr_count(void);
void setup_fixed_block_heaps(void);
void load_and_set_refl_on_weapon(void* weapon);
void pause_procs(int flag);
int get_level_fatality_done_flag_state(void);
void set_level_fatality_done_flag_state(int state);
void pos_cam_for_current_level(void);
void reset_severed_limbs(int player);
void set_far_clip_plane(float dist);
void* find_obj_by_id(int id);
void* proc_create(void* proc_fn, int proc_id);
int get_language(void);
void set_language(int language);
void initialize_language_settings(void);
int get_language_setting(void);
void blink_cursor(ScreenObj* obj, int proc_id, int on_ticks, int off_ticks);
void hide_or_show_2d_obj_by_id(int oid, int hide);
void service_game_timers(void);
void display_numerical_change(
    StringObj* string, int font, int start, int change,
    int ticks, int acceleration_interval, void* context);
void show_material(RpMaterial* material);
void hide_material(RpMaterial* material);
void material_set_color(RpMaterial* material, const RpMaterialColor* color);
void set_atomic_material_color_by_id(void* atomic, int id, int* color);
void set_atomic_material_color(void* atomic, int* color);
void obj_set_color_for_material_by_id(
    MkObj* obj, int id, const RpMaterialColor* color);
void obj_set_color_for_all_materials(void* obj, int* color);
void sobj_set_color_for_all_materials(void* sobj, int* color);
int save_profile(int player, int mode);
void save_both_profiles(int unused);
float p_load_profile(void);
int load_profile(int player, int port, unsigned char* code);
void pfx_2d_obj_set_alpha_by_id(int id, int alpha);
void pfx_2d_obj_set_alpha(ScreenObj* obj, int alpha);
void destroy_fade_box(void);
void create_fade_box(void);
void fade_from_black(int frames, int flag);
void fade_from_white(int frames, int flag);
void fade_to_black(int frames, int flag);
void fade_to_white(int frames, int flag);
void set_string_obj_alpha(void* obj, float alpha);
void set_screen_obj_alpha(void* obj, float alpha);
void* find_uv_scroll_control_for_obj(MkObj* owner);
void* material_start_uv_scroll(MkObj* owner, RpMaterial* material, float u1, float v1, float u2,
                               float v2);
void* sobj_start_uv_scroll(MkObj* owner, MkSobj* subobject, float u1, float v1, float u2,
                           float v2);
void* start_sobj_uv_scroll(MkObj* owner, int sobj_id, float u1, float v1, float u2,
                           float v2);
void* replace_sobj_texture_with_named_wiff(
    void* sobj, int handle, const char* texture, const char* wiff);
float sfrand_ab(float a, float b);
int random_percent(float percent);
float sfrand(float max);
float frand(float max);
float signrand(void);
unsigned int randu0(unsigned int max);
unsigned int random(void);
int get_mode_of_play(void);
void set_mode_of_play(int mode);
int player_control_allowed(void);
void pop_game_state(void);
void push_game_state(int state);
int is_game_state_in_stack(int state);
int get_game_state(void);
void reset_game_state(void);
void init_global_vars(void);
/* Returns elapsed usec in r3:r4; main uses the low word as the RNG seed. */
unsigned long long stop_usec_timer(int id);
void start_usec_timer(int id);
void get_clean_system(void);
int simple_3d_projectile_collision(
    const Vec* previous_position, const Vec* current_position,
    const Vec* target_position, int mode, float collision_radius_squared,
    float maximum_distance_squared, float close_distance_squared);
int is_blind(PlyrPdata* fighter);
int is_big_boss(PlyrPdata* fighter);
int has_sidekick(PlyrPdata* fighter);
int am_i_female(PlyrPdata* fighter);

extern char pathname_buffer[];
extern char usec_timer_data[];
extern int depth_of_field_active;

#endif
