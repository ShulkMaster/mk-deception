#ifndef MOVIE_MANAGER_H
#define MOVIE_MANAGER_H

typedef struct RwRaster RwRaster;
typedef struct RwTexture RwTexture;
typedef struct _mwMovPlayer _mwMovPlayer;
typedef int (*MovieTapoutFn)(void);

/*
 * Sofdec player shell (MovieManager.o).
 *
 * Retail: Attract/title wait path (game owns wait loop):
 *   play_movie(id) -> Simple_MoviePlayFullScreen(path, w, h, tapout)
 *     -> MovieNewFullScreen / MovieNew
 *     -> MoviePlayFullScreen
 *     -> while (MovieUpdate(movie) == 0) gc_native_display_render_movie();
 *     -> MovieStop + MovieDelete
 * MovieUpdate: returns 1 when mwMoviePlayTick reports done (EOF/tapout;
 *   state -> idle), else 0. state: 0 idle, 1 playing, 2 paused; else invalid.
 * Soft ceiling: Sofdec CRI / MovieManagerGC_* / RW upload Matching deferred;
 *   keep Midway shell callable.
 */

typedef struct MoviePlayer {
    int state;
    void* handle;
    RwRaster* raster;
} MoviePlayer;

/* Retail: non-zero = EOF or tapout (MovieUpdate wraps this). */
int mwMoviePlayTick(_mwMovPlayer* player);

void Simple_MoviePlayFullScreen(const char* path, int width, int height,
                                MovieTapoutFn tapout_cb);
void MovieDeleteTexture(RwTexture* texture);
RwTexture* MovieNewTexture(int width, int height);
int MovieIsPlaying(MoviePlayer* movie);
int MovieUpdate(MoviePlayer* movie);
void MovieStop(MoviePlayer* movie);
void MovieDelete(MoviePlayer* movie);
MoviePlayer* MovieNew(RwRaster* raster, int use_audio, int use_rw, int width, int height,
                      void* create_flag, void* buffer_bytes);
void MovieShutdownSystem(void);
void MoviePlayModeSelect(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height);
void MoviePlayFullScreen(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewFullScreen(int width, int height);

#endif
