#ifndef GAME_JMT_H
#define GAME_JMT_H

#include "math/gxVect.h"

typedef struct AnimPdata AnimPdata;
typedef struct MkHdr MkHdr;
typedef struct MkObj MkObj;
typedef struct PlyrPdata PlyrPdata;

void increment_taunts_performed(void);
int check_for_online_condition(PlyrPdata* player);
void online_sync_reset(void);
void remove_impaled_projectiles(void);
void adjust_kabal_position(void);
float get_adjusted_speed(float speed, float adjustment);
void player_area_collision_ticks(
    int region, int flags, void* script_args, float radius, float height,
    float depth, float ticks);
void flying_collision(
    int region, int reaction, int strength, void* script_args, float radius,
    float height, float reaction_rate, float exit_height,
    float collision_height, float max_frame, float max_ticks);
void kill_ermac_eyes(void);
void dizzy_kill_pfx(
    MkObj* opponent, int unused, PlyrPdata* player, int enabled);
void jmt_debug_script(int command, int value, const void* args, float scalar);
float animpdata_get_anim_hiframe(const AnimPdata* pdata);
MkHdr* mks_start_gusher(
    int player, int bone, void* script_args, float velocity_x,
    float velocity_y, float velocity_z, float direction_x,
    float direction_y, float direction_z);
void mks_plyr_stop(int player);
void mks_set_plyr_to_center_ang_offset(
    int player, void* script_args, float angle_offset);
void mks_bgnd_cam_offset_away(
    void* script_args, float distance, float height);
void mks_bgnd_pfx_bind_to_sobj(
    const char* effect_name, unsigned int sobj_id);
void collision_result_dont_care(void);
void clear_collision_result(void);
void online_combo_adjust(float* horizontal, float* vertical);
void enable_no_sync_anim_f(int enabled);
void enable_no_adjustment_f(int enabled);
void kill_plyr_life(int player);

#endif
