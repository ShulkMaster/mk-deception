#ifndef MWSCREENENGINE_TEXTURE_COLLECTION_H
#define MWSCREENENGINE_TEXTURE_COLLECTION_H

typedef struct RwTexture RwTexture;

/* Two parallel output arrays passed by value to retail texture-list helpers. */
typedef struct GVTexturePair {
    RwTexture** colors; /* +0x00 */
    RwTexture** alphas; /* +0x04 */
} GVTexturePair;

/* ScreenUtil allocation header followed by color and alpha pointer arrays. */
typedef struct GVTextureCollection {
    unsigned char pad00[0x08];
    RwTexture** colors; /* +0x08 */
    RwTexture** alphas; /* +0x0C */
    unsigned char pad10[0x04];
    unsigned int count; /* +0x14 */
    unsigned int refreshFlag; /* +0x18 */
    unsigned int reloadFlag; /* +0x1C */
    unsigned char pad20[0x10];
} GVTextureCollection;

/* Embedded two-word Get/FreeTextureCollection state used by ImageList. */
typedef struct GMTextureInfo_t {
    GVTextureCollection* data; /* +0x00 */
    unsigned int ready; /* +0x04 */
} GMTextureInfo_t;

#endif
