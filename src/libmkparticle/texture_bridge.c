#include "libmkparticle/texture_bridge.h"

#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"

static int pfxaux_set_render_state(int state, int value) {
    return RwEngineInstance->fpRenderStateSet(state, value);
}

/* Soft ceiling: pfxaux_upload_texture ~95.28% -- equivalent address-mode
 * extraction leaves a small register-allocation/branch-emission island. */
void pfxaux_upload_texture(RwTexture* texture) {
    unsigned int address_u;
    unsigned int address_v;

    unsigned int flags = texture->filter_flags;

    address_u = (flags & 0xF00) >> 8;
    address_v = (flags & 0xF000) >> 12;
    if (address_u != address_v) {
        address_v = 0;
    }

    pfxaux_set_render_state(2, address_v);
    pfxaux_set_render_state(1, (int)texture->raster);
    _rwDlTextureRasterFlush();
}
