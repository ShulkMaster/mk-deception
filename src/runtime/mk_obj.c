#include "runtime/mk_obj.h"
#include "runtime/light.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/utils.h"
#include "game/game_info.h"
#include "math/gxMat.h"
#include "math/gxMath.h"
#include "math/gxQuat.h"
#include "math/mk_math.h"
#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "rw/rpmatfx_types.h"
#include "rw/rtquat.h"

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

typedef struct LimbController {
    MkHdr hdr;
    char pad08[0x0C];
    signed char effect_bank; /* +0x14 */
    char pad15[0x3F];
    int type;                /* +0x54 */
    LimbRuntime* runtime;    /* +0x58 */
    MkObj* limb_obj;         /* +0x5C */
} LimbController;

typedef struct HeadTrackingPdata {
    MkHdr hdr;
    MkObj* obj;
    void* target;
    float angle_x;
    float angle_y;
    float field_18;
    float angle_z;
} HeadTrackingPdata;

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
    ShadowController* controller;
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
void limb_sever_show_z_meat_chunks_all(void* obj);
void limb_sever_hide_z_meat_chunks_all(void* obj);
int get_player_number(void* obj);

extern int limb_meat_chunk_list[];
extern int* limb_meats_mat_id_tbl[];
extern int* limb_children_table[];

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callback, void* data);
RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callback, void* data);
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack callback, void* data);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data);
void* RwFrameGetLTM(void* frame);
void* memcpy(void* dst, const void* src, unsigned int size);
int RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameCreate(void);
void RwFrameUpdateObjects(RwFrame* frame);
int RpClumpDestroy(RpClump* clump);
void set_atomic_material_alpha(RpAtomic* atomic, int alpha);
RpAtomic* force_atomic_material_alpha(RpAtomic* atomic, void* alpha);
void pull_clump_from_world(RpClump* clump);
void mkobj_destroy_bones(MkObj* obj);
void free_mem(void* ptr);
void* RpLightGetWorld(void* light);
void RpWorldRemoveLight(void* world, void* light);
void RpLightDestroy(void* light);
int stricmp(const char* lhs, const char* rhs);
int vdestroy_mkx_rplight(void* light);
extern MkObj* plyr_obj;
extern MkVtable5 vtbl_mkx_mem;
extern MkVtable5 vtbl_mkx_rplight;
extern MkVtable5 vtbl_mkobj;
extern _mwMemHeap* mkobj_heap;
extern void* World;
static void* pdata_headtracking;
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

GoroArmsFixupEntry goro_arms_fixup_map[12] = {
    {0x0C, 0x43}, {0x0F, 0x44}, {0x12, 0x45}, {0x14, 0x46},
    {0x16, 0x47}, {0x18, 0x48}, {0x0E, 0x50}, {0x11, 0x51},
    {0x13, 0x52}, {0x15, 0x53}, {0x17, 0x54}, {0x19, 0x55}
};

extern int exec_tick_ctr;
extern int mode_of_play;
extern int limb_root_bids[15];

void update_camera_facing_matrix(void);

static float p_obj(void);
static float p_bone_hierarchy(void);
static void limb_bone_calc_world_pos(MkHdr* data);
static void set_bone_world_pos_xz(void* obj, int bone, void* pos);
void set_bone_world_pos(void* obj, int bone, void* pos);

void update_bone_hierarchy(void* obj);
void ground_me(void* obj);
void atomic_set_transl_flag(void* atomic);
void render_mkobj(void* obj);
void get_bone_world_pos(void* obj, int bone, void* out);
void* get_mkobj_frame(int type, void* frame);
MkBone* alloc_bone(void);
void mkbone_remove(MkBone* bone);
void mkbone_insert_child_of_clone_parent(MkBone* bone, MkBone* parent);
void mkbone_insert_child_of_parent(MkBone* bone, MkBone* parent);
void RwMatrixUpdate(RwMatrix* matrix);
void PSVECAdd(const Vec* a, const Vec* b, Vec* dst);
void* find_pfx_by_name_by_bankowner(const char* name, unsigned int owner);
void reset_effect_ppfx(void* effect);
void pfx_spawn_at_bid(const char* name, void* obj, int bone);
void bone_make_parents_my_children(MkBone* bone);

static void rwframe_set_true_clip_flag_on_objects_and_children(
    RwFrame* frame, unsigned char flag);
static RwObject* rwobject_set_true_clip_flag(RwObject* object, unsigned char flag);
static RpAtomic* atomic_create_sobj_callback(
    RpAtomic* atomic, void* data);
static void* AtomicFaceCamera(void* atomic, void* data);
static void* rwframe_find_child_sobj_by_id(
    void* frame, unsigned int id, int depth);

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
        result = (MkSobj*)rwframe_find_child_sobj_by_id(frame, id, depth);
        if (result != 0) {
            return result;
        }
        frame = next;
    }
    return 0;
}

static void* rwframe_find_child_sobj_by_id(void* frame_arg, unsigned int id,
                                           int depth) {
    RwFrame* frame;
    void* link;
    RwObject* object;
    MksobjPluginData* plugin;
    MkSobj* sobj;
    RwFrame* child;
    RwFrame* next;

    frame = (RwFrame*)frame_arg;
    link = frame->object_list_next;
    while (link != &frame->object_list_next) {
        object = (RwObject*)((char*)link - 8);
        link = *(void**)link;
        if (object->type == 1) {
            plugin = (MksobjPluginData*)((char*)object + MksobjLocalOffset);
            sobj = plugin->sobj;
            if (sobj != 0 && (sobj->id_flags & 0xFFF) == id) {
                return sobj;
            }
        }
    }
    depth--;
    if (depth == 0) {
        return 0;
    }
    child = frame->child;
    while (child != 0) {
        next = child->next;
        sobj = (MkSobj*)rwframe_find_child_sobj_by_id(child, id, depth);
        if (sobj != 0) {
            return sobj;
        }
        child = next;
    }
    return 0;
}

int sobj_does_atomic_have_children(void* sobj) {
    return ((MkSobj*)sobj)->frame->child != 0;
}

void set_true_clip_flag_on_sobj_and_children(void* sobj, int flag) {
    MkSobj* mksobj;
    RwFrame* frame;
    RwFrame* next;

    mksobj = (MkSobj*)sobj;
    mksobj->flags09 =
        (unsigned char)((mksobj->flags09 & ~4) | ((flag & 1) << 2));
    frame = mksobj->frame->child;
    while (frame != 0) {
        next = frame->next;
        rwframe_set_true_clip_flag_on_objects_and_children(
            frame, (unsigned char)flag);
        frame = next;
    }
}

