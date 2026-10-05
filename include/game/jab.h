#ifndef GAME_JAB_H
#define GAME_JAB_H

#include "math/gxVect.h"

typedef struct MkObj MkObj;
typedef struct LightDef LightDef;

MkObj* jab_spawn_point_light_at_world_pos(LightDef* definition, Vec* position);
void jab_face_obj(MkObj* object, const Vec* direction);

#endif
