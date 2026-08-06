#include "math/gxMath.h"
#include "math/mk_math.h"
#include "game/controller.h"
#include "game/game_info.h"
#include "game/trial.h"
#include "msl/msl_types.h"
#include "platform/main.h"
#include "runtime/cam.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/anim_pdata.h"
#include "runtime/utils.h"

typedef struct EjbProcSleepVtable {
    void* reserved[4];
    int (*destroy)(void);
    void* dispatch;
    void (*sleep)(void);
    void* reserved_after_sleep[2];
    float (*transfer)(MkProcEntryFn entry, float delay);
} EjbProcSleepVtable;

typedef void (*EjbDispatchFn)(void);

typedef struct EjbActionRef {
    int field_00;
    unsigned int action;
} EjbActionRef;

typedef struct EjbSwitchLogEntry {
    unsigned int switch_id;
    int switch_value;
    const char* label;
    unsigned int pad_state;
    int mapped_index;
} EjbSwitchLogEntry;

typedef struct EjbFighterDefinitionView {
    int character_id;
} EjbFighterDefinitionView;

typedef struct EjbPlyrForcePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    PlyrPdata* player;
    unsigned int player_instance;
    int delay;
    float velocity_scale;
    int iterations;
} EjbPlyrForcePdata;

typedef struct EjbAnimPdataExtended {
    char pad00[0x64];
    float weight;
    float weight_velocity;
    char pad6C[8];
    float pose_frame; /* +0x74 */
    char pad78[0x80];
    float transition_rate; /* +0xF8 */
    float landing_frame; /* +0xFC */
} EjbAnimPdataExtended;

typedef struct EjbPlyrPdataExtended {
    char pad000[0x238];
    int initialize_mode; /* +0x238 */
    char pad23C[0x14];
    int hit_count; /* +0x250 */
    int reaction_hit_count; /* +0x254 */
    char pad258[4];
    int combo_hit_count; /* +0x25C */
    char pad260[0x2C];
    float taunt_life_scale; /* +0x28C */
    float postround_value; /* +0x290 */
    float combo_damage; /* +0x294 */
    char pad298[8];
    int script_exit_value; /* +0x2A0 */
    int attack_disable_ticks; /* +0x2A4 */
    int clear_opponent_block; /* +0x2A8 */
    int exit_stance_mode; /* +0x2AC */
    int fatality_advance; /* +0x2B0 */
} EjbPlyrPdataExtended;

typedef struct EjbFighterDefinitionExtended {
    int character_id;
    char pad04[8];
    MkObj* impale_source; /* +0x0C */
    unsigned int impale_source_instance;
    char pad14[0x10];
    MkObj* impale_target; /* +0x24 */
    unsigned int impale_target_instance;
    char pad2C[0x48];
    AniData* standing_animation; /* +0x74 */
    char pad78[0x4C];
    AniData* crouching_animation; /* +0xC4 */
} EjbFighterDefinitionExtended;

typedef struct EjbPlyrScaleView {
    char pad000[0x10C];
    MkHdr* scale_pdata;
    unsigned int scale_pdata_instance;
} EjbPlyrScaleView;

typedef struct EjbSuperchargeView {
    char pad000[0x10C];
    MkHdr* scale_pdata;
    unsigned int scale_pdata_instance;
    char pad114[0x16C];
    unsigned int recharge_tick;
    unsigned int active_until;
    float charge_scale;
} EjbSuperchargeView;

typedef struct EjbScalePdata {
    char pad00[4];
    unsigned int instance;
    char pad08[8];
    const unsigned int* script;
    const unsigned int* current_script;
    Vec scale;
    float script_frame;
} EjbScalePdata;

typedef struct EjbBoneMatcherView {
    char pad00[0x0C];
    float position_weight; /* +0x0C */
    char pad10[0x38];
    float rotation_weight; /* +0x48 */
} EjbBoneMatcherView;

typedef struct EjbSharedAnimationsView {
    char pad000[0x10];
    AniData* turn_around; /* +0x10 */
    AniData* airborne_turn; /* +0x14 */
    char pad018[0x18];
    AniData* step_throw_into;
    AniData* step_throw_out;
    char pad038[0x1D8];
    AniData* back_getup_3; /* +0x210 */
    AniData* back_getup_3_flipped; /* +0x214 */
    AniData* back_getup_6; /* +0x218 */
    AniData* back_getup_6_flipped; /* +0x21C */
    AniData* back_getup_9; /* +0x220 */
    AniData* back_getup_9_flipped; /* +0x224 */
    AniData* back_getup_12; /* +0x228 */
    AniData* back_getup_12_flipped; /* +0x22C */
    AniData* front_getup_4; /* +0x230 */
    AniData* front_getup_4_alt; /* +0x234 */
    AniData* front_getup_6; /* +0x238 */
    AniData* front_getup_6_alt; /* +0x23C */
    AniData* front_getup_10; /* +0x240 */
    AniData* front_getup_10_alt; /* +0x244 */
    AniData* front_getup_12; /* +0x248 */
    AniData* sit_getup_6; /* +0x24C */
    AniData* sit_getup_12; /* +0x250 */
    AniData* chamber_to_stance;
    AniData* chamber_to_stance_2;
    char pad25C[0x14];
    AniData* reverse_to_stance; /* +0x270 */
} EjbSharedAnimationsView;

extern PlyrPdata* his_pdata;
extern CameraObj* camera_obj;
extern MkObj* his_obj;
extern MkObj* plyr_obj;
extern MkProc* plyr_anim_proc;
extern AnimPdata* plyr_anim_pdata;
extern AnimPdata* anim_pdata;
extern int game_tick_ctr;
extern int exec_tick_ctr;
extern float game_speed;
extern float inverse_game_speed;
extern int p1_log_index;
extern int p2_log_index;
extern int p1_current_log_index;
extern int p2_current_log_index;
extern EjbSwitchLogEntry p1_switch_log[30];
extern EjbSwitchLogEntry p2_switch_log[30];

int check_switch(SwitchData* data, int switch_id);
int is_plyr_blocking(PlyrPdata* pdata);
void swap_active_plyr_proc(void);
int is_my_chest_to_screen(void);
void random_voice(int group);
int advance_anim(AnimPdata* anim);
void pose_anim(AnimPdata* anim, int update);
void transition_to_anim_script(
    AnimPdata* anim, AniData* animation, int transition, float blend_rate);
int set_anim_script_frame(
    AnimPdata* anim, AniData* animation, unsigned int flags,
    float frame);
void transition_to_anim_script_frame(
    AnimPdata* anim, AniData* animation, int flags, float frame);
void set_anim_script(AnimPdata* anim, AniData* animation, int transition);
void set_plyr_attack_region(
    int use_body, float radius, float extension);
void start_plyr_attack(float scale);
void online_combo_adjust(float* horizontal, float* vertical);
void online_combo_record(void);
int collide_plyr_vs_plyr(void);
void trial_state_collision_check(int collision_result, int player);
MslSoundHandle snd_req(int sound_id);
float p_wall_monitor(void);
int drone_ai_check_button_press(void);
void advance_cur_cmd_idx(void);
/*
 * Soft ceiling: exact retail size and opcode stream. The only objdiff residue
 * is a consistent saved-GPR permutation across the circular-log state.
 */
int check_button_and_pad(
    unsigned int button, unsigned int direction, int pad);
void advance_my_current_switch(void);
void random_hit(int group);
int local_collision_allowed_plyr_pdata(void);
int local_collision_allowed(PlyrPdata* pdata);
int collide_cylinder_vs_plyr(
    PlyrInfo* player, const Vec* center, const Vec* angles,
    float radius, float height);
int is_weapon_style(PlyrFighterDefinition* style);
int reaction_xfer_him(int reaction, float rate, int strength);
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots);
void plyr_weapon_trail_show(PlyrMirrorSlots* slots);
int is_special_move_available(PlyrPdata* pdata, unsigned int move);
void drone_ai_reset_all(void);
void plyr_bleed_small_cycle_ext(
    PlyrPdata* pdata, int bone, PlyrPdata* owner);
void plyr_bleed_medium_cycle(PlyrPdata* pdata, int bone);
void plyr_bleed_large_ext(
    PlyrPdata* pdata, int bone, PlyrPdata* owner);
void dead_liukang_snd_chain_check(
    PlyrPdata* pdata, int arg1, int arg2, int arg3);
MslSoundHandle random_foot(int group);
void check_bgnd_effect(void);
void anim_set_hiframe(AnimPdata* anim, float frame);
void uv_to_opponent(Vec* direction);
float p_animate(void);
float p_anim_idle(void);
void init_3d_move(void);
void init_ground_move(void);
void init_air_move(void);
void init_air_move_no_aniproc(void);
void init_move(void);
void set_my_state(int state);
void ani_to_blend_frame(float blend_frames);
void check_for_combo_message(void);
float j_exit(void);
float p_blend_to_stance_in_10(void);
void wait_to_land(void);
int joypad_state_5(PlyrPdata* pdata);
void wall_eligible_off(void);
void glitch_to_stance(float blend_rate);
void glitch_to_fstance(float blend_rate);
int blend_to_fstance(float blend_rate);
int blend_to_stance(float blend_rate);
void setup_for_flip_ani(void);
float end_of_round_check(void);
static inline void start_plyr_force(
    int duration, int animation, float damping);
static inline void getup_common(
    AniData* normal, AniData* alternate, int sound,
    int liukang_sound, int blend_frame);
static inline float transfer_roll(MkProcEntryFn entry);
float back_to_crouch(void);
static inline void exit_reaction_common(void);
void myvel_my_angle_y(float angle_offset, float velocity, float vertical);
float p_force_away(void);
void pw_plyr_force(void);
void ps_plyr_force(void);
float two_player_animation_blend(
    AniData* animation, int attacker_mode, int victim_mode,
    float attacker_blend, float victim_blend);
void* plyr_grab_other_flip_states(int self_mode, int other_mode);
void plyr_match_weapon_flip_to_obj_flip(
    PlyrPdata* player, MkObj* object);
float p_konquest_register_bleeding(void);
void special_move_cam_setup(
    int mode, int ticks, int flags, float x, float y, float z,
    float distance, float speed);
void release_other_player(void);
int drone_ai_check_button_direction(int direction);
int drone_ai_should_roll(int mode);
int mk_chess_should_i_fall_down(void);
void player_impale(MkObj* source, MkObj* target);
void trial_increment_state_value(int player, int state, int amount);
void adjust_player_life(int player);
void shake_camera(int ticks, float strength);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
void update_bone_hierarchy(MkHdr* object);
void ground_me(MkHdr* object);
float r_jump_slambounce_final_hit(void);
float r_jump_chin3_final_hit(void);
float r_hit_wall(void);
float j_counter_caught(void);
float x_advance_fatality(void);
float p_plyr_pz_fighter_entry(void);
float drone_entry(void);
float p_joy_entry(void);
int get_fatality_available_flag(void);
int trial_block_callback(int player);
void show_damage_text(int player, int combo_hits, int displayed_damage);
void trial_register_combo(
    int player, int hit_count, PlyrPdata* pdata, float damage);
void force_away(int duration, int animation, float force, float damping);
float ani_to_frame_x_col(
    int attack_region, int reaction, int collision_ticks,
    float frame, float x, float y, float z);
int is_he_blocking_throw(void);
void* start_scale_proc(void* object, const unsigned int* script);
float joy_duck_loop(void);

extern int f_fatality_finished;
extern EjbSharedAnimationsView shared_ani;

static void tremor_collision_check(void);
void scorpion_summon_read(void);
void scorpion_summon_collide(void);
void break_point(void);
void auto_ani_on(void);
static void disable_both_repel_flags(void);
static void taunt_raise_my_life_bar(void);
static void gut_tumble_air_check(void);
static void impale_him(void);
void fan_lift_prep(void);
static void wait_for_backland(void);
static void start_impale_bleeding(void);
static void subzero_propell_collision(void);
void disable_supercharge(void);
void slip_voice(void);
void electric_shaky_voice(void);
void scorpion_voice_call(void);
void drift_downwards(void);
void zero_my_hit_count(void);
void temp_vomit(void);

unsigned int scale_script_chargeup[] = {
    0x00840000, 0x3FA00000, 0x3FA00000, 0x3FA00000, 0x40A00000,
    0x00440000, 0x3FC00000, 0x3FC00000, 0x3FC00000, 0x40A00000,
    0x00240000, 0x3FC00000, 0x3FC00000, 0x3FC00000, 0x41400000,
    0x00840000, 0x3FA00000, 0x3FA00000, 0x3FA00000, 0x40A00000,
    0x00440000, 0x3F800000, 0x3F800000, 0x3F800000, 0x40A00000,
    0x00010000, 0, 0, 0, 0,
};

unsigned int scale_script_normal[] = {
    0x00440000, 0x3F800000, 0x3F800000, 0x3F800000, 0x40A00000,
    0x00010000, 0, 0, 0, 0,
};

static EjbDispatchFn ejb_function_pointers[0x35] = {
    tremor_collision_check,
    scorpion_summon_read,
    scorpion_summon_collide,
    0, 0,
    break_point,
    0, 0, 0, 0,
    auto_ani_on,
    0,
    disable_both_repel_flags,
    taunt_raise_my_life_bar,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    gut_tumble_air_check,
    0,
    impale_him,
    0, 0, 0,
    fan_lift_prep,
    wait_for_backland,
    0, 0,
    start_impale_bleeding,
    subzero_propell_collision,
    0,
    disable_supercharge,
    0, 0, 0, 0, 0, 0,
    slip_voice,
    electric_shaky_voice,
    0, 0, 0,
    scorpion_voice_call,
    drift_downwards,
    0,
    zero_my_hit_count,
    temp_vomit,
};
void liukang_in_fight_random_snd_check(void);
float j_stay_down_dead(void);
void mk_chess_set_breaker_value(void);
void back_to_normal(void);
static float ani_with_new_angle_y(
    AniData* animation, int transition, float frame,
    float step, float blend);
int rotate_towards_sync(void);
void toggle_obj_and_ani_flips(AnimPdata* anim);
int is_blind(PlyrPdata* pdata);
float front_rollup(void);
float j_front_roll_left(void);
float j_front_roll_right(void);
float j_ass_rollup(void);
float j_back_rollup_IN(void);
float j_back_rollup_OUT(void);
int is_big_boss(PlyrPdata* pdata);
float victory(void);
float big_boss_end_of_round(void);
float fall_dead(void);
float dizzy(void);
extern int round_winner;
extern int f_fatality_was_done;
extern int end_round_cam_done;
int trial_show_standard_fight_messages(void);
int mk_chess_did_the_king_just_lose(void);
void skip_end_of_trial_wrapup(void);
void end_of_trial_wrapup(int winner);
float sqrtf(float value);
float drone_start(void);
float go_into_major_pain(void);
float go_into_twitch_death(void);
extern int go_into_major_pain_please;
extern int go_into_twitch_death_please;
void unfreeze_player(void);

extern void (*small_ground_fx)(void);

int g_no_throw_f;
int debug_int_2;
int debug_int_1;
float debug_z;
float debug_y;
float debug_x;
void* plyr_force_pdata;

#define EJB_ADVANCE_TO_FRAME(animation, target_frame)                    \
    do {                                                                 \
        int advance_result;                                              \
        while ((animation)->frame <= (target_frame)) {                   \
            advance_result = advance_anim(animation);                    \
            pose_anim(animation, 1);                                     \
            _mkproc_sleep_ticks = 1.0f;                                  \
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();                 \
            if (((animation)->step * game_speed + (animation)->frame) >  \
                    (target_frame) ||                                    \
                advance_result == 0) {                                   \
                break;                                                   \
            }                                                            \
        }                                                                \
    } while (0)

static inline void ejb_sleep_ticks(float ticks) {
    _mkproc_sleep_ticks = ticks;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

static inline int blend_to_stance_inline(float blend_rate) {
    MkHdr* object_hdr;
    int crouching;

    crouching = 0;
    if (plyr_anim_pdata->last_exec_tick ==
        (unsigned int)exec_tick_ctr) {
        ejb_sleep_ticks(1.0f);
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        crouching = 1;
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->crouching_animation,
            0x20, blend_rate);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->standing_animation,
            0x20, blend_rate);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    return crouching;
}

