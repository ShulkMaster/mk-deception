#include "dolphin/gx.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RwDlFilterMode {
    RwInt32 minFilter;
    RwInt32 magFilter;
} RwDlFilterMode;

RwInt32 _RwGameCubeTextureExtOffset;
extern RwUInt16 _RwDlTokenCurrent;
extern RwRaster* _RwDlRasterWhite;
extern RwTexture* _RwDlTexture;

extern void _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster,
                                  RwBool releaseRaster);
extern RwInt32 RwTextureRegisterPlugin(
    RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);

static RwDlFilterMode _RwDlFilterModeConvTable[7] = {
    {0, 0}, {0, 0}, {1, 1}, {2, 0}, {3, 1}, {4, 0}, {5, 1},
};
static RwInt32 _RwDlAddressConvTable[5] = {0, 1, 2, 0, 0};
static RwTexture* _RwDlTextureCache[8] = {0, 0, 0, 0,
                                          0, 0, 0, 0};

static void _rwDlTextureConst(RwTexture* texture)
{
    RwGameCubeTextureExt* textureExt =
        RwGameCubeTextureExtension(texture);

    textureExt->flags = 0x01000000;
}

static void _rwDlTextureDest(RwTexture* texture)
{
    RwInt32 index;

    for (index = 0; index < 8; index++) {
        if (texture == _RwDlTextureCache[index]) {
            _RwDlTextureCache[index] = 0;
        }
    }
}

void _rwDlTextureCacheInit(void)
{
    RwUInt32 index = 8;

    while (index-- != 0) {
        _RwDlTextureCache[index] = 0;
    }
}

void _rwDlTexturePluginAttach(void)
{
    _RwGameCubeTextureExtOffset = RwTextureRegisterPlugin(
        sizeof(RwGameCubeTextureExt), 0x40C,
        (RwPluginObjectConstructor)_rwDlTextureConst,
        (RwPluginObjectDestructor)_rwDlTextureDest, 0);
}

static void _rwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias,
                                     RwUInt32 biasClamp, RwUInt32 edgeLod,
                                     RwInt32 maxAnisotropy,
                                     RwUInt32 textureMap)
{
    RwGameCubeTextureExt* textureExt =
        RwGameCubeTextureExtension(texture);
    RwRaster* raster = texture->raster;
    RwGameCubeRasterExt* rasterExt =
        RwGameCubeRasterExtension(raster->parent);
    RwInt32 rasterFormat = (RwUInt8)raster->format << 8;
    RwInt32 minFilter;
    RwInt32 magFilter;

    if ((rasterFormat & 0x6000) != 0) {
        RwUInt32 tlut;
        RwBool mipmap;

        if ((textureExt->flags & 0x02000000) != 0) {
            tlut = GXGetTexObjTlut(&textureExt->object);
        } else {
            tlut = textureMap;
        }
        if ((rasterFormat & 0x8000) != 0) {
            mipmap = 1;
        } else {
            mipmap = 0;
        }
        GXInitTexObjCI(&textureExt->object, rasterExt->imageData,
                       (RwUInt16)raster->width, (RwUInt16)raster->height,
                       rasterExt->format,
                       _RwDlAddressConvTable[(texture->filter_flags & 0xF00) >> 8],
                       _RwDlAddressConvTable[(texture->filter_flags & 0xF000) >> 12],
                       mipmap, tlut);
        if ((RwInt32)(RwUInt8)texture->filter_flags == 6 ||
            (RwInt32)(RwUInt8)texture->filter_flags == 5) {
            minFilter = _RwDlFilterModeConvTable[4].minFilter;
            magFilter = _RwDlFilterModeConvTable[4].magFilter;
        } else {
            minFilter =
                _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].minFilter;
            magFilter =
                _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].magFilter;
        }
    } else {
        RwBool mipmap;

        if ((rasterFormat & 0x8000) != 0) {
            mipmap = 1;
        } else {
            mipmap = 0;
        }

        GXInitTexObj(&textureExt->object, rasterExt->imageData,
                     (RwUInt16)raster->width, (RwUInt16)raster->height,
                     rasterExt->format,
                     _RwDlAddressConvTable[(texture->filter_flags & 0xF00) >> 8],
                     _RwDlAddressConvTable[(texture->filter_flags & 0xF000) >> 12],
                     mipmap);
        minFilter =
            _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].minFilter;
        magFilter =
            _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].magFilter;
    }

    GXInitTexObjLOD(&textureExt->object, minFilter, magFilter, 0.0f,
                    (RwReal)rasterExt->maxLod, lodBias, (RwUInt8)biasClamp,
                    (RwUInt8)edgeLod, maxAnisotropy);
    textureExt->flags =
        ((textureExt->flags & 0xFFFF0000) |
         (RwUInt16)texture->filter_flags) &
        ~0x01000000;
}

