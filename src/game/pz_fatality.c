#include "math/gxVect.h"
#include "math/mk_math.h"
#include "game/pz_fatality.h"
#include "runtime/cam_api.h"
#include "runtime/asset.h"
#include "runtime/cstring.h"
#include "rw/rwframe.h"

typedef float (*PuzzleFatalityPreroundFn)(void);
typedef float (*PuzzleFatalityActiveFn)(int active);
typedef void (*PuzzleFatalityStartFn)(int attacker, int victim);

typedef struct PuzzleFatalityPreroundTable {
    PuzzleFatalityPreroundFn entries[10];
} PuzzleFatalityPreroundTable;
typedef struct PuzzleFatalityActiveTable {
    PuzzleFatalityActiveFn entries[10];
} PuzzleFatalityActiveTable;
typedef struct PuzzleFatalityStartTable {
    PuzzleFatalityStartFn entries[10];
} PuzzleFatalityStartTable;
typedef struct PuzzleFatalityProcessTable {
    PuzzleFatalityProcessFn entries[10];
} PuzzleFatalityProcessTable;

typedef struct AniScript AniScript;
typedef struct PuzzleAnimPdata PuzzleAnimPdata;
typedef struct MkFileInfo MkFileInfo;
typedef struct PuzzleParticleEffect PuzzleParticleEffect;
typedef struct PuzzleFighterRenderObject PuzzleFighterRenderObject;
typedef struct PuzzleObjectVtable PuzzleObjectVtable;
typedef struct RpMaterial RpMaterial;
typedef struct RwTexture RwTexture;
typedef struct AniTextureControl AniTextureControl;
typedef struct PuzzleObjectFlags {
    unsigned char bit7 : 1;
    unsigned char airborne : 1;
    unsigned char gravity_enabled : 1;
    unsigned char transform_dirty : 1;
    unsigned char angular_velocity_enabled : 1;
    unsigned char rotation_enabled : 1;
    unsigned char scale_active : 1;
    unsigned char moving : 1;
} PuzzleObjectFlags;

typedef struct PuzzleObjectSecondaryFlags {
    unsigned char stopped : 1;
    unsigned char pad_low : 7;
} PuzzleObjectSecondaryFlags;

typedef struct PuzzleSobjMaterialData {
    char pad00[0x18];
    void* geometry; /* +0x18 */
} PuzzleSobjMaterialData;

typedef struct PuzzleProcessVtable {
    char pad00[0x18];
    void (*sleep)(struct PuzzleProcessVtable* vtable); /* +0x18 */
    char pad1C[8];
    float (*transfer)(PuzzleFatalityProcessFn entry, float delay);
} PuzzleProcessVtable;

typedef struct PuzzleProcess {
    PuzzleProcessVtable* vtbl;
} PuzzleProcess;

typedef struct PuzzleSharedAnimations {
    char pad00[0x1C];
    AniScript* solid_kick; /* +0x1C */
    char pad20[0x30];
    AniScript* push_into_grinder; /* +0x50 */
    char pad54[0x3C];
    AniScript* snake_eaten_start; /* +0x90 */
    char pad94[0x0C];
    AniScript* snake_idle; /* +0xA0 */
    AniScript* snake_lunge; /* +0xA4 */
    AniScript* snake_bite; /* +0xA8 */
    AniScript* snake_attacker; /* +0xAC */
    AniScript* snake_victim; /* +0xB0 */
    AniScript* burn_hurt; /* +0xB4 */
    AniScript* burn_loop; /* +0xB8 */
    AniScript* burn_recover; /* +0xBC */
    AniScript* burn_summon; /* +0xC0 */
    AniScript* burn_idle; /* +0xC4 */
    char padC8[0x40];
    AniScript* snake_eaten_end; /* +0x108 */
    char pad10C[0x60];
    AniScript* head_poked; /* +0x16C */
    char pad170[0x28];
    AniScript* burn_attacker_start; /* +0x198 */
    AniScript* burn_attacker_end; /* +0x19C */
    char pad1A0[0x7C];
    AniScript* objects_falling_crushed; /* +0x21C */
} PuzzleSharedAnimations;

typedef struct PuzzleBaseAnimations {
    char pad00[4];
    AniScript* walk_forward; /* +0x04 */
    char pad08[0x1BC];
    AniScript* fatality_bounce; /* +0x1C4 */
    char pad1C8[0x15C];
    AniScript* lightning_electrocution; /* +0x324 */
} PuzzleBaseAnimations;

struct PuzzleAnimPdata {
    char pad00[0x64];
    float animation_step; /* +0x64 */
};

typedef struct PuzzleFightersEngine {
    float balance; /* +0x000 */
    char pad004[0x3C];
    Vec fighter_posts[2]; /* +0x040 - home/grinder posts */
    char pad058[0x0C];
    PuzzleFatalityRandomEvent random_event; /* +0x064 */
    char pad088[0x94];
    int fatality_abort; /* +0x11C */
    int fatality_active; /* +0x120 */
    int fatality_ready; /* +0x124 */
    int fatality_victim; /* +0x128 */
    int fatality_attacker; /* +0x12C */
    PuzzleFighterRenderObject* grinder_meat_alt2; /* +0x130 */
    PuzzleFighterRenderObject* grinder_meat_final; /* +0x134 */
    PuzzleFighterRenderObject* fatality_piece; /* +0x138 */
    PuzzleFighterRenderObject* grinder_meat_alt0; /* +0x13C */
    PuzzleFighterRenderObject* grinder_meat_default; /* +0x140 */
    PuzzleFighterRenderObject* grinder_meat_alt1; /* +0x144 */
    PuzzleFighterRenderObject* left_eye; /* +0x148 */
    PuzzleFighterRenderObject* right_eye; /* +0x14C */
    char pad150[0x0C];
    int fatality_index; /* +0x15C */
    unsigned int random_event_cooldown; /* +0x160 */
    int fighter_reaction_cooldown; /* +0x164 */
    char pad168[0x10];
    unsigned int fatality_timer; /* +0x178 */
    float fatality_motion; /* +0x17C */
    char pad180[0x64];
    int snake_active; /* +0x1E4 */
} PuzzleFightersEngine;

struct PuzzleObjectVtable {
    char pad00[0x10];
    void (*destroy)(
        PuzzleFighterRenderObject* object, PuzzleObjectVtable* vtable);
};

struct PuzzleFighterRenderObject {
    PuzzleObjectVtable* vtbl; /* +0x00 */
    unsigned int instance; /* +0x04 */
    union {
        unsigned char flags; /* +0x08 */
        PuzzleObjectFlags flags_bits;
    };
    union {
        unsigned char secondary_flags; /* +0x09 */
        PuzzleObjectSecondaryFlags secondary_flags_bits;
    };
    char pad0A[0x16];
    void* frame; /* +0x20 */
    char pad24[8];
    int model_flags; /* +0x2C */
    float hazard_x; /* +0x30 */
    float hazard_y; /* +0x34 */
    float hazard_z; /* +0x38 */
    char pad3C[0x64];
    union {
        Vec position; /* +0xA0 */
        struct {
            float x;
            float y;
            float z;
        };
    };
    char padAC[4];
    float external_force_x; /* +0xB0 */
    float external_force_y; /* +0xB4 */
    float external_force_z; /* +0xB8 */
    char padBC[0x18];
    float facing_angle; /* +0xD4 */
    char padD8[8];
    union {
        Vec angular_velocity; /* +0xE0 */
        struct {
            float angular_velocity_x;
            float motion_rate; /* +0xE4 */
            float angular_velocity_z;
        };
    };
    char padEC[4];
    Vec scale; /* +0xF0 */
};

typedef struct PuzzleFighterMove {
    char pad00[0x20];
    int active; /* +0x20 */
} PuzzleFighterMove;

typedef struct PuzzlePlayerData {
    char pad00[0x1D0];
    int side; /* +0x1D0 */
    char pad1D4[0x70];
    int transition_locked; /* +0x244 */
    char pad248[0x4B4];
    int fatality_shove_active; /* +0x6FC */
} PuzzlePlayerData;

typedef struct PuzzleGameInfo {
    char pad00[0xFC];
    PuzzlePlayerData* player1; /* +0xFC */
    void* player1_physics; /* +0x100 */
    char pad104[0x64];
    PuzzlePlayerData* player2; /* +0x168 */
    void* player2_physics; /* +0x16C */
} PuzzleGameInfo;

typedef struct PuzzleFighterPhysics {
    char pad00[0x0A];
    unsigned char flags_0A;
    char pad0B[0xC9];
    float facing_angle;
} PuzzleFighterPhysics;

typedef struct PuzzleBoneData {
    char pad00[0x40];
    MKMATRIX* matrix;
} PuzzleBoneData;

typedef struct PuzzleBoneObjectView {
    char pad00[0x48];
    PuzzleBoneData* bone_data;
} PuzzleBoneObjectView;

typedef struct PuzzleFatalityHazardObject {
    char pad00[8];
    union {
        unsigned char flags; /* +0x08 */
        PuzzleObjectFlags flags_bits;
    };
    char pad09[0x0B];
    PuzzleSobjMaterialData* material_data; /* +0x14 */
    char pad18[0x10];
    float field_28; /* +0x28 - priority/depth control */
    int model_flags; /* +0x2C */
    float x; /* +0x30 */
    float y; /* +0x34 */
    float z; /* +0x38 */
    char pad3C[8];
    float motion; /* +0x44 */
    char pad48[0x28];
    Vec scale; /* +0x70 */
} PuzzleFatalityHazardObject;

typedef struct PuzzleFatalityHazardGroup {
    PuzzleFatalityHazardObject* objects[4]; /* +0x00 */
    char pad10[8];
} PuzzleFatalityHazardGroup; /* 0x18 */

typedef struct PuzzleFatalityEngine {
    union {
        struct {
            PuzzleFighterRenderObject* primary_object;
            PuzzleFighterRenderObject* secondary_object;
        };
        PuzzleFighterRenderObject* scene_objects[2];
    };
    PuzzleFatalityHazardGroup hazard_groups[2]; /* +0x08 */
    struct PuzzleFatalityController* controller; /* +0x38 */
    int active_effect; /* +0x3C */
    RwTexture* grinder_texture; /* +0x40 */
    char pad44[0x14];
    int effect_timer; /* +0x58 */
} PuzzleFatalityEngine; /* 0x5C */

typedef struct PuzzleFatalityController {
    char pad00[8];
    int unload_requested; /* +0x08 */
    int state; /* +0x0C */
    int substate; /* +0x10 */
    int active; /* +0x14 */
    int phase; /* +0x18 */
    int preround_active; /* +0x1C */
    int controller_step; /* +0x20 */
    int attacker_player; /* +0x24 */
    int victim_player; /* +0x28 */
    unsigned int preround_timer; /* +0x2C */
    unsigned int loop_sound; /* +0x30 */
    float phase_time; /* +0x34 */
    unsigned int preround_sound_started; /* +0x38 */
    float grinder_position[2]; /* +0x3C */
    float grinder_target[2]; /* +0x44 */
    float chomper_position[2][2]; /* +0x4C */
    float hazard_motion[2][2]; /* +0x5C */
    unsigned int hazard_initialized[3]; /* +0x6C */
    PuzzleAnimPdata* fighter_pdata[2]; /* +0x78 */
} PuzzleFatalityController;

typedef struct PuzzleGrinderMeatController {
    char pad00[8];
    unsigned int direction; /* +0x08 */
    unsigned int delay; /* +0x0C */
    int phase; /* +0x10 */
    PuzzleFighterRenderObject* object; /* +0x14 */
} PuzzleGrinderMeatController;

typedef struct PuzzleGrinderNoisePdata {
    char pad00[8];
    unsigned int duration; /* +0x08 */
    unsigned int timer; /* +0x0C */
} PuzzleGrinderNoisePdata;

typedef struct PuzzleFaceBleedPdata {
    char pad00[8];
    unsigned int duration; /* +0x08 */
    unsigned int interval; /* +0x0C */
    unsigned int state; /* +0x10 */
    PuzzleFighterRenderObject* object; /* +0x14 */
    PuzzlePlayerData* player_data; /* +0x18 */
} PuzzleFaceBleedPdata;

typedef struct PuzzleFleshchunkPdata {
    char pad00[8];
    PuzzleFighterRenderObject* object; /* +0x08 */
    unsigned int object_instance; /* +0x0C */
    int bounce_count; /* +0x10 */
    Vec initial_velocity; /* +0x14 */
    float gravity; /* +0x20 */
    int (*completion_callback)(void); /* +0x24 */
    char pad28[8];
    int field_30; /* +0x30 */
    float bounce_decay; /* +0x34 */
    float floor_height; /* +0x38 */
} PuzzleFleshchunkPdata;

typedef struct PuzzleFaceBleedProcess {
    char pad00[0xB0];
    void (*wait_routine)(void); /* +0xB0 */
    void (*script_routine)(void); /* +0xB4 */
} PuzzleFaceBleedProcess;

typedef struct PuzzleEffectBankContext {
    int art_handle;
    void* owner;
    void* context;
} PuzzleEffectBankContext;

typedef struct PuzzleParticleEmitter {
    char pad00[0x1C];
    union {
        unsigned char flags;
        struct {
            unsigned char hidden : 1;
            unsigned char flags_low : 7;
        } flags_bits;
    }; /* +0x1C */
} PuzzleParticleEmitter;

struct PuzzleParticleEffect {
    char pad00[0x40];
    unsigned char emitters[1]; /* +0x40 */
};

typedef struct PuzzleFatalityDefinition {
    int flags;
    float (*load_and_place)(void);
    int threshold;
} PuzzleFatalityDefinition;

typedef struct PuzzleAmbientLightDefinition {
    int type;
    float (*proc)(void);
    int flags;
    float color[4];
} PuzzleAmbientLightDefinition; /* 0x1C */

typedef struct PuzzleDirectLightDefinition {
    int type;
    float (*proc)(void);
    int flags;
    float color[4];
    float angle_y;
    float angle_x;
    float angle_z;
} PuzzleDirectLightDefinition; /* 0x28 */

extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleSharedAnimations pz_shared_ani;
extern PuzzleBaseAnimations shared_ani;
extern PuzzleProcess* aproc;
extern PuzzleProcess* plyr_anim_proc;
extern PuzzleAnimPdata* plyr_anim_pdata;
extern PuzzleGameInfo g_game_info;
extern int screen_width;
extern float _mkproc_sleep_ticks;
extern MkFileInfo sec_pz_danger_lightning;
extern MkFileInfo sec_pz_danger_1ton;
extern MkFileInfo sec_pz_danger_grinder;
extern MkFileInfo sec_pz_danger_burn;
extern MkFileInfo sec_pz_danger_crusher;
extern MkFileInfo sec_pz_danger_chomper;
extern MkFileInfo sec_pz_danger_snake;
extern void* apdata;

PuzzleFatalityEngine g_pz_fighter_fatality_engine;
extern PuzzlePlayerData* plyr_pdata;
extern PuzzlePlayerData* his_pdata;
extern PuzzleFighterRenderObject* plyr_obj;

double pow(double base, double exponent);
unsigned int randu0(unsigned int max);
int random_snd_req(int sound);
int pz_fighter_close_enough_to_super_move(unsigned int player);
float pz_fighter_attempt_push_into_grinder(void);
void pz_fighter_attack(AniScript* animation, PuzzleAttackParameters* attack,
                       int move);
void ani_to_end(void);
float pz_fighter_long_exit(void);
void unload_pz_fighter_fatality_banks(void);
int snd_req_vol(int sound, float volume);
int snd_req(int sound);
void snd_req_delay(int sound, int delay);
void set_snd_vol(int handle, int sound, float volume);
void snd_stop(int handle);
void* fx_by_owner(const char* name, int owner);
void fx_pause_emit(void* effect);
PuzzleFighterRenderObject* pz_fighter_get_player_obj(int player);
PuzzleProcess* pz_fighter_get_player_proc(int player);
PuzzleFighterMove* pz_get_fighter_move(void);
PuzzlePlayerData* pz_get_pdata_by_id(int player);
void xfer_proc(PuzzleProcess* process, PuzzleFatalityProcessFn entry);
void get_bone_world_pos(
    PuzzleFighterRenderObject* object, int bone, Vec* position);
void minigame_event(int* event);
float xz_distance_between_players(void);
int pan_snd_req(int sound, float pan);
int pan_vol_snd_req(int sound, float pan, float volume);
int plyr_snd_req(int sound);
void* bgnd_launch_fx_at_position(const char* name, float x, float y, float z);
void bgnd_set_fx_ang_y(float angle);
void snd_major_hit_voice(void);
void snd_death_voice(void);
void random_hit(int group);
void plyr_bleed_mouth(PuzzlePlayerData* player_data);
void face_bleed_me(int amount);
void mkproc_die(void);
void face_opponent_now(void);
void stop_me(void);
void init_air_move(void);
void init_ground_move(void);
void init_ground_move_no_aniproc(void);
void blend_to_ani(AniScript* animation, int frames, float blend);
void blend_to_ani_frame(
    AniScript* animation, int frames, float blend, float frame);
void glitch_to_ani(AniScript* animation, int flags);
void set_ani_speed(float speed);
void ani_to_frame_x(float frame);
void ani_to_blend_frame(float frame);
void ani_loop_more_frames(float frames);
void ani_1_frame(void);
void set_my_state(int state);
void shake_hit_voice(float strength, int flags, int voice, int group);
float pz_fighter_inline_force_away_with_ani(
    float velocity, unsigned int coast_ticks, float damping,
    unsigned int damping_ticks);
void head_tracking_off(void);
void force_forward(float force, int duration, float damping, int animation);
void slow_ani_x(float speed, float frame);
void obj_set_bone_collapse_flag(
    PuzzleFighterRenderObject* object, int bone);
void calc_bone_world_mat(PuzzleFighterRenderObject* object, int bone);
void pz_fighter_clear_out_external_forces(void);
void random_voice(int group);
void bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect, PuzzleFighterRenderObject* object, int bone);
void resume_effect_at_obj_bid(
    PuzzleFighterRenderObject* object, int bone, void* effect, int active,
    int flags);
void pz_fighter_walk_FB_true(
    int (*continue_test)(void), unsigned int duration, int forward);
void unhide_obj(PuzzleFighterRenderObject* object);
void hide_sobj(PuzzleFatalityHazardObject* object);
RpMaterial* sobj_find_material_with_texture(
    PuzzleFatalityHazardObject* object, const char* texture);
void RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);

float sfrand(float range);
float frand(float range);
void spawn_bld_fall(
    const char* effect, int flags, float* position, Vec* velocity,
    PuzzlePlayerData* player_data);
static PuzzleFleshchunkPdata* ft_create_flesh_path(
    PuzzleFighterRenderObject* object, Vec* position, int active,
    int flags,
    const Vec* initial_velocity, int mode, const Vec* terminal_velocity,
    int (*completion_callback)(void), float gravity, float bounce,
    float scale);
PuzzleFaceBleedProcess* _create_mkproc_generic_nostack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    PuzzleFleshchunkPdata** process_data);
void zero_pdata_payload(int size, void* process_data);
float sfrand_ab(float minimum, float maximum);
PuzzleFaceBleedProcess* _create_mkproc_generic_bigstack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    PuzzleFaceBleedPdata** process_data);
static float p_face_bleeding(void);
static void pw_face_bleeding(void);
static void ps_face_bleeding(void);
static int dropped_heart_snd_cb(void);
float pz_fighter_completely_prone(void);
float p_anim_idle(void);
float p_plyr_pz_fighter_entry(void);
float r_pz_fighter_grinding(void);
static float r_pz_fighter_burn(void);
static float r_pz_fighter_summon_burn(void);
float pz_fighter_disgusted_with_grinding(void);
static float pz_fighter_fatality_good_solid_kick(void);
static float pz_fighter_fatality_medium_shove(void);
static float pz_fighter_fatality_huge_shove(void);
float pz_fighter_dash_back(void);
float pz_fighter_shove(void);
float pz_fighter_execute_point_no_space_check(void);
float pz_fighter_execute_point_reaction_no_space(void);
float pz_fighter_execute_R_coming_down(void);
float pz_fighter_backflip_and_point(void);
float pz_fighter_execute_point(void);
float pz_fighter_exit(void);
float pz_fighter_perform_taunt(void);
static float r_pz_fighter_eaten(void);
float pz_fighter_just_backflip(void);
static float pz_fighter_walk_forward(void);
static float pz_fighter_fatality_victim_to_exact_spot(void);
static float pz_fighter_lightning_strike_victim_1(void);
static float pz_fighter_objects_falling_victim_crushed(void);
static float pz_fighter_chomper2_victim_crushed(void);
static float pz_fighter_chomper_victim_crushed(void);
static float pz_fighter_victim_head_poked(void);
static float pz_fighter_victim_fatality_bouncy(void);
float pz_fighter_one_arm_victory2(void);
float pz_fighter_wipe_blood_off(void);
float pz_fighter_won2(void);
static void pz_fighter_fatality_launch_eyes(void);
void pz_fighter_shake_camera(int duration, float strength);
static void pz_fighters_fatality_bird_in_place(int x, int y);
void pz_fighter_anim_object_to(
    unsigned int player, int mirror, int frame, Vec* start,
    Vec* target, Vec* velocity, int minimum_velocity_y,
    unsigned int target_ticks, float frame_rate, float gravity_step,
    int bounce, void (*arrival)(int x, int y));
void unhide_sobj(PuzzleFatalityHazardObject* object);
int transition_to_anim_script_frame(
    float transition_frames, float frame, PuzzleAnimPdata* pdata,
    AniScript* animation, unsigned int flags);
