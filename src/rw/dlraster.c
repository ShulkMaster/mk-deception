#include "libmkparticle/rw_engine.h"
#include "rw/gamecube_texture.h"
#include "rw/rwerror.h"

enum {
  rwRASTERFORMATPIXELFORMATMASK = 0x0F00,
  rwRASTERFORMAT1555 = 0x0100,
  rwRASTERFORMAT565 = 0x0200,
  rwRASTERFORMAT4444 = 0x0300,
  rwRASTERFORMATLUM8 = 0x0400,
  rwRASTERFORMAT8888 = 0x0500,
  rwRASTERFORMAT888 = 0x0600,
  rwRASTERFORMATPAL8 = 0x2000,
  rwRASTERFORMATPAL4 = 0x4000,
  rwRASTERFORMATMIPMAP = 0x8000,
};

enum {
  GX_TF_I4 = 0,
  GX_TF_I8 = 1,
  GX_TF_RGB565 = 4,
  GX_TF_RGB5A3 = 5,
  GX_TF_RGBA8 = 6,
  GX_TF_C4 = 8,
  GX_TF_C8 = 9,
  GX_TL_RGB565 = 1,
  GX_TL_RGB5A3 = 2,
};

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
      sizeof(RwGameCubeRasterExt), 0x40C, 0, 0, 0);
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
    width = width == 0 ? 1 : width;
    height = height == 0 ? 1 : height;
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
  RwInt32 stride = 0;
  RwInt32 width = raster->parent->width >> level;

  width = width == 0 ? 1 : width;

  switch ((RwUInt32)raster->depth) {
  case 4:
    stride = ((width + 7) & ~7) >> 1;
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
  if ((((raster->format & 0xFF) << 8) & rwRASTERFORMATMIPMAP) != 0) {
    if (raster->width > raster->height)
      return (RwUInt8)_rwDlFindMSB(raster->width) + 1;
    return (RwUInt8)_rwDlFindMSB(raster->height) + 1;
  }
  return 1;
}

RwBool _rwDlRasterGetNumMipLevels(void *levelsOut, void *rasterIn,
                                  RwInt32 unused) {
  RwGameCubeRasterExt *extension;
  RwRaster *raster = rasterIn;
  RwInt32 *levels = levelsOut;
  extension = RwGameCubeRasterExtension(raster->parent);

  if (extension->maxLod != 0xFF)
    *levels = extension->maxLod + 1;
  else
    *levels = DlRasterFindNumMipLevels(raster);
  return 1;
}

static void DlRasterTile(void *tiledData, const void *linearData, RwInt32 width,
                         RwInt32 height, RwInt32 depth, RwInt32 stride) {
  RwUInt8 *destination = tiledData;
  const RwUInt8 *source = linearData;
  RwInt32 y;

  switch (depth) {
  case 4: {
    RwInt32 tilesAcross = ((width + 7) & ~7) >> 3;
    for (y = 0; y < height; y++) {
      const RwUInt8 *row = source + stride * y;
      RwInt32 rowInTile = (y & 7) << 3;
      RwInt32 tileRow = tilesAcross * (y >> 3);
      RwInt32 x;
      for (x = 0; x < width; x += 8) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 3)) * 64;
        RwUInt32 byteOffset = (RwUInt32)texelOffset >> 1;
        memcpy(destination + byteOffset, row + (x >> 1), 4);
      }
    }
    break;
  }
  case 8: {
    RwInt32 tilesAcross = ((width + 7) & ~7) >> 3;
    for (y = 0; y < height; y++) {
      const RwUInt8 *row = source + stride * y;
      RwInt32 rowInTile = (y & 3) << 3;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      RwInt32 x;
      for (x = 0; x < width; x += 8) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 3)) * 32;
        RwInt32 byteOffset = texelOffset;
        memcpy(destination + byteOffset, row + x, 8);
      }
    }
    break;
  }
  case 16: {
    RwInt32 tilesAcross = ((width + 3) & ~3) >> 2;
    for (y = 0; y < height; y++) {
      RwInt32 rowInTile = (y & 3) << 2;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      const RwUInt8 *row = source + stride * y;
      RwInt32 x;
      for (x = 0; x < width; x += 4) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 2)) * 16;
        RwInt32 byteOffset = texelOffset << 1;
        memcpy(destination + byteOffset, row + (x << 1), 8);
      }
    }
    break;
  }
  case 32: {
    RwInt32 tilesAcross = ((width + 3) & ~3) >> 2;
    RwUInt16 *tiled = (RwUInt16 *)destination;
    for (y = 0; y < height; y++) {
      const RwUInt16 *row = (const RwUInt16 *)(source + stride * y);
      RwInt32 tileOffset = 0;
      RwInt32 rowInTile = (y & 3) << 2;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      RwInt32 x;
      for (x = 0; x < width; x++) {
        if ((x & 3) == 0)
          tileOffset = (tileRow + (x >> 2)) << 4;
        tiled[(tileOffset << 1) + rowInTile + (x & 3)] = row[x << 1];
        tiled[(tileOffset << 1) + 16 + rowInTile + (x & 3)] = row[(x << 1) + 1];
      }
    }
    break;
  }
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000C);
    RwErrorSet(&error);
    break;
  }
  }
}

