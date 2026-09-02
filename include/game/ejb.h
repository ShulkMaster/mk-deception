#ifndef GAME_EJB_H
#define GAME_EJB_H

typedef struct AniData AniData;
typedef struct PlyrPdata PlyrPdata;
typedef struct Vec Vec;

int am_i_blocking(void);
int am_i_flipped_or_turned(void);
void back_to_normal(void);
void blend_to_ani(AniData* animation, int transition, float blend_rate);
int blend_to_stance(float blend_rate);
void disable_this_move_exec(unsigned int move, int ticks);
void enable_this_move_exec(unsigned int move);
float end_of_round_check(void);
float get_my_angle_y_error(void);
void head_tracking_on(void);
void init_ground_move(void);
float j_exit(void);
float j_exit_6(void);
float j_exit_blend_stance(void);
float j_exit_react(void);
void rotate_towards_him(float max_step);
void rotate_towards_position(Vec* target, float max_step);
void set_my_state(int state);
int taunt_increase_life(float scale, float duration);
float which_way_is_towards(void);
int should_i_weapon_block(void);
int is_this_move_disabled_exec(unsigned int action);
void plyr_going_to_attack_with_action(unsigned int action);
float xz_distance_between_players(void);

#endif
