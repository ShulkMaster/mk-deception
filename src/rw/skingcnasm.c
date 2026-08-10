#include "rw/rpskin.h"
#include "rw/rtquat.h"

typedef struct RpSkinBlendPositionData {
    RwUInt8* destination;
    RwUInt8* source;
    RwUInt32 stride;
    RwUInt32 numVertices;
} RpSkinBlendPositionData;

typedef struct RpSkinBlendPositionNormalData {
    RwUInt8* destinationPositions;
    RwUInt8* destinationNormals;
    RwUInt8* sourcePositions;
    RwUInt8* sourceNormals;
    RwUInt32 positionStride;
    RwUInt32 normalStride;
    RwUInt32 nbtStride;
    RwUInt32 numVertices;
} RpSkinBlendPositionNormalData;

/* Retail implements these loops entirely with paired-single loads and stores.
 * The surrounding GameCube skin node programs GQR6/GQR7 for the concrete
 * vertex-buffer format. This portable source expresses the unquantized RwV3d
 * path; packed-format conversion remains the platform instruction boundary. */
#define SKIN_WEIGHT_SCALE (1.0f / 128.0f)

#define TRANSFORM_POINT(_result, _point, _matrix)                            \
    do {                                                                     \
        (_result).x = (_point)->x * (_matrix)->right.x +                     \
                      (_point)->y * (_matrix)->up.x +                        \
                      (_point)->z * (_matrix)->at.x + (_matrix)->pos.x;      \
        (_result).y = (_point)->x * (_matrix)->right.y +                     \
                      (_point)->y * (_matrix)->up.y +                        \
                      (_point)->z * (_matrix)->at.y + (_matrix)->pos.y;      \
        (_result).z = (_point)->x * (_matrix)->right.z +                     \
                      (_point)->y * (_matrix)->up.z +                        \
                      (_point)->z * (_matrix)->at.z + (_matrix)->pos.z;      \
    } while (0)

#define TRANSFORM_VECTOR(_result, _vector, _matrix)                          \
    do {                                                                     \
        (_result).x = (_vector)->x * (_matrix)->right.x +                    \
                      (_vector)->y * (_matrix)->up.x +                       \
                      (_vector)->z * (_matrix)->at.x;                        \
        (_result).y = (_vector)->x * (_matrix)->right.y +                    \
                      (_vector)->y * (_matrix)->up.y +                       \
                      (_vector)->z * (_matrix)->at.y;                        \
        (_result).z = (_vector)->x * (_matrix)->right.z +                    \
                      (_vector)->y * (_matrix)->up.z +                       \
                      (_vector)->z * (_matrix)->at.z;                        \
    } while (0)

#define ACCUMULATE(_result, _value, _weight)                                 \
    do {                                                                     \
        (_result).x += (_value).x * (_weight);                               \
        (_result).y += (_value).y * (_weight);                               \
        (_result).z += (_value).z * (_weight);                               \
    } while (0)

#define INITIALIZE_BLEND(_result, _value, _weight)                           \
    do {                                                                     \
        (_result).x = (_value).x * (_weight);                                \
        (_result).y = (_value).y * (_weight);                                \
        (_result).z = (_value).z * (_weight);                                \
    } while (0)

void _rwDlSkinUpdate2WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* source = data->source;
    RwUInt8* destination = data->destination;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        RwReal weight = weights[0] * SKIN_WEIGHT_SCALE;

        TRANSFORM_POINT(transformed, point, &matrices[indices[0]]);
        INITIALIZE_BLEND(*output, transformed, weight);

        weight = weights[1] * SKIN_WEIGHT_SCALE;
        if (weight > 0.0f) {
            TRANSFORM_POINT(transformed, point, &matrices[indices[1]]);
            ACCUMULATE(*output, transformed, weight);
        }

        weights += 2;
        indices += 2;
        source += data->stride;
        destination += data->stride;
    }
}

void _rwDlSkinUpdate2WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* sourcePositions = data->sourcePositions;
    RwUInt8* sourceNormals = data->sourceNormals;
    RwUInt8* destinationPositions = data->destinationPositions;
    RwUInt8* destinationNormals = data->destinationNormals;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        RwReal weight = weights[0] * SKIN_WEIGHT_SCALE;

        TRANSFORM_POINT(transformedPoint, point, &matrices[indices[0]]);
        TRANSFORM_VECTOR(transformedNormal, normal, &matrices[indices[0]]);
        INITIALIZE_BLEND(*outputPoint, transformedPoint, weight);
        INITIALIZE_BLEND(*outputNormal, transformedNormal, weight);

        weight = weights[1] * SKIN_WEIGHT_SCALE;
        if (weight > 0.0f) {
            TRANSFORM_POINT(transformedPoint, point, &matrices[indices[1]]);
            TRANSFORM_VECTOR(transformedNormal, normal, &matrices[indices[1]]);
            ACCUMULATE(*outputPoint, transformedPoint, weight);
            ACCUMULATE(*outputNormal, transformedNormal, weight);
        }

        weights += 2;
        indices += 2;
        sourcePositions += data->positionStride;
        destinationPositions += data->positionStride;
        sourceNormals += data->normalStride + data->nbtStride;
        destinationNormals += data->normalStride + data->nbtStride;
    }
}

