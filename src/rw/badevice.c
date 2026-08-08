#include "libmkparticle/rw_engine.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"

typedef struct RwVideoMode {
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwUInt32 flags;
    RwInt32 refRate;
    RwInt32 format;
} RwVideoMode;

typedef struct RwEngineOpenParams {
    void* displayID;
} RwEngineOpenParams;

extern void* memcpy(void* destination, const void* source, RwUInt32 size);

extern RwBool _rwPipeAttach(void);
extern RwDevice* _rwDeviceGetHandle(void);
extern RwBool _rwDeviceRegisterPlugin(void);
extern RwBool _rwStringOpen(void);
extern void _rwStringClose(void);
extern RwBool _rwFileSystemOpen(void);
extern void _rwFileSystemClose(void);
extern RwBool _rwPluginRegistryOpen(void);
extern void _rwPluginRegistryClose(void);
extern void RwImageSetGamma(RwReal gamma);
extern void RwErrorSet(const RwError* error);
extern RwInt32 _rwerror(RwInt32 errorCode, ...);

#define DECLARE_MODULE_CALLBACKS(name)                                      \
    extern void* _rw##name##Open(void*, RwInt32, RwInt32);                 \
    extern void* _rw##name##Close(void*, RwInt32, RwInt32)

DECLARE_MODULE_CALLBACKS(Error);
DECLARE_MODULE_CALLBACKS(Vector);
DECLARE_MODULE_CALLBACKS(Color);
DECLARE_MODULE_CALLBACKS(Matrix);
DECLARE_MODULE_CALLBACKS(Frame);
DECLARE_MODULE_CALLBACKS(Camera);
DECLARE_MODULE_CALLBACKS(Image);
DECLARE_MODULE_CALLBACKS(Raster);
DECLARE_MODULE_CALLBACKS(Texture);
DECLARE_MODULE_CALLBACKS(RenderPipeline);
DECLARE_MODULE_CALLBACKS(ChunkGroup);
DECLARE_MODULE_CALLBACKS(Im3D);
DECLARE_MODULE_CALLBACKS(Resources);
extern void* _rwStreamModuleOpen(void*, RwInt32, RwInt32);
extern void* _rwStreamModuleClose(void*, RwInt32, RwInt32);

static RwPluginRegistry engineTKList = { sizeof(RwGlobals), sizeof(RwGlobals),
                                         0, 0, NULL, NULL };
static RwChar onlyRenderingSubSystem[] = "Only rendering sub system";
static RwGlobals staticGlobals;
static RwInt32 engineInstancesOpened;
RwGlobals* RwEngineInstance;

static RwBool CorePluginAttach(void)
{
    RwInt32 result;

    result = RwEngineRegisterPlugin(8, 0x40F, _rwErrorOpen, _rwErrorClose);
    result |= RwEngineRegisterPlugin(8, 0x401, _rwVectorOpen, _rwVectorClose);
    result |= RwEngineRegisterPlugin(0, 0x40D, _rwColorOpen, _rwColorClose);
    result |= RwEngineRegisterPlugin(0x18, 0x402, _rwMatrixOpen, _rwMatrixClose);
    result |= RwEngineRegisterPlugin(4, 0x403, _rwFrameOpen, _rwFrameClose);
    result |= RwEngineRegisterPlugin(4, 0x404, _rwStreamModuleOpen,
                                     _rwStreamModuleClose);
    result |= RwEngineRegisterPlugin(4, 0x405, _rwCameraOpen, _rwCameraClose);
    result |= RwEngineRegisterPlugin(0x220, 0x406, _rwImageOpen, _rwImageClose);
    result |= RwEngineRegisterPlugin(0x64, 0x407, _rwRasterOpen, _rwRasterClose);
    result |= RwEngineRegisterPlugin(0x34, 0x408, _rwTextureOpen, _rwTextureClose);
    result |= RwEngineRegisterPlugin(0x60, 0x409, _rwRenderPipelineOpen,
                                     _rwRenderPipelineClose);
    result |= RwEngineRegisterPlugin(4, 0x412, _rwChunkGroupOpen,
                                     _rwChunkGroupClose);
    result |= _rwPipeAttach();
    result |= RwEngineRegisterPlugin(0x74, 0x40A, _rwIm3DOpen, _rwIm3DClose);
    result |= RwEngineRegisterPlugin(0x28, 0x40B, _rwResourcesOpen,
                                     _rwResourcesClose);
    if (result >= 0) {
        return TRUE;
    }
    return FALSE;
}

static void* MallocWrapper(RwFreeList* freeList, RwUInt32 hint)
{
    return RwEngineInstance->fpMalloc(freeList->entrySize, hint);
}

static RwFreeList* FreeWrapper(RwFreeList* freeList, void* entry)
{
    RwEngineInstance->fpFree(entry);
    return freeList;
}

static RwBool _rwDeviceSystemRequest(RwDevice* device, RwInt32 request,
                                     void* out, void* inOut, RwInt32 in)
{
    RwBool result = device->fpSystem(request, out, inOut, in);

    if (!result) {
        switch (request) {
        case 13:
            *(RwInt32*)out = 1;
            result = TRUE;
            break;
        case 14:
            result = in == 0;
            if (result) {
                RwEngineInstance->stringFuncs.vecStrcpy(
                    (RwChar*)out, onlyRenderingSubSystem);
            }
            break;
        case 15:
            *(RwInt32*)out = 0;
            result = TRUE;
            break;
        case 16:
            result = in == 0;
            break;
        default:
            result = TRUE;
            break;
        }
    }
    if (!result) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x18, request);
        RwErrorSet(&error);
    }
    return result;
}

