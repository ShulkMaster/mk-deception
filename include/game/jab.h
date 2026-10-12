#ifndef GAME_JAB_H
#define GAME_JAB_H

#include "math/gxVect.h"

typedef struct MkObj MkObj;
typedef struct LightDef LightDef;

MkObj* jab_spawn_point_light_at_world_pos(LightDef* definition, Vec* position);
void jab_face_obj(MkObj* object, Vec* direction);
void jab_setup_kiss_emitter_obj(MkObj* object);
void obj_scale_over_time(MkObj* object, Vec* target, float ticks);

#endif