void set_pdata_anim_step(PuzzleAnimPdata* pdata, float step);
void pz_fighter_clear_out_all_external_forces(void* fighter);
static float p_grinder_meat_throw_controller(void);
static float p_grinder_noise(void);
void* _create_mkproc_generic_tinystack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    void* pdata);
void load_art_section(int handle, MkFileInfo* section);
void load_effect_bank_with_context(
    const char* bank, PuzzleEffectBankContext* context);
AniTextureControl* replace_sobj_texture_with_named_wiff(
    PuzzleFatalityHazardObject* object, int handle, const char* texture,
    const char* wiff);
void set_ani_texture_framerate(AniTextureControl* texture, float rate);
void set_ani_texture_frame(AniTextureControl* texture, int frame);
void fx_set_param_v3(
    void* effect, int parameter, float x, float y, float z);
void fx_set_render_priority(void* effect, int priority);
MKMATRIX* force_calc_bone_world_mat(
    PuzzleFighterRenderObject* object, int bone);
void* RpGeometryForAllMaterials(
    void* geometry, RpMaterial* (*callback)(RpMaterial*, RwTexture*),
    RwTexture* texture);
static RpMaterial* material_set_texture(
    RpMaterial* material, RwTexture* texture);
static int pz_fighter_always_continue(void);
static float p_ft_bounce_path(void);
static void ft_fleshchunk_postsleep(void);
static void ft_fleshchunk_prewake(void);
void freeze_player(void);
void unfreeze_player(void);
void got_hit_fx(
    int strength, int bone, int blood, int unused1, int unused2, int active,
    float scale);
void hide_obj(PuzzleFighterRenderObject* object);
void insert_fgnd_mkobj(PuzzleFighterRenderObject* object);
void obj_set_pos(PuzzleFighterRenderObject* object, Vec* position);
void update_mkobj(PuzzleFighterRenderObject* object);
static void pz_fighter_set_objects_falling_obj(
    PuzzleFighterRenderObject* object1,
    PuzzleFighterRenderObject* object2);
void load_pz_fighter_fatality_bank(int bank);
static float p_lightning_controller(void);
static float p_objects_falling_controller2(void);
static float p_grinder_controller(void);
static float p_burn_controller(void);
static float p_chomper2_controller(void);
static float p_chomper_controller(void);
static float p_snake_controller(void);
void obj_create_sobjs(PuzzleFighterRenderObject* object);
PuzzleFatalityHazardObject* obj_find_sobj_by_id(
    PuzzleFighterRenderObject* object, int id);
void obj_change_to_skinned_obj_light_list(
    PuzzleFighterRenderObject* object, void* light_definition);
void obj_add_to_skinned_obj_light_list_with_ambient(
    PuzzleFighterRenderObject* object, void* ambient_definition);
PuzzleAnimPdata* animate_obj(
    PuzzleFighterRenderObject* object, AniScript* animation, float speed,
    void* light_data, int loop_start, int loop_end, int active);
PuzzleFatalityHazardObject* obj_first_sobj(
    PuzzleFighterRenderObject* object);
void sobj_set_priority(PuzzleFatalityHazardObject* object, int priority);
void fx_reset(void* effect);
void fx_resume_emit(void* effect);
PuzzleParticleEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(PuzzleParticleEffect* effect);
void pfx_bind_emitter_to_obj_bone(
    PuzzleParticleEffect* effect, PuzzleFighterRenderObject* object, int bone);
PuzzleParticleEmitter* pfx_get_emitter(void* emitters, int index);

static void pz_fighter_grinder_entering_fatality(
    int attacker, int victim);
static void pz_fighter_chomper_entering_fatality(
    int attacker, int victim);
static void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim);
static void pz_fighter_objects_falling_entering_fatality(
    int attacker, int victim);
/* Exact match. */
static void pz_fighter_lightning_entering_fatality(
    int attacker, int victim);
static void pz_fighter_snake_entering_fatality(
    int attacker, int victim);
static void pz_fighter_burn_entering_fatality(
    int attacker, int victim);

static float pz_fighter_grinder_unload(void);
static float pz_fighter_chomper_unload(void);
static float pz_fighter_chomper2_unload(void);
static float pz_fighter_objects_falling_unload(void);
static float pz_fighter_lightning_unload(void);
static float pz_fighter_snake_unload(void);
static float pz_fighter_burn_unload(void);

static float pz_fighter_grinder_actively_fighting(int active);
static float pz_fighter_chomper_actively_fighting(int active);
static float pz_fighter_chomper2_actively_fighting(int active);
static float pz_fighter_objects_falling_actively_fighting(int active);
static float pz_fighter_lightning_actively_fighting(int active);
static float pz_fighter_snake_actively_fighting(int active);
static float pz_fighter_burn_actively_fighting(int active);

static float pz_fighter_grinder_round_over(void);
static float pz_fighter_chomper_round_over(void);
static float pz_fighter_chomper2_round_over(void);
static float pz_fighter_objects_falling_round_over(void);
static float pz_fighter_lightning_round_over(void);
static float pz_fighter_snake_round_over(void);
static float pz_fighter_burn_round_over(void);

static float pz_fighters_grinder_fatality_in_progress(void);
static float pz_fighters_chomper_fatality_in_progress(void);
static float pz_fighters_chomper2_fatality_in_progress(void);
static float pz_fighters_objects_falling_fatality_in_progress(void);
static float pz_fighters_lightning_fatality_in_progress(void);
static float pz_fighters_snake_fatality_in_progress(void);
static float pz_fighters_burn_fatality_in_progress(void);

static float pz_fighters_grinder_fatality_prep(void);
static float pz_fighters_chomper_fatality_prep(void);
static float pz_fighters_chomper2_fatality_prep(void);
static float pz_fighters_objects_falling_fatality_prep(void);
static float pz_fighters_lightning_fatality_prep(void);
static float pz_fighters_snake_fatality_prep(void);
static float pz_fighters_burn_fatality_prep(void);

static float pz_fighters_grinder_fatality_preround(void);
static float pz_fighters_chomper_preround(void);
static float pz_fighters_chomper2_preround(void);
static float pz_fighters_objects_falling_preround(void);
static float pz_fighters_lightning_preround(void);
static float pz_fighters_snake_fatality_preround(void);
static float pz_fighters_burn_fatality_preround(void);

float p_track_cam_ang_y_light(void);
static float pz_fighter_load_and_place_initial_grinders(void);
static float pz_fighter_load_and_place_initial_chompers(void);
static float pz_fighter_load_and_place_initial_chompers2(void);
static float pz_fighter_load_and_place_initial_objects_falling(void);
static float pz_fighter_load_and_place_initial_lightning(void);
static float pz_fighter_load_and_place_initial_snake(void);
static float pz_fighter_load_and_place_initial_burn(void);

PuzzleDirectLightDefinition skinned_obj_light_def = {
    3, p_track_cam_ang_y_light, 1,
    {1.0f, 1.0f, 1.0f, 1.0f},
    6.010f, 3.190f, 0.0f,
};

PuzzleAmbientLightDefinition skinned_obj_ambient_light_def = {
    1, 0, 3, {0.2f, 0.2f, 0.2f, 1.0f},
};

int pz_snake_bones[9] = {1, 2, 3, 4, 5, 6, 7, 8, 0};

PuzzleFatalityDefinition g_fatalityTable[15] = {
    {3, pz_fighter_load_and_place_initial_grinders, 20},
    {0, pz_fighter_load_and_place_initial_chompers, 40},
    {0, pz_fighter_load_and_place_initial_chompers2, 60},
    {0, pz_fighter_load_and_place_initial_objects_falling, 80},
    {0, pz_fighter_load_and_place_initial_lightning, 90},
    {0, pz_fighter_load_and_place_initial_snake, 95},
    {0, pz_fighter_load_and_place_initial_burn, 100},
};

/*
 * Soft ceiling: retail keeps a redundant positive branch plus a shared-return
 * branch; clean C emits one inverse branch to the same zero result. Callers
 * pass the engine and mode even though retail reads the canonical global.
 */
int pz_fighter_check_fatality_random_event(
    PuzzleFightersEngine* caller_engine, int force) {
    PuzzleFightersEngine* engine = &g_pz_fighters_engine;
    PuzzleFatalityRandomEvent* event = &engine->random_event;

    if (engine->random_event_cooldown != 0) {
        engine->random_event_cooldown = 0;
    }
    if (pz_fighter_close_enough_to_super_move(0) == 1) {
        return 0;
    }
    if (pz_fighter_close_enough_to_super_move(1) == 1) {
        return 0;
    }

    event->state = 3;
    if (g_pz_fighters_engine.fatality_index == 0 &&
        engine->random_event_cooldown == 0) {
        if (g_pz_fighters_engine.balance < -0.65f) {
            event->active = 1;
            event->side = 0;
            pz_fighter_process_random_fatality_event(
                event, pz_fighter_attempt_push_into_grinder);
            engine->random_event_cooldown =
                (randu0(120) & 0xFFFF) + 180;
            return 1;
        }
        if (g_pz_fighters_engine.balance > 0.65f) {
            event->active = 1;
            event->side = 1;
            pz_fighter_process_random_fatality_event(
                event, pz_fighter_attempt_push_into_grinder);
            engine->random_event_cooldown =
                (randu0(120) & 0xFFFF) + 180;
            return 1;
        }
    }
    return 0;
}

int pz_fighter_fatality_during_round_stuff_over(void) {
    return 1;
}

/*
 * Dispatch-wrapper family: automatic initialized tables recover retail's
 * exact 0x68-byte bodies. Six wrappers are 99.04% with GPR allocation residue;
 * preround is 96.92% with the same residue plus an equivalent table-base
 * adjustment. Function order, signatures, calls, and table contents agree.
 */
