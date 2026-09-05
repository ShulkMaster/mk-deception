/*
 * Port readiness:
 *   Structs: PARTIAL
 *   Fuzzy: 16.98% (.text)
 *   Linked: NO
 *   Status: SCAFFOLD
 *   Gaps: attacks, reactions, movement, presentation, and scripts remain
 */

#include "runtime/plyr_pdata.h"
#include "game/game_info.h"
#include "game/pz_fatality.h"
#include "math/mk_math.h"
#include "rw/rwframe.h"
#include "rw/rwcore_types.h"

typedef float (*PuzzleMoveEntry)(void);
typedef float (*PuzzleProcessTransfer)(PuzzleMoveEntry entry, float delay);
typedef float (*PuzzleProcessSleep)(void);
typedef float (*PuzzleFighterFunction)(void);
typedef struct ScriptSlot ScriptSlot;
typedef struct MkPfx MkPfx;

typedef struct PuzzleCmdScript {
    char pad00[0x28];
    int function; /* +0x28 */
} PuzzleCmdScript;

typedef struct PuzzleProcessVtable {
    char pad00[0x18];
    PuzzleProcessSleep sleep; /* +0x18 */
    char pad1C[8];
    PuzzleProcessTransfer transfer; /* +0x24 */
} PuzzleProcessVtable;

typedef struct PuzzleProcess {
    PuzzleProcessVtable* vtbl;
    unsigned int instance;
} PuzzleProcess;

typedef struct PuzzlePresentState {
    char pad00[8];
    int state; /* +0x08 */
    PlyrPdata* owner; /* +0x0C */
} PuzzlePresentState;

typedef struct PuzzleFighterMove {
    char pad00[0x14];
    unsigned int script_move; /* +0x14 */
    char pad18[4];
    int distance_class; /* +0x1C */
    unsigned int active_flags; /* +0x20 */
} PuzzleFighterMove;

typedef struct PuzzleFightersEngine {
    char pad00[0x74];
    int peak_mode; /* +0x74 */
    unsigned int special_move_enabled; /* +0x78 */
    char pad7C[0x0C];
    int peak_active; /* +0x88 */
    union {
        unsigned char flags; /* +0x8C */
        struct {
            unsigned char pad_flags_7 : 1;
            unsigned char special_move_4 : 1;
            unsigned char special_move_5 : 1;
            unsigned char pad_flags_4 : 1;
            unsigned char continuation_allowed : 1;
            unsigned char continuation_reset : 1;
            unsigned char easy_continuation : 1;
            unsigned char pad_flags_0 : 1;
        } flag_bits;
    };
    union {
        unsigned char flags2; /* +0x8D */
        struct {
            unsigned char continuation_blocked : 1;
            unsigned char pad_flags2_6_0 : 7;
        } flag2_bits;
    };
    char pad8E[0xC2];
    struct PuzzleFighterObject* present_object; /* +0x150 */
    struct PuzzleFighterObject* projectile_objects[2]; /* +0x154 */
    char pad15C[8];
    int breakout; /* +0x164 */
    char pad168[0x54];
    PuzzlePresentState* present; /* +0x1BC */
    char pad1C0[0x24];
    int reactions_disabled; /* +0x1E4 */
} PuzzleFightersEngine;

typedef struct PuzzleReactionTransferData {
    char pad00[8];
    PuzzleProcess* opponent_proc; /* +0x08 */
    unsigned int opponent_proc_instance; /* +0x0C */
    PlyrPdata* opponent_pdata; /* +0x10 */
    struct PuzzleFighterObject* opponent_obj; /* +0x14 */
} PuzzleReactionTransferData;

typedef struct PuzzleReactionTransferEntry {
    int call_type;
    PuzzleMoveEntry entry;
    char pad08[8];
    unsigned int movement_flags;
} PuzzleReactionTransferEntry;

typedef struct PuzzleReactionDispatch {
    int call_type;
    PuzzleMoveEntry entry;
} PuzzleReactionDispatch;

typedef struct PuzzleProjectile {
    char pad00[8];
    struct PuzzleFighterObject* object; /* +0x08 */
    int launch_immediately; /* +0x0C */
    int state; /* +0x10 */
    struct PuzzleFighterObject* launch_bone_owner; /* +0x14 */
    struct PuzzleFighterObject* target; /* +0x18 */
    PlyrPdata* owner; /* +0x1C */
    PlyrPdata* opponent_pdata; /* +0x20 */
    int timer; /* +0x24 */
    unsigned int effect; /* +0x28 */
} PuzzleProjectile;

typedef struct PuzzleFighterObject {
    char pad00[8];
    union {
        unsigned char flags_08; /* +0x08 */
        struct {
            unsigned char pad08_bit7 : 1;
            unsigned char presentation_active : 1; /* bit6 */
            unsigned char pad08_middle : 5;
            unsigned char gravity_enabled : 1; /* bit0 */
        };
    };
    union {
        unsigned char flags_09; /* +0x09 */
        struct {
            unsigned char pad_high : 4;
            unsigned char reaction_locked : 1; /* bit3 */
            unsigned char pad_low : 3;
        } action_flags;
        struct {
            unsigned char pad_high : 6;
            unsigned char unk_bit1 : 1;
            unsigned char pad_low : 1;
        } presentation_flags;
        struct {
            unsigned char launched : 1;
            unsigned char pad_bit6 : 1;
            unsigned char tightrope_restricted : 1;
            unsigned char pad_bit4 : 1;
            unsigned char face_opponent : 1;
            unsigned char pad_bits2_0 : 3;
        } movement_flags;
    };
    char pad0A[0x16];
    RwFrame* frame; /* +0x20 */
    char pad24[0x0C];
    float gravity; /* +0x30 */
    char pad34[0x6C];
    union {
        struct {
            float x; /* +0xA0 */
            float y; /* +0xA4 */
            float z; /* +0xA8 */
        };
        Vec position;
    };
    char padAC[4];
    float external_force_x; /* +0xB0 */
    float vertical_velocity; /* +0xB4 */
    float external_force_z; /* +0xB8 */
    char padBC[0x18];
    float angle_y; /* +0xD4 */
} PuzzleFighterObject;

typedef AniData PuzzleAnimation;
typedef struct PuzzleAnimPdata {
    char pad00[0x30];
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float current_frame; /* +0x38 */
    char pad3C[4];
    float end_frame; /* +0x40 */
    float step; /* +0x44 */
    char pad48[0x1C];
    float field_64;
    float field_68;
    char pad6C[0x3C];
    float blend_weight; /* +0xA8 */
} PuzzleAnimPdata;

typedef struct PuzzleSharedCombatAnimations {
    char pad000[0x20];
    PuzzleAnimation* dash_back; /* +0x20 */
    char pad024[0x0C];
    PuzzleAnimation* step_throw; /* +0x30 */
    char pad034[0x108];
    PuzzleAnimation* swept_in; /* +0x13C */
    PuzzleAnimation* swept_reverse; /* +0x140 */
    PuzzleAnimation* swept_out; /* +0x144 */
    char pad148[0x7C];
    PuzzleAnimation* ermac_slam; /* +0x1C4 */
    char pad1C8[0xE4];
    PuzzleAnimation* block_high; /* +0x2AC */
    char pad2B0[0x34];
    PuzzleAnimation* block_low_loop; /* +0x2E4 */
    PuzzleAnimation* block_low_start; /* +0x2E8 */
    char pad2EC[0x40];
    PuzzleAnimation* dizzy; /* +0x32C */
    char pad330[0x48];
    PuzzleAnimation* dizzyfall_recover; /* +0x378 */
} PuzzleSharedCombatAnimations;

typedef struct PuzzleReactionDelayPdata {
    char header[8];
    int ticks; /* +0x08 */
    int reaction; /* +0x0C */
    void* saved_pdata; /* +0x10 */
} PuzzleReactionDelayPdata;

typedef struct PuzzleCameraShakePdata {
    char header[8];
    int duration; /* +0x08 */
    float strength; /* +0x0C */
} PuzzleCameraShakePdata;

typedef struct PuzzleSharedAnimations {
    char pad000[0x10];
    PuzzleAnimation* dizzy_punch; /* +0x10 */
    char pad014[4];
    PuzzleAnimation* double_arm_victory; /* +0x18 */
    char pad01C[8];
    PuzzleAnimation* uppercut_brush_back; /* +0x24 */
    char pad028[0x10];
    PuzzleAnimation* backflip; /* +0x38 */
    char pad03C[0x10];
    PuzzleAnimation* footstomp; /* +0x4C */
    PuzzleAnimation* shove; /* +0x50 */
    PuzzleAnimation* won2; /* +0x54 */
    char pad058[0x18];
    PuzzleAnimation* gaydance; /* +0x70 */
    PuzzleAnimation* whatever2; /* +0x74 */
    PuzzleAnimation* workthecrowd_start; /* +0x78 */
    PuzzleAnimation* workthecrowd_loop; /* +0x7C */
    PuzzleAnimation* go_get_him; /* +0x80 */
    PuzzleAnimation* laugh_start; /* +0x84 */
    PuzzleAnimation* laugh_loop; /* +0x88 */
    PuzzleAnimation* laugh_end; /* +0x8C */
    PuzzleAnimation* dont_get_me; /* +0x90 */
    PuzzleAnimation* taunt1; /* +0x94 */
    PuzzleAnimation* taunt2; /* +0x98 */
    PuzzleAnimation* taunt3; /* +0x9C */
    char pad0A0[0x48];
    PuzzleAnimation* fast_look; /* +0xE8 */
    PuzzleAnimation* one_arm_swing; /* +0xEC */
    char pad0F0[0x18];
    PuzzleAnimation* dizzyfall_holdface; /* +0x108 */
    char pad10C[0x18];
    PuzzleAnimation* shaking; /* +0x124 */
    char pad128[4];
    PuzzleAnimation* wipe_blood; /* +0x12C */
    PuzzleAnimation* disgusted_with_grinding; /* +0x130 */
    PuzzleAnimation* round_ground_pound; /* +0x134 */
    PuzzleAnimation* round_whew; /* +0x138 */
    PuzzleAnimation* wtf2; /* +0x13C */
    PuzzleAnimation* wtf; /* +0x140 */
    char pad144[0x0C];
    PuzzleAnimation* backflip_point; /* +0x150 */
    char pad154[4];
    PuzzleAnimation* almost_in_grinder; /* +0x158 */
    PuzzleAnimation* beg_start; /* +0x15C */
    PuzzleAnimation* beg_loop; /* +0x160 */
    PuzzleAnimation* beg_end; /* +0x164 */
    PuzzleAnimation* round_failure; /* +0x168 */
    char pad16C[4];
    PuzzleAnimation* bow_warmup; /* +0x170 */
    char pad174[4];
    PuzzleAnimation* happy_start; /* +0x178 */
    PuzzleAnimation* happy_loop; /* +0x17C */
    PuzzleAnimation* happy_end; /* +0x180 */
    char pad184[0x14];
    PuzzleAnimation* one_arm_victory_start; /* +0x198 */
    PuzzleAnimation* one_arm_victory_loop; /* +0x19C */
    char pad1A0[4];
    PuzzleAnimation* active_warmup1; /* +0x1A4 */
    PuzzleAnimation* active_warmup2; /* +0x1A8 */
    PuzzleAnimation* showoff_warmup1; /* +0x1AC */
    PuzzleAnimation* showoff_warmup2; /* +0x1B0 */
    char pad1B4[8];
    PuzzleAnimation* superman; /* +0x1BC */
    char pad1C0[8];
    PuzzleAnimation* propell_start; /* +0x1C8 */
    PuzzleAnimation* propell_air; /* +0x1CC */
    PuzzleAnimation* propell_end; /* +0x1D0 */
    char pad1D4[0x1C];
    PuzzleAnimation* peak; /* +0x1F0 */
    char pad1F4[0x3C];
    PuzzleAnimation* round_victory; /* +0x230 */
} PuzzleSharedAnimations;