int is_he_airborn(void) {
    PlyrPdata* opponent;
    MkObj* opponent_object;
    int state;

    opponent = plyr_pdata->his_plyr_pdata;
    opponent_object = plyr_pdata->his_obj;
    if (is_big_boss(opponent) != 0) {
        return 0;
    }

    state = opponent->state;
    if (state == 0x605) {
        return 1;
    }
    if (state == 0x3202) {
        return 1;
    }
    if ((unsigned int)state == 0xFFFFC602U) {
        return 1;
    }
    if (state == 0x6001) {
        return 1;
    }
    if (state == 0x60C) {
        return 0;
    }
    if (opponent_object->pos_vel.y != 0.0f &&
        opponent_object->gravity != 0.0f) {
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling for the four callers below: retail uses stmw/lmw while this
 * compiler emits two individual saves/restores; remaining body records are
 * local relocation labels. Keep this helper before callers so MWCC inlines it.
 */
static inline int is_plyr_airborn_impl(MkObj* object, PlyrPdata* player) {
    int state;

    if (is_big_boss(player) != 0) {
        return 0;
    }

    state = player->state;
    if (state == 0x605) {
        return 1;
    }
    if (state == 0x3202) {
        return 1;
    }
    if ((unsigned int)state == 0xFFFFC602U) {
        return 1;
    }
    if (state == 0x6001) {
        return 1;
    }
    if (state == 0x60C) {
        return 0;
    }
    if (object->pos_vel.y != 0.0f && object->gravity != 0.0f) {
        return 1;
    }
    return 0;
}

int is_pX_airborn(int player_number) {
    if (player_number == 0) {
        return is_plyr_airborn_impl(
            g_game_info.plyr0.slot.mirror_a,
            g_game_info.plyr0.slot.pdata);
    }
    return is_plyr_airborn_impl(
        g_game_info.plyr1.slot.mirror_a,
        g_game_info.plyr1.slot.pdata);
}

int am_i_airborn(void) {
    return is_plyr_airborn_impl(plyr_obj, plyr_pdata);
}

int am_i_airborn_check_in_reaction(void) {
    if (plyr_pdata->state == 0x600 &&
        plyr_pdata->previous_state == 0x605) {
        return 1;
    }
    return is_plyr_airborn_impl(plyr_obj, plyr_pdata);
}

int is_plyr_airborn(MkObj* object, PlyrPdata* player) {
    return is_plyr_airborn_impl(object, player);
}

int does_he_have_life_left(void) {
    float life;

    if (aproc->pid == 0x1001) {
        life = g_game_info.plyr1.field_0C;
    } else {
        life = g_game_info.plyr0.field_0C;
    }
    if (life == 0.0f) {
        return 0;
    }
    return 1;
}

int do_i_have_life_left(void) {
    float life;

    if (aproc->pid == 0x1001) {
        life = g_game_info.plyr0.field_0C;
    } else {
        life = g_game_info.plyr1.field_0C;
    }
    if (life == 0.0f) {
        return 0;
    }
    return 1;
}

int is_he_duck_blocking(void) {
    if (his_pdata->drone_request != 0) {
        if ((his_pdata->state & 0x800) && (his_pdata->state & 0x100)) {
            return 1;
        }
        return 0;
    }

    if (is_plyr_blocking(his_pdata) != 0) {
        return check_switch(his_pdata->switch_data, 0xE);
    }
    return 0;
}

int am_i_duck_blocking(void) {
    if (plyr_pdata->drone_request != 0) {
        if ((plyr_pdata->state & 0x800) && (plyr_pdata->state & 0x100)) {
            return 1;
        }
        return 0;
    }

    if (is_plyr_blocking(plyr_pdata) != 0) {
        return check_switch(plyr_pdata->switch_data, 0xE);
    }
    return 0;
}

int is_he_blocking_throw(void) {
    int blocking;
    int duck_blocking;
    int state;

    swap_active_plyr_proc();
    blocking = is_plyr_blocking(plyr_pdata) != 0;
    if (plyr_pdata->drone_request != 0) {
        duck_blocking =
            (plyr_pdata->state & 0x800) != 0 &&
            (plyr_pdata->state & 0x100) != 0;
    } else if (is_plyr_blocking(plyr_pdata) == 0) {
        duck_blocking = 0;
    } else {
        duck_blocking =
            check_switch(plyr_pdata->switch_data, 0xE);
    }
    if (duck_blocking != 0) {
        blocking = 0;
    }

    state = plyr_pdata->state;
    if (state == 0x101 ||
        state == 0x302 ||
        state == 0x900 ||
        state == 0x901) {
        blocking = 0;
    }
    swap_active_plyr_proc();
    return blocking;
}

int is_plyr_blocking(PlyrPdata* player) {
    MkObj* object;
    int state;

    state = player->state;
    if (state == 0x605) {
        return 0;
    }
    if (state == 0x3202) {
        return 0;
    }
    if (player->drone_request != 0) {
        return ((unsigned int)state >> 11) & 1;
    }

    object = player->tracked_obj;
    if (object != 0 &&
        object->hdr.instance != player->tracked_obj_instance) {
        object = 0;
    }
    if (object == 0) {
        return 0;
    }
    if (object->pos_vel.y != 0.0f &&
        object->gravity != 0.0f &&
        state != 0x60C) {
        return 0;
    }
    if (player->blocking_disabled != 0) {
        return 0;
    }
    if (player->blocking_disabled_2 != 0) {
        return 0;
    }
    if ((unsigned int)player->blocking_disable_tick_1 >
        (unsigned int)game_tick_ctr) {
        return 0;
    }
    if ((unsigned int)player->blocking_disable_tick_2 >
        (unsigned int)game_tick_ctr) {
        return 0;
    }
    if ((int)mode_of_play == 8 &&
        trial_block_callback(player->plyr_num) == 0) {
        return 0;
    }
    if ((g_game_info.flags & 0x20) == 0) {
        return 0;
    }
    return check_switch(player->switch_data, 1) != 0;
}

int is_he_blocking(void) {
    return is_plyr_blocking(his_pdata);
}

int am_i_blocking(void) {
    return is_plyr_blocking(plyr_pdata);
}

void disable_joy_temp(int ticks) {
    plyr_pdata->input_unlock_tick =
        game_tick_ctr + (int)((float)ticks * inverse_game_speed + 0.5f);
}

void disable_blocking(void) {
    plyr_pdata->blocking_disabled = 1;
}

void enable_his_blocking(void) {
    his_pdata->blocking_disabled_2 = 0;
}

void enable_all_my_blocking(void) {
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
}

int is_his_chest_to_screen(void) {
    int result;

    swap_active_plyr_proc();
    result = is_my_chest_to_screen();
    swap_active_plyr_proc();
    return result;
}

int is_my_chest_to_screen(void) {
    float camera_z;
    float camera_x;
    float cross;
    int flipped;

    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    cross =
        ((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z));

    if (cross < 0.0f) {
        flipped = 0;
        if (plyr_obj->hide_flag_bits.bit6 == 1) {
            flipped ^= 1;
        }
        if (plyr_anim_pdata->flags & 8) {
            flipped ^= 1;
        }
        if (flipped != 0) {
            return 0;
        }
        return 1;
    }

    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    if (flipped != 0) {
        return 1;
    }
    return 0;
}

int am_i_on_the_left2(MkObj* opponent, MkObj* me) {
    float camera_z = camera_obj->pos_z;
    float camera_x = camera_obj->pos_x;
    float cross =
        ((opponent->pos.x - camera_x) * -(me->pos.z - camera_z)) -
        ((me->pos.x - camera_x) * -(opponent->pos.z - camera_z));

    return cross < 0.0f;
}

int am_i_on_the_left(void) {
    float camera_z;
    float camera_x;
    float cross;

    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    cross =
        ((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z));
    return cross < 0.0f;
}

int is_he_flipped(void) {
    int flipped;

    swap_active_plyr_proc();
    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    flipped = flipped == 1;
    swap_active_plyr_proc();
    return flipped;
}

void random_voice_him(int group) {
    swap_active_plyr_proc();
    random_voice(group);
    swap_active_plyr_proc();
}

int am_i_a_big_character(void) {
    switch (plyr_pdata->character_id) {
    case 33:
        return 1;
    case 0:
        return 1;
    default:
        return 0;
    }
}

int am_i_flipped_or_turned(void) {
    float camera_z;
    float camera_x;
    float cross;
    int flipped;

    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }

    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    cross =
        ((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z));
    if (cross > 0.0f) {
        flipped ^= 1;
    }

    if (flipped == 0) {
        return 0;
    }
    return 1;
}

int am_i_flipped(void) {
    int flipped;

    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    if (flipped == 0) {
        return 0;
    }
    return 1;
}

static inline int getup_should_stay_down(void) {
    float life;
    int stay_down;

    if (g_game_info.pause_flag_bits.fatality_window) {
        life = aproc->pid == 0x1001
            ? g_game_info.plyr0.field_0C
            : g_game_info.plyr1.field_0C;
        if (life != 0.0f) {
            stay_down = 0;
        } else if ((int)mode_of_play == 0xA &&
                   mk_chess_should_i_fall_down() == 1) {
            stay_down = 1;
        } else if (plyr_pdata->state == 0x4203) {
            stay_down = 1;
        } else if (f_fatality_finished != 0) {
            stay_down = 1;
        } else {
            stay_down = 0;
        }
    } else {
        life = aproc->pid == 0x1001
            ? g_game_info.plyr0.field_0C
            : g_game_info.plyr1.field_0C;
        if (life == 0.0f) {
            stay_down = 1;
            plyr_obj->pos_vel.z = 0.0f;
            plyr_obj->pos_vel.y = 0.0f;
            plyr_obj->pos_vel.x = 0.0f;
        } else {
            stay_down = 0;
        }
    }
    return stay_down;
}

static inline void getup_common(
    AniData* animation, AniData* flipped_animation, int death_type,
    int liukang_sound, int blend_frame) {
    if (getup_should_stay_down()) {
        plyr_pdata->death_type = death_type;
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(j_stay_down_dead, 0.0f);
        return;
    }
    if (liukang_sound) {
        liukang_in_fight_random_snd_check();
    }
    plyr_obj->flags_09_bits.bit6 = 1;
    if ((plyr_anim_pdata->flags & 8) != 0 &&
        flipped_animation != 0) {
        animation = flipped_animation;
    }
    transition_to_anim_script(
        plyr_anim_pdata, animation, 3, 0.1f);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = 1.2f;
    if (blend_frame != 0) {
        ani_to_blend_frame((float)blend_frame);
    }
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
}

float j_getup_front_4(void) {
    getup_common(
        shared_ani.front_getup_4_alt,
        shared_ani.front_getup_10_alt, 7, 0, 0);
    return 0.0f;
}

float j_getup_sit_12(void) {
    getup_common(shared_ani.sit_getup_12, 0, 6, 0, 0);
    return 0.0f;
}

float j_getup_sit_6(void) {
    getup_common(shared_ani.sit_getup_6, 0, 5, 0, 0);
    return 0.0f;
}

float j_getup_front_6(void) {
    getup_common(shared_ani.front_getup_6, 0, 8, 1, 0);
    return 0.0f;
}

float j_getup_front_10(void) {
    getup_common(
        shared_ani.front_getup_10_alt, 0, 9, 0, 0);
    return 0.0f;
}

float j_getup_front_12(void) {
    getup_common(shared_ani.front_getup_12, 0, 4, 1, 0);
    return 0.0f;
}

float j_getup_back_3(void) {
    getup_common(
        shared_ani.back_getup_3, shared_ani.back_getup_9,
        3, 0, 0);
    return 0.0f;
}

float j_getup_back_12(void) {
    getup_common(shared_ani.back_getup_12, 0, 2, 1, 0);
    return 0.0f;
}

float j_getup_back_6(void) {
    getup_common(shared_ani.back_getup_6, 0, 1, 1, 0);
    return 0.0f;
}

float j_getup_back_9(void) {
    float life;

    if (aproc->pid == 0x1001) {
        life = g_game_info.plyr0.field_0C;
    } else {
        life = g_game_info.plyr1.field_0C;
    }
    if (life == 0.0f) {
        plyr_pdata->death_type = 0;
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(j_stay_down_dead, 0.0f);
        return 0.0f;
    }

    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.back_getup_9, 3, 0.1f);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = 1.2f;
    ani_to_blend_frame(10.0f);
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

void aniproc_land(void) {
    EjbAnimPdataExtended* animation;
    EjbFighterDefinitionExtended* fighter;

    animation = (EjbAnimPdataExtended*)anim_pdata;
    EJB_ADVANCE_TO_FRAME(anim_pdata, animation->transition_rate);
    anim_set_hiframe(anim_pdata, animation->landing_frame);
    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    transition_to_anim_script(
        anim_pdata, fighter->standing_animation, 0, 0.1f);
    plyr_pdata->saved_anim_script_word = 0;
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)p_animate, 0.0f);
}

void hit_START_chores(
    int first_sound, int second_sound,
    float shake_ticks, float shake_strength) {
    MkProc* process;

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
    if (first_sound != 0) {
        snd_req(first_sound);
    }
    if (second_sound != 0) {
        snd_req(second_sound);
    }
    if (shake_ticks != 0.0f) {
        shake_camera((int)shake_ticks, shake_strength);
    }
}

void land_chores(
    int land_sound, int second_sound,
    float shake_ticks, float shake_strength) {
    MkProc* process;

    wall_eligible_off();
    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance !=
            (int)plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        ((EjbProcSleepVtable*)process->vtbl)->destroy();
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_09_bits.bit6 = 1;
    plyr_obj->flags_09_bits.launched = 1;
    if (land_sound != 0) {
        snd_req(land_sound);
    }
    if (second_sound != 0) {
        snd_req(second_sound);
    }
    if (shake_ticks != 0.0f) {
        shake_camera((int)shake_ticks, shake_strength);
    }
    check_for_combo_message();
}

void launch_n_land_ani(
    AniData* animation, int landing_animation, float launch_frame,
    float launch_step, float landing_frame, float velocity_y,
    float gravity, float blend) {
    float discriminant;
    float flight_ticks;
    float target;

    plyr_anim_pdata->flags |= 0x40;
    transition_to_anim_script(
        plyr_anim_pdata, animation, 0x43, blend);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    if (launch_frame != 0.0f) {
        plyr_anim_pdata->step = launch_step;
        target = launch_frame < plyr_anim_pdata->high_frame
            ? launch_frame : plyr_anim_pdata->high_frame;
        while (plyr_anim_pdata->frame < target &&
               advance_anim(plyr_anim_pdata)) {
            pose_anim(plyr_anim_pdata, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        }
        plyr_anim_pdata->step = 1.0f;
    }
    plyr_obj->pos_vel.y = velocity_y;
    plyr_obj->gravity = gravity;
    plyr_obj->flags_08 |= 1;
    plyr_obj->flags_09 |= 0x80;
    discriminant = velocity_y * velocity_y -
        2.0f * gravity *
        ((plyr_obj->pos.y - 0.19f) - plyr_obj->ground_colls_y);
    if (discriminant < 0.001f) {
        discriminant = 0.001f;
    }
    flight_ticks = (-velocity_y - sqrtf(discriminant)) / gravity;
    if (flight_ticks < 1.0f) {
        flight_ticks = 1.0f;
    }
    plyr_anim_pdata->step =
        (landing_frame - launch_frame) / flight_ticks;
    wait_to_land();
    land_chores(landing_animation, 0, 0.0f, 0.0f);
}

void launch_me_up(float vertical_velocity, float gravity) {
    plyr_obj->pos_vel.y = vertical_velocity;
    plyr_obj->gravity = gravity;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->flags_09_bits.launched = 1;
}

void setup_to_match_land_frame(
    float vertical_velocity, float gravity, float frames) {
    float landing_ticks;
    float absolute_ticks;

    plyr_obj->pos_vel.y = vertical_velocity;
    plyr_obj->gravity = gravity;
    landing_ticks =
        2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (landing_ticks >= 0.0f) {
        absolute_ticks = landing_ticks;
    } else {
        absolute_ticks = -landing_ticks;
    }
    plyr_anim_pdata->step = frames / absolute_ticks;
}

void nudge_towards_him(float max_step) {
    float desired_angle;
    float error;
    float absolute_error;

    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return;
    }

    desired_angle = gxMathArcTanYX(
        his_obj->pos.x - plyr_obj->pos.x,
        his_obj->pos.z - plyr_obj->pos.z);
    error = ang_sub_ang(desired_angle, plyr_obj->ang.y);
    if (error >= 0.0f) {
        absolute_error = error;
    } else {
        absolute_error = -error;
    }
    if (absolute_error < max_step) {
        plyr_obj->ang.y = desired_angle;
        return;
    }
    if (error < 0.0f) {
        max_step = -max_step;
    }
    plyr_obj->ang.y += max_step;
}

void rotate_towards_position(const Vec* target, float max_step) {
    float desired;
    float difference;
    float absolute_difference;
    EjbAnimPdataExtended* animation;
    EjbFighterDefinitionExtended* fighter;

    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return;
    }
    desired = gxMathArcTanYX(
        target->x - plyr_obj->pos.x,
        target->z - plyr_obj->pos.z);
    difference = ang_sub_ang(desired, plyr_obj->ang.y);
    absolute_difference =
        difference >= 0.0f ? difference : -difference;
    if (absolute_difference > 2.7f) {
        set_my_state(0x201);
        init_air_move_no_aniproc();
        ani_with_new_angle_y(
            shared_ani.airborne_turn, 3, 9.0f, 1.0f, 0.1f);
        snd_req(0xD6B);
        if (plyr_anim_pdata->last_exec_tick ==
            (unsigned int)exec_tick_ctr) {
            ejb_sleep_ticks(1.0f);
        }
        fighter =
            (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
        if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
            (plyr_pdata->state & 0x100) != 0) {
            transition_to_anim_script(
                plyr_anim_pdata, fighter->crouching_animation,
                0x20, 0.1f);
        } else {
            transition_to_anim_script(
                plyr_anim_pdata, fighter->standing_animation,
                0x20, 0.1f);
        }
        xfer_proc(plyr_anim_proc, p_animate);
        plyr_obj->flags_09_bits.launched = 1;
        update_bone_hierarchy(as_mkhdr(&plyr_obj->hdr));
        ground_me(as_mkhdr(&plyr_obj->hdr));
        plyr_anim_pdata->step = 1.0f;
        animation = (EjbAnimPdataExtended*)plyr_anim_pdata;
        while (animation->pose_frame < 14.0f) {
            ejb_sleep_ticks(1.0f);
        }
        random_foot(0);
        init_ground_move();
        while (animation->pose_frame < 17.0f) {
            ejb_sleep_ticks(1.0f);
        }
        set_my_state(0);
        plyr_pdata->secondary_state = 0;
        if (is_blind(plyr_pdata) != 0 ||
            fighter->character_id == 0x33) {
            plyr_obj->flags_09_bits.head_tracking = 0;
        } else {
            plyr_obj->flags_09_bits.head_tracking = 1;
        }
    } else {
        while (absolute_difference > max_step) {
            if (difference < 0.0f) {
                plyr_obj->ang.y -= max_step;
            } else {
                plyr_obj->ang.y += max_step;
            }
            ejb_sleep_ticks(1.0f);
            desired = gxMathArcTanYX(
                target->x - plyr_obj->pos.x,
                target->z - plyr_obj->pos.z);
            difference = ang_sub_ang(desired, plyr_obj->ang.y);
            absolute_difference =
                difference >= 0.0f ? difference : -difference;
        }
    }
    plyr_obj->ang.y = desired;
}

void rotate_towards_him(float max_step) {
    float desired;
    float difference;
    float absolute_difference;
    EjbFighterDefinitionExtended* fighter;

    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0 ||
        (plyr_obj->flags_0B & 8) == 8) {
        return;
    }

    desired = gxMathArcTanYX(
        his_obj->pos.x - plyr_obj->pos.x,
        his_obj->pos.z - plyr_obj->pos.z);
    difference = ang_sub_ang(desired, plyr_obj->ang.y);
    if (rotate_towards_sync() == 1) {
        set_my_state(0x201);
        xfer_proc(plyr_anim_proc, p_anim_idle);
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.turn_around, 3, 0.125f);
        plyr_anim_pdata->step = 0.6f;
        ejb_sleep_ticks(1.0f);
        while (plyr_anim_pdata->frame < 4.8f) {
            advance_anim(plyr_anim_pdata);
            pose_anim(plyr_anim_pdata, 1);
            ejb_sleep_ticks(1.0f);
        }
        plyr_anim_pdata->flags |= 0x1000;
        plyr_obj->ang.y += 3.1415927f;
        toggle_obj_and_ani_flips(plyr_anim_pdata);
        fighter =
            (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
        transition_to_anim_script(
            plyr_anim_pdata, fighter->standing_animation, 0, 0.125f);
        plyr_anim_pdata->step = 1.0f;
        ejb_sleep_ticks(1.0f);
        while (plyr_anim_pdata->frame < 8.0f) {
            advance_anim(plyr_anim_pdata);
            pose_anim(plyr_anim_pdata, 1);
            ejb_sleep_ticks(1.0f);
        }
        set_my_state(0);
        plyr_pdata->secondary_state = 0;
        xfer_proc(plyr_anim_proc, p_animate);
    } else {
        absolute_difference =
            difference >= 0.0f ? difference : -difference;
        while (absolute_difference > max_step) {
            if (difference < 0.0f) {
                plyr_obj->ang.y -= max_step;
            } else {
                plyr_obj->ang.y += max_step;
            }
            ejb_sleep_ticks(1.0f);
            desired = gxMathArcTanYX(
                his_obj->pos.x - plyr_obj->pos.x,
                his_obj->pos.z - plyr_obj->pos.z);
            difference = ang_sub_ang(desired, plyr_obj->ang.y);
            absolute_difference =
                difference >= 0.0f ? difference : -difference;
        }
    }
    plyr_obj->ang.y = desired;
}

