#include "runtime/mk_obj.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "rw/rwcore_types.h"

#define RW_MATRIX_MAT33(matrix_) ((Mat33*)(matrix_))

static float p_morph(void);
static void do_morph(MkHdr* hdr);
static float p_bone_matcher(void);
float p_anim_idle(void);

MkProc* morph_proc;
Vec ani_flip_angs = {0.0f, 0.0f, 3.1415927f};
int pose_morph(MkHdr* hdr);

typedef struct MorphScript {
    unsigned int frame_count;
    unsigned short* frame_table;
} MorphScript;

typedef struct MorphFrameHeader {
    unsigned short frame;
    unsigned char data[6];
} MorphFrameHeader;

typedef struct MorphState {
    MkHdr hdr;
    unsigned int morph_target_count; /* +0x08 */
    RpAtomic* atomic;                /* +0x0C */
    void* interpolator;              /* +0x10 */
    MorphScript* script;             /* +0x14 */
    unsigned short* frame_table;     /* +0x18 */
    int frame_count;                 /* +0x1C */
    unsigned short* current_frame;   /* +0x20 */
    unsigned int flags;              /* +0x24 */
    float frame;
    float low_frame;                 /* +0x2C */
    float high_frame;                /* +0x30 */
    float frame_step;
    void (*frame_callback)(struct MorphState*, float*);
    MkPtr* list;
} MorphState;

static int set_morph_frameno(MorphState* morph);

typedef struct MorphInterpolator {
    unsigned int flags;
    short morph_target;
    short next_morph_target;
    char pad08[8];
    float position;
} MorphInterpolator;

typedef struct AnimChannelHeader {
    int type;
    unsigned int target;
    unsigned int data_offset;
} AnimChannelHeader;

typedef struct BoneMatcherFlags08Bits {
    unsigned char inactive : 1;                 /* bit7 */
    unsigned char copy_bone_matrix : 1;         /* bit6 */
    unsigned char copy_clone_matrix : 1;        /* bit5 */
    unsigned char preserve_bone_matrix : 1;     /* bit4 */
    unsigned char copy_parent_angles : 1;       /* bit3 */
    unsigned char flip_parent_angle_y : 1;      /* bit2 */
    unsigned char release_parent_weight : 1;    /* bit1 */
    unsigned char blend_child_transform : 1;    /* bit0 */
} BoneMatcherFlags08Bits;

typedef union BoneMatcherFlags08 {
    unsigned char raw;
    BoneMatcherFlags08Bits bits;
} BoneMatcherFlags08;

typedef struct BoneMatcherFlags09Bits {
    unsigned char use_unmirrored_parent : 1;    /* bit7 */
    unsigned char copy_child_flip : 1;          /* bit6 */
    unsigned char snap_child_transform : 1;     /* bit5 */
    unsigned char pad : 5;
} BoneMatcherFlags09Bits;

typedef union BoneMatcherFlags09 {
    unsigned char raw;
    BoneMatcherFlags09Bits bits;
} BoneMatcherFlags09;

typedef struct BoneMatcherClone {
    MkHdr hdr;
    char pad08[0x10];
    RwFrame* frame; /* +0x18 */
} BoneMatcherClone;

typedef struct BoneMatcherState {
    MkHdr hdr;
    BoneMatcherFlags08 flags_08;
    BoneMatcherFlags09 flags_09;
    char pad0A[2];
    float child_weight;
    MkObj* parent_obj;
    unsigned int parent_instance;
    int parent_bid;
    Vec parent_offset;
    MkObj* child_obj;
    unsigned int child_instance;
    BoneMatcherClone* clone_obj;
    unsigned int clone_instance;
    int fake_child_bid;
    Vec child_offset;
    float blend_ticks;
    char pad4C[4];
    RwMatrix child_matrix;
    RwMatrix flipped_child_matrix;
    Quat parent_rotation;
    Vec parent_translation;
    Quat mirrored_parent_rotation;
    Vec mirrored_parent_translation;
    char pad108[8];
} BoneMatcherState; /* 0x110 */

typedef struct BoneScanContext {
    RwMatrix* matrix;
    int bone_index;
} BoneScanContext;

typedef struct FlippedBoneMap {
    unsigned int count;
    unsigned int* bone_indices;
} FlippedBoneMap;

typedef struct HAnimHierarchyView {
    unsigned int flags;
    int node_count;
    RwMatrix* matrices;
    char pad0C[4];
    struct HAnimNodeInfo* nodes;
} HAnimHierarchyView;

typedef struct HAnimNodeInfo {
    unsigned int node_id;
    int node_index;
    unsigned int flags;
    RwFrame* frame;
} HAnimNodeInfo;

typedef AnimChannelHeader AnimTrack;

typedef struct AnimScript {
    char pad00[0x18];
    int frame_count;
    int track_count;
    char pad20[4];
    unsigned int tag_data_offset;
    unsigned int tag_end_offset;
    unsigned short flags;
    char pad2E[2];
    int loop_offset_x;             /* +0x30, fixed-point 1/10000 */
    int loop_offset_y;             /* +0x34 */
    int loop_offset_z;             /* +0x38 */
    AnimTrack tracks[1];
} AnimScript;

typedef struct AnimTagFrame {
    short frame;
    short field_02;
    short command;
    unsigned char field_06;
    unsigned char bone_index;
    char pad08[4];
} AnimTagFrame;

typedef struct AnimVecFrame {
    unsigned short frame;
    short x;
    short y;
    short z;
} AnimVecFrame;

typedef struct AnimQuatFrame {
    unsigned short frame;
    unsigned short field_02;
    int x;
    int y;
    int z;
    int w;
} AnimQuatFrame;

typedef struct AnimPackedQuatFrame {
    unsigned short frame;
    short packed_wx;
    unsigned int packed_xyz;
} AnimPackedQuatFrame;

typedef struct AnimMatrixFrame {
    short x;
    short y;
    short z;
    short packed_wx;
    unsigned int packed_xyz;
} AnimMatrixFrame;

typedef struct AnimScalarFrame {
    unsigned short frame;
    short value;
} AnimScalarFrame;

typedef struct AnimPoseFrame {
    unsigned short frame;
    short position_x;
    short position_y;
    short position_z;
    unsigned int bone_and_flags;
    unsigned short pose_id;
    short offset_x;
    short offset_y;
    short offset_z;
} AnimPoseFrame;

typedef struct AnimObjectLatch {
    MkObj* obj;
    unsigned int instance;
} AnimObjectLatch;

typedef struct AnimProcLatch {
    MkProc* proc;
    unsigned int instance;
} AnimProcLatch;

typedef struct AnimPoseProcSet {
    char pad00[0x1C];
    AnimProcLatch primary;
    char pad24[0x10];
    AnimProcLatch secondary;
} AnimPoseProcSet;

typedef struct AnimOwnerChannelSet {
    AnimObjectLatch primary;
    char pad08[0x10];
    AnimObjectLatch secondary;
} AnimOwnerChannelSet;

typedef struct AnimOwnerView {
    MkHdr hdr;
    char pad08[0x30];
    AnimObjectLatch tertiary;
    char pad40[0x2CC];
    AnimPoseProcSet* pose_set;
    AnimOwnerChannelSet* channel_set;
    char pad314[0x48];
    AnimProcLatch tertiary_pose;
} AnimOwnerView;

typedef struct AnimPoseProcData {
    MkHdr hdr;
    unsigned char flags_08;
    char pad09[0x0F];
    unsigned int pose_id;
    Vec offset;
    char pad28[0x10];
    unsigned int bone;
    Vec position;
} AnimPoseProcData;

typedef struct AnimState {
    MkHdr hdr;
    MkProc* proc;                    /* +0x08 */
    unsigned int proc_instance;     /* +0x0C */
    MkObj* obj;                    /* +0x10 */
    unsigned int obj_instance;    /* +0x14 */
    MkHdr* owner;                 /* +0x18 */
    unsigned int owner_instance;  /* +0x1C */
    int auxiliary_track;           /* +0x20 - face/hand process marker */
    unsigned int last_update_tick; /* +0x24 */
    unsigned int updates_this_tick; /* +0x28 */
    AnimScript* script;           /* +0x2C */
    unsigned int flags;           /* +0x30 */
    float previous_frame;         /* +0x34 */
    float frame;                  /* +0x38 */
    float low_frame;              /* +0x3C */
    float high_frame;             /* +0x40 */
    float step;                   /* +0x44 */
    float step_accel;             /* +0x48 */
    void (*frame_callback)(struct AnimState*, float*);
    Vec anim_offset;              /* +0x50 */
    float anim_angle;             /* +0x5C */
    float field_60;
    float obj_movement_weight;    /* +0x64 */
    float root_movement_weight;   /* +0x68 */
    AnimScript* old_script;       /* +0x6C */
    unsigned int old_flags;       /* +0x70 */
    float old_frame;              /* +0x74 */
    float old_low_frame;          /* +0x78 */
    float old_high_frame;         /* +0x7C */
    float old_step;               /* +0x80 */
    float old_step_accel;         /* +0x84 */
    void (*old_frame_callback)(struct AnimState*, float*);
    Vec old_anim_offset;          /* +0x8C */
    float old_anim_angle;         /* +0x98 */
    float old_field_9C;
    float old_obj_movement_weight; /* +0xA0 */
    float old_root_movement_weight; /* +0xA4 */
    float transition_weight;      /* +0xA8 */
    float transition_step;        /* +0xAC */
    float transition_accel;       /* +0xB0 */
    float hand_transition;        /* +0xB4 */
    float hand_transition_step;   /* +0xB8 */
    float hand_transition_limit;  /* +0xBC */
    int track_capacity;           /* +0xC0 */
    void** track_data;            /* +0xC4 */
    AnimTagFrame* tag_frame;      /* +0xC8 */
    char padCC[4];
    AnimScript* hand_script;      /* +0xD0 */
    AnimScript* next_hand_script; /* +0xD4 */
    float hand_transition_frames; /* +0xD8 */
    char padDC[8];
    unsigned int hand_flags;      /* +0xE4 */
} AnimState;

static MkObj* anim_state_obj(const AnimState* anim);

AnimState* anim_pdata;
void* anim_obj;
extern void* plyr_pdata;
FlippedBoneMap* flipped_bones;
static float flip_factor;
extern float game_speed;
extern int exec_tick_ctr;
extern MkVtable5 vtbl_mkpdata_anim;

float mka_next_fno;
float mka_prev_fno;
unsigned short* mka_next_fp;
unsigned short* mka_prev_fp;
int mka_bytes_per_frame;
float mka_sought_fno;
void* mka_merge_channel_hdr;
AnimChannelHeader* mka_channel_hdr;
unsigned short* mka_hdr;

void* memcpy(void* dst, const void* src, unsigned int size);
int sprintf(char* dst, const char* format, ...);
void* obj_get_1st_atomic(MkObj* obj);
void get_bone_world_pos(MkObj* obj, int bone, Vec* out);
void get_bone_offset_world_pos(
    MkObj* obj, int bone, const Vec* offset, Vec* out);
void update_bone_hierarchy(MkHdr* obj);
void RwFrameUpdateObjects(RwFrame* frame);
void* RpHAnimFrameGetHierarchy(RwFrame* frame);
RwFrame* RwFrameForAllChildren(
    RwFrame* frame,
    RwFrame* (*callback)(RwFrame*, void*),
    void* data);
void* RpSkinAtomicSetHAnimHierarchy(void* atomic, void* hierarchy);
void* RpSkinGeometryGetSkin(RpGeometry* geometry);
RwMatrix* RpSkinGetSkinToBoneMatrices(void* skin);
void insert_bone_hierarchy_mkobj(void* object);
void set_camera_focal_length();
int pose_anim(AnimState* anim, int update_object);
int set_anim_script_frame(
    float frame, AnimState* anim, AnimScript* script, unsigned int flags);
int transition_to_anim_script_frame(
    float transition_frames,
    float frame,
    AnimState* anim,
    AnimScript* script,
    unsigned int flags);

/*
 * Animation scripts are retail packed blobs with offsets relative to their
 * own base. Keep the byte walk isolated here instead of open-coding casts at
 * each consumer.
 */
static inline void* anim_script_data(const void* script, unsigned int offset) {
    return (unsigned char*)script + offset;
}

static void ps_anim(void) {
    anim_pdata = 0;
    anim_obj = 0;
    plyr_pdata = 0;
}

