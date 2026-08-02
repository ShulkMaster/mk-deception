#ifndef MOVIE_MANAGER_GC_DISP_H
#define MOVIE_MANAGER_GC_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

void MovieManager_Default_ProcessFrame(void* ctx, int unused, int width, int height);
void MovieManager_Default_VSync(void);
void MovieManager_Default_StopVideo(void);
void MovieManager_Default_StartVideo(void);

#ifdef __cplusplus
}
#endif

#endif

