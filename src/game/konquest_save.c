#include "game/konquest_save.h"

typedef struct KonquestProfileSave {
    int profile_valid;
    int profile_serial;
    char pad08[4];
    char common_time[0x18];
    char pad24[0x28];
    float monk_pos_x;
    float monk_pos_y;
    float monk_pos_z;
    float monk_pos_w;
    char pad5c;
    unsigned char active_region;
    unsigned char region_arg;
    unsigned char regions_dirty;
    unsigned char regions_loaded_mask;
    char pad61[0x37];
} KonquestProfileSave;

typedef struct KonquestPdata {
    char pad00[0x24];
    void* data_table_root;
    char pad28[0x10];
    void* monk_obj;
    char pad3C[0xBC];
    char pui_status_bits[];
} KonquestPdata;

typedef struct KonquestRegionBuffer {
    unsigned char header_valid;
    unsigned char checksum_flag;
    char pad02[2];
    int region_id;
    int pui_count;
    char pui_entries[0x190];
    char trigger_bytes[0x640];
    int npc_count;
    char npc_entries[];
} KonquestRegionBuffer;

typedef struct KonquestPuiSaveEntry {
    int inventory_bit;
    float pos_x;
    float pos_y;
    float pos_z;
} KonquestPuiSaveEntry;

typedef struct KonquestTriggerSaveEntry {
    int trigger_id;
} KonquestTriggerSaveEntry;

typedef struct KonquestNpcSaveEntry {
    float pos_x;
    float pos_y;
    float pos_z;
    int field_1C;
    int path_id;
    float path_param_a;
    float path_param_b;
    char path_byte_a;
    char path_byte_b;
    char path_byte_c;
    char pad[5];
    int path_flags[7];
} KonquestNpcSaveEntry;

typedef struct MkptrNode {
    struct MkptrNode* next;
    void* object;
    int serial;
    char pad0C[4];
    void* userdata;
    char pad14[0x10];
    void* path_data;
} MkptrNode;

typedef struct KonquestTrigger {
    char pad00[0x08];
    void* userdata;
    char pad0C[0x10];
    int field_1C;
    char pad20[4];
    unsigned char flags24;
} KonquestTrigger;

typedef struct PathData {
    int field00;
    void* path_ref;
    int path_kind;
    float field1C;
    float field20;
    float field24;
    int field30;
    int field34;
    int field3C;
    char pad40[];
} PathData;

extern KonquestProfileSave* p1_profile_konquest;
extern KonquestProfileSave* p2_profile_konquest;
extern KonquestPdata* konquest_pdata;
extern int p1_profile_status;
extern int p2_profile_status;
extern int p1_profile_device;
extern int p2_profile_device;
extern int p1_profile_slot;
extern int p2_profile_slot;
extern char storage_status[];
extern int f_writing_konquest_profile;
extern int f_writing_to_memcard;

void* memcpy(void* dst, const void* src, int size);
void* memset(void* dst, int value, int size);

int save_profile(int profile, int arg);
/* Decl: include/game/plyrprofile.h */
int validate_konq_save_location(int player);
int validate_konq_load_location(int player);
typedef struct CameraPdata CameraPdata;
CameraPdata* get_pdata_of_camera(void);
void set_current_time(char* time);
void set_monk_position(float x, float y, float z, float w);
void update_tile_grid(void);
void mcard_msg_card_inaccessable_in_konq(void);
void quit_from_konquest(void);
void* find_trigger_by_id(int id);
void* get_pui_item_at_inv_bit_index(int index);
void spawn_dynamic_pui_at_pos(void* item, int mode, float* pos, int arg, int flag);
void destroy_mkptr(void* node);
void* get_new_path_data_struct(void);
int get_tile_from_position(float* pos);
void* get_door_path(int id);
void* get_data_table(void* table, int id);
int get_row_count_for_table(void* table, int row);
int should_this_pui_be_saved(void* pui);
int get_konquest_pui_inventory_bit_index(void* pui);
void get_konquest_pui_object_pos(void* pui, float* pos);
const char* nbc_find_text(int device, int id);
int save_konquest_region_to_memcard_w_error(int device, int slot, int arg, void* status_field4,
                                              int text_id, void* buf, int size, void* field8,
                                              void* field10);
