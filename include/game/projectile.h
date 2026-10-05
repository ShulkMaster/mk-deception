#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "math/gxVect.h"

typedef struct ProjectileImpaleInfo {
    int parent_bone;
    Vec parent_offset;
    int child_bone;
    Vec child_offset;
    Vec rotation;
} ProjectileImpaleInfo;

typedef struct MkObj MkObj;
typedef struct PlyrPdata PlyrPdata;

#ifdef __cplusplus
extern "C" {
#endif

void set_active_projectile_target_ground(
    float ticks, float collision_ticks, float collision_radius);
void set_active_projectile_upward_attack(const Vec* target);
int get_bid_with_flip(MkObj* object, unsigned int bone_id);
void active_projectile_setup_done(void);
void set_active_projectile_velocity_damp(const Vec* damping);
void set_active_projectile_max_ticks(int ticks);
void set_active_projectile_target_pos(const Vec* position);
void set_active_projectile_p_handler(float (*handler)(void));
void set_active_projectile_dn_sound(int sound);
void set_active_projectile_sound(
    int start_sound, int flight_sound, int impact_sound);
void set_active_projectile_velocity(const Vec* velocity);
void set_active_add_ang_y(float angle);
void set_active_projectile_hit_gnd_script(unsigned int script_index);
void set_active_projectile_end_script(unsigned int script_index);
void set_active_projectile_hit_script(unsigned int script_index);
void set_active_projectile_block_script(unsigned int script_index);
void set_active_projectile_collision_info(
    float radius, int enabled, float height, float depth);
void set_active_projectile_random_rot(float x, float y, float z);
void set_active_projectile_random_pos(float x, float y, float z);
void set_active_projectile_impale_info(
    ProjectileImpaleInfo* info, const int* bone_tags);
void set_active_projectile_continue_thru_hit(void);
void set_active_projectile_3d_track(void);
void set_active_projectile_2d_track(void);
void set_active_projectile_not_duckable(void);
MkObj* start_projectile_from_sidekick_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset);
MkObj* start_projectile_from_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset);
int check_for_throw(PlyrPdata* player);
float p_projectile_die(void);
void get_projectile_script_velocity(Vec* velocity);
void get_projectile_script_last_pos(Vec* position);
PlyrPdata* get_projectile_script_plyr_pdata(void);
int get_projectile_his_plyr_num(void);
int get_projectile_script_plyr_num(void);

#ifdef __cplusplus
}
#endif

#endif
