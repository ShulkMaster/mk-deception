#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "mwScreenEngine/TextureCollection.h"
#include "runtime/mk_fileinfo.h"

/*
 * menu.o - NonMatching Midway menu (boot -> MAIN_MENU).
 *
 * Retail: Attract PRESS START exits via gamelogic_jump(6, p_main_menu).
 * Screen load name (retail @stringBase0+0x4D1):
 *   "common/main_menu/m_mode_select"
 *
 * Retail MUST RUN (p_main_menu / mode-select idle):
 *   set_section_memory_scheme(4)
 *   load_ssf(msel_art_file_table)          -- mk_fileinfo: "msel_art.ssf"
 *   add_art_section_by_name_async(0x90046, portrait_list[i].sec_name)
 *   get_modeselect_portrait_list -> load_named_tga / alpha ("MSEL_PORTRAIT")
 *   preload_screen_data("common/main_menu/m_mode_select", 0x90046)
 *   load_screen("common/main_menu/m_mode_select", 0x90046, 0, 0)
 *     -- LoadScreenSet opens screen_engine.ssf / scr_<leaf>.sec (see Screen.h)
 *   turn_camera_on()
 *   idle while target_game_mode == 0x18 (sentinel); sleep via mkproc
 *   Frame draw: render_2d_objs(0) via display / image / pfx2d -> ScreenMgr::Render
 *   Nav: ScreenEngine ctrl procs + pad edges -> FireEvent / focus /
 *     target_game_mode (confirm leave-menu)
 * Soft ceiling: pause / controller-config / soundtrack / profile UI deferred;
 *   empty bodies = unfinished decomp, not alternate semantics.
 *
 * See: docs/campaigns/index.md (B18d/B19),
 *      include/mw/mwScreenEngineGlue.h, include/runtime/asset.h.
 */

void adjust_screen_reset(void);
void adjust_screen_position(int direction);
int get_color_red_value(void);
int get_color_green_value(void);
int get_color_blue_value(void);
int get_gamma_value(void);
int get_widescreen_state(void);
int get_progressive_scan_state(void);
int get_contrast_value(void);
int get_brightness_value(void);
void push_video_settings(void);
void reset_video_defaults(void);
void adjust_brightness(int delta);

void play_current_soundtrack(void);
const char* get_current_soundtrack_composer(void);
const char* get_current_soundtrack_description(void);
const char* get_current_soundtrack_title(void);
int get_current_soundtrack(void);
void set_current_soundtrack(int index);
void get_soundtrack_title_list(const char*** titles_out, int* count_out, int* stride_out);

const char* get_screens_online_options_newaccountpassword(void);
const char* get_screens_online_options_newaccountname(void);
const char* get_p2_player_name(void);
const char* get_p1_player_name(void);

typedef struct ModeSelectPortrait {
    const char* sec_name;
    unsigned int flags;
} ModeSelectPortrait;

void get_modeselect_portrait_list(GVTexturePair out);
int get_num_modeselect_portraits(void);
void set_menu_mode(int mode);

void controller_setup_save_to_profile(int player, int save);
void cconfig_get_button_textures(RwTexture*** textures_out);
int controller_get_texture_index_for_button(int player, int button);
int controller_get_player_last_button(int player);
void cconfig_assign_button(int player, int button);
void cconfig_set_current_cell(int player, int cell);
int controller_get_num_adjustable_buttons(void);
void controller_setup_p2_state(int enabled);
void controller_setup_p1_state(int enabled);
int get_pause_menu_ssh(void);

float p_main_menu(void);
void menu_init(void);

#endif
