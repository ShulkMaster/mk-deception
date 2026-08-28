#ifndef PLATFORM_GCMCARD_H
#define PLATFORM_GCMCARD_H

#include "dolphin/card.h"

/*
 * gcmcard.o - Midway GC memcard facade (B20 + B21 PPWLS).
 * Campaign history: docs/campaigns/index.md (B20-B22)
 * Uses the canonical Dolphin CARDFileInfo shared with card.a.
 */

int init_gc_memcard(void);
int update_storage_status(int flag);
const char* get_device_reference_name(int device);

int format_card_and_create_mkda_file(int device);
int gc_format_procedure(int device);
int gc_delete_file(int device, const char* fileName);

int load_from_memcard2(int device, int modeFlag, unsigned int offset, const char* unusedStr,
                       const char* fileName, void* buffer, int size, const char* unusedCardName,
                       int unusedNameLen, unsigned int* freeBlocks, int* freeBytes,
                       int* checksumFailOut);
int save_to_memcard2(int device, int modeFlag, unsigned int offset, int createFlag,
                     const char* unusedStr, const char* fileName, void* buffer, int size,
                     unsigned int* freeBlocks,
                     int* freeBytes, int skipChecksum, int unused0, int unusedMode, int unused1);

int check_load_profile_result(int* result, int device);
int check_load_region_data_result(int* result, int device, int scratch, int flag);
int bad_load_region_data_result_resolution(int* result, int device);
int check_save_profile_result(int* result, int device, int flag);
int check_save_region_data_result(int* result, int device, int mode);
int bad_save_region_data_result_resolution(int* result, int device);

extern int gc_seek_position;
extern int force_insertions;
extern int force_removals;
/* const: retail places mcmasks in .sdata2 (r2-relative); non-const lands in
 * .sdata and flips the base register on every access. */
extern const int mcmasks[2];

#endif
