#ifndef GAME_COMBAT_H
#define GAME_COMBAT_H

#include "game/ejb.h"
#include "game/plyr.h"

int fatality_check_distance(unsigned int action);
int get_fatality_available_flag(void);
void pre_attack_chores(void);
void share_my_attack_info(float scale, float duration);
void do_my_fatality(void);
void do_my_2nd_fatality(void);

#endif
