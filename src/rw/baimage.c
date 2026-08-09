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

/*
 * The 30-instruction retail/current bodies are operationally identical. MWCC
 * inserts byte-normalizing clrlwi instructions after the clean typed loads;
 * no narrower honest source type removes them.
 */
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

/*
 * Allocation, ownership, sizes, hints, and failure CFG match retail. The
 * remaining diff is nonvolatile coloring and its frame/save-range selection.
 */
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
  RwInt32 i;

  switch (image->depth) {
  case 4:
  case 8: {
    RwInt32 paletteSize = 1 << image->depth;
    RwRGBA *palette = (RwRGBA *)image->palette;

    for (i = 0; i < paletteSize; i++) {
      RwInt32 opacity = palette[i].red;

      if (palette[i].green > opacity)
        opacity = palette[i].green;
      if (palette[i].blue > opacity)
        opacity = palette[i].blue;
      palette[i].alpha = (RwUInt8)opacity;
    }
    break;
  }
  case 32: {
    RwUInt8 *row = image->pixels;
    RwInt32 j;

    for (i = 0; i < image->height; i++) {
      RwRGBA *pixel = (RwRGBA *)row;

      for (j = 0; j < image->width; j++) {
        RwInt32 opacity = pixel[j].red;

        if (pixel[j].green > opacity)
          opacity = pixel[j].green;
        if (pixel[j].blue > opacity)
          opacity = pixel[j].blue;
        pixel[j].alpha = (RwUInt8)opacity;
      }
      row += image->stride;
    }
    break;
  }
  }

  return image;
}

RwImage *RwImageApplyMask(RwImage *image, const RwImage *mask) {
  if (image->width != mask->width || image->height != mask->height) {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x8000000A);
    RwErrorSet(&e);
    return NULL;
  }

  switch (image->depth) {
  case 4:
  case 8: {
    RwImage *tempImage =
        RwImageCreate(image->width, image->height, image->depth);

    if (tempImage == NULL)
      return NULL;
    if (RwImageAllocatePixels(tempImage) == NULL) {
      RwImageDestroy(tempImage);
      return NULL;
    }

    RwImageCopy(tempImage, image);
    if (image->flags & 1)
      RwImageFreePixels(image);

    image->depth = 32;
    RwImageAllocatePixels(image);
    RwImageCopy(image, tempImage);
    RwImageFreePixels(tempImage);
    RwImageDestroy(tempImage);
  }
  case 32: {
    RwInt32 i, j;
    const RwUInt8 *src = mask->pixels;
    const RwRGBA *palette = (const RwRGBA *)mask->palette;
    RwUInt8 *dst = image->pixels;

    for (i = 0; i < image->height; i++) {
      RwRGBA *dstPixel = (RwRGBA *)dst;

      switch (mask->depth) {
      case 4:
      case 8: {
        const RwUInt8 *srcIndex = src;

        for (j = 0; j < image->width; j++) {
          dstPixel->alpha = palette[*srcIndex].alpha;
          srcIndex++;
          dstPixel++;
        }
        break;
      }
      case 32: {
        const RwRGBA *srcPixel = (const RwRGBA *)src;

        for (j = 0; j < image->width; j++) {
          dstPixel->alpha = srcPixel->alpha;
          srcPixel++;
          dstPixel++;
        }
        break;
      }
      }

      dst += image->stride;
      src += mask->stride;
    }
    break;
  }
  default: {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x80000009);
    RwErrorSet(&e);
    return NULL;
  }
  }

  return image;
}

/*
 * Retail calls vecStrchr(pathElement, ';') twice and discards the first result.
 * The clean source keeps the single meaningful call; the remaining diff is
 * that dead seven-instruction call sequence and its register-coloring cascade.
 */
