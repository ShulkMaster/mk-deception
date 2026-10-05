#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "game/controller_types.h"

typedef struct PlyrInfo PlyrInfo;
extern SwitchMapEntry default_switch_map[16];

int find_bit(const SwitchMapEntry* switch_map, unsigned int bit);
const char* get_controller_vibration_string(int player);
PlyrInfo* get_player_for_port(int port);
void set_game_switch_map(PlyrInfo* player);
void set_default_switch_map(PlyrInfo* player);
void set_game_switch_maps(void);
void set_default_switch_maps(void);
void switch_map_unload_player_profile(PlyrInfo* player);
void update_pause_menu_controller_state(void);
int is_controller_removed(void);
void update_cnt_removed_controller_state(void);
void turn_all_rumble_motors_off(void);
void controller_removed(int port);
void ck_for_controller_removed(void);
int are_controllers_locked(void);
int assign_player(int port);
void ck_rumble_controller(int player, int strength, int ticks);
void unassign_player(PlyrInfo* player);
void init_temp_switch_map(int player, int use_profile);
void init_player_switch_maps(void);
int check_for_non_game_locked_controller_state(void);
int is_plyr_controller_enabled(PlyrInfo* player);
void init_port_info_struct(void);
void dispatch_pad_sticks(int port);
void dispatch_right_sticks(int port);

#endif
