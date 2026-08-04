#include "math/gxVect.h"
#include "math/mk_math.h"
#include "game/pz_fatality.h"

typedef void (*PuzzleFatalityFn)();
typedef struct PuzzleFatalityFnPair {
    PuzzleFatalityFn first;
    PuzzleFatalityFn second;
} PuzzleFatalityFnPair;
typedef struct PuzzleFatalityFnTable {
    union {
        PuzzleFatalityFn entries[10];
        PuzzleFatalityFnPair pairs[5];
    };
} PuzzleFatalityFnTable;
typedef struct PuzzleFatalityProcessPair {
    PuzzleFatalityProcessFn first;
    PuzzleFatalityProcessFn second;
} PuzzleFatalityProcessPair;
typedef struct PuzzleFatalityProcessTable {
    union {
        PuzzleFatalityProcessFn entries[10];
        PuzzleFatalityProcessPair pairs[5];
    };
} PuzzleFatalityProcessTable;
typedef struct PuzzleAttackWordPair {
    unsigned int first;
    unsigned int second;
} PuzzleAttackWordPair;
typedef union PuzzleAttackCopy {
    PuzzleAttackParameters attack;
    struct {
        PuzzleAttackWordPair pairs[7];
        unsigned int tail;
    } words;
} PuzzleAttackCopy;

static inline void copy_fatality_fn_table(
    PuzzleFatalityFnTable* destination,
    const PuzzleFatalityFnTable* source) {
    PuzzleFatalityFnPair* output = destination->pairs;
    const PuzzleFatalityFnPair* input = source->pairs;
    int count = 5;

    do {
        *output = *input;
        output++;
        input++;
        count--;
    } while (count != 0);
}

static inline void copy_fatality_process_table(
    PuzzleFatalityProcessTable* destination,
    const PuzzleFatalityProcessTable* source) {
    PuzzleFatalityProcessPair* output = destination->pairs;
    const PuzzleFatalityProcessPair* input = source->pairs;
    int count = 5;

    do {
        *output = *input;
        output++;
        input++;
        count--;
    } while (count != 0);
}

static inline void copy_attack_parameters(
    PuzzleAttackCopy* destination,
    const PuzzleAttackCopy* source) {
    PuzzleAttackWordPair* output = destination->words.pairs;
    const PuzzleAttackWordPair* input = source->words.pairs;
    int count = 7;

    do {
        *output = *input;
        output++;
        input++;
        count--;
    } while (count != 0);
    destination->words.tail = source->words.tail;
}
typedef struct AniScript AniScript;
typedef struct PuzzleAnimPdata PuzzleAnimPdata;
typedef struct MkFileInfo MkFileInfo;
typedef struct PuzzleParticleEffect PuzzleParticleEffect;
typedef struct PuzzleFighterRenderObject PuzzleFighterRenderObject;
typedef struct RpMaterial RpMaterial;
typedef struct RwTexture RwTexture;

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
    int fatality_timer; /* +0x178 */
    float fatality_motion; /* +0x17C */
    char pad180[0x64];
    int snake_active; /* +0x1E4 */
} PuzzleFightersEngine;

struct PuzzleFighterRenderObject {
    char pad00[4];
    unsigned int instance; /* +0x04 */
    unsigned char flags; /* +0x08 */
    unsigned char secondary_flags; /* +0x09 */
    char pad0A[0x22];
    int model_flags; /* +0x2C */
    float hazard_x; /* +0x30 */
    float hazard_y; /* +0x34 */
    float hazard_z; /* +0x38 */
    char pad3C[0x64];
    float x; /* +0xA0 */
    float y; /* +0xA4 */
    float z; /* +0xA8 */
    char padAC[4];
    float external_force_x; /* +0xB0 */
    float external_force_y; /* +0xB4 */
    float external_force_z; /* +0xB8 */
    char padBC[0x18];
    float facing_angle; /* +0xD4 */
    char padD8[0x0C];
    float motion_rate; /* +0xE4 */
    char padE8[8];
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
    unsigned char flags; /* +0x08 */
    char pad09[0x23];
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
    PuzzleFatalityHazardObject* objects[3]; /* +0x00 */
    char pad0C[0x0C];
} PuzzleFatalityHazardGroup; /* 0x18 */

typedef struct PuzzleFatalityEngine {
    PuzzleFighterRenderObject* primary_object;
    PuzzleFighterRenderObject* secondary_object;
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
    int preround_timer; /* +0x2C */
    int loop_sound; /* +0x30 */
    float phase_time; /* +0x34 */
    int preround_sound_started; /* +0x38 */
    float grinder_position[2]; /* +0x3C */
    float grinder_target[2]; /* +0x44 */
    float chomper_position[2][2]; /* +0x4C */
    float hazard_motion[2][2]; /* +0x5C */
    int hazard_initialized[3]; /* +0x6C */
    PuzzleAnimPdata* fighter_pdata[2]; /* +0x78 */
} PuzzleFatalityController;

typedef struct PuzzleGrinderMeatController {
    char pad00[8];
    int direction; /* +0x08 */
    int delay; /* +0x0C */
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
    unsigned char flags; /* +0x1C */
} PuzzleParticleEmitter;

struct PuzzleParticleEffect {
    char pad00[0x40];
    unsigned char emitters[1]; /* +0x40 */
};

typedef struct PuzzleFatalityDefinition {
    int flags;
    PuzzleFatalityFn load_and_place;
    int threshold;
} PuzzleFatalityDefinition;

extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleFatalityDefinition g_fatalityTable[];
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
extern unsigned char skinned_obj_light_def[];
extern unsigned char skinned_obj_ambient_light_def[];
extern void* apdata;

PuzzleFatalityEngine g_pz_fighter_fatality_engine;
PuzzleFleshchunkPdata* pdata_fleshchunk;
PuzzleFighterRenderObject* fleshchunk_obj;
PuzzlePlayerData* plyr_pdata;
extern PuzzlePlayerData* his_pdata;
PuzzleFighterRenderObject* plyr_obj;

void* memset(void* destination, int value, unsigned long size);
void* memcpy(void* destination, const void* source, unsigned long size);
unsigned int randu0(unsigned int max);
int random_snd_req(int sound);
int pz_fighter_close_enough_to_super_move(int player);
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
void get_camera_position(Vec* position);
void pz_fighter_clear_out_external_forces(void);
void random_voice(int group);
void* bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect, PuzzleFighterRenderObject* object, int bone);
void resume_effect_at_obj_bid(
    PuzzleFighterRenderObject* object, int bone, void* effect, int active,
    int flags);
void pz_fighter_walk_FB_true(
    int (*continue_test)(void), unsigned int duration, int forward);
void unhide_obj(PuzzleFighterRenderObject* object);
RpMaterial* sobj_find_material_with_texture(
    PuzzleFatalityHazardObject* object, const char* texture);
void RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);

float sfrand(float range);
float frand(float range);
void spawn_bld_fall(
    const char* effect, int flags, float* position, Vec* velocity,
    PuzzlePlayerData* player_data);
void ft_create_flesh_path(
    PuzzleFighterRenderObject* object, float* position, int active, int flags,
    const Vec* initial_velocity, int mode, const Vec* terminal_velocity,
    int (*completion_callback)(void), float gravity, float bounce,
    float scale);
PuzzleFaceBleedProcess* _create_mkproc_generic_bigstack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    PuzzleFaceBleedPdata** process_data);
static float p_face_bleeding(void);
static void pw_face_bleeding(void);
static void ps_face_bleeding(void);
int dropped_heart_snd_cb(void);
float pz_fighter_completely_prone(void);
float p_anim_idle(void);
float p_plyr_pz_fighter_entry(void);
float r_pz_fighter_grinding(void);
float r_pz_fighter_burn(void);
float r_pz_fighter_summon_burn(void);
float pz_fighter_disgusted_with_grinding(void);
float pz_fighter_fatality_good_solid_kick(void);
float pz_fighter_fatality_medium_shove(void);
float pz_fighter_fatality_huge_shove(void);
float pz_fighter_dash_back(void);
float pz_fighter_shove(void);
float pz_fighter_execute_point_no_space_check(void);
float pz_fighter_execute_point_reaction_no_space(void);
float pz_fighter_execute_R_coming_down(void);
float pz_fighter_backflip_and_point(void);
float pz_fighter_execute_point(void);
float pz_fighter_exit(void);
float pz_fighter_perform_taunt(void);
float r_pz_fighter_eaten(void);
float pz_fighter_just_backflip(void);
float pz_fighter_walk_forward(void);
float pz_fighter_fatality_victim_to_exact_spot(void);
float pz_fighter_lightning_strike_victim_1(void);
float pz_fighter_objects_falling_victim_crushed(void);
float pz_fighter_chomper2_victim_crushed(void);
float pz_fighter_chomper_victim_crushed(void);
float pz_fighter_victim_head_poked(void);
float pz_fighter_victim_fatality_bouncy(void);
float pz_fighter_one_arm_victory2(void);
float pz_fighter_wipe_blood_off(void);
float pz_fighter_won2(void);
void pz_fighter_fatality_launch_eyes(void);
void pz_fighter_shake_camera(int duration, float strength);
void unhide_sobj(PuzzleFatalityHazardObject* object);
void transition_to_anim_script_frame(
    PuzzleAnimPdata* pdata, AniScript* animation, float frame, float blend);
void set_pdata_anim_step(PuzzleAnimPdata* pdata, float step);
void pz_fighter_clear_out_all_external_forces(void* fighter);
float p_grinder_meat_throw_controller(void);
float p_grinder_noise(void);
void* _create_mkproc_generic_tinystack(
    int pid, int priority, PuzzleFatalityProcessFn entry, int pdata_size,
    void* pdata);
void load_art_section(int handle, MkFileInfo* section);
void load_effect_bank_with_context(
    const char* bank, PuzzleEffectBankContext* context);
PuzzleFighterRenderObject* load_model_from_slot(
    int handle, unsigned int art_object, int player);
PuzzleFighterRenderObject* load_named_model_from_slot(
    int handle, const char* name, int flags, int player);
void* replace_sobj_texture_with_named_wiff(
    PuzzleFatalityHazardObject* object, int handle, const char* texture,
    const char* wiff);
void set_ani_texture_framerate(void* texture, float rate);
void set_ani_texture_frame(void* texture, int frame);
void fx_set_param_v3(
    void* effect, int parameter, float x, float y, float z);
void freeze_player(void);
void unfreeze_player(void);
void got_hit_fx(
    int strength, int bone, int blood, int unused1, int unused2, int active,
    float scale);
void hide_obj(PuzzleFighterRenderObject* object);
void insert_fgnd_mkobj(PuzzleFighterRenderObject* object);
void obj_set_pos(PuzzleFighterRenderObject* object, const Vec* position);
void update_mkobj(PuzzleFighterRenderObject* object);
void pz_fighter_set_objects_falling_obj(
    PuzzleFighterRenderObject* object1,
    PuzzleFighterRenderObject* object2);
void load_pz_fighter_fatality_bank(int bank);
float p_lightning_controller(void);
float p_objects_falling_controller2(void);
float p_grinder_controller(void);
float p_burn_controller(void);
float p_chomper2_controller(void);
float p_chomper_controller(void);
float p_snake_controller(void);
void obj_create_sobjs(PuzzleFighterRenderObject* object);
void* load_tga(int handle, unsigned int art_object);
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

void pz_fighter_grinder_entering_fatality(
    int attacker, int victim);
void pz_fighter_chomper_entering_fatality(
    int attacker, int victim);
void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim);
void pz_fighter_objects_falling_entering_fatality(
    int attacker, int victim);
/* Broad pass: pz_fighter_lightning_entering_fatality ~70.81%. */
void pz_fighter_lightning_entering_fatality(
    int attacker, int victim);
void pz_fighter_snake_entering_fatality(
    int attacker, int victim);
void pz_fighter_burn_entering_fatality(
    int attacker, int victim);

static float pz_fighter_grinder_unload(void);
static float pz_fighter_chomper_unload(void);
static float pz_fighter_chomper2_unload(void);
static float pz_fighter_objects_falling_unload(void);
static float pz_fighter_lightning_unload(void);
static float pz_fighter_snake_unload(void);
static float pz_fighter_burn_unload(void);

float pz_fighter_grinder_actively_fighting(int active);
float pz_fighter_chomper_actively_fighting(int active);
float pz_fighter_chomper2_actively_fighting(int active);
float pz_fighter_objects_falling_actively_fighting(int active);
float pz_fighter_lightning_actively_fighting(int active);
float pz_fighter_snake_actively_fighting(int active);
float pz_fighter_burn_actively_fighting(int active);

float pz_fighter_grinder_round_over(void);
float pz_fighter_chomper_round_over(void);
float pz_fighter_chomper2_round_over(void);
float pz_fighter_objects_falling_round_over(void);
float pz_fighter_lightning_round_over(void);
float pz_fighter_snake_round_over(void);
float pz_fighter_burn_round_over(void);

