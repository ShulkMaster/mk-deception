#include "movie/MkMovies.h"

#include "movie/MovieConfig.h"
#include "movie/MovieManager.h"

#include "mw/mwMemHeap.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "rw/rwcore_types.h"

extern "C" {

int GetArtSlot__Fv(void);
#include "runtime/asset.h"
RwTexture* GetScreenPolyTexture__FPv(void* screen_poly);
void SetScreenPolyTexture__FPvP9RwTexture(void* screen_poly, RwTexture* texture);

static const char stringBase0[] = "MFS:%08x.%08x\0%s";

#define STR_MFS_PATH_FMT (&stringBase0[0])
#define STR_NAME_PATH_FMT (&stringBase0[0xE])

static MkMovieTexPlayer _mmp_data[2] = {0};

static inline void* mmp_screen_poly_at(MkMovieTexPlayer* player, int screen_offset) {
    screen_offset += RW_OFFSET_OF(MkMovieTexPlayer, screen_poly);
    return *(void**)((char*)player + screen_offset);
}

static inline RwTexture* mmp_saved_texture_at(MkMovieTexPlayer* player, int screen_offset) {
    screen_offset += RW_OFFSET_OF(MkMovieTexPlayer, saved_texture);
    return *(RwTexture**)((char*)player + screen_offset);
}

void mkMovieTexPlayerIdleUpdate(void) {
    int index;
    unsigned char anyPlaying;

    anyPlaying = 0;
    for (index = 0; index < 2; index++) {
        if (_mmp_data[index].movie != 0 && MovieIsPlaying(_mmp_data[index].movie) != 0) {
            anyPlaying = 1;
        }
    }
    if (anyPlaying == 1) {
        for (index = 0; index < 2; index++) {
            if (_mmp_data[index].movie != 0) {
                MovieUpdate(_mmp_data[index].movie);
            }
        }
    }
}

static inline void mmp_release_player(MkMovieTexPlayer* player) {
    int screenCount;
    int screenIndex;
    void* screenPoly;
    RwTexture* savedTex;
    int screenOffset;

    MovieDelete(player->movie);
    screenCount = player->screen_count;
    if (screenCount < 1) {
        screenCount = 1;
    }
    for (screenIndex = 0, screenOffset = 0; screenIndex < screenCount;
         screenIndex++, screenOffset += sizeof(void*)) {
        screenPoly = *(void**)((char*)player +
            RW_OFFSET_OF(MkMovieTexPlayer, screen_poly) + screenOffset);
        savedTex = *(RwTexture**)((char*)player +
            RW_OFFSET_OF(MkMovieTexPlayer, saved_texture) + screenOffset);
        RwTexture* tex = GetScreenPolyTexture__FPv(screenPoly);
        if (tex != savedTex) {
            SetScreenPolyTexture__FPvP9RwTexture(screenPoly, savedTex);
        }
    }
    MovieDeleteTexture(player->texture);
    memset(player, 0, sizeof(*player));
}

void movie_player_reset(void) {
    MkMovieTexPlayer* player;
    int playerIndex;

    playerIndex = 0;
    do {
        if (playerIndex < 2) {
            player = &_mmp_data[playerIndex];
            if (player->movie != 0) {
                mmp_release_player(player);
            }
        }
        playerIndex++;
    } while (playerIndex < 2);
    MovieShutdownSystem();
}

void mkMovieTexStop(int index) {
    MkMovieTexPlayer* player;

    if (index < 2) {
        player = &_mmp_data[index];
        if (player->movie != 0) {
            MovieStop(player->movie);
        }
    }
}

static inline MkMovieTexPlayer* mmp_prepare_movie_path(
    MkMovieTexPlayer* player, const char* name, int use_mfs) {
    void* block;
    int block_size;

    if (use_mfs != 0) {
        block = load_named_binary_block(GetArtSlot__Fv(), name, &block_size);
        if (block != 0) {
            sprintf(player->path, STR_MFS_PATH_FMT, block, block_size);
        } else {
            sprintf(player->path, STR_NAME_PATH_FMT, name);
            return 0;
        }
    } else {
        sprintf(player->path, STR_NAME_PATH_FMT, name);
    }
    return player;
}

void mkMovieTexPlay(int index, const char* name, int unused1, int unused2, int unused3, int use_mfs) {
    int screenCount;
    int screenIndex;
    void* screenPoly;
    MkMovieTexPlayer* player;
    RwTexture* tex;
    RwTexture* texture;
    MkMovieTexPlayer* playable;

    if (index < 2) {
        player = &_mmp_data[index];
        if (player->movie != 0) {
            screenCount = player->screen_count;
            if (screenCount < 1) {
                screenCount = 1;
            }
            screenIndex = 0;
            while (screenIndex < screenCount) {
                screenPoly = (&player->screen_poly)[screenIndex];
                if (screenPoly != 0) {
                    texture = player->texture;
                    tex = GetScreenPolyTexture__FPv(screenPoly);
                    if (tex != texture) {
                        SetScreenPolyTexture__FPvP9RwTexture(screenPoly, texture);
                    }
                }
                screenIndex++;
            }
            setMovieHeap(movie_heap);
            playable = mmp_prepare_movie_path(player, name, use_mfs);
            if (playable != 0) {
                MoviePlayModeSelect(player->movie, player->path);
            }
        }
    }
}

void mkMovieTexInit(int index, void* screen_poly, int width, int height) {
    MkMovieTexPlayer* player;

    setMovieHeap(permanent_heap);
    if (index < 2) {
        player = &_mmp_data[index];
        if (player->movie == 0) {
            player->texture = MovieNewTexture(width, height);
            player->movie = MovieNewModeSelect(
                player->texture->raster, width, height);
            player->unk28 = -1;
            player->screen_count = 1;
            player->screen_poly = screen_poly;
            player->saved_texture = GetScreenPolyTexture__FPv(screen_poly);
        }
    }
}

}
