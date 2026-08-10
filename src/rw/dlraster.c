#include "libmkparticle/rw_engine.h"
#include "rw/gamecube_texture.h"
#include "rw/rwerror.h"

#define rwRASTERFORMATPIXELFORMATMASK 0x0F00
#define rwRASTERFORMAT1555 0x0100
#define rwRASTERFORMAT565 0x0200
#define rwRASTERFORMAT4444 0x0300
#define rwRASTERFORMATLUM8 0x0400
#define rwRASTERFORMAT8888 0x0500
#define rwRASTERFORMAT888 0x0600
#define rwRASTERFORMATPAL8 0x2000
#define rwRASTERFORMATPAL4 0x4000
#define rwRASTERFORMATMIPMAP 0x8000

#define GX_TF_I4 0
#define GX_TF_I8 1
#define GX_TF_RGB565 4
#define GX_TF_RGB5A3 5
#define GX_TF_RGBA8 6
#define GX_TF_C4 8
#define GX_TF_C8 9
#define GX_TL_RGB565 1
#define GX_TL_RGB5A3 2

#define DL_RASTER_ERROR(code)                                                  \
  do {                                                                         \
    RwError error;                                                             \
    error.pluginID = 1;                                                        \
    error.errorCode = _rwerror(code);                                          \
    RwErrorSet(&error);                                                        \
    return FALSE;                                                              \
  } while (0)

extern RwInt32 RwRasterRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                      RwPluginObjectConstructor constructCB,
                                      RwPluginObjectDestructor destructCB,
                                      RwPluginObjectCopy copyCB);
extern RwInt32 _rwDlFindMSB(RwInt32 value);
extern RwUInt16 _RwDlTokenCurrent;
extern RwUInt16 _RwDlTokenLastSeen;
extern RwTexture *_RwDlTexture;
extern GXRenderModeObj *_RwDlRenderMode;
extern RwBool _rwDlTokenQueryDone(RwUInt16 token);
extern RwBool RwTextureRasterGenerateMipmaps(RwRaster *, RwImage *);
RwBool _rwDlRasterGetNumMipLevels(void *levelsOut, void *rasterIn,
                                  RwInt32 unused);
RwBool _rwDlTextureSetRaster(void *textureIn, void *rasterIn, RwInt32 unused);

static RwUInt32 DlRasterGetMipLevelSize(RwRaster *raster, RwUInt8 level);
static RwUInt32 DlRasterGetMipLevelOffset(RwRaster *raster, RwUInt8 level);
static RwUInt8 DlRasterFindNumMipLevels(RwRaster *raster);

void _rwDlRasterPluginAttach(void) {
  _RwGameCubeRasterExtOffset = RwRasterRegisterPlugin(
      sizeof(RwGameCubeRasterExt), 0x40C, NULL, NULL, NULL);
}

static RwUInt32 DlRasterGetMipLevelSize(RwRaster *raster, RwUInt8 level) {
  RwRaster *parent = raster->parent;
  RwUInt32 width;
  RwUInt32 height;
  RwUInt32 size = 0;

  if ((raster->privateFlags & 6) != 0) {
    width = parent->width;
    height = parent->height;
  } else {
    width = parent->width >> level;
    height = parent->height >> level;
    if (width == 0)
      width = 1;
    if (height == 0)
      height = 1;
  }
  switch (raster->depth) {
  case 4:
    width = (width + 7) & ~7U;
    height = (height + 7) & ~7U;
    size = width * height / 2;
    break;
  case 8:
    width = (width + 7) & ~7U;
    height = (height + 3) & ~3U;
    size = width * height;
    break;
  case 16:
    width = (width + 3) & ~3U;
    height = (height + 3) & ~3U;
    size = width * height * 2;
    break;
  case 32:
    width = (width + 3) & ~3U;
    height = (height + 3) & ~3U;
    size = width * height * 4;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000C);
    RwErrorSet(&error);
    return 0;
  }
  }
  return (size + 31) & ~31U;
}

static RwUInt32 DlRasterGetMipLevelOffset(RwRaster *raster, RwUInt8 level) {
  RwUInt32 offset = 0;

  while (level-- != 0)
    offset += DlRasterGetMipLevelSize(raster, level);
  return offset;
}

RwUInt32 _rwDlRasterGetSize(RwRaster *raster) {
  RwInt32 levels;
  RwUInt32 size = 0;

  _rwDlRasterGetNumMipLevels(&levels, raster, 0);
  while (levels-- != 0)
    size += DlRasterGetMipLevelSize(raster, (RwUInt8)levels);
  return size;
}

