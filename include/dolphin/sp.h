#ifndef DOLPHIN_SP_H
#define DOLPHIN_SP_H

typedef unsigned long u32;

typedef struct SPADPCM {
    unsigned char state[0x2E];
} SPADPCM;

typedef struct SPSoundEntry {
    u32 type;                 /* +0x00 */
    u32 sample_rate;          /* +0x04 */
    u32 loop_address;         /* +0x08 */
    u32 loop_end_address;     /* +0x0C */
    u32 end_address;          /* +0x10 */
    u32 current_address;      /* +0x14 */
    SPADPCM* adpcm;           /* +0x18 */
} SPSoundEntry; /* 0x1C */

typedef struct SPSoundTable {
    u32 entry_count;          /* +0x00 */
    SPSoundEntry entries[1];  /* +0x04 */
} SPSoundTable;

#ifdef __cplusplus
extern "C" {
#endif

void SPInitSoundTable(
    SPSoundTable* table, u32 aram_base, u32 zero_base);
SPSoundEntry* SPGetSoundEntry(SPSoundTable* table, u32 index);

#ifdef __cplusplus
}
#endif

#endif
