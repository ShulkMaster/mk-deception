#ifndef MKD_GAME_MK_CHESS_H
#define MKD_GAME_MK_CHESS_H

float p_board_switch_4(void);
float p_board_switch_over_3(void);
float p_board_switch_3(void);
float p_board_switch_1(void);
float p_board_switch_r2(void);
float p_board_switch_l1(void);
float p_board_switch_2(void);

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
struct AnimPdata;

typedef struct ChessRestriction {
    unsigned int expiry_clock;
} ChessRestriction;

typedef struct ChessScreenRef {
    ScreenObj* screen;
    unsigned int instance;
} ChessScreenRef;

typedef struct ChessClassFlags {
    unsigned char spellcaster : 1;
    unsigned char pad : 7;
} ChessClassFlags;

typedef struct ChessMovementSkill {
    int move_type;
    unsigned int limits;
    int flags;
} ChessMovementSkill;

typedef struct ChessClassDefinition {
    float initial_power;
    ChessMovementSkill movement_skills[MK_CHESS_MOVEMENT_SKILL_COUNT];
    unsigned int movement_skill_count;
    union {
        ChessPieceEventScript event_scripts[MK_CHESS_PIECE_EVENT_COUNT];
        unsigned int event_script_words[MK_CHESS_PIECE_EVENT_COUNT];
    };
    union {
        struct { ChessClassFlags flags; char pad169[3]; };
        unsigned int flags_word;
    };
    unsigned int field_16C;
    ChessScreenRef portraits[2];
    const struct ChessClassText* text;
    const struct ChessSpellDefinition* spell_definitions;
} ChessClassDefinition;

typedef struct ChessTeamDefinition {
    int next_record_fallback_class;
    int field_04;
    int field_08;
    int class_slot_5;
    int class_slot_2;
    int class_slot_3_4;
    int class_slot_1;
} ChessTeamDefinition;

typedef struct ChessPieceFlags {
    unsigned char snap_into_stance : 1;
    unsigned char glitch_into_stance : 1;
    unsigned char dont_constrain : 1;
    unsigned char unknown_bit4 : 1;
    unsigned char event_pending : 1;
    unsigned char unknown_bit2 : 1;
    unsigned char pad : 2;
} ChessPieceFlags;

typedef struct ChessMovementEvent {
    unsigned char coordinates[6];
    char pad06[2];
    struct ChessPiece* piece;
    float value;
} ChessMovementEvent;

typedef struct ChessPieceMovement {
    MkHdr hdr;
    struct AnimPdata* animation;
    struct ChessPiece* piece;
    union {
        struct {
            unsigned char cell_x;
            unsigned char cell_y;
            unsigned char event_byte_12;
            unsigned char event_byte_13;
            unsigned char event_byte_14;
            unsigned char event_byte_15;
            char pad16[2];
            struct ChessPiece* event_piece;
            float event_value;
        };
        ChessMovementEvent event_data;
    };
    float desired_cell_blend;
} ChessPieceMovement;

typedef struct ChessPieceRuntimeFields {
    unsigned int timers[1];
    Vec cell_offset;
    unsigned int primary_effect;
    unsigned int secondary_effect;
    int queued_event;
    unsigned int event_time;
    unsigned int event_script;
} ChessPieceRuntimeFields;

typedef union ChessPieceRuntime {
    unsigned int timer_slots[9];
    ChessPieceRuntimeFields fields;
} ChessPieceRuntime;

typedef struct ChessPiece {
    unsigned char id;
    char pad01[3];
    int current_event;
    union {
        unsigned int flags_word;
        struct { ChessPieceFlags flags; char pad09[3]; };
    };
    MkObj* object;
    struct MkPtr* effects;
    int field_14;
    int library_index;
    int state;
    float health;
    int type;
    unsigned char side;
    unsigned char cell_x;
    unsigned char cell_y;
    char pad2B;
    ChessPieceRuntime runtime;
    AniScript* normal_stance_script;
    AniScript* initial_stance_script;
    AniScript* requested_script;
    struct AnimPdata* animation;
    union { int proc_state; struct MkProc* proc; };
    int field_64;
    ChessPieceMovement* movement;
    struct ChessPieceMoveMap* move_map;
    ChessRestriction access_restrictions[6];
    struct ChessPieceSpellData* spells;
    unsigned int used_spells;
} ChessPiece;

typedef struct ChessGameEventData {
    union {
        struct { ChessPiece* piece; ChessPiece* other_piece; };
        ChessPiece* pieces[2];
    };
} ChessGameEventData;

