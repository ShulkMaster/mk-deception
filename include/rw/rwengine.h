#ifndef RW_RWENGINE_H
#define RW_RWENGINE_H

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
typedef int (*RwIm2DRenderLineCall)(void* vertices, int numVertices,
                                    int vert1, int vert2);
typedef int (*RwIm2DRenderTriangleCall)(void* vertices, int numVertices,
                                        int vert1, int vert2, int vert3);
typedef int (*RwIm2DRenderPrimitiveCall)(int primitiveType, void* vertices,
                                         int numVertices);
typedef int (*RwIm2DRenderIndexedPrimitiveCall)(int primitiveType,
                                                void* vertices,
                                                int numVertices,
                                                unsigned short* indices,
                                                int numIndices);
typedef int (*RwIm3DRenderLineCall)(int vert1, int vert2);
typedef int (*RwIm3DRenderTriangleCall)(int vert1, int vert2, int vert3);
typedef int (*RwIm3DRenderPrimitiveCall)(int primitiveType);
typedef int (*RwIm3DRenderIndexedPrimitiveCall)(int primitiveType,
                                                unsigned short* indices,
                                                int numIndices);
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
    int (*fpRenderStateSet)(int state, int value);
    int (*fpRenderStateGet)(int state, void* out);
    RwIm2DRenderLineCall fpIm2DRenderLine;
    RwIm2DRenderTriangleCall fpIm2DRenderTriangle;
    RwIm2DRenderPrimitiveCall fpIm2DRenderPrimitive;
    RwIm2DRenderIndexedPrimitiveCall fpIm2DRenderIndexedPrimitive;
    RwIm3DRenderLineCall fpIm3DRenderLine;
    RwIm3DRenderTriangleCall fpIm3DRenderTriangle;
    RwIm3DRenderPrimitiveCall fpIm3DRenderPrimitive;
    RwIm3DRenderIndexedPrimitiveCall fpIm3DRenderIndexedPrimitive;
} RwDevice;

typedef enum RwStandardIndex {
    rwSTANDARDNASTANDARD = 0,
    rwSTANDARDCAMERABEGINUPDATE = 1,
    rwSTANDARDRGBTOPIXEL = 2,
    rwSTANDARDPIXELTORGB = 3,
    rwSTANDARDRASTERCREATE = 4,
    rwSTANDARDRASTERDESTROY = 5,
    rwSTANDARDIMAGEGETRASTER = 6,
    rwSTANDARDRASTERSETIMAGE = 7,
    rwSTANDARDTEXTURESETRASTER = 8,
    rwSTANDARDIMAGEFINDRASTERFORMAT = 9,
    rwSTANDARDCAMERAENDUPDATE = 10,
    rwSTANDARDSETRASTERCONTEXT = 11,
    rwSTANDARDRASTERSUBRASTER = 12,
    rwSTANDARDRASTERCLEARRECT = 13,
    rwSTANDARDRASTERCLEAR = 14,
    rwSTANDARDRASTERLOCK = 15,
    rwSTANDARDRASTERUNLOCK = 16,
    rwSTANDARDRASTERRENDER = 17,
    rwSTANDARDRASTERRENDERSCALED = 18,
    rwSTANDARDRASTERRENDERFAST = 19,
    rwSTANDARDRASTERSHOWRASTER = 20,
    rwSTANDARDCAMERACLEAR = 21,
    rwSTANDARDHINTRENDERF2B = 22,
    rwSTANDARDRASTERLOCKPALETTE = 23,
    rwSTANDARDRASTERUNLOCKPALETTE = 24,
    rwSTANDARDNATIVETEXTUREGETSIZE = 25,
    rwSTANDARDNATIVETEXTUREREAD = 26,
    rwSTANDARDNATIVETEXTUREWRITE = 27,
    rwSTANDARDRASTERGETMIPLEVELS = 28,
    rwSTANDARDNUMOFSTANDARD = 29
} RwStandardIndex;

typedef struct RwGlobals {
    void* curCamera;             /* +0x000 */
    void* curWorld;              /* +0x004 */
    unsigned short renderFrame;  /* +0x008 */
    unsigned short lightFrame;   /* +0x00A */
    unsigned short pad0C[2];     /* +0x00C: canonical longword alignment. */
    RwDevice dOpenDevice;                   /* +0x010 */
    RwStandardFunc stdFunc[rwSTANDARDNUMOFSTANDARD]; /* +0x048 */
    RwLinkList dirtyFrameList;              /* +0x0BC */
    RwFileFunctions fileFuncs;              /* +0x0C4 */
    RwStringFunctions stringFuncs;          /* +0x0F0 */
    void* (*fpMalloc)(unsigned long size, unsigned int hint); /* +0x134 */
    void (*fpFree)(void* memory);            /* +0x138 */
    void* (*fpRealloc)(void* memory, unsigned long size,
                       unsigned int hint);   /* +0x13C */
    void* (*fpCalloc)(unsigned long count, unsigned long size,
                      unsigned int hint);    /* +0x140 */
    RwFreeListAllocCall fpFreeListAlloc;     /* +0x144 */
    RwFreeListFreeCall fpFreeListFree;       /* +0x148 */
    void* metrics;                           /* +0x14C */
    int engineStatus;                        /* +0x150 */
    unsigned int resArenaInitSize;           /* +0x154 */
} RwGlobals;

typedef char RwFileFunctionsSizeCheck[
    sizeof(RwFileFunctions) == 0x2C ? 1 : -1];
typedef char RwStringFunctionsSizeCheck[
    sizeof(RwStringFunctions) == 0x44 ? 1 : -1];
typedef char RwDeviceSizeCheck[sizeof(RwDevice) == 0x38 ? 1 : -1];
typedef char RwGlobalsSizeCheck[sizeof(RwGlobals) == 0x158 ? 1 : -1];

extern RwGlobals* RwEngineInstance;

#define rwEngineStandardCall(type, standard) \
    ((type)RwEngineInstance->stdFunc[(standard)])

#endif
