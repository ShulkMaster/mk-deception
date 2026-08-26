#include "game/bgnd.h"
#include "game/ejb.h"
#include "game/game_info.h"
#include "game/jdn.h"
#include "game/plyr.h"
#include "game/collision.h"
#include "game/constrain.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/anim_types.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pebble.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"
#include "runtime/cam.h"
#include "runtime/cstdio.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "math/mk_math.h"
#include "math/gxMath.h"
#include "platform/main.h"
#include "platform/gcutils.h"
#include "rw/rpmatfx.h"
#include "rw/rwcamera_internal.h"

#pragma use_lmw_stmw on

typedef struct BgndAppendTextureEntry BgndAppendTextureEntry;
typedef struct BgndSwapTextureEntry BgndSwapTextureEntry;
typedef struct SlaughterhouseData SlaughterhouseData;
typedef struct BgndObstacleEventData BgndObstacleEventData;
typedef void (*BgndScriptEntryFn)(void);
typedef int (*BgndArenaObstacleCallback)(BgndObstacleEventData* event);
typedef struct MorphScript {
    unsigned int frame_count;
    unsigned short* frame_table;
} MorphScript;

typedef struct BgndDangerZone {
    int shape_type;             /* +0x00 */
    unsigned int obstacle_id;   /* +0x04 */
    float height;               /* +0x08 */
    float width;                /* +0x0C */
    float depth;                /* +0x10 */
    float y_angle;              /* +0x14 */
    Vec center;                 /* +0x18 */
    ArenaObstacle* obstacle;    /* +0x24 */
    unsigned int collision_script_function; /* +0x28 */
} BgndDangerZone;
typedef char BgndDangerZoneSize[(sizeof(BgndDangerZone) == 0x2C) ? 1 : -1];

void bgnd_delete_danger_zone(unsigned int zone_index);
void bgnd_enable_danger_zone(unsigned int zone_index, int enabled);

typedef struct BgndNpc {
    MkHdr hdr;          /* +0x00 */
    unsigned int id;    /* +0x08 */
    MkObj* object;      /* +0x0C */
    MkProc* anim_process; /* +0x10 */
    AnimPdata* animation; /* +0x14 */
    MkProc* brains;     /* +0x18 */
    struct BgndNpcAuxData* aux_data; /* +0x1C */
    ArenaObstacle* obstacle; /* +0x20 */
    char pad24[0x90];
    MkProc* collision_track_process; /* +0xB4 */
    MkProc* command_process; /* +0xB8 */
    struct BgndNpcAniCommand* animation_command; /* +0xBC */
    float field_C0;
    float field_C4;
    float field_C8;
    float field_CC;
    float field_D0;
} BgndNpc;
typedef char BgndNpcSize[(sizeof(BgndNpc) == 0xD4) ? 1 : -1];

typedef struct BgndNpcAuxData {
    MkHdr hdr;       /* +0x00 */
    BgndNpc* npc;    /* +0x08 */
    int values[2]; /* +0x0C */
} BgndNpcAuxData;

typedef struct BgndScriptProcData {
    MkHdr hdr;
    unsigned int script_index; /* +0x08 */
} BgndScriptProcData;

typedef struct BgndNpcAniCommand {
    MkHdr hdr;
    BgndNpc* npc;              /* +0x08 */
    unsigned int animation_id; /* +0x0C */
    float speed;               /* +0x10 */
    unsigned int flags;        /* +0x14 */
    float transition_frames;   /* +0x18 */
} BgndNpcAniCommand;

typedef struct BgndNpcCollisionTrackData {
    MkHdr hdr;
    BgndNpc* npc;       /* +0x08 */
    float height;       /* +0x0C */
    float radius;       /* +0x10 */
    float offset_y;     /* +0x14 */
} BgndNpcCollisionTrackData;
typedef char BgndNpcCollisionTrackDataSize[
    (sizeof(BgndNpcCollisionTrackData) == 0x18) ? 1 : -1];

struct BgndObstacleEventData {
    int event_id;        /* +0x00 */
    int field_04;
    Vec* impact_vector;  /* +0x08 */
    PlyrPdata* player_pdata; /* +0x0C */
    union {
        unsigned char flags;
        struct {
            unsigned char player_side : 1; /* bit7 */
            unsigned char pad_low : 7;
        } flag_bits;
    }; /* +0x10 */
};

typedef struct BgndPebbleControl {
    Vec position;          /* +0x00 */
    Vec angles;            /* +0x0C */
    Vec target_position;   /* +0x18 - state 13 interpolation target */
    Vec scale;             /* +0x24 */
    Vec velocity;          /* +0x30 */
    Vec angular_velocity;  /* +0x3C, degrees */
    Vec bounce_velocity;   /* +0x48 */
    int bounce_flags;      /* +0x54 */
    unsigned int end_behavior; /* +0x58 */
    unsigned int state; /* +0x5C - render/motion state */
    unsigned int bounce_ticks; /* +0x60 */
    unsigned int launch_ticks; /* +0x64 */
    float gravity;             /* +0x68 */
    unsigned int bounce_param; /* +0x6C - command-script function index */
} BgndPebbleControl;

typedef struct BgndPebbleCollection {
    char pad00[8];
    MKMATRIX* matrices;              /* +0x08, stride 0x40 */
    char pad0C[0x0C];
    BgndPebbleControl* pebbles; /* +0x18, stride 0x70 */
} BgndPebbleCollection;

typedef struct BgndPebblePlayerData {
    char pad00[8];
    BgndPebbleCollection* collection; /* +0x08 */
    MkSobj* sobj;                     /* +0x0C */
    int count;                        /* +0x10 */
    int mode;                         /* +0x14 */
} BgndPebblePlayerData;

typedef struct BgndSobjLaunchEntry BgndSobjLaunchEntry;
typedef int (*BgndSobjLaunchTest)(BgndSobjLaunchEntry*, unsigned int);

typedef struct BgndSobjLaunchEntry {
    MkSobj* object;                 /* +0x00 */
    float parameter;                /* +0x04 */
    char pad08[0x0C];
    float ground_y;                 /* +0x14 */
    unsigned int active;            /* +0x18 */
    int collision_enabled;          /* +0x1C */
    unsigned int collision_function; /* +0x20 - command-script function index */
    BgndSobjLaunchTest collision_test; /* +0x24 */
    unsigned int collision_arg;     /* +0x28 */
    int kill_enabled;               /* +0x2C */
    BgndSobjLaunchTest kill_test;   /* +0x30 */
} BgndSobjLaunchEntry; /* 0x34 */

typedef struct BgndSobjLaunchMonitor {
    MkHdr hdr;
    BgndSobjLaunchEntry entries[25];
} BgndSobjLaunchMonitor;

typedef struct BgndChunkLaunchEntry {
    MkSobj* object;          /* +0x00 */
    float vertical_accel;    /* +0x04 */
    unsigned int end_mode;   /* +0x08 */
    int field_0C;            /* +0x0C */
    int field_10;            /* +0x10 */
    float ground_y;          /* +0x14 */
    unsigned int active;     /* +0x18 */
    char pad1C[0x18];
} BgndChunkLaunchEntry; /* 0x34 */

typedef struct BgndChunkLaunchMonitor {
    MkHdr hdr;
    BgndChunkLaunchEntry entries[25];
} BgndChunkLaunchMonitor;

typedef struct BgndPebbleMonitor {
    MkHdr hdr;
    PebbleData* pebble_data; /* +0x08 */
    MkSobj* object;          /* +0x0C */
    int count;               /* +0x10 */
    int mode;                /* +0x14 */
} BgndPebbleMonitor;

typedef struct BgndCrack {
    Vec position;        /* +0x00 */
    unsigned int active; /* +0x0C */
} BgndCrack; /* 0x10 */

typedef struct BgndCrackPlacer {
    MkHdr hdr;
    PlyrInfo* player;       /* +0x08 */
    PebbleData* crack_pool; /* +0x0C */
    unsigned int delay;     /* +0x10 */
} BgndCrackPlacer; /* 0x14 */

typedef union BgndCollisionItemFlags {
    unsigned char raw;
    struct {
        unsigned char bit7 : 1;
        unsigned char bit6 : 1;
        unsigned char return_result : 1; /* bit5 */
        unsigned char disabled : 1;      /* bit4 */
        unsigned char pad_low : 4;
    } bits;
} BgndCollisionItemFlags;

typedef struct BgndCollisionItem {
    MkHdr hdr;                  /* +0x00 */
    unsigned int collision_id; /* +0x08 */
    int monitor_mode;           /* +0x0C */
    unsigned int script_function; /* +0x10 */
    int collision_mode;         /* +0x14 */
    BgndCollisionItemFlags flags; /* +0x18 */
} BgndCollisionItem;

float p_animate(void);
void* memset(void* dst, int c, unsigned long n);
void obj_create_sobjs(MkObj* object);
void destroy_mkprocs_pid(int pid);
int is_sobj_hidden(void* sobj);
void update_mksobj(MkSobj* sobj);
void set_arena_obstacle_callback(BgndArenaObstacleCallback callback);
void reset_collision_system(void);
void drone_ai_ok_to_think(void);
void drone_ai_dont_think(void);
void turn_controllers_off(void);
void turn_controllers_on(void);
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots);
static int bgnd_collision_to_script_interface(BgndObstacleEventData* event);
static float p_bgnd_launch_sobj_monitor(void);
static float p_bgnd_launch_chunk_monitor(void);
static float p_pebble_manual_monitor(void);
static float p_pebble_burst_monitor(void);
static float p_pebble_path_monitor(void);
static float p_crack_placer(void);
static float p_bgnd_timer_monitor(void);
static void bgnd_pebble_burst_at(int player, const Vec* position,
                                 unsigned int first, unsigned int end);
int fx_by_owner(const char* name, int owner);
int fx_next_emitter(int effect);
void fx_restart_emit(unsigned int effect);
void fx_resume_emit(unsigned int effect);
MkPfx* pfx_from_handle(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
MkHdr* pfx_get_emitter_obj(MkPfx* effect, int emitter);
int emitter_id_from_handle(unsigned int handle);
MkObj* pfx_bind_to_new_obj(MkPfx* effect, void* frame_source);
void resume_effect(const char* name);
void reset_effect(const char* name);
MkObj* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z);
void fx_reset(unsigned int effect);
void start_blood_particles(int effect, int bone_id, PlyrPdata* player,
                           void* limb);
int get_first_shape_center_for_obstacle_id(unsigned int obstacle_id,
                                           Vec* center);
void set_collision_render_state(int enabled);
MkPfx* find_pfx_by_name(const char* name);
void move_player(MkObj* object, const Vec* position, const Vec* angles);
MkProc* get_player_proc(void* object);
void xfer_player_proc(MkProc* process, MkProcEntryFn entry);
static float bgnd_call_script_function(void);
static int launch_sobj_watch_dist_from_orgin(BgndSobjLaunchEntry* entry,
                                              unsigned int distance_squared);
static int launch_sobj_watch_y_far_down(BgndSobjLaunchEntry* entry,
                                         unsigned int unused);
static int launch_sobj_watch_y_ground_plane(BgndSobjLaunchEntry* entry,
                                             unsigned int unused);

extern int mode_of_play;
extern int nb_slave_bones[];
extern int konquest_npc_bones[];
extern MkFlippedBoneMap flipped_nb_slave_bones;
extern RwCamera* Camera;
extern float ShadowStrength;
extern float fog_density;
extern float fog_distance;
extern float fog_color_real[4];
extern GlobalBackgroundEntry global_background_data[];
extern char bgnd_animations[0x84];
extern int g_current_reaction_info[6]; /* 0x18 bytes; retail clears word at +0x14 */
extern MkPtr* g_bgnd_collision_to_script_if[8];
extern BgndObstacleEventData* g_active_obstacle_event_data;
extern BgndCollisionItem* g_active_bgnd_col_item;
extern void* bgnd_spec_light_list;
extern void* plyr_light_list;
extern void* bgnd_light_list;
extern void* weapon_trail_light_list;
extern unsigned int default_bgnd_bits[2];
extern unsigned int default_pz_bgnd_bits[2];
extern int exec_tick_ctr;
extern int g_bl_beetles;
extern BgndPebbleControl* g_current_pebble;
extern MkSobj* g_active_sobj;
extern unsigned int g_active_bgnd_danger_zone;
BgndDangerZone bgnd_danger_zones[24];
extern MkObj* g_bgnd_preloaded_models[15];
extern MorphScript linear_slow_script;
extern MorphScript linear_140_script;
extern MorphScript linear_240_script;
extern MorphScript cos_script;
extern MorphScript cos_fast_script;
extern MorphScript skytemple_banner_script;
extern MorphScript kuatan_banner_script;
extern MkObj* plyr_obj;
extern void stop_me(void);
extern PlyrPdata* plyr_pdata;
extern Vec g_bgnd_scratch_pad_vectors[9];
extern MkObj* g_latest_obj_pfx;
extern BgndSobjLaunchEntry* g_launched_sobj_crossing_plane_pdata;
extern BgndSobjLaunchEntry* g_active_launched_sobj_pdata;
extern BgndSobjLaunchMonitor* g_sobj_launch_monitor_pdata;
extern BgndChunkLaunchMonitor* g_chunk_launch_monitor_pdata;
extern PebbleData* g_bgnd_cracks;
extern unsigned int g_bgnd_last_crack_overwritten;
extern void* obj_start_morph(MkObj* object, int sobj_id,
                             MorphScript* script, unsigned int flags);
extern void delete_obstacle_from_background_by_id(int obstacle_id);
extern void insert_ground_me_mkobj(MkObj* object);
extern AnimScript** bgnd_animation_table;
extern void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, AnimPdata* animation);
extern int build_bones_tbl(MkObj* object, const int* bone_tags);
extern void transition_to_anim_script(
    AnimPdata* animation, AnimScript* script, int flags,
    float transition_frames);
extern RpAtomic* set_atomic_material_alpha(RpAtomic* atomic,
                                            unsigned int alpha);
extern RpAtomic* set_atomic_material_specular(RpAtomic* atomic,
                                               unsigned int specular);
extern void plyr_turn_off_mirrorguy(PlyrInfo* player);
extern void plyr_turn_off_shadowbox(PlyrInfo* player);
extern void plyr_turn_on_mirrorguy(PlyrInfo* player);
extern void plyr_turn_on_shadowbox(PlyrInfo* player);
extern void run_camera_script(ScriptSlot* script, int argument, int flags);
extern void force_forward(int duration, int animation, float force,
                          float damping);

static inline void bgnd_copy_vector(Vec* destination, const Vec* source) {
    destination->x = source->x;
    destination->y = source->y;
    destination->z = source->z;
}

static inline void bgnd_reset_crack_pool(void) {
    BgndCrack* cracks;
    unsigned int i;

    g_bgnd_last_crack_overwritten = 0;
    cracks = (BgndCrack*)g_bgnd_cracks->user_data;
    g_game_info.crack_count = 0.0f;
    for (i = 0; i < 10; i++) {
        cracks[i].active = 0;
        cracks[i].position.z = 0.0f;
        cracks[i].position.y = 0.0f;
        cracks[i].position.x = 0.0f;
        cracks[i].position.y = 10000.0f;
        MKMatrixTranslate(&g_bgnd_cracks->pebbles[i].matrix,
                          &cracks[i].position, 0);
    }
}

static inline MkObj* bgnd_get_live_tracked_obj(PlyrPdata* player) {
    MkObj* object;

    object = player->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance == player->tracked_obj_instance) {
            return object;
        }
    }
    return 0;
}

BgndPebbleCollection* g_pebbles[20] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
BgndPebblePlayerData* g_pebbles_pdata[20] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

typedef struct BgndUnlockData {
    char pad00[0x10];
    unsigned int bgnds[2]; /* +0x10 */
    char pad18[0x20];
    unsigned int puzzle_bgnds[2]; /* +0x38 */
} BgndUnlockData;

extern BgndUnlockData gp_data;

void RwImageSetGamma(float gamma);
void init_misc_bgnd_data(void);
void load_background_anims(void* anims, int bgnd_id);
void init_weapon_trail_light_list(void);
void load_lights(void* lights, void* list);
void insert_fgnd_mkobj(void* bgnd_obj);
void set_background_color(int r, int g, int b, int a);
void UpdateShadowCameraLightSource(void* light);
void turn_fog_on(void);
void turn_fog_off(void);
void initialize_bgnd_collisions(void* data);
void load_effect_bank(int bank);
void mk_chess_init_bgnd_for_fight_mode(void);

void bgnd_level_fatality_end(void) {
    set_level_fatality_done_flag_state(1);
    reset_collision_system();
    g_game_info.flag_bits.level_fatality_active = 0;
    g_game_info.flag_bits.level_transition_active = 0;
    g_game_info.plyr0.slot.pdata->collision_disabled = 0;
    g_game_info.plyr1.slot.pdata->collision_disabled = 0;
    drone_ai_ok_to_think();
}
/* Soft ceiling 82.47%: exact body; GPR save/restore emission differs. */
void bgnd_level_fatality_start(int player) {
    drone_ai_dont_think();
    turn_controllers_off();
    g_game_info.flag_bits.level_fatality_active = 1;
    g_game_info.flag_bits.level_transition_active = 0;
    plyr_weapon_trail_hide(g_game_info.plyr0.slot.pdata->mirror_slots);
    plyr_weapon_trail_hide(g_game_info.plyr1.slot.pdata->mirror_slots);
    g_game_info.plyr0.slot.pdata->collision_disabled = 1;
    g_game_info.plyr1.slot.pdata->collision_disabled = 1;
    g_game_info.field_200 = player;
}
void bgnd_level_transition_end(void) {
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
    g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 0;
    g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 0;
    turn_controllers_on();
    g_game_info.flag_bits.level_transition_active = 0;
    g_game_info.flag_bits.level_fatality_active = 0;
    g_game_info.plyr0.slot.pdata->collision_disabled = 0;
    g_game_info.plyr1.slot.pdata->collision_disabled = 0;
    drone_ai_ok_to_think();
}
/* Soft ceiling 86.29%: exact body; GPR save/restore emission differs. */
void bgnd_level_transition_start(void) {
    drone_ai_dont_think();
    turn_controllers_off();
    g_game_info.flag_bits.level_transition_active = 1;
    g_game_info.flag_bits.level_fatality_active = 0;
    plyr_weapon_trail_hide(g_game_info.plyr0.slot.pdata->mirror_slots);
    plyr_weapon_trail_hide(g_game_info.plyr1.slot.pdata->mirror_slots);
    g_game_info.plyr0.slot.pdata->collision_disabled = 1;
    g_game_info.plyr1.slot.pdata->collision_disabled = 1;
}
int is_bgnd_locked(int bgnd_id) {
    unsigned long long unlocked;
    unsigned long long mask;

    if (bgnd_id < 0 || bgnd_id > 0x23) {
        return 1;
    }

    mask = 1ULL << bgnd_id;
    if (mode_of_play == 6) {
        unlocked =
            ((unsigned long long)(gp_data.puzzle_bgnds[0] | default_pz_bgnd_bits[0]) << 32) |
            (gp_data.puzzle_bgnds[1] | default_pz_bgnd_bits[1]);
        return (unlocked & mask) == 0;
    }

    if ((g_game_info.field_04 & 0x80) != 0 && (g_game_info.field_04 & 0x40) == 0) {
        return 0;
    }

    unlocked = ((unsigned long long)(gp_data.bgnds[0] | default_bgnd_bits[0]) << 32) |
               (gp_data.bgnds[1] | default_bgnd_bits[1]);
    return (unlocked & mask) == 0;
}
int get_next_bgnd(void) { return 0; }
/*
 * Exact material selection and stores; 82.98%, retail/local 160/164 bytes.
 * Retail lowers the all-material loop through CTR while local MWCC retains an
 * index comparison.
 */
