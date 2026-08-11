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
        RwReal weight = weights[0] * (1.0f / 128.0f);

        RwV3dTransformPoint(&transformed, point, &matrices[indices[0]]);
        output->x = transformed.x * weight;
        output->y = transformed.y * weight;
        output->z = transformed.z * weight;

        weight = weights[1] * (1.0f / 128.0f);
        if (weight > 0.0f) {
            RwV3dTransformPoint(&transformed, point, &matrices[indices[1]]);
            output->x += transformed.x * weight;
            output->y += transformed.y * weight;
            output->z += transformed.z * weight;
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
        RwReal weight = weights[0] * (1.0f / 128.0f);

        RwV3dTransformPoint(&transformedPoint, point, &matrices[indices[0]]);
        RwV3dTransformVector(&transformedNormal, normal,
                             &matrices[indices[0]]);
        outputPoint->x = transformedPoint.x * weight;
        outputPoint->y = transformedPoint.y * weight;
        outputPoint->z = transformedPoint.z * weight;
        outputNormal->x = transformedNormal.x * weight;
        outputNormal->y = transformedNormal.y * weight;
        outputNormal->z = transformedNormal.z * weight;

        weight = weights[1] * (1.0f / 128.0f);
        if (weight > 0.0f) {
            RwV3dTransformPoint(&transformedPoint, point,
                                &matrices[indices[1]]);
            RwV3dTransformVector(&transformedNormal, normal,
                                 &matrices[indices[1]]);
            outputPoint->x += transformedPoint.x * weight;
            outputPoint->y += transformedPoint.y * weight;
            outputPoint->z += transformedPoint.z * weight;
            outputNormal->x += transformedNormal.x * weight;
            outputNormal->y += transformedNormal.y * weight;
            outputNormal->z += transformedNormal.z * weight;
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
            RwReal weight = weights[j] * (1.0f / 128.0f);
            if (j == 0 || weight > 0.0f) {
                RwV3dTransformPoint(&transformed, point,
                                    &matrices[indices[j]]);
                if (j == 0) {
                    output->x = transformed.x * weight;
                    output->y = transformed.y * weight;
                    output->z = transformed.z * weight;
                } else {
                    output->x += transformed.x * weight;
                    output->y += transformed.y * weight;
                    output->z += transformed.z * weight;
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
            RwReal weight = weights[j] * (1.0f / 128.0f);
            if (j == 0 || weight > 0.0f) {
                RwV3dTransformPoint(&transformedPoint, point,
                                    &matrices[indices[j]]);
                RwV3dTransformVector(&transformedNormal, normal,
                                     &matrices[indices[j]]);
                if (j == 0) {
                    outputPoint->x = transformedPoint.x * weight;
                    outputPoint->y = transformedPoint.y * weight;
                    outputPoint->z = transformedPoint.z * weight;
                    outputNormal->x = transformedNormal.x * weight;
                    outputNormal->y = transformedNormal.y * weight;
                    outputNormal->z = transformedNormal.z * weight;
                } else {
                    outputPoint->x += transformedPoint.x * weight;
                    outputPoint->y += transformedPoint.y * weight;
                    outputPoint->z += transformedPoint.z * weight;
                    outputNormal->x += transformedNormal.x * weight;
                    outputNormal->y += transformedNormal.y * weight;
                    outputNormal->z += transformedNormal.z * weight;
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
            RwReal weight = weights[j] * (1.0f / 128.0f);
            if (j == 0 || weight > 0.0f) {
                RwV3dTransformPoint(&transformed, point,
                                    &matrices[indices[j]]);
                if (j == 0) {
                    output->x = transformed.x * weight;
                    output->y = transformed.y * weight;
                    output->z = transformed.z * weight;
                } else {
                    output->x += transformed.x * weight;
                    output->y += transformed.y * weight;
                    output->z += transformed.z * weight;
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
            RwReal weight = weights[j] * (1.0f / 128.0f);
            if (j == 0 || weight > 0.0f) {
                RwV3dTransformPoint(&transformedPoint, point,
                                    &matrices[indices[j]]);
                RwV3dTransformVector(&transformedNormal, normal,
                                     &matrices[indices[j]]);
                if (j == 0) {
                    outputPoint->x = transformedPoint.x * weight;
                    outputPoint->y = transformedPoint.y * weight;
                    outputPoint->z = transformedPoint.z * weight;
                    outputNormal->x = transformedNormal.x * weight;
                    outputNormal->y = transformedNormal.y * weight;
                    outputNormal->z = transformedNormal.z * weight;
                } else {
                    outputPoint->x += transformedPoint.x * weight;
                    outputPoint->y += transformedPoint.y * weight;
                    outputPoint->z += transformedPoint.z * weight;
                    outputNormal->x += transformedNormal.x * weight;
                    outputNormal->y += transformedNormal.y * weight;
                    outputNormal->z += transformedNormal.z * weight;
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
