#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "runtime/mk_struct.h"

struct JoinInPdata {
    MkHdr hdr;
    int player;
};

#ifdef __cplusplus
extern "C" {
#endif
float p_gamelogic(void);
void display_load_meter(int section_slot);
#ifdef __cplusplus
}
#endif

int is_timer_off(void);

void init_bet_info_struct(void);
void init_game_info_struct(void);
extern int force_bgnd_num;

int ok_to_join_in(void);
float do_join_in(void);

#endif
