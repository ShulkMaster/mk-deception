#include "runtime/anims.h"

#include "runtime/mk_fileinfo.h"

void unload_section_slot(int handle);
int add_anim_section_async(int handle, MkFileInfo* section, void* destination,
                           int load_param, int async);
void wait_for_slot_load(int handle);
void add_anim_section_by_name_async_pal(int handle, const char* section_name,
                                        void* destination, int load_param, int async);

extern MkFileEntry puzzlefighter_file_table[49];
extern MkFileEntry misc_anims_list_file_table[5];
extern MkFileInfo sec_pz_shared_anims;
extern MkFileInfo sec_reduced_shared_anims;
extern MkFileInfo sec_hand_anims;
extern MkFileInfo sec_shared_anims;
extern char pz_shared_ani[];
extern char shared_ani[];
extern char bgnd_animations[];
extern int mode_of_play;

void load_pz_shared_anims(void) {
    int handle_group;
    MkFileInfo* section;
    void* destination;
    int handle;

    load_ssf(puzzlefighter_file_table);
    handle_group = 7;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x51;
    unload_section_slot(handle);
    section = &sec_pz_shared_anims;
    destination = pz_shared_ani;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x51;
    add_anim_section_async(handle, section, destination, 0, 1);
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x51;
    wait_for_slot_load(handle);
}

void load_reduced_shared_and_hand_anims(void) {
    int handle_group;
    MkFileInfo* section;
    void* destination;
    int handle;

    load_ssf(puzzlefighter_file_table);
    handle_group = 7;
    section = &sec_reduced_shared_anims;
    destination = shared_ani;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x3A;
    add_anim_section_async(handle, section, destination, 0, 1);
    load_ssf(misc_anims_list_file_table);
    section = &sec_hand_anims;
    destination = shared_ani + 0x380;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x3A;
    add_anim_section_async(handle, section, destination, 0, 1);
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 0x3A;
    wait_for_slot_load(handle);
}

void load_background_anims(const char* name, unsigned int bgnd_id) {
    int handle;

    if (mode_of_play == 9 || mode_of_play == 10) {
        handle = 0x8005D;
    } else if (mode_of_play == 11) {
        handle = 0x140064;
    } else {
        handle = 0x2001E;
    }
    if (bgnd_id == 0x16) {
        handle = 0x18006D;
    }
    add_anim_section_by_name_async_pal(handle, name, bgnd_animations, 0, 1);
    wait_for_slot_load(handle);
}

void load_shared_and_hand_anims(void) {
    int handle_group;
    MkFileInfo* section;
    void* destination;
    int handle;

    load_ssf(misc_anims_list_file_table);
    handle_group = 0xF;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 6;
    unload_section_slot(handle);
    section = &sec_shared_anims;
    destination = shared_ani;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 6;
    add_anim_section_async(handle, section, destination, 0, 1);
    section = &sec_hand_anims;
    destination = shared_ani + 0x380;
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 6;
    add_anim_section_async(handle, section, destination, 0, 1);
    handle = handle_group;
    handle = handle << 16;
    handle = handle + 6;
    wait_for_slot_load(handle);
}
