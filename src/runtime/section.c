#include "runtime/section.h"
#include "runtime/cstring.h"

#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "platform/gcutils.h"
#include "runtime/asset.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
#include "runtime/section_slot_file.h"
#include "runtime/utils.h"

extern _mwMemHeap* SystemSwappableHeap;
extern SectionSlotDef* section_memory_maps[];
extern char* strstr(const char* string, const char* substring);
extern void load_string_bank(int bank, const char* name);
extern void load_string_bank_async(int bank, const char* name);

static SecSysState sec_sys_state;
MkProc* saved_aproc;

static SecSlot* get_sec_slot_from_handle(int handle);
static void free_all_slot_groups_after_pos(int position);
static void free_all_slots_in_group(SecSlotGroup* group);

/* The async file API carries the retail section type through its userdata slot. */
#define SEC_FILE_USERDATA(type) ((void*)(type))

static void append_slot_file(SecSlot* slot, SecSlotFileEntry* file) {
    SecSlotFileEntry* tail = slot->files;

    while (tail != 0 && tail->next != 0) {
        tail = tail->next;
    }
    if (tail != 0) {
        file->buffer = tail->size_or_flag == 0
                           ? 0
                           : tail->buffer + tail->size_or_flag;
        tail->next = file;
    } else {
        file->buffer = slot->base;
        slot->files = file;
    }
    slot->file_count++;
}

int load_systemart_phase_2(void) {
    load_ssf(sec_sysart.table);
    load_art_section_language(0, &sec_sysart);
    load_string_bank(0x10000, "permanent_strings_eng.mko");
    return 1;
}

int load_systemart_phase_1(void) {
    load_ssf(sec_sysart.table);
    load_art_section_async_language(0, &sec_sysart);
    load_string_bank_async(0x10000, "permanent_strings_eng.mko");
    return 1;
}

void load_art_section_by_name(int handle, const char* name) {
    get_sec_slot_from_handle(handle);
    {
        MkFileInfo* info = find_section_by_name(name);
        if (info != 0) {
            load_art_section(handle, info);
        }
    }
}

void load_art_section_by_name_async(int handle, const char* name) {
    get_sec_slot_from_handle(handle);
    {
        MkFileInfo* info = find_section_by_name(name);
        if (info != 0) {
            load_art_section_async(handle, info);
        }
    }
}

int get_shared_art_section_for_plyr_pdata(PlyrPdata* pdata) {
    if (pdata->plyr_num == 0) {
        return 0x3000B;
    }
    if (pdata->plyr_num == 1) {
        return 0x4000B;
    }
    return -1;
}

int get_shared_art_section_for_player(SharedArtPlayer* player) {
    if (player->type == 0x1001) {
        return 0x3000B;
    }
    if (player->type == 0x1002) {
        return 0x4000B;
    }
    return -1;
}

void add_art_section(int handle, MkFileInfo* info) {
    get_sec_slot_from_handle(handle);
    add_art_section_async(handle, info);
    wait_for_slot_load(handle);
}

void load_art_section_language(int handle, MkFileInfo* info) {
    int language = get_language();
    info = offset_mk_file_info(info, language);
    load_art_section(handle, info);
}

void load_art_section(int handle, MkFileInfo* info) {
    get_sec_slot_from_handle(handle);
    load_art_section_async(handle, info);
    wait_for_slot_load(handle);
}

void load_art_section_async_language(int handle, MkFileInfo* info) {
    int language = get_language();
    info = offset_mk_file_info(info, language);
    load_art_section_async(handle, info);
}

static MkFileInfo* select_pal_animation(MkFileInfo* info) {
    if (refresh_rate() == 50) {
        MkFileInfo* pal_info = 0;
        unsigned int file_count = num_files_in_ssf(get_current_ssf_file());
        if ((unsigned int)(get_ssf_dir_index(info) + 1) < file_count) {
            pal_info = offset_mk_file_info(info, 1);
        }
        if (pal_info != 0 && strstr(pal_info->name, "_50") != 0) {
            info = pal_info;
        }
    }
    return info;
}

