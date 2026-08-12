#include "rw/gamecube.h"
#include "rw/rxpipeline.h"

extern float fmodf(float value, float modulus);





unsigned int rwGCNPosGetSize(const RwGameCubeVertexDescriptor *descriptor) {
  unsigned int components = descriptor->vatA & 1;
  unsigned int type;
  unsigned int result;

  if (components == 1)
    components = 3;
  else if (components == 0)
    components = 2;
  type = (descriptor->vatA >> 1) & 7;

  switch (type) {
  case 0:
  case 1:
    result = components;
    break;
  case 2:
  case 3:
    result = components * 2;
    break;
  case 4:
    result = components * 4;
    break;
  default:
    result = 0;
    break;
  }
  return result;
}

unsigned int rwGCNNrmGetSize(const RwGameCubeVertexDescriptor *descriptor) {
  unsigned int type = (descriptor->vatA >> 10) & 7;
  unsigned int result;

  switch (type) {
  case 1:
    result = 3;
    break;
  case 3:
    result = 6;
    break;
  case 4:
    result = 12;
    break;
  default:
    result = 0;
    break;
  }
  return result;
}

unsigned int rwGCNClrGetSize(const RwGameCubeVertexDescriptor *descriptor,
                         unsigned char colorIndex) {
  unsigned int type = (descriptor->vatA >> (14 + colorIndex * 4)) & 7;
  unsigned int result;

  switch (type) {
  case 0:
  case 3:
    result = 2;
    break;
  case 1:
  case 4:
    result = 3;
    break;
  case 2:
  case 5:
    result = 4;
    break;
  default:
    result = 0;
    break;
  }
  return result;
}

unsigned int rwGCNTexGetSize(const RwGameCubeVertexDescriptor *descriptor,
                         unsigned char texCoordIndex) {
  unsigned int components;
  unsigned int type;
  unsigned int result = 0;

  switch (texCoordIndex) {
  case 0:
    components = (descriptor->vatA >> 21) & 1;
    type = (descriptor->vatA >> 22) & 7;
    break;
  case 1:
    components = descriptor->vatB & 1;
    type = (descriptor->vatB >> 1) & 7;
    break;
  case 2:
    components = (descriptor->vatB >> 9) & 1;
    type = (descriptor->vatB >> 10) & 7;
    break;
  case 3:
    components = (descriptor->vatB >> 18) & 1;
    type = (descriptor->vatB >> 19) & 7;
    break;
  case 4:
    components = (descriptor->vatB >> 27) & 1;
    type = (descriptor->vatB >> 28) & 7;
    break;
  case 5:
    components = (descriptor->vatC >> 5) & 1;
    type = (descriptor->vatC >> 6) & 7;
    break;
  case 6:
    components = (descriptor->vatC >> 14) & 1;
    type = (descriptor->vatC >> 15) & 7;
    break;
  case 7:
    components = (descriptor->vatC >> 23) & 1;
    type = (descriptor->vatC >> 24) & 7;
    break;
  default:
    return 0;
  }

  if (components == 1)
    components = 2;
  else if (components == 0)
    components = 1;
  switch (type) {
  case 0:
  case 1:
    result = components;
    break;
  case 2:
  case 3:
    result = components * 2;
    break;
  case 4:
    result = components * 4;
    break;
  default:
    result = 0;
    break;
  }
  return result;
}






