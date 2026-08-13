#include "rw/gamecube.h"

void _rwGCNTriStripGetStats(unsigned short *indices, unsigned int numIndices,
                            unsigned int *numStrips, unsigned int *stripIndices,
                            int optimize) {
  unsigned int stripLength = 0;
  unsigned int index;

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

void _rwGCNInstanceIndicesCopy(unsigned short *source, unsigned int count,
                               unsigned int stride, unsigned int descriptor,
                               unsigned char *destination) {
  if (descriptor == 2) {
    unsigned int index;

    for (index = 0; index < count; index++) {
      *destination = (unsigned char)*source;
      source++;
      destination += stride;
    }
  } else if (descriptor == 3) {
    unsigned int index;

    for (index = 0; index < count; index++) {
      *(unsigned short *)destination = *source;
      source++;
      destination += stride;
    }
  }
}



void _rwGCNInstanceIndices(unsigned short *primitiveIndices,
                           unsigned short *attributeIndices,
                           unsigned int numIndices, int triangleStrip,
                           unsigned int stride, unsigned int descriptor,
                           int optimize, unsigned char *destination) {
  unsigned int stripIndex = 0;
  unsigned int emitted = 0;
  unsigned int stripLength = 0;

  if (triangleStrip != 0) {
    unsigned int index;

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




static void WriteHeaders(unsigned short *indices, unsigned int numIndices,
                         int triangleStrip, unsigned int stride,
                         unsigned char primitive, int optimize,
                         unsigned char *destination) {
  unsigned int stripIndex = 0;
  unsigned int emitted = 0;
  unsigned int stripLength = 0;

  if (triangleStrip != 0) {
    unsigned int index;

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
              unsigned char *header = destination + stripIndex * 3 + emitted * stride;
              *header++ = primitive;
              *(unsigned short *)header = (unsigned short)(stripLength + 2);
              emitted += stripLength;
              emitted += 2;
              stripIndex++;
              stripLength = 0;
            } else {
              stripLength++;
            }
          } else {
            unsigned char *header = destination + stripIndex * 3 + emitted * stride;
            *header++ = primitive;
            *(unsigned short *)header = (unsigned short)(stripLength + 2);
            emitted += stripLength;
            emitted += 2;
            stripIndex++;
            stripLength = 0;
          }
        } else {
          unsigned char *header = destination + stripIndex * 3 + emitted * stride;
          *header++ = primitive;
          *(unsigned short *)header = (unsigned short)(stripLength + 2);
          emitted += stripLength;
          emitted += 2;
          stripIndex++;
          stripLength = 0;
        }
      }
    }
    if (stripLength != 0) {
      unsigned char *header = destination + stripIndex * 3 + emitted * stride;
      *header++ = primitive;
      *(unsigned short *)header = (unsigned short)(stripLength + 2);
    }
  } else {
    unsigned char *header = destination;
    *header++ = primitive;
    *(unsigned short *)header = (unsigned short)numIndices;
  }
}






void _rwGCNDisplayListFill(const RwGameCubeVertexDescriptor *descriptor,
                           RwGameCubeDisplayList *displayList,
                           const RwGameCubeIndexData *indexData,
                           unsigned int numIndices, int triangleStrip,
                           unsigned int stride, int optimize, unsigned char primitive,
                           const RwV3d *remap) {
  unsigned int attribute;
  unsigned int offset = 0;

  WriteHeaders(indexData->attributes[9], numIndices, triangleStrip, stride,
               primitive, optimize, displayList->data);
  offset += 3;
  for (attribute = 0; attribute < 21; attribute++) {
    unsigned char *output = (unsigned char *)displayList->data + offset;
    unsigned int format;
    unsigned int textureType;
    float textureScale;

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
        unsigned int components = descriptor->vatA & 1;
        if (components == 1) {
          unsigned int positionType = (descriptor->vatA >> 1) & 7;
          float positionScale =
              (float)(1 << ((descriptor->vatA >> 4) & 0x1F));
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
      unsigned int vectors = (descriptor->vatA >> 9) & 1;
      format = (descriptor->vcdLo & (3U << 11)) >> 11;
      if (vectors == 1) {
        unsigned int vectorCount = descriptor->vatA >> 31;
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
        unsigned int normalType = (descriptor->vatA >> 10) & 7;
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
        unsigned int colorIndex = attribute - 11;
        unsigned int descriptorShift = 13 + colorIndex * 2;

        format = (descriptor->vcdLo & (3U << descriptorShift)) >>
                 descriptorShift;
        if (format == 1) {
          unsigned int colorType =
              (descriptor->vatA >> (14 + colorIndex * 4)) & 7;
          _rwGCNVtxFmtInstClr(output,
                              (const RwRGBA *)indexData->attributes[attribute],
                              colorType, numIndices, stride);
          offset += rwGCNClrGetSize(descriptor, (unsigned char)colorIndex);
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
            (float)(1 << ((descriptor->vatA >> 25) & 0x1F));
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
      unsigned int textureIndex = attribute - 13;
      unsigned int shift = (textureIndex - 1) * 9;

      format = (descriptor->vcdHi >> (textureIndex * 2)) & 3;
      if (format == 1) {
        textureType = (descriptor->vatB >> (shift + 1)) & 7;
        textureScale =
            (float)((1 << (descriptor->vatB >> (shift + 4))) & 0x1F);
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, (unsigned char)textureIndex);
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
        textureScale = (float)((1 << descriptor->vatC) & 0x1F);
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
      unsigned int textureIndex = attribute - 13;
      unsigned int shift = (textureIndex - 5) * 9;

      format = (descriptor->vcdHi >> (textureIndex * 2)) & 3;
      if (format == 1) {
        textureType = (descriptor->vatC >> (shift + 6)) & 7;
        textureScale =
            (float)((1 << (descriptor->vatC >> (shift + 9))) & 0x1F);
        _rwGCNVtxFmtInstTex(
            output, (const RwTexCoords *)indexData->attributes[attribute],
            textureType, numIndices, stride, textureScale);
        offset += rwGCNTexGetSize(descriptor, (unsigned char)textureIndex);
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
