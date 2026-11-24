#ifndef MK_OBJ_H
#define MK_OBJ_H

#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "math/gxVect.h"
#include "math/gxQuat.h"
#include "runtime/limb.h"
#include "runtime/mk_obj_bone.h"
#include "runtime/mk_obj_flags.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_obj_lists.h"
#include "game/weapon_types.h"

typedef struct  {
    unsigned char collision_disabled : 1;
    unsigned char collision_deferred : 1;
    unsigned char scale_controlled : 1;
    unsigned char pad_bit4 : 1;
    unsigned char pad_bit3 : 1;
    unsigned char reparent_toggle : 1;
    unsigned char preserve_rotation : 1;
    unsigned char pad_bit0 : 1;
} MkBoneFlags55;

typedef struct MkFlippedBoneMap {
    int count;
    int* bone_indices;
} MkFlippedBoneMap;

typedef struct MkBoneFlags54 {
    unsigned char transform_parented : 1;
    unsigned char tag_1000 : 1;
    unsigned char cloth_candidate : 1;
    unsigned char calculation_locked : 1;
    unsigned char field_bit3 : 1;
    unsigned char hierarchy_driven : 1;
    unsigned char pose_matrix_applied : 1;
    unsigned char has_children : 1;
} MkBoneFlags54;

typedef struct MkBone {
    struct RwMatrix matrix;
    struct RwMatrix* parent_matrix;
    struct RwMatrix* original_parent_matrix;
    int tag;
    int limb_id;
    int bone_index;
    union {
        struct {
            MkBoneFlags54 flags_54_bits;
            MkBoneFlags55 flags_55_bits;
            char pad56[2];
        };
        unsigned int flags_word_54;
    };
    struct ClothBone* cloth_link;
    float inverse_length;
    float field_60;
    float field_64;
    unsigned int update_tick;
    struct MkBone* transform_parent;
    struct MkBone* tree_next;
    struct MkBone* tree_child;
    struct MkBone* root_next;
    struct MkBone* clone_source;
    MkPtr* list_80;
    char pad84[0x0C];
    Quat rotation_90;
    RwMatrixPosition translation;
    Vec scale;
    char padBC[4];
    RwMatrixPosition delta;
    union {
        struct {
            union {
                struct {
                    Quat rotation;
                    Quat rotation_e0;
                };
                Quat rotations[2];
            };
            RwMatrixPosition velocity;
            Vec bind_offset;
            char pad10C[4];
        };
        RwMatrix trail_matrix;
    };
} MkBone;

typedef char MkBoneSizeCheck[sizeof(MkBone) == 0x110 ? 1 : -1];
typedef char MkBoneTrailMatrixSizeCheck[
    sizeof(((MkBone*)0)->trail_matrix) == 0x40 ? 1 : -1];

typedef struct ClothBoneFlags30 {
    unsigned char use_ground_y : 1;
    unsigned char pad : 7;
} ClothBoneFlags30;

typedef struct ClothCollisionPoint {
    Vec position;
    char pad_0C[4];
} ClothCollisionPoint;

typedef struct ClothBone {
    int collision_point_count;
    float table_scale;
    float stiffness_squared;
    float segment_length;
    float damping_factor;
    float initial_x;
    float initial_z;
    float table_weight;
    float force_step;
    float stretch_weight;
    float ground_y;
    int active;
    union {
        unsigned int flags_30;
        ClothBoneFlags30 flags_30_bits;
    };
    struct ClothBone* target_bone;
    MkBone* bone;
    float rest_length;
    float collision_amount;
    float current_length;
    char pad_48[8];
    Vec wind_normal;
    char pad_5C[4];
    Vec collision_offset;
    char pad_6C[4];
    union {
        ClothCollisionPoint collision_points[3];
        struct {
            char pad_local_70[0x20];
            Vec local_cloth_position;
            char pad_local_9C[4];
        };
    };
    Vec target_vector;
    char pad_AC[4];
    Vec world_target;
    char pad_BC[4];
    union {
        struct {
            Vec force_position;
            char pad_CC[4];
        };
        RwMatrixPosition force_position_row;
    };
    Vec velocity;
    char pad_DC[4];
    Quat collision_rotation;
    union {
        struct {
            Vec collision_local_position;
            char pad_FC[4];
        };
        RwMatrixPosition collision_local_position_row;
    };
    union {
        struct {
            Vec previous_collision_local_position;
            char pad_10C[4];
        };
        RwMatrixPosition previous_collision_local_position_row;
    };
    char pad_110[0x20];
} ClothBone;

