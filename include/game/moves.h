#ifndef GAME_MOVES_H
#define GAME_MOVES_H

float joy_dash_back(void);
float p_block(void);
float p_swap_levels(void);
float switch_proc_attack_4(void);
float switch_proc_attack_3(void);
float switch_proc_attack_1(void);
float switch_proc_attack_5(void);
float switch_proc_advance_moveset(void);
float switch_proc_pickup(void);
float switch_proc_attack_2(void);
int check_for_dead_movement(void);
float do_my_fatality(void);
float do_my_2nd_fatality(void);
void pre_attack_chores(void);
void share_my_attack_info(float duration, float divisor);
int get_fatality_available_flag(void);
float j_duck_block_loop(void);
float jump_away_opponent(void);
float jump_towards_opponent(void);
float step_backward(void);
float step_forward(void);
float step_left(void);
float step_right(void);
float x_block(void);

#endif
