#include "rw/rwcore_types.h"
#include "libmkparticle/rw_engine.h"

extern int rasterModule;
extern RwPluginRegistry rasterTKList;

typedef struct RwRasterModuleGlobals {
    char pad00[0x60];
    void* freelist; /* module base +0x60 */
} RwRasterModuleGlobals;

static RwRasterDeviceCall raster_device_call(unsigned long offset) {
    return *(RwRasterDeviceCall*)((char*)RwEngineInstance + offset);
}

#pragma optimization_level 4
RwRaster* RwRasterUnlock(RwRaster* raster) {
    RwEngineInstance->fpRasterUnlock(0, raster, 0);
    return raster;
}
#pragma optimization_level 0

int RwRasterGetNumLevels(RwRaster* raster) {
    int levels;

    if ((raster->format & 0x80) == 0) {
        levels = 1;
    } else if (raster_device_call(0xB8)(&levels, raster, 0) == 0) {
        levels = -1;
    }
    return levels;
}

RwRaster* RwRasterCreate(int width, int height, int depth, int flags) {
    RwRaster* raster;
    void* freelist;
    RwRasterDeviceCall create_call;

    freelist = ((RwRasterModuleGlobals*)((char*)RwEngineInstance + rasterModule))->freelist;
    raster = (RwRaster*)
        RwEngineInstance->fpFreeListAlloc(freelist, 0x30407);
    if (raster == 0) {
        return 0;
    }

    create_call = RwEngineInstance->fpRasterCreate;
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

    if (create_call(0, raster, flags) == 0) {
        freelist = ((RwRasterModuleGlobals*)((char*)RwEngineInstance + rasterModule))->freelist;
        RwEngineInstance->fpFreeListFree(freelist, raster);
        return 0;
    }

    _rwPluginRegistryInitObject(&rasterTKList, raster);
    return raster;
}

void* RwRasterLock(RwRaster* raster, unsigned char level, int flags) {
    void* pixels;

    if (raster_device_call(0x84)(&pixels, raster, flags + ((unsigned int)level << 8)) == 0) {
        pixels = 0;
    }
    return pixels;
}
