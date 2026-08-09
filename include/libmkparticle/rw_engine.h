#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

#include "runtime/cstddef.h"
#include "runtime/cstdarg.h"
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
typedef struct RwFreeList RwFreeList;
typedef void* (*RwFreeListAllocCall)(RwFreeList* freelist, unsigned int hint);
typedef RwFreeList* (*RwFreeListFreeCall)(RwFreeList* freelist, void* entry);
typedef void* (*RwMemoryAllocCall)(unsigned int size, unsigned int hint);
typedef void* (*RwMemoryReallocCall)(void* memory, unsigned int size,
                                     unsigned int hint);
typedef void* (*RwMemoryCallocCall)(unsigned int count, unsigned int size,
                                    unsigned int hint);
typedef RwBool (*RwSystemFunc)(RwInt32 option, void* out, void* inOut,
                               RwInt32 in);
typedef RwBool (*RwStandardFunc)(void* out, void* inOut, RwInt32 in);
typedef RwBool (*RwCameraDeviceCall)(void* out, RwCamera* camera, RwInt32 in);
typedef RwBool (*RwCameraClearCall)(RwCamera* camera, RwRGBA* color,
                                    RwInt32 clearMode);
typedef struct RwDevice {
    RwReal gammaCorrection;
    RwSystemFunc fpSystem;
    RwReal zBufferNear;
    RwReal zBufferFar;
    RwStandardFunc standard[29];
} RwDevice;
#ifndef RW_MEMORY_FUNCTIONS_DEFINED
#define RW_MEMORY_FUNCTIONS_DEFINED
typedef struct RwMemoryFunctions {
    RwMemoryAllocCall alloc;
    void (*free)(void* memory);
    RwMemoryReallocCall realloc;
    RwMemoryCallocCall calloc;
} RwMemoryFunctions;
#endif
typedef struct RwStringFunctions {
    int (*vecSprintf)(RwChar*, const RwChar*, ...);
    int (*vecVsprintf)(RwChar*, const RwChar*, __va_list);
    RwChar* (*vecStrcpy)(RwChar*, const RwChar*);
    RwChar* (*vecStrncpy)(RwChar*, const RwChar*, size_t);
    RwChar* (*vecStrcat)(RwChar*, const RwChar*);
    RwChar* (*vecStrncat)(RwChar*, const RwChar*, size_t);
    RwChar* (*vecStrrchr)(const RwChar*, int);
    RwChar* (*vecStrchr)(const RwChar*, int);
    RwChar* (*vecStrstr)(const RwChar*, const RwChar*);
    int (*vecStrcmp)(const RwChar*, const RwChar*);
    int (*vecStrncmp)(const RwChar*, const RwChar*, size_t);
    int (*vecStricmp)(const RwChar*, const RwChar*);
    size_t (*vecStrlen)(const RwChar*);
    void (*vecStrupr)(RwChar*);
    void (*vecStrlwr)(RwChar*);
    RwChar* (*vecStrtok)(RwChar*, const RwChar*);
    int (*vecSscanf)(const RwChar*, const RwChar*, ...);
} RwStringFunctions;

/** Confirmed portion of the retail RenderWare engine dispatch table. */
typedef struct RwGlobals {
    void* curCamera; /* +0x00 */
    void* curWorld; /* +0x04 */
    RwUInt16 renderFrame; /* +0x08 */
    RwUInt16 lightFrame; /* +0x0A */
    RwUInt16 pad0C[2];
    RwReal gammaCorrection; /* +0x10, first field of the open device */
    RwSystemFunc fpSystem; /* +0x14 */
    RwReal zBufferNear; /* +0x18 */
    RwReal zBufferFar; /* +0x1C */
    int (*fpRenderStateSet)(int state, int value); /* +0x20 */
    void (*fpRenderStateGet)(int state, void* out); /* +0x24 */
    char pad28[0x24];
    RwCameraDeviceCall fpCameraBeginUpdate; /* +0x4C */
    char pad50[0x8];
    RwRasterDeviceCall fpRasterCreate; /* +0x58 */
    RwRasterDeviceCall fpRasterDestroy; /* +0x5C */
    RwImageSetFromRasterCall fpImageSetFromRaster; /* +0x60 */
    RwRasterSetFromImageCall fpRasterSetFromImage; /* +0x64 */
    RwTextureRasterCall fpTextureSetRaster; /* +0x68 */
    RwImageFindRasterFormatCall fpImageFindRasterFormat; /* +0x6C */
    RwCameraDeviceCall fpCameraEndUpdate; /* +0x70 */
    char pad74[0x4];
    RwRasterDeviceCall fpRasterSubRaster; /* +0x78 */
    char pad7C[0x8];
    RwRasterDeviceCall fpRasterLock; /* +0x84 */
    RwRasterDeviceCall fpRasterUnlock; /* +0x88 */
    char pad8C[0xC];
    RwRasterDeviceCall fpRasterShowRaster; /* +0x98 */
    RwCameraClearCall fpCameraClear; /* +0x9C */
    RwStandardFunc fpHintRenderFrontToBack; /* +0xA0 */
    RwRasterDeviceCall fpRasterLockPalette; /* +0xA4 */
    RwRasterDeviceCall fpRasterUnlockPalette; /* +0xA8 */
    char padAC[0xC];
    RwRasterDeviceCall fpRasterGetNumLevels; /* +0xB8 */
    RwLinkList dirtyFrameList; /* +0xBC */
    RwFileFunctions fileFuncs; /* +0xC4 */
    RwStringFunctions stringFuncs; /* +0xF0 */
    RwMemoryAllocCall fpMalloc; /* +0x134 */
    void (*fpFree)(void* memory); /* +0x138 */
    RwMemoryReallocCall fpRealloc; /* +0x13C */
    RwMemoryCallocCall fpCalloc; /* +0x140 */
    RwFreeListAllocCall fpFreeListAlloc; /* +0x144 */
    RwFreeListFreeCall fpFreeListFree; /* +0x148 */
    void* metrics; /* +0x14C */
    int engineStatus; /* +0x150 */
    unsigned int resArenaInitSize; /* +0x154 */
} RwGlobals;

typedef RwGlobals PfxRwEngineInstance;

extern RwGlobals* RwEngineInstance;

#endif
