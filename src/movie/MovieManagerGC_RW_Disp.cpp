extern "C" {
#include "rw/rwcore_types.h"
#include "movie/mwsfx.h"

typedef struct MovieProcessCtx {
    int handle;
    int field_0x04;
    MwsFrameInfo frame; /* +0x08 -- source frame descriptor */
} MovieProcessCtx;

void DCFlushRangeNoSync(void* addr, unsigned long length);

/* MWCC emits .sbss in reverse declaration order. */
int gap_08_805108C4_sbss;
RwRaster* TargetRaster;

void MovieManager_RW_Set_Target_Raster(RwRaster* raster) {
    TargetRaster = raster;
}

/* Soft ceiling: MovieManager_RW_ProcessFrame ~72% -- retail stmw/lmw scheduling;
 * enabling the required flag crashes MWCC when small-data placement is active. */
void MovieManager_RW_ProcessFrame(MovieProcessCtx* ctx, int unused, int width, int height) {
    void* pixels;
    RwRaster* raster;

    (void)unused;
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

}
