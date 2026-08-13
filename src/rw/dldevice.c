#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/dltoken.h"
#include "rw/gamecube_texture.h"
#include "rw/rwdevice.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"
#include "rw/rtquat.h"
#include "rw/rxpipeline.h"

typedef struct RwDlOpenParams {
    GXRenderModeObj* renderMode;
    int pixelFormat;
    unsigned int fifoSize;
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
    unsigned char data[0x80];
} GXFifoObj;

typedef struct RwDlGlobals {
    RwCamera* camera;
    void* deviceGlobals;
} RwDlGlobals;

typedef struct RwDlDevice {
    float gammaCorrection;
    RwSystemFunc system;
    float zBufferNear;
    float zBufferFar;
    int (*renderStateSet)(int state, void* value);
    int (*renderStateGet)(int state, void* value);
    void* im2DRenderLine;
    void* im2DRenderTriangle;
    void* im2DRenderPrimitive;
    void* im2DRenderIndexedPrimitive;
    void* reserved[4];
} RwDlDevice;

typedef struct RwStandardEntry {
    int index;
    RwStandardFunc function;
} RwStandardEntry;

static void _rwDlBreakNext(void);
static void _rwDlBreakPtCallback(void);
static void _rwDlVIPreRetraceCallback(void);
static void _rwDlVIPostRetraceCallback(void);
static int _rwDlNullStandard(void* out, void* inOut, int in);
static int _rwDlDeviceSystemStandards(RwStandardFunc* standards,
                                          int numStandards);
static void _rwDlRenderModeSelect(GXRenderModeObj* renderMode,
                                   int pixelFormat);
static void _rwDlRenderModeInit(GXRenderModeObj* renderMode,
                                 int pixelFormat);
static int _rwDlSystem(int option, void* out, void* inOut, int in);
int _rwDlCameraClear(void* cameraObject, RwRGBA* color,
                        int clearMode);
int _rwDlCameraBeginUpdate(void* out, void* inOut, int in);
int _rwDlCameraEndUpdate(void* out, RwCamera* camera, int in);
int _rwDlRasterShowRaster(void* out, void* inOut, int in);

extern int _rwDlRasterPluginAttach(void);
extern int _rwDlTexturePluginAttach(void);
extern int _rwDlIm2DRenderLine(void*, int, int, int);
extern int _rwDlIm2DRenderTriangle(void*, int, int, int,
                                      int);
extern int _rwDlIm2DRenderPrimitive(int, void*, int);
extern int _rwDlIm2DRenderIndexedPrimitive(int, void*, int,
                                               unsigned short*, int);
extern int _rwDlRasterCamera_ZClearRect(RwRaster*, void*, RwRGBA*,
                                            int);

extern void MWY_GCN_RW_ActivateGxBreakPt(void* breakPoint, int index);
extern void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void);
extern void MWY_GCN_RW_InsertRwGxBreakPt(void* breakPoint);
extern void MWY_GCN_RW_NoteRwGxBreakPt(void* breakPoint);
extern void MWY_GCN_RW_SetGxBreakPtCallback(void (*callback)(void));
extern int OSDisableInterrupts(void);
extern void OSRestoreInterrupts(int enabled);
extern void OSInitThreadQueue(OSThreadQueue* queue);
extern void OSSleepThread(OSThreadQueue* queue);
extern void OSWakeupThread(OSThreadQueue* queue);
extern void DCInvalidateRange(void* memory, unsigned int size);
extern GXFifoObj* GXGetCPUFifo(void);
extern void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr,
                          void** writePtr);
extern void GXInitFifoBase(GXFifoObj* fifo, void* base, unsigned int size);
extern void GXSetCPUFifo(GXFifoObj* fifo);
extern void GXSetGPFifo(GXFifoObj* fifo);
extern void GXSetCurrentGXThread(void);
extern void GXSetDrawSync(unsigned short token);
extern void GXSetCopyClamp(int clamp);
extern void GXSetDispCopyGamma(int gamma);
extern void GXInvalidateTexRegion(void* region);
extern void GXLoadNrmMtxImm(Mtx matrix, unsigned int id);
extern void VISetPreRetraceCallback(void (*callback)(void));
extern void VISetPostRetraceCallback(void (*callback)(void));
extern void* memcpy(void* destination, const void* source,
                    unsigned long size);
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXMpal480IntDf;

