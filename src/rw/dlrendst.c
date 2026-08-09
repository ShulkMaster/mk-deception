#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwcore_types.h"

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
static RwDlStateCache _RwDlStateCache;

RwTexture* _RwDlTexture;
RwRaster* _RwDlRasterWhite;

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern void GXSetCurrentMtx(RwUInt32 matrix);
extern void _rwDlTextureCacheInit(void);
extern void _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster,
                                  RwBool releaseRaster);

static RwBool _rwDlRenderStateFogEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateFogColor(RwUInt32 color);
static RwBool _rwDlRenderStateFogType(RwInt32 type);
static RwBool _rwDlRenderStateFogDensity(RwReal density);
static RwBool _rwDlRenderStateTextureAddress(RwInt32 address);
static RwBool _rwDlRenderStateTextureAddressU(RwInt32 address);
static RwBool _rwDlRenderStateTextureAddressV(RwInt32 address);
static RwBool _rwDlRenderStateTextureFilter(RwUInt32 filter);
static RwBool _rwDlRenderStateTextureRaster(RwRaster* raster);
static void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture);
static RwBool _rwDlRenderStateZWriteEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateZTestEnable(RwUInt32 enable);
static RwBool _rwDlRenderStateSrcBlend(RwInt32 blend);
static RwBool _rwDlRenderStateDstBlend(RwInt32 blend);
static RwBool _rwDlRenderStateCullMode(RwInt32 cullMode);
static RwBool rwDlRenderStateAlphaTestFunction(RwInt32 function);
static RwBool rwDlRenderStateAlphaTestFunctionRef(RwInt32 reference);

void _rwDlRenderStateOpen(void)
{
    /*
     * Retail retains two unused texture-macro address values after creation.
     * The initialized cache, GX calls, raster ownership, and store order below
     * otherwise match; keeping those dead values would only force registers.
     */
    GXColor white = {0xFF, 0xFF, 0xFF, 0xFF};
    void* pixels;

    _RwDlStateCache.fogEnable = FALSE;
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
    _RwDlStateCache.zWriteEnable = TRUE;
    _RwDlStateCache.zTestEnable = TRUE;
    _RwDlStateCache.zCompLoc = TRUE;
    _RwDlStateCache.srcBlend = 5;
    _RwDlStateCache.dstBlend = 6;
    _RwDlStateCache.cullMode = 2;
    _RwDlStateCache.zCompare = 3;

    GXSetZMode(TRUE, 3, TRUE);
    GXSetZCompLoc(TRUE);

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
    GXSetColorUpdate(TRUE);
    GXSetAlphaUpdate(TRUE);
    GXSetCopyClear(white, 0xFFFFFF);
    GXSetCurrentMtx(0);

    _rwDlTextureCacheInit();
    _RwDlTexture = RwTextureCreate(NULL);
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
    _RwDlRasterWhite = NULL;
    RwTextureSetRaster(_RwDlTexture, NULL);
    RwTextureDestroy(_RwDlTexture);
    _RwDlTexture = NULL;
}