int load_konquest_region_from_memcard_w_error(
    int device, int slot, int arg, int region, void* buffer, char* card_name,
    int name_len, unsigned int* free_blocks, int* free_bytes);

static KonquestRegionBuffer konq_region_data_buffer;
static const float kPathDefaultFloat = 0.0f;

void profile_region_change(void) {
    full_konquest_save_to_memcard(p1_profile_konquest->region_arg, 0, 8);
}

static void copy_common_konquest_profile_data(KonquestProfileSave* profile) {
    void* monk;

    memcpy(&profile->common_time[0], (char*)konquest_pdata + 0x138, 0x18);
    monk = konquest_pdata->monk_obj;
    if (monk != 0 && *(int*)((char*)monk + 4) == *(int*)((char*)konquest_pdata + 0xFC)) {
        profile->monk_pos_x = *(float*)((char*)monk + 0xA0);
        profile->monk_pos_y = *(float*)((char*)monk + 0xA4);
        profile->monk_pos_z = *(float*)((char*)monk + 0xA8);
        profile->monk_pos_w = *(float*)((char*)monk + 0xD4);
    }
    profile->profile_valid = 1;
    profile->profile_serial = 1;
}

int full_konquest_save_to_memcard(int region, int slot, int arg) {
    int bit;
    int result;
    void* status;
    const char* text;

    copy_common_konquest_profile_data(p1_profile_konquest);
    p1_profile_konquest->profile_serial = slot;
    if (region > 0 && region < 9) {
        bit = 1 << (region - 1);
        p1_profile_konquest->regions_dirty |= (unsigned char)bit;
        p1_profile_konquest->regions_loaded_mask ^= (unsigned char)bit;
    }
    f_writing_konquest_profile = 1;
    result = save_profile(0, arg);
    if (result == 0) {
        f_writing_konquest_profile = 0;
        return 0;
    }
    result = save_konq_memory_to_krd_buffer(region);
    if (result == 0) {
        f_writing_konquest_profile = 0;
        return 0;
    }
    if (p1_profile_status != 1) {
        f_writing_to_memcard = 0;
        return 0;
    }
    result = validate_konq_save_location(0);
    if (result == 0) {
        f_writing_to_memcard = 0;
        return 0;
    }
    if (p1_profile_device < 0 || p1_profile_device >= 2) {
        f_writing_to_memcard = 0;
        return 0;
    }
    status = &storage_status[p1_profile_device * 0x28F0];
    text = nbc_find_text(1, 0x30);
    result = save_konquest_region_to_memcard_w_error(
        p1_profile_device, p1_profile_slot, arg, (char*)status + 4, (int)text,
        &konq_region_data_buffer, *(int*)((char*)&konq_region_data_buffer + 4) & 0xFF, 0,
        (char*)status + 8);
    if (result == 0) {
        return 0;
    }
    f_writing_konquest_profile = 0;
    return 1;
}

void full_konquest_load_from_memcard(void) {
    CameraPdata* camera_pdata;
    void* monk;

    camera_pdata = get_pdata_of_camera();
    if (p1_profile_konquest->profile_serial != 0) {
        set_current_time(p1_profile_konquest->common_time);
    }
    if (p1_profile_konquest->profile_valid != 0) {
        monk = konquest_pdata->monk_obj;
        if (monk != 0 && *(int*)((char*)monk + 4) == *(int*)((char*)konquest_pdata + 0xFC)) {
            set_monk_position(p1_profile_konquest->monk_pos_x, p1_profile_konquest->monk_pos_y,
                              p1_profile_konquest->monk_pos_z, p1_profile_konquest->monk_pos_w);
            if (camera_pdata != 0) {
                *(unsigned char*)((char*)camera_pdata + 0x6C) =
                    (*(unsigned char*)((char*)camera_pdata + 0x6C) & ~0x40) | 0x40;
            }
            update_tile_grid();
        }
    }
    if (p1_profile_konquest->active_region != 0) {
        if (konq_region_data_buffer.region_id != p1_profile_konquest->active_region) {
            if (load_krd_buffer_from_memcard(0, 1) != 0) {
                load_konq_memory_from_krd_buffer();
            }
        } else {
            load_konq_memory_from_krd_buffer();
        }
    }
}

