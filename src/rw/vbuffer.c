#include "rw/gamecube.h"

extern RwUInt32 rwGCNPosGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern RwUInt32 rwGCNNrmGetSize(const RwGameCubeVertexDescriptor* descriptor);
extern RwUInt32 rwGCNClrGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               RwUInt8 index);
extern RwUInt32 rwGCNTexGetSize(const RwGameCubeVertexDescriptor* descriptor,
                               RwUInt8 index);

#define ALIGN_32(value) (((value) + 31U) & ~31U)
#define IS_INDEXED(type) ((type) == 2 || (type) == 3)

#define ADD_VERTEX_ARRAY(strideValue, descriptorType)                          \
    do {                                                                       \
        vertexBuffer->arrays[numArrays].data = currentData;                   \
        vertexBuffer->arrays[numArrays].attribute = (RwUInt8)attribute;       \
        vertexBuffer->arrays[numArrays].stride = (RwUInt8)(strideValue);      \
        vertexBuffer->arrays[numArrays].descriptor =                          \
            (RwUInt8)(descriptorType);                                         \
        dataSize += ALIGN_32((strideValue) * vertexCounts[attribute]);         \
        numArrays++;                                                           \
    } while (0)

#define VERTEX_BUFFER_SIZE_TEX_CASE(attributeValue, textureIndex, shift)      \
    case (attributeValue):                                                     \
        descriptorType = descriptor->vcdHi & (3U << (shift));                \
        descriptorType >>= (shift);                                           \
        if (IS_INDEXED(descriptorType)) {                                      \
            size += ALIGN_32(vertexCounts[attribute] *                         \
                             rwGCNTexGetSize(descriptor, (textureIndex)));     \
        }                                                                      \
        break

#define VERTEX_BUFFER_INIT_TEX_CASE(attributeValue, textureIndex, shift)      \
    case (attributeValue):                                                     \
        descriptorType = descriptor->vcdHi & (3U << (shift));                \
        descriptorType >>= (shift);                                           \
        if (IS_INDEXED(descriptorType)) {                                      \
            stride = rwGCNTexGetSize(descriptor, (textureIndex));             \
            ADD_VERTEX_ARRAY(stride, descriptorType);                         \
        }                                                                      \
        break

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
            if (IS_INDEXED(descriptorType)) {
                size += ALIGN_32(vertexCounts[attribute] *
                                 rwGCNPosGetSize(descriptor));
            }
            break;
        case 10:
            descriptorType = descriptor->vcdLo & 0x1800U;
            descriptorType >>= 11;
            if (IS_INDEXED(descriptorType)) {
                hasNBT = (descriptor->vatA >> 9) & 1;
                if (hasNBT == 1) {
                    size += ALIGN_32(vertexCounts[attribute] *
                                     rwGCNNrmGetSize(descriptor) * 3);
                } else {
                    size += ALIGN_32(vertexCounts[attribute] *
                                     rwGCNNrmGetSize(descriptor));
                }
            }
            break;
        case 11:
        case 12:
            descriptorType =
                (descriptor->vcdLo >> (13 + (attribute - 11) * 2)) & 3;
            if (IS_INDEXED(descriptorType)) {
                size += ALIGN_32(
                    vertexCounts[attribute] *
                    rwGCNClrGetSize(descriptor, (RwUInt8)(attribute - 11)));
            }
            break;
        VERTEX_BUFFER_SIZE_TEX_CASE(13, 0, 0);
        VERTEX_BUFFER_SIZE_TEX_CASE(14, 1, 2);
        VERTEX_BUFFER_SIZE_TEX_CASE(15, 2, 4);
        VERTEX_BUFFER_SIZE_TEX_CASE(16, 3, 6);
        VERTEX_BUFFER_SIZE_TEX_CASE(17, 4, 8);
        VERTEX_BUFFER_SIZE_TEX_CASE(18, 5, 10);
        VERTEX_BUFFER_SIZE_TEX_CASE(19, 6, 12);
        VERTEX_BUFFER_SIZE_TEX_CASE(20, 7, 14);
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
            if (IS_INDEXED(descriptorType)) {
                stride = rwGCNPosGetSize(descriptor);
                ADD_VERTEX_ARRAY(stride, descriptorType);
            }
            break;
        case 10:
            descriptorType = descriptor->vcdLo & 0x1800U;
            descriptorType >>= 11;
            if (IS_INDEXED(descriptorType)) {
                hasNBT = (descriptor->vatA >> 9) & 1;
                if (hasNBT == 1) {
                    stride = rwGCNNrmGetSize(descriptor) * 3;
                    ADD_VERTEX_ARRAY(stride, descriptorType);
                } else {
                    stride = rwGCNNrmGetSize(descriptor);
                    ADD_VERTEX_ARRAY(stride, descriptorType);
                }
            }
            break;
        case 11:
        case 12:
            descriptorType =
                (descriptor->vcdLo >> (13 + (attribute - 11) * 2)) & 3;
            if (IS_INDEXED(descriptorType)) {
                stride = rwGCNClrGetSize(descriptor, (RwUInt8)(attribute - 11));
                ADD_VERTEX_ARRAY(stride, descriptorType);
            }
            break;
        VERTEX_BUFFER_INIT_TEX_CASE(13, 0, 0);
        VERTEX_BUFFER_INIT_TEX_CASE(14, 1, 2);
        VERTEX_BUFFER_INIT_TEX_CASE(15, 2, 4);
        VERTEX_BUFFER_INIT_TEX_CASE(16, 3, 6);
        VERTEX_BUFFER_INIT_TEX_CASE(17, 4, 8);
        VERTEX_BUFFER_INIT_TEX_CASE(18, 5, 10);
        VERTEX_BUFFER_INIT_TEX_CASE(19, 6, 12);
        VERTEX_BUFFER_INIT_TEX_CASE(20, 7, 14);
        }
        attribute++;
    }
    vertexBuffer->numArrays = numArrays;
}
