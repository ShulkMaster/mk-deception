#include "rw/gamecube.h"

extern unsigned int rwGCNPosGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern unsigned int rwGCNNrmGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern unsigned int rwGCNClrGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               unsigned char index);
extern unsigned int rwGCNTexGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               unsigned char index);

unsigned int _rwGCNVertexBufferHeaderGetSize(
    const RwGameCubeVertexDescriptor* descriptor)
{
    unsigned int size = 0x14;

    size += (descriptor->numIndexedAttrs - 1) * 8;
    return size;
}

unsigned int _rwGCNVertexBufferGetSize(
    const RwGameCubeVertexDescriptor* descriptor,
    const unsigned int* vertexCounts)
{
    unsigned int size = 0;
    unsigned int descriptorType;
    unsigned int attribute = 9;
    unsigned int hasNBT;

    while (attribute < 21) {
        switch (attribute) {
        case 9:
            descriptorType = descriptor->vcdLo & 0x600U;
            descriptorType >>= 9;
            if (descriptorType == 2 || descriptorType == 3) {
                unsigned int bytes =
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
                    unsigned int bytes = vertexCounts[attribute] *
                                     rwGCNNrmGetSize(descriptor) * 3;
                    size += (bytes + 31U) & ~31U;
                } else {
                    unsigned int bytes = vertexCounts[attribute] *
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
                unsigned int bytes = vertexCounts[attribute] *
                    rwGCNClrGetSize(descriptor, (unsigned char)(attribute - 11));
                size += (bytes + 31U) & ~31U;
            }
            break;
        default:
            descriptorType =
                (descriptor->vcdHi >> ((attribute - 13) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                unsigned int bytes = vertexCounts[attribute] *
                    rwGCNTexGetSize(descriptor, (unsigned char)(attribute - 13));
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
    RwGameCubeVertexBuffer* vertexBuffer, const unsigned int* vertexCounts,
    void* data)
{
    unsigned int numArrays = 0;
    unsigned int dataSize = 0;
    unsigned int attribute = 9;
    unsigned char* currentData;

    while (attribute < 21) {
        unsigned int descriptorType;
        unsigned int stride;
        unsigned int hasNBT;

        currentData = (unsigned char*)data + dataSize;

        switch (attribute) {
        case 9:
            descriptorType = descriptor->vcdLo & 0x600U;
            descriptorType >>= 9;
            if (descriptorType == 2 || descriptorType == 3) {
                stride = rwGCNPosGetSize(descriptor);
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (unsigned char)attribute;
                vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (unsigned char)descriptorType;
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
                        (unsigned char)attribute;
                    vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                    vertexBuffer->arrays[numArrays].descriptor =
                        (unsigned char)descriptorType;
                    dataSize +=
                        (stride * vertexCounts[attribute] + 31U) & ~31U;
                    numArrays++;
                } else {
                    stride = rwGCNNrmGetSize(descriptor);
                    vertexBuffer->arrays[numArrays].data = currentData;
                    vertexBuffer->arrays[numArrays].attribute =
                        (unsigned char)attribute;
                    vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                    vertexBuffer->arrays[numArrays].descriptor =
                        (unsigned char)descriptorType;
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
                stride = rwGCNClrGetSize(descriptor, (unsigned char)(attribute - 11));
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (unsigned char)attribute;
                vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (unsigned char)descriptorType;
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
                    descriptor, (unsigned char)(attribute - 13));
                vertexBuffer->arrays[numArrays].data = currentData;
                vertexBuffer->arrays[numArrays].attribute = (unsigned char)attribute;
                vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                vertexBuffer->arrays[numArrays].descriptor =
                    (unsigned char)descriptorType;
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
