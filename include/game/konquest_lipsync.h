#ifndef GAME_KONQUEST_LIPSYNC_H
#define GAME_KONQUEST_LIPSYNC_H

#include "game/konquest.h"
#include "runtime/image.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_proc.h"

typedef struct AnimScript AnimScript;
typedef struct KonquestNpc KonquestNpc;

typedef struct KonquestLipSyncPdata {
    MkHdr hdr;
    int mode;
    KonquestNpc* npc;
    union {
        AniTextureControl* texture;
        AnimPdata* animation;
        AnimPdata* player_animation;
    };
    union {
        unsigned int texture_instance;
        AnimScript** animation_table;
    };
    LipSyncKeyframe* keyframes;
    unsigned int sound_handle;
    float elapsed;
    int stop_requested;
} KonquestLipSyncPdata;

#endif
