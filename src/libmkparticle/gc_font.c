#include "libmkparticle/gc_font.h"
#include "libmkparticle/gc_state.h"
#include "libmkparticle/texture_bridge.h"
#include "math/gxQuat.h"

typedef struct GXColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GXColor;

void GXClearVtxDesc(void);
void GXSetVtxAttrFmt(int vtxfmt, int attr, int cnt, int type, int frac);
void GXSetVtxDesc(int attr, int type);
void GXSetChanMatColor(int chan, GXColor color);
void GXSetChanCtrl(int chan, int enable, int ambSrc, int matSrc, int lightMask, int diffFn,
                   int attnFn);
void GXSetNumChans(int n);
void GXLoadPosMtxImm(Mtx m, int id);
void GXCallDisplayList(void* list, unsigned long nbytes);
void GXBeginDisplayList(void* list, unsigned long nbytes);
unsigned long GXEndDisplayList(void);
void GXBegin(int primitive, int vtxfmt, int nverts);
void GXResetWriteGatherPipe(void);
void DCInvalidateRange(void* addr, unsigned long n);
void DCFlushRange(void* addr, unsigned long n);
extern int screen_height;

/* WGPIPE at 0xCC008000 -- s16 POS (1 frac bit) + f32 TEX0. */
#define WGPIPE_S16 (*(volatile short*)0xCC008000)
#define WGPIPE_F32 (*(volatile float*)0xCC008000)

__declspec(section ".sdata2") static unsigned int alignment_mask = 0xFFFFFFE0u;

static void set_vertex_format(void);

int nativefont_system_init(void) {
    return 1;
}

int nativefont_estimate_geometry_size(int glyph_count) {
    int size;

    /* Retail: mulli 0x30 + mulli 3 + 0x80 (not combined 0x33). */
    size = glyph_count * 0x30;
    size += glyph_count * 3;
    return size + 0x80;
}

/*
 * Soft ceiling: nativefont_string_render ~99.83% -- upload texture lwz uses
 * incoming r3 instead of the equivalent saved r29; constant-label relocs.
 */
void nativefont_string_render(NativeFontString* ctx, float x, float y) {
    NativeFontInstance* inst;
    Mtx pos;
    GXColor mat;
    unsigned char* rgba;
    PfxFontTransform* src;

    inst = &ctx->instance0;
    pfxaux_upload_texture(ctx->texture);

    while (inst != 0) {
        rgba = inst->rgba;
        mat.r = rgba[0];
        mat.g = rgba[1];
        mat.b = rgba[2];
        mat.a = rgba[3];
        GXSetChanMatColor(4, mat);

        src = ctx->transform;
        /* Copy 3x3 (skip pad columns); translation from +0x30/+0x34. */
        pos[0][0] = src->rx;
        pos[0][1] = src->ry;
        pos[0][2] = src->rz;
        pos[1][0] = src->ux;
        pos[1][1] = src->uy;
        pos[1][2] = src->uz;
        pos[2][0] = src->ax;
        pos[2][1] = src->ay;
        pos[2][2] = src->az;
        /* Snap to pixel centers; Y flipped into screen space. */
        pos[0][3] = (float)(int)(0.5f + (src->tx + x));
        pos[1][3] = (float)(int)(0.5f + (src->ty + ((float)screen_height - y)));
        pos[2][3] = 0.0f;

        GXLoadPosMtxImm(pos, 0);
        GXCallDisplayList(inst->dl, inst->dl_size);
        inst = inst->next;
    }
}

static void set_vertex_format(void) {
    GXClearVtxDesc();
    /* POS XY s16 frac=1; TEX0 ST f32 */
    GXSetVtxAttrFmt(0, 9, 0, 3, 1);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
}

