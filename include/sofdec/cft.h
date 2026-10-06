#ifndef SOFDEC_CFT_H
#define SOFDEC_CFT_H

#include "dolphin/types.h"

typedef struct CFTYcc420Planar {
    const u8* y;
    const u8* cb;
    const u8* cr;
    s32 y_stride;
    s32 cb_stride;
    s32 cr_stride;
} CFTYcc420Planar;

typedef struct CFTArgb8888Output {
    u8* data;
    s32 width;
    s32 height;
    s32 stride;
} CFTArgb8888Output;

typedef float CFTArgbTable[3][256][4];
typedef char CFTYcc420PlanarSizeCheck[sizeof(CFTYcc420Planar) == 0x18 ? 1 : -1];
typedef char CFTArgb8888OutputSizeCheck[sizeof(CFTArgb8888Output) == 0x10 ? 1 : -1];

void CFT_MakeYcc422ColAdjTbl(void* table);

#endif
