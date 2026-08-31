#include "dolphin/os.h"
#include "movie/MovieConfig.h"
#include "movie/mwMovie.h"
#include "movie/mwMovie_platform.h"
#include "runtime/cmath.h"
#include "runtime/cstdarg.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"

typedef struct MwsPlayer MwsPlayer;

typedef struct MwsPlayerInterface {
    void* reserved[3];
    void (*vsync)(void);
    int (*execute_server)(MwsPlayer*);
    void (*destroy)(MwsPlayer*);
    void (*start_filename)(MwsPlayer*, const char*);
    void (*stop)(MwsPlayer*);
    int (*get_status)(MwsPlayer*);
    void (*get_time)(MwsPlayer*, int*, int*);
    void (*pause)(MwsPlayer*, int);
    void (*set_volume)(MwsPlayer*, int);
} MwsPlayerInterface;

struct MwsPlayer {
    MwsPlayerInterface* interface;
};

typedef struct MwsInitParam {
    float frame_rate;
    int maximum_width;
    int decoder_count;
    int field_0C;
    int field_10;
    int field_14;
    int field_18;
    int svm_parameter;
} MwsInitParam;

typedef struct MwsCreateParams {
    int file_type;
    int maximum_bps;
    int width;
    int height;
    int frame_count;
    int decoder_count;
    void* work;
    int work_size;
    int composition_mode;
    int buffer_format;
    int field_28;
    int field_2C;
} MwsCreateParams;

typedef struct MwsTransportPair {
    int first;
    int second;
} MwsTransportPair;

typedef struct MwsTransportFrameInfo {
    MwsTransportPair pairs[7];
} MwsTransportFrameInfo;

typedef struct MwsFrameOutput {
    void* frame;
    int frame_structure;
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int picture_type;
    int frame_rate;
    int display_time;
    int display_time_source;
    int display_scale;
    int picture_order;
    int presentation_time;
    int presentation_source;
    int field_38;
    int field_3C;
    void* picture_user_data;
    int picture_user_size;
    int display_mode;
    int reserved_4C;
    MwsTransportFrameInfo transport;
} MwsFrameOutput;

typedef struct MwMoviePlayerParams {
    unsigned int maximum_bps;
    int audio_channel;
    unsigned int composition_flag;
    unsigned short width;
    unsigned short height;
    unsigned short output_width;
    unsigned short output_height;
    unsigned short frame_count;
    unsigned short fade_frames;
} MwMoviePlayerParams;

struct _mwMovPlayer {
    MwsPlayer* player_handle;
    int reserved_04;
    MwsFrameOutput frame;
    int state;
    MwMoviePlayerParams create;
    unsigned short fade_frame;
    unsigned short reserved_AE;
    int previous_frame;
    int reserved_B4;
};

typedef int (*MovieTapoutCallback)(void);
typedef void (*MovieStartCallback)(unsigned short, unsigned short);
typedef void (*MovieStopCallback)(void);
typedef void (*MovieVsyncCallback)(void);
typedef void (*MovieProcessCallback)(_mwMovPlayer*, void*, int, int,
                                     unsigned short, unsigned short, int);
typedef void (*MovieDiscErrorCallback)(void);

typedef struct mwMovieSetup {
    int once;
    int initialized;
    float refresh_rate;
    float volume;
    const char* path;
    MovieTapoutCallback tapout;
    MovieStartCallback start;
    MovieStopCallback stop;
    MovieVsyncCallback vsync;
    MovieProcessCallback process;
    MovieDiscErrorCallback disc_error;
    int cri_error;
} mwMovieSetup;

typedef char MwsInitParamSizeCheck[sizeof(MwsInitParam) == 0x20 ? 1 : -1];
typedef char MwsCreateParamsSizeCheck[sizeof(MwsCreateParams) == 0x30 ? 1 : -1];
typedef char MwsFrameOutputSizeCheck[sizeof(MwsFrameOutput) == 0x88 ? 1 : -1];
typedef char MwMoviePlayerParamsSizeCheck[
    sizeof(MwMoviePlayerParams) == 0x18 ? 1 : -1];
