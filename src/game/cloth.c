#include "game/cloth.h"
#include "game/game_info.h"
#include "math/mk_math.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"

static float p_cloth(void);
static float p_wind(void);
static float p_wind_lp(void);
static void cloth_bones_init_by_tbl(MkObj* obj, int table_id, int flags);
static void start_cloth_bones(MkObj* obj);
static float p_axis_track_bone_world_mat(void);
static void pw_axis(void);
static void ps_axis(void);
void mkobj_update_cloth(MkHdr* hdr);
void mkobj_update_weapon_trail(MkHdr* hdr);
RpMaterial* obj_find_material_by_id(MkObj* obj, int material_id);
void material_set_zbias(RpMaterial* material, float zbias);

extern float cloth_ground_plane;
extern MkPtr* cloth_mkobj_list;
extern MkPtr* weapon_trail_mkobj_list;
extern MkObj* g_bgnd_preloaded_models[15];
extern MkObj* plyr_obj;
extern ClothBone* mks_cb1;
extern ClothBone* mks_cb2;
extern ClothCollisionPlane* mks_ccp1;
extern ClothCollisionVolume* mks_cc1;

typedef struct ClothForcePdata {
    MkHdr hdr;
    ClothBone* first;
    ClothBone* second;
    float first_weight;
    float second_weight;
} ClothForcePdata;

typedef struct AxisPdata {
    MkHdr hdr;
    MkObj* axis;
    unsigned int axis_instance;
    MkObj* target;
    unsigned int target_instance;
    int state;
    int bone_index;
} AxisPdata;

typedef struct AxisTargetLatch {
    MkObj* object;
    unsigned int instance;
} AxisTargetLatch;

extern AxisPdata* axis_pdata;
extern MkObj* axis_obj;
extern MkSobj* axis_sobj;
extern AxisTargetLatch target_obj_item;
extern void cloth_coll_vector_cyl(void);
extern void cloth_coll_point_cyl_abs(void);
extern void cloth_coll_point_cyl_inside(void);
int obj_get_bid_for_tid(MkObj* obj, int tag);
MkObj* load_model_from_slot(int slot, int model_id, int heap_id);

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

typedef float (*ClothJumpSleepFn)(
    MkProcEntryFn entry,
    MkVtableMkproc* vtable,
    float sleep_ticks);

typedef struct ClothProcVtable {
    void* slots[9];
    ClothJumpSleepFn jump_sleep;
} ClothProcVtable;

Vec wind_v3;
Vec base_wind_v3;

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

void mks_obj_enable_update_cloth(MkObj* obj, int enabled) {
    obj->flags_0C_bits.cloth_update = enabled;
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

void mks_debug_display_cloth_ontop(int enabled) {
    (void)enabled;
}

void mks_debug_display_cloth_coll_plane(void) {
}

void mks_debug_display_cloth_coll_cyl(void) {
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

void mks_npc_cloth_bones_init_by_tbl(
    int model_index,
    int table_id,
    int flags) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj != 0) {
        cloth_bones_init_by_tbl(obj, table_id, flags);
    }
}

void mks_npc_start_cloth_bones(int model_index) {
    MkObj* obj;

    obj = g_bgnd_preloaded_models[model_index];
    if (obj != 0) {
        start_cloth_bones(obj);
    }
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

void mks_cc1_expand_cyl(float top, float bottom) {
    if (mks_cc1 != 0) {
        mks_cc1->cylinder_bottom -= bottom;
        mks_cc1->cylinder_top += top;
    }
}


/*
 * Soft ceiling: ~97% -- exact control flow and size; only the three volatile
 * locals rotate between r4-r6.
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

void mks_cb2_eq_cloth_bone(int bone_id) {
    mks_cb2 = find_cloth_bone_by_tag(plyr_obj, bone_id);
}

void mks_cb1_eq_cloth_bone(int bone_id) {
    mks_cb1 = find_cloth_bone_by_tag(plyr_obj, bone_id);
}

void mks_insert_cloth_force_bones(float first_weight, float second_weight) {
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
            pdata->first_weight = first_weight;
            pdata->second_weight = second_weight;
        }
    }
    if (pdata != 0) {
        mk_insert((MkHdr*)pdata, &obj->list_7C);
    }
}

void mks_mat_id_set_zbias(int material_id, float zbias) {
    RpMaterial* material;

    material = obj_find_material_by_id(plyr_obj, material_id);
    if (material != 0) {
        material_set_zbias(material, zbias);
    }
}

void mks_cloth_bones_init_by_tbl(int table_id, int flags) {
    cloth_bones_init_by_tbl(plyr_obj, table_id, flags);
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

int find_cloth_bone_id_from_tag(MkObj* obj, int tag) {
    ClothBone* bone;

    bone = find_cloth_bone_by_tag(obj, tag);
    if (bone != 0) {
        return bone->bone->bone_index;
    }
    return -1;
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