void bgnd_force_specularity_off_for_material(
    unsigned int object_id, unsigned int material_id) {
    MkSobj* object;
    RpMaterial* material;
    RpGeometry* geometry;
    unsigned int i;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        if (material_id != 0) {
            material = sobj_find_material_by_id(object, material_id);
            if (material != 0) {
                material->surface.specular = 0.0f;
            }
        } else {
            geometry = object->atomic->geometry;
            for (i = 0; i < geometry->matList.numMaterials; i++) {
                geometry->matList.materials[i]->surface.specular = 0.0f;
            }
        }
    }
}
/* Exact operations; 81.46%, retail/local 148/164 bytes from stmw/lmw choice. */
void bgnd_sobj_set_texture_kl_values(
    unsigned int object_id, unsigned int material_id, int dual_texture,
    float l, int k) {
    MkSobj* object;
    RpMaterial* material;
    RwTexture* texture;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        material = sobj_find_material_by_id(object, material_id);
        if (material != 0) {
            if (dual_texture != 0) {
                texture = RpMatFXMaterialGetDualTexture(material);
            } else {
                texture = material->texture;
            }
            if (texture != 0) {
                set_texture_mipmap_KL_manual(texture, k, l);
            }
        }
    }
}
float script_fabs(float value) {
    if (value >= 0.0f) {
        return value;
    }
    return -value;
}
AnimPdata* animate_obj(
    MkObj* object, AnimScript* script, const int* bone_tags,
    MkFlippedBoneMap* flipped_bones, void* ground_collisions, int active,
    float frame) {
    AnimPdata* animation;

    animation = 0;
    if (create_mkproc_anim(0x5002, p_animate, &animation) != 0) {
        animation->obj = object;
        animation->obj_instance = object->hdr.instance;
        if (bone_tags != 0) {
            build_bones_tbl(object, bone_tags);
        }
        object->flipped_bone_map = flipped_bones;
        object->ground_colls = ground_collisions;
        set_anim_script(animation, (AniData*)script, 0x21);
        animation->frame = frame;
        if (active != 0) {
            set_root_and_obj_movement_weights(0.0f, 1.0f, animation);
        }
    }
    return animation;
}
void set_obj_light_flags(MkObj* object, int flags) {
    if (object != 0) {
        object->light_flags = flags;
    }
}
void set_obj_ang(MkObj* object, float x, float y, float z) {
    if (object != 0) {
        object->ang.x = x;
        object->ang.y = y;
        object->ang.z = z;
        object->flags_08_bits.transform_dirty = 1;
    }
}
void set_obj_pos(MkObj* object, float x, float y, float z) {
    if (object != 0) {
        object->pos.value.x = x;
        object->pos.value.y = y;
        object->pos.value.z = z;
        object->flags_08_bits.bit7 = 1;
    }
}
void bgnd_sobj_start_morph(int object_id, int start_shape, int end_shape,
                           int ticks) {
    (void)object_id;
    (void)start_shape;
    (void)end_shape;
    (void)ticks;
}
int bgnd_get_obj_pointer(int player) {
    MkObj* object;

    object = 0;
    switch (player) {
    case 0:
        object = g_game_info.player_objects[0];
        break;
    case 1:
        object = g_game_info.player_objects[0];
        break;
    }
    return (int)object;
}
void scripted_camera_script_exit(void) {
    destroy_mkprocs_pid(0x9006);
    mkproc_die();
}
void skytemple_arrange_fence_pebbles_around_pos(int player, int count,
                                                const Vec* position) {
    (void)player;
    (void)count;
    (void)position;
}
void skytemple_set_fence_pebble_vel(int player, int count, float x, float y,
                                    float z) {
    (void)player;
    (void)count;
    (void)x;
    (void)y;
    (void)z;
}
static void p_sh_fatality_body_parts(void) {}
static void sh_update_fatality_body_part(void) {}
static void sh_start_fatality_body_parts(void) {}
void p_sh_throw_plyr_in_grinder(void) {}
static void p_sh_bottom_floor_blood_fall(void) {}
static void sh_update_blood_fall_pebbles(void) {}
static void sh_init_bottom_floor_blood_fall_pebbles(void) {}
void sh_lower_level_pebble_unhide(void) {}
void sh_lower_level_pebble_hide(void) {}
static void sh_load_objs(void) {}
void bgnd_sh_level_2(void) {}
void bgnd_sh_level_1(void) {}
void bgnd_start_sh_fx(void) {}
void bgnd_clean_slaughterhouse(void) {}
static void destroy_slaughterhouse_pdata(SlaughterhouseData* pdata);
void vdestroy_slaughterhouse_pdata(SlaughterhouseData* pdata) {
    destroy_slaughterhouse_pdata(pdata);
}
static void destroy_slaughterhouse_pdata(SlaughterhouseData* pdata) {
    (void)pdata;
}
void start_bl_beetles_live_top_floor(void) {}
static void p_bl_beetle_brains(void) {}
static void beetle_squashed(void) {}
static void bl_process_beetle_follow_plyr_personality(void) {}
static void bl_process_beetle_under_glass_personality(void) {}
static void bl_process_beetle_under_glass_traveller_personality(void) {}
static void bl_process_beetle_runaway_personality(void) {}
static void bl_process_beetle_transition_personality(void) {}
static void bl_process_beetle_chilling(void) {}
static void bl_process_beetle_track_plyr(void) {}
static void bl_process_beetle_climb_a_wall(void) {}
static void bl_process_general_movement(void) {}
static void bl_init_beetle_pebbles_second_floor(void) {}
static void bl_init_beetle_pebbles_first_floor(void) {}
void bgnd_clean_beetlelair(void) {
    g_bl_beetles = 0;
}
void bgnd_reg_col_cb_for_beetle_lair(void) {}
static void beetle_lair_collision_cb(void) {}
void r_beetle_lair_transition(void) {}
static void beetle_lair_react_to_wall_danger_zone_cb(void) {}
static void p_beetle_lair_wall_breaking_controller(void) {}
static void p_beetle_lair_watch_remaining_fall_scene(void) {}
static void winner_watching_him_fall(void) {}
static void victim_fall_down_a_level(void) {}
static void p_beetle_lair_front_wall_breaking(void) {}
static void p_beetle_lair_downstairs_wall_break_cam_control(void) {}
static void p_beetle_lair_column_breaking(void) {}
static void p_launch_final_column_piece(void) {}
static void p_launch_column_piece(void) {}
static void p_bl_flip_column_piece(void) {}
void bgnd_set_viewing_of_danger_zones(int enabled) {
    g_game_info.switch_input_flags.view_danger_zones = enabled;
    set_collision_render_state(enabled);
}
void bgnd_set_danger_zone_y_angle(void) {}
void bgnd_set_danger_zone_depth(void) {}
void bgnd_set_danger_zone_width(float width);
void bgnd_set_danger_zone_radius(float radius) {
    int shape_type;

    shape_type = bgnd_danger_zones[g_active_bgnd_danger_zone].shape_type;
    if (shape_type == 1 || shape_type == 2) {
        bgnd_set_danger_zone_width(radius);
    }
}
void bgnd_set_danger_zone_width(float width) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    CollisionShape box_shape __attribute__((aligned(16)));
    CollisionShape cylinder_shape __attribute__((aligned(16)));
    CollisionShape special_cylinder_shape __attribute__((aligned(16)));

    if (g_active_bgnd_danger_zone <= 24) {
        zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
        if (zone->obstacle != 0) {
            delete_obstacle_from_background_by_id(zone->obstacle_id);
            zone->obstacle = 0;
        }
    }
    zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
    zone->width = width;
    switch (zone->shape_type) {
    case 0:
        build_col_shape_vertical_box(
            &box_shape, &zone->center, zone->width, zone->height,
            zone->depth, zone->y_angle);
        obstacle = add_shape_to_background_obstacle_list(
            &box_shape, zone->obstacle_id);
        obstacle = obstacle != 0 ? obstacle : 0;
        obstacle->flags.bits.danger_zone = 1;
        break;
    case 1:
        build_col_shape_vertical_cylinder(
            &cylinder_shape, &zone->center, zone->width, zone->height);
        obstacle = add_shape_to_background_obstacle_list(
            &cylinder_shape, zone->obstacle_id);
        obstacle = obstacle != 0 ? obstacle : 0;
        obstacle->flags.bits.danger_zone = 1;
        break;
    case 2:
        build_col_shape_vertical_cylinder(
            &special_cylinder_shape, &zone->center, zone->width, zone->height);
        obstacle = add_shape_to_background_obstacle_list(
            &special_cylinder_shape, zone->obstacle_id);
        obstacle = obstacle != 0 ? obstacle : 0;
        obstacle->flags.value |= 0x10;
        obstacle->flags.bits.danger_zone = 1;
        break;
    default:
        obstacle = 0;
        break;
    }
    zone->obstacle = obstacle;
    set_background_obstacle_repel_flag(zone->obstacle_id, 0);
    bgnd_enable_danger_zone(g_active_bgnd_danger_zone, 0);
}
void bgnd_set_danger_zone_center_position(void) {}
/* Soft ceiling 89.78%: exact behavior; retail selects lwzu field induction. */
void bgnd_delete_danger_zone(unsigned int zone_index) {
    BgndDangerZone* zone;

    if (zone_index <= 24) {
        zone = &bgnd_danger_zones[zone_index];
        if (zone->obstacle != 0) {
            delete_obstacle_from_background_by_id(
                bgnd_danger_zones[zone_index].obstacle_id);
            zone->obstacle = 0;
        }
    }
}
void bgnd_enable_danger_zone(unsigned int zone_index, int enabled) {
    BgndCollisionItem* item;
    BgndDangerZone* zone;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;

    if (zone_index < 24) {
        zone = &bgnd_danger_zones[zone_index];
        if (zone->obstacle != 0) {
            if (enabled != 0) {
                if (zone->collision_script_function != 0) {
                    list = &g_bgnd_collision_to_script_if[7];
                    if (list != 0) {
                        link = *list;
                        while (link != 0) {
                            item = (BgndCollisionItem*)link->hdr;
                            if (link->instance != item->hdr.instance) {
                                next = link->next;
                                link->hdr = 0;
                                destroy_mkptr(link);
                                link = next;
                            } else if (item->collision_id == zone->obstacle_id) {
                                item->flags.bits.disabled = 0;
                                break;
                            } else {
                                link = link->next;
                            }
                        }
                    }
                }
                set_background_obstacle_disable_flag(zone->obstacle_id, 0);
            } else {
                if (zone->collision_script_function != 0) {
                    list = &g_bgnd_collision_to_script_if[7];
                    if (list != 0) {
                        link = *list;
                        while (link != 0) {
                            item = (BgndCollisionItem*)link->hdr;
                            if (link->instance != item->hdr.instance) {
                                next = link->next;
                                link->hdr = 0;
                                destroy_mkptr(link);
                                link = next;
                            } else if (item->collision_id == zone->obstacle_id) {
                                item->flags.bits.disabled = 1;
                                break;
                            } else {
                                link = link->next;
                            }
                        }
                    }
                }
                set_background_obstacle_disable_flag(zone->obstacle_id, 1);
            }
        }
    }
}
void bgnd_set_active_danger_zone(unsigned int zone) {
    if (zone < 0x18) {
        g_active_bgnd_danger_zone = zone;
    }
}
void bgnd_create_danger_zone(void) {}
static float plyr_is_prone(void) {
    stop_me();
    plyr_obj->pos.value.y = 100.0f;
    plyr_obj->flags_08_bits.moving = 0;
    plyr_obj->flags_09_bits.bit6 = 0;
    plyr_obj->flags_09_bits.launched = 0;
    update_mkobj(plyr_obj);
    return 1.0f;
}
void bgnd_delete_proc_by_id(int pid) {
    destroy_mkprocs_pid(pid);
}
/* Near match: 99.57%, exact size; only the -1.0f relocation label differs. */
float p_bgnd_script_in_proc(void) {
    BgndScriptProcData* pdata;

    pdata = (BgndScriptProcData*)pdata_of_proc(aproc);
    if (pdata->script_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(g_game_info.cmdscript, pdata->script_index);
    cmdscript_execute(g_game_info.cmdscript);
    return -1.0f;
}
void bgnd_start_script_in_proc_bigstack(
    int process_id, unsigned int script_index) {
    BgndScriptProcData* data;
    MkProc* process;

    data = 0;
    process = _create_mkproc_generic_bigstack(
        process_id, 0x1F, p_bgnd_script_in_proc,
        sizeof(BgndScriptProcData), (MkHdr**)&data);
    if (process != 0 && data != 0) {
        data->script_index = script_index;
        set_process_as_scriptable(process);
    }
}

void bgnd_start_script_in_proc(
    int process_id, unsigned int script_index) {
    BgndScriptProcData* data;
    MkProc* process;

    data = 0;
    process = _create_mkproc_generic_tinystack(
        process_id, 0x1F, p_bgnd_script_in_proc,
        sizeof(BgndScriptProcData), (MkHdr**)&data);
    if (process != 0 && data != 0) {
        data->script_index = script_index;
        set_process_as_scriptable(process);
    }
}
/*
 * Near match: 99.24%, retail/local 216/216 bytes. The remaining difference is
 * the register choice for the initial NPC and obstacle pointer loads.
 */
static float p_npc_track_colshape(void) {
    MkObj* object;
    BgndNpcCollisionTrackData* data;
    MkPtr** list;
    CollisionObj* collision;
    MkPtr* node;
    MkPtr* next;
    CollisionShape shape __attribute__((aligned(16)));
    Vec center;

    data = (BgndNpcCollisionTrackData*)apdata;
    object = data->npc->object;
    list = &data->npc->obstacle->shapes;
    if (list != 0) {
        node = *list;
        while (node != 0) {
            collision = (CollisionObj*)node->hdr;
            if (node->instance != collision->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
            } else {
                center.x = object->pos.value.x;
                center.y = object->pos.value.y;
                center.z = object->pos.value.z;
                center.y += data->offset_y;
                build_col_shape_vertical_cylinder(
                    &shape, &center, data->radius, data->height);
                collision_obj_set_shape(collision, &shape);
                node = node->next;
            }
        }
    }
    return 1.0f;
}

/* Shared inline lookup evidenced by the repeated retail CFG in NPC helpers. */
static inline BgndNpc* bgnd_find_npc(unsigned int npc_id) {
    MkPtr** list;
    MkPtr* node;
    MkPtr* next;
    BgndNpc* npc;

    list = &g_game_info.npc_list;
    if (list != 0) {
        node = *list;
        while (node != 0) {
            npc = (BgndNpc*)node->hdr;
            if (node->instance != npc->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
            } else {
                if (npc->id == npc_id) {
                    return npc;
                }
                node = node->next;
            }
        }
    }
    return 0;
}

void bgnd_npc_add_collision_shape(
    unsigned int npc_id, unsigned int obstacle_id, int track,
    float radius, float height, float offset_y, float unused_offset_z) {
    BgndNpc* npc;
    BgndNpcCollisionTrackData* data;
    CollisionShape shape __attribute__((aligned(16)));
    Vec center;

    npc = bgnd_find_npc(npc_id);
    center.x = npc->object->pos.value.x;
    center.y = npc->object->pos.value.y;
    center.z = npc->object->pos.value.z;
    center.y += offset_y;
    build_col_shape_vertical_cylinder(&shape, &center, radius, height);
    npc->obstacle =
        add_shape_to_background_obstacle_list(&shape, obstacle_id);

    if (track != 0) {
        npc->collision_track_process = _create_mkproc_generic_tinystack(
            0xC010, 0x1F, p_npc_track_colshape,
            sizeof(BgndNpcCollisionTrackData), (MkHdr**)&data);
        data->npc = npc;
        data->radius = radius;
        data->height = height;
        data->offset_y = offset_y;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(
                &npc->collision_track_process->hdr,
                &g_game_info.bgnd_obj->child_list);
        }
    }
}

/* Exact typed lookup and position copy. */
void bgnd_npc_get_pos(unsigned int npc_id, Vec* position) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc != 0) {
        position->x = npc->object->pos.value.x;
        position->y = npc->object->pos.value.y;
        position->z = npc->object->pos.value.z;
    }
}
/* Exact typed lookup and Y-position store. */
void bgnd_npc_set_pos_y(unsigned int npc_id, float y) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc != 0) {
        npc->object->pos.value.y = y;
    }
}
void bgnd_npc_set_ani_speed(unsigned int npc_id, float speed) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc != 0 && npc->animation != 0) {
        npc->animation->step = speed;
    }
}

/*
 * Near match: 95.70%. The typed compound guard emits a shorter equivalent
 * branch sequence than retail; lookup, bounds, widths, and return are exact.
 */
int bgnd_npc_get_aux_int_data(unsigned int npc_id, unsigned int index) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc == 0 || npc->aux_data == 0 || index >= 2) {
        return 0;
    }
    return npc->aux_data->values[index];
}

/* Near match: 97.71%; only equivalent null/bounds branch lowering differs. */
void bgnd_npc_set_aux_int_data(
    unsigned int npc_id, unsigned int index, int value) {
    BgndNpc* npc;
    BgndNpcAuxData* aux_data;

    npc = bgnd_find_npc(npc_id);
    if (npc != 0) {
        aux_data = npc->aux_data;
        if (aux_data == 0) {
            return;
        }
        if (index < 2) {
            aux_data->values[index] = value;
        }
    }
}
void bgnd_add_scripted_brains_to_npc(
    unsigned int npc_id, unsigned int script_index) {
    BgndNpc* npc;
    MkProc* process;
    BgndScriptProcData* process_data;

    npc = bgnd_find_npc(npc_id);
    process_data = 0;
    process = _create_mkproc_generic_tinystack(
        0xC015, 0x1F, p_bgnd_script_in_proc, sizeof(BgndScriptProcData),
        (MkHdr**)&process_data);
    if (process != 0 && process_data != 0) {
        process_data->script_index = script_index;
        set_process_as_scriptable(process);
    }

    npc->aux_data =
        (BgndNpcAuxData*)get_mkpdata_generic(sizeof(BgndNpcAuxData));
    npc->aux_data->npc = npc;
    if (g_game_info.bgnd_obj != 0) {
        mk_insert(&npc->aux_data->hdr, &g_game_info.bgnd_obj->child_list);
    }
}

void bgnd_add_brains_to_npc(
    unsigned int npc_id, MkProcEntryFn brain_function) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->brains = _create_mkproc_generic_tinystack(
        0xC015, 0x1F, brain_function, sizeof(BgndNpcAuxData),
        (MkHdr**)&npc->aux_data);
    npc->aux_data->npc = npc;
    if (g_game_info.bgnd_obj != 0) {
        mk_insert(&npc->brains->hdr, &g_game_info.bgnd_obj->child_list);
    }
}
void bgnd_npc_like_plyr(unsigned int npc_id) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->light_flags = 4;
    npc->object->flags_09_bits.launched = 1;
    npc->object->flags_09_bits.bit6 = 1;
    insert_ground_me_mkobj(npc->object);
}
void bgnd_npc_set_pos_vel(
    unsigned int npc_id, float x, float y, float z) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.gravity_enabled = 1;
    npc->object->pos_vel.x = x;
    npc->object->pos_vel.y = y;
    npc->object->pos_vel.z = z;
}

/*
 * Near match: 98.24%. Clean C folds retail's zero-times-speed expression;
 * the remaining differences are that fold and local constant relocations.
 */
void bgnd_npc_set_pos_vel_heading(unsigned int npc_id, float speed) {
    BgndNpc* npc;
    float heading;
    float sine;
    float cosine;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.gravity_enabled = 1;
    heading = 0.000005992112f *
              (float)(((int)(166886.1f * npc->object->ang.y)) & 0xFFFFF);
    sine = gxMathSin(heading);
    cosine = gxMathCos(heading);
    npc->object->pos_vel.x = sine * speed;
    npc->object->pos_vel.y = 0.0f * speed;
    npc->object->pos_vel.z = cosine * speed;
}

void bgnd_npc_set_scale(
    unsigned int npc_id, float x, float y, float z) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.scale_active = 1;
    npc->object->scale.x = x;
    npc->object->scale.y = y;
    npc->object->scale.z = z;
}

float bgnd_npc_get_ang_y(unsigned int npc_id) {
    return bgnd_find_npc(npc_id)->object->ang.y;
}

/* Near match: 99.48%, exact size; only local constant labels differ. */
void bgnd_npc_adjust_y_ang(unsigned int npc_id, float angle) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.angular_velocity_enabled = 1;
    npc->object->ang.y += angle;
    npc->object->ang.y = 0.000005992112f *
                         (float)(((int)(166886.1f * npc->object->ang.y)) &
                                 0xFFFFF);
}

/* Near match: 99.46%, exact size; only local constant labels differ. */
void bgnd_npc_set_y_ang(unsigned int npc_id, float angle) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.angular_velocity_enabled = 1;
    npc->object->ang.y = angle;
    npc->object->ang.y = 0.000005992112f *
                         (float)(((int)(166886.1f * npc->object->ang.y)) &
                                 0xFFFFF);
}

void bgnd_npc_set_pos(
    unsigned int npc_id, float x, float y, float z) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->pos.value.x = x;
    npc->object->pos.value.y = y;
    npc->object->pos.value.z = z;
    update_mkobj(npc->object);
}
/* Near match: 94.31%, exact lookup; only return register coloring differs. */
BgndNpc* bgnd_fetch_npc(unsigned int npc_id) {
    return bgnd_find_npc(npc_id);
}
static float bgnd_npc_play_ani(void);
float bgnd_npc_idle(void);
float p_animate(void);

/*
 * Near match: 99.37%, retail/local 772/772 bytes. The remaining differences
 * are pooled-string and local floating-constant relocation labels.
 */