void pz_fighters_fatality_preround_event(void) {
    PuzzleFatalityPreroundTable functions = {
        pz_fighters_grinder_fatality_preround,
        pz_fighters_chomper_preround,
        pz_fighters_chomper2_preround,
        pz_fighters_objects_falling_preround,
        pz_fighters_lightning_preround,
        pz_fighters_snake_fatality_preround,
        pz_fighters_burn_fatality_preround,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_prep_chores(void) {
    PuzzleFatalityProcessTable functions = {
        pz_fighters_grinder_fatality_prep,
        pz_fighters_chomper_fatality_prep,
        pz_fighters_chomper2_fatality_prep,
        pz_fighters_objects_falling_fatality_prep,
        pz_fighters_lightning_fatality_prep,
        pz_fighters_snake_fatality_prep,
        pz_fighters_burn_fatality_prep,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_in_progress(void) {
    PuzzleFatalityProcessTable functions = {
        pz_fighters_grinder_fatality_in_progress,
        pz_fighters_chomper_fatality_in_progress,
        pz_fighters_chomper2_fatality_in_progress,
        pz_fighters_objects_falling_fatality_in_progress,
        pz_fighters_lightning_fatality_in_progress,
        pz_fighters_snake_fatality_in_progress,
        pz_fighters_burn_fatality_in_progress,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighter_load_place_fatality_elements(int fatality) {
    g_pz_fighter_fatality_engine.primary_object = 0;
    g_pz_fighter_fatality_engine.secondary_object = 0;
    g_fatalityTable[fatality].load_and_place();
}

void pz_fighters_fatality_round_over(void) {
    PuzzleFatalityProcessTable functions = {
        pz_fighter_grinder_round_over,
        pz_fighter_chomper_round_over,
        pz_fighter_chomper2_round_over,
        pz_fighter_objects_falling_round_over,
        pz_fighter_lightning_round_over,
        pz_fighter_snake_round_over,
        pz_fighter_burn_round_over,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_normal_fighting(int enabled) {
    PuzzleFatalityActiveTable functions = {
        pz_fighter_grinder_actively_fighting,
        pz_fighter_chomper_actively_fighting,
        pz_fighter_chomper2_actively_fighting,
        pz_fighter_objects_falling_actively_fighting,
        pz_fighter_lightning_actively_fighting,
        pz_fighter_snake_actively_fighting,
        pz_fighter_burn_actively_fighting,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index](enabled);
}

void pz_fighters_fatality_unload(void) {
    PuzzleFatalityProcessTable functions = {
        pz_fighter_grinder_unload,
        pz_fighter_chomper_unload,
        pz_fighter_chomper2_unload,
        pz_fighter_objects_falling_unload,
        pz_fighter_lightning_unload,
        pz_fighter_snake_unload,
        pz_fighter_burn_unload,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_start(int attacker, int victim) {
    PuzzleFatalityStartTable functions = {
        pz_fighter_grinder_entering_fatality,
        pz_fighter_chomper_entering_fatality,
        pz_fighter_chomper2_entering_fatality,
        pz_fighter_objects_falling_entering_fatality,
        pz_fighter_lightning_entering_fatality,
        pz_fighter_snake_entering_fatality,
        pz_fighter_burn_entering_fatality,
        0, 0, 0,
    };

    functions.entries[g_pz_fighters_engine.fatality_index](attacker, victim);
}

static void pz_fighter_burn_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

static float pz_fighter_burn_unload(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_burn_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

/*
 * Soft ceiling: retail selects a different pooled-string base, saving one
 * address adjustment; the remaining body differences are register allocation,
 * float scheduling, and relocation labels.
 */
static float pz_fighter_load_and_place_initial_burn(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* burners[2];
    PuzzleFatalityController* controller;
    float effect_x;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_burn);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_burn_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 6;

    for (i = 0; i < 2; i++) {
        burners[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08240000, 0x6021);
        burners[i]->model_flags = 1;
        burners[i]->x = i != 0 ? 2.25f : -2.25f;
        if (screen_width > 650) {
            burners[i]->x = i != 0 ? 2.65f : -2.65f;
        }
        burners[i]->y = 0.0f;
        burners[i]->z = 0.0f;
        burners[i]->flags_bits.scale_active = 1;
        burners[i]->scale.x = 0.3f;
        burners[i]->scale.y = 0.3f;
        burners[i]->scale.z = 0.3f;
        insert_fgnd_mkobj(burners[i]);
    }

    g_pz_fighter_fatality_engine.primary_object = burners[0];
    g_pz_fighter_fatality_engine.secondary_object = burners[1];
    obj_create_sobjs(burners[0]);
    obj_create_sobjs(burners[1]);

    controller = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_burn_controller, 0x80, &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->loop_sound = 0;
        controller->hazard_initialized[0] = 0;
        controller->hazard_initialized[1] = 0;
        g_pz_fighter_fatality_engine.controller = controller;
    }

    load_pz_fighter_fatality_bank(0x84);
    effect_x = screen_width > 650 ? -2.3f : -1.8f;
    bgnd_launch_fx_at_position(
        "idle_flames1", effect_x - 0.15f, 0.0f, 0.0f);
    effect_x = screen_width > 650 ? 2.3f : 1.8f;
    bgnd_launch_fx_at_position(
        "idle_flames2", effect_x + 0.2f, 0.0f, 0.0f);
    pan_snd_req(0x1AB0, -0.7f);
    pan_snd_req(0x1AB0, 0.7f);
    return 0.0f;
}

/* Soft ceiling: pz_fighters_burn_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
static float pz_fighters_burn_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

static float pz_fighters_burn_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

/*
 * Emission-only near miss: 94.61%, exact retail 0x300 size. The state machine,
 * CFG, calls, comparisons, arithmetic, stores, signed-distance construction,
 * and attacker/victim ABI agree. Remaining differences are save/restore
 * selection, one GPR move, one FPR move, a reload, and a constant relocation.
 */
static float pz_fighters_burn_fatality_prep(void) {
    static int start_burn;
    static int attack_begun;
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterMove* victim_move;
    float target_x;
    float delta_x;
    float delta_z;
    float signed_distance;
    float effect_x;
    int direction;
    int event;
    int attacker;
    int victim;

    attacker = g_pz_fighters_engine.fatality_attacker;
    attacker_object = pz_fighter_get_player_obj(attacker);
    if (screen_width > 650) {
        target_x = attacker == 0 ? -2.3f : 2.3f;
    } else {
        target_x = attacker == 0 ? -1.8f : 1.8f;
    }

    delta_x = target_x - attacker_object->x;
    delta_z = 0.0f;
    delta_z -= attacker_object->z;
    direction = 1;
    if (target_x < 0.0f) {
        if (attacker_object->x < target_x) {
            direction = -1;
        }
    } else if (attacker_object->x > target_x) {
        direction = -1;
    }
    signed_distance =
        direction * ((delta_x * delta_x) + (delta_z * delta_z));

    if (start_burn == 0) {
        start_burn = 1;
        if (screen_width > 650) {
            effect_x = attacker == 0 ? -2.3f : 2.3f;
        } else {
            effect_x = attacker == 0 ? -1.8f : 1.8f;
        }
        if ((unsigned int)attacker == 0) {
            pan_snd_req(0x1AB2, -0.7f);
            effect_x -= 0.15f;
        } else {
            pan_snd_req(0x1AB2, 0.7f);
            effect_x += 0.2f;
        }
        bgnd_launch_fx_at_position(
            "super_roar_flames", effect_x, 0.0f, 0.0f);
    }

    if (signed_distance < 0.014f &&
        g_pz_fighters_engine.fatality_ready == 1) {
        if (attack_begun == 0) {
            event = 0;
            minigame_event(&event);
        }
        attack_begun = 0;
        start_burn = 0;
        g_pz_fighters_engine.fatality_active = 1;
        xfer_proc(pz_fighter_get_player_proc(attacker), r_pz_fighter_burn);
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_victim),
                  r_pz_fighter_summon_burn);
        g_pz_fighters_engine.fatality_timer = 138;
    } else if (g_game_info.player1->transition_locked == 0 &&
               g_game_info.player2->transition_locked == 0) {
        if (attack_begun == 0) {
            attack_begun = 1;
            event = 0;
            minigame_event(&event);
        }
        victim = g_pz_fighters_engine.fatality_victim;
        victim_move = pz_get_fighter_move();
        victim_move->active = 1;
        g_pz_fighters_engine.fighter_reaction_cooldown = 0;
        if (signed_distance < 0.19f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_good_solid_kick);
        } else if (signed_distance < 1.3f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_medium_shove);
        } else {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_huge_shove);
        }
    }
    return 0.0f;
}

/* Soft ceiling: r_pz_fighter_summon_burn ~94.35% - MWCC emit details; stop. */
static float r_pz_fighter_summon_burn(void) {
    int animation_flags = 3;

    pz_fighter_clear_out_external_forces();
    head_tracking_off();
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.burn_summon, 3, 0.1f);
    set_ani_speed(3.5f);
    ani_to_end();
    glitch_to_ani(pz_shared_ani.burn_idle, 0);
    set_ani_speed(1.0f);
    ani_loop_more_frames(175.0f);

    if (plyr_pdata->side == 1) {
        animation_flags |= 8;
    }
    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->secondary_flags &= (unsigned char)~2;
    blend_to_ani(
        pz_shared_ani.burn_attacker_start, animation_flags, 0.035f);
    set_ani_speed(0.5f);
    ani_to_frame_x(91.0f);

    animation_flags = 0;
    plyr_obj->secondary_flags &= (unsigned char)~2;
    if (plyr_pdata->side == 1) {
        animation_flags |= 8;
    }
    blend_to_ani(pz_shared_ani.burn_attacker_end, animation_flags, 0.5f);
    set_ani_speed(0.5f);
    ani_loop_more_frames(1000.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Exact match: victim ignition, looping burn, and bound limb effects. */
static float r_pz_fighter_burn(void) {
    int animation_flags = 0;

    if (plyr_pdata->side != 0) {
        animation_flags = 8;
    }
    head_tracking_off();
    pz_fighter_clear_out_external_forces();
    init_ground_move_no_aniproc();
    bgnd_launch_fx_at_bid_of_mkobj("limbburn_3", plyr_obj, 4);
    bgnd_launch_fx_at_bid_of_mkobj("limbburn_4", plyr_obj, 5);
    random_voice(14);
    snd_req(0x1AB3);
    blend_to_ani(
        pz_shared_ani.burn_hurt, animation_flags | 3, 0.1f);
    ani_to_end();
    bgnd_launch_fx_at_bid_of_mkobj("limbburn_1", plyr_obj, 20);
    bgnd_launch_fx_at_bid_of_mkobj("limbburn_2", plyr_obj, 21);
    snd_req(0x1AB4);
    glitch_to_ani(pz_shared_ani.burn_loop, animation_flags);
    ani_loop_more_frames(30.0f);
    snd_req(0x1AB5);
    blend_to_ani_frame(pz_shared_ani.burn_recover, 3, 0.05f, 37.0f);
    ani_to_blend_frame(100.0f);

    resume_effect_at_obj_bid(
        plyr_obj, 0x10, fx_by_owner("limbburn_5", 4), 1, 0);
    ani_to_blend_frame(50.0f);
    resume_effect_at_obj_bid(
        plyr_obj, 0x10, fx_by_owner("limbburn_6", 4), 1, 0);
    ani_to_end();

    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
}

static float pz_fighter_burn_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

/*
 * Genuine near match: exact 0x2C0 size and all 161 non-string instructions
 * match. The 15 delete/insert pairs are five equivalent three-instruction
 * pooled-string address sequences using different local relocation anchors.
 */
static float p_burn_controller(void) {
    unsigned short event;
    float x;

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        pan_snd_req(0x1AB6, -0.7f);
        x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
        bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
        pan_snd_req(0x1AB6, 0.7f);
        x = 0.2f + (screen_width > 650 ? 2.3f : 1.8f);
        bgnd_launch_fx_at_position("roar_flames2", x, 0.0f, 0.0f);
        g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[0] = 120;
        g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[1] = 120;
        _mkproc_sleep_ticks = 10.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    } else if (g_pz_fighter_fatality_engine.controller->phase == 1) {
        event = randu0(1000);
        if (event < 1 &&
            g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[0] == 0) {
            pan_snd_req(0x1AB6, -0.7f);
            x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
            bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 200;
        } else if (event < 2 &&
                   g_pz_fighter_fatality_engine.controller
                           ->hazard_initialized[1] == 0) {
            pan_snd_req(0x1AB6, 0.7f);
            x = 0.2f + (screen_width > 650 ? 2.3f : 1.8f);
            bgnd_launch_fx_at_position("roar_flames2", x, 0.0f, 0.0f);
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[1] = 200;
        } else if (event < 2 &&
                   g_pz_fighter_fatality_engine.controller
                           ->hazard_initialized[0] == 0) {
            pan_snd_req(0x1AB6, -0.7f);
            x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
            bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 200;
        }
    }

    if (g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[0] != 0) {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0]--;
    }
    if (g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[1] != 0) {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1]--;
    }
    return 1.0f;
}

/* Soft ceiling: pz_fighter_snake_entering_fatality ~99.75% -- stop. */
static void pz_fighter_snake_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.6f;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

static float pz_fighter_snake_unload(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_snake_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

#define SETUP_SNAKE_EFFECT(name, object, pause_after_setup)                  \
    do {                                                                    \
        effect = fx_by_owner(name, 4);                                      \
        fx_reset(effect);                                                   \
        fx_resume_emit(effect);                                             \
        particle_effect = find_pfx_by_name(name);                           \
        restart_effect_ppfx(particle_effect);                               \
        pfx_bind_emitter_to_obj_bone(particle_effect, object, 5);           \
        emitter = pfx_get_emitter(particle_effect->emitters, 0);            \
        emitter->flags &= (unsigned char)~0x80;                             \
        if (pause_after_setup) {                                            \
            fx_pause_emit(effect);                                          \
        }                                                                   \
    } while (0)

/*
 * Near match (92.02%, retail 0x670/current 0x654): complete loader and effect
 * setup. The animation and ambient-light arguments now use the typed globals
 * located at retail's +0x44 and +0x28 data offsets instead of incorrectly
 * scaled struct-pointer arithmetic. Remaining differences are relocation-base
 * ownership, string/register allocation, and emitter flag-update scheduling.
 */
static float pz_fighter_load_and_place_initial_snake(void) {
    PuzzleDirectLightDefinition* light_def = &skinned_obj_light_def;
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* snakes[2];
    PuzzleAnimPdata* snake_pdata[2];
    PuzzleFatalityController* controller;
    PuzzleParticleEffect* particle_effect;
    PuzzleParticleEmitter* emitter;
    void* effect;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_snake);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_snake_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 5;

    for (i = 0; i < 2; i++) {
        snakes[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08230000, 0x6021);
        obj_change_to_skinned_obj_light_list(
            snakes[i], light_def);
        snakes[i]->model_flags = 0x400;
        snakes[i]->x = i != 0 ? 2.18f : -2.18f;
        if (screen_width > 650) {
            snakes[i]->x = i != 0 ? 2.65f : -2.65f;
        }
        snakes[i]->y = 0.3f;
        snakes[i]->z = -0.28f;
        snakes[i]->flags_bits.angular_velocity_enabled = 1;
        snakes[i]->facing_angle =
            i != 0 ? -1.0207963f : 1.0207963f;
        snakes[i]->flags_bits.scale_active = 1;
        snakes[i]->scale.x = 0.48f;
        snakes[i]->scale.y = 0.48f;
        snakes[i]->scale.z = 0.48f;
        insert_fgnd_mkobj(snakes[i]);
        snake_pdata[i] = animate_obj(
            snakes[i], pz_shared_ani.snake_idle, 1.0f,
            pz_snake_bones, 0, 0, 1);
    }

    obj_add_to_skinned_obj_light_list_with_ambient(
        snakes[0], &skinned_obj_ambient_light_def);
    g_pz_fighter_fatality_engine.secondary_object = snakes[1];
    g_pz_fighter_fatality_engine.primary_object = snakes[0];
    obj_create_sobjs(snakes[0]);
    obj_create_sobjs(snakes[1]);
    sobj_set_priority(obj_first_sobj(snakes[0]), 0x12);
    sobj_set_priority(obj_first_sobj(snakes[1]), 0x12);

    controller = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_snake_controller, 0x80, &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->loop_sound = 0;
        g_pz_fighter_fatality_engine.controller = controller;
        controller->grinder_position[0] =
            g_pz_fighter_fatality_engine.primary_object->motion_rate;
        controller->grinder_position[1] =
            g_pz_fighter_fatality_engine.secondary_object->motion_rate;
        g_pz_fighter_fatality_engine.controller->grinder_target[0] =
            g_pz_fighter_fatality_engine.controller->grinder_position[0];
        g_pz_fighter_fatality_engine.controller->grinder_target[1] =
            g_pz_fighter_fatality_engine.controller->grinder_position[1];
    }
    g_pz_fighter_fatality_engine.controller->fighter_pdata[0] =
        snake_pdata[0];
    g_pz_fighter_fatality_engine.controller->fighter_pdata[1] =
        snake_pdata[1];
    load_pz_fighter_fatality_bank(0x86);

    SETUP_SNAKE_EFFECT("saliva1", snakes[0], 0);
    SETUP_SNAKE_EFFECT("saliva2", snakes[1], 0);
    SETUP_SNAKE_EFFECT("saliva_burst1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("saliva_burst2", snakes[1], 1);
    SETUP_SNAKE_EFFECT("bloody_mouth_dripping1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("bloody_mouth_dripping2", snakes[1], 1);
    SETUP_SNAKE_EFFECT("chunky_mouth_dripping1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("chunky_mouth_dripping2", snakes[1], 1);

    return 0.0f;
}

/* Soft ceiling: pz_fighters_snake_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
static float pz_fighters_snake_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

static float pz_fighters_snake_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

/*
 * Emission-only near miss: 94.93%, retail 0x488/current 0x490. The CFG, calls,
 * comparisons, arithmetic, stores, signed-distance test, and attacker/victim
 * engine views agree. Opcode multisets differ only by one extra current fmr
 * and one pointer reload; remaining islands are base/relocation scheduling.
 */
static float pz_fighters_snake_fatality_prep(void) {
  static int attack_begun;
  static int loser_anim;
  PuzzleFightersEngine *fighters;
  PuzzleFightersEngine *victim_fighters;
  PuzzleFighterRenderObject *attacker_object;
  PuzzleFighterMove *victim_move;
  PuzzlePlayerData *victim_data;
  PuzzlePlayerData *attacker_data;
  float target_x;
  float delta_x;
  float delta_z;
  float signed_distance;
  float player_distance;
  float lower_bound;
  float upper_bound;
  int direction;
  int event;
  int victim;
  unsigned int animation_flags;

  fighters = &g_pz_fighters_engine;
  victim_data = g_game_info.player1;
  attacker_data = g_game_info.player2;
  {
    int attacker = fighters->fatality_attacker;

    attacker_object = pz_fighter_get_player_obj(attacker);
    if (screen_width > 650) {
      target_x = attacker == 0 ? -1.4f : 1.4f;
    } else {
      target_x = attacker == 0 ? -0.6f : 0.6f;
    }

    delta_x = target_x - attacker_object->x;
    delta_z = 0.0f;
    delta_z -= attacker_object->z;
    direction = 1;
    if (target_x < 0.0f) {
      if (attacker_object->x < target_x) {
        direction = -1;
      }
    } else if (attacker_object->x > target_x) {
      direction = -1;
    }
    signed_distance =
        direction * ((delta_x * delta_x) + (delta_z * delta_z));
  }
  player_distance = xz_distance_between_players();
  victim_fighters = &g_pz_fighters_engine;

  lower_bound = -0.04f;
  upper_bound = 0.006f;
  if ((unsigned int)victim_fighters->fatality_victim == 0) {
    lower_bound = -0.006f;
    upper_bound = 0.04f;
  }
  if ((unsigned int)victim_fighters->fatality_victim != 0) {
    victim_data = g_game_info.player2;
    attacker_data = g_game_info.player1;
  }

  if (signed_distance > lower_bound && signed_distance < upper_bound) {
    if (attack_begun == 0) {
      event = 0;
      minigame_event(&event);
    }
    attack_begun = 0;
    g_pz_fighters_engine.fatality_active = 1;
    fx_pause_emit(fx_by_owner("saliva1", 4));
    fx_pause_emit(fx_by_owner("saliva2", 4));

    if (loser_anim == 0) {
      animation_flags = 0x23;
      if ((unsigned int)fighters->fatality_attacker != 0) {
        animation_flags |= 8;
      }
      transition_to_anim_script_frame(
          0.05f, 0.0f,
          g_pz_fighter_fatality_engine.controller
              ->fighter_pdata[victim_fighters->fatality_victim],
          pz_shared_ani.snake_victim, animation_flags);
      set_pdata_anim_step(
          g_pz_fighter_fatality_engine.controller
              ->fighter_pdata[victim_fighters->fatality_victim],
          1.0f);
    }
    loser_anim = 0;
    animation_flags = 0x23;
    if ((unsigned int)fighters->fatality_attacker == 0) {
      animation_flags |= 8;
    }
    transition_to_anim_script_frame(
        0.05f, 0.0f,
        g_pz_fighter_fatality_engine.controller
            ->fighter_pdata[fighters->fatality_attacker],
        pz_shared_ani.snake_attacker, animation_flags);
    set_pdata_anim_step(
        g_pz_fighter_fatality_engine.controller
            ->fighter_pdata[fighters->fatality_attacker],
        1.0f);
    g_pz_fighters_engine.snake_active = 1;
    xfer_proc(pz_fighter_get_player_proc(fighters->fatality_attacker),
              r_pz_fighter_eaten);
    if (victim_data->transition_locked == 0 && player_distance < 1.0f) {
      xfer_proc(pz_fighter_get_player_proc(victim_fighters->fatality_victim),
                pz_fighter_just_backflip);
    }
    g_pz_fighters_engine.fatality_timer = 210;
  } else {
    if (attack_begun == 0) {
      attack_begun = 1;
      event = 0;
      minigame_event(&event);
    }
    if (loser_anim == 0) {
      fx_pause_emit(fx_by_owner("saliva1", 4));
      fx_pause_emit(fx_by_owner("saliva2", 4));
      animation_flags = 0x23;
      if ((unsigned int)fighters->fatality_attacker != 0) {
        animation_flags |= 8;
      }
      transition_to_anim_script_frame(
          0.05f, 0.0f,
          g_pz_fighter_fatality_engine.controller
              ->fighter_pdata[victim_fighters->fatality_victim],
          pz_shared_ani.snake_victim, animation_flags);
      set_pdata_anim_step(
          g_pz_fighter_fatality_engine.controller
              ->fighter_pdata[victim_fighters->fatality_victim],
          1.0f);
      loser_anim = 1;
    }

    if (attacker_data->transition_locked == 0 &&
        signed_distance < lower_bound) {
      xfer_proc(pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_walk_forward);
    } else if (victim_data->transition_locked == 0 &&
               signed_distance > upper_bound) {
      victim = victim_fighters->fatality_victim;
      victim_move = pz_get_fighter_move();
      victim_move->active = 1;
      g_pz_fighters_engine.fighter_reaction_cooldown = 0;
      xfer_proc(pz_fighter_get_player_proc(victim),
                pz_fighter_fatality_victim_to_exact_spot);
    }

    if (signed_distance < lower_bound && victim_data->transition_locked == 0 &&
        player_distance < 1.0f) {
      xfer_proc(pz_fighter_get_player_proc(victim_fighters->fatality_victim),
                pz_fighter_just_backflip);
    }
  }
  return 0.0f;
}

static float pz_fighter_snake_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static inline void pz_snake_bite(unsigned int snake) {
    PuzzleParticleEffect* saliva;
    PuzzleParticleEffect* burst;

    pan_snd_req(0x1AC2, snake == 0 ? -0.7f : 0.7f);
    saliva = (PuzzleParticleEffect*)fx_by_owner(
        snake == 0 ? "saliva1" : "saliva2", 4);
    burst = (PuzzleParticleEffect*)fx_by_owner(
        snake == 0 ? "saliva_burst1" : "saliva_burst2", 4);
    fx_pause_emit(saliva);
    transition_to_anim_script_frame(
        0.1f, 35.0f,
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake],
        pz_shared_ani.snake_bite, 0x23);
    set_pdata_anim_step(
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake], 1.0f);
    _mkproc_sleep_ticks = 3.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    if (g_pz_fighters_engine.fatality_abort == 0) {
        fx_reset(burst);
        fx_resume_emit(burst);
        _mkproc_sleep_ticks = 45.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        if (g_pz_fighters_engine.fatality_abort == 0) {
            fx_resume_emit(saliva);
            transition_to_anim_script_frame(
                0.15f, 0.0f,
                g_pz_fighter_fatality_engine.controller
                    ->fighter_pdata[snake],
                pz_shared_ani.snake_idle, 0);
            set_pdata_anim_step(
                g_pz_fighter_fatality_engine.controller
                    ->fighter_pdata[snake],
                1.0f);
        }
    }
}

static inline void pz_snake_lunge(unsigned int snake) {
    PuzzleParticleEffect* saliva;

    saliva = (PuzzleParticleEffect*)fx_by_owner(
        snake == 0 ? "saliva1" : "saliva2", 4);
    pan_snd_req(0x1AC3, snake == 0 ? -0.7f : 0.7f);
    fx_pause_emit(saliva);
    transition_to_anim_script_frame(
        0.05f, 35.0f,
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake],
        pz_shared_ani.snake_lunge, 0x23);
    set_pdata_anim_step(
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake], 1.0f);
    _mkproc_sleep_ticks = 15.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    fx_resume_emit(saliva);
    if (g_pz_fighters_engine.fatality_abort == 1) {
        fx_pause_emit(saliva);
    } else {
        _mkproc_sleep_ticks = 15.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        fx_pause_emit(saliva);
        if (g_pz_fighters_engine.fatality_abort == 0) {
            _mkproc_sleep_ticks = 20.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            fx_resume_emit(saliva);
            if (g_pz_fighters_engine.fatality_abort == 1) {
                fx_pause_emit(saliva);
            } else {
                transition_to_anim_script_frame(
                    0.1f, 0.0f,
                    g_pz_fighter_fatality_engine.controller
                        ->fighter_pdata[snake],
                    pz_shared_ani.snake_idle, 0);
                set_pdata_anim_step(
                    g_pz_fighter_fatality_engine.controller
                        ->fighter_pdata[snake],
                    1.0f);
            }
        }
    }
}

/*
 * Emission-only near match (91.93%, retail 0x814/current 0x824). m2c and the
 * process creation site confirm the complete preround/event controller and
 * float-return process ABI. Remaining differences are saved-register
 * allocation and equivalent branch/load scheduling in the typed lunge and
 * bite helper expansions; the current expansion is four instructions larger.
 */
static float p_snake_controller(void) {
    unsigned int event;

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        pz_snake_lunge(0);
        _mkproc_sleep_ticks = 20.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        pz_snake_lunge(1);
        _mkproc_sleep_ticks = 10.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
        return 1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->phase != 1) {
        return 1.0f;
    }

    event = randu0(1000) & 0xFFFF;
    if (event < 1) {
        pz_snake_lunge(0);
    } else if (event < 2) {
        pz_snake_lunge(1);
    } else if (event < 4) {
        pz_snake_bite(0);
    } else if (event < 6) {
        pz_snake_bite(1);
    }
    return 1.0f;
}

/*
 * Near match: 95.02%, retail 0x42C/current 0x414. The effect selection,
 * explicit position branches, animation sequence, emitter bitfield updates,
 * ten-tick pauses, and terminal process loop match retail. The remaining six
 * instructions are effect-handle register reuse and local float/string
 * relocation scheduling.
 */
static float r_pz_fighter_eaten(void) {
    PuzzleParticleEffect* blood_burst;
    PuzzleParticleEffect* neck_blood;
    PuzzleParticleEmitter* emitter;
    void* mouth_blood;
    void* mouth_chunks;
    void* saliva;
    int i;

    if (plyr_pdata->side == 0) {
        mouth_blood = fx_by_owner("bloody_mouth_dripping1", 4);
        mouth_chunks = fx_by_owner("chunky_mouth_dripping1", 4);
    } else {
        mouth_blood = fx_by_owner("bloody_mouth_dripping2", 4);
        mouth_chunks = fx_by_owner("chunky_mouth_dripping2", 4);
    }

    pz_fighter_clear_out_external_forces();
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.snake_eaten_start, 3, 0.1f);
    snd_req(0x1AC3);
    ani_loop_more_frames(20.0f);

    {
        PuzzleFightersEngine* fighters = &g_pz_fighters_engine;

        if (fighters->fatality_victim == 0) {
            saliva = fx_by_owner("saliva1", 4);
        } else {
            saliva = fx_by_owner("saliva2", 4);
        }
        fx_resume_emit(saliva);
        ani_loop_more_frames(11.0f);
        ani_loop_more_frames(17.0f);
        xfer_proc(
            pz_fighter_get_player_proc(fighters->fatality_victim),
            pz_fighter_disgusted_with_grinding);
    }
    pz_fighter_shake_camera(3, 0.03f);
    snd_req(0x1AC4);

    blood_burst =
        (PuzzleParticleEffect*)fx_by_owner("pz_blood_burst", 4);
    fx_reset(blood_burst);
    fx_resume_emit(blood_burst);
    blood_burst = find_pfx_by_name("pz_blood_burst");
    restart_effect_ppfx(blood_burst);
    pfx_bind_emitter_to_obj_bone(blood_burst, plyr_obj, 9);
    emitter = pfx_get_emitter(blood_burst->emitters, 0);
    emitter->flags_bits.hidden = 0;
    random_hit(11);
    snd_req(0x1AEF);

    for (i = 0; i < 4; i++) {
        if (plyr_pdata->side == 0) {
            plyr_obj->x = -0.65f;
        } else {
            plyr_obj->x = 0.7f;
        }
        if (screen_width > 650) {
            if (plyr_pdata->side == 0) {
                plyr_obj->x = -1.45f;
            } else {
                plyr_obj->x = 1.4f;
            }
        }
        ani_loop_more_frames(1.0f);
    }

    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    random_hit(11);
    snd_req(0x1AC6);

    neck_blood = (PuzzleParticleEffect*)fx_by_owner("neck_blood", 4);
    fx_reset(neck_blood);
    fx_resume_emit(neck_blood);
    neck_blood = find_pfx_by_name("neck_blood");
    restart_effect_ppfx(neck_blood);
    pfx_bind_emitter_to_obj_bone(neck_blood, plyr_obj, 13);
    emitter = pfx_get_emitter(neck_blood->emitters, 0);
    emitter->flags_bits.hidden = 0;

    fx_resume_emit(mouth_blood);
    blend_to_ani(pz_shared_ani.snake_eaten_end, 3, 0.1f);
    set_ani_speed(0.5f);
    ani_to_frame_x(30.0f);
    snd_req(0x1AEF);
    fx_resume_emit(mouth_chunks);
    ani_to_frame_x(50.0f);
    random_hit(11);
    ani_to_frame_x(66.0f);
    snd_req(0x1AC5);
    random_hit(11);
    set_ani_speed(0.85f);
    ani_to_end();
    random_hit(11);

    fx_pause_emit(neck_blood);
    _mkproc_sleep_ticks = 40.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    random_hit(11);
    fx_resume_emit(neck_blood);
    fx_pause_emit(mouth_chunks);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    snd_req(0x1AEF);
    random_hit(11);
    fx_pause_emit(neck_blood);
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    random_hit(11);
    fx_resume_emit(neck_blood);
    fx_pause_emit(mouth_blood);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    fx_pause_emit(neck_blood);

    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
}

static void pz_fighter_lightning_entering_fatality(
    int attacker, int victim) {
    void* lightning_smoke;
    void* burning_smoke;

    lightning_smoke = fx_by_owner("pz_lightningsmoke", 4);
    burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    fx_pause_emit(burning_smoke);
    fx_pause_emit(lightning_smoke);

    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

static float pz_fighter_lightning_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_lightning_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

/* Exact match: load the lightning effects and initialize their controller. */
static float pz_fighter_load_and_place_initial_lightning(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFatalityController* controller;

    load_art_section(0x70036, &sec_pz_danger_lightning);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_lightning_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 4;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_lightning_controller, 0x80,
            &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->preround_timer = 1;
        controller->loop_sound = 0;
        controller->hazard_initialized[0] = 1;
        controller->hazard_initialized[1] = 1;
        controller->hazard_initialized[2] = 1;
        g_pz_fighter_fatality_engine.controller = controller;
    }

    load_pz_fighter_fatality_bank(0x85);
    return 0.0f;
}

/* Exact match: lightning victim transfer and victory gate. */
static float pz_fighters_lightning_fatality_in_progress(void) {
    PuzzleFightersEngine* fighters;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzlePlayerData* victim_data;

    switch (fatality->active_effect) {
    case 2:
        fatality->active_effect = 3;
        xfer_proc(
            pz_fighter_get_player_proc(
                g_pz_fighters_engine.fatality_attacker),
            pz_fighter_lightning_strike_victim_1);
        break;
    case 3:
        fighters = &g_pz_fighters_engine;
        victim_data = pz_get_pdata_by_id(fighters->fatality_victim);
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_victim),
                pz_fighter_won2);
        }
        break;
    }

    return 0.0f;
}

/*
 * Emission-only near match: 95.85%, retail 0x678/current 0x670. The unsigned
 * shock/cycle/frame counters reproduce retail's loop comparisons, and the
 * terminal transfer has its explicit process-callback zero return. Texture
 * animation, bolt destruction, collapse, effects, and sound order agree. The
 * remaining two-instruction gap is saved-register/constant-pool coloring.
 */
