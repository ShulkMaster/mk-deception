/*
 * Port readiness:
 *   Structs: CLEAN
 *   Matching: PARTIAL
 *   Linked: NO
 *   Status: REVIEW
 *   Gaps: reaction transfer, damage, animation, and cleanup implementations
 */
#include "runtime/mk_obj.h"
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

extern MkObj* his_obj;
extern MkObj* plyr_obj;
extern PlyrPdata* his_pdata;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;

typedef struct ReactionProcVtable ReactionProcVtable;
struct ReactionProcVtable {
    void* reserved[6];
    void (*sleep)(ReactionProcVtable* vtable);
    void* reserved_after_sleep[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
};

typedef struct LoadableReactionScript {
    int slot_count;
    int script;
    int flags;
    int power_level;
    int state;
} LoadableReactionScript;

typedef struct ReactionDamagePdata {
    char pad00[0x284];
    unsigned int damage_boost_until;
    float damage_boost;
    char pad28C[8];
    float accumulated_damage;
} ReactionDamagePdata;

typedef struct ReactionCurrentPdata {
    char pad00[0x2F8];
    unsigned int reaction_index;
} ReactionCurrentPdata;

typedef float (*ReactionEntry)(void);

typedef struct ReactionDispatchPair {
    int call_type;
    ReactionEntry entry;
} ReactionDispatchPair;

typedef struct ReactionXferAddress {
    ReactionDispatchPair dispatch;
    int power_level;
    int state;
    int flags;
} ReactionXferAddress;

typedef struct ReactionDispatchContext {
    ReactionDispatchPair transfer;
    int saved_state;
} ReactionDispatchContext;

typedef struct ReactionFighterDefinitionView {
    char pad00[0x158];
    int judo_throw_reaction;
} ReactionFighterDefinitionView;

typedef struct ReactionSharedAnimations {
    void* jax_piston_high;       /* +0x000 */
    int chest_stumble;           /* +0x004 */
    char pad008[0x54];
    void* jax_piston_low;        /* +0x05C */
    char pad060[0x2C];
    void* gut_on_butt;           /* +0x08C */
    char pad090[4];
    void* gut_on_feet;           /* +0x094 */
    char pad098[0x34];
    void* enough_air;            /* +0x0CC */
    char pad0D0[0x2C];
    void* cyrus_stomp;           /* +0x0FC */
    char pad100[0x30];
    void* feet_hit;              /* +0x130 */
    char pad134[8];
    int swept_in;                /* +0x13C */
    int swept_reverse;           /* +0x140 */
    int swept_out;               /* +0x144 */
    char pad148[0x0C];
    union {
        void* falling_back;
        int falling_back_id;
    };                           /* +0x154 */
    void* side_head_spin;        /* +0x158 */
    char pad15C[0x0C];
    void* side_head_dive;        /* +0x168 */
    void* airborn_small_lift;    /* +0x16C */
    char pad170[8];
    void* top_of_head_slam;      /* +0x178 */
    void* head_slam_fall;        /* +0x17C */
    char pad180[0x24];
    void* wall_hit;              /* +0x1A4 */
    char pad1A8[0x0C];
    void* jump_chin;             /* +0x1B4 */
    void* jump_slambounce;       /* +0x1B8 */
    char pad1BC[8];
    void* ermac_slam;            /* +0x1C4 */
    char pad1C8[8];
    void* cyrax_blade;           /* +0x1D0 */
    void* combo_broken_launch;   /* +0x1D4 */
    char pad1D8[0x10];
    void* combo_broken_recover;  /* +0x1E8 */
    char pad1EC[0x2C];
    int post_surf_getup;         /* +0x218 */
    char pad21C[0x0C];
    AniData* throw_getup;        /* +0x228 */
    char pad22C[0x60];
    void* throw_fall;            /* +0x28C */
    char pad290[0x1C];
    void* standing_block_a;      /* +0x2AC */
    char pad2B0[8];
    void* standing_block_b;      /* +0x2B8 */
    char pad2BC[0x0C];
    void* standing_block_c;      /* +0x2C8 */
    char pad2CC[8];
    void* standing_block_d;      /* +0x2D4 */
    char pad2D8[0x0C];
    void* duck_block;            /* +0x2E4 */
    void* standing_weapon_block; /* +0x2E8 */
    char pad2EC[4];
    void* counter_caught;        /* +0x2F0 */
    char pad2F4[0x1C];
    void* counter_caught_6;      /* +0x310 */
    void* counter_caught_7;      /* +0x314 */
    void* counter_caught_8;      /* +0x318 */
    void* counter_caught_9;      /* +0x31C */
    int combo_breaker;           /* +0x320 */
} ReactionSharedAnimations;

typedef struct ReactionPostSurfPdata {
    char pad000[0x6F4];
    int stay_down;
} ReactionPostSurfPdata;

typedef struct ReactionWallPdata {
    char pad000[0x258];
    int wall_hit_count;
} ReactionWallPdata;

typedef struct ReactionStatusFlagsView {
    char pad000[0x134];
    unsigned int cleanup_function;
} ReactionStatusFlagsView;

typedef struct ReactionImageFaderPdata {
    MkHdr hdr;
    ScreenObj* object;
    unsigned int object_instance;
    int direction;
    int alpha;
    int delay;
} ReactionImageFaderPdata;

typedef struct ReactionImageSource {
    MkHdr hdr;
    char pad08[0x54];
    MkObj* object;
} ReactionImageSource;

typedef struct ReactionTransferPdata {
    MkHdr hdr;
    MkProc* opponent_proc;
    unsigned int opponent_proc_instance;
    PlyrPdata* opponent_pdata;
    MkObj* opponent_obj;
} ReactionTransferPdata;

typedef struct ReactionFighterDefinitionDispatchView {
    char pad000[0x1A8];
    void* weapon_rest_animation;
} ReactionFighterDefinitionDispatchView;

typedef struct ReactionPdataRepelView {
    char pad000[0x71C];
    int bgnd_repel_id;
} ReactionPdataRepelView;

typedef struct ReactionPlyrInfoCombatView {
    char pad00[0x1C];
    unsigned char combat_flags;
} ReactionPlyrInfoCombatView;

typedef struct ReactionBladeTransform {
    char pad000[0x60];
    Vec position;
} ReactionBladeTransform;

typedef struct ReactionBladeRoot {
    char pad000[4];
    ReactionBladeTransform* transform;
} ReactionBladeRoot;

typedef struct ReactionBladeFighterDefinition {
    char pad000[4];
    ReactionBladeRoot* blade_root;
    char pad008[4];
    MkObj* blade_object;
    unsigned int blade_object_instance;
} ReactionBladeFighterDefinition;

static LoadableReactionScript g_loadable_reaction_scripts[8] = {
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
};

unsigned int fx_by_owner(const char* name, int owner);
unsigned int fx_next_emitter(unsigned int effect);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z);
void trial_increment_state_value(int player, int state, int amount);
void low_flash_check(void);
void face_opponent_now(void);
void random_voice(int group);
void blend_to_ani(void* animation, int transition, float blend);
void ani_to_end(void);
float j_blend_to_stance_in_x(void);
int reaction_xfer_him(int reaction, float rate, int strength);
void stop_me(void);
void init_ground_move(void);
void blocked_fx(int type, int bone, int third, int fourth, int fifth);
void force_away(int direction, int ticks, float speed, float damping);
void disable_my_attacks(int ticks);
void adjust_my_damage_multiplier(float multiplier);
void got_hit_fx(
    int first, int second, int third, int fourth, int fifth, int sixth,
    float value);
