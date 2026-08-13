#include "dolphin/gx.h"
#include "rw/dltextur.h"
#include "rw/dltoken.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct DlFilterMode {
    int minFilter;
    int magFilter;
} DlFilterMode;

int _RwGameCubeTextureExtOffset;
extern RwRaster* _RwDlRasterWhite;
extern RwTexture* _RwDlTexture;

static DlFilterMode _RwDlFilterModeConvTable[7] = {
    {0, 0}, {0, 0}, {1, 1}, {2, 0}, {3, 1}, {4, 0}, {5, 1},
};
static int _RwDlAddressConvTable[5] = {0, 1, 2, 0, 0};
static RwTexture* _RwDlTextureCache[8] = {0, 0, 0, 0,
                                          0, 0, 0, 0};

static void* _rwDlTextureConst(void* object, int offset, int size)
{
    RwGameCubeTextureExt* textureExt =
        (RwGameCubeTextureExt*)((unsigned char*)object +
                                _RwGameCubeTextureExtOffset);

    textureExt->flags = 0x01000000;
    return object;
}

static void _rwDlTextureDest(RwTexture* texture)
{
    int index;

    for (index = 0; index < 8; index++) {
        if (texture == _RwDlTextureCache[index]) {
            _RwDlTextureCache[index] = 0;
        }
    }
}

void _rwDlTextureCacheInit(void)
{
    unsigned int index = 8;

    while (index-- != 0) {
        _RwDlTextureCache[index] = 0;
    }
}

void _rwDlTexturePluginAttach(void)
{
    _RwGameCubeTextureExtOffset = RwTextureRegisterPlugin(
        sizeof(RwGameCubeTextureExt), 0x40C,
        _rwDlTextureConst,
        (RwPluginObjectDestructor)_rwDlTextureDest, 0);
}

static void _rwGameCubeTextureSetLOD(RwTexture* texture, float lodBias,
                                     unsigned int biasClamp, unsigned int edgeLod,
                                     int maxAnisotropy,
                                     unsigned int textureMap)
{
    RwGameCubeTextureExt* textureExt =
        RwGameCubeTextureExtension(texture);
    RwRaster* raster = texture->raster;
    RwGameCubeRasterExt* rasterExt =
        RwGameCubeRasterExtension(raster->parent);
    int rasterFormat = (unsigned char)raster->format << 8;
    int minFilter;
    int magFilter;

    if ((rasterFormat & 0x6000) != 0) {
        unsigned int tlut;
        int mipmap;

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
                       (unsigned short)raster->width, (unsigned short)raster->height,
                       rasterExt->format,
                       _RwDlAddressConvTable[(texture->filter_flags & 0xF00) >> 8],
                       _RwDlAddressConvTable[(texture->filter_flags & 0xF000) >> 12],
                       mipmap, tlut);
        if ((int)(unsigned char)texture->filter_flags == 6 ||
            (int)(unsigned char)texture->filter_flags == 5) {
            minFilter = _RwDlFilterModeConvTable[4].minFilter;
            magFilter = _RwDlFilterModeConvTable[4].magFilter;
        } else {
            minFilter =
                _RwDlFilterModeConvTable[(unsigned char)texture->filter_flags].minFilter;
            magFilter =
                _RwDlFilterModeConvTable[(unsigned char)texture->filter_flags].magFilter;
        }
    } else {
        int mipmap;

        if ((rasterFormat & 0x8000) != 0) {
            mipmap = 1;
        } else {
            mipmap = 0;
        }

        GXInitTexObj(&textureExt->object, rasterExt->imageData,
                     (unsigned short)raster->width, (unsigned short)raster->height,
                     rasterExt->format,
                     _RwDlAddressConvTable[(texture->filter_flags & 0xF00) >> 8],
                     _RwDlAddressConvTable[(texture->filter_flags & 0xF000) >> 12],
                     mipmap);
        minFilter =
            _RwDlFilterModeConvTable[(unsigned char)texture->filter_flags].minFilter;
        magFilter =
            _RwDlFilterModeConvTable[(unsigned char)texture->filter_flags].magFilter;
    }

    GXInitTexObjLOD(&textureExt->object, minFilter, magFilter, 0.0f,
                    (float)rasterExt->maxLod, lodBias, (unsigned char)biasClamp,
                    (unsigned char)edgeLod, maxAnisotropy);
    textureExt->flags =
        ((textureExt->flags & 0xFFFF0000) |
         (unsigned short)texture->filter_flags) &
        ~0x01000000;
}

void _rwDlTextureSet(RwTexture* texture, unsigned int textureMap)
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

    if ((int)((((unsigned int)raster->format & 0xFF) << 8) & 0x6000) != 0) {
        if ((textureExt->flags & 0x01000000) != 0) {
            _rwGameCubeTextureSetLOD(texture, 0.0f, 1, 1, 0,
                                     textureMap);
        } else if ((unsigned short)texture->filter_flags !=
                   (unsigned short)textureExt->flags) {
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
    } else if ((unsigned short)texture->filter_flags !=
               (unsigned short)textureExt->flags) {
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

void RwGameCubeTextureSetLOD(RwTexture* texture, float lodBias,
                             int biasClamp, int edgeLod,
                             int maxAnisotropy)
{
    _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod,
                             maxAnisotropy, 0);
}
