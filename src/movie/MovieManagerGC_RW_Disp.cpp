#include "movie/MovieManagerGC_RW_Disp.h"

#include "dolphin/cache.h"
#include "rw/rwcore_types.h"
#include "movie/mwsfx.h"

struct RwMovieProcessCtx {
    int handle;
    int field_0x04;
    MwsFrameInfo frame;
};

static RwRaster* TargetRaster;
int gap_08_805108C4_sbss;

void MovieManager_RW_Set_Target_Raster(RwRaster* raster) {
    TargetRaster = raster;
}

void MovieManager_RW_ProcessFrame(void* context, int unused, int width, int height) {
    RwMovieProcessCtx* ctx;
    void* pixels;
    RwRaster* raster;

    ctx = (RwMovieProcessCtx*)context;
    pixels = RwRasterLock(TargetRaster, 0, 0xd);
    raster = TargetRaster;
    mwPlyFxSetOutBufPitchHeight(ctx->handle, raster->width << 2, raster->height);
    mwPlyFxCnvFrmARGB8888(ctx->handle, &ctx->frame, pixels);
    DCFlushRangeNoSync(pixels, (width * height) << 2);
    RwRasterUnlock(TargetRaster);
}

void MovieManager_RW_VSync(void) {}

void MovieManager_RW_StopVideo(void) {}

void MovieManager_RW_StartVideo(void) {}