static RwChar *ImagePathForAllFullNames(const RwChar *name, RwInt32 extra,
                                        ImagePathCallBack callback,
                                        void *data) {
  RwInt32 pathSize;
  RwChar *fullName;
  const RwChar *pathElement;

  pathElement = IMAGEGLOBALS->imagePath;
  if (_rwpathisabsolute(name) || pathElement == NULL || pathElement[0] == '\0') {
    pathSize = RwEngineInstance->stringFuncs.vecStrlen(name) + extra;
    fullName = ImageGetScratchMem(pathSize);
    if (fullName == NULL)
      return NULL;
    RwEngineInstance->stringFuncs.vecStrcpy(fullName, name);
    callback(fullName, data);
  } else {
    while (pathElement != NULL && pathElement[0] != '\0') {
      const RwChar *nextPathElement =
          RwEngineInstance->stringFuncs.vecStrchr(pathElement, ';');
      RwInt32 pathElementLength;

      if (nextPathElement != NULL) {
        pathElementLength = nextPathElement - pathElement;
        nextPathElement++;
      } else {
        pathElementLength =
            RwEngineInstance->stringFuncs.vecStrlen(pathElement);
      }

      pathSize = pathElementLength +
                 RwEngineInstance->stringFuncs.vecStrlen(name) + extra;
      fullName = ImageGetScratchMem(pathSize);
      if (fullName == NULL)
        return NULL;

      memcpy(fullName, pathElement, pathElementLength);
      RwEngineInstance->stringFuncs.vecStrcpy(fullName + pathElementLength,
                                              name);
      if (callback(fullName, data) == NULL)
        return (RwChar *)name;

      pathElement = nextPathElement;
    }
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
  const RwChar *lastSeparator, *testSeparator, *extension;

  lastSeparator = name;
  testSeparator = RwEngineInstance->stringFuncs.vecStrrchr(lastSeparator, ':');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator = RwEngineInstance->stringFuncs.vecStrrchr(lastSeparator, '/');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator =
      RwEngineInstance->stringFuncs.vecStrrchr(lastSeparator, '\\');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  extension = RwEngineInstance->stringFuncs.vecStrrchr(lastSeparator, '.');

  if (extension != NULL) {
    RwImageFormat *format = IMAGEGLOBALS->formats;

    while (format != NULL) {
      if (!RwEngineInstance->stringFuncs.vecStrcmp(format->extension,
                                                   extension) ||
          !RwEngineInstance->stringFuncs.vecStrcmp(format->alternateExtension,
                                                   extension)) {
        if (format->read != NULL) {
          ImageReadState state;

          state.read = format->read;
          state.image = NULL;
          ImagePathForAllFullNames(name, 5, ImageAttempRead, &state);
          return state.image;
        }
        return NULL;
      }

      format = format->next;
    }

    return NULL;
  }

  return NULL;
}

/*
 * Retail and the clean typed body are instruction-identical. The remaining
 * difference is MWCC selecting _savegpr_28/_restgpr_28 instead of individual
 * saves for the same nonvolatile set.
 */
static RwChar *ImageDetermineExtender(RwChar *name, void *data) {
  RwChar **result = data;
  RwChar *end = name + RwEngineInstance->stringFuncs.vecStrlen(name);
  RwImageFormat *format = IMAGEGLOBALS->formats;

  while (format != NULL) {
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

    format = format->next;
  }

  return name;
}

const RwChar *RwImageFindFileType(const RwChar *name) {
  RwChar *result = NULL;
  ImagePathForAllFullNames(name, 20, ImageDetermineExtender, &result);
  return result;
}

/*
 * Retail and the functional body are instruction-identical. Only the compiler
 * save policy differs: helper calls for r29-r31 versus individual saves.
 */
RwImage *RwImageReadMaskedImage(const RwChar *imageName,
                                const RwChar *maskName) {
  RwImage *image = RwImageRead(imageName);

  if (image != NULL) {
    if (maskName != NULL && maskName[0] != '\0') {
      RwImage *mask = RwImageRead(maskName);

      if (mask == NULL) {
        RwImageDestroy(image);
        return NULL;
      }
      if (RwImageMakeMask(mask) == NULL) {
        RwImageDestroy(image);
        RwImageDestroy(mask);
        return NULL;
      }
      if (RwImageApplyMask(image, mask) == NULL) {
        RwImageDestroy(image);
        RwImageDestroy(mask);
        return NULL;
      }

      RwImageDestroy(mask);
    }

    return image;
  }

  return NULL;
}

RwRGBA *RwRGBASetFromPixel(RwRGBA *color, RwUInt32 pixel, RwInt32 format) {
  RwUInt32 value = pixel;
  (*(RwRGBASetFromPixelCallBack *)((RwUInt8 *)RwEngineInstance + 0x54))(
      color, &value, format);
  return color;
}

/*
 * Retail and current have identical copy/palette CFG and accesses; only the
 * source/destination nonvolatile coloring and corresponding frame differ.
 */
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
  const RwRGBA *palette = (const RwRGBA *)source->palette;
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
      RwRGBA *dstPixel = (RwRGBA *)dst;

      for (x = 0; x < width; x++)
        dstPixel[x] = palette[src[x]];
      src += source->stride;
      dst += destination->stride;
    }
    result = TRUE;
    break;
  case 0x804:
  case 0x2004:
  case 0x2008:
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
  switch (image->depth) {
  case 4:
  case 8: {
    RwRGBA *palette = (RwRGBA *)image->palette;
    RwUInt32 paletteSize = 1 << image->depth;

    if (palette == NULL) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return NULL;
    }
    _rwImageGammaCorrectArrayOfRGBA(palette, palette, paletteSize);
    break;
  }
  case 32: {
    RwUInt8 *row = image->pixels;
    RwInt32 width = image->width;
    RwInt32 height = image->height;
    RwInt32 y;

    if (row == NULL) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return NULL;
    }

    for (y = 0; y < height; y++) {
      _rwImageGammaCorrectArrayOfRGBA((RwRGBA *)row, (RwRGBA *)row, width);
      row += image->stride;
    }
    break;
  }
  default: {
    RwError e;
    e.pluginID = 1;
    e.errorCode = _rwerror(0x80000008);
    RwErrorSet(&e);
    return NULL;
  }
  }

  image->flags |= 2;
  return image;
}

RwBool RwImageSetGamma(RwReal gammaValue) {
  RwReal gammaExponent, inverseGammaExponent;
  RwInt32 i;

  IMAGEGLOBALS->gamma = gammaValue;
  gammaExponent = gammaValue;
  inverseGammaExponent = 1.0f / gammaExponent;

  IMAGEGLOBALS->gammaTable[0] = 0;
  IMAGEGLOBALS->inverseGammaTable[0] = 0;
  for (i = 1; i < 256; i++) {
    RwReal value = (RwReal)i / 255.0f;
    RwReal scaled;
    RwInt32 quantized;

    scaled = powf(value, inverseGammaExponent) * 255.0f;
    quantized = (RwInt32)(scaled + 0.5f);
    IMAGEGLOBALS->gammaTable[i] = (RwUInt8)quantized;

    scaled = powf(value, gammaExponent) * 255.0f;
    quantized = (RwInt32)(scaled + 0.5f);
    IMAGEGLOBALS->inverseGammaTable[i] = (RwUInt8)quantized;
  }
  return TRUE;
}