void add_anim_section_by_name_async_pal(int handle, const char* name,
                                        int* palette_table, int allow_duplicate,
                                        int clear_palette) {
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        add_anim_section_async(handle, select_pal_animation(info), palette_table,
                               allow_duplicate, clear_palette);
    }
}

void add_anim_section_async_pal(int handle, MkFileInfo* info,
                                int* palette_table, int allow_duplicate,
                                int clear_palette) {
    add_anim_section_async(handle, select_pal_animation(info), palette_table,
                           allow_duplicate, clear_palette);
}

void add_anim_section_by_name_async(int handle, const char* name,
                                    int* palette_table, int allow_duplicate,
                                    int clear_palette) {
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        add_anim_section_async(handle, info, palette_table, allow_duplicate,
                               clear_palette);
    }
}

int add_anim_section_async(int handle, MkFileInfo* info, int* palette_table,
                           int allow_duplicate, int clear_palette) {
    SecSlot* slot;
    SecSlotFileEntry* file;

    if (!allow_duplicate) {
        int loaded = is_section_loading_or_loaded(handle, info);
        if (loaded != 0) {
            return loaded;
        }
    }
    slot = get_sec_slot_from_handle(handle);
    file = _mwMemMalloc(section_table_heap, sizeof(*file), 3, 0, 0, 0);
    memset(file, 0, sizeof(*file));
    append_slot_file(slot, file);
    if (clear_palette) {
        file->flags |= 0x80;
    }
    file->palette_table = palette_table;
    sec_slot_file_open_read_async(file, slot, handle, info,
                                  SEC_FILE_USERDATA(SEC_FILE_TYPE_ANIM));
    return slot->file_count;
}

void wait_for_slot_load(int handle) {
    SecSlotFileEntry* file = get_sec_slot_from_handle(handle)->files;
    if (file == 0) {
        return;
    }
    do {
        if (file != 0) {
            if (file->load_state == 0) {
                sec_slot_file_wait_for_load(file);
                if (file->load_state == 0) {
                    if (file->section_info->type == SEC_FILE_TYPE_ANIM) {
                        process_anim_section_data(file);
                    } else if (file->section_info->type == SEC_FILE_TYPE_ART) {
                        process_art_section_data(file);
                    }
                }
            }
            file = file->next;
        }
    } while (file != 0);
}

int load_art_section_async(int handle, MkFileInfo* info) {
    SecSlot* slot = get_sec_slot_from_handle(handle);
    SecSlotFileEntry* file;

    if (slot->files != 0) {
        if (slot->files->section_info == info && slot->files->next == 0) {
            return 1;
        }
        unload_section_slot(handle);
    }
    file = _mwMemMalloc(section_table_heap, sizeof(*file), 3, 0, 0, 0);
    memset(file, 0, sizeof(*file));
    append_slot_file(slot, file);
    sec_slot_file_open_read_async(file, slot, handle, info,
                                  SEC_FILE_USERDATA(SEC_FILE_TYPE_ART));
    return 1;
}

void add_art_section_by_name_async_language(int handle, const char* name) {
    int language = get_language();
    MkFileInfo* info = find_section_by_name(name);
    add_art_section_async(handle, offset_mk_file_info(info, language));
}

void add_art_section_by_name_async(int handle, const char* name) {
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        add_art_section_async(handle, info);
    }
}

int add_art_section_async(int handle, MkFileInfo* info) {
    SecSlot* slot;
    SecSlotFileEntry* file;
    int loaded = is_section_loading_or_loaded(handle, info);
    if (loaded != 0) {
        return loaded;
    }
    slot = get_sec_slot_from_handle(handle);
    file = _mwMemMalloc(section_table_heap, sizeof(*file), 3, 0, 0, 0);
    memset(file, 0, sizeof(*file));
    append_slot_file(slot, file);
    sec_slot_file_open_read_async(file, slot, handle, info,
                                  SEC_FILE_USERDATA(SEC_FILE_TYPE_ART));
    return slot->file_count;
}