typedef struct ChessCell {
    Vec position;
    ChessPiece* piece;
    MkObj* object;
    unsigned int emitter;
    unsigned int second_emitter;
    int square_type;
    union {
        unsigned int flags_word;
        unsigned char flags;
        struct {
            unsigned char emitter_active : 1;
            unsigned char second_emitter_active : 1;
            unsigned char hidden : 1;
            unsigned char pad : 5;
        } flag_bits;
    };
    float saved_parameters[4];
} ChessCell;

typedef struct ChessBoardRow {
    ChessCell cells[MK_CHESS_BOARD_COLUMNS];
} ChessBoardRow;

typedef struct ChessSideHudState {
    MkHdr hdr;
    union {
        struct {
            unsigned char flags;
            union {
                unsigned char flags_09;
                struct {
                    unsigned char sliding : 1;
                    unsigned char slide_out : 1;
                    unsigned char turn_forfeit : 1;
                    unsigned char : 5;
                } team_art_bits;
            };
        };
        struct {
            unsigned char cursor_update : 1;
            unsigned char cursor_scaling : 1;
            unsigned char cursor_moving : 1;
            unsigned char cursor_mode : 2;
            unsigned char cursor_hidden : 1;
            unsigned char slide_active : 1;
            unsigned char slide_out : 1;
        } flag_bits;
        unsigned int flags_word;
    };
    unsigned int side;
    unsigned char cell_x;
    unsigned char cell_y;
    char pad12[2];
    float cursor_scale_step;
    ChessPiece* selected_piece;
    ChessPiece* saved_piece;
} ChessSideHudState;

typedef struct ChessSideState {
    MkHdr hdr;
    ChessPiece* pieces[17];
    unsigned int live_piece_count;
    unsigned int captured_piece_count;
    MkHdr* input_data;
    struct MkProc* input_proc;
    struct MkProc* team_proc;
    union { ChessSideHudState* hud; MkHdr* hud_header; };
    union { unsigned int field_64; struct MkProc* drone_proc; };
    union { struct ChessSideController* controller; MkHdr* controller_header; };
    ChessScreenRef team_art[5];
    ChessScreenRef portraits[6];
    unsigned int field_C4[6][2];
    MkObj* team_model;
    unsigned int team_model_instance;
    unsigned int saved_field_FC;
    unsigned int saved_field_100;
    unsigned int field_104;
    char pad108[8];
    unsigned int saved_field_110;
    unsigned int saved_state_114[6];
    union {
        unsigned int strategy_state[6];
        struct {
            unsigned int strategy;
            char pad130[0x14];
        };
    };
    unsigned int field_144;
    char pad148[0x1C];
    int field_164;
    char pad168[0x0C];
    unsigned int desired_x;
    unsigned int desired_y;
} ChessSideState;

typedef struct ChessSideControllerFlags {
    unsigned char drone_controlled : 1;
    unsigned char bit6 : 1;
    unsigned char trap_placement_active : 1;
    unsigned char pad : 5;
} ChessSideControllerFlags;

typedef struct ChessSideController {
    MkHdr hdr;
    union {
        struct { ChessSideControllerFlags flags; char pad09[3]; };
        unsigned int flags_word;
    };
    unsigned int side;
} ChessSideController;

typedef struct ChessSaveFlags {
    unsigned char board_input_seen : 1;
    unsigned char pad : 7;
} ChessSaveFlags;

typedef struct ChessSaveInputFlags {
    unsigned char pad_bit7 : 1;
    unsigned char p1_power_squares : 2;
    unsigned char p2_power_squares : 2;
    unsigned char p1_on_power_square : 1;
    unsigned char p2_on_power_square : 1;
    unsigned char input_locked : 1;
} ChessSaveInputFlags;

typedef struct ChessSpellState {
    MkHdr hdr;
    unsigned int side;
    int state;
    char pad10[8];
    int input_state;
    char pad1C[4];
    unsigned int caster_index;
    unsigned int spell_number;
    ChessPiece* caster;
    char pad2C[4];
    unsigned int field_30;
    char pad34[4];
    unsigned int target_rules;
    char pad3C[4];
    ChessPiece* temporary_piece;
    unsigned int target_x[MK_CHESS_SPELL_TARGET_COUNT];
    unsigned int target_y[MK_CHESS_SPELL_TARGET_COUNT];
    char pad54[8];
    int rescue_piece_type;
    char pad60[4];
} ChessSpellState;

