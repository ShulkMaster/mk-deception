#ifndef MKD_SOUND_TRACKER_H
#define MKD_SOUND_TRACKER_H

#include "msl/msl_types.h"
#include "math/gxVect.h"
#include "runtime/mk_struct.h"

/*
 * Tracked 3D sound blob from get_sound_tracker_data (get_mkhdr_generic 0x34).
 * Fire-pot / ambient emitters write pose + radii then mk_insert onto a list.
 */

typedef struct TrackedSound {
    MkHdr hdr;             /* +0x00 */
    MslSoundHandle sound_handle; /* +0x08 - MSL list-pool ID */
    int sound_id;          /* +0x0C */
    union {
        Vec pos;           /* +0x10 */
        struct {
            float pos_x;
            float pos_y;
            float pos_z;
        };
    };
    float max_dist;        /* +0x1C */
    float min_dist;        /* +0x20 */
    int out_of_range;      /* +0x24 - sound was stopped outside max_dist */
    int positional_pan;    /* +0x28 - update pan from world position */
    int tracking_enabled;  /* +0x2C - cleared on alloc */
    int owner_uid;         /* +0x30 - owning world object's UID, or zero */
} TrackedSound; /* 0x34 */

typedef struct SoundTrackerPdata {
    MkHdr hdr;             /* +0x00 */
    MkPtr** sound_list;    /* +0x08 - address of the mode-owned list head */
} SoundTrackerPdata; /* 0x0C */

float p_track_sound(void);
TrackedSound* get_sound_tracker_data(void);
void make_new_tracked_sound(MkPtr** list_head, TrackedSound* sound);
void stop_tracked_sound(MkPtr** list_head, TrackedSound* sound);
void stop_sound_tracking_process(MkPtr** sound_list);
void start_sound_tracking_process(MkPtr** sound_list);

#endif
