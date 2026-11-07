#ifndef GAME_LADDER_H
#define GAME_LADDER_H

const char* ladder_koin_type_to_string(int type);
int get_ladder_position(void);
void init_current_ladder_char(void);
int ladder_get_current_bgnd(void);
void one_player_ladder_init(void);

extern float p_ladder_select(void);
extern int advance_ladder_position(void);
int get_chess_leader_won_coin_award(void);
const char* get_rnd_chess_koin_type(int difficulty);
int get_chess_coin_award(int coin_index);
#endif
