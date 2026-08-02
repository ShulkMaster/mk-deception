#include "movie/MovieManager.h"

#include "movie/MovieConfig.h"
#include "movie/MovieManagerGC_Disp.h"
#include "movie/MovieSubtitle_GC.h"

typedef struct GameSettings {
    float volume[6];
    char pad[0x3440 - 0x18];
} GameSettings;

typedef struct RwTexture {
    RwRaster* raster;
    char name[0x40];
    int unk50;
} RwTexture;

/* Layout matches retail stack stores at MovieNew (sp+0x20). ntsc is always 0. */
typedef struct MwMovieInitParams {
    int pal;
    int ntsc;
    const char* path;
    int audio_enable;
    float volume;
    void* reserved;
    void (*start)(void);
    void (*stop)(void);
    void (*vsync)(void);
    void (*process)(void* ctx, int unused, int width, int height);
    void (*disc_err)(void);
} MwMovieInitParams;

/* Layout matches retail create-params block (sp+0x8, size 0x18). */
typedef struct MwMovieCreateParams {
    void* buffer_bytes;
    int reserved0; /* always 0 */
    void* create_flag;
    short width;
    short height;
    short width2;
    short height2;
    short const_one; /* always 1 */
    short const_four; /* always 4 */
} MwMovieCreateParams;

void mslStopAll(void);
void mslSuspendSpuDma(void);
void mslResumeSpuDma(void);
int get_language(void);
void mwMovieSetMovieVolume(void* handle, float volume);
void mwMovieSetTapoutCallback(void* cb);
void mwMovieUnPauseMovie(void* handle);
void mwMovieStopPlayback(void* handle);
void mwMovieDestroyPlayer(void* handle);
void mwMovieShutDown(void);
void mwMovieInit(MwMovieInitParams* params);
void* mwMovieCreatePlayer(MwMovieCreateParams* params);
void mwMovieStartPlayback(void* handle, const char* path);
void mwMovieStartPlaybackLooping(void* handle, const char* path);
void gc_native_display_render_movie(void);
void OSPanic(const char* file, int line, const char* fmt, ...);
void check_handle_disc_error(void);
void MovieManager_RW_Set_Target_Raster(RwRaster* raster);
void MovieManager_RW_ProcessFrame(void* ctx, int unused, int width, int height);
void MovieManager_RW_VSync(void);
void MovieManager_RW_StopVideo(void);
void MovieManager_RW_StartVideo(void);
int refresh_rate(void);
const char* get_movie_path(void);
RwRaster* RwRasterCreate(int width, int height, int depth, int format);
void* RwRasterLock(RwRaster* raster, int lockMode, int flags);
void RwRasterUnlock(RwRaster* raster);
RwTexture* RwTextureCreate(RwRaster* raster);
void RwTextureDestroy(RwTexture* texture);
void memset(void* dest, int val, int size);
char* strcpy(char* dest, const char* src);

extern GameSettings game_settings;

/* MWCC emits .sbss in reverse declaration order. */
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

/*
 * Fullscreen Sofdec path used by play_movie (attract intro / logos).
 *
 * Game owns the wait: loop MovieUpdate until mwMoviePlayTick signals done
 * (EOF / tapout), then Stop + Delete. Present converted frames behind the
 * tick (see MovieManager.h retail call contract).
 *
 * get_language() -> SetSubtitleLanguage(): 0/default=0, 1->3, 2->2, 3->1, 4->4.
 *
 * Soft ceiling: Simple_MoviePlayFullScreen ~0% - retail inlines
 * Update/Stop/Delete; we call the shells for readable structured C. Stop.
 */
void Simple_MoviePlayFullScreen(const char* path, int width, int height, void* tapout_cb) {
    MoviePlayer* movie;
    int lang;
    int sub_lang;

    mslStopAll();
    mslSuspendSpuDma();
    movie = MovieNewFullScreen(width, height);
    if (movie == 0) {
        OSPanic(STR_FILE_MOVIEMANAGER, 0x2D8, STR_ASSERT_FAILED);
    } else {
        mwMovieSetMovieVolume(movie->handle, game_settings.volume[0]);
        if (tapout_cb != 0) {
            mwMovieSetTapoutCallback(tapout_cb);
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
            /* 0 and >=5 */
            sub_lang = 0;
            break;
        }
        SetSubtitleLanguage(sub_lang);

        MoviePlayFullScreen(movie, path);
        /* MovieUpdate == 0 while playing; non-zero when tick reports EOF/tapout. */
        while (MovieUpdate(movie) == 0) {
            gc_native_display_render_movie();
        }
        MovieStop(movie);
        MovieDelete(movie);
    }
    mslResumeSpuDma();
}

void MovieDeleteTexture(RwTexture* texture) {
    RwTextureDestroy(texture);
}

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
    flags = texture->unk50;
    flags = (flags & 0xFF00FFFF) | 0x3300;
    texture->unk50 = flags;
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

/*
 * One play-frame step. Returns 1 when mwMoviePlayTick is non-zero (EOF /
 * tapout) and clears state to idle; else 0. Callers advance attract on true.
 * Soft ceiling: MovieUpdate ~63% - cascaded cmpwi leftover. Stop.
 */
