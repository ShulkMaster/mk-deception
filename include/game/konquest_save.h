#ifndef KONQUEST_SAVE_H
#define KONQUEST_SAVE_H

extern int f_writing_konquest_profile;

void profile_region_change(void);
int full_konquest_save_to_memcard(int region, int slot, int arg);
void full_konquest_load_from_memcard(void);
int load_krd_buffer_from_memcard(int player, int arg);
int load_konq_memory_from_krd_buffer(void);
int save_konq_memory_to_krd_buffer(int region);
void save_konq_common_data_to_buffer(void);
int validate_region_buffer(int region);
void clear_region_buffer(void);

#endif
