#ifndef LIBMKPARTICLE_PFX_RW_TYPES_H
#define LIBMKPARTICLE_PFX_RW_TYPES_H

#include "libmkparticle/pfx2d.h"

/* Particle renderer view of the texture's native raster prefix. */
typedef struct PfxRwTextureView {
    PfxNativeRasterView* raster; /* +0x00 */
} PfxRwTextureView;

static PfxRwTextureView* pfx_rw_texture_view(RwTexture* texture) {
    return (PfxRwTextureView*)texture;
}

#endif