typedef struct ChessSaveSide {
    int restore_pending;
    ChessSaveInputFlags input_flags;
    ChessSaveFlags flags;
    char pad06[0x0E];
    int field_14;
    char pad18[0x6EC];
    int field_704;
    unsigned int forced_fight_count;
    unsigned int fight_stat_70C;
    unsigned int fight_stat_710;
    unsigned int fight_stat_714;
    char pad718[0x30];
} ChessSaveSide;

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
    ChessRestriction access_restrictions[6];
    ChessSavedEffect effects[5];
    unsigned int effect_count;
} ChessSavedPiece;

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
} ChessSavedTeam;

typedef struct ChessBoardSave {
    union {
        struct { ChessSaveSide sides[2]; char padE90[0x14]; };
        struct {
            int restore_pending;
            union {
                unsigned int control_flags_word;
                struct {
                    ChessSaveInputFlags input_flags;
                    ChessSaveFlags flags;
                    char pad06[2];
                };
            };
            Vec camera_position;
            ChessSavedTeam teams[2];
        };
    };
    union {
        unsigned char ai_settings[4];
        unsigned short ai_settings_halves[2];
        unsigned int ai_settings_word;
        struct {
            unsigned char p1_king_fight : 1;
            unsigned char p1_difficulty : 4;
            unsigned char p1_pad : 3;
            unsigned char p1_pad_lo : 1;
            unsigned char p2_king_fight : 1;
            unsigned char p2_difficulty : 4;
            unsigned char p2_pad : 2;
        } ai_bits;
        struct {
            unsigned short pad_hi : 5;
            unsigned short p1_type : 4;
            unsigned short pad_lo : 7;
        } ai_half_bits;
        struct {
            unsigned int pad_hi : 14;
            unsigned int p2_type : 4;
            unsigned int pad_lo : 14;
        } ai_word_bits;
    };
    unsigned char active_x[2];
    unsigned char active_y[2];
    unsigned int active_side;
    unsigned int fighter_ids[2];
    float player_health[2];
    float knowledge_health[2];
    unsigned int saved_clock;
    unsigned int saved_field_110;
    int saved_spell_clock;
    unsigned int saved_field_118;
    int winning_side;
    struct {
        int type;
        union {
            struct {
                float x;
                float y;
                float z;
                float scale;
            };
            float saved_parameters[4];
        };
    } cells[10][10];
    unsigned char origin_x, origin_y, destination_x, destination_y;
    int field_16B0;
    int field_16B4;
    int profile_stat_ceiling;
    int field_16BC;
    int last_update_tick;
} ChessBoardSave;

typedef struct ChessBattlePieceRow {
    int index;
    unsigned int character_number;
    char pad08[8];
} ChessBattlePieceRow;

typedef struct ChessBoardGameController {
    struct ScriptSlot* command_script;
    ChessClassDefinition class_definitions[6];
    union {
        ChessBattlePieceRow* battle_piece_rows;
        struct ChessLibraryEntry* piece_libraries;
    };
    AniScript** piece_art_rows;
} ChessBoardGameController;

typedef struct ChessManagerInfo {
    union {
        unsigned char flags;
        struct {
            unsigned char bit7 : 1;
            unsigned char bit6 : 1;
            unsigned char suppress_power_cell_announcement : 1;
            unsigned char pad : 5;
        } flag_bits;
    };
    char pad01[3];
    unsigned int active_side;
    unsigned int winning_side;
    int input_state;
    int saved_input_state;
    ChessPiece* active_piece_by_side[2];
    ChessGameEventData event_data;
    ChessPiece* event_piece_24;
    union {
        ChessScreenRef spell_hud[19];
        struct {
            ChessScreenRef bar_28;
            ChessScreenRef bar_30;
            ChessScreenRef bar_38;
            char pad40[0x40];
            ScreenObj* hud_cursor;
            unsigned int hud_cursor_instance;
            char pad88[0x38];
        };
    };
    ChessSpellState* spell;
    struct ChessDirectionState* directional_state;
    int clock;
    unsigned int saved_field_110;
    int spell_completion_clock;
    unsigned int saved_field_118;
    unsigned int cursor_track;
    struct MkPtr* tracked_sounds;
    int fight_start_tick;
} ChessManagerInfo;

typedef struct ChessCursor {
    union { int state; MkHdr* object; };
    union { int selection; unsigned int object_instance; };
    unsigned char cell_x;
    unsigned char cell_y;
    char pad0A[2];
} ChessCursor;

