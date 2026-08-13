#include "libmkparticle/rw_engine.h"
#include "rw/rwcamera_internal.h"
#include "rw/batextur.h"
#include "rw/rwcolor.h"
#include "rw/rwfreelist.h"
#include "rw/rwframe.h"
#include "rw/rwgrp.h"
#include "rw/rwim3d.h"
#include "rw/rwmatrix.h"
#include "rw/rwimage.h"
#include "rw/rwplcore.h"
#include "rw/rxpipeline.h"
#include "rw/rwraster.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"
#include "rw/rwvector.h"

typedef struct RwVideoMode {
    int width;
    int height;
    int depth;
    unsigned int flags;
    int refRate;
    int format;
} RwVideoMode;

typedef struct RwSubSystemInfo {
    char name[80];
} RwSubSystemInfo;

typedef struct RwEngineOpenParams {
    void* displayID;
} RwEngineOpenParams;

extern void* memcpy(void* destination, const void* source, unsigned int size);

extern int _rwPipeAttach(void);
extern RwDevice* _rwDeviceGetHandle(void);
extern int _rwDeviceRegisterPlugin(void);
extern int _rwStringOpen(void);
extern void _rwStringClose(void);
extern int _rwFileSystemOpen(void);
extern void _rwFileSystemClose(void);
extern int _rwPluginRegistryOpen(void);
extern void _rwPluginRegistryClose(void);
extern void RwImageSetGamma(float gamma);
extern void RwErrorSet(const RwError* error);
extern int _rwerror(int errorCode, ...);

extern void* _rwErrorOpen(void*, int, int);
extern void* _rwErrorClose(void*, int, int);

static RwPluginRegistry engineTKList = { sizeof(RwGlobals), sizeof(RwGlobals),
                                         0, 0, 0, 0 };
static char onlyRenderingSubSystem[] = "Only rendering sub system";
static RwGlobals staticGlobals;
static int engineInstancesOpened;
RwGlobals* RwEngineInstance;

static int CorePluginAttach(void)
{
    int result;

    result = 0;
    result |= RwEngineRegisterPlugin(8, 0x40F, _rwErrorOpen, _rwErrorClose);
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
        return 1;
    }
    return 0;
}

static void* MallocWrapper(RwFreeList* freeList, unsigned int hint)
{
    return RwEngineInstance->fpMalloc(freeList->entrySize, hint);
}

static RwFreeList* FreeWrapper(RwFreeList* freeList, void* entry)
{
    RwEngineInstance->fpFree(entry);
    return freeList;
}

