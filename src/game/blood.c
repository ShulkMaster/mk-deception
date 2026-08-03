#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "runtime/asset.h"
#include "runtime/section.h"
#include "game/pfxscript.h"
#include "game/game_info.h"
#include "math/mk_math.h"

#define BLOOD_SPLAT_COUNT 24

typedef struct BloodSplat {
    int field_00;
    float reset_height; /* +0x04 */
    char pad08[0x10];
} BloodSplat; /* 0x18 */

typedef void (*BloodProcDestroyFn)(MkProc* proc);

typedef struct BloodProcVtablePrefix {
    void* reserved[4];
    BloodProcDestroyFn destroy;
} BloodProcVtablePrefix;

typedef union BloodProcVtableRef {
    MkVtableMkproc* base;
    BloodProcVtablePrefix* blood;
} BloodProcVtableRef;

typedef struct BloodProcLatch {
    MkProc* proc;
    unsigned int instance;
} BloodProcLatch;

typedef struct DecalEmitterWatcherPdata {
    MkHdr hdr;                /* +0x00 */
    char pad08[8];
    MKMATRIX matrices[10];    /* +0x10 */
    char pad290[4];
    int matrix_count;         /* +0x294 */
    char pad298[8];
} DecalEmitterWatcherPdata; /* 0x2A0 */

typedef struct BloodFxFlags {
    unsigned char pad_high : 3;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char pad_low : 3;
} BloodFxFlags;

typedef struct BloodFxUserdata {
    char pad00[0x40];
    union {
        unsigned char flags_40;
        BloodFxFlags flags_40_bits;
    };
} BloodFxUserdata;

typedef struct GusherStep {
    int blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

typedef struct GusherPdata {
    MkHdr hdr;
    GusherStep* steps;
    int owner;
    MkObj* object;
    unsigned int object_instance;
    int bone;
    Vec position;
    Vec direction;
    float velocity_max;
    float velocity_min;
    GusherStep* current_step;
} GusherPdata; /* 0x40 */

typedef struct BleedGroundWatcherPdata {
    MkHdr hdr;
    FighterMirror* decal_owner;
    MkObj* blood_object;
    unsigned int blood_object_instance;
    unsigned int effect_handle;
    unsigned char flags;
} BleedGroundWatcherPdata;

typedef struct FootPrintPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    FighterMirror* decal_owner;
    Vec bone_offset;
    int use_right_foot;
    Vec left_position;
    Vec right_position;
} FootPrintPdata;

typedef struct BloodDecalArrayView {
    const char* names[7];
} BloodDecalArrayView;

typedef struct BloodBoneMapEntry {
    char pad00[0x10];
    int bone;
    char pad14[0x6C];
} BloodBoneMapEntry; /* 0x80 */

typedef struct BloodSpawnTarget {
    int field_00;
    int bone_count;
    const int* bone_indices;
    int field_0C;
    int field_10;
    int field_14;
    int field_18;
} BloodSpawnTarget; /* 0x1C */

typedef struct BloodSpawnState {
    char pad00[0x10];
    BloodBoneMapEntry* bone_map;
    BloodSpawnTarget targets[1];
} BloodSpawnState;

typedef struct BloodSpawnStep {
    int target_index;
    int field_04;
    int blood_type;
    int field_0C;
    int delay;
} BloodSpawnStep; /* 0x14 */

typedef struct BleedPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    BloodSpawnStep* step;
    BloodSpawnState* spawn_state;
    int bone;
    unsigned int art_id;
    PlyrPdata* owner;
    int timer;
} BleedPdata; /* 0x28 */

typedef struct BloodSurfaceRecord {
    char pad00[0x10];
    int bone;
    char pad14[0x0C];
    Vec points[3];
    char pad44[0x3C];
} BloodSurfaceRecord; /* 0x80 */

typedef struct BloodSurface {
    char pad00[0x10];
    BloodSurfaceRecord* records;
} BloodSurface;

typedef struct BloodPath {
    BloodSurface* surface;
    int point_count;
    const int* record_indices;
    const int* corner_indices;
    float interpolation_bias;
    float speed_base;
    float speed_scale;
} BloodPath; /* 0x1C */