typedef char MwMoviePlayerSizeCheck[sizeof(_mwMovPlayer) == 0xB8 ? 1 : -1];
typedef char MwMovieSetupSizeCheck[sizeof(mwMovieSetup) == 0x30 ? 1 : -1];

extern "C" {
void ADXM_SetupThrd(int);
void ADXM_ShutdownThrd(void);
void ADXM_SetCbErr(void (*callback)(void*, char*), void* object);
void mwPlyInitSfdFx(MwsInitParam*);
void mwPlyFinishSfdFx(void);
void mwPlySetMallocFn(void* (*malloc_fn)(void*, unsigned int),
                      void (*free_fn)(void*, void*), void* object);
MwsPlayer* mwPlyCreateSofdec(const MwsCreateParams*);
void mwPlySetAudioCh(MwsPlayer*, int);
void mwPlySetSubtitleCh(MwsPlayer*, int);
void mwPlySetSyncMode(MwsPlayer*, int);
void mwPlySetFrmSync(MwsPlayer*, int);
void mwPlyStartFnameLp(MwsPlayer*, const char*);
void mwPlyGetCurFrm(MwsPlayer*, MwsFrameOutput*);
void mwPlyRelCurFrm(MwsPlayer*);
void __mwMovie_startVideo(_mwMovPlayer*);
void displayMovieFrame(_mwMovPlayer*);
}

static void errorCallback(void*, char*);
static void initSofdec(void);
static void shutdownSofdec(void);
static void createSofdecPlayer(_mwMovPlayer*);
static void destroySofdecPlayer(_mwMovPlayer*);
static void startMoviePlay(_mwMovPlayer*, char*, bool);
static void stopMoviePlay(_mwMovPlayer*);
static int executeMovieFrame(_mwMovPlayer*);
static void updatePlayerState(_mwMovPlayer*);
static void* mallocCallback(void*, unsigned int);
static void freeCallback(void*, void*);
void MOVPRINT(const char*, ...);

namespace {
extern char stringBase0[];
}

mwMovieSetup MoviePlayerSetup = {
    0, 0, 0.0f, 1.0f, stringBase0, 0, 0, 0, 0, 0, 0, 0,
};

namespace {
char stringBase0[] =
    "\0"
    "mwMovie.cpp\0"
    "Assertion failure: player != 0L\0"
    "Assertion failure: pBitmap != 0L\0"
    "mwMovie - Player already initialized!\n\0"
    "Assertion failure: 0 == 1\0"
    "mwMovie - Unknown Display Mode for initialization !\n\0"
    "mwMovie - Unknown Movie Source Device !\n\0"
    "Assertion failure: player->playerHandle != 0L\0"
    "mwMovie - Movie player was not initialized!\n\0"
    "Assertion failure: 0L != player\0"
    "mwMovie - Player is in process of stopping another movie!\n\0"
    "mwMovie - Unknown player state!\n\0"
    "mwMovie - No player object passed into movie close function!\n\0"
    "mwMovie Warning - Movie is still preparing to be played, not playing yet!\n\0"
    "mwMovie Warning - Cannot pause movies that have not started playback!\n\0"
    "mwMovie Warning - Player is already paused!\n\0"
    "mwMovie Warning - Player is not playing anything to pause!\n\0"
    "mwMovie Warning - Player is in the stopping phase!\n\0"
    "mwMovie - No player object passed into tick function!\n\0"
    "CRI Error - %s\n\0"
    "mwMovie Error - Sofdec player already created!\n\0"
    "mwMovie Error - Cannot create movie player handle!\n\0"
    "mwMovie - There is no Sofdec player to destroy!\n\0"
    "Assertion failure: fileName != 0L\0"
    "mwMovie dropped frame(s) between frame %d and frame %d\n\0"
    "Assertion failure: 0L != player->playerHandle\0"
    "mwMovie - Internal Error reported by CRI!\n\0"
    "mwMovie - Unknown CRI SOFDEC status!\n\0"
    "SOFDEC Allocated %.2f K \t at location: 0x%x\n\0"
    "SOFDEC Failed on allocation of %.2f K!\n\0"
    "SOFDEC Freeing block at: 0x%X\n"
    "\0\0\0\0\0\0";
}

