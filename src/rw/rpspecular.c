#include "libmkparticle/rw_engine.h"
#include "rw/batextur.h"
#include "rw/rpworld_types.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"

typedef struct SpecularGeometryPluginData {
    void* allocation;
    RwInt32 materialIndex;
} SpecularGeometryPluginData;

typedef struct SpecularColor {
    RwUInt8 red;
    RwUInt8 green;
    RwUInt8 blue;
    RwUInt8 alpha;
} SpecularColor;

typedef struct SpecularMaterialPluginData {
    void* light;
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* savedTexture;
    RwUInt8 savedSurface[0xC];
    RwInt32 clipValue;
    RwReal shininess;
    SpecularColor tint;
    RwReal gloss;
    RwUInt32 flags;
} SpecularMaterialPluginData;

extern void* ImagePixels;
extern RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
extern RwBool RwImageDestroy(RwImage* image);
extern RwReal powf(RwReal value, RwReal power);
extern RwInt32 RpMaterialRegisterPlugin(
    RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
extern RwInt32 RpMaterialRegisterPluginStream(
    RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);

RwTexture* PhongTextures[3] = { 0, 0, 0 };
RwReal PhongCoefficients[3] = { 10.0f, 25.0f, 40.0f };

RwInt32 SpecularMaterialOffset = -1;

static RwUInt32 SpecularInstances;
static RwImage* Image;
RwInt32 SpecularGeometryOffset;

static void specskin_close_images(void)
{
    RwInt32 index;

    if (Image != 0) {
        RwImageDestroy(Image);
    }

    for (index = 0; index < 3; index++) {
        if (PhongTextures[index] != 0) {
            RwTextureDestroy(PhongTextures[index]);
        }
    }
}

static void* SpecularClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (--SpecularInstances == 0) {
        specskin_close_images();
    }
    return instance;
}

static RwBool CreatePhongImage(RwInt32 imageIndex, RwReal coefficient)
{
    RwUInt8* pixel = (RwUInt8*)ImagePixels;
    RwUInt32 y;
    RwUInt32 x;

    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            RwReal nx = 2.0f * ((RwReal)x / 127.0f) - 1.0f;
            RwReal ny = 2.0f * ((RwReal)y / 127.0f) - 1.0f;
            RwReal height;
            RwInt32 intensity;
            RwUInt8 color;

            if (1.0f - nx * nx - ny * ny > 0.0f) {
                height = 1.0f - nx * nx - ny * ny;
            } else {
                height = 0.0f;
            }

            intensity = (RwInt32)(254.99f * powf(height, 0.5f * coefficient)) + 1;
            color = intensity > 255 ? 255 : (RwUInt8)intensity;
            pixel[0] = color;
            pixel[1] = color;
            pixel[2] = color;
            pixel[3] = 255;
            pixel += 4;
        }
    }

    return RwRasterSetFromImage(PhongTextures[imageIndex]->raster, Image) != 0;
}

static RwBool specskin_open_images(void)
{
    RwUInt32 index;

    Image = RwImageCreate(128, 128, 32);
    if (Image == 0) {
        return 0;
    }

    Image->stride = 512;
    Image->pixels = (RwUInt8*)ImagePixels;

    for (index = 0; index < 3; index++) {
        RwRaster* raster = RwRasterCreate(128, 128, 32, 0x604);
        if (raster == 0) {
            return 0;
        }

        PhongTextures[index] = RwTextureCreate(raster);
        if (PhongTextures[index] == 0) {
            return 0;
        }

        PhongTextures[index]->filter_flags =
                (PhongTextures[index]->filter_flags & ~0xFFU) | 2;
        PhongTextures[index]->filter_flags =
            (PhongTextures[index]->filter_flags & ~0xFF00U) | 0x1100;
    }

    for (index = 0; index < 3; index++) {
        CreatePhongImage(index, PhongCoefficients[index]);
    }

    return 1;
}

static void* SpecularOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (SpecularInstances++ == 0) {
        specskin_open_images();
    }
    return instance;
}

static void* SpecularGeometryConstructor(void* object, RwInt32 offset,
                                         RwInt32 size)
{
    ((SpecularGeometryPluginData*)((RwUInt8*)object +
                                   SpecularGeometryOffset))->allocation = 0;
    ((SpecularGeometryPluginData*)((RwUInt8*)object +
                                   SpecularGeometryOffset))->materialIndex = -1;
    return object;
}

static void* SpecularGeometryDestructor(void* object, RwInt32 offset,
                                        RwInt32 size)
{
    SpecularGeometryPluginData* data = (SpecularGeometryPluginData*)(
        (RwUInt8*)object + SpecularGeometryOffset);

    if (data->allocation != 0) {
        RwEngineInstance->fpFree(data->allocation);
    }
    return object;
}