typedef struct BloodVelocityState {
    Vec velocity;
    float pad0C;
    float travel_ticks;
    char pad14[4];
    BloodPath* path;
    int point_index;
    char pad20[8];
    float weight_0;
    float weight_1;
    float weight_step;
} BloodVelocityState; /* 0x34 */

/* Contiguous authored data block used by the player blood scripts. */
typedef struct BloodAssetData {
    unsigned char scorpion_sweat_vertices[0x3068];
    int blood_levels[12];               /* +0x3068 */
    char* blood_map[11];                /* +0x3098 */
    unsigned char shared_scripts[0x240];
    int front_left_script[20];          /* +0x3304 */
    unsigned char left_scripts[0x140];
    int front_right_script[20];         /* +0x3494 */
} BloodAssetData;

static const char* blood_decal_to_reset[] = {
    "blsplat",
    "blsplat2",
    "blpuddle",
    "blpuddle2",
    "blsmash",
    "blfoot",
    0,
};
static unsigned int decal_tick_counter;
int blood_type_list[12] = {1, 2, 2, 3, 3, 3, 3, 2, 3, 1, 2, 3};
extern BloodSplat ncs_blood_splat_list[BLOOD_SPLAT_COUNT];
extern BloodProcLatch ncs_pfx_decal_emitter_proc;
extern BloodProcLatch bleed_pfx_proc_item;
extern BloodProcLatch bleed_proc_item;
extern MkPtr* gusher_list;
extern int bleed_startup__fire_off_splat_watcher_func;
extern MkVtable5 vtbl_mkpdata_generic;
extern float game_speed;
extern int exec_tick_ctr;
extern BloodDecalArrayView mkpfx_ncs_decal_array;
extern BloodAssetData scorpion_sweat_bld_src_verts;

void* memset(void* destination, int value, unsigned long size);
unsigned int fx_by_owner(const char* name, int owner, ...);
void spawn_decal_emitter(
    const char* name, FighterMirror* owner, const Vec* position,
    const MKMATRIX* orientation,
    float angle);
void start_blood_splat_watcher(void);
void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
void calc_bone_world_mat(MkObj* object, int bone);
void spawn_bld_fall(
    int blood_type, MkBone* bone, const Vec* position,
    const Vec* velocity, int owner);
void plyr_bleed_small_cycle_ext(int owner, int bone, int source_owner);
void obj_set_bone_calc_world_mat_flag(MkObj* object, int bone);
unsigned int fx_next_emitter(void);
void fx_resume_emit(void);
MkPfx* pfx_from_emitter(unsigned int emitter);
int emitter_id_from_handle(unsigned int emitter);
float gxMathArcTanYX(float y, float x);
int strcmp(const char* left, const char* right);
PfxEmitter* pfx_get_emitter(void* pfx_vm, int emitter_index);
int obj_spawn_bld(
    MkObj* object, MkBone* bone, int blood_type, const int* script,
    void* spawn_state, int count, const Vec* velocity, unsigned int art_id,
    PlyrPdata* owner);

static float p_decal_emitter_watcher(void);
static float p_gusher(void);
static float p_watch_bleed_obj_for_gnd_coll(void);
static float p_foot_print(void);
static float p_bleed(void);
static float p_pfx_bleed(void);
static void do_pfx_bleed(MkHdr* hdr);
static int obj_set_bld_vel(
    MkObj* object, const Vec* position, BloodVelocityState* state);

int is_blood_disabled(void) {
    return get_blood_level() < 2;
}

