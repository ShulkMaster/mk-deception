#include "game/cloth.h"
#include "game/game_info.h"
#include "math/mk_math.h"
#include "math/gxMath.h"
#include "math/gxMat.h"
#include "math/gxQuat.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"

/*
 * Soft ceiling: retail gives several automatic Vec/Quat temporaries 16-byte
 * stack alignment for paired-single scheduling. Portable C has no automatic
 * object-alignment syntax, so compiler-specific alignment attributes are
 * intentionally avoided; the remaining residue is stack layout, register
 * allocation, and scheduling rather than cloth behavior or data layout.
 */

static float p_cloth(void);
static float p_wind(void);
static float p_wind_lp(void);
typedef struct ClothInitEntry ClothInitEntry;
void cloth_bones_init_by_tbl(
    MkObj* obj, ClothInitEntry* table, int count);
void start_cloth_bones(MkObj* obj);
float p_axis_track_bone_world_mat(void);
struct ClothForcePdata;
static void do_cloth_force(struct ClothForcePdata* force);
static void do_cloth_colls(MkHdr* collision);
static void calc_cloth_dwp(ClothBone* bone);
static void calc_cloth_stretch(ClothBone* bone);
static void set_cloth_pos(ClothBone* bone);
static void pw_axis(void);
static void ps_axis(void);
static void mkobj_update_cloth(MkHdr* hdr);
void mkobj_update_weapon_trail(MkHdr* hdr);
RpMaterial* obj_find_material_by_id(MkObj* obj, int material_id);
void material_set_zbias(RpMaterial* material, float zbias);

extern MkObj* g_bgnd_preloaded_models[15];
extern MkObj* plyr_obj;
typedef struct ClothCollisionScratch {
    Vec displacement;
    float pad_0C;
    Vec cross;
    float pad_1C;
    Vec bone_to_point;
    float pad_2C;
    Vec bone_axis;
    float pad_3C;
} ClothCollisionScratch;
Vec base_wind_v3;
Vec wind_v3;
RwMatrix cloth_obj_mat_inv;
ClothCollisionScratch pt_displacement_v;
RwMatrix cloth_coll_bone_parent_world_mat_inv;
#define cloth_displacement_v (pt_displacement_v.displacement)
#define v_cc_cross (pt_displacement_v.cross)
#define v_ccb_to_coll_pt (pt_displacement_v.bone_to_point)
#define cloth_coll_bone_uv (pt_displacement_v.bone_axis)
extern float game_speed;
extern float sqrt_game_speed;
extern int exec_tick_ctr;

typedef struct ClothForcePdata {
    MkHdr hdr;
    ClothBone* first;
    ClothBone* second;
    float rest_length;
    float stiffness;
    char pad18[0x38];
} ClothForcePdata;

typedef struct AxisPdata {
    MkHdr hdr;
    MkObj* axis;
    unsigned int axis_instance;
    MkObj* target;
    int state;
    int bone_index;
} AxisPdata;

typedef struct AxisTargetLatch {
    MkObj* object;
    unsigned int instance;
} AxisTargetLatch;

int hide_axis;
MkPtr* cloth_mkobj_list;
MkPtr* weapon_trail_mkobj_list;
MkObj* cloth_obj;
RwMatrix* cloth_obj_mat;
ClothCollisionVolume* cloth_coll;
MkBone* cloth_coll_bone;
float cloth_coll_radius_sq;
int* coll_cnt;
float cloth_ground_plane;
AxisTargetLatch target_obj_item;
ClothBone* cloth_bone;
ClothCollisionVolume* mks_cc1;
ClothCollisionPlane* mks_ccp1;
ClothBone* mks_cb1;
ClothBone* mks_cb2;
MkSobj* axis_sobj;
MkObj* axis_obj;
AxisPdata* axis_pdata;

int shadow_bones[37] = {
    0x1000, 0x1001, 0x1002, 0x1003, 0x1004, 0x1005, 0x1006,
    0x1007, 0x1008, 0x1009, 0x100C, 0x100D, 0x100E, 0x100F,
    0x1010, 0x1011, 0x1012, 0x1013, 0x1014, 0x1015, 0x1016,
    0x1017, 0x1018, 0x1019, 0x000B, 0x000C, 0x000D, 0x000E,
    0x000F, 0x0010, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C,
    0x001D, 0,
};
/*
 * Soft ceiling: retail keeps a zero word at .data+0x94. MWCC moves an
 * ordinary zero-initialized C object to BSS, so the split-assembly label is
 * retained instead of forcing section placement with a compiler extension.
 */
extern int gap_05_8033EDA4_data;

static void cloth_coll_vector_cyl(void);
static void cloth_coll_point_cyl_abs(void);
static void cloth_coll_point_cyl_inside(void);
static void cloth_coll_point_cyl_rel(void);
int obj_get_bid_for_tid(MkObj* obj, int tag);
MkObj* load_model_from_slot(int slot, int model_id, int heap_id);
void* memcpy(void* destination, const void* source, unsigned int size);
void RwMatrixUpdate(RwMatrix* matrix);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwMatrix* RwMatrixInvert(RwMatrix* output, const RwMatrix* input);
void PSVECAdd(const Vec* first, const Vec* second, Vec* output);

typedef struct ClothWindPdata {
    MkHdr hdr;
    float amplitude; /* +0x08 */
    float acceleration; /* +0x0C */
    float random_scale; /* +0x10 */
    float offset_x; /* +0x14 */
    float offset_z; /* +0x18 */
    float step_x; /* +0x1C */
    float step_z; /* +0x20 */
    int ticks; /* +0x24 */
} ClothWindPdata;

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
    int field_24;
};

/*
 * Soft ceiling: MWCC inlines this approximation at all three callers but
 * also emits an unused local copy. Declaring it inline or expanding it as a
 * macro crashes mwcceppc 2.7 at -O4,s, so the clean typed function is kept.
 */
static float cloth_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float refined;

    if (!(value > 0.0f)) {
        return 0.0f;
    }
    input.f = value;
    guess.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    guess.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) &
        0x7F800000U;
    refined = guess.f * (3.0f - (guess.f * guess.f) / value);
    return 0.5f * refined;
}

typedef struct ClothBoneScaleLink {
    char pad00[0x3C];
    float magnitude;
    char pad40[0x50];
    Vec scaled_axis;
    char pad9C[4];
    Vec normalized_axis;
} ClothBoneScaleLink;

typedef float (*ClothJumpSleepFn)(
    MkProcEntryFn entry,
    MkVtableMkproc* vtable,
    float sleep_ticks);

typedef struct ClothProcVtable {
    void* slots[9];
    ClothJumpSleepFn jump_sleep;
} ClothProcVtable;

/*
 * Soft ceilings: find_cloth_bone_id_from_tag ~99.77%, cb1/cb2 selectors
 * ~99.72% -- only beq vs equivalent ble on an unsigned zero-count check.
 */
static inline ClothBone* find_cloth_bone_by_tag(MkObj* obj, int bone_id) {
    ClothBone* bone;
    unsigned int remaining;

    bone_id &= 0xFFF;
    remaining = obj->cloth_bone_count;
    bone = obj->cloth_bones;
    while (remaining != 0) {
        if ((bone->bone->tag & 0xFFF) == bone_id) {
            return bone;
        }
        bone++;
        remaining--;
    }
    return 0;
}

void mks_debug_display_cloth_ontop(int enabled) {
    (void)enabled;
}

void mks_debug_display_cloth_coll_plane(void) {
}

void mks_debug_display_cloth_coll_cyl(void) {
}

void mks_obj_enable_update_cloth(MkObj* obj, int enabled) {
    obj->flags_0C_bits.cloth_update = enabled;
}

void mks_bgnd_obj_enable_cloth_update(int model_index, int enabled) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj == 0) {
        return;
    }
    obj->flags_0C_bits.cloth_update = enabled;
}

void mks_npc_disable_ground_y_all_cloth_bones(int model_index) {
    MkObj* obj;
    ClothBone* bone;
    unsigned int i;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj == 0) {
        return;
    }
    bone = obj->cloth_bones;
    for (i = 0;
         i < g_bgnd_preloaded_models[model_index]->cloth_bone_count;
         i++) {
        bone->flags_30_bits.use_ground_y = 0;
        bone->ground_y = 0.0f;
        bone++;
    }
}

void mks_npc_set_ground_y_all_cloth_bones(
    int model_index,
    float ground_y) {
    MkObj* obj;
    ClothBone* bone;
    unsigned int i;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj == 0) {
        return;
    }
    bone = obj->cloth_bones;
    for (i = 0;
         i < g_bgnd_preloaded_models[model_index]->cloth_bone_count;
         i++) {
        if (bone != 0) {
            bone->flags_30_bits.use_ground_y = 1;
            bone->ground_y = ground_y;
        }
        bone++;
    }
}

