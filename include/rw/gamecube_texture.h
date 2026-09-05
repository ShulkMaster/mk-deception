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

typedef char RwGameCubeTextureExtSizeCheck[
    sizeof(RwGameCubeTextureExt) == 0x24 ? 1 : -1];
typedef char RwGameCubeRasterExtSizeCheck[
    sizeof(RwGameCubeRasterExt) == 0x34 ? 1 : -1];

int _rwDlTextureSetRaster(void* texture, void* raster, int unused);

#define RW_TEXTURE_PLATFORM_DATA(texture) \
    ((RwGameCubeTextureExt*)((unsigned char*)(texture) + \
                            _RwGameCubeTextureExtOffset))
#define RW_RASTER_PLATFORM_DATA(raster) \
    ((RwGameCubeRasterExt*)((unsigned char*)(raster) + \
                           _RwGameCubeRasterExtOffset))

/* Repository-local views of the runtime plugin storage. */
static inline RwGameCubeTextureExt* rwTexturePlatformData(
    RwTexture* texture)
{
    return (RwGameCubeTextureExt*)((unsigned char*)texture +
                                   _RwGameCubeTextureExtOffset);
}

static inline RwGameCubeRasterExt* rwRasterPlatformData(RwRaster* raster)
{
    return (RwGameCubeRasterExt*)((unsigned char*)raster +
                                  _RwGameCubeRasterExtOffset);
}

#endif
