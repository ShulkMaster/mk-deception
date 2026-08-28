#include "rw/gamecube.h"

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
            descriptorType = (descriptor->vcdLo & 0x600U) >> 9;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] * rwGCNPosGetSize(descriptor) +
                     31U) &
                    ~31U;
            }
            break;
        case 10:
            descriptorType = (descriptor->vcdLo & 0x1800U) >> 11;
            if (descriptorType == 2 || descriptorType == 3) {
                hasNBT = (descriptor->vatA >> 9) & 1;
                if (hasNBT == 1) {
                    size +=
                        (vertexCounts[10] * rwGCNNrmGetSize(descriptor) * 3 +
                         31U) &
                        ~31U;
                } else {
                    size +=
                        (vertexCounts[10] * rwGCNNrmGetSize(descriptor) +
                         31U) &
                        ~31U;
                }
            }
            break;
        case 11:
        case 12:
            descriptorType =
                (descriptor->vcdLo >> (13 + (attribute - 11) * 2)) & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNClrGetSize(
                             descriptor, (unsigned char)(attribute - 11)) +
                     31U) &
                    ~31U;
            }
            break;
        case 13:
            descriptorType = descriptor->vcdHi & 3;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 14:
            descriptorType = (descriptor->vcdHi & 0xCU) >> 2;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 15:
            descriptorType = (descriptor->vcdHi & 0x30U) >> 4;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 16:
            descriptorType = (descriptor->vcdHi & 0xC0U) >> 6;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 17:
            descriptorType = (descriptor->vcdHi & 0x300U) >> 8;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 18:
            descriptorType = (descriptor->vcdHi & 0xC00U) >> 10;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 19:
            descriptorType = (descriptor->vcdHi & 0x3000U) >> 12;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
            }
            break;
        case 20:
            descriptorType = (descriptor->vcdHi & 0xC000U) >> 14;
            if (descriptorType == 2 || descriptorType == 3) {
                size +=
                    (vertexCounts[attribute] *
                         rwGCNTexGetSize(
                             descriptor, (unsigned char)(attribute - 13)) +
                     31U) &
                    ~31U;
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
            descriptorType = (descriptor->vcdLo & 0x600U) >> 9;
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
            descriptorType = (descriptor->vcdLo & 0x1800U) >> 11;
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
                        (stride * vertexCounts[10] + 31U) & ~31U;
                } else {
                    stride = rwGCNNrmGetSize(descriptor);
                    vertexBuffer->arrays[numArrays].data = currentData;
                    vertexBuffer->arrays[numArrays].attribute =
                        (unsigned char)attribute;
                    vertexBuffer->arrays[numArrays].stride = (unsigned char)stride;
                    vertexBuffer->arrays[numArrays].descriptor =
                        (unsigned char)descriptorType;
                    dataSize +=
                        (stride * vertexCounts[10] + 31U) & ~31U;
                }
                numArrays++;
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
        case 13:
            descriptorType = descriptor->vcdHi & 3;
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
        case 14:
            descriptorType = (descriptor->vcdHi & 0xCU) >> 2;
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
        case 15:
            descriptorType = (descriptor->vcdHi & 0x30U) >> 4;
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
        case 16:
            descriptorType = (descriptor->vcdHi & 0xC0U) >> 6;
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
        case 17:
            descriptorType = (descriptor->vcdHi & 0x300U) >> 8;
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
        case 18:
            descriptorType = (descriptor->vcdHi & 0xC00U) >> 10;
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
        case 19:
            descriptorType = (descriptor->vcdHi & 0x3000U) >> 12;
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
        case 20:
            descriptorType = (descriptor->vcdHi & 0xC000U) >> 14;
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
