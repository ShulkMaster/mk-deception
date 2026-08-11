#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

#include "runtime/cstdarg.h"
#include "rw/rwcore_types.h"

typedef int (*RwFileExistsCall)(const char* name);
typedef void* (*RwFileOpenCall)(const char* name, const char* mode);
typedef int (*RwFileCloseCall)(void* file);
typedef unsigned int (*RwFileReadCall)(void* address, unsigned int size,
                                       unsigned int count, void* file);
typedef unsigned int (*RwFileWriteCall)(const void* address,
                                        unsigned int size,
                                        unsigned int count, void* file);
typedef char* (*RwFileGetsCall)(char* buffer, int maxLength, void* file);
typedef int (*RwFilePutsCall)(const char* buffer, void* file);
typedef int (*RwFileEofCall)(void* file);
typedef int (*RwFileSeekCall)(void* file, long offset, int origin);
typedef int (*RwFileFlushCall)(void* file);
typedef int (*RwFileTellCall)(void* file);

typedef struct RwFileFunctions {
    RwFileExistsCall exists;
    RwFileOpenCall open;
    RwFileCloseCall close;
    RwFileReadCall read;
    RwFileWriteCall write;
    RwFileGetsCall gets;
    RwFilePutsCall puts;
    RwFileEofCall eof;
    RwFileSeekCall seek;
    RwFileFlushCall flush;
    RwFileTellCall tell;
} RwFileFunctions;

typedef struct RwStringFunctions {
    int (*sprintf)(char*, const char*, ...);
    int (*vsprintf)(char*, const char*, __va_list);
    char* (*strcpy)(char*, const char*);
    char* (*strncpy)(char*, const char*, unsigned long);
    char* (*strcat)(char*, const char*);
    char* (*strncat)(char*, const char*, unsigned long);
    char* (*strrchr)(const char*, int);
    char* (*strchr)(const char*, int);
    char* (*strstr)(const char*, const char*);
    int (*strcmp)(const char*, const char*);
    int (*strncmp)(const char*, const char*, unsigned long);
    int (*stricmp)(const char*, const char*);
    unsigned long (*strlen)(const char*);
    void (*strupr)(char*);
    void (*strlwr)(char*);
    char* (*strtok)(char*, const char*);
    int (*sscanf)(const char*, const char*, ...);
} RwStringFunctions;

typedef int (*RwRasterDeviceCall)(void* result, void* raster, int flags);
typedef int (*RwSystemCall)(int request, void* out, void* in_out, int value);
typedef int (*RwStandardCall)(void* out, void* in_out, int value);
typedef int (*RwCameraDeviceCall)(void* out, void* camera, int value);
typedef int (*RwCameraClearCall)(void* camera, void* color, int clear_mode);
typedef RwSystemCall RwSystemFunc;
typedef RwStandardCall RwStandardFunc;
typedef struct RwFreeList RwFreeList;
typedef void* (*RwFreeListAllocCall)(RwFreeList* freelist, unsigned int hint);
typedef RwFreeList* (*RwFreeListFreeCall)(RwFreeList* freelist, void* entry);
typedef void (*RwStringCopyCall)(char* destination, const char* source,
                                 unsigned int size);
typedef unsigned int (*RwStringLengthCall)(const char* string);

typedef struct RwDevice {
    float gammaCorrection;
    RwSystemCall fpSystem;
    float zBufferNear;
    float zBufferFar;
    RwStandardCall standard[29];
} RwDevice;

typedef struct RwGlobals {
    union {
        void* field_0x00;
        void* curCamera;
    };
    union {
        void* field_0x04;
        void* curWorld;
    };
    union {
        unsigned short field_0x08;
        unsigned short renderFrame;
    };
    union {
        unsigned short field_0x0A;
        unsigned short lightFrame;
    };
    unsigned int field_0x0C;
    float gammaCorrection;
    RwSystemCall fpSystem;
    float zBufferNear;
    float zBufferFar;
    int (*fpRenderStateSet)(int state, int value);
    void (*fpRenderStateGet)(int state, void* out);
    char pad28[0x24];
    union {
        void* field_0x4C;
        RwCameraDeviceCall fpCameraBeginUpdate;
    };
    char pad50[0x8];
    RwRasterDeviceCall fpRasterCreate;
    RwRasterDeviceCall fpRasterDestroy;
    RwRasterDeviceCall fpImageSetFromRaster;
    RwRasterDeviceCall fpRasterSetFromImage;
    RwRasterDeviceCall fpTextureSetRaster;
    RwRasterDeviceCall fpImageFindRasterFormat;
    union {
        void* field_0x70;
        RwCameraDeviceCall fpCameraEndUpdate;
    };
    void* field_0x74;
    RwRasterDeviceCall fpRasterSubRaster;
    char pad7C[0x8];
    RwRasterDeviceCall fpRasterLock;
    RwRasterDeviceCall fpRasterUnlock;
    char pad8C[0xC];
    RwRasterDeviceCall fpRasterShowRaster;
    union {
        void* field_0x9C;
        RwCameraClearCall fpCameraClear;
    };
    union {
        void* field_0xA0;
        RwStandardCall fpHintRenderFrontToBack;
    };
    RwRasterDeviceCall fpRasterLockPalette;
    RwRasterDeviceCall fpRasterUnlockPalette;
    char padAC[0xC];
    RwRasterDeviceCall fpRasterGetNumLevels;
    RwLinkList dirtyFrameList;
    RwFileFunctions fileFuncs;
    RwStringFunctions stringFuncs;
    void* (*fpMalloc)(unsigned int size, unsigned int hint);
    void (*fpFree)(void* memory);
    void* (*fpRealloc)(void* memory, unsigned int size, unsigned int hint);
    void* (*fpCalloc)(unsigned int count, unsigned int size,
                      unsigned int hint);
    RwFreeListAllocCall fpFreeListAlloc;
    RwFreeListFreeCall fpFreeListFree;
    void* metrics;
    int engineStatus;
    unsigned int resArenaInitSize;
} RwGlobals;

typedef RwGlobals PfxRwEngineInstance;

extern RwGlobals* RwEngineInstance;

#endif
