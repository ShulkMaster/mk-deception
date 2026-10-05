/* BUILD: retail object compiled with -opt nopeephole -schedule off -str reuse,pool:
 * prologue LR saves stay ahead of the body and the assertion strings share one pool. */
#include "cri/adx_sugc.h"
#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "movie/mwMovie.h"
#include "movie/mwMovie_platform.h"

typedef struct MwsPlayer MwsPlayer;

struct MwsTransportPair {
    int first;
    int second;
};

struct MwsTransportFrameInfo {
    MwsTransportPair pairs[7];
};

struct MwsFrameOutput {
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
};

struct _mwMovPlayer {
    MwsPlayer* player_handle;
    int reserved_04;
    MwsFrameOutput frame;
    int state;
    MwMovieCreateParams create;
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

typedef char MwsFrameOutputSizeCheck[sizeof(MwsFrameOutput) == 0x88 ? 1 : -1];
typedef char MwMoviePlayerSizeCheck[sizeof(_mwMovPlayer) == 0xB8 ? 1 : -1];
typedef char MwMovieSetupSizeCheck[sizeof(mwMovieSetup) == 0x30 ? 1 : -1];

extern mwMovieSetup MoviePlayerSetup;

extern "C" {
void __mwMovie_startVideo(_mwMovPlayer* player);
void displayMovieFrame(_mwMovPlayer* player);
}


static int mwMovie_video_initialized;

extern "C" void __mwMovie_initVideo(void)
{
}

extern "C" void __mwMovie_startVideo(_mwMovPlayer* player)
{
    if (MoviePlayerSetup.start != 0) {
        MoviePlayerSetup.start(player->create.width, player->create.height);
    }
    mwMovie_video_initialized = 1;
}

extern "C" void __mwMovie_shutdownVideo(void)
{
    if (MoviePlayerSetup.stop != 0) {
        MoviePlayerSetup.stop();
    }
    mwMovie_video_initialized = 0;
}

extern "C" void __mwMovie_syncFrame(void)
{
    VIWaitForRetrace();
}

extern "C" void displayMovieFrame(_mwMovPlayer* player)
{
    if (player == 0) {
        OSPanic("mwMovieGC.cpp", 0x93, "Assertion failure: player != 0L");
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
