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
    sizeof(RwImage), sizeof(RwImage), 0, 0, 0, 0};
static RwFreeList _rwImageFreeList;
static RwFreeList _rwImageFormatFreeList;
static RwInt32 _rwImageFreeListBlockSize = 0x80;
static RwInt32 _rwImageFreeListPreallocBlocks = 1;
static RwInt32 _rwImageFormatFreeListPreallocBlocks = 1;
static RwModuleInfo imageModule;
RwBool RwImageSetGamma(RwReal);
RwImage *RwImageCopy(RwImage *, const RwImage *);
RwImage *RwImageFreePixels(RwImage *);
RwBool RwImageDestroy(RwImage *);
RwImage *RwImageAllocatePixels(RwImage *);
RwImage *RwImageCreate(RwInt32, RwInt32, RwInt32);

void *_rwImageOpen(void *instance, RwInt32 offset, RwInt32 size) {
  imageModule.globalsOffset = offset;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->imageFreeList = RwFreeListCreateAndPreallocateSpace(
      imageTKList.sizeOfStruct, _rwImageFreeListBlockSize, 4,
      _rwImageFreeListPreallocBlocks, &_rwImageFreeList, 0x40018);
  if (!((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->imageFreeList)
    return 0;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->formatFreeList = RwFreeListCreateAndPreallocateSpace(
      sizeof(RwImageFormat), _rwImageFreeListBlockSize, 4,
      _rwImageFormatFreeListPreallocBlocks, &_rwImageFormatFreeList, 0x40406);
  if (!((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->formatFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePathSize = 0x100;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePath = RwEngineInstance->fpMalloc(
      ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imagePathSize,
      0x01040406);
  if (!((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->imagePath) {
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePath[0] = 0;
  imageModule.numInstances++;
  RwImageSetGamma(1.0f);
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->formats = 0;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->scratchSize = 0x100;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->scratchMemory = RwEngineInstance->fpMalloc(
      ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->scratchSize,
      0x01040018);
  if (!((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->scratchMemory) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->imagePath);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePath = 0;
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePathSize = 0;
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  return instance;
}

void *_rwImageClose(void *instance, RwInt32 offset, RwInt32 size) {
  if (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->scratchMemory) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->scratchMemory);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchMemory = 0;
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchSize = 0;
  }
  if (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imagePath) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->imagePath);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePath = 0;
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePathSize = 0;
  }
  while (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                             imageModule.globalsOffset))
             ->formats) {
    RwImageFormat *format = ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                                imageModule.globalsOffset))
                                ->formats;
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formats = format->next;
    RwEngineInstance->fpFreeListFree(
        ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                            imageModule.globalsOffset))
            ->formatFreeList,
        format);
  }
  if (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->formatFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
  }
  if (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imageFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
  }
  imageModule.numInstances--;
  return instance;
}

static void *ImageGetScratchMem(RwInt32 size) {
  void *memory;

  if (size > ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                 imageModule.globalsOffset))
                 ->scratchSize) {
    if (((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                            imageModule.globalsOffset))
            ->scratchMemory)
      memory = RwEngineInstance->fpRealloc(
          ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                              imageModule.globalsOffset))
              ->scratchMemory,
          size, 0x01040018);
    else
      memory = RwEngineInstance->fpMalloc(size, 0x01040018);
    if (!memory) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x80000013, size);
      RwErrorSet(&error);
      return 0;
    }
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchMemory = memory;
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchSize = size;
  }
  return ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                             imageModule.globalsOffset))
      ->scratchMemory;
}

void _rwImageGammaCorrectArrayOfRGBA(RwRGBA *out, const RwRGBA *in,
                                     RwInt32 count) {
  const RwUInt8 *table = ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                             imageModule.globalsOffset))
                             ->gammaTable;
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
  RwImage *image = RwEngineInstance->fpFreeListAlloc(
      ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imageFreeList,
      0x30018);
  if (!image)
    return 0;
  image->width = width;
  image->height = height;
  image->depth = depth;
  image->pixels = 0;
  image->palette = 0;
  image->flags = 0;
  _rwPluginRegistryInitObject(&imageTKList, image);
  return image;
}

