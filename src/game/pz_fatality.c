/* BUILD: -inline noauto,deferred requires reverse retail function order;
 * -O4,s, -use_lmw_stmw on and pooled readonly strings retain the retail codegen. */
#include "fdlibm.h"
#include "math/gxVect.h"
#include "math/mk_math.h"
#include "game/pz_fatality.h"
#include "game/game_info.h"
#include "runtime/cam_api.h"
#include "platform/display_metrics.h"
#include "runtime/asset.h"
#include "runtime/image.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "runtime/mk_struct.h"
#include "runtime/cstring.h"
#include "rw/rwframe.h"
#include "rw/rpworld_types.h"

typedef float (*PuzzleFatalityPreroundFn)(void);
typedef float (*PuzzleFatalityActiveFn)(int active);
typedef void (*PuzzleFatalityStartFn)(int attacker, int victim);

typedef struct AniScript AniScript;
typedef struct MkFlippedBoneMap MkFlippedBoneMap;
struct PuzzleObjectFlags {
    unsigned char bit7 : 1;
    unsigned char airborne : 1;
    unsigned char gravity_enabled : 1;
    unsigned char transform_dirty : 1;
    unsigned char angular_velocity_enabled : 1;
    unsigned char rotation_enabled : 1;
    unsigned char scale_active : 1;
    unsigned char moving : 1;
};

struct PuzzleObjectSecondaryFlags {
    unsigned char stopped : 1;
    unsigned char pad_mid : 5;
    unsigned char field_bit1 : 1;
    unsigned char pad_bit0 : 1;
};

struct PuzzleSobjMaterialData {
    char pad00[0x18];
    RpGeometry* geometry;
};

struct PuzzleProcessVtable {
    char pad00[0x18];
    void (*sleep)(struct PuzzleProcessVtable* vtable);
    char pad1C[8];
    MkProcJumpFn transfer;
};

struct PuzzleProcess {
    struct PuzzleProcessVtable* vtbl;
};

struct PuzzleSharedAnimations {
    char pad00[0x1C];
    AniScript* solid_kick;
    char pad20[0x30];
    AniScript* push_into_grinder;
    char pad54[0x3C];
    AniScript* snake_eaten_start;
    char pad94[0x0C];
    AniScript* snake_idle;
    AniScript* snake_lunge;
    AniScript* snake_bite;
    AniScript* snake_attacker;
    AniScript* snake_victim;
    AniScript* burn_hurt;
    AniScript* burn_loop;
    AniScript* burn_recover;
    AniScript* burn_summon;
    AniScript* burn_idle;
    char padC8[0x40];
    AniScript* snake_eaten_end;
    char pad10C[0x60];
    AniScript* head_poked;
    char pad170[0x28];
    AniScript* burn_attacker_start;
    AniScript* burn_attacker_end;
    char pad1A0[0x7C];
    AniScript* objects_falling_crushed;
};

struct PuzzleBaseAnimations {
    char pad00[4];
    AniScript* walk_forward;
    char pad08[0x1BC];
    AniScript* fatality_bounce;
    char pad1C8[0x15C];
    AniScript* lightning_electrocution;
};

struct PuzzleAnimPdata {
    char pad00[0x64];
    float animation_step;
};

struct PuzzleFightersEngine {
    float balance;
    char pad004[0x3C];
    Vec fighter_posts[2];
    char pad058[0x0C];
    PuzzleFatalityRandomEvent random_event;
    char pad088[0x94];
    int fatality_abort;
    int fatality_active;
    int fatality_ready;
    int fatality_victim;
    int fatality_attacker;
    struct PuzzleFighterRenderObject* grinder_meat_alt2;
    struct PuzzleFighterRenderObject* grinder_meat_final;
    struct PuzzleFighterRenderObject* fatality_piece;
    struct PuzzleFighterRenderObject* grinder_meat_alt0;
    struct PuzzleFighterRenderObject* grinder_meat_default;
    struct PuzzleFighterRenderObject* grinder_meat_alt1;
    struct PuzzleFighterRenderObject* left_eye;
    struct PuzzleFighterRenderObject* right_eye;
    char pad150[0x0C];
    int fatality_index;
    unsigned int random_event_cooldown;
    int fighter_reaction_cooldown;
    char pad168[0x10];
    unsigned int fatality_timer;
    float fatality_motion;
    char pad180[0x64];
    int snake_active;
};

struct PuzzleObjectVtable {
    char pad00[0x10];
    void (*destroy)(
        struct PuzzleFighterRenderObject* object, struct PuzzleObjectVtable* vtable);
};

struct PuzzleFighterRenderObject {
    struct PuzzleObjectVtable* vtbl;
    unsigned int instance;
    struct PuzzleObjectFlags flags_bits;
    struct PuzzleObjectSecondaryFlags secondary_flags_bits;
    char pad0A[0x16];
    void* frame;
    char pad24[8];
    int model_flags;
    float hazard_x;
    float hazard_y;
    float hazard_z;
    char pad3C[0x64];
    Vec position;
    char padAC[4];
    Vec external_force;
    char padBC[0x18];
    float facing_angle;
    char padD8[8];
    float angular_velocity_x;
    float motion_rate;
    float angular_velocity_z;
    char padEC[4];
    Vec scale;
};

struct PuzzleFighterMove {
    char pad00[0x20];
    int active;
};

struct PuzzleFighterPhysics {
    char pad00[0x0A];
    unsigned char flags_0A;
    char pad0B[0xC9];
    float facing_angle;
};

struct PuzzleBoneData {
    char pad00[0x40];
    MKMATRIX* matrix;
};

struct PuzzleBoneObjectView {
    char pad00[0x48];
    struct PuzzleBoneData* bone_data;
};

struct PuzzleFatalityHazardObject {
    char pad00[8];
    struct PuzzleObjectFlags flags_bits;
    char pad09[0x0B];
    struct PuzzleSobjMaterialData* material_data;
    char pad18[0x10];
    float field_28;
    int model_flags;
    float x;
    float y;
    float z;
    char pad3C[8];
    float motion;
    char pad48[0x28];
    Vec scale;
};

struct PuzzleFatalityHazardGroup {
    struct PuzzleFatalityHazardObject* objects[4];
    char pad10[8];
};

struct PuzzleFatalityEngine {
    struct PuzzleFighterRenderObject* scene_objects[2];
    struct PuzzleFatalityHazardGroup hazard_groups[2];
    struct PuzzleFatalityController* controller;
    int active_effect;
    RwTexture* grinder_texture;
    char pad44[0x14];
    int effect_timer;
};

struct PuzzleFatalityController {
    MkHdr hdr;
    int unload_requested;
    int state;
    int substate;
    int active;
    int phase;
    int preround_active;
    int controller_step;
    int attacker_player;
    int victim_player;
    unsigned int preround_timer;
    unsigned int loop_sound;
    float phase_time;
    unsigned int preround_sound_started;
    float grinder_position[2];
    float grinder_target[2];
    float chomper_position[2][2];
    float hazard_motion[2][2];
    unsigned int hazard_initialized[3];
    struct PuzzleAnimPdata* fighter_pdata[2];
};

struct PuzzleGrinderMeatController {
    MkHdr hdr;
    unsigned int direction;
    unsigned int delay;
    int phase;
    struct PuzzleFighterRenderObject* object;
};

struct PuzzleGrinderNoisePdata {
    MkHdr hdr;
    unsigned int duration;
    unsigned int timer;
};

struct PuzzleFaceBleedPdata {
    MkHdr hdr;
    unsigned int duration;
    unsigned int interval;
    unsigned int state;
    struct PuzzleFighterRenderObject* object;
    PlyrPdata* player_data;
};

struct PuzzleFleshchunkPdata {
    MkHdr hdr;
    struct PuzzleFighterRenderObject* object;
    unsigned int object_instance;
    int bounce_count;
    Vec initial_velocity;
    float gravity;
    int (*completion_callback)(void);
    char pad28[8];
    int field_30;
    float bounce_decay;
    float floor_height;
};

struct PuzzleFaceBleedProcess {
    char pad00[0xB0];
    void (*wait_routine)(void);
    void (*script_routine)(void);
};

struct PuzzleEffectBankContext {
    int art_handle;
    struct PuzzleFighterRenderObject* owner;
    void* context;
};

struct PuzzleParticleEmitter {
    char pad00[0x1C];
    unsigned char hidden : 1;
    unsigned char flags_low : 7;
};

struct PuzzleParticleEffect {
    char pad00[0x40];
    unsigned char emitters[1];
};

struct PuzzleFatalityDefinition {
    float (*load_and_place)(void);
    int threshold;
    unsigned int field_0x08;
};

struct PuzzleFatalityDefinitions {
    unsigned int field_0x00;
    struct PuzzleFatalityDefinition entries[15];
};

struct PuzzleAmbientLightDefinition {
    int type;
    float (*proc)(void);
    int flags;
    float color[4];
};

struct PuzzleDirectLightDefinition {
    int type;
    float (*proc)(void);
    int flags;
    float color[4];
    float angle_y;
    float angle_x;
    float angle_z;
};

extern struct PuzzleFightersEngine g_pz_fighters_engine;
extern struct PuzzleSharedAnimations pz_shared_ani;
extern struct PuzzleBaseAnimations shared_ani;
extern struct PuzzleProcess* aproc;
extern struct PuzzleProcess* plyr_anim_proc;
extern struct PuzzleAnimPdata* plyr_anim_pdata;
extern float _mkproc_sleep_ticks;
extern MkFileInfo sec_pz_danger_lightning;
extern MkFileInfo sec_pz_danger_1ton;
extern MkFileInfo sec_pz_danger_grinder;
extern MkFileInfo sec_pz_danger_burn;
extern MkFileInfo sec_pz_danger_crusher;
extern MkFileInfo sec_pz_danger_chomper;
extern MkFileInfo sec_pz_danger_snake;
extern void* apdata;

struct PuzzleFatalityEngine g_pz_fighter_fatality_engine;
extern PlyrPdata* his_pdata;
extern struct PuzzleFighterRenderObject* plyr_obj;

unsigned int randu0(unsigned int max);
int pz_fighter_close_enough_to_super_move(unsigned int player);
float pz_fighter_attempt_push_into_grinder(void);
void pz_fighter_attack(AniScript* animation, PuzzleAttackParameters* attack,
                       int move);
void ani_to_end(void);
float pz_fighter_long_exit(void);
void unload_pz_fighter_fatality_banks(void);
void snd_req_delay(int sound, int delay);
void snd_stop(int handle);
void* fx_by_owner(const char* name, int owner);
void fx_pause_emit(void* effect);
struct PuzzleFighterRenderObject* pz_fighter_get_player_obj(int player);
struct PuzzleProcess* pz_fighter_get_player_proc(int player);
struct PuzzleFighterMove* pz_get_fighter_move(void);
PlyrPdata* pz_get_pdata_by_id(int player);
void xfer_proc(struct PuzzleProcess* process, PuzzleFatalityProcessFn entry);
void minigame_event(int* event);
float xz_distance_between_players(void);
int pan_snd_req(int sound, float pan);
int pan_vol_snd_req(int sound, float pan, float volume);
int plyr_snd_req(int sound);
void bgnd_launch_fx_at_position(const char* name, float x, float y, float z);
void bgnd_set_fx_ang_y(float angle);
void snd_major_hit_voice(void);
void snd_death_voice(void);
void random_hit(int group);
void plyr_bleed_mouth(PlyrPdata* player_data);
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
void shake_hit_voice(
    int shake_ticks, float rumble_scale, int hit_voice, int fighter_voice);
float pz_fighter_inline_force_away_with_ani(
    float velocity, unsigned int coast_ticks, float damping,
    unsigned int damping_ticks);
void head_tracking_off(void);
void force_forward(float force, int duration, float damping, int animation);
void slow_ani_x(float speed, float frame);
void obj_set_bone_collapse_flag(
    struct PuzzleFighterRenderObject* object, int bone);
void calc_bone_world_mat(struct PuzzleFighterRenderObject* object, int bone);
float pz_fighter_clear_out_external_forces(void);
void random_voice(int group);
void bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect, struct PuzzleFighterRenderObject* object, int bone);
void resume_effect_at_obj_bid(
    struct PuzzleFighterRenderObject* object, int bone, void* effect, int active,
    int flags);
void pz_fighter_walk_FB_true(
    int (*continue_test)(void), unsigned int duration, int forward);
void unhide_obj(struct PuzzleFighterRenderObject* object);
void hide_sobj(struct PuzzleFatalityHazardObject* object);
RpMaterial* sobj_find_material_with_texture(
    struct PuzzleFatalityHazardObject* object, const char* texture);

