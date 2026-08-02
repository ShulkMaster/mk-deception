struct MovieDecoder;

struct _mwMovPlayer {
    MovieDecoder* decoder;
    unsigned char pad04[0x8C];
    int state;
    unsigned char create_params[0x16];
    unsigned short fade_total;
    unsigned short fade_current;
    unsigned char padAE[0x0A];
};

struct mwMovieInitParams {
    int display_mode;
    int source;
    const char* path;
    int audio_enable;
    float volume;
    void* tapout;
    void* start;
    void* stop;
    void* vsync;
    void* process;
    void* disc_error;
};

struct mwMovieSetup {
    int once;
    int initialized;
    float refresh_rate;
    float volume;
    const char* path;
    void* tapout;
    void* start;
    void* stop;
    void* vsync;
    void* process;
    void* disc_error;
    int reserved;
};

typedef void (*DecoderStateCall)(void* decoder, int state);

struct MovieDecoderVtable {
    void* slots_00[10];
    DecoderStateCall set_state; /* +0x28 */
};

struct MovieDecoder {
    MovieDecoderVtable* vtbl;
};

extern mwMovieSetup MoviePlayerSetup;

extern "C" void* mwMovMalloc(unsigned long size);
extern "C" void mwMovFree(void* memory);
extern "C" void* memset(void* memory, int value, unsigned long size);
extern "C" void* memcpy(void* destination, const void* source, unsigned long size);
extern "C" void __mwMovie_initVideo(void);
extern "C" void __mwMovie_shutdownVideo(void);
extern "C" void __mwMovie_syncFrame(void);
extern "C" void ADXM_ExecMain(void);
extern "C" void initADXwithDVD(const char* path, int audio_enable);
extern "C" void initADXwithPC(const char* path, int audio_enable);
extern "C" void initADXwithMEM(int audio_enable);
extern "C" void shutdownADX(void);
extern "C" void OSPanic(const char* file, int line, const char* format, ...);

void initSofdec(void);
void shutdownSofdec(void);
void createSofdecPlayer(_mwMovPlayer* player);
void destroySofdecPlayer(_mwMovPlayer* player);
void startMoviePlay(_mwMovPlayer* player, const char* path, unsigned char looping);
void stopMoviePlay(_mwMovPlayer* player);
void executeMovieFrame(_mwMovPlayer* player);
void updatePlayerState(_mwMovPlayer* player);
void MOVPRINT(const char* format, ...);

static const char mwMovie_file[] = "mwMovie.cpp";
static const char assert_zero[] = "Assertion failure: 0 && 1";
static const char assert_zero_retail[] = "Assertion failure: 0 == 1";
static const char assert_player[] = "Assertion failure: 0L != player";
static const char player_not_initialized[] = "mwMovie :: Movie player was not initialized.";
static const char no_player_for_tick[] =
    "mwMovie - No player object passed into tick function!\n";
static const char stopping_warning[] =
    "mwMovie Warning - Player is in the stopping phase!\n";
static const char unknown_player_state[] = "mwMovie - Unknown player state!\n";

extern "C" void mwMovieSetTapoutCallback(void* callback) {
    MoviePlayerSetup.tapout = callback;
}

extern "C" int mwMovieInit(mwMovieInitParams* params) {
    if (MoviePlayerSetup.once == 0) {
        MoviePlayerSetup.once = 1;
    }
    if (MoviePlayerSetup.initialized == 1) {
        return 0;
    }

    if (params->display_mode == 0) {
        MoviePlayerSetup.refresh_rate = 59.94f;
    } else if (params->display_mode == 1) {
        MoviePlayerSetup.refresh_rate = 50.0f;
    } else if (params->display_mode == 2) {
        MoviePlayerSetup.refresh_rate = 29.97f;
    } else {
        return 0;
    }

    __mwMovie_initVideo();
    MoviePlayerSetup.path = params->path;
    if (params->source == 0) {
        initADXwithDVD(params->path, params->audio_enable);
    } else if (params->source == 1) {
        initADXwithPC(params->path, params->audio_enable);
    } else if (params->source == 2) {
        initADXwithMEM(params->audio_enable);
    } else {
        return 0;
    }

    initSofdec();
    MoviePlayerSetup.volume = params->volume;
    MoviePlayerSetup.tapout = params->tapout;
    MoviePlayerSetup.start = params->start;
    MoviePlayerSetup.stop = params->stop;
    MoviePlayerSetup.vsync = params->vsync;
    MoviePlayerSetup.process = params->process;
    MoviePlayerSetup.disc_error = params->disc_error;
    MoviePlayerSetup.initialized = 1;
    return 1;
}

