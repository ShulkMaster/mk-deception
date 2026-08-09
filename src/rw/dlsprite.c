#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

typedef struct RwRect {
    RwInt32 x;
    RwInt32 y;
    RwInt32 w;
    RwInt32 h;
} RwRect;

typedef struct RwDlStateCache {
    RwBool zWriteEnable;
    RwBool zTestEnable;
    RwInt32 zCompare;
    RwInt32 cullMode;
    RwBool fogEnable;
    RwInt32 fogType;
    RwUInt32 fogColor;
    GXColor gxFogColor;
    RwInt32 fogDensity;
    RwReal fogStart;
    RwReal fogEnd;
    RwReal fogNear;
    RwReal fogFar;
    RwInt32 srcBlend;
    RwInt32 dstBlend;
    RwBool zCompLoc;
    RwInt32 alphaCompare0;
    RwInt32 alphaCompare1;
    RwInt32 alphaOperation;
    RwUInt8 alphaRef0;
    RwUInt8 alphaRef1;
    RwUInt8 alphaMode;
    RwUInt8 reserved_4F;
} RwDlStateCache;

RwRaster* _RwDlRasterTarget;

extern GXRenderModeObj* _RwDlRenderMode;
extern RwInt32 _RwDlFSAA;
extern RwInt32 _RwDlFSAATop;
extern RwInt32 _RwDlHalfHeight;
extern RwInt32 _RwDlPixelFormat;
extern RwInt32 _RwDlCurPixelFormat;
extern RwDlStateCache _RwDlStateCache;

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern RwRGBA* RwRGBASetFromPixel(RwRGBA* color, RwUInt32 pixel,
                                  RwInt32 format);
extern RwBool _rwDlGetRenderState(RwInt32 state, void* value);
extern RwBool _rwDlSetRenderState(RwInt32 state, void* value);
extern RwBool _rwDlRenderStateFogEnable(RwUInt32 enable);
extern void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture);
extern void _rwDlSetRenderStateSrcDestBlend(RwInt32 source,
                                             RwInt32 destination);

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix);
static void GXEnd(void);
static void GXTexCoord2f32(RwReal s, RwReal t);
static void GXPosition2s16(RwInt16 x, RwInt16 y);
static void GXPosition3f32(RwReal x, RwReal y, RwReal z);

static void _rwDlRasterRenderQuadInit(const RwRect* rectangle)
{
    /* Retail expands the stock viewport/scissor macros with several retained
     * intermediate values. The viewport, FSAA clipping, projection, and GX
     * state operations below follow the same branches and access widths. */
    static RwReal projVector[7] = {
        1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f
    };
    static Mtx posMatrix = {
        {1.0f, 0.0f, 0.0f, 0.5f},
        {0.0f, 1.0f, 0.0f, 0.5f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(0, 1, 4, 0x3C);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0, 0, 0xFF);
    GXSetTevOp(0, 3);
    GXSetNumChans(0);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);

    if (_RwDlRenderMode->field_rendering != 0) {
        GXSetViewportJitter(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                            _RwDlRenderMode->xfbHeight, 0.0f, 1.0f,
                            VIGetNextField() ^ 1);
    } else {
        GXSetViewport(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                      _RwDlRenderMode->xfbHeight, 0.0f, 1.0f);
    }

    if (_RwDlFSAA == 0) {
        GXSetScissor(_RwDlRasterTarget->offsetX + rectangle->x,
                     _RwDlRasterTarget->offsetY + rectangle->y,
                     rectangle->w, rectangle->h);
    } else if (_RwDlFSAATop != 0) {
        if (_RwDlRasterTarget->offsetY * 2 + rectangle->y + rectangle->h <=
            _RwDlHalfHeight + 2) {
            GXSetScissor(_RwDlRasterTarget->offsetX + rectangle->x,
                         _RwDlRasterTarget->offsetY * 2 + rectangle->y,
                         rectangle->w, rectangle->h);
        } else if (_RwDlRasterTarget->offsetY * 2 + rectangle->y >
                   _RwDlHalfHeight + 2) {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth,
                         _RwDlHalfHeight + 2);
        } else {
            RwInt32 top = _RwDlRasterTarget->offsetY * 2 + rectangle->y;
            GXSetScissor(_RwDlRasterTarget->offsetX + rectangle->x, top,
                         rectangle->w, _RwDlHalfHeight + 2 - top);
        }
        GXSetScissorBoxOffset(0, 0);
    } else {
        if (_RwDlRasterTarget->offsetY * 2 + rectangle->y >=
            _RwDlHalfHeight - 2) {
            GXSetScissor(_RwDlRasterTarget->offsetX + rectangle->x,
                         _RwDlRasterTarget->offsetY * 2 + rectangle->y,
                         rectangle->w, rectangle->h);
        } else if (_RwDlRasterTarget->offsetY * 2 + rectangle->y +
                       rectangle->h < _RwDlHalfHeight - 2) {
            GXSetScissor(0, _RwDlHalfHeight - 2,
                         _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
        } else {
            RwInt32 boundary = _RwDlHalfHeight - 2;
            RwInt32 height = rectangle->h;
            GXSetScissor(_RwDlRasterTarget->offsetX + rectangle->x,
                         boundary, rectangle->w,
                         _RwDlRasterTarget->offsetY * 2 + rectangle->y +
                             height - boundary);
        }
        GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
    }

    projVector[1] = 2.0f / _RwDlRenderMode->fbWidth;
    projVector[3] = -2.0f / _RwDlRenderMode->xfbHeight;
    GXSetProjectionv(projVector);
    GXLoadPosMtxImm(posMatrix, 0);
    GXSetCurrentMtx(0);
}