typedef struct ChessDirectionState {
    MkHdr hdr;
    struct MkPtr* strings;
    ScreenObj* title;
    ChessCursor cursors[2];
    int actions[2];
    int trap_x[2];
    int trap_y[2];
} ChessDirectionState;

typedef struct ChessCameraInfo {
    int viewing_quadrant;
    Vec current_look_at;
    float (*completion)(void);
    float (*look_at_completion)(void);
    ChessPiece* zoom_camera;
    ChessPiece* viewing_camera;
    char pad20[0x0C];
    Vec saved_position;
    Vec desired_look_at;
    unsigned int look_at_ticks;
    int zoom_sound_enabled;
    int field_4C;
    int field_50;
} ChessCameraInfo;

typedef struct ChessCameraSoundState {
    MslSoundHandle field_00;
    MslSoundHandle zoom_sound;
} ChessCameraSoundState;

typedef struct ChessLibraryEntry {
    const char* name;
    int character_id;
    unsigned int field_08;
    int sound_variant;
} ChessLibraryEntry;

typedef struct ChessModeState {
    MkHdr hdr;
    ChessCursor cursor;
    ChessCursor cursors[MK_CHESS_CURSOR_COUNT];
    ChessBoardRow* board;
    ChessSideState* sides[2];
    ChessManagerInfo manager;
    unsigned int turn_timeout;
    int input_transition_busy;
    ChessCameraInfo camera;
    ChessCameraSoundState camera_sound;
    unsigned int loaded_library_count;
    int loaded_character_ids[10];
    ChessLibraryEntry* last_loaded_library;
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
void mk_chess_put_active_piece_at_cell(float x, float y, int snap);
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
int mk_chess_check_for_fatality(void);
float p_board_switch_4(void);
float p_board_switch_over_3(void);
float p_board_switch_3(void);
float p_board_switch_2(void);
float p_board_switch_1(void);
float p_board_switch_r2(void);
float p_board_switch_l1(void);

struct ChessGameDefinition;
extern struct ChessGameDefinition g_chess_definition_info;
void mk_chess_advantage_hud(void);
void mk_chess_game_just_ended(void);
float mk_chess_transition_from_fight(void);
float p_mk_chess(void);

int mk_chess_fetch_bp_num_based_on_pchr_num(unsigned int character_number);
void mk_chess_launch_up(float vertical_speed, float gravity);
void mk_chess_stop_me(void);
MkObj* launch_fx_at_pos_with_obj( unsigned int effect, float x, float y, float z);
void mk_chess_rotate_towards_cell(float x, float y, float step, int track_other, float offset);
void mk_chess_piece_die(void);
void mk_chess_piece_event_from_script(int);
void mk_chess_place_special_cell_at(unsigned int x, unsigned int y, int type, float px, float py, float pz, float scale, int restored);
int mk_chess_xfer_piece_from_scripts(int, int, int);
void mk_chess_blend_into_cell_orgin_in_x_frames(float);
void mk_chess_force_away(float speed, int delay, float damping, int frames);
void mk_chess_ani_to_end(void);
void mk_chess_snap_to_stance(void);
void mk_chess_shifter_switch(int, int, float, float);
void mk_chess_launch_special_fx(unsigned int, unsigned int, unsigned int);
void mk_chess_activate_my_properties(void);
void mk_chess_snd_request(unsigned int event);
float mk_chess_get_piece_event_data(int);
void mk_chess_piece_temporarily_gone(void);
void mk_chess_ani_idle(void);
void mk_chess_set_piece_type_as(int, int);
void mk_chess_ani_until_reached_destination(float start_x, float start_y, float target_x, float target_y, float frames, int airborne);
void mk_chess_ani_to_blend_frame(float blend_frames);
int mk_chess_active_piece_near_edge(void);
void mk_chess_deactivate_my_properties(void);
void mk_chess_wait_until_attack_cam_closes_in(void);
int mk_chess_piece_test_and_set_timer(unsigned int timer_slot, unsigned int duration);
void mk_chess_ani_loop_more_frames(float frames);
void mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void);
void mk_chess_snap_to_my_cell_now(void);
void mk_chess_ani_to_frame_x(float target);
void mk_chess_init_piece(int slot, int character);
unsigned int mk_chess_request_piece_script_for_action(int action);
int mk_chess_should_i_fall_down(void);
MkObj* mk_chess_launch_fx_at_pos_with_obj_emit_based(unsigned int effect, float x, float y, float z);
void mk_chess_init_bgnd_for_fight_mode(void);
void mk_chess_set_breaker_value(void);
int mk_chess_did_the_king_just_lose(void);
#endif
