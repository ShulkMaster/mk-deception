#ifndef MKD_ANIM_PDATA_H
#define MKD_ANIM_PDATA_H

#include "runtime/mk_proc.h"

typedef struct AnimScript AnimScript;
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
    unsigned int last_exec_tick; /* +0x24 */
    char pad28[4];
    unsigned int script_word; /* +0x2C - preserved by player proc teardown */
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float frame;      /* +0x38 */
    float low_frame;  /* +0x3C */
    float high_frame; /* +0x40 */
    float step;       /* +0x44 */
    char pad48[0x1C];
    float weight;     /* +0x64 */
    float weight_velocity; /* +0x68 */
    char pad6C[0x3C];
    float transition_weight; /* +0xA8 */
    float transition_step; /* +0xAC */
    float transition_accel; /* +0xB0 */
    float hand_transition; /* +0xB4 */
    float transition_target; /* +0xB8 */
    char padBC[4];
    char padC0[0x10];
    MkProcEntryFn hand_script; /* +0xD0 */
    char padD4[0x20];
    int rest_ticks;   /* +0xF4 - weapon rest-loop countdown */
} AnimPdata;

#endif
