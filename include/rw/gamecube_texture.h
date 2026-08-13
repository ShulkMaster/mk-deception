#ifndef RW_GAMECUBE_TEXTURE_H
#define RW_GAMECUBE_TEXTURE_H

#include "dolphin/gx.h"
#include "rw/gamecube_globals.h"
#include "rw/rwcore_types.h"

typedef struct RwGameCubeTextureExt {
    GXTexObj object;
    unsigned int flags;
} RwGameCubeTextureExt;

typedef struct RwGameCubeRasterExt {
    GXTlutObj tlut;
    int format;
    int paletteFormat;
    unsigned int hasAlpha;
    void* allocation;
    void* imageData;
    void* paletteData;
    void* lockedTiledData;
    void* lockBuffer;
    GXTexRegion* textureRegion;
    unsigned short token;
    unsigned char maxLod;
    unsigned char lockedMipLevel;
} RwGameCubeRasterExt;

int _rwDlTextureSetRaster(void* texture, void* raster, int unused);

static inline RwGameCubeTextureExt* RwGameCubeTextureExtension(
    RwTexture* texture)
{
    return (RwGameCubeTextureExt*)((unsigned char*)texture +
                                   _RwGameCubeTextureExtOffset);
}

static inline RwGameCubeRasterExt* RwGameCubeRasterExtension(RwRaster* raster)
{
    return (RwGameCubeRasterExt*)((unsigned char*)raster +
                                  _RwGameCubeRasterExtOffset);
}

#endif
