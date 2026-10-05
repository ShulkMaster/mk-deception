#ifndef RUNTIME_ANIM_TRANSITION_H
#define RUNTIME_ANIM_TRANSITION_H

#include "runtime/anim_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void transition_to_anim_script(
    float transition_frames, AnimPdata* anim, AnimScript* script, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
