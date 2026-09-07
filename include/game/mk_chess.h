#ifndef MKD_GAME_MK_CHESS_H
#define MKD_GAME_MK_CHESS_H

#include "runtime/mk_struct.h"
#include "math/gxVect.h"
#include "msl/msl_types.h"

#define MK_CHESS_MOVEMENT_SKILL_COUNT 8
#define MK_CHESS_PIECE_EVENT_COUNT 64
#define MK_CHESS_BOARD_COLUMNS 10
#define MK_CHESS_SPELL_TARGET_COUNT 2
#define MK_CHESS_CURSOR_COUNT 3

typedef void (*ChessPieceEventScript)(void);
typedef struct AniScript AniScript;
typedef struct MkObj MkObj;
typedef struct ScreenObj ScreenObj;
struct CameraObj;
struct MkPtr;
struct ChessSideController;
struct MkProc;
struct ScriptSlot;
struct ChessDirectionState;
struct ChessSpellDefinition;
struct ChessLibraryEntry;

typedef struct ChessAnimPdata {
    char pad00[0x10];
    MkObj* object;
    unsigned int object_instance;
    char pad18[0x18];
    unsigned int flags; /* +0x30 - animation control flags */
    char pad34[4];
    float frame; /* +0x38 */
    char pad3C[4];
    float end_frame; /* +0x40 */
    float speed; /* +0x44 */
} ChessAnimPdata;

typedef struct ChessScreenRef {
    ScreenObj* screen;
    unsigned int instance;
} ChessScreenRef;

typedef struct ChessClassFlags {
    unsigned char spellcaster : 1; /* bit7 */
    unsigned char pad : 7;
} ChessClassFlags;

typedef struct ChessMovementSkill {
    int move_type;       /* +0x00 */
    unsigned int limits; /* +0x04 - two packed 16-bit script arguments */
    int flags;           /* +0x08 */
} ChessMovementSkill; /* 0x0C */

typedef struct ChessClassDefinition {
    float initial_power; /* +0x000 */
    ChessMovementSkill movement_skills[MK_CHESS_MOVEMENT_SKILL_COUNT]; /* +0x004 */
    unsigned int movement_skill_count; /* +0x064 */
    union {
        ChessPieceEventScript event_scripts[MK_CHESS_PIECE_EVENT_COUNT];
        unsigned int event_script_words[MK_CHESS_PIECE_EVENT_COUNT];
    }; /* +0x068 - retail initializes unused slots to 0xABABAB00 */
    union {
        struct { ChessClassFlags flags; char pad169[3]; };
        unsigned int flags_word;
    }; /* +0x168 */
    unsigned int field_16C;
    ChessScreenRef portraits[2]; /* +0x170 */
    const struct ChessClassText* text; /* +0x180 */
    const struct ChessSpellDefinition* spell_definitions; /* +0x184 */
} ChessClassDefinition; /* 0x188 */

typedef struct ChessTeamDefinition {
    int next_record_fallback_class; /* +0x00 - selected through prior record +0x1C */
    int field_04;
    int field_08;
    int class_slot_5; /* +0x0C */
    int class_slot_2; /* +0x10 */
    int class_slot_3_4; /* +0x14 */
    int class_slot_1; /* +0x18 */
} ChessTeamDefinition; /* 0x1C */

typedef struct ChessPieceFlags {
    unsigned char snap_into_stance : 1;   /* bit7 */
    unsigned char glitch_into_stance : 1; /* bit6 */
    unsigned char dont_constrain : 1;     /* bit5 */
    unsigned char unknown_bit4 : 1;
    unsigned char event_pending : 1; /* bit3 */
    unsigned char unknown_bit2 : 1;
    unsigned char pad : 2;
} ChessPieceFlags;

typedef struct ChessMovementEvent {
    unsigned char coordinates[6];
    char pad06[2];
    struct ChessPiece* piece;
    float value;
} ChessMovementEvent; /* 0x10 */

