#ifndef GAME_FX_H
#define GAME_FX_H

#include "game/moveset.h"

typedef struct FxHdrLatch {
    MkHdr* object;
    unsigned int instance;
} FxHdrLatch;

extern PlyrMirrorObjLatch p1_freeze_light_item;
extern PlyrMirrorObjLatch p2_freeze_light_item;
extern FxHdrLatch p1_freeze_proc_item;
extern FxHdrLatch p2_freeze_proc_item;

#ifdef __cplusplus
extern "C" {
#endif

void kill_all_fstyle_signs(void);

#ifdef __cplusplus
}
#endif

#endif
