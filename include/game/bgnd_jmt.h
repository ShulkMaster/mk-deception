#ifndef GAME_BGND_JMT_H
#define GAME_BGND_JMT_H

typedef struct MkProc MkProc;

typedef struct RopeProcLatch {
    MkProc* proc;
    unsigned int instance;
} RopeProcLatch;

extern RopeProcLatch rope_proc_item;
extern RopeProcLatch sobj_ctrl_proc_item;

void bgnd_preload_obj_attach_rope(int model_index);
void bgnd_attach_rope_to_bgnd_obj(int rope_model_index, int target_model_index, int object_id);
void mks_blend_start_update_by_group(int group_id, int blend_ticks);
void start_cliff_watcher(float wait_ticks);
void mks_removehide_by_group(int group_id, int remove_hide);
void clear_cliff_data(void);
void set_cliff_watcher_round(int round);
void mks_set_update_delay(int ticks, int random_ticks);
void start_rope_proc(void);
void destroy_sobj_ctrl_proc(void);
void mks_shadow_scale(int group_id, int blend_ticks, float start_scale, float end_scale);
void bgnd_detach_rope(int model_index);
void start_shadow_watcher(void);
int get_cliff_watcher_round(void);
void bgnd_obj_insert_obj_ctrl_section(int model_index, int section);
int get_exec_tick_ctr(void);
void start_sobj_ctrl_proc(void);
void mks_set_rotate_update_by_group(int, int, float, float, float, int);
void bgnd_insert_obj_ctrl_section(int object_id, int section);
int can_fallingcliff_fall(void);
void mks_set_sin_update_by_group(int group_id, int blend_ticks, int update_flags, float start_value, float end_value, float start_speed, float speed_param, float sin_rate, float sin_phase, int extra_flags);
struct BgndDamageState* get_cliff_data(void);
void mks_gravity_update_by_group(int group_id, int blend_ticks, float velocity_x, float velocity_y, float velocity_z, float gravity);
int check_damage_valid_fc(int player, float amount);
void mks_away_vel_update_by_group(int group_id, int blend_ticks, float speed, float speed_param, float random_range);
#endif
