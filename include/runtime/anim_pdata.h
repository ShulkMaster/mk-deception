#ifndef MKD_ANIM_PDATA_H
#define MKD_ANIM_PDATA_H

#include "math/gxVect.h"
#include "runtime/mk_proc.h"

typedef struct AnimScript AnimScript;
typedef struct AnimEntryName AnimEntryName;
typedef struct AniData AniData;
typedef struct MkObj MkObj;
typedef struct PlyrPdata PlyrPdata;
typedef struct MkProc MkProc;

/*
 * Common animation process data prefix.
 * The current frame and high frame are used by player and script helpers.
 */
typedef struct AnimPdata {
    MkHdr hdr;                       /* +0x00 */
    MkProc* proc;                    /* +0x08 */
    unsigned int proc_instance;      /* +0x0C */
    MkObj* obj;                      /* +0x10 */
    unsigned int obj_instance;       /* +0x14 */
    PlyrPdata* owner;                /* +0x18 */
    unsigned int owner_instance;     /* +0x1C */
    char pad20[4];
    union {
        unsigned int last_exec_tick;
        unsigned int last_update_tick; /* +0x24 - pose_anim update latch */
    };
    char pad28[4];
    union {
        unsigned int script_word; /* preserved by player proc teardown */
        AnimEntryName* script_entry;
        AniData* animation;
    }; /* +0x2C */
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float frame;      /* +0x38 */
    float low_frame;  /* +0x3C */
    float high_frame; /* +0x40 */
    float step;       /* +0x44 */
    char pad48[8];
    Vec root_offset; /* +0x50 */
    char pad5C[8];
    float weight;     /* +0x64 */
    float weight_velocity; /* +0x68 */
    char pad6C[4];
    unsigned int old_flags; /* +0x70 - previous animation flags */
    char pad74[0x0C];
    float field_80;
    char pad84[0x24];
    float transition_weight; /* +0xA8 */
    float transition_step; /* +0xAC */
    float transition_accel; /* +0xB0 */
    float hand_transition; /* +0xB4 */
    float transition_target; /* +0xB8 */
    char padBC[4];
    char padC0[0x0C];
    unsigned char* bone_remap; /* +0xCC - auxiliary bone index remapping */
    union {
        AniData* hand_animation;
        MkProcEntryFn hand_script;
    }; /* +0xD0 */
    char padD4[0x20];
    int rest_ticks;   /* +0xF4 - weapon rest-loop countdown */
} AnimPdata;

void set_anim_script(
    AnimPdata* animation, AniData* script, int transition);
int set_anim_script_frame(
    float frame, AnimPdata* animation, AniData* script,
    unsigned int flags);
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

#endif
