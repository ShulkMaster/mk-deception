#include "game/pwrbar.h"
#include "game/pfxscript_api.h"
#include "game/ncs.h"
#include "game/ground_fx.h"
#include "runtime/mk_obj.h"
#include "runtime/cam.h"
#include "game/ejb.h"
#include "game/constrain.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "game/bgnd.h"
#include "game/game_info.h"
#include "math/gxMath.h"
#include "platform/main.h"
#include "platform/io.h"
#include "game/plyr_globals.h"
#include "runtime/anim_api.h"
#include "runtime/sound.h"
#include "runtime/mk_pdata.h"
#include "rw/rwcore_types.h"
#include "runtime/plyr_anim_pdata.h"
#include "game/pfxscript.h"
#include "game/trial.h"
#include "game/moves.h"
#include "game/plyr.h"
#include "platform/joy.h"

extern PlyrPdata* his_pdata;
extern MkProc* plyr_anim_proc;

typedef float (*ReactionEntry)(void);

struct ReactionDispatchPair {
    int call_type;
    ReactionEntry entry;
};

typedef struct ReactionXferAddress {
    struct ReactionDispatchPair dispatch;
    int power_level;
    int state;
    int flags;
} ReactionXferAddress;

struct ReactionSharedAnimations {
    AniData* jax_piston_high;
    AniData* chest_stumble;
    AniData* pad008[21];
    AniData* jax_piston_low;
    AniData* pad060[11];
    AniData* gut_on_butt;
    AniData* pad090[1];
    AniData* gut_on_feet;
    AniData* pad098[13];
    AniData* enough_air;
    AniData* pad0D0[11];
    AniData* cyrus_stomp;
    AniData* pad100[12];
    AniData* feet_hit;
    AniData* pad134[2];
    AniData* swept_in;
    AniData* swept_reverse;
    AniData* swept_out;
    AniData* pad148[3];
    AniData* falling_back;
    AniData* side_head_spin;
    AniData* pad15C[3];
    AniData* side_head_dive;
    AniData* airborn_small_lift;
    AniData* pad170[2];
    AniData* top_of_head_slam;
    AniData* head_slam_fall;
    AniData* pad180[9];
    AniData* wall_hit;
    AniData* pad1A8[3];
    AniData* jump_chin;
    AniData* jump_slambounce;
    AniData* pad1BC[2];
    AniData* ermac_slam;
    AniData* pad1C8[2];
    AniData* cyrax_blade;
    AniData* combo_broken_launch;
    AniData* pad1D8[4];
    AniData* combo_broken_recover;
    AniData* pad1EC[11];
    AniData* post_surf_getup;
    AniData* pad21C[3];
    AniData* throw_getup;
    AniData* pad22C[24];
    AniData* throw_fall;
    AniData* pad290[7];
    AniData* standing_block_a;
    AniData* pad2B0[2];
    AniData* standing_block_b;
    AniData* pad2BC[3];
    AniData* standing_block_c;
    AniData* pad2CC[2];
    AniData* standing_block_d;
    AniData* pad2D8[3];
    AniData* duck_block;
    AniData* standing_weapon_block;
    AniData* pad2EC[1];
    AniData* counter_caught;
    AniData* pad2F4[7];
    AniData* counter_caught_6;
    AniData* counter_caught_7;
    AniData* counter_caught_8;
    AniData* counter_caught_9;
    AniData* combo_breaker;
};
typedef char ReactionSharedAnimationsSizeCheck[sizeof(struct ReactionSharedAnimations) == (804 / 4) * sizeof(AniData*) ? 1 : -1];

struct ReactionImageFaderPdata {
    MkHdr hdr;
    ScreenObj* object;
    unsigned int object_instance;
    int direction;
    int alpha;
    int delay;
};

struct ReactionTransferPdata {
    MkHdr hdr;
    MkProc* opponent_proc;
    unsigned int opponent_proc_instance;
    PlyrPdata* opponent_pdata;
    MkObj* opponent_obj;
};

static ReactionXferAddress g_loadable_reaction_scripts[8] = {
    {{5, 0}, 0, 0, 0}, {{5, 0}, 0, 0, 0},
    {{5, 0}, 0, 0, 0}, {{5, 0}, 0, 0, 0},
    {{5, 0}, 0, 0, 0}, {{5, 0}, 0, 0, 0},
    {{5, 0}, 0, 0, 0}, {{5, 0}, 0, 0, 0},
};

void* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z);
void low_flash_check(void);
void face_opponent_now(void);
void random_voice(int group);
void ani_to_end(void);
float j_blend_to_stance_in_x(void);
int reaction_xfer_him(int reaction, float rate, int strength);
void stop_me(void);
void blocked_fx(int type, int bone, int third, int fourth, int fifth);
void force_away(float speed, int direction, float damping, int ticks);
void disable_my_attacks(int ticks);
void adjust_my_damage_multiplier(float multiplier);
void got_hit_fx(int first, int second, int third, int fourth, int fifth, float value, int sixth);
void random_hit(int group);
static float j_block_common_reaction(void);
float j_block_loop(void);
static float chest_stumble_both(void);
float j_blend_to_fstance_in_x(void);
float j_getup_back_12(void);
int blend_to_fstance(float rate);
void freeze_player(void);
void unfreeze_player(void);
void glitch_to_ani(AniData* animation, int transition);
int should_weapon_block(PlyrPdata* player);

extern struct ReactionSharedAnimations shared_ani;

void ani_to_frame_x(float frame);
void ani_to_blend_frame(float frame);
void ani_to_frame_x_call(void (*callback)(void), float frame);
void add_facial_damage(float amount);
void check_for_combo_message(void);
void disable_both_repel_flags(void);
void init_air_move(void);
float p_blend_to_stance_in_10(void);
float p_sh_throw_plyr_in_grinder(void);
float r_beetle_lair_transition(void);
int big_boss_reaction_remap();
float drone_ai_get_big_boss_damage_scale();
void become_plyr1_proc();
void become_plyr2_proc();
void snd_stop();
void scale_me_normal(void);
void xfer_player_proc_to_script();
void init_ground_move_no_aniproc();
void init_3d_move_no_aniproc();
int check_damage_valid_fc();
float trial_damage_callback();
int drone_ai_check_block_at_reactions(void);
void drone_ai_hit();
void drone_ai_reset_ai_cmd();
int drone_ai_check_combo_breaker();
void enable_bgnd_obj_repel();
void exit_plyr_proc();
int my_joypad_state_5(void);
void stop_prison_grab_proc(void);
float p_glitch_to_stance(void);
float r_call_script_function(void);
float r_call_player_char_script_function(void);
static float r_call_other_player_char_script_function(void);
void run_reaction_cleanup_function(PlyrPdata* player);

extern int f_fatality_was_done;
extern int g_drone_blocking_in_reaction;
extern int g_drone_faked_out;
static float r_complete_ermac_slam(void);
static float r_face3_onback(void);
void set_ani_speed(float speed);
void set_anim_hiframe(float frame);
void wall_eligible_on(void);
void wall_eligible_off(void);
void blend_to_ani_frame(
    AniData* animation, int transition, float blend, float frame);
float j_getup_back_6(void);
float j_getup_front_12(void);
float blend_to_stance_j_exit(void);
float j_getup_back_3(void);
float j_getup_back_9(void);
float j_getup_back_12(void);
float j_getup_sit_12(void);
float j_stay_down_dead(void);
void ani_to_fall_to_frame(
    float landing_frame, int sound_id, float target_frame);
void back_rollup_check(void);
void danger_zone_eligible_on(void);
void face_bleed_me(int size);
void init_air_move_no_aniproc(void);
void land_chores(
    int land_sound, int second_sound,
    float shake_ticks, float shake_strength);
void launch_n_land_ani(
    void* animation, float launch_frame, float launch_step,
    float landing_frame, int landing_animation, float velocity_y,
    float gravity, float blend);
void myvel_his_angle_y(float y, float x, float z);
void newani_to_frame_x(void* animation, float frame, float x, float z, float blend, int flags);
void player_feet_land_chores(void);
void shake_camera(int ticks, float strength);
void shake_hit_voice(int shake_ticks, float rumble_scale, int hit_voice, int fighter_voice);
void start_blood_particles(
    int script, int bone, PlyrPdata* player, MkObj* object);
int stay_down_check(void);
void tightrope_restrictions_off(void);
void tightrope_restrictions_on(void);
void ani_1_frame(void);
void ani_loop_more_frames(float frames);
void ani_x_more_frames(float frames);
void blend_to_ani_INOUT(
    AniData* in_animation, AniData* out_animation, float blend_rate,
    float in_speed, float out_speed);
void disable_blocking(void);
void enable_all_my_blocking(void);
int am_i_duck_blocking(void);
int get_his_attack_counter(void);
int am_i_airborn_check_in_reaction(void);
void destroy_subzero_decoy(void);
void ejb_call(int command);
float fpick_a_float(float normal, float flipped_value);
int is_he_airborn(void);
void launch_me_up(float vertical_velocity, float gravity);
void myvel_my_angle_y(float angle, float x_velocity, float z_velocity);
void bulvan_function(int enabled);
void myvel_his_angle_y_inout(float y, float x, float z);
int my_joypad_state_5(void);

void wait_to_land(void);
float wall_dodge(void);
static float r_counter_caught_abort(void);
static void same_xz(void);
static void r_top_of_head_slam(void);
void fx_resume_emit(unsigned int effect);
static float p_image_fader(void);

#include "src/game/reactions_table_prototypes.inc"
#include "src/game/reactions_table.inc"

