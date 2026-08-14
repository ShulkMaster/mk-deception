#include "game/game_info.h"
#include "game/konquest.h"
#include "math/gxVect.h"
#include "msl/msl_types.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"

typedef struct KonquestMissionFightInfo {
    MkHdr hdr;
    int animation_side;
    float current_health;               /* +0x0C */
    float maximum_health;               /* +0x10 */
    char pad14[0x44];
    PlyrPdata* pdata;                    /* +0x58 */
    MkObj* active_object;               /* +0x5C */
    char pad60[4];
    MkProc* process;                    /* +0x64 */
} KonquestMissionFightInfo;

typedef struct KonquestMissionTuneEntry {
    int music;
    char pad04[8];
} KonquestMissionTuneEntry;

typedef struct KonquestMissionTuneTable {
    char pad00[0x14];
    KonquestMissionTuneEntry entries[];
} KonquestMissionTuneTable;

typedef struct KonquestMissionState {
    MkHdr hdr;
    int combo_hits;                    /* +0x008 */
    float combo_damage;                /* +0x00C */
    char pad010[0x180];
    int bleeding_required;             /* +0x190 */
    char pad194[0x11C];
    int display_item_a;                /* +0x2B0 */
    const char* display_format;         /* +0x2B4 */
    unsigned char progress_count;       /* +0x2B8 */
    unsigned char progress_required;    /* +0x2B9 */
    char pad2BA[2];
    int display_item_c;                /* +0x2BC */
    int move_description_flipped;      /* +0x2C0 */
    int move_message;                  /* +0x2C4 */
    int move_message_param;            /* +0x2C8 */
    int trial_type;                    /* +0x2CC */
    int condition_value;               /* +0x2D0 */
    int condition_player;              /* +0x2D4 */
    int condition_mode;                /* +0x2D8 */
    int condition_active;              /* +0x2DC */
    char pad2E0[0x10];
    int next_setup_function;           /* +0x2F0 */
    int winner_end_function;           /* +0x2F4 */
    int loser_end_function;            /* +0x2F8 */
    unsigned int tick_script_function; /* +0x2FC */
    unsigned int display_flags;        /* +0x300 */
    int player_one_switch_state;       /* +0x304 */
    int player_two_switch_state;       /* +0x308 */
    char pad30C[0x14];
    StringObj* progress_string;         /* +0x320 */
    unsigned int progress_string_instance; /* +0x324 */
    StringObj* countdown_string;        /* +0x328 */
    unsigned int countdown_string_instance; /* +0x32C */
    char pad330[0x78];
    MkProc* monk_process;              /* +0x3A8 */
    unsigned int monk_process_instance; /* +0x3AC */
    MkProc* script_process;            /* +0x3B0 */
    unsigned int script_process_instance; /* +0x3B4 */
    MkHdr* monk;                       /* +0x3B8 */
    unsigned int monk_instance;        /* +0x3BC */
    char pad3C0[8];
    KonquestMissionFightInfo* fight;   /* +0x3C8 */
    KonquestMissionFightInfo* drone;   /* +0x3CC */
    int num_rounds;                    /* +0x3D0 */
    int round_timer;                   /* +0x3D4 */
    ScriptSlot* tick_script;           /* +0x3D8 */
    int mission_index;                 /* +0x3DC */
    int player_one_wrapup_state;        /* +0x3E0 */
    int player_two_wrapup_state;        /* +0x3E4 */
    float round_health_restoration;    /* +0x3E8 */
    KonquestMissionTuneTable* tune_table; /* +0x3EC */
    int randomized_side;               /* +0x3F0 */
    char pad3F4[4];
    int restriction_phase;             /* +0x3F8 */
    float player_one_damage_scale;     /* +0x3FC */
    float player_two_damage_scale;     /* +0x400 */
    unsigned int restriction_flags;    /* +0x404 */
} KonquestMissionState;

typedef struct KonquestMissionSaveData {
    char pad00[0x38];
    char fight_name[0x40];             /* +0x38 */
    unsigned int mission_pair_a;       /* +0x78 */
    unsigned int mission_pair_b;       /* +0x7C */
    int next_value_a;                  /* +0x80 */
    int next_value_b;                  /* +0x84 */
    int next_mission;                  /* +0x88 */
    unsigned int background_and_flags; /* +0x8C */
    int passed_last_mission;
} KonquestMissionSaveData;

typedef struct KonquestMissionRegion {
    char pad00[0x24];
    int background_id;                 /* +0x24 */
} KonquestMissionRegion;

typedef struct KonquestMissionPdata {
    char pad00[0x28];
    KonquestMissionRegion* region;      /* +0x28 */
} KonquestMissionPdata;

typedef struct KonquestMissionStateLatch {
    KonquestMissionState* state;
    unsigned int instance;
} KonquestMissionStateLatch;

typedef struct KonquestMissionProcVtable {
    void* reserved[6];
    void (*sleep)(void);
} KonquestMissionProcVtable;

typedef struct KonquestMissionJumpVtable {
    void* reserved[9];
    float (*jump_sleep)(MkProcEntryFn entry, float ticks);
} KonquestMissionJumpVtable;

