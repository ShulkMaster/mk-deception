#ifndef PLATFORM_GCMCARD_H
#define PLATFORM_GCMCARD_H

/*
 * Dolphin CARDFileInfo (retail open/create/write work object).
 * First two words overlay McCardParams (chan / fileNo).
 */
typedef struct CARDFileInfo {
    long chan; /* +0x00 */
    long fileNo; /* +0x04 */
    long offset; /* +0x08 */
    long length; /* +0x0C */
    unsigned short iBlock; /* +0x10 */
    unsigned short padding; /* +0x12 */
    unsigned char pad14[0x20 - 0x14]; /* +0x14 .. +0x1F */
} CARDFileInfo; /* 0x20 */

/*
 * gcmcard.o - Midway GC memcard facade (B20 + B21 PPWLS).
 * Campaign history: docs/campaigns/index.md (B20-B22)
 * Nintendo card.a Matching is out of campaign.
 */

int init_gc_memcard(void);
int update_storage_status(int flag);
const char* get_device_reference_name(int device);

int format_card_and_create_mkda_file(int device);
int gc_format_procedure(int device);
int gc_delete_file(int device, const char* fileName);

int load_from_memcard2(int device, int modeFlag, unsigned int offset, char* unusedStr,
                       char* fileName, void* buffer, int size, char* unusedCardName,
                       int unusedNameLen, unsigned int* freeBlocks, int* freeBytes,
                       int* checksumFailOut);
int save_to_memcard2(int device, int modeFlag, unsigned int offset, int createFlag, char* unusedStr,
                     char* fileName, unsigned char* buffer, int size, unsigned int* freeBlocks,
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
extern int mcmasks[2];

#endif
