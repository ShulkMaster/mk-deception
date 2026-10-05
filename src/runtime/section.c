#include "runtime/section.h"
#include "runtime/cstring.h"

#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "platform/gcutils.h"
#include "platform/gcARam.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_obj.h"
#include "runtime/section_slot_file.h"
#include "runtime/utils.h"


static SecSysState sec_sys_state;
MkProc* saved_aproc;

static SecSlot* get_sec_slot_from_handle(int handle);
static void free_all_slot_groups_after_pos(int position);
static void free_all_slots_in_group(SecSlotGroup* group);

#define SEC_FILE_USERDATA(type) ((void*)(type))

static void append_slot_file(SecSlot* slot, SecSlotFileEntry* file) {
    SecSlotFileEntry* tail = slot->files;

    while (tail != 0 && tail->next != 0) {
        tail = tail->next;
    }
    if (tail != 0) {
        if (tail->size_or_flag == 0U) {
            file->buffer = 0;
        } else {
            file->buffer = tail->buffer + tail->size_or_flag;
        }
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
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        load_art_section(handle, info);
    }
}

void load_art_section_by_name_async(int handle, const char* name) {
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        load_art_section_async(handle, info);
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

int get_shared_art_section_for_player(MkObj* player) {
    if (player->oid == 0x1001) {
        return 0x3000B;
    }
    if (player->oid == 0x1002) {
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

static inline int section_uses_pal_animation(void) {
    return refresh_rate() == 50;
}

void add_anim_section_by_name_async_pal(int handle, const char* name,
                                        int* palette_table, int allow_duplicate,
                                        int clear_palette) {
    MkFileInfo* info;
    get_sec_slot_from_handle(handle);
    info = find_section_by_name(name);
    if (info != 0) {
        if (section_uses_pal_animation() != 0) {
            MkFileInfo* pal_info = 0;
            unsigned int file_count = num_files_in_ssf(get_current_ssf_file());

            if ((unsigned int)(get_ssf_dir_index(info) + 1) < file_count) {
                pal_info = offset_mk_file_info(info, 1);
            }
            if (pal_info != 0 && strstr(pal_info->name, "_50") != 0) {
                info = pal_info;
            }
        }
    }
    if (info != 0) {
        add_anim_section_async(handle, info, palette_table,
                               allow_duplicate, clear_palette);
    }
}

void add_anim_section_async_pal(int handle, MkFileInfo* info,
                                int* palette_table, int allow_duplicate,
                                int clear_palette) {
    if (section_uses_pal_animation() != 0) {
        MkFileInfo* pal_info = 0;
        unsigned int file_count = num_files_in_ssf(get_current_ssf_file());

        if ((unsigned int)(get_ssf_dir_index(info) + 1) < file_count) {
            pal_info = offset_mk_file_info(info, 1);
        }
        if (pal_info != 0 && strstr(pal_info->name, "_50") != 0) {
            info = pal_info;
        }
    }
    add_anim_section_async(handle, info, palette_table, allow_duplicate, clear_palette);
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

/* TODO: [near miss] 99.87%; pre-open file index uses r29 instead of retail r28. */
int add_anim_section_async(int handle, MkFileInfo* info, int* palette_table,
                           int allow_duplicate, int clear_palette) {
    int file_index;
    SecSlotFileEntry* file;
    SecSlot* slot;

    if (!allow_duplicate) {
        file_index = is_section_loading_or_loaded(handle, info);
        if (file_index != 0) {
            return file_index;
        }
    }
    slot = get_sec_slot_from_handle(handle);
    file = _mwMemMalloc(section_table_heap, sizeof(*file), 3, 0, 0, 0);
    memset(file, 0, sizeof(*file));
    append_slot_file(slot, file);
    if (clear_palette) {
        file->flag_bits.clear_palette = 1;
    }
    file->palette_table = palette_table;
    file_index = slot->file_count;
    sec_slot_file_open_read_async(file, slot, handle, info,
                                  SEC_FILE_USERDATA(SEC_FILE_TYPE_ANIM));
    return file_index;
}

/* TODO: [near miss] 97.10526%; guarded traversal agrees; retail keeps an initial head-to-cursor copy; honest forms exhausted. */
void wait_for_slot_load(int handle) {
    SecSlotFileEntry* file = get_sec_slot_from_handle(handle)->files;

    if (file == 0) {
        return;
    }
    while (file != 0) {
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
}

int load_art_section_async(int handle, MkFileInfo* info) {
    SecSlotFileEntry* file;
    SecSlot* slot = get_sec_slot_from_handle(handle);

    if (slot->files != 0) {
        if (info == slot->files->section_info && slot->files->next == 0) {
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
    SecSlotFileEntry* file;
    SecSlot* slot;
    int file_index = is_section_loading_or_loaded(handle, info);
    if (file_index != 0) {
        return file_index;
    }
    slot = get_sec_slot_from_handle(handle);
    file = _mwMemMalloc(section_table_heap, sizeof(*file), 3, 0, 0, 0);
    memset(file, 0, sizeof(*file));
    append_slot_file(slot, file);
    file_index = slot->file_count;
    sec_slot_file_open_read_async(file, slot, handle, info,
                                  SEC_FILE_USERDATA(SEC_FILE_TYPE_ART));
    return file_index;
}

static void release_slot_file_data(SecSlotFileEntry* file) {
    if (file->load_state != 0) {
        int type = file->section_info->type;
        if (type == SEC_FILE_TYPE_ART) {
            annihilate_art_section_data(file);
        } else if ((type == SEC_FILE_TYPE_ANIM) &
                   ((file->flags & 0x80) != 0)) {
            int* palette = file->palette_table;
            int index;
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

/* TODO: [near miss] 97.17%; palette loop agrees; type temporary r3 vs r0 and flag-load schedule remain. */
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

static inline void release_section_slot_files(SecSlot* slot) {
    SecSlotFileEntry* file = slot->files;
    SecSlotFileEntry* released;

    while (file != 0) {
        release_slot_file_data(file);
        file = file->next;
        slot->file_count--;
    }
    file = slot->files;
    while (file != 0) {
        released = file;
        file = file->next;
        _mwMemFree(released, 0, 0);
    }
    slot->files = 0;
    slot->file_count = 0;
}

/* TODO: [near miss] 96.79%; both traversal owners agree; section type and eager flag schedule remain. */
void unload_section_slot(int handle) {
    SecSlot* slot = get_sec_slot_from_handle(handle);
    release_section_slot_files(slot);
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

static inline SecSlotGroup* find_sec_slot_group(int group_id) {
    SecSlotGroup* group;

    for (group = sec_sys_state.group_list; group != 0; group = group->next) {
        if (group->group_id == group_id) {
            return group;
        }
    }
    return 0;
}

static SecSlot* get_sec_slot_from_handle(int handle) {
    unsigned short slot_id;
    int group_id;
    SecSlotGroup* group;
    SecSlot* slot;
    unsigned int count;
    unsigned int index;

    group_id = handle >> 16;
    slot_id = handle;

    group = find_sec_slot_group(group_id);
    count = group->slot_count;
    slot = group->slots;
    for (index = 0; index < count; index++) {
        if (slot->slot_id == slot_id) {
            return slot;
        }
        slot++;
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

static inline int common_section_position(SectionSlotDef** old_map,
                                          SectionSlotDef** new_map) {
    int position;
    position = -1;
    if (old_map != 0 && new_map != 0) {
        SectionSlotDef* old_def = *old_map;
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
            position++;
            old_def++;
            new_def++;
        }
    }
    return position;
}

static inline int count_section_slots(const SectionPerSlotDef* definitions) {
    int count = 0;

    while (definitions->slot_index != -1) {
        count++;
        definitions++;
    }
    return count;
}

/* TODO: [breakthrough] 91.21%; unsigned size, prefix and map reload recovered;
 * slot-footprint lowering and allocator register homes remain. */
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
    common_position = common_section_position(sec_sys_state.current_map, new_map);
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
    definition = *sec_sys_state.current_map;
    for (group_index = 0; definition->group_id != -1;
         group_index++, definition++) {
        SecSlotGroup* group;
        int slot_count;
        unsigned char* buffer_position;
        int slot_index;
        SecSlot* slot;
        SectionPerSlotDef* per_slot;

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
        slot_count = count_section_slots(definition->per_slot_defs);
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
            allocation_size = per_slot->buffer_size & 0x7FFFFFFFU;
            if (per_slot->buffer_size != allocation_size) {
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

/* TODO: [near miss] 97.61%; traversal agrees; shared section-type register and eager flag-load order remain. */
static void free_all_slots_in_group(SecSlotGroup* group) {
    SecSlot* slot;
    unsigned int slot_index;
    for (slot_index = 0; slot_index < group->slot_count; slot_index++) {
        slot = &group->slots[slot_index];
        release_section_slot_files(slot);
        if (slot->buffer_size != (slot->buffer_size & 0x7FFFFFFFU)) {
            _mwMemFree(slot->base, 0, 0);
        }
    }
    _mwMemFree(group->slots, 0, 0);
    group->slots = 0;
}