RwUInt32 _rwDlRasterGetStride(RwRaster *raster, RwUInt8 level) {
  RwUInt32 width = raster->parent->width >> level;
  RwUInt32 stride = 0;

  if (width == 0)
    width = 1;
  switch (raster->depth) {
  case 4:
    stride = ((width + 7) & ~7U) / 2;
    break;
  case 8:
    stride = (width + 7) & ~7U;
    break;
  case 16:
    stride = ((width + 3) & ~3U) * 2;
    break;
  case 32:
    stride = ((width + 3) & ~3U) * 4;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000C);
    RwErrorSet(&error);
    return 0;
  }
  }
  return stride;
}

static RwUInt8 DlRasterFindNumMipLevels(RwRaster *raster) {
  if ((raster->format << 8 & rwRASTERFORMATMIPMAP) != 0) {
    if (raster->width > raster->height)
      return _rwDlFindMSB(raster->width) + 1;
    return _rwDlFindMSB(raster->height) + 1;
  }
  return 1;
}

RwBool _rwDlRasterGetNumMipLevels(void *levelsOut, void *rasterIn,
                                  RwInt32 unused) {
  RwInt32 *levels = levelsOut;
  RwRaster *raster = rasterIn;
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster->parent);

  if (extension->maxLod != 0xFF)
    *levels = extension->maxLod + 1;
  else
    *levels = DlRasterFindNumMipLevels(raster);
  return TRUE;
}

static void DlRasterTile(void *tiledData, const void *linearData,
                         RwUInt32 width, RwUInt32 height, RwUInt32 depth,
                         RwUInt32 stride) {
  RwUInt8 *destination = tiledData;
  const RwUInt8 *source = linearData;
  RwUInt32 y;

  if (depth == 4) {
    RwUInt32 tilesAcross = (width + 7) / 8;
    for (y = 0; y < height; y++) {
      const RwUInt8 *row = source + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 8) {
        RwUInt32 offset =
            (((y & 7) * 8 + ((y / 8) * tilesAcross + x / 8) * 64) / 2);
        memcpy(destination + offset, row + x / 2, 4);
      }
    }
  } else if (depth == 8) {
    RwUInt32 tilesAcross = (width + 7) / 8;
    for (y = 0; y < height; y++) {
      const RwUInt8 *row = source + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 8) {
        RwUInt32 offset = (y & 3) * 8 + ((y / 4) * tilesAcross + x / 8) * 32;
        memcpy(destination + offset, row + x, 8);
      }
    }
  } else if (depth == 16) {
    RwUInt32 tilesAcross = (width + 3) / 4;
    for (y = 0; y < height; y++) {
      const RwUInt8 *row = source + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 4) {
        RwUInt32 offset =
            ((y & 3) * 4 + ((y / 4) * tilesAcross + x / 4) * 16) * 2;
        memcpy(destination + offset, row + x * 2, 8);
      }
    }
  } else if (depth == 32) {
    RwUInt32 tilesAcross = (width + 3) / 4;
    for (y = 0; y < height; y++) {
      const RwUInt16 *row = (const RwUInt16 *)(source + stride * y);
      RwUInt32 tileOffset = 0;
      RwUInt32 x;
      for (x = 0; x < width; x++) {
        if ((x & 3) == 0)
          tileOffset = ((y / 4) * tilesAcross + x / 4) * 16;
        destination[tileOffset * 4 + ((y & 3) * 4 + (x & 3)) * 2] =
            (RwUInt8)(row[x * 2] >> 8);
        destination[tileOffset * 4 + ((y & 3) * 4 + (x & 3)) * 2 + 1] =
            (RwUInt8)row[x * 2];
        destination[tileOffset * 4 + 32 + ((y & 3) * 4 + (x & 3)) * 2] =
            (RwUInt8)(row[x * 2 + 1] >> 8);
        destination[tileOffset * 4 + 32 + ((y & 3) * 4 + (x & 3)) * 2 + 1] =
            (RwUInt8)row[x * 2 + 1];
      }
    }
  } else {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000C);
    RwErrorSet(&error);
  }
}

