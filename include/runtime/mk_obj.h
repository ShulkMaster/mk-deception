#ifndef MK_OBJ_H
#define MK_OBJ_H

#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "math/gxVect.h"
#include "math/gxQuat.h"
#include "runtime/limb.h"
#include "runtime/mk_struct.h"

typedef struct  {
    unsigned char collision_disabled : 1; /* bit7 */
    unsigned char collision_deferred : 1; /* bit6 */
    unsigned char scale_controlled : 1; /* bit5 - blade/controller scale */
    unsigned char pad_bit4 : 1;
    unsigned char pad_bit3 : 1;
    unsigned char reparent_toggle : 1; /* bit2 */
    unsigned char preserve_rotation : 1;
    unsigned char pad_bit0 : 1;
} MkBoneFlags55;

typedef struct MkFlippedBoneMap {
    int count;
    int* bone_indices;
} MkFlippedBoneMap;

typedef struct MkBoneFlags54 {
    unsigned char transform_parented : 1; /* bit7 */
    unsigned char tag_1000 : 1; /* bit6 - copied from bone tag */
    unsigned char cloth_candidate : 1; /* bit5 */
    unsigned char calculation_locked : 1; /* bit4 */
    unsigned char field_bit3 : 1;
    unsigned char hierarchy_driven : 1; /* bit2 */
    unsigned char pose_matrix_applied : 1; /* bit1 */
    unsigned char has_children : 1; /* bit0 */
} MkBoneFlags54;

typedef struct MkBone {
    struct RwMatrix matrix; /* +0x00 - calculated bone matrix */
    struct RwMatrix* parent_matrix; /* +0x40 */
    struct RwMatrix* original_parent_matrix; /* +0x44 */
    int tag; /* +0x48 */
    int limb_id; /* +0x4C - limb transfer lookup id */
    int bone_index; /* +0x50 - index used by limb transfer */
    union {
        struct {
            MkBoneFlags54 flags_54_bits; /* +0x54 */
            MkBoneFlags55 flags_55_bits; /* +0x55 */
            char pad56[2];
        };
        unsigned int flags_word_54;
    };
    struct ClothBone* cloth_link; /* +0x58 - optional cloth state */
    float field_5C;
    float field_60;
    float field_64;
    unsigned int update_tick; /* +0x68 - last hierarchy update tick */
    struct MkBone* transform_parent;  /* +0x6C */
    struct MkBone* tree_next;      /* +0x70 */
    struct MkBone* tree_child;     /* +0x74 */
    struct MkBone* root_next;      /* +0x78 */
    struct MkBone* clone_source;   /* +0x7C */
    MkPtr* list_80;                /* +0x80 - owned auxiliary bone list */
    char pad84[0x0C];
    Quat rotation_90; /* +0x90 */
    RwMatrixPosition translation; /* +0xA0 */
    Vec scale; /* +0xB0 */
    char padBC[4];
    RwMatrixPosition delta; /* +0xC0 */
    struct {
        Quat rotation; /* +0xD0 - x, y, z, w */
        Quat rotation_e0; /* +0xE0 */
        RwMatrixPosition velocity; /* +0xF0 */
        Vec bind_offset; /* +0x100 - negated skin-to-bone translation */
        char pad10C[4];
    };
} MkBone;

typedef struct ClothBoneFlags30 {
    unsigned char use_ground_y : 1; /* bit7 */
    unsigned char pad : 7;
} ClothBoneFlags30;

typedef struct ClothCollisionPoint {
    Vec position; /* +0x00 */
    char pad_0C[4];
} ClothCollisionPoint; /* 0x10 */

