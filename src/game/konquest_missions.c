#include "game/game_info.h"
#include "game/controller.h"
#include "game/konquest.h"
#include "game/konquest_items.h"
#include "game/konquest_lipsync.h"
#include "game/trial.h"
#include "math/gxVect.h"
#include "msl/msl_types.h"
#include "platform/io.h"
#include "runtime/anim_pdata.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/cstring.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"

typedef struct KonquestMissionFightInfo {
    int field_0x0;
    int animation_side;                /* +0x04 */
    int player_state;                  /* +0x08 */
    float current_health;               /* +0x0C */
    float maximum_health;               /* +0x10 */
    char pad14[0x2C];
    int field_40;                        /* +0x40 */
    char pad44[0x10];
    int character_id;                    /* +0x54 */
    union {
        PlyrPdata* pdata;
        FighterMirror* fighter;
    };                                   /* +0x58 */
    MkObj* active_object;               /* +0x5C */
    char pad60[4];
    MkProc* process;                    /* +0x64 */
} KonquestMissionFightInfo;

typedef struct KonquestMissionTuneEntry {
    int round_one_music;
    int later_round_music;
    int end_music;
} KonquestMissionTuneEntry;

typedef struct KonquestMissionTuneTable {
    int basic_round_one_music[3];
    KonquestMissionTuneEntry entries[2];
    int script_music;
} KonquestMissionTuneTable;

typedef struct KonquestMissionCondition {
    int value;
    int type;
} KonquestMissionCondition;

typedef struct KonquestSuccessCondition {
    KonquestMissionCondition condition;
    int required_count;
    int current_count;
} KonquestSuccessCondition;

typedef struct KonquestScreenLatch {
    ScreenObj* object;
    unsigned int instance;
} KonquestScreenLatch;

typedef struct KonquestBackgroundBox {
    union {
        struct {
            KonquestScreenLatch fill;
            KonquestScreenLatch top;
            KonquestScreenLatch bottom;
            KonquestScreenLatch left;
            KonquestScreenLatch right;
        };
        KonquestScreenLatch pieces[5];
    };
} KonquestBackgroundBox;

typedef struct KonquestMissionState {
    MkHdr hdr;
    union {
        struct {
            int combo_hits;            /* +0x008 */
            float combo_damage;        /* +0x00C */
            int combo_complete;        /* +0x010 */
            char pad014[0x17C];
            int bleeding_required;     /* +0x190 */
            char pad194[0x11C];
        };
        struct {
            KonquestSuccessCondition success_conditions[35];
            char success_pad238[0x78];
        };
        unsigned char trial_data[0x2A8];
    };
    int display_item_a;                /* +0x2B0 */
    const char* display_format;         /* +0x2B4 */
    unsigned char progress_count;       /* +0x2B8 */
    unsigned char progress_required;    /* +0x2B9 */
    char pad2BA[2];
    int display_item_c;                /* +0x2BC */
    int move_description_flipped;      /* +0x2C0 */
    const char* move_message;           /* +0x2C4 */
    const char* move_message_param;     /* +0x2C8 */
    int trial_type;                    /* +0x2CC */
    int condition_value;               /* +0x2D0 */
    int condition_player;              /* +0x2D4 */
    int condition_mode;                /* +0x2D8 */
    int condition_active;              /* +0x2DC */
    int condition_ticks;               /* +0x2E0 */
    int drone_difficulty;               /* +0x2E4 */
    int field_2E8;                     /* +0x2E8 */
    int current_setup_function;        /* +0x2EC */
    int next_setup_function;           /* +0x2F0 */
    int winner_end_function;           /* +0x2F4 */
    int loser_end_function;            /* +0x2F8 */
    unsigned int tick_script_function; /* +0x2FC */
    unsigned int display_flags;        /* +0x300 */
    unsigned int player_one_switch_state; /* +0x304 */
    unsigned int player_two_switch_state; /* +0x308 */
    int field_30C;                     /* +0x30C */
    StringObj* move_string;             /* +0x310 */
    unsigned int move_string_instance;  /* +0x314 */
    StringObj* move_param_string;       /* +0x318 */
    unsigned int move_param_string_instance; /* +0x31C */
    StringObj* progress_string;         /* +0x320 */
    unsigned int progress_string_instance; /* +0x324 */
    StringObj* countdown_string;        /* +0x328 */
    unsigned int countdown_string_instance; /* +0x32C */
    KonquestBackgroundBox background_boxes[3]; /* +0x330 */
    MkProc* monk_process;              /* +0x3A8 */
    unsigned int monk_process_instance; /* +0x3AC */
    MkProc* script_process;            /* +0x3B0 */
    unsigned int script_process_instance; /* +0x3B4 */
    MkObj* monk;                       /* +0x3B8 */
    unsigned int monk_instance;        /* +0x3BC */
    AniTextureControl* monk_face_texture; /* +0x3C0 */
    unsigned int monk_face_texture_instance; /* +0x3C4 */
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
    int transform_complete;             /* +0x3F4 */
    int restriction_phase;             /* +0x3F8 */
    union {
        struct {
            float player_one_damage_scale;
            float player_two_damage_scale;
        };
        float damage_scale[2];
    };                                 /* +0x3FC */
    unsigned int restriction_flags;    /* +0x404 */
} KonquestMissionState;

typedef struct KonquestMissionSaveData {
    char pad00[0x0C];
    int region_index;                    /* +0x0C */
    char pad10[0x28];
    char fight_name[0x40];             /* +0x38 */
    unsigned int mission_pair_a;       /* +0x78 */
    unsigned int mission_pair_b;       /* +0x7C */
    int next_value_a;                  /* +0x80 */
    int next_value_b;                  /* +0x84 */
    int next_mission;                  /* +0x88 */
    unsigned int background_and_flags; /* +0x8C */
    int passed_last_mission;
    int progression;
} KonquestMissionSaveData;

typedef struct KonquestRegionAsset {
    void* fight_files;
    MkFileEntry* art_files;
    char pad08[4];
    char* string_bank;
    char pad10[4];
} KonquestRegionAsset;

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

typedef struct DroneOverrideInfo {
    float likelihood_scale;
    unsigned int flags;
} DroneOverrideInfo;

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

typedef struct KonquestRequiredSequence {
    const char* message;
    const char* message_parameter;
    struct KonquestRequiredAttack* current;
    struct KonquestRequiredAttack* first;
} KonquestRequiredSequence;

typedef struct KonquestRequiredAttack {
    unsigned char type;
    unsigned char value;
    char pad02[2];
    int flags;
    struct KonquestRequiredAttack* next;
} KonquestRequiredAttack;

typedef struct KonquestRequiredSequenceList {
    unsigned char sequence_count;
    unsigned char current_sequence;
    char pad02[2];
    KonquestRequiredSequence entries[12];
    KonquestRequiredAttack attacks[40];
    int attack_count;
} KonquestRequiredSequenceList;

typedef struct KonquestTrialScriptAttrs {
    char pad00[0x3C];
    int condition_ticks;
} KonquestTrialScriptAttrs;

typedef struct KonquestTrialAnimations {
    AnimScript* monk_animation;          /* +0x00 */
    char pad04[4];
    void* loser_start;                  /* +0x08 */
    char pad0C[8];
    void* loser_loop;                   /* +0x14 */
    AnimScript* monk_transform_animation; /* +0x18 */
    void* transform_animation;          /* +0x1C */
} KonquestTrialAnimations;

typedef struct KonquestRoundStartPositions {
    char pad00[0x0C];
    Vec player_one_position;
    Vec player_one_angles;
    Vec player_two_position;
    Vec player_two_angles;
} KonquestRoundStartPositions;

typedef struct TrialWrapupEntry {
    int sound_id;
    LipSyncKeyframe* lip_sync;
    float post_sound_delay;
    int animation_id;
} TrialWrapupEntry;

typedef struct TrialWrapupData {
    TrialWrapupEntry* selected_success;
    TrialWrapupEntry* success_table;
    TrialWrapupEntry* selected_failure;
    TrialWrapupEntry* failure_table;
} TrialWrapupData;

typedef struct KonquestAnimScriptView {
    char pad00[0x18];
    int frame_count;
} KonquestAnimScriptView;

typedef struct KonquestSwitchPdata {
    MkHdr hdr;
    PlyrInfo* player;
} KonquestSwitchPdata;

typedef struct KonquestTrialWindowPdata {
    MkHdr hdr;
    int left;                           /* +0x08 */
    int bottom;                         /* +0x0C */
    int priority;                       /* +0x10 */
    int width;                          /* +0x14 */
    int top;                            /* +0x18 */
    unsigned int prompt_flags;          /* +0x1C */
    int timeout;                        /* +0x20 */
    unsigned int color;                 /* +0x24 */
    int font;                           /* +0x28 */
    int controller_port;                /* +0x2C */
    const char* button_charmap;         /* +0x30 */
    int swap_buttons;                   /* +0x34 */
    char text[0x4B0];                   /* +0x38 */
    int visible_item_count;             /* +0x4E8 */
    int art_slot;                       /* +0x4EC */
    KonquestScreenLatch frame[9];       /* +0x4F0 */
    KonquestScreenLatch prompt_ok;      /* +0x538 */
    KonquestScreenLatch prompt_retry;   /* +0x540 */
    KonquestScreenLatch prompt_548;     /* +0x548 */
    KonquestScreenLatch prompt_550;     /* +0x550 */
    KonquestScreenLatch prompt_cancel;  /* +0x558 */
} KonquestTrialWindowPdata;

static KonquestMissionState* mission_state;
static KonquestMissionStateLatch mission_state_item;
static AnimPdata* current_anim_pdata;
char danton20_charmap[] = "|][<}=+{.....&.@.";
char movelist_charmap[] = "lrRLYBAX...../.:.";
TrialWrapupEntry generic_char_success_table[5] = {
    {0x39, 0, 10.0f, 0x0A}, {0x3A, 0, 10.0f, 0x0B},
    {0x3B, 0, 10.0f, 0x0A}, {0x3C, 0, 10.0f, 0x0C},
    {0x3D, 0, 10.0f, 0x0D}
};
TrialWrapupEntry generic_char_failure_table[5] = {
    {0x3E, 0, 10.0f, 0x0E}, {0x3F, 0, 10.0f, 0x0F},
    {0x40, 0, 10.0f, 0x10}, {0x41, 0, 10.0f, 0x11},
    {0x42, 0, 0.0f, 0x12}
};
TrialWrapupData generic_char_wrapup_data = {
    0, generic_char_success_table, 0, generic_char_failure_table
};
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
extern unsigned char* p1_profile_konquest;
extern int mcard_msg_active;
extern KonquestRegionAsset konquest_region_data[9];
extern int round_winner;
extern int winner;
extern int f_fatality_finished;
int text_window_state;

void camera_set_animation_mirror_plane(int mode);
int adjust_p1_life(float amount);
int adjust_p2_life(float amount);
MslSoundHandle snd_req(int sound);
void snd_stop(MslSoundHandle sound);
void transition_to_anim_script(
    float transition_frames, AnimPdata* animation,
    AnimScript* script, unsigned int flags);
int get_konq_profile_value(int category, int index);
void setDroneOverrideSwitch(int activated, DroneOverrideInfo* info);
void fade_to_black(int ticks, int flags);
void fade_from_black(int ticks, int flags);
void gamelogic_jump(int action, MkProcEntryFn entry);
float p_konquest_ending(void);
float p_gamelogic(void);
float j_exit_blend_stance(void);
float j_exit(void);
void set_ani_speed(float speed);
void blend_to_stance(float blend);
static float p_run_special_move(void);
static float call_mission_script(void);
void move_player(MkObj* object, const Vec* position, Vec* angle);
int sprintf(char* destination, const char* format, ...);
int is_a_to_the_right_of_b(MkObj* a, MkObj* b);
int is_weapon_style(PlyrFighterDefinition* fighter);
int is_timer_off(void);
int is_pX_airborn(int player);
extern int game_tick_ctr;
MkHdr* konquest_set_dialog_text(
    const char* text, const LipSyncKeyframe* lip_sync_keyframes);
void calc_print_speed_for_nis_dialog(void* dialog, unsigned int ticks);
void display_numerical_change(
    StringObj* string, int font, int start, int change,
    int ticks, int acceleration_interval, void* context);