int MovieUpdate(MoviePlayer* movie) {
    int state;

    state = movie->state;
    if (state == 1) {
        if (movie->raster != 0) {
            MovieManager_RW_Set_Target_Raster(movie->raster);
        }
        if (mwMoviePlayTick(movie->handle) != 0) {
            movie->state = 0;
            return 1;
        }
        return 0;
    }
    if (state < 0 || state >= 3) {
        mwMovLog(STR_INVALID_UPDATE);
    }
    /* state 0 (idle) / 2 (paused): not finished via tick. */
    return 0;
}

/*
 * Stop playback. Playing: stop. Paused: unpause then stop. Idle: no-op.
 * Soft ceiling: MovieStop ~74% - retail fallthrough vs structured if. Stop.
 */
void MovieStop(MoviePlayer* movie) {
    int state;

    state = movie->state;
    if (state == 1) {
        mwMovieStopPlayback(movie->handle);
        movie->state = 0;
    } else if (state == 2) {
        mwMovieUnPauseMovie(movie->handle);
        mwMovieStopPlayback(movie->handle);
        movie->state = 0;
    } else if (state != 0) {
        mwMovLog(STR_INVALID_STOP_PLAYBACK);
    }
}

/*
 * Destroy player. Invalid state logs and skips free (retail). Last player
 * shuts down mwMovie. DestroyPlayer frees player resources.
 * Soft ceiling: MovieDelete ~71% - call vs retail-inlined Stop. Stop.
 */
void MovieDelete(MoviePlayer* movie) {
    int state;

    state = movie->state;
    if (state < 0 || state >= 3) {
        mwMovLog(STR_INVALID_STOP);
    } else {
        if (state == 2) {
            mwMovieUnPauseMovie(movie->handle);
        }
        MovieStop(movie);
        mwMovieDestroyPlayer(movie->handle);
        movie->handle = 0;
        mwMovFree(movie);
        mwMovie_num_players--;
    }
    if (mwMovie_num_players == 0 && mwMovie_initialized != 0) {
        mwMovieShutDown();
        mwMovie_initialized = 0;
    }
}

/*
 * Create player; first call runs mwMovieInit (Sofdec once).
 * Fullscreen: MovieNew(0, audio=1, rw=0, w, h, flag=0, buf=0x2DC6C0).
 * Mode-select tex: MovieNew(raster, 0, rw=1, w, h, flag=1, buf=0x1E8480).
 * mwMovieCreatePlayer takes the 0x18-byte create-params block only.
 */
MoviePlayer* MovieNew(RwRaster* raster, int use_audio, int use_rw, int width, int height,
                      void* create_flag, void* buffer_bytes) {
    MoviePlayer* player;
    MwMovieInitParams initParams;
    MwMovieCreateParams createParams;

    if (mwMovie_initialized == 0) {
        initParams.pal = (refresh_rate() == 0x32);
        initParams.ntsc = 0;
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
            initParams.reserved = 0;
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
            initParams.reserved = 0;
            initParams.stop = cb_stop;
            initParams.vsync = cb_vsync;
            initParams.process = cb_process;
        }
        initParams.disc_err = check_handle_disc_error;
        mwMovieInit(&initParams);
        mwMovie_initialized = 1;
    }

    player = mwMovMalloc(0xC);
    if (player == 0) {
        mwMovLog(STR_OUT_OF_MEMORY);
    } else {
        short const_one;
        short const_four;

        const_one = 1;
        const_four = 4;
        createParams.const_one = const_one;
        createParams.reserved0 = 0;
        createParams.buffer_bytes = buffer_bytes;
        createParams.width = (short)width;
        createParams.height = (short)height;
        createParams.create_flag = create_flag;
        createParams.width2 = (short)width;
        createParams.height2 = (short)height;
        createParams.const_four = const_four;
        player->handle = mwMovieCreatePlayer(&createParams);
        player->state = 0;
        player->raster = raster;
    }
    /* Retail increments even on OOM (player may be NULL). */
    mwMovie_num_players++;
    return player;
}

void MovieShutdownSystem(void) {
    if (mwMovie_initialized != 0) {
        mwMovieShutDown();
        mwMovie_initialized = 0;
    }
}

void MoviePlayModeSelect(MoviePlayer* movie, const char* path) {
    int state;

    state = movie->state;
    if (state != 0) {
        if (state < 0 || state >= 3) {
            mwMovLog(STR_INVALID_START);
            return;
        }
        MovieStop(movie);
    }
    mwMovieStartPlaybackLooping(movie->handle, path);
    movie->state = 1;
}

MoviePlayer* MovieNewModeSelect(RwRaster* raster, int width, int height) {
    return MovieNew(raster, 0, 1, width, height, (void*)1, (void*)0x1E8480);
}

void MoviePlayFullScreen(MoviePlayer* movie, const char* path) {
    int state;

    state = movie->state;
    if (state != 0) {
        if (state < 0 || state >= 3) {
            mwMovLog(STR_INVALID_START);
            return;
        }
        /* Playing or paused: stop before restart (same as MovieStop). */
        MovieStop(movie);
    }
    mwMovieStartPlayback(movie->handle, path);
    movie->state = 1;
}

MoviePlayer* MovieNewFullScreen(int width, int height) {
    return MovieNew(0, 1, 0, width, height, 0, (void*)0x2DC6C0);
}
