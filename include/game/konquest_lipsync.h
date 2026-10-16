#ifndef GAME_KONQUEST_LIPSYNC_H
#define GAME_KONQUEST_LIPSYNC_H

#include "game/konquest.h"
#include "runtime/image.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_proc.h"

typedef struct AnimScript AnimScript;
typedef struct KonquestNpc KonquestNpc;

struct KonquestTextureLatch {
    AniTextureControl* texture;
    unsigned int instance;
};

struct KonquestLipSyncAnimation {
    AnimPdata* animation;
    AnimScript** table;
};

typedef struct KonquestLipSyncPdata {
    MkHdr hdr;
    int mode;
    KonquestNpc* npc;
    union {
        struct KonquestTextureLatch texture_latch;
        struct KonquestLipSyncAnimation player;
    };
    LipSyncKeyframe* keyframes;
    unsigned int sound_handle;
    float elapsed;
    int stop_requested;
} KonquestLipSyncPdata;

typedef char KonquestTextureLatchSizeCheck[
    sizeof(struct KonquestTextureLatch) == 0x08 ? 1 : -1];
typedef char KonquestLipSyncAnimationSizeCheck[
    sizeof(struct KonquestLipSyncAnimation) == 0x08 ? 1 : -1];
typedef char KonquestLipSyncPdataSizeCheck[
    sizeof(KonquestLipSyncPdata) == 0x28 ? 1 : -1];

#endif