typedef struct PuzzleRegisteredMove {
    int script_move;
    unsigned int chance;
    unsigned int conditions;
} PuzzleRegisteredMove;

typedef struct PuzzleCharacterMoveTable {
    unsigned int count;
    PuzzleRegisteredMove moves[15];
} PuzzleCharacterMoveTable; /* 0xB8 */

typedef struct PuzzleFighterMoveTables {
    unsigned int common_count;
    int common_moves[15];
    PuzzleCharacterMoveTable characters[14];
} PuzzleFighterMoveTables; /* 0xA50 */

typedef struct PuzzleSpacingChoice {
    PuzzleMoveEntry entry;
    unsigned int threshold;
    unsigned int unused;
} PuzzleSpacingChoice;
typedef struct PuzzleSpacingTable {
    unsigned int count;
    PuzzleSpacingChoice choices[15];
} PuzzleSpacingTable;

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

extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleProcess* aproc;
extern PuzzleProcess* plyr_anim_proc;
PuzzleProjectile* g_global_projectile;
extern PuzzleFighterObject* plyr_obj;
extern PuzzleFighterObject* his_obj;
extern PuzzleAnimPdata* plyr_anim_pdata;
extern PlyrPdata* his_pdata;
extern PuzzleSharedAnimations pz_shared_ani;
extern PuzzleSharedCombatAnimations shared_ani;
extern ScriptSlot* pz_shared_cmo;
extern PuzzleCmdScript* active_cmdscript;
extern void* apdata;
extern float _mkproc_sleep_ticks;
int g_pz_cam_already_shaking;
extern int exec_tick_ctr;
extern PuzzleFighterMoveTables g_pz_fighter_tables;

float pz_fighter_laugh(void);
float pz_fighter_whatever2(void);
static float pz_fighter_beg(void);
static float pz_fighter_workthecrowd(void);
static float pz_fighter_select_taunt_1(void);
static float pz_fighter_select_taunt_2(void);
static float pz_fighter_select_taunt_3(void);
static float pz_fighter_peak(void);
void pz_fighter_shaking(void);
static float pz_fighter_fast_look(void);
float pz_fighter_big_time_happy(void);
float pz_fighter_round_whew(void);
float pz_fighter_WTF(void);
float pz_fighter_WTF2(void);
static float pz_fighter_execute_distracts_and_hit(void);
float pz_fighter_execute_point_reaction_no_space(void);
static float pz_fighter_far_propell(void);
static float pz_fighter_go_get_him(void);
static float pz_fighter_one_arm_swing(void);
static float pz_fighter_gaydance(void);
static float pz_fighter_dont_get_me(void);
float pz_fighter_light_propell(void);
float pz_fighter_superman_move(void);
static float pz_fighter_propell(void);
float r_pz_call_script_function(void);
static float r_call_player_script_function(void);
static float r_call_character_cmo_function(void);
static float r_call_other_pz_player_char_script_function(void);
float pz_fighter_one_arm_victory(void);
float pz_fighter_one_arm_victory2(void);
static float pz_fighter_double_arm_victory(void);
void p_anim_idle(void);
void set_my_state(int state);
float p_plyr_pz_fighter_entry(void);
float pz_fighter_exit(void);
float pz_fighter_long_exit(void);
static float pz_fighter_shove_brush_back(void);
static float pz_fighter_uppercut_brush_back(void);
float j_exit(void);
float j_exit_6(void);
void face_opponent_now(void);
void head_tracking_off(void);
void head_tracking_on(void);
void cmdscript_reset_stack(void);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int function);
float call_player_script_function(ScriptSlot* slot);
void toggle_obj_and_ani_flips(PuzzleAnimPdata* animation);
void release_other_player(void);
void pz_fighter_reaction_xfer_him(int reaction);
static float p_force_reaction(void);
static float p_pz_shake_camera(void);
void* _create_mkproc_generic_tinystack(
    int pid, int priority, float (*entry)(void), int pdata_size,
    void* pdata_out);
void xfer_proc();
void set_ani_weight(float weight);
void blend_to_ani(PuzzleAnimation* animation, int flags, float blend);
void set_ani_speed(float speed);
void stop_me();
void avoid_double_ani(void);
void random_foot(int type);
void snd_major_hit_voice(void);
void myvel_my_angle_y(float angle, float x_velocity, float z_velocity);
void blend_to_ani_INOUT(
    PuzzleAnimation* animation, PuzzleAnimation* next, float blend,
    float in_weight, float out_weight);
float fpick_a_float(float first, float second);
void land_chores(int sound, int voice, float shake, float strength);
float j_getup_back_9(void);
float j_getup_back_6(void);
void launch_me_up(float vertical_velocity, float gravity);
void wait_to_land(void);
void bulvan_function(int enabled);
void wall_eligible_on(void);
int pz_fighter_should_he_breakout(void);
void tightrope_restrictions_off(void);
void transition_to_anim_script(
    PuzzleAnimPdata* animation, PuzzleAnimation* script, int flags,
    float blend);
void ani_to_frame_x_call(void (*callback)(void), float frame);
void advance_anim(PuzzleAnimPdata* animation);
void pose_anim(PuzzleAnimPdata* animation, int update_object);
void shake_hit_voice(float strength, int flags, int voice, int group);
void pan_snd_req(int sound, float pan);
void snd_req_delay(int sound, int delay);
void pz_fighter_get_grinder_post(int player, Vec* post);
void bgnd_launch_fx_at_position(
    const char* effect, float x, float y, float z);
void bgnd_set_fx_ang_y(float angle);
void get_bone_world_pos(
    PuzzleFighterObject* object, int bone, Vec* position);
void fx_reset(unsigned int effect);
void set_my_secondary_state(int state);
void set_block_requirement(int requirement);
void snd_req_vol(int sound, float volume);
PuzzleProcess* start_scorpion_spear(int field_34);
void ani_x_more_frames(float frames);
void play_sound_1(int sound);
void blend_to_fstance(float blend);
void hide_obj(PuzzleFighterObject* object);
void unhide_obj(PuzzleFighterObject* object);
void update_mkobj(PuzzleFighterObject* object);
unsigned int fx_by_owner(const char* name, int owner);
MkPfx* pfx_from_handle(unsigned int effect);
void pfx_bind_render_to_obj(PuzzleFighterObject* object, int bone);
void resume_effect(const char* name);
void swap_active_plyr_proc();
void snd_stop(MslSoundHandle sound);
void unfreeze_player(void);
void run_reaction_cleanup_function(PlyrPdata* pdata);
void xfer_player_proc(PuzzleProcess* proc, PuzzleMoveEntry entry);
extern void (*large_ground_fx)(void);
void init_ground_move_no_aniproc(void);
void init_ground_move(void);
void init_air_move(void);
void ani_loop_more_frames(float frames);
void ani_1_frame(void);
void pz_fighter_set_y_constrain(
    PuzzleFighterObject* fighter, int enabled, float height);
void pz_fighter_dont_fudge_desired_distance(void);
void pz_fighter_startup_attack(
    PuzzleAnimation* animation, unsigned int start_flags,
    unsigned int attack_flags, int blend_flags, int reaction,
    float attack_frame, float blend, float speed, float force, float damping);
void player_feet_land_chores(void);
void random_hit(int sound);
void random_voice(int sound);
void snd_req(int sound);
void glitch_to_ani(PuzzleAnimation* animation, int frame);
void p_animate(void);
float pz_fighter_ani_attack(
    int attack, unsigned int reaction, float active_frame, float hit_frame,
    float damage);
void set_both_face_opponent_flags(void);
void got_hit_fx(
    int type, int bone, int strength, int flags, int blood, int sound,
    float scale);
void myvel_his_angle_y(float y, float x, float z);
void init_air_move_no_aniproc(void);
void update_bone_hierarchy(void* object);
void ground_me(void* object);
void rotate_towards_him(float rate);
int get_his_attack_counter(void);
void force_forward(
    int duration, int interval, PuzzleAnimPdata* animation,
    float velocity, float damping);
void nudge_towards_him(float distance);
void ani_to_blend_frame(float frame);
void ani_to_frame_x(float frame);
void blend_to_stance(float blend);
void plyr_bleed_medium_cycle(PlyrPdata* pdata, int bone);
void force_away(
    int duration, int interval, float velocity, float damping);
void pz_fighter_attack(
    PuzzleAnimation* animation, PuzzleAttackParameters* attack, int reaction);
PuzzleFighterMove* pz_get_fighter_move(void);
void slow_ani_x(float speed, float frame);
void ani_to_end(void);
void pz_fighter_check_breakout(void);
float xz_distance_between_players(void);
int pz_fighter_close_enough_to_super_move(unsigned int player);
int pz_fighter_is_winning_big(unsigned int player);
int pz_fighter_is_losing_big(unsigned int player);
PuzzleProcess* get_player_proc(PuzzleFighterObject* fighter);
float pz_fighter_fetch_plyr_to_home_post_distance(int player);
void pz_fighters_calc_distance_to_desired_idle_pos_abs(
    float* player1_distance, float* player2_distance,
    float* player1_absolute, float* player2_absolute);
void pz_fighters_calc_distance_to_desired_idle_pos(
    float* player1_distance, float* player2_distance);
unsigned int randu0(unsigned int max);
void shake_camera(int duration, float strength);
void minigame_get_bgnd_y_value(int* first, int* second);
void minigame_set_bgnd_y_value(int first, int second);
static float pz_fighter_present_explode(void);
static float pz_fighter_present_given(void);
static float pz_fighter_present_on_attackers_hand(void);
static float p_present_control(void);
static float p_pz_fighter_projectile_launcher(void);
static float pz_fighter_scorpion_attack_start(void);
static float pz_fighter_jax_attack_start(void);
static float pz_fighter_r_null(void);
static float r_pz_ermac_slam(void);
static float r_pz_fighter_spear_tug(void);
static float r_pz_fighter_spear_hit(void);
static float r_pz_fighter_almost_in_grinder(void);
static float r_pz_fighter_feet3_swept_out(void);
static float r_pz_fighter_dizzyfall3_with_holdface(void);
static float r_pz_fighter_block_lo(void);
static float r_pz_fighter_block_hi(void);
float r_pz_fighter_grinding(void);
float r_pz_fighter_rx_get_to_point(void);

static const PuzzleReactionTransferEntry tbl_xfer_addresses[] = {
    { 4, (PuzzleMoveEntry)0x39, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3A, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3C, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3F, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3E, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_block_hi, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_block_lo, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x44, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x45, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x46, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x48, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x49, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x4A, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x4C, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x4D, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x4E, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x4B, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x50, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x51, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x52, { 0, 0 }, 0x12 },
    { 4, (PuzzleMoveEntry)0x4F, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_grinding, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x54, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x55, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x58, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x57, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_feet3_swept_out, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x59, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x5A, { 0, 0 }, 0x12 },
    { 4, (PuzzleMoveEntry)0x47, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_dizzyfall3_with_holdface, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x43, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x56, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_almost_in_grinder, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_spear_hit, { 0, 0 }, 0x12 },
    { 1, r_pz_fighter_spear_tug, { 0, 0 }, 0x12 },
    { 4, (PuzzleMoveEntry)0x53, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3B, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x42, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0xB, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x9, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0xA, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0xC, { 0, 0 }, 0x2 },
    { 4, (PuzzleMoveEntry)0x41, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x40, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x38, { 0, 0 }, 0x1 },
    { 4, (PuzzleMoveEntry)0x3D, { 0, 0 }, 0x1 },
    { 1, r_pz_ermac_slam, { 0, 0 }, 0x32 },
    { 3, (PuzzleMoveEntry)0x12, { 0, 0 }, 0x12 },
    { 3, (PuzzleMoveEntry)0x11, { 0, 0 }, 0x12 },
    { 3, (PuzzleMoveEntry)0x18, { 0, 0 }, 0x12 },
    { 3, (PuzzleMoveEntry)0xE, { 0, 0 }, 0x42 },
    { 3, (PuzzleMoveEntry)0x14, { 0, 0 }, 0x12 },
    { 3, (PuzzleMoveEntry)0x19, { 0, 0 }, 0x12 },
    { 1, pz_fighter_r_null, { 0, 0 }, 0x1 },
    { 3, (PuzzleMoveEntry)0x15, { 0, 0 }, 0x1 },
    { 1, r_pz_fighter_rx_get_to_point, { 0, 0 }, 0x1 },
};