static void DlRasterUntile(void *linearData, const void *tiledData,
                           RwUInt32 width, RwUInt32 height, RwUInt32 depth,
                           RwUInt32 stride) {
  RwUInt8 *destination = linearData;
  const RwUInt8 *source = tiledData;
  RwUInt32 y;

  if (depth == 4) {
    RwUInt32 tilesAcross = (width + 7) / 8;
    for (y = 0; y < height; y++) {
      RwUInt8 *row = destination + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 8) {
        RwUInt32 offset =
            (((y & 7) * 8 + ((y / 8) * tilesAcross + x / 8) * 64) / 2);
        memcpy(row + x / 2, source + offset, 4);
      }
    }
  } else if (depth == 8) {
    RwUInt32 tilesAcross = (width + 7) / 8;
    for (y = 0; y < height; y++) {
      RwUInt8 *row = destination + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 8) {
        RwUInt32 offset = (y & 3) * 8 + ((y / 4) * tilesAcross + x / 8) * 32;
        memcpy(row + x, source + offset, 8);
      }
    }
  } else if (depth == 16) {
    RwUInt32 tilesAcross = (width + 3) / 4;
    for (y = 0; y < height; y++) {
      RwUInt8 *row = destination + stride * y;
      RwUInt32 x;
      for (x = 0; x < width; x += 4) {
        RwUInt32 offset =
            ((y & 3) * 4 + ((y / 4) * tilesAcross + x / 4) * 16) * 2;
        memcpy(row + x * 2, source + offset, 8);
      }
    }
  } else if (depth == 32) {
    RwUInt32 tilesAcross = (width + 3) / 4;
    for (y = 0; y < height; y++) {
      RwUInt16 *row = (RwUInt16 *)(destination + stride * y);
      RwUInt32 tileOffset = 0;
      RwUInt32 x;
      for (x = 0; x < width; x++) {
        RwUInt32 pixelOffset;
        if ((x & 3) == 0)
          tileOffset = ((y / 4) * tilesAcross + x / 4) * 16;
        pixelOffset = tileOffset * 4 + ((y & 3) * 4 + (x & 3)) * 2;
        row[x * 2] =
            (RwUInt16)((source[pixelOffset] << 8) | source[pixelOffset + 1]);
        row[x * 2 + 1] = (RwUInt16)((source[pixelOffset + 32] << 8) |
                                    source[pixelOffset + 33]);
      }
    }
  } else {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000C);
    RwErrorSet(&error);
  }
}

RwBool _rwDlRasterLock(void *pixelsOut, void *rasterIn, RwInt32 flags) {
  void **pixels = pixelsOut;
  RwRaster *raster = rasterIn;
  RwRaster *parent = raster->parent;
  RwUInt8 level = (RwUInt8)(flags >> 8);
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(parent);
  RwUInt8 *tiled = (RwUInt8 *)extension->imageData +
                   DlRasterGetMipLevelOffset(raster, level);
  RwUInt32 stride = _rwDlRasterGetStride(raster, level);
  RwUInt32 width = parent->width >> level;
  RwUInt32 height = parent->height >> level;

  if ((raster->type & 7) == 3 || (raster->type & 7) >= 6) {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000E);
    RwErrorSet(&error);
    return FALSE;
  }
  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;
  if ((flags & 8) == 0) {
    extension->reserved_0x24[1] =
        (RwUInt32)RwEngineInstance->fpMalloc(stride * height, 0x30411);
    if (extension->reserved_0x24[1] == 0)
      return FALSE;
  }
  if (parent == raster) {
    raster->originalWidth = raster->width;
    raster->originalHeight = raster->height;
    raster->width = width;
    raster->height = height;
    raster->pixels =
        (flags & 8) != 0 ? tiled : (RwUInt8 *)extension->reserved_0x24[1];
  } else if ((flags & 8) == 0) {
    RwUInt8 *base = (RwUInt8 *)extension->reserved_0x24[1];
    switch (raster->depth) {
    case 4:
      raster->pixels = base + raster->offsetY * stride + raster->offsetX / 2;
      break;
    case 8:
      raster->pixels = base + raster->offsetY * stride + raster->offsetX;
      break;
    case 16:
      raster->pixels = base + raster->offsetY * stride + raster->offsetX * 2;
      break;
    case 32:
      raster->pixels = base + raster->offsetY * stride + raster->offsetX * 4;
      break;
    default:
      return FALSE;
    }
  } else {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000E);
    RwErrorSet(&error);
    return FALSE;
  }
  extension->reserved_0x24[0] = (RwUInt32)tiled;
  raster->stride = stride;
  extension->reserved_0x33 = level;
  if ((flags & 2) != 0) {
    raster->privateFlags |= 2;
    parent->privateFlags |= 2;
    if ((flags & 8) == 0)
      DlRasterUntile((void *)extension->reserved_0x24[1], tiled, raster->width,
                     raster->height, raster->depth, raster->stride);
  }
  if ((flags & 1) != 0) {
    raster->privateFlags |= 4;
    parent->privateFlags |= 4;
    if (extension->token == _RwDlTokenCurrent) {
      GXSetDrawSync(_RwDlTokenCurrent);
      _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (!_rwDlTokenQueryDone(extension->token)) {
    }
  }
  if ((flags & 8) != 0) {
    raster->privateFlags |= 0x20;
    parent->privateFlags |= 0x20;
  }
  *pixels = raster->pixels;
  return TRUE;
}