static float pz_fighter_lightning_strike_victim_1(void) {
    PuzzleParticleEffect* particle;
    PuzzleParticleEmitter* emitter;
    PuzzleFighterRenderObject* bolt;
    AniTextureControl* texture;
    void* spark = fx_by_owner("pz_spark", 4);
    void* head_zap = fx_by_owner("pz_headzap", 4);
    void* burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    void* burning_after = fx_by_owner("pz_burningsmoke_after", 4);
    void* blood_burst = fx_by_owner("pz_blood_burst", 4);
    void* chunk = fx_by_owner("pz_chunk", 4);
    Vec head;
    float volume;
    int electrical_sound;
    int voice_sound;
    int frozen = 0;
    unsigned int shock_frame = 0;
    unsigned int cycle;
    unsigned int frame;

    init_ground_move_no_aniproc();
    snd_req(0x1ABB);
    ani_loop_more_frames(5.0f);

    head.x = plyr_obj->x;
    head.y = plyr_obj->y;
    head.z = plyr_obj->z;
    get_bone_world_pos(plyr_obj, 0x10, &head);
    head.y += 0.1f;
    bolt = (PuzzleFighterRenderObject*)load_named_model_from_slot(
        0x70036, "BOLT_OBJECT", 0x2099, 0);
    insert_fgnd_mkobj(bolt);
    obj_set_pos(bolt, &head);
    update_mkobj(bolt);
    obj_create_sobjs(bolt);
    texture = replace_sobj_texture_with_named_wiff(
        obj_first_sobj(bolt), 0x70036,
        "PZ_LIGHTNING_PF_LIGHTNING", "Nightwolf_Bolt");
    set_ani_texture_framerate(texture, 1.0f);
    set_ani_texture_frame(texture, 0);

    electrical_sound = snd_req(0x1ABC);
    fx_reset(head_zap);
    fx_resume_emit(head_zap);
    particle = find_pfx_by_name("pz_headzap");
    restart_effect_ppfx(particle);
    pfx_bind_emitter_to_obj_bone(particle, plyr_obj, 0x10);
    emitter = pfx_get_emitter(particle->emitters, 0);
    emitter->flags_bits.hidden = 0;

    fx_reset(spark);
    fx_resume_emit(spark);
    fx_set_param_v3(spark, 0x202, head.x, head.y, head.z);
    fx_reset(burning_smoke);
    fx_set_param_v3(burning_smoke, 0x202, 0.0f, 0.0f, 0.0f);
    fx_resume_emit(burning_smoke);
    particle = find_pfx_by_name("pz_burningsmoke");
    restart_effect_ppfx(particle);
    pfx_bind_emitter_to_obj_bone(particle, plyr_obj, 0x10);
    emitter = pfx_get_emitter(particle->emitters, 0);
    emitter->flags_bits.hidden = 0;

    pz_fighter_shake_camera(3, 0.03f);
    voice_sound = plyr_snd_req(0x4E);
    random_hit(1);
    plyr_obj->facing_angle = 0.0f;

    for (cycle = 0; cycle < 8; cycle++) {
        glitch_to_ani(pz_shared_ani.head_poked, 3);
        for (frame = 0; frame < 14.0f; frame++) {
            shock_frame++;
            if (shock_frame > 5) {
                if (frozen != 0) {
                    unfreeze_player();
                } else {
                    freeze_player();
                }
                frozen = !frozen;
                shock_frame = 0;
            }
            ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            get_bone_world_pos(plyr_obj, 0x10, &head);
            head.y += 0.1f;
            bolt->x = head.x;
            bolt->y = head.y;
            bolt->z = head.z;
        }
    }
    if (frozen != 0) {
        unfreeze_player();
    }

    glitch_to_ani(pz_shared_ani.head_poked, 3);
    ani_to_frame_x(4.0f);
    snd_stop(electrical_sound);
    snd_stop(voice_sound);
    snd_req(0x1ABD);
    snd_req(0x1AC1);

    fx_reset(head_zap);
    fx_reset(burning_after);
    fx_resume_emit(burning_after);
    particle = find_pfx_by_name("pz_burningsmoke_after");
    restart_effect_ppfx(particle);
    pfx_bind_emitter_to_obj_bone(particle, plyr_obj, 9);
    emitter = pfx_get_emitter(particle->emitters, 0);
    emitter->flags_bits.hidden = 0;
    pz_fighter_shake_camera(3, 0.03f);
    pz_fighter_fatality_launch_eyes();

    fx_reset(blood_burst);
    fx_resume_emit(blood_burst);
    get_bone_world_pos(plyr_obj, 0x10, &head);
    head.y += 0.1f;
    fx_set_param_v3(blood_burst, 0x202, head.x, head.y, head.z);
    fx_reset(chunk);
    fx_resume_emit(chunk);
    fx_set_param_v3(chunk, 0x202, head.x, head.y, head.z);
    if (bolt->instance != 0) {
        bolt->vtbl->destroy(bolt, bolt->vtbl);
    }
    obj_set_bone_collapse_flag(plyr_obj, 0x10);

    blend_to_ani(shared_ani.lightning_electrocution, 3, 0.1f);
    ani_to_frame_x(2.0f);
    fx_pause_emit(burning_smoke);
    ani_to_frame_x(30.0f);
    snd_req(0x1ABE);
    ani_to_frame_x(49.0f);
    got_hit_fx(4, 9, 1, 0, 0, 1, 0.0f);
    ani_to_end();
    snd_req(0x1ABF);

    for (; cycle < 100; cycle++) {
        _mkproc_sleep_ticks = (float)(randu0(30) & 0xFFFF) + 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        frame = (randu0(3) & 0xFFFF) + 0x1ABE;
        volume = 0.5f + frand(0.5f);
        if ((randu0(100) & 0xFFFF) < 50) {
            snd_req_vol(frame, volume);
        } else if ((randu0(100) & 0xFFFF) < 50) {
            pan_vol_snd_req(frame, -0.7f, volume);
        } else {
            pan_vol_snd_req(frame, 0.7f, volume);
        }
    }

    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

/*
 * Near match: 99.18%, exact retail size. Branch-local offset and victim
 * lifetimes recover the repeated pointing helper; only register arguments
 * remain different.
 */
static float pz_fighters_lightning_fatality_prep(void) {
  static int one_last_hit = 1;
  static int start_pointing = 1;
  PuzzleFighterRenderObject *victim_object;
  void *lightning_smoke;
  void *burning_smoke;
  float player_distance;
  float target_x;
  float target_z;
  float delta_x;
  float delta_z;
  float signed_distance;
  int direction;
  int event;

  lightning_smoke = fx_by_owner("pz_lightningsmoke", 4);
  burning_smoke = fx_by_owner("pz_burningsmoke", 4);
  fx_pause_emit(burning_smoke);
  fx_pause_emit(lightning_smoke);

  switch (g_pz_fighter_fatality_engine.active_effect) {
  case 0:
    if (one_last_hit == 1) {
      if (g_game_info.player1->transition_locked == 0 &&
          g_game_info.player2->transition_locked == 0) {
        player_distance = xz_distance_between_players();
        {
          int victim = g_pz_fighters_engine.fatality_victim;

          victim_object = pz_fighter_get_player_obj(victim);
          direction = 1;
          {
            Vec offset = {0.05f, 0.0f, 0.0f};

            if (screen_width > 650) {
              offset.x = 0.35f;
            }
            if (victim == 0) {
              target_x = g_pz_fighters_engine.fighter_posts[1].x + offset.x;
              target_z = g_pz_fighters_engine.fighter_posts[1].z + offset.z;
            } else {
              target_x = g_pz_fighters_engine.fighter_posts[0].x - offset.x;
              target_z = g_pz_fighters_engine.fighter_posts[0].z - offset.z;
            }
          }

          delta_x = target_x - victim_object->x;
          delta_z = target_z - victim_object->z;
          if (target_x < 0.0f) {
            if (victim_object->x < target_x) {
              direction = -1;
            }
          } else if (victim_object->x > target_x) {
            direction = -1;
          }
          signed_distance =
              direction * ((delta_x * delta_x) + (delta_z * delta_z));
        }

        if (player_distance < 1.5f) {
          if (signed_distance > 0.9f) {
            xfer_proc(pz_fighter_get_player_proc(
                          g_pz_fighters_engine.fatality_victim),
                      pz_fighter_dash_back);
          } else {
            xfer_proc(pz_fighter_get_player_proc(
                          g_pz_fighters_engine.fatality_victim),
                      pz_fighter_shove);
          }
        }
        one_last_hit = 0;
        start_pointing = 1;
      }
    } else if (start_pointing == 1) {
      if (pz_get_pdata_by_id(
              g_pz_fighters_engine.fatality_victim)
              ->transition_locked != 0) {
        break;
      }
      start_pointing = 0;
      xfer_proc(pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_execute_point_no_space_check);
    } else {
      start_pointing = 1;
      one_last_hit = 1;
      event = 0;
      minigame_event(&event);
      xfer_proc(
          pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_attacker),
          pz_fighter_execute_point_reaction_no_space);
      g_pz_fighter_fatality_engine.active_effect = 1;
    }
    break;

  case 1:
    g_pz_fighter_fatality_engine.active_effect = 4;
    g_pz_fighter_fatality_engine.effect_timer = 63;
    break;

  case 4:
    if (g_pz_fighter_fatality_engine.effect_timer > 0) {
      g_pz_fighter_fatality_engine.effect_timer--;
    }
    if (g_pz_fighter_fatality_engine.effect_timer == 0) {
      g_pz_fighter_fatality_engine.active_effect = 2;
      g_pz_fighters_engine.fatality_active = 1;
      g_pz_fighters_engine.fatality_timer = 180;
    }
    break;
  }

  return 0.0f;
}

static float pz_fighter_lightning_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
        if ((randu0(100) & 0xFFFF) < 50) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 0;
        } else {
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 1;
        }
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static inline PuzzleFighterRenderObject* pz_resolve_lightning_bolt(
    PuzzleFighterRenderObject* bolt, unsigned int instance) {
    PuzzleFighterRenderObject* resolved;

    if (bolt != 0) {
        if (bolt->instance == instance) {
            resolved = bolt;
        } else {
            resolved = 0;
        }
    } else {
        resolved = 0;
    }
    return resolved;
}

static inline void pz_lightning_bolt(Vec* position, int pan_side) {
    PuzzleFighterRenderObject* bolt;
    PuzzleParticleEffect* lightning_smoke;
    PuzzleParticleEffect* spark;
    PuzzleParticleEffect* burning_smoke;
    AniTextureControl* texture;
    unsigned int bolt_instance;

    bolt = (PuzzleFighterRenderObject*)load_named_model_from_slot(
        0x70036, "BOLT_OBJECT", 0x2099, 0);
    insert_fgnd_mkobj(bolt);
    obj_set_pos(bolt, position);
    update_mkobj(bolt);
    obj_create_sobjs(bolt);
    texture = replace_sobj_texture_with_named_wiff(
        obj_first_sobj(bolt), 0x70036, "PZ_LIGHTNING_PF_LIGHTNING",
        "Nightwolf_Bolt");
    set_ani_texture_framerate(texture, 1.0f);
    set_ani_texture_frame(texture, 0);
    bolt_instance = bolt->instance;
    if (pan_side < 0) {
        pan_snd_req(0x1ABA, -0.7f);
    } else if (pan_side > 0) {
        pan_snd_req(0x1ABA, 0.7f);
    } else {
        if (position->x < 0.0f) {
            pan_snd_req(0x1ABA, -0.7f);
        } else {
            pan_snd_req(0x1ABA, 0.7f);
        }
    }

    lightning_smoke = fx_by_owner("pz_lightningsmoke", 4);
    spark = fx_by_owner("pz_spark", 4);
    burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    _mkproc_sleep_ticks = 3.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    if (g_pz_fighter_fatality_engine.controller->preround_active != 0 ||
        g_pz_fighter_fatality_engine.controller->phase != 0) {
        fx_reset(spark);
        fx_set_param_v3(
            spark, 0x202, position->x, position->y, position->z);
        fx_resume_emit(spark);
        fx_reset(burning_smoke);
        fx_set_param_v3(
            burning_smoke, 0x202, position->x, position->y + 0.3f,
            position->z);
        fx_resume_emit(burning_smoke);
        fx_reset(lightning_smoke);
        fx_set_param_v3(
            lightning_smoke, 0x202, position->x, position->y, position->z);
        fx_resume_emit(lightning_smoke);
        _mkproc_sleep_ticks = 20.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        if (g_pz_fighter_fatality_engine.controller->preround_active != 0 ||
            g_pz_fighter_fatality_engine.controller->phase != 0) {
            fx_pause_emit(burning_smoke);
            fx_pause_emit(lightning_smoke);
        }
    }

    bolt = pz_resolve_lightning_bolt(bolt, bolt_instance);
    if (bolt != 0 && bolt->instance != 0) {
        bolt->vtbl->destroy(bolt, bolt->vtbl);
    }
}

/*
 * Near match: 99.29%, exact 0x7FC size. Late pan evaluation, multiply-by-
 * negative-one, the local-result stale-instance resolver, and the joined
 * success return recover every retail operation; only register arguments
 * differ in objdiff.
 */
static float p_lightning_controller(void) {
    Vec position = {0.0f, 0.0f, 0.0f};

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        position.x = screen_width > 650 ? -2.2f : -1.9f;
        pz_lightning_bolt(&position, -1);
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        position.x = screen_width > 650 ? 2.2f : 1.9f;
        pz_lightning_bolt(&position, 1);
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    } else if (g_pz_fighter_fatality_engine.controller->phase == 1 &&
               (randu0(10000) & 0xFFFF) < 15) {
        position.x = screen_width > 650 ? -2.2f : -1.9f;
        if ((randu0(100) & 0xFFFF) < 50) {
            position.x *= -1.0f;
        }
        pz_lightning_bolt(&position, 0);
        _mkproc_sleep_ticks = 5.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
    return 1.0f;
}

static float pz_fighters_lightning_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

static void pz_fighter_objects_falling_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

static float pz_fighter_objects_falling_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_objects_falling_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

/* Soft ceiling: exact code; objdiff only relabels pooled string/float data. */
static float pz_fighter_load_and_place_initial_objects_falling(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* objects[2];
    PuzzleFatalityController* controller;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_1ton);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_1ton_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 3;

    for (i = 0; i < 2; i++) {
        objects[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08210000, 0x6021);
        objects[i]->model_flags = 1;
        hide_obj(objects[i]);
        insert_fgnd_mkobj(objects[i]);
    }
    pz_fighter_set_objects_falling_obj(objects[0], objects[1]);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_objects_falling_controller2, 0x80,
            &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->preround_timer = 1;
        controller->loop_sound = 0;
        controller->hazard_initialized[0] = 1;
        controller->hazard_initialized[1] = 1;
        controller->hazard_initialized[2] = 1;
        g_pz_fighter_fatality_engine.controller = controller;
        controller->preround_sound_started = 0;
    }

    load_pz_fighter_fatality_bank(0x83);
    return 0.0f;
}

/*
 * Emission-only near match (93.09%, exact retail 0x1A0 size). The complete
 * state machine and call order agree; residue is stack-slot and FPR/GPR
 * allocation, branch-local Vec scheduling, and constant relocations.
 */
static float pz_fighters_objects_falling_fatality_in_progress(void) {
    PuzzleFatalityHazardObject* falling_object;
    PuzzlePlayerData* victim_data;
    Vec impact_position;

    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 2: {
        Vec bone_offset = {0.0f, 0.6f, 0.0f};

        get_bone_world_pos(
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker),
            0x10,
            &impact_position);
        falling_object =
            g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
        impact_position.x += bone_offset.x;
        impact_position.y += bone_offset.y;
        impact_position.z += bone_offset.z;

        if (falling_object->y <= impact_position.y) {
            g_pz_fighter_fatality_engine.active_effect = 3;
            pz_fighter_fatality_launch_eyes();
            snd_req_vol(0x1AC9, 1.0f);
            snd_req(0x1ACA);
            snd_req_delay(0x1ADC, 10);
            snd_req_delay(0x1ADC, 22);
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_attacker),
                pz_fighter_objects_falling_victim_crushed);
            pz_fighter_shake_camera(3, 0.03f);
        } else {
            falling_object->motion -= 0.017f;
        }
        break;
    }
    case 3:
        falling_object =
            g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
        victim_data = pz_get_pdata_by_id(
            g_pz_fighters_engine.fatality_victim);
        if (falling_object->y < 0.25f) {
            falling_object->motion = 0.0f;
            falling_object->flags_bits.gravity_enabled = 0;
        }
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_won2);
        }
        break;
    }

    return 0.0f;
}

/*
 * Near match: 98.96%, exact retail size. Offset lifetime, signed-distance
 * comparison, branch-local victim reloads, and ordered hazard bit stores agree;
 * objdiff's remaining differences are register arguments only.
 */
static float pz_fighters_objects_falling_fatality_prep(void) {
  static int one_last_hit = 1;
  static int start_pointing = 1;
  PuzzleFatalityHazardObject *falling_object;
  PuzzleFighterRenderObject *victim_object;
  PuzzleFighterRenderObject *attacker_object;
  float player_distance;
  float target_x;
  float target_z;
  float delta_x;
  float delta_z;
  float signed_distance;
  int direction;
  int event;

  switch (g_pz_fighter_fatality_engine.active_effect) {
  case 0:
    if (one_last_hit == 1) {
      if (g_game_info.player1->transition_locked == 0 &&
          g_game_info.player2->transition_locked == 0) {
        player_distance = xz_distance_between_players();
        {
          int victim = g_pz_fighters_engine.fatality_victim;

          victim_object = pz_fighter_get_player_obj(victim);
          direction = 1;
          {
            Vec offset = {0.05f, 0.0f, 0.0f};

            if (victim == 0) {
              target_x = g_pz_fighters_engine.fighter_posts[1].x + offset.x;
              target_z = g_pz_fighters_engine.fighter_posts[1].z + offset.z;
            } else {
              target_x = g_pz_fighters_engine.fighter_posts[0].x - offset.x;
              target_z = g_pz_fighters_engine.fighter_posts[0].z - offset.z;
            }
          }

          delta_x = target_x - victim_object->x;
          delta_z = target_z - victim_object->z;
          if (target_x < 0.0f) {
            if (victim_object->x < target_x) {
              direction = -1;
            }
          } else if (victim_object->x > target_x) {
            direction = -1;
          }
          signed_distance =
              direction * ((delta_x * delta_x) + (delta_z * delta_z));
        }

        if (player_distance < 1.5f) {
          if (signed_distance > 1.0f) {
            xfer_proc(pz_fighter_get_player_proc(
                          g_pz_fighters_engine.fatality_victim),
                      pz_fighter_dash_back);
          } else {
            xfer_proc(pz_fighter_get_player_proc(
                          g_pz_fighters_engine.fatality_victim),
                      pz_fighter_shove);
          }
        }
        one_last_hit = 0;
        start_pointing = 1;
      }
    } else if (start_pointing == 1) {
      if (pz_get_pdata_by_id(
              g_pz_fighters_engine.fatality_victim)
              ->transition_locked != 0) {
        break;
      }
      start_pointing = 0;
      xfer_proc(pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_execute_point_no_space_check);
    } else {
      start_pointing = 1;
      one_last_hit = 1;
      event = 0;
      minigame_event(&event);
      xfer_proc(
          pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_attacker),
          pz_fighter_execute_R_coming_down);
      g_pz_fighter_fatality_engine.active_effect = 1;
    }
    break;

  case 1:
    g_pz_fighter_fatality_engine.active_effect = 4;
    g_pz_fighter_fatality_engine.effect_timer = 63;
    break;

  case 4:
    attacker_object =
        pz_fighter_get_player_obj(g_pz_fighters_engine.fatality_attacker);
    if (g_pz_fighter_fatality_engine.effect_timer > 0) {
      g_pz_fighter_fatality_engine.effect_timer--;
    }
    if (g_pz_fighter_fatality_engine.effect_timer == 0) {
      g_pz_fighter_fatality_engine.active_effect = 2;
      falling_object = g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
      falling_object->flags_bits.airborne = 1;
      falling_object->flags_bits.gravity_enabled = 1;
      falling_object->x = attacker_object->x;
      falling_object->z = attacker_object->z;
      falling_object->y = 3.2f;
      falling_object->motion = -0.052f;
      falling_object->flags_bits.scale_active = 1;
      falling_object->scale.x = 0.5f;
      falling_object->scale.y = 0.5f;
      falling_object->scale.z = 0.5f;
      _mkproc_sleep_ticks = 1.0f;
      aproc->vtbl->sleep(aproc->vtbl);
      unhide_sobj(falling_object);
      g_pz_fighters_engine.fatality_active = 1;
      g_pz_fighters_engine.fatality_timer = 150;
    }
    break;
  }

  return 0.0f;
}

static float pz_fighter_objects_falling_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
        if ((randu0(100) & 0xFFFF) < 50) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 0;
        } else {
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[0] = 1;
        }
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

/*
 * Soft ceiling: pz_fighter_objects_falling_victim_crushed ~94.90% --
 * the crushed pose, staged effects, and prone transfer agree; remaining
 * differences are MWCC expression/register scheduling.
 */
