#include "movie/MovieManager.h"

#include "dolphin/os.h"
#include "game/settings.h"
#include "movie/MovieConfig.h"
#include "movie/MovieManagerGC_Disp.h"
#include "movie/MovieManagerGC_RW_Disp.h"
#include "movie/MovieSubtitle_GC.h"
#include "movie/mwMovie.h"
#include "msl/mslcore.h"
#include "platform/disc_error.h"
#include "platform/gcdisplay.h"
#include "platform/gcutils.h"
#include "rw/rwcore_types.h"
#include "runtime/cstring.h"
#include "runtime/sound.h"
#include "runtime/utils.h"


extern "C" {

static int mwMovie_initialized;
static int mwMovie_num_players;

static const char stringBase0[] =
    "Invalid player state to update movie!\n\0"
    "Invalid player state to stop movie playback!\n\0"
    "Invalid player state to stop movie!\n\0"
    "MovieManager.cpp\0"
    "Assertion failed: 0\0"
    "MOVIE\0"
    "Invalid player state to start movie playback!\n\0"
    "Error creating a movie player, out of memory!\n";

#define STR_INVALID_UPDATE (&stringBase0[0])
#define STR_INVALID_STOP_PLAYBACK (&stringBase0[0x27])
#define STR_INVALID_STOP (&stringBase0[0x55])
#define STR_FILE_MOVIEMANAGER (&stringBase0[0x7A])
#define STR_ASSERT_FAILED (&stringBase0[0x8B])
#define STR_TEXTURE_NAME (&stringBase0[0x9F])
#define STR_INVALID_START (&stringBase0[0xA5])
#define STR_OUT_OF_MEMORY (&stringBase0[0xD4])
static inline void movie_stop_inline(MoviePlayer* movie) {
    switch (movie->state) {
    case 2:
        mwMovieUnPauseMovie(movie->handle);
    case 1:
        mwMovieStopPlayback(movie->handle);
        movie->state = 0;
        break;
    case 0:
        break;
    default:
        mwMovLog(STR_INVALID_STOP_PLAYBACK);
        break;
    }
}

static inline int movie_update_inline(MoviePlayer* movie) {
    switch (movie->state) {
    case 1:
        if (movie->raster != 0) {
            MovieManager_RW_Set_Target_Raster(movie->raster);
        }
        if (mwMoviePlayTick(movie->handle) != 0) {
            movie->state = 0;
            return 1;
        }
        break;
    case 0:
    case 2:
        break;
    default:
        mwMovLog(STR_INVALID_UPDATE);
        break;
    }
    return 0;
}

static inline void movie_delete_inline(MoviePlayer* movie) {
    switch (movie->state) {
    case 2:
        mwMovieUnPauseMovie(movie->handle);
    case 1:
        movie_stop_inline(movie);
    case 0:
        mwMovieDestroyPlayer(movie->handle);
        movie->handle = 0;
        mwMovFree(movie);
        mwMovie_num_players--;
        break;
    default:
        mwMovLog(STR_INVALID_STOP);
        break;
    }
    if (mwMovie_num_players == 0 && mwMovie_initialized != 0) {
        mwMovieShutDown();
        mwMovie_initialized = 0;
    }
}

void Simple_MoviePlayFullScreen(const char* path, int width, int height, MovieTapoutFn tapout_cb);
void MovieDeleteTexture(RwTexture* texture);
RwTexture* MovieNewTexture(int width, int height);
int MovieIsPlaying(MoviePlayer* movie);
int MovieUpdate(MoviePlayer* movie);
void MovieStop(MoviePlayer* movie);
void MovieDelete(MoviePlayer* movie);
MoviePlayer* MovieNew(RwRaster* raster, int use_audio, int use_rw, int width, int height, unsigned int composition_flag, unsigned int maximum_bps);
void MovieShutdownSystem(void);
void MoviePlayModeSelect(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height);
void MoviePlayFullScreen(MoviePlayer* movie, const char* path);
MoviePlayer* MovieNewFullScreen(int width, int height);



MoviePlayer* MovieNewFullScreen(int width, int height) {
    return MovieNew(0, 1, 0, width, height, 0, 0x2DC6C0);
}

void MoviePlayFullScreen(MoviePlayer* movie, const char* path) {
    switch (movie->state) {
    case 1:
    case 2:
        movie_stop_inline(movie);
    case 0:
        mwMovieStartPlayback(movie->handle, path);
        movie->state = 1;
        break;
    default:
        mwMovLog(STR_INVALID_START);
        break;
    }
}

MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height) {
    return MovieNew(raster, 0, 1, width, height, 1, 0x1E8480);
}

void MoviePlayModeSelect(MoviePlayer* movie, const char* path) {
    switch (movie->state) {
    case 1:
    case 2:
        movie_stop_inline(movie);
    case 0:
        mwMovieStartPlaybackLooping(movie->handle, path);
        movie->state = 1;
        break;
    default:
        mwMovLog(STR_INVALID_START);
        break;
    }
}

void MovieShutdownSystem(void) {
    if (mwMovie_initialized != 0) {
        mwMovieShutDown();
        mwMovie_initialized = 0;
    }
}

