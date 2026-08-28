#ifndef GAME_BLOOD_H
#define GAME_BLOOD_H

#include "math/gxVect.h"

typedef struct GusherPdata GusherPdata;
typedef struct MkObj MkObj;

typedef struct GusherStep {
    const char* blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

extern GusherStep heart_beat[];

GusherPdata* start_gusher(
    GusherStep* steps, void* owner, MkObj* object, int bone,
    const Vec* position, const Vec* direction);

#endif