static void rwframe_set_true_clip_flag_on_objects_and_children(
    RwFrame* frame, unsigned char flag) {
    RwLLLink* link;
    RwLLLink* next_link;
    RwLLLink* end;
    RwObject* object;
    MksobjPluginData* plugin;
    MkSobj* sobj;
    RwFrame* child;
    RwFrame* next_child;
    RwFrame* grandchild;
    RwFrame* next_grandchild;
    RwFrame* descendant;
    RwFrame* next_descendant;

    end = (RwLLLink*)&frame->object_list_next;
    link = frame->object_list_next;
    while (link != end) {
        object = RW_OBJECT_FROM_FRAME_LINK(link);
        if (object->type == 1) {
            plugin = MK_ATOMIC_PLUGIN((RpAtomic*)object);
            sobj = plugin->sobj;
            if (sobj != 0) {
                sobj->flags09 = (unsigned char)(
                    (sobj->flags09 & ~4) | ((flag << 2) & 4));
            }
        }
        link = link->next;
    }
    child = frame->child;
    while (child != 0) {
        next_child = child->next;
        end = (RwLLLink*)&child->object_list_next;
        link = child->object_list_next;
        while (link != end) {
            next_link = link->next;
            object = RW_OBJECT_FROM_FRAME_LINK(link);
            if (object->type == 1) {
                plugin = MK_ATOMIC_PLUGIN((RpAtomic*)object);
                sobj = plugin->sobj;
                if (sobj != 0) {
                    sobj->flags09 = (unsigned char)(
                        (sobj->flags09 & ~4) | ((flag << 2) & 4));
                }
            }
            link = next_link;
        }
        grandchild = child->child;
        while (grandchild != 0) {
            next_grandchild = grandchild->next;
            end = (RwLLLink*)&grandchild->object_list_next;
            link = grandchild->object_list_next;
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

void obj_set_sobj_alpha(void* obj, int sobj_index, int alpha) {
    MkObj* mkobj = (MkObj*)obj;
    MkPtr* ptr;
    MkSobj* sobj;

    if (mkobj != 0) {
        ptr = first_mkptr(&mkobj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            if ((sobj->id_flags & 0xFFF) == (unsigned int)sobj_index) {
                break;
            }
            ptr = next_mkptr(ptr);
        }
        if (ptr == 0) {
            sobj = 0;
        }
        if (sobj != 0) {
            set_atomic_material_alpha(sobj->atomic, alpha);
        }
    }
}

void obj_turn_gravity_off(void* obj) {
    MkObj* mkobj = (MkObj*)obj;

    if (mkobj != 0) {
        mkobj->flags_08 &= ~1u;
    }
}

void obj_enable_grounding(void* obj) {
    MkObj* mkobj;

    mkobj = (MkObj*)obj;
    mkobj->flags_09 |= 0x80;
    update_bone_hierarchy(obj);
    ground_me(obj);
}

void obj_set_gravity(void* obj, float gravity) {
    MkObj* mkobj = (MkObj*)obj;

    if (mkobj != 0) {
        mkobj->flags_08 |= 1;
        mkobj->gravity = gravity;
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

void obj_get_ang_vel(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;
    Vec* value = (Vec*)out;

    *value = mkobj->ang_vel;
}

void mks_set_flipped_bones(int flag) {
    plyr_obj->flipped_bones = flag;
}

void obj_set_flipped_bones(void* obj, int flag) {
    ((MkObj*)obj)->flipped_bones = flag;
}

void obj_set_light_flag(void* obj, int flag) {
    ((MkObj*)obj)->light_flags = flag;
}

void obj_set_ang_vel(void* obj, void* vel) {
    MkObj* mkobj = (MkObj*)obj;

    mkobj->flags_08 |= 4;
    mkobj->ang_vel = *(Vec*)vel;
}

void obj_get_pos_vel(void* obj, void* out) {
    *(Vec*)out = ((MkObj*)obj)->pos_vel;
}

void obj_set_pos_vel(void* obj, void* vel) {
    MkObj* mkobj = (MkObj*)obj;

    mkobj->flags_08 |= 0x20;
    mkobj->pos_vel = *(Vec*)vel;
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
    out->x = obj->pos_x;
    out->y = obj->pos_y;
    out->z = obj->pos_z;
}

void obj_set_pos(MkObj* obj, const Vec* pos) {
    obj->pos_x = pos->x;
    obj->pos_y = pos->y;
    obj->pos_z = pos->z;
    obj->flags_08 |= 0x40;
}

void obj_get_scale(void* obj, void* out) {
    MkObj* mkobj = (MkObj*)obj;

    if (mkobj != 0) {
        *(Vec*)out = mkobj->scale;
    }
}

void obj_set_scale(void* obj, void* scale) {
    MkObj* mkobj = (MkObj*)obj;

    if (mkobj != 0) {
        mkobj->flags_08 |= 2;
        mkobj->scale = *(Vec*)scale;
    }
}

void obj_set_ground_colls_y(void* obj, float y) {
    ((MkObj*)obj)->ground_colls_y = y;
}

void obj_set_ground_colls(void* obj, void* colls) {
    ((MkObj*)obj)->ground_colls = colls;
}

void sobj_get_ang_vel(void* sobj, void* out) {
    *(Vec*)out = ((MkSobj*)sobj)->ang_vel;
}

void sobj_set_ang_vel(void* sobj, void* vel) {
    MkSobj* mksobj = (MkSobj*)sobj;

    mksobj->flags_08 |= 4;
    mksobj->ang_vel = *(Vec*)vel;
}

void sobj_get_pos_vel(void* sobj, void* out) {
    *(Vec*)out = ((MkSobj*)sobj)->pos_vel;
}

void sobj_set_pos_vel(void* sobj, void* vel) {
    MkSobj* mksobj = (MkSobj*)sobj;

    mksobj->flags_08 |= 0x40;
    mksobj->flags_08 |= 0x20;
    mksobj->pos_vel = *(Vec*)vel;
}

void sobj_get_ang(void* sobj, void* out) {
    *(Vec*)out = ((MkSobj*)sobj)->ang;
}

void sobj_set_ang(void* sobj, void* ang) {
    MkSobj* mksobj = (MkSobj*)sobj;

    mksobj->flags_08 |= 0x10;
    mksobj->ang = *(Vec*)ang;
}

void sobj_get_pos(void* sobj, void* out) {
    *(Vec*)out = ((MkSobj*)sobj)->pos;
}

void sobj_set_pos(void* sobj, void* pos) {
    MkSobj* mksobj = (MkSobj*)sobj;

    mksobj->flags_08 |= 0x80;
    mksobj->pos = *(Vec*)pos;
}

Vec* sobj_get_world_pos(void* sobj) {
    RwMatrix* matrix;

    matrix = (RwMatrix*)RwFrameGetLTM(((MkSobj*)sobj)->frame);
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

void unhide_sobj_and_children(void* sobj) {
    MkSobj* mksobj;

    mksobj = (MkSobj*)sobj;
    mksobj->atomic->object.flags = 4;
    RwFrameForAllChildren(
        mksobj->frame, rwframe_unhide_objects_and_children, 0);
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

void hide_sobj_and_children(void* sobj) {
    MkSobj* mksobj;

    mksobj = (MkSobj*)sobj;
    mksobj->atomic->object.flags &= ~4u;
    RwFrameForAllChildren(
        mksobj->frame, rwframe_hide_objects_and_children, 0);
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

void kill_head_tracking(void* obj) {
    MkPtr* ptr;
    MkPtr* next;
    MkProc* proc;

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

static float p_headtracking_die(void) {
    HeadTrackingPdata* pdata;
    MkBone* bone;

    pdata = (HeadTrackingPdata*)pdata_headtracking;
    if (pdata->obj != 0) {
        bone = pdata->obj->bones[0];
        if (bone->parent_matrix != 0) {
            bone->flags_54 &= ~2;
        }
    }
    return -1.0f;
}

void create_mkproc_headtracking(int pid, void* obj, void* target) {
    HeadTrackingPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_nostack(
        pid, 0x14, p_plyr_head_tracking, sizeof(HeadTrackingPdata),
        (MkHdr**)&pdata);
    if (proc != 0) {
        proc->pre_destroy = trackhead_prewake;
        proc->destroy_cb = trackhead_postsleep;
        pdata->obj = (MkObj*)obj;
        pdata->target = target;
        ((MkObj*)obj)->flags_09 |= 2;
        pdata->angle_x = 0.0f;
        pdata->angle_y = 0.0f;
        pdata->angle_z = 0.0f;
    }
}

static float p_plyr_head_tracking(void) {
    return 1.0f;
}

static void trackhead_postsleep(void) {
    pdata_headtracking = 0;
}

static void trackhead_prewake(void) {
    if (aproc->pid != 0x6005 && aproc->pid != 0x6006) {
        mkproc_die();
    }
    pdata_headtracking = apdata;
    if (pdata_headtracking == 0) {
        mkproc_die();
    }
}

void mks_start_goro_arms_fixup(void* mks) {
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
    if (obj != 0) {
        entry = goro_arms_fixup_map;
        for (i = 0; i < 12; i++, entry++) {
            if (entry->source_bone < (int)obj->bone_count &&
                entry->target_bone < (int)obj->bone_count) {
                source = obj->bones[entry->source_bone];
                if (source != 0) {
                    target = obj->bones[entry->target_bone];
                    if (target != 0 &&
                        target->update_tick != (unsigned int)exec_tick_ctr) {
                        gxQuatInterpQuat(
                            &target->rotation, &target->rotation,
                            &source->rotation, 0.5f);
                        target->flags_55 |= 2;
                        gxQuatCopy(
                            &target->rotation_e0, &target->rotation);
                    }
                }
            }
        }
    }
    return 1.0f;
}

void mirror_guy(void* source_arg, void* mirror_arg, void* pdata_arg) {
    MkObj* source;
    MkObj* mirror;
    PlyrPdata* pdata;
    PlyrMirrorBoneMap* map;
    PlyrMirrorObjLatch* latch;
    MkObj* linked_obj;
    MkBone* bone;
    RwMatrix* matrix;
    Vec angles;
    float player_offset;
    int i;

    source = (MkObj*)source_arg;
    mirror = (MkObj*)mirror_arg;
    pdata = (PlyrPdata*)pdata_arg;
    map = pdata->mirror_bone_map;
    angles = source->ang;

    if (mirror != 0) {
        for (i = 0; i < map->count; i++) {
            bone = mirror->bones[map->entries[i].bone_index];
            if (bone != 0 && bone->parent_matrix != 0) {
                matrix = bone->parent_matrix;
                matrix->right.y = -matrix->right.y;
                matrix->up.y = -matrix->up.y;
                matrix->at.y = -matrix->at.y;
                matrix->pos.y = -matrix->pos.y;
            }
        }
        matrix = mirror->field_24;
        YXZ_angles_to_MKMATRIX(&angles, (MKMATRIX*)matrix);
        if (*(signed char*)((char*)pdata->plyr_info + 0x14) < 0) {
            player_offset =
                *(float*)((char*)pdata->status_flags + 0x40);
        } else {
            player_offset =
                *(float*)((char*)pdata->status_flags + 0x1C);
        }
        matrix->pos.y =
            -(g_game_info.misc->mirror_plane_offset + player_offset +
              (source->pos_y - g_game_info.field_34));
        matrix->pos.y += g_game_info.field_34;
        RwFrameUpdateObjects(mirror->frame);
    }

    latch = &pdata->mirror_slots->weapon[0].mirror;
    linked_obj = latch->obj;
    if (linked_obj == 0 || linked_obj->hdr.instance != latch->instance) {
        linked_obj = 0;
    }
    if (linked_obj != 0 && (linked_obj->hide_flags & 0x20) == 0) {
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
    if (linked_obj == 0 || linked_obj->hdr.instance != latch->instance) {
        linked_obj = 0;
    }
    if (linked_obj != 0 && (linked_obj->hide_flags & 0x20) == 0) {
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
    if (linked_obj == 0 || linked_obj->hdr.instance != latch->instance) {
        linked_obj = 0;
    }
    if (linked_obj != 0 && (linked_obj->hide_flags & 0x20) == 0) {
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

void create_shadow_proc(int pid, void* controller_arg, void* source_arg,
                        void* shadow_arg) {
    ShadowController* controller;
    ShadowPdata* pdata;
    MkProc* proc;

    controller = (ShadowController*)controller_arg;
    proc = _create_mkproc_generic_nostack(
        pid, 0x2A, p_shadow_obj, sizeof(ShadowPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        pdata->source = (MkObj*)source_arg;
        pdata->source_instance = ((MkObj*)source_arg)->hdr.instance;
        pdata->shadow = (MkObj*)shadow_arg;
        pdata->shadow_instance = ((MkObj*)shadow_arg)->hdr.instance;
        pdata->controller = controller;
        controller->proc = proc;
        proc->flags |= MKPROC_FLAG_SKIP_IF_PAUSED;
    } else {
        controller->proc = 0;
    }
}

static void shadow_update_pair(ShadowObjPair* pair) {
    MkObj* source;
    MkObj* shadow;

    source = pair->source;
    if (source == 0 || source->hdr.instance != pair->source_instance) {
        source = 0;
    }
    shadow = pair->shadow;
    if (shadow == 0 || shadow->hdr.instance != pair->shadow_instance) {
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

static float p_shadow_obj(void) {
    ShadowPdata* pdata;
    ShadowController* controller;
    ShadowBoneMap* map;
    ShadowBonePair* pair;
    MkObj* source;
    MkObj* shadow;
    MkBone* source_bone;
    MkBone* shadow_bone;
    int i;

    pdata = (ShadowPdata*)apdata;
    source = pdata->source;
    if (source == 0 || source->hdr.instance != pdata->source_instance) {
        source = 0;
    }
    if (source == 0) {
        return -1.0f;
    }
    shadow = pdata->shadow;
    if (shadow == 0 || shadow->hdr.instance != pdata->shadow_instance) {
        shadow = 0;
    }
    if (shadow == 0) {
        return -1.0f;
    }

    controller = pdata->controller;
    map = controller->bone_map;
    pair = map->pairs;
    for (i = 0; i < map->count; i++, pair++) {
        if (pair->source_bone < (int)source->bone_count &&
            pair->shadow_bone < (int)shadow->bone_count) {
            shadow_bone = shadow->bones[pair->shadow_bone];
            if (shadow_bone != 0) {
                source_bone = source->bones[pair->source_bone];
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

    shadow_update_pair(&controller->extras->first);
    shadow_update_pair(&controller->extras->second);
    shadow_update_pair(&controller->third);
    return 1.0f;
}

void start_bone_hierarchy_proc(void) {
    int flags;
    MkProc* proc;

    flags = 0;
    bone_hierarchy_mkobj_list = 0;
    limb_bone_list = 0;
    ground_me_mkobj_list = 0;
    proc = get_mkproc_nostack(&flags);
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

void auto_calc_limbobj_bone_world_pos(void* obj, int bone) {
    LimbBonePdata* pdata;

    pdata = (LimbBonePdata*)get_mkpdata_generic(
        sizeof(LimbBonePdata));
    if (pdata != 0) {
        pdata->obj = (MkObj*)obj;
        pdata->obj_instance = ((MkObj*)obj)->hdr.instance;
        pdata->bone = bone;
        mk_insert(&pdata->hdr, &limb_bone_list);
    }
}

static void limb_bone_calc_world_pos(MkHdr* data) {
    LimbBonePdata* pdata;
    MkObj* obj;
    MkBone* bone;

    pdata = (LimbBonePdata*)data;
    obj = pdata->obj;
    if (obj == 0 || obj->hdr.instance != pdata->obj_instance) {
        if (pdata->hdr.instance != 0) {
            ((void (*)(MkHdr*))pdata->hdr.vtbl->destroy)(&pdata->hdr);
        }
        return;
    }
    bone = obj->bones[pdata->bone];
    get_bone_world_pos(
        obj, pdata->bone, &bone->matrix.pos);
}

/* Soft ceiling: ground_me ~53.91% - matrix scheduling differs; stop. */
void ground_me(void* obj) {
    MkObj* mkobj;
    int* collision;
    MkBone* bone;
    RwMatrix* parent;
    RwMatrix* matrix;
    RwFrame* frame;
    MkSobj* sobj;
    float min_height;
    float height;
    int bone_index;
    int i;

    mkobj = (MkObj*)obj;
    if ((mkobj->flags_09 & 0x80) == 0 || mkobj->ground_colls == 0) {
        return;
    }
    matrix = mkobj->field_24;
    if ((mkobj->hide_flags & 2) != 0) {
        if (mode_of_play == 7) {
            set_bone_world_pos_xz(
                mkobj, mkobj->ground_bone, &mkobj->ground_restore_pos);
        } else {
            set_bone_world_pos(
                mkobj, mkobj->ground_bone, &mkobj->ground_restore_pos);
        }
    }
    mkobj->flags_0C &= (unsigned char)~0x10;
    min_height = 1000.0f;
    if ((mkobj->flags_0C & 8) != 0) {
        mkobj->flags_0C &= (unsigned char)~8;
        mkobj->hide_flags &= (unsigned char)~2;
    }

    collision = (int*)mkobj->ground_colls;
    while (collision[0] >= 0) {
        bone_index = collision[0];
        if ((mode_of_play != 6 || bone_index < 0x16 ||
             bone_index > 0x2F) &&
            bone_index < (int)mkobj->bone_count) {
            bone = mkobj->bones[bone_index];
            if (bone != 0 && bone->parent_matrix != 0) {
                parent = bone->parent_matrix;
                if ((bone->flags_54 & 0x10) == 0) {
                    bone->matrix.right.x =
                        parent->at.x * matrix->at.x +
                        parent->right.x * matrix->right.x +
                        parent->up.x * matrix->up.x;
                    bone->matrix.right.y =
                        parent->at.x * matrix->at.y +
                        parent->right.x * matrix->right.y +
                        parent->up.x * matrix->up.y;
                    bone->matrix.right.z =
                        parent->at.x * matrix->at.z +
                        parent->right.x * matrix->right.z +
                        parent->up.x * matrix->up.z;
                    bone->matrix.up.x =
                        parent->at.y * matrix->at.x +
                        parent->right.y * matrix->right.x +
                        parent->up.y * matrix->up.x;
                    bone->matrix.up.y =
                        parent->at.y * matrix->at.y +
                        parent->right.y * matrix->right.y +
                        parent->up.y * matrix->up.y;
                    bone->matrix.up.z =
                        parent->at.y * matrix->at.z +
                        parent->right.y * matrix->right.z +
                        parent->up.y * matrix->up.z;
                    bone->matrix.at.x =
                        parent->at.z * matrix->at.x +
                        parent->right.z * matrix->right.x +
                        parent->up.z * matrix->up.x;
                    bone->matrix.at.y =
                        parent->at.z * matrix->at.y +
                        parent->right.z * matrix->right.y +
                        parent->up.z * matrix->up.y;
                    bone->matrix.at.z =
                        parent->at.z * matrix->at.z +
                        parent->right.z * matrix->right.z +
                        parent->up.z * matrix->up.z;
                    bone->matrix.flags =
                        parent->flags & matrix->flags;
                    bone->matrix.pos.x =
                        mkobj->pos_x + parent->pos.z * matrix->at.x +
                        parent->pos.x * matrix->right.x +
                        parent->pos.y * matrix->up.x;
                    bone->matrix.pos.y =
                        mkobj->pos_y + parent->pos.z * matrix->at.y +
                        parent->pos.x * matrix->right.y +
                        parent->pos.y * matrix->up.y;
                    bone->matrix.pos.z =
                        mkobj->pos_z + parent->pos.z * matrix->at.z +
                        parent->pos.x * matrix->right.z +
                        parent->pos.y * matrix->up.z;
                }
                height =
                    bone->matrix.pos.y +
                    (float)collision[3] * bone->matrix.at.y +
                    (float)collision[1] * bone->matrix.right.y +
                    (float)collision[2] * bone->matrix.up.y -
                    (float)collision[4];
                if (height < min_height) {
                    min_height = height;
                }
            }
        }
        collision += 5;
    }

    if (min_height != 1000.0f) {
        if (mkobj->ground_colls_y > min_height ||
            (mkobj->flags_09 & 0x40) != 0) {
            mkobj->pos_y -= min_height - mkobj->ground_colls_y;
            matrix->pos.x = mkobj->pos_x;
            matrix->pos.y = mkobj->pos_y;
            matrix->pos.z = mkobj->pos_z;
            matrix->flags &= ~0x20000;
            if ((mkobj->hide_flags & 0x10) == 0) {
                RwFrameUpdateObjects(mkobj->frame);
                for (i = 1; i < mkobj->clump_count; i++) {
                    frame = (&mkobj->clump)[i]->object.parent;
                    frame->ltm.pos = matrix->pos;
                    RwFrameUpdateObjects(frame);
                }
            }
            if (mkobj->matrix_count > 1) {
                sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
                if (sobj != 0 && sobj->matrices != 0) {
                    for (i = 1; i < (int)mkobj->matrix_count; i++) {
                        sobj->matrices[mkobj->matrix_indices[i]].pos =
                            matrix->pos;
                    }
                }
            }
            if (mkobj->ground_colls_y > min_height &&
                ((mkobj->flags_08 & 0x20) == 0 ||
                 mkobj->pos_vel.y <= 0.0f)) {
                mkobj->pos_vel.y = 0.0f;
                mkobj->flags_08 &= (unsigned char)~1;
            }
        }
        mkobj->flags_0C &= (unsigned char)~0x40;
        if ((mkobj->flags_0C & 0x20) != 0) {
            mkobj->flags_0C &= (unsigned char)~0x20;
            mkobj->flags_09 &= (unsigned char)~0x40;
        }
    }
}

void obj_clear_bone_collapse_flag(void* obj, int bone) {
    MkBone* mkbone;

    mkbone = ((MkObj*)obj)->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_55 &= ~0x80;
    }
}

void obj_set_bone_collapse_flag(void* obj, int bone) {
    MkBone* mkbone;

    mkbone = ((MkObj*)obj)->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_55 =
            (unsigned char)((mkbone->flags_55 & ~0x80) | 0x80);
    }
}

void obj_set_bone_calc_world_mat_flag(void* obj, int bone) {
    MkBone* mkbone;

    mkbone = ((MkObj*)obj)->bones[bone];
    if (mkbone != 0) {
        mkbone->flags_54 =
            (unsigned char)((mkbone->flags_54 & ~0x10) | 0x10);
    }
}

void* force_calc_bone_world_mat(void* obj, int bone) {
    MkObj* mkobj;
    MkBone* mkbone;
    RwMatrix* objectMatrix;

    mkobj = (MkObj*)obj;
    if ((unsigned int)bone >= mkobj->bone_count) {
        return 0;
    }
    mkbone = mkobj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        return 0;
    }
    objectMatrix = mkobj->field_24;
    gxMat33x33(
        (Mat33*)&mkbone->matrix, (Mat33*)mkbone->parent_matrix,
        (Mat33*)objectMatrix);
    gxMatV3MatAddV3(
        (Vec*)&mkbone->matrix.pos, (Vec*)&mkbone->parent_matrix->pos,
        (Mat33*)objectMatrix, (Vec*)&objectMatrix->pos);
    return mkbone;
}

void calc_bone_world_mat(void* obj, int bone) {
    MkObj* mkobj;
    MkBone* mkbone;
    RwMatrix* objectMatrix;

    mkobj = (MkObj*)obj;
    if ((unsigned int)bone >= mkobj->bone_count) {
        return;
    }
    mkbone = mkobj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0 ||
        (mkbone->flags_54 & 0x10) != 0) {
        return;
    }
    objectMatrix = mkobj->field_24;
    gxMat33x33(
        (Mat33*)&mkbone->matrix, (Mat33*)mkbone->parent_matrix,
        (Mat33*)objectMatrix);
    gxMatV3MatAddV3(
        (Vec*)&mkbone->matrix.pos, (Vec*)&mkbone->parent_matrix->pos,
        (Mat33*)objectMatrix, (Vec*)&objectMatrix->pos);
}

void get_bone_offset_world_pos(
    void* obj, int bone, void* offset, void* out) {
    MkObj* mkobj;
    MkBone* mkbone;

    mkobj = (MkObj*)obj;
    if (bone >= (int)mkobj->bone_count) {
        *(Vec*)out = mkobj->pos;
        return;
    }
    if (bone == -1) {
        v3_x_mat_add_v3(
            (Vec*)out, (Vec*)offset, (MKMATRIX*)mkobj->field_24,
            (Vec*)&mkobj->field_24->pos);
        return;
    }
    mkbone = mkobj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        v3_x_mat_add_v3(
            (Vec*)out, (Vec*)offset, (MKMATRIX*)mkobj->field_24,
            (Vec*)&mkobj->field_24->pos);
        return;
    }
    calc_bone_world_mat(obj, bone);
    v3_x_mat_add_v3(
        (Vec*)out, (Vec*)offset, (MKMATRIX*)&mkbone->matrix,
        (Vec*)&mkbone->matrix.pos);
}

static void set_bone_world_pos_xz(
    void* obj, int bone, void* pos) {
    MkObj* mkobj;
    Vec current;
    Vec* target;

    mkobj = (MkObj*)obj;
    target = (Vec*)pos;
    get_bone_world_pos(obj, bone, &current);
    mkobj->pos.x += target->x - current.x;
    mkobj->pos.z += target->z - current.z;
    mkobj->flags_08 =
        (unsigned char)((mkobj->flags_08 & ~0x80) | 0x80);
    update_obj_pos(obj);
}

void set_bone_world_pos(void* obj, int bone, void* pos) {
    MkObj* mkobj;
    Vec current;
    Vec* target;

    mkobj = (MkObj*)obj;
    target = (Vec*)pos;
    get_bone_world_pos(obj, bone, &current);
    mkobj->pos.x += target->x - current.x;
    mkobj->pos.y += target->y - current.y;
    mkobj->pos.z += target->z - current.z;
    mkobj->flags_08 =
        (unsigned char)((mkobj->flags_08 & ~0x80) | 0x80);
    update_obj_pos(obj);
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

void get_bone_relative_pos(void* obj, int bone, void* out) {
    MkObj* mkobj;
    MkBone* mkbone;

    mkobj = (MkObj*)obj;
    if (bone >= (int)mkobj->bone_count || bone == -1) {
        *(Vec*)out = mkobj->pos;
        return;
    }
    mkbone = mkobj->bones[bone];
    if (mkbone != 0 && mkbone->parent_matrix != 0) {
        *(Vec*)out = *(Vec*)&mkbone->parent_matrix->pos;
    } else {
        *(Vec*)out = mkobj->pos;
    }
}

void get_bone_world_pos(void* obj, int bone, void* out) {
    MkObj* mkobj;
    MkBone* mkbone;

    mkobj = (MkObj*)obj;
    if (bone >= (int)mkobj->bone_count || bone == -1) {
        *(Vec*)out = mkobj->pos;
        return;
    }
    mkbone = mkobj->bones[bone];
    if (mkbone == 0 || mkbone->parent_matrix == 0) {
        *(Vec*)out = mkobj->pos;
        return;
    }
    v3_x_mat_add_v3(
        (Vec*)out, (Vec*)&mkbone->parent_matrix->pos,
        (MKMATRIX*)mkobj->field_24, &mkobj->pos);
}

void update_bone_hierarchy(void* obj) {
    MkObj* mkobj;
    MkBone* queue[152];
    MkBone* root;
    MkBone* bone;
    MkBone* walk;
    MkBone* parent;
    RwMatrix* matrix;
    Vec saved_pos;
    Vec impulse;
    unsigned int read_index;
    unsigned int count;
    int collapsed;

    mkobj = (MkObj*)obj;
    saved_pos.x = 0.0f;
    saved_pos.y = 0.0f;
    saved_pos.z = 0.0f;
    root = mkobj->bones[mkobj->fallback_bone_index];

    if ((mkobj->hide_flags & 1) != 0) {
        mkobj->hide_flags &= (unsigned char)~1;
        mkobj->flags_0B |= 0x80;
        mkobj->bone_angle_64 = quat_extract_ang_y(&root->rotation);
        mkobj->bone_angle_68 = mkobj->bone_angle_64;
    }

    read_index = 0;
    count = 1;
    queue[0] = root;
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

        if (bone->clone_source != 0 &&
            bone->clone_source->parent_matrix != 0) {
            bone->delta = bone->clone_source->delta;
            bone->velocity = bone->clone_source->velocity;
            gxQuatCopy(&bone->rotation_90, &bone->clone_source->rotation_90);
            memcpy(&bone->matrix, &bone->clone_source->matrix,
                   sizeof(RwMatrix));
            if (bone->parent_matrix != bone->clone_source->parent_matrix) {
                memcpy(bone->parent_matrix,
                       bone->clone_source->parent_matrix, sizeof(RwMatrix));
            }
            continue;
        }

        if ((bone->flags_54 & 8) != 0) {
            saved_pos.x = bone->matrix.pos.x;
            saved_pos.y = bone->matrix.pos.y;
            saved_pos.z = bone->matrix.pos.z;
        }

        parent = bone->transform_parent;
        if (parent != 0 && parent->parent_matrix == 0) {
            parent = 0;
        }
        if (parent == 0) {
            bone->flags_55 &= (unsigned char)~0x40;
        } else {
            bone->flags_55 =
                (unsigned char)((bone->flags_55 & ~0x40) |
                                ((parent->flags_55 & 0x80) >> 1));
        }
        collapsed = (bone->flags_55 & 0x40) != 0;

        if (collapsed || ((bone->flags_54 & 4) == 0 &&
                          (bone->flags_54 & 2) == 0)) {
            matrix = bone->parent_matrix;
            if (parent == 0) {
                gxQuatCopy(&bone->rotation_90, &bone->rotation);
                matrix->pos.x = bone->translation.x;
                matrix->pos.y = bone->translation.y;
                matrix->pos.z = bone->translation.z;
            } else if (!collapsed) {
                gxQuatMul(&bone->rotation_90, &parent->rotation_90,
                          &bone->rotation);
                gxMatV3MatAddV3(
                    (Vec*)&matrix->pos, &bone->translation,
                    (Mat33*)parent->parent_matrix,
                    (Vec*)&parent->parent_matrix->pos);
            } else {
                gxQuatSetZero(&bone->rotation_90);
                bone->rotation_90.w = 1.0f;
                matrix->pos.x = parent->parent_matrix->pos.x;
                matrix->pos.y = parent->parent_matrix->pos.y;
                matrix->pos.z = parent->parent_matrix->pos.z;
            }

            if (!collapsed) {
                if (bone->rotation_90.x == bone->rotation_90.y &&
                    bone->rotation_90.y == bone->rotation_90.z &&
                    bone->rotation_90.z == bone->rotation_90.w) {
                    bone->rotation_90.w = 1.0f;
                }
                gxQuatQuatToMat(RW_MATRIX_MAT33(matrix), &bone->rotation_90);
                if ((bone->flags_55 & 0x20) != 0) {
                    mat_scaled_by_v3(
                        (MKMATRIX*)matrix, (MKMATRIX*)matrix, &bone->scale);
                }
            } else {
                matrix->right.x = 0.0f;
                matrix->right.y = 0.0f;
                matrix->right.z = 0.0f;
                matrix->up.x = 0.0f;
                matrix->up.y = 0.0f;
                matrix->up.z = 0.0f;
                matrix->at.x = 0.0f;
                matrix->at.y = 0.0f;
                matrix->at.z = 0.0f;
                matrix->flags = 0;
            }
        }

        if ((bone->flags_54 & 0x10) != 0) {
            gxMat33x33(
                (Mat33*)&bone->matrix, (Mat33*)bone->parent_matrix,
                (Mat33*)mkobj->field_24);
            gxMatV3MatAddV3(
                (Vec*)&bone->matrix.pos, (Vec*)&bone->parent_matrix->pos,
                (Mat33*)mkobj->field_24, (Vec*)&mkobj->field_24->pos);
        }
        if ((bone->flags_54 & 8) != 0) {
            PSVECSubtract((Vec*)&bone->matrix.pos, &saved_pos, &bone->delta);
            PSVECScale(&bone->velocity, &bone->velocity, 0.8f);
            PSVECScale(&bone->delta, &impulse, 0.2f);
            PSVECAdd(&bone->velocity, &impulse, &bone->velocity);
        }
    }
}

void mkobj_bones_dest_mat_no_update(void* obj) {
    MkObj* mkobj;
    MkBone* bone;
    unsigned int i;

    mkobj = (MkObj*)obj;
    for (i = 0; i < mkobj->bone_count; i++) {
        bone = mkobj->bones[i];
        if (bone != 0) {
            bone->flags_54 =
                (unsigned char)((bone->flags_54 & ~2) | 2);
        }
    }
}

void mkobj_zero_bone_rots(void* obj) {
    MkObj* mkobj;
    MkBone* bone;
    RwMatrix* matrix;
    unsigned int i;

    mkobj = (MkObj*)obj;
    for (i = 0; i < mkobj->bone_count; i++) {
        bone = mkobj->bones[i];
        if (bone != 0) {
            gxQuatSetZero(&bone->rotation);
            gxQuatSetZero(&bone->rotation_e0);
            gxQuatSetZero(&bone->rotation_90);
            matrix = bone->parent_matrix;
            if (matrix != 0) {
                matrix->right.x = 1.0f;
                matrix->right.y = 0.0f;
                matrix->right.z = 0.0f;
                matrix->up.x = 0.0f;
                matrix->up.y = 1.0f;
                matrix->up.z = 0.0f;
                matrix->at.x = 0.0f;
                matrix->at.y = 0.0f;
                matrix->at.z = 1.0f;
                matrix->pos.x = 0.0f;
                matrix->pos.y = 0.0f;
                matrix->pos.z = 0.0f;
                matrix->flags |= 0x20003;
            }
            bone->flags_54 &= ~2;
        }
    }
    update_mkobj(obj);
}

MkProc* fade_material(float delta, void* obj, unsigned int sobj_id,
                      unsigned int material_id, int frames) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    FadeMaterialPdata* pdata;
    MkProc* proc;
    unsigned int offset;
    int i;

    mkobj = (MkObj*)obj;
    material = 0;
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFF) == sobj_id) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (ptr == 0) {
        sobj = 0;
    }
    if (sobj == 0) {
        return 0;
    }

    geometry = sobj->atomic->geometry;
    offset = 0;
    for (i = 0; i < geometry->matList.numMaterials; i++) {
        material =
            *(RpMaterial**)((char*)geometry->matList.materials + offset);
        mkmat = (MkmaterialPluginData*)((char*)material +
                                        MkmaterialLocalOffset);
        if ((mkmat->flags & 0xFFF) == material_id) {
            break;
        }
        offset += 4;
    }
    if (i == geometry->matList.numMaterials) {
        material = 0;
    }
    if (material == 0) {
        return 0;
    }

    proc = _create_mkproc_generic_nostack(
        0x5010, 0x20, p_fade_material, sizeof(FadeMaterialPdata),
        (MkHdr**)&pdata);
    if (pdata != 0) {
        pdata->obj = mkobj;
        pdata->obj_instance = mkobj->hdr.instance;
        pdata->delta = delta;
        pdata->frames = frames;
        pdata->material = material;
        pdata->accumulator = 0.0f;
    }
    return proc;
}

static float p_fade_material(void) {
    FadeMaterialPdata* pdata;
    MkObj* obj;
    unsigned char* color;
    float accumulated;
    int amount;
    int value;
    int i;

    pdata = (FadeMaterialPdata*)apdata;
    if (aproc->pid != 0x5010 || pdata == 0) {
        mkproc_die();
    }
    obj = pdata->obj;
    if (obj == 0 || obj->hdr.instance != pdata->obj_instance) {
        obj = 0;
    }
    if (obj == 0) {
        mkproc_die();
    }

    accumulated = pdata->accumulator + pdata->delta;
    amount = (int)accumulated;
    pdata->accumulator = accumulated - (float)amount;
    color = (unsigned char*)pdata->material + 4;
    for (i = 0; i < 4; i++) {
        value = color[i] + amount;
        if (value < 0) {
            value = 0;
        }
        if (value > 255) {
            value = 255;
        }
        color[i] = (unsigned char)value;
    }
    if (color[0] == 0 && color[1] == 0 && color[2] == 0) {
        obj->hide_flags |= 0x20;
        mkproc_die();
    }
    pdata->frames--;
    if (pdata->frames <= 0) {
        mkproc_die();
    }
    return 1.0f;
}

void obj_for_all_atomics_set_material_alpha(void* obj, int alpha) {
    MkObj* mkobj;
    RpClump** clumps;
    int i;

    mkobj = (MkObj*)obj;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], force_atomic_material_alpha, (void*)alpha);
        }
    }
}

void obj_set_material_fade(void* obj, unsigned int id, int alpha) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    unsigned int packedAlpha;
    int i;

    mkobj = (MkObj*)obj;
    material = 0;
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        geometry = sobj->atomic->geometry;
        for (i = 0; i < geometry->matList.numMaterials; i++) {
            material = geometry->matList.materials[i];
            mkmat = (MkmaterialPluginData*)((char*)material +
                                            MkmaterialLocalOffset);
            if ((mkmat->flags & 0xFFF) == id) {
                break;
            }
            material = 0;
        }
        if (material != 0) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (material != 0) {
        packedAlpha = (unsigned int)(alpha & 0xFF);
        packedAlpha |= packedAlpha << 8;
        packedAlpha |= packedAlpha << 16;
        material->color.packed = packedAlpha;
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
    RpClump** clumps;
    SobjCreateData data;
    MkPtr* ptr;
    MkSobj* sobj;
    int i;

    mkobj = (MkObj*)obj;
    if (mkobj->sobj_list == 0) {
        data.obj = mkobj;
        data.id = 0;
        data.mask = 0;
        data.result = 0;
        data.set_priority = 0;
        data.priority = 0x10;
        clumps = &mkobj->clump;
        for (i = 0; i < mkobj->clump_count; i++) {
            if (clumps[i] != 0) {
                RpClumpForAllAtomics(
                    clumps[i], atomic_create_sobj_callback, &data);
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

void* obj_first_sobj(void* obj) {
    return first_mkhdr(&((MkObj*)obj)->sobj_list);
}

#pragma dont_inline on
void update_mksobj(void* sobj) {
    MkSobj* mksobj;
    RwMatrix* matrix;
    int changed;

    mksobj = (MkSobj*)sobj;
    matrix = (RwMatrix*)((char*)mksobj->frame + 0x10);
    changed = 0;

    if ((mksobj->flags_08 & 0x20) != 0) {
        mksobj->pos.x += mksobj->pos_vel.x * obj_game_speed;
        mksobj->pos.y += mksobj->pos_vel.y * obj_game_speed;
        mksobj->pos.z += mksobj->pos_vel.z * obj_game_speed;
    }
    if ((mksobj->flags_08 & 0xE0) != 0) {
        changed = 1;
        mksobj->flags_08 &= ~0x80;
        matrix->pos = *(RwV3d*)&mksobj->pos;
    }

    if ((mksobj->flags_08 & 4) != 0) {
        mksobj->ang.x += mksobj->ang_vel.x * obj_game_speed;
        mksobj->ang.y += mksobj->ang_vel.y * obj_game_speed;
        mksobj->ang.z += mksobj->ang_vel.z * obj_game_speed;
    }
    if ((mksobj->flags_08 & 0x1E) != 0) {
        mksobj->flags_08 &= ~0x10;
        mksobj->ang.x = normalize_obj_angle(mksobj->ang.x);
        mksobj->ang.y = normalize_obj_angle(mksobj->ang.y);
        mksobj->ang.z = normalize_obj_angle(mksobj->ang.z);
        YXZ_angles_to_MKMATRIX(
            &mksobj->ang, (MKMATRIX*)matrix);
        changed = 1;
    }
    if ((mksobj->flags09 & 0x20) != 0 &&
        mksobj->atomic != 0) {
        AtomicFaceCamera(mksobj->atomic, 0);
    }
    if ((mksobj->flags_08 & 2) != 0) {
        mat_scaled_by_v3(
            (MKMATRIX*)matrix, (MKMATRIX*)matrix, &mksobj->scale);
        changed = 1;
    }
    if (changed != 0) {
        matrix->flags &= ~0x20000;
        RwFrameUpdateObjects(mksobj->frame);
    }
}
#pragma dont_inline reset

static void update_mkhdr_sobj(MkHdr* hdr) {
    update_mksobj(hdr);
}

int vdestroy_mksobj(void* sobj) {
    MkSobj* mksobj;
    MkHdr* bound;

    mksobj = (MkSobj*)sobj;
    mksobj->hdr.instance = 0;
    bound = mksobj->bound_hdr;
    if (bound != 0 &&
        bound->instance == mksobj->bound_instance) {
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

void obj_set_sobj_pos(void* obj, int id, void* pos) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;

    mkobj = (MkObj*)obj;
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFF) == (unsigned int)id) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (ptr == 0) {
        sobj = 0;
    }
    if (sobj != 0) {
        sobj->pos = *(Vec*)pos;
        sobj->flags_08 =
            (unsigned char)((sobj->flags_08 & ~0x80) | 0x80);
    }
}

void* obj_find_sobj_by_id(void* obj, unsigned int id) {
    MkObj* mkobj = (MkObj*)obj;
    MkPtr* ptr;
    MkSobj* sobj;

    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFFu) == id) {
            return sobj;
        }
        ptr = next_mkptr(ptr);
    }
    return 0;
}

void obj_force_culling_off(void* obj) {
    MkObj* mkobj;
    RpClump** clumps;
    SobjCreateData data;
    MkPtr* ptr;
    MkSobj* sobj;
    int i;

    mkobj = (MkObj*)obj;
    data.obj = mkobj;
    data.id = 0;
    data.mask = 0;
    data.result = 0;
    data.set_priority = 0;
    data.priority = 0x10;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if (sobj != 0) {
            sobj->flags09 =
                (unsigned char)((sobj->flags09 & ~0x10) | 0x10);
        }
        ptr = next_mkptr(ptr);
    }
}

void obj_apply_to_sobj_with_id(void* obj, unsigned int id, void* fn, void* data) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    void (*callback)(MkSobj*);

    mkobj = (MkObj*)obj;
    callback = (void (*)(MkSobj*))fn;
    ptr = first_mkptr(&mkobj->sobj_list);
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

void obj_set_all_sobjs_priority(void* obj, int priority) {
    MkObj* mkobj;
    RpClump** clumps;
    struct SobjCreateData data;
    int i;

    mkobj = (MkObj*)obj;
    data.obj = mkobj;
    data.id = 0;
    data.mask = 0;
    data.result = 0;
    data.set_priority = 1;
    data.priority = priority;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], atomic_create_sobj_callback, &data);
        }
    }
}

void* obj_create_sobjs_by_id(void* obj, int id) {
    MkObj* mkobj;
    RpClump** clumps;
    struct SobjCreateData data;
    int i;

    mkobj = (MkObj*)obj;
    data.obj = mkobj;
    data.id = (unsigned int)id;
    data.mask = 0xFFF;
    data.result = 0;
    data.set_priority = 0;
    data.priority = 0x10;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    return data.result;
}

void obj_create_sobjs(void* obj) {
    MkObj* mkobj;
    RpClump** clumps;
    struct SobjCreateData data;
    int i;

    mkobj = (MkObj*)obj;
    data.obj = mkobj;
    data.id = 0;
    data.mask = 0;
    data.result = 0;
    data.set_priority = 0;
    data.priority = 0x10;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], atomic_create_sobj_callback, &data);
        }
    }
}

