#include "rw/rwengine.h"

enum {
    rwRASTERGAMMACORRECTED = 0x01,
    rwIMAGEGAMMACORRECTED = 0x02
};

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    if (RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDIMAGEGETRASTER)(image, raster, 0)) {
        if ((raster->privateFlags & rwRASTERGAMMACORRECTED) != 0) {
            image->flags |= rwIMAGEGAMMACORRECTED;
        }
        return image;
    }
    return 0;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) {
    if (RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDRASTERSETIMAGE)(raster, image, 0)) {
        if ((image->flags & rwIMAGEGAMMACORRECTED) != 0) {
            raster->privateFlags |= rwRASTERGAMMACORRECTED;
        }
        return raster;
    }
    return 0;
}

RwImage* RwImageFindRasterFormat(RwImage* image, int rasterType,
                                 int* width, int* height,
                                 int* depth, int* format) {
    RwRaster raster;

    if (!RWENGINESTANDARD(RwRasterDeviceCall, rwSTANDARDIMAGEFINDRASTERFORMAT)(&raster, image,
                                                    rasterType)) {
        return 0;
    }
    *format = (((unsigned int)raster.format & 0xFF) << 8) | raster.type;
    *width = raster.width;
    *height = raster.height;
    *depth = raster.depth;
    return image;
}
