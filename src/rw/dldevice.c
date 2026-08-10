#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rxpipeline.h"

typedef struct RwVideoMode {
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 flags;
    RwInt32 refreshRate;
    RwInt32 format;
} RwVideoMode;

typedef struct RwDlOpenParams {
    GXRenderModeObj* renderMode;
    RwInt32 pixelFormat;
    RwUInt32 fifoSize;
} RwDlOpenParams;

typedef struct RwGCFrameQueueEntry {
    void* xfb;
    void* breakPoint;
} RwGCFrameQueueEntry;

typedef struct OSThreadQueue {
    void* head;
    void* tail;
} OSThreadQueue;

typedef struct GXFifoObj {
    RwUInt8 data[0x80];
} GXFifoObj;

struct RwCamera {
    RwObjectHasFrame object;
    RwInt32 projectionType;
    void* beginUpdate;
    void* endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
};

typedef struct RwDlGlobals {
    RwCamera* camera;
    void* deviceGlobals;
} RwDlGlobals;

typedef struct RwDlDevice {
    RwReal gammaCorrection;
    RwSystemFunc system;
    RwReal zBufferNear;
    RwReal zBufferFar;
    RwBool (*renderStateSet)(RwInt32 state, void* value);
    RwBool (*renderStateGet)(RwInt32 state, void* value);
    void* im2DRenderLine;
    void* im2DRenderTriangle;
    void* im2DRenderPrimitive;
    void* im2DRenderIndexedPrimitive;
    void* reserved[4];
} RwDlDevice;

typedef struct RwStandardEntry {
    RwInt32 index;
    RwStandardFunc function;
} RwStandardEntry;

static void _rwDlBreakNext(void);
static void _rwDlBreakPtCallback(void);
static void _rwDlVIPreRetraceCallback(void);
static void _rwDlVIPostRetraceCallback(void);
static RwBool _rwDlNullStandard(void* out, void* inOut, RwInt32 in);
static RwBool _rwDlDeviceSystemStandards(RwStandardFunc* standards,
                                          RwInt32 numStandards);
static void _rwDlRenderModeSelect(GXRenderModeObj* renderMode,
                                   RwInt32 pixelFormat);
static void _rwDlRenderModeInit(GXRenderModeObj* renderMode,
                                 RwInt32 pixelFormat);
static RwBool _rwDlSystem(RwInt32 option, void* out, void* inOut, RwInt32 in);
RwBool _rwDlCameraClear(void* cameraObject, RwRGBA* color,
                        RwInt32 clearMode);
RwBool _rwDlCameraBeginUpdate(void* out, void* inOut, RwInt32 in);
RwBool _rwDlCameraEndUpdate(void* out, RwCamera* camera, RwInt32 in);
RwBool _rwDlRasterShowRaster(void* out, void* inOut, RwInt32 in);
extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

extern RwBool _rwDlRasterPluginAttach(void);
extern RwBool _rwDlTexturePluginAttach(void);
extern RwBool _rwDlSetRenderState(RwInt32 state, void* value);
extern RwBool _rwDlGetRenderState(RwInt32 state, void* value);
extern RwBool _rwDlIm2DRenderLine(void*, RwInt32, RwInt32, RwInt32);
extern RwBool _rwDlIm2DRenderTriangle(void*, RwInt32, RwInt32, RwInt32,
                                      RwInt32);
extern RwBool _rwDlIm2DRenderPrimitive(RwInt32, void*, RwInt32);
extern RwBool _rwDlIm2DRenderIndexedPrimitive(RwInt32, void*, RwInt32,
                                               RwUInt16*, RwInt32);
extern RwBool _rwDlRasterCamera_ZClearRect(RwRaster*, void*, RwRGBA*,
                                            RwInt32);
extern RwUInt16 _RwDlTokenCurrent;
extern RwUInt16 _RwDlTokenLastSeen;

extern void MWY_GCN_RW_ActivateGxBreakPt(void* breakPoint, RwInt32 index);
extern void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void);
extern void MWY_GCN_RW_InsertRwGxBreakPt(void* breakPoint);
extern void MWY_GCN_RW_NoteRwGxBreakPt(void* breakPoint);
extern void MWY_GCN_RW_SetGxBreakPtCallback(void (*callback)(void));
extern RwBool _rwDlTokenQueryDone(RwUInt16 token);
extern RwBool OSDisableInterrupts(void);
extern void OSRestoreInterrupts(RwBool enabled);
extern void OSInitThreadQueue(OSThreadQueue* queue);
extern void OSSleepThread(OSThreadQueue* queue);
extern void OSWakeupThread(OSThreadQueue* queue);
extern void DCInvalidateRange(void* memory, RwUInt32 size);
extern GXFifoObj* GXGetCPUFifo(void);
extern void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr,
                          void** writePtr);
