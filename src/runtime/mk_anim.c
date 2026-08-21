#include "runtime/mk_obj.h"
#include "runtime/anim_types.h"
#include "runtime/cam_api.h"
#include "runtime/cstdio.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "rw/rphanim.h"
#include "rw/rpskin.h"
#include "rw/rwcore_types.h"
#include "rw/rwframe.h"

#define RW_MATRIX_MAT33(matrix_) ((Mat33*)(matrix_))

static float p_morph(void);
static void do_morph(MkHdr* hdr);
static float p_bone_matcher(void);
float p_anim_idle(void);

MkProc* morph_proc;
int pose_morph(MkHdr* hdr);

typedef struct MorphScript {
    unsigned int frame_count;
    unsigned short* frame_table;
} MorphScript;

typedef struct MorphFrameHeader {
    unsigned short frame;
    unsigned char target;
    unsigned char next_target;
    float position;
} MorphFrameHeader;
typedef char MorphFrameHeaderSize[(sizeof(MorphFrameHeader) == 8) ? 1 : -1];

typedef struct MorphState {
    MkHdr hdr;
    unsigned int morph_target_count; /* +0x08 */
    RpAtomic* atomic;                /* +0x0C */
    RpInterpolator* interpolator;    /* +0x10 */
    MorphScript* script;             /* +0x14 */
    unsigned short* frame_table;     /* +0x18 */
    int frame_count;                 /* +0x1C */
    unsigned short* current_frame;   /* +0x20 */
    unsigned int flags;              /* +0x24 */
    float frame;
    float low_frame;                 /* +0x2C */
    float high_frame;                /* +0x30 */
    float frame_step;
    void (*frame_callback)(float*);
    MkPtr* field_3C;
} MorphState;
typedef char MorphStateSize[(sizeof(MorphState) == 0x40) ? 1 : -1];

static int set_morph_frameno(MorphState* morph);
static unsigned short* morph_find_frame(
    MorphState* morph, unsigned short* current);

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

