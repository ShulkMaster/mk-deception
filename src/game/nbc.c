#include "game/nbc.h"

typedef struct TextTableInfo {
    const char** strings;
    int count;
} TextTableInfo;

extern const char* gc_mc_msg_text[];

/*
 * Retail @stringBase0 + nbc_general_text + text_table_info live in this TU.
 * Empty fallback is stringBase0 + 0x1A62 (a single space).
 */
/* 0x1A64 pool + 4-byte retail .rodata gap */
static const char stringBase0[0x1A68] = {
#include "game/nbc_stringBase0.inc"
    0,
    0,
    0,
    0,
};

#define nbc_empty_string (&stringBase0[0x1A62])

const char* nbc_general_text[] = {
#include "game/nbc_general_text.inc"
};

TextTableInfo text_table_info[] = {
    {(const char**)gc_mc_msg_text, 0x91},
    {nbc_general_text, 0x4E},
};

int get_language(void);
void eat_switch_edge(int player, int edge);
int check_switch_edge(int player, int edge);

void set_u8_bit(unsigned char* bits, int num_bits, int bit, int value) {
    unsigned int idx;
    unsigned int mask;

    if (bit == -1) {
        return;
    }
    if (bit >= num_bits || bit < 0) {
        return;
    }
    idx = (unsigned int)bit >> 3;
    mask = 1U << ((unsigned int)bit & 7);
    if (value != 0) {
        bits[idx] = bits[idx] | mask;
    } else {
        bits[idx] = bits[idx] & ~mask;
    }
}

int get_u8_bit(unsigned char* bits, int num_bits, int bit) {
    unsigned int idx;
    unsigned int mask;

    if (bit == -1) {
        return 0;
    }
    if (bit >= num_bits || bit < 0) {
        return 0;
    }
    idx = (unsigned int)bit >> 3;
    mask = bit & 7;
    mask = 1U << mask;
    return (mask & bits[idx]) != 0;
}

const char* nbc_find_text(int index, int table) {
    int language;
    const char** strings;

    if (table < 0 || (unsigned int)table >= 2) {
        return nbc_empty_string;
    }
    if (index < 0 || index >= text_table_info[table].count) {
        return nbc_empty_string;
    }
    language = get_language();
    if (language == 5) {
        language = 0;
    }
    if (language < 0 || language >= 5) {
        return nbc_empty_string;
    }
    strings = text_table_info[table].strings;
    return (strings + index * 5)[language];
}

int nbc_get_language(void) {
    int language;

    language = get_language();
    if (language == 5) {
        language = 0;
    }
    return language;
}

void eat_switch_action(int player, int action) {
    if (player == -1) {
        return;
    }
    switch (action) {
    case 2:
        eat_switch_edge(player, 6);
        break;
    case 0:
        eat_switch_edge(player, 6);
        eat_switch_edge(player, 11);
        break;
    case 1:
        eat_switch_edge(player, 7);
        break;
    case 3:
        eat_switch_edge(player, 5);
        break;
    }
}

int check_switch_action(int player, int action) {
    int result;

    if (player == -1) {
        return 0;
    }
    switch (action) {
    case 2:
        result = check_switch_edge(player, 6);
        if (result != 0) {
            return 1;
        }
        break;
    case 0:
        if (check_switch_edge(player, 6) || check_switch_edge(player, 11)) {
            return 1;
        }
        break;
    case 1:
        result = check_switch_edge(player, 7);
        if (result != 0) {
            return 1;
        }
        break;
    case 3:
        result = check_switch_edge(player, 5);
        if (result != 0) {
            return 1;
        }
        break;
    }
    return 0;
}