extern void GXInitFifoBase(GXFifoObj* fifo, void* base, RwUInt32 size);
extern void GXSetCPUFifo(GXFifoObj* fifo);
extern void GXSetGPFifo(GXFifoObj* fifo);
extern void GXSetCurrentGXThread(void);
extern void GXSetDrawSync(RwUInt16 token);
extern void GXSetCopyClamp(RwInt32 clamp);
extern void GXSetDispCopyGamma(RwInt32 gamma);
extern void GXInvalidateTexRegion(void* region);
extern void GXLoadNrmMtxImm(Mtx matrix, RwUInt32 id);
extern void VISetPreRetraceCallback(void (*callback)(void));
extern void VISetPostRetraceCallback(void (*callback)(void));
extern void* memcpy(void* destination, const void* source,
                    unsigned long size);
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXMpal480IntDf;

extern RwBool _rwDlRGBToPixel(void*, void*, RwInt32);
extern RwBool _rwDlPixelToRGB(void*, void*, RwInt32);
extern RwBool _rwDlRasterSetFromImage(void*, void*, RwInt32);
extern RwBool _rwDlImageGetFromRaster(void*, void*, RwInt32);
extern RwBool _rwDlRasterDestroy(void*, void*, RwInt32);
extern RwBool _rwDlRasterCreate(void*, void*, RwInt32);
extern RwBool _rwDlImageFindRasterFormat(void*, void*, RwInt32);
extern RwBool _rwDlTextureSetRaster(void*, void*, RwInt32);
extern RwBool _rwDlRasterLock(void*, void*, RwInt32);
extern RwBool _rwDlRasterUnlock(void*, void*, RwInt32);
extern RwBool _rwDlRasterLockPalette(void*, void*, RwInt32);
extern RwBool _rwDlRasterUnlockPalette(void*, void*, RwInt32);
extern RwBool _rwDlRasterClear(void*, void*, RwInt32);
extern RwBool _rwDlRasterClearRect(void*, void*, RwInt32);
extern RwBool _rwDlRasterRender(RwRaster*, void*);
extern RwBool _rwDlRasterRenderScaled(RwRaster*, void*);
extern RwBool _rwDlRasterRenderFast(RwRaster*, void*);
extern RwBool _rwDlSetRasterContext(void*, void*, RwInt32);
extern RwBool _rwDlRasterSubRaster(void*, void*, RwInt32);
extern RwBool _rwDlNativeTextureGetSize(void*, void*, RwInt32);
extern RwBool _rwDlNativeTextureWrite(void*, void*, RwInt32);
extern RwBool _rwDlNativeTextureRead(void*, void*, RwInt32);
extern RwBool _rwDlRasterGetNumMipLevels(void*, void*, RwInt32);
extern void _rwDlRenderStateOpen(void);
extern void _rwDlRenderStateClose(void);

static RwVideoMode _RwDlVideoModes[4] = {
    {640, 480, 24, 1, 0, 0},
    {640, 528, 24, 1, 0, 0},
    {640, 480, 24, 1, 0, 0},
    {0, 0, 0, 1, 0, 0},
};

RwInt32 _RwDlFSAATop = 1;
RwUInt32 _RwDlFifoSize = 0x40000;
static RwInt32 _RwDlFirstFrame = 1;
static RwInt32 _RwDlLatency = 3;
static RwUInt8 _RwDlRetraceCount = 1;
static RwUInt8 _RwDlRetraceMinCount = 1;

static RwGCFrameQueueEntry _RwGCFrameQueue[3];
static GXRenderModeObj _RwGameCubeRenderModeObj;
RwMatrix _RwDlInvCamLTM;