typedef int (*AnimProcSleepFn)(MkProcEntryFn entry, float ticks);
typedef int (*AnimProcJumpFn)(MkProcEntryFn entry, float ticks);
typedef struct AnimProcVtable {
    void* functions[6];
    AnimProcSleepFn sleep;
    void* stack_ops[2];
    AnimProcJumpFn jump_sleep;
} AnimProcVtable;

/* Soft ceiling: p_anim_reset_weight_idle ~69.71% - typed proc dispatch retained. */
void p_anim_reset_weight_idle(void) {
    AnimState* anim = (AnimState*)anim_pdata;
    AnimProcSleepFn sleep =
        ((AnimProcVtable*)aproc->vtbl)->sleep;

    anim->hand_transition = 1.0f;
    anim->hand_transition_step = 0.0f;
    sleep((MkProcEntryFn)p_anim_idle, 0.0f);
}

/* Soft ceiling: pw_anim ~68.70% - typed live-object latches retained. */
void pw_anim(void) {
    AnimState* anim = (AnimState*)apdata;

    anim_pdata = anim;
    anim_obj =
        anim->obj != 0 && anim->obj->hdr.instance == anim->obj_instance
        ? anim->obj : 0;
    if (anim_obj == 0) {
        mkproc_die();
    }
    plyr_pdata =
        anim->owner != 0 &&
        anim->owner->instance == anim->owner_instance
        ? anim->owner : 0;
}

AnimState* get_mkpdata_anim(void) {
    AnimState* anim =
        (AnimState*)get_mkhdr(&vtbl_mkpdata_anim, sizeof(AnimState));

    if (anim != 0) {
        MkHdr hdr = anim->hdr;
        memset(anim, 0, sizeof(AnimState));
        anim->hdr = hdr;
        anim->hand_transition = 1.0f;
        anim->hand_transition_limit = 1.0f;
        anim->step = 1.0f;
        anim->root_movement_weight = 1.0f;
        anim->hand_transition_frames = 1.0f;
    }
    return anim;
}

static void create_anim_process(
    int priority,
    int pid,
    MkProcEntryFn entry,
    AnimState** out_anim,
    int auxiliary_track) {
    int flags;
    MkProc* proc;
    AnimState* anim;

    flags = 0x40;
    proc = get_mkproc_tinystack(&flags);
    anim = get_mkpdata_anim();
    *out_anim = anim;
    proc = create_mkproc(priority, proc, pid, entry, &anim->hdr);
    if (proc != 0) {
        anim->proc = proc;
        anim->proc_instance = proc->instance;
        proc->pre_destroy = pw_anim;
        proc->destroy_cb = ps_anim;
        if (auxiliary_track != 0) {
            anim->auxiliary_track = 1;
        }
    }
}

void create_mkproc_anim(
    int pid, MkProcEntryFn entry, AnimState** out_anim) {
    create_anim_process(0xB, pid, entry, out_anim, 0);
}

void create_mkproc_anim2(
    int pid, MkProcEntryFn entry, AnimState** out_anim) {
    create_anim_process(0xC, pid, entry, out_anim, 0);
}

void create_mkproc_face_anim(
    int pid, MkProcEntryFn entry, AnimState** out_anim) {
    create_anim_process(0xD, pid, entry, out_anim, 1);
}

void create_mkproc_hand_anim(
    int pid, MkProcEntryFn entry, AnimState** out_anim) {
    create_anim_process(0xD, pid, entry, out_anim, 1);
}

int advance_anim(AnimState* anim) {
    MkObj* obj;
    float speed;
    float frame_step;
    float maximum_step;
    int maximum_step_int;

    obj = anim_state_obj(anim);
    speed = game_speed;
    if (obj != 0 && (obj->flags_0B & 0x20) != 0) {
        speed = 1.0f;
    }

    anim->hand_transition += anim->hand_transition_step * speed;
    if (anim->hand_transition <= 0.0f) {
        anim->hand_transition = 0.0f;
        return 1;
    }
    if (anim->hand_transition > anim->hand_transition_limit) {
        anim->hand_transition = anim->hand_transition_limit;
    }

    if (anim->transition_weight < 1.0f) {
        anim->transition_weight +=
            speed * (anim->transition_step + anim->transition_accel);
        if (anim->transition_weight > 1.0f) {
            anim->transition_weight = 1.0f;
            anim->transition_step = 0.0f;
            anim->transition_accel = 0.0f;
            anim->flags &= ~0x100;
        }
    }

    if (anim->transition_weight < 1.0f) {
        maximum_step = (float)anim->old_script->frame_count / 3.0f;
        maximum_step_int = (int)maximum_step;
        frame_step =
            speed * (anim->old_step + anim->old_step_accel);
        if (frame_step > maximum_step) {
            frame_step = (float)maximum_step_int;
        } else if (frame_step < -maximum_step) {
            frame_step = (float)-maximum_step_int;
        }
        anim->old_frame += frame_step;
    }

    anim->previous_frame = anim->frame;
    maximum_step = (float)anim->script->frame_count / 3.0f;
    maximum_step_int = (int)maximum_step;
    frame_step = speed * (anim->step + anim->step_accel);
    if (frame_step > maximum_step) {
        frame_step = (float)maximum_step_int;
    } else if (frame_step < -maximum_step) {
        frame_step = (float)-maximum_step_int;
    }
    anim->frame += frame_step;
    if (anim->frame < anim->low_frame ||
        anim->frame >= anim->high_frame + 1.0f) {
        return 0;
    }
    return 1;
}

void p_animate(void) {
    AnimState* anim;

    anim = (AnimState*)anim_pdata;
    if (anim->last_update_tick == (unsigned int)exec_tick_ctr) {
        return;
    }
    advance_anim(anim);
    if (anim->hand_transition == 0.0f) {
        ((AnimProcVtable*)aproc->vtbl)->jump_sleep(
            (MkProcEntryFn)p_anim_reset_weight_idle, 0.0f);
        return;
    }
    pose_anim(anim, 1);
}

unsigned short* find_frame(unsigned short* current) {
    unsigned short* first;
    unsigned short* last;
    float current_frame;
    float delta;

    current_frame = (float)*current;
    delta = mka_sought_fno - current_frame;
    first = (unsigned short*)((unsigned char*)mka_hdr +
                              mka_channel_hdr->data_offset);
    last = (unsigned short*)((unsigned char*)mka_hdr +
                             mka_channel_hdr[1].data_offset -
                             mka_bytes_per_frame);

    if (delta == 0.0f) {
        mka_prev_fp = current;
        mka_next_fp = current;
        mka_prev_fno = mka_sought_fno;
        mka_next_fno = mka_sought_fno;
        return current;
    }
    if (delta > 0.0f) {
        if (delta < 5.0f ||
            mka_sought_fno <
                0.5f * (current_frame + (float)*last)) {
            mka_next_fp = current;
            mka_next_fno = current_frame;
            mka_prev_fp = mka_next_fp;
            mka_prev_fno = mka_next_fno;
            while (mka_next_fno < mka_sought_fno) {
                mka_prev_fp = mka_next_fp;
                mka_prev_fno = mka_next_fno;
                mka_next_fp = (unsigned short*)(
                    (unsigned char*)mka_next_fp + mka_bytes_per_frame);
                mka_next_fno = (float)*mka_next_fp;
            }
            return mka_prev_fp;
        }
        mka_prev_fp = last;
        mka_prev_fno = (float)*last;
    } else {
        if (mka_sought_fno < 0.5f * current_frame) {
            mka_next_fp = first;
            mka_next_fno = 0.0f;
            mka_prev_fp = mka_next_fp;
            mka_prev_fno = mka_next_fno;
            while (mka_next_fno < mka_sought_fno) {
                mka_prev_fp = mka_next_fp;
                mka_prev_fno = mka_next_fno;
                mka_next_fp = (unsigned short*)(
                    (unsigned char*)mka_next_fp + mka_bytes_per_frame);
                mka_next_fno = (float)*mka_next_fp;
            }
            return mka_prev_fp;
        }
        mka_prev_fp = current;
        mka_prev_fno = current_frame;
    }

    mka_next_fp = mka_prev_fp;
    mka_next_fno = mka_prev_fno;
    while (mka_prev_fno > mka_sought_fno) {
        mka_next_fp = mka_prev_fp;
        mka_next_fno = mka_prev_fno;
        mka_prev_fp = (unsigned short*)(
            (unsigned char*)mka_prev_fp - mka_bytes_per_frame);
        mka_prev_fno = (float)*mka_prev_fp;
    }
    return mka_prev_fp;
}

static unsigned short* morph_find_frame(
    MorphState* morph, unsigned short* current) {
    unsigned short* first;
    unsigned short* last;
    float current_frame;
    float delta;

    first = morph->frame_table;
    last = (unsigned short*)((unsigned char*)first +
                             morph->frame_count * 8);
    current_frame = (float)*current;
    delta = mka_sought_fno - current_frame;
    if (delta == 0.0f) {
        mka_prev_fp = current;
        mka_next_fp = current;
        mka_prev_fno = mka_sought_fno;
        mka_next_fno = mka_sought_fno;
        return current;
    }

    if (delta > 0.0f &&
        (delta < 5.0f ||
         mka_sought_fno < 0.5f * (current_frame + (float)*last))) {
        mka_next_fp = current;
        mka_next_fno = current_frame;
        mka_prev_fp = mka_next_fp;
        mka_prev_fno = mka_next_fno;
        while (mka_next_fno < mka_sought_fno) {
            mka_prev_fp = mka_next_fp;
            mka_prev_fno = mka_next_fno;
            mka_next_fp = (unsigned short*)(
                (unsigned char*)mka_next_fp + mka_bytes_per_frame);
            mka_next_fno = (float)*mka_next_fp;
        }
        return mka_prev_fp;
    }
    if (delta < 0.0f &&
        mka_sought_fno < 0.5f * current_frame) {
        mka_next_fp = first;
        mka_next_fno = 0.0f;
        mka_prev_fp = mka_next_fp;
        mka_prev_fno = mka_next_fno;
        while (mka_next_fno < mka_sought_fno) {
            mka_prev_fp = mka_next_fp;
            mka_prev_fno = mka_next_fno;
            mka_next_fp = (unsigned short*)(
                (unsigned char*)mka_next_fp + mka_bytes_per_frame);
            mka_next_fno = (float)*mka_next_fp;
        }
        return mka_prev_fp;
    }

    mka_prev_fp = delta > 0.0f ? last : current;
    mka_prev_fno = (float)*mka_prev_fp;
    mka_next_fp = mka_prev_fp;
    mka_next_fno = mka_prev_fno;
    while (mka_prev_fno > mka_sought_fno) {
        mka_next_fp = mka_prev_fp;
        mka_next_fno = mka_prev_fno;
        mka_prev_fp = (unsigned short*)(
            (unsigned char*)mka_prev_fp - mka_bytes_per_frame);
        mka_prev_fno = (float)*mka_prev_fp;
    }
    return mka_prev_fp;
}

static void update_morph_interpolator(
    MorphState* morph,
    MorphInterpolator* interpolator,
    float position,
    short target,
    short next_target) {
    int changed;

    changed = 0;
    interpolator->position = position;
    interpolator->flags |= 3;
    if (interpolator->morph_target != target) {
        interpolator->morph_target = target;
        interpolator->flags |= 3;
        changed = 1;
    }
    if (interpolator->next_morph_target != next_target) {
        interpolator->next_morph_target = next_target;
        interpolator->flags |= 3;
        changed = 1;
    }
    if (((interpolator->flags & 4) == 0 || changed != 0) &&
        morph->atomic != 0 && morph->atomic->object.parent != 0) {
        RwFrameUpdateObjects(
            (RwFrame*)morph->atomic->object.parent);
    }
}