typedef struct MkxMem {
    MkHdr hdr;
    void* allocation;
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
    unsigned char angular_velocity_enabled : 1;
    unsigned char scale_dirty : 1;
    unsigned char bit0 : 1;
} MkSobjFlags08;

typedef struct MkSobj {
    MkHdr hdr;
    union {
        unsigned int flags_word_08;
        struct {
            union {
                unsigned char flags_08;
                MkSobjFlags08 flags_08_bits;
            };
            union {
                unsigned char flags09;
                MkSobjFlags09 flags09_bits;
            };
            char pad0A[2];
        };
    };
    unsigned int id_flags;
    int priority;
    RpAtomic* atomic;
    RwFrame* frame;
    struct MkObj* owner;
    MkHdr* bound_hdr;
    unsigned int bound_instance;
    float z_offset;
    unsigned int render_flags;
    Vec pos;
    char pad3C[4];
    Vec pos_vel;
    char pad4C[4];
    Vec ang;
    char pad5C[4];
    Vec ang_vel;
    char pad6C[4];
    Vec scale;
    char pad7C[4];
    struct RwMatrix* matrices;
} MkSobj;

typedef struct MksobjPluginData {
    unsigned int flags;
    float field_04;
    MkSobj* sobj;
    int field_0C;
} MksobjPluginData;

typedef struct MkObjFlags0C {
    unsigned int bit0C_7 : 1;
    unsigned int tag_flag_40 : 1;
    unsigned int tag_flag_20 : 1;
    unsigned int tag_flag_10 : 1;
    unsigned int tag_flag_08 : 1;
    unsigned int cloth_update : 1;
    unsigned int parented : 1;
    unsigned int bit0C_0 : 1;
} MkObjFlags0C;

