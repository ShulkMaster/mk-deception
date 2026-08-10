#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

extern void* memcpy(void* destination, const void* source, RwUInt32 size);

typedef struct RwGameCubeNativeTextureHeader {
    RwInt32 platform;
    RwUInt32 filterAddressing;
    RwInt32 maxAnisotropy;
    RwInt32 biasClamp;
    RwInt32 edgeLod;
    RwReal lodBias;
    RwChar name[32];
    RwChar mask[32];
} RwGameCubeNativeTextureHeader;

typedef struct RwGameCubeNativeRasterHeader {
    RwInt32 format;
    RwUInt16 width;
    RwUInt16 height;
    RwUInt8 depth;
    RwUInt8 numLevels;
    RwUInt8 tileMode;
    RwUInt8 paletteFormat;
    RwInt32 hasAlpha;
} RwGameCubeNativeRasterHeader;

extern RwUInt32 _rwDlRasterGetSize(RwRaster* raster);
extern RwBool _rwDlTextureRasterCreate(RwRaster* raster,
                                       RwUInt8 numLevels);
extern void DCFlushRange(void* address, RwUInt32 length);
extern void GXInvalidateTexAll(void);

RwBool _rwDlNativeTextureGetSize(RwUInt32* size, void* object, RwInt32 unused)
{
    RwUInt32 result;
    RwRaster* raster;

    result = sizeof(RwGameCubeNativeTextureHeader) + 12;
    raster = ((RwTexture*)object)->raster;
    if (raster == 0) {
        *size = result;
        return 1;
    }

    result += sizeof(RwGameCubeNativeRasterHeader);
    if ((RwInt32)(((RwUInt32)(RwUInt8)raster->format << 8) & 0x6000) != 0) {
        result += (1U << raster->depth) * sizeof(RwUInt16);
    }
    result += sizeof(RwUInt32);
    result += _rwDlRasterGetSize(raster);
    *size = result;
    return 1;
}



RwBool _rwDlNativeTextureWrite(RwStream* stream, void* object, RwInt32 unused)
{
    RwUInt32 rasterSize;
    RwUInt32 bytesRemaining;
    RwGameCubeNativeTextureHeader textureHeader;
    RwGameCubeNativeRasterHeader rasterHeader;
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
        ((RwUInt8)(((RwTexture*)object)->filter_flags & 0xFF) |
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
        ((RwUInt32)(RwUInt8)raster->format << 8) | raster->type;
    rasterHeader.width = (RwUInt16)raster->width;
    rasterHeader.height = (RwUInt16)raster->height;
    rasterHeader.depth = (RwUInt8)raster->depth;
    rasterHeader.numLevels = (RwUInt8)RwRasterGetNumLevels(raster);
    rasterHeader.tileMode = (RwUInt8)rasterExt->format;
    rasterHeader.paletteFormat = (RwUInt8)rasterExt->paletteFormat;
    rasterHeader.hasAlpha = rasterExt->hasAlpha & 1;
    if (RwStreamWrite(stream, &rasterHeader, sizeof(rasterHeader)) == 0) {
        return 0;
    }
    bytesRemaining -= sizeof(rasterHeader);

    if ((RwInt32)(((RwUInt32)(RwUInt8)raster->format << 8) & 0x6000) != 0) {
        RwUInt32 paletteSize = (1U << raster->depth) * sizeof(RwUInt16);

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





RwBool _rwDlNativeTextureRead(RwStream* stream, void* object, RwInt32 unused)
{
    RwUInt32 chunkLength;
    RwUInt32 version;
    RwGameCubeNativeTextureHeader textureHeader;
    RwGameCubeNativeRasterHeader rasterHeader;
    RwRaster* raster;
    RwGameCubeRasterExt* rasterExt;
    RwUInt32 rasterSize;
    RwUInt32 rasterFormatBit;
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

    if ((RwInt32)(((RwUInt32)(RwUInt8)raster->format << 8) & 0x6000) != 0) {
        RwUInt32 paletteSize = (1U << raster->depth) * sizeof(RwUInt16);

        if (RwStreamRead(stream, rasterExt->paletteData, paletteSize) !=
            paletteSize) {
            return 0;
        }
        DCFlushRange(rasterExt->paletteData,
                     (1U << raster->depth) * sizeof(RwUInt16));
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
        (RwUInt8)textureHeader.filterAddressing;
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
