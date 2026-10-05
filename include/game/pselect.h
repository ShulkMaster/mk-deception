#ifndef GAME_PSELECT_H
#define GAME_PSELECT_H

#include "game/pselect_textures.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RwTexture RwTexture;
typedef struct PlyrInfo PlyrInfo;

#define PSELECT_SEC_SLOT 0x17006A

extern int pselect_mode;

int is_pselect_mode(void);
int is_char_locked(int char_id, int alt_bit);

void send_player_status_msg(void);
void pselect_handicap_update(void);
void pselect_handicap_show(void);
void pselect_bgnd_select_done(void);

int pselect_bgnd_has_weapon(void);
int pselect_bgnd_has_level_transition(void);
int pselect_bgnd_has_deathtrap(void);
int pselect_get_arena_index(void);

int get_num_pselect_body_textures(void);

char* pselect_get_style_name(int player, int style_idx);
char* pselect_get_difficulty_level(int player);
char* pselect_get_arena_name(void);
char* pselect_get_player_name(int player);

void pselect_player_moved(int player);
void bg_pselect_player_canceled(int player);
void pselect_player_canceled(int player);
void resolve_alternate_palettes(PlyrInfo* plyr);
void pselect_player_selected(PlyrInfo* plyr);

int pselect_get_body_texture_index(int player);
int pselect_get_selbox_pos(int player);
void pselect_update_selbox_pos(int player, int new_pos);

int bg_pselect_get_stage(int team);
int bg_pselect_get_offender_class(int team);

float p_bg_pselect(void);
float p_pz_pselect(void);
float p_pselect(void);

#ifdef __cplusplus
}
#endif

#endif
