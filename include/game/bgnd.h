#ifndef GAME_BGND_H
#define GAME_BGND_H

#include "game/bgnd_types.h"

typedef struct MkObj MkObj;
typedef struct PlyrPdata PlyrPdata;

typedef struct LoadBgndCtx {
    int art_id;
    MkObj* bgnd_obj;
    union {
        void* field_08;
        int pad;
    };
} LoadBgndCtx;

#ifdef __cplusplus
extern "C" {
#endif

/* Critical Krypt / background entry points (Wave 2 NonMatching scaffold). */

void bgnd_anim_camera_ended(void);
void bgnd_anim_camera_setup(void);
void bgnd_clear_danger_zone_callback(PlyrPdata* pdata);
int is_bgnd_locked(int bgnd_id);
int load_background(int bgnd_id);

#ifdef __cplusplus
}
#endif

#endif /* GAME_BGND_H */