void pfx_2d_obj_set_alpha_by_id(int oid, int alpha);
void pfx_2d_obj_set_alpha(ScreenObj* object, int alpha);
void duck_sounds(float volume);
unsigned int fx_by_owner(const char* name, unsigned int owner);
unsigned int fx_next_emitter(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
void fx_restart_emit(unsigned int handle);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
AnimScript* get_animation(int animation_id);
float duration_of_lip_sync(const LipSyncKeyframe* keyframes);
MslSoundHandle plyr_snd_req(int sound_id);
void rotate_towards_him(float blend_time);
void plyr_weapon_hide(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
void plyr_weapon_show(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
void init_pwr_bars(void);
int are_powerbars_retracted(void);
void extend_powerbars(void);
void start_powerbar_monitor(void);
void update_plyr_medals(void);
void do_win_effect(void);
void end_music(void);
void destroy_onscreen_fight_2d_objects(void);
void reset_fight(int mode);
MkProc* load_hero_model(AnimScript* animation);
void insert_ground_me_mkobj(MkObj* object);
AniTextureControl* konquest_create_monk_face_ani_texture(MkObj* object);
static void trial_load_monk(void);
void push_game_state(int state);
void pop_game_state(void);
void xfer_player_proc(MkProc* process, MkProcEntryFn entry);
void trial_register_attack(
    int player, unsigned char type, unsigned char value);
static void increment_required_moves_progress(
    KonquestRequiredSequence* sequence,
    KonquestRequiredSequenceList* list);
MkObj* trial_get_monk(void);
void trial_show_monk(int show);
void trial_show_text_window(
    int string_id, int style, int flags, float x, float y, float scale);
static float failed_trial_drone_wrapup(void);
static float successful_trial_drone_wrapup(void);
static float successful_trial_player_wrapup(void);
static float p_finish_countdown(void);
float p_show_text_window(void);
static float p_transform_into_monk(void);
static float p_transform_into_player(void);
static void set_prompt_items(KonquestTrialWindowPdata* unused);
static void plot_and_show_window_frame(KonquestTrialWindowPdata* unused);
static void text_window_fade_out(unsigned char ticks);
void trial_increment_state_value(
    int player, int state, int increment);
static void increment_progress_count(unsigned char increment);
static void trial_show_move_message(void);
static void show_background_box(
    int style, int x, int y, int priority, int width, int height);
static void ps_konquest_trial_monk(void);
static void pw_konquest_trial_monk(void);
static float p_finish_transform_monk(void);
static float p_finish_transform_player(void);
void become_plyr1_proc(void);
void become_plyr2_proc(void);
void face_opponent_now(void);
void blend_to_ani(void* animation, int transition, float blend);
void ani_to_blend_frame(float frame);
float p_animate(void);
float p_do_lip_synch(void);
void set_anim_script(AnimPdata* pdata, AnimScript* animation, int transition);
float getup_from_ground(void);
float switch_proc_advance_moveset(void);
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
int set_anim_script_frame(
    float frame, AnimPdata* animation,
    AnimScript* script, unsigned int flags);
int transition_to_anim_script_frame(
    float transition_frames, float frame, AnimPdata* animation,
    AnimScript* script, unsigned int flags);
void animpdata_ani_1_frame(AnimPdata* animation);
void animpdata_ani_to_frame_x(AnimPdata* animation, float frame);
void update_mkobj(void* object);
void shake_camera(int strength, float duration);
void hide_player(PlyrPdata* player, int hide_weapons);
void rotate_towards_position(const Vec* target, float max_step);
void pre_switchp(void);
void post_switchp(void);
static void start_hero_transform_effect(MkObj* object);
static float p_blood_rush(void);
typedef struct GusherStep GusherStep;
extern GusherStep heart_beat[];
MkHdr* start_gusher(
    GusherStep* definition, MkObj* fighter, MkObj* mirror, int bone,
    const Vec* velocity, const Vec* acceleration);

static inline ScreenObj* get_screen_latch(KonquestScreenLatch* latch) {
    ScreenObj* raw = latch->object;
    ScreenObj* live;

    if (raw != 0) {
        if (raw->instance == latch->instance) {
            live = raw;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    return live;
}

static inline void hide_screen_latch(KonquestScreenLatch* latch) {
    ScreenObj* object = get_screen_latch(latch);

    if (object != 0) {
        object->flag_bits.hidden = 1;
    }
}

static inline StringObj* get_string_latch(
    StringObj* string, unsigned int instance) {
    if (string != 0) {
        if (string->instance == instance) {
            return string;
        }
        string = 0;
    } else {
        string = 0;
    }
    return string;
}

static inline StringObj* get_countdown_string_latch(
    KonquestMissionState* state) {
    StringObj* raw = state->countdown_string;
    StringObj* live;

    if (raw != 0) {
        if (raw->instance == state->countdown_string_instance) {
            live = raw;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    return live;
}

static inline MkHdr* get_dialog_latch(
    MkHdr* dialog, unsigned int instance) {
    if (dialog != 0) {
        if (dialog->instance == instance) {
            return dialog;
        }
        dialog = 0;
    } else {
        dialog = 0;
    }
    return dialog;
}

static inline MkProc* get_process_latch(
    MkProc* process, unsigned int instance) {
    MkProc* live;

    if (process != 0) {
        if (process->instance == instance) {
            live = process;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    return live;
}

static inline ScreenObj* load_prompt(
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
                mk_insert((MkHdr*)object, &aproc->pdata_list_b);
            }
        }
    }
    return object;
}

static inline void place_frame_piece(
    KonquestScreenLatch* latch, int x, int y,
    float scale_x, float scale_y, int scaled) {
    ScreenObj* object = get_screen_latch(latch);

    if (object == 0) {
        return;
    }
    object->flag_bits.hidden = 0;
    if (scaled) {
        object->flag_bits.scaled = 1;
    }
    object->x = x;
    object->y = y;
    if (scaled) {
        object->scale_x = scale_x;
        object->scale_y = scale_y;
    }
}

static inline void destroy_screen_latch(KonquestScreenLatch* latch) {
    ScreenObj* object = get_screen_latch(latch);

    if (object != 0) {
        object = latch->object;
        if (object->instance != 0) {
            ((void (*)(ScreenObj*))object->vtbl->destroy)(object);
        }
        latch->object = 0;
        latch->instance = 0;
    }
}

static inline void text_window_fade_in(unsigned char ticks) {
    KonquestTrialWindowPdata* pdata =
        (KonquestTrialWindowPdata*)apdata;
    unsigned char alpha = 0;

    if (pdata != 0) {
        unsigned char target = pdata->color;
        unsigned char step = target / ticks;

        do {
            pfx_2d_obj_set_alpha_by_id(0x9002, alpha);
            if (pdata->prompt_flags & 0x10) {
                pfx_2d_obj_set_alpha_by_id(0x9004, alpha);
            }
            if (target - step < alpha) {
                alpha = target;
            } else {
                alpha += step;
            }
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        } while (alpha < target);
        pfx_2d_obj_set_alpha_by_id(0x9002, target);
        if (pdata->prompt_flags & 0x10) {
            pfx_2d_obj_set_alpha_by_id(0x9004, target);
        }
    }
}

static inline KonquestMissionState* get_mission_state(void) {
    KonquestMissionState* state = mission_state_item.state;

    if (state != 0) {
        if (state->hdr.instance == mission_state_item.instance) {
            return state;
        }
        state = 0;
    } else {
        state = 0;
    }
    return state;
}

static inline MkObj* get_mission_monk(void) {
    KonquestMissionState* state = get_mission_state();
    MkObj* raw;
    MkObj* live;

    mission_state = state;
    if (state == 0) {
        live = 0;
    } else {
        raw = state->monk;
        if (raw != 0) {
            if (raw->hdr.instance == state->monk_instance) {
                live = raw;
            } else {
                live = 0;
            }
        } else {
            live = 0;
        }
    }
    return live;
}

static inline void start_transform_particle_set(
    const char* name, MkObj* hero) {
    unsigned int emitter = fx_by_owner(name, 4);
    MkPfx* particle;
    MkObj* effect_object;

    emitter = fx_next_emitter(emitter);
    particle = pfx_from_emitter(emitter);
    effect_object = pfx_bind_emitter_num_to_new_obj(
        particle, (void*)0x6015, 0);
    get_bone_world_pos(hero, 0x10, &effect_object->pos);
    effect_object->flags_08_bits.airborne = 1;
    update_mkobj(
        effect_object != 0 ? as_mkhdr(&effect_object->hdr) : 0);
    fx_restart_emit(emitter);

    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x00, 1);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x14, 2);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x15, 3);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x18, 4);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x19, 5);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x04, 6);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x05, 7);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x0A, 8);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x0B, 9);
    fx_restart_emit(emitter);
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, hero, 0x09, 10);
    fx_restart_emit(emitter);
}

static inline char** get_trial_sign_list(void) {
    static char* sign_list[3] = {
        (char*)0x0A93000F, (char*)0x0A930010, (char*)0x0A930011
    };

    return sign_list;
}

int trial_never_passed_this_mission(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    return get_konq_profile_value(5, state->mission_index) <= 0;
}

void drone_set_handicap(int player, float handicap) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return;
    }
    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        g_game_info.plyr0.field_10 = handicap;
        if (g_game_info.plyr0.field_0C > handicap) {
            g_game_info.plyr0.field_0C = handicap;
        }
    } else {
        g_game_info.plyr1.field_10 = handicap;
        if (g_game_info.plyr1.field_0C > handicap) {
            g_game_info.plyr1.field_0C = handicap;
        }
    }
}

void drone_start_bleeding(int player, float rate) {
    KonquestBloodRushPdata* pdata;
    KonquestMissionState* state;
    MkObj* fighter;
    MkObj* mirror;
    Vec velocity = {0.0f, 0.0f, 0.0f};
    Vec direction = {1.0f, 0.0f, 0.0f};

    state = get_mission_state();
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
        heart_beat, fighter, mirror, 9, &velocity, &direction);

    if (_create_mkproc_generic_nostack(
            0x501B, 0x1F, p_blood_rush,
            sizeof(KonquestBloodRushPdata), (MkHdr**)&pdata) != 0) {
        pdata->player = ((PlyrPdata*)fighter)->plyr_num;
        pdata->rate = -1.0f * rate;
    }
}

float p_blood_rush(void) {
    KonquestBloodRushPdata* pdata =
        (KonquestBloodRushPdata*)apdata;
    int player = pdata->player;
    float rate = pdata->rate;

    if (g_game_info.flag_bits.lens_flare_enabled) {
        if (player == 0) {
            adjust_p1_life(rate);
        } else {
            adjust_p2_life(rate);
        }
    }
    return 30.0f;
}

int trial_invisible_callback(PlyrPdata* player) {
    int player_number = player->plyr_num;
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 1;
    }
    if (player_number != state->fight->animation_side &&
        (state->restriction_flags & 0x20)) {
        return 0;
    }
    return 1;
}

int trial_change_style_callback(int player) {
    KonquestMissionState* state = get_mission_state();

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

int trial_block_callback(int player) {
    KonquestMissionState* state = get_mission_state();

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

void trial_set_special_restrictions(unsigned int restrictions) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->restriction_flags = restrictions;
    }
}