static float ani_with_new_angle_y(
    AniData* animation, int transition, float frame,
    float step, float blend) {
    AnimPdata* anim;
    float target_frame;
    float old_x;
    float old_z;

    plyr_obj->flags_09_bits.head_tracking = 0;
    transition_to_anim_script(
        plyr_anim_pdata, animation, transition, blend);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = step;
    target_frame = frame - 1.0f;
    anim = plyr_anim_pdata;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);

    old_x = plyr_obj->pos.x;
    old_z = plyr_obj->pos.z;
    plyr_obj->ang.y += 3.1428f;
    plyr_anim_pdata->frame = frame + 1.0f;
    pose_anim(plyr_anim_pdata, 1);
    plyr_obj->pos.x = old_x;
    plyr_obj->pos.z = old_z;
    ejb_sleep_ticks(1.0f);
    return 0.0f;
}

float get_my_angle_y_error(void) {
    float angle;

    angle = gxMathArcTanYX(
        his_obj->pos.x - plyr_obj->pos.x,
        his_obj->pos.z - plyr_obj->pos.z);
    return ang_sub_ang(angle, plyr_obj->ang.y);
}

void face_opponent_180(void) {
    plyr_obj->ang.y = his_obj->ang.y;
}

void face_position_now(const Vec* position) {
    float position_x;

    position_x = position->x;
    plyr_obj->ang.y = gxMathArcTanYX(
        position_x - plyr_obj->pos.x,
        position->z - plyr_obj->pos.z);
}

void face_opponent_now(void) {
    float angle;

    if (his_obj == 0 || plyr_obj == 0) {
        angle = 0.0f;
    } else {
        angle = gxMathArcTanYX(
            his_obj->pos.x - plyr_obj->pos.x,
            his_obj->pos.z - plyr_obj->pos.z);
    }
    plyr_obj->ang.y = angle;
}

void face_ang_from_pos_to_him(
    MkObj* opponent, const Vec* position, Vec* angle) {
    angle->x = 0.0f;
    angle->y = gxMathArcTanYX(
        opponent->pos.x - position->x,
        opponent->pos.z - position->z);
    angle->z = 0.0f;
}

void face_point(float x, float y, float z) {
    plyr_obj->ang.y =
        gxMathArcTanYX(x - plyr_obj->pos.x, z - plyr_obj->pos.z);
}

void tightrope_restrictions_off(void) {
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
}

void tightrope_restrictions_on(void) {
    plyr_obj->flags_09_bits.tightrope_restricted = 1;
}

void init_3d_move_no_aniproc(void) {
    init_3d_move();
    xfer_proc(plyr_anim_proc, p_anim_idle);
}

void set_both_face_opponent_flags(void) {
    plyr_obj->flags_09_bits.face_opponent = 1;
    his_obj->flags_09_bits.face_opponent = 1;
}

void clear_both_face_opponent_flags(void) {
    plyr_obj->flags_09_bits.face_opponent = 0;
    his_obj->flags_09_bits.face_opponent = 0;
}

void clear_my_face_opponent_flag(void) {
    plyr_obj->flags_09_bits.face_opponent = 0;
}

void init_3d_move_no_face(void) {
    init_ground_move();
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_09_bits.face_opponent = 0;
}

void init_3d_move(void) {
    init_ground_move();
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_09_bits.face_opponent = 1;
}

void end_air_move(void) {
    MkProc* process;

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&plyr_obj->hdr));
    ground_me(as_mkhdr(&plyr_obj->hdr));
}

/*
 * Honest soft ceiling for the air-move callers: retail preserves and writes
 * the animation step back to itself before the flag updates. That unobservable
 * load/store and its scheduling account for their exact 20-byte source gap.
 */
static inline void init_move_impl(void) {
    EjbAnimPdataExtended* animation;
    float weight;

    animation = (EjbAnimPdataExtended*)plyr_anim_pdata;
    plyr_obj->hide_flag_bits.still_move = 0;
    animation->weight_velocity = 0.0f;
    if ((int)mode_of_play == 6) {
        weight = 0.25f;
    } else {
        weight = 1.0f;
    }
    animation->weight = weight;
    plyr_anim_pdata->step = 1.0f;
    plyr_pdata->collision_result = -1;
    plyr_pdata->collision_disabled = 0;
}

void init_air_move_no_aniproc(void) {
    init_move_impl();
    plyr_obj->flags_09_bits.launched = 0;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->flags_09_bits.bit6 = 0;
    plyr_obj->flags_09_bits.launched = 1;
    xfer_proc(plyr_anim_proc, p_anim_idle);
}

void init_air_move(void) {
    init_move_impl();
    plyr_obj->flags_09_bits.launched = 0;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->flags_09_bits.bit6 = 0;
    plyr_obj->flags_09_bits.launched = 1;
}

void init_still_move(void) {
    init_ground_move();
    plyr_obj->hide_flag_bits.still_move = 1;
}

void init_ground_move_no_aniproc(void) {
    init_ground_move();
    xfer_proc(plyr_anim_proc, p_anim_idle);
}

void init_ground_move(void) {
    MkProc* process;

    plyr_obj->hide_flag_bits.still_move = 0;
    plyr_anim_pdata->weight_velocity = 0.0f;
    if ((int)mode_of_play == 6) {
        plyr_anim_pdata->weight = 0.25f;
    } else {
        plyr_anim_pdata->weight = 1.0f;
    }
    plyr_anim_pdata->step = 1.0f;
    plyr_pdata->collision_result = -1;
    plyr_pdata->collision_disabled = 0;
    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    plyr_obj->flags_09_bits.bit6 = 1;
    plyr_obj->flags_09_bits.launched = 1;
}

void init_move(void) {
    init_move_impl();
}

void set_ani_weight(float weight) {
    plyr_anim_pdata->weight = weight;
}

void set_ani_speed(float speed) {
    plyr_anim_pdata->step = speed;
}

void set_my_state(int state) {
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = state;
    if (state == 0) {
        plyr_pdata->secondary_state = 0;
    }
}

void set_my_secondary_state(int state) {
    plyr_pdata->secondary_state = state;
}

void set_my_damage_multiplier(float multiplier) {
    plyr_pdata->damage_multiplier = multiplier;
}

/*
 * Honest soft ceiling: this helper and its exported wrapper have retail's
 * exact 216-byte body. Residue is load/conversion scheduling and local labels;
 * retaining a PID-only lifetime regresses the inlined back_to_normal caller.
 */
static inline void check_for_combo_message_impl(void) {
    float displayed_damage;

    {
        MkProc* process = aproc;
        EjbPlyrPdataExtended* player =
            (EjbPlyrPdataExtended*)plyr_pdata;
        int combo_hits = player->combo_hit_count;

        if (combo_hits > 1 &&
            (combo_hits > 2 || player->combo_damage > 0.15f)) {
            displayed_damage = (int)(player->combo_damage * 100.5f);
            show_damage_text(
                process->pid == 0x1001, combo_hits, displayed_damage);
        }
    }
    trial_register_combo(
        plyr_pdata->plyr_num,
        ((EjbPlyrPdataExtended*)plyr_pdata)->combo_hit_count,
        plyr_pdata, ((EjbPlyrPdataExtended*)plyr_pdata)->combo_damage);
    ((EjbPlyrPdataExtended*)plyr_pdata)->hit_count = 0;
    ((EjbPlyrPdataExtended*)plyr_pdata)->reaction_hit_count = 0;
    ((EjbPlyrPdataExtended*)plyr_pdata)->combo_damage = 0.0f;
    ((EjbPlyrPdataExtended*)plyr_pdata)->combo_hit_count = 0;
}

void check_for_combo_message(void) {
    check_for_combo_message_impl();
}