RwBool RwImageDestroy(RwImage *image) {
  if (image->flags & 1)
    RwImageFreePixels(image);
  _rwPluginRegistryDeInitObject(&imageTKList, image);
  RwEngineInstance->fpFreeListFree(
      ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imageFreeList,
      image);
  return 1;
}

RwImage *RwImageAllocatePixels(RwImage *image) {
  RwUInt32 depth = image->depth;
  RwBool paletted = 1;
  RwInt32 paletteSize;
  RwInt32 pixelSize, totalSize;

  if (depth != 4 && depth != 8)
    paletted = 0;
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
    return 0;
  }
  image->palette = paletted ? image->pixels + pixelSize : 0;
  image->flags |= 1;
  return image;
}

RwImage *RwImageFreePixels(RwImage *image) {
  RwEngineInstance->fpFree(image->pixels);
  image->pixels = 0;
  image->palette = 0;
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
    return 0;
  }

  switch (image->depth) {
  case 4:
  case 8: {
    RwImage *tempImage =
        RwImageCreate(image->width, image->height, image->depth);

    if (tempImage == 0)
      return 0;
    if (RwImageAllocatePixels(tempImage) == 0) {
      RwImageDestroy(tempImage);
      return 0;
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
    return 0;
  }
  }

  return image;
}

static RwChar *ImagePathForAllFullNames(const RwChar *name, RwInt32 extra,
                                        ImagePathCallBack callback,
                                        void *data) {
  RwInt32 pathSize;
  RwChar *fullName;
  const RwChar *pathElement;

  pathElement = ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                    imageModule.globalsOffset))
                    ->imagePath;
  if (_rwpathisabsolute(name) || pathElement == 0 || pathElement[0] == '\0') {
    pathSize = RwEngineInstance->stringFuncs.strlen(name) + extra;
    fullName = ImageGetScratchMem(pathSize);
    if (fullName == 0)
      return 0;
    RwEngineInstance->stringFuncs.strcpy(fullName, name);
    callback(fullName, data);
  } else {
    while (pathElement != 0 && pathElement[0] != '\0') {
      const RwChar *nextPathElement =
          RwEngineInstance->stringFuncs.strchr(pathElement, ';');
      RwInt32 pathElementLength;

      if (nextPathElement != 0) {
        pathElementLength = nextPathElement - pathElement;
        nextPathElement++;
      } else {
        pathElementLength = RwEngineInstance->stringFuncs.strlen(pathElement);
      }

      pathSize = pathElementLength +
                 RwEngineInstance->stringFuncs.strlen(name) + extra;
      fullName = ImageGetScratchMem(pathSize);
      if (fullName == 0)
        return 0;

      memcpy(fullName, pathElement, pathElementLength);
      RwEngineInstance->stringFuncs.strcpy(fullName + pathElementLength, name);
      if (callback(fullName, data) == 0)
        return (RwChar *)name;

      pathElement = nextPathElement;
    }
  }

  return (RwChar *)name;
}

static RwChar *ImageAttempRead(RwChar *name, void *data) {
  ImageReadState *state = data;
  if (RwEngineInstance->fileFuncs.exists(name)) {
    state->image = state->read(name);
    if (state->image)
      return 0;
  }
  return name;
}

RwImage *RwImageRead(const RwChar *name) {
  const RwChar *lastSeparator, *testSeparator, *extension;

  lastSeparator = name;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, ':');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '/');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '\\');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  extension = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '.');

  if (extension != 0) {
    RwImageFormat *format = ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                                imageModule.globalsOffset))
                                ->formats;

    while (format != 0) {
      if (!RwEngineInstance->stringFuncs.strcmp(format->extension, extension) ||
          !RwEngineInstance->stringFuncs.strcmp(format->alternateExtension,
                                                extension)) {
        if (format->read != 0) {
          ImageReadState state;

          state.read = format->read;
          state.image = 0;
          ImagePathForAllFullNames(name, 5, ImageAttempRead, &state);
          return state.image;
        }
        return 0;
      }

      format = format->next;
    }

    return 0;
  }

  return 0;
}

