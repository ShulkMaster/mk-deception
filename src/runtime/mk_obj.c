#include "runtime/mk_obj.h"
#include "runtime/light.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"
#include "game/game_info.h"
#include "game/cloth.h"
#include "game/specular.h"
#include "runtime/asset.h"
#include "math/gxMat.h"
#include "math/gxMath.h"
#include "math/gxQuat.h"
#include "math/mk_math.h"
#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "rw/rpmatfx_types.h"
#include "rw/rplight.h"
#include "rw/rtquat.h"
#include "rw/rwframe.h"

typedef RwFrame* (*RwFrameCallBack)(RwFrame* frame, void* data);
typedef RwObject* (*RwObjectCallBack)(RwObject* object, void* data);
typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial* material, void* data);
typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);

/* RwMatrix begins with the padded 3x3 Mat33 rotation block. */
#define RW_MATRIX_MAT33(matrix_) ((Mat33*)(matrix_))
#define RW_OBJECT_FROM_FRAME_LINK(link_)                                      \
    ((RwObject*)((unsigned char*)(link_) - sizeof(RwObject)))

typedef struct SobjCreateData {
    MkObj* obj;
    unsigned int id;
    unsigned int mask;
    MkSobj* result;
    int set_priority;
    int priority;
} SobjCreateData;

typedef struct RwEngineFreeView {
    char pad00[0x138];
    void (*free)(void* memory);
} RwEngineFreeView;

typedef struct LimbBonePdata {
    MkHdr hdr;
    MkObj* obj;
    unsigned int obj_instance;
    int bone;
} LimbBonePdata;

typedef struct LightMkList {
    unsigned int mask;
    MkPtr** list;
    int state;
} LightMkList;

typedef struct HotAmbientLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    float color[4];
} HotAmbientLightDef;

typedef struct FadeMaterialPdata {
    MkHdr hdr;
    MkObj* obj;
    unsigned int obj_instance;
    RpMaterial* material;
    float delta;
    int frames;
    float accumulator;
} FadeMaterialPdata;

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

typedef struct LimbSet {
    char pad00[0x780];
    unsigned int moved_bones; /* +0x780 */
} LimbSet;

typedef struct HeadTrackingPdata {
    MkHdr hdr;
    MkObj* obj;
    PlyrPdata* target;
    float angle_x;
    float angle_y;
    float field_18;
    float blend_weight;
} HeadTrackingPdata;

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

typedef struct ShadowBonePair {
    int source_bone;
    int shadow_bone;
} ShadowBonePair;

typedef struct ShadowBoneMap {
    int count;
    ShadowBonePair* pairs;
} ShadowBoneMap;

typedef struct ShadowObjPair {
    MkObj* source;
    unsigned int source_instance;
    MkObj* shadow;
    unsigned int shadow_instance;
} ShadowObjPair;

typedef struct ShadowExtras {
    ShadowObjPair first;
    char pad10[8];
    ShadowObjPair second;
} ShadowExtras;

typedef struct ShadowController {
    char pad00[0x40];
    ShadowObjPair third;
    char pad50[0x184];
    ShadowBoneMap* bone_map;
    char pad1D8[0x138];
    ShadowExtras* extras;
    char pad314[0x160];
    MkProc* proc;
} ShadowController;

typedef struct ShadowPdata {
    MkHdr hdr;
    MkObj* source;
    unsigned int source_instance;
    MkObj* shadow;
    unsigned int shadow_instance;
    char pad18[4];
    PlyrPdata* controller;
} ShadowPdata;

static float p_fade_material(void);
static float p_scale(void);
static float p_headtracking_die(void);
static float p_plyr_head_tracking(void);
static float p_goro_arms_fixup(void);
static float p_shadow_obj(void);
static void trackhead_postsleep(void);
static void trackhead_prewake(void);
void mks_start_goro_xtra_weapons(void);
void limb_sever_show_z_meat_chunks_all(MkObj* obj);
void limb_sever_hide_z_meat_chunks_all(MkObj* obj);
int get_player_number(void* obj);

extern int limb_meat_chunk_list[];
extern int* limb_meats_mat_id_tbl[];
extern int* limb_children_table[];

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack callback, void* data);
void* memcpy(void* dst, const void* src, unsigned int size);
int RpClumpDestroy(RpClump* clump);
RpAtomic* set_atomic_material_alpha(RpAtomic* atomic, unsigned int alpha);
RpAtomic* force_atomic_material_alpha(RpAtomic* atomic, void* alpha);
void pull_clump_from_world(RpClump* clump);
void mkobj_destroy_bones(MkObj* obj);
void free_mem(void* ptr);
int stricmp(const char* lhs, const char* rhs);
void vdestroy_mkx_rplight(MkxRpLight* light);
extern MkObj* plyr_obj;
extern MkVtable5 vtbl_mkx_mem;
extern MkVtable5 vtbl_mkx_rplight;
extern MkVtable5 vtbl_mkobj;
extern _mwMemHeap* mkobj_heap;
extern RpWorld* World;
static HeadTrackingPdata* pdata_headtracking;
static MkPtr* limb_bone_list;
unsigned int uploaded_light_state;
int skip_light_setup;
static unsigned int last_obj_light_flags;
static RpAtomic* atomic1;
static unsigned int oid_to_kill_mask;
static unsigned int oid_to_kill;
MkPtr* ground_me_mkobj_list;
MkPtr* bone_hierarchy_mkobj_list;
MkPtr* point_light_list;
MkPtr* gore2_light_list;
MkPtr* skinned_obj_light_list;
MkPtr* board_piece_light_list;
MkPtr* bgnd_spec_light_list;
MkPtr* hol_plight_list;
MkPtr* weapon_trail_light_list;
MkPtr* clone_light_list;
MkPtr* freeze_light_list;
MkPtr* special_light_list;
MkPtr* plyr_light_list;
MkPtr* fgnd_light_list;
MkPtr* bgnd_light_list;
MkPtr* particle_mkobj_list;
MkPtr* fgnd_mkobj_list;
float obj_game_speed;
extern int MksobjLocalOffset;
extern float camera_facing_matrix_ay[16];
extern float game_speed;
extern RwEngineFreeView* RwEngineInstance;
extern MkPtr* pfx_render_list;
extern MkPtr* pfx_clone_render_list;

static HotAmbientLightDef hot_ambient_light = {
    1, 0, 3, {1.0f, 1.0f, 1.0f, 1.0f}
};

LightMkList light_mklists[13] = {
    {0x0001, &bgnd_light_list, 2},
    {0x0002, &fgnd_light_list, 2},
    {0x0004, &plyr_light_list, 2},
    {0x0008, &special_light_list, 2},
    {0x0010, &weapon_trail_light_list, 2},
    {0x0020, &freeze_light_list, 2},
    {0x2000, &clone_light_list, 2},
    {0x0100, &bgnd_spec_light_list, 2},
    {0x0200, &board_piece_light_list, 2},
    {0x0400, &skinned_obj_light_list, 2},
    {0x0800, &gore2_light_list, 2},
    {0x1000, &point_light_list, 2},
    {0, 0, 0}
};

typedef struct GoroArmsFixupEntry {
    int source_bone;
    int target_bone;
} GoroArmsFixupEntry;

typedef struct GroundCollisionEntry {
    int bone;
    Vec offset;
    float radius;
} GroundCollisionEntry;

GoroArmsFixupEntry goro_arms_fixup_map[12] = {
    {0x0C, 0x43}, {0x0F, 0x44}, {0x12, 0x45}, {0x14, 0x46},
    {0x16, 0x47}, {0x18, 0x48}, {0x0E, 0x50}, {0x11, 0x51},
    {0x13, 0x52}, {0x15, 0x53}, {0x17, 0x54}, {0x19, 0x55}
};

extern int exec_tick_ctr;
extern int mode_of_play;
extern int limb_root_bids[15];

void update_camera_facing_matrix(void);
int build_bones_tbl(MkObj* obj, const int* tags);
void* ft_fake_bone_matcher(
    MkObj* parent, MkObj* child, int child_bone,
    const Vec* parent_offset, const Vec* child_offset,
    const Vec* rotation, int mode, float blend);

static float p_obj(void);
static float p_bone_hierarchy(void);
static void limb_bone_calc_world_pos(MkHdr* data);
static void set_bone_world_pos_xz(void* obj, int bone, void* pos);
void set_bone_world_pos(void* obj, int bone, void* pos);

void update_bone_hierarchy(void* obj);
void ground_me(void* obj);
void atomic_set_transl_flag(RpAtomic* atomic);
void render_mkobj(void* obj);
void get_bone_world_pos(MkObj* obj, int bone, Vec* out);
MkObj* get_mkobj_frame(int type, RwFrame* frame);
MkBone* alloc_bone(void);
void mkbone_remove(MkBone* bone);
void mkbone_insert_child_of_clone_parent(MkBone* bone, MkBone* parent);
void mkbone_insert_child_of_parent(MkBone* bone, MkBone* parent);
void PSVECAdd(const Vec* a, const Vec* b, Vec* dst);
void* find_pfx_by_name_by_bankowner(const char* name, unsigned int owner);
void reset_effect_ppfx(void* effect);
void pfx_spawn_at_bid(const char* name, void* obj, int bone);
void bone_make_parents_my_children(MkBone* bone);

static void rwframe_set_true_clip_flag_on_objects_and_children(
    RwFrame* frame, int flag);
static RwObject* rwobject_set_true_clip_flag(RwObject* object, unsigned char flag);
static RpAtomic* atomic_create_sobj_callback(
    RpAtomic* atomic, void* data);
static void* AtomicFaceCamera(void* atomic, void* data);
static MkSobj* rwframe_find_child_sobj_by_id(
    RwFrame* frame, unsigned int id, int depth);

static float normalize_obj_angle(float angle) {
    int fixed;

    fixed = (int)(angle * 166886.1f);
    fixed &= 0xFFFFF;
    return (float)fixed * 0.000005992112f;
}

/* NonMatching recovery for mk_obj.o (169 funcs, retail order). */

void* obj_find_child_sobj_by_id(void* obj, unsigned int id, int depth) {
    RwFrame* frame;
    RwFrame* next;
    MkSobj* result;

    frame = ((MkObj*)obj)->frame->child;
    while (frame != 0) {
        next = frame->next;
        result = rwframe_find_child_sobj_by_id(frame, id, depth);
        if (result != 0) {
            return result;
        }
        frame = next;
    }
    return 0;
}

#pragma inline_depth(2)
static MkSobj* rwframe_find_child_sobj_by_id(RwFrame* frame, unsigned int id,
                                             int depth) {
    RwLLLink* link;
    RwObject* object;
    MksobjPluginData* plugin;
    MkSobj* sobj;
    RwFrame* child;
    RwFrame* next;
    int child_depth;

    link = frame->objectList.link.next;
    while (link != &frame->objectList.link) {
        object = RW_OBJECT_FROM_FRAME_LINK(link);
        link = link->next;
        if (object->type == 1) {
            plugin = (MksobjPluginData*)((char*)object + MksobjLocalOffset);
            sobj = plugin->sobj;
            if (sobj != 0 && (sobj->id_flags & 0xFFF) == id) {
                return sobj;
            }
        }
    }
    if (depth - 1 != 0) {
        child_depth = depth - 1;
        child = frame->child;
        while (child != 0) {
            next = child->next;
            sobj = rwframe_find_child_sobj_by_id(child, id, child_depth);
            if (sobj != 0) {
                return sobj;
            }
            child = next;
        }
    }
    return 0;
}
#pragma inline_depth reset

int sobj_does_atomic_have_children(MkSobj* sobj) {
    return sobj->frame->child != 0;
}

void set_true_clip_flag_on_sobj_and_children(MkSobj* sobj, int flag) {
    RwFrame* frame;
    RwFrame* next;

    sobj->flags09_bits.bit2 = flag;
    frame = sobj->frame;
    frame = frame->child;
    while (frame != 0) {
        next = frame->next;
        rwframe_set_true_clip_flag_on_objects_and_children(
            frame, flag);
        frame = next;
    }
}

static void rwframe_set_true_clip_flag_on_objects_and_children(
    RwFrame* frame, int flag) {
    RwLLLink* link;
    RwLLLink* next_link;
    RwLLLink* end;
    RwObject* object;
    MkSobj* sobj;
    RwFrame* child;
    RwFrame* next_child;
    RwFrame* grandchild;
    RwFrame* next_grandchild;
    RwFrame* descendant;
    RwFrame* next_descendant;

    end = &frame->objectList.link;
    link = frame->objectList.link.next;
    while (link != end) {
        object = RW_OBJECT_FROM_FRAME_LINK(link);
        if (object->type == 1) {
            sobj = MK_ATOMIC_PLUGIN((RpAtomic*)object)->sobj;
            if (sobj != 0) {
                sobj->flags09_bits.bit2 = flag;
            }
        }
        link = link->next;
    }
    child = frame->child;
    while (child != 0) {
        next_child = child->next;
        end = &child->objectList.link;
        link = child->objectList.link.next;
        while (link != end) {
            next_link = link->next;
            object = RW_OBJECT_FROM_FRAME_LINK(link);
            if (object->type == 1) {
                sobj = MK_ATOMIC_PLUGIN((RpAtomic*)object)->sobj;
                if (sobj != 0) {
                    sobj->flags09_bits.bit2 = flag;
                }
            }
            link = next_link;
        }
        grandchild = child->child;
        while (grandchild != 0) {
            next_grandchild = grandchild->next;
            end = &grandchild->objectList.link;
            link = grandchild->objectList.link.next;
            while (link != end) {
                next_link = link->next;
                rwobject_set_true_clip_flag(RW_OBJECT_FROM_FRAME_LINK(link),
                                            flag);
                link = next_link;
            }
            descendant = grandchild->child;
            while (descendant != 0) {
                next_descendant = descendant->next;
                rwframe_set_true_clip_flag_on_objects_and_children(
                    descendant, flag);
                descendant = next_descendant;
            }
            grandchild = next_grandchild;
        }
        child = next_child;
    }
}

static RwObject* rwobject_set_true_clip_flag(RwObject* object, unsigned char flag) {
    MksobjPluginData* plugin;
    MkSobj* sobj;

    if (object->type == 1) {
        plugin = (MksobjPluginData*)((char*)object + MksobjLocalOffset);
        sobj = plugin->sobj;
        if (sobj != 0) {
            sobj->flags09 =
                (unsigned char)((sobj->flags09 & ~4) | ((flag & 1) << 2));
        }
    }
    return object;
}

static inline MkSobj* find_sobj_by_id_inline(MkObj* obj, unsigned int id) {
    MkPtr* ptr;
    MkSobj* sobj;

    ptr = first_mkptr(&obj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFFu) == id) {
            return sobj;
        }
        ptr = next_mkptr(ptr);
    }
    return 0;
}

void obj_set_sobj_alpha(MkObj* obj, int sobj_index, int alpha) {
    MkSobj* sobj;

    if (obj != 0) {
        sobj = find_sobj_by_id_inline(obj, (unsigned int)sobj_index);
        if (sobj != 0) {
            set_atomic_material_alpha(sobj->atomic, alpha);
        }
    }
}

void obj_turn_gravity_off(MkObj* obj) {
    if (obj != 0) {
        obj->flags_08_bits.moving = 0;
    }
}

void obj_enable_grounding(MkObj* obj) {
    obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(obj != 0 ? as_mkhdr(&obj->hdr) : 0);
    ground_me(obj != 0 ? as_mkhdr(&obj->hdr) : 0);
}

void obj_set_gravity(MkObj* obj, float gravity) {
    if (obj != 0) {
        obj->flags_08_bits.moving = 1;
        obj->gravity = gravity;
    }
}

Quat* obj_get_bone_rot_quat(void* obj, int bone) {
    MkObj* mkobj = (MkObj*)obj;
    MkBone* mkbone;

    mkbone = mkobj->bones[bone];
    if (mkbone == 0) {
        mkbone = mkobj->bones[mkobj->fallback_bone_index];
        if (mkbone == 0) {
            return 0;
        }
    }
    return &mkbone->rotation;
}

void obj_get_ang_vel(MkObj* obj, Vec* out) {
    out->x = obj->ang_vel.x;
    out->y = obj->ang_vel.y;
    out->z = obj->ang_vel.z;
}

void mks_set_flipped_bones(MkFlippedBoneMap* bone_map) {
    plyr_obj->flipped_bone_map = bone_map;
}

void obj_set_flipped_bones(MkObj* obj, MkFlippedBoneMap* bone_map) {
    obj->flipped_bone_map = bone_map;
}

void obj_set_light_flag(void* obj, int flag) {
    ((MkObj*)obj)->light_flags = flag;
}

void obj_set_ang_vel(MkObj* obj, void* velocity) {
    Vec* vel = (Vec*)velocity;

    obj->flags_08_bits.rotation_enabled = 1;
    obj->ang_vel.x = vel->x;
    obj->ang_vel.y = vel->y;
    obj->ang_vel.z = vel->z;
}

void obj_get_pos_vel(MkObj* obj, Vec* out) {
    out->x = obj->pos_vel.x;
    out->y = obj->pos_vel.y;
    out->z = obj->pos_vel.z;
}

void obj_set_pos_vel(MkObj* obj, void* velocity) {
    Vec* vel = (Vec*)velocity;

    obj->flags_08_bits.gravity_enabled = 1;
    obj->pos_vel.x = vel->x;
    obj->pos_vel.y = vel->y;
    obj->pos_vel.z = vel->z;
}

void obj_get_ang(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* value = (Vec*)out;

    value->x = mkobj->dir_x;
    value->y = mkobj->dir_y;
    value->z = mkobj->dir_z;
}

void obj_set_ang(void* obj, void* ang) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* value = (Vec*)ang;

    mkobj->dir_x = value->x;
    mkobj->dir_y = value->y;
    mkobj->dir_z = value->z;
}

