#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"

extern float powf(float, float);

typedef RwImage *(*RwImageReadCallBack)(const RwChar *);
typedef RwBool (*RwRGBASetFromPixelCallBack)(RwRGBA *, const RwUInt32 *,
                                             RwInt32);
typedef struct RwImageFormat {
  RwChar extension[20];
  RwChar alternateExtension[20];
  RwImageReadCallBack read;
  void *reservedCallback;
  struct RwImageFormat *next;
} RwImageFormat;
typedef struct RwImageGlobals {
  RwFreeList *imageFreeList;
  RwChar *imagePath;
  RwInt32 imagePathSize;
  RwUInt8 gammaTable[256];
  RwUInt8 inverseGammaTable[256];
  RwReal gamma;
  void *scratchMemory;
  RwInt32 scratchSize;
  RwFreeList *formatFreeList;
  RwImageFormat *formats;
} RwImageGlobals;
typedef struct ImageReadState {
  RwImageReadCallBack read;
  RwImage *image;
} ImageReadState;
typedef RwChar *(*ImagePathCallBack)(RwChar *, void *);

static RwPluginRegistry imageTKList = {
    sizeof(RwImage), sizeof(RwImage), 0, 0, NULL, NULL};
static RwFreeList _rwImageFreeList;
static RwFreeList _rwImageFormatFreeList;
static RwInt32 _rwImageFreeListBlockSize = 0x80;
static RwInt32 _rwImageFreeListPreallocBlocks = 1;
static RwInt32 _rwImageFormatFreeListPreallocBlocks = 1;
static RwModuleInfo imageModule;
#define IMAGEGLOBALS                                                           \
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))

RwBool RwImageSetGamma(RwReal);
RwImage *RwImageCopy(RwImage *, const RwImage *);
RwImage *RwImageFreePixels(RwImage *);
RwBool RwImageDestroy(RwImage *);
RwImage *RwImageAllocatePixels(RwImage *);
RwImage *RwImageCreate(RwInt32, RwInt32, RwInt32);

void *_rwImageOpen(void *instance, RwInt32 offset, RwInt32 size) {
  imageModule.globalsOffset = offset;
  IMAGEGLOBALS->imageFreeList = RwFreeListCreateAndPreallocateSpace(
      imageTKList.sizeOfStruct, _rwImageFreeListBlockSize, 4,
      _rwImageFreeListPreallocBlocks, &_rwImageFreeList, 0x40018);
  if (!IMAGEGLOBALS->imageFreeList)
    return NULL;
  IMAGEGLOBALS->formatFreeList = RwFreeListCreateAndPreallocateSpace(
      sizeof(RwImageFormat), _rwImageFreeListBlockSize, 4,
      _rwImageFormatFreeListPreallocBlocks, &_rwImageFormatFreeList, 0x40406);
  if (!IMAGEGLOBALS->formatFreeList) {
    RwFreeListDestroy(IMAGEGLOBALS->imageFreeList);
    IMAGEGLOBALS->imageFreeList = NULL;
    return NULL;
  }
  IMAGEGLOBALS->imagePathSize = 0x100;
  IMAGEGLOBALS->imagePath = RwEngineInstance->fpMalloc(
      IMAGEGLOBALS->imagePathSize, 0x01040406);
  if (!IMAGEGLOBALS->imagePath) {
    RwFreeListDestroy(IMAGEGLOBALS->formatFreeList);
    IMAGEGLOBALS->formatFreeList = NULL;
    RwFreeListDestroy(IMAGEGLOBALS->imageFreeList);
    IMAGEGLOBALS->imageFreeList = NULL;
    return NULL;
  }
  IMAGEGLOBALS->imagePath[0] = 0;
  imageModule.numInstances++;
  RwImageSetGamma(1.0f);
  IMAGEGLOBALS->formats = NULL;
  IMAGEGLOBALS->scratchSize = 0x100;
  IMAGEGLOBALS->scratchMemory = RwEngineInstance->fpMalloc(
      IMAGEGLOBALS->scratchSize, 0x01040018);
  if (!IMAGEGLOBALS->scratchMemory) {
    RwEngineInstance->fpFree(IMAGEGLOBALS->imagePath);
    IMAGEGLOBALS->imagePath = NULL;
    IMAGEGLOBALS->imagePathSize = 0;
    RwFreeListDestroy(IMAGEGLOBALS->formatFreeList);
    IMAGEGLOBALS->formatFreeList = NULL;
    RwFreeListDestroy(IMAGEGLOBALS->imageFreeList);
    IMAGEGLOBALS->imageFreeList = NULL;
    return NULL;
  }
  return instance;
}

