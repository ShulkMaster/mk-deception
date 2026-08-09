#include "rw/gamecube.h"

typedef struct RwGameCubeIndexData {
  const RwUInt16 *attributes[21];
} RwGameCubeIndexData;

extern RwUInt32 rwGCNPosGetSize(const RwGameCubeVertexDescriptor *descriptor);
extern RwUInt32 rwGCNNrmGetSize(const RwGameCubeVertexDescriptor *descriptor);
extern RwUInt32 rwGCNClrGetSize(const RwGameCubeVertexDescriptor *descriptor,
                                RwUInt8 colorIndex);
extern RwUInt32 rwGCNTexGetSize(const RwGameCubeVertexDescriptor *descriptor,
                                RwUInt8 texCoordIndex);
extern void *_rwGCNVtxFmtInstPos3D(void *, const void *, RwUInt32, RwUInt32,
                                   RwUInt32, const RwV3d *, RwReal);
extern void *_rwGCNVtxFmtInstNrm(void *, const void *, RwUInt32, RwUInt32,
                                 RwUInt32);
extern void *_rwGCNVtxFmtInstClr(void *, const void *, RwUInt32, RwUInt32,
                                 RwUInt32);
extern void *_rwGCNVtxFmtInstTex(void *, const void *, RwUInt32, RwUInt32,
                                 RwUInt32, RwReal);

#define TRIANGLE_VALID(indices, index)                                         \
  ((indices)[index] != (indices)[(index) + 1] &&                               \
   (indices)[index] != (indices)[(index) + 2] &&                               \
   (indices)[(index) + 1] != (indices)[(index) + 2])

void _rwGCNTriStripGetStats(const RwUInt16 *indices, RwUInt32 numIndices,
                            RwUInt32 *numStrips, RwUInt32 *stripIndices,
                            RwBool optimize) {
  RwUInt32 stripLength = 0;
  RwUInt32 index;

  *numStrips = 0;
  *stripIndices = 0;
  for (index = 0; index < numIndices - 2; index++) {
    if (TRIANGLE_VALID(indices, index)) {
      if (stripLength == 0 && (index & 1) != 0 && optimize != FALSE)
        stripLength = 2;
      else
        stripLength++;
    } else if (stripLength != 0) {
      if (optimize != FALSE && index < numIndices - 3) {
        if (TRIANGLE_VALID(indices, index + 1)) {
          if (((index + 1) & 1) == 0) {
            *stripIndices += stripLength + 2;
            (*numStrips)++;
            stripLength = 0;
          } else {
            stripLength++;
          }
        } else {
          *stripIndices += stripLength + 2;
          (*numStrips)++;
          stripLength = 0;
        }
      } else {
        *stripIndices += stripLength + 2;
        (*numStrips)++;
        stripLength = 0;
      }
    }
  }
  if (stripLength != 0) {
    *stripIndices += stripLength + 2;
    (*numStrips)++;
  }
}

void _rwGCNInstanceIndicesCopy(const RwUInt16 *source, RwUInt32 count,
                               RwUInt32 stride, RwUInt32 descriptor,
                               RwUInt8 *destination) {
  if (descriptor == 2) {
    RwUInt32 index;

    for (index = 0; index < count; index++) {
      *destination = (RwUInt8)*source;
      source++;
      destination += stride;
    }
  } else if (descriptor == 3) {
    RwUInt32 index;

    for (index = 0; index < count; index++) {
      *(RwUInt16 *)destination = *source;
      source++;
      destination += stride;
    }
  }
}