static RwBool EngineOpen(RwDevice* device, RwEngineOpenParams* params)
{
    RwEngineInstance =
        RwEngineInstance->fpMalloc(engineTKList.sizeOfStruct, 0x40000);
    if (RwEngineInstance != NULL) {
        RwGlobals* oldGlobals = RwEngineInstance;
        memcpy(RwEngineInstance, &staticGlobals, sizeof(RwGlobals));
        _rwDeviceSystemRequest(device, 4, &RwEngineInstance->gammaCorrection,
                               &RwEngineInstance->fpMalloc, 0);
        if (_rwDeviceSystemRequest(device, 0, NULL, params, 0)) {
            _rwDeviceSystemRequest(device, 11, &device->standard[0], NULL, 29);
            engineInstancesOpened++;
            return TRUE;
        }
        RwEngineInstance = &staticGlobals;
        memcpy(&staticGlobals, oldGlobals, sizeof(RwGlobals));
        RwEngineInstance->fpFree(oldGlobals);
        return FALSE;
    }
    RwEngineInstance = &staticGlobals;
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, engineTKList.sizeOfStruct);
        RwErrorSet(&error);
    }
    return FALSE;
}

RwInt32 _rwGetNumEngineInstances(void)
{
    return engineInstancesOpened;
}

RwInt32 RwEngineGetVersion(void)
{
    return 0x36003;
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                               RwPluginObjectConstructor openCB,
                               RwPluginObjectDestructor closeCB)
{
    return _rwPluginRegistryAddPlugin(&engineTKList, size, pluginID, openCB,
                                      closeCB, NULL);
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID)
{
    return _rwPluginRegistryGetPluginOffset(&engineTKList, pluginID);
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeInfo, RwInt32 mode)
{
    if (!_rwDeviceSystemRequest((RwDevice*)&RwEngineInstance->gammaCorrection,
                                6, modeInfo, NULL, mode)) {
        modeInfo = NULL;
    }
    return modeInfo;
}

RwInt32 RwEngineGetCurrentVideoMode(void)
{
    RwInt32 mode;
    if (_rwDeviceSystemRequest((RwDevice*)&RwEngineInstance->gammaCorrection,
                               10, &mode, NULL, 0)) {
        return mode;
    }
    return -1;
}

RwBool RwEngineStart(void)
{
    RwDevice* device = (RwDevice*)&RwEngineInstance->gammaCorrection;
    if (_rwDeviceSystemRequest(device, 2, NULL, NULL, 0)) {
        if (_rwPluginRegistryInitObject(&engineTKList, RwEngineInstance)) {
            RwImageSetGamma(RwEngineInstance->gammaCorrection);
            _rwDeviceSystemRequest(device, 17, NULL, NULL, 0);
            RwEngineInstance->engineStatus = 3;
            return TRUE;
        }
        _rwDeviceSystemRequest(device, 3, NULL, NULL, 0);
    }
    return FALSE;
}

RwBool RwEngineClose(void)
{
    RwBool result = _rwDeviceSystemRequest(
        (RwDevice*)&RwEngineInstance->gammaCorrection, 1, NULL, NULL, 0);
    if (result) {
        RwGlobals* globals = RwEngineInstance;
        RwEngineInstance = &staticGlobals;
        memcpy(&staticGlobals, globals, sizeof(RwGlobals));
        RwEngineInstance->fpFree(globals);
        engineInstancesOpened--;
        RwEngineInstance->engineStatus = 1;
    }
    return result;
}

RwBool RwEngineOpen(RwEngineOpenParams* params)
{
    RwBool result;
    if (RwEngineInstance == NULL) {
        RwEngineInstance = &staticGlobals;
    }
    result = RwEngineInstance->engineStatus == 1;
    if (result) {
        result = params != NULL;
        if (result) {
            RwDevice* device = _rwDeviceGetHandle();
            result = device != NULL;
            if (result) {
                result = EngineOpen(device, params);
                if (result) {
                    RwEngineInstance->engineStatus = 2;
                }
            }
        } else {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000016);
            RwErrorSet(&error);
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000001);
        RwErrorSet(&error);
    }
    return result;
}

RwBool RwEngineTerm(void)
{
    RwBool result = engineInstancesOpened == 0;
    if (result) {
        _rwPluginRegistryClose();
        _rwFileSystemClose();
        _rwMemoryClose();
        RwEngineInstance->engineStatus = 0;
    }
    return result;
}

RwBool RwEngineInit(const RwMemoryFunctions* memoryFunctions,
                    RwUInt32 initFlags, RwUInt32 resArenaSize)
{
    RwBool result = FALSE;
    RwEngineInstance = &staticGlobals;
    if (initFlags & 1) {
        RwEngineInstance->fpFreeListAlloc = MallocWrapper;
        RwEngineInstance->fpFreeListFree = FreeWrapper;
        _rwFreeListEnable(FALSE);
    } else {
        RwEngineInstance->fpFreeListAlloc = _rwFreeListAllocReal;
        RwEngineInstance->fpFreeListFree = _rwFreeListFreeReal;
        _rwFreeListEnable(TRUE);
    }
    RwEngineInstance->resArenaInitSize = resArenaSize;
    if (RwEngineInstance->engineStatus == 0) {
        result = _rwStringOpen();
        if (result) {
            result = _rwMemoryOpen(memoryFunctions);
            if (result) {
                result = _rwFileSystemOpen();
                if (result) {
                    result = _rwPluginRegistryOpen();
                    if (result) {
                        result = CorePluginAttach();
                        if (result) {
                            result = _rwDeviceRegisterPlugin();
                            if (result) {
                                RwEngineInstance->engineStatus = 1;
                                return result;
                            }
                        }
                        _rwPluginRegistryClose();
                    }
                    _rwFileSystemClose();
                }
                _rwMemoryClose();
            }
            _rwStringClose();
        }
    }
    return result;
}