int pose_morph(MkHdr* hdr) {
    MorphState* morph;
    MorphInterpolator* interpolator;
    unsigned short* frame;
    float position;
    float fraction;
    float span;
    float previous_position;
    short target;
    short next_target;
    int result;

    morph = (MorphState*)hdr;
    result = set_morph_frameno(morph);
    mka_bytes_per_frame = 8;
    mka_sought_fno = morph->frame;
    morph->current_frame =
        morph_find_frame(morph, morph->current_frame);

    if (mka_next_fno == mka_sought_fno) {
        frame = mka_next_fp;
        target = frame[1] >> 8;
        next_target = frame[1] & 0xFF;
        position = *(float*)(frame + 2);
    } else if (mka_prev_fno == mka_sought_fno) {
        frame = mka_prev_fp;
        target = frame[1] >> 8;
        next_target = frame[1] & 0xFF;
        position = *(float*)(frame + 2);
    } else {
        span = mka_next_fno - mka_prev_fno;
        fraction = span != 0.0f
            ? (mka_next_fno - mka_sought_fno) / span
            : 0.0f;
        target = (short)((unsigned char*)mka_next_fp)[2];
        next_target = (short)((unsigned char*)mka_next_fp)[3];
        previous_position = *(float*)(mka_prev_fp + 2);
        if (((unsigned char*)mka_prev_fp)[3] !=
            ((unsigned char*)mka_next_fp)[3]) {
            previous_position = 0.0f;
        }
        position =
            previous_position * fraction +
            *(float*)(mka_next_fp + 2) * (1.0f - fraction);
    }

    interpolator = (MorphInterpolator*)morph->interpolator;
    update_morph_interpolator(
        morph, interpolator, position, target, next_target);
    return result;
}

void bone_matcher_child_set_offset(BoneMatcherState* matcher, const float* offset) {
    matcher->child_offset.x = offset[0];
    matcher->child_offset.y = offset[1];
    matcher->child_offset.z = offset[2];
}

void bone_matcher_parent_set_offset(BoneMatcherState* matcher, const float* offset) {
    matcher->parent_offset.x = offset[0];
    matcher->parent_offset.y = offset[1];
    matcher->parent_offset.z = offset[2];
}

float p_anim_idle(void) {
    return 1000.0f;
}

static float p_morph(void) {
    apply_to_mklist(do_morph, &aproc->pdata_list);
    return 1.0f;
}

static void do_morph(MkHdr* hdr) {
    if (hdr != 0) {
        float frame = ((MorphState*)hdr)->frame;

        ((MorphState*)hdr)->frame = frame + ((MorphState*)hdr)->frame_step;
        pose_morph(hdr);
    }
}

void start_morph_proc(void) {
    int flags[2];
    MkProc* proc;

    flags[1] = 0;
    flags[0] = 0;
    proc = get_mkproc_nostack(flags);
    morph_proc = proc;
    morph_proc = create_mkproc(0x12, proc, 0x500B, p_morph, 0);
}

void bm_force_fake_child_bid(BoneMatcherState* matcher, int bone_id) {
    matcher->fake_child_bid = bone_id;
}

void reset_ani_data_space(void) {
}

void mkbone_remove(MkBone* bone) {
    destroy_list(&bone->list_80);
    bone->parent_matrix = 0;
}

void mkbone_insert_child_of_clone_parent(MkBone* bone, MkBone* parent) {
    bone->clone_source = parent;
    bone->root_next = parent->root_next;
    parent->root_next = bone;
}

void _bone_make_parents_my_children(MkBone* bone);

void bone_make_parents_my_children(MkBone* bone) {
    _bone_make_parents_my_children(bone);
    bone->translation_row = bone->parent_matrix->pos_row;
    RtQuatConvertFromMatrix(&bone->rt_rotation, bone->parent_matrix);
}

static inline MkObj* anim_state_obj(const AnimState* anim) {
    if (anim->obj == 0 || anim->obj->hdr.instance != anim->obj_instance) {
        return 0;
    }
    return anim->obj;
}

static inline float anim_last_frame(const AnimScript* script) {
    return (float)script->frame_count - 1.0f;
}

static inline void select_flip_map(AnimState* anim, unsigned int flags) {
    MkObj* obj = anim_state_obj(anim);
    int flipped = obj != 0 && (obj->hide_flags & 0x40) != 0;

    if ((flags & 8) != 0) {
        flipped = 1 - flipped;
    }
    if (flipped == 0) {
        flipped_bones = 0;
        flip_factor = 1.0f;
    } else {
        flipped_bones = (FlippedBoneMap*)obj->allocation_74;
        flip_factor = -1.0f;
    }
}

static inline void apply_script_loop_offset(
    AnimState* anim, AnimScript* script, Vec* offset, float* angle, int sign) {
    const float scale = 0.0001f;

    if (script == 0) {
        return;
    }
    offset->x += sign * flip_factor * script->loop_offset_x * scale;
    offset->y += sign * script->loop_offset_y * scale;
    offset->z += sign * script->loop_offset_z * scale;
    *angle = norm_angle(*angle);
    (void)anim;
}

/* Soft ceiling: 68.11% - retail morph wrap modes and callbacks recovered. */
static int set_morph_frameno(MorphState* morph) {
    float frame = morph->frame;
    float low = morph->low_frame;
    unsigned int mode = morph->flags & 7;

    if (frame < low) {
        switch (mode) {
        case 0:
            morph->frame = frame + (morph->high_frame - low);
            break;
        case 2:
            morph->frame = 2.0f * low - frame;
            if (morph->frame_step < 0.0f) {
                morph->frame_step = -morph->frame_step;
            }
            break;
        case 3:
            morph->frame = low;
            break;
        }
        if (morph->frame_callback != 0) {
            morph->frame_callback(morph, &morph->frame);
        }
        return 0;
    }

    switch (mode) {
    case 0:
    case 1: {
        float end = 1.0f + morph->high_frame;
        if (frame < end) {
            return 1;
        }
        morph->frame = frame - (end - low);
        break;
    }
    case 2:
        if (frame <= morph->high_frame) {
            return 1;
        }
        morph->frame = 2.0f * morph->high_frame - frame;
        if (morph->frame_step > 0.0f) {
            morph->frame_step = -morph->frame_step;
        }
        break;
    case 3:
        if (frame <= morph->high_frame) {
            return 1;
        }
        morph->frame = morph->high_frame;
        morph->frame_step = 0.0f;
        break;
    case 4:
        break;
    default:
        return 1;
    }
    if (morph->frame_callback != 0) {
        morph->frame_callback(morph, &morph->frame);
    }
    return 0;
}

/* Soft ceiling: typed morph allocation/setup; retail failure diagnostics kept. */
MorphState* obj_start_morph(
    MkObj* obj, int sobj_id, MorphScript* script, unsigned int flags) {
    MorphState* morph = (MorphState*)get_mkpdata_generic(sizeof(MorphState));
    MkSobj* sobj = 0;

    if (morph == 0) {
        return 0;
    }
    morph->frame_callback = 0;
    morph->atomic = 0;
    if (sobj_id != 0) {
        char message[80];

        sobj = (MkSobj*)obj_find_sobj_by_id(obj, sobj_id);
        if (sobj == 0) {
            sobj = (MkSobj*)obj_create_sobjs_by_id(obj, sobj_id);
        }
        if (sobj == 0) {
            sprintf(
                message,
                "sobj ID: %d was not found. Error in obj_start_morph mk_anim.c",
                sobj_id);
            return 0;
        }
        morph->atomic = sobj->atomic;
    }
    if (morph->atomic == 0) {
        morph->atomic = (RpAtomic*)obj_get_1st_atomic(obj);
    }
    if (morph->atomic == 0) {
        return morph;
    }

    morph->interpolator = &morph->atomic->interpolatorFlags;
    morph->morph_target_count =
        morph->atomic->geometry->numMorphTargets;
    morph->frame_step = 1.0f;
    morph->flags = flags;
    morph->script = script;
    morph->frame_table = script->frame_table;
    morph->frame_count = script->frame_count;
    morph->current_frame = morph->frame_table;
    morph->frame = 0.0f;
    morph->low_frame = 0.0f;
    morph->high_frame =
        (float)(((MorphFrameHeader*)morph->frame_table)
                    [morph->frame_count]
                        .frame -
                1);
    pose_morph(&morph->hdr);
    mk_insert(&morph->hdr, &morph_proc->pdata_list);
    mk_insert(&morph->hdr, &obj->child_list);
    return morph;
}

/* Soft ceiling: typed tag-command state transitions recovered. */
static void apply_tag_frame(AnimState* anim, MkObj* obj) {
    unsigned int bone_index;

    switch (anim->tag_frame->command) {
    case 0x500:
        if ((anim->flags & 0x10000) == 0) {
            if ((obj->flags_0C & 0x40) == 0) {
                obj->flags_09 &= ~0x40;
            } else {
                obj->flags_0C = (obj->flags_0C & ~0x20) | 0x20;
            }
        }
        break;
    case 0x501:
    case 0x502:
        if ((anim->flags & 0x10000) == 0) {
            obj->flags_0C = (obj->flags_0C & ~0x40) | 0x40;
            obj->flags_09 = (obj->flags_09 & ~0x40) | 0x40;
        }
        break;
    case 0x503:
        if ((obj->flags_0C & 0x10) != 0 ||
            (anim->flags & 0x20000) != 0) {
            break;
        }
        bone_index = anim->tag_frame->bone_index;
        if (bone_index >= obj->bone_count) {
            break;
        }
        if (flipped_bones != 0 && bone_index < flipped_bones->count) {
            bone_index = flipped_bones->bone_indices[bone_index];
        }
        if ((unsigned int)obj->ground_bone == bone_index &&
            (obj->hide_flags & 2) != 0) {
            break;
        }
        obj->ground_bone = bone_index;
        obj->hide_flags = (obj->hide_flags & ~2) | 2;
        obj->flags_0C = (obj->flags_0C & ~0x10) | 0x10;
        get_bone_world_pos(obj, obj->ground_bone, &obj->ground_restore_pos);
        break;
    case 0x504:
        if ((anim->flags & 0x20000) == 0) {
            if ((obj->flags_0C & 0x40) == 0) {
                obj->hide_flags &= ~2;
            } else {
                obj->flags_0C = (obj->flags_0C & ~8) | 8;
            }
        }
        break;
    }
}

/* Soft ceiling: typed animation-offset accumulation recovered. */
static void apply_anim_offset(
    float frame_scale,
    AnimState* anim,
    MkObj* obj,
    Vec* offset,
    int old_anim,
    int apply_to_object) {
    Vec* previous = old_anim ? &anim->old_anim_offset : &anim->anim_offset;
    Vec delta;
    Vec world_delta;

    if (old_anim != 0) {
        apply_to_object = 1;
    }
    delta.x = offset->x - previous->x;
    delta.y = offset->y - previous->y;
    delta.z = offset->z - previous->z;
    *previous = *offset;
    offset->x *= anim->root_movement_weight;
    offset->y *= anim->root_movement_weight;
    offset->z *= anim->root_movement_weight;

    if (apply_to_object != 0 && anim->obj_movement_weight != 0.0f) {
        v3_x_mat(&world_delta, &delta, obj->field_24);
        frame_scale *= anim->obj_movement_weight;
        obj->pos.x += world_delta.x * frame_scale;
        obj->pos.y += world_delta.y * frame_scale;
        obj->pos.z += world_delta.z * frame_scale;
    }
}

/* Soft ceiling: 95.23% - typed movement-weight compensation recovered. */
void set_root_and_obj_movement_weights(
    float root_weight, float obj_weight, AnimState* anim) {
    MkObj* obj = anim_state_obj(anim);
    RwMatrix* root_matrix;
    Vec world_delta;
    Vec local_delta;
    float weight_delta = root_weight - anim->root_movement_weight;

    anim->root_movement_weight = root_weight;
    anim->obj_movement_weight = obj_weight;
    local_delta.x = anim->anim_offset.x * weight_delta;
    local_delta.y = anim->anim_offset.y * weight_delta;
    local_delta.z = anim->anim_offset.z * weight_delta;
    root_matrix = obj->bones[obj->fallback_bone_index]->parent_matrix;
    root_matrix->pos.x += local_delta.x;
    root_matrix->pos.y += local_delta.y;
    root_matrix->pos.z += local_delta.z;
    v3_x_mat(&world_delta, &local_delta, obj->field_24);
    obj->pos.x -= world_delta.x;
    obj->pos.y -= world_delta.y;
    obj->pos.z -= world_delta.z;
}

/* Soft ceiling: 89.11% - typed fast path/search recovered; loop emit differs. */
unsigned int obj_get_bid_for_tid(MkObj* obj, unsigned int tag) {
    unsigned int bone_index = tag & 0xFFF;
    unsigned int i;

    if (bone_index < obj->bone_count && obj->bones[bone_index] != 0 &&
        (unsigned int)obj->bones[bone_index]->tag == tag) {
        return bone_index;
    }
    if ((tag & 0x2000) != 0) {
        for (i = 0; i < obj->cloth_bone_count; i++) {
            MkBone* bone = obj->cloth_bones[i].bone;
            if ((unsigned int)bone->tag == tag) {
                return bone->bone_index;
            }
        }
    } else {
        for (i = 0; i < obj->bone_count; i++) {
            if (obj->bones[i] != 0 &&
                (unsigned int)obj->bones[i]->tag == tag) {
                return i;
            }
        }
    }
    return 0;
}

