#ifndef MOVIE_MANAGER_H
#define MOVIE_MANAGER_H

#include "movie/mwMovie.h"

typedef struct RwRaster RwRaster;
typedef struct RwTexture RwTexture;
typedef int (*MovieTapoutFn)(void);

typedef struct MoviePlayer {
    int state;
    _mwMovPlayer* handle;
    RwRaster* raster;
} MoviePlayer;

#ifdef __cplusplus
extern "C" {
#endif

void Simple_MoviePlayFullScreen(const char* path, int width, int height,
                                MovieTapoutFn tapout_cb);
void MovieDeleteTexture(RwTexture* texture);
RwTexture* MovieNewTexture(int width, int height);
int MovieIsPlaying(MoviePlayer* movie);
int MovieUpdate(MoviePlayer* movie);
void MovieStop(MoviePlayer* movie);
void MovieDelete(MoviePlayer* movie);
MoviePlayer* MovieNew(RwRaster* raster, int use_audio, int use_rw, int width, int height,
                      unsigned int composition_flag, unsigned int maximum_bps);
void MovieShutdownSystem(void);
void MoviePlayModeSelect(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height);
void MoviePlayFullScreen(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewFullScreen(int width, int height);

#ifdef __cplusplus
}
#endif

#endif