#define STR_FILE (&stringBase0[0x001])
#define STR_ASSERT_PLAYER (&stringBase0[0x00D])
#define STR_ASSERT_BITMAP (&stringBase0[0x02D])
#define STR_ALREADY_INITIALIZED (&stringBase0[0x04E])
#define STR_ASSERT_ZERO (&stringBase0[0x075])
#define STR_UNKNOWN_DISPLAY (&stringBase0[0x08F])
#define STR_UNKNOWN_SOURCE (&stringBase0[0x0C4])
#define STR_ASSERT_HANDLE (&stringBase0[0x0ED])
#define STR_NOT_INITIALIZED (&stringBase0[0x11B])
#define STR_ASSERT_PLAYER_REVERSED (&stringBase0[0x148])
#define STR_STOPPING_ANOTHER (&stringBase0[0x168])
#define STR_UNKNOWN_STATE (&stringBase0[0x1A3])
#define STR_NO_PLAYER_CLOSE (&stringBase0[0x1C4])
#define STR_STOPPING_PHASE (&stringBase0[0x2FD])
#define STR_NO_PLAYER_TICK (&stringBase0[0x331])
#define STR_CRI_ERROR (&stringBase0[0x368])
#define STR_ALREADY_CREATED (&stringBase0[0x378])
#define STR_CREATE_FAILED (&stringBase0[0x3A8])
#define STR_NO_PLAYER_DESTROY (&stringBase0[0x3DC])
#define STR_ASSERT_FILENAME (&stringBase0[0x40D])
#define STR_DROPPED_FRAMES (&stringBase0[0x42F])
#define STR_ASSERT_HANDLE_REVERSED (&stringBase0[0x467])
#define STR_INTERNAL_ERROR (&stringBase0[0x495])
#define STR_UNKNOWN_CRI_STATUS (&stringBase0[0x4C0])
#define STR_ALLOCATED (&stringBase0[0x4E6])
#define STR_ALLOCATION_FAILED (&stringBase0[0x513])
#define STR_FREEING (&stringBase0[0x53B])

static char PrintBuff[256];

extern "C" void mwMovieSetTapoutCallback(void* callback)
{
    MoviePlayerSetup.tapout = (MovieTapoutCallback)callback;
}

extern "C" int mwMovieInit(MwMovieInitParams* params)
{
    if (MoviePlayerSetup.once == 0) {
        MoviePlayerSetup.once = 1;
    }
    if (MoviePlayerSetup.initialized == 1) {
        MOVPRINT(STR_ALREADY_INITIALIZED);
        OSPanic(STR_FILE, 0x96, STR_ASSERT_ZERO);
        return 0;
    }

    switch (params->display_mode) {
    case 0:
        MoviePlayerSetup.refresh_rate = 59.94f;
        break;
    case 1:
        MoviePlayerSetup.refresh_rate = 50.0f;
        break;
    case 2:
        MoviePlayerSetup.refresh_rate = 60.0f;
        break;
    default:
        MOVPRINT(STR_UNKNOWN_DISPLAY);
        OSPanic(STR_FILE, 0xAB, STR_ASSERT_ZERO);
        return 0;
    }

    __mwMovie_initVideo();
    MoviePlayerSetup.path = params->path;
    switch (params->source) {
    case 0:
        initADXwithDVD(params->path, params->audio_enable);
        break;
    case 1:
        initADXwithPC(params->path, params->audio_enable);
        break;
    case 2:
        initADXwithMEM(params->audio_enable);
        break;
    default:
        MOVPRINT(STR_UNKNOWN_SOURCE);
        OSPanic(STR_FILE, 0xC5, STR_ASSERT_ZERO);
        return 0;
    }

    initSofdec();
    MoviePlayerSetup.volume = params->volume;
    MoviePlayerSetup.tapout = (MovieTapoutCallback)params->tapout;
    MoviePlayerSetup.start = (MovieStartCallback)params->start;
    MoviePlayerSetup.stop = (MovieStopCallback)params->stop;
    MoviePlayerSetup.vsync = (MovieVsyncCallback)params->vsync;
    MoviePlayerSetup.process = (MovieProcessCallback)params->process;
    MoviePlayerSetup.disc_error = (MovieDiscErrorCallback)params->disc_error;
    MoviePlayerSetup.initialized = 1;
    return 1;
}

