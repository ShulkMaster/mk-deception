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
#include "runtime/asset.h"
#include "runtime/cstring.h"
#include "rw/rtquat.h"
#include "rw/rwframe.h"

static float p_cloth(void);
static float p_wind(void);
static float p_wind_lp(void);
typedef struct ClothInitEntry ClothInitEntry;
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
extern int gap_05_8033EDA4_data;

static void cloth_coll_vector_cyl(void);
static void cloth_coll_point_cyl_abs(void);
static void cloth_coll_point_cyl_inside(void);
static void cloth_coll_point_cyl_rel(void);
int obj_get_bid_for_tid(MkObj* obj, int tag);

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


static float cloth_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float refined;

    input.f = value;
    if (value <= 0.0f) {
        return 0.0f;
    }
    guess.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 11) & 0x1FFF] << 8;
    guess.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) &
        0x7F800000U;
    refined = guess.f * (3.0f - (guess.f * guess.f) / value);
    return 0.5f * refined;
}

typedef float (*ClothJumpSleepFn)(
    MkProcEntryFn entry,
    MkVtableMkproc* vtable,
    float sleep_ticks);

typedef struct ClothProcVtable {
    void* slots[9];
    ClothJumpSleepFn jump_sleep;
} ClothProcVtable;

static inline ClothBone* find_cloth_bone_by_tag(MkObj* obj, int bone_id) {
    ClothBone* bone;
    unsigned int count;
    unsigned int index;

    bone_id &= 0xFFF;
    bone = obj->cloth_bones;
    count = obj->cloth_bone_count;
    for (index = 0; index < count; index++) {
        if ((bone->bone->tag & 0xFFF) == bone_id) {
            return bone;
        }
        bone++;
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
    MkBone* bone;
    MkObj* object;

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
            volume->reference_bone->flags_54_bits.calculation_locked = 1;
            if (volume->reference_bone->transform_parent != 0) {
                volume->reference_bone->transform_parent->flags_54_bits.calculation_locked = 1;
            }
        }
        if (volume != 0) {
            mk_insert((MkHdr*)volume, &object->list_80);
        }
        mks_cc1 = volume;
    }
}