/* Soft ceiling: typed ownership walk recovered. */
void mkobj_destroy_bones(MkObj* obj) {
    unsigned int i;

    if (obj->bones == 0) {
        return;
    }
    for (i = 0; i < obj->bone_count; i++) {
        if (obj->bones[i] != 0) {
            destroy_list(&obj->bones[i]->list_80);
            free_mem(obj->bones[i]);
        }
    }
    free_mem(obj->bones);
}

/* Soft ceiling: 83.43% - retail initializer recovered with typed members. */
MkBone* alloc_bone(void) {
    MkBone* bone = (MkBone*)get_mem(sizeof(MkBone));
    int i;

    if (bone != 0) {
        bone->tag = 0;
        bone->limb_id = -1;
        bone->parent_matrix = 0;
        bone->original_parent_matrix = 0;
        bone->flags_word_54 = 0;
        bone->list_80 = 0;
        bone->tree_next = 0;
        bone->tree_child = 0;
        bone->transform_parent = 0;
        bone->root_next = 0;
        bone->clone_source = 0;
        bone->field_58 = 0;
        bone->flags_word_54 = 0;
        bone->matrix.at.z = 1.0f;
        bone->matrix.up.y = 1.0f;
        bone->matrix.right.x = 1.0f;
        bone->matrix.up.x = 0.0f;
        bone->matrix.right.z = 0.0f;
        bone->matrix.right.y = 0.0f;
        bone->matrix.at.y = 0.0f;
        bone->matrix.at.x = 0.0f;
        bone->matrix.up.z = 0.0f;
        bone->matrix.pos.z = 0.0f;
        bone->matrix.pos.y = 0.0f;
        bone->matrix.pos.x = 0.0f;
        bone->matrix.flags |= 0x20003;
        bone->matrix.flags = 3;
        bone->delta.z = 0.0f;
        bone->delta.y = 0.0f;
        bone->delta.x = 0.0f;
        bone->velocity.z = 0.0f;
        bone->velocity.y = 0.0f;
        bone->velocity.x = 0.0f;
        bone->field_60 = 0.0f;
        bone->field_64 = 0.0f;
        bone->update_tick = exec_tick_ctr - 1;
        bone->rotation_90.x = 0.0f;
        bone->rotation_90.y = 0.0f;
        bone->rotation_90.z = 0.0f;
        bone->rotation_90.w = 1.0f;
        for (i = 0; i < 2; i++) {
            bone->rotations[i].x = 0.0f;
            bone->rotations[i].y = 0.0f;
            bone->rotations[i].z = 0.0f;
            bone->rotations[i].w = 1.0f;
        }
        bone->field_5C = 0.0f;
        bone->scale.x = 1.0f;
        bone->scale.y = 1.0f;
        bone->scale.z = 1.0f;
    }
    return bone;
}

/* Soft ceiling: recursive reparenting semantics recovered. */
void _bone_make_parents_my_children(MkBone* bone) {
    MkBone* parent = bone->transform_parent;

    if (parent == 0) {
        return;
    }
    if (parent->tree_child == bone) {
        parent->tree_child = bone->tree_next;
        if (parent->tree_child == 0) {
            parent->flags_54 &= ~1;
        }
    } else {
        MkBone* sibling = parent->tree_child;
        while (sibling != 0 && sibling->tree_next != bone) {
            sibling = sibling->tree_next;
        }
        if (sibling != 0) {
            sibling->tree_next = bone->tree_next;
        }
    }
    bone->transform_parent = 0;
    bone->tree_next = 0;

    _bone_make_parents_my_children(parent);
    if (parent->tree_next == 0) {
        bone->flags_54 = (bone->flags_54 & ~1) | 1;
        if (bone->tree_child == 0) {
            bone->tree_child = parent;
        } else {
            MkBone* child = bone->tree_child;
            while (child->tree_next != 0) {
                child = child->tree_next;
            }
            child->tree_next = parent;
        }
        parent->transform_parent = bone;
    }
    parent->rotation = bone->rotation;
    parent->rotation.w = -parent->rotation.w;
    PSVECSubtract(
        &parent->parent_matrix->pos_vec,
        &bone->parent_matrix->pos_vec,
        &parent->translation);
    quat_to_mat(&tmp_matrix, &bone->rotation_90);
    v3_x_mat(&parent->translation, &parent->translation, &tmp_matrix);
    parent->flags_55 =
        (parent->flags_55 & ~4) | (((parent->flags_55 & 4) == 0) ? 4 : 0);
}

/* Soft ceiling: process creation and matrix snapshots recovered. */
BoneMatcherState* start_bone_matcher(
    float blend_ticks,
    MkObj* parent_obj,
    int parent_bid,
    MkObj* child_obj,
    int child_bid) {
    BoneMatcherState* matcher = 0;
    MkBone* parent_bone;
    MkBone* child_bone;
    RwMatrix flip_matrix;

    _create_mkproc_generic_nostack(
        0x500F,
        0x15,
        p_bone_matcher,
        sizeof(BoneMatcherState),
        (MkHdr**)&matcher);
    if (matcher == 0) {
        return 0;
    }

    matcher->flags_08.raw = 0;
    matcher->flags_09.raw = 0;
    matcher->child_weight = 0.0f;
    matcher->parent_obj = parent_obj;
    matcher->parent_instance = parent_obj->hdr.instance;
    matcher->parent_bid = parent_bid;
    zero_v3(&matcher->parent_offset);
    matcher->child_obj = child_obj;
    matcher->child_instance = child_obj->hdr.instance;
    matcher->fake_child_bid = child_bid;
    zero_v3(&matcher->child_offset);

    parent_bone = parent_obj->bones[parent_bid];
    child_bone = child_obj->bones[child_bid];
    if (parent_bone == 0 || parent_bone->parent_matrix == 0 ||
        child_bone == 0 || child_bone->parent_matrix == 0) {
        destroy_mkpdata_generic(&matcher->hdr);
        return 0;
    }

    child_bone->flags_54 =
        (child_bone->flags_54 & ~0x10) | 0x10;
    matcher->clone_obj = 0;
    matcher->clone_instance = 0;
    matcher->parent_rotation = child_bone->rotation;
    matcher->mirrored_parent_rotation = child_bone->rotation;
    matcher->mirrored_parent_rotation.y =
        -matcher->mirrored_parent_rotation.y;
    matcher->mirrored_parent_rotation.z =
        -matcher->mirrored_parent_rotation.z;
    memcpy(
        &matcher->child_matrix,
        child_bone->parent_matrix,
        0x30);
    YXZ_angles_to_MKMATRIX(&ani_flip_angs, &flip_matrix);
    mat_x_mat(
        &matcher->flipped_child_matrix,
        &matcher->child_matrix,
        &flip_matrix);
    matcher->blend_ticks = blend_ticks;
    return matcher;
}

/*
 * Soft ceiling: p_bone_matcher ~88.25% - retail inlines nonvolatile saves and
 * schedules the two quaternion products differently; the typed algorithm and
 * all retail flag-controlled paths are retained.
 */
