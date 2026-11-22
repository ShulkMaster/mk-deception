#ifndef GAME_CLOTH_H
#define GAME_CLOTH_H

#include "runtime/mk_obj.h"
#include "game/cloth_wind.h"

typedef struct ClothInitEntry ClothInitEntry;

struct ClothInitEntry {
    int bone_tag;
    float stiffness;
    float segment_length;
    float force;
    float field_10;
    float damping;
    float initial_x;
    float initial_z;
    float table_scale;
    float field_24;
};
typedef char ClothInitEntrySizeCheck[sizeof(ClothInitEntry) == 0x28 ? 1 : -1];

extern int shadow_bones[37];
extern MkPtr* weapon_trail_mkobj_list;

void start_cloth_bones(MkObj* object);
void cloth_bones_init_by_tbl(
    MkObj* object, ClothInitEntry* table, int count);

typedef struct ClothCollisionPlane {
    MkHdr hdr;
    unsigned int bone_count;
    ClothBone* bones[12];
    char pad3C[0x30];
    union {
        unsigned int flags_storage;
        struct {
            unsigned char flags_80 : 1;
            unsigned char flags_40 : 1;
            unsigned char flags_rest : 6;
        } flags_6C_bits;
    };
    float weights[4];
    union {
        MkBone* reference_bone;
        MkBone* reference_bones[4];
    };
    Vec normal;
    char pad9C[4];
    float distance;
    char padA4[0x0C];
} ClothCollisionPlane;

typedef struct ClothCollisionVolume {
    MkHdr hdr;
    MkBone* reference_bone;
    float radius;
    float cylinder_bottom;
    float cylinder_top;
    void (*collision_fn)(void);
    unsigned int bone_count;
    ClothBone* bones[12];
    unsigned int bone_settings[13];
    unsigned char reserved_84[0xBC];
} ClothCollisionVolume;
typedef char ClothCollisionVolumeSizeCheck[sizeof(ClothCollisionVolume) == 0x140 ? 1 : -1];

void mks_bgnd_obj_enable_cloth_update(int model_index, int enabled);
void mks_npc_disable_ground_y_all_cloth_bones(int model_index);
void mks_npc_set_ground_y_all_cloth_bones(int model_index, float ground_y);
void mks_npc_cb1_eq_cloth_bone(int model_index, int bone_id);
void mks_npc_cloth_bones_init_by_tbl(
    int model_index,
    int table_id,
    int flags);
void mks_npc_start_cloth_bones(int model_index);
void obj_translate_cloth(MkObj* object, const Vec* translation);
void mks_ccp1_insert_cb1(void);
void mks_cc1_insert_cb1(void);
void mks_set_cb1_target_bone_cb2(void);
void mks_cb1_set_coll_offset(float x, float y, float z);
void mks_cb1_set_coll_offset_xz(float x, float z);
void mks_cb1_add_coll_pt(float x, float y, float z);
void mks_cb1_set_ground_y(float ground_y);
void mks_set_cb1_wind_normal(float x, float y, float z);
void mks_set_ground_y_all_cloth_bones(float ground_y);
void mks_cb2_eq_cloth_bone(int bone_id);
void mks_cb1_eq_cloth_bone(int bone_id);
void mks_mat_id_set_zbias(int material_id, float zbias);
void mks_cloth_bones_init_by_tbl(ClothInitEntry* table, int flags);
int find_cloth_bone_id_from_tag(MkObj* obj, int tag);
void mks_debug_display_cloth_ontop(int enabled);
void mks_debug_display_cloth_coll_plane(float arg0);
void mks_debug_display_cloth_coll_cyl(int arg0, int arg1, int arg2);
void cloth_change_ground_plane_for(float ground_plane);
void vdestroy_cloth_coll_volume(MkHdr* hdr);
void vdestroy_cloth_coll_plane(MkHdr* hdr);
void vdestroy_cloth_coll(MkHdr* hdr);
void start_cloth_proc(void);

void mks_ccp1_eq_insert_cloth_coll_plane(int, float, float, float, float);
void mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside(void);
void mks_cc1_expand_cyl(float top, float bottom);
void mks_npc_set_target(int model_index, int source_bone_id, int target_bone_id);
void mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl(void);
void mks_insert_cloth_force_bones(float, float);
void mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(int, float, int, float, int, float, int, float);
void mks_npc_cc1_eq_insert_cloth_coll( int model_index, int bone_index, float radius);
void mks_cb1_set_scale( int include_children, float scale_x, float scale_y, float scale_z);
void mks_start_axis_indicator_p_axis_track_bone_world_mat( int bone_tag, float scale);
void mks_obj_enable_update_cloth(MkObj* obj, int enabled);
void mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs(void);
void mks_cc1_eq_insert_cloth_coll(int bone_index, float radius);
#endif