void _rwGCNInstanceIndices(const RwUInt16 *primitiveIndices,
                           const RwUInt16 *attributeIndices,
                           RwUInt32 numIndices, RwBool triangleStrip,
                           RwUInt32 stride, RwUInt32 descriptor,
                           RwBool optimize, RwUInt8 *destination,
                           const void *remap) {
  RwUInt32 stripIndex = 0;
  RwUInt32 emitted = 0;
  RwUInt32 stripLength = 0;

  if (triangleStrip != FALSE) {
    RwUInt32 index;

    for (index = 0; index < numIndices - 2; index++) {
      if (TRIANGLE_VALID(primitiveIndices, index)) {
        if (stripLength == 0 && (index & 1) != 0 && optimize != FALSE)
          stripLength = 2;
        else
          stripLength++;
      } else if (stripLength != 0) {
        if (optimize != FALSE && index < numIndices - 3) {
          if (TRIANGLE_VALID(primitiveIndices, index + 1)) {
            if (((index + 1) & 1) == 0) {
              _rwGCNInstanceIndicesCopy(&attributeIndices[index - stripLength],
                                        stripLength + 2, stride, descriptor,
                                        destination + stripIndex * 3 +
                                            emitted * stride);
              emitted = stripLength + emitted + 2;
              stripIndex++;
              stripLength = 0;
            } else {
              stripLength++;
            }
          } else {
            _rwGCNInstanceIndicesCopy(
                &attributeIndices[index - stripLength], stripLength + 2, stride,
                descriptor, destination + stripIndex * 3 + emitted * stride);
            emitted = stripLength + emitted + 2;
            stripIndex++;
            stripLength = 0;
          }
        } else {
          _rwGCNInstanceIndicesCopy(
              &attributeIndices[index - stripLength], stripLength + 2, stride,
              descriptor, destination + stripIndex * 3 + emitted * stride);
          emitted = stripLength + emitted + 2;
          stripIndex++;
          stripLength = 0;
        }
      }
    }
    if (stripLength != 0) {
      _rwGCNInstanceIndicesCopy(
          &attributeIndices[index - stripLength], stripLength + 2, stride,
          descriptor, destination + stripIndex * 3 + emitted * stride);
    }
  } else {
    _rwGCNInstanceIndicesCopy(attributeIndices, numIndices, stride, descriptor,
                              destination);
  }
}

static void WriteHeaders(const RwUInt16 *indices, RwUInt16 numIndices,
                         RwBool triangleStrip, RwUInt32 stride,
                         RwUInt8 primitive, RwBool optimize,
                         RwUInt8 *destination, const void *remap) {
  RwUInt32 stripIndex = 0;
  RwUInt32 emitted = 0;
  RwUInt32 stripLength = 0;

  if (triangleStrip != FALSE) {
    RwUInt32 index;

    for (index = 0; index < (RwUInt32)(numIndices - 2); index++) {
      if (TRIANGLE_VALID(indices, index)) {
        if (stripLength == 0 && (index & 1) != 0 && optimize != FALSE)
          stripLength = 2;
        else
          stripLength++;
      } else if (stripLength != 0) {
        if (optimize != FALSE && index < (RwUInt32)(numIndices - 3)) {
          if (TRIANGLE_VALID(indices, index + 1)) {
            if (((index + 1) & 1) == 0) {
              RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
              header[0] = primitive;
              *(RwUInt16 *)&header[1] = (RwUInt16)(stripLength + 2);
              emitted = stripLength + emitted + 2;
              stripIndex++;
              stripLength = 0;
            } else {
              stripLength++;
            }
          } else {
            RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
            header[0] = primitive;
            *(RwUInt16 *)&header[1] = (RwUInt16)(stripLength + 2);
            emitted = stripLength + emitted + 2;
            stripIndex++;
            stripLength = 0;
          }
        } else {
          RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
          header[0] = primitive;
          *(RwUInt16 *)&header[1] = (RwUInt16)(stripLength + 2);
          emitted = stripLength + emitted + 2;
          stripIndex++;
          stripLength = 0;
        }
      }
    }
    if (stripLength != 0) {
      RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
      header[0] = primitive;
      *(RwUInt16 *)&header[1] = (RwUInt16)(stripLength + 2);
    }
  } else {
    destination[0] = primitive;
    *(RwUInt16 *)&destination[1] = numIndices;
  }
}

#define INSTANCE_INDEXED(descriptorValue)                                      \
  do {                                                                         \
    _rwGCNInstanceIndices(indexData->attributes[9],                            \
                          indexData->attributes[attribute], numIndices,        \
                          triangleStrip, stride, descriptorValue, optimize,    \
                          output, remap);                                      \
    offset += (descriptorValue) - 1;                                           \
  } while (0)

#define INSTANCE_COLOR(colorIndex, descriptorShift, typeShift)                 \
  do {                                                                         \
    format = (descriptor->vcdLo >> (descriptorShift)) & 3;                     \
    if (format == 1) {                                                         \
      RwUInt32 type = (descriptor->vatA >> (typeShift)) & 7;                   \
      _rwGCNVtxFmtInstClr(output, indexData->attributes[attribute], type,      \
                          numIndices, stride);                                 \
      offset += rwGCNClrGetSize(descriptor, (colorIndex));                     \
    } else if (format != 0) {                                                  \
      INSTANCE_INDEXED(format);                                                \
    }                                                                          \
  } while (0)

#define INSTANCE_TEX(textureIndex, vatWord, typeShift, fractionShift)          \
  do {                                                                         \
    format = (descriptor->vcdHi >> ((textureIndex) * 2)) & 3;                  \
    if (format == 1) {                                                         \
      RwUInt32 type = ((vatWord) >> (typeShift)) & 7;                          \
      RwUInt32 fraction = ((vatWord) >> (fractionShift)) & 0x1F;               \
      RwReal scale = (RwReal)(1U << fraction);                                 \
      _rwGCNVtxFmtInstTex(output, indexData->attributes[attribute], type,      \
                          numIndices, stride, scale);                          \
      offset += rwGCNTexGetSize(descriptor, (textureIndex));                   \
    } else if (format != 0) {                                                  \
      INSTANCE_INDEXED(format);                                                \
    }                                                                          \
  } while (0)