typedef struct KonquestBloodRushPdata {
    MkHdr hdr;
    int player;
    float rate;
} KonquestBloodRushPdata;

typedef struct KonquestMissionCondition {
    int value;
    int type;
} KonquestMissionCondition;

typedef struct KonquestRequiredMove {
    char pad00[8];
    struct KonquestRequiredMove* next;
    struct KonquestRequiredMove* sentinel;
    int message;
    int message_parameter;
} KonquestRequiredMove;

typedef struct KonquestRequiredMoveProgress {
    unsigned char required;
    unsigned char current;
} KonquestRequiredMoveProgress;

typedef struct KonquestTrialAnimations {
    char pad00[8];
    void* loser_start;                  /* +0x08 */
    char pad0C[8];
    void* loser_loop;                   /* +0x14 */
    char pad18[4];
    void* transform_animation;          /* +0x1C */
} KonquestTrialAnimations;

typedef struct KonquestScreenLatch {
    ScreenObj* object;
    unsigned int instance;
} KonquestScreenLatch;

typedef struct KonquestTrialWindowPdata {
    MkHdr hdr;
    int left;                           /* +0x08 */
    int bottom;                         /* +0x0C */
    int priority;                       /* +0x10 */
    int width;                          /* +0x14 */
    int top;                            /* +0x18 */
    unsigned int prompt_flags;          /* +0x1C */
    char pad20[8];
    int font;                           /* +0x28 */
    char pad2C[0x4BC];
    int visible_item_count;             /* +0x4E8 */
    int art_slot;                       /* +0x4EC */
    KonquestScreenLatch frame[9];       /* +0x4F0 */
    KonquestScreenLatch prompt_ok;      /* +0x538 */
    KonquestScreenLatch prompt_retry;   /* +0x540 */
    char pad548[0x10];
    KonquestScreenLatch prompt_cancel;  /* +0x558 */
} KonquestTrialWindowPdata;

static KonquestMissionState* mission_state;
static KonquestMissionStateLatch mission_state_item;
static AnimPdata* current_anim_pdata;
extern KonquestMissionSaveData konquest_save_data;
extern float _mkproc_sleep_ticks;
extern MslSoundHandle bgnd_music_ptr1;
extern MkObj* plyr_obj;
extern PlyrPdata* plyr_pdata;
extern GameInfo g_game_info;
extern int force_midpoint_calculation_update;
extern int force_bgnd_num;
extern KonquestMissionPdata* konquest_pdata;
extern int mode_of_play;
extern float inverse_game_speed;
extern int screen_width;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern KonquestTrialAnimations bgnd_animations;
extern CameraObj* camera_obj;

void camera_set_animation_mirror_plane(int mode);
void adjust_p1_life(void);
void adjust_p2_life(void);
MslSoundHandle snd_req(int sound);
void snd_stop(MslSoundHandle sound);
void transition_to_anim_script(
    AnimPdata* pdata, int animation, int transition);
int get_konq_profile_value(int category, int index);
unsigned short randu0(int limit, void* context);
void setDroneOverrideSwitch(float* direction);
void fade_to_black(int ticks, int flags);
void gamelogic_jump(int action, MkProcEntryFn entry);
float p_konquest_ending(void);
float p_gamelogic(void);
float j_exit_blend_stance(void);
float j_exit(void);
void set_ani_speed(float speed);
void blend_to_stance(float blend);
static void p_run_special_move(void);
static void call_mission_script(void);
void move_player(MkObj* object, const Vec* position, Vec* angle);
char* strncpy(char* destination, const char* source, unsigned int count);
int sprintf(char* destination, const char* format, ...);
int is_a_to_the_right_of_b(MkObj* a, MkObj* b);
int is_weapon_style(PlyrFighterDefinition* fighter);
MkHdr* konquest_set_dialog_text(
    const char* text, const LipSyncKeyframe* lip_sync_keyframes);
void calc_print_speed_for_nis_dialog(void* dialog, unsigned int ticks);
void xfer_player_proc(MkProc* process, MkProcEntryFn entry);
static void trial_register_attack(
    int player, unsigned char type, unsigned char value);
MkHdr* trial_get_monk(void);
void trial_show_monk(int show);
static void trial_show_text_window(
    int string_id, int style, int flags, float x, float y, float scale);
static float failed_trial_drone_wrapup(void);
static float successful_trial_drone_wrapup(void);
static void trial_increment_state_value(
    int player, int state, int increment);
static void increment_progress_count(unsigned char increment);
static void trial_show_move_message(void);
static void show_background_box(
    int style, int x, int y, int height, int width, int priority);
void become_plyr1_proc(void);
void become_plyr2_proc(void);
void face_opponent_now(void);
void blend_to_ani(void* animation, int transition, float blend);
void ani_to_blend_frame(float frame);
float p_animate(void);
float j_stay_down_dead(void);
void start_mkpfx_FadeSnapShot(void);
void move_plyrs_to_round_start(void);
void start_constrain_proc(void);
void skip_camera_intro(void);
void stop_tunes(void);
void set_camera_destination(float* position, CameraObj* camera);
void set_camera_target_angle(float* angle);
float p_camera_proc(void);
float p_anim_idle(void);
void set_anim_script_frame(
    AnimPdata* animation, void* script, int flags, float frame);