typedef struct ChessPieceMovement {
    MkHdr hdr;
    ChessAnimPdata* animation;
    struct ChessPiece* piece;
    union {
        struct {
            unsigned char cell_x; /* +0x10 - script movement destination */
            unsigned char cell_y;
            unsigned char event_byte_12;
            unsigned char event_byte_13;
            unsigned char event_byte_14;
            unsigned char event_byte_15;
            char pad16[2];
            struct ChessPiece* event_piece; /* +0x18 */
            float event_value; /* +0x1C */
        };
        ChessMovementEvent event_data; /* +0x10 */
    };
    float desired_cell_blend; /* +0x20 */
} ChessPieceMovement;

typedef struct ChessPieceRuntimeFields {
    unsigned int timer_0;
    Vec cell_offset; /* +0x04 */
    unsigned int primary_effect; /* +0x10 */
    unsigned int secondary_effect; /* +0x14 */
    int queued_event; /* +0x18 */
    int event_time; /* +0x1C */
    unsigned int event_script; /* +0x20 - command-script function index */
} ChessPieceRuntimeFields;

typedef union ChessPieceRuntime {
    unsigned int timer_slots[9];
    ChessPieceRuntimeFields fields;
} ChessPieceRuntime; /* 0x24 */

typedef struct ChessPiece {
    unsigned char id; /* +0x00 */
    char pad01[3];
    int current_event; /* +0x04 */
    union {
        unsigned int flags_word;
        struct { ChessPieceFlags flags; char pad09[3]; };
    }; /* +0x08 */
    MkObj* object; /* +0x0C */
    struct MkPtr* effects; /* +0x10 - owned effect list */
    int field_14; /* +0x14 - compared by piece-event selector 7 */
    int library_index; /* +0x18 */
    int state; /* +0x1C */
    float health; /* +0x20 */
    int type; /* +0x24 */
    unsigned char side; /* +0x28 */
    unsigned char cell_x; /* +0x29 */
    unsigned char cell_y; /* +0x2A */
    char pad2B;
    ChessPieceRuntime runtime; /* +0x2C */
    AniScript* normal_stance_script; /* +0x50 */
    AniScript* initial_stance_script; /* +0x54 - piece definition default */
    AniScript* requested_script; /* +0x58 */
    ChessAnimPdata* animation; /* +0x5C */
    union { int proc_state; struct MkProc* proc; }; /* +0x60 */
    unsigned int field_64;
    ChessPieceMovement* movement; /* +0x68 */
    struct ChessPieceMoveMap* move_map; /* +0x6C - packed three-bit cell values */
    unsigned int access_restrictions[6]; /* +0x70 */
    struct ChessPieceSpellData* spells; /* +0x88 - allocated spell rules */
    unsigned int used_spells; /* +0x8C - bit per cast spell */
} ChessPiece;

typedef struct ChessGameEventData {
    union {
        struct { ChessPiece* piece; ChessPiece* other_piece; };
        ChessPiece* pieces[2];
    };
} ChessGameEventData;

typedef struct ChessCell {
    Vec position; /* +0x00 */
    ChessPiece* piece; /* +0x0C */
    MkObj* object; /* +0x10 - special-cell visual */
    unsigned int emitter; /* +0x14 */
    unsigned int second_emitter; /* +0x18 */
    int square_type; /* +0x1C */
    unsigned char flags; /* +0x20 - emitter bits 7/6 and hidden bit 5 */
    char pad21[3];
    float saved_parameters[4]; /* +0x24 - serialized special-cell parameters */
} ChessCell; /* 0x34 */

typedef struct ChessBoardRow {
    ChessCell cells[MK_CHESS_BOARD_COLUMNS];
} ChessBoardRow; /* 0x208 */

typedef struct ChessSideHudState {
    char pad00[8];
    union {
        struct { unsigned char flags; unsigned char flags_09; char pad0A[2]; };
        unsigned int flags_word;
    }; /* +0x08 - bit7 requests cursor update */
    unsigned int side; /* +0x0C */
    unsigned char cell_x; /* +0x10 */
    unsigned char cell_y; /* +0x11 */
    char pad12[2];
    float cursor_scale_step; /* +0x14 */
    ChessPiece* selected_piece; /* +0x18 */
    ChessPiece* saved_piece; /* +0x1C - selection suspended by spell HUD */
} ChessSideHudState;



