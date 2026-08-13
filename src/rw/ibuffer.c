#include "rw/gamecube.h"
#include "runtime/cstring.h"

unsigned int _rwGCNDisplayListGetStride(
    const RwGameCubeVertexDescriptor* format)
{
    unsigned int stride = 0;
    int attribute;

    for (attribute = 0; attribute < 21; attribute++) {
        unsigned int type;

        switch (attribute) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            if (((1U << attribute) & format->vcdLo) != 0) {
                stride += 1;
            }
            break;
        case 9:
            type = format->vcdLo & (3U << 9);
            type >>= 9;
            if (type == 1) {
                stride += rwGCNPosGetSize(format);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 10: {
            unsigned int vectors;

            vectors = (format->vatA >> 9) & 1;
            type = format->vcdLo & (3U << 11);
            type >>= 11;
            if (vectors == 1) {
                vectors = format->vatA >> 31;
                if (vectors == 0) {
                    vectors = 1;
                } else {
                    vectors = 3;
                }
                if (type == 1) {
                    stride += vectors * rwGCNNrmGetSize(format);
                } else if (type == 2) {
                    stride += vectors;
                } else if (type == 3) {
                    stride += vectors * 2;
                }
            } else if (type == 1) {
                stride += rwGCNNrmGetSize(format);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        }
        case 11:
        case 12:
            type = (format->vcdLo >> (13 + ((attribute - 11) * 2))) & 3;
            if (type == 1) {
                stride += rwGCNClrGetSize(
                    format, (unsigned char)(signed char)(attribute - 11));
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        default:
            type = (format->vcdHi >> ((attribute - 13) * 2)) & 3;
            if (type == 1) {
                stride += rwGCNTexGetSize(
                    format, (unsigned int)(attribute - 13));
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        }
    }
    return stride;
}

unsigned int _rwGCNDisplayListGetSize(const RwGameCubeVertexDescriptor* format,
                                  unsigned int numIndices,
                                  unsigned int numVertices)
{
    unsigned int size;
    unsigned int stride;

    stride = _rwGCNDisplayListGetStride(format);
    size = numIndices * 3 + numVertices * stride;
    size = (size + 31) & ~31U;
    return size;
}

void _rwGCNDisplayListInitialize(RwGameCubeDisplayList* displayList,
                                 unsigned int index, unsigned int size, void* data)
{
    displayList->data = data;
    displayList->size = size;
    memset(data, 0, size);
}