void animpdata_ani_1_frame(AnimPdata* animation);
void update_mkobj(void* object);
void shake_camera(int strength, float duration);
static void start_hero_transform_effect(MkObj* object);
static float p_blood_rush(void);
extern int heart_beat;
MkHdr* start_gusher(
    int* definition, MkObj* fighter, MkObj* mirror, int bone,
    const Vec* velocity, const Vec* acceleration);

static ScreenObj* get_screen_latch(KonquestScreenLatch* latch) {
    ScreenObj* object = latch->object;

    if (object != 0 && object->instance == latch->instance) {
        return object;
    }
    return 0;
}

static ScreenObj* load_prompt(
    KonquestTrialWindowPdata* pdata, KonquestScreenLatch* latch,
    const char* name, int insert) {
    ScreenObj* object = get_screen_latch(latch);

    if (object == 0) {
        object = load_named_2d_pfxobj(
            pdata->art_slot, 0x9004, name, 0,
            pdata->priority - 1);
        if (object != 0) {
            latch->object = object;
            latch->instance = object->instance;
            if (insert) {
                mk_insert((MkHdr*)object, &aproc->pdata_list);
            }
        }
    }
    return object;
}

static void place_frame_piece(
    KonquestScreenLatch* latch, int x, int y,
    float scale_x, float scale_y, int scaled) {
    ScreenObj* object = get_screen_latch(latch);

    if (object == 0) {
        return;
    }
    object->flags &= ~0x10;
    if (scaled) {
        object->flags |= 8;
        object->scale_x = scale_x;
        object->scale_y = scale_y;
    }
    object->x = x;
    object->y = y;
}

void plot_and_show_window_frame(void) {
    KonquestTrialWindowPdata* pdata =
        (KonquestTrialWindowPdata*)apdata;
    int left;
    int right;
    int top;
    int bottom;
    int font_height;
    float width_scale;
    float height_scale;

    if (pdata == 0) {
        return;
    }
    left = pdata->left - 0xF;
    right = pdata->left + pdata->width + 0xF;
    font_height = (int)get_font_height(pdata->font);
    bottom = pdata->bottom + font_height + 3;
    top = (pdata->bottom - pdata->top) - font_height / 2;
    width_scale = (float)((right - left) - 3) * 0.0625f;
    height_scale = (float)((bottom - top) - 3) * 0.0625f;

    place_frame_piece(&pdata->frame[0], left, bottom, 1.0f, 1.0f, 0);
    place_frame_piece(
        &pdata->frame[1], left + 3, bottom,
        width_scale, 1.0f, 1);
    place_frame_piece(&pdata->frame[2], right, bottom, 1.0f, 1.0f, 0);
    place_frame_piece(
        &pdata->frame[3], left, top + 3,
        1.0f, height_scale, 1);
    place_frame_piece(
        &pdata->frame[4], left + 3, top + 3,
        width_scale, height_scale, 1);
    place_frame_piece(
        &pdata->frame[5], right, top + 3,
        1.0f, height_scale, 1);
    place_frame_piece(&pdata->frame[6], left, top, 1.0f, 1.0f, 0);
    place_frame_piece(
        &pdata->frame[7], left + 3, top,
        width_scale, 1.0f, 1);
    place_frame_piece(&pdata->frame[8], right, top, 1.0f, 1.0f, 0);
}