typedef struct ChessSideState {
    MkHdr hdr;
    /* Live pieces grow from the front; captured pieces grow from the back. */
    ChessPiece* pieces[17]; /* +0x08 */
    unsigned int live_piece_count; /* +0x4C */
    unsigned int captured_piece_count; /* +0x50 */
    MkHdr* input_data; /* +0x54 - input process payload */
    struct MkProc* input_proc; /* +0x58 */
    struct MkProc* team_proc; /* +0x5C */
    union { ChessSideHudState* hud; MkHdr* hud_header; }; /* +0x60 */
    union { unsigned int field_64; struct MkProc* drone_proc; };
    union { struct ChessSideController* controller; MkHdr* controller_header; }; /* +0x68 */
    ChessScreenRef team_art[5]; /* +0x6C */
    ChessScreenRef portraits[6]; /* +0x94 - one latch per chess class */
    unsigned int field_C4[6][2]; /* +0xC4 - paired words cleared per class */
    MkObj* team_model; /* +0xF4 */
    unsigned int team_model_instance; /* +0xF8 */
    unsigned int saved_field_FC;
    unsigned int saved_field_100;
    unsigned int field_104;
    char pad108[8];
    unsigned int saved_field_110;
    unsigned int saved_state_114[6];
    union {
        unsigned int strategy_state[6]; /* +0x12C */
        struct {
            unsigned int strategy; /* +0x12C - shared with the drone state */
            char pad130[0x14];
        };
    };
    unsigned int field_144;
    char pad148[0x1C];
    unsigned int field_164;
    char pad168[0x0C];
    unsigned int desired_x; /* +0x174 */
    unsigned int desired_y; /* +0x178 */
} ChessSideState;

typedef struct ChessSideControllerFlags {
    unsigned char drone_controlled : 1; /* bit7 */
    unsigned char bit6 : 1;
    unsigned char trap_placement_active : 1; /* bit5 */
    unsigned char pad : 5;
} ChessSideControllerFlags;

typedef struct ChessSideController {
    char pad00[8];
    union {
        struct { ChessSideControllerFlags flags; char pad09[3]; };
        unsigned int flags_word;
    }; /* +0x08 */
    unsigned int side; /* +0x0C */
} ChessSideController; /* 0x10-byte process allocation */

typedef struct ChessSaveFlags {
    unsigned char board_input_seen : 1; /* bit7 */
    unsigned char pad : 7;
} ChessSaveFlags;

typedef struct ChessSaveInputFlags {
    unsigned char pad_bit7 : 1;
    unsigned char p1_power_squares : 2; /* bits6-5 */
    unsigned char p2_power_squares : 2; /* bits4-3 */
    unsigned char pad_bits2_1 : 2;
    unsigned char input_locked : 1; /* bit0 */
} ChessSaveInputFlags;

typedef struct ChessSpellState {
    char pad00[8];
    unsigned int side; /* +0x08 */
    int state; /* +0x0C */
    char pad10[8];
    int input_state; /* +0x18 */
    char pad1C[4];
    unsigned int caster_index; /* +0x20 - ordinal among spellcasters */
    unsigned int spell_number; /* +0x24 */
    ChessPiece* caster; /* +0x28 */
    char pad2C[4];
    unsigned int field_30; /* +0x30 - current slot passed to spell lookup */
    char pad34[4];
    unsigned int target_rules; /* +0x38 - category and class exclusions */
    char pad3C[4];
    ChessPiece* temporary_piece; /* +0x40 */
    unsigned int target_x[MK_CHESS_SPELL_TARGET_COUNT]; /* +0x44 */
    unsigned int target_y[MK_CHESS_SPELL_TARGET_COUNT]; /* +0x4C */
    char pad54[8];
    int rescue_piece_type; /* +0x5C */
    char pad60[4]; /* +0x60 - stack HUD scratch extends to 0x64 */
} ChessSpellState;

typedef struct ChessSaveSide {
    int restore_pending; /* +0x00 - global save restore latch on side zero */
    ChessSaveInputFlags input_flags; /* +0x04 */
    ChessSaveFlags flags; /* +0x05 */
    char pad06[0x0E];
    int field_14; /* +0x14 - remaining team piece count */
    char pad18[0x6EC];
    int field_704;
    unsigned int forced_fight_count; /* +0x708 */
    unsigned int fight_stat_70C;
    unsigned int fight_stat_710;
    unsigned int fight_stat_714;
    char pad718[0x30];
} ChessSaveSide; /* 0x748 */