static float pz_fighter_objects_falling_victim_crushed(void) {
    bgnd_launch_fx_at_position(
        "chunk_crush1_fx", plyr_obj->x, plyr_obj->y, plyr_obj->z);
    init_ground_move();
    blend_to_ani(pz_shared_ani.objects_falling_crushed, 3, 0.5f);
    set_ani_speed(1.0f);
    ani_loop_more_frames(2.0f);
    bgnd_launch_fx_at_position(
        "chunk_crush2_fx", plyr_obj->x, plyr_obj->y, plyr_obj->z);
    ani_loop_more_frames(2.0f);
    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: exact-size eye launch; residue is aggregate/float relocation
 * labeling, global-base allocation, and one equivalent bit-test opcode.
 */
static void pz_fighter_fatality_launch_eyes(void) {
    Vec launch_offset = {0.0f, 0.1f, 0.02f};
    Vec left_offset = {-0.05f, 0.0f, 0.05f};
    Vec right_offset = {0.05f, 0.0f, 0.05f};
    PuzzleFighterRenderObject* left_eye;
    PuzzleFighterRenderObject* right_eye;
    PuzzleFighterPhysics* fighter;
    PuzzleBoneData* bone_data;
    MKMATRIX* bone_matrix;
    Vec bone_position;
    Vec eye_position;
    Vec camera_position;
    Vec camera_direction;
    float angle_offset;

    left_eye = g_pz_fighters_engine.left_eye;
    right_eye = g_pz_fighters_engine.right_eye;
    fighter = (PuzzleFighterPhysics*)g_game_info.player1_physics;
    if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 1) {
        fighter = (PuzzleFighterPhysics*)g_game_info.player2_physics;
    }

    if ((fighter->flags_0A & 0x40) != 0) {
        angle_offset = 1.5707964f;
    } else {
        angle_offset = -1.5707964f;
    }
    left_eye->facing_angle = fighter->facing_angle + angle_offset;
    right_eye->facing_angle = fighter->facing_angle + angle_offset;
    rotate_xz(
        &launch_offset, &launch_offset,
        fighter->facing_angle + angle_offset);

    get_bone_world_pos(
        (PuzzleFighterRenderObject*)fighter, 0x10, &bone_position);
    calc_bone_world_mat((PuzzleFighterRenderObject*)fighter, 0x10);
    bone_data = ((PuzzleBoneObjectView*)fighter)->bone_data;
    bone_matrix = bone_data->matrix;
    if (bone_matrix == 0) {
        return;
    }

    v3_x_mat_add_v3(
        &eye_position, &left_offset, bone_matrix, &bone_position);
    left_eye->x = eye_position.x;
    left_eye->y = eye_position.y;
    left_eye->z = eye_position.z;
    v3_x_mat_add_v3(
        &eye_position, &right_offset, bone_matrix, &bone_position);
    right_eye->x = eye_position.x;
    right_eye->y = eye_position.y;
    right_eye->z = eye_position.z;

    left_eye->flags_bits.gravity_enabled = 1;
    get_camera_position(&camera_position);
    v3_sub_v3(
        &camera_direction, &camera_position, (Vec*)&left_eye->x);
    scale_v3(
        (Vec*)&left_eye->external_force_x, &camera_direction, 0.02f);
    left_eye->external_force_y -= 0.0008f;

    right_eye->flags_bits.gravity_enabled = 1;
    scale_v3(
        (Vec*)&right_eye->external_force_x, &camera_direction, 0.02f);
    right_eye->external_force_y += 0.0013f;

    _mkproc_sleep_ticks = 2.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    unhide_obj(left_eye);
    unhide_obj(right_eye);
}

/*
 * Soft ceiling: exact-size retail algorithm and state ordering. Remaining
 * differences are branch inversion, group-pointer/register allocation, stack
 * frame allocation, and local float relocation labels.
 */
static float p_objects_falling_controller2(void) {
    PuzzleFatalityHazardGroup* group;
    unsigned int i;

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        for (i = 0; i < 2; i++) {
            if ((int)g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[i] == 1) {
                group = &g_pz_fighter_fatality_engine.hazard_groups[i];
                group->objects[0]->x = 1.7f;
                group->objects[1]->x = 1.7f;
                group->objects[2]->x = 1.7f;
                if (screen_width > 650) {
                    group->objects[0]->x = 2.1f;
                    group->objects[1]->x = 2.1f;
                    group->objects[2]->x = 2.1f;
                }
                if (i == 1) {
                    group->objects[0]->x *= -1.0f;
                    group->objects[1]->x *= -1.0f;
                    group->objects[2]->x *= -1.0f;
                }
                _mkproc_sleep_ticks = 1.0f;
                aproc->vtbl->sleep(aproc->vtbl);
                unhide_sobj(group->objects[0]);
                unhide_sobj(group->objects[1]);
                unhide_sobj(group->objects[2]);
                g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[i] = 0;
                group->objects[0]->motion = 0.02f;
                group->objects[1]->motion = 0.02f;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[i] == 0) {
                group = &g_pz_fighter_fatality_engine.hazard_groups[i];
                if (group->objects[0]->y > 4.0f) {
                    group->objects[0]->motion = 0.0f;
                    group->objects[1]->motion = 0.0f;
                    g_pz_fighter_fatality_engine.controller
                        ->hazard_initialized[i] = 2;
                }
            }
        }
    } else if (g_pz_fighter_fatality_engine.controller->phase == 1) {
        if ((int)g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[2] == 1) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[0][0] = -0.004f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[1][0] = 0.004f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[2] = 0;
        }

        if (g_pz_fighter_fatality_engine.hazard_groups[0]
                .objects[2]
                ->x > 2.2f) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[0][0] = -0.004f;
        }
        if (g_pz_fighter_fatality_engine.hazard_groups[0]
                .objects[2]
                ->x < 0.5f) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[0][0] = 0.004f;
        }
        if (g_pz_fighter_fatality_engine.hazard_groups[1]
                .objects[2]
                ->x < -2.2f) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[1][0] = 0.004f;
        }
        if (g_pz_fighter_fatality_engine.hazard_groups[1]
                .objects[2]
                ->x > -0.5f) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[1][0] = -0.004f;
        }
        g_pz_fighter_fatality_engine.hazard_groups[0].objects[2]->x +=
            g_pz_fighter_fatality_engine.controller->hazard_motion[0][0];
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[0][0] < 0.0f) {
            if (g_pz_fighter_fatality_engine.hazard_groups[0]
                    .objects[2]
                    ->x < 1.2f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] *= 0.98f;
            } else if (g_pz_fighter_fatality_engine.hazard_groups[0]
                           .objects[2]
                           ->x > 1.4f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] *= 1.02f;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] < -0.03f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] = -0.03f;
            } else if (g_pz_fighter_fatality_engine.controller
                           ->hazard_motion[0][0] > -0.004f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] = -0.004f;
            }
        }
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[0][0] > 0.0f) {
            if (g_pz_fighter_fatality_engine.hazard_groups[0]
                    .objects[2]
                    ->x > 1.4f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] *= 0.98f;
            } else if (g_pz_fighter_fatality_engine.hazard_groups[0]
                           .objects[2]
                           ->x < 1.2f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] *= 1.02f;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] > 0.03f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] = 0.03f;
            } else if (g_pz_fighter_fatality_engine.controller
                           ->hazard_motion[0][0] < 0.004f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[0][0] = 0.004f;
            }
        }

        g_pz_fighter_fatality_engine.hazard_groups[1].objects[2]->x +=
            g_pz_fighter_fatality_engine.controller->hazard_motion[1][0];
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[1][0] < 0.0f) {
            if (g_pz_fighter_fatality_engine.hazard_groups[1]
                    .objects[2]
                    ->x < -1.4f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] *= 0.98f;
            } else if (g_pz_fighter_fatality_engine.hazard_groups[1]
                           .objects[2]
                           ->x > -1.2f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] *= 1.02f;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] < -0.03f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] = -0.03f;
            } else if (g_pz_fighter_fatality_engine.controller
                           ->hazard_motion[1][0] > -0.004f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] = -0.004f;
            }
        }
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[1][0] > 0.0f) {
            if (g_pz_fighter_fatality_engine.hazard_groups[1]
                    .objects[2]
                    ->x > -1.2f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] *= 0.98f;
            } else if (g_pz_fighter_fatality_engine.hazard_groups[1]
                           .objects[2]
                           ->x < -1.4f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] *= 1.02f;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] > 0.03f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] = 0.03f;
            } else if (g_pz_fighter_fatality_engine.controller
                           ->hazard_motion[1][0] < 0.004f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[1][0] = 0.004f;
            }
        }
    }
    return 1.0f;
}

static float pz_fighters_objects_falling_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    if (g_pz_fighter_fatality_engine.controller
            ->preround_sound_started == 0) {
        snd_req(0x1ACB);
    }
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 1;
    return 0.0f;
}

static void pz_fighter_set_objects_falling_obj(
    PuzzleFighterRenderObject* object1,
    PuzzleFighterRenderObject* object2) {
    int side;
    unsigned int index;

    g_pz_fighter_fatality_engine.primary_object = object1;
    g_pz_fighter_fatality_engine.secondary_object = object2;
    obj_create_sobjs(object1);
    obj_create_sobjs(object2);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(object1, 1);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[1] =
        obj_find_sobj_by_id(object1, 3);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[2] =
        obj_find_sobj_by_id(object1, 4);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[3] =
        obj_find_sobj_by_id(object1, 5);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(object2, 1);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[1] =
        obj_find_sobj_by_id(object2, 3);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[2] =
        obj_find_sobj_by_id(object2, 4);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[3] =
        obj_find_sobj_by_id(object2, 5);
    unhide_obj(object1);
    unhide_obj(object2);

    for (index = 0; index < 4; index++) {
        hide_sobj(
            g_pz_fighter_fatality_engine.hazard_groups[0].objects[index]);
        hide_sobj(
            g_pz_fighter_fatality_engine.hazard_groups[1].objects[index]);
    }
    for (side = 0; side < 2; side++) {
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->flags_bits.gravity_enabled = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[1]->flags_bits.gravity_enabled = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->flags_bits.gravity_enabled = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->flags_bits.airborne = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[1]->flags_bits.airborne = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->flags_bits.airborne = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->flags_bits.scale_active = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->scale.x = 0.5f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->scale.y = 0.5f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[0]->scale.z = 0.5f;
        g_pz_fighter_fatality_engine.hazard_groups[side].objects[1]->y -=
            0.65f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->flags_bits.scale_active = 1;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->scale.x = 0.7f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->scale.y = 0.7f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->scale.z = 0.7f;
        g_pz_fighter_fatality_engine.hazard_groups[side]
            .objects[2]->field_28 = 50.0f;
    }
}

/*
 * Soft ceiling: the fixed-bound loop and all controller/hazard stores match;
 * residue is phase-zero register reuse and strength-reduced index scheduling.
 */
static void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim) {
    int i;

    i = 0;
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

    for (; i < 2; i++) {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[i][0] = 0.0f;
    }
}

static float pz_fighter_chomper2_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

/*
 * Soft ceiling: retail and source perform the same state/phase stores and
 * two hazard/motion-row clears. The fixed-bound loop restores mtctr/bdnz;
 * MWCC still reuses the phase-zero register and schedules the two strength-
 * reduced index updates differently. Two typed index variants did not improve
 * emission and were removed.
 */
static float pz_fighter_chomper2_round_over(void) {
    int i;

    i = 0;
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    for (; i < 2; i++) {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[i][0] = 0.0f;
    }
    return 0.0f;
}

/*
 * Soft ceiling: the complete retail loader/state setup is present; residue is
 * nested address induction/register scheduling and relocation labels.
 */
static float pz_fighter_load_and_place_initial_chompers2(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* columns[2];
    PuzzleFatalityController* controller;
    PuzzleFatalityHazardObject* chomper;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_crusher);
    g_pz_fighters_engine.fatality_index = 2;

    for (i = 0; i < 2; i++) {
        columns[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08200000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags_bits.scale_active = 1;
        if (screen_width > 650) {
            columns[i]->x = i != 0 ? 2.15f : -2.15f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.1f;
            columns[i]->scale.x = 1.2f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.3f;
        } else {
            columns[i]->x = i != 0 ? 1.75f : -1.75f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.1f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.1f;
        }
        columns[i]->flags_bits.gravity_enabled = 1;
        columns[i]->external_force_x = 0.0f;
        columns[i]->external_force_y = 0.0f;
        columns[i]->external_force_z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    g_pz_fighter_fatality_engine.primary_object = columns[0];
    g_pz_fighter_fatality_engine.secondary_object = columns[1];
    obj_create_sobjs(columns[0]);
    obj_create_sobjs(columns[1]);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(columns[0], 1);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(columns[1], 1);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_chomper2_controller, 0x80,
            &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->loop_sound = 0;
        g_pz_fighter_fatality_engine.controller = controller;
    }

    for (i = 0; i < 2; i++) {
        chomper =
            g_pz_fighter_fatality_engine.hazard_groups[i].objects[0];
        chomper->flags_bits.airborne = 1;
        chomper->flags_bits.gravity_enabled = 1;
        chomper->motion = 0.0f;
        g_pz_fighter_fatality_engine.controller
            ->chomper_position[i][0] = 0.0f;
        g_pz_fighter_fatality_engine.controller
            ->hazard_motion[i][0] = 0.0f;
        g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[i] = 0;
    }

    load_pz_fighter_fatality_bank(0x81);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_crusher_fx.mko", &effect_context);
    return 0.0f;
}

/*
 * Near match: 98.63%, retail 0x2E8/current 0x2E4. All state operations and
 * reloads agree; residue is register allocation plus one unreachable duplicate
 * branch in retail's switch dispatch. Explicit default/empty cases regress.
 */
static float pz_fighters_chomper2_fatality_in_progress(void) {
    static int launch_sounds = 1;
    static int launch_more_meat_chunks = 1;
    PuzzleFatalityHazardObject* crusher;
    PuzzlePlayerData* attacker_data;
    PuzzleFighterRenderObject* attacker_object;
    float blood_x;
    float blood_z;

    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 2:
        crusher = g_pz_fighter_fatality_engine
                      .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                      .objects[0];
        if (crusher->y < 3.4f) {
            crusher->motion = 0.07f;
        } else {
            snd_req(0x1AAC);
            g_pz_fighter_fatality_engine
                .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                .objects[0]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.active_effect = 3;
        }
        break;
    case 3:
        crusher = g_pz_fighter_fatality_engine
                      .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                      .objects[0];
        if (crusher->y > 1.6f) {
            crusher->motion = -0.45f;
            launch_sounds = 1;
        } else if (crusher->y > 0.6f) {
            crusher->motion = -0.45f;
            if (launch_sounds == 1) {
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                xfer_proc(
                    pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_attacker),
                    pz_fighter_chomper2_victim_crushed);
                xfer_proc(
                    pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_victim),
                    pz_fighter_wipe_blood_off);

                {
                    Vec blood_offset = {0.05f, 0.0f, 0.0f};

                    if (attacker_data->side == 0) {
                        blood_x =
                            g_pz_fighters_engine.fighter_posts[1].x +
                            blood_offset.x;
                        blood_z =
                            g_pz_fighters_engine.fighter_posts[1].z +
                            blood_offset.z;
                    } else {
                        blood_x =
                            g_pz_fighters_engine.fighter_posts[0].x -
                            blood_offset.x;
                        blood_z =
                            g_pz_fighters_engine.fighter_posts[0].z -
                            blood_offset.z;
                    }
                }
                blood_z -= 0.2f;

                bgnd_launch_fx_at_position(
                    "blood_crush_fx", blood_x, 0.1f, blood_z);
                if (attacker_data->side == 1) {
                    bgnd_set_fx_ang_y(3.1415927f);
                }
                bgnd_launch_fx_at_position(
                    "blood_splat_fx", blood_x, 0.1f, blood_z);
                if (attacker_data->side == 1) {
                    bgnd_set_fx_ang_y(3.1415927f);
                }
                snd_req(0x1AD6);
                launch_sounds = 0;
                launch_more_meat_chunks = 1;
            }
        } else if (crusher->y > 0.15f) {
            if (launch_more_meat_chunks == 1) {
                pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
                launch_more_meat_chunks = 0;
            }
            g_pz_fighter_fatality_engine
                .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                .objects[0]
                ->motion = -0.14f;
        } else {
            attacker_object =
                pz_fighter_get_player_obj(
                    g_pz_fighters_engine.fatality_attacker);
            g_pz_fighter_fatality_engine
                .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                .objects[0]
                ->motion = 0.0f;
            attacker_object->external_force_y = 0.0f;
            snd_req(0x1ADB);
            snd_req_vol(0x1AAE, 1.0f);
            snd_req_delay(0x1AAF, 7);
            plyr_pdata =
                pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
            snd_major_hit_voice();
            g_pz_fighter_fatality_engine.active_effect = 4;
        }
        break;
    }

    return 0.0f;
}

/*
 * Retail uses the old timer value in case 1 and reloads the attacker index for
 * each independently indexed engine access. Exact retail size; objdiff's
 * remaining 0.62% is register-argument allocation only.
 */
static float pz_fighters_chomper2_fatality_prep(void) {
  static int attack_begun;
  PuzzleFatalityHazardObject *chomper;
  PuzzleFighterRenderObject *attacker_object;
  PuzzleFighterMove *victim_move;
  PuzzlePlayerData *attacker_data;
  PuzzlePlayerData *victim_data;
  float target_x;
  float target_z;
  float delta_x;
  float delta_z;
  float signed_distance;
  float player_distance;
  int direction;
  int event;
  int attacker;
  int victim;
  int hazard_ready;

  switch (g_pz_fighter_fatality_engine.active_effect) {
  case 0: {
    float edge;

    attacker = g_pz_fighters_engine.fatality_attacker;
    attacker_object = pz_fighter_get_player_obj(attacker);
    direction = 1;
    {
      Vec offset = {0.05f, 0.0f, 0.0f};
      if (attacker == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[1].x + offset.x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z + offset.z;
      } else {
        target_x = g_pz_fighters_engine.fighter_posts[0].x - offset.x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z - offset.z;
      }
    }

    delta_x = target_x - attacker_object->x;
    delta_z = target_z - attacker_object->z;
    if (target_x < 0.0f) {
      if (attacker_object->x < target_x) {
        direction = -1;
      }
    } else if (attacker_object->x > target_x) {
      direction = -1;
    }
    hazard_ready = 0;
    signed_distance = direction * ((delta_x * delta_x) + (delta_z * delta_z));

    edge = 1.15f;
    if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 0) {
      if (g_pz_fighter_fatality_engine.primary_object->x < -edge) {
        g_pz_fighter_fatality_engine.primary_object->external_force_x = 0.02f;
      } else {
        g_pz_fighter_fatality_engine.primary_object->external_force_x = 0.0f;
      }
    } else {
      if (g_pz_fighter_fatality_engine.secondary_object->x > edge) {
        g_pz_fighter_fatality_engine.secondary_object->external_force_x =
            -0.02f;
      } else {
        g_pz_fighter_fatality_engine.secondary_object->external_force_x = 0.0f;
      }
    }

    attacker = g_pz_fighters_engine.fatality_attacker;
    chomper = g_pz_fighter_fatality_engine.hazard_groups[attacker].objects[0];
    if (chomper->y < 2.3f) {
      chomper->motion = 0.1f;
    } else {
      chomper->motion = 0.0f;
    }
    attacker = g_pz_fighters_engine.fatality_attacker;
    if (g_pz_fighter_fatality_engine.hazard_groups[attacker]
                .objects[0]
                ->motion == 0.0f &&
        g_pz_fighter_fatality_engine.scene_objects[attacker]
                ->external_force_x == 0.0f) {
      hazard_ready = 1;
    }

    if (signed_distance < 0.02f) {
      attacker_data = pz_get_pdata_by_id(
          g_pz_fighters_engine.fatality_attacker);
      pz_fighter_clear_out_all_external_forces(g_game_info.player1_physics);
      pz_fighter_clear_out_all_external_forces(g_game_info.player2_physics);
      if (attack_begun == 0) {
        attack_begun = 1;
        event = 0;
        minigame_event(&event);
      }

      if (g_game_info.player1->transition_locked == 0 &&
          g_game_info.player2->transition_locked == 0 &&
          hazard_ready == 1) {
        player_distance = xz_distance_between_players();
        g_pz_fighter_fatality_engine.active_effect = 1;
        attack_begun = 0;
        if (player_distance < 2.7f) {
          xfer_proc(pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_victim),
                    pz_fighter_backflip_and_point);
        } else {
          xfer_proc(pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_victim),
                    pz_fighter_execute_point);
        }
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_attacker),
                  pz_fighter_execute_point_reaction_no_space);
        g_pz_fighter_fatality_engine.effect_timer = 100;
      } else if (attacker_data->transition_locked != 0) {
        attacker_data->transition_locked = 0;
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_attacker),
                  pz_fighter_exit);
        xfer_proc(
            pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
            pz_fighter_exit);
      }
      break;
    }

    if (g_game_info.player1->transition_locked != 0 ||
        g_game_info.player2->transition_locked != 0) {
      break;
    }
    if (attack_begun == 0) {
      attack_begun = 1;
      event = 0;
      minigame_event(&event);
    }
    victim = g_pz_fighters_engine.fatality_victim;
    victim_move = pz_get_fighter_move();
    g_pz_fighters_engine.fighter_reaction_cooldown = 0;
    victim_move->active = 1;
    if (signed_distance < 1.3f) {
      xfer_proc(pz_fighter_get_player_proc(victim),
                pz_fighter_fatality_medium_shove);
    } else {
      xfer_proc(pz_fighter_get_player_proc(victim),
                pz_fighter_fatality_huge_shove);
    }
    break;
  }

  case 1:
    attacker_data = pz_get_pdata_by_id(
        g_pz_fighters_engine.fatality_attacker);
    victim_data = pz_get_pdata_by_id(
        g_pz_fighters_engine.fatality_victim);
    if (victim_data->transition_locked != 0) {
      break;
    }
    if (attacker_data->transition_locked != 0) {
      if (g_pz_fighter_fatality_engine.effect_timer-- >= 0) {
        break;
      }
    }
    g_pz_fighter_fatality_engine.active_effect = 2;
    g_pz_fighters_engine.fatality_active = 1;
    g_pz_fighters_engine.fatality_timer = 125;
    break;
  }

  return 0.0f;
}