static void DlRasterUntile(RwUInt8 *linearData, const RwUInt8 *tiledData,
                           RwInt32 width, RwInt32 height, RwInt32 depth,
                           RwInt32 stride) {
  RwInt32 y;

  if (depth == 4) {
    RwInt32 tilesAcross = ((width + 7) & ~7) >> 3;
    for (y = 0; y < height; y++) {
      RwUInt8 *row = linearData + stride * y;
      RwInt32 rowInTile = (y & 7) << 3;
      RwInt32 tileRow = tilesAcross * (y >> 3);
      RwInt32 x;
      for (x = 0; x < width; x += 8) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 3)) * 64;
        RwUInt32 byteOffset = texelOffset >> 1;
        memcpy(row + (x >> 1), tiledData + byteOffset, 4);
      }
    }
  } else if (depth == 8) {
    RwInt32 tilesAcross = ((width + 7) & ~7) >> 3;
    for (y = 0; y < height; y++) {
      RwUInt8 *row = linearData + stride * y;
      RwInt32 rowInTile = (y & 3) << 3;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      RwInt32 x;
      for (x = 0; x < width; x += 8) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 3)) * 32;
        RwInt32 byteOffset = texelOffset;
        memcpy(row + x, tiledData + byteOffset, 8);
      }
    }
  } else if (depth == 16) {
    RwInt32 tilesAcross = ((width + 3) & ~3) >> 2;
    for (y = 0; y < height; y++) {
      RwInt32 rowInTile = (y & 3) << 2;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      RwUInt8 *row = linearData + stride * y;
      RwInt32 x;
      for (x = 0; x < width; x += 4) {
        RwInt32 texelOffset = rowInTile + (tileRow + (x >> 2)) * 16;
        RwInt32 byteOffset = texelOffset << 1;
        memcpy(row + (x << 1), tiledData + byteOffset, 8);
      }
    }
  } else if (depth == 32) {
    RwInt32 tilesAcross = ((width + 3) & ~3) >> 2;
    const RwUInt16 *tiled = (const RwUInt16 *)tiledData;
    for (y = 0; y < height; y++) {
      RwUInt16 *row = (RwUInt16 *)(linearData + stride * y);
      RwInt32 tileOffset = 0;
      RwInt32 rowInTile = (y & 3) << 2;
      RwInt32 tileRow = tilesAcross * (y >> 2);
      RwInt32 x;
      for (x = 0; x < width; x++) {
        RwInt32 pixelOffset;
        if ((x & 3) == 0)
          tileOffset = (tileRow + (x >> 2)) << 4;
        pixelOffset = (tileOffset << 1) + rowInTile + (x & 3);
        row[x * 2] = tiled[pixelOffset];
        row[x * 2 + 1] = tiled[pixelOffset + 16];
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
  RwUInt8 *tiled;
  RwRaster *raster = rasterIn;
  RwRaster *parent = raster->parent;
  RwUInt8 level = (RwUInt8)((flags & 0xFF00) >> 8);
  RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(parent);

  switch (raster->type & 7) {
  case 0:
  case 4:
  case 5: {
    RwUInt32 stride;
    tiled = (RwUInt8 *)extension->imageData +
            DlRasterGetMipLevelOffset(raster, level);
    stride = _rwDlRasterGetStride(raster, level);

    if ((flags & 8) == 0) {
      RwInt32 height = parent->height >> level;
      height = height == 0 ? 1 : height;
      extension->lockBuffer =
          RwEngineInstance->fpMalloc(stride * height, 0x30411);
      if (extension->lockBuffer == 0)
        return 0;
    }
    if (parent == raster) {
      RwInt32 width = raster->width >> level;
      RwInt32 rootHeight = raster->height >> level;

      width = width == 0 ? 1 : width;
      rootHeight = rootHeight == 0 ? 1 : rootHeight;
      raster->originalWidth = raster->width;
      raster->originalHeight = raster->height;
      raster->width = width;
      raster->height = rootHeight;
      if ((flags & 8) == 0)
        raster->pixels = extension->lockBuffer;
      else
        raster->pixels = tiled;
    } else if ((flags & 8) == 0) {
      switch (raster->depth) {
      case 4:
        raster->pixels = (RwUInt8 *)extension->lockBuffer +
                         stride * raster->offsetY + (raster->offsetX >> 1);
        break;
      case 8:
        raster->pixels = (RwUInt8 *)extension->lockBuffer +
                         stride * raster->offsetY + raster->offsetX;
        break;
      case 16:
        raster->pixels = (RwUInt8 *)extension->lockBuffer +
                         stride * raster->offsetY + raster->offsetX * 2;
        break;
      case 32:
        raster->pixels = (RwUInt8 *)extension->lockBuffer +
                         stride * raster->offsetY + raster->offsetX * 4;
        break;
      default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        return 0;
      }
      }
    } else {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x8000000E);
      RwErrorSet(&error);
      return 0;
    }
    extension->lockedTiledData = tiled;
    raster->stride = stride;
    extension->lockedMipLevel = level;
    if ((flags & 2) != 0) {
      if ((raster->type & 7) == 5)
        DCFlushRange(extension->lockedTiledData,
                     DlRasterGetMipLevelSize(raster, level));
      raster->privateFlags |= 2;
      parent->privateFlags |= 2;
      if ((flags & 8) == 0)
        DlRasterUntile(extension->lockBuffer,
                       extension->lockedTiledData, parent->width,
                       parent->height, raster->depth, raster->stride);
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
    break;
  }
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000E);
    RwErrorSet(&error);
    return 0;
  }
  }
  *pixels = raster->pixels;
  return 1;
}

