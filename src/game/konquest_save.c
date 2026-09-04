#include "game/konquest_save.h"
#include "game/konquest.h"
#include "game/konquest_items.h"
#include "math/gxVect.h"
#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_struct.h"

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
    char pad28[8];
    MkPtr* triggers;
    MkPtr* temporary_triggers;
    int npc_count;
    MkPtr* npcs;
    char pad40[0xB8];
    MkObj* monk_obj;
    unsigned int monk_instance;
    char pad100[0x38];
    char current_time[0x18];
    char pad150[0x1A4];
    MkPtr* pui_list;
} KonquestPdata;

typedef struct KonquestPuiSaveEntry {
    int inventory_bit;
    float pos_x;
    float pos_y;
    float pos_z;
} KonquestPuiSaveEntry;

typedef struct KonquestTriggerSaveEntry {
    union {
        unsigned char flags;
        struct {
            unsigned char active : 1;
            unsigned char pad_flags : 7;
        } bits;
    };
    char pad01[3];
    int trigger_id;
} KonquestTriggerSaveEntry;

typedef struct KonquestNpcSaveEntry {
    char pad00[4];
    union {
        int flags;
        struct {
            unsigned char pad_flags0 : 3;
            unsigned char cleared_flag : 1;
            unsigned char pad_flags1 : 4;
            unsigned char flags_rest[3];
        } flag_bits;
    };
    float pos_y;
    float pos_z;
    float pos_x;
    int path_id;
    float path_param_a;
    float path_param_b;
    unsigned int path_flags;
    signed char path_field30;
    signed char path_field34;
    signed char path_field3C;
    char pad27;
} KonquestNpcSaveEntry;

typedef struct KonquestRegionBuffer {
    unsigned char header_valid;
    unsigned char loaded_snapshot;
    char pad02[2];
    int region_id;
    int pui_count;
    KonquestPuiSaveEntry pui_entries[0x19];
    KonquestTriggerSaveEntry trigger_entries[0xC8];
    int npc_count;
    KonquestNpcSaveEntry npc_entries[0x96];
    char tail[4];
} KonquestRegionBuffer;

typedef struct KonquestTriggerRuntime {
    char pad00[0x1C];
    unsigned int active;
} KonquestTriggerRuntime;

typedef struct KonquestTrigger {
    MkHdr hdr;
    KonquestTriggerRuntime* userdata;
    char pad0C[0x08];
    int trigger_id;
    char pad18[0x0C];
    unsigned char flags24;
    char pad25[0x1B];
    int save_state;
} KonquestTrigger;

typedef struct PathData {
    char pad00[8];
    void* path_ref;
    int path_id;
    int path_kind;
    char pad14[8];
    float param_a;
    float field_20;
    float param_b;
    char pad28[8];
    int field_30;
    int field_34;
    char pad38[4];
    int field_3C;
} PathData;

typedef struct KonquestNpcTransform {
    char pad00[0x4C];
    union {
        struct {
            float field_4C;
            char pad50[4];
            float field_54;
        };
        Vec tile_position;
    };
    float field_58;
} KonquestNpcTransform;

typedef struct KonquestNpcEventSlot {
    int enabled;
    char pad04[8];
} KonquestNpcEventSlot;

typedef struct KonquestNpc {
    MkHdr hdr;
    char pad08[4];
    KonquestNpcTransform* transform;
    PathData* path_data;
    char pad14[8];
    int flags;
    char pad20[0x20];
    int tile_index;
    char pad44[0x18C];
    KonquestNpcEventSlot event_slots[7];
} KonquestNpc;

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
int f_writing_konquest_profile;
extern int f_writing_to_memcard;

void* memcpy(void* dst, const void* src, int size);
void* memset(void* dst, int value, int size);

int save_profile(int profile, int arg);
/* Decl: include/game/plyrprofile.h */
int validate_konq_save_location(int player);
int validate_konq_load_location(int player);
void set_current_time(char* time);
void set_monk_position(float x, float y, float z, float w);
void update_tile_grid(void);
void mcard_msg_card_inaccessable_in_konq(void);
void quit_from_konquest(void);
KonquestTrigger* find_trigger_by_id(unsigned int id);
int spawn_dynamic_pui_at_pos(
    PuiItem* item, int source_type, const Vec* position, void* source,
    int critical);