float pz_fighters_grinder_fatality_in_progress(void);
float pz_fighters_chomper_fatality_in_progress(void);
float pz_fighters_chomper2_fatality_in_progress(void);
float pz_fighters_objects_falling_fatality_in_progress(void);
float pz_fighters_lightning_fatality_in_progress(void);
float pz_fighters_snake_fatality_in_progress(void);
float pz_fighters_burn_fatality_in_progress(void);

float pz_fighters_grinder_fatality_prep(void);
float pz_fighters_chomper_fatality_prep(void);
float pz_fighters_chomper2_fatality_prep(void);
float pz_fighters_objects_falling_fatality_prep(void);
float pz_fighters_lightning_fatality_prep(void);
float pz_fighters_snake_fatality_prep(void);
float pz_fighters_burn_fatality_prep(void);

void pz_fighters_grinder_fatality_preround(void);
float pz_fighters_chomper_preround(void);
float pz_fighters_chomper2_preround(void);
float pz_fighters_objects_falling_preround(void);
float pz_fighters_lightning_preround(void);
float pz_fighters_snake_fatality_preround(void);
float pz_fighters_burn_fatality_preround(void);

int pz_fighter_check_fatality_random_event(void) {
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
    if (engine->fatality_index != 0) {
        return 0;
    }
    if (engine->random_event_cooldown != 0) {
        return 0;
    }

    if (engine->balance < -0.65f) {
        event->active = 1;
        event->side = 0;
        pz_fighter_process_random_fatality_event(
            event, pz_fighter_attempt_push_into_grinder);
        engine->random_event_cooldown =
            (randu0(120) & 0xFFFF) + 180;
        return 1;
    }
    if (engine->balance > 0.65f) {
        event->active = 1;
        event->side = 1;
        pz_fighter_process_random_fatality_event(
            event, pz_fighter_attempt_push_into_grinder);
        engine->random_event_cooldown =
            (randu0(120) & 0xFFFF) + 180;
        return 1;
    }
    return 0;
}

int pz_fighter_fatality_during_round_stuff_over(void) {
    return 1;
}

void pz_fighters_fatality_preround_event(void) {
    static const PuzzleFatalityFnTable source = {
        pz_fighters_grinder_fatality_preround,
        (PuzzleFatalityFn)pz_fighters_chomper_preround,
        (PuzzleFatalityFn)pz_fighters_chomper2_preround,
        (PuzzleFatalityFn)pz_fighters_objects_falling_preround,
        (PuzzleFatalityFn)pz_fighters_lightning_preround,
        (PuzzleFatalityFn)pz_fighters_snake_fatality_preround,
        (PuzzleFatalityFn)pz_fighters_burn_fatality_preround,
        0,
        0,
        0,
    };
    PuzzleFatalityFnTable functions;

    copy_fatality_fn_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_prep_chores(void) {
    static const PuzzleFatalityProcessTable source = {
        pz_fighters_grinder_fatality_prep,
        pz_fighters_chomper_fatality_prep,
        pz_fighters_chomper2_fatality_prep,
        pz_fighters_objects_falling_fatality_prep,
        pz_fighters_lightning_fatality_prep,
        pz_fighters_snake_fatality_prep,
        pz_fighters_burn_fatality_prep,
        0,
        0,
        0,
    };
    PuzzleFatalityProcessTable functions;

    copy_fatality_process_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_in_progress(void) {
    static const PuzzleFatalityProcessTable source = {
        pz_fighters_grinder_fatality_in_progress,
        pz_fighters_chomper_fatality_in_progress,
        pz_fighters_chomper2_fatality_in_progress,
        pz_fighters_objects_falling_fatality_in_progress,
        pz_fighters_lightning_fatality_in_progress,
        pz_fighters_snake_fatality_in_progress,
        pz_fighters_burn_fatality_in_progress,
        0,
        0,
        0,
    };
    PuzzleFatalityProcessTable functions;

    copy_fatality_process_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

/* Soft ceiling: pz_fighters_burn_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
float pz_fighters_burn_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/* Soft ceiling: pz_fighters_snake_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
float pz_fighters_snake_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/* Soft ceiling: pz_fighters_grinder_fatality_in_progress ~95.58% - remaining MWCC emit details; stop. */
float pz_fighters_grinder_fatality_in_progress(void) {
    if ((float)g_pz_fighters_engine.fatality_timer == 12.0f) {
        g_pz_fighter_fatality_engine.controller->active = 0;
        g_pz_fighter_fatality_engine.controller->substate = 1;
        g_pz_fighter_fatality_engine.controller->phase = 0;
    }
    return 0.0f;
}

/* Broad pass: lightning victim transfer and victory gate, ~83.66%. */
float pz_fighters_lightning_fatality_in_progress(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzlePlayerData* victim_data;

    switch (fatality->active_effect) {
    case 2:
        fatality->active_effect = 3;
        xfer_proc(
            pz_fighter_get_player_proc(fighters->fatality_attacker),
            pz_fighter_lightning_strike_victim_1);
        break;
    case 3:
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

/* Broad pass: falling-object impact and victory flow, ~66.19%. */
float pz_fighters_objects_falling_fatality_in_progress(void) {
    static const Vec bone_offset = {0.0f, 0.6f, 0.0f};
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* falling_object =
        fatality->hazard_groups[0].objects[0];
    PuzzlePlayerData* victim_data;
    Vec impact_position;

    switch (fatality->active_effect) {
    case 2:
        get_bone_world_pos(
            pz_fighter_get_player_obj(fighters->fatality_attacker), 0x10,
            &impact_position);
        impact_position.x += bone_offset.x;
        impact_position.y += bone_offset.y;
        impact_position.z += bone_offset.z;

        if (falling_object->y <= impact_position.y) {
            fatality->active_effect = 3;
            pz_fighter_fatality_launch_eyes();
            snd_req_vol(0x1AC9, 1.0f);
            snd_req(0x1ACA);
            snd_req_delay(0x1ADC, 10);
            snd_req_delay(0x1ADC, 22);
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_objects_falling_victim_crushed);
            pz_fighter_shake_camera(3, 0.03f);
        } else {
            falling_object->motion -= 0.017f;
        }
        break;
    case 3:
        victim_data = pz_get_pdata_by_id(fighters->fatality_victim);
        if (falling_object->y < 0.25f) {
            falling_object->motion = 0.0f;
            falling_object->flags &= (unsigned char)~0x20;
        }
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_victim),
                pz_fighter_won2);
        }
        break;
    }

    return 0.0f;
}

/* Broad pass: two-column crusher impact and blood flow, ~65.78%. */
float pz_fighters_chomper2_fatality_in_progress(void) {
    static int launch_sounds;
    static int launch_more_meat_chunks;
    static const Vec blood_offset = {0.05f, 0.0f, 0.0f};
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* crusher =
        fatality->hazard_groups[fighters->fatality_attacker].objects[0];
    PuzzlePlayerData* attacker_data;
    PuzzleFighterRenderObject* attacker_object;
    float blood_x;
    float blood_z;

    switch (fatality->active_effect) {
    case 2:
        if (crusher->y < 3.4f) {
            crusher->motion = 0.07f;
        } else {
            snd_req(0x1AAC);
            crusher->motion = 0.0f;
            fatality->active_effect = 3;
        }
        break;
    case 3:
        if (crusher->y > 1.6f) {
            crusher->motion = -0.45f;
            launch_sounds = 1;
        } else if (crusher->y > 0.6f) {
            crusher->motion = -0.45f;
            if (launch_sounds == 1) {
                attacker_data =
                    pz_get_pdata_by_id(fighters->fatality_attacker);
                xfer_proc(
                    pz_fighter_get_player_proc(fighters->fatality_attacker),
                    pz_fighter_chomper2_victim_crushed);
                xfer_proc(
                    pz_fighter_get_player_proc(fighters->fatality_victim),
                    pz_fighter_wipe_blood_off);

                if (attacker_data->side == 0) {
                    blood_x = fighters->fighter_posts[1].x + blood_offset.x;
                    blood_z = fighters->fighter_posts[1].z + blood_offset.z;
                } else {
                    blood_x = fighters->fighter_posts[0].x - blood_offset.x;
                    blood_z = fighters->fighter_posts[0].z - blood_offset.z;
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
                pz_get_pdata_by_id(fighters->fatality_attacker);
                launch_more_meat_chunks = 0;
            }
            crusher->motion = -0.14f;
        } else {
            attacker_object =
                pz_fighter_get_player_obj(fighters->fatality_attacker);
            crusher->motion = 0.0f;
            attacker_object->external_force_y = 0.0f;
            snd_req(0x1ADB);
            snd_req_vol(0x1AAE, 1.0f);
            snd_req_delay(0x1AAF, 7);
            plyr_pdata = pz_get_pdata_by_id(fighters->fatality_attacker);
            snd_major_hit_voice();
            fatality->active_effect = 4;
        }
        break;
    }

    return 0.0f;
}

/* Broad pass: full Chomper impact and aftermath state machine, ~67.95%. */
float pz_fighters_chomper_fatality_in_progress(void) {
    static int mode_timer;
    static int launch_sounds;
    static float flesh_path_timer;
    static const Vec head_offset = {0.0f, 0.6f, 0.0f};
    static const Vec blood_velocity = {0.0f, -0.001f, 0.0f};
    static const Vec flesh_velocity = {0.0f, 0.1f, 0.0f};
    static const Vec flesh_terminal_velocity = {0.05f, 0.05f, -0.05f};
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* spike =
        fatality->hazard_groups[fighters->fatality_attacker].objects[0];
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterRenderObject* flesh_object = fighters->fatality_piece;
    PuzzlePlayerData* attacker_data;
    PuzzlePlayerData* victim_data;
    PuzzleFaceBleedPdata* bleed_data;
    PuzzleFaceBleedProcess* bleed_process;
    Vec impact_position;
    Vec velocity;
    RpMaterial* material;

    switch (fatality->active_effect) {
    case 2:
        attacker_object =
            pz_fighter_get_player_obj(fighters->fatality_attacker);
        victim_data = pz_get_pdata_by_id(fighters->fatality_victim);
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_victim),
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
        fatality->active_effect = 3;
        plyr_pdata = pz_get_pdata_by_id(fighters->fatality_attacker);
        plyr_obj = attacker_object;
        snd_death_voice();
        random_hit(5);
        pz_fighter_shake_camera(1, 0.01f);
        plyr_bleed_mouth(plyr_pdata);
        face_bleed_me(3);

        bleed_process = _create_mkproc_generic_bigstack(
            0x2001, 0x1F, p_face_bleeding, sizeof(PuzzleFaceBleedPdata),
            &bleed_data);
        if (bleed_process != 0 && bleed_data != 0) {
            bleed_data->duration = 200;
            bleed_data->interval = 20;
            bleed_data->state = 0;
            bleed_data->object = plyr_obj;
            bleed_data->player_data = plyr_pdata;
            bleed_process->wait_routine = pw_face_bleeding;
            bleed_process->script_routine = ps_face_bleeding;
        }
        xfer_proc(
            pz_fighter_get_player_proc(fighters->fatality_attacker),
            pz_fighter_victim_head_poked);
        break;

    case 3:
        attacker_object =
            pz_fighter_get_player_obj(fighters->fatality_attacker);
        victim_data = pz_get_pdata_by_id(fighters->fatality_victim);
        if (victim_data->transition_locked == 0) {
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_victim),
                pz_fighter_one_arm_victory2);
        }
        get_bone_world_pos(attacker_object, 0x10, &impact_position);

        if (spike->y < 3.95f) {
            spike->motion = 0.12f;
            attacker_object->secondary_flags &= (unsigned char)~0x80;
            attacker_object->external_force_y = 0.09f;
            mode_timer = 0;
        } else if (mode_timer == 0) {
            spike->motion = 0.0f;
            attacker_object->external_force_y = 0.0f;
            mode_timer = 200;
        } else if (mode_timer == 120) {
            attacker_object->flags |= 1;
            attacker_object->hazard_x = -0.002f;
            mode_timer--;
        } else if (mode_timer == 70) {
            attacker_object->flags &= (unsigned char)~1;
            attacker_object->hazard_x = 0.0f;
            mode_timer--;
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_victim_fatality_bouncy);
        } else if (mode_timer == 1) {
            snd_req(0x1AAC);
            fatality->active_effect = 4;
            mode_timer = 0;
        } else {
            mode_timer--;
        }
        break;

    case 4:
        attacker_object =
            pz_fighter_get_player_obj(fighters->fatality_attacker);
        if (spike->y > 1.25f) {
            spike->motion = -0.4f;
            attacker_object->secondary_flags &= (unsigned char)~0x80;
            launch_sounds = 1;
        } else if (spike->y > 0.35f) {
            spike->motion = -0.28f;
            attacker_object->secondary_flags &= (unsigned char)~0x80;
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

            material = sobj_find_material_with_texture(spike, "spikes");
            if (material != 0) {
                RpMaterialSetTexture(material, fatality->grinder_texture);
            }
            plyr_pdata = pz_get_pdata_by_id(fighters->fatality_attacker);
            snd_major_hit_voice();
            fatality->active_effect = 5;

            if (fighters->fatality_attacker == 1) {
                flesh_object->x = 1.56f;
                flesh_object->z = -0.27f;
            } else {
                flesh_object->x = -1.62f;
                flesh_object->z = -0.28f;
            }
            flesh_object->y = 0.2f;
            flesh_object->flags |= 0x20;
            flesh_object->external_force_x = 0.0f;
            flesh_object->external_force_y = 0.0f;
            flesh_object->external_force_z = 0.0f;
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_chomper_victim_crushed);
        }
        break;

    case 5:
        if (spike->y < 3.55f) {
            spike->motion = 0.3f;
            flesh_object->external_force_y = 0.206f;
            unhide_obj(flesh_object);
            if ((randu0(100) & 0xFFFF) < 70) {
                velocity = blood_velocity;
                attacker_data =
                    pz_get_pdata_by_id(fighters->fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &flesh_object->x, &velocity, attacker_data);
            }
        } else if (spike->y < 3.95f) {
            spike->motion = 0.17f;
            flesh_object->external_force_y = 0.138f;
            flesh_path_timer = 5.0f;
            if ((randu0(100) & 0xFFFF) < 70) {
                velocity = blood_velocity;
                attacker_data =
                    pz_get_pdata_by_id(fighters->fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &flesh_object->x, &velocity, attacker_data);
            }
        } else {
            flesh_path_timer -= 1.0f;
            spike->motion = 0.0f;
            flesh_object->external_force_y = 0.0f;
            if (fighters->fatality_motion == 1.0f &&
                flesh_path_timer <= 0.0f) {
                ft_create_flesh_path(
                    flesh_object, &flesh_object->x, 1, 0,
                    &flesh_velocity, 0, &flesh_terminal_velocity,
                    dropped_heart_snd_cb, -0.0015f, 0.7f, 0.1f);
                fatality->active_effect = 6;
            }
            if ((randu0(100) & 0xFFFF) < 40) {
                velocity = blood_velocity;
                attacker_data =
                    pz_get_pdata_by_id(fighters->fatality_attacker);
                spawn_bld_fall(
                    "gusher0", 0, &flesh_object->x, &velocity, attacker_data);
            }
        }
        break;

    case 6:
        if ((randu0(100) & 0xFFFF) < 8) {
            velocity = blood_velocity;
            attacker_data = pz_get_pdata_by_id(fighters->fatality_attacker);
            spawn_bld_fall(
                "gusher0", 0, &flesh_object->x, &velocity, attacker_data);
            velocity.x = sfrand(0.1f);
            velocity.z = sfrand(0.1f);
            spawn_bld_fall(
                "gusher0", 0, &flesh_object->x, &velocity, attacker_data);
        }
        break;
    }

    return 0.0f;
}

