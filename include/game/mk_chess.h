#ifndef MKD_GAME_MK_CHESS_H
#define MKD_GAME_MK_CHESS_H

#include "msl/msl_types.h"
#include "runtime/mk_obj.h"

#define MK_CHESS_MOVEMENT_SKILL_COUNT 8
#define MK_CHESS_PIECE_EVENT_COUNT 64
#define MK_CHESS_BOARD_COLUMNS 10
#define MK_CHESS_SPELL_TARGET_COUNT 2
#define MK_CHESS_CURSOR_COUNT 3

typedef void (*ChessPieceEventScript)(void);
typedef struct AniScript AniScript;
typedef struct ScreenObj ScreenObj;

typedef struct ChessAnimPdata {
    char pad00[0x38];
    float frame; /* +0x38 */
    char pad3C[4];
    float end_frame; /* +0x40 */
    float speed; /* +0x44 */
} ChessAnimPdata;

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
    ChessPieceEventScript event_scripts[MK_CHESS_PIECE_EVENT_COUNT]; /* +0x068 */
    ChessClassFlags flags; /* +0x168 */
    char pad169[0x1B];
    int spellcaster_type; /* +0x184 */
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
    unsigned char pad : 3;
} ChessPieceFlags;

typedef struct ChessPieceMovement {
    char pad00[0x20];
    float desired_cell_blend; /* +0x20 */
} ChessPieceMovement;

typedef struct ChessPieceRuntimeFields {
    unsigned int timer_0;
    Vec cell_offset; /* +0x04 */
    char pad10[8];
    int queued_event; /* +0x18 */
    int event_time; /* +0x1C */
    char pad20[4];
} ChessPieceRuntimeFields;

typedef union ChessPieceRuntime {
    unsigned int timer_slots[9];
    ChessPieceRuntimeFields fields;
} ChessPieceRuntime; /* 0x24 */

typedef struct ChessPiece {
    unsigned char id; /* +0x00 */
    char pad01[3];
    int current_event; /* +0x04 */
    ChessPieceFlags flags; /* +0x08 */
    char pad09[3];
    MkObj* object; /* +0x0C */
    char pad10[0x0C];
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
    int proc_state; /* +0x60 */
    char pad64[4];
    ChessPieceMovement* movement; /* +0x68 */
    char pad6C[4];
    unsigned int access_restrictions[6]; /* +0x70 */
} ChessPiece;

typedef struct ChessGameEventData {
    ChessPiece* piece;
    unsigned int player;
} ChessGameEventData;

typedef struct ChessCell {
    Vec position; /* +0x00 */
    ChessPiece* piece; /* +0x0C */
    char pad10[0x0C];
    int square_type; /* +0x1C */
    char pad20[0x14];
} ChessCell; /* 0x34 */

typedef struct ChessBoardRow {
    ChessCell cells[MK_CHESS_BOARD_COLUMNS];
} ChessBoardRow; /* 0x208 */

typedef struct ChessSideState {
    char pad00[8];
    ChessPiece* deadpool[17]; /* +0x08 */
    unsigned int deadpool_count; /* +0x4C */
    char pad50[8];
    struct MkProc* input_proc; /* +0x58 */
    char pad5C[0x0C];
    struct ChessSideController* controller; /* +0x68 */
    struct {
        ScreenObj* screen;
        unsigned int instance;
    } team_art[5]; /* +0x6C */
    struct {
        ScreenObj* screen;
        unsigned int instance;
    } portraits[6]; /* +0x94 - one latch per chess class */
    char padC4[0x30];
    MkObj* team_model; /* +0xF4 */
    unsigned int team_model_instance; /* +0xF8 */
} ChessSideState;

typedef struct ChessSideControllerFlags {
    unsigned char drone_controlled : 1; /* bit7 */
    unsigned char pad : 7;
} ChessSideControllerFlags;

typedef struct ChessSideController {
    char pad00[8];
    ChessSideControllerFlags flags; /* +0x08 */
} ChessSideController;

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
    char pad1C[0x24];
    ChessPiece* temporary_piece; /* +0x40 */
    unsigned int target_x[MK_CHESS_SPELL_TARGET_COUNT]; /* +0x44 */
    unsigned int target_y[MK_CHESS_SPELL_TARGET_COUNT]; /* +0x4C */
    char pad54[8];
    int rescue_piece_type; /* +0x5C */
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

typedef struct ChessBoardSave {
    ChessSaveSide sides[2]; /* +0x000 */
    char padE90[0x14];
    unsigned char ai_settings[4]; /* +0xEA4 - packed difficulty/king flags */
    char padEA8[0x10];
    float player_health[2]; /* +0xEB8 */
    char padEC0[0x18];
    int winning_side; /* +0xED8 */
    struct {
        int type;
        float x;
        float y;
        float z;
        float scale;
    } cells[10][10]; /* +0xEDC */
    char pad16AC[4];
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
    ChessBattlePieceRow* battle_piece_rows; /* +0x934 */
    void* piece_art_rows; /* +0x938 */
} ChessBoardGameController; /* 0x93C */

typedef struct ChessManagerInfo {
    char pad00[4];
    unsigned int active_side; /* +0x04 */
    char pad08[4];
    int input_state; /* +0x0C */
    char pad10[4];
    ChessPiece* active_piece_by_side[2]; /* +0x14 */
    ChessGameEventData event_data; /* +0x1C */
    char pad24[0x5C];
    ScreenObj* hud_cursor; /* +0x80 */
    unsigned int hud_cursor_instance; /* +0x84 */
    char pad88[0x38];
    ChessSpellState* spell; /* +0xC0 */
    int* directional_actions; /* +0xC4 */
    int clock; /* +0xC8 */
} ChessManagerInfo;

typedef struct ChessCursor {
    int state;
    int selection;
    unsigned char cell_x; /* +0x08 */
    unsigned char cell_y; /* +0x09 */
    char pad0A[2];
} ChessCursor; /* 0x0C */

typedef struct ChessCameraInfo {
    int viewing_quadrant; /* +0x00 */
    Vec current_look_at; /* +0x04 */
    float (*completion)(void); /* +0x10 */
    float (*look_at_completion)(void); /* +0x14 */
    ChessPiece* zoom_camera; /* +0x18 */
    ChessPiece* viewing_camera; /* +0x1C */
    char pad20[0x18];
    Vec desired_look_at; /* +0x38 */
    int look_at_ticks; /* +0x44 */
    int zoom_sound_enabled; /* +0x48 */
    char pad4C[8];
} ChessCameraInfo; /* 0x54 */

typedef struct ChessCameraSoundState {
    int field_00;
    MslSoundHandle zoom_sound; /* +0x04 */
} ChessCameraSoundState;

typedef struct ChessModeState {
    char pad00[0x14];
    ChessCursor cursors[MK_CHESS_CURSOR_COUNT]; /* +0x14 */
    ChessBoardRow* board; /* +0x38 */
    ChessSideState* sides[2]; /* +0x3C */
    ChessManagerInfo manager; /* +0x44 */
    char pad110[4];
    int spell_completion_clock; /* +0x114 */
    char pad118[0x10];
    unsigned int turn_timeout; /* +0x128 */
    int input_transition_busy; /* +0x12C */
    ChessCameraInfo camera; /* +0x130 */
    ChessCameraSoundState camera_sound; /* +0x184 */
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
void mk_chess_make_spellcaster(int spellcaster_type);
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
void mk_chess_air_move(void);
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
