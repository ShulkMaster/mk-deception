#include "rw/rpskin.h"
#include "rw/rtquat.h"

void _rwDlSkinUpdate2WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data)
{
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* source = data->source;
    unsigned char* destination = data->destination;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        float weight = weights[0] * (1.0f / 128.0f);

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
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* sourcePositions = data->sourcePositions;
    unsigned char* sourceNormals = data->sourceNormals;
    unsigned char* destinationPositions = data->destinationPositions;
    unsigned char* destinationNormals = data->destinationNormals;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        float weight = weights[0] * (1.0f / 128.0f);

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
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* source = data->source;
    unsigned char* destination = data->destination;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        unsigned int j;

        for (j = 0; j < 3; j++) {
            float weight = weights[j] * (1.0f / 128.0f);
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
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* sourcePositions = data->sourcePositions;
    unsigned char* sourceNormals = data->sourceNormals;
    unsigned char* destinationPositions = data->destinationPositions;
    unsigned char* destinationNormals = data->destinationNormals;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        unsigned int j;

        for (j = 0; j < 3; j++) {
            float weight = weights[j] * (1.0f / 128.0f);
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
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* source = data->source;
    unsigned char* destination = data->destination;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)source;
        RwV3d* output = (RwV3d*)destination;
        RwV3d transformed;
        unsigned int j;

        for (j = 0; j < 4; j++) {
            float weight = weights[j] * (1.0f / 128.0f);
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
    const unsigned char* weights = (const unsigned char*)skin->platformWeights;
    const unsigned char* indices = (const unsigned char*)skin->platformIndices;
    unsigned char* sourcePositions = data->sourcePositions;
    unsigned char* sourceNormals = data->sourceNormals;
    unsigned char* destinationPositions = data->destinationPositions;
    unsigned char* destinationNormals = data->destinationNormals;
    unsigned int i;

    for (i = 0; i < data->numVertices; i++) {
        const RwV3d* point = (const RwV3d*)sourcePositions;
        const RwV3d* normal = (const RwV3d*)sourceNormals;
        RwV3d* outputPoint = (RwV3d*)destinationPositions;
        RwV3d* outputNormal = (RwV3d*)destinationNormals;
        RwV3d transformedPoint;
        RwV3d transformedNormal;
        unsigned int j;

        for (j = 0; j < 4; j++) {
            float weight = weights[j] * (1.0f / 128.0f);
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