void set_prompt_items(void) {
    KonquestTrialWindowPdata* pdata =
        (KonquestTrialWindowPdata*)apdata;
    ScreenObj* object;
    int y;

    if (pdata == 0) {
        return;
    }
    get_font_height(pdata->font);
    y = (pdata->bottom - pdata->top) - 8;

    if (pdata->prompt_flags & 1) {
        object = load_prompt(
            pdata, &pdata->prompt_ok, "OK_BUTTON", 1);
        if (object != 0) {
            object->flags &= ~0x10;
            object->x =
                pdata->left + pdata->width - object->pfx2d->tex_w;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
    if (pdata->prompt_flags & 4) {
        object = load_prompt(
            pdata, &pdata->prompt_retry, "TRY_AGAIN_BUTTON", 1);
        if (object != 0) {
            object->flags &= ~0x10;
            object->x =
                pdata->left + pdata->width - object->pfx2d->tex_w;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
    if (pdata->prompt_flags & 2) {
        object = load_prompt(
            pdata, &pdata->prompt_cancel, "BUTTONCANCEL", 1);
        if (object != 0) {
            object->flags &= ~0x10;
            object->x = pdata->left;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
    if (pdata->prompt_flags & 8) {
        object = load_prompt(
            pdata, &pdata->prompt_cancel, "RETURN_BUTTON", 0);
        if (object != 0) {
            object->flags &= ~0x10;
            object->x = pdata->left;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
}

void konquest_map_setup_fight(
    int mission, int pair_a_low, int pair_a_high,
    int pair_b_low, int pair_b_high, int value_a, int value_b,
    int background_root, const char* fight_name) {
    konquest_save_data.mission_pair_a =
        (unsigned int)pair_a_low | ((unsigned int)pair_a_high << 16);
    konquest_save_data.mission_pair_b =
        (unsigned int)pair_b_low | ((unsigned int)pair_b_high << 16);
    konquest_save_data.next_value_a = value_a;
    konquest_save_data.next_value_b = value_b;
    konquest_save_data.next_mission = mission;
    konquest_save_data.background_and_flags =
        (unsigned int)konquest_pdata->region->background_id |
        ((unsigned int)background_root << 16);
    if (fight_name != 0) {
        strncpy(konquest_save_data.fight_name, fight_name, 0x40);
    } else {
        konquest_save_data.fight_name[0] = '\0';
    }
}

void trial_setup_fight(void) {
    PlyrInfo* player;
    PlyrInfo* drone;

    if (g_game_info.plyr0.player_state == 2) {
        player = &g_game_info.plyr1;
        drone = &g_game_info.plyr0;
    } else {
        player = &g_game_info.plyr0;
        drone = &g_game_info.plyr1;
    }
    drone->player_index =
        (unsigned short)konquest_save_data.mission_pair_a;
    drone->field_14 =
        (int)konquest_save_data.mission_pair_a >> 16;
    player->player_index =
        (unsigned short)konquest_save_data.mission_pair_b;
    player->field_14 =
        (int)konquest_save_data.mission_pair_b >> 16;
    force_bgnd_num =
        (unsigned short)konquest_save_data.background_and_flags;
}
static inline KonquestMissionState* get_mission_state(void);

int trial_invisible_callback(PlyrPdata* player) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 1;
    }
    if (player->plyr_num != state->fight->animation_side &&
        (state->restriction_flags & 0x20)) {
        return 0;
    }
    return 1;
}

float trial_damage_callback(
    int player, int damage_type, float damage) {
    KonquestMissionState* state = get_mission_state();
    int player_side = player == 0;
    PlyrPdata* selected;

    mission_state = state;
    if (state == 0) {
        return damage;
    }
    damage *= player == 0
        ? state->player_one_damage_scale
        : state->player_two_damage_scale;

    if (state->restriction_flags & 4) {
        selected = player_side
            ? g_game_info.plyr1.slot.pdata
            : g_game_info.plyr0.slot.pdata;
        if (player_side == state->fight->animation_side &&
            !is_weapon_style(selected->fighter_definition)) {
            damage = 0.0f;
        }
    } else if (state->restriction_flags & 8) {
        if (player_side == state->fight->animation_side) {
            if (state->restriction_phase == 0) {
                if (state->fight->pdata->state == 0x120C) {
                    state->restriction_phase = 1;
                } else {
                    damage = 0.0f;
                }
            } else if (state->drone->pdata->state == 0) {
                state->restriction_phase = 0;
                damage = 0.0f;
            }
        }
    } else if ((state->restriction_flags & 0x10) &&
               player_side == state->fight->animation_side &&
               damage_type != 1) {
        damage = 0.0f;
    }
    return damage;
}

int current_player_is_drone(void) {
    PlyrPdata* drone;

    if (plyr_pdata == 0) {
        return 0;
    }
    if (g_game_info.plyr0.player_state == 2) {
        drone = g_game_info.plyr1.slot.pdata;
    } else {
        drone = g_game_info.plyr0.slot.pdata;
    }
    return plyr_pdata == drone;
}

int trial_get_background_root(void) {
    return (int)konquest_save_data.background_and_flags >> 16;
}

void konquest_run_ending(void) {
    fade_to_black(8, 1);
    gamelogic_jump(2, p_konquest_ending);
}

static inline KonquestMissionState* get_mission_state(void) {
    KonquestMissionState* state = mission_state_item.state;

    if (state != 0 && state->hdr.instance == mission_state_item.instance) {
        return state;
    }
    return 0;
}

int trial_never_passed_this_mission(void) {
    KonquestMissionState* state = mission_state_item.state;

    if (state == 0 ||
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state == 0) {
        return 0;
    }
    return get_konq_profile_value(5, state->mission_index) <= 0;
}

int trial_change_style_callback(int player) {
    KonquestMissionState* state = mission_state_item.state;

    if (state == 0 ||
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state == 0) {
        return 1;
    }
    if (((player == 0 && state->fight->animation_side == 0) ||
         (player == 1 && state->fight->animation_side == 1)) &&
        (state->restriction_flags & 2)) {
        return 0;
    }
    return 1;
}

void trial_set_special_restrictions(unsigned int restrictions) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->restriction_flags = restrictions;
    }
}

void trial_set_round_health_restoration(float restoration) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        if (restoration < -1.0f || restoration > 1.0f) {
            restoration = 1.0f;
        }
        state->round_health_restoration = restoration;
    }
}

void trial_register_special_move(unsigned char move) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0 && mode_of_play == 8 &&
        (g_game_info.field_04 & 0x20) &&
        (aproc->pid == 0x1001 || aproc->pid == 0x1002) &&
        plyr_pdata != 0) {
        trial_register_attack(plyr_pdata->plyr_num, 3, move);
    }
}

void trial_register_script_function(unsigned char function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0 && mode_of_play == 8 &&
        (g_game_info.field_04 & 0x20) &&
        (aproc->pid == 0x1001 || aproc->pid == 0x1002) &&
        plyr_pdata != 0) {
        trial_register_attack(
            plyr_pdata->plyr_num,
            (unsigned char)plyr_pdata->player_slot, function);
    }
}

