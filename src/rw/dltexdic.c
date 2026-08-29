#include "dolphin/gx.h"
#include "runtime/cstring.h"
#include "rw/dltextur.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/native_internal.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

extern unsigned int _rwDlRasterGetSize(RwRaster* raster);
extern int _rwDlTextureRasterCreate(RwRaster* raster,
                                       unsigned char numLevels);
extern void DCFlushRange(void* address, unsigned int length);
int _rwDlNativeTextureGetSize(unsigned int* size, void* object, int unused)
{
    unsigned int result;
    RwRaster* raster;

    result = sizeof(GameCubeNativeTextureHeader) + 12;
    raster = ((RwTexture*)object)->raster;
    if (raster == 0) {
        *size = result;
        return 1;
    }

    result += sizeof(GameCubeNativeRasterHeader);
    if ((int)(((unsigned int)(unsigned char)raster->format << 8) & 0x6000) != 0) {
        result += (1U << raster->depth) * sizeof(unsigned short);
    }
    result += sizeof(unsigned int);
    result += _rwDlRasterGetSize(raster);
    *size = result;
    return 1;
}



int _rwDlNativeTextureWrite(RwStream* stream, void* object, int unused)
{
    unsigned int rasterSize;
    unsigned int bytesRemaining;
    GameCubeNativeTextureHeader textureHeader;
    GameCubeNativeRasterHeader rasterHeader;
    RwGameCubeTextureExt* textureExt;
    RwGameCubeRasterExt* rasterExt;
    RwRaster* raster;

    _rwDlNativeTextureGetSize(&bytesRemaining, object, 0);
    bytesRemaining -= 12;
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, bytesRemaining,
                                           0x36003, 0xFFFF) == 0) {
        return 0;
    }

    textureHeader.platform = 6;
    textureHeader.filterAddressing =
        (((((RwTexture*)object)->filter_flags & 0xF000) >> 12 << 12) & 0xF000) |
        ((unsigned char)(((RwTexture*)object)->filter_flags & 0xFF) |
         ((((RwTexture*)object)->filter_flags & 0xF00) >> 8 << 8) & 0xF00);
    textureExt = RwGameCubeTextureExtension((RwTexture*)object);
    if ((textureExt->flags & 0x01000000) != 0) {
        textureHeader.maxAnisotropy = 0;
        textureHeader.biasClamp = 1;
        textureHeader.edgeLod = 1;
        textureHeader.lodBias = 0.0f;
    } else {
        textureHeader.maxAnisotropy = GXGetTexObjMaxAniso(&textureExt->object);
        textureHeader.biasClamp = GXGetTexObjBiasClamp(&textureExt->object);
        textureHeader.edgeLod = GXGetTexObjEdgeLOD(&textureExt->object);
        textureHeader.lodBias = GXGetTexObjLODBias(&textureExt->object);
    }
    memcpy(textureHeader.name, ((RwTexture*)object)->name, sizeof(textureHeader.name));
    memcpy(textureHeader.mask, ((RwTexture*)object)->mask, sizeof(textureHeader.mask));
    if (RwStreamWrite(stream, &textureHeader, sizeof(textureHeader)) == 0) {
        return 0;
    }
    bytesRemaining -= sizeof(textureHeader);

    raster = ((RwTexture*)object)->raster;
    rasterExt = RwGameCubeRasterExtension(raster->parent);
    rasterHeader.format =
        ((unsigned int)(unsigned char)raster->format << 8) | raster->type;
    rasterHeader.width = (unsigned short)raster->width;
    rasterHeader.height = (unsigned short)raster->height;
    rasterHeader.depth = (unsigned char)raster->depth;
    rasterHeader.numLevels = (unsigned char)RwRasterGetNumLevels(raster);
    rasterHeader.tileMode = (unsigned char)rasterExt->format;
    rasterHeader.paletteFormat = (unsigned char)rasterExt->paletteFormat;
    rasterHeader.hasAlpha = rasterExt->hasAlpha & 1;
    if (RwStreamWrite(stream, &rasterHeader, sizeof(rasterHeader)) == 0) {
        return 0;
    }
    bytesRemaining -= sizeof(rasterHeader);

    if ((int)(((unsigned int)(unsigned char)raster->format << 8) & 0x6000) != 0) {
        unsigned int paletteSize = (1U << raster->depth) * sizeof(unsigned short);

        if (RwStreamWrite(stream, rasterExt->paletteData, paletteSize) ==
            0) {
            return 0;
        }
        bytesRemaining -= paletteSize;
    }

    rasterSize = _rwDlRasterGetSize(raster);
    if (RwStreamWrite(stream, &rasterSize, sizeof(rasterSize)) == 0) {
        return 0;
    }
    bytesRemaining -= sizeof(rasterSize);
    if (RwStreamWrite(stream, rasterExt->imageData, rasterSize) == 0) {
        return 0;
    }
    bytesRemaining -= rasterSize;
    return 1;
}





