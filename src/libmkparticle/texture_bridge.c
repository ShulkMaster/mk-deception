#include "libmkparticle/texture_bridge.h"

#include "rw/rwengine.h"
#include "rw/gamecube.h"

static int pfxaux_set_render_state(int state, long value) {
    return RwEngineInstance->dOpenDevice.fpRenderStateSet(state, value);
}

void pfxaux_upload_texture(RwTexture* texture) {
    unsigned int address_u;
    unsigned int address_v;

    unsigned int flags = texture->filter_flags;

    if ((address_u = (flags & 0xF00) >> 8) !=
        (address_v = (flags & 0xF000) >> 12)) {
        address_v = 0;
    }

    pfxaux_set_render_state(2, address_v);
    pfxaux_set_render_state(1, (long)texture->raster);
    _rwDlTextureRasterFlush();
}
