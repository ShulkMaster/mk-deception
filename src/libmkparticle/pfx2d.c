/* BUILD: No -use_lmw_stmw: retail uses _savegpr_25/_restgpr_25 in end_render. */

#include "libmkparticle/pfx2d.h"
#include "libmkparticle/gc_2d.h"
#include "libmkparticle/pfx_rw_types.h"
#include "rw/rwengine.h"
#include "platform/fast_rw.h"

#pragma scheduling off
#pragma peephole off

/* Retail keeps these explicitly zero-initialized pools in .data. */
static unsigned char is_allocated[PFX2D_POOL_SIZE] = {0};
static Pfx2dObj pfx_2d_buffer[PFX2D_POOL_SIZE] = {0};
static int must_draw[PFX2D_POOL_SIZE] = {0};

static int num_visible_objects;
static int first_potentially_available_location;

/* Retail order: pfx2d_init, then local get_initialized, then alloc... */
#pragma dont_inline on
void pfx2d_init(void) {
    native2d_init(0x1F4);
}

static Pfx2dObj* get_initialized_2d_object_by_index(int index) {
    Pfx2dObj* obj;

    is_allocated[index] = 1;
    obj = &pfx_2d_buffer[index];
    obj->pool_index = index;
    obj->src_blend = 5;
    obj->dst_blend = 6;
    native2d_init_object(obj);
    return obj;
}
#pragma dont_inline reset

Pfx2dObj* pfx2d_alloc_obj(void) {
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

void pfx2d_free_obj(Pfx2dObj* obj) {
    is_allocated[obj->pool_index] = 0;
}

void pfx2d_build_default_geometry(Pfx2dObj* obj) {
    float uvs[8] = {
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    };
    float width_f;
    float height_f;
    int i;

    obj->tex_w = pfx_rw_texture_view(obj->texture)->raster->width;
    obj->tex_h = pfx_rw_texture_view(obj->texture)->raster->height;
    width_f = (float)obj->tex_w;
    height_f = (float)obj->tex_h;

    for (i = 0; i < 4; i++) {
        obj->verts[i].x = width_f * uvs[i * 2];
        obj->verts[i].y = height_f * uvs[i * 2 + 1];
        obj->verts[i].u = uvs[i * 2];
        obj->verts[i].v = uvs[i * 2 + 1];
        obj->verts[i].r = 0xFF;
        obj->verts[i].g = 0xFF;
        obj->verts[i].b = 0xFF;
        obj->verts[i].a = 0xFF;
    }

    obj->x = 0;
    obj->y = 0;
    obj->scale_x = 1.0f;
    obj->scale_y = 1.0f;
    obj->mirror = 1;
}

void pfx2d_begin_render(void) {
    pfx2d_init();
    native2d_begin_render();
    num_visible_objects = 0;
}

void pfx2d_end_render(void) {
    int saved_cull;
    int i;
    int src;
    int dst;
    Pfx2dObj* obj;

    RwEngineInstance->dOpenDevice.fpRenderStateGet(0x14, &saved_cull);
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(0);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(0);
    RwRenderStateSet_rwRENDERSTATECULLMODE(1);
    RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(1);
    native2d_set_renderstate();

    src = pfx_2d_buffer[must_draw[0]].src_blend;
    dst = pfx_2d_buffer[must_draw[0]].dst_blend;
    RwRenderStateSet_SRCBLEND_DESTBLEND(src, dst);

    for (i = 0; i < num_visible_objects; i++) {
        obj = &pfx_2d_buffer[must_draw[i]];
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

void pfx2d_render(Pfx2dObj* obj) {
    int index;
    int n;

    native2d_instance_geometry(obj);
    index = obj->pool_index;
    n = num_visible_objects;
    num_visible_objects = n + 1;
    must_draw[n] = index;
}