int _rwDlNativeTextureRead(RwStream* stream, void* object, int unused)
{
    unsigned int chunkLength;
    unsigned int version;
    GameCubeNativeTextureHeader textureHeader;
    GameCubeNativeRasterHeader rasterHeader;
    RwRaster* raster;
    RwGameCubeRasterExt* rasterExt;
    unsigned int rasterSize;
    unsigned int rasterFormatBit;
    RwTexture* result;

    if (!RwStreamFindChunk(stream, 1, &chunkLength, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        return 0;
    }
    if (RwStreamRead(stream, &textureHeader, sizeof(textureHeader)) !=
        sizeof(textureHeader)) {
        return 0;
    }
    if (textureHeader.platform != 6) {
        return 0;
    }
    if (RwStreamRead(stream, &rasterHeader, sizeof(rasterHeader)) !=
        sizeof(rasterHeader)) {
        return 0;
    }

    raster = RwRasterCreate(rasterHeader.width, rasterHeader.height,
                            rasterHeader.depth, rasterHeader.format | 0x80);
    if (raster == 0) {
        return 0;
    }
    rasterExt = RwGameCubeRasterExtension(raster);
    rasterExt->format = rasterHeader.tileMode;
    rasterExt->paletteFormat = rasterHeader.paletteFormat;
    if (rasterHeader.hasAlpha != 0) {
        rasterExt->hasAlpha = 1;
    } else {
        rasterExt->hasAlpha = 0;
    }
    if (!_rwDlTextureRasterCreate(raster, rasterHeader.numLevels)) {
        RwRasterDestroy(raster);
        return 0;
    }
    raster->flags &= ~0x80;

    if ((int)(((unsigned int)(unsigned char)raster->format << 8) & 0x6000) != 0) {
        unsigned int paletteSize = (1U << raster->depth) * sizeof(unsigned short);

        if (RwStreamRead(stream, rasterExt->paletteData, paletteSize) !=
            paletteSize) {
            return 0;
        }
        DCFlushRange(rasterExt->paletteData,
                     (1U << raster->depth) * sizeof(unsigned short));
    }

    rasterFormatBit = raster->format & 0x10;
    raster->format &= ~rasterFormatBit;
    if (RwStreamRead(stream, &rasterSize, sizeof(rasterSize)) !=
        sizeof(rasterSize)) {
        return 0;
    }
    if (RwStreamRead(stream, rasterExt->imageData, rasterSize) != rasterSize) {
        return 0;
    }
    DCFlushRange(rasterExt->imageData, rasterSize);
    GXInvalidateTexAll();
    raster->format |= rasterFormatBit;

    result = RwTextureCreate(raster);
    if (result == 0) {
        RwRasterDestroy(raster);
        return 0;
    }
    result->filter_flags =
        (result->filter_flags & ~0xFF) |
        (unsigned char)textureHeader.filterAddressing;
    result->filter_flags =
        (result->filter_flags & ~0xF00) |
        ((((textureHeader.filterAddressing >> 8) & 0xF) << 8) & 0xF00);
    result->filter_flags =
        (result->filter_flags & ~0xF000) |
        ((((textureHeader.filterAddressing >> 12) & 0xF) << 12) & 0xF000);
    RwTextureSetName(result, textureHeader.name);
    RwTextureSetMaskName(result, textureHeader.mask);
    RwGameCubeTextureSetLOD(result, textureHeader.lodBias,
                            textureHeader.biasClamp, textureHeader.edgeLod,
                            textureHeader.maxAnisotropy);
    *(RwTexture**)object = result;
    return 1;
}