#if !defined(TARGET_PC)
#pragma peephole off
#endif
void nativefont_instance_lock(NativeFontInstance* inst) {
    if (inst == 0) {
        return;
    }
    if (inst->locked != 0) {
        return;
    }

    inst->locked = 1;
    /* Align DL start 0x20 past verts base; capacity from verts_bytes. */
    inst->dl = (void*)(((unsigned long)inst->verts + 0x20u) & ~0x1Fu);
    /* Each step writes inst->size so MWCC cannot fold -0x60+0x1F. */
    inst->size = (unsigned int)inst->verts_bytes - 0x60u;
    inst->size = inst->size + 0x1Fu;
    inst->size = inst->size & alignment_mask;
    inst->size = inst->size + 0x20u;

    DCInvalidateRange(inst->dl, inst->size);
    set_vertex_format();
    GXBeginDisplayList(inst->dl, inst->size);
    GXResetWriteGatherPipe();
}
#if !defined(TARGET_PC)
#pragma peephole reset
#endif

#if !defined(TARGET_PC)
#pragma peephole off
#endif
void nativefont_instance_unlock(NativeFontInstance* inst) {
    if (inst == 0) {
        return;
    }

    inst->locked = 0;
    inst->dl_size = (unsigned int)GXEndDisplayList();
    DCFlushRange(inst->dl, inst->size);
}
#if !defined(TARGET_PC)
#pragma peephole reset
#endif

/*
 * Soft ceiling: nativefont_instance_addglyph ~96.88% -- null/locked branch
 * shape and one final redundant extsh only. POS s16 frac=1 (*2); retail loads
 * Y then X, writes X then Y, and loads V then U before writing U then V.
 */
#pragma optimization_level 2
#pragma scheduling off
void nativefont_instance_addglyph(NativeFontString* ctx, NativeFontInstance* inst,
                                  NativeFontQuad* quad) {
    int t;
    int y;
    short x;
    float u;
    float v;
    volatile short* pipe;
    volatile float* pipef;

    (void)ctx;

    if (quad == 0) {
        return;
    }
    if (inst->locked == 0) {
        return;
    }

    GXBegin(0x80, 0, 4); /* GX_QUADS */

    /* (x0, y1) (u0, v1) */
    t = (short)(int)quad->y1;
    t = t << 1;
    y = (short)t;
    t = (short)(int)quad->x0;
    t = t << 1;
    x = (short)t;
    pipe = (volatile short*)0xCC008000;
    pipef = (volatile float*)0xCC008000;
    *pipe = x;
    *pipe = y;
    v = quad->v1;
    u = quad->u0;
    *pipef = u;
    *pipef = v;

    /* (x0, y0) (u0, v0) */
    t = (short)(int)quad->y0;
    t = t << 1;
    y = (short)t;
    t = (short)(int)quad->x0;
    t = t << 1;
    x = (short)t;
    pipe = (volatile short*)0xCC008000;
    *pipe = x;
    *pipe = y;
    v = quad->v0;
    u = quad->u0;
    *pipef = u;
    *pipef = v;

    /* (x1, y0) (u1, v0) */
    t = (short)(int)quad->y0;
    t = t << 1;
    y = (short)t;
    t = (short)(int)quad->x1;
    t = t << 1;
    x = (short)t;
    pipe = (volatile short*)0xCC008000;
    *pipe = x;
    *pipe = y;
    v = quad->v0;
    u = quad->u1;
    *pipef = u;
    *pipef = v;

    /* (x1, y1) (u1, v1) */
    t = (short)(int)quad->y1;
    t = t << 1;
    y = (short)t;
    t = (short)(int)quad->x1;
    t = t << 1;
    x = (short)t;
    pipe = (volatile short*)0xCC008000;
    *pipe = x;
    *pipe = y;
    v = quad->v1;
    u = quad->u1;
    *pipef = u;
    *pipef = v;
}
#pragma optimization_level 4
#pragma scheduling off

void nativefont_end_render(void) {
    restore_projection_matrix();
}

void nativefont_begin_render(void) {
    set_vertex_format();
    disable_vertex_lights();
    /* COLOR0A0: disabled, amb=vtx, mat=reg */
    GXSetChanCtrl(4, 0, 1, 0, 0, 0, 2);
    GXSetNumChans(1);
    apply_single_texture();
    save_projection_matrix();
    set_2d_projection();
}

void nativefont_string_cleanup(NativeFontString* ctx) {
    (void)ctx;
}

void pfxfont_release_delayed_vertex_buffers(void) {}
