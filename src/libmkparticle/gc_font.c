/* TODO: [review] layout stubs: get_num_newlines, render_string and nativefont_instance_geometry
 * reproduce linker-stripped retail functions only to recreate their pool layout; the bodies are
 * NOT recovered source. Replace with genuine bodies if a source turns up. */
#include "libmkparticle/gc_font.h"
#include "libmkparticle/gc_state.h"
#include "libmkparticle/texture_bridge.h"
#include "dolphin/gx.h"
#include "dolphin/cache.h"
#include "math/gxQuat.h"

#include "platform/display_metrics.h"

#define WGPIPE_S16 (*(volatile short*)GXFIFO_ADDR)
#define WGPIPE_F32 (*(volatile float*)GXFIFO_ADDR)

static void set_vertex_format(void);

int nativefont_system_init(void) {
    return 1;
}

int nativefont_estimate_geometry_size(int glyph_count) {
    int size;

    size = glyph_count * 0x30;
    size += glyph_count * 3;
    return size + 0x80;
}

static int get_num_newlines(const char* text) {
    return 0;
}

static float render_string(float x) {
    return (int)(0.5f + x);
}

int nativefont_instance_geometry(NativeFontInstance* inst) {
    return 0;
}

__declspec(section ".sdata2") static unsigned int alignment_mask = 0xFFFFFFE0u;

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
        pos[0][0] = src->rx;
        pos[0][1] = src->ry;
        pos[0][2] = src->rz;
        pos[1][0] = src->ux;
        pos[1][1] = src->uy;
        pos[1][2] = src->uz;
        pos[2][0] = src->ax;
        pos[2][1] = src->ay;
        pos[2][2] = src->az;
        pos[0][3] = (int)(0.5f + (src->tx + x));
        pos[1][3] = (int)(0.5f + (src->ty + ((float)screen_height - y)));
        pos[2][3] = 0.0f;

        GXLoadPosMtxImm(pos, 0);
        GXCallDisplayList(inst->dl, inst->dl_size);
        inst = inst->next;
    }
}

static void set_vertex_format(void) {
    GXClearVtxDesc();
    GXSetVtxAttrFmt(0, 9, 0, 3, 1);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
}

void nativefont_instance_lock(NativeFontInstance* inst) {
    if (inst == 0) {
        return;
    }
    if (inst->locked != 0) {
        return;
    }

    inst->locked = 1;
    inst->dl = (void*)(((unsigned long)inst->verts + 0x20u) &
                       ~(unsigned long)0x1Fu);
    inst->size = (unsigned int)inst->verts_bytes - 0x60u;
    inst->size = inst->size + 0x1Fu;
    inst->size = inst->size & alignment_mask;
    inst->size = inst->size + 0x20u;

    DCInvalidateRange(inst->dl, inst->size);
    set_vertex_format();
    GXBeginDisplayList(inst->dl, inst->size);
    GXResetWriteGatherPipe();
}

void nativefont_instance_unlock(NativeFontInstance* inst) {
    if (inst == 0) {
        return;
    }

    inst->locked = 0;
    inst->dl_size = GXEndDisplayList();
    DCFlushRange(inst->dl, inst->size);
}

void nativefont_instance_addglyph(NativeFontString* ctx, NativeFontInstance* inst,
                                  NativeFontQuad* quad) {
    short y;
    float v;

    if (quad == 0 || inst->locked == 0) {
        return;
    }

    GXBegin(0x80, 0, 4);

    y = (short)quad->y1 << 1;
    WGPIPE_S16 = (short)quad->x0 << 1;
    WGPIPE_S16 = y;
    v = quad->v1;
    WGPIPE_F32 = quad->u0;
    WGPIPE_F32 = v;

    y = (short)quad->y0 << 1;
    WGPIPE_S16 = (short)quad->x0 << 1;
    WGPIPE_S16 = y;
    v = quad->v0;
    WGPIPE_F32 = quad->u0;
    WGPIPE_F32 = v;

    y = (short)quad->y0 << 1;
    WGPIPE_S16 = (short)quad->x1 << 1;
    WGPIPE_S16 = y;
    v = quad->v0;
    WGPIPE_F32 = quad->u1;
    WGPIPE_F32 = v;

    y = (short)quad->y1 << 1;
    WGPIPE_S16 = (short)quad->x1 << 1;
    WGPIPE_S16 = y;
    v = quad->v1;
    WGPIPE_F32 = quad->u1;
    WGPIPE_F32 = v;
}

void nativefont_end_render(void) {
    restore_projection_matrix();
}

void nativefont_begin_render(void) {
    set_vertex_format();
    disable_vertex_lights();
    GXSetChanCtrl(4, 0, 1, 0, 0, 0, 2);
    GXSetNumChans(1);
    apply_single_texture();
    save_projection_matrix();
    set_2d_projection();
}

void nativefont_string_cleanup(NativeFontString* ctx) {
}

void pfxfont_release_delayed_vertex_buffers(void) {}
