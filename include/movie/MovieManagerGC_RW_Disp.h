#ifndef MOVIE_MANAGER_GC_RW_DISP_H
#define MOVIE_MANAGER_GC_RW_DISP_H

typedef struct RwRaster RwRaster;

#ifdef __cplusplus
extern "C" {
#endif

void MovieManager_RW_Set_Target_Raster(RwRaster* raster);
void MovieManager_RW_ProcessFrame(void* ctx, int unused, int width, int height);
void MovieManager_RW_VSync(void);
void MovieManager_RW_StopVideo(void);
void MovieManager_RW_StartVideo(void);

#ifdef __cplusplus
}
#endif

#endif