void mks_npc_cb1_eq_cloth_bone(int model_index, int bone_id) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj == 0) {
        return;
    }
    mks_cb1 = find_cloth_bone_by_tag(obj, bone_id);
}

void mks_npc_cc1_eq_insert_cloth_coll(
    int model_index, int bone_index, float radius) {
    ClothCollisionVolume* volume;
    MkObj* object;
    MkBone* bone;

    object = g_bgnd_preloaded_models[model_index];
    if (object != 0) {
        bone = object->bones[bone_index];
        volume = (ClothCollisionVolume*)get_mkhdr(
            &vtbl_cloth_coll, sizeof(*volume));
        if (volume != 0) {
            volume->reference_bone = bone;
            volume->radius = radius;
            volume->bone_count = 0;
            volume->collision_fn = cloth_coll_point_cyl_rel;
            volume->cylinder_bottom = -radius;
            volume->cylinder_top = radius + 1.0f / bone->field_5C;
            bone->flags_54_bits.calculation_locked = 1;
            if (bone->transform_parent != 0) {
                bone->transform_parent->flags_54_bits.calculation_locked = 1;
            }
        }
        if (volume != 0) {
            mk_insert((MkHdr*)volume, &object->list_80);
        }
        mks_cc1 = volume;
    }
}

void mks_npc_set_target(
    int model_index, int source_bone_id, int target_bone_id) {
    MkObj* object;
    ClothBone* source;
    ClothBone* target;

    object = g_bgnd_preloaded_models[model_index];
    if (object == 0) {
        return;
    }
    source = find_cloth_bone_by_tag(object, source_bone_id);
    if (source == 0) {
        return;
    }
    target = find_cloth_bone_by_tag(object, target_bone_id);
    if (target == 0) {
        return;
    }

    source->target_bone = target;
    gxVectUVV3ToV3(
        &source->target_vector,
        &source->bone->parent_matrix->pos_vec,
        &target->bone->parent_matrix->pos_vec);
}

void mks_npc_cloth_bones_init_by_tbl(
    int model_index,
    int table_id,
    int flags) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj != 0) {
        cloth_bones_init_by_tbl(
            obj, (ClothInitEntry*)table_id, flags);
    }
}

void mks_npc_start_cloth_bones(int model_index) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj != 0) {
        start_cloth_bones(obj);
    }
}

void mks_start_axis_indicator_p_axis_track_bone_world_mat(
    int bone_tag,
    float scale) {
    AxisPdata* pdata;
    MkProc* proc;
    MkObj* target;
    MkObj* axis;
    int bone_index;

    bone_index = obj_get_bid_for_tid(plyr_obj, bone_tag);
    if (bone_index < 0) {
        return;
    }

    target = plyr_obj;
    axis = load_model_from_slot(0, 0x1000C, 0x5001);
    if (axis == 0) {
        return;
    }

    axis->pos_z = 0.0f;
    axis->pos_y = 0.025f;
    axis->flags_08 |= 0x80;
    axis->hide_flags |= 0x20;
    axis->light_flags = 4;
    insert_fgnd_mkobj(axis);

    proc = _create_mkproc_generic_nostack(
        0x5004, 0x17, p_axis_track_bone_world_mat, sizeof(AxisPdata),
        (MkHdr**)&pdata);
    if (pdata != 0) {
        proc->pre_destroy = pw_axis;
        proc->destroy_cb = ps_axis;
        pdata->axis = axis;
        pdata->axis_instance = axis->hdr.instance;
        pdata->target = target;
        if (target != 0) {
            target_obj_item.object = target;
            target_obj_item.instance = target->hdr.instance;
        }
        pdata->state = 0;
        pdata->bone_index = bone_index;
    }

    if (scale == 1.0f) {
        axis->flags_08 &= ~2;
    } else {
        axis->flags_08 |= 2;
        axis->scale.x = scale;
        axis->scale.y = scale;
        axis->scale.z = scale;
    }
}

void mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(
    int bone_index_0, int bone_index_1,
    int bone_index_2, int bone_index_3,
    float weight_0, float weight_1, float weight_2, float weight_3) {
    ClothCollisionPlane* plane;
    MkObj* object;

    object = plyr_obj;
    plane = (ClothCollisionPlane*)get_mkhdr(
        &vtbl_cloth_coll_plane, sizeof(*plane));
    if (plane != 0) {
        plane->bone_count = 0;
        plane->reference_bone = 0;
        plane->flags_6C |= 0x80;
        plane->flags_6C |= 0x40;
        plane->reference_bones[0] = object->bones[bone_index_0];
        plane->reference_bones[1] = object->bones[bone_index_1];
        plane->reference_bones[2] = object->bones[bone_index_2];
        plane->reference_bones[3] = object->bones[bone_index_3];
        plane->reference_bones[0]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[1]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[2]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[3]->flags_54_bits.calculation_locked = 1;
        plane->weights[0] = weight_0;
        plane->weights[1] = weight_1;
        plane->weights[2] = weight_2;
        plane->weights[3] = weight_3;
        mk_insert((MkHdr*)plane, &object->list_80);
    }
    mks_ccp1 = plane;
}

void mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl(void) {
    if (mks_cc1 != 0) {
        mks_cc1->collision_fn = cloth_coll_vector_cyl;
    }
}

void mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs(void) {
    if (mks_cc1 != 0) {
        mks_cc1->collision_fn = cloth_coll_point_cyl_abs;
    }
}

void mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside(void) {
    if (mks_cc1 != 0) {
        mks_cc1->collision_fn = cloth_coll_point_cyl_inside;
    }
}

/*
 * Soft ceiling: ~97% -- exact control flow and size; three local values rotate
 * between r4-r6.
 */
void mks_ccp1_insert_cb1(void) {
    ClothCollisionPlane* plane;
    ClothBone* bone;
    unsigned int index;

    plane = mks_ccp1;
    bone = mks_cb1;
    if (plane == 0) {
        return;
    }
    if (bone == 0) {
        return;
    }
    index = plane->bone_count;
    if (index >= 12) {
        return;
    }
    plane->bone_count = index + 1;
    plane->bones[index] = bone;
}

void mks_cc1_expand_cyl(float top, float bottom) {
    if (mks_cc1 != 0) {
        mks_cc1->cylinder_bottom -= bottom;
        mks_cc1->cylinder_top += top;
    }
}

void mks_cc1_insert_cb1(void) {
    ClothCollisionVolume* volume;
    ClothBone* bone;
    unsigned int index;

    volume = mks_cc1;
    bone = mks_cb1;
    if (volume == 0) {
        return;
    }
    if (bone == 0) {
        return;
    }
    index = volume->bone_count;
    if (index >= 12) {
        return;
    }
    volume->bone_count = index + 1;
    volume->bones[index] = bone;
    volume->bone_settings[volume->bone_count] = 0;
}

void mks_ccp1_eq_insert_cloth_coll_plane(
    int bone_index, float distance,
    float normal_x, float normal_y, float normal_z) {
    ClothCollisionPlane* plane;
    MkObj* object;

    object = plyr_obj;
    plane = (ClothCollisionPlane*)get_mkhdr(
        &vtbl_cloth_coll_plane, sizeof(*plane));
    if (plane != 0) {
        plane->reference_bone = object->bones[bone_index];
        plane->reference_bone->flags_54_bits.calculation_locked = 1;
        plane->bone_count = 0;
        plane->flags_6C = 0;
        plane->distance = distance;
        plane->normal.x = normal_x;
        plane->normal.y = normal_y;
        plane->normal.z = normal_z;
        PSVECNormalize(&plane->normal, &plane->normal);
        mk_insert((MkHdr*)plane, &object->list_80);
    }
    mks_ccp1 = plane;
}

void mks_cc1_eq_insert_cloth_coll(int bone_index, float radius) {
    ClothCollisionVolume* volume;
    MkObj* object;
    MkBone* bone;

    object = plyr_obj;
    bone = object->bones[bone_index];
    volume = (ClothCollisionVolume*)get_mkhdr(
        &vtbl_cloth_coll, sizeof(*volume));
    if (volume != 0) {
        volume->reference_bone = bone;
        volume->radius = radius;
        volume->bone_count = 0;
        volume->collision_fn = cloth_coll_point_cyl_rel;
        volume->cylinder_bottom = -radius;
        volume->cylinder_top = radius + 1.0f / bone->field_5C;
        bone->flags_54_bits.calculation_locked = 1;
        if (bone->transform_parent != 0) {
            bone->transform_parent->flags_54_bits.calculation_locked = 1;
        }
    }
    if (volume != 0) {
        mk_insert((MkHdr*)volume, &object->list_80);
    }
    mks_cc1 = volume;
}