static float p_bone_matcher(void) {
    BoneMatcherState* matcher = (BoneMatcherState*)apdata;
    MkObj* raw_parent_obj;
    MkObj* parent_obj;
    MkObj* raw_child_obj;
    MkObj* child_obj;
    BoneMatcherClone* raw_clone_obj;
    BoneMatcherClone* clone_obj;
    FlippedBoneMap* bone_map;
    MkBone* child_bone;
    MkBone* parent_bone;
    RwMatrix* child_matrix;
    Vec child_offset;
    Vec parent_offset;
    Vec child_pos;
    Vec parent_pos;
    Quat composed_rotation;
    const Quat* target_rotation;
    const Vec* target_translation;
    unsigned int parent_bid;
    unsigned int child_bid;
    unsigned int mapped_bid;
    float fraction;
    float parent_weight;
    float child_weight;
    float correction_x;
    float correction_y;
    float correction_z;
    float parent_delta_x;
    float parent_delta_y;
    float parent_delta_z;
    float child_delta_x;
    float child_delta_y;
    float child_delta_z;

    if (matcher == 0) {
        mkproc_die();
    }
    if (matcher->flags_08.bits.inactive != 0) {
        return 1.0f;
    }

    raw_parent_obj = matcher->parent_obj;
    if (raw_parent_obj != 0) {
        if (raw_parent_obj->hdr.instance == matcher->parent_instance) {
            parent_obj = raw_parent_obj;
        } else {
            parent_obj = 0;
        }
    } else {
        parent_obj = 0;
    }
    if (parent_obj == 0) {
        mkproc_die();
    }

    raw_child_obj = matcher->child_obj;
    if (raw_child_obj != 0) {
        if (raw_child_obj->hdr.instance == matcher->child_instance) {
            child_obj = raw_child_obj;
        } else {
            child_obj = 0;
        }
    } else {
        child_obj = 0;
    }
    if (child_obj == 0) {
        mkproc_die();
    }

    if (matcher->flags_09.bits.copy_child_flip != 0) {
        child_obj->hide_flag_bits.bit6 = parent_obj->hide_flag_bits.bit6;
    }

    child_offset = matcher->child_offset;
    child_bid = matcher->fake_child_bid;
    if (child_obj->hide_flag_bits.bit6 != 0) {
        child_offset.x *= -1.0f;
        bone_map = (FlippedBoneMap*)child_obj->flipped_bones;
        if (bone_map != 0 && child_bid < bone_map->count) {
            child_bid = bone_map->bone_indices[child_bid];
        }
    }

    parent_offset = matcher->parent_offset;
    parent_bid = matcher->parent_bid;
    if (matcher->flags_09.bits.use_unmirrored_parent == 0 &&
        parent_obj->hide_flag_bits.bit6 != 0) {
        parent_offset.x *= -1.0f;
        bone_map = (FlippedBoneMap*)parent_obj->flipped_bones;
        if (bone_map != 0 && parent_bid < bone_map->count) {
            mapped_bid = bone_map->bone_indices[parent_bid];
            if (mapped_bid != parent_bid) {
                parent_bid = mapped_bid;
                if (matcher->flags_08.bits.preserve_bone_matrix == 0) {
                    child_bone = child_obj->bones[child_bid];
                    if (child_bone != 0 &&
                        child_bone->parent_matrix != 0) {
                        memcpy(
                            child_bone->parent_matrix,
                            &matcher->flipped_child_matrix,
                            0x30);
                    }
                }
            }
        }
    } else if (matcher->flags_08.bits.preserve_bone_matrix == 0) {
        child_bone = child_obj->bones[child_bid];
        if (child_bone != 0 && child_bone->parent_matrix != 0) {
            memcpy(
                child_bone->parent_matrix,
                &matcher->child_matrix,
                0x30);
        }
    }

    child_bone = child_obj->bones[child_bid];
    parent_bone = parent_obj->bones[parent_bid];
    if (child_bone == 0 || parent_bone == 0) {
        mkproc_die();
    }

    if (matcher->flags_08.bits.copy_clone_matrix != 0) {
        raw_clone_obj = matcher->clone_obj;
        if (raw_clone_obj != 0) {
            if (raw_clone_obj->hdr.instance == matcher->clone_instance) {
                clone_obj = raw_clone_obj;
            } else {
                clone_obj = 0;
            }
        } else {
            clone_obj = 0;
        }
        if (clone_obj == 0) {
            mkproc_die();
        }
        memcpy(&clone_obj->frame->ltm, parent_bone, 0x30);
        RwFrameUpdateObjects(clone_obj->frame);
    }

    if (matcher->flags_08.bits.copy_parent_angles != 0) {
        child_obj->ang.x = parent_obj->ang.x;
        child_obj->ang.y = parent_obj->ang.y;
        child_obj->ang.z = parent_obj->ang.z;
        child_obj->flags_08_bits.transform_dirty = 1;
        update_mkobj(
            child_obj != 0 ? as_mkhdr(&child_obj->hdr) : 0);
    }
    if (matcher->flags_08.bits.flip_parent_angle_y != 0) {
        child_obj->ang.x = parent_obj->ang.x;
        child_obj->ang.y = parent_obj->ang.y;
        child_obj->ang.z = parent_obj->ang.z;
        child_obj->ang.y += 3.1415927f;
        child_obj->flags_08_bits.transform_dirty = 1;
        update_mkobj(
            child_obj != 0 ? as_mkhdr(&child_obj->hdr) : 0);
    }

    if (matcher->flags_08.bits.copy_bone_matrix != 0) {
        child_matrix = child_obj->field_24;
        memcpy(child_matrix, parent_bone, 0x30);
        if ((parent_bone->flags_55 & 0x40) != 0) {
            child_matrix->right.x = 0.0001f;
            child_matrix->up.y = 0.0001f;
            child_matrix->at.z = 0.0001f;
        }
        RwFrameUpdateObjects(child_obj->frame);
    }

    if (matcher->flags_08.bits.blend_child_transform != 0 &&
        child_bone->update_tick != (unsigned int)exec_tick_ctr) {
        if (child_obj->hide_flag_bits.bit6 != 0) {
            target_rotation = &matcher->mirrored_parent_rotation;
            target_translation = &matcher->mirrored_parent_translation;
        } else {
            target_rotation = &matcher->parent_rotation;
            target_translation = &matcher->parent_translation;
        }

        composed_rotation.x =
            -(parent_bone->rotation_90.z * target_rotation->y -
              (parent_bone->rotation_90.y * target_rotation->z +
               parent_bone->rotation_90.w * target_rotation->x +
               parent_bone->rotation_90.x * target_rotation->w));
        composed_rotation.y =
            -(parent_bone->rotation_90.x * target_rotation->z -
              (parent_bone->rotation_90.z * target_rotation->x +
               parent_bone->rotation_90.w * target_rotation->y +
               parent_bone->rotation_90.y * target_rotation->w));
        composed_rotation.z =
            -(parent_bone->rotation_90.y * target_rotation->x -
              (parent_bone->rotation_90.x * target_rotation->y +
               parent_bone->rotation_90.w * target_rotation->z +
               parent_bone->rotation_90.z * target_rotation->w));
        composed_rotation.w =
            -(parent_bone->rotation_90.z * target_rotation->z -
              -(parent_bone->rotation_90.y * target_rotation->y -
                (parent_bone->rotation_90.w * target_rotation->w -
                 parent_bone->rotation_90.x * target_rotation->x)));

        interp_quat(
            &child_bone->rotation,
            &composed_rotation,
            &child_bone->rotation,
            0.2f);
        interp_v3(
            &matcher->child_offset,
            target_translation,
            &matcher->child_offset,
            0.2f);
        update_bone_hierarchy(
            child_obj != 0 ? as_mkhdr(&child_obj->hdr) : 0);
    }

    if (matcher->flags_09.bits.snap_child_transform != 0 &&
        child_bone->update_tick != (unsigned int)exec_tick_ctr) {
        if (child_obj->hide_flag_bits.bit6 != 0) {
            target_rotation = &matcher->mirrored_parent_rotation;
            target_translation = &matcher->mirrored_parent_translation;
        } else {
            target_rotation = &matcher->parent_rotation;
            target_translation = &matcher->parent_translation;
        }

        composed_rotation.x =
            -(parent_bone->rotation_90.z * target_rotation->y -
              (parent_bone->rotation_90.y * target_rotation->z +
               parent_bone->rotation_90.w * target_rotation->x +
               parent_bone->rotation_90.x * target_rotation->w));
        composed_rotation.y =
            -(parent_bone->rotation_90.x * target_rotation->z -
              (parent_bone->rotation_90.z * target_rotation->x +
               parent_bone->rotation_90.w * target_rotation->y +
               parent_bone->rotation_90.y * target_rotation->w));
        composed_rotation.z =
            -(parent_bone->rotation_90.y * target_rotation->x -
              (parent_bone->rotation_90.x * target_rotation->y +
               parent_bone->rotation_90.w * target_rotation->z +
               parent_bone->rotation_90.z * target_rotation->w));
        composed_rotation.w =
            -(parent_bone->rotation_90.z * target_rotation->z -
              -(parent_bone->rotation_90.y * target_rotation->y -
                (parent_bone->rotation_90.w * target_rotation->w -
                 parent_bone->rotation_90.x * target_rotation->x)));

        interp_quat(
            &child_bone->rotation,
            &composed_rotation,
            &child_bone->rotation,
            1.0f);
        interp_v3(
            &matcher->child_offset,
            target_translation,
            &matcher->child_offset,
            1.0f);
        update_bone_hierarchy(
            child_obj != 0 ? as_mkhdr(&child_obj->hdr) : 0);
    }

    if (child_obj->flags_08_bits.moving != 0) {
        mat_scaled_by_v3(
            child_obj->field_24,
            child_obj->field_24,
            &child_obj->scale);
        RwFrameUpdateObjects(child_obj->frame);
    }

    get_bone_offset_world_pos(
        parent_obj, parent_bid, &parent_offset, &parent_pos);
    get_bone_offset_world_pos(
        child_obj, child_bid, &child_offset, &child_pos);
    correction_x = parent_pos.x - child_pos.x;
    correction_y = parent_pos.y - child_pos.y;
    correction_z = parent_pos.z - child_pos.z;

    if (matcher->blend_ticks > 0.0f) {
        fraction = game_speed / (1.0f + matcher->blend_ticks);
        matcher->blend_ticks -= game_speed;
        correction_x *= fraction;
        correction_y *= fraction;
        correction_z *= fraction;
    } else {
        if (matcher->flags_08.bits.release_parent_weight != 0) {
            matcher->child_weight = 0.0f;
        }
    }

    if (matcher->child_weight != 0.0f) {
        parent_weight = -matcher->child_weight;
        child_weight = 1.0f - matcher->child_weight;

        parent_delta_x = correction_x * parent_weight;
        parent_delta_y = correction_y * parent_weight;
        parent_delta_z = correction_z * parent_weight;
        child_delta_x = correction_x * child_weight;
        child_delta_y = correction_y * child_weight;
        child_delta_z = correction_z * child_weight;
        parent_obj->pos.x += parent_delta_x;
        parent_obj->pos.y += parent_delta_y;
        parent_obj->pos.z += parent_delta_z;
        child_obj->pos.x += child_delta_x;
        child_obj->pos.y += child_delta_y;
        child_obj->pos.z += child_delta_z;
        update_obj_pos(child_obj);
        update_obj_pos(parent_obj);
    } else {
        child_obj->pos.x += correction_x;
        child_obj->pos.y += correction_y;
        child_obj->pos.z += correction_z;
        update_obj_pos(child_obj);
    }
    update_bone_hierarchy(
        child_obj != 0 ? as_mkhdr(&child_obj->hdr) : 0);
    return 1.0f;
}

static void* atomic_set_HAnimHierarchy(void* atomic, void* hierarchy) {
    RpSkinAtomicSetHAnimHierarchy(atomic, hierarchy);
    return atomic;
}

/* Soft ceiling: get_child_frame_hierarchy ~82.35% - typed recursion retained. */
static RwFrame* get_child_frame_hierarchy(RwFrame* frame, void* out_data) {
    void* hierarchy = RpHAnimFrameGetHierarchy(frame);

    if (hierarchy == 0) {
        RwFrameForAllChildren(frame, get_child_frame_hierarchy, out_data);
        return frame;
    }
    *(void**)out_data = hierarchy;
    return frame;
}

/* Soft ceiling: mkbone_insert_child_of_parent ~92.38% - typed list walk retained. */
void mkbone_insert_child_of_parent(MkBone* child, MkBone* parent) {
    MkBone* tail;

    if (child->tree_next != 0) {
        return;
    }
    parent->flags_54 |= 1;
    if (parent->tree_child == 0) {
        parent->tree_child = child;
    } else {
        tail = parent->tree_child;
        while (tail->tree_next != 0) {
            tail = tail->tree_next;
        }
        tail->tree_next = child;
    }
    child->transform_parent = parent;
}

static inline int update_anim_frame(
    AnimState* anim,
    AnimScript* script,
    unsigned int flags,
    float* frame,
    float* low_frame,
    float* high_frame,
    float* step,
    float* step_accel,
    void (*callback)(AnimState*, float*),
    Vec* anim_offset,
    float* anim_angle,
    int old_anim) {
    float last_frame = anim_last_frame(script);
    unsigned int mode = flags & 7;
    int changed = 0;

    if (*low_frame < 0.0f) {
        *low_frame = 0.0f;
    }
    if (*low_frame > last_frame) {
        *low_frame = last_frame;
    }
    if (*high_frame < *low_frame) {
        *high_frame = *low_frame;
    }
    if (*high_frame > last_frame) {
        *high_frame = last_frame;
    }
    select_flip_map(anim, flags);

    if (*frame < *low_frame) {
        if (mode < 2) {
            *frame += *high_frame - *low_frame;
            apply_script_loop_offset(
                anim, script, anim_offset, anim_angle, 1);
        } else if (mode == 2) {
            *frame = 2.0f * *low_frame - *frame;
            if (*step < 0.0f) {
                *step = -*step;
                *step_accel = -*step_accel;
            }
        } else if (mode == 3) {
            *frame = *low_frame;
        }
        if (mode < 5) {
            changed = 1;
        }
    } else if (*frame > *high_frame) {
        if (mode < 2) {
            *frame -= (1.0f + *high_frame) - *low_frame;
            apply_script_loop_offset(
                anim, script, anim_offset, anim_angle, -1);
        } else if (mode == 2) {
            *frame = 2.0f * *high_frame - *frame;
            if (*step > 0.0f) {
                *step = -*step;
                *step_accel = -*step_accel;
            }
        } else if (mode == 3) {
            *frame = *high_frame;
        }
        if (mode < 5) {
            changed = 1;
        }
    }
    if (changed != 0 && callback != 0) {
        callback(anim, frame);
    }
    if (*frame < 0.0f) {
        *frame = 0.0f;
    }
    if (*frame > last_frame + 1.0f) {
        *frame = last_frame + 1.0f;
    }
    return old_anim != 0 ? 1 : changed == 0;
}

/* Soft ceiling: shared range/wrap algorithm recovered with typed state. */
static int _set_old_frameno(AnimState* anim) {
    return update_anim_frame(
        anim,
        anim->old_script,
        anim->old_flags,
        &anim->old_frame,
        &anim->old_low_frame,
        &anim->old_high_frame,
        &anim->old_step,
        &anim->old_step_accel,
        anim->old_frame_callback,
        &anim->old_anim_offset,
        &anim->old_anim_angle,
        1);
}

/* Soft ceiling: shared range/wrap algorithm recovered with typed state. */
static int _set_frameno(AnimState* anim) {
    return update_anim_frame(
        anim,
        anim->script,
        anim->flags,
        &anim->frame,
        &anim->low_frame,
        &anim->high_frame,
        &anim->step,
        &anim->step_accel,
        anim->frame_callback,
        &anim->anim_offset,
        &anim->anim_angle,
        0);
}

static inline void rebuild_anim_track_table(AnimState* anim) {
    AnimScript* script = anim->script;
    int track_count = script->track_count;
    int i;
    AnimTrack* track;
    void** track_data;

    if (anim->track_capacity < track_count) {
        void** table = (void**)get_mem(track_count * 8);

        if (anim->track_capacity != 0) {
            memcpy(
                table + track_count,
                anim->track_data,
                anim->track_capacity * 4);
            free_mem(anim->track_data);
        }
        anim->track_data = table;
        anim->track_capacity = track_count;
    } else {
        memcpy(
            anim->track_data + anim->track_capacity,
            anim->track_data,
            anim->track_capacity * 4);
    }
    track = script->tracks;
    track_data = anim->track_data;
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
    for (i = 0; i < track_count; i++, track++) {
        track_data[i] = anim_script_data(script, track->data_offset);
    }
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset
    anim->tag_frame =
        (AnimTagFrame*)anim_script_data(script, track->data_offset);
    anim->tag_frame = (AnimTagFrame*)anim_script_data(
        script, script->tag_data_offset);
}

