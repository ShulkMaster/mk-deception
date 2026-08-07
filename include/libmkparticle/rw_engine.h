#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

#include "runtime/cstddef.h"
#include "rw/rwcore_types.h"

typedef int RwBool;
typedef char RwChar;

typedef RwBool (*rwFnFexist)(const RwChar* name);
typedef void* (*rwFnFopen)(const RwChar* name, const RwChar* mode);
typedef int (*rwFnFclose)(void* file);
typedef size_t (*rwFnFread)(void* address, size_t size, size_t count, void* file);
typedef size_t (*rwFnFwrite)(const void* address, size_t size, size_t count,
                             void* file);
typedef RwChar* (*rwFnFgets)(RwChar* buffer, int maxLength, void* file);
typedef int (*rwFnFputs)(const RwChar* buffer, void* file);
typedef int (*rwFnFeof)(void* file);
typedef int (*rwFnFseek)(void* file, long offset, int origin);
typedef int (*rwFnFflush)(void* file);
typedef int (*rwFnFtell)(void* file);

typedef struct RwFileFunctions {
    rwFnFexist rwfexist;
    rwFnFopen rwfopen;
    rwFnFclose rwfclose;
    rwFnFread rwfread;
    rwFnFwrite rwfwrite;
    rwFnFgets rwfgets;
    rwFnFputs rwfputs;
    rwFnFeof rwfeof;
    rwFnFseek rwfseek;
    rwFnFflush rwfflush;
    rwFnFtell rwftell;
} RwFileFunctions;

typedef int (*RwRasterDeviceCall)(void* result, void* raster, int flags);
typedef RwBool (*RwImageSetFromRasterCall)(RwImage* image, RwRaster* raster,
                                           RwInt32 flags);
typedef RwBool (*RwRasterSetFromImageCall)(RwRaster* raster, RwImage* image,
                                           RwInt32 flags);
typedef RwBool (*RwImageFindRasterFormatCall)(RwRaster* raster,
                                              RwImage* image,
                                              RwInt32 rasterType);
typedef int (*RwTextureRasterCall)(void* texture, void* raster, int flags);
typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwFreeListFreeCall)(void* freelist, void* entry);
typedef void* (*RwMemoryAllocCall)(unsigned int size, unsigned int hint);
typedef void (*RwStringCopyCall)(char* destination, const char* source,
                                 unsigned int size);
typedef char* (*RwStringConcatCall)(char* destination, const char* source);
typedef unsigned int (*RwStringLengthCall)(const char* string);

/** Confirmed portion of the retail RenderWare engine dispatch table. */
typedef struct RwGlobals {
    char pad00[0x20];
    int (*fpRenderStateSet)(int state, int value); /* +0x20 */
    void (*fpRenderStateGet)(int state, void* out); /* +0x24 */
    char pad28[0x30];
    RwRasterDeviceCall fpRasterCreate; /* +0x58 */
    RwRasterDeviceCall fpRasterDestroy; /* +0x5C */
    RwImageSetFromRasterCall fpImageSetFromRaster; /* +0x60 */
    RwRasterSetFromImageCall fpRasterSetFromImage; /* +0x64 */
    RwTextureRasterCall fpTextureSetRaster; /* +0x68 */
    RwImageFindRasterFormatCall fpImageFindRasterFormat; /* +0x6C */
    char pad70[0x8];
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
    RwLinkList dirtyFrameList; /* +0xBC */
    RwFileFunctions fileFuncs; /* +0xC4 */
    char padF0[0xC];
    RwStringCopyCall fpStringCopy; /* +0xFC */
    RwStringConcatCall fpStringConcat; /* +0x100 */
    char pad104[0x1C];
    RwStringLengthCall fpStringLength; /* +0x120 */
    char pad124[0x10];
    RwMemoryAllocCall fpMalloc; /* +0x134 */
    void (*fpFree)(void* memory); /* +0x138 */
    char pad13C[0x8];
    RwFreeListAllocCall fpFreeListAlloc; /* +0x144 */
    RwFreeListFreeCall fpFreeListFree; /* +0x148 */
    void* metrics; /* +0x14C */
    int engineStatus; /* +0x150 */
    unsigned int resArenaInitSize; /* +0x154 */
} RwGlobals;

typedef RwGlobals PfxRwEngineInstance;

extern RwGlobals* RwEngineInstance;

#endif
