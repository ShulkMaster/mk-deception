#ifndef RUNTIME_PLYR_ANIM_PDATA_H
#define RUNTIME_PLYR_ANIM_PDATA_H

#include "runtime/plyr_pdata.h"

typedef struct PlyrAnimPdata {
    char pad00[0x2C];
    AniData* current_animation; /* +0x2C */
    unsigned int flags; /* +0x30 */
    char pad34[0x10];
    float playback_rate; /* +0x44 */
    char pad48[0x1C];
    float field_64; /* +0x64 */
    char pad68[0x18];
    float field_80; /* +0x80 */
    char pad84[0x28];
    float field_AC; /* +0xAC */
} PlyrAnimPdata;

extern PlyrAnimPdata* plyr_anim_pdata;

#endif