void random_hit(int group);
static float j_block_common_reaction(void);
float j_block_loop(void);
float j_duck_block_loop(void);
float x_block(void);
static float chest_stumble_both(void);
float j_exit(void);
float j_blend_to_fstance_in_x(void);
float j_getup_back_12(void);
void blend_to_stance(float rate);
int blend_to_fstance(float rate);
void freeze_player(void);
void unfreeze_player(void);
void set_my_state(int state);
void glitch_to_ani(int animation, int transition);
void adjust_p1_life(float amount);
void adjust_p2_life(float amount);
int should_weapon_block(PlyrPdata* player);

extern int game_tick_ctr;
extern ScriptSlot* reactions_cmo;
extern ReactionSharedAnimations shared_ani;

void ani_to_frame_x(float frame);
void ani_to_blend_frame(float frame);
void ani_to_frame_x_call(void (*callback)(void), float frame);
void add_facial_damage(float amount);
void check_for_combo_message(void);
void disable_both_repel_flags(void);
void head_tracking_on(void);
void init_air_move(void);
void p_blend_to_stance_in_10(void);
float p_animate(void);
float p_anim_idle(void);
float p_sh_throw_plyr_in_grinder(void);
float r_beetle_lair_transition(void);
int is_big_boss();
int big_boss_reaction_remap();
float drone_ai_get_big_boss_damage_scale();
int is_plyr_airborn();
void swap_active_plyr_proc();
void become_plyr1_proc();
void become_plyr2_proc();
void snd_stop();
void scale_me_normal();
void release_other_player();
void xfer_player_proc();
void xfer_player_proc_to_script();
void init_ground_move_no_aniproc();
void init_3d_move_no_aniproc();
int check_damage_valid_fc();
float trial_damage_callback();
int drone_ai_check_block_at_reactions();
void drone_ai_hit();
void drone_ai_reset_ai_cmd();
int drone_ai_check_combo_breaker();
void enable_bgnd_obj_repel();
void exit_plyr_proc();
int my_joypad_state_5(void);
int check_switch();
void stop_prison_grab_proc(void);
float p_glitch_to_stance(void);
void p_animate_weapon_rest(void);
CmdScript* get_cmdscript_for_proc(MkProc* proc);
float r_call_script_function(void);
float r_call_player_char_script_function(void);
static float r_call_other_player_char_script_function(void);
void run_reaction_cleanup_function(PlyrPdata* player);
void plyr_spawn_anim(void* animation);

extern int f_fatality_was_done;
extern int g_drone_blocking_in_reaction;
extern int g_drone_faked_out;
extern int mode_of_play;
static float r_complete_ermac_slam(void);
static float r_face3_onback(void);
void set_ani_speed(float speed);
void set_anim_hiframe(float frame);
void special_move_cam_setup(
    int mode, int ticks, int flags, float x, float y, float z,
    float distance, float speed);
void update_bone_hierarchy(MkHdr* object);
void ground_me(MkHdr* object);
void wall_eligible_on(void);
void wall_eligible_off(void);
void blend_to_ani_frame(
    int animation, int transition, float blend, float frame);
float j_getup_back_6(void);
float j_getup_front_12(void);
float blend_to_stance_j_exit(void);
float j_getup_back_3(void);
float j_getup_back_9(void);
float j_getup_back_12(void);
float j_getup_sit_12(void);
float j_exit_blend_stance(void);
float j_stay_down_dead(void);
void ani_to_fall_to_frame(
    int sound_id, float landing_frame, float target_frame);
void back_rollup_check(void);
void danger_zone_eligible_on(void);
void face_bleed_me(int size);
void init_air_move_no_aniproc(void);
void land_chores(
    int land_sound, int second_sound,
    float shake_ticks, float shake_strength);
void launch_n_land_ani(
    void* animation, int flags, float x, float z, float frame,
    float speed, float gravity, float blend);
void myvel_his_angle_y(float y, float x, float z);
void newani_to_frame_x(
    void* animation, int flags, float frame, float x, float z, float blend);
void player_feet_land_chores(void);
void shake_camera(int ticks, float strength);
void shake_hit_voice(
    int shake_ticks, int hit_voice, int fighter_voice, float rumble_scale);
void start_blood_particles(
    int script, int bone, PlyrPdata* player, MkObj* object);
int stay_down_check(void);
void tightrope_restrictions_off(void);
void tightrope_restrictions_on(void);
extern void (*large_ground_fx)(void);
void ani_1_frame(void);
void ani_loop_more_frames(float frames);
void ani_x_more_frames(float frames);
void blend_to_ani_INOUT(
    int in_animation, int out_animation, float blend_rate,
    float in_speed, float out_speed);
