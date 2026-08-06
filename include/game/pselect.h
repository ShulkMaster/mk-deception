#ifndef GAME_PSELECT_H
#define GAME_PSELECT_H

/*
 * pselect.o - NonMatching character select (B19 Wave B/C + Wave D start).
 *
 * Retail: Entry from MAIN_MENU (src/game/menu.c p_main_menu):
 *   target_game_mode 6/7/11 -> gamelogic_jump(1, p_pselect)
 *   target_game_mode 8      -> gamelogic_jump(..., p_bg_pselect)
 *   target_game_mode 9      -> gamelogic_jump(..., p_pz_pselect)
 *
 * Screen names (retail stringBase0):
 *   "common/p_select/p_select"              -- versus / default
 *   "common/p_select/p_practice"            -- mode_of_play == 4
 *   "common/board_game/bg_player_select"    -- chess / bg
 *   "common/puzzle_fighter/pf_player_select"-- puzzle
 *
 * Art slot handle: 0x17006A (same pattern as menu 0x90046).
 *   load_ssf(pselect_file_table)
 *   load_art_section_by_name / async: pselect_art.sec /
 *     bg_pselect_art.sec / pz_pselect_art.sec
 *
 * Retail MUST RUN (do not skip before load_screen):
 *   pselect_init: set_section_memory_scheme(0xA), set_menu_mode(-1)
 *   load_ssf(pselect_file_table)
 *   load_art_section_* + load_screen(name, 0x17006A, ...)
 *   turn_camera_on()
 *   idle while character select is active (mkproc sleep)
 *   Frame draw: render_2d_objs(0) + Glue screen_engine_render /
 *     p_screen_engine_tick
 *   bg: get_mkpdata_generic(0x50) + load_screen share_pdata
 * Exit / confirm (Wave C):
 *   back: target_game_mode == 5 -> wait_for_screen_close ->
 *     gamelogic_jump(..., p_main_menu) on all three entry procs
 *   confirm: pselect_player_selected sets PlyrInfo.player_index
 *     (no fight jump required for B19)
 * Wave D (portrait / body panel + GetInt/GetString binds):
 *   mkGameVariables::GetInt 0x1FFB/0x1FFC ->
 *     pselect_get_body_texture_index(0/1) indexes body TGA array
 *   GetInt 0x1F94 arena index; 0x1FE7..0x1FE9 bgnd flags;
 *     0x1F76/0x1F77 selbox; bg stage / offender class
 *   GetString style / difficulty / arena / player name leaves
 * Soft ceilings: p_pselect ~84%; p_bg ~86%; p_pz ~93%;
 *   is_pselect_mode ~80%; is_char_locked __shl2i;
 *   init_startup / selbox update / TGA / confirm helpers -- Soft OK.
 *
 * See: docs/campaigns/index.md (B19), include/game/menu.h,
 *      include/mw/mwScreenEngineGlue.h.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RwTexture RwTexture;
typedef struct PlyrInfo PlyrInfo;

/* Section / ScreenEngine slot for pselect art + screens. */
#define PSELECT_SEC_SLOT 0x17006A

/* Out-params for head/body/bgnd TGA loaders (color + alpha arrays). */
typedef struct PselectTexOut {
    RwTexture** color; /* +0x00 */
    RwTexture** alpha; /* +0x04 */
} PselectTexOut;

/* pselect_mode: 0=normal, 1=bg/chess, 2=puzzle. */
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

void get_background_select_textures(PselectTexOut* out);
void get_pselect_body_textures(PselectTexOut* out);
int get_num_pselect_body_textures(void);
void get_bg_pselect_team_textures(PselectTexOut* out, int team);
void get_pselect_head_textures(PselectTexOut* out);

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
