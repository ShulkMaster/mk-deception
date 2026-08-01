#ifndef LIBMKPARTICLE_PFX2D_H
#define LIBMKPARTICLE_PFX2D_H

/*
 * Midway 2D particle/sprite draw (libmkparticle pfx2d.o).
 * Pool of 500 objects (0xD0 each) used by image.render_2d_objs / ScreenObj.
 *
 * Retail: image.render_2d_objs(layer)
 *   -> pfx2d_begin_render -> pfx2d_render(obj) -> pfx2d_end_render
 *   -> native2d_instance_geometry / native2d_draw (gc_2d).
 * pfx2d_build_default_geometry requires RwTexture + raster (tex_w/h).
 * load_*_2d_pfxobj* return NULL when TGA fails (no ScreenObj entry).
 *
 * Soft ceilings: build_default_geometry ~98.3%, end_render ~92.8% -- stop.
 * Matched: init, free, alloc, begin_render, render, get_initialized.
 */

typedef struct RwTexture RwTexture;

typedef struct Pfx2dVert {
    float x;
    float y;
    float u;
    float v;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Pfx2dVert; /* 0x14 */

typedef struct Pfx2dGpuVtx {
    /* +0x00 -- word or per-channel RGBA (gc_2d instance_geometry). */
    union {
        unsigned int color;
        unsigned char rgba[4];
    };
    float u;  /* +0x04 */
    float v;  /* +0x08 */
    short x;  /* +0x0C */
    short y;  /* +0x0E */
} Pfx2dGpuVtx; /* 0x10 */

/* Retail object size 0xD0. */
typedef struct Pfx2dObj {
    Pfx2dVert verts[4];       /* +0x00 */
    int mirror;               /* +0x50 */
    int tex_w;                /* +0x54 */
    int tex_h;                /* +0x58 */
    int x;                    /* +0x5C */
    int y;                    /* +0x60 */
    float scale_x;            /* +0x64 */
    float scale_y;            /* +0x68 */
    int src_blend;            /* +0x6C */
    int dst_blend;            /* +0x70 */
    Pfx2dGpuVtx gpu[4];       /* +0x74 */
    RwTexture* alpha_texture; /* +0xB4 */
    char padB8[0x10];
    RwTexture* texture; /* +0xC8 */
    int pool_index;     /* +0xCC */
} Pfx2dObj;

#define PFX2D_POOL_SIZE 0x1F4

void pfx2d_init(void);
Pfx2dObj* pfx2d_alloc_obj(void);
void pfx2d_free_obj(Pfx2dObj* obj);
void pfx2d_build_default_geometry(Pfx2dObj* obj);
void pfx2d_begin_render(void);
void pfx2d_end_render(void);
void pfx2d_render(Pfx2dObj* obj);

#endif
