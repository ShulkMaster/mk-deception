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
        descriptor->vatA &= ~0x1FFU;
        descriptor->vatA |= (componentCount | (componentType << 1) |
                             ((unsigned int)fraction << 4)) & 0x1FFU;
        break;
    case 10:
        descriptor->vatA &= ~0x1E00U;
        descriptor->vatA |= ((componentCount << 9) | (componentType << 10)) &
                            0x1E00U;
        descriptor->vatA &= 0x7FFFFFFFU;
        break;
    case 25:
        descriptor->vatA &= ~0x1E00U;
        descriptor->vatA |= ((componentCount << 9) | (componentType << 10)) &
                            0x1E00U;
        if (componentCount == 2) {
            descriptor->vatA |= 0x80000000U;
        } else {
            descriptor->vatA &= 0x7FFFFFFFU;
        }
        break;
    case 11:
        descriptor->vatA &= ~0x1E000U;
        descriptor->vatA |= ((componentCount << 13) | (componentType << 14)) &
                            0x1E000U;
        break;
    case 12:
        descriptor->vatA &= ~0x1E0000U;
        descriptor->vatA |= ((componentCount << 17) | (componentType << 18)) &
                            0x1E0000U;
        break;
    case 13:
        descriptor->vatA &= ~(0x1FFU << 21);
        descriptor->vatA |= ((componentCount << 21) | (componentType << 22) |
                             ((unsigned int)fraction << 25)) &
                            (0x1FFU << 21);
        break;
    case 14:
        descriptor->vatB &= ~0x1FFU;
        descriptor->vatB |= (componentCount | (componentType << 1) |
                             ((unsigned int)fraction << 4)) & 0x1FFU;
        break;
    case 15:
        descriptor->vatB &= ~(0x1FFU << 9);
        descriptor->vatB |= ((componentCount << 9) | (componentType << 10) |
                             ((unsigned int)fraction << 13)) &
                            (0x1FFU << 9);
        break;
    case 16:

        descriptor->vatB &= ~(0x3FFU << 18);
        descriptor->vatB |=
            ((componentCount << 18) | (componentType << 19) |
             ((unsigned char)fraction << 22)) &
            (0x3FFU << 18);
        break;
    case 17:
        descriptor->vatB &= ~0xF0000000U;
        descriptor->vatB |= ((componentCount << 27) | (componentType << 28)) &
                            0xF0000000U;
        descriptor->vatC &= ~0x1FU;
        descriptor->vatC |= (unsigned int)fraction & 0x1FU;
        break;
    case 18:
        descriptor->vatC &= ~(0x1FFU << 5);
        descriptor->vatC |= ((componentCount << 5) | (componentType << 6) |
                             ((unsigned int)fraction << 9)) &
                            (0x1FFU << 5);
        break;
    case 19:
        descriptor->vatC &= ~(0x1FFU << 14);
        descriptor->vatC |= ((componentCount << 14) | (componentType << 15) |
                             ((unsigned int)fraction << 18)) &
                            (0x1FFU << 14);
        break;
    case 20:
        descriptor->vatC &= ~(0x1FFU << 23);
        descriptor->vatC |= ((componentCount << 23) | (componentType << 24) |
                             ((unsigned int)fraction << 27)) &
                            (0x1FFU << 23);
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
        descriptor->vcdLo &= ~1U;
        descriptor->vcdLo |= (unsigned int)type & 1U;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        descriptor->vcdLo &= ~(2U << (attr - 1));
        descriptor->vcdLo |= ((unsigned int)type << attr) &
                             (2U << (attr - 1));
        break;
    case 9:
        descriptor->vcdLo &= ~0x600U;
        descriptor->vcdLo |= ((unsigned int)type << 9) & 0x600U;
        break;
    case 10: {
        normalCount = 0;
        descriptor->vcdLo &= ~0x1800U;
        descriptor->vcdLo |= ((unsigned int)type << 11) & 0x1800U;
        if (type != 0) {
            normalCount = 1;
        }
        descriptor->metadata &= ~0xCU;
        descriptor->metadata |= (normalCount << 2) & 0xCU;
        break;
    }
    case 25: {
        nbtCount = 0;
        descriptor->vcdLo &= ~0x1800U;
        descriptor->vcdLo |= ((unsigned int)type << 11) & 0x1800U;
        if (type != 0) {
            nbtCount = 2;
        }
        descriptor->metadata &= ~0xCU;
        descriptor->metadata |= (nbtCount << 2) & 0xCU;
        break;
    }
    case 11:
    case 12:
        colorCount = 0;
        descriptor->vcdLo &= ~(0x6000U << (attr - 11));
        descriptor->vcdLo |= ((unsigned int)type << (attr + 2)) &
                             (0x6000U << (attr - 11));
        if ((descriptor->vcdLo & 0x6000U) != 0) {
            colorCount++;
        }
        if ((descriptor->vcdLo & 0x18000U) != 0) {
            colorCount++;
        }
        descriptor->metadata &= ~3U;
        descriptor->metadata |= colorCount & 3U;
        break;
#define SET_TEXCOORD_DESC(shift)                                            \
    do {                                                                    \
        descriptor->vcdHi &= ~(3U << (shift));                              \
        descriptor->vcdHi |= ((unsigned int)type << (shift)) &              \
                             (3U << (shift));                               \
        texCoordIndex = 8;                                                   \
        texCoordCount = 0;                                                   \
        while (texCoordIndex-- != 0) {                                      \
            if ((descriptor->vcdHi & (3U << (texCoordIndex * 2))) != 0) {   \
                texCoordCount++;                                            \
            }                                                               \
        }                                                                   \
        descriptor->metadata &= ~0xF0U;                                     \
        descriptor->metadata |= (texCoordCount << 4) & 0xF0U;               \
    } while (0)
    case 13:
        SET_TEXCOORD_DESC(0);
        break;
    case 14:
        SET_TEXCOORD_DESC(2);
        break;
    case 15:
        SET_TEXCOORD_DESC(4);
        break;
    case 16:
        SET_TEXCOORD_DESC(6);
        break;
    case 17:
        SET_TEXCOORD_DESC(8);
        break;
    case 18:
        SET_TEXCOORD_DESC(10);
        break;
    case 19:
        SET_TEXCOORD_DESC(12);
        break;
    case 20:
        SET_TEXCOORD_DESC(14);
        break;
#undef SET_TEXCOORD_DESC
    }
}

void _rwGCNVertexDescSetNumIndexedAttr(
    RwGameCubeVertexDescriptor* descriptor, unsigned char count)
{
    descriptor->numIndexedAttrs = count;
}