RwBool _rwDlRasterUnlock(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;
  RwRaster *parent = raster->parent;
  RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(parent);

  switch (raster->type & 7) {
  case 0:
  case 4:
  case 5:
    if ((raster->privateFlags & 4) != 0) {
      if ((raster->privateFlags & 0x20) == 0)
        DlRasterTile(extension->lockedTiledData,
                     extension->lockBuffer, parent->width,
                     parent->height, raster->depth, raster->stride);
      DCFlushRange(extension->lockedTiledData,
                   DlRasterGetMipLevelSize(raster, extension->lockedMipLevel));
      GXInvalidateTexAll();
    }
    if (parent == raster) {
      raster->width = raster->originalWidth;
      raster->height = raster->originalHeight;
    }
    if ((raster->privateFlags & 0x20) == 0) {
      RwEngineInstance->fpFree(extension->lockBuffer);
      extension->lockBuffer = 0;
    }
    raster->stride = 0;
    raster->pixels = 0;
    if ((raster->privateFlags & 4) != 0 && (raster->format & 0x10) != 0 &&
        extension->lockedMipLevel == 0) {
      extension->lockedMipLevel = 0xFF;
      raster->privateFlags &= ~0x26;
      parent->privateFlags &= ~0x26;
      RwTextureRasterGenerateMipmaps(raster, 0);
    } else {
      extension->lockedMipLevel = 0xFF;
      raster->privateFlags &= ~0x26;
      parent->privateFlags &= ~0x26;
    }
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000011);
    RwErrorSet(&error);
    return 0;
  }
  }
  return 1;
}

