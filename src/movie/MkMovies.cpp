#include "movie/MkMovies.h"

#include "movie/MovieConfig.h"
#include "movie/MovieManager.h"

#include "mw/mwMemHeap.h"
#include "runtime/cstring.h"

extern "C" {

int GetArtSlot__Fv(void);
#include "runtime/asset.h"
RwTexture* GetScreenPolyTexture__FPv(void* screen_poly);
void SetScreenPolyTexture__FPvP9RwTexture(void* screen_poly, RwTexture* texture);
int sprintf(char* dest, const char* fmt, ...);

typedef struct RwRaster RwRaster;

typedef struct RwTextureWithRaster {
    RwRaster* raster;
} RwTextureWithRaster;

static const char stringBase0[] = "MFS:%08x.%08x\0%s";

#define STR_MFS_PATH_FMT (&stringBase0[0])
#define STR_NAME_PATH_FMT (&stringBase0[0xE])

static MkMovieTexPlayer _mmp_data[2] = {0};

/* Retail packs screen_poly / saved_texture pairs from offset 0x30 (stride 4). */
static inline void* mmp_screen_poly_at(MkMovieTexPlayer* player, int screen_offset) {
    return *(void**)((char*)&player->screen_poly + screen_offset);
}

static inline RwTexture* mmp_saved_texture_at(MkMovieTexPlayer* player, int screen_offset) {
    return *(RwTexture**)((char*)&player->saved_texture + screen_offset);
}

void mkMovieTexPlayerIdleUpdate(void) {
    unsigned char anyPlaying;
    int index;
    int byteOffset;
    MkMovieTexPlayer* player;

    anyPlaying = 0;
    byteOffset = 0;
    for (index = 0; index < 2; index++, byteOffset += sizeof(MkMovieTexPlayer)) {
        player = (MkMovieTexPlayer*)((char*)_mmp_data + byteOffset);
        if (player->movie != 0 && MovieIsPlaying(player->movie) != 0) {
            anyPlaying = 1;
        }
    }
    if (anyPlaying == 1) {
        byteOffset = 0;
        for (index = 0; index < 2; index++, byteOffset += sizeof(MkMovieTexPlayer)) {
            player = (MkMovieTexPlayer*)((char*)_mmp_data + byteOffset);
            if (player->movie != 0) {
                MovieUpdate(player->movie);
            }
        }
    }
}

/* TODO: [breakthrough] 86.02%; structure-derived reset offsets retain retail layout;
 * remaining loop/save scheduling needs caller and lifetime analysis. */
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
                     screenIndex++, screenOffset += sizeof(void*)) {
                    screenPoly = mmp_screen_poly_at(player, screenOffset);
                    savedTex = mmp_saved_texture_at(player, screenOffset);
                    tex = GetScreenPolyTexture__FPv(screenPoly);
                    if (tex != savedTex) {
                        SetScreenPolyTexture__FPvP9RwTexture(screenPoly, savedTex);
                    }
                }
                MovieDeleteTexture(player->texture);
                memset(player, 0, sizeof(*player));
            }
        }
        playerIndex++;
        byteOffset += sizeof(MkMovieTexPlayer);
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

/* TODO: [breakthrough] 75.00%; typed binding offsets preserve native ownership;
 * remaining path/loop register scheduling needs retail CFG analysis. */
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
    MkMovieTexPlayer* playable;

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
                screenOffset += sizeof(void*);
            } while (screenIndex < screenCount);
            setMovieHeap(movie_heap);
            playable = player;
            if (use_mfs == 0) {
                sprintf(player->path, STR_NAME_PATH_FMT, name);
            } else {
                block = load_named_binary_block((int)GetArtSlot__Fv(), (char*)name, &block_size);
                if (block == 0) {
                    sprintf(player->path, STR_NAME_PATH_FMT, name);
                    playable = 0;
                } else {
                    sprintf(player->path, STR_MFS_PATH_FMT, block, block_size);
                }
            }
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
                ((RwTextureWithRaster*)player->texture)->raster, width, height);
            player->unk28 = -1;
            player->screen_count = 1;
            player->screen_poly = screen_poly;
            player->saved_texture = GetScreenPolyTexture__FPv(screen_poly);
        }
    }
}

}