void _rwDlTextureSet(RwTexture* texture, RwUInt32 textureMap)
{
    RwRaster* raster;
    RwGameCubeRasterExt* rasterExt;
    RwGameCubeTextureExt* textureExt;

    if (texture == 0) {
        texture = _RwDlTexture;
        _rwDlTextureSetRaster(texture, _RwDlRasterWhite, 0);
    }

    raster = texture->raster;
    rasterExt = RwGameCubeRasterExtension(raster->parent);
    textureExt = RwGameCubeTextureExtension(texture);
    rasterExt->token = _RwDlTokenCurrent & 0xFFFF;

    if ((RwInt32)((((RwUInt32)raster->format & 0xFF) << 8) & 0x6000) != 0) {
        if ((textureExt->flags & 0x01000000) != 0) {
            _rwGameCubeTextureSetLOD(texture, 0.0f, 1, 1, 0,
                                     textureMap);
        } else if ((RwUInt16)texture->filter_flags !=
                   (RwUInt16)textureExt->flags) {
            _rwGameCubeTextureSetLOD(
                texture, GXGetTexObjLODBias(&textureExt->object),
                GXGetTexObjBiasClamp(&textureExt->object),
                GXGetTexObjEdgeLOD(&textureExt->object),
                GXGetTexObjMaxAniso(&textureExt->object), textureMap);
        } else if ((textureExt->flags & 0x02000000) == 0 &&
                   textureMap != GXGetTexObjTlut(&textureExt->object)) {
            _rwGameCubeTextureSetLOD(
                texture, GXGetTexObjLODBias(&textureExt->object),
                GXGetTexObjBiasClamp(&textureExt->object),
                GXGetTexObjEdgeLOD(&textureExt->object),
                GXGetTexObjMaxAniso(&textureExt->object), textureMap);
        }
        if ((textureExt->flags & 0x02000000) == 0) {
            GXLoadTlut(&rasterExt->tlut, textureMap);
        }
    } else if ((textureExt->flags & 0x01000000) != 0) {
        _rwGameCubeTextureSetLOD(texture, 0.0f, 1, 1, 0, 0);
    } else if ((RwUInt16)texture->filter_flags !=
               (RwUInt16)textureExt->flags) {
        _rwGameCubeTextureSetLOD(
            texture, GXGetTexObjLODBias(&textureExt->object),
            GXGetTexObjBiasClamp(&textureExt->object),
            GXGetTexObjEdgeLOD(&textureExt->object),
            GXGetTexObjMaxAniso(&textureExt->object), 0);
    }

    if ((textureExt->flags & 0x02000000) == 0) {
        GXLoadTexObj(&textureExt->object, textureMap);
    } else {
        GXLoadTexObjPreLoaded(&textureExt->object, rasterExt->textureRegion,
                              textureMap);
    }
    _RwDlTextureCache[textureMap] = texture;
}

void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias,
                             RwInt32 biasClamp, RwInt32 edgeLod,
                             RwInt32 maxAnisotropy)
{
    _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod,
                             maxAnisotropy, 0);
}