void disable_blocking(void);
void enable_all_my_blocking(void);
int am_i_blocking(void);
int am_i_duck_blocking(void);
int should_i_weapon_block(void);
int get_his_attack_counter(void);
int am_i_airborn_check_in_reaction(void);
int local_collision_allowed(PlyrPdata* player);
void destroy_subzero_decoy(void);
void ejb_call(int command);
float fpick_a_float(float normal, float flipped_value);
int is_he_airborn(void);
void launch_me_up(float vertical_velocity, float gravity);
void myvel_my_angle_y(float angle, float x_velocity, float z_velocity);
void bulvan_function(int enabled);
void myvel_his_angle_y_inout(float y, float x, float z);
int my_joypad_state_5(void);
void* start_blood_particles_scripts(int script, int bone);
void wait_to_land(void);
float xz_distance_between_players(void);
float p_joy_loop(void);
float wall_dodge(void);
static float r_counter_caught_abort(void);
static void same_xz(void);
static void r_top_of_head_slam(void);
MslSoundHandle snd_req(int sound_id);
int emitter_id_from_handle(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
void fx_set_param_v3(
    unsigned int effect, int parameter, float x, float y, float z);
void fx_resume_emit(unsigned int effect);
static float p_image_fader(void);
MkProc* _create_mkproc_generic_nostack(
    int proc_id, int priority, MkProcEntryFn proc_fn,
    int pdata_size, MkHdr** pdata_out);
void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
typedef struct ReactionScreenPos {
    float x;
    float y;
} ReactionScreenPos;

void camera_get_screen_pos_from_world_pos(
    const Vec* world, ReactionScreenPos* screen);

#include "src/game/reactions_table_prototypes.inc"
#include "src/game/reactions_table.inc"

/*
 * Soft ceiling: run_reaction_cleanup_function ~95.42% -- nonvolatile
 * coloring, one uncoalesced latch result, and final guard polarity.
 */
void run_reaction_cleanup_function(PlyrPdata* player) {
    if (player != 0 &&
        ((ReactionStatusFlagsView*)player->status_flags)
            ->cleanup_function != 0) {
        PlyrPdata* saved_player;
        PlyrPdata* saved_opponent;
        MkObj* saved_object;
        MkObj* saved_opponent_object;
        MkObj* object;
        MkObj* opponent_object;

        saved_player = plyr_pdata;
        saved_opponent = his_pdata;
        plyr_pdata = player;
        saved_object = plyr_obj;
        saved_opponent_object = his_obj;
        his_pdata = player->his_plyr_pdata;
        object = player->tracked_obj;
        object = object != 0
            ? (object->hdr.instance == player->tracked_obj_instance
                ? object : 0)
            : 0;
        plyr_obj = object;
        opponent_object = player->his_plyr_pdata->tracked_obj;
        opponent_object = opponent_object != 0
            ? (opponent_object->hdr.instance ==
                    player->his_plyr_pdata->tracked_obj_instance
                ? opponent_object : 0)
            : 0;
        his_obj = opponent_object;
        if (object != 0) {
            CmdScript* saved_script;

            if (opponent_object == 0) {
                return;
            }
            saved_script = active_cmdscript;
            active_cmdscript = &global_script_interpreter;
            cmdscript_set_parameters(
                &global_script_interpreter, 1, player);
            cmdscript_setup_execution(
                player->cmo,
                ((ReactionStatusFlagsView*)player->status_flags)
                    ->cleanup_function);
            cmdscript_execute(player->cmo);
            active_cmdscript = saved_script;
            plyr_pdata = saved_player;
            his_pdata = saved_opponent;
            plyr_obj = saved_object;
            his_obj = saved_opponent_object;
        }
    }
}

/* Soft ceiling: p_image_fader ~99.49% -- r3/r4 scratch roles in the destroy tail. */
static float p_image_fader(void) {
    ReactionImageFaderPdata* pdata;
    ScreenObj* object;

    pdata = (ReactionImageFaderPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->delay > 0) {
        object = pdata->object;
        if (object != 0) {
            object = (object->instance == pdata->object_instance)
                ? object : 0;
        } else {
            object = 0;
        }
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
        if (object != 0) {
            object = (object->instance == pdata->object_instance)
                ? object : 0;
        } else {
            object = 0;
        }
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, (unsigned char)pdata->alpha);
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
    if (object != 0) {
        object = (object->instance == pdata->object_instance)
            ? object : 0;
    } else {
        object = 0;
    }
    if (object != 0 && (unsigned int)object->instance != 0) {
        object->vtbl->destroy();
    }
    return -1.0f;
}

ScreenObj* display_image_by_plyr(
    int slot, const char* image_name,
    ReactionImageSource* source, float y_offset) {
    ReactionImageFaderPdata* fader;
    ScreenObj* image;
    int half_width;
    Vec bone_offset = {0.0f, 0.0f, 0.0f};
    Vec world_position;
    ReactionScreenPos screen_position;

    image = load_named_2d_pfxobj(
        slot, 0xC021, image_name, 0, 0x2F);
    get_bone_offset_world_pos(
        source->object, 9, &bone_offset, &world_position);
    world_position.y = y_offset + g_game_info.field_34;
    camera_get_screen_pos_from_world_pos(
        &world_position, &screen_position);
    half_width = image->pfx2d->tex_w / 2;
    image->x = (int)screen_position.x - half_width;
    image->y = (int)screen_position.y;

    if (_create_mkproc_generic_nostack(
            0xC02A, 0x1F, p_image_fader,
            sizeof(ReactionImageFaderPdata),
            (MkHdr**)&fader) != 0) {
        fader->object = image;
        fader->object_instance = image->instance;
        fader->delay = 60;
        fader->alpha = 0xFF;
        fader->direction = source->hdr.instance;
    }
    return image;
}

/*
 * Soft ceiling: the five hit-flash helpers are opcode-identical apart from
 * the object/effect nonvolatile pair coloring (r30/r31 swapped).
 */
void flash_hit_at_bid_with_y(float y_offset) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    object = plyr_obj;
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

void flash_hit_at_bid(int bone) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    object = plyr_obj;
    if (plyr_pdata->plyr_num == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    get_bone_world_pos(object, bone, &position);
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

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

    (void)script_args;
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

    (void)script_args;
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

/* Soft ceiling: fight_fx_blades_clash ~96.80% -- nonvolatile assignment order only. */
void fight_fx_blades_clash(PlyrPdata* player) {
    ReactionBladeFighterDefinition* fighter;
    ReactionBladeTransform* transform;
    MkObj* blade;
    MkObj* object;
    MkPfx* particle;
    unsigned int effect;
    int bone;
    int player_num;

    fighter =
        (ReactionBladeFighterDefinition*)player->fighter_definition;
    player_num = player->plyr_num;
    bone = 0;
    object = fighter->blade_object;
    if (object != 0) {
        object = (object->hdr.instance == fighter->blade_object_instance)
            ? object : 0;
    } else {
        object = 0;
    }
    blade = object;
    if (blade == 0 || blade->hide_flag_bits.hidden == 1) {
        bone = 0x1C;
        blade = player->plyr_info->slot.mirror_a;
    }
    if (player_num == 0) {
        effect = fx_by_owner("blade_flash", 1);
    } else {
        effect = fx_by_owner("blade_flash", 2);
    }
    effect = fx_next_emitter(effect);
    transform = ((ReactionBladeFighterDefinition*)
        player->fighter_definition)->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }

    if (player_num == 0) {
        effect = fx_by_owner("blade_sparks", 1);
    } else {
        effect = fx_by_owner("blade_sparks", 2);
    }
    effect = fx_next_emitter(effect);
    transform = ((ReactionBladeFighterDefinition*)
        player->fighter_definition)->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }

    if (player_num == 0) {
        effect = fx_by_owner("blade_bouncy_sparks", 1);
    } else {
        effect = fx_by_owner("blade_bouncy_sparks", 2);
    }
    effect = fx_next_emitter(effect);
    transform = ((ReactionBladeFighterDefinition*)
        player->fighter_definition)->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }
}

/*
 * Soft ceiling: reaction_xfer_him ~91.53% -- whole-function nonvolatile
 * register permutation (r14-r31 numbering); memory operations match.
 */
