#include "rw/gamecube.h"
#include "runtime/cstring.h"

extern RwUInt32 rwGCNPosGetSize(const RwGameCubeVtxFmt* format);
extern RwUInt32 rwGCNNrmGetSize(const RwGameCubeVtxFmt* format);
extern RwUInt32 rwGCNClrGetSize(const RwGameCubeVtxFmt* format,
                                RwUInt8 colorIndex);
extern RwUInt32 rwGCNTexGetSize(const RwGameCubeVtxFmt* format,
                                RwUInt32 texCoordIndex);

/* Retail's release assertion still evaluates its condition at -opt off. */
#define RWASSERT(condition) ((void)(condition))

/* Near miss: retail keeps each extracted two-bit VCD field in r30 while this
 * clean switch assigns the identical mask/shift result to r31. */
RwUInt32 _rwGCNDisplayListGetStride(const RwGameCubeVtxFmt* format)
{
    RwUInt32 stride = 0;
    RwInt32 attribute;

    for (attribute = 0; attribute < 21; attribute++) {
        RwUInt32 type;

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
            RwUInt32 vectors;

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
                    format, (RwUInt8)(signed char)(attribute - 11));
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 13:
            type = format->vcdHi & 3;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 0);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 14:
            type = format->vcdHi & (3U << 2);
            type >>= 2;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 1);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 15:
            type = format->vcdHi & (3U << 4);
            type >>= 4;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 2);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 16:
            type = format->vcdHi & (3U << 6);
            type >>= 6;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 3);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 17:
            type = format->vcdHi & (3U << 8);
            type >>= 8;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 4);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 18:
            type = format->vcdHi & (3U << 10);
            type >>= 10;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 5);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 19:
            type = format->vcdHi & (3U << 12);
            type >>= 12;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 6);
            } else if (type == 2) {
                stride += 1;
            } else if (type == 3) {
                stride += 2;
            }
            break;
        case 20:
            type = format->vcdHi & (3U << 14);
            type >>= 14;
            if (type == 1) {
                stride += rwGCNTexGetSize(format, 7);
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

RwUInt32 _rwGCNDisplayListGetSize(const RwGameCubeVtxFmt* format,
                                  RwUInt32 numIndices,
                                  RwUInt32 numVertices)
{
    RwUInt32 size;
    RwUInt32 stride;

    RWASSERT(numIndices == 1);
    stride = _rwGCNDisplayListGetStride(format);
    size = numIndices * 3 + numVertices * stride;
    size = (size + 31) & ~31U;
    return size;
}

void _rwGCNDisplayListInitialize(RwGameCubeDisplayList* displayList,
                                 RwUInt32 index, RwUInt32 size, void* data)
{
    displayList->data = data;
    displayList->size = size;
    memset(data, 0, size);
}