float trial_damage_callback(
    int player, int damage_type, float damage) {
    int player_side = player == 0;
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return damage;
    }
    damage *= state->damage_scale[player];

    if (state->restriction_flags & 4) {
        if (player_side == state->fight->animation_side &&
            !is_weapon_style(
                (&g_game_info.plyr0)[player_side]
                    .slot.pdata->fighter_definition)) {
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

void show_text(
    int font, unsigned int color, unsigned int string_id,
    unsigned int prompt_flags, int duration,
    float x, float y, float scale) {
    KonquestTrialWindowPdata* pdata;

    if (_create_mkproc_generic_bigstack(
            0x9002, aproc->priority + 1, p_show_text_window,
            sizeof(*pdata), (MkHdr**)&pdata) != 0) {
        zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
        pdata->visible_item_count = 0;
        pdata->top = 0;
        pdata->left = (int)((float)screen_width * x);
        pdata->bottom = (int)(480.0f - (480.0f * y));
        pdata->priority = 4;
        pdata->width = (int)((float)screen_width * scale);
        pdata->prompt_flags = prompt_flags;
        pdata->font = font;
        pdata->color = color;
        pdata->button_charmap = 0;
        pdata->swap_buttons = 0;
        pdata->art_slot = 0x2001E;
        if (prompt_flags & 0x400) {
            duration = (int)((float)duration * inverse_game_speed);
        }
        pdata->timeout = duration - 0x18;
        strcpy(pdata->text, get_string_by_id(string_id));
        text_window_state = 0;
        while (text_window_state == 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
    }
}

void trial_debug_mission_list(void) {
}

int trial_get_background_root(void) {
    return (int)konquest_save_data.background_and_flags >> 16;
}

void give_koin_award(int amount, int type) {
    static const char* mission_complete[6] = {
        "MISSION COMPLETE", "MISI\323N COMPLETA", "AUFTRAG ERFOLGREICH",
        "MISSION ACCOMPLIE", "MISSIONE COMPIUTA", "MISSION COMPLETE"
    };
    static int koin_list[6] = {
        0x0A930013, 0x0A930014, 0x0A930015,
        0x0A930016, 0x0A930017, 0x0A930018
    };
    char amount_text[8];
    ScreenObj* sign;
    StringObj* title;
    ScreenObj* koin;
    StringObj* amount_string;
    unsigned char alpha;

    duck_sounds(0.5f);
    _mkproc_sleep_ticks = 15.0f;
    aproc->vtbl->sleep();
    snd_req(0x15EC);
    add_to_konq_profile_value(type + 7, amount);
    sign = load_2d_pfxobj_xy(
        0x2001E, 0x9008, (char*)0x0A930012, 0,
        screen_width / 2 - 0x80, 0x66, 0x24);
    title = create_wrapped_string(
        0x9008, load_font(3),
        mission_complete[get_language_setting()],
        screen_width / 2 - 0x71, 0x138, 0xE2, 0, 1, 0);
    title->priority = 0x22;
    insert_string_obj((ScreenObj*)title);
    if (type > 6) {
        type = 2;
    }
    koin = load_2d_pfxobj_xy(
        0x2001E, 0x9008, (char*)koin_list[type], 0,
        screen_width / 2 - 0x40, 0xB6, 0x23);
    load_font(3);
    if (amount > 99999) {
        amount = 99999;
    }
    sprintf(amount_text, "%d", amount);
    amount_string = string_center_xy(
        0x9008, 3, amount_text,
        screen_width / 2 + 0x17, 0xC9, 0x23);

    if (sign != 0) {
        alpha = 0;
        while (alpha < 0xF0) {
            pfx_2d_obj_set_alpha_by_id(0x9008, alpha);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            alpha += 8;
        }
        pfx_2d_obj_set_alpha_by_id(0x9008, 0xFF);
        _mkproc_sleep_ticks = 120.0f;
        aproc->vtbl->sleep();
        alpha = 0xF0;
        while (alpha != 0) {
            pfx_2d_obj_set_alpha_by_id(0x9008, alpha);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            alpha -= 8;
        }
        if (sign->instance != 0) {
            ((int (*)(ScreenObj*))sign->vtbl->destroy)(sign);
        }
        if (koin->instance != 0) {
            ((int (*)(ScreenObj*))koin->vtbl->destroy)(koin);
        }
        if (amount_string->instance != 0) {
            ((int (*)(StringObj*))amount_string->vtbl->destroy)(amount_string);
        }
        if (title->instance != 0) {
            ((int (*)(StringObj*))title->vtbl->destroy)(title);
        }
    }
    duck_sounds(1.0f);
}

void konquest_run_ending(void) {
    fade_to_black(8, 1);
    gamelogic_jump(2, p_konquest_ending);
}

void trial_set_round_health_restoration(float restoration) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        if (restoration < -1.0 || restoration > 1.0) {
            restoration = 1.0f;
        }
        state->round_health_restoration = restoration;
    }
}

void cleanup_mission_state(void) {
    mission_state = 0;
    mission_state_item.state = 0;
    mission_state_item.instance = 0;
    current_anim_pdata = 0;
}

void trial_mirror_anims_if_needed(void) {
    if (mission_state->fight->animation_side == 1) {
        camera_set_animation_mirror_plane(3);
    }
}

int konquest_passed_last_mission(void) {
    return konquest_save_data.passed_last_mission;
}

void drone_set_anim_step(float step) {
    if (current_anim_pdata != 0) {
        current_anim_pdata->step = step;
    }
}

void trial_end_tunes(void) {
    KonquestMissionState* state = get_mission_state();
    int music;

    mission_state = state;
    if (state == 0) {
        return;
    }
    if (!(state->display_flags & 1)) {
        return;
    }
    music = state->tune_table->entries[state->randomized_side].end_music;
    if (music >= 0) {
        if (bgnd_music_ptr1 != 0) {
            snd_stop(bgnd_music_ptr1);
        }
        bgnd_music_ptr1 = snd_req(music);
    }
}

void trial_start_tunes(void) {
    int music = 0;
    KonquestMissionState* state = get_mission_state();
    unsigned int function;

    mission_state = state;
    if (state != 0) {
        function = check_script_function_exists(
            state->tick_script, "use_scene_music");
        if (function != 0) {
            cmdscript_setup_execution(mission_state->tick_script, function);
            cmdscript_execute(mission_state->tick_script);
            music = active_cmdscript->regs[0];
        }

        if (music != 0) {
            music = mission_state->tune_table->script_music;
            if (music >= 0) {
                if (bgnd_music_ptr1 != 0) {
                    snd_stop(bgnd_music_ptr1);
                }
                bgnd_music_ptr1 = snd_req(music);
            }
        } else if (mission_state->display_flags & 1) {
            if (g_game_info.pselect.field_1f4 == 1) {
                mission_state->randomized_side = (unsigned short)randu0(2);
                music = mission_state->tune_table
                    ->entries[mission_state->randomized_side].round_one_music;
                if (music >= 0) {
                    if (bgnd_music_ptr1 != 0) {
                        snd_stop(bgnd_music_ptr1);
                    }
                    bgnd_music_ptr1 = snd_req(music);
                }
            } else {
                music = mission_state->tune_table
                    ->entries[mission_state->randomized_side].later_round_music;
                if (music >= 0) {
                    if (bgnd_music_ptr1 != 0) {
                        snd_stop(bgnd_music_ptr1);
                    }
                    bgnd_music_ptr1 = snd_req(music);
                }
            }
        } else if (g_game_info.pselect.field_1f4 == 1) {
            mission_state->randomized_side = (unsigned short)randu0(3);
            music = mission_state->tune_table
                ->basic_round_one_music[mission_state->randomized_side];
            if (music >= 0) {
                if (bgnd_music_ptr1 != 0) {
                    snd_stop(bgnd_music_ptr1);
                }
                bgnd_music_ptr1 = snd_req(music);
            }
        }
    }
}

void play_background_music(int sound) {
    if (sound >= 0) {
        if (bgnd_music_ptr1 != 0) {
            snd_stop(bgnd_music_ptr1);
        }
        bgnd_music_ptr1 = snd_req(sound);
    }
}

void trial_setup_nis_scene(int setup) {
    Vec monk_position = {1.0f, 0.0f, 6.0f};
    Vec monk_angle = {0.0f, 3.1415927f, 0.0f};
    CamVec3 camera_position = {0.0f, 1.5478f, 3.129f};
    CamVec3 camera_angle = {0.0f, 3.1415927f, 0.0f};

    if (mission_state->fight ==
        (KonquestMissionFightInfo*)&g_game_info.plyr1) {
        monk_position.x = -1.0f;
    }

    if (setup != 0) {
        KonquestMissionState* state = get_mission_state();
        MkObj* raw;
        MkObj* monk;

        mission_state = state;
        if (state == 0) {
            monk = 0;
        } else {
            raw = state->monk;
            if (raw != 0) {
                if (raw->hdr.instance == state->monk_instance) {
                    monk = raw;
                } else {
                    monk = 0;
                }
            } else {
                monk = 0;
            }
        }

        if (monk != 0) {
            monk->hide_flag_bits.pin_animation = 0;
            monk->pos.x = monk_position.x;
            monk->pos.y = monk_position.y;
            monk->pos.z = monk_position.z;
            monk->ang.x = monk_angle.x;
            monk->ang.y = monk_angle.y;
            monk->ang.z = monk_angle.z;
            monk->hide_flag_bits.hidden = 0;
        }
        set_camera_position(&camera_position);
        set_camera_angle(&camera_angle);
        start_mkpfx_FadeSnapShot();
        move_plyrs_to_round_start();
        xfer_player_proc((MkProc*)g_game_info.plyr0.idle_proc, j_exit);
        xfer_player_proc((MkProc*)g_game_info.plyr1.idle_proc, j_exit);
    }
    xfer_camera(p_idle, 0);
}

void trial_restart_round(void) {
    MkObj* monk;

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
    monk = get_mission_monk();
    if (monk != 0) {
        monk->hide_flag_bits.hidden = 1;
    }
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
    if (plyr_pdata == drone) {
        return 1;
    }
    return 0;
}

void drone_lip_synch(int sound_id, LipSyncKeyframe* keyframes) {
    KonquestLipSyncPdata* lip;
    MkProc* process = _create_mkproc_generic_nostack(
        0x8232, 0x1F, p_do_lip_synch, sizeof(*lip), (MkHdr**)&lip);

    if (process == 0) {
        return;
    }
    zero_pdata_payload(sizeof(*lip), &lip->hdr);
    lip->sound_handle = sound_id;
    lip->keyframes = keyframes;

    if (aproc->pid == 0x9007) {
        KonquestMissionState* state;
        KonquestLipSyncPdata* target;

        mission_state = get_mission_state();
        if (mission_state == 0) {
            return;
        }
        lip->mode = 2;
        state = mission_state;
        target = lip;
        target->texture = state->monk_face_texture;
        target->texture_instance = state->monk_face_texture_instance;
        return;
    }

    if (aproc->pid == 0x1001 || aproc->pid == 0x1002) {
        AnimPdata* animation;

        process = get_process_latch(
            plyr_pdata->face_anim_proc,
            plyr_pdata->face_anim_proc_instance);
        if (process == 0) {
            return;
        }
        animation = (AnimPdata*)pdata_of_proc(process);
        if (animation != 0) {
            set_anim_script(animation, plyr_pdata->face_animations[0], 3);
            animation->step = 1.0f;
            animation->transition_step = 0.2f;
            animation->hand_transition = 1.0f;
            xfer_proc(process, p_animate);
        }
        lip->mode = 3;
        lip->player_animation = animation;
        lip->animation_table = plyr_pdata->face_animations;
        return;
    }

    if (process->instance != 0) {
        process->vtbl->destroy(process);
    }
}

void trial_register_special_move(unsigned int move) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0 && mode_of_play == 8) {
        if (g_game_info.flag_bits.lens_flare_enabled) {
            if ((aproc->pid == 0x1001 || aproc->pid == 0x1002) &&
                plyr_pdata != 0) {
                trial_register_attack(plyr_pdata->plyr_num, 3, move);
            }
        }
    }
}

void trial_register_script_function(unsigned int function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0 && mode_of_play == 8) {
        if (g_game_info.flag_bits.lens_flare_enabled) {
            if ((aproc->pid == 0x1001 || aproc->pid == 0x1002) &&
                plyr_pdata != 0) {
                trial_register_attack(
                    plyr_pdata->plyr_num,
                    (unsigned char)plyr_pdata->player_slot, function);
            }
        }
    }
}

int trial_show_standard_fight_messages(void) {
    return mission_state->display_flags & 1;
}

void set_prompt_items(KonquestTrialWindowPdata* unused) {
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
            object->flag_bits.hidden = 0;
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
            object->flag_bits.hidden = 0;
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
            object->flag_bits.hidden = 0;
            object->x = pdata->left;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
    if (pdata->prompt_flags & 8) {
        object = load_prompt(
            pdata, &pdata->prompt_cancel, "RETURN_BUTTON", 0);
        if (object != 0) {
            object->flag_bits.hidden = 0;
            object->x = pdata->left;
            object->y = y;
            pdata->visible_item_count++;
        }
    }
}

