#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "math/gxVect.h"

typedef struct ProjectileImpaleInfo {
    int parent_bone;
    Vec parent_offset;
    int child_bone;
    Vec child_offset;
    Vec rotation;
} ProjectileImpaleInfo;

void set_active_projectile_impale_info(
    ProjectileImpaleInfo* info, const int* bone_tags);

#endif