void obj_get_pos(MkObj* obj, Vec* out) {
    out->x = obj->pos.value.x;
    out->y = obj->pos.value.y;
    out->z = obj->pos.value.z;
}

void obj_set_pos(MkObj* obj, Vec* pos) {
    obj->pos.value.x = pos->x;
    obj->pos.value.y = pos->y;
    obj->pos.value.z = pos->z;
    obj->flags_08_bits.airborne = 1;
}

void obj_get_scale(MkObj* obj, Vec* out) {
    if (obj != 0) {
        out->x = obj->scale.x;
        out->y = obj->scale.y;
        out->z = obj->scale.z;
    }
}

void obj_set_scale(MkObj* obj, void* value) {
    Vec* scale = (Vec*)value;

    if (obj != 0) {
        obj->flags_08_bits.scale_active = 1;
        obj->scale.x = scale->x;
        obj->scale.y = scale->y;
        obj->scale.z = scale->z;
    }
}

void obj_set_ground_colls_y(void* obj, float y) {
    ((MkObj*)obj)->ground_colls_y = y;
}

void obj_set_ground_colls(void* obj, void* colls) {
    ((MkObj*)obj)->ground_colls = colls;
}

void sobj_get_ang_vel(MkSobj* sobj, Vec* out) {
    out->x = sobj->ang_vel.x;
    out->y = sobj->ang_vel.y;
    out->z = sobj->ang_vel.z;
}

void sobj_set_ang_vel(MkSobj* sobj, void* velocity) {
    Vec* vel = (Vec*)velocity;

    sobj->flags_08_bits.angular_velocity_enabled = 1;
    sobj->ang_vel.x = vel->x;
    sobj->ang_vel.y = vel->y;
    sobj->ang_vel.z = vel->z;
}

void sobj_get_pos_vel(MkSobj* sobj, Vec* out) {
    out->x = sobj->pos_vel.x;
    out->y = sobj->pos_vel.y;
    out->z = sobj->pos_vel.z;
}

void sobj_set_pos_vel(MkSobj* mksobj, void* vel) {
    Vec* velocity = (Vec*)vel;

    mksobj->flags_08_bits.bit6 = 1;
    mksobj->flags_08_bits.bit5 = 1;
    mksobj->pos_vel.x = velocity->x;
    mksobj->pos_vel.y = velocity->y;
    mksobj->pos_vel.z = velocity->z;
}

void sobj_get_ang(MkSobj* sobj, Vec* out) {
    out->x = sobj->ang.x;
    out->y = sobj->ang.y;
    out->z = sobj->ang.z;
}

void sobj_set_ang(MkSobj* sobj, void* value) {
    Vec* ang = (Vec*)value;

    sobj->flags_08_bits.bit4 = 1;
    sobj->ang.x = ang->x;
    sobj->ang.y = ang->y;
    sobj->ang.z = ang->z;
}

void sobj_get_pos(MkSobj* sobj, Vec* out) {
    out->x = sobj->pos.x;
    out->y = sobj->pos.y;
    out->z = sobj->pos.z;
}

void sobj_set_pos(MkSobj* sobj, void* value) {
    Vec* pos = (Vec*)value;

    sobj->flags_08_bits.bit7 = 1;
    sobj->pos.x = pos->x;
    sobj->pos.y = pos->y;
    sobj->pos.z = pos->z;
}

Vec* sobj_get_world_pos(MkSobj* sobj) {
    RwMatrix* matrix;

    matrix = (RwMatrix*)RwFrameGetLTM(sobj->frame);
    return (Vec*)&matrix->pos;
}

int is_sobj_hidden(void* sobj) {
    return ((((MkSobj*)sobj)->atomic->object.flags >> 2) & 1) == 0;
}

void unhide_atomic(void* atomic) {
    ((RpAtomic*)atomic)->object.flags = 4;
}

void hide_atomic(void* atomic) {
    ((RpAtomic*)atomic)->object.flags &= ~4u;
}

static RwFrame* rwframe_unhide_objects_and_children(RwFrame* frame, void* data);
static RwObject* rwobject_unhide_my_object(RwObject* object, void* data);
static RwFrame* rwframe_hide_objects_and_children(RwFrame* frame, void* data);
static RwObject* rwobject_hide_my_object(RwObject* object, void* data);

void unhide_sobj_and_children(MkSobj* sobj) {
    RwFrame* frame;
    RpAtomic* atomic;

    frame = sobj->frame;
    atomic = sobj->atomic;
    atomic->object.flags = 4;
    RwFrameForAllChildren(
        frame, rwframe_unhide_objects_and_children, 0);
}

static RwFrame* rwframe_unhide_objects_and_children(RwFrame* frame, void* data) {
    RwFrameForAllObjects(frame, rwobject_unhide_my_object, 0);
    RwFrameForAllChildren(frame, rwframe_unhide_objects_and_children, 0);
    return frame;
}

static RwObject* rwobject_unhide_my_object(RwObject* object, void* data) {
    if (object->type == 1) {
        object->flags = 4;
    }
    return object;
}

void hide_sobj_and_children(MkSobj* sobj) {
    RpAtomic* atomic;
    RwFrame* frame;

    atomic = sobj->atomic;
    frame = sobj->frame;
    atomic->object.flags &= ~4u;
    RwFrameForAllChildren(
        frame, rwframe_hide_objects_and_children, 0);
}

static RwFrame* rwframe_hide_objects_and_children(RwFrame* frame, void* data) {
    RwFrameForAllObjects(frame, rwobject_hide_my_object, 0);
    RwFrameForAllChildren(frame, rwframe_hide_objects_and_children, 0);
    return frame;
}

static RwObject* rwobject_hide_my_object(RwObject* object, void* data) {
    if (object->type == 1) {
        object->flags &= ~4u;
    }
    return object;
}

void kill_head_tracking(void) {
    MkPtr* next;
    MkPtr* ptr;
    MkProc* proc;

    if (&active_proc_list != 0) {
        ptr = active_proc_list;
        while (ptr != 0) {
            proc = (MkProc*)ptr->hdr;
            if (ptr->instance != proc->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                if (proc->pid == 0x6005) {
                    xfer_proc(proc, p_headtracking_die);
                }
                ptr = ptr->next;
            }
        }
    }
}

static float p_headtracking_die(void) {
    HeadTrackingPdata* pdata;
    MkBone* bone;

    pdata = (HeadTrackingPdata*)pdata_headtracking;
    if (pdata->obj != 0) {
        bone = pdata->obj->bones[16];
        if (bone != 0) {
            bone->flags_54_bits.pose_matrix_applied = 0;
        }
    }
    return -1.0f;
}

MkProc* create_mkproc_headtracking(int pid, MkObj* obj, PlyrPdata* target) {
    HeadTrackingPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_nostack(
        pid, 0x14, p_plyr_head_tracking, sizeof(HeadTrackingPdata),
        (MkHdr**)&pdata);
    if (proc != 0) {
        proc->pre_destroy = trackhead_prewake;
        proc->destroy_cb = trackhead_postsleep;
        pdata->obj = obj;
        pdata->target = target;
        obj->flags_09_bits.head_tracking = 1;
        pdata->angle_x = 0.0f;
        pdata->angle_y = 0.0f;
        pdata->blend_weight = 0.0f;
    }
    return proc;
}

static inline int head_tracking_should_fade(MkObj* obj, PlyrPdata* target,
                                            int* special_camera) {
    int state;

    if (g_game_info.plyr0.slot.mirror_a == obj ||
        g_game_info.plyr1.slot.mirror_a == obj) {
        state = target->state;
        if (state < 0x6200) {
            if (state < 0x6004 && state >= 0x6000) {
                return 1;
            }
        } else if (state < 0x6202) {
            return 1;
        }
        if (mode_of_play == 6) {
            if ((state & 0x400) != 0) {
                return 1;
            }
        } else if ((state & 0x600) != 0) {
            return 1;
        }
    }
    if (aproc->pid == 0x6006) {
        *special_camera = 1;
    }
    return obj->flags_09_bits.head_tracking == 0;
}

static float p_plyr_head_tracking(void) {
    PlyrPdata* target;
    MkObj* obj;
    MkObj* target_obj;
    MkBone* head;
    MkBone* neck;
    RwMatrix* camera_matrix;
    Vec object_pos;
    Vec target_pos;
    Vec target_low_pos;
    Vec direction;
    Vec target_angles;
    Vec neck_angles;
    Vec camera_angles;
    Quat desired_rotation;
    Quat local_rotation;
    Vec camera_offset;
    int special_camera;
    int saved_fallback_bone;

    special_camera = 0;
    obj = pdata_headtracking->obj;
    head = obj->bones[16];
    if (head != 0) {
        target = pdata_headtracking->target;
        target_obj = target->his_obj;
        if (target_obj != 0 && (int)target_obj->oid == 0) {
            return 1.0f;
        }
        if (head->flags_55_bits.collision_disabled != 0 ||
            head->parent_matrix == 0 ||
            head->transform_parent->parent_matrix == 0 ||
            target->fighter_definition->fighter_id == 0x33) {
            head->flags_54_bits.pose_matrix_applied = 0;
            return 1.0f;
        }

        if (head_tracking_should_fade(obj, target, &special_camera)) {
            pdata_headtracking->blend_weight -= 0.06f;
            if (special_camera != 0 &&
                pdata_headtracking->blend_weight <= 0.4f) {
                pdata_headtracking->blend_weight = 0.4f;
            } else if (pdata_headtracking->blend_weight <= 0.0f) {
                pdata_headtracking->blend_weight = 0.0f;
                head->flags_54_bits.pose_matrix_applied = 0;
                return 1.0f;
            }
        } else {
            pdata_headtracking->blend_weight += 0.06f;
            if (pdata_headtracking->blend_weight > 1.0f) {
                pdata_headtracking->blend_weight = 1.0f;
            }
        }

        head->flags_54_bits.pose_matrix_applied = 1;
        target_obj = pdata_headtracking->target->his_obj;
        if (target_obj == 0 && aproc->pid != 0x6006) {
            return -1.0f;
        }
        neck = obj->bones[9];
        if (neck != 0) {
            camera_matrix = RwFrameGetLTM(obj->frame);
            gxMatV3MatAddV3_Check(
                (Vec*)&head->parent_matrix->pos, &head->translation.value,
                (Mat33*)head->transform_parent->parent_matrix,
                (Vec*)&head->transform_parent->parent_matrix->pos);

            if ((int)obj->bone_count <= 16) {
                object_pos = obj->pos.value;
            } else {
                MkBone* root = obj->bones[16];
                if (root == 0 || root->parent_matrix == 0) {
                    object_pos.x = obj->pos.value.x;
                    object_pos.y = obj->pos.value.y;
                    object_pos.z = obj->pos.value.z;
                } else {
                    v3_x_mat_add_v3(&object_pos,
                                    (Vec*)&root->parent_matrix->pos,
                                    (MKMATRIX*)obj->field_24, &obj->pos.value);
                }
            }

            if (special_camera != 0) {
                PSVECScale((Vec*)&camera_matrix->up, &camera_offset, 0.1f);
                PSVECAdd((Vec*)&camera_matrix->at, &camera_offset, &target_pos);
                PSVECSubtract(&target_pos, (Vec*)&camera_matrix->right,
                              &target_pos);
                PSVECNormalize(&target_pos, &target_pos);
                v3_to_xy_ang(&target_angles, &target_pos);
            } else {
                if ((int)target_obj->bone_count <= 16) {
                    target_pos = target_obj->pos.value;
                } else {
                    MkBone* target_bone = target_obj->bones[16];
                    if (target_bone == 0 || target_bone->parent_matrix == 0) {
                        target_pos.x = target_obj->pos.value.x;
                        target_pos.y = target_obj->pos.value.y;
                        target_pos.z = target_obj->pos.value.z;
                    } else {
                        v3_x_mat_add_v3(
                            &target_pos, (Vec*)&target_bone->parent_matrix->pos,
                            (MKMATRIX*)target_obj->field_24, &target_obj->pos.value);
                    }
                }
                if ((int)target_obj->bone_count <= 9) {
                    target_low_pos = target_obj->pos.value;
                } else {
                    MkBone* target_bone = target_obj->bones[9];
                    if (target_bone == 0 || target_bone->parent_matrix == 0) {
                        target_low_pos.x = target_obj->pos.value.x;
                        target_low_pos.y = target_obj->pos.value.y;
                        target_low_pos.z = target_obj->pos.value.z;
                    } else {
                        v3_x_mat_add_v3(&target_low_pos,
                                        (Vec*)&target_bone->parent_matrix->pos,
                                        (MKMATRIX*)target_obj->field_24,
                                        &target_obj->pos.value);
                    }
                }
                if (target_pos.y > target_low_pos.y) {
                    PSVECSubtract(&target_pos, &object_pos, &direction);
                    PSVECNormalize(&direction, &direction);
                } else {
                    PSVECSubtract(&target_low_pos, &object_pos, &direction);
                    PSVECNormalize(&direction, &direction);
                }
                v3_to_xy_ang(&target_angles, &direction);
                v3_to_xy_ang(&camera_angles, (Vec*)&camera_matrix->at);
                target_angles.x -= camera_angles.x;
                target_angles.y -= camera_angles.y;
                target_angles.z = 0.0f;
                v3_to_xy_ang(&neck_angles, (Vec*)&neck->parent_matrix->at);
                target_angles.y -= neck_angles.y;
                if (target_angles.y > 3.1415927f) {
                    target_angles.y -= 6.2831855f;
                } else if (target_angles.y < -3.1415927f) {
                    target_angles.y += 6.2831855f;
                }
                if (target_angles.y > 1.0f) {
                    target_angles.y = 1.0f;
                } else if (target_angles.y < -1.0f) {
                    target_angles.y = -1.0f;
                }
                target_angles.y += neck_angles.y;
                if (target_angles.y < pdata_headtracking->angle_y) {
                    if (pdata_headtracking->angle_y - target_angles.y > 0.2f) {
                        target_angles.y = pdata_headtracking->angle_y - 0.2f;
                    }
                } else if (target_angles.y - pdata_headtracking->angle_y >
                           0.2f) {
                    target_angles.y = pdata_headtracking->angle_y + 0.2f;
                }
                if (target_angles.x > neck_angles.x + 1.0f) {
                    target_angles.x = neck_angles.x + 1.0f;
                } else if (target_angles.x < neck_angles.x - 1.0f) {
                    target_angles.x = neck_angles.x - 1.0f;
                }
                if (target_angles.x < pdata_headtracking->angle_x) {
                    if (pdata_headtracking->angle_x - target_angles.x >
                        0.035f) {
                        target_angles.x = pdata_headtracking->angle_x - 0.035f;
                    }
                } else if (target_angles.x - pdata_headtracking->angle_x >
                           0.035f) {
                    target_angles.x = pdata_headtracking->angle_x + 0.035f;
                }
                pdata_headtracking->angle_x = target_angles.x;
                pdata_headtracking->angle_y = target_angles.y;
            }
            if (special_camera != 0 ||
                pdata_headtracking->blend_weight >= 1.0f) {
                YXZ_angles_to_MKMATRIX(&target_angles,
                                       (MKMATRIX*)head->parent_matrix);
                RtQuatConvertFromMatrix(&head->rotation_90,
                                        head->parent_matrix);
            } else {
                YXZ_angles_to_quat(&target_angles, &desired_rotation);
                gxQuatMul(&local_rotation, &head->rotation,
                          &obj->bones[13]->rotation_90);
                gxQuatInterpQuat(&head->rotation_90, &desired_rotation,
                                 &local_rotation,
                                 pdata_headtracking->blend_weight);
                gxQuatQuatToMat((Mat33*)head->parent_matrix,
                                &head->rotation_90);
            }
            if (head->flags_55_bits.scale_controlled != 0) {
                gxMatScaledByV3((Mat33*)head->parent_matrix,
                                (Mat33*)head->parent_matrix, &head->scale);
            }
            if (head->flags_54_bits.calculation_locked != 0) {
                gxMat33x33_Check((Mat33*)&head->matrix,
                                 (Mat33*)head->parent_matrix,
                                 (Mat33*)camera_matrix);
                gxMatV3MatAddV3(
                    (Vec*)&head->matrix.pos, (Vec*)&head->parent_matrix->pos,
                    (Mat33*)camera_matrix, (Vec*)&camera_matrix->pos);
            }
            if (head->tree_child != 0) {
                saved_fallback_bone = obj->fallback_bone_index;
                obj->fallback_bone_index = head->bone_index;
                update_bone_hierarchy(obj != 0 ? as_mkhdr(&obj->hdr) : 0);
                obj->fallback_bone_index = saved_fallback_bone;
            }
        }
    }
    return 1.0f;
}

static void trackhead_postsleep(void) {
    pdata_headtracking = 0;
}

static void trackhead_prewake(void) {
    if (aproc->pid != 0x6005 && aproc->pid != 0x6006) {
        mkproc_die();
    }
    pdata_headtracking = (HeadTrackingPdata*)apdata;
    if (pdata_headtracking == 0) {
        mkproc_die();
    }
}

void mks_start_goro_arms_fixup(void) {
    MkProc* proc;

    proc = _create_mkproc_generic_nostack(
        0x502E, 0xE, p_goro_arms_fixup, 0, 0);
    if (proc != 0) {
        mk_insert_no_own((MkHdr*)plyr_obj, &proc->pdata_list);
    }
    mks_start_goro_xtra_weapons();
}

static float p_goro_arms_fixup(void) {
    MkObj* obj;
    MkBone* source;
    MkBone* target;
    GoroArmsFixupEntry* entry;
    int i;

    obj = (MkObj*)apdata;
    if (obj == 0) {
        return 1.0f;
    }
    for (i = 0; i < 12; i++) {
        entry = &goro_arms_fixup_map[i];
        if (entry->source_bone < (int)obj->bone_count &&
            entry->target_bone < (int)obj->bone_count) {
            source = obj->bones[entry->source_bone];
            if (source != 0) {
                target = obj->bones[entry->target_bone];
                if (target != 0 &&
                    target->update_tick != (unsigned int)exec_tick_ctr) {
                    gxQuatInterpQuat(
                        &target->rotation, &target->rotation,
                        &source->rotation, 0.9f);
                    target->flags_55_bits.preserve_rotation = 1;
                    gxQuatCopy(
                        &target->rotation_e0, &target->rotation);
                }
            }
        }
    }
    return 1.0f;
}

