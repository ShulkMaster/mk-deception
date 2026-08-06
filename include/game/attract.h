#ifndef GAME_ATTRACT_H
#define GAME_ATTRACT_H

#include "runtime/fonts.h"
#include "runtime/mk_proc.h"

/*
 * attract.o - title/attract mode (legal screen, demo loop, PRESS START gate).
 * Retail emission order preserved in attract.c.
 *
 * Retail: Title gate after intro (atm_list[2]):
 *   atm_mkda_logo
 *     -> play_movie(title) ; on fail atm_old_mkda_logo
 *     -> atm_setup_press_start_flasher (inlined in old-logo path):
 *          _create_mkproc_generic_tinystack(0x2005, ..., p_flash_atm_text)
 *            returns MkProc* in r3 (cast at call site; mk_pdata.h says void)
 *          string_center_xy(0x2010, 0, get_string(1), screen_width/2, 0x41, 0x1D)
 *          (get_string(1) == "PRESS START")
 *     -> p_flash_atm_text toggles press_start_item visibility (~20 ticks)
 *   Start/A (check_start_or_a):
 *     check_switch_edge_any_pad(0xB) || check_switch_edge_any_pad(6)
 *     -> gamelogic_jump(6, p_main_menu)
 * Demo interrupt: p_atm_start_button -> atm_current_page=2 +
 *   gamelogic_jump(0, p_atm_loop) (re-enters logo/PRESS START).
 * Soft ceiling: ATTRACT_PAGE_SETUP andi. vs retail rlwimi; p_flash_atm_text
 *   ~96% live-check; p_atm_loop ~99% page-index; bio screens not title path.
 *
 * Note: GameInfo late fields (field_1F8 / field_210) live at retail +0x1F8 /
 * +0x210 -- pads follow plyr1 with no pad174 gap (see game_info.h).
 */

/* Retail .data atm_list[26]; p_atm_loop indexes atm_list[atm_current_page]. */
extern MkProcEntryFn atm_list[];

/* First arg is bio_file_table[].unlock_bit (+0x0C), not sound_id. */
StringObj* put_bio_text(int unlock_bit, int use_alt);
void atm_reset_current_page(int page);
float p_atm_start_button(void);
float p_atm_loop(void);
float p_attract_mode(void);

#endif