void *_rwImageClose(void *instance, RwInt32 offset, RwInt32 size) {
  if (IMAGEGLOBALS->scratchMemory) {
    RwEngineInstance->fpFree(IMAGEGLOBALS->scratchMemory);
    IMAGEGLOBALS->scratchMemory = NULL;
    IMAGEGLOBALS->scratchSize = 0;
  }
  if (IMAGEGLOBALS->imagePath) {
    RwEngineInstance->fpFree(IMAGEGLOBALS->imagePath);
    IMAGEGLOBALS->imagePath = NULL;
    IMAGEGLOBALS->imagePathSize = 0;
  }
  while (IMAGEGLOBALS->formats) {
    RwImageFormat *format = IMAGEGLOBALS->formats;
    IMAGEGLOBALS->formats = format->next;
    RwEngineInstance->fpFreeListFree(IMAGEGLOBALS->formatFreeList, format);
  }
  if (IMAGEGLOBALS->formatFreeList) {
    RwFreeListDestroy(IMAGEGLOBALS->formatFreeList);
    IMAGEGLOBALS->formatFreeList = NULL;
  }
  if (IMAGEGLOBALS->imageFreeList) {
    RwFreeListDestroy(IMAGEGLOBALS->imageFreeList);
    IMAGEGLOBALS->imageFreeList = NULL;
  }
  imageModule.numInstances--;
  return instance;
}

static void *ImageGetScratchMem(RwInt32 size) {
  void *memory;

  if (size > IMAGEGLOBALS->scratchSize) {
    if (IMAGEGLOBALS->scratchMemory)
      memory = RwEngineInstance->fpRealloc(IMAGEGLOBALS->scratchMemory, size,
                                           0x01040018);
    else
      memory = RwEngineInstance->fpMalloc(size, 0x01040018);
    if (!memory) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x80000013, size);
      RwErrorSet(&error);
      return NULL;
    }
    IMAGEGLOBALS->scratchMemory = memory;
    IMAGEGLOBALS->scratchSize = size;
  }
  return IMAGEGLOBALS->scratchMemory;
}

void _rwImageGammaCorrectArrayOfRGBA(RwRGBA *out, const RwRGBA *in,
                                     RwInt32 count) {
  const RwUInt8 *table = IMAGEGLOBALS->gammaTable;
  while (count-- != 0) {
    out->red = table[in->red];
    out->green = table[in->green];
    out->blue = table[in->blue];
    out->alpha = in->alpha;
    out++;
    in++;
  }
}

RwImage *RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth) {
  RwImage *image =
      RwEngineInstance->fpFreeListAlloc(IMAGEGLOBALS->imageFreeList, 0x30018);
  if (!image)
    return NULL;
  image->width = width;
  image->height = height;
  image->depth = depth;
  image->pixels = NULL;
  image->palette = NULL;
  image->flags = 0;
  _rwPluginRegistryInitObject(&imageTKList, image);
  return image;
}

RwBool RwImageDestroy(RwImage *image) {
  if (image->flags & 1)
    RwImageFreePixels(image);
  _rwPluginRegistryDeInitObject(&imageTKList, image);
  RwEngineInstance->fpFreeListFree(IMAGEGLOBALS->imageFreeList, image);
  return TRUE;
}