void mirror_guy(MkObj* source, MkObj* mirror, PlyrPdata* pdata) {
    PlyrMirrorBoneMap* map;
    PlyrMirrorObjLatch* latch;
    MkObj* linked_obj;
    MkBone* bone;
    RwMatrix* matrix;
    Vec angles;
    int i;

    map = pdata->mirror_bone_map;
    angles.x = source->ang.x;
    angles.y = source->ang.y;
    angles.z = source->ang.z;

    if (mirror != 0) {
        for (i = 0; i < map->count; i++) {
            bone = mirror->bones[map->entries[i].bone_index];
            if (bone != 0 && bone->parent_matrix != 0) {
                matrix = bone->parent_matrix;
                matrix->pos.y = -matrix->pos.y;
                matrix->at.y = -matrix->at.y;
                matrix->right.y = -matrix->right.y;
                matrix->up.y = -matrix->up.y;
            }
        }
        matrix = mirror->field_24;
        YXZ_angles_to_MKMATRIX(&angles, (MKMATRIX*)matrix);
        if (pdata->plyr_info->flags_14_bits.alternate_costume) {
            matrix->pos.y =
                -(g_game_info.misc->mirror_plane_offset +
                  (pdata->runtime_data->alternate_mirror_offset +
                   (source->pos.value.y - g_game_info.field_34)));
            matrix->pos.y += g_game_info.field_34;
        } else {
            matrix->pos.y =
                -(g_game_info.misc->mirror_plane_offset +
                  (pdata->runtime_data->primary_mirror_offset +
                   (source->pos.value.y - g_game_info.field_34)));
            matrix->pos.y += g_game_info.field_34;
        }
        RwFrameUpdateObjects(mirror->frame);
    }

    latch = &pdata->mirror_slots->weapon[0].mirror;
    linked_obj = latch->obj;
    if (linked_obj != 0) {
        if (linked_obj->hdr.instance != latch->instance) {
            linked_obj = 0;
        }
    } else {
        linked_obj = 0;
    }
    if (linked_obj != 0 && !linked_obj->hide_flag_bits.hidden) {
        matrix = linked_obj->field_24;
        matrix->pos.y = -matrix->pos.y;
        matrix->at.y = -matrix->at.y;
        matrix->right.y = -matrix->right.y;
        matrix->up.y = -matrix->up.y;
        matrix->pos.y += 2.0f * g_game_info.field_34;
        RwMatrixUpdate(matrix);
        RwFrameUpdateObjects(linked_obj->frame);
    }

    latch = &pdata->mirror_slots->weapon[1].mirror;
    linked_obj = latch->obj;
    if (linked_obj != 0) {
        if (linked_obj->hdr.instance != latch->instance) {
            linked_obj = 0;
        }
    } else {
        linked_obj = 0;
    }
    if (linked_obj != 0 && !linked_obj->hide_flag_bits.hidden) {
        matrix = linked_obj->field_24;
        matrix->pos.y = -matrix->pos.y;
        matrix->at.y = -matrix->at.y;
        matrix->right.y = -matrix->right.y;
        matrix->up.y = -matrix->up.y;
        matrix->pos.y += 2.0f * g_game_info.field_34;
        RwMatrixUpdate(matrix);
        RwFrameUpdateObjects(linked_obj->frame);
    }

    latch = &pdata->mirror_obj;
    linked_obj = latch->obj;
    if (linked_obj != 0) {
        if (linked_obj->hdr.instance != latch->instance) {
            linked_obj = 0;
        }
    } else {
        linked_obj = 0;
    }
    if (linked_obj != 0 && !linked_obj->hide_flag_bits.hidden) {
        matrix = linked_obj->field_24;
        matrix->pos.y = -matrix->pos.y;
        matrix->at.y = -matrix->at.y;
        matrix->right.y = -matrix->right.y;
        matrix->up.y = -matrix->up.y;
        matrix->pos.y += 2.0f * g_game_info.field_34;
        RwMatrixUpdate(matrix);
        RwFrameUpdateObjects(linked_obj->frame);
    }
}

void create_shadow_proc(int pid, PlyrPdata* controller, MkObj* source,
                        MkObj* shadow) {
    ShadowPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_nostack(
        pid, 0x2A, p_shadow_obj, sizeof(ShadowPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        pdata->source = source;
        pdata->source_instance = source->hdr.instance;
        pdata->shadow = shadow;
        pdata->shadow_instance = shadow->hdr.instance;
        pdata->controller = controller;
        controller->shadow_proc = proc;
        proc->flags_bits.skip_if_paused = 1;
    } else {
        controller->shadow_proc = 0;
    }
}

static void shadow_update_pair(ShadowObjPair* pair) {
    MkObj* raw_source;
    MkObj* raw_shadow;
    MkObj* source;
    MkObj* shadow;

    raw_source = pair->source;
    if (raw_source != 0) {
        if (raw_source->hdr.instance == pair->source_instance) {
            source = raw_source;
        } else {
            source = 0;
        }
    } else {
        source = 0;
    }
    raw_shadow = pair->shadow;
    if (raw_shadow != 0) {
        if (raw_shadow->hdr.instance == pair->shadow_instance) {
            shadow = raw_shadow;
        } else {
            shadow = 0;
        }
    } else {
        shadow = 0;
    }
    if (source != 0 && shadow != 0) {
        memcpy(shadow->field_24, &source->bones[0]->matrix,
               sizeof(RwMatrix));
        RwFrameUpdateObjects(shadow->frame);
    } else if (shadow != 0 && source == 0 &&
               shadow->hdr.instance != 0) {
        ((void (*)(MkHdr*))shadow->hdr.vtbl->destroy)(&shadow->hdr);
    }
}

static inline MkObj* shadow_validate_obj(MkObj* raw,
                                         unsigned int expected_instance) {
    if (raw != 0) {
        if (raw->hdr.instance == expected_instance) {
            return raw;
        }
        return 0;
    }
    return 0;
}

static float p_shadow_obj(void) {
    ShadowPdata* pdata;
    PlyrMirrorBoneMap* map;
    PlyrMirrorBoneMapEntry* pair;
    MkObj* source;
    MkObj* shadow;
    MkBone* source_bone;
    MkBone* shadow_bone;
    int i;

    pdata = (ShadowPdata*)apdata;
    source = shadow_validate_obj(pdata->source, pdata->source_instance);
    if (source == 0) {
        return -1.0f;
    }
    shadow = shadow_validate_obj(pdata->shadow, pdata->shadow_instance);
    if (shadow == 0) {
        return -1.0f;
    }

    map = pdata->controller->mirror_bone_map;
    for (i = 0; i < map->count; i++) {
        pair = &map->entries[i];
        if (pair->field_00 < (int)source->bone_count &&
            pair->bone_index < (int)shadow->bone_count) {
            shadow_bone = shadow->bones[pair->bone_index];
            if (shadow_bone != 0) {
                source_bone = source->bones[pair->field_00];
                if (source_bone != 0 &&
                    shadow_bone->parent_matrix != 0 &&
                    source_bone->parent_matrix != 0) {
                    memcpy(shadow_bone->parent_matrix,
                           source_bone->parent_matrix, sizeof(RwMatrix));
                }
            }
        }
    }
    memcpy(shadow->field_24, source->field_24, sizeof(RwMatrix));
    RwFrameUpdateObjects(shadow->frame);

    shadow_update_pair((ShadowObjPair*)&pdata->controller->mirror_slots
                           ->weapon[0].primary);
    shadow_update_pair((ShadowObjPair*)&pdata->controller->mirror_slots
                           ->weapon[1].primary);
    shadow_update_pair((ShadowObjPair*)&pdata->controller->aux_weapon_latch);
    return 1.0f;
}

void start_bone_hierarchy_proc(void) {
    int flags[2];
    MkProc* proc;

    flags[1] = 0;
    bone_hierarchy_mkobj_list = 0;
    limb_bone_list = 0;
    ground_me_mkobj_list = 0;
    flags[0] = flags[1];
    proc = get_mkproc_nostack(&flags[0]);
    create_mkproc(0x13, proc, 0x5006, p_bone_hierarchy, 0);
}

static float p_bone_hierarchy(void) {
    apply_to_mklist(
        (MkListApplyFn)update_bone_hierarchy,
        &bone_hierarchy_mkobj_list);
    apply_to_mklist(
        (MkListApplyFn)limb_bone_calc_world_pos,
        &limb_bone_list);
    apply_to_mklist(
        (MkListApplyFn)ground_me, &ground_me_mkobj_list);
    return 1.0f;
}

void auto_calc_limbobj_bone_world_pos(MkObj* obj, int bone) {
    LimbBonePdata* pdata;

    pdata = (LimbBonePdata*)get_mkpdata_generic(
        sizeof(LimbBonePdata));
    if (pdata != 0) {
        pdata->obj = obj;
        pdata->obj_instance = obj->hdr.instance;
        pdata->bone = bone;
        mk_insert(&pdata->hdr, &limb_bone_list);
    }
}

static void limb_bone_calc_world_pos(MkHdr* data) {
    LimbBonePdata* pdata;
    MkObj* obj;
    MkBone* bone;
    RwMatrix* parent;
    Vec* out;
    int bone_index;

    pdata = (LimbBonePdata*)data;
    obj = pdata->obj;
    if (obj != 0) {
        if (obj->hdr.instance != pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        bone_index = pdata->bone;
        bone = obj->bones[bone_index];
        out = (Vec*)&bone->matrix.pos;
        if (bone_index >= (int)obj->bone_count) {
            *out = obj->pos.value;
            return;
        }
        if (bone_index == -1) {
            *out = obj->pos.value;
            return;
        }
        if (bone == 0 || (parent = bone->parent_matrix) == 0) {
            out->x = obj->pos.value.x;
            out->y = obj->pos.value.y;
            out->z = obj->pos.value.z;
            return;
        }
        v3_x_mat_add_v3(
            out, (Vec*)&parent->pos, (MKMATRIX*)obj->field_24,
            &obj->pos.value);
        return;
    }
    if (pdata->hdr.instance != 0) {
        ((void (*)(MkHdr*))pdata->hdr.vtbl->destroy)(&pdata->hdr);
    }
}

void ground_me(void* obj) {
    MkObj* mkobj;
    GroundCollisionEntry* collision;
    MkBone* bone;
    RwMatrix* parent;
    RwMatrix* matrix;
    RwFrame* frame;
    MkSobj* sobj;
    RwMatrix* matrices;
    float min_height;
    float height;
    int bone_index;
    int i;

    mkobj = (MkObj*)obj;
    if (mkobj->flags_09_bits.launched == 0) {
        return;
    }
    collision = (GroundCollisionEntry*)mkobj->ground_colls;
    if (collision == 0) {
        return;
    }
    matrix = mkobj->field_24;
    if (mkobj->hide_flag_bits.pin_animation != 0) {
        if (mode_of_play == 7) {
            set_bone_world_pos_xz(
                mkobj, mkobj->ground_bone, &mkobj->ground_restore_pos);
        } else {
            set_bone_world_pos(
                mkobj, mkobj->ground_bone, &mkobj->ground_restore_pos);
        }
    }
    mkobj->flags_0C_bits.tag_flag_10 = 0;
    if (mkobj->flags_0C_bits.tag_flag_08 != 0) {
        mkobj->flags_0C_bits.tag_flag_08 = 0;
        mkobj->hide_flag_bits.pin_animation = 0;
    }
    min_height = 1000.0f;

    while (collision->bone >= 0) {
        bone_index = collision->bone;
        if (mode_of_play != 6 || bone_index < 0x16 ||
            bone_index > 0x2F) {
            bone = mkobj->bones[bone_index];
            if (bone != 0 && bone->parent_matrix != 0 &&
                bone_index < (int)mkobj->bone_count) {
                parent = bone->parent_matrix;
                if (bone->flags_54_bits.calculation_locked == 0) {
                    bone->matrix.right.x =
                        parent->up.x * matrix->up.x +
                        parent->right.x * matrix->right.x +
                        parent->at.x * matrix->at.x;
                    bone->matrix.right.y =
                        parent->up.x * matrix->up.y +
                        parent->right.x * matrix->right.y +
                        parent->at.x * matrix->at.y;
                    bone->matrix.right.z =
                        parent->up.x * matrix->up.z +
                        parent->right.x * matrix->right.z +
                        parent->at.x * matrix->at.z;
                    bone->matrix.up.x =
                        parent->up.y * matrix->up.x +
                        parent->right.y * matrix->right.x +
                        parent->at.y * matrix->at.x;
                    bone->matrix.up.y =
                        parent->up.y * matrix->up.y +
                        parent->right.y * matrix->right.y +
                        parent->at.y * matrix->at.y;
                    bone->matrix.up.z =
                        parent->up.y * matrix->up.z +
                        parent->right.y * matrix->right.z +
                        parent->at.y * matrix->at.z;
                    bone->matrix.at.x =
                        parent->up.z * matrix->up.x +
                        parent->right.z * matrix->right.x +
                        parent->at.z * matrix->at.x;
                    bone->matrix.at.y =
                        parent->up.z * matrix->up.y +
                        parent->right.z * matrix->right.y +
                        parent->at.z * matrix->at.y;
                    bone->matrix.at.z =
                        parent->up.z * matrix->up.z +
                        parent->right.z * matrix->right.z +
                        parent->at.z * matrix->at.z;
                    bone->matrix.flags =
                        parent->flags & matrix->flags;
                    bone->matrix.pos.x =
                        parent->pos.y * matrix->up.x +
                        parent->pos.x * matrix->right.x +
                        parent->pos.z * matrix->at.x + mkobj->pos.value.x;
                    bone->matrix.pos.y =
                        parent->pos.y * matrix->up.y +
                        parent->pos.x * matrix->right.y +
                        parent->pos.z * matrix->at.y + mkobj->pos.value.y;
                    bone->matrix.pos.z =
                        parent->pos.y * matrix->up.z +
                        parent->pos.x * matrix->right.z +
                        parent->pos.z * matrix->at.z + mkobj->pos.value.z;
                }
                height = collision->offset.y * bone->matrix.up.y +
                         collision->offset.x * bone->matrix.right.y +
                         collision->offset.z * bone->matrix.at.y +
                         bone->matrix.pos.y - collision->radius;
                if (height < min_height) {
                    min_height = height;
                }
            }
        }
        collision++;
    }

    if (min_height != 1000.0f) {
        if (min_height < mkobj->ground_colls_y) {
            mkobj->pos.value.y -= min_height - mkobj->ground_colls_y;
            *(Vec*)&matrix->pos = mkobj->pos.value;
            matrix->flags &= ~0x20000;
            if ((mkobj->hide_flags & 0x10) == 0) {
                RwFrameUpdateObjects(mkobj->frame);
                for (i = 1; i < mkobj->clump_count; i++) {
                    frame = mkobj->clumps[i]->object.parent;
                    frame->modelling.pos = matrix->pos;
                    RwFrameUpdateObjects(frame);
                }
            }
            if (mkobj->matrix_count > 1) {
                sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
                if (sobj != 0 && sobj->matrices != 0) {
                    matrices = sobj->matrices;
                    for (i = 1; i < (int)mkobj->matrix_count; i++) {
                        matrices[mkobj->matrix_indices[i]].pos = matrix->pos;
                    }
                }
            }
            if (mkobj->flags_08_bits.gravity_enabled == 0 ||
                !(mkobj->pos_vel.y > 0.0f)) {
                mkobj->pos_vel.y = 0.0f;
                mkobj->flags_08_bits.moving = 0;
            }
        } else if (mkobj->flags_09_bits.bit6 != 0) {
            mkobj->pos.value.y -= min_height - mkobj->ground_colls_y;
            *(Vec*)&matrix->pos = mkobj->pos.value;
            matrix->flags &= ~0x20000;
            if ((mkobj->hide_flags & 0x10) == 0) {
                RwFrameUpdateObjects(mkobj->frame);
                for (i = 1; i < mkobj->clump_count; i++) {
                    frame = mkobj->clumps[i]->object.parent;
                    frame->modelling.pos = matrix->pos;
                    RwFrameUpdateObjects(frame);
                }
            }
            if (mkobj->matrix_count > 1) {
                sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
                if (sobj != 0 && sobj->matrices != 0) {
                    matrices = sobj->matrices;
                    for (i = 1; i < (int)mkobj->matrix_count; i++) {
                        matrices[mkobj->matrix_indices[i]].pos = matrix->pos;
                    }
                }
            }
        }
        mkobj->flags_0C_bits.tag_flag_40 = 0;
        if (mkobj->flags_0C_bits.tag_flag_20 != 0) {
            mkobj->flags_0C_bits.tag_flag_20 = 0;
            mkobj->flags_09_bits.bit6 = 0;
        }
    }
}

void obj_clear_bone_collapse_flag(MkObj* obj, int bone) {
    MkBone* mkbone;

    mkbone = obj->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_55_bits.collision_disabled = 0;
    }
}

void obj_set_bone_collapse_flag(MkObj* obj, int bone) {
    MkBone* mkbone;

    mkbone = obj->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_55_bits.collision_disabled = 1;
    }
}

void obj_set_bone_calc_world_mat_flag(MkObj* obj, int bone) {
    MkBone* mkbone;

    mkbone = obj->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_54_bits.calculation_locked = 1;
    }
}