void mks_cb1_set_scale(
    int include_children, float scale_x, float scale_y, float scale_z) {
    MkBone* bone;
    ClothBoneScaleLink* link;
    float inverse_length;

    if (mks_cb1 != 0) {
        bone = mks_cb1->bone;
        while (bone != 0) {
            bone->flags_55_bits.scale_controlled = 1;
            bone->scale.x = scale_x;
            bone->scale.y = scale_y;
            bone->scale.z = scale_z;
            link = (ClothBoneScaleLink*)bone->cloth_link;
            if (link != 0) {
                link->scaled_axis.x *= scale_x;
                link->scaled_axis.y *= scale_y;
                link->scaled_axis.z *= scale_z;
                link->magnitude = length_v3(&link->scaled_axis);
                inverse_length = 1.0f / link->magnitude;
                link->normalized_axis.x =
                    link->scaled_axis.x * inverse_length;
                link->normalized_axis.y =
                    link->scaled_axis.y * inverse_length;
                link->normalized_axis.z =
                    link->scaled_axis.z * inverse_length;
            }
            if (include_children != 0) {
                bone = bone->tree_child;
            } else {
                bone = 0;
            }
        }
    }
}

/*
 * Soft ceiling: ~97.95% -- exact control flow and size; the two typed cloth
 * pointers color to r4/r6 instead of retail r5/r4.
 */
void mks_set_cb1_target_bone_cb2(void) {
    ClothBone* bone;
    ClothBone* target;

    bone = mks_cb1;
    target = mks_cb2;
    if (bone != 0 && target != 0) {
        bone->target_bone = target;
        gxVectUVV3ToV3(
            &bone->target_vector,
            &bone->bone->parent_matrix->pos_vec,
            &target->bone->parent_matrix->pos_vec);
    }
}

void mks_cb1_set_coll_offset(float x, float y, float z) {
    ClothBone* bone;

    bone = mks_cb1;
    if (bone != 0) {
        bone->collision_offset.x = x;
        bone->collision_offset.y = y;
        bone->collision_offset.z = z;
    }
}

void mks_cb1_set_coll_offset_xz(float x, float z) {
    ClothBone* bone;

    bone = mks_cb1;
    if (bone != 0) {
        bone->collision_offset.x = x;
        bone->collision_offset.y = 0.0f;
        bone->collision_offset.z = z;
    }
}

void mks_cb1_add_coll_pt(float x, float y, float z) {
    ClothBone* bone;

    bone = mks_cb1;
    if (bone != 0) {
        bone->collision_points[bone->collision_point_count].position.x = x;
        bone->collision_points[bone->collision_point_count].position.y = y;
        bone->collision_points[bone->collision_point_count].position.z = z;
        bone->collision_point_count++;
    }
}

void mks_cb1_set_ground_y(float ground_y) {
    ClothBone* bone;

    bone = mks_cb1;
    if (bone != 0) {
        bone->flags_30_bits.use_ground_y = 1;
        bone->ground_y = ground_y;
    }
}

void mks_set_cb1_wind_normal(float x, float y, float z) {
    if (mks_cb1 != 0) {
        mks_cb1->wind_normal.x = x;
        mks_cb1->wind_normal.y = y;
        mks_cb1->wind_normal.z = z;
        PSVECNormalize(&mks_cb1->wind_normal, &mks_cb1->wind_normal);
    }
}

void mks_set_ground_y_all_cloth_bones(float ground_y) {
    ClothBone* bone;
    unsigned int i;

    bone = plyr_obj->cloth_bones;
    for (i = 0; i < plyr_obj->cloth_bone_count; i++) {
        if (bone != 0) {
            bone->flags_30_bits.use_ground_y = 1;
            bone->ground_y = ground_y;
        }
        bone++;
    }
}

void mks_insert_cloth_force_bones(float rest_length, float stiffness) {
    ClothForcePdata* pdata;
    ClothBone* first;
    ClothBone* second;
    MkObj* obj;

    first = mks_cb1;
    second = mks_cb2;
    obj = plyr_obj;
    if (first == 0 || second == 0) {
        pdata = 0;
    } else {
        pdata = (ClothForcePdata*)get_mkpdata_generic(sizeof(ClothForcePdata));
        if (pdata != 0) {
            pdata->first = first;
            pdata->second = second;
            pdata->rest_length = rest_length;
            pdata->stiffness = stiffness;
        }
    }
    if (pdata != 0) {
        mk_insert((MkHdr*)pdata, &obj->list_7C);
    }
}

void mks_cb2_eq_cloth_bone(int bone_id) {
    mks_cb2 = find_cloth_bone_by_tag(plyr_obj, bone_id);
}

void mks_cb1_eq_cloth_bone(int bone_id) {
    mks_cb1 = find_cloth_bone_by_tag(plyr_obj, bone_id);
}

void mks_mat_id_set_zbias(int material_id, float zbias) {
    RpMaterial* material;

    material = obj_find_material_by_id(plyr_obj, material_id);
    if (material != 0) {
        material_set_zbias(material, zbias);
    }
}

void mks_cloth_bones_init_by_tbl(int table_id, int flags) {
    cloth_bones_init_by_tbl(
        plyr_obj, (ClothInitEntry*)table_id, flags);
}

/* Soft ceiling: mks_bgnd_start_wind ~90.20% -- emit coloring/reloads only. */
void mks_bgnd_start_wind(double x, double y, double z) {
    MkHdr* pdata_hdr;
    ClothWindPdata* pdata;
    MkProc* proc;
    float wind_x;
    float wind_y;

    wind_x = (float)x;
    wind_y = (float)y;
    base_wind_v3.x = (float)x;
    base_wind_v3.y = (float)y;
    base_wind_v3.z = (float)z;
    wind_v3.x = wind_x;
    wind_v3.y = wind_y;
    wind_v3.z = (float)z;

    proc = _create_mkproc_generic_nostack(
        0x500D,
        0x2B,
        p_wind,
        sizeof(ClothWindPdata),
        &pdata_hdr);
    if (proc != 0) {
        if (g_game_info.bgnd_obj != 0) {
            mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
        }
        if (pdata_hdr != 0) {
            zero_pdata_payload(sizeof(ClothWindPdata), pdata_hdr);
            pdata = (ClothWindPdata*)pdata_hdr;
            pdata->amplitude = 0.3f;
            pdata->acceleration = 0.06f;
            pdata->random_scale = 0.01f;
        }
    }
}

static float p_wind(void) {
    ClothWindPdata* pdata;
    ClothProcVtable* vtable;

    pdata = (ClothWindPdata*)apdata;
    pdata->step_x = sfrand(pdata->random_scale);
    pdata->step_z = sfrand(pdata->random_scale);
    pdata->ticks = (unsigned short)randu0(20);
    pdata->ticks += 10;
    vtable = (ClothProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_wind_lp, (MkVtableMkproc*)vtable, 0.0f);
    return 0.0f;
}

/* Soft ceiling: p_wind_lp ~99.76% -- shared 1.0f pool label only. */
static float p_wind_lp(void) {
    ClothWindPdata* pdata;
    ClothProcVtable* vtable;

    pdata = (ClothWindPdata*)apdata;
    pdata->offset_x += pdata->step_x;
    if (pdata->offset_x > pdata->amplitude) {
        pdata->offset_x = pdata->amplitude;
    }
    if (pdata->offset_x < -pdata->amplitude) {
        pdata->offset_x = -pdata->amplitude;
    }

    pdata->offset_z += pdata->step_z;
    if (pdata->offset_z > pdata->amplitude) {
        pdata->offset_z = pdata->amplitude;
    }
    if (pdata->offset_z < -pdata->amplitude) {
        pdata->offset_z = -pdata->amplitude;
    }

    wind_v3.x = base_wind_v3.x + pdata->offset_x;
    wind_v3.z = base_wind_v3.z + pdata->offset_z;
    if (--pdata->ticks <= 0) {
        vtable = (ClothProcVtable*)aproc->vtbl;
        vtable->jump_sleep(p_wind, (MkVtableMkproc*)vtable, 1.0f);
        return 1.0f;
    }
    return 1.0f;
}

void cloth_change_ground_plane_for(float ground_plane) {
    cloth_ground_plane = ground_plane;
}

void vdestroy_cloth_coll_volume(MkHdr* hdr) {
    hdr->instance = 0;
    mkhdr_memfree(hdr);
}

void vdestroy_cloth_coll_plane(MkHdr* hdr) {
    hdr->instance = 0;
    mkhdr_memfree(hdr);
}

void vdestroy_cloth_coll(MkHdr* hdr) {
    hdr->instance = 0;
    mkhdr_memfree(hdr);
}

void start_cloth_proc(void) {
    int flags[2];
    MkProc* proc;

    cloth_mkobj_list = 0;
    cloth_ground_plane = 0.0f;
    flags[1] = 0;
    flags[0] = 0;
    proc = get_mkproc_nostack(flags);
    create_mkproc(0x16, proc, 0x5003, p_cloth, 0);
}