static RwBool _RwDlCopyClear;
RwInt32 _RwDlPixelFormat;
RwInt32 _RwDlCurPixelFormat;
RwInt32 _RwGameCubeVideoMode;
RwInt32 _RwDlFSAA;
void* _RwDl_FIFO_XFB;
void* _RwGCXFB1;
void* _RwGCXFB2;
void* _RwGCXFBCopy;
void* _RwGCXFBDisp;
static RwInt32 _RwDlFrameCurrent;
static RwInt32 _RwDlFrameNew;
static RwInt32 _RwDlFrameTokenNew;
static RwInt32 _RwDlFrameTokenCurrent;
static RwBool _RwDlBreakPointEnabled;
static RwBool _RwDlFrameReadyOnToken;
static RwBool _RwDlFrameWait;
static RwBool _RwDlFrameGo;
static OSThreadQueue _RwDlWaitingDoneRender;
static RwUInt16 _RwDlFrameSwap[3];
void* _RwDlDefaultFifo;
GXFifoObj* _RwDlDefaultFifoObj;
RwInt32 _RwDlHalfHeight;
GXRenderModeObj* _RwDlRenderMode;
RwDlGlobals dgGGlobals;

static void _rwDlBreakNext(void)
{
    static RwInt32 swap;
    RwInt32 next = (_RwDlFrameCurrent + 1) % _RwDlLatency;

    if (next == _RwDlFrameNew) {
        MWY_GCN_RW_RestartFromGxBreakPtCurrent();
        _RwDlBreakPointEnabled = 0;
    } else {
        MWY_GCN_RW_ActivateGxBreakPt(_RwGCFrameQueue[next].breakPoint,
                                     next * sizeof(RwGCFrameQueueEntry));
        MWY_GCN_RW_RestartFromGxBreakPtCurrent();
    }
    _RwDlFrameCurrent = next;
    OSWakeupThread(&_RwDlWaitingDoneRender);
    if (_RwDlFSAA == 0) {
        _RwDlFrameReadyOnToken = 1;
    } else if (swap == 1) {
        _RwDlFrameReadyOnToken = 1;
        swap = 0;
    } else {
        swap = 1;
    }
}

static void _rwDlBreakPtCallback(void)
{
    if (_RwGCFrameQueue[_RwDlFrameCurrent].xfb != _RwGCXFBDisp)
        _rwDlBreakNext();
    else
        _RwDlFrameWait = 1;
}

static void _rwDlVIPreRetraceCallback(void)
{
    RwInt16 frameToken;

    _RwDlRetraceCount++;
    frameToken = _RwDlFrameSwap[_RwDlFrameTokenCurrent];
    if (_RwDlFrameReadyOnToken == 1 &&
        _rwDlTokenQueryDone(frameToken) &&
        _RwDlRetraceCount >= _RwDlRetraceMinCount) {
        _RwDlRetraceCount = 0;
        _RwGCXFBDisp =
            _RwGCXFBDisp == _RwGCXFB1 ? _RwGCXFB2 : _RwGCXFB1;
        VISetNextFrameBuffer(_RwGCXFBDisp);
        if (_RwDlFirstFrame != 0) {
            VISetBlack(0);
            _RwDlFirstFrame = 0;
        }
        VIFlush();
        _RwDlFrameTokenCurrent =
            (_RwDlFrameTokenCurrent + 1) % _RwDlLatency;
        _RwDlFrameReadyOnToken = 0;
        _RwDlFrameGo = 1;
    }
}

static void _rwDlVIPostRetraceCallback(void)
{
    if (_RwDlFrameWait != 0 && _RwDlFrameGo != 0) {
        _rwDlBreakNext();
        _RwDlFrameWait = 0;
    }
    _RwDlFrameGo = 0;
}

static RwBool _rwDlNullStandard(void* out, void* inOut, RwInt32 in)
{
    return 0;
}

static RwBool _rwDlDeviceSystemStandards(RwStandardFunc* standards,
                                          RwInt32 numStandards)
{
    RwStandardEntry standardTable[27] = {
        {1, (RwStandardFunc)_rwDlCameraBeginUpdate},
        {10, (RwStandardFunc)_rwDlCameraEndUpdate},
        {21, (RwStandardFunc)_rwDlCameraClear},
        {20, (RwStandardFunc)_rwDlRasterShowRaster},
        {2, _rwDlRGBToPixel},
        {3, _rwDlPixelToRGB},
        {7, _rwDlRasterSetFromImage},
        {6, _rwDlImageGetFromRaster},
        {5, _rwDlRasterDestroy},
        {4, _rwDlRasterCreate},
        {9, _rwDlImageFindRasterFormat},
        {8, _rwDlTextureSetRaster},
        {15, _rwDlRasterLock},
        {16, _rwDlRasterUnlock},
        {23, _rwDlRasterLockPalette},
        {24, _rwDlRasterUnlockPalette},
        {14, _rwDlRasterClear},
        {13, _rwDlRasterClearRect},
        {17, (RwStandardFunc)_rwDlRasterRender},
        {18, (RwStandardFunc)_rwDlRasterRenderScaled},
        {19, (RwStandardFunc)_rwDlRasterRenderFast},
        {11, _rwDlSetRasterContext},
        {12, _rwDlRasterSubRaster},
        {25, _rwDlNativeTextureGetSize},
        {27, _rwDlNativeTextureWrite},
        {26, _rwDlNativeTextureRead},
        {28, _rwDlRasterGetNumMipLevels},
    };
    RwInt32 i = 0;
    RwInt32 numStandardFunctions = 27;

    while (i < numStandards) {
        standards[i] = _rwDlNullStandard;
        i++;
    }
    while (numStandardFunctions-- != 0) {



        if (standardTable->index < numStandards &&
            standardTable->index >= 0) {
            standards[standardTable[numStandardFunctions].index] =
                standardTable[numStandardFunctions].function;
        }
    }
    return 1;
}

