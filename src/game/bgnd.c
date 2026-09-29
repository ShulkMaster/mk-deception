#include "game/bgnd.h"
#include "game/profile_unlock.h"
#include "game/ejb.h"
#include "game/game_info.h"
#include "game/jdn.h"
#include "game/moveset.h"
#include "game/plyr.h"
#include "game/collision.h"
#include "game/constrain.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/anim_types.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/plyr_anim_pdata.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pebble.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"
#include "runtime/cam.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "runtime/image.h"
#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"
#include "runtime/shadow.h"
#include "runtime/sound.h"
#include "math/mk_math.h"
#include "math/gxMath.h"
#include "platform/main.h"
#include "platform/gcutils.h"
#include "rw/rpmatfx.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"

#pragma use_lmw_stmw on

extern MkFileEntry beetlelair_file_table[];
extern MkFileEntry courtyard_file_table[];
extern MkFileEntry darkprison_file_table[];
extern MkFileEntry deadpool_file_table[];
extern MkFileEntry dka_file_table[];
extern MkFileEntry dragonmountain_file_table[];
extern MkFileEntry fallingcliff_file_table[];
extern MkFileEntry fortress_file_table[];
extern MkFileEntry hellsfoundry_file_table[];
extern MkFileEntry kangtomb_file_table[];
extern MkFileEntry kuatanpalace_file_table[];
extern MkFileEntry livingforest_file_table[];
extern MkFileEntry lostwarriors_file_table[];
extern MkFileEntry lower_mines_file_table[];
extern MkFileEntry netherbelly_file_table[];
extern MkFileEntry nexus_arena_file_table[];
extern MkFileEntry portal_file_table[];
extern MkFileEntry reddragon_file_table[];
extern MkFileEntry skytemple_file_table[];
extern MkFileEntry slaughterhouse_file_table[];
extern MkFileEntry thepit_file_table[];
extern MkFileEntry yinyang_file_table[];
extern MkFileEntry ladder_file_table[];
extern MkFileEntry pz_ladder_file_table[];
extern MkFileEntry krypt_file_table[];
extern MkFileEntry kq_er1_fight_file_table[];
extern MkFileEntry kq_er2_fight_file_table[];
extern MkFileEntry kq_nr1_fight_file_table[];
extern MkFileEntry kq_cr1_fight_file_table[];
extern MkFileEntry kq_ow1_fight_file_table[];
extern MkFileEntry kq_or1_fight_file_table[];
extern MkFileEntry kq_ed1_fight_file_table[];
extern MkFileEntry kq_nx1_fight_file_table[];
extern MkFileEntry kq_nexus_nis_file_table[];

GlobalBackgroundEntry global_background_data[] = {
    {beetlelair_file_table, "beetlelair.mko", 0x00000012, 0x00000001},
    {courtyard_file_table, "courtyard.mko", 0x0000001C, 0x00000009},
    {darkprison_file_table, "darkprison.mko", 0x00000024, 0x00000001},
    {deadpool_file_table, "deadpool.mko", 0x00000020, 0x00000009},
    {dka_file_table, "dka.mko", 0x00000026, 0x00000001},
    {dragonmountain_file_table, "dragonmountain.mko", 0x0000001A, 0x00000001},
    {fallingcliff_file_table, "fallingcliff.mko", 0x0000001B, 0x00000001},
    {fortress_file_table, "fortress.mko", 0x00000022, 0x00000001},
    {hellsfoundry_file_table, "hellsfoundry.mko", 0x00000019, 0x00000001},
    {kangtomb_file_table, "kangtomb.mko", 0x00000013, 0x00000001},
    {kuatanpalace_file_table, "kuatanpalace.mko", 0x00000023, 0x00000001},
    {livingforest_file_table, "livingforest.mko", 0x00000027, 0x00000009},
    {lostwarriors_file_table, "lostwarriors.mko", 0x00000016, 0x00000001},
    {lower_mines_file_table, "lower_mines.mko", 0x00000021, 0x00000001},
    {netherbelly_file_table, "netherbelly.mko", 0x00000017, 0x00000001},
    {nexus_arena_file_table, "nexus_arena.mko", 0x00000028, 0x00000001},
    {portal_file_table, "portal.mko", 0x0000001D, 0x00000009},
    {reddragon_file_table, "reddragon.mko", 0x00000015, 0x00000001},
    {skytemple_file_table, "skytemple.mko", 0x00000018, 0x00000001},
    {slaughterhouse_file_table, "slaughterhouse.mko", 0x00000014, 0x00000001},
    {thepit_file_table, "thepit.mko", 0x0000001E, 0x00000009},
    {yinyang_file_table, "yinyang.mko", 0x0000001F, 0x00000001},
    {ladder_file_table, "ladder.mko", 0x00000000, 0x00000020},
    {pz_ladder_file_table, "pz_ladder.mko", 0x00000000, 0x00000020},
    {krypt_file_table, "krypt.mko", 0x00000000, 0x00000010},
    {kq_er1_fight_file_table, "earth_1.mko", 0x00000000, 0x00000004},
    {kq_er1_fight_file_table, "earth_1.mko", 0x00000000, 0x00000004},
    {kq_er2_fight_file_table, "earth_2.mko", 0x00000000, 0x00000004},
    {kq_nr1_fight_file_table, "netherrealm_1.mko", 0x00000000, 0x00000004},
    {kq_cr1_fight_file_table, "chaosrealm_1.mko", 0x00000000, 0x00000004},
    {kq_ow1_fight_file_table, "outworld_1.mko", 0x00000000, 0x00000004},
    {kq_or1_fight_file_table, "orderrealm_1.mko", 0x00000000, 0x00000004},
    {kq_ed1_fight_file_table, "edenia_1.mko", 0x00000000, 0x00000004},
    {kq_nx1_fight_file_table, "nexus_1.mko", 0x00000000, 0x00000004},
    {kq_nexus_nis_file_table, "nexus_nis.mko", 0x00000000, 0x00000000},
};

typedef struct SlaughterhouseData {
    MkHdr hdr;
    MkHdrLatch lower_level_pebbles[5]; /* +0x08 */
    MkHdrLatch blood_fall_pebbles[3]; /* +0x30 */
    MslSoundHandle upper_ambient_sound; /* +0x48 */
    MslSoundHandle lower_ambient_sound_1; /* +0x4C */
    MslSoundHandle lower_ambient_sound_2; /* +0x50 */
} SlaughterhouseData;

typedef struct ShBloodFallProcessData {
    MkHdr hdr;
    char pad08[0x0C];
    Vec origin;        /* +0x14 */
    float gravity;     /* +0x20 */
    int active_splats; /* +0x24 */
} ShBloodFallProcessData; /* 0x28 */

typedef struct ShFatalityBodyPartsProcessData {
    MkHdr hdr;
    PlyrPdata* player; /* +0x08 */
    float delay; /* +0x0C */
} ShFatalityBodyPartsProcessData; /* 0x10 */

typedef struct ShBloodPebbleControl {
    Vec position;         /* +0x00 */
    Vec angles;           /* +0x0C */
    Vec velocity;         /* +0x18 */
    Vec scale;            /* +0x24 */
    float lifetime;       /* +0x30 */
    float angle_degrees;  /* +0x34 */
    float angle_step;     /* +0x38 */
    int state;            /* +0x3C */
} ShBloodPebbleControl; /* 0x40 */
typedef char ShBloodPebbleControlSize[
    (sizeof(ShBloodPebbleControl) == 0x40) ? 1 : -1];
typedef struct BgndObstacleEventData BgndObstacleEventData;
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

typedef struct BgndDisplayedItem {
    MkHdr hdr;                   /* +0x00 */
    int type;                    /* +0x08 */
    MkObj* primary_object;       /* +0x0C */
    MkObj* secondary_object;     /* +0x10 */
    MkSobj* display_sobj;        /* +0x14 */
    MkSobj* secondary_sobj;      /* +0x18 */
    int object_id;               /* +0x1C */
    int field_20;                /* +0x20 */
    int sobj_id;                 /* +0x24 */
    int field_28;                /* +0x28 */
    int source_sobj_id;          /* +0x2C */
    int field_30;                /* +0x30 */
    Vec position;                /* +0x34 */
    Vec angles;                  /* +0x40 */
    Vec field_4C;                /* +0x4C */
    Vec field_58;                /* +0x58 */
    Vec collision_center;        /* +0x64 */
    float collision_radius;      /* +0x70 */
    float collision_height;      /* +0x74 */
    float field_78;               /* +0x78 */
    int flags;                    /* +0x7C */
    ArenaObstacle* obstacle;     /* +0x80 */
    union {
        unsigned int placement_flags; /* +0x84 */
        struct {
            unsigned int placement_id : 31;
            unsigned int flag_0 : 1;
        };
    };
} BgndDisplayedItem;
typedef char BgndDisplayedItemSize[
    (sizeof(BgndDisplayedItem) == 0x88) ? 1 : -1];

typedef struct BgndUvScrollControlItem {
    UvScrollControl* control;
    unsigned int instance;
} BgndUvScrollControlItem;

typedef struct BgndActAtTimeData {
    MkHdr hdr;
    int ticks;
    int script_function;
    Vec parameters;
    float ground_plane;
} BgndActAtTimeData;

typedef struct BgndFadeObjectData {
    MkHdr hdr;
    MkSobj* object;
    char pad0C[0x0C];
    float fade_step;
    int complete;
    char pad20[4];
    float alpha;
    unsigned int alpha_int;
    char pad2C[0x20];
} BgndFadeObjectData;

typedef struct BgndPulsateData {
    MkHdr hdr;                 /* +0x00 */
    MkSobj* object;            /* +0x08 */
    int field_0C;
    float field_10;
    int field_14;
    float field_18;
    int field_1C;
    int field_20;
    float alpha;               /* +0x24 */
    unsigned int alpha_int;    /* +0x28 */
    unsigned int field_2C;
    unsigned int field_30;
    float field_34;
    float field_38;
    float field_3C;
    float field_40;
    float field_44;
    float field_48;
} BgndPulsateData; /* 0x4C */

typedef struct BgndPointLightLifeData {
    MkHdr hdr;
    int ticks;
    MkObj* light;
    float radius_step;
} BgndPointLightLifeData; /* 0x14 */

typedef struct BlBeetlePdata {
    MkHdr hdr;
    PebbleData* pebble_data;
} BlBeetlePdata; /* 0x0C */

typedef struct BlBeetleControl {
    Vec position;                 /* +0x00 */
    Vec scale;                    /* +0x0C */
    unsigned int personality;     /* +0x18 */
    int personality_ticks;        /* +0x1C */
    int movement_state;           /* +0x20 */
    int wall_state;               /* +0x24 */
    int wall_ticks;               /* +0x28 */
    int heading_ticks;            /* +0x2C */
    int transition_ticks;         /* +0x30 */
    float speed_scale;            /* +0x34 */
    float heading_degrees;        /* +0x38 */
    float heading_step;           /* +0x3C */
    float movement_delta_a;       /* +0x40 */
    float movement_delta_b;       /* +0x44 */
    Vec wall_target;              /* +0x48 */
    Vec movement_target;          /* +0x54 */
    float distance_limit_sq;      /* +0x60 */
    int bounce_ticks;             /* +0x64 */
    int field_68;                 /* +0x68 */
    float vertical_velocity;      /* +0x6C */
    int fast_motion;              /* +0x70 */
    int surface;                  /* +0x74 */
} BlBeetleControl; /* 0x78 */
typedef char BlBeetleControlSize[
    (sizeof(BlBeetleControl) == 0x78) ? 1 : -1];

static BgndUvScrollControlItem bgnd_uv_scroll_control_item[8];

void bgnd_delete_danger_zone(unsigned int zone_index);
void bgnd_enable_danger_zone(unsigned int zone_index, int enabled);
void bgnd_collision_if_monitor_col_as(
    int list_index, unsigned int collision_id,
    unsigned int script_function, int monitor_type);
static float p_hide_walls(void);
static float p_act_at_time(void);
static float p_bgnd_fade_object(void);
static float p_bgnd_pulsate_object(void);
static float p_pulsate_object(void);
static float p_bgnd_point_light_life_span(void);
static float p_bl_beetle_brains(void);
static int beetle_squashed(
    BlBeetleControl* beetle, PlyrInfo** squashing_player);
static void bl_init_beetle_pebbles_first_floor(BlBeetlePdata* data);
static void bl_process_beetle_follow_plyr_personality(
    BlBeetleControl* beetle);
static void bl_process_beetle_under_glass_personality(
    BlBeetleControl* beetle);
static void bl_process_beetle_under_glass_traveller_personality(
    BlBeetleControl* beetle);
static void bl_process_beetle_runaway_personality(
    BlBeetleControl* beetle);
static void bl_process_beetle_transition_personality(
    BlBeetleControl* beetle);
static void bl_process_beetle_chilling(BlBeetleControl* beetle);
static void bl_process_beetle_track_plyr(BlBeetleControl* beetle);
static void bl_process_beetle_climb_a_wall(BlBeetleControl* beetle);
static void bl_process_general_movement(
    BlBeetleControl* beetle, const Vec* target, int heading_ticks,
    float distance_limit_sq, float heading_offset, float heading_divisor,
    float movement_scale_a, float movement_scale_b, int surface);





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
    unsigned int event_id; /* +0x00 */
    int field_04;
    Vec* impact_vector;  /* +0x08 */
    PlyrPdata* player_pdata; /* +0x0C */
    union {
        unsigned int flags;
        struct {
            unsigned int player_side : 1; /* bit31 */
            unsigned int pad_low : 31;
        } flag_bits;
    }; /* +0x10 */
};

struct BgndWallHiderRuntime {
    MkHdr hdr;
    int walls_to_hide[8];       /* +0x08 */
    unsigned int hide_count;    /* +0x28 */
    int walls_to_unhide[8];     /* +0x2C */
    unsigned int unhide_count;  /* +0x4C */
    unsigned int hidden_effects[4]; /* +0x50 */
    unsigned int hidden_effect_count; /* +0x60 */
    Vec normal;                      /* +0x64 */
    float normal_distance;           /* +0x70 */
    int normal_flags;                /* +0x74 */
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
    int bounce_ticks;          /* +0x60 */
    unsigned int launch_ticks; /* +0x64 */
    float gravity;             /* +0x68 */
    unsigned int bounce_param; /* +0x6C - command-script function index */
} BgndPebbleControl;

typedef struct BgndPebbleCollection {
    MkHdr hdr;
    MKMATRIX* matrices;              /* +0x08, stride 0x40 */
    int count;
    int active_count;
    PebbleRenderData* render_data;
    BgndPebbleControl* pebbles; /* +0x18, stride 0x70 */
    PebbleFlags* flags;
} BgndPebbleCollection;

typedef struct BgndPebblePlayerData {
    MkHdr hdr;
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
    int active; /* +0x0C */
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
int is_sobj_hidden(void* sobj);
void update_mksobj(MkSobj* sobj);
void set_arena_obstacle_callback(BgndArenaObstacleCallback callback);
void reset_collision_system(void);
void drone_ai_ok_to_think(void);
void drone_ai_dont_think(void);
void turn_controllers_off(void);
void turn_controllers_on(void);
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots);
void face_opponent_now(void);
void danger_zone_eligible_on(void);
void init_air_move_no_aniproc(void);
void launch_me_up(float vertical_velocity, float gravity);
void high_flash_check(void);
void ani_loop_more_frames(float frames);
void damage_me(float amount);
void damage_player(PlyrPdata* player, float amount);
void toggle_danger_zone(int enabled);
void fx_hide(unsigned int handle, int hidden);
static int bgnd_collision_to_script_interface(BgndObstacleEventData* event);
static float p_bgnd_launch_sobj_monitor(void);
static float p_bgnd_launch_chunk_monitor(void);
static float p_pebble_manual_monitor(void);
static float p_pebble_burst_monitor(void);
static float p_pebble_path_monitor(void);
static float p_crack_placer(void);
static float p_bgnd_timer_monitor(void);
static float p_bgnd_script_in_proc(void);
static void bgnd_pebble_burst_at(int player, const Vec* position,
                                 unsigned int first, unsigned int end);
int fx_by_owner(const char* name, int owner);
int fx_next_emitter(int effect);
void fx_restart_emit(unsigned int effect);
void fx_resume_emit(unsigned int effect);
int emitter_id_from_handle(unsigned int handle);
void resume_effect(const char* name);
void reset_effect(const char* name);
MkObj* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z);
void fx_reset(unsigned int effect);
void fx_set_param_v3(
    unsigned int effect, int parameter, float x, float y, float z);
void start_blood_particles(int effect, int bone_id, PlyrPdata* player,
                           void* limb);
int get_first_shape_center_for_obstacle_id(unsigned int obstacle_id,
                                           Vec* center);
void set_collision_render_state(int enabled);
void shake_camera(int amplitude, float duration);
int snd_req_vol(int sound_id, float volume);
double pow(double base, double exponent);
MkPfx* find_pfx_by_name(const char* name);
void move_player(MkObj* object, const Vec* position, const Vec* angles);
MkProc* get_player_proc(void* object);
void run_reaction_cleanup_function(PlyrPdata* player);
static float bgnd_call_script_function(void);
static int launch_sobj_watch_dist_from_orgin(BgndSobjLaunchEntry* entry,
                                              unsigned int distance_squared);
static int launch_sobj_watch_y_far_down(BgndSobjLaunchEntry* entry,
                                         unsigned int unused);
static int launch_sobj_watch_y_ground_plane(BgndSobjLaunchEntry* entry,
                                             unsigned int unused);

extern int nb_slave_bones[];
extern int konquest_npc_bones[];
extern MkFlippedBoneMap flipped_nb_slave_bones;
extern RwCamera* Camera;
extern float fog_density;
extern float fog_distance;
extern float fog_color_real[4];
extern int fog_type;
extern char bgnd_animations[0x84];
typedef struct BgndReactionInfo {
    PlyrInfo* player_info;
    int reaction;
    unsigned int power_level;
    unsigned int flags;
    unsigned int handler;
    int handler_enabled;
} BgndReactionInfo;

extern BgndReactionInfo g_current_reaction_info;
extern MkPtr* g_bgnd_collision_to_script_if[8];
extern BgndObstacleEventData* g_active_obstacle_event_data;
extern BgndCollisionItem* g_active_bgnd_col_item;
extern MkPtr* weapon_trail_light_list;
extern ProfileUnlockBits64 default_bgnd_bits;
extern ProfileUnlockBits64 default_pz_bgnd_bits;
extern PebbleData* g_bl_beetles;
extern BlBeetlePdata* g_bl_beetles_pdata;
extern SlaughterhouseData* g_slaughterhouse_pdata;
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
extern float r_call_script_function(void);
extern int intro_done(void);
extern Vec g_bgnd_scratch_pad_vectors[9];
extern MkObj* g_latest_obj_pfx;
extern BgndSobjLaunchEntry* g_launched_sobj_crossing_plane_pdata;
extern BgndSobjLaunchEntry* g_active_launched_sobj_pdata;
extern BgndSobjLaunchMonitor* g_sobj_launch_monitor_pdata;
extern BgndChunkLaunchMonitor* g_chunk_launch_monitor_pdata;
extern PebbleData* g_bgnd_cracks;
extern unsigned int g_bgnd_last_crack_overwritten;
extern void* obj_start_morph(MkObj* object, unsigned int sobj_id,
                             MorphScript* script, unsigned int flags);
float bgnd_process_collision_info(
    unsigned int operation, float value1, float value2, float value3,
    float value4, float value5, float value6, float value7, float value8);
extern void sh_set_grinder_speed(float speed);
extern void sh_spawn_grinder_crush_pfx(void);
extern void sh_start_grinder_crush_blood(Vec* position, PlyrPdata* player,
                                         MkObj* object);
extern void sh_start_grinder_crush_chunks(Vec* position, PlyrPdata* player);
extern void sh_start_grinder_meat_spew(Vec* position, PlyrPdata* player);
extern void sh_start_grinder_chunk_spew(Vec* position, PlyrPdata* player);
extern void hide_player(PlyrPdata* player, int hide_weapons);
extern void obj_set_pos_vel(MkObj* object, void* velocity);
extern void set_ani_speed(float speed);
extern int random_voice(int group);
extern void random_hit(int group);
extern MkProc* plyr_anim_proc;
extern unsigned char shared_ani[];
extern void delete_obstacle_from_background_by_id(int obstacle_id);
extern MkObj* load_weapon_from_slot(WeaponDefinition* definition, int slot);
extern void load_bgnd_fstyle_sign(int player);
extern void insert_ground_me_mkobj(MkObj* object);
extern AnimScript** bgnd_animation_table;
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
    unsigned int i;
    BgndCrack* cracks;

    g_bgnd_last_crack_overwritten = 0;
    cracks = g_bgnd_cracks->user_data;
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
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

BgndPebbleCollection* g_pebbles[20] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
BgndPebblePlayerData* g_pebbles_pdata[20] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


void RwImageSetGamma(float gamma);
void init_misc_bgnd_data(void);
void load_background_anims(void* anims, int bgnd_id);
void init_weapon_trail_light_list(void);
int mslSoundIsValid(MslSoundHandle sound);
void snd_stop(MslSoundHandle sound);
extern MkVtable5 vtbl_slaughterhouse_pdata;
void set_background_color(int r, int g, int b, int a);
void turn_fog_on(void);
void turn_fog_off(void);
void initialize_bgnd_collisions(void* data);
int load_effect_bank(char* name);
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
/* TODO: [breakthrough] 64.375%; canonical 64-bit unlock owner recovered; mask lifetime and return CFG remain. */
int is_bgnd_locked(int bgnd_id) {
    unsigned long long unlocked;
    unsigned long long mask;

    if (bgnd_id < 0 || bgnd_id > 0x23) {
        return 1;
    }

    mask = 1ULL << bgnd_id;
    if (mode_of_play == 6) {
        unlocked = gp_data.pz_bgnds.value | default_pz_bgnd_bits.value;
        return (unlocked & mask) == 0;
    }

    if ((g_game_info.field_04 & 0x80) != 0 && (g_game_info.field_04 & 0x40) == 0) {
        return 0;
    }

    unlocked = gp_data.cat3.value | default_bgnd_bits.value;
    return (unlocked & mask) == 0;
}
static int bgnd_cycle_tbl[22] = {
    0, 0x13, 0x12, 0xF, 6, 0xB, 0xC, 0xE, 7, 1, 8,
    0x10, 0x14, 0x11, 9, 0x15, 2, 5, 3, 0xD, 0xA, -1
};

/* TODO: [breakthrough needed] 0%; canonical 64-bit snapshots recovered;
 * wholesale register/allocation alignment and inlined lock predicate remain. */
int get_next_bgnd(void) {
    int play_mode = mode_of_play;
    unsigned long long puzzle_bits = gp_data.pz_bgnds.value;
    int background = bgnd_cycle_tbl[g_game_info.bgnd_cycle_index];
    unsigned long long default_puzzle_bits = default_pz_bgnd_bits.value;
    unsigned long long unlocked_bits = gp_data.cat3.value;
    unsigned long long default_bits = default_bgnd_bits.value;

    for (;;) {
        int locked;

        if (background < 0 || background > 0x23) {
            locked = 1;
        } else if (play_mode == 6) {
            unsigned long long mask = 1ULL << background;
            unsigned long long unlocked = puzzle_bits | default_puzzle_bits;
            locked = (unlocked & mask) == 0;
        } else if ((g_game_info.field_04 & 0x80) != 0 &&
                   (g_game_info.field_04 & 0x40) == 0) {
            locked = 0;
        } else {
            unsigned long long mask = 1ULL << background;
            unsigned long long unlocked = unlocked_bits | default_bits;
            locked = (unlocked & mask) == 0;
        }
        if (!locked) {
            g_game_info.bgnd_cycle_index++;
            if (bgnd_cycle_tbl[g_game_info.bgnd_cycle_index] == -1) {
                g_game_info.bgnd_cycle_index = 0;
            }
            return background;
        }
        g_game_info.bgnd_cycle_index++;
        if (bgnd_cycle_tbl[g_game_info.bgnd_cycle_index] == -1) {
            g_game_info.bgnd_cycle_index = 0;
        }
        background = bgnd_cycle_tbl[g_game_info.bgnd_cycle_index];
        if ((unsigned int)background > 0x23) {
            return 0x12;
        }
    }
}
void bgnd_force_specularity_off_for_material(
    unsigned int object_id, unsigned int material_id) {
    MkSobj* object;
    RpMaterial* material;
    RpGeometry* geometry;
    unsigned int i;
    unsigned int count;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        if (material_id != 0) {
            material = sobj_find_material_by_id(object, material_id);
            if (material != 0) {
                material->surface.specular = 0.0f;
            }
        } else {
            geometry = object->atomic->geometry;
            count = geometry->matList.numMaterials;
            for (i = 0; i < count; i++) {
                geometry->matList.materials[i]->surface.specular = 0.0f;
            }
        }
    }
}
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
    float playback_rate) {
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
        animation->step = playback_rate;
        if (active != 0) {
            set_root_and_obj_movement_weights(animation, 0.0f, 1.0f);
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
void bgnd_sobj_start_morph(MkObj* object, int sobj_id, int script_id,
                           unsigned int flags) {
    MorphScript* script;

    switch (script_id) {
    case 0:
        script = &linear_slow_script;
        break;
    case 1:
        script = &cos_script;
        break;
    case 4:
        script = &cos_fast_script;
        break;
    case 6:
        script = &skytemple_banner_script;
        break;
    default:
        script = &linear_slow_script;
        break;
    }
    obj_start_morph(object, sobj_id, script, flags);
    object->hide_flag_bits.hidden = 0;
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
void skytemple_arrange_fence_pebbles_around_pos(int player, unsigned int count,
                                                Vec* position) {
    unsigned int index;
    BgndPebbleControl* pebbles;

    pebbles = g_pebbles[player]->pebbles;
    for (index = 0; index < count; index++) {
        pebbles[index].position.x = position->x + sfrand(2.0f);
        pebbles[index].position.y = position->y + sfrand(2.0f);
        pebbles[index].position.z = position->z + sfrand(2.0f);
        pebbles[index].velocity.x = sfrand(0.05f);
        pebbles[index].velocity.z = frand(0.02f);
    }
}
void skytemple_set_fence_pebble_vel(
    int player, unsigned int count, float x, float y, float z) {
    unsigned int index;
    BgndPebbleControl* pebbles;

    pebbles = g_pebbles[player]->pebbles;
    for (index = 0; index < count; index++) {
        pebbles[index].velocity.x = x + frand(0.05f);
        pebbles[index].velocity.y = y + sfrand(0.06f);
        pebbles[index].velocity.z = z + sfrand(0.05f);
        pebbles[index].angular_velocity.x = sfrand(15.0f);
        pebbles[index].angular_velocity.y = sfrand(15.0f);
        pebbles[index].angular_velocity.z = sfrand(15.0f);
        pebbles[index].gravity = -0.002f;
    }
}
static void sh_update_fatality_body_part(MkObj* object);
extern void spawn_bld_splat(const char* name, int owner, Vec* position);

static inline MkHdr* sh_get_latched_object(MkHdrLatch* latch) {
    MkHdr* object = latch->hdr;

    if (object != 0) {
        if (object->instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

static inline float bgnd_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float product;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    input.f = value;
    estimate.u = 0x5F375A00U - (input.u >> 1);
    product = estimate.f * (value * estimate.f);
    correction = 3.0f - product;
    return 0.0625f * estimate.f * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline void sh_normalize_blood_xz(Vec* vector) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x;

    x = vector->x;
    squared = x * x + vector->z * vector->z;
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
    vector->z *= inverse_length;
}

/* TODO: [breakthrough] 91.31%; state CFG and splat copies improved; resolve movement Vec stack layout and latch branches. */
static float p_sh_fatality_body_parts(void) {
    ShFatalityBodyPartsProcessData* data;
    PebbleData* pebble_data;
    ShBloodPebbleControl* controls;
    MkObj* object;
    int index;

    data = (ShFatalityBodyPartsProcessData*)pdata_of_proc(aproc);
    if (data == 0) {
        return -1.0f;
    }
    if (g_slaughterhouse_pdata == 0) {
        return -1.0f;
    }
    if (data->delay > 0.0f) {
        data->delay -= game_speed;
        return 1.0f;
    }

    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[3]);
    if (object != 0) {
        sh_update_fatality_body_part(object);
    }
    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[4]);
    if (object != 0) {
        sh_update_fatality_body_part(object);
    }

    pebble_data = (PebbleData*)sh_get_latched_object(
        &g_slaughterhouse_pdata->blood_fall_pebbles[0]);
    if (pebble_data != 0) {
        controls = pebble_data->user_data;
        index = 0;
        while (index < pebble_data->count) {
            ShBloodPebbleControl* control = &controls[index];

            control->lifetime -= game_speed;
            if (control->lifetime <= 0.0f) {
                control->lifetime = 0.0f;
                if (control->state == 0) {
                    Vec step;

                    step.x = game_speed * control->velocity.x;
                    step.y = game_speed * control->velocity.y;
                    step.z = game_speed * control->velocity.z;
                    control->position.x += step.x;
                    control->position.y += step.y;
                    control->position.z += step.z;
                    control->velocity.y -= 0.004f;
                    control->angle_degrees += control->angle_step;
                    if (control->angle_degrees > 360.0f) {
                        control->angle_degrees = 0.0f;
                    }
                    if (control->position.z > 21.65f) {
                        control->position.z = 21.65f;
                        control->velocity.x *= 0.2f;
                        control->velocity.z *= -0.2f;
                        if ((unsigned short)randu0(10) <= 2) {
                            Vec splat_position;

                            bgnd_copy_vector(&splat_position, &control->position);
                            splat_position.z = 21.65f;
                            spawn_bld_splat(
                                "sh_bloodsplat", 0, &splat_position);
                            control->state = 2;
                            control->position.z = 21.6f;
                            control->velocity.x = 0.0f;
                            control->velocity.y = 0.0f;
                            control->velocity.z = 0.0f;
                            control->velocity.y = -0.001f;
                            control->angle_step = 90.0f;
                        }
                    }
                    if (control->position.y <= -15.5f) {
                        control->state = 1;
                        control->position.y = -15.5f;
                        control->velocity.y = 0.015f + frand(0.07f);
                        control->angles.x = sfrand(1.0f);
                        control->angles.y = sfrand(1.0f);
                        control->angles.z = sfrand(1.0f);
                        control->angle_degrees = frand(360.0f);
                    }
                } else if (control->state == 1) {
                    control->velocity.x *= 0.97f;
                    if (control->position.y < -15.5f) {
                        control->position.y = -15.5f;
                        if (control->velocity.y < 0.0f) {
                            control->velocity.y *= -0.2f;
                        }
                    } else if (control->position.y > -15.3f) {
                        if (control->velocity.y > 0.0f) {
                            control->velocity.y *= -0.3f;
                        }
                    }
                    if (control->velocity.z > -0.01f) {
                        control->velocity.z -= 0.0002f;
                    } else {
                        control->velocity.z *= 0.97f;
                    }
                    control->position.x += control->velocity.x;
                    control->position.y += control->velocity.y;
                    control->position.z += control->velocity.z;
                    if (control->lifetime < 90.0f) {
                        control->position.y -= 0.002f;
                    }
                } else if (control->state == 2) {
                    control->position.y += game_speed * control->velocity.y;
                    control->angle_step -= 1.0f;
                    if (control->angle_step <= 0.0f) {
                        Vec splat_position;

                        control->angle_step = 90.0f;
                        bgnd_copy_vector(&splat_position, &control->position);
                        splat_position.z = 21.65f;
                        spawn_bld_splat("sh_bloodsplat", 0, &splat_position);
                    }
                    if (control->velocity.y > -0.003f) {
                        control->velocity.y -= 0.0001f;
                    }
                    if (control->position.y < -15.4f) {
                        control->state = 1;
                    }
                }
            }
            MKMatrixRotateScaleTranslate(
                &pebble_data->pebbles[index].matrix, &control->angles,
                control->angle_degrees, &control->scale, &control->position);
            index++;
        }
    }

    pebble_data = (PebbleData*)sh_get_latched_object(
        &g_slaughterhouse_pdata->blood_fall_pebbles[2]);
    if (pebble_data != 0) {
        ShBloodPebbleControl* control =
            pebble_data->user_data;

        control->lifetime -= game_speed;
        if (control->lifetime <= 0.0f) {
            Vec step;

            control->lifetime = 0.0f;
            if (control->state == 0) {
                step.x = control->velocity.x * game_speed;
                step.y = control->velocity.y * game_speed;
                step.z = control->velocity.z * game_speed;
                control->position.x += step.x;
                control->position.y += step.y;
                control->position.z += step.z;
                control->velocity.y -= 0.004f;
                control->angle_degrees += control->angle_step;
                if (control->angle_degrees > 360.0f) {
                    control->angle_degrees = 0.0f;
                }
                if (control->position.z > 21.65f) {
                    control->position.z = 21.65f;
                    control->velocity.x = camera_obj->pos.x - control->position.x;
                    control->velocity.z = camera_obj->pos.z - control->position.z;
                    sh_normalize_blood_xz(&control->velocity);
                    control->velocity.x *= 0.07f;
                    control->velocity.z *= 0.07f;
                    control->velocity.y = -0.002f;
                    control->state = 1;
                }
            } else {
                step.x = control->velocity.x * game_speed;
                step.y = control->velocity.y * game_speed;
                step.z = control->velocity.z * game_speed;
                control->position.x += step.x;
                control->position.y += step.y;
                control->position.z += step.z;
                control->velocity.y -= 0.001f;
                control->angle_degrees += control->angle_step;
                if (control->angle_degrees > 360.0f) {
                    control->angle_degrees = 0.0f;
                }
            }
            MKMatrixRotateScaleTranslate(
                &pebble_data->pebbles[0].matrix, &control->angles,
                control->angle_degrees, &control->scale, &control->position);
        }
    }
    return 1.0f;
}
static void sh_update_fatality_body_part(MkObj* object) {
    object->flags_08_bits.airborne = 1;
    object->flags_08_bits.gravity_enabled = 1;
    object->flags_08_bits.angular_velocity_enabled = 1;
    object->flags_08_bits.rotation_enabled = 1;

    if (object->flags_08_bits.moving) {
        if (object->pos.value.z > 21.65f) {
            object->pos.value.z = 21.65f;
            object->pos_vel.x *= 0.3f;
            object->pos_vel.z *= -0.1f;
            object->ang_vel.x = 0.5f * object->ang_vel.x;
            object->ang_vel.y = 0.5f * object->ang_vel.y;
            object->ang_vel.z = 0.5f * object->ang_vel.z;
        }
        if (object->pos.value.y <= -15.6f) {
            object->flags_08_bits.moving = 0;
            object->ang_vel.z = 0.0f;
            object->ang_vel.y = 0.0f;
            object->ang_vel.x = 0.0f;
            object->pos.value.y = -15.6f;
            object->pos_vel.y = 0.03f + frand(0.07f);
            object->ang.x = sfrand(3.1415927f);
            object->ang.y = sfrand(3.1415927f);
            object->ang.z = sfrand(3.1415927f);
        }
    } else {
        object->pos_vel.x *= 0.97f;
        if (object->pos.value.y < -15.6f) {
            object->pos.value.y = -15.6f;
            if (object->pos_vel.y < 0.0f) {
                object->pos_vel.y *= -0.6f;
            }
        } else if (object->pos.value.y > -15.3f && object->pos_vel.y > 0.0f) {
            object->pos_vel.y *= -0.4f;
        }
        if (object->pos_vel.z > -0.01f) {
            object->pos_vel.z -= 0.0002f;
        } else {
            object->pos_vel.z *= 0.97f;
        }
    }
}
static inline void sh_normalize_xz(Vec* vector) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x_squared;
    float z_squared;

    x_squared = vector->x * vector->x;
    z_squared = vector->z * vector->z;
    squared = x_squared + z_squared;
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
    vector->x *= inverse_length;
    vector->z *= inverse_length;
}

static inline void sh_normalize_fatality_vector(Vec* vector) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x;

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

/* TODO: [near miss] 79.05%; ownership, RNG calls and fragment setup agree; latch-merge CSE and aggregate-copy scheduling remain (retail 2416 vs 2280 bytes). */
static void sh_start_fatality_body_parts(PlyrPdata* player) {
    ShFatalityBodyPartsProcessData* process_data;
    MkObj* source;
    MkObj* object;
    PebbleData* pebble_data;
    ShBloodPebbleControl* controls;
    MkProc* process;

    process_data = 0;
    process = _create_mkproc_generic_nostack(
        0xA011, 0x1F, p_sh_fatality_body_parts,
        sizeof(ShFatalityBodyPartsProcessData), (MkHdr**)&process_data);
    if (process == 0) {
        return;
    }

    source = player->tracked_obj;
    if (source != 0 && source->hdr.instance != player->tracked_obj_instance) {
        source = 0;
    }

    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[3]);
    if (object != 0) {
        float speed = 0.2f + frand(0.2f);

        object->pos.value = source->pos.value;
        object->flags_08 &= (unsigned char)~(0x40 | 0x20 | 8 | 4);
        object->pos_vel.x = sfrand(0.2f);
        object->pos_vel.y = 0.25f + sfrand(0.4f);
        object->pos_vel.z = 1.0f + frand(0.5f);
        sh_normalize_fatality_vector(&object->pos_vel);
        object->pos_vel.x *= speed;
        object->pos_vel.y *= speed;
        object->pos_vel.z *= speed;
        object->ang_vel.x = sfrand(0.05f);
        object->ang_vel.y = sfrand(0.05f);
        object->ang_vel.z = sfrand(0.05f);
        object->flags_08_bits.moving = 1;
        object->gravity = -0.003f;
        unhide_obj(object);
    }

    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[4]);
    if (object != 0) {
        float speed = 0.2f + frand(0.2f);

        object->pos.value = source->pos.value;
        object->flags_08 &= (unsigned char)~(0x40 | 0x20 | 8 | 4);
        object->pos_vel.x = sfrand(0.2f);
        object->pos_vel.y = 0.25f + sfrand(0.4f);
        object->pos_vel.z = 1.0f + frand(0.5f);
        sh_normalize_fatality_vector(&object->pos_vel);
        object->pos_vel.x *= speed;
        object->pos_vel.y *= speed;
        object->pos_vel.z *= speed;
        object->ang_vel.x = sfrand(0.05f);
        object->ang_vel.y = sfrand(0.05f);
        object->ang_vel.z = sfrand(0.05f);
        object->flags_08_bits.moving = 1;
        object->gravity = -0.003f;
        unhide_obj(object);
    }

    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[0]);
    if (object != 0) {
        unhide_obj(object);
    }
    pebble_data = (PebbleData*)sh_get_latched_object(
        &g_slaughterhouse_pdata->blood_fall_pebbles[0]);
    if (pebble_data != 0) {
        int index;

        controls = pebble_data->user_data;
        index = 0;
        while (index < pebble_data->count) {
            ShBloodPebbleControl* control = &controls[index];
            float speed = 0.2f + frand(0.2f);
            float scale_value = 1.0f + frand(0.8f);
            Vec scale = {0.5f, 0.5f, 0.5f};

            scale.x *= scale_value;
            scale.y *= scale_value;
            scale.z *= scale_value;
            control->position = source->pos.value;
            control->velocity.x = sfrand(0.5f);
            control->velocity.y = 0.5f + sfrand(0.8f);
            control->velocity.z = 1.0f + frand(0.5f);
            sh_normalize_fatality_vector(&control->velocity);
            control->velocity.x *= speed;
            control->velocity.y *= speed;
            control->velocity.z *= speed;
            control->angles.x = sfrand(0.8f);
            control->angles.y = sfrand(0.8f);
            control->angles.z = sfrand(0.8f);
            control->state = 0;
            control->angle_degrees = frand(360.0f);
            control->angle_step = 3.0f + frand(2.0f);
            control->scale = scale;
            MKMatrixScale(&pebble_data->pebbles[index].matrix, &scale, 0);
            control->lifetime = frand(60.0f);
            index++;
        }
    }

    object = (MkObj*)sh_get_latched_object(
        &g_slaughterhouse_pdata->lower_level_pebbles[2]);
    if (object != 0) {
        unhide_obj(object);
    }
    pebble_data = (PebbleData*)sh_get_latched_object(
        &g_slaughterhouse_pdata->blood_fall_pebbles[2]);
    if (pebble_data != 0) {
        ShBloodPebbleControl* control;
        float speed;
        Vec scale = {1.5f, 1.5f, 1.5f};

        speed = 0.2f + frand(0.2f);
        pebble_data->count = 1;
        control = pebble_data->user_data;
        control->position = source->pos.value;
        control->velocity.x = sfrand(0.3f);
        control->velocity.y = 0.5f + sfrand(0.5f);
        control->velocity.z = 1.0f + frand(0.5f);
        sh_normalize_fatality_vector(&control->velocity);
        control->velocity.x *= speed;
        control->velocity.y *= speed;
        control->velocity.z *= speed;
        control->angles.x = sfrand(0.8f);
        control->angles.y = sfrand(0.8f);
        control->angles.z = sfrand(0.8f);
        control->state = 0;
        control->angle_degrees = frand(360.0f);
        control->angle_step = 3.0f + frand(2.0f);
        control->scale = scale;
        MKMatrixScale(&pebble_data->pebbles[0].matrix, &scale, 0);
        control->lifetime = 0.0f;
    }
    process_data->delay = 30.0f + frand(60.0f);
    process_data->player = player;
}
static float p_sh_throw_plyr_in_grinder(void) {
    static float scale = 0.16f;
    Vec target;
    Vec zero_velocity;
    float inverse_length;
    float x;
    MkSobj* first;
    MkSobj* second;
    float distance;
    float second_x;
    float second_z;
    int voice;

    drone_ai_dont_think();
    turn_controllers_off();
    g_game_info.flag_bits.level_fatality_active = 1;
    g_game_info.flag_bits.level_transition_active = 0;
    plyr_weapon_trail_hide(g_game_info.plyr0.slot.pdata->mirror_slots);
    plyr_weapon_trail_hide(g_game_info.plyr1.slot.pdata->mirror_slots);
    g_game_info.plyr0.slot.pdata->collision_disabled = 1;
    g_game_info.plyr1.slot.pdata->collision_disabled = 1;
    g_game_info.field_200 = 0x18;
    bgnd_process_collision_info(9, 0.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    bgnd_process_collision_info(7, 0.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    zero_velocity.x = 0.0f;
    zero_velocity.y = 0.0f;
    zero_velocity.z = 0.0f;
    obj_set_pos_vel(g_game_info.player_objects[1], &zero_velocity);
    destroy_mkprocs_pid(0xA005);
    destroy_mkprocs_pid(0xA00D);
    destroy_mkprocs_pid(0xA00F);
    destroy_mkprocs_pid(0xA00E);
    destroy_mkprocs_pid(0xA010);
    destroy_mkprocs_pid(0xA011);
    hide_sobj(obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x14));
    hide_sobj(obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x15));

    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    sh_set_grinder_speed(0.15f);
    plyr_pdata->state_flags.bits.bit3 = 1;
    plyr_obj->flags_09_bits.launched = 0;
    plyr_obj->flags_09_bits.bit6 = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_09_bits.head_tracking = 0;
    plyr_obj->flags_0B_bits.bit6 = 1;
    plyr_obj->flags_0B_bits.bit3 = 1;

    first = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    target.x = first->pos.x;
    target.y = first->pos.y;
    target.z = first->pos.z;
    second = obj_find_sobj_by_id(g_game_info.bgnd_obj, 2);
    second_x = second->pos.x;
    second_z = second->pos.z;
    target.x -= second_x;
    target.z -= second_z;
    target.x = 0.5f * target.x;
    target.z = 0.5f * target.z;
    target.x += second_x;
    target.z += second_z;
    plyr_obj->pos_vel.x = target.x - plyr_obj->pos.value.x;
    plyr_obj->pos_vel.z = target.z - plyr_obj->pos.value.z;
    x = plyr_obj->pos_vel.x;
    inverse_length = bgnd_inv_sqrt(
        (float)(x * x) +
        (float)(plyr_obj->pos_vel.z * plyr_obj->pos_vel.z));
    plyr_obj->pos_vel.x = x * inverse_length;
    plyr_obj->pos_vel.z *= inverse_length;
    plyr_obj->pos_vel.x *= scale;
    plyr_obj->pos_vel.z *= scale;
    distance = dist_xz_to_xz(&plyr_obj->pos.value, &target);
    plyr_obj->pos_vel.y = 0.2f * (1.0f / distance);
    plyr_obj->flags_08_bits.moving = 0;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(*(AniData**)&shared_ani[0x44], 0, 1.0f);
    run_camera_script(g_game_info.cmdscript, 0x1F, 0);
    random_hit(4);
    do {
        distance = dist_xz_to_xz(&plyr_obj->pos.value, &target);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    } while (distance > 0.21f && plyr_obj->pos.value.z < target.z);

    plyr_obj->gravity = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos.value.x = target.x;
    plyr_obj->pos.value.z = target.z;
    target.y = plyr_obj->pos.value.y;
    sh_start_grinder_crush_blood(&target, plyr_pdata, plyr_obj);
    sh_start_grinder_crush_chunks(&target, plyr_pdata);
    if (g_slaughterhouse_pdata->lower_ambient_sound_2 != 0) {
        snd_stop(g_slaughterhouse_pdata->lower_ambient_sound_2);
        g_slaughterhouse_pdata->lower_ambient_sound_2 = 0;
    }
    sh_set_grinder_speed(-0.002f);
    random_voice(3);
    snd_req(0x146);
    snd_req(0x149);
    random_hit(5);
    set_ani_speed(0.1f);
    sh_spawn_grinder_crush_pfx();
    _mkproc_sleep_ticks = 80.0f;
    aproc->vtbl->sleep();
    sh_set_grinder_speed(0.2f);
    set_ani_speed(2.5f);
    _mkproc_sleep_ticks = 5.0f;
    aproc->vtbl->sleep();
    sh_set_grinder_speed(-0.002f);
    voice = random_voice(0xE);
    snd_req(0x147);
    snd_req(0x149);
    random_hit(5);
    sh_spawn_grinder_crush_pfx();
    _mkproc_sleep_ticks = 80.0f;
    aproc->vtbl->sleep();
    sh_set_grinder_speed(0.22f);
    sh_start_grinder_meat_spew(&target, plyr_pdata);
    sh_start_grinder_chunk_spew(&target, plyr_pdata);
    sh_start_fatality_body_parts(plyr_pdata);
    snd_req(0x145);
    snd_stop(voice);
    snd_req(0x146);
    random_hit(5);
    hide_player(plyr_pdata, 1);
    if (g_slaughterhouse_pdata->lower_ambient_sound_2 == 0) {
        g_slaughterhouse_pdata->lower_ambient_sound_2 = snd_req(0x148);
    }
    _mkproc_sleep_ticks = 240.0f;
    aproc->vtbl->sleep();
    kill_plyr_life(plyr_pdata->plyr_num);
    set_level_fatality_done_flag_state(1);
    reset_collision_system();
    g_game_info.flag_bits.level_fatality_active = 0;
    g_game_info.flag_bits.level_transition_active = 0;
    g_game_info.plyr0.slot.pdata->collision_disabled = 0;
    g_game_info.plyr1.slot.pdata->collision_disabled = 0;
    drone_ai_ok_to_think();
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}
static inline void sh_normalize_blood_direction(Vec* direction) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float correction;
    float inverse_length;
    float product;
    float squared;
    float x;
    float x_squared;
    float y_squared;
    float z_squared;

    x = direction->x;
    x_squared = x * x;
    y_squared = direction->y * direction->y;
    z_squared = direction->z * direction->z;
    squared = z_squared + (x_squared + y_squared);
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
    direction->x = x * inverse_length;
    direction->y *= inverse_length;
    direction->z *= inverse_length;
}