int trial_block_callback(int player) {
    KonquestMissionState* state = mission_state_item.state;

    if (state == 0 ||
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state == 0) {
        return 1;
    }
    if (((player == 0 && state->fight->animation_side == 0) ||
         (player == 1 && state->fight->animation_side == 1)) &&
        (state->restriction_flags & 1)) {
        return 0;
    }
    return 1;
}

void trial_debug_mission_list(void) {
}

void trial_mirror_anims_if_needed(void) {
    if (mission_state->fight->animation_side == 1) {
        camera_set_animation_mirror_plane(3);
    }
}

void drone_set_anim_step(float step) {
    if (current_anim_pdata != 0) {
        current_anim_pdata->step = step;
    }
}

void drone_set_damage_multiplier(int player, float multiplier) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return;
    }
    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        state->player_one_damage_scale = multiplier;
        return;
    }
    state->player_two_damage_scale = multiplier;
}

void drone_set_handicap(int player, float handicap) {
    KonquestMissionState* state = get_mission_state();
    PlyrInfo* info;

    mission_state = state;
    if (state == 0) {
        return;
    }
    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        info = &g_game_info.plyr0;
    } else {
        info = &g_game_info.plyr1;
    }
    info->field_10 = handicap;
    if (info->field_0C > handicap) {
        info->field_0C = handicap;
    }
}

void drone_do_special_move(int player, int script_function) {
    KonquestMissionFightInfo* fighter;
    CmdScript* script;

    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        fighter = mission_state->fight;
    } else {
        fighter = mission_state->drone;
    }
    script = get_cmdscript_for_proc(fighter->process);
    if (script != 0) {
        script->unk28 = script_function;
        xfer_proc(
            fighter->process, (MkProcEntryFn)p_run_special_move);
    }
}

void drone_set_position(
    int player, float x, float y, float z) {
    KonquestMissionFightInfo* fighter;
    Vec position;

    position.x = x;
    position.y = y;
    position.z = z;
    if (mission_state->fight->animation_side == 1) {
        position.x = -x;
        position.z = -z;
    }
    fighter = player == 0 ? mission_state->fight : mission_state->drone;
    move_player(
        fighter->active_object, &position,
        &fighter->active_object->ang);
    force_midpoint_calculation_update = 1;
}

void drone_set_script(int player, int script_function) {
    KonquestMissionState* state = get_mission_state();
    MkProc* process;
    CmdScript* script;

    mission_state = state;
    if (state == 0) {
        return;
    }
    if (player == 2) {
        process = state->script_process;
        if (process != 0 &&
            process->instance == state->script_process_instance) {
            script = get_cmdscript_for_proc(process);
            script->unk28 = script_function;
            xfer_proc(process, (MkProcEntryFn)call_mission_script);
        }
        return;
    }

    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        process = (MkProc*)g_game_info.plyr0.idle_proc;
    } else {
        process = (MkProc*)g_game_info.plyr1.idle_proc;
    }
    script = get_cmdscript_for_proc(process);
    script->unk28 = script_function;
    xfer_player_proc(process, (MkProcEntryFn)call_mission_script);
}

void cleanup_mission_state(void) {
    mission_state = 0;
    mission_state_item.state = 0;
    mission_state_item.instance = 0;
    current_anim_pdata = 0;
}

int konquest_passed_last_mission(void) {
    return konquest_save_data.passed_last_mission;
}

void play_background_music(int sound) {
    if (sound >= 0) {
        if (bgnd_music_ptr1 != 0) {
            snd_stop(bgnd_music_ptr1);
        }
        bgnd_music_ptr1 = snd_req(sound);
    }
}

void trial_end_tunes(void) {
    KonquestMissionState* state = get_mission_state();
    int music;

    mission_state = state;
    if (state == 0 || !(state->display_flags & 1)) {
        return;
    }
    music = state->tune_table->entries[state->randomized_side].music;
    if (music >= 0) {
        if (bgnd_music_ptr1 != 0) {
            snd_stop(bgnd_music_ptr1);
        }
        bgnd_music_ptr1 = snd_req(music);
    }
}

void trial_show_spoken_text_window(
    int string_id, int style, int sound, int flags,
    float x, float y, float scale, int unused_stack) {
    MslSoundHandle voice = 0;

    if (sound >= 0) {
        voice = snd_req(sound);
    }
    trial_show_text_window(string_id, style, flags, x, y, scale);
    if (voice != 0) {
        snd_stop(voice);
    }
}

int trial_show_standard_fight_messages(void) {
    return mission_state->display_flags & 1;
}

void drone_blend_to_ani(int animation, int transition) {
    transition_to_anim_script(
        current_anim_pdata, animation, transition);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
}

void drone_apply_damage(int player) {
    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        adjust_p1_life();
        return;
    }
    adjust_p2_life();
}

int trial_get_round_length(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    return state->round_timer;
}

void trial_set_tick_function(unsigned int script_enabled) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->tick_script_function = script_enabled;
    }
}

void trial_setup_onscreen_display_items(
    int item_c, int enabled, const char* format) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->display_item_c = item_c;
        state->display_item_a = enabled;
        state->display_format = format;
    }
}

