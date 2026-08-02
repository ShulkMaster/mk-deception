#ifndef PLATFORM_GCMCICON_H
#define PLATFORM_GCMCICON_H

#include "platform/gcmcard.h"

typedef struct CARDStat {
    char fileName[32]; /* +0x00 */
    unsigned long length; /* +0x20 */
    unsigned long time; /* +0x24 */
    char gameName[4]; /* +0x28 */
    char company[2]; /* +0x2C */
    unsigned char bannerFormat; /* +0x2E */
    unsigned char padding; /* +0x2F */
    unsigned long iconAddr; /* +0x30 */
    unsigned short iconFormat; /* +0x34 */
    unsigned short iconSpeed; /* +0x36 */
    unsigned long commentAddr; /* +0x38 */
    unsigned long offsetBanner; /* +0x3C */
    unsigned long offsetBannerTlut; /* +0x40 */
    unsigned long offsetIcon[8]; /* +0x44 */
    unsigned long offsetIconTlut; /* +0x64 */
    unsigned long offsetData; /* +0x68 */
} CARDStat; /* 0x6C */

void unload_memorycard_write_buffer(void);
int create_memorycard_write_buffer(const void* data, unsigned int size);
void load_icon_data(void);
int update_memory_card_status(const CARDFileInfo* file);

#endif