RwBool _rwDlRasterUnlock(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;
  RwRaster *parent = raster->parent;
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(parent);

  if ((raster->type & 7) == 3 || (raster->type & 7) >= 6) {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000011);
    RwErrorSet(&error);
    return FALSE;
  }
  if ((raster->privateFlags & 4) != 0) {
    if ((raster->privateFlags & 0x20) == 0)
      DlRasterTile((void *)extension->reserved_0x24[0],
                   (void *)extension->reserved_0x24[1], raster->width,
                   raster->height, raster->depth, raster->stride);
    DCFlushRange((void *)extension->reserved_0x24[0],
                 DlRasterGetMipLevelSize(raster, extension->reserved_0x33));
    GXInvalidateTexAll();
  }
  if (parent == raster) {
    raster->width = raster->originalWidth;
    raster->height = raster->originalHeight;
  }
  if ((raster->privateFlags & 0x20) == 0) {
    RwEngineInstance->fpFree((void *)extension->reserved_0x24[1]);
    extension->reserved_0x24[1] = 0;
  }
  raster->stride = 0;
  raster->pixels = NULL;
  if ((raster->privateFlags & 4) != 0 && (raster->format & 0x10) != 0 &&
      extension->reserved_0x33 == 0) {
    extension->reserved_0x33 = 0xFF;
    raster->privateFlags &= ~0x26;
    parent->privateFlags &= ~0x26;
    RwTextureRasterGenerateMipmaps(raster, NULL);
  } else {
    extension->reserved_0x33 = 0xFF;
    raster->privateFlags &= ~0x26;
    parent->privateFlags &= ~0x26;
  }
  return TRUE;
}

RwBool _rwDlRasterLockPalette(void *paletteOut, void *rasterIn, RwInt32 flags) {
  void **palette = paletteOut;
  RwRaster *raster = rasterIn;
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster);

  if ((raster->type & 7) != 0 && (raster->type & 7) != 4) {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000E);
    RwErrorSet(&error);
    return FALSE;
  }
  if (raster == raster->parent && raster->palette == NULL) {
    if ((flags & 2) != 0)
      raster->privateFlags |= 8;
    if ((flags & 1) != 0)
      raster->privateFlags |= 0x10;
    raster->palette = extension->paletteData;
    *palette = raster->palette;
  }
  return TRUE;
}

RwBool _rwDlRasterUnlockPalette(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster);

  if ((raster->type & 7) != 0 && (raster->type & 7) != 4) {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000011);
    RwErrorSet(&error);
    return FALSE;
  }
  if (raster == raster->parent) {
    if ((raster->privateFlags & 0x10) != 0)
      DCFlushRange(extension->paletteData, (1U << raster->depth) * 2);
    raster->privateFlags &= ~0x18;
    raster->palette = NULL;
  }
  return TRUE;
}

