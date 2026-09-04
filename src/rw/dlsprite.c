#include "dolphin/gx.h"
#include "runtime/cstring.h"
#include "dolphin/vi.h"
#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcore_types.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

RwRaster* _RwDlRasterTarget;

extern GXRenderModeObj* _RwDlRenderMode;
extern int _RwDlFSAA;
extern int _RwDlFSAATop;
extern int _RwDlHalfHeight;
extern int _RwDlPixelFormat;
extern int _RwDlCurPixelFormat;
extern RwRGBA* RwRGBASetFromPixel(RwRGBA* color, unsigned int pixel,
                                  int format);
static void GXEnd(void);
static void GXTexCoord2f32(float s, float t);
static void GXPosition2s16(short x, short y);
static void GXPosition3f32(float x, float y, float z);

static void _rwDlRasterRenderQuadInit(const RwRect* rectangle)
{

    static float projVector[7] = {
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
            int top = _RwDlRasterTarget->offsetY * 2 + rectangle->y;
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
            int boundary = _RwDlHalfHeight - 2;
            int height = rectangle->h;
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
                                  int scaled, int alpha)
{

    RwRaster* source = raster->parent;
    void* fogEnable;
    void* filterMode;
    void* addressU;
    void* addressV;
    void* oldRaster;
    float recipWidth;
    float recipHeight;

    _rwDlGetRenderState(14, &fogEnable);
    _rwDlGetRenderState(9, &filterMode);
    _rwDlGetRenderState(3, &addressU);
    _rwDlGetRenderState(4, &addressV);
    _rwDlGetRenderState(1, &oldRaster);
    GXSetCullMode(0);
    if (alpha != 0)
        _rwDlSetRenderStateSrcDestBlend(5, 6);
    else
        _rwDlSetRenderStateSrcDestBlend(2, 1);
    _rwDlSetRenderState(14, 0);
    _rwDlSetRenderState(9, (void*)(scaled != 0 ? 2 : 1));
    _rwDlSetRenderState(2, (void*)3);
    _rwDlSetRenderState(1, source);
    _rwDlTextureRasterFlush();
    _rwDlRasterRenderQuadInit(rectangle);

    recipWidth = 1.0f / source->width;
    recipHeight = 1.0f / source->height;
    GXBegin(0x80, 0, 4);
    if (scaled != 0) {
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

static int _rwDlRasterRenderGeneric(RwRaster* raster, RwRect* rectangle,
                                       int scaled, int alpha)
{

    RwRect destination = *rectangle;

    if (scaled == 0) {
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
            return 0;
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
            return 0;
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(
            2, "SRC, DST Raster render combination not supported");
        RwErrorSet(&error);
        return 0;
    }
    }
    return 1;
}

int _rwDlRasterRender(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, 0, 1);
}

int _rwDlRasterRenderFast(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, 0, 0);
}

int _rwDlRasterRenderScaled(RwRaster* raster, RwRect* rectangle)
{
    return _rwDlRasterRenderGeneric(raster, rectangle, 1, 1);
}

