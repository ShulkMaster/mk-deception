#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/dltextur.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwcore_types.h"

static int _RwDlFogConvTable[4] = {0, 2, 4, 5};
static int _RwDlBlendConvTable[12] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 0
};
RwDlStateCache _RwDlStateCache;

RwTexture* _RwDlTexture;
RwRaster* _RwDlRasterWhite;

extern void* memset(void* destination, int value, unsigned int size);
extern void _rwDlTextureCacheInit(void);
static int _rwDlRenderStateFogColor(unsigned int color);
static int _rwDlRenderStateFogType(int type);
static int _rwDlRenderStateFogDensity(float density);
static int _rwDlRenderStateTextureAddress(int address);
static int _rwDlRenderStateTextureAddressU(int address);
static int _rwDlRenderStateTextureAddressV(int address);
static int _rwDlRenderStateTextureFilter(unsigned char filter);
static int _rwDlRenderStateTextureRaster(RwRaster* raster);
static int _rwDlRenderStateZWriteEnable(unsigned int enable);
static int _rwDlRenderStateZTestEnable(unsigned int enable);
static int _rwDlRenderStateSrcBlend(int blend);
static int _rwDlRenderStateDstBlend(int blend);
static int _rwDlRenderStateCullMode(int cullMode);
static int rwDlRenderStateAlphaTestFunction(int function);
static int rwDlRenderStateAlphaTestFunctionRef(int reference);

void _rwDlRenderStateOpen(void)
{





    GXColor white = {0xFF, 0xFF, 0xFF, 0xFF};
    void* pixels;

    _RwDlStateCache.fogEnable = 0;
    _RwDlStateCache.fogType = 1;
    _RwDlStateCache.fogColor = 0;
    _RwDlStateCache.gxFogColor.r = 0;
    _RwDlStateCache.gxFogColor.g = 0;
    _RwDlStateCache.gxFogColor.b = 0;
    _RwDlStateCache.gxFogColor.a = 0;
    _RwDlStateCache.fogStart = 5.0f;
    _RwDlStateCache.fogEnd = 10.0f;
    _RwDlStateCache.fogNear = 0.05f;
    _RwDlStateCache.fogFar = 10.0f;
    _RwDlStateCache.fogDensity = 0;
    _RwDlStateCache.zWriteEnable = 1;
    _RwDlStateCache.zTestEnable = 1;
    _RwDlStateCache.zCompLoc = 1;
    _RwDlStateCache.srcBlend = 5;
    _RwDlStateCache.dstBlend = 6;
    _RwDlStateCache.cullMode = 2;
    _RwDlStateCache.zCompare = 3;

    GXSetZMode(1, 3, 1);
    GXSetZCompLoc(1);

    _RwDlStateCache.alphaCompare0 = 4;
    _RwDlStateCache.alphaRef0 = 0;
    _RwDlStateCache.alphaOperation = 0;
    _RwDlStateCache.alphaCompare1 = 7;
    _RwDlStateCache.alphaRef1 = 0;
    _RwDlStateCache.alphaMode = 0;
    GXSetAlphaCompare(7, 0, 0, 7, 0);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetCullMode(1);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetChanCtrl(2, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(3, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(1, 0, 0, 0, 0, 0, 2);
    GXSetChanMatColor(2, white);
    GXSetChanMatColor(3, white);
    GXSetChanMatColor(0, white);
    GXSetChanMatColor(1, white);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    GXSetCopyClear(white, 0xFFFFFF);
    GXSetCurrentMtx(0);

    _rwDlTextureCacheInit();
    _RwDlTexture = RwTextureCreate(0);
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFFF00) | 2;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF00FF) | 0x1100;
    _RwDlRasterWhite = RwRasterCreate(4, 4, 16, 0x204);
    pixels = RwRasterLock(_RwDlRasterWhite, 0, 9);
    memset(pixels, 0xFF, 0x20);
    RwRasterUnlock(_RwDlRasterWhite);
}

