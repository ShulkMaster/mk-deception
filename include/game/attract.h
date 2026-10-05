#ifndef GAME_ATTRACT_H
#define GAME_ATTRACT_H

#include "runtime/fonts.h"
#include "runtime/mk_proc.h"

extern MkProcEntryFn atm_list[];

StringObj* put_bio_text(int unlock_bit, int use_alt);
void atm_reset_current_page(int page);
float p_atm_start_button(void);
float p_atm_loop(void);
float p_attract_mode(void);

#endif