/*
 * Soft ceiling: the 88-byte source and retail bodies contain the same phase
 * store, two-iteration loop, hazard-object motion store, and controller-row
 * store. MWCC schedules the two induction increments earlier and forms the
 * indexed controller address in the opposite order; the zero constant also
 * carries a different TU-local relocation label.
 */
static float pz_fighter_chomper2_actively_fighting(int active) {
    int group;

    g_pz_fighter_fatality_engine.controller->phase = active;
    if (active == 0) {
        for (group = 0; group < 2; group++) {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[0]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][0] = 0.0f;
        }
    }
    return 0.0f;
}

/*
 * Soft ceiling: 96.15151% in the full-TU report. The automatic Vec restores
 * retail's 0x40 stack frame and complete instruction shape, while the TU's
 * pooled-string mode restores @stringBase0 addressing for both effects.
 * Residue is FPR/GPR allocation, one subtract scheduling choice, and local
 * constant/string relocation labels.
 */
static float pz_fighter_chomper2_victim_crushed(void) {
    Vec blood_offset = {0.05f, 0.0f, 0.0f};
    float blood_x;
    float blood_z;

    if (plyr_pdata->side == 0) {
        blood_x = g_pz_fighters_engine.fighter_posts[1].x + blood_offset.x;
        blood_z = g_pz_fighters_engine.fighter_posts[1].z + blood_offset.z;
    } else {
        blood_x = g_pz_fighters_engine.fighter_posts[0].x - blood_offset.x;
        blood_z = g_pz_fighters_engine.fighter_posts[0].z - blood_offset.z;
    }
    blood_z -= 0.2f;
    bgnd_launch_fx_at_position(
        "blood_crush_fx", blood_x, 0.1f, blood_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    init_ground_move();
    blend_to_ani(pz_shared_ani.objects_falling_crushed, 3, 0.1f);
    set_ani_speed(0.2f);
    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    obj_set_bone_collapse_flag(plyr_obj, 9);
    ani_loop_more_frames(2.0f);
    bgnd_launch_fx_at_position(
        "blood_splat_fx", blood_x, 0.1f, blood_z);
    ani_to_end();
    obj_set_bone_collapse_flag(plyr_obj, 0x14);
    obj_set_bone_collapse_flag(plyr_obj, 0x15);
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

static inline void pz_chomper_apply_motion(
    int side, int object_count, int fighting) {
    PuzzleFatalityHazardObject* object;
    float target;
    int object_index;

    for (object_index = 0; object_index < object_count; object_index++) {
        object = g_pz_fighter_fatality_engine
                     .hazard_groups[side].objects[object_index];
        target = g_pz_fighter_fatality_engine.controller
                     ->chomper_position[side][object_index];
        if ((object->y > target + 0.13f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[side][object_index] < 0.0f) ||
            (object->y < target - 0.13f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[side][object_index] > 0.0f)) {
            object->motion = g_pz_fighter_fatality_engine.controller
                                 ->hazard_motion[side][object_index];
        } else if (fighting == 0) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][object_index] = 0.0f;
            object->motion = 0.0f;
        } else if (g_pz_fighter_fatality_engine.controller->phase == 1) {
            object->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][object_index] = 0.0f;
        }
    }
}

static inline void pz_chomper_reverse_motion(
    int side, int object_count) {
    PuzzleFatalityHazardObject* object;
    int object_index;

    for (object_index = 0; object_index < object_count; object_index++) {
        object = g_pz_fighter_fatality_engine
                     .hazard_groups[side].objects[object_index];
        if (object->y > g_pz_fighter_fatality_engine.controller
                            ->chomper_position[side][object_index] &&
            g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[side][object_index] > 0.0f) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][object_index] *= -1.0f;
        }
    }
}

static inline void pz_chomper_start_motion(
    int side, float target, float motion) {
    g_pz_fighter_fatality_engine.controller
        ->chomper_position[side][0] = target;
    g_pz_fighter_fatality_engine.controller
        ->hazard_motion[side][0] = motion;
}

static inline void pz_chomper_update_state(
    unsigned int side, int fighting, int* motion_changed) {
    if (g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[side] == 0) {
        return;
    }
    switch (g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[side]) {
    case 1:
        if (fighting != 0) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[side] = 2;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][0] = 0.0f;
        } else {
            if (side == 0) {
                pan_vol_snd_req(0x1AAD, -0.5f, 1.0f);
            } else {
                pan_vol_snd_req(0x1AAD, 0.5f, 1.0f);
            }
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[side] = 2;
            pz_chomper_start_motion(side, 2.5f, 0.06f);
            *motion_changed = 1;
        }
        break;
    case 2:
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][0] == 0.0f) {
            if (side == 0) {
                pan_vol_snd_req(0x1AAC, -0.5f, 1.0f);
            } else {
                pan_vol_snd_req(0x1AAC, 0.5f, 1.0f);
            }
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[side] = 3;
            pz_chomper_start_motion(side, 0.0f, 0.2f);
            *motion_changed = 1;
        }
        break;
    case 3:
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][0] == 0.0f) {
            if (side == 0) {
                pan_vol_snd_req(0x1AAE, -0.5f, 1.0f);
            } else {
                pan_vol_snd_req(0x1AAE, 0.5f, 1.0f);
            }
            pz_fighter_shake_camera(1, 0.02f);
            g_pz_fighter_fatality_engine.controller
                ->hazard_initialized[side] = 4;
            pz_chomper_start_motion(side, 2.5f, 0.06f);
            *motion_changed = 1;
        }
        break;
    case 4:
        if (g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][0] == 0.0f) {
            if (fighting != 0) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[side] = 5;
                g_pz_fighter_fatality_engine.controller->preround_timer =
                    (int)(randu0(250) & 0xFFFF) + 250;
            } else {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[side] = 0;
            }
        }
        break;
    }
}

/*
 * Near match (99.01% report, retail 0x9D0/current 0x9D4). The controller is
 * recovered, including hazard ownership, store order, side-specific sound
 * branches, unsigned iteration, and fighting/preround zeroing order. One
 * extra instruction plus register/stack-slot coloring remains.
 */
static float p_chomper2_controller(void) {
    const int object_count = 1;
    const unsigned int bird_chance = 20;
    Vec bird_target_right;
    Vec bird_start_right;
    Vec bird_target_left;
    Vec bird_start_left;
    int motion_changed;
    unsigned int side;

    motion_changed = 0;
    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }
    if (g_pz_fighter_fatality_engine.controller->phase == 1) {
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 0 &&
            g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 0 &&
            (randu0(1000) & 0xFFFF) < 10) {
            if ((randu0(100) & 0xFFFF) < 50) {
                if ((randu0(100) & 0xFFFF) < bird_chance) {
                    bird_start_right.x = 640.0f;
                    bird_start_right.y = 230.0f;
                    bird_target_right.x = 50.0f;
                    bird_target_right.y = 60.0f;
                    pz_fighter_anim_object_to(
                        0, 1, 0, &bird_start_right, &bird_target_right,
                        0, 0, 120,
                        1.0f, 0.0f, 1,
                        pz_fighters_fatality_bird_in_place);
                    g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 6;
                } else {
                    pz_chomper_start_motion(1, 2.5f, 0.28f);
                    g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
                    motion_changed = 1;
                }
            } else if ((randu0(100) & 0xFFFF) < bird_chance) {
                bird_start_left.x = 0.0f;
                bird_start_left.y = 230.0f;
                bird_target_left.x = 580.0f;
                bird_target_left.y = 60.0f;
                pz_fighter_anim_object_to(
                    0, 0, 0, &bird_start_left, &bird_target_left,
                    0, 0, 120,
                    1.0f, 0.0f, 1,
                    pz_fighters_fatality_bird_in_place);
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 6;
            } else {
                pz_chomper_start_motion(0, 2.5f, 0.28f);
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
                motion_changed = 1;
            }
        }
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 8) {
            pz_chomper_start_motion(1, 2.5f, 0.55f);
            g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
            motion_changed = 1;
        }
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 7) {
            pz_chomper_start_motion(0, 2.5f, 0.55f);
            g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
            motion_changed = 1;
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_update_state(side, 1, &motion_changed);
        }
        if (g_pz_fighter_fatality_engine.controller->preround_timer != 0) {
            g_pz_fighter_fatality_engine.controller->preround_timer--;
            if (g_pz_fighter_fatality_engine.controller->preround_timer == 0) {
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
                g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
            }
        }
        if (motion_changed == 1) {
            for (side = 0; side < 2; side++) {
                pz_chomper_reverse_motion(side, object_count);
            }
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_apply_motion(side, object_count, 1);
        }
        return 1.0f;
    } else if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        switch (g_pz_fighter_fatality_engine.controller->preround_sound_started) {
        case 0:
            g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
            g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            break;
        case 1:
            if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 0) {
                g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
                g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            }
            break;
        case 2:
            if (g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 0) {
                g_pz_fighter_fatality_engine.controller->preround_sound_started++;
                return 1.0f;
            }
            break;
        case 3:
            if (g_pz_fighter_fatality_engine.hazard_groups[0]
                        .objects[0]->x <= -0.5f &&
                g_pz_fighter_fatality_engine.hazard_groups[1]
                        .objects[0]->x >= 0.5f) {
                g_pz_fighter_fatality_engine.controller->preround_active = 0;
                g_pz_fighter_fatality_engine.controller->preround_sound_started += 2;
                return 1.0f;
            }
            _mkproc_sleep_ticks = 20.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            return 1.0f;
        case 4:
            g_pz_fighter_fatality_engine.hazard_groups[0]
                .objects[0]->x -= 0.02f;
            g_pz_fighter_fatality_engine.hazard_groups[1]
                .objects[0]->x += 0.02f;
            if (g_pz_fighter_fatality_engine.hazard_groups[0]
                        .objects[0]->x <= -0.6f &&
                g_pz_fighter_fatality_engine.hazard_groups[1]
                        .objects[0]->x >= 0.6f) {
                g_pz_fighter_fatality_engine.controller->preround_active = 0;
                g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            }
            return 1.0f;
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_update_state(side, 0, &motion_changed);
        }
        if (motion_changed == 1) {
            for (side = 0; side < 2; side++) {
                pz_chomper_reverse_motion(side, object_count);
            }
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_apply_motion(side, object_count, 0);
        }
        return 1.0f;
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
        return 1.0f;
    }
}

static float pz_fighters_chomper2_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/*
 * Soft ceiling: the exact-size nested loop and controller initialization
 * match retail; only zero-register allocation/rematerialization differs.
 */
static void pz_fighter_chomper_entering_fatality(
    int attacker, int victim) {
    unsigned int group;
    int object;

    group = 0;
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

    for (; group < 2; group++) {
        for (object = 0; object < 2; object++) {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[object]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][object] = 0.0f;
        }
    }
}

static float pz_fighter_chomper_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

/*
 * Soft ceiling: retail and source have the same 128-byte nested loop and
 * state transitions. Residue is zero-register allocation and MWCC choosing
 * an inner-loop `li 0` instead of copying a retained zero value. A named
 * first-object bound compiled identically and was removed.
 */
static float pz_fighter_chomper_round_over(void) {
    unsigned int group;
    int object;

    group = 0;
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    for (; group < 2; group++) {
        for (object = 0; object < 2; object++) {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[object]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][object] = 0.0f;
        }
    }
    return 0.0f;
}

/*
 * Soft ceiling: the complete retail loader/state setup is present; residue is
 * loop induction/register scheduling plus pooled constant relocation labels.
 */
static float pz_fighter_load_and_place_initial_chompers(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* columns[2];
    PuzzleFatalityController* controller;
    PuzzleFatalityHazardObject* chomper;
    unsigned int i;
    unsigned int j;

    load_art_section(0x70036, &sec_pz_danger_chomper);
    g_pz_fighters_engine.fatality_index = 1;

    for (i = 0; i < 2; i++) {
        columns[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x081F0000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags_bits.scale_active = 1;
        if (screen_width > 650) {
            columns[i]->x = i != 0 ? 4.1f : -4.1f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.24f;
            columns[i]->scale.x = 1.1f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.14f;
        } else {
            columns[i]->x = i != 0 ? 3.6f : -3.6f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.24f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.04f;
        }
        columns[i]->flags_bits.gravity_enabled = 1;
        columns[i]->external_force_x = 0.0f;
        columns[i]->external_force_y = 0.0f;
        columns[i]->external_force_z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    g_pz_fighter_fatality_engine.primary_object = columns[0];
    g_pz_fighter_fatality_engine.secondary_object = columns[1];
    obj_create_sobjs(columns[0]);
    obj_create_sobjs(columns[1]);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(columns[0], 2);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(columns[1], 2);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[1] =
        obj_find_sobj_by_id(columns[0], 1);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[1] =
        obj_find_sobj_by_id(columns[1], 1);
    g_pz_fighter_fatality_engine.grinder_texture =
        load_tga(0x70036, 0x081F0002);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_chomper_controller, 0x80,
            &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->loop_sound = 0;
        g_pz_fighter_fatality_engine.controller = controller;
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            chomper =
                g_pz_fighter_fatality_engine.hazard_groups[i].objects[j];
            chomper->flags_bits.airborne = 1;
            chomper->flags_bits.gravity_enabled = 1;
            chomper->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->chomper_position[i][j] = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[i][j] = 0.0f;
        }
        g_pz_fighter_fatality_engine.controller
            ->hazard_initialized[i] = 0;
    }

    load_pz_fighter_fatality_bank(0x81);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_chomper_fx.mko", &effect_context);
    return 0.0f;
}

/*
 * Near match: 92.50%, retail 0x830/current 0x808. Automatic branch-local
 * vectors, direct fatality-piece loads, preserved pre-process globals, typed
 * flag assignments and the retail material null guard recover 292 bytes and
 * the complete m2c algorithm. Case 4 now preserves retail's player-object call
 * before indexed hazard lookup. The remaining 40 bytes are repeated hazard-base
 * formation, register allocation and equivalent switch-tail scheduling.
 */
static float pz_fighters_chomper_fatality_in_progress(void) {
    static int mode_timer;
    static int launch_sounds = 1;
    static float flesh_path_timer;
    PuzzleFatalityHazardObject* spike;
    PuzzleFighterRenderObject* attacker_object;
    PuzzlePlayerData* attacker_data;
    PuzzlePlayerData* victim_data;
    PuzzlePlayerData* saved_pdata;
    PuzzleFighterRenderObject* saved_object;
    PuzzleFaceBleedPdata* bleed_data;
    PuzzleFaceBleedProcess* bleed_process;
    Vec impact_position;
    RpMaterial* material;

    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 2: {
        Vec head_offset = {0.0f, 0.6f, 0.0f};
        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
        attacker_object =
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker);
        victim_data =
            pz_get_pdata_by_id(g_pz_fighters_engine.fatality_victim);
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_one_arm_victory2);
        }

        get_bone_world_pos(attacker_object, 0x10, &impact_position);
        impact_position.x += head_offset.x;
        impact_position.y += head_offset.y;
        impact_position.z += head_offset.z;
        if (spike->y > impact_position.y) {
            spike->motion = -0.2f;
            break;
        }

        spike->motion = 0.0f;
        g_pz_fighter_fatality_engine.active_effect = 3;
        plyr_pdata =
            pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
        plyr_obj = attacker_object;
        snd_death_voice();
        random_hit(5);
        pz_fighter_shake_camera(1, 0.01f);
        plyr_bleed_mouth(plyr_pdata);
        face_bleed_me(3);
        saved_pdata = plyr_pdata;
        saved_object = plyr_obj;

        bleed_process = _create_mkproc_generic_bigstack(
            0x2001, 0x1F, p_face_bleeding, sizeof(PuzzleFaceBleedPdata),
            &bleed_data);
        if (bleed_process != 0 && bleed_data != 0) {
            bleed_data->duration = 200;
            bleed_data->interval = 20;
            bleed_data->state = 0;
            bleed_data->object = saved_object;
            bleed_data->player_data = saved_pdata;
            bleed_process->wait_routine = pw_face_bleeding;
            bleed_process->script_routine = ps_face_bleeding;
        }
        xfer_proc(
            pz_fighter_get_player_proc(
                g_pz_fighters_engine.fatality_attacker),
            pz_fighter_victim_head_poked);
        break;
    }

    case 3:
        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
        attacker_object =
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker);
        victim_data =
            pz_get_pdata_by_id(g_pz_fighters_engine.fatality_victim);
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_one_arm_victory2);
        }
        get_bone_world_pos(attacker_object, 0x10, &impact_position);

        if (spike->y < 3.95f) {
            spike->motion = 0.12f;
            attacker_object->secondary_flags_bits.stopped = 0;
            attacker_object->external_force_y = 0.09f;
            mode_timer = 0;
        } else if (mode_timer == 0) {
            spike->motion = 0.0f;
            attacker_object->external_force_y = 0.0f;
            mode_timer = 200;
        } else if (mode_timer == 120) {
            attacker_object->flags_bits.moving = 1;
            attacker_object->hazard_x = -0.002f;
            mode_timer--;
        } else if (mode_timer == 70) {
            attacker_object->flags_bits.moving = 0;
            attacker_object->hazard_x = 0.0f;
            mode_timer--;
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_attacker),
                pz_fighter_victim_fatality_bouncy);
        } else if (mode_timer == 1) {
            snd_req(0x1AAC);
            g_pz_fighter_fatality_engine.active_effect = 4;
            mode_timer = 0;
        } else {
            mode_timer--;
        }
        break;

    case 4:
        attacker_object =
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker);
        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
        if (spike->y > 1.25f) {
            spike->motion = -0.4f;
            attacker_object->secondary_flags_bits.stopped = 0;
            launch_sounds = 1;
        } else if (spike->y > 0.35f) {
            spike->motion = -0.28f;
            attacker_object->secondary_flags_bits.stopped = 0;
            if (launch_sounds == 1) {
                snd_req(0x1AD6);
                launch_sounds = 0;
            }
        } else {
            spike->motion = 0.0f;
            attacker_object->external_force_y = 0.0f;
            pz_fighter_shake_camera(1, 0.03f);
            snd_req(0x1ADB);
            snd_req_vol(0x1AAE, 1.0f);
            snd_req_delay(0x1AAF, 7);

            if (spike != 0) {
                material = sobj_find_material_with_texture(spike, "spikes");
                if (material != 0) {
                    RpMaterialSetTexture(
                        material,
                        g_pz_fighter_fatality_engine.grinder_texture);
                }
            }
            plyr_pdata =
                pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
            snd_major_hit_voice();
            g_pz_fighter_fatality_engine.active_effect = 5;
            if (g_pz_fighters_engine.fatality_attacker == 1) {
                g_pz_fighters_engine.fatality_piece->x = 1.56f;
                g_pz_fighters_engine.fatality_piece->z = -0.27f;
            } else {
                g_pz_fighters_engine.fatality_piece->x = -1.62f;
                g_pz_fighters_engine.fatality_piece->z = -0.28f;
            }
            g_pz_fighters_engine.fatality_piece->y = 0.2f;
            g_pz_fighters_engine.fatality_piece->flags |= 0x20;
            g_pz_fighters_engine.fatality_piece->external_force_x = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force_y = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force_z = 0.0f;
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_attacker),
                pz_fighter_chomper_victim_crushed);
        }
        break;

    case 5:
        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
        if (spike->y < 3.55f) {
            spike->motion = 0.3f;
            g_pz_fighters_engine.fatality_piece->external_force_y = 0.206f;
            unhide_obj(g_pz_fighters_engine.fatality_piece);
            if ((randu0(100) & 0xFFFF) < 70) {
                Vec velocity = {0.0f, -0.001f, 0.0f};
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->x,
                    &velocity, attacker_data);
            }
        } else if (spike->y < 3.95f) {
            spike->motion = 0.17f;
            g_pz_fighters_engine.fatality_piece->external_force_y = 0.138f;
            flesh_path_timer = 5.0f;
            if ((randu0(100) & 0xFFFF) < 70) {
                Vec velocity = {0.0f, -0.001f, 0.0f};
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->x,
                    &velocity, attacker_data);
            }
        } else {
            flesh_path_timer -= 1.0f;
            spike->motion = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force_y = 0.0f;
            if (g_pz_fighters_engine.fatality_motion == 1.0f &&
                flesh_path_timer <= 0.0f) {
                Vec flesh_velocity = {0.0f, 0.1f, 0.0f};
                Vec flesh_terminal_velocity = {0.05f, 0.05f, -0.05f};
                ft_create_flesh_path(
                    g_pz_fighters_engine.fatality_piece,
                    &g_pz_fighters_engine.fatality_piece->position, 1, 0,
                    &flesh_velocity, 0, &flesh_terminal_velocity,
                    dropped_heart_snd_cb, -0.0015f, 0.7f, 0.1f);
                g_pz_fighter_fatality_engine.active_effect = 6;
            }
            if ((randu0(100) & 0xFFFF) < 40) {
                Vec velocity = {0.0f, -0.001f, 0.0f};
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->x,
                    &velocity, attacker_data);
            }
        }
        break;

    case 6:
        if ((randu0(100) & 0xFFFF) < 8) {
            Vec velocity = {0.0f, -0.001f, 0.0f};
            attacker_data =
                pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
            spawn_bld_fall(
                "gusher0", 0, &g_pz_fighters_engine.fatality_piece->x,
                &velocity, attacker_data);
            velocity.x = sfrand(0.1f);
            velocity.z = sfrand(0.1f);
            spawn_bld_fall(
                "gusher0", 0, &g_pz_fighters_engine.fatality_piece->x,
                &velocity, attacker_data);
        }
        break;
    }

    return 0.0f;
}