static void _rwDlRasterRenderQuad(RwRaster* raster, const RwRect* rectangle,
                                  RwBool scaled, RwBool alpha)
{
    /* The operational body is recovered; retail's stock raster/texture macros
     * retain additional address temporaries and therefore a wider save frame. */
    RwRaster* source = raster->parent;
    void* fogEnable;
    void* filterMode;
    void* addressU;
    void* addressV;
    void* oldRaster;
    RwReal recipWidth;
    RwReal recipHeight;

    _rwDlGetRenderState(14, &fogEnable);
    _rwDlGetRenderState(9, &filterMode);
    _rwDlGetRenderState(3, &addressU);
    _rwDlGetRenderState(4, &addressV);
    _rwDlGetRenderState(1, &oldRaster);
    GXSetCullMode(0);
    if (alpha != FALSE)
        _rwDlSetRenderStateSrcDestBlend(5, 6);
    else
        _rwDlSetRenderStateSrcDestBlend(2, 1);
    _rwDlSetRenderState(14, NULL);
    _rwDlSetRenderState(9, (void*)(scaled != FALSE ? 2 : 1));
    _rwDlSetRenderState(2, (void*)3);
    _rwDlSetRenderState(1, source);
    _rwDlTextureRasterFlush();
    _rwDlRasterRenderQuadInit(rectangle);

    recipWidth = 1.0f / source->width;
    recipHeight = 1.0f / source->height;
    GXBegin(0x80, 0, 4);
    if (scaled != FALSE) {
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x,
                       _RwDlRasterTarget->offsetY + rectangle->y);
        GXTexCoord2f32(raster->offsetX * recipWidth,
                       raster->offsetY * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x,
                       _RwDlRasterTarget->offsetY + rectangle->y + rectangle->h);
        GXTexCoord2f32(raster->offsetX * recipWidth,
                       (raster->offsetY + raster->height) * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x + rectangle->w,
                       _RwDlRasterTarget->offsetY + rectangle->y + rectangle->h);
        GXTexCoord2f32((raster->offsetX + raster->width) * recipWidth,
                       (raster->offsetY + raster->height) * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x + rectangle->w,
                       _RwDlRasterTarget->offsetY + rectangle->y);
        GXTexCoord2f32((raster->offsetX + raster->width) * recipWidth,
                       raster->offsetY * recipHeight);
    } else {
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x,
                       _RwDlRasterTarget->offsetY + rectangle->y);
        GXTexCoord2f32(raster->offsetX * recipWidth,
                       raster->offsetY * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x,
                       _RwDlRasterTarget->offsetY + rectangle->y + raster->height);
        GXTexCoord2f32(raster->offsetX * recipWidth,
                       (raster->offsetY + raster->height) * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x + raster->width,
                       _RwDlRasterTarget->offsetY + rectangle->y + raster->height);
        GXTexCoord2f32((raster->offsetX + raster->width) * recipWidth,
                       (raster->offsetY + raster->height) * recipHeight);
        GXPosition2s16(_RwDlRasterTarget->offsetX + rectangle->x + raster->width,
                       _RwDlRasterTarget->offsetY + rectangle->y);
        GXTexCoord2f32((raster->offsetX + raster->width) * recipWidth,
                       raster->offsetY * recipHeight);
    }
    GXEnd();

    GXSetCullMode(_RwDlStateCache.cullMode - 1);
    _rwDlSetRenderStateSrcDestBlend(_RwDlStateCache.srcBlend,
                                     _RwDlStateCache.dstBlend);
    _rwDlSetRenderState(14, fogEnable);
    _rwDlSetRenderState(9, filterMode);
    _rwDlSetRenderState(3, addressU);
    _rwDlSetRenderState(4, addressV);
    _rwDlSetRenderState(1, oldRaster);
}

