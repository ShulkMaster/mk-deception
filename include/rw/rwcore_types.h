#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"
#include "rw/rtquat.h"

/** Stock RenderWare doubly-linked list link. */
typedef struct RwLLLink {
    struct RwLLLink* next;
    struct RwLLLink* prev;
} RwLLLink;

typedef struct RwLinkList {
    RwLLLink link;
} RwLinkList;

#define rwLinkListAddLLLink(list, newLink)                                \
    ((newLink)->next = (list)->link.next, (newLink)->prev = &(list)->link, \
     ((list)->link.next)->prev = (newLink), (list)->link.next = (newLink))

#define rwLinkListRemoveLLLink(link) \
    (((link)->prev)->next = (link)->next, ((link)->next)->prev = (link)->prev)

typedef struct RwTexDictionary RwTexDictionary;

/* Stock RenderWare image layout used by RwImageCreate and ImageWriteTGA. */
typedef struct RwImage {
    int flags;                 /* +0x00 */
    int width;                 /* +0x04 */
    int height;                /* +0x08 */
    int depth;                 /* +0x0C */
    int stride;                /* +0x10 */
    unsigned char* pixels;     /* +0x14 */
    unsigned char* palette;    /* +0x18 */
} RwImage;

/** Stock RenderWare raster layout. Retail size: 0x34 bytes. */
typedef struct RwRaster {
    struct RwRaster* parent; /**< Retail offset 0x00. */
    unsigned char* pixels;   /**< Retail offset 0x04. */
    unsigned char* palette;  /**< Retail offset 0x08. */
    int width;               /**< Retail offset 0x0C. */
    int height;              /**< Retail offset 0x10. */
    int depth;               /**< Retail offset 0x14. */
    int stride;              /**< Retail offset 0x18. */
    short offsetX;           /**< Retail offset 0x1C. */
    short offsetY;           /**< Retail offset 0x1E. */
    unsigned char type;      /**< Retail offset 0x20. */
    unsigned char flags;     /**< Retail offset 0x21. */
    unsigned char privateFlags; /**< Retail offset 0x22. */
    unsigned char format;    /**< Retail offset 0x23. */
    unsigned char* originalPixels; /**< Retail offset 0x24. */
    int originalWidth;       /**< Retail offset 0x28. */
    int originalHeight;      /**< Retail offset 0x2C. */
    int originalStride;      /**< Retail offset 0x30. */
} RwRaster;

/** Stock RenderWare texture layout. Retail size: 0x58 bytes. */
typedef struct RwTexture {
    RwRaster* raster;          /**< Retail offset 0x00. */
    RwTexDictionary* dictionary; /**< Retail offset 0x04. */
    RwLLLink lInDictionary;       /**< Retail offset 0x08. */
    char name[32];             /**< Retail offset 0x10. */
    char mask[32];             /**< Retail offset 0x30. */
    unsigned int filter_flags; /**< Retail offset 0x50. */
    int ref_count;             /**< Retail offset 0x54. */
} RwTexture;

/** Stock RenderWare texture dictionary layout. Retail size: 0x18 bytes. */
struct RwTexDictionary {
    RwObject object;            /**< Retail offset 0x00. */
    RwLLLink textures;          /**< Retail offset 0x08. */
    RwLLLink lInInstance;       /**< Retail offset 0x10. */
};

/** Partial RenderWare frame layout. Known retail extent: 0xA4 bytes. */
typedef struct RwFrame {
    RwObject object;            /**< Retail offset 0x00. */
    RwLLLink inDirtyListLink;    /**< Retail offset 0x08. */
    RwMatrix modelling;         /**< Retail offset 0x10. */
    RwMatrix ltm;               /**< Retail offset 0x50. */
    RwLinkList objectList;       /**< Retail offset 0x90. */
    struct RwFrame* child;      /**< Retail offset 0x98. */
    struct RwFrame* next;       /**< Retail offset 0x9C. */
    struct RwFrame* root;       /**< Retail offset 0xA0. */
} RwFrame;

#ifdef __cplusplus
extern "C" {
#endif

RwRaster* RwRasterCreate(int width, int height, int depth, int flags);
int RwRasterDestroy(RwRaster* raster);
RwRaster* RwRasterUnlock(RwRaster* raster);
int RwRasterGetNumLevels(RwRaster* raster);
void* RwRasterLock(RwRaster* raster, unsigned char level, int flags);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image);
RwImage* RwImageFindRasterFormat(RwImage* image, int rasterType,
                                 int* width, int* height, int* depth,
                                 int* format);

RwTexture* RwTextureCreate(RwRaster* raster);
int RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureSetName(RwTexture* texture, const char* name);
RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster);
RwTexture* RwTextureRead(const char* name, const char* maskName);
RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif
