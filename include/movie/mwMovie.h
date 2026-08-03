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
    void* buffer_bytes;
    int reserved0;
    void* create_flag;
    short width;
    short height;
    short width2;
    short height2;
    short const_one;
    short const_four;
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