extern "C" void mwMovieShutDown(void)
{
    shutdownSofdec();
    shutdownADX();
    __mwMovie_shutdownVideo();
    MoviePlayerSetup.initialized = 0;
}

#pragma dont_inline on
extern "C" float mwMovieGetVolume(void)
{
    return MoviePlayerSetup.volume;
}

extern "C" int mwMovieDbVolFromLinear(float volume)
{
    int decibels;

    if (volume == 0.0f) {
        return -960;
    }
    decibels = (int)(100.0f * (float)log10(volume * volume));
    if (decibels > 0) {
        return 0;
    }
    if (decibels < -960) {
        decibels = -960;
    }
    return decibels;
}
#pragma dont_inline reset

extern "C" void mwMovieSetMovieVolume(void* handle, float volume)
{
    _mwMovPlayer* player = (_mwMovPlayer*)handle;
    int decibels;

    if (player == 0) {
        OSPanic(STR_FILE, 0x133, STR_ASSERT_PLAYER);
    }
    if (player->player_handle == 0) {
        OSPanic(STR_FILE, 0x134, STR_ASSERT_HANDLE);
    }
    decibels = mwMovieDbVolFromLinear(volume);
    player->player_handle->interface->set_volume(player->player_handle,
                                                  decibels);
    MoviePlayerSetup.volume = volume;
}

extern "C" _mwMovPlayer* mwMovieCreatePlayer(MwMovieCreateParams* params)
{
    _mwMovPlayer* player;

    if (MoviePlayerSetup.initialized == 0) {
        MOVPRINT(STR_NOT_INITIALIZED);
        OSPanic(STR_FILE, 0x146, STR_ASSERT_ZERO);
        return 0;
    }
    player = (_mwMovPlayer*)mwMovMalloc(sizeof(_mwMovPlayer));
    memset(player, 0, sizeof(_mwMovPlayer));
    memcpy(&player->create, params, sizeof(MwMoviePlayerParams));
    createSofdecPlayer(player);
    player->state = 0;
    return player;
}

#pragma peephole off
#pragma scheduling off
extern "C" void mwMovieDestroyPlayer(_mwMovPlayer* player)
{
    if (player == 0) {
        OSPanic(STR_FILE, 0x15F, STR_ASSERT_PLAYER_REVERSED);
    }
    destroySofdecPlayer(player);
    memset(player, 0, sizeof(_mwMovPlayer));
    mwMovFree(player);
}
#pragma scheduling reset
#pragma peephole reset

extern "C" void mwMovieStartPlayback(_mwMovPlayer* player, const char* filename)
{
    switch (player->state) {
    case 0:
    case 1:
    case 2:
    case 3:
        player->player_handle->interface->pause(player->player_handle, 1);
        startMoviePlay(player, (char*)filename, false);
        player->state = 1;
        break;
    case 4:
        MOVPRINT(STR_STOPPING_ANOTHER);
        OSPanic(STR_FILE, 0x17D, STR_ASSERT_ZERO);
        break;
    default:
        MOVPRINT(STR_UNKNOWN_STATE);
        OSPanic(STR_FILE, 0x182, STR_ASSERT_ZERO);
        break;
    }
}

extern "C" void mwMovieStartPlaybackLooping(_mwMovPlayer* player,
                                               const char* filename)
{
    switch (player->state) {
    case 0:
    case 1:
    case 2:
    case 3:
        player->player_handle->interface->pause(player->player_handle, 1);
        startMoviePlay(player, (char*)filename, true);
        player->state = 1;
        break;
    case 4:
        MOVPRINT(STR_STOPPING_ANOTHER);
        OSPanic(STR_FILE, 0x19A, STR_ASSERT_ZERO);
        break;
    default:
        MOVPRINT(STR_UNKNOWN_STATE);
        OSPanic(STR_FILE, 0x19F, STR_ASSERT_ZERO);
        break;
    }
}