RwImage *RwImageAllocatePixels(RwImage *image) {
  RwUInt32 depth = image->depth;
  RwBool paletted = TRUE;
  RwInt32 paletteSize;
  RwInt32 pixelSize, totalSize;

  if (depth != 4 && depth != 8)
    paletted = FALSE;
  if (paletted)
    paletteSize = (1 << depth) * 4;
  else
    paletteSize = 0;
  image->stride = (image->depth + 7) >> 3;
  image->stride *= image->width;
  image->stride = (image->stride + 3) & ~3;
  pixelSize = image->stride * image->height;
  totalSize = pixelSize + paletteSize;
  image->pixels = RwEngineInstance->fpMalloc(totalSize, 0x30018);
  if (!image->pixels) {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000013, totalSize);
    RwErrorSet(&error);
    return NULL;
  }
  image->palette = paletted ? image->pixels + pixelSize : NULL;
  image->flags |= 1;
  return image;
}

RwImage *RwImageFreePixels(RwImage *image) {
  RwEngineInstance->fpFree(image->pixels);
  image->pixels = NULL;
  image->palette = NULL;
  image->flags &= ~1;
  return image;
}

RwImage *RwImageMakeMask(RwImage *image) {
  RwInt32 x, y;
  if (image->depth == 4 || image->depth == 8) {
    for (x = 0; x < (1 << image->depth); x++) {
      RwRGBA *c = (RwRGBA *)image->palette + x;
      RwUInt8 a = c->red;
      if (c->green > a)
        a = c->green;
      if (c->blue > a)
        a = c->blue;
      c->alpha = a;
    }
  } else if (image->depth == 32) {
    RwUInt8 *row = image->pixels;
    for (y = 0; y < image->height; y++, row += image->stride) {
      RwRGBA *c = (RwRGBA *)row;
      for (x = 0; x < image->width; x++, c++) {
        RwUInt8 a = c->red;
        if (c->green > a)
          a = c->green;
        if (c->blue > a)
          a = c->blue;
        c->alpha = a;
      }
    }
  }
  return image;
}

RwImage *RwImageApplyMask(RwImage *image, const RwImage *mask) {
  RwInt32 x, y;
  RwUInt8 *maskRow, *imageRow;
  if (image->width != mask->width || image->height != mask->height) {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x8000000A);
    RwErrorSet(&e);
    return NULL;
  }
  if (image->depth == 4 || image->depth == 8) {
    RwImage *copy = RwImageCreate(image->width, image->height, image->depth);
    if (!copy)
      return NULL;
    if (!RwImageAllocatePixels(copy)) {
      RwImageDestroy(copy);
      return NULL;
    }
    RwImageCopy(copy, image);
    if (image->flags & 1)
      RwImageFreePixels(image);
    image->depth = 32;
    RwImageAllocatePixels(image);
    RwImageCopy(image, copy);
    RwImageFreePixels(copy);
    RwImageDestroy(copy);
  } else if (image->depth != 32) {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x80000009);
    RwErrorSet(&e);
    return NULL;
  }
  maskRow = mask->pixels;
  imageRow = image->pixels;
  for (y = 0; y < image->height; y++) {
    RwRGBA *dst = (RwRGBA *)imageRow;
    if (mask->depth == 4 || mask->depth == 8) {
      for (x = 0; x < image->width; x++)
        dst[x].alpha = ((RwRGBA *)mask->palette)[maskRow[x]].alpha;
    } else if (mask->depth == 32) {
      RwRGBA *src = (RwRGBA *)maskRow;
      for (x = 0; x < image->width; x++)
        dst[x].alpha = src[x].alpha;
    }
    imageRow += image->stride;
    maskRow += mask->stride;
  }
  return image;
}