void plot_and_show_window_frame(KonquestTrialWindowPdata* unused) {
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
    font_height = (int)get_font_height(pdata->font);
    left = pdata->left - 0xF;
    right = pdata->left + pdata->width + 0xF;
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

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static void text_window_fade_out(unsigned char ticks) {
    KonquestTrialWindowPdata* pdata =
        (KonquestTrialWindowPdata*)apdata;
    unsigned char alpha = 0xFF;
    unsigned char alpha_step = 0xFF / ticks;
    ScreenObj* object;
    int index;

    if (pdata != 0) {
        while (ticks != 0) {
            pfx_2d_obj_set_alpha_by_id(0x9002, alpha);
            if (pdata->prompt_flags & 0x10) {
                pfx_2d_obj_set_alpha_by_id(0x9004, alpha);
            }
            _mkproc_sleep_ticks = 1.0f;
            alpha -= alpha_step;
            ticks--;
            aproc->vtbl->sleep();
        }
        if (pdata->prompt_flags & 0x10) {
            pfx_2d_obj_set_alpha_by_id(0x9004, 0xFF);
            for (index = 0; index < 9; index++) {
                hide_screen_latch(&pdata->frame[index]);
            }
            object = get_screen_latch(&pdata->prompt_ok);
            if (object != 0) {
                object->flag_bits.hidden = 1;
            }
            object = get_screen_latch(&pdata->prompt_retry);
            if (object != 0) {
                object->flag_bits.hidden = 1;
            }
            object = get_screen_latch(&pdata->prompt_548);
            if (object != 0) {
                object->flag_bits.hidden = 1;
            }
            object = get_screen_latch(&pdata->prompt_cancel);
            if (object != 0) {
                object->flag_bits.hidden = 1;
            }
            object = get_screen_latch(&pdata->prompt_550);
            if (object != 0) {
                object->flag_bits.hidden = 1;
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

float p_show_text_window(void) {
    KonquestTrialWindowPdata* pdata =
        (KonquestTrialWindowPdata*)apdata;
    KonquestTrialWindowPdata* window = pdata;
    int result = 0;

    if ((pdata->prompt_flags & 0x10) && pdata != 0) {
        union {
            unsigned int word;
            struct {
                ScreenObjFlags bits;
                unsigned char pad[3];
            } bytes;
        } frame_flags;
        int index;

        frame_flags.word = 0;
        frame_flags.bytes.bits.hidden = 1;
        for (index = 0; index < 9; index++) {
            ScreenObj* object = load_2d_pfxobj(
                pdata->art_slot, 0x9004,
                (char*)(unsigned long)(
                    get_artid_of_named_item_in_slot(
                        pdata->art_slot, "JNY_WINDOW01", 1) + index),
                frame_flags.word, pdata->priority);

            pdata->frame[index].object = object;
            pdata->frame[index].instance = object->instance;
            mk_insert((MkHdr*)object, &aproc->pdata_list_b);
        }
    }

    pdata = (KonquestTrialWindowPdata*)apdata;
    if (pdata != 0) {
        int alignment;
        StringObj* string;
        unsigned char color[4];
        unsigned int packed_color;

        if (pdata->prompt_flags & 0x800) {
            alignment = 1;
        } else if (pdata->prompt_flags & 0x1000) {
            alignment = 2;
        } else {
            alignment = 0;
        }
        string = create_wrapped_string(
            0x9002, load_font(pdata->font), pdata->text,
            pdata->left, pdata->bottom, pdata->width,
            0, alignment, 2);
        string->priority = pdata->priority - 2;
        packed_color = pdata->color;
        color[0] = packed_color >> 24;
        color[1] = packed_color >> 16;
        color[2] = packed_color >> 8;
        color[3] = packed_color;
        pfxfont_set_string_color(
            &string->pfx, (unsigned int*)color);
        pdata->top = string->text_h;
        mk_insert((MkHdr*)string, &aproc->pdata_list_b);
        insert_2d_obj((ScreenObj*)string);
    }

    set_prompt_items(window);
    if (window->prompt_flags & 0x10) {
        plot_and_show_window_frame(window);
        push_game_state(0x15);
    }

    if (window->prompt_flags & 0x20) {
        text_window_fade_in(30);
    } else if (window->prompt_flags & 0x100) {
        text_window_fade_in(10);
    }

    while (result == 0) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if (mcard_msg_active == 0) {
            if (((window->prompt_flags & 1) ||
                 (window->prompt_flags & 4)) &&
                check_switch_edge(window->controller_port, 6)) {
                result = 1;
                snd_req(0x15ED);
            }
            if ((window->prompt_flags & 0x80) &&
                check_switch_edge(window->controller_port, 6)) {
                result = 1;
                snd_req(0x15ED);
            }
            if (((window->prompt_flags & 8) ||
                 (window->prompt_flags & 2)) &&
                check_switch_edge(window->controller_port, 7)) {
                result = 2;
                snd_req(0x15ED);
            }
            if (window->timeout > 0) {
                window->timeout--;
            }
            if (window->timeout == 0) {
                result = 3;
            }
        }
    }

    if (window->prompt_flags & 0x10) {
        pop_game_state();
    }
    if (window->prompt_flags & 0x40) {
        text_window_fade_out(30);
    } else if (window->prompt_flags & 0x200) {
        text_window_fade_out(10);
    }

    destroy_screen_latch(&window->prompt_ok);
    destroy_screen_latch(&window->prompt_retry);
    destroy_screen_latch(&window->prompt_548);
    destroy_screen_latch(&window->prompt_550);
    destroy_screen_latch(&window->prompt_cancel);
    text_window_state = result;
    return -1.0f;
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

int trial_get_drone_difficulty(void) {
    KonquestMissionState* state = get_mission_state();
    int completed;
    int index;

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    if (state->drone_difficulty == 9) {
        completed = 0;
        for (index = 0; index < 0x1C2; index++) {
            if (p1_profile_konquest[index + 0x6C] != 0) {
                completed++;
            }
        }
        state->drone_difficulty =
            (int)(7.0f * ((float)completed / 106.0f) + 0.5f);
    }
    return mission_state->drone_difficulty;
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
        mission_state->display_item_a = enabled;
        mission_state->display_format = format;
    }
}

void trial_start_countdown(int countdown, float x, float y) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        char text[2];
        int screen_x;
        int screen_y;
        int change;
        StringObj* string;

        text[0] = (char)countdown + '0';
        text[1] = '\0';
        screen_x = (int)((float)screen_width * x);
        screen_y = (int)(480.0f * y);
        change = -countdown;
        load_font(1);
        destroy_mkprocs_pid(0x900D);

        string = get_countdown_string_latch(mission_state);
        if (string != 0) {
            string = mission_state->countdown_string;
            if (string->instance != 0) {
                ((int (*)(StringObj*))string->vtbl->destroy)(string);
            }
            mission_state->countdown_string = 0;
            mission_state->countdown_string_instance = 0;
        }

        string = string_center_xy(
            0x9005, 1, text, screen_x, screen_y, 0x22);
        if (string != 0) {
            mission_state->countdown_string = string;
            mission_state->countdown_string_instance = string->instance;
            display_numerical_change(
                string, 1, countdown, change, 60, 0x98967F,
                mission_state);
        }

        if (_create_mkproc_generic_tinystack(
                0x900D, 0x1F, p_finish_countdown, 0, 0) == 0) {
            string = get_countdown_string_latch(mission_state);
            if (string != 0) {
                string = mission_state->countdown_string;
                if (string->instance != 0) {
                    ((int (*)(StringObj*))string->vtbl->destroy)(string);
                }
                mission_state->countdown_string = 0;
                mission_state->countdown_string_instance = 0;
            }
        }
    }
}

static float p_finish_countdown(void) {
    StringObj* string;

    aproc->flags_bits.use_game_speed = 1;
    string = get_countdown_string_latch(mission_state);
    if (string != 0) {
        update_string_obj(string, 1, "0");
        _mkproc_sleep_ticks = 60.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }

    {
        KonquestMissionState* state = mission_state;

        string = get_countdown_string_latch(state);
        if (string != 0) {
            string = state->countdown_string;
            if (string->instance != 0) {
                ((int (*)(StringObj*))string->vtbl->destroy)(string);
            }
            mission_state->countdown_string = 0;
            mission_state->countdown_string_instance = 0;
        }
    }
    return -1.0f;
}

void trial_do_dialog(
    int unused, int string_id,
    float unused_x, float unused_y, float unused_scale,
    unsigned int ticks, int wait) {
    MkHdr* dialog;
    unsigned int instance;
    int scaled_ticks = (int)((float)ticks * inverse_game_speed);

    dialog = konquest_set_dialog_text(
        get_string_by_id(string_id), 0);
    calc_print_speed_for_nis_dialog(dialog, scaled_ticks);
    if (dialog != 0) {
        instance = dialog->instance;
        if (wait != 0) {
            while (get_dialog_latch(dialog, instance) != 0) {
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
            }
        }
    }
}

void trial_show_spoken_text_window(
    int string_id, float x, float y, float scale,
    int style, int sound, int unused0, int unused1, int flags) {
    MslSoundHandle voice = 0;

    if (sound >= 0) {
        voice = snd_req(sound);
    }
    trial_show_text_window(string_id, style, flags, x, y, scale);
    if (voice != 0) {
        snd_stop(voice);
    }
}

void trial_show_text_window(
    int string_id, int style, int flags, float x, float y, float scale) {
    KonquestMissionState* state;
    KonquestTrialWindowPdata* pdata;
    const char* text;
    int swap_buttons = 0;
    int drone_is_right;

    state = get_mission_state();
    mission_state = state;
    if (state == 0) {
        return;
    }

    text = get_string_by_id(style);
    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        drone_is_right = is_a_to_the_right_of_b(
            state->fight->active_object, state->drone->active_object);
    } else {
        drone_is_right = 0;
    }
    if (drone_is_right != 0) {
        swap_buttons = 1;
    }

    if (_create_mkproc_generic_bigstack(
            0x9002, aproc->priority + 1, p_show_text_window,
            sizeof(*pdata), (MkHdr**)&pdata) != 0) {
        int window_width;
        int zero;

        zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
        window_width = screen_width;
        zero = 0;
        text_window_state = zero;
        pdata->visible_item_count = zero;
        pdata->top = zero;
        pdata->left = (int)((float)window_width * x);
        pdata->bottom = (int)(480.0f - (480.0f * y));
        pdata->priority = 0x24;
        pdata->width = (int)((float)window_width * scale);
        pdata->prompt_flags = flags | 0x10;
        pdata->font = 6;
        pdata->timeout = -1;
        pdata->color = string_id;
        pdata->controller_port = mission_state->fight->field_0x0;
        pdata->button_charmap = danton20_charmap;
        pdata->swap_buttons = swap_buttons;
        pdata->art_slot = 0x2001E;
        strcpy(pdata->text, text);
        if (pdata->button_charmap != 0) {
            rewrite_button_string(
                danton20_charmap, pdata->text, swap_buttons,
                (int*)&g_game_info.pads[pdata->controller_port].edge);
        }
        set_default_switch_maps();
        while (text_window_state == 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        set_game_switch_maps();
    }
}

void trial_set_ending_functions(
    int winner_function, int loser_function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->winner_end_function = winner_function;
        mission_state->loser_end_function = loser_function;
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

static float p_flip_move_description(void) {
    KonquestMissionState* state;
    int flipped;

    if (mission_state->display_item_c == 0) {
        return -1.0f;
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
    return 1.0f;
}

void drone_set_special_directions(int unused, int flags) {
    DroneOverrideInfo info;

    info.flags = flags;
    info.likelihood_scale = (flags & 1) ? 0.0f : 1.0f;
    setDroneOverrideSwitch(unused, &info);
}

void drone_set_difficulty_level(int difficulty) {
    KonquestMissionFightInfo* drone = mission_state->drone;
    FighterMirror* fighter = drone->fighter;
    MkProc* idle_process;
    int timeout = 0xF0;

    if (mission_state->fight->animation_side == 1) {
        idle_process = (MkProc*)g_game_info.plyr0.idle_proc;
    } else {
        idle_process = (MkProc*)g_game_info.plyr1.idle_proc;
    }
    if (idle_process != 0) {
        while (timeout != 0 && idle_process->entry == getup_from_ground) {
            _mkproc_sleep_ticks = 1.0f;
            timeout--;
            ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
        }
    }
    if (difficulty == 8) {
        if (drone->player_state == 0) {
            fighter->field_2C8 = 1;
        }
        drone->player_state = 3;
    } else if (difficulty != 8) {
        if (drone->player_state == 3) {
            fighter->field_2C8 = 1;
        }
        drone->player_state = 0;
    }
    mission_state->drone_difficulty = difficulty;
}

void trial_set_next_setup_function(int setup_function) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->next_setup_function = setup_function;
        mission_state->display_flags |= 4;
    }
}

void drone_blend_to_ani(
    AnimScript* animation, int transition, float blend) {
    transition_to_anim_script(
        blend, current_anim_pdata, animation, transition);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
}

void drone_face_monk(void) {
    MkObj* monk = get_mission_monk();

    if (monk != 0) {
        rotate_towards_position(&monk->pos, 0.05f);
    }
}

void trial_show_monk(int show) {
    MkObj* monk = get_mission_monk();

    if (monk != 0) {
        monk->hide_flag_bits.hidden = show == 0;
    }
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

void drone_set_script(int player, int script_function) {
    KonquestMissionState* state = get_mission_state();
    MkProc* process;
    CmdScript* script;

    mission_state = state;
    if (state == 0) {
        return;
    }
    if (player == 2) {
        process = get_process_latch(
            state->script_process, state->script_process_instance);
        if (process != 0) {
            script = get_cmdscript_for_proc(process);
            script->unk28 = script_function;
            xfer_proc(process, (MkProcEntryFn)call_mission_script);
        }
        return;
    }

    if ((player == 0 && state->fight->animation_side == 0) ||
        (player == 1 && state->fight->animation_side == 1)) {
        process = (MkProc*)g_game_info.plyr0.idle_proc;
        script = get_cmdscript_for_proc(process);
        script->unk28 = script_function;
        xfer_player_proc(process, (MkProcEntryFn)call_mission_script);
        return;
    }
    process = (MkProc*)g_game_info.plyr1.idle_proc;
    script = get_cmdscript_for_proc(process);
    script->unk28 = script_function;
    xfer_player_proc(process, (MkProcEntryFn)call_mission_script);
}

static float call_mission_script(void) {
    if ((unsigned int)active_cmdscript->unk28 != 0) {
        cmdscript_setup_execution(
            mission_state->tick_script,
            active_cmdscript->unk28);
        cmdscript_execute(mission_state->tick_script);
    }
    if (aproc->pid == 0x1001 || aproc->pid == 0x1002) {
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(j_exit, 0.0f);
        return 0.0f;
    }
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    }
}

void drone_apply_damage(int player, float damage) {
    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        adjust_p1_life(damage);
        return;
    }
    adjust_p2_life(damage);
}

void drone_set_health(int player, float health) {
    float* current_health;

    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        current_health = &g_game_info.plyr0.field_0C;
    } else {
        current_health = &g_game_info.plyr1.field_0C;
    }
    *current_health = health;
}

void drone_set_position(
    int player, float x, float y, float z) {
    KonquestMissionFightInfo* fighter;
    Vec position;

    position.x = x;
    position.y = y;
    position.z = z;
    if (mission_state->fight->animation_side == 1) {
        position.x = -position.x;
        position.z = -position.z;
    }
    fighter = player == 0 ? mission_state->fight : mission_state->drone;
    move_player(
        fighter->active_object, &position,
        &fighter->active_object->ang);
    force_midpoint_calculation_update = 1;
}

void drone_change_to_style(int player, int style) {
    KonquestMissionFightInfo* info;
    FighterMirror* fighter;
    MkProc* idle_process;
    int timeout;
    int attempts;

    attempts = 0;
    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        info = mission_state->fight;
    } else {
        info = mission_state->drone;
    }
    fighter = info->fighter;
    timeout = 0xF0;
    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        idle_process = (MkProc*)g_game_info.plyr0.idle_proc;
    } else {
        idle_process = (MkProc*)g_game_info.plyr1.idle_proc;
    }
    if (idle_process != 0) {
        while (timeout != 0 && idle_process->entry == getup_from_ground) {
            _mkproc_sleep_ticks = 1.0f;
            timeout--;
            ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
        }
    }
    while (fighter->style_idx != style && attempts < 5) {
        KonquestSwitchPdata* pdata;
        MkProc* process = _create_mkproc_generic_tinystack(
            0x3002, 6, switch_proc_advance_moveset,
            sizeof(KonquestSwitchPdata), (MkHdr**)&pdata);

        if (process != 0) {
            process->pre_destroy = pre_switchp;
            process->destroy_cb = post_switchp;
            pdata->player = (PlyrInfo*)info;
        }
        _mkproc_sleep_ticks = 15.0f;
        ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
        attempts++;
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

static float p_run_special_move(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
    return -1.0f;
}

void drone_dispatch_switches(int player) {
    SwitchMapEntry* switch_map;
    PlyrInfo* player_info;
    unsigned int switch_state;
    unsigned int mask;
    int index;

    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        switch_map = g_game_info.pads[0].switch_map;
        player_info = &g_game_info.plyr0;
        switch_state = mission_state->player_one_switch_state;
    } else {
        switch_map = g_game_info.pads[1].switch_map;
        player_info = &g_game_info.plyr1;
        switch_state = mission_state->player_two_switch_state;
    }
    mask = 1;
    for (index = 0; index < 16; index++, mask <<= 1) {
        if ((switch_state & mask) != 0) {
            KonquestSwitchPdata* pdata;
            MkProc* process = _create_mkproc_generic_tinystack(
                0x3002, 6, switch_map[index].proc_fn,
                sizeof(KonquestSwitchPdata), (MkHdr**)&pdata);

            if (process != 0) {
                process->pre_destroy = pre_switchp;
                process->destroy_cb = post_switchp;
                pdata->player = player_info;
            }
        }
    }
}

