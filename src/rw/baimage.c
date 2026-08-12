#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"

extern float powf(float, float);

typedef RwImage *(*RwImageReadCallBack)(const char *);
typedef int (*RwRGBASetFromPixelCallBack)(RwRGBA *, const unsigned int *,
                                             int);
typedef struct RwImageFormat {
  char extension[20];
  char alternateExtension[20];
  RwImageReadCallBack read;
  void *reservedCallback;
  struct RwImageFormat *next;
} RwImageFormat;
typedef struct RwImageGlobals {
  RwFreeList *imageFreeList;
  char *imagePath;
  int imagePathSize;
  unsigned char gammaTable[256];
  unsigned char inverseGammaTable[256];
  float gamma;
  void *scratchMemory;
  int scratchSize;
  RwFreeList *formatFreeList;
  RwImageFormat *formats;
} RwImageGlobals;
typedef struct ImageReadState {
  RwImageReadCallBack read;
  RwImage *image;
} ImageReadState;
typedef char *(*ImagePathCallBack)(char *, void *);

static RwPluginRegistry imageTKList = {
    sizeof(RwImage), sizeof(RwImage), 0, 0, 0, 0};
static RwFreeList _rwImageFreeList;
static RwFreeList _rwImageFormatFreeList;
static int _rwImageFreeListBlockSize = 0x80;
static int _rwImageFreeListPreallocBlocks = 1;
static int _rwImageFormatFreeListPreallocBlocks = 1;
static RwModuleInfo imageModule;
int RwImageSetGamma(float);
RwImage *RwImageCopy(RwImage *, const RwImage *);
RwImage *RwImageFreePixels(RwImage *);
int RwImageDestroy(RwImage *);
RwImage *RwImageAllocatePixels(RwImage *);
RwImage *RwImageCreate(int, int, int);

void *_rwImageOpen(void *instance, int offset, int size) {
  imageModule.globalsOffset = offset;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->imageFreeList = RwFreeListCreateAndPreallocateSpace(
      imageTKList.sizeOfStruct, _rwImageFreeListBlockSize, 4,
      _rwImageFreeListPreallocBlocks, &_rwImageFreeList, 0x40018);
  if (!((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->imageFreeList)
    return 0;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->formatFreeList = RwFreeListCreateAndPreallocateSpace(
      sizeof(RwImageFormat), _rwImageFreeListBlockSize, 4,
      _rwImageFormatFreeListPreallocBlocks, &_rwImageFormatFreeList, 0x40406);
  if (!((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->formatFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePathSize = 0x100;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePath = RwEngineInstance->fpMalloc(
      ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imagePathSize,
      0x01040406);
  if (!((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->imagePath) {
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->imagePath[0] = 0;
  imageModule.numInstances++;
  RwImageSetGamma(1.0f);
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->formats = 0;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->scratchSize = 0x100;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->scratchMemory = RwEngineInstance->fpMalloc(
      ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->scratchSize,
      0x01040018);
  if (!((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                           imageModule.globalsOffset))
           ->scratchMemory) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->imagePath);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePath = 0;
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePathSize = 0;
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
    return 0;
  }
  return instance;
}

void *_rwImageClose(void *instance, int offset, int size) {
  if (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->scratchMemory) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->scratchMemory);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchMemory = 0;
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchSize = 0;
  }
  if (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imagePath) {
    RwEngineInstance->fpFree(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                                 imageModule.globalsOffset))
                                 ->imagePath);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePath = 0;
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imagePathSize = 0;
  }
  while (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                             imageModule.globalsOffset))
             ->formats) {
    RwImageFormat *format = ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                                imageModule.globalsOffset))
                                ->formats;
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formats = format->next;
    RwEngineInstance->fpFreeListFree(
        ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                            imageModule.globalsOffset))
            ->formatFreeList,
        format);
  }
  if (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->formatFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->formatFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->formatFreeList = 0;
  }
  if (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imageFreeList) {
    RwFreeListDestroy(((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                          imageModule.globalsOffset))
                          ->imageFreeList);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->imageFreeList = 0;
  }
  imageModule.numInstances--;
  return instance;
}

static void *ImageGetScratchMem(int size) {
  void *memory;

  if (size > ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                                 imageModule.globalsOffset))
                 ->scratchSize) {
    if (((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                            imageModule.globalsOffset))
            ->scratchMemory)
      memory = RwEngineInstance->fpRealloc(
          ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchMemory = memory;
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->scratchSize = size;
  }
  return ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                             imageModule.globalsOffset))
      ->scratchMemory;
}

