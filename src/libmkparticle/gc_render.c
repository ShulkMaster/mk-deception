#include "dolphin/gx.h"
#include "dolphin/mtx.h"
#include "dolphin/types.h"
#include "libmkparticle/gc_state.h"
#include "libmkparticle/gc_render.h"
#include "libmkparticle/geometry.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/texture_anim.h"
#include "rw/gamecube.h"
#include "rw/dltextur.h"
#include "rw/rtquat.h"
#include "rw/rwengine.h"
#include <math.h>

static PfxColor rgba_white = {255, 255, 255, 255};
static GXColor opaque_white = {255, 255, 255, 255};

/* PfxMatrix and RwMatrix share the retail affine matrix layout. */
static void set_view_matrix(PfxVm* vm) {
    _rwDlTransformSetup((RwMatrix*)&vm->transforms[vm->active_transform].matrix, 0);
}

static void build_model_to_view_matrix(const RwMatrix* model, Mtx out) {
    RwMatrix combined;
    const RwMatrix* matrix;
    if (model != 0) {
        combined.flags = 0x20003;
        RwMatrixMultiply(&combined, model, &_RwDlInvCamLTM);
        matrix = &combined;
    } else {
        matrix = &_RwDlInvCamLTM;
    }
    out[0][0] = -matrix->right.x;
    out[0][1] = -matrix->up.x;
    out[0][2] = -matrix->at.x;
    out[0][3] = -matrix->pos.x;
    out[1][0] = matrix->right.y;
    out[1][1] = matrix->up.y;
    out[1][2] = matrix->at.y;
    out[1][3] = matrix->pos.y;
    out[2][0] = -matrix->right.z;
    out[2][1] = -matrix->up.z;
    out[2][2] = -matrix->at.z;
    out[2][3] = -matrix->pos.z;
}

/* The retail caller explicitly forwards vm; this GX setup does not use it. */
static void set_vertex_format(PfxVm* vm)
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
}

static void gc_set_render_state(BOOL use_alpha_map)
{
    GXSetNumChans(1);
    disable_vertex_lights();
    if (use_alpha_map) {
        apply_texture_with_alphamap();
    } else {
        apply_single_texture();
    }
}

/* These are actual GX FIFO writes, not match-forcing volatile accesses. */
#define WRITE_VERTEX(p, color, u, v) do { \
    *(volatile float*)GXFIFO_ADDR = (p).x; \
    *(volatile float*)GXFIFO_ADDR = (p).y; \
    *(volatile float*)GXFIFO_ADDR = (p).z; \
    *(volatile unsigned char*)GXFIFO_ADDR = (color)->r; \
    *(volatile unsigned char*)GXFIFO_ADDR = (color)->g; \
    *(volatile unsigned char*)GXFIFO_ADDR = (color)->b; \
    *(volatile unsigned char*)GXFIFO_ADDR = (color)->a; \
    *(volatile float*)GXFIFO_ADDR = (u); \
    *(volatile float*)GXFIFO_ADDR = (v); \
} while (0)