typedef struct BoneMatcherState {
    MkHdr hdr;
    union {
        unsigned int flags_word_08;
        struct {
            BoneMatcherFlags08 flags_08;
            BoneMatcherFlags09 flags_09;
            unsigned char pad0A[2];
        };
    };
    float child_weight;
    MkObj* parent_obj;
    unsigned int parent_instance;
    int parent_bid;
    Vec parent_offset;
    MkObj* child_obj;
    unsigned int child_instance;
    MkSobj* clone_obj;
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
typedef char BoneMatcherStateSize[
    (sizeof(BoneMatcherState) == 0x110) ? 1 : -1];

typedef struct BoneScanContext {
    RwMatrix* matrix;
    int bone_index;
} BoneScanContext;

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

typedef union AnimPackedXY {
    unsigned short raw;
    struct {
        signed short x : 12;
        unsigned short y_low : 4;
    } bits;
} AnimPackedXY;

typedef union AnimPackedZW {
    unsigned short raw;
    struct {
        unsigned short z_low : 4;
        signed short w : 12;
    } bits;
} AnimPackedZW;

typedef struct AnimPackedQuatFrame {
    unsigned short frame;
    AnimPackedXY packed_xy;
    unsigned int packed_yzw;
} AnimPackedQuatFrame;

typedef struct AnimMatrixFrame {
    short x;
    short y;
    short z;
    AnimPackedXY packed_xy;
    union {
        unsigned int packed_yzw;
        struct {
            signed char packed_y_high;
            unsigned char packed_z_high;
            AnimPackedZW packed_zw;
        };
    };
} AnimMatrixFrame;
typedef char AnimPackedQuatFrameSize[
    (sizeof(AnimPackedQuatFrame) == 8) ? 1 : -1];
typedef char AnimMatrixFrameSize[(sizeof(AnimMatrixFrame) == 12) ? 1 : -1];

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

typedef struct AnimSelectionFrame {
    unsigned short frame;
    union {
        struct {
            unsigned char field_02;
            unsigned char animation_index;
        };
        unsigned short selection;
    };
} AnimSelectionFrame;

AnimPdata* anim_pdata;
MkObj* anim_obj;
MkFlippedBoneMap* flipped_bones;
static float flip_factor;
static Quat qy180 = {0.0f, 1.0f, 0.0f, 0.0f};
static const char morph_not_found_format[64] =
    "sobj ID: %d was not found. Error in obj_start_morph mk_anim.c";
static int _flipped_human_bones[] = {
    0, 2, 1, 3, 5, 4, 6, 8, 7, 9, 11, 10, 14, 13, 12, 17, 16, 15,
    19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30, 33, 32,
    35, 34, 37, 36, 39, 38, 41, 40, 43, 42, 45, 44, 47, 46, 48, 50,
    49, 51, 52, 53, 55, 54, 57, 56
};
Vec ani_flip_angs = {0.0f, 0.0f, 3.1415927f};
MkFlippedBoneMap flipped_human_bones = {
    sizeof(_flipped_human_bones) / sizeof(_flipped_human_bones[0]),
    _flipped_human_bones
};
extern float game_speed;
extern int exec_tick_ctr;
extern MkVtable5 vtbl_mkpdata_anim;
extern unsigned char shared_ani[];

float mka_next_fno;
float mka_prev_fno;
unsigned short* mka_next_fp;
unsigned short* mka_prev_fp;
int mka_bytes_per_frame;
float mka_sought_fno;
AnimMergedChannelHeader* mka_merge_channel_hdr;
AnimChannelHeader* mka_channel_hdr;
unsigned short* mka_hdr;

void* memcpy(void* dst, const void* src, unsigned int size);
void get_bone_world_pos(MkObj* obj, int bone, Vec* out);
void get_bone_offset_world_pos(
    MkObj* obj, int bone, const Vec* offset, Vec* out);
void update_bone_hierarchy(MkHdr* obj);
int pose_anim(AnimPdata* anim, int update_object);
int set_anim_script_frame(
    float frame, AnimPdata* anim, AnimScript* script, unsigned int flags);
AnimPdata* get_mkpdata_anim(void);
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int transition_to_anim_script_frame(
    float transition_frames,
    float frame,
    AnimPdata* anim,
    AnimScript* script,
    unsigned int flags);
static void apply_tag_frame(AnimPdata* anim, MkObj* obj);
static void apply_anim_offset(
    float weight,
    AnimPdata* anim,
    MkObj* obj,
    Vec* offset,
    int transition_pass,
    int update_object);
static unsigned short* find_frame(unsigned short* current);
static int _set_old_frameno(AnimPdata* anim);
static int _set_frameno(AnimPdata* anim);
static void process_obj_bones(MkObj* obj, const int* tags);
static RpAtomic* atomic_set_HAnimHierarchy(
    RpAtomic* atomic, void* hierarchy);
static RwFrame* get_child_frame_hierarchy(
    RwFrame* frame, void* out_data);

/*
 * Animation scripts are retail packed blobs with offsets relative to their
 * own base. Keep the byte walk isolated here instead of open-coding casts at
 * each consumer.
 */
static inline void* anim_script_data(const void* script, unsigned int offset) {
    return (unsigned char*)script + offset;
}


static inline int anim_create_proc_flags(void) {
    int flags;

    flags = 0;
    ((MkProcCreateFlagBits*)&flags)->animation_pdata = 1;
    return flags;
}




















static void _bone_make_parents_my_children(MkBone* bone);


static inline float anim_last_frame(const AnimScript* script) {
    return (float)script->frame_count - 1.0f;
}

static inline void select_flip_map(AnimPdata* anim, unsigned int flags) {
    MkObj* obj = anim->obj;
    int flipped;

    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    flipped = obj != 0 && obj->hide_flag_bits.bit6 != 0;

    if ((flags & 8) != 0) {
        flipped = 1 - flipped;
    }
    if (flipped == 0) {
        flipped_bones = 0;
        flip_factor = 1.0f;
    } else {
        flipped_bones = obj->flipped_bone_map;
        flip_factor = -1.0f;
    }
}

static inline void add_script_loop_offset(
    AnimScript* script, Vec* offset, float* angle) {
    const float scale = 0.000030518044f;
    float scaled_x;

    if (script == 0) {
        return;
    }
    scaled_x = (float)script->loop_offset_x * scale;
    offset->x = flip_factor * scaled_x + offset->x;
    offset->y = offset->y + (float)script->loop_offset_y * scale;
    offset->z = offset->z + (float)script->loop_offset_z * scale;
    *angle = norm_angle(*angle);
}

static inline void subtract_script_loop_offset(
    AnimScript* script, Vec* offset, float* angle) {
    const float scale = 0.000030518044f;
    float scaled_x;

    if (script == 0) {
        return;
    }
    scaled_x = (float)script->loop_offset_x * scale;
    offset->x = -(flip_factor * scaled_x - offset->x);
    offset->y = offset->y - (float)script->loop_offset_y * scale;
    offset->z = offset->z - (float)script->loop_offset_z * scale;
    *angle = norm_angle(*angle);
}

static inline void rebuild_anim_track_table(AnimPdata* anim) {
    AnimScript* script = anim->script;
    int track_count = script->track_count;
    AnimChannelHeader* track;
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
    while (track_count > 0) {
        *track_data = anim_script_data(script, track->data_offset);
        track_data++;
        track++;
        track_count--;
    }
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset
    anim->tag_frame =
        (AnimTagFrame*)anim_script_data(script, track->data_offset);
    anim->tag_frame = (AnimTagFrame*)anim_script_data(
        script, script->tag_data_offset);
}

static inline MkObj* live_anim_object(const PlyrMirrorObjLatch* latch) {
    MkObj* obj = latch->obj;

    if (obj != 0) {
        if (obj->hdr.instance != latch->instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    return obj;
}

static inline MkProc* live_anim_proc(const PlyrProcLatch* latch) {
    MkProc* proc = latch->proc;

    if (proc != 0) {
        if (proc->instance != latch->instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    return proc;
}

static inline BoneMatcherState* live_anim_pose_state(
    const MkHdrLatch* latch) {
    BoneMatcherState* state = (BoneMatcherState*)latch->hdr;

    if (state != 0) {
        if (state->hdr.instance != latch->instance) {
            state = 0;
        }
    } else {
        state = 0;
    }
    return state;
}

static inline int anim_selection_is_none(
    const AnimSelectionFrame* selection) {
    return selection->animation_index == 0xFF &&
        selection->field_02 == 0xFF;
}

static inline int anim_selection_script(
    AnimPdata* hand,
    const AnimSelectionFrame* selection,
    AnimScript** scripts,
    int script_count,
    AnimScript** result) {
    if (anim_selection_is_none(selection)) {
        *result = hand->script;
        return 1;
    }
    if (selection->animation_index < script_count) {
        *result = scripts[selection->animation_index];
        return 1;
    }
    return 0;
}

static RpAtomic* ScanForBone_callback(
    RpAtomic* atomic, void* data);

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

int pose_morph(MkHdr* hdr) {
    MorphState* morph;
    MorphFrameHeader* frame;
    float position;
    float fraction;
    float span;
    float previous_position;
    int target;
    int next_target;
    int result;

    morph = (MorphState*)hdr;
    result = set_morph_frameno(morph);
    mka_bytes_per_frame = 8;
    mka_sought_fno = morph->frame;
    morph->current_frame =
        morph_find_frame(morph, morph->current_frame);

    if (mka_next_fno == mka_sought_fno) {
        frame = (MorphFrameHeader*)mka_next_fp;
        target = frame->target;
        next_target = frame->next_target;
        position = frame->position;
    } else if (mka_prev_fno == mka_sought_fno) {
        frame = (MorphFrameHeader*)mka_prev_fp;
        target = frame->target;
        next_target = frame->next_target;
        position = frame->position;
    } else {
        MorphFrameHeader* next_frame =
            (MorphFrameHeader*)mka_next_fp;
        MorphFrameHeader* previous_frame =
            (MorphFrameHeader*)mka_prev_fp;

        span = mka_next_fno - mka_prev_fno;
        if (span != 0.0f) {
            fraction = (mka_next_fno - mka_sought_fno) / span;
        } else {
            fraction = 0.0f;
        }
        target = next_frame->target;
        next_target = next_frame->next_target;
        previous_position = previous_frame->position;
        if (previous_frame->next_target != next_target) {
            previous_position = 0.0f;
        }
        position =
            previous_position * fraction +
            next_frame->position * (1.0f - fraction);
    }

    morph->interpolator->position = position;
    morph->interpolator->flags |= 3;
    if ((morph->interpolator->flags & 4) == 0 &&
        morph->atomic->object.parent != 0) {
        RwFrameUpdateObjects((RwFrame*)morph->atomic->object.parent);
    }
    if (morph->interpolator->startMorphTarget != target) {
        morph->interpolator->startMorphTarget = target;
        morph->interpolator->flags |= 3;
        if ((morph->interpolator->flags & 4) == 0 &&
            morph->atomic->object.parent != 0) {
            RwFrameUpdateObjects((RwFrame*)morph->atomic->object.parent);
        }
    }
    if (morph->interpolator->endMorphTarget != next_target) {
        morph->interpolator->endMorphTarget = next_target;
        morph->interpolator->flags |= 3;
        if ((morph->interpolator->flags & 4) == 0 &&
            morph->atomic->object.parent != 0) {
            RwFrameUpdateObjects((RwFrame*)morph->atomic->object.parent);
        }
    }
    return result;
}

static int set_morph_frameno(MorphState* morph) {
    float frame = morph->frame;
    float low = morph->low_frame;

    if (frame < low) {
        switch (morph->flags & 7) {
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
        case 4:
            break;
        }
        if (morph->frame_callback != 0) {
            morph->frame_callback(&morph->frame);
        }
        return 0;
    }

    switch (morph->flags & 7) {
    case 0:
    case 1: {
        float end = 1.0f + morph->high_frame;
        if (frame >= end) {
            morph->frame = frame - (end - low);
            if (morph->frame_callback != 0) {
                morph->frame_callback(&morph->frame);
            }
            return 0;
        }
        return 1;
    }
    case 2:
        if (frame > morph->high_frame) {
            morph->frame = 2.0f * morph->high_frame - frame;
            if (morph->frame_step > 0.0f) {
                morph->frame_step = -morph->frame_step;
            }
            if (morph->frame_callback != 0) {
                morph->frame_callback(&morph->frame);
            }
            return 0;
        }
        return 1;
    case 3:
        if (frame > morph->high_frame) {
            morph->frame = morph->high_frame;
            morph->frame_step = 0.0f;
            if (morph->frame_callback != 0) {
                morph->frame_callback(&morph->frame);
            }
            return 0;
        }
        return 1;
    case 4:
        if (morph->frame_callback != 0) {
            morph->frame_callback(&morph->frame);
        }
        return 0;
    default:
        return 1;
    }
}

static unsigned short* morph_find_frame(
    MorphState* morph, unsigned short* current) {
    float current_frame;
    float delta;

    current_frame = (float)*current;
    delta = mka_sought_fno - current_frame;
    do {
        if (delta > 0.0f) {
            if (delta < 5.0) {
                mka_next_fp = current;
                mka_next_fno = current_frame;
            } else {
                unsigned short* last = (unsigned short*)(
                    (unsigned char*)morph->frame_table +
                    morph->frame_count * 8);
                float last_frame = (float)*last;

                if (mka_sought_fno <
                    0.5f * (current_frame + last_frame)) {
                    mka_next_fp = current;
                    mka_next_fno = current_frame;
                } else {
                    mka_prev_fp = last;
                    mka_prev_fno = last_frame;
                    break;
                }
            }
        } else if (delta < 0.0f) {
            if (mka_sought_fno < 0.5f * current_frame) {
                mka_next_fp = morph->frame_table;
                mka_next_fno = 0.0f;
            } else {
                mka_prev_fp = current;
                mka_prev_fno = current_frame;
                break;
            }
        } else {
            mka_prev_fp = current;
            mka_next_fp = current;
            mka_prev_fno = mka_sought_fno;
            mka_next_fno = mka_sought_fno;
            return current;
        }
        while (mka_next_fno < mka_sought_fno) {
            mka_prev_fno = mka_next_fno;
            mka_prev_fp = mka_next_fp;
            mka_next_fp = (unsigned short*)(
                (unsigned char*)mka_next_fp + mka_bytes_per_frame);
            mka_next_fno = (float)*mka_next_fp;
        }
        return mka_next_fp;
    } while (0);

    while (mka_prev_fno > mka_sought_fno) {
        mka_next_fno = mka_prev_fno;
        mka_next_fp = mka_prev_fp;
        mka_prev_fp = (unsigned short*)(
            (unsigned char*)mka_prev_fp - mka_bytes_per_frame);
        mka_prev_fno = (float)*mka_prev_fp;
    }
    return mka_prev_fp;
}

MorphState* obj_start_morph(
    MkObj* obj, int sobj_id, MorphScript* script, unsigned int flags) {
    MorphState* morph = (MorphState*)get_mkpdata_generic(sizeof(MorphState));
    MkSobj* sobj = 0;

    if (morph != 0) {
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
                    morph_not_found_format,
                    sobj_id);
                return 0;
            }
            morph->atomic = sobj->atomic;
        }
        if (morph->atomic == 0) {
            morph->atomic = obj_get_1st_atomic(obj);
        }
        if (morph->atomic != 0) {
            morph->interpolator = &morph->atomic->interpolator;
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
        }
    }
    return morph;
}

void start_morph_proc(void) {
    int flags[2];

    flags[1] = 0;
    flags[0] = 0;
    morph_proc = get_mkproc_nostack(flags);
    morph_proc = create_mkproc(
        0x12, morph_proc, 0x500B, p_morph, 0);
}

void bm_force_fake_child_bid(BoneMatcherState* matcher, int bone_id) {
    matcher->fake_child_bid = bone_id;
}

BoneMatcherState* start_bone_matcher(
    float blend_ticks,
    MkObj* parent_obj,
    int parent_bid,
    MkObj* child_obj,
    int child_bid) {
    BoneMatcherState* matcher = 0;
    MkBone* parent_bone;
    MkBone* child_bone;
    RwMatrix flip_matrix __attribute__((aligned(16)));

    _create_mkproc_generic_nostack(
        0x500F,
        0x15,
        p_bone_matcher,
        sizeof(BoneMatcherState),
        (MkHdr**)&matcher);
    if (matcher == 0) {
        return 0;
    }

    matcher->flags_word_08 = 0;
    matcher->child_weight = 0.0f;
    matcher->parent_obj = parent_obj;
    matcher->parent_instance = parent_obj->hdr.instance;
    matcher->parent_bid = parent_bid;
    zero_v3(&matcher->parent_offset);

    parent_bone = parent_obj->bones[parent_bid];
    if (parent_bone != 0 && parent_bone->parent_matrix != 0) {
        child_bone = child_obj->bones[child_bid];
        if (child_bone != 0 && child_bone->parent_matrix != 0) {
            parent_bone->flags_54_bits.calculation_locked = 1;
            matcher->child_obj = child_obj;
            matcher->child_instance = child_obj->hdr.instance;
            matcher->fake_child_bid = child_bid;
            zero_v3(&matcher->child_offset);
            matcher->clone_obj = 0;
            matcher->clone_instance = 0;
            matcher->parent_rotation = child_bone->rotation;
            matcher->mirrored_parent_rotation = matcher->parent_rotation;
            matcher->mirrored_parent_rotation.y *= -1.0f;
            matcher->mirrored_parent_rotation.z *= -1.0f;
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
    }

    if (matcher->hdr.instance != 0) {
        matcher->hdr.typed_vtbl->destroy(&matcher->hdr);
    }
    return 0;
}

static inline void compose_bone_rotation(
    Quat* out, const Quat* parent, const Quat* target) {
    float cross_x =
        parent->y * target->z - parent->z * target->y;
    float cross_y =
        parent->z * target->x - parent->x * target->z;
    float cross_z =
        parent->x * target->y - parent->y * target->x;

    out->x = cross_x;
    out->y = cross_y;
    out->z = cross_z;
    out->x += target->x * parent->w;
    out->y += target->y * parent->w;
    out->z += target->z * parent->w;
    out->w = parent->w * target->w -
        (parent->z * target->z +
         (parent->x * target->x + parent->y * target->y));
    out->x += parent->x * target->w;
    out->y += parent->y * target->w;
    out->z += parent->z * target->w;
}

static float p_bone_matcher(void) {
    BoneMatcherState* matcher = (BoneMatcherState*)apdata;
    MkObj* parent_obj;
    MkObj* child_obj;
    MkSobj* clone_obj;
    MkFlippedBoneMap* bone_map;
    MkBone* child_bone;
    MkBone* parent_bone;
    RwMatrix* child_matrix;
    Vec parent_pos;
    Vec child_pos;
    Vec parent_offset;
    Vec child_offset;
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

    parent_obj = matcher->parent_obj;
    if (parent_obj != 0) {
        if (parent_obj->hdr.instance != matcher->parent_instance) {
            parent_obj = 0;
        }
    } else {
        parent_obj = 0;
    }
    if (parent_obj == 0) {
        mkproc_die();
    }

    child_obj = matcher->child_obj;
    if (child_obj != 0) {
        if (child_obj->hdr.instance != matcher->child_instance) {
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
        bone_map = child_obj->flipped_bone_map;
        if (bone_map != 0 && child_bid < bone_map->count) {
            child_bid = bone_map->bone_indices[child_bid];
        }
    }

    parent_offset = matcher->parent_offset;
    parent_bid = matcher->parent_bid;
    if (matcher->flags_09.bits.use_unmirrored_parent == 0 &&
        parent_obj->hide_flag_bits.bit6 != 0) {
        parent_offset.x *= -1.0f;
        bone_map = parent_obj->flipped_bone_map;
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
        clone_obj = matcher->clone_obj;
        if (clone_obj != 0) {
            if (clone_obj->hdr.instance != matcher->clone_instance) {
                clone_obj = 0;
            }
        } else {
            clone_obj = 0;
        }
        if (clone_obj == 0) {
            mkproc_die();
        }
        memcpy(&clone_obj->frame->modelling, parent_bone, 0x30);
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
        if (parent_bone->flags_55_bits.collision_deferred != 0) {
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

        compose_bone_rotation(
            &composed_rotation,
            &parent_bone->rotation_90,
            target_rotation);

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

        compose_bone_rotation(
            &composed_rotation,
            &parent_bone->rotation_90,
            target_rotation);

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

    if (child_obj->flags_08_bits.scale_active != 0) {
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

void bone_matcher_child_set_offset(
    BoneMatcherState* matcher, const Vec* offset) {
    matcher->child_offset.x = offset->x;
    matcher->child_offset.y = offset->y;
    matcher->child_offset.z = offset->z;
}

void bone_matcher_parent_set_offset(
    BoneMatcherState* matcher, const Vec* offset) {
    matcher->parent_offset.x = offset->x;
    matcher->parent_offset.y = offset->y;
    matcher->parent_offset.z = offset->z;
}

float p_anim_idle(void) {
    return 1000.0f;
}

static float p_anim_reset_weight_idle(void) {
    anim_pdata->hand_transition = 1.0f;
    anim_pdata->hand_transition_step = 0.0f;
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(
        p_anim_idle, 0.0f);
    return 0.0f;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static float p_pose_handanim(void) {
    if (anim_pdata->hand_transition > 0.0f) {
        if (anim_pdata->hand_anim_script != 0 &&
            anim_pdata->next_hand_script != 0 &&
            anim_pdata->hand_transition_frames < 1.0f &&
            anim_pdata->old_script != anim_pdata->next_hand_script) {
            anim_pdata->script = anim_pdata->next_hand_script;
            anim_pdata->flags = anim_pdata->hand_flags | 0x80;
            anim_pdata->frame = 0.0f;
            anim_pdata->low_frame = 0.0f;
            anim_pdata->high_frame =
                (float)(anim_pdata->script->frame_count - 1);
            anim_pdata->step = 1.0f;
            anim_pdata->frame_callback = 0;
            rebuild_anim_track_table(anim_pdata);

            transition_to_anim_script_frame(
                anim_pdata->hand_transition_frames,
                0.0f,
                anim_pdata,
                anim_pdata->hand_anim_script,
                anim_pdata->hand_flags | 0x80);
        } else if (anim_pdata->hand_anim_script != 0 &&
                   anim_pdata->script != anim_pdata->hand_anim_script) {
            if (anim_pdata->hand_transition_frames < 1.0f) {
                transition_to_anim_script_frame(
                    anim_pdata->hand_transition_frames,
                    0.0f,
                    anim_pdata,
                    anim_pdata->hand_anim_script,
                    anim_pdata->hand_flags | 0x80);
            } else {
                set_anim_script_frame(
                    0.0f,
                    anim_pdata,
                    anim_pdata->hand_anim_script,
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

    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(p_anim_idle, 0.0f);
    return 0.0f;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

static inline int advance_anim_state(AnimPdata* anim) {
    MkObj* obj;
    float speed;
    float frame_step;
    float maximum_step;
    int maximum_step_int;

    obj = anim->obj;
    speed = game_speed;
    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0 && obj->flags_0B_bits.force_anim_speed) {
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
        frame_step = speed * (anim->old_step + anim->old_step_accel);
        if (frame_step > 0.0f) {
            if (frame_step > maximum_step) {
                frame_step = (float)maximum_step_int;
            }
        } else if (frame_step < -maximum_step) {
            frame_step = (float)-maximum_step_int;
        }
        anim->old_frame += frame_step;
    }

    anim->previous_frame = anim->frame;
    maximum_step = (float)anim->script->frame_count / 3.0f;
    maximum_step_int = (int)maximum_step;
    frame_step = speed * (anim->step + anim->step_accel);
    if (frame_step > 0.0f) {
        if (frame_step > maximum_step) {
            frame_step = (float)maximum_step_int;
        }
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

float p_animate(void) {
    if (anim_pdata->last_update_tick != (unsigned int)exec_tick_ctr) {
        advance_anim_state(anim_pdata);
        if (anim_pdata->hand_transition == 0.0f) {
            ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(
                p_anim_reset_weight_idle, 0.0f);
            return 0.0f;
        }
        pose_anim(anim_pdata, 1);
    }
    return 1.0f;
}

static void ps_anim(void) {
    anim_pdata = 0;
    anim_obj = 0;
    plyr_pdata = 0;
}

void pw_anim(void) {
    MkObj* obj;
    PlyrPdata* owner;

    anim_pdata = (AnimPdata*)apdata;
    obj = anim_pdata->obj;
    if (obj != 0) {
        if (obj->hdr.instance != anim_pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    anim_obj = obj;
    if (obj == 0) {
        mkproc_die();
    }
    owner = anim_pdata->owner;
    if (owner != 0) {
        if (owner->instance != anim_pdata->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    plyr_pdata = owner;
}

int advance_anim(AnimPdata* anim) {
    return advance_anim_state(anim);
}

int pose_anim(AnimPdata* anim, int update_object) {
    AnimScript* script;
    PlyrPdata* owner;
    PlyrMirrorSlots* channel_set;
    AnimScript** shared_scripts;
    MkObj* channel_objects[3];
    MkObj* obj;
    MkObj* active_channel_obj;
    float transition_weight;
    float pass_weight;
    float channel_weight;
    float remaining_channel_weight;
    float previous_weight = 0.0f;
    Vec previous_vec;
    Vec next_vec;
    Quat previous_quat;
    Quat next_quat;
    BoneMatcherState previous_pose __attribute__((aligned(16)));
    BoneMatcherState next_pose __attribute__((aligned(16)));
    unsigned int flags;
    int merged_flag;
    unsigned int partial_flag;
    unsigned int zero_root_flag;
    unsigned int preserve_root_flag;
    unsigned int flip_flag;
    unsigned int suppress_face_flag;
    unsigned int pin_flag;
    int track_index;
    int transition_pass;
    int result;
    const float quat_scale = 4.6566129e-10f;
    const float focal_scale = 0.006994113f;
    const float translation_scale_1 = 0.0009765625f;
    const float packed_quat_scale = 0.00048828125f;
    const float translation_scale_9 = 0.001953125f;

    result = 1;
    if (anim->last_update_tick != (unsigned int)exec_tick_ctr) {
        anim->last_update_tick = exec_tick_ctr;
        anim->updates_this_tick = 1;
    } else {
        anim->updates_this_tick++;
    }

    owner = anim->owner;
    if (owner != 0) {
        if (owner->instance != anim->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    obj = anim->obj;
    if (obj != 0 && obj->hdr.instance != anim->obj_instance) {
        obj = 0;
    }
    if (obj == 0) {
        return result;
    }
    if (owner != 0 && owner->mirror_slots != 0) {
        channel_set = owner->mirror_slots;
        channel_objects[0] =
            live_anim_object(&channel_set->weapon[0].primary);
        channel_objects[1] =
            live_anim_object(&channel_set->weapon[1].primary);
        channel_objects[2] =
            live_anim_object(&owner->held_opponent_latch);
    } else {
        channel_objects[0] = 0;
        channel_objects[1] = 0;
        channel_objects[2] = 0;
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

    shared_scripts = (AnimScript**)(shared_ani + 0x380);
    do {
        if (transition_pass != 0) {
            _set_old_frameno(anim);
            pass_weight = 1.0f - transition_weight;
            script = anim->old_script;
            mka_hdr = (unsigned short*)script;
            mka_sought_fno = anim->old_frame;
            track_index = anim->track_capacity;
            flags = anim->old_flags;
        } else {
            result = _set_frameno(anim);
            script = anim->script;
            if (script == 0) {
                return result;
            }
            mka_hdr = (unsigned short*)script;
            pass_weight = transition_weight;
            track_index = 0;
            mka_sought_fno = anim->frame;
            flags = anim->flags;
            if (script->tag_data_offset != 0 &&
                !(anim->step < 0.0f)) {
                AnimTagFrame* first =
                    (AnimTagFrame*)anim_script_data(
                        script, script->tag_data_offset);
                AnimTagFrame* last =
                    (AnimTagFrame*)anim_script_data(
                        script, script->tag_end_offset) - 1;
                int frame = (int)(anim->previous_frame + 0.5f);

                while (anim->tag_frame->frame >= frame &&
                       anim->tag_frame != first) {
                    anim->tag_frame--;
                }
                if (anim->tag_frame->frame < frame) {
                    while (anim->tag_frame->frame < frame &&
                           anim->tag_frame != last) {
                        anim->tag_frame++;
                    }
                } else {
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
            }
        }

        merged_flag = flags & 0x2000;
        channel_weight = anim->hand_transition * pass_weight;
        {
            unsigned int i;
            int active_group = 0;

            active_channel_obj = obj;
            mka_channel_hdr =
                (AnimChannelHeader*)script->tracks;
            mka_merge_channel_hdr =
                (AnimMergedChannelHeader*)script->tracks;
            if (merged_flag != 0) {
                int frame_index = (int)(mka_sought_fno + 0.5f);

                mka_sought_fno = (float)frame_index;
                if (mka_sought_fno >
                    (float)(script->frame_count - 1)) {
                    frame_index = 0;
                    mka_sought_fno = 0.0f;
                }
                previous_weight = 1.0f;
                mka_prev_fp = (unsigned short*)(
                    (unsigned char*)script->tracks +
                    script->track_count *
                        sizeof(AnimMergedChannelHeader));
                mka_prev_fno = mka_sought_fno;
                mka_prev_fp = (unsigned short*)(
                    (unsigned char*)mka_prev_fp +
                    frame_index * script->merged_frame_stride);
            }
            remaining_channel_weight = 1.0f - channel_weight;
            partial_flag = flags & 0x800;
            zero_root_flag = flags & 0x40;
            preserve_root_flag = flags & 0x20;
            flip_flag = flags & 8;
            suppress_face_flag = flags & 0x4000;
            pin_flag = flags & 0x400;
            for (i = 0; i < (unsigned int)script->track_count;
                 i++, track_index = merged_flag != 0
                     ? (mka_merge_channel_hdr++,
                        mka_prev_fp = (unsigned short*)(
                            (unsigned char*)mka_prev_fp +
                            mka_bytes_per_frame),
                        track_index)
                     : (mka_channel_hdr++, track_index + 1)) {
                int channel_type;
                unsigned int channel_target;
                int group = 0;
                int bone_index = 0;
                int unmirrored_bone_index;
                MkObj* channel_obj;
                MkBone* bone;
                unsigned short* sample;

                if (merged_flag != 0) {
                    channel_type = mka_merge_channel_hdr->type;
                    channel_target = mka_merge_channel_hdr->target;
                } else {
                    channel_type = mka_channel_hdr->type;
                    channel_target = mka_channel_hdr->target;
                }
                group = (channel_target >> 16) & 0xF;
                bone_index = channel_target & 0xFFFF;
                if (anim->bone_remap != 0) {
                    bone_index = anim->bone_remap[bone_index];
                }
                unmirrored_bone_index = bone_index;

                switch (channel_type) {
                case 0:
                    break;
                case 1:
                case 9:
                case 10:
                    mka_bytes_per_frame = 8;
                    break;
                case 2:
                    mka_bytes_per_frame = 12;
                    break;
                case 4:
                    mka_bytes_per_frame = 8;
                    break;
                case 3:
                case 11:
                    mka_bytes_per_frame = 20;
                    break;
                case 5:
                    mka_bytes_per_frame = 4;
                    break;
                case 6:
                    mka_bytes_per_frame = 4;
                    break;
                case 7:
                    mka_bytes_per_frame = 20;
                    break;
                case 8:
                    mka_bytes_per_frame = 4;
                    break;
                case 12:
                    mka_bytes_per_frame = 12;
                    break;
                }
                if (group == 0) {
                    channel_obj = obj;
                } else if ((partial_flag == 0 ||
                            !(pass_weight < 1.0f)) &&
                           group <= 3) {
                    channel_obj = channel_objects[group - 1];
                } else {
                    continue;
                }
                if (channel_obj == 0) {
                    continue;
                }
                active_channel_obj = channel_obj;
                if (unmirrored_bone_index >= channel_obj->bone_count) {
                    continue;
                }
                if (group != active_group) {
                    int should_flip =
                        channel_obj != 0 &&
                        channel_obj->hide_flag_bits.bit6 != 0;

                    active_group = group;
                    if (flip_flag != 0) {
                        should_flip = 1 - should_flip;
                    }
                    if (should_flip) {
                        flipped_bones =
                            channel_obj->flipped_bone_map;
                        flip_factor = -1.0f;
                    } else {
                        flipped_bones = 0;
                        flip_factor = 1.0f;
                    }
                }
                if (flipped_bones != 0 &&
                    bone_index < flipped_bones->count) {
                    bone_index =
                        flipped_bones->bone_indices[bone_index];
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

                if (channel_type == 2 ||
                    channel_type == 3 ||
                    channel_type == 4 ||
                    channel_type == 11) {
                    if (transition_pass != 0) {
                        if ((anim->flags & 0x100) != 0) {
                            bone->rotation = bone->rotation_e0;
                            bone->field_60 += channel_weight;
                            continue;
                        }
                    } else if (
                        bone->flags_55_bits.preserve_rotation != 0 &&
                        transition_weight < 1.0f &&
                        bone->field_60 < 0.0001f) {
                        bone->rotation = bone->rotation_e0;
                        bone->field_60 = remaining_channel_weight;
                    }
                }

                if (merged_flag == 0) {
                    unsigned short* current =
                        (unsigned short*)
                            anim->track_data[track_index];

                    current = find_frame(current);
                    anim->track_data[track_index] = current;
                }
                sample = mka_prev_fp;
                if (mka_prev_fno != mka_sought_fno) {
                    if (mka_next_fno == mka_sought_fno) {
                        sample = mka_next_fp;
                    } else {
                        if (merged_flag == 0) {
                            float span = mka_next_fno - mka_prev_fno;

                            if (span != 0.0f) {
                                previous_weight =
                                    (mka_next_fno - mka_sought_fno) / span;
                            } else {
                                previous_weight = 0.0f;
                            }
                        }

                        {
                            int quaternion_channel = 0;

                            switch (channel_type) {
                    case 1: {
                        float contribution = channel_weight;

                        previous_vec.x = flip_factor *
                            (translation_scale_1 *
                             (float)((AnimVecFrame*)mka_prev_fp)->x);
                        previous_vec.y = translation_scale_1 *
                            (float)((AnimVecFrame*)mka_prev_fp)->y;
                        previous_vec.z = translation_scale_1 *
                            (float)((AnimVecFrame*)mka_prev_fp)->z;
                        next_vec.x = flip_factor *
                            (translation_scale_1 *
                             (float)((AnimVecFrame*)mka_next_fp)->x);
                        next_vec.y = translation_scale_1 *
                            (float)((AnimVecFrame*)mka_next_fp)->y;
                        next_vec.z = translation_scale_1 *
                            (float)((AnimVecFrame*)mka_next_fp)->z;
                        if (bone->field_64 == 0.0f) {
                            interp_v3(
                                &bone->translation.value,
                                &previous_vec,
                                &next_vec,
                                previous_weight);
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &bone->translation.value,
                                    transition_pass, update_object);
                            }
                            bone->field_64 = contribution;
                        } else {
                            float combined_weight;

                            interp_v3(
                                &previous_vec,
                                &previous_vec,
                                &next_vec,
                                previous_weight);
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &previous_vec,
                                    transition_pass, update_object);
                            }
                            combined_weight =
                                bone->field_64 + contribution;
                            interp_v3(
                                &bone->translation.value,
                                &bone->translation.value,
                                &previous_vec,
                                bone->field_64 / combined_weight);
                            bone->field_64 = combined_weight;
                        }
                        continue;
                    }
                    case 9:
                    case 10: {
                        float contribution = channel_weight;

                        previous_vec.x = flip_factor *
                            (translation_scale_9 *
                             (float)((AnimVecFrame*)mka_prev_fp)->x);
                        previous_vec.y = translation_scale_9 *
                            (float)((AnimVecFrame*)mka_prev_fp)->y;
                        previous_vec.z = translation_scale_9 *
                            (float)((AnimVecFrame*)mka_prev_fp)->z;
                        next_vec.x = flip_factor *
                            (translation_scale_9 *
                             (float)((AnimVecFrame*)mka_next_fp)->x);
                        next_vec.y = translation_scale_9 *
                            (float)((AnimVecFrame*)mka_next_fp)->y;
                        next_vec.z = translation_scale_9 *
                            (float)((AnimVecFrame*)mka_next_fp)->z;
                        if (bone->field_64 == 0.0f) {
                            interp_v3(
                                &bone->translation.value,
                                &previous_vec,
                                &next_vec,
                                previous_weight);
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &bone->translation.value,
                                    transition_pass, update_object);
                            }
                            bone->field_64 = contribution;
                        } else {
                            float combined_weight;

                            interp_v3(
                                &previous_vec,
                                &previous_vec,
                                &next_vec,
                                previous_weight);
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &previous_vec,
                                    transition_pass, update_object);
                            }
                            combined_weight =
                                bone->field_64 + contribution;
                            interp_v3(
                                &bone->translation.value,
                                &bone->translation.value,
                                &previous_vec,
                                bone->field_64 / combined_weight);
                            bone->field_64 = combined_weight;
                        }
                        continue;
                    }
                    case 3:
                    case 11: {
                        AnimQuatFrame* previous =
                            (AnimQuatFrame*)mka_prev_fp;
                        AnimQuatFrame* next =
                            (AnimQuatFrame*)mka_next_fp;
                        previous_quat.x = quat_scale * (float)previous->x;
                        previous_quat.y = flip_factor *
                            (quat_scale * (float)previous->y);
                        previous_quat.z = flip_factor *
                            (quat_scale * (float)previous->z);
                        previous_quat.w = quat_scale * (float)previous->w;
                        next_quat.x = quat_scale * (float)next->x;
                        next_quat.y = flip_factor *
                            (quat_scale * (float)next->y);
                        next_quat.z = flip_factor *
                            (quat_scale * (float)next->z);
                        next_quat.w = quat_scale * (float)next->w;
                        quaternion_channel = 1;
                    }
                    case 4: {
                        if (!quaternion_channel) {
                            AnimPackedQuatFrame* previous =
                                (AnimPackedQuatFrame*)mka_prev_fp;
                            AnimPackedQuatFrame* next =
                                (AnimPackedQuatFrame*)mka_next_fp;
                            int previous_x =
                                (short)previous->packed_xy.bits.x;
                            int previous_y =
                                ((int)previous->packed_yzw >> 24) * 16 +
                                previous->packed_xy.bits.y_low;
                            int previous_z =
                                (int)(previous->packed_yzw << 8) >> 20;
                            int previous_w =
                                (int)(previous->packed_yzw << 20) >> 20;
                            int next_x = (short)next->packed_xy.bits.x;
                            int next_y =
                                ((int)next->packed_yzw >> 24) * 16 +
                                next->packed_xy.bits.y_low;
                            int next_z =
                                (int)(next->packed_yzw << 8) >> 20;
                            int next_w =
                                (int)(next->packed_yzw << 20) >> 20;

                            previous_quat.x =
                                packed_quat_scale * (float)previous_x;
                            previous_quat.y = flip_factor *
                                (packed_quat_scale * (float)previous_y);
                            previous_quat.z = flip_factor *
                                (packed_quat_scale * (float)previous_z);
                            previous_quat.w =
                                packed_quat_scale * (float)previous_w;
                            next_quat.x = packed_quat_scale * (float)next_x;
                            next_quat.y = flip_factor *
                                (packed_quat_scale * (float)next_y);
                            next_quat.z = flip_factor *
                                (packed_quat_scale * (float)next_z);
                            next_quat.w = packed_quat_scale * (float)next_w;
                        }
                        if (bone->field_60 == 0.0f) {
                            gxQuatInterpQuat(
                                &bone->rotation,
                                &previous_quat,
                                &next_quat,
                                previous_weight);
                            bone->field_60 = channel_weight;
                        } else {
                            float combined_weight =
                                bone->field_60 + channel_weight;

                            gxQuatInterpQuat(
                                &previous_quat,
                                &previous_quat,
                                &next_quat,
                                previous_weight);
                            gxQuatInterpQuat(
                                &bone->rotation,
                                &bone->rotation,
                                &previous_quat,
                                bone->field_60 / combined_weight);
                            bone->field_60 = combined_weight;
                        }
                        continue;
                    }
                    case 5: {
                        AnimSelectionFrame* previous =
                            (AnimSelectionFrame*)mka_prev_fp;
                        AnimSelectionFrame* next =
                            (AnimSelectionFrame*)mka_next_fp;

                        if (previous->animation_index ==
                                next->animation_index ||
                            anim_selection_is_none(next)) {
                            sample = mka_prev_fp;
                            break;
                        }
                        if (owner != 0) {
                            MkProc* selected_proc = 0;
                            AnimScript** scripts = shared_scripts;
                            int script_count = 0x40;
                            unsigned int hand_flags = 0;
                            AnimPdata* hand;

                            switch (unmirrored_bone_index) {
                            case 0x10:
                                if (suppress_face_flag != 0) {
                                    continue;
                                }
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_8C);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->face_anim_latch);
                                }
                                scripts = owner->face_animations;
                                script_count = 0x1A;
                                break;
                            case 0x18:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_7C);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->left_hand_anim_latch);
                                }
                                break;
                            case 0x19:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_84);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->right_hand_anim_latch);
                                }
                                hand_flags = 8;
                                break;
                            case 0x48:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[0]);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[1]);
                                }
                                break;
                            case 0x55:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[2]);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[3]);
                                }
                                hand_flags = 8;
                                break;
                            default:
                                continue;
                            }
                            if (selected_proc == 0 &&
                                unmirrored_bone_index == 0x10) {
                                continue;
                            }
                            hand = (AnimPdata*)pdata_of_proc(selected_proc);
                            hand->hand_transition_frames =
                                1.0f - previous_weight;
                            hand->hand_transition = pass_weight;
                            anim_selection_script(
                                hand, previous,
                                scripts, script_count,
                                &hand->next_hand_script);
                            anim_selection_script(
                                hand, next,
                                scripts, script_count,
                                &hand->hand_anim_script);
                            hand->hand_flags = hand_flags;
                            xfer_proc(selected_proc, p_pose_handanim);
                        }
                        continue;
                    }
                    case 6: {
                        sample = mka_prev_fp;
                        break;
                    }
                    case 7: {
                        BoneMatcherState* pose = 0;

                        if (owner != 0) {
                            if (group == 1) {
                                pose = live_anim_pose_state(
                                    &channel_set->weapon[0].secondary_hdr);
                            } else if (group == 2) {
                                pose = live_anim_pose_state(
                                    &channel_set->weapon[1].secondary_hdr);
                            } else if (group == 3) {
                                pose = live_anim_pose_state(
                                    &owner->hold_hdr_latch);
                            }
                        }
                        if (pose != 0) {
                            if (group == 3 && pass_weight < 1.0f) {
                                pose->flags_08.bits.inactive = 1;
                            } else {
                                AnimPoseFrame* previous =
                                    (AnimPoseFrame*)mka_prev_fp;
                                AnimPoseFrame* next =
                                    (AnimPoseFrame*)mka_next_fp;
                                BoneMatcherState* selected_pose =
                                    &previous_pose;

                                pose->flags_08.bits.inactive = 0;
                                previous_pose.fake_child_bid =
                                    previous->bone_and_flags & 0xFFF;
                                previous_pose.child_offset.x =
                                    translation_scale_1 *
                                    (float)previous->position_x;
                                previous_pose.child_offset.y =
                                    translation_scale_1 *
                                    (float)previous->position_y;
                                previous_pose.child_offset.z =
                                    translation_scale_1 *
                                    (float)previous->position_z;
                                previous_pose.parent_bid = previous->pose_id;
                                previous_pose.parent_offset.x =
                                    translation_scale_1 *
                                    (float)previous->offset_x;
                                previous_pose.parent_offset.y =
                                    translation_scale_1 *
                                    (float)previous->offset_y;
                                previous_pose.parent_offset.z =
                                    translation_scale_1 *
                                    (float)previous->offset_z;
                                next_pose.fake_child_bid =
                                    next->bone_and_flags & 0xFFF;
                                next_pose.child_offset.x =
                                    translation_scale_1 *
                                    (float)next->position_x;
                                next_pose.child_offset.y =
                                    translation_scale_1 *
                                    (float)next->position_y;
                                next_pose.child_offset.z =
                                    translation_scale_1 *
                                    (float)next->position_z;
                                next_pose.parent_bid = next->pose_id;
                                next_pose.parent_offset.x =
                                    translation_scale_1 *
                                    (float)next->offset_x;
                                next_pose.parent_offset.y =
                                    translation_scale_1 *
                                    (float)next->offset_y;
                                next_pose.parent_offset.z =
                                    translation_scale_1 *
                                    (float)next->offset_z;

                                if (previous_pose.fake_child_bid ==
                                        next_pose.fake_child_bid &&
                                    previous_pose.parent_bid ==
                                        next_pose.parent_bid) {
                                    interp_v3(
                                        &pose->child_offset,
                                        &previous_pose.child_offset,
                                        &next_pose.child_offset,
                                        previous_weight);
                                    interp_v3(
                                        &pose->parent_offset,
                                        &previous_pose.parent_offset,
                                        &next_pose.parent_offset,
                                        previous_weight);
                                } else {
                                    if (previous_weight < 0.5f) {
                                        selected_pose = &next_pose;
                                    }
                                    pose->child_offset =
                                        selected_pose->child_offset;
                                    pose->parent_offset =
                                        selected_pose->parent_offset;
                                }
                                pose->fake_child_bid =
                                    selected_pose->fake_child_bid;
                                pose->parent_bid = selected_pose->parent_bid;
                            }
                        }
                        continue;
                    }
                    case 8: {
                        AnimScalarFrame* previous =
                            (AnimScalarFrame*)mka_prev_fp;
                        AnimScalarFrame* next =
                            (AnimScalarFrame*)mka_next_fp;
                        set_camera_focal_length(
                            (focal_scale * (float)previous->value +
                             focal_scale * (float)next->value) *
                            0.5f);
                        continue;
                    }
                    case 12:
                        continue;
                            }
                        }
                    }
                }

                {
                    int exact_quaternion = 0;

                    switch (channel_type) {
                    case 1: {
                        float contribution = channel_weight;

                        if (bone->field_64 == 0.0f) {
                            bone->translation.value.x = flip_factor *
                                (translation_scale_1 *
                                 (float)((AnimVecFrame*)sample)->x);
                            bone->translation.value.y = translation_scale_1 *
                                (float)((AnimVecFrame*)sample)->y;
                            bone->translation.value.z = translation_scale_1 *
                                (float)((AnimVecFrame*)sample)->z;
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &bone->translation.value,
                                    transition_pass, update_object);
                            }
                            bone->field_64 = contribution;
                        } else {
                            float combined_weight;

                            previous_vec.x = flip_factor *
                                (translation_scale_1 *
                                 (float)((AnimVecFrame*)sample)->x);
                            previous_vec.y = translation_scale_1 *
                                (float)((AnimVecFrame*)sample)->y;
                            previous_vec.z = translation_scale_1 *
                                (float)((AnimVecFrame*)sample)->z;
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &previous_vec,
                                    transition_pass, update_object);
                            }
                            combined_weight =
                                bone->field_64 + contribution;
                            interp_v3(
                                &bone->translation.value,
                                &bone->translation.value,
                                &previous_vec,
                                bone->field_64 / combined_weight);
                            bone->field_64 = combined_weight;
                        }
                        continue;
                    }
                    case 9:
                    case 10: {
                        float contribution = channel_weight;

                        if (bone->field_64 == 0.0f) {
                            bone->translation.value.x = flip_factor *
                                (translation_scale_9 *
                                 (float)((AnimVecFrame*)sample)->x);
                            bone->translation.value.y = translation_scale_9 *
                                (float)((AnimVecFrame*)sample)->y;
                            bone->translation.value.z = translation_scale_9 *
                                (float)((AnimVecFrame*)sample)->z;
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &bone->translation.value,
                                    transition_pass, update_object);
                            }
                            bone->field_64 = contribution;
                        } else {
                            float combined_weight;

                            previous_vec.x = flip_factor *
                                (translation_scale_9 *
                                 (float)((AnimVecFrame*)sample)->x);
                            previous_vec.y = translation_scale_9 *
                                (float)((AnimVecFrame*)sample)->y;
                            previous_vec.z = translation_scale_9 *
                                (float)((AnimVecFrame*)sample)->z;
                            if (unmirrored_bone_index ==
                                    channel_obj->fallback_bone_index ||
                                bone_index ==
                                    channel_obj->fallback_bone_index) {
                                if (zero_root_flag != 0) {
                                    contribution = 0.0f;
                                } else if (preserve_root_flag == 0) {
                                    contribution = 1.0f;
                                }
                                apply_anim_offset(
                                    contribution, anim, channel_obj,
                                    &previous_vec,
                                    transition_pass, update_object);
                            }
                            combined_weight =
                                bone->field_64 + contribution;
                            interp_v3(
                                &bone->translation.value,
                                &bone->translation.value,
                                &previous_vec,
                                bone->field_64 / combined_weight);
                            bone->field_64 = combined_weight;
                        }
                        continue;
                    }
                    case 3:
                    case 11: {
                        AnimQuatFrame* frame = (AnimQuatFrame*)sample;
                        if (bone->field_60 == 0.0f) {
                            bone->rotation.x = quat_scale * (float)frame->x;
                            bone->rotation.y = flip_factor *
                                (quat_scale * (float)frame->y);
                            bone->rotation.z = flip_factor *
                                (quat_scale * (float)frame->z);
                            bone->rotation.w = quat_scale * (float)frame->w;
                            bone->field_60 = channel_weight;
                            continue;
                        }
                        previous_quat.x = quat_scale * (float)frame->x;
                        previous_quat.y = flip_factor *
                            (quat_scale * (float)frame->y);
                        previous_quat.z = flip_factor *
                            (quat_scale * (float)frame->z);
                        previous_quat.w = quat_scale * (float)frame->w;
                        exact_quaternion = 1;
                    }
                    case 4: {
                        if (!exact_quaternion) {
                            AnimPackedQuatFrame* frame =
                                (AnimPackedQuatFrame*)sample;
                            Quat* exact_quat = bone->field_60 == 0.0f
                                ? &bone->rotation
                                : &previous_quat;
                            int x = (short)frame->packed_xy.bits.x;
                            int y = ((int)frame->packed_yzw >> 24) * 16 +
                                frame->packed_xy.bits.y_low;
                            int z =
                                (int)(frame->packed_yzw << 8) >> 20;
                            int w =
                                (int)(frame->packed_yzw << 20) >> 20;

                            exact_quat->x = packed_quat_scale * (float)x;
                            exact_quat->y = flip_factor *
                                (packed_quat_scale * (float)y);
                            exact_quat->z = flip_factor *
                                (packed_quat_scale * (float)z);
                            exact_quat->w = packed_quat_scale * (float)w;
                            if (bone->field_60 == 0.0f) {
                                bone->field_60 = channel_weight;
                                continue;
                            }
                        }
                        {
                            float combined_weight = bone->field_60 +
                                channel_weight;

                            gxQuatInterpQuat(
                                &bone->rotation,
                                &bone->rotation,
                                &previous_quat,
                                bone->field_60 / combined_weight);
                            bone->field_60 = combined_weight;
                        }
                        continue;
                    }
                    case 5: {
                        AnimSelectionFrame* selected =
                            (AnimSelectionFrame*)sample;

                        if (anim_selection_is_none(selected)) {
                            continue;
                        }
                        if (owner != 0) {
                            MkProc* selected_proc = 0;
                            AnimScript** scripts = shared_scripts;
                            int script_count = 0x40;
                            unsigned int hand_flags = 0;
                            AnimPdata* hand;

                            switch (unmirrored_bone_index) {
                            case 0x10:
                                if (suppress_face_flag != 0) {
                                    continue;
                                }
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_8C);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->face_anim_latch);
                                }
                                scripts = owner->face_animations;
                                script_count = 0x1A;
                                break;
                            case 0x18:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_7C);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->left_hand_anim_latch);
                                }
                                break;
                            case 0x19:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->field_84);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->right_hand_anim_latch);
                                }
                                hand_flags = 8;
                                break;
                            case 0x48:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[0]);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[1]);
                                }
                                break;
                            case 0x55:
                                if (transition_pass != 0) {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[2]);
                                } else {
                                    selected_proc = live_anim_proc(
                                        &owner->goro_hand_anim[3]);
                                }
                                hand_flags = 8;
                                break;
                            default:
                                continue;
                            }
                            if (selected_proc == 0 &&
                                unmirrored_bone_index == 0x10) {
                                continue;
                            }
                            hand = (AnimPdata*)pdata_of_proc(selected_proc);
                            hand->hand_transition_frames = 1.0f;
                            hand->hand_transition = pass_weight;
                            anim_selection_script(
                                hand, selected,
                                scripts, script_count,
                                &hand->hand_anim_script);
                            hand->hand_flags = hand_flags;
                            xfer_proc(selected_proc, p_pose_handanim);
                        }
                        continue;
                    }
                    case 6: {
                        AnimScalarFrame* frame =
                            (AnimScalarFrame*)sample;

                        if (pin_flag != 0 &&
                            channel_weight >= 0.0f) {
                            channel_obj->ground_bone =
                                (unsigned short)frame->value;
                            if (channel_obj->ground_bone == 0xFFFF) {
                                channel_obj->hide_flag_bits.pin_animation = 0;
                            } else {
                                channel_obj->hide_flag_bits.pin_animation = 1;
                            }
                            if (channel_obj->hide_flag_bits.pin_animation) {
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
                        continue;
                    }
                    case 7: {
                        BoneMatcherState* pose = 0;

                        if (owner != 0) {
                            if (group == 1) {
                                pose = live_anim_pose_state(
                                    &owner->fighter_definition->mirror_slots.
                                        weapon[0].secondary_hdr);
                            } else if (group == 2) {
                                pose = live_anim_pose_state(
                                    &owner->fighter_definition->mirror_slots.
                                        weapon[1].secondary_hdr);
                            } else if (group == 3) {
                                pose = live_anim_pose_state(
                                    &owner->hold_hdr_latch);
                            }
                        }
                        if (pose != 0) {
                            if (group == 3 && pass_weight < 1.0f) {
                                pose->flags_08.bits.inactive = 1;
                            } else {
                                AnimPoseFrame* frame =
                                    (AnimPoseFrame*)sample;
                                pose->flags_08.bits.inactive = 0;
                                pose->fake_child_bid =
                                    frame->bone_and_flags & 0xFFF;
                                pose->child_offset.x =
                                    translation_scale_1 *
                                    (float)frame->position_x;
                                pose->child_offset.y =
                                    translation_scale_1 *
                                    (float)frame->position_y;
                                pose->child_offset.z =
                                    translation_scale_1 *
                                    (float)frame->position_z;
                                pose->parent_bid = frame->pose_id;
                                pose->parent_offset.x =
                                    translation_scale_1 *
                                    (float)frame->offset_x;
                                pose->parent_offset.y =
                                    translation_scale_1 *
                                    (float)frame->offset_y;
                                pose->parent_offset.z =
                                    translation_scale_1 *
                                    (float)frame->offset_z;
                            }
                        }
                        continue;
                    }
                    case 8: {
                        set_camera_focal_length(
                            focal_scale *
                            (float)((AnimScalarFrame*)sample)->value);
                        continue;
                    }
                    case 12: {
                        AnimMatrixFrame* frame =
                            (AnimMatrixFrame*)sample;
                        int rotation_x = (short)frame->packed_xy.bits.x;
                        int rotation_y =
                            (int)frame->packed_y_high * 16 +
                            frame->packed_xy.bits.y_low;
                        int rotation_z =
                            (int)(frame->packed_yzw << 8) >> 20;
                        int rotation_w = (short)frame->packed_zw.bits.w;

                        bone->parent_matrix->pos.x =
                            flip_factor *
                            (translation_scale_1 * (float)frame->x);
                        bone->parent_matrix->pos.y =
                            translation_scale_1 * (float)frame->y;
                        bone->parent_matrix->pos.z =
                            translation_scale_1 * (float)frame->z;
                        bone->rotation_90.x =
                            packed_quat_scale * (float)rotation_x;
                        bone->rotation_90.y = flip_factor *
                            (packed_quat_scale * (float)rotation_y);
                        bone->rotation_90.z = flip_factor *
                            (packed_quat_scale * (float)rotation_z);
                        bone->rotation_90.w =
                            packed_quat_scale * (float)rotation_w;
                        gxQuatQuatToMat(
                            RW_MATRIX_MAT33(bone->parent_matrix),
                            &bone->rotation_90);
                        bone->flags_54_bits.pose_matrix_applied = 1;
                        continue;
                    }
                    }
                }

            }
        }
        if (transition_pass == 0) {
            break;
        }
        transition_pass = 0;
        if ((flags & 0x1000) != 0) {
            MkBone* root =
                obj->bones[active_channel_obj->fallback_bone_index];

            quat_x_quat(
                &root->rotation,
                &qy180,
                &root->rotation);
        }
    } while (1);

    return result;
}

