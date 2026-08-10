#include "rw/gamecube.h"

extern RwUInt32 rwGCNPosGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern RwUInt32 rwGCNNrmGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern RwUInt32 rwGCNClrGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               RwUInt8 index);
extern RwUInt32 rwGCNTexGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               RwUInt8 index);

RwUInt32 _rwGCNVertexBufferHeaderGetSize(
    const RwGameCubeVertexDescriptor* descriptor)
{
    RwUInt32 size = 0x14;

    size += (descriptor->numIndexedAttrs - 1) * 8;
    return size;
}

RwUInt32 _rwGCNVertexBufferGetSize(
    const RwGameCubeVertexDescriptor* descriptor,
    const RwUInt32* vertexCounts)
{
    RwUInt32 size = 0;
    RwUInt32 descriptorType;
    RwUInt32 attribute = 9;
    RwUInt32 hasNBT;

    while (attribute < 21) {
        switch (attribute) {
        case 9:
            descriptorType = descriptor->vcdLo & 0x600U;
            descriptorType >>= 9;
            if (descriptorType == 2 || descriptorType == 3) {
                RwUInt32 bytes =
                    vertexCounts[attribute] * rwGCNPosGetSize(descriptor);
                size += (bytes + 31U) & ~31U;
            }
            break;
        case 10:
            descriptorType = descriptor->vcdLo & 0x1800U;
            descriptorType >>= 11;
            if (descriptorType == 2 || descriptorType == 3) {
                hasNBT = (descriptor->vatA >> 9) & 1;
                if (hasNBT == 1) {
                    RwUInt32 bytes = vertexCounts[attribute] *
                                     rwGCNNrmGetSize(descriptor) * 3;
                    size += (bytes + 31U) & ~31U;
                } else {
                    RwUInt32 bytes = vertexCounts[attribute] *
                                     rwGCNNrmGetSize(descriptor);
                    size += (bytes + 31U) & ~31U;
                }
            }
            break;
        case 11:
        case 12:
            descriptorType =
                (descriptor->vcdLo >> (13 + (attribute - 11) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                RwUInt32 bytes = vertexCounts[attribute] *
                    rwGCNClrGetSize(descriptor, (RwUInt8)(attribute - 11));
                size += (bytes + 31U) & ~31U;
            }
            break;
        default:
            descriptorType =
                (descriptor->vcdHi >> ((attribute - 13) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                RwUInt32 bytes = vertexCounts[attribute] *
                    rwGCNTexGetSize(descriptor, (RwUInt8)(attribute - 13));
                size += (bytes + 31U) & ~31U;
            }
            break;
        }
        attribute++;
    }
    return size;
}

void _rwGCNVertexBufferInitialize(
    const RwGameCubeVertexDescriptor* descriptor,
    RwGameCubeVertexBuffer* vertexBuffer, const RwUInt32* vertexCounts,
    void* data)
{
    RwUInt32 numArrays = 0;
    RwUInt32 dataSize = 0;
    RwUInt32 attribute = 9;
    RwUInt8* currentData;

    while (attribute < 21) {
        RwUInt32 descriptorType;
        RwUInt32 stride;
        RwUInt32 hasNBT;

        currentData = (RwUInt8*)data + dataSize;

        switch (attribute) {
        case 9:
            descriptorType = descriptor->vcdLo & 0x600U;
            descriptorType >>= 9;
            if (descriptorType == 2 || descriptorType == 3) {
                stride = rwGCNPosGetSize(descriptor);
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (RwUInt8)attribute;
                vertexBuffer->arrays[numArrays].stride = (RwUInt8)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (RwUInt8)descriptorType;
                dataSize +=
                    (stride * vertexCounts[attribute] + 31U) & ~31U;
                numArrays++;
            }
            break;
        case 10:
            descriptorType = descriptor->vcdLo & 0x1800U;
            descriptorType >>= 11;
            if (descriptorType == 2 || descriptorType == 3) {
                hasNBT = (descriptor->vatA >> 9) & 1;
                if (hasNBT == 1) {
                    stride = rwGCNNrmGetSize(descriptor) * 3;
                    vertexBuffer->arrays[numArrays].data = currentData;
                    vertexBuffer->arrays[numArrays].attribute =
                        (RwUInt8)attribute;
                    vertexBuffer->arrays[numArrays].stride = (RwUInt8)stride;
                    vertexBuffer->arrays[numArrays].descriptor =
                        (RwUInt8)descriptorType;
                    dataSize +=
                        (stride * vertexCounts[attribute] + 31U) & ~31U;
                    numArrays++;
                } else {
                    stride = rwGCNNrmGetSize(descriptor);
                    vertexBuffer->arrays[numArrays].data = currentData;
                    vertexBuffer->arrays[numArrays].attribute =
                        (RwUInt8)attribute;
                    vertexBuffer->arrays[numArrays].stride = (RwUInt8)stride;
                    vertexBuffer->arrays[numArrays].descriptor =
                        (RwUInt8)descriptorType;
                    dataSize +=
                        (stride * vertexCounts[attribute] + 31U) & ~31U;
                    numArrays++;
                }
            }
            break;
        case 11:
        case 12:
            descriptorType =
                (descriptor->vcdLo >> (13 + (attribute - 11) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                stride = rwGCNClrGetSize(descriptor, (RwUInt8)(attribute - 11));
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (RwUInt8)attribute;
                vertexBuffer->arrays[numArrays].stride = (RwUInt8)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (RwUInt8)descriptorType;
                dataSize +=
                    (stride * vertexCounts[attribute] + 31U) & ~31U;
                numArrays++;
            }
            break;
        default:
            descriptorType =
                (descriptor->vcdHi >> ((attribute - 13) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                stride = rwGCNTexGetSize(
                    descriptor, (RwUInt8)(attribute - 13));
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (RwUInt8)attribute;
                vertexBuffer->arrays[numArrays].stride = (RwUInt8)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (RwUInt8)descriptorType;
                dataSize +=
                    (stride * vertexCounts[attribute] + 31U) & ~31U;
                numArrays++;
            }
            break;
        }
        attribute++;
    }
    vertexBuffer->numArrays = numArrays;
}
