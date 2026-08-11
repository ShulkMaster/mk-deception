#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"
#include "rw/rtquat.h"

typedef RwObject* (*RwObjectCallBack)(RwObject* object, void* data);

typedef struct RwLLLink {
    struct RwLLLink* next;
    struct RwLLLink* prev;
} RwLLLink;

typedef struct RwLinkList {
    RwLLLink link;
} RwLinkList;

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
    RwUInt8 red;
    RwUInt8 green;
    RwUInt8 blue;
    RwUInt8 alpha;
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
    union {
        RwLLLink lInDictionary;
        struct {
            RwLLLink* next_link;
            RwLLLink* prev_link;
        };
    };
    char name[32];
    char mask[32];
    unsigned int filter_flags;
    int ref_count;
} RwTexture;

struct RwTexDictionary {
    RwObject object;
    RwLLLink textures;
    RwLLLink lInInstance;
};


typedef struct RwFrame {
    RwObject object;
    union {
        RwLLLink inDirtyListLink;
        struct {
            void* object_link_next;
            void* object_link_prev;
        };
    };
    RwMatrix modelling;
    RwMatrix ltm;
    union {
        RwLinkList objectList;
        struct {
            void* object_list_next;
            void* object_list_prev;
        };
    };
    struct RwFrame* child;
    struct RwFrame* next;
    struct RwFrame* root;
} RwFrame;

typedef RwFrame* (*RwFrameCallBack)(RwFrame* frame, void* data);

#ifdef __cplusplus
extern "C" {
#endif

RwRaster* RwRasterCreate(int width, int height, int depth, int flags);
RwRaster* RwRasterUnlock(RwRaster* raster);
int RwRasterGetNumLevels(RwRaster* raster);
void* RwRasterLock(RwRaster* raster, unsigned char level, int flags);

RwTexture* RwTextureCreate(RwRaster* raster);
int RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureSetName(RwTexture* texture, const char* name);
RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif
