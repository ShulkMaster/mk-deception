#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

typedef int (*RwRasterDeviceCall)(void* result, void* raster, int flags);
typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwFreeListFreeCall)(void* freelist, void* entry);
typedef void (*RwStringCopyCall)(char* destination, const char* source,
                                 unsigned int size);
typedef unsigned int (*RwStringLengthCall)(const char* string);

/** Confirmed portion of the retail RenderWare engine dispatch table. */
typedef struct PfxRwEngineInstance {
    char pad00[0x20];
    int (*fpRenderStateSet)(int state, int value); /* +0x20 */
    void (*fpRenderStateGet)(int state, void* out); /* +0x24 */
    char pad28[0x30];
    RwRasterDeviceCall fpRasterCreate; /* +0x58 */
    char pad5C[0x28];
    RwRasterDeviceCall fpRasterLock; /* +0x84 */
    RwRasterDeviceCall fpRasterUnlock; /* +0x88 */
    char pad8C[0x2C];
    RwRasterDeviceCall fpRasterGetNumLevels; /* +0xB8 */
    char padBC[0x40];
    RwStringCopyCall fpStringCopy; /* +0xFC */
    char pad100[0x20];
    RwStringLengthCall fpStringLength; /* +0x120 */
    char pad124[0x20];
    RwFreeListAllocCall fpFreeListAlloc; /* +0x144 */
    RwFreeListFreeCall fpFreeListFree; /* +0x148 */
} PfxRwEngineInstance;

extern PfxRwEngineInstance* RwEngineInstance;

#endif