static inline const PuzzleReactionTransferEntry* reaction_transfer_at_offset(
    unsigned int offset) {
    return (const PuzzleReactionTransferEntry*)
        ((const unsigned char*)tbl_xfer_addresses + offset);
}

static const PuzzleAttackCopy pz_attack_uppercut = {{
    9.0f, 0.1f, 0.9f, 0x00010000, 0x00060008, 3,
    0.2f, 0.6f, 0.85f, 14.0f, 11.0f, 0, 1, 0, 0,
}};
static const PuzzleAttackCopy pz_attack_shove = {{
    45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
    0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
}};
static const PuzzleAttackCopy pz_attack_common = {{
    45.0f, 0.1f, 3.2f, 0x00010001, 0x00070007, 3,
    0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 0, 1, 0, 0,
}};
static const PuzzleAttackCopy pz_attack_dizzy_punch = {{
    9.0f, 0.1f, 1.15f, 0x00010001, 0x00070007, 3,
    0.2f, 0.75f, 0.92f, 13.0f, 11.0f, 1, 1, 0, 0,
}};
static const PuzzleAttackCopy pz_attack_showoff_punch = {{
    9.0f, 0.1f, 1.15f, 0x00010001, 0x00070007, 3,
    0.2f, 0.75f, 0.92f, 13.0f, 11.0f, 1, 1, 0, 0,
}};
static const PuzzleAttackCopy pz_attack_footstomp = {{
    27.0f, 0.1f, 0.85f, 0x00120000, 0x00120007, 3,
    0.2f, 0.6f, 0.8f, 33.0f, 18.0f, 1, 0, 2, 1,
}};

PuzzleFighterFunction pz_fighter_tbl[3] = {
    pz_fighter_present_on_attackers_hand,
    pz_fighter_present_given,
    pz_fighter_present_explode,
};

static PuzzleSpacingTable pz_spacing_table = {
    2,
    {
        {pz_fighter_uppercut_brush_back, 30, 0},
        {pz_fighter_shove_brush_back, 100, 0},
    },
};

static inline void pz_fighter_create_projectile(
    PuzzleProjectile** projectile_out) {
    PuzzleProjectile* projectile = 0;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_pz_fighter_projectile_launcher,
            sizeof(PuzzleProjectile), &projectile) != 0 &&
        projectile != 0) {
        projectile->object =
            g_pz_fighters_engine.projectile_objects[plyr_pdata->plyr_num];
        projectile->object->flags_08 |= 0x40;
        projectile->object->flags_08 |= 0x20;
        projectile->object->external_force_x = 0.0f;
        projectile->object->vertical_velocity = 0.0f;
        projectile->object->external_force_z = 0.0f;
        projectile->object->x = 0.0f;
        projectile->object->y = 0.0f;
        projectile->object->z = 0.0f;
        projectile->launch_immediately = 0;
        projectile->state = 0;
        projectile->timer = 5000;
        projectile->launch_bone_owner = plyr_obj;
        projectile->target = his_obj;
        projectile->owner = plyr_pdata;
        projectile->opponent_pdata = his_pdata;
        if (projectile->owner->character_id != 6) {
            projectile->effect = fx_by_owner("fireball_fx", 4);
        } else {
            projectile->effect = fx_by_owner("green_fireball_fx", 4);
        }
        fx_reset(projectile->effect);
        pfx_from_handle(projectile->effect);
        pfx_bind_render_to_obj(projectile->object, 0);
        if (projectile->owner->character_id != 6) {
            resume_effect("fireball_fx");
        } else {
            resume_effect("green_fireball_fx");
        }
        g_global_projectile = projectile;
    }
    *projectile_out = projectile;
}

static inline int select_scripted_move(
    unsigned int move_index,
    int distance_class,
    unsigned short roll) {
    const PuzzleCharacterMoveTable* table =
        &g_pz_fighter_tables.characters[move_index];
    unsigned int index;

    for (index = 0; index < table->count; index++) {
        if (roll < table->moves[index].chance &&
            (table->moves[index].conditions & distance_class) != 0) {
            return table->moves[index].script_move;
        }
    }

    for (index = 0; index < table->count; index++) {
        if ((table->moves[index].conditions & 1) != 0) {
            return table->moves[index].script_move;
        }
    }
    return table->moves[0].script_move;
}

int pz_fighter_should_handle_special_move(unsigned int player, unsigned int move) {
    if (move == 4) {
        g_pz_fighters_engine.flag_bits.special_move_4 = 1;
    } else if (move == 5) {
        g_pz_fighters_engine.flag_bits.special_move_5 = 1;
    } else if (move == 1) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: 99.94% - jump-table relocation label only. */
float pz_fighter_perform_special_move(void) {
    switch (plyr_pdata->character_id) {
    case 0:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            aproc->vtbl->transfer(pz_fighter_scorpion_attack_start, 0.0f);
            return 0.0f;
        }
        break;
    case 12:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x12;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 19:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            aproc->vtbl->transfer(pz_fighter_jax_attack_start, 0.0f);
            return 0.0f;
        }
        break;
    case 8:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x11;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 4:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x0F;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 5:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x17;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 10:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x0F;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 1:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x0D;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 3:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x13;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 23:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x18;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 21:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x16;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 29:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            active_cmdscript->function = 0x14;
            aproc->vtbl->transfer(r_call_character_cmo_function, 0.0f);
            return 0.0f;
        }
        break;
    case 2:
    case 6:
    case 7:
    case 9:
    case 11:
    case 33:
        if (g_pz_fighters_engine.special_move_enabled == 1) {
            aproc->vtbl->transfer(pz_fighter_jax_attack_start, 0.0f);
            return 0.0f;
        }
        break;
    }

    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_finish_him_request ~98.46% - emit-order island. */
float pz_finish_him_request(void) {
    if (xz_distance_between_players() > 2.0f) {
        aproc->vtbl->transfer(pz_fighter_far_propell, 0.0f);
    } else {
        active_cmdscript->function = 0x2B;
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            pz_shared_cmo, active_cmdscript->function);
        call_player_script_function(pz_shared_cmo);
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: 98.08% -- constant-branch and relocation emission remains. */
static float pz_fighter_scorpion_attack_start(void) {
    float player_distance = xz_distance_between_players();
    float home_distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);
    PuzzleProcess* spear_proc;

    if (player_distance <= 3.0f) {
        if (home_distance > 6.0f) {
            PuzzleAttackCopy attack;
            PuzzleFighterMove* move;

            attack = pz_attack_common;
            move = pz_get_fighter_move();
            move->active_flags |= 1;
            pz_fighter_attack(pz_shared_ani.shove, &attack.attack, 0x24);
            ani_to_end();
        } else if (home_distance < 4.0f) {
            avoid_double_ani();
            init_ground_move_no_aniproc();
            random_voice(9);
            set_my_state(0x6208);
            plyr_anim_pdata->flags |= 0x40;
            snd_req(0xD71);
            blend_to_ani(shared_ani.dash_back, 3, 0.2f);
            plyr_anim_pdata->step = 0.75f;
            plyr_anim_pdata->field_68 = 0.0f;
            plyr_anim_pdata->field_64 = 1.8f;
            ani_to_frame_x(12.0f);
            init_air_move();
            ani_to_frame_x(17.0f);
            init_ground_move();
            random_foot(1);
        } else {
            avoid_double_ani();
            init_ground_move_no_aniproc();
            random_voice(9);
            set_my_state(0x6208);
            plyr_anim_pdata->flags |= 0x40;
            snd_req(0xD71);
            blend_to_ani(shared_ani.dash_back, 3, 0.2f);
            plyr_anim_pdata->step = 0.9f;
            plyr_anim_pdata->field_68 = 0.0f;
            plyr_anim_pdata->field_64 = 1.0f;
            ani_to_frame_x(12.0f);
            init_air_move();
            ani_to_frame_x(17.0f);
            init_ground_move();
            random_foot(1);
        }
    }

    set_my_secondary_state(0x101);
    set_block_requirement(0);
    plyr_pdata->saved_position_x = plyr_obj->x;
    plyr_pdata->saved_position_z = plyr_obj->z;
    plyr_pdata->duck_reaction_active = 1;
    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_start, 3, 0.1f);
    set_ani_speed(0.9f);
    ani_to_frame_x(12.0f);
    snd_req_vol(0x1AF2, 0.75f);
    ani_to_end();
    set_my_state(0xD200);
    spear_proc = start_scorpion_spear(9);
    plyr_pdata->spear_proc = (struct MkProc*)spear_proc;
    plyr_pdata->spear_proc_instance = spear_proc->instance;
    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_loop, 0, 0.1f);
    set_ani_speed(0.9f);
    ani_x_more_frames(10.0f);
    ani_to_end();
    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_end, 3, 0.1f);
    set_ani_speed(0.5f);
    play_sound_1(0x2D4);
    pz_fighter_reaction_xfer_him(0x23);
    ani_to_blend_frame(10.0f);
    blend_to_fstance(0.1f);
    toggle_obj_and_ani_flips(plyr_anim_pdata);
    active_cmdscript->function = 0x2B;
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        pz_shared_cmo, active_cmdscript->function);
    call_player_script_function(pz_shared_cmo);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 97.49% -- inline register and string-pool emission remains. */
