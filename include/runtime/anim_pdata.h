#ifndef MKD_ANIM_PDATA_H
#define MKD_ANIM_PDATA_H

#include "runtime/anim_types.h"

void set_anim_script(
    AnimPdata* animation, AniData* script, unsigned int flags);
int set_anim_script_frame(
    AnimPdata* animation, AniData* script, unsigned int flags,
    float frame);
int pose_anim(AnimPdata* animation, int update_object);
int advance_anim(AnimPdata* animation);
MkProc* create_mkproc_anim(
    int pid, MkProcEntryFn entry, AnimPdata** pdata_out);
MkProc* create_mkproc_anim2(
    int pid, MkProcEntryFn entry, AnimPdata** pdata_out);
MkProc* create_mkproc_face_anim(
    int pid, MkProcEntryFn entry, AnimPdata** pdata_out);
MkProc* create_mkproc_hand_anim(
    int pid, MkProcEntryFn entry, AnimPdata** pdata_out);
float p_anim_idle(void);
int transition_to_anim_script_frame(
    AnimPdata* animation, AnimScript* script, unsigned int flags,
    float transition_frames, float frame);
AnimPdata* get_mkpdata_anim(void);
void vdestroy_mkpdata_anim(AnimPdata* animation);

#endif
