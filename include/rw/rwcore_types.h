#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"
#include "rw/rtquat.h"

#define RW_OFFSET_OF(type, member) ((unsigned long)&((type*)0)->member)
#define RW_CONTAINER_OF(pointer, type, member)                              \
    ((type*)((unsigned char*)(pointer) - RW_OFFSET_OF(type, member)))

typedef RwObject* (*RwObjectCallBack)(RwObject* object, void* data);

typedef struct RwLLLink {
    struct RwLLLink* next;
    struct RwLLLink* prev;
} RwLLLink;

typedef struct RwLinkList {
    RwLLLink link;
} RwLinkList;

typedef struct RwRect {
    int x;
    int y;
    int w;
    int h;
} RwRect;

typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef RwObjectHasFrame* (*RwObjectHasFrameSyncFunction)(
    RwObjectHasFrame* object);

struct RwObjectHasFrame {
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrameSyncFunction sync;
};

typedef struct RwTexDictionary RwTexDictionary;


typedef struct RwImage {
    int flags;
    int width;
    int height;
    int depth;
    int stride;
    unsigned char* pixels;
    unsigned char* palette;
} RwImage;

typedef struct RwRGBAReal {
    float red;
    float green;
    float blue;
    float alpha;
} RwRGBAReal;

typedef struct RwRGBA {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} RwRGBA;


typedef struct RwRaster {
    struct RwRaster* parent;
    unsigned char* pixels;
    unsigned char* palette;
    int width;
    int height;
    int depth;
    int stride;
    short offsetX;
    short offsetY;
    unsigned char type;
    unsigned char flags;
    unsigned char privateFlags;
    unsigned char format;
    unsigned char* originalPixels;
    int originalWidth;
    int originalHeight;
    int originalStride;
} RwRaster;


typedef struct RwTexture {
    RwRaster* raster;
    RwTexDictionary* dictionary;
    RwLLLink lInDictionary;
    char name[32];
    char mask[32];
    unsigned int filter_flags;
    int ref_count;
} RwTexture;

typedef char RwTextureSizeCheck[sizeof(RwTexture) == 0x58 ? 1 : -1];

struct RwTexDictionary {
    RwObject object;
    RwLLLink textures;
    RwLLLink lInInstance;
};


typedef struct RwFrame {
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrix modelling;
    RwMatrix ltm;
    RwLinkList objectList;
    struct RwFrame* child;
    struct RwFrame* next;
    struct RwFrame* root;
} RwFrame;

typedef RwFrame* (*RwFrameCallBack)(RwFrame* frame, void* data);

#ifdef __cplusplus
extern "C" {
#endif

RwRaster* RwRasterCreate(int width, int height, int depth, int flags);
int RwRasterDestroy(RwRaster* raster);
RwRaster* RwRasterUnlock(RwRaster* raster);
int RwRasterGetNumLevels(RwRaster* raster);
void* RwRasterLock(RwRaster* raster, unsigned char level, int flags);
RwRaster* RwRasterShowRaster(RwRaster* raster, void* device,
                             unsigned int flags);

RwTexture* RwTextureCreate(RwRaster* raster);
int RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster);
RwTexture* RwTextureSetName(RwTexture* texture, const char* name);
RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif
