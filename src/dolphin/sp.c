#include "dolphin/sp.h"

void SPInitSoundTable(
    SPSoundTable* table, u32 aram_base, u32 zero_base) {
    int i;
    SPSoundEntry* entry;
    SPADPCM* adpcm;
    u32 aram_base_4;
    u32 aram_base_8;
    u32 aram_base_16;
    u32 zero_base_4;
    u32 zero_base_8;
    u32 zero_base_16;

    aram_base_4 = aram_base << 1;
    zero_base_4 = (zero_base << 1) + 2;
    aram_base_8 = aram_base;
    zero_base_8 = zero_base;
    aram_base_16 = aram_base >> 1;
    zero_base_16 = zero_base >> 1;
    entry = table->entries;
    adpcm = (SPADPCM*)&table->entries[table->entry_count];

    for (i = 0; i < table->entry_count; entry++, i++) {
        switch (entry->type) {
        case 0:
            entry->loop_address = zero_base_4;
            entry->loop_end_address = 0;
            entry->end_address =
                aram_base_4 + entry->end_address;
            entry->current_address =
                aram_base_4 + entry->current_address;
            entry->adpcm = adpcm;
            adpcm++;
            break;
        case 1:
            entry->loop_address =
                aram_base_4 + entry->loop_address;
            entry->loop_end_address =
                aram_base_4 + entry->loop_end_address;
            entry->end_address =
                aram_base_4 + entry->end_address;
            entry->current_address =
                aram_base_4 + entry->current_address;
            entry->adpcm = adpcm;
            adpcm++;
            break;
        case 2:
            entry->loop_address = zero_base_16;
            entry->loop_end_address = 0;
            entry->end_address =
                aram_base_16 + entry->end_address;
            entry->current_address =
                aram_base_16 + entry->current_address;
            break;
        case 3:
            entry->loop_address =
                aram_base_16 + entry->loop_address;
            entry->loop_end_address =
                aram_base_16 + entry->loop_end_address;
            entry->end_address =
                aram_base_16 + entry->end_address;
            entry->current_address =
                aram_base_16 + entry->current_address;
            break;
        case 4:
            entry->loop_address = zero_base_8;
            entry->loop_end_address = 0;
            entry->end_address =
                aram_base_8 + entry->end_address;
            entry->current_address =
                aram_base_8 + entry->current_address;
            break;
        case 5:
            entry->loop_address =
                aram_base_8 + entry->loop_address;
            entry->loop_end_address =
                aram_base_8 + entry->loop_end_address;
            entry->end_address =
                aram_base_8 + entry->end_address;
            entry->current_address =
                aram_base_8 + entry->current_address;
            break;
        }
    }
}

SPSoundEntry* SPGetSoundEntry(SPSoundTable* table, u32 index) {
    if (table->entry_count > index) {
        return &table->entries[index];
    }
    return 0;
}