void destroy_mkptr(MkPtr* node);
PathData* get_new_path_data_struct(void);
int get_tile_from_position(const Vec* position);
KonquestWaypoint* get_door_path(int id);
void* get_data_table(void* table, int id);
int get_row_count_for_table(void* table, int row);
struct KonquestPuiRuntime;
struct MkSobj;
int should_this_pui_be_saved(const struct KonquestPuiRuntime* pui);
int get_konquest_pui_inventory_bit_index(const int* pui);
void get_konquest_pui_object_pos(Vec* position, const struct MkSobj* pui);
const char* nbc_find_text(int index, int table);
int save_konquest_region_to_memcard_w_error(
    int device, int slot, int mode, const char* title, unsigned int region,
    void* region_buffer, int flag, unsigned int* free_blocks, int* free_bytes);
int load_konquest_region_from_memcard_w_error(
    int device, int slot, int arg, int region, void* buffer, char* card_name,
    int name_len, unsigned int* free_blocks, int* free_bytes);

KonquestRegionBuffer konq_region_data_buffer;
static const float kPathDefaultFloat = 0.0f;

static inline int is_mkptr_list_valid(MkPtr** list) {
    return list != 0;
}

static inline KonquestNpcSaveEntry* get_npc_save_entry(
    KonquestRegionBuffer* buffer, int byte_offset) {
    return (KonquestNpcSaveEntry*)((unsigned char*)buffer->npc_entries +
                                   byte_offset);
}

static inline KonquestPuiSaveEntry* get_pui_save_entry(
    KonquestRegionBuffer* buffer, int byte_offset) {
    return (KonquestPuiSaveEntry*)((unsigned char*)buffer->pui_entries +
                                   byte_offset);
}

static inline KonquestNpcEventSlot* get_npc_event_slot(
    KonquestNpc* npc, int byte_offset) {
    return (KonquestNpcEventSlot*)((unsigned char*)npc->event_slots +
                                   byte_offset);
}

void profile_region_change(void) {
    full_konquest_save_to_memcard(p1_profile_konquest->region_arg, 0, 8);
}

