#include "rw/gamecube.h"

extern RwUInt32 rwGCNPosGetSize(const RwGameCubeVertexDescriptor *descriptor);
extern RwUInt32 rwGCNNrmGetSize(const RwGameCubeVertexDescriptor *descriptor);
extern RwUInt32 rwGCNClrGetSize(const RwGameCubeVertexDescriptor *descriptor,
                                RwUInt8 colorIndex);
extern RwUInt32 rwGCNTexGetSize(const RwGameCubeVertexDescriptor *descriptor,
                                RwUInt8 texCoordIndex);

void _rwGCNTriStripGetStats(RwUInt16 *indices, RwUInt32 numIndices,
                            RwUInt32 *numStrips, RwUInt32 *stripIndices,
                            RwBool optimize) {
  RwUInt32 stripLength = 0;
  RwUInt32 index;

  *numStrips = 0;
  *stripIndices = 0;
  for (index = 0; index < numIndices - 2; index++) {
    if (indices[index] != indices[index + 1] &&
        indices[index] != indices[index + 2] &&
        indices[index + 1] != indices[index + 2]) {
      if (stripLength == 0 && (index & 1) != 0 && optimize != 0)
        stripLength = 2;
      else
        stripLength++;
    } else if (stripLength != 0) {
      if (optimize != 0 && index < numIndices - 3) {
        if (indices[index + 1] != indices[index + 2] &&
            indices[index + 1] != indices[index + 3] &&
            indices[index + 2] != indices[index + 3]) {
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

void _rwGCNInstanceIndicesCopy(RwUInt16 *source, RwUInt32 count,
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



void _rwGCNInstanceIndices(RwUInt16 *primitiveIndices,
                           RwUInt16 *attributeIndices,
                           RwUInt32 numIndices, RwBool triangleStrip,
                           RwUInt32 stride, RwUInt32 descriptor,
                           RwBool optimize, RwUInt8 *destination) {
  RwUInt32 stripIndex = 0;
  RwUInt32 emitted = 0;
  RwUInt32 stripLength = 0;

  if (triangleStrip != 0) {
    RwUInt32 index;

    for (index = 0; index < numIndices - 2; index++) {
      if (primitiveIndices[index] != primitiveIndices[index + 1] &&
          primitiveIndices[index] != primitiveIndices[index + 2] &&
          primitiveIndices[index + 1] != primitiveIndices[index + 2]) {
        if (stripLength == 0 && (index & 1) != 0 && optimize != 0)
          stripLength = 2;
        else
          stripLength++;
      } else if (stripLength != 0) {
        if (optimize != 0 && index < numIndices - 3) {
          if (primitiveIndices[index + 1] != primitiveIndices[index + 2] &&
              primitiveIndices[index + 1] != primitiveIndices[index + 3] &&
              primitiveIndices[index + 2] != primitiveIndices[index + 3]) {
            if (((index + 1) & 1) == 0) {
              _rwGCNInstanceIndicesCopy(&attributeIndices[index - stripLength],
                                        stripLength + 2, stride, descriptor,
                                        destination + stripIndex * 3 +
                                            emitted * stride);
              emitted += stripLength;
              emitted += 2;
              stripIndex++;
              stripLength = 0;
            } else {
              stripLength++;
            }
          } else {
            _rwGCNInstanceIndicesCopy(
                &attributeIndices[index - stripLength], stripLength + 2, stride,
                descriptor, destination + stripIndex * 3 + emitted * stride);
            emitted += stripLength;
            emitted += 2;
            stripIndex++;
            stripLength = 0;
          }
        } else {
          _rwGCNInstanceIndicesCopy(
              &attributeIndices[index - stripLength], stripLength + 2, stride,
              descriptor, destination + stripIndex * 3 + emitted * stride);
          emitted += stripLength;
          emitted += 2;
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




static void WriteHeaders(RwUInt16 *indices, RwUInt32 numIndices,
                         RwBool triangleStrip, RwUInt32 stride,
                         RwUInt8 primitive, RwBool optimize,
                         RwUInt8 *destination) {
  RwUInt32 stripIndex = 0;
  RwUInt32 emitted = 0;
  RwUInt32 stripLength = 0;

  if (triangleStrip != 0) {
    RwUInt32 index;

    for (index = 0; index < numIndices - 2; index++) {
      if (indices[index] != indices[index + 1] &&
          indices[index] != indices[index + 2] &&
          indices[index + 1] != indices[index + 2]) {
        if (stripLength == 0 && (index & 1) != 0 && optimize != 0)
          stripLength = 2;
        else
          stripLength++;
      } else if (stripLength != 0) {
        if (optimize != 0 && index < numIndices - 3) {
          if (indices[index + 1] != indices[index + 2] &&
              indices[index + 1] != indices[index + 3] &&
              indices[index + 2] != indices[index + 3]) {
            if (((index + 1) & 1) == 0) {
              RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
              *header++ = primitive;
              *(RwUInt16 *)header = (RwUInt16)(stripLength + 2);
              emitted += stripLength;
              emitted += 2;
              stripIndex++;
              stripLength = 0;
            } else {
              stripLength++;
            }
          } else {
            RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
            *header++ = primitive;
            *(RwUInt16 *)header = (RwUInt16)(stripLength + 2);
            emitted += stripLength;
            emitted += 2;
            stripIndex++;
            stripLength = 0;
          }
        } else {
          RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
          *header++ = primitive;
          *(RwUInt16 *)header = (RwUInt16)(stripLength + 2);
          emitted += stripLength;
          emitted += 2;
          stripIndex++;
          stripLength = 0;
        }
      }
    }
    if (stripLength != 0) {
      RwUInt8 *header = destination + stripIndex * 3 + emitted * stride;
      *header++ = primitive;
      *(RwUInt16 *)header = (RwUInt16)(stripLength + 2);
    }
  } else {
    RwUInt8 *header = destination;
    *header++ = primitive;
    *(RwUInt16 *)header = (RwUInt16)numIndices;
  }
}






void _rwGCNDisplayListFill(const RwGameCubeVertexDescriptor *descriptor,
                           RwGameCubeDisplayList *displayList,
                           const RwGameCubeIndexData *indexData,
                           RwUInt32 numIndices, RwBool triangleStrip,
                           RwUInt32 stride, RwBool optimize, RwUInt8 primitive,
                           const RwV3d *remap) {
  RwUInt32 attribute;
  RwUInt32 offset = 0;

  WriteHeaders(indexData->attributes[9], numIndices, triangleStrip, stride,
               primitive, optimize, displayList->data);
  offset += 3;
  for (attribute = 0; attribute < 21; attribute++) {
    RwUInt8 *output = (RwUInt8 *)displayList->data + offset;
    RwUInt32 format;
    RwUInt32 textureType;
    RwReal textureScale;

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
            numIndices, triangleStrip, stride, 2, optimize, output);
        offset++;
      }
      break;
    case 9:
      format = (descriptor->vcdLo & (3U << 9)) >> 9;
      if (format == 1) {
        RwUInt32 components = descriptor->vatA & 1;
        if (components == 1) {
          RwUInt32 positionType = (descriptor->vatA >> 1) & 7;
          RwReal positionScale =
              (RwReal)(1 << ((descriptor->vatA >> 4) & 0x1F));
          _rwGCNVtxFmtInstPos3D(
              output, (const RwV3d *)indexData->attributes[attribute],
              positionType, numIndices, stride, remap, positionScale);
          offset += rwGCNPosGetSize(descriptor);
        }
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    case 10: {
      RwUInt32 vectors = (descriptor->vatA >> 9) & 1;
      format = (descriptor->vcdLo & (3U << 11)) >> 11;
      if (vectors == 1) {
        RwUInt32 vectorCount = descriptor->vatA >> 31;
        if (format == 1) {
          offset += rwGCNNrmGetSize(descriptor) * 3;
        } else if (format != 0) {
          if (vectorCount == 0) {
            vectorCount = 1;
            _rwGCNInstanceIndices(indexData->attributes[9],
                                  indexData->attributes[attribute], numIndices,
                                  triangleStrip, stride, format, optimize,
                                  output);
          } else {
            vectorCount = 3;
          }
          if (format == 2)
            offset += vectorCount;
          else if (format == 3)
            offset += vectorCount * 2;
        }
      } else if (format == 1) {
        RwUInt32 normalType = (descriptor->vatA >> 10) & 7;
        _rwGCNVtxFmtInstNrm(output,
                            (const RwV3d *)indexData->attributes[attribute],
                            normalType, numIndices, stride);
        offset += rwGCNNrmGetSize(descriptor);
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    }
    case 11:
    case 12:
      {
        RwUInt32 colorIndex = attribute - 11;
        RwUInt32 descriptorShift = 13 + colorIndex * 2;

        format = (descriptor->vcdLo & (3U << descriptorShift)) >>
                 descriptorShift;
        if (format == 1) {
          RwUInt32 colorType =
              (descriptor->vatA >> (14 + colorIndex * 4)) & 7;
          _rwGCNVtxFmtInstClr(output,
                              (const RwRGBA *)indexData->attributes[attribute],
                              colorType, numIndices, stride);
          offset += rwGCNClrGetSize(descriptor, (RwUInt8)colorIndex);
        } else if (format != 0) {
          _rwGCNInstanceIndices(indexData->attributes[9],
                                indexData->attributes[attribute], numIndices,
                                triangleStrip, stride, format, optimize, output);
          offset += format == 2 ? 1 : 2;
        }
      }
      break;
    case 13:
      format = descriptor->vcdHi & 3;
      if (format == 1) {
        textureType = (descriptor->vatA >> 22) & 7;
        textureScale =
            (RwReal)(1 << ((descriptor->vatA >> 25) & 0x1F));
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, 0);
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    case 14:
    case 15:
    case 16: {
      RwUInt32 textureIndex = attribute - 13;
      RwUInt32 shift = (textureIndex - 1) * 9;

      format = (descriptor->vcdHi >> (textureIndex * 2)) & 3;
      if (format == 1) {
        textureType = (descriptor->vatB >> (shift + 1)) & 7;
        textureScale =
            (RwReal)((1 << (descriptor->vatB >> (shift + 4))) & 0x1F);
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, (RwUInt8)textureIndex);
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    }
    case 17:
      format = (descriptor->vcdHi & (3U << 8)) >> 8;
      if (format == 1) {
        textureType = (descriptor->vatB >> 28) & 7;
        textureScale = (RwReal)((1 << descriptor->vatC) & 0x1F);
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, 4);
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    case 18:
    case 19:
    case 20: {
      RwUInt32 textureIndex = attribute - 13;
      RwUInt32 shift = (textureIndex - 5) * 9;

      format = (descriptor->vcdHi >> (textureIndex * 2)) & 3;
      if (format == 1) {
        textureType = (descriptor->vatC >> (shift + 6)) & 7;
        textureScale =
            (RwReal)((1 << (descriptor->vatC >> (shift + 9))) & 0x1F);
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, (RwUInt8)textureIndex);
      } else if (format != 0) {
        _rwGCNInstanceIndices(indexData->attributes[9],
                              indexData->attributes[attribute], numIndices,
                              triangleStrip, stride, format, optimize, output);
        offset += format == 2 ? 1 : 2;
      }
      break;
    }
    default:
      break;
    }
  }
}