RwBool _rwDlGetRenderState(RwInt32 state, void* value)
{
    /*
     * The retail switch has these same cases, result widths, and texture/cache
     * accesses. Its O0 label layout changes both generated jump-table targets,
     * so the remaining broad diff is source lowering rather than missing state.
     */
    switch (state) {
    case 1:
        *(RwRaster**)value = _RwDlTexture->raster;
        return TRUE;
    case 2:
        if (((_RwDlTexture->filter_flags >> 8) & 0xF) ==
            ((_RwDlTexture->filter_flags >> 12) & 0xF)) {
            *(RwInt32*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
            return TRUE;
        }
        return FALSE;
    case 3:
        *(RwInt32*)value = (_RwDlTexture->filter_flags >> 8) & 0xF;
        return TRUE;
    case 4:
        *(RwInt32*)value = (_RwDlTexture->filter_flags >> 12) & 0xF;
        return TRUE;
    case 5:
        *(RwInt32*)value = TRUE;
        return TRUE;
    case 6:
        *(RwInt32*)value = _RwDlStateCache.zTestEnable;
        return TRUE;
    case 7:
        *(RwInt32*)value = 2;
        return TRUE;
    case 8:
        *(RwInt32*)value = _RwDlStateCache.zWriteEnable;
        return TRUE;
    case 9:
        *(RwInt32*)value = (RwUInt8)_RwDlTexture->filter_flags;
        return TRUE;
    case 10:
        *(RwInt32*)value = _RwDlStateCache.srcBlend;
        return TRUE;
    case 11:
        *(RwInt32*)value = _RwDlStateCache.dstBlend;
        return TRUE;
    case 14:
        *(RwInt32*)value = _RwDlStateCache.fogEnable;
        return TRUE;
    case 15:
        *(RwUInt32*)value = _RwDlStateCache.fogColor;
        return TRUE;
    case 16:
        *(RwInt32*)value = _RwDlStateCache.fogType;
        return TRUE;
    case 17:
        return FALSE;
    case 20:
        *(RwInt32*)value = _RwDlStateCache.cullMode;
        return TRUE;
    case 29:
        if (_RwDlStateCache.alphaOperation == 0 &&
            _RwDlStateCache.alphaCompare1 == 7) {
            *(RwInt32*)value = _RwDlStateCache.alphaCompare0 + 1;
        } else {
            *(RwInt32*)value = 0;
        }
        return TRUE;
    case 30:
        *(RwInt32*)value = _RwDlStateCache.alphaRef0;
        return TRUE;
    default:
        return FALSE;
    }
}

static RwBool _rwDlRenderStateFogEnable(RwUInt32 enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.fogEnable == FALSE) {
            RwCamera* camera = (RwCamera*)RwEngineInstance->curCamera;
            if (camera != NULL) {
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
            _RwDlStateCache.fogEnable = TRUE;
        }
    } else if (_RwDlStateCache.fogEnable != FALSE) {
        GXSetFog(0, 5.0f, 10.0f, 0.05f, 10.0f,
                 _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogEnable = FALSE;
    }
    return TRUE;
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
    return TRUE;
}

static RwBool _rwDlRenderStateFogType(RwInt32 type)
{
    if (type != _RwDlStateCache.fogType) {
        RwCamera* camera;
        if (type != 1)
            return FALSE;
        camera = (RwCamera*)RwEngineInstance->curCamera;
        GXSetFog(_RwDlFogConvTable[type], camera->fogPlane,
                 camera->farPlane, camera->nearPlane, camera->farPlane,
                 _RwDlStateCache.gxFogColor);
        _RwDlStateCache.fogType = type;
    }
    return TRUE;
}

static RwBool _rwDlRenderStateFogDensity(RwReal density)
{
    return FALSE;
}

static RwBool _rwDlRenderStateTextureAddress(RwInt32 address)
{
    /* Stock texture macros retain an unused _RwDlTexture address in retail. */
    if (address == 4)
        return FALSE;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF00FF) |
        ((address << 8) & 0xF00) | ((address << 12) & 0xF000);
    return TRUE;
}

static RwBool _rwDlRenderStateTextureAddressU(RwInt32 address)
{
    /* Stock texture macros retain an unused _RwDlTexture address in retail. */
    if (address == 4)
        return FALSE;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFF0FF) |
        ((address << 8) & 0xF00);
    return TRUE;
}

static RwBool _rwDlRenderStateTextureAddressV(RwInt32 address)
{
    /* Stock texture macros retain an unused _RwDlTexture address in retail. */
    if (address == 4)
        return FALSE;
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFF0FFF) |
        ((address << 12) & 0xF000);
    return TRUE;
}

static RwBool _rwDlRenderStateTextureFilter(RwUInt32 filter)
{
    /* Stock texture macros retain an unused _RwDlTexture address in retail. */
    _RwDlTexture->filter_flags =
        (_RwDlTexture->filter_flags & 0xFFFFFF00) | (RwUInt8)filter;
    return TRUE;
}

static RwBool _rwDlRenderStateTextureRaster(RwRaster* raster)
{
    if (raster != _RwDlTexture->raster)
        _rwDlTextureSetRaster(_RwDlTexture, raster, FALSE);
    return TRUE;
}