RwBool _rwDlRasterLockPalette(void *paletteOut, void *rasterIn, RwInt32 flags) {
  void **palette = paletteOut;
  RwRaster *raster = rasterIn;

  switch (raster->type & 7) {
  case 0:
  case 4: {
    if (raster == raster->parent && raster->palette == 0) {
      RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster);
      if ((flags & 2) != 0)
        raster->privateFlags |= 8;
      if ((flags & 1) != 0)
        raster->privateFlags |= 0x10;
      raster->palette = extension->paletteData;
      *palette = raster->palette;
    }
    return 1;
  }
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000E);
    RwErrorSet(&error);
    return 0;
  }
  }
}

RwBool _rwDlRasterUnlockPalette(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;

  switch (raster->type & 7) {
  case 0:
  case 4: {
    if (raster == raster->parent) {
      RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster);
      if ((raster->privateFlags & 0x10) != 0)
        DCFlushRange(extension->paletteData, (1U << raster->depth) * 2);
      raster->privateFlags &= ~0x18;
      raster->palette = 0;
    }
    return 1;
  }
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000011);
    RwErrorSet(&error);
    return 0;
  }
  }
}

static RwBool DlGetRasterFormat(RwRaster *raster, RwInt32 flags) {
  RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster->parent);
  RwUInt32 format = flags & 0xFF00;

  raster->type = flags & 7;
  raster->flags = flags & ~7;
  switch (raster->type) {
  case 0:
  case 4:
  case 5:
    if (raster->type == 5 &&
        (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0) {
      RwError error;
      error.pluginID = 1;
      error.errorCode =
          _rwerror(2, "rwRASTERTYPECAMERATEXTURE can not be palletized");
      RwErrorSet(&error);
      return 0;
    }
    if ((format & rwRASTERFORMATPIXELFORMATMASK) == 0) {
      switch (raster->depth) {
      case 4:
        if (raster->type == 5) {
          format |= rwRASTERFORMAT565;
          extension->format = GX_TF_RGB565;
          extension->hasAlpha = 0;
          raster->depth = 16;
        } else {
          format |= rwRASTERFORMAT1555;
          extension->format = GX_TF_RGB5A3;
          extension->hasAlpha = 1;
          if ((format & rwRASTERFORMATPAL4) != 0)
            raster->depth = 4;
          else if ((format & rwRASTERFORMATPAL8) != 0)
            raster->depth = 8;
          else
            raster->depth = 16;
        }
        break;
      case 8:
        if ((format & rwRASTERFORMATPAL4) != 0) {
          format |= rwRASTERFORMAT1555;
          extension->format = GX_TF_C4;
          extension->paletteFormat = GX_TL_RGB5A3;
          extension->hasAlpha = 1;
        } else {
          extension->format = GX_TF_I4;
          extension->hasAlpha = 0;
        }
        break;
      case 16:
        if ((format & rwRASTERFORMATPAL8) != 0) {
          format |= rwRASTERFORMAT1555;
          extension->format = GX_TF_C8;
          extension->paletteFormat = GX_TL_RGB5A3;
          extension->hasAlpha = 1;
        } else {
          extension->format = GX_TF_I8;
          extension->hasAlpha = 0;
        }
        break;
      case 24:
        format |= rwRASTERFORMAT1555;
        extension->format = GX_TF_RGB5A3;
        extension->hasAlpha = 1;
        break;
      case 32:
        format |= rwRASTERFORMAT8888;
        extension->format = GX_TF_RGBA8;
        extension->hasAlpha = 1;
        break;
      default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        return 0;
      }
      }
    } else {
      switch (format & rwRASTERFORMATPIXELFORMATMASK) {
      case rwRASTERFORMAT1555:
      case rwRASTERFORMAT4444:
        if ((format & rwRASTERFORMATPAL4) != 0) {
          extension->format = GX_TF_C4;
          extension->paletteFormat = GX_TL_RGB5A3;
          raster->depth = 4;
        } else if ((format & rwRASTERFORMATPAL8) != 0) {
          extension->format = GX_TF_C8;
          extension->paletteFormat = GX_TL_RGB5A3;
          raster->depth = 8;
        } else {
          extension->format = GX_TF_RGB5A3;
          raster->depth = 16;
        }
        extension->hasAlpha = 1;
        break;
      case rwRASTERFORMAT565:
        if ((format & rwRASTERFORMATPAL4) != 0) {
          extension->format = GX_TF_C4;
          extension->paletteFormat = GX_TL_RGB565;
          raster->depth = 4;
        } else if ((format & rwRASTERFORMATPAL8) != 0) {
          extension->format = GX_TF_C8;
          extension->paletteFormat = GX_TL_RGB565;
          raster->depth = 8;
        } else {
          extension->format = GX_TF_RGB565;
          raster->depth = 16;
        }
        extension->hasAlpha = 0;
        break;
      case rwRASTERFORMAT8888:
        if ((format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0) {
          RwError error;
          error.pluginID = 1;
          error.errorCode = _rwerror(2, "rwRASTERFORMAT8888 invalid format");
          RwErrorSet(&error);
          return 0;
        }
        extension->format = GX_TF_RGBA8;
        extension->hasAlpha = 1;
        raster->depth = 32;
        break;
      case rwRASTERFORMAT888:
        if ((format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0) {
          RwError error;
          error.pluginID = 1;
          error.errorCode = _rwerror(2, "rwRASTERFORMAT888 invalid format");
          RwErrorSet(&error);
          return 0;
        }
        extension->format = GX_TF_RGBA8;
        extension->hasAlpha = 0;
        raster->depth = 32;
        break;
      case 0x0A00:
        if ((format & rwRASTERFORMATPAL4) != 0) {
          extension->format = GX_TF_C4;
          extension->paletteFormat = GX_TL_RGB5A3;
          raster->depth = 4;
        } else if ((format & rwRASTERFORMATPAL8) != 0) {
          extension->format = GX_TF_C8;
          extension->paletteFormat = GX_TL_RGB5A3;
          raster->depth = 8;
        } else {
          extension->format = GX_TF_RGB5A3;
          raster->depth = 16;
        }
        extension->hasAlpha = 0;
        break;
      case rwRASTERFORMATLUM8: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "rwRASTERFORMATLUM8 invalid format");
        RwErrorSet(&error);
        return 0;
      }
      case 0x0700: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "rwRASTERFORMAT16 invalid format");
        RwErrorSet(&error);
        return 0;
      }
      case 0x0800: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "rwRASTERFORMAT24 invalid format");
        RwErrorSet(&error);
        return 0;
      }
      case 0x0900: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "rwRASTERFORMAT32 invalid format");
        RwErrorSet(&error);
        return 0;
      }
      default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        return 0;
      }
      }
    }
    break;
  case 1:
  case 2:
    if ((format & rwRASTERFORMATPIXELFORMATMASK) == 0) {
      if (_RwDlRenderMode->aa != 0) {
        format |= rwRASTERFORMAT565;
        extension->format = GX_TF_RGB565;
        extension->hasAlpha = 0;
        raster->depth = 16;
      } else {
        format |= rwRASTERFORMAT888;
        extension->format = GX_TF_RGBA8;
        extension->hasAlpha = 0;
        raster->depth = 32;
      }
    } else if (format != 0x0700 || _RwDlRenderMode->aa == 0) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x8000000D);
      RwErrorSet(&error);
      return 0;
    }
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x8000000D);
    RwErrorSet(&error);
    return 0;
  }
  }
  raster->format = (RwUInt8)(format >> 8);
  return 1;
}

