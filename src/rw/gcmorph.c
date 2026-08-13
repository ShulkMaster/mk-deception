#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/rpworld_types.h"

static unsigned char vtxFmtTypeConvTable[5] = {4, 6, 5, 7, 0};
static unsigned char vtxFmtSizeConvTable[5] = {1, 1, 2, 2, 4};
static unsigned int interpGQR6;


static void _rwDlV3dInterpPosGQRSetup(const RpGameCubeVtxFmt* format,
                                     int* elementSize)
{
    unsigned int value = 0;

    if (format != 0) {
        value = ((unsigned int)format->positionFraction << 8) |
            vtxFmtTypeConvTable[format->positionType];
        *elementSize = vtxFmtSizeConvTable[format->positionType];
    } else {
        *elementSize = sizeof(float);
    }



    interpGQR6 = value;
}

static void _rwDlV3dInterpNormGQRSetup(const RpGameCubeVtxFmt* format,
                                      int* elementSize,
                                      int* extraSize)
{
    unsigned int value = 0;
    int extra;

    if (format != 0) {
        unsigned char typeTable[5] = {4, 6, 5, 7, 0};
        unsigned char sizeTable[5] = {1, 1, 2, 2, 4};
        unsigned char fractionTable[5] = {0, 6, 0, 14, 0};

        value = ((unsigned int)fractionTable[format->normalType] << 8) |
            typeTable[format->normalType];
        *elementSize = sizeTable[format->normalType];
        if (format->normalMode != 0)
            extra = *elementSize * 6;
        else
            extra = 0;
        *extraSize = extra;
    } else {
        *elementSize = sizeof(float);
        *extraSize = 0;
    }

    interpGQR6 = value;
}

static int ClampQuantized(float value, int minimum,
                              int maximum)
{
    if (value < (float)minimum)
        return minimum;
    if (value > (float)maximum)
        return maximum;
    return (int)value;
}

static void _rwDlV3dInterp(void* destination, const RwV3d* source,
                           const RwV3d* target, const float* position,
                           int count, unsigned int sourceElementSize,
                           unsigned int destinationElementSize,
                           int extraSize)
{




    unsigned char* output = destination;
    unsigned int type = interpGQR6 & 7;
    unsigned int fraction = (interpGQR6 >> 8) & 0x3F;
    float scale = (float)(1U << fraction);
    int vertex;

    for (vertex = 0; vertex < count; vertex++) {
        const float* first = (const float*)source;
        const float* second = (const float*)target;
        int component;

        for (component = 0; component < 3; component++) {
            float value = first[component] +
                (second[component] - first[component]) * *position;

            switch (type) {
            case 4:
                *(unsigned char*)output =
                    ClampQuantized(value * scale, 0, 0xFF);
                break;
            case 6:
                *(signed char*)output =
                    ClampQuantized(value * scale, -0x80, 0x7F);
                break;
            case 5:
                *(unsigned short*)output =
                    ClampQuantized(value * scale, 0, 0xFFFF);
                break;
            case 7:
                *(short*)output =
                    ClampQuantized(value * scale, -0x8000, 0x7FFF);
                break;
            default:
                *(float*)output = value;
                break;
            }
            output += destinationElementSize;
        }
        output += extraSize;
        source = (const RwV3d*)((const unsigned char*)source +
            sourceElementSize * 3);
        target = (const RwV3d*)((const unsigned char*)target +
            sourceElementSize * 3);
    }
}

void _rxGCInstanceMorphUpdate(RpGeometry* geometry,
                              RwGameCubeVertexBuffer* vertexBuffer,
                              const RpInterpolator* interpolator)
{



    int startMorphTarget = interpolator->startMorphTarget;
    int endMorphTarget = interpolator->endMorphTarget;
    void* positions = vertexBuffer->arrays[0].data;
    RpMorphTarget* source =
        &geometry->morphTarget[startMorphTarget];
    RpMorphTarget* target =
        &geometry->morphTarget[endMorphTarget];
    float position = interpolator->time * interpolator->recipTime;
    int elementSize;
    int extraSize;

    _rwDlV3dInterpPosGQRSetup(*(RpGameCubeVtxFmt**)(
        (unsigned char*)geometry + _rpDlGeomVtxFmtOffset), &elementSize);
    _rwDlV3dInterp(positions, source->verts,
                   target->verts, &position, geometry->numVertices,
                   sizeof(float), elementSize, 0);
    DCFlushRange(positions,
                 geometry->numVertices * (elementSize * 3));

    if ((geometry->flags & 0x10) != 0) {
        void* normals = vertexBuffer->arrays[1].data;

        _rwDlV3dInterpNormGQRSetup(*(RpGameCubeVtxFmt**)(
            (unsigned char*)geometry + _rpDlGeomVtxFmtOffset), &elementSize,
            &extraSize);
        _rwDlV3dInterp(normals, source->normals,
                       target->normals, &position, geometry->numVertices,
                       sizeof(float), elementSize, extraSize);
        DCFlushRange(normals,
                     geometry->numVertices *
                         (elementSize * 3 + extraSize));
    }
    GXInvalidateVtxCache();
}