void drone_set_switch_state(int player, unsigned int switch_state) {
    MkObj* first;
    MkObj* second;
    unsigned int* stored_state;

    if ((player == 0 && mission_state->fight->animation_side == 0) ||
        (player == 1 && mission_state->fight->animation_side == 1)) {
        first = g_game_info.plyr0.slot.mirror_a;
        stored_state = &mission_state->player_one_switch_state;
        second = g_game_info.plyr1.slot.mirror_a;
    } else {
        first = g_game_info.plyr1.slot.mirror_a;
        stored_state = &mission_state->player_two_switch_state;
        second = g_game_info.plyr0.slot.mirror_a;
    }
    if (is_a_to_the_right_of_b(first, second) != 0) {
        *stored_state = ((switch_state * 4) & 0x8000) |
            ((switch_state & 0xFFFF5FFF) |
             ((switch_state >> 2) & 0x2000));
        return;
    }
    *stored_state = switch_state;
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

#pragma dont_inline on
static void increment_progress_count(unsigned char increment) {
    StringObj* string;
    char text[48];

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
    if (string != 0) {
        if (string->instance != mission_state->progress_string_instance) {
            string = 0;
        }
    } else {
        string = 0;
    }
    if (string != 0) {
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
#pragma dont_inline reset

static void trial_show_move_message(void) {
    StringObj* string;
    char text[48];
    int index;

    if (mission_state->display_item_c == 0) {
        return;
    }
    if (mission_state->move_message != 0) {
        string = get_string_latch(
            mission_state->move_string,
            mission_state->move_string_instance);
        if (string != 0) {
            update_string_obj(string, 6, mission_state->move_message);
        } else {
            string = string_left_xy(
                0x9007, 6, mission_state->move_message,
                0x28, 0x16E, 0x22);
            if (string != 0) {
                mission_state->move_string = string;
                mission_state->move_string_instance = string->instance;
            }
        }
        if (*mission_state->move_message != ' ') {
            show_background_box(
                1, 0x1E, 0x16C, 0x24,
                string->text_w + 0x14, 0x1A);
        } else {
            for (index = 0; index < 5; index++) {
                hide_screen_latch(
                    &mission_state->background_boxes[1].pieces[index]);
            }
        }
    }

    if (mission_state->move_message_param != 0) {
        strcpy(text, mission_state->move_message_param);
        rewrite_button_string(
            movelist_charmap, text,
            mission_state->move_description_flipped,
            (int*)g_game_info
                .pads[mission_state->fight->field_0x0].switch_map);
        string = get_string_latch(
            mission_state->move_param_string,
            mission_state->move_param_string_instance);
        if (string != 0) {
            update_string_obj(string, 7, text);
        } else {
            string = string_right_xy(
                0x9007, 7, text, screen_width - 0x28,
                0x16C, 0x22);
            if (string != 0) {
                mission_state->move_param_string = string;
                mission_state->move_param_string_instance = string->instance;
            }
        }
        if (*mission_state->move_message_param != ' ') {
            show_background_box(
                0, (screen_width - 0x32) - string->text_w,
                0x166, 0x24, string->text_w + 0x14, 0x22);
        } else {
            for (index = 0; index < 5; index++) {
                hide_screen_latch(
                    &mission_state->background_boxes[0].pieces[index]);
            }
        }
    }
}

static void show_background_box(
    int style, int x, int y, int priority, int width, int height) {
    KonquestBackgroundBox* box =
        &mission_state->background_boxes[style];
    ScreenObj* object;

    object = get_screen_latch(&box->fill);
    if (object == 0) {
        object = load_2d_pfxobj(
            0x2001E, 0x9006, (char*)0x0A93000D, 0, priority);
        if (object != 0) {
            box->fill.object = object;
            box->fill.instance = object->instance;
        }
    }
    if (object != 0) {
        object->x = x + 1;
        object->y = y + 1;
        object->flag_bits.scaled = 1;
        object->flag_bits.hidden = 0;
        object->scale_x = (float)(width - 2);
        object->scale_y = (float)(height - 2);
        pfx_2d_obj_set_alpha(object, 0xA5);
    }

    object = get_screen_latch(&box->top);
    if (object == 0) {
        object = load_2d_pfxobj(
            0x2001E, 0x9006, (char*)0x0A93000E, 0, priority);
        if (object != 0) {
            box->top.object = object;
            box->top.instance = object->instance;
        }
    }
    if (object != 0) {
        object->x = x;
        object->y = y;
        object->flag_bits.scaled = 1;
        object->flag_bits.hidden = 0;
        object->scale_x = (float)width;
        object->scale_y = 1.0f;
        pfx_2d_obj_set_alpha(object, 0xA5);
    }

    object = get_screen_latch(&box->bottom);
    if (object == 0) {
        object = load_2d_pfxobj(
            0x2001E, 0x9006, (char*)0x0A93000E, 0, priority);
        if (object != 0) {
            box->bottom.object = object;
            box->bottom.instance = object->instance;
        }
    }
    if (object != 0) {
        object->x = x;
        object->y = y + height;
        object->flag_bits.scaled = 1;
        object->flag_bits.hidden = 0;
        object->scale_x = (float)width;
        object->scale_y = 1.0f;
        pfx_2d_obj_set_alpha(object, 0xA5);
    }

    object = get_screen_latch(&box->left);
    if (object == 0) {
        object = load_2d_pfxobj(
            0x2001E, 0x9006, (char*)0x0A93000E, 0, priority);
        if (object != 0) {
            box->left.object = object;
            box->left.instance = object->instance;
        }
    }
    if (object != 0) {
        object->x = x;
        object->y = y;
        object->flag_bits.scaled = 1;
        object->flag_bits.hidden = 0;
        object->scale_x = 1.0f;
        object->scale_y = (float)height;
        pfx_2d_obj_set_alpha(object, 0xA5);
    }

    object = get_screen_latch(&box->right);
    if (object == 0) {
        object = load_2d_pfxobj(
            0x2001E, 0x9006, (char*)0x0A93000E, 0, priority);
        if (object != 0) {
            box->right.object = object;
            box->right.instance = object->instance;
        }
    }
    if (object != 0) {
        object->x = x + width;
        object->y = y;
        object->flag_bits.scaled = 1;
        object->flag_bits.hidden = 0;
        object->scale_x = 1.0f;
        object->scale_y = (float)height;
        pfx_2d_obj_set_alpha(object, 0xA5);
    }
}

void trial_set_move_message(
    const char* message, const char* parameter) {
    mission_state->move_message = message;
    mission_state->move_message_param = parameter;
}

void trial_clear_provision(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0 && plyr_obj == state->fight->active_object) {
        state->condition_active = 0;
    }
}

void trial_register_combo(
    int player, int hit_count, PlyrPdata* pdata, float damage) {
    KonquestMissionState* state = get_mission_state();
    KonquestMissionFightInfo* fight;
    int complete;

    mission_state = state;
    if (state == 0) {
        return;
    }
    if (mode_of_play != 8) {
        return;
    }
    fight = state->fight;
    if (fight == 0) {
        return;
    }
    if (player == fight->animation_side) {
        return;
    }
    if (g_game_info.flag_bits.lens_flare_enabled) {
        if (state->trial_type == 3) {
            complete = 0;
            if (state->combo_damage > 0.0f && damage >= state->combo_damage) {
                complete = 1;
            }
            if (state->combo_hits > 0) {
                if (hit_count >= state->combo_hits) {
                    complete = 1;
                } else {
                    complete = 0;
                }
            }
            state->combo_complete = complete;
            return;
        }
        if (state->trial_type != 1) {
            return;
        }
        if (player != fight->animation_side) {
            KonquestRequiredSequenceList* list =
                (KonquestRequiredSequenceList*)&state->combo_hits;
            KonquestRequiredSequence* sequence =
                &list->entries[list->current_sequence];

            if (sequence != 0) {
                sequence->current = sequence->first;
            }
        }
    }
}

void trial_set_combo_requirement(int hits, float damage) {
    mission_state->combo_hits = hits;
    mission_state->combo_damage = damage;
}

void trial_add_required_attack(
    unsigned char attack, unsigned char count, int flags) {
    KonquestRequiredSequenceList* list =
        (KonquestRequiredSequenceList*)&mission_state->combo_hits;
    KonquestRequiredSequence* sequence =
        &list->entries[list->current_sequence - 1];
    KonquestRequiredAttack* required_attack =
        &list->attacks[list->attack_count];

    if (sequence != 0 && required_attack != 0) {
        if (list == 0) {
            return;
        }
        required_attack->type = attack;
        required_attack->value = count;
        required_attack->flags = flags;
        required_attack->next = 0;
        if (sequence->current != 0) {
            sequence->current->next = required_attack;
        } else {
            sequence->first = required_attack;
        }
        sequence->current = required_attack;
        list->attack_count++;
    }
}

void trial_add_required_sequence(const char* first, const char* second) {
    KonquestRequiredSequenceList* list =
        (KonquestRequiredSequenceList*)&mission_state->combo_hits;
    KonquestRequiredSequence* sequence =
        &list->entries[list->current_sequence];

    if (sequence != 0) {
        if (list != 0) {
            sequence->message_parameter = second;
            sequence->message = first;
            list->current_sequence++;
            list->sequence_count++;
            mission_state->progress_required++;
        }
    }
}

void trial_register_attack(
    int player, unsigned char attack_type, unsigned char attack_value) {
    KonquestMissionState* state = get_mission_state();
    int trial_type;

    mission_state = state;
    if (state == 0 || mode_of_play != 8) {
        return;
    }
    if (!g_game_info.flag_bits.lens_flare_enabled) {
        return;
    }
    if (active_cmdscript != 0 && plyr_pdata != 0 &&
        active_cmdscript->mko == plyr_pdata->cmo) {
        attack_type = 3;
    }
    trial_type = state->trial_type;
    if (trial_type == 1) {
        KonquestRequiredSequenceList* list;
        KonquestRequiredSequence* sequence;
        KonquestRequiredAttack* required_attack;
        unsigned int sequence_index;

        if (player != state->fight->animation_side) {
            return;
        }
        sequence_index = state->trial_data[1];
        list = (KonquestRequiredSequenceList*)state->trial_data;
        sequence = &list->entries[sequence_index];
        required_attack = sequence->current;
        if (sequence == 0 || required_attack == 0 || list == 0) {
            return;
        }
        if (required_attack->type != attack_type &&
            required_attack->type != 0xFF) {
            return;
        }
        if (required_attack->value != attack_value &&
            required_attack->value != 0xFF) {
            return;
        }
        if (required_attack->flags == 0) {
            increment_required_moves_progress(sequence, list);
            return;
        }
        state->condition_value = sequence_index;
        mission_state->condition_player =
            mission_state->drone->animation_side;
        mission_state->condition_mode = 1;
        mission_state->condition_active = 1;
        if (active_cmdscript != 0 && active_cmdscript->attrs_table != 0) {
            KonquestTrialScriptAttrs* attrs =
                (KonquestTrialScriptAttrs*)active_cmdscript->attrs_table;

            mission_state->condition_ticks = attrs->condition_ticks;
        } else {
            mission_state->condition_ticks = 1;
        }
    } else if (trial_type == 0 &&
               player == state->fight->animation_side) {
        trial_increment_state_value(player, 5, 0);
    }
}

void trial_set_type(int type) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->trial_type = type;
        if (type == 2) {
            KonquestMissionState* active_state = mission_state;

            active_state->display_flags = 9;
            mission_state->randomized_side = (unsigned short)randu0(2);
        }
    }
}

int trial_check_state(void) {
    KonquestMissionState* state = get_mission_state();
    int complete = 0;
    int index;
    int result;
    float fight_health;
    float drone_health;

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    if (!g_game_info.flag_bits.lens_flare_enabled) {
        return 0;
    }
    fight_health = state->fight->current_health;
    drone_health = state->drone->current_health;
    switch (state->trial_type) {
    case 0:
        complete = 1;
        for (index = 0; index < 35 && complete; index++) {
            if (state->success_conditions[index].current_count <
                state->success_conditions[index].required_count) {
                complete = 0;
            }
        }
        break;
    case 2:
        if (drone_health == 0.0f) {
            complete = 1;
        }
        if (g_game_info.field_204 <= 0 && is_timer_off() == 0) {
            complete = 0;
        }
        break;
    case 1:
        if (state->progress_count >= state->progress_required) {
            complete = 1;
        }
        break;
    case 3:
        complete = state->combo_complete;
        break;
    }
    if (mission_state->field_2E8 != 2) {
        if (complete) {
            result = 2;
            mission_state->current_setup_function =
                mission_state->next_setup_function;
            mission_state->next_setup_function = 0;
            if (mission_state->fight->animation_side == 0) {
                result = 1;
            }
            return result;
        }
        if (drone_health == 0.0f || fight_health == 0.0f) {
            result = 1;
            if (mission_state->fight->animation_side == 0) {
                result = 2;
            }
            return result;
        }
        if (g_game_info.field_204 != 0 || is_timer_off() != 0) {
            return 0;
        }
    }
    if (g_game_info.field_204 <= 0) {
        if (complete) {
            result = 2;
            if (mission_state->fight->animation_side == 0) {
                result = 1;
            }
            return result;
        }
        result = 1;
        if (mission_state->fight->animation_side == 0) {
            result = 2;
        }
        return result;
    }
    return 0;
}

