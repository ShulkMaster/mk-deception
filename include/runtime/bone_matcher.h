#ifndef MKD_BONE_MATCHER_H
#define MKD_BONE_MATCHER_H

#include "runtime/mk_struct.h"
#include "math/gxVect.h"
#include "math/gxQuat.h"
#include "rw/rwcore_types.h"

typedef struct MkObj MkObj;
typedef struct MkSobj MkSobj;

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

typedef char BoneMatcherStateSize[(sizeof(BoneMatcherState) == 0x110) ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
BoneMatcherState* start_bone_matcher(
    MkObj* parent, int parent_bone, MkObj* child, int child_bone,
    float blend_ticks);
#ifdef __cplusplus
}
#endif

#endif
