#include "libmkparticle/rw_engine.h"

enum {
    rwRASTERGAMMACORRECTED = 0x01,
    rwIMAGEGAMMACORRECTED = 0x02
};

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    if (RwEngineInstance->fpImageSetFromRaster(image, raster, 0)) {
        if ((raster->privateFlags & rwRASTERGAMMACORRECTED) != 0) {
            image->flags |= rwIMAGEGAMMACORRECTED;
        }
        return image;
    }
    return NULL;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) {
    if (RwEngineInstance->fpRasterSetFromImage(raster, image, 0)) {
        if ((image->flags & rwIMAGEGAMMACORRECTED) != 0) {
            raster->privateFlags |= rwRASTERGAMMACORRECTED;
        }
        return raster;
    }
    return NULL;
}

RwImage* RwImageFindRasterFormat(RwImage* image, RwInt32 rasterType,
                                 RwInt32* width, RwInt32* height,
                                 RwInt32* depth, RwInt32* format) {
    RwRaster raster;

    if (!RwEngineInstance->fpImageFindRasterFormat(&raster, image,
                                                    rasterType)) {
        return NULL;
    }
    *format = (((RwUInt32)raster.format & 0xFF) << 8) | raster.type;
    *width = raster.width;
    *height = raster.height;
    *depth = raster.depth;
    return image;
}
