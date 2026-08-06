#include "game/konquest_items.h"

#include "game/nbc.h"
#include "runtime/asset.h"
#include "runtime/utils.h"

#define konquest_items_string_base stringBase0

extern const char stringBase0[];

typedef struct CommonProfileSave {
    char pad00[0x38]; /* +0x00 */
    int kills;        /* +0x38 */
    int fights_won;   /* +0x3C */
    int fights_lost;  /* +0x40 */
    int koins;        /* +0x44 */
    int souls;        /* +0x48 */
    int secrets;      /* +0x4C */
} CommonProfileSave;

typedef struct KonquestProfileSave {
    char pad00[0x24];                         /* +0x000 */
    unsigned char flags_300[38];              /* +0x024, 300 bits */
    char pad4A[0x1E];                         /* +0x04A */
    int last_character_trained_with;          /* +0x068 */
    unsigned char trained_characters[0x1C2];  /* +0x06C */
    unsigned char konquest_bytes[0x18];       /* +0x22E */
    unsigned char flags_200_a[25];            /* +0x246, 200 bits */
    unsigned char flags_200_b[25];            /* +0x25F, 200 bits */
    unsigned char flags_200_c[25];            /* +0x278, 200 bits */
    unsigned char inventory_bits[];           /* +0x291 */
} KonquestProfileSave;

typedef struct KonquestPdata {
    char pad00[0x308];               /* +0x000 */
    unsigned char pui_status_bits[]; /* +0x308 */
} KonquestPdata;

extern unsigned char p1_profile[];
extern CommonProfileSave* p1_profile_common;
extern KonquestProfileSave* p1_profile_konquest;
extern KonquestPdata* konquest_pdata;

int is_mark_as_unlocked(void* profile, int category, int character);
int get_num_puis(void);
PuiItem* get_pui_item_at_inv_bit_index(int index);
int get_pui_inventory_bit_index(PuiItem* item);
const char* get_string_by_id(int id);
int strcmp(const char* a, const char* b);

int get_last_character_trained_with(void) {
    int count;
    int character;
    unsigned int target;

    if (get_konq_profile_value(0, 0) != 0) {
        count = 0;
        character = -1;
        target = randu0(0x2C) & 0xFFFF;
        do {
            character++;
            if (character >= 0x2C) {
                character = 0;
            }
            if (is_mark_as_unlocked(p1_profile, 1, character) != 0) {
                count++;
            }
        } while (count < target);
        return character & ~(character >> 31);
    }
    return p1_profile_konquest->last_character_trained_with;
}

void set_last_character_trained_with(int character) {
    p1_profile_konquest->last_character_trained_with = character;
}

const char* get_konq_profile_value_item_name(int index) {
    PuiItem* item;
    const char* result;
    int string_id;

    result = &konquest_items_string_base[0];
    if (index < 0 || index >= get_num_puis()) {
        return result;
    }
    item = get_pui_item_at_inv_bit_index(index);
    if (item != 0) {
        string_id = item->name_id;
        if ((unsigned int)(string_id + 1) != 0xFFFF) {
            result = get_string_by_id(string_id | 0x30000);
        }
    }
    return result;
}

const char* get_konq_profile_value_item_description(int index) {
    PuiItem* item;
    const char* result;
    int string_id;

    result = &konquest_items_string_base[0];
    if (index < 0 || index >= get_num_puis()) {
        return result;
    }
    item = get_pui_item_at_inv_bit_index(index);
    if (item != 0) {
        string_id = item->desc_id;
        if ((unsigned int)(string_id + 1) != 0xFFFF) {
            result = get_string_by_id(string_id | 0x30000);
        }
    }
    return result;
}

RwTexture* get_konq_profile_value_item_tga_alpha(int index) {
    PuiItem* item;
    RwTexture* result;

    result = 0;
    if (index < 0 || index >= get_num_puis()) {
        return result;
    }
    item = get_pui_item_at_inv_bit_index(index);
    if (item != 0 && strcmp(item->filename, &konquest_items_string_base[1]) != 0) {
        result = load_named_alpha_texture_from_slot(0x60025, item->filename);
    }
    return result;
}

RwTexture* get_konq_profile_value_item_tga(int index) {
    PuiItem* item;
    RwTexture* result;

    result = 0;
    if (index < 0 || index >= get_num_puis()) {
        return result;
    }
    item = get_pui_item_at_inv_bit_index(index);
    if (item != 0 && strcmp(item->filename, &konquest_items_string_base[1]) != 0) {
        result = load_named_tga_from_slot(0x60025, item->filename);
    }
    return result;
}

int find_next_item_in_inventory(int index) {
    int owned;
    PuiItem* item;

    if (index < -1) {
        return -1;
    }
    if (index >= get_num_puis()) {
        return -1;
    }
    index++;
    while (index < get_num_puis()) {
        if (index < 0) {
            owned = 0;
        } else if (index >= get_num_puis()) {
            owned = 0;
        } else {
            owned = get_u8_bit(p1_profile_konquest->inventory_bits, get_num_puis(), index);
            item = get_pui_item_at_inv_bit_index(index);
            if (item->type != 1) {
                owned = 0;
            }
        }
        if (owned != 0) {
            return index;
        }
        index++;
    }
    return -1;
}

int get_number_items_in_inventory(void) {
    int count;
    int index;
    int owned;
    PuiItem* item;

    count = 0;
    for (index = 0; index < get_num_puis(); index++) {
        if (index < 0) {
            owned = 0;
        } else if (index >= get_num_puis()) {
            owned = 0;
        } else {
            owned = get_u8_bit(p1_profile_konquest->inventory_bits, get_num_puis(), index);
            item = get_pui_item_at_inv_bit_index(index);
            if (item->type != 1) {
                owned = 0;
            }
        }
        if (owned != 0) {
            count++;
        }
    }
    return count;
}

