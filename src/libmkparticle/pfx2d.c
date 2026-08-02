#include "libmkparticle/pfx2d.h"
#include "libmkparticle/gc_2d.h"
#include "libmkparticle/pfx_rw_types.h"
#include "libmkparticle/rw_engine.h"
#include "platform/fast_rw.h"

/* Retail keeps these explicitly zero-initialized pools in .data. */
static unsigned char is_allocated[PFX2D_POOL_SIZE] = {0};
static Pfx2dObj pfx_2d_buffer[PFX2D_POOL_SIZE] = {0};
static int must_draw[PFX2D_POOL_SIZE] = {0};

static int first_potentially_available_location;
static int num_visible_objects;

/* Retail order: pfx2d_init, then local get_initialized, then alloc... */
#if !defined(TARGET_PC)
#pragma dont_inline on
#pragma scheduling off
#endif
void pfx2d_init(void) {
    /* Retail: stw lr then li r3,0x1F4 (scheduling off). */
    native2d_init(0x1F4);
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
static Pfx2dObj* get_initialized_2d_object_by_index(int index) {
    Pfx2dObj* obj;

    is_allocated[index] = 1;
    /* Soft ceiling: mulli index*0xD0 vs &buf[index] (same stride sizeof==0xD0). */
    obj = &pfx_2d_buffer[index];
    obj->pool_index = index;
    obj->src_blend = 5;
    obj->dst_blend = 6;
    native2d_init_object(obj);
    return obj;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#pragma dont_inline reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
Pfx2dObj* pfx2d_alloc_obj(void) {
    /* Decl order: start before i so MWCC maps start->r6, i->r7 (retail). */
    int start;
    int i;
    int limit;
    int slot;

    i = 0;
    start = first_potentially_available_location;
    limit = PFX2D_POOL_SIZE;
    for (; i < limit; i++) {
        slot = (start + i) % limit;
        if (is_allocated[slot] == 0) {
            is_allocated[slot] = 1;
            first_potentially_available_location = slot + 1;
            return get_initialized_2d_object_by_index(slot);
        }
    }
    return 0;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfx2d_free_obj(Pfx2dObj* obj) {
    /*
     * Retail leaf: li r4,0 ; lwz index ; lis/addi is_allocated->r3 ; stbx.
     * Zero must be materialized before the table address clobbers r3.
     */
    int zero = 0;
    int index = obj->pool_index;
    is_allocated[index] = (unsigned char)zero;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfx2d_build_default_geometry(Pfx2dObj* obj) {
    /* Soft ceiling: build_default ~98.3% -- vert/uv off GPRs shifted vs
     * retail r4/r5 (coloring); add-after-fmuls schedule matched. Stop. */
    /* Stack copy of UV unit quad -- retail copies @321 rodata onto SP. */
    float uvs[8] = {
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    };
    float width_f;
    float height_f;
    /* Decl order: vert_off then uv_off (retail li pair before mtctr). */
    int vert_off;
    int uv_off;
    int i;
    float* uv;
    float t;

    /* Retail requires texture+raster (null TGA loaders return NULL upstream). */
    obj->tex_w = obj->texture->raster->width;
    obj->tex_h = obj->texture->raster->height;
    width_f = (float)obj->tex_w;
    height_f = (float)obj->tex_h;

    vert_off = 0;
    uv_off = 0;
    for (i = 0; i < 4; i++) {
        /* Offset stores keep add-after-fmuls (typed v* hoists the add). */
        uv = (float*)((char*)uvs + uv_off);
        t = width_f * uv[0];
        *(float*)((char*)obj + vert_off) = t;
        t = height_f * uv[1];
        *(float*)((char*)obj + vert_off + 4) = t;
        *(float*)((char*)obj + vert_off + 8) = uv[0];
        *(float*)((char*)obj + vert_off + 12) = uv[1];
        *((unsigned char*)obj + vert_off + 0x10) = 0xFF;
        *((unsigned char*)obj + vert_off + 0x11) = 0xFF;
        *((unsigned char*)obj + vert_off + 0x12) = 0xFF;
        *((unsigned char*)obj + vert_off + 0x13) = 0xFF;
        vert_off += 0x14;
        uv_off += 0x8;
    }

    obj->x = 0;
    obj->y = 0;
    obj->scale_x = 1.0f;
    obj->scale_y = 1.0f;
    obj->mirror = 1;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

void pfx2d_begin_render(void) {
    pfx2d_init();
    native2d_begin_render();
    num_visible_objects = 0;
}

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfx2d_end_render(void) {
    /* Soft ceiling: end_render ~93.75% -- seed temps vs early NV bases;
     * reload-after-seed / new_src variants regressed (~90.9%). Stop. */
    int saved_cull;
    int src;
    int dst;
    int i;
    int index;
    Pfx2dObj* buffer;
    int* draw_list;
    Pfx2dObj* obj;

    RwEngineInstance->fpRenderStateGet(0x14, &saved_cull);
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(0);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(0);
    RwRenderStateSet_rwRENDERSTATECULLMODE(1);
    RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(1);
    native2d_set_renderstate();

    /* Retail: buffer base then must_draw base, seed blend from [0]. */
    buffer = pfx_2d_buffer;
    draw_list = must_draw;
    index = draw_list[0];
    obj = &buffer[index];
    src = obj->src_blend;
    dst = obj->dst_blend;
    RwRenderStateSet_SRCBLEND_DESTBLEND(src, dst);

    for (i = 0; i < num_visible_objects; i++) {
        index = draw_list[i];
        obj = &buffer[index];
        if (src != obj->src_blend || dst != obj->dst_blend) {
            src = obj->src_blend;
            dst = obj->dst_blend;
            RwRenderStateSet_SRCBLEND_DESTBLEND(src, dst);
        }
        native2d_draw(obj);
    }

    native2d_reset_renderstate();
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(1);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(1);
    RwRenderStateSet_rwRENDERSTATECULLMODE(saved_cull);
    RwRenderStateSet_SRCBLEND_DESTBLEND(5, 6);
    native2d_end_render();
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfx2d_render(Pfx2dObj* obj) {
    int index;
    int n;

    native2d_instance_geometry(obj);
    index = obj->pool_index;
    n = num_visible_objects;
    num_visible_objects = n + 1;
    must_draw[n] = index;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

/* Soft ceilings: end_render ~93.75% (seed/base NV allocation); build_default
 * ~98.26% (offset/view GPR coloring; add-after-fmuls is exact).
 * Matched: init/free/alloc/begin/render/get_initialized; pool .data is 100%. */