static RwChar *ImagePathForAllFullNames(const RwChar *name, RwInt32 extra,
                                        ImagePathCallBack callback,
                                        void *data) {
  const RwChar *path = IMAGEGLOBALS->imagePath;
  RwInt32 nameLength = RwEngineInstance->stringFuncs.vecStrlen(name);
  if (_rwpathisabsolute(name) || !path || !*path) {
    RwChar *full = ImageGetScratchMem(extra + nameLength);
    if (!full)
      return NULL;
    RwEngineInstance->stringFuncs.vecStrcpy(full, name);
    callback(full, data);
    return (RwChar *)name;
  }
  while (path && *path) {
    const RwChar *end;
    RwInt32 pathLength;
    RwChar *full;
    end = RwEngineInstance->stringFuncs.vecStrchr(path, ';');
    if (end) {
      pathLength = end - path;
      end++;
    } else
      pathLength = RwEngineInstance->stringFuncs.vecStrlen(path);
    full = ImageGetScratchMem(pathLength + nameLength + extra);
    if (!full)
      return NULL;
    memcpy(full, path, pathLength);
    RwEngineInstance->stringFuncs.vecStrcpy(full + pathLength, name);
    if (!callback(full, data))
      return (RwChar *)name;
    path = end;
  }
  return (RwChar *)name;
}

static RwChar *ImageAttempRead(RwChar *name, void *data) {
  ImageReadState *state = data;
  if (RwEngineInstance->fileFuncs.rwfexist(name)) {
    state->image = state->read(name);
    if (state->image)
      return NULL;
  }
  return name;
}

RwImage *RwImageRead(const RwChar *name) {
  const RwChar *colonPath;
  const RwChar *slashPath;
  const RwChar *backslashPath;
  RwChar *separator;
  RwChar *extension;
  RwImageFormat *format;
  separator = RwEngineInstance->stringFuncs.vecStrrchr(name, ':');
  colonPath = separator ? separator : name;
  separator = RwEngineInstance->stringFuncs.vecStrrchr(colonPath, '/');
  slashPath = separator ? separator : colonPath;
  separator = RwEngineInstance->stringFuncs.vecStrrchr(slashPath, '\\');
  backslashPath = separator ? separator : slashPath;
  extension = RwEngineInstance->stringFuncs.vecStrrchr(backslashPath, '.');
  if (!extension)
    return NULL;
  for (format = IMAGEGLOBALS->formats; format; format = format->next) {
    if (!RwEngineInstance->stringFuncs.vecStrcmp(format->extension,
                                                 extension) ||
        !RwEngineInstance->stringFuncs.vecStrcmp(format->alternateExtension,
                                                 extension)) {
      if (format->read) {
        ImageReadState state;
        state.read = format->read;
        state.image = NULL;
        ImagePathForAllFullNames(name, 5, ImageAttempRead, &state);
        return state.image;
      }
      return NULL;
    }
  }
  return NULL;
}

static RwChar *ImageDetermineExtender(RwChar *name, void *data) {
  RwChar **result = data;
  RwImageFormat *format;
  RwChar *end = name + RwEngineInstance->stringFuncs.vecStrlen(name);
  for (format = IMAGEGLOBALS->formats; format; format = format->next) {
    RwEngineInstance->stringFuncs.vecStrcpy(end, format->extension);
    if (RwEngineInstance->fileFuncs.rwfexist(name)) {
      *result = format->extension;
      return NULL;
    }
    RwEngineInstance->stringFuncs.vecStrcpy(end, format->alternateExtension);
    if (RwEngineInstance->fileFuncs.rwfexist(name)) {
      *result = format->alternateExtension;
      return NULL;
    }
  }
  return name;
}

const RwChar *RwImageFindFileType(const RwChar *name) {
  RwChar *result = NULL;
  ImagePathForAllFullNames(name, 20, ImageDetermineExtender, &result);
  return result;
}

RwImage *RwImageReadMaskedImage(const RwChar *imageName,
                                const RwChar *maskName) {
  RwImage *image = RwImageRead(imageName);
  if (image && maskName && *maskName) {
    RwImage *mask = RwImageRead(maskName);
    if (!mask) {
      RwImageDestroy(image);
      return NULL;
    }
    if (!RwImageMakeMask(mask)) {
      RwImageDestroy(image);
      RwImageDestroy(mask);
      return NULL;
    }
    if (!RwImageApplyMask(image, mask)) {
      RwImageDestroy(image);
      RwImageDestroy(mask);
      return NULL;
    }
    RwImageDestroy(mask);
  }
  return image;
}