void _rwDlSkinUpdate3WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* source = data->source;
    RwUInt8* destination = data->destination;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        RwUInt32 j;

        for (j = 0; j < 3; j++) {
            RwReal weight = weights[j] * SKIN_WEIGHT_SCALE;
            if (j == 0 || weight > 0.0f) {
                TRANSFORM_POINT(transformed, point, &matrices[indices[j]]);
                if (j == 0) {
                    INITIALIZE_BLEND(*output, transformed, weight);
                } else {
                    ACCUMULATE(*output, transformed, weight);
                }
            }
        }

        weights += 3;
        indices += 3;
        source += data->stride;
        destination += data->stride;
    }
}

void _rwDlSkinUpdate3WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* sourcePositions = data->sourcePositions;
    RwUInt8* sourceNormals = data->sourceNormals;
    RwUInt8* destinationPositions = data->destinationPositions;
    RwUInt8* destinationNormals = data->destinationNormals;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        RwUInt32 j;

        for (j = 0; j < 3; j++) {
            RwReal weight = weights[j] * SKIN_WEIGHT_SCALE;
            if (j == 0 || weight > 0.0f) {
                TRANSFORM_POINT(transformedPoint, point,
                                &matrices[indices[j]]);
                TRANSFORM_VECTOR(transformedNormal, normal,
                                 &matrices[indices[j]]);
                if (j == 0) {
                    INITIALIZE_BLEND(*outputPoint, transformedPoint, weight);
                    INITIALIZE_BLEND(*outputNormal, transformedNormal, weight);
                } else {
                    ACCUMULATE(*outputPoint, transformedPoint, weight);
                    ACCUMULATE(*outputNormal, transformedNormal, weight);
                }
            }
        }

        weights += 3;
        indices += 3;
        sourcePositions += data->positionStride;
        destinationPositions += data->positionStride;
        sourceNormals += data->normalStride + data->nbtStride;
        destinationNormals += data->normalStride + data->nbtStride;
    }
}

void _rwDlSkinUpdate4WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* source = data->source;
    RwUInt8* destination = data->destination;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        RwUInt32 j;

        for (j = 0; j < 4; j++) {
            RwReal weight = weights[j] * SKIN_WEIGHT_SCALE;
            if (j == 0 || weight > 0.0f) {
                TRANSFORM_POINT(transformed, point, &matrices[indices[j]]);
                if (j == 0) {
                    INITIALIZE_BLEND(*output, transformed, weight);
                } else {
                    ACCUMULATE(*output, transformed, weight);
                }
            }
        }

        weights += 4;
        indices += 4;
        source += data->stride;
        destination += data->stride;
    }
}

void _rwDlSkinUpdate4WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    const RwUInt8* weights = (const RwUInt8*)skin->platformWeights;
    const RwUInt8* indices = (const RwUInt8*)skin->platformIndices;
    RwUInt8* sourcePositions = data->sourcePositions;
    RwUInt8* sourceNormals = data->sourceNormals;
    RwUInt8* destinationPositions = data->destinationPositions;
    RwUInt8* destinationNormals = data->destinationNormals;
    RwUInt32 i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        RwUInt32 j;

        for (j = 0; j < 4; j++) {
            RwReal weight = weights[j] * SKIN_WEIGHT_SCALE;
            if (j == 0 || weight > 0.0f) {
                TRANSFORM_POINT(transformedPoint, point,
                                &matrices[indices[j]]);
                TRANSFORM_VECTOR(transformedNormal, normal,
                                 &matrices[indices[j]]);
                if (j == 0) {
                    INITIALIZE_BLEND(*outputPoint, transformedPoint, weight);
                    INITIALIZE_BLEND(*outputNormal, transformedNormal, weight);
                } else {
                    ACCUMULATE(*outputPoint, transformedPoint, weight);
                    ACCUMULATE(*outputNormal, transformedNormal, weight);
                }
            }
        }

        weights += 4;
        indices += 4;
        sourcePositions += data->positionStride;
        destinationPositions += data->positionStride;
        sourceNormals += data->normalStride + data->nbtStride;
        destinationNormals += data->normalStride + data->nbtStride;
    }
}