void _rwImageGammaCorrectArrayOfRGBA(RwRGBA *out, const RwRGBA *in,
                                     int count) {
  const unsigned char *table = ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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

RwImage *RwImageCreate(int width, int height, int depth) {
  RwImage *image = RwEngineInstance->fpFreeListAlloc(
      ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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

int RwImageDestroy(RwImage *image) {
  if (image->flags & 1)
    RwImageFreePixels(image);
  _rwPluginRegistryDeInitObject(&imageTKList, image);
  RwEngineInstance->fpFreeListFree(
      ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                          imageModule.globalsOffset))
          ->imageFreeList,
      image);
  return 1;
}

RwImage *RwImageAllocatePixels(RwImage *image) {
  unsigned int depth = image->depth;
  int paletted = 1;
  int paletteSize;
  int pixelSize, totalSize;

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
  int i;

  switch (image->depth) {
  case 4:
  case 8: {
    int paletteSize = 1 << image->depth;
    RwRGBA *palette = (RwRGBA *)image->palette;

    for (i = 0; i < paletteSize; i++) {
      int opacity = palette[i].red;

      if (palette[i].green > opacity)
        opacity = palette[i].green;
      if (palette[i].blue > opacity)
        opacity = palette[i].blue;
      palette[i].alpha = (unsigned char)opacity;
    }
    break;
  }
  case 32: {
    unsigned char *row = image->pixels;
    int j;

    for (i = 0; i < image->height; i++) {
      RwRGBA *pixel = (RwRGBA *)row;

      for (j = 0; j < image->width; j++) {
        int opacity = pixel[j].red;

        if (pixel[j].green > opacity)
          opacity = pixel[j].green;
        if (pixel[j].blue > opacity)
          opacity = pixel[j].blue;
        pixel[j].alpha = (unsigned char)opacity;
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
    int i, j;
    const unsigned char *src = mask->pixels;
    const RwRGBA *palette = (const RwRGBA *)mask->palette;
    unsigned char *dst = image->pixels;

    for (i = 0; i < image->height; i++) {
      RwRGBA *dstPixel = (RwRGBA *)dst;

      switch (mask->depth) {
      case 4:
      case 8: {
        const unsigned char *srcIndex = src;

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

static char *ImagePathForAllFullNames(const char *name, int extra,
                                        ImagePathCallBack callback,
                                        void *data) {
  int pathSize;
  char *fullName;
  const char *pathElement;

  pathElement = ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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
      const char *nextPathElement =
          RwEngineInstance->stringFuncs.strchr(pathElement, ';');
      int pathElementLength;

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
        return (char *)name;

      pathElement = nextPathElement;
    }
  }

  return (char *)name;
}

static char *ImageAttempRead(char *name, void *data) {
  ImageReadState *state = data;
  if (RwEngineInstance->fileFuncs.exists(name)) {
    state->image = state->read(name);
    if (state->image)
      return 0;
  }
  return name;
}

RwImage *RwImageRead(const char *name) {
  const char *lastSeparator, *testSeparator, *extension;

  lastSeparator = name;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, ':');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '/');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  testSeparator = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '\\');
  lastSeparator = testSeparator ? testSeparator : lastSeparator;
  extension = RwEngineInstance->stringFuncs.strrchr(lastSeparator, '.');

  if (extension != 0) {
    RwImageFormat *format = ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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

static char *ImageDetermineExtender(char *name, void *data) {
  char **result = data;
  char *end = name + RwEngineInstance->stringFuncs.strlen(name);
  RwImageFormat *format = ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
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

const char *RwImageFindFileType(const char *name) {
  char *result = 0;
  ImagePathForAllFullNames(name, 20, ImageDetermineExtender, &result);
  return result;
}

RwImage *RwImageReadMaskedImage(const char *imageName,
                                const char *maskName) {
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

RwRGBA *RwRGBASetFromPixel(RwRGBA *color, unsigned int pixel, int format) {
  unsigned int value = pixel;
  (*(RwRGBASetFromPixelCallBack *)((unsigned char *)RwEngineInstance + 0x54))(
      color, &value, format);
  return color;
}

static int ImageStraightCopy(RwImage *destination, const RwImage *source) {
  int rowSize;
  unsigned char *src;
  unsigned char *dst;
  int y;
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

static int ImageConvertDepth(RwImage *destination, const RwImage *source) {
  int result = 0;
  int width = destination->width;
  int height = destination->height;
  int x, y;
  const RwRGBA *palette = (const RwRGBA *)source->palette;
  unsigned char *src = source->pixels;
  unsigned char *dst = destination->pixels;
  int conversion = (source->depth << 8) | destination->depth;
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
    unsigned int paletteSize = 1 << image->depth;

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
    unsigned char *row = image->pixels;
    int width = image->width;
    int height = image->height;
    int y;

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

int RwImageSetGamma(float gammaValue) {
  float gammaExponent, inverseGammaExponent;
  int i;

  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->gamma = gammaValue;
  gammaExponent = gammaValue;
  inverseGammaExponent = 1.0f / gammaExponent;

  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->gammaTable[0] = 0;
  ((RwImageGlobals *)((unsigned char *)RwEngineInstance + imageModule.globalsOffset))
      ->inverseGammaTable[0] = 0;
  for (i = 1; i < 256; i++) {
    float value = (float)i / 255.0f;
    float scaled;
    int quantized;

    scaled = powf(value, inverseGammaExponent) * 255.0f;
    quantized = (int)(scaled + 0.5f);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->gammaTable[i] = (unsigned char)quantized;

    scaled = powf(value, gammaExponent) * 255.0f;
    quantized = (int)(scaled + 0.5f);
    ((RwImageGlobals *)((unsigned char *)RwEngineInstance +
                        imageModule.globalsOffset))
        ->inverseGammaTable[i] = (unsigned char)quantized;
  }
  return 1;
}
