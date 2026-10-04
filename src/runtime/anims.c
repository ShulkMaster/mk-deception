#include "runtime/anims.h"

#include "runtime/mk_fileinfo.h"
#include "runtime/section.h"
#include "platform/main.h"

extern char pz_shared_ani[];
extern char shared_ani[];
extern char bgnd_animations[];

void load_pz_shared_anims(void) {

    load_ssf(puzzlefighter_file_table);
    unload_section_slot(0x70051);
    add_anim_section_async(0x70051, &sec_pz_shared_anims, (int*)pz_shared_ani, 0, 1);
    wait_for_slot_load(0x70051);
}

void load_reduced_shared_and_hand_anims(void) {

    load_ssf(puzzlefighter_file_table);
    add_anim_section_async(0x7003A, &sec_reduced_shared_anims, (int*)shared_ani, 0, 1);
    load_ssf(misc_anims_list_file_table);
    add_anim_section_async(0x7003A, &sec_hand_anims, (int*)(shared_ani + 0x380), 0, 1);
    wait_for_slot_load(0x7003A);
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
    add_anim_section_by_name_async_pal(handle, name, (int*)bgnd_animations, 0, 1);
    wait_for_slot_load(handle);
}

void load_shared_and_hand_anims(void) {

    load_ssf(misc_anims_list_file_table);
    unload_section_slot(0xF0006);
    add_anim_section_async(0xF0006, &sec_shared_anims, (int*)shared_ani, 0, 1);
    add_anim_section_async(0xF0006, &sec_hand_anims, (int*)(shared_ani + 0x380), 0, 1);
    wait_for_slot_load(0xF0006);
}
