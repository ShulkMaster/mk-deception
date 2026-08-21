#ifndef PLATFORM_MAIN_H
#define PLATFORM_MAIN_H

/*
 * main.o - boot entry, exec loop, game-speed helpers.
 * Retail order: gamelogic_jump, main, reset_game_speed,
 * get_inverse_game_speed, get_game_speed, set_game_speed.
 *
 */

#include "platform/main_jump.h"

int main(void);
void reset_game_speed(void);
float get_inverse_game_speed(void);
float get_game_speed(void);
void set_game_speed(float speed);

extern int jump_target_mode;
extern int gameart_is_loaded;
extern float sqrt_game_speed;
extern float inverse_game_speed;
extern float game_speed;
extern void* empty_pdata;
extern void* mab_generic_pdata;
extern int jmp_where_id;
extern int mode_of_play;
extern float msecs_per_tick;
extern int game_tick_ctr;
extern int exec_tick_ctr;

#endif