void bgnd_create_named_npc_in_slot(
    unsigned int npc_id, const char* model_name, unsigned int animation_id,
    unsigned int bone_table) {
    char message[180];
    BgndNpcAniCommand* command;
    BgndNpc* npc;
    MkObj* object;
    int art_slot;

    command = 0;
    npc = (BgndNpc*)get_mkhdr_generic(0x88);
    npc->object = 0;
    npc->anim_process = 0;
    npc->animation = 0;
    npc->command_process = 0;
    npc->field_C8 = 0.0f;
    npc->field_C4 = 0.0f;
    npc->field_C0 = 0.0f;
    npc->field_D0 = 0.0f;
    npc->field_CC = 0.0f;

    if (npc_id < 15) {
        if (mode_of_play == 9 || mode_of_play == 10) {
            art_slot = 0x8003D;
        } else {
            art_slot = 0x2001E;
        }
        if (g_game_info.bgnd_id == 0x16) {
            art_slot = 0x18006D;
        }
        g_bgnd_preloaded_models[npc_id] = (MkObj*)load_named_model_from_slot(
            art_slot, model_name, npc_id + 0xC000, 0);
        if (g_bgnd_preloaded_models[npc_id] != 0) {
            g_bgnd_preloaded_models[npc_id]->pos.value.z = 0.0f;
            g_bgnd_preloaded_models[npc_id]->pos.value.y = 0.0f;
            g_bgnd_preloaded_models[npc_id]->pos.value.x = 0.0f;
            g_bgnd_preloaded_models[npc_id]->light_flags = 1;
            obj_create_sobjs(g_bgnd_preloaded_models[npc_id]);
            hide_obj(g_bgnd_preloaded_models[npc_id]);
            insert_fgnd_mkobj(g_bgnd_preloaded_models[npc_id]);
            if (g_bgnd_preloaded_models[npc_id] != 0 &&
                g_game_info.bgnd_obj != 0) {
                mk_insert(
                    &g_bgnd_preloaded_models[npc_id]->hdr,
                    &g_game_info.bgnd_obj->child_list);
            }
        } else {
            sprintf(message, "Could not load bgnd preload model %s", model_name);
        }
    }

    object = g_bgnd_preloaded_models[npc_id];
    object->flags_08_bits.gravity_enabled = 0;
    object->flags_08_bits.rotation_enabled = 0;
    object->flags_08_bits.airborne = 1;
    object->flags_08_bits.airborne = 1;
    if (bone_table == 0) {
        build_bones_tbl(object, nb_slave_bones);
        object->flipped_bone_map = &flipped_nb_slave_bones;
    } else if (bone_table == 1) {
        build_bones_tbl(object, konquest_npc_bones);
    }
    object->light_flags = 1;
    unhide_obj(object);

    npc->object = object;
    npc->id = npc_id;
    npc->anim_process = 0;
    npc->animation = 0;
    mk_insert(&npc->hdr, &g_game_info.npc_list);

    if (animation_id != 0xFFFF) {
        npc->anim_process =
            create_mkproc_anim(0xC01F, p_animate, &npc->animation);
        npc->animation->obj = object;
        npc->animation->obj_instance = object->hdr.instance;
        set_root_and_obj_movement_weights(0.0f, 1.0f, npc->animation);
        set_anim_script(
            npc->animation, (AniData*)bgnd_animation_table[animation_id], 0);
        npc->animation->hand_transition_limit = 0.125f;
        npc->command_process = _create_mkproc_generic_bigstack(
            0xC016, 0x1F, bgnd_npc_idle, sizeof(BgndNpcAniCommand),
            (MkHdr**)&command);
        if (npc->command_process != 0 && command != 0) {
            command->npc = npc;
            npc->animation_command = command;
            if (g_game_info.bgnd_obj != 0) {
                mk_insert(
                    &npc->command_process->hdr,
                    &g_game_info.bgnd_obj->child_list);
            }
        }
    }
}

void bgnd_npc_start_ani(
    unsigned int npc_id, unsigned int animation_id, unsigned int flags,
    float transition_frames, float speed) {
    BgndNpc* npc;
    BgndNpcAniCommand* command;

    npc = bgnd_find_npc(npc_id);
    command = npc->animation_command;
    command->animation_id = animation_id;
    npc->animation_command->speed = speed;
    npc->animation_command->flags = flags;
    npc->animation_command->transition_frames = transition_frames;
    xfer_proc(npc->command_process, bgnd_npc_play_ani);
}

static float bgnd_npc_play_ani(void) {
    BgndNpcAniCommand* command;

    command = (BgndNpcAniCommand*)apdata;
    transition_to_anim_script(
        command->npc->animation, bgnd_animation_table[command->animation_id],
        command->flags, command->transition_frames);
    command->npc->animation->step = command->speed;
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(bgnd_npc_idle, 0.0f);
    return 0.0f;
}

/* Near match: 99.74%, exact size; only the 1.0f relocation label differs. */
float bgnd_npc_idle(void) {
    BgndNpcAniCommand* command;

    command = (BgndNpcAniCommand*)apdata;
    xfer_proc(command->npc->anim_process, p_animate);
    for (;;) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}
/*
 * Exact direction, normalization, and scaling; 86.86%, retail/local 332/348.
 * Residue is the positive-length branch lowering and stmw/lmw selection.
 */
void bgnd_set_launch_velocity_based_on_sobj_pos(
    int velocity_index, unsigned int source_id, unsigned int target_id,
    void* script, float horizontal_velocity, float vertical_velocity) {
    union {
        float f;
        unsigned int u;
    } input, estimate;
    Vec* velocity;
    Vec* source;
    Vec* target;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x;
    float x_squared;
    float z_squared;

    (void)script;
    velocity = &g_bgnd_scratch_pad_vectors[velocity_index];
    source = sobj_get_world_pos(
        obj_find_sobj_by_id(g_game_info.bgnd_obj, source_id));
    target = sobj_get_world_pos(
        obj_find_sobj_by_id(g_game_info.bgnd_obj, target_id));
    velocity->x = target->x - source->x;
    velocity->z = target->z - source->z;
    x = velocity->x;
    x_squared = x * x;
    z_squared = velocity->z * velocity->z;
    squared = x_squared + z_squared;
    inverse_length = 0.0f;
    if (squared > 0.0f) {
        input.f = squared;
        estimate.u = 0x5F375A00U - (input.u >> 1);
        product = estimate.f * (squared * estimate.f);
        correction = 3.0f - product;
        inverse_length = 0.0625f * estimate.f * correction *
                         -(correction * (product * correction) - 12.0f);
    }
    velocity->x = x * inverse_length;
    velocity->z *= inverse_length;
    velocity->x *= horizontal_velocity;
    velocity->z *= horizontal_velocity;
    velocity->y = vertical_velocity;
}
void bgnd_set_sobj_launch_params_exact(
    int velocity_index, int angle_index, void* script, float impact_scale,
    float angle_x, float angle_y, float angle_z, float vertical_velocity,
    float heading) {
    Vec* velocity;
    Vec* angles;

    (void)script;
    velocity = &g_bgnd_scratch_pad_vectors[velocity_index];
    angles = &g_bgnd_scratch_pad_vectors[angle_index];
    velocity->x = g_game_info.impact_vector.x * impact_scale;
    velocity->y = g_game_info.impact_vector.y * impact_scale;
    velocity->z = g_game_info.impact_vector.z * impact_scale;
    angles->x = angle_x;
    angles->y = angle_y;
    angles->z = angle_z;
    velocity->y = vertical_velocity;
    rotate_xz(velocity, velocity, heading);
}
/*
 * Exact vector algorithm and call order; 92.23%, retail/local 348/352 bytes.
 * Residue is r30/r31 coloring, stmw/lmw selection, and equivalent branch
 * lowering around the impact-vector sentinel.
 */
void bgnd_set_sobj_launch_params(
    int velocity_index, int angle_index, unsigned int angle_source,
    void* script, float impact_scale, float angle_scale,
    float random_x, float random_y, float random_z,
    float vertical_velocity, float heading) {
    Vec* velocity;
    Vec* angles;
    Vec* source;
    float x_offset;
    float y_offset;
    float z_offset;

    (void)script;
    velocity = &g_bgnd_scratch_pad_vectors[velocity_index];
    angles = &g_bgnd_scratch_pad_vectors[angle_index];
    source = &g_game_info.impact_vector;
    velocity->x = source->x * impact_scale;
    velocity->y = source->y * impact_scale;
    velocity->z = source->z * impact_scale;
    if (angle_source == 0xF0) {
        /* Keep using the impact vector. */
    } else {
        source = &g_bgnd_scratch_pad_vectors[angle_source];
    }
    angles->x = source->x * angle_scale;
    angles->y = source->y * angle_scale;
    angles->z = source->z * angle_scale;
    z_offset = sfrand(random_z);
    y_offset = sfrand(random_y);
    x_offset = sfrand(random_x);
    angles->x += x_offset;
    angles->y += y_offset;
    angles->z += z_offset;
    velocity->y = vertical_velocity;
    rotate_xz(velocity, velocity, heading);
}
/*
 * Exact object selection, vector transfers, flags, and monitor allocation;
 * 92.66%, exact retail size (632 bytes). Residue is constant-compare lowering,
 * temporary register selection, and equivalent bounded-loop scheduling.
 */
void bgnd_launch_sobj(
    int model_index, unsigned int object_id, unsigned int position_index,
    unsigned int velocity_index, unsigned int angular_velocity_index,
    unsigned int angle_index, unsigned int scale_index, float parameter) {
    BgndSobjLaunchEntry* entry;
    MkObj* model;
    MkSobj* object;
    Vec* vector;
    unsigned int i;

    if (model_index == (int)0xDDDDEEEE) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object == 0) {
            return;
        }
    } else {
        model = g_bgnd_preloaded_models[model_index];
        if (model == 0) {
            return;
        }
        object = obj_find_sobj_by_id(model, object_id);
        if (object == 0) {
            return;
        }
    }
    if (g_sobj_launch_monitor_pdata == 0) {
        return;
    }

    object->flags_08_bits.bit6 = 1;
    if (position_index != 0xFF) {
        vector = &g_bgnd_scratch_pad_vectors[position_index];
        bgnd_copy_vector(&object->pos, vector);
    }
    object->flags_08_bits.bit3 = 1;
    if (angle_index != 0xFF) {
        vector = &g_bgnd_scratch_pad_vectors[angle_index];
        bgnd_copy_vector(&object->ang, vector);
    }
    if (velocity_index != 0xFF) {
        object->flags_08_bits.bit5 = 1;
        vector = &g_bgnd_scratch_pad_vectors[velocity_index];
        bgnd_copy_vector(&object->pos_vel, vector);
    }
    if (angular_velocity_index != 0xFF) {
        object->flags_08_bits.angular_velocity_enabled = 1;
        vector = &g_bgnd_scratch_pad_vectors[angular_velocity_index];
        bgnd_copy_vector(&object->ang_vel, vector);
    }
    if (scale_index != 0xFF) {
        object->flags_08_bits.scale_dirty = 1;
        vector = &g_bgnd_scratch_pad_vectors[scale_index];
        bgnd_copy_vector(&object->scale, vector);
    }

    update_mksobj(object);
    unhide_sobj(object);
    unhide_obj(object->owner);

    entry = 0;
    for (i = 0; i < 25; i++) {
        if (g_sobj_launch_monitor_pdata->entries[i].active == 0) {
            entry = &g_sobj_launch_monitor_pdata->entries[i];
            break;
        }
    }
    if (entry != 0) {
        entry->object = object;
        entry->parameter = parameter;
        entry->active = 1;
        entry->ground_y = g_game_info.field_34;
        g_active_launched_sobj_pdata = entry;
        entry->collision_enabled = 0;
        g_active_launched_sobj_pdata->kill_enabled = 0;
    }
}
/*
 * Exact 25-entry traversal and state updates; 57.23%, retail/local 104/108.
 * Retail retains a byte-offset induction variable while typed C retains an
 * entry pointer; the remaining register and addressing differences are
 * non-algorithmic.
 */
void bgnd_kill_all_launched_sobjs(void) {
    BgndSobjLaunchEntry* entry;
    unsigned int i;

    entry = g_sobj_launch_monitor_pdata->entries;
    i = 0;
    do {
        if (entry->active == 1) {
            hide_sobj(entry->object);
            entry->active = 0;
        }
        entry++;
        i++;
    } while (i < 25);
}
void bgnd_set_collision_plane_for_launched_sobj(
    int test_type, unsigned int function_index, unsigned int argument) {
    if (g_active_launched_sobj_pdata == 0) {
        return;
    }
    if (function_index == 0) {
        return;
    }
    g_active_launched_sobj_pdata->collision_enabled = 1;
    g_active_launched_sobj_pdata->collision_function = function_index;
    g_active_launched_sobj_pdata->collision_arg = argument;
    switch (test_type) {
    case 0:
        g_active_launched_sobj_pdata->collision_test =
            launch_sobj_watch_y_ground_plane;
        break;
    case 1:
        g_active_launched_sobj_pdata->collision_test =
            launch_sobj_watch_dist_from_orgin;
        break;
    }
}
void bgnd_set_kill_plane_for_launched_sobj(int test_type) {
    if (g_active_launched_sobj_pdata == 0) {
        return;
    }
    g_active_launched_sobj_pdata->kill_enabled = 1;
    switch (test_type) {
    case 0:
        g_active_launched_sobj_pdata->kill_test =
            launch_sobj_watch_y_ground_plane;
        break;
    case 2:
        g_active_launched_sobj_pdata->kill_test = launch_sobj_watch_y_far_down;
        break;
    case 1:
        g_active_launched_sobj_pdata->kill_test =
            launch_sobj_watch_dist_from_orgin;
        break;
    }
}
/*
 * Exact creation, initialization, and ownership; 81.79%, retail/local
 * 168/160 bytes. Retail selects an offset-based CTR clear loop while clean
 * typed C selects an entry-pointer countdown.
 */
void start_sobj_launch_monitor(void) {
    BgndSobjLaunchMonitor* monitor;
    BgndSobjLaunchEntry* entry;
    MkProc* process;
    unsigned int i;

    monitor = 0;
    process = _create_mkproc_generic_bigstack(
        0xC019, 0x1F, p_bgnd_launch_sobj_monitor,
        sizeof(BgndSobjLaunchMonitor), (MkHdr**)&monitor);
    if (process != 0 && monitor != 0) {
        entry = monitor->entries;
        i = 25;
        do {
            entry->active = 0;
            entry++;
        } while (--i != 0);
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    g_active_launched_sobj_pdata = 0;
    g_sobj_launch_monitor_pdata = monitor;
}
/*
 * Exact monitor algorithm and calls; 95.78%, retail/local 372/368 bytes.
 * Residue is entry-address induction, nonvolatile register coloring, frame
 * shape, and pooled return-value relocation labels.
 */
static float p_bgnd_launch_sobj_monitor(void) {
    BgndSobjLaunchMonitor* monitor;
    BgndSobjLaunchEntry* entry;
    CmdScript* previous_script;
    CmdScript* script;
    unsigned int i;

    monitor = (BgndSobjLaunchMonitor*)apdata;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    for (i = 0; i < 25; i++) {
        entry = &monitor->entries[i];
        if (entry->active != 0) {
            if (entry->ground_y != g_game_info.field_34) {
                hide_sobj(entry->object);
                entry->active = 0;
            } else if (entry->kill_enabled == 1 &&
                       entry->kill_test(entry, entry->collision_arg) == 1) {
                hide_sobj(entry->object);
                entry->active = 0;
            } else if (entry->collision_enabled == 1 &&
                       entry->collision_test(entry, entry->collision_arg) == 1) {
                script = alloc_cmdscript();
                previous_script = active_cmdscript;
                g_launched_sobj_crossing_plane_pdata = entry;
                active_cmdscript = script;
                cmdscript_setup_execution(g_game_info.cmdscript,
                                          entry->collision_function);
                cmdscript_execute(g_game_info.cmdscript);
                active_cmdscript = previous_script;
                if (script->instance != 0) {
                    ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
                }
                hide_sobj(entry->object);
                entry->active = 0;
            } else {
                entry->object->pos_vel.y += entry->parameter * game_speed;
            }
        }
    }
    return 1.0f;
}
/* 99.93%, exact 276-byte code; only the pooled 0.1f relocation differs. */
void bgnd_chunk_explosion_match_velocity_with_params(
    float velocity_scale, float vertical_velocity,
    float random_vertical_velocity, char* shard_name, int bounce_limit,
    unsigned int spawn_count, unsigned int scale_mode, int motion_mode) {
    Vec* world_position;
    Vec velocity;
    Vec position;
    unsigned int art_id;

    world_position = sobj_get_world_pos(
        g_launched_sobj_crossing_plane_pdata->object);
    bgnd_copy_vector(
        &velocity,
        &g_launched_sobj_crossing_plane_pdata->object->pos_vel);
    bgnd_copy_vector(&position, world_position);
    position.y += 0.1f;
    velocity.x *= velocity_scale;
    velocity.z *= velocity_scale;
    velocity.y = vertical_velocity + frand(random_vertical_velocity);
    art_id = get_artid_of_named_item_in_slot(0x2001E, shard_name, 1);
    start_pfx_glass_shards(art_id, &position, &velocity, bounce_limit,
                           spawn_count, scale_mode, motion_mode);
}
/* 99.72%, exact 72-byte code; only the pooled u32-to-double relocation differs. */
static int launch_sobj_watch_dist_from_orgin(
    BgndSobjLaunchEntry* entry, unsigned int distance_squared) {
    float squared;
    float threshold;
    float z;
    float z_squared;
    float x;
    float x_squared;

    x = entry->object->pos.x;
    x_squared = x * x;
    z = entry->object->pos.z;
    z_squared = z * z;
    squared = x_squared + z_squared;
    threshold = (float)distance_squared;
    return squared > threshold;
}
/* 99.67%, exact 60-byte code; only the pooled -20.0f relocation differs. */
static int launch_sobj_watch_y_far_down(BgndSobjLaunchEntry* entry,
                                         unsigned int unused) {
    Vec* position;

    (void)unused;
    position = sobj_get_world_pos(entry->object);
    return position->y <= -20.0f;
}
/* 99.74%, exact 76-byte code; only the pooled 0.03f relocation differs. */
static int launch_sobj_watch_y_ground_plane(BgndSobjLaunchEntry* entry,
                                             unsigned int unused) {
    Vec* position;

    (void)unused;
    position = sobj_get_world_pos(entry->object);
    return position->y <= g_game_info.field_34 + 0.03f;
}
/*
 * Exact transform, flag, call, and monitor-entry algorithm; 86.42%,
 * retail/local 472/504 bytes. Residue is stmw/lmw selection and typed
 * free-entry loop lowering.
 */
float bgnd_launch_chunk(
    MkSobj* object, const Vec* position, const Vec* velocity,
    const Vec* angular_velocity, const Vec* angles, unsigned int end_mode,
    const Vec* scale, int field_0C, float vertical_accel, int field_10) {
    BgndChunkLaunchEntry* entry;
    unsigned int i;

    if (g_chunk_launch_monitor_pdata == 0) {
        return 0.0f;
    }
    if (position == 0) {
        return 0.0f;
    }
    object->flags_08_bits.bit6 = 1;
    bgnd_copy_vector(&object->pos, position);
    if (velocity != 0) {
        object->flags_08_bits.bit5 = 1;
        bgnd_copy_vector(&object->pos_vel, velocity);
    }
    if (angular_velocity != 0) {
        object->flags_08_bits.angular_velocity_enabled = 1;
        bgnd_copy_vector(&object->ang_vel, angular_velocity);
    }
    if (angles != 0) {
        object->flags_08_bits.bit3 = 1;
        bgnd_copy_vector(&object->ang, angles);
    }
    if (scale != 0) {
        object->flags_08_bits.scale_dirty = 1;
        bgnd_copy_vector(&object->scale, scale);
    }
    update_mksobj(object);
    unhide_sobj(object);
    unhide_obj(object->owner);

    entry = 0;
    for (i = 0; i < 25; i++) {
        if (g_chunk_launch_monitor_pdata->entries[i].active == 0) {
            entry = &g_chunk_launch_monitor_pdata->entries[i];
            break;
        }
    }
    if (entry != 0) {
        entry->object = object;
        entry->vertical_accel = vertical_accel;
        entry->end_mode = end_mode;
        entry->field_0C = field_0C;
        entry->field_10 = field_10;
        entry->active = 1;
        entry->ground_y = g_game_info.field_34;
    }
    return 0.0f;
}
/*
 * Exact creation, initialization, and ownership; 81.38%, retail/local
 * 160/152 bytes. Retail selects an offset-based CTR clear loop while clean
 * typed C selects an entry-pointer countdown.
 */
void start_chunk_launch_monitor(void) {
    BgndChunkLaunchMonitor* monitor;
    BgndChunkLaunchEntry* entry;
    MkProc* process;
    unsigned int i;

    monitor = 0;
    process = _create_mkproc_generic_bigstack(
        0xC018, 0x1F, p_bgnd_launch_chunk_monitor,
        sizeof(BgndChunkLaunchMonitor), (MkHdr**)&monitor);
    if (process != 0 && monitor != 0) {
        entry = monitor->entries;
        i = 25;
        do {
            entry->active = 0;
            entry++;
        } while (--i != 0);
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    g_chunk_launch_monitor_pdata = monitor;
}
/*
 * Exact monitor branches, effects, sounds, and motion; 86.76%, retail/local
 * 460/464 bytes. Residue is typed-entry induction, register coloring,
 * scheduling, and pooled constant/string relocation labels.
 */
static float p_bgnd_launch_chunk_monitor(void) {
    BgndChunkLaunchMonitor* monitor;
    BgndChunkLaunchEntry* entry;
    int effect;
    unsigned int i;
    float x;
    float y;
    float z;

    monitor = (BgndChunkLaunchMonitor*)apdata;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    for (i = 0; i < 25; i++) {
        entry = &monitor->entries[i];
        if (entry->active != 0) {
            if (entry->ground_y != g_game_info.field_34) {
                hide_sobj(entry->object);
                entry->active = 0;
            } else {
                if (entry->object->pos.y <=
                    g_game_info.field_34 + 0.03f) {
                    if (entry->end_mode == 0) {
                        if (entry->object->pos.z > 60.0f) {
                            return -1.0f;
                        }
                        frand(0.1f);
                        frand(0.6f);
                        frand(0.9f);
                        if ((unsigned short)randu0(100) < 50) {
                            snd_req(0x92);
                        } else {
                            snd_req(0x93);
                        }
                        x = entry->object->pos.x;
                        y = entry->object->pos.y + 0.5f;
                        z = entry->object->pos.z;
                        effect = fx_by_owner("brick_piece_explosion", 4);
                        effect = fx_next_emitter(effect);
                        if (effect != 0) {
                            mk_chess_launch_fx_at_pos_with_obj_emit_based(
                                effect, x, y, z);
                        }
                        hide_sobj(entry->object);
                        entry->active = 0;
                    } else if (entry->end_mode == 1) {
                        entry->active = 0;
                    }
                } else {
                    entry->object->pos_vel.y +=
                        entry->vertical_accel * game_speed;
                }
            }
        }
    }
    return 1.0f;
}
/* Retail spells the two invalid cases as separate nulling branches. */
MkHdr* get_sobj_pebble_obj(MkSobj* object) {
    MkHdr* bound;

    bound = object->bound_hdr;
    if (bound != 0 && bound->instance != object->bound_instance) {
        bound = 0;
    }
    return bound;
}
void* get_general_pebble_data(PebbleData* pebble_data) {
    return pebble_data->user_data;
}
/* Exact: typed process payload, mode dispatch, initialization, and ownership. */
BgndPebbleMonitor* ncs_create_pebble_monitor_proc(
    MkSobj* object, PebbleData* pebble_data, int mode, int count) {
    BgndPebbleMonitor* monitor;
    MkProc* process;

    process = 0;
    monitor = 0;
    switch (mode) {
    case 0:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_path_monitor,
            sizeof(BgndPebbleMonitor), (MkHdr**)&monitor);
        break;
    case 1:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_burst_monitor,
            sizeof(BgndPebbleMonitor), (MkHdr**)&monitor);
        break;
    case 2:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_manual_monitor,
            sizeof(BgndPebbleMonitor), (MkHdr**)&monitor);
        break;
    }
    if (process == 0) {
        return 0;
    }
    monitor->object = object;
    monitor->count = count;
    monitor->mode = mode;
    monitor->pebble_data = pebble_data;
    set_process_as_scriptable(process);
    mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
    return monitor;
}
void ncs_set_pebble_pos(PebbleData* data, int index, Vec* position) {
    MKMatrixTranslate(&data->pebbles[index].matrix, position, 0);
}
/*
 * Exact constructor and initialization; 99.44%, retail/local 156/156 bytes.
 * The only residue is scheduling of the two independent loop increments.
 */
