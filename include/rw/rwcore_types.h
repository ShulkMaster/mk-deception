#ifndef RW_RWCORE_TYPES_H
#define RW_RWCORE_TYPES_H

#include "rw/rwobject.h"

/** Partial RenderWare raster view; known retail extent: 0x24 bytes. */
typedef struct RwRaster {
    char pad00[0x0C];      /**< Retail offsets 0x00-0x0B; fields unknown. */
    int width;             /**< Retail offset 0x0C. */
    int height;            /**< Retail offset 0x10. */
    unsigned int logSize;  /**< Retail offset 0x14. */
    char pad18[0x0B];      /**< Retail offsets 0x18-0x22; fields unknown. */
    unsigned char flags;   /**< Retail offset 0x23. */
} RwRaster;

/** RenderWare texture with Midway ownership extension. Retail layout: 0x58 bytes. */
typedef struct RwTexture {
    RwRaster* raster;          /**< Retail offset 0x00. */
    char pad04[0x0C];          /**< Retail offsets 0x04-0x0F; fields unknown. */
    char name[32];             /**< Retail offset 0x10. */
    char mask[32];             /**< Retail offset 0x30. */
    unsigned int filter_flags; /**< Retail offset 0x50. */
    unsigned int pin_flag;     /**< Retail offset 0x54; Midway ownership/pin state. */
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
