#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

typedef int (*RwRasterDeviceCall)(void* result, void* raster, int flags);
typedef int (*RwTextureRasterCall)(void* texture, void* raster, int flags);
typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwFreeListFreeCall)(void* freelist, void* entry);
typedef void (*RwStringCopyCall)(char* destination, const char* source,
                                 unsigned int size);
typedef char* (*RwStringConcatCall)(char* destination, const char* source);
typedef unsigned int (*RwStringLengthCall)(const char* string);

/** Confirmed portion of the retail RenderWare engine dispatch table. */
typedef struct PfxRwEngineInstance {
    char pad00[0x20];
    int (*fpRenderStateSet)(int state, int value); /* +0x20 */
    void (*fpRenderStateGet)(int state, void* out); /* +0x24 */
    char pad28[0x30];
    RwRasterDeviceCall fpRasterCreate; /* +0x58 */
    RwRasterDeviceCall fpRasterDestroy; /* +0x5C */
    char pad60[0x8];
    RwTextureRasterCall fpTextureSetRaster; /* +0x68 */
    char pad6C[0xC];
    RwRasterDeviceCall fpRasterSubRaster; /* +0x78 */
    char pad7C[0x8];
    RwRasterDeviceCall fpRasterLock; /* +0x84 */
    RwRasterDeviceCall fpRasterUnlock; /* +0x88 */
    char pad8C[0xC];
    RwRasterDeviceCall fpRasterShowRaster; /* +0x98 */
    char pad9C[0x8];
    RwRasterDeviceCall fpRasterLockPalette; /* +0xA4 */
    RwRasterDeviceCall fpRasterUnlockPalette; /* +0xA8 */
    char padAC[0xC];
    RwRasterDeviceCall fpRasterGetNumLevels; /* +0xB8 */
    char padBC[0x40];
    RwStringCopyCall fpStringCopy; /* +0xFC */
    RwStringConcatCall fpStringConcat; /* +0x100 */
    char pad104[0x1C];
    RwStringLengthCall fpStringLength; /* +0x120 */
    char pad124[0x14];
    void (*fpFree)(void* memory); /* +0x138 */
    char pad13C[0x8];
    RwFreeListAllocCall fpFreeListAlloc; /* +0x144 */
    RwFreeListFreeCall fpFreeListFree; /* +0x148 */
} PfxRwEngineInstance;

extern PfxRwEngineInstance* RwEngineInstance;

#endif
