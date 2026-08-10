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
typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwFreeListFreeCall)(void* freelist, void* entry);
typedef void (*RwStringCopyCall)(char* destination, const char* source,
                                 unsigned int size);
typedef unsigned int (*RwStringLengthCall)(const char* string);

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
    RwLinkList dirtyFrameList;
    RwFileFunctions fileFuncs;
    RwStringFunctions stringFuncs;
    void* (*fpMalloc)(unsigned int size, unsigned int hint);
    void (*fpFree)(void* memory);
    void* (*fpRealloc)(void* memory, unsigned int size, unsigned int hint);
    void* (*fpCalloc)(unsigned int count, unsigned int size,
                      unsigned int hint);
    RwFreeListAllocCall fpFreeListAlloc; /* +0x144 */
    RwFreeListFreeCall fpFreeListFree; /* +0x148 */
    void* metrics;
    int engineStatus;
    unsigned int resArenaInitSize;
} PfxRwEngineInstance;

extern PfxRwEngineInstance* RwEngineInstance;

#endif
