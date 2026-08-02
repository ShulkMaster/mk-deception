#ifndef MWSCREENENGINE_SCREEN_POLY_H
#define MWSCREENENGINE_SCREEN_POLY_H

/*
 * ScreenPoly -- retail runtime node (sizeof 0x7C) from CreatePoly.
 *
 * Disc blob = SEPolyElement_t (ScreenObject.h); do not widen SeRef in-place.
 * Layout matches GC Matching + Glue Render.
 *
 * Present (retail Glue): ScreenPoly::Render -> load_2d_pfxobj_with_texture
 *   oid 0x900B -> pfx2d_begin/render (gc_2d FIFO).
 * CreatePoly verts: Y = 480 - pos.y; V = 1 - uv.v; offsets init 0.
 */

typedef struct RwTexture RwTexture;
typedef struct RwFrame RwFrame;
typedef struct ScreenObj ScreenObj;
typedef struct ScreenMatrixStackC ScreenMatrixStackC;
typedef struct ScreenPoly ScreenPoly;

/* C views shared by the Glue implementations of ScreenPoly/Text/Particle. */
typedef struct ScreenRenderInfoC {
    unsigned int flags;        /* +0x00 */
    ScreenMatrixStackC* matrixStack; /* +0x04 */
    float colorScale[4];       /* +0x08 */
    float colorTranslation[4]; /* +0x18 */
} ScreenRenderInfoC; /* 0x28 */

struct ScreenMatrixStackC {
    void* vtbl;  /* +0x00 */
    RwFrame* frame; /* +0x04 -- passed to RwFrameGetLTM */
}; /* 0x08 */

/* Patched POLY element from the screen-set blob (retail ILP32). */
typedef struct SEPolyPosition {
    float x;
    float y;
    float z;
} SEPolyPosition; /* 0x0C */

typedef struct SEPoly_t {
    unsigned int typeTag;        /* +0x00 -- 'POLY' */
    int reserved04;              /* +0x04 */
    ScreenPoly* liveObject;      /* +0x08 -- after instancing */
    unsigned int flags;          /* +0x0C -- bit0 selects linear filtering */
    SEPolyPosition positions[4]; /* +0x10 */
    unsigned char colors[4][4];  /* +0x40 */
    float uvs[4][2];             /* +0x50 */
    char* textureString;          /* +0x70 -- patched SeRef */
} SEPoly_t; /* 0x74 */

typedef struct ScreenPolyVert {
    float x; /* +0x00 */
    float y; /* +0x04 */
    float u; /* +0x08 */
    float v; /* +0x0C */
    unsigned char rgba[4]; /* +0x10 */
} ScreenPolyVert; /* 0x14 */

typedef struct ScreenPolyFilterBits {
    unsigned char hidden : 1; /* bit7 */
    unsigned char linear : 1; /* bit6 */
    unsigned char pad : 6;
} ScreenPolyFilterBits;

struct ScreenPoly {
    void* vtbl; /* +0x00 ScreenNode */
    unsigned int flags; /* +0x04 */
    unsigned char pad08[8]; /* +0x08 */
    ScreenObj* screenObj; /* +0x10 -- live pfx chrome */
    int screenObjInstance; /* +0x14 */
    ScreenPolyVert verts[4]; /* +0x18 -- stride 0x14 */
    float offsetX; /* +0x68 */
    float offsetY; /* +0x6C */
    RwTexture* colorTex; /* +0x70 */
    RwTexture* alphaTex; /* +0x74 */
    unsigned char filterFlags; /* +0x78 -- bit7 hide; bit6 linear */
    unsigned char pad79[3];
}; /* 0x7C */

/* Retail vert index remap used by CreatePoly. */
extern int vert_map__10ScreenPoly[4];

#endif