extern "C" void mwMovieStopPlayback(_mwMovPlayer* player)
{
    if (player == 0) {
        MOVPRINT(STR_NO_PLAYER_CLOSE);
        OSPanic(STR_FILE, 0x1AA, STR_ASSERT_ZERO);
        return;
    }
    switch (player->state) {
    case 1:
    case 3:
        player->player_handle->interface->pause(player->player_handle, 0);
        /* fall through */
    case 2:
        stopMoviePlay(player);
        player->state = 0;
        break;
    case 0:
    case 4:
        break;
    default:
        MOVPRINT(STR_UNKNOWN_STATE);
        OSPanic(STR_FILE, 0x1C0, STR_ASSERT_ZERO);
        break;
    }
}

extern "C" void mwMovieUnPauseMovie(_mwMovPlayer* player)
{
    if (player == 0) {
        MOVPRINT(STR_NO_PLAYER_CLOSE);
        OSPanic(STR_FILE, 0x1F7, STR_ASSERT_ZERO);
        return;
    }
    switch (player->state) {
    case 0:
    case 1:
    case 2:
        break;
    case 3:
        player->player_handle->interface->pause(player->player_handle, 0);
        player->state = 2;
        break;
    case 4:
        MOVPRINT(STR_STOPPING_PHASE);
        break;
    default:
        MOVPRINT(STR_UNKNOWN_STATE);
        OSPanic(STR_FILE, 0x20D, STR_ASSERT_ZERO);
        break;
    }
}

extern "C" int mwMoviePlayTick(_mwMovPlayer* player)
{
    int finished = 0;

    if (player == 0) {
        MOVPRINT(STR_NO_PLAYER_TICK);
        OSPanic(STR_FILE, 0x21F, STR_ASSERT_ZERO);
        return 1;
    }
    __mwMovie_syncFrame();
    ADXM_ExecMain();
    updatePlayerState(player);

    switch (player->state) {
    case 0:
        finished = 1;
        break;
    case 1:
        finished = 0;
        break;
    case 2:
    case 3:
        executeMovieFrame(player);
        if (MoviePlayerSetup.tapout != 0 && MoviePlayerSetup.tapout() == 1) {
            player->state = 4;
            player->fade_frame = 0;
        }
        break;
    case 4:
        if (player->fade_frame < player->create.fade_frames) {
            float volume;
            int decibels;
            player->fade_frame++;
            volume = mwMovieGetVolume();
            volume -= volume * player->fade_frame / player->create.fade_frames;
            decibels = mwMovieDbVolFromLinear(volume);
            player->player_handle->interface->set_volume(
                player->player_handle, decibels);
        } else {
            finished = 1;
        }
        break;
    default:
        MOVPRINT(STR_UNKNOWN_STATE);
        OSPanic(STR_FILE, 0x260, STR_ASSERT_ZERO);
        finished = 1;
        break;
    }
    if (finished == 1) {
        stopMoviePlay(player);
        player->state = 0;
    }
    return finished;
}

static void errorCallback(void*, char* message)
{
    MOVPRINT(STR_CRI_ERROR, message);
    MoviePlayerSetup.cri_error = 1;
}

static void initSofdec(void)
{
    MwsInitParam params;

    memset(&params, 0, sizeof(params));
    ADXM_SetupThrd(0);
    params.frame_rate = MoviePlayerSetup.refresh_rate;
    params.maximum_width = 1;
    params.decoder_count = 1;
    params.field_0C = 0;
    ADXM_SetCbErr(errorCallback, 0);
    mwPlyInitSfdFx(&params);
}

static void shutdownSofdec(void)
{
    mwPlyFinishSfdFx();
    ADXM_ShutdownThrd();
}