static void _rwDlRasterCamera_ZClearRectInit(RwRaster* raster)
{
    static float projVector[7] = {
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
            rwRasterPlatformData(raster->parent);
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
                                  const RwRGBA* color, int clearMode)
{

    GXColor material;
    int fogWasEnabled = 0;

    if ((clearMode & 1) != 0) {
        material.r = color->red;
        material.g = color->green;
        material.b = color->blue;
        material.a = color->alpha;
        if ((clearMode & 2) != 0)
            GXSetZMode(1, 7, 1);
        else
            GXSetZMode(1, 7, 0);
    } else if ((clearMode & 2) != 0) {
        material.r = 0;
        material.g = 0;
        material.b = 0;
        material.a = 0xFF;
        GXSetZMode(1, 7, 1);
        GXSetColorUpdate(0);
    } else {
        return;
    }

    GXSetChanMatColor(4, material);
    GXSetBlendMode(1, 1, 0, 0);
    GXSetCullMode(0);
    _rwDlRenderStateSetZCompLoc(1);
    if (_RwDlStateCache.fogEnable != 0) {
        _rwDlRenderStateFogEnable(0);
        fogWasEnabled = 1;
    }
    _rwDlRasterCamera_ZClearRectInit(raster);
    GXBegin(0x80, 0, 4);
    GXPosition3f32((float)rectangle->x, (float)rectangle->y, 0.99999994f);
    GXPosition3f32((float)rectangle->x,
                   (float)(rectangle->y + rectangle->h), 0.99999994f);
    GXPosition3f32((float)(rectangle->x + rectangle->w),
                   (float)(rectangle->y + rectangle->h), 0.99999994f);
    GXPosition3f32((float)(rectangle->x + rectangle->w),
                   (float)rectangle->y, 0.99999994f);
    GXEnd();

    if (fogWasEnabled != 0)
        _rwDlRenderStateFogEnable(1);
    _rwDlSetRenderStateSrcDestBlend(_RwDlStateCache.srcBlend,
                                     _RwDlStateCache.dstBlend);
    GXSetZMode(1, _RwDlStateCache.zCompare,
               (unsigned char)_RwDlStateCache.zWriteEnable);
    GXSetCullMode(_RwDlStateCache.cullMode - 1);
    GXSetColorUpdate(1);
}

static int _rwDlRasterClearGeneric(RwRaster* raster, RwRect* rectangle,
                                      int pixel)
{

    int locked = 0;
    int y;

    switch (_RwDlRasterTarget->type) {
    case 2: {
        RwRGBA color;
        RwRGBASetFromPixel(&color, pixel, _RwDlRasterTarget->format << 8);
        rectangle->x += _RwDlRasterTarget->offsetX;
        rectangle->y += _RwDlRasterTarget->offsetY;
        _rwDlRasterCamera_ZClearRect(raster, rectangle, &color, 1);
        return 1;
    }
    case 1:
        rectangle->x += _RwDlRasterTarget->offsetX;
        rectangle->y += _RwDlRasterTarget->offsetY;
        _rwDlRasterCamera_ZClearRect(raster, rectangle, 0, 2);
        return 1;
    case 0:
    case 4:
    case 5:
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        return 0;
    }
    }

    if (_RwDlRasterTarget->parent == _RwDlRasterTarget &&
        _RwDlRasterTarget->offsetX == 0 &&
        _RwDlRasterTarget->offsetY == 0 &&
        _RwDlRasterTarget->width == rectangle->w &&
        _RwDlRasterTarget->height == rectangle->h) {
        if ((_RwDlRasterTarget->privateFlags & 4) == 0) {
            locked = 1;
            RwRasterLock(_RwDlRasterTarget, 0, 9);
        }
        if (pixel == 0 || pixel == -1) {
            memset(_RwDlRasterTarget->pixels, pixel,
                   _RwDlRasterTarget->stride * _RwDlRasterTarget->height);
            if (locked != 0)
                RwRasterUnlock(_RwDlRasterTarget);
            return 1;
        }
    } else if ((_RwDlRasterTarget->privateFlags & 4) == 0) {
        locked = 1;
        RwRasterLock(_RwDlRasterTarget, 0, 3);
    }

    switch (_RwDlRasterTarget->depth) {
    case 4: {
        unsigned char value = (unsigned char)(((pixel & 0xF) << 4) | (pixel & 0xF));
        for (y = 0; y < rectangle->h; y++) {
            unsigned char* destination = _RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y) +
                (rectangle->x >> 1);
            memset(destination, value, rectangle->w >> 1);
        }
        break;
    }
    case 8:
        for (y = 0; y < rectangle->h; y++) {
            unsigned char* destination = _RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y) + rectangle->x;
            memset(destination, pixel, rectangle->w);
        }
        break;
    case 16:
        for (y = 0; y < rectangle->h; y++) {
            unsigned short* destination = (unsigned short*)(_RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y)) + rectangle->x;
            int x;
            for (x = 0; x < rectangle->w; x++)
                *destination++ = (unsigned short)pixel;
        }
        break;
    case 32:
        for (y = 0; y < rectangle->h; y++) {
            unsigned int* destination = (unsigned int*)(_RwDlRasterTarget->pixels +
                _RwDlRasterTarget->stride * (rectangle->y + y)) + rectangle->x;
            int x;
            for (x = 0; x < rectangle->w; x++)
                *destination++ = (unsigned int)pixel;
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

    if (locked != 0)
        RwRasterUnlock(_RwDlRasterTarget);
    return 1;
}

int _rwDlRasterClearRect(void* unused, RwRect* rectangle, int pixel)
{
    return _rwDlRasterClearGeneric(_RwDlRasterTarget, rectangle, pixel);
}

int _rwDlRasterClear(void* unused0, void* unused1, int pixel)
{
    RwRect rectangle;
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.w = _RwDlRasterTarget->width;
    rectangle.h = _RwDlRasterTarget->height;
    return _rwDlRasterClearGeneric(_RwDlRasterTarget, &rectangle, pixel);
}

int _rwDlSetRasterContext(void* out, void* inOut, int in)
{
    RwRaster* raster = inOut;
    _RwDlRasterTarget = raster;
    return 1;
}

static void GXEnd(void)
{
}


static void GXTexCoord2f32(float s, float t)
{
    *(volatile float*)GXFIFO_ADDR = s;
    *(volatile float*)GXFIFO_ADDR = t;
}

static void GXPosition2s16(short x, short y)
{
    *(volatile short*)GXFIFO_ADDR = x;
    *(volatile short*)GXFIFO_ADDR = y;
}

static void GXPosition3f32(float x, float y, float z)
{
    *(volatile float*)GXFIFO_ADDR = x;
    *(volatile float*)GXFIFO_ADDR = y;
    *(volatile float*)GXFIFO_ADDR = z;
}