PebbleData* ncs_create_pebbles_with_sobj(MkSobj* object,
                                         unsigned int count) {
    PebbleData* pebble_data;
    unsigned int i;

    object->flags_08_bits.bit6 = 1;
    object->z_offset = 0.0f;
    object->flags09_bits.bit4 = 1;
    object->flags09_bits.bit3 = 1;
    pebble_data = create_pebble_userdata(
        object, count, sizeof(BgndPebbleControl));
    for (i = 0; i < count; i++) {
        MKMatrixTranslate(&pebble_data->pebbles[i].matrix, &object->pos, 0);
    }
    return pebble_data;
}
void pebble_turn_culling_off(int player) {
    MkSobj* object;

    object = g_pebbles_pdata[player]->sobj;
    if (object != 0) {
        object->flags09_bits.bit3 = 1;
    }
}
void pebble_turn_culling_on(int player) {
    MkSobj* object;

    object = g_pebbles_pdata[player]->sobj;
    if (object != 0) {
        object->flags09_bits.bit3 = 0;
    }
}
void pebble_unhide_me(int player, int index) {
    g_pebbles_pdata[player]->collection->pebbles[index].state = 2;
}
void pebble_hide_me(int player, int index) {
    g_pebbles_pdata[player]->collection->pebbles[index].state = 0;
}
void pebble_setup_bounce_props(int player, int index, Vec* velocity,
                               int flags) {
    BgndPebbleControl* pebble;
    BgndPebbleControl* pebbles;

    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];
    pebble->bounce_velocity.x = velocity->x;
    pebble->bounce_velocity.y = velocity->y;
    pebble->bounce_velocity.z = velocity->z;
    pebble->bounce_flags = flags;
}
/* Soft ceiling: 99.74% -- only the 57.295776f pool relocation label differs. */
void pebble_set_ang_vel(int player, int index, Vec* velocity) {
    BgndPebbleControl* pebble;

    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];
    pebble->angular_velocity.x = 57.295776f * velocity->x;
    pebble->angular_velocity.y = 57.295776f * velocity->y;
    pebble->angular_velocity.z = 57.295776f * velocity->z;
}
void pebble_set_ang(int player, int index, Vec* angles) {
    BgndPebbleControl* pebble;

    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];
    pebble->angles.x = angles->x;
    pebble->angles.y = angles->y;
    pebble->angles.z = angles->z;
}
void pebble_set_scale(int player, int index, Vec* scale) {
    BgndPebbleControl* pebble;

    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];
    pebble->scale.x = scale->x;
    pebble->scale.y = scale->y;
    pebble->scale.z = scale->z;
}
void pebble_set_vel(int player, int index, Vec* velocity) {
    BgndPebbleControl* pebble;

    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];
    pebble->velocity.x = velocity->x;
    pebble->velocity.y = velocity->y;
    pebble->velocity.z = velocity->z;
}
/* Exact-size typed copy; residue is indexed first access vs pre-added base. */
void pebble_get_pos(int player, int index, Vec* position) {
    BgndPebbleControl* pebbles;
    BgndPebbleControl* pebble;

    pebbles = g_pebbles_pdata[player]->collection->pebbles;
    position->x = pebbles[index].position.x;
    pebble = &pebbles[index];
    position->y = pebble->position.y;
    position->z = pebble->position.z;
}
/* Exact-size typed copy; residue is indexed first access vs pre-added base. */
void pebble_set_pos(int player, int index, Vec* position) {
    BgndPebbleControl* pebbles;
    BgndPebbleControl* pebble;

    pebbles = g_pebbles_pdata[player]->collection->pebbles;
    pebbles[index].position.x = position->x;
    pebble = &pebbles[index];
    pebble->position.y = position->y;
    pebble->position.z = position->z;
}
void bgnd_pebble_burst_at_pos(int unused, int first, int second, void* script,
                              float x, float y, float z) {
    Vec position;

    (void)unused;
    (void)script;
    position.x = x;
    position.y = y;
    position.z = z;
    bgnd_pebble_burst_at(unused, &position, first, second);
}
void bgnd_pebble_burst_at_pebble_pos(int unused, int first, int second) {
    bgnd_pebble_burst_at(unused, 0, first, second);
}
void bgnd_pebble_burst_at_chunk_pos(int player, int first, int end) {
    MkSobj* object;
    Vec position;

    object = g_launched_sobj_crossing_plane_pdata->object;
    position.x = object->pos.x;
    position.y = 0.11f;
    position.z = object->pos.z;
    bgnd_pebble_burst_at(player, &position, first, end);
}
/*
 * Soft ceiling: 69.79%, retail/local 112/104 bytes. The typed do-loop emits
 * the same bounded stores with a compare loop; retail selects CTR iteration.
 */
void bgnd_pebble_burst_set_end_state(int player, unsigned int first,
                                     unsigned int end, int state) {
    BgndPebbleCollection* collection;
    unsigned int index;

    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection != 0) {
        index = first;
        do {
            collection->pebbles[index].end_behavior = state;
            index++;
        } while (index < end);
    }
}
/*
 * Soft ceiling: 84.88%, retail/local 384/364 bytes. Selector mapping, radial
 * randomization, and stores agree; typed iteration shortens offset scheduling.
 */
void bgnd_pebble_burst_set_value_min_max(int player, unsigned int first,
                                         unsigned int end, int field,
                                         void* script, float minimum,
                                         float maximum) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebble;
    Vec* value;
    Vec direction;
    float magnitude;
    unsigned int index;

    (void)script;
    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection == 0 || minimum > maximum) {
        return;
    }

    pebble = &collection->pebbles[first];
    for (index = first; index < end; index++, pebble++) {
        switch (field) {
        case 0:
            value = &pebble->position;
            break;
        case 1:
            value = &pebble->velocity;
            break;
        case 2:
            value = &pebble->angular_velocity;
            break;
        case 3:
            value = &pebble->bounce_velocity;
            break;
        case 4:
            value = &pebble->angles;
            break;
        default:
            continue;
        }

        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        magnitude = minimum + frand(maximum - minimum);
        uv_from_angle_y(&direction, frand(6.2831855f));
        value->x = direction.x * magnitude;
        value->z = direction.z * magnitude;
    }
}
static inline void bgnd_set_randomized_component(float* destination,
                                                  float value, float spread) {
    if (value != 555599.6f) {
        if (spread < 0.0f) {
            *destination = value + frand(-spread) - (-0.5f * spread);
        } else {
            *destination = value + frand(spread);
        }
    }
}

/*
 * Soft ceiling: 93.14%, retail/local 676/664 bytes. Remaining differences are
 * loop induction and scheduling around the three inlined component updates.
 */
void bgnd_pebble_burst_set_value(int player, unsigned int first,
                                 unsigned int end, int field, void* script,
                                 float x, float y, float z, float x_spread,
                                 float y_spread, float z_spread) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebble;
    Vec* value;
    unsigned int index;

    (void)script;
    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection == 0) {
        return;
    }

    pebble = &collection->pebbles[first];
    for (index = first; index < end; index++, pebble++) {
        switch (field) {
        case 0:
            value = &pebble->position;
            break;
        case 1:
            value = &pebble->velocity;
            break;
        case 2:
            value = &pebble->angular_velocity;
            break;
        case 3:
            value = &pebble->bounce_velocity;
            break;
        case 4:
            value = &pebble->angles;
            break;
        default:
            continue;
        }

        bgnd_set_randomized_component(&value->x, x, x_spread);
        bgnd_set_randomized_component(&value->y, y, y_spread);
        bgnd_set_randomized_component(&value->z, z, z_spread);
    }
}
/*
 * Soft ceiling: 91.41%, 724/728 bytes. Retail keeps a separate 0x70-byte
 * offset induction variable; typed array iteration folds it into a pointer.
 * The bounds, two loops, randomization, matrix calls, and stores are intact.
 */
static void bgnd_pebble_burst_at(int player, const Vec* position,
                                 unsigned int first, unsigned int end) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    BgndPebbleControl* pebble;
    unsigned int index;

    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection == 0) {
        return;
    }
    pebbles = collection->pebbles;

    for (index = first; index < end; index++) {
        pebble = &pebbles[index];
        if (position != 0) {
            pebble->position.x = position->x + sfrand(0.1f);
            pebble->position.y = position->y + sfrand(0.05f);
            pebble->position.z = position->z + sfrand(0.1f);
        }

        pebble->angular_velocity.z = 0.0f;
        pebble->angular_velocity.y = 0.0f;
        pebble->angular_velocity.x = 0.0f;
        pebble->velocity.z = 0.0f;
        pebble->velocity.y = 0.0f;
        pebble->velocity.x = 0.0f;
        pebble->bounce_flags = 3;

        if (position != 0) {
            pebble->angles.z = 0.0f;
            pebble->angles.y = 0.0f;
            pebble->angles.x = 0.0f;
            pebble->angles.x = sfrand(360.0f);
            pebble->angles.y = sfrand(180.0f);
            pebble->angles.z = 0.0f;
        }

        pebble->angular_velocity.x = 8.0f + sfrand(3.0f);
        pebble->angular_velocity.y = 4.0f + frand(6.0f);
        pebble->angular_velocity.z = 0.0f;

        if (position != 0) {
            pebble->velocity.x = pebble->position.x - position->x;
            pebble->velocity.y = pebble->position.y - position->y;
            pebble->velocity.z = pebble->position.z - position->z;
            pebble->velocity.x *= 0.35f + frand(0.5f);
            pebble->velocity.y *= 0.35f + frand(0.5f);
            pebble->velocity.z *= 0.35f + frand(0.5f);
        } else {
            pebble->velocity.z = 0.0f;
            pebble->velocity.y = 0.0f;
            pebble->velocity.x = 0.0f;
        }

        pebble->velocity.y = 0.05f + frand(0.1f);
        pebble->state = 2;
        pebble->end_behavior = 4;
        pebble->bounce_velocity.x = 0.0f;
        pebble->bounce_velocity.z = 0.0f;
        pebble->bounce_velocity.y = 0.5f;
    }

    for (index = first; index < end; index++) {
        pebble = &pebbles[index];
        MKMatrixRotatXZYScaleTranslate(
            &g_pebbles[player]->matrices[index], pebble->angles.x,
            pebble->angles.z, pebble->angles.y, &pebble->scale,
            &pebble->position);
    }
    unhide_sobj(g_pebbles_pdata[player]->sobj);
}
/* Exact-size 99.70% near miss; residue is jump-table relocation labeling. */
float bgnd_pebble_fetch_current_info(unsigned int field) {
    switch (field) {
    case 0:
        return g_current_pebble->velocity.x;
    case 1:
        return g_current_pebble->velocity.y;
    case 2:
        return g_current_pebble->velocity.z;
    case 3:
        return g_current_pebble->angular_velocity.x;
    case 4:
        return g_current_pebble->angular_velocity.y;
    case 5:
        return g_current_pebble->angular_velocity.z;
    case 6:
        return g_current_pebble->angles.x;
    case 7:
        return g_current_pebble->angles.y;
    case 8:
        return g_current_pebble->angles.z;
    case 9:
        return g_current_pebble->position.x;
    case 10:
        return g_current_pebble->position.y;
    case 11:
        return g_current_pebble->position.z;
    case 12:
        return g_current_pebble->scale.x;
    case 13:
        return g_current_pebble->scale.y;
    case 14:
        return g_current_pebble->scale.z;
    case 15:
        return (float)g_current_pebble->state;
    default:
        return 0.0f;
    }
}
/* Exact-size 99.76% near miss; residue is jump-table relocation labeling. */
void bgnd_pebble_set_current_info(unsigned int field, void* script,
                                  float value) {
    (void)script;
    switch (field) {
    case 0:
        g_current_pebble->velocity.x = value;
        break;
    case 1:
        g_current_pebble->velocity.y = value;
        break;
    case 2:
        g_current_pebble->velocity.z = value;
        break;
    case 3:
        g_current_pebble->position.x = value;
        break;
    case 4:
        g_current_pebble->position.y = value;
        break;
    case 5:
        g_current_pebble->position.z = value;
        break;
    case 6:
        g_current_pebble->angles.x = value;
        break;
    case 7:
        g_current_pebble->angles.y = value;
        break;
    case 8:
        g_current_pebble->angles.z = value;
        break;
    case 9:
        g_current_pebble->scale.x = value;
        break;
    case 10:
        g_current_pebble->scale.y = value;
        break;
    case 11:
        g_current_pebble->scale.z = value;
        break;
    case 12:
        g_current_pebble->angular_velocity.x = value;
        break;
    case 13:
        g_current_pebble->angular_velocity.y = value;
        break;
    case 14:
        g_current_pebble->angular_velocity.z = value;
        break;
    case 15:
        g_current_pebble->state = (unsigned int)value;
        break;
    }
}
void bgnd_pebble_set_current_pebble(int player, int index) {
    g_current_pebble =
        &g_pebbles_pdata[player]->collection->pebbles[index];
}
void bgnd_pebble_change_current_end_behavior(int end_behavior) {
    g_current_pebble->end_behavior = end_behavior;
}
/* Exact-size near miss: 98.60%; only pooled-float relocation labeling remains. */
void bgnd_pebble_change_current_behavior_to_bounce(
    unsigned int ticks, int bounce_param, float velocity_x, float velocity_y,
    float velocity_z, float angular_x, float angular_y, float angular_z) {
    if (velocity_x != 555999.6f) {
        g_current_pebble->velocity.x = velocity_x;
    }
    if (velocity_y != 555999.6f) {
        g_current_pebble->velocity.y = velocity_y;
    }
    if (velocity_z != 555999.6f) {
        g_current_pebble->velocity.z = velocity_z;
    }
    if (angular_x != 555999.6f) {
        g_current_pebble->angular_velocity.x = angular_x;
    }
    if (angular_y != 555999.6f) {
        g_current_pebble->angular_velocity.y = angular_y;
    }
    if (angular_z != 555999.6f) {
        g_current_pebble->angular_velocity.z = angular_z;
    }

    g_current_pebble->bounce_flags = 3;
    g_current_pebble->state = 7;
    g_current_pebble->bounce_ticks =
        (unsigned int)((float)ticks * inverse_game_speed);
    if (g_current_pebble->bounce_ticks == 0) {
        g_current_pebble->bounce_ticks = 1;
    }
    g_current_pebble->bounce_param = bounce_param;
    g_current_pebble->end_behavior = 4;
}
/* Exact-size near miss: 98.54%; only pooled-float relocation labeling remains. */
void bgnd_pebble_change_current_behavior(
    unsigned int ticks, int behavior_param, float velocity_x, float velocity_y,
    float velocity_z, float angular_x, float angular_y, float angular_z) {
    if (velocity_x != 555999.6f) {
        g_current_pebble->velocity.x = velocity_x;
    }
    if (velocity_y != 555999.6f) {
        g_current_pebble->velocity.y = velocity_y;
    }
    if (velocity_z != 555999.6f) {
        g_current_pebble->velocity.z = velocity_z;
    }
    if (angular_x != 555999.6f) {
        g_current_pebble->angular_velocity.x = angular_x;
    }
    if (angular_y != 555999.6f) {
        g_current_pebble->angular_velocity.y = angular_y;
    }
    if (angular_z != 555999.6f) {
        g_current_pebble->angular_velocity.z = angular_z;
    }

    g_current_pebble->state = 6;
    g_current_pebble->bounce_ticks =
        (unsigned int)((float)ticks * inverse_game_speed);
    if (g_current_pebble->bounce_ticks == 0) {
        g_current_pebble->bounce_ticks = 1;
    }
    g_current_pebble->bounce_param = behavior_param;
    g_current_pebble->end_behavior = 4;
}
/*
 * Soft ceiling: 94.90%, retail/local 468/460 bytes. The launch state and matrix
 * update agree; residue is nonvolatile save grouping and store scheduling.
 */
void bgnd_pebble_launch_at_time(
    int player, int index, unsigned int delay, int behavior_param,
    float position_x, float position_y, float position_z, float scale_x,
    float scale_y, float scale_z, float angle_x, float angle_y, float angle_z) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebble;

    unhide_sobj(g_pebbles_pdata[player]->sobj);
    collection = g_pebbles_pdata[player]->collection;
    pebble = &collection->pebbles[index];

    pebble->angular_velocity.z = 0.0f;
    pebble->angular_velocity.y = 0.0f;
    pebble->angular_velocity.x = 0.0f;
    pebble->velocity.z = 0.0f;
    pebble->velocity.y = 0.0f;
    pebble->velocity.x = 0.0f;
    pebble->state = 3;
    pebble->end_behavior = 4;
    pebble->bounce_ticks = 0;
    pebble->launch_ticks =
        (unsigned int)((float)delay * inverse_game_speed);
    if (pebble->launch_ticks == 0) {
        pebble->launch_ticks = 1;
    }

    pebble->position.x = position_x;
    pebble->position.y = position_y;
    pebble->position.z = position_z;
    pebble->scale.x = scale_x;
    pebble->scale.y = scale_y;
    pebble->scale.z = scale_z;
    pebble->angles.x = angle_x;
    pebble->angles.y = angle_y;
    pebble->angles.z = angle_z;
    pebble->bounce_param = behavior_param;

    MKMatrixRotatXZYScaleTranslate(&collection->matrices[index],
                                   pebble->angles.x, pebble->angles.z,
                                   pebble->angles.y, &pebble->scale,
                                   &pebble->position);
}
/*
 * Soft ceiling: 85.98%, retail/local 200/216 bytes. All state writes and timing
 * conversion agree; residue is lifetime coloring and stmw/lmw selection.
 */
void bgnd_pebble_simple_launch_at_time(int player, int index,
                                       unsigned int delay,
                                       int behavior_param) {
    BgndPebblePlayerData* player_data;
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebble;

    player_data = g_pebbles_pdata[player];
    collection = player_data->collection;
    pebble = &collection->pebbles[index];
    unhide_sobj(player_data->sobj);

    pebble->angular_velocity.z = 0.0f;
    pebble->angular_velocity.y = 0.0f;
    pebble->angular_velocity.x = 0.0f;
    pebble->velocity.z = 0.0f;
    pebble->velocity.y = 0.0f;
    pebble->velocity.x = 0.0f;
    pebble->state = 3;
    pebble->bounce_ticks = 0;
    pebble->launch_ticks =
        (unsigned int)((float)delay * inverse_game_speed);
    if (pebble->launch_ticks == 0) {
        pebble->launch_ticks = 1;
    }
    pebble->bounce_param = behavior_param;
    pebble->end_behavior = 4;
}
/*
 * Near match: 96.03%, retail/local 728/728 bytes. Remaining differences are
 * nonvolatile register allocation and equivalent loop-induction scheduling.
 */