/* TODO: [breakthrough needed] 66.09151%; behavior recovered; aggregate spills and FIFO color-load ordering differ from retail. */
static void gc_generic_render(PfxVm* vm) {
    PfxVec3 axis0, axis1;
    PfxVec3 base0, base1, base_corner;
    PfxVec3 corner, point;
    PfxVec3* positions;
    PfxColor* colors;
    PfxTextureFrame* uv_stream;
    float* ages;
    float* scales;
    float* angles;
    int count, position_stride, age_stride, color_stride;
    int uv_mode, i;
    float u, v, right, bottom, du, dv;

    count = vm->transforms[vm->active_transform].live;
    pfx_get_billboard_vector(vm, &axis0, &axis1);
    if (!vm->flag150_02 && !vm->flag150_10) {
        axis1.x = -axis1.x;
        axis1.y = -axis1.y;
        axis1.z = -axis1.z;
    }
    corner.x = axis1.x - axis0.x;
    corner.y = axis1.y - axis0.y;
    corner.z = axis1.z - axis0.z;
    if (!vm->flag150_02) {
        axis1.x *= 2.0f; axis1.y *= 2.0f; axis1.z *= 2.0f;
        axis0.x *= 2.0f; axis0.y *= 2.0f; axis0.z *= 2.0f;
    }
    ages = pfx_get_field(vm, -2, 0x301);
    age_stride = pfx_get_struct_size(vm, 0x301);
    scales = (vm->flags_0x1D4 & 0x20) ? pfx_get_field(vm, -2, 0x102) : 0;
    angles = (vm->flags_0x1D4 & 0x40) ? pfx_get_field(vm, -2, 0x103) : 0;
    if (scales || angles) {
        base0 = axis0; base1 = axis1; base_corner = corner;
    }
    colors = vm->flag150_80 ? &vm->color1B4 : &rgba_white;
    color_stride = 0;
    if (vm->flags151 & 2) _rwDlTextureSet(vm->alpha_texture, 1);
    _rwDlTextureSet(vm->render_texture, 0);
    positions = pfx_get_field(vm, -2, 0x100);
    position_stride = pfx_get_struct_size(vm, 0x100);
    if (vm->flags_0x1D4 & 0x10) {
        colors = pfx_get_field(vm, -2, 0x101);
        color_stride = position_stride;
    }
    if (vm->texture_frame_count != 0) {
        uv_mode = (vm->flags_0x1D4 & 0x100) ? 1 : 0;
        du = vm->texture_u_step;
        dv = vm->texture_v_step;
        if (uv_mode == 1) uv_stream = pfx_get_field(vm, -2, 0x104);
    } else {
        uv_mode = 2;
        du = dv = right = bottom = 1.0f;
        u = v = 0.0f;
    }
    GXBegin(0x80, 0, (unsigned short)(count * 4));
    for (i = 0; i < count; i++) {
        if (uv_mode == 0) {
            const PfxTextureFrame* frame = vm->texture_frames +
                pfx_texture_getframe((const PfxTextureAnim*)&vm->texture_frame_count, *ages);
            u = frame->u; v = frame->v;
            right = u + du; bottom = v + dv;
        } else if (uv_mode == 1) {
            u = uv_stream->u; v = uv_stream->v;
            uv_stream = (PfxTextureFrame*)((unsigned char*)uv_stream + position_stride);
            right = u + du; bottom = v + dv;
        }
        if (scales) {
            axis0 = base0; axis1 = base1; corner = base_corner;
        }
        if (angles) {
            float sine = (float)sin(-*angles);
            float cosine = (float)cos(-*angles);
            axis1.x = base1.x * cosine - base0.x * sine;
            axis1.y = base1.y * cosine - base0.y * sine;
            axis1.z = base1.z * cosine - base0.z * sine;
            axis0.x = base1.x * sine + base0.x * cosine;
            axis0.y = base1.y * sine + base0.y * cosine;
            axis0.z = base1.z * sine + base0.z * cosine;
            corner.x = 0.5f * (axis1.x - axis0.x);
            corner.y = 0.5f * (axis1.y - axis0.y);
            corner.z = 0.5f * (axis1.z - axis0.z);
        }
        if (scales) {
            float scale = *scales;
            axis0.x *= scale; axis0.y *= scale; axis0.z *= scale;
            axis1.x *= scale; axis1.y *= scale; axis1.z *= scale;
            corner.x *= scale; corner.y *= scale; corner.z *= scale;
        }
        point.x = positions->x - corner.x;
        point.y = positions->y - corner.y;
        point.z = positions->z - corner.z;
        if (vm->flag150_02) {
            point.x += axis1.x;
            point.y = (480.0f - point.y) + 2.0f * axis0.y;
            point.z = 0.0f;
        }
        WRITE_VERTEX(point, colors, u, v);
        point.x -= axis0.x; point.y -= axis0.y; point.z -= axis0.z;
        WRITE_VERTEX(point, colors, u, bottom);
        point.x += axis1.x; point.y += axis1.y; point.z += axis1.z;
        WRITE_VERTEX(point, colors, right, bottom);
        point.x += axis0.x; point.y += axis0.y; point.z += axis0.z;
        WRITE_VERTEX(point, colors, right, v);
        positions = (PfxVec3*)((unsigned char*)positions + position_stride);
        colors = (PfxColor*)((unsigned char*)colors + color_stride);
        ages = (float*)((unsigned char*)ages + age_stride);
        if (scales) scales = (float*)((unsigned char*)scales + position_stride);
        if (angles) angles = (float*)((unsigned char*)angles + position_stride);
    }
}

static void rw_set_render_state(void) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(12, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(8, 0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(6, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(20, 1);
}

static void rw_reset_render_state(void) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(8, 1);
}

static void setup_lights(PfxVm* vm) {
    GXLightObj light;
    Mtx matrix;
    Vec position;
    GXColor color;
    if (vm->flag150_08) {
        build_model_to_view_matrix(
            (RwMatrix*)&vm->transforms[vm->active_transform].matrix, matrix);
        PSMTXMultVec(matrix, (Vec*)&vm->light_position, &position);
        color = *(GXColor*)&vm->light_color;
        GXInitLightColor(&light, color);
        GXInitLightPos(&light, position.x, position.y, position.z);
        GXInitLightAttn(&light, 1.0f, 0.0f, 0.0f,
                       vm->light_attenuation_k0, vm->light_attenuation_k1,
                       vm->light_attenuation_k2);
        GXLoadLightObjImm(&light, 1);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 1, 1, 0,
                      1, 0, 1);
        GXSetChanMatColor(4, opaque_white);
    }
}

void particle_render(PfxVm* vm) {
    set_view_matrix(vm);
    set_vertex_format(vm);
    rw_set_render_state();
    gc_set_render_state((vm->flags151 >> 1) & 1);
    setup_lights(vm);
    if (vm->flag150_02) {
        save_projection_matrix();
        set_2d_projection();
        set_2d_position(0, 0);
    }
    gc_generic_render(vm);
    if (vm->flag150_02) restore_projection_matrix();
    reset_tev_stages();
    rw_reset_render_state();
    GXSetNumTevStages(1);
}