MoviePlayer* MovieNew(RwRaster* raster, int use_audio, int use_rw, int width, int height,
                      unsigned int composition_flag, unsigned int maximum_bps) {
    MoviePlayer* player;
    MwMovieInitParams initParams;
    MwMovieCreateParams createParams;

    if (mwMovie_initialized == 0) {
        initParams.display_mode = (refresh_rate() != 0x32) ? 0 : 1;
        initParams.source = 0;
        initParams.path = get_movie_path();
        if (use_audio != 0) {
            initParams.audio_enable = 1;
            initParams.volume = game_settings.volume[0];
        } else {
            initParams.audio_enable = 0;
        }
        if (use_rw != 0) {
            void (*cb_start)(void);
            void (*cb_stop)(void);
            void (*cb_vsync)(void);
            void (*cb_process)(void* ctx, int unused, int w, int h);

            cb_start = MovieManager_RW_StartVideo;
            cb_stop = MovieManager_RW_StopVideo;
            cb_vsync = MovieManager_RW_VSync;
            cb_process = MovieManager_RW_ProcessFrame;
            initParams.start = cb_start;
            initParams.tapout = 0;
            initParams.stop = cb_stop;
            initParams.vsync = cb_vsync;
            initParams.process = cb_process;
        } else {
            void (*cb_start)(void);
            void (*cb_stop)(void);
            void (*cb_vsync)(void);
            void (*cb_process)(void* ctx, int unused, int w, int h);

            cb_start = MovieManager_Default_StartVideo;
            cb_stop = MovieManager_Default_StopVideo;
            cb_vsync = MovieManager_Default_VSync;
            cb_process = MovieManager_Default_ProcessFrame;
            initParams.start = cb_start;
            initParams.tapout = 0;
            initParams.stop = cb_stop;
            initParams.vsync = cb_vsync;
            initParams.process = cb_process;
        }
        initParams.disc_error = check_handle_disc_error;
        mwMovieInit(&initParams);
        mwMovie_initialized = 1;
    }

    player = (MoviePlayer*)mwMovMalloc(sizeof(*player));
    if (player == 0) {
        mwMovLog(STR_OUT_OF_MEMORY);
    } else {
        short const_one;
        short const_four;

        const_one = 1;
        const_four = 4;
        createParams.frame_count = const_one;
        createParams.audio_channel = 0;
        createParams.maximum_bps = maximum_bps;
        createParams.width = width;
        createParams.height = height;
        createParams.composition_flag = composition_flag;
        createParams.output_width = width;
        createParams.output_height = height;
        createParams.fade_frames = const_four;
        player->handle = mwMovieCreatePlayer(&createParams);
        player->state = 0;
        player->raster = raster;
    }
    mwMovie_num_players++;
    return player;
}

void MovieDelete(MoviePlayer* movie) {
    movie_delete_inline(movie);
}

void MovieStop(MoviePlayer* movie) {
    movie_stop_inline(movie);
}

int MovieUpdate(MoviePlayer* movie) {
    return movie_update_inline(movie);
}

int MovieIsPlaying(MoviePlayer* movie) {
    int state;

    state = movie->state;
    if (state == 1 || state == 2) {
        return 1;
    }
    return 0;
}

RwTexture* MovieNewTexture(int width, int height) {
    RwRaster* raster;
    void* pixels;
    RwTexture* texture;

    raster = RwRasterCreate(width, height, 0x20, 4);
    pixels = RwRasterLock(raster, 0, 9);
    if (pixels != 0) {
        memset(pixels, 0, width * height * 4);
    }
    RwRasterUnlock(raster);
    texture = RwTextureCreate(raster);
    rwTextureWriteAddressModes(texture, 3);
    strcpy(texture->name, STR_TEXTURE_NAME);
    return texture;
}

void MovieDeleteTexture(RwTexture* texture) {
    RwTextureDestroy(texture);
}

void Simple_MoviePlayFullScreen(const char* path, int width, int height,
                                MovieTapoutFn tapout_cb) {
    MoviePlayer* movie;
    int finished = 0;

    mslStopAll(msi);
    mslSuspendSpuDma();
    movie = MovieNewFullScreen(width, height);
    if (movie != 0) {
        mwMovieSetMovieVolume(movie->handle, game_settings.volume[0]);
        if (tapout_cb != 0) {
            mwMovieSetTapoutCallback((void*)tapout_cb);
        }
        switch (get_language()) {
        case 1:
            SetSubtitleLanguage(3);
            break;
        case 4:
            SetSubtitleLanguage(4);
            break;
        case 3:
            SetSubtitleLanguage(1);
            break;
        case 2:
            SetSubtitleLanguage(2);
            break;
        default:
            SetSubtitleLanguage(0);
            break;
        }
        MoviePlayFullScreen(movie, path);
        while (finished == 0) {
            finished = movie_update_inline(movie);
            gc_native_display_render_movie((void*)finished);
        }
        movie_stop_inline(movie);
        movie_delete_inline(movie);
    } else {
        OSPanic(STR_FILE_MOVIEMANAGER, 0x2D8, STR_ASSERT_FAILED);
    }
    mslResumeSpuDma();
}



}
