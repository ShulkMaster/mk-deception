#ifndef GAME_LADDER_H
#define GAME_LADDER_H

const char* ladder_koin_type_to_string(int type);
int get_ladder_position(void);
void init_current_ladder_char(void);
int ladder_get_current_bgnd(void);
void one_player_ladder_init(void);

#endif