static inline void initialize_anim_script(
    AnimState* anim,
    AnimScript* script,
    float frame,
    unsigned int flags) {
    if ((script->flags & 1) != 0) {
        anim->flags |= 0x2000;
    } else {
        anim->flags &= ~0x2000;
    }
    anim->script = script;
    anim->flags = flags | (anim->flags & 0x2000);
    anim->previous_frame = frame;
    anim->frame = frame;
    anim->low_frame = 0.0f;
    anim->high_frame = (float)(script->frame_count - 1);
    if ((flags & 7) != 4) {
        anim->frame_callback = 0;
    }
    rebuild_anim_track_table(anim);
}

/*
 * Soft ceiling: transition_to_anim_script_frame ~77.34% - retail uses a
 * dynamically aligned stack frame; the typed algorithm and object/bone state
 * transitions are retained.
 */
int transition_to_anim_script_frame(
    float transition_frames,
    float frame,
    AnimState* anim,
    AnimScript* script,
    unsigned int flags) {
    MkObj* obj;
    MkPtr* child_link;
    unsigned int i;
    float speed;
    float frame_step;
    float maximum_step;
    float saved_step;
    int maximum_step_int;

    if (script == 0) {
        return 0;
    }
    if (anim->script == 0) {
        return set_anim_script_frame(0.0f, anim, script, flags);
    }

    anim->old_script = anim->script;
    anim->old_flags = anim->flags;
    anim->old_step = anim->step;
    anim->old_step_accel = anim->step_accel;
    anim->old_frame = anim->frame;
    anim->old_high_frame = anim->high_frame;
    anim->old_low_frame = anim->low_frame;
    anim->old_anim_offset.x = anim->anim_offset.x;
    anim->old_anim_offset.y = anim->anim_offset.y;
    anim->old_anim_offset.z = anim->anim_offset.z;
    anim->old_anim_angle = anim->anim_angle;
    anim->old_field_9C = anim->field_60;
    anim->old_obj_movement_weight = anim->obj_movement_weight;
    anim->old_root_movement_weight = anim->root_movement_weight;
    anim->old_frame_callback = anim->frame_callback;

    anim->transition_step = transition_frames;
    if ((script->flags & 1) != 0) {
        anim->flags |= 0x2000;
    } else {
        anim->flags &= ~0x2000;
    }
    anim->script = script;
    anim->flags = flags | (anim->flags & 0x2000);
    anim->previous_frame = frame;
    anim->frame = frame;
    anim->low_frame = 0.0f;
    anim->high_frame = (float)(anim->script->frame_count - 1);
    if ((flags & 7) != 4) {
        anim->frame_callback = 0;
    }
    rebuild_anim_track_table(anim);
    if ((flags & 0x10) != 0) {
        anim->frame =
            anim->old_frame /
            (float)(anim->old_script->frame_count - 1) *
            (float)(anim->script->frame_count - 1);
        anim->previous_frame = anim->frame;
    }

    obj = anim_state_obj(anim);
    if ((flags & 0x80) == 0 && obj != 0) {
        if ((flags & 0x200) != 0) {
            obj->hide_flags = (obj->hide_flags & ~1) | 1;
        }
        if ((flags & 0x100) != 0 ||
            (obj->flags_0B & 0x80) != 0 ||
            (anim->transition_weight > 0.0f &&
             anim->transition_weight < 1.0f)) {
            if ((obj->flags_0B & 0x80) != 0) {
                MkBone* root = obj->bones[obj->fallback_bone_index];

                if (root != 0 && root->parent_matrix != 0) {
                    Mtx yaw_matrix;
                    Vec adjusted_position;
                    Quat correction;
                    Quat old_rotation;
                    float root_yaw = quat_extract_ang_y(&root->rotation);

                    obj->ang.y += root_yaw - obj->bone_angle_68;
                    y_angle_to_MKMATRIX((MKMATRIX*)yaw_matrix, root_yaw);
                    v3_x_mat_sub_v3(
                        &adjusted_position,
                        &root->translation,
                        (MKMATRIX*)yaw_matrix,
                        &obj->ground_restore_pos);
                    scale_v3(
                        &obj->ground_restore_pos,
                        &adjusted_position,
                        -1.0f);
                    obj->bone_angle_68 = root_yaw;
                    y_angle_to_MKMATRIX(
                        (MKMATRIX*)yaw_matrix,
                        obj->bone_angle_64 - root_yaw);
                    if (RtQuatConvertFromMatrix(
                            (RtQuat*)&correction,
                            (RwMatrix*)yaw_matrix)) {
                        gxQuatCopy(&old_rotation, &root->rotation);
                        gxQuatMul(
                            &root->rotation, &correction, &old_rotation);
                        gxQuatNorm(&root->rotation);
                    }
                }
                obj->flags_0B &= ~0x80;
            }
            for (i = 0; i < obj->bone_count; i++) {
                MkBone* bone = obj->bones[i];

                if (bone != 0) {
                    bone->rotation_e0 = bone->rotation;
                }
            }
            child_link = obj->list_44;
            while (child_link != 0) {
                MkObj* child = (MkObj*)child_link->hdr;

                if (child->hdr.instance != child_link->instance) {
                    MkPtr* next = child_link->next;

                    child_link->hdr = 0;
                    destroy_mkptr(child_link);
                    child_link = next;
                } else {
                    for (i = 0; i < child->bone_count; i++) {
                        MkBone* bone = child->bones[i];

                        if (bone != 0) {
                            bone->rotation_e0 = bone->rotation;
                        }
                    }
                    child_link = child_link->next;
                }
            }
            anim->flags |= 0x100;
        }
    }

    anim->transition_weight = 0.0f;
    anim->transition_accel = 0.0f;
    saved_step = anim->step;
    anim->step = 1.0f;

    obj = anim_state_obj(anim);
    speed = game_speed;
    if (obj != 0 && (obj->flags_0B & 0x20) != 0) {
        speed = 1.0f;
    }
    anim->hand_transition += anim->hand_transition_step * speed;
    if (anim->hand_transition <= 0.0f) {
        anim->hand_transition = 0.0f;
    } else {
        if (anim->hand_transition > anim->hand_transition_limit) {
            anim->hand_transition = anim->hand_transition_limit;
        }
        if (anim->transition_weight < 1.0f) {
            anim->transition_weight +=
                speed * (anim->transition_step + anim->transition_accel);
            if (anim->transition_weight > 1.0f) {
                anim->transition_weight = 1.0f;
                anim->transition_step = 0.0f;
                anim->transition_accel = 0.0f;
                anim->flags &= ~0x100;
            }
        }
        if (anim->transition_weight < 1.0f) {
            maximum_step = (float)anim->old_script->frame_count / 3.0f;
            maximum_step_int = (int)maximum_step;
            frame_step = speed * (anim->old_step + anim->old_step_accel);
            if (frame_step > maximum_step) {
                frame_step = (float)maximum_step_int;
            } else if (frame_step < -maximum_step) {
                frame_step = (float)-maximum_step_int;
            }
            anim->old_frame += frame_step;
        }
        anim->previous_frame = anim->frame;
        maximum_step = (float)anim->script->frame_count / 3.0f;
        maximum_step_int = (int)maximum_step;
        frame_step = speed * (anim->step + anim->step_accel);
        if (frame_step > maximum_step) {
            frame_step = (float)maximum_step_int;
        } else if (frame_step < -maximum_step) {
            frame_step = (float)-maximum_step_int;
        }
        anim->frame += frame_step;
    }
    anim->frame = frame;
    anim->step = saved_step;
    obj = anim_state_obj(anim);
    if (obj == 0 || (obj->hide_flags & 2) == 0) {
        pose_anim(anim, 0);
    } else {
        float ground_restore_x = obj->ground_restore_pos.x;
        float ground_restore_y = obj->ground_restore_pos.y;
        float ground_restore_z = obj->ground_restore_pos.z;

        obj->hide_flags &= ~2;
        pose_anim(anim, 0);
        obj->ground_restore_pos.x = ground_restore_x;
        obj->ground_restore_pos.y = ground_restore_y;
        obj->ground_restore_pos.z = ground_restore_z;
    }
    return 1;
}