void add_to_konq_profile_value(int type, int value) {
    int current;

    if (type == 1) {
        return;
    }
    if (type < 1) {
        return;
    }
    if (type >= 0xD) {
        return;
    }
    if (type < 7) {
        return;
    }
    current = get_konq_profile_value(type, 0);
    set_konq_profile_value(type, 0, current + value);
}

int get_konq_profile_value(int type, int index) {
    int value;

    value = 0;
    if ((unsigned int)type > 0xD) {
        return 0;
    }
    switch (type) {
    case 0:
        if (index < 0) {
            break;
        }
        if (index >= 0x12C) {
            break;
        }
        value = get_u8_bit(p1_profile_konquest->flags_300, 0x12C, index);
        break;
    case 1:
        break;
    case 2:
        if (index < 0) {
            break;
        }
        if (index >= 0xC8) {
            break;
        }
        value = get_u8_bit(p1_profile_konquest->flags_200_a, 0xC8, index);
        break;
    case 3:
        if (index < 0) {
            break;
        }
        if (index >= 0xC8) {
            break;
        }
        value = get_u8_bit(p1_profile_konquest->flags_200_b, 0xC8, index);
        break;
    case 4:
        if (index < 0) {
            break;
        }
        if (index >= 0xC8) {
            break;
        }
        value = get_u8_bit(p1_profile_konquest->flags_200_c, 0xC8, index);
        break;
    case 5:
        if (index < 0) {
            break;
        }
        if (index >= 0x1C2) {
            break;
        }
        value = p1_profile_konquest->trained_characters[index];
        break;
    case 6:
        if (index < 0) {
            break;
        }
        if (index >= get_num_puis()) {
            break;
        }
        if (index < 0) {
            break;
        }
        if (index >= get_num_puis()) {
            break;
        }
        value = get_u8_bit(p1_profile_konquest->inventory_bits, get_num_puis(), index);
        if (get_pui_item_at_inv_bit_index(index)->type != 1) {
            value = 0;
        }
        break;
    case 7:
        value = p1_profile_common->kills;
        break;
    case 8:
        value = p1_profile_common->fights_won;
        break;
    case 9:
        value = p1_profile_common->fights_lost;
        break;
    case 10:
        value = p1_profile_common->koins;
        break;
    case 11:
        value = p1_profile_common->souls;
        break;
    case 12:
        value = p1_profile_common->secrets;
        break;
    case 13:
        if (index < 0) {
            break;
        }
        if (index >= 0x18) {
            break;
        }
        value = p1_profile_konquest->konquest_bytes[index];
        break;
    }
    return value;
}

void set_konq_profile_value(int type, int index, int value) {
    if ((unsigned int)type > 0xD) {
        return;
    }
    switch (type) {
    case 0:
        if (index < 0) {
            return;
        }
        if (index >= 0x12C) {
            return;
        }
        set_u8_bit(p1_profile_konquest->flags_300, 0x12C, index, value);
        return;
    case 1:
        return;
    case 2:
        if (index < 0) {
            return;
        }
        if (index >= 0xC8) {
            return;
        }
        set_u8_bit(p1_profile_konquest->flags_200_a, 0xC8, index, value);
        return;
    case 3:
        if (index < 0) {
            return;
        }
        if (index >= 0xC8) {
            return;
        }
        set_u8_bit(p1_profile_konquest->flags_200_b, 0xC8, index, value);
        return;
    case 4:
        if (index < 0) {
            return;
        }
        if (index >= 0xC8) {
            return;
        }
        set_u8_bit(p1_profile_konquest->flags_200_c, 0xC8, index, value);
        return;
    case 5:
        if (index < 0) {
            return;
        }
        if (index >= 0x1C2) {
            return;
        }
        if (value < 0) {
            return;
        }
        if (value >= 0xFF) {
            return;
        }
        p1_profile_konquest->trained_characters[index] = (unsigned char)value;
        return;
    case 6:
        if (index < 0) {
            return;
        }
        if (index >= get_num_puis()) {
            return;
        }
        if (index < 0) {
            return;
        }
        if (index >= get_num_puis()) {
            return;
        }
        set_u8_bit(p1_profile_konquest->inventory_bits, get_num_puis(), index, value);
        return;
    case 7:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->kills = value;
        return;
    case 8:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->fights_won = value;
        return;
    case 9:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->fights_lost = value;
        return;
    case 10:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->koins = value;
        return;
    case 11:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->souls = value;
        return;
    case 12:
        if (value > 0xF423F) {
            value = 0xF423F;
        }
        p1_profile_common->secrets = value;
        return;
    case 13:
        if (index < 0) {
            return;
        }
        if (index >= 0x18) {
            return;
        }
        if (value < 0) {
            return;
        }
        if (value >= 0xFF) {
            return;
        }
        p1_profile_konquest->konquest_bytes[index] = (unsigned char)value;
        return;
    }
}

void set_pui_status(PuiItem* item, int status) {
    int bit_index;

    bit_index = get_pui_inventory_bit_index(item);
    if (bit_index < 0 || bit_index >= get_num_puis()) {
        return;
    }
    set_u8_bit(konquest_pdata->pui_status_bits, get_num_puis(), bit_index, status);
}

int get_pui_status(PuiItem* item) {
    int bit_index;

    bit_index = get_pui_inventory_bit_index(item);
    if (bit_index < 0 || bit_index >= get_num_puis()) {
        return 0;
    }
    return get_u8_bit(konquest_pdata->pui_status_bits, get_num_puis(), bit_index);
}

const char stringBase0[] = "\0""0\0";
const int gap_04_803106A4_rodata = 0;
