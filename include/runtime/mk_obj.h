#ifndef MK_OBJ_H
#define MK_OBJ_H

#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "math/gxVect.h"
#include "math/gxQuat.h"
#include "runtime/limb.h"
#include "runtime/mk_struct.h"

typedef struct MkBoneFlags55 {
    unsigned char pad_high : 2;
    unsigned char scale_controlled : 1; /* bit5 - blade/controller scale */
    unsigned char pad_low : 5;
} MkBoneFlags55;

typedef struct MkBoneFlags54 {
    unsigned char transform_parented : 1; /* bit7 */
    unsigned char pad_6_5 : 2;
    unsigned char calculation_locked : 1; /* bit4 */
    unsigned char field_bit3 : 1;
    unsigned char hierarchy_driven : 1; /* bit2 */
    unsigned char pad_1_0 : 2;
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
            union {
                unsigned char flags_54; /* +0x54 */
                MkBoneFlags54 flags_54_bits;
            };
            union {
                unsigned char flags_55; /* +0x55 - bit7 collapsed */
                MkBoneFlags55 flags_55_bits;
            };
            char pad56[2];
        };
        unsigned int flags_word_54;
    };
    int field_58;
    float field_5C;
    float field_60;
    float field_64;
    union {
        unsigned int update_tick; /* +0x68 - last hierarchy update tick */
        float root_angle_y;
    };
    struct MkBone* transform_parent;  /* +0x6C */
    struct MkBone* tree_next;      /* +0x70 */
    struct MkBone* tree_child;     /* +0x74 */
    struct MkBone* root_next;      /* +0x78 */
    struct MkBone* clone_source;   /* +0x7C */
    MkPtr* list_80;                /* +0x80 - owned auxiliary bone list */
    char pad84[0x0C];
    Quat rotation_90; /* +0x90 */
    union {
        struct {
            Vec translation; /* +0xA0 */
            unsigned int translation_pad;
        };
        RwMatrixPosition translation_row;
    };
    Vec scale; /* +0xB0 */
    char padBC[4];
    Vec delta; /* +0xC0 */
    char padCC[4];
    union {
        struct {
            union {
                Quat rotation;
                RtQuat rt_rotation;
            }; /* +0xD0 */
            Quat rotation_e0; /* +0xE0 */
            Vec velocity; /* +0xF0 */
            char padFC[4];
            Vec bind_offset; /* +0x100 - negated skin-to-bone translation */
            char pad10C[4];
        };
        Quat rotations[2];
        RwMatrix trail_matrix; /* +0xD0 - weapon-trail chain scratch transform */
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
    char pad_04[0x24];
    float ground_y; /* +0x28 */
    char pad_2C[4];
    union {
        unsigned char flags_30;
        ClothBoneFlags30 flags_30_bits;
    }; /* +0x30 */
    char pad_31[3];
    struct ClothBone* target_bone; /* +0x34 */
    MkBone* bone; /* +0x38 */
    char pad_3C[0x14];
    Vec wind_normal; /* +0x50 */
    char pad_5C[4];
    Vec collision_offset; /* +0x60 */
    char pad_6C[4];
    ClothCollisionPoint collision_points[3]; /* +0x70, stride 0x10 */
    Vec target_vector; /* +0xA0 */
    char pad_AC[0x84];
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

/* mk_obj.o - NonMatching scaffold (krypt Wave 2). */

/*
 * Midway mksobj (partial) -- pebble/render/hide agree on atomic @ +0x14.
 * id in low 12 bits of id_flags; bit 0x80000000 used with priority 0x12.
 * frame @ +0x18 used by particle binds (RwFrameGetLTM).
 */
typedef struct MkSobj {
    MkHdr hdr;             /* +0x00 */
    union {
        unsigned char flags_08; /* +0x08 - transform update flags */
        MkSobjFlags08 flags_08_bits;
    };
    union {
        unsigned char flags09; /* +0x09 - bit7 / bit5 render (mab rlwimi) */
        MkSobjFlags09 flags09_bits;
    };
    char pad0A[2];
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
    unsigned char pad_high : 5;
    unsigned char cloth_update : 1; /* bit2 */
    unsigned char pad_low : 2;
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

typedef struct MkObjHideFlags {
    unsigned char still_move : 1; /* bit7 */
    unsigned char bit6 : 1;
    unsigned char hidden : 1;     /* bit5 */
    unsigned char pad_4_3 : 2;
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
            unsigned char flags_0B; /* +0x0B */
        };
    };
    union {
        unsigned char flags_0C;
        MkObjFlags0C flags_0C_bits;
    }; /* +0x0C */
    char pad0D[3];
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
    char pad60[4];
    float bone_angle_64;
    float bone_angle_68;
    void* ground_colls;     /* +0x6C */
    float ground_colls_y;   /* +0x70 */
    union {
        void* allocation_74;
        ClothBone* cloth_bones;
    }; /* +0x74 - owned cloth-bone allocation */
    union {
        char pad78[4];
        unsigned int cloth_bone_count;
    }; /* +0x78 */
    MkPtr* list_7C;         /* +0x7C - owned list */
    MkPtr* list_80;         /* +0x80 - owned list */
    MkPtr* sobj_list; /* +0x84 */
    MkPtr* list_88;   /* +0x88 - owned list */
    union {
        int flipped_bones;
        void* flipped_bone_map;
    }; /* +0x8C */
    Vec ground_restore_pos; /* +0x90 */
    char pad9C[4];
    union {
        Vec pos; /* +0xA0 */
        struct {
            float pos_x;
            float pos_y;
            float pos_z;
        };
    };
    char padAC[4];
    Vec pos_vel; /* +0xB0 */
    char padBC[4];
    Vec pivot; /* +0xC0 - alternate-position matrix offset */
    char padCC[4];
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
            char padDC[4];
            Vec ang_vel; /* +0xE0 */
            char padEC[4];
            Vec scale;   /* +0xF0 */
        };
        struct {
            Quat orientation_quat; /* +0xD0 */
            char padE0_quat[0x0C];
            Quat secondary_quat; /* +0xEC */
        };
    };
} MkObj;

/* Critical krypt APIs */
void* obj_find_sobj_by_id(void* obj, unsigned int id);
void sobj_set_priority(void* sobj, int priority);
void hide_sobj(void* sobj);
void unhide_sobj(void* sobj);

/* Frequently called siblings */
void* obj_create_sobjs_by_id(void* obj, int id);
void insert_fgnd_mkobj(void* obj);
void update_mkobj(void* obj);
void update_obj_pos(void* obj);
void obj_set_pos(MkObj* obj, const Vec* pos);
void obj_get_pos(MkObj* obj, Vec* out);
void obj_set_sobj_alpha(void* obj, int sobj_index, int alpha);
void obj_set_sobj_priority(void* obj, int id, int priority);
void hide_sobj_by_sobj_id(void* obj, unsigned int id);
void unhide_sobj_by_sobj_id(void* obj, unsigned int id);
void hide_obj(void* obj);
void unhide_obj(void* obj);
void* obj_first_sobj(void* obj);
RwTexture* material_get_texture_pointer(
    RpMaterial* material, int use_matfx);
void material_set_texture_pointer(
    RpMaterial* material, RwTexture* texture, int use_matfx);
void* get_mkx_mem(void* allocation);
void* get_mkobj(int type, void* clump);
void destroy_mkobj(void* obj);

#endif