/*
 * Soft ceiling: set_anim_script_frame ~79.59% - retail uses a dynamically
 * aligned stack frame; the typed state, yaw correction, and pose flow remain.
 */
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int set_anim_script_frame(
    float frame,
    AnimState* anim,
    AnimScript* script,
    unsigned int flags) {
    MkObj* obj;
    int same_script = 1;

    if (script == 0) {
        return 0;
    }

    if ((script->flags & 1) != 0) {
        anim->flags |= 0x2000;
    } else {
        anim->flags &= ~0x2000;
    }
    anim->transition_step = 0.0f;
    anim->transition_accel = 0.0f;
    anim->transition_weight = 1.0f;
    if (anim->old_script == 0) {
        anim->old_script = script;
        if (mode_of_play == 7) {
            anim->old_frame = 0.0f;
        } else {
            anim->old_frame = frame;
        }
    }
    if (anim->script != script) {
        anim->script = script;
        same_script = 0;
    }
    anim->flags = flags | (anim->flags & 0x2000);
    anim->frame = frame;
    anim->previous_frame = frame;
    anim->low_frame = 0.0f;
    anim->high_frame = (float)(anim->script->frame_count - 1);
    rebuild_anim_track_table(anim);

    if ((flags & 0x10) != 0) {
        anim->frame =
            anim->old_frame /
            (float)(anim->old_script->frame_count - 1) *
            (float)(anim->script->frame_count - 1);
        anim->previous_frame = anim->frame;
    }

    obj = anim_state_obj(anim);
    if ((flags & 0x80) == 0 && obj != 0) {
        if ((flags & 0x200) != 0) {
            obj->hide_flags = (obj->hide_flags & ~1) | 1;
        }
        if ((obj->flags_0B & 0x80) != 0) {
            MkBone* root = obj->bones[obj->fallback_bone_index];

            if (root != 0 && root->parent_matrix != 0) {
                Mtx yaw_matrix;
                Vec adjusted_position;
                Quat correction;
                Quat old_rotation;
                float root_yaw = quat_extract_ang_y(&root->rotation);

                obj->ang.y += root_yaw - obj->bone_angle_68;
                y_angle_to_MKMATRIX((MKMATRIX*)yaw_matrix, root_yaw);
                v3_x_mat_sub_v3(
                    &adjusted_position,
                    &root->translation,
                    (MKMATRIX*)yaw_matrix,
                    &obj->ground_restore_pos);
                scale_v3(
                    &obj->ground_restore_pos,
                    &adjusted_position,
                    -1.0f);
                obj->bone_angle_68 = root_yaw;
                y_angle_to_MKMATRIX(
                    (MKMATRIX*)yaw_matrix,
                    obj->bone_angle_64 - root_yaw);
                if (RtQuatConvertFromMatrix(
                        (RtQuat*)&correction,
                        (RwMatrix*)yaw_matrix)) {
                    gxQuatCopy(&old_rotation, &root->rotation);
                    gxQuatMul(
                        &root->rotation, &correction, &old_rotation);
                    gxQuatNorm(&root->rotation);
                }
            }
            obj->flags_0B &= ~0x80;
        }
    }

    obj = anim_state_obj(anim);
    if (obj == 0 || (obj->hide_flags & 2) == 0) {
        pose_anim(anim, 0);
    } else {
        float ground_restore_x = obj->ground_restore_pos.x;
        float ground_restore_y = obj->ground_restore_pos.y;
        float ground_restore_z = obj->ground_restore_pos.z;

        obj->hide_flags &= ~2;
        pose_anim(anim, 0);
        obj->ground_restore_pos.x = ground_restore_x;
        obj->ground_restore_pos.y = ground_restore_y;
        obj->ground_restore_pos.z = ground_restore_z;
    }
    return same_script;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

#pragma dont_inline on
void set_anim_script(
    AnimState* anim,
    AnimScript* script,
    unsigned int flags) {
    set_anim_script_frame(0.0f, anim, script, flags);
}
#pragma dont_inline reset

/* Soft ceiling: anim_script_lastframe ~81.82% - typed field access retained. */
float anim_script_lastframe(const AnimScript* script) {
    return (float)(script->frame_count - 1);
}

void transition_to_anim_script(
    float transition_frames,
    AnimState* anim,
    AnimScript* script,
    unsigned int flags) {
    transition_to_anim_script_frame(
        transition_frames, 0.0f, anim, script, flags);
}

void anim_set_hiframe(AnimState* anim, float frame) {
    float last_frame;

    if (frame < 1.0f) {
        frame = 1.0f;
    }
    last_frame = (float)(anim->script->frame_count - 1);
    if (frame > last_frame) {
        frame = last_frame;
    }
    anim->high_frame = frame;
}

/* Soft ceiling: toggle_obj_and_ani_flips ~74.57% - typed flag updates retained. */
void toggle_obj_and_ani_flips(AnimState* anim) {
    MkObj* obj = anim->obj;

    if (obj != 0 && obj->hdr.instance != anim->obj_instance) {
        obj = 0;
    }
    obj->hide_flags ^= 0x40;
    anim->flags ^= 8;
    anim->old_flags ^= 8;
}

void vdestroy_mkpdata_anim(AnimState* anim) {
    if (anim->track_data != 0) {
        free_mem(anim->track_data);
    }
    anim->hdr.instance = 0;
    mkhdr_memfree(&anim->hdr);
}

/*
 * Soft ceiling: p_pose_handanim ~62.13% - retail keeps the inlined track-table
 * walk as one CTR loop; typed portable indexing is retained here.
 */
static float p_pose_handanim(void) {
    if (anim_pdata->hand_transition > 0.0f) {
        if (anim_pdata->hand_script != 0 &&
            anim_pdata->next_hand_script != 0 &&
            anim_pdata->hand_transition_frames < 1.0f &&
            anim_pdata->old_script != anim_pdata->next_hand_script) {
            anim_pdata->script = anim_pdata->next_hand_script;
            anim_pdata->flags = anim_pdata->hand_flags | 0x80;
            anim_pdata->frame = 0.0f;
            anim_pdata->low_frame = 0.0f;
            anim_pdata->high_frame =
                anim_last_frame(anim_pdata->script);
            anim_pdata->step = 1.0f;
            anim_pdata->frame_callback = 0;
            rebuild_anim_track_table(anim_pdata);

            transition_to_anim_script_frame(
                anim_pdata->hand_transition_frames,
                0.0f,
                anim_pdata,
                anim_pdata->hand_script,
                anim_pdata->hand_flags | 0x80);
        } else if (anim_pdata->hand_script != 0 &&
                   anim_pdata->script != anim_pdata->hand_script) {
            if (anim_pdata->hand_transition_frames < 1.0f) {
                transition_to_anim_script_frame(
                    anim_pdata->hand_transition_frames,
                    0.0f,
                    anim_pdata,
                    anim_pdata->hand_script,
                    anim_pdata->hand_flags | 0x80);
            } else {
                set_anim_script_frame(
                    0.0f,
                    anim_pdata,
                    anim_pdata->hand_script,
                    anim_pdata->hand_flags | 0x80);
            }
        } else if (anim_pdata->script != 0) {
            anim_pdata->transition_weight =
                anim_pdata->hand_transition_frames;
            if (anim_pdata->script == anim_pdata->old_script ||
                anim_pdata->old_script == 0) {
                anim_pdata->transition_weight = 1.0f;
            }
            anim_pdata->old_frame = 0.0f;
            anim_pdata->frame = 0.0f;
            pose_anim(anim_pdata, 1);
        }
    }

    ((AnimProcVtable*)aproc->vtbl)->jump_sleep(
        (MkProcEntryFn)p_anim_idle, 0.0f);
    return 0.0f;
}

static inline void apply_pending_anim_tags(
    AnimState* anim, MkObj* obj, AnimScript* script) {
    AnimTagFrame* first;
    AnimTagFrame* last;
    int frame;

    if (script->tag_data_offset == 0 || anim->step < 0.0f) {
        return;
    }

    first = (AnimTagFrame*)anim_script_data(
        script, script->tag_data_offset);
    last = (AnimTagFrame*)anim_script_data(
        script, script->tag_end_offset) - 1;
    frame = (int)(anim->previous_frame + 0.5f);

    while (anim->tag_frame->frame >= frame &&
           anim->tag_frame != first) {
        anim->tag_frame--;
    }
    while (anim->tag_frame->frame < frame &&
           anim->tag_frame != last) {
        anim->tag_frame++;
    }
    if (anim->frame < anim->previous_frame) {
        while (anim->tag_frame <= last) {
            apply_tag_frame(anim, obj);
            anim->tag_frame++;
        }
        anim->tag_frame = first;
    }

    frame = (int)(anim->frame + 0.5f);
    while (anim->tag_frame->frame <= frame) {
        apply_tag_frame(anim, obj);
        if (anim->tag_frame == last) {
            break;
        }
        anim->tag_frame++;
    }
}

static inline int anim_channel_frame_size(int type) {
    switch (type) {
    case 1:
    case 4:
    case 9:
    case 10:
        return 8;
    case 2:
    case 12:
        return 12;
    case 3:
    case 7:
    case 11:
        return 20;
    case 5:
    case 6:
    case 8:
        return 4;
    }
    return 0;
}

static inline float anim_frame_fraction(void) {
    float span = mka_next_fno - mka_prev_fno;

    if (span == 0.0f) {
        return 0.0f;
    }
    return (mka_sought_fno - mka_prev_fno) / span;
}

static inline void decode_anim_vec(Vec* out, const AnimVecFrame* frame) {
    const float scale = 0.0001f;

    out->x = flip_factor * scale * (float)frame->x;
    out->y = scale * (float)frame->y;
    out->z = scale * (float)frame->z;
}

static inline void decode_anim_quat(Quat* out, const AnimQuatFrame* frame) {
    const float scale = 4.6566129e-10f;

    out->w = scale * (float)frame->x;
    out->x = flip_factor * scale * (float)frame->y;
    out->y = flip_factor * scale * (float)frame->z;
    out->z = scale * (float)frame->w;
}

static inline int anim_sign_extend_12(unsigned int value) {
    return (int)(value << 20) >> 20;
}

static inline void decode_anim_packed_quat_values(
    Quat* out, short packed_wx, unsigned int packed) {
    const float scale = 0.00048828125f;
    int packed_x =
        ((int)packed >> 24) * 16 +
        ((unsigned short)packed_wx & 0xF);
    int packed_y =
        anim_sign_extend_12((packed << 8) | (packed >> 24));
    int packed_z =
        anim_sign_extend_12((packed << 20) | (packed >> 12));

    out->w = scale * (float)(packed_wx >> 4);
    out->x = flip_factor * scale * (float)packed_x;
    out->y = flip_factor * scale * (float)packed_y;
    out->z = scale * (float)packed_z;
}

static inline void decode_anim_packed_quat(
    Quat* out, const AnimPackedQuatFrame* frame) {
    decode_anim_packed_quat_values(
        out, frame->packed_wx, frame->packed_xyz);
}

static inline MkObj* live_anim_object(const AnimObjectLatch* latch) {
    if (latch->obj == 0 ||
        latch->obj->hdr.instance != latch->instance) {
        return 0;
    }
    return latch->obj;
}

static inline MkProc* live_anim_proc(const AnimProcLatch* latch) {
    if (latch->proc == 0 ||
        latch->proc->instance != latch->instance) {
        return 0;
    }
    return latch->proc;
}

/*
 * Soft ceiling: pose_anim - outer state/tags, transform channels, tracked-bone
 * channel 6, and pose-process channel 7 are recovered. Hand-process channel 5
 * still depends on the undocumented owner hand-animation process table.
 */
int pose_anim(AnimState* anim, int update_object) {
    AnimScript* script;
    AnimOwnerView* owner;
    AnimOwnerChannelSet* channel_set;
    MkObj* channel_objects[4];
    MkObj* obj;
    float transition_weight;
    float pass_weight;
    float channel_weight;
    unsigned int flags;
    int transition_pass;
    int result;

    result = 1;
    if (anim->last_update_tick == (unsigned int)exec_tick_ctr) {
        anim->updates_this_tick++;
    } else {
        anim->last_update_tick = exec_tick_ctr;
        anim->updates_this_tick = 1;
    }

    owner = (AnimOwnerView*)anim->owner;
    if (owner != 0 &&
        owner->hdr.instance != anim->owner_instance) {
        owner = 0;
    }
    obj = anim_state_obj(anim);
    if (obj == 0) {
        return result;
    }
    channel_objects[0] = obj;
    if (owner != 0 && owner->channel_set != 0) {
        channel_set = owner->channel_set;
        channel_objects[1] =
            live_anim_object(&channel_set->primary);
        channel_objects[2] =
            live_anim_object(&channel_set->secondary);
        channel_objects[3] =
            live_anim_object(&owner->tertiary);
    } else {
        channel_objects[1] = 0;
        channel_objects[2] = 0;
        channel_objects[3] = 0;
    }

    transition_weight = anim->transition_weight;
    transition_pass = 0;
    if (transition_weight < 1.0f) {
        float eased_weight;

        transition_pass = 1;
        if (transition_weight <= 0.5f) {
            eased_weight =
                0.5f - 0.5f * gxMathCos(3.1415927f * transition_weight);
        } else {
            eased_weight =
                0.5f +
                0.5f *
                    gxMathSin(
                        3.1415927f * (transition_weight - 0.5f));
        }
        transition_weight =
            0.5f * anim->transition_weight + 0.5f * eased_weight;
    }

    do {
        if (transition_pass != 0) {
            _set_old_frameno(anim);
            script = anim->old_script;
            mka_sought_fno = anim->old_frame;
            pass_weight = 1.0f - transition_weight;
            flags = anim->old_flags;
        } else {
            result = _set_frameno(anim);
            script = anim->script;
            if (script == 0) {
                return result;
            }
            mka_sought_fno = anim->frame;
            pass_weight = transition_weight;
            flags = anim->flags;
            apply_pending_anim_tags(anim, obj, script);
        }

        mka_hdr = (unsigned short*)script;
        channel_weight = anim->hand_transition * pass_weight;
        {
            unsigned int i;
            unsigned int track_base =
                transition_pass != 0 ? anim->track_capacity : 0;

            select_flip_map(anim, flags);
            mka_channel_hdr =
                (AnimChannelHeader*)script->tracks;
            for (i = 0; i < (unsigned int)script->track_count;
                 i++, mka_channel_hdr++) {
                AnimChannelHeader* channel = mka_channel_hdr;
                unsigned int group =
                    (channel->target >> 16) & 0xF;
                unsigned int bone_index =
                    channel->target & 0xFFFF;
                MkObj* channel_obj;
                MkBone* bone;

                mka_bytes_per_frame =
                    anim_channel_frame_size(channel->type);
                if (mka_bytes_per_frame == 0 || group >= 4) {
                    continue;
                }
                channel_obj = channel_objects[group];
                if (channel_obj == 0) {
                    continue;
                }
                if (flipped_bones != 0 &&
                    bone_index < flipped_bones->count) {
                    bone_index =
                        flipped_bones->bone_indices[bone_index];
                }
                if (bone_index >= channel_obj->bone_count) {
                    continue;
                }
                bone = channel_obj->bones[bone_index];
                if (bone == 0 || bone->parent_matrix == 0) {
                    continue;
                }

                if (bone->update_tick !=
                    (unsigned int)exec_tick_ctr) {
                    bone->update_tick = exec_tick_ctr;
                    bone->field_60 = 0.0f;
                    bone->field_64 = 0.0f;
                }

                if ((flags & 0x2000) == 0) {
                    unsigned short* current =
                        (unsigned short*)
                            anim->track_data[track_base + i];

                    current = find_frame(current);
                    anim->track_data[track_base + i] = current;
                }

                if (channel->type == 1 ||
                    channel->type == 9 ||
                    channel->type == 10) {
                    Vec previous;
                    Vec next;
                    Vec value;
                    float fraction = anim_frame_fraction();

                    decode_anim_vec(
                        &previous, (AnimVecFrame*)mka_prev_fp);
                    decode_anim_vec(
                        &next, (AnimVecFrame*)mka_next_fp);
                    interp_v3(
                        &value,
                        &previous,
                        &next,
                        1.0f - fraction);
                    if (bone->field_64 == 0.0f) {
                        bone->translation = value;
                        bone->field_64 = channel_weight;
                    } else {
                        float combined_weight =
                            bone->field_64 + channel_weight;

                        interp_v3(
                            &bone->translation,
                            &bone->translation,
                            &value,
                            bone->field_64 / combined_weight);
                        bone->field_64 = combined_weight;
                    }
                } else if (
                    channel->type == 3 || channel->type == 11) {
                    Quat previous;
                    Quat next;
                    Quat value;
                    float fraction = anim_frame_fraction();

                    decode_anim_quat(
                        &previous, (AnimQuatFrame*)mka_prev_fp);
                    decode_anim_quat(
                        &next, (AnimQuatFrame*)mka_next_fp);
                    gxQuatInterpQuat(
                        &value, &previous, &next, fraction);
                    if (bone->field_60 == 0.0f) {
                        bone->rotation = value;
                        bone->field_60 = channel_weight;
                    } else {
                        float combined_weight =
                            bone->field_60 + channel_weight;

                        gxQuatInterpQuat(
                            &bone->rotation,
                            &bone->rotation,
                            &value,
                            channel_weight / combined_weight);
                        bone->field_60 = combined_weight;
                    }
                } else if (channel->type == 4) {
                    Quat previous;
                    Quat next;
                    Quat value;
                    float fraction = anim_frame_fraction();

                    decode_anim_packed_quat(
                        &previous,
                        (AnimPackedQuatFrame*)mka_prev_fp);
                    decode_anim_packed_quat(
                        &next,
                        (AnimPackedQuatFrame*)mka_next_fp);
                    gxQuatInterpQuat(
                        &value, &previous, &next, fraction);
                    if (bone->field_60 == 0.0f) {
                        bone->rotation = value;
                        bone->field_60 = channel_weight;
                    } else {
                        float combined_weight =
                            bone->field_60 + channel_weight;

                        gxQuatInterpQuat(
                            &bone->rotation,
                            &bone->rotation,
                            &value,
                            channel_weight / combined_weight);
                        bone->field_60 = combined_weight;
                    }
                } else if (channel->type == 6) {
                    AnimScalarFrame* frame =
                        (AnimScalarFrame*)mka_prev_fp;

                    if ((flags & 0x400) != 0 &&
                        channel_weight >= 0.0f) {
                        channel_obj->ground_bone =
                            (unsigned short)frame->value;
                        if (channel_obj->ground_bone == 0xFFFF) {
                            channel_obj->hide_flags &= ~2;
                        } else {
                            channel_obj->hide_flags |= 2;
                        }
                        if ((channel_obj->hide_flags & 2) != 0) {
                            if (flipped_bones != 0 &&
                                (unsigned int)channel_obj->ground_bone <
                                    flipped_bones->count) {
                                channel_obj->ground_bone =
                                    flipped_bones->bone_indices[
                                        channel_obj->ground_bone];
                            }
                            get_bone_world_pos(
                                channel_obj,
                                channel_obj->ground_bone,
                                &channel_obj->ground_restore_pos);
                        }
                    }
                } else if (channel->type == 7) {
                    AnimPoseFrame* frame =
                        (AnimPoseFrame*)mka_prev_fp;
                    MkProc* pose_proc = 0;

                    if (owner != 0) {
                        if (group == 1 && owner->pose_set != 0) {
                            pose_proc = live_anim_proc(
                                &owner->pose_set->primary);
                        } else if (
                            group == 2 && owner->pose_set != 0) {
                            pose_proc = live_anim_proc(
                                &owner->pose_set->secondary);
                        } else if (group == 3) {
                            pose_proc =
                                live_anim_proc(&owner->tertiary_pose);
                        }
                    }
                    if (pose_proc != 0) {
                        AnimPoseProcData* pose =
                            (AnimPoseProcData*)pdata_of_proc(pose_proc);

                        if (group == 3 &&
                            transition_weight < 1.0f) {
                            pose->flags_08 |= 0x80;
                        } else {
                            const float pose_scale = 0.0001f;

                            pose->flags_08 &= ~0x80;
                            pose->bone =
                                frame->bone_and_flags & 0xFFF;
                            pose->position.x =
                                pose_scale *
                                (float)frame->position_x;
                            pose->position.y =
                                pose_scale *
                                (float)frame->position_y;
                            pose->position.z =
                                pose_scale *
                                (float)frame->position_z;
                            pose->pose_id = frame->pose_id;
                            pose->offset.x =
                                pose_scale *
                                (float)frame->offset_x;
                            pose->offset.y =
                                pose_scale *
                                (float)frame->offset_y;
                            pose->offset.z =
                                pose_scale *
                                (float)frame->offset_z;
                        }
                    }
                } else if (channel->type == 8) {
                    AnimScalarFrame* previous =
                        (AnimScalarFrame*)mka_prev_fp;
                    AnimScalarFrame* next =
                        (AnimScalarFrame*)mka_next_fp;
                    const float focal_scale = 0.01f;

                    set_camera_focal_length(
                        (focal_scale * (float)previous->value +
                         focal_scale * (float)next->value) *
                        0.5f);
                } else if (channel->type == 12) {
                    AnimMatrixFrame* frame =
                        (AnimMatrixFrame*)mka_prev_fp;
                    const float translation_scale = 0.0001f;

                    bone->parent_matrix->pos.x =
                        flip_factor *
                        translation_scale * (float)frame->x;
                    bone->parent_matrix->pos.y =
                        translation_scale * (float)frame->y;
                    bone->parent_matrix->pos.z =
                        translation_scale * (float)frame->z;
                    decode_anim_packed_quat_values(
                        &bone->rotation_90,
                        frame->packed_wx,
                        frame->packed_xyz);
                    gxQuatQuatToMat(
                        RW_MATRIX_MAT33(bone->parent_matrix),
                        &bone->rotation_90);
                    bone->flags_54 =
                        (bone->flags_54 & ~2) | 2;
                }
            }
        }
        apply_anim_offset(
            channel_weight,
            anim,
            obj,
            transition_pass != 0
                ? &anim->old_anim_offset
                : &anim->anim_offset,
            transition_pass,
            update_object);

        if (transition_pass == 0) {
            break;
        }
        transition_pass = 0;
    } while (1);

    return result;
}

static RpAtomic* ScanForBone_callback(
    RpAtomic* atomic, BoneScanContext* context);

/*
 * Soft ceiling: process_obj_bones ~68.59% - retail keeps an extra hierarchy
 * induction value in r17 and uses a 0xC60 stack frame; the typed mapping,
 * bind-matrix, and parent/child construction flow remains.
 */
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static void process_obj_bones(MkObj* obj, const int* tags) {
    HAnimHierarchyView* hierarchy = 0;
    MkBone* linked_bones[256];
    int bone_indices[256];
    MkBone* parent_stack[256];
    MkBone* parent = 0;
    RwMatrix* matrix;
    int bone_count;
    int hierarchy_count;
    int first_root;
    int next_unmapped;
    int stack_index;
    int i;

    RwFrameForAllChildren(
        (RwFrame*)obj->clump->object.parent,
        get_child_frame_hierarchy,
        &hierarchy);
    if (hierarchy == 0) {
        return;
    }
    hierarchy->flags |= 0x4000;
    RpClumpForAllAtomics(
        obj->clump,
        atomic_set_HAnimHierarchy,
        hierarchy);

    bone_count = obj->bone_count;
    hierarchy_count = hierarchy->node_count;
    first_root = bone_count;
    for (i = 0; i < bone_count; i++) {
        MkBone* bone = alloc_bone();

        obj->bones[i] = bone;
        if (bone != 0) {
            int tag;

            bone->bone_index = i;
            tag = tags[i];
            if (tag == 0) {
                first_root = i;
            }
            if (i < first_root && tag == -1) {
                bone->tag = -1;
            } else {
                bone->tag = -2;
            }
        }
    }

    for (i = 0; i < hierarchy_count; i++) {
        HAnimNodeInfo* node = &hierarchy->nodes[i];
        unsigned int node_id = node->node_id;
        int masked_id = node_id & 0xFFF07FFF;
        int limb_id = (node_id >> 16) & 0xF;
        int j;

        bone_indices[i] = -1;
        linked_bones[i] = 0;
        for (j = 0; ; j++) {
            MkBone* bone;

            if (tags[j] == 0 || j >= bone_count) {
                break;
            }
            if (tags[j] == -1 || masked_id != tags[j]) {
                continue;
            }
            bone = obj->bones[j];
            if ((node->node_id & 0x8000) != 0) {
                linked_bones[i] = bone;
            } else if (bone->tag == -2) {
                bone_indices[i] = j;
                bone->tag = node->node_id & 0xFFF0FFFF;
                bone->limb_id = limb_id;
            }
        }
    }

    matrix = hierarchy->matrices;
    next_unmapped = 0;
    stack_index = -1;
    for (i = 0; i < hierarchy_count; i++, matrix++) {
        HAnimNodeInfo* node = &hierarchy->nodes[i];
        MkBone* bone;
        BoneScanContext context;

        if (bone_indices[i] > -1) {
            bone = obj->bones[bone_indices[i]];
        } else {
            while (obj->bones[next_unmapped]->tag != -2) {
                next_unmapped++;
            }
            bone = obj->bones[next_unmapped];
            bone->tag = node->node_id & 0xFFF0FFFF;
            bone_indices[i] = next_unmapped;
            bone->limb_id = (node->node_id >> 16) & 0xF;
            if (linked_bones[i] != 0) {
                MkBone* linked = linked_bones[i];

                bone->clone_source = linked;
                bone->root_next = linked->root_next;
                linked->root_next = bone;
            }
        }

        if ((bone->tag & 0x1000) != 0) {
            bone->flags_54 |= 0x40;
        }
        if ((bone->tag & 0x2000) != 0) {
            bone->flags_54 |= 0x20;
        }
        bone->parent_matrix = matrix;
        bone->original_parent_matrix = matrix;
        matrix->flags = 3;
        bone->update_tick = exec_tick_ctr - 1;

        context.bone_index = i;
        RpClumpForAllAtomics(
            obj->clump, ScanForBone_callback, &context);
        if (context.matrix != 0) {
            bone->bind_offset.x = -context.matrix->pos.x;
            bone->bind_offset.y = -context.matrix->pos.y;
            bone->bind_offset.z = -context.matrix->pos.z;
        } else {
            bone->bind_offset.x = 0.0f;
            bone->bind_offset.y = 0.0f;
            bone->bind_offset.z = 0.0f;
        }

        if (parent != 0) {
            MkBone* tail;

            bone->translation.x =
                bone->bind_offset.x - parent->bind_offset.x;
            bone->translation.y =
                bone->bind_offset.y - parent->bind_offset.y;
            bone->translation.z =
                bone->bind_offset.z - parent->bind_offset.z;
            bone->field_5C = 1.0f / length_v3(&bone->translation);
            if (bone->tree_next == 0) {
                parent->flags_54 |= 1;
                if (parent->tree_child == 0) {
                    parent->tree_child = bone;
                } else {
                    tail = parent->tree_child;
                    while (tail->tree_next != 0) {
                        tail = tail->tree_next;
                    }
                    tail->tree_next = bone;
                }
                bone->transform_parent = parent;
            }
        } else {
            bone->translation.x = 0.0f;
            bone->translation.y = 0.0f;
            bone->translation.z = 0.0f;
            bone->field_5C = 0.0f;
            bone->transform_parent = 0;
        }

        if ((node->flags & 2) != 0) {
            parent_stack[++stack_index] = parent;
        }
        parent = bone;
        if ((node->flags & 1) != 0) {
            parent = parent_stack[stack_index--];
        }
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void build_bones_tbl(MkObj* obj, const int* tags) {
    HAnimHierarchyView* hierarchy;
    RwFrame* frame;
    int bone_count;
    const int* tag;

    if (obj->bones != 0) {
        return;
    }

    hierarchy = 0;
    frame = (RwFrame*)obj->clump->object.parent;
    RwFrameForAllChildren(
        frame, get_child_frame_hierarchy, (void*)&hierarchy);
    if (hierarchy == 0) {
        return;
    }

    bone_count = hierarchy->node_count;
    for (tag = tags; *tag != 0; tag++) {
        if (*tag == -1) {
            bone_count++;
        }
    }
    obj->bones = (MkBone**)get_mem(bone_count * sizeof(MkBone*));
    if (obj->bones == 0) {
        return;
    }
    obj->bone_count = bone_count;
    process_obj_bones(obj, tags);
    insert_bone_hierarchy_mkobj(obj);
    update_bone_hierarchy(as_mkhdr(&obj->hdr));
}

static RpAtomic* ScanForBone_callback(
    RpAtomic* atomic, BoneScanContext* context) {
    void* skin;

    context->matrix = 0;
    skin = RpSkinGeometryGetSkin(atomic->geometry);
    if (skin != 0) {
        context->matrix =
            RpSkinGetSkinToBoneMatrices(skin) + context->bone_index;
    }
    if (context->matrix != 0) {
        return 0;
    }
    return atomic;
}