static float pz_fighter_jax_attack_start(void) {
    float player_distance = xz_distance_between_players();
    float home_distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);
    PuzzleProjectile* projectile;
    unsigned int ticks;

    if (player_distance <= 3.0f) {
        if (home_distance > 6.0f) {
            PuzzleAttackCopy attack;
            PuzzleFighterMove* move;

            attack = pz_attack_common;
            move = pz_get_fighter_move();
            move->active_flags |= 1;
            pz_fighter_attack(pz_shared_ani.shove, &attack.attack, 0x24);
            ani_to_end();
        } else if (home_distance < 4.0f) {
            avoid_double_ani();
            init_ground_move_no_aniproc();
            random_voice(9);
            set_my_state(0x6208);
            plyr_anim_pdata->flags |= 0x40;
            snd_req(0xD71);
            blend_to_ani(shared_ani.dash_back, 3, 0.2f);
            plyr_anim_pdata->step = 0.75f;
            plyr_anim_pdata->field_68 = 0.0f;
            plyr_anim_pdata->field_64 = 1.8f;
            ani_to_frame_x(12.0f);
            init_air_move();
            ani_to_frame_x(17.0f);
            init_ground_move();
            random_foot(1);
        } else {
            avoid_double_ani();
            init_ground_move_no_aniproc();
            random_voice(9);
            set_my_state(0x6208);
            plyr_anim_pdata->flags |= 0x40;
            snd_req(0xD71);
            blend_to_ani(shared_ani.dash_back, 3, 0.2f);
            plyr_anim_pdata->step = 0.9f;
            plyr_anim_pdata->field_68 = 0.0f;
            plyr_anim_pdata->field_64 = 1.0f;
            ani_to_frame_x(12.0f);
            init_air_move();
            ani_to_frame_x(17.0f);
            init_ground_move();
            random_foot(1);
        }
    }

    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_start, 3, 0.3f);
    pz_fighter_create_projectile(&projectile);
    plyr_anim_pdata->step = 1.0f;
    ani_to_blend_frame(10.0f);
    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_loop, 0, 0.3f);
    set_ani_speed(0.3f);
    ticks = 0;
    do {
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if (g_pz_fighters_engine.flag_bits.special_move_4) {
            break;
        }
        ticks++;
    } while (ticks < 125);

    blend_to_ani(
        plyr_pdata->fighter_definition->spear_throw_end, 3, 0.3f);
    plyr_anim_pdata->step = 1.5f;
    ani_to_frame_x(21.0f);
    if (g_global_projectile == 0) {
        pz_fighter_create_projectile(&projectile);
    }
    g_global_projectile->launch_immediately = 1;
    ani_to_end();
    blend_to_ani(
        plyr_pdata->fighter_definition->projectile_return_loop, 0, 0.1f);
    ticks = 0;
    do {
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if (g_pz_fighters_engine.flag_bits.special_move_5) {
            break;
        }
        ticks++;
    } while (ticks < 125);
    blend_to_ani(
        plyr_pdata->fighter_definition->projectile_return_end, 3, 0.1f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.15f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighters_react_to_bomb_explosion(void) {
    if (pz_fighter_fetch_plyr_to_home_post_distance(
            ((PlyrPdata*)plyr_pdata)->plyr_num) >
        6.5f) {
        active_cmdscript->function = 0x44;
    } else {
        active_cmdscript->function = 0x45;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_ohyeah_move(void) {
    unsigned short choice = randu0(100);

    if (choice < 50) {
        aproc->vtbl->transfer(pz_fighter_one_arm_swing, 0.0f);
        return 0.0f;
    } else if (choice < 70) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
        return 0.0f;
    } else {
        aproc->vtbl->transfer(pz_fighter_gaydance, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_super_move_just_enabled(void) {
    unsigned short choice = randu0(100);

    if (choice < 50) {
        aproc->vtbl->transfer(pz_fighter_go_get_him, 0.0f);
        return 0.0f;
    } else if (choice < 60) {
        aproc->vtbl->transfer(pz_fighter_one_arm_swing, 0.0f);
        return 0.0f;
    } else if (choice < 75) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
        return 0.0f;
    } else {
        aproc->vtbl->transfer(pz_fighter_gaydance, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_other_guy_super_move_just_enabled(void) {
    aproc->vtbl->transfer(pz_fighter_dont_get_me, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_ohno_move(void) {
    unsigned short choice = randu0(100);

    if (choice < 30) {
        aproc->vtbl->transfer(pz_fighter_WTF, 0.0f);
        return 0.0f;
    } else if (choice < 70) {
        aproc->vtbl->transfer(pz_fighter_WTF2, 0.0f);
        return 0.0f;
    } else if (
        choice < 88 &&
        pz_fighter_is_losing_big(plyr_pdata->plyr_num) != 0) {
        aproc->vtbl->transfer(pz_fighter_beg, 0.0f);
        return 0.0f;
    } else {
        xfer_proc(
            get_player_proc(his_obj), pz_fighter_execute_distracts_and_hit);
        aproc->vtbl->transfer(
            pz_fighter_execute_point_reaction_no_space, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_center_pos_minor_adjustement(void) {
    if (xz_distance_between_players() > 1.2f) {
        aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
        return 0.0f;
    } else {
        active_cmdscript->function = 8;
        aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_center_pos_single_close_move(void) {
    aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
    return 0.0f;
}

float pz_fighter_smart_flippy(void) {
    float player1_distance;
    float player1_absolute;
    float player2_distance;
    float player2_absolute;
    float distance;

    pz_fighters_calc_distance_to_desired_idle_pos_abs(
        &player1_distance, &player2_distance, &player1_absolute,
        &player2_absolute);
    if (plyr_pdata->plyr_num == 0) {
        distance = player1_distance;
    } else {
        distance = player2_distance;
    }

    if (distance < 0.1f) {
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
        return 0.0f;
    }
    if (distance < 0.9f) {
        active_cmdscript->function = 4;
    } else {
        active_cmdscript->function = 3;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_center_pos_single_range_move(void) {
    if ((unsigned short)randu0(100) < 30) {
        aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
        return 0.0f;
    }

    active_cmdscript->function = 3;
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_center_pos_range_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 40) {
        aproc->vtbl->transfer(pz_fighter_superman_move, 0.0f);
        return 0.0f;
    } else if (choice < 55) {
        aproc->vtbl->transfer(pz_fighter_propell, 0.0f);
        return 0.0f;
    } else {
        active_cmdscript->function = 1;
        aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_dist_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 20) {
        active_cmdscript->function = 2;
    } else if (choice < 60) {
        active_cmdscript->function = 6;
    } else {
        active_cmdscript->function = 7;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_off_wall_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 40) {
        aproc->vtbl->transfer(pz_fighter_superman_move, 0.0f);
        return 0.0f;
    }
    if (choice < 80) {
        active_cmdscript->function = 1;
    } else {
        active_cmdscript->function = 2;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_random_taunt(void) {
    unsigned short roll = randu0(100);

    if (roll < 30) {
        aproc->vtbl->transfer(pz_fighter_select_taunt_1, 0.0f);
        return 0.0f;
    } else if (roll < 60) {
        aproc->vtbl->transfer(pz_fighter_select_taunt_2, 0.0f);
        return 0.0f;
    } else {
        aproc->vtbl->transfer(pz_fighter_select_taunt_3, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_other_guy_ohno(void) {
    aproc->vtbl->transfer(pz_fighter_laugh, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_other_guy_ohyeah(void) {
    aproc->vtbl->transfer(pz_fighter_whatever2, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_other_guy_holding_onto_super_move(void) {
    aproc->vtbl->transfer(pz_fighter_beg, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_holding_onto_super_move(void) {
    aproc->vtbl->transfer(pz_fighter_workthecrowd, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_peak_move(void) {
    if ((unsigned short)randu0(100) < 65) {
        aproc->vtbl->transfer(pz_fighter_peak, 0.0f);
        return 0.0f;
    } else {
        aproc->vtbl->transfer(pz_fighter_fast_look, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_perform_relief_move(void) {
    if ((unsigned short)randu0(100) < 65 &&
        pz_fighter_is_winning_big(plyr_pdata->plyr_num)) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
        return 0.0f;
    } else {
        aproc->vtbl->transfer(pz_fighter_round_whew, 0.0f);
        return 0.0f;
    }
}

float pz_fighter_dummy_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.65f) {
            plyr_pdata->collision_result = 1;
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_light_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.65f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x2B);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.75f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x29);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_far_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(30, 4, -0.065f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 30; ticks++) {
        if (xz_distance_between_players() <= 0.75f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x29);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: 98.88% at exact retail size. Both selection loops and the
 * script-transfer tail agree instruction-for-instruction; all remaining
 * objdiff entries are GPR allocation differences.
 */
float pz_fighter_perform_scripted_move(void) {
    PuzzleFighterMove* move = pz_get_fighter_move();
    int distance_class = move->distance_class;
    unsigned int move_index = move->script_move;
    unsigned short roll = randu0(100);
    int script_move =
        select_scripted_move(move_index, distance_class, roll);
    active_cmdscript->function = script_move;
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        pz_shared_cmo, active_cmdscript->function);
    call_player_script_function(pz_shared_cmo);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.66% - floating-constant relocation labels only. */
void pz_fighter_walk_FB_true(
    int (*continue_test)(void), unsigned int duration, int forward) {
    static float object_weight_setting = 0.2f;
    static float forward_speed = 1.0f;
    static float backward_speed = 1.0f;
    int start_tick;

    if (continue_test() != 0) {
        return;
    }
    start_tick = exec_tick_ctr;
    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    if (forward != 0) {
        set_my_state(0x2000);
        plyr_anim_pdata->flags |= 0x40;
        blend_to_ani(
            plyr_pdata->fighter_definition->walk_forward_start, 0x23, 0.2f);
        plyr_anim_pdata->step = forward_speed;
        plyr_anim_pdata->field_64 = object_weight_setting;
        ani_to_frame_x_call(
            face_opponent_now, plyr_anim_pdata->end_frame - 13.0f);
    } else {
        set_my_state(0x2001);
        plyr_anim_pdata->flags |= 0x40;
        blend_to_ani(
            plyr_pdata->fighter_definition->walk_backward_start, 0x23,
            0.2f);
        plyr_anim_pdata->step = backward_speed;
        plyr_anim_pdata->field_64 = object_weight_setting;
        ani_to_frame_x_call(
            face_opponent_now, plyr_anim_pdata->end_frame - 14.0f);
    }
    random_foot(1);
    if (continue_test() == 0 &&
        (unsigned int)(exec_tick_ctr - start_tick) < duration) {
        if (forward != 0) {
            blend_to_ani(
                plyr_pdata->fighter_definition->walk_forward_loop, 0, 0.2f);
            plyr_anim_pdata->step = forward_speed;
        } else {
            blend_to_ani(
                plyr_pdata->fighter_definition->walk_backward_loop, 0, 0.2f);
            plyr_anim_pdata->step = backward_speed;
        }
        while (continue_test() == 0 &&
               (unsigned int)(exec_tick_ctr - start_tick) < duration) {
            face_opponent_now();
            advance_anim(plyr_anim_pdata);
            pose_anim(plyr_anim_pdata, 1);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        blend_to_stance(0.1f);
        return;
    }
    ani_to_frame_x_call(
        face_opponent_now, plyr_anim_pdata->end_frame - 10.0f);
    plyr_anim_pdata->field_64 = object_weight_setting;
}

/* pz_fighter_shake_camera: 100% via typed out-pdata and spawn ownership. */
void pz_fighter_shake_camera(int duration, float strength) {
    PuzzleCameraShakePdata* pdata;

    shake_camera(duration, strength);
    if (g_pz_cam_already_shaking == 1) {
        return;
    }

    g_pz_cam_already_shaking = 1;
    if (_create_mkproc_generic_tinystack(
            0x1007, 0x1E, p_pz_shake_camera,
            sizeof(PuzzleCameraShakePdata), &pdata) != 0) {
        pdata->duration = duration;
        pdata->strength = strength;
    }
}

static float p_pz_shake_camera(void) {
    int i;
    PuzzleCameraShakePdata* pdata = apdata;
    int first;
    int second;
    int offset;

    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    minigame_get_bgnd_y_value(&first, &second);

    for (i = 0; i < pdata->duration; i++) {
        offset = (int)(340.0f * pdata->strength);
        minigame_set_bgnd_y_value(first + offset, second + offset);
        _mkproc_sleep_ticks = 3.0f;
        aproc->vtbl->sleep();
        minigame_set_bgnd_y_value(first, second);
        _mkproc_sleep_ticks = 3.0f;
        aproc->vtbl->sleep();
    }

    g_pz_cam_already_shaking = 0;
    return -1.0f;
}

/* Soft ceiling: 99.79% - threshold-constant relocation labels only. */
int pz_fighter_walk_until_danger_or_in_wrong_direction(void) {
    float player1_distance;
    float player2_distance;
    float distance;
    int state;

    pz_fighters_calc_distance_to_desired_idle_pos(
        &player1_distance, &player2_distance);
    if (plyr_pdata->plyr_num == 0) {
        distance = player1_distance;
    } else {
        distance = player2_distance;
    }
    if (distance > -0.01f && distance < 0.01f) {
        return 1;
    }
    if (g_pz_fighters_engine.breakout == 1) {
        return 1;
    }
    state = plyr_pdata->state;
    if (state == 0x2000 && distance < 0.0f) {
        return 1;
    }
    if (state == 0x2001 && distance > 0.0f) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: pz_fighter_walk_until_fight_distance ~99.58% - pool label only. */
int pz_fighter_walk_until_fight_distance(void) {
    return xz_distance_between_players() < 0.95f;
}

/*
 * Soft ceilings: pz_fighter_showoff_warmup2/1 and
 * pz_fighter_active_warmup2/1 ~99.29%; pz_fighter_bow_warmup ~99.39%.
 * The remaining deltas are float-pool symbol identities only.
 */
float pz_fighter_showoff_warmup2(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.showoff_warmup2, 3, 0.1f);
    set_ani_speed(0.15f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_showoff_warmup1(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.showoff_warmup1, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_active_warmup2(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.active_warmup2, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_active_warmup1(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.active_warmup1, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_bow_warmup(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.bow_warmup, 3, 0.1f);
    set_ani_speed(1.25f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.15f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void pz_fighter_kill_global_projectile(void) {
    if (g_global_projectile != 0) {
        g_global_projectile->state = 2;
    }
}

/* Near match: 98.53% - register allocation plus string-pool addressing. */
static float p_pz_fighter_projectile_launcher(void) {
    PuzzleProjectile* projectile = apdata;
    int passed_target = 0;
    Vec position;

    if ((unsigned int)projectile->state == 2) {
        g_global_projectile = 0;
        return -1.0f;
    }

    if (--projectile->timer != 0) {
        switch (projectile->state) {
        case 0: {
            RwFrame* frame = projectile->object->frame;

            get_bone_world_pos(
                projectile->launch_bone_owner, 0x1B, &position);
            projectile->object->x = position.x;
            projectile->object->y = position.y;
            projectile->object->z = position.z;
            if (projectile->launch_immediately == 1) {
                projectile->timer = 1;
            }
            RwFrameUpdateObjects(projectile->object->frame);
            frame->modelling.pos.x = projectile->object->x;
            frame->modelling.pos.y = projectile->object->y;
            frame->modelling.pos.z = projectile->object->z;
            RwFrameUpdateObjects(projectile->object->frame);
            break;
        }
        case 1: {
            RwFrame* frame = projectile->object->frame;

            RwFrameUpdateObjects(frame);
            projectile->object->x += projectile->object->external_force_x;
            projectile->object->y += projectile->object->vertical_velocity;
            projectile->object->z += projectile->object->external_force_z;
            frame->modelling.pos.x = projectile->object->x;
            frame->modelling.pos.y = projectile->object->y;
            frame->modelling.pos.z = projectile->object->z;
            RwFrameUpdateObjects(projectile->object->frame);
            get_bone_world_pos(projectile->target, 9, &position);
            if (projectile->object->external_force_x > 0.0f) {
                if (projectile->object->x > position.x) {
                    passed_target = 1;
                }
            } else if (projectile->object->x < position.x) {
                passed_target = 1;
            }
            if (passed_target == 1) {
                void* saved_pdata = apdata;

                apdata = projectile->owner;
                fx_reset(projectile->effect);
                if (projectile->owner->character_id != 6) {
                    bgnd_launch_fx_at_position(
                        "fireball_explosion_fx", projectile->target->x,
                        projectile->object->y, projectile->target->z);
                    bgnd_launch_fx_at_position(
                        "fireball_sparkies_fx", projectile->target->x,
                        projectile->object->y, projectile->target->z);
                } else {
                    bgnd_launch_fx_at_position(
                        "green_fireball_explosion_fx", projectile->target->x,
                        projectile->object->y, projectile->target->z);
                    bgnd_launch_fx_at_position(
                        "green_fireball_sparkies_fx", projectile->target->x,
                        projectile->object->y, projectile->target->z);
                }
                pz_fighter_reaction_xfer_him(4);
                apdata = saved_pdata;
                g_global_projectile = 0;
                return -1.0f;
            }
            break;
        }
        }
    } else {
        switch (projectile->state) {
        case 0:
            projectile->state = 1;
            projectile->object->external_force_x = 0.18f;
            if (projectile->owner->plyr_num == 1) {
                projectile->object->external_force_x *= -1.0f;
            }
            projectile->timer = 300;
            break;
        case 1:
            g_global_projectile = 0;
            return -1.0f;
        }
    }
    return 1.0f;
}

/* Soft ceiling: pz_fighter_completely_prone ~97.50% - pool label only. */
float pz_fighter_completely_prone(void) {
    return 1.0f;
}

/* Soft ceiling: pz_fighter_won2 ~99.67% - float pool labels only. */
float pz_fighter_won2(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.won2, flags, 0.1f);
    set_ani_speed(0.65f);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    ani_to_frame_x(131.0f);
    aproc->vtbl->transfer(pz_fighter_one_arm_victory, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_wipe_blood_off ~99.60% - float pool labels only. */
float pz_fighter_wipe_blood_off(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    blend_to_ani(pz_shared_ani.wipe_blood, flags, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(22.0f);
    plyr_bleed_medium_cycle(plyr_pdata, 9);
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x1A);
    ani_to_frame_x(80.0f);
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x19);
    ani_to_frame_x(128.0f);
    aproc->vtbl->transfer(pz_fighter_double_arm_victory, 0.0f);
    return 0.0f;
}

void pz_fighter_wipe_blood_off_hands(void) {
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x19);
}

/* Soft ceiling: pz_fighter_double_arm_victory ~99.63% - float pool labels only. */
static float pz_fighter_double_arm_victory(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }

    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.double_arm_victory, flags, 0.05f);
    set_ani_speed(0.5f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Exact: typed force_away arguments preserve the retail float ABI. */
float pz_fighter_whatever2(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    force_away(5, 5, 0.01f, 0.9f);
    head_tracking_on();
    blend_to_ani(pz_shared_ani.whatever2, flags, 0.1f);
    set_ani_speed(0.7f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_disgusted_with_grinding ~99.65% - float pool labels only. */
float pz_fighter_disgusted_with_grinding(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.disgusted_with_grinding, flags, 0.1f);
    set_ani_speed(0.75f);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    ani_to_frame_x(163.0f);
    aproc->vtbl->transfer(pz_fighter_one_arm_victory2, 0.0f);
    return 0.0f;
}

/* Soft ceilings: one-arm victory variants 99.53% - float relocations only. */
float pz_fighter_one_arm_victory2(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    blend_to_ani(pz_shared_ani.one_arm_victory_start, flags, 0.1f);
    set_ani_speed(0.5f);
    ani_to_frame_x(91.0f);
    flags = 0;
    plyr_obj->presentation_flags.unk_bit1 = 0;
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.one_arm_victory_loop, flags, 0.5f);
    set_ani_speed(0.5f);
    ani_loop_more_frames(1000.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_one_arm_victory(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    blend_to_ani(pz_shared_ani.one_arm_victory_start, flags, 0.2f);
    set_ani_speed(0.75f);
    ani_to_frame_x(91.0f);
    flags = 0;
    plyr_obj->presentation_flags.unk_bit1 = 0;
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.one_arm_victory_loop, flags, 0.5f);
    set_ani_speed(0.75f);
    ani_loop_more_frames(1000.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_ground_pound(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    pz_fighter_set_y_constrain(plyr_obj, 1, 0.1f);
    blend_to_ani(pz_shared_ani.round_ground_pound, flags, 0.1f);
    set_ani_speed(0.65f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_WTF2(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    blend_to_ani(pz_shared_ani.wtf2, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_gaydance(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.gaydance, flags, 0.1f);
    set_ani_speed(1.35f);
    ani_to_frame_x(2.0f);
    init_air_move();
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_dont_get_me(void) {
    int flags = 3;

    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.dont_get_me, flags, 0.1f);
    set_ani_speed(0.75f);
    ani_to_blend_frame(20.0f);
    blend_to_stance(0.2f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_go_get_him(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.go_get_him, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_one_arm_swing(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.one_arm_swing, flags, 0.1f);
    set_ani_speed(1.45f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_select_taunt_3(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt3, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_select_taunt_2(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt2, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_select_taunt_1(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt1, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_WTF(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    blend_to_ani(pz_shared_ani.wtf, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_failure(void) {
    unsigned int frame;

    head_tracking_off();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.round_failure, 3, 0.1f);
    set_ani_speed(0.75f);
    for (frame = 0; frame < 32; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += 0.049087387f;
        } else {
            plyr_obj->angle_y -= 0.049087387f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_to_frame_x(154.0f);
    for (frame = 0; frame < 16; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += -0.09817477f;
        } else {
            plyr_obj->angle_y -= -0.09817477f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_workthecrowd(void) {
    int flags = 0;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags = 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.workthecrowd_start, flags, 0.05f);
    set_ani_speed(0.8f);
    blend_to_ani(pz_shared_ani.workthecrowd_loop, flags, 0.1f);
    ani_loop_more_frames(220.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/*
 * Near-match looped-celebration family: retail 0x114/current 0x110 and
 * 92.72% each. m2c confirms identical animation, loop, gravity and transfer
 * operations; the four-byte residue is the final bitfield-value lifetime and
 * equivalent epilogue scheduling. A separate loop-only flags snapshot was
 * semantically valid but MWCC coalesced it completely, so it was removed.
 */
static float pz_fighter_beg(void) {
    int flags = 3;
    unsigned int loop;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->gravity_enabled = 0;
    blend_to_ani(pz_shared_ani.beg_start, flags, 0.1f);
    set_ani_speed(0.8f);
    loop = 0;
    do {
        blend_to_ani(pz_shared_ani.beg_loop, flags, 0.1f);
        ani_to_end();
        loop++;
    } while (loop < 2);
    blend_to_ani(pz_shared_ani.beg_end, flags, 0.1f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    plyr_obj->gravity_enabled = 1;
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.53% - floating-constant relocation labels only. */
float pz_fighter_laugh_small(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->gravity_enabled = 0;
    blend_to_ani(pz_shared_ani.laugh_start, flags, 0.1f);
    set_ani_speed(0.8f);
    blend_to_ani(pz_shared_ani.laugh_loop, flags, 0.1f);
    ani_to_end();
    blend_to_ani(pz_shared_ani.laugh_end, flags, 0.1f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    plyr_obj->gravity_enabled = 1;
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* See the looped-celebration near-match note on pz_fighter_beg. */
float pz_fighter_laugh(void) {
    int flags = 3;
    unsigned int loop;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->gravity_enabled = 0;
    blend_to_ani(pz_shared_ani.laugh_start, flags, 0.1f);
    set_ani_speed(0.8f);
    loop = 0;
    do {
        blend_to_ani(pz_shared_ani.laugh_loop, flags, 0.1f);
        ani_to_end();
        loop++;
    } while (loop < 2);
    blend_to_ani(pz_shared_ani.laugh_end, flags, 0.1f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    plyr_obj->gravity_enabled = 1;
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* See the looped-celebration near-match note on pz_fighter_beg. */
float pz_fighter_big_time_happy(void) {
    int flags = 3;
    unsigned int loop;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->gravity_enabled = 0;
    blend_to_ani(pz_shared_ani.happy_start, flags, 0.1f);
    set_ani_speed(0.8f);
    loop = 0;
    do {
        blend_to_ani(pz_shared_ani.happy_loop, flags, 0.1f);
        ani_to_end();
        loop++;
    } while (loop < 3);
    blend_to_ani(pz_shared_ani.happy_end, flags, 0.1f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    plyr_obj->gravity_enabled = 1;
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_fast_look(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.fast_look, flags, 0.33f);
    set_ani_speed(2.15f);
    ani_to_blend_frame(5.0f);
    blend_to_stance(0.2f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float pz_fighter_peak(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.peak, flags, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(46.0f);
    g_pz_fighters_engine.peak_active = 1;
    g_pz_fighters_engine.peak_mode = 2;
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_whew(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.round_whew, flags, 0.1f);
    set_ani_speed(0.55f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_victory(void) {
    unsigned int frame;

    xfer_proc(plyr_anim_proc, p_anim_idle);
    head_tracking_off();
    blend_to_ani(pz_shared_ani.round_victory, 3, 0.1f);
    set_ani_speed(0.75f);
    for (frame = 0; frame < 32; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += 0.049087387f;
        } else {
            plyr_obj->angle_y -= 0.049087387f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_to_frame_x(184.0f);
    for (frame = 0; frame < 16; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += -0.09817477f;
        } else {
            plyr_obj->angle_y -= -0.09817477f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Exact match: direct table indexing preserves retail's updating base load. */
void pz_fighter_register_move(
    int move, int table, int parameter1, int parameter2, int character) {
    unsigned int index;

    switch (table) {
    case 0:
        if (g_pz_fighter_tables.common_count < 15) {
            g_pz_fighter_tables
                .common_moves[g_pz_fighter_tables.common_count] = move;
            g_pz_fighter_tables.common_count++;
        }
        return;
    case 1:
        index = g_pz_fighter_tables.characters[character].count;
        if (index < 15) {
            g_pz_fighter_tables.characters[character]
                .moves[index].script_move = move;
            g_pz_fighter_tables.characters[character]
                .moves[g_pz_fighter_tables.characters[character].count]
                .chance = parameter1;
            g_pz_fighter_tables.characters[character]
                .moves[g_pz_fighter_tables.characters[character].count]
                .conditions = parameter2;
            g_pz_fighter_tables.characters[character].count++;
        }
        return;
    }
}

float pz_fighter_give_present(void) {
    PuzzleReactionDelayPdata* pdata;

    head_tracking_off();
    init_ground_move();
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_force_reaction,
            sizeof(PuzzleReactionDelayPdata), &pdata) != 0 &&
        pdata != 0) {
        pdata->ticks = 30;
        pdata->reaction = 0x2D;
        pdata->saved_pdata = apdata;
    }
    active_cmdscript->function = 0x37;
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        pz_shared_cmo, active_cmdscript->function);
    call_player_script_function(pz_shared_cmo);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_footstomp(void) {
    PuzzleAttackCopy attack;
    PuzzleReactionDelayPdata* pdata;

    attack = pz_attack_footstomp;
    init_ground_move();
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_force_reaction,
            sizeof(PuzzleReactionDelayPdata), &pdata) != 0 &&
        pdata != 0) {
        pdata->ticks = 22;
        pdata->reaction = 0x20;
        pdata->saved_pdata = apdata;
    }
    pz_fighter_attack(pz_shared_ani.footstomp, &attack.attack, 0x20);
    ani_to_frame_x(34.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_punch_dizzyfall(void) {
    PuzzleAttackParameters attack = pz_attack_dizzy_punch.attack;

    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.dizzy_punch, &attack, 0x11);
    pz_fighter_check_breakout();
    ani_to_frame_x(22.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_back_and_forth_showoff(void) {
    PuzzleAttackParameters attack = pz_attack_showoff_punch.attack;

    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.dizzy_punch, &attack, 0x1D);
    pz_fighter_check_breakout();
    ani_to_frame_x(22.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_distance_check_wo_super_check ~99.69% - pool label only. */
int pz_fighter_distance_check_wo_super_check(void) {
    float distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);

    if (distance < 2.45f) {
        return 1;
    }
    return 2;
}

/* Soft ceiling: pz_fighter_distance_check ~99.82% - pool label only. */
int pz_fighter_distance_check(void) {
    float distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);

    if (pz_fighter_close_enough_to_super_move(his_pdata->plyr_num) == 1) {
        return 0;
    }
    if (distance < 2.45f) {
        return 1;
    }
    return 2;
}

#define PZ_RUN_SHARED_FIGHTER_SCRIPT(script_index)                              \
    do {                                                                        \
        active_cmdscript->function = (script_index);                     \
        cmdscript_reset_stack();                                                \
        cmdscript_setup_execution(pz_shared_cmo,                                \
                                  active_cmdscript->function);            \
        call_player_script_function(pz_shared_cmo);                             \
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);                           \
    } while (0)

float pz_fighter_execute_point_reaction_no_space(void) {
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x36);
    return 0.0f;
}

static float pz_fighter_execute_distracts_and_hit(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x32);
    return 0.0f;
}

float pz_fighter_execute_R_coming_down(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x34);
    return 0.0f;
}

float pz_fighter_execute_point_no_space_check(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x33);
    return 0.0f;
}

float pz_fighter_execute_point(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x31);
    return 0.0f;
}

float pz_fighter_perform_taunt(void) {
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x30);
    return 0.0f;
}

#undef PZ_RUN_SHARED_FIGHTER_SCRIPT

/* Soft ceiling: 99.43% - animation-constant relocation labels only. */
void pz_fighter_shaking(void) {
    int flags = 0;
    int frame;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.shaking, flags, 0.1f);
    plyr_anim_pdata->step = 1.3f;
    ani_to_frame_x(15.0f);
    frame = 1;
    do {
        glitch_to_ani(pz_shared_ani.shaking, flags);
        ani_to_frame_x(15.0f);
        frame++;
    } while (frame < 6);
}

void pz_fighter_check_to_toggle_obj_and_ani_flips(
    unsigned int expected_player) {
    if ((unsigned int)plyr_pdata->plyr_num == expected_player) {
        toggle_obj_and_ani_flips(plyr_anim_pdata);
    }
}

void pz_fighter_release_other_player(int reaction) {
    int reaction_locked = 0;

    release_other_player();
    his_obj->action_flags.reaction_locked = reaction_locked;
    plyr_obj->action_flags.reaction_locked = reaction_locked;
    pz_fighter_reaction_xfer_him(reaction);
}

/* Near match: retail has no return value; only constant-pool labels differ. */
void pz_fighter_step_throw_into_check(void) {
    init_ground_move();
    random_voice(9);
    random_hit(0xE);
    set_my_state(0x120C);
    blend_to_ani(shared_ani.step_throw, 3, 0.1f);
    plyr_anim_pdata->step = 1.6f;
    ani_to_frame_x(8.0f);
    pz_fighter_ani_attack(0x12, 2, 10.0f, 9.0f, 1.0f);
    set_both_face_opponent_flags();
}

/* Near match: 96.56% - table-symbol address formation and relocations. */
void pz_fighter_create_space_between_fighters(void) {
    unsigned short roll = randu0(100);
    float player_distance = xz_distance_between_players();
    float home_distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);
    unsigned int index;

    if (player_distance > 1.5f) {
        return;
    }
    if (home_distance < 1.5f) {
        set_my_state(0x4208);
        init_air_move();
        head_tracking_off();
        pz_fighter_dont_fudge_desired_distance();
        pz_fighter_startup_attack(
            pz_shared_ani.backflip, 0x10000, 0x10008, 3, 2, 14.0f,
            0.1f, 0.6f, 0.3f, 0.8f);
        ani_to_frame_x(24.0f);
        player_feet_land_chores();
        init_ground_move();
        ani_to_frame_x(32.0f);
        init_air_move();
        ani_to_frame_x(38.0f);
        init_ground_move();
        ani_to_end();
        return;
    }
    for (index = 0; index < pz_spacing_table.count; index++) {
        if (roll < pz_spacing_table.choices[index].threshold) {
            pz_spacing_table.choices[index].entry();
            return;
        }
    }
}

/* Soft ceiling: 99.51% - floating-constant relocation labels only. */
float pz_fighter_dash_back(void) {
    avoid_double_ani();
    init_ground_move_no_aniproc();
    random_voice(9);
    set_my_state(0x6208);
    plyr_anim_pdata->flags |= 0x40;
    snd_req(0xD71);
    blend_to_ani(shared_ani.dash_back, 3, 0.2f);
    plyr_anim_pdata->step = 0.9f;
    plyr_anim_pdata->field_68 = 0.0f;
    plyr_anim_pdata->field_64 = 1.0f;
    ani_to_frame_x(12.0f);
    init_air_move();
    ani_to_frame_x(17.0f);
    init_ground_move();
    random_foot(1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

void pz_fighter_create_space_between_fighters_for_special_moves(void) {
    float player_distance = xz_distance_between_players();
    float home_distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);

    if (player_distance > 3.0f) {
        return;
    }
    if (home_distance > 6.0f) {
        PuzzleAttackCopy attack;
        PuzzleFighterMove* move;

        attack = pz_attack_common;
        move = pz_get_fighter_move();
        move->active_flags |= 1;
        pz_fighter_attack(pz_shared_ani.shove, &attack.attack, 0x24);
        ani_to_end();
        return;
    }
    if (home_distance < 4.0f) {
        avoid_double_ani();
        init_ground_move_no_aniproc();
        random_voice(9);
        set_my_state(0x6208);
        plyr_anim_pdata->flags |= 0x40;
        snd_req(0xD71);
        blend_to_ani(shared_ani.dash_back, 3, 0.2f);
        plyr_anim_pdata->step = 0.75f;
        plyr_anim_pdata->field_68 = 0.0f;
        plyr_anim_pdata->field_64 = 1.8f;
        ani_to_frame_x(12.0f);
        init_air_move();
        ani_to_frame_x(17.0f);
        init_ground_move();
        random_foot(1);
        return;
    }
    avoid_double_ani();
    init_ground_move_no_aniproc();
    random_voice(9);
    set_my_state(0x6208);
    plyr_anim_pdata->flags |= 0x40;
    snd_req(0xD71);
    blend_to_ani(shared_ani.dash_back, 3, 0.2f);
    plyr_anim_pdata->step = 0.9f;
    plyr_anim_pdata->field_68 = 0.0f;
    plyr_anim_pdata->field_64 = 1.0f;
    ani_to_frame_x(12.0f);
    init_air_move();
    ani_to_frame_x(17.0f);
    init_ground_move();
    random_foot(1);
}

/* Soft ceiling: 99.63% - floating-constant relocation labels only. */
float pz_fighter_dizzy(void) {
    int flags = 0;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    init_ground_move_no_aniproc();
    rotate_towards_him(0.1f);
    set_my_state(0x4203);
    plyr_pdata->state_flags.bits.dizzy = 1;
    plyr_obj->presentation_flags.unk_bit1 = 0;
    blend_to_ani(shared_ani.dizzy, flags, 0.1f);
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    set_my_state(0);
    for (;;) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}

float pz_fighter_backflip_and_point(void) {
    set_my_state(0x4208);
    init_air_move();
    head_tracking_off();
    pz_fighter_dont_fudge_desired_distance();
    pz_fighter_startup_attack(
        pz_shared_ani.backflip, 0x10000, 0x10008, 3, 2, 14.0f,
        0.1f, 0.6f, 0.3f, 0.8f);
    ani_to_frame_x(24.0f);
    player_feet_land_chores();
    init_ground_move();
    ani_to_frame_x(32.0f);
    init_air_move();
    ani_to_frame_x(38.0f);
    init_ground_move();
    ani_to_end();
    set_my_state(0x420A);
    blend_to_ani(pz_shared_ani.backflip_point, 3, 0.1f);
    set_ani_speed(0.8f);
    ani_to_frame_x(18.0f);
    random_hit(7);
    ani_to_frame_x(30.0f);
    random_hit(7);
    ani_to_blend_frame(20.0f);
    set_my_state(0);
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_just_backflip(void) {
    set_my_state(0x4208);
    init_air_move();
    head_tracking_off();
    pz_fighter_dont_fudge_desired_distance();
    pz_fighter_startup_attack(
        pz_shared_ani.backflip, 0x10000, 0x10008, 3, 2, 14.0f,
        0.1f, 0.6f, 0.3f, 0.8f);
    ani_to_frame_x(24.0f);
    player_feet_land_chores();
    init_ground_move();
    ani_to_frame_x(32.0f);
    init_air_move();
    ani_to_frame_x(38.0f);
    init_ground_move();
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_shove(void) {
    PuzzleAttackCopy attack;
    PuzzleFighterMove* move;

    plyr_pdata->state = 0x120B;
    attack = pz_attack_shove;
    move = pz_get_fighter_move();
    move->active_flags |= 1;
    pz_fighter_attack(pz_shared_ani.shove, &attack.attack, 0x14);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float pz_fighter_shove_brush_back(void) {
    PuzzleAttackParameters attack = pz_attack_shove.attack;
    PuzzleFighterMove* move;

    move = pz_get_fighter_move();
    move->active_flags |= 1;
    pz_fighter_attack(pz_shared_ani.shove, &attack, 0x14);
    ani_to_end();
    return 0.0f;
}

static float pz_fighter_uppercut_brush_back(void) {
    PuzzleAttackParameters attack = pz_attack_uppercut.attack;

    pz_fighter_attack(pz_shared_ani.uppercut_brush_back, &attack, 8);
    slow_ani_x(0.3f, 17.0f);
    ani_to_end();
    return 0.0f;
}

float pz_fighter_superman_move(void) {
    int ticks;
    int reached = 0;
    void* object;

    init_air_move_no_aniproc();
    head_tracking_off();
    plyr_obj->movement_flags.launched = 0;
    blend_to_ani(pz_shared_ani.superman, 3, 0.2f);
    set_ani_speed(0.6f);
    ani_to_frame_x(17.0f);
    set_ani_speed(0.75f);
    myvel_his_angle_y(0.0f, -0.035f, -0.035f);
    for (ticks = 0; ticks < 15; ticks++) {
        if (xz_distance_between_players() < 1.25f) {
            reached = 1;
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    stop_me();
    if (reached == 1) {
        pz_fighter_reaction_xfer_him(0x27);
    }
    ani_to_frame_x(37.0f);
    plyr_obj->movement_flags.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr((MkHdr*)plyr_obj) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr((MkHdr*)plyr_obj) : 0;
    ground_me(object);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_exit(void) {
    plyr_pdata->script_exit_value_int = 10;
    plyr_pdata->script_exit_args[0] = 0;
    plyr_pdata->script_exit_args[1] = 0;
    plyr_pdata->script_exit_arg_2 = 0;
    plyr_pdata->input_unlock_tick = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
    aproc->vtbl->transfer(j_exit_6, 0.0f);
    return 0.0f;
}

float pz_fighter_long_exit(void) {
    plyr_pdata->script_exit_value_int = 20;
    plyr_pdata->script_exit_args[0] = 0;
    plyr_pdata->script_exit_args[1] = 0;
    plyr_pdata->script_exit_arg_2 = 0;
    plyr_pdata->input_unlock_tick = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
    aproc->vtbl->transfer(j_exit_6, 0.0f);
    return 0.0f;
}

/* Exact: explicit process-allocation success and pdata checks. */
void pz_fighter_force_reaction_in_ticks(int reaction, int ticks) {
    PuzzleReactionDelayPdata* pdata;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_force_reaction,
            sizeof(PuzzleReactionDelayPdata), &pdata) != 0 &&
        pdata != 0) {
        pdata->ticks = ticks;
        pdata->reaction = reaction;
        pdata->saved_pdata = apdata;
    }
}

/* Soft ceiling: 99.47% - return-constant relocation labels only. */
static float p_force_reaction(void) {
    PuzzleReactionDelayPdata* pdata = apdata;

    if (--pdata->ticks > 0) {
        return 1.0f;
    }

    apdata = pdata->saved_pdata;
    pz_fighter_reaction_xfer_him(pdata->reaction);
    return -1.0f;
}


static inline PuzzleProcess* puzzle_reaction_transfer_data_live_opponent_proc(PuzzleReactionTransferData* owner) {
    PuzzleProcess* object = owner->opponent_proc;
    if (object != 0) {
        if (object->instance == owner->opponent_proc_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

static inline PuzzleProcess* plyr_pdata_live_hold_proc(PlyrPdata* owner) {
    PuzzleProcess* object = (PuzzleProcess*) owner->hold_proc;
    if (object != 0) {
        if (object->instance == owner->hold_proc_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* Preserve the reaction opponent across hold cleanup for the final dispatch. */




/* TODO: [breakthrough needed] 94.857956%; branch/load placement and register allocation remain; no further evidence-backed source change. */
void pz_fighter_reaction_xfer_him(int reaction) {
    const PuzzleReactionTransferEntry* transfer;
    PuzzleReactionTransferData* reaction_data = apdata;
    PuzzleProcess* opponent_proc;
    PuzzleProcess* hold_proc;
    PuzzleCmdScript* script;
    PuzzleReactionDispatch dispatch;
    unsigned int transfer_offset;

    if (g_pz_fighters_engine.reactions_disabled != 0) {
        return;
    }

    transfer_offset = reaction * sizeof(PuzzleReactionTransferEntry);
    transfer = reaction_transfer_at_offset(transfer_offset);
    dispatch = *(const PuzzleReactionDispatch*)transfer;
    opponent_proc = puzzle_reaction_transfer_data_live_opponent_proc(reaction_data);

    his_obj = reaction_data->opponent_obj;
    his_pdata = reaction_data->opponent_pdata;
    script = (PuzzleCmdScript*)get_cmdscript_for_proc(opponent_proc);
    his_pdata->state = 0x600;
    swap_active_plyr_proc();

    if (plyr_pdata->scream_sound_handle != 0) {
        snd_stop(plyr_pdata->scream_sound_handle);
        plyr_pdata->scream_sound_handle = 0;
    }

    hold_proc = plyr_pdata_live_hold_proc(plyr_pdata);

    if (hold_proc != 0) {
        release_other_player();
        if (plyr_pdata == (PlyrPdata*)g_game_info.plyr0.slot.fighter) {
            xfer_player_proc(
                (PuzzleProcess*)g_game_info.plyr1.idle_proc, j_exit);
        } else {
            xfer_player_proc(
                (PuzzleProcess*)g_game_info.plyr0.idle_proc, j_exit);
        }
    }

    plyr_anim_pdata->flags |= 0x40;
    plyr_anim_pdata->step = 1.0f;
    if (plyr_pdata->state_flags.bits.frozen) {
        unfreeze_player();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    if (reaction != 0x2D && g_pz_fighters_engine.present != 0) {
        g_pz_fighters_engine.present->state = 5;
    }
    if (reaction != 4 && g_global_projectile != 0) {
        g_global_projectile->state = 2;
    }
    run_reaction_cleanup_function(plyr_pdata);

    transfer = reaction_transfer_at_offset(transfer_offset);
    if (transfer->movement_flags & 2) {
        init_air_move_no_aniproc();
    } else {
        if (transfer->movement_flags & 1) {
            init_ground_move_no_aniproc();
        }
        if (transfer->movement_flags & 4) {
            init_3d_move_no_aniproc();
        }
    }
    set_my_state(0x600);
    swap_active_plyr_proc();

    if (dispatch.call_type == 4) {
        script->function = (int)dispatch.entry;
        xfer_player_proc(opponent_proc, r_pz_call_script_function);
    } else if (dispatch.call_type == 0) {
        script->function = (int)dispatch.entry;
        xfer_player_proc(opponent_proc, r_call_player_script_function);
    } else if (dispatch.call_type == 2) {
        script->function = (int)dispatch.entry;
        xfer_player_proc(opponent_proc, r_call_character_cmo_function);
    } else if (dispatch.call_type == 3) {
        script->function = (int)dispatch.entry;
        xfer_player_proc(
            opponent_proc, r_call_other_pz_player_char_script_function);
    } else {
        xfer_player_proc(opponent_proc, dispatch.entry);
    }
}

static float pz_fighter_r_null(void) {
    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 97.72% - floating-constant relocation labels only. */
static float r_pz_ermac_slam(void) {
    got_hit_fx(2, 0xD, 4, 0, 0, 2, 0.0f);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    _mkproc_sleep_ticks = 50.0f;
    aproc->vtbl->sleep();
    plyr_obj->gravity = -0.1f;
    snd_req(0x254);
    blend_to_ani(shared_ani.ermac_slam, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(6.0f);
    wait_to_land();
    snd_req(0x255);
    snd_req(0x1D7);
    set_my_state(0x3203);
    init_air_move();
    got_hit_fx(4, 9, 1, 0, 0, 2, 0.0f);
    plyr_anim_pdata->step = 0.6f;
    launch_me_up(0.08f, -0.003f);
    ani_to_frame_x(34.0f);
    set_my_state(0x600);
    got_hit_fx(4, 9, 1, 0, 0, 2, 0.0f);
    random_hit(9);
    bulvan_function(0);
    init_ground_move();
    stop_me();
    ani_to_end();
    aproc->vtbl->transfer(j_getup_back_6, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.46% - floating-constant relocation labels only. */
static float r_pz_fighter_spear_tug(void) {
    int ticks;

    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_start, 3, 0.1f);
    ani_to_end();
    snd_req(0xD70);
    myvel_his_angle_y(0.0f, -0.05f, -0.05f);
    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_loop, 0, 0.1f);
    plyr_anim_pdata->step = 0.45f;
    ticks = 0;
    while (xz_distance_between_players() > 1.0f && ticks < 120) {
        _mkproc_sleep_ticks = 1.0f;
        ticks++;
        aproc->vtbl->sleep();
    }
    init_ground_move();
    stop_me();
    set_my_state(0x4204);
    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_end, 3, 0.2f);
    plyr_anim_pdata->step = 1.3f;
    ani_loop_more_frames(120.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

static float r_pz_fighter_spear_hit(void) {
    stop_me();
    init_air_move();
    set_my_state(0x603);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    blend_to_ani(his_pdata->fighter_definition->spear_hit, 3, 0.1f);
    ani_to_frame_x(83.0f);
    set_my_state(0x604);
    ani_to_end();
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 94.14% - constant/string relocation labeling and address formation only. */
static float r_pz_fighter_almost_in_grinder(void) {
    face_opponent_now();
    shake_hit_voice(0.02f, 0, 0, 4);

    if (plyr_obj->x > -1.45f && plyr_obj->x < 1.45f) {
        if (plyr_obj->x < -1.35f || plyr_obj->x > 1.35f) {
            force_away(6, 4, 0.03f, 0.9f);
        } else if (plyr_obj->x < -1.2f || plyr_obj->x > 1.2f) {
            force_away(6, 4, 0.0415f, 0.8f);
        } else if (plyr_obj->x < -1.1f || plyr_obj->x > 1.1f) {
            force_away(6, 4, 0.05f, 0.8f);
        } else {
            force_away(6, 4, 0.06f, 0.8f);
        }
    }

    blend_to_ani(pz_shared_ani.almost_in_grinder, 3, 0.1f);
    ani_to_frame_x(10.0f);

    if (plyr_obj->x < -1.35f) {
        pan_snd_req(0x1AB9, -0.5f);
        pan_snd_req(0xD8A, -0.5f);
        pan_snd_req(0xD5D, -0.5f);
        snd_req_delay(0xD5D, 5);
        snd_req_delay(0xD89, 5);
        snd_req_delay(0xD8A, 8);
        snd_req_delay(0xD8D, 10);
    } else {
        pan_snd_req(0x1AB9, 0.5f);
        pan_snd_req(0xD8A, 0.5f);
        pan_snd_req(0xD5D, 0.5f);
        snd_req_delay(0xD5D, 5);
        snd_req_delay(0xD89, 5);
        snd_req_delay(0xD8A, 8);
        snd_req_delay(0xD8D, 10);
    }

    if (plyr_obj->x < -1.35f || plyr_obj->x > 1.35f) {
        Vec post;

        pz_fighter_get_grinder_post(plyr_pdata->plyr_num, &post);
        if (plyr_pdata->plyr_num == 0) {
            post.x -= 0.4f;
        } else {
            post.x += 0.4f;
        }
        bgnd_launch_fx_at_position(
            "post_blood", post.x, plyr_obj->y, post.z);
        if (plyr_pdata->plyr_num == 1) {
            bgnd_set_fx_ang_y(3.1415927f);
        }
    }

    set_ani_speed(0.85f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 98.62% - floating-constant relocation labels only. */
static float r_pz_fighter_feet3_swept_out(void) {
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    plyr_obj->movement_flags.face_opponent = 0;
    plyr_obj->movement_flags.tightrope_restricted = 0;
    pz_fighter_set_y_constrain(plyr_obj, 1, 0.0f);
    blend_to_ani_INOUT(
        shared_ani.swept_out, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xD7F, 0xCB8, 0.0f, 0.0f);
    pz_fighter_set_y_constrain(plyr_obj, 1, 0.3f);
    ani_to_end();
    aproc->vtbl->transfer(j_getup_back_9, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.75% - floating-constant relocation labels only. */
float pz_fighter_inline_force_away_with_ani(
    float velocity, unsigned int coast_ticks, float damping,
    unsigned int damping_ticks) {
    unsigned int ticks;

    myvel_my_angle_y(3.1428f, velocity, velocity);
    for (ticks = 0; ticks < coast_ticks; ticks++) {
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    for (ticks = 0; ticks < damping_ticks; ticks++) {
        plyr_obj->external_force_x *= damping;
        plyr_obj->external_force_z *= damping;
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    return 0.0f;
}

void suspend_in_midair(float ticks) {
    float saved_velocity = plyr_obj->vertical_velocity;

    plyr_obj->gravity_enabled = 0;
    plyr_obj->vertical_velocity = 0.0f;
    _mkproc_sleep_ticks = ticks;
    aproc->vtbl->sleep();
    plyr_obj->vertical_velocity = saved_velocity;
    plyr_obj->gravity_enabled = 1;
}

/* Soft ceiling: 97.48% - floating-constant relocation labels only. */
static float r_pz_fighter_dizzyfall3_with_holdface(void) {
    face_opponent_now();
    wall_eligible_on();
    got_hit_fx(0, 1, 0, 2, 0, 0, 0.0f);
    snd_major_hit_voice();
    force_away(2, 2, 0.04f, 0.4f);
    blend_to_ani(pz_shared_ani.dizzyfall_holdface, 0xB, 0.1f);
    ani_to_frame_x(95.0f);
    got_hit_fx(0, 0xC, 0, 4, 0, 1, 0.0f);
    random_hit(9);
    ani_to_end();
    if (pz_fighter_should_he_breakout() == 0) {
        init_ground_move_no_aniproc();
        plyr_obj->presentation_flags.unk_bit1 = 0;
        tightrope_restrictions_off();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->blend_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.dizzyfall_recover, 0, 0.05f);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        ani_to_frame_x(2.0f);
        init_air_move();
        plyr_anim_pdata->step = 0.9f;
        ani_to_frame_x(12.0f);
        while (plyr_anim_pdata->current_frame <= plyr_anim_pdata->end_frame) {
            ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            if (pz_fighter_should_he_breakout() == 1) {
                break;
            }
        }
    }
    pz_fighter_set_y_constrain(plyr_obj, 1, 0.3f);
    aproc->vtbl->transfer(j_getup_back_6, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.59% - floating-constant relocation labels only. */
static float r_pz_fighter_block_lo(void) {
    stop_me();
    init_ground_move();
    random_hit(1);
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(shared_ani.block_low_start, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(shared_ani.block_low_loop, 0, 0.2f);
    plyr_anim_pdata->step = 0.5f;
    do {
        init_ground_move();
        set_my_state(0x900);
        nudge_towards_him(0.2f);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    } while ((plyr_pdata->state & 0x800) != 0 && his_pdata->state != 0);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

static float r_pz_fighter_block_hi(void) {
    stop_me();
    init_ground_move();
    random_hit(1);
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    set_my_state(0xA00);
    blend_to_ani(shared_ani.block_high, 0, 0.5f);
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->step = 1.0f;
    force_forward(6, 3, plyr_anim_pdata, 0.01f, 0.5f);
    for (;;) {
        nudge_towards_him(0.2f);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if ((plyr_pdata->state & 0x800) == 0 || his_pdata->state == 0) {
            break;
        }
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void pz_fighter_function(unsigned int function) {
    if (function < 3) {
        pz_fighter_tbl[function]();
    }
}

static float pz_fighter_present_explode(void) {
    if (g_pz_fighters_engine.present != 0) {
        g_pz_fighters_engine.present->state = 2;
    }
    return 0.0f;
}

static float pz_fighter_present_given(void) {
    if (g_pz_fighters_engine.present != 0) {
        g_pz_fighters_engine.present->state = 3;
    }
    return 0.0f;
}

static float pz_fighter_present_on_attackers_hand(void) {
    PuzzlePresentState* present = 0;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_present_control,
            sizeof(PuzzlePresentState), &present) != 0 &&
        present != 0) {
        present->state = 0;
        g_pz_fighters_engine.present = present;
        present->owner = plyr_pdata;
    }
    return 0.0f;
}

void pz_fighter_kill_present(void) {
    if (g_pz_fighters_engine.present == 0) {
        return;
    }
    g_pz_fighters_engine.present->state = 5;
}

/* Near match: 97.39% - vector temporary scheduling and pool labels only. */
static float p_present_control(void) {
    static int l_blend_ticks;
    PuzzlePresentState* present = apdata;
    Vec offset;
    Vec target;
    Vec bone_a;
    Vec bone_b;

    switch (present->state) {
    case 0:
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->plyr_info->slot.mirror_a,
            0x1A, &bone_a);
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->plyr_info->slot.mirror_a,
            0x1B, &bone_b);
        v3_sub_v3(&offset, &bone_b, &bone_a);
        offset.x = 0.5f * offset.x;
        offset.y = 0.5f * offset.y;
        offset.z = 0.5f * offset.z;
        g_pz_fighters_engine.present_object->x = offset.x + bone_a.x;
        g_pz_fighters_engine.present_object->y = offset.y + bone_a.y;
        g_pz_fighters_engine.present_object->z = offset.z + bone_a.z;
        update_mkobj(g_pz_fighters_engine.present_object);
        unhide_obj(g_pz_fighters_engine.present_object);
        g_pz_fighters_engine.present_object->presentation_active = 1;
        return 1.0f;
    case 3:
        present->state = 4;
        l_blend_ticks = 5;
        /* fall through */
    case 4:
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->his_plyr_pdata
                ->plyr_info->slot.mirror_a,
            0x1A, &bone_a);
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->his_plyr_pdata
                ->plyr_info->slot.mirror_a,
            0x1B, &bone_b);
        v3_sub_v3(&offset, &bone_b, &bone_a);
        offset.x = 0.5f * offset.x;
        offset.y = 0.5f * offset.y;
        offset.z = 0.5f * offset.z;
        target.x = offset.x + bone_a.x;
        target.y = offset.y + bone_a.y;
        target.z = offset.z + bone_a.z;
        v3_sub_v3(
            &offset, &target,
            &g_pz_fighters_engine.present_object->position);
        offset.x = 0.2f * offset.x;
        offset.y = 0.2f * offset.y;
        offset.z = 0.2f * offset.z;
        g_pz_fighters_engine.present_object->x =
            offset.x + g_pz_fighters_engine.present_object->x;
        g_pz_fighters_engine.present_object->y =
            offset.y + g_pz_fighters_engine.present_object->y;
        g_pz_fighters_engine.present_object->z =
            offset.z + g_pz_fighters_engine.present_object->z;
        update_mkobj(g_pz_fighters_engine.present_object);
        if (--l_blend_ticks == 0) {
            present->state = 1;
        }
        return 1.0f;
    case 1:
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->his_plyr_pdata
                ->plyr_info->slot.mirror_a,
            0x1A, &bone_a);
        get_bone_world_pos(
            (PuzzleFighterObject*)present->owner->his_plyr_pdata
                ->plyr_info->slot.mirror_a,
            0x1B, &bone_b);
        v3_sub_v3(&offset, &bone_b, &bone_a);
        offset.x = 0.5f * offset.x;
        offset.y = 0.5f * offset.y;
        offset.z = 0.5f * offset.z;
        g_pz_fighters_engine.present_object->x = offset.x + bone_a.x;
        g_pz_fighters_engine.present_object->y = offset.y + bone_a.y;
        g_pz_fighters_engine.present_object->z = offset.z + bone_a.z;
        update_mkobj(g_pz_fighters_engine.present_object);
        return 1.0f;
    case 2:
        snd_req(0x1B12);
        hide_obj(g_pz_fighters_engine.present_object);
        bgnd_launch_fx_at_position(
            "present_explosion_fx",
            g_pz_fighters_engine.present_object->x,
            g_pz_fighters_engine.present_object->y,
            g_pz_fighters_engine.present_object->z);
        bgnd_launch_fx_at_position(
            "present_shrapnel_fx",
            g_pz_fighters_engine.present_object->x,
            g_pz_fighters_engine.present_object->y,
            g_pz_fighters_engine.present_object->z);
        g_pz_fighters_engine.present = 0;
        return -1.0f;
    case 5:
        hide_obj(g_pz_fighters_engine.present_object);
        g_pz_fighters_engine.present = 0;
        return -1.0f;
    default:
        return -1.0f;
    }
}

void pz_fighter_allow_easy_continuation(void) {
    g_pz_fighters_engine.flag_bits.easy_continuation = 1;
    g_pz_fighters_engine.flag_bits.continuation_allowed = 1;
}

void pz_fighter_reset_continuation(void) {
    g_pz_fighters_engine.flag_bits.continuation_reset = 0;
}

void pz_fighter_disallow_continuation(void) {
    g_pz_fighters_engine.flag_bits.continuation_allowed = 0;
    g_pz_fighters_engine.flag2_bits.continuation_blocked = 0;
}

void pz_fighter_allow_continuation(void) {
    g_pz_fighters_engine.flag_bits.easy_continuation = 0;
    g_pz_fighters_engine.flag_bits.continuation_allowed = 1;
}

/* Soft ceiling: pz_fighter_clear_out_external_forces ~97.50% - pool label only. */
void pz_fighter_clear_out_external_forces(void) {
    plyr_obj->external_force_x = 0.0f;
    plyr_obj->external_force_z = 0.0f;
}

void pz_fighter_clear_out_all_external_forces(
    PuzzleFighterObject* fighter) {
    fighter->external_force_x = 0.0f;
    fighter->external_force_z = 0.0f;
}

/* Exact: repeated typed slot access preserves retail post-setup reloads. */
static float r_call_other_pz_player_char_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->his_plyr_pdata->cmo,
                              active_cmdscript->function);
    call_player_script_function(plyr_pdata->his_plyr_pdata->cmo);
    return 0.0f;
}

static float r_call_character_cmo_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo,
                              active_cmdscript->function);
    call_player_script_function(plyr_pdata->cmo);
    return 0.0f;
}

static float r_call_player_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->fighter_definition->cmo,
                              active_cmdscript->function);
    call_player_script_function(plyr_pdata->fighter_definition->cmo);
    return 0.0f;
}

float r_pz_call_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(pz_shared_cmo,
                              active_cmdscript->function);
    call_player_script_function(pz_shared_cmo);
    return 0.0f;
}
