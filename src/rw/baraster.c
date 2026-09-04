#include "rw/rwcore_types.h"
#include "runtime/cstring.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rwraster.h"
#include "rw/rwengine.h"

typedef struct RwRasterModuleGlobals {
    RwRaster* currentRaster;
    char pad04[0x24];
    int field_0x28;
    void* field_0x2c;
    void* field_0x30;
    void* field_0x34;
    void* field_0x38;
    void* field_0x3c;
    void* field_0x40;
    char pad44[8];
    unsigned char field_0x4c;
    unsigned char field_0x4d;
    char pad4e[0x12];
    RwFreeList* freelist;
} RwRasterModuleGlobals;

extern void _rwResourcesPurge(void);
static RwPluginRegistry rasterTKList = {
    sizeof(RwRaster), sizeof(RwRaster), 0, 0, 0, 0
};
static RwFreeList _rwRasterFreeList;
static int _rwRasterFreeListBlockSize = 0x80;
static int _rwRasterFreeListPreallocBlocks = 1;
static RwModuleInfo rasterModule;

static RwRasterModuleGlobals* RasterGlobals(void)
{
    return (RwRasterModuleGlobals*)((char*)RwEngineInstance +
                                    rasterModule.globalsOffset);
}

#pragma optimization_level 4
RwRaster* RwRasterUnlock(RwRaster* raster) {
    RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERUNLOCK)(0, raster, 0);
    return raster;
}
#pragma optimization_level 0

RwRaster* RwRasterUnlockPalette(RwRaster* raster) {
    RwRasterDeviceCall unlockPalette = RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERUNLOCKPALETTE);
    unlockPalette(0, raster, 0);
    raster->privateFlags &= ~0x18;
    return raster;
}

int RwRasterDestroy(RwRaster* raster) {
    _rwPluginRegistryDeInitObject(&rasterTKList, raster);
    RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERDESTROY)(0, raster, 0);
    RwEngineInstance->fpFreeListFree(RasterGlobals()->freelist, raster);
    return 1;
}

int RwRasterRegisterPlugin(int size, unsigned int pluginID,
                               RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB,
                               RwPluginObjectCopy copyCB) {
    int offset;
    offset = _rwPluginRegistryAddPlugin(&rasterTKList, size, pluginID, constructCB, destructCB,
                                        copyCB);
    return offset;
}

void* RwRasterLockPalette(RwRaster* raster, int flags) {
    unsigned char* palette;
    if (RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERLOCKPALETTE)(&palette, raster, flags) != 0) {
        return palette;
    }
    return 0;
}

int RwRasterGetNumLevels(RwRaster* raster) {
    int levels;
    RwRasterDeviceCall getNumLevels;
    if ((int)(((unsigned int)(unsigned char)raster->format << 8) & 0x8000) == 0) {
        return 1;
    }
    getNumLevels = RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERGETMIPLEVELS);
    if (getNumLevels(&levels, raster, 0) != 0) {
        return levels;
    }
    return -1;
}

RwRaster* RwRasterShowRaster(RwRaster* raster, void* device, unsigned int flags) {
    RwRasterDeviceCall showRaster = RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERSHOWRASTER);
    _rwResourcesPurge();
    if (showRaster(raster, device, flags) != 0) {
        return raster;
    }
    return 0;
}

RwRaster* RwRasterSubRaster(RwRaster* raster, RwRaster* parent, RwRect* rect) {
    if ((raster->flags & 0x80) == 0) {
        return 0;
    }
    raster->width = rect->w;
    raster->height = rect->h;
    raster->offsetX = parent->offsetX + (short)rect->x;
    raster->offsetY = parent->offsetY + (short)rect->y;
    if (RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERSUBRASTER)(raster, parent, 0) != 0) {
        raster->parent = parent->parent;
        return raster;
    }
    return 0;
}

RwRaster* RwRasterCreate(int width, int height, int depth, int flags) {
    RwRaster* raster;
    RwRasterDeviceCall createRaster;

    raster = (RwRaster*)RwEngineInstance->fpFreeListAlloc(RasterGlobals()->freelist, 0x30407);
    if (raster != 0) {
        createRaster = RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERCREATE);
        raster->privateFlags = 0;
        raster->flags = 0;
        raster->width = width;
        raster->height = height;
        raster->offsetX = 0;
        raster->offsetY = 0;
        raster->depth = depth;
        raster->parent = raster;
        raster->pixels = 0;
        raster->palette = 0;

        if (createRaster(0, raster, flags) == 0) {
            RwEngineInstance->fpFreeListFree(RasterGlobals()->freelist, raster);
            return 0;
        }

        _rwPluginRegistryInitObject(&rasterTKList, raster);
        return raster;
    }
    return 0;
}

void* RwRasterLock(RwRaster* raster, unsigned char level, int flags) {
    unsigned char* pixels;
    if (RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERLOCK)(&pixels, raster, flags + ((unsigned int)level << 8)) != 0) {
        return pixels;
    }
    return 0;
}

void* _rwRasterClose(void* instance, int offset, int size) {
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        rasterModule.globalsOffset + 0x60) != 0) {
        RwFreeListDestroy(*(RwFreeList**)((unsigned char*)RwEngineInstance +
                                          rasterModule.globalsOffset + 0x60));
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        rasterModule.globalsOffset + 0x60) = 0;
    }
    rasterModule.numInstances--;
    return instance;
}

void* _rwRasterOpen(void* instance, int offset, int size) {
    rasterModule.globalsOffset = offset;
    memset(&((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                                      rasterModule.globalsOffset))->field_0x2c,
           0, 0x34);
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x38 = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x3c = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x40 = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x4d = 0x80;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x30 = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x34 = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x4c = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->field_0x28 = 0;
    ((RwRasterModuleGlobals*)((unsigned char*)RwEngineInstance +
                              rasterModule.globalsOffset))->currentRaster =
        (RwRaster*)((unsigned char*)RwEngineInstance +
                    rasterModule.globalsOffset + 0x2c);
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    rasterModule.globalsOffset + 0x60) =
        RwFreeListCreateAndPreallocateSpace(
        rasterTKList.sizeOfStruct, _rwRasterFreeListBlockSize, 4,
        _rwRasterFreeListPreallocBlocks, &_rwRasterFreeList, 0x40407);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        rasterModule.globalsOffset + 0x60) == 0) {
        return 0;
    }
    rasterModule.numInstances++;
    return instance;
}