static RwBool DlGetRasterFormat(RwRaster *raster, RwInt32 flags) {
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster->parent);
  RwUInt32 format = flags & 0xFF00;
  RwUInt32 type = flags & 7;
  RwUInt32 pal4 = format & rwRASTERFORMATPAL4;
  RwUInt32 pal8 = format & rwRASTERFORMATPAL8;

  raster->type = type;
  raster->flags = flags & ~7;
  if (type == 3 || type >= 6)
    DL_RASTER_ERROR(0x8000000D);

  if (type == 1 || type == 2) {
    if ((format & rwRASTERFORMATPIXELFORMATMASK) == 0) {
      if (_RwDlRenderMode->aa != 0) {
        format |= rwRASTERFORMAT565;
        extension->format = GX_TF_RGB565;
        extension->hasAlpha = FALSE;
        raster->depth = 16;
      } else {
        format |= rwRASTERFORMAT888;
        extension->format = GX_TF_RGBA8;
        extension->hasAlpha = FALSE;
        raster->depth = 32;
      }
    } else if (format != 0x0700 || _RwDlRenderMode->aa == 0) {
      DL_RASTER_ERROR(0x8000000D);
    }
    raster->format = (RwUInt8)(format >> 8);
    return TRUE;
  }

  if (type == 5 && (pal4 != 0 || pal8 != 0))
    DL_RASTER_ERROR(0x8000000C);

  if ((format & rwRASTERFORMATPIXELFORMATMASK) == 0) {
    switch (raster->depth) {
    case 4:
      if (type == 5) {
        format |= rwRASTERFORMAT565;
        extension->format = GX_TF_RGB565;
        extension->hasAlpha = FALSE;
        raster->depth = 16;
      } else {
        format |= rwRASTERFORMAT1555;
        extension->format = GX_TF_RGB5A3;
        extension->hasAlpha = TRUE;
        if (pal4 != 0)
          raster->depth = 4;
        else if (pal8 != 0)
          raster->depth = 8;
        else
          raster->depth = 16;
      }
      break;
    case 8:
      if (pal4 != 0) {
        format |= rwRASTERFORMAT1555;
        extension->format = GX_TF_C4;
        extension->paletteFormat = GX_TL_RGB5A3;
        extension->hasAlpha = TRUE;
      } else {
        extension->format = GX_TF_I4;
        extension->hasAlpha = FALSE;
      }
      break;
    case 16:
      if (pal8 != 0) {
        format |= rwRASTERFORMAT1555;
        extension->format = GX_TF_C8;
        extension->paletteFormat = GX_TL_RGB5A3;
        extension->hasAlpha = TRUE;
      } else {
        extension->format = GX_TF_I8;
        extension->hasAlpha = FALSE;
      }
      break;
    case 32:
      format |= rwRASTERFORMAT8888;
      extension->format = GX_TF_RGBA8;
      extension->hasAlpha = TRUE;
      break;
    default:
      DL_RASTER_ERROR(0x8000000C);
    }
  } else {
    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
      if (pal4 != 0) {
        extension->format = GX_TF_C4;
        extension->paletteFormat = GX_TL_RGB5A3;
        raster->depth = 4;
      } else if (pal8 != 0) {
        extension->format = GX_TF_C8;
        extension->paletteFormat = GX_TL_RGB5A3;
        raster->depth = 8;
      } else {
        extension->format = GX_TF_RGB5A3;
        raster->depth = 16;
      }
      extension->hasAlpha = TRUE;
      break;
    case rwRASTERFORMAT565:
      if (pal4 != 0) {
        extension->format = GX_TF_C4;
        extension->paletteFormat = GX_TL_RGB565;
        raster->depth = 4;
      } else if (pal8 != 0) {
        extension->format = GX_TF_C8;
        extension->paletteFormat = GX_TL_RGB565;
        raster->depth = 8;
      } else {
        extension->format = GX_TF_RGB565;
        raster->depth = 16;
      }
      extension->hasAlpha = FALSE;
      break;
    case rwRASTERFORMAT8888:
      if (pal4 != 0 || pal8 != 0)
        DL_RASTER_ERROR(0x8000000C);
      extension->format = GX_TF_RGBA8;
      extension->hasAlpha = TRUE;
      raster->depth = 32;
      break;
    case rwRASTERFORMAT888:
      if (pal4 != 0 || pal8 != 0)
        DL_RASTER_ERROR(0x8000000C);
      extension->format = GX_TF_RGBA8;
      extension->hasAlpha = FALSE;
      raster->depth = 32;
      break;
    case 0x0A00:
      if (pal4 != 0) {
        extension->format = GX_TF_C4;
        extension->paletteFormat = GX_TL_RGB5A3;
        raster->depth = 4;
      } else if (pal8 != 0) {
        extension->format = GX_TF_C8;
        extension->paletteFormat = GX_TL_RGB5A3;
        raster->depth = 8;
      } else {
        extension->format = GX_TF_RGB5A3;
        raster->depth = 16;
      }
      extension->hasAlpha = FALSE;
      break;
    default:
      DL_RASTER_ERROR(0x8000000D);
    }
  }
  raster->format = (RwUInt8)(format >> 8);
  return TRUE;
}