/*
 * The success and abort paths intentionally retain separate event guards, as
 * emitted by retail. Exact retail size; objdiff's remaining 0.58% is
 * register-argument allocation only.
 */
static float pz_fighters_chomper_fatality_prep(void) {
  static int attack_begun;
  PuzzleFatalityHazardObject *chomper;
  PuzzleFighterRenderObject *attacker_object;
  PuzzleFighterMove *victim_move;
  PuzzlePlayerData *attacker_data;
  float edge = 2.05f;
  float target_x;
  float target_z;
  float delta_x;
  float delta_z;
  float signed_distance;
  float player_distance;
  int direction;
  int event;
  int attacker;
  int victim;
  int hazard_ready;

  if (screen_width > 650) {
    edge = 2.3f;
  }

  switch (g_pz_fighter_fatality_engine.active_effect) {
  case 0: {
    attacker = g_pz_fighters_engine.fatality_attacker;
    attacker_object = pz_fighter_get_player_obj(attacker);
    direction = 1;
    {
      Vec offset = {0.5f, 0.0f, 0.0f};
      if (attacker == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[1].x + offset.x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z + offset.z;
      } else {
        target_x = g_pz_fighters_engine.fighter_posts[0].x - offset.x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z - offset.z;
      }
    }

    delta_x = target_x - attacker_object->x;
    delta_z = target_z - attacker_object->z;
    if (target_x < 0.0f) {
      if (attacker_object->x < target_x) {
        direction = -1;
      }
    } else if (attacker_object->x > target_x) {
      direction = -1;
    }
    hazard_ready = 0;
    signed_distance = direction * ((delta_x * delta_x) + (delta_z * delta_z));

    if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 0) {
      if (g_pz_fighter_fatality_engine.primary_object->x < -edge) {
        g_pz_fighter_fatality_engine.primary_object->external_force_x = 0.02f;
      } else {
        g_pz_fighter_fatality_engine.primary_object->external_force_x = 0.0f;
      }
    } else {
      if (g_pz_fighter_fatality_engine.secondary_object->x > edge) {
        g_pz_fighter_fatality_engine.secondary_object->external_force_x =
            -0.02f;
      } else {
        g_pz_fighter_fatality_engine.secondary_object->external_force_x = 0.0f;
      }
    }

    attacker = g_pz_fighters_engine.fatality_attacker;
    chomper = g_pz_fighter_fatality_engine.hazard_groups[attacker].objects[0];
    if (chomper->y < 2.5f) {
      chomper->motion = 0.1f;
    } else {
      chomper->motion = 0.0f;
    }
    attacker = g_pz_fighters_engine.fatality_attacker;
    if (g_pz_fighter_fatality_engine.hazard_groups[attacker]
                .objects[0]
                ->motion == 0.0f &&
        g_pz_fighter_fatality_engine.scene_objects[attacker]
                ->external_force_x == 0.0f) {
      hazard_ready = 1;
    }

    if (signed_distance < 0.02f) {
      attacker_data = pz_get_pdata_by_id(
          g_pz_fighters_engine.fatality_attacker);
      pz_fighter_clear_out_all_external_forces(g_game_info.player1_physics);
      pz_fighter_clear_out_all_external_forces(g_game_info.player2_physics);
      if (attack_begun == 0) {
        attack_begun = 1;
        event = 0;
        minigame_event(&event);
      }

      if (g_game_info.player1->transition_locked == 0 &&
          g_game_info.player2->transition_locked == 0 &&
          hazard_ready == 1) {
        player_distance = xz_distance_between_players();
        g_pz_fighter_fatality_engine.active_effect = 1;
        attack_begun = 0;
        if (player_distance < 2.7f) {
          xfer_proc(pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_victim),
                    pz_fighter_just_backflip);
        } else {
          xfer_proc(pz_fighter_get_player_proc(
                        g_pz_fighters_engine.fatality_victim),
                    pz_fighter_perform_taunt);
        }
        g_pz_fighter_fatality_engine.effect_timer = 35;
      } else if (attacker_data->transition_locked != 0) {
        if (attack_begun == 0) {
          attack_begun = 1;
          event = 0;
          minigame_event(&event);
        }
        attacker_data->transition_locked = 0;
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_attacker),
                  pz_fighter_exit);
        xfer_proc(
            pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
            pz_fighter_exit);
      }
      break;
    }

    if (g_game_info.player1->transition_locked != 0 ||
        g_game_info.player2->transition_locked != 0) {
      break;
    }
    if (attack_begun == 0) {
      attack_begun = 1;
      event = 0;
      minigame_event(&event);
    }
    if ((unsigned int)g_pz_fighters_engine.fatality_victim == 1) {
      g_game_info.player1->fatality_shove_active = 1;
    } else {
      g_game_info.player2->fatality_shove_active = 1;
    }
    victim = g_pz_fighters_engine.fatality_victim;
    victim_move = pz_get_fighter_move();
    g_pz_fighters_engine.fighter_reaction_cooldown = 0;
    victim_move->active = 1;
    if (signed_distance < 1.3f) {
      xfer_proc(pz_fighter_get_player_proc(victim),
                pz_fighter_fatality_medium_shove);
    } else {
      xfer_proc(pz_fighter_get_player_proc(victim),
                pz_fighter_fatality_huge_shove);
    }
    break;
  }

  case 1:
    g_pz_fighter_fatality_engine.effect_timer--;
    if (g_pz_fighter_fatality_engine.effect_timer != 0) {
      break;
    }
    g_pz_fighter_fatality_engine.active_effect = 2;
    g_pz_fighters_engine.fatality_active = 1;
    if ((randu0(100) & 0xFFFF) < 30) {
      g_pz_fighters_engine.fatality_motion = 1.0f;
      g_pz_fighters_engine.fatality_timer = 315;
    } else {
      g_pz_fighters_engine.fatality_motion = 0.0f;
      g_pz_fighters_engine.fatality_timer = 285;
    }
    break;
  }

  return 0.0f;
}

/*
 * Soft ceiling: 99.666664%. The instruction stream and control flow match;
 * objdiff only reports the TU-local 0.0f relocation label (@91 vs @2542).
 */
static float pz_fighter_chomper_actively_fighting(int active) {
    unsigned int group;
    int object;

    g_pz_fighter_fatality_engine.controller->phase = active;
    if (active == 0) {
        for (group = 0; group < 2; group++) {
            for (object = 0; object < 2; object++) {
                g_pz_fighter_fatality_engine.hazard_groups[group]
                    .objects[object]
                    ->motion = 0.0f;
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[group][object] = 0.0f;
            }
        }
    }
    return 0.0f;
}

static float pz_fighter_chomper_victim_crushed(void) {
    Vec head_position;
    Vec ankle_position;

    get_bone_world_pos(plyr_obj, 0x10, &head_position);
    get_bone_world_pos(plyr_obj, 7, &ankle_position);
    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    obj_set_bone_collapse_flag(plyr_obj, 4);
    obj_set_bone_collapse_flag(plyr_obj, 0x15);
    bgnd_launch_fx_at_position(
        "chunk_head_fx", head_position.x, head_position.y, head_position.z);
    bgnd_launch_fx_at_position(
        "chunk_ankle_fx", ankle_position.x, ankle_position.y,
        ankle_position.z);
    bgnd_launch_fx_at_position(
        "blood_up_spray_fx", plyr_obj->x, plyr_obj->y, plyr_obj->z);
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

static float pz_fighter_victim_head_poked(void) {
    unsigned int i;

    for (i = 0; i < 10; i++) {
        blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
        plyr_anim_pdata->animation_step = 0.0f;
        ani_to_end();
    }
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighters_chomper_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/*
 * Emission-only near match (93.72%, retail 0x958/current 0x954). m2c confirms
 * both complete two-sided chomper state machines, bird launch paths, timers,
 * sound/camera calls, and nested motion loops. The side induction variable is
 * unsigned, matching retail's cmplwi loop tests. Remaining differences are
 * localized register/stack-slot coloring and equivalent helper scheduling.
 */
static float p_chomper_controller(void) {
    Vec bird_target_right;
    Vec bird_start_right;
    Vec bird_target_left;
    Vec bird_start_left;
    int motion_changed;
    unsigned int side;

    motion_changed = 0;
    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }
    if (g_pz_fighter_fatality_engine.controller->phase == 1) {
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 0 &&
            g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 0 &&
            (randu0(1000) & 0xFFFF) < 10) {
            if ((randu0(100) & 0xFFFF) < 50) {
                if ((randu0(100) & 0xFFFF) < 5) {
                    bird_start_right.x = 640.0f;
                    bird_start_right.y = 230.0f;
                    bird_target_right.x = 50.0f;
                    bird_target_right.y = 60.0f;
                    pz_fighter_anim_object_to(
                        0, 1, 0, &bird_start_right, &bird_target_right,
                        0, 0, 120,
                        1.0f, 0.0f, 1,
                        pz_fighters_fatality_bird_in_place);
                    g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 6;
                } else {
                    g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
                    pz_chomper_start_motion(1, 2.5f, 0.28f);
                    motion_changed = 1;
                }
            } else if ((randu0(100) & 0xFFFF) < 5) {
                bird_start_left.x = 0.0f;
                bird_start_left.y = 230.0f;
                bird_target_left.x = 580.0f;
                bird_target_left.y = 60.0f;
                pz_fighter_anim_object_to(
                    0, 0, 0, &bird_start_left, &bird_target_left,
                    0, 0, 120,
                    1.0f, 0.0f, 1,
                    pz_fighters_fatality_bird_in_place);
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 6;
            } else {
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
                pz_chomper_start_motion(0, 2.5f, 0.28f);
                motion_changed = 1;
            }
        }
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 8) {
            g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
            pz_chomper_start_motion(1, 2.5f, 0.55f);
            motion_changed = 1;
        }
        if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 7) {
            g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
            pz_chomper_start_motion(0, 2.5f, 0.55f);
            motion_changed = 1;
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_update_state(side, 1, &motion_changed);
        }
        if ((int)g_pz_fighter_fatality_engine.controller->preround_timer != 0) {
            g_pz_fighter_fatality_engine.controller->preround_timer--;
            if ((int)g_pz_fighter_fatality_engine.controller->preround_timer == 0) {
                g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
                g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
            }
        }
        if (motion_changed == 1) {
            for (side = 0; side < 2; side++) {
                pz_chomper_reverse_motion(side, 2);
            }
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_apply_motion(side, 2, 1);
        }
        return 1.0f;
    } else if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        switch (g_pz_fighter_fatality_engine.controller->preround_sound_started) {
        case 0:
            g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 1;
            g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            break;
        case 1:
            if (g_pz_fighter_fatality_engine.controller->hazard_initialized[0] == 0) {
                g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
                g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            }
            break;
        case 2:
            if (g_pz_fighter_fatality_engine.controller->hazard_initialized[1] == 0) {
                g_pz_fighter_fatality_engine.controller->preround_active = 0;
                g_pz_fighter_fatality_engine.controller->preround_sound_started++;
            }
            break;
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_update_state(side, 0, &motion_changed);
        }
        if (motion_changed == 1) {
            for (side = 0; side < 2; side++) {
                pz_chomper_reverse_motion(side, 2);
            }
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_apply_motion(side, 2, 0);
        }
        return 1.0f;
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
        return 1.0f;
    }
}

static int dropped_heart_snd_cb(void) {
    snd_req(0x1ADD);
    return 0;
}

static void pz_fighters_fatality_bird_in_place(int x, int y) {
    if (x > 300) {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 8;
        random_snd_req(0xB1);
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 7;
        random_snd_req(0xB1);
    }
}

/* Soft ceiling: pz_fighter_grinder_entering_fatality ~99.71% -- stop. */
static void pz_fighter_grinder_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->loop_sound =
        snd_req_vol(0x1AB7, 0.6f);
    g_pz_fighter_fatality_engine.controller->phase_time = 0.6f;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

static float pz_fighter_grinder_unload(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_grinder_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

/*
 * Soft ceiling: exact-size retail algorithm; only register allocation and
 * pooled string/float relocation labels remain.
 */
static float pz_fighter_load_and_place_initial_grinders(void) {
    PuzzleEffectBankContext effect_context;
    PuzzleFighterRenderObject* grinders[2];
    PuzzleFatalityController* controller;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_grinder);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_grinder_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 0;

    for (i = 0; i < 2; i++) {
        grinders[i] = (PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x081E0000, 0x6021);
        grinders[i]->model_flags = 1;
        grinders[i]->x = i != 0 ? 2.2f : -2.2f;
        if (screen_width > 650) {
            grinders[i]->x = i != 0 ? 2.65f : -2.65f;
        }
        grinders[i]->y = 0.0f;
        grinders[i]->z = 0.0f;
        grinders[i]->flags_bits.rotation_enabled = 1;
        grinders[i]->motion_rate = 0.05f;
        grinders[i]->flags_bits.scale_active = 1;
        grinders[i]->scale.x = 0.6f;
        grinders[i]->scale.y = 0.6f;
        grinders[i]->scale.z = 0.6f;
        insert_fgnd_mkobj(grinders[i]);
    }

    g_pz_fighter_fatality_engine.primary_object = grinders[0];
    g_pz_fighter_fatality_engine.secondary_object = grinders[1];
    obj_create_sobjs(grinders[0]);
    obj_create_sobjs(grinders[1]);
    g_pz_fighter_fatality_engine.grinder_texture =
        load_tga(0x70036, 0x081E0002);
    controller = 0;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_controller, 0x80, &controller) != 0 &&
        controller != 0) {
        controller->unload_requested = 0;
        controller->state = 0;
        controller->substate = 0;
        controller->active = 0;
        controller->phase = 0;
        controller->preround_active = 0;
        controller->controller_step = 0;
        controller->loop_sound = 0;
        g_pz_fighter_fatality_engine.controller = controller;
        controller->grinder_position[0] =
            g_pz_fighter_fatality_engine.primary_object->motion_rate;
        controller->grinder_position[1] =
            g_pz_fighter_fatality_engine.secondary_object->motion_rate;
        g_pz_fighter_fatality_engine.controller->grinder_target[0] =
            g_pz_fighter_fatality_engine.controller->grinder_position[0];
        g_pz_fighter_fatality_engine.controller->grinder_target[1] =
            g_pz_fighter_fatality_engine.controller->grinder_position[1];
    }

    load_pz_fighter_fatality_bank(0x82);
    return 0.0f;
}

/* Soft ceiling: pz_fighters_grinder_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
static float pz_fighters_grinder_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/*
 * Near match: 99.64%, exact 0xE0 size. Explicitly preserving both u16 random
 * results and computing the delay before the second call restores retail's
 * instruction stream; only four register arguments differ.
 */
static float pz_fighters_grinder_fatality_preround(void) {
    PuzzleGrinderMeatController* meat_controller;
    unsigned short direction_random;
    unsigned int delay;
    unsigned int direction;

    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->loop_sound =
        snd_req_vol(0x1AB7, 0.3f);
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;

    delay = (unsigned short)randu0(10) + 10;
    direction_random = randu0(100);
    direction = (unsigned int)(direction_random - 50) >> 31;
    meat_controller = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_meat_throw_controller, 0x18,
            &meat_controller) != 0 &&
        meat_controller != 0) {
        meat_controller->direction = direction;
        meat_controller->delay = delay;
        meat_controller->phase = 0;
    }
    return 0.0f;
}

/*
 * Near match: 95.10%, retail 0x244/current 0x248. Direct global reloads,
 * victim lifetime, and the shared success/attack-selection tail recover the
 * retail CFG. Residue is one zero lifetime plus FPR/GPR allocation and
 * equivalent instruction scheduling.
 */
static float pz_fighters_grinder_fatality_prep(void) {
    static int attack_begun;
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterMove* victim_move;
    float target_x;
    float target_z;
    float delta_x;
    float delta_z;
    float signed_distance;
    int direction;
    int event;
    int attacker;
    int victim;

    attacker = g_pz_fighters_engine.fatality_attacker;
    attacker_object = pz_fighter_get_player_obj(attacker);
    if (attacker == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[1].x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z;
    } else {
        target_x = g_pz_fighters_engine.fighter_posts[0].x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z;
    }

    delta_x = target_x - attacker_object->x;
    delta_z = target_z - attacker_object->z;
    direction = 1;
    if (target_x < 0.0f) {
        if (attacker_object->x >= target_x) {
            direction = -1;
        }
    } else if (attacker_object->x > target_x) {
        direction = -1;
    }
    signed_distance =
        direction * ((delta_x * delta_x) + (delta_z * delta_z));

    if (signed_distance < 0.014f &&
        g_pz_fighters_engine.fatality_ready == 1) {
        if (attack_begun == 0) {
            event = 0;
            minigame_event(&event);
        }
        attack_begun = 0;
        g_pz_fighters_engine.fatality_active = 1;
        xfer_proc(pz_fighter_get_player_proc(attacker),
                  r_pz_fighter_grinding);
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_victim),
                  pz_fighter_disgusted_with_grinding);
        g_pz_fighters_engine.fatality_timer = 180;
    } else if (g_game_info.player1->transition_locked == 0 &&
               g_game_info.player2->transition_locked == 0) {
        if (attack_begun == 0) {
            attack_begun = 1;
            event = 0;
            minigame_event(&event);
        }
        victim = g_pz_fighters_engine.fatality_victim;
        victim_move = pz_get_fighter_move();
        victim_move->active = 1;
        g_pz_fighters_engine.fighter_reaction_cooldown = 0;
        if (signed_distance < 0.19f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_good_solid_kick);
        } else if (signed_distance < 1.3f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_medium_shove);
        } else {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_huge_shove);
        }
    }
    return 0.0f;
}

static float pz_fighter_grinder_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static inline void pz_grinder_scale_launch_vector(
    Vec* output, const RwV3d* input, float scale) {
    output->x = input->x * scale;
    output->y = input->y * scale;
    output->z = input->z * scale;
}