BgndPebbleControl* bgnd_create_pebbles_with_sobj(
    MkSobj* object, unsigned int player, int mode, unsigned int count) {
    BgndPebbleCollection* collection;
    BgndPebblePlayerData* player_data;
    BgndPebblePlayerData* result;
    BgndPebbleControl* pebble;
    MkProc* process;
    unsigned int index;

    if (player >= 20) {
        return 0;
    }
    object->flags_08_bits.angular_velocity_enabled = 1;
    object->z_offset = 0.0f;
    object->flags09_bits.bit4 = 1;
    object->flags09_bits.bit3 = 1;

    collection = (BgndPebbleCollection*)create_pebble_userdata(
        object, count, sizeof(BgndPebbleControl));
    for (index = 0; index < count; index++) {
        MKMatrixTranslate(
            &collection->matrices[index], &object->pos, 0);
    }
    g_pebbles[player] = collection;
    if (g_pebbles[player] == 0) {
        return 0;
    }
    hide_sobj(object);

    for (index = 0; index < count; index++) {
        pebble = &g_pebbles[player]->pebbles[index];
        pebble->position.z = 0.0f;
        pebble->position.y = 0.0f;
        pebble->position.x = 0.0f;
        pebble->scale.z = 1.0f;
        pebble->scale.y = 1.0f;
        pebble->scale.x = 1.0f;
        pebble->angles.z = 0.0f;
        pebble->angles.y = 0.0f;
        pebble->angles.x = 0.0f;
        pebble->angular_velocity.z = 0.0f;
        pebble->angular_velocity.y = 0.0f;
        pebble->angular_velocity.x = 0.0f;
        pebble->velocity.z = 0.0f;
        pebble->velocity.y = 0.0f;
        pebble->velocity.x = 0.0f;
        pebble->bounce_flags = 0;
        pebble->state = 0;
        pebble->bounce_ticks = 0;
        pebble->launch_ticks = 0;
        pebble->gravity = 0.0f;
        pebble->bounce_velocity.x = 0.0f;
        pebble->bounce_velocity.z = 0.0f;
        pebble->bounce_velocity.y = 0.5f;
        MKMatrixRotatXZYScaleTranslate(
            &g_pebbles[player]->matrices[index],
            pebble->angles.x, pebble->angles.z, pebble->angles.y,
            &pebble->scale, &pebble->position);
    }

    process = 0;
    player_data = 0;
    switch (mode) {
    case 0:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_path_monitor,
            sizeof(BgndPebblePlayerData), (MkHdr**)&player_data);
        break;
    case 1:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_burst_monitor,
            sizeof(BgndPebblePlayerData), (MkHdr**)&player_data);
        break;
    case 2:
        process = _create_mkproc_generic_tinystack(
            0xC014, 0x2E, p_pebble_manual_monitor,
            sizeof(BgndPebblePlayerData), (MkHdr**)&player_data);
        break;
    }
    if (process == 0) {
        result = 0;
    } else {
        player_data->sobj = object;
        player_data->count = count;
        player_data->mode = mode;
        player_data->collection = collection;
        set_process_as_scriptable(process);
        mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        result = player_data;
    }
    g_pebbles_pdata[player] = result;
    return result->collection->pebbles;
}

void bgnd_create_pebbles(
    int model_index, unsigned int object_id, unsigned int player, int mode,
    unsigned int count) {
    MkSobj* object;

    switch (model_index) {
    case -572657938:
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object == 0) {
            object = 0;
        }
        break;
    default:
        unhide_obj(g_bgnd_preloaded_models[model_index]);
        if (g_bgnd_preloaded_models[model_index] == 0) {
            object = 0;
        } else {
            object = obj_find_sobj_by_id(
                g_bgnd_preloaded_models[model_index], object_id);
            if (object == 0) {
                object = obj_first_sobj(g_bgnd_preloaded_models[model_index]);
                if (object == 0) {
                    object = 0;
                }
            }
        }
        break;
    }
    bgnd_create_pebbles_with_sobj(object, player, mode, count);
}
/*
 * Clean-C ceiling: 73.83%, retail/local 212/184 bytes. Retail retains explicit
 * null-normalization branches that MWCC folds from the typed selection logic.
 */
void bgnd_set_material_color(int model_index, unsigned int object_id,
                             unsigned char red, unsigned char green,
                             unsigned char blue, unsigned char alpha) {
    MkObj* model;
    MkSobj* object;
    RwRGBA color;

    if (model_index == (int)0xDDDDEEEE) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    } else {
        model = g_bgnd_preloaded_models[model_index];
        unhide_obj(model);
        if (model == 0) {
            object = 0;
        } else {
            object = obj_find_sobj_by_id(model, object_id);
            if (object == 0) {
                object = obj_first_sobj(model);
            }
        }
    }
    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = alpha;
    set_atomic_material_color(object->atomic, &color);
}

/*
 * Clean-C ceiling: 50.02%, retail/local 168/148 bytes. The low fuzzy score is
 * alignment fallout from folded null-normalization branches; calls and CFG
 * outcomes match the immediately preceding retail selection pattern.
 */
MkSobj* bgnd_fetch_sobj(int model_index, unsigned int object_id) {
    MkObj* model;
    MkSobj* object;

    if (model_index == (int)0xDDDDEEEE) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    } else {
        model = g_bgnd_preloaded_models[model_index];
        unhide_obj(model);
        if (model == 0) {
            object = 0;
        } else {
            object = obj_find_sobj_by_id(model, object_id);
            if (object == 0) {
                object = obj_first_sobj(model);
            }
        }
    }
    return object;
}
MkObj* bgnd_fetch_obj(int model_id) {
    return g_bgnd_preloaded_models[model_id];
}
void bgnd_unhide_pebbles(int player) {
    unhide_sobj(g_pebbles_pdata[player]->sobj);
}
/*
 * Soft ceiling: 85.02%, retail/local 172/168 bytes. Typed matrix/pebble indexing
 * differs from retail byte-offset induction; all loop stores and calls agree.
 */
void bgnd_hide_pebbles(int player) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebble;
    unsigned int index;

    collection = g_pebbles[player];
    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++) {
        pebble = &collection->pebbles[index];
        pebble->state = 0;
        pebble->position.y = -200.0f;
        MKMatrixTranslate(&collection->matrices[index], &pebble->position, 0);
    }
    hide_sobj(g_pebbles_pdata[player]->sobj);
}
/*
 * Clean-C ceiling: 68.31%, retail/local 168/184 bytes. Retail keeps separate
 * byte-offset/index lifetimes and uses stmw/lmw; the typed pointer loop agrees.
 */
void bgnd_pebble_rand_scale(int player, void* script, float base,
                            float range) {
    BgndPebbleControl* pebble;
    float scale;
    unsigned int index;

    (void)script;
    pebble = g_pebbles[player]->pebbles;
    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++, pebble++) {
        scale = base + frand(range);
        pebble->scale.z = scale;
        pebble->scale.y = scale;
        pebble->scale.x = scale;
    }
}
/*
 * Exact-size typed loop. Retail hoists the userdata base and advances a byte
 * offset; typed indexing reloads the stable base and colors the loop GPRs.
 */
void bgnd_pebble_gravity(int player, float gravity) {
    unsigned int index;

    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++) {
        g_pebbles[player]->pebbles[index].gravity = gravity;
    }
}
void bgnd_init_pebbles(void) {}
/*
 * Exact state gating, integration, and matrix update; 94.61%, retail/local
 * 288/280 bytes. Retail uses independent 0x70 control and 0x40 matrix byte
 * offsets; clean typed C selects pointer induction and different coloring.
 */
static float p_pebble_manual_monitor(void) {
    BgndPebbleMonitor* monitor;
    BgndPebbleControl* pebble;
    BgndPebbleCollection* collection;
    unsigned int i;

    monitor = (BgndPebbleMonitor*)pdata_of_proc(aproc);
    collection = (BgndPebbleCollection*)monitor->pebble_data;
    pebble = (BgndPebbleControl*)collection->pebbles;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    if (mode_of_play != 9 && get_game_state() != 5 &&
        (g_game_info.plyr0.slot.mirror_a == 0 ||
         g_game_info.plyr1.slot.mirror_a == 0)) {
        return 1.0f;
    }
    for (i = 0; i < (unsigned int)monitor->count; i++, pebble++) {
        v3_add_v3_scaled(&pebble->position, &pebble->position,
                         &pebble->velocity, game_speed);
        pebble->velocity.y += game_speed * pebble->gravity;
        v3_add_v3_scaled(&pebble->angles, &pebble->angles,
                         &pebble->angular_velocity, game_speed);
        MKMatrixRotatXZYScaleTranslate(
            &collection->matrices[i], pebble->angles.x, pebble->angles.z,
            pebble->angles.y, &pebble->scale, &pebble->position);
    }
    return 1.0f;
}
/* Soft ceiling 98.60%: typed array induction and register coloring. */
static float p_pebble_burst_monitor(void) {
    BgndPebbleMonitor* monitor;
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    BgndPebbleControl* pebble;
    unsigned int i;
    unsigned int end_state;
    int any_active;

    monitor = (BgndPebbleMonitor*)pdata_of_proc(aproc);
    collection = (BgndPebbleCollection*)monitor->pebble_data;
    pebbles = collection->pebbles;
    any_active = 0;
    if (g_game_info.bgnd_obj == 0 || pebbles == 0) {
        return -1.0f;
    }
    if (is_sobj_hidden(monitor->object)) {
        return 1.0f;
    }
    if (mode_of_play != 9 &&
        (g_game_info.plyr0.slot.mirror_a == 0 ||
         g_game_info.plyr1.slot.mirror_a == 0)) {
        return 1.0f;
    }

    for (i = 0; i < (unsigned int)monitor->count; i++) {
        pebble = &pebbles[i];
        if (pebble->state == 1) {
            continue;
        }
        any_active = 1;
        if (pebble->angles.x > 360.0f) {
            pebble->angles.x -= 360.0f;
        }
        if (pebble->angles.x < -360.0f) {
            pebble->angles.x += 360.0f;
        }
        if (pebble->angles.y > 360.0f) {
            pebble->angles.y -= 360.0f;
        }
        if (pebble->angles.y < -360.0f) {
            pebble->angles.y += 360.0f;
        }
        if (pebble->angles.z > 360.0f) {
            pebble->angles.z -= 360.0f;
        }
        if (pebble->angles.z < -360.0f) {
            pebble->angles.z += 360.0f;
        }

        if (pebble->state == 0 || pebble->state == 10) {
            pebble->position.z = 0.0f;
            pebble->position.y = 0.0f;
            pebble->position.x = 0.0f;
            pebble->position.y = -200.0f;
            MKMatrixTranslate(&collection->matrices[i], &pebble->position, 0);
            if (pebble->state == 10) {
                pebble->state = 1;
            }
        } else if (pebble->state == 4 || pebble->state == 12) {
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
            if (pebble->state == 12) {
                pebble->state = 1;
            }
        } else if (pebble->state == 9) {
            pebble->angles.x += pebble->angular_velocity.x;
            pebble->angles.y += pebble->angular_velocity.y;
            pebble->angles.z += pebble->angular_velocity.z;
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
        } else if (pebble->state == 8 || pebble->state == 13) {
            if (pebble->angles.x > 180.0f) {
                pebble->angles.x *= 1.3f;
            } else {
                pebble->angles.x *= 0.7f;
            }
            if (pebble->angles.z > 180.0f) {
                pebble->angles.z *= 1.3f;
            } else {
                pebble->angles.z *= 0.7f;
            }
            if (pebble->angles.x < 10.0f ||
                (pebble->angles.x > 350.0f && pebble->angles.z < 10.0f) ||
                pebble->angles.z > 350.0f) {
                end_state = 4;
                pebble->angles.x = 0.0f;
                pebble->angles.z = 0.0f;
                if (pebble->state == 13) {
                    end_state = 1;
                }
                pebble->state = end_state;
            }
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
        } else {
            v3_add_v3_scaled(&pebble->position, &pebble->position,
                             &pebble->velocity, game_speed);
            pebble->velocity.y += -0.003f * game_speed;
            v3_add_v3_scaled(&pebble->angles, &pebble->angles,
                             &pebble->angular_velocity, game_speed);
            if (pebble->bounce_flags == 0) {
                if (pebble->position.y < g_game_info.field_34 + 0.1f) {
                    pebble->position.y = g_game_info.field_34 + 0.025f;
                }
                pebble->velocity.x = 0.96f * pebble->velocity.x;
                pebble->velocity.y = 0.96f * pebble->velocity.y;
                pebble->velocity.z = 0.96f * pebble->velocity.z;
                pebble->velocity.y = 0.0f;
                pebble->angular_velocity.z = 0.0f;
                pebble->angular_velocity.y = 0.0f;
                pebble->angular_velocity.x = 0.0f;
                if (pebble->state == 8 || pebble->state == 13) {
                    pebble->angles.x *= 0.8f;
                }
                if (pebble->velocity.x * pebble->velocity.x +
                        pebble->velocity.y * pebble->velocity.y +
                        pebble->velocity.z * pebble->velocity.z <
                    0.001f) {
                    pebble->state = pebble->end_behavior;
                }
            } else if (pebble->position.y <
                           g_game_info.field_34 + 0.025f &&
                       pebble->velocity.y < 0.0f) {
                if (--pebble->bounce_flags > 0) {
                    pebble->velocity.y *= -1.0f * pebble->bounce_velocity.y;
                    pebble->velocity.x +=
                        frand(pebble->bounce_velocity.x * pebble->velocity.x);
                    pebble->velocity.z +=
                        frand(pebble->bounce_velocity.z * pebble->velocity.z);
                    pebble->angular_velocity.x *= 1.0f - frand(0.3f);
                    pebble->angular_velocity.y *= 1.0f - frand(0.3f);
                    pebble->angular_velocity.z *= 1.0f - frand(0.3f);
                }
            }
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
        }
    }
    if (any_active) {
        return 1.0f;
    }
    if (pebbles[0].end_behavior == 13) {
        monitor->object->flags09_bits.bit7 = 1;
    }
    return -1.0f;
}
/* Soft ceiling 97.55%: typed array induction and register coloring. */
static float p_pebble_path_monitor(void) {
    BgndPebbleMonitor* monitor;
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    BgndPebbleControl* pebble;
    unsigned int i;
    int any_active;
    float dx;
    float dy;
    float dz;
    float step_x;
    float step_y;
    float step_z;

    monitor = (BgndPebbleMonitor*)pdata_of_proc(aproc);
    collection = (BgndPebbleCollection*)monitor->pebble_data;
    pebbles = collection->pebbles;
    any_active = 0;
    if (g_game_info.bgnd_obj == 0 || pebbles == 0) {
        return -1.0f;
    }
    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return 1.0f;
    }

    for (i = 0; i < (unsigned int)monitor->count; i++) {
        pebble = &pebbles[i];
        if (pebble->state == 1) {
            continue;
        }
        any_active = 1;
        if (pebble->state == 0 || pebble->state == 10) {
            pebble->position.z = 0.0f;
            pebble->position.y = 0.0f;
            pebble->position.x = 0.0f;
            pebble->position.y = -20000.0f;
            MKMatrixTranslate(&collection->matrices[i], &pebble->position, 0);
            if (pebble->state == 10) {
                pebble->state = 1;
            }
            continue;
        }
        if (pebble->state == 4 || pebble->state == 12) {
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
            if (pebble->state == 12) {
                pebble->state = 1;
            }
            continue;
        }
        if (pebble->state == 3) {
            if (--pebble->launch_ticks == 0) {
                pebble->state = 4;
                g_current_pebble = pebble;
                cmdscript_setup_execution(g_game_info.cmdscript,
                                          pebble->bounce_param);
                cmdscript_execute(g_game_info.cmdscript);
            } else {
                MKMatrixRotatXZYScaleTranslate(
                    &collection->matrices[i], pebble->angles.x,
                    pebble->angles.z, pebble->angles.y, &pebble->scale,
                    &pebble->position);
                continue;
            }
        }
        if (pebble->state == 8) {
            pebble->angles.x = 0.96f * pebble->angles.x;
            pebble->angles.z = 0.96f * pebble->angles.z;
            if (pebble->angles.x * pebble->angles.x +
                    pebble->angles.z * pebble->angles.z <
                2.0f) {
                pebble->state = 4;
            }
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
            continue;
        }
        if (pebble->state == 13) {
            dx = pebble->target_position.x - pebble->angles.x;
            dy = pebble->target_position.y - pebble->angles.y;
            dz = pebble->target_position.z - pebble->angles.z;
            step_x = 0.33f * dx;
            step_y = 0.33f * dy;
            step_z = 0.33f * dz;
            pebble->angles.x = step_x + pebble->angles.x;
            pebble->angles.y = step_y + pebble->angles.y;
            pebble->angles.z = step_z + pebble->angles.z;
            if (dx * dx + dy * dy + dz * dz < 50.0f) {
                pebble->angles.x = pebble->target_position.x;
                pebble->angles.y = pebble->target_position.y;
                pebble->angles.z = pebble->target_position.z;
                pebble->state = 1;
            }
            MKMatrixRotatXZYScaleTranslate(
                &collection->matrices[i], pebble->angles.x,
                pebble->angles.z, pebble->angles.y, &pebble->scale,
                &pebble->position);
            continue;
        }

        v3_add_v3_scaled(&pebble->position, &pebble->position,
                         &pebble->velocity, game_speed);
        if (pebble->state != 5) {
            pebble->velocity.y += -0.003f * game_speed;
        }
        v3_add_v3_scaled(&pebble->angles, &pebble->angles,
                         &pebble->angular_velocity, game_speed);
        if (pebble->angles.x > 360.0f) {
            pebble->angles.x -= 360.0f;
        }
        if (pebble->angles.y > 360.0f) {
            pebble->angles.y -= 360.0f;
        }
        if (pebble->angles.z > 360.0f) {
            pebble->angles.z -= 360.0f;
        }
        if (pebble->state == 7) {
            if (pebble->bounce_flags == 0) {
                if (pebble->position.y < g_game_info.field_34 + 0.1f) {
                    pebble->position.y = g_game_info.field_34 + 0.025f;
                }
                pebble->velocity.x = 0.96f * pebble->velocity.x;
                pebble->velocity.y = 0.96f * pebble->velocity.y;
                pebble->velocity.z = 0.96f * pebble->velocity.z;
                pebble->velocity.y = 0.0f;
                pebble->angular_velocity.z = 0.0f;
                pebble->angular_velocity.y = 0.0f;
                pebble->angular_velocity.x = 0.0f;
                if (pebble->state == 8 || pebble->state == 13) {
                    pebble->angles.x = 0.96f * pebble->angles.x;
                    pebble->angles.z = 0.96f * pebble->angles.z;
                }
                if (pebble->velocity.x * pebble->velocity.x +
                        pebble->velocity.y * pebble->velocity.y +
                        pebble->velocity.z * pebble->velocity.z <
                    0.001f) {
                    pebble->state = pebble->end_behavior;
                }
            } else if (pebble->position.y <
                           g_game_info.field_34 + 0.025f &&
                       pebble->velocity.y < 0.0f &&
                       --pebble->bounce_flags > 0) {
                pebble->velocity.y *= -0.5f;
                pebble->angular_velocity.x *= 1.0f - frand(0.3f);
                pebble->angular_velocity.y *= 1.0f - frand(0.3f);
                pebble->angular_velocity.z *= 1.0f - frand(0.3f);
            }
        }
        if (--pebble->bounce_ticks == 0) {
            pebble->state = 4;
            g_current_pebble = pebble;
            if (pebble->bounce_param != 0) {
                cmdscript_setup_execution(g_game_info.cmdscript,
                                          pebble->bounce_param);
                cmdscript_execute(g_game_info.cmdscript);
            }
        }
        MKMatrixRotatXZYScaleTranslate(
            &collection->matrices[i], pebble->angles.x, pebble->angles.z,
            pebble->angles.y, &pebble->scale, &pebble->position);
    }
    if (any_active) {
        return 1.0f;
    }
    if (pebbles[0].end_behavior == 13) {
        monitor->object->flags09_bits.bit7 = 1;
    }
    return -1.0f;
}
/* Soft ceiling 85.07%: typed crack/matrix induction and save-set lowering. */
void bgnd_start_cracks(void) {
    MkSobj* object;
    PebbleData* pebble_data;
    RpAtomic* atomic;

    object = obj_first_sobj(g_bgnd_preloaded_models[0]);
    g_bgnd_preloaded_models[0]->light_flags = 1;
    unhide_obj(g_bgnd_preloaded_models[0]);
    object->flags_08_bits.bit6 = 1;
    unhide_sobj(object);
    object->z_offset = -50.0f;
    object->flags_08_bits.scale_dirty = 1;
    object->scale.x = 3.0f;
    object->scale.y = 3.0f;
    object->scale.z = 3.0f;
    sobj_set_priority(object, 9);
    object->flags09_bits.bit6 = 1;
    object->flags09_bits.bit7 = 1;
    atomic = object->atomic;
    atomic->geometry->flags |= 0x40;
    set_atomic_material_alpha(atomic, 0xAA);
    set_atomic_material_specular(object->atomic, 0xAA);
    object->flags09_bits.bit4 = 1;
    object->flags09_bits.bit3 = 1;
    pebble_data = create_pebble_userdata(
        object, 10, sizeof(BgndCrack));
    g_bgnd_cracks = pebble_data;
    if (pebble_data != 0 && g_bgnd_cracks != 0) {
        bgnd_reset_crack_pool();
    }
}
/* Soft ceiling 69.84%: typed crack/matrix induction and save-set lowering. */
void bgnd_remove_cracks(void) {
    if (g_bgnd_cracks != 0) {
        bgnd_reset_crack_pool();
    }
    g_bgnd_cracks = 0;
}
/* Soft ceiling 67.60%: typed crack/matrix induction and save-set lowering. */
void bgnd_init_cracks(void) {
    if (g_bgnd_cracks != 0) {
        bgnd_reset_crack_pool();
    }
}
/* Soft ceiling 71.90%: save-set and independent argument scheduling only. */
void bgnd_place_crack_when_plyr_hits_ground(unsigned int player_index) {
    BgndCrackPlacer* placer;
    PlyrInfo* player;
    MkProc* process;

    player = &g_game_info.plyr0;
    placer = 0;
    process = _create_mkproc_generic_tinystack(
        0x3018, 0x2E, p_crack_placer, sizeof(BgndCrackPlacer),
        (MkHdr**)&placer);
    if (process != 0) {
        if (player_index == 1) {
            player = &g_game_info.plyr1;
        }
        placer->crack_pool = g_bgnd_cracks;
        placer->player = player;
        placer->delay = 20;
        mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
    }
}
static float p_crack_placer(void) { return 0.0f; }
void bgnd_kill_fx(const char* name) {
    unsigned int effect;

    effect = fx_by_owner(name, 4);
    if (effect != 0) {
        fx_reset(effect);
    }
}
void bgnd_set_fx_z_offset(const char* name, float z_offset) {
    MkPfx* effect;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->field_28 = z_offset;
    }
}
void bgnd_set_fx_ang_y(void* script, float angle) {
    (void)script;
    if (g_latest_obj_pfx != 0) {
        g_latest_obj_pfx->flags_08_bits.angular_velocity_enabled = 1;
        g_latest_obj_pfx->ang.y = angle;
    }
}
void bgnd_set_fx_ang_dir_to_i_vector(void) {
    if (g_latest_obj_pfx != 0) {
        g_latest_obj_pfx->ang.y = gxMathArcTanYX(
            g_game_info.impact_vector.z, g_game_info.impact_vector.x);
    }
}
/* Soft ceiling 92.94%: exact body; GPR save/restore emission differs. */
void bgnd_launch_fx_at_active_sobj_pos_with_offset(
    const char* name, float x_offset, float y_offset, float z_offset) {
    MkPfx* effect;

    reset_effect(name);
    effect = find_pfx_by_name(name);
    if (effect != 0) {
        g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
        if (g_latest_obj_pfx == 0) {
            g_latest_obj_pfx =
                pfx_bind_to_new_obj(effect, (void*)0x8227);
        }
        if (g_latest_obj_pfx != 0) {
            g_latest_obj_pfx->flags_08_bits.airborne = 1;
            g_latest_obj_pfx->pos.value.x = g_active_sobj->pos.x + x_offset;
            g_latest_obj_pfx->pos.value.y = g_active_sobj->pos.y + y_offset;
            g_latest_obj_pfx->pos.value.z = g_active_sobj->pos.z + z_offset;
            update_mkobj(g_latest_obj_pfx);
            resume_effect(name);
        }
    }
}
/* Soft ceiling 72.45%: exact body; FPR/GPR prologue scheduling differs. */
unsigned int pfxhandle_bgnd_spawn_at_position(
    const char* name, float x, float y, float z) {
    MkPfx* effect;
    MkObj* object;
    int handle;

    handle = fx_by_owner(name, 4);
    if (handle == 0) {
        return 0;
    }
    handle = fx_next_emitter(handle);
    if (handle == 0) {
        return 0;
    }
    fx_resume_emit(handle);
    effect = pfx_from_emitter(handle);
    if (effect == 0) {
        return 0;
    }
    object = pfx_bind_emitter_num_to_new_obj(
        effect, (void*)0x6015, emitter_id_from_handle(handle));
    if (object == 0) {
        return 0;
    }
    object->flags_08_bits.airborne = 1;
    object->pos.value.x = x;
    object->pos.value.y = y;
    object->pos.value.z = z;
    update_mkobj(object);
    return handle;
}
/* Soft ceiling 84.14%: exact body; GPR save/restore emission differs. */
void bgnd_launch_fx_to_sobj(const char* name, int object_id) {
    MkPfx* effect;
    MkSobj* object;
    unsigned int handle;

    handle = fx_by_owner(name, 4);
    if (handle != 0) {
        fx_restart_emit(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (object != 0) {
                pfx_bind_emitter_to_sobj(effect, object, 1);
            }
        }
    }
}
/* Soft ceiling 69.81%: exact body; FPR/GPR save/restore scheduling differs. */
void bgnd_launch_fx_at_position(const char* name, float x, float y, float z) {
    MkPfx* effect;
    unsigned int handle;

    handle = fx_by_owner(name, 4);
    if (handle != 0) {
        fx_reset(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
            if (g_latest_obj_pfx == 0) {
                g_latest_obj_pfx =
                    pfx_bind_to_new_obj(effect, (void*)0x8227);
            }
            if (g_latest_obj_pfx != 0) {
                g_latest_obj_pfx->flags_08_bits.airborne = 1;
                g_latest_obj_pfx->pos.value.x = x;
                g_latest_obj_pfx->pos.value.y = y;
                g_latest_obj_pfx->pos.value.z = z;
                update_mkobj(g_latest_obj_pfx);
                resume_effect(name);
            }
        }
    }
}
MkObj* bgnd_fx_get_binded_obj(unsigned int handle) {
    MkPfx* effect;
    MkObj* object;

    effect = pfx_from_handle(handle);
    if (effect == 0) {
        return 0;
    }
    object = (MkObj*)pfx_get_emitter_obj(effect, 0);
    g_latest_obj_pfx = object;
    if (object != 0) {
        return object;
    }
    return 0;
}
/* Soft ceiling 74.44%: exact body; nonvolatile register allocation differs. */
void bgnd_launch_fx_at_plyr_bid(const char* name, int bone) {
    MkObj* object;
    MkPfx* effect;
    unsigned int handle;

    object = plyr_obj;
    handle = fx_by_owner(name, 4);
    if (handle != 0 && object != 0) {
        fx_restart_emit(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            pfx_bind_emitter_to_obj_bone(effect, object, bone);
            fx_resume_emit(handle);
        }
    }
}
/* Soft ceiling 80.44%: exact body; GPR save/restore scheduling differs. */
unsigned int bgnd_pfxhandle_spawn_at_bid(const char* name, MkObj* object,
                                         int bone) {
    MkPfx* effect;
    int handle;
    int emitter;

    handle = fx_by_owner(name, 4);
    if (handle == 0) {
        return 0;
    }
    handle = fx_next_emitter(handle);
    if (handle == 0) {
        return 0;
    }
    fx_resume_emit(handle);
    effect = pfx_from_emitter(handle);
    if (effect == 0) {
        return 0;
    }
    emitter = emitter_id_from_handle(handle);
    if ((unsigned int)bone == 0x40000000) {
        pfx_bind_emitter_num_to_obj(effect, object, 0, emitter);
    } else {
        pfx_bind_emitter_num_to_obj_bone(effect, object, bone, emitter);
    }
    return handle;
}
/* Soft ceiling 74.81%: exact body; nonvolatile register allocation differs. */
void bgnd_launch_fx_at_bid_of_mkobj(const char* name, MkObj* object,
                                    int bone) {
    MkPfx* effect;
    unsigned int handle;

    handle = fx_by_owner(name, 4);
    if (handle != 0 && object != 0) {
        fx_restart_emit(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            pfx_bind_emitter_to_obj_bone(effect, object, bone);
            fx_resume_emit(handle);
        }
    }
}
/* Soft ceiling 93.34%: exact body; GPR save/restore emission differs. */
void bgnd_launch_fx_at_sobj_pos(
    const char* name, unsigned int object_id, float y_offset) {
    MkPfx* effect;
    MkSobj* object;
    unsigned int handle;
    float x;
    float y;
    float z;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        z = object->pos.z;
        y = object->pos.y + y_offset;
        x = object->pos.x;
        handle = fx_by_owner(name, 4);
        if (handle != 0) {
            fx_reset(handle);
            effect = pfx_from_handle(handle);
            if (effect != 0) {
                g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
                if (g_latest_obj_pfx == 0) {
                    g_latest_obj_pfx =
                        pfx_bind_to_new_obj(effect, (void*)0x8227);
                }
                if (g_latest_obj_pfx != 0) {
                    g_latest_obj_pfx->flags_08_bits.airborne = 1;
                    g_latest_obj_pfx->pos.value.x = x;
                    g_latest_obj_pfx->pos.value.y = y;
                    g_latest_obj_pfx->pos.value.z = z;
                    update_mkobj(g_latest_obj_pfx);
                    resume_effect(name);
                }
            }
        }
    }
}
void bgnd_launch_plyr_blood_fx(int effect, int bone_id) {
    start_blood_particles(effect, bone_id, plyr_pdata,
                          plyr_pdata->plyr_info->slot.mirror_a);
}
void bgnd_launch_fx_at_plyr_pos_and_y(const char* name, float y) {
    MkPfx* effect;

    reset_effect(name);
    effect = find_pfx_by_name(name);
    if (effect != 0) {
        g_latest_obj_pfx =
            pfx_bind_to_new_obj(effect, (void*)0x8227);
        if (g_latest_obj_pfx != 0) {
            g_latest_obj_pfx->flags_08_bits.airborne = 1;
            g_latest_obj_pfx->pos.value.x = plyr_obj->pos.value.x;
            g_latest_obj_pfx->pos.value.y = plyr_obj->pos.value.y;
            g_latest_obj_pfx->pos.value.z = plyr_obj->pos.value.z;
            g_latest_obj_pfx->pos.value.y = y;
            update_mkobj(g_latest_obj_pfx);
            resume_effect(name);
        }
    }
}
static inline void bgnd_copy_vec(Vec* destination, Vec* source) {
    destination->x = source->x;
    destination->y = source->y;
    destination->z = source->z;
}