void _rwDlRenderStateClose(void)
{
    RwRasterDestroy(_RwDlRasterWhite);
    _RwDlRasterWhite = 0;
    RwTextureSetRaster(_RwDlTexture, 0);
    RwTextureDestroy(_RwDlTexture);
    _RwDlTexture = 0;
}

int _rwDlGetRenderState(int state, void* value)
{





    switch (state) {
    case 1:
        *(RwRaster**)value = _RwDlTexture->raster;
        return 1;
    case 2:
        if (((_RwDlTexture->filter_flags >> 8) & 0xF) ==
            ((_RwDlTexture->filter_flags >> 12) & 0xF)) {
            *(int*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
            return 1;
        }
        return 0;
    case 3:
        *(int*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
        return 1;
    case 4:
        *(int*)value = (_RwDlTexture->filter_flags >> 12) & 0xF;
        return 1;
    case 5:
        *(int*)value = 1;
        return 1;
    case 6:
        *(int*)value = _RwDlStateCache.zTestEnable;
        return 1;
    case 7:
        *(int*)value = 2;
        return 1;
    case 8:
        *(int*)value = _RwDlStateCache.zWriteEnable;
        return 1;
    case 9:
        *(int*)value = (unsigned char)_RwDlTexture->filter_flags;
        return 1;
    case 10:
        *(int*)value = _RwDlStateCache.srcBlend;
        return 1;
    case 11:
        *(int*)value = _RwDlStateCache.dstBlend;
        return 1;
    case 14:
        *(int*)value = _RwDlStateCache.fogEnable;
        return 1;
    case 15:
        *(unsigned int*)value = _RwDlStateCache.fogColor;
        return 1;
    case 16:
        *(int*)value = _RwDlStateCache.fogType;
        return 1;
    case 17:
        return 0;
    case 20:
        *(int*)value = _RwDlStateCache.cullMode;
        return 1;
    case 29:
        if (_RwDlStateCache.alphaOperation == 0 &&
            _RwDlStateCache.alphaCompare1 == 7) {
            *(int*)value = _RwDlStateCache.alphaCompare0 + 1;
        } else {
            *(int*)value = 0;
        }
        return 1;
    case 30:
        *(int*)value = _RwDlStateCache.alphaRef0;
        return 1;
    default:
        return 0;
    }
}

int _rwDlRenderStateFogEnable(unsigned int enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.fogEnable == 0) {
            RwCamera* camera = (RwCamera*)RwEngineInstance->curCamera;
            if (camera != 0) {
                if (_RwDlStateCache.fogDensity == 0)
                    _RwDlStateCache.fogEnd = camera->farPlane;
                _RwDlStateCache.fogStart = camera->fogPlane;
                _RwDlStateCache.fogNear = camera->nearPlane;
                _RwDlStateCache.fogFar = camera->farPlane;
            }
            GXSetFog(_RwDlFogConvTable[_RwDlStateCache.fogType],
                     _RwDlStateCache.fogStart, _RwDlStateCache.fogEnd,
                     _RwDlStateCache.fogNear, _RwDlStateCache.fogFar,
                     _RwDlStateCache.gxFogColor);
            _RwDlStateCache.fogEnable = 1;
        }
    } else if (_RwDlStateCache.fogEnable != 0) {
        GXSetFog(0, 5.0f, 10.0f, 0.05f, 10.0f,
                 _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogEnable = 0;
    }
    return 1;
}

static int _rwDlRenderStateFogColor(unsigned int color)
{
    if (color != _RwDlStateCache.fogColor) {
        RwCamera* camera = (RwCamera*)RwEngineInstance->curCamera;
        _RwDlStateCache.gxFogColor.a = (unsigned char)(color >> 24);
        _RwDlStateCache.gxFogColor.r = (unsigned char)(color >> 16);
        _RwDlStateCache.gxFogColor.g = (unsigned char)(color >> 8);
        _RwDlStateCache.gxFogColor.b = (unsigned char)color;
        GXSetFog(_RwDlFogConvTable[_RwDlStateCache.fogType],
                 camera->fogPlane, camera->farPlane, camera->nearPlane,
                 camera->farPlane, _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogColor = color;
    }
    return 1;
}

static int _rwDlRenderStateFogType(int type)
{
    if (type != _RwDlStateCache.fogType) {
        RwCamera* camera;
        if (type != 1)
            return 0;
        camera = (RwCamera*)RwEngineInstance->curCamera;
        GXSetFog(_RwDlFogConvTable[type], camera->fogPlane,
                 camera->farPlane, camera->nearPlane, camera->farPlane,
                 _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogType = type;
    }
    return 1;
}

static int _rwDlRenderStateFogDensity(float density)
{
    return 0;
}

static int _rwDlRenderStateTextureAddress(int address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF00FF) |
        ((address << 8) & 0xF00) | ((address << 12) & 0xF000);
    return 1;
}

static int _rwDlRenderStateTextureAddressU(int address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFF0FF) |
        ((address << 8) & 0xF00);
    return 1;
}

static int _rwDlRenderStateTextureAddressV(int address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF0FFF) |
        ((address << 12) & 0xF000);
    return 1;
}

