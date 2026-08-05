#ifndef GAME_CLOTH_H
#define GAME_CLOTH_H

#include "runtime/mk_obj.h"

typedef struct ClothCollisionPlane {
    MkHdr hdr;
    unsigned int bone_count; /* +0x08 */
    ClothBone* bones[12];    /* +0x0C */
    char pad3C[0x30];
    unsigned char flags_6C;
    char pad6D[3];
    float weights[4];       /* +0x70 */
    union {
        MkBone* reference_bone; /* +0x80 */
        MkBone* reference_bones[4]; /* +0x80..+0x8C */
    };
    Vec normal;             /* +0x90 */
    char pad9C[4];
    float distance;         /* +0xA0 */
    char padA4[0x0C];
} ClothCollisionPlane; /* 0xB0 */

typedef struct ClothCollisionVolume {
    MkHdr hdr;
    MkBone* reference_bone; /* +0x08 */
    float radius; /* +0x0C */
    float cylinder_bottom; /* +0x10 */
    float cylinder_top;    /* +0x14 */
    void (*collision_fn)(void); /* +0x18 */
    unsigned int bone_count; /* +0x1C */
    ClothBone* bones[12];    /* +0x20 */
    unsigned int bone_settings[13]; /* +0x50; slot zero is reserved */
} ClothCollisionVolume;

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
void mks_cloth_bones_init_by_tbl(int table_id, int flags);
int find_cloth_bone_id_from_tag(MkObj* obj, int tag);
void mks_debug_display_cloth_ontop(int enabled);
void mks_debug_display_cloth_coll_plane(void);
void mks_debug_display_cloth_coll_cyl(void);
void cloth_change_ground_plane_for(float ground_plane);
void vdestroy_cloth_coll_volume(MkHdr* hdr);
void vdestroy_cloth_coll_plane(MkHdr* hdr);
void vdestroy_cloth_coll(MkHdr* hdr);
void start_cloth_proc(void);

#endif