static RwBool _rwDlRasterRenderGeneric(RwRaster* raster, RwRect* rectangle,
                                       RwBool scaled, RwBool alpha)
{
    /* The retail raster-type decision tree and both error paths are complete.
     * Remaining broad differences are O0 switch/error-macro source lowering. */
    RwRect destination = *rectangle;

    if (scaled == FALSE) {
        destination.w = raster->width;
        destination.h = raster->height;
    }

    switch (_RwDlRasterTarget->type) {
    case 2:
        switch (raster->type) {
        case 0:
        case 4:
        case 5:
            _rwDlRasterRenderQuad(raster, &destination, scaled, alpha);
            break;
        case 3:
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(
                2, "SRC, DST Raster render combination not supported");
            RwErrorSet(&error);
            return FALSE;
        }
        }
        break;
    case 0:
    case 4:
    case 5:
        if (raster->type != 3) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(
                2, "SRC, DST Raster render combination not supported");
            RwErrorSet(&error);
            return FALSE;
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(
            2, "SRC, DST Raster render combination not supported");
        RwErrorSet(&error);
        return FALSE;
    }
    }
    return TRUE;
}

RwBool _rwDlRasterRender(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, FALSE, TRUE);
}

RwBool _rwDlRasterRenderFast(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, FALSE, FALSE);
}

RwBool _rwDlRasterRenderScaled(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, TRUE, TRUE);
}

static void _rwDlRasterCamera_ZClearRectInit(RwRaster* raster)
{
    static RwReal projVector[7] = {
        1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f
    };
    static Mtx posMatrix = {
        {1.0f, 0.0f, 0.0f, 0.5f},
        {0.0f, 1.0f, 0.0f, 0.5f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);

    if (raster->type == 5) {
        RwGameCubeRasterExt* extension =
            RW_GAMECUBE_RASTER_EXTENSION(raster->parent);
        if ((extension->hasAlpha & 1) != 0) {
            if (_RwDlPixelFormat != 1) {
                GXSetPixelFmt(1, 0);
                _RwDlCurPixelFormat = 1;
            }
        } else if (_RwDlPixelFormat != 0) {
            GXSetPixelFmt(0, 0);
            _RwDlCurPixelFormat = 0;
        }
        GXSetViewport(0.0f, 0.0f, 640.0f, 528.0f, 0.0f, 1.0f);
        GXSetScissor(0, 0, 640, 528);
        projVector[1] = 2.0f / 639.0f;
        projVector[3] = -2.0f / 527.0f;
    } else {
        if (_RwDlCurPixelFormat != _RwDlPixelFormat) {
            GXSetPixelFmt(_RwDlPixelFormat, 0);
            _RwDlCurPixelFormat = _RwDlPixelFormat;
        }
        if (_RwDlRenderMode->field_rendering != 0) {
            GXSetViewportJitter(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                                _RwDlRenderMode->xfbHeight, 0.0f, 1.0f,
                                VIGetNextField() ^ 1);
        } else {
            GXSetViewport(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                          _RwDlRenderMode->xfbHeight, 0.0f, 1.0f);
        }
        if (_RwDlFSAA == 0) {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth,
                         _RwDlRenderMode->xfbHeight);
        } else if (_RwDlFSAATop != 0) {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth,
                         _RwDlHalfHeight + 2);
            GXSetScissorBoxOffset(0, 0);
        } else {
            GXSetScissor(0, _RwDlHalfHeight - 2,
                         _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
            GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
        }
        projVector[1] = 2.0f / _RwDlRenderMode->fbWidth;
        projVector[3] = -2.0f / _RwDlRenderMode->xfbHeight;
    }
    GXSetProjectionv(projVector);
    GXLoadPosMtxImm(posMatrix, 0);
    GXSetCurrentMtx(0);
}