static void sh_update_blood_fall_pebbles(
    PebbleData* pebble_data, int* active_splats, const Vec* origin,
    float gravity);
static inline void sh_normalize_blood_direction(Vec* direction);
extern void spawn_bld_splat(const char* name, int owner, Vec* position);




static float p_sh_bottom_floor_blood_fall(void) {
    ShBloodFallProcessData* data;
    PebbleData* pebbles;

    data = (ShBloodFallProcessData*)pdata_of_proc(aproc);
    if (data == 0) {
        return -1.0f;
    }
    if (g_slaughterhouse_pdata == 0) {
        return -1.0f;
    }

    pebbles = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[1].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[1].instance);

    sh_update_blood_fall_pebbles(
        pebbles, &data->active_splats, &data->origin, data->gravity);

    pebbles = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[0].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[0].instance);

    sh_update_blood_fall_pebbles(
        pebbles, &data->active_splats, &data->origin, data->gravity);

    pebbles = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[2].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[2].instance);

    sh_update_blood_fall_pebbles(
        pebbles, &data->active_splats, &data->origin, data->gravity);
    return 1.0f;
}
/* TODO: [breakthrough needed] 83.77%; state dispatch now matches; constant
 * vector materialization and normalization stack layout still differ. */
static void sh_update_blood_fall_pebbles(
    PebbleData* pebble_data, int* active_splats, const Vec* origin,
    float gravity) {
    ShBloodPebbleControl* controls;
    int index;

    if (pebble_data == 0) {
        return;
    }

    controls = pebble_data->user_data;
    index = 0;
    while (index < pebble_data->count) {
        ShBloodPebbleControl* control = &controls[index];

        if (control->state == 0) {
            control->position.y += game_speed * control->velocity.y;
            control->velocity.y += gravity;
            control->angle_degrees += control->angle_step;
            if (control->angle_degrees > 360.0f) {
                control->angle_degrees = 0.0f;
            }
            if (control->position.y < origin->y - 9.15f) {
                unsigned short roll = randu0(100);

                if (roll < 50) {
                    control->state = 1;
                    control->position.y = origin->y - 9.5f - frand(1.0f);
                    control->velocity.x = control->position.x - origin->x;
                    control->velocity.y = 0.0f;
                    control->velocity.z = control->position.z - origin->z;
                    sh_normalize_blood_direction(&control->velocity);
                    if (!(control->velocity.z >= 0.0f)) {
                        control->velocity.z = -control->velocity.z;
                    }
                    control->velocity.z = -control->velocity.z;
                    control->velocity.x *= 0.1f;
                    control->velocity.y *= 0.1f;
                    control->velocity.z *= 0.1f;
                    control->velocity.y += frand(0.07f);
                    control->lifetime = 240.0f + frand(240.0f);
                    control->angles.x = sfrand(1.0f);
                    control->angles.y = sfrand(1.0f);
                    control->angles.z = sfrand(1.0f);
                    control->angle_degrees = frand(360.0f);
                } else if (roll == 99 && *active_splats < 3) {
                    Vec rotated_direction;
                    Vec direction = {0.0f, 0.0f, 1.0f};
                    float angle;
                    float distance;

                    control->state = 2;
                    (*active_splats)++;
                    sh_normalize_blood_direction(&direction);
                    direction.x *= 0.795f;
                    direction.y *= 0.795f;
                    direction.z *= 0.795f;
                    angle = sfrand(3.1415927f);
                    rotate_xz(&rotated_direction, &direction, angle);
                    if (!(angle >= 0.0f)) {
                        angle = -angle;
                    }
                    distance = angle / 3.1415927f + frand(1.5f);
                    rotated_direction.x *= distance;
                    rotated_direction.y *= distance;
                    rotated_direction.z *= distance;
                    control->position.x = origin->x + rotated_direction.x;
                    control->position.y = origin->y;
                    control->position.z = origin->z + rotated_direction.z;
                    control->position.y += 9.15f;

                    control->velocity.x = -control->position.x;
                    control->velocity.y = 0.0f;
                    control->velocity.z = 1.0f;
                    sh_normalize_blood_direction(&control->velocity);
                    control->velocity.x *= 0.06f;
                    control->velocity.z *= 0.06f;
                    control->velocity.y = -frand(0.05f) - 0.05f;
                    control->angle_degrees = frand(360.0f);
                    control->angle_step = 3.0f + frand(5.0f);
                    control->lifetime = 3.0f;
                    control->scale.x = 1.2f;
                    control->scale.y = 1.2f;
                    control->scale.z = 1.2f;
                } else {
                    Vec rotated_direction;
                    Vec direction = {0.0f, 0.0f, 1.0f};
                    float angle;
                    float distance;

                    sh_normalize_blood_direction(&direction);
                    direction.x *= 0.795f;
                    direction.y *= 0.795f;
                    direction.z *= 0.795f;
                    angle = sfrand(3.1415927f);
                    rotate_xz(&rotated_direction, &direction, angle);
                    if (!(angle >= 0.0f)) {
                        angle = -angle;
                    }
                    distance = angle / 3.1415927f + frand(1.5f);
                    rotated_direction.x *= distance;
                    rotated_direction.y *= distance;
                    rotated_direction.z *= distance;
                    control->position.x = origin->x + rotated_direction.x;
                    control->position.y = origin->y;
                    control->position.z = origin->z + rotated_direction.z;
                    control->position.y += 9.15f;
                    control->velocity.y = -frand(0.05f) - 0.05f;
                    control->angle_degrees = frand(360.0f);
                    control->angle_step = 3.0f + frand(5.0f);
                }
            }
            MKMatrixRotateScaleTranslate(
                &pebble_data->pebbles[index].matrix, &control->angles,
                control->angle_degrees, &control->scale, &control->position);
        } else if (control->state == 1) {
            control->velocity.x *= 0.97f;
            control->lifetime -= 1.0f;
            if (control->lifetime <= 0.0f) {
                Vec rotated_direction;
                Vec direction = {0.0f, 0.0f, 1.0f};
                float angle;
                float distance;

                control->state = 0;
                sh_normalize_blood_direction(&direction);
                direction.x *= 0.795f;
                direction.y *= 0.795f;
                direction.z *= 0.795f;
                angle = sfrand(3.1415927f);
                rotate_xz(&rotated_direction, &direction, angle);
                if (angle < 0.0f) {
                    angle = -angle;
                }
                distance = angle / 3.1415927f + frand(1.5f);
                rotated_direction.x *= distance;
                rotated_direction.y *= distance;
                rotated_direction.z *= distance;
                control->position.x = origin->x + rotated_direction.x;
                control->position.y = origin->y;
                control->position.z = origin->z + rotated_direction.z;
                control->position.y += 9.15f;
                control->velocity.y = -frand(0.05f) - 0.05f;
                control->angle_degrees = frand(360.0f);
                control->angle_step = 3.0f + frand(5.0f);
            }
            if (control->position.y < origin->y - 9.1f) {
                if (control->velocity.y < 0.0f) {
                    control->velocity.y *= -0.4f;
                }
            } else if (control->position.y > origin->y - 8.9f) {
                if (control->velocity.y > 0.0f) {
                    control->velocity.y *= -0.4f;
                }
            }
            if (control->velocity.z > -0.01f) {
                control->velocity.z -= 0.0002f;
            } else {
                control->velocity.z *= 0.97f;
            }
            control->position.x += control->velocity.x;
            control->position.y += control->velocity.y;
            control->position.z += control->velocity.z;
            if (control->lifetime < 90.0f) {
                control->position.y -= 0.002f;
            }
            MKMatrixRotateScaleTranslate(
                &pebble_data->pebbles[index].matrix, &control->angles,
                control->angle_degrees, &control->scale, &control->position);
        } else if (control->state == 2) {
            control->position.x += control->velocity.x;
            control->position.y += control->velocity.y;
            control->position.z += control->velocity.z;
            control->angle_degrees += control->angle_step;
            if (control->angle_degrees > 360.0f) {
                control->angle_degrees = 0.0f;
            }
            if (control->lifetime <= 0.0f) {
                control->velocity.x = 0.0f;
                control->velocity.y = 0.0f;
                control->velocity.z = 0.0f;
                control->position.y = g_game_info.field_34 + 0.15f;
                control->angle_step = 0.0f;
            } else if (control->position.y < g_game_info.field_34 + 0.15f) {
                control->velocity.y += -0.008f;
                if (control->velocity.y < 0.0f) {
                    Vec splat_position;

                    control->lifetime -= 1.0f;
                    control->velocity.y *= -0.4f;
                    splat_position = control->position;
                    splat_position.y = g_game_info.field_34;
                    spawn_bld_splat("blsplat", 0, &splat_position);
                }
            } else {
                control->velocity.y += -0.008f;
            }
            MKMatrixRotateScaleTranslate(
                &pebble_data->pebbles[index].matrix, &control->angles,
                control->angle_degrees, &control->scale, &control->position);
        }
        index++;
    }
}










static inline float sh_random_blood_pebble_direction(Vec* rotated_direction) {
    Vec direction = {0.0f, 0.0f, 1.0f};
    float angle;
    float inverse_length;

    inverse_length = bgnd_inv_sqrt(
        direction.z * direction.z +
        (direction.x * direction.x + direction.y * direction.y));
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    direction.x = 0.795f * direction.x;
    direction.y = 0.795f * direction.y;
    direction.z = 0.795f * direction.z;
    angle = sfrand(3.1415927f);
    rotate_xz(rotated_direction, &direction, angle);
    return angle;
}

/* TODO: [near miss] 99.62%; GPR assignment rotated (data retail r25, ours r31; pebble sets r30-r28). */
static void sh_init_bottom_floor_blood_fall_pebbles(
    ShBloodFallProcessData* data) {
    PebbleData* large;
    PebbleData* small;
    PebbleData* largest;

    large = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[1].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[1].instance);

    small = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[0].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[0].instance);

    largest = MK_HDR_LIVE((PebbleData*) g_slaughterhouse_pdata->blood_fall_pebbles[2].hdr, g_slaughterhouse_pdata->blood_fall_pebbles[2].instance);


    if (large != 0) {
        ShBloodPebbleControl* controls;
        int index;

        controls = large->user_data;
        index = 0;
        while (index < large->count) {
            float local_scale = 1.5f + frand(1.0f);
            ShBloodPebbleControl* control;
            Vec scale;
            Vec rotated_direction;
            float angle;
            float distance;

            scale.x = scale.y = scale.z = local_scale;
            angle = sh_random_blood_pebble_direction(&rotated_direction);
            angle = angle >= 0.0f ? angle : -angle;
            distance = angle / 3.1415927f + frand(1.5f);
            control = &controls[index];
            rotated_direction.x *= distance;
            rotated_direction.y *= distance;
            rotated_direction.z *= distance;
            control->position.x = rotated_direction.x + data->origin.x;
            control->position.y = data->origin.y;
            control->position.z = rotated_direction.z + data->origin.z;
            control->position.y += sfrand(9.15f);
            control->velocity.x = control->velocity.y =
                control->velocity.z = 0.0f;
            control->velocity.y = -0.01f - frand(0.01f);
            control->angle_degrees = frand(360.0f);
            control->angle_step = 3.0f + frand(2.0f);
            control->angles.x = control->position.x;
            control->angles.y = control->position.y;
            control->angles.z = control->position.z;
            control->angles.y = 0.0f;
            control->angles.x = -1.0f * control->angles.x;
            control->angles.y = -1.0f * control->angles.y;
            control->angles.z = -1.0f * control->angles.z;
            control->state = 0;
            control->scale.x = scale.x;
            control->scale.y = scale.y;
            control->scale.z = scale.z;
            MKMatrixScale(&large->pebbles[index].matrix, &scale, 0);
            index++;
        }
    }
    if (small != 0) {
        ShBloodPebbleControl* controls;
        int index;

        controls = small->user_data;
        index = 0;
        while (index < small->count) {
            float local_scale = 0.5f + frand(1.5f);
            ShBloodPebbleControl* control;
            Vec scale;
            Vec rotated_direction;
            float angle;
            float distance;

            scale.x = scale.y = scale.z = local_scale;
            angle = sh_random_blood_pebble_direction(&rotated_direction);
            angle = angle >= 0.0f ? angle : -angle;
            distance = angle / 3.1415927f + frand(1.5f);
            control = &controls[index];
            rotated_direction.x *= distance;
            rotated_direction.y *= distance;
            rotated_direction.z *= distance;
            control->position.x = rotated_direction.x + data->origin.x;
            control->position.y = data->origin.y;
            control->position.z = rotated_direction.z + data->origin.z;
            control->position.y += sfrand(9.15f);
            control->velocity.x = control->velocity.y =
                control->velocity.z = 0.0f;
            control->velocity.y = -0.01f - frand(0.01f);
            control->angle_degrees = frand(360.0f);
            control->angle_step = 3.0f + frand(5.0f);
            control->angles.x = control->position.x;
            control->angles.y = control->position.y;
            control->angles.z = control->position.z;
            control->angles.y = 0.0f;
            control->angles.x = -1.0f * control->angles.x;
            control->angles.y = -1.0f * control->angles.y;
            control->angles.z = -1.0f * control->angles.z;
            control->state = 0;
            control->scale.x = scale.x;
            control->scale.y = scale.y;
            control->scale.z = scale.z;
            MKMatrixScale(&small->pebbles[index].matrix, &scale, 0);
            index++;
        }
    }
    if (largest != 0) {
        ShBloodPebbleControl* controls;
        int index;

        controls = largest->user_data;
        index = 0;
        while (index < largest->count) {
            float local_scale = 2.5f + frand(0.5f);
            ShBloodPebbleControl* control;
            Vec scale;
            Vec rotated_direction;
            float angle;
            float distance;

            scale.x = scale.y = scale.z = local_scale;
            angle = sh_random_blood_pebble_direction(&rotated_direction);
            angle = angle >= 0.0f ? angle : -angle;
            distance = angle / 3.1415927f + frand(1.5f);
            control = &controls[index];
            rotated_direction.x *= distance;
            rotated_direction.y *= distance;
            rotated_direction.z *= distance;
            control->position.x = rotated_direction.x + data->origin.x;
            control->position.y = data->origin.y;
            control->position.z = rotated_direction.z + data->origin.z;
            control->position.y += sfrand(9.15f);
            control->velocity.x = control->velocity.y =
                control->velocity.z = 0.0f;
            control->velocity.y = -0.01f - frand(0.01f);
            control->angle_degrees = frand(360.0f);
            control->angle_step = 3.0f + frand(2.0f);
            control->angles.x = control->position.x;
            control->angles.y = control->position.y;
            control->angles.z = control->position.z;
            control->angles.y = 0.0f;
            control->angles.x = -1.0f * control->angles.x;
            control->angles.y = -1.0f * control->angles.y;
            control->angles.z = -1.0f * control->angles.z;
            control->state = 0;
            control->scale.x = scale.x;
            control->scale.y = scale.y;
            control->scale.z = scale.z;
            MKMatrixScale(&largest->pebbles[index].matrix, &scale, 0);
            index++;
        }
    }
}



void sh_lower_level_pebble_unhide(void) {
    MkObj* object;

    if (g_slaughterhouse_pdata != 0) {
        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[0].hdr, g_slaughterhouse_pdata->lower_level_pebbles[0].instance);
        if (object != 0) {
            unhide_obj(object);
        }

        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[1].hdr, g_slaughterhouse_pdata->lower_level_pebbles[1].instance);
        if (object != 0) {
            unhide_obj(object);
        }

        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[2].hdr, g_slaughterhouse_pdata->lower_level_pebbles[2].instance);
        if (object != 0) {
            unhide_obj(object);
        }
    }
}

void sh_lower_level_pebble_hide(void) {
    MkObj* object;

    if (g_slaughterhouse_pdata != 0) {
        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[0].hdr, g_slaughterhouse_pdata->lower_level_pebbles[0].instance);
        if (object != 0) {
            hide_obj(object);
        }

        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[1].hdr, g_slaughterhouse_pdata->lower_level_pebbles[1].instance);
        if (object != 0) {
            hide_obj(object);
        }

        object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[2].hdr, g_slaughterhouse_pdata->lower_level_pebbles[2].instance);
        if (object != 0) {
            hide_obj(object);
        }
    }
}







/* TODO: [near miss] 98.844536%; typed sobj bit4 matches; r30/r31 owner-object coloring remains. */
static void sh_load_objs(void) {
    SlaughterhouseData* data;
    MkObj* object;
    MkSobj* sobj;
    PebbleData* pebbles;

    data = g_slaughterhouse_pdata;
    if (data == 0) {
        return;
    }

    object = MK_HDR_LIVE((MkObj*) data->lower_level_pebbles[0].hdr, data->lower_level_pebbles[0].instance);

    if (object == 0) {
        object = load_model_from_slot(0x2001E, 0x013F000C, 0xA01D);
        if (object != 0) {
            data->lower_level_pebbles[0].hdr = &object->hdr;
            data->lower_level_pebbles[0].instance = object->hdr.instance;
            object->light_flags = 4;
            object->flags_08_bits.airborne = 1;
            obj_create_sobjs(object);
            insert_fgnd_mkobj(object);
            hide_obj(object);
            sobj = obj_find_sobj_by_id(object, 7);
            sobj->flags09_bits.bit4 = 1;
            pebbles = create_pebble_userdata(sobj, 20, 0x40);
            if (pebbles != 0) {
                data->blood_fall_pebbles[0].hdr = &pebbles->hdr;
                data->blood_fall_pebbles[0].instance = pebbles->hdr.instance;
            }
        }
    }

    data = g_slaughterhouse_pdata;
    object = MK_HDR_LIVE((MkObj*) data->lower_level_pebbles[1].hdr, data->lower_level_pebbles[1].instance);

    if (object == 0) {
        object = load_model_from_slot(0x2001E, 0x013F000A, 0xA01D);
        if (object != 0) {
            data->lower_level_pebbles[1].hdr = &object->hdr;
            data->lower_level_pebbles[1].instance = object->hdr.instance;
            object->light_flags = 4;
            object->flags_08_bits.airborne = 1;
            obj_create_sobjs(object);
            insert_fgnd_mkobj(object);
            hide_obj(object);
            sobj = obj_find_sobj_by_id(object, 0);
            sobj->flags09_bits.bit4 = 1;
            pebbles = create_pebble_userdata(sobj, 10, 0x40);
            if (pebbles != 0) {
                data->blood_fall_pebbles[1].hdr = &pebbles->hdr;
                data->blood_fall_pebbles[1].instance = pebbles->hdr.instance;
            }
        }
    }

    data = g_slaughterhouse_pdata;
    object = MK_HDR_LIVE((MkObj*) data->lower_level_pebbles[2].hdr, data->lower_level_pebbles[2].instance);

    if (object == 0) {
        object = load_model_from_slot(0x2001E, 0x013F0009, 0xA01D);
        if (object != 0) {
            data->lower_level_pebbles[2].hdr = &object->hdr;
            data->lower_level_pebbles[2].instance = object->hdr.instance;
            object->light_flags = 4;
            object->flags_08_bits.airborne = 1;
            obj_create_sobjs(object);
            insert_fgnd_mkobj(object);
            hide_obj(object);
            sobj = obj_find_sobj_by_id(object, 0);
            sobj->flags09_bits.bit4 = 1;
            pebbles = create_pebble_userdata(sobj, 10, 0x40);
            if (pebbles != 0) {
                data->blood_fall_pebbles[2].hdr = &pebbles->hdr;
                data->blood_fall_pebbles[2].instance = pebbles->hdr.instance;
            }
        }
    }

    object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[3].hdr, g_slaughterhouse_pdata->lower_level_pebbles[3].instance);

    if (object == 0) {
        object = load_model_from_slot(0x2001E, 0x013F000B, 0xA01E);
        g_slaughterhouse_pdata->lower_level_pebbles[3].hdr = &object->hdr;
        g_slaughterhouse_pdata->lower_level_pebbles[3].instance =
            object->hdr.instance;
        object->light_flags = 4;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        hide_obj(object);
    }

    object = MK_HDR_LIVE((MkObj*) g_slaughterhouse_pdata->lower_level_pebbles[4].hdr, g_slaughterhouse_pdata->lower_level_pebbles[4].instance);

    if (object == 0) {
        object = load_model_from_slot(0x10005, 0x20006, 0xA01F);
        g_slaughterhouse_pdata->lower_level_pebbles[4].hdr = &object->hdr;
        g_slaughterhouse_pdata->lower_level_pebbles[4].instance =
            object->hdr.instance;
        object->light_flags = 4;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        hide_obj(object);
    }
}


static inline void sh_hide_latched_object(MkHdrLatch* latch) {
    MkHdr* object;

    object = MK_LIVE(latch->hdr, latch->instance);
    if (object != 0) {
        hide_obj(object);
    }
}

void bgnd_sh_level_2(void) {
    ShBloodFallProcessData* process_data;
    MkSobj* sobj;
    MkProc* process;
    RwMatrix* matrix;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x28);
    if (g_game_info.bgnd_obj != 0) {
        sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x28);
        if (sobj != 0) {
            hide_sobj_and_children(sobj);
        }
    }
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x1E);
    if (g_game_info.bgnd_obj != 0) {
        sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x1E);
        if (sobj != 0) {
            unhide_sobj_and_children(sobj);
        }
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    sobj->flags_08_bits.angular_velocity_enabled = 1;
    sobj->ang_vel.y = 0.05f;
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 2);
    sobj->flags_08_bits.angular_velocity_enabled = 1;
    sobj->ang_vel.y = -0.05f;
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 10);
    sobj->flags_08_bits.angular_velocity_enabled = 1;
    sobj->ang_vel.z = -0.03f;
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 11);
    sobj->flags_08_bits.angular_velocity_enabled = 1;
    sobj->ang_vel.z = 0.03f;

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 6);
    process_data = 0;
    if (sobj != 0 && g_slaughterhouse_pdata != 0) {
        matrix = RwFrameGetLTM(sobj->frame);
        process = _create_mkproc_generic_tinystack(
            0xA005, 0x2E, p_sh_bottom_floor_blood_fall,
            sizeof(ShBloodFallProcessData), (MkHdr**)&process_data);
        if (process != 0) {
            process_data->active_splats = 0;
            process_data->gravity = -0.001f;
            process_data->origin.x = matrix->pos.x;
            process_data->origin.y = matrix->pos.y;
            process_data->origin.z = matrix->pos.z;
            sh_init_bottom_floor_blood_fall_pebbles(process_data);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }

    if (g_slaughterhouse_pdata != 0) {
        if (mslSoundIsValid(g_slaughterhouse_pdata->upper_ambient_sound)) {
            snd_stop(g_slaughterhouse_pdata->upper_ambient_sound);
            g_slaughterhouse_pdata->upper_ambient_sound = 0;
        }
        if (!mslSoundIsValid(g_slaughterhouse_pdata->lower_ambient_sound_1)) {
            g_slaughterhouse_pdata->lower_ambient_sound_1 = snd_req(0x13D);
        }
        if (!mslSoundIsValid(g_slaughterhouse_pdata->lower_ambient_sound_2)) {
            g_slaughterhouse_pdata->lower_ambient_sound_2 = snd_req(0x13E);
        }
        unhide_obj(MK_LIVE(g_slaughterhouse_pdata->lower_level_pebbles[0].hdr, g_slaughterhouse_pdata->lower_level_pebbles[0].instance));
        unhide_obj(MK_LIVE(g_slaughterhouse_pdata->lower_level_pebbles[1].hdr, g_slaughterhouse_pdata->lower_level_pebbles[1].instance));
        unhide_obj(MK_LIVE(g_slaughterhouse_pdata->lower_level_pebbles[2].hdr, g_slaughterhouse_pdata->lower_level_pebbles[2].instance));
    }
}
void bgnd_sh_level_1(void) {
    SlaughterhouseData* data;

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x28);
    if (g_game_info.bgnd_obj != 0) {
        MkSobj* sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x28);
        if (sobj != 0) {
            unhide_sobj_and_children(sobj);
        }
    }
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x1E);
    if (g_game_info.bgnd_obj != 0) {
        MkSobj* sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x1E);
        if (sobj != 0) {
            hide_sobj_and_children(sobj);
        }
    }

    if (g_slaughterhouse_pdata != 0) {
        sh_hide_latched_object(&g_slaughterhouse_pdata->lower_level_pebbles[3]);
        sh_hide_latched_object(&g_slaughterhouse_pdata->lower_level_pebbles[4]);
        sh_hide_latched_object(&g_slaughterhouse_pdata->lower_level_pebbles[0]);
        sh_hide_latched_object(&g_slaughterhouse_pdata->lower_level_pebbles[1]);
        sh_hide_latched_object(&g_slaughterhouse_pdata->lower_level_pebbles[2]);
    }

    destroy_mkprocs_pid(0xA005);
    destroy_mkprocs_pid(0xA00D);
    destroy_mkprocs_pid(0xA00F);
    destroy_mkprocs_pid(0xA00E);
    destroy_mkprocs_pid(0xA010);
    destroy_mkprocs_pid(0xA011);
    hide_sobj(obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x14));
    hide_sobj(obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x15));

    if (g_slaughterhouse_pdata == 0) {
        data = (SlaughterhouseData*)get_mkhdr(
            &vtbl_slaughterhouse_pdata, sizeof(SlaughterhouseData));
        if (data != 0) {
            zero_pdata_payload(sizeof(SlaughterhouseData), &data->hdr);
        }
        g_slaughterhouse_pdata = data;
        if (data == 0) {
            return;
        }
        mk_insert(&data->hdr, &g_game_info.bgnd_obj->child_list);
        sh_load_objs();
    }
    if (mslSoundIsValid(g_slaughterhouse_pdata->lower_ambient_sound_2)) {
        snd_stop(g_slaughterhouse_pdata->lower_ambient_sound_2);
        g_slaughterhouse_pdata->lower_ambient_sound_2 = 0;
    }
    if (mslSoundIsValid(g_slaughterhouse_pdata->lower_ambient_sound_1)) {
        snd_stop(g_slaughterhouse_pdata->lower_ambient_sound_1);
        g_slaughterhouse_pdata->lower_ambient_sound_1 = 0;
    }
    if (!mslSoundIsValid(g_slaughterhouse_pdata->upper_ambient_sound)) {
        g_slaughterhouse_pdata->upper_ambient_sound = snd_req(0x13C);
    }
}
void bgnd_start_sh_fx(void) {
    set_background_color(0x78, 0x62, 0x56, 0xFF);
    fog_color_real[0] = 0.33f;
    fog_color_real[1] = 0.26f;
    fog_color_real[2] = 0.19f;
    fog_color_real[3] = 1.0f;
    fog_density = 1.0f;
    fog_distance = -10.0f;
    fog_type = 1;
    turn_fog_on();
    hide_sobj(obj_find_sobj_by_id(g_game_info.bgnd_obj, 9));
}
void bgnd_clean_slaughterhouse(void) {}
static void destroy_slaughterhouse_pdata(SlaughterhouseData* pdata);
void vdestroy_slaughterhouse_pdata(SlaughterhouseData* pdata) {
    destroy_slaughterhouse_pdata(pdata);
}
static inline void sh_destroy_latched_object(MkHdrLatch* latch) {
    MkHdr* object;

    if (MK_LIVE(latch->hdr, latch->instance) != 0) {
        object = latch->hdr;
        if (object->instance != 0) {
            object->typed_vtbl->destroy(object);
        }
        latch->hdr = 0;
        latch->instance = 0;
    }
}

