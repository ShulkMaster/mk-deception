#include "dolphin/gx.h"
#include "dolphin/types.h"
#include "libmkparticle/gc_state.h"

static void set_vertex_format(void)
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