static inline void cloth_bone_set_target(ClothBone* bone, ClothBone* target) {
    if (bone != 0 && target != 0) {
        bone->target_bone = target;
        gxVectUVV3ToV3(
            &bone->target_vector,
            &bone->bone->parent_matrix->pos_vec,
            &target->bone->parent_matrix->pos_vec);
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
    cloth_bone_set_target(source, target);
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

static inline MkObj* start_axis_indicator(
    MkObj* target, int bone_index, float scale, MkProcEntryFn track) {
    AxisPdata* pdata;
    MkProc* proc;
    MkObj* axis;

    axis = load_model_from_slot(0, 0x1000C, 0x5001);
    if (axis != 0) {
        axis->pos.value.z = 0.0f;
        axis->pos.value.y = 0.025f;
        axis->flags_08_bits.bit7 = 1;
        if (track != 0) {
            axis->hide_flag_bits.hidden = 1;
        }
        axis->light_flags = 4;
        insert_fgnd_mkobj(axis);

        if (track != 0) {
            proc = _create_mkproc_generic_nostack(
                0x5004, 0x17, track, sizeof(AxisPdata), (MkHdr**)&pdata);
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
        }
    }

    if (axis != 0) {
        if (scale == 1.0f) {
            axis->flags_08_bits.scale_active = 0;
        } else {
            axis->flags_08_bits.scale_active = 1;
            axis->scale.x = axis->scale.y = axis->scale.z = scale;
        }
    }
    return axis;
}

void mks_start_axis_indicator_p_axis_track_bone_world_mat(
    int bone_tag,
    float scale) {
    int bone_index;

    bone_index = obj_get_bid_for_tid(plyr_obj, bone_tag);
    if (bone_index < 0) {
        return;
    }
    start_axis_indicator(plyr_obj, bone_index, scale, p_axis_track_bone_world_mat);
}

void mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(
    int bone_index_0, float weight_0, int bone_index_1, float weight_1,
    int bone_index_2, float weight_2, int bone_index_3, float weight_3) {
    ClothCollisionPlane* plane;
    MkObj* object;

    object = plyr_obj;
    plane = (ClothCollisionPlane*)get_mkhdr(
        &vtbl_cloth_coll_plane, sizeof(*plane));
    if (plane != 0) {
        plane->bone_count = 0;
        plane->reference_bone = 0;
        plane->flags_6C_bits.flags_80 = 1;
        plane->flags_6C_bits.flags_40 = 1;
        plane->reference_bones[0] = object->bones[bone_index_0];
        plane->reference_bones[0]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[1] = object->bones[bone_index_1];
        plane->reference_bones[1]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[2] = object->bones[bone_index_2];
        plane->reference_bones[2]->flags_54_bits.calculation_locked = 1;
        plane->reference_bones[3] = object->bones[bone_index_3];
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

static inline void cloth_plane_add_bone(ClothCollisionPlane* plane, ClothBone* bone) {
    unsigned int index;

    if (plane == 0) {
        return;
    }
    if (bone == 0) {
        return;
    }
    index = plane->bone_count;
    if (index < 12) {
        plane->bone_count = index + 1;
        plane->bones[index] = bone;
    }
}

void mks_ccp1_insert_cb1(void) {
    cloth_plane_add_bone(mks_ccp1, mks_cb1);
}

void mks_cc1_expand_cyl(float top, float bottom) {
    if (mks_cc1 != 0) {
        mks_cc1->cylinder_bottom -= bottom;
        mks_cc1->cylinder_top += top;
    }
}

static inline void cloth_volume_add_bone(ClothCollisionVolume* volume, ClothBone* bone) {
    unsigned int index;

    if (volume == 0) {
        return;
    }
    if (bone == 0) {
        return;
    }
    index = volume->bone_count;
    if (index < 12) {
        volume->bone_count = index + 1;
        volume->bones[index] = bone;
        volume->bone_settings[volume->bone_count] = 0;
    }
}

void mks_cc1_insert_cb1(void) {
    cloth_volume_add_bone(mks_cc1, mks_cb1);
}

/* TODO: [near miss] 97.55102%; only independent constant/address preparation order differs; retail allocation, flags and normalization agree; stop at scheduling */
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
        plane->flags_storage = 0;
        plane->distance = distance;
        plane->normal.x = normal_x;
        plane->normal.y = normal_y;
        plane->normal.z = normal_z;
        PSVECNormalize(&plane->normal, &plane->normal);
        mk_insert((MkHdr*)plane, &object->list_80);
    }
    mks_ccp1 = plane;
}

static inline ClothCollisionVolume* create_cloth_coll_on_obj(
    MkObj* object, int bone_index, float radius) {
    MkBone* bone;
    ClothCollisionVolume* volume;

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
        volume->reference_bone->flags_54_bits.calculation_locked = 1;
        if (volume->reference_bone->transform_parent != 0) {
            volume->reference_bone->transform_parent->flags_54_bits.calculation_locked = 1;
        }
    }
    if (volume != 0) {
        mk_insert((MkHdr*)volume, &object->list_80);
    }
    return volume;
}

void mks_cc1_eq_insert_cloth_coll(int bone_index, float radius) {
    mks_cc1 = create_cloth_coll_on_obj(plyr_obj, bone_index, radius);
}

void mks_cb1_set_scale(
    int include_children, float scale_x, float scale_y, float scale_z) {
    MkBone* bone;
    ClothBone* link;
    float inverse_length;

    if (mks_cb1 != 0) {
        bone = mks_cb1->bone;
        while (bone != 0) {
            bone->flags_55_bits.scale_controlled = 1;
            bone->scale.x = scale_x;
            bone->scale.y = scale_y;
            bone->scale.z = scale_z;
            link = bone->cloth_link;
            if (link != 0) {
                link->local_cloth_position.x *= scale_x;
                link->local_cloth_position.y *= scale_y;
                link->local_cloth_position.z *= scale_z;
                link->rest_length = length_v3(&link->local_cloth_position);
                inverse_length = 1.0f / link->rest_length;
                link->target_vector.x =
                    link->local_cloth_position.x * inverse_length;
                link->target_vector.y =
                    link->local_cloth_position.y * inverse_length;
                link->target_vector.z =
                    link->local_cloth_position.z * inverse_length;
            }
            if (include_children != 0) {
                bone = bone->tree_child;
            } else {
                bone = 0;
            }
        }
    }
}

void mks_set_cb1_target_bone_cb2(void) {
    cloth_bone_set_target(mks_cb1, mks_cb2);
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

static inline void insert_cloth_force(
    MkObj* obj, ClothBone* first, ClothBone* second,
    float rest_length, float stiffness) {
    ClothForcePdata* pdata;

    if (first == 0) {
        pdata = 0;
    } else if (second == 0) {
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

void mks_insert_cloth_force_bones(float rest_length, float stiffness) {
    insert_cloth_force(plyr_obj, mks_cb1, mks_cb2, rest_length, stiffness);
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

void mks_cloth_bones_init_by_tbl(ClothInitEntry* table, int flags) {
    cloth_bones_init_by_tbl(plyr_obj, table, flags);
}

/* TODO: [near miss] 98.50%; retail stores base_wind_v3 from the raw double arguments and wind_v3 from frsp copies; ours reuses the frsp values for both. */
void mks_bgnd_start_wind(double x, double y, double z) {
    ClothWindPdata* pdata;
    MkProc* proc;

    base_wind_v3.x = x;
    base_wind_v3.y = y;
    base_wind_v3.z = z;
    wind_v3.x = x;
    wind_v3.y = y;
    wind_v3.z = z;

    proc = _create_mkproc_generic_nostack(
        0x500D,
        0x2B,
        p_wind,
        sizeof(ClothWindPdata),
        (MkHdr**)&pdata);
    if (proc != 0) {
        if (g_game_info.bgnd_obj != 0) {
            mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
        }
        if (pdata != 0) {
            zero_pdata_payload(sizeof(ClothWindPdata), &pdata->hdr);
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
    MKVECTOR difference;
    MKVECTOR direction;
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
    if (distance) {
        inverse_distance = 1.0f / distance;
    } else {
        inverse_distance = 1.0f;
    }
    PSVECScale(&difference, &direction, inverse_distance);

    if (distance < force->rest_length) {
        Vec second_compression;
        Vec first_compression;

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
        Vec second_extension;
        Vec first_extension;

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

/* TODO: [near miss] 94.27%; cloth_coll_bone_uv is a pt_displacement_v member alias (TU data layout: retail has a separate static, no CSE of its address), deferred-return branch and volume register remain. */
static void do_cloth_colls(MkHdr* collision) {
    ClothCollisionVolume* volume;
    ClothCollisionPlane* plane;
    MkBone* reference;
    MKVECTOR plane_offset;
    MKVECTOR plane_point;
    MKVECTOR world_normal;
    RwMatrixPosition point_0 __attribute__((aligned(16)));
    RwMatrixPosition point_1 __attribute__((aligned(16)));
    MKVECTOR center;
    MKVECTOR edge_1;
    MKVECTOR edge_0;
    MKVECTOR adjustment;
    MKVECTOR offset;
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
        reference = cloth_coll->reference_bone;
        cloth_coll_bone = reference;
        if (!cloth_coll_bone->flags_55_bits.collision_disabled) {
            if (cloth_coll_bone->flags_55_bits.collision_deferred) {
                return;
            }
            cloth_coll_radius_sq = cloth_coll->radius;
            cloth_coll_radius_sq *= cloth_coll_radius_sq;
            PSVECSubtract(
                &cloth_coll_bone->matrix.pos_vec,
                &cloth_coll_bone->transform_parent->matrix.pos_vec,
                &cloth_coll_bone_uv);
            PSVECScale(
                &cloth_coll_bone_uv, &cloth_coll_bone_uv,
                cloth_coll_bone->field_5C);
            RwMatrixInvert(
                &cloth_coll_bone_parent_world_mat_inv,
                &cloth_coll_bone->transform_parent->matrix);
            cloth_bone = cloth_obj->cloth_bones;
            for (index = 0; index < cloth_coll->bone_count; index++) {
                cloth_bone = cloth_coll->bones[index];
                coll_cnt = (int*)&cloth_coll->bone_settings[index];
                if (cloth_bone->active != 0) {
                    cloth_coll->collision_fn();
                }
            }
        }
    } else {
        if (collision->vtbl == &vtbl_cloth_coll_plane) {
            plane = (ClothCollisionPlane*)collision;
        } else {
            plane = 0;
        }
        if (plane != 0) {
            if (!plane->flags_6C_bits.flags_80) {
                reference = plane->reference_bone;
                cloth_coll_bone = reference;
                if (!cloth_coll_bone->flags_55_bits.collision_disabled) {
                    if (cloth_coll_bone->flags_55_bits.collision_deferred) {
                        return;
                    }
                    gxMat33Tx31(
                        &world_normal, &plane->normal,
                        (Mat33*)&cloth_coll_bone->matrix);
                    PSVECScale(
                        &plane->normal, &plane_offset, plane->distance);
                    gxMatV3MatAddV3(
                        &plane_point, &plane_offset,
                        (Mat33*)&cloth_coll_bone->matrix,
                        &cloth_coll_bone->matrix.pos_vec);
                    plane_distance =
                        PSVECDotProduct(&world_normal, &plane_point);
                    for (index = 0; index < plane->bone_count; index++) {
                        cloth_bone = plane->bones[index];
                        point_distance = PSVECDotProduct(
                            &world_normal, &cloth_bone->force_position);
                        if (point_distance < plane_distance) {
                            Vec* position = &cloth_bone->force_position;
                            Vec push;

                            PSVECScale(
                                &world_normal, &push,
                                plane_distance - point_distance);
                            PSVECAdd(position, &push, position);
                        }
                    }
                }
            } else {
                cloth_coll_bone = 0;
                if (plane->flags_6C_bits.flags_40) {
                    point_0 = plane->reference_bones[0]->matrix.pos_row;
                    point_1 = plane->reference_bones[1]->matrix.pos_row;
                    PSVECSubtract(
                        &plane->reference_bones[3]->matrix.pos_vec,
                        &plane->reference_bones[2]->matrix.pos_vec,
                        &center);
                    PSVECScale(&center, &center, 0.5f);
                    PSVECAdd(
                        &center,
                        &plane->reference_bones[2]->matrix.pos_vec,
                        &center);
                    PSVECSubtract(&point_0.value, &center, &edge_1);
                    PSVECSubtract(&point_1.value, &center, &edge_0);
                    PSVECCrossProduct(&edge_0, &edge_1, &plane->normal);
                    PSVECNormalize(&plane->normal, &plane->normal);
                    PSVECScale(
                        &plane->normal, &adjustment, plane->weights[0]);
                    PSVECAdd(&point_0.value, &adjustment, &point_0.value);
                    PSVECScale(
                        &plane->normal, &adjustment, plane->weights[1]);
                    PSVECAdd(&point_1.value, &adjustment, &point_1.value);
                    PSVECScale(
                        &plane->normal, &adjustment,
                        plane->weights[2] +
                            0.5f * (plane->weights[3] - plane->weights[2]));
                    PSVECAdd(&center, &adjustment, &center);
                    PSVECSubtract(&point_0.value, &center, &edge_1);
                    PSVECSubtract(&point_1.value, &center, &edge_0);
                    PSVECCrossProduct(&edge_0, &edge_1, &plane->normal);
                    PSVECNormalize(&plane->normal, &plane->normal);
                }
                for (index = 0; index < plane->bone_count; index++) {
                    cloth_bone = plane->bones[index];
                    PSVECSubtract(
                        &cloth_bone->force_position, &point_0.value, &offset);
                    point_distance =
                        PSVECDotProduct(&offset, &plane->normal);
                    if (point_distance < 0.0f) {
                        Vec* position = &cloth_bone->force_position;
                        Vec push;

                        PSVECScale(&plane->normal, &push, -point_distance);
                        PSVECAdd(position, &push, position);
                    }
                }
            }
        } else {
            ClothCollisionVolume* unused_volume;

            if (collision->vtbl == &vtbl_cloth_coll_volume) {
                unused_volume = (ClothCollisionVolume*)collision;
            } else {
                unused_volume = 0;
            }
            if (unused_volume != 0) {
            }
        }
    }
}

/* TODO: [near miss] 93.20%; axis/radial temps are 16-byte rows; frame is 0x90 vs retail 0xa0 and the scratch base is CSE'd differently (retail keeps only r31). */
static void cloth_coll_point_cyl_inside(void) {
    ClothCollisionScratch* scratch;
    Vec* force_position;
    MKVECTOR world_point;
    RwMatrixPosition radial_direction_0;
    RwMatrixPosition axis_point_0;
    RwMatrixPosition radial_direction_1;
    RwMatrixPosition axis_point_1;
    Vec axis_offset_0;
    Vec axis_offset_1;
    int index;
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
        {
            Vec* origin = &cloth_coll_bone->transform_parent->matrix.pos_vec;
            PSVECScale(&scratch->bone_axis, &axis_offset_0, along_axis);
            PSVECAdd(origin, &axis_offset_0, &axis_point_0.value);
        }
        gxVectUVV3ToV3(
            &radial_direction_0.value, &axis_point_0.value,
            force_position);
        PSVECScale(
            &radial_direction_0.value, &radial_direction_0.value,
            cloth_coll->radius);
        PSVECAdd(&axis_point_0.value, &radial_direction_0.value, &axis_point_0.value);
        PSVECSubtract(
            &axis_point_0.value, force_position,
            &scratch->displacement);
        collided = 1;
    } else {
        collided = 0;
    }
    if (collided) {
        PSVECAdd(
            &cloth_bone->force_position, &scratch->displacement,
            &cloth_bone->force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (Mat33*)&cloth_bone->bone->matrix,
            &cloth_bone->force_position);
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
            {
                Vec* origin = &cloth_coll_bone->transform_parent->matrix.pos_vec;
                PSVECScale(&scratch->bone_axis, &axis_offset_1, along_axis);
                PSVECAdd(origin, &axis_offset_1, &axis_point_1.value);
            }
            gxVectUVV3ToV3(
                &radial_direction_1.value, &axis_point_1.value, &world_point);
            PSVECScale(
                &radial_direction_1.value, &radial_direction_1.value,
                cloth_coll->radius);
            PSVECAdd(
                &axis_point_1.value, &radial_direction_1.value, &axis_point_1.value);
            PSVECSubtract(
                &axis_point_1.value, &world_point, &scratch->displacement);
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
    RwMatrixPosition world_position;
    RwMatrixPosition target_position;
    RwMatrixPosition direction;
    RwMatrixPosition displacement;
    float along_axis;
    float previous_y;

    if (point->x * point->x + point->z * point->z <
            cloth_coll_radius_sq &&
        (along_axis = -point->y) > cloth_coll->cylinder_bottom &&
        along_axis <= cloth_coll->cylinder_top) {
        PSVECSubtract(
            point, &cloth_bone->previous_collision_local_position,
            &displacement.value);
        PSVECScale(&displacement.value, &direction.value, 0.5f);
        PSVECAdd(
            &direction.value, &cloth_bone->previous_collision_local_position,
            &direction.value);
        direction.value.y = 0.0f;
        PSVECAdd(&direction.value, &cloth_bone->collision_offset, &direction.value);
        PSVECNormalize(&direction.value, &direction.value);
        previous_y = cloth_bone->previous_collision_local_position.y;
        PSVECScale(
            &direction.value, &target_position.value,
            cloth_coll->radius + 0.01f);
        target_position.value.y = previous_y;
        PSVECSubtract(&target_position.value, point, &displacement.value);
        PSVECAdd(
            &cloth_bone->collision_local_position, &displacement.value,
            &cloth_bone->collision_local_position);
        gxMat33Tx31(
            &displacement.value, &cloth_bone->collision_local_position,
            (Mat33*)&cloth_coll_bone->transform_parent->matrix);
        PSVECAdd(
            &displacement.value,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &world_position.value);
        PSVECSubtract(
            &world_position.value, &cloth_bone->force_position,
            &cloth_displacement_v);
        return 1;
    }
    return 0;
}

static void cloth_coll_vector_cyl(void) {
    MKVECTOR world_point;
    RwMatrixPosition local_position __attribute__((aligned(16)));
    MKVECTOR object_offset;
    int index;
    float position_weight;

    cloth_bone->previous_collision_local_position_row =
        cloth_bone->collision_local_position_row;
    PSVECSubtract(
        &cloth_bone->force_position,
        &cloth_coll_bone->transform_parent->matrix.pos_vec,
        &object_offset);
    gxMat33Tx31(
        &local_position.value, &object_offset,
        (Mat33*)&cloth_coll_bone_parent_world_mat_inv);
    cloth_bone->collision_local_position_row = local_position;
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
            (Mat33*)&cloth_bone->bone->matrix,
            &cloth_bone->force_position);
        PSVECSubtract(
            &world_point,
            &cloth_coll_bone->transform_parent->matrix.pos_vec,
            &object_offset);
        gxMat33Tx31(
            &world_point, &object_offset,
            (Mat33*)&cloth_coll_bone_parent_world_mat_inv);
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

/* TODO: [near miss] 88.51257%; Retail predicates, signed loop bounds and owner reloads agree; remaining 16-byte automatic vector stack, scratch-base materialization and register allocation. Stop pending recovered aligned type. */
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
    int index;
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
        {
            Vec* origin = &cloth_coll_bone->transform_parent->matrix.pos_vec;
            PSVECScale(&scratch->bone_axis, &axis_offset_0, along_axis);
            PSVECAdd(origin, &axis_offset_0, &axis_point_0);
        }
        gxMatV3MatAddV3(
            &offset_point_0, &cloth_bone->collision_offset,
            (Mat33*)&cloth_bone->bone->matrix,
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
            &cloth_bone->force_position, &scratch->displacement,
            &cloth_bone->force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (Mat33*)&cloth_bone->bone->matrix,
            &cloth_bone->force_position);
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
            {
                Vec* origin = &cloth_coll_bone->transform_parent->matrix.pos_vec;
                PSVECScale(&scratch->bone_axis, &axis_offset_1, along_axis);
                PSVECAdd(origin, &axis_offset_1, &axis_point_1);
            }
            gxMatV3MatAddV3(
                &offset_point_1, &cloth_bone->collision_offset,
                (Mat33*)&cloth_bone->bone->matrix, &world_point);
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
                &cloth_bone->force_position, &scratch->displacement,
                &cloth_bone->force_position);
            coll_cnt++;
        }
    }
}

/* TODO: [near miss] 94.13%; aligned vector slots match retail; retail keeps only the scratch base in r31 (no CSE of its field addresses) and force_position in r29. */
static void cloth_coll_point_cyl_rel(void) {
    ClothCollisionScratch* scratch;
    Vec* force_position;
    MKVECTOR world_point;
    MKVECTOR offset_point_0;
    MKVECTOR radial_direction_0;
    MKVECTOR axis_point_0;
    MKVECTOR offset_point_1;
    MKVECTOR radial_direction_1;
    MKVECTOR axis_point_1;
    int index;
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
            (Mat33*)&cloth_bone->bone->matrix,
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
            &cloth_bone->force_position, &scratch->displacement,
            &cloth_bone->force_position);
        coll_cnt++;
    }
    for (index = 0; index < cloth_bone->collision_point_count; index++) {
        gxMatV3MatAddV3(
            &world_point,
            &cloth_bone->collision_points[index].position,
            (Mat33*)&cloth_bone->bone->matrix,
            &cloth_bone->force_position);
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
                (Mat33*)&cloth_bone->bone->matrix, &world_point);
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
                &cloth_bone->force_position, &scratch->displacement,
                &cloth_bone->force_position);
            coll_cnt++;
        }
    }
}

