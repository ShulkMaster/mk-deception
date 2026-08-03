#include "libmkparticle/gc_state.h"
#include "dolphin/gx.h"
#include "dolphin/mtx.h"
#include "math/gxQuat.h"
#include "platform/display_metrics.h"
#include "rw/alphapass.h"

/* Static identity-ish position matrix; Z scale -1 for screen space. */
static Mtx posMatrix = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, -1.0f, 0.0f},
};

/* GXGetProjectionv writes 7 floats (0x1C). */
static float old_projection_matrix[7];

/* Retail: stw lr before arg loads / lis; required retail scheduler region. */
#pragma scheduling off

void disable_vertex_lights(void) {
    /* COLOR0A0 / COLOR1A1 -- enable off, vtx material on chan0. */
    GXSetChanCtrl(4, 0, 1, 1, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
}

void set_2d_projection(void) {
    Mtx44 ortho;

    /* Signed int->float -> xoris + fsubs; ortho top=0 bottom=h left=0 right=w near=0 far=1. */
    C_MTXOrtho(ortho, 0.0f, (float)screen_height, 0.0f, (float)screen_width, 0.0f, 1.0f);
    GXSetProjection(ortho, 1); /* GX_ORTHOGRAPHIC */
}

void set_2d_position(int x, int y) {
    posMatrix[0][3] = (float)x;
    posMatrix[1][3] = (float)y;
    GXLoadPosMtxImm(posMatrix, 0);
}

void save_projection_matrix(void) {
    GXGetProjectionv(old_projection_matrix);
}

void restore_projection_matrix(void) {
    GXSetProjectionv(old_projection_matrix);
}

/* Retail: apply_texture_with_alphamap bl's this -- must not inline (-inline off). */
#pragma dont_inline on
void apply_single_texture(void) {
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0); /* GX_MODULATE */
}
#pragma dont_inline reset

void apply_texture_with_alphamap(void) {
    RxGCTevAlphaPass pass;

    /* Store order: +0x24, +0x1C, +0x08, +0x20. */
    pass.field_0x24 = 0;
    pass.field_0x1C = 0;
    pass.mode = 4;
    pass.field_0x20 = 0;
    _rxGCTevAlphaPassSetup(&pass);
    apply_single_texture();
}

void reset_tev_stages(void) {
    RxGCTevAlphaPass pass;

    pass.field_0x24 = 0;
    pass.field_0x1C = 0;
    pass.mode = 4;
    pass.field_0x20 = 0;
    _rxGCTevAlphaPassCleanup(&pass);
}

#pragma scheduling reset