static void createSofdecPlayer(_mwMovPlayer* player)
{
    MwsCreateParams params;
    int decibels;

    memset(&params, 0, sizeof(params));
    if (player == 0) {
        OSPanic(STR_FILE, 0x2AE, STR_ASSERT_PLAYER);
    }
    if (player->player_handle != 0) {
        MOVPRINT(STR_ALREADY_CREATED);
        OSPanic(STR_FILE, 0x2B4, STR_ASSERT_ZERO);
        return;
    }
    __mwMovie_startVideo(player);
    params.file_type = 1;
    params.maximum_bps = player->create.maximum_bps;
    params.frame_count = player->create.frame_count;
    params.width = player->create.width;
    params.height = player->create.height;
    params.decoder_count = 1;
    if (player->create.composition_flag == 1) {
        params.height *= 2;
        params.composition_mode = 0x21;
    } else {
        params.composition_mode = 0;
    }
    params.work = 0;
    params.work_size = 0;

    mwPlySetMallocFn(mallocCallback, freeCallback, 0);
    player->player_handle = mwPlyCreateSofdec(&params);
    if (player->player_handle == 0) {
        MOVPRINT(STR_CREATE_FAILED);
        OSPanic(STR_FILE, 0x2DA, STR_ASSERT_ZERO);
        return;
    }
    mwPlySetAudioCh(player->player_handle, player->create.audio_channel);
    decibels = mwMovieDbVolFromLinear(MoviePlayerSetup.volume);
    player->player_handle->interface->set_volume(
        player->player_handle, decibels);
    mwPlySetSubtitleCh(player->player_handle, 0);
    mwPlySetSyncMode(player->player_handle, 2);
    mwPlySetFrmSync(player->player_handle, 1);
}

static void destroySofdecPlayer(_mwMovPlayer* player)
{
    if (player == 0) {
        OSPanic(STR_FILE, 0x2F5, STR_ASSERT_PLAYER);
    }
    if (player->player_handle == 0) {
        MOVPRINT(STR_NO_PLAYER_DESTROY);
        OSPanic(STR_FILE, 0x2FA, STR_ASSERT_ZERO);
        return;
    }
    player->player_handle->interface->destroy(player->player_handle);
    player->player_handle = 0;
}

static void startMoviePlay(_mwMovPlayer* player, char* fileName, bool loop)
{
    char fullPath[256];
    char* cursor = fullPath;

    strcpy(cursor, MoviePlayerSetup.path);
    cursor += strlen(MoviePlayerSetup.path);
    strcpy(cursor, fileName);
    fileName = fullPath;
    if (player == 0) {
        OSPanic(STR_FILE, 0x312, STR_ASSERT_PLAYER);
    }
    if (player->player_handle == 0) {
        OSPanic(STR_FILE, 0x313, STR_ASSERT_HANDLE);
    }
    if (fileName == 0) {
        OSPanic(STR_FILE, 0x314, STR_ASSERT_FILENAME);
    }
    player->previous_frame = -1;
    if (loop == true) {
        mwPlyStartFnameLp(player->player_handle, fileName);
    } else {
        player->player_handle->interface->start_filename(player->player_handle,
                                                          fileName);
    }
}

static void stopMoviePlay(_mwMovPlayer* player)
{
    if (player == 0) {
        OSPanic(STR_FILE, 0x328, STR_ASSERT_PLAYER);
    }
    if (player->player_handle == 0) {
        OSPanic(STR_FILE, 0x329, STR_ASSERT_HANDLE);
    }
    player->player_handle->interface->stop(player->player_handle);
}