static int _rwDlRenderStateTextureFilter(unsigned char filter)
{

    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFFF00) | filter;
    return 1;
}

static int _rwDlRenderStateTextureRaster(RwRaster* raster)
{
    if (raster != _RwDlTexture->raster)
        _rwDlTextureSetRaster(_RwDlTexture, raster, 0);
    return 1;
}

void _rwDlRenderStateSetZCompLoc(int beforeTexture)
{
    if (_RwDlStateCache.zCompLoc != beforeTexture) {
        if (beforeTexture == 1) {
            GXSetAlphaCompare(7, 0, 0, 7, 0);
        } else {
            GXSetAlphaCompare(
                _RwDlStateCache.alphaCompare0,
                _RwDlStateCache.alphaRef0,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        GXSetZCompLoc((unsigned char)beforeTexture);
        _RwDlStateCache.zCompLoc = beforeTexture;
    }
}

void _rwDlTextureRasterFlush(void)
{
    if (_RwDlTexture->raster != 0) {
        RwGameCubeRasterExt* rasterExt;
        int beforeTexture;
        _rwDlTextureSet(_RwDlTexture, 0);
        rasterExt = RwGameCubeRasterExtension(_RwDlTexture->raster);
        beforeTexture = (rasterExt->hasAlpha & 1) == 0;
        _rwDlRenderStateSetZCompLoc(beforeTexture);
    }
}

static int _rwDlRenderStateZWriteEnable(unsigned int enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.zWriteEnable == 0) {
            GXSetZMode(1, _RwDlStateCache.zCompare, 1);
            _RwDlStateCache.zWriteEnable = 1;
        }
    } else if (_RwDlStateCache.zWriteEnable != 0) {
        GXSetZMode(1, _RwDlStateCache.zCompare, 0);
        _RwDlStateCache.zWriteEnable = 0;
    }
    return 1;
}

static int _rwDlRenderStateZTestEnable(unsigned int enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.zTestEnable == 0) {
            GXSetZMode(1, 3, (unsigned char)_RwDlStateCache.zWriteEnable);
            _RwDlStateCache.zCompare = 3;
            _RwDlStateCache.zTestEnable = 1;
        }
    } else if (_RwDlStateCache.zTestEnable != 0) {
        GXSetZMode(1, 7, (unsigned char)_RwDlStateCache.zWriteEnable);
        _RwDlStateCache.zCompare = 7;
        _RwDlStateCache.zTestEnable = 0;
    }
    return 1;
}

void _rwDlSetRenderStateSrcDestBlend(int source, int destination)
{
    GXSetBlendMode(1, _RwDlBlendConvTable[source],
                   _RwDlBlendConvTable[destination], 0);
}

static int _rwDlRenderStateSrcBlend(int blend)
{
    if (blend != _RwDlStateCache.srcBlend) {
        if (!((blend >= 1 && blend < 3) ||
              (blend >= 5 && blend < 11))) {
            return 0;
        }
        GXSetBlendMode(1, _RwDlBlendConvTable[blend],
                       _RwDlBlendConvTable[_RwDlStateCache.dstBlend], 0);
        _RwDlStateCache.srcBlend = blend;
    }
    return 1;
}