void plyr_bleed_mouth(PlyrPdata* pdata) {
    BloodAssetData* assets;
    MkObj* object;
    int art_section;
    unsigned int blood_art_id;

    assets = &scorpion_sweat_bld_src_verts;
    if (get_blood_level() >= assets->blood_levels[3] &&
        pdata->blood_model_data != 0) {
        object = pdata->tracked_obj;
        if (object != 0) {
            if (object->hdr.instance == pdata->tracked_obj_instance) {
                /* The instance latch is still live. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            art_section = get_shared_art_section_for_plyr_pdata(pdata);
            blood_art_id = get_artid_of_named_item_in_slot(
                art_section, assets->blood_map[4], 1);
            obj_spawn_bld(
                object, 0, 2, assets->front_left_script,
                pdata->left_blood_spawn_state, 9, 0, blood_art_id, pdata);
            obj_spawn_bld(
                object, 0, 2, assets->front_right_script,
                pdata->right_blood_spawn_state, 9, 0, blood_art_id, pdata);
        }
    }
}

GusherPdata* start_gusher(
    GusherStep* steps, int owner, MkObj* object, int bone,
    const Vec* position, const Vec* direction) {
    GusherPdata* pdata;

    if (get_blood_level() < blood_type_list[5]) {
        return 0;
    }
    if (bone != 0x40000000 && object->bones[bone] == 0) {
        return 0;
    }
    if (_create_mkproc_generic_nostack(
            0x501B, 0x2E, p_gusher, sizeof(GusherPdata),
            (MkHdr**)&pdata) != 0) {
        pdata->current_step = steps;
        pdata->steps = steps;
        pdata->owner = owner;
        pdata->object = object;
        pdata->object_instance = object->hdr.instance;
        pdata->bone = bone;
        pdata->position.x = position->x;
        pdata->position.y = position->y;
        pdata->position.z = position->z;
        /* Retail redundantly copies this vector twice. */
        pdata->position.x = position->x;
        pdata->position.y = position->y;
        pdata->position.z = position->z;
        pdata->direction.x = direction->x;
        pdata->direction.y = direction->y;
        pdata->direction.z = direction->z;
        pdata->velocity_max = 0.18f;
        pdata->velocity_min = 0.15f;
    }
    return pdata;
}

static float p_gusher(void) {
    GusherPdata* pdata;
    MkObj* object;
    MkBone* bone;
    Vec velocity;
    float velocity_scale;
    float time;

    pdata = (GusherPdata*)apdata;
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    bone = object->bones[pdata->bone];
    if (bone == 0 || bone->parent_matrix == 0) {
        return -1.0f;
    }

    time = -game_speed;
    while (time < 0.0f) {
        while (pdata->current_step->blood_type == 0) {
            pdata->current_step = pdata->steps;
            pdata->velocity_max =
                0.98f * (pdata->velocity_max - 0.01f) + 0.01f;
            pdata->velocity_min =
                0.98f * (pdata->velocity_min - 0.0075f) + 0.0075f;
        }

        calc_bone_world_mat(object, pdata->bone);
        bone = object->bones[pdata->bone];
        if (bone == 0) {
            return -1.0f;
        }

        v3_x_mat(
            &velocity, &pdata->direction, (MKMATRIX*)&bone->matrix);
        velocity_scale =
            sfrand_ab(pdata->velocity_min, pdata->velocity_max) *
            pdata->current_step->velocity_scale;
        velocity.x *= velocity_scale;
        velocity.y *= velocity_scale;
        velocity.z *= velocity_scale;
        spawn_bld_fall(
            pdata->current_step->blood_type, bone, &pdata->position,
            &velocity, pdata->owner);
        pdata->current_step++;
        time += pdata->current_step->interval;
    }

    time += 0.99f;
    if (pdata->owner != 0 && random_percent(0.014f * time) != 0) {
        plyr_bleed_small_cycle_ext(
            pdata->owner, pdata->bone, pdata->owner);
    }
    return time;
}

static float p_watch_bleed_obj_for_gnd_coll(void) {
    BleedGroundWatcherPdata* pdata;
    MkObj* object;

    pdata = (BleedGroundWatcherPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    object = pdata->blood_object;
    if (object != 0 &&
        object->hdr.instance != pdata->blood_object_instance) {
        object = 0;
    }
    if (object != 0) {
        if (object->pos.y > g_game_info.field_34) {
            object->pos_vel.y -= 0.0015f * game_speed;
            object->pos_vel.x *= 0.99f;
            object->pos_vel.y *= 0.99f;
            object->pos_vel.z *= 0.99f;
            return 1.0f;
        }
        if (object->pos.y >= g_game_info.field_34 - 1.0f &&
            (pdata->flags & 0xC0) != 0) {
            object->pos.y = g_game_info.field_34 + 0.01f;
            spawn_decal_emitter(
                "blsplat", pdata->decal_owner, &object->pos, 0, 0.0f);
        }
        if (object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy((MkHdr*)object);
        }
    }
    fx_reset_emit(pdata->effect_handle);
    return -1.0f;
}

static float p_foot_print(void) {
    FootPrintPdata* pdata;
    MkObj* object;
    Vec* previous_position;
    Vec position;
    int bone;
    float angle;
    float delta_x;
    float delta_z;

    pdata = (FootPrintPdata*)apdata;
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return 60.0f;
    }

    if (pdata->use_right_foot != 0) {
        angle = -1.5707964f;
        previous_position = &pdata->right_position;
        bone = 0xB;
    } else {
        angle = 1.5707964f;
        previous_position = &pdata->left_position;
        bone = 0xA;
    }

    get_bone_offset_world_pos(
        object, bone, &pdata->bone_offset, &position);
    if (position.y <= object->ground_colls_y + 0.1f) {
        delta_x = previous_position->x - position.x;
        delta_z = previous_position->z - position.z;
        if (delta_x * delta_x + delta_z * delta_z >= 0.5f) {
            position.y = object->ground_colls_y + 0.005f;
            spawn_decal_emitter(
                mkpfx_ncs_decal_array.names[5],
                pdata->decal_owner, &position,
                &object->bones[bone]->matrix, angle);
            previous_position->x = position.x;
            previous_position->z = position.z;
        }
    }
    pdata->use_right_foot = 1 - pdata->use_right_foot;
    return 20.0f;
}

void spawn_decal_emitter(
    const char* name, FighterMirror* owner, const Vec* position,
    const MKMATRIX* orientation, float angle) {
    MkProc* watcher_proc;
    DecalEmitterWatcherPdata* watcher;
    MKMATRIX* matrix;
    MkPfx* pfx;
    PfxEmitter* emitter_vm;
    unsigned int emitter;
    int emitter_index;
    int index;

    if (owner != 0 && get_blood_level() < blood_type_list[1]) {
        for (index = 0; index < 6; index++) {
            if (strcmp(mkpfx_ncs_decal_array.names[index], name) == 0) {
                return;
            }
        }
    }

    if (owner != 0 &&
        (unsigned int)(exec_tick_ctr - decal_tick_counter) >=
            randu0(75) + 25) {
        decal_tick_counter = exec_tick_ctr;
    }

    watcher_proc = ncs_pfx_decal_emitter_proc.proc;
    if (watcher_proc != 0 &&
        (unsigned int)watcher_proc->instance !=
            ncs_pfx_decal_emitter_proc.instance) {
        watcher_proc = 0;
    }
    if (watcher_proc == 0) {
        ncs_pfx_decal_emitter_proc.proc = 0;
        ncs_pfx_decal_emitter_proc.instance = 0;
        return;
    }

    watcher = (DecalEmitterWatcherPdata*)pdata_of_proc(watcher_proc);
    if (watcher == 0) {
        if (watcher_proc->instance != 0) {
            BloodProcVtableRef vtbl;

            vtbl.base = watcher_proc->vtbl;
            vtbl.blood->destroy(watcher_proc);
        }
        ncs_pfx_decal_emitter_proc.proc = 0;
        ncs_pfx_decal_emitter_proc.instance = 0;
        return;
    }
    if (watcher->matrix_count >= 10 || g_game_info.bgnd_obj == 0) {
        return;
    }

    matrix = &watcher->matrices[watcher->matrix_count];
    if (angle != 0.0f) {
        if (orientation != 0) {
            angle -= gxMathArcTanYX(
                orientation->at.x, orientation->at.z);
        }
        y_angle_to_MKMATRIX(matrix, angle);
    } else {
        MKMatrixSetIdentity(matrix);
    }
    matrix->pos.x = position->x;
    matrix->pos.y = position->y;
    matrix->pos.z = position->z;

    if (owner != 0) {
        int owner_index;

        owner_index = owner->blood_owner->owner_index;
        emitter = fx_by_owner(name, 1 << owner_index, 1);
    } else {
        emitter = fx_by_owner(name, 4);
    }
    if (emitter == 0) {
        return;
    }

    emitter = fx_next_emitter();
    if (emitter == 0) {
        return;
    }
    fx_resume_emit();
    pfx = pfx_from_emitter(emitter);
    if (pfx == 0) {
        return;
    }

    emitter_index = emitter_id_from_handle(emitter);
    pfx_bind_emitter_num_to_obj(
        pfx, g_game_info.bgnd_obj, 0, emitter_index);
    emitter_vm = (PfxEmitter*)pfx_get_emitter(
        &pfx->matrix, emitter_index);
    emitter_vm->transform = matrix;
    watcher->matrix_count++;
}

void gusher_destroy_list(void) {
    MkPtr* ptr;

    if (&gusher_list != 0) {
        ptr = gusher_list;
        while (ptr != 0) {
            MkHdr* hdr;

            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                MkPtr* next;

                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
                continue;
            }

            if ((void*)hdr->vtbl == (void*)&vtbl_mkpdata_generic &&
                hdr->instance != 0U) {
                BloodProcVtableRef vtbl;

                vtbl.base = (MkVtableMkproc*)hdr->vtbl;
                vtbl.blood->destroy((MkProc*)hdr);
            }
            ptr = ptr->next;
        }
    }
    gusher_list = 0;
}

void kill_gusher(MkProc* proc) {
    BloodProcVtableRef vtbl;

    if (proc->instance != 0U) {
        vtbl.base = proc->vtbl;
        vtbl.blood->destroy(proc);
    }
}

static float p_foot_print_wait(void) {
    return 60.0f;
}

void spawn_bld_splat(
    const char* name, FighterMirror* owner, const Vec* position) {
    spawn_decal_emitter(name, owner, position, 0, 0.0f);
}

void start_decal_emitter_watcher(void) {
    DecalEmitterWatcherPdata* pdata;
    MkProc* proc;
    int index;

    proc = _create_mkproc_generic_nostack(
        0x601A, 0x30, p_decal_emitter_watcher,
        sizeof(DecalEmitterWatcherPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        zero_pdata_payload(sizeof(DecalEmitterWatcherPdata), &pdata->hdr);
        ncs_pfx_decal_emitter_proc.proc = proc;
        ncs_pfx_decal_emitter_proc.instance = proc->instance;
        for (index = 0; index < 10; index++) {
            MKMatrixSetIdentity(&pdata->matrices[index]);
        }
    }
}

static float p_decal_emitter_watcher(void) {
    MkPfx* pfx;

    pfx = (MkPfx*)apdata;
    if (pfx == 0) {
        return -1.0f;
    }
    pfx->field_294 = 0;
    return 1.0f;
}

void reset_blood_decals(void) {
    int index;

    index = 0;
    while (blood_decal_to_reset[index] != 0) {
        unsigned int effect;

        effect = fx_by_owner(blood_decal_to_reset[index], 1);
        if (effect != 0) {
            fx_reset(effect);
        }
        effect = fx_by_owner(blood_decal_to_reset[index], 2);
        if (effect != 0) {
            fx_reset(effect);
        }
        index++;
    }

    memset(ncs_blood_splat_list, 0, sizeof(ncs_blood_splat_list));
    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
        ncs_blood_splat_list[index].reset_height = -10000.0f;
    }
}

void bleed_startup(void) {
    MkProc* proc;
    int flags;

    flags = 0;
    proc = create_mkproc(
        0x30, get_mkproc_nostack(&flags), 0x5013, p_bleed, 0);
    if (proc != 0) {
        bleed_proc_item.proc = proc;
        bleed_proc_item.instance = proc->instance;
    }

    flags = 0;
    proc = create_mkproc(
        0x2E, get_mkproc_nostack(&flags), 0x5014, p_pfx_bleed, 0);
    if (proc != 0) {
        bleed_pfx_proc_item.proc = proc;
        bleed_pfx_proc_item.instance = proc->instance;
    }

    if (bleed_startup__fire_off_splat_watcher_func != 0) {
        start_blood_splat_watcher();
    }
}

void bleed_init(void) {
    int index;

    bleed_proc_item.proc = 0;
    bleed_proc_item.instance = 0;
    bleed_pfx_proc_item.proc = 0;
    bleed_pfx_proc_item.instance = 0;
    gusher_list = 0;
    bleed_startup__fire_off_splat_watcher_func = 1;

    memset(ncs_blood_splat_list, 0, sizeof(ncs_blood_splat_list));
    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
        ncs_blood_splat_list[index].reset_height = -10000.0f;
    }
}