static int executeMovieFrame(_mwMovPlayer* player)
{
    MwsFrameOutput frame;
    MwsTransportPair* destination;
    MwsTransportPair* source;
    int pairs_remaining;

    if (player == 0) {
        OSPanic(STR_FILE, 0x333, STR_ASSERT_PLAYER);
    }
    if (player->player_handle == 0) {
        OSPanic(STR_FILE, 0x334, STR_ASSERT_HANDLE);
    }
    mwPlyGetCurFrm(player->player_handle, &frame);
    if (frame.frame != 0) {
        player->frame.frame = frame.frame;
        player->frame.frame_structure = frame.frame_structure;
        player->frame.width = frame.width;
        player->frame.height = frame.height;
        player->frame.macroblocks_per_row = frame.macroblocks_per_row;
        player->frame.macroblock_rows = frame.macroblock_rows;
        player->frame.picture_type = frame.picture_type;
        player->frame.frame_rate = frame.frame_rate;
        player->frame.display_time = frame.display_time;
        player->frame.display_time_source = frame.display_time_source;
        player->frame.display_scale = frame.display_scale;
        player->frame.picture_order = frame.picture_order;
        player->frame.presentation_time = frame.presentation_time;
        player->frame.presentation_source = frame.presentation_source;
        player->frame.field_38 = frame.field_38;
        player->frame.field_3C = frame.field_3C;
        player->frame.picture_user_data = frame.picture_user_data;
        player->frame.picture_user_size = frame.picture_user_size;
        player->frame.display_mode = frame.display_mode;
        player->frame.reserved_4C = frame.reserved_4C;
        destination = player->frame.transport.pairs;
        source = frame.transport.pairs;
        pairs_remaining = 7;
        do {
            *destination++ = *source++;
        } while (--pairs_remaining != 0);
        if (frame.picture_order - player->previous_frame != 1) {
            MOVPRINT(STR_DROPPED_FRAMES, player->previous_frame,
                     frame.picture_order);
        }
        player->previous_frame = frame.picture_order;
        displayMovieFrame(player);
        mwPlyRelCurFrm(player->player_handle);
    }
    return 0;
}

static void updatePlayerState(_mwMovPlayer* player)
{
    int status;

    if (player == 0) {
        OSPanic(STR_FILE, 0x358, STR_ASSERT_PLAYER_REVERSED);
    }
    if (player->player_handle == 0) {
        OSPanic(STR_FILE, 0x359, STR_ASSERT_HANDLE_REVERSED);
    }
    if (MoviePlayerSetup.cri_error == 1) {
        MoviePlayerSetup.cri_error = 0;
        player->state = 0;
        return;
    }
    if (player->state == 4) {
        return;
    }

    switch (DVDGetDriveStatus()) {
    case -1:
    case 4:
    case 5:
    case 6:
    case 11:
        player->player_handle->interface->set_volume(player->player_handle,
                                                      -904);
        if (MoviePlayerSetup.disc_error != 0) {
            MoviePlayerSetup.disc_error();
        }
        player->state = 0;
        return;
    default:
        break;
    }

    status = player->player_handle->interface->get_status(player->player_handle);
    switch (status) {
    case 0:
    case 3:
        player->state = 0;
        break;
    case 1:
        player->state = 1;
        break;
    case 2:
        if (player->state == 1) {
            player->player_handle->interface->pause(player->player_handle, 0);
        }
        player->state = 2;
        break;
    case 4:
        MOVPRINT(STR_INTERNAL_ERROR);
        OSPanic(STR_FILE, 0x396, STR_ASSERT_ZERO);
        break;
    default:
        MOVPRINT(STR_UNKNOWN_CRI_STATUS);
        OSPanic(STR_FILE, 0x39B, STR_ASSERT_ZERO);
        break;
    }
}

static void* mallocCallback(void*, unsigned int size)
{
    void* memory = mwMovMalloc(size);

    if (memory != 0) {
        MOVPRINT(STR_ALLOCATED, (float)size * 0.0009765625f, memory);
    } else {
        MOVPRINT(STR_ALLOCATION_FAILED, (float)size * 0.0009765625f);
    }
    return memory;
}

static void freeCallback(void*, void* memory)
{
    MOVPRINT(STR_FREEING, memory);
    mwMovFree(memory);
}

void MOVPRINT(const char* format, ...)
{
    MkVaListState arguments;

    __builtin_va_info(&arguments);
    vsprintf(PrintBuff, format, &arguments);
    mwMovLog(PrintBuff);
}
