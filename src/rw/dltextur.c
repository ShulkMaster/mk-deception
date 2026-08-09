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
static RwTexture* _RwDlTextureCache[8] = {NULL, NULL, NULL, NULL,
                                          NULL, NULL, NULL, NULL};

static void _rwDlTextureConst(RwTexture* texture)
{
    RwGameCubeTextureExt* textureExt =
        RW_GAMECUBE_TEXTURE_EXTENSION(texture);

    textureExt->flags = 0x01000000;
}

static void _rwDlTextureDest(RwTexture* texture)
{
    RwInt32 index;

    for (index = 0; index < 8; index++) {
        if (texture == _RwDlTextureCache[index]) {
            _RwDlTextureCache[index] = NULL;
        }
    }
}

void _rwDlTextureCacheInit(void)
{
    RwUInt32 index = 8;

    while (index-- != 0) {
        _RwDlTextureCache[index] = NULL;
    }
}

void _rwDlTexturePluginAttach(void)
{
    _RwGameCubeTextureExtOffset = RwTextureRegisterPlugin(
        sizeof(RwGameCubeTextureExt), 0x40C,
        (RwPluginObjectConstructor)_rwDlTextureConst,
        (RwPluginObjectDestructor)_rwDlTextureDest, NULL);
}

static void _rwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias,
                                     RwUInt32 biasClamp, RwUInt32 edgeLod,
                                     RwInt32 maxAnisotropy,
                                     RwUInt32 textureMap)
{
    RwGameCubeTextureExt* textureExt =
        RW_GAMECUBE_TEXTURE_EXTENSION(texture);
    RwRaster* raster = texture->raster;
    RwGameCubeRasterExt* rasterExt =
        RW_GAMECUBE_RASTER_EXTENSION(raster->parent);
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
        mipmap = (rasterFormat & 0x8000) != 0;
        GXInitTexObjCI(&textureExt->object, rasterExt->imageData,
                       (RwUInt16)raster->width, (RwUInt16)raster->height,
                       rasterExt->format,
                       _RwDlAddressConvTable[(texture->filter_flags >> 8) & 0xF],
                       _RwDlAddressConvTable[(texture->filter_flags >> 12) & 0xF],
                       mipmap, tlut);
        if ((RwUInt8)texture->filter_flags == 6 ||
            (RwUInt8)texture->filter_flags == 5) {
            minFilter = _RwDlFilterModeConvTable[4].minFilter;
            magFilter = _RwDlFilterModeConvTable[4].magFilter;
        } else {
            minFilter =
                _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].minFilter;
            magFilter =
                _RwDlFilterModeConvTable[(RwUInt8)texture->filter_flags].magFilter;
        }
    } else {
        RwBool mipmap = (rasterFormat & 0x8000) != 0;

        GXInitTexObj(&textureExt->object, rasterExt->imageData,
                     (RwUInt16)raster->width, (RwUInt16)raster->height,
                     rasterExt->format,
                     _RwDlAddressConvTable[(texture->filter_flags >> 8) & 0xF],
                     _RwDlAddressConvTable[(texture->filter_flags >> 12) & 0xF],
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

    if (texture == NULL) {
        texture = _RwDlTexture;
        _rwDlTextureSetRaster(texture, _RwDlRasterWhite, FALSE);
    }

    raster = texture->raster;
    rasterExt = RW_GAMECUBE_RASTER_EXTENSION(raster->parent);
    textureExt = RW_GAMECUBE_TEXTURE_EXTENSION(texture);
    rasterExt->token = _RwDlTokenCurrent;

    if (((RwUInt32)raster->format << 8 & 0x6000) != 0) {
        if ((textureExt->flags & 0x01000000) != 0) {
            _rwGameCubeTextureSetLOD(texture, 0.0f, TRUE, TRUE, 0,
                                     textureMap);
        } else if ((RwUInt16)texture->filter_flags !=
                   (RwUInt16)textureExt->flags) {
            RwInt32 maxAnisotropy = GXGetTexObjMaxAniso(&textureExt->object);
            RwUInt8 edgeLod = GXGetTexObjEdgeLOD(&textureExt->object);
            RwUInt8 biasClamp = GXGetTexObjBiasClamp(&textureExt->object);
            RwReal lodBias = GXGetTexObjLODBias(&textureExt->object);

            _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod,
                                     maxAnisotropy, textureMap);
        } else if ((textureExt->flags & 0x02000000) == 0 &&
                   textureMap != GXGetTexObjTlut(&textureExt->object)) {
            RwInt32 maxAnisotropy = GXGetTexObjMaxAniso(&textureExt->object);
            RwUInt8 edgeLod = GXGetTexObjEdgeLOD(&textureExt->object);
            RwUInt8 biasClamp = GXGetTexObjBiasClamp(&textureExt->object);
            RwReal lodBias = GXGetTexObjLODBias(&textureExt->object);

            _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod,
                                     maxAnisotropy, textureMap);
        }
        if ((textureExt->flags & 0x02000000) == 0) {
            GXLoadTlut(&rasterExt->tlut, textureMap);
        }
    } else if ((textureExt->flags & 0x01000000) != 0) {
        _rwGameCubeTextureSetLOD(texture, 0.0f, TRUE, TRUE, 0, 0);
    } else if ((RwUInt16)texture->filter_flags !=
               (RwUInt16)textureExt->flags) {
        RwInt32 maxAnisotropy = GXGetTexObjMaxAniso(&textureExt->object);
        RwUInt8 edgeLod = GXGetTexObjEdgeLOD(&textureExt->object);
        RwUInt8 biasClamp = GXGetTexObjBiasClamp(&textureExt->object);
        RwReal lodBias = GXGetTexObjLODBias(&textureExt->object);

        _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod,
                                 maxAnisotropy, 0);
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
