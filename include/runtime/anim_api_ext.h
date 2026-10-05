#ifndef MKD_RUNTIME_ANIM_API_EXT_H
#define MKD_RUNTIME_ANIM_API_EXT_H

#include "runtime/anim_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* get_animation(int animation_id);
int build_bones_tbl(MkObj* object, const int* bone_tags);

#ifdef __cplusplus
}
#endif

#endif
