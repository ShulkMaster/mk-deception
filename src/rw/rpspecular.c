#include "libmkparticle/rw_engine.h"
#include "rw/bamateri.h"
#include "rw/batextur.h"
#include "rw/gcspecular.h"
#include "rw/rpworld_types.h"
#include "rw/rwplcore.h"
#include "rw/rwimage.h"
#include "rw/rwstream.h"

typedef struct SpecularGeometryPluginData {
    void* allocation;
    int materialIndex;
} SpecularGeometryPluginData;

extern void* ImagePixels;
extern float powf(float value, float power);
RwTexture* PhongTextures[3] = { 0, 0, 0 };
float PhongCoefficients[3] = { 10.0f, 25.0f, 40.0f };

int SpecularMaterialOffset = -1;

static unsigned int SpecularInstances;
static RwImage* Image;
int SpecularGeometryOffset;

static void specskin_close_images(void)
{
    int index;

    if (Image != 0) {
        RwImageDestroy(Image);
    }

    for (index = 0; index < 3; index++) {
        if (PhongTextures[index] != 0) {
            RwTextureDestroy(PhongTextures[index]);
        }
    }
}

static void* SpecularClose(void* instance, int offset, int size)
{
    if (--SpecularInstances == 0) {
        specskin_close_images();
    }
    return instance;
}

static int CreatePhongImage(int imageIndex, float coefficient)
{
    unsigned char* pixel = (unsigned char*)ImagePixels;
    unsigned int y;
    unsigned int x;

    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            float nx = 2.0f * ((float)x / 127.0f) - 1.0f;
            float ny = 2.0f * ((float)y / 127.0f) - 1.0f;
            float height;
            int intensity;
            unsigned char color;

            if (1.0f - nx * nx - ny * ny > 0.0f) {
                height = 1.0f - nx * nx - ny * ny;
            } else {
                height = 0.0f;
            }

            intensity = (int)(254.99f * powf(height, 0.5f * coefficient)) + 1;
            color = intensity > 255 ? 255 : (unsigned char)intensity;
            pixel[0] = color;
            pixel[1] = color;
            pixel[2] = color;
            pixel[3] = 255;
            pixel += 4;
        }
    }

    return RwRasterSetFromImage(PhongTextures[imageIndex]->raster, Image) != 0;
}

static int specskin_open_images(void)
{
    unsigned int index;

    Image = RwImageCreate(128, 128, 32);
    if (Image == 0) {
        return 0;
    }

    Image->stride = 512;
    Image->pixels = (unsigned char*)ImagePixels;

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

static void* SpecularOpen(void* instance, int offset, int size)
{
    if (SpecularInstances++ == 0) {
        specskin_open_images();
    }
    return instance;
}

static void* SpecularGeometryConstructor(void* object, int offset,
                                         int size)
{
    ((SpecularGeometryPluginData*)((unsigned char*)object +
                                   SpecularGeometryOffset))->allocation = 0;
    ((SpecularGeometryPluginData*)((unsigned char*)object +
                                   SpecularGeometryOffset))->materialIndex = -1;
    return object;
}

static void* SpecularGeometryDestructor(void* object, int offset,
                                        int size)
{
    SpecularGeometryPluginData* data = (SpecularGeometryPluginData*)(
        (unsigned char*)object + SpecularGeometryOffset);

    if (data->allocation != 0) {
        RwEngineInstance->fpFree(data->allocation);
    }
    return object;
}

static void* SpecularGeometryCopy(void* destination, const void* source,
                                  int offset, int size)
{
    SpecularGeometryPluginData* destinationData =
        (SpecularGeometryPluginData*)((unsigned char*)destination +
                                      SpecularGeometryOffset);
    const SpecularGeometryPluginData* sourceData =
        (const SpecularGeometryPluginData*)((const unsigned char*)source +
                                            SpecularGeometryOffset);

    if (destinationData->allocation != 0) {
        RwEngineInstance->fpFree(destinationData->allocation);
    }
    destinationData->allocation = sourceData->allocation;
    destinationData->materialIndex = sourceData->materialIndex;
    return destination;
}

static void* SpecularMaterialConstructor(void* object, int offset,
                                         int size)
{
    static const RwRGBA whiteColor = { 255, 255, 255, 255 };
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->texture = 0;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->saved_texture = 0;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->light = 0;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->frame = 0;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->clipValue = 0;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->shininess = 0.0f;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->tint = whiteColor;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->gloss = 0.0f;
    ((SpecularMaterialPluginData*)((unsigned char*)object +
                                   SpecularMaterialOffset))->flags.word = 0;
    return object;
}

static void* SpecularMaterialDestructor(void* object, int offset,
                                        int size)
{
    return object;
}

static void* SpecularMaterialCopy(void* destination, const void* source,
                                  int offset, int size)
{
    SpecularMaterialPluginData* destinationData =
        (SpecularMaterialPluginData*)((unsigned char*)destination +
                                      SpecularMaterialOffset);
    const SpecularMaterialPluginData* sourceData =
        (const SpecularMaterialPluginData*)((const unsigned char*)source +
                                            SpecularMaterialOffset);

    destinationData->frame = sourceData->frame;
    destinationData->light = sourceData->light;
    destinationData->texture = sourceData->texture;
    destinationData->shininess = sourceData->shininess;
    destinationData->tint = sourceData->tint;
    destinationData->flags.word = sourceData->flags.word;
    return destination;
}

static RwStream* SpecularMaterialStreamWrite(RwStream* stream,
                                             int binaryLength,
                                             const void* object,
                                             int offset, int size)
{
    const SpecularMaterialPluginData* data =
        (const SpecularMaterialPluginData*)((const unsigned char*)object +
                                            SpecularMaterialOffset);
    int texturePresent = data->texture != 0;

    if (RwStreamWrite(stream, &texturePresent, sizeof(texturePresent)) == 0) {
        return 0;
    }
    if (texturePresent && RwTextureStreamWrite(data->texture, stream) == 0) {
        return 0;
    }
    return stream;
}

static RwStream* SpecularMaterialStreamRead(RwStream* stream,
                                            int binaryLength, void* object,
                                            int offset, int size)
{
    SpecularMaterialPluginData* data = (SpecularMaterialPluginData*)(
        (unsigned char*)object + SpecularMaterialOffset);
    int texturePresent;
    unsigned int chunkLength;

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

static int SpecularMaterialStreamGetSize(const void* object,
                                             int offset, int size)
{
    const SpecularMaterialPluginData* data =
        (const SpecularMaterialPluginData*)((const unsigned char*)object +
                                            SpecularMaterialOffset);
    int streamSize = sizeof(int);

    if (data->texture != 0) {
        streamSize += RwTextureStreamGetSize(data->texture) + 12;
    }
    return streamSize;
}

int RpSpecularPluginAttach(void)
{
    int result;

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