float sfrand(float range);
float frand(float range);
void spawn_bld_fall(
    const char* effect, int flags, float* position, Vec* velocity,
    PlyrPdata* player_data);
static struct PuzzleFleshchunkPdata* ft_create_flesh_path(
    struct PuzzleFighterRenderObject* object, Vec* position, int active,
    int flags,
    Vec* initial_velocity, int mode, Vec* terminal_velocity,
    int (*completion_callback)(void), float gravity, float bounce,
    float scale);
struct PuzzleFaceBleedProcess* _create_mkproc_generic_nostack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    struct PuzzleFleshchunkPdata** process_data);
void zero_pdata_payload(int size, void* process_data);
float sfrand_ab(float minimum, float maximum);
struct PuzzleFaceBleedProcess* _create_mkproc_generic_bigstack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    struct PuzzleFaceBleedPdata** process_data);
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
void unhide_sobj(struct PuzzleFatalityHazardObject* object);
int transition_to_anim_script_frame(
    float transition_frames, float frame, struct PuzzleAnimPdata* pdata,
    AniScript* animation, unsigned int flags);
void set_pdata_anim_step(struct PuzzleAnimPdata* pdata, float step);
void pz_fighter_clear_out_all_external_forces(void* fighter);
static float p_grinder_meat_throw_controller(void);
static float p_grinder_noise(void);
void* _create_mkproc_generic_tinystack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    void* pdata);
void load_effect_bank_with_context(
    const char* bank, struct PuzzleEffectBankContext* context);
AniTextureControl* replace_sobj_texture_with_named_wiff(
    struct PuzzleFatalityHazardObject* object, int handle, const char* texture,
    const char* wiff);
void fx_set_param_v3(
    void* effect, int parameter, float x, float y, float z);
void fx_set_render_priority(void* effect, int priority);
MKMATRIX* force_calc_bone_world_mat(
    struct PuzzleFighterRenderObject* object, int bone);
static RpMaterial* material_set_texture(
    RpMaterial* material, void* texture);
static int pz_fighter_always_continue(void);
static float p_ft_bounce_path(void);
static void ft_fleshchunk_postsleep(void);
static void ft_fleshchunk_prewake(void);
void freeze_player(void);
void unfreeze_player(void);
void got_hit_fx(int strength, int bone, int blood, int unused1, int unused2, float scale, int active);
void hide_obj(struct PuzzleFighterRenderObject* object);
void insert_fgnd_mkobj(struct PuzzleFighterRenderObject* object);
void obj_set_pos(struct PuzzleFighterRenderObject* object, Vec* position);
void update_mkobj(struct PuzzleFighterRenderObject* object);
static void pz_fighter_set_objects_falling_obj(
    struct PuzzleFighterRenderObject* object1,
    struct PuzzleFighterRenderObject* object2);
void load_pz_fighter_fatality_bank(int bank);
static float p_lightning_controller(void);
static float p_objects_falling_controller2(void);
static float p_grinder_controller(void);
static float p_burn_controller(void);
static float p_chomper2_controller(void);
static float p_chomper_controller(void);
static float p_snake_controller(void);
void obj_create_sobjs(struct PuzzleFighterRenderObject* object);
struct PuzzleFatalityHazardObject* obj_find_sobj_by_id(
    struct PuzzleFighterRenderObject* object, int id);
void obj_change_to_skinned_obj_light_list(
    struct PuzzleFighterRenderObject* object, void* light_definition);
void obj_add_to_skinned_obj_light_list_with_ambient(
    struct PuzzleFighterRenderObject* object, void* ambient_definition);
struct PuzzleAnimPdata* animate_obj(
    struct PuzzleFighterRenderObject* object, AniScript* animation,
    const int* bone_tags, MkFlippedBoneMap* flipped_bones,
    void* ground_collisions, float playback_rate, int active);
struct PuzzleFatalityHazardObject* obj_first_sobj(
    struct PuzzleFighterRenderObject* object);
void sobj_set_priority(struct PuzzleFatalityHazardObject* object, int priority);
void fx_reset(void* effect);
void fx_resume_emit(void* effect);
struct PuzzleParticleEffect* find_pfx_by_name(const char* name);
void restart_effect_ppfx(struct PuzzleParticleEffect* effect);
void pfx_bind_emitter_to_obj_bone(
    struct PuzzleParticleEffect* effect, struct PuzzleFighterRenderObject* object, int bone);
struct PuzzleParticleEmitter* pfx_get_emitter(void* emitters, int index);

static void pz_fighter_grinder_entering_fatality(
    int attacker, int victim);
static void pz_fighter_chomper_entering_fatality(
    int attacker, int victim);
static void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim);
static void pz_fighter_objects_falling_entering_fatality(
    int attacker, int victim);

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

struct PuzzleDirectLightDefinition skinned_obj_light_def = {
    3, p_track_cam_ang_y_light, 1,
    {1.0f, 1.0f, 1.0f, 1.0f},
    6.010f, 3.190f, 0.0f,
};

struct PuzzleAmbientLightDefinition skinned_obj_ambient_light_def = {
    1, 0, 3, {0.2f, 0.2f, 0.2f, 1.0f},
};

int pz_snake_bones[9] = {1, 2, 3, 4, 5, 6, 7, 8, 0};

struct PuzzleFatalityDefinitions g_fatalityTable = {
    3, {
        {pz_fighter_load_and_place_initial_grinders, 20, 0},
        {pz_fighter_load_and_place_initial_chompers, 40, 0},
        {pz_fighter_load_and_place_initial_chompers2, 60, 0},
        {pz_fighter_load_and_place_initial_objects_falling, 80, 0},
        {pz_fighter_load_and_place_initial_lightning, 90, 0},
        {pz_fighter_load_and_place_initial_snake, 95, 0},
        {pz_fighter_load_and_place_initial_burn, 100, 0},
    }
};
int pz_fighter_check_fatality_random_event(void);
int pz_fighter_fatality_during_round_stuff_over(void);
void pz_fighters_fatality_preround_event(void);
void pz_fighters_fatality_prep_chores(void);
void pz_fighters_fatality_in_progress(void);
void pz_fighter_load_place_fatality_elements(int fatality);
void pz_fighters_fatality_round_over(void);
void pz_fighters_fatality_normal_fighting(int enabled);
void pz_fighters_fatality_unload(void);
void pz_fighters_fatality_start(int attacker, int victim);

#define SETUP_SNAKE_EFFECT(name, object, pause_after_setup)                  \
    do {                                                                    \
        effect = fx_by_owner(name, 4);                                      \
        fx_reset(effect);                                                   \
        fx_resume_emit(effect);                                             \
        particle_effect = find_pfx_by_name(name);                           \
        restart_effect_ppfx(particle_effect);                               \
        pfx_bind_emitter_to_obj_bone(particle_effect, object, 5);           \
        pfx_get_emitter(particle_effect->emitters, 0)->hidden = 0; \
        if (pause_after_setup) {                                            \
            fx_pause_emit(effect);                                          \
        }                                                                   \
    } while (0)