MkObj* ncs_bgnd_preload_named_model(
    const char* section_name, const char* model_name, int object_type,
    int transl, Vec* position, Vec* angles, Vec* scale) {
    MkFileInfo* section;
    MkObj* object;

    object = 0;
    section = find_section_by_name(section_name);
    if (section != 0) {
        add_art_section(0x2001E, section);
        object = load_named_model_from_slot(
            0x2001E, model_name, object_type, transl);
        if (object != 0) {
            object->light_flags = 4;
            object->flags_08_bits.scale_active = 1;
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.angular_velocity_enabled = 1;
            bgnd_copy_vec(&object->pos.value, position);
            bgnd_copy_vec(&object->ang, angles);
            bgnd_copy_vec(&object->scale, scale);
            insert_fgnd_mkobj(object);
            update_mkobj(object);
        }
    }
    return object;
}

/* Exact-size 99.85%; only the pooled 1.0f relocation label differs. */
void bgnd_enable_obj_pos_and_ang_setting(
    MkObj* object, Vec* position, Vec* angles) {
    object->flags_08_bits.airborne = 1;
    object->flags_08_bits.angular_velocity_enabled = 1;
    object->flags_08_bits.scale_active = 1;
    bgnd_copy_vec(&object->pos.value, position);
    bgnd_copy_vec(&object->ang, angles);
    object->scale.x = object->scale.y = object->scale.z = 1.0f;
    update_mkobj(object);
}
/*
 * Soft ceiling 90.13%: the retail body uses stmw/lmw and addresses the error
 * format as a suffix of @stringBase0. The recovered body emits individual
 * saves/restores and a standalone pooled-string relocation; behavior and CFG
 * otherwise agree instruction-for-instruction.
 */
MkObj* bgnd_preload_named_model(const char* model_name, unsigned int model_index) {
    char message[180];
    int art_slot;

    if (model_index >= 15) {
        return 0;
    }

    if (mode_of_play == 9 || mode_of_play == 10) {
        art_slot = 0x8003D;
    } else {
        art_slot = 0x2001E;
    }
    if (g_game_info.bgnd_id == 0x16) {
        art_slot = 0x18006D;
    }

    g_bgnd_preloaded_models[model_index] = (MkObj*)load_named_model_from_slot(
        art_slot, model_name, model_index + 0xC000, 0);
    if (g_bgnd_preloaded_models[model_index] != 0) {
        g_bgnd_preloaded_models[model_index]->pos.value.z = 0.0f;
        g_bgnd_preloaded_models[model_index]->pos.value.y = 0.0f;
        g_bgnd_preloaded_models[model_index]->pos.value.x = 0.0f;
        g_bgnd_preloaded_models[model_index]->light_flags = 1;
        obj_create_sobjs(g_bgnd_preloaded_models[model_index]);
        hide_obj(g_bgnd_preloaded_models[model_index]);
        insert_fgnd_mkobj(g_bgnd_preloaded_models[model_index]);
        if (g_bgnd_preloaded_models[model_index] != 0 &&
            g_game_info.bgnd_obj != 0) {
            mk_insert(&g_bgnd_preloaded_models[model_index]->hdr,
                      &g_game_info.bgnd_obj->child_list);
        }
        return g_bgnd_preloaded_models[model_index];
    }

    sprintf(message, "Could not load bgnd preload model %s", model_name);
    return 0;
}
MkObj* bgnd_get_preload_obj(int model_index) {
    MkObj* object;

    object = g_bgnd_preloaded_models[model_index];
    if (object == 0) {
        return 0;
    }
    return object;
}
/* Exact-size 99.52%; only the compiler-local jump-table relocation differs. */
void bgnd_start_preload_sobj_morph(
    int model_index, int sobj_id, unsigned int script_id,
    unsigned int flags) {
    switch (script_id) {
    case 0:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &linear_slow_script, flags);
        break;
    case 1:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &cos_script, flags);
        break;
    case 2:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &linear_140_script, flags);
        break;
    case 3:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &linear_240_script, flags);
        break;
    case 4:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &cos_fast_script, flags);
        break;
    case 5:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &kuatan_banner_script, flags);
        break;
    case 6:
        obj_start_morph(g_bgnd_preloaded_models[model_index], sobj_id,
                        &skytemple_banner_script, flags);
        break;
    }
}
void bgnd_start_preload_sobj_uv_scroll(
    int model_index, int object_id, float u1, float v1, float u2, float v2) {
    start_sobj_uv_scroll(
        g_bgnd_preloaded_models[model_index], object_id, u1, v1, u2, v2);
}
void bgnd_hide_preload_obj(int model_index) {
    MkObj* object;

    object = g_bgnd_preloaded_models[model_index];
    if (object != 0) {
        hide_obj(object);
    }
}
void bgnd_unhide_preload_obj(int model_index) {
    MkObj* object;

    object = g_bgnd_preloaded_models[model_index];
    if (object != 0) {
        unhide_obj(object);
    }
}
/* Soft ceiling 75.67%: exact behavior; retail selects a CTR clear loop. */
void bgnd_init_timers(int create_monitor) {
    MkProc* process;
    int timer;

    timer = 0;
    do {
        g_game_info.bgnd_timer_ticks[timer] = 0;
        g_game_info.bgnd_timer_limits[timer] = 0;
        timer++;
    } while (timer != 3);
    if (create_monitor == 1) {
        process = _create_mkproc_generic_tinystack(
            0xC010, 0x1F, p_bgnd_timer_monitor, 0, 0);
        if (g_game_info.bgnd_obj != 0) {
            mk_insert((MkHdr*)process, &g_game_info.bgnd_obj->child_list);
        }
    }
}
void bgnd_start_timer(unsigned int timer, int ticks, int limit) {
    if (timer < 3) {
        g_game_info.bgnd_timer_ticks[timer] = ticks;
        g_game_info.bgnd_timer_limits[timer] = limit;
    }
}
int bgnd_timer_get_tick_count(int timer) {
    return g_game_info.bgnd_timer_ticks[timer];
}
/* Soft ceiling 89.29%: typed timer induction and register coloring differ. */
static float p_bgnd_timer_monitor(void) {
    CmdScript* previous_script;
    CmdScript* script;
    unsigned int timer;

    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    for (timer = 0; timer < 3; timer++) {
        if (g_game_info.bgnd_timer_ticks[timer] != 0 &&
            --g_game_info.bgnd_timer_ticks[timer] == 0) {
            script = alloc_cmdscript();
            previous_script = active_cmdscript;
            active_cmdscript = script;
            if (g_game_info.bgnd_timer_limits[timer] != 0) {
                cmdscript_setup_execution(
                    g_game_info.cmdscript,
                    g_game_info.bgnd_timer_limits[timer]);
                cmdscript_execute(g_game_info.cmdscript);
            }
            active_cmdscript = previous_script;
            if (script->instance != 0) {
                ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
            }
        }
    }
    return 1.0f;
}
void bgnd_collison_if_to_scripts_activate(void) {
    set_arena_obstacle_callback(bgnd_collision_to_script_interface);
}
void bgnd_collison_if_set_return_result(int result) {
    g_active_bgnd_col_item->flags.bits.return_result = result;
}
/* Soft ceiling 81.09%: exact dataflow; latch-branch shape and coloring differ. */
void bgnd_collison_if_set_info(void) {
    MkObj* opponent_object;
    MkObj* player_object;
    PlyrPdata* opponent;
    PlyrPdata* player;

    player = g_active_obstacle_event_data->player_pdata;
    player_object = bgnd_get_live_tracked_obj(player);
    if (player_object != 0) {
        opponent = player->his_plyr_pdata;
        opponent_object = bgnd_get_live_tracked_obj(opponent);
        if (opponent_object != 0) {
            g_game_info.collision_player_info = player->plyr_info;
            g_game_info.active_player = opponent->plyr_info;
            g_game_info.impact_vector.x =
                g_active_obstacle_event_data->impact_vector->x;
            g_game_info.impact_vector.y =
                g_active_obstacle_event_data->impact_vector->y;
            g_game_info.impact_vector.z =
                g_active_obstacle_event_data->impact_vector->z;
            g_game_info.player_objects[0] = opponent_object;
            g_game_info.player_objects[1] = player_object;
            g_game_info.collision_player_pdata = player;
            g_game_info.collision_player_side =
                g_active_obstacle_event_data->flag_bits.player_side;
            g_game_info.collision_event_id =
                g_active_obstacle_event_data->event_id;
        }
    }
}
/* Soft ceiling 90.00%: exact body; GPR save/restore emission differs. */
void bgnd_collision_if_rx_override(unsigned int collision_id) {
    PlyrPdata* player;
    ScriptTableDef* table;
    unsigned int* row;

    if (g_game_info.bgnd_id == 2 && collision_id == 0xE8) {
        table = &g_game_info.cmdscript->table_defs[109];
        row = &g_game_info.cmdscript->table_data[table->data_index];
        row[4] = (unsigned int)plyr_pdata_get_plyr_obj(
            g_active_obstacle_event_data->player_pdata);
    }
    player = g_active_obstacle_event_data->player_pdata;
    player->online_sync_index = collision_id;
    if (player->plyr_num == 0) {
        g_game_info.plyr0.fighting_lights.green_trigger = 1;
    } else {
        g_game_info.plyr1.fighting_lights.green_trigger = 1;
    }
}
static inline void bgnd_collision_if_add_monitor(
    int list_index, unsigned int collision_id, unsigned int script_function,
    int monitor_mode, int collision_mode, int bit7) {
    BgndCollisionItem* item;

    item = (BgndCollisionItem*)get_mkhdr_generic(sizeof(BgndCollisionItem));
    item->monitor_mode = monitor_mode;
    item->collision_id = collision_id;
    item->flags.bits.bit7 = bit7;
    item->script_function = script_function;
    item->flags.bits.return_result = 0;
    item->flags.bits.bit6 = 0;
    item->flags.bits.disabled = 0;
    item->collision_mode = collision_mode;
    mk_insert(&item->hdr, &g_bgnd_collision_to_script_if[list_index]);
}