static void _rwDlRenderModeSelect(GXRenderModeObj* renderMode,
                                   RwInt32 pixelFormat)
{
    if (renderMode != 0) {
        memcpy(&_RwGameCubeRenderModeObj, renderMode,
               sizeof(_RwGameCubeRenderModeObj));
        _RwDlRenderMode = &_RwGameCubeRenderModeObj;
        _RwGameCubeVideoMode = 42;
        _RwDlPixelFormat = _RwDlRenderMode->aa != 0 ? 2 : pixelFormat;
        _RwDlCurPixelFormat = _RwDlPixelFormat;
        _RwDlVideoModes[3].width = _RwDlRenderMode->fbWidth;
        _RwDlVideoModes[3].height = _RwDlRenderMode->efbHeight;
        switch (pixelFormat) {
        case 0:
        case 1:
            _RwDlVideoModes[3].depth = 24;
            break;
        case 2:
            _RwDlVideoModes[3].depth = 16;
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(2, "Invalid pixel format");
            RwErrorSet(&error);
            break;
        }
        }
        return;
    }

    switch (VIGetTvFormat()) {
    case 0:
        _RwDlRenderMode = &GXNtsc480IntDf;
        _RwGameCubeVideoMode = 0;
        break;
    case 1:
    case 5:
        _RwDlRenderMode = &GXPal528IntDf;
        _RwGameCubeVideoMode = 1;
        break;
    case 2:
        _RwDlRenderMode = &GXMpal480IntDf;
        _RwGameCubeVideoMode = 2;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2, "Invalid TV format");
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlRenderModeInit(GXRenderModeObj* renderMode,
                                 RwInt32 pixelFormat)
{
    _RwDlFirstFrame = 1;
}

RwBool _rwDeviceRegisterPlugin(void)
{
    _rwDlRasterPluginAttach();
    _rwDlTexturePluginAttach();
    return 1;
}

static RwDlDevice* _rwDeviceGetHandle(void);