void trial_set_ending_functions(
    int winner_function, int loser_function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->winner_end_function = winner_function;
        state->loser_end_function = loser_function;
    }
}

void trial_set_round_timer(int ticks) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->round_timer = ticks;
    }
}

void trial_set_num_rounds(int rounds) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->num_rounds = rounds;
    }
}

void drone_set_special_directions(int flags) {
    float direction;

    direction = (flags & 1) ? 0.0f : 1.0f;
    setDroneOverrideSwitch(&direction);
}

void skip_end_of_trial_wrapup(void) {
    if (mission_state->fight->animation_side ==
        (aproc->pid != 0x1001)) {
        mission_state->player_one_wrapup_state = 2;
        return;
    }
    mission_state->player_two_wrapup_state = 2;
}

void trial_set_next_setup_function(int setup_function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->next_setup_function = setup_function;
        state->display_flags |= 4;
    }
}

void trial_set_next_mission(
    int mission, int pair_a_low, int pair_a_high,
    int pair_b_low, int pair_b_high, int value_a, int value_b,
    int background_root) {
    unsigned int old_background = konquest_save_data.background_and_flags;

    konquest_save_data.next_mission = mission;
    konquest_save_data.mission_pair_a =
        (unsigned int)pair_a_low | ((unsigned int)pair_a_high << 16);
    konquest_save_data.mission_pair_b =
        (unsigned int)pair_b_low | ((unsigned int)pair_b_high << 16);
    konquest_save_data.next_value_a = value_a;
    konquest_save_data.next_value_b = value_b;
    konquest_save_data.background_and_flags =
        ((unsigned int)background_root << 16) |
        (old_background & 0xFFFF);
    fade_to_black(8, 1);
    gamelogic_jump(2, p_gamelogic);
}

void trial_clear_provision(void) {
    KonquestMissionState* state = mission_state_item.state;

    if (state == 0 ||
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state != 0 && plyr_obj == state->fight->active_object) {
        state->condition_active = 0;
    }
}

static void register_condition(
    const KonquestMissionCondition* condition) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return;
    }
    switch (condition->type) {
    case 0:
        state->condition_active = 0;
        return;
    case 1:
        state->condition_value = condition->value;
        state->condition_player = state->fight->animation_side;
        state->condition_mode = 1;
        state->condition_active = 1;
        return;
    case 2:
        state->condition_value = condition->value;
        state->condition_player = state->drone->animation_side;
        state->condition_mode = 1;
        state->condition_active = 1;
        return;
    case 3:
        state->condition_value = condition->value;
        state->condition_player = state->fight->animation_side;
        state->condition_mode = 0;
        state->condition_active = 1;
        return;
    case 4:
        state->condition_value = condition->value;
        state->condition_player = state->drone->animation_side;
        state->condition_mode = 2;
        state->condition_active = 1;
        return;
    case 5:
        state->condition_value = condition->value;
        state->condition_player = state->drone->animation_side;
        state->condition_mode = 3;
        state->condition_active = 1;
        return;
    }
}

void increment_required_moves_progress(
    KonquestRequiredMove* move,
    KonquestRequiredMoveProgress* progress) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return;
    }
    move->next = move->next->next;
    if (move->next == 0) {
        move->next = move->sentinel;
        increment_progress_count(1);
        progress->current++;
        if (progress->current == progress->required) {
            progress->current = 0;
            state->move_message = 0;
            state->move_message_param = 0;
        } else {
            state->move_message = move->message;
            state->move_message_param = move->message_parameter;
        }
        trial_show_move_message();
    }
}

static void p_flip_move_description(void) {
    KonquestMissionState* state;
    int flipped;

    if (mission_state->display_item_c == 0) {
        return;
    }
    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        flipped = is_a_to_the_right_of_b(
            state->fight->active_object,
            state->drone->active_object);
    } else {
        flipped = 0;
    }
    if (flipped != mission_state->move_description_flipped) {
        mission_state->move_description_flipped = flipped;
        trial_show_move_message();
    }
}

static void increment_progress_count(unsigned char increment) {
    StringObj* string;
    char text[64];

    mission_state->progress_count += increment;
    if (mission_state->progress_count >=
        mission_state->progress_required) {
        mission_state->progress_count =
            mission_state->progress_required;
    }
    if (increment != 0) {
        snd_req(0x15EB);
    }
    if (increment != 0 && (mission_state->display_flags & 2)) {
        mission_state->drone->current_health = 1.0f;
    }
    if (mission_state->display_item_a == 0) {
        return;
    }

    sprintf(
        text, mission_state->display_format,
        mission_state->progress_count,
        mission_state->progress_required);
    string = mission_state->progress_string;
    if (string != 0 &&
        string->instance == mission_state->progress_string_instance) {
        update_string_obj(string, 6, text);
    } else {
        string = string_right_xy(
            0x9007, 6, text, screen_width - 0x28,
            0x146, 0x22);
        if (string != 0) {
            mission_state->progress_string = string;
            mission_state->progress_string_instance = string->instance;
        }
    }
    show_background_box(
        2, (screen_width - 0x32) - string->text_w,
        0x144, 0x24, string->text_w + 0x14, 0x1C);
}

