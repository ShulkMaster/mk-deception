#ifndef GAME_COMBAT_H
#define GAME_COMBAT_H

#include "runtime/plyr_pdata.h"

typedef struct ScriptSlot ScriptSlot;

int fatality_check_distance(unsigned int action);
int get_fatality_available_flag(void);
int is_special_move_available(PlyrPdata* player, int move_id);
void pre_attack_chores(void);
void plyr_going_to_attack_with_action(unsigned int action);
void share_my_attack_info(float scale, float duration);
void do_my_fatality(void);
void do_my_2nd_fatality(void);

extern ScriptSlot* reactions_cmo;

#endif