static inline MkObj* plyr_live_tracked_obj(PlyrPdata* player) {
    MkObj* object = player->tracked_obj;
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

/* TODO: [near miss] 99.29578%; live object/opponent owner swap r4/r5;
 * recover another latch lifetime after neutral helper/type probes. */
void run_reaction_cleanup_function(PlyrPdata* player) {
    if (player != 0 && player->runtime_data->reaction_cleanup != 0) {
        PlyrPdata* saved_player;
        PlyrPdata* saved_opponent;
        MkObj* saved_object;
        MkObj* saved_opponent_object;
        MkObj* object;
        MkObj* opponent_object;
        CmdScript* saved_script;

        saved_player = plyr_pdata;
        saved_opponent = his_pdata;
        plyr_pdata = player;
        saved_object = plyr_obj;
        saved_opponent_object = his_obj;
        his_pdata = player->his_plyr_pdata;
        object = plyr_live_tracked_obj(player);
        plyr_obj = object;
        opponent_object = plyr_live_tracked_obj(player->his_plyr_pdata);
        his_obj = opponent_object;
        if (object == 0 || opponent_object == 0) {
            return;
        }
        saved_script = active_cmdscript;
        active_cmdscript = &global_script_interpreter;
        cmdscript_set_parameters(&global_script_interpreter, 1, player);
        cmdscript_setup_execution(
            player->cmo, player->runtime_data->reaction_cleanup);
        cmdscript_execute(player->cmo);
        active_cmdscript = saved_script;
        plyr_pdata = saved_player;
        his_pdata = saved_opponent;
        plyr_obj = saved_object;
        his_obj = saved_opponent_object;
    }
}

static float p_image_fader(void) {
    ScreenObj* object;
    struct ReactionImageFaderPdata* pdata;

    pdata = (struct ReactionImageFaderPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->delay > 0) {
        object = pdata->object;
        object = MK_LIVE(object, pdata->object_instance);
        if (object != 0) {
            if (pdata->direction == 0) {
                object->x--;
                object->y++;
            } else {
                object->x++;
                object->y++;
            }
        }
        pdata->delay--;
        return 2.0f;
    }

    if (pdata->alpha > 0) {
        pdata->alpha -= 8;
        object = pdata->object;
        object = MK_LIVE(object, pdata->object_instance);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, pdata->alpha);
            if (pdata->direction == 0) {
                object->x--;
                object->y++;
            } else {
                object->x++;
                object->y++;
            }
        }
        if (pdata->alpha - 8 < 0) {
            pdata->alpha = 0;
        }
        return 2.0f;
    }

    object = pdata->object;
    object = MK_LIVE(object, pdata->object_instance);
    if (object != 0 && object->instance != 0) {
        object->typed_vtbl->destroy(object);
    }
    return -1.0f;
}

ScreenObj* display_image_by_plyr(
    int slot, const char* image_name, PlyrInfo* source, int unused,
    float y_offset) {
    struct ReactionImageFaderPdata* fader;
    ScreenObj* image;
    int half_width;
    Vec bone_offset = {0.0f, 0.0f, 0.0f};
    Vec world_position;
    RwV2d screen_position;

    image = load_named_2d_pfxobj(
        slot, 0xC021, image_name, 0, 0x2F);
    get_bone_offset_world_pos(
        source->slot.mirror_a, 9, &bone_offset, &world_position);
    world_position.y = y_offset + g_game_info.field_34;
    camera_get_screen_pos_from_world_pos(
        &world_position, &screen_position);
    half_width = image->pfx2d->tex_w / 2;
    image->x = (int)screen_position.x - half_width;
    image->y = screen_position.y;

    if (_create_mkproc_generic_nostack(
            0xC02A, 0x1F, p_image_fader,
            sizeof(struct ReactionImageFaderPdata),
            (MkHdr**)&fader) != 0) {
        fader->object = image;
        fader->object_instance = image->instance;
        fader->delay = 60;
        fader->alpha = 0xFF;
        fader->direction = source->field_04;
    }
    return image;
}

