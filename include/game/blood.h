#ifndef GAME_BLOOD_H
#define GAME_BLOOD_H

#include "math/gxVect.h"

typedef struct GusherPdata GusherPdata;
typedef struct MkObj MkObj;
typedef struct FighterMirror FighterMirror;

struct BloodVelocityState;
struct BloodSpawnStep;
struct BloodPath;
struct PlyrPdata;

int obj_spawn_bld(
    MkObj* object, struct BloodVelocityState* previous, int batch_count,
    struct BloodSpawnStep* step, struct BloodPath* path, int point_index,
    const Vec* position, unsigned int art_id, struct PlyrPdata* owner);

void spawn_bld_splat(const char* name, FighterMirror* owner, const Vec* position);

typedef struct GusherStep {
    const char* blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

extern GusherStep heart_beat[];

GusherPdata* start_gusher(
    GusherStep* steps, void* owner, MkObj* object, int bone,
    Vec* position, Vec* direction);

#endif