extern int _rwDlRGBToPixel(void*, void*, int);
extern int _rwDlPixelToRGB(void*, void*, int);
extern int _rwDlRasterSetFromImage(void*, void*, int);
extern int _rwDlImageGetFromRaster(void*, void*, int);
extern int _rwDlRasterDestroy(void*, void*, int);
extern int _rwDlRasterCreate(void*, void*, int);
extern int _rwDlImageFindRasterFormat(void*, void*, int);
extern int _rwDlRasterLock(void*, void*, int);
extern int _rwDlRasterUnlock(void*, void*, int);
extern int _rwDlRasterLockPalette(void*, void*, int);
extern int _rwDlRasterUnlockPalette(void*, void*, int);
extern int _rwDlRasterClear(void*, void*, int);
extern int _rwDlRasterClearRect(void*, void*, int);
extern int _rwDlRasterRender(RwRaster*, void*);
extern int _rwDlRasterRenderScaled(RwRaster*, void*);
extern int _rwDlRasterRenderFast(RwRaster*, void*);
extern int _rwDlSetRasterContext(void*, void*, int);
extern int _rwDlRasterSubRaster(void*, void*, int);
extern int _rwDlNativeTextureGetSize(void*, void*, int);
extern int _rwDlNativeTextureWrite(void*, void*, int);
extern int _rwDlNativeTextureRead(void*, void*, int);
extern int _rwDlRasterGetNumMipLevels(void*, void*, int);
extern void _rwDlRenderStateOpen(void);
extern void _rwDlRenderStateClose(void);

static RwVideoMode _RwDlVideoModes[4] = {
    {640, 480, 24, 1, 0, 0},
    {640, 528, 24, 1, 0, 0},
    {640, 480, 24, 1, 0, 0},
    {0, 0, 0, 1, 0, 0},
};

int _RwDlFSAATop = 1;
unsigned int _RwDlFifoSize = 0x40000;
static int _RwDlFirstFrame = 1;
static int _RwDlLatency = 3;
static unsigned char _RwDlRetraceCount = 1;
static unsigned char _RwDlRetraceMinCount = 1;

static RwGCFrameQueueEntry _RwGCFrameQueue[3];
static GXRenderModeObj _RwGameCubeRenderModeObj;
RwMatrix _RwDlInvCamLTM;

static int _RwDlCopyClear;
int _RwDlPixelFormat;
int _RwDlCurPixelFormat;
int _RwGameCubeVideoMode;
int _RwDlFSAA;
void* _RwDl_FIFO_XFB;
void* _RwGCXFB1;
void* _RwGCXFB2;
void* _RwGCXFBCopy;
void* _RwGCXFBDisp;
static int _RwDlFrameCurrent;
static int _RwDlFrameNew;
static int _RwDlFrameTokenNew;
static int _RwDlFrameTokenCurrent;
static int _RwDlBreakPointEnabled;
static int _RwDlFrameReadyOnToken;
static int _RwDlFrameWait;
static int _RwDlFrameGo;
static OSThreadQueue _RwDlWaitingDoneRender;
static unsigned short _RwDlFrameSwap[3];
void* _RwDlDefaultFifo;
GXFifoObj* _RwDlDefaultFifoObj;
int _RwDlHalfHeight;
GXRenderModeObj* _RwDlRenderMode;
RwDlGlobals dgGGlobals;

static void _rwDlBreakNext(void)
{
    static int swap;
    int next = (_RwDlFrameCurrent + 1) % _RwDlLatency;

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
    short frameToken;

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

static int _rwDlNullStandard(void* out, void* inOut, int in)
{
    return 0;
}

static int _rwDlDeviceSystemStandards(RwStandardFunc* standards,
                                          int numStandards)
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
    int i = 0;
    int numStandardFunctions = 27;

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
                                   int pixelFormat)
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
                                 int pixelFormat)
{
    _RwDlFirstFrame = 1;
}