static inline void copy_common_konquest_profile_data(
    KonquestProfileSave* profile) {
    KonquestPdata* pdata;
    MkObj* candidate;
    MkObj* monk;

    memcpy(&profile->common_time[0], konquest_pdata->current_time, 0x18);
    pdata = konquest_pdata;
    candidate = pdata->monk_obj;
    if (candidate != 0) {
        if (candidate->hdr.instance == pdata->monk_instance) {
            monk = candidate;
        } else {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk != 0) {
        profile->monk_pos_x = monk->pos.value.x;
        profile->monk_pos_y = monk->pos.value.y;
        profile->monk_pos_z = monk->pos.value.z;
        profile->monk_pos_w = monk->ang.y;
    }
    profile->profile_valid = 1;
    profile->profile_serial = 1;
}

/*
 * Near match: CFG and call ABI agree; the 28-byte size residue is individual
 * nonvolatile saves/restores instead of stmw/lmw, plus r4/r5 coloring.
 */
int full_konquest_save_to_memcard(int region, int profile_valid, int arg) {
    unsigned int bit;
    int result;
    int device;
    int card_slot;
    char* status;

    copy_common_konquest_profile_data(p1_profile_konquest);
    p1_profile_konquest->profile_valid = profile_valid;
    if (region > 0 && region < 9) {
        bit = 1 << (region - 1);
        p1_profile_konquest->regions_dirty |= bit;
        p1_profile_konquest->regions_loaded_mask ^= bit;
    }
    f_writing_konquest_profile = 1;
    result = save_profile(0, arg);
    if (result == 0) {
        return 0;
    }
    result = save_konq_memory_to_krd_buffer(region);
    if (result == 0) {
        result = 0;
    } else if (p1_profile_status != 1) {
        result = 0;
        f_writing_to_memcard = 0;
    } else if (validate_konq_save_location(0) == 0) {
        result = 0;
        f_writing_to_memcard = 0;
    } else {
        device = p1_profile_device;
        card_slot = p1_profile_slot;
        if (device < 0 || device >= 2) {
            result = 0;
            f_writing_to_memcard = 0;
        } else {
            status = &storage_status[device * 0x28F0];
            result = save_konquest_region_to_memcard_w_error(
                device, card_slot, arg, nbc_find_text(0x30, 1),
                (unsigned char)konq_region_data_buffer.region_id,
                &konq_region_data_buffer, 0, (unsigned int*)(status + 4),
                (int*)(status + 8));
        }
    }
    if (result == 0) {
        return 0;
    }
    f_writing_konquest_profile = 0;
    return 1;
}

/*
 * Near match: algorithm and latch CFG agree; the 20-byte size residue is
 * nonvolatile save/restore scheduling plus camera/profile register coloring.
 */
void full_konquest_load_from_memcard(void) {
    KonquestProfileSave* profile;
    KonquestRegionBuffer* buffer;
    CameraPdata* camera_pdata;
    MkObj* candidate;
    MkObj* monk;

    profile = p1_profile_konquest;
    buffer = &konq_region_data_buffer;
    camera_pdata = get_pdata_of_camera();
    if (profile->profile_serial != 0) {
        set_current_time(profile->common_time);
    }
    if (profile->profile_valid != 0) {
        candidate = konquest_pdata->monk_obj;
        if (candidate != 0) {
            if (candidate->hdr.instance == konquest_pdata->monk_instance) {
                monk = candidate;
            } else {
                monk = 0;
            }
        } else {
            monk = 0;
        }
        if (monk != 0) {
            set_monk_position(profile->monk_pos_x, profile->monk_pos_y,
                              profile->monk_pos_z, profile->monk_pos_w);
            if (camera_pdata != 0) {
                camera_pdata->flags_bits.konquest_mode = 1;
            }
            update_tile_grid();
        }
    }
    if (p1_profile_konquest->active_region != 0) {
        if (buffer->region_id != p1_profile_konquest->active_region) {
            if (load_krd_buffer_from_memcard(0, 1) != 0) {
                load_konq_memory_from_krd_buffer();
            }
        } else {
            load_konq_memory_from_krd_buffer();
        }
    }
}

/*
 * Near match: CFG, calls, and access widths agree; the 16-byte size residue is
 * the compiler's nonvolatile-register assignment and stmw/lmw selection.
 */
int load_krd_buffer_from_memcard(int player, int arg) {
    KonquestProfileSave* profile;
    KonquestRegionBuffer* buffer;
    int status;
    int device;
    int slot;
    char* card_status;
    unsigned char region;

    buffer = &konq_region_data_buffer;

    if (player == 0) {
        status = p1_profile_status;
        profile = p1_profile_konquest;
    } else {
        status = p2_profile_status;
        profile = p2_profile_konquest;
    }
    region = profile->active_region;
    if (region == 0) {
        return 1;
    }
    if (region == 0 || region >= 9) {
        return 0;
    }
    if (buffer->region_id != region &&
        (profile->regions_dirty & (1 << (region - 1)))) {
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
    return 1;
}

/*
 * Near match: validation, trigger/PUI restore, stale-link cleanup, NPC/path
 * restoration, and record advancement match retail. Residue is typed entry-base
 * selection, GPR coloring, and the seven-event CTR-loop lowering.
 */
int load_konq_memory_from_krd_buffer(void) {
    KonquestProfileSave* profile;
    KonquestRegionBuffer* buffer;
    int region;
    int bit;
    int loaded;
    int valid;
    int trigger_index;
    int pui_index;
    KonquestTrigger* trigger;
    KonquestPuiSaveEntry* pui_entry;
    PuiItem* item;
    Vec pos;
    MkPtr** npc_list;
    MkPtr* node;
    MkPtr* next;
    int npc_index;
    int flag_index;

    profile = p1_profile_konquest;
    buffer = &konq_region_data_buffer;
    region = profile->active_region;
    bit = 1 << (region - 1);
    loaded = (profile->regions_loaded_mask & bit) != 0;
    if ((profile->regions_dirty & bit) != 0) {
        if (buffer->header_valid == 0 || buffer->loaded_snapshot != loaded ||
            buffer->region_id != region) {
            valid = 0;
        } else {
            valid = 1;
        }
        if (valid == 0) {
            return 0;
        }
    }
    if (buffer->header_valid == 0 ||
        buffer->region_id != profile->active_region) {
        return 1;
    }
    for (trigger_index = 0; trigger_index < 0xC8; trigger_index++) {
        KonquestTriggerSaveEntry* entry;

        entry = &buffer->trigger_entries[trigger_index];
        if (entry->trigger_id == -1) {
            break;
        }
        trigger = find_trigger_by_id(entry->trigger_id);
        if (trigger != 0 && ((trigger->flags24 & 1) == 0)) {
            trigger->userdata->active = entry->bits.active;
        }
    }
    for (pui_index = 0; pui_index < buffer->pui_count; pui_index++) {
        pui_entry = &buffer->pui_entries[pui_index];
        item = get_pui_item_at_inv_bit_index(pui_entry->inventory_bit);
        if (item != 0) {
            pos.x = pui_entry->pos_x;
            pos.y = pui_entry->pos_y;
            pos.z = pui_entry->pos_z;
            spawn_dynamic_pui_at_pos(item, 3, &pos, 0, 1);
        }
    }
    if (profile->profile_valid == 0) {
        return 1;
    }
    npc_index = 0;
    if (buffer->npc_count != konquest_pdata->npc_count) {
        return 0;
    }
    npc_list = &konquest_pdata->npcs;
    if (is_mkptr_list_valid(npc_list)) {
        node = *npc_list;
        while (node != 0) {
            KonquestNpc* npc;
            KonquestNpcSaveEntry* npc_save;

            npc = (KonquestNpc*)node->hdr;
            if (node->instance != npc->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            if (npc != 0) {
                npc_save = &buffer->npc_entries[npc_index];
                npc->transform->field_58 = npc_save->pos_x;
                npc->transform->field_4C = npc_save->pos_y;
                npc->transform->field_54 = npc_save->pos_z;
                npc->tile_index =
                    get_tile_from_position(&npc->transform->tile_position);
                npc->flags = npc_save->flags & 0x02110000;
                if (npc->path_data == 0) {
                    npc->path_data = get_new_path_data_struct();
                }
                if (npc->path_data != 0) {
                    if (npc_save->path_id == -2) {
                        npc->path_data->path_ref = 0;
                        npc->path_data->path_kind = 0;
                    } else if (npc_save->path_id == -1) {
                        npc->path_data->path_ref = 0;
                        npc->path_data->param_a = npc_save->path_param_a;
                        npc->path_data->field_20 = kPathDefaultFloat;
                        npc->path_data->param_b = npc_save->path_param_b;
                        npc->path_data->path_kind = 1;
                    } else if (npc_save->path_id == 0) {
                        npc->path_data->path_ref = 0;
                        npc->path_data->path_kind = 1;
                    } else if (
                        (unsigned int)npc_save->path_id >= 0x10000000U) {
                        npc->path_data->path_ref =
                            get_door_path(npc_save->path_id);
                        npc->path_data->path_kind = 4;
                    } else {
                        npc->path_data->path_ref = get_data_table(
                            konquest_pdata->data_table_root,
                            npc_save->path_id);
                        npc->path_data->path_kind = get_row_count_for_table(
                            konquest_pdata->data_table_root,
                            npc_save->path_id);
                    }
                    npc->path_data->path_id = npc_save->path_id;
                    npc->path_data->field_34 = npc_save->path_field34;
                    npc->path_data->field_30 = npc_save->path_field30;
                    npc->path_data->field_3C = npc_save->path_field3C;
                }
                flag_index = 0;
                do {
                    npc->event_slots[flag_index].enabled =
                        (npc_save->path_flags & (1U << flag_index)) != 0;
                    flag_index++;
                } while (flag_index < 7);
            }
            node = node->next;
            npc_index++;
        }
    }
    return 1;
}

/*
 * Soft ceiling: save_konq_memory_to_krd_buffer ~87.44% (4 bytes smaller) --
 * algorithms, CFG, record advancement, layouts, and store order match; residue
 * is fixed-base versus cursor addressing, coloring, booleans, and CTR lowering.
 */
int save_konq_memory_to_krd_buffer(int region) {
    KonquestRegionBuffer* buffer;
    MkPtr** trigger_list;
    MkPtr** pui_list;
    MkPtr* node;
    MkPtr* next;
    KonquestTrigger* trigger;
    KonquestNpcSaveEntry* npc_save;
    KonquestPuiSaveEntry* pui_save;
    const struct KonquestPuiRuntime* pui;
    Vec pui_position;
    int trigger_count;
    int pui_count;
    int pui_offset;
    int npc_count;
    int npc_offset;
    unsigned int bit;
    unsigned int loaded;

    buffer = &konq_region_data_buffer;
    memset(buffer->trigger_entries, -1, sizeof(buffer->trigger_entries));
    trigger_list = &konquest_pdata->triggers;
    trigger_count = 0;
    if (is_mkptr_list_valid(trigger_list)) {
        node = *trigger_list;
        while (node != 0) {
            KonquestTriggerSaveEntry* trigger_save;

            trigger = (KonquestTrigger*)node->hdr;
            if (node->instance != trigger->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            if (trigger != 0 && ((trigger->flags24 >> 4) & 1) == 0) {
                if (trigger_count >= 0xC8) {
                    break;
                }
                trigger_save = &buffer->trigger_entries[trigger_count];
                if (trigger->save_state == 2) {
                    trigger_save->bits.active = 0;
                } else if (trigger->save_state == 1) {
                    trigger_save->bits.active = 1;
                } else {
                    trigger_save->bits.active = trigger->userdata->active != 0;
                }
                trigger_save->trigger_id = trigger->trigger_id;
                trigger_count++;
            }
            node = node->next;
        }
    }
    pui_list = &konquest_pdata->pui_list;
    pui_count = 0;
    pui_offset = 0;
    if (is_mkptr_list_valid(pui_list)) {
        node = *pui_list;
        while (node != 0) {
            if (node->instance != node->hdr->instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            pui = (const struct KonquestPuiRuntime*)node->hdr;
            if (should_this_pui_be_saved(pui) != 0) {
                if (pui_count >= 0x19) {
                    break;
                }
                pui_save = get_pui_save_entry(buffer, pui_offset);
                pui_save->inventory_bit =
                    get_konquest_pui_inventory_bit_index((const int*)pui);
                get_konquest_pui_object_pos(&pui_position,
                                            (const struct MkSobj*)pui);
                pui_save->pos_x = pui_position.x;
                pui_save->pos_y = pui_position.y;
                pui_save->pos_z = pui_position.z;
                pui_count++;
                pui_offset += sizeof(KonquestPuiSaveEntry);
            }
            node = node->next;
        }
    }
    buffer->pui_count = pui_count;
    npc_count = 0;
    npc_offset = 0;
    trigger_list = &konquest_pdata->npcs;
    if (is_mkptr_list_valid(trigger_list)) {
        node = *trigger_list;
        while (node != 0) {
            int flag_index;
            int event_offset;
            unsigned int* packed_flags;
            KonquestNpc* npc;

            npc = (KonquestNpc*)node->hdr;
            if (node->instance != npc->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            if (npc_count < 0x96 && npc != 0) {
                npc_save = get_npc_save_entry(buffer, npc_offset);
                npc_save->pos_x = npc->transform->field_58;
                npc_save->pos_y = npc->transform->field_4C;
                npc_save->pos_z = npc->transform->field_54;
                npc_save->flags = npc->flags;
                npc_save->flag_bits.cleared_flag = 0;
                if (npc->path_data != 0) {
                    npc_save->path_id = npc->path_data->path_id;
                    npc_save->path_field30 =
                        (signed char)npc->path_data->field_30;
                    npc_save->path_field34 =
                        (signed char)npc->path_data->field_34;
                    npc_save->path_field3C =
                        (signed char)npc->path_data->field_3C;
                    npc_save->path_param_a = npc->path_data->param_a;
                    npc_save->path_param_b = npc->path_data->param_b;
                } else {
                    npc_save->path_id = 0;
                    npc_save->path_field30 = 0;
                    npc_save->path_field34 = 0;
                    npc_save->path_field3C = 0;
                    npc_save->path_param_a = kPathDefaultFloat;
                    npc_save->path_param_b = kPathDefaultFloat;
                }
                packed_flags = &npc_save->path_flags;
                *packed_flags = 0;
                flag_index = 0;
                event_offset = 0;
                do {
                    if (get_npc_event_slot(npc, event_offset)->enabled != 0) {
                        *packed_flags |= 1U << flag_index;
                    }
                    flag_index++;
                    event_offset += sizeof(KonquestNpcEventSlot);
                } while (flag_index < 7);
            }
            node = node->next;
            npc_count++;
            npc_offset += sizeof(KonquestNpcSaveEntry);
        }
    }
    buffer->npc_count = npc_count;
    bit = 1 << (region - 1);
    buffer->region_id = region;
    buffer->header_valid = 1;
    loaded = (p1_profile_konquest->regions_loaded_mask & bit) != 0;
    buffer->loaded_snapshot = (unsigned char)loaded;
    return 1;
}

/* Retail-sized; remaining mismatch is r4/r5 coloring in the monk latch. */
void save_konq_common_data_to_buffer(void) {
    copy_common_konquest_profile_data(p1_profile_konquest);
}

/*
 * Soft ceiling: validate_region_buffer ~98.1% at the exact retail size under
 * this TU's retail-supported -O4,s mode. Predicate, booleanization, CFG, and
 * access widths agree; only local GPR coloring remains.
 */
int validate_region_buffer(int region) {
    KonquestProfileSave* profile;
    KonquestRegionBuffer* buffer;
    int bit;
    int loaded;

    profile = p1_profile_konquest;
    buffer = &konq_region_data_buffer;
    bit = 1 << (region - 1);
    loaded = (profile->regions_loaded_mask & bit) != 0;
    if ((profile->regions_dirty & bit) &&
        (buffer->header_valid == 0 ||
         buffer->loaded_snapshot != loaded ||
         buffer->region_id != region)) {
        return 0;
    }
    return 1;
}

void clear_region_buffer(void) {
    memset(&konq_region_data_buffer, 0, 0x1F54);
}
