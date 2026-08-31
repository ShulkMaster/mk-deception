#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "movie/mwMovie.h"
#include "movie/mwMovie_platform.h"

typedef struct MwsPlayer MwsPlayer;

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

typedef struct MwMovieVideoState {
    int initialized;
    int reserved;
} MwMovieVideoState;

typedef char MwsFrameOutputSizeCheck[sizeof(MwsFrameOutput) == 0x88 ? 1 : -1];
typedef char MwMoviePlayerSizeCheck[sizeof(_mwMovPlayer) == 0xB8 ? 1 : -1];
typedef char MwMovieSetupSizeCheck[sizeof(mwMovieSetup) == 0x30 ? 1 : -1];
typedef char MwMovieVideoStateSizeCheck[
    sizeof(MwMovieVideoState) == 0x8 ? 1 : -1];

extern mwMovieSetup MoviePlayerSetup;

extern "C" {
void ADXGC_SetupDvdFs(const int* read_mode);
void __mwMovie_startVideo(_mwMovPlayer* player);
void displayMovieFrame(_mwMovPlayer* player);
}

namespace {
/* Retail combines both diagnostics and retains two trailing alignment bytes. */
char stringBase0[] =
    "mwMovieGC.cpp\0"
    "Assertion failure: player != 0L\0\0";
}

/* The second word is the retail .sbss alignment tail. */
static MwMovieVideoState mwMovie_video_initialized;

extern "C" void __mwMovie_initVideo(void)
{
}

extern "C" void __mwMovie_startVideo(_mwMovPlayer* player)
{
    if (MoviePlayerSetup.start != 0) {
        MoviePlayerSetup.start(player->create.width, player->create.height);
    }
    mwMovie_video_initialized.initialized = 1;
}

extern "C" void __mwMovie_shutdownVideo(void)
{
    if (MoviePlayerSetup.stop != 0) {
        MoviePlayerSetup.stop();
    }
    mwMovie_video_initialized.initialized = 0;
}

extern "C" void __mwMovie_syncFrame(void)
{
    VIWaitForRetrace();
}

extern "C" void displayMovieFrame(_mwMovPlayer* player)
{
    if (player == 0) {
        OSPanic(stringBase0, 0x93, &stringBase0[0xE]);
    }
    if (MoviePlayerSetup.process != 0) {
        MoviePlayerSetup.process(player, player->frame.frame,
                                 player->frame.width, player->frame.height,
                                 player->create.output_width,
                                 player->create.output_height, 0);
    }
}

extern "C" void initADXwithPC(const char* path, int audio_enable)
{
    initADXwithDVD(path, audio_enable);
}

extern "C" void initADXwithMEM(int)
{
}

extern "C" void initADXwithDVD(const char*, int)
{
    ADXGC_SetupDvdFs(0);
}

extern "C" void shutdownADX(void)
{
}