static RwBool _rwDlSystem(RwInt32 option, void* out, void* inOut, RwInt32 in)
{
    switch (option) {
    case 7:
        return in < 1;
    default:
        return 0;
    case 5:
        *(RwInt32*)out = 1;
        return 1;
    case 6:
        if (in < 1) {
            switch (_RwGameCubeVideoMode) {
            case 0:
                *(RwVideoMode*)out = _RwDlVideoModes[0];
                break;
            case 1:
            case 5:
                *(RwVideoMode*)out = _RwDlVideoModes[1];
                break;
            case 2:
                *(RwVideoMode*)out = _RwDlVideoModes[2];
                break;
            case 42:
                *(RwVideoMode*)out = _RwDlVideoModes[3];
                break;
            }
            return 1;
        }
        return 0;
    case 10:
        *(RwInt32*)out = 0;
        return 1;
    case 8:
    case 9:
        return 1;
    case 4:
        *(RwDlDevice*)out = *_rwDeviceGetHandle();
        dgGGlobals.deviceGlobals = inOut;
        return 1;
    case 0: {
        RwDlOpenParams* params = *(RwDlOpenParams**)inOut;
        RwUInt32 xfbSize;
        if (params != 0) {
            _rwDlRenderModeSelect(params->renderMode, params->pixelFormat);
            _RwDlFifoSize = params->fifoSize;
        } else {
            _rwDlRenderModeSelect(0, _RwDlPixelFormat);
        }
        if (_RwGCXFBDisp == 0) {
            RwUInt32 width = (_RwDlRenderMode->fbWidth + 15) & ~15;
            xfbSize = width * _RwDlRenderMode->xfbHeight * 2;
            _RwDl_FIFO_XFB = RwEngineInstance->fpMalloc(
                _RwDlFifoSize + xfbSize * 2 + 31, 0x40411);
            _RwDlDefaultFifo =
                (void*)(((RwUInt32)_RwDl_FIFO_XFB + 31) & ~31U);
            DCInvalidateRange(_RwDlDefaultFifo, _RwDlFifoSize);
            _RwGCXFB1 = (RwUInt8*)_RwDlDefaultFifo + _RwDlFifoSize;
            _RwGCXFB2 = (RwUInt8*)_RwGCXFB1 + xfbSize;
            _RwGCXFBDisp = _RwGCXFB1;
            _RwGCXFBCopy = _RwGCXFB2;
        }
        return 1;
    }
    case 1:
        GXFlush();
        RwEngineInstance->fpFree(_RwDl_FIFO_XFB);
        _RwGCXFB1 = 0;
        _RwGCXFB2 = 0;
        _RwGCXFBDisp = 0;
        _RwGCXFBCopy = 0;
        _RwDlDefaultFifo = 0;
        _RwDl_FIFO_XFB = 0;
        return 1;
    case 2: {
        static RwBool gxInit;
        if (gxInit == 0) {
            _RwDlDefaultFifoObj =
                (GXFifoObj*)GXInit(_RwDlDefaultFifo, _RwDlFifoSize);
            gxInit = 1;
        } else {
            GXFifoObj fifo;
            GXInitFifoBase(&fifo, _RwDlDefaultFifo, _RwDlFifoSize);
            GXSetCPUFifo(&fifo);
            GXSetGPFifo(&fifo);
            GXInitFifoBase(_RwDlDefaultFifoObj, _RwDlDefaultFifo,
                           _RwDlFifoSize);
            GXSetCPUFifo(_RwDlDefaultFifoObj);
            GXSetGPFifo(_RwDlDefaultFifoObj);
        }
        while (GXReadDrawSync() != _RwDlTokenLastSeen)
            GXSetDrawSync(_RwDlTokenLastSeen);
        _rwDlRenderModeInit(_RwDlRenderMode, _RwDlPixelFormat);
        GXSetDispCopyGamma(0);
        _RwDlRetraceCount = _RwDlRetraceMinCount;
        VISetPreRetraceCallback(_rwDlVIPreRetraceCallback);
        VISetPostRetraceCallback(_rwDlVIPostRetraceCallback);
        MWY_GCN_RW_SetGxBreakPtCallback(_rwDlBreakPtCallback);
        OSInitThreadQueue(&_RwDlWaitingDoneRender);
        return 1;
    }
    case 18:
        _rwDlRenderStateOpen();
        return 1;
    case 19:
        _rwDlRenderStateClose();
        return 1;
    case 3:
        while (_RwDlFrameReadyOnToken == 1 ||
               _RwDlFrameTokenNew != _RwDlFrameTokenCurrent) {
        }
        return 1;
    case 11:
        return _rwDlDeviceSystemStandards(out, in);
    case 20:
        *(RwInt32*)out = 0x400;
        return 1;
    case 22:
        *(RwUInt16*)out = 3;
        return 1;
    }
}

RwBool _rwDlCameraClear(void* cameraObject, RwRGBA* color,
                        RwInt32 clearMode)
{
    RwCamera* camera = cameraObject;

    if (_RwDlCopyClear == 0) {
        RwRaster* raster = camera->frameBuffer;
        struct {
            RwInt32 x, y, w, h;
        } rectangle;
        if (_RwDlFSAA == 0) {
            rectangle.x = raster->offsetX;
            rectangle.y = raster->offsetY;
            rectangle.w = raster->width;
            rectangle.h = raster->height;
        } else {
            rectangle.x = raster->offsetX;
            rectangle.y = raster->offsetY * 2;
            rectangle.w = raster->width;
            rectangle.h = raster->height * 2;
        }
        _rwDlRasterCamera_ZClearRect(raster, &rectangle, color, clearMode);
    } else if ((clearMode & 1) != 0) {
        GXColor gxColor;
        gxColor.r = color->red;
        gxColor.g = color->green;
        gxColor.b = color->blue;
        gxColor.a = color->alpha;
        GXSetCopyClear(gxColor, 0xFFFFFF);
    }
    return 1;
}