static RwChar *ImageDetermineExtender(RwChar *name, void *data) {
  RwChar **result = data;
  RwChar *end = name + RwEngineInstance->stringFuncs.strlen(name);
  RwImageFormat *format = ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                                              imageModule.globalsOffset))
                              ->formats;

  while (format != 0) {
    RwEngineInstance->stringFuncs.strcpy(end, format->extension);
    if (RwEngineInstance->fileFuncs.exists(name)) {
      *result = format->extension;
      return 0;
    }
    RwEngineInstance->stringFuncs.strcpy(end, format->alternateExtension);
    if (RwEngineInstance->fileFuncs.exists(name)) {
      *result = format->alternateExtension;
      return 0;
    }

    format = format->next;
  }

  return name;
}

const RwChar *RwImageFindFileType(const RwChar *name) {
  RwChar *result = 0;
  ImagePathForAllFullNames(name, 20, ImageDetermineExtender, &result);
  return result;
}

RwImage *RwImageReadMaskedImage(const RwChar *imageName,
                                const RwChar *maskName) {
  RwImage *image = RwImageRead(imageName);

  if (image != 0) {
    if (maskName != 0 && maskName[0] != '\0') {
      RwImage *mask = RwImageRead(maskName);

      if (mask == 0) {
        RwImageDestroy(image);
        return 0;
      }
      if (RwImageMakeMask(mask) == 0) {
        RwImageDestroy(image);
        RwImageDestroy(mask);
        return 0;
      }
      if (RwImageApplyMask(image, mask) == 0) {
        RwImageDestroy(image);
        RwImageDestroy(mask);
        return 0;
      }

      RwImageDestroy(mask);
    }

    return image;
  }

  return 0;
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
  return 1;
}

static RwBool ImageConvertDepth(RwImage *destination, const RwImage *source) {
  RwBool result = 0;
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
    result = 1;
    break;
  case 0x408:
    for (y = 0; y < height; y++) {
      memcpy(dst, src, width);
      src += source->stride;
      dst += destination->stride;
    }
    result = 1;
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
    result = 1;
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
      destination = 0;
  } else if (!ImageConvertDepth(destination, source))
    destination = 0;
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

    if (palette == 0) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return 0;
    }
    _rwImageGammaCorrectArrayOfRGBA(palette, palette, paletteSize);
    break;
  }
  case 32: {
    RwUInt8 *row = image->pixels;
    RwInt32 width = image->width;
    RwInt32 height = image->height;
    RwInt32 y;

    if (row == 0) {
      RwError e;
      e.pluginID = 1;
      e.errorCode = _rwerror(0x80000016);
      RwErrorSet(&e);
      return 0;
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
    return 0;
  }
  }

  image->flags |= 2;
  return image;
}

RwBool RwImageSetGamma(RwReal gammaValue) {
  RwReal gammaExponent, inverseGammaExponent;
  RwInt32 i;

  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->gamma = gammaValue;
  gammaExponent = gammaValue;
  inverseGammaExponent = 1.0f / gammaExponent;

  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->gammaTable[0] = 0;
  ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance + imageModule.globalsOffset))
      ->inverseGammaTable[0] = 0;
  for (i = 1; i < 256; i++) {
    RwReal value = (RwReal)i / 255.0f;
    RwReal scaled;
    RwInt32 quantized;

    scaled = powf(value, inverseGammaExponent) * 255.0f;
    quantized = (RwInt32)(scaled + 0.5f);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->gammaTable[i] = (RwUInt8)quantized;

    scaled = powf(value, gammaExponent) * 255.0f;
    quantized = (RwInt32)(scaled + 0.5f);
    ((RwImageGlobals *)((RwUInt8 *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->inverseGammaTable[i] = (RwUInt8)quantized;
  }
  return 1;
}