typedef struct ClothBone {
    unsigned int collision_point_count; /* +0x00 */
    float table_scale; /* +0x04 */
    float stiffness_squared; /* +0x08 */
    float segment_length; /* +0x0C */
    float damping_factor; /* +0x10 */
    float initial_x; /* +0x14 */
    float initial_z; /* +0x18 */
    float table_weight; /* +0x1C */
    float force_step; /* +0x20 */
    float stretch_weight; /* +0x24 */
    float ground_y; /* +0x28 */
    int active; /* +0x2C */
    union {
        unsigned char flags_30;
        ClothBoneFlags30 flags_30_bits;
    }; /* +0x30 */
    char pad_31[3];
    struct ClothBone* target_bone; /* +0x34 */
    MkBone* bone; /* +0x38 */
    float rest_length; /* +0x3C */
    float collision_amount; /* +0x40 */
    float current_length; /* +0x44 */
    char pad_48[8];
    Vec wind_normal; /* +0x50 */
    char pad_5C[4];
    Vec collision_offset; /* +0x60 */
    char pad_6C[4];
    union {
        ClothCollisionPoint collision_points[3]; /* +0x70, stride 0x10 */
        struct {
            char pad_local_70[0x20];
            Vec local_cloth_position; /* +0x90 */
            char pad_local_9C[4];
        };
    };
    Vec target_vector; /* +0xA0 */
    char pad_AC[4];
    Vec world_target; /* +0xB0 */
    char pad_BC[4];
    Vec force_position; /* +0xC0 */
    char pad_CC[4];
    Vec velocity; /* +0xD0 */
    char pad_DC[4];
    Quat collision_rotation; /* +0xE0 */
    Vec collision_local_position; /* +0xF0 */
    char pad_FC[4];
    Vec previous_collision_local_position; /* +0x100 */
    char pad_10C[0x24];
} ClothBone; /* 0x130 */

typedef struct MkxMem {
    MkHdr hdr; /* +0x00 */
    void* allocation; /* +0x08 */
} MkxMem;

typedef struct MkSobjFlags09 {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char bit2 : 1;
    unsigned char has_pebbles : 1;
    unsigned char bit0 : 1;
} MkSobjFlags09;

typedef struct MkSobjFlags08 {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char angular_velocity_enabled : 1; /* bit2 */
    unsigned char scale_dirty : 1; /* bit1 */
    unsigned char bit0 : 1;
} MkSobjFlags08;


/*
 * Midway mksobj (partial) -- pebble/render/hide agree on atomic @ +0x14.
 * id in low 12 bits of id_flags; bit 0x80000000 used with priority 0x12.
 * frame @ +0x18 used by particle binds (RwFrameGetLTM).
 */
typedef struct MkSobj {
    MkHdr hdr;             /* +0x00 */
    union {
        unsigned int flags_word_08;
        struct {
            union {
                unsigned char flags_08; /* +0x08 - transform update flags */
                MkSobjFlags08 flags_08_bits;
            };
            union {
                unsigned char flags09; /* +0x09 - render flags */
                MkSobjFlags09 flags09_bits;
            };
            char pad0A[2];
        };
    };
    unsigned int id_flags; /* +0x0C */
    int priority;          /* +0x10 */
    RpAtomic* atomic;      /* +0x14 */
    RwFrame* frame;        /* +0x18 */
    struct MkObj* owner;   /* +0x1C */
    MkHdr* bound_hdr;      /* +0x20 */
    unsigned int bound_instance; /* +0x24 */
    float z_offset;        /* +0x28 */
    unsigned int render_flags; /* +0x2C */
    Vec pos;               /* +0x30 */
    char pad3C[4];
    Vec pos_vel;           /* +0x40 */
    char pad4C[4];
    Vec ang;               /* +0x50 */
    char pad5C[4];
    Vec ang_vel;           /* +0x60 */
    char pad6C[4];
    Vec scale;             /* +0x70 */
    char pad7C[4];
    struct RwMatrix* matrices; /* +0x80 - mapped transform palette */
} MkSobj;

/* Atomic plugin userdata (id 0x895302) -- same layout as mk_plugins MksobjPluginData. */
typedef struct MksobjPluginData {
    unsigned int flags; /* +0x00 -- bit 0x80000000 = transl/priority */
    float field_04;     /* +0x04 */
    MkSobj* sobj;       /* +0x08 -- runtime link; not streamed */
    int field_0C;          /* +0x0C -- pipeline/material type */
} MksobjPluginData;

typedef struct MkObjFlags0C {
    unsigned char pad_bit7 : 1;
    unsigned char tag_flag_40 : 1; /* bit6 */
    unsigned char tag_flag_20 : 1; /* bit5 */
    unsigned char tag_flag_10 : 1; /* bit4 */
    unsigned char tag_flag_08 : 1; /* bit3 */
    unsigned char cloth_update : 1; /* bit2 */
    unsigned char parented : 1; /* bit1 */
    unsigned char bit0 : 1;
} MkObjFlags0C;

