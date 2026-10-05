#ifndef MOVIE_MWMOVIE_H
#define MOVIE_MWMOVIE_H

typedef struct _mwMovPlayer _mwMovPlayer;

typedef struct MwMovieInitParams {
    int display_mode;
    int source;
    const char* path;
    int audio_enable;
    float volume;
    void* tapout;
    void (*start)(void);
    void (*stop)(void);
    void (*vsync)(void);
    void (*process)(void* context, int unused, int width, int height);
    void (*disc_error)(void);
} MwMovieInitParams;

typedef struct MwMovieCreateParams {
    unsigned int maximum_bps;
    int audio_channel;
    unsigned int composition_flag;
    unsigned short width;
    unsigned short height;
    unsigned short output_width;
    unsigned short output_height;
    unsigned short frame_count;
    unsigned short fade_frames;
} MwMovieCreateParams;

#ifdef __cplusplus
extern "C" {
#endif

void mwMovieSetTapoutCallback(void* callback);
void mwMovieSetMovieVolume(void* handle, float volume);
int mwMovieInit(MwMovieInitParams* params);
void mwMovieShutDown(void);
float mwMovieGetVolume(void);
_mwMovPlayer* mwMovieCreatePlayer(MwMovieCreateParams* params);
void mwMovieDestroyPlayer(_mwMovPlayer* player);
void mwMovieStartPlayback(_mwMovPlayer* player, const char* path);
void mwMovieStartPlaybackLooping(_mwMovPlayer* player, const char* path);
void mwMovieStopPlayback(_mwMovPlayer* player);
void mwMovieUnPauseMovie(_mwMovPlayer* player);
int mwMoviePlayTick(_mwMovPlayer* player);

#ifdef __cplusplus
}
#endif

#endif