static void set_cloth_pos(ClothBone* bone) {
    MkBone* render_bone;
    MkBone* parent_bone;
    MKMATRIX inverse_parent;
    MKVECTOR direction;
    MKVECTOR normalized_direction;
    MKVECTOR object_position;
    MKVECTOR target_direction;
    Quat rotation;
    Vec zero_scale;
    float interpolation;

    render_bone = bone->bone;
    if (!render_bone->flags_55_bits.collision_deferred &&
        render_bone->parent_matrix != 0 &&
        render_bone->flags_54_bits.hierarchy_driven) {
        parent_bone = render_bone->transform_parent;
        PSVECSubtract(
            &bone->force_position, &render_bone->matrix.pos_vec,
            &bone->velocity);
        render_bone->matrix.pos_row = bone->force_position_row;
        PSVECSubtract(
            &render_bone->matrix.pos_vec, &cloth_obj_mat->pos_vec,
            &object_position);
        gxMat33Tx31(
            &render_bone->parent_matrix->pos_vec, &object_position,
            (Mat33*)&cloth_obj_mat_inv);

        if (bone->target_bone != 0) {
            PSVECSubtract(
                &bone->target_bone->force_position,
                &bone->force_position, &object_position);
            gxMat33Tx31(
                &target_direction, &object_position,
                (Mat33*)&cloth_obj_mat_inv);
            RwMatrixInvert(&inverse_parent, parent_bone->parent_matrix);
            gxMat33Tx31(
                &object_position, &target_direction,
                (Mat33*)&inverse_parent);
            PSVECNormalize(&object_position, &target_direction);
            gxVectV3V3ToQuat(
                &rotation, &bone->target_vector, &target_direction);
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
            zero_scale.x = zero_scale.y = zero_scale.z = 0.0f;
            inverse_parent.right.x = inverse_parent.up.y =
                inverse_parent.at.z = 1.0f;
            inverse_parent.right.y = inverse_parent.right.z =
                inverse_parent.up.x = 0.0f;
            inverse_parent.up.z = inverse_parent.at.x =
                inverse_parent.at.y = 0.0f;
            inverse_parent.pos.x = inverse_parent.pos.y =
                inverse_parent.pos.z = 0.0f;
            inverse_parent.flags |= 0x20003;
            RwMatrixScale(&inverse_parent, (const RwV3d*)&zero_scale, 0);
        } else {
            RwMatrixInvert(&inverse_parent, parent_bone->parent_matrix);
        }
        gxMat33Tx31(&direction, &object_position, (Mat33*)&inverse_parent);
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
    MKVECTOR difference;
    MKVECTOR direction;
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
            if (distance) {
                inverse_distance = 1.0f / distance;
            } else {
                inverse_distance = 1.0f;
            }
            PSVECScale(&difference, &direction, inverse_distance);
            if (distance < bone->rest_length) {
                Vec parent_push;
                Vec push;

                correction =
                    -(0.1f * bone->stretch_weight - 1.0f) *
                    (bone->rest_length - distance);
                bone->current_length = distance + correction;
                if (parent_cloth != 0 && parent_cloth->stretch_weight) {
                    parent_adjustment =
                        correction * parent_cloth->stretch_weight /
                        (bone->stretch_weight +
                         parent_cloth->stretch_weight);
                    PSVECScale(&direction, &parent_push, -parent_adjustment);
                    PSVECAdd(
                        &parent_cloth->force_position, &parent_push,
                        &parent_cloth->force_position);
                } else {
                    parent_adjustment = 0.0f;
                }
                PSVECScale(&direction, &push, correction - parent_adjustment);
                PSVECAdd(&bone->force_position, &push, &bone->force_position);
            } else {
                Vec parent_pull;
                Vec pull;

                correction =
                    -(0.1f * bone->stretch_weight - 1.0f) *
                    (distance - bone->rest_length);
                bone->current_length = distance - correction;
                if (parent_cloth != 0 && parent_cloth->stretch_weight) {
                    parent_adjustment =
                        correction * parent_cloth->stretch_weight /
                        (bone->stretch_weight +
                         parent_cloth->stretch_weight);
                    PSVECScale(&direction, &parent_pull, parent_adjustment);
                    PSVECAdd(
                        &parent_cloth->force_position, &parent_pull,
                        &parent_cloth->force_position);
                } else {
                    parent_adjustment = 0.0f;
                }
                PSVECScale(&direction, &pull, parent_adjustment - correction);
                PSVECAdd(&bone->force_position, &pull, &bone->force_position);
            }
        }
    }
}