int load_krd_buffer_from_memcard(int player, int arg) {
    KonquestProfileSave* profile;
    int status;
    int device;
    int slot;
    char* card_status;
    int bit;

    if (player == 0) {
        status = p1_profile_status;
        profile = p1_profile_konquest;
    } else {
        status = p2_profile_status;
        profile = p2_profile_konquest;
    }
    if (profile->active_region == 0) {
        return 1;
    }
    if (profile->active_region >= 9) {
        return 0;
    }
    if (konq_region_data_buffer.region_id == profile->active_region) {
        return 1;
    }
    bit = 1 << (profile->active_region - 1);
    if ((profile->regions_dirty & bit) == 0) {
        return 1;
    }
    if (status != 1) {
        f_writing_to_memcard = 0;
        return 0;
    }
    if (validate_konq_load_location(player) == 0) {
        mcard_msg_card_inaccessable_in_konq();
        quit_from_konquest();
    }
    if (player == 0) {
        device = p1_profile_device;
        slot = p1_profile_slot;
    } else {
        device = p2_profile_device;
        slot = p2_profile_slot;
    }
    if (device < 0 || device >= 2) {
        f_writing_to_memcard = 0;
        return 0;
    }
    card_status = &storage_status[device * 0x28F0];
    return load_konquest_region_from_memcard_w_error(
        device, slot, arg, profile->active_region, &konq_region_data_buffer,
        card_status + 0xD, 0xB, (unsigned int*)(card_status + 4),
        (int*)(card_status + 8));
}

static int region_buffer_matches_profile(int region) {
    int bit;
    int loaded;

    bit = 1 << (region - 1);
    loaded = (p1_profile_konquest->regions_loaded_mask & bit) != 0;
    loaded = loaded - 1;
    loaded = -loaded;
    if (((p1_profile_konquest->regions_dirty & bit) != 0) == 0) {
        return 1;
    }
    if (konq_region_data_buffer.header_valid == 0) {
        return 0;
    }
    if (konq_region_data_buffer.checksum_flag != loaded) {
        return 0;
    }
    if (konq_region_data_buffer.region_id != region) {
        return 0;
    }
    return 1;
}

