#ifndef GAME_BLOOD_H
#define GAME_BLOOD_H

typedef struct Vec Vec;

typedef struct GusherPdata GusherPdata;
typedef struct MkObj MkObj;
typedef struct FighterMirror FighterMirror;

struct BloodVelocityState;
struct BloodSpawnStep;
struct BloodPath;
struct PlyrPdata;



void spawn_bld_splat(const char* name, FighterMirror* owner, Vec* position);

typedef struct GusherStep {
    const char* blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

extern GusherStep heart_beat[];

GusherPdata* start_gusher(
    GusherStep* steps, void* owner, MkObj* object, int bone,
    Vec* position, Vec* direction);

int is_blood_disabled(void);
void gusher_destroy_list(void);
void reset_blood_decals(void);
extern void bleed_startup(void);
void bleed_init(void);
extern void bleed_restart(void);
#endif
