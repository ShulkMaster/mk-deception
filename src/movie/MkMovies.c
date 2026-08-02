#include "movie/MkMovies.h"

#include "movie/MovieConfig.h"
#include "movie/MovieManager.h"

#include "mw/mwMemHeap.h"

void* GetArtSlot__Fv(void);
#include "runtime/asset.h"
void* GetScreenPolyTexture__FPv(void* screen_poly);
void SetScreenPolyTexture__FPvP9RwTexture(void* screen_poly, RwTexture* texture);
int sprintf(char* dest, const char* fmt, ...);
void memset(void* dest, int val, int size);

typedef struct RwRaster RwRaster;

typedef struct RwTextureWithRaster {
    RwRaster* raster;
} RwTextureWithRaster;

static const char stringBase0[] = "MFS:%08x.%08x\0%s";

#define STR_MFS_PATH_FMT (&stringBase0[0])
#define STR_NAME_PATH_FMT (&stringBase0[0xE])

static MkMovieTexPlayer _mmp_data[2];

/* Retail packs screen_poly / saved_texture pairs from offset 0x30 (stride 4). */
static void* mmp_screen_poly_at(MkMovieTexPlayer* player, int screen_offset) {
    return *(void**)((char*)player + screen_offset + 0x30);
}

static RwTexture* mmp_saved_texture_at(MkMovieTexPlayer* player, int screen_offset) {
    return *(RwTexture**)((char*)player + screen_offset + 0x34);
}

void mkMovieTexPlayerIdleUpdate(void) {
    int anyPlaying;
    int index;
    MkMovieTexPlayer* player;

    anyPlaying = 0;
    for (index = 0; index < 2; index++) {
        player = &_mmp_data[index];
        if (player->movie != 0 && MovieIsPlaying(player->movie) != 0) {
            anyPlaying = 1;
        }
    }
    if ((anyPlaying & 0xFF) == 1) {
        for (index = 0; index < 2; index++) {
            player = &_mmp_data[index];
            if (player->movie != 0) {
                MovieUpdate(player->movie);
            }
        }
    }
}

void movie_player_reset(void) {
    int playerIndex;
    int byteOffset;
    MkMovieTexPlayer* player;
    int screenCount;
    int screenIndex;
    int screenOffset;
    void* screenPoly;
    RwTexture* savedTex;
    RwTexture* tex;

    playerIndex = 0;
    byteOffset = 0;
    do {
        if (playerIndex < 2) {
            player = (MkMovieTexPlayer*)((char*)_mmp_data + byteOffset);
            if (player->movie != 0) {
                MovieDelete(player->movie);
                screenCount = player->screen_count;
                if (screenCount < 1) {
                    screenCount = 1;
                }
                for (screenIndex = 0, screenOffset = 0; screenIndex < screenCount;
                     screenIndex++, screenOffset += 4) {
                    screenPoly = mmp_screen_poly_at(player, screenOffset);
                    savedTex = mmp_saved_texture_at(player, screenOffset);
                    tex = GetScreenPolyTexture__FPv(screenPoly);
                    if (tex != savedTex) {
                        SetScreenPolyTexture__FPvP9RwTexture(screenPoly, savedTex);
                    }
                }
                MovieDeleteTexture(player->texture);
                memset(player, 0, 0x38);
            }
        }
        playerIndex++;
        byteOffset += 0x38;
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

void mkMovieTexPlay(int index, const char* name, int unused1, int unused2, int unused3, int use_mfs) {
    MkMovieTexPlayer* player;
    RwTexture* tex;
    int screenIndex;
    int screenCount;
    int screenOffset;
    void* screenPoly;
    RwTexture* texture;
    void* block;
    int block_size;
    int can_play;

    (void)unused1;
    (void)unused2;
    (void)unused3;
    if (index < 2) {
        player = &_mmp_data[index];
        if (player->movie != 0) {
            screenCount = player->screen_count;
            if (screenCount < 1) {
                screenCount = 1;
            }
            screenIndex = 0;
            screenOffset = 0;
            do {
                screenPoly = mmp_screen_poly_at(player, screenOffset);
                if (screenPoly != 0) {
                    texture = player->texture;
                    tex = GetScreenPolyTexture__FPv(screenPoly);
                    if (tex != texture) {
                        SetScreenPolyTexture__FPvP9RwTexture(screenPoly, texture);
                    }
                }
                screenIndex++;
                screenOffset += 4;
            } while (screenIndex < screenCount);
            setMovieHeap__FP10_mwMemHeap(movie_heap);
            can_play = 1;
            if (use_mfs == 0) {
                sprintf(player->path, STR_NAME_PATH_FMT, name);
            } else {
                block = load_named_binary_block((int)GetArtSlot__Fv(), (char*)name, &block_size);
                if (block == 0) {
                    sprintf(player->path, STR_NAME_PATH_FMT, name);
                    can_play = 0;
                } else {
                    sprintf(player->path, STR_MFS_PATH_FMT, block, block_size);
                }
            }
            if (can_play != 0) {
                MoviePlayModeSelect(player->movie, player->path);
            }
        }
    }
}

void mkMovieTexInit(int index, void* screen_poly, int width, int height) {
    MkMovieTexPlayer* player;
    RwTexture* tex;
    void* movie;
    RwTexture* saved;

    setMovieHeap__FP10_mwMemHeap(permanent_heap);
    if (index < 2) {
        player = &_mmp_data[index];
        if (player->movie == 0) {
            tex = MovieNewTexture(width, height);
            player->texture = tex;
            movie =
                MovieNewModeSelect(((RwTextureWithRaster*)tex)->raster, width, height);
            player->movie = movie;
            player->unk28 = -1;
            player->screen_count = 1;
            player->screen_poly = screen_poly;
            saved = GetScreenPolyTexture__FPv(screen_poly);
            player->saved_texture = saved;
        }
    }
}