typedef struct MkObjFlags09 {
    unsigned char launched : 1; /* bit7 */
    unsigned char bit6 : 1;
    unsigned char tightrope_restricted : 1; /* bit5 */
    unsigned char bit4 : 1;
    unsigned char face_opponent : 1;        /* bit3 */
    unsigned char wall_restricted : 1;      /* bit2 */
    unsigned char head_tracking : 1;        /* bit1 */
    unsigned char bit0 : 1;
} MkObjFlags09;

typedef struct MkObjFlags08 {
    unsigned char bit7 : 1;
    unsigned char airborne : 1; /* bit6 */
    unsigned char gravity_enabled : 1; /* bit5 */
    unsigned char transform_dirty : 1; /* bit4 */
    unsigned char angular_velocity_enabled : 1; /* bit3 */
    unsigned char rotation_enabled : 1; /* bit2 */
    unsigned char scale_active : 1; /* bit1 */
    unsigned char moving : 1;          /* bit0 */
} MkObjFlags08;

typedef struct MkObjFlags0B {
    unsigned char root_transform_pending : 1; /* bit7 */
    unsigned char bit6 : 1;
    unsigned char force_anim_speed : 1; /* bit5 */
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char pivot_enabled : 1; /* bit2 */
    unsigned char special_texture : 1; /* bit1 */
    unsigned char pad_low : 1;
} MkObjFlags0B;

typedef struct MkObjHideFlags {
    unsigned char still_move : 1; /* bit7 */
    unsigned char bit6 : 1;
    unsigned char hidden : 1;     /* bit5 */
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char weapon_effect : 1; /* bit2 - weapon/cloth auxiliary */
    unsigned char pin_animation : 1; /* bit1 */
    unsigned char bit0 : 1;
} MkObjHideFlags;

typedef struct MkObjItemAttachData {
    char pad00[8];
    int bone_index; /* +0x08 */
    Vec position;   /* +0x0C */
    Vec rotation;   /* +0x18 */
    Vec scale;      /* +0x24 */
} MkObjItemAttachData; /* 0x30 */

/*
 * MkObj (partial) -- fields used by mk_obj + particle binds.
 * hide_flags bit5 = hidden; flags_0C bit1 = inherit hide from parent.
 */