static inline void pz_grinder_launch_piece(
    PuzzleFighterRenderObject* object, int bone, float height,
    float velocity_x, float velocity_y, float velocity_z,
    const Vec* terminal) {
    MKMATRIX* bone_matrix;
    Vec position;
    Vec velocity;
    float direction = 1.0f;

    if (plyr_pdata->side == 1) {
        direction = -1.0f;
    }

    bone_matrix = force_calc_bone_world_mat(plyr_obj, bone);
    RwFrameTransform(object->frame, bone_matrix, 0);
    get_bone_world_pos(plyr_obj, bone, &position);
    position.y = height;
    unhide_obj(object);
    pz_grinder_scale_launch_vector(&velocity, &bone_matrix->at, 0.1f);
    velocity.x = velocity_x * direction;
    velocity.y = velocity_y;
    velocity.z = velocity_z * direction;
    ft_create_flesh_path(
        object, &position, 0, 0, &velocity, 0, terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

/*
 * Emission-only near miss (88.77%, retail 0x874/current 0x870). Each launch
 * uses the same typed inline vector-scale helper that retail expands into a
 * nine-instruction RwMatrix.at-to-velocity sequence before the piece-specific
 * overrides. This recovers all four 0x28 islands and retail's r28 matrix
 * lifetime without forcing registers or volatility. The 11-argument
 * ft_create_flesh_path ABI and every transform, launch, material, timing, and
 * process operation agree. The sole size residue is one redundant retail
 * plyr_pdata->side reload; remaining differences are its localized register
 * allocation and instruction scheduling.
 */
float r_pz_fighter_grinding(void) {
    PuzzleGrinderNoisePdata* noise;
    PuzzleGrinderMeatController* meat;
    Vec final_position;
    Vec final_velocity;
    float effect_x;
    float effect_z;
    PuzzleFatalityHazardObject* body_sobj;

    pz_fighter_clear_out_external_forces();
    if (plyr_pdata->side == 0) {
        effect_x = g_pz_fighters_engine.fighter_posts[1].x;
        effect_z = g_pz_fighters_engine.fighter_posts[1].z;
    } else {
        effect_x = g_pz_fighters_engine.fighter_posts[0].x;
        effect_z = g_pz_fighters_engine.fighter_posts[0].z;
    }
    if (plyr_pdata->side == 0) {
        effect_x -= 0.4f;
    } else {
        effect_x += 0.4f;
    }

    bgnd_launch_fx_at_position(
        "grinding_fx", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    bgnd_launch_fx_at_position(
        "blood_up_spray_fx", plyr_obj->x, 0.0f, plyr_obj->z);
    fx_set_render_priority(fx_by_owner("blood_up_spray_fx", 4), 11);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    snd_req_vol(0x1AB8, 1.0f);

    if (_create_mkproc_generic_bigstack(
            0x2001, 0x1F, p_grinder_noise, 0x10,
            (PuzzleFaceBleedPdata**)&noise) != 0 &&
        noise != 0) {
        noise->duration = 150;
        noise->timer = 0;
    }

    blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(12.0f);
    bgnd_launch_fx_at_position(
        "chunk_1", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    snd_death_voice();
    ani_to_frame_x(22.0f);

    {
        Vec terminal = {0.04f, 0.08f, 0.03f};
        pz_grinder_launch_piece(
            g_pz_fighters_engine.grinder_meat_alt0, 9, 0.8f,
            0.02f, 0.05f, 0.003f, &terminal);
    }
    snd_req_delay(0x1ADD, 40);
    body_sobj = obj_first_sobj(
        g_pz_fighter_fatality_engine.scene_objects[plyr_pdata->side]);
    if (body_sobj != 0 && body_sobj->material_data->geometry != 0) {
        RpGeometryForAllMaterials(
            body_sobj->material_data->geometry, material_set_texture,
            g_pz_fighter_fatality_engine.grinder_texture);
    }
    ani_to_frame_x(36.0f);

    {
        Vec terminal = {0.02f, 0.02f, 0.07f};
        pz_grinder_launch_piece(
            g_pz_fighters_engine.grinder_meat_alt2, 20, 0.8f,
            0.02f, 0.03f, 0.003f, &terminal);
    }
    obj_set_bone_collapse_flag(plyr_obj, 20);
    snd_req(0x1ADC);
    snd_req_delay(0x1ADD, 35);
    snd_req(0x1AEF);
    bgnd_launch_fx_at_position(
        "chunk_2", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    ani_to_end();
    blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(26.0f);

    {
        Vec terminal = {0.04f, 0.08f, 0.03f};
        pz_grinder_launch_piece(
            g_pz_fighters_engine.grinder_meat_alt1, 9, 0.45f,
            0.04f, 0.04f, 0.003f, &terminal);
    }
    {
        Vec terminal = {0.04f, 0.08f, 0.03f};
        pz_grinder_launch_piece(
            g_pz_fighters_engine.grinder_meat_default, 9, 0.6f,
            0.04f, 0.08f, -0.005f, &terminal);
    }
    snd_req(0x1AD7);
    snd_req_delay(0x1ADD, 35);
    snd_req_delay(0x1ADD, 50);
    ani_to_frame_x(34.0f);

    final_position.x = 2.05f;
    final_position.y = 1.8f;
    final_position.z = 0.0f;
    final_velocity.x = -0.0625f;
    final_velocity.y = 0.105f;
    final_velocity.z = 0.0f;
    if (screen_width > 650) {
        final_velocity.x *= 1.1f;
    }
    if (plyr_pdata->side == 0) {
        final_position.x *= -1.0f;
        final_velocity.x *= -1.0f;
    }
    {
        PuzzleFighterRenderObject* final_object =
            g_pz_fighters_engine.grinder_meat_final;
        Vec final_terminal = {0.04f, 0.08f, 0.03f};
        unhide_obj(final_object);
        ft_create_flesh_path(
            final_object, &final_position, 0, 0,
            &final_velocity, 0, &final_terminal, 0, -0.004f, 0.5f, 0.1f);
    }

    obj_set_bone_collapse_flag(plyr_obj, 21);
    meat = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_meat_throw_controller, 0x18,
            &meat) != 0 &&
        meat != 0) {
        meat->direction = his_pdata->side;
        meat->object = g_pz_fighters_engine.grinder_meat_final;
        meat->delay = 0;
        meat->phase = 1;
    }

    bgnd_launch_fx_at_position(
        "chunk_3", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    ani_to_blend_frame(10.0f);
    hide_obj(plyr_obj);
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

static float p_grinder_noise(void) {
    PuzzleGrinderNoisePdata* noise = (PuzzleGrinderNoisePdata*)apdata;
    unsigned int timer;

    timer = noise->timer + 1;
    noise->timer = timer;
    if (timer >= noise->duration) {
        return -1.0f;
    }
    if ((noise->timer % 20) == 0) {
        snd_req_vol((randu0(3) & 0xFFFF) + 0x1AEB, 1.0f);
    }
    return 1.0f;
}

/*
 * Soft ceiling: 99.62%; paired grinder phase targets and loop-sound fade
 * agree, with only register/scheduling residue.
 */
static float p_grinder_controller(void) {
    int changed = 0;
    int i;

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->active == 1) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        changed = 1;
        g_pz_fighter_fatality_engine.controller->grinder_target[
            g_pz_fighter_fatality_engine.controller->victim_player] = 0.3f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[
            g_pz_fighter_fatality_engine.controller->victim_player][0] =
            0.002f;
        g_pz_fighter_fatality_engine.controller->grinder_target[
            g_pz_fighter_fatality_engine.controller->attacker_player] = 0.2f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[
            g_pz_fighter_fatality_engine.controller->attacker_player][0] =
            0.001f;
    } else if (g_pz_fighter_fatality_engine.controller->substate == 1) {
        g_pz_fighter_fatality_engine.controller->substate = 0;
        g_pz_fighter_fatality_engine.controller->controller_step = 1;
        changed = 1;
        g_pz_fighter_fatality_engine.controller->grinder_target[
            g_pz_fighter_fatality_engine.controller->victim_player] = 0.05f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[
            g_pz_fighter_fatality_engine.controller->victim_player][0] =
            0.002f;
        g_pz_fighter_fatality_engine.controller->grinder_target[
            g_pz_fighter_fatality_engine.controller->attacker_player] = 0.05f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[
            g_pz_fighter_fatality_engine.controller->attacker_player][0] =
            0.001f;
    }

    if (g_pz_fighter_fatality_engine.controller->controller_step == 1) {
        if (g_pz_fighter_fatality_engine.controller->phase_time > 0.05f) {
            g_pz_fighter_fatality_engine.controller->phase_time -= 0.005f;
            set_snd_vol(
                g_pz_fighter_fatality_engine.controller->loop_sound,
                0x1AB7,
                g_pz_fighter_fatality_engine.controller->phase_time);
        } else if (g_pz_fighter_fatality_engine.controller->loop_sound != 0) {
            snd_stop(g_pz_fighter_fatality_engine.controller->loop_sound);
            g_pz_fighter_fatality_engine.controller->loop_sound = 0;
        }
    }

    if (g_pz_fighter_fatality_engine.controller->phase == 1) {
        if (g_pz_fighters_engine.balance < -0.5f) {
            changed = 1;
            g_pz_fighter_fatality_engine.controller->grinder_target[1] =
                0.25f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[1][0] =
                0.001f;
        } else if (g_pz_fighters_engine.balance > -0.1f) {
            changed = 1;
            g_pz_fighter_fatality_engine.controller->grinder_target[1] =
                0.18f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[1][0] =
                0.001f;
        }

        if (g_pz_fighters_engine.balance > 0.5f) {
            changed = 1;
            g_pz_fighter_fatality_engine.controller->grinder_target[0] =
                0.25f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[0][0] =
                0.001f;
        } else if (g_pz_fighters_engine.balance < 0.1f) {
            changed = 1;
            g_pz_fighter_fatality_engine.controller->grinder_target[0] =
                0.18f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[0][0] =
                0.001f;
        }
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1 ||
        (g_pz_fighter_fatality_engine.controller->phase == 1 &&
         g_pz_fighter_fatality_engine.controller->loop_sound != 0)) {
        changed = 1;
        g_pz_fighter_fatality_engine.controller->grinder_target[0] = 0.25f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[0][0] =
            0.002f;
        g_pz_fighter_fatality_engine.controller->grinder_target[1] = 0.25f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[1][0] =
            0.002f;

        if (g_pz_fighter_fatality_engine.controller->preround_timer == 0) {
            changed = 1;
            g_pz_fighter_fatality_engine.controller->grinder_target[0] =
                0.1f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[0][0] =
                0.0012f;
            g_pz_fighter_fatality_engine.controller->grinder_target[1] =
                0.1f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[1][0] =
                0.0012f;
        } else {
            g_pz_fighter_fatality_engine.controller->preround_timer--;
        }

        if (g_pz_fighter_fatality_engine.controller->preround_timer != 0) {
            if (g_pz_fighter_fatality_engine.controller->phase_time < 0.7f) {
                g_pz_fighter_fatality_engine.controller->phase_time += 0.01f;
                set_snd_vol(
                    g_pz_fighter_fatality_engine.controller->loop_sound,
                    0x1AB7,
                    g_pz_fighter_fatality_engine.controller->phase_time);
            }
        } else if (g_pz_fighter_fatality_engine.controller->phase_time >
                   0.05f) {
            g_pz_fighter_fatality_engine.controller->phase_time -= 0.005f;
            set_snd_vol(
                g_pz_fighter_fatality_engine.controller->loop_sound,
                0x1AB7,
                g_pz_fighter_fatality_engine.controller->phase_time);
        } else if (g_pz_fighter_fatality_engine.controller->loop_sound != 0) {
            snd_stop(g_pz_fighter_fatality_engine.controller->loop_sound);
            g_pz_fighter_fatality_engine.controller->loop_sound = 0;
        }
    }

    if (changed == 1) {
        for (i = 0; i < 2; i++) {
            if (g_pz_fighter_fatality_engine.controller
                        ->grinder_position[i] >
                    g_pz_fighter_fatality_engine.controller
                        ->grinder_target[i] &&
                g_pz_fighter_fatality_engine.controller
                        ->hazard_motion[i][0] > 0.0f) {
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[i][0] *= -1.0f;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if ((g_pz_fighter_fatality_engine.controller
                     ->grinder_position[i] >
                 g_pz_fighter_fatality_engine.controller
                         ->grinder_target[i] +
                     0.001f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[i][0] < 0.0f) ||
            (g_pz_fighter_fatality_engine.controller
                     ->grinder_position[i] <
                 g_pz_fighter_fatality_engine.controller
                         ->grinder_target[i] -
                     0.001f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[i][0] > 0.0f)) {
            g_pz_fighter_fatality_engine.scene_objects[i]->motion_rate +=
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[i][0];
            g_pz_fighter_fatality_engine.controller->grinder_position[i] =
                g_pz_fighter_fatality_engine.scene_objects[i]->motion_rate;
        }
    }
    return 1.0f;
}

float pz_fighter_attempt_push_into_grinder(void) {
    PuzzleAttackParameters attack = {
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3, 0.4f, 0.8f,
        1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    };

    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack, 0x21);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

/*
 * Emission-only near match (95.52%, retail 0x240/current 0x23C). The u16
 * random choice and shared successful return tail match retail's truncation
 * and CFG. Remaining differences are one instruction plus automatic-Vec
 * relocation labels, FPR/GPR allocation, and equivalent object scheduling.
 */
static float p_grinder_meat_throw_controller(void) {
    PuzzleGrinderMeatController* meat =
        (PuzzleGrinderMeatController*)apdata;
    PuzzleFighterRenderObject* object;
    Vec throw_velocity;
    Vec drift_velocity;
    unsigned short choice;

    if (meat->delay != 0) {
        meat->delay--;
    } else if (meat->phase == 0) {
        throw_velocity.x = 2.2f;
        throw_velocity.y = 1.8f;
        throw_velocity.z = 0.0f;
        drift_velocity.x = -0.073f;
        drift_velocity.y = 0.095f;
        drift_velocity.z = 0.0f;
        if (screen_width > 650) {
            drift_velocity.x *= 1.1f;
        }
        if (meat->direction == 1) {
            throw_velocity.x *= -1.0f;
            drift_velocity.x *= -1.0f;
        }

        {
            Vec path_velocity = {0.04f, 0.08f, 0.03f};

            object = g_pz_fighters_engine.grinder_meat_default;
            choice = randu0(100);
            if (choice < 20) {
                object = g_pz_fighters_engine.grinder_meat_alt0;
            } else if (choice < 40) {
                object = g_pz_fighters_engine.grinder_meat_alt1;
            } else if (choice < 60) {
                object = g_pz_fighters_engine.grinder_meat_alt2;
            }

            unhide_obj(object);
            ft_create_flesh_path(
                object, &throw_velocity, 0, 0, &drift_velocity, 0,
                &path_velocity, 0, -0.004f, 0.5f, 0.1f);
            meat->object = object;
            if (meat->object == 0) {
                return -1.0f;
            }
        }
        meat->phase = 1;
    } else {
        object = meat->object;
        if (meat->direction == 0) {
            if (object->x < -1.8f && object->y < 0.8f) {
                snd_req(0x1ADB);
                bgnd_launch_fx_at_position(
                    "chunk_at_left_grinder", object->x, object->y, object->z);
                return -1.0f;
            }
        } else if (object->x > 1.8f && object->y < 0.8f) {
            snd_req(0x1ADB);
            bgnd_launch_fx_at_position(
                "chunk_at_right_grinder", object->x, object->y, object->z);
            return -1.0f;
        }
    }
    return 1.0f;
}

void pz_fighter_get_grinder_post(int player, Vec* post) {
    if (player == 0) {
        post->x = g_pz_fighters_engine.fighter_posts[1].x;
        post->y = g_pz_fighters_engine.fighter_posts[1].y;
        post->z = g_pz_fighters_engine.fighter_posts[1].z;
        return;
    }

    post->x = g_pz_fighters_engine.fighter_posts[0].x;
    post->y = g_pz_fighters_engine.fighter_posts[0].y;
    post->z = g_pz_fighters_engine.fighter_posts[0].z;
}

/* Soft ceiling: pz_fighter_victim_fatality_bouncy ~99.26% - MWCC emit details; stop. */
static float pz_fighter_victim_fatality_bouncy(void) {
    face_opponent_now();
    stop_me();
    init_air_move();
    blend_to_ani(shared_ani.fatality_bounce, 11, 0.2f);
    set_ani_speed(1.75f);
    ani_to_frame_x(5.0f);
    init_ground_move();
    random_hit(5);
    random_hit(12);
    pz_fighter_shake_camera(2, 0.01f);
    face_bleed_me(3);
    ani_to_frame_x(7.0f);
    init_air_move();
    set_my_state(0x605);
    set_ani_speed(0.8f);
    ani_to_frame_x(34.0f);
    set_my_state(0x600);
    random_hit(5);
    random_hit(12);
    face_bleed_me(3);
    set_my_state(0x600);
    ani_to_frame_x(51.0f);
    random_hit(5);
    random_hit(12);
    face_bleed_me(3);
    init_ground_move();
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_walk_forward ~99.52% - process-transfer coloring; stop. */
static float pz_fighter_walk_forward(void) {
    pz_fighter_walk_FB_true(pz_fighter_always_continue, 500, 1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static int pz_fighter_always_continue(void) {
    return 0;
}

/* Soft ceiling: r_pz_fighter_rx_get_to_point ~98.92% - process-transfer coloring; stop. */
float r_pz_fighter_rx_get_to_point(void) {
    face_opponent_now();
    shake_hit_voice(0.02f, 0, 0, 4);
    blend_to_ani(shared_ani.walk_forward, 3, 0.1f);
    set_ani_speed(0.55f);
    pz_fighter_inline_force_away_with_ani(0.063f, 50, 0.5f, 8);
    ani_to_blend_frame(15.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_fatality_victim_to_exact_spot(void) {
    PuzzleAttackParameters attack = {
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 3.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    };

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack, 0x38);
    g_pz_fighters_engine.fatality_ready = 1;
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_fatality_good_solid_kick(void) {
    PuzzleAttackParameters attack = {
        11.0f, 0.1f, 0.9f, 0x00010001, 0x00070008, 3,
        0.2f, 0.75f, 1.35f, 14.0f, 12.0f, 1, 1, 1, 0,
    };

    set_my_state(0x1200);
    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.solid_kick, &attack, 0x18);
    force_forward(0.02f, 9, 0.5f, 3);
    g_pz_fighters_engine.fatality_ready = 1;
    slow_ani_x(0.44f, 19.0f);
    ani_to_frame_x(31.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_fatality_huge_shove(void) {
    PuzzleAttackParameters attack = {
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    };

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack, 0x17);
    g_pz_fighters_engine.fatality_ready = 1;
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_fatality_medium_shove(void) {
    PuzzleAttackParameters attack = {
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    };

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack, 0x16);
    g_pz_fighters_engine.fatality_ready = 1;
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static void ps_face_bleeding(void) {
    plyr_pdata = 0;
    plyr_obj = 0;
}

static void pw_face_bleeding(void) {
    PuzzleFaceBleedPdata* bleed_data = (PuzzleFaceBleedPdata*)apdata;

    plyr_pdata = bleed_data->player_data;
    plyr_obj = bleed_data->object;
}

static float p_face_bleeding(void) {
    PuzzleFaceBleedPdata* bleed_data = (PuzzleFaceBleedPdata*)apdata;
    unsigned int state;

    state = bleed_data->state + 1;
    bleed_data->state = state;
    if (state >= bleed_data->duration) {
        return -1.0f;
    }
    if (bleed_data->state % bleed_data->interval == 0) {
        plyr_bleed_mouth(plyr_pdata);
        face_bleed_me(3);
    }
    return 1.0f;
}

static RpMaterial* material_set_texture(
    RpMaterial* material, RwTexture* texture) {
    RpMaterialSetTexture(material, texture);
    return material;
}

/*
 * Soft ceiling: 97.07042% in the full-TU report. The recovered body has the
 * retail instruction count and access widths. With the authentic TU-level
 * stmw/lmw setting restored, residue is limited to adjacent Vec-component
 * load/store scheduling and local float relocation labels. No force-match
 * workaround is retained.
 */
static PuzzleFleshchunkPdata* ft_create_flesh_path(
    PuzzleFighterRenderObject* object, Vec* position, int active,
    int flags,
    const Vec* initial_velocity, int mode, const Vec* terminal_velocity,
    int (*completion_callback)(void), float gravity, float bounce,
    float scale) {
    PuzzleFaceBleedProcess* process;
    PuzzleFleshchunkPdata* pdata;

    process = _create_mkproc_generic_nostack(
        0xC00F, 0x1F, p_ft_bounce_path, sizeof(PuzzleFleshchunkPdata),
        &pdata);
    if (process == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(PuzzleFleshchunkPdata), pdata);
    pdata->bounce_count = 0;
    pdata->bounce_decay = bounce;
    pdata->floor_height = scale;
    pdata->gravity = gravity;
    pdata->completion_callback = completion_callback;
    pdata->field_30 = 0;
    if (flags != 0) {
        pdata->initial_velocity.x = sfrand(0.03f);
        pdata->initial_velocity.y = sfrand_ab(0.06f, 0.08f);
        pdata->initial_velocity.z = sfrand(0.03f);
    } else {
        pdata->initial_velocity.x = initial_velocity->x;
        pdata->initial_velocity.y = initial_velocity->y;
        pdata->initial_velocity.z = initial_velocity->z;
    }
    pdata->object = object;
    pdata->object_instance = object->instance;
    object->x = position->x;
    object->y = position->y;
    object->z = position->z;
    if (mode != 0) {
        object->angular_velocity_x = sfrand(0.04f);
        object->motion_rate = sfrand(0.02f);
    } else {
        object->angular_velocity_x = terminal_velocity->x;
        object->motion_rate = terminal_velocity->y;
    }
    object->angular_velocity_z = 0.0f;
    object->external_force_x = pdata->initial_velocity.x;
    if (active != 0) {
        object->external_force_y = -pdata->initial_velocity.y;
    } else {
        object->external_force_y = pdata->initial_velocity.y;
    }
    object->external_force_z = pdata->initial_velocity.z;
    object->flags_bits.gravity_enabled = 1;
    object->flags_bits.rotation_enabled = 1;
    process->wait_routine = ft_fleshchunk_prewake;
    process->script_routine = ft_fleshchunk_postsleep;
    return pdata;
}

static PuzzleFighterRenderObject* fleshchunk_obj;
static PuzzleFleshchunkPdata* pdata_fleshchunk;

static float p_ft_bounce_path(void) {
    int complete;

    complete = 0;
    if (fleshchunk_obj->y > pdata_fleshchunk->floor_height ||
        fleshchunk_obj->external_force_y > 0.0f) {
        fleshchunk_obj->external_force_y += pdata_fleshchunk->gravity;
    } else {
        pdata_fleshchunk->bounce_count++;
        fleshchunk_obj->external_force_x =
            pdata_fleshchunk->initial_velocity.x *
            (float)pow(
                (double)pdata_fleshchunk->bounce_decay,
                (double)pdata_fleshchunk->bounce_count);
        fleshchunk_obj->external_force_y =
            pdata_fleshchunk->initial_velocity.y *
            (float)pow(
                (double)pdata_fleshchunk->bounce_decay,
                (double)pdata_fleshchunk->bounce_count);
        fleshchunk_obj->external_force_z =
            pdata_fleshchunk->initial_velocity.z *
            (float)pow(
                (double)pdata_fleshchunk->bounce_decay,
                (double)pdata_fleshchunk->bounce_count);
        if (pdata_fleshchunk->completion_callback != 0) {
            complete = pdata_fleshchunk->completion_callback();
        }
        if (complete != 0 || fleshchunk_obj->external_force_y < 0.01f) {
            fleshchunk_obj->flags_bits.gravity_enabled = 0;
            fleshchunk_obj->flags_bits.rotation_enabled = 0;
            fleshchunk_obj->external_force_y = 0.01f;
            fleshchunk_obj->secondary_flags_bits.stopped = 1;
            fleshchunk_obj->y = pdata_fleshchunk->floor_height;
            fleshchunk_obj->flags_bits.bit7 = 1;
            return -1.0f;
        }
        fleshchunk_obj->flags_bits.gravity_enabled = 1;
        fleshchunk_obj->flags_bits.rotation_enabled = 1;
    }
    return 1.0f;
}

static void ft_fleshchunk_postsleep(void) {
    pdata_fleshchunk = 0;
    fleshchunk_obj = 0;
}

static inline PuzzleFighterRenderObject* fleshchunk_live_object(
    PuzzleFleshchunkPdata* owner) {
    PuzzleFighterRenderObject* object = owner->object;
    if (object != 0) {
        if (object->instance == owner->object_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

static void ft_fleshchunk_prewake(void) {
    PuzzleFighterRenderObject* object;

    pdata_fleshchunk = (PuzzleFleshchunkPdata*)apdata;
    if (pdata_fleshchunk == 0) {
        mkproc_die();
    }

    object = fleshchunk_live_object(pdata_fleshchunk);
    fleshchunk_obj = object;
    if (object == 0) {
        mkproc_die();
    }
}

void cleanup_pz_fatality_stuff(void) {
    memset(&g_pz_fighter_fatality_engine, 0,
           sizeof(g_pz_fighter_fatality_engine));
    pdata_fleshchunk = 0;
    fleshchunk_obj = 0;
}






































































































































































































































































































































































































#undef SETUP_SNAKE_EFFECT