/* Soft ceiling 95.67%: exact inlined cases; GPR save/restore differs. */
void bgnd_collision_if_monitor_col_as(
    int list_index, unsigned int collision_id, unsigned int script_function,
    int monitor_type) {
    switch (monitor_type) {
    case 0:
        bgnd_collision_if_add_monitor(
            list_index, collision_id, script_function, 0, 0, 1);
        break;
    case 2:
        bgnd_collision_if_add_monitor(
            list_index, collision_id, script_function, 2, 2, 0);
        break;
    case 4:
        bgnd_collision_if_add_monitor(
            list_index, collision_id, script_function, 1, 2, 0);
        break;
    case 5:
        bgnd_collision_if_add_monitor(
            list_index, collision_id, script_function, 3, 2, 0);
        break;
    case 3:
        bgnd_collision_if_add_monitor(
            list_index, collision_id, script_function, 0, 3, 1);
        break;
    }
}
void bgnd_collison_if_monitor_col(
    int list_index, unsigned int collision_id, int monitor_mode, int bit7,
    int bit6, unsigned int script_function, int return_result) {
    BgndCollisionItem* item;

    item = (BgndCollisionItem*)get_mkhdr_generic(sizeof(BgndCollisionItem));
    item->monitor_mode = monitor_mode;
    item->collision_id = collision_id;
    item->flags.bits.bit7 = bit7;
    item->script_function = script_function;
    item->flags.bits.return_result = return_result;
    item->flags.bits.bit6 = bit6;
    item->flags.bits.disabled = 0;
    item->collision_mode = 1;
    mk_insert(&item->hdr, &g_bgnd_collision_to_script_if[list_index]);
}
/* Soft ceiling 83.08%: exact body; GPR save/restore emission differs. */
void bgnd_collision_if_enable_col(int list_index, unsigned int collision_id) {
    BgndCollisionItem* item;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;

    list = &g_bgnd_collision_to_script_if[list_index];
    if (list != 0) {
        link = *list;
        while (link != 0) {
            item = (BgndCollisionItem*)link->hdr;
            if (link->instance != item->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (item->collision_id == collision_id) {
                    item->flags.bits.disabled = 0;
                    return;
                }
                link = link->next;
            }
        }
    }
}
/* Soft ceiling 83.08%: exact body; GPR save/restore emission differs. */
void bgnd_collision_if_disable_col(int list_index, unsigned int collision_id) {
    BgndCollisionItem* item;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;

    list = &g_bgnd_collision_to_script_if[list_index];
    if (list != 0) {
        link = *list;
        while (link != 0) {
            item = (BgndCollisionItem*)link->hdr;
            if (link->instance != item->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (item->collision_id == collision_id) {
                    item->flags.bits.disabled = 1;
                    return;
                }
                link = link->next;
            }
        }
    }
}
/* Soft ceiling 85.12%: exact behavior; typed-list CFG and coloring differ. */
static int bgnd_collision_to_script_interface(BgndObstacleEventData* event) {
    BgndCollisionItem* item;
    CmdScript* previous_script;
    CmdScript* script;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    PlyrPdata* player;
    unsigned int list_index;
    unsigned int player_side;

    if (g_game_info.plyr0.slot.pdata->state != 0x4203 &&
        g_game_info.plyr1.slot.pdata->state != 0x4203) {
        if (g_game_info.plyr0.field_0C == 0.0f ||
            g_game_info.plyr1.field_0C == 0.0f) {
            return 0;
        }
        if (!g_game_info.flag_bits.lens_flare_enabled) {
            return 0;
        }
    }
    player = event->player_pdata;
    if (player != 0 &&
        (player->plyr_info->slot.mirror_a == 0 || player->his_obj == 0)) {
        return 0;
    }
    for (list_index = 0; list_index < 8; list_index++) {
        list = &g_bgnd_collision_to_script_if[list_index];
        if (list != 0) {
            link = *list;
            while (link != 0) {
                item = (BgndCollisionItem*)link->hdr;
                if (link->instance != item->hdr.instance) {
                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                    continue;
                }
                player_side = event->flag_bits.player_side;
                if ((item->collision_mode != 0 || player_side) &&
                    !item->flags.bits.disabled &&
                    event->event_id == item->collision_id &&
                    (item->monitor_mode == 0 ||
                     (item->monitor_mode == 1 && player->state == 0x609) ||
                     (item->monitor_mode == 2 && (player->state & 0x400)) ||
                     (item->monitor_mode == 3 && (player->state & 0x1000))) &&
                    (!item->flags.bits.bit7 || player != 0)) {
                    script = alloc_cmdscript();
                    previous_script = active_cmdscript;
                    if (player_side == 1 && item->flags.bits.bit6 == 1 &&
                        player->state != 0x609) {
                        return 0;
                    }
                    if (item->collision_mode != 0 &&
                        item->collision_mode != 3 && player != 0 &&
                        player_side == 1) {
                        event->player_pdata = player->his_plyr_pdata;
                    }
                    g_active_obstacle_event_data = event;
                    g_active_bgnd_col_item = item;
                    active_cmdscript = script;
                    cmdscript_setup_execution(
                        g_game_info.cmdscript, item->script_function);
                    cmdscript_execute(g_game_info.cmdscript);
                    active_cmdscript = previous_script;
                    if (script->instance != 0) {
                        ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
                    }
                    return item->flags.bits.return_result;
                }
                link = link->next;
            }
        }
    }
    return 0;
}
void bgnd_swap_level(void) {}
void bgnd_move_plyrs_to_initial_pos(void) {}
void bgnd_set_new_ground_plane(void) {}
void bgnd_set_player_shadow_ground_plane(void) {}
void bgnd_enable_wall_hider(void) {}
void bgnd_set_wall_hide_distance(void* script, float distance) {
    (void)script;
    g_game_info.wall_hider->hide_distance = distance;
}
void bgnd_add_fx_to_hide(void) {}
void bgnd_add_wall_to_unhide(void) {}
void bgnd_add_wall_to_hide(void) {}
void bgnd_add_new_normal_check_for_hider(void) {}
void bgnd_start_wall_hider(void) {}
void bgnd_remove_wall_from_hider(void) {}
static void p_hide_walls(void) {}
void bgnd_place_object_at_position(void) {}
void bgnd_place_weapon_at_position(void) {}
void bgnd_get_item_from_displayed_list(void) {}
void disable_bgnd_obj_repel(void) {}
void enable_bgnd_obj_repel(void) {}
int bgnd_get_exec_tick_ctr(void) {
    return exec_tick_ctr;
}
void bgnd_act_at_time(void) {}
static void p_act_at_time(void) {}
/* Exact calls/arguments; 73.60%, retail/local 100/108 bytes from stmw/lmw. */
void bgnd_run_camera_script(int argument, int flags) {
    camera_set_attacker(g_game_info.player_objects[1]);
    camera_set_victim(g_game_info.player_objects[0]);
    run_camera_script(g_game_info.cmdscript, argument, flags);
}
void* ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void) {
    return g_active_obstacle_event_data->player_pdata;
}
/* Exact-size 99.82% near miss; only pooled-zero relocation labeling differs. */
void spad_set_y_angle_plus_offset_from_xz_vector(int index, void* script,
                                                 float x, float z,
                                                 float offset) {
    Vec angles;
    Vec direction;
    Vec* output;

    (void)script;
    direction.x = x;
    direction.y = 0.0f;
    direction.z = z;
    output = &g_bgnd_scratch_pad_vectors[index];
    v3_to_xy_ang(&angles, &direction);
    output->x = 0.0f;
    output->y = norm_angle(angles.y + offset);
    output->z = 0.0f;
}
void spad_set_vector_setting(
    int index, void* script, float x, float y, float z) {
    Vec* vector;

    (void)script;
    vector = &g_bgnd_scratch_pad_vectors[index];
    vector->x = x;
    vector->y = y;
    vector->z = z;
}
/*
 * Exact-size 92.09% near miss. The inverse-square-root algorithm and stores
 * agree; residue is stack-slot reuse and floating-point scheduling.
 */
void spad_norm_vector(int index) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    Vec* vector;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x;

    vector = &g_bgnd_scratch_pad_vectors[index];
    x = vector->x;
    squared = vector->z * vector->z +
              (x * x + vector->y * vector->y);
    if (squared <= 0.0f) {
        inverse_length = 0.0f;
    } else {
        input.f = squared;
        estimate.u = 0x5F375A00U - (input.u >> 1);
        product = estimate.f * (squared * estimate.f);
        correction = 3.0f - product;
        inverse_length = 0.0625f * estimate.f * correction *
                         -(correction * (product * correction) - 12.0f);
    }
    vector->x = x * inverse_length;
    vector->y *= inverse_length;
    vector->z *= inverse_length;
}
void spad_rotate_xz_vector(int index, void* script, float angle) {
    Vec* vector;

    (void)script;
    vector = &g_bgnd_scratch_pad_vectors[index];
    rotate_xz(vector, vector, angle);
}
float spad_xz_dot_xz(int first, int second) {
    Vec* a;
    Vec* b;

    a = &g_bgnd_scratch_pad_vectors[first];
    b = &g_bgnd_scratch_pad_vectors[second];
    return a->z * b->z + (a->x * b->x + a->y * b->y);
}
void spad_set_vector(void) {}
/* Exact-size 99.34% near miss; only pooled constants/relocation labels differ. */
float spad_xz_length_vector(int index) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float squared;
    Vec* vector;

    vector = &g_bgnd_scratch_pad_vectors[index];
    squared = vector->x * vector->x + vector->z * vector->z;
    input.f = squared;
    if (squared <= 0.0f) {
        return 0.0f;
    }

    estimate.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    estimate.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    return 0.5f *
           (estimate.f * (3.0f - (estimate.f * estimate.f) / squared));
}
float spad_get_pos(int index, unsigned int component) {
    Vec* vector;
    float value;

    vector = &g_bgnd_scratch_pad_vectors[index];
    value = vector->x;
    if (component == 1) {
        value = vector->y;
    } else if (component == 2) {
        value = vector->z;
    }
    return value;
}
/* Exact operations/size; remaining 3.81% is GPR argument coloring. */
void spad_sub_vectors(int destination, int first, int second) {
    Vec* output;
    Vec* a;
    Vec* b;

    a = &g_bgnd_scratch_pad_vectors[first];
    b = &g_bgnd_scratch_pad_vectors[second];
    output = &g_bgnd_scratch_pad_vectors[destination];
    output->x = a->x - b->x;
    output->y = a->y - b->y;
    output->z = a->z - b->z;
}
void spad_add_vector(int index, void* script, float x, float y, float z) {
    Vec* vector;

    (void)script;
    vector = &g_bgnd_scratch_pad_vectors[index];
    vector->x += x;
    vector->y += y;
    vector->z += z;
}
/* Exact operations/size; remaining 3.33% is GPR argument coloring. */
void spad_set_vector_y(int index, void* script, float y) {
    (void)script;
    g_bgnd_scratch_pad_vectors[index].y = y;
}
/*
 * Clean-C ceiling: 84.75%, retail/local 316/304 bytes. Both SDK square-root
 * refinements and the final dot/length division agree; stack reuse differs.
 */
float spad_xz_cos_two_vectors(int first, int second) {
    union {
        float f;
        unsigned int u;
    } estimate1, input1, estimate2, input2;
    Vec* first_vector;
    Vec* second_vector;
    float first_length;
    float second_length;
    float squared;

    first_vector = &g_bgnd_scratch_pad_vectors[first];
    second_vector = &g_bgnd_scratch_pad_vectors[second];
    squared = first_vector->x * first_vector->x +
              first_vector->z * first_vector->z;
    input1.f = squared;
    if (squared <= 0.0f) {
        first_length = 0.0f;
    } else {
        estimate1.u =
            (unsigned int)GXMathSqrtTable[(input1.u >> 10) & 0x3FFE] << 8;
        estimate1.u |= (((input1.u & 0x7F800000U) + 0x3F800000U) >> 1) &
                       0x7F800000U;
        first_length =
            0.5f * estimate1.f *
            (3.0f - (estimate1.f * estimate1.f) / squared);
    }

    squared = second_vector->x * second_vector->x +
              second_vector->z * second_vector->z;
    input2.f = squared;
    if (squared <= 0.0f) {
        second_length = 0.0f;
    } else {
        estimate2.u =
            (unsigned int)GXMathSqrtTable[(input2.u >> 10) & 0x3FFE] << 8;
        estimate2.u |= (((input2.u & 0x7F800000U) + 0x3F800000U) >> 1) &
                       0x7F800000U;
        second_length =
            0.5f * estimate2.f *
            (3.0f - (estimate2.f * estimate2.f) / squared);
    }

    return (first_vector->x * second_vector->x +
            first_vector->z * second_vector->z) /
           (first_length * second_length);
}
/* Exact-size 99.15% near miss; only pooled constants/relocation labels differ. */
void spad_set_heading_vector_to(int index, void* script, float heading,
                                float offset) {
    Vec* vector;
    float angle;
    int normalized;

    (void)script;
    normalized = (int)(166886.1f * (heading + offset)) & 0xFFFFF;
    angle = 0.000005992112f * (float)normalized;
    vector = &g_bgnd_scratch_pad_vectors[index];
    vector->x = gxMathSin(angle);
    vector->y = 0.0f;
    vector->z = gxMathCos(angle);
}
void spad_scale_vector(int destination, unsigned int source, void* script,
                       float scale) {
    Vec* input;
    Vec* output;

    (void)script;
    output = &g_bgnd_scratch_pad_vectors[destination];
    if (source == 0xF0) {
        input = &g_game_info.impact_vector;
    } else {
        input = &g_bgnd_scratch_pad_vectors[source];
    }
    output->x = input->x * scale;
    output->y = input->y * scale;
    output->z = input->z * scale;
}
void bgnd_xfer_attacker(void) {}
void bgnd_process_collision_info(void) {}
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player) {
    (void)player;
}
void dont_fence_plyr_in(int disabled) {
    plyr_obj->flags_0B_bits.bit6 = disabled;
}
void bgnd_takeover_plyr(void) {}
void bgnd_process_active_sobj_info(void) {}
void bgnd_move_player(unsigned int player, int position_index,
                      int angle_index) {
    MkObj* object;
    Vec* position;
    Vec* angles;

    position = &g_bgnd_scratch_pad_vectors[position_index];
    angles = &g_bgnd_scratch_pad_vectors[angle_index];
    object = g_game_info.plyr0.slot.mirror_a;
    if (player == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    }
    move_player(object, position, angles);
}
int bgnd_get_first_shape_center_for_obstacle_id(int obstacle_id,
                                                int scratch_index) {
    return get_first_shape_center_for_obstacle_id(
        obstacle_id, &g_bgnd_scratch_pad_vectors[scratch_index]);
}
void bgnd_make_displayed_item_pickupable_at_active_sobj_pos(void) {}
void bgnd_rotate_xz_about_orgin_active_sobj(void) {}
void bgnd_hide_active_sobj(void) {
    hide_sobj(g_active_sobj);
}
void bgnd_unhide_active_sobj(void) {
    unhide_sobj(g_active_sobj);
}
void bgnd_get_active_sobj_pos(Vec* position) {
    position->x = g_active_sobj->pos.x;
    position->y = g_active_sobj->pos.y;
    position->z = g_active_sobj->pos.z;
}
void bgnd_apply_active_sobj_pos_vel_drag(void* script, float x, float y,
                                         float z) {
    (void)script;
    g_active_sobj->pos_vel.x *= x;
    g_active_sobj->pos_vel.y *= y;
    g_active_sobj->pos_vel.z *= z;
}
/* Exact operations/size; remaining 1.43% is constant relocation labeling. */
void bgnd_set_active_sobj_pos_vel(void* script, float x, float y, float z) {
    (void)script;
    g_active_sobj->flags_08_bits.angular_velocity_enabled = 1;
    if (x != 555999.6f) {
        g_active_sobj->pos_vel.x = x;
    }
    if (y != 555999.6f) {
        g_active_sobj->pos_vel.y = y;
    }
    if (z != 555999.6f) {
        g_active_sobj->pos_vel.z = z;
    }
}
void bgnd_set_active_sobj_rop(int priority) {
    sobj_set_priority(g_active_sobj, priority);
}
void bgnd_set_active_sobj_scale(void* script, float x, float y, float z) {
    (void)script;
    g_active_sobj->flags_08_bits.scale_dirty = 1;
    g_active_sobj->scale.x = x;
    g_active_sobj->scale.y = y;
    g_active_sobj->scale.z = z;
}
void bgnd_set_active_sobj_ang(void* script, float x, float y, float z) {
    (void)script;
    g_active_sobj->flags_08_bits.bit3 = 1;
    g_active_sobj->ang.x = x;
    g_active_sobj->ang.y = y;
    g_active_sobj->ang.z = z;
}
void bgnd_set_active_sobj_pos(void* script, float x, float y, float z) {
    (void)script;
    g_active_sobj->flags_08_bits.bit6 = 1;
    g_active_sobj->pos.x = x;
    g_active_sobj->pos.y = y;
    g_active_sobj->pos.z = z;
}
void bgnd_reset_sobj(int object_id) {
    MkSobj* object;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    object->flags_08_bits.bit5 = 0;
    object->flags_08_bits.angular_velocity_enabled = 0;
    object->pos_vel.z = 0.0f;
    object->pos_vel.y = 0.0f;
    object->pos_vel.x = 0.0f;
    object->ang_vel.z = 0.0f;
    object->ang_vel.y = 0.0f;
    object->ang_vel.x = 0.0f;
}
void bgnd_update_active_mksobj(void) {
    update_mksobj(g_active_sobj);
}
void bgnd_set_active_sobj_zoffset(void* script, float z_offset) {
    (void)script;
    g_active_sobj->z_offset = z_offset;
}
void bgnd_active_sobj_no_ztest(void) {
    g_active_sobj->flags09_bits.bit6 = 1;
}
void bgnd_active_sobj_no_zwrite(void) {
    g_active_sobj->flags09_bits.bit7 = 1;
}
/*
 * Clean-C ceiling: 50.02%, retail/local 176/152 bytes. As in bgnd_fetch_sobj,
 * MWCC folds retail's explicit null-normalization branches from typed C.
 */
void bgnd_set_active_sobj_in_obj(int model_index, unsigned int object_id) {
    MkObj* model;
    MkSobj* object;

    if (model_index == (int)0xDDDDEEEE) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    } else {
        model = g_bgnd_preloaded_models[model_index];
        unhide_obj(model);
        if (model == 0) {
            object = 0;
        } else {
            object = obj_find_sobj_by_id(model, object_id);
            if (object == 0) {
                object = obj_first_sobj(model);
            }
        }
    }
    g_active_sobj = object;
}
int bgnd_is_active_sobj_hidden(void) {
    return is_sobj_hidden(g_active_sobj);
}
/* Soft ceiling: 92.86%, 52/56 bytes -- one unused retail null comparison. */
void bgnd_set_active_sobj(int object_id) {
    g_active_sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
}
void bgnd_sobj_cam_frustum_test_into_transparent(void) {}
void obj_sobj_cam_frustum_test_into_transparent(void) {}
void bgnd_sobj_cam_volume_test_steer_over(void) {}
void bgnd_clear_danger_zone_callback(PlyrPdata* pdata) {
    pdata->online_sync_index = -1;
    if (pdata->plyr_num == 0) {
        g_game_info.plyr0.fighting_lights.green_trigger = 0;
        return;
    }
    g_game_info.plyr1.fighting_lights.green_trigger = 0;
}
void bgnd_register_danger_zone_callback(PlyrPdata* pdata, int callback) {
    pdata->online_sync_index = callback;
    if (pdata->plyr_num == 0) {
        g_game_info.plyr0.fighting_lights.green_trigger = 1;
        return;
    }
    g_game_info.plyr1.fighting_lights.green_trigger = 1;
}
void bgnd_rx_notify(void) {}
void bgnd_current_rx_set_info(void) {}
void bgnd_current_rx_get_info(void) {}
void bgnd_setup_rx_handler(int handler) {
    g_current_reaction_info[5] = 1;
    g_current_reaction_info[4] = handler;
}
void bgnd_anim_camera_ended(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    void* script_ptr;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    info = &g_game_info;
    active_cmdscript = script;
    script_ptr = info->section != 0 ? info->section->cam_ended_script : 0;
    if (script_ptr != 0) {
        cmdscript_setup_execution(info->cmdscript, (unsigned int)script_ptr);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((int (*)(CmdScript*))script->vtbl->destroy)(script);
    }
}

void bgnd_anim_camera_setup(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    void* script_ptr;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    active_cmdscript = script;
    cam_set_intro_cam_pause_ticks(0.0f);
    info = &g_game_info;
    script_ptr = info->section != 0 ? info->section->cam_setup_script : 0;
    if (script_ptr != 0) {
        cmdscript_setup_execution(info->cmdscript, (unsigned int)script_ptr);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((int (*)(CmdScript*))script->vtbl->destroy)(script);
    }
}

void bgnd_fade_object(void) {}
static void p_bgnd_fade_object(void) {}
void pulsate_object(void) {}
void bgnd_pulsate_object(void) {}
void bgnd_pulsate_object_with_caps_and_scale(void) {}
void bgnd_pulsate_object_with_caps(void) {}
static void p_bgnd_pulsate_object(void) {}
static void p_pulsate_object(void) {}
static inline void set_subobject_transl(MkSobj* object) {
    RpAtomic* atomic;
    RpGeometry* geometry;

    if (object != 0) {
        atomic = object->atomic;
        if (atomic != 0) {
            geometry = atomic->geometry;
            if (geometry != 0) {
                geometry->flags |= 0x40;
            }
            sobj_set_transl_flag(object);
        }
    }
}

void bgnd_make_mkobj_transl(MkObj* object) {
    set_subobject_transl(obj_first_sobj(object));
}

