#ifndef RW_GAMECUBE_TEXTURE_H
#define RW_GAMECUBE_TEXTURE_H

#include "dolphin/gx.h"
#include "rw/rwcore_types.h"

typedef struct RwGameCubeTextureExt {
    GXTexObj object;
    RwUInt32 flags;
} RwGameCubeTextureExt;

typedef struct RwGameCubeRasterExt {
    GXTlutObj tlut;
    RwInt32 format;
    RwInt32 paletteFormat;
    RwUInt32 hasAlpha;
    void* allocation;
    void* imageData;
    void* paletteData;
    void* lockedTiledData;
    void* lockBuffer;
    GXTexRegion* textureRegion;
    RwUInt16 token;
    RwUInt8 maxLod;
    RwUInt8 lockedMipLevel;
} RwGameCubeRasterExt;

extern RwInt32 _RwGameCubeRasterExtOffset;
extern RwInt32 _RwGameCubeTextureExtOffset;

static inline RwGameCubeTextureExt* RwGameCubeTextureExtension(
    RwTexture* texture)
{
    return (RwGameCubeTextureExt*)((RwUInt8*)texture +
                                   _RwGameCubeTextureExtOffset);
}

static inline RwGameCubeRasterExt* RwGameCubeRasterExtension(RwRaster* raster)
{
    return (RwGameCubeRasterExt*)((RwUInt8*)raster +
                                  _RwGameCubeRasterExtOffset);
}

#endif
