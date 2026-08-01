#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"

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
    void* dictionary;          /**< Retail offset 0x04. */
    struct RwTexture** next_link; /**< Retail offset 0x08. */
    struct RwTexture** prev_link; /**< Retail offset 0x0C. */
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
    float ltm[16];              /**< Retail offset 0x10: modelling matrix. */
    char pad50[0x40];           /**< Retail offsets 0x50-0x8F; fields unknown. */
    void* object_list_next;     /**< Retail offset 0x90. */
    void* object_list_prev;     /**< Retail offset 0x94. */
    struct RwFrame* child;      /**< Retail offset 0x98. */
    struct RwFrame* next;       /**< Retail offset 0x9C. */
    struct RwFrame* root;       /**< Retail offset 0xA0. */
} RwFrame;

#endif
