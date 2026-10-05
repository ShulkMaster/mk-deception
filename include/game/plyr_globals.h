#ifndef GAME_PLYR_GLOBALS_H
#define GAME_PLYR_GLOBALS_H

#include "runtime/plyr_info.h"

struct MkObj;

#ifdef __cplusplus
extern "C" {
#endif

extern GlobalPlayerEntry global_player_data[44];

extern struct MkObj* plyr_obj;
extern struct MkObj* his_obj;

#ifdef __cplusplus
}
#endif

#endif
