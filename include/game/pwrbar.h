#ifndef GAME_PWRBAR_H
#define GAME_PWRBAR_H

typedef struct PlyrInfo PlyrInfo;

#ifdef __cplusplus
extern "C" {
#endif

void show_wins_in_a_row(void);
int check_for_red_light(PlyrInfo* player);
int check_for_green_light(PlyrInfo* player);
void init_fighting_state_lights(void);
void retract_power_bars(void);
void extend_powerbars(void);
float p_move_pbars_off_screen(void);
int adjust_p2_life(float amount);
int adjust_p1_life(float amount);
int adjust_player_life(int player_index, float amount);
void update_plyr_medals(void);
void destroy_pwr_bars(void);
void pbar_force_pb_setting_with_offset(unsigned int player, float offset);
void start_powerbar_monitor(void);
int are_powerbars_retracted(void);
void init_pwr_bars(void);

#ifdef __cplusplus
}
#endif

#endif
