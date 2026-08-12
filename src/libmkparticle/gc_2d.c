#include "libmkparticle/gc_2d.h"
#include "libmkparticle/gc_state.h"
#include "libmkparticle/pfx_rw_types.h"
#include "dolphin/gx.h"
#include "platform/display_metrics.h"
#include "rw/dltextur.h"
#include "runtime/cstring.h"

/* WGPIPE at 0xCC008000 -- mixed short/word/float FIFO writes. */
#define WGPIPE_U16 (*(volatile unsigned short*)0xCC008000)
#define WGPIPE_U32 (*(volatile unsigned int*)0xCC008000)
#define WGPIPE_F32 (*(volatile float*)0xCC008000)

int native2d_init(int pool_size) {
    (void)pool_size;
    return 1;
}

void native2d_begin_render(void) {}

void native2d_end_render(void) {}

void native2d_set_renderstate(void) {
    GXClearVtxDesc();
    /* POS XY s16, CLR0 RGBA8, TEX0 ST f32 */
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXSetVtxAttrFmt(0, 0xB, 1, 5, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetNumChans(1);
    disable_vertex_lights();
    apply_texture_with_alphamap();
    save_projection_matrix();
    set_2d_projection();
    set_2d_position(0, 0);
}

void native2d_reset_renderstate(void) {
    restore_projection_matrix();
    reset_tev_stages();
}

/*
 * Soft ceiling: native2d_draw ~97.95% -- first alpha lwz from r3 (not r31);
 * y/WGPIPE r4<->r3 coloring only (same ops: lha/lis/sth/extsh). A named
 * MMIO base regressed to ~92.5%; narrowing y compiled identically. Stop.
 */
/* Retail native2d_draw requires O2 locally; applying O2 to the full object
 * regresses native2d_instance_geometry by 12.81 percentage points. */
#pragma optimization_level 2
void native2d_draw(Pfx2dObj* obj) {
    Pfx2dObj* o;
    int y;
    int x;
    float u;
    float vt;

    /* Retail: mr r31,r3 before lwz alpha_texture@0xB4(r31). */
    o = obj;
    if (o->alpha_texture != 0) {
        GXSetNumTevStages(2);
        _rwDlTextureSet(o->alpha_texture, 1);
    } else {
        GXSetNumTevStages(1);
    }
    _rwDlTextureSet(o->texture, 0);

    GXBegin(0x80, 0, 4); /* GX_QUADS */

    /* Load Y then X; write X then Y. (short)y forces extsh after lha. */
    y = o->gpu[0].y;
    x = o->gpu[0].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = o->gpu[0].color;
    vt = o->gpu[0].v;
    u = o->gpu[0].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = o->gpu[1].y;
    x = o->gpu[1].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = o->gpu[1].color;
    vt = o->gpu[1].v;
    u = o->gpu[1].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = o->gpu[2].y;
    x = o->gpu[2].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = o->gpu[2].color;
    vt = o->gpu[2].v;
    u = o->gpu[2].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = o->gpu[3].y;
    x = o->gpu[3].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = o->gpu[3].color;
    vt = o->gpu[3].v;
    u = o->gpu[3].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;
}

/*
 * Soft ceiling: native2d_instance_geometry ~92.1% -- raster r5 vs r4 reuse;
 * FPR/lis 4330 coloring; ptr++ walk vs li offs+add (byte-off/do-while
 * loses mtctr ~89%). Full mismatch remains one allocation phase. Stop.
 */
#pragma optimization_level 4
void native2d_instance_geometry(Pfx2dObj* obj) {
    float tex_w;
    float tex_h;
    float inv_w;
    float inv_h;
    float u_scale;
    float v_scale;
    float half;
    float fx;
    float fy;
    float t;
    int i;
    Pfx2dVert* src;
    Pfx2dGpuVtx* dst;
    Pfx2dGpuVtx* gpu_base;
    PfxNativeRasterView* ras;

    ras = pfx_rw_texture_view(obj->texture)->raster;
    tex_w = (float)ras->width;
    tex_h = (float)ras->height;

    inv_w = 1.0f / tex_w;
    inv_h = 1.0f / tex_h;
    /* Retail places addi gpu base between divs and (tex-1). */
    gpu_base = obj->gpu;
    u_scale = tex_w - 1.0f;
    v_scale = tex_h - 1.0f;
    half = 0.5f;

    src = obj->verts;
    dst = gpu_base;
    for (i = 0; i < 4; i++) {
        /* Retail: i2f screen_h, i2f obj.y, sy*vy, fadds, fsubs (not fused). */
        fy = (float)screen_height;
        t = (float)obj->y;
        t = t + obj->scale_y * src->y;
        fy = fy - t;

        fx = (float)obj->x;
        fx = fx + obj->scale_x * src->x;

        dst->x = (short)(int)fx;
        dst->y = (short)(int)fy;

        t = u_scale * src->u;
        t = half + t;
        dst->u = inv_w * t;

        t = 1.0f - src->v;
        t = v_scale * t;
        t = half + t;
        dst->v = inv_h * t;

        dst->rgba[0] = src->r;
        dst->rgba[1] = src->g;
        dst->rgba[2] = src->b;
        dst->rgba[3] = src->a;

        src++;
        dst++;
    }
}

void native2d_init_object(Pfx2dObj* obj) {
    /* Retail: addi r3,r3,0x74 ; li r4,0 ; li r5,0x44 ; bl memset.
     * Rebase obj first so addi lands before the li args. */
    obj = (Pfx2dObj*)&obj->gpu[0];
    memset(obj, 0, 0x44);
}