extern "C" void mwMovieShutDown(void) {
    int zero;

    shutdownSofdec();
    shutdownADX();
    __mwMovie_shutdownVideo();
    zero = 0;
    MoviePlayerSetup.initialized = zero;
}

extern "C" float mwMovieGetVolume(void) {
    return MoviePlayerSetup.volume;
}

extern "C" _mwMovPlayer* mwMovieCreatePlayer(void* params) {
    _mwMovPlayer* player;

    if (MoviePlayerSetup.initialized == 0) {
        MOVPRINT(player_not_initialized);
        OSPanic(mwMovie_file, 0x146, assert_zero);
        return 0;
    }
    player = (_mwMovPlayer*)mwMovMalloc(sizeof(_mwMovPlayer));
    memset(player, 0, sizeof(_mwMovPlayer));
    memcpy(player->create_params, params, 0x18);
    createSofdecPlayer(player);
    player->state = 0;
    return player;
}

extern "C" void mwMovieDestroyPlayer(_mwMovPlayer* player) {
    if (player == 0) {
        OSPanic(mwMovie_file, 0x15F, assert_player);
    }
    destroySofdecPlayer(player);
    memset(player, 0, sizeof(_mwMovPlayer));
    mwMovFree(player);
}

static void set_decoder_state(_mwMovPlayer* player, int state) {
    player->decoder->vtbl->set_state(player->decoder, state);
}

extern "C" void mwMovieStartPlayback(_mwMovPlayer* player, const char* path) {
    if (player->state >= 0 && player->state < 4) {
        set_decoder_state(player, 1);
        startMoviePlay(player, path, 0);
        player->state = 1;
    }
}

extern "C" void mwMovieStartPlaybackLooping(_mwMovPlayer* player, const char* path) {
    if (player->state >= 0 && player->state < 4) {
        set_decoder_state(player, 1);
        startMoviePlay(player, path, 1);
        player->state = 1;
    }
}

extern "C" void mwMovieStopPlayback(_mwMovPlayer* player) {
    if (player == 0 || player->state == 0 || player->state == 4) {
        return;
    }
    if (player->state == 1 || player->state == 3) {
        set_decoder_state(player, 0);
    }
    if (player->state >= 1 && player->state <= 3) {
        stopMoviePlay(player);
        player->state = 0;
    }
}

extern "C" void mwMovieUnPauseMovie(_mwMovPlayer* player) {
    if (player == 0) {
        MOVPRINT(no_player_for_tick);
        OSPanic(mwMovie_file, 0x1F7, assert_zero_retail);
        return;
    }

    switch (player->state) {
    case 3:
        set_decoder_state(player, 0);
        player->state = 2;
        break;
    case 4:
        MOVPRINT(stopping_warning);
        break;
    default:
        MOVPRINT(unknown_player_state);
        OSPanic(mwMovie_file, 0x20D, assert_zero_retail);
        break;
    }
}

extern "C" int mwMoviePlayTick(_mwMovPlayer* player) {
    int finished;

    if (player == 0) {
        return 1;
    }
    finished = 0;
    __mwMovie_syncFrame();
    ADXM_ExecMain();
    updatePlayerState(player);
    if (player->state == 0) {
        finished = 1;
    } else if (player->state == 2 || player->state == 3) {
        executeMovieFrame(player);
        if (MoviePlayerSetup.tapout != 0 &&
            ((int (*)(void))MoviePlayerSetup.tapout)() == 1) {
            player->state = 4;
            player->fade_current = 0;
        }
    } else if (player->state == 4) {
        if (player->fade_current < player->fade_total) {
            player->fade_current++;
        } else {
            finished = 1;
        }
    } else if (player->state != 1) {
        finished = 1;
    }
    if (finished == 1) {
        stopMoviePlay(player);
        player->state = 0;
    }
    return finished;
}