typedef struct ChessSavedEffect {
    int kind;
    unsigned int expiry_clock;
} ChessSavedEffect;

typedef struct ChessSavedPiece {
    unsigned char id;
    char pad01[3];
    int type;
    unsigned char cell_x;
    unsigned char cell_y;
    char pad0A[2];
    float health;
    int library_index;
    unsigned int used_spells;
    int event_pending;
    int queued_event;
    int event_time;
    unsigned int access_restrictions[6];
    ChessSavedEffect effects[5];
    unsigned int effect_count;
} ChessSavedPiece; /* 0x68 */

typedef struct ChessSavedTeam {
    unsigned int live_piece_count;
    unsigned int captured_piece_count;
    ChessSavedPiece pieces[17];
    char pad6F0[0x14];
    unsigned int saved_field_FC;
    unsigned int saved_field_100;
    unsigned int saved_field_110;
    unsigned int strategy_state[6];
    unsigned int saved_state_114[6];
    unsigned int desired_x;
    unsigned int desired_y;
} ChessSavedTeam; /* 0x748 */

typedef struct ChessBoardSave {
    union {
        /* Legacy shifted view retained for existing accessors. */
        struct { ChessSaveSide sides[2]; char padE90[0x14]; };
        struct {
            int restore_pending;
            ChessSaveInputFlags input_flags;
            ChessSaveFlags flags;
            char pad06[2];
            Vec camera_position;
            ChessSavedTeam teams[2]; /* +0x14 */
        };
    };
    /* Retail reads this big-endian packed storage at byte, halfword and word widths. */
    union {
        unsigned char ai_settings[4]; /* +0xEA4 - packed difficulty/king flags */
        unsigned short ai_settings_halves[2];
        unsigned int ai_settings_word;
    };
    unsigned char active_x[2];
    unsigned char active_y[2];
    unsigned int active_side;
    unsigned int fighter_ids[2];
    float player_health[2]; /* +0xEB8 */
    float knowledge_health[2]; /* +0xEC0 - health baselines for skill learning */
    unsigned int saved_clock;
    unsigned int saved_field_110;
    int saved_spell_clock;
    unsigned int saved_field_118;
    int winning_side; /* +0xED8 */
    struct {
        int type;
        float x;
        float y;
        float z;
        float scale;
    } cells[10][10]; /* +0xEDC */
    unsigned char origin_x, origin_y, destination_x, destination_y;
    int field_16B0;
    int field_16B4;
    int profile_stat_ceiling; /* +0x16B8 - initialized to 100000 */
    int field_16BC;
    int last_update_tick; /* +0x16C0 */
} ChessBoardSave; /* 0x16C4 */

typedef struct ChessBattlePieceRow {
    int index;
    unsigned int character_number; /* +0x04 */
    char pad08[8];
} ChessBattlePieceRow; /* 0x10 */

typedef struct ChessBoardGameController {
    struct ScriptSlot* command_script; /* +0x000 */
    ChessClassDefinition class_definitions[6]; /* +0x004 */
    union {
        ChessBattlePieceRow* battle_piece_rows;
        struct ChessLibraryEntry* piece_libraries;
    }; /* +0x934 */
    AniScript** piece_art_rows; /* +0x938 */
} ChessBoardGameController; /* 0x93C */

typedef struct ChessManagerInfo {
    unsigned char flags; /* +0x00 - bit5 suppresses the next power-cell announcement */
    char pad01[3];
    unsigned int active_side; /* +0x04 */
    unsigned int winning_side; /* +0x08 - copied to saved result */
    int input_state; /* +0x0C */
    int saved_input_state; /* +0x10 - restored by mode 7 */
    ChessPiece* active_piece_by_side[2]; /* +0x14 */
    ChessGameEventData event_data; /* +0x1C */
    ChessPiece* event_piece_24; /* +0x24 - alternate piece for event selector 7 */
    union {
        ChessScreenRef spell_hud[19]; /* +0x28..+0xBF */
        struct {
            ChessScreenRef bar_28;
            ChessScreenRef bar_30;
            ChessScreenRef bar_38;
            char pad40[0x40];
            ScreenObj* hud_cursor; /* +0x80 */
            unsigned int hud_cursor_instance; /* +0x84 */
            char pad88[0x38];
        };
    };
    ChessSpellState* spell; /* +0xC0 */
    struct ChessDirectionState* directional_state; /* +0xC4 */
    int clock; /* +0xC8 */
} ChessManagerInfo;

