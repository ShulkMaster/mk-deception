#include "movie/MovieManagerGC_RW_Disp.h"

#include "dolphin/cache.h"
#include "rw/rwcore_types.h"
#include "movie/mwsfx.h"

typedef struct RwMovieProcessCtx {
    int handle;
    int field_0x04;
    MwsFrameInfo frame; /* +0x08 -- source frame descriptor */
} RwMovieProcessCtx;

/* MWCC emits .sbss in reverse declaration order. */
int gap_08_805108C4_sbss;
RwRaster* TargetRaster;

void MovieManager_RW_Set_Target_Raster(RwRaster* raster) {
    TargetRaster = raster;
}

/* Soft ceiling: MovieManager_RW_ProcessFrame ~72% -- retail stmw/lmw scheduling;
 * enabling the required flag crashes MWCC when small-data placement is active. */
void MovieManager_RW_ProcessFrame(void* context, int unused, int width, int height) {
    RwMovieProcessCtx* ctx;
    void* pixels;
    RwRaster* raster;

    (void)unused;
    ctx = (RwMovieProcessCtx*)context;
    pixels = RwRasterLock(TargetRaster, 0, 0xd);
    raster = TargetRaster;
    mwPlyFxSetOutBufPitchHeight(ctx->handle, raster->width << 2, raster->height);
    mwPlyFxCnvFrmARGB8888(ctx->handle, &ctx->frame, pixels);
    DCFlushRangeNoSync(pixels, (unsigned long)((width * height) << 2));
    RwRasterUnlock(TargetRaster);
}

void MovieManager_RW_VSync(void) {}

void MovieManager_RW_StopVideo(void) {}

void MovieManager_RW_StartVideo(void) {}