static inline void reaction_flash_at_height(MkObj* object, float y_offset) {
    unsigned int effect;
    Vec position;

    if (plyr_pdata->plyr_num == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    get_bone_world_pos(object, 0, &position);
    position.y = y_offset + g_game_info.field_34;
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void flash_hit_at_bid_with_y(float y_offset) {
    reaction_flash_at_height(plyr_obj, y_offset);
}

static inline void flash_object_hit_at_bid(MkObj* object, int bone) {
    unsigned int effect;
    Vec position;
    if (plyr_pdata->plyr_num == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    get_bone_world_pos(object, bone, &position);
    mk_chess_launch_fx_at_pos_with_obj_emit_based(effect, position.x, position.y, position.z);
}

void flash_hit_at_bid(int bone) {
    flash_object_hit_at_bid(plyr_obj, bone);
}

/* TODO: [near miss] 99.57%; object/effect nonvolatile pair swapped (r30/r31); stop at coloring. */
void low_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1F, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 0.5f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

/* TODO: [near miss] 99.57%; object/effect nonvolatile pair swapped (r30/r31); stop at coloring. */
void medium_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1E, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 1.15f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

/* TODO: [near miss] 99.57%; object/effect nonvolatile pair swapped (r30/r31); stop at coloring. */
void high_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1D, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 1.7f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

void fight_fx_im_hit_with_breaker_flash(
    int player,
    MkObj* object,
    int bone,
    int use_bone,
    void* script_args,
    float y_offset) {
    unsigned int effect;
    Vec position;

    if (player == 0) {
        effect = fx_by_owner("breaker_hit_fx", 1);
    } else {
        effect = fx_by_owner("breaker_hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void fight_fx_im_hit_flash(
    int player,
    MkObj* object,
    int bone,
    int use_bone,
    void* script_args,
    float y_offset) {
    unsigned int effect;
    Vec position;

    if (player == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void general_flash_fx(
    int player,
    MkObj* object,
    const char* effect_name,
    int bone,
    int use_bone,
    float y_offset) {
    unsigned int effect;
    Vec position;

    if (player == 0) {
        effect = fx_by_owner(effect_name, 1);
    } else {
        effect = fx_by_owner(effect_name, 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

static inline void start_blade_clash_fx(
    PlyrPdata* player, unsigned int effect, MkObj* blade, int bone) {
    MkPfx* particle;
    WeaponDefinition* weapon;

    effect = fx_next_emitter(effect);
    weapon = player->fighter_definition->definition->primary_weapon;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, weapon->clash_fx_offset.x,
            weapon->clash_fx_offset.y, weapon->clash_fx_offset.z);
        fx_resume_emit(effect);
    }
}

/* TODO: [near miss] 99.58%; player/blade homes (r30/r31) and the third
 * clash-fx expansion's effect/weapon homes swap; stop at coloring. */
void fight_fx_blades_clash(PlyrPdata* player) {
    MkObj* blade;
    unsigned int effect;
    int bone;
    int player_num;

    player_num = player->plyr_num;
    bone = 0;
    blade = MK_HDR_LIVE(player->fighter_definition->primary_weapon, player->fighter_definition->primary_weapon_instance);
    if (blade == 0 || blade->hide_flag_bits.hidden == 1) {
        bone = 0x1C;
        blade = player->plyr_info->slot.mirror_a;
    }
    if (player_num == 0) {
        effect = fx_by_owner("blade_flash", 1);
    } else {
        effect = fx_by_owner("blade_flash", 2);
    }
    start_blade_clash_fx(player, effect, blade, bone);
    if (player_num == 0) {
        effect = fx_by_owner("blade_sparks", 1);
    } else {
        effect = fx_by_owner("blade_sparks", 2);
    }
    start_blade_clash_fx(player, effect, blade, bone);
    if (player_num == 0) {
        effect = fx_by_owner("blade_bouncy_sparks", 1);
    } else {
        effect = fx_by_owner("blade_bouncy_sparks", 2);
    }
    start_blade_clash_fx(player, effect, blade, bone);
}

static inline void reaction_cleanup_active_player(void) {
    CmdScript* saved_cmdscript;
    PlyrPdata* saved_player;
    PlyrPdata* saved_opponent;
    MkObj* saved_object;
    MkObj* saved_opponent_object;
    MkObj* cleanup_object;
    MkObj* cleanup_opponent_object;

    if (plyr_pdata != 0 &&
        plyr_pdata->runtime_data->reaction_cleanup != 0) {
        saved_player = plyr_pdata;
        saved_opponent = his_pdata;
        saved_object = plyr_obj;
        saved_opponent_object = his_obj;
        plyr_pdata = saved_player;
        his_pdata = saved_player->his_plyr_pdata;
        cleanup_object = MK_HDR_LIVE(saved_player->tracked_obj, saved_player->tracked_obj_instance);

        plyr_obj = cleanup_object;
        cleanup_opponent_object =
            MK_HDR_LIVE(saved_player->his_plyr_pdata->tracked_obj, saved_player->his_plyr_pdata->tracked_obj_instance);
        his_obj = cleanup_opponent_object;
        if (cleanup_object != 0 && cleanup_opponent_object != 0) {
            saved_cmdscript = active_cmdscript;
            active_cmdscript = &global_script_interpreter;
            cmdscript_set_parameters(
                &global_script_interpreter, 1, saved_player);
            cmdscript_setup_execution(
                saved_player->cmo,
                saved_player->runtime_data->reaction_cleanup);
            cmdscript_execute(saved_player->cmo);
            active_cmdscript = saved_cmdscript;
            plyr_pdata = saved_player;
            his_pdata = saved_opponent;
            plyr_obj = saved_object;
            his_obj = saved_opponent_object;
        }
    }
}

/* TODO: [breakthrough] 99.65767%; reaction lifetimes/cleanup owner fixed;
 * state/previous-state spill exchange, call constants and coloring remain. */
int reaction_xfer_him(int reaction, float damage_scale, int block_type) {
    struct ReactionTransferPdata* transfer;
    PlyrPdata* boost_source;
    PlyrFighterDefinition* fighter;
    PlyrFightingLightState* lights;
    struct ReactionDispatchPair dispatch_pair;
    int face_after;
    CmdScript* cmdscript;
    MkProc* opponent_proc;
    MkProc* hold_proc;
    PlyrPdata* victim;
    MkObj* victim_obj;
    int original_previous_state;
    int state_for_bgnd;
    int saved_state;
    int selected_reaction;
    int blocked;
    int force_air;
    int face_reaction;
    int reaction_state;
    int big_boss;
    int both_special;
    int input_state;
    float applied_damage;
    float damage;

    reaction_state = tbl_xfer_addresses[reaction].state;
    face_after = 1;
    face_reaction = 0;
    force_air = 0;
    if (g_game_info.flag_bits.level_fatality_active ||
        g_game_info.flag_bits.level_transition_active ||
        f_fatality_was_done != 0) {
        return 0;
    }

    transfer = (struct ReactionTransferPdata*)apdata;
    opponent_proc = MK_HDR_LIVE(transfer->opponent_proc, transfer->opponent_proc_instance);

    victim_obj = transfer->opponent_obj;
    victim = transfer->opponent_pdata;
    cmdscript = get_cmdscript_for_proc(opponent_proc);

    big_boss = is_big_boss(victim);
    if (big_boss != 0) {
        reaction = big_boss_reaction_remap(reaction);
        if (victim_obj == g_game_info.plyr0.slot.mirror_a &&
            g_game_info.plyr1.slot.pdata->secondary_state & 0x100) {
            if (damage_scale > 0.06f) {
                damage_scale *= 0.15f;
            }
        } else if (victim_obj == g_game_info.plyr1.slot.mirror_a &&
                   g_game_info.plyr0.slot.pdata->secondary_state & 0x100) {
            if (damage_scale > 0.06f) {
                damage_scale *= 0.15f;
            }
        } else if (victim->drone_request != 0) {
            damage_scale *= drone_ai_get_big_boss_damage_scale(victim);
        } else {
            damage_scale = 0.9f * damage_scale;
        }
    }
    selected_reaction = reaction;

    victim->hit_flash_enabled = 0;
    victim->throw_restriction = 0;
    victim_obj->flags_09_bits.tightrope_restricted = 1;
    victim_obj->flags_09_bits.face_opponent = 0;
    victim->block_requirement = block_type;
    if (is_plyr_airborn(victim_obj, victim) == 1) {
        if (victim_obj == g_game_info.plyr0.slot.mirror_a) {
            if (victim->state & 0x400) {
                g_game_info.plyr0.slot.pdata->reaction_hit_count++;
            }
            input_state = g_game_info.plyr0.slot.pdata->reaction_hit_count;
        } else {
            if (victim->state & 0x400) {
                g_game_info.plyr1.slot.pdata->reaction_hit_count++;
            }
            input_state = g_game_info.plyr1.slot.pdata->reaction_hit_count;
        }
        if (input_state >= 4 &&
            !(tbl_xfer_addresses[reaction].flags & 0x10)) {
            force_air = 1;
            selected_reaction = 0xF1;
        } else if (!(tbl_xfer_addresses[reaction].flags & 0x10)) {
            force_air = 1;
            selected_reaction = 0xF1;
        }
    }

    switch (aproc->pid) {
    case 0x1001:
    case 0x1002:
        swap_active_plyr_proc();
        break;
    case 0x501D:
    case 0x2026:
    case 0x5019:
    case 0xB00E:
    case 0xB00F:
    case 0xB011:
    case 0xB012:
        if (victim_obj == g_game_info.plyr1.slot.mirror_a) {
            become_plyr2_proc();
        } else {
            become_plyr1_proc();
        }
        break;
    case 0xB010:
    default:
        break;
    }

    if (plyr_pdata->scream_sound_handle != 0) {
        snd_stop(plyr_pdata->scream_sound_handle);
        plyr_pdata->scream_sound_handle = 0;
    }
    reaction_cleanup_active_player();
    plyr_pdata->duck_reaction_active = 0;
    plyr_pdata->his_plyr_pdata->duck_reaction_active = 0;
    scale_me_normal();
    fighter = plyr_pdata->fighter_definition;
    if (fighter->weapon_rest_animation != 0) {
        plyr_spawn_anim(fighter->weapon_rest_animation, p_animate_weapon_rest);
    }

    original_previous_state = plyr_pdata->previous_state;
    saved_state = plyr_pdata->state;
    plyr_obj->flags_09_bits.wall_restricted = 0;
    hold_proc = MK_HDR_LIVE(plyr_pdata->hold_proc, plyr_pdata->hold_proc_instance);

    if (hold_proc != 0) {
        release_other_player();
        if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
            xfer_player_proc(g_game_info.plyr1.idle_proc, p_glitch_to_stance);
        } else {
            xfer_player_proc(g_game_info.plyr0.idle_proc, p_glitch_to_stance);
        }
    }
    plyr_anim_pdata->flags |= 0x40;
    plyr_anim_pdata->step = 1.0f;
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        destroy_mkprocs_pid(0x1005);
    }
    if (plyr_obj == g_game_info.plyr1.slot.mirror_a) {
        destroy_mkprocs_pid(0x1006);
    }

    blocked = 0;
    state_for_bgnd = plyr_pdata->state;
    if ((block_type == 0 || block_type == 7) &&
        am_i_duck_blocking() == 0) {
        blocked = am_i_blocking();
    }
    if (block_type == 1 || block_type == 8) {
        blocked = am_i_duck_blocking();
    }
    if (block_type == 4) {
        if (am_i_blocking() != 0) {
            blocked = 1;
        }
        if (am_i_duck_blocking() != 0) {
            blocked = 0;
        }
    }
    if (block_type == 5) {
        if (am_i_blocking() != 0) {
            blocked = 1;
        }
        if (am_i_duck_blocking() != 0) {
            blocked = 1;
        }
    }
    if (block_type == 9 && am_i_blocking() != 0) {
        blocked = 1;
    }
    if (plyr_pdata->drone_request != 0 && blocked == 0) {
        g_drone_blocking_in_reaction = 0;
        blocked = drone_ai_check_block_at_reactions();
        if (blocked != 0 && block_type == 6) {
            blocked = 0;
        }
        if (plyr_obj->pos_vel.y != 0.0f && plyr_obj->gravity != 0.0f) {
            blocked = 0;
        }
        if (plyr_pdata->blocking_disabled != 0) {
            blocked = 0;
        }
        if (plyr_pdata->blocking_disabled_2 != 0) {
            blocked = 0;
        }
        if (plyr_pdata->blocking_disable_tick_1 >
            (unsigned int)game_tick_ctr) {
            blocked = 0;
        }
        if (plyr_pdata->blocking_disable_tick_2 >
            (unsigned int)game_tick_ctr) {
            blocked = 0;
        }
        if (plyr_pdata->state == 0x4203) {
            blocked = 0;
        }
        if (plyr_pdata->state == 0x4200) {
            blocked = 0;
        }
        if (am_i_airborn() != 0) {
            blocked = 0;
        }
        if (g_drone_faked_out == 1) {
            blocked = 0;
        }
        if (blocked == 1) {
            face_after = 0;
            face_opponent_now();
            g_drone_blocking_in_reaction = 1;
        }
    }
    if (block_type == 2) {
        blocked = 0;
    }
    if (tbl_xfer_addresses[reaction].flags & 8) {
        blocked = 0;
    }
    if (big_boss != 0) {
        if (block_type == 6) {
            blocked = 1;
        }
        if (plyr_pdata->state == 0x60D) {
            blocked = 0;
        }
    }

    if (blocked != 0) {
        applied_damage = damage_scale * 0.1f;
    } else {
        applied_damage = damage_scale * plyr_pdata->damage_multiplier;
    }
    if (opponent_proc->pid == 0x1001) {
        if (check_damage_valid_fc(0, applied_damage) != 0) {
            applied_damage = 0.0f;
        }
        if (mode_of_play == 8) {
            applied_damage =
                trial_damage_callback(0, block_type, applied_damage);
        }
        boost_source =
            g_game_info.plyr1.slot.pdata;
        damage = applied_damage * 0.8f;
        if (boost_source->damage_boost_until >
            (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        (g_game_info.plyr0.slot.pdata)
            ->combo_damage += damage;
        if (g_game_info.plyr0.field_0C == 0.0f) {
            blocked = 0;
        }
    } else {
        if (check_damage_valid_fc(1, applied_damage) != 0) {
            applied_damage = 0.0f;
        }
        if (mode_of_play == 8) {
            applied_damage =
                trial_damage_callback(1, block_type, applied_damage);
        }
        boost_source =
            g_game_info.plyr0.slot.pdata;
        damage = applied_damage * 0.8f;
        if (boost_source->damage_boost_until >
            (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p2_life(-damage);
        (g_game_info.plyr1.slot.pdata)
            ->combo_damage += damage;
        if (g_game_info.plyr1.field_0C == 0.0f) {
            blocked = 0;
        }
    }
    g_drone_faked_out = 0;
    if (blocked == 0 && face_after == 0) {
        face_after = 1;
    }

    if (blocked != 0) {
        selected_reaction = 0xF1;
        plyr_pdata->his_plyr_pdata->collision_result = 2;
        if (tbl_xfer_addresses[reaction].power_level == 0) {
            selected_reaction = 0xF2;
        }
        if (tbl_xfer_addresses[reaction].power_level == 3) {
            selected_reaction = 0xF4;
        }
        if (tbl_xfer_addresses[reaction].power_level == 4 ||
            tbl_xfer_addresses[reaction].power_level == 5) {
            selected_reaction = 0xF5;
        }
        if (tbl_xfer_addresses[reaction].power_level == 0x64) {
            selected_reaction = 0xF6;
        }
    } else {
        plyr_pdata->hit_flash_enabled = plyr_pdata->blocking_disabled;
        if (plyr_pdata->blocking_disable_tick_1 >
            (unsigned int)game_tick_ctr) {
            plyr_pdata->hit_flash_enabled = 1;
        }
        plyr_pdata->his_plyr_pdata->collision_result = 1;
        if (reaction_state == 1) {
            face_reaction = 1;
            plyr_pdata->blocking_disabled_2 = 1;
            plyr_pdata->blocking_disabled = 0;
            victim_obj->flags_09_bits.face_opponent = 1;
        }
        if (reaction_state == 0) {
            plyr_pdata->blocking_disabled = 0;
            plyr_pdata->blocking_disabled_2 = 0;
        }
        if (reaction_state == 2) {
            plyr_pdata->blocking_disabled = 0;
            plyr_pdata->blocking_disabled_2 = 0;
            plyr_pdata->blocking_disable_tick_1 = 0;
            plyr_pdata->blocking_disable_tick_2 = 0;
        }
        plyr_pdata->hit_count++;
        lights = plyr_pdata->plyr_num == 0
                     ? &g_game_info.plyr0.fighting_lights
                     : &g_game_info.plyr1.fighting_lights;
        if (lights->airborne_active ||
            plyr_pdata->combo_hit_count == 0) {
            plyr_pdata->combo_hit_count++;
        } else {
            check_for_combo_message();
            plyr_pdata->combo_hit_count++;
        }
        drone_ai_hit();
        plyr_pdata->hit_streak++;
        plyr_pdata->his_plyr_pdata->hit_streak = 0;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    if (force_air != 0 ||
        (tbl_xfer_addresses[reaction].flags & 2)) {
        init_air_move_no_aniproc();
    } else {
        if (tbl_xfer_addresses[reaction].flags & 1) {
            init_ground_move_no_aniproc();
        }
        if (tbl_xfer_addresses[reaction].flags & 4) {
            init_3d_move_no_aniproc();
        }
    }
    if (g_game_info.plyr0.slot.pdata->state != 0x4203 &&
        g_game_info.plyr1.slot.pdata->state != 0x4203) {
        both_special = 0;
    } else {
        both_special = 1;
    }
    plyr_pdata->state = saved_state;
    plyr_pdata->previous_state = original_previous_state;
    set_my_state(0x600);
    if (aproc->pid == 0x5019 && force_air != 0) {
        plyr_pdata->state |= 0x606;
    }
    if (plyr_pdata->state_flags.bits.frozen) {
        unfreeze_player();
    }
    stop_prison_grab_proc();
    if (plyr_pdata->drone_request != 0) {
        drone_ai_reset_ai_cmd();
    }
    input_state = my_joypad_state_5();
    if (((check_switch(plyr_pdata->controller_port, 1) != 0 &&
          input_state == 3) ||
        (plyr_pdata->drone_request != 0 &&
          drone_ai_check_combo_breaker() != 0)) &&
        g_game_info.flag_bits.lens_flare_enabled &&
        (tbl_xfer_addresses[reaction].flags & 0x200)) {
        if (victim->breaker_strength > 0) {
            selected_reaction = 0x78;
            victim->breaker_strength--;
        }
    }

    if (aproc->pid == 0x1001 || aproc->pid == 0x1002) {
        swap_active_plyr_proc();
    } else {
        exit_plyr_proc();
    }

    dispatch_pair = tbl_xfer_addresses[selected_reaction].dispatch;
    if (opponent_proc != 0) {
        int dispatch_reaction;

        dispatch_reaction = selected_reaction;
        if (face_reaction != 0 && plyr_obj != 0) {
            face_opponent_now();
        }
        if ((unsigned int)state_for_bgnd == 0x4210U) {
            enable_bgnd_obj_repel(
                victim->active_pickup);
        }
        if (blocked != 0) {
            bgnd_clear_danger_zone_callback(victim);
        }
        if (mode_of_play != 6) {
            bgnd_rx_notify(
                victim->plyr_info, selected_reaction,
                tbl_xfer_addresses[selected_reaction].power_level,
                tbl_xfer_addresses[selected_reaction].flags);
        }
        if (!(tbl_xfer_addresses[selected_reaction].flags & 0x100) ||
            big_boss != 0) {
            bgnd_clear_danger_zone_callback(victim);
        }
        if (block_type == 1) {
            bgnd_clear_danger_zone_callback(victim);
        }
        if ((g_game_info.plyr0.field_0C == 0.0f ||
             g_game_info.plyr1.field_0C == 0.0f) &&
            !both_special) {
            bgnd_clear_danger_zone_callback(victim);
        }
        if (victim->online_sync_index != -1) {
            dispatch_reaction = victim->online_sync_index;
            dispatch_pair =
                tbl_xfer_addresses[dispatch_reaction].dispatch;
        }
        if (dispatch_reaction >= 0xE6 && dispatch_reaction <= 0xED) {
            dispatch_pair =
                g_loadable_reaction_scripts[dispatch_reaction - 0xE6].dispatch;
        }
        if (dispatch_pair.call_type == 4) {
            cmdscript->unk28 = (unsigned int)dispatch_pair.entry;
            xfer_player_proc(opponent_proc, r_call_script_function);
        } else if (dispatch_pair.call_type == 3) {
            if ((unsigned int)dispatch_pair.entry == 0x39 &&
                victim->character_id == 0x1B) {
                cmdscript->unk28 = (unsigned int)dispatch_pair.entry;
                xfer_player_proc(
                    opponent_proc, r_call_player_char_script_function);
            } else {
                cmdscript->unk28 = (unsigned int)dispatch_pair.entry;
                xfer_player_proc(
                    opponent_proc, r_call_other_player_char_script_function);
            }
        } else if (dispatch_pair.call_type == 5) {
            xfer_player_proc_to_script(victim_obj, dispatch_pair.entry);
        } else if (dispatch_pair.call_type == 1) {
            xfer_player_proc(opponent_proc, dispatch_pair.entry);
        } else if (dispatch_pair.call_type == 2) {
            cmdscript->unk28 = (unsigned int)dispatch_pair.entry;
            xfer_player_proc(
                opponent_proc, r_call_player_char_script_function);
        }
    }
    return face_after;
}

int reaction_fetch_current_flags(int player) {
    int reaction_index;

    reaction_index =
        g_game_info.plyr0.slot.pdata->pending_reaction;
    if (player == 1) {
        reaction_index =
            g_game_info.plyr1.slot.pdata->pending_reaction;
    }
    if (reaction_index == 0xFFFF) {
        return 0;
    }
    return tbl_xfer_addresses[reaction_index].flags;
}

int reaction_fetch_current_power_level(int player) {
    int reaction_index;

    reaction_index =
        g_game_info.plyr0.slot.pdata->pending_reaction;
    if (player == 1) {
        reaction_index =
            g_game_info.plyr1.slot.pdata->pending_reaction;
    }
    if (reaction_index == 0xFFFF) {
        return 2;
    }
    return tbl_xfer_addresses[reaction_index].power_level;
}

void load_script_as_reaction(unsigned int slot, int script) {
    if (slot <= 7U) {
        g_loadable_reaction_scripts[slot].dispatch.entry = (ReactionEntry)script;
    }
}

void reaction_xfer_him_nohit(int reaction) {
    int hit_count;

    if (his_pdata->blocking_disabled_2 != 0) {
        hit_count = his_pdata->hit_count;
        his_pdata->hit_count = hit_count - 1;
        reaction_xfer_him(reaction, 0.0f, 2);
    }
}

static float r_ZZZZZZZ(void) {
    return 1.0f;
}

void damage_player(PlyrPdata* source, float amount) {
    PlyrPdata* boost_source;
    float damage;

    if (source->plyr_num == 0) {
        boost_source =
            g_game_info.plyr1.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        (g_game_info.plyr0.slot.pdata)
            ->combo_damage += damage;
        return;
    }

    boost_source = g_game_info.plyr0.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    (g_game_info.plyr1.slot.pdata)->combo_damage +=
        damage;
}

void damage_him(float amount) {
    PlyrPdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                g_game_info.plyr0.slot.pdata;
            damage = amount;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            (g_game_info.plyr1.slot.pdata)
                ->combo_damage += damage;
            return;
        }

        boost_source =
            g_game_info.plyr1.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        (g_game_info.plyr0.slot.pdata)
            ->combo_damage += damage;
    }
}

void damage_me(float amount) {
    PlyrPdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                g_game_info.plyr1.slot.pdata;
            damage = amount;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            (g_game_info.plyr0.slot.pdata)
                ->combo_damage += damage;
            return;
        }

        boost_source =
            g_game_info.plyr0.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p2_life(-damage);
        (g_game_info.plyr1.slot.pdata)
            ->combo_damage += damage;
    }
}

void damage_p2(float amount) {
    PlyrPdata* boost_source;
    float damage;

    boost_source = g_game_info.plyr0.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    (g_game_info.plyr1.slot.pdata)->combo_damage +=
        damage;
}

void damage_p1(float amount) {
    PlyrPdata* boost_source;
    float damage;

    boost_source = g_game_info.plyr1.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p1_life(-damage);
    (g_game_info.plyr0.slot.pdata)->combo_damage +=
        damage;
}

static float r_call_other_player_char_script_function(void) {
    PlyrPdata* other;

    cmdscript_reset_stack();
    other = plyr_pdata->his_plyr_pdata;
    cmdscript_setup_execution(other->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->his_plyr_pdata->cmo);
    return 0.0f;
}

float r_call_player_char_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
    return 0.0f;
}

float r_call_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(reactions_cmo, active_cmdscript->unk28);
    call_player_script_function(reactions_cmo);
    return 0.0f;
}

static float r_chest2_separate(void) {
    medium_flash_check();
    face_opponent_now();
    wall_eligible_on();
    got_hit_fx(2, 4, 0, 0xA, 0, 0.025f, 0);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(0.1f, 9, 0.9f, 8);
    plyr_anim_pdata->weight = 1.0f;
    plyr_anim_pdata->step = 0.7f;
    glitch_to_ani(shared_ani.chest_stumble, 3);
    set_anim_hiframe(47.0f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_cyrus_stomp(void) {
    face_opponent_now();
    disable_both_repel_flags();
    got_hit_fx(2, 5, 1, 0xA, 0, 0.05f, 0);
    blend_to_ani(shared_ani.cyrus_stomp, 3, 0.2f);
    ani_to_frame_x_call(same_xz, 8.0f);
    got_hit_fx(4, 8, 1, 0, 0, 0.0f, 0);
    init_ground_move();
    ani_to_end();
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

static void same_xz(void) {
    plyr_obj->pos.value.x = his_obj->pos.value.x;
    plyr_obj->pos.value.z = his_obj->pos.value.z;
}

float r_obstacle_falldown(void) {
    init_air_move_no_aniproc();
    stop_me();
    face_opponent_now();
    danger_zone_eligible_on();
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 0.05f, 0);
    myvel_his_angle_y(0.0f, 0.045f, 0.045f);
    launch_n_land_ani(
        shared_ani.falling_back, 0.0f, 0.0f, 24.0f, 0,
        0.1f, -0.004f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_complete_ermac_slam(void) {
    PlyrPdata* boost_source;
    float damage;

    _mkproc_sleep_ticks = 50.0f * inverse_game_speed;
    aproc->vtbl->sleep();
    plyr_obj->gravity = -0.1f;
    if (his_pdata->character_id == 0x19 ||
        his_pdata->character_id == 0x1A) {
        snd_req(0x307);
    } else {
        snd_req(0x254);
    }
    blend_to_ani(shared_ani.ermac_slam, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(6.0f);
    wait_to_land();
    snd_req(0x255);
    snd_req(0x1D7);
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                g_game_info.plyr1.slot.pdata;
            damage = 0.07f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            (g_game_info.plyr0.slot.pdata)
                ->combo_damage += damage;
        } else {
            boost_source =
                g_game_info.plyr0.slot.pdata;
            damage = 0.07f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            (g_game_info.plyr1.slot.pdata)
                ->combo_damage += damage;
        }
    }
    adjust_my_damage_multiplier(0.6f);
    set_my_state(0x3203);
    init_air_move();
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 2);
    plyr_anim_pdata->step = 0.6f;
    launch_me_up(0.08f, -0.003f);
    myvel_my_angle_y(3.1428f, -0.04f, -0.04f);
    ani_to_frame_x(34.0f);
    set_my_state(0x600);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 2);
    random_hit(9);
    bulvan_function(0);
    init_ground_move();
    stop_me();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_shujinko_slam(void) {
    got_hit_fx(2, 0xD, 4, 0, 0, 0.0f, 2);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation_a, 0, 0.1f);
    aproc->vtbl->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

static float r_ermac_slam(void) {
    got_hit_fx(2, 0xD, 4, 0, 0, 0.0f, 2);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    aproc->vtbl->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

static float r_fan_lift(void) {
    adjust_my_damage_multiplier(0.6f);
    face_opponent_now();
    set_my_state(0x4206);
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_pdata->blocking_disabled = 1;
    random_voice(5);
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    force_away(0.1f, 8, 0.85f, 8);
    _mkproc_sleep_ticks = 30.0f;
    aproc->vtbl->sleep();
    force_away(-0.03f, 0x64, 0.95f, 0xA);
    _mkproc_sleep_ticks = 120.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float j_counter_caught(void) {
    int reaction;

    init_ground_move_no_aniproc();
    stop_me();
    random_voice(9);
    shake_hit_voice(2, 0.02f, 6, 5);
    disable_blocking();
    reaction = plyr_pdata->script_exit_value_int;
    switch (reaction) {
    case 9:
        blend_to_ani(shared_ani.counter_caught_9, 0, 0.1f);
        break;
    case 8:
        blend_to_ani(shared_ani.counter_caught_8, 0, 0.1f);
        break;
    case 7:
        blend_to_ani(shared_ani.counter_caught_7, 0, 0.1f);
        break;
    case 6:
        blend_to_ani(shared_ani.counter_caught_6, 0, 0.1f);
        break;
    }
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 22.0f;
    aproc->vtbl->sleep();
    while (his_pdata->state == 0x4000) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    _mkproc_sleep_ticks = 15.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->jump_sleep(r_counter_caught_abort, 0.0f);
    return 0.0f;
}

static float r_counter_caught_abort(void) {
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    blend_to_fstance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_counter_catch_med(void) {
    blend_to_ani(shared_ani.counter_caught, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    set_my_state(0x4202);
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep();
    set_my_state(0x4000);
    _mkproc_sleep_ticks = 70.0f;
    aproc->vtbl->sleep();
    reaction_xfer_him(0xBF, 0.0f, 2);
    plyr_pdata->summon_position_x = 20.0f;
    aproc->vtbl->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

static float r_post_surf_throw(void) {
    if (stay_down_check() != 0) {
        plyr_pdata->death_type = 1;
        aproc->vtbl->jump_sleep(j_stay_down_dead, 0.0f);
        return 0.0f;
    }
    force_away(0.1f, 0xA, 0.9f, 0x10);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    tightrope_restrictions_on();
    glitch_to_ani(shared_ani.post_surf_getup, 3);
    plyr_anim_pdata->step = 1.2f;
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_block_hit_projectile(void) {
    stop_me();
    init_ground_move();
    blocked_fx(5, 0, 0, 0, 0);
    force_away(0.25f, 2, 0.4f, 5);
    aproc->vtbl->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 99.97%; hit/world Vec stack slots swapped; both declaration orders regress. */
/* TODO: [Scope warn] hit_position and bone_offset/world_position/screen_position blocks: hoisting drops 99.97% -> 90.59%. */
static float r_combo_broken_part2(void) {
    struct ReactionImageFaderPdata* fader;
    PlyrInfo* source;
    ScreenObj* image;
    MkObj* object;
    unsigned int effect;
    int index;
    int half_width;

    {
        Vec hit_position;

        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("breaker_hit_fx", 1);
        } else {
            effect = fx_by_owner("breaker_hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &hit_position);
        hit_position.y = 1.7f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, hit_position.x, hit_position.y, hit_position.z);
    }

    source = plyr_pdata->plyr_info;
    {
        Vec bone_offset = {0.0f, 0.0f, 0.0f};
        Vec world_position;
        RwV2d screen_position;

        image = load_named_2d_pfxobj(
            0x10005, 0xC021, "BREAKER", 0, 0x2F);
        get_bone_offset_world_pos(
            source->slot.mirror_a, 9, &bone_offset, &world_position);
        world_position.y = 1.7f + g_game_info.field_34;
        camera_get_screen_pos_from_world_pos(
            &world_position, &screen_position);
        half_width = image->pfx2d->tex_w / 2;
        image->x = (int)screen_position.x - half_width;
        image->y = screen_position.y;
    }

    if (_create_mkproc_generic_nostack(
            0xC02A, 0x1F, p_image_fader,
            sizeof(struct ReactionImageFaderPdata),
            (MkHdr**)&fader) != 0) {
        fader->object = image;
        fader->object_instance = image->instance;
        fader->delay = 60;
        fader->alpha = 0xFF;
        fader->direction = source->field_04;
    }
    if (image != 0 && his_pdata->breaker_strength == 0) {
        for (index = 0; index < 4; index++) {
            image->pfx2d->verts[index].r = 0x80;
            image->pfx2d->verts[index].g = 0;
            image->pfx2d->verts[index].b = 0;
            image->pfx2d->verts[index].a = 0xFF;
        }
    }
    snd_req(0xDC4);
    face_opponent_now();
    wall_eligible_on();
    start_blood_particles_scripts(0x39, 0x10);
    got_hit_fx(0, 2, 4, 1, 0, 0.0f, 0);
    force_away(0.025f, 0x14, 0.9f, 8);
    blend_to_ani(shared_ani.combo_broken_launch, 3, 0.5f);
    set_ani_speed(0.6f);
    ani_to_frame_x(13.0f);
    got_hit_fx(4, 8, 0, 0, 0, 0.0f, 0);
    back_rollup_check();
    ani_x_more_frames(5.0f);
    blend_to_ani(shared_ani.combo_broken_recover, 3, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_combo_broken_part1(void) {
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    _mkproc_sleep_ticks = 40.0f;
    aproc->vtbl->sleep();
    blend_to_stance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_combo_breaker(void) {
    trial_increment_state_value(plyr_pdata->plyr_num, 0x20, 0);
    adjust_my_damage_multiplier(0.75f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_anim_pdata->weight = 1.3f;
    plyr_anim_pdata->step = 0.7f;
    face_opponent_now();
    got_hit_fx(2, 5, 2, 0, 0, 0.0f, 0);
    reaction_xfer_him(0x79, 0.0f, 2);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    plyr_obj->flags_09_bits.bit6 = 1;
    blend_to_ani_frame(shared_ani.combo_breaker, 3, 0.33f, 7.0f);
    set_ani_speed(1.0f);
    ani_to_frame_x(11.0f);
    reaction_xfer_him(0x7A, 0.0f, 2);
    got_hit_fx(2, 5, 1, 0, 0, 0.0f, 0);
    set_ani_speed(0.5f);
    ani_to_frame_x(25.0f);
    ani_to_blend_frame(3.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_block_hit_p5(void) {
    stop_me();
    init_ground_move();
    blocked_fx(0xB, 5, 0, 0, 0);
    force_away(0.2f, 3, 0.85f, 8);
    aproc->vtbl->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

static float r_block_hit_p3(void) {
    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(0.25f, 2, 0.4f, 5);
    aproc->vtbl->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

static float r_block_hit_p1(void) {
    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(0.15f, 2, 0.5f, 4);
    disable_my_attacks(6);
    aproc->vtbl->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

static float r_block_hit_p0(void) {
    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(0.25f, 2, 0.4f, 5);
    disable_my_attacks(6);
    aproc->vtbl->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

static float j_block_common_reaction(void) {
    if (!(plyr_pdata->previous_state & 0x800) &&
        plyr_pdata->drone_request == 1) {
        aproc->vtbl->jump_sleep(x_block, 0.0f);
        return 0.0f;
    }
    if (plyr_pdata->previous_state & 0x100) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x12, 0);
        trial_increment_state_value(plyr_pdata->plyr_num, 0x13, 0);
        init_ground_move();
        set_my_state(0xF00);
        force_away(0.15f, 2, 0.85f, 8);
        if (should_i_weapon_block() != 0) {
            xfer_proc(plyr_anim_proc, p_anim_idle);
            blend_to_ani(
                plyr_pdata->fighter_definition->weapon_block_reaction,
                3, 0.2f);
            plyr_anim_pdata->step = 1.0f;
            ani_to_end();
            xfer_proc(plyr_anim_proc, p_animate);
        } else {
            xfer_proc(plyr_anim_proc, p_anim_idle);
            blend_to_ani(shared_ani.standing_weapon_block, 3, 0.2f);
            plyr_anim_pdata->step = 1.0f;
            ani_to_end();
            xfer_proc(plyr_anim_proc, p_animate);
        }
    } else {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x12, 0);
        trial_increment_state_value(plyr_pdata->plyr_num, 0x14, 0);
        if (should_i_weapon_block() == 0) {
            if (plyr_pdata->previous_state == 0xA00 ||
                !(plyr_pdata->previous_state & 0x800)) {
                plyr_pdata->his_attack_counter = get_his_attack_counter();
                set_my_state(0xA00);
                blend_to_ani(shared_ani.standing_block_a, 0, 0.5f);
            }
            if (plyr_pdata->previous_state == 0xA01) {
                plyr_pdata->his_attack_counter = get_his_attack_counter();
                set_my_state(0xA01);
                blend_to_ani(shared_ani.standing_block_b, 0, 0.5f);
            }
            if (plyr_pdata->previous_state == 0xA02) {
                plyr_pdata->his_attack_counter = get_his_attack_counter();
                set_my_state(0xA02);
                blend_to_ani(shared_ani.standing_block_c, 0, 0.5f);
            }
            if (plyr_pdata->previous_state == 0xA03) {
                plyr_pdata->his_attack_counter = get_his_attack_counter();
                set_my_state(0xA03);
                blend_to_ani(shared_ani.standing_block_d, 0, 0.5f);
            }
        } else {
            set_my_state(0xA00);
            blend_to_ani(
                plyr_pdata->fighter_definition->weapon_block_animation,
                3, 0.1f);
            plyr_anim_pdata->step = 1.0f;
        }
    }
    if (am_i_duck_blocking() != 0) {
        if (should_i_weapon_block() != 0) {
            blend_to_ani(
                plyr_pdata->fighter_definition->duck_block_animation,
                0, 0.2f);
        } else {
            blend_to_ani(shared_ani.duck_block, 0, 0.2f);
        }
        aproc->vtbl->jump_sleep(j_duck_block_loop, 0.0f);
        return 0.0f;
    }
    if (am_i_blocking() != 0) {
        if (plyr_pdata->state & 0x100) {
            plyr_pdata->his_attack_counter = get_his_attack_counter();
            set_my_state(0xA00);
            blend_to_ani(shared_ani.standing_block_a, 0, 0.1f);
        }
        aproc->vtbl->jump_sleep(j_block_loop, 0.0f);
        return 0.0f;
    }
    blend_to_stance(0.1f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_nightwolf_lightning(void) {
    high_flash_check();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 0.05f, 1);
    random_voice(0x13);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0.0f, 0.0f, 24.0f, 0,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_mileena_hit(void) {
    PlyrPdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                g_game_info.plyr1.slot.pdata;
            damage = 0.12f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            (g_game_info.plyr0.slot.pdata)
                ->combo_damage += damage;
        } else {
            boost_source =
                g_game_info.plyr0.slot.pdata;
            damage = 0.12f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            (g_game_info.plyr1.slot.pdata)
                ->combo_damage += damage;
        }
    }
    aproc->vtbl->jump_sleep(r_face3_onback, 0.0f);
    return 0.0f;
}

static float r_nightwolf_charge(void) {
    high_flash_check();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    if (his_pdata->character_id == 1 && his_pdata->attack_region == 0) {
        random_voice(3);
        random_hit(0xD);
        shake_camera(2, 0.02f);
        start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
        face_bleed_me(3);
    } else {
        got_hit_fx(0, 2, 1, 3, 0, 0.05f, 0);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0.0f, 0.0f, 24.0f, 0,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_face3_onback(void) {
    high_flash_check();
    face_opponent_now();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    if (his_pdata->character_id == 1 && his_pdata->attack_region == 0) {
        random_voice(3);
        random_hit(0xD);
        shake_camera(2, 0.02f);
        start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
        face_bleed_me(3);
    } else {
        got_hit_fx(0, 2, 1, 3, 0, 0.05f, 0);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0.0f, 0.0f, 24.0f, 0,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_cyrax_blade(void) {
    float angle;
    float sine;
    float cosine;

    angle = his_obj->ang.y;
    angle = 0.000005992112f * (float)(((int)(166886.1f * angle)) & 0xFFFFF);
    sine = gxMathSin(angle);
    cosine = gxMathCos(angle);
    plyr_obj->pos.value.x = his_obj->pos.value.x + 2.0f * sine;
    plyr_obj->pos.value.z = his_obj->pos.value.z + 2.0f * cosine;
    plyr_obj->pos_vel.x = 0.0f;
    plyr_obj->pos_vel.z = 0.0f;
    plyr_obj->gravity = -0.0075f;
    face_opponent_now();
    wall_eligible_off();
    shake_camera(6, 0.02f);
    start_blood_particles(0x18, 0x10, plyr_pdata, plyr_obj);
    random_voice(3);
    blend_to_ani(his_pdata->reaction_animation, 3, 0.1f);
    plyr_anim_pdata->weight = 0.0f;
    plyr_anim_pdata->step = 0.75f;
    xfer_proc(plyr_anim_proc, p_animate);
    while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame - 20.0f &&
           his_pdata->state != 0x420F) {
        start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
        _mkproc_sleep_ticks = 9.0f;
        aproc->vtbl->sleep();
        snd_req(0xD81);
        _mkproc_sleep_ticks = 9.0f;
        aproc->vtbl->sleep();
        snd_req(0xD81);
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    init_ground_move();
    face_bleed_me(1);
    face_opponent_now();
    snd_req(0xD81);
    force_away(0.2f, 3, 0.8f, 8);
    blend_to_ani(shared_ani.cyrax_blade, 3, 0.5f);
    set_ani_speed(0.5f);
    ani_to_frame_x(20.0f);
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_head3_onback(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 0, 0, 3, 0, 0.05f, 3);
    r_top_of_head_slam();
    blend_to_ani(shared_ani.head_slam_fall, 3, 0.2f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(27.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    got_hit_fx(4, 8, 1, 0, 0, 0.0f, 0);
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static void r_top_of_head_slam(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0.05f, 0);
    random_hit(3);
    blend_to_ani(shared_ani.top_of_head_slam, 3, 0.5f);
    plyr_anim_pdata->step = 1.5f;
    ani_to_frame_x(9.0f);
    plyr_anim_pdata->step = 2.5f;
    ani_to_blend_frame(4.0f);
}

float r_jump_slambounce_final_hit(void) {
    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    special_move_cam_setup(
        1.47f, 4.1f, 1.0f, -1.75f, -0.15f, 0xA, 0x3C, 0);
    reaction_xfer_him(0xDB, 0.0f, 2);
    got_hit_fx(0, 2, 1, 1, 0, 0.05f, 0);
    blend_to_ani(shared_ani.jump_slambounce, 3, 0.2f);
    set_ani_speed(0.7f);
    ani_to_frame_x(6.0f);
    init_ground_move();
    ani_to_frame_x(50.0f);
    got_hit_fx(4, 0, 1, 0, 0, 0.0f, 1);
    ani_to_end();
    check_for_combo_message();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_jump_chin3_final_hit(void) {
    special_move_cam_setup(
        1.47f, 4.1f, 1.0f, -1.75f, -0.15f, 0xA, 0x3C, 0);
    reaction_xfer_him(0xDC, 0.0f, 2);
    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->pos.value.y = his_obj->pos.value.y;
    stop_me();
    init_air_move();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(2, 0.02f, 4, 3);
    start_blood_particles_scripts(0x39, 0x10);
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    set_my_state(0x605);
    blend_to_ani(shared_ani.jump_chin, 3, 0.2f);
    set_ani_speed(0.7f);
    ani_to_frame_x(43.0f);
    plyr_obj->gravity = -0.1f;
    wait_to_land();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    got_hit_fx(4, 0, 1, 0, 0, 0.0f, 1);
    set_my_state(0x600);
    back_rollup_check();
    ani_to_frame_x(60.0f);
    set_ani_speed(2.0f);
    init_ground_move();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_slamdown_final_hitter(void) {
    head_tracking_on();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    set_ani_speed(0.25f);
    ani_to_blend_frame(3.0f);
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_stance(0.05f);
    _mkproc_sleep_ticks = 6.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_popup_final_hitter(void) {
    int ticks;

    head_tracking_on();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    if (xz_distance_between_players() < 1.0f) {
        force_away(0.1f, 9, 0.9f, 8);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    set_ani_speed(0.33f);
    _mkproc_sleep_ticks = 30.0f;
    aproc->vtbl->sleep();
    blend_to_fstance(0.05f);
    ticks = 0x28;
    while (is_he_airborn() == 1 && ticks > 0) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        ticks--;
    }
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_enough_air_already(void) {
    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(3);
    shake_hit_voice(2, 0.02f, 1, 0xD);
    random_hit(4);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.enough_air, 0.0f, 0.0f, 28.0f, 0xCA1,
        0.05f, -0.008f, 0.33f);
    shake_hit_voice(2, 0.02f, 9, 8);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

static float r_airborn_small_lift(void) {
    medium_flash_check();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(0, 0.0f, 0, 4);
    if (plyr_pdata->reaction_hit_count == 2) {
        plyr_pdata->f_constrained = 0;
    }
    if ((plyr_pdata->f_constrained == 1) &
        (plyr_pdata->reaction_hit_count >= 3)) {
        reaction_xfer_him(0xFC, 0.0f, 2);
    }
    myvel_his_angle_y(0.0f, 0.06f, 0.06f);
    launch_n_land_ani(
        shared_ani.airborn_small_lift, 0.0f, 0.0f, 31.0f, 0xCA1,
        0.0325f, -0.006f, 0.33f);
    shake_hit_voice(2, 0.02f, 9, 8);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(36.0f);
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_hit_airborn1(void) {
    medium_flash_check();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(0, 0.0f, 1, 3);
    if (plyr_pdata->reaction_hit_count == 2) {
        plyr_pdata->f_constrained = 0;
    }
    if ((plyr_pdata->f_constrained == 1) &
        (plyr_pdata->reaction_hit_count >= 3)) {
        reaction_xfer_him(0xFC, 0.0f, 2);
    }
    if (plyr_pdata->reaction_hit_count >= 3) {
        myvel_his_angle_y(0.0f, 0.06f, 0.06f);
    } else {
        myvel_his_angle_y(0.0f, 0.04f, 0.04f);
    }
    launch_n_land_ani(
        shared_ani.airborn_small_lift, 0.0f, 0.0f, 31.0f, 0xCA1,
        0.08f, -0.006f, 0.33f);
    enable_all_my_blocking();
    shake_hit_voice(2, 0.02f, 9, 8);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(36.0f);
    aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_corner_repell_ani(void) {
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(0.15f, 0x12, 0.9f, 8);
    ani_to_blend_frame(2.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_corner_repell(void) {
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(0.1f, 9, 0.9f, 8);
    _mkproc_sleep_ticks = 20.0f;
    aproc->vtbl->sleep();
    blend_to_stance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_sidehead3_dive_opposite(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0.05f, 0);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(2);
    newani_to_frame_x(shared_ani.side_head_dive, 22.0f, 1.0f, 1.0f, 0.2f, 2);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0, 0, 0.0f, 0.0f);
    shake_hit_voice(2, 0.02f, 9, 7);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

static float r_sidehead3_dive(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0.05f, 0);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    face_bleed_me(2);
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    newani_to_frame_x(shared_ani.side_head_dive, 22.0f, 1.0f, 1.0f, 0.2f, 1);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xCA1, 0xCA1, 2.0f, 0.02f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 98.49315%; equivalent ordered-sign bne+b branch remains;
 * measured scalar, owner, complete-step and in-place helpers regress. */
static float r_sidehead3_spin(void) {
    float flight_ticks;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 3, 0, 0.05f, 0);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    myvel_his_angle_y(-0.71f, 0.03f, 0.03f);
    plyr_anim_pdata->step = 0.7f;
    blend_to_ani(shared_ani.side_head_spin, 3, 0.33f);
    launch_me_up(0.06f, -0.003f);
    flight_ticks = 2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (flight_ticks >= 0.0f) {
    } else {
        flight_ticks = -flight_ticks;
    }
    plyr_anim_pdata->step = 27.0f / flight_ticks;
    ani_to_frame_x(27.0f);
    wait_to_land();
    tightrope_restrictions_on();
    shake_camera(3, 0.03f);
    ani_to_end();
    blend_to_stance(0.1f);
    aproc->vtbl->jump_sleep(p_joy_loop, 0.0f);
    return 0.0f;
}

static float r_feet3_sweptout_rev(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0.0f, 0x10);
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_reverse, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(-1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_swept_in(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0.0f, 0x10);
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_in, shared_ani.swept_out,
        0.2f, 0.8f, 1.0f);
    myvel_his_angle_y_inout(1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_swept_out(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0.0f, 0x10);
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_out, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(-1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    back_rollup_check();
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_sweptin_rev(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0.0f, 0x10);
    tightrope_restrictions_off();
    blend_to_ani_INOUT(
        shared_ani.swept_reverse, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet1a(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0.0f, 0x10);
    force_away(0.1f, 3, 0.9f, 8);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_feet1_stay_close(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0.0f, 0x10);
    force_away(0.04f, 3, 0.9f, 8);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_gut3_onfeet_hard(void) {
    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0.0f, 0);
    plyr_obj->flags_09_bits.launched = 0;
    blend_to_ani(shared_ani.gut_on_feet, 3, 0.33f);
    plyr_anim_pdata->step = 0.85f;
    plyr_anim_pdata->weight = 1.6f;
    ani_to_frame_x(24.0f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    player_feet_land_chores();
    ani_to_blend_frame(10.0f);
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_gut3_onfeet(void) {
    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0.0f, 0);
    plyr_obj->flags_09_bits.launched = 0;
    blend_to_ani(shared_ani.gut_on_feet, 3, 0.33f);
    plyr_anim_pdata->step = 0.85f;
    ani_to_frame_x(24.0f);
    init_ground_move();
    blend_to_stance(0.04f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_gut3_onbutt(void) {
    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0.0f, 0);
    plyr_obj->flags_09_bits.launched = 0;
    force_away(0.1f, 3, 0.9f, 8);
    blend_to_ani(shared_ani.gut_on_butt, 3, 0.33f);
    ani_to_frame_x(18.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    ani_to_fall_to_frame(24.0f, 0xD7F, plyr_anim_pdata->high_frame);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->jump_sleep(j_getup_sit_12, 0.0f);
    return 0.0f;
}

static float r_jax_piston_hi(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 1, 5, 4, 0, 0.025f, 0);
    blend_to_ani(shared_ani.jax_piston_high, 3, 0.33f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_blend_frame(10.0f);
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_jax_piston_lo(void) {
    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 5, 0, 0, 0.0f, 0);
    adjust_my_damage_multiplier(0.75f);
    blend_to_ani(shared_ani.jax_piston_low, 3, 0.2f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_frame_x(20.0f);
    plyr_pdata->summon_position_x = 15.0f;
    aproc->vtbl->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

static float r_chest2_stumble_shake(void) {
    adjust_my_damage_multiplier(0.75f);
    face_opponent_now();
    got_hit_fx(2, 2, 1, 0xB, 0, 0.0f, 0);
    random_hit(5);
    aproc->vtbl->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}

float r_chest2_stumble(void) {
    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0.0f, 0);
    aproc->vtbl->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}

static float chest_stumble_both(void) {
    medium_flash_check();
    add_facial_damage(0.025f);
    face_opponent_now();
    wall_eligible_on();
    adjust_my_damage_multiplier(0.75f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_anim_pdata->weight = 1.3f;
    plyr_anim_pdata->step = 0.7f;
    blend_to_ani_frame(shared_ani.chest_stumble, 3, 0.33f, 7.0f);
    set_anim_hiframe(47.0f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_esp1_B(void) {
    float damage;
    int ticks;

    ticks = 0x1E;
    face_opponent_now();
    plyr_obj->gravity = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->flags_09_bits.face_opponent = 0;
    random_voice(0xD);
    wall_eligible_on();
    myvel_his_angle_y(0.0f, -0.16f, -0.16f);
    blend_to_ani(his_pdata->reaction_animation_a, 3, 0.1f);
    ani_to_end();
    blend_to_ani(his_pdata->reaction_animation_b, 0, 0.1f);
    while (xz_distance_between_players() > 2.25 && ticks > 0) {
        ticks--;
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_loop_more_frames(7.0f);
    blend_to_ani(his_pdata->reaction_animation_c, 3, 0.1f);
    ani_to_frame_x(19.0f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            damage = 0.14f;
            damage *= 0.8f;
            if ((g_game_info.plyr1.slot.pdata)
                    ->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= (
                    g_game_info.plyr1.slot.pdata)->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            (g_game_info.plyr0.slot.pdata)
                ->combo_damage += damage;
        } else {
            damage = 0.14f;
            damage *= 0.8f;
            if ((g_game_info.plyr0.slot.pdata)
                    ->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= (
                    g_game_info.plyr0.slot.pdata)->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            (g_game_info.plyr1.slot.pdata)
                ->combo_damage += damage;
        }
    }
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

static float r_esp1_A(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0.0f, 2);
    blend_to_ani(his_pdata->esp1_reaction_animation, 3, 0.1f);
    plyr_obj->gravity = -0.0075f;
    plyr_anim_pdata->step = 0.8f;
    ani_to_blend_frame(10.0f);
    plyr_pdata->summon_position_x = 10.0f;
    aproc->vtbl->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

static float r_summon_flames(void) {
    low_flash_check();
    face_opponent_now();
    random_voice(0x14);
    blend_to_ani(his_pdata->reaction_animation_a, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    blend_to_ani(his_pdata->reaction_animation_b, 3, 0.5f);
    ani_to_end();
    blend_to_ani(his_pdata->reaction_animation_c, 3, 0.1f);
    ani_to_end();
    plyr_pdata->summon_position_x = 15.0f;
    (aproc->vtbl)
        ->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

static float r_subzero_iceball(void) {
    int his_character;
    int my_character;
    int collision;
    int demo_interrupt = 0;

    medium_flash_check();
    destroy_subzero_decoy();
    init_air_move_no_aniproc();
    collision = local_collision_allowed(plyr_pdata);
    if (collision != 0 &&
        (plyr_pdata->previous_state == 0xC600 ||
         plyr_pdata->previous_state == 0xC602)) {
        reaction_xfer_him(0xA1, 0.0f, 2);
        blend_to_stance(0.05f);
        aproc->vtbl->jump_sleep(j_exit, 0.0f);
        return 0.0f;
    }
    if (g_game_info.feature_flags.bits.high_bit == 0 ? 0 : demo_interrupt) {
        aproc->vtbl->jump_sleep(blend_to_stance_j_exit, 0.0f);
        return 0.0f;
    }
    plyr_pdata->blocking_disabled = 1;
    plyr_pdata->blocking_disabled_2 = 1;
    freeze_player();
    if (am_i_airborn_check_in_reaction() != 0) {
        init_air_move_no_aniproc();
        set_my_state(0xC602);
        plyr_obj->flags_09_bits.face_opponent = 0;
        stop_me();
        _mkproc_sleep_ticks = 120.0f;
        aproc->vtbl->sleep();
        plyr_obj->gravity = -0.01f;
        wait_to_land();
        unfreeze_player();
        plyr_pdata->blocking_disabled = 0;
        plyr_pdata->blocking_disabled_2 = 0;
        set_my_state(0);
        got_hit_fx(4, 8, 1, 0, 0, 0.0f, 0);
        face_opponent_now();
        plyr_pdata->summon_position_x = plyr_obj->pos.value.x;
        plyr_pdata->summon_position_z = plyr_obj->pos.value.z;
        glitch_to_ani(shared_ani.falling_back, 3);
        plyr_anim_pdata->frame = 23.0f;
        ani_1_frame();
        plyr_obj->pos.value.x = plyr_pdata->summon_position_x;
        plyr_obj->pos.value.z = plyr_pdata->summon_position_z;
        back_rollup_check();
        ani_to_end();
        aproc->vtbl->jump_sleep(j_getup_back_6, 0.0f);
        return 0.0f;
    }
    set_my_state(0xC600);
    plyr_obj->flags_09_bits.face_opponent = 0;
    stop_me();
    init_ground_move();
    his_character = his_pdata->character_id;
    if (his_character == 3) {
        blend_to_ani(his_pdata->reaction_animation, 3, 0.2f);
    } else {
        my_character = plyr_pdata->character_id;
        if (my_character == 3) {
            blend_to_ani(plyr_pdata->reaction_animation, 3, 0.2f);
        } else if (his_character == 0x19 || his_character == 0x1A) {
            blend_to_ani(his_pdata->ice_reaction_animation, 3, 0.2f);
        } else if (my_character == 0x19 || my_character == 0x1A) {
            blend_to_ani(plyr_pdata->ice_reaction_animation, 3, 0.2f);
        }
    }
    ani_to_end();
    _mkproc_sleep_ticks = 40.0f;
    aproc->vtbl->sleep();
    unfreeze_player();
    set_my_state(0);
    plyr_pdata->summon_position_x = 25.0f;
    aproc->vtbl->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

static float r_iceball_reversal(void) {
    face_opponent_now();
    set_my_state(0xC600);
    plyr_pdata->blocking_disabled = 1;
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_obj->flags_09_bits.face_opponent = 0;
    freeze_player();
    _mkproc_sleep_ticks = 180.0f;
    aproc->vtbl->sleep();
    unfreeze_player();
    set_my_state(0);
    plyr_pdata->summon_position_x = 25.0f;
    aproc->vtbl->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

static float r_null(void) {
    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    blend_to_stance(0.1f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_throw(void) {
    high_flash_check();
    face_opponent_now();
    blend_to_ani(shared_ani.throw_fall, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(39.0f);
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    plyr_obj->ang.y += 3.14f;
    set_anim_script(plyr_anim_pdata, shared_ani.throw_getup, 3);
    aproc->vtbl->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

float r_hit_wall(void) {
    PlyrPdata* boost_source;
    float damage;

    trial_increment_state_value(plyr_pdata->plyr_num, 0x1C, 1);
    adjust_my_damage_multiplier(0.6f);
    set_my_state(0x601);
    init_air_move_no_aniproc();
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                g_game_info.plyr1.slot.pdata;
            damage = 0.06f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            (g_game_info.plyr0.slot.pdata)
                ->combo_damage += damage;
        } else {
            boost_source =
                g_game_info.plyr0.slot.pdata;
            damage = 0.06f;
            damage *= 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            (g_game_info.plyr1.slot.pdata)
                ->combo_damage += damage;
        }
    }
    plyr_pdata->hit_count++;
    plyr_pdata->hit_streak++;
    plyr_pdata->his_plyr_pdata->hit_streak = 0;
    shake_hit_voice(2, 0.02f, 9, 8);
    blend_to_ani(shared_ani.wall_hit, 3, 0.5f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(7.0f);
    plyr_obj->gravity = -0.0075f;
    if (plyr_pdata->drone_request != 0) {
        plyr_pdata->script_exit_value_int = (randu0(2) & 0xFFFF) + 1;
        aproc->vtbl->jump_sleep(wall_dodge, 0.0f);
        return 0.0f;
    }
    while (plyr_anim_pdata->frame < 23.0f) {
        plyr_pdata->script_exit_value_int = my_joypad_state_5();
        if (plyr_pdata->script_exit_value_int == 1 ||
            plyr_pdata->script_exit_value_int == 2) {
            aproc->vtbl->jump_sleep(wall_dodge, 0.0f);
            return 0.0f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_to_blend_frame(10.0f);
    plyr_obj->flags_09_bits.wall_restricted = 0;
    check_for_combo_message();
    aproc->vtbl->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float r_scorpion_spear_2(void) {
    MkVtableMkproc* vtable;
    int ticks;

    blend_to_ani(his_pdata->scorpion_spear_pull, 3, 0.1f);
    ani_to_end();
    snd_req(0xD70);
    myvel_his_angle_y(0.0f, -0.13f, -0.13f);
    blend_to_ani(his_pdata->scorpion_spear_recover, 0, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ticks = 0;
    while (xz_distance_between_players() > 1.0f && ticks < 0x50) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = aproc->vtbl;
        ticks++;
        vtable->sleep();
    }
    init_ground_move();
    stop_me();
    set_my_state(0x4204);
    if (his_pdata->character_id != 0x19 &&
        his_pdata->character_id != 0x1A) {
        blend_to_ani(his_pdata->reaction_animation, 3, 0.2f);
    } else {
        blend_to_ani(his_pdata->reaction_animation_c, 3, 0.2f);
    }
    plyr_anim_pdata->step = 1.2f;
    plyr_pdata->summon_position_x = 20.0f;
    aproc->vtbl->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

static float r_scorpion_spear_1(void) {
    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    ejb_call(0x31);
    set_my_state(0x603);
    got_hit_fx(2, 5, 1, 0, 0, 0.0f, 0);
    start_blood_particles(0x20, 9, plyr_pdata, plyr_obj);
    blend_to_ani(his_pdata->scorpion_spear_hit, 3, 0.1f);
    ani_to_frame_x(83.0f);
    set_my_state(0x604);
    ani_to_end();
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_judo_throw1(void) {
    PlyrFighterDefinition* fighter;

    fighter = his_pdata->fighter_definition;
    glitch_to_ani(fighter->judo_throw_reaction, 3);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

static float r_raiden_shocker_fall(void) {
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 60.0f;
    aproc->vtbl->sleep();
    init_ground_move();
    aproc->vtbl->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}