RwRGBA *RwRGBASetFromPixel(RwRGBA *color, RwUInt32 pixel, RwInt32 format) {
  RwUInt32 value = pixel;
  (*(RwRGBASetFromPixelCallBack *)((RwUInt8 *)RwEngineInstance + 0x54))(
      color, &value, format);
  return color;
}

static RwBool ImageStraightCopy(RwImage *destination, const RwImage *source) {
  RwInt32 rowSize;
  RwUInt8 *src;
  RwUInt8 *dst;
  RwInt32 y;
  if (destination->palette && source->palette && source->depth <= 8)
    memcpy(destination->palette, source->palette, (1 << source->depth) * 4);
  rowSize = ((destination->depth + 7) >> 3) * destination->width;
  src = source->pixels;
  dst = destination->pixels;
  for (y = 0; y < destination->height; y++) {
    memcpy(dst, src, rowSize);
    dst += destination->stride;
    src += source->stride;
  }
  return TRUE;
}

static RwBool ImageConvertDepth(RwImage *destination, const RwImage *source) {
  RwBool result = FALSE;
  RwInt32 width = destination->width;
  RwInt32 height = destination->height;
  RwInt32 x, y;
  RwUInt8 *src = source->pixels;
  RwUInt8 *dst = destination->pixels;
  RwInt32 conversion = (source->depth << 8) | destination->depth;
  switch (conversion) {
  case 0x404:
  case 0x808:
  case 0x2020:
    result = TRUE;
    break;
  case 0x408:
    for (y = 0; y < height; y++) {
      memcpy(dst, src, width);
      src += source->stride;
      dst += destination->stride;
    }
    result = TRUE;
    break;
  case 0x420:
  case 0x820:
    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++)
        ((RwRGBA *)dst)[x] = ((RwRGBA *)source->palette)[src[x]];
      src += source->stride;
      dst += destination->stride;
    }
    result = TRUE;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000009);
    RwErrorSet(&error);
    break;
  }
  }
  return result;
}

RwImage *RwImageCopy(RwImage *destination, const RwImage *source) {
  if (destination->depth == source->depth) {
    if (!ImageStraightCopy(destination, source))
      destination = NULL;
  } else if (!ImageConvertDepth(destination, source))
    destination = NULL;
  destination->flags &= ~2;
  destination->flags |= source->flags & 2;
  return destination;
}

RwImage *RwImageGammaCorrect(RwImage *image) {
  if (image->depth == 4 || image->depth == 8) {
    RwRGBA *palette = (RwRGBA *)image->palette;
    if (!palette) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return NULL;
    }
    _rwImageGammaCorrectArrayOfRGBA(palette, palette, 1 << image->depth);
  } else if (image->depth == 32) {
    RwUInt8 *row = image->pixels;
    RwInt32 width = image->width;
    RwInt32 height = image->height;
    RwInt32 y;
    if (!row) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return NULL;
    }
    for (y = 0; y < height; y++, row += image->stride)
      _rwImageGammaCorrectArrayOfRGBA((RwRGBA *)row, (RwRGBA *)row, width);
  } else {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x80000008);
    RwErrorSet(&e);
    return NULL;
  }
  image->flags |= 2;
  return image;
}

RwBool RwImageSetGamma(RwReal gammaValue) {
  RwInt32 i;
  IMAGEGLOBALS->gamma = gammaValue;
  IMAGEGLOBALS->gammaTable[0] = IMAGEGLOBALS->inverseGammaTable[0] = 0;
  for (i = 1; i < 256; i++) {
    RwReal value = (RwReal)i / 255.0f;
    IMAGEGLOBALS->gammaTable[i] =
        (RwUInt8)(RwInt32)(0.5f + 255.0f * powf(value, 1.0f / gammaValue));
    IMAGEGLOBALS->inverseGammaTable[i] =
        (RwUInt8)(RwInt32)(0.5f + 255.0f * powf(value, gammaValue));
  }
  return TRUE;
}