RwBool _rwDlTextureRasterCreate(RwRaster *raster, RwUInt8 levels) {
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster);
  RwUInt32 size;
  RwUInt32 paletteSize;

  extension->maxLod = levels - 1;
  size = _rwDlRasterGetSize(raster);
  if (((raster->format << 8) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) !=
      0) {
    paletteSize = (1U << raster->depth) * 2;
    extension->reserved_0x18 =
        (RwUInt32)RwEngineInstance->fpMalloc(size + paletteSize + 31, 0x30411);
    if (extension->reserved_0x18 == 0)
      DL_RASTER_ERROR(0x80000013);
    extension->imageData = (void *)((extension->reserved_0x18 + 31) & ~31U);
    extension->paletteData = (RwUInt8 *)extension->imageData + size;
    GXInitTlutObj(&extension->tlut, extension->paletteData,
                  extension->paletteFormat, 1U << raster->depth);
  } else {
    extension->reserved_0x18 =
        (RwUInt32)RwEngineInstance->fpMalloc(size + 31, 0x30411);
    if (extension->reserved_0x18 == 0)
      DL_RASTER_ERROR(0x80000013);
    extension->imageData = (void *)((extension->reserved_0x18 + 31) & ~31U);
    if ((raster->type & 7) == 5)
      DCInvalidateRange(extension->imageData, size);
  }
  return TRUE;
}

RwBool _rwDlRasterCreate(void *unused, void *rasterIn, RwInt32 flags) {
  RwRaster *raster = rasterIn;
  RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster);
  RwUInt32 type;

  raster->stride = 0;
  extension->format = -1;
  extension->paletteFormat = -1;
  extension->hasAlpha = FALSE;
  extension->reserved_0x18 = 0;
  extension->imageData = NULL;
  extension->paletteData = NULL;
  extension->reserved_0x24[0] = 0;
  extension->reserved_0x24[1] = 0;
  extension->textureRegion = NULL;
  extension->token = _RwDlTokenLastSeen;
  extension->maxLod = 0xFF;
  extension->reserved_0x33 = 0xFF;
  if (!DlGetRasterFormat(raster, flags))
    return FALSE;
  if (raster->width == 0 || raster->height == 0) {
    raster->flags = 0x80;
    return TRUE;
  }
  type = raster->type & 7;
  if (type == 0 || type == 4 || type == 5) {
    if ((raster->flags & 0x80) == 0 &&
        !_rwDlTextureRasterCreate(raster, DlRasterFindNumMipLevels(raster))) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(2, "Raster creation failed");
      RwErrorSet(&error);
      return FALSE;
    }
  } else if (type == 1 || type == 2) {
    raster->flags = 0x80;
  } else {
    DL_RASTER_ERROR(0x8000000D);
  }
  return TRUE;
}

RwBool _rwDlRasterDestroy(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;

  if (raster->parent == raster && (raster->flags & 0x80) == 0) {
    RwUInt32 type = raster->type & 7;
    if (type == 0 || type == 4 || type == 5) {
      RwGameCubeRasterExt *extension = RW_GAMECUBE_RASTER_EXTENSION(raster);
      if (extension->token == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
      }
      while (!_rwDlTokenQueryDone(extension->token)) {
      }
      if (_RwDlTexture != NULL && _RwDlTexture->raster == raster)
        _rwDlTextureSetRaster(_RwDlTexture, NULL, 0);
      RwEngineInstance->fpFree((void *)extension->reserved_0x18);
    } else if (type != 1 && type != 2) {
      DL_RASTER_ERROR(0x8000000D);
    }
  }
  return TRUE;
}

RwBool _rwDlTextureSetRaster(void *textureIn, void *rasterIn, RwInt32 unused) {
  RwTexture *texture = textureIn;
  texture->raster = rasterIn;
  RW_GAMECUBE_TEXTURE_EXTENSION(texture)->flags = 0x01000000;
  return TRUE;
}

RwBool _rwDlRasterSubRaster(void *subRasterIn, void *rasterIn, RwInt32 unused) {
  RwRaster *subRaster = subRasterIn;
  RwRaster *raster = rasterIn;
  subRaster->stride = raster->stride;
  subRaster->depth = raster->depth;
  subRaster->type = raster->type;
  subRaster->format = raster->format;
  subRaster->pixels = NULL;
  return TRUE;
}