static void* SpecularGeometryCopy(void* destination, const void* source,
                                  RwInt32 offset, RwInt32 size)
{
    SpecularGeometryPluginData* destinationData =
        (SpecularGeometryPluginData*)((RwUInt8*)destination +
                                      SpecularGeometryOffset);
    const SpecularGeometryPluginData* sourceData =
        (const SpecularGeometryPluginData*)((const RwUInt8*)source +
                                            SpecularGeometryOffset);

    if (destinationData->allocation != 0) {
        RwEngineInstance->fpFree(destinationData->allocation);
    }
    destinationData->allocation = sourceData->allocation;
    destinationData->materialIndex = sourceData->materialIndex;
    return destination;
}

static void* SpecularMaterialConstructor(void* object, RwInt32 offset,
                                         RwInt32 size)
{
    static const SpecularColor whiteColor = { 255, 255, 255, 255 };
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->texture = 0;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->savedTexture = 0;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->light = 0;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->frame = 0;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->clipValue = 0;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->shininess = 0.0f;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->tint = whiteColor;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->gloss = 0.0f;
    ((SpecularMaterialPluginData*)((RwUInt8*)object +
                                   SpecularMaterialOffset))->flags = 0;
    return object;
}

static void* SpecularMaterialDestructor(void* object, RwInt32 offset,
                                        RwInt32 size)
{
    return object;
}

static void* SpecularMaterialCopy(void* destination, const void* source,
                                  RwInt32 offset, RwInt32 size)
{
    SpecularMaterialPluginData* destinationData =
        (SpecularMaterialPluginData*)((RwUInt8*)destination +
                                      SpecularMaterialOffset);
    const SpecularMaterialPluginData* sourceData =
        (const SpecularMaterialPluginData*)((const RwUInt8*)source +
                                            SpecularMaterialOffset);

    destinationData->frame = sourceData->frame;
    destinationData->light = sourceData->light;
    destinationData->texture = sourceData->texture;
    destinationData->shininess = sourceData->shininess;
    destinationData->tint = sourceData->tint;
    destinationData->flags = sourceData->flags;
    return destination;
}

static RwStream* SpecularMaterialStreamWrite(RwStream* stream,
                                             RwInt32 binaryLength,
                                             const void* object,
                                             RwInt32 offset, RwInt32 size)
{
    const SpecularMaterialPluginData* data =
        (const SpecularMaterialPluginData*)((const RwUInt8*)object +
                                            SpecularMaterialOffset);
    RwInt32 texturePresent = data->texture != 0;

    if (RwStreamWrite(stream, &texturePresent, sizeof(texturePresent)) == 0) {
        return 0;
    }
    if (texturePresent && RwTextureStreamWrite(data->texture, stream) == 0) {
        return 0;
    }
    return stream;
}

static RwStream* SpecularMaterialStreamRead(RwStream* stream,
                                            RwInt32 binaryLength, void* object,
                                            RwInt32 offset, RwInt32 size)
{
    SpecularMaterialPluginData* data = (SpecularMaterialPluginData*)(
        (RwUInt8*)object + SpecularMaterialOffset);
    RwInt32 texturePresent;
    RwUInt32 chunkLength;

    if (RwStreamRead(stream, &texturePresent, sizeof(texturePresent)) !=
        sizeof(texturePresent)) {
        return 0;
    }
    if (texturePresent) {
        if (!RwStreamFindChunk(stream, 6, &chunkLength, 0)) {
            return 0;
        }
        data->texture = RwTextureStreamRead(stream);
        if (data->texture == 0) {
            return 0;
        }
    }
    return stream;
}

static RwInt32 SpecularMaterialStreamGetSize(const void* object,
                                             RwInt32 offset, RwInt32 size)
{
    const SpecularMaterialPluginData* data =
        (const SpecularMaterialPluginData*)((const RwUInt8*)object +
                                            SpecularMaterialOffset);
    RwInt32 streamSize = sizeof(RwInt32);

    if (data->texture != 0) {
        streamSize += RwTextureStreamGetSize(data->texture) + 12;
    }
    return streamSize;
}

RwBool RpSpecularPluginAttach(void)
{
    RwInt32 result;

    if (RwEngineRegisterPlugin(0, 0xFF, SpecularOpen, SpecularClose) < 0) {
        return 0;
    }

    SpecularGeometryOffset = RpGeometryRegisterPlugin(
        sizeof(SpecularGeometryPluginData), 0x1FF,
        SpecularGeometryConstructor, SpecularGeometryDestructor,
        SpecularGeometryCopy);
    if (SpecularGeometryOffset < 0) {
        return 0;
    }

    SpecularMaterialOffset = RpMaterialRegisterPlugin(
        sizeof(SpecularMaterialPluginData), 0x1FF,
        SpecularMaterialConstructor, SpecularMaterialDestructor,
        SpecularMaterialCopy);
    if (SpecularMaterialOffset < 0) {
        return 0;
    }

    result = RpMaterialRegisterPluginStream(
        0x1FF, SpecularMaterialStreamRead, SpecularMaterialStreamWrite,
        SpecularMaterialStreamGetSize);
    if (result < 0) {
        return 0;
    }
    return 1;
}
