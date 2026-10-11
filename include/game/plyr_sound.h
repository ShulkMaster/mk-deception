#ifndef GAME_PLYR_SOUND_H
#define GAME_PLYR_SOUND_H

#include "msl/msl_types.h"

struct PlyrPdata;
MslSoundHandle plyr_snd_req_no_plyr_proc(struct PlyrPdata* fighter, int sound_offset);

#endif
