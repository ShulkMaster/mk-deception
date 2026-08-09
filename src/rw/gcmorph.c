#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/rpworld_types.h"

static RwUInt8 vtxFmtTypeConvTable[5] = {4, 6, 5, 7, 0};
static RwUInt8 vtxFmtSizeConvTable[5] = {1, 1, 2, 2, 4};
static RwUInt32 interpGQR6;

extern RwInt32 _rpDlGeomVtxFmtOffset;

static void _rwDlV3dInterpPosGQRSetup(const RpGameCubeVtxFmt* format,
                                     RwInt32* elementSize)
{
    RwUInt32 value = 0;

    if (format != NULL) {
        value = ((RwUInt32)format->positionFraction << 8) |
            vtxFmtTypeConvTable[format->positionType];
        *elementSize = vtxFmtSizeConvTable[format->positionType];
    } else {
        *elementSize = sizeof(RwReal);
    }
    /* Portable shadow of GQR6. Retail writes this value directly to the SPR;
     * the scalar interpolator below consumes the same type/fraction fields. */
    interpGQR6 = value;
}

static void _rwDlV3dInterpNormGQRSetup(const RpGameCubeVtxFmt* format,
                                      RwInt32* elementSize,
                                      RwInt32* extraSize)
{
    RwUInt32 value = 0;
    RwInt32 extra;

    if (format != NULL) {
        RwUInt8 typeTable[5] = {4, 6, 5, 7, 0};
        RwUInt8 sizeTable[5] = {1, 1, 2, 2, 4};
        RwUInt8 fractionTable[5] = {0, 6, 0, 14, 0};

        value = ((RwUInt32)fractionTable[format->normalType] << 8) |
            typeTable[format->normalType];
        *elementSize = sizeTable[format->normalType];
        if (format->normalMode != 0)
            extra = *elementSize * 6;
        else
            extra = 0;
        *extraSize = extra;
    } else {
        *elementSize = sizeof(RwReal);
        *extraSize = 0;
    }
    interpGQR6 = value;
}

static RwInt32 ClampQuantized(RwReal value, RwInt32 minimum,
                              RwInt32 maximum)
{
    if (value < (RwReal)minimum)
        return minimum;
    if (value > (RwReal)maximum)
        return maximum;
    return (RwInt32)value;
}

static void _rwDlV3dInterp(void* destination, const RwV3d* source,
                           const RwV3d* target, const RwReal* position,
                           RwInt32 count, RwUInt32 sourceElementSize,
                           RwUInt32 destinationElementSize,
                           RwInt32 extraSize)
{
    RwUInt8* output = destination;
    RwUInt32 type = interpGQR6 & 7;
    RwUInt32 fraction = (interpGQR6 >> 8) & 0x3F;
    RwReal scale = (RwReal)(1U << fraction);
    RwInt32 vertex;

    for (vertex = 0; vertex < count; vertex++) {
        const RwReal* first = (const RwReal*)source;
        const RwReal* second = (const RwReal*)target;
        RwInt32 component;

        for (component = 0; component < 3; component++) {
            RwReal value = first[component] +
                (second[component] - first[component]) * *position;

            switch (type) {
            case 4:
                *(RwUInt8*)output =
                    ClampQuantized(value * scale, 0, 0xFF);
                break;
            case 6:
                *(signed char*)output =
                    ClampQuantized(value * scale, -0x80, 0x7F);
                break;
            case 5:
                *(RwUInt16*)output =
                    ClampQuantized(value * scale, 0, 0xFFFF);
                break;
            case 7:
                *(RwInt16*)output =
                    ClampQuantized(value * scale, -0x8000, 0x7FFF);
                break;
            default:
                *(RwReal*)output = value;
                break;
            }
            output += destinationElementSize;
        }
        output += extraSize;
        source = (const RwV3d*)((const RwUInt8*)source +
            sourceElementSize * 3);
        target = (const RwV3d*)((const RwUInt8*)target +
            sourceElementSize * 3);
    }
}

void _rxGCInstanceMorphUpdate(RpGeometry* geometry,
                              RwGameCubeVertexBuffer* vertexBuffer,
                              const RpInterpolator* interpolator)
{
    RwInt32 startMorphTarget = interpolator->startMorphTarget;
    RwInt32 endMorphTarget = interpolator->endMorphTarget;
    void* positions = vertexBuffer->arrays[0].data;
    RpMorphTarget* source =
        &geometry->morphTarget[startMorphTarget];
    RpMorphTarget* target =
        &geometry->morphTarget[endMorphTarget];
    RwReal position = interpolator->time * interpolator->recipTime;
    RwInt32 elementSize;
    RwInt32 extraSize;

    _rwDlV3dInterpPosGQRSetup(*(RpGameCubeVtxFmt**)(
        (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset), &elementSize);
    _rwDlV3dInterp(positions, source->verts,
                   target->verts, &position, geometry->numVertices,
                   sizeof(RwReal), elementSize, 0);
    DCFlushRange(positions,
                 geometry->numVertices * elementSize * 3);

    if ((geometry->flags & 0x10) != 0) {
        void* normals = vertexBuffer->arrays[1].data;

        _rwDlV3dInterpNormGQRSetup(*(RpGameCubeVtxFmt**)(
            (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset), &elementSize,
            &extraSize);
        _rwDlV3dInterp(normals, source->normals,
                       target->normals, &position, geometry->numVertices,
                       sizeof(RwReal), elementSize, extraSize);
        DCFlushRange(normals,
                     geometry->numVertices *
                         (elementSize * 3 + extraSize));
    }
    GXInvalidateVtxCache();
}