static void release_slot_file_data(SecSlotFileEntry* file) {
    if (file->load_state != 0) {
        if (file->section_info->type == SEC_FILE_TYPE_ART) {
            annihilate_art_section_data(file);
        } else if ((file->section_info->type == SEC_FILE_TYPE_ANIM) &
                   ((file->flags & 0x80) != 0)) {
            int index;
            int* palette = file->palette_table;
            for (index = 0; index < file->member_count; index++) {
                *palette = 0;
                palette++;
            }
        }
        file->load_state = 0;
    } else {
        sec_slot_file_cancel_async(file);
    }
    file->section_info = 0;
    file->member_count = 0;
    file->section_id = 0;
}

void unload_section_slot_file(int handle, int file_index) {
    SecSlot* slot;
    SecSlotFileEntry* file;
    SecSlotFileEntry* previous;

    if (file_index == 1) {
        unload_section_slot(handle);
        return;
    }
    slot = get_sec_slot_from_handle(handle);
    file = get_nth_sec_slot_file_from_handle(handle, file_index);
    previous = get_nth_sec_slot_file_from_handle(handle, 1);
    while (previous->next != file) {
        previous = previous->next;
    }
    release_slot_file_data(file);
    _mwMemFree(file, 0, 0);
    previous->next = 0;
    slot->file_count--;
}

void unload_section_slot(int handle) {
    SecSlot* slot = get_sec_slot_from_handle(handle);
    SecSlotFileEntry* file = slot->files;
    SecSlotFileEntry* next;

    while (file != 0) {
        release_slot_file_data(file);
        file = file->next;
        slot->file_count--;
    }
    file = slot->files;
    while (file != 0) {
        next = file->next;
        _mwMemFree(file, 0, 0);
        file = next;
    }
    slot->files = 0;
    slot->file_count = 0;
}

int is_section_loading_or_loaded(int handle, MkFileInfo* info) {
    int index = 1;
    SecSlotFileEntry* file = get_sec_slot_from_handle(handle)->files;
    while (file != 0) {
        if (file->section_info == info) {
            return index;
        }
        file = file->next;
        index++;
    }
    return 0;
}

SecSlotFileEntry* get_nth_sec_slot_file_from_handle(int handle, int index) {
    SecSlot* slot = get_sec_slot_from_handle(handle);
    SecSlotFileEntry* file = slot->files;
    int remaining;
    if (index < 1 || index > slot->file_count || file == 0) {
        return 0;
    }
    if (index > 1) {
        for (remaining = 1; remaining < index; remaining++) {
            file = file->next;
            if (file == 0) {
                return 0;
            }
        }
    }
    return file;
}

int get_slot_file_count(int handle) {
    return get_sec_slot_from_handle(handle)->file_count;
}

static SecSlot* get_sec_slot_from_handle(int handle) {
    unsigned short slot_id;
    int group_id;
    SecSlotGroup* group = sec_sys_state.group_list;
    SecSlot* slot;
    unsigned int remaining;

    group_id = handle >> 16;
    slot_id = (unsigned short)handle;

    while (group != 0) {
        if (group->group_id == group_id) {
            break;
        }
        group = group->next;
    }
    if (group == 0) {
        group = 0;
    }
    remaining = group->slot_count;
    slot = group->slots;
    while (remaining != 0) {
        if (slot->slot_id == slot_id) {
            return slot;
        }
        slot++;
        remaining--;
    }
    return 0;
}

void init_section_system(void) {
    MwMemHeapInfo info;
    memset(&sec_sys_state, 0, sizeof(sec_sys_state));
    mwMemHeapGetInfo(section_heap, &info);
    sec_sys_state.total_memory = info.arenaSize;
    set_section_memory_scheme(0);
    init_ssf_system();
    init_sec_slot_files();
}

int get_current_section_memory_scheme(void) {
    return sec_sys_state.current_map - section_memory_maps;
}