void bleed_restart(void) {
    MkProc* proc;
    MkProc* foot_proc;
    FootPrintPdata* foot_pdata;
    FighterMirror* fighter;
    MkObj* object;
    int flags;
    int index;

    destroy_mkprocs_pid(0x5013);
    destroy_mkprocs_pid(0x5014);
    destroy_mkprocs_pid(0x5018);
    destroy_mkprocs_pid(0x501B);
    destroy_mkprocs_pid(0x5015);

    proc = bleed_pfx_proc_item.proc;
    if (proc != 0 &&
        (unsigned int)proc->instance != bleed_pfx_proc_item.instance) {
        proc = 0;
    }
    if (proc != 0) {
        destroy_list(&proc->pdata_list);
    }
    bleed_proc_item.proc = 0;
    bleed_proc_item.instance = 0;
    bleed_pfx_proc_item.proc = 0;
    bleed_pfx_proc_item.instance = 0;
    gusher_list = 0;
    bleed_startup__fire_off_splat_watcher_func = 1;

    memset(ncs_blood_splat_list, 0, sizeof(ncs_blood_splat_list));
    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
        ncs_blood_splat_list[index].reset_height = -10000.0f;
    }

    bleed_startup__fire_off_splat_watcher_func = 0;
    flags = 0;
    proc = create_mkproc(
        0x30, get_mkproc_nostack(&flags), 0x5013, p_bleed, 0);
    if (proc != 0) {
        bleed_proc_item.proc = proc;
        bleed_proc_item.instance = proc->instance;
    }
    flags = 0;
    proc = create_mkproc(
        0x2E, get_mkproc_nostack(&flags), 0x5014, p_pfx_bleed, 0);
    if (proc != 0) {
        bleed_pfx_proc_item.proc = proc;
        bleed_pfx_proc_item.instance = proc->instance;
    }
    if (bleed_startup__fire_off_splat_watcher_func != 0) {
        start_blood_splat_watcher();
    }

    fighter = g_game_info.plyr0.slot.fighter;
    object = g_game_info.plyr0.slot.mirror_a;
    if (get_blood_level() < blood_type_list[0]) {
        foot_proc = 0;
    } else {
        obj_set_bone_calc_world_mat_flag(object, 0xB);
        obj_set_bone_calc_world_mat_flag(object, 0xA);
        foot_proc = _create_mkproc_generic_nostack(
            0x5018, 0x2C, p_foot_print_wait,
            sizeof(FootPrintPdata), (MkHdr**)&foot_pdata);
        if (foot_proc != 0) {
            foot_proc->scheduling_flags |= 0x10;
            foot_proc->sleep_ticks = 60.0f;
            foot_pdata->object = object;
            foot_pdata->object_instance = object->hdr.instance;
            foot_pdata->decal_owner = fighter;
            foot_pdata->left_position.x = -1000.0f;
            foot_pdata->left_position.y = -1000.0f;
            foot_pdata->left_position.z = -1000.0f;
            foot_pdata->right_position.x = -1000.0f;
            foot_pdata->right_position.y = -1000.0f;
            foot_pdata->right_position.z = -1000.0f;
            foot_pdata->bone_offset.x = 0.0f;
            foot_pdata->bone_offset.y = 0.0f;
            foot_pdata->bone_offset.z = -0.03f;
            foot_pdata->use_right_foot = 0;
        }
    }
    if (foot_proc != 0) {
        fighter->foot_print_proc = foot_proc;
        fighter->foot_print_proc_instance = foot_proc->instance;
    }

    fighter = g_game_info.plyr1.slot.fighter;
    object = g_game_info.plyr1.slot.mirror_a;
    if (get_blood_level() < blood_type_list[0]) {
        foot_proc = 0;
    } else {
        obj_set_bone_calc_world_mat_flag(object, 0xB);
        obj_set_bone_calc_world_mat_flag(object, 0xA);
        foot_proc = _create_mkproc_generic_nostack(
            0x5018, 0x2C, p_foot_print_wait,
            sizeof(FootPrintPdata), (MkHdr**)&foot_pdata);
        if (foot_proc != 0) {
            foot_proc->scheduling_flags |= 0x10;
            foot_proc->sleep_ticks = 60.0f;
            foot_pdata->object = object;
            foot_pdata->object_instance = object->hdr.instance;
            foot_pdata->decal_owner = fighter;
            foot_pdata->left_position.x = -1000.0f;
            foot_pdata->left_position.y = -1000.0f;
            foot_pdata->left_position.z = -1000.0f;
            foot_pdata->right_position.x = -1000.0f;
            foot_pdata->right_position.y = -1000.0f;
            foot_pdata->right_position.z = -1000.0f;
            foot_pdata->bone_offset.x = 0.0f;
            foot_pdata->bone_offset.y = 0.0f;
            foot_pdata->bone_offset.z = -0.03f;
            foot_pdata->use_right_foot = 0;
        }
    }
    if (foot_proc != 0) {
        fighter->foot_print_proc = foot_proc;
        fighter->foot_print_proc_instance = foot_proc->instance;
    }
}

