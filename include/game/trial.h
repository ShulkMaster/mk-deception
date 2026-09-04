#ifndef GAME_TRIAL_H
#define GAME_TRIAL_H

typedef struct PlyrInfo PlyrInfo;

int current_player_is_drone(void);
int get_konquest_drone_switch_state(int player);
void trial_register_special_move(unsigned int action);
void trial_register_script_function(unsigned int function_index);
void trial_register_attack(
    int player, unsigned char attack_type, unsigned char attack_value);
void trial_clear_provision(void);
void trial_increment_state_value(
    int player, int state_index, int opponent_event);
PlyrInfo* trial_get_drone_info(void);

#endif