static int _rwDlRenderStateDstBlend(int blend)
{
    if (blend != _RwDlStateCache.dstBlend) {
        if (blend < 1 || blend >= 9)
            return 0;
        GXSetBlendMode(1, _RwDlBlendConvTable[_RwDlStateCache.srcBlend],
                       _RwDlBlendConvTable[blend], 0);
        _RwDlStateCache.dstBlend = blend;
    }
    return 1;
}

static int _rwDlRenderStateCullMode(int cullMode)
{
    if (cullMode != _RwDlStateCache.cullMode) {
        GXSetCullMode(cullMode - 1);
        _RwDlStateCache.cullMode = cullMode;
    }
    return 1;
}

static int rwDlRenderStateAlphaTestFunction(int function)
{
    if (_RwDlStateCache.alphaMode != 0) {
        if (_RwDlStateCache.zCompLoc != 1)
            GXSetAlphaCompare(function - 1, _RwDlStateCache.alphaRef0,
                              0, 7, 0);
        _RwDlStateCache.alphaCompare0 = function - 1;
        _RwDlStateCache.alphaOperation = 0;
        _RwDlStateCache.alphaCompare1 = 7;
        _RwDlStateCache.alphaRef1 = 0;
    } else if (function - 1 != _RwDlStateCache.alphaCompare0) {
        if (_RwDlStateCache.zCompLoc != 1) {
            GXSetAlphaCompare(
                function - 1, _RwDlStateCache.alphaRef0,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        _RwDlStateCache.alphaCompare0 = function - 1;
    }
    return 1;
}

static int rwDlRenderStateAlphaTestFunctionRef(int reference)
{
    if (reference != _RwDlStateCache.alphaRef0) {
        if (_RwDlStateCache.zCompLoc != 1) {
            GXSetAlphaCompare(
                _RwDlStateCache.alphaCompare0, (unsigned char)reference,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        _RwDlStateCache.alphaRef0 = (unsigned char)reference;
    }
    return 1;
}

int _rwDlSetRenderState(int state, void* value)
{





    int result = 0;

    switch (state) {
    case 1:
        result = _rwDlRenderStateTextureRaster(value);
        break;
    case 2:
        result = _rwDlRenderStateTextureAddress((int)value);
        break;
    case 3:
        result = _rwDlRenderStateTextureAddressU((int)value);
        break;
    case 4:
        result = _rwDlRenderStateTextureAddressV((int)value);
        break;
    case 5:
        result = (int)value;
        break;
    case 6:
        result = _rwDlRenderStateZTestEnable((unsigned int)value);
        break;
    case 7:
    {
        int supported;
        if ((int)value == 2)
            supported = 1;
        else
            supported = 0;
        result = supported;
        break;
    }
    case 8:
        result = _rwDlRenderStateZWriteEnable((unsigned int)value);
        break;
    case 9:
        result = _rwDlRenderStateTextureFilter((unsigned char)value);
        break;
    case 10:
        result = _rwDlRenderStateSrcBlend((int)value);
        break;
    case 11:
        result = _rwDlRenderStateDstBlend((int)value);
        break;
    case 13:
        result = 0;
        break;
    case 14:
        result = _rwDlRenderStateFogEnable((unsigned int)value);
        break;
    case 15:
        result = _rwDlRenderStateFogColor((unsigned int)value);
        break;
    case 16:
        result = _rwDlRenderStateFogType((int)value);
        break;
    case 17:
        result = _rwDlRenderStateFogDensity(*(float*)value);
        break;
    case 20:
        result = _rwDlRenderStateCullMode((int)value);
        break;
    case 29:
        result = rwDlRenderStateAlphaTestFunction((int)value);
        break;
    case 30:
        result = rwDlRenderStateAlphaTestFunctionRef((int)value);
        break;
    }
    return result;
}
