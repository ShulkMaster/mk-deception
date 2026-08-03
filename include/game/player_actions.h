#ifndef GAME_PLAYER_ACTIONS_H
#define GAME_PLAYER_ACTIONS_H

#include "runtime/plyr_anim_pdata.h"
#include "runtime/plyr_pdata.h"

typedef struct MkProc MkProc;

typedef struct JoySharedAnimations {
    unsigned char pad00[0x2E4];
    AniData* duck_block_animation;
} JoySharedAnimations;

void jump_away_opponent(void);
void jump_towards_opponent(void);
float j_exit_blend_stance(void);
float j_exit(void);
void blend_to_ani(AniData* animation, int flags, float blend_time);
void set_my_state(int state);
void init_ground_move(PlyrPdata* pdata, PlyrAnimPdata* anim_pdata);
void back_to_normal(void);
void rotate_towards_him(float blend_time);
void head_tracking_on(void);
int check_for_dead_movement(void);
void drone_ai_finished_request(void);
int am_i_blocking(void);
float j_duck_block_loop(void);
float x_block(void);
float drone_start(void);
void disable_this_move_exec(int move, int duration);
void enable_this_move_exec(int move);
void end_of_round_check(void);
void angle_jump_scan_after_move(void);
float get_my_angle_y_error(void);
int am_i_flipped_or_turned(void);
void blend_to_stance(float blend_time);
float step_backward(void);
float step_forward(void);
float step_left(void);
float step_right(void);
void p_animate(void);

extern PlyrPdata* his_pdata;
extern MkProc* plyr_anim_proc;
extern int round_winner;
extern int f_fatality_available;
extern int my_next_duck_state;
extern int g_drone_blocking_in_reaction;
extern unsigned char shared_ani[];

#endif