typedef struct MkObj {
    MkHdr hdr;                /* +0x00 - instance @ +0x04 */
    union {
        unsigned int flags_word_08;
        struct {
            unsigned char flags_08;
            unsigned char flags_09;
            unsigned char hide_flags;
            unsigned char flags_0B;
        } flag_bytes;
        struct {
            union {
                unsigned char flags_08;
                MkObjFlags08 flags_08_bits;
            }; /* +0x08 */
            union {
                unsigned char flags_09;
                MkObjFlags09 flags_09_bits;
            }; /* +0x09 */
            union {
                unsigned char hide_flags;
                MkObjHideFlags hide_flag_bits;
            }; /* +0x0A */
            union {
                unsigned char flags_0B;
                MkObjFlags0B flags_0B_bits;
            }; /* +0x0B */
        };
    };
    union {
        unsigned int flags_word_0C;
        struct {
            union {
                unsigned char flags_0C;
                MkObjFlags0C flags_0C_bits;
            }; /* +0x0C */
            char pad0D[3];
        };
    };
    unsigned int oid;       /* +0x10 - object id / destroy mask */
    int clump_count;        /* +0x14 - populated inline clump slots */
    union {
        struct {
            RpClump* clump;   /* +0x18 - first clump */
            RpClump* clump_1; /* +0x1C - second inline clump slot */
        };
        RpClump* clumps[2];
    };
    RwFrame* frame;         /* +0x20 */
    struct RwMatrix* field_24; /* +0x24 - matrix / fallback bone LTM */
    MkPtr* child_list;        /* +0x28 - mk_insert list head (sky / children) */
    unsigned int light_flags; /* +0x2C - bgnd load 0 / 0x1000 / 0x1009 */
    float gravity;             /* +0x30 */
    char pad34[4];
    int ground_bone;           /* +0x38 */
    MkHdr* parent_hdr;        /* +0x3C */
    unsigned int parent_inst; /* +0x40 */
    MkPtr* list_44;           /* +0x44 - owned list */
    MkBone** bones; /* +0x48 */
    unsigned int bone_count; /* +0x4C */
    int fallback_bone_index; /* +0x50 */
    int* matrix_indices;     /* +0x54 - owned mapped-matrix indices */
    unsigned int matrix_count; /* +0x58 */
    union {
        void* field_5C;
        MkObjItemAttachData* item_attach_data;
    }; /* +0x5C - cleared during destruction / item attachment data */
    unsigned int field_60;
    float bone_angle_64;
    float bone_angle_68;
    void* ground_colls;     /* +0x6C */
    float ground_colls_y;   /* +0x70 */
    ClothBone* cloth_bones; /* +0x74 - owned cloth-bone allocation */
    unsigned int cloth_bone_count; /* +0x78 */
    MkPtr* list_7C;         /* +0x7C - owned list */
    MkPtr* list_80;         /* +0x80 - owned list */
    MkPtr* sobj_list; /* +0x84 */
    MkPtr* list_88;   /* +0x88 - owned list */
    MkFlippedBoneMap* flipped_bone_map; /* +0x8C */
    Vec ground_restore_pos; /* +0x90 */
    char pad9C[4];
    RwMatrixPosition pos; /* +0xA0 */
    union {
        struct {
            Vec pos_vel; /* +0xB0 */
            unsigned int pos_vel_pad;
        };
        RwMatrixPosition pos_vel_row;
    };
    Vec pivot; /* +0xC0 - alternate-position matrix offset */
    char padCC[4];
    union {
        struct {
            union {
                struct {
                    union {
                        Vec ang; /* +0xD0 */
                        struct {
                            float dir_x; /* light direction */
                            float dir_y;
                            float dir_z;
                        };
                    };
                    unsigned int ang_pad;
                };
                RwMatrixPosition ang_row;
            };
            union {
                struct {
                    Vec ang_vel; /* +0xE0 */
                    unsigned int ang_vel_pad;
                };
                RwMatrixPosition ang_vel_row;
            };
            union {
                struct {
                    Vec scale;   /* +0xF0 */
                    unsigned int scale_pad;
                };
                RwMatrixPosition scale_row;
            };
        };
        struct {
            Quat orientation_quat; /* +0xD0 */
            char padE0_quat[0x0C];
            Quat secondary_quat; /* +0xEC */
        };
    };
} MkObj;

/* Critical krypt APIs */
MkSobj* obj_find_sobj_by_id(MkObj* obj, unsigned int id);
RpMaterial* sobj_find_material_by_id(MkSobj* sobj,
                                     unsigned int material_id);
void sobj_set_priority(void* sobj, int priority);
void hide_sobj(void* sobj);
void unhide_sobj(void* sobj);
void hide_sobj_and_children(MkSobj* sobj);
void unhide_sobj_and_children(MkSobj* sobj);

/* Frequently called siblings */
MkSobj* obj_create_sobjs_by_id(MkObj* obj, int id);
void insert_fgnd_mkobj(void* obj);
void update_mkobj(void* obj);
void update_obj_pos(MkObj* obj);
void obj_set_pos(MkObj* obj, Vec* pos);
void obj_get_pos(MkObj* obj, Vec* out);
void obj_set_sobj_alpha(MkObj* obj, int sobj_index, int alpha);
void obj_set_sobj_priority(MkObj* obj, int id, int priority);
MkSobj* hide_sobj_by_sobj_id(void* obj, unsigned int id);
MkSobj* unhide_sobj_by_sobj_id(void* obj, unsigned int id);
void hide_obj(void* obj);
void unhide_obj(void* obj);
MkSobj* obj_first_sobj(MkObj* obj);
void sobj_set_transl_flag(MkSobj* sobj);
Vec* sobj_get_world_pos(MkSobj* sobj);
RpAtomic* obj_get_1st_atomic(MkObj* obj);
void insert_bone_hierarchy_mkobj(MkObj* obj);
RwTexture* material_get_texture_pointer(
    RpMaterial* material, int use_matfx);
void material_set_texture_pointer(
    RpMaterial* material, RwTexture* texture, int use_matfx);
void* get_mkx_mem(void* allocation);
MkObj* get_mkobj(int type, RpClump* clump);
void destroy_mkobj(void* obj);

#endif