float pz_fighter_chomper_victim_crushed(void) {
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

float pz_fighter_victim_head_poked(void) {
    unsigned int i;

    for (i = 0; i < 10; i++) {
        blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
        plyr_anim_pdata->animation_step = 0.0f;
        ani_to_end();
    }
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

int dropped_heart_snd_cb(void) {
    snd_req(0x1ADD);
    return 0;
}

void pz_fighters_fatality_bird_in_place(int elapsed) {
    if (elapsed > 300) {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[1] = 8;
        random_snd_req(0xB1);
    } else {
        g_pz_fighter_fatality_engine.controller->hazard_initialized[0] = 7;
        random_snd_req(0xB1);
    }
}

/* Soft ceiling: pz_fighter_victim_fatality_bouncy ~99.26% - MWCC emit details; stop. */
float pz_fighter_victim_fatality_bouncy(void) {
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

int pz_fighter_always_continue(void) {
    return 0;
}

/* Soft ceiling: pz_fighter_walk_forward ~99.52% - process-transfer coloring; stop. */
float pz_fighter_walk_forward(void) {
    pz_fighter_walk_FB_true(pz_fighter_always_continue, 500, 1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
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

float pz_fighter_fatality_victim_to_exact_spot(void) {
    static const PuzzleAttackCopy source = {{
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 3.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    }};
    PuzzleAttackCopy attack;

    copy_attack_parameters(&attack, &source);

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack.attack, 0x38);
    g_pz_fighters_engine.fatality_ready = 1;
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_fatality_good_solid_kick(void) {
    static const PuzzleAttackCopy source = {{
        11.0f, 0.1f, 0.9f, 0x00010001, 0x00070008, 3,
        0.2f, 0.75f, 1.35f, 14.0f, 12.0f, 1, 1, 1, 0,
    }};
    PuzzleAttackCopy attack;

    copy_attack_parameters(&attack, &source);

    set_my_state(0x1200);
    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.solid_kick, &attack.attack, 0x18);
    force_forward(0.02f, 9, 0.5f, 3);
    g_pz_fighters_engine.fatality_ready = 1;
    slow_ani_x(0.44f, 19.0f);
    ani_to_frame_x(31.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_fatality_huge_shove(void) {
    static const PuzzleAttackCopy source = {{
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    }};
    PuzzleAttackCopy attack;

    copy_attack_parameters(&attack, &source);

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack.attack, 0x17);
    g_pz_fighters_engine.fatality_ready = 1;
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_fatality_medium_shove(void) {
    static const PuzzleAttackCopy source = {{
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    }};
    PuzzleAttackCopy attack;

    copy_attack_parameters(&attack, &source);

    set_my_state(0x120B);
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack.attack, 0x16);
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

/* Soft ceiling: p_face_bleeding ~91.22% - local load/color scheduling. */
static float p_face_bleeding(void) {
    PuzzleFaceBleedPdata* bleed_data = (PuzzleFaceBleedPdata*)apdata;

    bleed_data->state++;
    if (bleed_data->state >= bleed_data->duration) {
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

/* Soft ceiling: r_pz_fighter_summon_burn ~94.35% - MWCC emit details; stop. */
float r_pz_fighter_summon_burn(void) {
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

/* Broad pass: victim ignition, looping burn, and bound limb effects. */
float r_pz_fighter_burn(void) {
    int animation_flags = 0;
    void* effect;

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

    effect = fx_by_owner("limbburn_5", 4);
    resume_effect_at_obj_bid(plyr_obj, 0x10, effect, 1, 0);
    ani_to_blend_frame(50.0f);
    effect = fx_by_owner("limbburn_6", 4);
    resume_effect_at_obj_bid(plyr_obj, 0x10, effect, 1, 0);
    ani_to_end();

    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
}

/* Broad pass: preround flame pair and randomized ambient flame controller. */
float p_burn_controller(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;
    unsigned int event;
    float x;

    if (controller->unload_requested == 1) {
        return -1.0f;
    }

    if (controller->preround_active == 1) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        pan_snd_req(0x1AB6, -0.7f);
        x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
        bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
        pan_snd_req(0x1AB6, 0.7f);
        x = 0.2f + (screen_width > 650 ? 2.3f : 1.8f);
        bgnd_launch_fx_at_position("roar_flames2", x, 0.0f, 0.0f);
        controller->hazard_initialized[0] = 120;
        controller->hazard_initialized[1] = 120;
        _mkproc_sleep_ticks = 10.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        controller->preround_active = 0;
    } else if (controller->phase == 1) {
        event = randu0(1000) & 0xFFFF;
        if (event < 1 && controller->hazard_initialized[0] == 0) {
            pan_snd_req(0x1AB6, -0.7f);
            x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
            bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
            controller->hazard_initialized[0] = 200;
        } else if (event < 2 &&
                   controller->hazard_initialized[1] == 0) {
            pan_snd_req(0x1AB6, 0.7f);
            x = 0.2f + (screen_width > 650 ? 2.3f : 1.8f);
            bgnd_launch_fx_at_position("roar_flames2", x, 0.0f, 0.0f);
            controller->hazard_initialized[1] = 200;
        } else if (event < 2 &&
                   controller->hazard_initialized[0] == 0) {
            pan_snd_req(0x1AB6, -0.7f);
            x = (screen_width > 650 ? -2.3f : -1.8f) - 0.15f;
            bgnd_launch_fx_at_position("roar_flames1", x, 0.0f, 0.0f);
            controller->hazard_initialized[0] = 200;
        }
    }

    if (controller->hazard_initialized[0] != 0) {
        controller->hazard_initialized[0]--;
    }
    if (controller->hazard_initialized[1] != 0) {
        controller->hazard_initialized[1]--;
    }
    return 1.0f;
}

/*
 * Broad pass: crushed pose, staged debris effects, and prone transition.
 * Soft ceiling: pz_fighter_objects_falling_victim_crushed ~94.90% --
 * remaining differences are MWCC expression/register scheduling.
 */
float pz_fighter_objects_falling_victim_crushed(void) {
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

void pz_fighter_fatality_launch_eyes(void) {
    static const Vec launch_offset = {0.0f, 0.1f, 0.02f};
    static const Vec left_offset = {-0.05f, 0.0f, 0.05f};
    static const Vec right_offset = {0.05f, 0.0f, 0.05f};
    PuzzleFighterRenderObject* left_eye;
    PuzzleFighterRenderObject* right_eye;
    PuzzleFighterPhysics* fighter;
    PuzzleBoneData* bone_data;
    MKMATRIX* bone_matrix;
    Vec rotated_offset;
    Vec bone_position;
    Vec eye_position;
    Vec camera_position;
    Vec camera_direction;
    float angle_offset;
    float launch_angle;

    left_eye = g_pz_fighters_engine.left_eye;
    right_eye = g_pz_fighters_engine.right_eye;
    fighter = (PuzzleFighterPhysics*)g_game_info.player1_physics;
    if (g_pz_fighters_engine.fatality_attacker == 1) {
        fighter = (PuzzleFighterPhysics*)g_game_info.player2_physics;
    }

    if ((fighter->flags_0A & 0x40) != 0) {
        angle_offset = 1.5707964f;
    } else {
        angle_offset = -1.5707964f;
    }
    launch_angle = fighter->facing_angle + angle_offset;
    left_eye->facing_angle = launch_angle;
    right_eye->facing_angle = launch_angle;
    rotate_xz(&rotated_offset, &launch_offset, launch_angle);

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

    left_eye->flags |= 0x20;
    get_camera_position(&camera_position);
    v3_sub_v3(
        &camera_direction, &camera_position, (Vec*)&left_eye->x);
    scale_v3(
        (Vec*)&left_eye->external_force_x, &camera_direction, 0.02f);
    left_eye->external_force_y -= 0.0008f;

    right_eye->flags |= 0x20;
    scale_v3(
        (Vec*)&right_eye->external_force_x, &camera_direction, 0.02f);
    right_eye->external_force_y += 0.0013f;

    _mkproc_sleep_ticks = 2.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    unhide_obj(left_eye);
    unhide_obj(right_eye);
}

float pz_fighter_objects_falling_actively_fighting(int active) {
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

/* Broad pass: reveal both three-piece hazards, then orbit their inner debris. */
float p_objects_falling_controller2(void) {
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityController* controller = fatality->controller;
    PuzzleFatalityHazardGroup* group;
    PuzzleFatalityHazardObject* object;
    float x;
    int i;
    int j;

    if (controller->unload_requested == 1) {
        return -1.0f;
    }

    if (controller->preround_active == 1) {
        for (i = 0; i < 2; i++) {
            group = &fatality->hazard_groups[i];
            if (controller->hazard_initialized[i] == 1) {
                x = screen_width > 650 ? 2.1f : 1.7f;
                if (i == 1) {
                    x = -x;
                }
                for (j = 0; j < 3; j++) {
                    group->objects[j]->x = x;
                }
                _mkproc_sleep_ticks = 1.0f;
                aproc->vtbl->sleep(aproc->vtbl);
                for (j = 0; j < 3; j++) {
                    unhide_sobj(group->objects[j]);
                }
                controller->hazard_initialized[i] = 0;
                group->objects[0]->motion = 0.02f;
                group->objects[1]->motion = 0.02f;
            }
            if (controller->hazard_initialized[i] == 0 &&
                group->objects[0]->y > 4.0f) {
                group->objects[0]->motion = 0.0f;
                group->objects[1]->motion = 0.0f;
                controller->hazard_initialized[i] = 2;
            }
        }
    } else if (controller->phase == 1) {
        if (controller->hazard_initialized[2] == 1) {
            controller->hazard_motion[0][0] = -0.004f;
            controller->hazard_motion[1][0] = 0.004f;
            controller->hazard_initialized[2] = 0;
        }

        object = fatality->hazard_groups[0].objects[2];
        if (object->x > 1.7f) {
            controller->hazard_motion[0][0] = -0.004f;
        }
        if (object->x < 0.5f) {
            controller->hazard_motion[0][0] = 0.004f;
        }
        object->x += controller->hazard_motion[0][0];
        if (controller->hazard_motion[0][0] < 0.0f) {
            if (object->x < 1.2f) {
                controller->hazard_motion[0][0] *= 0.98f;
            } else if (object->x > 1.7f) {
                controller->hazard_motion[0][0] *= 1.02f;
            }
            if (controller->hazard_motion[0][0] < -0.03f) {
                controller->hazard_motion[0][0] = -0.03f;
            } else if (controller->hazard_motion[0][0] > -0.004f) {
                controller->hazard_motion[0][0] = -0.004f;
            }
        } else if (controller->hazard_motion[0][0] > 0.0f) {
            if (object->x > 1.7f) {
                controller->hazard_motion[0][0] *= 0.98f;
            } else if (object->x < 1.2f) {
                controller->hazard_motion[0][0] *= 1.02f;
            }
            if (controller->hazard_motion[0][0] > 0.03f) {
                controller->hazard_motion[0][0] = 0.03f;
            } else if (controller->hazard_motion[0][0] < 0.004f) {
                controller->hazard_motion[0][0] = 0.004f;
            }
        }

        object = fatality->hazard_groups[1].objects[2];
        if (object->x < -1.7f) {
            controller->hazard_motion[1][0] = 0.004f;
        }
        if (object->x > -0.5f) {
            controller->hazard_motion[1][0] = -0.004f;
        }
        object->x += controller->hazard_motion[1][0];
        if (controller->hazard_motion[1][0] < 0.0f) {
            if (object->x < -1.7f) {
                controller->hazard_motion[1][0] *= 0.98f;
            } else if (object->x > -1.2f) {
                controller->hazard_motion[1][0] *= 1.02f;
            }
            if (controller->hazard_motion[1][0] < -0.03f) {
                controller->hazard_motion[1][0] = -0.03f;
            } else if (controller->hazard_motion[1][0] > -0.004f) {
                controller->hazard_motion[1][0] = -0.004f;
            }
        } else if (controller->hazard_motion[1][0] > 0.0f) {
            if (object->x > -1.2f) {
                controller->hazard_motion[1][0] *= 0.98f;
            } else if (object->x < -1.7f) {
                controller->hazard_motion[1][0] *= 1.02f;
            }
            if (controller->hazard_motion[1][0] > 0.03f) {
                controller->hazard_motion[1][0] = 0.03f;
            } else if (controller->hazard_motion[1][0] < 0.004f) {
                controller->hazard_motion[1][0] = 0.004f;
            }
        }
    }
    return 1.0f;
}

float pz_fighter_grinder_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

float pz_fighter_burn_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

/* Soft ceiling: 35.50% - clean fixed 2x2 loop is unrolled by this compiler pass. */
float pz_fighter_chomper_actively_fighting(int active) {
    int group;
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

float pz_fighter_chomper2_actively_fighting(int active) {
    int group;
    int count;

    g_pz_fighter_fatality_engine.controller->phase = active;
    if (active == 0) {
        group = 0;
        count = 2;
        do {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[0]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][0] = 0.0f;
            group++;
            count--;
        } while (count != 0);
    }
    return 0.0f;
}

float p_grinder_noise(void) {
    PuzzleGrinderNoisePdata* noise = (PuzzleGrinderNoisePdata*)apdata;

    noise->timer++;
    if (noise->timer >= noise->duration) {
        return -1.0f;
    }
    if ((noise->timer % 20) == 0) {
        snd_req_vol((randu0(3) & 0xFFFF) + 0x1AEB, 1.0f);
    }
    return 1.0f;
}

/*
 * Broad pass: drive the paired grinder speeds toward their phase targets,
 * including the loop-sound fade used during fighting and preround.
 */
float p_grinder_controller(void) {
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityController* controller = fatality->controller;
    PuzzleFighterRenderObject* grinders[2];
    int changed = 0;
    int i;

    grinders[0] = fatality->primary_object;
    grinders[1] = fatality->secondary_object;

    if (controller->unload_requested == 1) {
        return -1.0f;
    }

    if (controller->active == 1) {
        controller->active = 0;
        changed = 1;
        controller->grinder_target[controller->victim_player] = 0.3f;
        controller->hazard_motion[controller->victim_player][0] = 0.002f;
        controller->grinder_target[controller->attacker_player] = 0.2f;
        controller->hazard_motion[controller->attacker_player][0] = 0.001f;
    } else if (controller->substate == 1) {
        controller->substate = 0;
        controller->controller_step = 1;
        changed = 1;
        controller->grinder_target[controller->victim_player] = 0.05f;
        controller->hazard_motion[controller->victim_player][0] = 0.002f;
        controller->grinder_target[controller->attacker_player] = 0.05f;
        controller->hazard_motion[controller->attacker_player][0] = 0.001f;
    }

    if (controller->controller_step == 1) {
        if (controller->phase_time > 0.05f) {
            controller->phase_time -= 0.005f;
            set_snd_vol(
                controller->loop_sound, 0x1AB7, controller->phase_time);
        } else if (controller->loop_sound != 0) {
            snd_stop(controller->loop_sound);
            controller->loop_sound = 0;
        }
    }

    if (controller->phase == 1) {
        if (g_pz_fighters_engine.balance < -0.5f) {
            controller->grinder_target[1] = 0.25f;
            controller->hazard_motion[1][0] = 0.001f;
            changed = 1;
        } else if (g_pz_fighters_engine.balance > -0.1f) {
            controller->grinder_target[1] = 0.18f;
            controller->hazard_motion[1][0] = 0.001f;
            changed = 1;
        }

        if (g_pz_fighters_engine.balance > 0.5f) {
            controller->grinder_target[0] = 0.25f;
            controller->hazard_motion[0][0] = 0.001f;
            changed = 1;
        } else if (g_pz_fighters_engine.balance < 0.1f) {
            controller->grinder_target[0] = 0.18f;
            controller->hazard_motion[0][0] = 0.001f;
            changed = 1;
        }
    }

    if (controller->preround_active == 1 ||
        (controller->phase == 1 && controller->loop_sound != 0)) {
        controller->grinder_target[0] = 0.25f;
        controller->grinder_target[1] = 0.25f;
        controller->hazard_motion[0][0] = 0.002f;
        controller->hazard_motion[1][0] = 0.002f;
        changed = 1;

        if (controller->preround_timer == 0) {
            controller->grinder_target[0] = 0.1f;
            controller->grinder_target[1] = 0.1f;
            controller->hazard_motion[0][0] = 0.0012f;
            controller->hazard_motion[1][0] = 0.0012f;
            changed = 1;
        } else {
            controller->preround_timer--;
        }

        if (controller->preround_timer != 0) {
            if (controller->phase_time < 0.7f) {
                controller->phase_time += 0.01f;
                set_snd_vol(
                    controller->loop_sound, 0x1AB7, controller->phase_time);
            }
        } else if (controller->phase_time > 0.05f) {
            controller->phase_time -= 0.005f;
            set_snd_vol(
                controller->loop_sound, 0x1AB7, controller->phase_time);
        } else if (controller->loop_sound != 0) {
            snd_stop(controller->loop_sound);
            controller->loop_sound = 0;
        }
    }

    if (changed == 1) {
        for (i = 0; i < 2; i++) {
            if (controller->grinder_position[i] >
                    controller->grinder_target[i] &&
                controller->hazard_motion[i][0] > 0.0f) {
                controller->hazard_motion[i][0] *= -1.0f;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if ((controller->grinder_position[i] >
                 controller->grinder_target[i] + 0.001f &&
             controller->hazard_motion[i][0] < 0.0f) ||
            (controller->grinder_position[i] <
                 controller->grinder_target[i] - 0.001f &&
             controller->hazard_motion[i][0] > 0.0f)) {
            grinders[i]->motion_rate += controller->hazard_motion[i][0];
            controller->grinder_position[i] = grinders[i]->motion_rate;
        }
    }
    return 1.0f;
}

float pz_fighter_snake_actively_fighting(int active) {
    if (active == 1) {
        g_pz_fighter_fatality_engine.controller->preround_active = 0;
    }
    g_pz_fighter_fatality_engine.controller->phase = active;
    return 0.0f;
}

/*
 * Broad pass: paired Snake idle/lunge/bite animation control and the
 * saliva/burst emitter timing shared by preround and active fighting.
 * Soft ceiling: p_snake_controller 41.12% -- broad state flow retained.
 */
float p_snake_controller(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;
    PuzzleParticleEffect* saliva;
    PuzzleParticleEffect* burst;
    PuzzleAnimPdata* pdata;
    const char* saliva_name;
    const char* burst_name;
    unsigned int event;
    int snake;

    if (controller->unload_requested == 1) {
        return -1.0f;
    }

    if (controller->preround_active == 1) {
        for (snake = 0; snake < 2; snake++) {
            saliva_name = snake == 0 ? "saliva1" : "saliva2";
            saliva = (PuzzleParticleEffect*)fx_by_owner(saliva_name, 4);
            pan_snd_req(0x1AC3, snake == 0 ? -0.7f : 0.7f);
            fx_pause_emit(saliva);

            pdata = controller->fighter_pdata[snake];
            transition_to_anim_script_frame(
                pdata, pz_shared_ani.snake_lunge, 0.05f, 35.0f);
            set_pdata_anim_step(pdata, 1.0f);
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
                    if (g_pz_fighters_engine.fatality_abort == 0) {
                        transition_to_anim_script_frame(
                            pdata, pz_shared_ani.snake_idle, 0.1f, 0.0f);
                        set_pdata_anim_step(pdata, 1.0f);
                    } else {
                        fx_pause_emit(saliva);
                    }
                }
            }
            if (snake == 0) {
                _mkproc_sleep_ticks = 20.0f;
                aproc->vtbl->sleep(aproc->vtbl);
            }
        }
        _mkproc_sleep_ticks = 175.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        controller->preround_active = 0;
        return 1.0f;
    }

    if (controller->phase != 1) {
        return 1.0f;
    }

    event = randu0(1000) & 0xFFFF;
    if (event < 2) {
        snake = event;
        saliva_name = snake == 0 ? "saliva1" : "saliva2";
        saliva = (PuzzleParticleEffect*)fx_by_owner(saliva_name, 4);
        pan_snd_req(0x1AC3, snake == 0 ? -0.7f : 0.7f);
        fx_pause_emit(saliva);
        pdata = controller->fighter_pdata[snake];
        transition_to_anim_script_frame(
            pdata, pz_shared_ani.snake_lunge, 0.05f, 35.0f);
        set_pdata_anim_step(pdata, 1.0f);
        _mkproc_sleep_ticks = 15.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        fx_resume_emit(saliva);
        if (g_pz_fighters_engine.fatality_abort == 0) {
            _mkproc_sleep_ticks = 15.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            fx_pause_emit(saliva);
            if (g_pz_fighters_engine.fatality_abort == 0) {
                _mkproc_sleep_ticks = 20.0f;
                aproc->vtbl->sleep(aproc->vtbl);
                fx_resume_emit(saliva);
                transition_to_anim_script_frame(
                    pdata, pz_shared_ani.snake_idle, 0.1f, 0.0f);
                set_pdata_anim_step(pdata, 1.0f);
            }
        } else {
            fx_pause_emit(saliva);
        }
    } else if (event < 6) {
        snake = event < 4 ? 0 : 1;
        saliva_name = snake == 0 ? "saliva1" : "saliva2";
        burst_name = snake == 0 ? "saliva_burst1" : "saliva_burst2";
        pan_snd_req(0x1AC2, snake == 0 ? -0.7f : 0.7f);
        saliva = (PuzzleParticleEffect*)fx_by_owner(saliva_name, 4);
        burst = (PuzzleParticleEffect*)fx_by_owner(burst_name, 4);
        fx_pause_emit(saliva);
        pdata = controller->fighter_pdata[snake];
        transition_to_anim_script_frame(
            pdata, pz_shared_ani.snake_bite, 0.1f, 35.0f);
        set_pdata_anim_step(pdata, 1.0f);
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
                    pdata, pz_shared_ani.snake_idle, 0.15f, 0.0f);
                set_pdata_anim_step(pdata, 1.0f);
            }
        }
    }
    return 1.0f;
}

/*
 * Broad pass: Snake fatality victim animation, attached blood effects,
 * bite staging, and the long-running post-fatality bleed state.
 * Soft ceiling: r_pz_fighter_eaten 77.56% -- register scheduling remains.
 */
float r_pz_fighter_eaten(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
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

    saliva = fx_by_owner(
        fighters->fatality_victim == 0 ? "saliva1" : "saliva2", 4);
    fx_resume_emit(saliva);
    ani_loop_more_frames(11.0f);
    ani_loop_more_frames(17.0f);
    xfer_proc(
        pz_fighter_get_player_proc(fighters->fatality_victim),
        pz_fighter_disgusted_with_grinding);
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
    emitter->flags &= (unsigned char)~0x80;
    random_hit(11);
    snd_req(0x1AEF);

    for (i = 0; i < 4; i++) {
        plyr_obj->x = plyr_pdata->side == 0 ? -0.65f : 0.7f;
        if (screen_width > 650) {
            plyr_obj->x = plyr_pdata->side == 0 ? -1.45f : 1.4f;
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
    emitter->flags &= (unsigned char)~0x80;

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
    _mkproc_sleep_ticks = 175.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    snd_req(0x1AEF);
    random_hit(11);
    fx_pause_emit(neck_blood);
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    random_hit(11);
    fx_resume_emit(neck_blood);
    fx_pause_emit(mouth_blood);
    _mkproc_sleep_ticks = 175.0f;
    aproc->vtbl->sleep(aproc->vtbl);
    fx_pause_emit(neck_blood);

    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
}

/*
 * Broad pass: Lightning victim electrocution, head-bolt presentation,
 * attached smoke/blood effects, collapse, and residual shock audio.
 * Soft ceiling: pz_fighter_lightning_strike_victim_1 -- broad CFG pass.
 */
float pz_fighter_lightning_strike_victim_1(void) {
    PuzzleParticleEffect* particle;
    PuzzleParticleEmitter* emitter;
    PuzzleFighterRenderObject* bolt;
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
    int shock_frame = 0;
    int cycle;
    int frame;

    init_ground_move_no_aniproc();
    snd_req(0x1ABB);
    ani_loop_more_frames(5.0f);

    head.x = plyr_obj->x;
    head.y = plyr_obj->y;
    head.z = plyr_obj->z;
    get_bone_world_pos(plyr_obj, 0x10, &head);
    head.y += 0.1f;
    bolt = load_named_model_from_slot(
        0x70036, "BOLT_OBJECT", 0x2099, 0);
    insert_fgnd_mkobj(bolt);
    obj_set_pos(bolt, &head);
    update_mkobj(bolt);
    obj_create_sobjs(bolt);
    set_ani_texture_frame(
        replace_sobj_texture_with_named_wiff(
            obj_first_sobj(bolt), 0x70036,
            "PZ_LIGHTNING_PF_LIGHTNING", "Nightwolf_Bolt"),
        0);

    electrical_sound = snd_req(0x1ABC);
    fx_reset(head_zap);
    fx_resume_emit(head_zap);
    particle = find_pfx_by_name("pz_headzap");
    restart_effect_ppfx(particle);
    pfx_bind_emitter_to_obj_bone(particle, plyr_obj, 0x10);
    emitter = pfx_get_emitter(particle->emitters, 0);
    emitter->flags &= (unsigned char)~0x80;

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
    emitter->flags &= (unsigned char)~0x80;

    pz_fighter_shake_camera(3, 0.03f);
    voice_sound = plyr_snd_req(0x4E);
    random_hit(1);
    plyr_obj->facing_angle = 0.0f;

    for (cycle = 0; cycle < 8; cycle++) {
        glitch_to_ani(pz_shared_ani.head_poked, 3);
        for (frame = 0; frame < 14; frame++) {
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
    emitter->flags &= (unsigned char)~0x80;
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

    for (cycle = 0; cycle < 100; cycle++) {
        _mkproc_sleep_ticks = (float)(randu0(30) & 0xFFFF) + 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        volume = 0.5f + frand(0.5f);
        frame = (randu0(3) & 0xFFFF) + 0x1ABE;
        if ((randu0(100) & 0xFFFF) < 50) {
            snd_req_vol(frame, volume);
        } else if ((randu0(100) & 0xFFFF) < 50) {
            pan_vol_snd_req(frame, -1.0f, volume);
        } else {
            pan_vol_snd_req(frame, 1.0f, volume);
        }
    }

    return aproc->vtbl->transfer(pz_fighter_completely_prone, 0.0f);
}

float pz_fighter_lightning_actively_fighting(int active) {
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
 * Broad pass: Lightning preround pair and sparse during-fight bolt events.
 * Soft ceiling: p_lightning_controller -- duplicated retail bolt lifecycle
 * is represented without register-order refinement.
 */
float p_lightning_controller(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;
    PuzzleFighterRenderObject* bolt;
    PuzzleFatalityHazardObject* bolt_sobj;
    void* spark;
    void* smoke;
    void* burning;
    Vec position = {0.0f, 0.0f, 0.0f};
    int side;
    int pass;

    if (controller->unload_requested == 1) {
        return -1.0f;
    }

    if (controller->preround_active == 1) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        for (pass = 0; pass < 2; pass++) {
            side = pass;
            position.x = side == 0
                ? (screen_width > 650 ? -2.2f : -1.9f)
                : (screen_width > 650 ? 2.2f : 1.9f);
            bolt = load_named_model_from_slot(
                0x70036, "BOLT_OBJECT", 0x2099, 0);
            insert_fgnd_mkobj(bolt);
            obj_set_pos(bolt, &position);
            update_mkobj(bolt);
            obj_create_sobjs(bolt);
            bolt_sobj = obj_first_sobj(bolt);
            set_ani_texture_framerate(
                replace_sobj_texture_with_named_wiff(
                    bolt_sobj, 0x70036, "PZ_LIGHTNING_PF_LIGHTNING",
                    "Nightwolf_Bolt"),
                1.0f);
            pan_snd_req(0x1ABA, side == 0 ? -1.0f : 1.0f);

            smoke = fx_by_owner("pz_lightningsmoke", 4);
            spark = fx_by_owner("pz_spark", 4);
            burning = fx_by_owner("pz_burningsmoke", 4);
            _mkproc_sleep_ticks = 10.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            if (controller->preround_active != 0 ||
                controller->phase != 0) {
                fx_reset(spark);
                fx_set_param_v3(
                    spark, 0x202, position.x, position.y, position.z);
                fx_resume_emit(spark);
                fx_reset(burning);
                fx_set_param_v3(
                    burning, 0x202, position.x, position.y + 0.5f,
                    position.z);
                fx_resume_emit(burning);
                fx_reset(smoke);
                fx_set_param_v3(
                    smoke, 0x202, position.x, position.y, position.z);
                fx_resume_emit(smoke);
                _mkproc_sleep_ticks = 20.0f;
                aproc->vtbl->sleep(aproc->vtbl);
                if (controller->preround_active != 0 ||
                    controller->phase != 0) {
                    fx_pause_emit(burning);
                    fx_pause_emit(smoke);
                }
            }
            _mkproc_sleep_ticks = 60.0f;
            aproc->vtbl->sleep(aproc->vtbl);
        }
        controller->preround_active = 0;
        return 1.0f;
    }

    if (controller->phase == 1 &&
        (randu0(10000) & 0xFFFF) < 15) {
        side = controller->hazard_initialized[0];
        position.x = side == 0
            ? (screen_width > 650 ? -2.2f : -1.9f)
            : (screen_width > 650 ? 2.2f : 1.9f);
        if ((randu0(100) & 0xFFFF) < 50) {
            position.x = -position.x;
        }
        pan_snd_req(0x1ABA, position.x < 0.0f ? -1.0f : 1.0f);
        spark = fx_by_owner("pz_spark", 4);
        smoke = fx_by_owner("pz_lightningsmoke", 4);
        burning = fx_by_owner("pz_burningsmoke", 4);
        fx_reset(spark);
        fx_set_param_v3(
            spark, 0x202, position.x, position.y, position.z);
        fx_resume_emit(spark);
        fx_reset(burning);
        fx_set_param_v3(
            burning, 0x202, position.x, position.y + 0.5f, position.z);
        fx_resume_emit(burning);
        fx_reset(smoke);
        fx_set_param_v3(
            smoke, 0x202, position.x, position.y, position.z);
        fx_resume_emit(smoke);
        _mkproc_sleep_ticks = 20.0f;
        aproc->vtbl->sleep(aproc->vtbl);
        fx_pause_emit(burning);
        fx_pause_emit(smoke);
        _mkproc_sleep_ticks = 5.0f;
        aproc->vtbl->sleep(aproc->vtbl);
    }
    return 1.0f;
}

/*
 * Broad pass: staged Grinder victim dismemberment, reusable flesh objects,
 * blood effects, and the final background meat throw.
 */
float r_pz_fighter_grinding(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleGrinderNoisePdata* noise = 0;
    PuzzleGrinderMeatController* meat = 0;
    PuzzleFighterRenderObject* pieces[4];
    Vec positions[4];
    Vec velocity;
    Vec terminal;
    Vec path;
    float effect_x;
    float effect_z;
    float direction;
    int bones[4] = {9, 20, 9, 9};
    int i;

    pz_fighter_clear_out_external_forces();
    if (plyr_pdata->side == 0) {
        effect_x = fighters->fighter_posts[1].x - 0.4f;
        effect_z = fighters->fighter_posts[1].z;
        direction = 1.0f;
    } else {
        effect_x = fighters->fighter_posts[0].x + 0.4f;
        effect_z = fighters->fighter_posts[0].z;
        direction = -1.0f;
    }

    bgnd_launch_fx_at_position(
        "grinding_fx", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    bgnd_launch_fx_at_position(
        "blood_up_spray_fx", plyr_obj->x, 0.0f, plyr_obj->z);
    snd_req_vol(0x1AB8, 1.0f);

    if (_create_mkproc_generic_bigstack(
            0x2001, 0x1F, p_grinder_noise, 0x10,
            (PuzzleFaceBleedPdata**)&noise) != 0 &&
        noise != 0) {
        noise->duration = 150;
        noise->timer = 0;
    }

    blend_to_ani(pz_shared_ani.head_poked, 3, 0.1f);
    set_ani_speed(0.44f);
    ani_to_frame_x(12.0f);
    bgnd_launch_fx_at_position(
        "chunk_1", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    snd_death_voice();
    ani_to_frame_x(22.0f);

    pieces[0] = fighters->grinder_meat_alt0;
    pieces[1] = fighters->grinder_meat_alt2;
    pieces[2] = fighters->grinder_meat_alt1;
    pieces[3] = fighters->grinder_meat_default;

    for (i = 0; i < 4; i++) {
        get_bone_world_pos(plyr_obj, bones[i], &positions[i]);
        if (i < 2) {
            positions[i].y = 0.8f;
        } else if (i == 2) {
            positions[i].y = 0.75f;
        } else {
            positions[i].y = 0.55f;
        }

        velocity.x = (i == 2 ? 0.105f : 0.02f) * direction;
        velocity.y = i == 0 ? 0.05f : (i == 1 ? 0.03f : 0.105f);
        velocity.z = (i == 3 ? 0.063f : -0.073f) * direction;
        terminal.x = 0.1f;
        terminal.y = i == 1 ? 0.03f : 0.05f;
        terminal.z = 0.1f * direction;
        path.x = 0.04f;
        path.y = 0.08f;
        path.z = 0.03f;

        unhide_obj(pieces[i]);
        ft_create_flesh_path(
            pieces[i], &positions[i].x, 0, 0, &velocity, 0, &terminal, 0,
            -0.004f, 0.5f, 0.1f);

        if (i == 0) {
            snd_req_delay(0x1ADD, 40);
            ani_to_frame_x(36.0f);
        } else if (i == 1) {
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
            set_ani_speed(0.44f);
            ani_to_frame_x(51.0f);
        } else if (i == 2) {
            snd_req(0x1AD7);
            snd_req_delay(0x1ADD, 35);
            snd_req_delay(0x1ADD, 50);
            ani_to_frame_x(55.0f);
        }
    }

    obj_set_bone_collapse_flag(plyr_obj, 21);
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_meat_throw_controller, 0x18,
            &meat) != 0 &&
        meat != 0) {
        meat->direction = his_pdata->side;
        meat->object = fighters->grinder_meat_final;
        meat->delay = 0;
        meat->phase = 1;
    }

    bgnd_launch_fx_at_position(
        "chunk_3", effect_x, plyr_obj->y, effect_z);
    if (plyr_pdata->side == 1) {
        bgnd_set_fx_ang_y(3.1415927f);
    }
    ani_to_blend_frame(175.0f);
    hide_obj(plyr_obj);
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

/* Broad pass: throw reusable flesh chunks across either grinder mouth. */
float p_grinder_meat_throw_controller(void) {
    PuzzleGrinderMeatController* meat =
        (PuzzleGrinderMeatController*)apdata;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFighterRenderObject* object;
    Vec throw_velocity = {1.1f, 1.8f, 0.0f};
    Vec drift_velocity = {-0.073f, 0.095f, 0.0f};
    Vec path_velocity = {0.04f, 0.08f, 0.03f};
    unsigned int choice;

    if (meat->delay != 0) {
        meat->delay--;
        return 1.0f;
    }

    if (meat->phase == 0) {
        if (screen_width > 650) {
            drift_velocity.x *= 1.1f;
        }
        if (meat->direction == 1) {
            throw_velocity.x *= -1.0f;
            drift_velocity.x *= -1.0f;
        }

        object = fighters->grinder_meat_default;
        choice = randu0(100) & 0xFFFF;
        if (choice < 20) {
            object = fighters->grinder_meat_alt0;
        } else if (choice < 40) {
            object = fighters->grinder_meat_alt1;
        } else if (choice < 60) {
            object = fighters->grinder_meat_alt2;
        }

        unhide_obj(object);
        ft_create_flesh_path(
            object, &throw_velocity.x, 0, 0, &drift_velocity, 0,
            &path_velocity, 0, -0.004f, 0.5f, 0.1f);
        meat->object = object;
        if (meat->object == 0) {
            return -1.0f;
        }
        meat->phase = 1;
        return 1.0f;
    }

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
    return 1.0f;
}

void pz_fighter_load_place_fatality_elements(int fatality) {
    g_pz_fighter_fatality_engine.primary_object = 0;
    g_pz_fighter_fatality_engine.secondary_object = 0;
    g_fatalityTable[fatality].load_and_place();
}

void pz_fighters_fatality_round_over(void) {
    static const PuzzleFatalityProcessTable source = {
        pz_fighter_grinder_round_over,
        pz_fighter_chomper_round_over,
        pz_fighter_chomper2_round_over,
        pz_fighter_objects_falling_round_over,
        pz_fighter_lightning_round_over,
        pz_fighter_snake_round_over,
        pz_fighter_burn_round_over,
        0,
        0,
        0,
    };
    PuzzleFatalityProcessTable functions;

    copy_fatality_process_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_normal_fighting(void) {
    static const PuzzleFatalityFnTable source = {
        (PuzzleFatalityFn)pz_fighter_grinder_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_chomper_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_chomper2_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_objects_falling_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_lightning_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_snake_actively_fighting,
        (PuzzleFatalityFn)pz_fighter_burn_actively_fighting,
        0,
        0,
        0,
    };
    PuzzleFatalityFnTable functions;

    copy_fatality_fn_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_unload(void) {
    static const PuzzleFatalityProcessTable source = {
        pz_fighter_grinder_unload,
        pz_fighter_chomper_unload,
        pz_fighter_chomper2_unload,
        pz_fighter_objects_falling_unload,
        pz_fighter_lightning_unload,
        pz_fighter_snake_unload,
        pz_fighter_burn_unload,
        0,
        0,
        0,
    };
    PuzzleFatalityProcessTable functions;

    copy_fatality_process_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

void pz_fighters_fatality_start(void) {
    static const PuzzleFatalityFnTable source = {
        pz_fighter_grinder_entering_fatality,
        pz_fighter_chomper_entering_fatality,
        pz_fighter_chomper2_entering_fatality,
        pz_fighter_objects_falling_entering_fatality,
        pz_fighter_lightning_entering_fatality,
        pz_fighter_snake_entering_fatality,
        pz_fighter_burn_entering_fatality,
        0,
        0,
        0,
    };
    PuzzleFatalityFnTable functions;

    copy_fatality_fn_table(&functions, &source);
    functions.entries[g_pz_fighters_engine.fatality_index]();
}

/* Broad pass: pz_fighter_attempt_push_into_grinder ~67.58%. */
float pz_fighter_attempt_push_into_grinder(void) {
    static const PuzzleAttackParameters source = {
        45.0f,
        0.1f,
        1.3f,
        0x00010001,
        0x00070007,
        3,
        0.4f,
        0.8f,
        1.1f,
        55.0f,
        47.0f,
        1,
        0,
        0,
        0,
    };
    PuzzleAttackParameters attack;

    memcpy(&attack, &source, sizeof(attack));
    pz_fighter_attack(pz_shared_ani.push_into_grinder, &attack, 0x21);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
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

/* Broad pass: load both Chomper columns, spikes, gushers, and controller. */
float pz_fighter_load_and_place_initial_chompers(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* columns[2];
    PuzzleFatalityController* controller = 0;
    PuzzleFatalityHazardObject* chomper;
    int i;
    int j;

    load_art_section(0x70036, &sec_pz_danger_chomper);
    g_pz_fighters_engine.fatality_index = 1;

    for (i = 0; i < 2; i++) {
        columns[i] =
            load_model_from_slot(0x70036, 0x081F0000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags |= 0x22;
        if (screen_width > 650) {
            columns[i]->x = i == 0 ? -4.1f : 4.1f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.24f;
            columns[i]->scale.x = 1.1f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.14f;
        } else {
            columns[i]->x = i == 0 ? -3.6f : 3.6f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.24f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.7f;
            columns[i]->scale.z = 1.04f;
        }
        columns[i]->external_force_x = 0.0f;
        columns[i]->external_force_y = 0.0f;
        columns[i]->external_force_z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    fatality->primary_object = columns[0];
    fatality->secondary_object = columns[1];
    obj_create_sobjs(columns[0]);
    obj_create_sobjs(columns[1]);
    fatality->hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(columns[0], 2);
    fatality->hazard_groups[1].objects[0] =
        obj_find_sobj_by_id(columns[1], 2);
    fatality->hazard_groups[0].objects[1] =
        obj_find_sobj_by_id(columns[0], 1);
    fatality->hazard_groups[1].objects[1] =
        obj_find_sobj_by_id(columns[1], 1);
    fatality->grinder_texture = load_tga(0x70036, 0x081F0002);

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
        fatality->controller = controller;
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            chomper = fatality->hazard_groups[i].objects[j];
            chomper->flags |= 0x60;
            chomper->motion = 0.0f;
            fatality->controller->chomper_position[i][j] = 0.0f;
            fatality->controller->hazard_motion[i][j] = 0.0f;
        }
        fatality->controller->hazard_initialized[i] = 0;
    }

    load_pz_fighter_fatality_bank(0x81);
    load_effect_bank_with_context("pz_chomper_fx.mko", &effect_context);
    return 0.0f;
}

float pz_fighters_chomper_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/* Broad pass: single-column chomper preparation and launch state, ~63.15%. */
float pz_fighters_chomper_fatality_prep(void) {
    static int attack_begun;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* chomper;
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterMove* victim_move;
    PuzzlePlayerData* attacker_data;
    Vec offset = {0.5f, 0.0f, 0.0f};
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

    attacker = fighters->fatality_attacker;
    victim = fighters->fatality_victim;

    switch (fatality->active_effect) {
    case 0:
        attacker_object = pz_fighter_get_player_obj(attacker);
        if (attacker == 0) {
            target_x = fighters->fighter_posts[1].x + offset.x;
            target_z = fighters->fighter_posts[1].z + offset.z;
        } else {
            target_x = fighters->fighter_posts[0].x - offset.x;
            target_z = fighters->fighter_posts[0].z - offset.z;
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

        if (attacker == 0) {
            if (fatality->primary_object->x < -edge) {
                fatality->primary_object->external_force_x = 0.02f;
            } else {
                fatality->primary_object->external_force_x = 0.0f;
            }
        } else {
            if (fatality->secondary_object->x > edge) {
                fatality->secondary_object->external_force_x = -0.02f;
            } else {
                fatality->secondary_object->external_force_x = 0.0f;
            }
        }

        chomper = fatality->hazard_groups[attacker].objects[0];
        if (chomper->y < 2.5f) {
            chomper->motion = 0.1f;
        } else {
            chomper->motion = 0.0f;
        }
        hazard_ready =
            chomper->motion == 0.0f &&
            (attacker == 0
                 ? fatality->primary_object->external_force_x == 0.0f
                 : fatality->secondary_object->external_force_x == 0.0f);

        if (signed_distance < 0.02f) {
            attacker_data = pz_get_pdata_by_id(attacker);
            pz_fighter_clear_out_all_external_forces(
                g_game_info.player1_physics);
            pz_fighter_clear_out_all_external_forces(
                g_game_info.player2_physics);
            if (attack_begun == 0) {
                attack_begun = 1;
                event = 0;
                minigame_event(&event);
            }

            if (g_game_info.player1->transition_locked == 0 &&
                g_game_info.player2->transition_locked == 0 &&
                hazard_ready) {
                player_distance = xz_distance_between_players();
                fatality->active_effect = 1;
                attack_begun = 0;
                if (player_distance < 2.7f) {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_just_backflip);
                } else {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_perform_taunt);
                }
                fatality->effect_timer = 35;
            } else if (attacker_data->transition_locked != 0) {
                attacker_data->transition_locked = 0;
                xfer_proc(pz_fighter_get_player_proc(attacker),
                          pz_fighter_exit);
                xfer_proc(pz_fighter_get_player_proc(victim),
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
        if (victim == 1) {
            g_game_info.player1->fatality_shove_active = 1;
        } else {
            g_game_info.player2->fatality_shove_active = 1;
        }
        victim_move = pz_get_fighter_move();
        fighters->fighter_reaction_cooldown = 0;
        victim_move->active = 1;
        if (signed_distance < 1.3f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_medium_shove);
        } else {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_huge_shove);
        }
        break;

    case 1:
        fatality->effect_timer--;
        if (fatality->effect_timer != 0) {
            break;
        }
        fatality->active_effect = 2;
        fighters->fatality_active = 1;
        if ((randu0(100) & 0xFFFF) < 30) {
            fighters->fatality_motion = 1.0f;
            fighters->fatality_timer = 315;
        } else {
            fighters->fatality_motion = 0.0f;
            fighters->fatality_timer = 285;
        }
        break;
    }

    return 0.0f;
}

/* Broad pass: load and initialize both Chomper2 columns and subobjects. */
float pz_fighter_load_and_place_initial_chompers2(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* columns[2];
    PuzzleFatalityController* controller = 0;
    PuzzleFatalityHazardObject* chomper;
    int i;

    load_art_section(0x70036, &sec_pz_danger_crusher);
    g_pz_fighters_engine.fatality_index = 2;

    for (i = 0; i < 2; i++) {
        columns[i] =
            load_model_from_slot(0x70036, 0x08200000, 0x6021);
        columns[i]->model_flags = 1;
        columns[i]->flags |= 0x22;
        if (screen_width > 650) {
            columns[i]->x = i == 0 ? -2.15f : 2.15f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.1f;
            columns[i]->scale.x = 1.2f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.3f;
        } else {
            columns[i]->x = i == 0 ? -1.75f : 1.75f;
            columns[i]->y = 0.0f;
            columns[i]->z = -0.1f;
            columns[i]->scale.x = 1.0f;
            columns[i]->scale.y = 0.8f;
            columns[i]->scale.z = 1.1f;
        }
        columns[i]->external_force_x = 0.0f;
        columns[i]->external_force_y = 0.0f;
        columns[i]->external_force_z = 0.0f;
        insert_fgnd_mkobj(columns[i]);
    }

    fatality->primary_object = columns[0];
    fatality->secondary_object = columns[1];
    obj_create_sobjs(columns[0]);
    obj_create_sobjs(columns[1]);
    fatality->hazard_groups[0].objects[0] =
        obj_find_sobj_by_id(columns[0], 1);
    fatality->hazard_groups[1].objects[0] =
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
        fatality->controller = controller;
    }

    for (i = 0; i < 2; i++) {
        chomper = fatality->hazard_groups[i].objects[0];
        chomper->flags |= 0x60;
        chomper->motion = 0.0f;
        fatality->controller->chomper_position[i][0] = 0.0f;
        fatality->controller->hazard_motion[i][0] = 0.0f;
        fatality->controller->hazard_initialized[i] = 0;
    }

    load_pz_fighter_fatality_bank(0x81);
    load_effect_bank_with_context("pz_crusher_fx.mko", &effect_context);
    return 0.0f;
}

float pz_fighters_chomper2_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/* Broad pass: two-column chomper preparation and launch state. */
float pz_fighters_chomper2_fatality_prep(void) {
    static int attack_begun;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* chomper;
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterMove* victim_move;
    PuzzlePlayerData* attacker_data;
    PuzzlePlayerData* victim_data;
    Vec offset = {0.05f, 0.0f, 0.0f};
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

    attacker = fighters->fatality_attacker;
    victim = fighters->fatality_victim;

    switch (fatality->active_effect) {
    case 0:
        attacker_object = pz_fighter_get_player_obj(attacker);
        if (attacker == 0) {
            target_x = fighters->fighter_posts[1].x + offset.x;
            target_z = fighters->fighter_posts[1].z + offset.z;
        } else {
            target_x = fighters->fighter_posts[0].x - offset.x;
            target_z = fighters->fighter_posts[0].z - offset.z;
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

        if (attacker == 0) {
            if (fatality->primary_object->x < -1.15f) {
                fatality->primary_object->external_force_x = 0.02f;
            } else {
                fatality->primary_object->external_force_x = 0.0f;
            }
        } else {
            if (fatality->secondary_object->x > 1.15f) {
                fatality->secondary_object->external_force_x = -0.02f;
            } else {
                fatality->secondary_object->external_force_x = 0.0f;
            }
        }

        chomper = fatality->hazard_groups[attacker].objects[0];
        if (chomper->y < 2.3f) {
            chomper->motion = 0.1f;
        } else {
            chomper->motion = 0.0f;
        }
        hazard_ready =
            chomper->motion == 0.0f &&
            (attacker == 0
                 ? fatality->primary_object->external_force_x == 0.0f
                 : fatality->secondary_object->external_force_x == 0.0f);

        if (signed_distance < 0.02f) {
            attacker_data = pz_get_pdata_by_id(attacker);
            pz_fighter_clear_out_all_external_forces(
                g_game_info.player1_physics);
            pz_fighter_clear_out_all_external_forces(
                g_game_info.player2_physics);
            if (attack_begun == 0) {
                attack_begun = 1;
                event = 0;
                minigame_event(&event);
            }

            if (g_game_info.player1->transition_locked == 0 &&
                g_game_info.player2->transition_locked == 0 &&
                hazard_ready) {
                player_distance = xz_distance_between_players();
                fatality->active_effect = 1;
                attack_begun = 0;
                if (player_distance < 2.7f) {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_backflip_and_point);
                } else {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_execute_point);
                }
                xfer_proc(
                    pz_fighter_get_player_proc(attacker),
                    pz_fighter_execute_point_reaction_no_space);
                fatality->effect_timer = 100;
            } else if (attacker_data->transition_locked != 0) {
                attacker_data->transition_locked = 0;
                xfer_proc(pz_fighter_get_player_proc(attacker),
                          pz_fighter_exit);
                xfer_proc(pz_fighter_get_player_proc(victim),
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
        victim_move = pz_get_fighter_move();
        fighters->fighter_reaction_cooldown = 0;
        victim_move->active = 1;
        if (signed_distance < 1.3f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_medium_shove);
        } else {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_fatality_huge_shove);
        }
        break;

    case 1:
        attacker_data = pz_get_pdata_by_id(attacker);
        victim_data = pz_get_pdata_by_id(victim);
        if (victim_data->transition_locked != 0) {
            break;
        }
        if (attacker_data->transition_locked != 0) {
            fatality->effect_timer--;
            if (fatality->effect_timer >= 0) {
                break;
            }
        }
        fatality->active_effect = 2;
        fighters->fatality_active = 1;
        fighters->fatality_timer = 125;
        break;
    }

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

/* Broad pass: load Snake models, animation state, controller, and emitters. */
float pz_fighter_load_and_place_initial_snake(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* snakes[2];
    PuzzleAnimPdata* snake_pdata[2];
    PuzzleFatalityController* controller = 0;
    PuzzleParticleEffect* particle_effect;
    PuzzleParticleEmitter* emitter;
    void* effect;
    int i;

    load_art_section(0x70036, &sec_pz_danger_snake);
    load_effect_bank_with_context("pz_snake_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 5;

    for (i = 0; i < 2; i++) {
        snakes[i] =
            load_model_from_slot(0x70036, 0x08230000, 0x6021);
        obj_change_to_skinned_obj_light_list(
            snakes[i], skinned_obj_light_def);
        snakes[i]->model_flags = 0x400;
        snakes[i]->x = i == 0 ? -2.18f : 2.18f;
        if (screen_width > 650) {
            snakes[i]->x = i == 0 ? -2.65f : 2.65f;
        }
        snakes[i]->y = 0.3f;
        snakes[i]->z = -0.28f;
        snakes[i]->flags |= 0x0A;
        snakes[i]->facing_angle =
            i == 0 ? 1.0207963f : -1.0207963f;
        snakes[i]->scale.x = 0.48f;
        snakes[i]->scale.y = 0.48f;
        snakes[i]->scale.z = 0.48f;
        insert_fgnd_mkobj(snakes[i]);
        snake_pdata[i] = animate_obj(
            snakes[i], pz_shared_ani.snake_idle, 1.0f,
            skinned_obj_light_def + 0x44, 0, 0, 1);
    }

    obj_add_to_skinned_obj_light_list_with_ambient(
        snakes[0], skinned_obj_ambient_light_def);
    fatality->secondary_object = snakes[1];
    fatality->primary_object = snakes[0];
    obj_create_sobjs(snakes[0]);
    obj_create_sobjs(snakes[1]);
    sobj_set_priority(obj_first_sobj(snakes[0]), 0x12);
    sobj_set_priority(obj_first_sobj(snakes[1]), 0x12);

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
        fatality->controller = controller;
        controller->grinder_position[0] =
            fatality->primary_object->motion_rate;
        controller->grinder_position[1] =
            fatality->secondary_object->motion_rate;
        controller->grinder_target[0] = controller->grinder_position[0];
        controller->grinder_target[1] = controller->grinder_position[1];
    }
    fatality->controller->fighter_pdata[0] = snake_pdata[0];
    fatality->controller->fighter_pdata[1] = snake_pdata[1];
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

#undef SETUP_SNAKE_EFFECT

float pz_fighters_snake_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

/* Broad pass: snake spacing, animation, and bite preparation. */
float pz_fighters_snake_fatality_prep(void) {
    static int attack_begun;
    static int loser_anim;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;
    PuzzleFighterRenderObject* attacker_object;
    PuzzleFighterMove* victim_move;
    PuzzlePlayerData* victim_data;
    PuzzlePlayerData* attacker_data;
    float target_x;
    float delta_x;
    float delta_z;
    float signed_distance;
    float player_distance;
    float lower_bound;
    float upper_bound;
    int direction;
    int event;
    int attacker;
    int victim;

    attacker = fighters->fatality_attacker;
    victim = fighters->fatality_victim;
    attacker_object = pz_fighter_get_player_obj(attacker);
    if (screen_width > 650) {
        target_x = attacker == 0 ? -1.4f : 1.4f;
    } else {
        target_x = attacker == 0 ? -0.6f : 0.6f;
    }

    delta_x = target_x - attacker_object->x;
    delta_z = -attacker_object->z;
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
    player_distance = xz_distance_between_players();

    lower_bound = -0.04f;
    upper_bound = 0.006f;
    if (victim == 0) {
        lower_bound = -0.006f;
        upper_bound = 0.04f;
        victim_data = g_game_info.player1;
        attacker_data = g_game_info.player2;
    } else {
        victim_data = g_game_info.player2;
        attacker_data = g_game_info.player1;
    }

    if (signed_distance > lower_bound &&
        signed_distance < upper_bound) {
        if (attack_begun == 0) {
            event = 0;
            minigame_event(&event);
        }
        attack_begun = 0;
        fighters->fatality_active = 1;
        fx_pause_emit(fx_by_owner("saliva1", 4));
        fx_pause_emit(fx_by_owner("saliva2", 4));

        if (loser_anim == 0) {
            transition_to_anim_script_frame(
                controller->fighter_pdata[victim],
                pz_shared_ani.snake_victim, 0.05f, 0.0f);
            set_pdata_anim_step(
                controller->fighter_pdata[victim], 1.0f);
        }
        loser_anim = 0;
        transition_to_anim_script_frame(
            controller->fighter_pdata[attacker],
            pz_shared_ani.snake_attacker, 0.05f, 0.0f);
        set_pdata_anim_step(
            controller->fighter_pdata[attacker], 1.0f);
        fighters->snake_active = 1;
        xfer_proc(pz_fighter_get_player_proc(attacker),
                  r_pz_fighter_eaten);
        if (victim_data->transition_locked == 0 &&
            player_distance < 1.0f) {
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_just_backflip);
        }
        fighters->fatality_timer = 210;
        return 0.0f;
    }

    if (attack_begun == 0) {
        attack_begun = 1;
        event = 0;
        minigame_event(&event);
    }
    if (loser_anim == 0) {
        fx_pause_emit(fx_by_owner("saliva1", 4));
        fx_pause_emit(fx_by_owner("saliva2", 4));
        transition_to_anim_script_frame(
            controller->fighter_pdata[victim],
            pz_shared_ani.snake_victim, 0.05f, 0.0f);
        set_pdata_anim_step(
            controller->fighter_pdata[victim], 1.0f);
        loser_anim = 1;
    }

    if (attacker_data->transition_locked == 0 &&
        signed_distance < lower_bound) {
        xfer_proc(pz_fighter_get_player_proc(attacker),
                  pz_fighter_walk_forward);
    } else if (victim_data->transition_locked == 0 &&
               signed_distance > upper_bound) {
        victim_move = pz_get_fighter_move();
        victim_move->active = 1;
        fighters->fighter_reaction_cooldown = 0;
        xfer_proc(pz_fighter_get_player_proc(victim),
                  pz_fighter_fatality_victim_to_exact_spot);
    }

    if (signed_distance < lower_bound &&
        victim_data->transition_locked == 0 &&
        player_distance < 1.0f) {
        xfer_proc(pz_fighter_get_player_proc(victim),
                  pz_fighter_just_backflip);
    }
    return 0.0f;
}

/* Broad pass: load the one-ton hazards and their shared controller. */
float pz_fighter_load_and_place_initial_objects_falling(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFighterRenderObject* objects[2];
    PuzzleFatalityController* controller = 0;
    int i;

    load_art_section(0x70036, &sec_pz_danger_1ton);
    load_effect_bank_with_context("pz_1ton_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 3;

    for (i = 0; i < 2; i++) {
        objects[i] = load_model_from_slot(0x70036, 0x08210000, 0x6021);
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

float pz_fighters_objects_falling_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    if (g_pz_fighter_fatality_engine.controller
            ->preround_sound_started == 0) {
        snd_req(0x1ACB);
    }
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 1;
    return 0.0f;
}

/* Broad pass: falling-object pointing and activation state machine. */
float pz_fighters_objects_falling_fatality_prep(void) {
    static int one_last_hit;
    static int start_pointing;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFatalityHazardObject* falling_object;
    PuzzleFighterRenderObject* victim_object;
    PuzzleFighterRenderObject* attacker_object;
    Vec offset = {0.05f, 0.0f, 0.0f};
    float player_distance;
    float target_x;
    float target_z;
    float delta_x;
    float delta_z;
    float signed_distance;
    int direction;
    int event;
    int victim;

    switch (fatality->active_effect) {
    case 0:
        if (one_last_hit == 1) {
            if (g_game_info.player1->transition_locked != 0 ||
                g_game_info.player2->transition_locked != 0) {
                break;
            }

            player_distance = xz_distance_between_players();
            victim = fighters->fatality_victim;
            victim_object = pz_fighter_get_player_obj(victim);
            if (victim == 0) {
                target_x = fighters->fighter_posts[1].x + offset.x;
                target_z = fighters->fighter_posts[1].z + offset.z;
            } else {
                target_x = fighters->fighter_posts[0].x - offset.x;
                target_z = fighters->fighter_posts[0].z - offset.z;
            }

            delta_x = target_x - victim_object->x;
            delta_z = target_z - victim_object->z;
            direction = 1;
            if (target_x < 0.0f) {
                if (victim_object->x >= target_x) {
                    direction = -1;
                }
            } else if (victim_object->x > target_x) {
                direction = -1;
            }
            signed_distance =
                direction * ((delta_x * delta_x) + (delta_z * delta_z));

            if (player_distance < 1.5f) {
                if (signed_distance > 1.0f) {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_dash_back);
                } else {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_shove);
                }
            }
            one_last_hit = 0;
            start_pointing = 1;
        } else if (start_pointing == 1) {
            victim = fighters->fatality_victim;
            if (pz_get_pdata_by_id(victim)->transition_locked != 0) {
                break;
            }
            start_pointing = 0;
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_execute_point_no_space_check);
        } else {
            start_pointing = 1;
            one_last_hit = 1;
            event = 0;
            minigame_event(&event);
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_execute_R_coming_down);
            fatality->active_effect = 1;
        }
        break;

    case 1:
        fatality->active_effect = 4;
        fatality->effect_timer = 63;
        break;

    case 4:
        attacker_object =
            pz_fighter_get_player_obj(fighters->fatality_attacker);
        if (fatality->effect_timer > 0) {
            fatality->effect_timer--;
        }
        if (fatality->effect_timer == 0) {
            fatality->active_effect = 2;
            falling_object = fatality->hazard_groups[0].objects[0];
            falling_object->flags |= 0x62;
            falling_object->x = attacker_object->x;
            falling_object->y = 3.2f;
            falling_object->z = attacker_object->z;
            falling_object->motion = -0.052f;
            falling_object->scale.x = 0.5f;
            falling_object->scale.y = 0.5f;
            falling_object->scale.z = 0.5f;
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep(aproc->vtbl);
            unhide_sobj(falling_object);
            fighters->fatality_active = 1;
            fighters->fatality_timer = 150;
        }
        break;
    }

    return 0.0f;
}

/* Broad pass: load the lightning effects and initialize their controller. */
float pz_fighter_load_and_place_initial_lightning(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityController* controller = 0;

    load_art_section(0x70036, &sec_pz_danger_lightning);
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

float pz_fighters_lightning_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->preround_sound_started = 0;
    return 0.0f;
}

/* Broad pass: lightning pointing and activation state machine. */
float pz_fighters_lightning_fatality_prep(void) {
    static int one_last_hit;
    static int start_pointing;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* victim_object;
    void* lightning_smoke;
    void* burning_smoke;
    Vec offset = {0.05f, 0.0f, 0.0f};
    float player_distance;
    float target_x;
    float target_z;
    float delta_x;
    float delta_z;
    float signed_distance;
    int direction;
    int event;
    int victim;

    lightning_smoke = fx_by_owner("pz_lightningsmoke", 4);
    burning_smoke = fx_by_owner("pz_burningsmoke", 4);
    fx_pause_emit(burning_smoke);
    fx_pause_emit(lightning_smoke);

    switch (fatality->active_effect) {
    case 0:
        if (one_last_hit == 1) {
            if (g_game_info.player1->transition_locked != 0 ||
                g_game_info.player2->transition_locked != 0) {
                break;
            }

            player_distance = xz_distance_between_players();
            victim = fighters->fatality_victim;
            victim_object = pz_fighter_get_player_obj(victim);
            if (screen_width > 650) {
                offset.x = 0.35f;
            }
            if (victim == 0) {
                target_x = fighters->fighter_posts[1].x + offset.x;
                target_z = fighters->fighter_posts[1].z + offset.z;
            } else {
                target_x = fighters->fighter_posts[0].x - offset.x;
                target_z = fighters->fighter_posts[0].z - offset.z;
            }

            delta_x = target_x - victim_object->x;
            delta_z = target_z - victim_object->z;
            direction = 1;
            if (target_x < 0.0f) {
                if (victim_object->x >= target_x) {
                    direction = -1;
                }
            } else if (victim_object->x > target_x) {
                direction = -1;
            }
            signed_distance =
                direction * ((delta_x * delta_x) + (delta_z * delta_z));

            if (player_distance < 1.5f) {
                if (signed_distance > 0.9f) {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_dash_back);
                } else {
                    xfer_proc(pz_fighter_get_player_proc(victim),
                              pz_fighter_shove);
                }
            }
            one_last_hit = 0;
            start_pointing = 1;
        } else if (start_pointing == 1) {
            victim = fighters->fatality_victim;
            if (pz_get_pdata_by_id(victim)->transition_locked != 0) {
                break;
            }
            start_pointing = 0;
            xfer_proc(pz_fighter_get_player_proc(victim),
                      pz_fighter_execute_point_no_space_check);
        } else {
            start_pointing = 1;
            one_last_hit = 1;
            event = 0;
            minigame_event(&event);
            xfer_proc(
                pz_fighter_get_player_proc(fighters->fatality_attacker),
                pz_fighter_execute_point_reaction_no_space);
            fatality->active_effect = 1;
        }
        break;

    case 1:
        fatality->active_effect = 4;
        fatality->effect_timer = 63;
        break;

    case 4:
        if (fatality->effect_timer > 0) {
            fatality->effect_timer--;
        }
        if (fatality->effect_timer == 0) {
            fatality->active_effect = 2;
            fighters->fatality_active = 1;
            fighters->fatality_timer = 180;
        }
        break;
    }

    return 0.0f;
}

/* Broad pass: load both burn hazards, controller, and idle flame emitters. */
float pz_fighter_load_and_place_initial_burn(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* burners[2];
    PuzzleFatalityController* controller = 0;
    float effect_x;
    int i;

    load_art_section(0x70036, &sec_pz_danger_burn);
    load_effect_bank_with_context("pz_burn_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 6;

    for (i = 0; i < 2; i++) {
        burners[i] =
            load_model_from_slot(0x70036, 0x08240000, 0x6021);
        burners[i]->model_flags = 1;
        burners[i]->x = i == 0 ? -2.25f : 2.25f;
        if (screen_width > 650) {
            burners[i]->x = i == 0 ? -2.65f : 2.65f;
        }
        burners[i]->y = 0.0f;
        burners[i]->z = 0.0f;
        burners[i]->flags |= 0x02;
        burners[i]->scale.x = 0.3f;
        burners[i]->scale.y = 0.3f;
        burners[i]->scale.z = 0.3f;
        insert_fgnd_mkobj(burners[i]);
    }

    fatality->primary_object = burners[0];
    fatality->secondary_object = burners[1];
    obj_create_sobjs(burners[0]);
    obj_create_sobjs(burners[1]);

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
        fatality->controller = controller;
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

float pz_fighters_burn_fatality_preround(void) {
    g_pz_fighter_fatality_engine.controller->preround_active = 1;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.3f;
    g_pz_fighter_fatality_engine.controller->preround_timer = 120;
    return 0.0f;
}

/* Broad pass: burn fatality preparation and attack selection. */
float pz_fighters_burn_fatality_prep(void) {
    static int start_burn;
    static int attack_begun;
    PuzzleFightersEngine* engine = &g_pz_fighters_engine;
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

    attacker = engine->fatality_attacker;
    victim = engine->fatality_victim;
    attacker_object = pz_fighter_get_player_obj(attacker);
    if (screen_width > 650) {
        target_x = attacker == 0 ? -2.3f : 2.3f;
    } else {
        target_x = attacker == 0 ? -1.8f : 1.8f;
    }

    delta_x = target_x - attacker_object->x;
    delta_z = -attacker_object->z;
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

    if (start_burn == 0) {
        start_burn = 1;
        if (screen_width > 650) {
            effect_x = attacker == 0 ? -2.3f : 2.3f;
        } else {
            effect_x = attacker == 0 ? -1.8f : 1.8f;
        }
        if (attacker == 0) {
            pan_snd_req(0x1AB2, -0.7f);
            effect_x -= 0.15f;
        } else {
            pan_snd_req(0x1AB2, 0.7f);
            effect_x += 0.2f;
        }
        bgnd_launch_fx_at_position(
            "super_roar_flames", effect_x, 0.0f, 0.0f);
    }

    if (signed_distance < 0.014f && engine->fatality_ready == 1) {
        if (attack_begun == 0) {
            event = 0;
            minigame_event(&event);
        }
        attack_begun = 0;
        start_burn = 0;
        engine->fatality_active = 1;
        xfer_proc(pz_fighter_get_player_proc(attacker), r_pz_fighter_burn);
        xfer_proc(pz_fighter_get_player_proc(victim),
                  r_pz_fighter_summon_burn);
        engine->fatality_timer = 138;
        return 0.0f;
    }

    if (g_game_info.player1->transition_locked != 0 ||
        g_game_info.player2->transition_locked != 0) {
        return 0.0f;
    }

    if (attack_begun == 0) {
        attack_begun = 1;
        event = 0;
        minigame_event(&event);
    }
    victim_move = pz_get_fighter_move();
    victim_move->active = 1;
    engine->fighter_reaction_cooldown = 0;
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
    return 0.0f;
}

/* Broad pass: load, place, and initialize both grinder scene objects. */
float pz_fighter_load_and_place_initial_grinders(void) {
    PuzzleEffectBankContext effect_context = {0x70036, 0, 0};
    PuzzleFatalityEngine* fatality = &g_pz_fighter_fatality_engine;
    PuzzleFighterRenderObject* grinders[2];
    PuzzleFatalityController* controller = 0;
    int i;

    load_art_section(0x70036, &sec_pz_danger_grinder);
    load_effect_bank_with_context("pz_grinder_fx.mko", &effect_context);
    g_pz_fighters_engine.fatality_index = 0;

    for (i = 0; i < 2; i++) {
        grinders[i] =
            load_model_from_slot(0x70036, 0x081E0000, 0x6021);
        grinders[i]->model_flags = 1;
        grinders[i]->x = i == 0 ? -2.2f : 2.2f;
        if (screen_width > 650) {
            grinders[i]->x = i == 0 ? -2.65f : 2.65f;
        }
        grinders[i]->y = 0.0f;
        grinders[i]->z = 0.0f;
        grinders[i]->flags |= 0x06;
        grinders[i]->motion_rate = 0.05f;
        grinders[i]->scale.x = 0.6f;
        grinders[i]->scale.y = 0.6f;
        grinders[i]->scale.z = 0.6f;
        insert_fgnd_mkobj(grinders[i]);
    }

    fatality->primary_object = grinders[0];
    fatality->secondary_object = grinders[1];
    obj_create_sobjs(grinders[0]);
    obj_create_sobjs(grinders[1]);
    fatality->grinder_texture = load_tga(0x70036, 0x081E0002);

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
        fatality->controller = controller;
        controller->grinder_position[0] =
            fatality->primary_object->motion_rate;
        controller->grinder_position[1] =
            fatality->secondary_object->motion_rate;
        controller->grinder_target[0] = controller->grinder_position[0];
        controller->grinder_target[1] = controller->grinder_position[1];
    }

    load_pz_fighter_fatality_bank(0x82);
    return 0.0f;
}

void pz_fighters_grinder_fatality_preround(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;
    PuzzleGrinderMeatController* meat_controller = 0;
    int delay;
    int direction;

    controller->preround_active = 1;
    controller->loop_sound = snd_req_vol(0x1AB7, 0.3f);
    controller->phase_time = 0.3f;
    controller->preround_timer = 120;

    delay = (randu0(10) & 0xFFFF) + 10;
    direction = ((int)(randu0(100) & 0xFFFF) - 50) < 0;
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_grinder_meat_throw_controller, 0x18,
            &meat_controller) != 0 &&
        meat_controller != 0) {
        meat_controller->direction = direction;
        meat_controller->delay = delay;
        meat_controller->phase = 0;
    }
}

/* Broad pass: shared fatality-prep attack-selection skeleton. */
float pz_fighters_grinder_fatality_prep(void) {
    static int attack_begun;
    PuzzleFightersEngine* engine = &g_pz_fighters_engine;
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

    attacker = engine->fatality_attacker;
    victim = engine->fatality_victim;
    attacker_object = pz_fighter_get_player_obj(attacker);
    if (attacker == 0) {
        target_x = engine->fighter_posts[1].x;
        target_z = engine->fighter_posts[1].z;
    } else {
        target_x = engine->fighter_posts[0].x;
        target_z = engine->fighter_posts[0].z;
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

    if (signed_distance < 0.014f && engine->fatality_ready == 1) {
        if (attack_begun == 0) {
            event = 0;
            minigame_event(&event);
        }
        attack_begun = 0;
        engine->fatality_active = 1;
        xfer_proc(pz_fighter_get_player_proc(attacker),
                  r_pz_fighter_grinding);
        xfer_proc(pz_fighter_get_player_proc(victim),
                  pz_fighter_disgusted_with_grinding);
        engine->fatality_timer = 180;
        return 0.0f;
    }

    if (g_game_info.player1->transition_locked != 0 ||
        g_game_info.player2->transition_locked != 0) {
        return 0.0f;
    }

    if (attack_begun == 0) {
        attack_begun = 1;
        event = 0;
        minigame_event(&event);
    }
    victim_move = pz_get_fighter_move();
    victim_move->active = 1;
    engine->fighter_reaction_cooldown = 0;
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
    return 0.0f;
}

void pz_fighter_burn_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

/* Soft ceiling: pz_fighter_snake_entering_fatality ~99.75% -- stop. */
void pz_fighter_snake_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase_time = 0.6f;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

void pz_fighter_objects_falling_entering_fatality(
    int attacker, int victim) {
    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;
}

void pz_fighter_lightning_entering_fatality(
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

/* Broad pass: pz_fighter_chomper2_entering_fatality ~66.72%. */
void pz_fighter_chomper2_entering_fatality(
    int attacker, int victim) {
    int i;

    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

    for (i = 0; i < 2; i++) {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[i][0] = 0.0f;
    }
}

/* Broad pass: pz_fighter_chomper_entering_fatality ~56.33%. */
void pz_fighter_chomper_entering_fatality(
    int attacker, int victim) {
    int group;
    int object;

    g_pz_fighter_fatality_engine.active_effect = 0;
    g_pz_fighter_fatality_engine.controller->active = 1;
    g_pz_fighter_fatality_engine.controller->substate = 0;
    g_pz_fighter_fatality_engine.controller->state = 0;
    g_pz_fighter_fatality_engine.controller->attacker_player = attacker;
    g_pz_fighter_fatality_engine.controller->victim_player = victim;
    g_pz_fighter_fatality_engine.controller->phase = 0;

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

/* Soft ceiling: pz_fighter_grinder_entering_fatality ~99.71% -- stop. */
void pz_fighter_grinder_entering_fatality(
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

float pz_fighter_burn_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

float pz_fighter_snake_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

float pz_fighter_lightning_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

float pz_fighter_objects_falling_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
}

/* Broad pass: pz_fighter_chomper2_round_over ~58.71%. */
float pz_fighter_chomper2_round_over(void) {
    int i;

    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    i = 0;
    do {
        g_pz_fighter_fatality_engine.hazard_groups[i].objects[0]->motion =
            0.0f;
        g_pz_fighter_fatality_engine.controller->hazard_motion[i][0] = 0.0f;
        i++;
    } while (i < 2);
    return 0.0f;
}

/* Broad pass: pz_fighter_chomper_round_over ~59.34%. */
float pz_fighter_chomper_round_over(void) {
    int group;
    int object;

    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    group = 0;
    do {
        object = 0;
        do {
            g_pz_fighter_fatality_engine.hazard_groups[group]
                .objects[object]
                ->motion = 0.0f;
            g_pz_fighter_fatality_engine.controller
                ->hazard_motion[group][object] = 0.0f;
            object++;
        } while (object < 2);
        group++;
    } while (group < 2);
    return 0.0f;
}

float pz_fighter_grinder_round_over(void) {
    g_pz_fighter_fatality_engine.controller->state = 1;
    g_pz_fighter_fatality_engine.controller->phase = 0;
    return 0.0f;
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

static float pz_fighter_snake_unload(void) {
    PuzzleFatalityController* controller =
        g_pz_fighter_fatality_engine.controller;

    if (controller != 0) {
        controller->unload_requested = 1;
    }
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_lightning_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_objects_falling_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_chomper2_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
}

static float pz_fighter_chomper_unload(void) {
    g_pz_fighter_fatality_engine.controller->unload_requested = 1;
    unload_pz_fighter_fatality_banks();
    return 0.0f;
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

static void ft_fleshchunk_postsleep(void) {
    pdata_fleshchunk = 0;
    fleshchunk_obj = 0;
}

/*
 * Soft ceiling: ft_fleshchunk_prewake ~96.07% -
 * object-latch branch/register scheduling.
 */
static void ft_fleshchunk_prewake(void) {
    PuzzleFighterRenderObject* object;

    pdata_fleshchunk = (PuzzleFleshchunkPdata*)apdata;
    if (pdata_fleshchunk == 0) {
        mkproc_die();
    }

    object = pdata_fleshchunk->object;
    if (object != 0) {
        if (object->instance == pdata_fleshchunk->object_instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
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
