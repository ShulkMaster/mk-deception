#ifndef RUNTIME_MK_OBJ_BONE_H
#define RUNTIME_MK_OBJ_BONE_H

#include "math/gxVect.h"

typedef struct MkObj MkObj;

#ifdef __cplusplus
extern "C" {
#endif

void get_bone_world_pos(MkObj* obj, int bone, Vec* out);
void update_bone_hierarchy(void* obj);
void calc_bone_world_mat(MkObj* obj, int bone);
void obj_set_bone_calc_world_mat_flag(MkObj* obj, int bone);

void get_bone_offset_world_pos(
    MkObj* obj, int bone, Vec* offset, Vec* out);

#ifdef __cplusplus
}
#endif

#endif
