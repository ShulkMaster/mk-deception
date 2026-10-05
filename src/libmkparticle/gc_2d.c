/* BUILD: -schedule off: keep source-order i2f/fctiwz closer to retail. -fp_contract off: retail
 * uses fmuls+fadds (not fmadds) in geometry. */

#include "libmkparticle/gc_2d.h"
#include "libmkparticle/gc_state.h"
#include "libmkparticle/pfx_rw_types.h"
#include "dolphin/gx.h"
#include "platform/display_metrics.h"
#include "rw/dltextur.h"
#include "runtime/cstring.h"
#include "rw/rwcore_types.h"

/* WGPIPE at 0xCC008000 -- mixed short/word/float FIFO writes. */
union Native2dFifo {
    unsigned short u16;
    unsigned int u32;
    float f32;
};

volatile union Native2dFifo native2d_fifo : 0xCC008000;

#define WGPIPE_U16 native2d_fifo.u16
#define WGPIPE_U32 native2d_fifo.u32
#define WGPIPE_F32 native2d_fifo.f32

int native2d_init(int pool_size) {
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

#pragma optimization_level 2
/* TODO: [near miss] 97.95%; alpha load owner and Y/FIFO r3-r4 allocation remain. */
void native2d_draw(Pfx2dObj* obj) {
    int y;
    int x;
    float u;
    float vt;

    if (obj->alpha_texture != 0) {
        GXSetNumTevStages(2);
        _rwDlTextureSet(obj->alpha_texture, 1);
    } else {
        GXSetNumTevStages(1);
    }
    _rwDlTextureSet(obj->texture, 0);

    GXBegin(0x80, 0, 4); /* GX_QUADS */

    y = obj->gpu[0].y;
    x = obj->gpu[0].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = obj->gpu[0].color;
    vt = obj->gpu[0].v;
    u = obj->gpu[0].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = obj->gpu[1].y;
    x = obj->gpu[1].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = obj->gpu[1].color;
    vt = obj->gpu[1].v;
    u = obj->gpu[1].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = obj->gpu[2].y;
    x = obj->gpu[2].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = obj->gpu[2].color;
    vt = obj->gpu[2].v;
    u = obj->gpu[2].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;

    y = obj->gpu[3].y;
    x = obj->gpu[3].x;
    WGPIPE_U16 = (unsigned short)x;
    WGPIPE_U16 = (unsigned short)(short)y;
    WGPIPE_U32 = obj->gpu[3].color;
    vt = obj->gpu[3].v;
    u = obj->gpu[3].u;
    WGPIPE_F32 = u;
    WGPIPE_F32 = vt;
}

#pragma optimization_level 4
/* TODO: [near miss] 93.179779%; coordinate and UV operations agree; indexed
 * walks still fold differently, with raster and FP register residue. */
void native2d_instance_geometry(Pfx2dObj* obj) {
    float tex_w;
    float tex_h;
    float inv_w;
    float inv_h;
    float u_scale;
    float v_scale;
    float half;
    int i;
    Pfx2dGpuVtx* gpu_base;
    PfxNativeRasterView* ras;

    ras = pfx_rw_texture_view(obj->texture)->raster;
    tex_w = ras->width;
    tex_h = ras->height;

    inv_w = 1.0f / tex_w;
    inv_h = 1.0f / tex_h;
    gpu_base = obj->gpu;
    u_scale = tex_w - 1.0f;
    v_scale = tex_h - 1.0f;
    half = 0.5f;

    for (i = 0; i < 4; i++) {
        float fy = (float)screen_height - ((float)obj->y + obj->scale_y * obj->verts[i].y);

        float fx = (float)obj->x + obj->scale_x * obj->verts[i].x;

        gpu_base[i].x = (int)fx;
        gpu_base[i].y = (int)fy;

        gpu_base[i].u = inv_w * (half + u_scale * obj->verts[i].u);

        gpu_base[i].v = inv_h * (half + v_scale * (1.0f - obj->verts[i].v));

        gpu_base[i].rgba[0] = obj->verts[i].r;
        gpu_base[i].rgba[1] = obj->verts[i].g;
        gpu_base[i].rgba[2] = obj->verts[i].b;
        gpu_base[i].rgba[3] = obj->verts[i].a;

    }
}

/* TODO: [near miss] 88.55%; GPU clear bounds agree; memset argument setup order differs. */
void native2d_init_object(Pfx2dObj* obj) {
    /* Clear the GPU vertices, alpha texture pointer and intervening alignment. */
    memset(obj->gpu, 0,
           RW_OFFSET_OF(Pfx2dObj, padB8) - RW_OFFSET_OF(Pfx2dObj, gpu));
}