void make_subobject_transl(MkSobj* object) {
    set_subobject_transl(object);
}

void bgnd_make_object_transl(unsigned int object_id) {
    MkSobj* object;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        set_subobject_transl(object);
    }
}
void mks_xfer_collision_info_plyr_to_bgnd_script(
    PlyrPdata* player, BgndScriptEntryFn entry) {
    (void)player;
    (void)entry;
}
void mks_xfer_collision_info_plyr_to_script(BgndScriptEntryFn entry,
                                             int player) {
    (void)entry;
    (void)player;
}
/*
 * Clean-C emission ceiling: MWCC uses paired stw/lwz saves locally while
 * retail uses stmw/lmw for the same r30/r31 lifetimes.
 */
void xfer_player_proc_to_script_manual_messaging(
    FighterMirror* fighter, MkObj* object, int function) {
    MkProc* process;

    (void)fighter;
    process = get_player_proc(object);
    get_cmdscript_for_proc(process)->unk28 = function;
    xfer_player_proc(process, bgnd_call_script_function);
}
void xfer_player_proc_to_script(MkObj* object, int function) {
    MkProc* process;

    process = get_player_proc(object);
    get_cmdscript_for_proc(process)->unk28 = function;
    xfer_player_proc(process, bgnd_call_script_function);
}
static float bgnd_call_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(g_game_info.cmdscript, active_cmdscript->unk28);
    call_player_script_function(g_game_info.cmdscript);
    return 0.0f;
}
void bgnd_append_texture_to_material(int sobj_id, int material_id,
                                     char* texture_name, int texture_slot) {
    (void)sobj_id;
    (void)material_id;
    (void)texture_name;
    (void)texture_slot;
}
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries) {
    (void)entries;
}
void bgnd_swap_textures(int sobj_id, int material_id, int frame) {
    (void)sobj_id;
    (void)material_id;
    (void)frame;
}
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries, int frame) {
    (void)entries;
    (void)frame;
}
/* Exact operations; 49.29%, retail/local 112/120 from FPR helper selection. */
void bgnd_rotate_sobj(unsigned int object_id, void* script, float x, float y,
                      float z) {
    MkSobj* object;

    (void)script;
    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        object->flags_08_bits.angular_velocity_enabled = 1;
        object->ang_vel.x = x;
        object->ang_vel.y = y;
        object->ang_vel.z = z;
    }
}
void bgnd_replace_tex_with_wiff_and_ani(void) {}
float bgnd_blood_control(int player, int enabled, void* script, float value) {
    (void)script;
    (void)value;
    switch (player) {
    case 0:
        g_game_info.blood_flags.blood_enabled = enabled;
        break;
    default:
        break;
    }
    return 0.0f;
}
void bgnd_shadow_control(unsigned int enabled) {
    if (enabled == 0) {
        plyr_turn_off_mirrorguy(&g_game_info.plyr0);
        plyr_turn_off_shadowbox(&g_game_info.plyr0);
        plyr_turn_off_mirrorguy(&g_game_info.plyr1);
        plyr_turn_off_shadowbox(&g_game_info.plyr1);
        return;
    }

    plyr_turn_on_mirrorguy(&g_game_info.plyr0);
    plyr_turn_on_shadowbox(&g_game_info.plyr0);
    plyr_turn_on_mirrorguy(&g_game_info.plyr1);
    plyr_turn_on_shadowbox(&g_game_info.plyr1);
}
/* Exact operations; 82.31%, retail/local 104/112 bytes from stmw/lmw choice. */
void bgnd_always_face_y(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            object->flags09_bits.bit5 = 1;
        }
    }
}
/* Exact operations; 82.31%, retail/local 104/112 bytes from stmw/lmw choice. */
void bgnd_no_z_test(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            object->flags09_bits.bit6 = 1;
        }
    }
}

/* Exact operations; 82.31%, retail/local 104/112 bytes from stmw/lmw choice. */
void bgnd_no_z_write(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            object->flags09_bits.bit7 = 1;
        }
    }
}

/* Exact operations; 82.08%, retail/local 104/112 bytes from stmw/lmw choice. */
void bgnd_apply_zoffset(unsigned int object_id, void* script, float z_offset) {
    MkSobj* object;

    (void)script;
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            object->z_offset = z_offset;
        }
    }
}

/* Exact operations; 72.80%, retail/local 100/116 bytes from stmw/lmw choice. */
void bgnd_sobj_set_priority(unsigned int object_id, int priority) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            sobj_set_priority(object, priority);
        }
    }
}
void bgnd_set_sobj_uv_scroll_abs_values(void) {}
void bgnd_set_sobj_uv_scroll_rate_values(void) {}
void bgnd_init_all_uv_scroll_w_control(void) {}
void bgnd_destroy_sobj_uv_scroll_w_control(void) {}
void bgnd_start_sobj_uv_scroll_w_control(void) {}
void bgnd_restore_player(void) {
    back_to_normal();
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_0B_bits.bit6 = 0;
    plyr_pdata->state_flags.bits.bit3 = 0;
}
void bgnd_force_plyr_ground_plane(
    unsigned int player_index, void* script, float height) {
    MkObj* object;

    (void)script;
    object = g_game_info.plyr0.slot.mirror_a;
    if (player_index == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    }
    object->ground_colls_y = height;
}
void bgnd_force_ground_to(void* script, float height) {
    (void)script;
    g_game_info.field_34 = height;
}
/*
 * Exact validated-process and movement test; 80.36%, retail/local 112/96.
 * This compiler folds retail's explicit null-normalization branches.
 */
int bgnd_launch_plyr_up_and_forward_running(void) {
    PlyrPdata* pdata;
    MkProc* process;

    pdata = g_game_info.collision_player_pdata->his_plyr_pdata;
    process = pdata->transient_proc;
    if (process != 0 && process->instance != pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 || g_game_info.player_objects[0]->flags_08_bits.moving) {
        return 1;
    }
    return 0;
}
void bgnd_launch_plyr_up_and_forward(
    int duration, int animation, float ground_y, float gravity,
    float vertical_velocity, float forward_velocity, float damping) {
    plyr_obj->ground_colls_y = ground_y;
    plyr_obj->flags_08_bits.gravity_enabled = 1;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->gravity = gravity;
    plyr_obj->pos_vel.y = vertical_velocity;
    if (duration != 0 && forward_velocity != 0.0f) {
        force_forward(duration, animation, forward_velocity, damping);
    }
}
void bgnd_turn_off_backface_culling(void) {}
void bgnd_turn_on_backface_culling(void) {}
void bgnd_allow_dirty_floor(void) {
    g_game_info.floor_flags.clean_floor = 0;
}
void bgnd_set_plyr_gravity(void* script, float gravity) {
    (void)script;
    plyr_obj->gravity = gravity;
    plyr_obj->flags_08_bits.moving = 1;
}
void bgnd_clean_up_floor(void) {
    g_game_info.floor_flags.clean_floor = 1;
}
void bgnd_sobj_set_rel_pos(unsigned int object_id, void* script, float x,
                           float y, float z) {
    MkSobj* object;

    (void)script;
    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        object->flags_08_bits.bit6 = 1;
        object->pos.x += x;
        object->pos.y += y;
        object->pos.z += z;
    }
}
/* Exact operations; 80.00%, retail/local 92/100 bytes from stmw/lmw choice. */
void bgnd_unhide_sobj_and_children(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            unhide_sobj_and_children(object);
        }
    }
}
/* Exact operations; 80.00%, retail/local 92/100 bytes from stmw/lmw choice. */
void bgnd_hide_sobj_and_children(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            hide_sobj_and_children(object);
        }
    }
}
/* Exact operations; 80.00%, retail/local 92/100 bytes from stmw/lmw selection. */
void bgnd_unhide_sobj(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            unhide_sobj(object);
        }
    }
}
void bgnd_unhide_sobj_list(int list_id) {
    (void)list_id;
}
/* Exact operations; 80.00%, retail/local 92/100 bytes from stmw/lmw selection. */
void bgnd_hide_sobj(unsigned int object_id) {
    MkSobj* object;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            hide_sobj(object);
        }
    }
}
void bgnd_hide_sobj_list(int list_id) {
    (void)list_id;
}
void bgnd_sobj_get_ang(unsigned int object_id, Vec* angles) {
    MkSobj* object;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        angles->x = object->ang.x;
        angles->y = object->ang.y;
        angles->z = object->ang.z;
    }
}
/*
 * Exact body operations; 49.29%, retail/local 112/120 bytes. Retail calls the
 * shared FPR save/restore helpers while this compiler invocation emits stores.
 */
void bgnd_sobj_set_ang(unsigned int object_id, void* script, float x,
                       float y, float z) {
    MkSobj* object;

    (void)script;
    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        object->flags_08_bits.bit3 = 1;
        object->ang.x = x;
        object->ang.y = y;
        object->ang.z = z;
    }
}
/* Same exact-operation FPR save/restore ceiling as bgnd_sobj_set_ang. */
void bgnd_sobj_set_pos_vel(unsigned int object_id, void* script, float x,
                           float y, float z) {
    MkSobj* object;

    (void)script;
    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        object->flags_08_bits.bit5 = 1;
        object->pos_vel.x = x;
        object->pos_vel.y = y;
        object->pos_vel.z = z;
    }
}
float bgnd_sobj_get_z_pos(int object_id) {
    MkSobj* object;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        return object->pos.z;
    }
    return 0.0f;
}
float bgnd_sobj_get_y_pos(int object_id) {
    MkSobj* object;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        return object->pos.y;
    }
    return 0.0f;
}
float bgnd_sobj_get_x_pos(int object_id) {
    MkSobj* object;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        return object->pos.x;
    }
    return 0.0f;
}
/* Same exact-operation FPR save/restore ceiling as bgnd_sobj_set_ang. */
void bgnd_sobj_set_pos(unsigned int object_id, void* script, float x,
                       float y, float z) {
    MkSobj* object;

    (void)script;
    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        object->flags_08_bits.bit6 = 1;
        object->pos.x = x;
        object->pos.y = y;
        object->pos.z = z;
    }
}
float bgnd_get_sobj_ang_y(int object_id) {
    MkSobj* object;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object == 0) {
        return 0.0f;
    }
    return object->ang.y;
}
void bgnd_start_sobj_uv_scroll(void) {}
void bgnd_start_sobj_uv_scroll_tbl(void) {}
void bgnd_light_set_color(int light_id, float red, float green, float blue) {
    (void)light_id;
    (void)red;
    (void)green;
    (void)blue;
}
void bgnd_sobj_set_ani_framerate(unsigned int object_id,
                                 unsigned int material_id, void* script,
                                 float rate) {
    AniTextureControl* control;
    MkSobj* object;

    (void)script;
    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            control = find_atc_for_atomic_material_id(object->atomic,
                                                       material_id);
            if (control != 0) {
                set_ani_texture_framerate(control, rate);
            }
        }
    }
}

/* Exact operations; 82.86%, retail/local 112/120 bytes from stmw/lmw selection. */
void bgnd_sobj_set_ani_frame(unsigned int object_id,
                             unsigned int material_id, int frame) {
    AniTextureControl* control;
    MkSobj* object;

    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            control = find_atc_for_atomic_material_id(object->atomic,
                                                       material_id);
            if (control != 0) {
                set_ani_texture_frame(control, frame);
            }
        }
    }
}
void bgnd_sobj_set_alpha(unsigned int object_id, unsigned int alpha) {
    MkSobj* object;

    if (g_game_info.bgnd_obj != 0) {
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            set_atomic_material_alpha(object->atomic, alpha);
        }
    }
}
float bgnd_get_float(int value_id) {
    switch (value_id) {
    case 0:
        return g_game_info.field_34;
    default:
        return 0.0f;
    }
}
unsigned int bgnd_get_u32(int value_id) {
    switch (value_id) {
    case 0:
        return 0;
    default:
        return 0;
    }
}
void bgnd_clear_face_opponent_flags(void) {
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 0;
}
int bgnd_get_int(int value_id) {
    (void)value_id;
    return 0;
}
void bgnd_create_sobjs(void) {
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0);
}
void bgnd_place_point_light_for_ticks(void) {}
static void p_bgnd_point_light_life_span(void) {}
/* Exact code bytes apart from float-pool relocation identities. */
float degrees_to_rad(void* script, float degrees) {
    (void)script;
    return 3.1415927f * degrees / 180.0f;
}
/* Exact code bytes apart from float-pool relocation identities. */
float rad_to_degrees(void* script, float radians) {
    (void)script;
    return 180.0f * radians / 3.1415927f;
}
/* Exact instruction stream apart from the conversion-pool relocation label. */
float int_to_float(int value) { return value; }
int float_to_int(void* script, float value) {
    (void)script;
    return (int)value;
}
void obj_sobj_set_material(void) {}
int force_atomic_material_alpha(void) { return 0; }
void p_track_cam_ang_y_light(void) {}
void load_bgnd_style(void) {}
void ncs_bgnd_nuke_collision_to_script_interface(void) {}
MkObj* retrieve_bgnd_obj(void) {
    return g_game_info.bgnd_obj;
}
void destroy_background_extras(void) {}
static void add_mkx_light_obj_to_bgnd_cleanup_list(void* obj) { (void)obj; }
int load_background(int bgnd_id) {
    char* anims;
    char* gbd;
    int entry_off;
    BgndDataTable* data_table;
    BgndMisc* misc;
    MkObj* bgnd_obj;
    ScriptSlot* slot;
    char* art_name;
    int art_id;
    int i;
    int n;
    int zero;
    char* react;
    char* col;
    int* effect_list;
    int effect_off;
    float inv255;
    float* fog_col;
    LoadBgndCtx ctx;
    GlobalBackgroundEntry* entry;

    anims = bgnd_animations;

    if (mode_of_play == 6 && bgnd_id != 0x17) {
        return 0;
    }

    RwImageSetGamma(1.0f);

    /* Array indexing coax: retail uses lwzx with bgnd_id<<4. */
    gbd = (char*)global_background_data;
    entry_off = bgnd_id * 4; /* word index into 16-byte records */
    entry = &global_background_data[bgnd_id];
    load_ssf((MkFileEntry*)((void**)gbd)[entry_off]);

    slot = cmdscript_loadfile_by_name(0xB, (char*)((void**)gbd)[entry_off + 1]);
    g_game_info.cmdscript = slot;

    data_table = (BgndDataTable*)get_data_table(slot, slot->table_count);
    g_game_info.section = data_table;
    misc = data_table != 0 ? data_table->misc : 0;
    g_game_info.misc = misc;

    init_misc_bgnd_data();

    zero = 0;
    g_game_info.field_64 = zero;
    g_game_info.field_60 = zero;
    g_game_info.npc_list = (MkPtr*)zero;
    react = anims + 0xF0;
    col = anims + 0x588;
    *(int*)(react + 0x14) = zero;
    i = 0;
    n = 8;
    do {
        *(int*)(col + i) = zero;
        i += 4;
    } while (--n);
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;

    data_table = g_game_info.section;
    if (data_table != 0 && (data_table->flags88 & 1) != 0) {
        return 1;
    }

    if (mode_of_play == 9 || mode_of_play == 10) {
        if ((entry->flags & 8) != 0) {
            art_name = data_table->art_name;
            art_id = 0x8003D;
        } else {
            return 0;
        }
    } else if (mode_of_play == 0xB) {
        if ((entry->flags & 0x10) != 0) {
            art_name = data_table->art_name;
            art_id = 0x140064;
        } else {
            return 0;
        }
    } else {
        art_name = data_table->art_name;
        art_id = 0x2001E;
    }

    if (bgnd_id == 0x16) {
        art_id = 0x18006D;
    }

    if (data_table != 0 && (unsigned int)data_table->art_name != 0) {
        load_art_section_by_name(art_id, art_name);
    }

    bgnd_obj = (MkObj*)load_named_model_from_slot(art_id, "BACKGROUND", 0x1004, 0);
    g_game_info.bgnd_obj = bgnd_obj;

    data_table = g_game_info.section;
    if (data_table != 0) {
        if (data_table->anims != 0) {
            load_background_anims(data_table->anims, bgnd_id);
        } else {
            memset(anims, 0, 0x84);
        }
    }

    init_weapon_trail_light_list();

    misc = g_game_info.misc;
    if ((unsigned int)misc->lights_spec != 0) {
        load_lights(misc->lights_spec, &bgnd_spec_light_list);
    }
    if ((unsigned int)misc->lights_plyr != 0) {
        load_lights(misc->lights_plyr, &plyr_light_list);
    }
    if ((unsigned int)misc->lights_bgnd != 0) {
        load_lights(misc->lights_bgnd, &bgnd_light_list);
    } else {
        g_game_info.bgnd_obj->light_flags = 0;
    }

    bgnd_obj = g_game_info.bgnd_obj;
    if (bgnd_obj != 0) {
        insert_fgnd_mkobj(bgnd_obj);
    } else {
        return 0;
    }

    if ((unsigned int)misc->lights_bgnd != 0) {
        g_game_info.bgnd_obj->light_flags = 0x1009;
    } else {
        g_game_info.bgnd_obj->light_flags = 0x1000;
    }

    data_table = g_game_info.section;
    set_background_color(
        (int)data_table->bg_r,
        (int)data_table->bg_g,
        (int)data_table->bg_b,
        (int)data_table->bg_a);

    g_game_info.field_34 = 0.0f;

    data_table = g_game_info.section;
    art_name = data_table->sky_name;
    if (art_name != 0 && art_name[0] != 0) {
        g_game_info.sky =
            (MkObj*)load_named_model_from_slot(art_id, art_name, 0x201F, 0);
    }

    if (g_game_info.sky != 0) {
        mk_insert((MkHdr*)g_game_info.sky, &g_game_info.bgnd_obj->child_list);
    }

    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&weapon_trail_light_list);
    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&bgnd_light_list);
    apply_to_mklist((MkListApplyFn)add_mkx_light_obj_to_bgnd_cleanup_list,
                    (MkPtr**)&plyr_light_list);

    data_table = g_game_info.section;
    if ((data_table->flags70 & 1) != 0) {
        UpdateShadowCameraLightSource(&misc->shadow_cam_light);
    }

    misc = g_game_info.misc;
    data_table = g_game_info.section;
    ShadowStrength = misc->shadow_strength;
    inv255 = 255.0f;
    fog_col = fog_color_real;
    fog_col[0] = data_table->fog_r / inv255;
    fog_col[1] = data_table->fog_g / inv255;
    fog_col[2] = data_table->fog_b / inv255;
    fog_col[3] = data_table->fog_a / inv255;

    RwCameraSetNearClipPlane(Camera, data_table->near_clip);
    data_table = g_game_info.section;
    RwCameraSetFarClipPlane(Camera, data_table->far_clip_cam);
    fog_density = data_table->fog_density;
    fog_distance = data_table->fog_distance;
    if (data_table->fog_enable != 0) {
        turn_fog_on();
    } else {
        turn_fog_off();
    }

    if (mode_of_play != 9 && mode_of_play != 0xB) {
        initialize_bgnd_collisions(g_game_info.section);
    }

    if (Camera != 0) {
        data_table = g_game_info.section;
        RwCameraSetNearClipPlane(Camera, data_table->near_clip);
        data_table = g_game_info.section;
        RwCameraSetFarClipPlane(Camera, data_table->far_clip_cam);
    }

    bgnd_obj = g_game_info.bgnd_obj;
    g_game_info.bgnd_id = bgnd_id;
    ctx.art_id = art_id;
    ctx.bgnd_obj = bgnd_obj;
    ctx.pad = 0;

    slot = g_game_info.cmdscript;
    active_cmdscript->mko = slot;
    slot->load_ctx = &ctx;

    effect_list = g_game_info.section->effect_banks;
    if (effect_list != 0) {
        for (effect_off = 0; (i = effect_list[effect_off]) != 0; effect_off++) {
            load_effect_bank(i);
        }
    }

    g_game_info.cmdscript->load_ctx = 0;

    data_table = g_game_info.section;
    if ((unsigned int)data_table->load_script != 0) {
        slot = g_game_info.cmdscript;
        cmdscript_setup_execution(slot, (unsigned int)data_table->load_script);
        cmdscript_execute(slot);
    }

    g_game_info.wall_hider = 0;
    misc = g_game_info.misc;
    if ((unsigned int)misc->script != 0) {
        slot = g_game_info.cmdscript;
        cmdscript_setup_execution(slot, (unsigned int)misc->script);
        cmdscript_execute(slot);
    }

    g_game_info.field_08 = 1;
    g_game_info.field_74 = 0;
    if (mode_of_play == 10) {
        mk_chess_init_bgnd_for_fight_mode();
    }
    return 1;
}

void init_bgnd_info_struct(void) {
    g_game_info.field_08 = 0;
    g_game_info.section = 0;
    g_game_info.misc = 0;
    g_game_info.active_level = 0;
    g_game_info.bgnd_obj = 0;
    g_game_info.sky = 0;
    g_game_info.field_78 = 0;
    g_game_info.mode_table = 0;
}
int get_bgnd_flags(void) {
    if (g_game_info.field_08 == 0) {
        return 0;
    }
    if (g_game_info.section != 0) {
        return g_game_info.section->flags70;
    }
    return 0;
}