void _rwGCNDisplayListFill(const RwGameCubeVertexDescriptor *descriptor,
                           RwGameCubeDisplayList *displayList,
                           const RwGameCubeIndexData *indexData,
                           RwUInt32 numIndices, RwBool triangleStrip,
                           RwUInt32 stride, RwBool optimize, RwUInt8 primitive,
                           const RwV3d *remap) {
  RwUInt32 offset = 0;
  RwUInt32 attribute;

  WriteHeaders(indexData->attributes[9], (RwUInt16)numIndices, triangleStrip,
               stride, primitive, optimize, displayList->data, remap);
  offset += 3;
  for (attribute = 0; attribute < 21; attribute++) {
    RwUInt8 *output = (RwUInt8 *)displayList->data + offset;
    RwUInt32 format;

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
      if (((1U << attribute) & descriptor->vcdLo) != 0) {
        _rwGCNInstanceIndices(
            indexData->attributes[9], indexData->attributes[attribute],
            numIndices, triangleStrip, stride, 2, optimize, output, remap);
        offset++;
      }
      break;
    case 9:
      format = (descriptor->vcdLo >> 9) & 3;
      if (format == 1) {
        RwUInt32 components = descriptor->vatA & 1;
        if (components == 1) {
          RwUInt32 type = (descriptor->vatA >> 1) & 7;
          RwReal scale = (RwReal)(1U << ((descriptor->vatA >> 4) & 0x1F));
          _rwGCNVtxFmtInstPos3D(output, indexData->attributes[attribute], type,
                                numIndices, stride, remap, scale);
          offset += rwGCNPosGetSize(descriptor);
        }
      } else if (format != 0) {
        INSTANCE_INDEXED(format);
      }
      break;
    case 10: {
      RwUInt32 vectors = (descriptor->vatA >> 9) & 1;
      format = (descriptor->vcdLo >> 11) & 3;
      if (vectors == 1) {
        RwUInt32 vectorCount = descriptor->vatA >> 31;
        vectorCount = vectorCount == 0 ? 1 : 3;
        if (format == 1) {
          offset += rwGCNNrmGetSize(descriptor) * 3;
        } else if (format != 0) {
          _rwGCNInstanceIndices(indexData->attributes[9],
                                indexData->attributes[attribute], numIndices,
                                triangleStrip, stride, format, optimize, output,
                                remap);
          offset += vectorCount * (format - 1);
        }
      } else if (format == 1) {
        RwUInt32 type = (descriptor->vatA >> 10) & 7;
        _rwGCNVtxFmtInstNrm(output, indexData->attributes[attribute], type,
                            numIndices, stride);
        offset += rwGCNNrmGetSize(descriptor);
      } else if (format != 0) {
        INSTANCE_INDEXED(format);
      }
      break;
    }
    case 11:
      INSTANCE_COLOR(0, 13, 14);
      break;
    case 12:
      INSTANCE_COLOR(1, 15, 18);
      break;
    case 13:
      INSTANCE_TEX(0, descriptor->vatA, 22, 25);
      break;
    case 14:
      INSTANCE_TEX(1, descriptor->vatB, 1, 4);
      break;
    case 15:
      INSTANCE_TEX(2, descriptor->vatB, 10, 13);
      break;
    case 16:
      INSTANCE_TEX(3, descriptor->vatB, 19, 22);
      break;
    case 17:
      format = (descriptor->vcdHi >> 8) & 3;
      if (format == 1) {
        RwUInt32 type = (descriptor->vatB >> 28) & 7;
        RwUInt32 fraction = descriptor->vatC & 0x1F;
        RwReal scale = (RwReal)(1U << fraction);
        _rwGCNVtxFmtInstTex(output, indexData->attributes[attribute], type,
                            numIndices, stride, scale);
        offset += rwGCNTexGetSize(descriptor, 4);
      } else if (format != 0) {
        INSTANCE_INDEXED(format);
      }
      break;
    case 18:
      INSTANCE_TEX(5, descriptor->vatC, 6, 9);
      break;
    case 19:
      INSTANCE_TEX(6, descriptor->vatC, 15, 18);
      break;
    case 20:
      INSTANCE_TEX(7, descriptor->vatC, 24, 27);
      break;
    default:
      break;
    }
  }
}
