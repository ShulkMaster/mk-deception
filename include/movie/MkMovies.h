#ifndef MK_MOVIES_H
#define MK_MOVIES_H

typedef struct RwTexture RwTexture;
typedef struct MoviePlayer MoviePlayer;

typedef struct MkMovieTexPlayer {
    char path[0x20];
    MoviePlayer* movie;
    RwTexture* texture;
    int unk28;
    int screen_count;
    void* screen_poly;
    RwTexture* saved_texture;
} MkMovieTexPlayer;

#ifdef __cplusplus
extern "C" {
#endif

void mkMovieTexPlayerIdleUpdate(void);
void movie_player_reset(void);
void mkMovieTexStop(int index);
void mkMovieTexPlay(int index, const char* name, int unused1, int unused2, int unused3,
                    int use_mfs);
void mkMovieTexInit(int index, void* screen_poly, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