MkBone* force_calc_bone_world_mat(MkObj* obj, int bone) {
    RwMatrix* objectMatrix;
    MkBone* mkbone;

    if (bone >= (int)obj->bone_count) {
        return 0;
    }
    mkbone = obj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        return 0;
    }
    objectMatrix = obj->field_24;
    gxMat33x33(
        (Mat33*)&mkbone->matrix, (Mat33*)mkbone->parent_matrix,
        (Mat33*)objectMatrix);
    gxMatV3MatAddV3(
        (Vec*)&mkbone->matrix.pos, (Vec*)&mkbone->parent_matrix->pos,
        (Mat33*)objectMatrix, (Vec*)&objectMatrix->pos);
    return mkbone;
}

void calc_bone_world_mat(MkObj* obj, int bone) {
    RwMatrix* objectMatrix;
    MkBone* mkbone;

    if (bone < (int)obj->bone_count) {
        mkbone = obj->bones[bone];
        if (mkbone == 0) {
            return;
        }
        if (mkbone->parent_matrix != 0) {
            if (mkbone->flags_54_bits.calculation_locked != 0) {
                return;
            }
            objectMatrix = obj->field_24;
            gxMat33x33(
                (Mat33*)&mkbone->matrix, (Mat33*)mkbone->parent_matrix,
                (Mat33*)objectMatrix);
            gxMatV3MatAddV3(
                (Vec*)&mkbone->matrix.pos, (Vec*)&mkbone->parent_matrix->pos,
                (Mat33*)objectMatrix, (Vec*)&objectMatrix->pos);
        }
    }
}

void get_bone_offset_world_pos(
    MkObj* obj, int bone, const Vec* offset, Vec* out) {
    MkBone* mkbone;
    RwMatrix* object_matrix;
    int bone_count;

    bone_count = (int)obj->bone_count;
    if (bone >= bone_count) {
        *out = obj->pos.value;
        return;
    }
    mkbone = obj->bones[bone];
    if (bone == -1 || mkbone == 0 ||
        mkbone->parent_matrix == 0) {
        object_matrix = obj->field_24;
        v3_x_mat_add_v3(
            out, offset, (MKMATRIX*)object_matrix,
            (Vec*)&object_matrix->pos);
        return;
    }
    if (bone < bone_count && mkbone != 0 && mkbone->parent_matrix != 0 &&
        mkbone->flags_54_bits.calculation_locked == 0) {
        object_matrix = obj->field_24;
        gxMat33x33(
            (Mat33*)&mkbone->matrix, (Mat33*)mkbone->parent_matrix,
            (Mat33*)object_matrix);
        gxMatV3MatAddV3(
            (Vec*)&mkbone->matrix.pos, (Vec*)&mkbone->parent_matrix->pos,
            (Mat33*)object_matrix, (Vec*)&object_matrix->pos);
    }
    out->x = offset->y * mkbone->matrix.up.x +
             offset->x * mkbone->matrix.right.x +
             offset->z * mkbone->matrix.at.x + mkbone->matrix.pos.x;
    out->y = offset->y * mkbone->matrix.up.y +
             offset->x * mkbone->matrix.right.y +
             offset->z * mkbone->matrix.at.y + mkbone->matrix.pos.y;
    out->z = offset->y * mkbone->matrix.up.z +
             offset->x * mkbone->matrix.right.z +
             offset->z * mkbone->matrix.at.z + mkbone->matrix.pos.z;
}

static void set_bone_world_pos_xz(
    void* obj, int bone, void* pos) {
    MkObj* mkobj;
    MkBone* mkbone;
    RwMatrix* parent;
    RwMatrix* matrix;
    RwFrame* frame;
    RwMatrix* frame_matrix;
    MkSobj* sobj;
    Vec current;
    Vec* target;
    int i;
    int matrix_index;

    mkobj = (MkObj*)obj;
    target = (Vec*)pos;
    if (bone >= (int)mkobj->bone_count) {
        current = mkobj->pos.value;
    } else if (bone == -1) {
        current = mkobj->pos.value;
    } else {
        mkbone = mkobj->bones[bone];
        if (mkbone == 0 || (parent = mkbone->parent_matrix) == 0) {
            current.x = mkobj->pos.value.x;
            current.y = mkobj->pos.value.y;
            current.z = mkobj->pos.value.z;
        } else {
            v3_x_mat_add_v3(
                &current, (Vec*)&parent->pos,
                (MKMATRIX*)mkobj->field_24, &mkobj->pos.value);
        }
    }
    mkobj->pos.value.x += target->x - current.x;
    mkobj->pos.value.z += target->z - current.z;
    mkobj->flags_08_bits.bit7 = 1;

    matrix = mkobj->field_24;
    matrix->pos = *(RwV3d*)&mkobj->pos.value;
    matrix->flags &= ~0x20000;
    if ((mkobj->hide_flags & 0x10) == 0) {
        RwFrameUpdateObjects(mkobj->frame);
        for (i = 1; i < mkobj->clump_count; i++) {
            frame = (RwFrame*)mkobj->clumps[i]->object.parent;
            frame_matrix = (RwMatrix*)((char*)frame + 0x10);
            frame_matrix->pos = matrix->pos;
            RwFrameUpdateObjects(frame);
        }
    }
    if (mkobj->matrix_count > 1) {
        sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
        if (sobj != 0 && sobj->matrices != 0) {
            for (i = 1; i < (int)mkobj->matrix_count; i++) {
                matrix_index = mkobj->matrix_indices[i];
                sobj->matrices[matrix_index].pos = matrix->pos;
            }
        }
    }
}

void set_bone_world_pos(void* obj, int bone, void* pos) {
    MkObj* mkobj;
    MkBone* mkbone;
    RwMatrix* parent;
    RwMatrix* matrix;
    RwFrame* frame;
    RwMatrix* frame_matrix;
    MkSobj* sobj;
    Vec current;
    Vec* target;
    int i;
    int matrix_index;

    mkobj = (MkObj*)obj;
    target = (Vec*)pos;
    if (bone >= (int)mkobj->bone_count) {
        current = mkobj->pos.value;
    } else if (bone == -1) {
        current = mkobj->pos.value;
    } else {
        mkbone = mkobj->bones[bone];
        if (mkbone == 0 || (parent = mkbone->parent_matrix) == 0) {
            current.x = mkobj->pos.value.x;
            current.y = mkobj->pos.value.y;
            current.z = mkobj->pos.value.z;
        } else {
            v3_x_mat_add_v3(
                &current, (Vec*)&parent->pos,
                (MKMATRIX*)mkobj->field_24, &mkobj->pos.value);
        }
    }
    mkobj->pos.value.x += target->x - current.x;
    mkobj->pos.value.y += target->y - current.y;
    mkobj->pos.value.z += target->z - current.z;
    mkobj->flags_08_bits.bit7 = 1;

    matrix = mkobj->field_24;
    matrix->pos = *(RwV3d*)&mkobj->pos.value;
    matrix->flags &= ~0x20000;
    if ((mkobj->hide_flags & 0x10) == 0) {
        RwFrameUpdateObjects(mkobj->frame);
        for (i = 1; i < mkobj->clump_count; i++) {
            frame = (RwFrame*)mkobj->clumps[i]->object.parent;
            frame_matrix = (RwMatrix*)((char*)frame + 0x10);
            frame_matrix->pos = matrix->pos;
            RwFrameUpdateObjects(frame);
        }
    }
    if (mkobj->matrix_count > 1) {
        sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
        if (sobj != 0 && sobj->matrices != 0) {
            for (i = 1; i < (int)mkobj->matrix_count; i++) {
                matrix_index = mkobj->matrix_indices[i];
                sobj->matrices[matrix_index].pos = matrix->pos;
            }
        }
    }
}

void* get_bone_with_tag(void* obj, int tag) {
    MkObj* mkobj;
    MkBone* bone;
    unsigned int i;

    mkobj = (MkObj*)obj;
    for (i = 0; i < mkobj->bone_count; i++) {
        bone = mkobj->bones[i];
        if (bone != 0 && bone->tag == tag) {
            return bone;
        }
    }
    return 0;
}

void get_bone_relative_pos(MkObj* obj, int bone, Vec* out) {
    MkBone* mkbone;

    if (bone >= (int)obj->bone_count) {
        *out = obj->pos.value;
        return;
    }
    if (bone == -1) {
        *out = obj->pos.value;
        return;
    }
    mkbone = obj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        out->x = obj->pos.value.x;
        out->y = obj->pos.value.y;
        out->z = obj->pos.value.z;
    } else {
        out->x = mkbone->parent_matrix->pos.x;
        out->y = mkbone->parent_matrix->pos.y;
        out->z = mkbone->parent_matrix->pos.z;
    }
}

void get_bone_world_pos(MkObj* obj, int bone, Vec* out) {
    MkBone* mkbone;

    if (bone >= (int)obj->bone_count) {
        *out = obj->pos.value;
        return;
    }
    if (bone == -1) {
        *out = obj->pos.value;
        return;
    }
    mkbone = obj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        out->x = obj->pos.value.x;
        out->y = obj->pos.value.y;
        out->z = obj->pos.value.z;
        return;
    }
    v3_x_mat_add_v3(
        out, (Vec*)&mkbone->parent_matrix->pos,
        (MKMATRIX*)obj->field_24, (Vec*)&obj->pos.value);
}

void update_bone_hierarchy(void* obj) {
    MkObj* mkobj;
    MkBone* queue[152];
    MkBone* root;
    MkBone* bone;
    MkBone* walk;
    MkBone* parent;
    Quat* rotation;
    RwMatrix* matrix;
    RwMatrixPosition saved_pos;
    Vec impulse;
    unsigned int read_index;
    unsigned int count;

    mkobj = (MkObj*)obj;
    saved_pos.value.x = 0.0f;
    saved_pos.value.y = 0.0f;
    saved_pos.value.z = 0.0f;
    bone = mkobj->bones[mkobj->fallback_bone_index];
    if (mkobj->hide_flag_bits.bit0 != 0) {
        mkobj->hide_flag_bits.bit0 = 0;
        mkobj->flags_0B_bits.root_transform_pending = 1;
        mkobj->bone_angle_68 = mkobj->bone_angle_64 =
            quat_extract_ang_y(&bone->rotation);
    }

    root = mkobj->bones[mkobj->fallback_bone_index];
    read_index = 0;
    count = 1;
    queue[read_index] = root;
    walk = root->root_next;
    while (walk != 0) {
        queue[count] = walk;
        count++;
        walk = walk->root_next;
    }

    while (read_index != count) {
        bone = queue[read_index];
        read_index++;
        if (bone->parent_matrix == 0) {
            continue;
        }
        walk = bone->tree_child;
        while (walk != 0) {
            queue[count] = walk;
            count++;
            walk = walk->tree_next;
        }

        walk = bone->clone_source;
        if (walk != 0 && walk->parent_matrix != 0) {
            bone->delta = walk->delta;
            bone->velocity = walk->velocity;
            gxQuatCopy(&bone->rotation_90, &walk->rotation_90);
            memcpy(&bone->matrix, &walk->matrix, sizeof(RwMatrix));
            if (bone->parent_matrix != walk->parent_matrix) {
                memcpy(bone->parent_matrix, walk->parent_matrix,
                       sizeof(RwMatrix));
            }
            continue;
        }
        rotation = &bone->rotation_90;

        if (bone->flags_54_bits.field_bit3 != 0) {
            saved_pos = bone->matrix.pos_row;
        }

        parent = bone->transform_parent;
        if (parent != 0 && parent->parent_matrix == 0) {
            parent = 0;
        }
        if (parent != 0) {
            bone->flags_55_bits.collision_deferred =
                bone->flags_55_bits.collision_disabled |
                parent->flags_55_bits.collision_deferred;
        } else {
            bone->flags_55_bits.collision_deferred =
                bone->flags_55_bits.collision_disabled;
        }
        if (bone->flags_55_bits.collision_deferred == 0 &&
            bone->flags_54_bits.hierarchy_driven != 0) {
            continue;
        }
        if (bone->flags_55_bits.collision_deferred != 0 ||
            bone->flags_54_bits.pose_matrix_applied == 0) {
            matrix = bone->parent_matrix;
            if (parent != 0) {
                if (bone->flags_55_bits.collision_deferred != 0) {
                    gxQuatSetZero(rotation);
                    rotation->w = 0.0f;
                    matrix->pos_row = parent->parent_matrix->pos_row;
                } else {
                    gxQuatMul(rotation, &parent->rotation_90,
                              &bone->rotation);
                    gxMatV3MatAddV3(
                        (Vec*)&matrix->pos, &bone->translation.value,
                        (Mat33*)parent->parent_matrix,
                        (Vec*)&parent->parent_matrix->pos);
                }
            } else {
                gxQuatCopy(rotation, &bone->rotation);
                matrix->pos_row = bone->translation;
            }

            if (bone->flags_55_bits.collision_deferred != 0) {
                matrix->at.z = 0.0f;
                matrix->at.y = 0.0f;
                matrix->at.x = 0.0f;
                matrix->up.z = 0.0f;
                matrix->up.y = 0.0f;
                matrix->up.x = 0.0f;
                matrix->right.z = 0.0f;
                matrix->right.y = 0.0f;
                matrix->right.x = 0.0f;
                matrix->flags = 0;
            } else {
                if (rotation->x == rotation->y &&
                    rotation->y == rotation->z &&
                    rotation->z == rotation->w) {
                    rotation->w = 1.0f;
                }
                gxQuatQuatToMat(RW_MATRIX_MAT33(matrix), rotation);
                if (bone->flags_55_bits.scale_controlled != 0) {
                    mat_scaled_by_v3(
                        (MKMATRIX*)matrix, (MKMATRIX*)matrix, &bone->scale);
                }
            }
        }

        if (bone->flags_54_bits.calculation_locked != 0) {
            matrix = mkobj->field_24;
            gxMat33x33(
                (Mat33*)&bone->matrix, (Mat33*)bone->parent_matrix,
                (Mat33*)matrix);
            gxMatV3MatAddV3(
                (Vec*)&bone->matrix.pos, (Vec*)&bone->parent_matrix->pos,
                (Mat33*)matrix, (Vec*)&matrix->pos);
        }
        if (bone->flags_54_bits.field_bit3 != 0) {
            PSVECSubtract((Vec*)&bone->matrix.pos, &saved_pos.value,
                          &bone->delta.value);
            PSVECScale(
                &bone->velocity.value, &bone->velocity.value, 0.8f);
            PSVECScale(&bone->delta.value, &impulse, 0.2f);
            PSVECAdd(
                &bone->velocity.value, &impulse, &bone->velocity.value);
        }
    }
}

void mkobj_bones_dest_mat_no_update(MkObj* obj) {
    MkBone* bone;
    unsigned int i;

    for (i = 0; i < obj->bone_count; i++) {
        bone = obj->bones[i];
        if (bone != 0) {
            bone->flags_54_bits.pose_matrix_applied = 1;
        }
    }
}

void mkobj_zero_bone_rots(void* obj) {
    MkObj* mkobj;
    MkBone* bone;
    unsigned int i;

    mkobj = (MkObj*)obj;
    for (i = 0; i < mkobj->bone_count; i++) {
        bone = mkobj->bones[i];
        if (bone != 0) {
            gxQuatSetZero(&bone->rotation);
            gxQuatSetZero(&bone->rotation_e0);
            gxQuatSetZero(&bone->rotation_90);
            if (bone->parent_matrix != 0) {
                bone->parent_matrix->at.z = 1.0f;
                bone->parent_matrix->up.y = 1.0f;
                bone->parent_matrix->right.x = 1.0f;
                bone->parent_matrix->up.x = 0.0f;
                bone->parent_matrix->right.z = 0.0f;
                bone->parent_matrix->right.y = 0.0f;
                bone->parent_matrix->at.y = 0.0f;
                bone->parent_matrix->at.x = 0.0f;
                bone->parent_matrix->up.z = 0.0f;
                bone->parent_matrix->pos.z = 0.0f;
                bone->parent_matrix->pos.y = 0.0f;
                bone->parent_matrix->pos.x = 0.0f;
                bone->parent_matrix->flags |= 0x20003;
            }
            bone->flags_54_bits.pose_matrix_applied = 0;
        }
    }
    update_mkobj(obj != 0 ? as_mkhdr(&mkobj->hdr) : 0);
}

static inline RpMaterial* find_geometry_material_by_id(
    RpGeometry* geometry, unsigned int id) {
    RpMaterial* material;
    unsigned int count;
    unsigned int i;
    unsigned int offset;

    count = geometry->matList.numMaterials;
    offset = 0;
    for (i = 0; i < count; i++) {
        material =
            *(RpMaterial**)((char*)geometry->matList.materials + offset);
        if ((MK_MATERIAL_PLUGIN(material)->flags & 0xFFF) == id) {
            return material;
        }
        offset += sizeof(material);
    }
    return 0;
}

MkProc* fade_material(float delta, MkObj* obj, unsigned int sobj_id,
                      unsigned int material_id, int frames) {
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    FadeMaterialPdata* pdata;
    MkProc* proc;

    material = 0;
    sobj = find_sobj_by_id_inline(obj, sobj_id);
    if (sobj == 0) {
        return 0;
    }

    geometry = sobj->atomic->geometry;
    material = find_geometry_material_by_id(geometry, material_id);
    if (material == 0) {
        return 0;
    }

    proc = _create_mkproc_generic_nostack(
        0x5010, 0x20, p_fade_material, sizeof(FadeMaterialPdata),
        (MkHdr**)&pdata);
    if (pdata != 0) {
        pdata->obj = obj;
        pdata->obj_instance = obj->hdr.instance;
        pdata->delta = delta;
        pdata->frames = frames;
        pdata->material = material;
        pdata->accumulator = 0.0f;
    }
    return proc;
}

