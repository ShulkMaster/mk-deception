#ifndef ACB_H
#define ACB_H

void* movelist_get_character_name(void);
void* movelist_get_counter(void);
void movelist_change_move(int delta);
void movelist_change_style(int delta);
void* get_movelist_strings(int* out_max);
void start_movelist(void);

#endif