static void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture)
{
    if (_RwDlStateCache.zCompLoc != beforeTexture) {
        if (beforeTexture == TRUE) {
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
    if (_RwDlTexture->raster != NULL) {
        RwGameCubeRasterExt* rasterExt;
        RwBool beforeTexture;
        _rwDlTextureSet(_RwDlTexture, 0);
        rasterExt = RW_GAMECUBE_RASTER_EXTENSION(_RwDlTexture->raster);
        beforeTexture = (rasterExt->hasAlpha & 1) == 0;
        _rwDlRenderStateSetZCompLoc(beforeTexture);
    }
}

static RwBool _rwDlRenderStateZWriteEnable(RwUInt32 enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.zWriteEnable == FALSE) {
            GXSetZMode(TRUE, _RwDlStateCache.zCompare, TRUE);
            _RwDlStateCache.zWriteEnable = TRUE;
        }
    } else if (_RwDlStateCache.zWriteEnable != FALSE) {
        GXSetZMode(TRUE, _RwDlStateCache.zCompare, FALSE);
        _RwDlStateCache.zWriteEnable = FALSE;
    }
    return TRUE;
}

static RwBool _rwDlRenderStateZTestEnable(RwUInt32 enable)
{
    if (enable != 0) {
        if (_RwDlStateCache.zTestEnable == FALSE) {
            GXSetZMode(TRUE, 3, (RwUInt8)_RwDlStateCache.zWriteEnable);
            _RwDlStateCache.zCompare = 3;
            _RwDlStateCache.zTestEnable = TRUE;
        }
    } else if (_RwDlStateCache.zTestEnable != FALSE) {
        GXSetZMode(TRUE, 7, (RwUInt8)_RwDlStateCache.zWriteEnable);
        _RwDlStateCache.zCompare = 7;
        _RwDlStateCache.zTestEnable = FALSE;
    }
    return TRUE;
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
            return FALSE;
        }
        GXSetBlendMode(1, _RwDlBlendConvTable[blend],
                       _RwDlBlendConvTable[_RwDlStateCache.dstBlend], 0);
        _RwDlStateCache.srcBlend = blend;
    }
    return TRUE;
}

static RwBool _rwDlRenderStateDstBlend(RwInt32 blend)
{
    if (blend != _RwDlStateCache.dstBlend) {
        if (blend < 1 || blend >= 9)
            return FALSE;
        GXSetBlendMode(1, _RwDlBlendConvTable[_RwDlStateCache.srcBlend],
                       _RwDlBlendConvTable[blend], 0);
        _RwDlStateCache.dstBlend = blend;
    }
    return TRUE;
}

static RwBool _rwDlRenderStateCullMode(RwInt32 cullMode)
{
    if (cullMode != _RwDlStateCache.cullMode) {
        GXSetCullMode(cullMode - 1);
        _RwDlStateCache.cullMode = cullMode;
    }
    return TRUE;
}

static RwBool rwDlRenderStateAlphaTestFunction(RwInt32 function)
{
    if (_RwDlStateCache.alphaMode != 0) {
        if (_RwDlStateCache.zCompLoc != TRUE)
            GXSetAlphaCompare(function - 1, _RwDlStateCache.alphaRef0,
                              0, 7, 0);
        _RwDlStateCache.alphaCompare0 = function - 1;
        _RwDlStateCache.alphaOperation = 0;
        _RwDlStateCache.alphaCompare1 = 7;
        _RwDlStateCache.alphaRef1 = 0;
    } else if (function - 1 != _RwDlStateCache.alphaCompare0) {
        if (_RwDlStateCache.zCompLoc != TRUE) {
            GXSetAlphaCompare(
                function - 1, _RwDlStateCache.alphaRef0,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        _RwDlStateCache.alphaCompare0 = function - 1;
    }
    return TRUE;
}

static RwBool rwDlRenderStateAlphaTestFunctionRef(RwInt32 reference)
{
    if (reference != _RwDlStateCache.alphaRef0) {
        if (_RwDlStateCache.zCompLoc != TRUE) {
            GXSetAlphaCompare(
                _RwDlStateCache.alphaCompare0, (RwUInt8)reference,
                _RwDlStateCache.alphaOperation,
                _RwDlStateCache.alphaCompare1,
                _RwDlStateCache.alphaRef1);
        }
        _RwDlStateCache.alphaRef0 = (RwUInt8)reference;
    }
    return TRUE;
}

RwBool _rwDlSetRenderState(RwInt32 state, void* value)
{
    /*
     * Retail dispatches the same complete state set. O0 save selection and
     * label placement shift both switch tables; no synthetic locals are kept
     * solely to reproduce those addresses.
     */
    RwBool result = FALSE;

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
        result = (RwInt32)value == 2;
        break;
    case 6:
        result = _rwDlRenderStateZTestEnable((RwUInt32)value);
        break;
    case 7:
        result = (RwBool)value;
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
        result = FALSE;
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