static float p_pfx_bleed(void) {
    apply_to_mklist(do_pfx_bleed, &aproc->pdata_list);
    return 1.0f;
}

static inline int blood_bone_is_compatible(int requested, int candidate) {
    switch (requested) {
    case 0:
    case 3:
        return candidate >= 0 && candidate < 3;
    case 6:
    case 9:
    case 13:
        switch (candidate) {
        case 3:
        case 6:
        case 12:
        case 14:
        case 15:
        case 17:
        case 18:
        case 19:
            return 1;
        default:
            return 0;
        }
    case 16:
        return candidate == 9 || candidate == 13;
    case 15:
    case 18:
        return candidate == 18 || candidate == 20;
    case 17:
    case 19:
        return candidate == 19 || candidate == 21;
    case 20:
    case 22:
    case 24:
        return candidate == 22 || candidate == 24;
    case 21:
    case 23:
    case 25:
        return candidate == 23 || candidate == 25;
    default:
        return 0;
    }
}

static float p_bleed(void) {
    BloodProcVtableRef vtbl;
    BloodSpawnTarget* target;
    BleedPdata* pdata;
    BloodSpawnStep* step;
    MkPtr** list;
    MkPtr* item;
    MkPtr* next;
    MkObj* object;
    int candidate;
    int handled;
    int index;

    list = &aproc->pdata_list;
    if (list != 0) {
        item = *list;
        while (item != 0) {
            pdata = (BleedPdata*)item->hdr;
            if (item->instance != pdata->hdr.instance) {
                next = item->next;
                item->hdr = 0;
                destroy_mkptr(item);
                item = next;
                continue;
            }
            if (pdata == 0) {
                item = item->next;
                continue;
            }

            if (--pdata->timer <= 0) {
                object = pdata->object;
                if (object != 0) {
                    if (object->hdr.instance == pdata->object_instance) {
                        /* Keep the live object. */
                    } else {
                        object = 0;
                    }
                } else {
                    object = 0;
                }

                if (object != 0 && !object->hide_flag_bits.hidden) {
                    step = pdata->step;
                    target = &pdata->spawn_state->targets[
                        step->target_index];
                    if (target != 0) {
                        handled = 0;
                        for (index = 0; index < target->bone_count; index++) {
                            candidate = pdata->spawn_state->bone_map[
                                target->bone_indices[index]].bone;
                            if (candidate == pdata->bone ||
                                blood_bone_is_compatible(
                                    pdata->bone, candidate)) {
                                obj_spawn_bld(
                                    object, 0, step->blood_type,
                                    (const int*)step, target, index, 0,
                                    pdata->art_id, pdata->owner);
                                if (step->delay < 0) {
                                    if (pdata->hdr.instance != 0) {
                                        vtbl.base = (MkVtableMkproc*)
                                            pdata->hdr.vtbl;
                                        vtbl.blood->destroy((MkProc*)pdata);
                                    }
                                } else {
                                    pdata->timer = step->delay;
                                    pdata->step++;
                                }
                                handled = 1;
                                break;
                            }
                        }
                        if (handled) {
                            item = item->next;
                            continue;
                        }
                    }
                }

                if (pdata->hdr.instance != 0) {
                    vtbl.base = (MkVtableMkproc*)pdata->hdr.vtbl;
                    vtbl.blood->destroy((MkProc*)pdata);
                }
            }
            item = item->next;
        }
    }
    return 1.0f;
}