extern RwMatrix* RwMatrixInvert(RwMatrix* destination,
                                 const RwMatrix* source);
extern RwMatrix* RwMatrixMultiply(RwMatrix* destination,
                                   const RwMatrix* first,
                                   const RwMatrix* second);

RwBool _rwDlCameraBeginUpdate(void* out, void* inOut, RwInt32 in)
{
    static RwReal projVector[7] = {0.0f, 1.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f};
    RwMatrix viewOffset;
    RwV3d* right = &viewOffset.right;
    RwV3d* up = &viewOffset.up;
    RwV3d* at = &viewOffset.at;
    RwV3d* pos = &viewOffset.pos;
    RwCamera* camera;
    RwRaster* raster;

    GXSetCurrentGXThread();
    camera = inOut;
    dgGGlobals.camera = camera;
    right->x = 1.0f;
    right->y = 0.0f;
    right->z = 0.0f;
    up->x = 0.0f;
    up->y = 1.0f;
    up->z = 0.0f;
    at->x = -camera->viewOffset.x;
    at->y = camera->viewOffset.y;
    at->z = 1.0f;
    pos->x = camera->viewOffset.x;
    pos->y = -camera->viewOffset.y;
    pos->z = 0.0f;
    viewOffset.flags = 0x20003;
    _RwDlInvCamLTM.flags = 0x20003;
    RwMatrixInvert(&_RwDlInvCamLTM,
                   RwFrameGetLTM((RwFrame*)camera->object.object.parent));
    RwMatrixTransform(&_RwDlInvCamLTM, &viewOffset,
                   2);

    projVector[1] = camera->recipViewWindow.x;
    projVector[3] = camera->recipViewWindow.y;
    if (camera->projectionType == 2) {
        projVector[0] = 1.0f;
        projVector[5] = -1.0f / (camera->farPlane - camera->nearPlane);
        projVector[6] = camera->farPlane * projVector[5];
    } else {
        projVector[0] = 0.0f;
        projVector[5] = -camera->nearPlane /
            (camera->farPlane - camera->nearPlane);
        projVector[6] = camera->farPlane * projVector[5];
    }
    GXSetProjectionv(projVector);

    raster = camera->frameBuffer;
    if ((raster->type & 5) != 0) {
        RwGameCubeRasterExt* extension =
            RwGameCubeRasterExtension(raster->parent);
        if ((extension->hasAlpha & 1) != 0) {
            if (_RwDlCurPixelFormat != 1) {
                GXSetPixelFmt(1, 0);
                _RwDlCurPixelFormat = 1;
            }
        } else if (_RwDlCurPixelFormat != 0) {
            GXSetPixelFmt(0, 0);
            _RwDlCurPixelFormat = 0;
        }
        GXSetViewport(raster->offsetX, raster->offsetY, raster->width,
                      raster->height, 0.0f, 1.0f);
        GXSetScissor(raster->offsetX, raster->offsetY, raster->width,
                     raster->height);
        GXSetScissorBoxOffset(0, 0);
    } else {
        if (_RwDlCurPixelFormat != _RwDlPixelFormat) {
            GXSetPixelFmt(_RwDlPixelFormat, 0);
            _RwDlCurPixelFormat = _RwDlPixelFormat;
        }
        if (_RwDlFSAA == 0) {
            if (_RwDlRenderMode->field_rendering != 0)
                GXSetViewportJitter(raster->offsetX, raster->offsetY,
                    raster->width, raster->height, 0.0f, 1.0f,
                    VIGetNextField() ^ 1);
            else
                GXSetViewport(raster->offsetX, raster->offsetY,
                    raster->width, raster->height, 0.0f, 1.0f);
            GXSetScissor(raster->offsetX, raster->offsetY, raster->width,
                         raster->height);
        } else {
            if (_RwDlRenderMode->field_rendering != 0)
                GXSetViewportJitter(raster->offsetX, raster->offsetY,
                    raster->width, raster->height, 0.0f, 1.0f,
                    VIGetNextField() ^ 1);
            else
                GXSetViewport(raster->offsetX, raster->offsetY * 2,
                    raster->width, raster->height * 2, 0.0f, 1.0f);
            if (_RwDlFSAATop != 0) {
                if ((raster->offsetY + raster->height) * 2 <=
                    _RwDlHalfHeight + 2)
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                        _RwDlRenderMode->fbWidth, raster->height * 2);
                else if (raster->offsetY * 2 > _RwDlHalfHeight + 2)
                    GXSetScissor(0, 0, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                else
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                        _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
                GXSetScissorBoxOffset(0, 0);
            } else {
                if (raster->offsetY * 2 >= _RwDlHalfHeight - 2)
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                        _RwDlRenderMode->fbWidth, raster->height * 2);
                else if ((raster->offsetY + raster->height) * 2 <
                         _RwDlHalfHeight + 2)
                    GXSetScissor(0, _RwDlHalfHeight - 2,
                        _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
                else
                    GXSetScissor(raster->offsetX, _RwDlHalfHeight - 2,
                        _RwDlRenderMode->fbWidth, raster->height * 2);
                GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
            }
        }
    }
    return 1;
}

