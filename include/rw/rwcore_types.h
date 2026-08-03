#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"
#include "rw/rtquat.h"

/** Stock RenderWare doubly-linked list link. */
typedef struct RwLLLink {
    struct RwLLLink* next;
    struct RwLLLink* prev;
} RwLLLink;

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

/** RenderWare raster prefix used by the retail core. Retail layout: 0x24 bytes. */
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
} RwRaster;

/** RenderWare texture with Midway ownership extension. Retail layout: 0x58 bytes. */
typedef struct RwTexture {
    RwRaster* raster;          /**< Retail offset 0x00. */
    RwTexDictionary* dictionary; /**< Retail offset 0x04. */
    RwLLLink* next_link;          /**< Retail offset 0x08. */
    RwLLLink* prev_link;          /**< Retail offset 0x0C. */
    char name[32];             /**< Retail offset 0x10. */
    char mask[32];             /**< Retail offset 0x30. */
    unsigned int filter_flags; /**< Retail offset 0x50. */
    int ref_count;             /**< Retail offset 0x54. */
} RwTexture;

/** Partial RenderWare frame layout. Known retail extent: 0xA4 bytes. */
typedef struct RwFrame {
    RwObject object;            /**< Retail offset 0x00. */
    void* object_link_next;     /**< Retail offset 0x08. */
    void* object_link_prev;     /**< Retail offset 0x0C. */
    RwMatrix modelling;         /**< Retail offset 0x10. */
    RwMatrix ltm;               /**< Retail offset 0x50. */
    void* object_list_next;     /**< Retail offset 0x90. */
    void* object_list_prev;     /**< Retail offset 0x94. */
    struct RwFrame* child;      /**< Retail offset 0x98. */
    struct RwFrame* next;       /**< Retail offset 0x9C. */
    struct RwFrame* root;       /**< Retail offset 0xA0. */
} RwFrame;

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
void RwTexDictionaryRemoveTexture(RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif
