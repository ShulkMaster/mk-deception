#ifndef MKD_ANIM_TYPES_H
#define MKD_ANIM_TYPES_H

#include "math/gxVect.h"
#include "runtime/mk_proc.h"

typedef struct AnimTagFrame AnimTagFrame;
typedef struct AnimEntryName AnimEntryName;
typedef struct AniData AniData;
typedef struct MkObj MkObj;
typedef struct PlyrPdata PlyrPdata;

typedef struct AnimChannelHeader {
    int type;
    unsigned int target;
    unsigned int data_offset;
} AnimChannelHeader;

typedef struct AnimMergedChannelHeader {
    int type;
    unsigned int target;
} AnimMergedChannelHeader;

typedef struct AnimScript {
    char pad00[0x18];
    unsigned int frame_count;
    int track_count;
    unsigned int merged_frame_stride;
    unsigned int tag_data_offset;
    unsigned int tag_end_offset;
    unsigned short flags;
    char pad2E[2];
    int loop_offset_x; /* fixed-point 1/10000 */
    int loop_offset_y;
    int loop_offset_z;
    union {
        AnimChannelHeader tracks[1];
        AnimMergedChannelHeader merged_tracks[1];
    };
} AnimScript;

typedef struct AnimPdata {
    MkHdr hdr;
    MkProc* proc;
    unsigned int proc_instance;
    MkObj* obj;
    unsigned int obj_instance;
    PlyrPdata* owner;
    unsigned int owner_instance;
    int auxiliary_track;
    union {
        unsigned int last_exec_tick;
        unsigned int last_update_tick;
    };
    unsigned int updates_this_tick;
    union {
        unsigned int script_word;
        AnimEntryName* script_entry;
        AniData* animation;
        AnimScript* script;
    };
    unsigned int flags;
    float previous_frame;
    float frame;
    float low_frame;
    float high_frame;
    float step;
    float step_accel;
    void (*frame_callback)(struct AnimPdata*, float*);
    union {
        Vec root_offset;
        Vec anim_offset;
    };
    float anim_angle;
    float field_60;
    union {
        float weight;
        float obj_movement_weight;
    };
    union {
        float weight_velocity;
        float root_movement_weight;
    };
    AnimScript* old_script;
    unsigned int old_flags;
    float old_frame;
    float old_low_frame;
    float old_high_frame;
    union {
        float field_80;
        float old_step;
    };
    float old_step_accel;
    void (*old_frame_callback)(struct AnimPdata*, float*);
    Vec old_anim_offset;
    float old_anim_angle;
    float old_field_9C;
    float old_obj_movement_weight;
    float old_root_movement_weight;
    float transition_weight;
    float transition_step;
    float transition_accel;
    float hand_transition;
    union {
        float transition_target;
        float hand_transition_step;
    };
    float hand_transition_limit;
    int track_capacity;
    void** track_data;
    AnimTagFrame* tag_frame;
    unsigned char* bone_remap;
    union {
        AniData* hand_animation;
        MkProcEntryFn hand_script;
        AnimScript* hand_anim_script;
    };
    AnimScript* next_hand_script;
    float hand_transition_frames;
    char padDC[8];
    unsigned int hand_flags;
    unsigned int field_E8;
    char padEC[8];
    int rest_ticks;
    char padF8[0x0C];
} AnimPdata;

typedef char AnimPdataSizeCheck[sizeof(AnimPdata) == 0x104 ? 1 : -1];

#endif
