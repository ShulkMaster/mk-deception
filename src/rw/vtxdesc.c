#include "rw/gamecube.h"

#define SET_FIELD(value, fieldValue, shift, mask)                              \
    do {                                                                       \
        (value) &= ~(mask);                                                    \
        (value) |= ((fieldValue) << (shift)) & (mask);                         \
    } while (0)

#define SET_VERTEX_ATTRIBUTE(value, count, type, fraction, shift)             \
    do {                                                                       \
        (value) &= ~(0x1FFU << (shift));                                       \
        (value) |= (((count) << (shift)) | ((type) << ((shift) + 1)) |        \
                    ((RwUInt8)(fraction) << ((shift) + 4))) &                  \
                   (0x1FFU << (shift));                                        \
    } while (0)

#define SET_COMPONENT_ATTRIBUTE(value, count, type, shift, mask)              \
    do {                                                                       \
        (value) &= ~(mask);                                                    \
        (value) |= (((count) << (shift)) | ((type) << ((shift) + 1))) &       \
                   (mask);                                                     \
    } while (0)

#define UPDATE_TEXCOORD_COUNT(descriptor)                                      \
    do {                                                                       \
        texCoordIndex = 8;                                                     \
        texCoordCount = 0;                                                     \
        while (texCoordIndex-- != 0) {                                         \
            if (((descriptor)->vcdHi & (3U << (texCoordIndex * 2))) != 0) {   \
                texCoordCount++;                                               \
            }                                                                  \
        }                                                                      \
        SET_FIELD((descriptor)->metadata, texCoordCount, 4, 0xF0U);           \
    } while (0)

void _rwVertexDescriptorInit(RwGameCubeVertexDescriptor* descriptor)
{
    descriptor->vat = 0;
    descriptor->vatA = 0x40000000;
    descriptor->vatB = 0x80000000;
    descriptor->vatC = 0;
    descriptor->vcdLo = 0;
    descriptor->vcdHi = 0;
    descriptor->metadata = 0;
}

void _rwGCNVertexDescSetVAT(RwGameCubeVertexDescriptor* descriptor,
                            RwUInt32 vat)
{
    descriptor->vat = vat;
}

void _rwGCNVertexDescSetElementAttr(RwGameCubeVertexDescriptor* descriptor,
                                    RwUInt32 attr, RwInt32 componentCount,
                                    RwUInt32 componentType, RwUInt8 fraction)
{
    /* Near match: retail differs only in four temporary-register choices. */
    switch (attr) {
    case 9:
        SET_VERTEX_ATTRIBUTE(descriptor->vatA, componentCount, componentType,
                             fraction, 0);
        break;
    case 10:
        SET_COMPONENT_ATTRIBUTE(descriptor->vatA, componentCount,
                                componentType, 9, 0x1E00U);
        descriptor->vatA &= 0x7FFFFFFFU;
        break;
    case 25:
        SET_COMPONENT_ATTRIBUTE(descriptor->vatA, componentCount,
                                componentType, 9, 0x1E00U);
        if (componentCount == 2) {
            descriptor->vatA |= 0x80000000U;
        } else {
            descriptor->vatA &= 0x7FFFFFFFU;
        }
        break;
    case 11:
        SET_COMPONENT_ATTRIBUTE(descriptor->vatA, componentCount,
                                componentType, 13, 0x1E000U);
        break;
    case 12:
        SET_COMPONENT_ATTRIBUTE(descriptor->vatA, componentCount,
                                componentType, 17, 0x1E0000U);
        break;
    case 13:
        SET_VERTEX_ATTRIBUTE(descriptor->vatA, componentCount, componentType,
                             fraction, 21);
        break;
    case 14:
        SET_VERTEX_ATTRIBUTE(descriptor->vatB, componentCount, componentType,
                             fraction, 0);
        break;
    case 15:
        SET_VERTEX_ATTRIBUTE(descriptor->vatB, componentCount, componentType,
                             fraction, 9);
        break;
    case 16:
        SET_VERTEX_ATTRIBUTE(descriptor->vatB, componentCount, componentType,
                             fraction, 18);
        break;
    case 17:
        SET_COMPONENT_ATTRIBUTE(descriptor->vatB, componentCount,
                                componentType, 27, 0xF8000000U);
        SET_FIELD(descriptor->vatC, (RwUInt8)fraction, 0, 0x1FU);
        break;
    case 18:
        SET_VERTEX_ATTRIBUTE(descriptor->vatC, componentCount, componentType,
                             fraction, 5);
        break;
    case 19:
        SET_VERTEX_ATTRIBUTE(descriptor->vatC, componentCount, componentType,
                             fraction, 14);
        break;
    case 20:
        SET_VERTEX_ATTRIBUTE(descriptor->vatC, componentCount, componentType,
                             fraction, 23);
        break;
    }
}