static float p_cloth(void) {
    apply_to_mklist(mkobj_update_cloth, &cloth_mkobj_list);
    apply_to_mklist(mkobj_update_weapon_trail, &weapon_trail_mkobj_list);
    return 1.0f;
}

static void mkobj_update_cloth(MkHdr* header) {
    MkObj* object;
    ClothBone* bone;
    unsigned int index;

    object = (MkObj*)header;
    cloth_obj = object;
    if (object->flags_0C_bits.cloth_update) {
        bone = object->cloth_bones;
        if (bone != 0) {
            cloth_obj_mat = &object->frame->modelling;
            RwMatrixInvert(&cloth_obj_mat_inv, cloth_obj_mat);

            bone = cloth_obj->cloth_bones;
            for (index = 0; index < cloth_obj->cloth_bone_count; index++) {
                if (bone->active == 1) {
                    calc_cloth_dwp(bone);
                }
                bone++;
            }

            bone = cloth_obj->cloth_bones;
            for (index = 0; index < cloth_obj->cloth_bone_count; index++) {
                if (bone->active == 1) {
                    calc_cloth_stretch(bone);
                }
                bone++;
            }

            apply_to_mklist(
                (MkListApplyFn)do_cloth_force, &cloth_obj->list_7C);
            bone = cloth_obj->cloth_bones;
            for (index = 0; index < cloth_obj->cloth_bone_count; index++) {
                bone->collision_amount = 0.0f;
                bone++;
            }
            apply_to_mklist(do_cloth_colls, &cloth_obj->list_80);
            bone = cloth_obj->cloth_bones;
            for (index = 0; index < cloth_obj->cloth_bone_count; index++) {
                if (bone->active != 0) {
                    set_cloth_pos(bone);
                }
                bone++;
            }
        }
    }
}

static void do_cloth_force(ClothForcePdata* force) {
    ClothBone* first;
    ClothBone* second;
    Vec* first_position;
    Vec* second_position;
    Vec difference;
    Vec direction;
    Vec second_compression;
    Vec first_compression;
    Vec second_extension;
    Vec first_extension;
    float distance_squared;
    float distance;
    float inverse_distance;
    float force_amount;
    float first_adjustment;

    first = force->first;
    second = force->second;
    first_position = &first->force_position;
    second_position = &second->force_position;
    PSVECSubtract(second_position, first_position, &difference);
    distance_squared = PSVECDotProduct(&difference, &difference);
    distance = cloth_sqrt(distance_squared);
    if (distance != 0.0f) {
        inverse_distance = 1.0f / distance;
    } else {
        inverse_distance = 1.0f;
    }
    PSVECScale(&difference, &direction, inverse_distance);

    if (distance < force->rest_length) {
        force_amount =
            force->stiffness * (force->rest_length - distance);
        first_adjustment =
            force_amount * first->table_weight /
            (first->table_weight + second->table_weight);
        PSVECScale(&direction, &second_compression, first_adjustment);
        PSVECAdd(
            second_position, &second_compression, second_position);
        PSVECScale(
            &direction, &first_compression,
            first_adjustment - force_amount);
        PSVECAdd(
            first_position, &first_compression, first_position);
    } else {
        force_amount = force->stiffness * (distance - force->rest_length);
        first_adjustment =
            force_amount * first->table_weight /
            (first->table_weight + second->table_weight);
        PSVECScale(&direction, &second_extension, -first_adjustment);
        PSVECAdd(
            second_position, &second_extension, second_position);
        PSVECScale(
            &direction, &first_extension,
            force_amount - first_adjustment);
        PSVECAdd(
            first_position, &first_extension, first_position);
    }
}

static void do_cloth_colls(MkHdr* collision) {
    ClothCollisionVolume* volume;
    ClothCollisionPlane* plane;
    MkBone* reference;
    Vec world_normal;
    Vec plane_offset;
    Vec plane_point;
    Vec point_0;
    Vec point_1;
    Vec center;
    Vec edge_0;
    Vec edge_1;
    Vec adjustment;
    float plane_distance;
    float point_distance;
    unsigned int index;

    if (collision->vtbl == &vtbl_cloth_coll) {
        volume = (ClothCollisionVolume*)collision;
    } else {
        volume = 0;
    }
    cloth_coll = volume;
    if (volume != 0) {
        reference = volume->reference_bone;
        cloth_coll_bone = reference;
        if (!reference->flags_55_bits.collision_disabled) {
            if (reference->flags_55_bits.collision_deferred) {
                return;
            }
            cloth_coll_radius_sq =
                volume->cylinder_bottom * volume->cylinder_bottom;
            PSVECSubtract(
                &reference->matrix.pos_vec,
                &reference->transform_parent->matrix.pos_vec,
                &cloth_coll_bone_uv);
            PSVECScale(
                &cloth_coll_bone_uv, &cloth_coll_bone_uv,
                reference->field_5C);
            RwMatrixInvert(
                &cloth_coll_bone_parent_world_mat_inv,
                &reference->transform_parent->matrix);
            cloth_bone = cloth_obj->cloth_bones;
            for (index = 0; index < volume->bone_count; index++) {
                cloth_bone = volume->bones[index];
                coll_cnt = (int*)&volume->bone_settings[index];
                if (cloth_bone->active != 0) {
                    volume->collision_fn();
                }
            }
        }
        return;
    }

    if (collision->vtbl == &vtbl_cloth_coll_plane) {
        plane = (ClothCollisionPlane*)collision;
    } else {
        plane = 0;
    }
    if (plane != 0) {
        if (!(plane->flags_6C & 0x80)) {
            reference = plane->reference_bone;
            cloth_coll_bone = reference;
            if (!reference->flags_55_bits.collision_disabled) {
                if (reference->flags_55_bits.collision_deferred) {
                    return;
                }
                gxMat33Tx31(
                    &world_normal, &plane->normal,
                    (const Mat33*)&reference->matrix);
                PSVECScale(
                    &plane->normal, &plane_offset, plane->distance);
                gxMatV3MatAddV3(
                    &plane_point, &plane_offset,
                    (const Mat33*)&reference->matrix,
                    &reference->matrix.pos_vec);
                plane_distance =
                    PSVECDotProduct(&world_normal, &plane_point);
                for (index = 0; index < plane->bone_count; index++) {
                    cloth_bone = plane->bones[index];
                    point_distance = PSVECDotProduct(
                        &world_normal, &cloth_bone->force_position);
                    if (point_distance < plane_distance) {
                        PSVECScale(
                            &world_normal, &adjustment,
                            plane_distance - point_distance);
                        PSVECAdd(
                            &cloth_bone->force_position, &adjustment,
                            &cloth_bone->force_position);
                    }
                }
            }
        } else {
            cloth_coll_bone = 0;
            if (plane->flags_6C & 0x40) {
                point_0 = plane->reference_bones[0]->matrix.pos_vec;
                point_1 = plane->reference_bones[1]->matrix.pos_vec;
                PSVECSubtract(
                    &plane->reference_bones[3]->matrix.pos_vec,
                    &plane->reference_bones[2]->matrix.pos_vec,
                    &center);
                PSVECScale(&center, &center, 0.5f);
                PSVECAdd(
                    &center,
                    &plane->reference_bones[2]->matrix.pos_vec,
                    &center);
                PSVECSubtract(&point_0, &center, &edge_1);
                PSVECSubtract(&point_1, &center, &edge_0);
                PSVECCrossProduct(&edge_0, &edge_1, &plane->normal);
                PSVECNormalize(&plane->normal, &plane->normal);
                PSVECScale(
                    &plane->normal, &adjustment, plane->weights[0]);
                PSVECAdd(&point_0, &adjustment, &point_0);
                PSVECScale(
                    &plane->normal, &adjustment, plane->weights[1]);
                PSVECAdd(&point_1, &adjustment, &point_1);
                PSVECScale(
                    &plane->normal, &adjustment,
                    plane->weights[2] +
                        0.5f * (plane->weights[3] - plane->weights[2]));
                PSVECAdd(&center, &adjustment, &center);
                PSVECSubtract(&point_0, &center, &edge_1);
                PSVECSubtract(&point_1, &center, &edge_0);
                PSVECCrossProduct(&edge_0, &edge_1, &plane->normal);
                PSVECNormalize(&plane->normal, &plane->normal);
            }
            for (index = 0; index < plane->bone_count; index++) {
                cloth_bone = plane->bones[index];
                PSVECSubtract(
                    &cloth_bone->force_position, &point_0, &adjustment);
                point_distance =
                    PSVECDotProduct(&adjustment, &plane->normal);
                if (point_distance < 0.0f) {
                    PSVECScale(
                        &plane->normal, &adjustment, -point_distance);
                    PSVECAdd(
                        &cloth_bone->force_position, &adjustment,
                        &cloth_bone->force_position);
                }
            }
        }
    } else if (collision->vtbl == &vtbl_cloth_coll_volume) {
        /* Retail reserves this collision object type without an update path. */
    }
}

