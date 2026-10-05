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
#include "runtime/utils.h"

extern "C" {

int mwMovie_num_players;
int mwMovie_initialized;

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

/* TODO: [breakthrough needed] 0.00%; retail inlines MovieUpdate/Stop/Delete defined later (-inline deferred lead) and calls SetSubtitleLanguage per case. */
void Simple_MoviePlayFullScreen(const char* path, int width, int height,
                                MovieTapoutFn tapout_cb) {
    MoviePlayer* movie;
    int lang;
    int sub_lang;

    mslStopAll(gMsi);
    mslSuspendSpuDma();
    movie = MovieNewFullScreen(width, height);
    if (movie == 0) {
        OSPanic(STR_FILE_MOVIEMANAGER, 0x2D8, STR_ASSERT_FAILED);
    } else {
        mwMovieSetMovieVolume(movie->handle, game_settings.volume[0]);
        if (tapout_cb != 0) {
            mwMovieSetTapoutCallback((void*)tapout_cb);
        }

        lang = get_language();
        switch (lang) {
        case 1:
            sub_lang = 3;
            break;
        case 2:
            sub_lang = 2;
            break;
        case 3:
            sub_lang = 1;
            break;
        case 4:
            sub_lang = 4;
            break;
        default:
            sub_lang = 0;
            break;
        }
        SetSubtitleLanguage(sub_lang);

        MoviePlayFullScreen(movie, path);
        while (MovieUpdate(movie) == 0) {
            gc_native_display_render_movie(0);
        }
        MovieStop(movie);
        MovieDelete(movie);
    }
    mslResumeSpuDma();
}

void MovieDeleteTexture(RwTexture* texture) {
    RwTextureDestroy(texture);
}

/* TODO: [borked] 82.15385%; addressing mask clears bits16..23 instead of8..15;
 * save/restore and parameter register differences also remain. */
RwTexture* MovieNewTexture(int width, int height) {
    RwRaster* raster;
    void* pixels;
    RwTexture* texture;
    int flags;

    raster = RwRasterCreate(width, height, 0x20, 4);
    pixels = RwRasterLock(raster, 0, 9);
    if (pixels != 0) {
        memset(pixels, 0, width * height * 4);
    }
    RwRasterUnlock(raster);
    texture = RwTextureCreate(raster);
    flags = texture->filter_flags;
    flags = (flags & 0xFF00FFFF) | 0x3300;
    texture->filter_flags = flags;
    strcpy(texture->name, STR_TEXTURE_NAME);
    return texture;
}

int MovieIsPlaying(MoviePlayer* movie) {
    int state;

    state = movie->state;
    if (state == 1 || state == 2) {
        return 1;
    }
    return 0;
}

int MovieUpdate(MoviePlayer* movie) {
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

void MovieStop(MoviePlayer* movie) {
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

void MovieDelete(MoviePlayer* movie) {
    switch (movie->state) {
    case 2:
        mwMovieUnPauseMovie(movie->handle);
    case 1:
        MovieStop(movie);
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

void MovieShutdownSystem(void) {
    if (mwMovie_initialized != 0) {
        mwMovieShutDown();
        mwMovie_initialized = 0;
    }
}

/* TODO: [near miss] 68.02%; switch shape matches; retail inlines MovieStop here (inlining-mode lead for MovieManager.o). */
void MoviePlayModeSelect(MoviePlayer* movie, const char* path) {
    switch (movie->state) {
    case 0:
        break;
    case 1:
    case 2:
        MovieStop(movie);
        break;
    default:
        mwMovLog(STR_INVALID_START);
        return;
    }
    mwMovieStartPlaybackLooping(movie->handle, path);
    movie->state = 1;
}

MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height) {
    return MovieNew(raster, 0, 1, width, height, 1, 0x1E8480);
}

/* TODO: [near miss] 68.02%; switch shape matches; retail inlines MovieStop here (inlining-mode lead for MovieManager.o). */
void MoviePlayFullScreen(MoviePlayer* movie, const char* path) {
    switch (movie->state) {
    case 0:
        break;
    case 1:
    case 2:
        MovieStop(movie);
        break;
    default:
        mwMovLog(STR_INVALID_START);
        return;
    }
    mwMovieStartPlayback(movie->handle, path);
    movie->state = 1;
}

MoviePlayer* MovieNewFullScreen(int width, int height) {
    return MovieNew(0, 1, 0, width, height, 0, 0x2DC6C0);
}

}