void _rwGCNVertexDescSetElementDesc(RwGameCubeVertexDescriptor* descriptor,
                                    RwUInt32 attr, RwInt32 type)
{
    RwUInt32 normalCount;
    RwUInt32 nbtCount;
    RwUInt32 colorCount;
    RwUInt32 texCoordCount;
    RwUInt32 texCoordIndex;

    /* Near match: the functional instruction stream is exact; MWCC assigns
     * the scoped metadata temporaries a wider nonvolatile register range. */
    switch (attr) {
    case 0:
        SET_FIELD(descriptor->vcdLo, type, 0, 1U);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        SET_FIELD(descriptor->vcdLo, type, attr, 2U << (attr - 1));
        break;
    case 9:
        SET_FIELD(descriptor->vcdLo, type, 9, 0x600U);
        break;
    case 10: {
        RwUInt32 metadataValue;
        RwUInt32 metadataBits;

        normalCount = 0;
        SET_FIELD(descriptor->vcdLo, type, 11, 0x1800U);
        if (type != 0) {
            normalCount = 1;
        }
        descriptor->metadata &= ~0xCU;
        metadataValue = descriptor->metadata;
        metadataBits = normalCount << 2;
        metadataBits &= 0xCU;
        descriptor->metadata = metadataValue | metadataBits;
        break;
    }
    case 25: {
        RwUInt32 metadataValue;
        RwUInt32 metadataBits;

        nbtCount = 0;
        SET_FIELD(descriptor->vcdLo, type, 11, 0x1800U);
        if (type != 0) {
            nbtCount = 2;
        }
        descriptor->metadata &= ~0xCU;
        metadataValue = descriptor->metadata;
        metadataBits = nbtCount << 2;
        metadataBits &= 0xCU;
        descriptor->metadata = metadataValue | metadataBits;
        break;
    }
    case 11:
    case 12:
        colorCount = 0;
        SET_FIELD(descriptor->vcdLo, type, attr + 2,
                  0x6000U << (attr - 11));
        if ((descriptor->vcdLo & 0x6000U) != 0) {
            colorCount++;
        }
        if ((descriptor->vcdLo & 0x18000U) != 0) {
            colorCount++;
        }
        SET_FIELD(descriptor->metadata, colorCount, 0, 3U);
        break;
    case 13:
        SET_FIELD(descriptor->vcdHi, type, 0, 3U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 14:
        SET_FIELD(descriptor->vcdHi, type, 2, 0xCU);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 15:
        SET_FIELD(descriptor->vcdHi, type, 4, 0x30U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 16:
        SET_FIELD(descriptor->vcdHi, type, 6, 0xC0U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 17:
        SET_FIELD(descriptor->vcdHi, type, 8, 0x300U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 18:
        SET_FIELD(descriptor->vcdHi, type, 10, 0xC00U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 19:
        SET_FIELD(descriptor->vcdHi, type, 12, 0x3000U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    case 20:
        SET_FIELD(descriptor->vcdHi, type, 14, 0xC000U);
        UPDATE_TEXCOORD_COUNT(descriptor);
        break;
    }
}

void _rwGCNVertexDescSetNumIndexedAttr(
    RwGameCubeVertexDescriptor* descriptor, RwUInt8 count)
{
    descriptor->numIndexedAttrs = count;
}
