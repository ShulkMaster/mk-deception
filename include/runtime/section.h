#ifndef RUNTIME_SECTION_H
#define RUNTIME_SECTION_H

#include "runtime/plyr_pdata.h"
#include "runtime/section_types.h"

#define SECTION_MEMORY_SCHEME_ATTRACT 9
#define SEC_SLOT_HANDLE_ATTRACT_LEGAL 0x90046

int load_systemart_phase_2(void);
int load_systemart_phase_1(void);
void load_art_section_by_name(int handle, const char* name);
void load_art_section_by_name_async(int handle, const char* name);
int get_shared_art_section_for_plyr_pdata(PlyrPdata* pdata);
int get_shared_art_section_for_player(SharedArtPlayer* player);
void add_art_section(int handle, MkFileInfo* info);
void load_art_section_language(int handle, MkFileInfo* info);
void load_art_section(int handle, MkFileInfo* info);
void load_art_section_async_language(int handle, MkFileInfo* info);
void add_anim_section_by_name_async_pal(int handle, const char* name,
                                        int* palette_table, int allow_duplicate,
                                        int clear_palette);
void add_anim_section_async_pal(int handle, MkFileInfo* info,
                                int* palette_table, int allow_duplicate,
                                int clear_palette);
void add_anim_section_by_name_async(int handle, const char* name,
                                    int* palette_table, int allow_duplicate,
                                    int clear_palette);
int add_anim_section_async(int handle, MkFileInfo* info, int* palette_table,
                           int allow_duplicate, int clear_palette);
void wait_for_slot_load(int handle);
int load_art_section_async(int handle, MkFileInfo* info);
void add_art_section_by_name_async_language(int handle, const char* name);
void add_art_section_by_name_async(int handle, const char* name);
int add_art_section_async(int handle, MkFileInfo* info);
void unload_section_slot_file(int handle, int file_index);
void unload_section_slot(int handle);
int is_section_loading_or_loaded(int handle, MkFileInfo* info);
SecSlotFileEntry* get_nth_sec_slot_file_from_handle(int handle, int index);
int get_slot_file_count(int handle);
void init_section_system(void);
int get_current_section_memory_scheme(void);
void set_section_memory_scheme(int scheme);

#endif