/* TODO: [near miss] 99.84%; only the grandparent/cloth-link volatile register pair (r4/r3 vs retail r3/r4) differs. */
static void calc_cloth_dwp(ClothBone* bone) {
    MkBone* render_bone;
    MkBone* parent_bone;
    MkBone* grandparent_bone;
    ClothBone* parent_cloth;
    const RwMatrix* target_matrix;
    const Vec* target_origin;
    MKVECTOR velocity;
    MKVECTOR adjustment;
    MKVECTOR transformed_normal;
    MKVECTOR spring_velocity;
    float collision_scale;
    float spring_scale;
    float wind_scale;

    render_bone = bone->bone;
    if (!render_bone->flags_55_bits.collision_deferred &&
        render_bone->parent_matrix != 0 &&
        render_bone->flags_54_bits.hierarchy_driven) {
        parent_bone = render_bone->transform_parent;
        PSVECScale(
            &bone->velocity, &velocity,
            -(((1.0f - bone->segment_length) * sqrt_game_speed) - 1.0f));
        if (bone->collision_amount) {
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
            (Mat33*)target_matrix, (Vec*)target_origin);
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

        if (bone->initial_z) {
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
            wind_scale =
                PSVECDotProduct(&adjustment, &transformed_normal);
            if (wind_scale < 0.0f) {
                wind_scale *= -1.0f;
            }
            wind_scale *= bone->initial_z;
            wind_scale += bone->initial_x;
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
    unsigned int cloth_index;
    unsigned int bone_index;
    unsigned int cloth_count;
    float rest_length;

    if (obj->pos.value.x != obj->pos.value.x) {
        obj->pos.value.x = 0.0f;
    }
    if (obj->pos.value.y != obj->pos.value.y) {
        obj->pos.value.y = 0.0f;
    }
    if (obj->pos.value.z != obj->pos.value.z) {
        obj->pos.value.z = 0.0f;
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
                while ((parent_bone = render_bone->transform_parent) != 0 &&
                       parent_bone->flags_54_bits.cloth_candidate &&
                       parent_bone->cloth_link == 0) {
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
                bone->local_cloth_position = bone->bone->translation.value;
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

/* TODO: [near miss] 98.68%; indexed rows and mirror_a compares match; inlined bone-search registers (r12/r11 vs r31/r12) differ. */
void cloth_bones_init_by_tbl(
    MkObj* object, ClothInitEntry* table, int count) {
    ClothBone* cloth_bone;
    MkBone* bone;
    int index;

    for (index = 0; index < count; index++) {
        cloth_bone = find_cloth_bone_by_tag(object, table[index].bone_tag);
        if (cloth_bone != 0) {
            cloth_bone->active = 1;
            bone = cloth_bone->bone;
            bone->flags_54_bits.hierarchy_driven = 1;
            if (object == g_game_info.plyr0.slot.mirror_a ||
                object == g_game_info.plyr1.slot.mirror_a) {
                if (table[index].initial_x != 0.0f) {
                    table[index].initial_x = 0.0f;
                }
                if (table[index].initial_z != 0.0f) {
                    table[index].initial_z = 0.0f;
                }
            }
            cloth_bone->stiffness_squared =
                table[index].stiffness * table[index].stiffness;
            cloth_bone->table_weight = table[index].segment_length;
            cloth_bone->segment_length = cloth_sqrt(table[index].segment_length);
            cloth_bone->force_step = -table[index].force / 50.0f;
            cloth_bone->stretch_weight = 0.0f;
            cloth_bone->damping_factor =
                1.0f - table[index].damping * table[index].damping;
            cloth_bone->initial_x = table[index].initial_x;
            cloth_bone->initial_z = table[index].initial_z;
            cloth_bone->table_scale = table[index].table_scale;
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
            (Mat33*)matrix, (Mat33*)matrix, &axis_obj->scale);
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

static inline MkObj* axis_pdata_live_axis(AxisPdata* pdata) {
    MkObj* object = pdata->axis;

    if (object != 0) {
        if (object->hdr.instance == pdata->axis_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

static void pw_axis(void) {
    axis_pdata = (AxisPdata*)apdata;
    if (axis_pdata != 0) {
        axis_obj = axis_pdata_live_axis(axis_pdata);
        if (axis_obj == 0) {
            mkproc_die();
        }
    }
}