void set_section_memory_scheme(int scheme) {
    SectionSlotDef** new_map = &section_memory_maps[scheme];
    int common_position;
    int group_count;
    unsigned int required_memory;
    SectionSlotDef* definition;
    int group_index;

    if (sec_sys_state.current_map == new_map) {
        return;
    }
    common_position = -1;
    if (sec_sys_state.current_map != 0 && new_map != 0) {
        SectionSlotDef* old_def = *sec_sys_state.current_map;
        SectionSlotDef* new_def = *new_map;
        while (old_def->group_id != -1 && new_def->group_id != -1) {
            int definitions_match;
            if (old_def->group_id == new_def->group_id &&
                old_def->group_buffer_size == new_def->group_buffer_size &&
                old_def->per_slot_defs == new_def->per_slot_defs) {
                definitions_match = 1;
            } else {
                definitions_match = 0;
            }
            if (!definitions_match) {
                break;
            }
            common_position++;
            old_def++;
            new_def++;
        }
    }
    if (common_position >= 0) {
        free_all_slot_groups_after_pos(common_position);
    }
    sec_sys_state.current_map = new_map;
    group_count = 0;
    required_memory = 0;
    definition = *new_map;
    while (definition->group_id != -1) {
        group_count++;
        required_memory += definition->group_buffer_size;
        definition++;
    }
    if (required_memory > (unsigned int)sec_sys_state.total_memory) {
        return;
    }
    sec_sys_state.group_count = group_count;
    definition = *new_map;
    for (group_index = 0; definition->group_id != -1;
         group_index++, definition++) {
        SecSlotGroup* group;
        SectionPerSlotDef* per_slot;
        unsigned char* buffer_position;
        SecSlot* slot;
        int slot_count;
        int slot_index;

        if (group_index <= common_position) {
            continue;
        }
        group = _mwMemMalloc(section_table_heap, sizeof(*group), 3, 0, 0, 0);
        memset(group, 0, sizeof(*group));
        group->next = sec_sys_state.group_list;
        group->buffer = _mwMemMalloc(section_heap, definition->group_buffer_size,
                                     7, 0, 0, 0);
        group->buffer_size = definition->group_buffer_size;
        group->group_id = definition->group_id;
        group->map_index = group_index;
        buffer_position = group->buffer;
        slot_count = 0;
        for (per_slot = definition->per_slot_defs; per_slot->slot_index != -1;
             per_slot++) {
            slot_count++;
        }
        group->slot_count = slot_count;
        group->slots = _mwMemMalloc(section_table_heap,
                                    slot_count * sizeof(*group->slots), 3, 0, 0, 0);
        per_slot = definition->per_slot_defs;
        slot = group->slots;
        for (slot_index = 0; slot_index < slot_count;
             slot_index++, slot++, per_slot++) {
            unsigned int allocation_size;
            slot->slot_id = per_slot->slot_index;
            slot->buffer_size = per_slot->buffer_size;
            allocation_size = (unsigned int)per_slot->buffer_size & 0x7FFFFFFF;
            if ((unsigned int)per_slot->buffer_size != allocation_size) {
                slot->base = _mwMemMalloc(SystemSwappableHeap, allocation_size,
                                          7, 0, 0, 0);
            } else {
                slot->base = buffer_position;
                buffer_position += allocation_size;
            }
            slot->files = 0;
            slot->file_count = 0;
        }
        sec_sys_state.group_list = group;
    }
}

static void free_all_slot_groups_after_pos(int position) {
    SecSlotGroup* group = sec_sys_state.group_list;
    while (group != 0) {
        SecSlotGroup* next = group->next;
        if (group->map_index > position) {
            free_all_slots_in_group(group);
            _mwMemFree(group->buffer, 0, 0);
            _mwMemFree(group, 0, 0);
            sec_sys_state.group_list = next;
        }
        group = next;
    }
}

static void free_all_slots_in_group(SecSlotGroup* group) {
    int slot_index;
    for (slot_index = 0; slot_index < group->slot_count; slot_index++) {
        SecSlot* slot = &group->slots[slot_index];
        SecSlotFileEntry* file = slot->files;
        SecSlotFileEntry* next;
        while (file != 0) {
            release_slot_file_data(file);
            file = file->next;
            slot->file_count--;
        }
        file = slot->files;
        while (file != 0) {
            next = file->next;
            _mwMemFree(file, 0, 0);
            file = next;
        }
        slot->files = 0;
        slot->file_count = 0;
        if ((slot->buffer_size & 0x80000000U) != 0) {
            _mwMemFree(slot->base, 0, 0);
        }
    }
    _mwMemFree(group->slots, 0, 0);
    group->slots = 0;
}
