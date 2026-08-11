#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcore_types.h"

typedef struct RwDlPair {
    RwReal x;
    RwReal y;
} RwDlPair;

struct RwCamera {
    RwObjectHasFrame object;
    RwInt32 projectionType;
    void* beginUpdate;
    void* endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwDlPair viewWindow;
    RwDlPair recipViewWindow;
    RwDlPair viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
};

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

static RwInt32 _RwDlFogConvTable[4] = {0, 2, 4, 5};
static RwInt32 _RwDlBlendConvTable[12] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 0
};
RwDlStateCache _RwDlStateCache;

RwTexture* _RwDlTexture;
RwRaster* _RwDlRasterWhite;

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern void GXSetCurrentMtx(RwUInt32 matrix);
extern void _rwDlTextureCacheInit(void);
extern void _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster,
                                  RwBool releaseRaster);

RwBool _rwDlRenderStateFogEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateFogColor(RwUInt32 color);
static RwBool _rwDlRenderStateFogType(RwInt32 type);
static RwBool _rwDlRenderStateFogDensity(RwReal density);
static RwBool _rwDlRenderStateTextureAddress(RwInt32 address);
static RwBool _rwDlRenderStateTextureAddressU(RwInt32 address);
static RwBool _rwDlRenderStateTextureAddressV(RwInt32 address);
static RwBool _rwDlRenderStateTextureFilter(RwUInt32 filter);
static RwBool _rwDlRenderStateTextureRaster(RwRaster* raster);
void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture);
static RwBool _rwDlRenderStateZWriteEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateZTestEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateSrcBlend(RwInt32 blend);
static RwBool _rwDlRenderStateDstBlend(RwInt32 blend);
static RwBool _rwDlRenderStateCullMode(RwInt32 cullMode);
static RwBool rwDlRenderStateAlphaTestFunction(RwInt32 function);
static RwBool rwDlRenderStateAlphaTestFunctionRef(RwInt32 reference);

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

RwBool _rwDlGetRenderState(RwInt32 state, void* value)
{





    switch (state) {
    case 1:
        *(RwRaster**)value = _RwDlTexture->raster;
        return 1;
    case 2:
        if (((_RwDlTexture->filter_flags >> 8) & 0xF) ==
            ((_RwDlTexture->filter_flags >> 12) & 0xF)) {
            *(RwInt32*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
            return 1;
        }
        return 0;
    case 3:
        *(RwInt32*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
        return 1;
    case 4:
        *(RwInt32*)value = (_RwDlTexture->filter_flags >> 12) & 0xF;
        return 1;
    case 5:
        *(RwInt32*)value = 1;
        return 1;
    case 6:
        *(RwInt32*)value = _RwDlStateCache.zTestEnable;
        return 1;
    case 7:
        *(RwInt32*)value = 2;
        return 1;
    case 8:
        *(RwInt32*)value = _RwDlStateCache.zWriteEnable;
        return 1;
    case 9:
        *(RwInt32*)value = (RwUInt8)_RwDlTexture->filter_flags;
        return 1;
    case 10:
        *(RwInt32*)value = _RwDlStateCache.srcBlend;
        return 1;
    case 11:
        *(RwInt32*)value = _RwDlStateCache.dstBlend;
        return 1;
    case 14:
        *(RwInt32*)value = _RwDlStateCache.fogEnable;
        return 1;
    case 15:
        *(RwUInt32*)value = _RwDlStateCache.fogColor;
        return 1;
    case 16:
        *(RwInt32*)value = _RwDlStateCache.fogType;
        return 1;
    case 17:
        return 0;
    case 20:
        *(RwInt32*)value = _RwDlStateCache.cullMode;
        return 1;
    case 29:
        if (_RwDlStateCache.alphaOperation == 0 &&
            _RwDlStateCache.alphaCompare1 == 7) {
            *(RwInt32*)value = _RwDlStateCache.alphaCompare0 + 1;
        } else {
            *(RwInt32*)value = 0;
        }
        return 1;
    case 30:
        *(RwInt32*)value = _RwDlStateCache.alphaRef0;
        return 1;
    default:
        return 0;
    }
}

RwBool _rwDlRenderStateFogEnable(RwUInt32 enable)
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

static RwBool _rwDlRenderStateFogColor(RwUInt32 color)
{
    if (color != _RwDlStateCache.fogColor) {
        RwCamera* camera = (RwCamera*)RwEngineInstance->curCamera;
        _RwDlStateCache.gxFogColor.a = (RwUInt8)(color >> 24);
        _RwDlStateCache.gxFogColor.r = (RwUInt8)(color >> 16);
        _RwDlStateCache.gxFogColor.g = (RwUInt8)(color >> 8);
        _RwDlStateCache.gxFogColor.b = (RwUInt8)color;
        GXSetFog(_RwDlFogConvTable[_RwDlStateCache.fogType],
                 camera->fogPlane, camera->farPlane, camera->nearPlane,
                 camera->farPlane, _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogColor = color;
    }
    return 1;
}

static RwBool _rwDlRenderStateFogType(RwInt32 type)
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

static RwBool _rwDlRenderStateFogDensity(RwReal density)
{
    return 0;
}

static RwBool _rwDlRenderStateTextureAddress(RwInt32 address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF00FF) |
        ((address << 8) & 0xF00) | ((address << 12) & 0xF000);
    return 1;
}

static RwBool _rwDlRenderStateTextureAddressU(RwInt32 address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFF0FF) |
        ((address << 8) & 0xF00);
    return 1;
}

static RwBool _rwDlRenderStateTextureAddressV(RwInt32 address)
{

    if (address == 4)
        return 0;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF0FFF) |
        ((address << 12) & 0xF000);
    return 1;
}