void trial_add_success_condition(int index, int value, int required_count) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state != 0) {
        state->success_conditions[index].required_count = required_count;
        mission_state->success_conditions[index].condition.type = value;
        mission_state->success_conditions[index].condition.value = index;
        mission_state->progress_required += (unsigned char)required_count;
    }
}

void trial_state_collision_check(int collision_result, int player) {
    KonquestMissionState* state = get_mission_state();
    int airborne;
    int boost_active;
    int condition_mode;

    mission_state = state;
    if (state != 0 && mode_of_play == 8) {
        if (g_game_info.flag_bits.lens_flare_enabled) {
            if (state->condition_active != 0) {
                state->condition_ticks--;
                if (mission_state->condition_ticks < 0) {
                    mission_state->condition_ticks = 0;
                }
                if (mission_state->trial_type == 0) {
                    KonquestSuccessCondition* conditions =
                        mission_state->success_conditions;

                    if (mission_state->condition_player != player) {
                        return;
                    }
                    airborne = is_pX_airborn(player);
                    if (player == 0) {
                        boost_active = (unsigned int)game_tick_ctr <
                            g_game_info.plyr1.slot.pdata
                                ->damage_boost_until;
                    } else {
                        boost_active = (unsigned int)game_tick_ctr <
                            g_game_info.plyr0.slot.pdata
                                ->damage_boost_until;
                    }
                    condition_mode = mission_state->condition_mode;
                    if ((condition_mode == 1 && collision_result == 1) ||
                        (condition_mode == 2 && airborne != 0 &&
                         collision_result == 1) ||
                        (condition_mode == 3 && boost_active != 0 &&
                         collision_result == 1) ||
                        (condition_mode == 0 && collision_result == 0)) {
                        increment_progress_count(1);
                        conditions[mission_state->condition_value]
                            .current_count++;
                        mission_state->condition_active = 0;
                    }
                } else if (mission_state->trial_type == 1) {
                    KonquestRequiredSequenceList* list =
                        (KonquestRequiredSequenceList*)
                            mission_state->trial_data;
                    KonquestRequiredSequence* sequence =
                        &list->entries[mission_state->condition_value];

                    if (sequence != 0) {
                        if (list == 0) {
                            return;
                        }
                        if (mission_state->condition_player == player) {
                            if (collision_result == 0) {
                                if (mission_state->condition_ticks == 0) {
                                    sequence->current = sequence->first;
                                    mission_state->condition_active = 0;
                                }
                            } else {
                                increment_required_moves_progress(
                                    sequence, list);
                                mission_state->condition_active = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}

static int register_condition(
    const KonquestMissionCondition* condition) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    switch (condition->type) {
    case 0:
        state->condition_active = 0;
        return 1;
    case 1:
        state->condition_value = condition->value;
        mission_state->condition_player =
            mission_state->fight->animation_side;
        mission_state->condition_mode = 1;
        mission_state->condition_active = 1;
        break;
    case 2:
        state->condition_value = condition->value;
        mission_state->condition_player =
            mission_state->drone->animation_side;
        mission_state->condition_mode = 1;
        mission_state->condition_active = 1;
        break;
    case 3:
        state->condition_value = condition->value;
        mission_state->condition_player =
            mission_state->fight->animation_side;
        mission_state->condition_mode = 0;
        mission_state->condition_active = 1;
        break;
    case 4:
        state->condition_value = condition->value;
        mission_state->condition_player =
            mission_state->drone->animation_side;
        mission_state->condition_mode = 2;
        mission_state->condition_active = 1;
        break;
    case 5:
        state->condition_value = condition->value;
        mission_state->condition_player =
            mission_state->drone->animation_side;
        mission_state->condition_mode = 3;
        mission_state->condition_active = 1;
        break;
    }
    return 0;
}

static void increment_required_moves_progress(
    KonquestRequiredSequence* sequence,
    KonquestRequiredSequenceList* list) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return;
    }
    sequence->current = sequence->current->next;
    if (sequence->current == 0) {
        sequence->current = sequence->first;
        increment_progress_count(1);
        list->current_sequence++;
        if (list->current_sequence == list->sequence_count) {
            list->current_sequence = 0;
            mission_state->move_message = 0;
            mission_state->move_message_param = 0;
        } else {
            const char* message = sequence[1].message;
            const char* message_parameter =
                sequence[1].message_parameter;

            mission_state->move_message = message;
            mission_state->move_message_param = message_parameter;
        }
        trial_show_move_message();
    }
}

void trial_increment_state_value(
    int player, int state_index, int opponent_event) {
    KonquestMissionState* state = get_mission_state();
    KonquestSuccessCondition* conditions;

    mission_state = state;
    if (state != 0 && mode_of_play == 8) {
        if (!g_game_info.flag_bits.lens_flare_enabled) {
            return;
        }
        conditions = state->success_conditions;
        if ((opponent_event != 0 ||
             player == state->fight->animation_side) &&
            (opponent_event != 1 ||
             player != state->fight->animation_side) &&
            state->trial_type == 0 &&
            conditions[state_index].required_count > 0) {
            if (conditions[state_index].current_count >=
                conditions[state_index].required_count) {
                return;
            }
            if (register_condition(&conditions[state_index].condition)) {
                increment_progress_count(1);
                conditions[state_index].current_count++;
            }
        }
    }
}

#pragma dont_inline on
float p_konquest_register_bleeding(void) {
    KonquestMissionState* state = mission_state_item.state;

    if (state != 0) {
        if (state->hdr.instance != mission_state_item.instance) {
            state = 0;
        }
    } else {
        state = 0;
    }

    mission_state = state;
    if (state == 0) {
        return -1.0f;
    }
    if (mode_of_play != 8 ||
        !g_game_info.flag_bits.lens_flare_enabled) {
        return -1.0f;
    }
    if (state->trial_type != 0 || state->bleeding_required <= 0) {
        return -1.0f;
    }
    if (state->drone->pdata->postround_value == 0.0f) {
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
#pragma dont_inline reset

static float p_trial_tick(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return -1.0f;
    }
    if (state->tick_script_function == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(
        state->tick_script, state->tick_script_function);
    cmdscript_execute(mission_state->tick_script);
    return 1.0f;
}

void trial_start_new_round(void) {
    g_game_info.flag_bits.field_bit0 = 0;
    if (mission_state->round_health_restoration > 0.0) {
        mission_state->fight->current_health +=
            mission_state->round_health_restoration;
    } else {
        mission_state->fight->current_health =
            mission_state->fight->maximum_health;
    }
    if (mission_state->fight->current_health > 1.0) {
        mission_state->fight->current_health = 1.0f;
    }
    mission_state->drone->current_health =
        mission_state->drone->maximum_health;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int trial_end_round(void) {
    KonquestMissionState* state;
    KonquestRequiredSequenceList* sequences;
    ScreenObj* sign;
    MkObj* monk;
    MkProc* player_process;
    int player_rounds;
    int drone_rounds;
    int drone_round_limit;
    int player_won_round = 0;
    int retry;
    int value;
    int i = 0x168;
    unsigned char alpha;

    g_game_info.flag_bits.pad_bit6 = 0;
    g_game_info.flag_bits.field_bit0 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    destroy_mkprocs_pid(0x9003);
    destroy_mkprocs_pid(0x9004);
    del_string_obj_by_id(0x9005);
    del_string_obj_by_id(0x9007);
    delete_screen_obj_oid(0x9006);
    mission_state->player_one_switch_state = 0;
    mission_state->player_two_switch_state = 0;
    mission_state->field_30C = 0;

    state = get_mission_state();
    mission_state = state;
    if (state == 0) {
        drone_round_limit = 0;
    } else if (state->display_flags & 1) {
        drone_round_limit = state->num_rounds;
    } else {
        drone_round_limit = 1;
    }

    if (round_winner == 1) {
        g_game_info.plyr0.field_40++;
        if (state->fight->animation_side == 0) {
            player_won_round = 1;
        }
    } else if (round_winner == 2) {
        g_game_info.plyr1.field_40++;
        if (state->fight->animation_side == 1) {
            player_won_round = 1;
        }
    }

    turn_controllers_off();
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr0.slot.mirror_a->pos_vel.z = 0.0f;
    g_game_info.plyr0.slot.mirror_a->pos_vel.y = 0.0f;
    g_game_info.plyr0.slot.mirror_a->pos_vel.x = 0.0f;
    g_game_info.plyr1.slot.mirror_a->pos_vel.z = 0.0f;
    g_game_info.plyr1.slot.mirror_a->pos_vel.y = 0.0f;
    g_game_info.plyr1.slot.mirror_a->pos_vel.x = 0.0f;

    if (mission_state->display_flags & 1) {
        end_music();
    }
    if (!(mission_state->display_flags & 1)) {
        while ((mission_state->player_one_wrapup_state < 2 ||
                mission_state->player_two_wrapup_state < 2) &&
               i != 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            i--;
        }
    }

    player_rounds = mission_state->fight->field_40;
    drone_rounds = mission_state->drone->field_40;
    if (mission_state->display_flags & 1) {
        update_plyr_medals();
        do_win_effect();
    }
    if (player_rounds >= mission_state->num_rounds ||
        drone_rounds >= drone_round_limit) {
        g_game_info.pause_flag_bits.fatality_window = 1;
        winner = round_winner != 1;
    }
    f_fatality_finished = 1;

    if (!(mission_state->display_flags & 1)) {
        if (player_won_round != 0 &&
            player_rounds < mission_state->num_rounds) {
            if (!(mission_state->display_flags & 1)) {
                _mkproc_sleep_ticks = 15.0f;
                aproc->vtbl->sleep();
                snd_req(0x15E8);
                sign = load_2d_pfxobj_xy(
                    0x2001E, 0x9009,
                    get_trial_sign_list()[randu0(3)], 0,
                    ((screen_width - 0x280) / 2) + 0xC0,
                    0x138, 0x24);
                if (sign != 0) {
                    alpha = 0;
                    while (alpha < 0xF0) {
                        pfx_2d_obj_set_alpha(sign, alpha);
                        _mkproc_sleep_ticks = 1.0f;
                        aproc->vtbl->sleep();
                        alpha += 8;
                    }
                    _mkproc_sleep_ticks = 60.0f;
                    aproc->vtbl->sleep();
                    alpha = 0xF0;
                    while (alpha != 0) {
                        pfx_2d_obj_set_alpha(sign, alpha);
                        _mkproc_sleep_ticks = 1.0f;
                        aproc->vtbl->sleep();
                        alpha -= 8;
                    }
                    if (sign->instance != 0) {
                        ((void (*)(ScreenObj*))sign->vtbl->destroy)(sign);
                    }
                }
                _mkproc_sleep_ticks = 30.0f;
                aproc->vtbl->sleep();
            }
            state = get_mission_state();
            mission_state = state;
            if (state != 0) {
                konquest_save_data.passed_last_mission = 1;
                if (state->winner_end_function != 0) {
                    cmdscript_setup_execution(
                        state->tick_script, state->winner_end_function);
                    cmdscript_execute(mission_state->tick_script);
                }
            }
        }
        f_fatality_finished = 1;
    }

    winner = -1;
    g_game_info.pselect.field_1f4++;
    if (player_rounds >= mission_state->num_rounds) {
        value = get_konq_profile_value(5, mission_state->mission_index) + 1;
        if (value >= 0xFF) {
            value = 0xFF;
        }
        set_konq_profile_value(5, mission_state->mission_index, value);

        state = get_mission_state();
        mission_state = state;
        if (state != 0) {
            konquest_save_data.passed_last_mission = 1;
            if (state->winner_end_function != 0) {
                cmdscript_setup_execution(
                    state->tick_script, state->winner_end_function);
                cmdscript_execute(mission_state->tick_script);
            }
        }

        monk = get_mission_monk();

        state = get_mission_state();
        mission_state = state;
        if (state != 0) {
            if (monk != 0) {
                player_process = state->fight->process;
                xfer_proc(player_process, p_transform_into_monk);
                while (player_process->entry == p_transform_into_monk) {
                    _mkproc_sleep_ticks = 1.0f;
                    aproc->vtbl->sleep();
                }
            }
            mission_state->transform_complete = 1;
        }
        _mkproc_sleep_ticks = 30.0f;
        aproc->vtbl->sleep();
        return 0;
    }

    if (drone_rounds >= drone_round_limit) {
        turn_controllers_on();
        konquest_save_data.passed_last_mission = 0;
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep();
        if (mission_state->loser_end_function != 0) {
            cmdscript_setup_execution(
                mission_state->tick_script,
                mission_state->loser_end_function);
            cmdscript_execute(mission_state->tick_script);
        }
        if (text_window_state == 1) {
            if (mission_state->display_flags & 1) {
                g_game_info.pselect.field_1f4 = 1;
                g_game_info.plyr1.field_40 = 0;
                g_game_info.plyr0.field_40 = 0;
                g_game_info.flag_bits.level_fatality_done = 1;
                update_plyr_medals();
            } else {
                mission_state->drone->field_40 = 0;
            }
            retry = 0;
        } else {
            retry = 1;
        }
        turn_controllers_off();
        if (retry != 0) {
            mission_state->field_2E8 = 2;
            return 0;
        }
        g_game_info.pause_flag_bits.fatality_window = 0;
        mission_state->display_flags |= 4;
    }

    if (!(mission_state->display_flags & 1)) {
        mission_state->display_flags |= 4;
    }
    if (mission_state->display_flags & 4) {
        destroy_onscreen_fight_2d_objects();
    }

    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        if (state->trial_type == 0) {
            for (i = 0; i < 35; i++) {
                mission_state->success_conditions[i].current_count = 0;
            }
        } else if (state->trial_type == 1) {
            for (i = 0; i < 12; i++) {
                sequences = (KonquestRequiredSequenceList*)
                    mission_state->trial_data;
                if (sequences->entries[i].first == 0) {
                    break;
                }
                sequences->entries[i].current = sequences->entries[i].first;
            }
            sequences = (KonquestRequiredSequenceList*)
                mission_state->trial_data;
            sequences->current_sequence = 0;
        } else if (state->trial_type == 3) {
            state->combo_complete = 0;
        }
        mission_state->condition_active = 0;
        mission_state->progress_count = 0;
        mission_state->move_message = 0;
        mission_state->move_message_param = 0;
    }
    if ((mission_state->display_flags & 1) ||
        (mission_state->display_flags & 8)) {
        reset_fight(1);
    } else {
        reset_fight(0);
    }
    return 1;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void skip_end_of_trial_wrapup(void) {
    if (mission_state->fight->animation_side ==
        (aproc->pid != 0x1001)) {
        mission_state->player_one_wrapup_state = 2;
        return;
    }
    mission_state->player_two_wrapup_state = 2;
}

float end_of_trial_wrapup(int winner) {
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
        if (winner != 0) {
            ((KonquestMissionJumpVtable*)aproc->vtbl)
                ->jump_sleep(successful_trial_player_wrapup, 0.0f);
            return 0.0f;
        }
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(successful_trial_player_wrapup, 0.0f);
        return 0.0f;
    }
    if (mission_state->player_two_wrapup_state != 0) {
        mission_state->player_two_wrapup_state = 2;
        return 0.0f;
    }
    mission_state->player_two_wrapup_state = 1;
    if (winner != 0) {
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(failed_trial_drone_wrapup, 0.0f);
        return 0.0f;
    }
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(successful_trial_drone_wrapup, 0.0f);
    return 0.0f;
}

static float failed_trial_drone_wrapup(void) {
    TrialWrapupData* wrapup =
        mission_state->drone->pdata->status_data->trial_wrapup_data;
    TrialWrapupEntry* entry;
    MkProc* animation_process;
    AnimPdata* animation_pdata;
    AnimScript* animation;
    unsigned short choice;
    int generic;
    int sound_id;
    LipSyncKeyframe* lip_sync;
    float duration;
    float animation_duration;

    if (wrapup != 0) {
        entry = wrapup->failure_table;
        choice = randu0(
            (unsigned short)get_row_count_for_table_by_pointer(
                plyr_pdata->cmo, entry));
        while (choice != 0) {
            entry++;
            choice--;
        }
        wrapup->selected_failure = entry;
        generic = 0;
    } else {
        entry = generic_char_failure_table;
        choice = randu0(5);
        while (choice != 0) {
            entry++;
            choice--;
        }
        generic_char_wrapup_data.selected_failure = entry;
        generic = 1;
    }
    sound_id = entry->sound_id;
    lip_sync = entry->lip_sync;

    set_ani_speed(1.0f);
    rotate_towards_him(0.2f);
    blend_to_stance(0.1f);
    _mkproc_sleep_ticks = 30.0f;
    aproc->vtbl->sleep();

    if (apdata != 0) {
        PlyrPdata* pdata = (PlyrPdata*)apdata;

        animation_process = pdata->anim_proc;
        if (animation_process != 0) {
            if (animation_process->instance != pdata->anim_proc_instance) {
                animation_process = 0;
            }
        } else {
            animation_process = 0;
        }
        if (animation_process != 0) {
            animation_pdata =
                (AnimPdata*)pdata_of_proc(animation_process);
            if (animation_pdata != 0) {
                animation_pdata->flags |= 0x4000;
            }
        }
    }

    duration = duration_of_lip_sync(lip_sync);
    if (entry->animation_id >= 3000) {
        animation = get_animation(entry->animation_id);
    } else {
        animation = get_animation(entry->animation_id + 3000);
    }
    if (animation != 0 && plyr_anim_pdata != 0) {
        transition_to_anim_script(
            0.1f, plyr_anim_pdata, animation, 3);
        animation_duration =
            (float)((KonquestAnimScriptView*)animation)->frame_count;
        if (animation_duration > 10.0f) {
            animation_duration -= 10.0f;
        }
        if (animation_duration > duration) {
            duration = animation_duration;
        }
    }

    _mkproc_sleep_ticks = 0.0f;
    aproc->vtbl->sleep();
    if (generic != 0) {
        plyr_snd_req(sound_id);
    } else {
        drone_lip_synch(sound_id, lip_sync);
    }
    if (duration < 0.0f) {
        duration = 0.0f;
    }
    _mkproc_sleep_ticks = duration;
    aproc->vtbl->sleep();

    if (apdata != 0) {
        PlyrPdata* pdata = (PlyrPdata*)apdata;

        animation_process = pdata->anim_proc;
        if (animation_process != 0) {
            if (animation_process->instance != pdata->anim_proc_instance) {
                animation_process = 0;
            }
        } else {
            animation_process = 0;
        }
        if (animation_process != 0) {
            animation_pdata =
                (AnimPdata*)pdata_of_proc(animation_process);
            if (animation_pdata != 0) {
                animation_pdata->flags &= ~0x4000;
            }
        }
    }
    mission_state->player_two_wrapup_state = 2;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float successful_trial_drone_wrapup(void) {
    TrialWrapupData* wrapup =
        mission_state->drone->pdata->status_data->trial_wrapup_data;
    TrialWrapupEntry* entry;
    MkProc* animation_process;
    AnimPdata* animation_pdata;
    AnimScript* animation;
    unsigned short choice;
    int generic;
    int sound_id;
    LipSyncKeyframe* lip_sync;
    float post_sound_delay;
    float duration;
    float animation_duration;

    if (wrapup != 0) {
        entry = wrapup->success_table;
        choice = randu0(
            (unsigned short)get_row_count_for_table_by_pointer(
                plyr_pdata->cmo, entry));
        while (choice != 0) {
            entry++;
            choice--;
        }
        wrapup->selected_success = entry;
        generic = 0;
    } else {
        entry = generic_char_success_table;
        choice = randu0(5);
        while (choice != 0) {
            entry++;
            choice--;
        }
        generic_char_wrapup_data.selected_success = entry;
        generic = 1;
    }
    sound_id = entry->sound_id;
    lip_sync = entry->lip_sync;
    post_sound_delay = entry->post_sound_delay;

    set_ani_speed(1.0f);
    rotate_towards_him(0.2f);
    blend_to_stance(0.1f);
    _mkproc_sleep_ticks = 30.0f;
    aproc->vtbl->sleep();
    if (is_weapon_style(plyr_pdata->fighter_definition)) {
        plyr_weapon_hide(plyr_pdata, 0, plyr_pdata->mirror_slots);
    }

    if (apdata != 0) {
        PlyrPdata* pdata = (PlyrPdata*)apdata;

        animation_process = pdata->anim_proc;
        if (animation_process != 0) {
            if (animation_process->instance != pdata->anim_proc_instance) {
                animation_process = 0;
            }
        } else {
            animation_process = 0;
        }
        if (animation_process != 0) {
            animation_pdata =
                (AnimPdata*)pdata_of_proc(animation_process);
            if (animation_pdata != 0) {
                animation_pdata->flags |= 0x4000;
            }
        }
    }

    duration = duration_of_lip_sync(lip_sync);
    if (entry->animation_id >= 3000) {
        animation = get_animation(entry->animation_id);
    } else {
        animation = get_animation(entry->animation_id + 3000);
    }
    if (animation != 0 && plyr_anim_pdata != 0) {
        transition_to_anim_script(
            0.1f, plyr_anim_pdata, animation, 0x4003);
        animation_duration =
            (float)((KonquestAnimScriptView*)animation)->frame_count;
        if (animation_duration > 10.0f) {
            animation_duration -= 10.0f;
        }
        if (animation_duration > duration) {
            duration = animation_duration;
        }
    }

    _mkproc_sleep_ticks = post_sound_delay;
    aproc->vtbl->sleep();
    if (generic != 0) {
        plyr_snd_req(sound_id);
    } else {
        drone_lip_synch(sound_id, lip_sync);
    }
    duration -= post_sound_delay;
    if (duration < 0.0f) {
        duration = 0.0f;
    }
    _mkproc_sleep_ticks = duration;
    aproc->vtbl->sleep();

    if (apdata != 0) {
        PlyrPdata* pdata = (PlyrPdata*)apdata;

        animation_process = pdata->anim_proc;
        if (animation_process != 0) {
            if (animation_process->instance != pdata->anim_proc_instance) {
                animation_process = 0;
            }
        } else {
            animation_process = 0;
        }
        if (animation_process != 0) {
            animation_pdata =
                (AnimPdata*)pdata_of_proc(animation_process);
            if (animation_pdata != 0) {
                animation_pdata->flags &= ~0x4000;
            }
        }
    }
    if (is_weapon_style(plyr_pdata->fighter_definition)) {
        plyr_weapon_show(plyr_pdata, 0, plyr_pdata->mirror_slots);
    }
    mission_state->player_two_wrapup_state = 2;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float successful_trial_player_wrapup(void) {
    set_ani_speed(1.0f);
    blend_to_stance(0.1f);
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();
    mission_state->player_one_wrapup_state = 2;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

void trial_round_init(void) {
    KonquestMissionState* state = get_mission_state();
    KonquestRequiredSequenceList* sequences;
    MkObj* monk;
    MkProc* script_process;
    MkProc* process;
    int i;
    int flipped;

    mission_state = state;
    if (state == 0) {
        return;
    }

    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        turn_camera_on();
        if (g_game_info.pselect.field_1f4 == 1) {
            turn_camera_on();
            skip_camera_intro();
            if (mission_state->transform_complete == 0) {
                fade_from_black(20, 0);
            }
        } else if (mission_state->display_flags & 1) {
            _mkproc_sleep_ticks = 90.0f;
            aproc->vtbl->sleep();
        }
    }

    push_game_state(6);

    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        monk = state->monk;
        if (monk != 0 && monk->hdr.instance != state->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }

    state = get_mission_state();
    mission_state = state;
    if (state != 0) {
        if (monk != 0) {
            script_process = state->script_process;
            if (script_process != 0 &&
                script_process->instance != state->script_process_instance) {
                script_process = 0;
            }
            if (script_process != 0) {
                if (state->transform_complete == 0) {
                    xfer_proc(script_process, p_transform_into_player);
                    while (script_process->entry == p_transform_into_player) {
                        _mkproc_sleep_ticks = 1.0f;
                        aproc->vtbl->sleep();
                    }
                }
                mission_state->transform_complete = 1;
            }
        } else {
            mission_state->transform_complete = 1;
        }
    }

    mission_state->player_one_wrapup_state = 0;
    mission_state->player_two_wrapup_state = 0;
    mission_state->restriction_phase = 0;
    if (mission_state->display_flags & 4) {
        turn_controllers_on();
        mission_state->display_flags &= ~4;
        memset(mission_state->trial_data, 0, sizeof(mission_state->trial_data));
        mission_state->progress_count = 0;
        mission_state->progress_required = 0;
        mission_state->field_2E8 = 0;
        mission_state->tick_script_function = 0;
        g_game_info.plyr0.slot.pdata->breaker_strength = 3;
        g_game_info.plyr1.slot.pdata->breaker_strength = 3;
        if (mission_state->current_setup_function == 0) {
            g_game_info.pselect.field_1f4 = 1;
            mission_state->fight->field_40 = 0;
            mission_state->drone->field_40 = 0;
            mission_state->current_setup_function = 1;
        }
        cmdscript_setup_execution(
            mission_state->tick_script,
            mission_state->current_setup_function);
        cmdscript_execute(mission_state->tick_script);
        if (mission_state->display_flags & 1) {
            turn_controllers_off();
        }
        init_pwr_bars();
        if (are_powerbars_retracted()) {
            extend_powerbars();
        }
    } else {
        if (are_powerbars_retracted()) {
            init_pwr_bars();
            extend_powerbars();
        }
        start_powerbar_monitor();
    }
    pop_game_state();

    if (mission_state->trial_type == 1) {
        sequences = (KonquestRequiredSequenceList*)mission_state->trial_data;
        for (i = 0; i < sequences->sequence_count; i++) {
            sequences->entries[i].current = sequences->entries[i].first;
        }
        mission_state->progress_required = sequences->sequence_count;
        sequences->current_sequence = 0;
        mission_state->move_message = sequences->entries[0].message;
        mission_state->move_message_param =
            sequences->entries[0].message_parameter;
    }

    if (mission_state->tick_script_function != 0) {
        process = _create_mkproc_generic_bigstack(
            0x9003, 6, p_trial_tick, 0, 0);
        if (process != 0) {
            set_process_as_scriptable(process);
        }
    }
    if (mission_state->display_item_c != 0) {
        _create_mkproc_generic_nostack(
            0x9004, 0x1F, p_flip_move_description, 0, 0);
        state = get_mission_state();
        mission_state = state;
        if (state != 0) {
            flipped = is_a_to_the_right_of_b(
                state->fight->active_object, state->drone->active_object);
        } else {
            flipped = 0;
        }
        mission_state->move_description_flipped = flipped;
    }
    trial_show_move_message();
    increment_progress_count(0);
    if (mission_state->drone_difficulty == 8 &&
        mission_state->drone->field_0x0 >= 0) {
        turn_port_off(mission_state->drone->field_0x0);
    }
    mission_state->field_2E8 = 0;
    g_game_info.field_204 = mission_state->round_timer;
}

void trial_game_init(void) {
    char mission_name[16];
    KonquestMissionState* state = (KonquestMissionState*)get_mkhdr(
        &vtbl_mkpdata_generic, sizeof(KonquestMissionState));

    mission_state = state;
    if (state != 0) {
        konquest_save_data.passed_last_mission = 0;
        konquest_save_data.progression = 0;
        zero_pdata_payload(sizeof(*state), &state->hdr);
        mission_state_item.state = mission_state;
        mission_state_item.instance = mission_state->hdr.instance;
        if (g_game_info.plyr0.player_state == 2) {
            mission_state->fight =
                (KonquestMissionFightInfo*)&g_game_info.plyr0;
            mission_state->drone =
                (KonquestMissionFightInfo*)&g_game_info.plyr1;
        } else {
            mission_state->fight =
                (KonquestMissionFightInfo*)&g_game_info.plyr1;
            mission_state->drone =
                (KonquestMissionFightInfo*)&g_game_info.plyr0;
        }
        unassign_player((PlyrInfo*)mission_state->drone);
        g_game_info.pads[2].flag_bits.connected = 1;
        assign_player(2);
        g_game_info.pads[2].flag_bits.connected = 0;
        set_game_switch_maps();
        mission_state->display_flags =
            (unsigned int)konquest_save_data.next_value_a | 4;
        mission_state->drone_difficulty = konquest_save_data.next_value_b;
        mission_state->mission_index = konquest_save_data.next_mission;
        mission_state->current_setup_function = 1;
        mission_state->player_one_damage_scale = 1.0f;
        mission_state->player_two_damage_scale = 1.0f;
        if (mission_state->display_flags & 1) {
            mission_state->trial_type = 2;
        }
        load_ssf(konquest_region_data[konquest_save_data.region_index].art_files);
        load_string_bank(
            0x20000,
            konquest_region_data[konquest_save_data.region_index].string_bank);
        sprintf(
            mission_name, "mission_%03d.mko",
            mission_state->mission_index);
        mission_state->tick_script =
            cmdscript_loadfile_by_name(0x10, mission_name);
        trial_load_monk();
        add_art_section_by_name_async_language(
            0x2001E, "trial_sprites.sec");
        wait_for_slot_load(0x2001E);
        load_font(6);
        load_font(7);
        mission_state->tune_table =
            (KonquestMissionTuneTable*)get_data_table_by_name("bgnd_music");
        g_game_info.plyr0.field_0C = 1.0f;
        g_game_info.plyr1.field_0C = 1.0f;
        g_game_info.plyr0.field_10 = 1.0f;
        g_game_info.plyr1.field_10 = 1.0f;
    }
}

PlyrInfo* trial_get_drone_info(void) {
    KonquestMissionState* state = get_mission_state();

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    return (PlyrInfo*)state->drone;
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

static float p_transform_into_monk(void) {
    MkProc* script_process = mission_state->script_process;
    AnimPdata* animation = plyr_anim_pdata;

    if (script_process != 0 &&
        script_process->instance !=
            mission_state->script_process_instance) {
        script_process = 0;
    }
    if (plyr_anim_proc != 0 && animation != 0) {
        xfer_proc(plyr_anim_proc, p_anim_idle);
        animation->step = -1.0f;
        transition_to_anim_script_frame(
            0.05f, 105.0f, animation,
            bgnd_animations.transform_animation, 0x43);
        plyr_obj->flags_08_bits.scale_active = 1;
        plyr_obj->scale.z = 1.0f;
        plyr_obj->scale.x = 1.0f;
        plyr_obj->scale.y = 1.0f;
        while (animation->frame > 63.0f) {
            animpdata_ani_1_frame(animation);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        snd_req(0x15EF);
        xfer_proc(script_process, p_finish_transform_monk);
        while (script_process->entry == p_finish_transform_monk) {
            animpdata_ani_1_frame(animation);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
    }
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float p_transform_into_player(void) {
    MkProc* monk_process = mission_state->monk_process;
    AnimPdata* animation;
    MkProc* player_process;

    if (monk_process != 0 &&
        monk_process->instance !=
            mission_state->monk_process_instance) {
        monk_process = 0;
    }
    if (monk_process != 0) {
        animation = (AnimPdata*)pdata_of_proc(monk_process);
        animation->step = 0.8f;
        xfer_proc(monk_process, p_anim_idle);
        transition_to_anim_script(
            0.05f, animation,
            bgnd_animations.monk_transform_animation, 0x43);
        animpdata_ani_to_frame_x(animation, 32.0f);
        snd_req(0x15EF);
        player_process = mission_state->fight->process;
        xfer_player_proc(player_process, p_finish_transform_player);
        while (player_process->entry == p_finish_transform_player) {
            animpdata_ani_1_frame(animation);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
    }
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_idle, 0.0f);
    return 0.0f;
}

static float p_finish_transform_player(void) {
    KonquestMissionState* state = get_mission_state();
    MkProc* animation_process;
    AnimPdata* animation;
    MkObj* monk;

    mission_state = state;
    animation_process = state->fight->pdata->anim_proc;
    if (animation_process != 0) {
        if (animation_process->instance !=
            state->fight->pdata->anim_proc_instance) {
            animation_process = 0;
        }
    } else {
        animation_process = 0;
    }
    animation = (AnimPdata*)pdata_of_proc(animation_process);
    monk = get_mission_monk();

    xfer_proc(animation_process, p_anim_idle);
    set_anim_script_frame(
        63.0f, animation, bgnd_animations.transform_animation, 0x43);
    plyr_obj->pos.x = monk->pos.x;
    plyr_obj->pos.y = monk->pos.y;
    plyr_obj->pos.z = monk->pos.z;
    update_mkobj(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    start_hero_transform_effect(plyr_obj);
    shake_camera(3, 0.03f);
    _mkproc_sleep_ticks = 15.0f;
    ((KonquestMissionProcVtable*)aproc->vtbl)->sleep();

    plyr_obj->flags_08_bits.scale_active = 1;
    plyr_obj->scale.z = 1.0f;
    plyr_obj->scale.x = 1.0f;
    plyr_obj->scale.y = 0.8f;
    plyr_obj->hide_flag_bits.hidden = 0;
    monk->hide_flag_bits.hidden = 1;

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
    update_mkobj(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_obj->flags_08_bits.scale_active = 0;
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

static float p_finish_transform_monk(void) {
    Vec position = {4000.0f, 0.0f, 4000.0f};
    Vec angles = {0.0f, 0.0f, 0.0f};
    KonquestMissionState* state = mission_state;
    PlyrPdata* fighter = state->fight->pdata;
    MkObj* player_object = state->fight->active_object;
    MkObj* sidekick = fighter->sidekick_obj;
    MkProc* raw_monk_process = state->monk_process;
    MkProc* monk_process;
    AnimPdata* animation;
    MkObj* monk;

    if (sidekick != 0) {
        if (sidekick->hdr.instance != fighter->sidekick_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    if (raw_monk_process != 0) {
        if (raw_monk_process->instance != state->monk_process_instance) {
            raw_monk_process = 0;
        }
    } else {
        raw_monk_process = 0;
    }
    monk_process = raw_monk_process;
    if (monk_process != 0) {
        animation = (AnimPdata*)pdata_of_proc(monk_process);
        animation->step = -0.8f;
    } else {
        ((KonquestMissionJumpVtable*)aproc->vtbl)
            ->jump_sleep(p_idle, 0.0f);
        return 0.0f;
    }
    state = get_mission_state();
    mission_state = state;
    if (state == 0) {
        monk = 0;
    } else {
        monk = state->monk;
        if (monk != 0) {
            if (monk->hdr.instance != state->monk_instance) {
                monk = 0;
            }
        } else {
            monk = 0;
        }
    }
    xfer_proc(monk_process, p_anim_idle);
    set_anim_script_frame(
        32.0f, animation,
        bgnd_animations.monk_transform_animation, 0x43);
    monk->pos.x = player_object->pos.x;
    monk->pos.y = player_object->pos.y;
    monk->pos.z = player_object->pos.z;
    monk->ang.x = player_object->ang.x;
    monk->ang.y = player_object->ang.y;
    monk->ang.z = player_object->ang.z;
    if (monk != 0) {
        update_mkobj(as_mkhdr(&monk->hdr));
    } else {
        update_mkobj(0);
    }
    start_hero_transform_effect(player_object);
    shake_camera(3, 0.03f);
    _mkproc_sleep_ticks = 15.0f;
    aproc->vtbl->sleep();

    xfer_camera(p_idle, 1);
    hide_player(mission_state->fight->pdata, 1);
    move_player(player_object, &position, &angles);
    if (sidekick != 0) {
        sidekick->pos.x = position.x;
        sidekick->pos.y = position.y;
        sidekick->pos.z = position.z;
    }
    monk->hide_flag_bits.hidden = 0;
    while (animation->frame > 0.0f) {
        animpdata_ani_1_frame(animation);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    animation->step = 1.0f;
    xfer_proc(monk_process, p_animate);
    transition_to_anim_script(
        0.1f, animation, bgnd_animations.monk_animation, 0x40);
    ((KonquestMissionJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_idle, 0.0f);
    return 0.0f;
}

static void start_hero_transform_effect(MkObj* hero) {
    start_transform_particle_set("hero_transform_smoke", hero);
    start_transform_particle_set("hero_transform_explode", hero);
}

static void trial_load_monk(void) {
    KonquestMissionState* state = get_mission_state();
    KonquestRoundStartPositions* starts;
    const Vec* position;
    const Vec* angles;
    MkProc* monk_process;
    AnimPdata* animation;
    MkObj* player_object;
    MkObj* monk;
    MkProc* script_process;
    AniTextureControl* face_texture;

    mission_state = state;
    if (state != 0 && state->fight->character_id != 0x1A) {
        if (state->fight->character_id == 0x19) {
            return;
        }

        starts = (KonquestRoundStartPositions*)g_game_info.misc;
        if (state->fight->animation_side == 0) {
            position = &starts->player_one_position;
            angles = &starts->player_one_angles;
        } else {
            position = &starts->player_two_position;
            angles = &starts->player_two_angles;
        }

        monk_process = load_hero_model(bgnd_animations.monk_animation);
        if (monk_process != 0) {
        player_object = mission_state->fight->active_object;
        player_object->hide_flag_bits.hidden = 1;
        xfer_proc(monk_process, p_animate);
        animation = (AnimPdata*)pdata_of_proc(monk_process);
        mission_state->monk_process = monk_process;
        mission_state->monk_process_instance = monk_process->instance;

        monk = animation->obj;
        if (monk != 0) {
            if (monk->hdr.instance != animation->obj_instance) {
                monk = 0;
            }
        } else {
            monk = 0;
        }
        if (monk != 0) {
            monk->light_flags = player_object->light_flags;
            monk->pos.x = position->x;
            monk->pos.y = position->y;
            monk->pos.z = position->z;
            monk->ang.x = angles->x;
            monk->ang.y = angles->y;
            monk->ang.z = angles->z;
            monk->flags_08_bits.scale_active = 1;
            monk->scale.x = 1.1f;
            monk->scale.y = 1.1f;
            monk->scale.z = 1.1f;
            monk->hide_flag_bits.pin_animation = 0;
            mission_state->monk = monk;
            mission_state->monk_instance = monk->hdr.instance;
            insert_ground_me_mkobj(monk);
            monk->flags_09_bits.launched = 1;
            monk->flags_09_bits.bit6 = 1;
        }

        script_process = _create_mkproc_generic_bigstack(
            0x9007, 0x1F, p_idle, 0, 0);
        if (script_process != 0) {
            set_process_as_scriptable(script_process);
            mission_state->script_process = script_process;
            mission_state->script_process_instance = script_process->instance;
            script_process->pre_destroy = pw_konquest_trial_monk;
            script_process->destroy_cb = ps_konquest_trial_monk;
        }
            face_texture = konquest_create_monk_face_ani_texture(monk);
            if (face_texture != 0) {
                mission_state->monk_face_texture = face_texture;
                mission_state->monk_face_texture_instance =
                    face_texture->instance;
            }
        }
    }
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
    if (process != 0) {
        if (process->instance != state->monk_process_instance) {
            process = 0;
        }
    } else {
        process = 0;
    }
    if (process != 0) {
        current_anim_pdata = (AnimPdata*)pdata_of_proc(aproc);
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

MkObj* trial_get_monk(void) {
    KonquestMissionState* state = get_mission_state();
    MkObj* monk;

    mission_state = state;
    if (state == 0) {
        return 0;
    }
    monk = state->monk;
    if (monk != 0) {
        if (monk->hdr.instance != state->monk_instance) {
            return 0;
        }
        return monk;
    }
    return 0;
}