typedef struct MkObj {
    MkHdr hdr;
    union {
        unsigned int flags_word_08;
        MkObjFlags flags;
        struct {
            unsigned char flags_08;
            char pad09;
            unsigned char hide_flags;
            char pad0B;
        };
    };
    union {
        unsigned int flags_word_0C;
        MkObjFlags0C flags_0C_bits;
    };
    int oid;
    int clump_count;
    union {
        RpClump* clump;
        RpClump* clumps[2];
    };
    RwFrame* frame;
    struct RwMatrix* field_24;
    MkPtr* child_list;
    unsigned int light_flags;
    float gravity;
    float field_34;
    int ground_bone;
    MkHdr* parent_hdr;
    unsigned int parent_inst;
    MkPtr* list_44;
    MkBone** bones;
    unsigned int bone_count;
    int fallback_bone_index;
    int* matrix_indices;
    unsigned int matrix_count;
    union {
        void* field_5C;
        MkObjItemAttachData* item_attach_data;
        struct WeaponDefinition* weapon_definition;
    };
    int field_60;
    float bone_angle_64;
    float bone_angle_68;
    void* ground_colls;
    float ground_colls_y;
    ClothBone* cloth_bones;
    unsigned int cloth_bone_count;
    MkPtr* list_7C;
    MkPtr* list_80;
    MkPtr* sobj_list;
    MkPtr* list_88;
    MkFlippedBoneMap* flipped_bone_map;
    Vec ground_restore_pos;
    char pad9C[4];
    RwMatrixPosition pos;
    union {
        struct {
            Vec pos_vel;
            unsigned int pos_vel_pad;
        };
        RwMatrixPosition pos_vel_row;
    };
    Vec pivot;
    char padCC[4];
    union {
        struct {
            union {
                struct {
                    union {
                        Vec ang;
                        struct {
                            float dir_x;
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
                    Vec ang_vel;
                    unsigned int ang_vel_pad;
                };
                RwMatrixPosition ang_vel_row;
            };
            union {
                struct {
                    Vec scale;
                    unsigned int scale_pad;
                };
                RwMatrixPosition scale_row;
            };
        };
        struct {
            Quat orientation_quat;
            char padE0_quat[0x0C];
            Quat secondary_quat;
        };
    };
} MkObj;

#ifndef __cplusplus
typedef char MkFlippedBoneMapSizeCheck[
    sizeof(MkFlippedBoneMap) == 0x08 ? 1 : -1];
typedef char MkBoneSizeCheck[sizeof(MkBone) == 0x110 ? 1 : -1];
typedef char ClothCollisionPointSizeCheck[
    sizeof(ClothCollisionPoint) == 0x10 ? 1 : -1];
typedef char ClothBoneSizeCheck[sizeof(ClothBone) == 0x130 ? 1 : -1];
typedef char MkxMemSizeCheck[sizeof(MkxMem) == 0x0C ? 1 : -1];
typedef char MkSobjSizeCheck[sizeof(MkSobj) == 0x84 ? 1 : -1];
typedef char MksobjPluginDataSizeCheck[
    sizeof(MksobjPluginData) == 0x10 ? 1 : -1];
typedef char MkObjItemAttachDataSizeCheck[
    sizeof(MkObjItemAttachData) == 0x70 ? 1 : -1];
typedef char MkObjSizeCheck[sizeof(MkObj) == 0x100 ? 1 : -1];
#endif

#ifdef __cplusplus
extern "C" {
#endif

MkSobj* obj_find_sobj_by_id(MkObj* obj, unsigned int id);
RpMaterial* sobj_find_material_by_id(MkSobj* sobj,
                                     unsigned int material_id);
void sobj_set_priority(void* sobj, int priority);
void hide_sobj(void* sobj);
void unhide_sobj(void* sobj);
void hide_sobj_and_children(MkSobj* sobj);
void unhide_sobj_and_children(MkSobj* sobj);

MkSobj* obj_create_sobjs_by_id(MkObj* obj, int id);
MkSobj* obj_find_child_sobj_by_id(MkObj* obj, unsigned int id, int depth);
int sobj_does_atomic_have_children(MkSobj* sobj);
void set_true_clip_flag_on_sobj_and_children(MkSobj* sobj, int flag);
void insert_fgnd_mkobj(void* obj);
void update_mkobj(void* obj);
void vdestroy_mksobj(MkSobj* sobj);
void update_mksobj(MkSobj* sobj);
void vdestroy_mkx_mem(MkxMem* mem);
void vdestroy_mkobj(MkObj* obj);
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
void sobj_disable_blending(void* sobj);
void obj_create_sobjs(MkObj* obj);
RpMaterial* obj_find_material_with_texture(MkObj* obj,
                                           const char* texture_name);
void sobj_set_transl_flag(MkSobj* sobj);
Vec* sobj_get_world_pos(MkSobj* sobj);
RpAtomic* obj_get_1st_atomic(MkObj* obj);
void insert_bone_hierarchy_mkobj(MkObj* obj);
void atomic_set_transl_flag(RpAtomic* atomic);
void obj_set_rw_lights(MkObj* obj);
void force_rw_lights(void);
struct PlyrPdata;
struct MkProc* create_mkproc_headtracking(int pid, MkObj* object, struct PlyrPdata* target);
void mirror_guy(MkObj* source, MkObj* mirror, struct PlyrPdata* pdata);
void create_shadow_proc(int pid, struct PlyrPdata* controller, MkObj* source,
                        MkObj* shadow);
void limb_sever_hide_z_meat_chunks_all(MkObj* obj);
void obj_set_all_sobjs_priority(MkObj* obj, int priority);
void obj_set_pos_vel(MkObj* obj, void* velocity);
void render_fgnd_mkobjs(void);
extern unsigned int uploaded_light_state;
RwTexture* material_get_texture_pointer(
    RpMaterial* material, int use_matfx);
void material_set_texture_pointer(
    RpMaterial* material, RwTexture* texture, int use_matfx);
void* get_mkx_mem(void* allocation);
MkObj* get_mkobj(int type, RpClump* clump);
void destroy_clump(void* clump);
MkObj* get_mkobj_frame(int type, RwFrame* frame);
void destroy_mkobj(void* obj);

extern MkPtr* fgnd_mkobj_list;
extern MkPtr* board_piece_light_list;
extern MkPtr* clone_light_list;
extern MkPtr* freeze_light_list;
extern MkPtr* gore2_light_list;
extern MkPtr* special_light_list;
extern MkPtr* weapon_trail_light_list;

#ifdef __cplusplus
}
#endif

void start_obj_proc(void);
void start_bone_hierarchy_proc(void);

typedef struct ScaleScriptEntry {
    unsigned int flags;
    Vec scale;
    float duration;
} ScaleScriptEntry;

typedef struct ScalePdata {
    MkHdr hdr;
    MkObj* obj;
    unsigned int obj_instance;
    ScaleScriptEntry* script_start;
    ScaleScriptEntry* script;
    Vec prior_scale;
    float elapsed;
} ScalePdata;

extern void insert_ground_me_mkobj(void* obj);
void remove_fgnd_mkobj(void* obj);
RpMaterial* obj_find_material_by_id(MkObj* obj, int material_id);
void obj_set_gravity(MkObj* object, float gravity);
void pull_bone_hierarchy_mkobj(void* obj);
void material_set_zbias(void* material, float zbias);
void hide_atomic(void* atomic);
void obj_set_ang_vel(MkObj* object, void* velocity);
void* get_bone_with_tag(void* object, int tag);
void limb_sever_show_z_meat_chunks_all(MkObj* object);
void unhide_atomic(void* atomic);
void init_weapon_trail_light_list(void);
void mkobj_zero_bone_rots(MkObj* object);
void* mkobj_get_matrix(void* obj);
extern MkPtr* plyr_light_list;
void bind_rplight_to_obj(RpLight* light, MkObj* obj);
extern MkPtr* point_light_list;
extern MkPtr* skinned_obj_light_list;
extern MkPtr* bgnd_spec_light_list;
void obj_set_light_flag(void* object, int flag);
void obj_set_flipped_bones(MkObj* obj, MkFlippedBoneMap* bone_map);
void obj_set_ground_colls(void* object, void* value);
void mkobj_bones_dest_mat_no_update(MkObj* object);
void sobj_set_ang(MkSobj* sobj, void* value);
void kill_head_tracking(void);
void sobj_get_pos_vel(MkSobj* sobj, Vec* out);
void get_bone_relative_pos(MkObj* obj, int bone, Vec* out);
void obj_clear_bone_collapse_flag(MkObj* object, int bone);
void obj_force_culling_off(MkObj* object);
void mks_set_flipped_bones(MkFlippedBoneMap* bone_map);
void material_set_reflection_texture(void* material, void* texture);
void obj_set_ang(void* object, void* value);
void obj_set_ground_colls_y(void* object, float value);
void obj_get_ang(void* out, void* object);
void sobj_set_pos(MkSobj* sobj, void* value);
void sobj_set_ang_vel(MkSobj* sobj, void* value);
void obj_enable_grounding(MkObj* obj);
void sobj_no_zwrite(MkSobj* sobj);
void sobj_get_pos(MkSobj* sobj, Vec* out);
void obj_turn_gravity_off(MkObj* obj);
Quat* obj_get_bone_rot_quat(void* obj, int bone);
void obj_set_sobj_pos(MkObj* obj, unsigned int id, Vec* pos);
void limb_sever_reset_limbs(PlyrInfo* player);
void sobj_get_ang_vel(MkSobj* sobj, Vec* out);
void* start_scale_proc(void* obj, void* script);
void obj_set_scale(MkObj*, void*);
void limb_sever_show_z_meat_chunks(MkObj* obj, int limb, int show_all);
void obj_match_pos_ang_to_src_obj(MkObj* destination, MkObj* source);
void limb_sever_show_z_meat_chunks_all_plyr_num(int a);
void sobj_get_ang(MkSobj* sobj, Vec* out);
void obj_get_pos_vel(MkObj* obj, Vec* out);
void sobj_set_pos_vel(MkSobj* sobj, void* value);
void obj_get_scale(MkObj* obj, Vec* out);
void mks_start_goro_arms_fixup(void);
void delete_light_lists(void);
void mkobj_get_matrix_pos(void* out, void* object);
void obj_get_ang_vel(MkObj* obj, Vec* out);
void sobj_set_alpha(void* sobj, int alpha);
int is_sobj_hidden(void* sobj);
#endif