static RwBool _rwDlRenderStateTextureFilter(RwUInt32 filter)
{

    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFFF00) | (RwUInt8)filter;
    return 1;
}

static RwBool _rwDlRenderStateTextureRaster(RwRaster* raster)
{
    if (raster != _RwDlTexture->raster)
        _rwDlTextureSetRaster(_RwDlTexture, raster, 0);
    return 1;
}

void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture)
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
        GXSetZCompLoc((RwUInt8)beforeTexture);
        _RwDlStateCache.zCompLoc = beforeTexture;
    }
}

void _rwDlTextureRasterFlush(void)
{
    if (_RwDlTexture->raster != 0) {
        RwGameCubeRasterExt* rasterExt;
        RwBool beforeTexture;
        _rwDlTextureSet(_RwDlTexture, 0);
        rasterExt = RwGameCubeRasterExtension(_RwDlTexture->raster);
        beforeTexture = (rasterExt->hasAlpha & 1) == 0;
        _rwDlRenderStateSetZCompLoc(beforeTexture);
    }
}

static RwBool _rwDlRenderStateZWriteEnable(RwUInt32 enable)
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

static RwBool _rwDlRenderStateZTestEnable(RwUInt32 enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.zTestEnable == 0) {
            GXSetZMode(1, 3, (RwUInt8)_RwDlStateCache.zWriteEnable);
            _RwDlStateCache.zCompare = 3;
            _RwDlStateCache.zTestEnable = 1;
        }
    } else if (_RwDlStateCache.zTestEnable != 0) {
        GXSetZMode(1, 7, (RwUInt8)_RwDlStateCache.zWriteEnable);
        _RwDlStateCache.zCompare = 7;
        _RwDlStateCache.zTestEnable = 0;
    }
    return 1;
}

void _rwDlSetRenderStateSrcDestBlend(RwInt32 source, RwInt32 destination)
{
    GXSetBlendMode(1, _RwDlBlendConvTable[source],
                   _RwDlBlendConvTable[destination], 0);
}

static RwBool _rwDlRenderStateSrcBlend(RwInt32 blend)
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

static RwBool _rwDlRenderStateDstBlend(RwInt32 blend)
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

static RwBool _rwDlRenderStateCullMode(RwInt32 cullMode)
{
    if (cullMode != _RwDlStateCache.cullMode) {
        GXSetCullMode(cullMode - 1);
        _RwDlStateCache.cullMode = cullMode;
    }
    return 1;
}

static RwBool rwDlRenderStateAlphaTestFunction(RwInt32 function)
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

static RwBool rwDlRenderStateAlphaTestFunctionRef(RwInt32 reference)
{
    if (reference != _RwDlStateCache.alphaRef0) {
        if (_RwDlStateCache.zCompLoc != 1) {
            GXSetAlphaCompare(
                _RwDlStateCache.alphaCompare0, (RwUInt8)reference,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        _RwDlStateCache.alphaRef0 = (RwUInt8)reference;
    }
    return 1;
}

RwBool _rwDlSetRenderState(RwInt32 state, void* value)
{





    RwBool result = 0;

    switch (state) {
    case 1:
        result = _rwDlRenderStateTextureRaster(value);
        break;
    case 2:
        result = _rwDlRenderStateTextureAddress((RwInt32)value);
        break;
    case 3:
        result = _rwDlRenderStateTextureAddressU((RwInt32)value);
        break;
    case 4:
        result = _rwDlRenderStateTextureAddressV((RwInt32)value);
        break;
    case 5:
        result = (RwBool)value;
        break;
    case 6:
        result = _rwDlRenderStateZTestEnable((RwUInt32)value);
        break;
    case 7:
        result = (RwInt32)value == 2;
        break;
    case 8:
        result = _rwDlRenderStateZWriteEnable((RwUInt32)value);
        break;
    case 9:
        result = _rwDlRenderStateTextureFilter((RwUInt32)value);
        break;
    case 10:
        result = _rwDlRenderStateSrcBlend((RwInt32)value);
        break;
    case 11:
        result = _rwDlRenderStateDstBlend((RwInt32)value);
        break;
    case 13:
        result = 0;
        break;
    case 14:
        result = _rwDlRenderStateFogEnable((RwUInt32)value);
        break;
    case 15:
        result = _rwDlRenderStateFogColor((RwUInt32)value);
        break;
    case 16:
        result = _rwDlRenderStateFogType((RwInt32)value);
        break;
    case 17:
        result = _rwDlRenderStateFogDensity(*(RwReal*)value);
        break;
    case 20:
        result = _rwDlRenderStateCullMode((RwInt32)value);
        break;
    case 29:
        result = rwDlRenderStateAlphaTestFunction((RwInt32)value);
        break;
    case 30:
        result = rwDlRenderStateAlphaTestFunctionRef((RwInt32)value);
        break;
    }
    return result;
}