void trial_do_dialog(
    int unused, int string_id, unsigned int ticks, int wait) {
    MkHdr* dialog;
    unsigned int instance;
    int scaled_ticks = (int)((float)ticks * inverse_game_speed);

    dialog = konquest_set_dialog_text(
        get_string_by_id(string_id), 0);
    calc_print_speed_for_nis_dialog(dialog, scaled_ticks);
    if (dialog == 0 || wait == 0) {
        return;
    }
    instance = dialog->instance;
    while (dialog != 0 && dialog->instance == instance) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }
}

int get_konquest_drone_switch_state(int player) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    if (player == 0) {
        return state->player_one_switch_state;
    }
    return state->player_two_switch_state;
}

void trial_set_move_message(int message, int parameter) {
    mission_state->move_message = message;
    mission_state->move_message_param = parameter;
}

void trial_set_combo_requirement(int hits, float damage) {
    mission_state->combo_hits = hits;
    mission_state->combo_damage = damage;
}

void trial_set_type(int type) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->trial_type = type;
        if (type == 2) {
            state->display_flags = 9;
            state->randomized_side = randu0(2, state);
        }
    }
}

static void p_run_special_move(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
}

static float p_trial_tick(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0 || state->tick_script_function == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(
        state->tick_script, state->tick_script_function);
    cmdscript_execute(state->tick_script);
    return 1.0f;
}

static float p_finish_countdown(void) {
    StringObj* string = mission_state->countdown_string;

    aproc->flags |= 0x10;
    if (string != 0 &&
        string->instance == mission_state->countdown_string_instance) {
        update_string_obj(string, 1, "MISSION COMPLETE");
        _mkproc_sleep_ticks = 60.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }

    string = mission_state->countdown_string;
    if (string != 0 &&
        string->instance == mission_state->countdown_string_instance) {
        if (string->instance != 0) {
            ((int (*)(StringObj*))string->vtbl->destroy)(string);
        }
        mission_state->countdown_string = 0;
        mission_state->countdown_string_instance = 0;
    }
    return -1.0f;
}

void trial_restart_round(void) {
    start_mkpfx_FadeSnapShot();
    move_plyrs_to_round_start();
    start_constrain_proc();
    skip_camera_intro();
    stop_tunes();
    bgnd_music_ptr1 = snd_req(
        g_game_info.section->music_id_round_0_1);
    set_camera_destination(&camera_obj->pos_x, camera_obj);
    set_camera_target_angle(&camera_obj->ang_x);
    force_midpoint_calculation_update = 0;
    xfer_camera(p_camera_proc, 1);
    trial_show_monk(0);
}

void p_finish_transform_player(void) {
    KonquestMissionState* state = get_mission_state();
    MkProc* animation_process;
    AnimPdata* animation;
    MkObj* monk;

    mission_state = state;
    animation_process = state->fight->pdata->anim_proc;
    if (animation_process != 0 &&
        animation_process->instance !=
            state->fight->pdata->anim_proc_instance) {
        animation_process = 0;
    }
    animation = (AnimPdata*)pdata_of_proc(animation_process);
    monk = (MkObj*)trial_get_monk();

    xfer_proc(animation_process, p_anim_idle);
    set_anim_script_frame(
        animation, bgnd_animations.transform_animation,
        0x43, 63.0f);
    plyr_obj->pos = monk->pos;
    update_mkobj(plyr_obj);
    start_hero_transform_effect(plyr_obj);
    shake_camera(3, 0.03f);
    _mkproc_sleep_ticks = 15.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();

    plyr_obj->flags_08 |= 2;
    plyr_obj->scale.x = 1.0f;
    plyr_obj->scale.y = 0.8f;
    plyr_obj->scale.z = 1.0f;
    plyr_obj->hide_flags &= ~0x20;
    monk->hide_flags |= 0x20;

    while (animation->frame < animation->high_frame - 10.0f) {
        animpdata_ani_1_frame(animation);
        if (plyr_obj->scale.y < 1.0f) {
            plyr_obj->scale.y += 0.02f;
        } else {
            plyr_obj->scale.y = 1.0f;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }
    plyr_obj->scale.y = 1.0f;
    update_mkobj(plyr_obj);
    plyr_obj->flags_08 &= ~2;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
}

static void call_mission_script(void) {
    if (active_cmdscript->unk28 != 0) {
        cmdscript_setup_execution(
            mission_state->tick_script,
            mission_state->tick_script_function);
        cmdscript_execute(mission_state->tick_script);
    }
    if (aproc->pid == 0x1001 || aproc->pid == 0x1002) {
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(j_exit, 0.0f);
        return;
    }
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }
}

void trial_start_new_round(void) {
    KonquestMissionFightInfo* player = mission_state->fight;
    KonquestMissionFightInfo* drone = mission_state->drone;

    g_game_info.flags &= ~1;
    if (mission_state->round_health_restoration > 0.0f) {
        player->current_health +=
            mission_state->round_health_restoration;
    } else {
        player->current_health = player->maximum_health;
    }
    if (player->current_health > 1.0f) {
        player->current_health = 1.0f;
    }
    drone->current_health = drone->maximum_health;
}

