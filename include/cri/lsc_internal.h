#ifndef CRI_LSC_INTERNAL_H
#define CRI_LSC_INTERNAL_H

#include "cri/sj.h"
#include "dolphin/types.h"

typedef struct ADXStream ADXStream;

typedef struct LSCStreamInfo {
    s32 id;
    const char* filename;
    u32 filename_checksum;
    void* directory;
    s32 offset;
    s32 sector_count;
    s32 state;
    s32 read_sectors;
} LSCStreamInfo;

typedef struct LSCObject {
    s8 used;
    s8 state;
    s8 reading;
    s8 loop;
    s8 paused;
    u8 reserved_05[3];
    SJ* sj;
    s32 reserved_0C;
    s32 reserved_10;
    s32 minimum_buffer_size;
    s32 buffer_size;
    s32 write_position;
    s32 read_position;
    s32 stream_count;
    ADXStream* stream;
    s32 file_sectors;
    s32 reserved_30;
    s32 reserved_34;
    LSCStreamInfo stream_info[16];
} LSCObject;

typedef char LSCStreamInfoSizeCheck[sizeof(LSCStreamInfo) == 0x20 ? 1 : -1];
typedef char LSCObjectSizeCheck[sizeof(LSCObject) == 0x238 ? 1 : -1];

#endif