unsigned int _rwGCNVtxFmtInstPos3D(void *destination, const RwV3d *source,
                               unsigned int type, unsigned int count, unsigned int stride,
                               const RwV3d *origin, float scale) {
  const RwV3d *current = source;
  const RwV3d *positionOrigin = origin;
  unsigned int outputStride = stride;
  unsigned int format = type;
  float offsetX;
  float offsetY;
  float offsetZ;
  unsigned int result = 0;

  if (positionOrigin != 0) {
    offsetX = positionOrigin->x - fmodf(positionOrigin->x, 1.0f / scale);
    offsetY = positionOrigin->y - fmodf(positionOrigin->y, 1.0f / scale);
    offsetZ = positionOrigin->z - fmodf(positionOrigin->z, 1.0f / scale);
  } else {
    offsetX = 0.0f;
    offsetY = 0.0f;
    offsetZ = 0.0f;
  }

  switch (format) {
  case 0: {
    unsigned char *output = destination;
    unsigned int index;
    if (outputStride == 0)
      outputStride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (unsigned char)(int)(scale * (current->x - offsetX));
      output[1] = (unsigned char)(int)(scale * (current->y - offsetY));
      output[2] = (unsigned char)(int)(scale * (current->z - offsetZ));
      current++;
      output += outputStride;
    }
    result = count * 3;
    break;
  }
  case 1: {
    char *output = destination;
    unsigned int index;
    if (outputStride == 0)
      outputStride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (char)(scale * (current->x - offsetX));
      output[1] = (char)(scale * (current->y - offsetY));
      output[2] = (char)(scale * (current->z - offsetZ));
      current++;
      output += outputStride;
    }
    result = count * 3;
    break;
  }
  case 2: {
    unsigned short *output = destination;
    unsigned int index;
    if (outputStride == 0)
      outputStride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (unsigned short)(int)(scale * (current->x - offsetX));
      output[1] = (unsigned short)(int)(scale * (current->y - offsetY));
      output[2] = (unsigned short)(int)(scale * (current->z - offsetZ));
      current++;
      output = (unsigned short *)((unsigned char *)output + outputStride);
    }
    result = count * 6;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (outputStride == 0)
      outputStride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (short)(scale * (current->x - offsetX));
      output[1] = (short)(scale * (current->y - offsetY));
      output[2] = (short)(scale * (current->z - offsetZ));
      current++;
      output = (short *)((unsigned char *)output + outputStride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    unsigned int index;
    if (outputStride == 0)
      outputStride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((unsigned char *)output + outputStride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstNrm(void *destination, const RwV3d *source,
                             unsigned int type, unsigned int count, unsigned int stride) {
  const RwV3d *current = source;
  float byteScale = 64.0f;
  float shortScale = 16384.0f;
  unsigned int result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (char)(current->x * byteScale);
      output[1] = (char)(current->y * byteScale);
      output[2] = (char)(current->z * byteScale);
      current++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (short)(current->x * shortScale);
      output[1] = (short)(current->y * shortScale);
      output[2] = (short)(current->z * shortScale);
      current++;
      output = (short *)((unsigned char *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((unsigned char *)output + stride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstNrmCmp(void *destination, const void *source,
                                unsigned int type, unsigned int count,
                                unsigned int stride) {
  const char *input = source;
  float byteScale = 64.0f;
  float shortScale = 16384.0f;
  unsigned int result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (char)(normal.x * byteScale);
      output[1] = (char)(normal.y * byteScale);
      output[2] = (char)(normal.z * byteScale);
      input += 4;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (short)(normal.x * shortScale);
      output[1] = (short)(normal.y * shortScale);
      output[2] = (short)(normal.z * shortScale);
      input += 4;
      output = (short *)((unsigned char *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      *output = normal;
      input += 4;
      output = (RwV3d *)((unsigned char *)output + stride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstNBT(void *destination, const RwV3d *source,
                             unsigned int type, unsigned int count, unsigned int stride) {
  const RwV3d *current = source;
  float byteScale = 64.0f;
  float shortScale = 16384.0f;
  unsigned int result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (char)(current->x * byteScale);
      output[1] = (char)(current->y * byteScale);
      output[2] = (char)(current->z * byteScale);
      current++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (short)(current->x * shortScale);
      output[1] = (short)(current->y * shortScale);
      output[2] = (short)(current->z * shortScale);
      current++;
      output = (short *)((unsigned char *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((unsigned char *)output + stride);
    }
    result = count * 36;
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstNBTCmp(void *destination, const void *source,
                                unsigned int type, unsigned int count,
                                unsigned int stride) {
  const char *input = source;
  float byteScale = 64.0f;
  float shortScale = 16384.0f;
  unsigned int result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (char)(normal.x * byteScale);
      output[1] = (char)(normal.y * byteScale);
      output[2] = (char)(normal.z * byteScale);
      input += 4;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (short)(normal.x * shortScale);
      output[1] = (short)(normal.y * shortScale);
      output[2] = (short)(normal.z * shortScale);
      input += 4;
      output = (short *)((unsigned char *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      *output = normal;
      input += 4;
      output = (RwV3d *)((unsigned char *)output + stride);
    }
    result = count * 36;
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstClr(void *destination, const RwRGBA *source,
                             unsigned int type, unsigned int count, unsigned int stride) {
  unsigned int result = 0;

  switch (type) {
  case 0: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      *(unsigned short *)output =
          (unsigned short)((((unsigned short)source->blue >> 3) & 0x1F) |
                     ((((unsigned short)source->red << 8) & 0xF800) |
                      (((unsigned short)source->green * 8) & 0x7E0)));
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 1: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] = source->red;
      output[1] = source->green;
      output[2] = source->blue;
      source++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      *(unsigned short *)output =
          (unsigned short)((((unsigned short)source->alpha >> 4) & 0xF) |
                     ((source->blue & 0xF0) |
                      ((((unsigned short)source->red << 8) & 0xF000) |
                       (((unsigned short)source->green * 0x10) & 0xF00))));
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 4: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] =
          (unsigned char)((source->red & 0xFC) | (((unsigned char)source->green >> 6) & 3));
      output[1] = (unsigned char)(((source->green * 4) & 0xF0) |
                            (((unsigned char)source->blue >> 4) & 0xF));
      output[2] = (unsigned char)(((source->blue * 0x10) & 0xC0) |
                            (((unsigned char)source->alpha >> 2) & 0x3F));
      source++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 2:
  case 5: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 4;
    for (index = 0; index < count; index++) {
      output[0] = source->red;
      output[1] = source->green;
      output[2] = source->blue;
      output[3] = source->alpha;
      source++;
      output += stride;
    }
    result = count * 4;
    break;
  }
  default:
    break;
  }
  return result;
}

unsigned int _rwGCNVtxFmtInstTex(void *destination, const RwTexCoords *source,
                             unsigned int type, unsigned int count, unsigned int stride,
                             float scale) {
  unsigned int result = 0;

  switch (type) {
  case 0: {
    unsigned char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      output[0] = (unsigned char)(int)(source->u * scale);
      output[1] = (unsigned char)(int)(source->v * scale);
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 1: {
    char *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      output[0] = (char)(source->u * scale);
      output[1] = (char)(source->v * scale);
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 2: {
    unsigned short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 4;
    for (index = 0; index < count; index++) {
      output[0] = (unsigned short)(int)(source->u * scale);
      output[1] = (unsigned short)(int)(source->v * scale);
      source++;
      output = (unsigned short *)((unsigned char *)output + stride);
    }
    result = count * 4;
    break;
  }
  case 3: {
    short *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = 4;
    for (index = 0; index < count; index++) {
      output[0] = (short)(source->u * scale);
      output[1] = (short)(source->v * scale);
      source++;
      output = (short *)((unsigned char *)output + stride);
    }
    result = count * 4;
    break;
  }
  case 4: {
    RwTexCoords *output = destination;
    unsigned int index;
    if (stride == 0)
      stride = sizeof(RwTexCoords);
    for (index = 0; index < count; index++) {
      *output = *source++;
      output = (RwTexCoords *)((unsigned char *)output + stride);
    }
    result = count * sizeof(RwTexCoords);
    break;
  }
  default:
    break;
  }
  return result;
}