int reaction_xfer_him(int reaction, float damage_scale, int block_type) {
    ReactionTransferPdata* transfer;
    ReactionDamagePdata* boost_source;
    ReactionFighterDefinitionDispatchView* fighter;
    ReactionPlyrInfoCombatView* combat_info;
    ReactionDispatchContext dispatch;
    LoadableReactionScript* loadable;
    ReactionStatusFlagsView* cleanup_status;
    CmdScript* cmdscript;
    CmdScript* saved_cmdscript;
    MkProc* opponent_proc;
    MkProc* hold_proc;
    PlyrPdata* victim;
    PlyrPdata* saved_player;
    PlyrPdata* saved_opponent;
    MkObj* victim_obj;
    MkObj* saved_object;
    MkObj* saved_opponent_object;
    MkObj* cleanup_object;
    MkObj* cleanup_opponent_object;
    int original_previous_state;
    int state_for_bgnd;
    int reaction_state;
    int original_reaction;
    int dispatch_reaction;
    int blocked;
    int face_after;
    int face_reaction;
    int force_air;
    int big_boss;
    int both_special;
    int input_state;
    float applied_damage;
    float damage;

    reaction_state = tbl_xfer_addresses[reaction].state;
    dispatch_reaction = reaction;
    face_after = 1;
    face_reaction = 0;
    force_air = 0;
    if (((g_game_info.flags >> 3) & 1) != 0 ||
        ((g_game_info.flags >> 4) & 1) != 0 ||
        f_fatality_was_done != 0) {
        return 0;
    }

    transfer = (ReactionTransferPdata*)apdata;
    opponent_proc = transfer->opponent_proc;
    if (opponent_proc != 0) {
        opponent_proc = (opponent_proc->hdr.instance ==
            transfer->opponent_proc_instance)
            ? opponent_proc : 0;
    } else {
        opponent_proc = 0;
    }
    victim_obj = transfer->opponent_obj;
    victim = transfer->opponent_pdata;
    cmdscript = get_cmdscript_for_proc(opponent_proc);

    big_boss = is_big_boss(victim);
    if (big_boss != 0) {
        dispatch_reaction = big_boss_reaction_remap(dispatch_reaction);
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
    original_reaction = dispatch_reaction;

    victim->hit_flash_enabled = 0;
    victim->throw_restriction = 0;
    victim_obj->flags_09_bits.tightrope_restricted = 1;
    victim_obj->flags_09_bits.face_opponent = 0;
    victim->block_requirement = block_type;
    if (is_plyr_airborn(victim_obj, victim, 1, 0) == 1) {
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
            !(tbl_xfer_addresses[original_reaction].flags & 0x10)) {
            force_air = 1;
            dispatch_reaction = 0xF1;
        } else if (!(tbl_xfer_addresses[original_reaction].flags & 0x10)) {
            force_air = 1;
            dispatch_reaction = 0xF1;
        }
    }

    switch (aproc->pid) {
    case 0x501D:
    case 0x2026:
    case 0x5019:
        if (victim_obj == g_game_info.plyr1.slot.mirror_a) {
            become_plyr2_proc();
        } else {
            become_plyr1_proc();
        }
        break;
    case 0xB010:
        break;
    default:
        swap_active_plyr_proc();
        break;
    }

    if (plyr_pdata->scream_sound_handle != 0) {
        snd_stop(plyr_pdata->scream_sound_handle);
        plyr_pdata->scream_sound_handle = 0;
    }
    if (plyr_pdata != 0 &&
        ((ReactionStatusFlagsView*)plyr_pdata->status_flags)
            ->cleanup_function != 0) {
        cleanup_status = (ReactionStatusFlagsView*)plyr_pdata->status_flags;
        saved_player = plyr_pdata;
        saved_opponent = his_pdata;
        saved_object = plyr_obj;
        saved_opponent_object = his_obj;
        his_pdata = saved_player->his_plyr_pdata;
        cleanup_object = saved_player->tracked_obj;
        if (cleanup_object != 0 &&
            cleanup_object->hdr.instance !=
                saved_player->tracked_obj_instance) {
            cleanup_object = 0;
        }
        plyr_obj = cleanup_object;
        cleanup_opponent_object = his_pdata->tracked_obj;
        if (cleanup_opponent_object != 0 &&
            cleanup_opponent_object->hdr.instance !=
                his_pdata->tracked_obj_instance) {
            cleanup_opponent_object = 0;
        }
        his_obj = cleanup_opponent_object;
        if (cleanup_object != 0 && cleanup_opponent_object != 0) {
            saved_cmdscript = active_cmdscript;
            active_cmdscript = &global_script_interpreter;
            cmdscript_set_parameters(
                &global_script_interpreter, 1, saved_player);
            cmdscript_setup_execution(
                saved_player->cmo, cleanup_status->cleanup_function);
            cmdscript_execute(saved_player->cmo);
            active_cmdscript = saved_cmdscript;
            plyr_pdata = saved_player;
            his_pdata = saved_opponent;
            plyr_obj = saved_object;
            his_obj = saved_opponent_object;
        }
    }
    plyr_pdata->duck_reaction_active = 0;
    plyr_pdata->his_plyr_pdata->duck_reaction_active = 0;
    scale_me_normal(plyr_pdata->his_plyr_pdata);
    fighter = (ReactionFighterDefinitionDispatchView*)
        plyr_pdata->fighter_definition;
    if (fighter->weapon_rest_animation != 0) {
        plyr_spawn_anim(p_animate_weapon_rest);
    }

    original_previous_state = plyr_pdata->previous_state;
    dispatch.saved_state = plyr_pdata->state;
    plyr_obj->flags_09_bits.wall_restricted = 0;
    hold_proc = plyr_pdata->hold_proc;
    if (hold_proc != 0) {
        if (hold_proc->hdr.instance != plyr_pdata->hold_proc_instance) {
            hold_proc = 0;
        }
    } else {
        hold_proc = 0;
    }
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
    if (tbl_xfer_addresses[original_reaction].flags & 8) {
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
            (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
        damage = applied_damage * 0.8f;
        if (boost_source->damage_boost_until >
            (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
            ->accumulated_damage += damage;
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
            (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
        damage = applied_damage * 0.8f;
        if (boost_source->damage_boost_until >
            (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p2_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
            ->accumulated_damage += damage;
        if (g_game_info.plyr1.field_0C == 0.0f) {
            blocked = 0;
        }
    }
    g_drone_faked_out = 0;
    if (blocked == 0 && face_after == 0) {
        face_after = 1;
    }

    if (blocked != 0) {
        dispatch_reaction = 0xF1;
        plyr_pdata->his_plyr_pdata->collision_result = 2;
        if (tbl_xfer_addresses[original_reaction].power_level == 0) {
            dispatch_reaction = 0xF2;
        }
        if (tbl_xfer_addresses[original_reaction].power_level == 3) {
            dispatch_reaction = 0xF4;
        }
        if (tbl_xfer_addresses[original_reaction].power_level == 4 ||
            tbl_xfer_addresses[original_reaction].power_level == 5) {
            dispatch_reaction = 0xF5;
        }
        if (tbl_xfer_addresses[original_reaction].power_level == 0x64) {
            dispatch_reaction = 0xF6;
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
        combat_info = (ReactionPlyrInfoCombatView*)&g_game_info.plyr1;
        if (plyr_pdata->plyr_num == 0) {
            combat_info = (ReactionPlyrInfoCombatView*)&g_game_info.plyr0;
        }
        if ((combat_info->combat_flags & 0x20) != 0 ||
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
        (tbl_xfer_addresses[original_reaction].flags & 2)) {
        init_air_move_no_aniproc();
    } else {
        if (tbl_xfer_addresses[original_reaction].flags & 1) {
            init_ground_move_no_aniproc();
        }
        if (tbl_xfer_addresses[original_reaction].flags & 4) {
            init_3d_move_no_aniproc();
        }
    }
    if (g_game_info.plyr0.slot.pdata->state != 0x4203 &&
        g_game_info.plyr1.slot.pdata->state != 0x4203) {
        both_special = 0;
    } else {
        both_special = 1;
    }
    plyr_pdata->state = dispatch.saved_state;
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
    if (((check_switch(plyr_pdata->controller_port, 1, plyr_pdata) != 0 &&
          input_state == 3) ||
        (plyr_pdata->drone_request != 0 &&
          drone_ai_check_combo_breaker() != 0)) &&
        g_game_info.flag_bits.lens_flare_enabled &&
        (tbl_xfer_addresses[original_reaction].flags & 0x200)) {
        if (victim->breaker_strength > 0) {
            dispatch_reaction = 0x78;
            victim->breaker_strength--;
        }
    }

    if (aproc->pid == 0x1001 || aproc->pid == 0x1002) {
        swap_active_plyr_proc();
    } else {
        exit_plyr_proc();
    }

    dispatch.transfer = tbl_xfer_addresses[dispatch_reaction].dispatch;
    if (opponent_proc != 0) {
        if (face_reaction != 0 && plyr_obj != 0) {
            face_opponent_now();
        }
        if ((unsigned int)state_for_bgnd == 0x4210U) {
            enable_bgnd_obj_repel(
                ((ReactionPdataRepelView*)victim)->bgnd_repel_id);
        }
        if (blocked != 0) {
            bgnd_clear_danger_zone_callback(victim);
        }
        if (mode_of_play != 6) {
            bgnd_rx_notify(
                victim->plyr_info, dispatch_reaction,
                tbl_xfer_addresses[dispatch_reaction].power_level,
                tbl_xfer_addresses[dispatch_reaction].flags);
        }
        if (!(tbl_xfer_addresses[dispatch_reaction].flags & 0x100) ||
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
            dispatch.transfer =
                tbl_xfer_addresses[dispatch_reaction].dispatch;
        }
        if (dispatch_reaction >= 0xE6 && dispatch_reaction <= 0xED) {
            loadable =
                &g_loadable_reaction_scripts[dispatch_reaction - 0xE6];
            dispatch.transfer.call_type = loadable->slot_count;
            dispatch.transfer.entry = (ReactionEntry)loadable->script;
        }
        switch (dispatch.transfer.call_type) {
        case 4:
            cmdscript->unk28 = (unsigned int)dispatch.transfer.entry;
            xfer_player_proc(opponent_proc, r_call_script_function);
            break;
        case 3:
            if ((unsigned int)dispatch.transfer.entry == 0x39 &&
                victim->character_id == 0x1B) {
                cmdscript->unk28 = (unsigned int)dispatch.transfer.entry;
                xfer_player_proc(
                    opponent_proc, r_call_player_char_script_function);
            } else {
                cmdscript->unk28 = (unsigned int)dispatch.transfer.entry;
                xfer_player_proc(
                    opponent_proc, r_call_other_player_char_script_function);
            }
            break;
        case 5:
            xfer_player_proc_to_script(victim_obj, dispatch.transfer.entry);
            break;
        case 1:
            xfer_player_proc(opponent_proc, dispatch.transfer.entry);
            break;
        case 2:
            cmdscript->unk28 = (unsigned int)dispatch.transfer.entry;
            xfer_player_proc(
                opponent_proc, r_call_player_char_script_function);
            break;
        }
    }
    return face_after;
}

int reaction_fetch_current_flags(int player) {
    int reaction_index;

    reaction_index =
        ((ReactionCurrentPdata*)g_game_info.plyr0.slot.pdata)->reaction_index;
    if (player == 1) {
        reaction_index =
            ((ReactionCurrentPdata*)g_game_info.plyr1.slot.pdata)
                ->reaction_index;
    }
    if (reaction_index == 0xFFFF) {
        return 0;
    }
    return tbl_xfer_addresses[reaction_index].flags;
}

int reaction_fetch_current_power_level(int player) {
    int reaction_index;

    reaction_index =
        ((ReactionCurrentPdata*)g_game_info.plyr0.slot.pdata)->reaction_index;
    if (player == 1) {
        reaction_index =
            ((ReactionCurrentPdata*)g_game_info.plyr1.slot.pdata)
                ->reaction_index;
    }
    if (reaction_index == 0xFFFF) {
        return 2;
    }
    return tbl_xfer_addresses[reaction_index].power_level;
}

void load_script_as_reaction(unsigned int slot, int script) {
    if (slot <= 7U) {
        g_loadable_reaction_scripts[slot].script = script;
    }
}

/* Soft ceiling: reaction_xfer_him_nohit ~99.71% -- pool-label noise only. */
void reaction_xfer_him_nohit(int reaction) {
    int hit_count;

    if (his_pdata->blocking_disabled_2 != 0) {
        hit_count = his_pdata->hit_count;
        his_pdata->hit_count = hit_count - 1;
        reaction_xfer_him(reaction, 0.0f, 2);
    }
}

/* Soft ceiling: r_ZZZZZZZ ~97.50% -- pool-label noise only. */
static float r_ZZZZZZZ(void) {
    return 1.0f;
}

/*
 * Soft ceiling: the five damage leaves are opcode-identical apart from
 * TU-local float-pool labels.
 */
void damage_player(PlyrPdata* source, float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (source->plyr_num == 0) {
        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
            ->accumulated_damage += damage;
        return;
    }

    boost_source = (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)->accumulated_damage +=
        damage;
}

void damage_him(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
            return;
        }

        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
            ->accumulated_damage += damage;
    }
}

void damage_me(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
            return;
        }

        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
        damage = amount;
        damage *= 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p2_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
            ->accumulated_damage += damage;
    }
}

void damage_p2(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    boost_source = (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)->accumulated_damage +=
        damage;
}

void damage_p1(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    boost_source = (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
    damage = amount;
    damage *= 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p1_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)->accumulated_damage +=
        damage;
}

/*
 * Soft ceiling: these three script-call wrappers are opcode-identical to
 * retail; objdiff only distinguishes their TU-local zero-float labels.
 */
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
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    wall_eligible_on();
    got_hit_fx(2, 4, 0, 0xA, 0, 0, 0.025f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(9, 8, 0.1f, 0.9f);
    plyr_anim_pdata->weight = 1.0f;
    plyr_anim_pdata->step = 0.7f;
    glitch_to_ani(shared_ani.chest_stumble, 3);
    set_anim_hiframe(47.0f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_cyrus_stomp(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    disable_both_repel_flags();
    got_hit_fx(2, 5, 1, 0xA, 0, 0, 0.05f);
    blend_to_ani(shared_ani.cyrus_stomp, 3, 0.2f);
    ani_to_frame_x_call(same_xz, 8.0f);
    got_hit_fx(4, 8, 1, 0, 0, 0, 0.0f);
    init_ground_move();
    ani_to_end();
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

/* Retail TU-local; referenced by remaining split reaction code. */
static void same_xz(void) {
    plyr_obj->pos.value.x = his_obj->pos.value.x;
    plyr_obj->pos.value.z = his_obj->pos.value.z;
}

float r_obstacle_falldown(void) {
    ReactionProcVtable* vtable;

    init_air_move_no_aniproc();
    stop_me();
    face_opponent_now();
    danger_zone_eligible_on();
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    myvel_his_angle_y(0.0f, 0.045f, 0.045f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.004f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_complete_ermac_slam(void) {
    ReactionProcVtable* vtable;
    ReactionDamagePdata* boost_source;
    float damage;

    _mkproc_sleep_ticks = 50.0f * inverse_game_speed;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
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
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    adjust_my_damage_multiplier(0.6f);
    set_my_state(0x3203);
    init_air_move();
    got_hit_fx(4, 9, 1, 0, 0, 2, 0.0f);
    plyr_anim_pdata->step = 0.6f;
    launch_me_up(0.08f, -0.003f);
    myvel_my_angle_y(3.1428f, -0.04f, -0.04f);
    ani_to_frame_x(34.0f);
    set_my_state(0x600);
    got_hit_fx(4, 9, 1, 0, 0, 2, 0.0f);
    random_hit(9);
    bulvan_function(0);
    init_ground_move();
    stop_me();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_shujinko_slam(void) {
    ReactionProcVtable* vtable;

    got_hit_fx(2, 0xD, 4, 0, 0, 2, 0.0f);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation_a, 0, 0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

static float r_ermac_slam(void) {
    ReactionProcVtable* vtable;

    got_hit_fx(2, 0xD, 4, 0, 0, 2, 0.0f);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

static float r_fan_lift(void) {
    ReactionProcVtable* vtable;

    adjust_my_damage_multiplier(0.6f);
    face_opponent_now();
    set_my_state(0x4206);
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_pdata->blocking_disabled = 1;
    random_voice(5);
    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    force_away(8, 8, 0.1f, 0.85f);
    _mkproc_sleep_ticks = 30.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    force_away(0x64, 0xA, -0.03f, 0.95f);
    _mkproc_sleep_ticks = 120.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float j_counter_caught(void) {
    ReactionProcVtable* vtable;
    int reaction;

    init_ground_move_no_aniproc();
    stop_me();
    random_voice(9);
    shake_hit_voice(2, 6, 5, 0.02f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    while (his_pdata->state == 0x4000) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
    }
    _mkproc_sleep_ticks = 15.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_counter_caught_abort, 0.0f);
    return 0.0f;
}

static float r_counter_caught_abort(void) {
    ReactionProcVtable* vtable;

    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    blend_to_fstance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_counter_catch_med(void) {
    ReactionProcVtable* vtable;

    blend_to_ani(shared_ani.counter_caught, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    set_my_state(0x4202);
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    set_my_state(0x4000);
    _mkproc_sleep_ticks = 70.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    reaction_xfer_him(0xBF, 0.0f, 2);
    plyr_pdata->summon_position_x = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

static float r_post_surf_throw(void) {
    ReactionProcVtable* vtable;
    ReactionPostSurfPdata* player;

    if (stay_down_check() != 0) {
        player = (ReactionPostSurfPdata*)plyr_pdata;
        player->stay_down = 1;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_stay_down_dead, 0.0f);
        return 0.0f;
    }
    force_away(0xA, 0x10, 0.1f, 0.9f);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    tightrope_restrictions_on();
    glitch_to_ani(shared_ani.post_surf_getup, 3);
    plyr_anim_pdata->step = 1.2f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_projectile ~99.31% -- pool-label noise only. */
static float r_block_hit_projectile(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(5, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: r_combo_broken_part2 ~96.53% -- stack-slot assignment order
 * for the Vec locals and template-copy scheduling.
 */
static float r_combo_broken_part2(void) {
    ReactionProcVtable* vtable;
    ReactionImageFaderPdata* fader;
    ReactionImageSource* source;
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

    {
        Vec world_position;
        Vec bone_offset = {0.0f, 0.0f, 0.0f};
        ReactionScreenPos screen_position;

        source = (ReactionImageSource*)plyr_pdata->plyr_info;
        image = load_named_2d_pfxobj(
            0x10005, 0xC021, "BREAKER", 0, 0x2F);
        get_bone_offset_world_pos(
            source->object, 9, &bone_offset, &world_position);
        world_position.y = 1.7f + g_game_info.field_34;
        camera_get_screen_pos_from_world_pos(
            &world_position, &screen_position);
        half_width = image->pfx2d->tex_w / 2;
        image->x = (int)screen_position.x - half_width;
        image->y = (int)screen_position.y;
    }

    if (_create_mkproc_generic_nostack(
            0xC02A, 0x1F, p_image_fader,
            sizeof(ReactionImageFaderPdata),
            (MkHdr**)&fader) != 0) {
        fader->object = image;
        fader->object_instance = image->instance;
        fader->delay = 60;
        fader->alpha = 0xFF;
        fader->direction = source->hdr.instance;
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
    got_hit_fx(0, 2, 4, 1, 0, 0, 0.0f);
    force_away(0x14, 8, 0.025f, 0.9f);
    blend_to_ani(shared_ani.combo_broken_launch, 3, 0.5f);
    set_ani_speed(0.6f);
    ani_to_frame_x(13.0f);
    got_hit_fx(4, 8, 0, 0, 0, 0, 0.0f);
    back_rollup_check();
    ani_x_more_frames(5.0f);
    blend_to_ani(shared_ani.combo_broken_recover, 3, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_combo_broken_part1(void) {
    ReactionProcVtable* vtable;

    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    _mkproc_sleep_ticks = 40.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_combo_breaker(void) {
    ReactionProcVtable* vtable;

    trial_increment_state_value(plyr_pdata->plyr_num, 0x20, 0);
    adjust_my_damage_multiplier(0.75f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_anim_pdata->weight = 1.3f;
    plyr_anim_pdata->step = 0.7f;
    face_opponent_now();
    got_hit_fx(2, 5, 2, 0, 0, 0, 0.0f);
    reaction_xfer_him(0x79, 0.0f, 2);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    plyr_obj->flags_09_bits.bit6 = 1;
    blend_to_ani_frame(shared_ani.combo_breaker, 3, 0.33f, 7.0f);
    set_ani_speed(1.0f);
    ani_to_frame_x(11.0f);
    reaction_xfer_him(0x7A, 0.0f, 2);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    set_ani_speed(0.5f);
    ani_to_frame_x(25.0f);
    ani_to_blend_frame(3.0f);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p5 ~99.31% -- pool-label noise only. */
static float r_block_hit_p5(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xB, 5, 0, 0, 0);
    force_away(3, 8, 0.2f, 0.85f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p3 ~99.31% -- pool-label noise only. */
static float r_block_hit_p3(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p1 ~99.35% -- pool-label noise only. */
static float r_block_hit_p1(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 4, 0.15f, 0.5f);
    disable_my_attacks(6);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p0 ~99.35% -- pool-label noise only. */
static float r_block_hit_p0(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    disable_my_attacks(6);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

static float j_block_common_reaction(void) {
    ReactionProcVtable* vtable;

    if (!(plyr_pdata->previous_state & 0x800) &&
        plyr_pdata->drone_request == 1) {
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(x_block, 0.0f);
        return 0.0f;
    }
    if (plyr_pdata->previous_state & 0x100) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x12, 0);
        trial_increment_state_value(plyr_pdata->plyr_num, 0x13, 0);
        init_ground_move();
        set_my_state(0xF00);
        force_away(2, 8, 0.15f, 0.85f);
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
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_duck_block_loop, 0.0f);
        return 0.0f;
    }
    if (am_i_blocking() != 0) {
        if (plyr_pdata->state & 0x100) {
            plyr_pdata->his_attack_counter = get_his_attack_counter();
            set_my_state(0xA00);
            blend_to_ani(shared_ani.standing_block_a, 0, 0.1f);
        }
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_block_loop, 0.0f);
        return 0.0f;
    }
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_nightwolf_lightning(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 1, 0.05f);
    random_voice(0x13);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_mileena_hit(void) {
    ReactionProcVtable* vtable;
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_face3_onback, 0.0f);
    return 0.0f;
}

static float r_nightwolf_charge(void) {
    ReactionProcVtable* vtable;

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
        got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_face3_onback(void) {
    ReactionProcVtable* vtable;

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
        got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_cyrax_blade ~99.87% -- fmuls scratch-FPR selection only. */
static float r_cyrax_blade(void) {
    ReactionProcVtable* vtable;
    float angle;
    float sine;
    float cosine;

    angle = 0.000005992112f *
        (float)(((int)(166886.1f * his_obj->ang.y)) & 0xFFFFF);
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
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        snd_req(0xD81);
        _mkproc_sleep_ticks = 9.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        snd_req(0xD81);
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    init_ground_move();
    face_bleed_me(1);
    face_opponent_now();
    snd_req(0xD81);
    force_away(3, 8, 0.2f, 0.8f);
    blend_to_ani(shared_ani.cyrax_blade, 3, 0.5f);
    set_ani_speed(0.5f);
    ani_to_frame_x(20.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_head3_onback(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 0, 0, 3, 0, 3, 0.05f);
    r_top_of_head_slam();
    blend_to_ani(shared_ani.head_slam_fall, 3, 0.2f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(27.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    got_hit_fx(4, 8, 1, 0, 0, 0, 0.0f);
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static void r_top_of_head_slam(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.05f);
    random_hit(3);
    blend_to_ani(shared_ani.top_of_head_slam, 3, 0.5f);
    plyr_anim_pdata->step = 1.5f;
    ani_to_frame_x(9.0f);
    plyr_anim_pdata->step = 2.5f;
    ani_to_blend_frame(4.0f);
}

float r_jump_slambounce_final_hit(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    special_move_cam_setup(
        0xA, 0x3C, 0, 1.47f, 4.1f, 1.0f, -1.75f, -0.15f);
    reaction_xfer_him(0xDB, 0.0f, 2);
    got_hit_fx(0, 2, 1, 1, 0, 0, 0.05f);
    blend_to_ani(shared_ani.jump_slambounce, 3, 0.2f);
    set_ani_speed(0.7f);
    ani_to_frame_x(6.0f);
    init_ground_move();
    ani_to_frame_x(50.0f);
    got_hit_fx(4, 0, 1, 0, 0, 1, 0.0f);
    ani_to_end();
    check_for_combo_message();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_jump_chin3_final_hit(void) {
    ReactionProcVtable* vtable;

    special_move_cam_setup(
        0xA, 0x3C, 0, 1.47f, 4.1f, 1.0f, -1.75f, -0.15f);
    reaction_xfer_him(0xDC, 0.0f, 2);
    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->pos.value.y = his_obj->pos.value.y;
    stop_me();
    init_air_move();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(2, 4, 3, 0.02f);
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
    got_hit_fx(4, 0, 1, 0, 0, 1, 0.0f);
    set_my_state(0x600);
    back_rollup_check();
    ani_to_frame_x(60.0f);
    set_ani_speed(2.0f);
    init_ground_move();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_slamdown_final_hitter(void) {
    ReactionProcVtable* vtable;

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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_popup_final_hitter(void) {
    ReactionProcVtable* vtable;
    int ticks;

    head_tracking_on();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    if (xz_distance_between_players() < 1.0f) {
        force_away(9, 8, 0.1f, 0.9f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    set_ani_speed(0.33f);
    _mkproc_sleep_ticks = 30.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_fstance(0.05f);
    ticks = 0x28;
    while (is_he_airborn() == 1 && ticks > 0) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        ticks--;
    }
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_enough_air_already(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(3);
    shake_hit_voice(2, 1, 0xD, 0.02f);
    random_hit(4);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.enough_air, 0xCA1, 0.0f, 0.0f, 28.0f,
        0.05f, -0.008f, 0.33f);
    shake_hit_voice(2, 9, 8, 0.02f);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

static float r_airborn_small_lift(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(0, 0, 4, 0.0f);
    if (plyr_pdata->reaction_hit_count == 2) {
        plyr_pdata->f_constrained = 0;
    }
    if ((plyr_pdata->f_constrained == 1) &
        (plyr_pdata->reaction_hit_count >= 3)) {
        reaction_xfer_him(0xFC, 0.0f, 2);
    }
    myvel_his_angle_y(0.0f, 0.06f, 0.06f);
    launch_n_land_ani(
        shared_ani.airborn_small_lift, 0xCA1, 0.0f, 0.0f, 31.0f,
        0.0325f, -0.006f, 0.33f);
    shake_hit_voice(2, 9, 8, 0.02f);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(36.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_hit_airborn1(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(0, 1, 3, 0.0f);
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
        shared_ani.airborn_small_lift, 0xCA1, 0.0f, 0.0f, 31.0f,
        0.08f, -0.006f, 0.33f);
    enable_all_my_blocking();
    shake_hit_voice(2, 9, 8, 0.02f);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(36.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

static float r_corner_repell_ani(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(0x12, 8, 0.15f, 0.9f);
    ani_to_blend_frame(2.0f);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_corner_repell(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(9, 8, 0.1f, 0.9f);
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_sidehead3_dive_opposite(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(2);
    newani_to_frame_x(
        shared_ani.side_head_dive, 2, 22.0f, 1.0f, 1.0f, 0.2f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0, 0, 0.0f, 0.0f);
    shake_hit_voice(2, 9, 7, 0.02f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

static float r_sidehead3_dive(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    face_bleed_me(2);
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    newani_to_frame_x(
        shared_ani.side_head_dive, 1, 22.0f, 1.0f, 1.0f, 0.2f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xCA1, 0xCA1, 2.0f, 0.02f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_sidehead3_spin ~98.49% -- float-guard branch layout (bne+b vs inverted beq). */
static float r_sidehead3_spin(void) {
    ReactionProcVtable* vtable;
    float flight_ticks;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    myvel_his_angle_y(-0.71f, 0.03f, 0.03f);
    plyr_anim_pdata->step = 0.7f;
    blend_to_ani(shared_ani.side_head_spin, 3, 0.33f);
    launch_me_up(0.06f, -0.003f);
    flight_ticks = 2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (flight_ticks >= 0.0f) {
        /* Flight time is already positive. */
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_joy_loop, 0.0f);
    return 0.0f;
}

static float r_feet3_sweptout_rev(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_swept_in(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_swept_out(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet3_sweptin_rev(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    tightrope_restrictions_off();
    blend_to_ani_INOUT(
        shared_ani.swept_reverse, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

static float r_feet1a(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0x10, 0.0f);
    force_away(3, 8, 0.1f, 0.9f);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_feet1_stay_close(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0x10, 0.0f);
    force_away(3, 8, 0.04f, 0.9f);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_gut3_onfeet_hard(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_gut3_onfeet(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    plyr_obj->flags_09_bits.launched = 0;
    blend_to_ani(shared_ani.gut_on_feet, 3, 0.33f);
    plyr_anim_pdata->step = 0.85f;
    ani_to_frame_x(24.0f);
    init_ground_move();
    blend_to_stance(0.04f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_gut3_onbutt(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    plyr_obj->flags_09_bits.launched = 0;
    force_away(3, 8, 0.1f, 0.9f);
    blend_to_ani(shared_ani.gut_on_butt, 3, 0.33f);
    ani_to_frame_x(18.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    ani_to_fall_to_frame(0xD7F, 24.0f, plyr_anim_pdata->high_frame);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_sit_12, 0.0f);
    return 0.0f;
}

static float r_jax_piston_hi(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 1, 5, 4, 0, 0, 0.025f);
    blend_to_ani(shared_ani.jax_piston_high, 3, 0.33f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_jax_piston_lo(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 5, 0, 0, 0, 0.0f);
    adjust_my_damage_multiplier(0.75f);
    blend_to_ani(shared_ani.jax_piston_low, 3, 0.2f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_frame_x(20.0f);
    plyr_pdata->summon_position_x = 15.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_chest2_stumble_shake ~99.31% -- pool-label noise only. */
static float r_chest2_stumble_shake(void) {
    ReactionProcVtable* vtable;

    adjust_my_damage_multiplier(0.75f);
    face_opponent_now();
    got_hit_fx(2, 2, 1, 0xB, 0, 0, 0.0f);
    random_hit(5);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_chest2_stumble ~99.42% -- pool-label noise only. */
float r_chest2_stumble(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0, 0.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}

static float chest_stumble_both(void) {
    ReactionProcVtable* vtable;

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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_esp1_B(void) {
    ReactionProcVtable* vtable;
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
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
    }
    ani_loop_more_frames(7.0f);
    blend_to_ani(his_pdata->reaction_animation_c, 3, 0.1f);
    ani_to_frame_x(19.0f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            damage = 0.14f;
            damage *= 0.8f;
            if (((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                    ->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= ((ReactionDamagePdata*)
                    g_game_info.plyr1.slot.pdata)->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            damage = 0.14f;
            damage *= 0.8f;
            if (((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                    ->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= ((ReactionDamagePdata*)
                    g_game_info.plyr0.slot.pdata)->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

static float r_esp1_A(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 2, 0.0f);
    blend_to_ani(his_pdata->esp1_reaction_animation, 3, 0.1f);
    plyr_obj->gravity = -0.0075f;
    plyr_anim_pdata->step = 0.8f;
    ani_to_blend_frame(10.0f);
    plyr_pdata->summon_position_x = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
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
    ((ReactionProcVtable*)aproc->vtbl)
        ->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_subzero_iceball ~98.85% -- scratch register naming in the demo-flag select. */
static float r_subzero_iceball(void) {
    ReactionProcVtable* vtable;
    int his_character;
    int my_character;
    int collision;

    medium_flash_check();
    destroy_subzero_decoy();
    init_air_move_no_aniproc();
    collision = local_collision_allowed(plyr_pdata);
    if (collision != 0 &&
        (plyr_pdata->previous_state == 0xC600 ||
         plyr_pdata->previous_state == 0xC602)) {
        reaction_xfer_him(0xA1, 0.0f, 2);
        blend_to_stance(0.05f);
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_exit, 0.0f);
        return 0.0f;
    }
    if (g_game_info.feature_flags.bits.high_bit == 0 ? 0 : collision) {
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(blend_to_stance_j_exit, 0.0f);
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
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        plyr_obj->gravity = -0.01f;
        wait_to_land();
        unfreeze_player();
        plyr_pdata->blocking_disabled = 0;
        plyr_pdata->blocking_disabled_2 = 0;
        set_my_state(0);
        got_hit_fx(4, 8, 1, 0, 0, 0, 0.0f);
        face_opponent_now();
        plyr_pdata->summon_position_x = plyr_obj->pos.value.x;
        plyr_pdata->summon_position_z = plyr_obj->pos.value.z;
        glitch_to_ani(shared_ani.falling_back_id, 3);
        plyr_anim_pdata->frame = 23.0f;
        ani_1_frame();
        plyr_obj->pos.value.x = plyr_pdata->summon_position_x;
        plyr_obj->pos.value.z = plyr_pdata->summon_position_z;
        back_rollup_check();
        ani_to_end();
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_getup_back_6, 0.0f);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    unfreeze_player();
    set_my_state(0);
    plyr_pdata->summon_position_x = 25.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: these terminal reaction leaves are opcode-identical to retail;
 * objdiff only distinguishes their TU-local float-pool labels.
 */
static float r_iceball_reversal(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    set_my_state(0xC600);
    plyr_pdata->blocking_disabled = 1;
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_obj->flags_09_bits.face_opponent = 0;
    freeze_player();
    _mkproc_sleep_ticks = 180.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    unfreeze_player();
    set_my_state(0);
    plyr_pdata->summon_position_x = 25.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

static float r_null(void) {
    ReactionProcVtable* vtable;

    _mkproc_sleep_ticks = 8.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

static float r_throw(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    blend_to_ani(shared_ani.throw_fall, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(39.0f);
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    plyr_obj->ang.y += 3.14f;
    set_anim_script(plyr_anim_pdata, shared_ani.throw_getup, 3);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

float r_hit_wall(void) {
    ReactionProcVtable* vtable;
    ReactionDamagePdata* boost_source;
    float damage;

    trial_increment_state_value(plyr_pdata->plyr_num, 0x1C, 1);
    adjust_my_damage_multiplier(0.6f);
    set_my_state(0x601);
    init_air_move_no_aniproc();
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
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
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    plyr_pdata->hit_count++;
    ((ReactionWallPdata*)plyr_pdata)->wall_hit_count++;
    ((ReactionWallPdata*)plyr_pdata->his_plyr_pdata)->wall_hit_count = 0;
    shake_hit_voice(2, 9, 8, 0.02f);
    blend_to_ani(shared_ani.wall_hit, 3, 0.5f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(7.0f);
    plyr_obj->gravity = -0.0075f;
    if (plyr_pdata->drone_request != 0) {
        plyr_pdata->script_exit_value_int = (randu0(2) & 0xFFFF) + 1;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(wall_dodge, 0.0f);
        return 0.0f;
    }
    while (plyr_anim_pdata->frame < 23.0f) {
        plyr_pdata->script_exit_value_int = my_joypad_state_5();
        if (plyr_pdata->script_exit_value_int == 1 ||
            plyr_pdata->script_exit_value_int == 2) {
            vtable = (ReactionProcVtable*)aproc->vtbl;
            vtable->jump_sleep(wall_dodge, 0.0f);
            return 0.0f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
    }
    ani_to_blend_frame(10.0f);
    plyr_obj->flags_09_bits.wall_restricted = 0;
    check_for_combo_message();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float r_scorpion_spear_2(void) {
    ReactionProcVtable* vtable;
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
        vtable = (ReactionProcVtable*)aproc->vtbl;
        ticks++;
        vtable->sleep(vtable);
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
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

static float r_scorpion_spear_1(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    ejb_call(0x31);
    set_my_state(0x603);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    start_blood_particles(0x20, 9, plyr_pdata, plyr_obj);
    blend_to_ani(his_pdata->scorpion_spear_hit, 3, 0.1f);
    ani_to_frame_x(83.0f);
    set_my_state(0x604);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float r_judo_throw1(void) {
    ReactionProcVtable* vtable;
    ReactionFighterDefinitionView* fighter;

    fighter = (ReactionFighterDefinitionView*)his_pdata->fighter_definition;
    glitch_to_ani(fighter->judo_throw_reaction, 3);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

static float r_raiden_shocker_fall(void) {
    ReactionProcVtable* vtable;

    blend_to_ani(his_pdata->reaction_animation, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 60.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    init_ground_move();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}
