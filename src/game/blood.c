#include "runtime/mk_obj.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "runtime/asset.h"
#include "runtime/section.h"
#include "game/pfxscript.h"
#include "game/game_info.h"
#include "libmkparticle/color.h"
#include "math/mk_math.h"
#include "math/gxMath.h"

#define BLOOD_SPLAT_COUNT 24

typedef struct BloodSplat {
    Vec position;
    unsigned int expiry_tick;
    int splat_count;
    int reuse_count;
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
    const char* blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

typedef struct GusherPdata {
    MkHdr hdr;
    GusherStep* steps;
    FighterMirror* owner;
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
    union {
        unsigned char flags;
        struct {
            unsigned char create_decal : 1;
            unsigned char pad_flags : 7;
        };
    };
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

typedef struct BloodSurface BloodSurface;

typedef struct BloodParticleDefinition {
    int field_00;
    float spawn_interval;
    float size;
    float field_0C;
    float red;
    float green;
    float blue;
    float alpha;
    int disable_ground_splat;
} BloodParticleDefinition; /* 0x24 */

typedef struct BloodSpawnTarget {
    BloodSurface* surface;
    int point_count;
    const int* record_indices;
    int* corner_indices;
    float interpolation_bias;
    float speed_base;
    float speed_scale;
} BloodSpawnTarget; /* 0x1C */

typedef struct BloodSpawnState {
    char pad00[0x10];
    BloodBoneMapEntry* bone_map;
    BloodSpawnTarget targets[1];
} BloodSpawnState;

typedef struct BloodSpawnStep {
    int target_index;
    BloodParticleDefinition* definition;
    int blood_type;
    int spawn_count;
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
    Vec normal;
    float plane_distance;
    int bone;
    int vertex_indices[3];
    Vec points[3];
    int neighbors[3];
    struct {
        Vec normal;
        float plane_distance;
    } edges[3];
} BloodSurfaceRecord; /* 0x80 */

typedef struct BloodSurfaceVertex {
    int tag;
    Vec position;
} BloodSurfaceVertex; /* 0x10 */

struct BloodSurface {
    void* field_00;
    BloodSurfaceVertex* vertices;
    int record_count;
    int (*triangles)[3];
    BloodSurfaceRecord* records;
}; /* 0x14 */

typedef struct BloodPath {
    BloodSurface* surface;
    int point_count;
    const int* record_indices;
    int* corner_indices;
    float interpolation_bias;
    float speed_base;
    float speed_scale;
} BloodPath; /* 0x1C */

typedef struct BloodModelData {
    BloodSurface surface;
    BloodPath paths[10];
} BloodModelData; /* 0x12C */

typedef struct BloodPathFile {
    BloodSurface surface;
    int relocation_marker;
    BloodPath* paths[10];
} BloodPathFile;

typedef struct BloodDefaultBlob {
    char pad0000[0x2B14];
    BloodSurface surface;
    char pad2B28[0x50];
    BloodPath paths_0;
    char pad2B94[0x90];
    BloodPath paths_1;
    char pad2C40[0x8C];
    BloodPath paths_2;
    char pad2CE8[0x78];
    BloodPath paths_3;
    char pad2D7C[0x30];
    BloodPath paths_4;
    char pad2DC8[0x50];
    BloodPath paths_5;
    char pad2E34[0x90];
    BloodPath paths_6;
    char pad2EE0[0x8C];
    BloodPath paths_7;
    char pad2F88[0x78];
    BloodPath paths_8;
    char pad301C[0x30];
    BloodPath paths_9;
} BloodDefaultBlob; /* 0x3068 */

typedef struct BloodVelocityState {
    Vec velocity;
    float spawn_delay;
    float travel_ticks;
    BloodSpawnStep* step;
    BloodPath* path;
    int point_index;
    unsigned char flags;
    char pad21[3];
    int path_point;
    float weight_0;
    float weight_1;
    float weight_step;
} BloodVelocityState; /* 0x34 */

typedef struct BloodPfxVmView {
    char pad00[0x50];
    int particle_capacity;
    int particle_count;
    char pad58[0x5C];
    int position_stride; /* +0xB4 */
} BloodPfxVmView;

typedef struct BloodParticlePosition {
    Vec position;
    float u;
    float v;
} BloodParticlePosition;

typedef struct BloodPfxConfigView {
    char pad000[0x190];
    unsigned char flags_190;
    char pad191[0x1F];
    float field_1B0;
    char pad1B4[0x0C];
    unsigned short field_1C0;
    char pad1C2[2];
    float field_1C4;
    float field_1C8;
    float field_1CC;
    char pad1D0[0x24];
    PfxColor color_1F4;
    float field_1F8;
} BloodPfxConfigView;

/* Contiguous authored data block used by the player blood scripts. */
typedef struct BloodAssetData {
    BloodDefaultBlob default_blob;
    int blood_levels[12];               /* +0x3068 */
    char* blood_map[11];                /* +0x3098 */
    unsigned char script_prefix[0x48];
    Vec path_weights;                    /* +0x310C */
    char pad3118[0x0C];
    BloodSpawnStep medium_scripts[10][2]; /* +0x3124, stride 0x28 */
    BloodSpawnStep bleed_scripts[10][4]; /* +0x32B4, stride 0x50 */
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
extern MkVtable5 vtbl_pfx;
extern float game_speed;
extern int exec_tick_ctr;
extern BloodDecalArrayView mkpfx_ncs_decal_array;
extern BloodAssetData scorpion_sweat_bld_src_verts;

void* memset(void* destination, int value, unsigned long size);
unsigned int fx_by_owner(const char* name, int owner);
void spawn_decal_emitter(
    const char* name, FighterMirror* owner, const Vec* position,
    const MKMATRIX* orientation,
    float angle);
void start_blood_splat_watcher(void);
void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
void calc_bone_world_mat(MkObj* object, int bone);
void spawn_bld_fall(
    const char* blood_type, MkBone* bone, const Vec* position,
    const Vec* velocity, FighterMirror* owner);
void plyr_bleed_small_cycle_ext(
    PlyrPdata* pdata, int bone, PlyrPdata* owner);
void plyr_bleed_large_ext(PlyrPdata* pdata, int bone, PlyrPdata* owner);
void plyr_bleed_medium_cycle(PlyrPdata* pdata, int bone);
void plyr_obj_load_bld_data(
    FighterMirror* fighter, BloodModelData* model, MkObj* object,
    char* path_name);
void obj_set_bone_calc_world_mat_flag(MkObj* object, int bone);
unsigned int fx_next_emitter(unsigned int emitter);
void fx_resume_emit(unsigned int emitter);
MkPfx* pfx_from_emitter(unsigned int emitter);
int emitter_id_from_handle(unsigned int emitter);
float gxMathArcTanYX(float y, float x);
int strcmp(const char* left, const char* right);
int obj_get_bid_for_tid(MkObj* object, int tag);
PfxEmitter* pfx_get_emitter(void* pfx_vm, int emitter_index);
void* pfx_get_field(void* pfx_vm, int emitter_index, int field);
int pfx_get_struct_size(void* pfx_vm, int field);
void update_live_particles(void* pfx_vm);
RwMatrix* RwMatrixInvert(RwMatrix* output, const RwMatrix* input);
int obj_spawn_bld(
    MkObj* object, BloodVelocityState* previous, int batch_count,
    BloodSpawnStep* step, BloodSpawnTarget* path, int point_index,
    const Vec* position, unsigned int art_id,
    PlyrPdata* owner);

static float p_decal_emitter_watcher(void);
static float p_gusher(void);
static float p_watch_bleed_obj_for_gnd_coll(void);
static float p_foot_print(void);
static float p_foot_print_wait(void);
static float p_bleed(void);
static float p_pfx_bleed(void);
static void do_pfx_bleed(MkHdr* hdr);
static int obj_set_bld_vel(
    MkObj* object, const Vec* position, BloodVelocityState* state);
static void obj_bld_surface_build_polys(
    MkObj* object, BloodSurface* output, const BloodSurface* source);
static void bloodfx_init(BloodFxUserdata* userdata);

static inline void blood_interpolate_direction(
    Vec* direction, const Vec* current, const Vec* next,
    float current_weight, float next_weight, const Vec* position) {
    direction->x =
        current->x * current_weight + next->x * next_weight;
    direction->y =
        current->y * current_weight + next->y * next_weight;
    direction->z =
        current->z * current_weight + next->z * next_weight;
    direction->x -= position->x;
    direction->y -= position->y;
    direction->z -= position->z;
}

static inline void queue_blood_spawn(
    MkObj* object, BloodSpawnStep* step, BloodSpawnState* spawn_state,
    int bone, unsigned int art_id, PlyrPdata* owner, int timer) {
    MkProc* proc;
    BleedPdata* pdata;

    proc = bleed_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == bleed_proc_item.instance) {
            /* The process instance latch is still valid. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (BleedPdata*)get_mkpdata_generic(sizeof(*pdata));
        if (pdata != 0) {
            pdata->object = object;
            pdata->object_instance = object->hdr.instance;
            pdata->step = step;
            pdata->spawn_state = spawn_state;
            pdata->timer = timer;
            pdata->bone = bone;
            pdata->art_id = art_id;
            pdata->owner = owner;
            mk_insert(&pdata->hdr, &proc->pdata_list);
        }
    }
}

static inline float blood_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;

    if (!(value > 0.0f)) {
        return 0.0f;
    }
    input.f = value;
    guess.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    guess.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    return 0.5f * guess.f *
        (3.0f - (guess.f * guess.f) / value);
}

static inline void prepare_blood_path(
    BloodModelData* model, BloodPath* destination,
    const BloodPath* source, const Vec* weights) {
    BloodSurfaceRecord* record;
    int triangle_index;
    int next_triangle;
    int point_index;
    int corner;

    memcpy(destination, source, sizeof(*destination));
    destination->interpolation_bias = weights->x;
    destination->speed_base = weights->y;
    destination->speed_scale = weights->z;
    destination->surface = &model->surface;

    for (point_index = 0; point_index < destination->point_count;
         point_index++) {
        if (point_index + 1 == destination->point_count) {
            destination->corner_indices[point_index] = -1;
            continue;
        }
        triangle_index = destination->record_indices[point_index];
        if (triangle_index >= model->surface.record_count) {
            destination->corner_indices[point_index] = -1;
            continue;
        }

        next_triangle = destination->record_indices[point_index + 1];
        record = &model->surface.records[triangle_index];
        destination->corner_indices[point_index] = -1;
        for (corner = 0; corner < 3; corner++) {
            if (record->neighbors[corner] == next_triangle) {
                destination->corner_indices[point_index] = corner;
                break;
            }
        }
    }
}

int is_blood_disabled(void) {
    return get_blood_level() < 2;
}

void plyr_obj_load_bld_data(
    FighterMirror* fighter, BloodModelData* model, MkObj* object,
    char* path_name) {
    BloodAssetData* assets;
    BloodPathFile* file;
    BloodSurface* source_surface;
    BloodPath* source_paths[10];
    FootPrintPdata* foot_pdata;
    MkProc* foot_proc;
    int relocate;
    int art_section;
    int index;

    assets = &scorpion_sweat_bld_src_verts;
    if (get_blood_level() < assets->blood_levels[0]) {
        foot_proc = 0;
    } else {
        obj_set_bone_calc_world_mat_flag(object, 0xB);
        obj_set_bone_calc_world_mat_flag(object, 0xA);
        foot_proc = _create_mkproc_generic_nostack(
            0x5018, 0x2C, p_foot_print_wait, sizeof(*foot_pdata),
            (MkHdr**)&foot_pdata);
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

    if (path_name != 0) {
        art_section = get_shared_art_section_for_player(
            (SharedArtPlayer*)object);
        file = (BloodPathFile*)load_named_bloodpath_data_from_slot(
            art_section, path_name);
        relocate = 1;
        if (file->relocation_marker < 0) {
            relocate = 0;
        } else {
            file->relocation_marker = -file->relocation_marker;
        }
        source_surface = &file->surface;
        if (relocate) {
            source_surface->vertices = (BloodSurfaceVertex*)(
                (char*)file + (unsigned int)source_surface->vertices);
            source_surface->triangles = (int(*)[3])(
                (char*)file + (unsigned int)source_surface->triangles);
        }
        for (index = 0; index < 10; index++) {
            source_paths[index] = (BloodPath*)(
                (char*)file + (unsigned int)file->paths[index]);
            if (relocate) {
                source_paths[index]->record_indices = (const int*)(
                    (char*)file +
                    (unsigned int)source_paths[index]->record_indices);
                source_paths[index]->corner_indices = (int*)(
                    (char*)file +
                    (unsigned int)source_paths[index]->corner_indices);
            }
        }
    } else {
        source_surface = &assets->default_blob.surface;
        source_paths[0] = &assets->default_blob.paths_0;
        source_paths[1] = &assets->default_blob.paths_1;
        source_paths[2] = &assets->default_blob.paths_2;
        source_paths[3] = &assets->default_blob.paths_3;
        source_paths[4] = &assets->default_blob.paths_4;
        source_paths[5] = &assets->default_blob.paths_5;
        source_paths[6] = &assets->default_blob.paths_6;
        source_paths[7] = &assets->default_blob.paths_7;
        source_paths[8] = &assets->default_blob.paths_8;
        source_paths[9] = &assets->default_blob.paths_9;
    }

    obj_bld_surface_build_polys(object, &model->surface, source_surface);
    prepare_blood_path(
        model, &model->paths[0], source_paths[0], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[1], source_paths[1], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[2], source_paths[2], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[3], source_paths[3], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[4], source_paths[4], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[5], source_paths[5], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[6], source_paths[6], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[7], source_paths[7], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[8], source_paths[8], &assets->path_weights);
    prepare_blood_path(
        model, &model->paths[9], source_paths[9], &assets->path_weights);
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
                object, 0, 2, assets->bleed_scripts[1],
                (BloodSpawnTarget*)pdata->left_blood_spawn_state,
                9, 0, blood_art_id, pdata);
            obj_spawn_bld(
                object, 0, 2, assets->bleed_scripts[7],
                (BloodSpawnTarget*)pdata->right_blood_spawn_state,
                9, 0, blood_art_id, pdata);
        }
    }
}

void plyr_bleed_large_ext(
    PlyrPdata* pdata, int bone, PlyrPdata* owner) {
    BloodAssetData* assets;
    BloodSpawnState* spawn_state;
    MkObj* object;
    int art_section;
    unsigned int blood_art_id;

    assets = &scorpion_sweat_bld_src_verts;
    if (get_blood_level() >= assets->blood_levels[3] &&
        pdata->blood_model_data != 0) {
        object = pdata->tracked_obj;
        if (object != 0) {
            if (object->hdr.instance == pdata->tracked_obj_instance) {
                /* The object instance latch is still valid. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0 && pdata->next_large_bleed_tick <
                (unsigned int)exec_tick_ctr) {
            pdata->next_large_bleed_tick = exec_tick_ctr + 45;
            art_section = get_shared_art_section_for_plyr_pdata(owner);
            blood_art_id = get_artid_of_named_item_in_slot(
                art_section, assets->blood_map[4], 1);
            spawn_state = (BloodSpawnState*)pdata->large_blood_spawn_state;

            queue_blood_spawn(
                object, assets->bleed_scripts[0], spawn_state,
                bone, blood_art_id, owner, 1);
            queue_blood_spawn(
                object, assets->bleed_scripts[1], spawn_state,
                bone, blood_art_id, owner, 3);
            queue_blood_spawn(
                object, assets->bleed_scripts[2], spawn_state,
                bone, blood_art_id, owner, 5);
            queue_blood_spawn(
                object, assets->bleed_scripts[3], spawn_state,
                bone, blood_art_id, owner, 7);
            queue_blood_spawn(
                object, assets->bleed_scripts[4], spawn_state,
                bone, blood_art_id, owner, 9);
            queue_blood_spawn(
                object, assets->bleed_scripts[5], spawn_state,
                bone, blood_art_id, owner, 11);
            queue_blood_spawn(
                object, assets->bleed_scripts[6], spawn_state,
                bone, blood_art_id, owner, 13);
            queue_blood_spawn(
                object, assets->bleed_scripts[7], spawn_state,
                bone, blood_art_id, owner, 15);
            queue_blood_spawn(
                object, assets->bleed_scripts[8], spawn_state,
                bone, blood_art_id, owner, 17);
            queue_blood_spawn(
                object, assets->bleed_scripts[9], spawn_state,
                bone, blood_art_id, owner, 19);
        }
    }
}

void plyr_bleed_medium_cycle(PlyrPdata* pdata, int bone) {
    static int cycle_index;
    BloodAssetData* assets;
    BloodSpawnState* spawn_state;
    MkObj* object;
    int art_section;
    unsigned int blood_art_id;

    assets = &scorpion_sweat_bld_src_verts;
    if (get_blood_level() >= assets->blood_levels[3] &&
        pdata->blood_model_data != 0) {
        object = pdata->tracked_obj;
        if (object != 0) {
            if (object->hdr.instance == pdata->tracked_obj_instance) {
                /* The object instance latch is still valid. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0 && pdata->next_large_bleed_tick <
                (unsigned int)exec_tick_ctr) {
            pdata->next_large_bleed_tick = exec_tick_ctr + 45;
            art_section = get_shared_art_section_for_plyr_pdata(pdata);
            blood_art_id = get_artid_of_named_item_in_slot(
                art_section, assets->blood_map[4], 1);
            spawn_state = (BloodSpawnState*)pdata->large_blood_spawn_state;

            switch (cycle_index) {
            case 0:
                queue_blood_spawn(object, assets->medium_scripts[0],
                    spawn_state, bone, blood_art_id, pdata, 1);
                queue_blood_spawn(object, assets->bleed_scripts[1],
                    spawn_state, bone, blood_art_id, pdata, 3);
                queue_blood_spawn(object, assets->medium_scripts[2],
                    spawn_state, bone, blood_art_id, pdata, 5);
                queue_blood_spawn(object, assets->medium_scripts[4],
                    spawn_state, bone, blood_art_id, pdata, 7);
                queue_blood_spawn(object, assets->medium_scripts[5],
                    spawn_state, bone, blood_art_id, pdata, 9);
                queue_blood_spawn(object, assets->medium_scripts[7],
                    spawn_state, bone, blood_art_id, pdata, 11);
                queue_blood_spawn(object, assets->medium_scripts[8],
                    spawn_state, bone, blood_art_id, pdata, 13);
                break;
            case 1:
                queue_blood_spawn(object, assets->medium_scripts[0],
                    spawn_state, bone, blood_art_id, pdata, 1);
                queue_blood_spawn(object, assets->medium_scripts[2],
                    spawn_state, bone, blood_art_id, pdata, 3);
                queue_blood_spawn(object, assets->medium_scripts[3],
                    spawn_state, bone, blood_art_id, pdata, 5);
                queue_blood_spawn(object, assets->medium_scripts[5],
                    spawn_state, bone, blood_art_id, pdata, 7);
                queue_blood_spawn(object, assets->bleed_scripts[7],
                    spawn_state, bone, blood_art_id, pdata, 9);
                queue_blood_spawn(object, assets->medium_scripts[8],
                    spawn_state, bone, blood_art_id, pdata, 11);
                queue_blood_spawn(object, assets->medium_scripts[9],
                    spawn_state, bone, blood_art_id, pdata, 13);
                break;
            case 2:
                queue_blood_spawn(object, assets->bleed_scripts[0],
                    spawn_state, bone, blood_art_id, pdata, 1);
                queue_blood_spawn(object, assets->medium_scripts[1],
                    spawn_state, bone, blood_art_id, pdata, 3);
                queue_blood_spawn(object, assets->medium_scripts[3],
                    spawn_state, bone, blood_art_id, pdata, 5);
                queue_blood_spawn(object, assets->medium_scripts[4],
                    spawn_state, bone, blood_art_id, pdata, 7);
                queue_blood_spawn(object, assets->medium_scripts[6],
                    spawn_state, bone, blood_art_id, pdata, 9);
                queue_blood_spawn(object, assets->medium_scripts[7],
                    spawn_state, bone, blood_art_id, pdata, 11);
                queue_blood_spawn(object, assets->medium_scripts[9],
                    spawn_state, bone, blood_art_id, pdata, 13);
                break;
            }
            cycle_index++;
            if (cycle_index > 2) {
                cycle_index = 0;
            }
        }
    }
}

void plyr_bleed_small_cycle_ext(
    PlyrPdata* pdata, int bone, PlyrPdata* owner) {
    static int cycle_index;
    BloodAssetData* assets;
    BloodSpawnState* spawn_state;
    MkObj* object;
    int art_section;
    unsigned int blood_art_id;

    assets = &scorpion_sweat_bld_src_verts;
    if (get_blood_level() >= assets->blood_levels[3] &&
        pdata->blood_model_data != 0) {
        object = pdata->tracked_obj;
        if (object != 0) {
            if (object->hdr.instance == pdata->tracked_obj_instance) {
                /* The object instance latch is still valid. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0 && pdata->next_large_bleed_tick <
                (unsigned int)exec_tick_ctr) {
            pdata->next_large_bleed_tick = exec_tick_ctr + 45;
            art_section = get_shared_art_section_for_plyr_pdata(owner);
            blood_art_id = get_artid_of_named_item_in_slot(
                art_section, assets->blood_map[4], 1);
            spawn_state = (BloodSpawnState*)pdata->large_blood_spawn_state;

            switch (cycle_index) {
            case 0:
                queue_blood_spawn(object, assets->medium_scripts[0],
                    spawn_state, bone, blood_art_id, owner, 1);
                queue_blood_spawn(object, assets->medium_scripts[3],
                    spawn_state, bone, blood_art_id, owner, 3);
                queue_blood_spawn(object, assets->medium_scripts[6],
                    spawn_state, bone, blood_art_id, owner, 5);
                queue_blood_spawn(object, assets->medium_scripts[9],
                    spawn_state, bone, blood_art_id, owner, 7);
                break;
            case 1:
                queue_blood_spawn(object, assets->medium_scripts[1],
                    spawn_state, bone, blood_art_id, owner, 1);
                queue_blood_spawn(object, assets->medium_scripts[6],
                    spawn_state, bone, blood_art_id, owner, 3);
                queue_blood_spawn(object, assets->medium_scripts[4],
                    spawn_state, bone, blood_art_id, owner, 5);
                queue_blood_spawn(object, assets->medium_scripts[7],
                    spawn_state, bone, blood_art_id, owner, 7);
                break;
            case 2:
                queue_blood_spawn(object, assets->medium_scripts[0],
                    spawn_state, bone, blood_art_id, owner, 1);
                queue_blood_spawn(object, assets->medium_scripts[2],
                    spawn_state, bone, blood_art_id, owner, 3);
                queue_blood_spawn(object, assets->medium_scripts[5],
                    spawn_state, bone, blood_art_id, owner, 5);
                queue_blood_spawn(object, assets->medium_scripts[8],
                    spawn_state, bone, blood_art_id, owner, 7);
                break;
            }
            cycle_index++;
            if (cycle_index > 2) {
                cycle_index = 0;
            }
        }
    }
}

GusherPdata* start_gusher(
    GusherStep* steps, FighterMirror* owner, MkObj* object, int bone,
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
            (PlyrPdata*)pdata->owner, pdata->bone,
            (PlyrPdata*)pdata->owner);
    }
    return time;
}

void spawn_bld_fall(
    const char* blood_type, MkBone* bone, const Vec* position,
    const Vec* velocity, FighterMirror* owner) {
    BleedGroundWatcherPdata* watcher;
    BloodSplat* splat;
    MkObj* object;
    MkPfx* pfx;
    unsigned int effect;
    unsigned int oldest_age;
    int oldest_index;
    int nearby_index;
    int expired_nearby_count;
    int splat_limit;
    float nearby_radius;
    int index;

    watcher = 0;
    object = 0;
    effect = fx_by_owner(
        blood_type, 1 << owner->blood_owner->owner_index);
    if (effect != 0) {
        pfx = pfx_from_emitter(effect);
        if (pfx != 0 &&
            _create_mkproc_generic_nostack(
                0x601B, 0x1F, p_watch_bleed_obj_for_gnd_coll,
                sizeof(BleedGroundWatcherPdata),
                (MkHdr**)&watcher) != 0) {
            zero_pdata_payload(
                sizeof(BleedGroundWatcherPdata), &watcher->hdr);
            effect = fx_next_emitter(effect);
            if (effect != 0) {
                object = (MkObj*)get_mkobj_frame((void*)0x6015, 0);
                if (object != 0) {
                    fx_resume_emit(effect);
                    pfx_bind_emitter_num_to_obj(
                        pfx, object, 0,
                        emitter_id_from_handle(effect));
                    insert_particle_mkobj(object);
                    object->flags_08_bits.airborne = 1;
                    object->flags_08_bits.gravity_enabled = 1;

                    if (bone != 0) {
                        object->pos.x = bone->matrix.pos.x +
                            position->x * bone->matrix.right.x +
                            position->y * bone->matrix.up.x +
                            position->z * bone->matrix.at.x;
                        object->pos.y = bone->matrix.pos.y +
                            position->x * bone->matrix.right.y +
                            position->y * bone->matrix.up.y +
                            position->z * bone->matrix.at.y;
                        object->pos.z = bone->matrix.pos.z +
                            position->x * bone->matrix.right.z +
                            position->y * bone->matrix.up.z +
                            position->z * bone->matrix.at.z;
                    } else {
                        object->pos.x = position->x;
                        object->pos.y = position->y;
                        object->pos.z = position->z;
                    }

                    object->pos_vel.x =
                        0.7f * velocity->x + sfrand(0.004f);
                    object->pos_vel.z =
                        0.7f * velocity->z + sfrand(0.004f);
                    object->pos_vel.y =
                        0.5f * velocity->y + sfrand(0.002f);
                    update_mkobj(object);

                    oldest_age = 0;
                    oldest_index = -1;
                    nearby_index = -1;
                    expired_nearby_count = 0;
                    if (strcmp("bleedfall", blood_type) == 0) {
                        nearby_radius = 1.8f;
                        splat_limit = 3;
                    } else {
                        nearby_radius = 0.9f;
                        splat_limit = 6;
                    }

                    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
                        float x;
                        float y;
                        float z;

                        x = ncs_blood_splat_list[index].position.x -
                            object->pos.x;
                        y = ncs_blood_splat_list[index].position.y -
                            object->pos.y;
                        z = ncs_blood_splat_list[index].position.z -
                            object->pos.z;
                        if (blood_sqrt(x * x + y * y + z * z) <
                            nearby_radius) {
                            nearby_index = index;
                        }
                        if (ncs_blood_splat_list[index].expiry_tick <
                            (unsigned int)exec_tick_ctr) {
                            unsigned int age;

                            if (nearby_index == index) {
                                nearby_index = -1;
                                expired_nearby_count++;
                            }
                            age = (unsigned int)exec_tick_ctr -
                                ncs_blood_splat_list[index].expiry_tick;
                            if (oldest_age < age) {
                                oldest_age = age;
                                oldest_index = index;
                            }
                        }
                    }

                    if (nearby_index < 0 && oldest_index >= 0 &&
                        expired_nearby_count < BLOOD_SPLAT_COUNT) {
                        splat = &ncs_blood_splat_list[oldest_index];
                        splat->reuse_count = 0;
                        splat->splat_count = 0;
                        splat->position.x = object->pos.x;
                        splat->position.y = object->pos.y;
                        splat->position.z = object->pos.z;
                        splat->expiry_tick =
                            (unsigned int)exec_tick_ctr + 180;
                        splat->splat_count++;
                        watcher->create_decal = 1;
                    } else {
                        splat = &ncs_blood_splat_list[nearby_index];
                        if (splat->splat_count < splat_limit) {
                            splat->splat_count++;
                            watcher->create_decal = 1;
                        } else if (splat->expiry_tick <
                            (unsigned int)exec_tick_ctr) {
                            splat->reuse_count++;
                            if (splat->reuse_count < 3) {
                                splat->splat_count = 0;
                                splat->position.x = object->pos.x;
                                splat->position.y = object->pos.y;
                                splat->position.z = object->pos.z;
                                splat->expiry_tick =
                                    (unsigned int)exec_tick_ctr + 180;
                                splat->splat_count++;
                                watcher->create_decal = 1;
                            }
                        }
                    }
                    watcher->blood_object = object;
                    watcher->blood_object_instance = object->hdr.instance;
                    watcher->effect_handle = effect;
                    watcher->decal_owner = owner;
                    return;
                }
            }
        }
    }

    if (watcher != 0) {
        if (object != 0 && object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy((MkHdr*)object);
        }
        if (effect != 0) {
            fx_reset_emit(effect);
        }
        if (watcher->hdr.instance != 0) {
            watcher->hdr.typed_vtbl->destroy(&watcher->hdr);
        }
    }
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
        emitter = fx_by_owner(name, 1 << owner_index);
    } else {
        emitter = fx_by_owner(name, 4);
    }
    if (emitter == 0) {
        return;
    }

    emitter = fx_next_emitter(emitter);
    if (emitter == 0) {
        return;
    }
    fx_resume_emit(emitter);
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

static inline MkHdr* blood_as_generic_pdata(MkHdr* hdr) {
    if ((void*)hdr->vtbl == (void*)&vtbl_mkpdata_generic) {
        return hdr;
    }
    return 0;
}

void gusher_destroy_list(void) {
    MkPtr* next;
    MkPtr* ptr;

    if (&gusher_list != 0) {
        ptr = gusher_list;
        while (ptr != 0) {
            MkHdr* hdr;

            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
                continue;
            }

            hdr = blood_as_generic_pdata(hdr);
            if (hdr != 0 && hdr->instance != 0U) {
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
        ncs_blood_splat_list[index].position.y = -10000.0f;
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
        ncs_blood_splat_list[index].position.y = -10000.0f;
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
        ncs_blood_splat_list[index].position.y = -10000.0f;
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
                        /* The object instance latch is still valid. */
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
                        for (index = 0; index < target->point_count; index++) {
                            candidate = pdata->spawn_state->bone_map[
                                target->record_indices[index]].bone;
                            if (candidate == pdata->bone ||
                                blood_bone_is_compatible(
                                    pdata->bone, candidate)) {
                                obj_spawn_bld(
                                    object, 0, step->blood_type,
                                    step, target, index, 0,
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

static void do_pfx_bleed(MkHdr* hdr) {
    BloodParticlePosition* destination;
    BloodParticlePosition* source;
    BloodVelocityState* states;
    BloodVelocityState* state;
    BloodPfxVmView* vm;
    BloodSurfaceRecord* record;
    BloodSurfaceRecord* previous_record;
    BloodPath* path;
    FighterMirror* owner;
    MkProc* foot_proc;
    MkBone* old_bone;
    MkBone* new_bone;
    MkPfx* pfx;
    RwMatrix inverse;
    Vec world_position;
    Vec fall_velocity;
    Vec local_position;
    float edge_distance;
    int position_stride;
    int state_stride;
    int removed_count;
    int index;
    int remove_particle;
    int old_bone_id;
    int new_bone_id;

    apdata = hdr;
    pfx_pre_wake();
    pfx = apfx;
    if (pfx == 0) {
        return;
    }

    vm = (BloodPfxVmView*)pfx->matrix;
    if (!apfx_render_obj->hide_flag_bits.hidden && vm->particle_count != 0) {
        position_stride = vm->position_stride;
        destination = (BloodParticlePosition*)pfx_get_field(vm, -2, 0x100);
        source = (BloodParticlePosition*)pfx_get_field(vm, -1, 0x100);
        state_stride = pfx_get_struct_size(vm, 0x600);
        states = (BloodVelocityState*)pfx_get_field(vm, -2, 0x600);
        removed_count = 0;
        index = 0;

        while (index < vm->particle_count - removed_count) {
            state = (BloodVelocityState*)((char*)states +
                state_stride * index);
            destination = (BloodParticlePosition*)((char*)
                pfx_get_field(vm, -2, 0x100) + position_stride * index);
            source = (BloodParticlePosition*)((char*)
                pfx_get_field(vm, -1, 0x100) + position_stride * index);
            remove_particle = 0;

            if (state->spawn_delay > 0.0f) {
                state->spawn_delay -= game_speed;
                memcpy(destination, source, position_stride);
                index++;
                continue;
            }

            if ((state->flags & 0x80) == 0) {
                state->flags |= 0x80;
                removed_count += obj_spawn_bld(
                    apfx_render_obj, state, 1,
                    state->step,
                    (BloodSpawnTarget*)state->path,
                    state->point_index, &source->position,
                    pfx->field_288, (PlyrPdata*)pfx->decal_owner);
            }

            destination->position.x =
                source->position.x + state->velocity.x * game_speed;
            destination->position.y =
                source->position.y + state->velocity.y * game_speed;
            destination->position.z =
                source->position.z + state->velocity.z * game_speed;
            destination->u = source->u;
            destination->v = source->v;
            state->travel_ticks -= game_speed;

            if (state->travel_ticks <= 0.0f) {
                path = state->path;
                record = &path->surface->records[
                    path->record_indices[state->point_index]];
                edge_distance = v3_dot_v3(
                    &record->edges[path->corner_indices[
                        state->point_index]].normal,
                    &destination->position) + 0.0005f;
                if (edge_distance >= record->edges[
                        path->corner_indices[state->point_index]].plane_distance) {
                    state->point_index++;
                    if (state->point_index >= path->point_count ||
                        path->corner_indices[state->point_index] < 0) {
                        if (state->path_point == 0 &&
                            state->step->definition->disable_ground_splat == 0) {
                            calc_bone_world_mat(apfx_render_obj, record->bone);
                            old_bone = apfx_render_obj->bones[record->bone];
                            world_position.x =
                                destination->position.x *
                                    old_bone->matrix.right.x +
                                destination->position.y *
                                    old_bone->matrix.up.x +
                                destination->position.z *
                                    old_bone->matrix.at.x + old_bone->delta.x;
                            world_position.y =
                                destination->position.x *
                                    old_bone->matrix.right.y +
                                destination->position.y *
                                    old_bone->matrix.up.y +
                                destination->position.z *
                                    old_bone->matrix.at.y + old_bone->delta.y;
                            world_position.z =
                                destination->position.x *
                                    old_bone->matrix.right.z +
                                destination->position.y *
                                    old_bone->matrix.up.z +
                                destination->position.z *
                                    old_bone->matrix.at.z + old_bone->delta.z;
                            fall_velocity.x = world_position.x * 0.5f;
                            fall_velocity.y = world_position.y * 0.5f - 0.002f;
                            fall_velocity.z = world_position.z * 0.5f;
                            spawn_bld_fall(
                                "bleedfall", (MkBone*)pfx->bone_mat,
                                &destination->position, &fall_velocity,
                                pfx->decal_owner);

                            owner = pfx->decal_owner;
                            foot_proc = owner->foot_print_proc;
                            if (foot_proc != 0 &&
                                foot_proc->instance ==
                                    owner->foot_print_proc_instance &&
                                foot_proc->entry == p_foot_print_wait) {
                                xfer_proc(foot_proc, p_foot_print);
                            }
                        }
                        remove_particle = 1;
                    } else {
                        previous_record = record;
                        record = &path->surface->records[
                            path->record_indices[state->point_index]];
                        old_bone_id = previous_record->bone;
                        new_bone_id = record->bone;
                        if (old_bone_id != new_bone_id) {
                            if (state->path_point == 0) {
                                old_bone = apfx_render_obj->bones[old_bone_id];
                                new_bone = apfx_render_obj->bones[new_bone_id];
                                if (old_bone != 0 && new_bone != 0 &&
                                    old_bone->parent_matrix != 0 &&
                                    new_bone->parent_matrix != 0) {
                                    world_position.x =
                                        destination->position.x *
                                            old_bone->parent_matrix->right.x +
                                        destination->position.y *
                                            old_bone->parent_matrix->up.x +
                                        destination->position.z *
                                            old_bone->parent_matrix->at.x +
                                        old_bone->parent_matrix->pos.x;
                                    world_position.y =
                                        destination->position.x *
                                            old_bone->parent_matrix->right.y +
                                        destination->position.y *
                                            old_bone->parent_matrix->up.y +
                                        destination->position.z *
                                            old_bone->parent_matrix->at.y +
                                        old_bone->parent_matrix->pos.y;
                                    world_position.z =
                                        destination->position.x *
                                            old_bone->parent_matrix->right.z +
                                        destination->position.y *
                                            old_bone->parent_matrix->up.z +
                                        destination->position.z *
                                            old_bone->parent_matrix->at.z +
                                        old_bone->parent_matrix->pos.z;
                                    local_position.x = world_position.x -
                                        new_bone->parent_matrix->pos.x;
                                    local_position.y = world_position.y -
                                        new_bone->parent_matrix->pos.y;
                                    local_position.z = world_position.z -
                                        new_bone->parent_matrix->pos.z;
                                    RwMatrixInvert(
                                        &inverse, new_bone->parent_matrix);
                                    destination->position.x =
                                        local_position.x * inverse.right.x +
                                        local_position.y * inverse.up.x +
                                        local_position.z * inverse.at.x;
                                    destination->position.y =
                                        local_position.x * inverse.right.y +
                                        local_position.y * inverse.up.y +
                                        local_position.z * inverse.at.y;
                                    destination->position.z =
                                        local_position.x * inverse.right.z +
                                        local_position.y * inverse.up.z +
                                        local_position.z * inverse.at.z;
                                    obj_spawn_bld(
                                        apfx_render_obj, state, 1,
                                        state->step,
                                        (BloodSpawnTarget*)state->path,
                                        state->point_index,
                                        &destination->position,
                                        pfx->field_288,
                                        (PlyrPdata*)pfx->decal_owner);
                                }
                            }
                            remove_particle = 1;
                        }
                    }
                }
                if (!remove_particle &&
                    !obj_set_bld_vel(
                        apfx_render_obj, &destination->position, state)) {
                    remove_particle = 1;
                }
            }

            if (!remove_particle) {
                index++;
                continue;
            }

            vm->particle_count--;
            if (index < vm->particle_count) {
                BloodVelocityState* last_state;
                BloodParticlePosition* last_position;

                last_state = (BloodVelocityState*)((char*)
                    pfx_get_field(vm, -2, 0x600) +
                    state_stride * vm->particle_count);
                memcpy(state, last_state, state_stride);
                if (removed_count != 0) {
                    last_position = (BloodParticlePosition*)((char*)
                        pfx_get_field(vm, -2, 0x100) +
                        position_stride * vm->particle_count);
                    removed_count--;
                } else {
                    last_position = (BloodParticlePosition*)((char*)
                        pfx_get_field(vm, -1, 0x100) +
                        position_stride * vm->particle_count);
                }
                memcpy(destination, last_position, position_stride);
            }
        }
        pfx_post_sleep();
        return;
    }

    pfx_post_sleep();
    if (pfx->hdr.instance != 0) {
        pfx->hdr.typed_vtbl->destroy(&pfx->hdr);
    }
}

int obj_spawn_bld(
    MkObj* object, BloodVelocityState* previous, int batch_count,
    BloodSpawnStep* step, BloodSpawnTarget* path, int point_index,
    const Vec* position, unsigned int art_id, PlyrPdata* owner) {
    BloodParticleDefinition* definition;
    BloodSurfaceRecord* record;
    BloodPfxConfigView* config;
    BloodPfxVmView* vm;
    BloodVelocityState* state;
    BloodVelocityState* prior_state;
    MkProc* proc;
    MkPfx* pfx;
    MkPtr* item;
    MkPtr* next;
    MkBone* bone;
    BloodParticlePosition* particle_position;
    Vec weights;
    float inverse_weight;
    float elapsed;
    float spawn_delay;
    int state_stride;
    int position_stride;
    int path_point;
    int last_path_point;
    int spawned;
    int batch;
    int frame;

    definition = step->definition;
    record = &path->surface->records[path->record_indices[point_index]];
    bone = object->bones[record->bone];
    spawned = 0;
    if (bone->parent_matrix == 0) {
        return 0;
    }

    pfx = 0;
    item = bone->list_80;
    while (item != 0) {
        MkHdr* hdr;

        hdr = item->hdr;
        if (item->instance != hdr->instance) {
            next = item->next;
            item->hdr = 0;
            destroy_mkptr(item);
            item = next;
            continue;
        }
        if (hdr->vtbl == &vtbl_pfx &&
            ((MkPfx*)hdr)->field_288 == (int)art_id) {
            pfx = (MkPfx*)hdr;
            break;
        }
        item = item->next;
    }

    if (pfx == 0) {
        proc = bleed_pfx_proc_item.proc;
        if (proc != 0) {
            if (proc->instance == bleed_pfx_proc_item.instance) {
                /* The process instance latch is still valid. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc != 0 &&
            pfx_create_raw_userdata(
                0, (void*)0x34, definition->field_00, 0x102, 0,
                (PfxInitCb)bloodfx_init, 0, 0, (void**)&pfx) != 0 &&
            pfx != 0) {
            mk_insert(&pfx->hdr, &proc->pdata_list);
            set_pfx_texture(
                (PfxVm*)pfx->matrix,
                (void*)get_shared_art_section_for_plyr_pdata(owner),
                (void*)art_id);
            pfx_bind_render_to_obj_bone(pfx, object, record->bone);
            pfx->field_288 = art_id;
            pfx->field_28C = (int)definition;
            pfx->decal_owner = (FighterMirror*)owner;

            config = (BloodPfxConfigView*)pfx;
            config->field_1F8 = definition->size;
            pfx_native_set_rgba(
                &config->color_1F4, definition->red, definition->green,
                definition->blue, definition->alpha);
            config->field_1B0 = definition->field_0C;
            config->field_1C4 = 1.0f;
            config->field_1C0 = 0x10;
            config->field_1C8 = 0.25f;
            config->field_1CC = 0.25f;
            config->flags_190 |= 0x40;
            config->flags_190 |= 0x80;
            mk_insert(&pfx->hdr, &bone->list_80);
            pfx->flags |= 0x10;
            pfx->name_dst = "blood";
        }
    }

    if (pfx == 0) {
        return 0;
    }

    last_path_point = step->spawn_count;
    if (last_path_point < 0) {
        last_path_point = 1;
    }
    inverse_weight = 1.0f / (float)last_path_point;
    if (definition->spawn_interval < game_speed) {
        spawn_delay = 0.0f;
    } else {
        spawn_delay = definition->spawn_interval - game_speed;
    }

    vm = (BloodPfxVmView*)pfx->matrix;
    position_stride = vm->position_stride;
    state_stride = pfx_get_struct_size(vm, 0x600);
    particle_position = (BloodParticlePosition*)(
        (char*)pfx_get_field(vm, -2, 0x100) +
        position_stride * vm->particle_count);
    state = (BloodVelocityState*)((char*)pfx_get_field(vm, -2, 0x600) +
        state_stride * vm->particle_count);
    prior_state = 0;

    for (batch = 0; batch < batch_count; batch++) {
        path_point = previous != 0 ? previous->path_point : -1;
        elapsed = 0.0f;
        while (elapsed < game_speed &&
               vm->particle_count < vm->particle_capacity) {
            path_point++;
            if (path_point >= last_path_point) {
                break;
            }

            state->flags = 0;
            state->path_point = path_point;
            state->spawn_delay = spawn_delay;
            state->point_index = point_index;
            state->step = step;
            state->path = (BloodPath*)path;
            if (previous != 0) {
                state->weight_0 = previous->weight_0;
                state->weight_1 = previous->weight_1;
                state->weight_step = previous->weight_step;
            } else {
                state->weight_0 = frand(1.0f);
                state->weight_1 = frand(1.0f);
                state->weight_step = frand(1.0f);
            }

            if (position == 0) {
                weights.x = state->weight_0 + path->interpolation_bias;
                weights.y = state->weight_1 + path->interpolation_bias;
                weights.z = state->weight_step + path->interpolation_bias;
                inverse_weight = 1.0f / (weights.x + weights.y + weights.z);
                weights.x *= inverse_weight;
                weights.y *= inverse_weight;
                weights.z *= inverse_weight;
                v3_blend3(
                    &particle_position->position, &weights,
                    &record->points[0],
                    &record->points[1], &record->points[2]);
            } else {
                particle_position->position = *position;
            }

            if (prior_state != 0) {
                prior_state->flags |= 0x80;
            }
            prior_state = state;
            if (path_point >= last_path_point - 1) {
                state->flags |= 0x80;
            }

            frame = -(int)(
                (16.0f / (float)last_path_point) *
                (float)(last_path_point - path_point) - 16.0f);
            particle_position->u = 0.25f * (float)(frame & 3);
            particle_position->v = 0.25f * (float)(frame >> 2);

            if (previous != 0) {
                state->velocity = previous->velocity;
                state->travel_ticks = previous->travel_ticks;
            } else {
                obj_set_bld_vel(
                    object, &particle_position->position, state);
            }

            particle_position = (BloodParticlePosition*)(
                (char*)particle_position + position_stride);
            state = (BloodVelocityState*)((char*)state + state_stride);
            spawned++;
            vm->particle_count++;
            elapsed += definition->spawn_interval;
        }
    }

    if ((unsigned int)pfx->tick == (unsigned int)exec_tick_ctr) {
        update_live_particles(vm);
    }
    return spawned;
}

static int obj_set_bld_vel(
    MkObj* object, const Vec* position, BloodVelocityState* state) {
    BloodSurfaceRecord* record;
    const Vec* current;
    const Vec* next;
    RwMatrix* matrix;
    int result;
    BloodPath* path;
    Vec direction;
    float current_weight;
    float next_weight;
    float inverse_distance;
    float speed;
    float projection;
    float duration;
    int corner;

    path = state->path;
    result = 1;
    if (state->point_index >= path->point_count) {
        result = 0;
        state->travel_ticks = 0.0f;
        state->velocity.z = 0.0f;
        state->velocity.y = 0.0f;
        state->velocity.x = 0.0f;
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
        blood_interpolate_direction(
            &direction, current, next, current_weight, next_weight,
            position);

        duration = length_v3(&direction);
        inverse_distance = 1.0f / duration;
        speed = path->speed_base + state->weight_step * path->speed_scale;

        calc_bone_world_mat(object, record->bone);
        matrix = &object->bones[record->bone]->matrix;
        projection = direction.y * matrix->up.y +
            direction.x * matrix->right.y +
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
        if (position->y * matrix->up.y +
                position->x * matrix->right.y +
                position->z * matrix->at.y + matrix->pos.y <
            object->ground_colls_y + 0.2f) {
            result = 0;
            state->travel_ticks = 0.0f;
        }

        inverse_distance = speed * inverse_distance;
        state->travel_ticks = duration / speed;
        state->velocity.x = direction.x * inverse_distance;
        state->velocity.y = direction.y * inverse_distance;
        state->velocity.z = direction.z * inverse_distance;
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

static inline int blood_surface_find_neighbor(
    const BloodSurface* surface, int triangle_index, int corner) {
    const int (*triangles)[3] = surface->triangles;
    const int* triangle = triangles[triangle_index];
    int next_corner = corner + 1;
    int candidate_triangle;
    int candidate_corner;
    int matching_corner;

    if (next_corner == 3) {
        next_corner = 0;
    }
    for (candidate_triangle = 0;
         candidate_triangle < surface->record_count;
         candidate_triangle++) {
        const int* candidate;

        if (candidate_triangle == triangle_index) {
            continue;
        }
        candidate = triangles[candidate_triangle];
        for (candidate_corner = 0; candidate_corner < 3; candidate_corner++) {
            if (triangle[corner] == candidate[candidate_corner]) {
                for (matching_corner = 0;
                     matching_corner < 3;
                     matching_corner++) {
                    if (triangle[next_corner] == candidate[matching_corner]) {
                        return candidate_triangle;
                    }
                }
            }
        }
    }
    return -1;
}

static void obj_bld_surface_build_polys(
    MkObj* object, BloodSurface* output, const BloodSurface* source) {
    BloodSurfaceRecord* record;
    BloodSurfaceVertex* vertices;
    int (*triangles)[3];
    const int* triangle;
    MkBone* ancestor;
    Vec edge_vectors[3];
    int selected_bone;
    int triangle_bones[3];
    int triangle_index;
    int corner;

    memcpy(output, source, sizeof(*source));
    output->records = get_mem(source->record_count * sizeof(*output->records));
    if (output->records == 0) {
        return;
    }

    vertices = output->vertices;
    record = output->records;
    triangles = output->triangles;
    for (triangle_index = 0;
         triangle_index < output->record_count;
         triangle_index++, record++) {
        triangle = triangles[triangle_index];
        triangle_bones[0] = obj_get_bid_for_tid(
            object, vertices[triangle[0]].tag);
        triangle_bones[1] = obj_get_bid_for_tid(
            object, vertices[triangle[1]].tag);
        if (triangle_bones[0] != triangle_bones[1]) {
            triangle_bones[2] = obj_get_bid_for_tid(
                object, vertices[triangle[2]].tag);
            if (triangle_bones[0] != triangle_bones[2]) {
                if (triangle_bones[1] == triangle_bones[2]) {
                    triangle_bones[0] = triangle_bones[1];
                } else {
                    for (corner = 1; corner < 3; corner++) {
                        ancestor = object->bones[
                            triangle_bones[corner]]->transform_parent;
                        while (ancestor != 0) {
                            if (ancestor == object->bones[triangle_bones[0]]) {
                                triangle_bones[0] = triangle_bones[corner];
                                break;
                            }
                            ancestor = ancestor->transform_parent;
                        }
                    }
                }
            }
        }

        selected_bone = triangle_bones[0];
        record->bone = selected_bone;
        object->bones[selected_bone]->flags_54_bits.calculation_locked = 1;
        object->bones[selected_bone]->flags_54_bits.field_bit3 = 1;
        for (corner = 0; corner < 3; corner++) {
            record->vertex_indices[corner] = triangle[corner];
            v3_sub_v3(
                &record->points[corner],
                &vertices[triangle[corner]].position,
                &object->bones[selected_bone]->bind_offset);
        }

        for (corner = 0; corner < 3; corner++) {
            record->neighbors[corner] = blood_surface_find_neighbor(
                output, triangle_index, corner);
        }

        uv_v3_to_v3(&edge_vectors[0], &record->points[0], &record->points[1]);
        uv_v3_to_v3(&edge_vectors[1], &record->points[1], &record->points[2]);
        uv_v3_to_v3(&edge_vectors[2], &record->points[2], &record->points[0]);
        v3_cross_v3(&record->normal, &edge_vectors[0], &edge_vectors[1]);
        record->plane_distance =
            v3_dot_v3(&record->normal, &record->points[0]);
        for (corner = 0; corner < 3; corner++) {
            v3_cross_v3(
                &record->edges[corner].normal,
                &edge_vectors[corner], &record->normal);
            record->edges[corner].plane_distance = v3_dot_v3(
                &record->edges[corner].normal, &record->points[corner]);
        }
    }
}

static void bloodfx_init(BloodFxUserdata* userdata) {
    userdata->flags_40_bits.bit4 = 1;
    userdata->flags_40_bits.bit3 = 1;
}
