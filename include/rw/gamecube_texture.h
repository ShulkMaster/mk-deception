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
    RwUInt32 reserved_0x18;
    void* imageData;
    void* paletteData;
    RwUInt32 reserved_0x24[2];
    GXTexRegion* textureRegion;
    RwUInt16 token;
    RwUInt8 maxLod;
    RwUInt8 reserved_0x33;
} RwGameCubeRasterExt;

extern RwInt32 _RwGameCubeRasterExtOffset;
extern RwInt32 _RwGameCubeTextureExtOffset;

#define RW_GAMECUBE_TEXTURE_EXTENSION(texture)                              \
    ((RwGameCubeTextureExt*)((RwUInt8*)(texture) +                          \
                             _RwGameCubeTextureExtOffset))
#define RW_GAMECUBE_RASTER_EXTENSION(raster)                                \
    ((RwGameCubeRasterExt*)((RwUInt8*)(raster) +                            \
                            _RwGameCubeRasterExtOffset))

#endif