typedef struct ChessCursor {
    union { int state; MkHdr* object; };
    union { int selection; unsigned int object_instance; };
    unsigned char cell_x; /* +0x08 */
    unsigned char cell_y; /* +0x09 */
    char pad0A[2];
} ChessCursor; /* 0x0C */

typedef struct ChessDirectionState {
    MkHdr hdr;
    struct MkPtr* strings; /* +0x08 */
    ScreenObj* title; /* +0x0C */
    ChessCursor cursors[2]; /* +0x10 - one per side */
    int actions[2]; /* +0x28 */
    int trap_x[2]; /* +0x30 */
    int trap_y[2]; /* +0x38 */
} ChessDirectionState; /* 0x40-byte allocation */

typedef struct ChessCameraInfo {
    int viewing_quadrant; /* +0x00 */
    Vec current_look_at; /* +0x04 */
    float (*completion)(void); /* +0x10 */
    float (*look_at_completion)(void); /* +0x14 */
    ChessPiece* zoom_camera; /* +0x18 */
    ChessPiece* viewing_camera; /* +0x1C */
    char pad20[0x0C];
    Vec saved_position; /* +0x2C */
    Vec desired_look_at; /* +0x38 */
    int look_at_ticks; /* +0x44 */
    int zoom_sound_enabled; /* +0x48 */
    int field_4C; /* input L1 gate; initialized to zero */
    int field_50; /* input L1 gate; initialized to zero */
} ChessCameraInfo; /* 0x54 */

typedef struct ChessCameraSoundState {
    int field_00;
    MslSoundHandle zoom_sound; /* +0x04 */
} ChessCameraSoundState;

typedef struct ChessLibraryEntry {
    const char* name; /* +0x00 */
    int character_id; /* +0x04 */
    union {
        char pad08[8];
        struct {
            unsigned int field_08; /* +0x08 - nonzero offsets right portrait by 128 */
            int sound_variant; /* +0x0C - selects paired chess voice sounds */
        };
    };
} ChessLibraryEntry; /* 0x10-byte library table stride */

typedef struct ChessModeState {
    MkHdr hdr;
    ChessCursor cursor; /* +0x08 */
    ChessCursor cursors[MK_CHESS_CURSOR_COUNT]; /* +0x14 */
    ChessBoardRow* board; /* +0x38 */
    ChessSideState* sides[2]; /* +0x3C */
    ChessManagerInfo manager; /* +0x44 */
    unsigned int saved_field_110; /* +0x110 */
    int spell_completion_clock; /* +0x114 */
    unsigned int saved_field_118; /* +0x118 */
    unsigned int cursor_track; /* +0x11C - manager +0xD8 */
    struct MkPtr* tracked_sounds; /* +0x120 */
    int fight_start_tick; /* +0x124 */
    unsigned int turn_timeout; /* +0x128 */
    int input_transition_busy; /* +0x12C */
    ChessCameraInfo camera; /* +0x130 */
    ChessCameraSoundState camera_sound; /* +0x184 */
    unsigned int loaded_library_count; /* +0x18C */
    int loaded_character_ids[10]; /* +0x190 */
    ChessLibraryEntry* last_loaded_library; /* +0x1B8 */
} ChessModeState;

enum ChessPieceInfo {
    MK_CHESS_PIECE_INFO_X = 0,
    MK_CHESS_PIECE_INFO_Y = 1,
    MK_CHESS_PIECE_INFO_Z = 2,
    MK_CHESS_PIECE_INFO_Y_ANGLE = 3,
};