static RpAtomic* atomic_create_sobj_callback(
    RpAtomic* atomic, void* dataArg) {
    struct SobjCreateData* data;
    MksobjPluginData* plugin;
    MkSobj* sobj;
    unsigned int flags;
    RwMatrix* frameMatrix;

    data = (struct SobjCreateData*)dataArg;
    plugin = (MksobjPluginData*)((char*)atomic + MksobjLocalOffset);
    flags = plugin->flags;
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
    sobj = plugin->sobj;
    if (sobj == 0) {
        sobj = (MkSobj*)_mwMemMalloc(
            mksobj_heap, 0x90, 0x80, 0, 0, 0);
        if (sobj != 0) {
            sobj->hdr.vtbl = (MkVtable5*)&vtbl_mksobj;
            mk_set_instance(&sobj->hdr.instance);
            sobj->bound_hdr = 0;
            sobj->bound_instance = 0;
            sobj->id_flags = flags;
            sobj->priority =
                (flags & 0x80000000) == 0 ? 0x10 : 0x12;
            sobj->frame = (RwFrame*)atomic->object.parent;
            sobj->flags_08 = 0x81;
            sobj->render_flags = 0;
            frameMatrix = (RwMatrix*)((char*)sobj->frame + 0x10);
            sobj->pos = *(Vec*)&frameMatrix->pos;
            sobj->pos_vel.x = 0.0f;
            sobj->pos_vel.y = 0.0f;
            sobj->pos_vel.z = 0.0f;
            sobj->ang.x = 0.0f;
            sobj->ang.y = 0.0f;
            sobj->ang.z = 0.0f;
            sobj->ang_vel.x = 0.0f;
            sobj->ang_vel.y = 0.0f;
            sobj->ang_vel.z = 0.0f;
            sobj->z_offset = 0.0f;
            sobj->matrices = 0;
        }
        sobj->atomic = atomic;
        mk_insert(&sobj->hdr, &data->obj->sobj_list);
        sobj->owner = data->obj;
        plugin->sobj = sobj;
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

void* obj_find_material_by_id(void* obj, int id) {
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    int offset;
    unsigned int count;

    material = 0;
    ptr = first_mkptr(&((MkObj*)obj)->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        geometry = sobj->atomic->geometry;
        offset = 0;
        count = geometry->matList.numMaterials;
        while (count != 0) {
            material =
                *(RpMaterial**)((char*)geometry->matList.materials + offset);
            mkmat = (MkmaterialPluginData*)((char*)material +
                                            MkmaterialLocalOffset);
            if ((mkmat->flags & 0xFFF) == (unsigned int)id) {
                break;
            }
            offset += 4;
            count--;
        }
        if (count == 0) {
            material = 0;
        }
        if (material != 0) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    return material;
}

void* sobj_find_material_by_id(void* sobj, int id) {
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    int offset;
    unsigned int count;

    geometry = ((MkSobj*)sobj)->atomic->geometry;
    offset = 0;
    count = geometry->matList.numMaterials;
    while (count != 0) {
        material = *(RpMaterial**)((char*)geometry->matList.materials + offset);
        mkmat = (MkmaterialPluginData*)((char*)material + MkmaterialLocalOffset);
        if ((mkmat->flags & 0xFFF) == (unsigned int)id) {
            return material;
        }
        offset += 4;
        count--;
    }
    return 0;
}

void* sobj_find_material_with_texture(void* sobj, void* tex) {
    MaterialTextureFind find;
    MkSobj* mksobj = (MkSobj*)sobj;

    find.name = (const char*)tex;
    find.material = 0;
    find.texture = 0;
    if (mksobj->atomic != 0) {
        RpGeometryForAllMaterials(
            mksobj->atomic->geometry, material_find_texture_callback, &find);
    }
    return find.material;
}

void* obj_find_material_with_texture(void* obj, void* tex) {
    MaterialTextureFind find;
    MkObj* mkobj = (MkObj*)obj;

    find.name = (const char*)tex;
    find.material = 0;
    find.texture = 0;
    if (mkobj->clump != 0) {
        RpClumpForAllAtomics(
            mkobj->clump, atomic_find_texture_callback, &find);
    }
    return find.material;
}

static RpAtomic* atomic_find_texture_callback(RpAtomic* atomic, void* data) {
    MaterialTextureFind* find = (MaterialTextureFind*)data;

    RpGeometryForAllMaterials(
        atomic->geometry, material_find_texture_callback, find);
    if (find->texture != 0) {
        return 0;
    }
    return atomic;
}

static RpMaterial* material_find_texture_callback(
    RpMaterial* material, void* data) {
    MaterialTextureFind* find = (MaterialTextureFind*)data;

    if (material->texture != 0 &&
        stricmp(material->texture->name, find->name) == 0) {
        find->material = material;
        find->texture = material->texture;
        return 0;
    }
    return material;
}

void* obj_get_1st_atomic(void* obj) {
    RpClumpForAllAtomics(
        ((MkObj*)obj)->clump, obj_get_1st_atomic_callback, 0);
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
    RpClump** clumps;
    RpClump* clump;
    int i;

    if ((mkobj->oid & oid_to_kill_mask) != oid_to_kill) {
        return;
    }

    mkobj->hdr.instance = 0;
    if ((mkobj->hide_flags & 8) != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        clump = clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->allocation_74 != 0) {
        free_mem(mkobj->allocation_74);
    }
    if (mkobj->matrix_indices != 0) {
        free_mem(mkobj->matrix_indices);
    }
    mkobj->field_5C = 0;
    _mwMemFree(mkobj, 0, 0);
}

void start_obj_proc(void) {
    int flags;
    MkProc* proc;

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
    flags = 0;
    proc = get_mkproc_nostack(&flags);
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

void obj_match_pos_ang_to_src_obj(void* dst, void* src) {
    MkObj* dst_obj = (MkObj*)dst;
    MkObj* src_obj = (MkObj*)src;
    MkHdr* hdr;

    dst_obj->pos = src_obj->pos;
    dst_obj->pos_vel = src_obj->pos_vel;
    dst_obj->ang = src_obj->ang;
    dst_obj->ang_vel = src_obj->ang_vel;
    dst_obj->scale = src_obj->scale;
    dst_obj->flags_word_08 = src_obj->flags_word_08;
    dst_obj->flags_08 &= ~0x20;
    dst_obj->flags_08 &= ~4;
    dst_obj->flags_08 |= 0x80;
    dst_obj->flags_08 |= 0x10;
    if (dst_obj == 0) {
        hdr = 0;
    } else {
        hdr = as_mkhdr(&dst_obj->hdr);
    }
    update_mkobj(hdr);
    dst_obj->flags_word_08 = src_obj->flags_word_08;
    dst_obj->light_flags = src_obj->light_flags;
}

void update_obj_pos(void* obj) {
    MkObj* mkobj = (MkObj*)obj;
    RwMatrix* matrix;
    RwFrame* frame;
    RwMatrix* frame_matrix;
    MkSobj* sobj;
    int i;
    int matrix_index;

    matrix = mkobj->field_24;
    matrix->pos = *(RwV3d*)&mkobj->pos;
    matrix->flags &= ~0x20000;
    if ((mkobj->hide_flags & 0x10) == 0) {
        RwFrameUpdateObjects(mkobj->frame);
        for (i = 1; i < mkobj->clump_count; i++) {
            frame = (RwFrame*)((&mkobj->clump)[i]->object.parent);
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

void update_mkobj(void* obj) {
    MkObj* mkobj;
    RwMatrix* matrix;
    RwFrame* frame;
    RwMatrix* frameMatrix;
    MkSobj* sobj;
    Vec scaled;
    Vec pivot;
    int changed;
    int i;
    int matrixIndex;

    mkobj = (MkObj*)obj;
    apply_to_mklist(update_mkhdr_sobj, &mkobj->sobj_list);
    obj_game_speed = game_speed;
    if ((mkobj->flags_0B & 0x20) != 0) {
        obj_game_speed = 1.0f;
    }
    matrix = mkobj->field_24;
    changed = 0;

    if ((mkobj->flags_08 & 4) != 0) {
        PSVECScale(&mkobj->ang_vel, &scaled, obj_game_speed);
        mkobj->ang.x += scaled.x;
        mkobj->ang.y += scaled.y;
        mkobj->ang.z += scaled.z;
    }
    if ((mkobj->flags_08 & 0x1E) != 0) {
        mkobj->flags_08 &= ~0x10;
        mkobj->ang.x = normalize_obj_angle(mkobj->ang.x);
        mkobj->ang.y = normalize_obj_angle(mkobj->ang.y);
        mkobj->ang.z = normalize_obj_angle(mkobj->ang.z);
        YXZ_angles_to_MKMATRIX(
            &mkobj->ang, (MKMATRIX*)matrix);
        changed = 1;
    }
    if ((mkobj->flags_09 & 1) != 0 && mkobj->clump != 0) {
        RpClumpForAllAtomics(
            mkobj->clump, (RpAtomicCallBack)AtomicFaceCamera, 0);
    }
    if ((mkobj->flags_08 & 1) != 0) {
        mkobj->flags_08 =
            (unsigned char)((mkobj->flags_08 & ~0x20) | 0x20);
        mkobj->pos_vel.y += mkobj->gravity * obj_game_speed;
    }
    if ((mkobj->flags_08 & 0x20) != 0) {
        PSVECScale(&mkobj->pos_vel, &scaled, obj_game_speed);
        mkobj->pos.x += scaled.x;
        mkobj->pos.y += scaled.y;
        mkobj->pos.z += scaled.z;
    }
    if ((mkobj->flags_0B & 4) == 0) {
        if ((mkobj->flags_08 & 0xE0) != 0) {
            changed = 1;
            mkobj->flags_08 &= ~0x80;
            matrix->pos = *(RwV3d*)&mkobj->pos;
        }
    } else {
        gxMat33Tx31(
            &pivot, &mkobj->pivot, (Mat33*)matrix);
        PSVECSubtract(&mkobj->pos, &pivot, (Vec*)&matrix->pos);
        changed = 1;
    }
    if ((mkobj->flags_08 & 2) != 0) {
        gxMatScaledByV3(
            (Mat33*)matrix, (Mat33*)matrix, &mkobj->scale);
        changed = 1;
    }

    if (changed != 0) {
        matrix->flags &= ~0x20000;
        if ((mkobj->hide_flags & 0x10) == 0) {
            RwFrameUpdateObjects(mkobj->frame);
            for (i = 1; i < mkobj->clump_count; i++) {
                frame = (RwFrame*)((&mkobj->clump)[i]->object.parent);
                frameMatrix = (RwMatrix*)((char*)frame + 0x10);
                *frameMatrix = *matrix;
                RwFrameUpdateObjects(frame);
            }
        }
    }
    if (mkobj->matrix_count != 0) {
        sobj = (MkSobj*)first_mkhdr(&mkobj->sobj_list);
        if (sobj != 0 && sobj->matrices != 0) {
            for (i = 1; i < (int)mkobj->matrix_count; i++) {
                matrixIndex = mkobj->matrix_indices[i];
                sobj->matrices[matrixIndex] = *matrix;
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

void* find_mkx_rplight_in_obj(void* obj, void* light) {
    MkObj* mkobj = (MkObj*)obj;
    MkPtr* ptr;
    MkxRpLight* link;

    if (mkobj->child_list != 0) {
        ptr = first_mkptr(&mkobj->child_list);
        while (ptr != 0) {
            link = (MkxRpLight*)ptr->hdr;
            if (link == 0 ||
                link->hdr.vtbl->destroy !=
                    (MkVtblFn)vdestroy_mkx_rplight) {
                link = 0;
            }
            if (link != 0) {
                return link;
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
        mk_insert(&link->hdr, &mkobj->child_list);
        link->obj = mkobj;
        link->obj_instance = mkobj->hdr.instance;
    }
}

int vdestroy_mkx_rplight(void* light) {
    MkxRpLight* link = (MkxRpLight*)light;
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
    if (obj == 0 || obj->hdr.instance != link->obj_instance) {
        obj = 0;
    }
    if (obj != 0 && obj->hdr.instance != 0) {
        ((int (*)(void*))obj->hdr.vtbl->destroy)(obj);
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
    RpClump** clumps;
    RpClump* clump;
    int i;

    mkobj->hdr.instance = 0;
    if ((mkobj->hide_flags & 8) != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        clump = clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->allocation_74 != 0) {
        free_mem(mkobj->allocation_74);
    }
    if (mkobj->matrix_indices != 0) {
        free_mem(mkobj->matrix_indices);
    }
    mkobj->field_5C = 0;
    _mwMemFree(mkobj, 0, 0);
}

void destroy_mkobj(void* obj) {
    MkObj* mkobj = (MkObj*)obj;
    RpClump** clumps;
    RpClump* clump;
    int i;

    mkobj->hdr.instance = 0;
    if ((mkobj->hide_flags & 8) != 0) {
        RwFrameDestroy(mkobj->frame);
    }
    destroy_list(&mkobj->list_88);
    destroy_list(&mkobj->sobj_list);
    destroy_list(&mkobj->child_list);
    destroy_list(&mkobj->list_44);
    destroy_list(&mkobj->list_7C);
    destroy_list(&mkobj->list_80);

    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        clump = clumps[i];
        if (clump != 0) {
            pull_clump_from_world(clump);
            RpClumpForAllAtomics(
                clump, atomic_null_texture_pointers, 0);
            RpClumpDestroy(clump);
            clumps[i] = 0;
        }
    }
    if (mkobj->bones != 0) {
        mkobj_destroy_bones(mkobj);
    }
    if (mkobj->allocation_74 != 0) {
        free_mem(mkobj->allocation_74);
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
    int effect;

    if (use_matfx == 0) {
        return material->texture;
    }

    effect = RpMatFXMaterialGetEffects(material);
    switch (effect) {
    case rpMATFXEFFECTENVMAP:
        return ((RpMatFXEnvMapData*)MatFXGetData(
                    material, rpMATFXEFFECTENVMAP))
            ->texture;
    case rpMATFXEFFECTBUMPENVMAP:
        return ((RpMatFXEnvMapData*)MatFXGetData(
                    material, rpMATFXEFFECTENVMAP))
            ->texture;
    case rpMATFXEFFECTBUMPMAP:
        return ((RpMatFXBumpMapData*)MatFXGetData(
                    material, rpMATFXEFFECTBUMPMAP))
            ->texture;
    default:
        return 0;
    }
}

void material_set_texture_pointer(
    RpMaterial* material, RwTexture* texture, int use_matfx) {
    int effect;

    if (use_matfx == 0) {
        material->texture = texture;
        return;
    }

    effect = RpMatFXMaterialGetEffects(material);
    switch (effect) {
    case rpMATFXEFFECTENVMAP:
        ((RpMatFXEnvMapData*)MatFXGetData(
             material, rpMATFXEFFECTENVMAP))
            ->texture = texture;
        break;
    case rpMATFXEFFECTBUMPENVMAP:
        ((RpMatFXEnvMapData*)MatFXGetData(
             material, rpMATFXEFFECTENVMAP))
            ->texture = texture;
        break;
    case rpMATFXEFFECTBUMPMAP:
        ((RpMatFXBumpMapData*)MatFXGetData(
             material, rpMATFXEFFECTBUMPMAP))
            ->texture = texture;
        break;
    }
}

static RpMaterial* material_null_texture_pointer(
    RpMaterial* material, void* data) {
    int effect;
    RpMatFXEnvMapData* env;
    RpMatFXBumpMapData* bump;

    material->texture = 0;
    effect = RpMatFXMaterialGetEffects(material);
    if (effect != rpMATFXEFFECTNULL) {
        if (effect == rpMATFXEFFECTENVMAP) {
            env = (RpMatFXEnvMapData*)MatFXGetData(
                material, rpMATFXEFFECTENVMAP);
            env->texture = 0;
        } else if (effect == rpMATFXEFFECTBUMPMAP) {
            bump = (RpMatFXBumpMapData*)MatFXGetData(
                material, rpMATFXEFFECTBUMPMAP);
            bump->bumped_texture = 0;
            bump->coefficient = 1.0f;
        } else if (effect != rpMATFXEFFECTDUAL &&
                   effect == rpMATFXEFFECTBUMPENVMAP) {
            env = (RpMatFXEnvMapData*)MatFXGetData(
                material, rpMATFXEFFECTENVMAP);
            env->texture = 0;
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
        pdata->prior_scale.x = 1.0f;
        pdata->prior_scale.y = 1.0f;
        pdata->prior_scale.z = 1.0f;
        pdata->elapsed = 0.0f;
    }
    return pdata;
}

static float p_scale(void) {
    ScalePdata* pdata;
    MkObj* obj;
    ScaleScriptEntry* script;
    unsigned int flags;
    float t;

    pdata = (ScalePdata*)apdata;
    if (aproc->pid != 0x5022 || pdata == 0) {
        return -1.0f;
    }
    obj = pdata->obj;
    if (obj == 0 || obj->hdr.instance != pdata->obj_instance) {
        obj = 0;
    }
    if (obj == 0) {
        return -1.0f;
    }

    script = pdata->script;
    flags = script->flags;
    if ((flags & 0x10000) != 0) {
        return -1.0f;
    }
    pdata->elapsed += game_speed;
    if (pdata->elapsed > script->duration) {
        t = 1.0f;
    } else {
        t = pdata->elapsed / script->duration;
        if ((flags & 0x400000) != 0) {
            t = gxMathSin(1.5707964f * t);
        } else if ((flags & 0x800000) != 0) {
            t = 1.0f - gxMathCos(1.5707964f * t);
        }
    }
    if ((flags & 0x40000) != 0) {
        obj->flags_08 |= 2;
        interp_v3(&obj->scale, &script->scale, &pdata->prior_scale, t);
        if (obj->scale.x == obj->scale.y &&
            obj->scale.z == (float)(obj->scale.x == obj->scale.y)) {
            obj->flags_08 &= ~2;
        }
    }
    if (pdata->elapsed >= script->duration) {
        pdata->elapsed -= script->duration;
        pdata->prior_scale = script->scale;
        pdata->script++;
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

void limb_sever_reset_limbs(void* obj) {
    LimbController* controller;
    LimbRuntime* runtime;
    LimbProcLatch* latch;
    MkObj* limb;
    MkSobj* sobj;
    LimbSet* limb_set;
    MkBone* bone;
    MkHdr* hdr;
    MkPtr* walk;
    MkPtr* next;
    void* effect;
    unsigned int i;

    controller = (LimbController*)obj;
    runtime = controller->runtime;
    limb = controller->limb_obj;
    limb_set = 0;
    if (limb != 0) {
        sobj = (MkSobj*)first_mkhdr(&limb->sobj_list);
        if (sobj != 0) {
            limb_set = (LimbSet*)sobj->matrices;
        }
    }

    for (i = 0; i < limb->bone_count; i++) {
        bone = limb->bones[i];
        bone->parent_matrix = bone->original_parent_matrix;
    }

    for (i = 0; i < 15; i++) {
        latch = &runtime->bone_procs[i];
        hdr = latch->hdr;
        if (hdr == 0 || hdr->instance != latch->instance) {
            hdr = 0;
        }
        if (hdr != 0 && hdr->instance != 0) {
            ((void (*)(MkHdr*))hdr->vtbl->destroy)(hdr);
        }
        latch->hdr = 0;
        latch->instance = 0;
        if (limb_set != 0) {
            limb_set->moved_bones &= ~(1U << i);
        }
    }

    latch = &runtime->controller_proc;
    hdr = latch->hdr;
    if (hdr == 0 || hdr->instance != latch->instance) {
        hdr = 0;
    }
    if (hdr != 0 && hdr != (MkHdr*)aproc && hdr->instance != 0) {
        ((void (*)(MkHdr*))hdr->vtbl->destroy)(hdr);
    }
    latch->hdr = 0;
    latch->instance = 0;

    walk = runtime->proc_list;
    while (walk != 0) {
        hdr = walk->hdr;
        if (walk->instance == hdr->instance) {
            if (hdr != 0 && hdr != (MkHdr*)aproc && hdr->instance != 0) {
                ((void (*)(MkHdr*))hdr->vtbl->destroy)(hdr);
            }
            walk = walk->next;
        } else {
            next = walk->next;
            walk->hdr = 0;
            destroy_mkptr(walk);
            walk = next;
        }
    }
    runtime->proc_list = 0;

    limb_sever_hide_z_meat_chunks_all(limb);
    if (controller->type == 6 && controller->effect_bank >= 0) {
        effect = find_pfx_by_name_by_bankowner(
            "eyelt", 1U << controller->hdr.instance);
        if (effect != 0) {
            reset_effect_ppfx(effect);
        }
        pfx_spawn_at_bid("eyelt", limb, 0x10);
        effect = find_pfx_by_name_by_bankowner(
            "eyert", 1U << controller->hdr.instance);
        if (effect != 0) {
            reset_effect_ppfx(effect);
        }
        pfx_spawn_at_bid("eyert", limb, 0x10);
    }
}

void limb_sever_show_z_meat_chunks_all_plyr_num(int plyr) {
    if (plyr == 0) {
        limb_sever_show_z_meat_chunks_all(g_game_info.plyr0.slot.mirror_a);
    } else if (plyr == 1) {
        limb_sever_show_z_meat_chunks_all(g_game_info.plyr1.slot.mirror_a);
    }
}

void limb_sever_show_z_meat_chunks_all(void* obj) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    unsigned int material_id_base;
    unsigned int material_id;
    unsigned int chunk_offset;
    unsigned int material_offset;
    unsigned int material_count;
    int chunk_id;

    mkobj = (MkObj*)obj;
    if (get_player_number(obj) == 0) {
        if (g_game_info.plyr0.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    } else {
        if (g_game_info.plyr1.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    }

    chunk_offset = 0;
    while (*(int*)((char*)limb_meat_chunk_list + chunk_offset) > -1) {
        chunk_id = *(int*)((char*)limb_meat_chunk_list + chunk_offset);
        material_id = material_id_base + (unsigned int)chunk_id;
        material = 0;
        ptr = first_mkptr(&mkobj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material_offset = 0;
            material_count = geometry->matList.numMaterials;
            while (material_count != 0) {
                material = *(RpMaterial**)((char*)geometry->matList.materials +
                                           material_offset);
                mkmat = (MkmaterialPluginData*)((char*)material +
                                                MkmaterialLocalOffset);
                if ((mkmat->flags & 0xFFF) == material_id) {
                    break;
                }
                material_offset += 4;
                material_count--;
            }
            if (material_count == 0) {
                material = 0;
            }
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

void limb_sever_show_z_meat_chunks(void* obj, int limb, int show_all) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    unsigned int material_id_base;
    unsigned int material_id;
    unsigned int material_offset;
    unsigned int material_count;
    int* material_ids;
    int shown;

    mkobj = (MkObj*)obj;
    shown = 0;
    if (get_player_number(obj) == 0) {
        if (g_game_info.plyr0.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    } else {
        if (g_game_info.plyr1.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    }

    material_ids = limb_meats_mat_id_tbl[limb];
    while (*material_ids != -1) {
        material_id = material_id_base + (unsigned int)*material_ids;
        material = 0;
        ptr = first_mkptr(&mkobj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material_offset = 0;
            material_count = geometry->matList.numMaterials;
            while (material_count != 0) {
                material = *(RpMaterial**)((char*)geometry->matList.materials +
                                           material_offset);
                mkmat = (MkmaterialPluginData*)((char*)material +
                                                MkmaterialLocalOffset);
                if ((mkmat->flags & 0xFFF) == material_id) {
                    break;
                }
                material_offset += 4;
                material_count--;
            }
            if (material_count == 0) {
                material = 0;
            }
            if (material != 0) {
                break;
            }
            ptr = next_mkptr(ptr);
        }
        if (material != 0) {
            show_material(material);
        }
        material_ids++;
        if (shown != 0 && show_all == 0) {
            break;
        }
        shown++;
    }
}

void limb_sever_hide_z_meat_chunks_all(void* obj) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;
    RpGeometry* geometry;
    RpMaterial* material;
    MkmaterialPluginData* mkmat;
    unsigned int material_id_base;
    unsigned int material_id;
    unsigned int chunk_offset;
    unsigned int material_offset;
    unsigned int material_count;
    int chunk_id;

    mkobj = (MkObj*)obj;
    if (get_player_number(obj) == 0) {
        if (g_game_info.plyr0.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    } else {
        if (g_game_info.plyr1.slot.fighter->limb_material_bank != 0) {
            material_id_base = 0x400;
        } else {
            material_id_base = 0;
        }
    }

    chunk_offset = 0;
    while (*(int*)((char*)limb_meat_chunk_list + chunk_offset) > -1) {
        chunk_id = *(int*)((char*)limb_meat_chunk_list + chunk_offset);
        material_id = material_id_base + (unsigned int)chunk_id;
        material = 0;
        ptr = first_mkptr(&mkobj->sobj_list);
        while (ptr != 0) {
            sobj = (MkSobj*)ptr->hdr;
            geometry = sobj->atomic->geometry;
            material_offset = 0;
            material_count = geometry->matList.numMaterials;
            while (material_count != 0) {
                material = *(RpMaterial**)((char*)geometry->matList.materials +
                                           material_offset);
                mkmat = (MkmaterialPluginData*)((char*)material +
                                                MkmaterialLocalOffset);
                if ((mkmat->flags & 0xFFF) == material_id) {
                    break;
                }
                material_offset += 4;
                material_count--;
            }
            if (material_count == 0) {
                material = 0;
            }
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
    void* source_arg, void* limb_arg, int bone_index, int include_children);

/* Soft ceiling: obj_sever_limb ~37.46% - fighter chain FX remain; stop. */
void* obj_sever_limb(
    void* obj, int limb, Vec* limb_velocities, int include_children) {
    MkObj* source;
    MkObj* severed;
    MkSobj* sobj;
    MkBone* root;
    MkPtr* sobj_ref;
    Vec saved_pos;
    Vec transformed;
    Vec correction;
    int root_index;
    int saved_fallback;
    unsigned int i;

    source = (MkObj*)obj;
    sobj = (MkSobj*)first_mkhdr(&source->sobj_list);
    if (sobj == 0) {
        return 0;
    }
    sobj->flags09 = (unsigned char)((sobj->flags09 & ~0x10) | 0x10);
    if (sobj->matrices == 0) {
        return 0;
    }

    severed = (MkObj*)get_mkobj_frame(
        source->oid == 0x1001 ? 0x1005 :
        source->oid == 0x1002 ? 0x1006 : 0x1007, 0);
    if (severed == 0) {
        return 0;
    }
    severed->parent_hdr = &source->hdr;
    severed->parent_inst = source->hdr.instance;
    severed->flags_0C = (unsigned char)((severed->flags_0C & ~2) | 2);
    severed->hide_flags =
        (unsigned char)((severed->hide_flags & ~0x20) | 0x20);
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
    severed->flipped_bones = source->flipped_bones;
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

    severed->field_24 = &sobj->matrices[limb];
    _move_bones_from_obj_to_limbobj(
        source, severed, limb, include_children);
    severed->pos = source->pos;
    severed->pos_vel = source->pos_vel;
    severed->ang = source->ang;
    severed->ang_vel = source->ang_vel;
    severed->scale = source->scale;
    memcpy(severed->field_24, source->field_24, sizeof(RwMatrix));

    root_index = limb_root_bids[limb];
    severed->fallback_bone_index = root_index;
    root = severed->bones[root_index];
    if (root != 0 && root->parent_matrix != 0) {
        bone_make_parents_my_children(root);
        v3_x_mat(
            &transformed, (Vec*)&root->parent_matrix->pos,
            (MKMATRIX*)severed->field_24);
        severed->pos.x += transformed.x;
        severed->pos.y += transformed.y;
        severed->pos.z += transformed.z;
        root->translation.x = 0.0f;
        root->translation.y = 0.0f;
        root->translation.z = 0.0f;
        RtQuatConvertFromMatrix(
            (RtQuat*)&root->rotation, root->parent_matrix);
        update_mkobj(as_mkhdr(&severed->hdr));
        saved_fallback = severed->fallback_bone_index;
        severed->fallback_bone_index = root->bone_index;
        update_bone_hierarchy(as_mkhdr(&severed->hdr));
        severed->fallback_bone_index = saved_fallback;
    }

    if (limb_velocities != 0) {
        if ((severed->flags_0B & 4) == 0) {
            saved_pos = severed->pos;
        } else {
            gxMat33Tx31(
                &transformed, &severed->pivot,
                (Mat33*)severed->field_24);
            PSVECSubtract(&severed->pos, &transformed, &saved_pos);
        }
        severed->pivot = limb_velocities[limb];
        severed->flags_0B =
            (unsigned char)((severed->flags_0B & ~4) | 4);
        gxMat33Tx31(
            &transformed, &severed->pivot,
            (Mat33*)severed->field_24);
        PSVECSubtract(&severed->pos, &transformed, &correction);
        PSVECSubtract(&correction, &saved_pos, &transformed);
        PSVECSubtract(&severed->pos, &transformed, &severed->pos);
    }
    mk_insert(&severed->hdr, &fgnd_mkobj_list);
    severed->flags_08 =
        (unsigned char)((severed->flags_08 & ~0x40) | 0x40);
    mk_insert(&severed->hdr, &bone_hierarchy_mkobj_list);
    return severed;
}

static void xfer_bone_tree_from_obj_to_limb_obj(
    int bone_index, MkBone* parent, void* source_obj, void* limb_obj);
static void scan_tree_to_xfer_bone_tree_from_obj_to_limb_obj(
    void* source_arg, void* limb_arg, int limb_id);

static void _move_bones_from_obj_to_limbobj(
    void* source_arg, void* limb_arg, int bone_index, int include_children) {
    MkObj* source;
    MkObj* limb;
    MkSobj* sobj;
    LimbSet* limb_set;
    int* child;

    source = (MkObj*)source_arg;
    limb = (MkObj*)limb_arg;
    sobj = (MkSobj*)first_mkhdr(&source->sobj_list);
    if (sobj == 0 || sobj->matrices == 0) {
        return;
    }
    limb_set = (LimbSet*)sobj->matrices;
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

static void scan_tree_to_xfer_bone_tree_from_obj_to_limb_obj(
    void* source_arg, void* limb_arg, int limb_id) {
    MkObj* source;
    MkBone* queue[150];
    MkBone* bone;
    unsigned int read_index;
    unsigned int count;

    source = (MkObj*)source_arg;
    read_index = 0;
    count = 1;
    queue[0] = source->bones[source->fallback_bone_index];
    bone = queue[0]->root_next;
    while (bone != 0) {
        queue[count] = bone;
        count++;
        bone = bone->root_next;
    }

    while (read_index < count) {
        bone = queue[read_index];
        read_index++;
        if (bone->parent_matrix != 0) {
            if (bone->limb_id == limb_id) {
                xfer_bone_tree_from_obj_to_limb_obj(
                    bone->bone_index, 0, source_arg, limb_arg);
                break;
            }
            bone = bone->tree_child;
            while (bone != 0) {
                queue[count] = bone;
                count++;
                bone = bone->tree_next;
            }
        }
    }
}

static void xfer_bone_tree_from_obj_to_limb_obj(
    int bone_index, MkBone* parent, void* source_arg, void* limb_arg) {
    MkObj* source;
    MkObj* limb;
    MkBone* source_bone;
    MkBone* clone_parent;
    MkBone* new_bone;
    int destination_index;

    source = (MkObj*)source_arg;
    limb = (MkObj*)limb_arg;
    while (1) {
        source_bone = source->bones[bone_index];
        if (source_bone != 0 && source_bone->parent_matrix != 0) {
            clone_parent = 0;
            if (source_bone->clone_source == 0) {
                destination_index = bone_index;
                if (limb->bones[bone_index] != 0) {
                    return;
                }
            } else {
                destination_index = source_bone->clone_source->bone_index;
                clone_parent = limb->bones[destination_index];
                if (clone_parent == 0 && limb->bones[bone_index] != 0) {
                    return;
                }
            }

            new_bone = alloc_bone();
            limb->bones[destination_index] = new_bone;
            if (new_bone == 0) {
                return;
            }
            memcpy(new_bone, source_bone, 0x110);
            new_bone->bone_index = destination_index;
            new_bone->field_60 = 1.0f;
            new_bone->field_64 = 1.0f;
            new_bone->update_tick = (unsigned int)(exec_tick_ctr - 1);
            new_bone->transform_parent = 0;
            new_bone->tree_next = 0;
            new_bone->tree_child = 0;
            new_bone->root_next = 0;
            new_bone->clone_source = 0;
            new_bone->list_80 = 0;
            new_bone->flags_54 &= (unsigned char)~2;
            new_bone->flags_54 &= (unsigned char)~4;
            if (clone_parent != 0) {
                mkbone_insert_child_of_clone_parent(new_bone, clone_parent);
            }
            new_bone->tag = source_bone->tag;
            new_bone->limb_id = source_bone->limb_id;
            new_bone->parent_matrix = source_bone->parent_matrix;
            if (parent == 0) {
                limb->fallback_bone_index = destination_index;
            } else {
                mkbone_insert_child_of_parent(new_bone, parent);
            }
            mkbone_remove(source_bone);
            if (source_bone->tree_child != 0) {
                xfer_bone_tree_from_obj_to_limb_obj(
                    source_bone->tree_child->bone_index, new_bone,
                    source_arg, limb_arg);
            }
        }
        if (parent == 0 || source_bone->tree_next == 0) {
            return;
        }
        bone_index = source_bone->tree_next->bone_index;
    }
}

void* get_mkobj(int type, void* clump) {
    MkObj* obj;

    obj = (MkObj*)get_mkobj_frame(
        type, ((RpClump*)clump)->object.parent);
    if (obj != 0) {
        obj->clump_count = 1;
        obj->clump = (RpClump*)clump;
    }
    return obj;
}

void* get_mkobj_frame(int type, void* frame) {
    MkObj* obj;
    RwMatrix* matrix;

    obj = (MkObj*)_mwMemMalloc(
        mkobj_heap, 0x100, 4, 0, 0, 0);
    if (obj == 0) {
        return 0;
    }
    obj->hdr.vtbl = &vtbl_mkobj;
    mk_set_instance(&obj->hdr.instance);
    obj->flags_word_08 = 0;
    obj->flags_0C = 0;
    if (frame == 0) {
        frame = RwFrameCreate();
        if (frame == 0) {
            obj->hdr.instance = 0;
            _mwMemFree(obj, 0, 0);
            return 0;
        }
        obj->hide_flags =
            (unsigned char)((obj->hide_flags & ~8) | 8);
        obj->hide_flags =
            (unsigned char)((obj->hide_flags & ~0x10) | 0x10);
    }
    mk_insert(&obj->hdr, &master_clean_up_list);
    obj->child_list = 0;
    obj->sobj_list = 0;
    obj->parent_hdr = 0;
    obj->parent_inst = 0;
    obj->list_44 = 0;
    obj->list_88 = 0;
    obj->oid = (unsigned int)type;
    obj->clump_count = 0;
    obj->frame = (RwFrame*)frame;
    obj->field_24 = (RwMatrix*)((char*)frame + 0x10);
    obj->bones = 0;
    obj->bone_count = 0;
    obj->fallback_bone_index = 0;
    obj->matrix_indices = 0;
    obj->matrix_count = 0;
    obj->flags_08 =
        (unsigned char)((obj->flags_08 & ~0x80) | 0x80);
    obj->flags_08 &= ~0x10;
    matrix = obj->field_24;
    obj->pos = *(Vec*)&matrix->pos;
    obj->pos_vel.x = 0.0f;
    obj->pos_vel.y = 0.0f;
    obj->pos_vel.z = 0.0f;
    obj->ang.x = 0.0f;
    obj->ang.y = 0.0f;
    obj->ang.z = 0.0f;
    obj->ang_vel.x = 0.0f;
    obj->ang_vel.y = 0.0f;
    obj->ang_vel.z = 0.0f;
    obj->gravity = 0.0f;
    obj->ground_colls = 0;
    obj->ground_colls_y = 0.0f;
    obj->field_5C = 0;
    obj->allocation_74 = 0;
    obj->list_7C = 0;
    obj->list_80 = 0;
    obj->flipped_bones = 0;
    return obj;
}

void insert_ground_me_mkobj(void* obj) {
    mk_insert((MkHdr*)obj, &ground_me_mkobj_list);
}

void pull_bone_hierarchy_mkobj(void* obj) {
    mk_pull_discard((MkHdr*)obj, &bone_hierarchy_mkobj_list);
}

void insert_bone_hierarchy_mkobj(void* obj) {
    mk_insert((MkHdr*)obj, &bone_hierarchy_mkobj_list);
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

void obj_set_rw_lights(void* obj) {
    MkObj* mkobj;
    LightMkList* entry;
    MkPtr* ptr;
    MkxRpLight* wrapper;
    unsigned int flags;
    int active;
    int i;

    mkobj = (MkObj*)obj;
    flags = mkobj == 0 ? 0 : mkobj->light_flags;
    skip_light_setup = 0;
    if (last_obj_light_flags == flags) {
        skip_light_setup = 1;
        return;
    }
    entry = light_mklists;
    for (i = 0; i < 13; i++, entry++) {
        if (entry->state == 1) {
            if ((flags & entry->mask) != 0) {
                continue;
            }
            active = 0;
            entry->state = 0;
        } else if (entry->state == 0) {
            if ((flags & entry->mask) == 0) {
                continue;
            }
            active = 1;
            entry->state = 1;
        } else {
            active = (flags & entry->mask) != 0;
            entry->state = active;
        }
        if (entry->list != 0) {
            ptr = first_mkptr(entry->list);
            while (ptr != 0) {
                wrapper = (MkxRpLight*)ptr->hdr;
                if (active != 0) {
                    wrapper->light->object.flags |= 3;
                } else {
                    wrapper->light->object.flags &= ~3u;
                }
                ptr = next_mkptr(ptr);
            }
        }
    }
    uploaded_light_state = 0;
    last_obj_light_flags = flags;
}

void force_rw_lights(void) {
    LightMkList* entry;
    int count;

    entry = light_mklists;
    count = 13;
    do {
        entry->state = 2;
        entry++;
        count--;
    } while (count != 0);
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

void obj_set_sobj_priority(void* obj, int id, int priority) {
    MkObj* mkobj;
    RpClump** clumps;
    SobjCreateData data;
    int i;

    if (obj == 0) {
        return;
    }
    mkobj = (MkObj*)obj;
    data.obj = mkobj;
    data.id = (unsigned int)id;
    data.mask = 0xFFF;
    data.result = 0;
    data.set_priority = 0;
    data.priority = 0x10;
    clumps = &mkobj->clump;
    for (i = 0; i < mkobj->clump_count; i++) {
        if (clumps[i] != 0) {
            RpClumpForAllAtomics(
                clumps[i], atomic_create_sobj_callback, &data);
        }
    }
    if (data.result != 0) {
        sobj_set_priority(data.result, priority);
    }
}

void sobj_set_alpha(void* sobj, int alpha) {
    set_atomic_material_alpha(((MkSobj*)sobj)->atomic, alpha);
}

void sobj_no_zwrite(void* sobj, int flag) {
    ((MkSobj*)sobj)->flags09 |= 0x80;
}

void sobj_disable_blending(void* sobj) {
    ((MkSobj*)sobj)->render_flags = 0x20001;
}

void sobj_set_transl_flag(void* sobj) {
    RpAtomic* atomic;
    MksobjPluginData* plugin;
    MkSobj* linked;

    atomic = ((MkSobj*)sobj)->atomic;
    plugin = (MksobjPluginData*)((char*)atomic + MksobjLocalOffset);
    plugin->flags |= 0x80000000;
    linked = plugin->sobj;
    if (linked != 0) {
        linked->id_flags |= 0x80000000;
        linked->priority = 0x12;
    }
}

void atomic_set_transl_flag(void* atomic) {
    RpAtomic* rpAtomic;
    MksobjPluginData* plugin;
    MkSobj* sobj;

    rpAtomic = (RpAtomic*)atomic;
    plugin = (MksobjPluginData*)((char*)rpAtomic + MksobjLocalOffset);
    plugin->flags |= 0x80000000;
    sobj = plugin->sobj;
    if (sobj != 0) {
        sobj->id_flags |= 0x80000000;
        sobj->priority = 0x12;
    }
}

void unhide_sobj_by_sobj_id(void* obj, unsigned int id) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;

    mkobj = (MkObj*)obj;
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFF) == id) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (ptr == 0) {
        sobj = 0;
    }
    if (sobj != 0) {
        sobj->atomic->object.flags = 4;
    }
}

void hide_sobj_by_sobj_id(void* obj, unsigned int id) {
    MkObj* mkobj;
    MkPtr* ptr;
    MkSobj* sobj;

    mkobj = (MkObj*)obj;
    ptr = first_mkptr(&mkobj->sobj_list);
    while (ptr != 0) {
        sobj = (MkSobj*)ptr->hdr;
        if ((sobj->id_flags & 0xFFF) == id) {
            break;
        }
        ptr = next_mkptr(ptr);
    }
    if (ptr == 0) {
        sobj = 0;
    }
    if (sobj != 0) {
        sobj->atomic->object.flags &= ~4u;
    }
}

void unhide_sobj(void* sobj_arg) {
    MkSobj* sobj = (MkSobj*)sobj_arg;

    sobj->atomic->object.flags = 0x4;
}

void hide_sobj(void* sobj_arg) {
    MkSobj* sobj = (MkSobj*)sobj_arg;

    sobj->atomic->object.flags = (unsigned char)(sobj->atomic->object.flags & ~0x4u);
}

void unhide_obj(void* obj) {
    ((MkObj*)obj)->hide_flags &= ~0x20u;
}

void hide_obj(void* obj) {
    ((MkObj*)obj)->hide_flags |= 0x20;
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