void _rwDlRasterCamera_ZClearRect(RwRaster* raster, const RwRect* rectangle,
                                  const RwRGBA* color, RwInt32 clearMode)
{
    /* Retail and current perform the same clear-state transition and restore;
     * only byte-color temporary lowering remains compiler-dependent. */
    GXColor material;
    RwBool fogWasEnabled = FALSE;

    if ((clearMode & 1) != 0) {
        material.r = color->red;
        material.g = color->green;
        material.b = color->blue;
        material.a = color->alpha;
        if ((clearMode & 2) != 0)
            GXSetZMode(TRUE, 7, TRUE);
        else
            GXSetZMode(TRUE, 7, FALSE);
    } else if ((clearMode & 2) != 0) {
        material.r = 0;
        material.g = 0;
        material.b = 0;
        material.a = 0xFF;
        GXSetZMode(TRUE, 7, TRUE);
        GXSetColorUpdate(FALSE);
    } else {
        return;
    }

    GXSetChanMatColor(4, material);
    GXSetBlendMode(1, 1, 0, 0);
    GXSetCullMode(0);
    _rwDlRenderStateSetZCompLoc(TRUE);
    if (_RwDlStateCache.fogEnable != FALSE) {
        _rwDlRenderStateFogEnable(FALSE);
        fogWasEnabled = TRUE;
    }
    _rwDlRasterCamera_ZClearRectInit(raster);
    GXBegin(0x80, 0, 4);
    GXPosition3f32((RwReal)rectangle->x, (RwReal)rectangle->y, 0.99999994f);
    GXPosition3f32((RwReal)rectangle->x,
                   (RwReal)(rectangle->y + rectangle->h), 0.99999994f);
    GXPosition3f32((RwReal)(rectangle->x + rectangle->w),
                   (RwReal)(rectangle->y + rectangle->h), 0.99999994f);
    GXPosition3f32((RwReal)(rectangle->x + rectangle->w),
                   (RwReal)rectangle->y, 0.99999994f);
    GXEnd();

    if (fogWasEnabled != FALSE)
        _rwDlRenderStateFogEnable(TRUE);
    _rwDlSetRenderStateSrcDestBlend(_RwDlStateCache.srcBlend,
                                     _RwDlStateCache.dstBlend);
    GXSetZMode(TRUE, _RwDlStateCache.zCompare,
               (RwUInt8)_RwDlStateCache.zWriteEnable);
    GXSetCullMode(_RwDlStateCache.cullMode - 1);
    GXSetColorUpdate(TRUE);
}