static inline void pz_snake_bite(unsigned int snake) {
    struct PuzzleParticleEffect* burst;
    struct PuzzleParticleEffect* saliva;

    pan_snd_req(0x1AC2, snake == 0 ? -0.7f : 0.7f);
    if (snake == 0) {
        saliva = fx_by_owner("saliva1", 4);
        burst = fx_by_owner("saliva_burst1", 4);
    } else {
        burst = fx_by_owner("saliva_burst2", 4);
        saliva = fx_by_owner("saliva2", 4);
    }
    fx_pause_emit(saliva);
    transition_to_anim_script_frame(
        0.1f, 0.0f,
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake],
        pz_shared_ani.snake_bite, 0x23);
    set_pdata_anim_step(
        g_pz_fighter_fatality_engine.controller->fighter_pdata[snake], 1.0f);
    _mkproc_sleep_ticks = 3.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    if (g_pz_fighters_engine.fatality_abort != 1) {
        fx_reset(burst);
        fx_resume_emit(burst);
        _mkproc_sleep_ticks = 45.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        if (g_pz_fighters_engine.fatality_abort != 1) {
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
    struct PuzzleParticleEffect* saliva;

    saliva = fx_by_owner(
        snake == 0 ? "saliva1" : "saliva2", 4);
    pan_snd_req(0x1AC3, snake == 0 ? -0.7f : 0.7f);
    fx_pause_emit(saliva);
    transition_to_anim_script_frame(
        0.05f, 0.0f,
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
        if (g_pz_fighters_engine.fatality_abort != 1) {
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

static inline void pz_lightning_bolt(Vec* position, int pan_side) {
    struct PuzzleFighterRenderObject* bolt;
    struct PuzzleParticleEffect* lightning_smoke;
    struct PuzzleParticleEffect* spark;
    struct PuzzleParticleEffect* burning_smoke;
    AniTextureControl* texture;
    unsigned int bolt_instance;

    bolt = (struct PuzzleFighterRenderObject*)load_named_model_from_slot(
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

    bolt = MK_LIVE(bolt, bolt_instance);
    if (bolt != 0 && bolt->instance != 0) {
        bolt->vtbl->destroy(bolt, bolt->vtbl);
    }
}

static inline void pz_chomper_apply_motion(
    int side, int object_count, int fighting) {
    int object_index;

    for (object_index = 0; object_index < object_count; object_index++) {
        float target = g_pz_fighter_fatality_engine.controller
                           ->chomper_position[side][object_index];
        if ((g_pz_fighter_fatality_engine.hazard_groups[side].objects[object_index]->y >
                 target + 0.13f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[side][object_index] < 0.0f) ||
            (g_pz_fighter_fatality_engine.hazard_groups[side].objects[object_index]->y <
                 target - 0.13f &&
             g_pz_fighter_fatality_engine.controller
                     ->hazard_motion[side][object_index] > 0.0f)) {
            g_pz_fighter_fatality_engine.hazard_groups[side].objects[object_index]->motion =
                g_pz_fighter_fatality_engine.controller
                    ->hazard_motion[side][object_index];
        } else if (fighting == 0) {
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][object_index] = 0.0f;
            g_pz_fighter_fatality_engine.hazard_groups[side]
                .objects[object_index]->motion = 0.0f;
        } else if (g_pz_fighter_fatality_engine.controller->phase == 1) {
            g_pz_fighter_fatality_engine.hazard_groups[side]
                .objects[object_index]->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[side][object_index] = 0.0f;
        }
    }
}

static inline void pz_chomper_reverse_motion(
    int side, int object_count) {
    struct PuzzleFatalityHazardObject* object;
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

static inline void pz_grinder_scale_launch_vector(
    Vec* output, const RwV3d* input, float scale) {
    output->x = scale * input->x;
    output->y = scale * input->y;
    output->z = scale * input->z;
}

void pz_fighter_get_grinder_post(int player, Vec* post);
float r_pz_fighter_rx_get_to_point(void);

static struct PuzzleFighterRenderObject* fleshchunk_obj;
static struct PuzzleFleshchunkPdata* pdata_fleshchunk;

void cleanup_pz_fatality_stuff(void) {
    memset(&g_pz_fighter_fatality_engine, 0,
           sizeof(g_pz_fighter_fatality_engine));
    pdata_fleshchunk = 0;
    fleshchunk_obj = 0;
}

static void ft_fleshchunk_prewake(void) {
    struct PuzzleFighterRenderObject* object;

    pdata_fleshchunk = apdata;
    if (pdata_fleshchunk == 0) {
        mkproc_die();
    }

    object = MK_LIVE(pdata_fleshchunk->object, pdata_fleshchunk->object_instance);
    fleshchunk_obj = object;
    if (object == 0) {
        mkproc_die();
    }
}

static void ft_fleshchunk_postsleep(void) {
    pdata_fleshchunk = 0;
    fleshchunk_obj = 0;
}

static float p_ft_bounce_path(void) {
    int complete;

    complete = 0;
    if (fleshchunk_obj->position.y > pdata_fleshchunk->floor_height ||
        fleshchunk_obj->external_force.y > 0.0f) {
        fleshchunk_obj->external_force.y += pdata_fleshchunk->gravity;
    } else {
        pdata_fleshchunk->bounce_count++;
        fleshchunk_obj->external_force.x =
            pdata_fleshchunk->initial_velocity.x *
            (float)pow(
                pdata_fleshchunk->bounce_decay,
                pdata_fleshchunk->bounce_count);
        fleshchunk_obj->external_force.y =
            pdata_fleshchunk->initial_velocity.y *
            (float)pow(
                pdata_fleshchunk->bounce_decay,
                pdata_fleshchunk->bounce_count);
        fleshchunk_obj->external_force.z =
            pdata_fleshchunk->initial_velocity.z *
            (float)pow(
                pdata_fleshchunk->bounce_decay,
                pdata_fleshchunk->bounce_count);
        if (pdata_fleshchunk->completion_callback != 0) {
            complete = pdata_fleshchunk->completion_callback();
        }
        if (complete != 0 || fleshchunk_obj->external_force.y < 0.01f) {
            fleshchunk_obj->flags_bits.gravity_enabled = 0;
            fleshchunk_obj->flags_bits.rotation_enabled = 0;
            fleshchunk_obj->external_force.y = 0.01f;
            fleshchunk_obj->secondary_flags_bits.stopped = 1;
            fleshchunk_obj->position.y = pdata_fleshchunk->floor_height;
            fleshchunk_obj->flags_bits.bit7 = 1;
            return -1.0f;
        }
        fleshchunk_obj->flags_bits.gravity_enabled = 1;
        fleshchunk_obj->flags_bits.rotation_enabled = 1;
    }
    return 1.0f;
}

static struct PuzzleFleshchunkPdata* ft_create_flesh_path(
    struct PuzzleFighterRenderObject* object, Vec* position, int active,
    int flags,
    Vec* initial_velocity, int mode, Vec* terminal_velocity,
    int (*completion_callback)(void), float gravity, float bounce,
    float scale) {
    struct PuzzleFaceBleedProcess* process;
    struct PuzzleFleshchunkPdata* pdata;

    process = _create_mkproc_generic_nostack(
        0xC00F, 0x1F, p_ft_bounce_path, sizeof(struct PuzzleFleshchunkPdata),
        &pdata);
    if (process == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(struct PuzzleFleshchunkPdata), pdata);
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
    object->position.x = position->x;
    object->position.y = position->y;
    object->position.z = position->z;
    if (mode != 0) {
        object->angular_velocity_x = sfrand(0.04f);
        object->motion_rate = sfrand(0.02f);
    } else {
        object->angular_velocity_x = terminal_velocity->x;
        object->motion_rate = terminal_velocity->y;
    }
    object->angular_velocity_z = 0.0f;
    object->external_force.x = pdata->initial_velocity.x;
    if (active != 0) {
        object->external_force.y = -pdata->initial_velocity.y;
    } else {
        object->external_force.y = pdata->initial_velocity.y;
    }
    object->external_force.z = pdata->initial_velocity.z;
    object->flags_bits.gravity_enabled = 1;
    object->flags_bits.rotation_enabled = 1;
    process->wait_routine = ft_fleshchunk_prewake;
    process->script_routine = ft_fleshchunk_postsleep;
    return pdata;
}

static RpMaterial* material_set_texture(
    RpMaterial* material, void* texture) {
    RpMaterialSetTexture(material, texture);
    return material;
}

static float p_face_bleeding(void) {
    struct PuzzleFaceBleedPdata* bleed_data = apdata;
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

static void pw_face_bleeding(void) {
    struct PuzzleFaceBleedPdata* bleed_data = apdata;

    plyr_pdata = bleed_data->player_data;
    plyr_obj = bleed_data->object;
}

static void ps_face_bleeding(void) {
    plyr_pdata = 0;
    plyr_obj = 0;
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

float r_pz_fighter_rx_get_to_point(void) {
    face_opponent_now();
    shake_hit_voice(0, 0.02f, 0, 4);
    blend_to_ani(shared_ani.walk_forward, 3, 0.1f);
    set_ani_speed(0.55f);
    pz_fighter_inline_force_away_with_ani(0.063f, 50, 0.5f, 8);
    ani_to_blend_frame(15.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static int pz_fighter_always_continue(void) {
    return 0;
}

static float pz_fighter_walk_forward(void) {
    pz_fighter_walk_FB_true(pz_fighter_always_continue, 500, 1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

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

/* TODO: [Scope warn] path_velocity block: branch-entry initializer drops 100% to 86.03%; late component stores to 93.08%. */
static float p_grinder_meat_throw_controller(void) {
    struct PuzzleGrinderMeatController* meat =
        apdata;
    struct PuzzleFighterRenderObject* object;
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
        if (meat->direction == 0) {
            struct PuzzleFighterRenderObject* left_object;

            left_object = meat->object;
            if (left_object->position.x < -1.8f && left_object->position.y < 0.8f) {
                snd_req(0x1ADB);
                bgnd_launch_fx_at_position(
                    "chunk_at_left_grinder", left_object->position.x, left_object->position.y, left_object->position.z);
                return -1.0f;
            }
        } else {
            struct PuzzleFighterRenderObject* right_object;

            right_object = meat->object;
            if (right_object->position.x > 1.8f && right_object->position.y < 0.8f) {
                snd_req(0x1ADB);
                bgnd_launch_fx_at_position(
                    "chunk_at_right_grinder", right_object->position.x, right_object->position.y, right_object->position.z);
                return -1.0f;
            }
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

static float p_grinder_noise(void) {
    struct PuzzleGrinderNoisePdata* noise = apdata;
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

static inline void pz_launch_grinder_meat_alt0(void) {
    MKMATRIX* bone_matrix;
    Vec position;
    Vec velocity;
    Vec terminal = {0.04f, 0.08f, 0.03f};
    float direction = 1.0f;

    if (plyr_pdata->plyr_num == 1) {
        direction = -1.0f;
    }

    bone_matrix = force_calc_bone_world_mat(plyr_obj, 9);
    RwFrameTransform(g_pz_fighters_engine.grinder_meat_alt0->frame, bone_matrix, 0);
    get_bone_world_pos(plyr_obj, 9, &position);
    position.y = 0.8f;
    unhide_obj(g_pz_fighters_engine.grinder_meat_alt0);
    pz_grinder_scale_launch_vector(&velocity, &bone_matrix->at, 0.1f);
    velocity.y = 0.05f;
    velocity.x = 0.02f * direction;
    velocity.z = 0.003f * direction;
    ft_create_flesh_path(
        g_pz_fighters_engine.grinder_meat_alt0, &position, 0, 0, &velocity, 0, &terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

static inline void pz_launch_grinder_meat_alt2(void) {
    MKMATRIX* bone_matrix;
    Vec position;
    Vec velocity;
    Vec terminal = {0.02f, 0.02f, 0.07f};
    float direction = 1.0f;

    if (plyr_pdata->plyr_num == 1) {
        direction = -1.0f;
    }

    bone_matrix = force_calc_bone_world_mat(plyr_obj, 20);
    RwFrameTransform(g_pz_fighters_engine.grinder_meat_alt2->frame, bone_matrix, 0);
    get_bone_world_pos(plyr_obj, 20, &position);
    position.y = 0.8f;
    unhide_obj(g_pz_fighters_engine.grinder_meat_alt2);
    pz_grinder_scale_launch_vector(&velocity, &bone_matrix->at, 0.1f);
    velocity.y = 0.03f;
    velocity.x = 0.02f * direction;
    velocity.z = 0.003f * direction;
    ft_create_flesh_path(
        g_pz_fighters_engine.grinder_meat_alt2, &position, 0, 0, &velocity, 0, &terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

static inline void pz_launch_grinder_meat_alt1(void) {
    MKMATRIX* bone_matrix;
    Vec position;
    Vec velocity;
    Vec terminal = {0.04f, 0.08f, 0.03f};
    float direction = 1.0f;

    if (plyr_pdata->plyr_num == 1) {
        direction = -1.0f;
    }

    bone_matrix = force_calc_bone_world_mat(plyr_obj, 9);
    RwFrameTransform(g_pz_fighters_engine.grinder_meat_alt1->frame, bone_matrix, 0);
    get_bone_world_pos(plyr_obj, 9, &position);
    position.y = 0.45f;
    unhide_obj(g_pz_fighters_engine.grinder_meat_alt1);
    pz_grinder_scale_launch_vector(&velocity, &bone_matrix->at, 0.1f);
    velocity.y = 0.04f;
    velocity.x = 0.04f * direction;
    velocity.z = 0.003f * direction;
    ft_create_flesh_path(
        g_pz_fighters_engine.grinder_meat_alt1, &position, 0, 0, &velocity, 0, &terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

static inline void pz_launch_grinder_meat_default(void) {
    MKMATRIX* bone_matrix;
    Vec position;
    Vec velocity;
    Vec terminal = {0.04f, 0.08f, 0.03f};
    float direction = 1.0f;

    if (plyr_pdata->plyr_num == 1) {
        direction = -1.0f;
    }

    bone_matrix = force_calc_bone_world_mat(plyr_obj, 9);
    RwFrameTransform(g_pz_fighters_engine.grinder_meat_default->frame, bone_matrix, 0);
    get_bone_world_pos(plyr_obj, 9, &position);
    position.y = 0.6f;
    unhide_obj(g_pz_fighters_engine.grinder_meat_default);
    pz_grinder_scale_launch_vector(&velocity, &bone_matrix->at, 0.1f);
    velocity.y = 0.08f;
    velocity.x = 0.04f * direction;
    velocity.z = -0.005f * direction;
    ft_create_flesh_path(
        g_pz_fighters_engine.grinder_meat_default, &position, 0, 0, &velocity, 0, &terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

static inline void pz_grinder_create_final_path(Vec* position, Vec* velocity) {
    struct PuzzleFighterRenderObject* object = g_pz_fighters_engine.grinder_meat_final;
    Vec terminal = {0.04f, 0.08f, 0.03f};

    unhide_obj(object);
    ft_create_flesh_path(
        object, position, 0, 0, velocity, 0, &terminal, 0,
        -0.004f, 0.5f, 0.1f);
}

static inline void pz_launch_grinder_meat_final(void) {
    Vec final_position;
    Vec final_velocity;

    final_position.x = 2.05f;
    final_position.y = 1.8f;
    final_position.z = 0.0f;
    final_velocity.x = -0.0625f;
    final_velocity.y = 0.105f;
    final_velocity.z = 0.0f;
    if (screen_width > 650) {
        final_velocity.x *= 1.1f;
    }
    if (plyr_pdata->plyr_num == 0) {
        final_position.x *= -1.0f;
        final_velocity.x *= -1.0f;
    }
    pz_grinder_create_final_path(&final_position, &final_velocity);
}

/* TODO: [near miss] 99.97%; verified equal vector constants remain at different TU pool offsets. */
float r_pz_fighter_grinding(void) {
    struct PuzzleGrinderNoisePdata* noise;
    struct PuzzleGrinderMeatController* meat;
    float effect_x;
    float effect_z;
    struct PuzzleFatalityHazardObject* body_sobj;
    PlyrPdata* player;

    pz_fighter_clear_out_external_forces();
    player = plyr_pdata;
    if (player->plyr_num == 0) {
        effect_x = g_pz_fighters_engine.fighter_posts[1].x;
        effect_z = g_pz_fighters_engine.fighter_posts[1].z;
    } else {
        effect_x = g_pz_fighters_engine.fighter_posts[0].x;
        effect_z = g_pz_fighters_engine.fighter_posts[0].z;
    }
    if (player->plyr_num == 0) {
        effect_x -= 0.4f;
    } else {
        effect_x += 0.4f;
    }

    bgnd_launch_fx_at_position(
        "grinding_fx", effect_x, plyr_obj->position.y, effect_z);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    bgnd_launch_fx_at_position(
        "blood_splat_fx", plyr_obj->position.x, 0.0f, plyr_obj->position.z);
    fx_set_render_priority(fx_by_owner("blood_splat_fx", 4), 11);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    snd_req_vol(0x1AB8, 1.0f);

    if (_create_mkproc_generic_bigstack(
            0x2001, 0x1F, p_grinder_noise, sizeof(struct PuzzleGrinderNoisePdata),
            (struct PuzzleFaceBleedPdata**)&noise) != 0 &&
        noise != 0) {
        noise->duration = 150;
        noise->timer = 0;
    }

    blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(12.0f);
    bgnd_launch_fx_at_position(
        "chunk_1", effect_x, plyr_obj->position.y, effect_z);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    snd_death_voice();
    ani_to_frame_x(22.0f);

    pz_launch_grinder_meat_alt0();
    snd_req_delay(0x1ADD, 40);
    body_sobj = obj_first_sobj(
        g_pz_fighter_fatality_engine.scene_objects[plyr_pdata->plyr_num]);
    if (body_sobj != 0 && body_sobj->material_data->geometry != 0) {
        RpGeometryForAllMaterials(
            body_sobj->material_data->geometry, material_set_texture,
            g_pz_fighter_fatality_engine.grinder_texture);
    }
    ani_to_frame_x(36.0f);

    pz_launch_grinder_meat_alt2();
    obj_set_bone_collapse_flag(plyr_obj, 20);
    snd_req(0x1ADC);
    snd_req_delay(0x1ADD, 35);
    snd_req(0x1AEF);
    bgnd_launch_fx_at_position(
        "chunk_2", effect_x, plyr_obj->position.y, effect_z);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    ani_to_end();
    blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(26.0f);

    pz_launch_grinder_meat_alt1();
    pz_launch_grinder_meat_default();
    snd_req(0x1AD7);
    snd_req_delay(0x1ADD, 35);
    snd_req_delay(0x1ADD, 50);
    ani_to_frame_x(34.0f);

    pz_launch_grinder_meat_final();

    obj_set_bone_collapse_flag(plyr_obj, 21);
    meat = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_meat_throw_controller, sizeof(struct PuzzleGrinderMeatController),
            &meat) != 0 &&
        meat != 0) {
        meat->direction = his_pdata->plyr_num;
        meat->object = g_pz_fighters_engine.grinder_meat_final;
        meat->delay = 0;
        meat->phase = 1;
    }

    bgnd_launch_fx_at_position(
        "chunk_3", effect_x, plyr_obj->position.y, effect_z);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    ani_to_blend_frame(10.0f);
    hide_obj(plyr_obj);
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_grinder_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static float pz_fighters_grinder_fatality_prep(void) {
    static int fatality_attackbegun;
    struct PuzzleFighterRenderObject* attacker_object;
    struct PuzzleFighterMove* victim_move;
    float delta_x;
    float target_x;
    float target_z;
    float attacker_x;
    float delta_z;
    float signed_distance;
    int direction;
    int event;
    int attacker;
    int victim;

    attacker = g_pz_fighters_engine.fatality_attacker;
    attacker_object = pz_fighter_get_player_obj(attacker);
    direction = 1;
    if (attacker == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[1].x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z;
    } else {
        target_x = g_pz_fighters_engine.fighter_posts[0].x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z;
    }

    attacker_x = attacker_object->position.x;
    delta_x = target_x - attacker_x;
    delta_z = target_z - attacker_object->position.z;
    if (target_x < 0.0f) {
        if (attacker_x < target_x) {
            direction = -1;
        }
    } else if (attacker_x > target_x) {
        direction = -1;
    }
    signed_distance =
        direction * ((delta_x * delta_x) + (delta_z * delta_z));

    if (signed_distance < 0.014f &&
        g_pz_fighters_engine.fatality_ready == 1) {
        if (fatality_attackbegun == 0) {
            event = 0;
            minigame_event(&event);
        }
        fatality_attackbegun = 0;
        g_pz_fighters_engine.fatality_active = 1;
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_attacker),
                  r_pz_fighter_grinding);
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_victim),
                  pz_fighter_disgusted_with_grinding);
        g_pz_fighters_engine.fatality_timer = 180;
    } else if (g_game_info.plyr0.slot.pdata->state == 0 &&
               g_game_info.plyr1.slot.pdata->state == 0) {
        if (fatality_attackbegun == 0) {
            fatality_attackbegun = 1;
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

static float pz_fighters_grinder_fatality_preround(void) {
    struct PuzzleGrinderMeatController* meat_controller;
    unsigned short direction_random;
    unsigned int direction;
    unsigned int delay;

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
            0xC001, 0x1F, p_grinder_meat_throw_controller, sizeof(struct PuzzleGrinderMeatController),
            &meat_controller) != 0 &&
        meat_controller != 0) {
        meat_controller->direction = direction;
        meat_controller->delay = delay;
        meat_controller->phase = 0;
    }
    return 0.0f;
}

static float pz_fighters_grinder_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/* TODO: [near miss] 99.90385%; secondary grinder r29/r30 coloring remains;
 * direct snapshot regresses bytes and two ng runs yield no honest closure. */
static float pz_fighter_load_and_place_initial_grinders(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* grinders[2];
    unsigned int i;
    struct PuzzleFatalityController* controller;

    load_art_section(0x70036, &sec_pz_danger_grinder);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_grinder_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 0;

    for (i = 0; i < 2; i++) {
        grinders[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x081E0000, 0x6021);
        grinders[i]->model_flags = 1;
        grinders[i]->position.x = i != 0 ? 2.2f : -2.2f;
        if (screen_width > 650) {
            grinders[i]->position.x = i != 0 ? 2.65f : -2.65f;
        }
        grinders[i]->position.y = 0.0f;
        grinders[i]->position.z = 0.0f;
        grinders[i]->flags_bits.rotation_enabled = 1;
        grinders[i]->motion_rate = 0.05f;
        grinders[i]->flags_bits.scale_active = 1;
        grinders[i]->scale.x = 0.6f;
        grinders[i]->scale.y = 0.6f;
        grinders[i]->scale.z = 0.6f;
        insert_fgnd_mkobj(grinders[i]);
    }

    g_pz_fighter_fatality_engine.scene_objects[0] = grinders[0];
    g_pz_fighter_fatality_engine.scene_objects[1] = grinders[1];
    obj_create_sobjs(grinders[0]);
    obj_create_sobjs(grinders[1]);
    g_pz_fighter_fatality_engine.grinder_texture =
        load_tga(0x70036, 0x081E0002);
    controller = 0;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_controller,
            sizeof(struct PuzzleFatalityController), &controller) != 0 &&
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
            g_pz_fighter_fatality_engine.scene_objects[0]->motion_rate;
        g_pz_fighter_fatality_engine.controller->grinder_position[1] =
            g_pz_fighter_fatality_engine.scene_objects[1]->motion_rate;
        g_pz_fighter_fatality_engine.controller->grinder_target[0] =
            g_pz_fighter_fatality_engine.controller->grinder_position[0];
        g_pz_fighter_fatality_engine.controller->grinder_target[1] =
            g_pz_fighter_fatality_engine.controller->grinder_position[1];
    }

    load_pz_fighter_fatality_bank(0x82);
    return 0.0f;
}

static float pz_fighter_grinder_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

static float pz_fighter_grinder_unload(void) {
    struct PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

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

static void pz_fighters_fatality_bird_in_place(int x, int y) {
    if (x > 300) {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 8;
        random_snd_req(0xB1);
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 7;
        random_snd_req(0xB1);
    }
}

static int dropped_heart_snd_cb(void) {
    snd_req(0x1ADD);
    return 0;
}

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
                    pz_chomper_start_motion(1, 2.5f, 0.28f);
                    g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 1;
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
                pz_chomper_reverse_motion(side, 2);
            }
        }
        for (side = 0; side < 2; side++) {
            pz_chomper_apply_motion(side, 2, 1);
        }
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
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
    }
    return 1.0f;
}

static float pz_fighters_chomper_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
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
        "blood_up_spray_fx", plyr_obj->position.x, plyr_obj->position.y, plyr_obj->position.z);
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

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

static inline float pz_fighter_signed_squared_distance_to_post(
    int attacker, struct PuzzleFighterRenderObject* object) {
    Vec offset = {0.5f, 0.0f, 0.0f};
    float object_x;
    float target_z;
    float delta_x;
    float target_x;
    float delta_z;
    int direction = 1;

    if (attacker == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[1].x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z;
        target_x += offset.x;
        target_z += offset.z;
    } else {
        target_x = g_pz_fighters_engine.fighter_posts[0].x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z;
        target_x -= offset.x;
        target_z -= offset.z;
    }
    object_x = object->position.x;
    delta_x = target_x - object_x;
    delta_z = target_z - object->position.z;
    if (target_x < 0.0f) {
        if (object_x < target_x) {
            direction = -1;
        }
    } else if (object_x > target_x) {
        direction = -1;
    }
    return direction * ((delta_x * delta_x) + (delta_z * delta_z));
}

static float pz_fighters_chomper_fatality_prep(void) {
    static int attack_begun;
    struct PuzzleFatalityHazardObject *chomper;
    struct PuzzleFighterRenderObject *attacker_object;
    struct PuzzleFighterMove *victim_move;
    PlyrPdata *attacker_data;
    float edge = 2.05f;
    float signed_distance_sq;
    float player_distance;
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
        signed_distance_sq = pz_fighter_signed_squared_distance_to_post(
                attacker, attacker_object);
        hazard_ready = 0;

        if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 0) {
            if (g_pz_fighter_fatality_engine.scene_objects[0]->position.x < -edge) {
                g_pz_fighter_fatality_engine.scene_objects[0]->external_force.x = 0.02f;
            } else {
                g_pz_fighter_fatality_engine.scene_objects[0]->external_force.x = 0.0f;
            }
        } else {
            if (g_pz_fighter_fatality_engine.scene_objects[1]->position.x > edge) {
                g_pz_fighter_fatality_engine.scene_objects[1]->external_force.x =
                        -0.02f;
            } else {
                g_pz_fighter_fatality_engine.scene_objects[1]->external_force.x = 0.0f;
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
                                ->external_force.x == 0.0f) {
            hazard_ready = 1;
        }

        if (signed_distance_sq < 0.02f) {
            attacker_data = pz_get_pdata_by_id(
                    g_pz_fighters_engine.fatality_attacker);
            pz_fighter_clear_out_all_external_forces(g_game_info.plyr0.slot.mirror_a);
            pz_fighter_clear_out_all_external_forces(g_game_info.plyr1.slot.mirror_a);
            if (attack_begun == 0) {
                attack_begun = 1;
                event = 0;
                minigame_event(&event);
            }

            if (g_game_info.plyr0.slot.pdata->state == 0 &&
                    g_game_info.plyr1.slot.pdata->state == 0) {
                if (hazard_ready == 1) {
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
                }
            } else if (attacker_data->state != 0) {
                if (attack_begun == 0) {
                    attack_begun = 1;
                    event = 0;
                    minigame_event(&event);
                }
                attacker_data->state = 0;
                xfer_proc(pz_fighter_get_player_proc(
                                            g_pz_fighters_engine.fatality_attacker),
                                    pz_fighter_exit);
                xfer_proc(
                        pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
                        pz_fighter_exit);
            }
            break;
        }

        if (g_game_info.plyr0.slot.pdata->state != 0 ||
                g_game_info.plyr1.slot.pdata->state != 0) {
            break;
        }
        if (attack_begun == 0) {
            attack_begun = 1;
            event = 0;
            minigame_event(&event);
        }
        if ((unsigned int)g_pz_fighters_engine.fatality_victim == 1) {
            g_game_info.plyr0.slot.pdata->fatality_shove_active = 1;
        } else {
            g_game_info.plyr1.slot.pdata->fatality_shove_active = 1;
        }
        victim = g_pz_fighters_engine.fatality_victim;
        victim_move = pz_get_fighter_move();
        g_pz_fighters_engine.fighter_reaction_cooldown = 0;
        victim_move->active = 1;
        if (signed_distance_sq < 1.3f) {
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

/* TODO: [near miss] 99.72%; TU data layout: Vec initializers sit at pool @503+0x354 in retail
 * (ours +0x180), plus later vector stack slots and one face-bleed result copy. */
static float pz_fighters_chomper_fatality_in_progress(void) {
    static int mode_timer;
    static int launch_sounds = 1;
    static float flesh_path_timer;
    struct PuzzleFatalityHazardObject* spike;
    struct PuzzleFighterRenderObject* attacker_object;
    PlyrPdata* attacker_data;
    PlyrPdata* victim_data;
    struct PuzzleFighterRenderObject* saved_object;
    PlyrPdata* saved_pdata;
    struct PuzzleFaceBleedPdata* bleed_data;
    struct PuzzleFaceBleedProcess* bleed_process;
    RpMaterial* material;

    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 2: {
        Vec impact_position;
        Vec head_offset = {0.0f, 0.6f, 0.0f};
        attacker_object =
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker);
        victim_data =
            pz_get_pdata_by_id(g_pz_fighters_engine.fatality_victim);
        if (victim_data->state == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_one_arm_victory2);
        }

        get_bone_world_pos(attacker_object, 0x10, &impact_position);
        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
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
            0x2001, 0x1F, p_face_bleeding, sizeof(struct PuzzleFaceBleedPdata),
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

    case 3: {
        Vec impact_position;
        attacker_object =
            pz_fighter_get_player_obj(
                g_pz_fighters_engine.fatality_attacker);
        victim_data =
            pz_get_pdata_by_id(g_pz_fighters_engine.fatality_victim);
        if (victim_data->state == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_one_arm_victory2);
        }
        get_bone_world_pos(attacker_object, 0x10, &impact_position);

        spike = g_pz_fighter_fatality_engine
                    .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                    .objects[0];
        if (spike->y < 3.95f) {
            spike->motion = 0.12f;
            attacker_object->secondary_flags_bits.stopped = 0;
            attacker_object->external_force.y = 0.09f;
            mode_timer = 0;
        } else if (mode_timer == 0) {
            spike->motion = 0.0f;
            attacker_object->external_force.y = 0.0f;
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
    }

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
            attacker_object->external_force.y = 0.0f;
            pz_fighter_shake_camera(1, 0.03f);
            snd_req(0x1ADB);
            snd_req_vol(0x1AAE, 1.0f);
            snd_req_delay(0x1AAF, 7);

            spike = g_pz_fighter_fatality_engine
                        .hazard_groups[g_pz_fighters_engine.fatality_attacker]
                        .objects[0];
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
            if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 1) {
                g_pz_fighters_engine.fatality_piece->position.x = 1.56f;
                g_pz_fighters_engine.fatality_piece->position.z = -0.27f;
            } else {
                g_pz_fighters_engine.fatality_piece->position.x = -1.62f;
                g_pz_fighters_engine.fatality_piece->position.z = -0.28f;
            }
            g_pz_fighters_engine.fatality_piece->position.y = 0.2f;
            g_pz_fighters_engine.fatality_piece->flags_bits.gravity_enabled = 1;
            g_pz_fighters_engine.fatality_piece->external_force.x = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force.y = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force.z = 0.0f;
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
            g_pz_fighters_engine.fatality_piece->external_force.y = 0.206f;
            unhide_obj(g_pz_fighters_engine.fatality_piece);
            if ((randu0(100) & 0xFFFF) < 70) {
                Vec velocity = {0.0f, -0.001f, 0.0f};
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->position.x,
                    &velocity, attacker_data);
            }
        } else if (spike->y < 3.95f) {
            spike->motion = 0.17f;
            g_pz_fighters_engine.fatality_piece->external_force.y = 0.138f;
            flesh_path_timer = 5.0f;
            if ((randu0(100) & 0xFFFF) < 70) {
                Vec velocity = {0.0f, -0.001f, 0.0f};
                attacker_data =
                    pz_get_pdata_by_id(
                        g_pz_fighters_engine.fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->position.x,
                    &velocity, attacker_data);
            }
        } else {
            flesh_path_timer -= 1.0f;
            spike->motion = 0.0f;
            g_pz_fighters_engine.fatality_piece->external_force.y = 0.0f;
            if (g_pz_fighters_engine.fatality_motion == 1.0f &&
                flesh_path_timer <= 0.0f) {
                Vec flesh_velocity = {0.0f, 0.05f, 0.0f};
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
                    "gusher0", 0, &g_pz_fighters_engine.fatality_piece->position.x,
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
                "gusher0", 0, &g_pz_fighters_engine.fatality_piece->position.x,
                &velocity, attacker_data);
            velocity.x = sfrand(0.1f);
            velocity.z = sfrand(0.1f);
            spawn_bld_fall(
                "gusher0", 0, &g_pz_fighters_engine.fatality_piece->position.x,
                &velocity, attacker_data);
        }
        break;
    }

    return 0.0f;
}

static inline void pz_fighter_register_chomper_columns(
    struct PuzzleFighterRenderObject* primary, struct PuzzleFighterRenderObject* secondary) {
    g_pz_fighter_fatality_engine.scene_objects[0] = primary;
    g_pz_fighter_fatality_engine.scene_objects[1] = secondary;
    obj_create_sobjs(primary);
    obj_create_sobjs(secondary);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(primary, 2);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(secondary, 2);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[1] =
        obj_find_sobj_by_id(primary, 1);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[1] =
        obj_find_sobj_by_id(secondary, 1);
}

static float pz_fighter_load_and_place_initial_chompers(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* columns[2];
    struct PuzzleFatalityController* controller;
    unsigned int i;
    unsigned int j;

    load_art_section(0x70036, &sec_pz_danger_chomper);
    g_pz_fighters_engine.fatality_index = 1;

    for (i = 0; i < 2; i++) {
        columns[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x081F0000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags_bits.scale_active = 1;
        if (screen_width > 650) {
            columns[i]->position.x = i != 0 ? 4.1f : -4.1f;
            columns[i]->position.y = 0.0f;
            columns[i]->position.z = -0.24f;
            columns[i]->scale.x = 1.1f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.14f;
        } else {
            columns[i]->position.x = i != 0 ? 3.6f : -3.6f;
            columns[i]->position.y = 0.0f;
            columns[i]->position.z = -0.24f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.04f;
        }
        columns[i]->flags_bits.gravity_enabled = 1;
        columns[i]->external_force.x = 0.0f;
        columns[i]->external_force.y = 0.0f;
        columns[i]->external_force.z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    pz_fighter_register_chomper_columns(columns[0], columns[1]);
    g_pz_fighter_fatality_engine.grinder_texture =
        load_tga(0x70036, 0x081F0002);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_chomper_controller, sizeof(struct PuzzleFatalityController),
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
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->flags_bits.airborne = 1;
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->flags_bits.gravity_enabled = 1;
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->motion = 0.0f;
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

static inline void reset_chomper_motion(void) {
    unsigned int group;
    int object;
    for (group = 0; group < 2; group++) {
        for (object = 0; object < 2; object++) {
            g_pz_fighter_fatality_engine.hazard_groups[group].objects[object]->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller->hazard_motion[group][object] = 0.0f;
        }
    }
}

static float pz_fighter_chomper_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    reset_chomper_motion();
    return 0.0f;
}

static float pz_fighter_chomper_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static inline void reset_chomper_hazard_motion(void) {
    unsigned int group;
    int object;

    for (group = 0; group < 2; group++) {
        for (object = 0; object < 2; object++) {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[object]->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][object] = 0.0f;
        }
    }
}

static void pz_fighter_chomper_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

    reset_chomper_hazard_motion();
}

static float pz_fighters_chomper2_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

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
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 0;
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 0;
    }
    return 1.0f;
}

/* TODO: [breakthrough] 99.80%; retail chunk-crush effects fixed; four prologue GPR operands remain. */
static float pz_fighter_chomper2_victim_crushed(void) {
    Vec blood_offset = {0.05f, 0.0f, 0.0f};
    float blood_x;
    float blood_z;

    if (plyr_pdata->plyr_num == 0) {
        blood_x = g_pz_fighters_engine.fighter_posts[1].x;
        blood_z = g_pz_fighters_engine.fighter_posts[1].z;
        blood_x += blood_offset.x;
        blood_z += blood_offset.z;
    } else {
        blood_x = g_pz_fighters_engine.fighter_posts[0].x;
        blood_z = g_pz_fighters_engine.fighter_posts[0].z;
        blood_x -= blood_offset.x;
        blood_z -= blood_offset.z;
    }
    blood_z -= 0.2f;
    bgnd_launch_fx_at_position(
        "chunk_crush1_fx", blood_x, 0.1f, blood_z);
    if (plyr_pdata->plyr_num == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    init_ground_move();
    blend_to_ani(pz_shared_ani.objects_falling_crushed, 3, 0.1f);
    set_ani_speed(0.2f);
    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    obj_set_bone_collapse_flag(plyr_obj, 9);
    ani_loop_more_frames(2.0f);
    bgnd_launch_fx_at_position(
        "chunk_crush2_fx", blood_x, 0.1f, blood_z);
    ani_to_end();
    obj_set_bone_collapse_flag(plyr_obj, 0x14);
    obj_set_bone_collapse_flag(plyr_obj, 0x15);
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
    return 0.0f;
}

#pragma opt_propagation off
/* TODO: [near miss] 99.32%; motion-array base and stfsx operand order differ;
 * typed owner/helper forms and propagation control did not close the residue. */
static float pz_fighter_chomper2_actively_fighting(int active) {
    int group;
    float (*motion)[2];

    g_pz_fighter_fatality_engine.controller->phase = active;
    if (active == 0) {
        for (group = 0; group < 2; group++) {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[0]
                ->motion = 0.0f;
            motion = g_pz_fighter_fatality_engine.controller->hazard_motion;
            motion[group][0] = 0.0f;
        }
    }
    return 0.0f;
}
#pragma opt_propagation reset

/* TODO: [Scope warn] offset block: moving initializer to case entry drops 100% to 96.27%. */
static float pz_fighters_chomper2_fatality_prep(void) {
    static int fatality_attackbegun;
    struct PuzzleFatalityHazardObject *chomper;
    struct PuzzleFighterRenderObject *attacker_object;
    struct PuzzleFighterMove *victim_move;
    PlyrPdata *attacker_data;
    PlyrPdata *victim_data;
    float target_x;
    float delta_x;
    float delta_z;
    float target_z;
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
        float attacker_x;

        attacker = g_pz_fighters_engine.fatality_attacker;
        attacker_object = pz_fighter_get_player_obj(attacker);
        direction = 1;
        {
            Vec offset = {0.05f, 0.0f, 0.0f};
            if (attacker == 0) {
                target_x = g_pz_fighters_engine.fighter_posts[1].x + offset.x;
                target_z = g_pz_fighters_engine.fighter_posts[1].z;
                target_z += offset.z;
            } else {
                target_x = g_pz_fighters_engine.fighter_posts[0].x - offset.x;
                target_z = g_pz_fighters_engine.fighter_posts[0].z;
                target_z -= offset.z;
            }
        }

        attacker_x = attacker_object->position.x;
        delta_x = target_x - attacker_x;
        delta_z = target_z - attacker_object->position.z;
        if (target_x < 0.0f) {
            if (attacker_x < target_x) {
                direction = -1;
            }
        } else if (attacker_x > target_x) {
            direction = -1;
        }
        hazard_ready = 0;
        signed_distance = direction * ((delta_x * delta_x) + (delta_z * delta_z));

        edge = 1.15f;
        if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 0) {
            if (g_pz_fighter_fatality_engine.scene_objects[0]->position.x < -edge) {
                g_pz_fighter_fatality_engine.scene_objects[0]->external_force.x = 0.02f;
            } else {
                g_pz_fighter_fatality_engine.scene_objects[0]->external_force.x = 0.0f;
            }
        } else {
            if (g_pz_fighter_fatality_engine.scene_objects[1]->position.x > edge) {
                g_pz_fighter_fatality_engine.scene_objects[1]->external_force.x =
                        -0.02f;
            } else {
                g_pz_fighter_fatality_engine.scene_objects[1]->external_force.x = 0.0f;
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
                                ->external_force.x == 0.0f) {
            hazard_ready = 1;
        }

        if (signed_distance < 0.02f) {
            attacker_data = pz_get_pdata_by_id(
                    g_pz_fighters_engine.fatality_attacker);
            pz_fighter_clear_out_all_external_forces(g_game_info.plyr0.slot.mirror_a);
            pz_fighter_clear_out_all_external_forces(g_game_info.plyr1.slot.mirror_a);
            if (fatality_attackbegun == 0) {
                fatality_attackbegun = 1;
                event = 0;
                minigame_event(&event);
            }

            if (g_game_info.plyr0.slot.pdata->state == 0 &&
                    g_game_info.plyr1.slot.pdata->state == 0) {
                if (hazard_ready == 1) {
                    player_distance = xz_distance_between_players();
                    g_pz_fighter_fatality_engine.active_effect = 1;
                    fatality_attackbegun = 0;
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
                }
            } else if (attacker_data->state != 0) {
                attacker_data->state = 0;
                xfer_proc(pz_fighter_get_player_proc(
                                            g_pz_fighters_engine.fatality_attacker),
                                    pz_fighter_exit);
                xfer_proc(
                        pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
                        pz_fighter_exit);
            }
            break;
        }

        if (g_game_info.plyr0.slot.pdata->state != 0 ||
                g_game_info.plyr1.slot.pdata->state != 0) {
            break;
        }
        if (fatality_attackbegun == 0) {
            fatality_attackbegun = 1;
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
        if (victim_data->state != 0) {
            break;
        }
        if (attacker_data->state != 0) {
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

/* TODO: [Scope warn] blood_offset block: moving initializer to launch_sounds branch entry drops 100% to 91.28%. */
static float pz_fighters_chomper2_fatality_in_progress(void) {
    static int launch_sounds = 1;
    static int launch_more_meat_chunks = 1;
    struct PuzzleFatalityHazardObject* crusher;
    PlyrPdata* attacker_data;
    struct PuzzleFighterRenderObject* attacker_object;
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

                    if (attacker_data->plyr_num == 0) {
                        blood_x = g_pz_fighters_engine.fighter_posts[1].x;
                        blood_z = g_pz_fighters_engine.fighter_posts[1].z;
                        blood_x += blood_offset.x;
                        blood_z += blood_offset.z;
                    } else {
                        blood_x = g_pz_fighters_engine.fighter_posts[0].x;
                        blood_z = g_pz_fighters_engine.fighter_posts[0].z;
                        blood_x -= blood_offset.x;
                        blood_z -= blood_offset.z;
                    }
                }
                blood_z -= 0.2f;

                bgnd_launch_fx_at_position(
                    "blood_crush_fx", blood_x, 0.1f, blood_z);
                if (attacker_data->plyr_num == 1) {
                    bgnd_set_fx_ang_y(3.1415927f);
                }
                bgnd_launch_fx_at_position(
                    "blood_splat_fx", blood_x, 0.1f, blood_z);
                if (attacker_data->plyr_num == 1) {
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
            attacker_object->external_force.y = 0.0f;
            snd_req(0x1ADB);
            snd_req_vol(0x1AAE, 1.0f);
            snd_req_delay(0x1AAF, 7);
            plyr_pdata =
                pz_get_pdata_by_id(g_pz_fighters_engine.fatality_attacker);
            snd_major_hit_voice();
            g_pz_fighter_fatality_engine.active_effect = 4;
        }
        break;
    case 4:
        break;
    }

    return 0.0f;
}

static inline void pz_fighter_register_chomper2_columns(
    struct PuzzleFighterRenderObject* primary, struct PuzzleFighterRenderObject* secondary) {
    g_pz_fighter_fatality_engine.scene_objects[0] = primary;
    g_pz_fighter_fatality_engine.scene_objects[1] = secondary;
    obj_create_sobjs(primary);
    obj_create_sobjs(secondary);
    g_pz_fighter_fatality_engine.hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(primary, 1);
    g_pz_fighter_fatality_engine.hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(secondary, 1);
}

static float pz_fighter_load_and_place_initial_chompers2(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* columns[2];
    struct PuzzleFatalityController* controller;
    unsigned int i;
    unsigned int j;

    load_art_section(0x70036, &sec_pz_danger_crusher);
    g_pz_fighters_engine.fatality_index = 2;

    for (i = 0; i < 2; i++) {
        columns[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08200000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags_bits.scale_active = 1;
        if (screen_width > 650) {
            columns[i]->position.x = i != 0 ? 2.15f : -2.15f;
            columns[i]->position.y = 0.0f;
            columns[i]->position.z = -0.1f;
            columns[i]->scale.x = 1.2f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.3f;
        } else {
            columns[i]->position.x = i != 0 ? 1.75f : -1.75f;
            columns[i]->position.y = 0.0f;
            columns[i]->position.z = -0.1f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.1f;
        }
        columns[i]->flags_bits.gravity_enabled = 1;
        columns[i]->external_force.x = 0.0f;
        columns[i]->external_force.y = 0.0f;
        columns[i]->external_force.z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    pz_fighter_register_chomper2_columns(columns[0], columns[1]);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_chomper2_controller, sizeof(struct PuzzleFatalityController),
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
        for (j = 0; j < 1; j++) {
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->flags_bits.airborne = 1;
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->flags_bits.gravity_enabled = 1;
            g_pz_fighter_fatality_engine.hazard_groups[i]
                .objects[j]->motion = 0.0f;
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
    load_effect_bank_with_context("pz_crusher_fx.mko", &effect_context);
    return 0.0f;
}

#pragma opt_propagation off
/* TODO: [near miss] 93.13%; initial phase/index zero is shared; motion-array base and indexed-store operands remain. */
static float pz_fighter_chomper2_round_over(void) {
    int i;
    float (*motion)[2];

    i = 0;
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    for (; i < 2; i++) {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        motion = g_pz_fighter_fatality_engine.controller->hazard_motion;
        motion[i][0] = 0.0f;
    }
    return 0.0f;
}
#pragma opt_propagation reset

static float pz_fighter_chomper2_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

#pragma opt_propagation off
/* TODO: [near miss] 95.625%; scalar reset zero shares the hazard-stride register;
 * motion-row address home remains different; structured alternatives exhausted. */
static void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim) {
    int i;
    float (*motion)[2];

    i = 0;
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->state =
        g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

    for (; i < 2; i++) {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        motion = g_pz_fighter_fatality_engine.controller->hazard_motion;
        motion[i][0] = 0.0f;
    }
}
#pragma opt_propagation reset

static void pz_fighter_set_objects_falling_obj(
    struct PuzzleFighterRenderObject* object1,
    struct PuzzleFighterRenderObject* object2) {
    int side;
    unsigned int index;

    g_pz_fighter_fatality_engine.scene_objects[0] = object1;
    g_pz_fighter_fatality_engine.scene_objects[1] = object2;
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

static float pz_fighters_objects_falling_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    if (g_pz_fighter_fatality_engine.controller
            ->preround_sound_started == 0) {
        snd_req(0x1ACB);
    }
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 1;
    return 0.0f;
}

static float p_objects_falling_controller2(void) {
    struct PuzzleFatalityHazardObject** first;
    struct PuzzleFatalityHazardObject** second;
    struct PuzzleFatalityHazardObject** third;
    unsigned int i;

    if (g_pz_fighter_fatality_engine.controller->unload_requested == 1) {
        return -1.0f;
    }

    if (g_pz_fighter_fatality_engine.controller->preround_active == 1) {
        for (i = 0; i < 2; i++) {
            switch (g_pz_fighter_fatality_engine.controller
                        ->hazard_initialized[i]) {
            case 1:
                first = &g_pz_fighter_fatality_engine.hazard_groups[i].objects[0];
                second = &g_pz_fighter_fatality_engine.hazard_groups[i].objects[1];
                third = &g_pz_fighter_fatality_engine.hazard_groups[i].objects[2];
                (*first)->x = 1.7f;
                (*second)->x = 1.7f;
                (*third)->x = 1.7f;
                if (screen_width > 650) {
                    (*first)->x = 2.1f;
                    (*second)->x = 2.1f;
                    (*third)->x = 2.1f;
                }
                if (i == 1) {
                    (*first)->x *= -1.0f;
                    (*second)->x *= -1.0f;
                    (*third)->x *= -1.0f;
                }
                _mkproc_sleep_ticks = 1.0f;
                aproc->vtbl->sleep(aproc->vtbl);
                unhide_sobj((*first));
                unhide_sobj((*second));
                unhide_sobj((*third));
                g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[i] = 0;
                (*first)->motion = 0.02f;
                (*second)->motion = 0.02f;
                break;
            }
            if (g_pz_fighter_fatality_engine.controller
                    ->hazard_initialized[i] == 0) {
                if (g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->y > 4.0f) {
                    g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion = 0.0f;
                    g_pz_fighter_fatality_engine.hazard_groups[i].objects[1]->motion = 0.0f;
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

/* TODO: [near miss] 99.93%; nine equal-value Vec initializer pool offsets differ;
 * recover missing TU rodata prefix before link equivalence. */
static void pz_fighter_fatality_launch_eyes(void) {
    struct PuzzleFighterRenderObject* left_eye = g_pz_fighters_engine.left_eye;
    struct PuzzleFighterRenderObject* right_eye = g_pz_fighters_engine.right_eye;
    Vec launch_offset = {0.0f, 0.1f, 0.02f};
    Vec left_offset = {-0.05f, 0.0f, 0.1f};
    Vec right_offset = {0.05f, 0.0f, 0.1f};
    struct PuzzleBoneData* bone_data;
    MKMATRIX* bone_matrix;
    Vec bone_position;
    Vec eye_position;
    Vec camera_position;
    Vec camera_direction;
    float angle_offset;
    struct PuzzleFighterPhysics* fighter =
        (struct PuzzleFighterPhysics*)g_game_info.plyr0.slot.mirror_a;

    if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 1) {
        fighter = (struct PuzzleFighterPhysics*)g_game_info.plyr1.slot.mirror_a;
    }

    if (((fighter->flags_0A >> 6) & 1) != 0) {
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
        (struct PuzzleFighterRenderObject*)fighter, 0x10, &bone_position);
    calc_bone_world_mat((struct PuzzleFighterRenderObject*)fighter, 0x10);
    bone_data = ((struct PuzzleBoneObjectView*)fighter)->bone_data;
    bone_matrix = bone_data->matrix;
    if (bone_matrix == 0) {
        return;
    }

    v3_x_mat_add_v3(
        &eye_position, &left_offset, bone_matrix, &bone_position);
    left_eye->position.x = eye_position.x;
    left_eye->position.y = eye_position.y;
    left_eye->position.z = eye_position.z;
    v3_x_mat_add_v3(
        &eye_position, &right_offset, bone_matrix, &bone_position);
    right_eye->position.x = eye_position.x;
    right_eye->position.y = eye_position.y;
    right_eye->position.z = eye_position.z;

    left_eye->flags_bits.gravity_enabled = 1;
    get_camera_position(&camera_position);
    v3_sub_v3(
        &camera_direction, &camera_position, &left_eye->position);
    scale_v3(
        &left_eye->external_force, &camera_direction, 0.02f);
    left_eye->external_force.y -= 0.0008f;

    right_eye->flags_bits.gravity_enabled = 1;
    scale_v3(
        &right_eye->external_force, &camera_direction, 0.03f);
    right_eye->external_force.y += 0.0013f;

    _mkproc_sleep_ticks = 2.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    unhide_obj(left_eye);
    unhide_obj(right_eye);
}

static float pz_fighter_objects_falling_victim_crushed(void) {
    bgnd_launch_fx_at_position(
        "chunk_crush1_fx", plyr_obj->position.x, plyr_obj->position.y, plyr_obj->position.z);
    init_ground_move();
    blend_to_ani(pz_shared_ani.objects_falling_crushed, 3, 0.5f);
    set_ani_speed(1.0f);
    ani_loop_more_frames(2.0f);
    bgnd_launch_fx_at_position(
        "chunk_crush2_fx", plyr_obj->position.x, plyr_obj->position.y, plyr_obj->position.z);
    ani_loop_more_frames(2.0f);
    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
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

/* TODO: [near miss] 99.65%; FPR coloring of the post/offset targets and victim-x delta remains (offset f2/f1, delta_x f5/f2). */
/* TODO: [Scope warn] offset block: moving initializer to unlocked-player branch entry drops 99.65% to 92.15%. */
static float pz_fighters_objects_falling_fatality_prep(void) {
    static int one_last_hit = 1;
    static int start_pointing = 1;
    struct PuzzleFighterRenderObject *victim_object;
    struct PuzzleFighterRenderObject *attacker_object;
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
            if (g_game_info.plyr0.slot.pdata->state == 0 &&
                    g_game_info.plyr1.slot.pdata->state == 0) {
                int victim;

                player_distance = xz_distance_between_players();
                victim = g_pz_fighters_engine.fatality_victim;

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

                delta_x = target_x - victim_object->position.x;
                delta_z = target_z - victim_object->position.z;
                if (target_x < 0.0f) {
                    if (victim_object->position.x < target_x) {
                        direction = -1;
                    }
                } else if (victim_object->position.x > target_x) {
                    direction = -1;
                }
                signed_distance =
                        direction * ((delta_x * delta_x) + (delta_z * delta_z));

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
                            ->state != 0) {
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
            struct PuzzleFatalityHazardObject *falling_object;

            g_pz_fighter_fatality_engine.active_effect = 2;
            falling_object = g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
            falling_object->flags_bits.airborne = 1;
            falling_object->flags_bits.gravity_enabled = 1;
            falling_object->x = attacker_object->position.x;
            falling_object->z = attacker_object->position.z;
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

static float pz_fighters_objects_falling_fatality_in_progress(void) {
    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 2: {
        struct PuzzleFatalityHazardObject* falling_object;
        Vec bone_offset = {0.0f, 0.6f, 0.0f};
        Vec impact_position;

        struct PuzzleFighterRenderObject* attacker;

        attacker = pz_fighter_get_player_obj(
            g_pz_fighters_engine.fatality_attacker);
        falling_object =
            g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
        get_bone_world_pos(attacker, 0x10, &impact_position);
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
    case 3: {
        struct PuzzleFatalityHazardObject* falling_object;
        PlyrPdata* victim_data;

        falling_object =
            g_pz_fighter_fatality_engine.hazard_groups[0].objects[0];
        victim_data = pz_get_pdata_by_id(
            g_pz_fighters_engine.fatality_victim);
        if (falling_object->y < 0.25f) {
            falling_object->motion = 0.0f;
            falling_object->flags_bits.gravity_enabled = 0;
        }
        if (victim_data->state == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fatality_victim),
                pz_fighter_won2);
        }
        break;
    }
    }

    return 0.0f;
}

static float pz_fighter_load_and_place_initial_objects_falling(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* objects[2];
    struct PuzzleFatalityController* controller;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_1ton);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_1ton_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 3;

    for (i = 0; i < 2; i++) {
        objects[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08210000, 0x6021);
        objects[i]->model_flags = 1;
        hide_obj(objects[i]);
        insert_fgnd_mkobj(objects[i]);
    }
    pz_fighter_set_objects_falling_obj(objects[0], objects[1]);

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_objects_falling_controller2, sizeof(struct PuzzleFatalityController),
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

static float pz_fighter_objects_falling_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

static float pz_fighter_objects_falling_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
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

static float pz_fighters_lightning_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/* TODO: [near miss] 99.46%; bolt/effect ownership and stale-instance checks agree;
 * inlined helper GPR coloring remains; stop at this soft ceiling. */
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

static inline void pz_lightning_target_position(int victim, float* target_x, float* target_z) {
    Vec offset = {0.05f, 0.0f, 0.0f};
    if (screen_width > 650) {
        offset.x = 0.35f;
    }
    if (victim == 0) {
        *target_x = g_pz_fighters_engine.fighter_posts[1].x;
        *target_x += offset.x;
        *target_z = g_pz_fighters_engine.fighter_posts[1].z;
        *target_z += offset.z;
    } else {
        *target_x = g_pz_fighters_engine.fighter_posts[0].x;
        *target_x -= offset.x;
        *target_z = g_pz_fighters_engine.fighter_posts[0].z;
        *target_z -= offset.z;
    }
}

/* TODO: [near miss] 99.82%; typed target-position helper removes bare scopes;
 * current-x/delta-x FPR coloring remains after owner/scope/boundary trials. */
static float pz_fighters_lightning_fatality_prep(void) {
    static int one_last_hit = 1;
    static int start_pointing = 1;
    int victim;
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
    struct PuzzleFighterRenderObject *victim_object;

    lightning_smoke = fx_by_owner("pz_lightningsmoke", 4);
    burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    fx_pause_emit(burning_smoke);
    fx_pause_emit(lightning_smoke);

    switch (g_pz_fighter_fatality_engine.active_effect) {
    case 0:
        if (one_last_hit == 1) {
            if (g_game_info.plyr0.slot.pdata->state == 0 &&
                    g_game_info.plyr1.slot.pdata->state == 0) {
                player_distance = xz_distance_between_players();
                victim = g_pz_fighters_engine.fatality_victim;

                victim_object = pz_fighter_get_player_obj(victim);
                direction = 1;
                pz_lightning_target_position(victim, &target_x, &target_z);

                delta_x = target_x - victim_object->position.x;
                delta_z = target_z - victim_object->position.z;
                if (target_x < 0.0f) {
                    if (victim_object->position.x < target_x) {
                        direction = -1;
                    }
                } else if (victim_object->position.x > target_x) {
                    direction = -1;
                }
                signed_distance =
                        direction * ((delta_x * delta_x) + (delta_z * delta_z));

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
                            ->state != 0) {
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

static float pz_fighter_lightning_strike_victim_1(void) {
    unsigned int cycle;
    unsigned int frame;
    unsigned int shock_frame = 0;
    void* spark = fx_by_owner("pz_spark", 4);
    void* head_zap = fx_by_owner("pz_headzap", 4);
    void* burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    void* burning_after = fx_by_owner("pz_burningsmoke_after", 4);
    void* blood_burst = fx_by_owner("pz_blood_burst", 4);
    void* chunk = fx_by_owner("pz_chunk", 4);
    int frozen = 0;
    struct PuzzleFighterRenderObject* bolt;
    int voice_sound;
    int electrical_sound;
    struct PuzzleParticleEffect* particle;
    AniTextureControl* texture;
    Vec head;
    float volume;

    init_ground_move_no_aniproc();
    snd_req(0x1ABB);
    ani_loop_more_frames(5.0f);

    head.x = plyr_obj->position.x;
    head.y = plyr_obj->position.y;
    head.z = plyr_obj->position.z;
    head.y = 0.0f;
    get_bone_world_pos(plyr_obj, 0x10, &head);
    head.y = 0.1f + head.y;
    bolt = (struct PuzzleFighterRenderObject*)load_named_model_from_slot(
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
    pfx_get_emitter(particle->emitters, 0)->hidden = 0;

    fx_reset(spark);
    fx_resume_emit(spark);
    fx_set_param_v3(spark, 0x202, head.x, head.y, head.z);
    fx_reset(burning_smoke);
    fx_set_param_v3(burning_smoke, 0x202, 0.0f, 0.0f, 0.0f);
    fx_resume_emit(burning_smoke);
    particle = find_pfx_by_name("pz_burningsmoke");
    restart_effect_ppfx(particle);
    pfx_bind_emitter_to_obj_bone(particle, plyr_obj, 0x10);
    pfx_get_emitter(particle->emitters, 0)->hidden = 0;

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
            head.y = 0.1f + head.y;
            bolt->position.x = head.x;
            bolt->position.y = head.y;
            bolt->position.z = head.z;
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
    pfx_get_emitter(particle->emitters, 0)->hidden = 0;
    pz_fighter_shake_camera(3, 0.03f);
    pz_fighter_fatality_launch_eyes();

    fx_reset(blood_burst);
    fx_resume_emit(blood_burst);
    get_bone_world_pos(plyr_obj, 0x10, &head);
    head.y = 0.1f + head.y;
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
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 1);
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

static float pz_fighters_lightning_fatality_in_progress(void) {
    struct PuzzleFightersEngine* fighters;
    struct PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PlyrPdata* victim_data;

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
        if (victim_data->state == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_victim),
                pz_fighter_won2);
        }
        break;
    }

    return 0.0f;
}

static float pz_fighter_load_and_place_initial_lightning(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFatalityController* controller;

    load_art_section(0x70036, &sec_pz_danger_lightning);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_lightning_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 4;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_lightning_controller, sizeof(struct PuzzleFatalityController),
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

static float pz_fighter_lightning_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

static float pz_fighter_lightning_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
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

static float r_pz_fighter_eaten(void) {
    struct PuzzleParticleEffect* blood_burst;
    struct PuzzleParticleEffect* neck_blood;
    void* mouth_blood;
    void* mouth_chunks;
    void* saliva;
    void* neck_effect;
    unsigned int i;

    if (plyr_pdata->plyr_num == 0) {
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

    if ((unsigned int)g_pz_fighters_engine.fatality_victim == 0) {
        saliva = fx_by_owner("saliva1", 4);
        fx_resume_emit(saliva);
    } else {
        saliva = fx_by_owner("saliva2", 4);
        fx_resume_emit(saliva);
    }
    ani_loop_more_frames(11.0f);
    ani_loop_more_frames(17.0f);
    xfer_proc(
        pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
        pz_fighter_disgusted_with_grinding);
    pz_fighter_shake_camera(3, 0.03f);
    snd_req(0x1AC4);

    blood_burst =
        fx_by_owner("pz_blood_burst", 4);
    fx_reset(blood_burst);
    fx_resume_emit(blood_burst);
    blood_burst = find_pfx_by_name("pz_blood_burst");
    restart_effect_ppfx(blood_burst);
    pfx_bind_emitter_to_obj_bone(blood_burst, plyr_obj, 9);
    pfx_get_emitter(blood_burst->emitters, 0)->hidden = 0;
    random_hit(11);
    snd_req(0x1AEF);

    for (i = 0; i < 4; i++) {
        if (plyr_pdata->plyr_num == 0) {
            plyr_obj->position.x = -0.65f;
        } else {
            plyr_obj->position.x = 0.7f;
        }
        if (screen_width > 650) {
            if (plyr_pdata->plyr_num == 0) {
                plyr_obj->position.x = -1.45f;
            } else {
                plyr_obj->position.x = 1.4f;
            }
        }
        ani_loop_more_frames(1.0f);
    }

    obj_set_bone_collapse_flag(plyr_obj, 0x10);
    random_hit(11);
    snd_req(0x1AC6);

    neck_blood = fx_by_owner("neck_blood", 4);
    fx_reset(neck_blood);
    fx_resume_emit(neck_blood);
    neck_blood = find_pfx_by_name("neck_blood");
    restart_effect_ppfx(neck_blood);
    pfx_bind_emitter_to_obj_bone(neck_blood, plyr_obj, 13);
    pfx_get_emitter(neck_blood->emitters, 0)->hidden = 0;

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

    neck_effect = fx_by_owner("neck_blood", 4);
    fx_pause_emit(neck_effect);
    _mkproc_sleep_ticks = 40.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    random_hit(11);
    fx_resume_emit(neck_effect);
    fx_pause_emit(mouth_chunks);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    snd_req(0x1AEF);
    random_hit(11);
    fx_pause_emit(neck_effect);
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    random_hit(11);
    fx_resume_emit(neck_effect);
    fx_pause_emit(mouth_blood);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    fx_pause_emit(neck_effect);

    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
}

/* TODO: [breakthrough] 94.19%; bite lookup order recovered; base/call scheduling and normal-return
 * joins differ. */
static float p_snake_controller(void) {
    unsigned short event;

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
    } else {
        if (g_pz_fighter_fatality_engine.controller->phase != 1) {
            return 1.0f;
        }
        event = randu0(1000);
        if (event < 1) {
            pz_snake_lunge(0);
        } else if (event < 2) {
            pz_snake_lunge(1);
        } else if (event < 4) {
            pz_snake_bite(0);
        } else if (event < 6) {
            pz_snake_bite(1);
        } else {
            return 1.0f;
        }
    }
    return 1.0f;
}

static float pz_fighter_snake_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static inline void pz_snake_start_victim_anim(void) {
  unsigned int animation_flags;

  animation_flags = 0x23;
  if ((unsigned int)g_pz_fighters_engine.fatality_attacker != 0) {
    animation_flags |= 8;
  }
  transition_to_anim_script_frame(
      0.05f, 0.0f,
      g_pz_fighter_fatality_engine.controller
          ->fighter_pdata[g_pz_fighters_engine.fatality_victim],
      pz_shared_ani.snake_victim, animation_flags);
  set_pdata_anim_step(
      g_pz_fighter_fatality_engine.controller
          ->fighter_pdata[g_pz_fighters_engine.fatality_victim],
      1.0f);
}

/* TODO: [near miss] 97.97%; entry global-base order agrees; transition_to_anim_script_frame argument registers and load schedule remain. */
static float pz_fighters_snake_fatality_prep(void) {
    static int attack_begun;
    static int loser_anim;
    struct PuzzleFighterRenderObject *attacker_object;
    struct PuzzleFighterMove *victim_move;
    PlyrPdata *victim_data;
    PlyrPdata *attacker_data;
    float target_z;
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
    int attacker;
    float attacker_x;

    victim_data = g_game_info.plyr0.slot.pdata;
    attacker_data = g_game_info.plyr1.slot.pdata;
    attacker_object = pz_fighter_get_player_obj(
            attacker = g_pz_fighters_engine.fatality_attacker);
    direction = 1;
    target_z = 0.0f;
    if (screen_width > 650) {
        target_x = attacker == 0 ? -1.4f : 1.4f;
    } else {
        target_x = attacker == 0 ? -0.6f : 0.6f;
    }

    attacker_x = attacker_object->position.x;
    delta_x = target_x - attacker_x;
    delta_z = target_z - attacker_object->position.z;
    if (target_x < 0.0f) {
        if (attacker_x < target_x) {
            direction = -1;
        }
    } else if (attacker_x > target_x) {
        direction = -1;
    }
    signed_distance =
            direction * ((delta_x * delta_x) + (delta_z * delta_z));
    player_distance = xz_distance_between_players();

    lower_bound = -0.04f;
    upper_bound = 0.006f;
    if ((unsigned int)g_pz_fighters_engine.fatality_victim == 0) {
        lower_bound = -0.006f;
        upper_bound = 0.04f;
    }
    if ((unsigned int)g_pz_fighters_engine.fatality_victim != 0) {
        victim_data = g_game_info.plyr1.slot.pdata;
        attacker_data = g_game_info.plyr0.slot.pdata;
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
            pz_snake_start_victim_anim();
        }
        loser_anim = 0;
        animation_flags = 0x23;
        if ((unsigned int)g_pz_fighters_engine.fatality_attacker == 0) {
            animation_flags |= 8;
        }
        transition_to_anim_script_frame(
                0.05f, 0.0f,
                g_pz_fighter_fatality_engine.controller
                        ->fighter_pdata[g_pz_fighters_engine.fatality_attacker],
                pz_shared_ani.snake_attacker, animation_flags);
        set_pdata_anim_step(
                g_pz_fighter_fatality_engine.controller
                        ->fighter_pdata[g_pz_fighters_engine.fatality_attacker],
                1.0f);
        g_pz_fighters_engine.snake_active = 1;
        xfer_proc(pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_attacker),
                            r_pz_fighter_eaten);
        if (victim_data->state == 0 && player_distance < 1.0f) {
            xfer_proc(pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
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
            pz_snake_start_victim_anim();
            loser_anim = 1;
        }

        if (attacker_data->state == 0 &&
                signed_distance < lower_bound) {
            xfer_proc(pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_attacker),
                                pz_fighter_walk_forward);
        } else if (victim_data->state == 0 &&
                              signed_distance > upper_bound) {
            victim = g_pz_fighters_engine.fatality_victim;
            victim_move = pz_get_fighter_move();
            victim_move->active = 1;
            g_pz_fighters_engine.fighter_reaction_cooldown = 0;
            xfer_proc(pz_fighter_get_player_proc(victim),
                                pz_fighter_fatality_victim_to_exact_spot);
        }

        if (signed_distance < lower_bound && victim_data->state == 0 &&
                player_distance < 1.0f) {
            xfer_proc(pz_fighter_get_player_proc(g_pz_fighters_engine.fatality_victim),
                                pz_fighter_just_backflip);
        }
    }
    return 0.0f;
}

static float pz_fighters_snake_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

static float pz_fighters_snake_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

static float pz_fighter_load_and_place_initial_snake(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* snakes[2];
    struct PuzzleAnimPdata* snake_pdata[2];
    unsigned int i;
    struct PuzzleFatalityController* controller;
    struct PuzzleParticleEffect* particle_effect;
    void* effect;
    struct PuzzleFighterRenderObject* secondary_snake;

    load_art_section(0x70036, &sec_pz_danger_snake);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_snake_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 5;

    for (i = 0; i < 2; i++) {
        snakes[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08230000, 0x6021);
        obj_change_to_skinned_obj_light_list(
            snakes[i], &skinned_obj_light_def);
        snakes[i]->model_flags = 0x400;
        snakes[i]->position.x = i != 0 ? 2.18f : -2.18f;
        if (screen_width > 650) {
            snakes[i]->position.x = i != 0 ? 2.65f : -2.65f;
        }
        snakes[i]->position.y = 0.3f;
        snakes[i]->position.z = -0.28f;
        snakes[i]->flags_bits.angular_velocity_enabled = 1;
        snakes[i]->facing_angle =
            i != 0 ? -1.0207963f : 1.0207963f;
        snakes[i]->flags_bits.scale_active = 1;
        snakes[i]->scale.x = 0.48f;
        snakes[i]->scale.y = 0.48f;
        snakes[i]->scale.z = 0.48f;
        insert_fgnd_mkobj(snakes[i]);
        snake_pdata[i] = animate_obj(
            snakes[i], pz_shared_ani.snake_idle, pz_snake_bones,
            0, 0, 1.0f, 1);
    }

    obj_add_to_skinned_obj_light_list_with_ambient(
        snakes[0], &skinned_obj_ambient_light_def);
    secondary_snake = snakes[1];
    g_pz_fighter_fatality_engine.scene_objects[1] = secondary_snake;
    g_pz_fighter_fatality_engine.scene_objects[0] = snakes[0];
    obj_create_sobjs(snakes[0]);
    obj_create_sobjs(secondary_snake);
    sobj_set_priority(obj_first_sobj(snakes[0]), 0x12);
    sobj_set_priority(obj_first_sobj(snakes[1]), 0x12);

    controller = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_snake_controller,
            sizeof(struct PuzzleFatalityController), &controller) != 0 &&
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
            g_pz_fighter_fatality_engine.scene_objects[0]->motion_rate;
        g_pz_fighter_fatality_engine.controller->grinder_position[1] =
            g_pz_fighter_fatality_engine.scene_objects[1]->motion_rate;
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
    SETUP_SNAKE_EFFECT("saliva2", secondary_snake, 0);
    SETUP_SNAKE_EFFECT("saliva_burst1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("saliva_burst2", snakes[1], 1);
    SETUP_SNAKE_EFFECT("bloody_mouth_dripping1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("bloody_mouth_dripping2", snakes[1], 1);
    SETUP_SNAKE_EFFECT("chunky_mouth_dripping1", snakes[0], 1);
    SETUP_SNAKE_EFFECT("chunky_mouth_dripping2", snakes[1], 1);

    return 0.0f;
}

static float pz_fighter_snake_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

static float pz_fighter_snake_unload(void) {
    struct PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

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

#pragma opt_propagation off
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
#pragma opt_propagation reset

static float pz_fighter_burn_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

static float r_pz_fighter_burn(void) {
    int animation_flags = 0;

    if (plyr_pdata->plyr_num != 0) {
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

    if (plyr_pdata->plyr_num == 1) {
        animation_flags |= 8;
    }
    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->secondary_flags_bits.field_bit1 = 0;
    blend_to_ani(
        pz_shared_ani.burn_attacker_start, animation_flags, 0.035f);
    set_ani_speed(0.5f);
    ani_to_frame_x(91.0f);

    animation_flags = 0;
    plyr_obj->secondary_flags_bits.field_bit1 = 0;
    if (plyr_pdata->plyr_num == 1) {
        animation_flags |= 8;
    }
    blend_to_ani(pz_shared_ani.burn_attacker_end, animation_flags, 0.5f);
    set_ani_speed(0.5f);
    ani_loop_more_frames(1000.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* TODO: [breakthrough] 98.52%; effect-block attacker reload restored; distance FPR roles and zero-constant copy remain. */
static float pz_fighters_burn_fatality_prep(void) {
    static int start_burn;
    static int attack_begun;
    struct PuzzleFighterRenderObject* attacker_object;
    struct PuzzleFighterMove* victim_move;
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
    direction = 1;
    target_x = screen_width > 650 ?
        (attacker == 0 ? -2.3f : 2.3f) :
        (attacker == 0 ? -1.8f : 1.8f);

    delta_x = target_x - attacker_object->position.x;
    delta_z = 0.0f;
    delta_z -= attacker_object->position.z;
    if (target_x < 0.0f) {
        if (attacker_object->position.x < target_x) {
            direction = -1;
        }
    } else if (attacker_object->position.x > target_x) {
        direction = -1;
    }
    signed_distance =
        direction * ((delta_x * delta_x) + (delta_z * delta_z));

    if (start_burn == 0) {
        start_burn = 1;
        attacker = g_pz_fighters_engine.fatality_attacker;
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
            effect_x = 0.2f + effect_x;
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
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_attacker),
                  r_pz_fighter_burn);
        xfer_proc(pz_fighter_get_player_proc(
                      g_pz_fighters_engine.fatality_victim),
                  r_pz_fighter_summon_burn);
        g_pz_fighters_engine.fatality_timer = 138;
    } else if (g_game_info.plyr0.slot.pdata->state == 0 &&
               g_game_info.plyr1.slot.pdata->state == 0) {
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

static float pz_fighters_burn_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

static float pz_fighters_burn_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/* TODO: [near miss] 99.91%; only r29/r30 coloring of burners[1] after the loop remains. */
static float pz_fighter_load_and_place_initial_burn(void) {
    struct PuzzleEffectBankContext effect_context;
    struct PuzzleFighterRenderObject* burners[2];
    struct PuzzleFatalityController* controller;
    float effect_x;
    unsigned int i;

    load_art_section(0x70036, &sec_pz_danger_burn);
    effect_context.art_handle = 0x70036;
    effect_context.owner = 0;
    effect_context.context = 0;
    load_effect_bank_with_context("pz_burn_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 6;

    for (i = 0; i < 2; i++) {
        burners[i] = (struct PuzzleFighterRenderObject*)load_model_from_slot(
            0x70036, 0x08240000, 0x6021);
        burners[i]->model_flags = 1;
        burners[i]->position.x = i != 0 ? 2.25f : -2.25f;
        if (screen_width > 650) {
            burners[i]->position.x = i != 0 ? 2.65f : -2.65f;
        }
        burners[i]->position.y = 0.0f;
        burners[i]->position.z = 0.0f;
        burners[i]->flags_bits.scale_active = 1;
        burners[i]->scale.x = 0.3f;
        burners[i]->scale.y = 0.3f;
        burners[i]->scale.z = 0.3f;
        insert_fgnd_mkobj(burners[i]);
    }

    g_pz_fighter_fatality_engine.scene_objects[0] = burners[0];
    g_pz_fighter_fatality_engine.scene_objects[1] = burners[1];
    obj_create_sobjs(burners[0]);
    obj_create_sobjs(burners[1]);

    controller = 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_burn_controller,
            sizeof(struct PuzzleFatalityController), &controller) != 0 &&
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
    if (screen_width > 650) {
        effect_x = -2.3f;
    } else {
        effect_x = -1.8f;
    }
    effect_x -= 0.15f;
    bgnd_launch_fx_at_position("idle_flames1", effect_x, 0.0f, 0.0f);
    if (screen_width > 650) {
        effect_x = 2.3f;
    } else {
        effect_x = 1.8f;
    }
    effect_x = 0.2f + effect_x;
    bgnd_launch_fx_at_position("idle_flames2", effect_x, 0.0f, 0.0f);
    pan_snd_req(0x1AB0, -0.7f);
    pan_snd_req(0x1AB0, 0.7f);
    return 0.0f;
}

static float pz_fighter_burn_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

static float pz_fighter_burn_unload(void) {
    struct PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
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

void pz_fighters_fatality_start(int attacker, int victim) {
    PuzzleFatalityStartFn functions[10] = {
        pz_fighter_grinder_entering_fatality,
        pz_fighter_chomper_entering_fatality,
        pz_fighter_chomper2_entering_fatality,
        pz_fighter_objects_falling_entering_fatality,
        pz_fighter_lightning_entering_fatality,
        pz_fighter_snake_entering_fatality,
        pz_fighter_burn_entering_fatality,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index](attacker, victim);
}

void pz_fighters_fatality_unload(void) {
    PuzzleFatalityProcessFn functions[10] = {
        pz_fighter_grinder_unload,
        pz_fighter_chomper_unload,
        pz_fighter_chomper2_unload,
        pz_fighter_objects_falling_unload,
        pz_fighter_lightning_unload,
        pz_fighter_snake_unload,
        pz_fighter_burn_unload,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_normal_fighting(int enabled) {
    PuzzleFatalityActiveFn functions[10] = {
        pz_fighter_grinder_actively_fighting,
        pz_fighter_chomper_actively_fighting,
        pz_fighter_chomper2_actively_fighting,
        pz_fighter_objects_falling_actively_fighting,
        pz_fighter_lightning_actively_fighting,
        pz_fighter_snake_actively_fighting,
        pz_fighter_burn_actively_fighting,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index](enabled);
}

void pz_fighters_fatality_round_over(void) {
    PuzzleFatalityProcessFn functions[10] = {
        pz_fighter_grinder_round_over,
        pz_fighter_chomper_round_over,
        pz_fighter_chomper2_round_over,
        pz_fighter_objects_falling_round_over,
        pz_fighter_lightning_round_over,
        pz_fighter_snake_round_over,
        pz_fighter_burn_round_over,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index]();
}

void pz_fighter_load_place_fatality_elements(int fatality) {
    g_pz_fighter_fatality_engine.scene_objects[0] = 0;
    g_pz_fighter_fatality_engine.scene_objects[1] = 0;
    g_fatalityTable.entries[fatality].load_and_place();
}

void pz_fighters_fatality_in_progress(void) {
    PuzzleFatalityProcessFn functions[10] = {
        pz_fighters_grinder_fatality_in_progress,
        pz_fighters_chomper_fatality_in_progress,
        pz_fighters_chomper2_fatality_in_progress,
        pz_fighters_objects_falling_fatality_in_progress,
        pz_fighters_lightning_fatality_in_progress,
        pz_fighters_snake_fatality_in_progress,
        pz_fighters_burn_fatality_in_progress,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_prep_chores(void) {
    PuzzleFatalityProcessFn functions[10] = {
        pz_fighters_grinder_fatality_prep,
        pz_fighters_chomper_fatality_prep,
        pz_fighters_chomper2_fatality_prep,
        pz_fighters_objects_falling_fatality_prep,
        pz_fighters_lightning_fatality_prep,
        pz_fighters_snake_fatality_prep,
        pz_fighters_burn_fatality_prep,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_preround_event(void) {
    PuzzleFatalityPreroundFn functions[10] = {
        pz_fighters_grinder_fatality_preround,
        pz_fighters_chomper_preround,
        pz_fighters_chomper2_preround,
        pz_fighters_objects_falling_preround,
        pz_fighters_lightning_preround,
        pz_fighters_snake_fatality_preround,
        pz_fighters_burn_fatality_preround,
        0, 0, 0,
    };

    functions[g_pz_fighters_engine.fatality_index]();
}

int pz_fighter_fatality_during_round_stuff_over(void) {
    return 1;
}

int pz_fighter_check_fatality_random_event(void) {
    struct PuzzleFightersEngine* engine = &g_pz_fighters_engine;
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
    switch (g_pz_fighters_engine.fatality_index) {
    case 0:
        if (engine->random_event_cooldown == 0) {
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
        break;
    }
    return 0;
}

#undef SETUP_SNAKE_EFFECT