/* TODO: [near miss] 90.77%; retail re-reads latch->hdr after validation (ours reuses the validated pointer); registers follow. */
static void destroy_slaughterhouse_pdata(SlaughterhouseData* pdata) {
    sh_destroy_latched_object(&pdata->blood_fall_pebbles[0]);
    sh_destroy_latched_object(&pdata->blood_fall_pebbles[1]);
    sh_destroy_latched_object(&pdata->blood_fall_pebbles[2]);
    sh_destroy_latched_object(&pdata->lower_level_pebbles[0]);
    sh_destroy_latched_object(&pdata->lower_level_pebbles[1]);
    sh_destroy_latched_object(&pdata->lower_level_pebbles[2]);
    sh_destroy_latched_object(&pdata->lower_level_pebbles[3]);
    sh_destroy_latched_object(&pdata->lower_level_pebbles[4]);
    pdata->hdr.instance = 0;
    mkhdr_memfree(&pdata->hdr);
    g_slaughterhouse_pdata = 0;
}
void start_bl_beetles_live_top_floor(void) {
    BlBeetlePdata* data;
    MkSobj* object;
    MkProc* process;

    data = 0;
    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0xF0);
    object->z_offset = 50.0f;
    object->flags09_bits.bit4 = 1;
    object->flags09_bits.bit3 = 1;
    g_bl_beetles = create_pebble_userdata(object, 0x1F, 0x78);
    if (g_bl_beetles != 0) {
        process = _create_mkproc_generic_tinystack(
            0xA020, 0x2E, p_bl_beetle_brains,
            sizeof(BlBeetlePdata), (MkHdr**)&data);
        if (process != 0) {
            data->pebble_data = g_bl_beetles;
            g_bl_beetles_pdata = data;
            bl_init_beetle_pebbles_first_floor(data);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}
static float bl_bounce_fast_amount = 0.008f;
static int bl_bounce_fast_ticks = 3;
static float bl_bounce_calm_amount = 0.005f;
static int bl_bounce_calm_ticks = 3;
static unsigned int next_beetle_exec_tick_counter;

extern void spawn_bld_splat(const char* name, int owner, Vec* position);

/* TODO: [near miss] 99.97%; only the Vec initializer .rodata offsets (+0x204 retail vs +0x6c) remain: TU data layout. */
static float p_bl_beetle_brains(void) {
    BlBeetlePdata* pdata;
    BlBeetleControl* beetle;
    PebbleData* pebble_data;
    BlBeetleControl* beetles;
    PlyrInfo* squashing_player;
    Vec x_axis;
    Vec z_axis;
    Vec y_axis;
    int index;

    pdata = (BlBeetlePdata*)pdata_of_proc(aproc);
    pebble_data = pdata->pebble_data;
    beetles = pebble_data->user_data;
    x_axis = (Vec){1.0f, 0.0f, 0.0f};
    z_axis = (Vec){0.0f, 0.0f, 1.0f};
    y_axis = (Vec){0.0f, 1.0f, 0.0f};
    if (g_bl_beetles == 0) {
        return -1.0f;
    }
    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return 1.0f;
    }

    for (index = 0; index < pdata->pebble_data->count; index++) {
        beetle = &beetles[index];

        switch (beetle->personality) {
        case 0:
            bl_process_beetle_runaway_personality(beetle);
            break;
        case 1:
            bl_process_beetle_under_glass_traveller_personality(beetle);
            break;
        case 3:
            bl_process_beetle_follow_plyr_personality(beetle);
            break;
        case 4:
            bl_process_beetle_under_glass_personality(beetle);
            break;
        case 5:
            bl_process_beetle_transition_personality(beetle);
            break;
        case 6:
            MKMatrixTranslate(
                &pebble_data->pebbles[index].matrix, &beetle->position, 2);
            continue;
        }

        if (beetle_squashed(beetle, &squashing_player) == 1) {
            beetle->position.y = 0.005f + g_game_info.field_34;
            spawn_bld_splat("beetlesplat", 0, &beetle->position);
            if (exec_tick_ctr >= next_beetle_exec_tick_counter) {
                snd_req(0x94);
                next_beetle_exec_tick_counter = exec_tick_ctr + 10;
            }
            beetle->personality = 6;
            beetle->position.y = 1000.0f;
            MKMatrixTranslate(
                &pebble_data->pebbles[index].matrix, &beetle->position, 2);
            continue;
        }

        switch (beetle->movement_state) {
        case 0:
            bl_process_beetle_chilling(beetle);
            break;
        case 1: {
            float movement_scale = -0.05f * beetle->speed_scale;
            bl_process_general_movement(
                beetle, &g_game_info.misc->beetle_target, 20,
                100.0f, 0.0f, 20.0f,
                movement_scale, movement_scale, beetle->surface);
            break;
        }
        case 2:
        case 3:
            bl_process_beetle_track_plyr(beetle);
            break;
        case 4:
            bl_process_beetle_climb_a_wall(beetle);
            break;
        case 5:
            beetle->fast_motion = 0;
            bl_process_general_movement(
                beetle, &beetle->movement_target, 25, beetle->distance_limit_sq, -1.5707964f, 25.0f,
                -0.02f, 0.02f, beetle->surface);
            break;
        case 6: {
            float movement_scale;
            if (beetle->speed_scale > 0.7f) {
                beetle->fast_motion = 1;
            } else {
                beetle->fast_motion = 0;
            }
            movement_scale = -0.05f * beetle->speed_scale;
            bl_process_general_movement(
                beetle, &beetle->movement_target, 20, beetle->distance_limit_sq, 0.0f, 20.0f,
                movement_scale, movement_scale, beetle->surface);
            break;
        }
        case 7:
            beetle->fast_motion = 0;
            if (beetle->surface == 3) {
                bl_process_general_movement(
                    beetle, &beetle->movement_target, 25, beetle->distance_limit_sq, 1.5707964f, 25.0f,
                    0.02f, -0.02f, beetle->surface);
            } else {
                bl_process_general_movement(
                    beetle, &beetle->movement_target, 25, beetle->distance_limit_sq, 0.0f, 25.0f,
                    0.02f, -0.02f, beetle->surface);
            }
            break;
        }

        switch (beetle->surface) {
        case 1:
            MKMatrixRotate(&pebble_data->pebbles[index].matrix, &y_axis,
                beetle->heading_degrees, 0);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &x_axis, 90.0f, 2);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &z_axis, 180.0f, 1);
            break;
        case 3:
            MKMatrixRotate(&pebble_data->pebbles[index].matrix, &x_axis,
                beetle->heading_degrees, 0);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &z_axis, 90.0f, 1);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &y_axis, 0.0f, 2);
            break;
        case 4:
            MKMatrixRotate(&pebble_data->pebbles[index].matrix, &x_axis,
                beetle->heading_degrees, 0);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &z_axis, 90.0f, 1);
            MKMatrixRotate(
                &pebble_data->pebbles[index].matrix, &y_axis, 180.0f, 2);
            break;
        default:
            MKMatrixRotate(&pebble_data->pebbles[index].matrix, &y_axis,
                beetle->heading_degrees, 0);
            break;
        }

        if (beetle->movement_state != 0 && beetle->surface == 0) {
            if (--beetle->bounce_ticks < 0) {
                float bounce_amount;
                int bounce_ticks;
                if (beetle->fast_motion == 1) {
                    bounce_amount = bl_bounce_fast_amount;
                    bounce_ticks = bl_bounce_fast_ticks;
                } else {
                    bounce_amount = bl_bounce_calm_amount;
                    bounce_ticks = bl_bounce_calm_ticks;
                }
                if (beetle->vertical_velocity < 0.0f) {
                    beetle->vertical_velocity = bounce_amount;
                } else {
                    beetle->vertical_velocity = -bounce_amount;
                }
                beetle->bounce_ticks = bounce_ticks;
            }
            beetle->position.y += beetle->vertical_velocity;
            if (beetle->position.y < -0.01f + g_game_info.field_34 ||
                beetle->position.y > 0.03f) {
                beetle->position.y = g_game_info.field_34;
                beetle->vertical_velocity = -1.0f;
                beetle->bounce_ticks = 0;
            }
        }
        MKMatrixScale(
            &pebble_data->pebbles[index].matrix, &beetle->scale, 2);
        MKMatrixTranslate(
            &pebble_data->pebbles[index].matrix, &beetle->position, 2);
    }
    return 1.0f;
}
extern int is_load_meter_active(void);
extern void get_bone_world_pos(MkObj* object, int bone, Vec* position);

static inline int bl_beetle_near_bone(
    BlBeetleControl* beetle, MkObj* fighter, int bone, float limit_sq) {
    Vec bone_position;
    Vec offset = {0.0f, 0.0f, 0.0f};

    get_bone_world_pos(fighter, bone, &bone_position);
    offset.x = beetle->position.x - bone_position.x;
    offset.y = beetle->position.y - bone_position.y;
    offset.z = beetle->position.z - bone_position.z;
    return offset.x * offset.x + offset.y * offset.y + offset.z * offset.z <
        limit_sq;
}

static int beetle_squashed(
    BlBeetleControl* beetle, PlyrInfo** squashing_player) {
    unsigned int player_index;

    if (is_load_meter_active() != 0) {
        return 0;
    }

    for (player_index = 0; player_index < 2; player_index++) {
        if (player_index == 0) {
            *squashing_player = &g_game_info.plyr0;
        } else {
            *squashing_player = &g_game_info.plyr1;
        }

        if ((*squashing_player)->slot.fighter->active != 0) {
            Vec offset_0 = {0.0f, 0.0f, 0.0f};
            Vec bone_position_0;

            get_bone_world_pos(
                (*squashing_player)->slot.mirror_a, 0, &bone_position_0);
            offset_0.x = beetle->position.x - bone_position_0.x;
            offset_0.z = beetle->position.z - bone_position_0.z;
            if (offset_0.x * offset_0.x + offset_0.z * offset_0.z < 1.0f) {
                if (bl_beetle_near_bone(beetle,
                        (*squashing_player)->slot.mirror_a, 11, 0.08f) ||
                    bl_beetle_near_bone(beetle,
                        (*squashing_player)->slot.mirror_a, 10, 0.08f) ||
                    bl_beetle_near_bone(beetle,
                        (*squashing_player)->slot.mirror_a, 3, 0.3f) ||
                    bl_beetle_near_bone(beetle,
                        (*squashing_player)->slot.mirror_a, 9, 0.3f)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}
static void bl_process_beetle_follow_plyr_personality(
    BlBeetleControl* beetle) {
    unsigned int personality_roll;

    if (beetle->movement_state == 0) {
        if (--beetle->personality_ticks == 0) {
            if ((unsigned short)randu0(100) < 50) {
                beetle->movement_state = 2;
            } else {
                beetle->movement_state = 3;
            }
            beetle->personality_ticks =
                (unsigned short)randu0(300) + 180;
            beetle->heading_ticks = 0;
            beetle->speed_scale = 0.95f + frand(0.3f);
        }
    } else {
        if (--beetle->personality_ticks == 0) {
            personality_roll = (unsigned short)randu0(100);
            beetle->heading_ticks = 0;
            if (personality_roll < 3) {
                beetle->personality = 0;
                beetle->personality_ticks =
                    (unsigned short)randu0(300) + 180;
            } else if (personality_roll < 8) {
                beetle->personality = 2;
                beetle->personality_ticks =
                    (unsigned short)randu0(300) + 180;
                beetle->speed_scale = 0.6f;
            } else {
                beetle->movement_state = 0;
                beetle->personality_ticks = 120;
            }
            beetle->movement_target.x = beetle->position.x;
            beetle->movement_target.y = beetle->position.y;
            beetle->movement_target.z = beetle->position.z;
            beetle->distance_limit_sq = 1.0f + frand(3.0f);
            beetle->distance_limit_sq *= 2.0f;
        } else if (beetle->personality_ticks < 50) {
            beetle->speed_scale *= 0.97f;
        } else if (beetle->personality_ticks < 120) {
            beetle->speed_scale = 0.85f;
        }
    }
}
static void bl_process_beetle_under_glass_personality(
    BlBeetleControl* beetle) {
    unsigned int personality;

    if (beetle->movement_state == 0) {
        if (g_game_info.floor_flags.field_74_bit6 == 1) {
            beetle->movement_state = 1;
            beetle->personality_ticks = 300;
            beetle->heading_ticks = 0;
            beetle->speed_scale = 1.5f;
        }
    } else if (beetle->movement_state == 1) {
        beetle->fast_motion = 1;
        if (beetle->personality_ticks > 0) {
            if (--beetle->personality_ticks == 0) {
                personality = (unsigned short)randu0(100);
                if (personality < 30) {
                    beetle->personality = 0;
                } else if (personality < 65) {
                    beetle->personality = 3;
                } else {
                    beetle->personality = 2;
                }
                beetle->movement_state = 0;
                beetle->heading_ticks = 0;
                beetle->personality_ticks =
                    (unsigned short)randu0(300) + 180;
                beetle->movement_target.x = beetle->position.x;
                beetle->movement_target.y = beetle->position.y;
                beetle->movement_target.z = beetle->position.z;
                beetle->distance_limit_sq = 1.0f + frand(3.0f);
                beetle->distance_limit_sq *= 2.0f;
            } else if (beetle->personality_ticks < 50) {
                beetle->speed_scale *= 0.97f;
                beetle->fast_motion = 0;
            } else if (beetle->personality_ticks < 120) {
                beetle->speed_scale = 1.0f;
            }
        }
    }
}
static void bl_process_beetle_under_glass_traveller_personality(
    BlBeetleControl* beetle) {
    unsigned int personality;

    if (beetle->movement_state == 0) {
        if (g_game_info.floor_flags.field_74_bit6 == 1) {
            beetle->movement_state = 1;
            beetle->personality_ticks = 300;
            beetle->heading_ticks = 0;
            beetle->speed_scale = 1.5f;
        } else if ((unsigned short)randu0(600) < 1) {
            beetle->movement_state = 4;
            beetle->heading_ticks = 0;
            beetle->wall_state = 0;
            beetle->wall_target.x = sfrand(4.0f);
            beetle->wall_target.y = 0.0f;
            beetle->wall_target.z = 15.17f;
        }
    } else if (beetle->movement_state == 1) {
        beetle->fast_motion = 1;
        if (beetle->personality_ticks > 0) {
            if (--beetle->personality_ticks == 0) {
                personality = (unsigned short)randu0(100);
                if (personality < 40) {
                    beetle->personality = 0;
                } else if (personality < 75) {
                    beetle->personality = 3;
                } else {
                    beetle->personality = 2;
                }
                beetle->movement_state = 0;
                beetle->heading_ticks = 0;
                beetle->personality_ticks =
                    (unsigned short)randu0(300) + 180;
                beetle->movement_target.x = beetle->position.x;
                beetle->movement_target.y = beetle->position.y;
                beetle->movement_target.z = beetle->position.z;
                beetle->distance_limit_sq = 1.0f + frand(3.0f);
                beetle->distance_limit_sq *= 2.0f;
            } else if (beetle->personality_ticks < 50) {
                beetle->speed_scale *= 0.97f;
                beetle->fast_motion = 0;
            } else if (beetle->personality_ticks < 120) {
                beetle->speed_scale = 1.0f;
            }
        }
    }
}
/* TODO: [near miss] 97.72358%; coordinate FPR coloring and shared 4.0f load scheduling remain. */
static void bl_process_beetle_runaway_personality(
    BlBeetleControl* beetle) {
    float x;
    float y;
    float z;
    float distance_sq;

    y = g_game_info.plyr0.slot.mirror_a->pos.value.y - beetle->position.y;
    x = g_game_info.plyr0.slot.mirror_a->pos.value.x - beetle->position.x;
    z = g_game_info.plyr0.slot.mirror_a->pos.value.z - beetle->position.z;
    distance_sq = z * z + (x * x + y * y);
    if (distance_sq < 4.0f) {
        beetle->movement_state = 1;
        beetle->personality_ticks = (unsigned short)randu0(30) + 180;
        beetle->heading_ticks = 0;
        beetle->speed_scale = 1.35f + frand(0.2f);
        beetle->fast_motion = 1;
    } else {
        y = g_game_info.plyr1.slot.mirror_a->pos.value.y - beetle->position.y;
        x = g_game_info.plyr1.slot.mirror_a->pos.value.x - beetle->position.x;
        z = g_game_info.plyr1.slot.mirror_a->pos.value.z - beetle->position.z;
        distance_sq = z * z + (x * x + y * y);
        if (distance_sq < 4.0f) {
            beetle->movement_state = 1;
            beetle->personality_ticks =
                (unsigned short)randu0(30) + 180;
            beetle->heading_ticks = 0;
            beetle->speed_scale = 1.35f + frand(0.2f);
            beetle->fast_motion = 1;
        } else if (beetle->personality_ticks > 0) {
            if (--beetle->personality_ticks == 0) {
                if ((unsigned short)randu0(100) < 3) {
                    beetle->personality = 3;
                    beetle->movement_state = 0;
                    beetle->personality_ticks = 30;
                } else {
                    beetle->movement_state = 0;
                    beetle->heading_ticks = 0;
                    beetle->movement_target.x = beetle->position.x;
                    beetle->movement_target.y = beetle->position.y;
                    beetle->movement_target.z = beetle->position.z;
                    beetle->distance_limit_sq = 1.0f + frand(3.0f);
                    beetle->distance_limit_sq *= 2.0f;
                }
            } else if (beetle->personality_ticks < 50) {
                beetle->speed_scale *= 0.96f;
                beetle->fast_motion = 0;
            } else {
                beetle->speed_scale = 1.0f;
            }
        }
    }
}
static void bl_process_beetle_transition_personality(
    BlBeetleControl* beetle) {
    Vec delta;

    if (--beetle->transition_ticks == 0) {
        float scale = 1.1f + frand(0.45f);
        unsigned int personality = (unsigned short)randu0(100);

        beetle->scale.x = scale;
        beetle->scale.y = scale;
        beetle->scale.z = scale;
        beetle->fast_motion = 0;
        if (personality < 20) {
            beetle->personality = 2;
            beetle->movement_state = 0;
            beetle->movement_target.x = beetle->position.x;
            beetle->movement_target.y = beetle->position.y;
            beetle->movement_target.z = beetle->position.z;
            beetle->distance_limit_sq = 3.0f;
        } else if (personality < 30) {
            beetle->personality = 3;
            beetle->movement_state = 0;
            beetle->personality_ticks = (unsigned short)randu0(60);
        } else if (personality < 40) {
            beetle->personality = 0;
            beetle->movement_state = 0;
            beetle->personality_ticks =
                (unsigned short)randu0(180) + 180;
        } else {
            beetle->movement_target.x = g_game_info.misc->beetle_target.x;
            beetle->movement_target.y = g_game_info.misc->beetle_target.y;
            beetle->movement_target.z = g_game_info.misc->beetle_target.z;
            beetle->distance_limit_sq = 100.0f;
            beetle->personality = 2;
            beetle->movement_state = 6;
            beetle->personality_ticks =
                (unsigned short)randu0(180) + 180;
            beetle->speed_scale = 0.5f + frand(0.25f);
        }
        beetle->heading_ticks = 0;
    } else {
        delta.x = g_game_info.plyr0.slot.mirror_a->pos.value.x -
            beetle->position.x;
        delta.y = 0.0f;
        delta.z = g_game_info.plyr0.slot.mirror_a->pos.value.z -
            beetle->position.z;
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 7.0f) {
            beetle->movement_state = 1;
            beetle->personality_ticks = beetle->transition_ticks + 3;
            beetle->heading_ticks = 0;
            beetle->speed_scale = 1.65f + frand(0.4f);
            beetle->fast_motion = 1;
        } else {
            delta.x = g_game_info.plyr1.slot.mirror_a->pos.value.x -
                beetle->position.x;
            delta.y = g_game_info.plyr1.slot.mirror_a->pos.value.y -
                beetle->position.y;
            delta.z = g_game_info.plyr1.slot.mirror_a->pos.value.z -
                beetle->position.z;
            if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 7.0f) {
                beetle->movement_state = 1;
                beetle->personality_ticks = beetle->transition_ticks + 3;
                beetle->heading_ticks = 0;
                beetle->speed_scale = 1.65f + frand(0.4f);
                beetle->fast_motion = 1;
            } else if (beetle->personality_ticks > 0) {
                int ticks = beetle->personality_ticks - 1;

                beetle->personality_ticks = ticks;
                if (ticks < 30) {
                    beetle->speed_scale *= 0.96f;
                    beetle->fast_motion = 0;
                } else if (beetle->personality_ticks < 100) {
                    beetle->speed_scale = 1.0f;
                }
            }
        }
    }
}
static void bl_process_beetle_chilling(BlBeetleControl* beetle) {
    int changed;

    changed = 0;
    if (--beetle->heading_ticks <= 0) {
        beetle->wall_state = (unsigned short)randu0(3);
        changed = 1;
    }

    switch (beetle->wall_state) {
    case 0:
        if (changed == 1) {
            float x = beetle->position.x - beetle->movement_target.x;
            float y = 0.0f;
            float z = beetle->position.z - beetle->movement_target.z;

            if (x * x + y * y + z * z >
                beetle->distance_limit_sq - 0.2f) {
                float heading_step;

                beetle->heading_step =
                    (180.0f * gxMathArcTanYX(x, z)) / 3.1415927f -
                    beetle->heading_degrees;
                heading_step = beetle->heading_step;
                if (heading_step > 360.0f) {
                    heading_step -= 360.0f;
                } else if (heading_step < -360.0f) {
                    heading_step += 360.0f;
                }
                if (heading_step > 360.0f) {
                    heading_step -= 360.0f;
                } else if (heading_step < -360.0f) {
                    heading_step += 360.0f;
                }
                beetle->heading_step = heading_step;
                beetle->heading_step /= 30.0f;
                beetle->heading_ticks = 30;
            } else {
                beetle->heading_step = sfrand(2.0f);
                beetle->heading_ticks = 15;
            }
        }
        beetle->heading_degrees += beetle->heading_step;
        break;
    case 1:
        if (changed == 1) {
            float heading =
                (3.1415927f * beetle->heading_degrees) / 180.0f;

            beetle->heading_step = -0.004f * gxMathSin(heading);
            beetle->movement_delta_a = -0.004f * gxMathCos(heading);
            beetle->heading_ticks = (unsigned short)randu0(6) + 6;
        }
        beetle->position.x += beetle->heading_step;
        beetle->position.z += beetle->movement_delta_a;
        break;
    case 2:
        if (changed == 1) {
            beetle->heading_ticks = 8;
        }
        break;
    }
}
static void bl_process_beetle_track_plyr(BlBeetleControl* beetle) {
    unsigned int direction_roll;
    float distance_sq;
    float heading;
    float motion_scale;
    float x;
    float y;
    float z;

    direction_roll = (unsigned short)randu0(100);
    if (--beetle->heading_ticks <= 0) {
        if (beetle->movement_state == 2) {
            x = beetle->position.x -
                g_game_info.plyr0.slot.mirror_a->pos.value.x;
            z = beetle->position.z -
                g_game_info.plyr0.slot.mirror_a->pos.value.z;
        } else {
            x = beetle->position.x -
                g_game_info.plyr1.slot.mirror_a->pos.value.x;
            z = beetle->position.z -
                g_game_info.plyr1.slot.mirror_a->pos.value.z;
        }
        y = 0.0f;
        distance_sq = x * x + y * y + z * z;
        if (distance_sq > 4.25f) {
            beetle->fast_motion = 1;
        } else {
            beetle->fast_motion = 0;
        }

        if (distance_sq > 3.0f) {
            float heading_step;

            heading = gxMathArcTanYX(x, z);
            beetle->heading_step =
                (180.0f * heading) / 3.1415927f + sfrand(30.0f) -
                beetle->heading_degrees;
            heading_step = beetle->heading_step;
            if (heading_step > 360.0f) {
                heading_step -= 360.0f;
            } else if (heading_step < -360.0f) {
                heading_step += 360.0f;
            }
            if (heading_step > 360.0f) {
                heading_step -= 360.0f;
            } else if (heading_step < -360.0f) {
                heading_step += 360.0f;
            }
            beetle->heading_step = heading_step;
            heading_step = beetle->heading_step;
            if (heading_step > 200.0f) {
                heading_step -= 360.0f;
            } else if (heading_step < -200.0f) {
                heading_step = 360.0f + heading_step;
            }
            beetle->heading_step = heading_step;
            beetle->heading_step /= 25.0f;
            beetle->heading_ticks = 25;
        } else {
            if (direction_roll < 50) {
                beetle->heading_step = 3.0f + frand(2.0f);
                if (direction_roll < 25) {
                    beetle->heading_step *= -1.0f;
                }
            } else {
                beetle->heading_step = frand(2.0f);
                if (direction_roll < 75) {
                    beetle->heading_step *= -1.0f;
                }
            }
            beetle->heading_ticks = (unsigned short)randu0(20) + 15;
        }
    }

    beetle->heading_degrees += beetle->heading_step;
    heading = (3.1415927f * beetle->heading_degrees) / 180.0f;
    if (beetle->fast_motion == 1) {
        motion_scale = 1.0f + frand(0.5f);
    } else {
        motion_scale = 0.55f + frand(0.15f);
    }
    beetle->movement_delta_a =
        beetle->speed_scale * (motion_scale * (-0.045f * gxMathSin(heading)));
    beetle->movement_delta_b =
        beetle->speed_scale * (motion_scale * (-0.045f * gxMathCos(heading)));
    beetle->position.x += beetle->movement_delta_a;
    beetle->position.z += beetle->movement_delta_b;
}
static inline void bl_move_beetle_on_surface(
    BlBeetleControl* beetle, float movement_scale_a,
    float movement_scale_b) {
    int surface;
    float heading;
    float motion_scale;

    surface = beetle->surface;
    beetle->heading_degrees += beetle->heading_step;
    heading = (3.1415927f * beetle->heading_degrees) / 180.0f;
    motion_scale = 1.0f + frand(0.5f);
    beetle->movement_delta_a =
        movement_scale_a * (motion_scale * gxMathSin(heading));
    beetle->movement_delta_b =
        movement_scale_b * (motion_scale * gxMathCos(heading));

    switch (surface) {
    case 4:
        beetle->position.y += beetle->movement_delta_a;
        beetle->position.z -= beetle->movement_delta_b;
        break;
    case 3:
        beetle->position.y += beetle->movement_delta_a;
        beetle->position.z += beetle->movement_delta_b;
        break;
    case 1:
        beetle->position.x += beetle->movement_delta_a;
        beetle->position.y += beetle->movement_delta_b;
        break;
    default:
        beetle->position.x += beetle->movement_delta_a;
        beetle->position.z += beetle->movement_delta_b;
        break;
    }
}

/* TODO: [near miss] 99.84%; float-register coloring in the heading atan/pi expressions remains. */
static void bl_process_beetle_climb_a_wall(BlBeetleControl* beetle) {
    float heading_step;
    float x;
    float y;
    float z;

    beetle->fast_motion = 0;
    switch (beetle->wall_state) {
    case 0:
        beetle->fast_motion = 1;
        x = beetle->position.x - beetle->wall_target.x;
        y = 0.0f;
        z = beetle->position.z - beetle->wall_target.z;
        beetle->heading_step =
            (180.0f * gxMathArcTanYX(x, z)) / 3.1415927f -
            beetle->heading_degrees;
        if (x * x + y * y + z * z < 0.1f ||
            beetle->position.z > beetle->wall_target.z) {
            beetle->position.x = beetle->wall_target.x;
            beetle->position.z = beetle->wall_target.z;
            beetle->wall_state = 1;
            beetle->heading_degrees = 0.0f;
            beetle->position.y = 0.05f;
            beetle->heading_ticks = 15;
            beetle->surface = 1;
            beetle->movement_target.x = beetle->position.x;
            beetle->movement_target.y = beetle->position.y;
            beetle->movement_target.z = beetle->position.z;
            beetle->movement_target.x = 0.0f;
            beetle->movement_target.y = 4.5f;
            beetle->distance_limit_sq = 10.0f;
            beetle->wall_ticks = (unsigned short)randu0(240) + 300;
            return;
        }
        bl_move_beetle_on_surface(beetle, -0.06f, -0.06f);
        break;
    case 3:
        beetle->fast_motion = 1;
        x = beetle->position.x - beetle->wall_target.x;
        y = 0.0f;
        z = beetle->position.z - beetle->wall_target.z;
        beetle->heading_step =
            (180.0f * gxMathArcTanYX(x, z)) / 3.1415927f -
            beetle->heading_degrees;
        if (x * x + y * y + z * z < 0.03f ||
            beetle->position.z < beetle->wall_target.z) {
            beetle->movement_state = 0;
            beetle->heading_ticks = 0;
            return;
        }
        bl_move_beetle_on_surface(beetle, -0.04f, -0.04f);
        break;
    case 2:
        x = beetle->wall_target.x - beetle->position.x;
        y = beetle->wall_target.y - beetle->position.y;
        z = 0.0f;
        beetle->heading_step =
            (180.0f * (gxMathArcTanYX(y, x) - 1.5707964f)) /
                3.1415927f -
            beetle->heading_degrees;
        heading_step = beetle->heading_step;
        if (heading_step > 360.0f) {
            heading_step -= 360.0f;
        } else if (heading_step < -360.0f) {
            heading_step += 360.0f;
        }
        if (heading_step > 360.0f) {
            heading_step -= 360.0f;
        } else if (heading_step < -360.0f) {
            heading_step += 360.0f;
        }
        beetle->heading_step = heading_step;
        beetle->heading_step /= 40.0f;
        if (x * x + y * y + z * z < 0.03f || beetle->position.y < 0.0f) {
            beetle->position.y = 0.0f;
            beetle->surface = 0;
            beetle->wall_state = 3;
            beetle->movement_target.x = -1.0f + sfrand(0.8f);
            beetle->movement_target.z = 1.99f + sfrand(0.8f);
            beetle->wall_target.x = beetle->movement_target.x;
            beetle->wall_target.y = beetle->movement_target.y;
            beetle->wall_target.z = beetle->movement_target.z;
            beetle->distance_limit_sq = 1.0f;
            return;
        }
        bl_move_beetle_on_surface(beetle, -0.03f, 0.03f);
        break;
    case 1:
        if (--beetle->wall_ticks <= 0 &&
            !g_game_info.floor_flags.field_74_bit6) {
            beetle->wall_state = 2;
            return;
        }
        bl_process_general_movement(
            beetle, &beetle->movement_target, 25, beetle->distance_limit_sq, -1.5707964f, 25.0f,
            -0.02f, 0.02f, beetle->surface);
        break;
    }
}
static void bl_process_general_movement(
    BlBeetleControl* beetle, const Vec* target, int heading_ticks,
    float distance_limit_sq, float heading_offset, float heading_divisor,
    float movement_scale_a, float movement_scale_b, int surface) {
    unsigned int direction_roll;
    float heading;
    float heading_step;
    float motion_scale;
    float angle;
    float x;
    float y;
    float z;

    direction_roll = (unsigned short)randu0(100);
    if (--beetle->heading_ticks <= 0) {
        if (surface != 0u) {
            x = target->x - beetle->position.x;
            y = target->y - beetle->position.y;
            z = target->z - beetle->position.z;
        } else {
            x = beetle->position.x - target->x;
            y = beetle->position.y - target->y;
            z = beetle->position.z - target->z;
        }

        switch (surface) {
        case 3:
        case 4:
            x = 0.0f;
            break;
        case 1:
            z = 0.0f;
            break;
        default:
            y = 0.0f;
            break;
        }

        if (x * x + y * y + z * z > distance_limit_sq) {
            switch (surface) {
            case 4:
                heading = heading_offset + gxMathArcTanYX(y, z);
                break;
            case 3:
                heading = heading_offset + gxMathArcTanYX(z, y);
                break;
            case 1:
                heading = heading_offset + gxMathArcTanYX(y, x);
                break;
            default:
                heading = heading_offset + gxMathArcTanYX(x, z);
                break;
            }
            beetle->heading_step =
                (180.0f * heading) / 3.1415927f - beetle->heading_degrees;
            heading_step = beetle->heading_step;
            if (heading_step > 360.0f) {
                heading_step -= 360.0f;
            } else if (heading_step < -360.0f) {
                heading_step += 360.0f;
            }
            if (heading_step > 360.0f) {
                heading_step -= 360.0f;
            } else if (heading_step < -360.0f) {
                heading_step += 360.0f;
            }
            beetle->heading_step = heading_step;
            beetle->heading_step /= heading_divisor;
            beetle->heading_ticks = heading_ticks;
        } else {
            if (direction_roll < 50) {
                beetle->heading_step = 3.0f + frand(2.0f);
                if (direction_roll < 25) {
                    beetle->heading_step *= -1.0f;
                }
            } else {
                beetle->heading_step = frand(2.0f);
                if (direction_roll < 75) {
                    beetle->heading_step *= -1.0f;
                }
            }
            beetle->heading_ticks = (unsigned short)randu0(20) + 15;
        }
    }

    beetle->heading_degrees += beetle->heading_step;
    angle = (3.1415927f * beetle->heading_degrees) / 180.0f;
    motion_scale = 1.0f + frand(0.5f);
    beetle->movement_delta_a =
        movement_scale_a * (motion_scale * gxMathSin(angle));
    beetle->movement_delta_b =
        movement_scale_b * (motion_scale * gxMathCos(angle));

    switch (surface) {
    case 4:
        beetle->position.y += beetle->movement_delta_a;
        beetle->position.z -= beetle->movement_delta_b;
        break;
    case 3:
        beetle->position.y += beetle->movement_delta_a;
        beetle->position.z += beetle->movement_delta_b;
        break;
    case 1:
        beetle->position.x += beetle->movement_delta_a;
        beetle->position.y += beetle->movement_delta_b;
        break;
    default:
        beetle->position.x += beetle->movement_delta_a;
        beetle->position.z += beetle->movement_delta_b;
        break;
    }
}
static inline void bl_init_beetle(
    BlBeetleControl* beetle, Pebble* pebble,
    const Vec* position, float local_scale, int personality,
    int movement_state, int personality_ticks, float distance_limit_sq,
    int transition_ticks, int surface) {
    Vec scale;
    Vec y_axis = {0.0f, 1.0f, 0.0f};
    scale.x = scale.y = scale.z = local_scale;
    beetle->position.x = beetle->position.y = beetle->position.z = 0.0f;
    beetle->position.x = position->x;
    beetle->position.y = position->y;
    beetle->position.z = position->z;
    beetle->movement_target.x = beetle->position.x;
    beetle->movement_target.y = beetle->position.y;
    beetle->movement_target.z = beetle->position.z;
    beetle->distance_limit_sq = distance_limit_sq;
    beetle->personality = personality;
    beetle->movement_state = movement_state;
    beetle->personality_ticks = personality_ticks;
    beetle->heading_degrees = (float)(unsigned short)randu0(360);
    beetle->heading_step = 0.0f;
    beetle->wall_ticks = (unsigned short)randu0(60);
    beetle->heading_ticks = 0;
    beetle->bounce_ticks = 0;
    beetle->field_68 = 3;
    beetle->vertical_velocity = -0.003f;
    beetle->fast_motion = 0;
    beetle->transition_ticks = transition_ticks;
    beetle->surface = surface;
    beetle->scale.x = scale.x;
    beetle->scale.y = scale.y;
    beetle->scale.z = scale.z;
    MKMatrixRotateScaleTranslate(&pebble->matrix, &y_axis,
                                 beetle->heading_degrees, &scale,
                                 &beetle->position);
}

/* TODO: [near miss] 99.52%; loops 1-3 and the last loop swap beetle/pebble GPRs; f21/f22 swap in the movement-target loops. */
static void bl_init_beetle_pebbles_second_floor(BlBeetlePdata* data) {
    Pebble* pebble;
    BlBeetleControl* beetles;
    BlBeetleControl* beetle;
    Vec position;
    int random_ticks;
    int index;

    beetles = data->pebble_data->user_data;
    index = 0;
    for (; index < 5; index++) {
        float local_scale = 1.7f + frand(0.25f);
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 37.0f + sfrand(1.4f);
        position.x = 0.2f + sfrand(1.4f);
        position.y = -10.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 5, 0,
                       random_ticks, 1.0f, 0x136, 0);
    }
    for (; index < 7; index++) {
        float local_scale = 1.0f + frand(0.25f);
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 40.0f + sfrand(0.25f);
        position.x = 0.2f + sfrand(0.85f);
        position.y = -9.9f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 0,
                       random_ticks, 0.5f, 0, 0);
    }
    for (; index < 10; index++) {
        float local_scale = 1.35f + frand(0.15f);
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 34.25f + sfrand(0.45f);
        position.x = -0.03f + sfrand(0.05f);
        position.y = -10.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 0,
                       random_ticks, 0.5f, 0, 0);
    }
    for (; index < 12; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.y = -7.0f + sfrand(1.5f);
        position.x = sfrand(2.5f);
        position.z = 47.9f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 5,
                       random_ticks, 2.0f, 0, 1);
        beetle->movement_target.x = sfrand(0.5f);
        beetle->movement_target.y = -7.0f + sfrand(1.0f);
        beetle->movement_target.z = 47.9f;
    }
    for (; index < 14; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.y = -7.0f + sfrand(1.5f);
        position.x = 7.6f + sfrand(2.5f);
        position.z = 47.7f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 5,
                       random_ticks, 4.0f, 0, 1);
        beetle->movement_target.x = 7.6f + sfrand(1.0f);
        beetle->movement_target.y = -7.0f + sfrand(1.0f);
        beetle->movement_target.z = 47.7f;
    }
    for (; index < 16; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.y = -7.0f + sfrand(1.5f);
        position.x = -7.6f + sfrand(2.5f);
        position.z = 47.7f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 5,
                       random_ticks, 4.0f, 0, 1);
        beetle->movement_target.x = -7.6f + sfrand(1.0f);
        beetle->movement_target.y = -7.0f + sfrand(1.0f);
        beetle->movement_target.z = 47.7f;
    }
    for (; index < 19; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 21.5f + sfrand(0.85f);
        position.x = 8.25f + sfrand(0.85f);
        position.y = -10.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 6,
                       random_ticks, 1.5f, 0, 0);
        beetle->speed_scale = 0.5f + frand(0.25f);
    }
    for (; index < 21; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 44.0f + sfrand(1.5f);
        position.y = -7.0f + sfrand(2.5f);
        position.x = 14.3f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 7,
                       random_ticks, 4.0f, 0, 3);
        beetle->movement_target.x = 14.3f;
        beetle->movement_target.y = -7.0f;
        beetle->movement_target.z = 44.0f;
    }
    for (; index < 22; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 44.0f + sfrand(1.5f);
        position.y = -7.0f + sfrand(2.5f);
        position.x = -14.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 7,
                       random_ticks, 4.0f, 0, 4);
        beetle->movement_target.x = -14.0f;
        beetle->movement_target.y = -7.0f;
        beetle->movement_target.z = 44.0f;
    }
    for (; index < 24; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 23.0f + sfrand(1.5f);
        position.y = -7.0f + sfrand(2.5f);
        position.x = 14.3f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 7,
                       random_ticks, 4.0f, 0, 3);
        beetle->movement_target.x = 14.3f;
        beetle->movement_target.y = -7.0f;
        beetle->movement_target.z = 23.0f;
    }
    for (; index < 25; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 23.0f + sfrand(1.5f);
        position.y = -7.0f + sfrand(2.5f);
        position.x = -14.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 7,
                       random_ticks, 4.0f, 0, 4);
        beetle->movement_target.x = -14.0f;
        beetle->movement_target.y = -7.0f;
        beetle->movement_target.z = 23.0f;
    }
    for (; index < 28; index++) {
        float local_scale = 1.0f + frand(0.45f);
        unsigned int random_ticks;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 21.5f + sfrand(0.85f);
        position.x = -8.25f + sfrand(0.85f);
        position.y = -10.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 6,
                       random_ticks, 1.5f, 0, 0);
        beetle->speed_scale = 0.5f + frand(0.25f);
    }
    data->pebble_data->count = 10;
}
/* TODO: [near miss] 98.65%; ranges use bl_init_beetle; data/beetle/pebble GPR coloring remains (data r31, retail r26). */
static void bl_init_beetle_pebbles_first_floor(BlBeetlePdata* data) {
    BlBeetleControl* beetles;
    BlBeetleControl* beetle;
    Vec position;
    int random_ticks;
    int index;

    beetles = data->pebble_data->user_data;
    index = 0;
    for (; index < 6; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 1.99f + sfrand(0.8f);
        position.x = -1.0f + sfrand(0.8f);
        position.y = 0.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 1, 0,
                       random_ticks, 1.0f, 0, 0);
    }
    for (; index < 9; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 5.0f + sfrand(0.15f);
        position.x = -4.0f + sfrand(0.15f);
        position.y = 0.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 0, 0,
                       random_ticks, 1.0f, 0, 0);
    }
    for (; index < 21; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = 1.99f + sfrand(0.25f);
        position.x = -1.0f + sfrand(0.25f);
        position.y = 0.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 4, 0,
                       random_ticks, 0.55f, 0, 0);
    }
    for (; index < 23; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.z = sfrand(7.0f);
        position.x = sfrand(7.0f);
        position.y = 0.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 0,
                       random_ticks, 9.0f, 0, 0);
    }
    for (; index < 27; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(180) + 180;
        position.y = 2.0f + sfrand(1.5f);
        position.x = sfrand(2.5f);
        position.z = 15.17f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 2, 5,
                       random_ticks, 9.0f, 0, 1);
    }
    for (; index < 31; index++) {
        float local_scale = 1.0f + frand(0.45f);
        Pebble* pebble;
        random_ticks = (unsigned short)randu0(60);
        position.z = sfrand(7.0f);
        position.x = sfrand(7.0f);
        position.y = 0.0f;
        pebble = &data->pebble_data->pebbles[index];
        beetle = &beetles[index];
        bl_init_beetle(beetle, pebble, &position, local_scale, 3, 0,
                       random_ticks, 9.0f, 0, 0);
    }
    data->pebble_data->count = 31;
}
void bgnd_clean_beetlelair(void) { g_bl_beetles = 0; }
typedef struct BlDangerObjectRef {
    char pad00[0x5C];
    MkObj* object;
} BlDangerObjectRef;
typedef struct BlDangerSource {
    char pad00[0x18];
    BlDangerObjectRef* object_ref;
} BlDangerSource;
typedef struct BlDangerEvent {
    char pad00[0x10];
    BlDangerSource* source;
    char pad14[4];
    BlDangerObjectRef* target;
} BlDangerEvent;
typedef struct BlColumnBreakData {
    MkHdr hdr;
    PlyrPdata* player;
    int side;
    Vec direction;
    int use_camera;
} BlColumnBreakData;
static int beetle_lair_react_to_wall_danger_zone_cb(BlDangerEvent* event);
static float p_beetle_lair_column_breaking(void);
static int beetle_lair_collision_cb(BgndObstacleEventData* event);
void bgnd_reg_col_cb_for_beetle_lair(void) {
    set_arena_obstacle_callback(beetle_lair_collision_cb);
    set_background_obstacle_repel_flag(0x41, 0);
    set_background_obstacle_repel_flag(0x42, 0);
}
/* TODO: [breakthrough] 88.76%; ordered event-class checks and fighting-light
 * trigger stores match; wall-target and normalization stack layout remains. */
