#ifndef GAME_AI_H
#define GAME_AI_H

typedef struct PlyrPdata PlyrPdata;
typedef struct GlobalMoveset PlyrWeaponStyle;

void drone_ai_finished_request(void);
float drone_start(void);
void cleanup_drone_ai(void);
int drone_ai_check_switching_to(int command);
void generate_ai_table_player(PlyrPdata* player);
void generate_ai_table_moveset(PlyrWeaponStyle* moveset);

void got_hit_fx(int type, int region, int arg2, int arg3, int arg4, float scale, int arg5);
void set_attackers_attack_region(int region);
float go_into_major_pain(void);
void random_dk_foot(void);
int drone_ai_should_roll(int mode);
float go_into_twitch_death(void);
void whoosh_fx(int sound);
void setDroneOverrideSwitch(int activated, struct DroneOverrideInfo* info);
extern int go_into_major_pain_please;
extern int go_into_twitch_death_please;
extern float drone_ai_watcher(void);
int drone_ai_check_button_direction(int direction);
float drone_entry(void);
int drone_ai_check_button_press(int button);
void drone_ai_increase_big_boss_stage(PlyrPdata* victim);
extern float drone_blocking_done(void);
int drone_super_combo_refresh(void);
void drone_ai_reset_all(void);
void advance_cur_cmd_idx(void);
void liukang_in_fight_random_snd_check(void);
int drone_ai_check_block_fakeout(void);
void force_ai_style(int style);
int drone_ai_should_ermac_fly_kick(void);
int drone_ai_should_ermac_ground_slam(void);
void dk_taunt_at_screen(void);
extern float getup_from_ground(void);
float drone_ai_get_big_boss_damage_scale(PlyrPdata* player);
int drone_ai_check_block_at_reactions(void);
int drone_ai_check_combo_breaker();
unsigned int big_boss_reaction_remap(unsigned int reaction);
extern int g_big_boss_intro_tap_out_f;
extern float big_boss_taunt_cam_cut(void);
void drone_ai_reset_ai_cmd();
void drone_ai_ok_to_think(void);
void drone_ai_hit();
void drone_ai_clear_avoidance_area_duration(int player);
int can_big_boss_make_special_vo_call(unsigned int cooldown_ticks);
extern float give_some_distance(void);
void blocked_fx(int type, int bone, int third, int fourth, int fifth);
extern void big_boss_wait_for_intro(void);
void drone_ai_dont_think(void);
#endif
