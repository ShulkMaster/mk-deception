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
int check_for_winner(void);
int is_load_meter_active(void);

#ifdef VERSION_GQNE5D
#define MK6_VERSION_STRING "0.142_gc"
#endif

extern const char* mk6_version_string;
extern int b_game_timer_off;
extern int game_save_loop_count;
extern int winner;
extern int f_fatality_finished;
extern int f_fatality_available;
extern int f_fatality_was_done;
extern int round_winner;
extern int g_GameLossesInARow;


void init_bet_info_struct(void);
void init_game_info_struct(void);
extern int force_bgnd_num;

int ok_to_join_in(void);
float do_join_in(void);

void ck_do_profile_save(void);
void move_plyrs_to_round_start(void);
void do_win_effect(void);
void reset_fight(int mode);
void round_init(void);
void destroy_onscreen_fight_2d_objects(void);
float do_continue(void);
int ck_fatality_available(void);
void show_damage_text(int player, int combo_hits, int displayed_damage);
void cleanup_fight(void);
int get_continue_timer(void);
#endif