void animpdata_ani_to_blend_frame(
    AnimPdata* anim, float blend_frames) {
    float target_frame;

    target_frame = anim->high_frame - blend_frames;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_to_blend_frame(float blend_frames) {
    AnimPdata* anim;
    float target_frame;

    anim = plyr_anim_pdata;
    target_frame = anim->high_frame - blend_frames;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void if_collision_autoface_him(void) {
    if (plyr_pdata->collision_result != 0) {
        his_obj->flags_09_bits.face_opponent = 1;
    }
}

void if_collision_autoface_me(void) {
    if (plyr_pdata->collision_result != 0) {
        plyr_obj->flags_09_bits.face_opponent = 1;
    }
}

void if_collision_slow_ani_x(float speed, float frame) {
    float old_speed;
    float target_frame;
    AnimPdata* anim;

    old_speed = plyr_anim_pdata->step;
    if (plyr_pdata->collision_result != 0) {
        plyr_anim_pdata->step = speed;
    }
    anim = plyr_anim_pdata;
    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
    plyr_anim_pdata->step = old_speed;
}

void slow_ani_end(float speed) {
    float old_speed;
    float target_frame;
    AnimPdata* anim;

    old_speed = plyr_anim_pdata->step;
    plyr_anim_pdata->step = speed;
    anim = plyr_anim_pdata;
    target_frame = anim->high_frame;
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
    plyr_anim_pdata->step = old_speed;
}

void set_ani_speed_miss_hit(float miss_speed, float hit_speed) {
    if (plyr_pdata->collision_result == 2 ||
        plyr_pdata->collision_result == 0) {
        plyr_anim_pdata->step = miss_speed;
        return;
    }

    if (hit_speed != -1.0f) {
        plyr_anim_pdata->step = hit_speed;
    }
}

void slow_ani_x_if_miss(
    float miss_speed, float hit_speed, float frame) {
    float old_speed;
    float target_frame;
    AnimPdata* anim;
    int collision_result;

    collision_result = plyr_pdata->collision_result;
    old_speed = plyr_anim_pdata->step;
    if (collision_result == 0 || collision_result == 2) {
        plyr_anim_pdata->step = miss_speed;
    } else if (hit_speed != -1.0f) {
        plyr_anim_pdata->step = hit_speed;
    }

    anim = plyr_anim_pdata;
    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
    plyr_anim_pdata->step = old_speed;
}

void slow_ani_x(float speed, float frame) {
    float old_speed;
    float target_frame;
    AnimPdata* anim;

    old_speed = plyr_anim_pdata->step;
    plyr_anim_pdata->step = speed;
    anim = plyr_anim_pdata;
    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
    plyr_anim_pdata->step = old_speed;
}

void ani_to_frame_x_aniproc(float frame) {
    float target_frame;
    AnimPdata* anim;

    anim = anim_pdata;
    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_to_fall_to_frame(
    int sound_id, float landing_frame, float target_frame) {
    EJB_ADVANCE_TO_FRAME(plyr_anim_pdata, landing_frame);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&plyr_obj->hdr));
    ground_me(as_mkhdr(&plyr_obj->hdr));
    snd_req(sound_id);
    shake_camera(3, 0.03f);
    EJB_ADVANCE_TO_FRAME(plyr_anim_pdata, target_frame);
}

void ani_to_frame_sound(
    int sound_id, float target_frame, float sound_frame) {
    if (target_frame > sound_frame) {
        EJB_ADVANCE_TO_FRAME(plyr_anim_pdata, sound_frame);
    }
    snd_req(sound_id);
    EJB_ADVANCE_TO_FRAME(plyr_anim_pdata, target_frame);
}

void animpdata_ani_to_end_at1(AnimPdata* anim) {
    float target_frame;

    anim->step = 1.0f;
    target_frame = anim->high_frame;
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_through_end(void) {
    AnimPdata* animation;
    int advancing;

    animation = plyr_anim_pdata;
    advancing = 1;
    while (advancing != 0) {
        advancing = advance_anim(animation);
        pose_anim(animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

void animpdata_ani_to_end(AnimPdata* anim) {
    float target_frame;

    target_frame = anim->high_frame;
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_to_end(void) {
    AnimPdata* anim;
    float target_frame;

    anim = plyr_anim_pdata;
    target_frame = anim->high_frame;
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void animpdata_ani_x_more_frames(AnimPdata* anim, float frames) {
    float target_frame;

    target_frame = anim->frame + frames;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_x_more_frames(float frames) {
    AnimPdata* anim;
    float target_frame;

    anim = plyr_anim_pdata;
    target_frame = anim->frame + frames;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void animpdata_ani_loop_more_frames(AnimPdata* anim, float frames) {
    while (frames > 0.0f) {
        advance_anim(anim);
        pose_anim(anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        frames -= 1.0f;
    }
}

void ani_loop_more_frames(float frames) {
    AnimPdata* anim;

    anim = plyr_anim_pdata;
    while (frames > 0.0f) {
        advance_anim(anim);
        pose_anim(anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        frames -= 1.0f;
    }
}

void ani_to_frame_x_call(
    void (*callback)(void), float target_frame) {
    while (plyr_anim_pdata->frame <= target_frame) {
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        callback();
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

void animpdata_ani_to_frame_x(AnimPdata* anim, float frame) {
    float target_frame;

    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void ani_to_frame_x(float frame) {
    AnimPdata* anim;
    float target_frame;

    anim = plyr_anim_pdata;
    target_frame = frame;
    if (target_frame > anim->high_frame) {
        target_frame = anim->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(anim, target_frame);
}

void mks_animpdata_set_cur_frame(AnimPdata* anim, float frame) {
    anim->frame = frame;
}

void animpdata_ani_1_frame(AnimPdata* anim) {
    advance_anim(anim);
    pose_anim(anim, 1);
}

void ani_1_frame(void) {
    AnimPdata* anim;

    anim = plyr_anim_pdata;
    advance_anim(anim);
    pose_anim(anim, 1);
}

void newani_to_frame_x(
    AniData* animation, int flip_mode, float frame, float step,
    float weight, float blend) {
    int flags = flip_mode == 2 ? 8 : 0;
    float target;

    if (flip_mode != 0 && flip_mode != 1 && flip_mode != 2) {
        swap_active_plyr_proc();
        if (visual_flip_state()) {
            flags ^= 8;
        }
        swap_active_plyr_proc();
        if (visual_flip_state()) {
            flags ^= 8;
        }
    }
    transition_to_anim_script(
        plyr_anim_pdata, animation, flags | 3, blend);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    plyr_anim_pdata->step = step;
    plyr_anim_pdata->weight_velocity = 0.0f;
    plyr_anim_pdata->weight = weight;
    target = frame > plyr_anim_pdata->high_frame
        ? plyr_anim_pdata->high_frame : frame;
    while (plyr_anim_pdata->frame < target) {
        int active = advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        if (!active) {
            break;
        }
    }
}

float fpick_a_float(float normal, float flipped_value) {
    int flipped;

    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    if (flipped != 0) {
        return flipped_value;
    }
    return normal;
}

int should_weapon_block(PlyrPdata* player) {
    if (is_big_boss(player) != 0) {
        return 1;
    }
    return is_weapon_style(player->fighter_definition) == 1;
}

int should_i_weapon_block(void) {
    PlyrPdata* player;

    player = plyr_pdata;
    if (is_big_boss(player) != 0) {
        return 1;
    }
    if (is_weapon_style(player->fighter_definition) == 0) {
        return 0;
    }
    return 1;
}

void blend_to_ani_INOUT(
    AniData* in_animation, AniData* out_animation, float blend_rate,
    float in_speed, float out_speed) {
    float animation_speed;

    if (is_his_chest_to_screen() != 0) {
        if (is_my_chest_to_screen() != 0) {
            transition_to_anim_script(
                plyr_anim_pdata, in_animation, 3, blend_rate);
            _mkproc_sleep_ticks = 1.0f;
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
            animation_speed = in_speed;
        } else {
            transition_to_anim_script(
                plyr_anim_pdata, out_animation, 3, blend_rate);
            _mkproc_sleep_ticks = 1.0f;
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
            animation_speed = out_speed;
        }
    } else if (is_my_chest_to_screen() != 0) {
        transition_to_anim_script(
            plyr_anim_pdata, out_animation, 3, blend_rate);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        animation_speed = out_speed;
    } else {
        transition_to_anim_script(
            plyr_anim_pdata, in_animation, 3, blend_rate);
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        animation_speed = in_speed;
    }
    plyr_anim_pdata->step = animation_speed;
}

void blend_to_ani_nosleep(
    AniData* animation, int transition, float blend_rate) {
    transition_to_anim_script(
        plyr_anim_pdata, animation, transition, blend_rate);
}

void glitch_to_ani_frame(
    AniData* animation, int flags, float frame) {
    set_anim_script_frame(
        plyr_anim_pdata, animation, flags, frame);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

void blend_to_ani_frame(
    AniData* animation, int flags, float frame) {
    transition_to_anim_script_frame(
        plyr_anim_pdata, animation, flags, frame);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

void blend_to_ani(AniData* animation, int transition, float blend_rate) {
    transition_to_anim_script(
        plyr_anim_pdata, animation, transition, blend_rate);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

static inline void exit_reaction_common(void) {
    EjbPlyrPdataExtended* player;
    EjbFighterDefinitionExtended* fighter;
    MkHdr* object_hdr;
    float frames;
    float ticks;
    float exit_ticks;
    float blend_rate;
    unsigned int duration;

    player = (EjbPlyrPdataExtended*)plyr_pdata;
    xfer_proc(plyr_anim_proc, p_animate);
    frames = plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
    ticks = frames / plyr_anim_pdata->step;
    if (player->script_exit_value < 3 ||
        player->script_exit_value > 100) {
        player->script_exit_value = 20;
    }
    if (player->attack_disable_ticks != 0) {
        if (player->attack_disable_ticks < 2 ||
            player->attack_disable_ticks > 60) {
            player->attack_disable_ticks = 2;
        }
        plyr_pdata->attacks_disabled_until =
            game_tick_ctr +
            (int)((float)player->attack_disable_ticks *
                  inverse_game_speed + 0.5f);
    }
    duration = plyr_pdata->input_unlock_tick;
    if (duration != 0) {
        plyr_pdata->input_unlock_tick =
            game_tick_ctr +
            (int)((float)duration * inverse_game_speed + 0.5f);
    }
    duration = plyr_pdata->blocking_disable_tick_2;
    if (duration != 0) {
        plyr_pdata->blocking_disable_tick_2 =
            game_tick_ctr +
            (int)((float)duration * inverse_game_speed + 0.5f);
    }
    if (player->clear_opponent_block != 0) {
        his_pdata->blocking_disabled_2 = 0;
    }

    exit_ticks = (float)player->script_exit_value;
    if (ticks > exit_ticks) {
        _mkproc_sleep_ticks = (float)(int)(ticks - exit_ticks);
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        frames = plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
        ticks = frames * plyr_anim_pdata->step;
    }
    if (frames < 1.0f) {
        plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;
        ejb_sleep_ticks(1.0f);
    } else {
        plyr_anim_pdata->step *= ticks / exit_ticks;
    }
    blend_rate = 1.0f / exit_ticks;
    switch (player->exit_stance_mode) {
    case 0:
        if (plyr_anim_pdata->last_exec_tick ==
            (unsigned int)exec_tick_ctr) {
            ejb_sleep_ticks(1.0f);
        }
        fighter =
            (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
        if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
            (plyr_pdata->state & 0x100)) {
            transition_to_anim_script(
                plyr_anim_pdata, fighter->crouching_animation,
                0x20, blend_rate);
        } else {
            transition_to_anim_script(
                plyr_anim_pdata, fighter->standing_animation,
                0x20, blend_rate);
        }
        xfer_proc(plyr_anim_proc, p_animate);
        plyr_obj->flags_09_bits.launched = 1;
        object_hdr =
            plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
        update_bone_hierarchy(object_hdr);
        object_hdr =
            plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
        ground_me(object_hdr);
        plyr_anim_pdata->step = 1.0f;
        break;
    case 1:
        blend_to_fstance(blend_rate);
        break;
    case 2:
        glitch_to_fstance(blend_rate);
        break;
    }
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void j_exit_react(void) {
    exit_reaction_common();
}

void j_exit_6(void) {
    exit_reaction_common();
}

float j_blend_to_fstance_in_x(void) {
    float remaining_frames;

    if (plyr_pdata->summon_position_x > 30.0f ||
        plyr_pdata->summon_position_x < 3.0f) {
        plyr_pdata->summon_position_x = 3.0f;
    }
    remaining_frames =
        plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
    if (remaining_frames > plyr_pdata->summon_position_x) {
        ani_to_blend_frame(plyr_pdata->summon_position_x);
    }
    remaining_frames =
        plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
    if (remaining_frames < 1.0f) {
        plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;
        pose_anim(plyr_anim_pdata, 1);
    } else {
        plyr_anim_pdata->step =
            remaining_frames / plyr_pdata->summon_position_x;
    }
    blend_to_fstance(1.0f / plyr_pdata->summon_position_x);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float j_blend_to_stance_in_x(void) {
    MkHdr* object_hdr;
    float remaining_frames;
    float blend_rate;

    if (plyr_pdata->summon_position_x > 30.0f ||
        plyr_pdata->summon_position_x < 3.0f) {
        plyr_pdata->summon_position_x = 3.0f;
    }
    remaining_frames =
        plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
    if (remaining_frames > plyr_pdata->summon_position_x) {
        ani_to_blend_frame(plyr_pdata->summon_position_x);
    }
    remaining_frames =
        plyr_anim_pdata->high_frame - plyr_anim_pdata->frame;
    if (remaining_frames < 1.0f) {
        plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;
        pose_anim(plyr_anim_pdata, 1);
    } else {
        plyr_anim_pdata->step =
            remaining_frames / plyr_pdata->summon_position_x;
    }
    blend_rate = 1.0f / plyr_pdata->summon_position_x;
    if (plyr_anim_pdata->last_exec_tick ==
        (unsigned int)exec_tick_ctr) {
        ejb_sleep_ticks(1.0f);
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100) != 0) {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->crouching_animation,
            0x20, blend_rate);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->standing_animation,
            0x20, blend_rate);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static inline void glitch_to_stance_impl(float blend_rate) {
    MkHdr* object_hdr;

    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->crouching_animation,
            0x20, blend_rate);
    } else {
        set_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->standing_animation,
            0x20);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
}

float p_glitch_to_stance(void) {
    plyr_anim_pdata->step = 1.0f;
    glitch_to_stance_impl(0.5f);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float p_reverse_to_stance_in_10(void) {
    AnimPdata* animation;
    float target_frame;

    plyr_anim_pdata->step = 1.0f;
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.reverse_to_stance, 3, 0.1f);
    ejb_sleep_ticks(1.0f);
    animation = plyr_anim_pdata;
    target_frame = 11.0f;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

void p_comboexit_to_stance(void) {
    AnimPdata* animation;
    float target_frame;

    plyr_anim_pdata->step = plyr_pdata->summon_position_x;
    animation = plyr_anim_pdata;
    target_frame = plyr_pdata->summon_position_z;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    blend_to_stance_inline(0.1f);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void glitch_to_ani(AniData* animation, int transition) {
    set_anim_script(plyr_anim_pdata, animation, transition);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

void turn_me_pi(void) {
    plyr_obj->ang.y += 3.1415927f;
}

void ejb_release_other_player(int reaction) {
    release_other_player();
    his_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.face_opponent = 0;
    reaction_xfer_him(reaction, 2.0f, 0);
}

void ejb_too_close_repell(void) {
    MkObj* player_one;
    MkObj* player_two;
    float delta_z;
    float delta_x;

    player_one = g_game_info.plyr0.slot.mirror_a;
    player_two = g_game_info.plyr1.slot.mirror_a;
    delta_z = player_one->pos.z - player_two->pos.z;
    delta_x = player_one->pos.x - player_two->pos.x;
    if (delta_x * delta_x + delta_z * delta_z < 1.0f) {
        reaction_xfer_him(0x135, 2.0f, 0);
    }
}

static inline EjbBoneMatcherView* prepare_two_player_animation(
    int self_flip_mode, int opponent_flip_mode, int animate_opponent) {
    PlyrPdata* opponent;
    MkProc* process;
    MkProc* opponent_anim_proc;
    MkObj* tracked_object;
    MkObj* held_by_object;
    AnimPdata* opponent_anim;
    EjbBoneMatcherView* matcher;

    process = plyr_pdata->player_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->player_proc_instance) {
        process = 0;
    }
    if (process != 0) {
        xfer_proc(process, p_idle);
        his_pdata->previous_state = his_pdata->state;
        his_pdata->state = 0xC603;
    }

    opponent = plyr_pdata->his_plyr_pdata;
    opponent_anim_proc = opponent->anim_proc;
    if (opponent_anim_proc != 0 &&
        opponent_anim_proc->instance != opponent->anim_proc_instance) {
        opponent_anim_proc = 0;
    }
    if (opponent_anim_proc != 0) {
        xfer_proc(opponent_anim_proc, p_anim_idle);
    }

    tracked_object = opponent->tracked_obj;
    if (tracked_object != 0 &&
        tracked_object->hdr.instance != opponent->tracked_obj_instance) {
        tracked_object = 0;
    }
    if (tracked_object != 0) {
        process = opponent->transient_proc;
        if (process != 0 &&
            process->instance != opponent->transient_proc_instance) {
            process = 0;
        }
        if (process != 0 && process != aproc && process->instance != 0) {
            process->vtbl->destroy(process);
        }
        tracked_object->pos_vel.x = 0.0f;
        tracked_object->pos_vel.y = 0.0f;
        tracked_object->pos_vel.z = 0.0f;
        tracked_object->gravity = 0.0f;
    }

    matcher = (EjbBoneMatcherView*)plyr_grab_other_flip_states(
        self_flip_mode, opponent_flip_mode);
    plyr_obj->hide_flag_bits.still_move = 0;

    held_by_object = opponent->held_by_object_latch.obj;
    if (held_by_object != 0 &&
        held_by_object->hdr.instance !=
            opponent->held_by_object_latch.instance) {
        held_by_object = 0;
    }
    if (held_by_object == 0) {
        opponent->held_by_object_latch.obj = plyr_obj;
        opponent->held_by_object_latch.instance = plyr_obj->hdr.instance;
    }

    opponent_anim_proc = opponent->anim_proc;
    if (opponent_anim_proc != 0 &&
        opponent_anim_proc->instance != opponent->anim_proc_instance) {
        opponent_anim_proc = 0;
    }
    if (opponent_anim_proc != 0) {
        opponent_anim = (AnimPdata*)pdata_of_proc(opponent_anim_proc);
        if (opponent_anim != 0) {
            if (animate_opponent != 0) {
                xfer_proc(opponent_anim_proc, p_animate);
                opponent_anim->transition_weight = 1.0f;
                opponent_anim->transition_step = -0.1f;
            } else {
                matcher->position_weight = 0.0f;
                matcher->rotation_weight = 0.0f;
            }
        }
    }
    return matcher;
}

float two_player_animation_match_attacker(
    AniData* animation, float attacker_step) {
    int opponent_flip_mode;

    opponent_flip_mode =
        plyr_obj->hide_flag_bits.bit6 != 0 ? 2 : 1;
    prepare_two_player_animation(0, opponent_flip_mode, 1);
    plyr_anim_pdata->transition_step = 0.1f;
    plyr_anim_pdata->transition_weight = 0.1f;
    plyr_anim_pdata->transition_target = 1.0f;
    transition_to_anim_script(
        plyr_anim_pdata, animation, 3, 0.5f);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = attacker_step;
    return 0.0f;
}

float two_player_animation_blend(
    AniData* animation, int attacker_mode, int victim_mode,
    float attacker_step, float victim_frame) {
    int opponent_flip_mode;

    opponent_flip_mode = victim_mode != 0 ? 2 : 1;
    prepare_two_player_animation(
        1, opponent_flip_mode, attacker_mode);
    plyr_anim_pdata->transition_step = 0.1f;
    plyr_anim_pdata->transition_weight = 0.1f;
    plyr_anim_pdata->transition_target = 1.0f;
    if (attacker_mode != 0) {
        transition_to_anim_script(
            plyr_anim_pdata, animation, 3, 0.5f);
        ejb_sleep_ticks(1.0f);
        plyr_anim_pdata->step = attacker_step;
    } else {
        plyr_anim_pdata->transition_weight = 1.0f;
        plyr_anim_pdata->step = attacker_step;
        set_anim_script_frame(
            plyr_anim_pdata, animation, 3, victim_frame);
    }
    return 0.0f;
}

float two_player_animation_flip(
    AniData* animation, float attacker_step) {
    prepare_two_player_animation(2, 2, 1);
    plyr_anim_pdata->transition_step = 0.1f;
    plyr_anim_pdata->transition_weight = 0.1f;
    plyr_anim_pdata->transition_target = 1.0f;
    transition_to_anim_script(
        plyr_anim_pdata, animation, 3, 0.5f);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = attacker_step;
    return 0.0f;
}

void two_player_animation(
    AniData* animation, void* unused, float attacker_blend) {
    (void)unused;
    two_player_animation_blend(
        animation, 1, 0, attacker_blend, 0.0f);
}

void idle_victim(void) {
    MkObj* tracked_object;
    PlyrPdata* opponent;
    MkProc* process;

    process = plyr_pdata->player_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->player_proc_instance) {
        process = 0;
    }
    if (process != 0) {
        xfer_proc(process, p_idle);
        his_pdata->previous_state = his_pdata->state;
        his_pdata->state = 0xC603;
    }

    opponent = plyr_pdata->his_plyr_pdata;
    process = opponent->anim_proc;
    if (process != 0 &&
        process->instance != opponent->anim_proc_instance) {
        process = 0;
    }
    if (process != 0) {
        xfer_proc(process, p_anim_idle);
    }

    tracked_object = opponent->tracked_obj;
    if (tracked_object != 0 &&
        tracked_object->hdr.instance != opponent->tracked_obj_instance) {
        tracked_object = 0;
    }
    if (tracked_object != 0) {
        process = opponent->transient_proc;
        if (process != 0 &&
            process->instance != opponent->transient_proc_instance) {
            process = 0;
        }
        if (process != 0 && process != aproc && process->instance != 0) {
            process->vtbl->destroy(process);
        }
        tracked_object->pos_vel.x = 0.0f;
        tracked_object->pos_vel.y = 0.0f;
        tracked_object->pos_vel.z = 0.0f;
        tracked_object->gravity = 0.0f;
    }
}

int ani_col_abort(
    int attack_region, int reaction, int strength,
    float target_frame, float region_scale,
    float attack_scale, float reaction_rate) {
    int passed_target;
    int collision_result;
    int collision_blocked;
    int counter_allowed;
    int state;

    plyr_pdata->pending_hit_strength = strength;
    passed_target = 0;
    plyr_pdata->attack_region = attack_region;
    g_no_throw_f = 0;
    start_plyr_attack(attack_scale);

    if (plyr_anim_pdata->frame <= target_frame) {
        plyr_pdata->attack_region = attack_region;
        if (his_pdata->collision_disabled != 0) {
            collision_result = 0;
        } else {
            set_plyr_attack_region(
                attack_region, region_scale, 0.0f);
            collision_result = collide_plyr_vs_plyr();
            if (collision_result == 1) {
                trial_state_collision_check(
                    collision_result, aproc->pid == 0x1001);
            }
        }
        if (collision_result != 0) {
            if (is_plyr_blocking(his_pdata) != 0) {
                plyr_pdata->collision_result = 2;
            } else {
                plyr_pdata->collision_result = 1;
            }
            if (reaction_xfer_him(
                    reaction, reaction_rate, strength) == 0) {
                plyr_pdata->collision_result = 2;
            }
        }
    }

    while (plyr_anim_pdata->frame <= target_frame &&
           passed_target == 0 &&
           plyr_pdata->collision_result == -1) {
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        ejb_sleep_ticks(1.0f);
        if (plyr_anim_pdata->frame > target_frame) {
            passed_target = 1;
            plyr_anim_pdata->frame = target_frame;
        }
        if (plyr_pdata->collision_result != -1) {
            continue;
        }

        collision_blocked = 0;
        if (strength == 5 && his_pdata->state == 0x101) {
            collision_blocked = 1;
        }
        if (strength == 5 && his_pdata->state == 0x302) {
            collision_blocked = 1;
        }
        if (is_big_boss(plyr_pdata) != 0) {
            collision_blocked = 1;
        }
        if (collision_blocked != 0) {
            continue;
        }

        plyr_pdata->attack_region = attack_region;
        if (his_pdata->collision_disabled != 0) {
            collision_result = 0;
        } else {
            set_plyr_attack_region(
                attack_region, region_scale, 0.0f);
            collision_result = collide_plyr_vs_plyr();
            if (collision_result == 1) {
                trial_state_collision_check(
                    collision_result, aproc->pid == 0x1001);
            }
        }
        if (collision_result == 0) {
            continue;
        }

        if (is_plyr_blocking(his_pdata) != 0) {
            plyr_pdata->collision_result = 2;
        } else {
            plyr_pdata->collision_result = 1;
        }
        if (his_pdata->state != 0x6205) {
            counter_allowed = 0;
        } else if (plyr_pdata->state == 0x120B) {
            counter_allowed = 0;
        } else if (plyr_pdata->state == 0x120C) {
            counter_allowed = 0;
        } else if (is_weapon_style(
                       plyr_pdata->fighter_definition) != 0) {
            counter_allowed = 0;
        } else {
            state = plyr_pdata->state;
            switch (state) {
            case 0x1210:
            case 0x1211:
            case 0x120B:
            case 0x3204:
            case 0x3205:
                counter_allowed = 0;
                break;
            default:
                if (ejb_function_pointers[42] != 0) {
                    ejb_function_pointers[42]();
                }
                counter_allowed = 1;
                break;
            }
        }
        if (counter_allowed != 0) {
            reaction_xfer_him(0xBE, 0.0f, 2);
            plyr_pdata->script_exit_value_int = attack_region;
            ((EjbProcSleepVtable*)aproc->vtbl)->transfer(
                j_counter_caught, 0.0f);
            return 0;
        }
        if (reaction_xfer_him(
                reaction, reaction_rate, strength) == 0) {
            plyr_pdata->collision_result = 2;
        }
    }

    plyr_pdata->throw_restriction = 3;
    if (plyr_pdata->collision_result == -1 ||
        plyr_pdata->collision_result == 3) {
        if (local_collision_allowed(plyr_pdata) != 0) {
            plyr_pdata->collision_result = 0;
        }
        trial_state_collision_check(
            0, plyr_pdata == g_game_info.plyr0.slot.pdata);
    }
    if (plyr_pdata->collision_result != 0) {
        return 1;
    }
    return 0;
}

void check_to_register_miss(void) {
    int collision_result;

    collision_result = plyr_pdata->collision_result;
    if (collision_result == -1 || collision_result == 3) {
        if (local_collision_allowed(plyr_pdata) != 0) {
            plyr_pdata->collision_result = 0;
        }
        trial_state_collision_check(
            0, plyr_pdata == g_game_info.plyr0.slot.pdata);
    }
}

float ani_to_frame_x_col(
    int attack_region, int reaction, int strength,
    float target_frame, float horizontal_scale,
    float vertical_scale, float reaction_rate) {
    int passed_target;
    int advance_result;
    int collision_result;
    int collision_blocked;
    int counter_allowed;
    int state;
    float adjusted_horizontal;
    float adjusted_vertical;

    adjusted_horizontal = horizontal_scale;
    adjusted_vertical = vertical_scale;
    online_combo_adjust(&adjusted_horizontal, &adjusted_vertical);
    passed_target = 0;
    plyr_pdata->pending_reaction = reaction;
    plyr_pdata->pending_hit_strength = strength;
    plyr_pdata->attack_region = attack_region;
    g_no_throw_f = 0;
    start_plyr_attack(adjusted_vertical);

    if (plyr_anim_pdata->frame <= target_frame) {
        plyr_pdata->attack_region = attack_region;
        if (his_pdata->collision_disabled != 0) {
            collision_result = 0;
        } else {
            set_plyr_attack_region(
                attack_region, 0.0f, adjusted_horizontal);
            collision_result = collide_plyr_vs_plyr();
            if (collision_result == 1) {
                trial_state_collision_check(
                    collision_result, aproc->pid == 0x1001);
            }
        }
        if (collision_result != 0) {
            if (is_plyr_blocking(his_pdata) != 0) {
                plyr_pdata->collision_result = 2;
            } else {
                plyr_pdata->collision_result = 1;
            }
            if (reaction_xfer_him(
                    reaction, reaction_rate, strength) == 0) {
                plyr_pdata->collision_result = 2;
            }
        }
    }

    for (;;) {
        if (plyr_anim_pdata->frame > target_frame ||
            passed_target != 0) {
            break;
        }
        advance_result = advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        ejb_sleep_ticks(1.0f);
        if (plyr_anim_pdata->frame > target_frame) {
            passed_target = 1;
            plyr_anim_pdata->frame = target_frame;
        }

        if (plyr_pdata->collision_result == -1 ||
            plyr_pdata->collision_result == 3) {
            collision_blocked = 0;
            switch (strength) {
            case 0: {
                int opponent_state = his_pdata->state;

                switch (opponent_state) {
                case 0x100:
                case 0x101:
                case 0x302:
                case 0x900:
                case 0x901:
                case 0x1300:
                    collision_blocked = 1;
                    break;
                default:
                    break;
                }
                break;
            }
            case 5: {
                int opponent_state = his_pdata->state;

                switch (opponent_state) {
                case 0x101:
                case 0x302:
                case 0x900:
                    collision_blocked = 1;
                    break;
                default:
                    break;
                }
                break;
            }
            case 6: {
                MkObj* opponent_object = plyr_pdata->his_obj;
                int opponent_state = his_pdata->state;

                if ((opponent_state & 0x1000) != 0) {
                    if (his_pdata->throw_restriction != 3 &&
                        (unsigned int)(game_tick_ctr -
                            his_pdata->last_collision_tick) >= 10) {
                        collision_blocked = 1;
                    }
                }
                if (opponent_state == 0x120C) {
                    collision_blocked = 0;
                }
                if (is_big_boss(
                        plyr_pdata->his_plyr_pdata) == 0) {
                    if (opponent_state == 0x605) {
                        collision_blocked = 1;
                    } else if (opponent_state == 0x3202) {
                        collision_blocked = 1;
                    } else if (opponent_state == -0x39FE) {
                        collision_blocked = 1;
                    } else if (opponent_state == 0x6001) {
                        collision_blocked = 1;
                    } else if (opponent_state == 0x60C) {
                        collision_blocked = 0;
                    } else if (opponent_object->pos_vel.y != 0.0f &&
                               opponent_object->gravity != 0.0f) {
                        collision_blocked = 1;
                    }
                }
                if (is_big_boss(his_pdata) != 0) {
                    collision_blocked = 1;
                }
                if (opponent_state == 0x101) {
                    collision_blocked = 1;
                }
                if (opponent_state == 0x302) {
                    collision_blocked = 1;
                }
                if (opponent_state == 0x900) {
                    collision_blocked = 1;
                }
                if (opponent_state == 0x901) {
                    collision_blocked = 1;
                }
                if ((opponent_state & 0x400) != 0 &&
                    opponent_state != -0x3A00 &&
                    opponent_state != -0x39FF &&
                    opponent_state != 0x4207) {
                    collision_blocked = 1;
                }
                if (collision_blocked != 0 &&
                    (plyr_pdata->collision_result == -1 ||
                     plyr_pdata->collision_result == 3)) {
                    if (local_collision_allowed(plyr_pdata) != 0) {
                        plyr_pdata->collision_result = 0;
                    }
                    trial_state_collision_check(
                        0, plyr_pdata == g_game_info.plyr0.slot.pdata);
                }
                break;
            }
            case 7:
                if (his_pdata->state == 0x901) {
                    collision_blocked = 1;
                }
                break;
            case 8:
            default:
                break;
            }

            if (collision_blocked == 0) {
                plyr_pdata->attack_region = attack_region;
                if (his_pdata->collision_disabled != 0) {
                    collision_result = 0;
                } else {
                    set_plyr_attack_region(
                        attack_region, 0.0f, adjusted_horizontal);
                    collision_result = collide_plyr_vs_plyr();
                    if (collision_result == 1) {
                        trial_state_collision_check(
                            collision_result, aproc->pid == 0x1001);
                    }
                }
                if (collision_result != 0) {
                    if (is_plyr_blocking(his_pdata) != 0) {
                        plyr_pdata->collision_result = 2;
                    } else {
                        plyr_pdata->collision_result = 1;
                    }
                    if ((plyr_pdata->secondary_state & 0x400) != 0) {
                        passed_target = 1;
                    }
                    if (his_pdata->state != 0x6205) {
                        counter_allowed = 0;
                    } else if (plyr_pdata->state == 0x120B) {
                        counter_allowed = 0;
                    } else if (plyr_pdata->state == 0x120C) {
                        counter_allowed = 0;
                    } else if (is_weapon_style(
                                   plyr_pdata->fighter_definition) != 0) {
                        counter_allowed = 0;
                    } else {
                        state = plyr_pdata->state;
                        switch (state) {
                        case 0x1210:
                        case 0x1211:
                        case 0x120B:
                        case 0x3204:
                        case 0x3205:
                            counter_allowed = 0;
                            break;
                        default:
                            if (ejb_function_pointers[42] != 0) {
                                ejb_function_pointers[42]();
                            }
                            counter_allowed = 1;
                            break;
                        }
                    }
                    if (counter_allowed != 0) {
                        reaction_xfer_him(0xBE, 0.0f, 2);
                        plyr_pdata->script_exit_value_int = attack_region;
                        ((EjbProcSleepVtable*)aproc->vtbl)->transfer(
                            j_counter_caught, 0.0f);
                        return 0.0f;
                    }
                    if (reaction_xfer_him(
                            reaction, reaction_rate, strength) == 0) {
                        plyr_pdata->collision_result = 2;
                    }
                }
            }
        }
        if (advance_result == 0) {
            break;
        }
    }

    plyr_pdata->throw_restriction = 3;
    if (plyr_pdata->collision_result == -1 ||
        plyr_pdata->collision_result == 3) {
        if (local_collision_allowed(plyr_pdata) != 0) {
            plyr_pdata->collision_result = 0;
        }
        trial_state_collision_check(
            0, plyr_pdata == g_game_info.plyr0.slot.pdata);
    }
    online_combo_record();
    return 0.0f;
}

void air_collision_pause(
    int pause_ticks, float target_frame, float gravity) {
    MkProc* process;
    float saved_step;

    if (plyr_pdata->collision_result != -1) {
        plyr_obj->flags_09_bits.launched = 0;
        process = plyr_pdata->transient_proc;
        if (process != 0 &&
            process->instance !=
                (int)plyr_pdata->transient_proc_instance) {
            process = 0;
        }
        if (process != 0 && process != aproc &&
            process->instance != 0) {
            ((EjbProcSleepVtable*)process->vtbl)->destroy();
        }
        plyr_obj->pos_vel.x = 0.0f;
        plyr_obj->pos_vel.y = 0.0f;
        plyr_obj->pos_vel.z = 0.0f;
        plyr_obj->gravity = 0.0f;
        if (pause_ticks != 0 &&
            target_frame > plyr_anim_pdata->frame) {
            AnimPdata* animation;

            saved_step = plyr_anim_pdata->step;
            plyr_anim_pdata->step =
                (target_frame - plyr_anim_pdata->frame) /
                (float)pause_ticks;
            animation = plyr_anim_pdata;
            if (target_frame > animation->high_frame) {
                target_frame = animation->high_frame;
            }
            EJB_ADVANCE_TO_FRAME(animation, target_frame);
            plyr_anim_pdata->step = saved_step;
        } else {
            _mkproc_sleep_ticks = (float)pause_ticks;
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->gravity = gravity;
}

int collision_2(int attack_region) {
    int collision_result;

    plyr_pdata->attack_region = attack_region;
    if (his_pdata->collision_disabled != 0) {
        return 0;
    }

    set_plyr_attack_region(attack_region, 0.0f, 0.0f);
    collision_result = collide_plyr_vs_plyr();
    if (collision_result == 1) {
        trial_state_collision_check(
            collision_result, aproc->pid == 0x1001);
    }
    return collision_result;
}

void wait_to_land(void) {
    MkProc* process;

    plyr_obj->flags_09_bits.launched = 1;
    while (plyr_obj->flags_08_bits.moving &&
           plyr_obj->gravity != 0.0f) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance !=
            (int)plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        ((EjbProcSleepVtable*)process->vtbl)->destroy();
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_09_bits.bit6 = 1;
}

void set_collision_made_flag(void) {
    if (is_plyr_blocking(his_pdata) != 0) {
        plyr_pdata->collision_result = 2;
        return;
    }
    plyr_pdata->collision_result = 1;
}

/*
 * Soft ceiling: the two callers contain the complete retail state machine.
 * Source is eight bytes larger from separate saved-register stores/restores;
 * remaining records are allocation, float scheduling, and local labels.
 */
static inline int joypad_state_5_impl(PlyrPdata* pdata) {
    int state = pdata->state;

    if (pdata->drone_request != 0) {
        return 0;
    }
    if (check_switch(pdata->switch_data, 0xC) != 0) {
        return 1;
    }
    if (check_switch(pdata->switch_data, 0xE) != 0) {
        if (state == 0x100) {
            return 0;
        }
        return 2;
    }
    if (check_switch(pdata->switch_data, 0xD) != 0) {
        MkObj* object = pdata->plyr_info->slot.mirror_a;
        MkObj* opponent = pdata->his_plyr_pdata->plyr_info->slot.mirror_a;
        float camera_z = camera_obj->pos_z;
        float camera_x = camera_obj->pos_x;
        float direction =
            (object->pos_x - camera_x) * -(opponent->pos_z - camera_z) -
            (opponent->pos_x - camera_x) * -(object->pos_z - camera_z);

        return direction < 0.0f ? 3 : 4;
    }
    if (check_switch(pdata->switch_data, 0xF) != 0) {
        MkObj* object = pdata->plyr_info->slot.mirror_a;
        MkObj* opponent = pdata->his_plyr_pdata->plyr_info->slot.mirror_a;
        float camera_z = camera_obj->pos_z;
        float camera_x = camera_obj->pos_x;
        float direction =
            (object->pos_x - camera_x) * -(opponent->pos_z - camera_z) -
            (opponent->pos_x - camera_x) * -(object->pos_z - camera_z);

        return direction > 0.0f ? 3 : 4;
    }
    return 0;
}

int my_joypad_state_5(void) {
    return joypad_state_5_impl(plyr_pdata);
}

int joypad_state_5(PlyrPdata* pdata) {
    return joypad_state_5_impl(pdata);
}

void my_pad_position(void) {
    if (plyr_pdata->drone_request != 0) {
        return;
    }
    if (check_switch(plyr_pdata->switch_data, 0xF) &&
        check_switch(plyr_pdata->switch_data, 0xE)) {
        return;
    }
    if (check_switch(plyr_pdata->switch_data, 0xD) &&
        check_switch(plyr_pdata->switch_data, 0xE)) {
        return;
    }
    check_switch(plyr_pdata->switch_data, 0xC);
}

float which_way_is_towards(void) {
    float camera_z;
    float camera_x;

    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    return ((plyr_obj->pos_x - camera_x) *
            -(his_obj->pos_z - camera_z)) -
           ((his_obj->pos_x - camera_x) *
            -(plyr_obj->pos_z - camera_z));
}

void play_sound_1(int sound) {
    snd_req(sound);
}

void play_sound_2(int first_sound, int second_sound) {
    snd_req(first_sound);
    snd_req(second_sound);
}

static inline void start_plyr_force(
    int duration, int animation, float damping) {
    EjbPlyrForcePdata* force_data;
    MkProc* process;

    force_data = 0;
    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        process = _create_mkproc_generic_tinystack(
            0x1005, 0xA, p_force_away,
            sizeof(EjbPlyrForcePdata), (MkHdr**)&force_data);
    } else if (plyr_obj == g_game_info.plyr1.slot.mirror_a) {
        process = _create_mkproc_generic_tinystack(
            0x1006, 0xA, p_force_away,
            sizeof(EjbPlyrForcePdata), (MkHdr**)&force_data);
    } else {
        process = 0;
    }
    if (process != 0) {
        force_data->object = plyr_obj;
        force_data->object_instance = plyr_obj->hdr.instance;
        force_data->player = plyr_pdata;
        force_data->player_instance = plyr_pdata->instance;
        force_data->delay = duration;
        force_data->velocity_scale = damping;
        force_data->iterations = animation;
        process->pre_destroy = pw_plyr_force;
        process->destroy_cb = ps_plyr_force;
        plyr_pdata->transient_proc = process;
        plyr_pdata->transient_proc_instance = process->instance;
    }
}

void force_forward(
    int duration, int animation, float force, float damping) {
    myvel_my_angle_y(0.0f, force, force);
    start_plyr_force(duration, animation, damping);
}

void force_away(
    int duration, int animation, float force, float damping) {
    myvel_my_angle_y(3.1428f, force, force);
    start_plyr_force(duration, animation, damping);
}

float p_force_away(void) {
    EjbPlyrForcePdata* force;
    int iteration;

    force = (EjbPlyrForcePdata*)plyr_force_pdata;
    _mkproc_sleep_ticks = (float)force->delay;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    if (force->iterations > 0 && force->iterations < 60) {
        for (iteration = 0; iteration < force->iterations; iteration++) {
            plyr_obj->pos_vel.x *= force->velocity_scale;
            plyr_obj->pos_vel.z *= force->velocity_scale;
            _mkproc_sleep_ticks = 1.0f;
            ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    return 0.0f;
}

void danger_zone_eligible_on(void) {
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0x609;
}

void wall_eligible_off(void) {
    int state;

    plyr_obj->flags_09_bits.wall_restricted = 0;
    state = plyr_pdata->state;
    if (state == 0x602) {
        plyr_pdata->previous_state = state;
        plyr_pdata->state = 0x600;
    }
}

void wall_eligible_on(void) {
    plyr_obj->flags_09_bits.wall_restricted = 0;
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0x602;
}

void create_wall_monitor(void) {
    _create_mkproc_generic_nostack(
        0x1008, 0x1B, p_wall_monitor, 0, 0);
}

float p_wall_monitor(void) {
    MkObj* player_one_object;
    MkObj* player_two_object;
    PlyrPdata* player_one;
    PlyrPdata* player_two;

    player_one_object = g_game_info.plyr0.slot.mirror_a;
    player_two_object = g_game_info.plyr1.slot.mirror_a;
    if (player_one_object == 0 || player_two_object == 0) {
        return 0.0f;
    }

    player_one = g_game_info.plyr0.slot.pdata;
    player_two = g_game_info.plyr1.slot.pdata;
    if (player_one_object->flags_09_bits.wall_restricted &&
        player_one->state == 0x602) {
        xfer_player_proc(
            (MkProc*)g_game_info.plyr0.idle_proc, r_hit_wall);
    }
    if (player_two_object->flags_09_bits.wall_restricted &&
        player_two->state == 0x602) {
        xfer_player_proc(
            (MkProc*)g_game_info.plyr1.idle_proc, r_hit_wall);
    }
    return 0.0f;
}

void ps_plyr_force(void) {
    plyr_force_pdata = 0;
    plyr_pdata = 0;
    plyr_obj = 0;
}

void pw_plyr_force(void) {
    EjbPlyrForcePdata* force;
    MkObj* object;
    PlyrPdata* player;

    force = (EjbPlyrForcePdata*)apdata;
    plyr_force_pdata = force;

    object = force->object;
    if (object != 0 && object->hdr.instance != force->object_instance) {
        object = 0;
    }
    plyr_obj = object;

    player = force->player;
    if (player != 0 && player->instance != force->player_instance) {
        player = 0;
    }
    plyr_pdata = player;
}

void stop_me(void) {
    MkProc* process;

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = 0.0f;
}

float xz_distance_between_players(void) {
    MkObj* player_one;
    MkObj* player_two;
    float delta_z;
    float delta_x;

    player_one = g_game_info.plyr0.slot.mirror_a;
    player_two = g_game_info.plyr1.slot.mirror_a;
    delta_z = player_one->pos.z - player_two->pos.z;
    delta_x = player_one->pos.x - player_two->pos.x;
    return delta_x * delta_x + delta_z * delta_z;
}

void myvel_his_angle_y_inout(
    float angle_offset, float x_velocity, float z_velocity) {
    float camera_x = camera_obj->pos_x;
    float camera_z = camera_obj->pos_z;
    float cross;
    float angle;
    float sine = 0.0f;
    float cosine = 0.0f;

    cross = (plyr_obj->pos.x - camera_x) *
                -(his_obj->pos.z - camera_z) -
            (his_obj->pos.x - camera_x) *
                -(plyr_obj->pos.z - camera_z);
    if (is_his_chest_to_screen() != 0) {
        if (cross < 0.0f) {
            angle_offset *= -1.0f;
        }
    } else if (cross >= 0.0f) {
        angle_offset *= -1.0f;
    }
    if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
        angle = g_game_info.plyr1.slot.mirror_a->ang.y + angle_offset;
        angle = 0.000005992112f *
                (float)((int)(166886.1f * angle) & 0xFFFFF);
        sine = gxMathSin(angle);
        cosine = gxMathCos(angle);
    } else if (plyr_pdata == g_game_info.plyr1.slot.pdata) {
        angle = g_game_info.plyr0.slot.mirror_a->ang.y + angle_offset;
        angle = 0.000005992112f *
                (float)((int)(166886.1f * angle) & 0xFFFFF);
        sine = gxMathSin(angle);
        cosine = gxMathCos(angle);
    }
    plyr_obj->pos_vel.x = sine * x_velocity;
    plyr_obj->pos_vel.z = cosine * z_velocity;
}

void myvel_my_angle_y(
    float angle_offset, float x_velocity, float z_velocity) {
    MkProc* process;
    float angle;
    int flipped;

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance != plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process->instance != 0) {
        process->vtbl->destroy(process);
    }
    plyr_pdata->transient_proc = 0;
    plyr_pdata->transient_proc_instance = 0;
    swap_active_plyr_proc();
    flipped = plyr_obj->hide_flag_bits.bit6 != 0;
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    swap_active_plyr_proc();
    angle = plyr_obj->ang.y + (flipped ? -angle_offset : angle_offset);
    angle = 0.000005992112f *
            (float)((int)(166886.1f * angle) & 0xFFFFF);
    plyr_obj->pos_vel.x = gxMathSin(angle) * x_velocity;
    plyr_obj->pos_vel.z = gxMathCos(angle) * z_velocity;
}

/*
 * Honest soft ceiling: retail leaves sine/cosine undefined for an impossible
 * non-player pdata value. Clean C initializes them, accounting for the 16-byte
 * excess and the remaining FPR allocation/move differences.
 */
void myvel_his_angle_y(
    float angle_offset, float x_velocity, float z_velocity) {
    float angle;
    float sine = 0.0f;
    float cosine = 0.0f;
    int flipped;

    swap_active_plyr_proc();
    flipped = 0;
    if (plyr_obj->hide_flag_bits.bit6 == 1) {
        flipped ^= 1;
    }
    if (plyr_anim_pdata->flags & 8) {
        flipped ^= 1;
    }
    swap_active_plyr_proc();
    if (flipped != 0) {
        if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
            angle = g_game_info.plyr1.slot.mirror_a->ang.y - angle_offset;
            angle = 0.000005992112f *
                    (float)((int)(166886.1f * angle) & 0xFFFFF);
            sine = gxMathSin(angle);
            cosine = gxMathCos(angle);
        } else if (plyr_pdata == g_game_info.plyr1.slot.pdata) {
            angle = g_game_info.plyr0.slot.mirror_a->ang.y - angle_offset;
            angle = 0.000005992112f *
                    (float)((int)(166886.1f * angle) & 0xFFFFF);
            sine = gxMathSin(angle);
            cosine = gxMathCos(angle);
        }
    } else {
        if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
            angle = g_game_info.plyr1.slot.mirror_a->ang.y + angle_offset;
            angle = 0.000005992112f *
                    (float)((int)(166886.1f * angle) & 0xFFFFF);
            sine = gxMathSin(angle);
            cosine = gxMathCos(angle);
        } else if (plyr_pdata == g_game_info.plyr1.slot.pdata) {
            angle = g_game_info.plyr0.slot.mirror_a->ang.y + angle_offset;
            angle = 0.000005992112f *
                    (float)((int)(166886.1f * angle) & 0xFFFFF);
            sine = gxMathSin(angle);
            cosine = gxMathCos(angle);
        }
    }
    plyr_obj->pos_vel.x = sine * x_velocity;
    plyr_obj->pos_vel.z = cosine * z_velocity;
}

void uv_my_angle_y(Vec* direction, float angle_offset) {
    float angle;
    float wrapped_angle;

    angle = plyr_obj->ang.y + angle_offset;
    wrapped_angle =
        0.000005992112f *
        (float)(((int)(166886.1f * angle)) & 0xFFFFF);
    direction->x = gxMathSin(wrapped_angle);
    direction->y = 0.0f;
    direction->z = gxMathCos(wrapped_angle);
}

int super_charge_me(void) {
    EjbSuperchargeView* player;
    EjbScalePdata* scale_pdata;
    int recharge_ticks;

    player = (EjbSuperchargeView*)plyr_pdata;
    if (player->recharge_tick >= (unsigned int)game_tick_ctr) {
        snd_req(0xD88);
        return 0;
    }

    recharge_ticks =
        (int)(660.0f * inverse_game_speed + 0.5f);
    player->recharge_tick = game_tick_ctr + recharge_ticks;
    player->charge_scale = 1.3f;
    player->active_until =
        game_tick_ctr +
        (int)(60.0f * inverse_game_speed + 0.5f);
    scale_pdata = (EjbScalePdata*)player->scale_pdata;
    if (scale_pdata != 0 &&
        scale_pdata->instance != player->scale_pdata_instance) {
        scale_pdata = 0;
    }
    if (scale_pdata != 0) {
        scale_pdata->scale = plyr_obj->scale;
        scale_pdata->script = scale_script_chargeup;
        scale_pdata->current_script = scale_script_chargeup;
        scale_pdata->script_frame = 0.0f;
    } else {
        scale_pdata = (EjbScalePdata*)start_scale_proc(
            plyr_obj, scale_script_chargeup);
        player->scale_pdata = (MkHdr*)scale_pdata;
        player->scale_pdata_instance = scale_pdata->instance;
    }
    return 1;
}

void disable_supercharge(void) {
    if (plyr_pdata->player_slot == 0) {
        plyr_pdata->attack_disable_tick_p1 =
            game_tick_ctr +
            (int)(120.0f * inverse_game_speed + 0.5f);
    }
    if (plyr_pdata->player_slot == 1) {
        plyr_pdata->attack_disable_tick_p2 =
            game_tick_ctr +
            (int)(120.0f * inverse_game_speed + 0.5f);
    }
}

void adjust_my_damage_multiplier(float multiplier) {
    plyr_pdata->damage_multiplier *= multiplier;
    if (plyr_pdata->damage_multiplier < 0.01f) {
        plyr_pdata->damage_multiplier = 0.01f;
    }
}

void set_his_damage_multiplier(float multiplier) {
    if (aproc->pid == 0x1001) {
        g_game_info.plyr1.slot.pdata->damage_multiplier = multiplier;
        return;
    }
    g_game_info.plyr0.slot.pdata->damage_multiplier = multiplier;
}

void adjust_his_damage_multiplier(float multiplier) {
    his_pdata->damage_multiplier *= multiplier;
    if (his_pdata->damage_multiplier < 0.01f) {
        his_pdata->damage_multiplier = 0.01f;
    }
}

int get_his_attack_counter(void) {
    if (aproc->pid == 0x1001) {
        return g_game_info.plyr1.slot.pdata->attack_counter;
    }
    return g_game_info.plyr0.slot.pdata->attack_counter;
}

int was_i_hit_x_times(unsigned int hits) {
    return (int)hits <= plyr_pdata->hit_count;
}

int was_button_pressed(int button) {
    if (plyr_pdata->drone_request != 0) {
        if (drone_ai_check_button_press() != 0) {
            advance_cur_cmd_idx();
            return 1;
        }
        return 0;
    }
    return check_button_and_pad(button, 0, 0);
}

int was_button_and_direction(int button, int direction) {
    if (plyr_pdata->drone_request != 0) {
        if (drone_ai_check_button_press() != 0 &&
            drone_ai_check_button_direction(direction) != 0) {
            advance_cur_cmd_idx();
            return 1;
        }
        return 0;
    }
    return check_button_and_pad(button, direction, 1) != 0;
}

int check_button_and_pad(
    unsigned int button, unsigned int direction, int pad) {
    EjbSwitchLogEntry* switch_log;
    int current_index;
    int next_index;
    int log_index;
    int drone_state;
    int player_state;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        current_index = p1_current_log_index;
        next_index = p1_current_log_index;
        if (p1_current_log_index != p1_log_index) {
            next_index = p1_current_log_index + 1;
            if (next_index >= 30) {
                next_index = 0;
            }
        }
        log_index = p1_log_index;
        switch_log = p1_switch_log;
        drone_state = get_konquest_drone_switch_state(0);
        player_state = g_game_info.plyr0.player_state;
    } else {
        current_index = p2_current_log_index;
        next_index = p2_current_log_index;
        if (p2_current_log_index != p2_log_index) {
            next_index = p2_current_log_index + 1;
            if (next_index >= 30) {
                next_index = 0;
            }
        }
        log_index = p2_log_index;
        switch_log = p2_switch_log;
        drone_state = get_konquest_drone_switch_state(1);
        player_state = g_game_info.plyr1.player_state;
    }

    if (player_state == 3) {
        if ((drone_state & (1U << button)) != 0) {
            plyr_pdata->round_attack_count++;
            return 1;
        }
        return 0;
    }
    if (current_index == log_index) {
        return 0;
    }
    if (button == switch_log[next_index].switch_id &&
        (pad == 0 || direction == switch_log[next_index].pad_state)) {
        advance_my_current_switch();
        plyr_pdata->round_attack_count++;
        return 1;
    }
    return 0;
}

void glitch_to_fstance(float blend_rate) {
    EjbFighterDefinitionExtended* fighter;

    plyr_obj->hide_flag_bits.bit6 ^= 1;
    plyr_anim_pdata->flags ^= 8;
    plyr_match_weapon_flip_to_obj_flip(plyr_pdata, plyr_obj);
    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->crouching_animation,
            0x20, blend_rate);
    } else {
        set_anim_script(
            plyr_anim_pdata, fighter->standing_animation, 0x20);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&plyr_obj->hdr));
    ground_me(as_mkhdr(&plyr_obj->hdr));
}

void glitch_to_stance(float blend_rate) {
    glitch_to_stance_impl(blend_rate);
}

int blend_to_fstance(float blend_rate) {
    EjbFighterDefinitionExtended* fighter;
    MkHdr* object_hdr;
    int crouching;

    plyr_obj->hide_flag_bits.bit6 ^= 1;
    plyr_anim_pdata->flags ^= 8;
    plyr_match_weapon_flip_to_obj_flip(plyr_pdata, plyr_obj);
    crouching = 0;
    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        crouching = 1;
        transition_to_anim_script(
            plyr_anim_pdata, fighter->crouching_animation,
            0x20, blend_rate);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->standing_animation,
            0x20, blend_rate);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    return crouching;
}

int blend_to_stance(float blend_rate) {
    return blend_to_stance_inline(blend_rate);
}

void ani_with_pos(void) {
    plyr_anim_pdata->flags &= ~0x40;
}

void ani_no_pos(void) {
    plyr_anim_pdata->flags |= 0x40;
}

static void disable_both_repel_flags(void) {
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.bit4 = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.bit4 = 0;
}

void disable_my_attacks(int ticks) {
    plyr_pdata->attacks_disabled_until =
        game_tick_ctr +
        (int)((float)ticks * inverse_game_speed + 0.5f);
}

void setup_for_flip_ani(void) {
    plyr_obj->hide_flag_bits.bit6 ^= 1;
    plyr_anim_pdata->flags ^= 8;
    plyr_match_weapon_flip_to_obj_flip(plyr_pdata, plyr_obj);
}

float p_chamber_to_stance_2(void) {
    EjbAnimPdataExtended* animation;

    animation = (EjbAnimPdataExtended*)plyr_anim_pdata;
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.chamber_to_stance_2,
        3, animation->transition_rate);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    ani_to_blend_frame(10.0f);
    blend_to_stance_inline(0.1f);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float p_chamber_to_stance(void) {
    EjbFighterDefinitionExtended* fighter;
    AnimPdata* animation;
    MkHdr* object_hdr;
    float target_frame;

    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.chamber_to_stance,
        3, 0.2f);
    ejb_sleep_ticks(1.0f);
    animation = plyr_anim_pdata;
    target_frame = 6.0f;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    if (plyr_anim_pdata->last_exec_tick ==
        (unsigned int)exec_tick_ctr) {
        ejb_sleep_ticks(1.0f);
    }
    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->crouching_animation,
            0x20, 0.1f);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->standing_animation,
            0x20, 0.1f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float p_blend_to_fstance_in_10(void) {
    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    ani_to_blend_frame(10.0f);
    blend_to_fstance(0.1f);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float p_blend_to_stance_in_10(void) {
    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    ani_to_blend_frame(10.0f);
    blend_to_stance_inline(0.1f);
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float j_exit_blend_stance(void) {
    EjbFighterDefinitionExtended* fighter;
    MkHdr* object_hdr;

    if (plyr_anim_pdata->last_exec_tick ==
        (unsigned int)exec_tick_ctr) {
        ejb_sleep_ticks(1.0f);
    }
    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100)) {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->crouching_animation,
            0x20, 0.1f);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata, fighter->standing_animation,
            0x20, 0.1f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09_bits.launched = 1;
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr =
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float j_exit(void) {
    EjbPlyrPdataExtended* player;

    player = (EjbPlyrPdataExtended*)plyr_pdata;
    if (plyr_anim_pdata->last_exec_tick == (unsigned int)exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (player->fatality_advance != 0 &&
        get_fatality_available_flag() != 0) {
        return ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(x_advance_fatality, 0.0f);
    }
    if ((int)mode_of_play == 6) {
        return ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(p_plyr_pz_fighter_entry, 0.0f);
    }
    if (plyr_pdata->drone_request != 0) {
        return ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(drone_entry, 0.0f);
    }
    return ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer(p_joy_entry, 0.0f);
}

void special_move_cam_him(
    int mode, int ticks, int flags, float x, float y, float z,
    float distance, float speed) {
    swap_active_plyr_proc();
    special_move_cam_setup(
        mode, ticks, flags, x, y, z, distance, speed);
    swap_active_plyr_proc();
}

/*
 * Soft ceiling: exact retail size and behavior. MWCC lowers only the final
 * fatality-finished test as arithmetic boolean normalization instead of a
 * branch; the clean inverse-guard variant compiles identically.
 */
int stay_down_check(void) {
    float life;

    if (g_game_info.pause_flag_bits.fatality_window) {
        if (aproc->pid == 0x1001) {
            life = g_game_info.plyr0.field_0C;
        } else {
            life = g_game_info.plyr1.field_0C;
        }
        if (life != 0.0f) {
            return 0;
        }
        if ((int)mode_of_play == 0xA &&
            mk_chess_should_i_fall_down() == 1) {
            return 1;
        }
        if (plyr_pdata->state == 0x4203) {
            return 1;
        }
        if (f_fatality_finished != 0) {
            return 1;
        }
        return 0;
    }

    if (aproc->pid == 0x1001) {
        life = g_game_info.plyr0.field_0C;
    } else {
        life = g_game_info.plyr1.field_0C;
    }
    if (life == 0.0f) {
        plyr_obj->pos_vel.z = 0.0f;
        plyr_obj->pos_vel.y = 0.0f;
        plyr_obj->pos_vel.x = 0.0f;
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling: retail m2c and callers confirm the full two-phase death wait,
 * boss animation loop, reaction transfers, and final stance/drone dispatch.
 * Clean structured C is 12 bytes larger: MWCC keeps one additional saved GPR
 * and emits the shared stay-down result with an extra li/branch pair. The
 * remaining body differences are register allocation and float relocations.
 */
float j_stay_down_dead(void) {
    float life;
    float frames;
    AnimPdata* animation;
    MkHdr* object_hdr;

    if ((int)mode_of_play == 8) {
        skip_end_of_trial_wrapup();
    }
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0x4200;
    life = aproc->pid == 0x1001
        ? g_game_info.plyr0.field_0C
        : g_game_info.plyr1.field_0C;
    if (life == 0.0f && (g_game_info.flags & 1) != 0) {
        plyr_obj->flags_09_bits.head_tracking = 0;
        plyr_obj->flags_09_bits.tightrope_restricted = 0;
        if (getup_should_stay_down() == 0) {
            end_of_round_check();
        }
        if (g_game_info.feature_flags.bits.high_bit) {
            go_into_major_pain_please = 0;
        }
        if (!is_big_boss(plyr_pdata)) {
            if (g_game_info.pause_flag_bits.fatality_window &&
                go_into_twitch_death_please == 1) {
                go_into_twitch_death_please = 0;
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(go_into_twitch_death, 0.0f);
                return 0.0f;
            }
            if (!g_game_info.pause_flag_bits.fatality_window &&
                go_into_major_pain_please == 1) {
                go_into_major_pain_please = 0;
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(go_into_major_pain, 0.0f);
                return 0.0f;
            }
        } else {
            if (plyr_anim_pdata->last_exec_tick ==
                (unsigned int)exec_tick_ctr) {
                ejb_sleep_ticks(1.0f);
            }
            if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
                (plyr_pdata->state & 0x100) != 0) {
                transition_to_anim_script(
                    plyr_anim_pdata,
                    ((EjbFighterDefinitionExtended*)
                        plyr_pdata->fighter_definition)->crouching_animation,
                    0x20, 0.05f);
            } else {
                transition_to_anim_script(
                    plyr_anim_pdata,
                    ((EjbFighterDefinitionExtended*)
                        plyr_pdata->fighter_definition)->standing_animation,
                    0x20, 0.05f);
            }
            xfer_proc(plyr_anim_proc, p_animate);
            plyr_obj->flags_09_bits.launched = 1;
            object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
            update_bone_hierarchy(object_hdr);
            object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
            ground_me(object_hdr);
            plyr_anim_pdata->step = 1.0f;

            for (;;) {
                life = aproc->pid == 0x1001
                    ? g_game_info.plyr0.field_0C
                    : g_game_info.plyr1.field_0C;
                if (life != 0.0f) {
                    break;
                }
                frames = 1.0f;
                animation = plyr_anim_pdata;
                while (frames > 0.0f) {
                    advance_anim(animation);
                    pose_anim(animation, 1);
                    ejb_sleep_ticks(1.0f);
                    frames -= 1.0f;
                }
            }
        }

        for (;;) {
            life = aproc->pid == 0x1001
                ? g_game_info.plyr0.field_0C
                : g_game_info.plyr1.field_0C;
            if (life != 0.0f) {
                break;
            }
            ejb_sleep_ticks(1.0f);
        }
    }
    plyr_obj->flags_09 |= 0x20;
    plyr_obj->flags_0B &= ~0x40;
    if (plyr_anim_pdata->last_exec_tick ==
        (unsigned int)exec_tick_ctr) {
        ejb_sleep_ticks(1.0f);
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        (plyr_pdata->state & 0x100) != 0) {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->crouching_animation,
            0x20, 0.1f);
    } else {
        transition_to_anim_script(
            plyr_anim_pdata,
            ((EjbFighterDefinitionExtended*)
                plyr_pdata->fighter_definition)->standing_animation,
            0x20, 0.1f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->flags_09 |= 0x80;
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object_hdr);
    object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object_hdr);
    plyr_anim_pdata->step = 1.0f;
    if (plyr_pdata->drone_request != 0) {
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(drone_start, 0.0f);
        return 0.0f;
    }
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)j_exit, 0.0f);
    return 0.0f;
}

void random_hit_n_voice(int hit_group, int voice_group) {
    if (hit_group != -1) {
        random_hit(hit_group);
    }
    if (voice_group != -1) {
        random_voice(voice_group);
    }
}

void shake_hit_voice(
    int shake_ticks, int hit_voice, int fighter_voice,
    float rumble_scale) {
    int rumble_strength;
    int controller;

    if (shake_ticks != 0) {
        shake_camera(shake_ticks, rumble_scale);
    }
    if (hit_voice != -1) {
        random_hit(hit_voice);
    }
    if (fighter_voice != -1) {
        random_voice(fighter_voice);
    }
    rumble_strength = (int)(400.0f * rumble_scale);
    if (rumble_strength > 10) {
        rumble_strength = 10;
    }
    if (aproc->pid == 0x1001) {
        controller = g_game_info.plyr0.controller_slot;
    } else if (aproc->pid == 0x1002) {
        controller = g_game_info.plyr1.controller_slot;
    } else {
        return;
    }
    ck_rumble_controller(
        controller, rumble_strength, shake_ticks * 15);
}

void electric_shaky_voice(void) {
    his_obj->pos_vel.y = 0.0f;
    his_obj->gravity = 0.0f;
}

void slip_voice(void) {
}

void ejb_call(int index) {
    EjbDispatchFn callback = ejb_function_pointers[index];

    if (callback != 0) {
        callback();
    }
}

void scorpion_voice_call(void) {
    if (plyr_pdata->character_id == 0) {
        snd_req(0x2D3);
    }
}

void fan_lift_prep(void) {
}

static void gut_tumble_air_check(void) {
    plyr_obj->gravity = -0.005f;
}

static void subzero_propell_collision(void) {
    int ticks;
    int collision_result;

    start_plyr_attack(0.0f);
    ticks = 0;
    for (;;) {
        if (plyr_pdata->collision_result == -1 &&
            local_collision_allowed(plyr_pdata) != 0) {
            plyr_pdata->attack_region = 7;
            if (his_pdata->collision_disabled != 0) {
                collision_result = 0;
            } else {
                set_plyr_attack_region(7, 1.0f, 0.0f);
                collision_result = collide_plyr_vs_plyr();
                if (collision_result == 1) {
                    trial_state_collision_check(
                        collision_result, aproc->pid == 0x1001);
                }
            }
            if (collision_result != 0) {
                plyr_pdata->collision_result = 1;
                reaction_xfer_him(8, 0.12f, 4);
                break;
            }
        }
        ejb_sleep_ticks(1.0f);
        ticks++;
        if (ticks >= 20) {
            break;
        }
    }
    if (plyr_pdata->collision_result == -1 ||
        plyr_pdata->collision_result == 3) {
        if (local_collision_allowed(plyr_pdata) != 0) {
            plyr_pdata->collision_result = 0;
        }
        trial_state_collision_check(
            0, plyr_pdata == g_game_info.plyr0.slot.pdata);
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
}

static void wait_for_backland(void) {
    MkObj* object;
    MkProc* process;

    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->gravity = -0.01f;
    plyr_obj->pos_vel.y = -0.1f;
    while ((object = plyr_obj)->flags_08_bits.moving) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }

    process = plyr_pdata->transient_proc;
    if (process != 0 &&
        process->instance !=
            (int)plyr_pdata->transient_proc_instance) {
        process = 0;
    }
    if (process != 0 && process != aproc && process->instance != 0) {
        ((EjbProcSleepVtable*)process->vtbl)->destroy();
    }
    object->pos_vel.x = 0.0f;
    object->pos_vel.y = 0.0f;
    object->pos_vel.z = 0.0f;
    object->gravity = 0.0f;
    init_ground_move();
}

static void impale_him(void) {
    EjbFighterDefinitionExtended* fighter;
    MkObj* source;
    MkObj* target;

    if (is_weapon_style(plyr_pdata->fighter_definition) == 0) {
        return;
    }

    fighter =
        (EjbFighterDefinitionExtended*)plyr_pdata->fighter_definition;
    source = fighter->impale_source;
    if (source != 0 &&
        source->hdr.instance != fighter->impale_source_instance) {
        source = 0;
    }
    target = fighter->impale_target;
    if (target != 0 &&
        target->hdr.instance != fighter->impale_target_instance) {
        target = 0;
    }
    player_impale(source, target);
}

void temp_vomit(void) {
    transition_to_anim_script(
        plyr_anim_pdata, his_pdata->reaction_animation, 0, 0.1f);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
}

void drift_downwards(void) {
    PlyrPdata* pdata;
    MkObj* object;
    MkProc* process;

    pdata = plyr_pdata;
    object = plyr_obj;
    process = pdata->transient_proc;
    if (process != 0) {
        if (process->instance != pdata->transient_proc_instance) {
            process = 0;
        }
    } else {
        process = 0;
    }

    if (process != 0 && process != aproc && process->instance != 0) {
        ((EjbProcSleepVtable*)process->vtbl)->destroy();
    }

    object->pos_vel.x = 0.0f;
    object->pos_vel.y = 0.0f;
    object->pos_vel.z = 0.0f;
    object->gravity = 0.0f;
    plyr_obj->gravity = -0.005f;
}

static void taunt_raise_my_life_bar(void) {
    EjbPlyrPdataExtended* player;

    player = (EjbPlyrPdataExtended*)plyr_pdata;
    trial_increment_state_value(plyr_pdata->plyr_num, 0x1B, 0);
    if (player->taunt_life_scale > 1.0f) {
        player->taunt_life_scale = 1.0f;
    }
    player->taunt_life_scale *= 0.8f;
    if (g_game_info.plyr0.field_0C != 0.0f &&
        g_game_info.plyr1.field_0C != 0.0f) {
        if (aproc->pid == 0x1001) {
            adjust_player_life(0);
        } else {
            adjust_player_life(1);
        }
    }
    if (plyr_pdata->player_slot == 0) {
        plyr_pdata->attack_disable_tick_p1 =
            game_tick_ctr +
            (int)(70.0f * inverse_game_speed + 0.5f);
    }
    if (plyr_pdata->player_slot == 1) {
        plyr_pdata->attack_disable_tick_p2 =
            game_tick_ctr +
            (int)(70.0f * inverse_game_speed + 0.5f);
    }
    shake_camera(2, 0.02f);
}

static void taunt_increase_life(float amount, float multiplier) {
    EjbPlyrPdataExtended* player;

    (void)amount;
    player = (EjbPlyrPdataExtended*)plyr_pdata;
    trial_increment_state_value(plyr_pdata->plyr_num, 0x1B, 0);
    if (player->taunt_life_scale > 1.0f) {
        player->taunt_life_scale = 1.0f;
    }
    player->taunt_life_scale *= multiplier;
    if (g_game_info.plyr0.field_0C != 0.0f &&
        g_game_info.plyr1.field_0C != 0.0f) {
        if (aproc->pid == 0x1001) {
            adjust_player_life(0);
        } else {
            adjust_player_life(1);
        }
    }
}

void auto_ani_off(void) {
    xfer_proc(plyr_anim_proc, p_anim_idle);
}

void auto_ani_on(void) {
    xfer_proc(plyr_anim_proc, p_animate);
}

void break_point(void) {
}

int player_area_collision_check(
    int reaction, int reaction_flags, float radius, float height,
    float reaction_scale) {
    int player;

    if (local_collision_allowed(plyr_pdata) == 0) {
        return 0;
    }
    if (collide_cylinder_vs_plyr(
            plyr_pdata->his_plyr_pdata->plyr_info,
            &plyr_obj->pos, &plyr_obj->ang, radius, height) != 0) {
        player = plyr_pdata == g_game_info.plyr0.slot.pdata;
        trial_state_collision_check(1, player);
        if (is_plyr_blocking(his_pdata) != 0) {
            plyr_pdata->collision_result = 2;
        } else {
            plyr_pdata->collision_result = 1;
        }
        reaction_xfer_him(reaction, reaction_scale, reaction_flags);
        return 1;
    }
    return 0;
}

void scorpion_summon_collide(void) {
    float delta_x;
    float delta_z;
    float distance_x;
    float distance_z;

    delta_x = his_obj->pos_x - plyr_pdata->summon_position_x;
    delta_z = his_obj->pos_z - plyr_pdata->summon_position_z;
    distance_x = delta_x * delta_x;
    distance_z = delta_z * delta_z;
    if ((his_pdata->state_flags.raw & 0x02) != 0x02) {
        if (local_collision_allowed_plyr_pdata() != 0 &&
            distance_x + distance_z < 0.25f) {
            trial_state_collision_check(1, his_pdata->plyr_num);
            reaction_xfer_him(0xB3, 0.13f, 2);
            return;
        }
        trial_state_collision_check(0, his_pdata->plyr_num);
    }
}

void scorpion_summon_read(void) {
    plyr_pdata->summon_position_x = his_obj->pos_x;
    plyr_pdata->summon_position_z = his_obj->pos_z;
}

/*
 * Soft ceiling: complete retail collision state machine. Source's eight-byte
 * excess is separate r30/r31 saves/restores; body residue is their permutation
 * and local relocation labels.
 */
static void tremor_collision_check(void) {
    PlyrPdata* opponent;
    MkObj* opponent_object;
    int collision_blocked;
    int state;

    if (local_collision_allowed_plyr_pdata() != 0) {
        opponent = plyr_pdata->his_plyr_pdata;
        opponent_object = plyr_pdata->his_obj;
        if (is_big_boss(opponent) != 0) {
            collision_blocked = 0;
        } else {
            state = opponent->state;
            if (state == 0x605) {
                collision_blocked = 1;
            } else if (state == 0x3202) {
                collision_blocked = 1;
            } else if ((unsigned int)state == 0xFFFFC602U) {
                collision_blocked = 1;
            } else if (state == 0x6001) {
                collision_blocked = 1;
            } else if (state == 0x60C) {
                collision_blocked = 0;
            } else if (opponent_object->pos_vel.y != 0.0f &&
                       opponent_object->gravity != 0.0f) {
                collision_blocked = 1;
            } else {
                collision_blocked = 0;
            }
        }
        if (collision_blocked == 0) {
            trial_state_collision_check(
                1, plyr_obj == g_game_info.plyr0.slot.mirror_a);
            reaction_xfer_him(0xB1, 0.13f, 2);
            return;
        }
    }
    trial_state_collision_check(
        0, plyr_obj == g_game_info.plyr0.slot.mirror_a);
}

void zero_my_hit_count(void) {
    plyr_pdata->hit_count = 0;
}

int disable_impale_check(void) {
    unsigned int previous_state;

    if (plyr_pdata->blocking_disabled_2 == 1) {
        return 1;
    }
    previous_state = plyr_pdata->previous_state;
    if (previous_state == (unsigned int)-0x39FE) {
        return 1;
    }
    if (previous_state == (unsigned int)-0x3A00) {
        return 1;
    }
    return previous_state == 0x4206U;
}

void set_my_float_1(float value) {
    plyr_pdata->summon_position_x = value;
}

void set_jump_towards_velocities(void) {
    Vec direction;

    uv_to_opponent(&direction);
    plyr_obj->pos_vel.x = 0.03f * direction.x;
    plyr_obj->pos_vel.z = 0.03f * direction.z;
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_obj->pos_vel.y = 0.11f;
    plyr_obj->gravity = -0.007f;
}

void weapon_trail_off(void) {
    plyr_weapon_trail_hide(plyr_pdata->mirror_slots);
}

void weapon_trail_on(void) {
    plyr_weapon_trail_show(plyr_pdata->mirror_slots);
}

void plyr_going_to_attack_with(const EjbActionRef* action_ref) {
    unsigned int action;

    action = action_ref->action;
    if (action == plyr_pdata->previous_action) {
        plyr_pdata->repeated_action_count++;
    } else {
        plyr_pdata->repeated_action_count = 0;
    }
    plyr_pdata->previous_action = action;
}

void plyr_going_to_attack_with_action(unsigned int action) {
    if (action == plyr_pdata->previous_action) {
        plyr_pdata->repeated_action_count++;
    } else {
        plyr_pdata->repeated_action_count = 0;
    }
    plyr_pdata->previous_action = action;
}

void player_postround_chores(void) {
    drone_ai_reset_all();
    g_game_info.plyr0.slot.pdata->postround_value = 0.0f;
    g_game_info.plyr1.slot.pdata->postround_value = 0.0f;
}

void player_initialize_chores(void) {
    plyr_pdata->breaker_strength = 3;
    if ((int)mode_of_play == 0xA) {
        mk_chess_set_breaker_value();
    }
    plyr_pdata->attack_counter = 0;
    plyr_pdata->shared_attack_until = 0;
    plyr_pdata->opponent_attack_counter = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
    plyr_pdata->previous_action = 0;
    plyr_pdata->repeated_action_count = 0;
    plyr_pdata->round_attack_count = 0;
    plyr_pdata->round_attack_stage = 0;
    plyr_pdata->combo_depth = 0;
    plyr_pdata->scream_sound_handle = 0;
    plyr_pdata->state_flags.raw &= ~0x10;
    init_ground_move();
    back_to_normal();
    plyr_obj->flags_09 |= 0x80;
    update_bone_hierarchy((MkHdr*)plyr_obj);
    ground_me((MkHdr*)plyr_obj);
    plyr_pdata->impaled_projectile_state = 0;
    plyr_pdata->taunts_performed = 0;
}

void avoid_double_ani(void) {
    if (plyr_anim_pdata->last_exec_tick == exec_tick_ctr) {
        _mkproc_sleep_ticks = 1.0f;
        ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

void match_my_ypos_with_his(void) {
    plyr_obj->pos_y = his_obj->pos_y;
}

float slamdown_reaction_max_hit_rules(void) {
    EjbPlyrPdataExtended* player;

    player = (EjbPlyrPdataExtended*)plyr_pdata;
    if (player->reaction_hit_count >= 2) {
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(r_jump_slambounce_final_hit, 0.0f);
    }
    return 0.0f;
}

float popup_reaction_max_hit_rules(void) {
    EjbPlyrPdataExtended* player;

    player = (EjbPlyrPdataExtended*)plyr_pdata;
    if (player->reaction_hit_count >= 2) {
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(r_jump_chin3_final_hit, 0.0f);
    }
    return 0.0f;
}

int is_fast_getup(void) {
    float life;
    float camera_z;
    float camera_x;
    float cross;
    int direction_pressed;

    if (aproc->pid == 0x1001) {
        life = g_game_info.plyr0.field_0C;
    } else {
        life = g_game_info.plyr1.field_0C;
    }
    if (life == 0.0f) {
        return 0;
    }
    if (plyr_pdata->drone_request != 0) {
        return drone_ai_should_roll(0) == 1;
    }

    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    cross =
        ((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z));
    if (cross < 0.0f) {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xF);
    } else {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xD);
    }
    if (direction_pressed != 0) {
        return 1;
    }
    return check_switch(plyr_pdata->switch_data, 0xC) != 0;
}

static inline float transfer_roll(MkProcEntryFn entry) {
    ((EjbProcSleepVtable*)aproc->vtbl)->transfer(entry, 0.0f);
    return 0.0f;
}

static inline void face_opponent_for_reverse_roll(void) {
    float angle;

    if (his_obj == 0 || plyr_obj == 0) {
        angle = 0.0f;
    } else {
        angle = gxMathArcTanYX(
            his_obj->pos.x - plyr_obj->pos.x,
            his_obj->pos.z - plyr_obj->pos.z);
    }
    plyr_obj->ang.y = angle;
    init_ground_move();
}

float front_rollup_check(void) {
    float life;
    float camera_z;
    float camera_x;
    int direction_pressed;

    if (getup_should_stay_down()) {
        return 0.0f;
    }
    life = aproc->pid == 0x1001
        ? g_game_info.plyr0.field_0C
        : g_game_info.plyr1.field_0C;
    if (life == 0.0f) {
        return 0.0f;
    }
    if (plyr_pdata->drone_request != 0) {
        unsigned short choice;

        if (drone_ai_should_roll(0) == 0) {
            return 0.0f;
        }
        choice = randu0(3);
        if (choice == 0) {
            return transfer_roll(front_rollup);
        }
        if (choice == 1) {
            return transfer_roll(j_front_roll_left);
        }
        if (choice == 2) {
            return transfer_roll(j_front_roll_right);
        }
    }
    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    if (((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z)) < 0.0f) {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xF);
    } else {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xD);
    }
    if (direction_pressed != 0) {
        return transfer_roll(front_rollup);
    }
    if (check_switch(plyr_pdata->switch_data, 0xC)) {
        return transfer_roll(j_front_roll_left);
    }
    if (check_switch(plyr_pdata->switch_data, 0xE)) {
        return transfer_roll(j_front_roll_right);
    }
    return 0.0f;
}

float back_rollup_check_reverse(void) {
    float life;
    float camera_z;
    float camera_x;
    int direction_pressed;

    if (getup_should_stay_down()) {
        return 0.0f;
    }
    life = aproc->pid == 0x1001
        ? g_game_info.plyr0.field_0C
        : g_game_info.plyr1.field_0C;
    if (life == 0.0f) {
        return 0.0f;
    }
    if (plyr_pdata->drone_request != 0) {
        unsigned short choice;

        if (drone_ai_should_roll(0) == 0) {
            return 0.0f;
        }
        choice = randu0(3);
        if (choice == 0) {
            face_opponent_for_reverse_roll();
            return transfer_roll(j_ass_rollup);
        }
        if (choice == 1) {
            return transfer_roll(j_back_rollup_IN);
        }
        if (choice == 2) {
            return transfer_roll(j_back_rollup_OUT);
        }
    }
    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    if (((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z)) < 0.0f) {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xF);
    } else {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xD);
    }
    if (direction_pressed != 0) {
        face_opponent_for_reverse_roll();
        return transfer_roll(j_ass_rollup);
    }
    if (check_switch(plyr_pdata->switch_data, 0xC) != 0) {
        return transfer_roll(j_back_rollup_IN);
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0) {
        return transfer_roll(j_back_rollup_OUT);
    }
    return 0.0f;
}

float back_rollup_check(void) {
    float life;
    float camera_z;
    float camera_x;
    int direction_pressed;

    if (getup_should_stay_down()) {
        return 0.0f;
    }
    life = aproc->pid == 0x1001
        ? g_game_info.plyr0.field_0C
        : g_game_info.plyr1.field_0C;
    if (life == 0.0f) {
        return 0.0f;
    }
    if (plyr_pdata->drone_request != 0) {
        unsigned short choice;

        if (drone_ai_should_roll(0) == 0) {
            return 0.0f;
        }
        choice = randu0(3);
        if (choice == 0) {
            return transfer_roll(j_ass_rollup);
        }
        if (choice == 1) {
            return transfer_roll(j_back_rollup_IN);
        }
        if (choice == 2) {
            return transfer_roll(j_back_rollup_OUT);
        }
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0) {
        if (check_switch(plyr_pdata->switch_data, 0xD) != 0 ||
            check_switch(plyr_pdata->switch_data, 0xF) != 0) {
            return transfer_roll((MkProcEntryFn)back_to_crouch);
        }
        return transfer_roll(j_back_rollup_OUT);
    }
    camera_z = camera_obj->pos_z;
    camera_x = camera_obj->pos_x;
    if (((plyr_obj->pos.x - camera_x) *
         -(his_obj->pos.z - camera_z)) -
        ((his_obj->pos.x - camera_x) *
         -(plyr_obj->pos.z - camera_z)) < 0.0f) {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xF);
    } else {
        direction_pressed = check_switch(plyr_pdata->switch_data, 0xD);
    }
    if (direction_pressed != 0) {
        return transfer_roll(j_ass_rollup);
    }
    if (check_switch(plyr_pdata->switch_data, 0xC) != 0) {
        return transfer_roll(j_back_rollup_IN);
    }
    return 0.0f;
}

float back_to_crouch(void) {
    AnimPdata* animation;
    float target_frame;

    force_away(10, 6, 0.05f, 0.8f);
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.back_getup_6,
        3, 0.1f);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    plyr_anim_pdata->step = 1.5f;
    target_frame = 12.0f;
    animation = plyr_anim_pdata;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    transition_to_anim_script(
        plyr_anim_pdata,
        ((EjbFighterDefinitionExtended*)
            plyr_pdata->fighter_definition)->crouching_animation,
        0, 0.1f);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    xfer_proc(plyr_anim_proc, p_animate);
    init_ground_move();
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer(joy_duck_loop, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: the retail-sized decision tree and every process-transfer
 * target are recovered. MWCC emits two additional return-tail instructions
 * from the clean structured branches; the remaining records are branch-target
 * alignment, bit-test spelling, register allocation, and float relocations.
 */
float end_of_round_check(void) {
    int winner;
    MkHdr* object_hdr;

    if ((g_game_info.flags & 1) == 0) {
        return 0.0f;
    }
    winner = 0;
    if (round_winner == 1 && aproc->pid == 0x1001) {
        winner = 1;
    }
    if (round_winner == 2 && aproc->pid == 0x1002) {
        winner = 1;
    }
    if ((int)mode_of_play != 8 ||
        trial_show_standard_fight_messages() != 0) {
        if (winner) {
            if (g_game_info.pause_flag_bits.fatality_window &&
                f_fatality_finished) {
                if (f_fatality_was_done != 0 &&
                    is_big_boss(his_pdata) == 0) {
                    end_round_cam_done = 1;
                    return 0.0f;
                }
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(victory, 0.0f);
                return 0.0f;
            }
            if (is_big_boss(plyr_pdata) &&
                !g_game_info.pause_flag_bits.fatality_window) {
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(big_boss_end_of_round, 0.0f);
                return 0.0f;
            }
            return 0.0f;
        }
        if (g_game_info.pause_flag_bits.fatality_window) {
            if (f_fatality_was_done || (g_game_info.flags & 4) != 0) {
                return 0.0f;
            }
            if (is_big_boss(plyr_pdata) != 0) {
                if (plyr_anim_pdata->last_exec_tick ==
                    (unsigned int)exec_tick_ctr) {
                    ejb_sleep_ticks(1.0f);
                }
                if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
                    (plyr_pdata->state & 0x100) != 0) {
                    transition_to_anim_script(
                        plyr_anim_pdata,
                        ((EjbFighterDefinitionExtended*)
                            plyr_pdata->fighter_definition)
                                ->crouching_animation,
                        0x20, 0.1f);
                } else {
                    transition_to_anim_script(
                        plyr_anim_pdata,
                        ((EjbFighterDefinitionExtended*)
                            plyr_pdata->fighter_definition)
                                ->standing_animation,
                        0x20, 0.1f);
                }
                xfer_proc(plyr_anim_proc, p_animate);
                plyr_obj->flags_09_bits.launched = 1;
                object_hdr = plyr_obj != 0
                    ? as_mkhdr(&plyr_obj->hdr) : 0;
                update_bone_hierarchy(object_hdr);
                object_hdr = plyr_obj != 0
                    ? as_mkhdr(&plyr_obj->hdr) : 0;
                ground_me(object_hdr);
                plyr_anim_pdata->step = 1.0f;
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer((MkProcEntryFn)j_exit, 0.0f);
                return 0.0f;
            }
            if (f_fatality_finished != 0) {
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(fall_dead, 0.0f);
                return 0.0f;
            }
            if ((int)mode_of_play == 0xA &&
                mk_chess_did_the_king_just_lose() == 0) {
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(fall_dead, 0.0f);
                return 0.0f;
            }
            if ((plyr_pdata->state_flags.raw & 0x10) != 0) {
                ((EjbProcSleepVtable*)aproc->vtbl)
                    ->transfer(fall_dead, 0.0f);
                return 0.0f;
            }
            ((EjbProcSleepVtable*)aproc->vtbl)
                ->transfer(dizzy, 0.0f);
            return 0.0f;
        }
        if ((g_game_info.flags & 4) != 0) {
            return 0.0f;
        }
        if (is_big_boss(plyr_pdata) != 0) {
            ((EjbProcSleepVtable*)aproc->vtbl)
                ->transfer(big_boss_end_of_round, 0.0f);
            return 0.0f;
        }
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer(fall_dead, 0.0f);
    } else if (plyr_pdata->state == 0x4200 ||
               (plyr_pdata->state_flags.raw & 0x10) != 0) {
        skip_end_of_trial_wrapup();
    } else {
        end_of_trial_wrapup(winner);
    }
    return 0.0f;
}

int plyr_get_f_constrained(PlyrPdata* pdata) {
    return pdata->f_constrained;
}

/*
 * Soft ceiling: all retail state, animation, combo, weapon, and tracking
 * resets are present in retail store order. Source is four bytes smaller;
 * objdiff isolates the residue to combo-message register allocation/conversion
 * scheduling, equivalent bitfield masks, and local float relocations.
 */
void back_to_normal(void) {
    MkHdr* object_hdr;

    if (plyr_pdata->drone_request == 1 &&
        (plyr_pdata->state & 0x100) != 0) {
        plyr_pdata->previous_state = plyr_pdata->state;
        plyr_pdata->state = 0;
        plyr_pdata->secondary_state = 0;
        if (plyr_anim_pdata->last_exec_tick ==
            (unsigned int)exec_tick_ctr) {
            ejb_sleep_ticks(1.0f);
        }
        if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
            (plyr_pdata->state & 0x100) != 0) {
            transition_to_anim_script(
                plyr_anim_pdata,
                ((EjbFighterDefinitionExtended*)
                    plyr_pdata->fighter_definition)->crouching_animation,
                0x20, 0.1f);
        } else {
            transition_to_anim_script(
                plyr_anim_pdata,
                ((EjbFighterDefinitionExtended*)
                    plyr_pdata->fighter_definition)->standing_animation,
                0x20, 0.1f);
        }
        xfer_proc(plyr_anim_proc, p_animate);
        plyr_obj->flags_09_bits.launched = 1;
        object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
        update_bone_hierarchy(object_hdr);
        object_hdr = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
        ground_me(object_hdr);
        plyr_anim_pdata->step = 1.0f;
    }
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    plyr_obj->flags_0B &= ~8;
    plyr_obj->hide_flag_bits.still_move = 1;
    plyr_obj->flags_09_bits.bit6 = 1;
    plyr_obj->flags_09_bits.bit4 = 1;
    plyr_obj->gravity = 0.0f;
    plyr_anim_pdata->weight_velocity = 0.0f;
    plyr_anim_pdata->weight =
        (int)mode_of_play == 6 ? 0.25f : 1.0f;
    plyr_anim_pdata->step = 1.0f;
    plyr_pdata->special_move_disabled = 0;
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_pdata->hit_flash_enabled = 0;
    plyr_pdata->f_constrained = 0;
    plyr_pdata->throw_restriction = 0;
    plyr_pdata->collision_result = -1;
    plyr_pdata->damage_multiplier = 1.0f;
    plyr_pdata->attack_region = 0x10;
    plyr_pdata->pending_reaction = 0xFFFF;
    plyr_pdata->weapon_impact = 0;
    plyr_pdata->summon_position_x = 0.0f;
    plyr_pdata->summon_position_z = 0.0f;
    plyr_pdata->summon_position_y = 0.0f;
    plyr_pdata->strafe_direction = 0;
    plyr_pdata->attack_type = 0;
    if (plyr_pdata->state_flags.bits.frozen) {
        unfreeze_player();
    }
    plyr_pdata->state_flags.raw &= ~4;
    wall_eligible_off();
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0;
    plyr_pdata->secondary_state = 0;
    trial_register_combo(
        plyr_pdata->plyr_num, plyr_pdata->combo_hit_count,
        plyr_pdata, plyr_pdata->combo_damage);
    check_for_combo_message_impl();
    ((EjbPlyrPdataExtended*)plyr_pdata)->combo_damage = 0.0f;
    plyr_pdata->damage_multiplier = 1.0f;
    ((EjbPlyrPdataExtended*)plyr_pdata)->hit_count = 0;
    ((EjbPlyrPdataExtended*)plyr_pdata)->reaction_hit_count = 0;
    ((EjbPlyrPdataExtended*)plyr_pdata)->combo_hit_count = 0;
    plyr_pdata->combo_flags = 0;
    plyr_pdata->script_exit_value_int = 0;
    plyr_pdata->script_exit_args[0] = 0;
    plyr_pdata->script_exit_args[1] = 0;
    plyr_pdata->collision_disabled = 0;
    plyr_pdata->push_blocked = 0;
    plyr_pdata->field_234 = 0;
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0;
    plyr_weapon_trail_hide(plyr_pdata->mirror_slots);
    if (is_blind(plyr_pdata) != 0) {
        plyr_obj->flags_09_bits.head_tracking = 0;
    } else if (((EjbFighterDefinitionExtended*)
                    plyr_pdata->fighter_definition)->character_id == 0x33) {
        plyr_obj->flags_09_bits.head_tracking = 0;
    } else {
        plyr_obj->flags_09_bits.head_tracking = 1;
    }
    plyr_pdata->mirror_slots =
        &plyr_pdata->fighter_definition->mirror_slots;
}

void disable_attack5(int ticks) {
    if (plyr_pdata->player_slot == 0) {
        plyr_pdata->attack_disable_tick_p1 =
            game_tick_ctr +
            (int)((float)ticks * inverse_game_speed + 0.5f);
    }
    if (plyr_pdata->player_slot == 1) {
        plyr_pdata->attack_disable_tick_p2 =
            game_tick_ctr +
            (int)((float)ticks * inverse_game_speed + 0.5f);
    }
}

int is_this_move_disabled_exec(unsigned int move) {
    if (is_special_move_available(plyr_pdata, move) == 0) {
        return 1;
    }
    if (plyr_pdata->disabled_moves[0].move == move &&
        plyr_pdata->disabled_moves[0].until_tick >
            (unsigned int)game_tick_ctr) {
        return 1;
    }
    if (plyr_pdata->disabled_moves[1].move == move &&
        plyr_pdata->disabled_moves[1].until_tick >
            (unsigned int)game_tick_ctr) {
        return 1;
    }
    if (plyr_pdata->disabled_moves[2].move == move &&
        plyr_pdata->disabled_moves[2].until_tick >
            (unsigned int)game_tick_ctr) {
        return 1;
    }
    if (plyr_pdata->disabled_moves[3].move == move &&
        plyr_pdata->disabled_moves[3].until_tick >
            (unsigned int)game_tick_ctr) {
        return 1;
    }
    return 0;
}

void enable_this_move_exec(unsigned int move) {
    if (plyr_pdata->disabled_moves[0].move == move) {
        plyr_pdata->disabled_moves[0].until_tick = 0;
        plyr_pdata->disabled_moves[0].move = 0;
        return;
    }
    if (plyr_pdata->disabled_moves[1].move == move) {
        plyr_pdata->disabled_moves[1].until_tick = 0;
        plyr_pdata->disabled_moves[1].move = 0;
        return;
    }
    if (plyr_pdata->disabled_moves[2].move == move) {
        plyr_pdata->disabled_moves[2].until_tick = 0;
        plyr_pdata->disabled_moves[2].move = 0;
        return;
    }
    if (plyr_pdata->disabled_moves[3].move == move) {
        plyr_pdata->disabled_moves[3].until_tick = 0;
        plyr_pdata->disabled_moves[3].move = 0;
    }
}

void disable_this_move_exec(unsigned int move, int ticks) {
    if (plyr_pdata->disabled_moves[0].move == move) {
        plyr_pdata->disabled_moves[0].until_tick =
            game_tick_ctr +
            (int)((float)ticks * inverse_game_speed + 0.5f);
        return;
    }
    plyr_pdata->disabled_moves[3].until_tick =
        plyr_pdata->disabled_moves[2].until_tick;
    plyr_pdata->disabled_moves[3].move =
        plyr_pdata->disabled_moves[2].move;
    plyr_pdata->disabled_moves[2].until_tick =
        plyr_pdata->disabled_moves[1].until_tick;
    plyr_pdata->disabled_moves[2].move =
        plyr_pdata->disabled_moves[1].move;
    plyr_pdata->disabled_moves[1].until_tick =
        plyr_pdata->disabled_moves[0].until_tick;
    plyr_pdata->disabled_moves[1].move =
        plyr_pdata->disabled_moves[0].move;
    plyr_pdata->disabled_moves[0].until_tick =
        game_tick_ctr +
        (int)((float)ticks * inverse_game_speed + 0.5f);
    plyr_pdata->disabled_moves[0].move = move;
}

void head_tracking_off(void) {
    plyr_obj->flags_09_bits.head_tracking = 0;
}

void head_tracking_on(void) {
    EjbFighterDefinitionView* fighter;

    if (is_blind(plyr_pdata) != 0) {
        plyr_obj->flags_09_bits.head_tracking = 0;
        return;
    }

    fighter =
        (EjbFighterDefinitionView*)plyr_pdata->fighter_definition;
    if (fighter->character_id == 0x33) {
        plyr_obj->flags_09_bits.head_tracking = 0;
        return;
    }
    plyr_obj->flags_09_bits.head_tracking = 1;
}

void gut_bleed_me(int size) {
    switch (size) {
    case 1:
        plyr_bleed_small_cycle_ext(plyr_pdata, 9, plyr_pdata);
        return;
    case 2:
        plyr_bleed_medium_cycle(plyr_pdata, 9);
        return;
    case 3:
        plyr_bleed_large_ext(plyr_pdata, 9, plyr_pdata);
        return;
    }
}

void face_bleed_me(int size) {
    switch (size) {
    case 1:
        plyr_bleed_small_cycle_ext(plyr_pdata, 0x10, plyr_pdata);
        return;
    case 2:
        plyr_bleed_medium_cycle(plyr_pdata, 0x10);
        return;
    case 3:
        plyr_bleed_large_ext(plyr_pdata, 0x10, plyr_pdata);
        return;
    }
}

void player_feet_land_chores(void) {
    dead_liukang_snd_chain_check(plyr_pdata, 0, 4, 0x32);
    random_foot(1);
    check_bgnd_effect();
    random_voice(0xA);
    if (small_ground_fx != 0) {
        small_ground_fx();
    }
}

float step_throw_outof_retract(void) {
    AnimPdata* animation;
    float target_frame;

    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0xD201;
    plyr_anim_pdata->step = 0.8f;
    ani_to_end();
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.step_throw_out,
        3, 0.1f);
    ejb_sleep_ticks(1.0f);
    plyr_anim_pdata->step = 0.5f;
    animation = plyr_anim_pdata;
    target_frame = 10.0f;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    ((EjbProcSleepVtable*)aproc->vtbl)
        ->transfer((MkProcEntryFn)j_exit_blend_stance, 0.0f);
    return 0.0f;
}

float step_throw_into_check(void) {
    AnimPdata* animation;
    float target_frame;

    init_ground_move();
    plyr_pdata->blocking_disabled = 1;
    random_voice(9);
    random_hit(0xE);
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0x120C;
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.step_throw_into,
        3, 0.1f);
    _mkproc_sleep_ticks = 1.0f;
    ((EjbProcSleepVtable*)aproc->vtbl)->sleep();
    plyr_anim_pdata->step = 1.6f;
    target_frame = 8.0f;
    animation = plyr_anim_pdata;
    if (target_frame > animation->high_frame) {
        target_frame = animation->high_frame;
    }
    EJB_ADVANCE_TO_FRAME(animation, target_frame);
    ani_to_frame_x_col(9, 0xAD, 6, 10.0f, 1.0f, 0.0f, 0.0f);
    if (plyr_pdata->collision_result == 0) {
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer((MkProcEntryFn)step_throw_outof_retract, 0.0f);
        return 0.0f;
    }
    if (plyr_pdata->collision_result == 2 &&
        is_he_blocking_throw() != 0) {
        ((EjbProcSleepVtable*)aproc->vtbl)
            ->transfer((MkProcEntryFn)step_throw_outof_retract, 0.0f);
        return 0.0f;
    }
    plyr_obj->flags_09_bits.face_opponent = 1;
    his_obj->flags_09_bits.face_opponent = 1;
    return 0.0f;
}

int get_his_secondary_state(void) {
    return his_pdata->secondary_state;
}

int get_his_previous_state(void) {
    return his_pdata->previous_state;
}

static void start_impale_bleeding(void) {
    plyr_pdata->postround_value = -0.00041666668f;
    if ((int)mode_of_play == 7) {
        _create_mkproc_generic_nostack(
            0x900F, 0x1F, p_konquest_register_bleeding, 0, 0);
    }
}

void scale_me_normal(void) {
    EjbPlyrScaleView* player;
    EjbScalePdata* scale_pdata;

    player = (EjbPlyrScaleView*)plyr_pdata;
    scale_pdata = (EjbScalePdata*)player->scale_pdata;
    if (scale_pdata != 0 &&
        scale_pdata->instance != player->scale_pdata_instance) {
        scale_pdata = 0;
    }
    if (scale_pdata == 0) {
        return;
    }

    scale_pdata->scale = plyr_obj->scale;
    scale_pdata->script = scale_script_normal;
    scale_pdata->current_script = scale_script_normal;
    scale_pdata->script_frame = 0.0f;
}

void set_anim_hiframe(float frame) {
    anim_set_hiframe(plyr_anim_pdata, frame);
}

void plyr_rotate_obj_y180(void) {
    plyr_anim_pdata->flags |= 0x1000;
    plyr_obj->ang.y += 3.1415927f;
}

void init_debug_variables(void) {
    debug_x = 1.3f;
    debug_y = 1.1f;
    debug_z = 0.008f;
    debug_int_1 = 30;
    debug_int_2 = 30;
}















































































































































































































static int visual_flip_state(void) {
    int flipped = (plyr_obj->hide_flags & 0x40) != 0;

    if ((plyr_anim_pdata->flags & 8) != 0) {
        flipped ^= 1;
    }
    return flipped;
}