static RwBool _rwDlRasterClearGeneric(RwRaster* raster, RwRect* rectangle,
                                      RwInt32 pixel)
{
    /* Direct retail behavior: camera/Z clears for types 2/1, otherwise the
     * locked 4/8/16/32-bit CPU paths. The depth switch/save-frame lowering is
     * still materially different, so this function is not classified near. */
    RwBool locked = FALSE;
    RwInt32 y;

    switch (_RwDlRasterTarget->type) {
    case 2: {
        RwRGBA color;
        RwRGBASetFromPixel(&color, pixel, _RwDlRasterTarget->format << 8);
        rectangle->x += _RwDlRasterTarget->offsetX;
        rectangle->y += _RwDlRasterTarget->offsetY;
        _rwDlRasterCamera_ZClearRect(raster, rectangle, &color, 1);
        return TRUE;
    }
    case 1:
        rectangle->x += _RwDlRasterTarget->offsetX;
        rectangle->y += _RwDlRasterTarget->offsetY;
        _rwDlRasterCamera_ZClearRect(raster, rectangle, NULL, 2);
        return TRUE;
    case 0:
    case 4:
    case 5:
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        return FALSE;
    }
    }

    if (_RwDlRasterTarget->parent == _RwDlRasterTarget &&
        _RwDlRasterTarget->offsetX == 0 &&
        _RwDlRasterTarget->offsetY == 0 &&
        _RwDlRasterTarget->width == rectangle->w &&
        _RwDlRasterTarget->height == rectangle->h) {
        if ((_RwDlRasterTarget->privateFlags & 4) == 0) {
            locked = TRUE;
            RwRasterLock(_RwDlRasterTarget, 0, 9);
        }
        if (pixel == 0 || pixel == -1) {
            memset(_RwDlRasterTarget->pixels, pixel,
                   _RwDlRasterTarget->stride * _RwDlRasterTarget->height);
            if (locked != FALSE)
                RwRasterUnlock(_RwDlRasterTarget);
            return TRUE;
        }
    } else if ((_RwDlRasterTarget->privateFlags & 4) == 0) {
        locked = TRUE;
        RwRasterLock(_RwDlRasterTarget, 0, 3);
    }

    switch (_RwDlRasterTarget->depth) {
    case 4: {
        RwUInt8 value = (RwUInt8)(((pixel & 0xF) << 4) | (pixel & 0xF));
        for (y = 0; y < rectangle->h; y++) {
            RwUInt8* destination = _RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y) +
                (rectangle->x >> 1);
            memset(destination, value, rectangle->w >> 1);
        }
        break;
    }
    case 8:
        for (y = 0; y < rectangle->h; y++) {
            RwUInt8* destination = _RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y) + rectangle->x;
            memset(destination, pixel, rectangle->w);
        }
        break;
    case 16:
        for (y = 0; y < rectangle->h; y++) {
            RwUInt16* destination = (RwUInt16*)(_RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y)) + rectangle->x;
            RwInt32 x;
            for (x = 0; x < rectangle->w; x++)
                *destination++ = (RwUInt16)pixel;
        }
        break;
    case 32:
        for (y = 0; y < rectangle->h; y++) {
            RwUInt32* destination = (RwUInt32*)(_RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y)) + rectangle->x;
            RwInt32 x;
            for (x = 0; x < rectangle->w; x++)
                *destination++ = (RwUInt32)pixel;
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        break;
    }
    }

    if (locked != FALSE)
        RwRasterUnlock(_RwDlRasterTarget);
    return TRUE;
}

RwBool _rwDlRasterClearRect(void* unused, RwRect* rectangle, RwInt32 pixel)
{
    return _rwDlRasterClearGeneric(_RwDlRasterTarget, rectangle, pixel);
}

RwBool _rwDlRasterClear(void* unused0, void* unused1, RwInt32 pixel)
{
    RwRect rectangle;
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.w = _RwDlRasterTarget->width;
    rectangle.h = _RwDlRasterTarget->height;
    return _rwDlRasterClearGeneric(_RwDlRasterTarget, &rectangle, pixel);
}

RwBool _rwDlSetRasterContext(void* out, void* inOut, RwInt32 in)
{
    RwRaster* raster = inOut;
    _RwDlRasterTarget = raster;
    return TRUE;
}

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix)
{
    GXSetTexCoordGen2(destination, function, source, matrix, 0, 0x7D);
}

static void GXEnd(void)
{
}

/* Stock GX immediate-mode helpers write directly to the hardware FIFO. */
static void GXTexCoord2f32(RwReal s, RwReal t)
{
    *(volatile RwReal*)0xCC008000 = s;
    *(volatile RwReal*)0xCC008000 = t;
}

static void GXPosition2s16(RwInt16 x, RwInt16 y)
{
    *(volatile RwInt16*)0xCC008000 = x;
    *(volatile RwInt16*)0xCC008000 = y;
}

static void GXPosition3f32(RwReal x, RwReal y, RwReal z)
{
    *(volatile RwReal*)0xCC008000 = x;
    *(volatile RwReal*)0xCC008000 = y;
    *(volatile RwReal*)0xCC008000 = z;
}