static int beetle_lair_collision_cb(BgndObstacleEventData* event) {
    BlColumnBreakData* column_data;
    BgndScriptProcData* script_data;
    MkProc* process;
    MkObj* player_object;
    MkObj* opponent_object;
    MkPfx* effect;
    Vec direction;
    float inverse_length;
    float length;
    float player_length;
    float separation_x;
    float separation_z;
    float player_x;
    float player_z;
    float reaction_flag;
    unsigned int handle;
    int eligible;
    int aligned;

    eligible = 0;
    reaction_flag = (float)(g_current_reaction_info.flags & 0x100);
    if (g_game_info.plyr0.field_0C == 0.0f ||
        g_game_info.plyr1.field_0C == 0.0f) {
        return 0;
    }
    if (event->player_pdata != 0 &&
        (((reaction_flag > 0.0f) &&
          (event->player_pdata->state & 0x400) != 0) ||
         (event->player_pdata->state & 0x1000) != 0)) {
        eligible = 1;
    }

    if (event->field_04 == 7) {
        if (event->event_id == 0x12C) {
            if (beetle_lair_react_to_wall_danger_zone_cb(
                    (BlDangerEvent*)event)) {
                event->player_pdata->online_sync_index = 0x131;
                if (event->player_pdata->plyr_num == 0) {
                    g_game_info.plyr0.fighting_lights.green_trigger = 1;
                } else {
                    g_game_info.plyr1.fighting_lights.green_trigger = 1;
                }
            }
        } else if (event->event_id == 0x12D && event->player_pdata != 0) {
            Vec wall_target = {-0.9854f, 0.0f, 1.855f};
            union {
                float f;
                unsigned int u;
            } estimate1, input1, estimate2, input2;
            float squared;
            aligned = 0;
            if (event->player_pdata->his_plyr_pdata != 0) {
                player_object =
                    event->player_pdata->his_plyr_pdata->plyr_info->slot.mirror_a;
                opponent_object =
                    event->player_pdata->plyr_info->slot.mirror_a;
                player_z = player_object->pos.value.z - wall_target.z;
                player_x = player_object->pos.value.x - wall_target.x;
                separation_z = player_z -
                    (opponent_object->pos.value.z - wall_target.z);
                separation_x = player_x -
                    (opponent_object->pos.value.x - wall_target.x);
                squared = separation_x * separation_x +
                    separation_z * separation_z;
                input1.f = squared;
                if (squared <= 0.0f) {
                    length = 0.0f;
                } else {
                    estimate1.u = (unsigned int)GXMathSqrtTable[
                        (input1.u >> 11) & 0x1FFF] << 8;
                    estimate1.u |=
                        (((input1.u & 0x7F800000U) + 0x3F800000U) >> 1) &
                        0x7F800000U;
                    length = 0.5f * estimate1.f *
                        (3.0f - (estimate1.f * estimate1.f) / squared);
                }
                squared = player_x * player_x + player_z * player_z;
                input2.f = squared;
                if (squared <= 0.0f) {
                    player_length = 0.0f;
                } else {
                    estimate2.u = (unsigned int)GXMathSqrtTable[
                        (input2.u >> 11) & 0x1FFF] << 8;
                    estimate2.u |=
                        (((input2.u & 0x7F800000U) + 0x3F800000U) >> 1) &
                        0x7F800000U;
                    player_length = 0.5f * estimate2.f *
                        (3.0f - (estimate2.f * estimate2.f) / squared);
                }
                if ((separation_x * player_x + separation_z * player_z) /
                        (length * player_length) >
                    0.9f) {
                    aligned = 1;
                }
            }
            if (aligned) {
                event->player_pdata->online_sync_index = 0xE3;
                if (event->player_pdata->plyr_num == 0) {
                    g_game_info.plyr0.fighting_lights.green_trigger = 1;
                } else {
                    g_game_info.plyr1.fighting_lights.green_trigger = 1;
                }
            }
        }
        return 0;
    }
    if (event->field_04 == 5) {
        if ((event->event_id == 0x97 || event->event_id == 0x40) &&
            eligible != 0 && !g_game_info.floor_flags.field_74_bit6) {
            Vec smoke_position = {0.0f, 0.0f, 0.0f};
            if (event->flag_bits.player_side == 1) {
                if (event->player_pdata->state != 0x609) {
                    return 0;
                }
                event->player_pdata = event->player_pdata->his_plyr_pdata;
                damage_player(event->player_pdata->his_plyr_pdata, 0.04f);
            }

            direction.x = event->impact_vector->x;
            direction.y = event->impact_vector->y;
            direction.z = event->impact_vector->z;
            inverse_length = bgnd_inv_sqrt(direction.x * direction.x +
                direction.y * direction.y + direction.z * direction.z);
            event->impact_vector->x = 0.045f * direction.x * inverse_length;
            event->impact_vector->y = 0.045f * direction.y * inverse_length;
            event->impact_vector->z = 0.045f * direction.z * inverse_length;

            player_object = MK_HDR_LIVE(event->player_pdata->tracked_obj, event->player_pdata->tracked_obj_instance);
            opponent_object = MK_HDR_LIVE(event->player_pdata->his_plyr_pdata->tracked_obj, event->player_pdata->his_plyr_pdata->tracked_obj_instance);
            if (player_object != 0 && opponent_object != 0) {
                g_game_info.collision_player_info =
                    event->player_pdata->plyr_info;
                g_game_info.active_player =
                    event->player_pdata->his_plyr_pdata->plyr_info;
                g_game_info.impact_vector.x = event->impact_vector->x;
                g_game_info.impact_vector.y = event->impact_vector->y;
                g_game_info.impact_vector.z = event->impact_vector->z;
                g_game_info.player_objects[0] = opponent_object;
                g_game_info.player_objects[1] = player_object;
                g_game_info.collision_player_pdata = event->player_pdata;
                g_game_info.collision_player_side =
                    event->flag_bits.player_side;
                g_game_info.collision_event_id = event->event_id;
            }

            script_data = 0;
            process = _create_mkproc_generic_tinystack(
                0xC012, 0x1F, p_bgnd_script_in_proc,
                sizeof(BgndScriptProcData), (MkHdr**)&script_data);
            if (process != 0 && script_data != 0) {
                script_data->script_index = 7;
                set_process_as_scriptable(process);
            }

            smoke_position.x = 0.35f * event->impact_vector->x;
            smoke_position.y = 0.0f;
            smoke_position.z = 0.35f * event->impact_vector->z;
            handle = fx_by_owner("glass_base_explosion_fx", 4);
            if (handle != 0) {
                fx_reset(handle);
                effect = pfx_from_handle(handle);
                if (effect != 0) {
                    g_latest_obj_pfx =
                        (MkObj*)pfx_get_emitter_obj(effect, 0);
                    if (g_latest_obj_pfx == 0) {
                        g_latest_obj_pfx =
                            pfx_bind_to_new_obj(effect, 0x8227);
                    }
                    if (g_latest_obj_pfx != 0) {
                        g_latest_obj_pfx->flags_08_bits.airborne = 1;
                        g_latest_obj_pfx->pos.value.x = -1.0f;
                        g_latest_obj_pfx->pos.value.y = 0.2f;
                        g_latest_obj_pfx->pos.value.z = 1.9f;
                        update_mkobj(g_latest_obj_pfx);
                        resume_effect("glass_base_explosion_fx");
                    }
                }
            }
            handle = fx_by_owner("glass_base_explosion_fx", 4);
            fx_set_param_v3(
                handle, 0x201, smoke_position.x,
                smoke_position.y, smoke_position.z);
            g_game_info.floor_flags.field_74_bit6 = 1;
            if (bgnd_danger_zones[1].obstacle != 0) {
                delete_obstacle_from_background_by_id(
                    bgnd_danger_zones[1].obstacle_id);
                bgnd_danger_zones[1].obstacle = 0;
            }
            return 1;
        }

        if (event->event_id == 0x3E && eligible != 0) {
            if (event->flag_bits.player_side == 1) {
                if ((event->player_pdata->state & 0x1000) != 0) {
                    event->flag_bits.player_side = 0;
                } else {
                    event->player_pdata =
                        event->player_pdata->his_plyr_pdata;
                    damage_player(event->player_pdata->his_plyr_pdata, 0.04f);
                }
            }
            column_data = 0;
            process = _create_mkproc_generic_bigstack(
                0xC01A, 0x1F, p_beetle_lair_column_breaking,
                sizeof(BlColumnBreakData), (MkHdr**)&column_data);
            if (process != 0 && column_data != 0) {
                column_data->player = event->player_pdata;
                if (event->impact_vector != 0) {
                    column_data->direction.x = event->impact_vector->x;
                    column_data->direction.y = event->impact_vector->y;
                    column_data->direction.z = event->impact_vector->z;
                } else {
                    column_data->direction.x = 0.0f;
                    column_data->direction.y = 0.0f;
                    column_data->direction.z = 0.075f;
                }
                column_data->direction.x *= 1.5f;
                column_data->direction.y *= 1.5f;
                column_data->direction.z *= 1.5f;
                column_data->side = 0;
                column_data->use_camera = event->flag_bits.player_side;
                if (g_game_info.bgnd_obj != 0) {
                    mk_insert(&process->hdr,
                              &g_game_info.bgnd_obj->child_list);
                }
            }
            toggle_danger_zone(1);
            return 1;
        }
        if (event->event_id == 0x3F && eligible != 0) {
            if (event->flag_bits.player_side == 1) {
                if ((event->player_pdata->state & 0x1000) != 0) {
                    event->flag_bits.player_side = 0;
                } else {
                    event->player_pdata =
                        event->player_pdata->his_plyr_pdata;
                    damage_player(event->player_pdata->his_plyr_pdata, 0.04f);
                }
            }
            column_data = 0;
            process = _create_mkproc_generic_bigstack(
                0xC01A, 0x1F, p_beetle_lair_column_breaking,
                sizeof(BlColumnBreakData), (MkHdr**)&column_data);
            if (process != 0 && column_data != 0) {
                column_data->player = event->player_pdata;
                if (event->impact_vector != 0) {
                    column_data->direction.x = event->impact_vector->x;
                    column_data->direction.y = event->impact_vector->y;
                    column_data->direction.z = event->impact_vector->z;
                } else {
                    column_data->direction.x = 0.0f;
                    column_data->direction.y = 0.0f;
                    column_data->direction.z = 0.075f;
                }
                column_data->direction.x *= 1.5f;
                column_data->direction.y *= 1.5f;
                column_data->direction.z *= 1.5f;
                column_data->side = 1;
                column_data->use_camera = event->flag_bits.player_side;
                if (g_game_info.bgnd_obj != 0) {
                    mk_insert(&process->hdr,
                              &g_game_info.bgnd_obj->child_list);
                }
            }
            toggle_danger_zone(0);
            return 1;
        }
        if ((event->event_id == 0x40 || event->event_id == 0x97) &&
            g_game_info.floor_flags.field_74_bit6) {
            return 1;
        }
        return 0;
    }
    return 0;
}
typedef struct BlWallBreakCameraData {
    MkHdr hdr;
    PlyrPdata* player;
} BlWallBreakCameraData;

typedef struct BlWallBreakControllerData {
    MkHdr hdr;
    PlyrPdata* player;
    unsigned int initial_delay;
    int remaining_scene_ticks;
} BlWallBreakControllerData;

static float p_beetle_lair_front_wall_breaking(void);
static float p_beetle_lair_wall_breaking_controller(void);
extern void tightrope_restrictions_off(void);

/* TODO: [near miss] 99.97%; only stack slots swap: inlined bgnd_inv_sqrt input (retail 0x14) vs controller output (retail 0xc). */
float r_beetle_lair_transition(void) {
    BlWallBreakControllerData* controller_data;
    BlWallBreakControllerData* front_wall_data;
    MkProc* process;
    PlyrPdata* player;
    Vec velocity;
    float inverse_length;
    float z;
    float x;
    int launched;

    launched = 0;
    drone_ai_dont_think();
    turn_controllers_off();
    g_game_info.flag_bits.level_transition_active = 1;
    g_game_info.flag_bits.level_fatality_active = 0;
    plyr_weapon_trail_hide(g_game_info.plyr0.slot.pdata->mirror_slots);
    plyr_weapon_trail_hide(g_game_info.plyr1.slot.pdata->mirror_slots);
    g_game_info.plyr0.slot.pdata->collision_disabled = 1;
    g_game_info.plyr1.slot.pdata->collision_disabled = 1;
    high_flash_check();
    face_opponent_now();
    danger_zone_eligible_on();
    bgnd_process_collision_info(
        9, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    bgnd_process_collision_info(
        7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    velocity.x = 0.0f;
    velocity.y = 0.0f;
    velocity.z = 0.0f;
    obj_set_pos_vel(g_game_info.player_objects[1], &velocity);
    stop_me();
    init_air_move_no_aniproc();
    random_hit(1);
    random_hit(4);
    random_voice(0x13);

    x = g_game_info.player_objects[0]->pos.value.x -
        g_game_info.player_objects[1]->pos.value.x;
    z = g_game_info.player_objects[0]->pos.value.z -
        g_game_info.player_objects[1]->pos.value.z;
    inverse_length = bgnd_inv_sqrt(x * x + z * z);
    x *= inverse_length;
    velocity.z = 0.23f;
    velocity.x = 0.23f * x;
    if (plyr_obj->pos.value.x > 2.5f ||
        plyr_obj->pos.value.x < -2.5f) {
        velocity.x *= 0.5f;
        velocity.z = 0.23f;
    }
    obj_set_pos_vel(g_game_info.player_objects[0], &velocity);
    tightrope_restrictions_off();
    plyr_obj->flags_0B_bits.bit6 = 1;
    special_move_cam_setup(0.35f, 4.0f, 2.0f, -2.67f, 0.2f, 1, 0xC8, 0);
    launch_me_up(0.025f, 0.0f);
    blend_to_ani(*(AniData**)&shared_ani[0x194], 0, 0.1f);
    set_ani_speed(1.0f);
    g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 1;
    g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 1;

    while (launched == 0) {
        ani_loop_more_frames(1.0f);
        if (plyr_obj->pos.value.z > 14.9f) {
            launched = 1;
        }
    }
    damage_me(0.05f);
    ck_rumble_controller(get_my_plyr_num(), 8, 0x19);

    front_wall_data = 0;
    player = plyr_pdata;
    process = _create_mkproc_generic_tinystack(
        0xC01A, 0x1F, p_beetle_lair_front_wall_breaking,
        sizeof(BlWallBreakControllerData), (MkHdr**)&front_wall_data);
    if (process != 0 && front_wall_data != 0) {
        front_wall_data->player = player;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    delete_obstacle_from_background_by_id(0x42);
    player->online_sync_index = -1;
    if (player->plyr_num == 0) {
        g_game_info.plyr0.fighting_lights.green_trigger = 0;
    } else {
        g_game_info.plyr1.fighting_lights.green_trigger = 0;
    }

    controller_data = 0;
    player = plyr_pdata;
    process = _create_mkproc_generic_bigstack(
        0xC01A, 0x1F, p_beetle_lair_wall_breaking_controller,
        sizeof(BlWallBreakControllerData), (MkHdr**)&controller_data);
    if (process != 0 && controller_data != 0) {
        controller_data->player = player;
        controller_data->initial_delay = 0x28;
        controller_data->remaining_scene_ticks = 0x348;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    ani_loop_more_frames(1000.0f);
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}
static int beetle_lair_react_to_wall_danger_zone_cb(BlDangerEvent* event) {
    MkObj* source;
    MkObj* target;
    Vec wall_normal = {0.0f, 0.0f, 1.0f};
    float dot;
    float inverse_length;
    float x;
    float y;
    float z;
    float target_z;

    target = event->target->object;
    source = event->source->object_ref->object;
    y = target->pos.value.y - source->pos.value.y;
    x = target->pos.value.x - source->pos.value.x;
    target_z = target->pos.value.z;
    z = target_z - source->pos.value.z;
    inverse_length = bgnd_inv_sqrt(z * z + (x * x + y * y));
    x *= inverse_length;
    y *= inverse_length;
    z *= inverse_length;
    dot = wall_normal.x * x + wall_normal.y * y + wall_normal.z * z;
    if (target_z < 10.0f) {
        if (dot > 0.7f) {
            return 1;
        }
    } else if (dot > 0.3f) {
        return 1;
    }
    return 0;
}
static float p_beetle_lair_watch_remaining_fall_scene(void);
static float winner_watching_him_fall(void);
static float victim_fall_down_a_level(void);
static int g_go_back_to_fight_position;

static float p_beetle_lair_downstairs_wall_break_cam_control(void);
static float plyr_is_prone(void);
void bgnd_swap_level(int level);
float bgnd_launch_chunk(
    MkSobj* object, const Vec* position, const Vec* velocity,
    const Vec* angular_velocity, const Vec* angles, unsigned int end_mode,
    const Vec* scale, int field_0C, float vertical_accel, int field_10);

extern unsigned int fx(const char* name);
extern void fx_set_param_v3(
    unsigned int effect, int parameter, float x, float y, float z);
extern void shake_camera_y(int count, float strength);

static inline void bl_front_wall_effect_at(
    const char* name, float x, float y, float z) {
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
                    pfx_bind_to_new_obj(effect, 0x8227);
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

/* TODO: [breakthrough needed] 89.94%; missing earlier Vec pool template shifts addends; launch FPR/copy scheduling remains. */
static float p_beetle_lair_wall_breaking_controller(void) {
    Vec camera_start = {2.166f, -9.5f, 42.734f};
    Vec camera_end = {0.4512f, -0.5f, 18.8713f};
    Vec velocity;
    Vec angular_velocity;
    Vec scale = {1.0f, 1.0f, 1.0f};
    Vec position0;
    Vec angles0;
    Vec position1;
    Vec angles1;
    Vec position2;
    Vec angles2;
    Vec position3;
    Vec angles3;
    Vec position4;
    Vec angles4;
    BlWallBreakControllerData* data;
    BlWallBreakCameraData* camera_data;
    MkPfx* effect;
    MkProc* process;
    PlyrPdata* player;
    MkObj* player_object;
    MkObj* opponent_object;
    MkSobj* chunk;
    float camera_dx;
    float camera_dz;

    data = (BlWallBreakControllerData*)apdata;
    _mkproc_sleep_ticks = (float)data->initial_delay;
    aproc->vtbl->sleep();
    g_go_back_to_fight_position = 0;

    player_object = data->player->plyr_info->slot.mirror_a;
    opponent_object =
        data->player->his_plyr_pdata->plyr_info->slot.mirror_a;
    xfer_proc(get_player_proc(player_object), plyr_is_prone);
    xfer_proc(get_player_proc(opponent_object), plyr_is_prone);

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x32);
    if (g_game_info.bgnd_obj != 0) {
        chunk = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x32);
        if (chunk != 0) {
            unhide_sobj_and_children(chunk);
        }
    }

    g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.launched = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.bit6 = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.head_tracking = 0;
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit3 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 1;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.launched = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.bit6 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.head_tracking = 0;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 1;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit3 = 1;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 0;

    player = data->player;
    camera_data = 0;
    process = _create_mkproc_generic_tinystack(
        0xC01C, 0x1F, p_beetle_lair_downstairs_wall_break_cam_control,
        sizeof(BlWallBreakCameraData), (MkHdr**)&camera_data);
    if (process != 0 && camera_data != 0) {
        camera_data->player = player;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }

    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    bgnd_swap_level(1);
    hide_obj(g_bgnd_preloaded_models[5]);
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x28);
    if (g_game_info.bgnd_obj != 0) {
        chunk = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x28);
        if (chunk != 0) {
            hide_sobj_and_children(chunk);
        }
    }

    _mkproc_sleep_ticks = 19.0f;
    aproc->vtbl->sleep();
    if (g_bl_beetles != 0) {
        bl_init_beetle_pebbles_second_floor(g_bl_beetles_pdata);
    }
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    shake_camera_y(2, 0.01f);
    snd_req(0x8F);
    bl_front_wall_effect_at("smoke_wall", -0.2512f, 0.0f, 18.8613f);
    effect = find_pfx_by_name("smoke_wall");
    if (effect != 0) {
        effect->depth_bias = -50.0f;
    }

    camera_dx = camera_start.x - camera_end.x;
    camera_dz = camera_start.z - camera_end.z;

    position0 = (Vec){0.4512f, 0.8f, 18.2113f};
    velocity.x = 0.05f + camera_dx / 100.0f;
    velocity.y = camera_dz / 100.0f;
    velocity.z = 0.05f;
    angular_velocity.x = 0.05f;
    angular_velocity.y = 0.0f;
    angular_velocity.z = 0.0f;
    angles0 = (Vec){3.1415927f, 0.0f, 0.0f};
    chunk = obj_first_sobj(g_bgnd_preloaded_models[0]);
    bgnd_launch_chunk(
        chunk, &position0, &velocity, &angular_velocity, &angles0, 0,
        &scale, 1, -0.003f, 1);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    position1 = (Vec){0.0f, 1.1f, 18.2113f};
    velocity.x = 0.0f;
    velocity.y = 0.105f;
    velocity.z = -0.01f;
    angular_velocity.x = 0.043f;
    angular_velocity.y = -0.07f;
    angular_velocity.z = 0.0f;
    angles1 = (Vec){3.1415927f, 0.0f, 1.0471976f};
    chunk = obj_first_sobj(g_bgnd_preloaded_models[1]);
    bgnd_launch_chunk(
        chunk, &position1, &velocity, &angular_velocity, &angles1, 0,
        &scale, 1, -0.004f, 1);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    position2 = (Vec){0.6f, 1.3f, 18.2113f};
    velocity.x = camera_dx / 110.0f - 0.11f;
    velocity.y = camera_dz / 110.0f;
    velocity.z = 0.03f;
    angular_velocity.x = 0.05f;
    angular_velocity.y = 0.0f;
    angular_velocity.z = 0.18f;
    angles2.x = 3.1415927f;
    angles2.y = 0.0f;
    angles2.z = 0.0f;
    chunk = obj_first_sobj(g_bgnd_preloaded_models[2]);
    bgnd_launch_chunk(
        chunk, &position2, &velocity, &angular_velocity, &angles2, 0,
        &scale, 1, -0.004f, 1);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    position3 = (Vec){-0.8512f, 1.3f, 18.2113f};
    velocity.x = camera_dx / 300.0f - 0.05f;
    velocity.y = camera_dz / 200.0f;
    velocity.z = 0.2f;
    angular_velocity.x = 0.2f;
    angular_velocity.y = 0.0f;
    angular_velocity.z = 0.15f;
    angles3 = (Vec){3.1415927f, 0.0f, 0.0f};
    chunk = obj_first_sobj(g_bgnd_preloaded_models[3]);
    bgnd_launch_chunk(
        chunk, &position3, &velocity, &angular_velocity, &angles3, 0,
        &scale, 1, -0.003f, 1);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    position4 = (Vec){-0.6f, 0.6f, 18.4313f};
    velocity.x = camera_dx / 110.0f;
    velocity.y = camera_dz / 110.0f;
    velocity.z = 0.03f;
    angular_velocity.x = 0.01f;
    angular_velocity.y = 0.1f;
    angular_velocity.z = 0.0f;
    angles4.x = 3.1415927f;
    angles4.y = 3.1415927f;
    angles4.z = 0.0f;
    chunk = obj_first_sobj(g_bgnd_preloaded_models[4]);
    bgnd_launch_chunk(
        chunk, &position4, &velocity, &angular_velocity, &angles4, 0,
        &scale, 1, -0.004f, 1);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0x70);
    if (g_game_info.bgnd_obj != 0) {
        chunk = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x70);
        if (chunk != 0) {
            unhide_sobj(chunk);
        }
    }
    bl_front_wall_effect_at("bw_explosion", -0.2512f, 0.8f, 18.2113f);

    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    player_object = data->player->plyr_info->slot.mirror_a;
    opponent_object =
        data->player->his_plyr_pdata->plyr_info->slot.mirror_a;
    xfer_proc(get_player_proc(player_object), victim_fall_down_a_level);
    xfer_proc(get_player_proc(opponent_object), winner_watching_him_fall);
    _mkproc_sleep_ticks = 75.0f;
    aproc->vtbl->sleep();
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(
        p_beetle_lair_watch_remaining_fall_scene, 0.0f);
    return 0.0f;
}

extern void cam_recalc_midpoint(void);

static float p_beetle_lair_watch_remaining_fall_scene(void) {
    BlWallBreakControllerData* data;

    data = (BlWallBreakControllerData*)apdata;
    if (--data->remaining_scene_ticks != 0 &&
        g_go_back_to_fight_position == 0) {
        return 1.0f;
    }

    cam_recalc_midpoint();
    _mkproc_sleep_ticks = 50.0f;
    aproc->vtbl->sleep();
    g_bl_beetles_pdata->pebble_data->count = 0x1C;
    if (g_game_info.wall_hider != 0) {
        g_game_info.wall_hider->flag_bits.disabled = 0;
    }

    g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.launched = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.bit6 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.head_tracking = 1;
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 0;
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit3 = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 1;

    g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.launched = 1;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.bit6 = 1;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.head_tracking = 1;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit3 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 1;

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
    return -1.0f;
}

extern MkProc* plyr_anim_proc;
extern unsigned char shared_ani[];
extern float p_anim_idle(void);
extern void face_opponent_now(void);
extern void avoid_double_ani(void);
extern void init_air_move(void);
extern void launch_n_land_ani(
    AniData* animation, float launch_frame, float launch_step,
    float landing_frame, int landing_animation, float velocity_y,
    float gravity, float blend);
extern void shake_hit_voice(int shake_ticks, float rumble_scale, int hit_voice, int fighter_voice);
extern void set_ani_speed(float speed);
extern void ani_to_frame_x(float frame);
extern void face_position_now(Vec* position);
extern void bulvan_function(int command);
extern void force_away(float velocity, int duration, float damping,
                       int interval);
extern void plyr_bleed_mouth(PlyrPdata* player);
extern void glitch_to_ani(AniData* animation, int flags);
extern void ani_loop_more_frames(float frames);
extern void plyr_snd_req(int sound);
extern void ani_1_frame(void);
extern void random_hit(int group);
extern void damage_me(float amount);
extern int random_voice(int group);
extern void head_tracking_off(void);
extern void ani_to_end(void);
extern int do_i_have_life_left(void);
extern float j_getup_back_6(void);
extern void tightrope_restrictions_off(void);
extern void init_ground_move_no_aniproc(void);
extern void ani_to_blend_frame(float frame);
extern float p_blend_to_stance_in_10(void);

static float winner_watching_him_fall(void) {
    Vec bone_position;
    AniData* animation;
    MkSobj* debris_piece;
    float effect_x;
    float effect_y;
    float effect_z;

    plyr_obj->pos.value.x = 0.0f;
    plyr_obj->pos.value.y = 0.0f;
    plyr_obj->pos.value.z = 18.8713f;
    update_mkobj(plyr_obj);
    face_opponent_now();
    _mkproc_sleep_ticks = 2.0f;
    aproc->vtbl->sleep();
    face_opponent_now();
    _mkproc_sleep_ticks = 98.0f;
    aproc->vtbl->sleep();
    face_opponent_now();
    avoid_double_ani();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_my_state(0x3202);
    init_air_move();
    force_forward(0x78, 0x14, 0.1325f, 0.9f);
    animation = *(AniData**)&shared_ani[0x3C];
    launch_n_land_ani(
        animation, 0.0f, 0.0f, 28.0f, 0, 0.12f, -0.0035f, 0.2f);

    get_bone_world_pos(plyr_obj, 0xA, &bone_position);
    bone_position.y = g_game_info.field_34 + 0.015f;
    g_bgnd_preloaded_models[5]->pos.value.x = bone_position.x;
    g_bgnd_preloaded_models[5]->pos.value.y = bone_position.y;
    g_bgnd_preloaded_models[5]->pos.value.z = bone_position.z;
    g_bgnd_preloaded_models[5]->flags_08_bits.airborne = 1;
    g_bgnd_preloaded_models[5]->flags_08_bits.scale_active = 1;
    g_bgnd_preloaded_models[5]->flags_08_bits.angular_velocity_enabled = 1;
    g_bgnd_preloaded_models[5]->ang.x = 1.5707964f;
    g_bgnd_preloaded_models[5]->ang.y = 0.0f;
    g_bgnd_preloaded_models[5]->ang.z = 0.0f;
    g_bgnd_preloaded_models[5]->scale.x = 1.25f;
    g_bgnd_preloaded_models[5]->scale.y = 1.0f;
    g_bgnd_preloaded_models[5]->scale.z = 1.25f;
    obj_first_sobj(g_bgnd_preloaded_models[5])->z_offset = -50.0f;
    update_mkobj(g_bgnd_preloaded_models[5]);
    unhide_obj(g_bgnd_preloaded_models[5]);

    get_bone_world_pos(plyr_obj, 0xB, &bone_position);
    bone_position.y = g_game_info.field_34 + 0.03f;
    g_bgnd_preloaded_models[7]->pos.value.x = bone_position.x;
    g_bgnd_preloaded_models[7]->pos.value.y = bone_position.y;
    g_bgnd_preloaded_models[7]->pos.value.z = bone_position.z;
    g_bgnd_preloaded_models[7]->flags_08_bits.airborne = 1;
    g_bgnd_preloaded_models[7]->flags_08_bits.scale_active = 1;
    g_bgnd_preloaded_models[7]->flags_08_bits.angular_velocity_enabled = 1;
    g_bgnd_preloaded_models[7]->ang.x = 1.5707964f;
    g_bgnd_preloaded_models[7]->ang.y = 0.0f;
    g_bgnd_preloaded_models[7]->ang.z = 0.0f;
    g_bgnd_preloaded_models[7]->scale.x = 1.25f;
    g_bgnd_preloaded_models[7]->scale.y = 1.0f;
    g_bgnd_preloaded_models[7]->scale.z = 1.25f;
    obj_first_sobj(g_bgnd_preloaded_models[7])->z_offset = -40.0f;
    update_mkobj(g_bgnd_preloaded_models[7]);
    unhide_obj(g_bgnd_preloaded_models[7]);

    init_ground_move();
    shake_hit_voice(3, 0.03f, -1, 7);
    snd_req(0x90);
    bl_front_wall_effect_at(
        "dust_aland_gnd_pnd", plyr_obj->pos.value.x,
        g_game_info.field_34 + 0.01f, plyr_obj->pos.value.z);
    effect_z = plyr_obj->pos.value.z;
    effect_y = g_game_info.field_34 + 0.01f;
    effect_x = plyr_obj->pos.value.x;
    bl_front_wall_effect_at("wall_debris_1", effect_x, effect_y, effect_z);
    set_ani_speed(0.5f);
    ani_to_frame_x(50.0f);
    blend_to_stance(0.2f);
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 99.96%; initializer block @1626 is at +0x198 (retail +0xFC, TU data layout); loop fadds operand order remains. */
static float victim_fall_down_a_level(void) {
    Vec face_target = {0.0f, 0.0f, 0.0f};
    Vec target_position = {0.4512f, -0.5f, 18.8713f};
    Vec move_position = {0.0f, 0.0f, 0.0f};
    Vec follower_position;
    Vec bone_position;
    MkSobj* debris_piece;

    stop_me();
    bulvan_function(1);
    move_position.z = target_position.z;
    move_player(plyr_obj, &move_position, &plyr_obj->ang);
    face_target.y = plyr_obj->pos.value.y;
    face_position_now(&face_target);
    plyr_obj->pos_vel.y = 0.05f;
    force_away(0.22f, 0x50, 0.9f, 0x1E);

    get_bone_world_pos(plyr_obj, 0, &follower_position);
    bl_front_wall_effect_at(
        "debris_follower_fx", follower_position.x, follower_position.y,
        follower_position.z);
    plyr_bleed_mouth(plyr_pdata);
    plyr_obj->flags_08_bits.moving = 0;
    plyr_obj->flags_09_bits.launched = 0;
    glitch_to_ani(*(AniData**)&shared_ani[0x198], 0);
    set_ani_speed(1.3f);
    ani_loop_more_frames(3.0f);
    plyr_snd_req(0x45);
    ani_loop_more_frames(22.0f);

    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->gravity = -0.009f;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 0.1f) {
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    get_bone_world_pos(plyr_obj, 0, &bone_position);
    bone_position.y = g_game_info.field_34 + 0.015f;
    g_bgnd_preloaded_models[8]->pos.value.x = bone_position.x;
    g_bgnd_preloaded_models[8]->pos.value.y = bone_position.y;
    g_bgnd_preloaded_models[8]->pos.value.z = bone_position.z;
    g_bgnd_preloaded_models[8]->flags_08_bits.airborne = 1;
    g_bgnd_preloaded_models[8]->flags_08_bits.scale_active = 1;
    g_bgnd_preloaded_models[8]->flags_08_bits.angular_velocity_enabled = 1;
    g_bgnd_preloaded_models[8]->ang.x = 1.5707964f;
    g_bgnd_preloaded_models[8]->ang.y = 0.0f;
    g_bgnd_preloaded_models[8]->ang.z = 0.0f;
    g_bgnd_preloaded_models[8]->scale.x = 1.25f;
    g_bgnd_preloaded_models[8]->scale.y = 1.0f;
    g_bgnd_preloaded_models[8]->scale.z = 1.25f;
    debris_piece = obj_first_sobj(g_bgnd_preloaded_models[8]);
    update_mkobj(g_bgnd_preloaded_models[8]);
    g_bgnd_preloaded_models[8]->flags_08_bits.scale_active = 1;
    g_bgnd_preloaded_models[8]->scale.x = 2.0f;
    g_bgnd_preloaded_models[8]->scale.y = 2.0f;
    g_bgnd_preloaded_models[8]->scale.z = 2.0f;
    sobj_set_priority(debris_piece, 9);
    debris_piece->flags09_bits.bit6 = 1;
    debris_piece->flags09_bits.bit7 = 1;
    unhide_obj(g_bgnd_preloaded_models[8]);

    shake_camera(3, 0.03f);
    bl_front_wall_effect_at("dust_aland_gnd_pnd", plyr_obj->pos.value.x,
        g_game_info.field_34 + 0.01f, plyr_obj->pos.value.z);
    bl_front_wall_effect_at("wall_debris_1", plyr_obj->pos.value.x,
        g_game_info.field_34 + 0.01f, plyr_obj->pos.value.z);
    random_hit(9);
    random_hit(5);
    damage_me(0.05f);
    ck_rumble_controller(get_my_plyr_num(), 8, 0x19);
    random_voice(8);
    init_ground_move();
    head_tracking_off();
    blend_to_ani(*(AniData**)&shared_ani[0x60], 3, 0.1f);
    set_ani_speed(0.75f);
    ani_to_end();
    set_ani_speed(0.5f);
    blend_to_ani(*(AniData**)&shared_ani[0x64], 0, 0.2f);
    ani_loop_more_frames(10.0f);

    if (do_i_have_life_left() == 0) {
        while (g_game_info.flag_bits.level_transition_active) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(j_getup_back_6, 0.0f);
        return 0.0f;
    } else {
        plyr_pdata->death_type = 1;
        back_to_normal();
        plyr_obj->flags_09_bits.head_tracking = 0;
        tightrope_restrictions_off();
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, *(AnimScript**)&shared_ani[0x338], 3, 0.05f);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        ani_to_frame_x(2.0f);
        plyr_obj->flags_09_bits.launched = 0;
        plyr_anim_pdata->step = 0.7f;
        ani_to_frame_x(30.0f);
        ani_to_blend_frame(10.0f);
        init_ground_move();
        back_to_normal();
        rotate_towards_him(0.1f);
        while (do_i_have_life_left() == 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        xfer_proc(plyr_anim_proc, p_animate);
        turn_controllers_on();
        plyr_anim_pdata->step = 1.2f;
        ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(
            p_blend_to_stance_in_10, 0.0f);
        return 0.0f;
    }
}








/* TODO: [breakthrough] 97.121216%; fifth typed wall-position vector fixes frame; wall writes and TU rodata offset remain. */
static float p_beetle_lair_front_wall_breaking(void) {
    Vec camera_velocity = {0.0f, 0.0f, -0.05f};
    Vec effect_position = {0.0f, 0.0f, 0.0f};
    Vec wall_position = {0.0f, 0.0f, 0.0f};
    Vec effect_offset = {0.0f, 0.0f, 0.6f};
    Vec wall_offset = {1.5f, 0.75f, 0.0f};
    BlWallBreakControllerData* data;
    CameraObj* camera;
    MkObj* player_object;
    MkSobj* wall;

    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);


    _mkproc_sleep_ticks = 3.0f;
    data = (BlWallBreakControllerData*)apdata;
    aproc->vtbl->sleep();
    snd_req(0x8A);
    shake_camera_y(3, 0.03f);

    wall = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0xBE);
    player_object = data->player->plyr_info->slot.mirror_a;
    wall_position.x = player_object->pos.value.x + wall_offset.x;
    wall_position.y = player_object->pos.value.y + wall_offset.y;
    wall_position.z = 13.1f;
    wall->pos.x = wall_position.x;
    wall->pos.y = wall_position.y;
    wall->pos.z = wall_position.z;
    wall->flags_08_bits.bit6 = 1;
    wall->flags_08_bits.scale_dirty = 1;
    wall->scale.x = 2.0f;
    wall->scale.y = 3.0f;
    wall->scale.z = 3.0f;
    effect_position.z = 13.1f;
    unhide_sobj(wall);

    player_object = data->player->plyr_info->slot.mirror_a;
    effect_position.x = player_object->pos.value.x + effect_offset.x;
    effect_position.y = player_object->pos.value.y + effect_offset.y;
    effect_position.z = player_object->pos.value.z + effect_offset.z;
    effect_position.y += 0.1f;
    effect_position.z = 15.0f;
    bl_front_wall_effect_at(
        "fw_explosion", effect_position.x, effect_position.y,
        effect_position.z);
    bl_front_wall_effect_at(
        "smoke_front_wall", effect_position.x, effect_position.y + 0.5f,
        effect_position.z);

    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    camera_velocity.x = (camera->pos.x - effect_position.x) / 40.0f;
    camera_velocity.y =
        (camera->pos.y - effect_position.y + 0.2f) / 40.0f;
    camera_velocity.z = (camera->pos.z - effect_position.z) / 40.0f;
    bl_front_wall_effect_at(
        "at_cam_explosion", effect_position.x, effect_position.y,
        effect_position.z);
    fx_set_param_v3(
        fx("at_cam_explosion"), 0x201, camera_velocity.x,
        camera_velocity.y, camera_velocity.z);
    return -1.0f;
}