RwBool _rwDlCameraEndUpdate(void* out, RwCamera* camera, RwInt32 in)
{
    dgGGlobals.camera = 0;
    return 1;
}

RwBool _rwDlRasterShowRaster(void* out, void* inOut, RwInt32 in)
{
    void* readPtr;
    void* writePtr;
    RwBool interrupts;
    RwBool queueFull;

    interrupts = OSDisableInterrupts();
    queueFull = (_RwDlFrameNew - _RwDlFrameCurrent == -1) ||
        (_RwDlLatency - 1 == _RwDlFrameNew - _RwDlFrameCurrent);
    if (queueFull)
        OSSleepThread(&_RwDlWaitingDoneRender);
    OSRestoreInterrupts(interrupts);

    if (_RwDlFSAA == 0) {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        interrupts = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].breakPoint = writePtr;
        _RwGCFrameQueue[_RwDlFrameNew].xfb = _RwGCXFBCopy;
        _RwDlFrameSwap[_RwDlFrameTokenNew] = _RwDlTokenCurrent;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        _RwDlFrameTokenNew =
            (_RwDlFrameTokenNew + 1) % _RwDlLatency;
        if (_RwDlBreakPointEnabled == 0) {
            _RwDlBreakPointEnabled = 1;
            MWY_GCN_RW_InsertRwGxBreakPt(writePtr);
        } else {
            MWY_GCN_RW_NoteRwGxBreakPt(writePtr);
        }
        OSRestoreInterrupts(interrupts);
        GXCopyDisp(_RwGCXFBCopy, (RwUInt8)_RwDlCopyClear);
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
        GXFlush();
        _RwGCXFBCopy =
            _RwGCXFBCopy == _RwGCXFB1 ? _RwGCXFB2 : _RwGCXFB1;
    } else if (_RwDlFSAATop != 0) {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        interrupts = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].breakPoint = writePtr;
        _RwGCFrameQueue[_RwDlFrameNew].xfb = _RwGCXFBCopy;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        if (_RwDlBreakPointEnabled == 0) {
            _RwDlBreakPointEnabled = 1;
            MWY_GCN_RW_InsertRwGxBreakPt(writePtr);
        } else {
            MWY_GCN_RW_NoteRwGxBreakPt(writePtr);
        }
        OSRestoreInterrupts(interrupts);
        GXCopyDisp(_RwGCXFBCopy, (RwUInt8)_RwDlCopyClear);
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
        GXFlush();
        _RwDlFSAATop = 0;
    } else {
        RwUInt32 stride;
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        interrupts = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].breakPoint = writePtr;
        _RwGCFrameQueue[_RwDlFrameNew].xfb = _RwGCXFBCopy;
        _RwDlFrameSwap[_RwDlFrameTokenNew] = _RwDlTokenCurrent;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        _RwDlFrameTokenNew =
            (_RwDlFrameTokenNew + 1) % _RwDlLatency;
        if (_RwDlBreakPointEnabled == 0) {
            _RwDlBreakPointEnabled = 1;
            MWY_GCN_RW_InsertRwGxBreakPt(writePtr);
        } else {
            MWY_GCN_RW_NoteRwGxBreakPt(writePtr);
        }
        OSRestoreInterrupts(interrupts);
        GXSetCopyClamp(2);
        GXSetDispCopySrc(0, 2, _RwDlRenderMode->fbWidth,
                         _RwDlRenderMode->efbHeight - 2);
        stride = (_RwDlRenderMode->fbWidth + 15) & ~15;
        GXCopyDisp((RwUInt8*)_RwGCXFBCopy +
                       stride * (_RwDlRenderMode->efbHeight - 2) * 2,
                   (RwUInt8)_RwDlCopyClear);
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
        GXSetCopyClamp(3);
        GXSetDispCopySrc(0, 0, _RwDlRenderMode->fbWidth,
                         _RwDlRenderMode->efbHeight);
        GXFlush();
        _RwGCXFBCopy =
            _RwGCXFBCopy == _RwGCXFB1 ? _RwGCXFB2 : _RwGCXFB1;
        _RwDlFSAATop = 1;
    }
    return 1;
}