enum ChessSpellStateValue {
    MK_CHESS_SPELL_WAITING_FOR_COMPLETION = 0x15,
    MK_CHESS_SPELL_WAITING_FOR_FIGHT = 0x16,
    MK_CHESS_SPELL_COMPLETE = 0x17,
};

int mk_chess_fetch_active_defined_teams_class(int class_slot);
int mk_chess_fetch_active_defined_team(void);
void mk_chess_make_spellcaster(const struct ChessSpellDefinition* definitions);
void mk_chess_set_piece_event_script(unsigned int event,
                                     ChessPieceEventScript script);
void mk_chess_add_movement_skill(int move_type, unsigned int limit_a,
                                 unsigned int limit_b, int flags);
void mk_chess_define_class_initial_power(float power);
void mk_chess_set_normal_stance_script(AniScript* script);
void mk_chess_spell_has_completed_but_wait_for_fight(void);
void mk_chess_spell_has_completed(void);
void mk_chess_spell_rescue_current_target(void);
void mk_chess_spell_target_add_access_restrictions(unsigned int target,
                                                   unsigned int restriction,
                                                   int duration, int reset);
void mk_chess_spell_force_fight(void);
int mk_chess_spell_is_this_a_forced_fight(void);
void mk_chess_spell_kill_target(unsigned int target);
void mk_chess_spell_move_target_from_temp_area_to(unsigned int target);
void mk_chess_spell_move_target_to_temp_area(unsigned int target);
void mk_chess_spell_move_target_to_target(unsigned int source_target,
                                          unsigned int destination_target);
float mk_chess_spell_get_target_health(unsigned int target);
void mk_chess_spell_set_target_health(unsigned int target, float health);
float mk_chess_spell_get_target_max_health(unsigned int target);
void mk_chess_spell_show_target_portrait(unsigned int target);
void mk_chess_set_piece_info(int info, float value);
float mk_chess_get_piece_info(int info);
void mk_chess_ani_1_frame(void);
void mk_chess_piece_match_y_ang_to_anim(void);
int mk_chess_input_possibile(void);
int mk_chess_return_active_pad(void);
int mk_chess_allow_cam_control(void);
int mk_chess_fake_demo_cam(float* camera_speed);
void mk_chess_glitch_to_ani_frame(int animation, int flags, float speed,
                                  float frame);
void mk_chess_blend_to_ani_frame(int animation, int flags, float blend,
                                 float speed, float frame);
void mk_chess_set_ani_speed(float speed);
void mk_chess_set_obj_move_weight(float weight);
void mk_chess_blend_to_ani(int animation, int flags, float blend, float speed);
void mk_chess_air_move(void);
void mk_chess_blend_to_my_cell_pos(float distance);
void mk_chess_snap_into_cell_orgin_over_x_frames(float frames);
void mk_chess_put_active_piece_at_cell(int snap, float x, float y);
void mk_chess_blend_to_desired_cell_position_setting(float blend);
void mk_chess_queue_up_piece_event(int event, int delay);
void mk_chess_blend_to_normal_stance(void);
void mk_chess_set_cell_offset(float x, float y, float z);
void mk_chess_set_piece_state(int state);
void mk_chess_set_glitch_stance_flag(void);
int mk_chess_check_glitch_into_stance(void);
int mk_chess_check_snap_into_stance(void);
void mk_chess_dont_constrain_piece(void);
void mk_chess_piece_set_state(int state);
void mk_chess_piece_is_idle(void);
ChessPiece* mk_chess_fetch_piece_at_cursor(void);
int mk_chess_fetch_current_side_based_on_ones(unsigned int side);
ChessManagerInfo* mk_chess_fetch_manager_info(void);
ChessCameraInfo* mk_chess_fetch_camera_info(void);
int mk_chess_allow_setting_of_viewing_quadrant(void);
void mk_chess_set_viewing_quadrant(struct CameraObj* camera);
void mk_chess_enable_cam_zoom_sound(int enabled);
void mk_chess_cleanup(void);
void mk_chess_in_fight_setup(void);
float p_board_switch_4(void);
float p_board_switch_over_3(void);
float p_board_switch_3(void);
float p_board_switch_2(void);
float p_board_switch_1(void);
float p_board_switch_r2(void);
float p_board_switch_l1(void);

#endif