static float p_fade_material(void) {
    MkObj* obj;
    FadeMaterialPdata* pdata;
    RpMaterialColor* color;
    float accumulated;
    int amount;
    int value;

    pdata = (FadeMaterialPdata*)apdata;
    if (aproc->pid != 0x5010 || pdata == 0) {
        mkproc_die();
    }
    obj = pdata->obj;
    if (obj != 0) {
        if (obj->hdr.instance != pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj == 0) {
        mkproc_die();
    }

    accumulated = pdata->accumulator + pdata->delta;
    amount = (int)accumulated;
    pdata->accumulator = accumulated - (float)amount;
    color = &pdata->material->color;
    value = color->red + amount;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    color->red = (unsigned char)value;
    value = color->green + amount;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    color->green = (unsigned char)value;
    value = color->blue + amount;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    color->blue = (unsigned char)value;
    value = color->alpha + amount;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    color->alpha = (unsigned char)value;
    if (color->red == 0 && color->green == 0 && color->blue == 0) {
        obj->hide_flag_bits.hidden = 1;
        mkproc_die();
    }
    pdata->frames--;
    if (pdata->frames <= 0) {
        mkproc_die();
    }
    return 1.0f;
}

void obj_for_all_atomics_set_material_alpha(MkObj* obj, int alpha) {
    RpClump* clump;
    int i;

    for (i = 0; i < obj->clump_count; i++) {
        clump = obj->clumps[i];
        if (clump != 0) {
            RpClumpForAllAtomics(
                clump, force_atomic_material_alpha, (void*)alpha);
        }
    }
}

void obj_set_material_fade(MkObj* obj, unsigned int id, signed char alpha) {
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    RpMaterialColor color;

    material = 0;
    ptr = first_mkptr(&obj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        geometry = sobj->atomic->geometry;
        material = find_geometry_material_by_id(geometry, id);
        if (material != 0) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (material != 0) {
        color.red = alpha;
        color.green = alpha;
        color.blue = alpha;
        color.alpha = alpha;
        material->color = color;
    }
}

void sobj_use_material_color(void* sobj) {
    if (((MkSobj*)sobj)->atomic->geometry == 0) {
        return;
    }
    ((MkSobj*)sobj)->atomic->geometry->flags |= 0x40;
}

void obj_set_z_offsets(float offset, void* obj) {
    MkObj* mkobj;
    SobjCreateData data;
    MkPtr* ptr;
    MkSobj* sobj;
    int i;

    mkobj = (MkObj*)obj;
    if (mkobj->sobj_list == 0) {
        data.obj = mkobj;
        data.id = 0;
        data.mask = 0;
        data.priority = 0x10;
        data.set_priority = 0;
        for (i = 0; i < mkobj->clump_count; i++) {
            if (mkobj->clumps[i] != 0) {
                RpClumpForAllAtomics(
                    mkobj->clumps[i], atomic_create_sobj_callback, &data);
            }
        }
    }
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if (sobj != 0) {
            sobj->z_offset = offset;
        }
        ptr = next_mkptr(ptr);
    }
}

MkSobj* obj_first_sobj(MkObj* obj) {
    return (MkSobj*)first_mkhdr(&obj->sobj_list);
}

void update_mksobj(MkSobj* sobj) {
    MkSobj* mksobj;
    RwMatrix* matrix;
    RwFrame* face_frame;
    RwMatrix* face_matrix;
    RwMatrix* camera_matrix;
    int changed;

    mksobj = sobj;
    matrix = (RwMatrix*)((char*)mksobj->frame + 0x10);
    changed = 0;

    if (mksobj->flags_08_bits.bit5 != 0) {
        mksobj->pos.x += mksobj->pos_vel.x * obj_game_speed;
        mksobj->pos.y += mksobj->pos_vel.y * obj_game_speed;
        mksobj->pos.z += mksobj->pos_vel.z * obj_game_speed;
    }
    if (mksobj->flags_08_bits.bit5 != 0 ||
        mksobj->flags_08_bits.bit6 != 0 ||
        mksobj->flags_08_bits.bit7 != 0) {
        changed = 1;
        mksobj->flags_08_bits.bit7 = 0;
        matrix->pos = *(RwV3d*)&mksobj->pos;
    }

    if (mksobj->flags_08_bits.angular_velocity_enabled != 0) {
        mksobj->ang.x += mksobj->ang_vel.x * obj_game_speed;
        mksobj->ang.y += mksobj->ang_vel.y * obj_game_speed;
        mksobj->ang.z += mksobj->ang_vel.z * obj_game_speed;
    }
    if (mksobj->flags_08_bits.angular_velocity_enabled != 0 ||
        mksobj->flags_08_bits.bit3 != 0 ||
        mksobj->flags_08_bits.bit4 != 0 ||
        mksobj->flags_08_bits.scale_dirty != 0) {
        mksobj->flags_08_bits.bit4 = 0;
        mksobj->ang.x = normalize_obj_angle(mksobj->ang.x);
        mksobj->ang.y = normalize_obj_angle(mksobj->ang.y);
        mksobj->ang.z = normalize_obj_angle(mksobj->ang.z);
        YXZ_angles_to_MKMATRIX(
            &mksobj->ang, (MKMATRIX*)matrix);
        changed = 1;
    }
    if (mksobj->flags09_bits.bit5 != 0) {
        if (mksobj->atomic != 0) {
            face_frame = (RwFrame*)mksobj->atomic->object.parent;
            if (face_frame != 0) {
                face_frame->object.privateFlags |= 0x20;
                face_matrix = (RwMatrix*)((char*)face_frame + 0x10);
                camera_matrix = (RwMatrix*)camera_facing_matrix_ay;
                face_matrix->right = camera_matrix->right;
                face_matrix->up = camera_matrix->up;
                face_matrix->at = camera_matrix->at;
                RwFrameUpdateObjects(face_frame);
            }
        }
    }
    if (mksobj->flags_08_bits.scale_dirty != 0) {
        mat_scaled_by_v3(
            (MKMATRIX*)matrix, (MKMATRIX*)matrix, &mksobj->scale);
        changed = 1;
    }
    if (changed != 0) {
        matrix->flags &= ~0x20000;
        RwFrameUpdateObjects(mksobj->frame);
    }
}

static void update_mkhdr_sobj(MkHdr* hdr) {
    update_mksobj((MkSobj*)hdr);
}

void vdestroy_mksobj(MkSobj* sobj) {
    MkSobj* mksobj;
    MkHdr* raw_bound;
    MkHdr* bound;

    mksobj = sobj;
    mksobj->hdr.instance = 0;
    raw_bound = mksobj->bound_hdr;
    if (raw_bound != 0) {
        if (raw_bound->instance == mksobj->bound_instance) {
            bound = raw_bound;
        } else {
            bound = 0;
        }
    } else {
        bound = 0;
    }
    if (bound != 0) {
        bound = mksobj->bound_hdr;
        if (bound->instance != 0) {
            ((void (*)(MkHdr*))bound->vtbl->destroy)(bound);
        }
        mksobj->bound_hdr = 0;
        mksobj->bound_instance = 0;
    }
    if (mksobj->matrices != 0) {
        RwEngineInstance->free(mksobj->matrices);
    }
    _mwMemFree(mksobj, 0, 0);
}

void obj_set_sobj_pos(MkObj* obj, unsigned int id, const Vec* pos) {
    MkSobj* sobj;

    sobj = find_sobj_by_id_inline(obj, id);
    if (sobj != 0) {
        sobj->pos.x = pos->x;
        sobj->pos.y = pos->y;
        sobj->pos.z = pos->z;
        sobj->flags_08_bits.bit7 = 1;
    }
}

MkSobj* obj_find_sobj_by_id(MkObj* obj, unsigned int id) {
    MkPtr* ptr;
    MkSobj* sobj;

    ptr = first_mkptr(&obj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFFu) == id) {
            return sobj;
        }
        ptr = next_mkptr(ptr);
    }
    return 0;
}