extern float p_idle_camera(void);
extern int move_to_end_point(const Vec* endpoint, float* initial_speed,
                             float* final_speed, int reset, float time);
extern void get_current_target(Vec* target);


/* TODO: [near miss] 99.93%; code exact; @1626 aggregate-literal pool offsets are 0x180 higher than retail (TU data layout). */
static float p_beetle_lair_downstairs_wall_break_cam_control(void) {
    Vec cut_position = {4.996f, 2.0f, 27.5f};
    Vec fixed_position = {1.166f, -9.5f, 43.734f};
    Vec endpoint = {3.0f, -8.4543f, 35.572f};
    Vec target = {0.4512f, -0.5f, 18.8713f};
    Vec cut_target = {0.0f, 0.0f, 18.8713f};
    CameraObj* camera;
    MkObj* opponent;
    Vec current_target;
    Vec movement;
    Vec look_target;
    float initial_speed = 0.0f;
    float final_speed = 0.0f;
    float delta_x;
    float delta_y;
    float delta_z;
    float pitch;
    float angle_offset = 0.0f;
    int follow_ticks;
    int move_ticks;
    unsigned int elapsed;
    BlWallBreakCameraData* data;

    elapsed = 0;
    follow_ticks = (int)(120.0f * inverse_game_speed);
    move_ticks = (int)(100.0f * inverse_game_speed);
    data = (BlWallBreakCameraData*)apdata;

    go_to_camera_cut(&cut_position, &cut_target);
    _mkproc_sleep_ticks = 37.0f;
    aproc->vtbl->sleep();
    xfer_camera(p_idle_camera, 1);

    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);


    get_current_target(&look_target);
    current_target.x = look_target.x;
    current_target.y = look_target.y;
    current_target.z = look_target.z;
    get_target_movement_vector(&look_target, &target, &movement, 2.0f);
    remove_camera_offsets();
    while (--follow_ticks > 0) {
        MkObj* player_object = data->player->plyr_info->slot.mirror_a;
        current_target.x = player_object->pos.value.x;
        current_target.y = player_object->pos.value.y;
        current_target.z = player_object->pos.value.z;
        look_at_target(&current_target);
        add_camera_offsets();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        remove_camera_offsets();
    }

    add_camera_offsets();
    set_camera_position(&fixed_position);
    look_at_target(&target);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    get_current_target(&look_target);
    current_target.x = look_target.x;
    current_target.y = look_target.y;
    current_target.z = look_target.z;
    target.x = 1.427625f;
    target.y = -8.6525f;
    target.z = 35.45204f;
    remove_camera_offsets();
    move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, 2.0f);
    final_speed = 0.07f;
    initial_speed = 0.01f;

    while (move_to_end_point(
               &endpoint, &initial_speed, &final_speed, 0, 3.1f) == 0 &&
           --move_ticks > 0) {
        Vec angle = {-0.33f, 3.1415927f, 0.0f};
        opponent = data->player->his_plyr_pdata->plyr_info->slot.mirror_a;
        delta_y = camera->pos.y - opponent->pos.value.y;
        delta_z = camera->pos.z - opponent->pos.value.z;
        delta_x = camera->pos.x - opponent->pos.value.x;
        angle.y = 3.1415927f + gxMathArcTanYX(delta_x, delta_z);
        pitch = gxMathArcTanYX(delta_y, delta_z);
        if (pitch > -0.33f) {
            angle.x = pitch;
        }
        if (pitch > 0.0f) {
            angle.x = 0.0f;
        }
        ++elapsed;
        if (elapsed > 42) {
            angle_offset += 0.008f;
            angle.y += angle_offset;
        }
        set_camera_angle(&angle);
        add_camera_offsets();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        remove_camera_offsets();
    }

    g_go_back_to_fight_position = 1;
    return -1.0f;
}

typedef struct BlFinalColumnPieceData {
    MkHdr hdr;
    Vec launch_position; /* +0x08 - retained by the controller */
    MkSobj* piece;       /* +0x14 */
    float gravity;       /* +0x18 */
    int mode;            /* +0x1C */
} BlFinalColumnPieceData;

typedef struct BlColumnPieceData {
    MkHdr hdr;
    int replacement_sobj_id; /* +0x08 */
    Vec rotation_source;     /* +0x0C */
    MkSobj* piece;           /* +0x18 */
} BlColumnPieceData;

static float p_launch_final_column_piece(void);
static float p_launch_column_piece(void);
static float p_bl_flip_column_piece(void);

static inline void bl_column_effect_at(const char* name, float x, float y,
                                       float z) {
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
                    pfx_bind_to_new_obj(effect, 0x8227);
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

static inline void bl_set_column_piece_motion(MkSobj* piece,
                                               const Vec* axis,
                                               float angular_scale) {
    piece->ang_vel.x = piece->pos_vel.y * axis->z -
        piece->pos_vel.z * axis->y;
    piece->ang_vel.y = piece->pos_vel.z * axis->x -
        piece->pos_vel.x * axis->z;
    piece->ang_vel.z = piece->pos_vel.x * axis->y -
        piece->pos_vel.y * axis->x;
    piece->ang_vel.x = angular_scale * piece->ang_vel.x;
    piece->ang_vel.y = angular_scale * piece->ang_vel.y;
    piece->ang_vel.z = angular_scale * piece->ang_vel.z;
}

static inline void bl_enable_column_piece_motion(MkSobj* piece) {
    piece->flags_08_bits.bit6 = 1;
    piece->flags_08_bits.bit5 = 1;
    piece->flags_08_bits.angular_velocity_enabled = 1;
}

static inline float bl_column_vector_length(const Vec* vector) {
    union {
        float f;
        unsigned int u;
    } estimate, input;
    float squared;

    squared = vector->x * vector->x + vector->y * vector->y +
        vector->z * vector->z;
    input.f = squared;
    if (squared <= 0.0f) {
        return 0.0f;
    }
    estimate.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 11) & 0x1FFF] << 8;
    estimate.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    return 0.5f *
        (estimate.f * (3.0f - (estimate.f * estimate.f) / squared));
}

static inline void bl_column_normalize_vector(Vec* vector) {
    float inverse_length;
    float x;

    x = vector->x;
    inverse_length = bgnd_inv_sqrt(vector->z * vector->z +
                                   (x * x + vector->y * vector->y));
    vector->x = x * inverse_length;
    vector->y *= inverse_length;
    vector->z *= inverse_length;
}







static inline void bl_set_vector(Vec* vector, float x, float y, float z) {
    vector->x = x;
    vector->y = y;
    vector->z = z;
}

static inline void bl_spawn_launch_column_piece(MkSobj* piece,
                                                const Vec* direction,
                                                float gravity, int mode) {
    BlFinalColumnPieceData* launch_data;
    MkProc* process;

    process = _create_mkproc_generic_tinystack(
        0x8105, 0x1F, p_launch_column_piece,
        sizeof(BlFinalColumnPieceData), (MkHdr**)&launch_data);
    if (process != 0 && launch_data != 0) {
        launch_data->piece = piece;
        launch_data->launch_position.x = direction->x;
        launch_data->launch_position.y = direction->y;
        launch_data->launch_position.z = direction->z;
        launch_data->gravity = gravity;
        launch_data->mode = mode;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}

/* TODO: [breakthrough] 98.29%; launch spawns are an inline helper (retail output slots);
 * flip/final output slots, sqrt/inv-sqrt temp slots and length CSE remain. */
static float p_beetle_lair_column_breaking(void) {
    Vec axis = {0.0f, 1.0f, 0.0f};
    BlColumnBreakData* data;
    MkObj* target;
    MkObj* reference;
    MkPfx* effect;
    MkProc* process;
    MkSobj* piece;
    unsigned int first_piece_id;
    unsigned int object_id;
    float effect_x;
    float blast_y;
    float effect_z;
    float flip_x;
    float flip_y;
    float flip_z;
    float length;
    float random_base_x;
    float random_base_z;
    float random_x;
    float random_z;
    float scale;
    float speed;

    data = (BlColumnBreakData*)apdata;
    _mkproc_sleep_ticks = 3.0f;
    aproc->vtbl->sleep();

    target = MK_HDR_LIVE(data->player->tracked_obj, data->player->tracked_obj_instance);

    if (target == 0) {
        return -1.0f;
    }
    reference = MK_HDR_LIVE(data->player->his_plyr_pdata->tracked_obj, data->player->his_plyr_pdata->tracked_obj_instance);

    if (reference == 0) {
        return -1.0f;
    }

    length = bl_column_vector_length(&data->direction);
    speed = (2.0f * length) / 5.0f;
    bl_column_normalize_vector(&data->direction);

    if (speed < 0.04) {
        scale = 0.04f;
    } else if (speed > 0.06f) {
        scale = 0.06f;
    } else {
        scale = speed;
    }
    data->direction.x *= scale;
    data->direction.y *= scale;
    data->direction.z *= scale;

    if (data->use_camera != 0) {
        special_move_cam_setup2(
            2.2f, 2.6f, 2.0f, -0.5f, 0.2f, 30, 100, 0, target, reference);
    }

    if (data->side == 0) {
        obj_create_sobjs_by_id(g_game_info.bgnd_obj, 13);
        if (g_game_info.bgnd_obj != 0) {
            piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, 13);
            if (piece != 0) {
                hide_sobj(piece);
            }
        }
        effect_x = 4.6f;
        first_piece_id = 0x82;
        blast_y = 1.3f;
        effect_z = 1.85f;
    } else {
        obj_create_sobjs_by_id(g_game_info.bgnd_obj, 12);
        if (g_game_info.bgnd_obj != 0) {
            piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, 12);
            if (piece != 0) {
                hide_sobj(piece);
            }
        }
        effect_x = -6.3f;
        first_piece_id = 0x78;
        blast_y = 1.3f;
        effect_z = 1.85f;
    }

    for (object_id = first_piece_id; object_id < first_piece_id + 6;
         ++object_id) {
        obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
        if (g_game_info.bgnd_obj != 0) {
            piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (piece != 0) {
                unhide_sobj(piece);
            }
        }
    }

    snd_req(0x8F);
    bl_column_effect_at("dust", effect_x, 0.8f, effect_z);
    effect = find_pfx_by_name("dust");
    if (effect != 0) {
        effect->depth_bias = 50.0f;
    }

    piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, first_piece_id + 4);
    piece->flags_08_bits.bit6 = 1;
    flip_x = 0.15f * data->direction.x;
    flip_y = 0.15f * data->direction.y;
    flip_z = 0.15f * data->direction.z;
    {
        BlColumnPieceData* flip_data;

        process = _create_mkproc_generic_tinystack(
            0x8105, 0x1F, p_bl_flip_column_piece,
            sizeof(BlColumnPieceData), (MkHdr**)&flip_data);
        if (process != 0 && flip_data != 0) {
            flip_data->piece = piece;
            flip_data->replacement_sobj_id = first_piece_id;
            flip_data->rotation_source.x = flip_x;
            flip_data->rotation_source.y = flip_y;
            flip_data->rotation_source.z = flip_z;
            if (g_game_info.bgnd_obj != 0) {
                mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
            }
        }
    }

    bl_column_effect_at("blast_col_fx", effect_x, blast_y, effect_z);
    bl_column_effect_at("blast2_col_fx", effect_x, blast_y, effect_z);

    piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, first_piece_id + 1);
    bl_enable_column_piece_motion(piece);
    bl_set_vector(&piece->pos_vel, 0.55f * data->direction.x, 0.02f,
                  0.55f * data->direction.z);
    bl_set_column_piece_motion(piece, &axis, -0.5f);
    bl_spawn_launch_column_piece(piece, &data->direction, 0.002f, 1);

    piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, first_piece_id + 2);
    bl_enable_column_piece_motion(piece);
    random_base_x = 1.05f * data->direction.x;
    random_base_z = 1.05f * data->direction.z;
    random_x = random_base_x + sfrand(0.01f);
    random_z = random_base_z + sfrand(0.01f);
    piece->pos_vel.x = random_x;
    piece->pos_vel.y = 0.08f;
    piece->pos_vel.z = random_z;
    bl_set_column_piece_motion(piece, &axis, -0.18f);
    rotate_xz(&piece->pos_vel, &piece->pos_vel, -0.7853982f);
    bl_spawn_launch_column_piece(piece, &data->direction, 0.003f, 0);

    piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, first_piece_id + 3);
    bl_enable_column_piece_motion(piece);
    bl_set_vector(&piece->pos_vel, 1.05f * data->direction.x, 0.1f,
                  1.05f * data->direction.z);
    bl_set_column_piece_motion(piece, &axis, -1.8f);
    rotate_xz(&piece->pos_vel, &piece->pos_vel, 0.5235988f);
    bl_spawn_launch_column_piece(piece, &data->direction, 0.0025f, 0);

    piece = obj_find_sobj_by_id(g_game_info.bgnd_obj, first_piece_id + 5);
    bl_enable_column_piece_motion(piece);
    bl_set_vector(&piece->pos_vel, 0.35f * data->direction.x, 0.02f,
                  0.35f * data->direction.z);
    piece->ang_vel.x = 0.0f;
    piece->ang_vel.y = 0.0f;
    piece->ang_vel.z = 0.01f;
    rotate_xz(&piece->pos_vel, &piece->pos_vel, -1.5707964f);
    {
        BlFinalColumnPieceData* launch_data;

        process = _create_mkproc_generic_tinystack(
            0x8105, 0x1F, p_launch_final_column_piece,
            sizeof(BlFinalColumnPieceData), (MkHdr**)&launch_data);
        if (process != 0 && launch_data != 0) {
            launch_data->piece = piece;
            launch_data->launch_position.x = data->direction.x;
            launch_data->launch_position.y = data->direction.y;
            launch_data->launch_position.z = data->direction.z;
            launch_data->gravity = 0.002f;
            if (g_game_info.bgnd_obj != 0) {
                mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
            }
        }
    }
    return -1.0f;
}

static inline void bl_launch_final_column_fx(const char* name, MkSobj* piece) {
    MkPfx* effect;
    unsigned int handle;
    float z;
    float y;
    float x;

    z = piece->pos.z;
    y = piece->pos.y;
    x = piece->pos.x;
    handle = fx_by_owner(name, 4);
    if (handle != 0) {
        fx_reset(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
            if (g_latest_obj_pfx == 0) {
                g_latest_obj_pfx =
                    pfx_bind_to_new_obj(effect, 0x8227);
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

static float p_launch_final_column_piece(void) {
    BlFinalColumnPieceData* data;
    MkSobj* piece;
    int ticks;
    float damping;

    ticks = 200;
    damping = (float)pow(0.992, game_speed);
    data = (BlFinalColumnPieceData*)apdata;
    if (data == 0) {
        return -1.0f;
    }

    piece = data->piece;
    while (piece->pos.y > 0.1f && --ticks != 0) {
        piece->pos_vel.y -= data->gravity * game_speed;
        piece->pos_vel.x *= damping;
        piece->pos_vel.z *= damping;
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    hide_sobj(piece);
    snd_req_vol(0x8C, 1.0f);

    bl_launch_final_column_fx("final_col_fx", piece);
    bl_launch_final_column_fx("dust_small_gnd_pnd", piece);
    return -1.0f;
}

static unsigned int bl_column_dust_number;

static inline void bl_launch_column_misc_fx(MkSobj* piece) {
    MkPfx* effect;
    MkObj* emitter_object;
    int handle;
    float z;
    float y;
    float x;

    z = piece->pos.z;
    y = piece->pos.y;
    x = piece->pos.x;
    handle = fx_by_owner("misc_col_fx", 4);
    if (handle != 0) {
        handle = fx_next_emitter(handle);
        if (handle != 0) {
            fx_resume_emit(handle);
            effect = pfx_from_emitter(handle);
            if (effect != 0) {
                emitter_object = pfx_bind_emitter_num_to_new_obj(
                    effect, 0x6015, emitter_id_from_handle(handle));
                if (emitter_object != 0) {
                    emitter_object->flags_08_bits.airborne = 1;
                    emitter_object->pos.value.x = x;
                    emitter_object->pos.value.y = y;
                    emitter_object->pos.value.z = z;
                    update_mkobj(emitter_object);
                }
            }
        }
    }
}

static inline void bl_launch_column_dust(const char* name, MkSobj* piece) {
    MkPfx* effect;
    MkPfx* first_dust;
    unsigned int handle;
    float z;
    float y;
    float x;

    z = piece->pos.z;
    y = piece->pos.y;
    x = piece->pos.x;
    handle = fx_by_owner(name, 4);
    if (handle != 0) {
        fx_reset(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
            if (g_latest_obj_pfx == 0) {
                g_latest_obj_pfx =
                    pfx_bind_to_new_obj(effect, 0x8227);
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

    first_dust = find_pfx_by_name("dust_small_1");
    if (first_dust != 0) {
        first_dust->depth_bias = 50.0f;
    }
}

static float p_launch_column_piece(void) {
    BlFinalColumnPieceData* data;
    int ticks;
    MkSobj* piece;
    float damping;

    ticks = 200;
    damping = (float)pow(0.992, game_speed);
    data = (BlFinalColumnPieceData*)apdata;
    if (data == 0) {
        return -1.0f;
    }

    piece = data->piece;
    while (piece->pos.y > 0.1f && --ticks != 0) {
        piece->pos_vel.y -= data->gravity * game_speed;
        piece->pos_vel.x *= damping;
        piece->pos_vel.z *= damping;
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    snd_req_vol(0x8C, 1.0f);
    hide_sobj(piece);

    bl_launch_column_misc_fx(piece);

    if (bl_column_dust_number == 0) {
        bl_launch_column_dust("dust_small_1", piece);
    } else if (bl_column_dust_number == 1) {
        bl_launch_column_dust("dust_small_2", piece);
    } else if (bl_column_dust_number == 2) {
        bl_launch_column_dust("dust_small_3", piece);
    } else {
        return -1.0f;
    }

    ++bl_column_dust_number;
    if (bl_column_dust_number > 2) {
        bl_column_dust_number = 0;
    }
    return -1.0f;
}
static float p_bl_flip_column_piece(void) {
    Vec rotation_axis = {0.0f, 1.0f, 0.0f};
    BlColumnPieceData* data;
    MkPfx* effect;
    MkSobj* piece;
    MkSobj* replacement;
    unsigned int handle;
    int replacement_sobj_id;
    float dust_x;
    float dust_y;
    float dust_z;
    float flip_x;
    float flip_y;
    float flip_z;

    data = (BlColumnPieceData*)apdata;
    if (data == 0) {
        return -1.0f;
    }

    piece = data->piece;
    piece->flags_08_bits.bit5 = 1;
    piece->flags_08_bits.angular_velocity_enabled = 1;
    piece->pos_vel.x = data->rotation_source.x;
    piece->pos_vel.y = data->rotation_source.y;
    piece->pos_vel.z = data->rotation_source.z;
    piece->ang_vel.x = piece->pos_vel.y * rotation_axis.z -
        piece->pos_vel.z * rotation_axis.y;
    piece->ang_vel.y = piece->pos_vel.z * rotation_axis.x -
        piece->pos_vel.x * rotation_axis.z;
    piece->ang_vel.z = piece->pos_vel.x * rotation_axis.y -
        piece->pos_vel.y * rotation_axis.x;
    piece->ang_vel.x = 2.0f * piece->ang_vel.x;
    piece->ang_vel.y = 2.0f * piece->ang_vel.y;
    piece->ang_vel.z = 2.0f * piece->ang_vel.z;
    piece->pos_vel.y = 0.05f;

    while (piece->pos.y > 0.75f) {
        piece->pos_vel.y -= 0.004f * game_speed;
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    piece->pos.y = 0.2f;

    flip_z = piece->pos.z;
    flip_y = piece->pos.y;
    flip_x = piece->pos.x;
    handle = fx_by_owner("flip_col_fx", 4);
    if (handle != 0) {
        fx_reset(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
            if (g_latest_obj_pfx == 0) {
                g_latest_obj_pfx =
                    pfx_bind_to_new_obj(effect, 0x8227);
            }
            if (g_latest_obj_pfx != 0) {
                g_latest_obj_pfx->flags_08_bits.airborne = 1;
                g_latest_obj_pfx->pos.value.x = flip_x;
                g_latest_obj_pfx->pos.value.y = flip_y;
                g_latest_obj_pfx->pos.value.z = flip_z;
                update_mkobj(g_latest_obj_pfx);
                resume_effect("flip_col_fx");
            }
        }
    }

    snd_req(0x8B);
    dust_z = piece->pos.z;
    dust_y = piece->pos.y;
    dust_x = piece->pos.x;
    handle = fx_by_owner("dust_small_gnd_pnd_2", 4);
    if (handle != 0) {
        fx_reset(handle);
        effect = pfx_from_handle(handle);
        if (effect != 0) {
            g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
            if (g_latest_obj_pfx == 0) {
                g_latest_obj_pfx =
                    pfx_bind_to_new_obj(effect, 0x8227);
            }
            if (g_latest_obj_pfx != 0) {
                g_latest_obj_pfx->flags_08_bits.airborne = 1;
                g_latest_obj_pfx->pos.value.x = dust_x;
                g_latest_obj_pfx->pos.value.y = dust_y;
                g_latest_obj_pfx->pos.value.z = dust_z;
                update_mkobj(g_latest_obj_pfx);
                resume_effect("dust_small_gnd_pnd_2");
            }
        }
    }

    shake_camera(3, 0.03f);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    replacement_sobj_id = data->replacement_sobj_id;
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, replacement_sobj_id);
    if (g_game_info.bgnd_obj != 0) {
        replacement = obj_find_sobj_by_id(
            g_game_info.bgnd_obj, replacement_sobj_id);
        if (replacement != 0) {
            hide_sobj(replacement);
        }
    }
    hide_sobj(piece);
    piece->pos_vel.z = 0.0f;
    piece->pos_vel.y = 0.0f;
    piece->pos_vel.x = 0.0f;
    piece->ang_vel.z = 0.0f;
    piece->ang_vel.y = 0.0f;
    piece->ang_vel.x = 0.0f;
    return -1.0f;
}
void bgnd_set_viewing_of_danger_zones(int enabled) {
    g_game_info.switch_input_flags.view_danger_zones = enabled;
    set_collision_render_state(enabled);
}
/* TODO: [breakthrough needed] 66.95%; rebuild sequence agrees; retail keeps redundant post-create null-normalization branches our build folds (488 vs 460 bytes). */
void bgnd_set_danger_zone_y_angle(float y_angle) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    CollisionShape box_shape;
    CollisionShape cylinder_shape;
    CollisionShape special_cylinder_shape;

    zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
    if (zone->shape_type == 0) {
        if (g_active_bgnd_danger_zone <= 24) {
            zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
            if (zone->obstacle != 0) {
                delete_obstacle_from_background_by_id(zone->obstacle_id);
                zone->obstacle = 0;
            }
        }
        zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
        zone->y_angle = y_angle;
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
                &special_cylinder_shape, &zone->center, zone->width,
                zone->height);
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
}
/* TODO: [breakthrough needed] 67.00%; same rebuild as the y-angle setter; four folded null-normalization branches (488 vs 460 bytes). */
void bgnd_set_danger_zone_depth(float depth) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    CollisionShape box_shape;
    CollisionShape cylinder_shape;
    CollisionShape special_cylinder_shape;

    zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
    if (zone->shape_type == 0) {
        if (g_active_bgnd_danger_zone <= 24) {
            zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
            if (zone->obstacle != 0) {
                delete_obstacle_from_background_by_id(zone->obstacle_id);
                zone->obstacle = 0;
            }
        }
        zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
        zone->depth = depth;
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
                &special_cylinder_shape, &zone->center, zone->width,
                zone->height);
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
}
void bgnd_set_danger_zone_width(float width);
void bgnd_set_danger_zone_radius(float radius) {
    int shape_type;

    shape_type = bgnd_danger_zones[g_active_bgnd_danger_zone].shape_type;
    if (shape_type == 1 || shape_type == 2) {
        bgnd_set_danger_zone_width(radius);
    }
}
/* TODO: [breakthrough needed] 72.47%; shape paths and state agree; retail's redundant post-create null normalization is folded here (476 vs 448 bytes). */
void bgnd_set_danger_zone_width(float width) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    CollisionShape box_shape;
    CollisionShape cylinder_shape;
    CollisionShape special_cylinder_shape;

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
/* TODO: [breakthrough needed] 67.68%; center stores and rebuild agree; folded null normalization shifts scheduling (456 vs 436 bytes). */
void bgnd_set_danger_zone_center_position(float x, float y, float z) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    CollisionShape box_shape;
    CollisionShape cylinder_shape;
    CollisionShape special_cylinder_shape;

    zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
    zone->center.x = x;
    zone->center.y = y;
    zone->center.z = z;
    if (g_active_bgnd_danger_zone <= 24 && zone->obstacle != 0) {
        delete_obstacle_from_background_by_id(zone->obstacle_id);
        zone->obstacle = 0;
    }
    zone = &bgnd_danger_zones[g_active_bgnd_danger_zone];
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
void bgnd_delete_danger_zone(unsigned int zone_index) {
    if (zone_index <= 24) {
        if (bgnd_danger_zones[zone_index].obstacle != 0) {
            delete_obstacle_from_background_by_id(
                bgnd_danger_zones[zone_index].obstacle_id);
            bgnd_danger_zones[zone_index].obstacle = 0;
        }
    }
}
void bgnd_enable_danger_zone(unsigned int zone_index, int enabled) {
    BgndCollisionItem* item;
    BgndDangerZone* zone;
    MkPtr** list;
    MkPtr* link;
    unsigned int obstacle_id;
    MkPtr* next;

    if (zone_index < 24) {
        zone = &bgnd_danger_zones[zone_index];
        if (zone->obstacle != 0) {
            if (enabled != 0) {
                if (zone->collision_script_function != 0) {
                    obstacle_id = zone->obstacle_id;
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
                            } else if (item->collision_id == obstacle_id) {
                                item->flags.bits.disabled = 0;
                                break;
                            } else {
                                link = link->next;
                            }
                        }
                    }
                }
                set_background_obstacle_disable_flag(
                    bgnd_danger_zones[zone_index].obstacle_id, 0);
            } else {
                if (zone->collision_script_function != 0) {
                    obstacle_id = zone->obstacle_id;
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
                            } else if (item->collision_id == obstacle_id) {
                                item->flags.bits.disabled = 1;
                                break;
                            } else {
                                link = link->next;
                            }
                        }
                    }
                }
                set_background_obstacle_disable_flag(
                    bgnd_danger_zones[zone_index].obstacle_id, 1);
            }
        }
    }
}
void bgnd_set_active_danger_zone(unsigned int zone) {
    if (zone < 0x18) {
        g_active_bgnd_danger_zone = zone;
    }
}
/* TODO: [breakthrough needed] 70.37%; zone setup and registration agree; retail's redundant null normalization after obstacle creation is folded (584 vs 568 bytes). */
void bgnd_create_danger_zone(
    int shape_type, unsigned int zone_index, unsigned int obstacle_id,
    float height, unsigned int collision_script_function) {
    ArenaObstacle* obstacle;
    BgndDangerZone* zone;
    Vec center = {0.0f, 0.0f, 0.0f};
    CollisionShape box_shape;
    CollisionShape cylinder_shape;
    CollisionShape special_cylinder_shape;

    if (shape_type < 3 && zone_index < 24) {
        if (get_obstacle_type_from_id(obstacle_id) != 7) {
            return;
        }
        zone = &bgnd_danger_zones[zone_index];
        if (zone->obstacle == 0) {
            zone->shape_type = shape_type;
            zone->obstacle_id = obstacle_id;
            zone->collision_script_function = collision_script_function;
            zone->height = height;
            zone->width = 1.0f;
            zone->depth = 1.0f;
            zone->y_angle = 0.0f;
            zone->center = center;
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
                    &special_cylinder_shape, &zone->center, zone->width,
                    zone->height);
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
            if (collision_script_function != 0) {
                bgnd_collision_if_monitor_col_as(
                    7, obstacle_id, collision_script_function, 0);
            }
            set_background_obstacle_repel_flag(obstacle_id, 0);
            bgnd_enable_danger_zone(zone_index, 0);
            g_active_bgnd_danger_zone = zone_index;
        }
    }
}
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
static inline int bgnd_list_exists(MkPtr** list) {
    return list != 0;
}

static float p_npc_track_colshape(void) {
    MkObj* object;
    BgndNpc* npc;
    ArenaObstacle* obstacle;
    BgndNpcCollisionTrackData* data;
    CollisionObj* collision;
    MkPtr* node;
    MkPtr* next;
    CollisionShape shape;
    Vec center;

    data = (BgndNpcCollisionTrackData*)apdata;
    npc = data->npc;
    obstacle = npc->obstacle;
    object = npc->object;
    if (bgnd_list_exists(&obstacle->shapes)) {
        node = obstacle->shapes;
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
    CollisionShape shape;
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

void bgnd_npc_get_pos(unsigned int npc_id, Vec* position) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc != 0) {
        position->x = npc->object->pos.value.x;
        position->y = npc->object->pos.value.y;
        position->z = npc->object->pos.value.z;
    }
}
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

int bgnd_npc_get_aux_int_data(unsigned int npc_id, unsigned int index) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc == 0 || npc->aux_data == 0) {
        return 0;
    }
    if (index >= 2) {
        return 0;
    }
    return npc->aux_data->values[index];
}

void bgnd_npc_set_aux_int_data(
    unsigned int npc_id, unsigned int index, int value) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    if (npc == 0 || npc->aux_data == 0) {
        return;
    }
    if (index >= 2) {
        return;
    }
    npc->aux_data->values[index] = value;
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

static inline float bgnd_normalize_y_angle(float angle) {
    int fixed_angle = (int)(166886.1f * angle) & 0xFFFFF;
    return 0.000005992112f * (float)fixed_angle;
}

/* TODO: [near miss] 99.65%; coefficient and angle FPR coloring remains. */
void bgnd_npc_set_pos_vel_heading(unsigned int npc_id, float speed) {
    BgndNpc* npc;
    float heading;
    Vec direction;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.gravity_enabled = 1;
    heading = bgnd_normalize_y_angle(npc->object->ang.y);
    direction.x = gxMathSin(heading);
    direction.y = 0.0f;
    direction.z = gxMathCos(heading);
    npc->object->pos_vel.x = direction.x * speed;
    npc->object->pos_vel.y = direction.y * speed;
    npc->object->pos_vel.z = direction.z * speed;
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

/* TODO: [near miss] 99.70149%; normalization constants occupy swapped f2/f3 and pool labels differ. */
void bgnd_npc_adjust_y_ang(unsigned int npc_id, float angle) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.angular_velocity_enabled = 1;
    npc->object->ang.y += angle;
    npc->object->ang.y = bgnd_normalize_y_angle(npc->object->ang.y);
}