int load_konq_memory_from_krd_buffer(void) {
    int region;
    int trigger_index;
    int pui_index;
    KonquestTrigger* trigger;
    KonquestPuiSaveEntry* pui_entry;
    void* item;
    float pos[3];
    MkptrNode* npc_list;
    MkptrNode* node;
    int npc_index;
    PathData* path;
    int flag_index;

    region = p1_profile_konquest->active_region;
    if (region_buffer_matches_profile(region) == 0) {
        return 0;
    }
    if (konq_region_data_buffer.header_valid == 0 ||
        konq_region_data_buffer.region_id != region) {
        return 1;
    }
    for (trigger_index = 0; trigger_index < 0xC8; trigger_index++) {
        KonquestTriggerSaveEntry* entry;

        entry = (KonquestTriggerSaveEntry*)&konq_region_data_buffer.trigger_bytes[trigger_index * 8];
        if (entry->trigger_id == -1) {
            break;
        }
        trigger = find_trigger_by_id(entry->trigger_id);
        if (trigger != 0 && ((trigger->flags24 & 1) == 0)) {
            *(int*)((char*)trigger->userdata + 0x1C) =
                (konq_region_data_buffer.trigger_bytes[trigger_index * 8 + 0x1C] >> 7) & 1;
        }
    }
    for (pui_index = 0; pui_index < konq_region_data_buffer.pui_count; pui_index++) {
        pui_entry = (KonquestPuiSaveEntry*)&konq_region_data_buffer.pui_entries[pui_index * 0x10];
        item = get_pui_item_at_inv_bit_index(pui_entry->inventory_bit);
        if (item != 0) {
            pos[0] = pui_entry->pos_x;
            pos[1] = pui_entry->pos_y;
            pos[2] = pui_entry->pos_z;
            spawn_dynamic_pui_at_pos(item, 3, pos, 0, 1);
        }
    }
    if (p1_profile_konquest->profile_valid == 0) {
        return 1;
    }
    if (konq_region_data_buffer.npc_count != *(int*)((char*)konquest_pdata + 0x38)) {
        return 0;
    }
    npc_list = *(MkptrNode**)((char*)konquest_pdata + 0x3C);
    npc_index = 0;
    for (node = npc_list; node != 0; node = node->next) {
        KonquestTrigger* npc;
        KonquestNpcSaveEntry* npc_save;

        npc = (KonquestTrigger*)node->object;
        if (node->serial != npc->field_1C) {
            destroy_mkptr(node);
            continue;
        }
        if (npc == 0) {
            continue;
        }
        npc_save = (KonquestNpcSaveEntry*)&konq_region_data_buffer.npc_entries[npc_index * 0x28];
        *(float*)((char*)npc->userdata + 0x58) = npc_save->pos_x;
        *(float*)((char*)npc->userdata + 0x4C) = npc_save->pos_y;
        *(float*)((char*)npc->userdata + 0x54) = npc_save->pos_z;
        npc->field_1C = get_tile_from_position((float*)((char*)npc->userdata + 0x4C));
        npc->field_1C = npc_save->field_1C & 0x211;
        if (node->path_data == 0) {
            node->path_data = get_new_path_data_struct();
        }
        path = node->path_data;
        if (path == 0) {
            continue;
        }
        if (npc_save->path_id == -2) {
            path->path_ref = 0;
            path->path_kind = 0;
        } else if (npc_save->path_id == -1) {
            path->path_ref = 0;
            path->path_kind = 1;
            path->field1C = npc_save->path_param_a;
            path->field20 = kPathDefaultFloat;
            path->field24 = npc_save->path_param_b;
        } else if (npc_save->path_id == 0) {
            path->path_ref = 0;
            path->path_kind = 1;
        } else if (npc_save->path_id >= 0x1000) {
            path->path_ref = get_door_path(npc_save->path_id);
            path->path_kind = 4;
        } else {
            path->path_ref =
                get_data_table(*(void**)((char*)konquest_pdata + 0x24), npc_save->path_id);
            path->path_kind = get_row_count_for_table(
                *(void**)((char*)konquest_pdata + 0x24), npc_save->path_id);
        }
        path->field00 = npc_save->path_id;
        path->field30 = npc_save->path_byte_b;
        path->field34 = npc_save->path_byte_a;
        path->field3C = npc_save->path_byte_c;
        for (flag_index = 0; flag_index < 7; flag_index++) {
            int* dst;

            dst = (int*)((char*)path + 0x1D0 + flag_index * 0xC);
            *dst = (npc_save->path_flags[flag_index] != 0);
        }
        npc_index++;
    }
    return 1;
}

static void purge_invalid_mkptr_list(MkptrNode** list) {
    MkptrNode* node;
    MkptrNode* next;

    node = *list;
    while (node != 0) {
        if (node->serial != ((KonquestTrigger*)node->object)->field_1C) {
            next = node->next;
            node->next = 0;
            destroy_mkptr(node);
            node = next;
        } else {
            node = node->next;
        }
    }
}