static int obj_set_bld_vel(
    MkObj* object, const Vec* position, BloodVelocityState* state) {
    BloodSurfaceRecord* record;
    const Vec* current;
    const Vec* next;
    RwMatrix* matrix;
    BloodPath* path;
    Vec direction;
    float current_weight;
    float next_weight;
    float inverse_distance;
    float speed;
    float projection;
    float duration;
    int corner;
    int result;

    path = state->path;
    result = 1;
    if (state->point_index >= path->point_count) {
        result = 0;
        state->velocity.x = 0.0f;
        state->velocity.y = 0.0f;
        state->velocity.z = 0.0f;
        state->travel_ticks = 0.0f;
    } else {
        record = &path->surface->records[
            path->record_indices[state->point_index]];
        corner = path->corner_indices[state->point_index];
        current = &record->points[corner];
        corner++;
        if (corner >= 3) {
            corner = 0;
        }
        next = &record->points[corner];

        current_weight = state->weight_0 + path->interpolation_bias;
        next_weight = state->weight_1 + path->interpolation_bias;
        inverse_distance = 1.0f / (current_weight + next_weight);
        current_weight *= inverse_distance;
        next_weight *= inverse_distance;
        direction.x = current->x * current_weight + next->x * next_weight;
        direction.y = current->y * current_weight + next->y * next_weight;
        direction.z = current->z * current_weight + next->z * next_weight;
        direction.x -= position->x;
        direction.y -= position->y;
        direction.z -= position->z;

        duration = length_v3(&direction);
        inverse_distance = 1.0f / duration;
        speed = path->speed_base + state->weight_step * path->speed_scale;

        calc_bone_world_mat(object, record->bone);
        matrix = &object->bones[record->bone]->matrix;
        projection = direction.x * matrix->right.y +
            direction.y * matrix->up.y +
            direction.z * matrix->at.y;
        speed *= 1.0f - projection * inverse_distance;
        if (speed < 0.001f) {
            speed = 0.001f;
        }
        if (projection > 0.0f) {
            speed *= 0.35f;
            if (projection * inverse_distance > 2.0f * state->weight_0) {
                result = 0;
            }
        }
        if (matrix->pos.y + position->x * matrix->right.y +
                position->y * matrix->up.y +
                position->z * matrix->at.y <
            object->ground_colls_y + 0.2f) {
            result = 0;
            state->travel_ticks = 0.0f;
        }

        speed *= inverse_distance;
        state->velocity.x = direction.x * speed;
        state->velocity.y = direction.y * speed;
        state->velocity.z = direction.z * speed;
        state->travel_ticks = duration / (speed / inverse_distance);
        if (state->travel_ticks > 30.0f) {
            state->travel_ticks = 30.0f;
        }

        state->weight_0 += state->weight_1;
        if (state->weight_0 >= 1.0f) {
            state->weight_0 -= 1.0f;
        }
        state->weight_1 += state->weight_step;
        if (state->weight_1 >= 1.0f) {
            state->weight_1 -= 1.0f;
        }
        state->weight_step += state->weight_0;
        if (state->weight_step >= 1.0f) {
            state->weight_step -= 1.0f;
        }
    }
    return result;
}

static void bloodfx_init(BloodFxUserdata* userdata) {
    userdata->flags_40_bits.bit4 = 1;
    userdata->flags_40_bits.bit3 = 1;
}