void obj_force_culling_off(MkObj* obj) {
    MkObj* mkobj;
    SobjCreateData data;
    MkPtr* ptr;
    MkSobj* sobj;
    int i;

    mkobj = obj;
    data.obj = mkobj;
    data.id = 0;
    data.mask = 0;
    data.priority = 0x10;
    data.set_priority = 0;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (mkobj->clumps[i] != 0) {
            RpClumpForAllAtomics(
                mkobj->clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if (sobj != 0) {
            sobj->flags09_bits.bit4 = 1;
        }
        ptr = next_mkptr(ptr);
    }
}

void obj_apply_to_sobj_with_id(
    MkObj* obj, unsigned int id, void (*callback)(MkSobj*)) {
    MkPtr* ptr;
    MkSobj* sobj;

    ptr = first_mkptr(&obj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if (id == 0 || (sobj->id_flags & 0xFFF) == id) {
            callback(sobj);
        }
        ptr = next_mkptr(ptr);
    }
}

typedef struct MaterialTextureSwap {
    unsigned int material_id;
    RwTexture* texture;
} MaterialTextureSwap;

typedef struct MaterialTextureFind {
    const char* name;
    RpMaterial* material;
    RwTexture* texture;
} MaterialTextureFind;

static RpMaterial* material_set_texture(RpMaterial* material, void* data);
static RpAtomic* atomic_find_texture_callback(RpAtomic* atomic, void* data);
static RpMaterial* material_find_texture_callback(
    RpMaterial* material, void* data);
static RpAtomic* obj_get_1st_atomic_callback(RpAtomic* atomic, void* data);
static void _destroy_mkobj_oid_mask(MkHdr* obj);
static RpAtomic* atomic_null_texture_pointers(
    RpAtomic* atomic, void* data);
static RpMaterial* material_null_texture_pointer(
    RpMaterial* material, void* data);

void sobj_swap_material_texture(void* sobj, void* a, void* b) {
    MaterialTextureSwap swap;
    RpGeometry* geometry;

    swap.material_id = (unsigned int)a;
    swap.texture = (RwTexture*)b;
    geometry = ((MkSobj*)sobj)->atomic->geometry;
    if (geometry != 0) {
        RpGeometryForAllMaterials(geometry, material_set_texture, &swap);
    }
}

static RpMaterial* material_set_texture(RpMaterial* material, void* data) {
    MaterialTextureSwap* swap = (MaterialTextureSwap*)data;
    MkmaterialPluginData* mkmat;

    mkmat = (MkmaterialPluginData*)((char*)material + MkmaterialLocalOffset);
    if (swap->material_id != (mkmat->flags & 0xFFF)) {
        return material;
    }
    material->texture = swap->texture;
    return 0;
}

void obj_set_all_sobjs_priority(MkObj* obj, int priority) {
    RpClump* clump;
    SobjCreateData data;
    int i;

    data.obj = obj;
    data.id = 0;
    data.mask = 0;
    data.priority = priority;
    data.set_priority = 1;
    for (i = 0; i < obj->clump_count; i++) {
        clump = obj->clumps[i];
        if (clump != 0) {
            RpClumpForAllAtomics(
                clump, atomic_create_sobj_callback, &data);
        }
    }
}

MkSobj* obj_create_sobjs_by_id(MkObj* obj, int id) {
    struct SobjCreateData data;
    int i;

    data.result = 0;
    data.obj = obj;
    data.id = (unsigned int)id;
    data.mask = 0xFFF;
    data.priority = 0x10;
    data.set_priority = 0;
    for (i = 0; i < obj->clump_count; i++) {
        if (obj->clumps[i] != 0) {
            RpClumpForAllAtomics(
                obj->clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    return data.result;
}

void obj_create_sobjs(MkObj* obj) {
    RpClump* clump;
    SobjCreateData data;
    int i;

    data.obj = obj;
    data.id = 0;
    data.mask = 0;
    data.priority = 0x10;
    data.set_priority = 0;
    for (i = 0; i < obj->clump_count; i++) {
        clump = obj->clumps[i];
        if (clump != 0) {
            RpClumpForAllAtomics(
                clump, atomic_create_sobj_callback, &data);
        }
    }
}

static RpAtomic* atomic_create_sobj_callback(
    RpAtomic* atomic, void* dataArg) {
    struct SobjCreateData* data;
    MkObj* owner;
    MkSobj* sobj;
    MkSobj* new_sobj;
    unsigned int flags;
    RwFrame* frame;
    RwMatrix* frameMatrix;

    data = (struct SobjCreateData*)dataArg;
    flags = MK_ATOMIC_PLUGIN(atomic)->flags;
    if (data->mask != 0) {
        if (data->id == 0) {
            if ((flags & data->mask) == 0) {
                return atomic;
            }
        } else if ((data->id & data->mask) !=
                   (flags & data->mask)) {
            return atomic;
        }
    }
    sobj = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (sobj == 0) {
        owner = data->obj;
        frame = (RwFrame*)atomic->object.parent;
        new_sobj = (MkSobj*)_mwMemMalloc(
            mksobj_heap, 0x90, 0x80, 0, 0, 0);
        if (new_sobj != 0) {
            new_sobj->hdr.vtbl = (MkVtable5*)&vtbl_mksobj;
            mk_set_instance(&new_sobj->hdr.instance);
            new_sobj->bound_hdr = 0;
            new_sobj->bound_instance = 0;
            new_sobj->id_flags = flags;
            if ((flags & 0x80000000) != 0) {
                new_sobj->priority = 0x12;
            } else {
                new_sobj->priority = 0x10;
            }
            new_sobj->frame = frame;
            new_sobj->flags_08 = 0;
            new_sobj->flags_08_bits.bit7 = 1;
            new_sobj->flags_08_bits.bit0 = 1;
            new_sobj->render_flags = 0;
            frameMatrix = &frame->modelling;
            new_sobj->pos = *(Vec*)&frameMatrix->pos;
            new_sobj->pos_vel.z = 0.0f;
            new_sobj->pos_vel.y = 0.0f;
            new_sobj->pos_vel.x = 0.0f;
            new_sobj->ang.z = 0.0f;
            new_sobj->ang.y = 0.0f;
            new_sobj->ang.x = 0.0f;
            new_sobj->ang_vel.z = 0.0f;
            new_sobj->ang_vel.y = 0.0f;
            new_sobj->ang_vel.x = 0.0f;
            new_sobj->z_offset = 0.0f;
            new_sobj->matrices = 0;
        }
        new_sobj->atomic = atomic;
        sobj = new_sobj;
        mk_insert(&new_sobj->hdr, &owner->sobj_list);
        new_sobj->owner = owner;
        MK_ATOMIC_PLUGIN(atomic)->sobj = new_sobj;
    }
    if (data->set_priority != 0) {
        sobj_set_priority(sobj, data->priority);
    }
    data->result = sobj;
    return atomic;
}

void material_restore_reflection_texture(void* material) {
    SpecularMaterialPluginData* spec;

    spec = (SpecularMaterialPluginData*)((char*)material + SpecularMaterialOffset);
    if (spec->saved_texture != 0) {
        spec->texture = spec->saved_texture;
        spec->saved_texture = 0;
    }
}

void material_cache_reflection_texture(void* material) {
    SpecularMaterialPluginData* spec;

    spec = (SpecularMaterialPluginData*)((char*)material + SpecularMaterialOffset);
    if (spec->saved_texture == 0) {
        spec->saved_texture = spec->texture;
        spec->texture = 0;
    }
}

void material_set_reflection_texture(void* material, void* tex) {
    SpecularMaterialPluginData* spec;

    spec = (SpecularMaterialPluginData*)((char*)material + SpecularMaterialOffset);
    spec->texture = (RwTexture*)tex;
}

void material_set_zbias(void* material, float zbias) {
    MkmaterialPluginData* mkmat;
    SpecularMaterialPluginData* spec;

    mkmat = (MkmaterialPluginData*)((char*)material + MkmaterialLocalOffset);
    spec = (SpecularMaterialPluginData*)((char*)material + SpecularMaterialOffset);
    mkmat->z_bias = zbias;
    spec->gloss = zbias;
}

RpMaterial* obj_find_material_by_id(MkObj* obj, int id) {
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;

    material = 0;
    ptr = first_mkptr(&obj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        geometry = sobj->atomic->geometry;
        material = find_geometry_material_by_id(
            geometry, (unsigned int)id);
        if (material != 0) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    return material;
}

RpMaterial* sobj_find_material_by_id(MkSobj* sobj, unsigned int id) {
    return find_geometry_material_by_id(sobj->atomic->geometry, id);
}

RpMaterial* sobj_find_material_with_texture(
    MkSobj* sobj, const char* texture_name) {
    MaterialTextureFind find;

    find.name = texture_name;
    find.texture = 0;
    find.material = 0;
    if (sobj->atomic != 0) {
        RpGeometryForAllMaterials(
            sobj->atomic->geometry, material_find_texture_callback, &find);
    }
    return find.material;
}

RpMaterial* obj_find_material_with_texture(
    MkObj* obj, const char* texture_name) {
    MaterialTextureFind find;

    find.name = texture_name;
    find.texture = 0;
    find.material = 0;
    if (obj->clump != 0) {
        RpClumpForAllAtomics(
            obj->clump, atomic_find_texture_callback, &find);
    }
    return find.material;
}

static RpAtomic* atomic_find_texture_callback(RpAtomic* atomic, void* data) {
    MaterialTextureFind* find;

    find = (MaterialTextureFind*)data;
    RpGeometryForAllMaterials(
        atomic->geometry, material_find_texture_callback, find);
    if (find->texture != 0) {
        return 0;
    }
    return atomic;
}

static RpMaterial* material_find_texture_callback(
    RpMaterial* material, void* data) {
    MaterialTextureFind* find;

    find = (MaterialTextureFind*)data;
    if (material->texture != 0 &&
        stricmp(material->texture->name, find->name) == 0) {
        find->material = material;
        find->texture = material->texture;
        return 0;
    }
    return material;
}

RpAtomic* obj_get_1st_atomic(MkObj* obj) {
    RpClumpForAllAtomics(
        obj->clump, obj_get_1st_atomic_callback, 0);
    return atomic1;
}

static RpAtomic* obj_get_1st_atomic_callback(RpAtomic* atomic, void* data) {
    atomic1 = atomic;
    return 0;
}

void destroy_mkobjs_oid(int oid) {
    oid_to_kill = (unsigned int)oid;
    oid_to_kill_mask = 0xFFFFFFFF;
    apply_to_mklist(_destroy_mkobj_oid_mask, &fgnd_mkobj_list);
    apply_to_mklist(_destroy_mkobj_oid_mask, &particle_mkobj_list);
}

static void _destroy_mkobj_oid_mask(MkHdr* obj) {
    MkObj* mkobj = (MkObj*)obj;
    RpClump* clump;
    int i;

    if ((mkobj->oid & oid_to_kill_mask) != oid_to_kill) {
        return;
    }

    mkobj->hdr.instance = 0;
    if (mkobj->hide_flag_bits.bit3 != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    for (i = 0; i < mkobj->clump_count; i++) {
        clump = mkobj->clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            mkobj->clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->cloth_bones != 0) {
        free_mem(mkobj->cloth_bones);
    }
    if (mkobj->matrix_indices != 0) {
        free_mem(mkobj->matrix_indices);
    }
    mkobj->field_5C = 0;
    _mwMemFree(mkobj, 0, 0);
}

void start_obj_proc(void) {
    int flags[2];
    MkProc* proc;

    flags[1] = 0;
    fgnd_mkobj_list = 0;
    particle_mkobj_list = 0;
    bgnd_light_list = 0;
    bgnd_spec_light_list = 0;
    point_light_list = 0;
    board_piece_light_list = 0;
    skinned_obj_light_list = 0;
    gore2_light_list = 0;
    fgnd_light_list = 0;
    plyr_light_list = 0;
    special_light_list = 0;
    freeze_light_list = 0;
    clone_light_list = 0;
    weapon_trail_light_list = 0;
    hol_plight_list = 0;
    pfx_render_list = 0;
    pfx_clone_render_list = 0;
    flags[0] = flags[1];
    proc = get_mkproc_nostack(&flags[0]);
    create_mkproc(0xF, proc, 0x5001, p_obj, 0);
}

void init_weapon_trail_light_list(void) {
    load_light(
        (LightDef*)&hot_ambient_light,
        &weapon_trail_light_list, 0);
}

static float p_obj(void) {
    update_camera_facing_matrix();
    apply_to_mklist(
        (MkListApplyFn)update_mkobj, &fgnd_mkobj_list);
    apply_to_mklist(
        (MkListApplyFn)update_mkobj, &particle_mkobj_list);
    return 1.0f;
}

void obj_match_pos_ang_to_src_obj(MkObj* dst_obj, MkObj* src_obj) {
    MkHdr* hdr;

    dst_obj->pos.value = src_obj->pos.value;
    dst_obj->pos_vel = src_obj->pos_vel;
    dst_obj->ang = src_obj->ang;
    dst_obj->ang_vel = src_obj->ang_vel;
    dst_obj->scale = src_obj->scale;
    dst_obj->flags_word_08 = src_obj->flags_word_08;
    dst_obj->flags_08_bits.gravity_enabled = 0;
    dst_obj->flags_08_bits.rotation_enabled = 0;
    dst_obj->flags_08_bits.bit7 = 1;
    dst_obj->flags_08_bits.transform_dirty = 1;
    if (dst_obj != 0) {
        hdr = as_mkhdr(&dst_obj->hdr);
    } else {
        hdr = 0;
    }
    update_mkobj(hdr);
    dst_obj->flags_word_08 = src_obj->flags_word_08;
    dst_obj->light_flags = src_obj->light_flags;
}

void update_obj_pos(MkObj* mkobj) {
    RwMatrix* matrix;
    RwMatrix* matrices;
    RwFrame* frame;
    RwMatrix* frame_matrix;
    MkSobj* sobj;
    int i;
    int matrix_index;

    matrix = mkobj->field_24;
    matrix->pos = *(RwV3d*)&mkobj->pos.value;
    matrix->flags &= ~0x20000;
    if (mkobj->hide_flag_bits.bit4 == 0) {
        RwFrameUpdateObjects(mkobj->frame);
        for (i = 1; i < mkobj->clump_count; i++) {
            frame = (RwFrame*)mkobj->clumps[i]->object.parent;
            frame_matrix = (RwMatrix*)((char*)frame + 0x10);
            frame_matrix->pos = matrix->pos;
            RwFrameUpdateObjects(frame);
        }
    }

    if (mkobj->matrix_count > 1) {
        sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
        if (sobj != 0 && sobj->matrices != 0) {
            matrices = sobj->matrices;
            for (i = 1; i < (int)mkobj->matrix_count; i++) {
                matrix_index = mkobj->matrix_indices[i];
                matrices[matrix_index].pos = matrix->pos;
            }
        }
    }
}

void update_mkobj(void* obj) {
    MkObj* mkobj;
    RwMatrix* matrix;
    RwFrame* frame;
    RwMatrix* frameMatrix;
    RwMatrix* matrices;
    MkSobj* sobj;
    Vec pivot;
    Vec scaled_ang;
    Vec scaled_pos;
    int changed;
    int i;
    int matrixIndex;

    mkobj = (MkObj*)obj;
    changed = 0;
    apply_to_mklist(update_mkhdr_sobj, &mkobj->sobj_list);
    if (mkobj->flags_0B_bits.force_anim_speed != 0) {
        obj_game_speed = 1.0f;
    } else {
        obj_game_speed = game_speed;
    }
    matrix = mkobj->field_24;

    if (mkobj->flags_08_bits.rotation_enabled != 0) {
        PSVECScale(&mkobj->ang_vel, &scaled_ang, obj_game_speed);
        PSVECAdd(&mkobj->ang, &scaled_ang, &mkobj->ang);
    }
    if (mkobj->flags_08_bits.rotation_enabled != 0 ||
        mkobj->flags_08_bits.angular_velocity_enabled != 0 ||
        mkobj->flags_08_bits.transform_dirty != 0 ||
        mkobj->flags_08_bits.scale_active != 0) {
        mkobj->flags_08_bits.transform_dirty = 0;
        mkobj->ang.x = normalize_obj_angle(mkobj->ang.x);
        mkobj->ang.y = normalize_obj_angle(mkobj->ang.y);
        mkobj->ang.z = normalize_obj_angle(mkobj->ang.z);
        YXZ_angles_to_MKMATRIX(
            &mkobj->ang, (MKMATRIX*)matrix);
        changed = 1;
    }
    if (mkobj->flags_09_bits.bit0 != 0 && mkobj->clump != 0) {
        RpClumpForAllAtomics(
            mkobj->clump, (RpAtomicCallBack)AtomicFaceCamera, 0);
    }
    if (mkobj->flags_08_bits.moving != 0) {
        mkobj->flags_08_bits.gravity_enabled = 1;
        mkobj->pos_vel.y += mkobj->gravity * obj_game_speed;
    }
    if (mkobj->flags_08_bits.gravity_enabled != 0) {
        PSVECScale(&mkobj->pos_vel, &scaled_pos, obj_game_speed);
        PSVECAdd(&mkobj->pos.value, &scaled_pos, &mkobj->pos.value);
    }
    if (mkobj->flags_0B_bits.pivot_enabled != 0) {
        gxMat33Tx31(
            &pivot, &mkobj->pivot, (Mat33*)matrix);
        PSVECSubtract(&mkobj->pos.value, &pivot, (Vec*)&matrix->pos);
        changed = 1;
    } else if (mkobj->flags_08_bits.gravity_enabled != 0 ||
               mkobj->flags_08_bits.airborne != 0 ||
               mkobj->flags_08_bits.bit7 != 0) {
        changed = 1;
        mkobj->flags_08_bits.bit7 = 0;
        matrix->pos = *(RwV3d*)&mkobj->pos.value;
    }
    if (mkobj->flags_08_bits.scale_active != 0) {
        gxMatScaledByV3(
            (Mat33*)matrix, (Mat33*)matrix, &mkobj->scale);
        changed = 1;
    }

    if (changed != 0) {
        matrix->flags &= ~0x20000;
        if (mkobj->hide_flag_bits.bit4 == 0) {
            RwFrameUpdateObjects(mkobj->frame);
            for (i = 1; i < mkobj->clump_count; i++) {
                frame = (RwFrame*)mkobj->clumps[i]->object.parent;
                frameMatrix = &frame->modelling;
                memcpy(frameMatrix, matrix, sizeof(RwMatrix));
                RwFrameUpdateObjects(frame);
            }
        }
    }
    if (mkobj->matrix_count != 0) {
        sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
        if (sobj != 0 && sobj->matrices != 0) {
            matrices = sobj->matrices;
            for (i = 1; i < (int)mkobj->matrix_count; i++) {
                matrixIndex = mkobj->matrix_indices[i];
                memcpy(
                    &matrices[matrixIndex], matrix,
                    sizeof(RwMatrix));
            }
        }
    }
}

static void* AtomicFaceCamera(void* atomic, void* data) {
    RpAtomic* rpAtomic;
    RwFrame* frame;
    RwMatrix* frameMatrix;
    RwMatrix* cameraMatrix;

    rpAtomic = (RpAtomic*)atomic;
    frame = (RwFrame*)rpAtomic->object.parent;
    if (frame != 0) {
        frame->object.privateFlags |= 0x20;
        frameMatrix = (RwMatrix*)((char*)frame + 0x10);
        cameraMatrix = (RwMatrix*)camera_facing_matrix_ay;
        frameMatrix->right = cameraMatrix->right;
        frameMatrix->up = cameraMatrix->up;
        frameMatrix->at = cameraMatrix->at;
        RwFrameUpdateObjects(frame);
    }
    return atomic;
}

void delete_light_lists(void) {
    last_obj_light_flags = 0xFFFFFFFF;
    uploaded_light_state = 0;
    skip_light_setup = 0;
}

MkxRpLight* find_mkx_rplight_in_obj(MkObj* obj) {
    MkPtr* ptr;
    int matches_type;
    MkxRpLight* link;
    MkxRpLight* candidate;

    if (obj->child_list != 0) {
        ptr = first_mkptr(&obj->child_list);
        while (ptr != 0) {
            link = (MkxRpLight*)ptr->hdr;
            matches_type = 0;
            if (link != 0) {
                if (link->hdr.vtbl->destroy ==
                    (MkVtblFn)vdestroy_mkx_rplight) {
                    matches_type = 1;
                }
            }
            if (matches_type != 0) {
                candidate = link;
            } else {
                candidate = 0;
            }
            if (candidate != 0) {
                return candidate;
            }
            ptr = next_mkptr(ptr);
        }
    }
    return 0;
}

void bind_rplight_to_obj(void* light, void* obj) {
    MkxRpLight* link;
    MkObj* mkobj = (MkObj*)obj;

    link = (MkxRpLight*)get_mkhdr(
        &vtbl_mkx_rplight, sizeof(MkxRpLight));
    if (link != 0) {
        link->light = light;
        link->obj = 0;
        link->obj_instance = 0;
    }
    if (link != 0) {
        mk_insert(&link->hdr, &mkobj->child_list);
        link->obj = mkobj;
        link->obj_instance = mkobj->hdr.instance;
    }
}

void vdestroy_mkx_rplight(MkxRpLight* link) {
    MkObj* obj;
    RwFrame* frame;

    link->hdr.instance = 0;
    if (RpLightGetWorld(link->light) != 0) {
        RpWorldRemoveLight(World, link->light);
    }
    frame = (RwFrame*)((RwObject*)link->light)->parent;
    if (frame != 0) {
        RwFrameDestroy(frame);
    }
    RpLightDestroy(link->light);

    obj = link->obj;
    if (obj != 0) {
        if (obj->hdr.instance == link->obj_instance) {
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0 && obj->hdr.instance != 0) {
        ((void (*)(MkHdr*))obj->hdr.vtbl->destroy)(&obj->hdr);
    }
    link->hdr.instance = 0;
    mkhdr_memfree(&link->hdr);
}

void* get_mkx_rplight(void* light) {
    MkxRpLight* link;

    link = (MkxRpLight*)get_mkhdr(
        &vtbl_mkx_rplight, sizeof(MkxRpLight));
    if (link != 0) {
        link->light = light;
        link->obj = 0;
        link->obj_instance = 0;
    }
    return link;
}

int vdestroy_mkx_mem(void* mem) {
    MkxMem* mkxmem = (MkxMem*)mem;

    _mwMemFree(mkxmem->allocation, 0, 0);
    mkxmem->hdr.instance = 0;
    mkhdr_memfree(&mkxmem->hdr);
}

void* get_mkx_mem(void* allocation) {
    MkxMem* mkxmem;

    mkxmem = (MkxMem*)get_mkhdr(&vtbl_mkx_mem, sizeof(MkxMem));
    if (mkxmem != 0) {
        mkxmem->allocation = allocation;
    }
    return mkxmem;
}

int vdestroy_mkobj(void* obj) {
    MkObj* mkobj = (MkObj*)obj;
    RpClump* clump;
    int i;

    mkobj->hdr.instance = 0;
    if (mkobj->hide_flag_bits.bit3 != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    for (i = 0; i < mkobj->clump_count; i++) {
        clump = mkobj->clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            mkobj->clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->cloth_bones != 0) {
        free_mem(mkobj->cloth_bones);
    }
    if (mkobj->matrix_indices != 0) {
        free_mem(mkobj->matrix_indices);
    }
    mkobj->field_5C = 0;
    _mwMemFree(mkobj, 0, 0);
}

void destroy_mkobj(void* obj) {
    MkObj* mkobj = (MkObj*)obj;
    RpClump* clump;
    int i;

    mkobj->hdr.instance = 0;
    if (mkobj->hide_flag_bits.bit3 != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    for (i = 0; i < mkobj->clump_count; i++) {
        clump = mkobj->clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            mkobj->clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->cloth_bones != 0) {
        free_mem(mkobj->cloth_bones);
    }
    if (mkobj->matrix_indices != 0) {
        free_mem(mkobj->matrix_indices);
    }
    mkobj->field_5C = 0;
    _mwMemFree(mkobj, 0, 0);
}

void destroy_clump(void* clump) {
    pull_clump_from_world((RpClump*)clump);
    RpClumpForAllAtomics(
        (RpClump*)clump, atomic_null_texture_pointers, 0);
    RpClumpDestroy((RpClump*)clump);
}

static RpAtomic* atomic_null_texture_pointers(
    RpAtomic* atomic, void* data) {
    RpGeometryForAllMaterials(
        atomic->geometry, material_null_texture_pointer, 0);
    return atomic;
}

RwTexture* material_get_texture_pointer(RpMaterial* material, int use_matfx) {
    RpMatFXMaterialFlags effect;

    if (use_matfx == 0) {
        return material->texture;
    }

    effect = RpMatFXMaterialGetEffects(material);
    switch (effect) {
    case rpMATFXEFFECTDUAL:
        return ((RpMatFXDualData*)MatFXGetData(
                    material, rpMATFXEFFECTDUAL))
            ->texture;
    case rpMATFXEFFECTDUALUVTRANSFORM:
        return ((RpMatFXDualData*)MatFXGetData(
                    material, rpMATFXEFFECTDUAL))
            ->texture;
    case rpMATFXEFFECTBUMPMAP:
        return ((RpMatFXBumpMapData*)MatFXGetData(
                    material, rpMATFXEFFECTBUMPMAP))
            ->texture;
    case rpMATFXEFFECTNULL:
    default:
        return 0;
    }
}

void material_set_texture_pointer(
    RpMaterial* material, RwTexture* texture, int use_matfx) {
    RpMatFXMaterialFlags effect;

    if (use_matfx == 0) {
        material->texture = texture;
        return;
    }

    effect = RpMatFXMaterialGetEffects(material);
    switch (effect) {
    case rpMATFXEFFECTDUAL:
        ((RpMatFXDualData*)MatFXGetData(
             material, rpMATFXEFFECTDUAL))
            ->texture = texture;
        return;
    case rpMATFXEFFECTDUALUVTRANSFORM:
        ((RpMatFXDualData*)MatFXGetData(
             material, rpMATFXEFFECTDUAL))
            ->texture = texture;
        return;
    case rpMATFXEFFECTBUMPMAP:
        ((RpMatFXBumpMapData*)MatFXGetData(
             material, rpMATFXEFFECTBUMPMAP))
            ->texture = texture;
        return;
    case rpMATFXEFFECTNULL:
    default:
        return;
    }
}

static RpMaterial* material_null_texture_pointer(
    RpMaterial* material, void* data) {
    int effect;
    RpMatFXDualData* dual;
    RpMatFXBumpMapData* bump;

    material->texture = 0;
    effect = RpMatFXMaterialGetEffects(material);
    if (effect != rpMATFXEFFECTNULL) {
        if (effect == rpMATFXEFFECTDUAL) {
            dual = (RpMatFXDualData*)MatFXGetData(
                material, rpMATFXEFFECTDUAL);
            dual->texture = 0;
        } else if (effect == rpMATFXEFFECTBUMPMAP) {
            bump = (RpMatFXBumpMapData*)MatFXGetData(
                material, rpMATFXEFFECTBUMPMAP);
            bump->bumped_texture = 0;
            bump->coefficient = 1.0f;
        } else if (effect != rpMATFXEFFECTUVTRANSFORM &&
                   effect == rpMATFXEFFECTDUALUVTRANSFORM) {
            dual = (RpMatFXDualData*)MatFXGetData(
                material, rpMATFXEFFECTDUAL);
            dual->texture = 0;
        }
    }
    return material;
}

void* start_scale_proc(void* obj, void* script) {
    ScalePdata* pdata;

    _create_mkproc_generic_nostack(
        0x5022, 0x20, p_scale, sizeof(ScalePdata), (MkHdr**)&pdata);
    if (pdata != 0) {
        pdata->obj = (MkObj*)obj;
        pdata->obj_instance = ((MkObj*)obj)->hdr.instance;
        pdata->script = (ScaleScriptEntry*)script;
        pdata->script_start = (ScaleScriptEntry*)script;
        pdata->prior_scale.z = 1.0f;
        pdata->prior_scale.y = 1.0f;
        pdata->prior_scale.x = 1.0f;
        pdata->elapsed = 0.0f;
    }
    return pdata;
}

static inline MkObj* scale_validate_obj(MkObj* obj,
                                        unsigned int expected_instance) {
    if (obj != 0) {
        if (obj->hdr.instance == expected_instance) {
            return obj;
        }
        return 0;
    }
    return 0;
}

static float p_scale(void) {
    ScalePdata* pdata;
    MkObj* obj;
    ScaleScriptEntry* script;
    unsigned int flags;
    float elapsed;
    float t;

    pdata = (ScalePdata*)apdata;
    if (aproc->pid != 0x5022 || pdata == 0) {
        return -1.0f;
    }
    obj = scale_validate_obj(pdata->obj, pdata->obj_instance);
    if (obj == 0) {
        return -1.0f;
    }

    script = pdata->script;
    flags = script->flags;
    if ((flags & 0x10000) != 0) {
        return -1.0f;
    }
    elapsed = pdata->elapsed + game_speed;
    pdata->elapsed = elapsed;
    if (elapsed > script->duration) {
        t = 1.0f;
    } else {
        t = elapsed / script->duration;
        if ((flags & 0x400000) != 0) {
            t = gxMathSin(1.5707964f * t);
        } else if ((flags & 0x800000) != 0) {
            t = 1.0f - gxMathCos(1.5707964f * t);
        }
    }
    if ((flags & 0x40000) != 0) {
        obj->flags_08_bits.scale_active = 1;
        interp_v3(&obj->scale, &script->scale, &pdata->prior_scale, t);
        if ((float)((float)(obj->scale.x == obj->scale.y) ==
                    obj->scale.z) == 1.0f) {
            obj->flags_08_bits.scale_active = 0;
        }
    }
    if (pdata->elapsed >= script->duration) {
        pdata->elapsed -= script->duration;
        pdata->prior_scale = script->scale;
        pdata->script = script + 1;
    }
    return 1.0f;
}

void* limb_sever_find_limbset(void* obj, int id) {
    MkObj* limb_obj;
    MkSobj* sobj;
    void* result;

    result = 0;
    limb_obj = (MkObj*)((MkObj*)obj)->field_5C;
    if (limb_obj != 0) {
        sobj = (MkSobj*)first_mkhdr(&limb_obj->sobj_list);
        if (sobj != 0) {
            result = sobj->matrices;
        }
    }
    return result;
}

void limb_sever_reset_limbs(PlyrInfo* player) {
    FighterMirror* fighter;
    FighterObjectRef* ref;
    MkObj* limb;
    MkSobj* sobj;
    LimbSet* limb_set;
    MkBone* bone;
    MkHdr* hdr;
    MkPtr* walk;
    MkPtr* next;
    void* effect;
    int i;

    limb_set = 0;
    fighter = player->slot.fighter;
    if (player->slot.mirror_a != 0) {
        sobj = (MkSobj*)first_mkhdr(&player->slot.mirror_a->sobj_list);
        if (sobj != 0) {
            limb_set = (LimbSet*)sobj->matrices;
        }
    }

    limb = player->slot.mirror_a;
    for (i = 0; i < (int)limb->bone_count; i++) {
        bone = limb->bones[i];
        bone->parent_matrix = bone->original_parent_matrix;
    }

    for (i = 0; i < 15; i++) {
        ref = &fighter->severed_limbs[i];
        hdr = &ref->object->hdr;
        if (hdr != 0) {
            if (hdr->instance != ref->instance) {
                hdr = 0;
            }
        } else {
            hdr = 0;
        }
        if (hdr != 0 && hdr->instance != 0) {
            hdr->typed_vtbl->destroy(hdr);
        }
        ref->object = 0;
        ref->instance = 0;
        if (limb_set != 0) {
            limb_set->moved_bones &= ~(1U << i);
        }
    }

    hdr = (MkHdr*)fighter->limb_update_proc;
    if (hdr != 0) {
        if (hdr->instance != fighter->limb_update_proc_instance) {
            hdr = 0;
        }
    } else {
        hdr = 0;
    }
    if (hdr != 0 && hdr != (MkHdr*)aproc && hdr->instance != 0) {
        hdr->typed_vtbl->destroy(hdr);
    }
    fighter->limb_update_proc = 0;
    fighter->limb_update_proc_instance = 0;

    if (&fighter->attach_proc_list != 0) {
        walk = fighter->attach_proc_list;
        while (walk != 0) {
            hdr = walk->hdr;
            if (walk->instance != hdr->instance) {
                next = walk->next;
                walk->hdr = 0;
                destroy_mkptr(walk);
                walk = next;
            } else {
                if (hdr != 0 && hdr != (MkHdr*)aproc &&
                    hdr->instance != 0) {
                    hdr->typed_vtbl->destroy(hdr);
                }
                walk = walk->next;
            }
        }
    }
    fighter->attach_proc_list = 0;

    limb_sever_hide_z_meat_chunks_all(player->slot.mirror_a);
    if (player->player_index == 6 &&
        player->flags_14_bits.alternate_costume == 0) {
        effect = find_pfx_by_name_by_bankowner(
            "eyelt\0eyert\0ARMCHAIN", 1U << player->field_04);
        if (effect != 0) {
            reset_effect_ppfx(effect);
        }
        pfx_spawn_at_bid(
            "eyelt\0eyert\0ARMCHAIN", player->slot.mirror_a, 0x10);
        effect = find_pfx_by_name_by_bankowner(
            "eyelt\0eyert\0ARMCHAIN" + 6, 1U << player->field_04);
        if (effect != 0) {
            reset_effect_ppfx(effect);
        }
        pfx_spawn_at_bid(
            "eyelt\0eyert\0ARMCHAIN" + 6,
            player->slot.mirror_a, 0x10);
    }
}

void limb_sever_show_z_meat_chunks_all_plyr_num(int plyr) {
    if (plyr == 0) {
        limb_sever_show_z_meat_chunks_all(g_game_info.plyr0.slot.mirror_a);
    } else if (plyr == 1) {
        limb_sever_show_z_meat_chunks_all(g_game_info.plyr1.slot.mirror_a);
    }
}

static inline unsigned int limb_material_bank_base(MkObj* obj) {
    unsigned int base;

    base = 0;
    if (get_player_number(obj) == 0) {
        if (g_game_info.plyr0.slot.fighter->limb_material_bank != 0) {
            base = 0x400;
        }
    } else if (g_game_info.plyr1.slot.fighter->limb_material_bank != 0) {
        base = 0x400;
    }
    return base;
}

void limb_sever_show_z_meat_chunks_all(MkObj* obj) {
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    unsigned int material_id_base;
    unsigned int material_id;
    unsigned int chunk_offset;
    int chunk_id;

    material_id_base = limb_material_bank_base(obj);

    chunk_offset = 0;
    while (limb_meat_chunk_list[
               chunk_offset / sizeof(*limb_meat_chunk_list)] > -1) {
        chunk_id = limb_meat_chunk_list[
            chunk_offset / sizeof(*limb_meat_chunk_list)];
        material_id = material_id_base + (unsigned int)chunk_id;
        material = 0;
        ptr = first_mkptr(&obj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material = find_geometry_material_by_id(geometry, material_id);
            if (material != 0) {
                break;
            }
            ptr = next_mkptr(ptr);
        }
        if (material != 0) {
            show_material(material);
        }
        chunk_offset += 4;
    }
}

void limb_sever_show_z_meat_chunks(
    MkObj* obj, int limb, int include_children) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    unsigned int material_id_base;
    unsigned int material_id;
    int* material_ids;
    int shown;

    mkobj = obj;
    shown = 0;
    material_id_base = limb_material_bank_base(obj);

    material_ids = limb_meats_mat_id_tbl[limb];
    while (*material_ids != -1) {
        material_id = material_id_base + (unsigned int)*material_ids;
        material = 0;
        ptr = first_mkptr(&mkobj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material = find_geometry_material_by_id(geometry, material_id);
            if (material != 0) {
                break;
            }
            ptr = next_mkptr(ptr);
        }
        if (material != 0) {
            show_material(material);
        }
        material_ids++;
        if (shown != 0 && include_children == 0) {
            break;
        }
        shown++;
    }
}

void limb_sever_hide_z_meat_chunks_all(MkObj* obj) {
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    unsigned int material_id_base;
    unsigned int material_id;
    unsigned int chunk_offset;
    int chunk_id;

    material_id_base = limb_material_bank_base(obj);

    chunk_offset = 0;
    while (limb_meat_chunk_list[
               chunk_offset / sizeof(*limb_meat_chunk_list)] > -1) {
        chunk_id = limb_meat_chunk_list[
            chunk_offset / sizeof(*limb_meat_chunk_list)];
        material_id = material_id_base + (unsigned int)chunk_id;
        material = 0;
        ptr = first_mkptr(&obj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material = find_geometry_material_by_id(geometry, material_id);
            if (material != 0) {
                break;
            }
            ptr = next_mkptr(ptr);
        }
        if (material != 0) {
            hide_material(material);
        }
        chunk_offset += 4;
    }
}

static void _move_bones_from_obj_to_limbobj(
    MkObj* source_arg, MkObj* limb_arg, int bone_index, int include_children);

MkObj* obj_sever_limb(
    MkObj* obj, int limb, Vec* limb_velocities, int include_children) {
    MkObj* source;
    LimbSet* limb_set;
    MkObj* severed;
    int severed_type;
    PlyrInfo* player;
    MkSobj* sobj;
    MkBone* root;
    MkBone* source_bone;
    MkBone* chain_bone;
    MkPtr* sobj_ref;
    ClothBone* source_cloth;
    ClothBone* chain_cloth_bone;
    MkObj* chain;
    RwMatrix* source_matrix;
    Vec* limb_velocity;
    void* effect;
    Vec root_offset;
    RwMatrixPosition correction;
    RwMatrixPosition saved_pos;
    RwMatrixPosition transformed;
    int root_index;
    int saved_fallback;
    int chain_count;
    unsigned int i;

    source = obj;
    player = 0;
    if ((int)source->oid == 0x1001) {
        player = &g_game_info.plyr0;
        severed_type = 0x1005;
    } else if ((int)source->oid == 0x1002) {
        player = &g_game_info.plyr1;
        severed_type = 0x1006;
    } else {
        severed_type = 0x1007;
    }
    sobj = (MkSobj*)first_mkhdr(&source->sobj_list);
    if (sobj == 0) {
        return 0;
    }
    sobj->flags09_bits.bit4 = 1;
    limb_set = (LimbSet*)sobj->matrices;
    if (limb_set == 0) {
        return 0;
    }

    severed = (MkObj*)get_mkobj_frame(severed_type, 0);
    if (severed == 0) {
        return 0;
    }
    severed->parent_hdr = &source->hdr;
    severed->parent_inst = source->hdr.instance;
    severed->flags_0C_bits.parented = 1;
    severed->hide_flag_bits.hidden = 1;
    severed->hide_flags =
        (unsigned char)((severed->hide_flags & ~0x40) |
                        (source->hide_flags & 0x40));

    sobj_ref = get_mkptr_not_owns_mkhdr(&sobj->hdr);
    insert_mkptr(sobj_ref, &severed->sobj_list);
    severed->matrix_indices = (int*)get_mem(0x3C);
    if (severed->matrix_indices == 0) {
        if (severed->hdr.instance != 0) {
            ((void (*)(MkHdr*))severed->hdr.vtbl->destroy)(&severed->hdr);
        }
        return 0;
    }
    severed->flipped_bone_map = source->flipped_bone_map;
    severed->bone_count = source->bone_count;
    severed->bones = (MkBone**)get_mem(severed->bone_count * 4);
    if (severed->bones == 0) {
        if (severed->hdr.instance != 0) {
            ((void (*)(MkHdr*))severed->hdr.vtbl->destroy)(&severed->hdr);
        }
        return 0;
    }
    for (i = 0; i < severed->bone_count; i++) {
        severed->bones[i] = 0;
    }

    severed->field_24 = &((RwMatrix*)limb_set)[limb];
    _move_bones_from_obj_to_limbobj(
        source, severed, limb, include_children);
    severed->pos = source->pos;
    severed->pos_vel_row = source->pos_vel_row;
    severed->ang_row = source->ang_row;
    severed->ang_vel_row = source->ang_vel_row;
    severed->scale_row = source->scale_row;
    memcpy(severed->field_24, source->field_24, sizeof(RwMatrix));

    root_index = limb_root_bids[limb];
    severed->fallback_bone_index = root_index;
    root = severed->bones[root_index];
    if (root != 0 && root->parent_matrix != 0) {
        bone_make_parents_my_children(root);
        v3_x_mat(
            &root_offset, (Vec*)&root->parent_matrix->pos,
            (MKMATRIX*)severed->field_24);
        severed->pos.value.x += root_offset.x;
        severed->pos.value.y += root_offset.y;
        severed->pos.value.z += root_offset.z;
        root->translation.value.z = 0.0f;
        root->translation.value.y = 0.0f;
        root->translation.value.x = 0.0f;
        RtQuatConvertFromMatrix(&root->rotation, root->parent_matrix);
        update_mkobj(severed != 0 ? as_mkhdr(&severed->hdr) : 0);
        saved_fallback = severed->fallback_bone_index;
        severed->fallback_bone_index = root->bone_index;
        update_bone_hierarchy(
            severed != 0 ? as_mkhdr(&severed->hdr) : 0);
        severed->fallback_bone_index = saved_fallback;
    }

    if (limb_velocities != 0) {
        source_matrix = severed->field_24;
        if ((severed->flags_0B & 4) == 0) {
            saved_pos = severed->pos;
        } else {
            gxMat33Tx31(
                &transformed.value, &severed->pivot,
                (Mat33*)source_matrix);
            PSVECSubtract(
                &severed->pos.value, &transformed.value, &saved_pos.value);
        }
        limb_velocity = &limb_velocities[limb];
        severed->pivot.x = limb_velocity->x;
        severed->pivot.y = limb_velocity->y;
        severed->pivot.z = limb_velocity->z;
        severed->flags_0B_bits.pivot_enabled = 1;
        gxMat33Tx31(
            &transformed.value, &severed->pivot,
            (Mat33*)source_matrix);
        PSVECSubtract(
            &severed->pos.value, &transformed.value, &correction.value);
        PSVECSubtract(
            &correction.value, &saved_pos.value, &transformed.value);
        PSVECSubtract(
            &severed->pos.value, &transformed.value, &severed->pos.value);
    }
    mk_insert(&severed->hdr, &fgnd_mkobj_list);
    severed->flags_08_bits.airborne = 1;
    mk_insert(&severed->hdr, &bone_hierarchy_mkobj_list);

    if (player != 0) {
        if (player->player_index == 6) {
            if (limb == 0) {
                effect = find_pfx_by_name_by_bankowner(
                    "eyelt\0eyert\0ARMCHAIN", 1U << player->field_04);
                if (effect != 0) {
                    reset_effect_ppfx(effect);
                }
                effect = find_pfx_by_name_by_bankowner(
                    "eyelt\0eyert\0ARMCHAIN" + 6,
                    1U << player->field_04);
                if (effect != 0) {
                    reset_effect_ppfx(effect);
                }
            }
        } else if (player->player_index == 0x10 &&
                   player->flags_14_bits.alternate_costume == 0 &&
                   (severed->matrix_count > 1 || limb == 5 || limb == 2)) {
            chain_count = 0;
            if (severed->bones[113] != 0) {
                obj_set_bone_collapse_flag(severed, 113);
                chain_count = 1;
            }
            if (severed->bones[96] != 0) {
                obj_set_bone_collapse_flag(severed, 96);
                chain_count++;
            }

            while (chain_count != 0) {
                chain = (MkObj*)load_named_model_for_player(
                    "eyelt\0eyert\0ARMCHAIN" + 12, player->field_04,
                    severed_type, 0);
                if (chain != 0) {
                    int chain_bones[7] = {
                        0x16, 0x2001, 0x2002, 0x2003,
                        0x2004, 0x2005, 0x2006
                    };
                    ClothInitEntry chain_cloth[6] = {
                        {0x0C, 0.25f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0},
                        {0x0D, 0.22f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0},
                        {0x0E, 0.19f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0},
                        {0x0F, 0.16f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0},
                        {0x10, 0.13f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0},
                        {0x11, 0.1f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0}
                    };

                    mk_insert(&chain->hdr, &fgnd_mkobj_list);
                    specskin_initialize_clump(chain->clump);
                    chain->light_flags = source->light_flags;
                    mk_insert(&chain->hdr, &source->child_list);
                    update_mkobj(chain);
                    build_bones_tbl(chain, chain_bones);
                    start_cloth_bones(chain);
                    cloth_bones_init_by_tbl(chain, chain_cloth, 6);

                    if (chain_count <= 1 && severed->bones[22] != 0) {
                        source_matrix = source->field_24;
                        if (source->bones[22] != 0) {
                            source_matrix = &source->bones[22]->matrix;
                        }
                        memcpy(chain->field_24, source_matrix,
                               sizeof(RwMatrix));
                        RwMatrixUpdate(chain->field_24);
                        RwFrameUpdateObjects(chain->frame);
                        ft_fake_bone_matcher(
                            chain, severed, 0x16, 0, 0, 0, 1, 0.0f);
                        if (severed->bones[113] != 0) {
                          for (i = 0; i < 6; i++) {
                            source_bone = source->bones[113 + i];
                            chain_bone = chain->bones[i + 1];
                            memcpy(&chain_bone->matrix, &source_bone->matrix,
                                   sizeof(RwMatrix));
                            memcpy(chain_bone->parent_matrix,
                                   source_bone->original_parent_matrix,
                                   sizeof(RwMatrix));
                            chain_cloth_bone = chain_bone->cloth_link;
                            source_cloth = severed->bones[113 + i]->cloth_link;
                            chain_cloth_bone->world_target.x =
                                source_cloth->world_target.x;
                            chain_cloth_bone->world_target.y =
                                source_cloth->world_target.y;
                            chain_cloth_bone->world_target.z =
                                source_cloth->world_target.z;
                            chain_cloth_bone->force_position.x =
                                source_cloth->force_position.x;
                            chain_cloth_bone->force_position.y =
                                source_cloth->force_position.y;
                            chain_cloth_bone->force_position.z =
                                source_cloth->force_position.z;
                            chain_cloth_bone->velocity.x =
                                source_cloth->velocity.x;
                            chain_cloth_bone->velocity.y =
                                source_cloth->velocity.y;
                            chain_cloth_bone->velocity.z =
                                source_cloth->velocity.z;
                          }
                        }
                    } else if (severed->bones[23] != 0) {
                      source_matrix = source->field_24;
                      if (source->bones[23] != 0) {
                        source_matrix = &source->bones[23]->matrix;
                      }
                      memcpy(chain->field_24, source_matrix, sizeof(RwMatrix));
                      RwMatrixUpdate(chain->field_24);
                      RwFrameUpdateObjects(chain->frame);
                      ft_fake_bone_matcher(chain, severed, 0x17, 0, 0, 0, 1,
                                           0.0f);
                      if (severed->bones[96] != 0) {
                        for (i = 0; i < 6; i++) {
                          source_bone = source->bones[96 + i];
                          chain_bone = chain->bones[i + 1];
                          memcpy(&chain_bone->matrix, &source_bone->matrix,
                                 sizeof(RwMatrix));
                          memcpy(chain_bone->parent_matrix,
                                 source_bone->original_parent_matrix,
                                 sizeof(RwMatrix));
                          chain_cloth_bone = chain_bone->cloth_link;
                          source_cloth = severed->bones[96 + i]->cloth_link;
                          chain_cloth_bone->world_target.x =
                              source_cloth->world_target.x;
                          chain_cloth_bone->world_target.y =
                              source_cloth->world_target.y;
                          chain_cloth_bone->world_target.z =
                              source_cloth->world_target.z;
                          chain_cloth_bone->force_position.x =
                              source_cloth->force_position.x;
                          chain_cloth_bone->force_position.y =
                              source_cloth->force_position.y;
                          chain_cloth_bone->force_position.z =
                              source_cloth->force_position.z;
                          chain_cloth_bone->velocity.x =
                              source_cloth->velocity.x;
                          chain_cloth_bone->velocity.y =
                              source_cloth->velocity.y;
                          chain_cloth_bone->velocity.z =
                              source_cloth->velocity.z;
                        }
                      }
                    }
                }
                chain_count--;
            }
        }
    }
    return severed;
}

static void xfer_bone_tree_from_obj_to_limb_obj(int bone_index, MkBone *parent,
                                                MkObj *source_obj,
                                                MkObj *limb_obj);
static void scan_tree_to_xfer_bone_tree_from_obj_to_limb_obj(MkObj *source_arg,
                                                             MkObj *limb_arg,
                                                             int limb_id);

#pragma inline_depth(2)
static inline void scan_tree_to_xfer_bone_tree_from_obj_to_limb_obj(
    MkObj* source_arg, MkObj* limb_arg, int limb_id) {
    MkObj* source;
    MkBone* queue[150];
    MkBone* bone;
    unsigned int read_index;
    unsigned int count;

    source = source_arg;
    read_index = 0;
    count = 1;
    queue[0] = source->bones[source->fallback_bone_index];
    bone = queue[0]->root_next;
    while (bone != 0) {
        queue[count++] = bone;
        bone = bone->root_next;
    }

    while (read_index < count) {
        bone = queue[read_index++];
        if (bone->parent_matrix != 0) {
            if (bone->limb_id == limb_id) {
                xfer_bone_tree_from_obj_to_limb_obj(
                    bone->bone_index, 0, source_arg, limb_arg);
                break;
            }
            bone = bone->tree_child;
            while (bone != 0) {
                queue[count++] = bone;
                bone = bone->tree_next;
            }
        }
    }
}

static void _move_bones_from_obj_to_limbobj(
    MkObj* source_arg, MkObj* limb_arg, int bone_index, int include_children) {
    MkObj* source;
    MkObj* limb;
    MkSobj* sobj;
    LimbSet* limb_set;
    int* child;

    source = source_arg;
    limb = limb_arg;
    sobj = (MkSobj*)obj_first_sobj(source);
    if (sobj == 0) {
        return;
    }
    limb_set = (LimbSet*)sobj->matrices;
    if (limb_set == 0) {
        return;
    }
    limb->matrix_indices[limb->matrix_count] = bone_index;
    limb->matrix_count++;
    limb_set->moved_bones |= 1U << bone_index;
    scan_tree_to_xfer_bone_tree_from_obj_to_limb_obj(
        source_arg, limb_arg, bone_index);

    if (include_children != 0) {
        child = limb_children_table[bone_index];
        while (*child != -1) {
            _move_bones_from_obj_to_limbobj(
                source_arg, limb_arg, *child, include_children);
            child++;
        }
    }
}
#pragma inline_depth reset

static void xfer_bone_tree_from_obj_to_limb_obj(
    int bone_index, MkBone* parent, MkObj* source_arg, MkObj* limb_arg) {
    MkObj* source;
    MkObj* limb;
    MkBone* source_bone;
    MkBone* clone_parent;
    MkBone* new_bone;
    int destination_index;

    source = source_arg;
    limb = limb_arg;
    while (1) {
        source_bone = source->bones[bone_index];
        if (source_bone != 0 && source_bone->parent_matrix != 0) {
            clone_parent = 0;
            if (!((source_bone->clone_source != 0 &&
                   (destination_index =
                        source_bone->clone_source->bone_index,
                    clone_parent = limb->bones[destination_index]) == 0) ||
                  (destination_index = bone_index,
                   limb->bones[destination_index] == 0))) {
                return;
            }

            new_bone = alloc_bone();
            limb->bones[destination_index] = new_bone;
            if (new_bone == 0) {
                return;
            }
            memcpy(new_bone, source_bone, 0x110);
            new_bone->bone_index = destination_index;
            new_bone->field_60 = 0.0f;
            new_bone->field_64 = 0.0f;
            new_bone->update_tick = (unsigned int)(exec_tick_ctr - 1);
            new_bone->transform_parent = 0;
            new_bone->tree_next = 0;
            new_bone->tree_child = 0;
            new_bone->root_next = 0;
            new_bone->clone_source = 0;
            new_bone->flags_54_bits.pose_matrix_applied = 0;
            new_bone->flags_54_bits.hierarchy_driven = 0;
            new_bone->list_80 = 0;
            if (clone_parent != 0) {
                mkbone_insert_child_of_clone_parent(new_bone, clone_parent);
            }
            new_bone->tag = source_bone->tag;
            new_bone->limb_id = source_bone->limb_id;
            new_bone->parent_matrix = source_bone->parent_matrix;
            if (parent != 0) {
                mkbone_insert_child_of_parent(new_bone, parent);
            } else {
                limb->fallback_bone_index = destination_index;
            }
            mkbone_remove(source_bone);
            if (source_bone->tree_child != 0) {
                xfer_bone_tree_from_obj_to_limb_obj(
                    source_bone->tree_child->bone_index, new_bone,
                    source_arg, limb_arg);
            }
        }
        if (parent == 0) {
            return;
        }
        if (source_bone->tree_next == 0) {
            return;
        }
        bone_index = source_bone->tree_next->bone_index;
    }
}

MkObj* get_mkobj(int type, RpClump* clump) {
    MkObj* obj;

    obj = get_mkobj_frame(type, (RwFrame*)clump->object.parent);
    if (obj != 0) {
        obj->clump_count = 1;
        obj->clump = clump;
    }
    return obj;
}

MkObj* get_mkobj_frame(int type, RwFrame* frame) {
    MkObj* obj;
    RwMatrix* matrix;

    obj = (MkObj*)_mwMemMalloc(
        mkobj_heap, 0x100, 4, 0, 0, 0);
    if (obj != 0) {
        obj->hdr.vtbl = &vtbl_mkobj;
        mk_set_instance(&obj->hdr.instance);
        obj->flags_word_08 = 0;
        obj->flags_word_0C = 0;
        if (frame == 0) {
            frame = RwFrameCreate();
            if (frame == 0) {
                obj->hdr.instance = 0;
                _mwMemFree(obj, 0, 0);
                obj = 0;
            } else {
                obj->hide_flag_bits.bit3 = 1;
                obj->hide_flag_bits.bit4 = 1;
            }
        }
        if (frame != 0) {
            mk_insert(&obj->hdr, &master_clean_up_list);
            obj->child_list = 0;
            obj->sobj_list = 0;
            obj->parent_hdr = 0;
            obj->parent_inst = 0;
            obj->list_44 = 0;
            obj->list_88 = 0;
            obj->oid = (unsigned int)type;
            obj->clump_count = 0;
            obj->frame = frame;
            obj->field_24 = &frame->modelling;
            obj->bones = 0;
            obj->bone_count = 0;
            obj->fallback_bone_index = 0;
            obj->matrix_indices = 0;
            obj->matrix_count = 0;
            obj->flags_08_bits.bit7 = 1;
            obj->flags_08_bits.transform_dirty = 1;
            matrix = obj->field_24;
            obj->pos.value = *(Vec*)&matrix->pos;
            obj->pos_vel.z = 0.0f;
            obj->pos_vel.y = 0.0f;
            obj->pos_vel.x = 0.0f;
            obj->ang.z = 0.0f;
            obj->ang.y = 0.0f;
            obj->ang.x = 0.0f;
            obj->ang_vel.z = 0.0f;
            obj->ang_vel.y = 0.0f;
            obj->ang_vel.x = 0.0f;
            obj->gravity = 0.0f;
            obj->ground_colls = 0;
            obj->ground_colls_y = 0.0f;
            obj->field_5C = 0;
            obj->field_60 = 0;
            obj->cloth_bones = 0;
            obj->cloth_bone_count = 0;
            obj->list_7C = 0;
            obj->list_80 = 0;
            obj->flipped_bone_map = 0;
        }
    }
    return obj;
}

void insert_ground_me_mkobj(void* obj) {
    mk_insert((MkHdr*)obj, &ground_me_mkobj_list);
}

void pull_bone_hierarchy_mkobj(void* obj) {
    mk_pull_discard((MkHdr*)obj, &bone_hierarchy_mkobj_list);
}

void insert_bone_hierarchy_mkobj(MkObj* obj) {
    mk_insert(&obj->hdr, &bone_hierarchy_mkobj_list);
}

void insert_particle_mkobj(void* obj) {
    mk_insert((MkHdr*)obj, &particle_mkobj_list);
}

void remove_fgnd_mkobj(void* obj) {
    mk_pull_discard((MkHdr*)obj, &fgnd_mkobj_list);
}

void insert_fgnd_mkobj(void* obj) {
    mk_insert((MkHdr*)obj, &fgnd_mkobj_list);
}

void render_fgnd_mkobjs(void) {
    apply_to_mklist((MkListApplyFn)render_mkobj, &fgnd_mkobj_list);
}

void obj_set_rw_lights(MkObj* obj) {
    int i;
    unsigned int flags;
    LightMkList* entry;
    int active;
    MkPtr* next;
    MkPtr* ptr;
    MkxRpLight* wrapper;
    RpLight* light;

    if (obj != 0) {
        flags = obj->light_flags;
    } else {
        flags = 0;
    }
    skip_light_setup = 0;
    if (last_obj_light_flags == flags) {
        skip_light_setup = 1;
        return;
    }
    entry = light_mklists;
    for (i = 0; i < 13; i++, entry++) {
        switch (entry->state) {
        case 0:
            if ((flags & entry->mask) == 0) {
                continue;
            }
            active = 1;
            entry->state = 1;
            break;
        case 1:
            if ((flags & entry->mask) != 0) {
                continue;
            }
            active = 0;
            entry->state = 0;
            break;
        default:
            if ((flags & entry->mask) != 0) {
                active = 1;
                entry->state = 1;
            } else {
                active = 0;
                entry->state = 0;
            }
            break;
        }
        if (entry->list != 0) {
            ptr = *entry->list;
            while (ptr != 0) {
                wrapper = (MkxRpLight*)ptr->hdr;
                if (ptr->instance != wrapper->hdr.instance) {
                    next = ptr->next;
                    ptr->hdr = 0;
                    destroy_mkptr(ptr);
                    ptr = next;
                    continue;
                }
                light = wrapper->light;
                if (active != 0) {
                    light->object.object.flags |= 3;
                } else {
                    light->object.object.flags &= ~3u;
                }
                ptr = ptr->next;
            }
        }
    }
    last_obj_light_flags = flags;
    uploaded_light_state = 0;
}

void force_rw_lights(void) {
    int i;

    for (i = 0; i < 13; i++) {
        light_mklists[i].state = 2;
    }
    last_obj_light_flags = 0xFFFFFFFF;
    uploaded_light_state = 0;
}

#pragma dont_inline on
void sobj_set_priority(void* sobj_arg, int priority) {
    MkSobj* sobj = (MkSobj*)sobj_arg;
    RpAtomic* atomic;
    MkSobj* linked;

    if (priority == 0x12) {
        atomic = sobj->atomic;
        *(unsigned int*)((char*)atomic + MksobjLocalOffset) |= 0x80000000u;
        linked = *(MkSobj**)((char*)atomic + MksobjLocalOffset + 8);
        if (linked == 0) {
            return;
        }
        linked->id_flags |= 0x80000000u;
        linked->priority = 0x12;
        return;
    }

    /* Retail: bne to plain store; fallthrough clears plugin bits when was 0x12. */
    if (sobj->priority == 0x12) {
        sobj->priority = priority;
        atomic = sobj->atomic;
        *(unsigned int*)((char*)atomic + MksobjLocalOffset) &= 0x7FFFFFFFu;
        linked = *(MkSobj**)((char*)atomic + MksobjLocalOffset + 8);
        if (linked == 0) {
            return;
        }
        linked->id_flags &= 0x7FFFFFFFu;
        if (linked->priority != 0x12) {
            return;
        }
        linked->priority = 0x10;
        return;
    }

    sobj->priority = priority;
}
#pragma dont_inline reset

void obj_set_sobj_priority(MkObj* obj, int id, int priority) {
    MkObj* mkobj;
    SobjCreateData data;
    int i;

    if (obj == 0) {
        return;
    }
    mkobj = obj;
    data.result = 0;
    data.obj = mkobj;
    data.id = (unsigned int)id;
    data.mask = 0xFFF;
    data.priority = 0x10;
    data.set_priority = 0;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (mkobj->clumps[i] != 0) {
            RpClumpForAllAtomics(
                mkobj->clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    if (data.result != 0) {
        sobj_set_priority(data.result, priority);
    }
}

void sobj_set_alpha(void* sobj, int alpha) {
    set_atomic_material_alpha(((MkSobj*)sobj)->atomic, alpha);
}

void sobj_no_zwrite(MkSobj* sobj) {
    sobj->flags09_bits.bit7 = 1;
}

void sobj_disable_blending(void* sobj) {
    ((MkSobj*)sobj)->render_flags = 0x20001;
}

void sobj_set_transl_flag(MkSobj* sobj) {
    RpAtomic* atomic;
    MkSobj* linked;

    atomic = sobj->atomic;
    MK_ATOMIC_PLUGIN(atomic)->flags |= 0x80000000;
    linked = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (linked != 0) {
        linked->id_flags |= 0x80000000;
        linked->priority = 0x12;
    }
}

void atomic_set_transl_flag(RpAtomic* atomic) {
    MkSobj* sobj;

    MK_ATOMIC_PLUGIN(atomic)->flags |= 0x80000000;
    sobj = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (sobj != 0) {
        sobj->id_flags |= 0x80000000;
        sobj->priority = 0x12;
    }
}

MkSobj* unhide_sobj_by_sobj_id(void* obj, unsigned int id) {
    MkSobj* sobj;

    sobj = obj_find_sobj_by_id((MkObj*)obj, id);
    if (sobj != 0) {
        sobj->atomic->object.flags = 4;
    }
    return sobj;
}

MkSobj* hide_sobj_by_sobj_id(void* obj, unsigned int id) {
    MkSobj* sobj;

    sobj = obj_find_sobj_by_id((MkObj*)obj, id);
    if (sobj != 0) {
        sobj->atomic->object.flags &= ~4u;
    }
    return sobj;
}

void unhide_sobj(void* sobj_arg) {
    MkSobj* sobj = (MkSobj*)sobj_arg;

    sobj->atomic->object.flags = 0x4;
}

void hide_sobj(void* sobj_arg) {
    MkSobj* sobj = (MkSobj*)sobj_arg;

    sobj->atomic->object.flags = (unsigned char)(sobj->atomic->object.flags & ~0x4u);
}

void unhide_obj(void* obj_arg) {
    MkObj* obj = (MkObj*)obj_arg;

    obj->hide_flag_bits.hidden = 0;
}

void hide_obj(void* obj_arg) {
    MkObj* obj = (MkObj*)obj_arg;

    obj->hide_flag_bits.hidden = 1;
}

void mkobj_get_matrix_pos(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* pos = (Vec*)out;

    pos->x = mkobj->field_24->pos.x;
    pos->y = mkobj->field_24->pos.y;
    pos->z = mkobj->field_24->pos.z;
}

void mkobj_get_matrix_right(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* right = (Vec*)out;

    right->x = mkobj->field_24->right.x;
    right->y = mkobj->field_24->right.y;
    right->z = mkobj->field_24->right.z;
}

void mkobj_get_matrix_at(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* at = (Vec*)out;

    at->x = mkobj->field_24->at.x;
    at->y = mkobj->field_24->at.y;
    at->z = mkobj->field_24->at.z;
}

void* mkobj_get_matrix(void* obj) {
    return ((MkObj*)obj)->field_24;
}