int save_konq_memory_to_krd_buffer(int region) {
    MkptrNode* trigger_list;
    MkptrNode* pui_list;
    MkptrNode* node;
    KonquestTrigger* trigger;
    KonquestNpcSaveEntry* npc_save;
    KonquestPuiSaveEntry* pui_save;
    int trigger_count;
    int pui_count;
    int npc_count;
    int bit;
    int loaded;

    memset((char*)&konq_region_data_buffer + 0x19C, -1, 0x640);
    trigger_list = *(MkptrNode**)((char*)konquest_pdata + 0x30);
    purge_invalid_mkptr_list(&trigger_list);
    trigger_count = 0;
    for (node = trigger_list; node != 0; node = node->next) {
        trigger = (KonquestTrigger*)node->object;
        if (trigger == 0 || ((trigger->flags24 >> 3) & 1) != 0) {
            continue;
        }
        if (trigger_count >= 0xC8) {
            break;
        }
        if (trigger->field_1C == 2) {
            konq_region_data_buffer.trigger_bytes[trigger_count + 0x19C] &= ~0x80;
        } else if (trigger->field_1C == 1) {
            konq_region_data_buffer.trigger_bytes[trigger_count + 0x19C] |= 0x80;
        } else {
            int active;

            active = (*(int*)((char*)trigger->userdata + 0x1C) != 0);
            konq_region_data_buffer.trigger_bytes[trigger_count + 0x19C] =
                (konq_region_data_buffer.trigger_bytes[trigger_count + 0x19C] & ~0x80) |
                (active << 7);
        }
        *(int*)&konq_region_data_buffer.trigger_bytes[trigger_count * 8 + 0x1A0] = node->serial;
        trigger_count++;
    }
    pui_list = *(MkptrNode**)((char*)konquest_pdata + 0x2F4);
    purge_invalid_mkptr_list(&pui_list);
    pui_count = 0;
    for (node = pui_list; node != 0; node = node->next) {
        if (should_this_pui_be_saved(node->object) == 0) {
            continue;
        }
        if (pui_count >= 0x19) {
            break;
        }
        pui_save = (KonquestPuiSaveEntry*)&konq_region_data_buffer.pui_entries[pui_count * 0x10];
        pui_save->inventory_bit = get_konquest_pui_inventory_bit_index(node->object);
        get_konquest_pui_object_pos(node->object, (float*)pui_save);
        pui_count++;
    }
    konq_region_data_buffer.pui_count = pui_count;
    npc_count = 0;
    trigger_list = *(MkptrNode**)((char*)konquest_pdata + 0x3C);
    for (node = trigger_list; node != 0; node = node->next) {
        PathData* path;
        int flag_index;

        trigger = (KonquestTrigger*)node->object;
        if (node->serial != trigger->field_1C) {
            destroy_mkptr(node);
            continue;
        }
        if (npc_count >= 0x96 || trigger == 0) {
            break;
        }
        npc_save = (KonquestNpcSaveEntry*)&konq_region_data_buffer.npc_entries[npc_count * 0x28];
        *(float*)&npc_save->pos_x = *(float*)((char*)trigger->userdata + 0x58);
        *(float*)&npc_save->pos_y = *(float*)((char*)trigger->userdata + 0x4C);
        *(float*)&npc_save->pos_z = *(float*)((char*)trigger->userdata + 0x54);
        npc_save->field_1C = trigger->field_1C;
        npc_save->field_1C &= ~0x10;
        path = node->path_data;
        if (path != 0) {
            npc_save->path_id = path->field00;
            npc_save->path_byte_a = (char)path->field34;
            npc_save->path_byte_b = (char)path->field30;
            npc_save->path_byte_c = (char)path->field3C;
            npc_save->path_param_a = path->field1C;
            npc_save->path_param_b = path->field24;
            for (flag_index = 0; flag_index < 7; flag_index++) {
                int* src;

                src = (int*)((char*)path + 0x1D0 + flag_index * 0xC);
                if (*src != 0) {
                    npc_save->path_flags[flag_index] |= 1 << flag_index;
                }
            }
        } else {
            npc_save->path_id = 0;
            npc_save->path_byte_a = 0;
            npc_save->path_byte_b = 0;
            npc_save->path_byte_c = 0;
            npc_save->path_param_a = kPathDefaultFloat;
            npc_save->path_param_b = kPathDefaultFloat;
        }
        npc_count++;
    }
    konq_region_data_buffer.npc_count = npc_count;
    bit = 1 << (region - 1);
    loaded = p1_profile_konquest->regions_loaded_mask & bit;
    loaded = loaded - 1;
    loaded = -loaded;
    konq_region_data_buffer.header_valid = 1;
    konq_region_data_buffer.region_id = region;
    konq_region_data_buffer.checksum_flag = (unsigned char)loaded;
    return 1;
}

void save_konq_common_data_to_buffer(void) {
    copy_common_konquest_profile_data(p1_profile_konquest);
}

int validate_region_buffer(int region) {
    int bit;
    int loaded;

    bit = 1 << (region - 1);
    loaded = (p1_profile_konquest->regions_loaded_mask & bit) != 0;
    loaded = loaded - 1;
    loaded = -loaded;
    if (((p1_profile_konquest->regions_dirty & bit) != 0) == 0) {
        return 1;
    }
    if (konq_region_data_buffer.header_valid == 0) {
        return 0;
    }
    if (konq_region_data_buffer.checksum_flag != loaded) {
        return 0;
    }
    if (konq_region_data_buffer.region_id != region) {
        return 0;
    }
    return 1;
}

void clear_region_buffer(void) {
    memset(&konq_region_data_buffer, 0, 0x1F54);
}