/* TODO: [near miss] 99.69231%; two angle constants use swapped FPRs; stop at coloring. */
void bgnd_npc_set_y_ang(unsigned int npc_id, float angle) {
    BgndNpc* npc;

    npc = bgnd_find_npc(npc_id);
    npc->object->flags_08_bits.angular_velocity_enabled = 1;
    npc->object->ang.y = angle;
    npc->object->ang.y = bgnd_normalize_y_angle(npc->object->ang.y);
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
BgndNpc* bgnd_fetch_npc(unsigned int npc_id) {
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
static float bgnd_npc_play_ani(void);
float bgnd_npc_idle(void);
float p_animate(void);

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
        g_bgnd_preloaded_models[npc_id] = load_named_model_from_slot(
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
    object->flags_08_bits.angular_velocity_enabled = 1;
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
        set_root_and_obj_movement_weights(npc->animation, 0.0f, 1.0f);
        set_anim_script(
            npc->animation, (AniData*)bgnd_animation_table[animation_id], 0);
        npc->animation->hand_transition = 0.125f;
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

float bgnd_npc_idle(void) {
    BgndNpcAniCommand* command;

    command = (BgndNpcAniCommand*)apdata;
    xfer_proc(command->npc->anim_process, p_animate);
    for (;;) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}
void bgnd_set_launch_velocity_based_on_sobj_pos(
    int velocity_index, unsigned int source_id, unsigned int target_id,
    void* script, float horizontal_velocity, float vertical_velocity) {
    Vec* velocity;
    Vec* source;
    Vec* target;
    float inverse_length;
    float x;
    float x_squared;
    float z_squared;
    float length_squared;

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
    length_squared = x_squared + z_squared;
    inverse_length = bgnd_inv_sqrt(length_squared);
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
void bgnd_set_sobj_launch_params(
    int velocity_index, int angle_index, unsigned int angle_source,
    void* script, float impact_scale, float angle_scale,
    float random_x, float random_y, float random_z,
    float vertical_velocity, float heading) {
    Vec* angles;
    Vec* velocity;
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
        source = &g_game_info.impact_vector;
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

#define BGND_MODEL_BACKGROUND ((int)0xDDDDEEEE)

void bgnd_launch_sobj(
    int model_index, unsigned int object_id, unsigned int position_index,
    unsigned int velocity_index, unsigned int angular_velocity_index,
    unsigned int angle_index, float parameter, unsigned int scale_index) {
    BgndSobjLaunchEntry* entry;
    MkObj* model;
    MkSobj* object;
    Vec* vector;
    unsigned int i;

    switch (model_index) {
    case BGND_MODEL_BACKGROUND:
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object == 0) {
            return;
        }
        break;
    default:
        model = g_bgnd_preloaded_models[model_index];
        if (model == 0) {
            return;
        }
        object = obj_find_sobj_by_id(model, object_id);
        if (object == 0) {
            return;
        }
        break;
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
        vector = &g_bgnd_scratch_pad_vectors[velocity_index];
        object->flags_08_bits.bit5 = 1;
        bgnd_copy_vector(&object->pos_vel, vector);
    }
    if (angular_velocity_index != 0xFF) {
        vector = &g_bgnd_scratch_pad_vectors[angular_velocity_index];
        object->flags_08_bits.angular_velocity_enabled = 1;
        bgnd_copy_vector(&object->ang_vel, vector);
    }
    if (scale_index != 0xFF) {
        vector = &g_bgnd_scratch_pad_vectors[scale_index];
        object->flags_08_bits.scale_dirty = 1;
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
void bgnd_kill_all_launched_sobjs(void) {
    unsigned int i;

    i = 0;
    do {
        if (g_sobj_launch_monitor_pdata->entries[i].active == 1) {
            hide_sobj(g_sobj_launch_monitor_pdata->entries[i].object);
            g_sobj_launch_monitor_pdata->entries[i].active = 0;
        }
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
void start_sobj_launch_monitor(void) {
    BgndSobjLaunchMonitor* monitor;
    MkProc* process;
    unsigned int i;

    monitor = 0;
    process = _create_mkproc_generic_bigstack(
        0xC019, 0x1F, p_bgnd_launch_sobj_monitor,
        sizeof(BgndSobjLaunchMonitor), (MkHdr**)&monitor);
    if (process != 0 && monitor != 0) {
        for (i = 0; i < 25; i++) {
            monitor->entries[i].active = 0;
        }
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    g_sobj_launch_monitor_pdata = monitor;
    g_active_launched_sobj_pdata = 0;
}
static float p_bgnd_launch_sobj_monitor(void) {
    BgndSobjLaunchMonitor* monitor;
    BgndSobjLaunchEntry* entry;
    unsigned int i;
    CmdScript* script;
    CmdScript* previous_script;

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
static int launch_sobj_watch_y_far_down(BgndSobjLaunchEntry* entry,
                                         unsigned int unused) {
    Vec* position;

    (void)unused;
    position = sobj_get_world_pos(entry->object);
    return position->y <= -20.0f;
}
static int launch_sobj_watch_y_ground_plane(BgndSobjLaunchEntry* entry,
                                             unsigned int unused) {
    Vec* position;

    (void)unused;
    position = sobj_get_world_pos(entry->object);
    return position->y <= g_game_info.field_34 + 0.03f;
}
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
void start_chunk_launch_monitor(void) {
    BgndChunkLaunchMonitor* monitor;
    MkProc* process;
    unsigned int i;

    monitor = 0;
    process = _create_mkproc_generic_bigstack(
        0xC018, 0x1F, p_bgnd_launch_chunk_monitor,
        sizeof(BgndChunkLaunchMonitor), (MkHdr**)&monitor);
    if (process != 0 && monitor != 0) {
        for (i = 0; i < 25; i++) {
            monitor->entries[i].active = 0;
        }
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
    g_chunk_launch_monitor_pdata = monitor;
}
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
                        y = entry->object->pos.y;
                        z = entry->object->pos.z;
                        y += 0.5f;
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
MkHdr* get_sobj_pebble_obj(MkSobj* object) {
    return MK_LIVE(object->bound_hdr, object->bound_instance);
}

void* get_general_pebble_data(PebbleData* pebble_data) {
    return pebble_data->user_data;
}
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
void pebble_get_pos(int player, int index, Vec* position) {
    BgndPebbleControl* pebbles;

    pebbles = g_pebbles_pdata[player]->collection->pebbles;
    position->x = pebbles[index].position.x;
    position->y = pebbles[index].position.y;
    position->z = pebbles[index].position.z;
}
void pebble_set_pos(int player, int index, Vec* position) {
    BgndPebbleControl* pebbles;

    pebbles = g_pebbles_pdata[player]->collection->pebbles;
    pebbles[index].position.x = position->x;
    pebbles[index].position.y = position->y;
    pebbles[index].position.z = position->z;
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
void bgnd_pebble_burst_set_end_state(int player, unsigned int first,
                                     unsigned int end, int state) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    unsigned int index;

    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection != 0) {
        pebbles = collection->pebbles;
        for (index = first; index < end; index++) {
            pebbles[index].end_behavior = state;
        }
    }
}
void bgnd_pebble_burst_set_value_min_max(int player, unsigned int first,
                                         unsigned int end, int field,
                                         void* script, float minimum,
                                         float maximum) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    unsigned int index;
    Vec* value;
    Vec direction;
    float magnitude;

    (void)script;
    if (first >= (unsigned int)g_pebbles_pdata[player]->count ||
        end > (unsigned int)g_pebbles_pdata[player]->count || first >= end) {
        return;
    }

    collection = g_pebbles[player];
    if (collection == 0) {
        return;
    }
    if (minimum > maximum) {
        return;
    }

    pebbles = collection->pebbles;
    for (index = first; index < end; index++) {
        switch (field) {
        case 0:
            value = &pebbles[index].position;
            break;
        case 1:
            value = &pebbles[index].velocity;
            break;
        case 2:
            value = &pebbles[index].angular_velocity;
            break;
        case 4:
            value = &pebbles[index].angles;
            break;
        case 3:
            value = &pebbles[index].bounce_velocity;
            break;
        default:
            continue;
        }

        direction.z = 0.0f;
        direction.y = 0.0f;
        direction.x = 0.0f;
        magnitude = minimum + frand(maximum - minimum);
        uv_from_angle_y(&direction, frand(6.2831855f));
        direction.x *= magnitude;
        direction.z *= magnitude;
        value->x = direction.x;
        value->z = direction.z;
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

void bgnd_pebble_burst_set_value(int player, unsigned int first,
                                 unsigned int end, int field, void* script,
                                 float x, float y, float z, float x_spread,
                                 float y_spread, float z_spread) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    unsigned int index;
    Vec* value;

    (void)script;
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
        switch (field) {
        case 0:
            value = &pebbles[index].position;
            break;
        case 1:
            value = &pebbles[index].velocity;
            break;
        case 2:
            value = &pebbles[index].angular_velocity;
            break;
        case 4:
            value = &pebbles[index].angles;
            break;
        case 3:
            value = &pebbles[index].bounce_velocity;
            break;
        default:
            continue;
        }

        bgnd_set_randomized_component(&value->x, x, x_spread);
        bgnd_set_randomized_component(&value->y, y, y_spread);
        bgnd_set_randomized_component(&value->z, z, z_spread);
    }
}
static void bgnd_pebble_burst_at(int player, const Vec* position,
                                 unsigned int first, unsigned int end) {
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    unsigned int index;
    BgndPebbleControl* pebble;

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
        if (position != 0) {
            pebbles[index].position.x = position->x + sfrand(0.1f);
            pebbles[index].position.y = position->y + sfrand(0.05f);
            pebbles[index].position.z = position->z + sfrand(0.1f);
        }

        pebble = &pebbles[index];
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
void bgnd_pebble_change_current_behavior_to_bounce(
    float velocity_x, float velocity_y, float velocity_z, float angular_x,
    float angular_y, float angular_z, unsigned int ticks, int bounce_param) {
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
void bgnd_pebble_change_current_behavior(
    float velocity_x, float velocity_y, float velocity_z, float angular_x,
    float angular_y, float angular_z, unsigned int ticks, int behavior_param) {
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
/* TODO: [near miss] 99.91453%; pebble-array base uses r7 instead of retail r29; stop at coloring. */
void bgnd_pebble_launch_at_time(
    int player, int index, float position_x, float position_y, float position_z,
    float scale_x, float scale_y, float scale_z, float angle_x, float angle_y,
    float angle_z, unsigned int delay, int behavior_param) {
    BgndPebbleControl* pebble;

    unhide_sobj(g_pebbles_pdata[player]->sobj);
    pebble = &g_pebbles_pdata[player]->collection->pebbles[index];

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

    MKMatrixRotatXZYScaleTranslate(
        &g_pebbles_pdata[player]->collection->matrices[index],
                                   pebble->angles.x, pebble->angles.z,
                                   pebble->angles.y, &pebble->scale,
                                   &pebble->position);
}
void bgnd_pebble_simple_launch_at_time(int player, int index,
                                       unsigned int delay,
                                       int behavior_param) {
    BgndPebblePlayerData* player_data;
    BgndPebbleControl* pebble;

    player_data = g_pebbles_pdata[player];
    pebble = player_data->collection->pebbles;
    pebble += index;
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
/* TODO: [near miss] 96.33%; one extra saved GPR (0x70 frame): retail frees the collection register in loop 2 and reloads g_pebbles[player]; plain reload regresses. */
BgndPebbleControl* bgnd_create_pebbles_with_sobj(
    MkSobj* object, unsigned int player, int mode, unsigned int count) {
    BgndPebbleCollection* collection;
    BgndPebblePlayerData* player_data;
    BgndPebblePlayerData* result;
    BgndPebbleControl* pebble;
    MkProc* process;
    unsigned int index;
    unsigned int i;

    if (player >= 20) {
        return 0;
    }
    object->flags_08_bits.bit6 = 1;
    object->z_offset = 0.0f;
    object->flags09_bits.bit4 = 1;
    object->flags09_bits.bit3 = 1;

    collection = (BgndPebbleCollection*)create_pebble_userdata(
        object, count, sizeof(BgndPebbleControl));
    for (i = 0; i < count; i++) {
        MKMatrixTranslate(
            &collection->matrices[i], &object->pos, 0);
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
    case BGND_MODEL_BACKGROUND:
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
static inline MkSobj* bgnd_lookup_sobj(int model_index, unsigned int object_id) {
    MkSobj* object;

    switch (model_index) {
    case BGND_MODEL_BACKGROUND:
        object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
        if (object == 0) {
            return 0;
        }
        break;
    default:
        unhide_obj(g_bgnd_preloaded_models[model_index]);
        if (g_bgnd_preloaded_models[model_index] == 0) {
            return 0;
        } else {
            object = obj_find_sobj_by_id(
                g_bgnd_preloaded_models[model_index], object_id);
            if (object == 0) {
                object = obj_first_sobj(g_bgnd_preloaded_models[model_index]);
                if (object == 0) {
                    return 0;
                }
            }
        }
    }
    return object;
}
void bgnd_set_material_color(int model_index, unsigned int object_id,
                             int red, int green, int blue, int alpha) {
    MkSobj* object;
    RwRGBA color;

    object = bgnd_lookup_sobj(model_index, object_id);
    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = alpha;
    set_atomic_material_color(object->atomic, &color);
}

MkSobj* bgnd_fetch_sobj(int model_index, unsigned int object_id) {
    return bgnd_lookup_sobj(model_index, object_id);
}
MkObj* bgnd_fetch_obj(int model_id) {
    return g_bgnd_preloaded_models[model_id];
}
void bgnd_unhide_pebbles(int player) {
    unhide_sobj(g_pebbles_pdata[player]->sobj);
}
void bgnd_hide_pebbles(int player) {
    unsigned int index;
    BgndPebbleControl* pebbles;

    pebbles = g_pebbles[player]->pebbles;
    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++) {
        pebbles[index].state = 0;
        pebbles[index].position.y = -200.0f;
        MKMatrixTranslate(&g_pebbles[player]->matrices[index],
            &pebbles[index].position, 0);
    }
    hide_sobj(g_pebbles_pdata[player]->sobj);
}
void bgnd_pebble_rand_scale(int player, void* script, float base,
                            float range) {
    BgndPebbleControl* pebble;
    BgndPebbleControl* pebbles;
    float scale;
    unsigned int index;

    (void)script;
    pebbles = g_pebbles[player]->pebbles;
    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++) {
        pebble = &pebbles[index];
        scale = base + frand(range);
        pebble->scale.z = scale;
        pebble->scale.y = scale;
        pebble->scale.x = scale;
    }
}
void bgnd_pebble_gravity(int player, void* script, float gravity) {
    BgndPebbleControl* pebbles;
    unsigned int index;

    (void)script;
    pebbles = g_pebbles[player]->pebbles;
    for (index = 0; index < (unsigned int)g_pebbles_pdata[player]->count;
         index++) {
        pebbles[index].gravity = gravity;
    }
}
void bgnd_init_pebbles(int player, unsigned int first, unsigned int end) {
    BgndPebbleControl* pebble;
    unsigned int index;

    for (index = first; index < end; index++) {
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
            &g_pebbles[player]->matrices[index], pebble->angles.x,
            pebble->angles.z, pebble->angles.y,
            &pebble->scale, &pebble->position);
    }
}
static float p_pebble_manual_monitor(void) {
    BgndPebbleControl* pebble;
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
    unsigned int i;
    BgndPebbleMonitor* monitor;

    monitor = (BgndPebbleMonitor*)pdata_of_proc(aproc);
    collection = (BgndPebbleCollection*)monitor->pebble_data;
    pebbles = collection->pebbles;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    if (mode_of_play != 9 && get_game_state() != 5 &&
        (g_game_info.plyr0.slot.mirror_a == 0 ||
         g_game_info.plyr1.slot.mirror_a == 0)) {
        return 1.0f;
    }
    for (i = 0; i < (unsigned int)monitor->count; i++) {
        pebble = &pebbles[i];

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
static float p_pebble_burst_monitor(void) {
    BgndPebbleControl* pebble;
    BgndPebbleMonitor* monitor;
    BgndPebbleCollection* collection;
    unsigned int end_state;
    BgndPebbleControl* pebbles;
    unsigned int i;
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
/* TODO: [near miss] 98.86%; state-13 FP coloring and one call argument order remain. */
static float p_pebble_path_monitor(void) {
    BgndPebbleMonitor* monitor;
    BgndPebbleControl* pebble;
    BgndPebbleCollection* collection;
    BgndPebbleControl* pebbles;
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
            pebble->angles.y = pebble->angles.y + step_y;
            pebble->angles.z = step_z + pebble->angles.z;
            if (dz * dz + (dx * dx + dy * dy) < 50.0f) {
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
void bgnd_remove_cracks(void) {
    if (g_bgnd_cracks != 0) {
        bgnd_reset_crack_pool();
    }
    g_bgnd_cracks = 0;
}
void bgnd_init_cracks(void) {
    unsigned int i;
    BgndCrack* cracks;

    if (g_bgnd_cracks != 0) {
        g_bgnd_last_crack_overwritten = 0;
        cracks = g_bgnd_cracks->user_data;
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
}
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
/* TODO: [near miss] 92.54%; zero stores via rodata copy; scalar zeroing is 100% text but drops the TU's zero-Vec pool entry (note 1534). */
static float p_crack_placer(void) {
    BgndCrackPlacer* placer;
    PebbleData* crack_pool;
    BgndCrack* cracks;
    Vec position = {0.0f, 0.0f, 0.0f};
    unsigned int crack_index;
    int place_crack;

    placer = (BgndCrackPlacer*)pdata_of_proc(aproc);
    crack_pool = placer->crack_pool;
    place_crack = 0;
    cracks = crack_pool->user_data;
    if (g_bgnd_cracks == 0) {
        return -1.0f;
    }
    if (g_game_info.plyr0.slot.mirror_a == 0 ||
            g_game_info.plyr1.slot.mirror_a == 0) {
        return 1.0f;
    }

    if (placer->delay != 0) {
        placer->delay--;
    } else {
        get_bone_world_pos(placer->player->slot.mirror_a, 10, &position);
        if (position.y <= g_game_info.field_34 + 0.2f) {
            place_crack = 1;
        } else {
            get_bone_world_pos(placer->player->slot.mirror_a, 11, &position);
            if (position.y <= g_game_info.field_34 + 0.2f) {
                place_crack = 1;
            }
        }
    }
    if (place_crack == 0) {
        get_bone_world_pos(placer->player->slot.mirror_a, 0, &position);
        if (position.y <= g_game_info.field_34 + 0.27f) {
            place_crack = 1;
        } else {
            get_bone_world_pos(placer->player->slot.mirror_a, 16, &position);
            if (position.y <= g_game_info.field_34 + 0.27f) {
                place_crack = 1;
            } else {
                get_bone_world_pos(placer->player->slot.mirror_a, 3, &position);
                if (position.y <= g_game_info.field_34 + 0.27f) {
                    place_crack = 1;
                }
            }
        }
    }
    if (place_crack == 1) {
        int found_unused = 0;

        crack_index = 0;
        while (crack_index < 10) {
            if (cracks[crack_index].active == 0) {
                found_unused = 1;
                break;
            }
            crack_index++;
        }
        g_game_info.crack_count += 1.0f;
        if (!found_unused) {
            crack_index = g_bgnd_last_crack_overwritten;
            g_bgnd_last_crack_overwritten++;
            if (g_bgnd_last_crack_overwritten >= 10) {
                g_bgnd_last_crack_overwritten = 0;
            }
        }
        cracks[crack_index].active = 1;
        cracks[crack_index].position.x = position.x;
        cracks[crack_index].position.y = position.y;
        cracks[crack_index].position.z = position.z;
        cracks[crack_index].position.y = g_game_info.field_34;
        MKMatrixTranslate(&crack_pool->pebbles[crack_index].matrix,
            &cracks[crack_index].position, 0);
        snd_req(0x11B);
        return -1.0f;
    }
    return 1.0f;
}
void bgnd_kill_fx(const char* name) {
    int effect;

    effect = fx_by_owner(name, 4);
    if (effect != 0) {
        fx_reset(effect);
    }
}
void bgnd_set_fx_z_offset(const char* name, float z_offset) {
    MkPfx* effect;

    effect = find_pfx_by_name(name);
    if (effect != 0) {
        effect->depth_bias = z_offset;
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
void bgnd_launch_fx_at_active_sobj_pos_with_offset(
    const char* name, float x_offset, float y_offset, float z_offset) {
    MkPfx* effect;

    reset_effect(name);
    effect = find_pfx_by_name(name);
    if (effect != 0) {
        g_latest_obj_pfx = (MkObj*)pfx_get_emitter_obj(effect, 0);
        if (g_latest_obj_pfx == 0) {
            g_latest_obj_pfx =
                pfx_bind_to_new_obj(effect, 0x8227);
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
        effect, 0x6015, emitter_id_from_handle(handle));
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
                    pfx_bind_to_new_obj(effect, 0x8227);
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
void bgnd_launch_fx_at_plyr_bid(const char* name, int bone) {
    unsigned int handle;
    MkObj* object;
    MkPfx* effect;

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
                        pfx_bind_to_new_obj(effect, 0x8227);
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
            pfx_bind_to_new_obj(effect, 0x8227);
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

    g_bgnd_preloaded_models[model_index] = load_named_model_from_slot(
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
void bgnd_init_timers(int create_monitor) {
    MkProc* process;
    int timer;

    for (timer = 0; timer < 3; timer++) {
        g_game_info.bgnd_timer_ticks[timer] = 0;
        g_game_info.bgnd_timer_limits[timer] = 0;
    }
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
static float p_bgnd_timer_monitor(void) {
    CmdScript* script;
    unsigned int timer;
    CmdScript* previous_script;

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
static inline void bgnd_apply_collision_info(BgndObstacleEventData* event) {
    MkObj* opponent_object;
    MkObj* player_object;
    PlyrPdata* opponent;
    PlyrPdata* player;
    const Vec* impact;

    player = event->player_pdata;
    player_object = bgnd_get_live_tracked_obj(player);
    if (player_object != 0) {
        opponent = player->his_plyr_pdata;
        opponent_object = bgnd_get_live_tracked_obj(opponent);
        if (opponent_object != 0) {
            g_game_info.collision_player_info = player->plyr_info;
            player = event->player_pdata;
            g_game_info.active_player = player->his_plyr_pdata->plyr_info;
            impact = event->impact_vector;
            g_game_info.impact_vector.x = impact->x;
            g_game_info.impact_vector.y = impact->y;
            g_game_info.impact_vector.z = impact->z;
            g_game_info.player_objects[0] = opponent_object;
            g_game_info.player_objects[1] = player_object;
            g_game_info.collision_player_side = event->flag_bits.player_side;
            g_game_info.collision_player_pdata = player;
            g_game_info.collision_event_id = event->event_id;
        }
    }
}

/* TODO: [near miss] 98.679245%; reload, latches and publication order agree;
 * player/global base registers differ; stop at coloring. */
void bgnd_collison_if_set_info(void) {
    bgnd_apply_collision_info(g_active_obstacle_event_data);
}
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
/* TODO: [near miss] 97.44%; only signedness left: BgndCollisionItem monitor_mode/collision_mode should be unsigned int (retail cmplwi). */
static int bgnd_collision_to_script_interface(BgndObstacleEventData* event) {
    unsigned int list_index;
    BgndCollisionItem* item;
    CmdScript* script;
    CmdScript* previous_script;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;

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
    if (event->player_pdata != 0 &&
        (event->player_pdata->plyr_info->slot.mirror_a == 0 ||
         event->player_pdata->his_obj == 0)) {
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
                if ((item->collision_mode != 0 || event->flag_bits.player_side) &&
                    !item->flags.bits.disabled &&
                    event->event_id == item->collision_id &&
                    (item->monitor_mode == 0 ||
                     (item->monitor_mode == 1 && event->player_pdata->state == 0x609) ||
                     (item->monitor_mode == 2 && (event->player_pdata->state & 0x400)) ||
                     (item->monitor_mode == 3 && (event->player_pdata->state & 0x1000))) &&
                    (!item->flags.bits.bit7 || event->player_pdata != 0)) {
                    unsigned int player_side;

                    script = alloc_cmdscript();
                    previous_script = active_cmdscript;
                    player_side = event->flag_bits.player_side;
                    if (player_side == 1 && item->flags.bits.bit6 == 1 &&
                        event->player_pdata->state != 0x609) {
                        return 0;
                    }
                    if (item->collision_mode != 0 &&
                        item->collision_mode != 3 && event->player_pdata != 0 &&
                        player_side == 1) {
                        event->player_pdata = event->player_pdata->his_plyr_pdata;
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
void bgnd_swap_level(int level) {
    CmdScript* script;
    CmdScript* previous_script;
    BgndWallHiderData* hider;
    float ground_plane;

    script = alloc_cmdscript();
    previous_script = active_cmdscript;
    active_cmdscript = script;
    if ((unsigned int)level <
        get_row_count_for_table_by_pointer(
            g_game_info.cmdscript, g_game_info.section->misc) - 1) {
        if (g_game_info.wall_hider != 0) {
            g_game_info.wall_hider->flag_bits.disabled = 1;
        }
        g_game_info.section = get_data_table(
            g_game_info.cmdscript, g_game_info.cmdscript->table_count);
        g_game_info.misc = g_game_info.section->misc + level;

        if (g_game_info.misc->lights_bgnd != 0) {
            load_back_in_lights(
                g_game_info.misc->lights_bgnd, &bgnd_light_list);
        } else {
            g_game_info.bgnd_obj->light_flags = 0;
            clear_all_lights_in(&bgnd_light_list);
        }
        if (g_game_info.misc->lights_spec != 0) {
            load_back_in_lights(
                g_game_info.misc->lights_spec, &bgnd_spec_light_list);
        }
        if (g_game_info.misc->lights_plyr != 0) {
            load_back_in_lights(
                g_game_info.misc->lights_plyr, &plyr_light_list);
        } else {
            clear_all_lights_in(&plyr_light_list);
        }
        if (g_game_info.misc->lights_bgnd != 0) {
            g_game_info.bgnd_obj->light_flags = 0x1009;
        } else {
            g_game_info.bgnd_obj->light_flags = 0;
        }
        if ((g_game_info.section->flags70 & 1) != 0) {
            UpdateShadowCameraLightSource(
                g_game_info.misc->shadow_cam_light);
        }
        ShadowStrength = g_game_info.misc->shadow_strength;
        ground_plane = g_game_info.misc->ground_plane;
        if (g_game_info.bgnd_id >= 0) {
            g_game_info.field_34 = ground_plane;
        }
        cloth_change_ground_plane_for(ground_plane);
        if (g_game_info.plyr0.slot.shadow_object != 0) {
            g_game_info.plyr0.slot.shadow_object->ground_colls_y = ground_plane;
            shadow_set_new_ground_plane(
                g_game_info.plyr0.slot.shadow,
                g_game_info.plyr0.slot.shadow_ground, ground_plane);
        }
        if (g_game_info.plyr1.slot.shadow_object != 0) {
            g_game_info.plyr1.slot.shadow_object->ground_colls_y = ground_plane;
            shadow_set_new_ground_plane(
                g_game_info.plyr1.slot.shadow,
                g_game_info.plyr1.slot.shadow_ground, ground_plane);
        }
        cam_set_ground_plane(ground_plane);
        if (g_game_info.misc->enter_script != 0) {
            cmdscript_setup_execution(
                g_game_info.cmdscript,
                g_game_info.misc->enter_script);
            cmdscript_execute(g_game_info.cmdscript);
        }

        hider = g_game_info.wall_hider;
        g_game_info.active_level = level;
        if (hider != 0) {
            hider->flag_bits.disabled = 1;
        }
        if (hider != 0) {
            destroy_list(&hider->walls);
        }
        if (g_game_info.misc->script != 0) {
            cmdscript_setup_execution(
                g_game_info.cmdscript,
                g_game_info.misc->script);
            cmdscript_execute(g_game_info.cmdscript);
        }
        if (hider != 0) {
            hider->flag_bits.disabled = 0;
        }
        if (g_game_info.wall_hider != 0) {
            g_game_info.wall_hider->flag_bits.disabled = 0;
        }
        active_cmdscript = previous_script;
        if (script->instance != 0) {
            ((int (*)(CmdScript*))script->vtbl->destroy)(script);
        }
    }
}
float bgnd_move_plyrs_to_initial_pos(void) {
    unsigned int player0_state =
        g_game_info.plyr0.slot.pdata->state_flags.raw_word;
    unsigned int player1_state =
        g_game_info.plyr1.slot.pdata->state_flags.raw_word;

    g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 1;
    g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 1;
    g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 1;
    if (g_game_info.plyr0.slot.mirror_a != 0) {
        Vec* player0_start = &g_game_info.misc->player0_start;
        Vec player0_angles = {0.0f, 1.5707964f, 0.0f};
        player0_start->y = g_game_info.field_34;
        move_player(g_game_info.plyr0.slot.mirror_a,
                    player0_start, &player0_angles);
    }
    if (g_game_info.plyr1.slot.mirror_a != 0) {
        Vec* player1_start = &g_game_info.misc->player1_start;
        Vec player1_angles = {0.0f, -1.5707964f, 0.0f};
        player1_start->y = g_game_info.field_34;
        move_player(g_game_info.plyr1.slot.mirror_a,
                    player1_start, &player1_angles);
    }
    force_midpoint_calculation_update = 1;
    _mkproc_sleep_ticks = 4.0f;
    aproc->vtbl->sleep();
    g_game_info.plyr0.slot.pdata->state_flags.raw_word = player0_state;
    g_game_info.plyr1.slot.pdata->state_flags.raw_word = player1_state;
    return 0.0f;
}
void bgnd_set_new_ground_plane(void* script, float ground_y) {
    (void)script;
    if (g_game_info.bgnd_id >= 0) {
        g_game_info.field_34 = ground_y;
    }
    cloth_change_ground_plane_for(ground_y);
    if (g_game_info.plyr0.slot.shadow_object != 0) {
        g_game_info.plyr0.slot.shadow_object->ground_colls_y = ground_y;
        shadow_set_new_ground_plane(
            g_game_info.plyr0.slot.shadow,
            g_game_info.plyr0.slot.shadow_ground, ground_y);
    }
    if (g_game_info.plyr1.slot.shadow_object != 0) {
        g_game_info.plyr1.slot.shadow_object->ground_colls_y = ground_y;
        shadow_set_new_ground_plane(
            g_game_info.plyr1.slot.shadow,
            g_game_info.plyr1.slot.shadow_ground, ground_y);
    }
    cam_set_ground_plane(ground_y);
}
void bgnd_set_player_shadow_ground_plane(int player, float ground_y) {
    if (player == 0 && g_game_info.plyr0.slot.shadow_object != 0) {
        g_game_info.plyr0.slot.shadow_object->ground_colls_y = ground_y;
        shadow_set_new_ground_plane(
            g_game_info.plyr0.slot.shadow,
            g_game_info.plyr0.slot.shadow_ground, ground_y);
    }
    if (player == 1 && g_game_info.plyr1.slot.shadow_object != 0) {
        g_game_info.plyr1.slot.shadow_object->ground_colls_y = ground_y;
        shadow_set_new_ground_plane(
            g_game_info.plyr1.slot.shadow,
            g_game_info.plyr1.slot.shadow_ground, ground_y);
    }
}
void bgnd_enable_wall_hider(unsigned int enabled) {
    if (enabled != 0) {
        if (g_game_info.wall_hider != 0) {
            g_game_info.wall_hider->flag_bits.disabled = 0;
        }
    } else if (g_game_info.wall_hider != 0) {
        g_game_info.wall_hider->flag_bits.disabled = 1;
    }
}
void bgnd_set_wall_hide_distance(void* script, float distance) {
    (void)script;
    g_game_info.wall_hider->hide_distance = distance;
}
void bgnd_add_fx_to_hide(const char* effect_name) {
    BgndWallHiderRuntime* runtime;
    int effect;

    if (g_game_info.wall_hider != 0) {
        runtime = g_game_info.wall_hider->runtime;
        if (runtime != 0 && runtime->hidden_effect_count < 4) {
            effect = fx_by_owner(effect_name, 4);
            runtime = g_game_info.wall_hider->runtime;
            runtime->hidden_effects[runtime->hidden_effect_count++] = effect;
        }
    }
}
void bgnd_add_wall_to_unhide(int object_id) {
    BgndWallHiderRuntime* runtime;
    MkSobj* object;

    if (g_game_info.wall_hider != 0) {
        runtime = g_game_info.wall_hider->runtime;
        if (runtime != 0 && runtime->unhide_count < 8) {
            obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
            if (g_game_info.bgnd_obj != 0) {
                object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
                if (object != 0) {
                    sobj_set_priority(object, 0x14);
                }
            }
            obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
            if (g_game_info.bgnd_obj != 0) {
                object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
                if (object != 0) {
                    hide_sobj(object);
                }
            }
            runtime = g_game_info.wall_hider->runtime;
            runtime->walls_to_unhide[runtime->unhide_count++] = object_id;
        }
    }
}
void bgnd_add_wall_to_hide(int object_id) {
    BgndWallHiderRuntime* runtime;
    MkSobj* object;

    if (g_game_info.wall_hider != 0) {
        runtime = g_game_info.wall_hider->runtime;
        if (runtime != 0 && runtime->hide_count < 8) {
            obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
            if (g_game_info.bgnd_obj != 0) {
                object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
                if (object != 0) {
                    unhide_sobj(object);
                }
            }
            runtime = g_game_info.wall_hider->runtime;
            runtime->walls_to_hide[runtime->hide_count++] = object_id;
        }
    }
}
void bgnd_add_new_normal_check_for_hider(
    void* script_args, float normal_x, float normal_y, float normal_z,
    float distance) {
    BgndWallHiderRuntime* runtime;

    (void)script_args;
    runtime = (BgndWallHiderRuntime*)get_mkhdr_generic(
        sizeof(BgndWallHiderRuntime));
    runtime->normal.z = 0.0f;
    runtime->normal.y = 0.0f;
    runtime->normal.x = 0.0f;
    runtime->normal_distance = 0.0f;
    runtime->normal_flags = 0;
    runtime->hide_count = 0;
    runtime->unhide_count = 0;
    runtime->hidden_effect_count = 0;
    if (runtime != 0) {
        runtime->normal.x = normal_x;
        runtime->normal.y = normal_y;
        runtime->normal.z = normal_z;
        runtime->normal_distance = distance;
        g_game_info.wall_hider->runtime = runtime;
        mk_insert(&runtime->hdr, &g_game_info.wall_hider->walls);
    }
}
void bgnd_start_wall_hider(int unused) {
    BgndWallHiderData* hider;
    MkProc* process;

    (void)unused;
    hider = 0;
    process = _create_mkproc_generic_tinystack(
        0x8107, 0x1F, p_hide_walls, sizeof(BgndWallHiderData),
        (MkHdr**)&hider);
    if (process != 0 && hider != 0) {
        hider->walls = 0;
        hider->hide_distance = 100.0f;
        hider->flags = 0;
        mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        g_game_info.wall_hider = hider;
    }
}
void bgnd_remove_wall_from_hider(unsigned int object_id) {
    BgndWallHiderRuntime* runtime;
    MkPtr* link;
    MkPtr* next;
    unsigned int index;
    BgndWallHiderData* hider = g_game_info.wall_hider;

    if (&hider->walls != 0) {
        link = hider->walls;
        while (link != 0) {
            runtime = (BgndWallHiderRuntime*)link->hdr;
            if (link->instance != runtime->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                index = 0;
                while (index < runtime->hide_count) {
                    if (runtime->walls_to_hide[index] == object_id) {
                        runtime->hide_count--;
                        if (runtime->hide_count != 0) {
                            runtime->walls_to_hide[index] =
                                runtime->walls_to_hide[runtime->hide_count];
                        }
                        break;
                    }
                    index++;
                }
                link = link->next;
            }
        }
    }
}

static float p_hide_walls(void) {
    BgndWallHiderData* hider;
    BgndWallHiderRuntime* runtime;
    CameraObj* camera;
    MkPtr* link;
    MkPtr* next;
    Vec forward = {0.0f, 0.0f, 1.0f};
    float camera_radius_squared;
    float hide_distance;
    unsigned int index;

    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);

    if (camera == 0) {
        return -1.0f;
    }

    hider = (BgndWallHiderData*)apdata;
    if (hider == 0) {
        return -1.0f;
    }
    if (g_game_info.bgnd_obj == 0 ||
        hider->flag_bits.kill_process == 1) {
        return -1.0f;
    }
    if (hider->flag_bits.disabled == 1) {
        return 1.0f;
    }

    camera_radius_squared =
        camera->pos.x * camera->pos.x +
        camera->pos.y * camera->pos.y +
        camera->pos.z * camera->pos.z;
    hide_distance = hider->hide_distance;
    rotate_xz(&forward, &forward, camera->ang.y);

    if (&hider->walls != 0) {
        link = hider->walls;
        while (link != 0) {
            runtime = (BgndWallHiderRuntime*)link->hdr;
            if (link->instance != runtime->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (camera_radius_squared >= hide_distance) {
                    if (runtime->normal.x * forward.x +
                            runtime->normal.y * forward.y +
                            runtime->normal.z * forward.z >=
                        runtime->normal_distance) {
                        if (runtime->normal_flags == 0) {
                            runtime->normal_flags = 1;
                            for (index = 0; index < runtime->hide_count;
                                 index++) {
                                hide_sobj(obj_find_sobj_by_id(
                                    g_game_info.bgnd_obj,
                                    runtime->walls_to_hide[index]));
                            }
                            for (index = 0; index < runtime->unhide_count;
                                 index++) {
                                unhide_sobj(obj_find_sobj_by_id(
                                    g_game_info.bgnd_obj,
                                    runtime->walls_to_unhide[index]));
                            }
                            for (index = 0;
                                 index < runtime->hidden_effect_count;
                                 index++) {
                                fx_hide(runtime->hidden_effects[index], 1);
                            }
                        }
                    } else if (runtime->normal_flags != 0) {
                        runtime->normal_flags = 0;
                        for (index = 0; index < runtime->hide_count; index++) {
                            unhide_sobj(obj_find_sobj_by_id(
                                g_game_info.bgnd_obj,
                                runtime->walls_to_hide[index]));
                        }
                        for (index = 0; index < runtime->unhide_count;
                             index++) {
                            hide_sobj(obj_find_sobj_by_id(
                                g_game_info.bgnd_obj,
                                runtime->walls_to_unhide[index]));
                        }
                        for (index = 0; index < runtime->hidden_effect_count;
                             index++) {
                            fx_hide(runtime->hidden_effects[index], 0);
                        }
                    }
                } else if (runtime->normal_flags != 0) {
                    runtime->normal_flags = 0;
                    for (index = 0; index < runtime->hide_count; index++) {
                        unhide_sobj(obj_find_sobj_by_id(
                            g_game_info.bgnd_obj,
                            runtime->walls_to_hide[index]));
                    }
                    for (index = 0; index < runtime->unhide_count; index++) {
                        hide_sobj(obj_find_sobj_by_id(
                            g_game_info.bgnd_obj,
                            runtime->walls_to_unhide[index]));
                    }
                    for (index = 0; index < runtime->hidden_effect_count;
                         index++) {
                        fx_hide(runtime->hidden_effects[index], 0);
                    }
                }
                link = link->next;
            }
        }
    }
    return 1.0f;
}
void bgnd_place_object_at_position(
    int object_id, int sobj_id, Vec* position, Vec* angles,
    int flags) {
    BgndDisplayedItem* item;
    CollisionShape shape;
    Vec center;

    item = (BgndDisplayedItem*)get_mkhdr_generic(sizeof(BgndDisplayedItem));
    item->type = 0;
    item->primary_object = 0;
    item->secondary_object = 0;
    item->display_sobj = 0;
    item->secondary_sobj = 0;
    item->object_id = 0;
    item->field_20 = 0;
    item->sobj_id = 0;
    item->field_28 = 0;
    item->source_sobj_id = 0;
    item->field_30 = 0;
    item->position.x = item->position.y = item->position.z = 0.0f;
    item->angles.x = item->angles.y = item->angles.z = 0.0f;
    item->field_4C.x = item->field_4C.y = item->field_4C.z = 0.0f;
    item->field_58.x = item->field_58.y = item->field_58.z = 0.0f;
    item->collision_center.x = item->collision_center.y =
        item->collision_center.z = 0.0f;
    item->collision_radius = 0.0f;
    item->collision_height = 0.0f;
    item->field_78 = 0.0f;
    item->flags = 0;
    item->placement_flags = 0;
    item->flag_0 = 0;
    item->placement_id = g_game_info.field_78++;

    if (item != 0) {
        center.x = position->x;
        center.z = position->z;
        center.y = g_game_info.field_34;
        item->type = 2;
        item->primary_object = 0;
        item->object_id = object_id;
        item->display_sobj =
            obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
        item->primary_object = 0;
        item->display_sobj->flags_08_bits.bit6 = 1;
        item->position.x = position->x;
        item->position.y = position->y;
        item->position.z = position->z;
        item->display_sobj->pos.x = position->x;
        item->display_sobj->pos.y = position->y;
        item->display_sobj->pos.z = position->z;
        item->display_sobj->flags_08_bits.bit3 = 1;
        item->angles.x = angles->x;
        item->angles.y = angles->y;
        item->angles.z = angles->z;
        item->display_sobj->ang.x = angles->x;
        item->display_sobj->ang.y = angles->y;
        item->display_sobj->ang.z = angles->z;
        update_mksobj(item->display_sobj);
        item->source_sobj_id = sobj_id;
        item->sobj_id = 0;
        item->collision_radius = 0.4f;
        item->collision_height = 3.0f;
        item->flags = flags;
        item->collision_center.x = center.x;
        item->collision_center.y = center.y;
        item->collision_center.z = center.z;
        build_col_shape_vertical_cylinder(&shape, &center, 0.4f, 3.0f);
        item->obstacle = add_shape_to_background_obstacle_list(
            &shape, object_id + 0x100);

        if (item != 0) {
            if (item->obstacle != 0 &&
                !g_game_info.feature_flags.bits.high_bit) {
                item->obstacle->flags.bits.disabled = 0;
            }
            mk_insert(&item->hdr, &g_game_info.field_64);
            if (item->primary_object != 0) {
                insert_fgnd_mkobj(item->primary_object);
                update_mkobj(item->primary_object);
                hide_obj(item->primary_object);
            }
            if (item->secondary_object != 0) {
                insert_fgnd_mkobj(item->secondary_object);
                update_mkobj(item->secondary_object);
                hide_obj(item->secondary_object);
            }
            if (item->display_sobj != 0) {
                update_mksobj(item->display_sobj);
                unhide_sobj(item->display_sobj);
            }
            if (item->secondary_sobj != 0) {
                update_mksobj(item->secondary_sobj);
                unhide_sobj(item->secondary_sobj);
            }
        }
    }
}

void bgnd_place_weapon_at_position(
    int primary_object_id, int secondary_object_id, int primary_sobj_id,
    int secondary_sobj_id, int paired, int pickup_sobj_id, int permanent,
    float primary_x, float primary_y, float primary_z,
    float primary_angle_x, float primary_angle_y, float primary_angle_z,
    float secondary_x, float secondary_y, float secondary_z,
    float secondary_angle_x, float secondary_angle_y,
    float secondary_angle_z, float radius, float height,
    float collision_x, float collision_y, float collision_z) {
    BgndDisplayedItem* item;
    CollisionShape shape;
    Vec center;

    item = (BgndDisplayedItem*)get_mkhdr_generic(sizeof(BgndDisplayedItem));
    item->type = 0;
    item->primary_object = 0;
    item->secondary_object = 0;
    item->display_sobj = 0;
    item->secondary_sobj = 0;
    item->object_id = 0;
    item->field_20 = 0;
    item->sobj_id = 0;
    item->field_28 = 0;
    item->source_sobj_id = 0;
    item->field_30 = 0;
    item->position.x = item->position.y = item->position.z = 0.0f;
    item->angles.x = item->angles.y = item->angles.z = 0.0f;
    item->field_4C.x = item->field_4C.y = item->field_4C.z = 0.0f;
    item->field_58.x = item->field_58.y = item->field_58.z = 0.0f;
    item->collision_center.x = item->collision_center.y =
        item->collision_center.z = 0.0f;
    item->collision_radius = 0.0f;
    item->collision_height = 0.0f;
    item->field_78 = 0.0f;
    item->flags = 0;
    item->placement_flags = 0;
    item->flag_0 = 0;
    item->placement_id = g_game_info.field_78++;

    if (item != 0) {
        if (paired == 0) {
            item->type = 0;
            item->primary_object =
                MK_HDR_LIVE(global_movesets[6].primary_weapon, global_movesets[6].primary_weapon_instance);
            item->object_id = primary_object_id;
            item->display_sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, primary_sobj_id);
            item->primary_object->flags_08_bits.airborne = 1;
            item->display_sobj->flags_08_bits.bit6 = 1;
            item->primary_object->pos.value.x = primary_x;
            item->primary_object->pos.value.y = primary_y;
            item->primary_object->pos.value.z = primary_z;
            item->position.x = primary_x;
            item->position.y = primary_y;
            item->position.z = primary_z;
            item->display_sobj->pos.x = primary_x;
            item->display_sobj->pos.y = primary_y;
            item->display_sobj->pos.z = primary_z;
            item->primary_object->flags_08_bits.angular_velocity_enabled = 1;
            item->display_sobj->flags_08_bits.bit3 = 1;
            item->primary_object->ang.x = primary_angle_x;
            item->primary_object->ang.y = primary_angle_y;
            item->primary_object->ang.z = primary_angle_z;
            item->angles.x = primary_angle_x;
            item->angles.y = primary_angle_y;
            item->angles.z = primary_angle_z;
            item->display_sobj->ang.x = primary_angle_x;
            item->display_sobj->ang.y = primary_angle_y;
            item->display_sobj->ang.z = primary_angle_z;
            update_mkobj(item->primary_object);
            update_mksobj(item->display_sobj);
            item->source_sobj_id = primary_sobj_id;
        } else {
            item->type = 1;
            item->primary_object =
                MK_HDR_LIVE(global_movesets[6].primary_weapon, global_movesets[6].primary_weapon_instance);
            item->primary_object =
                MK_HDR_LIVE(global_movesets[6].secondary_weapon, global_movesets[6].secondary_weapon_instance);
            item->object_id = primary_object_id;
            item->field_20 = secondary_object_id;
            item->display_sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, primary_sobj_id);
            item->secondary_sobj = obj_find_sobj_by_id(
                g_game_info.bgnd_obj, secondary_sobj_id);
            item->primary_object->flags_08_bits.airborne = 1;
            item->display_sobj->flags_08_bits.bit6 = 1;
            item->primary_object->pos.value.x = primary_x;
            item->primary_object->pos.value.y = primary_y;
            item->primary_object->pos.value.z = primary_z;
            item->position.x = primary_x;
            item->position.y = primary_y;
            item->position.z = primary_z;
            item->display_sobj->pos.x = primary_x;
            item->display_sobj->pos.y = primary_y;
            item->display_sobj->pos.z = primary_z;
            item->primary_object->flags_08_bits.angular_velocity_enabled = 1;
            item->display_sobj->flags_08_bits.bit3 = 1;
            item->primary_object->ang.x = primary_angle_x;
            item->primary_object->ang.y = primary_angle_y;
            item->primary_object->ang.z = primary_angle_z;
            item->angles.x = primary_angle_x;
            item->angles.y = primary_angle_y;
            item->angles.z = primary_angle_z;
            item->display_sobj->ang.x = primary_angle_x;
            item->display_sobj->ang.y = primary_angle_y;
            item->display_sobj->ang.z = primary_angle_z;
            item->secondary_object->flags_08_bits.airborne = 1;
            item->secondary_sobj->flags_08_bits.bit6 = 1;
            item->secondary_object->pos.value.x = secondary_x;
            item->secondary_object->pos.value.y = secondary_y;
            item->secondary_object->pos.value.z = secondary_z;
            item->field_4C.x = secondary_x;
            item->field_4C.y = secondary_y;
            item->field_4C.z = secondary_z;
            item->secondary_sobj->pos.x = secondary_x;
            item->secondary_sobj->pos.y = secondary_y;
            item->secondary_sobj->pos.z = secondary_z;
            item->secondary_object->flags_08_bits.angular_velocity_enabled = 1;
            item->secondary_sobj->flags_08_bits.bit3 = 1;
            item->secondary_object->ang.x = secondary_angle_x;
            item->secondary_object->ang.y = secondary_angle_y;
            item->secondary_object->ang.z = secondary_angle_z;
            item->field_58.x = secondary_angle_x;
            item->field_58.y = secondary_angle_y;
            item->field_58.z = secondary_angle_z;
            item->secondary_sobj->ang.x = secondary_angle_x;
            item->secondary_sobj->ang.y = secondary_angle_y;
            item->secondary_sobj->ang.z = secondary_angle_z;
            update_mkobj(item->primary_object);
            update_mksobj(item->display_sobj);
            update_mkobj(item->secondary_object);
            update_mksobj(item->secondary_sobj);
            item->source_sobj_id = primary_sobj_id;
            item->field_30 = primary_sobj_id;
        }

        item->sobj_id = pickup_sobj_id;
        item->collision_radius = radius;
        item->collision_height = height;
        center.x = collision_x;
        center.y = collision_y;
        center.z = collision_z;
        item->collision_center.x = center.x;
        item->collision_center.y = center.y;
        item->collision_center.z = center.z;
        build_col_shape_vertical_cylinder(&shape, &center, radius, height);
        item->obstacle = add_shape_to_background_obstacle_list(
            &shape, primary_object_id + 0x100);

        if (permanent != 0) {
            if (item != 0) {
                if (item->obstacle != 0 &&
                    !g_game_info.feature_flags.bits.high_bit) {
                    item->obstacle->flags.bits.disabled = 0;
                }
                mk_insert(&item->hdr, &g_game_info.field_64);
                if (item->primary_object != 0) {
                    insert_fgnd_mkobj(item->primary_object);
                    update_mkobj(item->primary_object);
                    hide_obj(item->primary_object);
                }
                if (item->secondary_object != 0) {
                    insert_fgnd_mkobj(item->secondary_object);
                    update_mkobj(item->secondary_object);
                    hide_obj(item->secondary_object);
                }
                if (item->display_sobj != 0) {
                    update_mksobj(item->display_sobj);
                    unhide_sobj(item->display_sobj);
                }
                if (item->secondary_sobj != 0) {
                    update_mksobj(item->secondary_sobj);
                    unhide_sobj(item->secondary_sobj);
                }
            }
        } else {
            if (item->obstacle != 0 &&
                !g_game_info.feature_flags.bits.high_bit) {
                item->obstacle->flags.bits.disabled = 1;
            }
            mk_insert(&item->hdr, &g_game_info.displayed_items);
            if (item->primary_object != 0) {
                insert_fgnd_mkobj(item->primary_object);
                update_mkobj(item->primary_object);
                hide_obj(item->primary_object);
            }
            if (item->secondary_object != 0) {
                insert_fgnd_mkobj(item->secondary_object);
                update_mkobj(item->secondary_object);
                hide_obj(item->secondary_object);
            }
            if (item->display_sobj != 0) {
                update_mksobj(item->display_sobj);
                unhide_sobj(item->display_sobj);
            }
            if (item->secondary_sobj != 0) {
                update_mksobj(item->secondary_sobj);
                unhide_sobj(item->secondary_sobj);
            }
        }
    }
}
BgndDisplayedItem* bgnd_get_item_from_displayed_list(int item_id) {
    BgndDisplayedItem* item;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;

    list = &g_game_info.displayed_items;
    if (list != 0) {
        link = *list;
        while (link != 0) {
            item = (BgndDisplayedItem*)link->hdr;
            if (link->instance != item->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (item->type == item_id) {
                    return item;
                }
                link = link->next;
            }
        }
    }
    return 0;
}
void disable_bgnd_obj_repel(BgndDisplayedItem* item) {
    ArenaObstacle* obstacle;

    obstacle = item->obstacle;
    if (obstacle != 0 && !g_game_info.feature_flags.bits.high_bit) {
        obstacle->flags.bits.disabled = 1;
    }
}
void enable_bgnd_obj_repel(BgndDisplayedItem* item) {
    ArenaObstacle* obstacle;

    obstacle = item->obstacle;
    if (obstacle != 0 && !g_game_info.feature_flags.bits.high_bit) {
        obstacle->flags.bits.disabled = 0;
    }
}
int bgnd_get_exec_tick_ctr(void) {
    return exec_tick_ctr;
}
void bgnd_act_at_time(int ticks, int script_function, void* script,
                      float x, float y, float z) {
    BgndActAtTimeData* data;
    MkProc* process;

    (void)script;
    data = 0;
    process = _create_mkproc_generic_tinystack(
        0xC013, 0x2E, p_act_at_time, sizeof(BgndActAtTimeData),
        (MkHdr**)&data);
    if (process != 0) {
        data->ticks = ticks;
        data->script_function = script_function;
        data->parameters.x = x;
        data->parameters.y = y;
        data->parameters.z = z;
        data->ground_plane = g_game_info.field_34;
        mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
    }
}

static float p_act_at_time(void) {
    BgndActAtTimeData* data;
    float y;
    float z;
    int ticks;

    data = (BgndActAtTimeData*)pdata_of_proc(aproc);
    if (data->ground_plane != g_game_info.field_34) {
        return -1.0f;
    }
    ticks = data->ticks - 1;
    data->ticks = ticks;
    if (ticks != 0) {
        return 1.0f;
    }
    z = data->parameters.z;
    y = data->parameters.y;
    g_bgnd_scratch_pad_vectors[0].x = data->parameters.x;
    g_bgnd_scratch_pad_vectors[0].y = y;
    g_bgnd_scratch_pad_vectors[0].z = z;
    active_cmdscript = &global_script_interpreter;
    cmdscript_setup_execution(g_game_info.cmdscript, data->script_function);
    cmdscript_execute(g_game_info.cmdscript);
    return -1.0f;
}
void bgnd_run_camera_script(int argument, int flags) {
    camera_set_attacker(g_game_info.player_objects[1]);
    camera_set_victim(g_game_info.player_objects[0]);
    run_camera_script(g_game_info.cmdscript, argument, flags);
}
void* ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void) {
    return g_active_obstacle_event_data->player_pdata;
}
void spad_set_y_angle_plus_offset_from_xz_vector(int index, void* script,
                                                 float x, float z,
                                                 float offset) {
    Vec* output;
    Vec direction;
    Vec angles;

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
void spad_norm_vector(int index) {
    Vec* vector;
    float inverse_length;
    float x;

    vector = &g_bgnd_scratch_pad_vectors[index];
    x = vector->x;
    inverse_length = bgnd_inv_sqrt(
        (float)(vector->z * vector->z) +
        ((float)(x * x) + (float)(vector->y * vector->y)));
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
void spad_set_vector(int index, unsigned int source) {
    Vec* output = &g_bgnd_scratch_pad_vectors[index];

    switch (source) {
    case 1:
        output->x = g_game_info.player_objects[1]->pos.value.x;
        output->y = g_game_info.player_objects[1]->pos.value.y;
        output->z = g_game_info.player_objects[1]->pos.value.z;
        output->y = 0.0f;
        break;
    case 2:
        output->x = g_active_sobj->pos.x;
        output->y = g_active_sobj->pos.y;
        output->z = g_active_sobj->pos.z;
        break;
    case 0x17:
        output->x = g_active_sobj->ang.x;
        output->y = g_active_sobj->ang.y;
        output->z = g_active_sobj->ang.z;
        break;
    case 0xC:
        output->x = plyr_obj->pos.value.x;
        output->y = plyr_obj->pos.value.y;
        output->z = plyr_obj->pos.value.z;
        break;
    case 0xB:
        output->x = g_active_sobj->pos.x - plyr_obj->pos.value.x;
        output->z = g_active_sobj->pos.z - plyr_obj->pos.value.z;
        break;
    case 0x10:
        output->x = g_game_info.player_objects[0]->pos.value.x;
        output->y = g_game_info.player_objects[0]->pos.value.y;
        output->z = g_game_info.player_objects[0]->pos.value.z;
        break;
    case 0x31:
        output->x = g_game_info.player_objects[1]->pos.value.x;
        output->y = g_game_info.player_objects[1]->pos.value.y;
        output->z = g_game_info.player_objects[1]->pos.value.z;
        break;
    case 0x11:
        output->x = g_game_info.player_objects[0]->ang.x;
        output->y = g_game_info.player_objects[0]->ang.y;
        output->z = g_game_info.player_objects[0]->ang.z;
        break;
    case 0x36:
        output->x = g_game_info.player_objects[1]->ang.x;
        output->y = g_game_info.player_objects[1]->ang.y;
        output->z = g_game_info.player_objects[1]->ang.z;
        break;
    case 0x12:
        output->x = g_current_reaction_info.player_info->slot.mirror_a->pos.value.x;
        output->y = g_current_reaction_info.player_info->slot.mirror_a->pos.value.y;
        output->z = g_current_reaction_info.player_info->slot.mirror_a->pos.value.z;
        break;
    case 0x13:
        output->x = g_current_reaction_info.player_info->slot.pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.x;
        output->y = g_current_reaction_info.player_info->slot.pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.y;
        output->z = g_current_reaction_info.player_info->slot.pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.z;
        break;
    case 0x15:
        output->x = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->pos.value.x;
        output->y = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->pos.value.y;
        output->z = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->pos.value.z;
        break;
    case 0x14:
        output->x = g_active_obstacle_event_data->player_pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.x;
        output->y = g_active_obstacle_event_data->player_pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.y;
        output->z = g_active_obstacle_event_data->player_pdata->his_plyr_pdata->plyr_info->slot.mirror_a->pos.value.z;
        break;
    case 0x1E:
        output->x = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->ang.x;
        output->y = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->ang.y;
        output->z = g_active_obstacle_event_data->player_pdata->plyr_info->slot.mirror_a->ang.z;
        break;
    case 0x1A:
        output->x = (float)g_active_obstacle_event_data->player_pdata->plyr_num;
        break;
    case 0x1B:
        output->x = (float)g_active_obstacle_event_data->player_pdata->attack_counter;
        break;
    case 0x1C:
        output->x = (float)g_active_obstacle_event_data->player_pdata->state;
        break;
    case 0x19:
        output->x = g_active_obstacle_event_data->impact_vector->x;
        output->y = g_active_obstacle_event_data->impact_vector->y;
        output->z = g_active_obstacle_event_data->impact_vector->z;
        break;
    case 0x1D:
        output->x = (float)g_active_obstacle_event_data->flags;
        break;
    }
}
float spad_xz_length_vector(int index) {
    Vec* vector;

    vector = &g_bgnd_scratch_pad_vectors[index];
    return gxMathFastSqrt(vector->x * vector->x + vector->z * vector->z);
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
void spad_sub_vectors(int destination, int first, int second) {
    Vec* output = &g_bgnd_scratch_pad_vectors[destination];
    Vec* a = &g_bgnd_scratch_pad_vectors[first];
    Vec* b = &g_bgnd_scratch_pad_vectors[second];

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
void spad_set_vector_y(int index, void* script, float y) {
    Vec* vector;

    (void)script;
    vector = &g_bgnd_scratch_pad_vectors[index];
    vector->y = y;
}
static inline float bgnd_fast_sqrt(float value) {
    unsigned int bits;
    float guess;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    bits = (unsigned int)GXMathSqrtTable[(*(unsigned int*)&value >> 11) & 0x1FFF]
           << 8;
    bits |= (((*(unsigned int*)&value & 0x7F800000U) + 0x3F800000U) >> 1) &
            0x7F800000U;
    guess = *(float*)&bits;
    correction = 3.0f - (guess * guess) / value;
    return 0.5f * (guess * correction);
}

/* TODO: [near miss] 98.48%; only FPR numbering differs inside the two inlined bgnd_fast_sqrt copies. */
float spad_xz_cos_two_vectors(int first, int second) {
    Vec* first_vector;
    Vec* second_vector;
    float first_length;
    float second_length;

    first_vector = &g_bgnd_scratch_pad_vectors[first];
    second_vector = &g_bgnd_scratch_pad_vectors[second];
    first_length = bgnd_fast_sqrt(first_vector->x * first_vector->x +
                                  first_vector->z * first_vector->z);
    second_length = bgnd_fast_sqrt(second_vector->x * second_vector->x +
                                   second_vector->z * second_vector->z);
    return (first_vector->x * second_vector->x +
            first_vector->z * second_vector->z) /
           (first_length * second_length);
}
void spad_set_heading_vector_to(int index, void* script, float heading,
                                float offset) {
    Vec* vector;
    float angle;
    int normalized;

    (void)script;
    vector = &g_bgnd_scratch_pad_vectors[index];
    heading += offset;
    normalized = (int)(166886.1f * heading) & 0xFFFFF;
    angle = 0.000005992112f * (float)normalized;
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
void bgnd_xfer_attacker(int script_function) {
    MkProc* process;
    CmdScript* script;

    process = get_player_proc(g_game_info.player_objects[1]);
    script = get_cmdscript_for_proc(process);
    run_reaction_cleanup_function(g_game_info.collision_player_info->slot.pdata);
    script->unk28 = script_function;
    xfer_player_proc(process, bgnd_call_script_function);
}






/* TODO: [near miss] 99.62%; rsqrt stack-slot order (retail groups inputs
 * above estimates), case 0xD cross-product and case 0x16/0x3A GPR coloring remain. */
float bgnd_process_collision_info(
    unsigned int operation, float value1, float value2, float value3,
    float value4, float value5, float value6, float value7, float value8) {
    Vec up = {0.0f, 1.0f, 0.0f};
    float result = 0.0f;

    switch (operation) {
    case 0xA:
        return value1 * (g_game_info.player_objects[0]->pos.value.y -
                         g_game_info.player_objects[1]->pos.value.y) +
            g_game_info.player_objects[1]->pos.value.y;
    case 0xE:
        return g_game_info.player_objects[0]->pos.value.y;
    case 0x18:
        return g_game_info.player_objects[0]->pos.value.x;
    case 0x20:
        return g_game_info.player_objects[0]->pos.value.z;
    case 0x32:
        return g_game_info.player_objects[1]->pos.value.x;
    case 0xF:
        return g_game_info.player_objects[1]->pos.value.y;
    case 0x38:
        return g_game_info.player_objects[1]->pos.value.z;

    case 0:
        sh_normalize_blood_direction(&g_game_info.impact_vector);
        g_game_info.impact_vector.x *= value1;
        g_game_info.impact_vector.z *= value1;
        g_game_info.impact_vector.y = 0.0f;
        break;

    case 3: {
        Vec* output = &g_bgnd_scratch_pad_vectors[(unsigned int)value3];
        MkSobj* object = obj_find_sobj_by_id(
            g_game_info.bgnd_obj, (unsigned int)value2);
        Vec scaled;

        scaled.x = g_game_info.impact_vector.x;
        scaled.y = g_game_info.impact_vector.y;
        scaled.z = g_game_info.impact_vector.z;
        scaled.x *= value1;
        scaled.y *= value1;
        scaled.z *= value1;
        output->x = scaled.y * up.z - scaled.z * up.y;
        output->y = scaled.z * up.x - scaled.x * up.z;
        output->z = scaled.x * up.y - scaled.y * up.x;
        rotate_xz(output, output, object->ang.y);
        break;
    }
    case 4:
        if (g_game_info.collision_player_side != 0) {
            special_move_cam_setup2(
                value1, value2, value3, value4, value5, (int)value6,
                (int)value7, (int)value8, g_game_info.player_objects[1],
                g_game_info.player_objects[0]);
        }
        break;
    case 0x3C:
        if (g_active_obstacle_event_data->flag_bits.player_side) {
            special_move_cam_setup2(
                value1, value2, value3, value4, value5, (int)value6,
                (int)value7, (int)value8, g_game_info.player_objects[1],
                g_game_info.player_objects[0]);
        }
        break;
    case 0x28:
        special_move_cam_setup2(
            value1, value2, value3, value4, value5, (int)value6, (int)value7,
            (int)value8, g_game_info.player_objects[1],
            g_game_info.player_objects[0]);
        break;

    case 5: {
        PlyrPdata* player = g_game_info.collision_player_pdata;

        player->online_sync_index = -1;
        if (player->plyr_num == 0) {
            g_game_info.plyr0.fighting_lights.green_trigger = 0;
        } else {
            g_game_info.plyr1.fighting_lights.green_trigger = 0;
        }
        break;
    }

    case 7: {
        PlyrPdata* player = g_game_info.collision_player_pdata;
        PlyrInfo* info = player->plyr_info;

        info->slot.pdata->state_flags.bits.bit3 = 1;
        info->slot.mirror_a->flags_09_bits.launched = 0;
        info->slot.mirror_a->flags_09_bits.bit6 = 0;
        info->slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
        info->slot.mirror_a->flags_09_bits.head_tracking = 0;
        info->slot.mirror_a->flags_0B_bits.bit6 = 1;
        info->slot.mirror_a->flags_0B_bits.bit3 = 1;
        info->slot.mirror_a->flags_09_bits.face_opponent = 0;
        result = (float)g_game_info.collision_player_pdata->plyr_num;
        break;
    }
    case 8: {
        PlyrPdata* player = g_game_info.collision_player_pdata->his_plyr_pdata;
        PlyrInfo* info = player->plyr_info;

        info->slot.pdata->state_flags.bits.bit3 = 1;
        info->slot.mirror_a->flags_09_bits.launched = 0;
        info->slot.mirror_a->flags_09_bits.bit6 = 0;
        info->slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
        info->slot.mirror_a->flags_09_bits.head_tracking = 0;
        info->slot.mirror_a->flags_0B_bits.bit6 = 1;
        info->slot.mirror_a->flags_0B_bits.bit3 = 1;
        info->slot.mirror_a->flags_09_bits.face_opponent = 0;
        result = (float)g_game_info.collision_player_pdata->his_plyr_pdata->
            plyr_num;
        break;
    }
    case 0x21:
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.pdata->state_flags.bits.bit3 = 0;
        break;
    case 0x22:
        g_game_info.collision_player_pdata->plyr_info->slot.pdata->
            state_flags.bits.bit3 = 0;
        break;

    case 9: {
        MkProc* process = get_player_proc(g_game_info.player_objects[1]);
        CmdScript* script = get_cmdscript_for_proc(process);

        run_reaction_cleanup_function(
            g_game_info.collision_player_info->slot.pdata);
        script->unk28 = 0x8E;
        xfer_player_proc(process, r_call_script_function);
        break;
    }

    case 0xD: {
        CameraObj* camera = camera_item.node;
        Vec target;

        camera = MK_HDR_LIVE(camera, camera_item.instance);
        get_current_target(&target);
        target.x -= camera->pos.x;
        target.z -= camera->pos.z;
        sh_normalize_xz(&target);
        if ((plyr_obj->pos.value.x - camera->pos.x) * target.z +
                (plyr_obj->pos.value.z - camera->pos.z) * -target.x > 0.0f) {
            result = 1.0f;
        }
        break;
    }

    case 0x16: {
        MkObj* first = g_active_obstacle_event_data->player_pdata->
            plyr_info->slot.mirror_a;
        MkObj* second = g_active_obstacle_event_data->player_pdata->
            his_plyr_pdata->plyr_info->slot.mirror_a;
        Vec direction;

        direction.x = second->pos.value.x - first->pos.value.x;
        direction.y = second->pos.value.y - first->pos.value.y;
        direction.z = second->pos.value.z - first->pos.value.z;
        sh_normalize_fatality_vector(&direction);
        result = value3 * direction.z +
            (value1 * direction.x + value2 * direction.y);
        break;
    }
    case 0x3A: {
        MkObj* first = g_active_obstacle_event_data->player_pdata->
            plyr_info->slot.mirror_a;
        MkObj* second = g_active_obstacle_event_data->player_pdata->
            his_plyr_pdata->plyr_info->slot.mirror_a;
        Vec direction = {0.0f, 0.0f, 0.0f};

        direction.x = second->pos.value.x - first->pos.value.x;
        direction.z = second->pos.value.z - first->pos.value.z;
        result = value3 * direction.z +
            (value1 * direction.x + value2 * direction.y);
        break;
    }

    case 0x1F:
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->pos_vel.x = value1;
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->pos_vel.z = value2;
        break;
    case 0x3B:
        return g_active_obstacle_event_data->player_pdata->plyr_info->
            slot.mirror_a->pos.value.z;
    case 0x30:
        result = (float)g_active_obstacle_event_data->event_id;
        break;
    case 0x27:
        result = (float)g_game_info.collision_event_id;
        break;
    case 0x29:
        g_game_info.player_objects[0]->pos.value.y = value1;
        break;
    case 0x39:
        g_game_info.player_objects[0]->pos.value.z = value1;
        break;
    case 0x35:
        g_game_info.player_objects[1]->pos.value.y = value1;
        break;
    case 0x24:
        g_game_info.player_objects[0]->pos.value.x = value1;
        g_game_info.player_objects[0]->pos.value.y = value2;
        g_game_info.player_objects[0]->pos.value.z = value3;
        break;
    case 0x25:
        g_game_info.player_objects[0]->ang.x = value1;
        g_game_info.player_objects[0]->ang.y = value2;
        g_game_info.player_objects[0]->ang.z = value3;
        break;
    case 0x26:
        g_game_info.player_objects[0]->pos_vel.x = 0.0f;
        g_game_info.player_objects[0]->pos_vel.y = 0.0f;
        g_game_info.player_objects[0]->pos_vel.z = 0.0f;
        g_game_info.player_objects[0]->ang_vel.x = 0.0f;
        g_game_info.player_objects[0]->ang_vel.y = 0.0f;
        g_game_info.player_objects[0]->ang_vel.z = 0.0f;
        break;
    case 0x37:
        g_game_info.player_objects[1]->pos_vel.x = 0.0f;
        g_game_info.player_objects[1]->pos_vel.y = 0.0f;
        g_game_info.player_objects[1]->pos_vel.z = 0.0f;
        g_game_info.player_objects[1]->ang_vel.x = 0.0f;
        g_game_info.player_objects[1]->ang_vel.y = 0.0f;
        g_game_info.player_objects[1]->ang_vel.z = 0.0f;
        break;
    case 0x23:
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->ang.y = value1;
        break;
    case 0x2A:
        g_game_info.player_objects[1]->ang.y = value1;
        break;
    case 0x2B:
        g_game_info.player_objects[1]->pos.value.x = value1;
        g_game_info.player_objects[1]->pos.value.y = value2;
        g_game_info.player_objects[1]->pos.value.z = value3;
        break;
    case 0x2F:
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->pos_vel.y = value1;
        break;
    case 0x2C:
        return g_game_info.player_objects[0]->pos_vel.x;
    case 0x2D:
        return g_game_info.player_objects[0]->pos_vel.y;
    case 0x34:
        g_game_info.player_objects[0]->pos_vel.y = value1;
        break;
    case 0x2E:
        return g_game_info.player_objects[0]->pos_vel.z;
    case 0x33:
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->ang.x += value1;
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->ang.y += value2;
        g_game_info.collision_player_pdata->his_plyr_pdata->plyr_info->
            slot.mirror_a->ang.z += value3;
        return 0.0f;
    }
    return result;
}
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player) {
    CmdScript* script;
    MkProc* process;

    process = MK_LIVE(player->own_player_proc, player->own_player_proc_instance);
    script = get_cmdscript_for_proc(process);
    if (process != 0) {
        run_reaction_cleanup_function(player);
        script->unk28 = 0x8E;
        xfer_player_proc(process, r_call_script_function);
    }
}
void dont_fence_plyr_in(int disabled) {
    plyr_obj->flags_0B_bits.bit6 = disabled;
}
void bgnd_takeover_plyr(PlyrInfo* player) {
    player->slot.pdata->state_flags.bits.bit3 = 1;
    player->slot.mirror_a->flags_09_bits.launched = 0;
    player->slot.mirror_a->flags_09_bits.bit6 = 0;
    player->slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    player->slot.mirror_a->flags_09_bits.head_tracking = 0;
    player->slot.mirror_a->flags_0B_bits.bit6 = 1;
    player->slot.mirror_a->flags_0B_bits.bit3 = 1;
    player->slot.mirror_a->flags_09_bits.face_opponent = 0;
}
float bgnd_process_active_sobj_info(
    int info_id, void* script_args, float angle_component, float offset) {
    (void)script_args;
    switch (info_id) {
    case 6:
        g_active_sobj->flags_08_bits.bit3 = 1;
        g_active_sobj->ang.x = g_active_sobj->ang.y =
            g_active_sobj->ang.z = 0.0f;
        g_active_sobj->ang.y = offset + gxMathArcCos(angle_component);
        break;
    default:
        break;
    }
    return 0.0f;
}
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
#pragma dont_inline on
/* TODO: [near miss] 99.60674%; item ID and displayed-list head register coloring remains; check supported lifetimes. */
void bgnd_make_displayed_item_pickupable_at_active_sobj_pos(int item_id) {
    BgndDisplayedItem* item;
    MkPtr** displayed_list;
    MkPtr* link;
    Vec* active_position;

    active_position = &g_active_sobj->pos;
    item = bgnd_get_item_from_displayed_list(item_id);
    if (item != 0 && active_position != 0 && item->display_sobj != 0) {
        item->position.x = active_position->x;
        item->position.y = active_position->y;
        item->position.z = active_position->z;
        item->display_sobj->pos.x = item->position.x;
        item->display_sobj->pos.y = item->position.y;
        item->display_sobj->pos.z = item->position.z;
        update_mksobj(item->display_sobj);
    }

    displayed_list = &g_game_info.displayed_items;
    if (displayed_list != 0) {
        link = *displayed_list;
        while (link != 0) {
            BgndDisplayedItem* list_item;
            list_item = (BgndDisplayedItem*)link->hdr;
            if (link->instance != list_item->hdr.instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else if (list_item->type == item_id) {
                mk_pull_discard(&list_item->hdr, displayed_list);
                if (list_item->obstacle != 0 &&
                        !g_game_info.feature_flags.bits.high_bit) {
                    list_item->obstacle->flags.bits.disabled = 0;
                }
                mk_insert(&list_item->hdr, &g_game_info.field_64);
                if (list_item->sobj_id != 0) {
                    unhide_sobj(obj_create_sobjs_by_id(
                        g_game_info.bgnd_obj, list_item->sobj_id));
                }
                break;
            } else {
                link = link->next;
            }
        }
    }
}
#pragma dont_inline reset
void bgnd_rotate_xz_about_orgin_active_sobj(float angle) {
    g_active_sobj->flags_08_bits.bit6 = 1;
    g_active_sobj->flags_08_bits.bit3 = 1;
    rotate_xz(&g_active_sobj->pos, &g_active_sobj->pos, angle);
    g_active_sobj->ang.y = angle;
    update_mksobj(g_active_sobj);
}
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
void bgnd_set_active_sobj_pos_vel(void* script, float x, float y, float z) {
    (void)script;
    g_active_sobj->flags_08_bits.bit5 = 1;
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
/* TODO: [near miss] 97.72727%; canonical lookup expands exactly;
 * unused retail null comparison before publication remains. */
void bgnd_set_active_sobj_in_obj(int model_index, unsigned int object_id) {
    g_active_sobj = bgnd_fetch_sobj(model_index, object_id);
}
int bgnd_is_active_sobj_hidden(void) {
    return is_sobj_hidden(g_active_sobj);
}
/* TODO: [near miss] 92.85714%; unused retail null comparison before publication; stop. */
void bgnd_set_active_sobj(int object_id) {
    g_active_sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
}
void bgnd_sobj_cam_frustum_test_into_transparent(
    unsigned int sobj_id, float scale, float y_offset) {
    MkSobj* object;
    BackgroundDangerZone* zone;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
    if (object != 0) {
        zone = add_background_danger_zone(object->atomic, 1, 1);
        set_danger_zone_properties(zone, scale, y_offset);
    }
}
void obj_sobj_cam_frustum_test_into_transparent(
    MkObj* object, unsigned int sobj_id, float scale, float y_offset) {
    MkSobj* subobject;
    BackgroundDangerZone* zone;

    subobject = obj_find_sobj_by_id(object, sobj_id);
    if (subobject != 0) {
        zone = add_background_danger_zone(subobject->atomic, 1, 1);
        set_danger_zone_properties(zone, scale, y_offset);
    }
}
void bgnd_sobj_cam_volume_test_steer_over(
    unsigned int sobj_id, float scale, float y_offset) {
    MkSobj* object;
    BackgroundDangerZone* zone;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
    if (object != 0) {
        zone = add_background_danger_zone(object->atomic, 0, 0);
        set_danger_zone_properties(zone, scale, y_offset);
    }
}
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
void bgnd_rx_notify(
    PlyrInfo* player_info, int reaction,
    unsigned int power_level, unsigned int flags) {
    CmdScript* script;
    CmdScript* previous_script;
    GameInfo* info;

    g_current_reaction_info.player_info = player_info;
    g_current_reaction_info.reaction = reaction;
    g_current_reaction_info.power_level = power_level;
    g_current_reaction_info.flags = flags;

    if (g_current_reaction_info.handler_enabled == 1) {
        script = alloc_cmdscript();
        previous_script = active_cmdscript;
        info = &g_game_info;
        active_cmdscript = script;
        cmdscript_setup_execution(
            info->cmdscript, g_current_reaction_info.handler);
        cmdscript_execute(info->cmdscript);
        active_cmdscript = previous_script;
        if (script->instance != 0) {
            ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
        }
    }
}
/* TODO: [near miss] 98.84%; case 7 differs only in g_game_info base / Vec word register coloring. */
void bgnd_current_rx_set_info(int info_id, void* script_args, float value) {
    PlyrPdata* player;

    (void)script_args;
    switch (info_id) {
    case 4:
        player = g_current_reaction_info.player_info->slot.pdata;
        player->online_sync_index = (int)value;
        if (player->plyr_num == 0) {
            g_game_info.plyr0.fighting_lights.green_trigger = 1;
        } else {
            g_game_info.plyr1.fighting_lights.green_trigger = 1;
        }
        break;
    case 5:
        g_game_info.crack_count = value;
        break;
    case 7:
    {
        Vec impact = {0.0f, 0.0f, 0.0f};
        BgndObstacleEventData event;
        BgndObstacleEventData* data = &event;

        data->player_pdata =
            g_current_reaction_info.player_info->slot.pdata->his_plyr_pdata;
        data->impact_vector = &impact;
        data->event_id = g_game_info.collision_event_id;
        bgnd_apply_collision_info(data);
        break;
    }
    }
}
float bgnd_current_rx_get_info(int info_id) {
    PlyrPdata* pdata;

    switch (info_id) {
    case 9:
        return g_current_reaction_info.flags & 0x100;
    case 0:
        return g_current_reaction_info.flags & 0x80;
    case 1:
        return g_current_reaction_info.flags & 0x40;
    case 2:
        pdata = g_current_reaction_info.player_info->slot.pdata;
        return pdata->plyr_num;
    case 3:
        return g_current_reaction_info.power_level;
    case 4:
        pdata = g_current_reaction_info.player_info->slot.pdata;
        return pdata->online_sync_index;
    case 5:
        return g_game_info.crack_count;
    case 6:
        return g_game_info.field_34;
    case 10:
        pdata = g_current_reaction_info.player_info->slot.pdata;
        return pdata->previous_state;
    default:
        return 0.0f;
    }
}
void bgnd_setup_rx_handler(int handler) {
    g_current_reaction_info.handler_enabled = 1;
    g_current_reaction_info.handler = handler;
}
void bgnd_anim_camera_ended(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    unsigned int script_index;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    info = &g_game_info;
    active_cmdscript = script;
    script_index = info->section->cam_ended_script;
    if (script_index != 0) {
        cmdscript_setup_execution(info->cmdscript, script_index);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
    }
}

void bgnd_anim_camera_setup(void) {
    CmdScript* script;
    CmdScript* prev;
    GameInfo* info;
    unsigned int script_index;

    script = alloc_cmdscript();
    prev = active_cmdscript;
    active_cmdscript = script;
    cam_set_intro_cam_pause_ticks(0.0f);
    info = &g_game_info;
    script_index = info->section->cam_setup_script;
    if (script_index != 0) {
        cmdscript_setup_execution(info->cmdscript, script_index);
        cmdscript_execute(info->cmdscript);
    }
    active_cmdscript = prev;
    if (script->instance != 0) {
        ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
    }
}

/* TODO: [near miss] 99.81481%; data-pointer r28/r29 coloring remains at alpha conversion; stop at coloring. */
void bgnd_fade_object(int object_id, void* script, float fade_step) {
    BgndFadeObjectData* data;
    MkProc* process;
    MkSobj* object;
    MkSobj* found;
    unsigned int alpha;
    RpAtomic* atomic;
    RpGeometry* geometry;

    (void)script;
    data = 0;
    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        process = _create_mkproc_generic_tinystack(
            0xA00D, 0x1F, p_bgnd_fade_object,
            sizeof(BgndFadeObjectData), (MkHdr**)&data);
        if (process != 0 && data != 0) {
            found = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (found != 0) {
                set_subobject_transl(found);
            }
            data->object = object;
            data->fade_step = fade_step;
            data->alpha = 255.0f;
            data->alpha_int = (unsigned int)data->alpha;
            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}

static float p_bgnd_fade_object(void) {
    MkSobj* object;
    unsigned int alpha;
    BgndFadeObjectData* data;
    RpAtomic* atomic;
    RpGeometry* geometry;

    data = (BgndFadeObjectData*)apdata;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    if (data->alpha > data->fade_step) {
        data->alpha -= data->fade_step;
    } else {
        data->complete = 1;
        data->alpha = 0.0f;
    }
    data->alpha_int = (unsigned int)data->alpha;
    object = data->object;
    alpha = data->alpha_int;
    atomic = object->atomic;
    geometry = atomic->geometry;
    geometry->flags |= 0x40;
    set_atomic_material_alpha(atomic, alpha);
    set_atomic_material_specular(object->atomic, alpha);
    if (data->alpha == 0.0f) {
        hide_sobj(data->object);
        return -1.0f;
    }
    return 1.0f;
}
void pulsate_object(
    MkHdr* owner, int sobj_id, int max_hold_ticks, int min_hold_ticks,
    float fade_in_step, float fade_out_step) {
    unsigned int alpha;
    BgndPulsateData* data;
    MkSobj* object;
    MkObj* owner_object;
    RpGeometry* geometry;
    RpAtomic* atomic;
    MkProc* process;

    data = 0;
    owner_object = (MkObj*)owner;
    object = obj_find_sobj_by_id(owner_object, sobj_id);
    if (object != 0) {
        process = _create_mkproc_generic_tinystack(
            0xA00D, 0x1F, p_pulsate_object,
            sizeof(BgndPulsateData), (MkHdr**)&data);
        if (process != 0 && data != 0) {
            set_subobject_transl(object);
            data->object = object;
            data->field_0C = max_hold_ticks;
            data->field_10 = fade_in_step;
            data->field_14 = min_hold_ticks;
            data->field_18 = fade_out_step;
            data->field_20 = 0;
            data->field_1C = max_hold_ticks;
            data->alpha = 255.0f;
            data->field_2C = 0;
            data->field_30 = 0xFF;
            data->field_34 = 0.0f;
            data->field_38 = 0.0f;
            data->field_44 = 1.0f;
            data->field_48 = 1.0f;
            data->field_3C = 1.0f;
            data->field_40 = 1.0f;
            data->alpha_int = (unsigned int)data->alpha;

            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            mk_insert(&process->hdr, &owner_object->child_list);
        }
    }
}
void bgnd_pulsate_object(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step) {
    BgndPulsateData* data;
    MkProc* process;
    MkSobj* object;
    MkSobj* found;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int alpha;

    data = 0;
    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        process = _create_mkproc_generic_tinystack(
            0xA00D, 0x1F, p_bgnd_pulsate_object,
            sizeof(BgndPulsateData), (MkHdr**)&data);
        if (process != 0 && data != 0) {
            found = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (found != 0) {
                set_subobject_transl(found);
            }
            sobj_set_priority(object, 0xE);
            object->z_offset = -0.2f;
            data->object = object;
            data->field_0C = max_hold_ticks;
            data->field_10 = fade_in_step;
            data->field_14 = min_hold_ticks;
            data->field_18 = fade_out_step;
            data->field_20 = 0;
            data->field_1C = max_hold_ticks;
            data->alpha = 255.0f;
            data->field_2C = 0;
            data->field_30 = 0xFF;
            data->field_34 = 0.0f;
            data->field_38 = 0.0f;
            data->field_44 = 1.0f;
            data->field_48 = 1.0f;
            data->field_3C = 1.0f;
            data->field_40 = 1.0f;
            data->alpha_int = (unsigned int)data->alpha;

            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}
void bgnd_pulsate_object_with_caps_and_scale(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step, unsigned int min_alpha, unsigned int max_alpha,
    float scale_step_xz, float scale_step_y,
    float min_scale_xz, float min_scale_y,
    float max_scale_xz, float max_scale_y) {
    BgndPulsateData* data;
    MkProc* process;
    MkSobj* object;
    MkSobj* found;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int alpha;

    data = 0;
    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        build_sine_table();
        process = _create_mkproc_generic_tinystack(
            0xA00D, 0x1F, p_bgnd_pulsate_object,
            sizeof(BgndPulsateData), (MkHdr**)&data);
        if (process != 0 && data != 0) {
            found = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (found != 0) {
                set_subobject_transl(found);
            }
            data->object = object;
            data->field_0C = max_hold_ticks;
            data->field_10 = fade_in_step;
            data->field_14 = min_hold_ticks;
            data->field_18 = fade_out_step;
            data->field_20 = 0;
            data->field_1C = max_hold_ticks;
            data->alpha = (float)max_alpha;
            data->field_2C = min_alpha;
            data->field_30 = max_alpha;
            data->field_34 = scale_step_xz;
            data->field_38 = scale_step_y;
            data->object->scale.x = 1.0f;
            data->object->scale.y = 1.0f;
            data->object->scale.z = 1.0f;
            data->field_44 = max_scale_xz;
            data->field_48 = max_scale_y;
            data->field_3C = min_scale_xz;
            data->field_40 = min_scale_y;
            data->object->flags_08_bits.scale_dirty = 1;
            data->alpha_int = (unsigned int)data->alpha;

            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}
void bgnd_pulsate_object_with_caps(
    int object_id, int max_hold_ticks, float fade_in_step, int min_hold_ticks,
    float fade_out_step, unsigned int min_alpha, unsigned int max_alpha) {
    BgndPulsateData* data;
    MkProc* process;
    MkSobj* object;
    MkSobj* found;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int alpha;

    data = 0;
    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
    if (object != 0) {
        process = _create_mkproc_generic_tinystack(
            0xA00D, 0x1F, p_bgnd_pulsate_object,
            sizeof(BgndPulsateData), (MkHdr**)&data);
        if (process != 0 && data != 0) {
            found = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (found != 0) {
                set_subobject_transl(found);
            }
            data->object = object;
            data->field_0C = max_hold_ticks;
            data->field_10 = fade_in_step;
            data->field_14 = min_hold_ticks;
            data->field_18 = fade_out_step;
            data->field_20 = 0;
            data->field_1C = max_hold_ticks;
            data->alpha = (float)max_alpha;
            data->field_34 = 0.0f;
            data->field_38 = 0.0f;
            data->field_44 = 1.0f;
            data->field_48 = 1.0f;
            data->field_3C = 1.0f;
            data->field_40 = 1.0f;
            data->field_2C = min_alpha;
            data->field_30 = max_alpha;
            data->alpha_int = (unsigned int)data->alpha;

            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            mk_insert(&process->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}
static float p_bgnd_pulsate_object(void) {
    BgndPulsateData* data;
    MkSobj* object;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int alpha;
    unsigned int sine_index;

    data = (BgndPulsateData*)apdata;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }

    object = data->object;
    if ((object->atomic->object.flags & 4) == 0) {
        data->field_20 = 2;
        data->field_1C = 1;
        data->alpha = 0.0f;
        data->alpha_int = (unsigned int)data->alpha;
        object = data->object;
        alpha = data->alpha_int;
        atomic = object->atomic;
        geometry = atomic->geometry;
        geometry->flags |= 0x40;
        set_atomic_material_alpha(atomic, alpha);
        set_atomic_material_specular(object->atomic, alpha);
        return 1.0f;
    }

    if (--data->field_1C != 0) {
        switch (data->field_20) {
        case 1:
            if (data->alpha > (float)data->field_2C + data->field_18) {
                data->alpha -= data->field_18;
            } else {
                data->field_1C = 1;
                data->alpha = (float)data->field_2C;
            }

            if (data->alpha > (float)data->field_2C) {
                sine_index = (unsigned int)(
                    (data->alpha - (float)data->field_2C) *
                    (float)(0x100U / (data->field_30 - data->field_2C)));
                if (data->object->scale.x > data->field_3C) {
                    data->object->scale.x =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                    data->object->scale.z =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                }
                if (data->object->scale.y > data->field_40) {
                    data->object->scale.y =
                        soul_sine[sine_index] *
                        (data->field_48 - data->field_40) + data->field_40;
                }
            }
            data->alpha_int = (unsigned int)data->alpha;
            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            break;
        case 3:
            data->alpha += data->field_10;
            if (data->alpha < (float)data->field_2C) {
                data->alpha += data->field_10;
            }
            if (data->alpha >= (float)data->field_30) {
                data->field_1C = 1;
                data->alpha = (float)data->field_30;
            }

            if (data->alpha >= (float)data->field_2C &&
                data->alpha < (float)data->field_30) {
                sine_index = (unsigned int)(
                    (data->alpha - (float)data->field_2C) *
                    (float)(0x100U / (data->field_30 - data->field_2C)));
                if (data->object->scale.x < data->field_44) {
                    data->object->scale.x =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                    data->object->scale.z =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                }
                if (data->object->scale.y < data->field_48) {
                    data->object->scale.y =
                        soul_sine[sine_index] *
                        (data->field_48 - data->field_40) + data->field_40;
                }
            } else if (data->alpha < (float)data->field_2C) {
                data->object->scale.x = data->field_3C;
                data->object->scale.z = data->field_3C;
            }
            data->alpha_int = (unsigned int)data->alpha;
            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            break;
        default:
            break;
        }
        return 1.0f;
    } else {
        switch (data->field_20) {
        case 0:
            data->field_20 = 1;
            data->field_1C = 1000;
            break;
        case 1:
            data->field_20 = 2;
            data->field_1C = data->field_14;
            break;
        case 2:
            data->field_20 = 3;
            data->field_1C = 1000;
            break;
        case 3:
            data->field_20 = 0;
            data->field_1C = data->field_0C;
            break;
        default:
            break;
        }
    }
    return 1.0f;
}

static float p_pulsate_object(void) {
    BgndPulsateData* data;
    MkSobj* object;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int alpha;
    unsigned int sine_index;

    data = (BgndPulsateData*)apdata;
    object = data->object;
    if ((object->atomic->object.flags & 4) == 0) {
        data->field_20 = 2;
        data->field_1C = 1;
        data->alpha = 0.0f;
        data->alpha_int = (unsigned int)data->alpha;
        object = data->object;
        alpha = data->alpha_int;
        atomic = object->atomic;
        geometry = atomic->geometry;
        geometry->flags |= 0x40;
        set_atomic_material_alpha(atomic, alpha);
        set_atomic_material_specular(object->atomic, alpha);
        return 1.0f;
    }

    if (--data->field_1C != 0) {
        switch (data->field_20) {
        case 1:
            if (data->alpha > (float)data->field_2C + data->field_18) {
                data->alpha -= data->field_18;
            } else {
                data->field_1C = 1;
                data->alpha = (float)data->field_2C;
            }

            if (data->alpha > (float)data->field_2C) {
                sine_index = (unsigned int)(
                    (data->alpha - (float)data->field_2C) *
                    (float)(0x100U / (data->field_30 - data->field_2C)));
                if (data->object->scale.x > data->field_3C) {
                    data->object->scale.x =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                    data->object->scale.z =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                }
                if (data->object->scale.y > data->field_40) {
                    data->object->scale.y =
                        soul_sine[sine_index] *
                        (data->field_48 - data->field_40) + data->field_40;
                }
            }
            data->alpha_int = (unsigned int)data->alpha;
            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            break;
        case 3:
            data->alpha += data->field_10;
            if (data->alpha < (float)data->field_2C) {
                data->alpha += data->field_10;
            }
            if (data->alpha >= (float)data->field_30) {
                data->field_1C = 1;
                data->alpha = (float)data->field_30;
            }

            if (data->alpha >= (float)data->field_2C &&
                data->alpha < (float)data->field_30) {
                sine_index = (unsigned int)(
                    (data->alpha - (float)data->field_2C) *
                    (float)(0x100U / (data->field_30 - data->field_2C)));
                if (data->object->scale.x < data->field_44) {
                    data->object->scale.x =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                    data->object->scale.z =
                        soul_sine[sine_index] *
                        (data->field_44 - data->field_3C) + data->field_3C;
                }
                if (data->object->scale.y < data->field_48) {
                    data->object->scale.y =
                        soul_sine[sine_index] *
                        (data->field_48 - data->field_40) + data->field_40;
                }
            } else if (data->alpha < (float)data->field_2C) {
                data->object->scale.x = data->field_3C;
                data->object->scale.z = data->field_3C;
            }
            data->alpha_int = (unsigned int)data->alpha;
            object = data->object;
            alpha = data->alpha_int;
            atomic = object->atomic;
            geometry = atomic->geometry;
            geometry->flags |= 0x40;
            set_atomic_material_alpha(atomic, alpha);
            set_atomic_material_specular(object->atomic, alpha);
            break;
        default:
            break;
        }
        return 1.0f;
    } else {
        switch (data->field_20) {
        case 0:
            data->field_20 = 1;
            data->field_1C = 1000;
            break;
        case 1:
            data->field_20 = 2;
            data->field_1C = data->field_14;
            break;
        case 2:
            data->field_20 = 3;
            data->field_1C = 1000;
            break;
        case 3:
            data->field_20 = 0;
            data->field_1C = data->field_0C;
            break;
        default:
            break;
        }
    }
    return 1.0f;
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
    PlyrPdata* player, int script_function) {
    CmdScript* script;
    MkProc* process;

    process = MK_LIVE(player->own_player_proc, player->own_player_proc_instance);
    if (process != 0) {
        script = get_cmdscript_for_proc(process);
        if (script != 0) {
            script->unk28 = script_function;
            xfer_player_proc(process, bgnd_call_script_function);
        }
    }
}


static inline MkProc* player_live_own_player_proc(PlyrPdata* owner) {
    MkProc* object = owner->own_player_proc;
    if (object != 0) {
        if (object->instance == owner->own_player_proc_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 99.0625%; first player-process latch swaps owner/object r4 and r5. */
void mks_xfer_collision_info_plyr_to_script(int script_function,
                                             int player) {
    PlyrPdata* pdata;
    MkProc* process;

    if (player == 1) {
        pdata = g_active_obstacle_event_data->player_pdata;
        process = MK_LIVE(pdata->player_proc, pdata->player_proc_instance);
    } else {
        pdata = g_active_obstacle_event_data->player_pdata;
        process = player_live_own_player_proc(pdata);
    }
    get_cmdscript_for_proc(process)->unk28 = script_function;
    xfer_player_proc(process, bgnd_call_script_function);
}
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
    char message[180];
    MkSobj* object;
    AniTextureControl* texture;

    if (g_game_info.bgnd_obj != 0) {
        object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, sobj_id);
        if (object == 0) {
            sprintf(message,
                    "sobj %d was not found. Error in bgnd_append_texture_to_material",
                    sobj_id);
            return;
        }
        if (g_game_info.bgnd_id == 0x16) {
            texture = append_texture_by_name_to_atomic_material_id(
                0x18006D, texture_name, object->atomic,
                material_id, texture_slot);
        } else {
            texture = append_texture_by_name_to_atomic_material_id(
                0x2001E, texture_name, object->atomic,
                material_id, texture_slot);
        }
        if (texture == 0) {
            return;
        }
    }
}
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries) {
    char message[180];
    MkSobj* object;
    unsigned int index;
    unsigned int sobj_id;
    int material_id;
    char* texture_name;
    int texture_slot;

    index = 0;
    while ((sobj_id = entries[index].sobj_id) != 0) {
        material_id = entries[index].material_id;
        texture_name = entries[index].texture_name;
        texture_slot = entries[index].texture_slot;
        if (g_game_info.bgnd_obj != 0) {
            object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, sobj_id);
            if (object == 0) {
                sprintf(message,
                        "sobj %d was not found. Error in bgnd_append_texture_to_material",
                        sobj_id);
            } else if (g_game_info.bgnd_id == 0x16) {
                append_texture_by_name_to_atomic_material_id(
                    0x18006D, texture_name, object->atomic,
                    material_id, texture_slot);
            } else {
                append_texture_by_name_to_atomic_material_id(
                    0x2001E, texture_name, object->atomic,
                    material_id, texture_slot);
            }
        }
        index++;
    }
}
void bgnd_swap_textures(int sobj_id, int material_id, unsigned int frame) {
    char sobj_error[80];
    char material_error[80];
    AniTextureControl* control;
    MkSobj* object;

    object = obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
    if (object == 0) {
        sprintf(sobj_error,
                "bgnd_swap_textures: sobj_id = %d not found", sobj_id);
        return;
    }
    if (frame <= 1) {
        control = find_atc_for_atomic_material_id(object->atomic, material_id);
        if (control != 0) {
            set_ani_texture_frame(control, frame);
            return;
        }
        sprintf(material_error,
                "Material ID not found. sobj_id = %d, mat_id = %d",
                sobj_id, material_id);
    }
}
/* TODO: [near miss] 98.94%; auto-inlined bgnd_swap_textures loop; only frame/sobj_id r27/r28 coloring swap remains. */
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries, int frame) {
    unsigned int index = 0;

    while (entries[index].sobj_id != 0) {
        bgnd_swap_textures(entries[index].sobj_id,
                           entries[index].material_id, frame);
        index++;
    }
}
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
void bgnd_replace_tex_with_wiff_and_ani(
    int object_id, const char* wiff_name, float frame_rate,
    int first_frame, int texture_type) {
    AniTextureControl* control;
    MkSobj* object;
    int handle;

    object = obj_create_sobjs_by_id(g_game_info.bgnd_obj, object_id);
    control = 0;
    if (object != 0) {
        switch (texture_type) {
        case 0:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x10005, "SINGLEFIRE", wiff_name);
            break;
        case 1:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x10005, "SINGLEFIRE", wiff_name);
            break;
        case 2:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x2001E, "ST_THUNDER", wiff_name);
            break;
        case 5:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x2001E, "LM_PINCONES1", wiff_name);
            break;
        case 6:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x2001E, "LM_LIONSHADOW1", wiff_name);
            break;
        case 4:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x140064, "KRYPT_SINGLEFIRE", wiff_name);
            break;
        case 7:
            control = replace_sobj_texture_with_named_wiff(
                object, 0x2001E, "GREEN_FIRE_WIF", wiff_name);
            break;
        case 3:
            handle = 0x8003D;
            if (mode_of_play != 9 && mode_of_play != 10) {
                handle = 0x2001E;
            }
            control = replace_sobj_texture_with_named_wiff(
                object, handle, "PT_BOLT", wiff_name);
            break;
        }
        if (control != 0) {
            set_ani_texture_framerate(control, frame_rate);
            set_ani_texture_frame(control, first_frame);
        }
    }
}
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
void bgnd_set_sobj_uv_scroll_abs_values(
    float u1, float v1, float u2, float v2, unsigned int index) {
    UvScrollControl* control;
    BgndUvScrollControlItem* item;

    if (index < 8) {
        item = &bgnd_uv_scroll_control_item[index];
        control = MK_HDR_LIVE(item->control, item->instance);
        if (control != 0) {
            control->mtx1[12] = u1;
            control->mtx1[13] = v1;
            control->mtx2[12] = u2;
            control->mtx2[13] = v2;
        }
    }
}
void bgnd_set_sobj_uv_scroll_rate_values(
    float u1, float v1, float u2, float v2, unsigned int index) {
    UvScrollControl* control;
    BgndUvScrollControlItem* item;

    if (index < 8) {
        item = &bgnd_uv_scroll_control_item[index];
        control = MK_HDR_LIVE(item->control, item->instance);
        if (control != 0) {
            control->rateU1 = u1;
            control->rateV1 = v1;
            control->rateU2 = u2;
            control->rateV2 = v2;
        }
    }
}
static inline void bgnd_clear_uv_scroll_control(unsigned int index) {
    BgndUvScrollControlItem* item;
    UvScrollControl* control;

    if (index < 8) {
        item = &bgnd_uv_scroll_control_item[index];
        control = item->control;
        control = MK_HDR_LIVE(control, item->instance);
        if (control != 0 && control->hdr.instance != 0) {
            control->hdr.typed_vtbl->destroy(&control->hdr);
        }
        item->control = 0;
        item->instance = 0;
    }
}

void bgnd_init_all_uv_scroll_w_control(void) {
    int index;

    for (index = 0; index < 8; index++) {
        bgnd_clear_uv_scroll_control(index);
    }
}
void bgnd_destroy_sobj_uv_scroll_w_control(unsigned int index) {
    bgnd_clear_uv_scroll_control(index);
}
static inline unsigned int bgnd_uv_scroll_control_slot_available(unsigned int index) {
    UvScrollControl* control;
    BgndUvScrollControlItem* item;

    if (index >= 8) {
        return 0;
    }
    item = &bgnd_uv_scroll_control_item[index];
    control = item->control;
    control = MK_HDR_LIVE(control, item->instance);
    if (control != 0) {
        return 0;
    }
    item->control = 0;
    item->instance = 0;
    return 1;
}

int bgnd_start_sobj_uv_scroll_w_control(
    int sobj_id, float u1, float v1, float u2, float v2,
    unsigned int translucent, unsigned int index) {
    UvScrollControl* control;
    BgndUvScrollControlItem* item;
    MkSobj* object;
    int result;

    result = 0;
    if (bgnd_uv_scroll_control_slot_available(index) &&
        g_game_info.bgnd_obj != 0) {
        control = start_sobj_uv_scroll(
            g_game_info.bgnd_obj, sobj_id, u1, v1, u2, v2);
        if (translucent == 1) {
            object = obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
            if (object != 0) {
                object->flags09_bits.bit5 = translucent;
            }
        }
        if (control != 0) {
            result = 1;
            item = &bgnd_uv_scroll_control_item[index];
            item->control = control;
            item->instance = control->hdr.instance;
        }
    }
    return result;
}
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
int bgnd_launch_plyr_up_and_forward_running(void) {
    MkProc* process;
    int moving;

    process = MK_LIVE(g_game_info.collision_player_pdata->his_plyr_pdata->transient_proc, g_game_info.collision_player_pdata->his_plyr_pdata->transient_proc_instance);
    moving = g_game_info.player_objects[0]->flags_08_bits.moving;
    if (process != 0 || moving) {
        return 1;
    }
    return 0;
}
void bgnd_launch_plyr_up_and_forward(
    float ground_y, float gravity, float vertical_velocity,
    float forward_velocity, int duration, float damping, int animation) {
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
void bgnd_unhide_sobj_list(unsigned int* object_ids) {
    unsigned int object_id;
    unsigned int* next_id;
    MkSobj* object;
    GameInfo* game = &g_game_info;

    object_id = *object_ids;
    next_id = object_ids + 1;
    while (object_id != 0) {
        if (object_id == 0x63) {
            object = obj_find_sobj_by_id(game->bgnd_obj, 0xCA);
            if (object != 0 && is_sobj_hidden(object) != 0) {
                break;
            }
        }
        if (object_id == 0x65) {
            object = obj_find_sobj_by_id(game->bgnd_obj, 0xCA);
            if (object != 0 && is_sobj_hidden(object) != 0) {
                break;
            }
        }
        if (object_id == 0x64) {
            object = obj_find_sobj_by_id(game->bgnd_obj, 0xCB);
            if (object != 0 && is_sobj_hidden(object) != 0) {
                break;
            }
        }
        if (object_id == 0x66) {
            object = obj_find_sobj_by_id(game->bgnd_obj, 0xCB);
            if (object != 0 && is_sobj_hidden(object) != 0) {
                break;
            }
        }

        obj_create_sobjs_by_id(game->bgnd_obj, object_id);
        if (game->bgnd_obj != 0) {
            object = obj_find_sobj_by_id(game->bgnd_obj, object_id);
            if (object != 0) {
                unhide_sobj(object);
            }
        }
        object_id = *next_id;
        next_id++;
    }
}
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
void bgnd_hide_sobj_list(unsigned int* object_ids) {
    GameInfo* game_info;
    unsigned int object_id;
    unsigned int* next_id;
    MkSobj* object;

    game_info = &g_game_info;
    next_id = object_ids + 1;
    object_id = *object_ids;
    while (object_id != 0) {
        obj_create_sobjs_by_id(game_info->bgnd_obj, object_id);
        if (game_info->bgnd_obj != 0) {
            object = obj_find_sobj_by_id(game_info->bgnd_obj, object_id);
            if (object != 0) {
                hide_sobj(object);
            }
        }
        object_id = *next_id;
        next_id++;
    }
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
void bgnd_start_sobj_uv_scroll(
    int object_id, float u1, float v1, float u2, float v2,
    unsigned int translucent) {
    MkSobj* object;

    if (g_game_info.bgnd_obj != 0) {
        start_sobj_uv_scroll(
            g_game_info.bgnd_obj, object_id, u1, v1, u2, v2);
        if (translucent == 1) {
            object = obj_find_sobj_by_id(g_game_info.bgnd_obj, object_id);
            if (object != 0) {
                object->flags09_bits.bit5 = translucent;
            }
        }
    }
}
void bgnd_start_sobj_uv_scroll_tbl(BgndUvScrollEntry* entries) {
    BgndUvScrollEntry* entry;
    MkSobj* object;
    int index;
    unsigned int object_id;
    unsigned int translucent;
    float u1;
    float v1;
    float u2;
    float v2;

    index = 0;
    while ((object_id = entries[index].object_id) != 0) {
        entry = &entries[index];
        u1 = entry->u1;
        v1 = entry->v1;
        u2 = entry->u2;
        v2 = entry->v2;
        translucent = entry->translucent;
        if (g_game_info.bgnd_obj != 0) {
            start_sobj_uv_scroll(
                g_game_info.bgnd_obj, object_id,
                u1, v1, u2, v2);
            if (translucent == 1) {
                object = obj_find_sobj_by_id(
                    g_game_info.bgnd_obj, object_id);
                if (object != 0) {
                    object->flags09_bits.bit5 = translucent;
                }
            }
        }
        index++;
    }
}
void bgnd_light_set_color(int light_id, float red, float green, float blue) {
    MkObj* object;
    MkxRpLight* wrapper;
    RpLight* light;
    RwRGBAReal color;

    object = find_obj_by_id(light_id);
    if (object != 0) {
        wrapper = find_mkx_rplight_in_obj(object);
        if (wrapper != 0) {
            light = wrapper->light;
            if (light != 0) {
                color.red = red;
                color.green = green;
                color.blue = blue;
                color.alpha = 0.0f;
                RpLightSetColor(light, &color);
            }
        }
    }
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
    switch (value_id) {
    case 1:
        return g_game_info.active_level;
    case 2:
        return g_game_info.flag_bits.field_bit6;
    case 3:
        return g_game_info.flag_bits.high_res_path;
    case 0:
        return 1;
    case 4:
        return g_game_info.flag_bits.lens_flare_enabled;
    case 5:
        return intro_done();
    default:
        return 0;
    }
}
void bgnd_create_sobjs(void) {
    obj_create_sobjs_by_id(g_game_info.bgnd_obj, 0);
}
/* TODO: [near miss] 97.67123%; z component and light_x swap f3/f5, and the
 * half-width operands load in swapped order; equivalent coloring/scheduling. */
MkObj* bgnd_place_point_light_for_ticks(
    LightDef* light_def, int ticks, int offset_from_tightrope,
    float radius_step) {
    BgndPointLightLifeData* data;
    MkObj* light;
    MkProc* process;
    float perpendicular_x;
    float perpendicular_z;
    float light_x;
    float half_width;

    data = 0;
    if ((offset_from_tightrope & 1) != 0) {
        perpendicular_x = tightrope_perp_uv.x;
        perpendicular_z = tightrope_perp_uv.z;
        light_x = light_def->field20;
        if (perpendicular_x * (light_def->field20 - camera_obj->pos.x) +
                perpendicular_z *
                    (light_def->field28 - camera_obj->pos.z) >
            0.0f) {
            perpendicular_x = -1.0f * perpendicular_x;
            perpendicular_z = -1.0f * perpendicular_z;
        }
        half_width = light_def->field1C * 0.5f;
        perpendicular_x *= half_width;
        perpendicular_z *= half_width;
        light_def->field20 = light_x + perpendicular_x;
        light_def->field28 += perpendicular_z;
    }

    light = load_light(light_def, &point_light_list, 0);
    if (light == 0) {
        return 0;
    }
    process = _create_mkproc_generic_tinystack(
        0xA00D, 0x1F, p_bgnd_point_light_life_span,
        sizeof(BgndPointLightLifeData), (MkHdr**)&data);
    if (process != 0 && data != 0) {
        data->light = light;
        data->ticks = ticks;
        data->radius_step = radius_step;
    }
    return light;
}
static float p_bgnd_point_light_life_span(void) {
    BgndPointLightLifeData* data;

    data = (BgndPointLightLifeData*)apdata;
    if (g_game_info.bgnd_obj == 0) {
        return -1.0f;
    }
    if (--data->ticks != 0) {
        return 1.0f;
    }

    if (data->radius_step != 0.0f) {
        while (adjust_point_light_associated_with_obj_radius(
                   data->light, data->radius_step) == 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
    }
    if (data->light->hdr.instance != 0) {
        data->light->hdr.typed_vtbl->destroy(&data->light->hdr);
    }
    return -1.0f;
}
float degrees_to_rad(void* script, float degrees) {
    (void)script;
    return 3.1415927f * degrees / 180.0f;
}
float rad_to_degrees(void* script, float radians) {
    (void)script;
    return 180.0f * radians / 3.1415927f;
}
float int_to_float(int value) { return value; }
int float_to_int(void* script, float value) {
    (void)script;
    return (int)value;
}
void obj_sobj_set_material(MkSobj* object, unsigned int alpha) {
    RpAtomic* atomic;

    atomic = object->atomic;
    atomic->geometry->flags |= 0x40;
    set_atomic_material_alpha(atomic, alpha);
    set_atomic_material_specular(object->atomic, alpha);
}
RpAtomic* force_atomic_material_alpha(RpAtomic* atomic, void* alpha) {
    atomic->geometry->flags |= 0x40;
    set_atomic_material_alpha(atomic, (unsigned int)alpha);
    return atomic;
}
float p_track_cam_ang_y_light(void) {
    float initial_yaw;

    if (light_obj != 0) {
        initial_yaw = light_obj->ang.y;
    } else {
        return -1.0f;
    }

    for (;;) {
        if (light_obj != 0) {
            light_obj->ang.y =
                camera_obj->ang.y - 3.1415927f + initial_yaw;
            update_mkobj(light_obj);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        } else {
            break;
        }
    }
    return -1.0f;
}







void load_bgnd_style(int player, const char* script_name, void* script_args) {
    GlobalMoveset* moveset;
    MkFileInfo* animation_section;
    MkObj* weapon;

    (void)script_args;
    if (player < 2) {
        moveset = &global_movesets[player + 6];
        moveset->script =
            cmdscript_loadfile_by_name(player + 12, script_name);
        if (moveset->script != 0) {
            if (moveset->script->table_count != 0) {
                moveset->definition = (MovesetDefinition*)get_data_table(
                    moveset->script, moveset->script->table_count);
                moveset->animation_header =
                    moveset->definition->animation_header;
            }
            if (moveset->definition->primary_weapon != 0) {
                weapon = MK_HDR_LIVE(moveset->primary_weapon, moveset->primary_weapon_instance);

                if (weapon != 0) {
                    return;
                }
                weapon = load_weapon_from_slot(
                    moveset->definition->primary_weapon, 0x2001E);
                if (weapon != 0) {
                    moveset->primary_weapon = weapon;
                    moveset->primary_weapon_instance = weapon->hdr.instance;
                    mk_insert((MkHdr*)weapon, &moveset->script->pdata_list);
                    mk_insert((MkHdr*)weapon,
                              &g_game_info.bgnd_obj->child_list);
                }
            }
            if (moveset->definition->secondary_weapon != 0) {
                weapon = MK_HDR_LIVE(moveset->secondary_weapon, moveset->secondary_weapon_instance);

                if (weapon != 0) {
                    return;
                }
                weapon = load_weapon_from_slot(
                    moveset->definition->secondary_weapon, 0x2001E);
                if (weapon != 0) {
                    moveset->secondary_weapon = weapon;
                    moveset->secondary_weapon_instance = weapon->hdr.instance;
                    mk_insert((MkHdr*)weapon, &moveset->script->pdata_list);
                    mk_insert((MkHdr*)weapon,
                              &g_game_info.bgnd_obj->child_list);
                }
            }
            animation_section = find_section_by_name(
                moveset->definition->animation_section_name);
            add_anim_section_async_pal(
                0x2001E, animation_section,
                (int*)global_movesets[player + 6U].animation_data,
                0, 1);
            wait_for_slot_load(0x2001E);
            load_bgnd_fstyle_sign(player);
        }
    }
}
/* TODO: [near miss] 95.2381%; first loop initializes offset with li rather
 * than moving the existing zero; stop at the zero-materialization ceiling. */
void ncs_bgnd_nuke_collision_to_script_interface(void) {
    unsigned int index;

    for (index = 0; index < 8; index++) {
        destroy_list(&g_bgnd_collision_to_script_if[index]);
    }
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;
    for (index = 0; index < 8; index++) {
        g_bgnd_collision_to_script_if[index] = 0;
    }
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;
    for (index = 0; index < 24; index++) {
        bgnd_danger_zones[index].obstacle = 0;
    }
}
MkObj* retrieve_bgnd_obj(void) {
    return g_game_info.bgnd_obj;
}
static inline void bgnd_destroy_object_latch(
    MkObj** object_ptr, unsigned int* instance) {
    MkObj* object = *object_ptr;

    if (object != 0) {
        if (object->hdr.instance != *instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        object = *object_ptr;
        if (object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        *object_ptr = 0;
        *instance = 0;
    }
}

/* TODO: [near miss] 79.67%; teardown sequence agrees (696 vs 676 bytes); latch merge and loop-induction scheduling remain. */
void destroy_background_extras(void) {
    unsigned int index;

    unload_script(0xC);
    unload_script(0xD);
    for (index = 0; index < 8; index++) {
        destroy_list(&g_bgnd_collision_to_script_if[index]);
    }
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;

    for (index = 6; index < 8; index++) {
        bgnd_destroy_object_latch(
            &global_movesets[index].primary_weapon,
            &global_movesets[index].primary_weapon_instance);
        bgnd_destroy_object_latch(
            &global_movesets[index].secondary_weapon,
            &global_movesets[index].secondary_weapon_instance);
    }
    if (g_game_info.field_64 != 0) {
        destroy_list(&g_game_info.field_64);
    }
    if (g_game_info.displayed_items != 0) {
        destroy_list(&g_game_info.displayed_items);
    }
    if (g_game_info.npc_list != 0) {
        destroy_list(&g_game_info.npc_list);
    }
    g_game_info.wall_hider = 0;
    if (g_game_info.camera_proc != 0) {
        MkProc* process = g_game_info.camera_proc;

        if (process->hdr.instance != g_game_info.camera_proc_instance) {
            process = 0;
        }
        if (process != 0) {
            if (g_game_info.camera_proc->hdr.instance != 0) {
                g_game_info.camera_proc->hdr.typed_vtbl->destroy(
                    &g_game_info.camera_proc->hdr);
            }
            g_game_info.camera_proc = 0;
            g_game_info.camera_proc_instance = 0;
        }
    }
    for (index = 0; index < 24; index++) {
        bgnd_danger_zones[index].obstacle = 0;
    }
    for (index = 0; index < 15; index++) {
        g_bgnd_preloaded_models[index] = 0;
    }
    g_active_sobj = 0;
    g_latest_obj_pfx = 0;
    g_active_obstacle_event_data = 0;
    g_active_bgnd_danger_zone = 0;
    g_active_bgnd_col_item = 0;
    g_bgnd_cracks = 0;
    g_bgnd_last_crack_overwritten = 0;
    for (index = 0; index < 20; index++) {
        g_pebbles[index] = 0;
        g_pebbles_pdata[index] = 0;
    }
    g_current_pebble = 0;
    g_chunk_launch_monitor_pdata = 0;
    g_active_launched_sobj_pdata = 0;
    g_launched_sobj_crossing_plane_pdata = 0;
    g_sobj_launch_monitor_pdata = 0;
}
static void add_mkx_light_obj_to_bgnd_cleanup_list(MkHdr* header) {
    MkxRpLight* light;
    MkObj* object;

    light = MKX_RPLIGHT_FROM_HDR(header);
    object = MK_HDR_LIVE(light->obj, light->obj_instance);
    if (object != 0) {
        mk_insert(&object->hdr, &g_game_info.bgnd_obj->child_list);
    }
}
/* TODO: [near miss] 97.54%; retail pools g_current_reaction_info/collision lists off
 * bgnd_animations (TU data layout: bgnd.c defines no .bss); r30/r31 coloring follows. */
int load_background(int bgnd_id) {
    char* art_name;
    int art_id;
    int i;
    BgndDataTable* data_table;
    char** effect_list;
    LoadBgndCtx ctx;

    if (mode_of_play == 6 && bgnd_id != 0x17) {
        return 0;
    }

    RwImageSetGamma(1.0f);

    load_ssf((MkFileEntry*)global_background_data[bgnd_id].ssf_entry);
    g_game_info.cmdscript =
        cmdscript_loadfile_by_name(0xB, global_background_data[bgnd_id].script_name);
    g_game_info.section = get_data_table(
        g_game_info.cmdscript, g_game_info.cmdscript->table_count);
    g_game_info.misc = g_game_info.section->misc;

    init_misc_bgnd_data();

    g_game_info.field_64 = 0;
    g_game_info.displayed_items = 0;
    g_game_info.npc_list = 0;
    g_current_reaction_info.handler_enabled = 0;
    for (i = 0; i < 8; i++) {
        g_bgnd_collision_to_script_if[i] = 0;
    }
    g_active_obstacle_event_data = 0;
    g_active_bgnd_col_item = 0;

    data_table = g_game_info.section;
    if (data_table != 0 && (data_table->flags88 & 1) != 0) {
        return 1;
    }

    if (mode_of_play == 9 || mode_of_play == 10) {
        if ((global_background_data[bgnd_id].flags & 8) != 0) {
            art_name = data_table->art_name;
            art_id = 0x8003D;
        } else {
            return 0;
        }
    } else if (mode_of_play == 11) {
        if ((global_background_data[bgnd_id].flags & 0x10) != 0) {
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

    if (data_table != 0 && data_table->art_name != 0) {
        load_art_section_by_name(art_id, art_name);
    }

    g_game_info.bgnd_obj =
        (MkObj*)load_named_model_from_slot(art_id, "BACKGROUND", 0x1004, 0);

    if (g_game_info.section != 0) {
        if (g_game_info.section->anims != 0) {
            load_background_anims(g_game_info.section->anims, bgnd_id);
        } else {
            memset(bgnd_animations, 0, 0x84);
        }
    }

    init_weapon_trail_light_list();

    if (g_game_info.misc->lights_spec != 0) {
        load_lights(g_game_info.misc->lights_spec, &bgnd_spec_light_list);
    }
    if (g_game_info.misc->lights_plyr != 0) {
        load_lights(g_game_info.misc->lights_plyr, &plyr_light_list);
    }
    if (g_game_info.misc->lights_bgnd != 0) {
        load_lights(g_game_info.misc->lights_bgnd, &bgnd_light_list);
    } else {
        g_game_info.bgnd_obj->light_flags = 0;
    }

    if (g_game_info.bgnd_obj != 0) {
        insert_fgnd_mkobj(g_game_info.bgnd_obj);
    } else {
        return 0;
    }

    if (g_game_info.misc->lights_bgnd != 0) {
        g_game_info.bgnd_obj->light_flags = 0x1009;
    } else {
        g_game_info.bgnd_obj->light_flags = 0x1000;
    }

    set_background_color(
        (int)g_game_info.section->bg_r,
        (int)g_game_info.section->bg_g,
        (int)g_game_info.section->bg_b,
        (int)g_game_info.section->bg_a);

    g_game_info.field_34 = 0.0f;

    art_name = g_game_info.section->sky_name;
    if (art_name != 0 && art_name[0] != 0) {
        g_game_info.sky =
            (MkObj*)load_named_model_from_slot(art_id, art_name, 0x201F, 0);
    }

    if (g_game_info.sky != 0) {
        mk_insert((MkHdr*)g_game_info.sky, &g_game_info.bgnd_obj->child_list);
    }

    apply_to_mklist(add_mkx_light_obj_to_bgnd_cleanup_list,
                    &weapon_trail_light_list);
    apply_to_mklist(add_mkx_light_obj_to_bgnd_cleanup_list,
                    &bgnd_light_list);
    apply_to_mklist(add_mkx_light_obj_to_bgnd_cleanup_list,
                    &plyr_light_list);

    if ((g_game_info.section->flags70 & 1) != 0) {
        UpdateShadowCameraLightSource(g_game_info.misc->shadow_cam_light);
    }

    data_table = g_game_info.section;
    ShadowStrength = g_game_info.misc->shadow_strength;
    fog_color_real[0] = data_table->fog_r / 255.0f;
    fog_color_real[1] = data_table->fog_g / 255.0f;
    fog_color_real[2] = data_table->fog_b / 255.0f;
    fog_color_real[3] = data_table->fog_a / 255.0f;

    RwCameraSetNearClipPlane(Camera, data_table->near_clip);
    RwCameraSetFarClipPlane(Camera, g_game_info.section->far_clip_cam);
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
        RwCameraSetNearClipPlane(Camera, g_game_info.section->near_clip);
        RwCameraSetFarClipPlane(Camera, g_game_info.section->far_clip_cam);
    }

    g_game_info.bgnd_id = bgnd_id;
    ctx.art_id = art_id;
    ctx.bgnd_obj = g_game_info.bgnd_obj;
    ctx.pad = 0;
    active_cmdscript->mko = g_game_info.cmdscript;
    g_game_info.cmdscript->load_ctx = &ctx;

    effect_list = g_game_info.section->effect_banks;
    if (effect_list != 0) {
        for (i = 0; effect_list[i] != 0; i++) {
            load_effect_bank(effect_list[i]);
        }
    }

    g_game_info.cmdscript->load_ctx = 0;

    if (g_game_info.section->load_script != 0) {
        cmdscript_setup_execution(g_game_info.cmdscript, g_game_info.section->load_script);
        cmdscript_execute(g_game_info.cmdscript);
    }

    g_game_info.wall_hider = 0;
    if (g_game_info.misc->script != 0) {
        cmdscript_setup_execution(g_game_info.cmdscript, g_game_info.misc->script);
        cmdscript_execute(g_game_info.cmdscript);
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