static int _rwDeviceSystemRequest(RwDevice* device, int request,
                                     void* out, void* inOut, int in)
{
    int result = device->fpSystem(request, out, inOut, in);

    if (!result) {
        switch (request) {
        case 17:
        case 18:
            result = 1;
            break;
        case 13:
            *(int*)out = 1;
            result = 1;
            break;
        case 14:
            result = in == 0;
            if (result) {
                RwSubSystemInfo* subSystemInfo = out;
                RwEngineInstance->stringFuncs.strcpy(
                    subSystemInfo->name, onlyRenderingSubSystem);
            }
            break;
        case 15:
            *(int*)out = 0;
            result = 1;
            break;
        case 16:
            result = in == 0;
            break;
        default:
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

static int EngineOpen(RwDevice* device, RwEngineOpenParams* params)
{
    RwEngineInstance =
        RwEngineInstance->fpMalloc(engineTKList.sizeOfStruct, 0x40000);
    if (RwEngineInstance != 0) {
        RwGlobals* oldGlobals = RwEngineInstance;
        memcpy(RwEngineInstance, &staticGlobals, sizeof(RwGlobals));
        _rwDeviceSystemRequest(device, 4, &RwEngineInstance->gammaCorrection,
                               &RwEngineInstance->fpMalloc, 0);
        if (_rwDeviceSystemRequest(device, 0, 0, params, 0)) {
            _rwDeviceSystemRequest(device, 11, &device->standard[0], 0, 29);
            engineInstancesOpened++;
            return 1;
        }
        RwEngineInstance = &staticGlobals;
        memcpy(&staticGlobals, oldGlobals, sizeof(RwGlobals));
        RwEngineInstance->fpFree(oldGlobals);
        return 0;
    }
    RwEngineInstance = &staticGlobals;
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, engineTKList.sizeOfStruct);
        RwErrorSet(&error);
    }
    return 0;
}

int _rwGetNumEngineInstances(void)
{
    return engineInstancesOpened;
}

int RwEngineGetVersion(void)
{
    return 0x36003;
}

int RwEngineRegisterPlugin(int size, unsigned int pluginID,
                               RwPluginObjectConstructor openCB,
                               RwPluginObjectDestructor closeCB)
{
    int result;
    result = _rwPluginRegistryAddPlugin(&engineTKList, size, pluginID, openCB,
                                        closeCB, 0);
    return result;
}

int RwEngineGetPluginOffset(unsigned int pluginID)
{
    int offset;
    offset = _rwPluginRegistryGetPluginOffset(&engineTKList, pluginID);
    return offset;
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeInfo, int mode)
{
    if (!_rwDeviceSystemRequest((RwDevice*)&RwEngineInstance->gammaCorrection,
                                6, modeInfo, 0, mode)) {
        modeInfo = 0;
    }
    return modeInfo;
}

int RwEngineGetCurrentVideoMode(void)
{
    int mode;
    if (_rwDeviceSystemRequest((RwDevice*)&RwEngineInstance->gammaCorrection,
                               10, &mode, 0, 0)) {
        return mode;
    }
    return -1;
}

int RwEngineStart(void)
{
    RwDevice* device = (RwDevice*)&RwEngineInstance->gammaCorrection;
    if (_rwDeviceSystemRequest(device, 2, 0, 0, 0)) {
        if (_rwPluginRegistryInitObject(&engineTKList, RwEngineInstance)) {
            RwImageSetGamma(RwEngineInstance->gammaCorrection);
            _rwDeviceSystemRequest(device, 17, 0, 0, 0);
            RwEngineInstance->engineStatus = 3;
            return 1;
        }
        _rwDeviceSystemRequest(device, 3, 0, 0, 0);
    }
    return 0;
}

int RwEngineClose(void)
{


    int result;
    RwDevice* target;

    target = (RwDevice*)&RwEngineInstance->gammaCorrection;
    result = _rwDeviceSystemRequest(target, 1, 0, 0, 0);
    if (result) {
        void* instance;

        instance = RwEngineInstance;
        RwEngineInstance = &staticGlobals;
        memcpy(RwEngineInstance, instance, sizeof(RwGlobals));
        RwEngineInstance->fpFree(instance);
        engineInstancesOpened--;
        RwEngineInstance->engineStatus = 1;
    }
    return result;
}

int RwEngineOpen(RwEngineOpenParams* params)
{
    int result;
    if (RwEngineInstance == 0) {
        RwEngineInstance = &staticGlobals;
    }
    result = RwEngineInstance->engineStatus == 1;
    if (result) {
        result = params != 0;
        if (result) {
            RwDevice* device = _rwDeviceGetHandle();
            result = device != 0;
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

int RwEngineTerm(void)
{
    int result = engineInstancesOpened == 0;
    if (result) {
        _rwPluginRegistryClose();
        _rwFileSystemClose();
        _rwMemoryClose();
        RwEngineInstance->engineStatus = 0;
    }
    return result;
}

int RwEngineInit(const RwMemoryFunctions* memoryFunctions,
                    unsigned int initFlags, unsigned int resArenaSize)
{
    int result = 0;
    RwEngineInstance = &staticGlobals;
    if (initFlags & 1) {
        RwEngineInstance->fpFreeListAlloc = MallocWrapper;
        RwEngineInstance->fpFreeListFree = FreeWrapper;
        _rwFreeListEnable(0);
    } else {
        RwEngineInstance->fpFreeListAlloc = _rwFreeListAllocReal;
        RwEngineInstance->fpFreeListFree = _rwFreeListFreeReal;
        _rwFreeListEnable(1);
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