static void apply_tag_frame(AnimPdata* anim, MkObj* obj) {
    unsigned int bone_index;

    switch (anim->tag_frame->command) {
    case 0x500:
        if ((anim->flags & 0x10000) == 0) {
            if (obj->flags_0C_bits.tag_flag_40) {
                obj->flags_0C_bits.tag_flag_20 = 1;
            } else {
                obj->flags_09_bits.bit6 = 0;
            }
        }
        break;
    case 0x501:
        if ((anim->flags & 0x10000) == 0) {
            obj->flags_0C_bits.tag_flag_40 = 1;
            obj->flags_09_bits.bit6 = 1;
        }
        break;
    case 0x502:
        if ((anim->flags & 0x10000) == 0) {
            obj->flags_0C_bits.tag_flag_40 = 1;
            obj->flags_09_bits.bit6 = 1;
        }
        break;
    case 0x503:
        if (obj->flags_0C_bits.tag_flag_10 ||
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
        if ((unsigned int)obj->ground_bone != bone_index) {
            obj->ground_bone = bone_index;
        } else if (obj->hide_flag_bits.pin_animation) {
            break;
        }
        obj->hide_flag_bits.pin_animation = 1;
        obj->flags_0C_bits.tag_flag_10 = 1;
        get_bone_world_pos(obj, obj->ground_bone, &obj->ground_restore_pos);
        break;
    case 0x504:
        if ((anim->flags & 0x20000) == 0) {
            if (obj->flags_0C_bits.tag_flag_40) {
                obj->flags_0C_bits.tag_flag_08 = 1;
            } else {
                obj->hide_flag_bits.pin_animation = 0;
            }
        }
        break;
    }
}

static void apply_anim_offset(
    float frame_scale,
    AnimPdata* anim,
    MkObj* obj,
    Vec* offset,
    int old_anim,
    int apply_to_object) {
    Vec* previous;
    Vec delta;
    Vec world_delta;
    float root_weight;
    float object_scale;

    if (old_anim != 0) {
        previous = &anim->old_anim_offset;
        apply_to_object = 1;
    } else {
        previous = &anim->anim_offset;
    }
    delta.x = offset->x - previous->x;
    delta.y = offset->y - previous->y;
    delta.z = offset->z - previous->z;
    previous->x = offset->x;
    previous->y = offset->y;
    previous->z = offset->z;
    root_weight = anim->root_movement_weight;
    offset->x *= root_weight;
    offset->y *= root_weight;
    offset->z *= root_weight;

    if (apply_to_object != 0 && anim->obj_movement_weight != 0.0f) {
        v3_x_mat(&world_delta, &delta, obj->field_24);
        object_scale = frame_scale * anim->obj_movement_weight;
        world_delta.x *= object_scale;
        world_delta.y *= object_scale;
        world_delta.z *= object_scale;
        obj->pos.x += world_delta.x;
        obj->pos.y += world_delta.y;
        obj->pos.z += world_delta.z;
    }
}

void set_root_and_obj_movement_weights(
    float root_weight, float obj_weight, AnimPdata* anim) {
    MkObj* obj = anim->obj;
    RwMatrix* root_matrix;
    Vec world_delta;
    Vec local_delta;
    float weight_delta = root_weight - anim->root_movement_weight;

    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }

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

static unsigned short* find_frame(unsigned short* current) {
    float current_frame;
    float delta;

    current_frame = (float)*current;
    delta = mka_sought_fno - current_frame;

    do {
        if (delta > 0.0f) {
            if (delta < 5.0f) {
                mka_next_fp = current;
                mka_next_fno = current_frame;
            } else {
                unsigned short* last = (unsigned short*)(
                    (unsigned char*)mka_hdr +
                    mka_channel_hdr[1].data_offset -
                    mka_bytes_per_frame);
                float last_frame = (float)*last;

                if (mka_sought_fno <
                    0.5f * (current_frame + last_frame)) {
                    mka_next_fp = current;
                    mka_next_fno = current_frame;
                } else {
                    mka_prev_fp = last;
                    mka_prev_fno = last_frame;
                    break;
                }
            }
        } else if (delta < 0.0f) {
            if (mka_sought_fno < 0.5f * current_frame) {
                mka_next_fp = mka_hdr;
                mka_next_fno = 0.0f;
                mka_next_fp = (unsigned short*)(
                    (unsigned char*)mka_next_fp +
                    mka_channel_hdr->data_offset);
            } else {
                mka_prev_fp = current;
                mka_prev_fno = current_frame;
                break;
            }
        } else {
            mka_prev_fp = current;
            mka_next_fp = current;
            mka_prev_fno = mka_sought_fno;
            mka_next_fno = mka_sought_fno;
            return current;
        }
        mka_prev_fno = mka_next_fno;
        mka_prev_fp = mka_next_fp;
        while (mka_next_fno < mka_sought_fno) {
            mka_prev_fp = mka_next_fp;
            mka_prev_fno = mka_next_fno;
            mka_next_fp = (unsigned short*)(
                (unsigned char*)mka_next_fp + mka_bytes_per_frame);
            mka_next_fno = (float)*mka_next_fp;
        }
        return mka_next_fp;
    } while (0);

    mka_next_fno = mka_prev_fno;
    mka_next_fp = mka_prev_fp;
    while (mka_prev_fno > mka_sought_fno) {
        mka_next_fp = mka_prev_fp;
        mka_next_fno = mka_prev_fno;
        mka_prev_fp = (unsigned short*)(
            (unsigned char*)mka_prev_fp - mka_bytes_per_frame);
        mka_prev_fno = (float)*mka_prev_fp;
    }
    return mka_prev_fp;
}

static int _set_old_frameno(AnimPdata* anim) {
    float last_frame = anim_last_frame(anim->old_script);

    if (anim->old_low_frame < 0.0f) {
        anim->old_low_frame = 0.0f;
    }
    if (anim->old_low_frame > last_frame) {
        anim->old_low_frame = last_frame;
    }
    if (anim->old_high_frame < anim->old_low_frame) {
        anim->old_high_frame = anim->old_low_frame;
    }
    if (anim->old_high_frame > last_frame) {
        anim->old_high_frame = last_frame;
    }
    select_flip_map(anim, anim->old_flags);

    if (anim->old_frame < anim->old_low_frame) {
        switch (anim->old_flags & 7) {
        case 0:
        case 1:
            anim->old_frame +=
                anim->old_high_frame - anim->old_low_frame;
            add_script_loop_offset(
                anim->old_script,
                &anim->old_anim_offset,
                &anim->old_anim_angle);
            break;
        case 2:
            anim->old_frame =
                2.0f * anim->old_low_frame - anim->old_frame;
            if (anim->old_step < 0.0f) {
                anim->old_step = -anim->old_step;
                anim->old_step_accel = -anim->old_step_accel;
            }
            break;
        case 3:
            anim->old_frame = anim->old_low_frame;
            break;
        }
        if (anim->old_frame_callback != 0) {
            anim->old_frame_callback(anim, &anim->old_frame);
        }
    } else {
        switch (anim->old_flags & 7) {
        case 0:
        case 1: {
            float end = 1.0f + anim->old_high_frame;

            if (anim->old_frame >= end) {
                anim->old_frame -= end - anim->old_low_frame;
                subtract_script_loop_offset(
                    anim->old_script,
                    &anim->old_anim_offset,
                    &anim->old_anim_angle);
                if (anim->old_frame_callback != 0) {
                    anim->old_frame_callback(anim, &anim->old_frame);
                }
            }
            break;
        }
        case 2:
            if (anim->old_frame > anim->old_high_frame) {
                anim->old_frame =
                    2.0f * anim->old_high_frame - anim->old_frame;
                if (anim->old_step > 0.0f) {
                    anim->old_step = -anim->old_step;
                    anim->old_step_accel = -anim->old_step_accel;
                }
                if (anim->old_frame_callback != 0) {
                    anim->old_frame_callback(anim, &anim->old_frame);
                }
            }
            break;
        case 3:
            if (anim->old_frame > anim->old_high_frame) {
                anim->old_frame = anim->old_high_frame;
                if (anim->old_frame_callback != 0) {
                    anim->old_frame_callback(anim, &anim->old_frame);
                }
            }
            break;
        case 4:
            if (anim->old_frame_callback != 0) {
                anim->old_frame_callback(anim, &anim->old_frame);
            }
            break;
        }
    }
    if (anim->old_frame < 0.0f) {
        anim->old_frame = 0.0f;
    }
    if (anim->old_frame > last_frame + 1.0f) {
        anim->old_frame = last_frame + 1.0f;
    }
    return 1;
}

static int _set_frameno(AnimPdata* anim) {
    float last_frame = anim_last_frame(anim->script);
    int result = 1;

    if (anim->low_frame < 0.0f) {
        anim->low_frame = 0.0f;
    }
    if (anim->low_frame > last_frame) {
        anim->low_frame = last_frame;
    }
    if (anim->high_frame < anim->low_frame) {
        anim->high_frame = anim->low_frame;
    }
    if (anim->high_frame > last_frame) {
        anim->high_frame = last_frame;
    }
    select_flip_map(anim, anim->flags);

    if (anim->frame < anim->low_frame) {
        switch (anim->flags & 7) {
        case 0:
        case 1:
            anim->frame += anim->high_frame - anim->low_frame;
            add_script_loop_offset(
                anim->script,
                &anim->anim_offset,
                &anim->anim_angle);
            break;
        case 2:
            anim->frame = 2.0f * anim->low_frame - anim->frame;
            if (anim->step < 0.0f) {
                anim->step = -anim->step;
                anim->step_accel = -anim->step_accel;
            }
            break;
        case 3:
            anim->frame = anim->low_frame;
            break;
        }
        if (anim->frame_callback != 0) {
            anim->frame_callback(anim, &anim->frame);
        }
        result = 0;
    } else {
        switch (anim->flags & 7) {
        case 0:
        case 1: {
            float end = 1.0f + anim->high_frame;

            if (anim->frame >= end) {
                anim->frame -= end - anim->low_frame;
                subtract_script_loop_offset(
                    anim->script,
                    &anim->anim_offset,
                    &anim->anim_angle);
                if (anim->frame_callback != 0) {
                    anim->frame_callback(anim, &anim->frame);
                }
                result = 0;
            }
            break;
        }
        case 2:
            if (anim->frame > anim->high_frame) {
                anim->frame =
                    2.0f * anim->high_frame - anim->frame;
                if (anim->step > 0.0f) {
                    anim->step = -anim->step;
                    anim->step_accel = -anim->step_accel;
                }
                if (anim->frame_callback != 0) {
                    anim->frame_callback(anim, &anim->frame);
                }
                result = 0;
            }
            break;
        case 3:
            if (anim->frame > anim->high_frame) {
                anim->frame = anim->high_frame;
                if (anim->frame_callback != 0) {
                    anim->frame_callback(anim, &anim->frame);
                }
                result = 0;
            }
            break;
        case 4:
            if (anim->frame_callback != 0) {
                anim->frame_callback(anim, &anim->frame);
            }
            result = 0;
            break;
        }
    }
    if (anim->frame < 0.0f) {
        anim->frame = 0.0f;
    }
    if (anim->frame > last_frame + 1.0f) {
        anim->frame = last_frame + 1.0f;
    }
    return result;
}

int transition_to_anim_script_frame(
    float transition_frames,
    float frame,
    AnimPdata* anim,
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
    anim->frame = frame;
    anim->previous_frame = frame;
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

    if ((flags & 0x80) == 0) {
        obj = anim->obj;
        if (obj != 0) {
            if (obj->hdr.instance != anim->obj_instance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            if ((flags & 0x200) != 0) {
                obj->hide_flag_bits.bit0 = 1;
            }
            if ((flags & 0x100) != 0 ||
                obj->flags_0B_bits.root_transform_pending != 0 ||
                (anim->transition_weight < 1.0f &&
                 anim->transition_weight > 0.0f)) {
            if (obj->flags_0B_bits.root_transform_pending != 0) {
                MkBone* root = obj->bones[obj->fallback_bone_index];

                if (root != 0 && root->parent_matrix != 0) {
                    MKMATRIX yaw_matrix __attribute__((aligned(16)));
                    Vec adjusted_position;
                    Quat old_rotation;
                    Quat correction;
                    Quat* root_rotation = &root->rotation;
                    float root_yaw = quat_extract_ang_y(root_rotation);

                    obj->ang.y += root_yaw - obj->bone_angle_68;
                    y_angle_to_MKMATRIX(&yaw_matrix, root_yaw);
                    v3_x_mat_sub_v3(
                        &adjusted_position,
                        &root->translation.value,
                        &yaw_matrix,
                        &obj->pos);
                    scale_v3(
                        &obj->pos,
                        &adjusted_position,
                        -1.0f);
                    obj->bone_angle_68 = root_yaw;
                    y_angle_to_MKMATRIX(
                        &yaw_matrix,
                        obj->bone_angle_64 - root_yaw);
                    if (RtQuatConvertFromMatrix(
                            (RtQuat*)&correction,
                            &yaw_matrix)) {
                        gxQuatCopy(&old_rotation, root_rotation);
                        gxQuatMul(
                            root_rotation, &correction, &old_rotation);
                        gxQuatNorm(root_rotation);
                    }
                }
                obj->flags_0B_bits.root_transform_pending = 0;
            }
            for (i = 0; i < obj->bone_count; i++) {
                MkBone* bone = obj->bones[i];

                if (bone != 0) {
                    bone->rotation_e0.x = bone->rotation.x;
                    bone->rotation_e0.y = bone->rotation.y;
                    bone->rotation_e0.z = bone->rotation.z;
                    bone->rotation_e0.w = bone->rotation.w;
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
                            bone->rotation_e0.x = bone->rotation.x;
                            bone->rotation_e0.y = bone->rotation.y;
                            bone->rotation_e0.z = bone->rotation.z;
                            bone->rotation_e0.w = bone->rotation.w;
                        }
                    }
                    child_link = child_link->next;
                }
            }
            anim->flags |= 0x100;
            }
        }
    }

    anim->transition_weight = 0.0f;
    saved_step = anim->step;
    anim->step = 1.0f;

    speed = game_speed;
    obj = anim->obj;
    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0 && obj->flags_0B_bits.force_anim_speed) {
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
            if (frame_step > 0.0f) {
                if (frame_step > maximum_step) {
                    frame_step = (float)maximum_step_int;
                }
            } else if (frame_step < -maximum_step) {
                frame_step = (float)-maximum_step_int;
            }
            anim->old_frame += frame_step;
        }
        anim->previous_frame = anim->frame;
        maximum_step = (float)anim->script->frame_count / 3.0f;
        maximum_step_int = (int)maximum_step;
        frame_step = speed * (anim->step + anim->step_accel);
        if (frame_step > 0.0f) {
            if (frame_step > maximum_step) {
                frame_step = (float)maximum_step_int;
            }
        } else if (frame_step < -maximum_step) {
            frame_step = (float)-maximum_step_int;
        }
        anim->frame += frame_step;
    }
    anim->step = saved_step;
    anim->frame = frame;
    obj = anim->obj;
    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0 && obj->hide_flag_bits.pin_animation != 0) {
        float ground_restore_x;
        float ground_restore_y;
        float ground_restore_z;

        obj->hide_flag_bits.pin_animation = 0;
        ground_restore_x = obj->ground_restore_pos.x;
        ground_restore_y = obj->ground_restore_pos.y;
        ground_restore_z = obj->ground_restore_pos.z;

        pose_anim(anim, 0);
        obj->ground_restore_pos.x = ground_restore_x;
        obj->ground_restore_pos.y = ground_restore_y;
        obj->ground_restore_pos.z = ground_restore_z;
    } else {
        pose_anim(anim, 0);
    }
    return 1;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void transition_to_anim_script(
    float transition_frames,
    AnimPdata* anim,
    AnimScript* script,
    unsigned int flags) {
    transition_to_anim_script_frame(
        transition_frames, 0.0f, anim, script, flags);
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int set_anim_script_frame(
    float frame,
    AnimPdata* anim,
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

    if ((flags & 0x80) == 0) {
        obj = anim->obj;
        if (obj != 0) {
            if (obj->hdr.instance != anim->obj_instance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            if ((flags & 0x200) != 0) {
                obj->hide_flag_bits.bit0 = 1;
            }
            if (obj->flags_0B_bits.root_transform_pending != 0) {
                MkBone* root = obj->bones[obj->fallback_bone_index];

                if (root != 0 && root->parent_matrix != 0) {
                    MKMATRIX yaw_matrix __attribute__((aligned(16)));
                    Vec adjusted_position;
                    Quat old_rotation;
                    Quat correction;
                    Quat* root_rotation = &root->rotation;
                    float root_yaw = quat_extract_ang_y(root_rotation);

                    obj->ang.y += root_yaw - obj->bone_angle_68;
                    y_angle_to_MKMATRIX(&yaw_matrix, root_yaw);
                    v3_x_mat_sub_v3(
                        &adjusted_position,
                        &root->translation.value,
                        &yaw_matrix,
                        &obj->pos);
                    scale_v3(
                        &obj->pos,
                        &adjusted_position,
                        -1.0f);
                    obj->bone_angle_68 = root_yaw;
                    y_angle_to_MKMATRIX(
                        &yaw_matrix,
                        obj->bone_angle_64 - root_yaw);
                    if (RtQuatConvertFromMatrix(
                            (RtQuat*)&correction,
                            &yaw_matrix)) {
                        gxQuatCopy(&old_rotation, root_rotation);
                        gxQuatMul(
                            root_rotation, &correction, &old_rotation);
                        gxQuatNorm(root_rotation);
                    }
                }
                obj->flags_0B_bits.root_transform_pending = 0;
            }
        }
    }

    obj = anim->obj;
    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0 && obj->hide_flag_bits.pin_animation != 0) {
        float ground_restore_x;
        float ground_restore_y;
        float ground_restore_z;

        obj->hide_flag_bits.pin_animation = 0;
        ground_restore_x = obj->ground_restore_pos.x;
        ground_restore_y = obj->ground_restore_pos.y;
        ground_restore_z = obj->ground_restore_pos.z;
        pose_anim(anim, 0);
        obj->ground_restore_pos.x = ground_restore_x;
        obj->ground_restore_pos.y = ground_restore_y;
        obj->ground_restore_pos.z = ground_restore_z;
    } else {
        pose_anim(anim, 0);
    }
    return same_script;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

#pragma dont_inline on
void set_anim_script(
    AnimPdata* anim,
    AnimScript* script,
    unsigned int flags) {
    set_anim_script_frame(0.0f, anim, script, flags);
}
#pragma dont_inline reset

void toggle_obj_and_ani_flips(AnimPdata* anim) {
    MkObj* obj = anim->obj;

    if (obj != 0) {
        if (obj->hdr.instance != anim->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    obj->hide_flag_bits.bit6 = 1 - obj->hide_flag_bits.bit6;
    anim->flags ^= 8;
    anim->old_flags ^= 8;
}

void anim_set_hiframe(AnimPdata* anim, float frame) {
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

float anim_script_lastframe(const AnimScript* script) {
    return (float)(script->frame_count - 1);
}

void reset_ani_data_space(void) {
}

MkProc* create_mkproc_face_anim(
    int pid, MkProcEntryFn entry, AnimPdata** out_anim) {
    int flags = anim_create_proc_flags();
    MkProc* proc = get_mkproc_tinystack(&flags);
    AnimPdata* anim = get_mkpdata_anim();

    *out_anim = anim;
    proc = create_mkproc(0xD, proc, pid, entry, &anim->hdr);
    if (proc != 0) {
        anim->proc = proc;
        anim->proc_instance = proc->instance;
        proc->pre_destroy = pw_anim;
        proc->destroy_cb = ps_anim;
    }
    if (proc != 0) {
        (*out_anim)->auxiliary_track = 1;
    }
    return proc;
}

MkProc* create_mkproc_hand_anim(
    int pid, MkProcEntryFn entry, AnimPdata** out_anim) {
    int flags = anim_create_proc_flags();
    MkProc* proc = get_mkproc_tinystack(&flags);
    AnimPdata* anim = get_mkpdata_anim();

    *out_anim = anim;
    proc = create_mkproc(0xD, proc, pid, entry, &anim->hdr);
    if (proc != 0) {
        anim->proc = proc;
        anim->proc_instance = proc->instance;
        proc->pre_destroy = pw_anim;
        proc->destroy_cb = ps_anim;
    }
    if (proc != 0) {
        (*out_anim)->auxiliary_track = 1;
    }
    return proc;
}

MkProc* create_mkproc_anim2(
    int pid, MkProcEntryFn entry, AnimPdata** out_anim) {
    int flags = anim_create_proc_flags();
    MkProc* proc = get_mkproc_tinystack(&flags);
    AnimPdata* anim = get_mkpdata_anim();

    *out_anim = anim;
    proc = create_mkproc(0xC, proc, pid, entry, &anim->hdr);
    if (proc != 0) {
        anim->proc = proc;
        anim->proc_instance = proc->instance;
        proc->pre_destroy = pw_anim;
        proc->destroy_cb = ps_anim;
    }
    return proc;
}

MkProc* create_mkproc_anim(
    int pid, MkProcEntryFn entry, AnimPdata** out_anim) {
    int flags = anim_create_proc_flags();
    MkProc* proc = get_mkproc_tinystack(&flags);
    AnimPdata* anim = get_mkpdata_anim();

    *out_anim = anim;
    proc = create_mkproc(0xB, proc, pid, entry, &anim->hdr);
    if (proc != 0) {
        anim->proc = proc;
        anim->proc_instance = proc->instance;
        proc->pre_destroy = pw_anim;
        proc->destroy_cb = ps_anim;
    }
    return proc;
}

void vdestroy_mkpdata_anim(AnimPdata* anim) {
    if (anim->track_data != 0) {
        free_mem(anim->track_data);
    }
    anim->hdr.instance = 0;
    mkhdr_memfree(&anim->hdr);
}

AnimPdata* get_mkpdata_anim(void) {
    AnimPdata* anim =
        (AnimPdata*)get_mkhdr(&vtbl_mkpdata_anim, sizeof(AnimPdata));

    if (anim != 0) {
        anim->proc = 0;
        anim->proc_instance = 0;
        anim->obj = 0;
        anim->obj_instance = 0;
        anim->owner = 0;
        anim->owner_instance = 0;
        anim->script = 0;
        anim->old_script = 0;
        anim->flags = 0;
        anim->old_flags = 0;
        anim->transition_weight = 0.0f;
        anim->transition_step = 0.0f;
        anim->transition_accel = 0.0f;
        anim->hand_transition = 1.0f;
        anim->hand_transition_step = 0.0f;
        anim->hand_transition_limit = 1.0f;
        anim->previous_frame = 0.0f;
        anim->frame = 0.0f;
        anim->old_frame = 0.0f;
        anim->low_frame = 0.0f;
        anim->old_low_frame = 0.0f;
        anim->high_frame = 0.0f;
        anim->old_high_frame = 0.0f;
        anim->step = 1.0f;
        anim->step_accel = 0.0f;
        anim->frame_callback = 0;
        anim->old_frame_callback = 0;
        anim->obj_movement_weight = 0.0f;
        anim->root_movement_weight = 1.0f;
        anim->track_capacity = 0;
        anim->track_data = 0;
        anim->tag_frame = 0;
        anim->bone_remap = 0;
        anim->hand_anim_script = 0;
        anim->next_hand_script = 0;
        anim->hand_transition_frames = 1.0f;
        anim->hand_flags = 0;
        anim->field_E8 = 0;
        anim->anim_offset.x = 0.0f;
        anim->anim_offset.y = 0.0f;
        anim->anim_offset.z = 0.0f;
        anim->anim_angle = 0.0f;
    }
    return anim;
}

int obj_get_bid_for_tid(MkObj* obj, int tag) {
    unsigned int bone_index = tag & 0xFFF;
    unsigned int i;
    MkBone* bone;

    if (bone_index < obj->bone_count) {
        bone = obj->bones[bone_index];
        if (bone != 0 && bone->tag == tag) {
            return (int)bone_index;
        }
    }
    if ((tag & 0x2000) != 0) {
        for (i = 0; i < obj->cloth_bone_count; i++) {
            bone = obj->cloth_bones[i].bone;
            if (bone->tag == tag) {
                return bone->bone_index;
            }
        }
    } else {
        for (i = 0; i < obj->bone_count; i++) {
            bone = obj->bones[i];
            if (bone != 0 && bone->tag == tag) {
                return (int)i;
            }
        }
    }
    return 0;
}

void mkobj_destroy_bones(MkObj* obj) {
    MkBone* bone;
    unsigned int i;

    if (obj->bones != 0) {
        for (i = 0; i < obj->bone_count; i++) {
            bone = obj->bones[i];
            if (bone != 0) {
                destroy_list(&bone->list_80);
                free_mem(bone);
            }
        }
        free_mem(obj->bones);
    }
}

int build_bones_tbl(MkObj* obj, const int* tags) {
    RpHAnimHierarchy* hierarchy;
    RwFrame* frame;
    int bone_count;
    const int* tag;

    if (obj->bones != 0) {
        return 1;
    }

    hierarchy = 0;
    frame = (RwFrame*)obj->clump->object.parent;
    RwFrameForAllChildren(
        frame, get_child_frame_hierarchy, (void*)&hierarchy);
    if (hierarchy == 0) {
        return 0;
    }

    bone_count = hierarchy->numNodes;
    for (tag = tags; *tag != 0; tag++) {
        if (*tag == -1) {
            bone_count++;
        }
    }
    obj->bones = (MkBone**)get_mem(bone_count * sizeof(MkBone*));
    if (obj->bones == 0) {
        return 0;
    }
    obj->bone_count = bone_count;
    process_obj_bones(obj, tags);
    insert_bone_hierarchy_mkobj(obj);
    update_bone_hierarchy(obj != 0 ? as_mkhdr(&obj->hdr) : 0);
    return 1;
}

MkBone* alloc_bone(void) {
    MkBone* bone = (MkBone*)get_mem(sizeof(MkBone));

    if (bone != 0) {
        Quat* rotation;
        int count;

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
        bone->cloth_link = 0;
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
        bone->delta.value.z = 0.0f;
        bone->delta.value.y = 0.0f;
        bone->delta.value.x = 0.0f;
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
        rotation = bone->rotations;
        count = 2;
        do {
            rotation->x = 0.0f;
            rotation->y = 0.0f;
            rotation->z = 0.0f;
            rotation->w = 1.0f;
            rotation++;
        } while (--count != 0);
        bone->field_5C = 0.0f;
        bone->scale.x = 1.0f;
        bone->scale.y = 1.0f;
        bone->scale.z = 1.0f;
    }
    return bone;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static void process_obj_bones(MkObj* obj, const int* tags) {
    RpHAnimHierarchy* found_hierarchy = 0;
    RpHAnimHierarchy* hierarchy;
    MkBone* parent_stack[256];
    int bone_indices[256];
    MkBone* linked_bones[256];
    MkBone* parent;
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
        &found_hierarchy);
    hierarchy = found_hierarchy;
    if (hierarchy == 0) {
        return;
    }
    hierarchy->flags |= 0x4000;
    RpClumpForAllAtomics(
        obj->clump,
        atomic_set_HAnimHierarchy,
        hierarchy);

    bone_count = obj->bone_count;
    stack_index = -1;
    matrix = hierarchy->pMatrixArray;
    parent = 0;
    hierarchy_count = hierarchy->numNodes;
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
        RpHAnimNodeInfo* node = &hierarchy->pNodeInfo[i];
        unsigned int node_id = node->nodeID;
        int masked_id = node_id & 0xFFF07FFF;
        int limb_id = (node_id >> 16) & 0xF;
        int tag;
        int j;

        bone_indices[i] = -1;
        linked_bones[i] = 0;
        j = 0;
        while ((tag = tags[j]) != 0 && j < bone_count) {
            if (tag != -1) {
                MkBone* bone = obj->bones[j];

                if (masked_id == tag) {
                    if ((node->nodeID & 0x8000) != 0) {
                        linked_bones[i] = bone;
                    } else if (bone->tag == -2) {
                        bone_indices[i] = j;
                        bone->tag = node->nodeID & 0xFFF0FFFF;
                        bone->limb_id = limb_id;
                    }
                }
            }
            j++;
        }
    }

    next_unmapped = 0;
    for (i = 0; i < hierarchy_count; i++, matrix++) {
        RpHAnimNodeInfo* node = &hierarchy->pNodeInfo[i];
        MkBone* bone;
        BoneScanContext context;

        if (bone_indices[i] > -1) {
            bone = obj->bones[bone_indices[i]];
        } else {
            while (obj->bones[next_unmapped]->tag != -2) {
                next_unmapped++;
            }
            bone = obj->bones[next_unmapped];
            bone->tag = node->nodeID & 0xFFF0FFFF;
            bone_indices[i] = next_unmapped;
            bone->limb_id = (node->nodeID >> 16) & 0xF;
            if (linked_bones[i] != 0) {
                MkBone* linked = linked_bones[i];

                bone->clone_source = linked;
                bone->root_next = linked->root_next;
                linked->root_next = bone;
            }
        }

        if ((bone->tag & 0x1000) != 0) {
            bone->flags_54_bits.tag_1000 = 1;
        }
        if ((bone->tag & 0x2000) != 0) {
            bone->flags_54_bits.cloth_candidate = 1;
        }
        bone->parent_matrix = matrix;
        bone->original_parent_matrix = matrix;
        matrix->flags = 3;
        bone->update_tick = exec_tick_ctr - 1;

        context.bone_index = i;
        RpClumpForAllAtomics(
            obj->clump, ScanForBone_callback, &context);
        {
            RwMatrix* scan_matrix = context.matrix;

            if (scan_matrix != 0) {
                bone->bind_offset.x = -scan_matrix->pos.x;
                bone->bind_offset.y = -scan_matrix->pos.y;
                bone->bind_offset.z = -scan_matrix->pos.z;
            } else {
                bone->bind_offset.x = 0.0f;
                bone->bind_offset.y = 0.0f;
                bone->bind_offset.z = 0.0f;
            }
        }

        if (parent != 0) {
            MkBone* tail;

            bone->translation.value.x =
                bone->bind_offset.x - parent->bind_offset.x;
            bone->translation.value.y =
                bone->bind_offset.y - parent->bind_offset.y;
            bone->translation.value.z =
                bone->bind_offset.z - parent->bind_offset.z;
            bone->field_5C = 1.0f / length_v3(&bone->translation.value);
            if (bone->tree_next == 0) {
                parent->flags_54_bits.has_children = 1;
                tail = parent->tree_child;
                if (tail == 0) {
                    parent->tree_child = bone;
                } else {
                    while (tail->tree_next != 0) {
                        tail = tail->tree_next;
                    }
                    tail->tree_next = bone;
                }
                bone->transform_parent = parent;
            }
        } else {
            bone->translation.value.x = 0.0f;
            bone->translation.value.y = 0.0f;
            bone->translation.value.z = 0.0f;
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

void mkbone_remove(MkBone* bone) {
    destroy_list(&bone->list_80);
    bone->parent_matrix = 0;
}

void mkbone_insert_child_of_clone_parent(MkBone* bone, MkBone* parent) {
    bone->clone_source = parent;
    bone->root_next = parent->root_next;
    parent->root_next = bone;
}

void bone_make_parents_my_children(MkBone* bone) {
    _bone_make_parents_my_children(bone);
    bone->translation = bone->parent_matrix->pos_row;
    RtQuatConvertFromMatrix(&bone->rt_rotation, bone->parent_matrix);
}

static inline void quat_to_normalized_matrix(
    RwMatrix* matrix, const Quat* rotation) {
    float x = rotation->x;
    float y = rotation->y;
    float z = rotation->z;
    float w = rotation->w;
    float x_squared = x * x;
    float y_squared = y * y;
    float z_squared = z * z;
    float w_squared = w * w;
    float magnitude_squared =
        w_squared + (z_squared + (x_squared + y_squared));
    float scale = 2.0f / magnitude_squared;
    float xs = x * scale;
    float ys = y * scale;
    float zs = z * scale;
    float xx = x * xs;
    float yy = y * ys;
    float zz = z * zs;
    float xw = xs * w;
    float yz = y * zs;
    float xz = z * xs;
    float yw = ys * w;
    float xy = x * ys;
    float zw = zs * w;

    matrix->pos.x = 0.0f;
    matrix->pos.y = 0.0f;
    matrix->pos.z = 0.0f;
    matrix->flags = 3;
    matrix->right.x = 1.0f - (yy + zz);
    matrix->up.y = 1.0f - (zz + xx);
    matrix->at.z = 1.0f - (xx + yy);
    matrix->up.z = yz + xw;
    matrix->at.y = yz - xw;
    matrix->right.z = xz - yw;
    matrix->at.x = xz + yw;
    matrix->right.y = xy + zw;
    matrix->up.x = xy - zw;
}

static void _bone_make_parents_my_children(MkBone* bone) {
    MkBone* parent = bone->transform_parent;
    Vec displacement;
    Quat inverse_rotation __attribute__((aligned(16)));

    if (parent == 0) {
        return;
    }
    if (parent->tree_child == bone) {
        parent->tree_child = bone->tree_next;
        if (parent->tree_child == 0) {
            parent->flags_54_bits.has_children = 0;
        }
    } else {
        MkBone* sibling = parent->tree_child;
        while (sibling != 0) {
            MkBone* next = sibling->tree_next;

            if (next == bone) {
                sibling->tree_next = bone->tree_next;
                break;
            }
            sibling = next;
        }
    }
    bone->transform_parent = 0;
    bone->tree_next = 0;

    _bone_make_parents_my_children(parent);
    if (parent->tree_next == 0) {
        bone->flags_54_bits.has_children = 1;
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
    parent->rotation.w *= -1.0f;
    PSVECSubtract(
        &parent->parent_matrix->pos_vec,
        &bone->parent_matrix->pos_vec,
        &displacement);
    inverse_rotation = bone->rotation_90;
    inverse_rotation.w *= -1.0f;
    quat_to_normalized_matrix(&tmp_matrix, &inverse_rotation);
    v3_x_mat(&parent->translation.value, &displacement, &tmp_matrix);
    parent->flags_55_bits.reparent_toggle =
        1 - parent->flags_55_bits.reparent_toggle;
}

void mkbone_insert_child_of_parent(MkBone* child, MkBone* parent) {
    MkBone* tail;

    if (child->tree_next != 0) {
        return;
    }
    parent->flags_54_bits.has_children = 1;
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

static RpAtomic* ScanForBone_callback(
    RpAtomic* atomic, void* data) {
    BoneScanContext* context = data;
    RpSkin* skin;

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

static RpAtomic* atomic_set_HAnimHierarchy(RpAtomic* atomic, void* hierarchy) {
    RpSkinAtomicSetHAnimHierarchy(atomic, hierarchy);
    return atomic;
}

static RwFrame* get_child_frame_hierarchy(RwFrame* frame, void* out_data) {
    RpHAnimHierarchy* hierarchy = RpHAnimFrameGetHierarchy(frame);

    if (hierarchy == 0) {
        RwFrameForAllChildren(frame, get_child_frame_hierarchy, out_data);
        return frame;
    }
    *(RpHAnimHierarchy**)out_data = hierarchy;
    return 0;
}
