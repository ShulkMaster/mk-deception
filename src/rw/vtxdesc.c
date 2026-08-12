#include "rw/gamecube.h"

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
                            unsigned int vat)
{
    descriptor->vat = vat;
}

void _rwGCNVertexDescSetElementAttr(RwGameCubeVertexDescriptor* descriptor,
                                    unsigned int attr, int componentCount,
                                    unsigned int componentType, unsigned char fraction)
{
    switch (attr) {
    case 9:
        descriptor->vatA = (descriptor->vatA & ~0x1FFU) |
            ((componentCount | (componentType << 1) |
              ((unsigned int)fraction << 4)) & 0x1FFU);
        break;
    case 10:
        descriptor->vatA = (descriptor->vatA & ~0x1E00U) |
            (((componentCount << 9) | (componentType << 10)) & 0x1E00U);
        descriptor->vatA &= 0x7FFFFFFFU;
        break;
    case 25:
        descriptor->vatA = (descriptor->vatA & ~0x1E00U) |
            (((componentCount << 9) | (componentType << 10)) & 0x1E00U);
        if (componentCount == 2) {
            descriptor->vatA |= 0x80000000U;
        } else {
            descriptor->vatA &= 0x7FFFFFFFU;
        }
        break;
    case 11:
        descriptor->vatA = (descriptor->vatA & ~0x1E000U) |
            (((componentCount << 13) | (componentType << 14)) & 0x1E000U);
        break;
    case 12:
        descriptor->vatA = (descriptor->vatA & ~0x1E0000U) |
            (((componentCount << 17) | (componentType << 18)) & 0x1E0000U);
        break;
    case 13:
        descriptor->vatA = (descriptor->vatA & ~(0x1FFU << 21)) |
            (((componentCount << 21) | (componentType << 22) |
              ((unsigned int)fraction << 25)) & (0x1FFU << 21));
        break;
    case 14:
        descriptor->vatB = (descriptor->vatB & ~0x1FFU) |
            ((componentCount | (componentType << 1) |
              ((unsigned int)fraction << 4)) & 0x1FFU);
        break;
    case 15:
        descriptor->vatB = (descriptor->vatB & ~(0x1FFU << 9)) |
            (((componentCount << 9) | (componentType << 10) |
              ((unsigned int)fraction << 13)) & (0x1FFU << 9));
        break;
    case 16:

        descriptor->vatB &= ~(0x3FFU << 18);
        descriptor->vatB |=
            ((componentCount << 18) | (componentType << 19) |
             ((unsigned char)fraction << 22)) &
            (0x3FFU << 18);
        break;
    case 17:
        descriptor->vatB = (descriptor->vatB & ~0xF0000000U) |
            (((componentCount << 27) | (componentType << 28)) & 0xF0000000U);
        descriptor->vatC =
            (descriptor->vatC & ~0x1FU) | ((unsigned int)fraction & 0x1FU);
        break;
    case 18:
        descriptor->vatC = (descriptor->vatC & ~(0x1FFU << 5)) |
            (((componentCount << 5) | (componentType << 6) |
              ((unsigned int)fraction << 9)) & (0x1FFU << 5));
        break;
    case 19:
        descriptor->vatC = (descriptor->vatC & ~(0x1FFU << 14)) |
            (((componentCount << 14) | (componentType << 15) |
              ((unsigned int)fraction << 18)) & (0x1FFU << 14));
        break;
    case 20:
        descriptor->vatC = (descriptor->vatC & ~(0x1FFU << 23)) |
            (((componentCount << 23) | (componentType << 24) |
              ((unsigned int)fraction << 27)) & (0x1FFU << 23));
        break;
    }
}

void _rwGCNVertexDescSetElementDesc(RwGameCubeVertexDescriptor* descriptor,
                                    unsigned int attr, int type)
{
    unsigned int normalCount;
    unsigned int nbtCount;
    unsigned int colorCount;
    unsigned int texCoordCount;
    unsigned int texCoordIndex;


    switch (attr) {
    case 0:
        descriptor->vcdLo = (descriptor->vcdLo & ~1U) | ((unsigned int)type & 1U);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        descriptor->vcdLo = (descriptor->vcdLo & ~(1U << attr)) |
            (((unsigned int)type << attr) & (1U << attr));
        break;
    case 9:
        descriptor->vcdLo = (descriptor->vcdLo & ~0x600U) |
            (((unsigned int)type << 9) & 0x600U);
        break;
    case 10: {
        unsigned int metadataValue;
        unsigned int metadataBits;

        normalCount = 0;
        descriptor->vcdLo = (descriptor->vcdLo & ~0x1800U) |
            (((unsigned int)type << 11) & 0x1800U);
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
        unsigned int metadataValue;
        unsigned int metadataBits;

        nbtCount = 0;
        descriptor->vcdLo = (descriptor->vcdLo & ~0x1800U) |
            (((unsigned int)type << 11) & 0x1800U);
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
        descriptor->vcdLo =
            (descriptor->vcdLo & ~(0x6000U << (attr - 11))) |
            (((unsigned int)type << (attr + 2)) &
             (0x6000U << (attr - 11)));
        if ((descriptor->vcdLo & 0x6000U) != 0) {
            colorCount++;
        }
        if ((descriptor->vcdLo & 0x18000U) != 0) {
            colorCount++;
        }
        descriptor->metadata =
            (descriptor->metadata & ~3U) | (colorCount & 3U);
        break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20: {
        unsigned int shift = (attr - 13) * 2;
        unsigned int mask = 3U << shift;

        descriptor->vcdHi = (descriptor->vcdHi & ~mask) |
            (((unsigned int)type << shift) & mask);
        texCoordIndex = 8;
        texCoordCount = 0;
        while (texCoordIndex-- != 0) {
            if ((descriptor->vcdHi & (3U << (texCoordIndex * 2))) != 0)
                texCoordCount++;
        }
        descriptor->metadata = (descriptor->metadata & ~0xF0U) |
            ((texCoordCount << 4) & 0xF0U);
        break;
    }
    }
}

void _rwGCNVertexDescSetNumIndexedAttr(
    RwGameCubeVertexDescriptor* descriptor, unsigned char count)
{
    descriptor->numIndexedAttrs = count;
}