static void cloth_coll_point_cyl_inside(void) {
    ClothCollisionScratch* scratch;
    Vec* force_position;
    Vec axis_offset_0;
    Vec axis_point_0;
    Vec radial_direction_0;
    Vec axis_offset_1;
    Vec axis_point_1;
    Vec radial_direction_1;
    Vec world_point;
    unsigned int index;
    int collided;
    float position_weight;
    float along_axis;

    scratch = &pt_displacement_v;
    force_position = &cloth_bone->force_position;
    PSVECSubtract(
        force_position,
        &cloth_coll_bone->transform_parent->matrix.pos_vec,
        &scratch->bone_to_point);
    PSVECCrossProduct(
        &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
    if (PSVECDotProduct(&scratch->cross, &scratch->cross) >
        cloth_coll_radius_sq) {
        along_axis =
            PSVECDotProduct(&scratch->bone_to_point, &scratch->bone_axis);
        if (along_axis < cloth_coll->cylinder_bottom) {
            along_axis = cloth_coll->cylinder_bottom;
        }
        PSVECScale(&scratch->bone_axis, &axis_offset_0, along_axis);
        PSVECAdd(
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &axis_offset_0, &axis_point_0);
        gxVectUVV3ToV3(
            &radial_direction_0, &axis_point_0,
            force_position);
        PSVECScale(
            &radial_direction_0, &radial_direction_0,
            cloth_coll->radius);
        PSVECAdd(&axis_point_0, &radial_direction_0, &axis_point_0);
        PSVECSubtract(
            &axis_point_0, force_position,
            &scratch->displacement);
        collided = 1;
    } else {
        collided = 0;
    }
    if (collided) {
        PSVECAdd(
            force_position, &scratch->displacement,
            force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (const Mat33*)&cloth_bone->bone->matrix,
            force_position);
        PSVECSubtract(
            &world_point,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &scratch->bone_to_point);
        PSVECCrossProduct(
            &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
        if (PSVECDotProduct(&scratch->cross, &scratch->cross) >
            cloth_coll_radius_sq) {
            along_axis = PSVECDotProduct(
                &scratch->bone_to_point, &scratch->bone_axis);
            if (along_axis < cloth_coll->cylinder_bottom) {
                along_axis = cloth_coll->cylinder_bottom;
            }
            PSVECScale(
                &scratch->bone_axis, &axis_offset_1, along_axis);
            PSVECAdd(
                &cloth_coll_bone->transform_parent->matrix.pos_vec,
                &axis_offset_1, &axis_point_1);
            gxVectUVV3ToV3(
                &radial_direction_1, &axis_point_1, &world_point);
            PSVECScale(
                &radial_direction_1, &radial_direction_1,
                cloth_coll->radius);
            PSVECAdd(
                &axis_point_1, &radial_direction_1, &axis_point_1);
            PSVECSubtract(
                &axis_point_1, &world_point, &scratch->displacement);
            collided = 1;
        } else {
            collided = 0;
        }
        if (collided) {
            position_weight = 1.0f - cloth_bone->table_scale;
            scratch->displacement.x *= position_weight;
            scratch->displacement.y *= position_weight;
            scratch->displacement.z *= position_weight;
            cloth_bone->force_position.x += scratch->displacement.x;
            cloth_bone->force_position.y += scratch->displacement.y;
            cloth_bone->force_position.z += scratch->displacement.z;
            coll_cnt++;
        }
    }
}

static inline int cloth_vector_cylinder_displacement(const Vec* point) {
    Vec midpoint;
    Vec radial_direction;
    Vec target_position;
    Vec local_displacement;
    Vec world_displacement;
    Vec world_position;
    float along_axis;
    float previous_y;

    if (point->x * point->x + point->z * point->z <
            cloth_coll_radius_sq &&
        (along_axis = -point->y) > cloth_coll->cylinder_bottom &&
        along_axis <= cloth_coll->cylinder_top) {
        PSVECSubtract(
            point, &cloth_bone->previous_collision_local_position,
            &midpoint);
        PSVECScale(&midpoint, &midpoint, 0.5f);
        PSVECAdd(
            &midpoint, &cloth_bone->previous_collision_local_position,
            &midpoint);
        midpoint.y = 0.0f;
        PSVECAdd(&midpoint, &cloth_bone->collision_offset, &midpoint);
        PSVECNormalize(&midpoint, &radial_direction);
        previous_y = cloth_bone->previous_collision_local_position.y;
        PSVECScale(
            &radial_direction, &target_position,
            cloth_coll->radius + 0.01f);
        target_position.y = previous_y;
        PSVECSubtract(&target_position, point, &local_displacement);
        PSVECAdd(
            &cloth_bone->collision_local_position, &local_displacement,
            &cloth_bone->collision_local_position);
        gxMat33Tx31(
            &world_displacement, &cloth_bone->collision_local_position,
            (const Mat33*)&cloth_coll_bone->transform_parent->matrix);
        PSVECAdd(
            &world_displacement,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &world_position);
        PSVECSubtract(
            &world_position, &cloth_bone->force_position,
            &cloth_displacement_v);
        return 1;
    }
    return 0;
}

static void cloth_coll_vector_cyl(void) {
    Vec object_offset;
    Vec world_point;
    unsigned int index;
    float position_weight;

    cloth_bone->previous_collision_local_position =
        cloth_bone->collision_local_position;
    PSVECSubtract(
        &cloth_bone->force_position,
        &cloth_coll_bone->transform_parent->matrix.pos_vec,
        &object_offset);
    gxMat33Tx31(
        &cloth_bone->collision_local_position, &object_offset,
        (const Mat33*)&cloth_coll_bone_parent_world_mat_inv);
    if (cloth_vector_cylinder_displacement(
            &cloth_bone->collision_local_position)) {
        PSVECAdd(
            &cloth_bone->force_position, &cloth_displacement_v,
            &cloth_bone->force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (const Mat33*)&cloth_bone->bone->matrix,
            &cloth_bone->force_position);
        PSVECSubtract(
            &world_point,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &object_offset);
        gxMat33Tx31(
            &world_point, &object_offset,
            (const Mat33*)&cloth_coll_bone_parent_world_mat_inv);
        if (cloth_vector_cylinder_displacement(&world_point)) {
            position_weight = 1.0f - cloth_bone->table_scale;
            PSVECScale(
                &cloth_displacement_v, &cloth_displacement_v,
                position_weight);
            PSVECAdd(
                &cloth_bone->force_position, &cloth_displacement_v,
                &cloth_bone->force_position);
            coll_cnt++;
        }
    }
}

static void cloth_coll_point_cyl_abs(void) {
    ClothCollisionScratch* scratch;
    Vec* force_position;
    Vec axis_offset_0;
    Vec axis_point_0;
    Vec offset_point_0;
    Vec radial_direction_0;
    Vec axis_offset_1;
    Vec axis_point_1;
    Vec offset_point_1;
    Vec radial_direction_1;
    Vec world_point;
    unsigned int index;
    int collided;
    float position_weight;
    float along_axis;
    float displacement_length;

    scratch = &pt_displacement_v;
    force_position = &cloth_bone->force_position;
    PSVECSubtract(
        force_position,
        &cloth_coll_bone->transform_parent->matrix.pos_vec,
        &scratch->bone_to_point);
    PSVECCrossProduct(
        &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
    if (PSVECDotProduct(&scratch->cross, &scratch->cross) <
            cloth_coll_radius_sq &&
        (along_axis =
             PSVECDotProduct(&scratch->bone_to_point, &scratch->bone_axis)) >
            cloth_coll->cylinder_bottom &&
        along_axis <= cloth_coll->cylinder_top) {
        PSVECScale(&scratch->bone_axis, &axis_offset_0, along_axis);
        PSVECAdd(
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &axis_offset_0, &axis_point_0);
        gxMatV3MatAddV3(
            &offset_point_0, &cloth_bone->collision_offset,
            (const Mat33*)&cloth_bone->bone->matrix,
            force_position);
        gxVectUVV3ToV3(
            &radial_direction_0, &axis_point_0, &offset_point_0);
        PSVECScale(
            &radial_direction_0, &radial_direction_0,
            cloth_coll->radius);
        PSVECAdd(&axis_point_0, &radial_direction_0, &axis_point_0);
        PSVECSubtract(
            &axis_point_0, force_position,
            &scratch->displacement);
        displacement_length = PSVECMag(&scratch->displacement);
        if (displacement_length > cloth_bone->collision_amount) {
            cloth_bone->collision_amount = displacement_length;
        }
        collided = 1;
    } else {
        collided = 0;
    }
    if (collided) {
        PSVECAdd(
            force_position, &scratch->displacement,
            force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (const Mat33*)&cloth_bone->bone->matrix,
            force_position);
        PSVECSubtract(
            &world_point,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &scratch->bone_to_point);
        PSVECCrossProduct(
            &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
        if (PSVECDotProduct(&scratch->cross, &scratch->cross) <
                cloth_coll_radius_sq &&
            (along_axis = PSVECDotProduct(
                 &scratch->bone_to_point, &scratch->bone_axis)) >
                cloth_coll->cylinder_bottom &&
            along_axis <= cloth_coll->cylinder_top) {
            PSVECScale(
                &scratch->bone_axis, &axis_offset_1, along_axis);
            PSVECAdd(
                &cloth_coll_bone->transform_parent->matrix.pos_vec,
                &axis_offset_1, &axis_point_1);
            gxMatV3MatAddV3(
                &offset_point_1, &cloth_bone->collision_offset,
                (const Mat33*)&cloth_bone->bone->matrix, &world_point);
            gxVectUVV3ToV3(
                &radial_direction_1, &axis_point_1, &offset_point_1);
            PSVECScale(
                &radial_direction_1, &radial_direction_1,
                cloth_coll->radius);
            PSVECAdd(
                &axis_point_1, &radial_direction_1, &axis_point_1);
            PSVECSubtract(
                &axis_point_1, &world_point, &scratch->displacement);
            displacement_length = PSVECMag(&scratch->displacement);
            if (displacement_length > cloth_bone->collision_amount) {
                cloth_bone->collision_amount = displacement_length;
            }
            collided = 1;
        } else {
            collided = 0;
        }
        if (collided) {
            position_weight = 1.0f - cloth_bone->table_scale;
            PSVECScale(
                &scratch->displacement, &scratch->displacement,
                position_weight);
            PSVECAdd(
                force_position, &scratch->displacement,
                force_position);
            coll_cnt++;
        }
    }
}

static void cloth_coll_point_cyl_rel(void) {
    ClothCollisionScratch* scratch;
    Vec* force_position;
    Vec axis_point_0;
    Vec offset_point_0;
    Vec radial_direction_0;
    Vec axis_point_1;
    Vec offset_point_1;
    Vec radial_direction_1;
    Vec world_point;
    unsigned int index;
    int collided;
    float position_weight;
    float radial_squared;
    float along_axis;
    float displacement_length;

    scratch = &pt_displacement_v;
    force_position = &cloth_bone->force_position;
    PSVECSubtract(
        force_position,
        &cloth_coll_bone->transform_parent->matrix.pos_vec,
        &scratch->bone_to_point);
    PSVECCrossProduct(
        &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
    radial_squared = PSVECDotProduct(&scratch->cross, &scratch->cross);
    if (radial_squared < cloth_coll_radius_sq &&
        (along_axis =
             PSVECDotProduct(&scratch->bone_to_point, &scratch->bone_axis)) >
            cloth_coll->cylinder_bottom &&
        along_axis <= cloth_coll->cylinder_top) {
        PSVECScale(&scratch->bone_axis, &axis_point_0, along_axis);
        PSVECAdd(
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &axis_point_0, &axis_point_0);
        gxMatV3MatAddV3(
            &offset_point_0, &cloth_bone->collision_offset,
            (const Mat33*)&cloth_bone->bone->matrix,
            force_position);
        gxVectUVV3ToV3(
            &radial_direction_0, &axis_point_0, &offset_point_0);
        displacement_length =
            (cloth_coll_radius_sq - radial_squared) /
            cloth_coll->radius;
        if (displacement_length > cloth_bone->collision_amount) {
            cloth_bone->collision_amount = displacement_length;
        }
        PSVECScale(
            &radial_direction_0, &scratch->displacement,
            displacement_length);
        collided = 1;
    } else {
        collided = 0;
    }
    if (collided) {
        PSVECAdd(
            force_position, &scratch->displacement,
            force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (const Mat33*)&cloth_bone->bone->matrix,
            force_position);
        PSVECSubtract(
            &world_point,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &scratch->bone_to_point);
        PSVECCrossProduct(
            &scratch->bone_axis, &scratch->bone_to_point, &scratch->cross);
        radial_squared = PSVECDotProduct(&scratch->cross, &scratch->cross);
        if (radial_squared < cloth_coll_radius_sq &&
            (along_axis = PSVECDotProduct(
                 &scratch->bone_to_point, &scratch->bone_axis)) >
                cloth_coll->cylinder_bottom &&
            along_axis <= cloth_coll->cylinder_top) {
            PSVECScale(
                &scratch->bone_axis, &axis_point_1, along_axis);
            PSVECAdd(
                &cloth_coll_bone->transform_parent->matrix.pos_vec,
                &axis_point_1, &axis_point_1);
            gxMatV3MatAddV3(
                &offset_point_1, &cloth_bone->collision_offset,
                (const Mat33*)&cloth_bone->bone->matrix, &world_point);
            gxVectUVV3ToV3(
                &radial_direction_1, &axis_point_1, &offset_point_1);
            displacement_length =
                (cloth_coll_radius_sq - radial_squared) /
                cloth_coll->radius;
            if (displacement_length > cloth_bone->collision_amount) {
                cloth_bone->collision_amount = displacement_length;
            }
            PSVECScale(
                &radial_direction_1, &scratch->displacement,
                displacement_length);
            collided = 1;
        } else {
            collided = 0;
        }
        if (collided) {
            position_weight = 1.0f - cloth_bone->table_scale;
            PSVECScale(
                &scratch->displacement, &scratch->displacement,
                position_weight);
            PSVECAdd(
                force_position, &scratch->displacement,
                force_position);
            coll_cnt++;
        }
    }
}

static void set_cloth_pos(ClothBone* bone) {
    MkBone* render_bone;
    MkBone* parent_bone;
    Vec object_position;
    Vec direction;
    Vec normalized_direction;
    Quat rotation;
    RwMatrix inverse_parent;
    float interpolation;

    render_bone = bone->bone;
    if (!render_bone->flags_55_bits.collision_deferred &&
        render_bone->parent_matrix != 0 &&
        render_bone->flags_54_bits.hierarchy_driven) {
        parent_bone = render_bone->transform_parent;
        PSVECSubtract(
            &bone->force_position, &render_bone->matrix.pos_vec,
            &bone->velocity);
        render_bone->matrix.pos_vec = bone->force_position;
        PSVECSubtract(
            &render_bone->matrix.pos_vec, &cloth_obj_mat->pos_vec,
            &object_position);
        gxMat33Tx31(
            &render_bone->parent_matrix->pos_vec, &object_position,
            (const Mat33*)&cloth_obj_mat_inv);

        if (bone->target_bone != 0) {
            PSVECSubtract(
                &bone->target_bone->force_position,
                &bone->force_position, &object_position);
            gxMat33Tx31(
                &direction, &object_position,
                (const Mat33*)&cloth_obj_mat_inv);
            RwMatrixInvert(&inverse_parent, parent_bone->parent_matrix);
            gxMat33Tx31(
                &object_position, &direction,
                (const Mat33*)&inverse_parent);
            PSVECNormalize(&object_position, &direction);
            gxVectV3V3ToQuat(
                &rotation, &bone->target_vector, &direction);
            if (render_bone->update_tick == (unsigned int)exec_tick_ctr &&
                render_bone->field_60 > 0.0f) {
                interpolation = render_bone->field_60 > 1.0f
                                    ? 1.0f
                                    : render_bone->field_60;
                gxQuatInterpQuat(
                    &rotation, &render_bone->rotation, &rotation,
                    interpolation);
            }
            gxQuatQuatToMat((Mat33*)&inverse_parent, &rotation);
            gxMat33x33_Check(
                (Mat33*)render_bone->parent_matrix,
                (const Mat33*)&inverse_parent,
                (const Mat33*)parent_bone->parent_matrix);
            gxMat33x33_Check(
                (Mat33*)&render_bone->matrix,
                (const Mat33*)render_bone->parent_matrix,
                (const Mat33*)cloth_obj_mat);
            return;
        }

        PSVECSubtract(
            &render_bone->parent_matrix->pos_vec,
            &parent_bone->parent_matrix->pos_vec, &object_position);
        if (render_bone->flags_55_bits.scale_controlled &&
            !parent_bone->flags_54_bits.hierarchy_driven &&
            parent_bone->flags_55_bits.scale_controlled) {
            Vec zero_scale;
            zero_scale.x = 0.0f;
            zero_scale.y = 0.0f;
            zero_scale.z = 0.0f;
            inverse_parent.right.x = 1.0f;
            inverse_parent.right.y = 0.0f;
            inverse_parent.right.z = 0.0f;
            inverse_parent.flags = 0x20003;
            inverse_parent.up.x = 0.0f;
            inverse_parent.up.y = 1.0f;
            inverse_parent.up.z = 0.0f;
            inverse_parent.at.x = 0.0f;
            inverse_parent.at.y = 0.0f;
            inverse_parent.at.z = 1.0f;
            inverse_parent.pos.x = 0.0f;
            inverse_parent.pos.y = 0.0f;
            inverse_parent.pos.z = 0.0f;
            RwMatrixScale(
                &inverse_parent, (const RwV3d*)&zero_scale, 0);
        } else {
            RwMatrixInvert(&inverse_parent, parent_bone->parent_matrix);
        }
        gxMat33Tx31(
            &direction, &object_position,
            (const Mat33*)&inverse_parent);
        if (direction.x == 0.0f && direction.y == 0.0f &&
            direction.z == 0.0f) {
            direction.z = 0.00001f;
        }
        PSVECNormalize(&direction, &normalized_direction);
        gxVectV3V3ToQuat(
            &rotation, &bone->target_vector, &normalized_direction);
        if (render_bone->update_tick == (unsigned int)exec_tick_ctr &&
            render_bone->field_60 > 0.0f) {
            interpolation = render_bone->field_60 > 1.0f
                                ? 1.0f
                                : render_bone->field_60;
            gxQuatInterpQuat(
                &rotation, &render_bone->rotation, &rotation,
                interpolation);
        }
        gxQuatQuatToMat((Mat33*)&inverse_parent, &rotation);
        gxMat33x33_Check(
            (Mat33*)render_bone->parent_matrix,
            (const Mat33*)&inverse_parent,
            (const Mat33*)parent_bone->parent_matrix);
        gxMat33x33_Check(
            (Mat33*)&render_bone->matrix,
            (const Mat33*)render_bone->parent_matrix,
            (const Mat33*)cloth_obj_mat);
    }
}

static void calc_cloth_stretch(ClothBone* bone) {
    MkBone* render_bone;
    MkBone* parent_bone;
    ClothBone* parent_cloth;
    Vec difference;
    Vec direction;
    Vec adjustment;
    float distance_squared;
    float distance;
    float inverse_distance;
    float correction;
    float parent_adjustment;

    render_bone = bone->bone;
    if (!render_bone->flags_55_bits.collision_deferred &&
        render_bone->parent_matrix != 0 &&
        render_bone->flags_54_bits.hierarchy_driven) {
        parent_bone = render_bone->transform_parent;
        if (parent_bone != 0) {
            parent_cloth = parent_bone->cloth_link;
            if (parent_cloth != 0 &&
                parent_bone->flags_54_bits.hierarchy_driven) {
                PSVECSubtract(
                    &bone->force_position,
                    &parent_cloth->force_position, &difference);
            } else {
                PSVECSubtract(
                    &bone->force_position,
                    &parent_bone->matrix.pos_vec, &difference);
            }
            distance_squared = PSVECDotProduct(&difference, &difference);
            distance = cloth_sqrt(distance_squared);
            if (distance != 0.0f) {
                inverse_distance = 1.0f / distance;
            } else {
                inverse_distance = 1.0f;
            }
            PSVECScale(&difference, &direction, inverse_distance);
            if (distance < bone->rest_length) {
                correction =
                    -(0.1f * bone->stretch_weight - 1.0f) *
                    (bone->rest_length - distance);
                bone->current_length = distance + correction;
                if (parent_cloth != 0 &&
                    parent_cloth->stretch_weight != 0.0f) {
                    parent_adjustment =
                        correction * parent_cloth->stretch_weight /
                        (bone->stretch_weight +
                         parent_cloth->stretch_weight);
                    PSVECScale(
                        &direction, &adjustment, -parent_adjustment);
                    PSVECAdd(
                        &parent_cloth->force_position, &adjustment,
                        &parent_cloth->force_position);
                } else {
                    parent_adjustment = 0.0f;
                }
                PSVECScale(
                    &direction, &adjustment,
                    correction - parent_adjustment);
                PSVECAdd(
                    &bone->force_position, &adjustment,
                    &bone->force_position);
            } else {
                correction =
                    -(0.1f * bone->stretch_weight - 1.0f) *
                    (distance - bone->rest_length);
                bone->current_length = distance - correction;
                if (parent_cloth != 0 &&
                    parent_cloth->stretch_weight != 0.0f) {
                    parent_adjustment =
                        correction * parent_cloth->stretch_weight /
                        (bone->stretch_weight +
                         parent_cloth->stretch_weight);
                    PSVECScale(
                        &direction, &adjustment, parent_adjustment);
                    PSVECAdd(
                        &parent_cloth->force_position, &adjustment,
                        &parent_cloth->force_position);
                } else {
                    parent_adjustment = 0.0f;
                }
                PSVECScale(
                    &direction, &adjustment,
                    parent_adjustment - correction);
                PSVECAdd(
                    &bone->force_position, &adjustment,
                    &bone->force_position);
            }
        }
    }
}

static void calc_cloth_dwp(ClothBone* bone) {
    MkBone* render_bone;
    MkBone* parent_bone;
    MkBone* grandparent_bone;
    ClothBone* parent_cloth;
    const RwMatrix* target_matrix;
    const Vec* target_origin;
    Vec velocity;
    Vec adjustment;
    Vec spring_velocity;
    Vec transformed_normal;
    float collision_scale;
    float spring_scale;
    float wind_dot;
    float wind_scale;

    render_bone = bone->bone;
    if (!render_bone->flags_55_bits.collision_deferred &&
        render_bone->parent_matrix != 0 &&
        render_bone->flags_54_bits.hierarchy_driven) {
        parent_bone = render_bone->transform_parent;
        PSVECScale(
            &bone->velocity, &velocity,
            -(((1.0f - bone->segment_length) * sqrt_game_speed) - 1.0f));
        if (bone->collision_amount != 0.0f) {
            collision_scale =
                bone->collision_amount * (50.0f * sqrt_game_speed);
            if (collision_scale > 1.0f) {
                collision_scale = 1.0f;
            }
            PSVECScale(
                &velocity, &adjustment,
                (1.0f - bone->damping_factor) * collision_scale);
            PSVECSubtract(&velocity, &adjustment, &velocity);
        }

        target_matrix = cloth_obj_mat;
        target_origin = &cloth_obj_mat->pos_vec;
        velocity.y += bone->force_step * game_speed;
        if (parent_bone != 0) {
            parent_cloth = parent_bone->cloth_link;
            if (parent_cloth != 0 &&
                parent_bone->flags_54_bits.hierarchy_driven) {
                target_origin = &parent_cloth->force_position;
            } else {
                target_origin = &parent_bone->matrix.pos_vec;
            }
            grandparent_bone = parent_bone->transform_parent;
            if (grandparent_bone != 0 &&
                grandparent_bone->cloth_link != 0 &&
                grandparent_bone->cloth_link->target_bone != 0) {
                target_matrix = &grandparent_bone->matrix;
            } else {
                target_matrix = &parent_bone->matrix;
            }
        }
        gxMatV3MatAddV3(
            &bone->world_target, &bone->local_cloth_position,
            (const Mat33*)target_matrix, target_origin);
        PSVECSubtract(
            &bone->world_target, &render_bone->matrix.pos_vec,
            &adjustment);
        if (game_speed < 1.0f) {
            spring_scale = bone->stiffness_squared * sqrt_game_speed;
        } else {
            spring_scale = bone->stiffness_squared;
        }
        PSVECScale(&adjustment, &spring_velocity, spring_scale);
        PSVECAdd(&velocity, &spring_velocity, &velocity);

        if (bone->initial_z != 0.0f) {
            transformed_normal.x =
                bone->wind_normal.z * render_bone->parent_matrix->at.x +
                (bone->wind_normal.x * render_bone->parent_matrix->right.x +
                 bone->wind_normal.y * render_bone->parent_matrix->up.x);
            transformed_normal.y =
                bone->wind_normal.z * render_bone->parent_matrix->at.y +
                (bone->wind_normal.x * render_bone->parent_matrix->right.y +
                 bone->wind_normal.y * render_bone->parent_matrix->up.y);
            transformed_normal.z =
                bone->wind_normal.z * render_bone->parent_matrix->at.z +
                (bone->wind_normal.x * render_bone->parent_matrix->right.z +
                 bone->wind_normal.y * render_bone->parent_matrix->up.z);
            adjustment.x =
                transformed_normal.z * cloth_obj_mat->at.x +
                (transformed_normal.x * cloth_obj_mat->right.x +
                 transformed_normal.y * cloth_obj_mat->up.x);
            adjustment.y =
                transformed_normal.z * cloth_obj_mat->at.y +
                (transformed_normal.x * cloth_obj_mat->right.y +
                 transformed_normal.y * cloth_obj_mat->up.y);
            adjustment.z =
                transformed_normal.z * cloth_obj_mat->at.z +
                (transformed_normal.x * cloth_obj_mat->right.z +
                 transformed_normal.y * cloth_obj_mat->up.z);
            transformed_normal.x = wind_v3.x - velocity.x;
            transformed_normal.y = wind_v3.y - velocity.y;
            transformed_normal.z = wind_v3.z - velocity.z;
            wind_dot =
                PSVECDotProduct(&adjustment, &transformed_normal);
            if (wind_dot < 0.0f) {
                wind_dot *= -1.0f;
            }
            wind_scale = wind_dot * bone->initial_z + bone->initial_x;
            adjustment.x = wind_v3.x * wind_scale;
            adjustment.y = wind_v3.y * wind_scale;
            adjustment.z = wind_v3.z * wind_scale;
            velocity.x += adjustment.x;
            velocity.y += adjustment.y;
            velocity.z += adjustment.z;
        }
        PSVECAdd(
            &render_bone->matrix.pos_vec, &velocity,
            &bone->force_position);
        if (bone->flags_30_bits.use_ground_y &&
            bone->force_position.y < bone->ground_y + cloth_ground_plane) {
            bone->force_position.y = bone->ground_y + cloth_ground_plane;
        }
    }
}

void start_cloth_bones(MkObj* obj) {
    MkBone* render_bone;
    MkBone* parent_bone;
    ClothBone* bone;
    unsigned int cloth_count;
    unsigned int bone_index;
    unsigned int cloth_index;
    float rest_length;

    if (obj->pos.x != obj->pos.x) {
        obj->pos.x = 0.0f;
    }
    if (obj->pos.y != obj->pos.y) {
        obj->pos.y = 0.0f;
    }
    if (obj->pos.z != obj->pos.z) {
        obj->pos.z = 0.0f;
    }

    cloth_count = 0;
    for (bone_index = 0; bone_index < obj->bone_count; bone_index++) {
        render_bone = obj->bones[bone_index];
        if (render_bone != 0 &&
            render_bone->flags_54_bits.cloth_candidate) {
            cloth_count++;
            render_bone->flags_54_bits.calculation_locked = 1;
            if (render_bone->transform_parent != 0) {
                render_bone->transform_parent->flags_54_bits
                    .calculation_locked = 1;
            }
        }
    }

    if (cloth_count != 0) {
        obj->cloth_bone_count = cloth_count;
        obj->cloth_bones = get_mem(cloth_count * sizeof(ClothBone));
        if (obj->cloth_bones != 0) {
            bone_index = 0;
            render_bone = obj->bones[0];
            for (cloth_index = 0; cloth_index < cloth_count;
                 cloth_index++) {
                bone = &obj->cloth_bones[cloth_index];
                while (bone_index < obj->bone_count) {
                    render_bone = obj->bones[bone_index];
                    if (render_bone->flags_54_bits.cloth_candidate &&
                        render_bone->cloth_link == 0) {
                        break;
                    }
                    bone_index++;
                }
                for (;;) {
                    parent_bone = render_bone->transform_parent;
                    if (parent_bone == 0 ||
                        !parent_bone->flags_54_bits.cloth_candidate ||
                        parent_bone->cloth_link != 0) {
                        break;
                    }
                    render_bone = parent_bone;
                }
                bone->bone = render_bone;
                render_bone->cloth_link = bone;
                bone->world_target.x = 0.0f;
                bone->world_target.y = 0.0f;
                bone->world_target.z = 0.0f;
                bone->force_position.x = 0.0f;
                bone->force_position.y = 0.0f;
                bone->force_position.z = 0.0f;
                bone->velocity.x = 0.0f;
                bone->velocity.y = 0.0f;
                bone->velocity.z = 0.0f;
                gxQuatSetZero(&bone->collision_rotation);
                bone->previous_collision_local_position.x = 0.0f;
                bone->previous_collision_local_position.y = 0.0f;
                bone->previous_collision_local_position.z = 0.0f;
                bone->collision_local_position.x = 0.0f;
                bone->collision_local_position.y = 0.0f;
                bone->collision_local_position.z = 0.0f;
                bone->collision_amount = 0.0f;
                bone->target_bone = 0;
                bone->wind_normal.x = 0.0f;
                bone->wind_normal.y = 0.0f;
                bone->wind_normal.z = 0.0f;
                bone->active = 0;
                bone->flags_30 = 0;
                bone->flags_30_bits.use_ground_y = 1;
                bone->ground_y = 0.1f;
                bone->local_cloth_position = render_bone->translation;
                rest_length = PSVECMag(&bone->local_cloth_position);
                bone->rest_length = rest_length;
                bone->current_length = rest_length;
                PSVECScale(
                    &bone->local_cloth_position, &bone->target_vector,
                    1.0f / bone->rest_length);
                bone->collision_offset.x = 0.0f;
                bone->collision_offset.y = 0.0f;
                bone->collision_offset.z = 0.0f;
                bone->collision_point_count = 0;
                bone->stretch_weight = 0.0f;
                bone->table_scale = 0.0f;
                bone->stiffness_squared = 1.0f;
                bone->segment_length = 0.0f;
                bone->damping_factor = 0.0f;
                bone->initial_x = 0.0f;
                bone->initial_z = 0.0f;
                bone->force_step = 0.0f;
            }
        }
        obj->flags_0C_bits.cloth_update = 1;
        mk_insert((MkHdr*)obj, &cloth_mkobj_list);
    }
}

void cloth_bones_init_by_tbl(
    MkObj* object, ClothInitEntry* table, int count) {
    ClothBone* cloth_bone;
    MkBone* bone;
    int index;

    for (index = 0; index < count; index++, table++) {
        cloth_bone = find_cloth_bone_by_tag(object, table->bone_tag);
        if (cloth_bone != 0) {
            cloth_bone->active = 1;
            bone = cloth_bone->bone;
            bone->flags_54_bits.hierarchy_driven = 1;
            if (object == g_game_info.player_objects[0] ||
                object == g_game_info.player_objects[1]) {
                if (table->initial_x != 0.0f) {
                    table->initial_x = 0.0f;
                }
                if (table->initial_z != 0.0f) {
                    table->initial_z = 0.0f;
                }
            }
            cloth_bone->stiffness_squared =
                table->stiffness * table->stiffness;
            cloth_bone->table_weight = table->segment_length;
            cloth_bone->segment_length = cloth_sqrt(table->segment_length);
            cloth_bone->force_step = -table->force / 50.0f;
            cloth_bone->stretch_weight = 0.0f;
            cloth_bone->damping_factor =
                1.0f - table->damping * table->damping;
            cloth_bone->initial_x = table->initial_x;
            cloth_bone->initial_z = table->initial_z;
            cloth_bone->table_scale = table->table_scale;
        }
    }
}

int find_cloth_bone_id_from_tag(MkObj* obj, int tag) {
    ClothBone* bone;

    bone = find_cloth_bone_by_tag(obj, tag);
    if (bone != 0) {
        return bone->bone->bone_index;
    }
    return -1;
}

void obj_translate_cloth(MkObj* object, const Vec* translation) {
    ClothBone* bone;
    unsigned int index;

    bone = object->cloth_bones;
    for (index = 0; index < object->cloth_bone_count; index++) {
        v3_add_v3(&bone->bone->matrix.pos_vec,
                  &bone->bone->matrix.pos_vec,
                  translation);
        bone++;
    }
}

float p_axis_track_bone_world_mat(void) {
    MkBone* target_bone;
    RwFrame* frame;
    RwMatrix* matrix;
    MkObj* object;

    target_bone = axis_pdata->target->bones[axis_pdata->bone_index];
    if (target_bone == 0) {
        return -1.0f;
    }

    frame = axis_obj->frame;
    matrix = &frame->modelling;
    memcpy(matrix, &target_bone->matrix, sizeof(*matrix));
    if (axis_obj->flags_08_bits.scale_active) {
        gxMatScaledByV3(
            (Mat33*)matrix, (const Mat33*)matrix, &axis_obj->scale);
    }
    RwMatrixUpdate(matrix);
    RwFrameUpdateObjects(frame);
    object = axis_obj;
    if (hide_axis != 0) {
        object->hide_flag_bits.hidden = 1;
    } else {
        object->hide_flag_bits.hidden = 0;
    }
    return 1.0f;
}

static void ps_axis(void) {
    axis_pdata = 0;
    axis_obj = 0;
    axis_sobj = 0;
}

static void pw_axis(void) {
    MkObj* object;

    axis_pdata = (AxisPdata*)apdata;
    if (axis_pdata != 0) {
        object = axis_pdata->axis;
        if (object != 0 &&
            object->hdr.instance != axis_pdata->axis_instance) {
            object = 0;
        }
        axis_obj = object;
        if (object == 0) {
            mkproc_die();
        }
    }
}
