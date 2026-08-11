#include "rw/gamecube.h"
#include "rw/rxpipeline.h"

extern RwReal fmodf(RwReal value, RwReal modulus);





RwUInt32 rwGCNPosGetSize(const RwGameCubeVertexDescriptor *descriptor) {
  RwUInt32 components = descriptor->vatA & 1;
  RwUInt32 type;
  RwUInt32 result;

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

RwUInt32 rwGCNNrmGetSize(const RwGameCubeVertexDescriptor *descriptor) {
  RwUInt32 type = (descriptor->vatA >> 10) & 7;
  RwUInt32 result;

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

RwUInt32 rwGCNClrGetSize(const RwGameCubeVertexDescriptor *descriptor,
                         RwUInt8 colorIndex) {
  RwUInt32 type = (descriptor->vatA >> (14 + colorIndex * 4)) & 7;
  RwUInt32 result;

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

RwUInt32 rwGCNTexGetSize(const RwGameCubeVertexDescriptor *descriptor,
                         RwUInt8 texCoordIndex) {
  RwUInt32 components;
  RwUInt32 type;
  RwUInt32 result = 0;

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






RwUInt32 _rwGCNVtxFmtInstPos3D(void *destination, const RwV3d *source,
                               RwUInt32 type, RwUInt32 count, RwUInt32 stride,
                               const RwV3d *origin, RwReal scale) {
  const RwV3d *current = source;
  const RwV3d *positionOrigin = origin;
  RwUInt32 outputStride = stride;
  RwUInt32 format = type;
  RwReal offsetX;
  RwReal offsetY;
  RwReal offsetZ;
  RwUInt32 result = 0;

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
    RwUInt8 *output = destination;
    RwUInt32 index;
    if (outputStride == 0)
      outputStride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (RwUInt8)(RwInt32)(scale * (current->x - offsetX));
      output[1] = (RwUInt8)(RwInt32)(scale * (current->y - offsetY));
      output[2] = (RwUInt8)(RwInt32)(scale * (current->z - offsetZ));
      current++;
      output += outputStride;
    }
    result = count * 3;
    break;
  }
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (outputStride == 0)
      outputStride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (RwChar)(scale * (current->x - offsetX));
      output[1] = (RwChar)(scale * (current->y - offsetY));
      output[2] = (RwChar)(scale * (current->z - offsetZ));
      current++;
      output += outputStride;
    }
    result = count * 3;
    break;
  }
  case 2: {
    RwUInt16 *output = destination;
    RwUInt32 index;
    if (outputStride == 0)
      outputStride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (RwUInt16)(RwInt32)(scale * (current->x - offsetX));
      output[1] = (RwUInt16)(RwInt32)(scale * (current->y - offsetY));
      output[2] = (RwUInt16)(RwInt32)(scale * (current->z - offsetZ));
      current++;
      output = (RwUInt16 *)((RwUInt8 *)output + outputStride);
    }
    result = count * 6;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (outputStride == 0)
      outputStride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (RwInt16)(scale * (current->x - offsetX));
      output[1] = (RwInt16)(scale * (current->y - offsetY));
      output[2] = (RwInt16)(scale * (current->z - offsetZ));
      current++;
      output = (RwInt16 *)((RwUInt8 *)output + outputStride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    RwUInt32 index;
    if (outputStride == 0)
      outputStride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((RwUInt8 *)output + outputStride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

RwUInt32 _rwGCNVtxFmtInstNrm(void *destination, const RwV3d *source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride) {
  const RwV3d *current = source;
  RwReal byteScale = 64.0f;
  RwReal shortScale = 16384.0f;
  RwUInt32 result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (RwChar)(current->x * byteScale);
      output[1] = (RwChar)(current->y * byteScale);
      output[2] = (RwChar)(current->z * byteScale);
      current++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (RwInt16)(current->x * shortScale);
      output[1] = (RwInt16)(current->y * shortScale);
      output[2] = (RwInt16)(current->z * shortScale);
      current++;
      output = (RwInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((RwUInt8 *)output + stride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

RwUInt32 _rwGCNVtxFmtInstNrmCmp(void *destination, const void *source,
                                RwUInt32 type, RwUInt32 count,
                                RwUInt32 stride) {
  const RwChar *input = source;
  RwReal byteScale = 64.0f;
  RwReal shortScale = 16384.0f;
  RwUInt32 result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (RwChar)(normal.x * byteScale);
      output[1] = (RwChar)(normal.y * byteScale);
      output[2] = (RwChar)(normal.z * byteScale);
      input += 4;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (RwInt16)(normal.x * shortScale);
      output[1] = (RwInt16)(normal.y * shortScale);
      output[2] = (RwInt16)(normal.z * shortScale);
      input += 4;
      output = (RwInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      *output = normal;
      input += 4;
      output = (RwV3d *)((RwUInt8 *)output + stride);
    }
    result = count * sizeof(RwV3d);
    break;
  }
  default:
    break;
  }
  return result;
}

RwUInt32 _rwGCNVtxFmtInstNBT(void *destination, const RwV3d *source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride) {
  const RwV3d *current = source;
  RwReal byteScale = 64.0f;
  RwReal shortScale = 16384.0f;
  RwUInt32 result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] = (RwChar)(current->x * byteScale);
      output[1] = (RwChar)(current->y * byteScale);
      output[2] = (RwChar)(current->z * byteScale);
      current++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      output[0] = (RwInt16)(current->x * shortScale);
      output[1] = (RwInt16)(current->y * shortScale);
      output[2] = (RwInt16)(current->z * shortScale);
      current++;
      output = (RwInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      *output = *current++;
      output = (RwV3d *)((RwUInt8 *)output + stride);
    }
    result = count * 36;
    break;
  }
  default:
    break;
  }
  return result;
}

RwUInt32 _rwGCNVtxFmtInstNBTCmp(void *destination, const void *source,
                                RwUInt32 type, RwUInt32 count,
                                RwUInt32 stride) {
  const RwChar *input = source;
  RwReal byteScale = 64.0f;
  RwReal shortScale = 16384.0f;
  RwUInt32 result = 0;

  switch (type) {
  case 0:
  case 2:
    break;
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (RwChar)(normal.x * byteScale);
      output[1] = (RwChar)(normal.y * byteScale);
      output[2] = (RwChar)(normal.z * byteScale);
      input += 4;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 6;
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      output[0] = (RwInt16)(normal.x * shortScale);
      output[1] = (RwInt16)(normal.y * shortScale);
      output[2] = (RwInt16)(normal.z * shortScale);
      input += 4;
      output = (RwInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 6;
    break;
  }
  case 4: {
    RwV3d *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = sizeof(RwV3d);
    for (index = 0; index < count; index++) {
      RwV3d normal;
      normal.x = input[0] * 0.0078125f;
      normal.y = input[1] * 0.0078125f;
      normal.z = input[2] * 0.0078125f;
      *output = normal;
      input += 4;
      output = (RwV3d *)((RwUInt8 *)output + stride);
    }
    result = count * 36;
    break;
  }
  default:
    break;
  }
  return result;
}

RwUInt32 _rwGCNVtxFmtInstClr(void *destination, const RwRGBA *source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride) {
  RwUInt32 result = 0;

  switch (type) {
  case 0: {
    RwUInt8 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      *(RwUInt16 *)output =
          (RwUInt16)((((RwUInt16)source->blue >> 3) & 0x1F) |
                     ((((RwUInt16)source->red << 8) & 0xF800) |
                      (((RwUInt16)source->green * 8) & 0x7E0)));
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 1: {
    RwUInt8 *output = destination;
    RwUInt32 index;
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
    RwUInt8 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      *(RwUInt16 *)output =
          (RwUInt16)((((RwUInt16)source->alpha >> 4) & 0xF) |
                     ((source->blue & 0xF0) |
                      ((((RwUInt16)source->red << 8) & 0xF000) |
                       (((RwUInt16)source->green * 0x10) & 0xF00))));
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 4: {
    RwUInt8 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 3;
    for (index = 0; index < count; index++) {
      output[0] =
          (RwUInt8)((source->red & 0xFC) | (((RwUInt8)source->green >> 6) & 3));
      output[1] = (RwUInt8)(((source->green * 4) & 0xF0) |
                            (((RwUInt8)source->blue >> 4) & 0xF));
      output[2] = (RwUInt8)(((source->blue * 0x10) & 0xC0) |
                            (((RwUInt8)source->alpha >> 2) & 0x3F));
      source++;
      output += stride;
    }
    result = count * 3;
    break;
  }
  case 2:
  case 5: {
    RwUInt8 *output = destination;
    RwUInt32 index;
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

RwUInt32 _rwGCNVtxFmtInstTex(void *destination, const RwTexCoords *source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride,
                             RwReal scale) {
  RwUInt32 result = 0;

  switch (type) {
  case 0: {
    RwUInt8 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      output[0] = (RwUInt8)(RwInt32)(source->u * scale);
      output[1] = (RwUInt8)(RwInt32)(source->v * scale);
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 1: {
    RwChar *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 2;
    for (index = 0; index < count; index++) {
      output[0] = (RwChar)(source->u * scale);
      output[1] = (RwChar)(source->v * scale);
      source++;
      output += stride;
    }
    result = count * 2;
    break;
  }
  case 2: {
    RwUInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 4;
    for (index = 0; index < count; index++) {
      output[0] = (RwUInt16)(RwInt32)(source->u * scale);
      output[1] = (RwUInt16)(RwInt32)(source->v * scale);
      source++;
      output = (RwUInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 4;
    break;
  }
  case 3: {
    RwInt16 *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = 4;
    for (index = 0; index < count; index++) {
      output[0] = (RwInt16)(source->u * scale);
      output[1] = (RwInt16)(source->v * scale);
      source++;
      output = (RwInt16 *)((RwUInt8 *)output + stride);
    }
    result = count * 4;
    break;
  }
  case 4: {
    RwTexCoords *output = destination;
    RwUInt32 index;
    if (stride == 0)
      stride = sizeof(RwTexCoords);
    for (index = 0; index < count; index++) {
      *output = *source++;
      output = (RwTexCoords *)((RwUInt8 *)output + stride);
    }
    result = count * sizeof(RwTexCoords);
    break;
  }
  default:
    break;
  }
  return result;
}