void drone_start_bleeding(int player, float rate) {
    static const Vec zero_vector = {0.0f, 0.0f, 0.0f};
    static const Vec direction_vector = {1.0f, 0.0f, 0.0f};
    KonquestBloodRushPdata* pdata;
    KonquestMissionState* state;
    MkObj* fighter;
    MkObj* mirror;
    Vec direction;
    Vec velocity;

    velocity = zero_vector;
    direction = direction_vector;

    state = mission_state_item.state;
    if (state != 0 &&
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state == 0) {
        return;
    }

    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        direction.x = 1.0f;
        fighter = (MkObj*)g_game_info.plyr0.slot.fighter;
        mirror = (MkObj*)g_game_info.plyr0.slot.mirror_a;
    } else {
        direction.x = -1.0f;
        fighter = (MkObj*)g_game_info.plyr1.slot.fighter;
        mirror = (MkObj*)g_game_info.plyr1.slot.mirror_a;
    }

    start_gusher(
        &heart_beat, fighter, mirror, 9, &velocity, &direction);

    pdata = 0;
    if (_create_mkproc_generic_nostack(
            0x501B, 0x1F, p_blood_rush,
            sizeof(KonquestBloodRushPdata), (MkHdr**)&pdata) == 0) {
        return;
    }

    pdata->player = *(int*)((char*)fighter + 0x1D0);
    pdata->rate = -1.0f * rate;
}

float p_blood_rush(void) {
    KonquestBloodRushPdata* pdata =
        (KonquestBloodRushPdata*)apdata;

    if (g_game_info.field_04 & 0x20) {
        if (pdata->player == 0) {
            adjust_p1_life();
        } else {
            adjust_p2_life();
        }
    }
    return 30.0f;
}

float successful_trial_player_wrapup(void) {
    set_ani_speed(1.0f);
    blend_to_stance(0.1f);
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    mission_state->player_one_wrapup_state = 2;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

float trial_run_loser_animation_script(void) {
    plyr_anim_pdata->step = 1.0f;
    face_opponent_now();
    plyr_pdata->death_type = 0xA;
    blend_to_ani(bgnd_animations.loser_start, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    blend_to_ani(bgnd_animations.loser_loop, 0, 0.1f);
    xfer_proc(plyr_anim_proc, p_animate);
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_stay_down_dead, 0.0f);
    return 0.0f;
}

float end_of_trial_wrapup(int failed) {
    if (mission_state->display_flags & 1) {
        mission_state->player_one_wrapup_state = 2;
        mission_state->player_two_wrapup_state = 2;
        return 0.0f;
    }
    if (mission_state->fight->animation_side ==
        (aproc->pid != 0x1001)) {
        if (mission_state->player_one_wrapup_state != 0) {
            mission_state->player_one_wrapup_state = 2;
            return 0.0f;
        }
        mission_state->player_one_wrapup_state = 1;
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(successful_trial_player_wrapup, 0.0f);
        return 0.0f;
    }
    if (mission_state->player_two_wrapup_state != 0) {
        mission_state->player_two_wrapup_state = 2;
        return 0.0f;
    }
    mission_state->player_two_wrapup_state = 1;
    ((KonquestMissionJumpVtable*)aproc->vtbl)->jump_sleep(
        failed ? failed_trial_drone_wrapup
               : successful_trial_drone_wrapup,
        0.0f);
    return 0.0f;
}

float p_konquest_register_bleeding(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0 || mode_of_play != 8 ||
        !(g_game_info.field_04 & 0x20) ||
        state->trial_type != 0 || state->bleeding_required <= 0 ||
        state->drone->pdata->postround_value == 0.0f) {
        return -1.0f;
    }
    if (state->fight->animation_side == 0) {
        become_plyr2_proc();
    } else {
        become_plyr1_proc();
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 0x18, 1);
    return 30.0f;
}

static void ps_konquest_trial_monk(void) {
    current_anim_pdata = 0;
}

static void pw_konquest_trial_monk(void) {
    KonquestMissionState* state = get_mission_state();
    MkProc* process;

    mission_state = state;
    if (state == 0) {
        return;
    }
    process = state->monk_process;
    if (process != 0 &&
        process->instance == state->monk_process_instance) {
        current_anim_pdata = (AnimPdata*)pdata_of_proc(aproc);
    }
}

KonquestMissionFightInfo* trial_get_drone_info(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    return state->drone;
}

void trial_show_monk(int show) {
    MkHdr* monk_hdr = trial_get_monk();
    MkObj* monk;

    if (monk_hdr == 0) {
        return;
    }
    monk = (MkObj*)monk_hdr;
    if (show != 0) {
        monk->hide_flags &= ~0x20;
    } else {
        monk->hide_flags |= 0x20;
    }
}

MkHdr* trial_get_monk(void) {
    KonquestMissionState* state = mission_state_item.state;
    MkHdr* monk;

    if (state == 0 ||
        state->hdr.instance != mission_state_item.instance) {
        state = 0;
    }
    mission_state = state;
    if (state == 0) {
        return 0;
    }
    monk = state->monk;
    if (monk != 0 && monk->instance == state->monk_instance) {
        return monk;
    }
    return 0;
}