int _rwDeviceRegisterPlugin(void)
{
    _rwDlRasterPluginAttach();
    _rwDlTexturePluginAttach();
    return 1;
}

static RwDlDevice* _rwDeviceGetHandle(void);

static int _rwDlSystem(int option, void* out, void* inOut, int in)
{
    switch (option) {
    case 7:
        return in < 1;
    default:
        return 0;
    case 5:
        *(int*)out = 1;
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
        *(int*)out = 0;
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
        unsigned int xfbSize;
        if (params != 0) {
            _rwDlRenderModeSelect(params->renderMode, params->pixelFormat);
            _RwDlFifoSize = params->fifoSize;
        } else {
            _rwDlRenderModeSelect(0, _RwDlPixelFormat);
        }
        if (_RwGCXFBDisp == 0) {
            unsigned int width = (_RwDlRenderMode->fbWidth + 15) & ~15;
            xfbSize = width * _RwDlRenderMode->xfbHeight * 2;
            _RwDl_FIFO_XFB = RwEngineInstance->fpMalloc(
                _RwDlFifoSize + xfbSize * 2 + 31, 0x40411);
            _RwDlDefaultFifo =
                (void*)(((unsigned int)_RwDl_FIFO_XFB + 31) & ~31U);
            DCInvalidateRange(_RwDlDefaultFifo, _RwDlFifoSize);
            _RwGCXFB1 = (unsigned char*)_RwDlDefaultFifo + _RwDlFifoSize;
            _RwGCXFB2 = (unsigned char*)_RwGCXFB1 + xfbSize;
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
        static int gxInit;
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
        *(int*)out = 0x400;
        return 1;
    case 22:
        *(unsigned short*)out = 3;
        return 1;
    }
}

int _rwDlCameraClear(void* cameraObject, RwRGBA* color,
                        int clearMode)
{
    RwCamera* camera = cameraObject;

    if (_RwDlCopyClear == 0) {
        RwRaster* raster = camera->frameBuffer;
        struct {
            int x, y, w, h;
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

int _rwDlCameraBeginUpdate(void* out, void* inOut, int in)
{
    static float projVector[7] = {0.0f, 1.0f, 0.0f, 1.0f,
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

int _rwDlCameraEndUpdate(void* out, RwCamera* camera, int in)
{
    dgGGlobals.camera = 0;
    return 1;
}

int _rwDlRasterShowRaster(void* out, void* inOut, int in)
{
    void* readPtr;
    void* writePtr;
    int interrupts;
    int queueFull;

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
        GXCopyDisp(_RwGCXFBCopy, (unsigned char)_RwDlCopyClear);
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
        GXCopyDisp(_RwGCXFBCopy, (unsigned char)_RwDlCopyClear);
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
        GXFlush();
        _RwDlFSAATop = 0;
    } else {
        unsigned int stride;
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
        GXCopyDisp((unsigned char*)_RwGCXFBCopy +
                       stride * (_RwDlRenderMode->efbHeight - 2) * 2,
                   (unsigned char)_RwDlCopyClear);
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

void _rwDlTransformSetup(const RwMatrix* matrix, int normalize)
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

void RwGameCubeCameraTextureFlush(RwRaster* raster, int mipmap)
{
    RwRaster* parent = raster->parent;
    RwGameCubeRasterExt* extension =
        RwGameCubeRasterExtension(parent);
    unsigned int offset;

    GXSetCopyFilter(0, 0, 0, 0);
    if (mipmap != 0) {
        GXSetTexCopySrc((unsigned short)(raster->offsetX * 2),
                        (unsigned short)(raster->offsetY * 2),
                        (unsigned short)(raster->width * 2),
                        (unsigned short)(raster->height * 2));
    } else {
        GXSetTexCopySrc((unsigned short)raster->offsetX,
                        (unsigned short)raster->offsetY,
                        (unsigned short)raster->width,
                        (unsigned short)raster->height);
    }
    GXSetTexCopyDst((unsigned short)parent->width, (unsigned short)parent->height,
                    extension->format, (unsigned char)mipmap);



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

    GXCopyTex((unsigned char*)extension->imageData + offset,
              (unsigned char)_RwDlCopyClear);
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