RwBool _rwDlTextureRasterCreate(RwRaster *raster, RwUInt8 levels) {
  RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster);
  RwUInt32 size;
  RwUInt32 paletteSize;

  extension->maxLod = levels - 1;
  size = _rwDlRasterGetSize(raster);
  if ((((raster->format & 0xFF) << 8) &
       (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0) {
    paletteSize = (1U << raster->depth) * 2;
    extension->allocation =
        RwEngineInstance->fpMalloc(size + paletteSize + 31, 0x30411);
    if (extension->allocation == 0) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x80000013, size + paletteSize + 31);
      RwErrorSet(&error);
      return 0;
    }
    extension->imageData =
        (void *)(((RwUInt32)extension->allocation + 31) & ~31U);
    extension->paletteData = (RwUInt8 *)extension->imageData + size;
    GXInitTlutObj(&extension->tlut, extension->paletteData,
                  extension->paletteFormat, 1U << raster->depth);
  } else {
    extension->allocation = RwEngineInstance->fpMalloc(size + 31, 0x30411);
    if (extension->allocation == 0) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x80000013, size + 31);
      RwErrorSet(&error);
      return 0;
    }
    extension->imageData =
        (void *)(((RwUInt32)extension->allocation + 31) & ~31U);
    if (raster->type == 5)
      DCInvalidateRange(extension->imageData, size);
  }
  return 1;
}