static RwDlDevice* _rwDeviceGetHandle(void)
{
    static RwDlDevice device = {
        1.0f,
        _rwDlSystem,
        0.0f,
        0.99999994f,
        _rwDlSetRenderState,
        _rwDlGetRenderState,
        _rwDlIm2DRenderLine,
        _rwDlIm2DRenderTriangle,
        _rwDlIm2DRenderPrimitive,
        _rwDlIm2DRenderIndexedPrimitive,
        {0, 0, 0, 0},
    };
    return &device;
}

void _rwDlTransformSetup(const RwMatrix* matrix, RwBool normalize)
{
    RwMatrix combined;
    const RwMatrix* source;
    Mtx gxMatrix;

    if (matrix != 0) {
        combined.flags = 0x20003;
        RwMatrixMultiply(&combined, matrix, &_RwDlInvCamLTM);
        source = &combined;
    } else {
        source = &_RwDlInvCamLTM;
    }
    gxMatrix[0][0] = -source->right.x;
    gxMatrix[0][1] = -source->up.x;
    gxMatrix[0][2] = -source->at.x;
    gxMatrix[0][3] = -source->pos.x;
    gxMatrix[1][0] = source->right.y;
    gxMatrix[1][1] = source->up.y;
    gxMatrix[1][2] = source->at.y;
    gxMatrix[1][3] = source->pos.y;
    gxMatrix[2][0] = -source->right.z;
    gxMatrix[2][1] = -source->up.z;
    gxMatrix[2][2] = -source->at.z;
    gxMatrix[2][3] = -source->pos.z;
    GXLoadPosMtxImm(gxMatrix, 0);
    if (normalize != 0)
        GXLoadNrmMtxImm(gxMatrix, 0);
    GXSetCurrentMtx(0);
}

void RwGameCubeCameraTextureFlush(RwRaster* raster, RwBool mipmap)
{
    RwRaster* parent = raster->parent;
    RwGameCubeRasterExt* extension =
        RwGameCubeRasterExtension(parent);
    RwUInt32 offset;

    GXSetCopyFilter(0, 0, 0, 0);
    if (mipmap != 0) {
        GXSetTexCopySrc((RwUInt16)(raster->offsetX * 2),
                        (RwUInt16)(raster->offsetY * 2),
                        (RwUInt16)(raster->width * 2),
                        (RwUInt16)(raster->height * 2));
    } else {
        GXSetTexCopySrc((RwUInt16)raster->offsetX,
                        (RwUInt16)raster->offsetY,
                        (RwUInt16)raster->width,
                        (RwUInt16)raster->height);
    }
    GXSetTexCopyDst((RwUInt16)parent->width, (RwUInt16)parent->height,
                    extension->format, (RwUInt8)mipmap);



    switch (parent->depth) {
    case 4:
        offset = (raster->offsetX * 8 +
                  ((parent->width + 7) & ~7) * raster->offsetY) >> 1;
        break;
    case 8:
        offset = raster->offsetX * 4 +
            ((parent->width + 7) & ~7) * raster->offsetY;
        break;
    case 16:
        offset = (raster->offsetX * 4 +
                  ((parent->width + 3) & ~3) * raster->offsetY) * 2;
        break;
    case 32:
        offset = (raster->offsetX * 4 +
                  ((parent->width + 3) & ~3) * raster->offsetY) * 4;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        return;
    }
    }

    GXCopyTex((RwUInt8*)extension->imageData + offset,
              (RwUInt8)_RwDlCopyClear);
    GXPixModeSync();
    GXSetCopyFilter(_RwDlRenderMode->aa, _RwDlRenderMode->sample_pattern,
                    1, _RwDlRenderMode->vfilter);
    if (extension->textureRegion != 0)
        GXInvalidateTexRegion(extension->textureRegion);
    else
        GXInvalidateTexAll();
}

void RwGameCubeGetXFBs(void** displayed, void** copying)
{
    while (_RwDlFrameReadyOnToken == 1 ||
           _RwDlFrameTokenNew != _RwDlFrameTokenCurrent) {
    }
    *displayed = _RwGCXFBDisp;
    *copying = _RwGCXFBCopy;
}
