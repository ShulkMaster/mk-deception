#include "rw/rwcore_types.h"

typedef int (*RwRasterDeviceCall)(void* result, void* raster, int flags);
typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwFreeListFreeCall)(void* freelist, void* entry);

extern void* RwEngineInstance;
extern int rasterModule;
extern char rasterTKList[];
extern void* _rwPluginRegistryInitObject(void* registry, void* object);

static RwRasterDeviceCall raster_device_call(unsigned long offset) {
    return *(RwRasterDeviceCall*)((char*)RwEngineInstance + offset);
}

#pragma optimization_level 4
RwRaster* RwRasterUnlock(RwRaster* raster) {
    raster_device_call(0x88)(0, raster, 0);
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

    freelist = *(void**)((char*)RwEngineInstance + rasterModule + 0x60);
    raster = (RwRaster*)
        (*(RwFreeListAllocCall*)((char*)RwEngineInstance + 0x144))(freelist, 0x30407);
    if (raster == 0) {
        return 0;
    }

    create_call = *(RwRasterDeviceCall*)((char*)RwEngineInstance + 0x58);
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
        freelist = *(void**)((char*)RwEngineInstance + rasterModule + 0x60);
        (*(RwFreeListFreeCall*)((char*)RwEngineInstance + 0x148))(freelist, raster);
        return 0;
    }

    _rwPluginRegistryInitObject(rasterTKList, raster);
    return raster;
}

void* RwRasterLock(RwRaster* raster, unsigned char level, int flags) {
    void* pixels;

    if (raster_device_call(0x84)(&pixels, raster, flags + ((unsigned int)level << 8)) == 0) {
        pixels = 0;
    }
    return pixels;
}