RwBool _rwDlRasterCreate(void *unused, void *rasterIn, RwInt32 flags) {
  RwRaster *raster = rasterIn;
  RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster);

  raster->stride = 0;
  extension->format = 0xFF;
  extension->paletteFormat = 0xFF;
  extension->hasAlpha = 0;
  extension->allocation = 0;
  extension->imageData = 0;
  extension->paletteData = 0;
  extension->lockedTiledData = 0;
  extension->lockBuffer = 0;
  extension->textureRegion = 0;
  extension->token = (RwUInt16)_RwDlTokenLastSeen;
  extension->maxLod = 0xFF;
  extension->lockedMipLevel = 0xFF;
  if (!DlGetRasterFormat(raster, flags))
    return 0;
  if (raster->width != 0 && raster->height != 0) {
    switch (raster->type) {
    case 0:
    case 4:
    case 5:
      if ((raster->flags & 0x80) == 0 &&
          !_rwDlTextureRasterCreate(raster, DlRasterFindNumMipLevels(raster))) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "Raster creation failed");
        RwErrorSet(&error);
        return 0;
      }
      break;
    case 1:
    case 2:
      raster->flags = 0x80;
      break;
    default: {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x8000000D);
      RwErrorSet(&error);
      return 0;
    }
    }
  } else {
    raster->flags = 0x80;
    return 1;
  }
  return 1;
}

RwBool _rwDlRasterDestroy(void *unused, void *rasterIn, RwInt32 in) {
  RwRaster *raster = rasterIn;

  if (raster->parent == raster && (raster->flags & 0x80) == 0) {
    switch (raster->type) {
    case 0:
    case 4:
    case 5: {
      RwGameCubeRasterExt *extension = RwGameCubeRasterExtension(raster);
      if (extension->token == _RwDlTokenCurrent) {
        GXSetDrawSync((RwUInt32)_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
      }
      while (!_rwDlTokenQueryDone(extension->token)) {
      }
      if (_RwDlTexture != 0 && raster == _RwDlTexture->raster)
        _rwDlTextureSetRaster(_RwDlTexture, 0, 0);
      RwEngineInstance->fpFree(extension->allocation);
      break;
    }
    case 1:
    case 2:
      break;
    default: {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x8000000D);
      RwErrorSet(&error);
      return 0;
    }
    }
  }
  return 1;
}

RwBool _rwDlTextureSetRaster(void *textureIn, void *rasterIn, RwInt32 unused) {
  RwGameCubeTextureExt *extension;
  ((RwTexture *)textureIn)->raster = rasterIn;
  extension = RwGameCubeTextureExtension((RwTexture *)textureIn);
  extension->flags = 0x01000000;
  return 1;
}

RwBool _rwDlRasterSubRaster(void *subRasterIn, void *rasterIn, RwInt32 unused) {
  RwRaster *subRaster = subRasterIn;
  RwRaster *raster = rasterIn;
  subRaster->stride = raster->stride;
  subRaster->depth = raster->depth;
  subRaster->type = raster->type;
  subRaster->format = raster->format;
  subRaster->pixels = 0;
  return 1;
}
