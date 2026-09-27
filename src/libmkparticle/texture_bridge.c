#include "libmkparticle/texture_bridge.h"

#include "rw/rwengine.h"
#include "rw/gamecube.h"

static int pfxaux_set_render_state(int state, int value) {
    return RwEngineInstance->dOpenDevice.fpRenderStateSet(state, value);
}

void pfxaux_upload_texture(RwTexture* texture) {
    int address;

    /* RwTextureGetAddressing: V mode when U and V agree, else none. */
    address = (((texture->filter_flags & 0xF00) >> 8) ==
               ((texture->filter_flags & 0xF000) >> 12))
                  ? (texture->filter_flags & 0xF000) >> 12
                  : 0;
    pfxaux_set_render_state(2, address);
    pfxaux_set_render_state(1, (int)texture->raster);
    _rwDlTextureRasterFlush();
}
