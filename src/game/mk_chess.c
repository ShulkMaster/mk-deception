#include "game/mk_chess.h"
#include "game/game_info.h"
#include "game/settings.h"
#include "game/menu.h"
#include "mw/mwScreenEngineGlue.h"
#include "game/attract.h"
#include "game/plyr.h"
#include "game/bgnd.h"
#include "runtime/light.h"
#include "game/controller.h"
#include "game/switch.h"
#include "platform/main.h"
#include "platform/display.h"
#include "platform/gcutils.h"
#include "math/gxMath.h"
#include "platform/main_jump.h"
#include "platform/io.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "runtime/asset.h"
#include "runtime/section.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/utils.h"
#include "runtime/sound_tracker.h"
#include "rw/rwresources.h"
#include "rw/rwcamera_internal.h"
#include "game/pfxscript.h"
#include "libmkparticle/pfx2d.h"

typedef struct ChessMoveTarget {
    unsigned int cell_x;
    unsigned int cell_y;
    int kind;
} ChessMoveTarget; /* 0x0C */

typedef struct ChessPieceMoveMap {
    MkHdr hdr;
    unsigned int rows[10]; /* +0x08 - ten three-bit cells per row */
    ChessMoveTarget targets[8]; /* +0x30 */
    unsigned int target_count; /* +0x90 */
} ChessPieceMoveMap; /* 0x94-byte allocation */

typedef union ChessEffectFlags {
    unsigned int word;
    unsigned char byte;
    struct {
        unsigned char bit7 : 1;
        unsigned char bit6 : 1;
        unsigned char bit5 : 1;
        unsigned char bit4 : 1;
        unsigned char remaining : 4;
    } bits;
} ChessEffectFlags;

typedef struct ChessPieceEffect {
    MkHdr hdr;
    ChessEffectFlags flags; /* +0x08 */
    MkObj* object; /* +0x0C */
    unsigned int object_instance; /* +0x10 */
    unsigned int emitter; /* +0x14 */
    TrackedSound* sound; /* +0x18 */
    unsigned int sound_instance; /* +0x1C */
    unsigned int expiry_clock; /* +0x20 */
    int kind; /* +0x24 */
} ChessPieceEffect; /* 0x28-byte allocation */

typedef struct ChessImprisonPdata {
    MkHdr hdr;
    ChessPieceEffect* effect;
    ChessPiece* piece;
} ChessImprisonPdata; /* 0x10-byte process allocation */

typedef struct ChessClassText {
    unsigned int class_index;
    char* portrait_name;
    unsigned int field_08;
    char* rescue_name; /* +0x0C - string passed to the rescue prompt */
    unsigned int init_script;
} ChessClassText;

typedef struct ChessSpellPageText {
    unsigned int line_ids[2];
    unsigned int title_id; /* +0x08 - target scroll caption */
} ChessSpellPageText;

typedef struct ChessSpellDefinition {
    unsigned int name_id;
    unsigned int target_rules[2]; /* +0x04 - first and second target */
    unsigned int target_access_flags;
    const ChessSpellPageText* pages[2];
    unsigned int enabled;
} ChessSpellDefinition; /* 0x1C */

typedef struct ChessPieceSpellData {
    MkHdr hdr;
    unsigned int target_access_flags[4]; /* +0x08 */
    const ChessSpellPageText* pages[4][2]; /* +0x18 */
} ChessPieceSpellData; /* 0x38-byte allocation */

typedef int (*ChessProcVtableFn)(void);
typedef int (*ChessProcJumpFn)(MkProcEntryFn entry, float ticks);

typedef struct ChessProcVtable {
    ChessProcVtableFn reserved[6];
    ChessProcVtableFn sleep;
    ChessProcVtableFn stack_ops[2];
    ChessProcJumpFn jump_sleep; /* +0x24 */
} ChessProcVtable;


typedef struct ChessPieceEventResult {
    int values[2];
} ChessPieceEventResult;

typedef struct ChessAttackGridPosition {
    unsigned char origin_x;
    unsigned char origin_y;
    unsigned char cell_x;
    unsigned char cell_y;
} ChessAttackGridPosition;

typedef struct ChessHudState {
    MkHdr hdr;
    unsigned int side;
    int state;
    int countdown; /* +0x10 */
    int field_14;
    int input_state; /* +0x18 */
    MkPtr* images; /* +0x1C */
    char pad20[4];
    unsigned int spell_number; /* +0x24 */
    ChessPiece* caster; /* +0x28 */
    int selected_name; /* +0x2C */
    int cursor_slot; /* +0x30 */
    int names_state; /* +0x34 */
    unsigned int target_rules; /* +0x38 */
    MkPtr* strings; /* +0x3C */
    ChessPiece* temporary_piece; /* +0x40 */
    unsigned int target_x[2]; /* +0x44 */
    unsigned int target_y[2]; /* +0x4C */
    int field_54;
    MkHdr* image_58;
    int rescue_piece_type; /* +0x5C */
} ChessHudState;

typedef struct ChessSpellHudPdata {
    MkHdr hdr;
    char pad08[8];
    ChessHudState* selected_spell;
    ChessHudState* target_1;
} ChessSpellHudPdata;

typedef struct ChessStringFadePdata {
    MkHdr hdr;
    MkPtr** strings; /* +0x08 - owning list head */
    float (*completed)(void); /* +0x0C */
    int step; /* +0x10 */
    ChessHudState* owner; /* +0x14 */
} ChessStringFadePdata; /* 0x18-byte process allocation */

typedef struct ChessImageFadePdata {
    MkHdr hdr;
    float (*completed)(void); /* +0x08 */
    MkPtr** images; /* +0x0C - owning list head */
    ChessHudState* owner; /* +0x10 */
} ChessImageFadePdata; /* 0x14-byte allocation */

typedef struct ChessPieceProcPdata {
    MkHdr hdr;
    ChessPiece* piece;
} ChessPieceProcPdata;

typedef struct ChessGroundBlastPdata {
    MkHdr hdr;
    MkObj* object; /* +0x08 */
    ChessPiece* piece; /* +0x0C */
    float elapsed; /* +0x10 */
    float maximum_scale; /* +0x14 */
    float fade_start_scale; /* +0x18 */
    float scale_per_tick; /* +0x1C */
    int alpha_step; /* +0x20 */
    int fade_ticks; /* +0x24 */
    int spawn_secondary; /* +0x28 */
} ChessGroundBlastPdata; /* 0x2C-byte allocation */

typedef struct ChessPieceFadePdata {
    MkHdr hdr;
    ChessPiece* piece; /* +0x08 */
    RwRGBA color; /* +0x0C */
    unsigned int target_alpha; /* +0x10 */
    unsigned int delay; /* +0x14 */
    int step; /* +0x18 */
} ChessPieceFadePdata; /* 0x1C, from the process allocation */

typedef struct ChessInputPdata {
    MkHdr hdr;
    unsigned int side;
    int state; /* +0x0C */
    int switch_index; /* +0x10 */
    int analog; /* +0x14 */
    unsigned int direction; /* +0x18 */
    unsigned int direction_delay; /* +0x1C */
} ChessInputPdata;

typedef struct ChessTurnExpiredPdata {
    MkHdr hdr;
    char pad08[4];
    unsigned int side; /* +0x0C */
    char pad10[0x10];
} ChessTurnExpiredPdata; /* 0x20-byte process allocation */

typedef struct ChessAttackBurstPdata {
    MkHdr hdr;
    int pebble_group;
    int pebble;
    float amount;
    float acceleration;
    float maximum;
} ChessAttackBurstPdata;

typedef struct ChessShowSidePdata {
    MkHdr hdr;
    ChessSideState* side;
    int pebble_group;
    float amount;
    float acceleration;
    float maximum;
    float current;
} ChessShowSidePdata;

typedef struct ChessBlinkStringPdata {
    MkHdr hdr;
    StringObj* text;
    unsigned int text_instance;
    int timer;
    int interval;
} ChessBlinkStringPdata;

typedef struct ChessBezierCameraState {
    char pad00[0x38];
    float progress;
} ChessBezierCameraState;

typedef struct ChessTableHeader {
    unsigned int setup_function; /* +0x00 */
    unsigned int fight_function; /* +0x04 */
    unsigned int init_function;
    unsigned int restore_function; /* +0x0C */
    char pad10[8];
    int music; /* +0x18 */
} ChessTableHeader;

typedef struct ChessForcePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    char pad10[8];
    int delay;
    float damping;
    int frames;
} ChessForcePdata;

typedef struct ChessSlideMessagePdata {
    MkHdr hdr;
    ScreenObj* object;
    float (*completed)(void);
    int step;
    int target_x;
    unsigned int initial_delay;
} ChessSlideMessagePdata;

typedef struct ChessFadeMessagePdata {
    MkHdr hdr;
    ScreenObj* object; /* +0x08 */
    char pad0C[4];
    int step; /* +0x10 */
    char pad14[4];
    unsigned int initial_delay; /* +0x18 */
} ChessFadeMessagePdata; /* 0x1C-byte allocation */

typedef struct ChessScaleMessagePdata {
    MkHdr hdr;
    ScreenObj* object; /* +0x08 */
    float step; /* +0x0C */
    float start_scale; /* +0x10 */
    float target_scale; /* +0x14 */
    unsigned int initial_delay; /* +0x18 */
    int center_x; /* +0x1C */
    int center_y; /* +0x20 */
    unsigned int texture_width; /* +0x24 */
    int destroy_after_delay; /* +0x28 */
    unsigned int final_delay; /* +0x2C */
    int fade_after_midpoint; /* +0x30 */
} ChessScaleMessagePdata; /* 0x34 */

typedef struct ChessSpellHudNames {
    MkHdr hdr;
    int side;
    int page;
    char pad10[0x1C];
    int selected_name;
    int cursor_slot;
    int state;
} ChessSpellHudNames;

typedef struct ChessDroneState {
    char pad00[8];
    /* The representative piece is the first entry of the side's live roster. */
    union {
        ChessPiece* piece; /* +0x08 */
        ChessPiece* pieces[17]; /* +0x08..+0x4B */
    };
    unsigned int live_piece_count; /* +0x4C */
    unsigned int captured_piece_count; /* +0x50 - roster entries stored at the end */
    char pad54[4];
    MkProc* proc; /* +0x58 */
    char pad5C[0x0C];
    union {
        MkObj* cursor; /* legacy partial view used by complete_action */
        ChessSideController* controller; /* +0x68 */
    };
    char pad6C[0x98];
    int think_delay; /* +0x104 */
    int action_delay; /* +0x108 */
    unsigned int cooldown; /* +0x10C */
    char pad110[0x1C];
    int strategy; /* +0x12C */
    char pad130[0x14];
    unsigned int field_144;
    unsigned int spell; /* +0x148 */
    unsigned int spell_target; /* +0x14C */
    unsigned int state; /* +0x150 */
    unsigned int target_0_x; /* +0x154 */
    unsigned int target_1_x; /* +0x158 */
    unsigned int target_0_y; /* +0x15C */
    unsigned int target_1_y; /* +0x160 */
    unsigned int action_state; /* +0x164 */
    int field_168; /* +0x168 */
    int trap_countdown; /* +0x16C */
    int placement_countdown; /* +0x170 */
    unsigned int desired_x; /* +0x174 */
    unsigned int desired_y; /* +0x178 */
} ChessDroneState;

typedef struct ChessGameTeamDefinition {
    int type;
    int field_04;
    int characters[5];
} ChessGameTeamDefinition; /* 0x1C */

typedef struct ChessGameDefinition {
    int background;
    union {
        ChessGameTeamDefinition teams[2];
        struct {
            int side_0_type;
            int field_08;
            int side_0_characters[5];
            int side_1_type;
            int field_24;
            int side_1_characters[5];
        };
    };
    int enabled;
} ChessGameDefinition; /* 0x40 overlay on g_chess_definition_info */

typedef struct ChessProfileStats {
    char pad00[0x4FC];
    unsigned int local_ai_wins;
    unsigned int local_ai_losses;
    unsigned int local_human_wins;
    unsigned int local_human_losses;
    unsigned int online_wins;
    unsigned int online_losses;
} ChessProfileStats;

extern int g_active_team;
extern int G_BOARD_GAME_BIGSTACK_COUNTER;
extern ChessTeamDefinition g_chess_definition_info[];
extern ChessClassDefinition* g_active_class_definition;
extern ChessPiece* g_active_piece;
extern ChessPiece* g_active_piece_being_defined;
extern ChessModeState* mk_chess_pdata;
extern ChessBoardSave board_game_save_data;
extern ChessBoardGameController g_board_game_controller;
extern AniScript** mkc_animations;
extern float _mkproc_sleep_ticks;
extern MkProc* aproc;
extern int g_loser_life_resolved;
extern int f_fatality_available;
extern int f_fatality_finished;
extern int winner;
extern int screen_width;
extern int screen_height;
extern int game_save_loop_count;
extern ChessProfileStats p1_profile;
extern ChessProfileStats p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
extern const MkFileEntry mkchess_ingame_art_file_table[];
extern SwitchPdata* switch_pdata;
extern CameraObj* camera_obj;
extern ChessBezierCameraState g_bezier_cam;

void snd_req_delay(int sound_id, int delay);
unsigned int fx_by_owner(const char* name, unsigned int owner);
unsigned int fx_next_emitter(unsigned int effect);
void fx_set_param_v3(unsigned int effect, int parameter, float x, float y, float z);
void fx_resume_emit(unsigned int effect);
void fx_reset_emit(unsigned int effect);
void fx_restart_emit(int effect);
int emitter_id_from_handle(unsigned int handle);
static void mk_chess_cursor_go_to_new_track(
    ChessPiece* piece, unsigned int axis, unsigned int direction, unsigned int track);
typedef struct ChessGroundCollisionEntry {
    int bone;
    Vec offset;
    float radius;
} ChessGroundCollisionEntry;
extern ChessGroundCollisionEntry mk_chess_piece_ground_colls[3];
extern const int mk_chess_piece_bones[];

double __fabs(double value);
double sqrt(double value);
static void mk_chess_remove_piece_at_cell_into_deadpool(unsigned char x,
                                                      unsigned char y);
void obj_clear_bone_collapse_flag(MkObj* object, int bone);
void mk_chess_piece_event(ChessPiece* piece, unsigned int event,
                          void* data);
void mk_chess_game_event(unsigned int event, ChessPiece** pieces,
                         unsigned int count, void* data);
void mk_chess_piece_type_to_piece_script(
    unsigned int side, int type, unsigned int occurrence, unsigned int script);
void mk_chess_xfer_to_piece_script(
    ChessPiece* piece, int event);
static ScreenObj* mk_chess_create_portrait_from_library(
    ChessLibraryEntry* library, unsigned int flags);
extern const int piece_model_ss_tbl[10];
ChessPiece* mk_chess_move_piece_from_deadpool_to(unsigned int side,
                                          unsigned int deadpool_index,
                                          unsigned int x, unsigned int y);
float mk_chess_request_piece_fight(ChessPiece* piece, unsigned char x,
                                  unsigned char y, int forced);
void mk_chess_place_special_cell_at(unsigned int x, unsigned int y, int type,
                                    int restored, float px,
                                    float py, float pz, float scale);
static void mk_chess_spell_hud_show_my_spells(ChessHudState* hud);
static int mk_chess_place_spell_hud_cursor_at_open_slot(
    ScreenObj* cursor, int side, unsigned int slot);
void mk_chess_hud_set_piece_portrait(ChessPiece* piece);
void mk_chess_remove_piece_from_team(ChessPiece* piece, int keep_active);
void mk_chess_activate_piece_properties(ChessPiece* piece);
void fx_set(unsigned int effect, int parameter, float value);
int transition_to_anim_script_frame(
    float transition_frames, float frame, AnimPdata* animation,
    AniData* script, unsigned int flags);
void transition_to_anim_script(
    float transition_frames, AnimPdata* animation,
    AniData* script, unsigned int flags);
void fx_pause_emit(unsigned int effect);
int ck_eat_online_switches(void);
int is_plyr_controller_enabled(PlyrInfo* player);
void mk_chess_set_game_mode(int mode);
void mk_chess_timeout_msg(int message, unsigned int side);
void mk_chess_set_viewing_quadrant(CameraObj* camera);
MslSoundHandle snd_req(int sound_id);
int is_load_meter_active(void);
void snd_stop(MslSoundHandle handle);
void shake_camera(int ticks, float strength);
int ck_fatality_available(void);
void end_music(void);
static float p_mk_chess_start_fatality(void);
MkProc* get_player_proc(void* player);
float do_my_fatality(void);
float do_my_2nd_fatality(void);
extern float p_mk_chess_game_restore(void);
extern float p_mk_chess_game_setup(void);
extern float p_mk_chess_game_over(void);
extern float p_mk_chess_continue(void);
static void mk_chess_set_default_chess_game(void);
extern void mk_chess_set_up_passed_in_chess_game(void);
extern void mk_chess_set_default_chess_demo_game(void);
void ck_do_profile_save(void);
void fade_to_black(int ticks, int sleep);
void del_string_obj_by_id(int id);
float bgnd_pebble_fetch_current_info(int field);
void bgnd_pebble_set_current_info(int field, float value);
void bgnd_pebble_set_current_pebble(int group, int pebble);
void mk_chess_camera_init(void);
void update_mkobj(void* object);
void bgnd_make_mkobj_transl(MkObj* object);
void obj_create_sobjs(MkObj* object);
void obj_set_z_offsets(float offset, void* object);
int build_bones_tbl(MkObj* object, const int* tags);
void insert_ground_me_mkobj(MkObj* object);
void pbar_force_pb_setting_with_offset(unsigned int player, float offset);
float p_mk_chess_cam_control(void);
float p_mk_chess_cam_chase_cursor(void);
float p_monitor_chess_input(void);
float x_chess_4(void);
float x_chess_3(void);
float x_chess_2(void);
float x_chess_1(void);
float x_chess_r2(void);
static float x_chess_l1(void);
static void mk_chess_show_spell_hud(unsigned int side);
float x_chess_down(void);
float x_chess_right(void);
static void mk_chess_drone_cursor_movement_to_target(
    ChessDroneState* drone, ChessCursor* cursor_position,
    unsigned int target_x, unsigned int target_y,
    unsigned int moving_delay, unsigned int arrived_delay,
    unsigned int arrived_state);
static int mk_chess_spell_hud_locate_spell_num(
    ChessSpellState* spell, unsigned int side, unsigned int slot,
    unsigned int target, unsigned int spell_number, int select);
static void mk_chess_remove_piece_from_board(ChessPiece* piece);
static void start_gnd_blast(ChessPiece* piece, Vec* scale, int spawn_secondary,
                            float initial_scale);
static void mk_chess_end_of_turn(void);
static int mk_chess_check_input_from_correct_side_no_ai(void);
static int mk_chess_drone_handle_the_big_chill_opening_move(
    ChessDroneState* drone);
static int mk_chess_drone_attempt_to_cast_spell_with_random_targets(
    ChessDroneState* drone, unsigned int target, unsigned int spell);
static int mk_chess_drone_check_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell);
static void mk_chess_drone_setup_spell_hud_pdata_for(
    ChessDroneState* drone, ChessSpellState* hud, unsigned int target,
    unsigned int spell);
static int mk_chess_drone_validate_target(
    ChessDroneState* drone, ChessSpellState* hud, unsigned int x, unsigned int y,
    unsigned int target);
static int mk_chess_drone_attempt_to_cast_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell,
    unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
static int mk_chess_drone_validate_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell,
    unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
static int mk_chess_drone_move_piece_out_of_danger(
    unsigned int side, ChessPiece* piece);
static int mk_chess_drone_move_best_matchup_against_piece(
    unsigned int side, ChessPiece* piece, int flags);
static int mk_chess_drone_help_piece_by_blocking(
    unsigned int side, ChessPiece* threatened, ChessPiece* attacker);
static int mk_chess_fetch_desired_attack_script(
    ChessPiece* piece, ChessAttackGridPosition* attack,
    unsigned int* distance);
static void mk_chess_spell_hud_show_page(ChessHudState* page, int forward);
static float p_mk_chess_piece_proc_entry(void);
static float p_mk_chess_piece_proc(void);
static float p_mk_chess_piece_constrain_to_cell(void);
static float p_mk_chess_scale_display_msg_handler(void);
static void mk_chess_cell_hide(unsigned int x, unsigned int y, int hidden);
static void mk_chess_move_cursor_to_next_piece(
    unsigned int side, unsigned int direction);
static void mk_chess_move_cursor_to_next_diagnal_piece(
    unsigned int side, unsigned int direction);
static void mk_chess_move_cursor_to_next_square_track_line(
    unsigned int side, unsigned int direction);
static void mk_chess_cell_monitor(unsigned int x, unsigned int y);
static void mk_chess_create_piece(
    unsigned char side, int slot, int class_slot, unsigned char x,
    unsigned char y, int character, int active, float health);
static MkObj* mk_chess_create_piece_model_from_library(ChessLibraryEntry* library);
static void mk_chess_drone_select_cell_for_trap(
    unsigned int side, unsigned int* x, unsigned int* y);
unsigned int randu0(unsigned int max);
extern float inverse_game_speed;
extern const int available_chess_chars[26];
extern const int available_chess_bgnds[5];
int is_char_locked(int character, int player);

typedef struct ChessStartingCell {
    unsigned char x;
    unsigned char y;
} ChessStartingCell;

static ChessStartingCell g_starting_cells[16] = {
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0},
    {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1},
};

static unsigned int mk_chess_drone_calc_fight_favorability_rating(
    ChessPiece* attacker, ChessPiece* defender);

/* TODO: [near miss] 88.63%; PPC32 priority checks agree; king-lookup addressing and coloring remain. */
static int mk_chess_drone_kill_ai_casting(ChessDroneState* drone) {
    ChessPiece* target = 0;
    unsigned int index;
    unsigned int side = drone->piece->side;
    ChessSideState* enemies = mk_chess_pdata->sides[!side];
    ChessPiece* king = 0;
    ChessPiece* enemy;
    ChessPieceMoveMap* moves;

    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        if (mk_chess_pdata->sides[side]->pieces[index]->type == 5) {
            king = mk_chess_pdata->sides[side]->pieces[index];
            break;
        }
    }
    for (index = 0; index < enemies->live_piece_count; index++) {
        enemy = enemies->pieces[index];
        if (enemy->type == 1 || enemy->type == 0) {
            moves = enemy->move_map;
            if ((moves->rows[4] >> 3) & 7) {
                target = enemy;
                if (enemy->type == 1) {
                    break;
                }
            } else if ((moves->rows[5] >> 24) & 7) {
                target = enemy;
                if (enemy->type == 1) {
                    break;
                }
            } else if ((moves->rows[king->cell_y] >> (king->cell_x * 3)) & 7) {
                target = enemy;
                break;
            }
        }
    }
    if (target != 0) {
        drone->target_1_x = 255;
        drone->target_1_y = 255;
        drone->target_0_x = target->cell_x;
        drone->target_0_y = target->cell_y;
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 96.18%; shared target publication restored; class-test lowering and coloring remain. */
static int mk_chess_drone_imprison_ai_casting(ChessDroneState* drone) {
    ChessPiece* target = 0;
    unsigned int index;
    unsigned int other;
    unsigned int best_rating;
    unsigned int rating;
    unsigned int enemy_side;
    ChessSideState* enemies = mk_chess_pdata->sides[!drone->piece->side];
    ChessPiece* piece;
    ChessPiece* enemy;

    for (index = 0; index < enemies->live_piece_count; index++) {
        enemy = enemies->pieces[index];
        if ((enemy->type == 3 || enemy->type == 4) &&
            (randu0(100) & 0xFFFF) < 25) {
            target = enemy;
            break;
        }
    }
    if (target == 0) {
        for (index = 0; index < drone->live_piece_count; index++) {
            piece = drone->pieces[index];
            best_rating = 0;
            enemy_side = !piece->side;
            for (other = 0; other < mk_chess_pdata->sides[enemy_side]->live_piece_count; other++) {
                enemy = mk_chess_pdata->sides[enemy_side]->pieces[other];
                if (enemy != 0 &&
                    ((enemy->move_map->rows[piece->cell_y] >> (piece->cell_x * 3)) & 7) == 2) {
                    rating = mk_chess_drone_calc_fight_favorability_rating(enemy, piece);
                    if (best_rating < rating) {
                        best_rating = rating;
                        target = enemy;
                    }
                }
            }
            if (best_rating >= 7 &&
                (drone->pieces[index]->type == 3 ||
                 drone->pieces[index]->type == 4 ||
                 drone->pieces[index]->type == 5)) {
                break;
            }
        }
    }
    if (target != 0) {
        drone->target_1_x = 255;
        drone->target_1_y = 255;
        drone->target_0_x = target->cell_x;
        drone->target_0_y = target->cell_y;
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 95.33%; PPC32 checks agree; stop at nested-loop register coloring. */
static int mk_chess_drone_protect_ai_casting(ChessDroneState* drone) {
    unsigned int index = 0;
    unsigned int other;
    unsigned int best_rating;
    unsigned int rating;
    unsigned int enemy_side;
    ChessPiece* piece;
    ChessPiece* enemy;

    drone->target_1_x = 255;
    drone->target_1_y = 255;
    for (; index < drone->live_piece_count; index++) {
        piece = drone->pieces[index];
        switch (piece->type) {
        case 3:
        case 4:
        case 5:
            best_rating = 0;
            enemy_side = !piece->side;
            for (other = 0; other < mk_chess_pdata->sides[enemy_side]->live_piece_count; other++) {
                enemy = mk_chess_pdata->sides[enemy_side]->pieces[other];
                if (enemy != 0 &&
                    ((enemy->move_map->rows[piece->cell_y] >> (piece->cell_x * 3)) & 7) == 2) {
                    rating = mk_chess_drone_calc_fight_favorability_rating(enemy, piece);
                    if (best_rating < rating) {
                        best_rating = rating;
                    }
                }
            }
            if (best_rating >= 5) {
                drone->target_0_x = drone->pieces[index]->cell_x;
                drone->target_0_y = drone->pieces[index]->cell_y;
                return 1;
            }
            break;
        }
    }
    return 0;
}

static int mk_chess_drone_think(ChessDroneState* drone);
static void mk_chess_drone_complete_action(ChessDroneState* drone);
static void mk_chess_drone_think_in_trap_placement(ChessDroneState* drone);

/* TODO: [near miss] 99.94%; data-value exact; anonymous relocation identity remains. */
static float p_drone_monitor(void) {
    ChessSideController* controller = (ChessSideController*)pdata_of_proc(aproc);
    ChessManagerInfo* manager;
    ChessDroneState* drone;
    int accepts_input;

    if (controller->flags.drone_controlled) {
        manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        if (ck_eat_online_switches() != 0) {
            accepts_input = 0;
        } else {
            switch (manager->input_state) {
            case 0:
            case 1:
            case 9:
            case 10:
                accepts_input = 1;
                break;
            default:
                accepts_input = 0;
                break;
            }
        }
        if (accepts_input) {
            drone = (ChessDroneState*)mk_chess_pdata->sides[controller->side];
            if (controller->flags.bit6) {
                if (mk_chess_drone_think(drone) != 0) {
                    drone->action_delay = 300;
                    drone->controller->flags.bit6 = 0;
                }
            } else if (controller->flags.trap_placement_active) {
                if ((int)drone->action_state == 0) {
                    mk_chess_drone_think_in_trap_placement(drone);
                    drone->action_delay = 300;
                } else {
                    mk_chess_drone_complete_action(drone);
                }
            } else if ((int)drone->action_state != 0) {
                mk_chess_drone_complete_action(drone);
            }
        }
    }
    return 1.0f;
}

static void mk_chess_drone_setup_your_traps(ChessDroneState* drone) {
    if (board_game_save_data.sides[0].input_flags.input_locked) {
        drone->controller->flags.trap_placement_active = 1;
        drone->placement_countdown = (randu0(2) & 0xFFFF) + 1;
        drone->field_168 = 1;
        drone->trap_countdown = randu0((unsigned short)drone->placement_countdown) & 0xFFFF;
    } else {
        drone->controller->flags.trap_placement_active = 1;
        drone->placement_countdown = (randu0(4) & 0xFFFF) + 6;
        drone->field_168 = 1;
        drone->trap_countdown = randu0((unsigned short)drone->placement_countdown) & 0xFFFF;
    }
    mk_chess_drone_select_cell_for_trap(
        drone->piece->side, &drone->desired_x, &drone->desired_y);
}

static unsigned int mk_chess_drone_prepare_high_priority_items(unsigned int side);
static int mk_chess_drone_opening_move_based_on_strategy(ChessDroneState* drone);
static int mk_chess_drone_cast_random_spell(ChessDroneState* drone);
static int mk_chess_drone_handle_strategy(ChessDroneState* drone);
static int mk_chess_drone_random_piece_move(ChessDroneState* drone);

static inline int mk_chess_drone_spellcaster_ordinal(
    ChessDroneState* drone, unsigned int selected) {
    int ordinal = 0;
    unsigned int index;

    for (index = 0; index < drone->live_piece_count; index++) {
        int type = drone->pieces[index]->type;
        if (type == 3) {
            if (selected == 0) {
                return ordinal;
            }
            ordinal++;
        } else if (type == 4) {
            if (selected == 1) {
                return ordinal;
            }
            ordinal++;
        }
    }
    return -1;
}

static int mk_chess_drone_think(ChessDroneState* drone) {
    unsigned int roll;

    if (mk_chess_drone_prepare_high_priority_items(drone->piece->side) == 1) {
        return 1;
    }
    if (mk_chess_drone_opening_move_based_on_strategy(drone) != 0) {
        return 1;
    }
    if (drone->captured_piece_count != 0) {
        roll = randu0(100) & 0xFFFF;
        if (roll < 20 || (board_game_save_data.sides[0].input_flags.input_locked && roll < 25)) {
            if (mk_chess_drone_attempt_to_cast_spell_with_random_targets(drone,
                    mk_chess_drone_spellcaster_ordinal(drone, 0), 2) != 0) {
                return 1;
            }
        }
    }
    if ((randu0(100) & 0xFFFF) < 10 && mk_chess_drone_cast_random_spell(drone) != 0) {
        return 1;
    }
    if ((unsigned int)mk_chess_pdata->manager.clock > 65) {
        mk_chess_pdata->sides[drone->piece->side]->strategy = 1;
    }
    if (mk_chess_drone_handle_strategy(drone) != 0) {
        return 1;
    }
    return mk_chess_drone_random_piece_move(drone) != 0;
}

static void mk_chess_drone_think_in_trap_placement(ChessDroneState* drone) {
    if (drone->trap_countdown == 0) {
        drone->action_state = 13;
        drone->cooldown = 5;
        drone->target_0_x = drone->desired_x;
        drone->target_0_y = drone->desired_y;
        drone->trap_countdown = -1;
        return;
    }
    if (drone->placement_countdown == 0) {
        drone->action_state = 17;
        drone->cooldown = 5;
        drone->placement_countdown = -1;
        return;
    }
    if (drone->placement_countdown >= -(int)(randu0(5) & 0xFFFF)) {
        drone->action_state = 15;
        drone->cooldown = 5;
        mk_chess_drone_select_cell_for_trap(
            drone->piece->side, &drone->target_0_x, &drone->target_0_y);
        drone->trap_countdown--;
        drone->placement_countdown--;
    }
}


/* TODO: [near miss] 99.94%; data-value exact; anonymous relocation identity remains. */
static void mk_chess_drone_complete_action(ChessDroneState* drone) {
    ChessManagerInfo* manager;
    ChessSpellState* spell;
    unsigned int side;

    manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;

    if (drone->cooldown != 0) {
        if (--drone->cooldown != 0) {
            return;
        }
    }

    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        return;
    }

    if (--drone->action_delay == 0) {
        if (drone->cursor->flags_08_bits.gravity_enabled) {
            drone->action_state = 0;
            return;
        }

        drone->cursor->flags_08_bits.airborne = 1;
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_4);
        return;
    }

    switch (drone->action_state) {
    case 13:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->manager.directional_state->cursors[side],
            drone->target_0_x, drone->target_0_y, 3, 7, 14);
        return;
    case 14:
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_1);
        return;
    case 15:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->manager.directional_state->cursors[side],
            drone->target_0_x, drone->target_0_y, 3, 7, 16);
        return;
    case 16:
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_4);
        return;
    case 17:
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_3);
        return;
    case 1:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->cursors[side],
            drone->target_0_x, drone->target_0_y, 7, 10, 2);
        return;
    case 2:
        xfer_proc(drone->proc, x_chess_3);
        drone->action_state = 3;
        drone->cooldown = 60;
        return;
    case 3:
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->cursor,
            drone->target_1_x, drone->target_1_y, 4, 10, 4);
        return;
    case 4:
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_3);
        return;
    case 5:
        xfer_proc(drone->proc, x_chess_l1);
        drone->action_state = 6;
        drone->cooldown = 80;
        drone->think_delay = 800;
        return;
    case 6:
        spell = manager->spell;
        if (mk_chess_spell_hud_locate_spell_num(
                spell, drone->piece->side, spell->field_30,
                drone->spell_target, drone->spell, 1) != 0) {
            drone->action_state = 7;
            drone->cooldown = 30;
        } else {
            xfer_proc(drone->proc, x_chess_down);
            drone->cooldown = 10;
        }
        return;
    case 7:
        xfer_proc(drone->proc, x_chess_3);
        if (drone->state < 16) {
            drone->action_state = 11;
            drone->cooldown = 30;
        } else {
            drone->action_state = 8;
            drone->cooldown = 80;
        }
        return;
    case 11:
        if (drone->state == 0) {
            drone->action_state = 12;
            drone->cooldown = 25;
        } else {
            xfer_proc(drone->proc, x_chess_right);
            drone->cooldown = 25;
            drone->state--;
        }
        return;
    case 8:
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->cursors[2],
            drone->target_0_x, drone->target_0_y, 5, 10, 9);
        return;
    case 12:
        xfer_proc(drone->proc, x_chess_3);
        drone->action_state = 10;
        drone->cooldown = 120;
        drone->think_delay = 800;
        return;
    case 9:
        xfer_proc(drone->proc, x_chess_3);
        if (drone->target_1_x == 0xFF) {
            drone->action_state = 0;
            drone->think_delay = 800;
        } else {
            drone->action_state = 10;
            drone->cooldown = 40;
        }
        return;
    case 10:
        mk_chess_drone_cursor_movement_to_target(
            drone, &mk_chess_pdata->cursors[2],
            drone->target_1_x, drone->target_1_y, 5, 10, 4);
        return;
    case 0:
        return;
    }
}

/* TODO: [breakthrough needed] 82.43284%; no-argument demo call corrected; global base addressing and initialization lowering remain. */
void p_mk_chess(void) {
    ChessProcVtable* vtable;

    g_loser_life_resolved = 0;
    g_active_piece = 0;
    g_active_team = 0;
    g_active_piece_being_defined = 0;
    mk_chess_pdata = 0;
    g_active_class_definition = 0;
    memset(&g_board_game_controller, 0, sizeof(g_board_game_controller));
    memset(&board_game_save_data, 0, sizeof(board_game_save_data));

    g_game_info.plyr0.field_44 = 0;
    g_game_info.plyr1.field_44 = 0;
    board_game_save_data.sides[0].input_flags.input_locked = 0;
    board_game_save_data.sides[0].restore_pending = 0;
    board_game_save_data.field_16B0 = 0;
    board_game_save_data.field_16B4 = 0;
    board_game_save_data.profile_stat_ceiling = 100000;
    board_game_save_data.field_16BC = 0;
    board_game_save_data.last_update_tick = exec_tick_ctr;
    board_game_save_data.sides[0].forced_fight_count = 0;
    board_game_save_data.sides[0].fight_stat_70C = 0;
    board_game_save_data.sides[0].fight_stat_710 = 0;
    board_game_save_data.sides[0].fight_stat_714 = 0;
    board_game_save_data.sides[1].forced_fight_count = 0;
    board_game_save_data.sides[1].fight_stat_70C = 0;
    board_game_save_data.sides[1].fight_stat_710 = 0;
    board_game_save_data.sides[1].fight_stat_714 = 0;

    if ((g_game_info.field_04 & 0x20) != 0) {
        board_game_save_data.sides[0].input_flags.input_locked = 1;
        mk_chess_set_default_chess_demo_game();
    }

    vtable = (ChessProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_mk_chess_continue, 0.0f);
}

/* TODO: [near miss] 97.50%; data-value exact; anonymous relocation identity remains. */
float mk_chess_fight_cam_target_reached(void) {
    return 0.0f;
}

/* TODO: [near miss] 97.50%; data-value exact; anonymous relocation identity remains. */
float mk_chess_piece_proc_force_dead(void) {
    return -1.0f;
}


extern const MkFileEntry mk_chess_file_table[];
static void mk_chess_load_common_data(void);
void mk_chess_load_piece_art(void);
static void mk_chess_restore_teams(void);
void mk_chess_restore_board(void);
static void mk_chess_game_manager_init(int restoring);
static void mk_chess_alive_pieces_initialize(void);
static void mk_chess_calc_knowledge_base(ChessPiece* first, ChessPiece* second, unsigned int result);
void mk_chess_request_defender_won(ChessPiece* defender, unsigned char x, unsigned char y);
static void mk_chess_request_attacker_won(ChessPiece* attacker, unsigned char x, unsigned char y);
float p_mk_chess_blink_string(void);
float p_mk_chess_loop(void);

extern MkFileInfo sec_mkchess_ingame_art;
extern MkFileInfo sec_mkchess_ingame_anims;
extern int mkc_in_fight_anims[89];
static void mk_chess_team_init(unsigned int side);
static void mk_chess_load_team_art(unsigned int side);
void mk_chess_camera_init_for_place_traps(void);
float p_mk_chess_place_traps(void);

/* TODO: [breakthrough needed] 83.67%; setup flow recovered; board initialization, global bases and message scheduling remain. */
float p_mk_chess_game_setup(void)
{
    int x, y;
    int pause_slot;
    MkHdr* allocation;
    MkProc* process;
    StringObj* text;
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
            cell->piece = 0;
            cell->square_type = 0;
            cell->flags = 0;
            cell->object = 0;
            cell->position.x = ((float)x - 10.0f) + (1.0f + (float)x);
            cell->position.y = 0.0f;
            cell->position.z = ((float)y - 10.0f) + (1.0f + (float)y);
        }
    }
    setup_sound_banks(6);
    wait_for_sound_banks_to_load();
    mk_chess_pdata->tracked_sounds = 0;
    start_sound_tracking_process(&mk_chess_pdata->tracked_sounds);
    load_background(((ChessGameDefinition*)g_chess_definition_info)->background);
    RwCameraSetNearClipPlane(Camera, 5.0f);
    RwCameraSetFarClipPlane(Camera, 1500.0f);
    load_ssf((MkFileEntry*)mkchess_ingame_art_file_table);
    load_art_section_language(0xE003E, &sec_mkchess_ingame_art);
    unload_section_slot(0xE003F);
    add_anim_section_async(0xE003F, &sec_mkchess_ingame_anims, mkc_in_fight_anims, 0, 0);
    wait_for_slot_load(0xE003F);
    load_ssf((MkFileEntry*)mk_chess_file_table);
    mk_chess_load_common_data();
    mk_chess_load_piece_art();
    mk_chess_team_init(0);
    g_active_team = 0;
    cmdscript_setup_execution(g_board_game_controller.command_script, 1);
    cmdscript_execute(g_board_game_controller.command_script);
    mk_chess_load_team_art(0);
    mk_chess_team_init(1);
    g_active_team = 1;
    cmdscript_setup_execution(g_board_game_controller.command_script, 1);
    cmdscript_execute(g_board_game_controller.command_script);
    mk_chess_load_team_art(1);
    if (g_game_info.mode_table != 0 && ((ChessTableHeader*)g_game_info.mode_table)->setup_function != 0) {
        cmdscript_setup_execution(g_game_info.cmdscript, ((ChessTableHeader*)g_game_info.mode_table)->setup_function);
        cmdscript_execute(g_game_info.cmdscript);
    }
    if (get_game_state() == 3) mk_chess_camera_init();
    else mk_chess_camera_init_for_place_traps();
    mk_chess_game_manager_init(0);
    mk_chess_alive_pieces_initialize();
    mk_chess_pdata->camera_sound.field_00 = snd_req(((ChessTableHeader*)g_game_info.mode_table)->music);
    g_game_info.flags |= 2;
    turn_camera_off();
    while (g_game_info.flags & 0x80) {
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    if (get_game_state() != 3) mk_chess_set_game_mode(10);
    pause_slot = get_pause_menu_ssh();
    unload_section_slot(pause_slot);
    if (get_game_state() == 3) {
        if (board_game_save_data.input_flags.input_locked) {
            text = string_center_xy(0x2010, 0, get_string(1), screen_width / 2, 65, 29);
            process = _create_mkproc_generic_tinystack(0xC026, 31, p_mk_chess_blink_string,
                sizeof(ChessBlinkStringPdata), &allocation);
            if (process != 0) {
                ChessBlinkStringPdata* blink = (ChessBlinkStringPdata*)allocation;
                blink->text = text;
                blink->text_instance = text->instance;
                mk_insert((MkHdr*)text, &process->pdata_list_b);
                blink->timer = 20;
                blink->interval = 20;
            }
        }
    } else {
        preload_screen_data("pause_menu/pause_menu", pause_slot);
    }
    turn_camera_on();
    fade_from_black(10, 1);
    turn_controllers_on();
    if (get_game_state() == 3) {
        mk_chess_pdata->sides[0]->desired_x = 4;
        mk_chess_pdata->sides[0]->desired_y = 4;
        mk_chess_pdata->sides[1]->desired_x = 5;
        mk_chess_pdata->sides[1]->desired_y = 5;
        mk_chess_place_special_cell_at(4, 4, 2, 0, 0.0f, 0.0f, 0.0f, 0.0f);
        mk_chess_place_special_cell_at(5, 5, 2, 0, 1.0f, 0.0f, 0.0f, 0.0f);
        mk_chess_set_game_mode(0);
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_loop, 0.0f);
        return 0.0f;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_place_traps, 0.0f);
    return 0.0f;
}

static inline void mk_chess_announce_restored_king_win(void)
{
    unsigned short roll;
    snd_req_delay(12, 15);
    if ((unsigned short)randu0(100) < 100) {
        roll = randu0(100);
        if (roll < 20) snd_req_delay(1, 260);
        else if (roll < 40) snd_req_delay(2, 260);
        else if (roll < 60) snd_req_delay(3, 260);
        else snd_req_delay(5, 260);
    }
}

/* TODO: [breakthrough needed] 89.33%; restore flow recovered; board initialization and announcement scheduling remain. */
float p_mk_chess_game_restore(void)
{
    int x, y;
    ChessPiece* attacker;
    ChessPiece* defender;
    ChessPiece* surviving_defender;
    unsigned short roll;
    MkHdr* allocation;
    MkProc* process;
    StringObj* text;
    set_game_switch_maps();
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
            cell->piece = 0;
            cell->square_type = 0;
            cell->flags = 0;
            cell->object = 0;
            cell->position.x = ((float)x - 10.0f) + (1.0f + (float)x);
            cell->position.y = 0.0f;
            cell->position.z = ((float)y - 10.0f) + (1.0f + (float)y);
        }
    }
    setup_sound_banks(6);
    wait_for_sound_banks_to_load();
    mk_chess_pdata->tracked_sounds = 0;
    start_sound_tracking_process(&mk_chess_pdata->tracked_sounds);
    load_background(((ChessGameDefinition*)g_chess_definition_info)->background);
    RwCameraSetNearClipPlane(Camera, 5.0f);
    RwCameraSetFarClipPlane(Camera, 1500.0f);
    load_ssf((MkFileEntry*)mk_chess_file_table);
    mk_chess_load_common_data();
    mk_chess_load_piece_art();
    mk_chess_restore_teams();
    if (g_game_info.mode_table != 0 && ((ChessTableHeader*)g_game_info.mode_table)->restore_function != 0) {
        cmdscript_setup_execution(g_game_info.cmdscript, ((ChessTableHeader*)g_game_info.mode_table)->restore_function);
        cmdscript_execute(g_game_info.cmdscript);
    }
    mk_chess_camera_init();
    mk_chess_game_manager_init(1);
    mk_chess_alive_pieces_initialize();
    mk_chess_restore_board();
    mk_chess_pdata->camera_sound.field_00 = snd_req(((ChessTableHeader*)g_game_info.mode_table)->music);
    board_game_save_data.restore_pending = 0;
    g_game_info.flags |= 2;
    turn_camera_off();
    while (g_game_info.flags & 0x80) {
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    attacker = mk_chess_pdata->board[board_game_save_data.origin_x].cells[board_game_save_data.origin_y].piece;
    defender = mk_chess_pdata->board[board_game_save_data.destination_x].cells[board_game_save_data.destination_y].piece;
    if (attacker->side == 0) mk_chess_calc_knowledge_base(attacker, defender, winner);
    else mk_chess_calc_knowledge_base(defender, attacker, winner);
    if ((winner == 1 && attacker->side == 0) || (winner == 2 && attacker->side == 1)) {
        if (attacker->health < 0.03f) attacker->health = 0.03f;
        board_game_save_data.sides[attacker->side].fight_stat_714++;
        if (mk_chess_pdata->board[board_game_save_data.destination_x].cells[board_game_save_data.destination_y].square_type == 1) {
            if ((unsigned short)randu0(100) < 100) {
                roll = randu0(100);
                if (roll < 20) snd_req_delay(1, 15);
                else if (roll < 40) snd_req_delay(2, 15);
                else if (roll < 80) snd_req_delay(17, 15);
                else snd_req_delay(18, 15);
            }
            mk_chess_pdata->manager.flags |= 0x20;
        } else if (attacker->type == 5) {
            mk_chess_announce_restored_king_win();
        }
        mk_chess_request_attacker_won(attacker, board_game_save_data.destination_x, board_game_save_data.destination_y);
    } else {
        surviving_defender = mk_chess_pdata->board[board_game_save_data.destination_x].cells[board_game_save_data.destination_y].piece;
        if (surviving_defender->health < 0.03f) surviving_defender->health = 0.03f;
        board_game_save_data.sides[defender->side].fight_stat_714++;
        if (defender->type == 5) mk_chess_announce_restored_king_win();
        mk_chess_request_defender_won(surviving_defender, board_game_save_data.origin_x, board_game_save_data.origin_y);
    }
    if (board_game_save_data.input_flags.input_locked) {
        text = string_center_xy(0x2010, 0, get_string(1), screen_width / 2, 65, 29);
        process = _create_mkproc_generic_tinystack(0xC026, 31, p_mk_chess_blink_string,
            sizeof(ChessBlinkStringPdata), &allocation);
        if (process != 0) {
            ChessBlinkStringPdata* blink = (ChessBlinkStringPdata*)allocation;
            blink->text = text;
            blink->text_instance = text->instance;
            mk_insert((MkHdr*)text, &process->pdata_list_b);
            blink->timer = 20;
            blink->interval = 20;
        }
    }
    unload_section_slot(0x11005C);
    turn_camera_on();
    fade_from_black(10, 1);
    turn_controllers_on();
    mk_chess_end_of_turn();
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_loop, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 79.94%; byte-coordinate ABI restored; typed board addressing and register scheduling remain. */
void mk_chess_request_defender_won(
    ChessPiece* defender,
    unsigned char cell_x,
    unsigned char cell_y) {
    ChessGameEventData event;

    mk_chess_set_game_mode(2);
    event.piece = mk_chess_pdata->board[cell_x].cells[cell_y].piece;
    event.other_piece = defender;
    mk_chess_game_event(3, event.pieces, 2, 0);
    mk_chess_remove_piece_at_cell_into_deadpool(cell_x, cell_y);
}


/* TODO: [near miss] 90.19%; byte-coordinate ABI restored; typed board address reuse and register scheduling remain. */
static void mk_chess_request_attacker_won(
    ChessPiece* attacker, unsigned char x, unsigned char y) {
    ChessGameEventData event;
    ChessCell* cell;
    ChessCell* previous;

    mk_chess_set_game_mode(2);
    event.piece = mk_chess_pdata->board[x].cells[y].piece;
    event.other_piece = attacker;
    mk_chess_game_event(3, event.pieces, 2, 0);
    mk_chess_remove_piece_at_cell_into_deadpool(x, y);
    cell = &mk_chess_pdata->board[x].cells[y];
    previous = &mk_chess_pdata->board[attacker->cell_x].cells[attacker->cell_y];
    if (previous->piece == attacker) {
        previous->piece = 0;
    }
    cell->piece = attacker;
    attacker->cell_x = x;
    attacker->cell_y = y;
    attacker->object->hide_flag_bits.pin_animation = 0;
    attacker->object->pos.value.x =
        cell->position.x + attacker->runtime.fields.cell_offset.x;
    attacker->object->pos.value.z =
        cell->position.z + attacker->runtime.fields.cell_offset.z;
    update_obj_pos(attacker->object);
}

static inline ChessPiece* mk_chess_find_piece_of_type(
    unsigned int side, int type, unsigned int occurrence) {
    unsigned int index;

    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        if (mk_chess_pdata->sides[side]->pieces[index]->type == type) {
            if (occurrence == 0) {
                return mk_chess_pdata->sides[side]->pieces[index];
            }
            occurrence--;
        }
    }
    return 0;
}




static int move_cursor_based_on_quadrant(unsigned int side, int* x, int* y,
    unsigned int direction, int minimum, int maximum, int options);

/* TODO: [near miss] 82.58%; recovered priority and destination scans; local-array addressing and helper joins remain. */
static int mk_chess_drone_rescue_ai_casting(ChessDroneState* drone) {
    int available[6] = {0, 0, 0, 0, 0, 0};
    int candidate_x[8];
    int candidate_y[8];
    int selected_class;
    int slot = 0;
    int index;
    unsigned short count = 0;
    unsigned short direction;
    int x;
    int y;
    unsigned char king_x;
    unsigned char king_y;
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);

    for (index = (unsigned char)(17 - drone->captured_piece_count); index < 17; index++) {
        available[drone->pieces[index]->type] = 1;
    }
    if (available[2] != 0) {
        selected_class = 2;
    } else if (available[1] != 0) {
        selected_class = 1;
    } else {
        return 0;
    }
    for (index = 0; index < selected_class; index++) {
        if (available[index] != 0) {
            slot++;
        }
    }
    drone->state = slot;
    king_x = king->cell_x;
    king_y = king->cell_y;
    for (direction = 0; direction < 8; direction++) {
        candidate_x[count] = king_x;
        candidate_y[count] = king_y;
        if (move_cursor_based_on_quadrant(king->side,
                &candidate_x[count], &candidate_y[count], direction, 0, 10, 0) != 0 &&
            mk_chess_pdata->board[candidate_x[count]].cells[candidate_y[count]].piece == 0 &&
            ((king->side == 0 && candidate_y[count] < 2) ||
             (king->side == 1 && candidate_y[count] > 7))) {
            count++;
        }
    }
    if (count != 0) {
        index = randu0(count) & 0xFFFF;
        drone->target_1_x = candidate_x[index];
        drone->target_1_y = candidate_y[index];
        return 1;
    }
    switch (drone->piece->side) {
    case 0:
        for (x = 0; x < 10; x++) {
            for (y = 0; y < 2; y++) {
                if (mk_chess_pdata->board[x].cells[y].piece == 0) {
                    drone->target_1_x = x;
                    drone->target_1_y = y;
                    return 1;
                }
            }
        }
        break;
    case 1:
        for (x = 0; x < 10; x++) {
            for (y = 8; y < 10; y++) {
                if (mk_chess_pdata->board[x].cells[y].piece == 0) {
                    drone->target_1_x = x;
                    drone->target_1_y = y;
                    return 1;
                }
            }
        }
        break;
    }
    return 0;
}

static int mk_chess_find_my_smart_piece(ChessDroneState* drone,
    ChessPiece** selected, float minimum_health);

/* TODO: [near miss] 75.81%; repeated neighbor search recovered; local-array addressing and helper joins remain. */
static int mk_chess_drone_teleport_ai_casting(ChessDroneState* drone) {
    int attempts = 8;
    ChessPiece* king = mk_chess_find_piece_of_type(!drone->piece->side, 5, 0);
    ChessPiece* piece;
    int candidate_x[8];
    int candidate_y[8];
    unsigned short count;
    unsigned short direction;
    unsigned char king_x;
    unsigned char king_y;
    unsigned int index;
    int x;
    int y;

    while (--attempts != 0) {
        count = 0;
        king_x = king->cell_x;
        king_y = king->cell_y;
        for (direction = 0; direction < 8; direction++) {
            candidate_x[count] = king_x;
            candidate_y[count] = king_y;
            if (move_cursor_based_on_quadrant(king->side,
                    &candidate_x[count], &candidate_y[count], direction, 0, 10, 0) != 0 &&
                mk_chess_pdata->board[candidate_x[count]].cells[candidate_y[count]].piece == 0) {
                count++;
            }
        }
        if (count != 0) {
            index = randu0(count) & 0xFFFF;
            x = candidate_x[index];
            y = candidate_y[index];
            if (mk_chess_find_my_smart_piece(drone, &piece, 0.75f) != 0) {
                drone->target_0_x = piece->cell_x;
                drone->target_0_y = piece->cell_y;
                drone->target_1_x = x;
                drone->target_1_y = y;
                return 1;
            }
        }
    }
    return 0;
}

static inline int mk_chess_pieces_are_neighbors(ChessPiece* piece, ChessPiece* other) {
    int dx;
    int dy;
    unsigned int distance;
    unsigned int distance_x;

    if (piece == 0 || other == 0) {
        return 0;
    }
    dx = (int)other->cell_x - piece->cell_x;
    dy = (int)other->cell_y - piece->cell_y;
    distance = dy < 0 ? -dy : dy;
    distance_x = dx < 0 ? -dx : dx;
    if (distance < distance_x) {
        distance = distance_x;
    }
    return distance == 1;
}

static inline unsigned int mk_chess_piece_cell_distance(ChessPiece* piece, ChessPiece* other) {
    int dx = (int)other->cell_x - piece->cell_x;
    int dy = (int)other->cell_y - piece->cell_y;
    unsigned int distance_x = dx < 0 ? -dx : dx;
    unsigned int distance_y = dy < 0 ? -dy : dy;
    return distance_x > distance_y ? distance_x : distance_y;
}

/* TODO: [near miss] 59.62%; priorities and duplicate weighting recovered; distance helper and roster addressing differ. */
static int mk_chess_drone_exchange_ai_casting(ChessDroneState* drone) {
    /* The limit is checked before a piece can contribute up to three entries. */
    ChessPiece* candidates[12];
    unsigned short count = 0;
    unsigned int side = drone->piece->side;
    ChessPiece* king = mk_chess_find_piece_of_type(side, 5, 0);
    ChessSideState* enemies = mk_chess_pdata->sides[!side];
    ChessPiece* enemy_king = mk_chess_find_piece_of_type(!side, 5, 0);
    ChessPiece* enemy_class_3 = mk_chess_find_piece_of_type(!side, 3, 0);
    ChessPiece* enemy_class_4 = mk_chess_find_piece_of_type(!side, 4, 0);
    ChessPiece* selected = 0;
    ChessPiece* piece;
    unsigned int index;

    for (index = 0; index < drone->live_piece_count; index++) {
        piece = drone->pieces[index];
        if (mk_chess_pieces_are_neighbors(king, piece) == 0) {
            piece = drone->pieces[index];
            if (piece->type == 1) {
                if (mk_chess_piece_cell_distance(piece, enemy_king) > 1) {
                    selected = drone->pieces[index];
                    break;
                }
            } else if ((piece->type == 2 || piece->type == 0) &&
                piece->health > 0.35f &&
                mk_chess_piece_cell_distance(piece, enemy_king) > 1) {
                if (selected == 0 || selected->health < drone->pieces[index]->health) {
                    selected = drone->pieces[index];
                }
            }
        }
    }
    if (selected == 0) {
        return 0;
    }
    drone->target_0_x = selected->cell_x;
    drone->target_0_y = selected->cell_y;
    for (index = 0; index < enemies->live_piece_count && count < 10; index++) {
        piece = enemies->pieces[index];
        if (mk_chess_pieces_are_neighbors(enemy_king, piece) &&
            enemies->pieces[index]->type == 0) {
            candidates[count++] = enemies->pieces[index];
        }
        piece = enemies->pieces[index];
        if (mk_chess_pieces_are_neighbors(enemy_class_3, piece) &&
            enemies->pieces[index]->type == 0) {
            candidates[count++] = enemies->pieces[index];
        }
        piece = enemies->pieces[index];
        if (mk_chess_pieces_are_neighbors(enemy_class_4, piece) &&
            enemies->pieces[index]->type == 0) {
            candidates[count++] = enemies->pieces[index];
        }
    }
    if (count == 0) {
        return 0;
    }
    piece = candidates[randu0(count) & 0xFFFF];
    drone->target_1_x = piece->cell_x;
    drone->target_1_y = piece->cell_y;
    return 1;
}

/* TODO: [near miss] 81.86%; selection rules recovered; equivalent neighbor helper and loop coloring remain. */
static int mk_chess_drone_sacrifice_ai_casting(ChessDroneState* drone) {
    ChessPiece* donor = 0;
    ChessPiece* recipient = 0;
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
    ChessPiece* piece;
    unsigned int index;

    for (index = 0; index < drone->live_piece_count; index++) {
        piece = drone->pieces[index];
        if (mk_chess_pieces_are_neighbors(king, piece) == 0) {
            piece = drone->pieces[index];
            if (piece->type == 0) {
                if (donor == 0) {
                    donor = piece;
                } else if (donor->health > piece->health) {
                    donor = piece;
                } else if (donor->health == piece->health &&
                    (randu0(100) & 0xFFFF) < 20) {
                    donor = drone->pieces[index];
                }
            }
        }
        piece = drone->pieces[index];
        if ((piece->type == 2 || piece->type == 5) &&
            (recipient == 0 || recipient->health > piece->health) &&
            piece->health < 0.75f) {
            recipient = piece;
        }
    }
    if (donor == 0 || recipient == 0) {
        return 0;
    }
    drone->target_0_x = donor->cell_x;
    drone->target_0_y = donor->cell_y;
    drone->target_1_x = recipient->cell_x;
    drone->target_1_y = recipient->cell_y;
    return 1;
}

/* TODO: [near miss] 68.25%; filters recovered; equivalent distance helper and candidate-array lowering remain. */
static int mk_chess_find_my_smart_piece(ChessDroneState* drone,
    ChessPiece** selected, float minimum_health) {
    ChessPiece* candidates[17];
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
    unsigned short count = 0;
    unsigned int index;
    ChessPiece* piece;

    for (index = 0; index < drone->live_piece_count; index++) {
        candidates[count] = drone->pieces[index];
        piece = candidates[count];
        if ((piece->type == 1 || piece->type == 0 || piece->type == 2) &&
            (piece->health >= minimum_health || piece->type == 1) &&
            mk_chess_pieces_are_neighbors(piece, king) == 0) {
            count++;
        }
    }
    if (count == 0) {
        return 0;
    }
    *selected = candidates[randu0(count) & 0xFFFF];
    return 1;
}

/* TODO: [near miss] 81.66%; equivalent Chebyshev-distance helper; king lookup and branch lowering remain. */
static int mk_chess_drone_heal_ai_casting(ChessDroneState* drone) {
    int attempts = 8;
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
    ChessPiece* piece;
    unsigned int index;

    drone->target_1_x = 255;
    drone->target_1_y = 255;
    while (--attempts != 0) {
        index = randu0((unsigned short)drone->live_piece_count) & 0xFFFF;
        drone->target_0_x = drone->pieces[index]->cell_x;
        drone->target_0_y = drone->pieces[index]->cell_y;
        piece = drone->pieces[index];
        if (piece->type != 1 &&
            mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type == 0) {
            if (drone->strategy == 3) {
                if (mk_chess_pieces_are_neighbors(piece, king) && piece->health < 0.4f) {
                    return 1;
                }
            } else if (piece->health < 0.6f &&
                (piece->type != 0 || (unsigned int)mk_chess_pdata->manager.clock > 15)) {
                return 1;
            }
        }
    }
    return 0;
}

void mk_chess_register_name_and_portrait_in_team(
    unsigned int side_index, unsigned int portrait_index, ChessLibraryEntry* library);






/* TODO: [near miss] 99.09091%; data-value exact; anonymous relocation identity remains. */
int mk_chess_fake_demo_cam(float* camera_speed) {
    if (camera_obj->pos.z > 20.3f) {
        return 0;
    }
    *camera_speed = 0.1f;
    return 1;
}

int mk_chess_should_i_fall_down(void) {
    return 0;
}

static int mk_chess_drone_heal_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_teleport_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_rescue_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_imprison_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_exchange_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_sacrifice_ai_casting(ChessDroneState* drone);
static int mk_chess_drone_cast_random_spell(ChessDroneState* drone) {
    unsigned int target;

    target = randu0(2) & 0xFFFF;
    return mk_chess_drone_attempt_to_cast_spell_with_random_targets(
        drone, target, randu0(4) & 0xFFFF);
}

static inline int mk_chess_drone_select_and_validate_random_targets(
    ChessDroneState* drone, ChessSpellState* hud) {
    int (*selectors[2][4])(ChessDroneState*) = {
        {mk_chess_drone_heal_ai_casting, mk_chess_drone_teleport_ai_casting,
         mk_chess_drone_rescue_ai_casting, mk_chess_drone_protect_ai_casting},
        {mk_chess_drone_kill_ai_casting, mk_chess_drone_imprison_ai_casting,
         mk_chess_drone_exchange_ai_casting, mk_chess_drone_sacrifice_ai_casting}
    };

    if (hud->caster->type == 3) {
        if (selectors[0][hud->spell_number](drone) == 0) {
            return 0;
        }
    } else if (selectors[1][hud->spell_number](drone) == 0) {
        return 0;
    }
    if (mk_chess_drone_validate_target(drone, hud,
            drone->target_0_x, drone->target_0_y, 0) == 0) {
        return 0;
    }
    if (mk_chess_drone_validate_target(drone, hud,
            drone->target_1_x, drone->target_1_y, 1) == 1) {
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 98.86%; retail indexed coordinates preserved; local scheduling and lifetimes remain. */
static int mk_chess_drone_teleport_weak_side_piece_to(
    ChessDroneState* drone, unsigned int x, unsigned int y) {
    int found = 0;
    ChessPiece* selected = 0;
    if (mk_chess_drone_check_spell(drone, 0, 1) == 1) {
        unsigned int index;
        for (index = 0; index < drone->live_piece_count; index++) {
            ChessPiece* piece = drone->pieces[index];
            if ((piece->type == 0 || piece->type == 1) &&
                (piece->cell_x <= 1 || piece->cell_x >= 9)) {
                /* Retail indexes the fetched piece pointer again for this test. */
                if (mk_chess_pdata->board[piece[index].cell_x].cells[piece[index].cell_y].square_type != 1 &&
                    (found == 0 || (unsigned short)randu0(100) < 70)) {
                    found = 1;
                    selected = piece;
                }
            }
        }
        if (found != 0 && selected != 0 &&
            mk_chess_drone_attempt_to_cast_spell(drone, 0, 1,
                selected->cell_x, selected->cell_y, x, y) != 0) {
            return 1;
        }
    }
    return 0;
}

/* TODO: [near miss] 99.73%; data-value exact after callback recovery; anonymous table relocation identity remains. */
static int mk_chess_drone_attempt_to_cast_spell_with_random_targets(
    ChessDroneState* drone, unsigned int target, unsigned int spell) {
    ChessSpellState hud;

    if (mk_chess_drone_check_spell(drone, target, spell) == 1) {
        drone->state = 0x10;
        drone->spell = spell;
        drone->spell_target = target;
        mk_chess_drone_setup_spell_hud_pdata_for(drone, &hud, target, spell);
        if (mk_chess_drone_select_and_validate_random_targets(drone, &hud) == 0) {
            return 0;
        }
        drone->action_state = 5;
        drone->cooldown = 0x50;
        return 1;
    }
    return 0;
}

/* TODO: [breakthrough needed] 92.94%; resolve retail beq 0x784 and its surrounding ownership/CFG before further tuning. */
static int mk_chess_drone_opening_move_based_on_strategy(
    ChessDroneState* drone) {
    if (drone->strategy == 5 &&
        mk_chess_drone_handle_the_big_chill_opening_move(drone) != 0) {
        return 1;
    }
    return 0;
}


/* TODO: [breakthrough needed] 73.33149%; scale conversion, owner reloads and fade-loop structure remain unresolved. */
static float p_mk_chess_scale_display_msg_handler(void) {
    ChessScaleMessagePdata* pdata;
    ScreenObj* object;
    float midpoint;
    int index;

    pdata = (ChessScaleMessagePdata*)apdata;
    _mkproc_sleep_ticks = (float)pdata->initial_delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    object = pdata->object;
    unhide_screen_obj(object);
    midpoint =
        pdata->start_scale +
        (pdata->target_scale - pdata->start_scale) * 0.5f;

    while (object->scale_x < pdata->target_scale) {
        object->scale_x += pdata->step;
        object->scale_y += pdata->step;
        object->x =
            -(int)(((float)(pdata->texture_width >> 1) *
                     object->scale_x) -
                   (float)pdata->center_x);
        object->y =
            -(int)(((float)(object->pfx2d->tex_h / 2) *
                     object->scale_y) -
                   (float)pdata->center_y);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();

        if (pdata->fade_after_midpoint != 0 &&
            object->scale_x > midpoint &&
            object->pfx2d->verts[0].a != 0) {
            if (object->pfx2d->verts[0].a > 10) {
                for (index = 0; index < 4; index++) {
                    object->pfx2d->verts[index].a -= 10;
                }
            } else {
                for (index = 0; index < 4; index++) {
                    object->pfx2d->verts[index].a = 0;
                }
            }
        }
    }

    object->scale_x = pdata->target_scale;
    object->scale_y = pdata->target_scale;
    if (pdata->destroy_after_delay != 0) {
        _mkproc_sleep_ticks = (float)pdata->final_delay;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        if (object->instance != 0) {
            destroy_screen_obj(object);
        }
    }
    return 0.0f;
}


/* TODO: [breakthrough needed] 57.163635%; float result restored; prior payload and owner-reload differences remain. */
static float mk_chess_check_leader_vs_leader_condition(void) {
    ChessScaleMessagePdata* pdata;
    MkHdr* raw_pdata;
    ChessPiece* attacker;
    ChessPiece* defender;
    ScreenObj* message;
    int center_x;
    int center_y;

    if (mk_chess_pdata->sides[0]->live_piece_count != 1 ||
        mk_chess_pdata->sides[1]->live_piece_count != 1) {
        return 0.0f;
    }

    _mkproc_sleep_ticks = 80.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    message = load_named_2d_pfxobj(
        0xD003C, 0xC01C, "FINALMATCH_MSG", 0, 0x52);
    center_x = screen_width / 2;
    center_y = screen_height / 2 + 50;

    raw_pdata = 0;
    _create_mkproc_generic_tinystack(
        0xC023, 0x1F, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &raw_pdata);
    pdata = (ChessScaleMessagePdata*)raw_pdata;
    pdata->object = message;
    unhide_screen_obj(message);
    pdata->step = 0.07666667f;
    pdata->start_scale = 0.25f;
    pdata->target_scale = 1.4f;
    pdata->initial_delay = 0;
    pdata->center_x = center_x;
    pdata->center_y = center_y;
    pdata->texture_width = message->pfx2d->tex_w;
    pdata->destroy_after_delay = 1;
    pdata->final_delay = 100;
    pdata->fade_after_midpoint = 0;

    message->scale_x = 0.25f;
    message->scale_y = 0.25f;
    message->x =
        -(int)(((float)(pdata->texture_width >> 1) *
                 message->scale_x) -
               (float)center_x);
    message->y =
        -(int)(((float)(message->pfx2d->tex_h / 2) *
                 message->scale_y) -
               (float)center_y);
    message->flags |= 8;
    snd_req(0x34);

    _mkproc_sleep_ticks = 80.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    attacker = mk_chess_pdata->sides[0]->pieces[0];
    defender = mk_chess_pdata->sides[1]->pieces[0];
    mk_chess_hud_set_piece_portrait(attacker);
    mk_chess_hud_set_piece_portrait(defender);
    mk_chess_request_piece_fight(
        attacker, defender->cell_x, defender->cell_y, 1);
    return 1.0f;
}

/* TODO: [breakthrough needed] 18.91%; retail unrolled art initialization and owner reloads still differ; reconstruct from the full call sequence. */
static void mk_chess_load_team_art(unsigned int side_index) {
    static const char* side_0_names[5] = {
        "RED_BORDER", "POWERBAR_FRAME", "POWERBAR_FILL_RED",
        "POWERBAR_FILL_GREEN", "PLYR_ONE_TURN"
    };
    static const char* side_1_names[5] = {
        "BLUE_BORDER", "POWERBAR_FRAME", "POWERBAR_FILL_RED",
        "POWERBAR_FILL_GREEN", "PLYR_TWO_TURN"
    };
    static const int priorities[5] = {0x52, 0x4E, 0x50, 0x4F, 0x52};
    ChessSideState* side;
    ScreenObj* art;
    MkObj* model;
    MkObj* first_sobj;
    unsigned int index;
    int flags;

    side = mk_chess_pdata->sides[side_index];
    for (index = 0; index < 5; index++) {
        side->team_art[index].screen = 0;
        side->team_art[index].instance = 0;
    }
    side->team_model = 0;
    side->team_model_instance = 0;

    for (index = 0; index < 5; index++) {
        flags = side_index == 1 && index < 4 ? 0x20000000 : 0;
        art = load_named_2d_pfxobj(
            0xD003C, 0xC01C,
            side_index == 1 ? side_1_names[index] : side_0_names[index],
            flags, priorities[index]);
        side->team_art[index].screen = art;
        side->team_art[index].instance = art->instance;
        if (index == 2) {
            art->scale_x = 6.0f;
        }
        hide_screen_obj(art);
    }

    model = load_model_from_slot(0xD003C, 0x0A430004, 0xC01E);
    side->team_model = model;
    side->team_model_instance = model->hdr.instance;
    obj_create_sobjs(model);
    model->light_flags = 0x10;
    first_sobj = (MkObj*)obj_first_sobj(model);
    first_sobj->flags_09 |= 0x80;
    sobj_set_priority(first_sobj, 0x13);
    model->light_flags = 0xD;
    model->flags_08 |= 0x40;
    insert_fgnd_mkobj(model);
    hide_obj(model);
}



void mk_chess_drone_handle_power_cell_change_strategy(ChessDroneState* drone);
static int mk_chess_drone_force_a_fight(ChessDroneState* drone, int mode);
/* TODO: [breakthrough needed] retail strategy dispatcher reaches unwritten destination Y at far-edge kings; full gameplay reachability remains unresolved. */
static int mk_chess_drone_handle_defend_the_king_strategy(ChessDroneState* drone);
static int mk_chess_drone_handle_get_the_king_strategy(ChessDroneState* drone);
/* TODO: [breakthrough needed] retail dispatch and real path generation still consume unwritten ratings; resolve defined behavior. */
static int mk_chess_drone_handle_power_cell_strategy(ChessDroneState* drone);
static int mk_chess_drone_get_close_to_piece(ChessDroneState* drone,
    ChessPiece* target, ChessPiece** selected, int* x, int* y);

static inline unsigned int mk_chess_drone_owned_power_cells(ChessDroneState* drone) {
    unsigned int count = 0;
    ChessPiece* piece = mk_chess_pdata->board[1].cells[4].piece;
    if (piece != 0 && piece->side == drone->piece->side) {
        count = 1;
    }
    piece = mk_chess_pdata->board[8].cells[5].piece;
    if (piece != 0 && piece->side == drone->piece->side) {
        count++;
    }
    return count;
}

/* TODO: [near miss] 76.12%; strategy decisions recovered; branch scheduling, local arrays and coloring differ. */
static int mk_chess_drone_handle_strategy(ChessDroneState* drone) {
    unsigned int roll;
    unsigned int own_count;
    unsigned int enemy_count;
    unsigned int owned_cells;
    ChessPiece* strong_targets[3];
    ChessPiece* other_targets[3];
    unsigned short strong_count;
    unsigned short other_count;
    ChessSideState* enemies;
    ChessPiece* piece;
    ChessPiece* target;
    ChessPiece* selected;
    unsigned int index;
    unsigned int rating;
    int x;
    int y;

    switch (drone->strategy) {
    case 0:
        roll = randu0(100) & 0xFFFF;
        if (game_settings.arcade_difficulty >= 2) {
            if (game_settings.arcade_difficulty == 2) {
                if (roll < 5) {
                    drone->strategy = 4;
                } else if (roll < 8) {
                    drone->strategy = 2;
                }
            } else if (roll < 10) {
                drone->strategy = 4;
            } else if (roll < 20) {
                drone->strategy = 2;
            } else if (roll < 40) {
                drone->strategy = 3;
            } else if (roll < 50) {
                drone->strategy = 1;
            }
        }
        break;
    case 1:
        if (mk_chess_drone_owned_power_cells(drone) == 0 &&
            (randu0(100) & 0xFFFF) < 10) {
            drone->strategy = 2;
        }
        break;
    case 2:
        mk_chess_drone_handle_power_cell_change_strategy(drone);
        break;
    case 3:
        own_count = drone->live_piece_count;
        enemy_count = mk_chess_pdata->sides[!drone->piece->side]->live_piece_count;
        if (own_count < 8) {
            roll = randu0(100) & 0xFFFF;
            if (enemy_count > 8) {
                if (roll < 20) {
                    drone->strategy = 2;
                } else if (roll < 30) {
                    drone->strategy = 1;
                }
            } else if (roll < 10) {
                drone->strategy = 2;
            } else if (roll < 20) {
                drone->strategy = 1;
            }
        } else if ((int)(own_count - enemy_count) > 5) {
            roll = randu0(100) & 0xFFFF;
            if (roll < 20) {
                drone->strategy = 2;
            } else if (roll < 40) {
                drone->strategy = 1;
            } else if (roll < 50) {
                drone->strategy = 4;
            }
        }
        break;
    case 4:
        roll = randu0(100) & 0xFFFF;
        owned_cells = mk_chess_drone_owned_power_cells(drone);
        if (owned_cells == 0 && roll < 10) {
            drone->strategy = 2;
        } else if (roll < 6) {
            drone->strategy = 1;
        } else if (game_settings.arcade_difficulty == 2 && roll < 9) {
            drone->strategy = 0;
        }
        break;
    }
    switch (drone->strategy) {
    case 3:
        return mk_chess_drone_handle_defend_the_king_strategy(drone);
    case 1:
        return mk_chess_drone_handle_get_the_king_strategy(drone);
    case 4:
        strong_count = 0;
        other_count = 0;
        enemies = mk_chess_pdata->sides[!drone->piece->side];
        if (mk_chess_drone_force_a_fight(drone, 1) != 0) {
            return 1;
        }
        for (index = 0; index < enemies->live_piece_count; index++) {
            piece = enemies->pieces[index];
            if (mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type != 1) {
                rating = 0;
            } else if (piece->type == 1) {
                rating = 1;
            } else if (piece->health > 0.8f) {
                rating = 0;
            } else if (piece->health > 0.55f) {
                rating = 1;
            } else if (piece->health > 0.3f) {
                rating = 2;
            } else {
                rating = 3;
            }
            if (strong_count < 3 && rating == 3) {
                strong_targets[strong_count++] = piece;
            } else if (other_count < 3 && rating >= 2) {
                other_targets[other_count++] = piece;
            }
        }
        target = 0;
        if (strong_count != 0 && (randu0(100) & 0xFFFF) < 75) {
            target = strong_targets[randu0(strong_count) & 0xFFFF];
        } else if (other_count != 0) {
            target = other_targets[randu0(other_count) & 0xFFFF];
        }
        if (target != 0 && mk_chess_drone_get_close_to_piece(drone, target, &selected, &x, &y) != 0) {
            drone->target_0_x = selected->cell_x;
            drone->target_0_y = selected->cell_y;
            drone->target_1_x = x;
            drone->target_1_y = y;
            drone->action_state = 1;
            drone->cooldown = 5;
            return 1;
        }
        return 0;
    case 2:
        return mk_chess_drone_handle_power_cell_strategy(drone);
    default:
        return 0;
    }
}

static int mk_chess_drone_fetch_non_king_vulnerable_matchup(ChessDroneState* drone,
    ChessPiece** attackers, ChessPiece** defenders, unsigned int count,
    ChessPiece** selected_attacker, ChessPiece** selected_defender);

/* TODO: [near miss] 87.28%; attack groups recovered; count lifetimes, array addressing and register allocation differ. */
static int mk_chess_drone_force_a_fight(ChessDroneState* drone, int mode) {
    ChessPiece* low_attackers[3];
    ChessPiece* low_defenders[3];
    ChessPiece* medium_attackers[3];
    ChessPiece* medium_defenders[3];
    ChessPiece* high_attackers[3];
    ChessPiece* high_defenders[3];
    unsigned short high_count = 0;
    unsigned short medium_count = 0;
    unsigned short low_count = 0;
    unsigned int index;
    unsigned int move;
    unsigned int rating;
    unsigned int slot;
    ChessPiece* attacker;
    ChessPiece* defender;
    ChessMoveTarget* target;

    if ((randu0(100) & 0xFFFF) < 10) {
        return 0;
    }
    for (index = 0; index < drone->live_piece_count; index++) {
        attacker = drone->pieces[index];
        if (mk_chess_pdata->board[attacker->cell_x].cells[attacker->cell_y].square_type == 0 &&
            attacker->type != 5) {
            for (move = 0; move < drone->pieces[index]->move_map->target_count; move++) {
                target = &drone->pieces[index]->move_map->targets[move];
                if (target->kind == 1) {
                    defender = mk_chess_pdata->board[target->cell_x].cells[target->cell_y].piece;
                    rating = mk_chess_drone_calc_fight_favorability_rating(drone->pieces[index], defender);
                    if (rating > 7) {
                        if (high_count < 3) {
                            high_attackers[high_count] = drone->pieces[index];
                            high_defenders[high_count] = defender;
                            high_count++;
                        } else {
                            slot = randu0(6) & 0xFFFF;
                            if (slot < 3) {
                                high_attackers[slot] = drone->pieces[index];
                                high_defenders[slot] = defender;
                            }
                        }
                    }
                    else if (rating > 3) {
                        if (drone->pieces[index]->type != 2) {
                            if (medium_count < 3) {
                                medium_attackers[medium_count] = drone->pieces[index];
                                medium_defenders[medium_count] = defender;
                                medium_count++;
                            } else {
                                slot = randu0(6) & 0xFFFF;
                                if (slot < 3) {
                                    medium_attackers[slot] = drone->pieces[index];
                                    medium_defenders[slot] = defender;
                                }
                            }
                        }
                    }
                    else {
                        if (drone->pieces[index]->type != 2) {
                            if (low_count < 3) {
                                low_attackers[low_count] = drone->pieces[index];
                                low_defenders[low_count] = defender;
                                low_count++;
                            } else {
                                slot = randu0(6) & 0xFFFF;
                                if (slot < 3) {
                                    low_attackers[slot] = drone->pieces[index];
                                    low_defenders[slot] = defender;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    defender = 0;
    attacker = 0;
    if (mode == 0) {
        if (medium_count != 0 && (randu0(100) & 0xFFFF) < 50) {
            slot = randu0(medium_count) & 0xFFFF;
            defender = medium_defenders[slot];
            attacker = medium_attackers[slot];
        } else if (high_count != 0 && (randu0(100) & 0xFFFF) < 80) {
            slot = randu0(high_count) & 0xFFFF;
            defender = high_defenders[slot];
            attacker = high_attackers[slot];
        } else if (low_count != 0) {
            slot = randu0(low_count) & 0xFFFF;
            defender = low_defenders[slot];
            attacker = low_attackers[slot];
        }
    } else if (mk_chess_drone_fetch_non_king_vulnerable_matchup(drone,
            high_attackers, high_defenders, high_count, &attacker, &defender) == 0 &&
        mk_chess_drone_fetch_non_king_vulnerable_matchup(drone,
            medium_attackers, medium_defenders, medium_count, &attacker, &defender) == 0) {
        return 0;
    }
    if (defender != 0 && attacker != 0) {
        drone->target_0_x = attacker->cell_x;
        drone->target_0_y = attacker->cell_y;
        drone->target_1_x = defender->cell_x;
        drone->target_1_y = defender->cell_y;
        drone->action_state = 1;
        drone->cooldown = 5;
        return 1;
    }
    return 0;
}

static int mk_chess_calc_all_moves_cb(ChessPiece* piece,
    unsigned int context0, unsigned int context1, unsigned int context2);

static inline void mk_chess_recalculate_all_piece_moves(void) {
    unsigned int side;
    unsigned int index;

    for (side = 0; side < 2; side++) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
            mk_chess_calc_all_moves_cb(mk_chess_pdata->sides[side]->pieces[index], 0, 0, 0);
        }
    }
}

/* TODO: [near miss] 95.45%; trial/restore order recovered; king scan, board addressing and register allocation differ. */
static int mk_chess_drone_fetch_non_king_vulnerable_matchup(ChessDroneState* drone,
    ChessPiece** attackers, ChessPiece** defenders, unsigned int count,
    ChessPiece** selected_attacker, ChessPiece** selected_defender) {
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
    unsigned int index;
    unsigned int enemy_index;
    unsigned int enemy_side;
    unsigned char original_x;
    unsigned char original_y;
    int found = 0;
    int threatened;
    ChessPiece* piece;

    for (index = 0; index < count; index++) {
        original_x = attackers[index]->cell_x;
        original_y = attackers[index]->cell_y;
        mk_chess_pdata->board[original_x].cells[original_y].piece = 0;
        attackers[index]->cell_x = defenders[index]->cell_x;
        attackers[index]->cell_y = defenders[index]->cell_y;
        piece = attackers[index];
        mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].piece = piece;
        mk_chess_recalculate_all_piece_moves();
        threatened = 0;
        enemy_side = !drone->piece->side;
        for (enemy_index = 0; enemy_index < mk_chess_pdata->sides[enemy_side]->live_piece_count; enemy_index++) {
            piece = mk_chess_pdata->sides[enemy_side]->pieces[enemy_index];
            if (((piece->move_map->rows[king->cell_y] >> (king->cell_x * 3)) & 7) == 2) {
                threatened = 1;
                break;
            }
        }
        if (threatened == 0 && (found == 0 || (randu0(100) & 0xFFFF) < 30)) {
            found = 1;
            *selected_attacker = attackers[index];
            *selected_defender = defenders[index];
        }
        defenders[index]->cell_x = attackers[index]->cell_x;
        defenders[index]->cell_y = attackers[index]->cell_y;
        piece = attackers[index];
        mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].piece = defenders[index];
        attackers[index]->cell_x = original_x;
        attackers[index]->cell_y = original_y;
        mk_chess_pdata->board[original_x].cells[original_y].piece = attackers[index];
    }
    mk_chess_recalculate_all_piece_moves();
    return found;
}

void mk_chess_drone_handle_power_cell_change_strategy(
    ChessDroneState* drone) {
    ChessPiece* power_cell_piece;
    unsigned int owned_cells;
    unsigned int roll;

    owned_cells = 0;
    power_cell_piece = mk_chess_pdata->board[1].cells[4].piece;
    if (power_cell_piece != 0 &&
        power_cell_piece->side == drone->piece->side) {
        owned_cells++;
    }
    power_cell_piece = mk_chess_pdata->board[8].cells[5].piece;
    if (power_cell_piece != 0 &&
        power_cell_piece->side == drone->piece->side) {
        owned_cells++;
    }

    if (owned_cells == 2) {
        roll = randu0(100) & 0xFFFF;
        if (roll < 25) {
            drone->strategy = 3;
        } else if (roll < 50) {
            drone->strategy = 4;
        } else {
            drone->strategy = 1;
        }
    } else if (owned_cells == 1) {
        roll = randu0(100) & 0xFFFF;
        if (game_settings.arcade_difficulty < 3) {
            if (roll < 3) {
                drone->strategy = 1;
            } else if (roll < 8) {
                drone->strategy = 4;
            } else if (roll < 10) {
                drone->strategy = 0;
            }
        } else if (roll < 8) {
            drone->strategy = 1;
        } else if (roll < 10) {
            drone->strategy = 4;
        }
    }

    if (drone->live_piece_count < 7 && (randu0(100) & 0xFFFF) < 30) {
        drone->strategy = 1;
    }
}


/* TODO: [near miss] 96.79%; expiry and notification CFG restored; constant relocation or register residue remains; five-attempt limit. */
int mk_chess_handle_turn_timeout_scenerios(unsigned int side) {
    int show_message = 0;
    int message = 0;
    unsigned int timeout;
    if (mk_chess_pdata->turn_timeout != 0) {
        mk_chess_pdata->turn_timeout--;
        timeout = mk_chess_pdata->turn_timeout;
        if (timeout == 0) {
            if (mk_chess_pdata->manager.input_state == 9 &&
                mk_chess_pdata->manager.spell->state < 0x18 &&
                mk_chess_pdata->manager.spell->state >= 0x13) {
                mk_chess_pdata->turn_timeout = 1;
                return 0;
            }
            if (side < 2) mk_chess_timeout_msg(5, side);
            return 1;
        }
        if (timeout < 620) {
            if (timeout == (unsigned int)(600.0f * inverse_game_speed)) {
                show_message = 1; message = 0;
            } else if (timeout == (unsigned int)(480.0f * inverse_game_speed)) {
                show_message = 1; message = 1;
            } else if (timeout == (unsigned int)(360.0f * inverse_game_speed)) {
                show_message = 1; message = 2;
            } else if (timeout == (unsigned int)(240.0f * inverse_game_speed)) {
                show_message = 1; message = 3;
            } else if (timeout == (unsigned int)(120.0f * inverse_game_speed)) {
                show_message = 1; message = 4;
            }
            if (show_message) mk_chess_timeout_msg(message, side);
        }
    }
    return 0;
}

int mk_chess_fetch_single_space_attack_script(ChessAttackGridPosition* position, unsigned int direction);

static inline unsigned int mk_chess_action_event_script(int event) {
    unsigned int script = (unsigned int)g_board_game_controller.class_definitions[
        g_active_piece->type].event_scripts[event];
    if (script == 0xABABAB00) {
        return (unsigned int)g_board_game_controller.piece_art_rows[event];
    }
    return script;
}

/* TODO: [breakthrough needed] 68.57872%; selection recovered; retail unused distance calculation and local lowering remain. */
unsigned int mk_chess_request_piece_script_for_action(int action) {
    unsigned int distance;
    int event;
    switch (action) {
    case 5:
        event = mk_chess_fetch_desired_attack_script(g_active_piece,
            (ChessAttackGridPosition*)&g_active_piece->movement->cell_x, &distance);
        if (event >= 64) {
            return (unsigned int)g_board_game_controller.class_definitions[
                g_active_piece->type].event_scripts[1];
        }
        return mk_chess_action_event_script(event);
    case 26: {
        ChessPieceMovement* movement = g_active_piece->movement;
        int dx = (int)movement->event_byte_12 - movement->cell_x;
        int dy = (int)movement->event_byte_13 - movement->cell_y;
        unsigned int direction;
        if (dx == 0) {
            direction = dy > 0 ? 0 : 4;
        } else if (dy == 0) {
            direction = dx > 0 ? 2 : 6;
        } else if (dx > 0) {
            direction = dy > 0 ? 1 : 3;
        } else {
            direction = dy > 0 ? 7 : 5;
        }
        if (g_active_piece->side == 0) {
            direction += 4;
            if (direction >= 8) {
                direction -= 8;
            }
        }
        event = mk_chess_fetch_single_space_attack_script(
            (ChessAttackGridPosition*)&g_active_piece->movement->cell_x, direction);
        if (event >= 64) {
            return (unsigned int)g_board_game_controller.class_definitions[
                g_active_piece->type].event_scripts[1];
        }
        return mk_chess_action_event_script(event);
    }
    case 8: {
        ChessPieceMovement* movement = g_active_piece->movement;
        if (movement->event_byte_12 <= 1 || movement->event_byte_12 >= 8 ||
            movement->event_byte_13 <= 1 || movement->event_byte_13 >= 8) {
            return mk_chess_action_event_script(43);
        }
        return mk_chess_action_event_script(22);
    }
    case 12: {
        ChessPieceMovement* movement = g_active_piece->movement;
        if (movement->event_byte_12 <= 1 || movement->event_byte_12 >= 8 ||
            movement->event_byte_13 <= 1 || movement->event_byte_13 >= 8) {
            return mk_chess_action_event_script(44);
        }
        return mk_chess_action_event_script(25);
    }
    default:
        return mk_chess_action_event_script(11);
    }
}

/* TODO: [breakthrough] 48.24762%; all board-coordinate cases agree; absolute-value and dispatch lowering remain. */
static int mk_chess_fetch_desired_attack_script(
    ChessPiece* piece,
    ChessAttackGridPosition* attack,
    unsigned int* distance) {
    int delta_x;
    int delta_y;
    unsigned int direction;
    unsigned int abs_x;
    unsigned int abs_y;

    delta_x = (int)attack->cell_x - (int)attack->origin_x;
    delta_y = (int)attack->cell_y - (int)attack->origin_y;
    abs_x = delta_x < 0 ? -delta_x : delta_x;
    abs_y = delta_y < 0 ? -delta_y : delta_y;
    *distance = abs_y;

    if (delta_x == 0) {
        direction = delta_y > 0 ? 0 : 4;
    } else if (delta_y == 0) {
        *distance = abs_x;
        direction = delta_x > 0 ? 2 : 6;
    } else {
        if (*distance < abs_x) {
            *distance = abs_x;
        }
        if (delta_x > 0) {
            direction = delta_y > 0 ? 1 : 3;
        } else {
            direction = delta_y > 0 ? 7 : 5;
        }
    }

    if (piece->side == 0) {
        direction = (direction + 4) & 7;
    }
    if (*distance == 1) {
        return mk_chess_fetch_single_space_attack_script(
            attack, direction);
    }
    if (*distance != 2) {
        return 0x40;
    }

    switch (direction) {
    case 4:
        return 0x36;
    case 0:
        return 0x37;
    case 6:
        return 0x38;
    case 2:
        return 0x39;
    case 5:
        return 0x3A;
    case 3:
        return 0x3B;
    case 7:
        return 0x3C;
    default:
        return 0x3D;
    }
}

static int mk_chess_handle_attack_request(ChessPiece* piece, const unsigned char* attack, unsigned int* distance);
static void mk_chess_handle_guy_falling_from_sky_event(ChessPiece* piece);

static inline unsigned int mk_chess_event_direction(int dx, int dy, unsigned int* distance)
{
    unsigned int ax = dx < 0 ? -dx : dx;
    unsigned int ay = dy < 0 ? -dy : dy;
    *distance = ay;
    if (dx == 0) return dy > 0 ? 0 : 4;
    if (dy == 0) {
        *distance = ax;
        return dx > 0 ? 2 : 6;
    }
    if (*distance < ax) *distance = ax;
    if (dx > 0) return dy > 0 ? 1 : 3;
    return dy > 0 ? 7 : 5;
}

static inline unsigned int mk_chess_relative_event_direction(ChessPiece* piece, unsigned int direction)
{
    if (piece->side == 0) {
        direction += 4;
        if (direction >= 8) direction -= 8;
    }
    return direction;
}

/* TODO: [breakthrough needed] 61.920288%; verify movement and reaction branches; dispatch scheduling remains. */
void mk_chess_piece_event(ChessPiece* piece, unsigned int event, void* data)
{
    unsigned char* coordinates = (unsigned char*)data;
    unsigned int distance;
    unsigned int direction;
    unsigned int side, index;
    unsigned int timeout;
    int script;
    ChessPiece* other;
    piece->current_event = event;
    switch (event) {
    case 24: mk_chess_handle_guy_falling_from_sky_event(piece); return;
    case 23: mk_chess_xfer_to_piece_script(piece, 34); return;
    case 0:
        if (piece != 0) {
            switch (piece->type) {
            case 5: fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f); break;
            case 3:
                fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
                fx_set(piece->runtime.fields.secondary_effect, 0x204, 1.0f);
                fx_pause_emit(piece->runtime.fields.primary_effect);
                fx_pause_emit(piece->runtime.fields.secondary_effect);
                break;
            case 4: fx_pause_emit(piece->runtime.fields.primary_effect); break;
            }
        }
        mk_chess_xfer_to_piece_script(piece, 1); return;
    case 1: mk_chess_xfer_to_piece_script(piece, 2); return;
    case 2: mk_chess_xfer_to_piece_script(piece, 3); return;
    case 3: mk_chess_xfer_to_piece_script(piece, 4); return;
    case 4:
        memcpy(&piece->movement->event_data, data, sizeof(ChessMovementEvent));
        direction = mk_chess_event_direction(coordinates[2] - coordinates[0], coordinates[3] - coordinates[1], &distance);
        direction = mk_chess_relative_event_direction(piece, direction);
        piece->movement->event_value = (float)distance;
        script = 9;
        if (distance == 1) {
            switch (direction) {
            case 4: script = 5; break;
            case 0: script = 6; break;
            case 6: script = 7; break;
            case 2: script = 8; break;
            case 5: script = 46; break;
            case 3: script = 47; break;
            case 7: script = 48; break;
            default: script = 49; break;
            }
        } else if (distance == 2) {
            switch (direction) {
            case 4: script = 50; break;
            case 0: script = 51; break;
            case 6: script = 52; break;
            case 2: script = 53; break;
            }
        }
        mk_chess_xfer_to_piece_script(piece, script); return;
    case 7: mk_chess_xfer_to_piece_script(piece, 10); return;
    case 27: mk_chess_xfer_to_piece_script(piece, 62); return;
    case 28: mk_chess_xfer_to_piece_script(piece, 63); return;
    case 20:
        for (side = 0; side < 2; side++) {
            for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
                other = mk_chess_pdata->sides[side]->pieces[index];
                if (other->type != 5 && other->type != 0 && other->state == 0) {
                    direction = mk_chess_event_direction(piece->cell_x - other->cell_x, piece->cell_y - other->cell_y, &distance);
                    direction = mk_chess_relative_event_direction(other, direction);
                    if (distance == 1) {
                        switch (direction) {
                        case 0: mk_chess_xfer_to_piece_script(other, 30); break;
                        case 6: mk_chess_xfer_to_piece_script(other, 31); break;
                        case 2: mk_chess_xfer_to_piece_script(other, 32); break;
                        default:
                            if ((direction - 4 <= 1 || direction == 3) &&
                                piece->object->pos_vel.y > 0.1f && piece->object->flags_08_bits.gravity_enabled)
                                mk_chess_xfer_to_piece_script(other, 33);
                            break;
                        }
                    }
                }
            }
        }
        return;
    case 5:
        memcpy(&piece->movement->event_data, data, sizeof(ChessMovementEvent));
        if (mk_chess_handle_attack_request(piece, coordinates, &distance) == 0) {
            piece->movement->event_value = (float)distance;
            piece->movement->event_byte_14 = coordinates[2] - (coordinates[2] - coordinates[0]) / (unsigned char)distance;
            piece->movement->event_byte_15 = coordinates[3] - (coordinates[3] - coordinates[1]) / (unsigned char)distance;
            mk_chess_xfer_to_piece_script(piece, 19);
            if (piece == 0) return;
            timeout = 599;
            do {
                _mkproc_sleep_ticks = 1.0f;
                ((ChessProcVtable*)aproc->vtbl)->sleep();
            } while (piece->state != 0 && timeout-- != 0);
        }
        return;
    case 6:
        direction = mk_chess_event_direction(coordinates[2] - coordinates[0], coordinates[3] - coordinates[1], &distance);
        direction = mk_chess_relative_event_direction(piece, direction);
        if (coordinates[2] <= 1 || coordinates[2] >= 8 || coordinates[3] <= 1 || coordinates[3] >= 8) {
            switch (direction) {
            case 0: mk_chess_xfer_to_piece_script(piece, 36); return;
            case 4: mk_chess_xfer_to_piece_script(piece, 38); return;
            case 6: mk_chess_xfer_to_piece_script(piece, 40); return;
            case 2: mk_chess_xfer_to_piece_script(piece, 42); return;
            }
        } else {
            switch (direction) {
            case 0: case 1: case 7: script = 12; break;
            case 4: script = 14; break;
            case 6: script = 16; break;
            case 2: script = 18; break;
            default: script = 14; break;
            }
            mk_chess_xfer_to_piece_script(piece, script);
        }
        return;
    case 14: mk_chess_xfer_to_piece_script(piece, 26); return;
    case 15: mk_chess_xfer_to_piece_script(piece, 27); return;
    }
}

static int mk_chess_handle_attack_request(
    ChessPiece* piece,
    const unsigned char* attack,
    unsigned int* distance) {
    int script;

    script = mk_chess_fetch_desired_attack_script(
        piece, (ChessAttackGridPosition*)attack, distance);
    if (script == 0x40) {
        return 0;
    }
    mk_chess_xfer_to_piece_script(piece, script);
    return 1;
}

static inline int mk_chess_choose_king_neighbor(ChessPiece* king,
    unsigned char* selected_x, unsigned char* selected_y)
{
    unsigned char side = king->side;
    unsigned char king_x = king->cell_x;
    unsigned char king_y = king->cell_y;
    unsigned char x, y;
    int found = 0;
    for (x = 0; x < 10; x++) {
        int dx = x - king_x;
        for (y = 0; y < 10; y++) {
            int dy = y - king_y;
            unsigned int distance;
            ChessPiece* candidate;
            mk_chess_event_direction(dx, dy, &distance);
            if (distance == 1) {
                candidate = mk_chess_pdata->board[x].cells[y].piece;
                if (candidate != 0 && side == candidate->side &&
                    (found != 1 || (unsigned short)randu0(100) >= 50)) {
                    *selected_x = x;
                    *selected_y = y;
                    found = 1;
                }
            }
        }
    }
    return found;
}

static inline ChessPiece* mk_chess_opening_target(unsigned int side, int type)
{
    ChessSideState* team = mk_chess_pdata->sides[side];
    unsigned int i;
    for (i = 0; i < team->live_piece_count; i++) {
        ChessPiece* piece = team->pieces[i];
        if (piece->type == type && mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type != 1)
            return team->pieces[i];
    }
    return 0;
}

/* TODO: [breakthrough] 79.70%; tested opening paths agree; neighbor-scan and target-search lowering remain. */
static int mk_chess_drone_handle_the_big_chill_opening_move(ChessDroneState* drone)
{
    unsigned int enemy_side = drone->piece->side == 0;
    ChessPiece* king = mk_chess_find_piece_of_type(enemy_side, 5, 0);
    ChessPiece* target;
    unsigned char x, y;
    if (mk_chess_drone_check_spell(drone, 1, 2) == 1 &&
        mk_chess_pdata->board[drone->desired_x].cells[drone->desired_y].piece != 0) {
        if (mk_chess_choose_king_neighbor(king, &x, &y) &&
            mk_chess_drone_attempt_to_cast_spell(drone, 1, 2, x, y, drone->desired_x, drone->desired_y)) return 1;
    }
    target = mk_chess_opening_target(enemy_side, 2);
    if (target == 0) target = mk_chess_opening_target(enemy_side, 1);
    if (target != 0 && mk_chess_drone_attempt_to_cast_spell(drone, 0, 1,
        target->cell_x, target->cell_y, drone->desired_x, drone->desired_y)) return 1;
    if (mk_chess_choose_king_neighbor(king, &x, &y)) {
        if (mk_chess_drone_attempt_to_cast_spell(drone, 1, 0, x, y, 255, 255)) return 1;
        target = mk_chess_opening_target(enemy_side, 0);
        /* This retail opening path requires an eligible class-zero fallback. */
        if (mk_chess_drone_attempt_to_cast_spell(drone, 1, 0, target->cell_x, target->cell_y, 255, 255)) return 1;
    }
    if ((unsigned short)randu0(100) < 20) drone->strategy = 2;
    else drone->strategy = 1;
    return 0;
}

static inline int mk_chess_first_empty_king_neighbor(ChessPiece* king,
    int* selected_x, int* selected_y)
{
    unsigned int x, y;
    for (x = 0; x < 10; x++) {
        int dx = (int)x - king->cell_x;
        for (y = 0; y < 10; y++) {
            unsigned int distance;
            mk_chess_event_direction(dx, (int)y - king->cell_y, &distance);
            if (distance == 1 && mk_chess_pdata->board[x].cells[y].piece == 0) {
                *selected_x = x;
                *selected_y = y;
                return 1;
            }
        }
    }
    return 0;
}

static inline ChessPiece* mk_chess_drone_opening_target(ChessDroneState* drone, int type)
{
    unsigned int i;
    for (i = 0; i < drone->live_piece_count; i++) {
        ChessPiece* piece = drone->pieces[i];
        if (piece->type == type && mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type != 1)
            return drone->pieces[i];
    }
    return 0;
}

/* TODO: [breakthrough] 66.60%; tested retry paths agree; neighbor-scan and target-search lowering remain. */
static int mk_chess_drone_handle_get_the_king_strategy(ChessDroneState* drone)
{
    ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side == 0, 5, 0);
    ChessPiece* selected;
    int x, y;
    unsigned int distance;
    unsigned char neighbor_x, neighbor_y;
    int spell_available = mk_chess_drone_check_spell(drone, 0, 1);
    int retry;
    if (spell_available == 1 && mk_chess_first_empty_king_neighbor(king, &x, &y) &&
        (unsigned short)randu0(100) < 75) {
        selected = mk_chess_drone_opening_target(drone, 1);
        if (selected == 0) selected = mk_chess_drone_opening_target(drone, 2);
        if (selected != 0 && mk_chess_drone_attempt_to_cast_spell(drone, 0, 1,
            selected->cell_x, selected->cell_y, x, y)) return 1;
    }
    for (retry = 3; retry > 0; retry--) {
        unsigned short roll = randu0(100);
        if (roll < 50) {
            if (mk_chess_drone_check_spell(drone, 1, 2) == 1 &&
                !mk_chess_first_empty_king_neighbor(king, &x, &y) &&
                mk_chess_choose_king_neighbor(king, &neighbor_x, &neighbor_y)) {
                selected = mk_chess_drone_opening_target(drone, 1);
                if (selected == 0) selected = mk_chess_drone_opening_target(drone, 2);
                if (selected != 0) {
                    mk_chess_event_direction(king->cell_x - selected->cell_x,
                        king->cell_y - selected->cell_y, &distance);
                    if (distance > 2 && mk_chess_drone_attempt_to_cast_spell(drone, 1, 2,
                        selected->cell_x, selected->cell_y, neighbor_x, neighbor_y)) return 1;
                }
            }
        } else if (roll < 80) {
            if (spell_available == 1 && !mk_chess_first_empty_king_neighbor(king, &x, &y) &&
                mk_chess_choose_king_neighbor(king, &neighbor_x, &neighbor_y) &&
                mk_chess_drone_attempt_to_cast_spell(drone, 0, 1, neighbor_x, neighbor_y,
                    drone->desired_x, drone->desired_y)) return 1;
        } else {
            if (mk_chess_drone_check_spell(drone, 1, 0) == 1 &&
                !mk_chess_first_empty_king_neighbor(king, &x, &y) &&
                mk_chess_choose_king_neighbor(king, &neighbor_x, &neighbor_y) &&
                mk_chess_drone_attempt_to_cast_spell(drone, 1, 0,
                    neighbor_x, neighbor_y, 255, 255)) return 1;
        }
    }
    if (mk_chess_drone_get_close_to_piece(drone, king, &selected, &x, &y)) {
        drone->target_0_x = selected->cell_x;
        drone->target_0_y = selected->cell_y;
        drone->target_1_x = x;
        drone->target_1_y = y;
        drone->action_state = 1;
        drone->cooldown = 5;
        return 1;
    }
    return 0;
}

static int mk_chess_drone_attempt_to_cast_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell,
    unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    int ordinal = mk_chess_drone_spellcaster_ordinal(drone, target);

    if (ordinal < 0) {
        return 0;
    }
    if (mk_chess_drone_validate_spell(drone, ordinal, spell, x0, y0, x1, y1) == 1) {
        drone->state = 0x10;
        drone->spell = spell;
        drone->spell_target = ordinal;
        drone->target_0_x = x0;
        drone->target_0_y = y0;
        drone->target_1_x = x1;
        drone->target_1_y = y1;
        drone->action_state = 5;
        drone->cooldown = 0x50;
        return 1;
    }
    return 0;
}

static int mk_chess_drone_validate_spell(
    ChessDroneState* drone,
    unsigned int target,
    unsigned int spell,
    unsigned int target_0_x,
    unsigned int target_0_y,
    unsigned int target_1_x,
    unsigned int target_1_y) {
    ChessSpellState hud;

    if (mk_chess_drone_check_spell(drone, target, spell) == 1) {
        drone->state = 0x10;
        drone->spell = spell;
        drone->spell_target = target;
        mk_chess_drone_setup_spell_hud_pdata_for(
            drone, &hud, target, spell);
        drone->target_0_x = target_0_x;
        drone->target_0_y = target_0_y;
        drone->target_1_x = target_1_x;
        drone->target_1_y = target_1_y;
        if (mk_chess_drone_validate_target(
                drone, &hud, target_0_x, target_0_y, 0) == 0) {
            return 0;
        }
        if (mk_chess_drone_validate_target(
                drone, &hud, target_1_x, target_1_y, 1) == 1) {
            return 1;
        }
    }
    return 0;
}

static inline ChessPiece* mk_chess_find_spellcaster_on_side(
    unsigned int side, unsigned int target) {
    unsigned int ordinal = 0;
    unsigned int index;

    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        if ((int)g_board_game_controller.class_definitions[
                mk_chess_pdata->sides[side]->pieces[index]->type].flags.spellcaster == 1) {
            if (ordinal == target) {
                return mk_chess_pdata->sides[side]->pieces[index];
            }
            ordinal++;
        }
    }
    return 0;
}

static void mk_chess_drone_setup_spell_hud_pdata_for(
    ChessDroneState* drone, ChessSpellState* hud, unsigned int target,
    unsigned int spell) {
    hud->caster_index = target;
    hud->caster = mk_chess_find_spellcaster_on_side(drone->piece->side, target);
    hud->spell_number = spell;
    hud->side = drone->piece->side;
}

static inline unsigned int* mk_chess_piece_restriction(
    ChessPiece* piece, unsigned int restriction) {
    if (piece == 0) {
        return 0;
    }
    if (restriction < 6) {
        return &piece->access_restrictions[restriction];
    }
    return &piece->access_restrictions[0];
}

static inline int mk_chess_piece_access_allowed(
    ChessPiece* piece, unsigned int restriction) {
    unsigned int* expires = mk_chess_piece_restriction(piece, restriction);
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;

    if (*expires >= (unsigned int)manager->clock) {
        return 0;
    }
    return 1;
}

/* TODO: [near miss] 86.39%; access-helper guard scheduling remains; stop at five cumulative attempts/reviews. */
static int mk_chess_drone_check_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell) {
    ChessPiece* caster = mk_chess_find_spellcaster_on_side(drone->piece->side, target);

    if (caster == 0) {
        return 0;
    }
    if ((caster->used_spells & (1U << spell)) != 0) {
        return 0;
    }
    if (mk_chess_piece_access_allowed(caster, 3) == 0) {
        return 0;
    }
    return 1;
}



/* TODO: [breakthrough needed] 91.93%; resolve retail slwi r0, r26, 2 and its surrounding ownership/CFG before further tuning. */
static int mk_chess_deal_with_threat_to(
    unsigned int side,
    ChessPiece* threatened,
    ChessPiece* attacker) {
    ChessDroneState* drone;
    unsigned int attempt;
    unsigned int roll;
    unsigned int edge_y;

    drone = (ChessDroneState*)mk_chess_pdata->sides[side];
    edge_y = side == 0 ? 0 : 9;
    for (attempt = 0; attempt < 8; attempt++) {
        roll = randu0(100) & 0xFFFF;
        if (roll < 10) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 0, 1, attacker->cell_x, attacker->cell_y,
                    drone->desired_x, drone->desired_y)) {
                return 1;
            }
        } else if (roll < 15) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 0, 1, attacker->cell_x, attacker->cell_y,
                    0, edge_y)) {
                return 1;
            }
        } else if (roll < 20) {
            if (threatened != 0 &&
                mk_chess_drone_attempt_to_cast_spell(
                    drone, 0, 3, threatened->cell_x, threatened->cell_y,
                    0xFF, 0xFF)) {
                return 1;
            }
        } else if (roll < 25) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 1, 1, attacker->cell_x, attacker->cell_y,
                    0xFF, 0xFF)) {
                return 1;
            }
        } else if (roll < 35) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 1, 0, attacker->cell_x, attacker->cell_y,
                    0xFF, 0xFF)) {
                return 1;
            }
        } else if (roll < 45) {
            if (threatened != 0 &&
                mk_chess_drone_move_piece_out_of_danger(side, threatened)) {
                return 1;
            }
        } else if (roll < 70) {
            if (mk_chess_drone_move_best_matchup_against_piece(
                    side, attacker, 0)) {
                return 1;
            }
        } else if (roll < 90 && threatened != 0 &&
                   mk_chess_drone_help_piece_by_blocking(
                       side, threatened, attacker)) {
            return 1;
        }
    }
    return 0;
}


/* TODO: [breakthrough needed] 95.77%; success return and low-roll fallthrough restored; loop scheduling remains. */
static int mk_chess_deal_with_minor_threat_to(
    unsigned int side,
    ChessPiece* threatened,
    ChessPiece* attacker) {
    ChessDroneState* drone;
    unsigned int attempt;
    unsigned int roll;

    drone = (ChessDroneState*)mk_chess_pdata->sides[side];
    for (attempt = 0; attempt < 8; attempt++) {
        roll = randu0(100) & 0xFFFF;
        if (roll < 20) {
            if (threatened != 0 &&
                mk_chess_drone_attempt_to_cast_spell(
                    drone, 0, 3, threatened->cell_x, threatened->cell_y,
                    0xFF, 0xFF)) {
                return 1;
            }
        }
        if (roll < 35) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 1, 0, attacker->cell_x, attacker->cell_y,
                    0xFF, 0xFF)) {
                return 1;
            }
        } else if (roll < 70) {
            if (threatened != 0 &&
                mk_chess_drone_move_piece_out_of_danger(side, threatened)) {
                return 1;
            }
        } else if (roll < 80) {
            if (mk_chess_drone_move_best_matchup_against_piece(
                    side, attacker, 0)) {
                return 1;
            }
        } else if (roll < 90 && threatened != 0 &&
                   mk_chess_drone_help_piece_by_blocking(
                       side, threatened, attacker)) {
            return 1;
        }
    }
    return 0;
}

/* TODO: [near miss] 99.71%; data-value exact; anonymous relocation identity remains. */
static float mk_chess_spell_hud_target_1_text_faded_goto_next_target_cb(void) {
    ChessSpellHudPdata* hud;

    hud = (ChessSpellHudPdata*)apdata;
    mk_chess_spell_hud_show_page(hud->target_1, 1);
    hud->target_1->state = 0x12;
    return 0.0f;
}


/* TODO: [near miss] 99.81%; data-value exact; anonymous relocation identity remains. */
static float mk_chess_piece_handle_event(void) {
    AniScript* requested;

    g_active_piece->requested_script = 0;
    cmdscript_setup_execution(
        g_board_game_controller.command_script,
        g_active_piece->runtime.fields.event_script);
    cmdscript_execute(g_board_game_controller.command_script);
    g_active_piece->flags.unknown_bit4 = 1;
    requested = g_active_piece->requested_script;
    if (requested != 0) {
        g_active_piece->requested_script = 0;
        cmdscript_setup_execution(
            g_board_game_controller.command_script, (unsigned int)requested);
        cmdscript_execute(g_board_game_controller.command_script);
    }
    g_active_piece->flags.unknown_bit4 = 1;
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_piece_proc_entry, 0.0f);
    return 0.0f;
}

static inline void mk_chess_start_active_piece_script(void) {
    g_active_piece->flags.snap_into_stance = 1;
    cmdscript_setup_execution(
        g_board_game_controller.command_script,
        (unsigned int)g_active_piece->normal_stance_script);
    cmdscript_execute(g_board_game_controller.command_script);
    g_active_piece->flags.unknown_bit4 = 1;
}

/* TODO: [near miss] 99.72%; data-value exact; anonymous relocation identity remains. */
static float p_mk_chess_piece_init(void) {
    ChessProcVtable* vtable;

    mk_chess_start_active_piece_script();
    vtable = (ChessProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_mk_chess_piece_proc_entry, 0.0f);
    return 0.0f;
}

void mk_chess_snap_to_stance(void) {
    mk_chess_start_active_piece_script();
}

void mk_chess_launch_up(float vertical_speed, float gravity) {
    g_active_piece->object->pos_vel.y = vertical_speed;
    g_active_piece->object->gravity = gravity;
    g_active_piece->object->flags_08_bits.moving = 1;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 1;
    g_active_piece->object->flags_09_bits.bit6 = 0;
}


/* TODO: [near miss] 99.85%; data-value exact; anonymous relocation identity remains. */
void mk_chess_stop_me(void) {
    g_active_piece->object->flags_08_bits.moving = 0;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 0;
    g_active_piece->object->pos_vel.z = 0.0f;
    g_active_piece->object->pos_vel.y = 0.0f;
    g_active_piece->object->pos_vel.x = 0.0f;
    g_active_piece->object->flags_09_bits.bit6 = 1;
}


/* TODO: [near miss] 99.86%; data-value exact; anonymous relocation identity remains. */
void mk_chess_ani_idle(void) {
    mk_chess_stop_me();
    g_active_piece->flags.unknown_bit4 = 0;
}

/* TODO: [near miss] 99.76%; data-value exact; anonymous relocation identity remains. */
void mk_chess_ani_loop_more_frames(float frames) {
    ChessAnimPdata* animation;

    animation = g_active_piece->animation;
    while (frames > 0.0f) {
        advance_anim((AnimPdata*)animation);
        pose_anim((AnimPdata*)animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        frames -= 1.0f;
    }
}


/* TODO: [breakthrough needed] 99.11%; resolve retail lwz r30, 0x5c(r3) and its surrounding ownership/CFG before further tuning. */
void mk_chess_ani_to_frame_x(float target) {
    ChessAnimPdata* animation = g_active_piece->animation;

    while (animation->frame <= target) {
        ChessAnimPdata* current = g_active_piece->animation;
        advance_anim((AnimPdata*)current);
        pose_anim((AnimPdata*)current, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        if (animation->frame + animation->speed * game_speed > target) {
            break;
        }
    }
}


/* TODO: [near miss] 99.89%; data-value exact; anonymous relocation identity remains. */
void mk_chess_ani_to_end(void) {
    mk_chess_ani_to_frame_x(g_active_piece->animation->end_frame);
}


/* TODO: [near miss] 99.89%; data-value exact; anonymous relocation identity remains. */
void mk_chess_ani_to_blend_frame(float blend_frames) {
    mk_chess_ani_to_frame_x(
        g_active_piece->animation->end_frame - blend_frames);
}

void mk_chess_random_specials_positive_reaction(void) {
    unsigned int occurrence;
    for (occurrence = 0; occurrence < 2; occurrence++) {
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side == 0, 1, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side == 0, 3, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side == 0, 4, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side == 0, 2, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side, 1, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side, 3, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side, 4, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                g_active_piece->side, 2, occurrence, 0x1D);
        }
    }
}

void mk_chess_piece_type_to_piece_script(
    unsigned int side_index,
    int piece_type,
    unsigned int occurrence,
    unsigned int script) {
    ChessPiece* piece = mk_chess_find_piece_of_type(side_index, piece_type, occurrence);

    if (piece != 0 && piece->state == 0) {
        mk_chess_xfer_to_piece_script(piece, script);
    }
}

void mk_chess_deactivate_all_special_cells(int hidden) {
    unsigned int x;
    unsigned int y;

    for (x = 0; x < MK_CHESS_BOARD_COLUMNS; x++) {
        for (y = 0; y < MK_CHESS_BOARD_COLUMNS; y++) {
            mk_chess_cell_hide(x, y, hidden);
        }
    }
}


/* TODO: [near miss] 99.52%; data-value exact; anonymous relocation identity remains. */
static float p_mk_chess_piece_proc_entry(void) {
    g_active_piece->state = 0;
    g_active_piece->flags.snap_into_stance = 0;
    g_active_piece->flags.glitch_into_stance = 0;
    g_active_piece->object->flags_08_bits.gravity_enabled = 0;
    g_active_piece->movement->desired_cell_blend = 30.0f;
    if (g_active_piece->flags.dont_constrain) {
        g_active_piece->flags.dont_constrain = 0;
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_piece_proc, 0.0f);
        return 0.0f;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_piece_constrain_to_cell, 0.0f);
    return 0.0f;
}

static inline float mk_chess_direction_input(
    unsigned int direction,
    int action,
    int spell_input,
    int diagonal) {
    ChessInputPdata* input = (ChessInputPdata*)apdata;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)
            ->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }

    switch (mk_chess_pdata->manager.input_state) {
    case 10:
        mk_chess_pdata->manager.directional_state->actions[input->side] = action;
        break;
    case 9:
        mk_chess_pdata->manager.spell->input_state = spell_input;
        break;
    case 0:
        if (diagonal) {
            mk_chess_move_cursor_to_next_diagnal_piece(mk_chess_pdata->manager.active_side, direction);
        } else {
            mk_chess_move_cursor_to_next_piece(mk_chess_pdata->manager.active_side, direction);
        }
        break;
    case 1:
        mk_chess_move_cursor_to_next_square_track_line(mk_chess_pdata->manager.active_side, direction);
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}


/* TODO: [near miss] 98.85%; same control flow and accesses; mode/input pointer coloring remains. */
float x_chess_down(void) {
    return mk_chess_direction_input(0, 5, 2, 0);
}


/* TODO: [near miss] 98.85%; same control flow and accesses; mode/input pointer coloring remains. */
float x_chess_up(void) {
    return mk_chess_direction_input(4, 4, 1, 0);
}


/* TODO: [near miss] 98.85%; same control flow and accesses; mode/input pointer coloring remains. */
float x_chess_right(void) {
    return mk_chess_direction_input(2, 7, 4, 0);
}


/* TODO: [near miss] 98.85%; same control flow and accesses; mode/input pointer coloring remains. */
float x_chess_left(void) {
    return mk_chess_direction_input(6, 6, 3, 0);
}


/* TODO: [near miss] 98.54%; correct targets; diagonal case emission order and pointer coloring remain. */
float x_chess_right_and_down(void) {
    return mk_chess_direction_input(1, 9, 6, 1);
}


/* TODO: [near miss] 98.54%; correct targets; diagonal case emission order and pointer coloring remain. */
float x_chess_right_and_up(void) {
    return mk_chess_direction_input(3, 8, 5, 1);
}


/* TODO: [near miss] 98.54%; correct targets; diagonal case emission order and pointer coloring remain. */
float x_chess_left_and_up(void) {
    return mk_chess_direction_input(5, 10, 7, 1);
}


/* TODO: [near miss] 98.54%; correct targets; diagonal case emission order and pointer coloring remain. */
float x_chess_left_and_down(void) {
    return mk_chess_direction_input(7, 11, 8, 1);
}


/* TODO: [breakthrough needed] 93.65%; resolve retail beq 0x12ff8 and its surrounding ownership/CFG before further tuning. */
float p_board_monitor(void) {
    int x;
    int y;

    if (mk_chess_pdata->manager.input_state == 0xB) {
        for (x = 0; x < MK_CHESS_BOARD_COLUMNS; x++) {
            for (y = 0; y < MK_CHESS_BOARD_COLUMNS; y++) {
                mk_chess_cell_monitor(x, y);
            }
        }
    }
    return 1.0f;
}


/* TODO: [breakthrough needed] 99.50%; resolve retail fadds f31, f1, f0 and its surrounding ownership/CFG before further tuning. */
float p_mk_chess_attack_burst(void) {
    ChessAttackBurstPdata* burst;
    float amount;

    burst = (ChessAttackBurstPdata*)apdata;
    bgnd_pebble_set_current_pebble(
        burst->pebble_group + 2, burst->pebble);
    amount = bgnd_pebble_fetch_current_info(0xD) + burst->amount;
    if (amount > burst->maximum) {
        amount = burst->maximum;
    }
    bgnd_pebble_set_current_info(0xD, amount);
    burst->amount += burst->acceleration;
    if (amount >= burst->maximum) {
        return -1.0f;
    }
    return 1.0f;
}


/* TODO: [near miss] 97.60%; same pebble updates and returns; process/index register coloring remains. */
float p_mk_chess_show_my_side(void) {
    ChessShowSidePdata* display;
    unsigned int pebble;

    display = (ChessShowSidePdata*)apdata;
    display->current += display->amount;
    if (display->current >= display->maximum) {
        display->current = display->maximum;
    }
    display->amount += display->acceleration;
    for (pebble = 0; pebble < display->side->live_piece_count; pebble++) {
        bgnd_pebble_set_current_pebble(display->pebble_group, pebble);
        bgnd_pebble_set_current_info(0xE, display->current);
        bgnd_pebble_set_current_info(0xC, display->current);
    }
    if (display->current >= display->maximum) {
        return -1.0f;
    }
    return 1.0f;
}


/* TODO: [breakthrough needed] 93.48%; correct StringObj visibility; validated-pointer latch CFG still differs. */
float p_mk_chess_blink_string(void) {
    ChessBlinkStringPdata* blink = (ChessBlinkStringPdata*)apdata;
    StringObj* text = blink->text;

    if (text != 0) {
        if (text->instance != blink->text_instance) {
            text = 0;
        }
    } else {
        text = 0;
    }
    if (--blink->timer == 0) {
        text->visibility.hidden = 0;
    } else if (blink->timer == -blink->interval) {
        blink->timer = blink->interval;
        text->visibility.hidden = 1;
    }
    return 1.0f;
}


/* TODO: [breakthrough needed] 98.90%; resolve retail li r29, 0x4af and its surrounding ownership/CFG before further tuning. */
void mk_chess_wait_until_attack_cam_closes_in(void) {
    int timeout = 0x4AF;

    do {
        ChessAnimPdata* animation = g_active_piece->animation;
        advance_anim((AnimPdata*)animation);
        pose_anim((AnimPdata*)animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    } while (!(g_bezier_cam.progress > 0.8f) && timeout-- > 0);
}


static float p_gnd_blast(void);

/* TODO: [near miss] 84.93%; initialization order agrees; allocator-output reloads and board-address coloring remain. */
static void start_gnd_blast(ChessPiece* piece, Vec* scale, int spawn_secondary,
                            float initial_scale) {
    MkHdr* allocation;
    ChessGroundBlastPdata* pdata;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    MkObj* object = load_named_model_from_slot(0xD003C, "GND_BLAST", 0xC01E, 0);
    MkSobj* sobj;

    if (object != 0 &&
        _create_mkproc_generic_nostack(0xC023, 0x20, p_gnd_blast,
                                      sizeof(ChessGroundBlastPdata), &allocation) != 0) {
        zero_pdata_payload(sizeof(ChessGroundBlastPdata), allocation);
        obj_create_sobjs(object);
        sobj = obj_first_sobj(object);
        object->light_flags = 0xD;
        sobj_set_priority(sobj, 9);
        sobj->flags09_bits.bit6 = 1;
        sobj->flags09_bits.bit7 = 1;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        object->light_flags = 0;
        pdata = (ChessGroundBlastPdata*)allocation;
        pdata->piece = piece;
        pdata->object = object;
        if (spawn_secondary != 0) {
            pdata->maximum_scale = 6.0f;
            pdata->fade_start_scale = 2.5f;
            pdata->scale_per_tick = 0.2f;
        } else {
            pdata->maximum_scale = 4.0f;
            pdata->fade_start_scale = 2.5f;
            pdata->scale_per_tick = 0.2f;
        }
        pdata->alpha_step = 255 / (int)((pdata->maximum_scale -
            pdata->fade_start_scale) / pdata->scale_per_tick);
        pdata->fade_ticks = 0;
        pdata->spawn_secondary = spawn_secondary;
        pdata->elapsed = 1.0f + initial_scale / pdata->scale_per_tick;
        object->pos.value.x = cell->position.x;
        object->pos.value.y = cell->position.y;
        object->pos.value.z = cell->position.z;
        object->pos.value.y = 0.0001f;
        object->flags_08_bits.scale_active = 1;
        object->scale.x = scale->x * initial_scale;
        object->scale.y = scale->y * initial_scale;
        object->scale.z = scale->z * initial_scale;
        update_obj_pos(object);
        insert_fgnd_mkobj(object);
        unhide_obj(object);
    }
}

/* TODO: [near miss] 99.38%; data-value exact; anonymous relocation identity remains. */
static float p_gnd_blast(void) {
    RwRGBA color = {255, 255, 255, 255};
    ChessGroundBlastPdata* pdata = (ChessGroundBlastPdata*)apdata;
    float scale = pdata->elapsed * pdata->scale_per_tick;
    MkObj* object;

    if (scale > pdata->maximum_scale) {
        object = pdata->object;
        if (object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        return -1.0f;
    }
    if (scale >= pdata->fade_start_scale) {
        if (pdata->spawn_secondary != 0) {
            Vec initial_scale = {1.0f, 1.0f, 1.0f};
            start_gnd_blast(pdata->piece, &initial_scale, 0, 1.0f);
            pdata->spawn_secondary = 0;
        }
        color.alpha = 255 - pdata->fade_ticks * pdata->alpha_step;
        obj_set_color_for_all_materials(pdata->object, &color);
        pdata->fade_ticks++;
    }
    object = pdata->object;
    object->scale.x = scale;
    object->scale.z = scale;
    pdata->elapsed += game_speed;
    return 1.0f;
}

static inline MkObj* mk_chess_live_imprison_object(ChessPieceEffect* effect) {
    MkObj* object = effect->object;
    if (object != 0) {
        if (object->hdr.instance == effect->object_instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static float p_mk_chess_imprison(void);

static inline MkObj* mk_chess_live_team_model(ChessSideState* side) {
    MkObj* object = side->team_model;
    if (object != 0) {
        if (object->hdr.instance == side->team_model_instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

/* TODO: [near miss] 95.90%; equivalent side guard, allocator-output reload and coloring remain. */
static void start_imprison_effect(ChessPiece* piece, unsigned int duration) {
    MkHdr* allocation;
    ChessImprisonPdata* pdata;
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    ChessPieceEffect* effect;
    MkObj* object;

    if (piece != 0) {
        if (piece->side >= 2) {
            return;
        }
        effect = (ChessPieceEffect*)get_mkpdata_generic(sizeof(ChessPieceEffect));
        if (effect != 0) {
            object = mk_chess_live_team_model(mk_chess_pdata->sides[piece->side]);
            if (object == 0) {
                if (effect->hdr.instance != 0) {
                    effect->hdr.typed_vtbl->destroy(&effect->hdr);
                }
            } else {
                effect->object = 0;
                effect->object_instance = 0;
                effect->object = object;
                effect->object_instance = object->hdr.instance;
                effect->kind = 2;
                effect->expiry_clock = (unsigned int)manager->clock + duration;
                effect->flags.word = 0;
                effect->flags.bits.bit6 = 1;
                object->flags_08_bits.scale_active = 1;
                object->scale.x = 1.0f;
                object->scale.y = 0.0f;
                object->scale.z = 1.0f;
                effect->sound = 0;
                effect->sound_instance = 0;
                object->flags_08_bits.airborne = 1;
                object->pos.value.x = cell->position.x;
                object->pos.value.y = cell->position.y;
                object->pos.value.z = cell->position.z;
                update_mkobj(object);
                if (is_load_meter_active() == 0) {
                    snd_req(0x390);
                }
                object->pos.value.y = 0.0f;
                unhide_obj(object);
                mk_insert(&effect->hdr, &piece->effects);
                _create_mkproc_generic_tinystack(0xC01E, 0x1F, p_mk_chess_imprison,
                    sizeof(ChessImprisonPdata), &allocation);
                pdata = (ChessImprisonPdata*)allocation;
                pdata->effect = effect;
                pdata->piece = piece;
            }
        }
    }
}

/* TODO: [near miss] 99.56044%; data-value exact; anonymous float relocation identity remains. */
static float p_mk_chess_imprison(void) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessImprisonPdata* pdata = (ChessImprisonPdata*)apdata;
    ChessPieceEffect* effect = pdata->effect;
    MkObj* object = mk_chess_live_imprison_object(effect);

    if (object == 0) {
        if (effect->hdr.instance != 0) {
            effect->hdr.typed_vtbl->destroy(&effect->hdr);
        }
        return -1.0f;
    }
    if (effect->expiry_clock < (unsigned int)manager->clock) {
        if (object->scale.y == 1.5f && is_load_meter_active() == 0) {
            snd_req(0x390);
        }
        if (object->scale.y > 0.0f) {
            object->scale.y -= 0.1f;
            if (object->scale.y <= 0.0f) {
                hide_obj(object);
                mk_pull_destroy(&pdata->effect->hdr, &pdata->piece->effects);
                effect = pdata->effect;
                if (effect->hdr.instance != 0) {
                    effect->hdr.typed_vtbl->destroy(&effect->hdr);
                }
                return -1.0f;
            }
        }
    } else if (object->scale.y < 1.5f) {
        object->scale.y += 0.1f;
        if (object->scale.y > 1.5f) {
            object->scale.y = 1.5f;
        }
    }
    return 1.0f;
}

/* TODO: [near miss] 99.75%; data-value exact; anonymous relocation identity remains. */
static float p_mk_chess_turn_expired(void) {
    ChessTurnExpiredPdata* data = (ChessTurnExpiredPdata*)pdata_of_proc(aproc);
    int state = mk_chess_pdata->input_transition_busy;
    int input = mk_chess_pdata->manager.input_state;
    ChessSideState* side = mk_chess_pdata->sides[data->side];

    switch (state) {
    case 1:
        switch (input) {
        case 0:
            mk_chess_end_of_turn();
            return -1.0f;
        case 1:
            xfer_proc(side->input_proc, x_chess_4);
            mk_chess_pdata->input_transition_busy = 3;
            break;
        case 9:
            xfer_proc(side->input_proc, x_chess_4);
            mk_chess_pdata->input_transition_busy = 3;
            break;
        }
        break;
    case 2:
        break;
    case 3:
        if ((unsigned int)input == 0) {
            mk_chess_end_of_turn();
            return -1.0f;
        }
        break;
    }
    return 1.0f;
}

/* TODO: [near miss] 93.29%; seven-integer create ABI recovered; stop at five attempts with address scheduling/coloring residue. */
void mk_chess_init_piece(int slot, int character) {
    unsigned int x;
    unsigned int y;

    x = g_starting_cells[slot].x;
    y = g_starting_cells[slot].y;
    mk_chess_create_piece(
        g_active_team, slot, slot, x,
        y + g_active_team * (10 - (y * 2 + 1)),
        character, 1, 1.0f);
    mk_chess_pdata->sides[g_active_team]->live_piece_count++;
}

void mk_chess_load_chess_table(ChessTableHeader* table) {
    g_game_info.mode_table = table;
}

void mk_chess_init_bgnd_for_fight_mode(void) {
    ChessTableHeader* table;

    table = (ChessTableHeader*)g_game_info.mode_table;
    if (table != 0 && table->init_function != 0) {
        cmdscript_setup_execution(
            g_game_info.cmdscript, table->init_function);
        cmdscript_execute(g_game_info.cmdscript);
    }
}

/* TODO: [breakthrough needed] 77.98%; resolve retail lwz r31, camera_item@sda21 and its surrounding ownership/CFG before further tuning. */
void mk_chess_camera_init_for_place_traps(void) {
    CameraObj* camera;
    Vec target;

    mk_chess_camera_init();
    camera = camera_item.node;
    if (camera != 0 && camera->hdr.instance != camera_item.instance) {
        camera = 0;
    }
    camera->pos.x = -38.45f;
    camera->pos.y = 24.8f;
    camera->pos.z = 0.0f;
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 0.0f;
    look_at_target(&target);
    update_mkobj((MkHdr*)camera);
    mk_chess_pdata->camera.viewing_quadrant = 3;
}


/* TODO: [near miss] 94.04%; float completion return recovered; latch lowering and register scheduling remain. */
float p_mk_chess_apply_force(void) {
    ChessForcePdata* force;
    MkObj* object;
    int frame;

    force = (ChessForcePdata*)apdata;
    object = force->object;
    if (object != 0 && object->hdr.instance != force->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    _mkproc_sleep_ticks = force->delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if (force->frames > 0 && force->frames < 60) {
        for (frame = 0; frame < force->frames; frame++) {
            object->pos_vel.x *= force->damping;
            object->pos_vel.z *= force->damping;
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        }
    }
    object->pos_vel.x = 0.0f;
    object->pos_vel.z = 0.0f;
    return -1.0f;
}

int mk_chess_fetch_single_space_attack_script(
    ChessAttackGridPosition* position,
    unsigned int direction) {
    int outside_inner_board;

    if (position->cell_x <= 1 || position->cell_x >= 8 ||
        position->cell_y <= 1 || position->cell_y >= 8) {
        outside_inner_board = 1;
    } else {
        outside_inner_board = 0;
    }
    if (outside_inner_board != 0) {
        if (direction == 4) {
            return 0x23;
        }
        if (direction == 0) {
            return 0x25;
        }
        if (direction == 6) {
            return 0x27;
        }
        if (direction == 2) {
            return 0x29;
        }
        if (direction == 3 || direction == 5) {
            return 0x14;
        }
        return 0x17;
    }
    if (direction == 4) {
        return 0x0B;
    }
    if (direction == 0) {
        return 0x0D;
    }
    if (direction == 6) {
        return 0x0F;
    }
    if (direction == 2) {
        return 0x11;
    }
    if (direction == 3 || direction == 5) {
        return 0x14;
    }
    return 0x17;
}

int mk_chess_fetch_bp_num_based_on_pchr_num(unsigned int character_number) {
    unsigned int index;

    for (index = 0; index < 26; index++) {
        if (g_board_game_controller.battle_piece_rows[index].character_number ==
            character_number) {
            return index;
        }
    }
    return 0;
}

int mk_chess_fetch_active_defined_teams_class(int class_slot) {
    int team = g_active_team;

    switch (class_slot) {
    case 5:
        return g_chess_definition_info[team].class_slot_5;
    case 2:
        return g_chess_definition_info[team].class_slot_2;
    case 3:
    case 4:
        return g_chess_definition_info[team].class_slot_3_4;
    case 1:
        return g_chess_definition_info[team].class_slot_1;
    default:
        return g_chess_definition_info[team + 1].next_record_fallback_class;
    }
}

int mk_chess_fetch_active_defined_team(void) {
    return g_active_team;
}

void mk_chess_make_spellcaster(const ChessSpellDefinition* definitions) {
    ChessClassDefinition* definition = g_active_class_definition;

    if (definition == 0) {
        return;
    }
    definition->flags.spellcaster = 1;
    g_active_class_definition->spell_definitions = definitions;
}

void mk_chess_set_piece_event_script(unsigned int event,
                                     ChessPieceEventScript script) {
    ChessClassDefinition* definition = g_active_class_definition;

    if (definition == 0) {
        return;
    }
    if (event >= MK_CHESS_PIECE_EVENT_COUNT) {
        return;
    }
    definition->event_scripts[event] = script;
}

void mk_chess_add_movement_skill(int move_type, unsigned int limit_a,
                                 unsigned int limit_b, int flags) {
    if (g_active_class_definition == 0) {
        return;
    }
    if (g_active_class_definition->movement_skill_count >=
        MK_CHESS_MOVEMENT_SKILL_COUNT) {
        return;
    }

    g_active_class_definition->movement_skills[
        g_active_class_definition->movement_skill_count].move_type = move_type;
    g_active_class_definition->movement_skills[
        g_active_class_definition->movement_skill_count].limits =
        (limit_a << 16) | limit_b;
    g_active_class_definition->movement_skills[
        g_active_class_definition->movement_skill_count].flags = flags;
    g_active_class_definition->movement_skill_count++;
}

void mk_chess_define_class_initial_power(float power) {
    if (g_active_class_definition == 0) {
        return;
    }
    g_active_class_definition->initial_power = power;
}

void mk_chess_set_normal_stance_script(AniScript* script) {
    if (g_active_piece_being_defined == 0 && g_active_piece == 0) {
        return;
    }

    if (g_active_piece != 0) {
        g_active_piece->normal_stance_script = script;
        return;
    }

    g_active_piece_being_defined->normal_stance_script = script;
    g_active_piece_being_defined->initial_stance_script = script;
}


static int mk_chess_spell_hud_check_target_rules(
    ChessSpellState* spell, ChessCell* cell, unsigned int excluded_side,
    unsigned int x, unsigned int y);

static int mk_chess_check_spell_target_rules(
    ChessSpellState* spell, ChessCell* cell, unsigned int excluded_side,
    unsigned int x, unsigned int y);

/* TODO: [near miss] 99.88%; spell target categories recovered; local lowering and relocations remain. */
static int mk_chess_drone_validate_target(
    ChessDroneState* drone, ChessSpellState* hud, unsigned int x, unsigned int y,
    unsigned int target) {
    unsigned char category;
    if (target == 0) {
        hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
            .spell_definitions[hud->spell_number].target_rules[0];
    } else {
        hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
            .spell_definitions[hud->spell_number].target_rules[1];
    }
    category = hud->target_rules;
    if (target == 1 && drone->target_0_x == drone->target_1_x &&
        drone->target_0_y == drone->target_1_y) {
        return 0;
    }
    switch (category) {
    case 2: {
        ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
        if (piece == 0 || drone->piece->side == piece->side) {
            return 0;
        }
        break;
    }
    case 1: {
        ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
        if (piece == 0 || drone->piece->side != piece->side) {
            return 0;
        }
        break;
    }
    case 3:
        if (mk_chess_pdata->board[x].cells[y].piece == 0) {
            return 0;
        }
        break;
    case 7:
        if (mk_chess_pdata->board[x].cells[y].piece != 0) {
            return 0;
        }
        break;
    case 0:
        return 1;
    case 8:
        return drone->captured_piece_count != 0;
    case 16:
        if (mk_chess_pdata->board[x].cells[y].piece != 0) {
            return 0;
        }
        if (hud->side == 0 && y >= 2) {
            return 0;
        }
        if (hud->side == 1 && y < 7) {
            return 0;
        }
        return 1;
    }
    if (category != 8 &&
        mk_chess_check_spell_target_rules(hud, &mk_chess_pdata->board[x].cells[y], 2, x, y) != 0) {
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 99.86%; data-value exact; jump-table relocation identity remains. */
static int mk_chess_spell_hud_check_target_rules(
    ChessSpellState* spell, ChessCell* cell, unsigned int excluded_side,
    unsigned int x, unsigned int y) {
    unsigned int rules = spell->target_rules;
    int allowed = 0;
    unsigned int exclusions = rules & 0xFF00;
    ChessPiece* piece;

    switch (rules & 0xFF) {
    case 1:
        piece = cell->piece;
        if (piece != 0 && piece->side == spell->side) {
            allowed = 1;
        }
        break;
    case 2:
        piece = cell->piece;
        if (piece != 0 && piece->side != spell->side) {
            allowed = 1;
        }
        break;
    case 3:
        if (cell->piece != 0) {
            allowed = 1;
        }
        break;
    case 7:
        piece = cell->piece;
        allowed = 1;
        if (piece != 0 && piece->side == excluded_side) {
            allowed = 0;
        }
        break;
    case 16:
        allowed = 1;
        if (cell->piece != 0) {
            allowed = 0;
        } else if (spell->side == 0) {
            if (mk_chess_pdata->cursors[2].cell_y > 1) {
                allowed = 0;
            }
        } else if (spell->side == 1 && mk_chess_pdata->cursors[2].cell_y < 8) {
            allowed = 0;
        }
        break;
    }
    piece = cell->piece;
    if (piece != 0) {
        if ((exclusions & 0x200) && piece->type == 5) {
            allowed = 0;
        }
        if ((exclusions & 0x1000) && piece->type == 3) {
            allowed = 0;
        }
        if ((exclusions & 0x2000) && piece->type == 4) {
            allowed = 0;
        }
        if (exclusions == 0x3A00 && piece->type != 0 && piece->type != 1) {
            allowed = 0;
        }
        if (spell->state == 0x12 && x == (unsigned char)spell->target_x[0] &&
            y == (unsigned char)spell->target_y[0]) {
            allowed = 0;
        }
    }
    return allowed;
}

/* TODO: [near miss] 98.06%; traversal and alpha guards agree; stop at list/finished-flag coloring. */
static float p_mk_chess_spell_hud_string_fade(void) {
    int finished = 0;
    ChessStringFadePdata* pdata = (ChessStringFadePdata*)apdata;
    MkPtr* link;
    MkPtr* next;
    StringObj* string;
    int step;
    int alpha;

    if (pdata->strings != 0) {
        link = *pdata->strings;
        while (link != 0) {
            string = (StringObj*)link->hdr;
            if (link->instance != string->instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                step = pdata->step;
                alpha = string->pfx.instance0.native_color.a;
                if ((alpha < -step && step < 0) ||
                    (alpha > 255 - step && step > 0)) {
                    finished = 1;
                } else {
                    string->pfx.instance0.native_color.a = alpha + step;
                }
                link = link->next;
            }
        }
    }
    if (finished == 1) {
        if (pdata->step < 0 && pdata->strings != 0) {
            link = *pdata->strings;
            while (link != 0) {
                string = (StringObj*)link->hdr;
                if (link->instance != string->instance) {
                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else {
                    destroy_string_obj(string);
                    link = link->next;
                }
            }
        }
        if (pdata->completed != 0) {
            pdata->completed();
        }
        return -1.0f;
    }
    return 1.0f;
}

/* TODO: [near miss] 99.17%; data-value exact; anonymous relocation identity remains. */
float mk_chess_spell_hud_target_1_text_faded_exit_cb(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->target_1->state = 0x13;
    return 0.0f;
}


/* TODO: [near miss] 99.17%; data-value exact; anonymous relocation identity remains. */
float p_mk_chess_names_retracted_move_up_selected_spell(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->selected_spell->state = 9;
    return 0.0f;
}


/* TODO: [near miss] 99.64%; data-value exact; anonymous relocation identity remains. */
float mk_chess_spell_hud_spell_text_faded_exit_cb(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->target_1->state = 6;
    snd_req(0x396);
    return 0.0f;
}


/* TODO: [near miss] 98.48%; same operations and list effects; stop at pointer/finished-flag coloring. */
static float p_mk_chess_fade_images(void) {
    int finished = 0;
    ChessImageFadePdata* data = (ChessImageFadePdata*)apdata;

    if (data->images != 0) {
        MkPtr* image = *data->images;
        while (image != 0) {
            if (image->instance != image->hdr->instance) {
                MkPtr* next = image->next;
                image->hdr = 0;
                destroy_mkptr(image);
                image = next;
            } else {
                Pfx2dObj* effect = ((ScreenObj*)image->hdr)->pfx2d;
                if (effect->verts[0].a < 20) {
                    finished = 1;
                } else {
                    effect->verts[0].a -= 20;
                }
                image = image->next;
            }
        }
    }
    if (finished == 1) {
        if (data->images != 0) {
            MkPtr* image = *data->images;
            while (image != 0) {
                if (image->instance != image->hdr->instance) {
                    MkPtr* next = image->next;
                    image->hdr = 0;
                    destroy_mkptr(image);
                    image = next;
                } else {
                    hide_screen_obj((ScreenObj*)image->hdr);
                    image = image->next;
                }
            }
        }
        if (data->completed != 0) {
            data->completed();
        }
        return -1.0f;
    }
    return 1.0f;
}

/* TODO: [near miss] 99.64%; data-value exact; anonymous relocation identity remains. */
float p_mk_chess_completed_all_image_retraction_for_targetting(void) {
    ChessImageFadePdata* pdata = (ChessImageFadePdata*)apdata;

    pdata->owner->state = 0xF;
    snd_req(0x396);
    return 0.0f;
}


/* TODO: [near miss] 99.64%; data-value exact; anonymous relocation identity remains. */
float p_mk_chess_completed_all_image_retraction(void) {
    ChessImageFadePdata* pdata = (ChessImageFadePdata*)apdata;

    pdata->owner->state = 6;
    snd_req(0x396);
    return 0.0f;
}


/* TODO: [near miss] 99.13043%; data-value exact; anonymous relocation identity remains. */
float mk_chess_attacker_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 0.1f);
    return 0.0f;
}


/* TODO: [near miss] 99.13043%; data-value exact; anonymous relocation identity remains. */
float mk_chess_attacker_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 0.1f);
    return 0.0f;
}


/* TODO: [near miss] 99.13%; data-value exact; anonymous relocation identity remains. */
float mk_chess_pwr_cell_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 0.25f);
    return 0.0f;
}


/* TODO: [near miss] 99.13%; data-value exact; anonymous relocation identity remains. */
float mk_chess_pwr_cell_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 0.25f);
    return 0.0f;
}


/* TODO: [near miss] 99.13%; data-value exact; anonymous relocation identity remains. */
float mk_chess_on_pwr_cell_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 1.0f);
    return 0.0f;
}


/* TODO: [near miss] 99.13%; data-value exact; anonymous relocation identity remains. */
float mk_chess_on_pwr_cell_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 1.0f);
    return 0.0f;
}

void mk_chess_spell_has_completed_but_wait_for_fight(void) {
    mk_chess_pdata->manager.spell->state = MK_CHESS_SPELL_WAITING_FOR_FIGHT;
}

void mk_chess_spell_has_completed(void) {
    mk_chess_pdata->spell_completion_clock = mk_chess_pdata->manager.clock;
    if (mk_chess_pdata->manager.spell->state !=
        MK_CHESS_SPELL_WAITING_FOR_COMPLETION) {
        return;
    }
    mk_chess_pdata->manager.spell->state = MK_CHESS_SPELL_COMPLETE;
}

void mk_chess_spell_rescue_current_target(void) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    ChessSideState* side = mk_chess_pdata->sides[spell->side];
    unsigned int x = spell->target_x[1];
    unsigned int y = spell->target_y[1];
    unsigned int index;

    for (index = 17 - side->captured_piece_count; index < 17; index++) {
        if (side->pieces[index]->type == spell->rescue_piece_type) {
            mk_chess_move_piece_from_deadpool_to(spell->side, index, x, y);
            spell->target_x[0] = x;
            spell->target_y[0] = y;
            return;
        }
    }
}

/* TODO: [breakthrough needed] 77.24%; resolve retail mulli r3, r3, 0x208 and its surrounding ownership/CFG before further tuning. */
void mk_chess_spell_target_add_access_restrictions(unsigned int target,
                                                   unsigned int restriction,
                                                   int duration, int reset) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
    unsigned int* expires;

    if (piece == 0) {
        expires = 0;
    } else if (restriction < 6) {
        expires = &piece->access_restrictions[restriction];
    } else {
        expires = &piece->access_restrictions[0];
    }

    if ((*expires > (unsigned int)mk_chess_pdata->manager.clock) && (reset == 0)) {
        *expires += duration;
        return;
    }
    *expires = mk_chess_pdata->manager.clock + duration;
}

/* TODO: [breakthrough needed] 64.12%; resolve retail lwz r8, 0x38(r4) and its surrounding ownership/CFG before further tuning. */
void mk_chess_spell_force_fight(void) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[0];
    unsigned int y = (unsigned char)spell->target_y[0];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    board_game_save_data.sides[spell->side].forced_fight_count++;
    mk_chess_request_piece_fight(
        piece, (unsigned char)spell->target_x[1],
        (unsigned char)spell->target_y[1], 1);
}


/* TODO: [breakthrough needed] 90.61%; resolve retail lwz r6, 0x38(r3) and its surrounding ownership/CFG before further tuning. */
int mk_chess_spell_is_this_a_forced_fight(void) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x0 = (unsigned char)spell->target_x[0];
    unsigned int y0 = (unsigned char)spell->target_y[0];
    unsigned int x1 = (unsigned char)spell->target_x[1];
    unsigned int y1 = (unsigned char)spell->target_y[1];
    ChessPiece* first = mk_chess_pdata->board[x0].cells[y0].piece;
    ChessPiece* second = mk_chess_pdata->board[x1].cells[y1].piece;

    if ((first != 0) && (second != 0) && (first->side != second->side)) {
        return 1;
    }
    return 0;
}

void mk_chess_spell_kill_target(unsigned int target);
void start_protect_effect(ChessPiece* piece, int duration);
static float p_mk_chess_fade_single_piece(void);
static void mk_chess_do_smoke_effect(void);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void bgnd_launch_fx_at_bid_of_mkobj(const char* name, MkObj* object, int bone);
static void start_magic_rings_effect(Vec* position, Vec* scale, const char* model, int mode,
    float initial_scale, float maximum_scale, float fade_start_scale, float scale_per_tick);
static void start_gnd_light_beam_effect(Vec* position, const char* model_name,
    int alpha_step, int reverse_scroll, float radial_limit, float radial_initial,
    float radial_step, float radial_start, float vertical_limit,
    float vertical_initial, float vertical_step, float vertical_start, float fade_start);

static inline void mk_chess_special_emitter(const char* name, Vec* position,
    const char* priority_owner, int priority)
{
    unsigned int emitter = fx_by_owner(name, 4);
    fx_set_param_v3(emitter, 0x202, position->x, position->y, position->z);
    fx_reset(emitter);
    fx_resume_emit(emitter);
    fx_set_render_priority(fx_by_owner(priority_owner, 4), priority);
}

static inline void mk_chess_special_fade(ChessPiece* piece, unsigned int alpha,
    unsigned int delay, int step)
{
    MkHdr* allocation = 0;
    if (_create_mkproc_generic_tinystack(0xC024, 31, p_mk_chess_fade_single_piece,
        sizeof(ChessPieceFadePdata), &allocation) != 0 && allocation != 0) {
        ChessPieceFadePdata* fade = (ChessPieceFadePdata*)allocation;
        fade->piece = piece;
        fade->target_alpha = alpha;
        fade->delay = delay;
        fade->step = step;
        fade->color.red = 255;
        fade->color.green = 255;
        fade->color.blue = 255;
        fade->color.alpha = 255;
        obj_set_color_for_all_materials(fade->piece->object, &fade->color);
    }
}

/* TODO: [breakthrough needed] 46.04%; call boundaries restored; fade allocation and dispatcher lowering remain; five-attempt limit. */
void mk_chess_launch_special_fx(unsigned int kind, unsigned int mode, unsigned int expiry)
{
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    ChessPiece* piece;
    ChessCell* cell;
    Vec scale = {1.0f, 1.0f, 1.0f};
    Vec position;
    MkProc* process;
    unsigned int emitter;
    MkPfx* particle;
    MkObj* object;
    if (mode == 2) piece = g_active_piece;
    else piece = mk_chess_pdata->board[(unsigned char)spell->target_x[mode]].cells[(unsigned char)spell->target_y[mode]].piece;
    if (piece != 0) cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    else cell = &mk_chess_pdata->board[spell->target_x[mode]].cells[spell->target_y[mode]];
    switch (kind) {
    case 16: mk_chess_do_smoke_effect(); return;
    case 17: fx_pause_emit(fx_by_owner("shifter_smoke", 4)); return;
    case 0: start_gnd_blast(piece, &scale, 1, 1.0f); return;
    case 1: start_protect_effect(piece, expiry); return;
    case 2: start_imprison_effect(piece, expiry); return;
    case 3:
        cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
        mk_chess_special_emitter("kill_flames", &cell->position, "kill_flames", 19);
        if ((unsigned short)randu0(100) < 2) snd_req_delay(0x373, 30);
        return;
    case 4:
        cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
        mk_chess_special_emitter("fat_kill_flames", &cell->position, "fat_kill_flames", 19);
        obj_set_bone_collapse_flag(piece->object, 1);
        obj_set_bone_collapse_flag(piece->object, 2);
        piece->object->pos_vel.z = 0.0f;
        piece->object->pos_vel.y = 0.0f;
        piece->object->pos_vel.x = 0.0f;
        piece->object->flags_08_bits.moving = 1;
        piece->object->flags_09_bits.launched = 1;
        piece->object->flags_08_bits.gravity_enabled = 1;
        piece->object->flags_09_bits.bit6 = 0;
        piece->object->pos_vel.y = -0.01f;
        piece->object->gravity = -0.01f;
        return;
    case 5: piece->object->flags_08_bits.gravity_enabled = 0; piece->object->flags_08_bits.moving = 0; return;
    case 6:
        fx_pause_emit(fx_by_owner("kill_flames", 4));
        obj_clear_bone_collapse_flag(piece->object, 1); obj_clear_bone_collapse_flag(piece->object, 2);
        mk_chess_spell_kill_target(0); return;
    case 7: fx_pause_emit(fx_by_owner("fat_kill_flames", 4)); return;
    case 8: start_magic_rings_effect(&piece->object->pos.value, &scale, "GND_BLAST", 120, 1.0f, 4.0f, 2.75f, 0.055f); return;
    case 19: mk_chess_special_fade(piece, 0, 1, -3); return;
    case 20:
        obj_set_bone_collapse_flag(piece->object, 10);
        get_bone_world_pos(g_active_piece->object, 10, &position);
        position.y = 0.0f;
        mk_chess_special_emitter("meat_squash", &position, "meat_squash", 19);
        mk_chess_special_emitter("blood_squash", &position, "blood_squash", 19); return;
    case 22:
        get_bone_world_pos(g_active_piece->object, 6, &position); position.x -= 0.8f;
        mk_chess_special_emitter("blood_puddle_1", &position, "blood_puddle_1", 12); return;
    case 23:
        get_bone_world_pos(g_active_piece->object, 6, &position); position.x -= 0.8f;
        mk_chess_special_emitter("blood_spurt", &position, "blood_puddle_1", 16); return;
    case 29:
        piece = mk_chess_pdata->board[(unsigned char)spell->target_x[1]].cells[(unsigned char)spell->target_y[1]].piece;
        emitter = fx_by_owner("sc_life", 4);
        get_bone_world_pos(piece->object, 6, &position);
        if (emitter != 0) {
            fx_reset_emit(emitter);
            particle = pfx_from_emitter(emitter);
            if (particle != 0) {
                object = pfx_bind_emitter_num_to_new_obj(particle, 0x8227, emitter_id_from_handle(emitter));
                if (object != 0) {
                    object->flags_08_bits.airborne = 1;
                    object->pos.value = position;
                    update_mkobj(object);
                    fx_resume_emit(emitter);
                }
            }
        }
        snd_req(0x3A0); return;
    case 21:
        obj_clear_bone_collapse_flag(piece->object, 12); obj_clear_bone_collapse_flag(piece->object, 13);
        obj_clear_bone_collapse_flag(piece->object, 10); return;
    case 9:
        position = cell->position; position.y = 0.0f;
        start_gnd_light_beam_effect(&position, "RESURRECT_LIGHT", 5, 0, 1.25f, .1f, .03f, 30.0f, 10.0f, 0.0f, .01f, 1.0f, 135.0f);
        mk_chess_special_fade(piece, 255, 70, 2); return;
    case 10: start_magic_rings_effect(&cell->position, &scale, "GND_BLAST", 120, 1.0f, 4.0f, 2.75f, .055f); return;
    case 28: start_magic_rings_effect(&cell->position, &scale, "RES_BLAST", 120, 1.0f, 4.0f, 2.75f, .055f); return;
    case 18: snd_req(0x39E); return;
    case 11:
        position = cell->position; position.y = 0.0f;
        start_gnd_light_beam_effect(&position, "TELEPORT_SHARDS", 2, 0, 1.5f, .5f, .03f, 1.0f, .75f, .1f, .03f, 1.0f, 120.0f); return;
    case 14:
        position = cell->position; position.y = 0.0f;
        start_gnd_light_beam_effect(&position, "HEALTH_SHARDS", 2, 0, 1.0f, .5f, .5f, 1.0f, 1.75f, .1f, .03f, 1.0f, 110.0f); return;
    case 15:
        position = cell->position; position.y = 0.0f;
        start_gnd_light_beam_effect(&position, "HEALTH_DECAL", 2, 1, 1.5f, 1.5f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 90.0f); return;
    case 12:
        process = find_mkproc_pid(0xC024);
        if (process != 0 && process->instance != 0) ((MkHdr*)process)->typed_vtbl->destroy((MkHdr*)process);
        mk_chess_special_fade(piece, 0, expiry, -2);
        return;
    case 13:
        process = find_mkproc_pid(0xC024);
        if (process != 0 && process->instance != 0) ((MkHdr*)process)->typed_vtbl->destroy((MkHdr*)process);
        mk_chess_special_fade(piece, 255, expiry, 2);
        return;
    case 30:
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_1", g_active_piece->object, 12);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_2", g_active_piece->object, 13);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_3", g_active_piece->object, 14);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_4", g_active_piece->object, 15);
        fx_set_render_priority(fx_by_owner("limbburn_1", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_2", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_3", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_4", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_5", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_6", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_7", 4), 19);
        fx_set_render_priority(fx_by_owner("limbburn_8", 4), 19);
        return;
    case 31:
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_1", g_active_piece->object, 12);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_2", g_active_piece->object, 13);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_3", g_active_piece->object, 14);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_4", g_active_piece->object, 15);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_5", g_active_piece->object, 3);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_6", g_active_piece->object, 10);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_7", g_active_piece->object, 7);
        bgnd_launch_fx_at_bid_of_mkobj("limbburn_8", g_active_piece->object, 8);
        return;
    case 32:
        fx_pause_emit(fx_by_owner("limbburn_1", 4));
        fx_pause_emit(fx_by_owner("limbburn_2", 4));
        fx_pause_emit(fx_by_owner("limbburn_3", 4));
        fx_pause_emit(fx_by_owner("limbburn_4", 4));
        fx_pause_emit(fx_by_owner("limbburn_5", 4));
        fx_pause_emit(fx_by_owner("limbburn_6", 4));
        fx_pause_emit(fx_by_owner("limbburn_7", 4));
        fx_pause_emit(fx_by_owner("limbburn_8", 4));
        return;
    case 24:
        bgnd_launch_fx_at_bid_of_mkobj("sorcerer_big_glow_1", g_active_piece->object, 14);
        bgnd_launch_fx_at_bid_of_mkobj("sorcerer_big_glow_2", g_active_piece->object, 15);
        fx_set_render_priority(fx_by_owner("sorcerer_big_glow_1", 4), 18);
        fx_set_render_priority(fx_by_owner("sorcerer_big_glow_2", 4), 18);
        return;
    case 25:
        fx_pause_emit(fx_by_owner("sorcerer_big_glow_1", 4));
        fx_pause_emit(fx_by_owner("sorcerer_big_glow_2", 4));
        return;
    case 26:
        bgnd_launch_fx_at_bid_of_mkobj("sorcerer_big_glow_1_2", g_active_piece->object, 14);
        bgnd_launch_fx_at_bid_of_mkobj("sorcerer_big_glow_2_2", g_active_piece->object, 15);
        fx_set_render_priority(fx_by_owner("sorcerer_big_glow_1_2", 4), 18);
        fx_set_render_priority(fx_by_owner("sorcerer_big_glow_2_2", 4), 18);
        return;
    case 27:
        fx_pause_emit(fx_by_owner("sorcerer_big_glow_1_2", 4));
        fx_pause_emit(fx_by_owner("sorcerer_big_glow_2_2", 4));
        return;
    }
}

void mk_chess_spell_kill_target(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;

    mk_chess_remove_piece_at_cell_into_deadpool(
        (unsigned char)spell->target_x[target],
        (unsigned char)spell->target_y[target]);
}


/* TODO: [near miss] 95.87%; same operations and stores; target/piece register coloring remains. */
void mk_chess_spell_move_target_from_temp_area_to(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    ChessPiece* piece = spell->temporary_piece;
    unsigned int y = spell->target_y[target];
    unsigned int x = spell->target_x[target];
    ChessCell* old_cell =
        &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    ChessCell* cell =
        &mk_chess_pdata->board[(unsigned char)x].cells[(unsigned char)y];

    if (old_cell->piece == piece) {
        old_cell->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    piece->object->hide_flag_bits.pin_animation = 0;
    piece->object->pos.value.x =
        cell->position.x + piece->runtime.fields.cell_offset.x;
    piece->object->pos.value.z =
        cell->position.z + piece->runtime.fields.cell_offset.z;
    update_obj_pos(piece->object);

    piece = spell->temporary_piece;
    if (piece->side == mk_chess_pdata->manager.active_side) {
        mk_chess_pdata->manager.active_piece_by_side[piece->side] = piece;
    }
}


/* TODO: [near miss] 86.35%; typed target address grouping and pointer coloring remain. */
void mk_chess_spell_move_target_to_temp_area(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    mk_chess_remove_piece_from_board(piece);
    spell->temporary_piece = piece;
}


/* TODO: [near miss] 94.52%; retail reload order restored; typed target and board address grouping remains. */
void mk_chess_spell_move_target_to_target(unsigned int source_target,
                                          unsigned int destination_target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int source_x =
        (unsigned char)spell->target_x[source_target];
    unsigned int source_y =
        (unsigned char)spell->target_y[source_target];
    ChessPiece* piece =
        mk_chess_pdata->board[source_x].cells[source_y].piece;
    unsigned int x;
    unsigned int y;
    ChessCell* old_cell;
    ChessCell* cell;

    mk_chess_remove_piece_from_board(piece);
    x = (unsigned char)spell->target_x[destination_target];
    y = (unsigned char)spell->target_y[destination_target];
    old_cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    cell = &mk_chess_pdata->board[x].cells[y];

    if (old_cell->piece == piece) {
        old_cell->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    piece->object->hide_flag_bits.pin_animation = 0;
    piece->object->pos.value.x =
        cell->position.x + piece->runtime.fields.cell_offset.x;
    piece->object->pos.value.z =
        cell->position.z + piece->runtime.fields.cell_offset.z;
    update_obj_pos(piece->object);

    if (piece->side == mk_chess_pdata->manager.active_side) {
        mk_chess_pdata->manager.active_piece_by_side[piece->side] = piece;
    }
}

/* TODO: [near miss] 79.38%; typed target/board address grouping differs; do not form out-of-bounds struct views. */
float mk_chess_spell_get_target_health(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];

    return mk_chess_pdata->board[x].cells[y].piece->health;
}

void resolve_alternate_palettes(PlyrInfo* player);
void mk_chess_count_p1_power_squares(unsigned int x, unsigned int y);
void mk_chess_count_p2_power_squares(unsigned int x, unsigned int y);
void bgnd_hide_pebbles(int player);
void bgnd_unhide_pebbles(int group);
static void mk_chess_cell_end_of_turn(unsigned int x, unsigned int y);
int mk_chess_handle_buffered_events_cb(ChessPiece* piece);

static inline void mk_chess_begin_hud_slide_out(ChessSideHudState* hud)
{
    if (hud->selected_piece != 0) {
        hud->saved_piece = hud->selected_piece;
        hud->selected_piece = 0;
        hud->flags |= 1;
        hud->flags |= 2;
    } else if (hud->flags & 2) {
        hud->flags |= 1;
    }
}

static inline void mk_chess_wait_for_active_piece(ChessPiece* piece)
{
    unsigned int timeout = 599;
    if (piece != 0) {
        do {
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        } while (piece->state != 0 && timeout-- != 0);
    }
}

/* TODO: [breakthrough needed] 84.44628%; retail calls restored; branch merging and owner scheduling remain; five attempts exhausted. */
static void mk_chess_end_of_turn(void)
{
    unsigned int next_side = 1;
    unsigned int side, index;
    int x, y;
    int buffered = 0;
    ChessSideState* team;
    ChessPiece* leader;
    ChessPiece* piece;
    ChessCell* cell;
    unsigned int active_side;
    int group;
    MkHdr* allocation = 0;
    ChessShowSidePdata* display;
    if (mk_chess_pdata->manager.active_side == 1) next_side = 0;
    if (board_game_save_data.input_flags.input_locked &&
        (unsigned int)mk_chess_pdata->manager.clock >= 20)
        gamelogic_jump(0, p_atm_loop);
    mk_chess_set_game_mode(11);
    mk_chess_begin_hud_slide_out(mk_chess_pdata->sides[0]->hud);
    mk_chess_begin_hud_slide_out(mk_chess_pdata->sides[1]->hud);
    destroy_mkprocs_pid(0xC021);
    bgnd_hide_pebbles(16);
    bgnd_hide_pebbles(17);
    for (x = 0; x < 10; x++)
        for (y = 0; y < 10; y++) mk_chess_cell_end_of_turn(x, y);
    mk_chess_wait_for_active_piece(mk_chess_pdata->manager.active_piece_by_side[0]);
    mk_chess_wait_for_active_piece(mk_chess_pdata->manager.active_piece_by_side[1]);
    for (side = 0; side < 2; side++) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++)
            mk_chess_handle_buffered_events_cb(mk_chess_pdata->sides[side]->pieces[index]);
    }
    for (side = 0; side < 2; side++) {
        unsigned char captured_index = 17 - mk_chess_pdata->sides[side]->captured_piece_count;
        for (; captured_index < 17; captured_index++) {
            if (mk_chess_handle_buffered_events_cb(mk_chess_pdata->sides[side]->pieces[captured_index]) != 0)
                buffered = 1;
        }
    }
    if (buffered == 1) {
        _mkproc_sleep_ticks = 140.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    team = mk_chess_pdata->sides[mk_chess_pdata->manager.active_side];
    if (team->controller->flags.drone_controlled && team->field_164 != 0) team->field_164 = 0;
    board_game_save_data.input_flags.p1_power_squares = 0;
    board_game_save_data.input_flags.p2_power_squares = 0;
    for (x = 0; x < 10; x++)
        for (y = 0; y < 10; y++) mk_chess_count_p1_power_squares(x, y);
    for (x = 0; x < 10; x++)
        for (y = 0; y < 10; y++) mk_chess_count_p2_power_squares(x, y);
    mk_chess_pdata->manager.active_side = next_side;
    mk_chess_pdata->manager.clock++;
    destroy_mkprocs_pid(0xC027);
    mk_chess_pdata->turn_timeout = (unsigned int)(1800.0f * inverse_game_speed);
    mk_chess_pdata->input_transition_busy = 0;
    if (!(g_game_info.field_04 & 0x80)) {
        switch (game_settings.arcade_difficulty) {
        case 0: mk_chess_pdata->turn_timeout = 10800; break;
        case 1: mk_chess_pdata->turn_timeout = 7200; break;
        case 2: mk_chess_pdata->turn_timeout = 3600; break;
        case 3: mk_chess_pdata->turn_timeout = 1800; break;
        case 4:
            if (!mk_chess_pdata->sides[mk_chess_pdata->manager.active_side]->controller->flags.drone_controlled)
                mk_chess_pdata->turn_timeout = 900;
            break;
        }
        mk_chess_pdata->turn_timeout = (unsigned int)((float)mk_chess_pdata->turn_timeout * inverse_game_speed);
    }
    for (side = 0; side < 2; side++) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++)
            mk_chess_calc_all_moves_cb(mk_chess_pdata->sides[side]->pieces[index], 0, 0, 0);
    }
    for (side = 0; side < 2; side++) {
        team = mk_chess_pdata->sides[side];
        leader = 0;
        for (index = 0; index < team->live_piece_count; index++) {
            if (mk_chess_pdata->sides[side]->pieces[index]->type == 5) {
                leader = team->pieces[index];
                break;
            }
        }
        if (leader == 0 || (team->live_piece_count == 1 && team->pieces[0]->move_map->target_count == 0) ||
            (team->hud->flags_09 & 0x20)) {
            mk_chess_pdata->manager.winning_side = side == 0;
            mk_chess_set_game_mode(8);
            break;
        }
    }
    active_side = mk_chess_pdata->manager.active_side;
    group = active_side == 1 ? 17 : 16;
    if (_create_mkproc_generic_bigstack(0xC021, 31, p_mk_chess_show_my_side,
        sizeof(ChessShowSidePdata), &allocation) != 0) {
        display = (ChessShowSidePdata*)allocation;
        display->side = mk_chess_pdata->sides[active_side];
        display->pebble_group = group;
        display->amount = 0.01f;
        display->acceleration = 0.02f;
        display->maximum = 1.2f;
        display->current = 0.0f;
        for (index = 0; index < display->side->live_piece_count; index++) {
            piece = display->side->pieces[index];
            cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
            bgnd_pebble_set_current_pebble(display->pebble_group, index);
            bgnd_pebble_set_current_info(10, 0.0f);
            bgnd_pebble_set_current_info(9, cell->position.x);
            bgnd_pebble_set_current_info(11, cell->position.z);
            bgnd_pebble_set_current_info(14, 0.01f);
            bgnd_pebble_set_current_info(12, 0.01f);
            bgnd_pebble_set_current_info(15, 4.0f);
        }
        bgnd_unhide_pebbles(group);
    }
    if (mk_chess_check_leader_vs_leader_condition() == 0.0f)
        mk_chess_set_game_mode(0);
}



/* TODO: [breakthrough needed] 78.63223%; fight setup recovered; packed flags and fighter selection scheduling remain. */
static void mk_chess_set_vars_for_a_mk_fight(void) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPiece* second = manager->event_piece_24;
    ChessPiece* first = manager->event_data.other_piece;
    ChessCell* first_cell = &mk_chess_pdata->board[first->cell_x].cells[first->cell_y];
    ChessCell* second_cell = &mk_chess_pdata->board[second->cell_x].cells[second->cell_y];
    unsigned int x, y;
    g_game_info.field_1F8 = 2;
    board_game_save_data.input_flags.pad_bit7 = first->side == 0;
    board_game_save_data.input_flags.pad_bits2_1 &= ~2;
    board_game_save_data.input_flags.pad_bits2_1 &= ~1;
    if (second_cell->square_type == 1) {
        if (second->side == 0) board_game_save_data.input_flags.pad_bits2_1 |= 2;
        else board_game_save_data.input_flags.pad_bits2_1 |= 1;
    }
    board_game_save_data.input_flags.p1_power_squares = 0;
    board_game_save_data.input_flags.p2_power_squares = 0;
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) mk_chess_count_p1_power_squares(x, y);
    }
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) mk_chess_count_p2_power_squares(x, y);
    }
    if (first->side == 0 && first_cell->square_type == 1) {
        board_game_save_data.input_flags.p1_power_squares--;
    } else if (first->side == 1 && first_cell->square_type == 1) {
        board_game_save_data.input_flags.p2_power_squares--;
    }
    if ((second->type == 1 && first->type == 1) || (second->type != 1 && first->type != 1)) {
        if (first->side == 0) {
            g_game_info.plyr0.player_index = first->field_14;
            g_game_info.plyr1.player_index = second->field_14;
            g_game_info.plyr0.field_10 = first->health;
            if (first->type == 1) g_game_info.plyr0.field_10 = 1.0f;
            g_game_info.plyr1.field_10 = second->health;
            if (second->type == 1) g_game_info.plyr1.field_10 = 1.0f;
        } else {
            g_game_info.plyr0.player_index = second->field_14;
            g_game_info.plyr1.player_index = first->field_14;
            g_game_info.plyr0.field_10 = second->health;
            if (second->type == 1) g_game_info.plyr0.field_10 = 1.0f;
            g_game_info.plyr1.field_10 = first->health;
            if (first->type == 1) g_game_info.plyr1.field_10 = 1.0f;
        }
    } else if (first->side == 0) {
        g_game_info.plyr0.player_index = first->field_14;
        g_game_info.plyr1.player_index = second->field_14;
        g_game_info.plyr0.field_10 = first->health;
        if (first->type == 1) {
            g_game_info.plyr0.field_10 = second->health;
            g_game_info.plyr0.player_index = second->field_14;
        }
        g_game_info.plyr1.field_10 = second->health;
        if (second->type == 1) {
            g_game_info.plyr1.field_10 = first->health;
            g_game_info.plyr1.player_index = g_game_info.plyr0.player_index;
        }
    } else {
        g_game_info.plyr0.player_index = second->field_14;
        g_game_info.plyr1.player_index = first->field_14;
        g_game_info.plyr0.field_10 = second->health;
        if (second->type == 1) {
            g_game_info.plyr0.field_10 = first->health;
            g_game_info.plyr0.player_index = first->field_14;
        }
        g_game_info.plyr1.field_10 = first->health;
        if (first->type == 1) {
            g_game_info.plyr1.field_10 = second->health;
            g_game_info.plyr1.player_index = g_game_info.plyr0.player_index;
        }
    }
    resolve_alternate_palettes(&g_game_info.plyr0);
}

typedef struct ChessPowerCellEvent {
    unsigned char cell_x;
    unsigned char pad01;
    unsigned char cell_y;
    unsigned char pad03[5];
    ChessPiece* piece;
} ChessPowerCellEvent;

static inline void mk_chess_power_cell_announcement(void)
{
    if ((unsigned short)randu0(100) < 100) {
        unsigned short choice = (unsigned short)randu0(100);
        if (choice < 20) snd_req_delay(1, 30);
        else if (choice < 40) snd_req_delay(2, 30);
        else if (choice < 60) snd_req_delay(3, 30);
        else snd_req_delay(5, 30);
    }
}

/* TODO: [breakthrough needed] 89.66207%; retail counter calls restored; branch/register and relocation residue remains. */
static void mk_chess_cell_monitor(unsigned int x, unsigned int y)
{
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    ChessPiece* piece;
    MkObj* object;
    unsigned int effect;
    int column, row;
    float dx, dz;
    ChessPowerCellEvent event;

    if (cell->flags & 0x20) return;
    if (cell->square_type != 1) return;
    piece = cell->piece;
    if (piece == 0 && (cell->flags & 0x40)) {
        cell->flags &= ~0x40;
        fx_reset_emit(cell->second_emitter);
        return;
    }
    if (piece == 0) return;
    if ((float)piece->side == cell->saved_parameters[1] &&
        (float)piece->id == cell->saved_parameters[2] && (cell->flags & 0x40)) return;
    object = piece->object;
    if (!(object->pos.value.y < 2.5f)) return;
    dz = cell->position.z - object->pos.value.z;
    dx = cell->position.x - object->pos.value.x;
    if (!(dx * dx + dz * dz < 1.0f)) return;
    if (!(cell->flags & 0x40)) {
        MkPfx* particle;
        float px, py, pz;
        cell->flags |= 0x40;
        cell->second_emitter = fx_by_owner("big_health_effect", 4);
        cell->second_emitter = fx_next_emitter(cell->second_emitter);
        effect = cell->second_emitter;
        pz = cell->position.z;
        py = cell->position.y;
        px = cell->position.x;
        if (effect != 0) {
            fx_reset_emit(effect);
            particle = pfx_from_emitter(effect);
            if (particle != 0) {
                object = pfx_bind_emitter_num_to_new_obj(
                    particle, 0x8227, emitter_id_from_handle(effect));
                if (object != 0) {
                    object->flags_08_bits.airborne = 1;
                    object->pos.value.x = px;
                    object->pos.value.y = py;
                    object->pos.value.z = pz;
                    update_mkobj(object);
                    fx_resume_emit(effect);
                }
            }
        }
    }
    effect = fx_by_owner("monster_health", 4);
    fx_set_param_v3(effect, 0x202, cell->position.x, cell->position.y, cell->position.z);
    fx_reset(effect);
    fx_resume_emit(effect);
    snd_req(0x371);
    if (!(mk_chess_pdata->manager.flags & 0x20)) {
        board_game_save_data.input_flags.p1_power_squares = 0;
        board_game_save_data.input_flags.p2_power_squares = 0;
        for (column = 0; column < 10; ++column) {
            for (row = 0; row < 10; ++row) mk_chess_count_p1_power_squares(column, row);
        }
        for (column = 0; column < 10; ++column) {
            for (row = 0; row < 10; ++row) mk_chess_count_p2_power_squares(column, row);
        }
        if (cell->piece->side == 0 && board_game_save_data.input_flags.p1_power_squares == 2) {
            mk_chess_power_cell_announcement();
        } else if (cell->piece->side == 1 && board_game_save_data.input_flags.p2_power_squares == 2) {
            mk_chess_power_cell_announcement();
        } else {
            snd_req_delay(0x2F, 30);
        }
    }
    mk_chess_pdata->manager.flags &= ~0x20;
    cell->saved_parameters[1] = (float)cell->piece->side;
    cell->saved_parameters[2] = (float)cell->piece->id;
    event.cell_x = x;
    event.cell_y = y;
    event.piece = cell->piece;
    mk_chess_piece_event(cell->piece, 23, &event);
}


void mk_chess_count_p2_power_squares(unsigned int x, unsigned int y) {
    ChessCell* cell;

    cell = &mk_chess_pdata->board[x].cells[y];
    if (cell->square_type != 1) {
        return;
    }
    if (cell->piece == 0) {
        return;
    }
    if (cell->piece->side != 1) {
        return;
    }
    board_game_save_data.sides[0].input_flags.p2_power_squares++;
}

void mk_chess_count_p1_power_squares(unsigned int x, unsigned int y) {
    ChessCell* cell;

    cell = &mk_chess_pdata->board[x].cells[y];
    if (cell->square_type != 1) {
        return;
    }
    if (cell->piece == 0) {
        return;
    }
    if (cell->piece->side != 0) {
        return;
    }
    board_game_save_data.sides[0].input_flags.p1_power_squares++;
}

static inline ScreenObj* mk_chess_live_screen(ChessScreenRef* owner) {
    ScreenObj* screen = owner->screen;
    if (screen != 0) {
        if (screen->instance == owner->instance) {
            return screen;
        }
        screen = 0;
    } else {
        screen = 0;
    }
    return screen;
}

static inline void mk_chess_refresh_side_health(unsigned int side_index) {
    ChessSideState* side = mk_chess_pdata->sides[side_index];
    ChessPiece* selected = side->hud->selected_piece;
    ScreenObj* bar;

    if (selected != 0) {
        bar = mk_chess_live_screen(&side->team_art[3]);
        bar->scale_x = 6.0f * selected->health;
        if (side_index == 1) {
            bar->scale_x *= -1.0f;
        }
    }
}


/* TODO: [near miss] 91.09%; typed target address grouping and HUD-latch pointer coloring remain. */
void mk_chess_spell_set_target_health(unsigned int target, float health) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    piece->health = health;
    mk_chess_refresh_side_health(piece->side);
}

/* TODO: [near miss] 81.19%; typed target/board/class address grouping differs; preserve proven array extents. */
float mk_chess_spell_get_target_max_health(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    return g_board_game_controller.class_definitions[piece->type].initial_power;
}

/* TODO: [near miss] 85.00%; typed target/board address grouping differs; stop without new layout evidence. */
void mk_chess_spell_show_target_portrait(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->manager.spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];

    mk_chess_hud_set_piece_portrait(
        mk_chess_pdata->board[x].cells[y].piece);
}

void mk_chess_piece_die(void) {
    mk_chess_remove_piece_at_cell_into_deadpool(
        g_active_piece->cell_x, g_active_piece->cell_y);
}

void mk_chess_piece_temporarily_gone(void) {
    hide_obj(g_active_piece->object);
    mk_chess_remove_piece_from_board(g_active_piece);
    mk_chess_remove_piece_from_team(g_active_piece, 0);
    g_active_piece->state = 0;
}

void mk_chess_set_piece_info(int info, float value) {
    if (info != 3) {
        return;
    }
    g_active_piece->object->ang.y = value;
}


/* TODO: [near miss] 99.82%; data-value exact; anonymous relocation identity remains. */
float mk_chess_get_piece_info(int info) {
    switch (info) {
    case MK_CHESS_PIECE_INFO_X:
        return g_active_piece->object->pos.value.x;
    case MK_CHESS_PIECE_INFO_Y:
        return g_active_piece->object->pos.value.y;
    case MK_CHESS_PIECE_INFO_Z:
        return g_active_piece->object->pos.value.z;
    case MK_CHESS_PIECE_INFO_Y_ANGLE:
        return g_active_piece->object->ang.y;
    default:
        return 0.0f;
    }
}

void mk_chess_piece_post_sleep(void) {
    pdata_of_proc(aproc);
    g_active_piece = 0;
}

void mk_chess_piece_pre_wake(void) {
    ChessPieceProcPdata* pdata =
        (ChessPieceProcPdata*)pdata_of_proc(aproc);

    g_active_piece = pdata->piece;
}


/* TODO: [near miss] 99.38%; data-value exact; anonymous relocation identity remains. */
float p_mk_chess_piece_proc(void) {
    G_BOARD_GAME_BIGSTACK_COUNTER--;
    g_active_piece->proc_state = 0;
    return -1.0f;
}

void mk_chess_ani_1_frame(void) {
    ChessAnimPdata* animation = g_active_piece->animation;

    advance_anim((AnimPdata*)animation);
    pose_anim((AnimPdata*)animation, 1);
}

void mk_chess_piece_match_y_ang_to_anim(void) {
    g_active_piece->object->hide_flag_bits.bit0 = 1;
}

void mk_chess_glitch_to_ani_frame(int animation, int flags, float speed,
                                  float frame) {
    ChessAnimPdata* anim = g_active_piece->animation;

    g_active_piece->flags.glitch_into_stance = 0;
    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    anim->speed = speed;
    set_anim_script_frame(
        frame, (AnimPdata*)anim, (AniData*)mkc_animations[animation],
        flags);
}


/* TODO: [near miss] 99.83%; data-value exact; anonymous relocation identity remains. */
void mk_chess_blend_to_ani_frame(int animation, int flags, float blend,
                                 float speed, float frame) {
    ChessAnimPdata* anim = g_active_piece->animation;

    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    anim->speed = speed;
    transition_to_anim_script_frame(
        blend, frame, (AnimPdata*)anim,
        (AniData*)mkc_animations[animation], flags);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
}

/* TODO: [near miss] 97.35%; same operations; redundant animation-pointer move remains; stop at coloring. */
void mk_chess_blend_to_ani(int animation, int flags, float blend, float speed) {
    ChessAnimPdata* anim = g_active_piece->animation;

    set_root_and_obj_movement_weights(0.0f, 1.0f, (AnimPdata*)anim);
    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    anim->speed = speed;
    transition_to_anim_script(
        blend, (AnimPdata*)anim, (AniData*)mkc_animations[animation], flags);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
}


void mk_chess_air_move(void) {
    g_active_piece->object->flags_09_bits.bit6 = 0;
}

void mk_chess_set_ani_speed(float speed) {
    g_active_piece->animation->speed = speed;
}


/* TODO: [near miss] 99.58%; data-value exact; anonymous relocation identity remains. */
void mk_chess_set_obj_move_weight(float weight) {
    set_root_and_obj_movement_weights(
        0.0f, weight, (AnimPdata*)g_active_piece->animation);
}

void mk_chess_blend_to_desired_cell_position_setting(float blend) {
    g_active_piece->movement->desired_cell_blend = blend;
}

void mk_chess_queue_up_piece_event(int event, int delay) {
    if (g_active_piece == 0) {
        return;
    }
    g_active_piece->flags.event_pending = 1;
    g_active_piece->runtime.fields.queued_event = event;
    g_active_piece->runtime.fields.event_time = mk_chess_pdata->manager.clock + delay;
}

/* TODO: [breakthrough needed] 97.87%; resolve retail stwu r1, -0x20(r1) and its surrounding ownership/CFG before further tuning. */
int mk_chess_handle_buffered_events_cb(ChessPiece* piece) {
    ChessManagerInfo* manager;
    ChessPieceEventResult result;

    manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    if (piece->flags.event_pending == 1 &&
        piece->runtime.fields.event_time == manager->clock) {
        piece->flags.event_pending = 0;
        mk_chess_piece_event(
            piece, piece->runtime.fields.queued_event, &result);
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 69.62%; same timer accesses; lwzx/stwx versus displaced loads/stores; preserve array bounds. */
int mk_chess_piece_test_and_set_timer(unsigned int timer_slot,
                                      unsigned int duration) {
    unsigned int* timer;
    unsigned int now;

    timer = &g_active_piece->runtime.timer_slots[timer_slot];
    now = (unsigned int)exec_tick_ctr;
    if (*timer < now) {
        *timer = now + duration;
        return 1;
    }
    return 0;
}

void mk_chess_blend_to_normal_stance(void) {
    g_active_piece->requested_script = g_active_piece->normal_stance_script;
}

void mk_chess_set_piece_state(int state) {
    g_active_piece->state = state;
}


/* TODO: [near miss] 99.78%; data-value exact; anonymous relocation identity remains. */
void mk_chess_set_cell_offset(float x, float y, float z) {
    g_active_piece->runtime.fields.cell_offset.x = x;
    g_active_piece->runtime.fields.cell_offset.y = y;
    g_active_piece->runtime.fields.cell_offset.z = z;

    if (g_active_piece->side != 1) {
        return;
    }
    g_active_piece->runtime.fields.cell_offset.x =
        -1.0f * g_active_piece->runtime.fields.cell_offset.x;
    g_active_piece->runtime.fields.cell_offset.y =
        -1.0f * g_active_piece->runtime.fields.cell_offset.y;
    g_active_piece->runtime.fields.cell_offset.z =
        -1.0f * g_active_piece->runtime.fields.cell_offset.z;
}

/* TODO: [near miss] 99.86%; data-value exact; anonymous relocation identity remains. */
void mk_chess_snap_into_cell_orgin_over_x_frames(float frames) {
    while (frames-- > 0.0f) {
        ChessCell* cell = &mk_chess_pdata->board[g_active_piece->cell_x]
                              .cells[g_active_piece->cell_y];
        ChessAnimPdata* animation;

        g_active_piece->object->pos.value.x =
            cell->position.x + g_active_piece->runtime.fields.cell_offset.x;
        g_active_piece->object->pos.value.z =
            cell->position.z + g_active_piece->runtime.fields.cell_offset.z;
        update_obj_pos(g_active_piece->object);
        g_active_piece->object->hide_flag_bits.pin_animation = 0;
        animation = g_active_piece->animation;
        advance_anim((AnimPdata*)animation);
        pose_anim((AnimPdata*)animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
}

static inline void mk_chess_blend_piece_to_cell(float frames) {
    ChessPiece* piece = g_active_piece;
    MkObj* object = piece->object;
    float inverse_frames = 1.0f / frames;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    float step_x = (object->pos.value.x -
        (cell->position.x + piece->runtime.fields.cell_offset.x)) * inverse_frames;
    float step_z = (object->pos.value.z -
        (cell->position.z + piece->runtime.fields.cell_offset.z)) * inverse_frames;
    ChessAnimPdata* animation;

    while (frames-- > 0.0f) {
        g_active_piece->object->pos.value.x -= step_x;
        g_active_piece->object->pos.value.z -= step_z;
        update_obj_pos(g_active_piece->object);
        g_active_piece->object->hide_flag_bits.pin_animation = 0;
        animation = g_active_piece->animation;
        advance_anim((AnimPdata*)animation);
        pose_anim((AnimPdata*)animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
}

/* TODO: [near miss] 73.50%; fixed-step math/calls agree; retail vector temporary stores and FP scheduling differ. */
void mk_chess_blend_into_cell_orgin_in_x_frames(float frames) {
    mk_chess_blend_piece_to_cell(frames);
}

/* TODO: [near miss] 79.05%; duration/reset and blend agree; vector temporary stores and FP scheduling differ. */
void mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void) {
    mk_chess_blend_piece_to_cell(g_active_piece->movement->desired_cell_blend);
    g_active_piece->movement->desired_cell_blend = 30.0f;
}

/* TODO: [near miss] 98.17461%; retained X snapshot restored; stop at FP coloring and zero-result move. */
void mk_chess_blend_to_my_cell_pos(float distance) {
    ChessPiece* piece = g_active_piece;
    MkObj* object = piece->object;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    float original_x = object->pos.value.x;
    float dx = cell->position.x + piece->runtime.fields.cell_offset.x - original_x;
    float dz = cell->position.z + piece->runtime.fields.cell_offset.z - object->pos.value.z;
    float length_squared = dx * dx + dz * dz;
    float inverse_length;
    union { float f; unsigned int u; } input, estimate;

    if (length_squared < distance * distance) {
        return;
    }
    if (length_squared <= 0.0f) {
        inverse_length = 0.0f;
    } else {
        float product;
        float correction;

        /* Retail's bit estimate and polynomial inverse square root. */
        input.f = length_squared;
        estimate.u = 0x5F375A00U - (input.u >> 1);
        product = estimate.f * (length_squared * estimate.f);
        correction = 3.0f - product;
        inverse_length = (0.0625f * estimate.f) * correction *
            (12.0f - correction * (product * correction));
    }
    dx *= inverse_length;
    dz *= inverse_length;
    dx *= distance;
    dz *= distance;
    object->pos.value.x = original_x + dx;
    g_active_piece->object->pos.value.z += dz;
}

void mk_chess_snap_to_my_cell_now(void) {
    ChessCell* cell;

    cell = &mk_chess_pdata->board[g_active_piece->cell_x]
                .cells[g_active_piece->cell_y];
    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    g_active_piece->object->pos.value.x = cell->position.x;
    g_active_piece->object->pos.value.z = cell->position.z;
    update_obj_pos(g_active_piece->object);
}


/* TODO: [near miss] 95.90%; conversions and stores agree; stop at board-address scheduling and pointer coloring. */
void mk_chess_put_active_piece_at_cell(int snap, float x, float y) {
    ChessPiece* piece = g_active_piece;
    ChessCell* previous = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    ChessCell* cell = &mk_chess_pdata->board[(unsigned char)x].cells[(unsigned char)y];

    if (previous->piece == piece) {
        previous->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    if (snap == 1) {
        piece->object->hide_flag_bits.pin_animation = 0;
        piece->object->pos.value.x = cell->position.x + piece->runtime.fields.cell_offset.x;
        piece->object->pos.value.z = cell->position.z + piece->runtime.fields.cell_offset.z;
        update_obj_pos(piece->object);
    }
    g_active_piece->movement->cell_x = x;
    g_active_piece->movement->cell_y = y;
}


static int mk_chess_deactivate_scene_except_selected_pieces_cb(
    ChessPiece* piece, ChessPiece* first, ChessPiece* second);
static int mk_chess_hide_all_except_selected_pieces_cb(
    ChessPiece* piece, ChessPiece* selected, ChessPiece* other_selected);

typedef struct ChessSceneFadePdata {
    MkHdr hdr;
    ChessPiece* first;
    ChessPiece* second;
    unsigned int delay;
    int step;
} ChessSceneFadePdata; /* 0x18, allocated by mk_chess_request_piece_fight. */

void make_subobject_transl(MkSobj* object);
void obj_sobj_set_material(MkSobj* object, unsigned int alpha);

/* TODO: [breakthrough needed] 97.540985%; retail callback boundary restored; loop scheduling and unused callback arguments remain. */
static float p_mk_chess_fade_scene_for_fight(void) {
    unsigned int piece_alpha = 255;
    unsigned int background_alpha = 255;
    unsigned int overlay_alpha = 0;
    MkSobj* background = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    MkSobj* overlay = obj_find_sobj_by_id(g_game_info.bgnd_obj, 2);
    ChessSceneFadePdata* data = (ChessSceneFadePdata*)apdata;
    ChessPiece* first;
    ChessPiece* second;
    unsigned int side, index;

    make_subobject_transl(overlay);
    make_subobject_transl(background);
    sobj_set_priority(overlay, 8);
    overlay->flags09_bits.bit7 = 1;
    sobj_set_priority(background, 9);
    obj_sobj_set_material(overlay, 0);
    unhide_sobj(overlay);
    _mkproc_sleep_ticks = data->delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    second = data->second;
    first = data->first;
    for (side = 0; side < 2; side++) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
            mk_chess_deactivate_scene_except_selected_pieces_cb(
                mk_chess_pdata->sides[side]->pieces[index], first, second);
        }
    }
    while (piece_alpha > 8) {
        obj_sobj_set_material(background, background_alpha);
        obj_sobj_set_material(overlay, overlay_alpha);
        second = data->second;
        first = data->first;
        for (side = 0; side < 2; side++) {
            for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
                ChessPiece* piece = mk_chess_pdata->sides[side]->pieces[index];
                if (piece != first && piece != second) {
                    obj_sobj_set_material(obj_first_sobj(piece->object), piece_alpha);
                }
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        background_alpha -= 3;
        overlay_alpha = 255 - background_alpha;
        piece_alpha -= data->step;
    }
    second = data->second;
    first = data->first;
    for (side = 0; side < 2; side++) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
            mk_chess_hide_all_except_selected_pieces_cb(
                mk_chess_pdata->sides[side]->pieces[index], first, second);
        }
    }
    while (background_alpha > 3) {
        obj_sobj_set_material(background, background_alpha);
        obj_sobj_set_material(overlay, overlay_alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        background_alpha -= 3;
        overlay_alpha = 255 - background_alpha;
    }
    hide_sobj(background);
    overlay->flags09_bits.bit7 = 0;
    return -1.0f;
}

/* TODO: [near miss] 99.70%; data-value exact; anonymous relocation identity remains. */
static int mk_chess_deactivate_scene_except_selected_pieces_cb(
    ChessPiece* piece, ChessPiece* first, ChessPiece* second) {
    if (piece == first) {
        return 0;
    }
    if (piece == second) {
        return 0;
    }
    if (piece != 0) {
        switch (piece->type) {
        case 5:
            fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
            break;
        case 3:
            fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
            fx_set(piece->runtime.fields.secondary_effect, 0x204, 1.0f);
            fx_pause_emit(piece->runtime.fields.primary_effect);
            fx_pause_emit(piece->runtime.fields.secondary_effect);
            break;
        case 4:
            fx_pause_emit(piece->runtime.fields.primary_effect);
            break;
        }
    }
    return 0;
}


/* TODO: [near miss] 99.63%; data-value exact; anonymous relocation identity remains. */
void mk_chess_deactivate_my_properties(void) {
    ChessPiece* piece = g_active_piece;

    if (piece == 0) {
        return;
    }

    switch (piece->type) {
    case 5:
        fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
        return;
    case 3:
        fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
        fx_set(piece->runtime.fields.secondary_effect, 0x204, 1.0f);
        fx_pause_emit(piece->runtime.fields.primary_effect);
        fx_pause_emit(piece->runtime.fields.secondary_effect);
        return;
    case 4:
        fx_pause_emit(piece->runtime.fields.primary_effect);
        break;
    }
}

/* TODO: [near miss] 90.06%; bone/effect/removal order agrees; stop at board-address lowering. */
static void mk_chess_remove_piece_at_cell_into_deadpool(
    unsigned char x, unsigned char y) {
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    obj_clear_bone_collapse_flag(piece->object, 1);
    obj_clear_bone_collapse_flag(piece->object, 2);
    obj_clear_bone_collapse_flag(piece->object, 10);
    obj_clear_bone_collapse_flag(piece->object, 12);
    obj_clear_bone_collapse_flag(piece->object, 13);
    if (piece != 0) {
        switch (piece->type) {
        case 5:
            fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
            break;
        case 3:
            fx_set(piece->runtime.fields.primary_effect, 0x204, 1.0f);
            fx_set(piece->runtime.fields.secondary_effect, 0x204, 1.0f);
            fx_pause_emit(piece->runtime.fields.primary_effect);
            fx_pause_emit(piece->runtime.fields.secondary_effect);
            break;
        case 4:
            fx_pause_emit(piece->runtime.fields.primary_effect);
            break;
        }
    }
    mk_chess_remove_piece_from_team(piece, 1);
    mk_chess_remove_piece_from_board(piece);
}

void mk_chess_activate_my_properties(void) {
    mk_chess_activate_piece_properties(g_active_piece);
}

void mk_chess_set_glitch_stance_flag(void) {
    g_active_piece->flags.glitch_into_stance = 1;
}

int mk_chess_check_glitch_into_stance(void) {
    return g_active_piece->flags.glitch_into_stance;
}

int mk_chess_check_snap_into_stance(void) {
    return g_active_piece->flags.snap_into_stance;
}

void mk_chess_dont_constrain_piece(void) {
    g_active_piece->flags.dont_constrain = 1;
}

/* TODO: [breakthrough needed] 98.16%; resolve retail fabs f3, f1 and its surrounding ownership/CFG before further tuning. */
int mk_chess_active_piece_near_edge(void) {
    float x;
    float z;

    x = (float)__fabs(g_active_piece->object->pos.value.x);
    z = (float)__fabs(g_active_piece->object->pos.value.z);
    if (x >= 8.0f || z >= 8.0f) {
        return 1;
    }
    return 0;
}

void mk_chess_piece_set_state(int state) {
    g_active_piece->state = state;
}

void mk_chess_piece_is_idle(void) {
    g_active_piece->state = 0;
}

static inline ChessPiece* mk_chess_find_piece_on_board(unsigned int side) {
    unsigned char x;
    unsigned char y;

    for (x = 0; x < MK_CHESS_BOARD_COLUMNS; x++) {
        for (y = 0; y < MK_CHESS_BOARD_COLUMNS; y++) {
            ChessPiece* candidate = mk_chess_pdata->board[x].cells[y].piece;
            if (candidate != 0 && candidate->side == side) {
                return candidate;
            }
        }
    }
    return 0;
}


/* TODO: [near miss] 96.22%; same board search; active-slot address grouping remains; stop at lowering. */
static void mk_chess_remove_piece_from_board(ChessPiece* piece) {
    ChessPiece** active_piece;

    mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].piece = 0;
    active_piece = &mk_chess_pdata->manager.active_piece_by_side[piece->side];
    if (*active_piece != 0 && (*active_piece)->id == piece->id) {
        *active_piece = mk_chess_find_piece_on_board(piece->side);
    }
}


/* TODO: [breakthrough needed] 90.53%; resolve retail lwz r6, mk_chess_pdata@sda21 and its surrounding ownership/CFG before further tuning. */
ChessPiece* mk_chess_fetch_piece_at_cursor(void) {
    ChessManagerInfo* manager;
    ChessCursor* cursor;

    if (mk_chess_pdata != 0) {
        manager = &mk_chess_pdata->manager;
    } else {
        manager = 0;
    }
    cursor = &mk_chess_pdata->cursors[manager->active_side];
    return mk_chess_pdata->board[cursor->cell_x]
        .cells[cursor->cell_y].piece;
}

int mk_chess_fetch_current_side_based_on_ones(unsigned int side) {
    int result = -1;

    if (side == 0) {
        result = 1;
    }
    return result;
}

ChessManagerInfo* mk_chess_fetch_manager_info(void) {
    if (mk_chess_pdata != 0) {
        return &mk_chess_pdata->manager;
    }
    return 0;
}

void mk_chess_enable_cam_zoom_sound(int enabled) {
    ChessCameraSoundState* sound;

    sound = mk_chess_pdata != 0 ? &mk_chess_pdata->camera_sound : 0;
    if (sound->zoom_sound != 0) {
        snd_stop(sound->zoom_sound);
        sound->zoom_sound = 0;
    }

    if (enabled == 1) {
        sound->zoom_sound = snd_req(0x374);
    }
}

ChessCameraInfo* mk_chess_fetch_camera_info(void) {
    if (mk_chess_pdata != 0) {
        return &mk_chess_pdata->camera;
    }
    return 0;
}

int mk_chess_allow_setting_of_viewing_quadrant(void) {
    return g_game_info.feature_flags.bits.high_bit == 0;
}

/* TODO: [near miss] 99.75%; data-value exact; anonymous relocation identity remains. */
static float p_mk_chess_fade_single_piece(void) {
    int finished = 0;
    ChessPieceFadePdata* fade = (ChessPieceFadePdata*)apdata;

    if (fade->delay != 0) {
        fade->delay--;
        return 1.0f;
    }
    unhide_obj(fade->piece->object);
    if (fade->step < 0 && fade->color.alpha < -fade->step) {
        finished = 1;
    }
    if (fade->step > 0 && fade->color.alpha > fade->target_alpha - fade->step) {
        finished = 1;
    }
    if (finished == 1) {
        if (fade->target_alpha == 0) {
            hide_obj(fade->piece->object);
        }
        fade->color.alpha = fade->target_alpha;
        obj_set_color_for_all_materials(fade->piece->object, &fade->color);
        return -1.0f;
    }
    fade->color.alpha += fade->step;
    obj_set_color_for_all_materials(fade->piece->object, &fade->color);
    return 1.0f;
}

void mk_chess_cleanup(void) {
    g_loser_life_resolved = 0;
    g_active_piece = 0;
    g_active_team = 0;
    g_active_piece_being_defined = 0;
    mk_chess_pdata = 0;
    g_active_class_definition = 0;
    memset(&g_board_game_controller, 0, sizeof(g_board_game_controller));
}


/* TODO: [near miss] 94.25%; pooled string base/addend differs; recover real TU string order before linking. */
void mk_chess_in_fight_setup(void) {
    load_ssf((MkFileEntry*)mkchess_ingame_art_file_table);
    load_string_bank(0x20000, "boardgame_strings_eng.mko");
    g_game_info.plyr0.field_0C = 1.0f;
    g_game_info.plyr1.field_0C = 1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_4(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_4);
    }
    return -1.0f;
}


/* TODO: [near miss] 99.62%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_over_3(void) {
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        board_game_save_data.sides[0].flags.board_input_seen = 1;
    }
    return -1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_3(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_3);
    }
    return -1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_2(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_2);
    }
    return -1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_1(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_1);
    }
    return -1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_r2(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_r2);
    }
    return -1.0f;
}


/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float p_board_switch_l1(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->player->controller_slot]->input_proc,
            x_chess_l1);
    }
    return -1.0f;
}

#pragma dont_inline on
int mk_chess_input_possibile(void) {
    ChessManagerInfo* manager;
    int state_accepts_input;

    if (mk_chess_check_input_from_correct_side_no_ai() != 0) {
        manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        if (ck_eat_online_switches() != 0) {
            state_accepts_input = 0;
        } else {
            switch (manager->input_state) {
            case 0:
            case 1:
            case 9:
            case 10:
                state_accepts_input = 1;
                break;
            default:
                state_accepts_input = 0;
                break;
            }
        }

        if (state_accepts_input &&
            board_game_save_data.sides[0].input_flags.input_locked == 0) {
            return 1;
        }
    }
    return 0;
}


/* TODO: [breakthrough needed] 78.68%; retail has a redundant drone-rejection branch; resolve unsigned/lifetime lowering without fake reads. */
static int mk_chess_check_input_from_correct_side_no_ai(void) {
    ChessManagerInfo* manager;
    unsigned int switch_player;

    if (switch_pdata->player == 0) {
        return 0;
    }

    /* Retail 0x80139EB0..B4 dereferences a null manager when a player exists
     * without Chess state. Preserve that failure path; do not add a guard. */
    manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    switch_player = switch_pdata->player->controller_slot;

    if (manager->input_state == 10) {
        return 1;
    }
    if (switch_player == manager->active_side) {
        return 1;
    }

    /*
     * A drone-controlled active side also rejects human input. Retail checks
     * this flag before the common false return.
     */
    if (mk_chess_pdata->sides[manager->active_side]
            ->controller->flags.drone_controlled) {
        return 0;
    }
    return 0;
}
#pragma dont_inline reset

int mk_chess_return_active_pad(void) {
    ChessManagerInfo* manager =
        mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;

    if (manager->active_side == 0) {
        return g_game_info.plyr0.pad_index;
    }
    return g_game_info.plyr1.pad_index;
}

int mk_chess_allow_cam_control(void) {
    ChessManagerInfo* manager =
        mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int controller_enabled;
    int result;

    if (manager->active_side == 0) {
        controller_enabled =
            is_plyr_controller_enabled(&g_game_info.plyr0);
    } else {
        controller_enabled =
            is_plyr_controller_enabled(&g_game_info.plyr1);
    }

    if (controller_enabled == 0) {
        result = 0;
    } else {
        result = 0;
        if ((unsigned int)manager->input_state != 10 &&
            (unsigned int)manager->input_state != 13) {
            result = 1;
        }
    }
    return result;
}

static inline CameraObj* camera_live_node(CameraItem* owner) {
    CameraObj* object = owner->node;
    if (object != 0) {
        if (object->hdr.instance == owner->instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

extern float DEFAULT_ASPECTRATIO;
void CameraSize(RwCamera* camera, RwRect* rect, float view_window, float aspect_ratio);

/* TODO: [near miss] 99.48%; data-value exact; anonymous aggregate and float relocations remain. */
void mk_chess_camera_init(void) {
    Vec target = {0.0f, 0.0f, 0.0f};
    CameraObj* initial_camera = camera_live_node(&camera_item);

    CameraSize(Camera, 0, gxMathTan(0.28797933f), DEFAULT_ASPECTRATIO);
    if (board_game_save_data.restore_pending != 0) {
        initial_camera->pos.x = board_game_save_data.camera_position.x;
        initial_camera->pos.y = board_game_save_data.camera_position.y;
        initial_camera->pos.z = board_game_save_data.camera_position.z;
    } else {
        ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        CameraObj* camera = camera_live_node(&camera_item);
        Vec origin;

        camera->pos.y = 30.2f;
        if (manager->active_side == 1) {
            camera->pos.x = 0.0f;
            camera->pos.z = 33.2f;
        } else {
            camera->pos.z = -33.2f;
            camera->pos.x = 0.0f;
        }
        origin.z = 0.0f;
        origin.y = 0.0f;
        origin.x = 0.0f;
        look_at_target(&origin);
        update_mkobj(camera != 0 ? as_mkhdr(&camera->hdr) : 0);
    }
    mk_chess_pdata->camera.completion = 0;
    look_at_target(&target);
    mk_chess_pdata->camera.current_look_at.x = target.x;
    mk_chess_pdata->camera.current_look_at.y = target.y;
    mk_chess_pdata->camera.current_look_at.z = target.z;
    xfer_camera(p_mk_chess_cam_control, 0);
    mk_chess_pdata->camera.viewing_quadrant = 0;
    mk_chess_pdata->camera.field_4C = 0;
    mk_chess_pdata->camera.field_50 = 0;
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    mk_chess_set_viewing_quadrant(initial_camera);
}

/* TODO: [near miss] 99.70%; data-value exact; anonymous relocation identity remains. */
float mk_chess_zoom_return_completed(void) {
    CameraObj* camera = camera_live_node(&camera_item);


    if (camera != 0) {
        mk_chess_set_viewing_quadrant(camera);
    }

    mk_chess_set_game_mode(7);
    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_mk_chess_cam_control, 0.0f);
    return 0.0f;
}




/* TODO: [near miss] 99.79%; data-value exact; anonymous relocation identity remains. */
float mk_chess_zoom_completed(void) {
    ChessCameraInfo* camera_info =
        mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    CameraObj* camera = camera_live_node(&camera_item);



    camera_info->viewing_camera = camera_info->zoom_camera;
    camera_info->current_look_at.x = camera_info->desired_look_at.x;
    camera_info->current_look_at.y = camera_info->desired_look_at.y;
    camera_info->current_look_at.z = camera_info->desired_look_at.z;
    mk_chess_set_game_mode(7);
    mk_chess_set_viewing_quadrant(camera);
    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_mk_chess_cam_chase_cursor, 0.0f);
    return 0.0f;
}


/* TODO: [near miss] 99.62%; data-value exact; anonymous relocation identity remains. */
static float x_chess_1(void) {
    ChessInputPdata* pdata = (ChessInputPdata*)apdata;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }
    switch (mk_chess_pdata->manager.input_state) {
    case 10:
        mk_chess_pdata->manager.directional_state->actions[pdata->side] = 1;
        break;
    case 1:
        snd_req(0x36F);
        mk_chess_game_event(2, mk_chess_pdata->manager.event_data.pieces, 1, 0);
        mk_chess_set_game_mode(0);
        break;
    case 9:
        mk_chess_pdata->manager.spell->input_state = 10;
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 99.49%; data-value exact; anonymous relocation identity remains. */
static float x_chess_l1(void) {
    ChessCameraInfo* camera = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    if (camera->field_4C != 0 || camera->field_50 != 0) {
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }
    switch (mk_chess_pdata->manager.input_state) {
    case 9:
        mk_chess_pdata->manager.spell->input_state = 10;
        break;
    case 0:
        mk_chess_show_spell_hud(mk_chess_pdata->manager.active_side);
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 98.41%; camera reset/handoff agree; latch lowering and constant addressing remain. */
float x_chess_r2(void) {
    ChessManagerInfo* manager;
    CameraObj* camera;
    Vec target;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }
    if (mk_chess_pdata->manager.input_state == 0) {
        snd_req(0x379);
        manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        camera = camera_live_node(&camera_item);
        camera->pos.y = 30.2f;
        if (manager->active_side == 1) {
            camera->pos.x = 0.0f;
            camera->pos.z = 33.2f;
        } else {
            camera->pos.z = -33.2f;
            camera->pos.x = 0.0f;
        }
        target.z = 0.0f;
        target.y = 0.0f;
        target.x = 0.0f;
        look_at_target(&target);
        update_mkobj(camera != 0 ? as_mkhdr(&camera->hdr) : 0);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        mk_chess_set_viewing_quadrant(camera_live_node(&camera_item));
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 99.71%; data-value exact; anonymous relocation identity remains. */
float x_chess_4(void) {
    ChessInputPdata* input = (ChessInputPdata*)apdata;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    switch (mk_chess_pdata->manager.input_state) {
    case 10:
        mk_chess_pdata->manager.directional_state->actions[input->side] = 2;
        break;
    case 1:
        snd_req(0x36F);
        mk_chess_game_event(2, mk_chess_pdata->manager.event_data.pieces, 1, 0);
        mk_chess_set_game_mode(0);
        break;
    case 9:
        mk_chess_pdata->manager.spell->input_state = 10;
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}


/* TODO: [near miss] 99.64%; data-value exact; anonymous relocation identity remains. */
float x_chess_2(void) {
    if (mk_chess_pdata == 0) {
        return -1.0f;
    }

    switch (mk_chess_pdata->manager.input_state) {
    case 1:
        snd_req(0x36F);
        mk_chess_game_event(2, mk_chess_pdata->manager.event_data.pieces, 1, 0);
        mk_chess_set_game_mode(0);
        break;
    case 9:
        mk_chess_pdata->manager.spell->input_state = 10;
        break;
    }

    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

int mk_chess_get_current_difficulty_for_ai(unsigned int side) {
    if (side == 0) {
        return (board_game_save_data.ai_settings[0] >> 3) & 0xF;
    }
    return (board_game_save_data.ai_settings[1] >> 2) & 0xF;
}

void mk_chess_set_breaker_value(void) {
    int difficulty;

    if (plyr_pdata->plyr_num == 0) {
        difficulty =
            (board_game_save_data.ai_settings_halves[0] >> 7) & 0xF;
    } else {
        difficulty =
            (board_game_save_data.ai_settings_word >> 14) & 0xF;
    }

    switch (difficulty) {
    case 0:
    case 3:
    case 4:
        plyr_pdata->breaker_strength = 1;
        break;
    case 5:
        plyr_pdata->breaker_strength = 3;
        break;
    default:
        plyr_pdata->breaker_strength = 2;
        break;
    }
}

/* TODO: [breakthrough needed] 88.67%; resolve retail extrwi r0, r0, 1, 25 and its surrounding ownership/CFG before further tuning. */
int mk_chess_did_the_king_just_lose(void) {
    if ((board_game_save_data.ai_settings[1] & 0x40) != 0 &&
        g_game_info.plyr1.field_0C == 0.0f) {
        return 1;
    }
    if ((board_game_save_data.ai_settings[0] & 0x80) != 0 &&
        g_game_info.plyr0.field_0C == 0.0f) {
        return 1;
    }
    return 0;
}

void mk_chess_restore_board(void) {
    unsigned int x;
    unsigned int y;

    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            mk_chess_place_special_cell_at(
                x, y, board_game_save_data.cells[x][y].type, 1,
                board_game_save_data.cells[x][y].x,
                board_game_save_data.cells[x][y].y,
                board_game_save_data.cells[x][y].z,
                board_game_save_data.cells[x][y].scale);
        }
    }
}

static void mk_chess_team_init(unsigned int side) {
    unsigned int index;
    unsigned short roll = randu0(100);

    mk_chess_pdata->sides[side]->live_piece_count = 0;
    mk_chess_pdata->sides[side]->captured_piece_count = 0;
    for (index = 0; index < 6; index++) {
        mk_chess_pdata->sides[side]->portraits[index].screen = 0;
        mk_chess_pdata->sides[side]->portraits[index].instance = 0;
        mk_chess_pdata->sides[side]->field_C4[index][0] = 0;
        mk_chess_pdata->sides[side]->field_C4[index][1] = 0;
    }
    mk_chess_pdata->sides[side]->field_164 = 0;
    mk_chess_pdata->sides[side]->field_144 = 0;
    mk_chess_pdata->sides[side]->saved_field_FC = 0xFFFF;
    mk_chess_pdata->sides[side]->saved_field_100 = 0;
    if (game_settings.arcade_difficulty < 2) {
        mk_chess_pdata->sides[side]->strategy = 0;
    } else if (game_settings.arcade_difficulty == 2) {
        if (roll < 20) {
            mk_chess_pdata->sides[side]->strategy = 0;
        } else if (roll < 60) {
            mk_chess_pdata->sides[side]->strategy = 2;
        } else if (roll < 95) {
            mk_chess_pdata->sides[side]->strategy = 4;
        } else {
            mk_chess_pdata->sides[side]->strategy = 1;
        }
    } else if (game_settings.arcade_difficulty == 3) {
        if (roll < 20) {
            mk_chess_pdata->sides[side]->strategy = 2;
        } else if (roll < 45) {
            mk_chess_pdata->sides[side]->strategy = 3;
        } else if (roll < 90) {
            mk_chess_pdata->sides[side]->strategy = 1;
        } else {
            mk_chess_pdata->sides[side]->strategy = 5;
        }
    } else if (roll < 45) {
        mk_chess_pdata->sides[side]->strategy = 3;
    } else if (roll < 55) {
        mk_chess_pdata->sides[side]->strategy = 1;
    } else {
        mk_chess_pdata->sides[side]->strategy = 5;
    }
    if (get_game_state() == 3) {
        roll = randu0(100);
        if (roll < 50) {
            mk_chess_pdata->sides[side]->strategy = 1;
        } else if (roll < 60) {
            mk_chess_pdata->sides[side]->strategy = 3;
        } else {
            mk_chess_pdata->sides[side]->strategy = 5;
        }
    }
    for (index = 0; index < 6; index++) {
        mk_chess_pdata->sides[side]->saved_state_114[index] = 4;
    }
    mk_chess_pdata->sides[side]->saved_field_110 = 0;
}
void mk_chess_set_piece_type_as(int type, int restoring);
void mk_chess_launch_special_fx(unsigned int kind, unsigned int mode, unsigned int expiry);

static inline void mk_chess_restore_piece_event(ChessSavedPiece* saved) {
    if (saved->event_pending == 1) {
        g_active_piece_being_defined->runtime.fields.queued_event = saved->queued_event;
        g_active_piece_being_defined->runtime.fields.event_time = saved->event_time;
        g_active_piece_being_defined->flags.event_pending = 1;
    }
}

static inline void mk_chess_restore_live_piece(unsigned char side, unsigned char index,
                                               ChessSavedPiece* saved, float health) {
    mk_chess_create_piece(side, index, saved->id, saved->cell_x, saved->cell_y,
                          saved->library_index, 1, health);
    mk_chess_restore_piece_event(saved);
    mk_chess_set_piece_type_as(saved->type, 1);
    g_active_piece_being_defined->used_spells = saved->used_spells;
}

static inline int mk_chess_has_live_effect(MkPtr** effects, unsigned int kind) {
    MkPtr* link;
    MkPtr* next;
    ChessPieceEffect* effect;
    if (effects != 0) {
        link = *effects;
        while (link != 0) {
            effect = (ChessPieceEffect*)link->hdr;
            if (link->instance != effect->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if ((unsigned int)effect->kind == kind) {
                    return 1;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/* TODO: [near miss] 82.00%; typed restore/control flow agrees; saved-record addressing, effect lookup and coloring differ. */
static void mk_chess_restore_teams(void) {
    unsigned char side;
    unsigned char index;
    unsigned char field;
    unsigned char effect_index;
    float health = g_game_info.plyr0.field_0C;
    ChessSavedTeam* saved_team;
    ChessSavedPiece* saved;
    ChessPiece* piece;
    unsigned int kind;
    unsigned int expiry;

    for (side = 0; side < 2; side++) {
        mk_chess_team_init(side);
        mk_chess_load_team_art(side);
        if (side != 0) {
            health = g_game_info.plyr1.field_0C;
        }
        saved_team = &board_game_save_data.teams[side];
        mk_chess_pdata->sides[side]->live_piece_count = saved_team->live_piece_count;
        for (index = 0; index < saved_team->live_piece_count; index++) {
            saved = &saved_team->pieces[index];
            if ((side == 0 && board_game_save_data.fighter_ids[0] == saved->id) ||
                (side == 1 && board_game_save_data.fighter_ids[1] == saved->id)) {
                mk_chess_restore_live_piece(side, index, saved, health);
            } else {
                mk_chess_restore_live_piece(side, index, saved, saved->health);
            }
            for (field = 0; field < 6; field++) {
                g_active_piece_being_defined->access_restrictions[field] =
                    saved->access_restrictions[field];
            }
            for (effect_index = 0; effect_index < saved->effect_count; effect_index++) {
                piece = g_active_piece_being_defined;
                expiry = saved->effects[effect_index].expiry_clock;
                kind = saved->effects[effect_index].kind;
                if (g_active_piece == 0) {
                    if (kind != 1 || !mk_chess_has_live_effect(&piece->effects, kind)) {
                        g_active_piece = piece;
                        mk_chess_launch_special_fx(kind, 2, expiry);
                        g_active_piece = 0;
                    }
                }
            }
        }
        mk_chess_pdata->sides[side]->captured_piece_count = saved_team->captured_piece_count;
        for (index = 17 - mk_chess_pdata->sides[side]->captured_piece_count;
             index < 17; index++) {
            saved = &saved_team->pieces[index];
            mk_chess_create_piece(side, index, saved->id, 0, 0,
                                  saved->library_index, 0, 0.0f);
            mk_chess_set_piece_type_as(saved->type, 1);
            g_active_piece_being_defined->used_spells = saved->used_spells;
            mk_chess_restore_piece_event(saved);
            for (field = 0; field < 6; field++) {
                g_active_piece_being_defined->access_restrictions[field] =
                    saved->access_restrictions[field];
            }
        }
        mk_chess_pdata->sides[side]->saved_field_FC = saved_team->saved_field_FC;
        mk_chess_pdata->sides[side]->saved_field_100 = saved_team->saved_field_100;
        mk_chess_pdata->sides[side]->saved_field_110 = saved_team->saved_field_110;
        memcpy(mk_chess_pdata->sides[side]->strategy_state, saved_team->strategy_state, 0x18);
        memcpy(mk_chess_pdata->sides[side]->saved_state_114, saved_team->saved_state_114, 0x18);
        mk_chess_pdata->sides[side]->desired_x = saved_team->desired_x;
        mk_chess_pdata->sides[side]->desired_y = saved_team->desired_y;
    }
}

/* TODO: [breakthrough needed] 90.69%; resolve retail extrwi r0, r0, 1, 25 and its surrounding ownership/CFG before further tuning. */
void mk_chess_check_for_fatality(void) {
    if (ck_fatality_available() != 0 &&
        mk_chess_did_the_king_just_lose() != 0) {
        _create_mkproc_generic_tinystack(
            0xC01E, 0x1F, p_mk_chess_start_fatality, 0, 0);
        return;
    }

    end_music();
    turn_controllers_off();
    f_fatality_available = 0;
    f_fatality_finished = 1;
}


/* TODO: [near miss] 99.7619%; callback ABI corrected; relocation identity remains. */
static float p_mk_chess_slide_display_msg_handler(void) {
    ChessSlideMessagePdata* pdata = (ChessSlideMessagePdata*)apdata;

    _mkproc_sleep_ticks = (float)pdata->initial_delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    while ((pdata->step < 0 && pdata->object->x + pdata->step > pdata->target_x) ||
           (pdata->step > 0 && pdata->object->x + pdata->step < pdata->target_x)) {
        pdata->object->x += pdata->step;
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    pdata->object->x = pdata->target_x;
    pdata->completed();
    return -1.0f;
}

/* TODO: [near miss] 95.8%; object snapshots and signed clamp agree; stop at alpha-copy/loop coloring. */
static float p_mk_chess_fade_display_msg_handler(void) {
    ChessFadeMessagePdata* pdata = (ChessFadeMessagePdata*)apdata;
    int alpha;
    int vertex;
    ScreenObj* object;

    _mkproc_sleep_ticks = (float)pdata->initial_delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    alpha = pdata->object->pfx2d->verts[0].a;
    while (alpha > 0) {
        alpha = alpha < pdata->step ? 0 : alpha - pdata->step;
        object = pdata->object;
        for (vertex = 0; vertex < 4; vertex++) {
            object->pfx2d->verts[vertex].a = alpha;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    object = pdata->object;
    for (vertex = 0; vertex < 4; vertex++) {
        object->pfx2d->verts[vertex].a = 0;
    }
    if (pdata->object->instance != 0) {
        pdata->object->typed_vtbl->destroy(pdata->object);
    }
    return -1.0f;
}

/* TODO: [breakthrough needed] 96.09%; resolve retail extrwi. r0, r0, 1, 24 and its surrounding ownership/CFG before further tuning. */
void mk_chess_transition_from_fight(void) {
    int game_over;

    turn_controllers_off();
    mk_chess_set_game_mode(5);
    del_string_obj_by_id(0x2023);
    if (game_save_loop_count >= 8) {
        if ((g_game_info.field_04 & 0x80) == 0) {
            ck_do_profile_save();
        }
    } else {
        fade_to_black(8, 1);
    }

    game_over =
        ((board_game_save_data.ai_settings[1] & 0x40) != 0 &&
         g_game_info.plyr1.field_0C == 0.0f) ||
        ((board_game_save_data.ai_settings[0] & 0x80) != 0 &&
         g_game_info.plyr0.field_0C == 0.0f);

    if (game_over) {
        if (winner == 1) {
            board_game_save_data.winning_side = 0;
            board_game_save_data.sides[0].fight_stat_714++;
            board_game_save_data.sides[1].field_704 =
                board_game_save_data.sides[1].field_14 - 1;
            board_game_save_data.sides[0].field_704 =
                board_game_save_data.sides[0].field_14;
        } else {
            board_game_save_data.winning_side = 1;
            board_game_save_data.sides[1].fight_stat_714++;
            board_game_save_data.sides[0].field_704 =
                board_game_save_data.sides[0].field_14 - 1;
            board_game_save_data.sides[1].field_704 =
                board_game_save_data.sides[1].field_14;
        }

        if (p1_profile_status == 1) {
            if ((g_game_info.field_04 & 0x80) != 0) {
                if (winner == 1) {
                    p1_profile.online_wins++;
                } else {
                    p1_profile.online_losses++;
                }
            } else if (winner == 1) {
                if (g_game_info.plyr1.player_state == 0) {
                    p1_profile.local_ai_wins++;
                } else {
                    p1_profile.local_human_wins++;
                }
            } else if (g_game_info.plyr1.player_state == 0) {
                p1_profile.local_ai_losses++;
            } else {
                p1_profile.local_human_losses++;
            }

            if (p1_profile.online_wins > 999999) {
                p1_profile.online_wins = 999999;
            }
            if (p1_profile.online_losses > 999999) {
                p1_profile.online_losses = 999999;
            }
            if (p1_profile.local_ai_wins > 999999) {
                p1_profile.local_ai_wins = 999999;
            }
            if (p1_profile.local_human_wins > 999999) {
                p1_profile.local_human_wins = 999999;
            }
            if (p1_profile.local_ai_losses > 999999) {
                p1_profile.local_ai_losses = 999999;
            }
            if (p1_profile.local_human_losses > 999999) {
                p1_profile.local_human_losses = 999999;
            }
        }

        if (p2_profile_status == 1) {
            if ((g_game_info.field_04 & 0x80) != 0) {
                if (winner == 2) {
                    p2_profile.online_wins++;
                } else {
                    p2_profile.online_losses++;
                }
            } else if (winner == 2) {
                if (g_game_info.plyr0.player_state == 0) {
                    p2_profile.local_ai_wins++;
                } else {
                    p2_profile.local_human_wins++;
                }
            } else if (g_game_info.plyr0.player_state == 0) {
                p2_profile.local_ai_losses++;
            } else {
                p2_profile.local_human_losses++;
            }

            if (p2_profile.online_wins > 999999) {
                p2_profile.online_wins = 999999;
            }
            if (p2_profile.online_losses > 999999) {
                p2_profile.online_losses = 999999;
            }
            if (p2_profile.local_ai_wins > 999999) {
                p2_profile.local_ai_wins = 999999;
            }
            if (p2_profile.local_human_wins > 999999) {
                p2_profile.local_human_wins = 999999;
            }
            if (p2_profile.local_ai_losses > 999999) {
                p2_profile.local_ai_losses = 999999;
            }
            if (p2_profile.local_human_losses > 999999) {
                p2_profile.local_human_losses = 999999;
            }
        }

        if ((g_game_info.field_04 & 0x80) == 0) {
            ck_do_profile_save();
        }
        gamelogic_jump(5, p_mk_chess_game_over);
    }

    if (winner == 1) {
        if (g_game_info.plyr0.field_0C >
            board_game_save_data.player_health[0]) {
            g_game_info.plyr0.field_0C =
                board_game_save_data.player_health[0];
        }
    } else if (winner == 2 &&
               g_game_info.plyr1.field_0C >
                   board_game_save_data.player_health[1]) {
        g_game_info.plyr1.field_0C =
            board_game_save_data.player_health[1];
    }
    gamelogic_jump(5, p_mk_chess_continue);
}


/* TODO: [breakthrough needed] 72.03%; resolve retail addi r28, r4, g_chess_definition_info@l and its surrounding ownership/CFG before further tuning. */
static void mk_chess_set_default_chess_game(void) {
    ChessGameDefinition* definition;
    int character;
    int accepted;
    int attempts;
    unsigned int slot;
    unsigned int prior;

    definition = (ChessGameDefinition*)g_chess_definition_info;
    definition->background = 0x10;

    for (slot = 0; slot < 5; slot++) {
        accepted = 0;
        attempts = 50;
        while (!accepted) {
            character = available_chess_chars[randu0(26)];
            accepted = 1;
            for (prior = 0; prior < slot; prior++) {
                if (definition->side_0_characters[prior] == character) {
                    accepted = 0;
                }
            }
            if (is_char_locked(character, 0) != 0) {
                accepted = 0;
            }
            for (prior = 0; prior < 26; prior++) {
                if (available_chess_chars[prior] == character) {
                    break;
                }
            }
            if (prior == 26) {
                accepted = 0;
            }
            attempts--;
            if (attempts <= 0) {
                character = 0;
                accepted = 1;
            }
        }
        definition->side_0_characters[slot] = character;
    }

    for (slot = 0; slot < 5; slot++) {
        accepted = 0;
        attempts = 50;
        while (!accepted) {
            character = available_chess_chars[randu0(26)];
            accepted = 1;
            for (prior = 0; prior < slot; prior++) {
                if (definition->side_1_characters[prior] == character) {
                    accepted = 0;
                }
            }
            if (is_char_locked(character, 0) != 0) {
                accepted = 0;
            }
            for (prior = 0; prior < 26; prior++) {
                if (available_chess_chars[prior] == character) {
                    break;
                }
            }
            if (prior == 26) {
                accepted = 0;
            }
            attempts--;
            if (attempts <= 0) {
                character = 0;
                accepted = 1;
            }
        }
        definition->side_1_characters[slot] = character;
    }

    definition->side_0_type = 0;
    definition->side_1_type = 0;
    definition->enabled = 1;
}

static inline int mk_chess_demo_character_is_available(unsigned int character) {
    unsigned int index;

    for (index = 0; index < 26; index++) {
        if (character == (unsigned int)available_chess_chars[index]) {
            return 1;
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 76.68889%; boolean membership lowering recovered; base addressing and loop lifetimes remain. */
void mk_chess_set_default_chess_demo_game(void) {
    ChessGameDefinition* definition = (ChessGameDefinition*)g_chess_definition_info;
    unsigned int side;
    unsigned int slot;
    unsigned int prior;
    int* characters;
    int accepted;
    int attempts;

    int background = available_chess_bgnds[(unsigned short)randu0(5)];

    definition->teams[0].type = 1;
    definition->background = background;
    definition->teams[1].type = 1;
    for (side = 0; side < 2; side++) {
        characters = definition->teams[side].characters;
        for (slot = 0; slot < 5; slot++) {
            accepted = 0;
            attempts = 50;
            while (!accepted) {
                accepted = 1;
                characters[slot] = available_chess_chars[(unsigned short)randu0(26)];
                for (prior = 0; prior < slot; prior++) {
                    if (characters[prior] == characters[slot]) {
                        accepted = 0;
                    }
                }
                if (is_char_locked(characters[slot], 0) != 0) {
                    accepted = 0;
                }
                if (!mk_chess_demo_character_is_available(characters[slot])) {
                    accepted = 0;
                }
                attempts--;
                if (attempts <= 0) {
                    accepted = 1;
                    characters[slot] = 0;
                }
            }
        }
    }
    definition->enabled = 1;
}

/* TODO: [breakthrough needed] 80.74725%; team layout and runtime agree; resolve loop addressing and register lifetimes. */
void mk_chess_set_up_passed_in_chess_game(void) {
    ChessGameDefinition* definition = (ChessGameDefinition*)g_chess_definition_info;
    unsigned int side;
    unsigned int slot;
    unsigned int prior;
    int* characters;
    int accepted;
    int attempts;

    if ((unsigned int)definition->background >= 35) {
        definition->background = available_chess_bgnds[(unsigned short)randu0(5)];
    }
    for (side = 0; side < 2; side++) {
        if (definition->teams[side].type != 1) {
            continue;
        }
        characters = definition->teams[side].characters;
        for (slot = 0; slot < 5; slot++) {
            accepted = 0;
            attempts = 50;
            while (!accepted) {
                accepted = 1;
                characters[slot] = available_chess_chars[(unsigned short)randu0(26)];
                for (prior = 0; prior < slot; prior++) {
                    if (characters[prior] == characters[slot]) {
                        accepted = 0;
                    }
                }
                if (is_char_locked(characters[slot], 0) != 0) {
                    accepted = 0;
                }
                for (prior = 0; prior < 26; prior++) {
                    if ((unsigned int)characters[slot] == (unsigned int)available_chess_chars[prior]) {
                        break;
                    }
                }
                if (prior == 26) {
                    accepted = 0;
                }
                attempts--;
                if (attempts <= 0) {
                    accepted = 1;
                    characters[slot] = 0;
                }
            }
        }
    }
}

extern const MkFileEntry gameart_file_table[];
extern MkFileInfo sec_fightingart;
void display_load_meter(int section_slot);
static float p_mk_chess_init(void);

/* TODO: [near miss] 99.954956%; data-value exact; anonymous constant relocation identity remains. */
float p_mk_chess_continue(void) {
    MkProc* proc;
    unsigned int index;

    if (((g_game_info.field_04 >> 5) & 1) != 0) {
        push_game_state(3);
    } else {
        push_game_state(0x17);
    }
    G_BOARD_GAME_BIGSTACK_COUNTER = 0;
    turn_controllers_off();
    set_section_memory_scheme(3);
    set_mode_of_play(9);
    RwResourcesSetArenaSize(0x48000);
    display_load_meter(0x11005C);
    turn_camera_on();
    load_ssf((MkFileEntry*)gameart_file_table);
    load_art_section(0x10005, &sec_fightingart);
    if (((g_game_info.field_04 >> 7) & 1) != 0) {
        ck_for_controller_removed();
    }
    proc = _create_mkproc_generic_bigstack(
        0x2001, 0x23, p_mk_chess_init, sizeof(ChessModeState), (MkHdr**)&mk_chess_pdata);
    if (proc == 0) {
        gamelogic_jump(0, p_attract_mode);
    }
    zero_pdata_payload(sizeof(ChessModeState), &mk_chess_pdata->hdr);
    mk_chess_pdata->manager.input_state = 13;
    mk_chess_pdata->loaded_library_count = 0;
    mk_chess_pdata->sides[0] = (ChessSideState*)get_mkpdata_generic(sizeof(ChessSideState));
    mk_chess_pdata->sides[1] = (ChessSideState*)get_mkpdata_generic(sizeof(ChessSideState));
    mk_chess_pdata->board = (ChessBoardRow*)get_mem(0x1454);
    for (index = 0; index < 17; index++) {
        mk_chess_pdata->sides[0]->pieces[index] = (ChessPiece*)get_mem(sizeof(ChessPiece));
        mk_chess_pdata->sides[1]->pieces[index] = (ChessPiece*)get_mem(sizeof(ChessPiece));
    }
    if (mk_chess_pdata->sides[0] == 0 || mk_chess_pdata->sides[1] == 0) {
        gamelogic_jump(0, p_attract_mode);
    }
    set_process_as_scriptable(proc);
    return -1.0f;
}

/* TODO: [breakthrough needed] 76.78%; resolve retail stw r0, g_active_piece@sda21 and its surrounding ownership/CFG before further tuning. */
static float p_mk_chess_init(void) {
    ChessProcVtable* vtable;

    g_active_piece = 0;
    mk_chess_pdata->cursors[0].state = 0;
    mk_chess_pdata->cursors[0].selection = 0;
    mk_chess_pdata->cursors[1].state = 0;
    mk_chess_pdata->cursors[1].selection = 0;

    vtable = (ChessProcVtable*)aproc->vtbl;
    if (board_game_save_data.sides[0].restore_pending == 1) {
        vtable->jump_sleep(p_mk_chess_game_restore, 0.0f);
        return 0.0f;
    }
    if (g_chess_definition_info[2].field_04 == 0) {
        mk_chess_set_default_chess_game();
    }
    mk_chess_set_up_passed_in_chess_game();
    vtable->jump_sleep(p_mk_chess_game_setup, 0.0f);
    return 0.0f;
}


static void mk_chess_spell_hud_handle_names_slide_out(ChessSpellHudNames* names);
static void mk_chess_spell_hud_pick_a_spell_input(ChessHudState* hud);
static void mk_chess_spell_hud_pick_a_piece_for_rescue(ChessHudState* hud);
static void mk_chess_request_for_target(ChessHudState* hud);
unsigned int mk_chess_spell_hud_handle_bar_slide_out(ChessHudState* hud);
static float p_mk_chess_cast_spell(void);
static float p_mk_chess_spell_targetting_hud(void);

static inline MkHdr* mk_chess_live_spell_bar_cursor(void)
{
    ChessCursor* owner = &mk_chess_pdata->cursors[2];
    MkHdr* object = owner->object;
    if (object != 0) {
        if (object->instance == owner->object_instance) return object;
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

static inline void mk_chess_close_spell_bars_vertical(ChessHudState* hud, int next_state)
{
    ScreenObj* image;
    turn_controllers_off();
    hud->countdown += hud->field_14;
    image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_38);
    unhide_screen_obj(image);
    /* Retail closing motion is per tick, independent of game_speed. */
    image->y += 8;
    image->scale_y -= 0.07f;
    image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_28);
    unhide_screen_obj(image);
    image->y -= hud->countdown;
    image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_30);
    unhide_screen_obj(image);
    image->y += hud->countdown;
    if (image->y > screen_height / 2 - 32) {
        image->y = screen_height / 2 - 32;
        hide_screen_obj(mk_chess_live_screen(&mk_chess_pdata->manager.bar_38));
        hud->state = next_state;
        snd_req(0x397);
        if (hud->side == 1) {
            hud->countdown = (int)(12.0f * game_speed);
            hud->field_14 = (int)(2.0f * game_speed);
        } else {
            hud->countdown = (int)(-12.0f * game_speed);
            hud->field_14 = (int)(-2.0f * game_speed);
        }
    }
    hide_obj(mk_chess_live_spell_bar_cursor());
}

static inline int mk_chess_close_spell_bars_horizontal(ChessHudState* hud)
{
    ScreenObj* image;
    turn_controllers_off();
    hud->countdown += hud->field_14;
    image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_28);
    image->x += hud->countdown;
    image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_30);
    image->x += hud->countdown;
    if (image->x < -255 || image->x > screen_width) {
        hide_screen_obj(image);
        hide_screen_obj(mk_chess_live_screen(&mk_chess_pdata->manager.bar_28));
        hide_screen_obj(mk_chess_live_screen(&mk_chess_pdata->manager.bar_38));
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 99.83173%; data-value exact; relocation identity remains. */
static float p_mk_chess_spell_hud(void)
{
    ChessHudState* hud = (ChessHudState*)apdata;
    int delay = 0;
    ScreenObj* image;
    StringObj* string;
    int x;
    for (;;) {
        switch (hud->state) {
        case 0:
            turn_controllers_off();
            delay = mk_chess_spell_hud_handle_bar_slide_out(hud);
            break;
        case 1:
            turn_controllers_off();
            if (--delay == 0) {
                hud->state = 2;
                snd_req(0x395);
            }
            break;
        case 2:
            turn_controllers_off();
            hud->countdown += hud->field_14;
            image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_38);
            unhide_screen_obj(image);
            image->scale_y += 0.06f * game_speed;
            image->y -= (int)(8.0f * game_speed);
            image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_28);
            unhide_screen_obj(image);
            image->y -= hud->countdown;
            if (image->y > screen_height / 2 + 118)
                image->y = screen_height / 2 + 118;
            image = mk_chess_live_screen(&mk_chess_pdata->manager.bar_30);
            unhide_screen_obj(image);
            image->y += hud->countdown;
            if (image->y < screen_height / 2 - 150) {
                image->y = screen_height / 2 - 150;
                hud->state = 3;
            }
            break;
        case 3:
            turn_controllers_off();
            mk_chess_spell_hud_handle_names_slide_out((ChessSpellHudNames*)hud);
            break;
        case 4:
            mk_chess_spell_hud_pick_a_spell_input(hud);
            break;
        case 9:
            turn_controllers_off();
            x = 118;
            if (hud->side == 1) x = screen_width - 101;
            hud->strings = 0;
            string = string_center_xy(0xC01C, 8, get_string_by_id(0x2001E),
                x, screen_height / 2 + 87, 0x4E);
            string->pfx.instance0.native_color.r = 0x66;
            string->pfx.instance0.native_color.g = 0x33;
            string->pfx.instance0.native_color.b = 0;
            mk_insert((MkHdr*)string, &hud->strings);
            mk_chess_request_for_target(hud);
            turn_controllers_on();
            break;
        case 11:
            mk_chess_spell_hud_pick_a_piece_for_rescue(hud);
            break;
        case 6:
            mk_chess_close_spell_bars_vertical(hud, 7);
            break;
        case 7:
            if (mk_chess_close_spell_bars_horizontal(hud) == 1) {
                if (hud->field_54 != 0) {
                    hud->state = 20;
                    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_cast_spell, 0.0f);
                    return 0.0f;
                }
                turn_controllers_on();
                mk_chess_set_game_mode(0);
                return -1.0f;
            }
            break;
        case 15:
            mk_chess_close_spell_bars_vertical(hud, 16);
            break;
        case 16:
            if (mk_chess_close_spell_bars_horizontal(hud) == 1) {
                ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_spell_targetting_hud, 0.0f);
                return 0.0f;
            }
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
}

/* TODO: [breakthrough needed] 73.295456%; HUD argument restored; retail branch 0x22898 and owner layout remain unresolved. */
static void mk_chess_spell_hud_handle_names_slide_out(
    ChessSpellHudNames* names) {
    ScreenObj* cursor;
    int vertex;

    mk_chess_spell_hud_show_my_spells((ChessHudState*)names);
    names->state = 2;
    names->selected_name = 0;
    names->page = 4;
    turn_controllers_on();

    cursor = mk_chess_pdata->manager.hud_cursor;
    if (cursor != 0 &&
        (unsigned int)cursor->instance !=
            mk_chess_pdata->manager.hud_cursor_instance) {
        cursor = 0;
    }
    if (mk_chess_place_spell_hud_cursor_at_open_slot(
            cursor, names->side, names->cursor_slot) == 0) {
        hide_screen_obj(cursor);
        return;
    }

    for (vertex = 0; vertex < 4; vertex++) {
        cursor->pfx2d->verts[vertex].a = 0xFF;
    }
    unhide_screen_obj(cursor);
}

static inline void mk_chess_start_player_fatality(
    PlyrInfo* winner_info, PlyrInfo* loser_info) {
    MkProcEntryFn fatality;

    if (loser_info->slot.pdata->state != 0x4203) {
        return;
    }
    fatality = do_my_fatality;
    if (winner_info->slot.pdata->character_id == 0x1B &&
        winner_info->slot.pdata->sidekick_active != 0) {
        fatality = do_my_2nd_fatality;
    }
    xfer_proc(get_player_proc(winner_info->slot.mirror_a), fatality);
}

/* TODO: [breakthrough needed] 71.59%; resolve retail beq 0x109cc and its surrounding ownership/CFG before further tuning. */
static float p_mk_chess_start_fatality(void) {
    if (f_fatality_available != 0) {
        turn_controllers_off();
        _mkproc_sleep_ticks = 80.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        if (winner == 1) {
            mk_chess_start_player_fatality(
                &g_game_info.plyr0, &g_game_info.plyr1);
        } else {
            mk_chess_start_player_fatality(
                &g_game_info.plyr1, &g_game_info.plyr0);
        }
    }
    return 0.0f;
}

static void mk_chess_iterate_thru_move(ChessPiece* piece, ChessPieceMoveMap* map,
    ChessMovementSkill* skill, int x_step, int y_step);

static int mk_chess_calc_all_moves_cb(ChessPiece* piece,
    unsigned int context0, unsigned int context1, unsigned int context2) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPieceMoveMap* map = piece->move_map;
    unsigned int* restriction = piece == 0 ? 0 : &piece->access_restrictions[1];
    unsigned int index;
    ChessMovementSkill* skill;

    if (piece->flags.unknown_bit2) {
        return 0;
    }
    map->target_count = 0;
    for (index = 0; index < 10; index++) {
        map->rows[index] = 0;
    }
    if (*restriction >= (unsigned int)manager->clock) {
        return 0;
    }
    for (index = 0;
         index < g_board_game_controller.class_definitions[piece->type].movement_skill_count;
         index++) {
        skill = &g_board_game_controller.class_definitions[piece->type].movement_skills[index];
        switch (skill->move_type) {
        case 0:
            mk_chess_iterate_thru_move(piece, map, skill, 1, 0);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, -1, 0);
            }
            mk_chess_iterate_thru_move(piece, map, skill, 0, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, 0, -1);
            }
            break;
        case 1:
            mk_chess_iterate_thru_move(piece, map, skill, 1, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, 1, -1);
            }
            mk_chess_iterate_thru_move(piece, map, skill, -1, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, -1, -1);
            }
            break;
        }
    }
    return 0;
}

static inline void mk_chess_append_empty_move_end(ChessPiece* piece,
    ChessPieceMoveMap* map, int x, int y) {
    if (x != piece->cell_x || y != piece->cell_y) {
        map->targets[map->target_count].cell_x = x;
        map->targets[map->target_count].cell_y = y;
        map->targets[map->target_count].kind = 0;
        map->target_count++;
    }
}

/* TODO: [near miss] 83.81%; traversal and terminal paths agree; local scheduling/coloring remains. */
static void mk_chess_iterate_thru_move(ChessPiece* piece, ChessPieceMoveMap* map,
    ChessMovementSkill* skill, int x_step, int y_step) {
    unsigned int distance = skill->limits >> 16;
    unsigned int remaining = skill->limits & 0xFFFF;
    int last_x = piece->cell_x;
    int last_y = piece->cell_y;
    ChessBoardRow* board = mk_chess_pdata->board;
    int x = last_x + x_step * (int)distance;
    int y = last_y + y_step * (int)distance;

    while (x < 10 && x >= 0 && y < 10 && y >= 0 && remaining != 0) {
        if (board[x].cells[y].piece != 0) {
            ChessPiece* target = board[x].cells[y].piece;
            if (piece->side != target->side) {
                ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
                unsigned int* target_restriction = target == 0 ? 0 : &target->access_restrictions[2];
                unsigned int* source_restriction = piece == 0 ? 0 : &piece->access_restrictions[0];
                unsigned int clock = manager->clock;
                if (*target_restriction < clock && *source_restriction < clock) {
                    map->rows[y] &= ~(7U << (x * 3));
                    map->rows[y] |= 2U << (x * 3);
                    map->targets[map->target_count].cell_x = x;
                    map->targets[map->target_count].cell_y = y;
                    map->targets[map->target_count].kind = 1;
                    map->target_count++;
                    return;
                }
                mk_chess_append_empty_move_end(piece, map, last_x, last_y);
            } else {
                mk_chess_append_empty_move_end(piece, map, last_x, last_y);
            }
            return;
        }
        remaining--;
        map->rows[y] &= ~(7U << (x * 3));
        map->rows[y] |= 1U << (x * 3);
        last_x = x;
        last_y = y;
        x += x_step;
        y += y_step;
    }
    mk_chess_append_empty_move_end(piece, map, last_x, last_y);
}

void update_x_cursor_position(int* x, int* y, int x_step, int y_step,
    int min_y, int max_y, int wrap);
void update_y_cursor_position(int* x, int* y, int x_step, int y_step,
    int min_y, int max_y, int wrap);

void cam_calc_right_at_up_offsets(const Vec* position, float* forward_offset,
    float* right_offset, float* up_offset);

static inline unsigned char mk_chess_target_paper_alpha(ScreenObj* paper, int obscured)
{
    unsigned char alpha = paper->pfx2d->verts[0].a;
    if (obscured) {
        if (alpha > 50) alpha -= 10;
    } else if (alpha < 255) {
        /* Retail narrows before its redundant upper-bound test. */
        alpha += 5;
    }
    return alpha;
}

static inline void mk_chess_set_target_paper_alpha(ScreenObj* paper, unsigned char alpha)
{
    int vertex;
    for (vertex = 0; vertex < 4; vertex++) paper->pfx2d->verts[vertex].a = alpha;
}

static inline void mk_chess_move_spell_target(int* x, int* y, unsigned int direction)
{
    ChessModeState* mode;
    ChessCell* cell;
    MkObj* cursor;
    move_cursor_based_on_quadrant(mk_chess_pdata->manager.active_side, x, y, direction, 0, 10, 0);
    mode = mk_chess_pdata;
    cell = &mode->board[(unsigned char)*x].cells[(unsigned char)*y];
    cursor = (MkObj*)mk_chess_live_spell_bar_cursor();
    mode->cursors[2].cell_x = (unsigned char)*x;
    mode->cursors[2].cell_y = (unsigned char)*y;
    cursor->pos.value.x = cell->position.x;
    cursor->pos.value.y = cell->position.y;
    cursor->pos.value.z = cell->position.z;
    update_obj_pos(cursor);
    snd_req(0x36D);
}

/* TODO: [breakthrough needed] 86.942696%; retail calls restored; alpha/latch lowering and scheduling remain. */
static unsigned int mk_chess_spell_hud_choose_target_v2(ChessHudState* hud,
    int target, int* excluded_side)
{
    unsigned int result = 2;
    int x = mk_chess_pdata->cursors[2].cell_x;
    int y = mk_chess_pdata->cursors[2].cell_y;
    MkObj* cursor = (MkObj*)mk_chess_live_spell_bar_cursor();
    ScreenObj* paper;
    unsigned char alpha;
    float top_forward, top_right, top_up;
    float bottom_forward, bottom_right, bottom_up;
    cam_calc_right_at_up_offsets(&cursor->pos.value, &top_forward, &top_right, &top_up);
    paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[14]);
    alpha = mk_chess_target_paper_alpha(paper, top_up < -5.2f);
    mk_chess_set_target_paper_alpha(paper, alpha);
    paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[15]);
    mk_chess_set_target_paper_alpha(paper, alpha);
    cam_calc_right_at_up_offsets(&cursor->pos.value, &bottom_forward, &bottom_right, &bottom_up);
    paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[18]);
    alpha = mk_chess_target_paper_alpha(paper, bottom_up > 5.8f && __fabs(bottom_right) < 4.0);
    mk_chess_set_target_paper_alpha(paper, alpha);
    switch (hud->input_state) {
    case 1: mk_chess_move_spell_target(&x, &y, 4); break;
    case 7: mk_chess_move_spell_target(&x, &y, 5); break;
    case 5: mk_chess_move_spell_target(&x, &y, 3); break;
    case 2: mk_chess_move_spell_target(&x, &y, 0); break;
    case 8: mk_chess_move_spell_target(&x, &y, 7); break;
    case 6: mk_chess_move_spell_target(&x, &y, 1); break;
    case 4: mk_chess_move_spell_target(&x, &y, 2); break;
    case 3: mk_chess_move_spell_target(&x, &y, 6); break;
    case 9: {
        unsigned int cell_x = mk_chess_pdata->cursors[2].cell_x;
        unsigned int cell_y = mk_chess_pdata->cursors[2].cell_y;
        ChessPiece* piece;
        if (mk_chess_check_spell_target_rules((ChessSpellState*)hud,
                &mk_chess_pdata->board[cell_x].cells[cell_y], *excluded_side, cell_x, cell_y) == 0) {
            snd_req(0x370);
        } else {
            piece = mk_chess_pdata->board[mk_chess_pdata->cursors[2].cell_x]
                .cells[mk_chess_pdata->cursors[2].cell_y].piece;
            snd_req(0x393);
            if (piece != 0) *excluded_side = piece->side;
            hud->target_x[target] = mk_chess_pdata->cursors[2].cell_x;
            hud->target_y[target] = mk_chess_pdata->cursors[2].cell_y;
            result = 1;
        }
        break;
    }
    case 10:
        result = 0;
        turn_controllers_off();
        break;
    }
    hud->input_state = 0;
    return result;
}

/* TODO: [near miss] 61.14%; PPC32 checks agree; redundant nullable accessors and result joins differ. */
static int mk_chess_check_spell_target_rules(
    ChessSpellState* spell, ChessCell* cell, unsigned int excluded_side,
    unsigned int x, unsigned int y) {
    ChessPiece* piece;
    ChessManagerInfo* manager;
    unsigned int flags;

    if (cell->square_type == 1) {
        return 0;
    }
    piece = cell->piece;
    if (piece != 0) {
        flags = spell->caster->spells->target_access_flags[spell->spell_number];
        manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        if ((flags & 4) && piece->access_restrictions[4] >= (unsigned int)manager->clock) {
            return 0;
        }
        if ((flags & 1) && piece->access_restrictions[5] >= (unsigned int)manager->clock) {
            return 0;
        }
    }
    if (mk_chess_spell_hud_check_target_rules(spell, cell, excluded_side, x, y) == 0) {
        return 0;
    }
    return 1;
}

/* TODO: [near miss] 99.88%; data-value exact; anonymous switch-table relocation identity remains. */
static int move_cursor_based_on_quadrant(unsigned int side, int* x, int* y,
    unsigned int direction, int min_y, int max_y, int wrap) {
    int old_x = *x;
    int old_y = *y;
    int quadrant = mk_chess_pdata->camera.viewing_quadrant;

    if (mk_chess_pdata->sides[side]->controller->flags.drone_controlled == 1) {
        quadrant = 0;
    }
    switch (quadrant) {
    case 0:
        switch (direction) {
        case 2:
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 6:
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 4:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 0:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 5:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 3:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 7:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 1:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        }
        break;
    case 2:
        switch (direction) {
        case 6:
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 2:
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 0:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 4:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 5:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 3:
            update_y_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 7:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, 1, min_y, max_y, wrap);
            break;
        case 1:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        }
        break;
    case 1:
        switch (direction) {
        case 0:
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 4:
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 2:
            update_y_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 6:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 5:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 3:
            update_y_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 7:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 1:
            update_y_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        }
        break;
    case 3:
        switch (direction) {
        case 4:
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 0:
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 6:
            update_y_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 2:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            break;
        case 5:
            update_y_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 3:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, 1, -1, min_y, max_y, wrap);
            break;
        case 7:
            update_y_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        case 1:
            update_y_cursor_position(x, y, -1, 1, min_y, max_y, wrap);
            update_x_cursor_position(x, y, -1, -1, min_y, max_y, wrap);
            break;
        }
        break;
    }
    if ((old_x != *x) || (old_y != *y)) {
        return 1;
    }
    return 0;
}

/* TODO: [breakthrough needed] 97.92%; resolve retail instruction insertion/deletion and its surrounding ownership/CFG before further tuning. */
void update_x_cursor_position(
    int* x, int* y, int x_step, int y_step, int min_y, int max_y,
    int wrap) {
    *x += x_step;
    if (wrap == 1) {
        if (*x >= 10) {
            *x = 0;
            *y += y_step;
            if (*y >= max_y) {
                *y = min_y;
                return;
            }
            if (*y < min_y) {
                *y = max_y - 1;
            }
        } else if (*x < 0) {
            *x = 9;
            *y -= y_step;
            if (*y >= max_y) {
                *y = min_y;
                return;
            }
            if (*y < min_y) {
                *y = max_y - 1;
            }
        }
    } else if (*x >= 10 || *x < 0) {
        *x -= x_step;
    }
}

/* TODO: [breakthrough needed] 97.96%; resolve retail instruction insertion/deletion and its surrounding ownership/CFG before further tuning. */
void update_y_cursor_position(
    int* x, int* y, int x_step, int y_step, int min_y, int max_y,
    int wrap) {
    *y += y_step;
    if (wrap == 1) {
        if (*y >= max_y) {
            *y = min_y;
            *x += x_step;
            if (*x >= 10) {
                *x = 0;
                return;
            }
            if (*x < 0) {
                *x = 9;
            }
        } else if (*y < min_y) {
            *y = max_y - 1;
            *x -= x_step;
            if (*x >= 10) {
                *x = 0;
                return;
            }
            if (*x < 0) {
                *x = 9;
            }
        }
    } else if (*y >= max_y || *y < min_y) {
        *y -= y_step;
    }
}

/* TODO: [near miss] 99.50%; effect-name/handle ABI recovered; stop at object-load register coloring. */
void mk_chess_launch_fx_at_active_piece_with_offset(
    const char* effect_name, float x_offset, float y, float z_offset) {
    MkObj* object = g_active_piece->object;
    float x = object->pos.value.x + x_offset;
    float z = object->pos.value.z + z_offset;
    unsigned int effect = fx_by_owner(effect_name, 4);

    fx_set_param_v3(effect, 0x202, x, y, z);
    fx_reset(effect);
    fx_resume_emit(effect);
}

/* TODO: [near miss] 97.50%; data-value exact; anonymous relocation identity remains. */
float p_mk_chess_loop(void) {
    return 1.0f;
}


void mk_chess_disarmed_msg(void);

/* TODO: [near miss] 96.68421%; retail call boundaries restored; typed cell addressing and countdown scheduling remain. */
static void mk_chess_cell_end_of_turn(unsigned int x, unsigned int y) {
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    ChessPiece* piece;
    MkObj* object;
    unsigned char coordinates[2];
    int remaining;
    float dx;
    float dz;
    float distance_squared;

    if (cell->flags & 0x20) {
        return;
    }
    if (cell->square_type != 2) {
        return;
    }
    piece = cell->piece;
    if (piece != 0 && (float)piece->side != cell->saved_parameters[0]) {
        remaining = 240;
        while (--remaining > 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            object = piece->object;
            dz = cell->position.z - object->pos.value.z;
            dx = cell->position.x - object->pos.value.x;
            distance_squared = dx * dx + dz * dz;
            if (distance_squared < 0.2f) {
                break;
            }
            if (distance_squared < 0.8f) {
                remaining -= 5;
            }
        }
        if (cell->piece->type == 5) {
            mk_chess_disarmed_msg();
            coordinates[0] = cell->piece->cell_x;
            coordinates[1] = cell->piece->cell_y;
            mk_chess_game_event(6, &cell->piece, 1, coordinates);
            cell->square_type = 0;
        } else {
            board_game_save_data.sides[cell->piece->side].fight_stat_70C++;
            mk_chess_pdata->saved_field_118 = mk_chess_pdata->manager.clock;
            coordinates[0] = cell->piece->cell_x;
            coordinates[1] = cell->piece->cell_y;
            mk_chess_game_event(7, &cell->piece, 1, coordinates);
        }
    }
}

/* TODO: [breakthrough needed] 52.385246%; resolve retail lis r3, @stringBase0@ha and its surrounding ownership/CFG before further tuning. */
void mk_chess_disarmed_msg(void) {
    ChessScaleMessagePdata* pdata;
    MkHdr* raw_pdata = 0;
    ScreenObj* message;
    int center_x;
    int center_y;

    message =
        load_named_2d_pfxobj(0xD003C, 0xC01C, "DISARMED_MSG", 0, 0x52);
    center_x = screen_width / 2;
    center_y = screen_height / 2 + 50;

    _create_mkproc_generic_tinystack(
        0xC023, 0x1F, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &raw_pdata);
    pdata = (ChessScaleMessagePdata*)raw_pdata;
    pdata->object = message;
    unhide_screen_obj(message);
    pdata->step = 0.07666667f;
    pdata->start_scale = 0.25f;
    pdata->target_scale = 1.4f;
    pdata->initial_delay = 0;
    pdata->center_x = center_x;
    pdata->center_y = center_y;
    pdata->texture_width = message->pfx2d->tex_w;
    pdata->destroy_after_delay = 1;
    pdata->final_delay = 0x41;
    pdata->fade_after_midpoint = 0;

    message->scale_x = 0.25f;
    message->scale_y = 0.25f;
    message->x =
        -(int)(((float)(pdata->texture_width >> 1) *
                 message->scale_x) -
               (float)center_x);
    message->y =
        -(int)(((float)(message->pfx2d->tex_h / 2) *
                 message->scale_y) -
               (float)center_y);
    message->flags |= 8;
    snd_req(0x2C);
}

/* TODO: [breakthrough needed] 34.55%; retail vector math and quadrant tests still need reconstruction. */
static void mk_chess_set_up_zoom_cam(Vec* target);
static void mk_chess_set_up_zoom_cam_return(Vec* destination);
float p_mk_chess_cam_bezier_controller(void);

/* TODO: [breakthrough needed] 81.02734%; quadrant call restored; camera and timeout scheduling remain. */
static void mk_chess_monitor_cam_zoom_scenerios(ChessInputPdata* input) {
    ChessCameraInfo* info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int pad = g_game_info.plyr0.pad_index;
    if (input->side == 1) pad = g_game_info.plyr1.pad_index;
    switch (mk_chess_pdata->manager.input_state) {
    case 1:
    case 9:
    case 10:
        return;
    }
    if ((info->field_50 != 0 || info->field_4C != 0 || manager->input_state != 0) && info->field_4C != 0) {
        info->field_4C = 0;
        if (info->field_50 == 0) {
            CameraObj* camera = camera_live_node(&camera_item);
            Vec target = {0.0f, 0.0f, 0.0f};
            set_camera_position(&info->saved_position);
            if (camera != 0) mk_chess_set_viewing_quadrant(camera);
            look_at_target(&target);
            info->current_look_at.x = target.x;
            info->current_look_at.y = target.y;
            info->current_look_at.z = target.z;
            xfer_camera(p_mk_chess_cam_control, 0);
        }
    }
    if (info->field_50 == 0 && info->field_4C == 0) {
        if (!((float)mk_chess_pdata->turn_timeout < 660.0f * game_speed) && check_switch(pad, 1) != 0) {
            ChessManagerInfo* current = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
            CameraObj* camera = camera_live_node(&camera_item);
            ChessCursor* cursor = &mk_chess_pdata->cursors[current->active_side];
            ChessCameraInfo* completion_info;
            info->field_50 = 1;
            if (mk_chess_pdata->turn_timeout > 10) {
                mk_chess_pdata->turn_timeout = (unsigned int)-(5.0f * game_speed - (float)mk_chess_pdata->turn_timeout);
            }
            if (manager->input_state != 6) {
                info->saved_position.x = camera->pos.x;
                info->saved_position.y = camera->pos.y;
                info->saved_position.z = camera->pos.z;
            }
            mk_chess_set_up_zoom_cam(&mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece->object->pos.value);
            current = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
            cursor = &mk_chess_pdata->cursors[current->active_side];
            info->zoom_camera = mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece;
            info->zoom_sound_enabled = 1;
            completion_info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
            completion_info->completion = mk_chess_zoom_completed;
            mk_chess_set_game_mode(6);
            xfer_camera(p_mk_chess_cam_bezier_controller, 0);
        }
    } else if (info->field_50 != 0 && check_switch(pad, 1) == 0) {
        info->field_50 = 0;
        if (info->field_4C == 0) {
            ChessCameraInfo* completion_info;
            if (mk_chess_pdata->turn_timeout > 10) {
                mk_chess_pdata->turn_timeout = (unsigned int)-(5.0f * game_speed - (float)mk_chess_pdata->turn_timeout);
            }
            mk_chess_set_up_zoom_cam_return(&info->saved_position);
            info->zoom_sound_enabled = 1;
            completion_info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
            completion_info->completion = mk_chess_zoom_return_completed;
            mk_chess_set_game_mode(6);
            xfer_camera(p_mk_chess_cam_bezier_controller, 0);
        }
    }
}

void mk_chess_set_viewing_quadrant(CameraObj* camera) {
    RwFrame* frame = camera->frame;
    float camera_x = frame->modelling.at.x;
    float camera_z = frame->modelling.at.z;
    float facing_x = -frame->modelling.right.x;
    float facing_z = -frame->modelling.right.z;
    float left = -camera_z;

    if (left > 0.0f) {
        if (left > camera_x) {
            if (-facing_z > facing_x) {
                mk_chess_pdata->camera.viewing_quadrant = 1;
            } else {
                mk_chess_pdata->camera.viewing_quadrant = 0;
            }
        } else {
            mk_chess_pdata->camera.viewing_quadrant = 3;
        }
    } else if (fabs(left) > camera_x) {
        if (-facing_z > fabs(facing_x)) {
            mk_chess_pdata->camera.viewing_quadrant = 1;
        } else {
            mk_chess_pdata->camera.viewing_quadrant = 2;
        }
    } else {
        mk_chess_pdata->camera.viewing_quadrant = 3;
    }
}


/* TODO: [breakthrough] 73.9162%; piece/four-argument ABI restored; check direction lowering and register lifetimes. */
int mk_chess_cursor_square_move_next_step(
    int cell_x, int cell_y, ChessPiece* cursor,
    unsigned int current_track, int reverse, int use_x_axis,
    unsigned int primary_direction, int primary_track,
    unsigned int secondary_direction, int secondary_track,
    unsigned int tertiary_direction, int tertiary_track,
    int fallback_track, unsigned int* movement_state) {
    int delta_x = cursor->cell_x - cell_x;
    int delta_y = cursor->cell_y - cell_y;
    unsigned int distance =
        delta_y < 0 ? (unsigned int)-delta_y : (unsigned int)delta_y;
    int relative_direction = 0;
    int view = mk_chess_pdata->camera.viewing_quadrant;
    unsigned int negative;
    unsigned int axis;

    if (delta_x != 0) {
        if (delta_y == 0 ||
            distance <
                (unsigned int)(delta_x < 0 ? -delta_x : delta_x)) {
            distance =
                (unsigned int)(delta_x < 0 ? -delta_x : delta_x);
        }
    }

    if (use_x_axis != 0) {
        switch (view) {
        case 0:
            relative_direction = cell_x - cursor->cell_x;
            break;
        case 1:
            relative_direction = -(cell_y - cursor->cell_y);
            break;
        case 2:
            relative_direction = -(cell_x - cursor->cell_x);
            break;
        case 3:
            relative_direction = cell_y - cursor->cell_y;
            break;
        }
    } else {
        switch (view) {
        case 0:
            relative_direction = cell_y - cursor->cell_y;
            break;
        case 1:
            relative_direction = cell_x - cursor->cell_x;
            break;
        case 2:
            relative_direction = -(cell_y - cursor->cell_y);
            break;
        case 3:
            relative_direction = -(cell_x - cursor->cell_x);
            break;
        }
    }
    negative = (unsigned int)relative_direction >> 31;
    if (reverse != 0) {
        negative = negative == 0;
    }

    axis = *movement_state != 6 && *movement_state != 2;
    if (view == 3 || view == 1) {
        axis = axis == 0;
    }
    if (current_track == primary_direction) {
        if ((double)distance != 1.0) {
            return 0;
        }
        if (negative != 0) {
            mk_chess_cursor_go_to_new_track(
                cursor, axis, primary_track, secondary_direction);
        } else {
            mk_chess_cursor_go_to_new_track(
                cursor, axis, secondary_track, tertiary_direction);
        }
        return 1;
    }
    if (current_track == secondary_direction) {
        if (negative != 0 || (double)distance != 1.0) {
            return 0;
        }
        mk_chess_cursor_go_to_new_track(
            cursor, axis, tertiary_track, primary_direction);
        return 1;
    }
    if (current_track == tertiary_direction && negative != 0) {
        if ((double)distance != 1.0) {
            return 0;
        }
        mk_chess_cursor_go_to_new_track(
            cursor, axis, fallback_track, primary_direction);
        return 1;
    }
    return 0;
}


/* TODO: [near miss] 88.38%; visibility/emitter flow recovered; branch lowering, flags and register scheduling remain. */
static void mk_chess_cell_hide(unsigned int x, unsigned int y, int hidden) {
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    ChessCell* current;
    unsigned int emitter;
    MkPfx* particle;
    MkObj* object;
    float px;
    float py;
    float pz;

    if (hidden == 1 && !(cell->flags & 0x20)) {
        if (cell->object != 0) {
            hide_obj(cell->object);
        }
        if (cell->flags & 0x80) {
            fx_reset_emit(cell->emitter);
        }
        if (cell->flags & 0x40) {
            fx_reset_emit(cell->second_emitter);
        }
    } else if (hidden == 0 && (cell->flags & 0x20)) {
        if (cell->object != 0) {
            unhide_obj(cell->object);
        }
        current = &mk_chess_pdata->board[x].cells[y];
        if (current->square_type == 1) {
            current->emitter = fx_by_owner("health_effect", 4);
            current->emitter = fx_next_emitter(current->emitter);
            emitter = current->emitter;
            if (emitter != 0) {
                pz = current->position.z;
                py = current->position.y;
                px = current->position.x;
                fx_reset_emit(emitter);
                particle = pfx_from_emitter(emitter);
                if (particle != 0) {
                    object = pfx_bind_emitter_num_to_new_obj(
                        particle, 0x8227, emitter_id_from_handle(emitter));
                    if (object != 0) {
                        object->flags_08_bits.airborne = 1;
                        object->pos.value.x = px;
                        object->pos.value.y = py;
                        object->pos.value.z = pz;
                        update_mkobj(object);
                        fx_resume_emit(emitter);
                    }
                }
                current->flags |= 0x80;
            }
        }
    }
    cell->flags = (cell->flags & ~0x20) | ((hidden & 1) << 5);
}


MkObj* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z) {
    MkPfx* particle;
    MkObj* object;

    if (effect == 0) {
        return 0;
    }
    fx_reset_emit(effect);
    particle = pfx_from_emitter(effect);
    if (particle == 0) {
        return 0;
    }
    object = pfx_bind_emitter_num_to_new_obj(
        particle, 0x8227, emitter_id_from_handle(effect));
    if (object == 0) {
        return 0;
    }
    object->flags_08_bits.airborne = 1;
    object->pos.value.x = x;
    object->pos.value.y = y;
    object->pos.value.z = z;
    update_mkobj(object);
    fx_resume_emit(effect);
    return object;
}


/* TODO: [breakthrough needed] 98.65%; resolve retail mr r30, r0 and its surrounding ownership/CFG before further tuning. */
MkObj* launch_fx_at_pos_with_obj(
    unsigned int effect, float x, float y, float z) {
    MkPfx* particle;
    MkObj* object;

    particle = pfx_from_handle(effect);
    fx_reset(effect);
    if (particle == 0) {
        return 0;
    }
    object = (MkObj*)pfx_get_emitter_obj(particle, 0);
    if (object == 0) {
        object = pfx_bind_to_new_obj(particle, 0x8227);
    }
    if (object == 0) {
        return 0;
    }
    object->flags_08_bits.airborne = 1;
    object->pos.value.x = x;
    object->pos.value.y = y;
    object->pos.value.z = z;
    update_mkobj(object);
    fx_resume_emit(effect);
    return object;
}

extern int quad_tbl[8][2];

static void mk_chess_fetch_quadrant_params(
    int x, int y, unsigned int direction, int* first_x, int* first_y,
    int* end_x, int* end_y, unsigned int alternate) {
    int quadrant = mk_chess_pdata->camera.viewing_quadrant;
    unsigned int index;

    if (mk_chess_pdata->sides[mk_chess_pdata->manager.active_side]
            ->controller->flags.drone_controlled == 1) {
        quadrant = 0;
    }
    index = direction + quadrant * 2;
    if (index >= 8) {
        index -= 8;
    }
    switch (quad_tbl[index][alternate]) {
    case 1:
        *first_x = 0;
        *first_y = 0;
        *end_x = x;
        *end_y = y;
        break;
    case 2:
        *first_x = 0;
        *first_y = y + 1;
        *end_x = x;
        *end_y = 10;
        break;
    case 3:
        *first_x = x + 1;
        *first_y = y + 1;
        *end_x = 10;
        *end_y = 10;
        break;
    case 4:
        *first_x = x + 1;
        *first_y = 0;
        *end_x = 10;
        *end_y = y;
        break;
    }
    if (*first_x > 10) {
        *first_x = 10;
    }
    if (*first_y > 10) {
        *first_y = 10;
    }
    if (*end_x < 0) {
        *end_x = 0;
    }
    if (*end_y < 0) {
        *end_y = 0;
    }
}

static inline int mk_chess_hud_spell_available(ChessPiece* caster, unsigned int spell) {
    unsigned int used = caster->used_spells & (1U << spell);
    unsigned int* expires = mk_chess_piece_restriction(caster, 3);
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;

    if (used != 0) {
        return 0;
    }
    if (*expires >= (unsigned int)manager->clock) {
        return 0;
    }
    return 1;
}

/* TODO: [near miss] 98.83%; ordinal/access paths agree; local scheduling/coloring remains. */
void mk_chess_spell_hud_retract_all_for_targetting(ChessHudState* hud);

static inline ScreenObj* mk_chess_live_spell_cursor(void)
{
    ScreenObj* cursor = mk_chess_pdata->manager.hud_cursor;
    if (cursor != 0 && cursor->instance != mk_chess_pdata->manager.hud_cursor_instance) cursor = 0;
    return cursor;
}

static inline void mk_chess_start_pick_image_fade(ChessHudState* hud, float (*completed)(void))
{
    MkHdr* allocation;
    mk_insert((MkHdr*)mk_chess_live_spell_cursor(), &hud->images);
    if (_create_mkproc_generic_tinystack(0xC022, 31, p_mk_chess_fade_images,
            sizeof(ChessImageFadePdata), &allocation) != 0) {
        ChessImageFadePdata* fade = (ChessImageFadePdata*)allocation;
        fade->images = &hud->images;
        fade->completed = completed;
        fade->owner = hud;
    }
}

static inline int mk_chess_start_pick_string_fade(ChessHudState* hud,
    float step, float (*completed)(void))
{
    MkPtr* link = hud->strings;
    int has_strings = 0;
    MkHdr* allocation;
    while (link != 0) {
        StringObj* string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            string->pfx.instance0.native_color.a = 255;
            has_strings = 1;
            link = link->next;
        }
    }
    if (has_strings && _create_mkproc_generic_tinystack(0xC022, 31,
            p_mk_chess_spell_hud_string_fade, sizeof(ChessStringFadePdata), &allocation) != 0) {
        ChessStringFadePdata* fade = (ChessStringFadePdata*)allocation;
        fade->strings = &hud->strings;
        fade->completed = completed;
        fade->owner = hud;
        fade->step = (int)(step * game_speed);
        return 1;
    }
    return 0;
}

static void mk_chess_spell_targetting_display_hud(ChessHudState* hud, int hide);
static unsigned int mk_chess_spell_hud_choose_target_v2(ChessHudState* hud,
    int target, int* direction);

/* TODO: [breakthrough needed] 92.602646%; target/fade states recovered; list and allocation lowering remain. */
static float p_mk_chess_spell_targetting_hud(void)
{
    ChessHudState* hud = (ChessHudState*)apdata;
    int direction = 2;
    int finished = 0;
    MkHdr* initial_cursor;
    MkObj* cursor;
    ChessBoardRow* board;
    unsigned int result;
    hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
        .spell_definitions[hud->spell_number].target_rules[0];
    mk_chess_spell_targetting_display_hud(hud, 0);
    if (hud->target_rules & 8) {
        hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
            .spell_definitions[hud->spell_number].target_rules[1];
        mk_chess_spell_hud_show_page(hud, 1);
        hud->state = 18;
    } else {
        mk_chess_spell_hud_show_page(hud, 0);
        hud->state = 17;
    }
    initial_cursor = mk_chess_live_spell_bar_cursor();
    unhide_obj(initial_cursor);
    board = mk_chess_pdata->board;
    cursor = (MkObj*)mk_chess_live_spell_bar_cursor();
    mk_chess_pdata->cursors[2].cell_x = 5;
    mk_chess_pdata->cursors[2].cell_y = 5;
    cursor->pos.value.x = board[5].cells[5].position.x;
    cursor->pos.value.y = board[5].cells[5].position.y;
    cursor->pos.value.z = board[5].cells[5].position.z;
    update_obj_pos(cursor);
    turn_controllers_on();
    hud->field_54 = 0;
    while (!finished) {
        switch (hud->state) {
        case 17:
            result = mk_chess_spell_hud_choose_target_v2(hud, 0, &direction);
            if (result == 1) {
                hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
                    .spell_definitions[hud->spell_number].target_rules[1];
                if (hud->target_rules != 0) {
                    mk_chess_start_pick_string_fade(hud, -10.0f,
                        mk_chess_spell_hud_target_1_text_faded_goto_next_target_cb);
                    hud->state = 12;
                } else {
                    mk_chess_start_pick_string_fade(hud, -20.0f,
                        mk_chess_spell_hud_target_1_text_faded_exit_cb);
                    hud->field_54 = 1;
                    hud->state = 12;
                }
            } else if (result == 0) {
                mk_chess_start_pick_string_fade(hud, -20.0f,
                    mk_chess_spell_hud_target_1_text_faded_exit_cb);
                hud->state = 12;
            }
            break;
        case 18:
            result = mk_chess_spell_hud_choose_target_v2(hud, 1, &direction);
            if (result == 1) {
                hud->field_54 = 1;
                mk_chess_start_pick_string_fade(hud, -20.0f,
                    mk_chess_spell_hud_target_1_text_faded_exit_cb);
                hud->state = 12;
            } else if (result == 0) {
                mk_chess_start_pick_string_fade(hud, -20.0f,
                    mk_chess_spell_hud_target_1_text_faded_exit_cb);
                hud->state = 12;
            }
            break;
        case 19:
            finished = 1;
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    mk_chess_spell_targetting_display_hud(hud, 1);
    hide_obj(initial_cursor);
    mk_chess_pdata->sides[hud->side]->field_104 = 800;
    if (hud->field_54 != 0) {
        turn_controllers_off();
        hud->state = 20;
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_cast_spell, 0.0f);
        return 0.0f;
    }
    turn_controllers_on();
    mk_chess_set_game_mode(0);
    return -1.0f;
}

/* TODO: [breakthrough needed] 90.08709%; retail cursor calls restored; fade allocation and latch lowering remain. */
static void mk_chess_spell_hud_pick_a_spell_input(ChessHudState* hud)
{
    ScreenObj* cursor;
    int slot;
    switch (hud->input_state) {
    case 1:
        snd_req(0x392);
        cursor = mk_chess_live_spell_cursor();
        if (hud->cursor_slot != 0 && mk_chess_place_spell_hud_cursor_at_open_slot(
                cursor, hud->side, (unsigned int)hud->cursor_slot - 1) == 1) {
            hud->cursor_slot--;
        } else {
            for (slot = 7; slot >= 0; --slot) {
                if (mk_chess_place_spell_hud_cursor_at_open_slot(cursor, hud->side, slot) == 1) {
                    hud->cursor_slot = slot;
                    break;
                }
            }
            mk_chess_place_spell_hud_cursor_at_open_slot(cursor, hud->side, hud->cursor_slot);
        }
        break;
    case 2:
        snd_req(0x392);
        cursor = mk_chess_live_spell_cursor();
        if ((unsigned int)hud->cursor_slot + 1 < 8 && mk_chess_place_spell_hud_cursor_at_open_slot(
                cursor, hud->side, (unsigned int)hud->cursor_slot + 1) == 1) {
            hud->cursor_slot++;
        } else if (mk_chess_place_spell_hud_cursor_at_open_slot(cursor, hud->side, 0) == 1) {
            hud->cursor_slot = 0;
        }
        break;
    case 9:
        if (mk_chess_spell_hud_locate_spell_num((ChessSpellState*)hud, hud->side,
                hud->cursor_slot, 0, 0, 0) != 0) {
            unsigned char rules;
            hud->target_rules = g_board_game_controller.class_definitions[hud->caster->type]
                .spell_definitions[hud->spell_number].target_rules[0];
            rules = hud->target_rules;
            snd_req(0x393);
            if (rules & 8) {
                mk_chess_start_pick_image_fade(hud, p_mk_chess_names_retracted_move_up_selected_spell);
                mk_chess_start_pick_string_fade(hud, -25.0f, 0);
                hud->countdown = (int)(10.0f * game_speed);
                hud->state = 8;
            } else {
                hud->state = 14;
                mk_chess_spell_hud_retract_all_for_targetting(hud);
            }
            turn_controllers_off();
        }
        break;
    case 10:
        mk_chess_start_pick_image_fade(hud, 0);
        hud->countdown = (int)(10.0f * game_speed);
        hud->field_14 = 0;
        hud->state = 5;
        if (!mk_chess_start_pick_string_fade(hud, -20.0f, mk_chess_spell_hud_spell_text_faded_exit_cb)) {
            hud->state = 6;
            snd_req(0x396);
        }
        turn_controllers_off();
        break;
    }
    hud->input_state = 0;
}


static int mk_chess_place_spell_hud_cursor_at_open_slot(
    ScreenObj* cursor, int side, unsigned int slot) {
    int row = 0;
    unsigned int target;
    int spell;

    for (target = 0; target < 2; target++) {
        ChessPiece* caster = mk_chess_find_spellcaster_on_side(side, target);
        if (caster != 0) {
            for (spell = 0; spell < 4; spell++, row++) {
                if (mk_chess_hud_spell_available(caster, spell) == 1) {
                    if (slot-- == 0) {
                        cursor->y = 0x155 - row * 0x1C;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}


static int mk_chess_spell_hud_locate_spell_num(
    ChessSpellState* hud, unsigned int side, unsigned int slot,
    unsigned int target, unsigned int spell_number, int select) {
    unsigned int caster_index;
    unsigned int spell;

    for (caster_index = 0; caster_index < 2; caster_index++) {
        ChessPiece* caster = mk_chess_find_spellcaster_on_side(side, caster_index);
        if (caster == 0) {
            return 0;
        }
        for (spell = 0; spell < 4; spell++) {
            if (mk_chess_hud_spell_available(caster, spell) == 1) {
                if (slot-- == 0) {
                    if (select != 0) {
                        if (target == caster_index && spell_number == spell) {
                            return 1;
                        }
                        return 0;
                    }
                    hud->caster_index = caster_index;
                    hud->spell_number = spell;
                    hud->caster = caster;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* TODO: [near miss] 74.48%; ray and publication paths agree; cached-owner and register scheduling remains. */
static void mk_chess_cursor_go_to_new_track(
    ChessPiece* piece, unsigned int axis, unsigned int direction, unsigned int track) {
    ChessModeState* owner = mk_chess_pdata;
    ChessManagerInfo* manager = owner != 0 ? &owner->manager : 0;
    unsigned int distance;
    unsigned int step;
    int delta;
    int x;
    int y;
    int last_x;
    int last_y;
    ChessSideHudState* hud;

    if (axis == 0) {
        delta = piece->cell_y - mk_chess_pdata->cursor.cell_y;
    } else {
        delta = piece->cell_x - mk_chess_pdata->cursor.cell_x;
    }
    distance = delta < 0 ? -delta : delta;
    x = last_x = piece->cell_x;
    y = last_y = piece->cell_y;
    for (step = 0; step < distance; step++) {
        move_cursor_based_on_quadrant(manager->active_side, &x, &y, direction, 0, 10, 0);
        if (((piece->move_map->rows[y] >> (x * 3)) & 7) == 0) {
            if (mk_chess_pdata->cursor.cell_x == last_x &&
                mk_chess_pdata->cursor.cell_y == last_y) {
                return;
            }
            x = last_x;
            y = last_y;
            break;
        }
        last_x = x;
        last_y = y;
    }
    hud = mk_chess_pdata->sides[manager->active_side]->hud;
    hud->flags |= 0x80;
    hud->cell_x = x;
    hud->cell_y = y;
    mk_chess_pdata->cursors[0].cell_x = x;
    mk_chess_pdata->cursors[0].cell_y = y;
    mk_chess_pdata->cursors[1].cell_x = x;
    mk_chess_pdata->cursors[1].cell_y = y;
    mk_chess_pdata->cursor.cell_x = x;
    mk_chess_pdata->cursor.cell_y = y;
    owner->cursor_track = track;
}

/* TODO: [near miss] 86.55%; dispatch and publication agree; shared-join/owner scheduling remains. */
static void mk_chess_move_cursor_to_next_square_track_line(
    unsigned int side, unsigned int direction) {
    ChessModeState* owner = mk_chess_pdata;
    ChessManagerInfo* manager = owner != 0 ? &owner->manager : 0;
    int old_x = mk_chess_pdata->cursor.cell_x;
    int old_y = mk_chess_pdata->cursor.cell_y;
    ChessPiece* piece = manager->active_piece_by_side[side];
    int x = old_x;
    int y = old_y;
    unsigned int requested_direction = direction;
    int handled = 0;
    ChessSideHudState* hud;
    ChessPiece* selected;

    if (x == piece->cell_x && y == piece->cell_y) {
        owner->cursor_track = 0;
    }
    switch (direction) {
    case 4:
        handled = mk_chess_cursor_square_move_next_step(x, y, piece,
            owner->cursor_track, 0, 1, 1, 5, 3, 3, 4, 2, 6, &requested_direction);
        break;
    case 0:
        handled = mk_chess_cursor_square_move_next_step(x, y, piece,
            owner->cursor_track, 1, 1, 1, 1, 3, 7, 4, 6, 2, &requested_direction);
        break;
    case 6:
        handled = mk_chess_cursor_square_move_next_step(x, y, piece,
            owner->cursor_track, 0, 0, 2, 5, 3, 7, 4, 0, 4, &requested_direction);
        break;
    case 2:
        handled = mk_chess_cursor_square_move_next_step(x, y, piece,
            owner->cursor_track, 1, 0, 2, 1, 3, 3, 4, 4, 0, &requested_direction);
        break;
    }
    if (handled == 0) {
        move_cursor_based_on_quadrant(side, &x, &y, requested_direction, 0, 10, 0);
        if ((piece->cell_x == x && piece->cell_y == y) ||
            ((piece->move_map->rows[y] >> (x * 3)) & 7) != 0) {
            switch (requested_direction) {
            case 2:
            case 6:
                owner->cursor_track = 1;
                break;
            case 0:
            case 4:
                owner->cursor_track = 2;
                break;
            case 1:
            case 5:
                owner->cursor_track = 3;
                break;
            case 3:
            case 7:
                owner->cursor_track = 4;
                break;
            }
            hud = mk_chess_pdata->sides[manager->active_side]->hud;
            hud->flags |= 0x80;
            hud->cell_x = x;
            hud->cell_y = y;
            mk_chess_pdata->cursors[0].cell_x = x;
            mk_chess_pdata->cursors[0].cell_y = y;
            mk_chess_pdata->cursors[1].cell_x = x;
            mk_chess_pdata->cursors[1].cell_y = y;
            mk_chess_pdata->cursor.cell_x = x;
            mk_chess_pdata->cursor.cell_y = y;
        }
    }
    if (old_x != mk_chess_pdata->cursor.cell_x || old_y != mk_chess_pdata->cursor.cell_y) {
        snd_req(0x36D);
    }
    selected = mk_chess_pdata->board[mk_chess_pdata->cursor.cell_x]
        .cells[mk_chess_pdata->cursor.cell_y].piece;
    if (selected != 0) {
        mk_chess_hud_set_piece_portrait(selected);
    }
}

/* TODO: [near miss] 99.33673%; selector operations agree; anonymous relocation identity remains. */
float mk_chess_get_piece_event_data(int selector) {
    ChessPiece* other;

    switch (selector) {
    case 0:
        return g_active_piece->movement->event_value;
    case 1:
        return g_active_piece->movement->event_byte_12;
    case 2:
        return g_active_piece->movement->event_byte_13;
    case 3:
        return g_active_piece->movement->cell_x;
    case 4:
        return g_active_piece->movement->cell_y;
    case 5:
        return g_active_piece->movement->event_byte_14;
    case 6:
        return g_active_piece->movement->event_byte_15;
    case 7:
        other = mk_chess_pdata->manager.event_data.other_piece;
        if (other == g_active_piece) {
            other = mk_chess_pdata->manager.event_piece_24;
        }
        if (g_active_piece->type == 1 && other->type != 1) {
            if (g_active_piece->field_14 == other->field_14) {
                return 0.0f;
            }
            return 1.0f;
        }
        return 0.0f;
    default:
        return 1.0f;
    }
}

static inline float mk_chess_inverse_vector_length(float squared_length) {
    union { float f; unsigned int u; } input, estimate;
    float product;
    float correction;

    if (squared_length <= 0.0f) {
        return 0.0f;
    }
    input.f = squared_length;
    estimate.u = 0x5F375A00U - (input.u >> 1);
    product = estimate.f * (squared_length * estimate.f);
    correction = 3.0f - product;
    return (0.0625f * estimate.f) * correction *
        (12.0f - correction * (product * correction));
}

/* TODO: [near miss] 91.485435%; force/payload flow recovered; FP register scheduling and allocator-output reloads remain. */
void mk_chess_force_away(int delay, int frames, float speed, float damping) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPiece* other = manager->event_data.other_piece;
    MkHdr* allocation = 0;
    ChessForcePdata* force;
    MkObj* object;
    MkObj* other_object;
    float x;
    float z;
    float inverse_length;

    if (other == g_active_piece) {
        other = manager->event_piece_24;
    }
    g_active_piece->object->flags_08_bits.gravity_enabled = 1;
    other_object = other->object;
    object = g_active_piece->object;
    z = object->pos.value.z - other_object->pos.value.z;
    x = object->pos.value.x - other_object->pos.value.x;
    inverse_length = mk_chess_inverse_vector_length(x * x + z * z);
    x *= inverse_length;
    z *= inverse_length;
    object->pos_vel.x = x * speed;
    g_active_piece->object->pos_vel.z = z * speed;
    if (_create_mkproc_generic_tinystack(0xC01E, 10, p_mk_chess_apply_force,
                                      sizeof(ChessForcePdata), &allocation) != 0) {
        force = (ChessForcePdata*)allocation;
        force->object = g_active_piece->object;
        force->object_instance = g_active_piece->object->hdr.instance;
        force->delay = delay;
        force->damping = damping;
        force->frames = frames;
    }
}

static void mk_chess_classify_zoom_cam_pos_return(
    Vec* from, Vec* to, int* position_class, int* return_class);

void BezierCamera_Init(ChessBezierCameraState* camera, float step,
    const Vec* p0, const Vec* p1, const Vec* p2, const Vec* p3);
void BezierCamera_LinearAccelerateDeccelerate(ChessBezierCameraState* camera,
    float acceleration, float decel_step, float accel_end, float decel_start,
    float minimum_step, float maximum_step);
void BezierCamera_SetOverallCameraTimeInTicks(ChessBezierCameraState* camera, float ticks);
static int mk_chess_choose_middle_control_points_for_zoom_cam_return(
    int position_class, int return_class, Vec* from, Vec* middle_1, Vec* middle_2, Vec* to);

/* TODO: [near miss] 98.78906%; data-value exact; anonymous constant and switch-table relocation identity remains. */
static void mk_chess_set_up_zoom_cam_return(Vec* destination) {
    Vec from;
    Vec middle_1;
    Vec middle_2;
    Vec to;
    int position_class;
    int return_class;
    CameraObj* camera = camera_live_node(&camera_item);
    ChessCameraInfo* info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    int timing;

    from.x = camera->pos.x;
    from.y = camera->pos.y;
    from.z = camera->pos.z;
    to.x = destination->x;
    to.y = destination->y;
    to.z = destination->z;
    mk_chess_classify_zoom_cam_pos_return(&from, &to, &position_class, &return_class);
    timing = mk_chess_choose_middle_control_points_for_zoom_cam_return(
        position_class, return_class, &from, &middle_1, &middle_2, &to);
    info->desired_look_at.z = 0.0f;
    info->desired_look_at.y = 0.0f;
    info->desired_look_at.x = 0.0f;
    info->look_at_ticks = 40;
    BezierCamera_Init(&g_bezier_cam, 0.0f, &from, &middle_1, &middle_2, &to);
    switch (timing) {
    case 8:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00015f, -0.00005f, 0.7f, 0.72f, 0.0035f, 0.014f);
        break;
    case 9:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00085f, -0.00005f, 0.3f, 0.5f, 0.0035f, 0.025f);
        break;
    case 7:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.0005f, -0.0001f, 0.5f, 0.8f, 0.005f, 0.011f);
        break;
    case 6:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00025f, -0.0001f, 0.5f, 0.8f, 0.0035f, 0.008f);
        break;
    case 12:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 40.0f);
        break;
    case 11:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 70.0f);
        break;
    case 10:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 100.0f);
        break;
    }
}

/* TODO: [near miss] 95.77778%; polynomial and threshold agree; FP scheduling/register lifetimes remain. */
static void mk_chess_classify_zoom_cam_pos_return(
    Vec* from, Vec* to, int* position_class, int* return_class) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessCursor* cursor = &mk_chess_pdata->cursors[manager->active_side];
    MkObj* object = mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece->object;
    float object_z = object->pos.value.z;
    float object_x = object->pos.value.x;
    float to_z = to->z - object_z;
    float to_x = to->x - object_x;
    float inverse = mk_chess_inverse_vector_length(to_x * to_x + to_z * to_z);
    float from_z;
    float from_x;

    to_x *= inverse;
    to_z *= inverse;
    from_z = object_z - from->z;
    from_x = object_x - from->x;
    inverse = mk_chess_inverse_vector_length(from_x * from_x + from_z * from_z);
    from_z *= inverse;
    from_x *= inverse;
    *position_class = 1;
    if (from_x * to_x + from_z * to_z > 0.95f) {
        *position_class = 2;
    }
    *return_class = 4;
}

/* TODO: [near miss] 95.20%; effect cleanup agrees; latch/owner scheduling remains. */
static int mk_chess_hide_all_except_selected_pieces_cb(
    ChessPiece* piece, ChessPiece* selected, ChessPiece* other_selected) {
    MkPtr** effects;
    MkPtr* link;
    MkPtr* next;
    ChessPieceEffect* effect;
    TrackedSound* sound;
    MkObj* object;

    if (piece == selected) {
        return 0;
    }
    if (piece == other_selected) {
        return 0;
    }
    hide_obj(piece->object);
    effects = &piece->effects;
    if (effects != 0) {
        link = *effects;
        while (link != 0) {
            effect = (ChessPieceEffect*)link->hdr;
            if (link->instance != effect->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (effect->flags.bits.bit7) {
                    fx_pause_emit(effect->emitter);
                }
                if (effect->flags.bits.bit4) {
                    sound = effect->sound;
                    if (sound != 0 && sound->hdr.instance != effect->sound_instance) {
                        sound = 0;
                    }
                    if (sound != 0) {
                        stop_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
                        if (sound->hdr.instance != 0) {
                            sound->hdr.typed_vtbl->destroy(&sound->hdr);
                        }
                    }
                    effect->sound = 0;
                    effect->sound_instance = 0;
                }
                if (effect->flags.bits.bit6) {
                    object = mk_chess_live_imprison_object(effect);
                    if (object != 0) {
                        hide_obj(object);
                    }
                }
                link = link->next;
            }
        }
    }
    return 0;
}

typedef struct ChessGroundBeamPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    char pad10[0x0C];
    RwRGBA color; /* +0x1C */
    float fade_start;
    float age;
    float radial_start;
    float radial_limit;
    float radial_step;
    float vertical_start;
    float vertical_limit;
    float vertical_step;
    int alpha_step;
} ChessGroundBeamPdata; /* 0x44-byte process allocation */

/* TODO: [near miss] 93.76%; fade/growth operations agree; latch validation and FP scheduling remain. */
static float p_gnd_light_beam_fx(void) {
    ChessGroundBeamPdata* pdata = (ChessGroundBeamPdata*)apdata;
    MkObj* object;

    pdata->age += 1.0f;
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }
    if (pdata->age > pdata->fade_start) {
        if (pdata->color.alpha <= pdata->alpha_step) {
            if (object->hdr.instance != 0) {
                object->hdr.typed_vtbl->destroy(&object->hdr);
            }
            return -1.0f;
        }
        pdata->color.alpha -= pdata->alpha_step;
        obj_set_color_for_all_materials(object, &pdata->color);
    }
    if (pdata->age >= pdata->radial_start) {
        if (object->scale.x < pdata->radial_limit) {
            object->scale.x += pdata->radial_step;
            object->scale.z += pdata->radial_step;
            if (object->scale.x > pdata->radial_limit) {
                object->scale.x = pdata->radial_limit;
                object->scale.z = pdata->radial_limit;
            }
        }
    }
    if (pdata->age >= pdata->vertical_start) {
        if (object->scale.y < pdata->vertical_limit) {
            object->scale.y += pdata->vertical_step;
            if (object->scale.y > pdata->vertical_limit) {
                object->scale.y = pdata->vertical_limit;
            }
        }
    }
    return 1.0f;
}

/* TODO: [near miss] 91.47%; initialization/render paths agree; allocator-output reloads and scheduling remain. */
static void start_gnd_light_beam_effect(Vec* position, const char* model_name,
    int alpha_step, int reverse_scroll, float radial_limit, float radial_initial,
    float radial_step, float radial_start, float vertical_limit,
    float vertical_initial, float vertical_step, float vertical_start, float fade_start) {
    MkHdr* allocation;
    ChessGroundBeamPdata* pdata;
    MkObj* object = load_named_model_from_slot(0xD003C, model_name, 0xC01E, 0);
    MkSobj* sobj;

    if (object != 0 &&
        _create_mkproc_generic_tinystack(0xC023, 0x20, p_gnd_light_beam_fx,
            sizeof(ChessGroundBeamPdata), &allocation) != 0) {
        zero_pdata_payload(sizeof(ChessGroundBeamPdata), allocation);
        pdata = (ChessGroundBeamPdata*)allocation;
        pdata->object = object;
        pdata->object_instance = object->hdr.instance;
        pdata->radial_step = radial_step;
        pdata->radial_start = radial_start;
        pdata->radial_limit = radial_limit;
        pdata->vertical_limit = vertical_limit;
        pdata->vertical_step = vertical_step;
        pdata->vertical_start = vertical_start;
        pdata->fade_start = fade_start;
        pdata->alpha_step = alpha_step;
        pdata->color.alpha = 255;
        pdata->color.red = 255;
        pdata->color.blue = 255;
        pdata->color.green = 255;
        pdata->age = 0.0f;
        object->pos.value.x = position->x;
        object->pos.value.y = position->y;
        object->pos.value.z = position->z;
        object->flags_08_bits.scale_active = 1;
        object->scale.x = radial_initial;
        object->scale.y = vertical_initial;
        object->scale.z = radial_initial;
        if (reverse_scroll == 0) {
            obj_create_sobjs(object);
            object->light_flags = 0x10;
            sobj = obj_first_sobj(object);
            sobj->flags09_bits.bit7 = 1;
            sobj_set_priority(sobj, 0x13);
            object->light_flags = 0xD;
            object->flags_08_bits.airborne = 1;
            insert_fgnd_mkobj(object);
            sobj_start_uv_scroll(object, sobj, 0.0f, 0.02f, 0.0f, 0.0f);
        } else {
            obj_create_sobjs(object);
            sobj = obj_first_sobj(object);
            object->light_flags = 0xD;
            sobj_set_priority(sobj, 9);
            sobj->flags09_bits.bit6 = 1;
            sobj->flags09_bits.bit7 = 1;
            object->flags_08_bits.airborne = 1;
            insert_fgnd_mkobj(object);
            sobj_start_uv_scroll(object, sobj, 0.0f, -0.02f, 0.0f, 0.0f);
        }
        update_mkobj(object);
        insert_fgnd_mkobj(object);
    }
}

static float mk_chess_continue_pre_fight_chores(void);

#ifndef __MWERKS__
extern float fmaf(float, float, float);
#endif

static inline float mk_chess_camera_distance_squared(float x, float z) {
    /* This distance test is fused; the surrounding camera updates are not. */
#ifdef __MWERKS__
    return __fmadds(x, x, z * z);
#else
    return fmaf(x, x, z * z);
#endif
}

/* TODO: [near miss] 83.81%; fused/separate FP operations restored; stop at five cumulative attempts/reviews. */
#pragma fp_contract off
static float mk_chess_pre_fight_cam_ended(void) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPiece* other = manager->event_piece_24;
    ChessPiece* first = manager->event_data.other_piece;
    ChessCameraInfo* camera = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    unsigned int remaining = 99;
    Vec target;

    _create_mkproc_generic_tinystack(0xC01E, 0x1F,
        mk_chess_continue_pre_fight_chores, 0, 0);
    target = camera->desired_look_at;
    do {
        MkObj* other_object = other->object;
        MkObj* first_object = first->object;
        float first_x = first_object->pos.value.x;
        float first_z = first_object->pos.value.z;
        /* Retail rounds midpoint and settling updates before addition. */
        float dx = (0.5f * (other_object->pos.value.x - first_x) + first_x) - target.x;
        float dz = (0.5f * (other_object->pos.value.z - first_z) + first_z) - target.z;

        if (remaining < 70 && mk_chess_camera_distance_squared(dx, dz) < 0.003f) {
            break;
        }
        target.x += 0.15f * dx;
        target.z += 0.15f * dz;
        look_at_target(&target);
        add_camera_offsets();
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        remove_camera_offsets();
    } while (remaining-- != 0);
    return 0.0f;
}

#pragma fp_contract reset

extern int b_game_timer_off;
extern int force_bgnd_num;
void screen_engine_cleanup(void);
void reset_ani_data_space(void);
float p_gamelogic(void);
static float p_mk_chess_show_fight_message(void);
static void mk_chess_save_current_state(ChessPiece* piece, unsigned char x, unsigned char y);

static inline void mk_chess_wait_for_piece_idle(ChessPiece* piece) {
    unsigned int remaining;
    if (piece != 0) {
        remaining = 599;
        do {
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            if (piece->state == 0) {
                break;
            }
        } while (remaining-- != 0);
    }
}

static inline MkHdr* mk_chess_cursor_live_header(ChessCursor* cursor) {
    MkHdr* object = cursor->object;
    if (object != 0 && object->instance != cursor->object_instance) {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 92.88%; handoff/teardown agrees; latch and retained-owner scheduling remain. */
static float mk_chess_continue_pre_fight_chores(void) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPiece* other = manager->event_piece_24;
    ChessPiece* first = manager->event_data.other_piece;
    MkHdr* header;

    mk_chess_wait_for_piece_idle(other);
    mk_chess_wait_for_piece_idle(first);
    if ((other->type == 1 || first->type == 1) && other->type != first->type) {
        _mkproc_sleep_ticks = 80.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    mk_chess_set_game_mode(4);
    _create_mkproc_generic_tinystack(0x9030, 0x1F, p_mk_chess_show_fight_message, 0, 0);
    _mkproc_sleep_ticks = 20.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    b_game_timer_off = 0;
    fade_to_black(4, 1);
    screen_engine_cleanup();
    destroy_mkprocs_pid(0x9030);
    mk_chess_save_current_state(first, other->cell_x, other->cell_y);
    /* The handoff owns three live cursor allocations. */
    header = mk_chess_cursor_live_header(&mk_chess_pdata->cursors[0]);
    if (header->instance != 0) {
        header->typed_vtbl->destroy(header);
    }
    header = mk_chess_cursor_live_header(&mk_chess_pdata->cursors[1]);
    if (header->instance != 0) {
        header->typed_vtbl->destroy(header);
    }
    header = mk_chess_cursor_live_header(&mk_chess_pdata->cursor);
    if (header->instance != 0) {
        header->typed_vtbl->destroy(header);
    }
    destroy_mkprocs_pid(0xC01D);
    destroy_mkprocs_pid(0x5002);
    destroy_mkprocs_pid(0xC01E);
    destroy_fonts();
    cmdscript_unload(g_board_game_controller.command_script);
    reset_ani_data_space();
    if (mk_chess_pdata->camera_sound.field_00 != 0) {
        snd_stop(mk_chess_pdata->camera_sound.field_00);
        mk_chess_pdata->camera_sound.field_00 = 0;
    }
    header = &mk_chess_pdata->sides[0]->hdr;
    if (header->instance != 0) {
        header->typed_vtbl->destroy(header);
    }
    mk_chess_pdata->sides[0] = 0;
    header = &mk_chess_pdata->sides[1]->hdr;
    if (header->instance != 0) {
        header->typed_vtbl->destroy(header);
    }
    mk_chess_pdata->sides[1] = 0;
    if (mk_chess_pdata->hdr.instance != 0) {
        mk_chess_pdata->hdr.typed_vtbl->destroy(&mk_chess_pdata->hdr);
    }
    mk_chess_pdata = 0;
    force_bgnd_num = g_game_info.bgnd_id;
    RwResourcesSetArenaSize(0x48000);
    mode_of_play = 10;
    force_bgnd_num = g_game_info.bgnd_id;
    destroy_mkprocs_pid(0xC026);
    gamelogic_jump(2, p_gamelogic);
    return -1.0f;
}

static inline void mk_chess_center_fight_message(ScreenObj* message) {
    message->x = (int)-((float)(message->pfx2d->tex_w / 2) * message->scale_x -
        (float)(screen_width / 2));
    message->y = (int)-((float)(message->pfx2d->tex_h / 2) * message->scale_y -
        (float)(screen_height / 2));
}

/* TODO: [near miss] 91.99%; centering/scale operations agree; pointer lifetime and scheduling remain. */
static float p_mk_chess_show_fight_message(void) {
    ScreenObj* message = load_named_2d_pfxobj(0xD003C, 0xC01E, "DRAGON_LOGO", 0, 0x2D);
    ScreenObj* original;
    unsigned int instance;
    int tick;

    if (message == 0) {
        return -1.0f;
    }
    original = message;
    instance = message->instance;
    mk_insert((MkHdr*)message, &aproc->pdata_list_b);
    message->scale_x = 0.0f;
    message->scale_y = 0.0f;
    mk_chess_center_fight_message(message);
    message->flag_bits.scaled = 1;
    for (tick = 0; tick < 600; tick++) {
        message->scale_x += 0.09f * game_speed;
        message->scale_y += 0.09f * game_speed;
        mk_chess_center_fight_message(message);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        message = original;
        if (message != 0 && message->instance != instance) {
            message = 0;
        }
        if (message == 0) {
            return -1.0f;
        }
    }
    return -1.0f;
}

typedef struct ChessHandicapRange { int minimum, maximum; } ChessHandicapRange;
extern const ChessHandicapRange the_drone_master_handicap_levels[5][6];

static inline int mk_chess_save_handicap(ChessPiece* piece, ChessPiece* opponent) {
    int minimum = the_drone_master_handicap_levels[game_settings.arcade_difficulty][piece->type].minimum;
    int handicap = the_drone_master_handicap_levels[game_settings.arcade_difficulty][piece->type].maximum;
    unsigned int rating;

    if (get_game_state() != 3) {
        if ((unsigned int)mk_chess_pdata->manager.clock >= 15) {
            if ((unsigned int)mk_chess_pdata->manager.clock > 50) {
                handicap = minimum;
            } else {
                rating = mk_chess_drone_calc_fight_favorability_rating(piece, opponent);
                if (rating > 7) {
                    handicap = minimum;
                } else if (rating >= 3) {
                    handicap = minimum + (unsigned short)randu0((unsigned short)(handicap - minimum + 1));
                }
            }
        }
    }
    return handicap;
}

static inline void mk_chess_save_fighter_handicaps(ChessPiece* first, ChessPiece* second) {
    int handicap = mk_chess_save_handicap(first, second);
    board_game_save_data.ai_settings[0] = (board_game_save_data.ai_settings[0] & ~0x78U) |
        (((unsigned int)handicap << 3) & 0x78);
    handicap = mk_chess_save_handicap(second, first);
    board_game_save_data.ai_settings[1] = (board_game_save_data.ai_settings[1] & ~0x3CU) |
        (((unsigned int)handicap << 2) & 0x3C);
}

static inline void mk_chess_save_effect_list(MkPtr** effects, ChessSavedPiece* saved_piece) {
    MkPtr* link;
    MkPtr* next;
    ChessPieceEffect* effect;
    if (effects != 0) {
        link = *effects;
        while (link != 0) {
            effect = (ChessPieceEffect*)link->hdr;
            if (link->instance != effect->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                saved_piece->effects[saved_piece->effect_count].kind = effect->kind;
                saved_piece->effects[saved_piece->effect_count].expiry_clock = effect->expiry_clock;
                saved_piece->effect_count++;
                link = link->next;
            }
        }
    }
}

/* TODO: [near miss] 78.23%; full-buffer/runtime checks agree; owner reloads, loops and register scheduling remain. */
static void mk_chess_save_current_state(ChessPiece* attacker, unsigned char x, unsigned char y) {
    ChessCameraInfo* camera = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    ChessPiece* defender = mk_chess_pdata->board[x].cells[y].piece;
    unsigned int side, index, field, cell_x, cell_y;
    ChessSavedTeam* saved_team;
    ChessSavedPiece* saved_piece;
    ChessPiece* piece;

    board_game_save_data.restore_pending = 1;
    board_game_save_data.camera_position = camera->saved_position;
    for (side = 0; side < 2; side++) {
        saved_team = &board_game_save_data.teams[side];
        saved_team->live_piece_count = mk_chess_pdata->sides[side]->live_piece_count;
        saved_team->captured_piece_count = mk_chess_pdata->sides[side]->captured_piece_count;
        for (index = 0; index < 17; index++) {
            saved_piece = &saved_team->pieces[index];
            piece = mk_chess_pdata->sides[side]->pieces[index];
            saved_piece->id = piece->id;
            saved_piece->library_index = piece->library_index;
            saved_piece->type = piece->type;
            saved_piece->health = piece->health;
            saved_piece->cell_x = piece->cell_x;
            saved_piece->cell_y = piece->cell_y;
            saved_piece->used_spells = piece->used_spells;
            saved_piece->event_pending = 0;
            saved_piece->queued_event = piece->runtime.fields.queued_event;
            saved_piece->event_time = piece->runtime.fields.event_time;
            if (piece->flags.event_pending) {
                saved_piece->event_pending = 1;
            }
            for (field = 0; field < 6; field++) {
                saved_piece->access_restrictions[field] =
                    mk_chess_pdata->sides[side]->pieces[index]->access_restrictions[field];
            }
            saved_piece->effect_count = 0;
            if (index < mk_chess_pdata->sides[side]->live_piece_count) {
                piece = mk_chess_pdata->sides[side]->pieces[index];
                mk_chess_save_effect_list(&piece->effects, saved_piece);
            }
        }
        saved_team->saved_field_FC = mk_chess_pdata->sides[side]->saved_field_FC;
        saved_team->saved_field_100 = mk_chess_pdata->sides[side]->saved_field_100;
        saved_team->saved_field_110 = mk_chess_pdata->sides[side]->saved_field_110;
        memcpy(saved_team->strategy_state, mk_chess_pdata->sides[side]->strategy_state, 0x18);
        memcpy(saved_team->saved_state_114, mk_chess_pdata->sides[side]->saved_state_114, 0x18);
        saved_team->desired_x = mk_chess_pdata->sides[side]->desired_x;
        saved_team->desired_y = mk_chess_pdata->sides[side]->desired_y;
    }
    board_game_save_data.ai_settings_word = 0;
    board_game_save_data.active_side = mk_chess_pdata->manager.active_side;
    board_game_save_data.saved_field_110 = mk_chess_pdata->saved_field_110;
    board_game_save_data.saved_spell_clock = mk_chess_pdata->spell_completion_clock;
    board_game_save_data.saved_field_118 = mk_chess_pdata->saved_field_118;
    for (side = 0; side < 2; side++) {
        piece = mk_chess_pdata->manager.active_piece_by_side[side];
        if (piece != 0) {
            board_game_save_data.active_x[side] = piece->cell_x;
            board_game_save_data.active_y[side] = mk_chess_pdata->manager.active_piece_by_side[side]->cell_y;
        }
    }
    board_game_save_data.saved_clock = mk_chess_pdata->manager.clock;
    if (attacker->side == 0) {
        board_game_save_data.fighter_ids[0] = attacker->id;
        board_game_save_data.fighter_ids[1] = defender->id;
        board_game_save_data.player_health[0] = attacker->health;
        board_game_save_data.player_health[1] = defender->health;
        if (attacker->type == 5) {
            board_game_save_data.ai_settings[0] |= 0x80;
        }
        board_game_save_data.ai_settings_halves[0] = (board_game_save_data.ai_settings_halves[0] & ~0x780U) |
            (((unsigned int)attacker->type << 7) & 0x780);
        board_game_save_data.ai_settings_word = (board_game_save_data.ai_settings_word & ~0x3C000U) |
            (((unsigned int)defender->type << 14) & 0x3C000);
        if (defender->type == 5) {
            board_game_save_data.ai_settings[1] |= 0x40;
        }
        mk_chess_save_fighter_handicaps(attacker, defender);
    } else {
        board_game_save_data.fighter_ids[0] = defender->id;
        board_game_save_data.fighter_ids[1] = attacker->id;
        board_game_save_data.player_health[0] = defender->health;
        board_game_save_data.player_health[1] = attacker->health;
        if (attacker->type == 5) {
            board_game_save_data.ai_settings[1] |= 0x40;
        }
        board_game_save_data.ai_settings_word = (board_game_save_data.ai_settings_word & ~0x3C000U) |
            (((unsigned int)attacker->type << 14) & 0x3C000);
        board_game_save_data.ai_settings_halves[0] = (board_game_save_data.ai_settings_halves[0] & ~0x780U) |
            (((unsigned int)defender->type << 7) & 0x780);
        if (defender->type == 5) {
            board_game_save_data.ai_settings[0] |= 0x80;
        }
        mk_chess_save_fighter_handicaps(defender, attacker);
    }
    for (cell_x = 0; cell_x < 10; cell_x++) {
        for (cell_y = 0; cell_y < 10; cell_y++) {
            board_game_save_data.cells[cell_x][cell_y].type = mk_chess_pdata->board[cell_x].cells[cell_y].square_type;
            memcpy(&board_game_save_data.cells[cell_x][cell_y].x,
                mk_chess_pdata->board[cell_x].cells[cell_y].saved_parameters, 4 * sizeof(float));
        }
    }
    board_game_save_data.origin_x = attacker->cell_x;
    board_game_save_data.origin_y = attacker->cell_y;
    board_game_save_data.destination_x = x;
    board_game_save_data.destination_y = y;
}


static inline void mk_chess_wait_for_event_piece(ChessPiece** pieces) {
    ChessPiece* piece;
    unsigned int remaining = 599;
    _mkproc_sleep_ticks = 3.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    piece = pieces[0];
    if (piece != 0) {
        do {
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        } while (piece->state != 0 && remaining-- != 0);
    }
}

/* TODO: [near miss] 94.67857%; counted dispatch/payload contract recovered; wait-loop branches and register scheduling remain. */
void mk_chess_game_event(unsigned int event, ChessPiece** pieces,
                         unsigned int count, void* data) {
    switch (event) {
    case 1:
        if (count == 1) {
            mk_chess_piece_event(pieces[0], 0, 0);
        }
        break;
    case 2:
        if (count == 1) {
            mk_chess_piece_event(pieces[0], 1, 0);
        }
        break;
    case 3:
        if (count == 2) {
            mk_chess_piece_event(pieces[0], 2, 0);
            mk_chess_piece_event(pieces[1], 3, 0);
        }
        break;
    case 4:
        if (count == 1) {
            mk_chess_piece_event(pieces[0], 4, data);
        }
        break;
    case 5:
        if (count == 2) {
            mk_chess_piece_event(pieces[0], 5, data);
            mk_chess_piece_event(pieces[1], 7, data);
        }
        break;
    case 8:
        if (count == 2) {
            mk_chess_piece_event(pieces[0], 27, data);
            mk_chess_piece_event(pieces[1], 28, data);
        }
        break;
    case 6:
        if (count == 1) {
            mk_chess_piece_event(pieces[0], 14, data);
            mk_chess_wait_for_event_piece(pieces);
        }
        break;
    case 7:
        if (count == 1) {
            mk_chess_piece_event(pieces[0], 15, data);
            mk_chess_wait_for_event_piece(pieces);
        }
        break;
    }
}


/* TODO: [near miss] 84.545456%; mixed-precision thresholds and handoff agree; vector stores and FP scheduling differ. */
static float p_mk_chess_piece_constrain_to_cell(void) {
    ChessPiece* piece = g_active_piece;
    MkObj* object = piece->object;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    float target_z = cell->position.z + piece->runtime.fields.cell_offset.z;
    float target_x = cell->position.x + piece->runtime.fields.cell_offset.x;
    float dz = object->pos.value.z - target_z;
    float dx = object->pos.value.x - target_x;
    float frames = 300.0f;
    float squared_distance = dx * dx + dz * dz;
    float inverse_frames;

    if (squared_distance < 0.005f) {
        object->pos.value.x = target_x;
        g_active_piece->object->pos.value.z = target_z;
    } else {
        if ((double)squared_distance < 0.03) {
            frames = 120.0f;
        } else if (squared_distance < 0.1f) {
            frames = 200.0f;
        }
        inverse_frames = 1.0f / frames;
        dx *= inverse_frames;
        dz *= inverse_frames;
        while (frames-- > 0.0f) {
            g_active_piece->object->hide_flag_bits.pin_animation = 0;
            g_active_piece->object->pos.value.x -= dx;
            g_active_piece->object->pos.value.z -= dz;
            update_obj_pos(g_active_piece->object);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        }
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_piece_proc, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 91.76%; cursor/list/fade flow recovered; allocator reloads and list lowering remain. */
void mk_chess_spell_hud_retract_all_for_targetting(ChessHudState* hud) {
    ChessStringFadePdata* strings;
    ChessImageFadePdata* images;
    ScreenObj* cursor = mk_chess_pdata->manager.hud_cursor;
    MkPtr* link;
    int has_strings = 0;

    if (cursor != 0 && cursor->instance !=
        mk_chess_pdata->manager.hud_cursor_instance) {
        cursor = 0;
    }
    mk_insert((MkHdr*)cursor, &hud->images);
    link = hud->strings;
    while (link != 0) {
        StringObj* string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            string->pfx.instance0.native_color.a = 255;
            has_strings = 1;
            link = link->next;
        }
    }
    if (has_strings && _create_mkproc_generic_tinystack(
            0xC022, 0x1F, p_mk_chess_spell_hud_string_fade,
            sizeof(*strings), (MkHdr**)&strings)) {
        strings->strings = &hud->strings;
        strings->completed = 0;
        strings->owner = hud;
        strings->step = (int)(-20.0f * game_speed);
    }
    if (_create_mkproc_generic_tinystack(
            0xC022, 0x1F, p_mk_chess_fade_images,
            sizeof(*images), (MkHdr**)&images)) {
        images->images = &hud->images;
        images->completed = p_mk_chess_completed_all_image_retraction_for_targetting;
        images->owner = hud;
    }
    hud->state = 0xD;
    hud->countdown = (int)(10.0f * game_speed);
    hud->field_14 = 0;
    turn_controllers_off();
}

/* TODO: [near miss] 92.80%; retraction flow recovered; list guard and allocator-output reloads remain. */
void mk_chess_spell_hud_retract_all_during_deadpool_select(
    ChessHudState* hud, int targetting) {
    ChessStringFadePdata* strings;
    ScreenObj* cursor;
    MkPtr* link;
    int has_strings = 0;

    if (hud->image_58 != 0) {
        mk_insert(hud->image_58, &hud->images);
    }
    cursor = mk_chess_pdata->manager.hud_cursor;
    if (cursor != 0 && cursor->instance !=
        mk_chess_pdata->manager.hud_cursor_instance) {
        cursor = 0;
    }
    mk_insert((MkHdr*)cursor, &hud->images);
    if (targetting == 1) {
        ChessImageFadePdata* images;
        if (_create_mkproc_generic_tinystack(
                0xC022, 0x1F, p_mk_chess_fade_images,
                sizeof(*images), (MkHdr**)&images)) {
            images->images = &hud->images;
            images->completed = p_mk_chess_completed_all_image_retraction_for_targetting;
            images->owner = hud;
        }
        hud->state = 0xD;
    } else {
        ChessImageFadePdata* images;
        if (_create_mkproc_generic_tinystack(
                0xC022, 0x1F, p_mk_chess_fade_images,
                sizeof(*images), (MkHdr**)&images)) {
            images->images = &hud->images;
            images->completed = p_mk_chess_completed_all_image_retraction;
            images->owner = hud;
        }
        hud->state = 5;
    }
    hud->countdown = (int)(10.0f * game_speed);
    hud->field_14 = 0;
    link = hud->strings;
    while (link != 0) {
        StringObj* string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            string->pfx.instance0.native_color.a = 255;
            has_strings = 1;
            link = link->next;
        }
    }
    if (has_strings && _create_mkproc_generic_tinystack(
            0xC022, 0x1F, p_mk_chess_spell_hud_string_fade,
            sizeof(*strings), (MkHdr**)&strings)) {
        strings->strings = &hud->strings;
        strings->completed = 0;
        strings->owner = hud;
        strings->step = (int)(-25.0f * game_speed);
    }
}

static inline ScreenObj* mk_chess_latched_screen(ChessScreenRef* ref) {
    ScreenObj* screen = ref->screen;
    if (screen != 0 && screen->instance != ref->instance) {
        screen = 0;
    }
    return screen;
}

/* TODO: [near miss] 92.11%; motion/clamps and completion recovered; latch branch lowering remains. */
unsigned int mk_chess_spell_hud_handle_bar_slide_out(ChessHudState* hud) {
    int direction = 1;
    int finished = 0;
    ScreenObj* screen = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_28);

    if (hud->side != 0) {
        direction = -1;
    }
    hud->countdown += hud->field_14;
    unhide_screen_obj(screen);
    screen->x += hud->countdown;
    if (screen->x > -35 && direction == 1) {
        screen->x = -35;
    } else if (screen->x < screen_width - 255 && direction == -1) {
        screen->x = screen_width - 255;
    }
    screen = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_30);
    unhide_screen_obj(screen);
    screen->x += hud->countdown;
    if (screen->x > -35 && direction == 1) {
        finished = 1;
        screen->x = -35;
    } else if (screen->x < screen_width - 255 && direction == -1) {
        screen->x = screen_width - 255;
        finished = 1;
    }
    if (finished) {
        int x = screen->x;
        hud->state = 1;
        hud->countdown = (int)(-10.0f * game_speed);
        hud->field_14 = 0;
        screen = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_38);
        unhide_screen_obj(screen);
        screen->scale_y = 0.25f;
        screen->y = ((screen_height / 2) * 2 - 32) / 2;
        screen->x = x;
        return (unsigned int)(6.0f * inverse_game_speed);
    }
    return 0;
}

/* TODO: [near miss] 90.98%; typed page/text flow recovered; list lowering and allocation reloads remain. */
static void mk_chess_spell_hud_show_page(ChessHudState* hud, int forward) {
    float height = get_font_height(6);
    const ChessSpellPageText* page = hud->caster->spells->pages[hud->spell_number][forward];
    StringObj* string;
    MkPtr* link;
    int has_strings = 0;
    ChessStringFadePdata* fade;

    hud->strings = 0;
    string = string_center_xy(0xC01C, 8,
        get_string_by_id(page->line_ids[0] | 0x20000),
        screen_width / 2, (int)(2.0f * height + 15.0f), 0x4E);
    string->pfx.instance0.native_color.r = 0x66;
    string->pfx.instance0.native_color.g = 0x33;
    string->pfx.instance0.native_color.b = 0;
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_center_xy(0xC01C, 8,
        get_string_by_id(page->line_ids[1] | 0x20000),
        screen_width / 2, (int)(15.0f + height), 0x4E);
    string->pfx.instance0.native_color.r = 0x66;
    string->pfx.instance0.native_color.g = 0x33;
    string->pfx.instance0.native_color.b = 0;
    mk_insert((MkHdr*)string, &hud->strings);
    link = hud->strings;
    while (link != 0) {
        string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            string->pfx.instance0.native_color.a = 0;
            has_strings = 1;
            link = link->next;
        }
    }
    if (has_strings && _create_mkproc_generic_tinystack(
            0xC022, 0x1F, p_mk_chess_spell_hud_string_fade,
            sizeof(*fade), (MkHdr**)&fade)) {
        fade->strings = &hud->strings;
        fade->completed = 0;
        fade->owner = hud;
        fade->step = (int)(10.0f * game_speed);
    }
}


/* TODO: [near miss] 92.29%; retail call boundary restored; bounded-copy indexing and owner scheduling remain. */
void mk_chess_set_piece_type_as(int type, int restoring) {
    ChessClassDefinition* definition;
    AniScript* stance;

    if (g_active_piece_being_defined == 0) {
        return;
    }
    mk_chess_register_name_and_portrait_in_team(
        g_active_piece_being_defined->side, type,
        &g_board_game_controller.piece_libraries[g_active_piece_being_defined->library_index]);
    g_active_piece_being_defined->type = type;
    stance = (AniScript*)g_board_game_controller.class_definitions[
        g_active_piece_being_defined->type].event_scripts[0];
    if ((unsigned int)stance == 0xABABAB00) {
        stance = g_board_game_controller.piece_art_rows[0];
    }
    if (g_active_piece_being_defined != 0 || g_active_piece != 0) {
        if (g_active_piece != 0) {
            g_active_piece->normal_stance_script = stance;
        } else {
            g_active_piece_being_defined->normal_stance_script = stance;
            g_active_piece_being_defined->initial_stance_script = stance;
        }
    }
    definition = &g_board_game_controller.class_definitions[type];
    if (definition->flags.spellcaster) {
        ChessPieceSpellData* spells = (ChessPieceSpellData*)get_mkpdata_generic(sizeof(*spells));
        int i;
        for (i = 0; i < 4; i++) {
            if (!definition->spell_definitions[i].enabled) {
                break;
            }
            spells->target_access_flags[i] = definition->spell_definitions[i].target_access_flags;
            spells->pages[i][0] = definition->spell_definitions[i].pages[0];
            spells->pages[i][1] = definition->spell_definitions[i].pages[1];
        }
        g_active_piece_being_defined->spells = spells;
    }
    if (restoring == 0) {
        g_active_piece_being_defined->health = g_board_game_controller.class_definitions[type].initial_power;
    }
}


/* TODO: [breakthrough needed] 57.92%; retail call boundary restored; caller frame and latch structure remain. */
void mk_chess_register_name_and_portrait_in_team(
    unsigned int side_index,
    unsigned int portrait_index,
    ChessLibraryEntry* library) {
    ChessSideState* side;
    ScreenObj* portrait;
    unsigned int flags;

    flags = side_index == 1 ? 0x20000000 : 0;
    side = mk_chess_pdata->sides[side_index];
    portrait = side->portraits[portrait_index].screen;
    if (portrait != 0 &&
        portrait->instance !=
            side->portraits[portrait_index].instance) {
        portrait = 0;
    }

    if (portrait == 0) {
        portrait =
            mk_chess_create_portrait_from_library(library, flags);
        side->portraits[portrait_index].screen = portrait;
        side->portraits[portrait_index].instance =
            portrait->instance;
        hide_screen_obj(portrait);
    }
}

/* TODO: [near miss] 89.30%; lookup accesses agree; string-pool addressing remains. */
static ScreenObj* mk_chess_create_portrait_from_library(
    ChessLibraryEntry* library, unsigned int flags) {
    char name[24];
    unsigned int index = 0;
    int slot;

    for (; index < mk_chess_pdata->loaded_library_count; index++) {
        if (mk_chess_pdata->loaded_character_ids[index] == library->character_id) {
            strcpy(name, "PORT_");
            strcat(name, library->name);
            slot = index >= 10 ? 0x100052 : piece_model_ss_tbl[index];
            return load_named_2d_pfxobj(slot, 0xC01C, name, flags, 0x51);
        }
    }
    return 0;
}

void mk_chess_create_piece_obj(ChessPiece* piece, unsigned char side, ChessLibraryEntry* library);

void mk_chess_shifter_switch(int animation_index, int flags, float speed, float frame) {
    ChessPiece* other = mk_chess_pdata->manager.event_data.other_piece;
    MkObj* object;
    ChessAnimPdata* animation;
    float x, y, z, ax, ay, az;

    if (other == g_active_piece) {
        other = mk_chess_pdata->manager.event_piece_24;
    }
    hide_obj(g_active_piece->object);
    object = g_active_piece->object;
    x = object->pos.value.x;
    y = object->pos.value.y;
    z = object->pos.value.z;
    ax = object->ang.x;
    ay = object->ang.y;
    az = object->ang.z;
    mk_chess_create_piece_obj(g_active_piece, g_active_piece->side,
        &g_board_game_controller.piece_libraries[other->library_index]);
    g_active_piece->animation->object = g_active_piece->object;
    g_active_piece->animation->object_instance = g_active_piece->object->hdr.instance;
    animation = g_active_piece->animation;
    g_active_piece->flags.glitch_into_stance = 0;
    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    animation->speed = speed;
    set_anim_script_frame(frame, (AnimPdata*)animation,
        (AniData*)mkc_animations[animation_index], flags);
    g_active_piece->object->pos.value.x = x;
    g_active_piece->object->pos.value.y = y;
    g_active_piece->object->pos.value.z = z;
    g_active_piece->object->ang.x = ax;
    g_active_piece->object->ang.y = ay;
    g_active_piece->object->ang.z = az;
    object = g_active_piece->object;
    update_mkobj(object != 0 ? as_mkhdr(&object->hdr) : 0);
}

/* TODO: [near miss] 93.97727%; allocation/placement recovered; board lowering and owner scheduling remain. */
static void mk_chess_create_piece(
    unsigned char side, int slot, int class_slot, unsigned char x,
    unsigned char y, int character, int active, float health) {
    ChessLibraryEntry* library = &g_board_game_controller.piece_libraries[character];
    ChessPiece* piece;
    int i;

    if (side >= 2 || (unsigned int)class_slot >= 16 || (unsigned int)slot >= 17) {
        return;
    }
    if (strlen(library->name) >= 12) {
        return;
    }
    piece = mk_chess_pdata->sides[side]->pieces[slot];
    piece->effects = 0;
    piece->runtime.fields.timer_0 = 0;
    piece->flags_word = 0;
    piece->state = 0;
    piece->id = class_slot;
    piece->field_14 = library->character_id;
    piece->health = health;
    piece->side = side;
    piece->type = 0;
    piece->current_event = 0x1D;
    piece->library_index = character;
    piece->spells = 0;
    piece->used_spells = 0;
    piece->move_map = (ChessPieceMoveMap*)get_mkpdata_generic(sizeof(*piece->move_map));
    for (i = 0; i < 6; i++) {
        piece->access_restrictions[i] = 0;
    }
    piece->runtime.fields.cell_offset.z = 0.0f;
    piece->runtime.fields.cell_offset.y = 0.0f;
    piece->runtime.fields.cell_offset.x = 0.0f;
    piece->field_64 = 0;
    mk_chess_create_piece_obj(piece, side, library);
    if (active == 1) {
        ChessCell* cell;
        piece->cell_x = x;
        piece->cell_y = y;
        x = piece->cell_x;
        y = piece->cell_y;
        cell = &mk_chess_pdata->board[x].cells[y];
        if (cell->piece == piece) {
            cell->piece = 0;
        }
        cell->piece = piece;
        piece->cell_x = x;
        piece->cell_y = y;
        piece->object->hide_flag_bits.pin_animation = 0;
        piece->object->pos.value.x = cell->position.x + piece->runtime.fields.cell_offset.x;
        piece->object->pos.value.z = cell->position.z + piece->runtime.fields.cell_offset.z;
        update_obj_pos(piece->object);
        update_mkobj(&piece->object->hdr);
    } else {
        piece->object->pos.value.z = 0.0f;
        piece->object->pos.value.y = 0.0f;
        piece->object->pos.value.x = 0.0f;
        update_mkobj(&piece->object->hdr);
        hide_obj(piece->object);
    }
    piece->animation = (ChessAnimPdata*)get_mkpdata_anim();
    piece->animation->object = piece->object;
    piece->animation->object_instance = piece->object->hdr.instance;
    set_root_and_obj_movement_weights(0.0f, 1.0f, (AnimPdata*)piece->animation);
    set_anim_script_frame((float)(unsigned short)randu0(45),
        (AnimPdata*)piece->animation, (AniData*)mkc_animations[0], 0);
    piece->animation->speed = 1.0f;
    piece->proc_state = 0;
    piece->movement = (ChessPieceMovement*)get_mkpdata_generic(sizeof(*piece->movement));
    piece->movement->animation = piece->animation;
    piece->movement->piece = piece;
    g_active_piece_being_defined = piece;
}

/* TODO: [near miss] 99.72222%; data-value exact; anonymous relocation identity remains. */
void mk_chess_create_piece_obj(
    ChessPiece* piece,
    unsigned char side,
    ChessLibraryEntry* library) {
    piece->object = mk_chess_create_piece_model_from_library(library);
    piece->object->light_flags = 0x200;
    piece->object->flags_09_bits.launched = 1;
    piece->object->flags_09_bits.bit6 = 1;
    piece->object->ground_colls = mk_chess_piece_ground_colls;
    obj_set_z_offsets(25.0f, piece->object);
    obj_create_sobjs(piece->object);
    bgnd_make_mkobj_transl(piece->object);
    piece->object->flags_08_bits.scale_active = 1;
    piece->object->scale.z = 2.2f;
    piece->object->scale.y = 2.2f;
    piece->object->scale.x = 2.2f;
    piece->object->flags_08_bits.airborne = 1;
    piece->object->ang.y = 3.1415927f * side;
    build_bones_tbl(piece->object, mk_chess_piece_bones);
    insert_fgnd_mkobj(piece->object);
    insert_ground_me_mkobj(piece->object);
}

char* strlwr(char* string);

static inline MkObj* mk_chess_find_loaded_piece_model(ChessLibraryEntry* library) {
    unsigned int index;
    int slot;
    for (index = 0; index < mk_chess_pdata->loaded_library_count; index++) {
        if (mk_chess_pdata->loaded_character_ids[index] == library->character_id) {
            slot = index >= 10 ? 0x100052 : piece_model_ss_tbl[index];
            return load_named_model_from_slot(slot, library->name, 0xA00A, 0);
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 73.31214%; 19-reference layout recovered; dispatch merging and string-pool lowering remain. */
void mk_chess_load_board_spell_hud(void) {
    unsigned int index;
    ScreenObj* object;

    for (index = 0; index < 19; index++) {
        mk_chess_pdata->manager.spell_hud[index].screen = 0;
        mk_chess_pdata->manager.spell_hud[index].instance = 0;
        switch (index) {
        case 0:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELL_LIST_TOP", 0, 0x56);
            break;
        case 1:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELL_LIST_BOTTOM", 0, 0x56);
            break;
        case 2:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELL_LIST_BGND", 0, 0x57);
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELL_LIST_GREYED_OUT", 0, 0x53);
            break;
        case 11:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELL_LIST_HIGHLIGHT", 0, 0x55);
            object->scale_x = 0.9f;
            object->scale_y = 0.9f;
            break;
        case 12:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_ROLL", 0, 0x57);
            break;
        case 13:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_ROLL", 0, 0x57);
            break;
        case 14:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_PAPER", 0, 0x57);
            break;
        case 15:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_PAPER", 0, 0x57);
            break;
        case 16:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_ROLL", 0, 0x57);
            break;
        case 17:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_ROLL", 0, 0x57);
            break;
        case 18:
            object = load_named_2d_pfxobj(0xD003C, 0xC01C, "SPELLCAST_PAPER", 0, 0x57);
            break;
        default:
            continue;
        }
        mk_chess_pdata->manager.spell_hud[index].screen = object;
        mk_chess_pdata->manager.spell_hud[index].instance = object->instance;
        hide_screen_obj(object);
    }
}

static inline int mk_chess_piece_library_is_loaded(int character) {
    unsigned int index;
    for (index = 0; index < mk_chess_pdata->loaded_library_count; index++) {
        if (mk_chess_pdata->loaded_character_ids[index] == character) {
            return 1;
        }
    }
    return 0;
}

static inline unsigned int mk_chess_piece_art_character(unsigned int side, int piece_class) {
    switch (piece_class) {
    case 5: return ((ChessGameDefinition*)g_chess_definition_info)->teams[side].characters[0];
    case 2: return ((ChessGameDefinition*)g_chess_definition_info)->teams[side].characters[1];
    case 3:
    case 4: return ((ChessGameDefinition*)g_chess_definition_info)->teams[side].characters[2];
    case 1: return ((ChessGameDefinition*)g_chess_definition_info)->teams[side].characters[3];
    default: return ((ChessGameDefinition*)g_chess_definition_info)->teams[side].characters[4];
    }
}

/* TODO: [near miss] 97.10345%; lookup load placement, register allocation and string-pool addressing remain. */
void mk_chess_load_piece_art(void) {
    int classes[5] = {5, 0, 1, 3, 2};
    char filename[100];
    unsigned int class_index;
    unsigned int side;
    unsigned int character;
    unsigned int library_index;
    ChessLibraryEntry* library;
    int slot;

    if (g_board_game_controller.piece_libraries != 0) {
        for (class_index = 0; class_index < 5; class_index++) {
            for (side = 0; side < 2; side++) {
                character = mk_chess_piece_art_character(side, classes[class_index]);
                library_index = mk_chess_fetch_bp_num_based_on_pchr_num(character);
                library = &g_board_game_controller.piece_libraries[library_index];
                if (!mk_chess_piece_library_is_loaded(library->character_id) &&
                    mk_chess_pdata->loaded_library_count < 10) {
                    sprintf(filename, "mkc_%s.sec", library->name);
                    strlwr(filename);
                    slot = mk_chess_pdata->loaded_library_count >= 10 ? 0x100052 :
                        piece_model_ss_tbl[mk_chess_pdata->loaded_library_count];
                    load_art_section_by_name(slot, filename);
                    mk_chess_pdata->loaded_character_ids[mk_chess_pdata->loaded_library_count] = library->character_id;
                    mk_chess_pdata->last_loaded_library = library;
                    mk_chess_pdata->loaded_library_count++;
                }
            }
        }
    }
}

/* TODO: [near miss] 99.00%; lookup/call flow agrees; filename frame and string-pool addressing remain. */
static MkObj* mk_chess_create_piece_model_from_library(ChessLibraryEntry* library) {
    char filename[112];
    MkObj* object = mk_chess_find_loaded_piece_model(library);
    int slot;
    if (object == 0) {
        if (mk_chess_pdata->loaded_library_count < 10) {
            sprintf(filename, "mkc_%s.sec", library->name);
            strlwr(filename);
            slot = mk_chess_pdata->loaded_library_count >= 10 ? 0x100052 :
                piece_model_ss_tbl[mk_chess_pdata->loaded_library_count];
            load_art_section_by_name(slot, filename);
            mk_chess_pdata->loaded_character_ids[mk_chess_pdata->loaded_library_count] =
                library->character_id;
            mk_chess_pdata->last_loaded_library = library;
            mk_chess_pdata->loaded_library_count++;
        }
        return mk_chess_find_loaded_piece_model(library);
    }
    return object;
}

/* TODO: [near miss] 79.71%; dispatch/timing agree; omitted dead magnitude comparison and local lowering remain. */
static void mk_chess_drone_cursor_movement_to_target(
    ChessDroneState* drone, ChessCursor* cursor,
    unsigned int target_x, unsigned int target_y,
    unsigned int moving_delay, unsigned int arrived_delay,
    unsigned int arrived_state) {
    if (cursor->cell_x != target_x || cursor->cell_y != target_y) {
        int dx = target_x - cursor->cell_x;
        int dy = target_y - cursor->cell_y;
        unsigned int direction;
        if (dx == 0) {
            direction = dy > 0 ? 0 : 4;
        } else if (dy == 0) {
            direction = dx > 0 ? 2 : 6;
        } else if (dx > 0) {
            direction = dy > 0 ? 1 : 3;
        } else {
            direction = dy > 0 ? 7 : 5;
        }
        switch (direction) {
        case 0: xfer_proc(drone->proc, x_chess_down); break;
        case 1: xfer_proc(drone->proc, x_chess_right_and_down); break;
        case 7: xfer_proc(drone->proc, x_chess_left_and_down); break;
        case 4: xfer_proc(drone->proc, x_chess_up); break;
        case 3: xfer_proc(drone->proc, x_chess_right_and_up); break;
        case 5: xfer_proc(drone->proc, x_chess_left_and_up); break;
        case 2: xfer_proc(drone->proc, x_chess_right); break;
        case 6: xfer_proc(drone->proc, x_chess_left); break;
        }
        drone->cooldown = moving_delay + (unsigned short)randu0(8);
        return;
    }
    drone->action_state = arrived_state;
    drone->cooldown = arrived_delay + (unsigned short)randu0(15);
}

/* TODO: [near miss] 88.80%; teardown and roster flow recovered; latch/list lowering and owner scheduling remain. */
void mk_chess_remove_piece_from_team(ChessPiece* piece, int keep_active) {
    unsigned int side = piece->side;
    MkPtr* link = piece->effects;
    unsigned int count;
    unsigned int i;
    ChessSideState* team;

    while (link != 0) {
        ChessPieceEffect* effect = (ChessPieceEffect*)link->hdr;
        if (link->instance != effect->hdr.instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (effect->flags.bits.bit7) {
                fx_pause_emit(effect->emitter);
            }
            if (effect->flags.bits.bit4) {
                TrackedSound* sound = effect->sound;
                if (sound != 0 && sound->hdr.instance != effect->sound_instance) {
                    sound = 0;
                }
                if (sound != 0) {
                    stop_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
                    if (sound->hdr.instance != 0) {
                        sound->hdr.typed_vtbl->destroy(&sound->hdr);
                    }
                }
                effect->sound = 0;
                effect->sound_instance = 0;
            }
            if (effect->flags.bits.bit6) {
                if (effect->kind == 2) {
                    effect->expiry_clock = 0xFFFFFFFF;
                } else {
                    MkObj* object = mk_chess_live_imprison_object(effect);
                    if (object != 0 && object->hdr.instance != 0) {
                        object->hdr.typed_vtbl->destroy(&object->hdr);
                    }
                    if (effect->hdr.instance != 0) {
                        effect->hdr.typed_vtbl->destroy(&effect->hdr);
                    }
                }
            }
            link = link->next;
        }
    }
    destroy_list(&piece->effects);
    piece->effects = 0;
    count = mk_chess_pdata->sides[side]->live_piece_count;
    for (i = 0; i < count; i++) {
        if (mk_chess_pdata->sides[side]->pieces[i]->id == piece->id) {
            mk_chess_pdata->sides[side]->live_piece_count--;
            if (keep_active != 0) {
                team = mk_chess_pdata->sides[side];
                team->pieces[16 - team->captured_piece_count] = team->pieces[i];
                team = mk_chess_pdata->sides[side];
                hide_obj(team->pieces[16 - team->captured_piece_count]->object);
                mk_chess_pdata->sides[side]->captured_piece_count++;
            }
            team = mk_chess_pdata->sides[side];
            team->pieces[i] = team->pieces[team->live_piece_count];
            return;
        }
    }
}

/* TODO: [near miss] 92.5946%; roster/placement/process revival recovered; addressing and owner scheduling remain. */
ChessPiece* mk_chess_move_piece_from_deadpool_to(
    unsigned int side, unsigned int deadpool_index, unsigned int x, unsigned int y) {
    ChessSideState* team = mk_chess_pdata->sides[side];
    ChessPiece* piece = team->pieces[deadpool_index];
    ChessCell* cell;
    ChessCell* old_cell;
    ChessPieceProcPdata* data;

    team->pieces[team->live_piece_count] = piece;
    mk_chess_pdata->sides[side]->live_piece_count++;
    mk_chess_pdata->sides[side]->captured_piece_count = 0;
    piece->object->flags_08_bits.angular_velocity_enabled = 0;
    piece->object->flags_08_bits.rotation_enabled = 0;
    piece->object->ang.z = 0.0f;
    piece->object->ang.y = 0.0f;
    piece->object->ang.x = 0.0f;
    piece->object->ang.y = 3.1415927f * (float)side;
    cell = &mk_chess_pdata->board[(unsigned char)x].cells[(unsigned char)y];
    old_cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    if (old_cell->piece == piece) {
        old_cell->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    piece->object->hide_flag_bits.pin_animation = 0;
    piece->object->pos.value.x = cell->position.x + piece->runtime.fields.cell_offset.x;
    piece->object->pos.value.z = cell->position.z + piece->runtime.fields.cell_offset.z;
    update_obj_pos(piece->object);
    update_mkobj(&piece->object->hdr);
    piece->field_64 = 0;
    piece->proc = _create_mkproc_generic_bigstack(
        0xC01E, 0x1F, p_mk_chess_piece_init, sizeof(*data), (MkHdr**)&data);
    if (piece->proc != 0 && piece->movement != 0 && data != 0) {
        G_BOARD_GAME_BIGSTACK_COUNTER++;
        data->piece = piece;
        piece->proc->pre_destroy = mk_chess_piece_pre_wake;
        piece->proc->destroy_cb = mk_chess_piece_post_sleep;
        set_process_as_scriptable(piece->proc);
    }
    _mkproc_sleep_ticks = 2.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    hide_obj(piece->object);
    piece->health = g_board_game_controller.class_definitions[piece->type].initial_power;
    mk_chess_activate_piece_properties(piece);
    return piece;
}

void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void bgnd_launch_fx_at_bid_of_mkobj(const char* name, MkObj* object, int bone);

#pragma fp_contract off
/* TODO: [near miss] 76.91%; class effects and separate midpoint operations agree; string-pool/codegen residue remains. */
void mk_chess_activate_piece_properties(ChessPiece* piece) {
    if (piece != 0) {
        switch (piece->type) {
        case 5: {
            Vec first, second;
            float x, y, z;
            unsigned int effect;
            get_bone_world_pos(piece->object, 14, &first);
            get_bone_world_pos(piece->object, 15, &second);
            x = second.x + 0.5f * (first.x - second.x);
            y = second.y + 0.5f * (first.y - second.y);
            z = second.z + 0.5f * (first.z - second.z);
            if (piece->side == 0) {
                piece->runtime.fields.primary_effect = fx_by_owner("king_glow_1", 4);
                effect = piece->runtime.fields.primary_effect;
                fx_set_param_v3(effect, 0x202, x, y, z);
                fx_reset(effect);
                fx_resume_emit(effect);
            } else {
                piece->runtime.fields.primary_effect = fx_by_owner("king_glow_2", 4);
                effect = piece->runtime.fields.primary_effect;
                fx_set_param_v3(effect, 0x202, x, y, z);
                fx_reset(effect);
                fx_resume_emit(effect);
            }
            fx_set_render_priority(piece->runtime.fields.primary_effect, 18);
            fx_set(piece->runtime.fields.primary_effect, 0x204, -1.0f);
            return;
        }
        case 3:
            if (piece->side == 0) {
                piece->runtime.fields.primary_effect = fx_by_owner("sorcerer_glow_1", 4);
                bgnd_launch_fx_at_bid_of_mkobj("sorcerer_glow_1", piece->object, 14);
                fx_set_render_priority(piece->runtime.fields.primary_effect, 19);
                piece->runtime.fields.secondary_effect = fx_by_owner("sorcerer_glow_2", 4);
                bgnd_launch_fx_at_bid_of_mkobj("sorcerer_glow_2", piece->object, 15);
                fx_set_render_priority(piece->runtime.fields.secondary_effect, 19);
                return;
            }
            piece->runtime.fields.primary_effect = fx_by_owner("sorcerer_glow_3", 4);
            bgnd_launch_fx_at_bid_of_mkobj("sorcerer_glow_3", piece->object, 14);
            fx_set_render_priority(piece->runtime.fields.primary_effect, 19);
            piece->runtime.fields.secondary_effect = fx_by_owner("sorcerer_glow_4", 4);
            bgnd_launch_fx_at_bid_of_mkobj("sorcerer_glow_4", piece->object, 15);
            fx_set_render_priority(piece->runtime.fields.secondary_effect, 19);
            return;
        case 4:
            if (piece->side == 0) {
                piece->runtime.fields.primary_effect = fx_by_owner("sorcerer2_glow_1", 4);
                bgnd_launch_fx_at_bid_of_mkobj("sorcerer2_glow_1", piece->object, 14);
                fx_set_render_priority(piece->runtime.fields.primary_effect, 18);
                return;
            }
            piece->runtime.fields.primary_effect = fx_by_owner("sorcerer2_glow_2", 4);
            bgnd_launch_fx_at_bid_of_mkobj("sorcerer2_glow_2", piece->object, 14);
            fx_set_render_priority(piece->runtime.fields.primary_effect, 18);
            break;
        }
    }
}
#pragma fp_contract on

typedef struct ChessRepeatInput {
    char pad00[0x0C];
    int countdown;
    int switch_index;
    int analog;
    unsigned int direction;
} ChessRepeatInput;

/* TODO: [near miss] 90.15%; input/countdown operations agree; relocation and local lowering remain. */
float mk_chess_handle_repeatable_input(int port, ChessRepeatInput* input) {
    float x, y;
    if (input->countdown > 0) {
        while (--input->countdown > 0) {
            int matched = 0;
            if (input->analog == 1) {
                if (get_stick_pos(port, 0, &x, &y)) {
                    unsigned int direction = input->direction;
                    if (direction == 5 && x >= 0.35f && y >= 0.35f) {
                        matched = 1;
                    } else if (direction == 6 && x >= 0.35f && y <= -0.35f) {
                        matched = 1;
                    } else if (direction == 7 && x <= -0.35f && y >= 0.35f) {
                        matched = 1;
                    } else if (direction == 8 && x <= -0.35f && y <= -0.35f) {
                        matched = 1;
                    } else if (direction == 1 && x >= 0.35f) {
                        matched = 1;
                    } else if (direction == 2 && x <= -0.35f) {
                        matched = 1;
                    } else if (direction == 3 && y >= 0.35f) {
                        matched = 1;
                    } else if (direction == 4 && y <= -0.35f) {
                        matched = 1;
                    }
                }
            } else if (input->direction != 0) {
                int a = check_switch(port, 15);
                int b = check_switch(port, 13);
                int c = check_switch(port, 12);
                int d = check_switch(port, 14);
                switch (input->direction) {
                case 5: if (a == 0 && c == 0) matched = 1; break;
                case 6: if (a == 0 && d == 0) matched = 1; break;
                case 7: if (b == 0 && c == 0) matched = 1; break;
                case 8: if (b == 0 && d == 0) matched = 1; break;
                }
            } else if (check_switch(port, input->switch_index) == 0) {
                matched = 1;
            }
            if (matched) {
                _mkproc_sleep_ticks = 4.0f;
                ((ChessProcVtable*)aproc->vtbl)->sleep();
                input->countdown = 0;
            } else {
                _mkproc_sleep_ticks = 1.0f;
                ((ChessProcVtable*)aproc->vtbl)->sleep();
            }
        }
    }
    return 0.0f;
}

static void mk_chess_calc_all_attackers_in_turn(
    ChessPiece* piece, unsigned int turns, MkPtr** attackers,
    unsigned int* rating, int immediate);

/* TODO: [near miss] 85.04%; search/restore flow recovered; nested scan addressing and owner scheduling remain. */
static int mk_chess_drone_move_piece_out_of_danger(unsigned int side, ChessPiece* piece) {
    unsigned char best_x = 0, best_y = 0;
    unsigned int best_rating = 1000;
    int found = 0;
    unsigned char old_x = piece->cell_x, old_y = piece->cell_y;
    unsigned char x, y;
    MkPtr* attackers;
    unsigned int rating;

    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            unsigned int move = (piece->move_map->rows[y] >> (3 * x)) & 7;
            if (move != 0 && move != 2) {
                mk_chess_pdata->board[x].cells[y].piece = piece;
                piece->cell_x = x;
                piece->cell_y = y;
                mk_chess_recalculate_all_piece_moves();
                mk_chess_calc_all_attackers_in_turn(piece, 0, &attackers, &rating, 1);
                destroy_list(&attackers);
                if (best_rating > rating ||
                    (best_rating == rating && (unsigned short)randu0(100) < 100)) {
                    best_rating = rating;
                    best_x = x;
                    best_y = y;
                    found = 1;
                }
                mk_chess_pdata->board[x].cells[y].piece = 0;
                mk_chess_pdata->board[old_x].cells[old_y].piece = piece;
                piece->cell_x = old_x;
                piece->cell_y = old_y;
                mk_chess_recalculate_all_piece_moves();
            }
        }
    }
    if (found && best_rating == 0) {
        ChessDroneState* drone = (ChessDroneState*)mk_chess_pdata->sides[side];
        drone->target_0_x = piece->cell_x;
        drone->target_0_y = piece->cell_y;
        drone->target_1_x = best_x;
        drone->target_1_y = best_y;
        drone->action_state = 1;
        drone->cooldown = 5;
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 75.79%; retail bounds and rating selection recovered; loop addressing and register lifetimes remain. */
static int mk_chess_drone_help_piece_by_blocking(
    unsigned int side, ChessPiece* threatened, ChessPiece* attacker) {
    unsigned int best_rating = 100000;
    unsigned int best_x = 0;
    unsigned char best_y = 0;
    ChessPiece* best = 0;
    unsigned char min_x = attacker->cell_x;
    unsigned char max_x = threatened->cell_x;
    unsigned char min_y = attacker->cell_y;
    unsigned char max_y = threatened->cell_y;
    unsigned int index;

    if (min_x > max_x) {
        min_x = threatened->cell_x;
        max_x = attacker->cell_x;
    }
    /* Retail compares attacker Y with the sorted upper X bound here. */
    if (min_y > max_x) {
        min_y = threatened->cell_y;
        max_y = attacker->cell_y;
    }
    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        ChessPiece* piece = mk_chess_pdata->sides[side]->pieces[index];
        if (piece != threatened) {
            unsigned int x, y;
            for (x = min_x; x <= max_x; x++) {
                for (y = min_y; y <= max_y; y++) {
                    if (mk_chess_pdata->board[x].cells[y].piece == 0 &&
                        ((piece->move_map->rows[y] >> (3 * x)) & 7) != 0) {
                        unsigned int rating = mk_chess_drone_calc_fight_favorability_rating(attacker, piece);
                        if (best_rating > rating) {
                            best_x = x;
                            best_rating = rating;
                            best = piece;
                            best_y = y;
                        }
                    }
                }
            }
        }
    }
    if (best != 0 && best_rating < 8) {
        ChessDroneState* drone = (ChessDroneState*)mk_chess_pdata->sides[side];
        drone->target_0_x = best->cell_x;
        drone->target_0_y = best->cell_y;
        drone->target_1_x = best_x;
        drone->target_1_y = best_y;
        drone->action_state = 1;
        drone->cooldown = 5;
        return 1;
    }
    return 0;
}

extern int matchup_type_adjustor_for_attacker[6];
extern int matchup_type_adjustor_for_defender[6];

/* TODO: [near miss] 94.14976%; health and skill rating recovered; local lowering and relocations remain. */
static unsigned int mk_chess_drone_calc_fight_favorability_rating(
    ChessPiece* attacker, ChessPiece* defender) {
    int attack_skill = 4;
    int defend_skill = 4;
    float attack_health = attacker->health;
    float defend_health = defender->health;
    int attack_type = attacker->type;
    int defend_type;
    unsigned int side;
    int difference, rating;
    float health_difference;

    if (attack_type == 1 && defender->type != 1) {
        attack_health = defender->health;
    }
    if (attack_type != 1 && defender->type == 1) {
        defend_health = attacker->health;
    }
    if (attack_type == 1 && defender->type == 1) {
        defend_health = 1.0f;
        attack_health = 1.0f;
    }
    side = attacker->side;
    attack_health += 0.1f;
    if (side == 0) {
        attack_health = 0.25f * (unsigned int)board_game_save_data.input_flags.p1_power_squares + attack_health;
        defend_health = 0.25f * (unsigned int)board_game_save_data.input_flags.p2_power_squares + defend_health;
    } else {
        attack_health = 0.25f * (unsigned int)board_game_save_data.input_flags.p2_power_squares + attack_health;
        defend_health = 0.25f * (unsigned int)board_game_save_data.input_flags.p1_power_squares + defend_health;
    }
    if (attack_health > 1.0f) {
        attack_health = 1.0f;
    }
    if (defend_health > 1.0f) {
        defend_health = 1.0f;
    }
    defend_type = defender->type;
    if (defend_type != 1) {
        defend_skill = mk_chess_pdata->sides[side]->saved_state_114[defend_type];
    }
    if (g_chess_definition_info[side].field_04 != 0) {
        const ChessHandicapRange* range =
            &the_drone_master_handicap_levels[game_settings.arcade_difficulty][attack_type];
        attack_skill = range->minimum + ((unsigned int)(range->maximum - range->minimum) >> 1);
    } else if (attack_type != 1) {
        attack_skill = mk_chess_pdata->sides[defender->side]->saved_state_114[attack_type];
    }
    difference = attack_skill - defend_skill;
    if (difference < -6) {
        rating = 1;
    } else if (difference < -3) {
        rating = 3;
    } else if (difference < 0) {
        rating = 4;
    } else if (difference < 2) {
        rating = 5;
    } else if (difference < 5) {
        rating = 7;
    } else if (difference < 7) {
        rating = 8;
    } else {
        rating = 10;
    }
    health_difference = attack_health - defend_health;
    if (health_difference < -0.8f) {
        rating -= 4;
    } else if (health_difference < -0.6f) {
        rating -= 3;
    } else if (health_difference < -0.3f) {
        rating -= 2;
    } else if (health_difference < -0.15f) {
        rating -= 1;
    } else if (!(health_difference < 0.15f)) {
        if (health_difference < 0.3f) {
            rating += 1;
        } else if (health_difference < 0.6f) {
            rating += 2;
        } else if (health_difference < 0.9f) {
            rating += 3;
        } else {
            rating = 100;
        }
    }
    if (defend_health < 0.15f) {
        rating += 3;
    }
    if (attack_health < 0.15f) {
        rating -= 3;
    }
    rating += matchup_type_adjustor_for_defender[defend_type] +
        matchup_type_adjustor_for_attacker[attack_type];
    if (rating < 0) {
        rating = 1;
    }
    return rating;
}

typedef struct ChessAttackerInfo {
    MkHdr hdr;
    ChessPiece* piece;
    unsigned int attack_rating;
    unsigned int counterattack_rating;
    unsigned int cell_x;
    unsigned int cell_y;
} ChessAttackerInfo; /* 0x1C */

static inline unsigned int mk_chess_best_counterattack_rating(
    ChessPiece* attacker, ChessPiece* excluded) {
    unsigned int best = 0;
    unsigned int index;
    unsigned int side = attacker->side == 0;
    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        ChessPiece* piece = mk_chess_pdata->sides[side]->pieces[index];
        if (piece != excluded &&
            ((piece->move_map->rows[attacker->cell_y] >> (attacker->cell_x * 3)) & 7) == 2) {
            unsigned int rating = mk_chess_drone_calc_fight_favorability_rating(piece, attacker);
            if (best < rating) {
                best = rating;
            }
        }
    }
    return best;
}

/* TODO: [near miss] 97.29%; attacker selection and list effects recovered; local scheduling and lifetimes remain. */
static void mk_chess_calc_all_attackers_for(
    ChessPiece* target, MkPtr** attackers, unsigned int* count) {
    unsigned int index;
    unsigned int side = target->side == 0;
    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        ChessPiece* piece = mk_chess_pdata->sides[side]->pieces[index];
        if (((piece->move_map->rows[target->cell_y] >> (target->cell_x * 3)) & 7) == 2) {
            ChessAttackerInfo* existing = 0;
            ChessAttackerInfo* info;
            unsigned int counterattack;
            int update = 0;
            if (attackers != 0) {
                MkPtr* link = *attackers;
                while (link != 0) {
                    ChessAttackerInfo* entry = (ChessAttackerInfo*)link->hdr;
                    if (link->instance != entry->hdr.instance) {
                        MkPtr* next = link->next;
                        link->hdr = 0;
                        destroy_mkptr(link);
                        link = next;
                    } else if (entry->piece == piece) {
                        existing = entry;
                        break;
                    } else {
                        link = link->next;
                    }
                }
            }
            if (existing != 0) {
                counterattack = mk_chess_best_counterattack_rating(piece, target);
                if (counterattack < existing->counterattack_rating &&
                    (unsigned short)randu0(100) < 100) {
                    update = 1;
                } else if (counterattack == existing->counterattack_rating &&
                    (unsigned short)randu0(100) < 100) {
                    update = 1;
                }
                info = existing;
            } else {
                update = 1;
                counterattack = mk_chess_best_counterattack_rating(piece, target);
                info = (ChessAttackerInfo*)get_mkpdata_generic(sizeof(ChessAttackerInfo));
                if (info != 0) {
                    zero_pdata_payload(sizeof(ChessAttackerInfo), &info->hdr);
                }
                mk_insert(info != 0 ? as_mkhdr(&info->hdr) : 0, attackers);
                (*count)++;
            }
            if (update == 1) {
                info->piece = piece;
                info->attack_rating = mk_chess_drone_calc_fight_favorability_rating(piece, target);
                info->counterattack_rating = counterattack;
                info->cell_x = piece->cell_x;
                info->cell_y = piece->cell_y;
            }
        }
    }
}

/* TODO: [near miss] 92.08%; recursive trial and restore flow recovered; nested addressing and lifetimes remain. */
static void mk_chess_calc_all_attackers_in_turn(
    ChessPiece* target, unsigned int turns, MkPtr** attackers,
    unsigned int* counts, int initialize) {
    int work_remaining = 6;
    unsigned int side = target->side == 0;
    unsigned int index;
    if (initialize != 0) {
        for (index = 0; index <= turns; index++) {
            counts[index] = 0;
            attackers[index] = 0;
        }
    }
    if (turns == 0) {
        mk_chess_calc_all_attackers_for(target, &attackers[turns], &counts[turns]);
        return;
    }
    mk_chess_calc_all_attackers_for(target, &attackers[turns], &counts[turns]);
    for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
        ChessPiece* piece = mk_chess_pdata->sides[side]->pieces[index];
        ChessPieceMoveMap moves;
        unsigned int x, y;
        if (--work_remaining == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            work_remaining = 6;
        }
        memcpy(&moves, piece->move_map, sizeof(moves));
        for (x = 0; x < 10; x++) {
            for (y = 0; y < 10; y++) {
                unsigned int move = (moves.rows[y] >> (3 * x)) & 7;
                if (move != 0) {
                    ChessPiece* captured;
                    unsigned int old_x, old_y;
                    /* Retail reserves two slots; known callers request at most one turn. */
                    MkPtr* trial_attackers[2];
                    unsigned int trial_counts[2];
                    MkPtr* link;
                    unsigned int best_counterattack = 0;
                    unsigned int best_attack = 0;
                    if (--work_remaining == 0) {
                        _mkproc_sleep_ticks = 1.0f;
                        ((ChessProcVtable*)aproc->vtbl)->sleep();
                        work_remaining = 6;
                    }
                    if (move == 2) {
                        captured = mk_chess_pdata->board[x].cells[y].piece;
                        captured->flags.unknown_bit2 = 1;
                    } else {
                        captured = 0;
                    }
                    old_x = piece->cell_x;
                    old_y = piece->cell_y;
                    mk_chess_pdata->board[x].cells[y].piece = piece;
                    mk_chess_pdata->board[old_x].cells[old_y].piece = 0;
                    piece->cell_x = x;
                    piece->cell_y = y;
                    mk_chess_recalculate_all_piece_moves();
                    mk_chess_calc_all_attackers_in_turn(
                        target, turns - 1, trial_attackers, trial_counts, 1);
                    link = trial_attackers[0];
                    while (link != 0) {
                        ChessAttackerInfo* info = (ChessAttackerInfo*)link->hdr;
                        if (link->instance != info->hdr.instance) {
                            MkPtr* next = link->next;
                            link->hdr = 0;
                            destroy_mkptr(link);
                            link = next;
                        } else {
                            if (best_counterattack < info->counterattack_rating) {
                                best_counterattack = info->counterattack_rating;
                            }
                            if (best_attack < info->attack_rating) {
                                best_attack = info->attack_rating;
                            }
                            link = link->next;
                        }
                    }
                    destroy_list(&trial_attackers[0]);
                    if (trial_counts[0] != 0) {
                        ChessAttackerInfo* info = 0;
                        MkPtr** output = &attackers[turns - 1];
                        if (output != 0) {
                            link = *output;
                            while (link != 0) {
                                ChessAttackerInfo* entry = (ChessAttackerInfo*)link->hdr;
                                if (link->instance != entry->hdr.instance) {
                                    MkPtr* next = link->next;
                                    link->hdr = 0;
                                    destroy_mkptr(link);
                                    link = next;
                                } else if (entry->piece == piece) {
                                    info = entry;
                                    break;
                                } else {
                                    link = link->next;
                                }
                            }
                        }
                        if (info == 0) {
                            info = (ChessAttackerInfo*)get_mkpdata_generic(sizeof(ChessAttackerInfo));
                            if (info != 0) {
                                zero_pdata_payload(sizeof(ChessAttackerInfo), &info->hdr);
                            }
                        }
                        info->piece = piece;
                        info->attack_rating = best_attack;
                        info->counterattack_rating = best_counterattack;
                        info->cell_x = x;
                        info->cell_y = y;
                        mk_insert(info != 0 ? as_mkhdr(&info->hdr) : 0, output);
                        counts[turns - 1]++;
                    }
                    mk_chess_pdata->board[x].cells[y].piece = captured;
                    if (captured != 0) {
                        captured->flags.unknown_bit2 = 0;
                    }
                    mk_chess_pdata->board[old_x].cells[old_y].piece = piece;
                    piece->cell_x = old_x;
                    piece->cell_y = old_y;
                }
            }
        }
        mk_chess_recalculate_all_piece_moves();
    }
}

static inline ChessAttackerInfo* mk_chess_select_best_attack(
    MkPtr* link, unsigned int side) {
    unsigned int best_rating = 0;
    ChessAttackerInfo* best = 0;
    while (link != 0) {
        ChessAttackerInfo* info = (ChessAttackerInfo*)link->hdr;
        if (link->instance != info->hdr.instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if ((info->attack_rating > best_rating ||
                 (info->attack_rating == best_rating && (unsigned short)randu0(100) < 30)) &&
                (mk_chess_pdata->sides[side]->desired_x != info->cell_x ||
                 mk_chess_pdata->sides[side]->desired_y != info->cell_y ||
                 ((unsigned short)randu0(100) < 10 && game_settings.arcade_difficulty < 3))) {
                best_rating = info->attack_rating;
                best = info;
            }
            link = link->next;
        }
    }
    return best;
}

/* TODO: [near miss] 87.99%; matchup selection and cleanup recovered; local scheduling and lifetimes remain. */
static int mk_chess_drone_move_best_matchup_against_piece(
    unsigned int side, ChessPiece* target, int allow_setup) {
    MkPtr* attackers[2];
    unsigned int counts[2];
    ChessDroneState* drone = (ChessDroneState*)mk_chess_pdata->sides[side];
    ChessAttackerInfo* best;
    mk_chess_calc_all_attackers_in_turn(target, 1, attackers, counts, 1);
    best = mk_chess_select_best_attack(attackers[1], target->side);
    if (best != 0) {
        ChessPiece* piece = best->piece;
        drone->target_0_x = piece->cell_x;
        drone->target_0_y = piece->cell_y;
        drone->target_1_x = target->cell_x;
        drone->target_1_y = target->cell_y;
        drone->action_state = 1;
        drone->cooldown = 5;
    } else if (allow_setup != 0) {
        best = mk_chess_select_best_attack(attackers[0], side);
        if (best != 0) {
            ChessPiece* piece = best->piece;
            unsigned int x = piece->cell_x;
            unsigned int y = piece->cell_y;
            if (mk_chess_pdata->board[x].cells[y].square_type != 0) {
                destroy_list(&attackers[0]);
                destroy_list(&attackers[1]);
                return 0;
            }
            if (piece->type == 5) {
                destroy_list(&attackers[0]);
                destroy_list(&attackers[1]);
                return 0;
            }
            drone->target_0_x = x;
            drone->target_0_y = y;
            drone->target_1_x = best->cell_x;
            drone->target_1_y = best->cell_y;
            drone->action_state = 1;
            drone->cooldown = 5;
        }
    }
    destroy_list(&attackers[0]);
    destroy_list(&attackers[1]);
    return best != 0;
}

static inline unsigned int mk_chess_select_vulnerability_attack(
    MkPtr* link, unsigned int side, ChessPiece** selected) {
    unsigned int best = 0;
    while (link != 0) {
        ChessAttackerInfo* info = (ChessAttackerInfo*)link->hdr;
        if (link->instance != info->hdr.instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if ((info->attack_rating > best ||
                 (info->attack_rating == best && (unsigned short)randu0(100) < 30)) &&
                (mk_chess_pdata->sides[side]->desired_x != info->cell_x ||
                 mk_chess_pdata->sides[side]->desired_y != info->cell_y ||
                 ((unsigned short)randu0(100) < 10 && game_settings.arcade_difficulty < 3))) {
                best = info->attack_rating;
                *selected = info->piece;
            }
            link = link->next;
        }
    }
    return best;
}

static inline int mk_chess_vulnerability_band(unsigned int rating) {
    if (rating < 3) {
        return 1;
    } else if (rating < 5) {
        return 3;
    } else if (rating < 7) {
        return 5;
    } else if (rating < 10) {
        return 10;
    }
    return 255;
}

/* TODO: [near miss] 92.49%; vulnerability bands and output order recovered; local scheduling and lifetimes remain. */
static int mk_chess_calc_piece_vulnerability_rating(
    ChessPiece* target, int* immediate_rating, int* setup_rating,
    ChessPiece** immediate_attacker, ChessPiece** setup_attacker) {
    MkPtr* attackers[2];
    unsigned int counts[2];
    unsigned int rating;
    int vulnerability;
    mk_chess_calc_all_attackers_in_turn(target, 1, attackers, counts, 1);
    rating = mk_chess_select_vulnerability_attack(attackers[1], target->side, immediate_attacker);
    vulnerability = mk_chess_vulnerability_band(rating);
    *immediate_rating = vulnerability;
    rating = mk_chess_select_vulnerability_attack(attackers[0], target->side, setup_attacker);
    if (rating > 5) {
        vulnerability++;
    }
    if (rating > 9) {
        vulnerability++;
    }
    if (rating > 99) {
        vulnerability++;
    }
    if (rating > 254) {
        vulnerability = 255;
    }
    *setup_rating = mk_chess_vulnerability_band(rating);
    destroy_list(&attackers[0]);
    destroy_list(&attackers[1]);
    return vulnerability;
}

/* TODO: [near miss] 99.48%; data-value exact; anonymous constant relocation identity remains. */
static void mk_chess_calc_knowledge_base(ChessPiece* first, ChessPiece* second, unsigned int result) {
    unsigned int first_skill = 4;
    unsigned int second_skill = 4;
    float first_loss = board_game_save_data.knowledge_health[0] - first->health;
    float second_loss = board_game_save_data.knowledge_health[1] - second->health;
    float difference = first_loss - second_loss;
    if (difference < -0.2f) {
        float advantage = -1.0f * difference;
        if (advantage > 0.89f) {
            first_skill = 8;
            second_skill = 1;
        } else if (advantage > 0.79f) {
            first_skill = 7;
            second_skill = 1;
        } else if (advantage > 0.6f) {
            first_skill = 6;
            second_skill = 1;
        } else if (advantage > 0.4f) {
            first_skill = 5;
            second_skill = 2;
        } else {
            first_skill = 4;
            second_skill = 3;
        }
    } else if (difference > 0.2f) {
        if (difference > 0.89f) {
            second_skill = 8;
            first_skill = 1;
        } else if (difference > 0.79f) {
            second_skill = 7;
            first_skill = 1;
        } else if (difference > 0.6f) {
            second_skill = 6;
            first_skill = 1;
        } else if (difference > 0.4f) {
            second_skill = 5;
            first_skill = 2;
        } else {
            second_skill = 4;
            first_skill = 3;
        }
    }
    if (result == 1 && second_skill != 0) {
        second_skill--;
    } else if (result == 2 && first_skill != 0) {
        first_skill--;
    }
    if (result == 1 && first_loss < 0.2f && second_loss > 0.35f) {
        first_skill += 2;
        second_skill--;
    } else if (result == 2 && second_loss < 0.2f && first_loss > 0.35f) {
        second_skill += 2;
        first_skill--;
    }
    mk_chess_pdata->sides[0]->saved_state_114[second->type] =
        (mk_chess_pdata->sides[0]->saved_state_114[second->type] + second_skill) >> 1;
    if (second->type == 3 || second->type == 4) {
        mk_chess_pdata->sides[0]->saved_state_114[3] = second_skill;
        mk_chess_pdata->sides[0]->saved_state_114[4] = second_skill;
    }
    mk_chess_pdata->sides[1]->saved_state_114[first->type] =
        (mk_chess_pdata->sides[1]->saved_state_114[first->type] + first_skill) >> 1;
    if (first->type == 3 || first->type == 4) {
        mk_chess_pdata->sides[1]->saved_state_114[3] = first_skill;
        mk_chess_pdata->sides[1]->saved_state_114[4] = first_skill;
    }
}

static int mk_chess_drone_piece_best_path_to(
    ChessPiece* piece, unsigned int x, unsigned int y,
    unsigned int* capture, unsigned int* rating, int* next_x, int* next_y);

/* TODO: [near miss] 82.44%; roster path priorities recovered; helper lowering and lifetimes remain. */
static int mk_chess_drone_best_path_to(
    ChessDroneState* drone, unsigned int x, unsigned int y,
    ChessPiece** selected, unsigned int* capture, unsigned int* rating,
    int* next_x, int* next_y) {
    int found = 0;
    unsigned int index;
    *selected = 0;
    for (index = 0; index < drone->live_piece_count; index++) {
        ChessPiece* piece = drone->pieces[index];
        if (mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type != 1) {
            int consider = 1;
            if (board_game_save_data.input_flags.input_locked == 0) {
                ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
                if (mk_chess_pieces_are_neighbors(piece, king) != 0 &&
                    (unsigned short)randu0(100) < 90) {
                    consider = 0;
                }
            }
            if (consider) {
                unsigned int candidate_capture, candidate_rating;
                int candidate_x, candidate_y;
                if (mk_chess_drone_piece_best_path_to(piece, x, y,
                        &candidate_capture, &candidate_rating, &candidate_x, &candidate_y) != 0 &&
                    (found == 0 || *rating != 255 || (unsigned short)randu0(100) >= 35)) {
                    if (piece->health < 1.0f && piece->type != 1 && candidate_capture == 0) {
                        *capture = 0;
                        found = 1;
                        *rating = 255;
                        *next_x = candidate_x;
                        *next_y = candidate_y;
                        *selected = piece;
                    } else if (candidate_rating > *rating) {
                        found = 1;
                        *capture = candidate_capture;
                        *rating = candidate_rating;
                        *next_x = candidate_x;
                        *next_y = candidate_y;
                        *selected = piece;
                    }
                }
            }
        }
    }
    return found;
}

static inline int mk_chess_rebuild_piece_path_moves(ChessPiece* piece) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessPieceMoveMap* map = piece->move_map;
    unsigned int* restriction = piece == 0 ? 0 : &piece->access_restrictions[1];
    unsigned int index;
    ChessMovementSkill* skill;

    if (piece->flags.unknown_bit2) {
        return 0;
    }
    map->target_count = 0;
    for (index = 0; index < 10; index++) {
        map->rows[index] = 0;
    }
    if (*restriction >= (unsigned int)manager->clock) {
        return 0;
    }
    for (index = 0;
         index < g_board_game_controller.class_definitions[piece->type].movement_skill_count;
         index++) {
        skill = &g_board_game_controller.class_definitions[piece->type].movement_skills[index];
        switch (skill->move_type) {
        case 0:
            mk_chess_iterate_thru_move(piece, map, skill, 1, 0);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, -1, 0);
            }
            mk_chess_iterate_thru_move(piece, map, skill, 0, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, 0, -1);
            }
            break;
        case 1:
            mk_chess_iterate_thru_move(piece, map, skill, 1, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, 1, -1);
            }
            mk_chess_iterate_thru_move(piece, map, skill, -1, 1);
            if (skill->flags & 0x200) {
                mk_chess_iterate_thru_move(piece, map, skill, -1, -1);
            }
            break;
        }
    }
    return 0;
}

/* TODO: [near miss] 94.66%; trial map and restoration recovered; nested addressing and helper lowering remain. */
static int mk_chess_drone_piece_best_path_to(
    ChessPiece* piece, unsigned int target_x, unsigned int target_y,
    unsigned int* capture, unsigned int* rating, int* next_x, int* next_y) {
    int found = 0;
    unsigned int x, y;
    *capture = 0;
    *rating = 0;
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            unsigned int move = (piece->move_map->rows[y] >> (3 * x)) & 7;
            if (move != 0) {
                ChessPiece* captured = move == 2 ? mk_chess_pdata->board[x].cells[y].piece : 0;
                unsigned char old_x = piece->cell_x;
                unsigned char old_y = piece->cell_y;
                mk_chess_pdata->board[x].cells[y].piece = piece;
                mk_chess_pdata->board[old_x].cells[old_y].piece = 0;
                piece->cell_x = x;
                piece->cell_y = y;
                mk_chess_rebuild_piece_path_moves(piece);
                if (((piece->move_map->rows[target_y] >> (3 * target_x)) & 7) == 0) {
                    mk_chess_pdata->board[x].cells[y].piece = captured;
                    mk_chess_pdata->board[old_x].cells[old_y].piece = piece;
                    piece->cell_x = old_x;
                    piece->cell_y = old_y;
                    mk_chess_rebuild_piece_path_moves(piece);
                } else {
                    mk_chess_pdata->board[x].cells[y].piece = captured;
                    mk_chess_pdata->board[old_x].cells[old_y].piece = piece;
                    piece->cell_x = old_x;
                    piece->cell_y = old_y;
                    mk_chess_rebuild_piece_path_moves(piece);
                    if (captured != 0) {
                        unsigned int candidate = mk_chess_drone_calc_fight_favorability_rating(piece, captured);
                        if (*rating < candidate) {
                            found = 1;
                            *capture = 1;
                            *rating = candidate;
                            *next_x = x;
                            *next_y = y;
                        }
                    } else if (found == 0 || *rating != 255 || (unsigned short)randu0(100) >= 35) {
                        *capture = 0;
                        found = 1;
                        *rating = 255;
                        *next_x = x;
                        *next_y = y;
                    }
                }
            }
        }
    }
    return found;
}

/* TODO: [near miss] 77.96%; approach selection recovered; nested addressing and local lifetimes remain. */
static int mk_chess_drone_get_close_to_piece(ChessDroneState* drone,
    ChessPiece* target, ChessPiece** selected, int* next_x, int* next_y) {
    ChessAttackerInfo candidates[16];
    unsigned short count = 0;
    unsigned int best_x = 0, best_y = 0;
    int prefer_capture = 1;
    unsigned int index;
    for (index = 0; index < drone->live_piece_count; index++) {
        ChessPiece* piece = drone->pieces[index];
        int consider = 1;
        if (mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type == 1) {
            continue;
        }
        if (board_game_save_data.input_flags.input_locked == 0 && drone->live_piece_count > 6) {
            ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
            if (mk_chess_pieces_are_neighbors(piece, king) != 0 &&
                (unsigned short)randu0(100) < 100) {
                consider = 0;
            }
        }
        if (consider) {
            unsigned int best_distance = 7;
            unsigned int x, y;
            for (x = 0; x < 10; x++) {
                for (y = 0; y < 10; y++) {
                    unsigned int move = (drone->pieces[index]->move_map->rows[y] >> (3 * x)) & 7;
                    if (move != 0 &&
                        (mk_chess_pdata->sides[target->side]->desired_x != x ||
                         mk_chess_pdata->sides[target->side]->desired_y != y ||
                         (unsigned short)randu0(100) >= 90)) {
                        int dx = (int)target->cell_x - (int)x;
                        int dy = (int)target->cell_y - (int)y;
                        unsigned int distance = (dy < 0 ? -dy : dy);
                        if (dx != 0) {
                            if (dy == 0) {
                                distance = (dx < 0 ? -dx : dx);
                            } else if (distance < (unsigned int)(dx < 0 ? -dx : dx)) {
                                distance = (dx < 0 ? -dx : dx);
                            }
                        }
                        if ((distance < best_distance ||
                             (distance == best_distance && (unsigned short)randu0(100) < 20)) &&
                            (prefer_capture != 0 || move != 2 ||
                             (distance != best_distance + 1 && distance <= 4))) {
                            best_distance = distance;
                            best_x = x;
                            best_y = y;
                            prefer_capture = move == 2;
                        }
                    }
                }
            }
            if (best_distance != 7) {
                candidates[count].piece = drone->pieces[index];
                count++;
                candidates[count - 1].cell_x = best_x;
                candidates[count - 1].cell_y = best_y;
            }
            if (count == 16) {
                break;
            }
        }
    }
    if (count != 0) {
        unsigned int choice = (unsigned short)randu0(count);
        *selected = candidates[choice].piece;
        *next_x = candidates[choice].cell_x;
        *next_y = candidates[choice].cell_y;
        return 1;
    }
    return 0;
}

static int mk_chess_drone_random_piece_move_with_definition(
    ChessDroneState* drone, ChessPiece* piece, ChessMoveTarget* target);

/* TODO: [near miss] 81.22%; random movement retry and capture rules recovered; local lowering and lifetimes remain. */
static int mk_chess_drone_random_piece_move(ChessDroneState* drone) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int found = 0;
    drone->field_144 = 2;
    while (found == 0) {
        ChessPiece* piece = drone->pieces[(unsigned short)randu0((unsigned short)drone->live_piece_count)];
        if (mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].square_type == 1 &&
            (unsigned short)randu0(100) < 80) {
            continue;
        }
        if (board_game_save_data.input_flags.input_locked == 0 &&
            (unsigned int)mk_chess_pdata->manager.clock < 12) {
            ChessPiece* king = mk_chess_find_piece_of_type(drone->piece->side, 5, 0);
            if (mk_chess_pieces_are_neighbors(piece, king) != 0) {
                continue;
            }
        }
        if (piece->access_restrictions[1] < (unsigned int)manager->clock &&
            piece->move_map->target_count != 0) {
            unsigned int choice;
            drone->target_0_x = piece->cell_x;
            drone->target_0_y = piece->cell_y;
            choice = (unsigned short)randu0((unsigned short)piece->move_map->target_count);
            if (piece->move_map->targets[choice].kind == 0) {
                if (mk_chess_drone_random_piece_move_with_definition(
                        drone, piece, &piece->move_map->targets[choice]) != 0) {
                    found = 1;
                }
            } else if ((piece->type != 5 && piece->type != 3 && piece->type != 4) ||
                       ((unsigned short)randu0(100) < 10 && game_settings.arcade_difficulty < 3)) {
                unsigned int clock = manager->clock;
                if (piece->access_restrictions[0] < clock) {
                    ChessMoveTarget* target = &piece->move_map->targets[choice];
                    ChessPiece* defender = mk_chess_pdata->board[target->cell_x].cells[target->cell_y].piece;
                    if (defender->access_restrictions[2] < clock &&
                        (mk_chess_drone_calc_fight_favorability_rating(piece, defender) > 4 ||
                         (unsigned short)randu0(100) < 10)) {
                        found = 1;
                        drone->target_1_x = piece->move_map->targets[choice].cell_x;
                        drone->target_1_y = piece->move_map->targets[choice].cell_y;
                    }
                }
            }
        }
    }
    drone->action_state = 1;
    drone->cooldown = 5;
    return 1;
}

/* TODO: [near miss] 82.44%; directional rejection and step selection recovered; local lifetimes and relocations remain. */
static int mk_chess_drone_random_piece_move_with_definition(
    ChessDroneState* drone, ChessPiece* piece, ChessMoveTarget* target) {
    unsigned char edge_distance = 0;
    int dx = (int)target->cell_x - piece->cell_x;
    int dy = (int)target->cell_y - piece->cell_y;
    unsigned short distance = dy < 0 ? -dy : dy;
    unsigned int direction;
    int x, y;
    if (dx == 0) {
        direction = dy > 0 ? 0 : 4;
    } else if (dy == 0) {
        distance = dx < 0 ? -dx : dx;
        direction = dx > 0 ? 2 : 6;
    } else {
        if (distance < (unsigned int)(dx < 0 ? -dx : dx)) {
            distance = dx < 0 ? -dx : dx;
        }
        if (dx > 0) {
            direction = dy > 0 ? 1 : 3;
        } else {
            direction = dy > 0 ? 7 : 5;
        }
    }
    {
        unsigned int retreat_rejection[10] = {100, 95, 80, 60, 30, 30, 30, 15, 15, 5};
        if (piece->type == 0) {
            unsigned short roll = (unsigned short)randu0(100);
            unsigned char side = drone->piece->side;
            if (side == 1 && (direction == 0 || direction == 7 || direction == 1)) {
                edge_distance = 9 - piece->cell_y;
            } else if (side == 0 && (direction == 4 || direction == 5 || direction == 3)) {
                edge_distance = piece->cell_y;
            } else {
                roll = 100;
            }
            if (roll < retreat_rejection[edge_distance]) {
                return 0;
            }
            if (direction != 0 && direction != 4 && (unsigned short)randu0(100) < 20) {
                return 0;
            }
        }
    }
    if ((unsigned short)randu0(100) < 50 ||
        (piece->type == 0 && (unsigned short)randu0(100) < 25)) {
        x = target->cell_x;
        y = target->cell_y;
    } else {
        unsigned char old_x = piece->cell_x;
        int steps = (unsigned short)randu0(distance) + 1;
        int step_x = (int)target->cell_x - old_x;
        unsigned char old_y;
        int step_y;
        if (step_x > 0) {
            step_x = 1;
        } else if (step_x < 0) {
            step_x = -1;
        }
        old_y = piece->cell_y;
        step_y = (int)target->cell_y - old_y;
        if (step_y > 0) {
            step_y = 1;
        } else if (step_y < 0) {
            step_y = -1;
        }
        x = old_x + step_x * steps;
        y = old_y + step_y * steps;
    }
    if (((piece->move_map->rows[y] >> (3 * x)) & 7) == 0) {
        return 0;
    }
    drone->target_1_x = x;
    drone->target_1_y = y;
    return 1;
}

static inline void mk_chess_start_transferred_piece(ChessPiece* piece, int stack_kind) {
    ChessPieceProcPdata* data;

    piece->field_64 = stack_kind;
    if (stack_kind == 1) {
        piece->proc = _create_mkproc_generic_tinystack(
            0xC01E, 0x1F, mk_chess_piece_handle_event, sizeof(*data), (MkHdr**)&data);
    } else {
        piece->proc = _create_mkproc_generic_bigstack(
            0xC01E, 0x1F, mk_chess_piece_handle_event, sizeof(*data), (MkHdr**)&data);
    }
    if (piece->proc != 0 && piece->movement != 0 && data != 0) {
        G_BOARD_GAME_BIGSTACK_COUNTER++;
        data->piece = piece;
        piece->proc->pre_destroy = mk_chess_piece_pre_wake;
        piece->proc->destroy_cb = mk_chess_piece_post_sleep;
        set_process_as_scriptable(piece->proc);
    }
}

/* TODO: [near miss] 97.65958%; transfer paths and signedness agree; board-cell address lowering remains. */
int mk_chess_xfer_piece_from_scripts(int target, int script, int stack_kind) {
    ChessPiece* piece;

    if ((unsigned int)target == 2) {
        piece = g_active_piece;
    } else {
        ChessSpellState* spell = mk_chess_pdata->manager.spell;
        piece = mk_chess_pdata->board[(unsigned char)spell->target_x[target]].cells[
            (unsigned char)spell->target_y[target]].piece;
    }
    if (g_active_piece == piece) {
        return 0;
    }
    piece->state = 1;
    piece->runtime.fields.event_script = script;
    if (piece->proc == 0) {
        mk_chess_start_transferred_piece(piece, stack_kind);
    } else if ((int)piece->field_64 != stack_kind) {
        xfer_proc(piece->proc, mk_chess_piece_proc_force_dead);
        mk_chess_start_transferred_piece(piece, stack_kind);
    } else {
        xfer_proc(piece->proc, mk_chess_piece_handle_event);
    }
    return 1;
}

void mk_chess_xfer_to_piece_script(ChessPiece* piece, int event);

/* TODO: [breakthrough needed] 93.62421%; transfer boundary restored; roster search and address scheduling remain. */
static void mk_chess_handle_guy_falling_from_sky_event(ChessPiece* piece) {
    unsigned int index;
    unsigned int count = mk_chess_pdata->sides[piece->side]->live_piece_count;
    unsigned short available = 0;
    unsigned short selection;
    unsigned char x, y;
    for (index = 0; index < count; index++) {
        if (mk_chess_pdata->sides[piece->side]->pieces[index] == piece) {
            return;
        }
    }
    for (x = 3; x < 7; x++) {
        for (y = 3; y < 7; y++) {
            if (mk_chess_pdata->board[x].cells[y].piece == 0 &&
                mk_chess_pdata->board[(unsigned char)(x + 1)].cells[y].piece == 0) {
                available++;
            }
        }
    }
    selection = randu0(available);
    for (x = 3; x < 7; x++) {
        for (y = 3; y < 7; y++) {
            if (mk_chess_pdata->board[x].cells[y].piece == 0 &&
                mk_chess_pdata->board[(unsigned char)(x + 1)].cells[y].piece == 0) {
                if (selection == 0) {
                    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
                    piece->object->pos.value.x = cell->position.x - 1.3f;
                    piece->object->pos.value.y = 14.6f;
                    piece->object->pos.value.z = cell->position.z;
                    piece->object->ang.z = 0.0f;
                    piece->object->ang.y = 0.0f;
                    piece->object->ang.x = 0.0f;
                    piece->object->pos_vel.z = 0.0f;
                    piece->object->pos_vel.y = 0.0f;
                    piece->object->pos_vel.x = 0.0f;
                    piece->object->flags_08_bits.gravity_enabled = 0;
                    piece->object->flags_08_bits.rotation_enabled = 0;
                    update_obj_pos(piece->object);
                    mk_chess_xfer_to_piece_script(piece, 45);
                    g_active_piece = piece;
                    piece->object->flags_09_bits.bit6 = 0;
                    g_active_piece = 0;
                    _mkproc_sleep_ticks = 1.0f;
                    ((ChessProcVtable*)aproc->vtbl)->sleep();
                    unhide_obj(piece->object);
                    return;
                }
                selection--;
            }
        }
    }
}


static inline void mk_chess_react_idle_ally(int type, unsigned int occurrence)
{
    ChessPiece* target = mk_chess_find_piece_of_type(g_active_piece->side, type, occurrence);
    /* Retail requires the selected roster entry to exist on this event path. */
    if (g_active_piece != target && target->state == 0)
        mk_chess_xfer_to_piece_script(target, 28);
}

/* TODO: [near miss] 95.00%; dispatch traces agree; occurrence-selection and register scheduling remain. */
void mk_chess_piece_event_from_script(int event)
{
    ChessPieceMovement* movement = g_active_piece->movement;
    unsigned char x = movement->event_byte_12;
    unsigned char y = movement->event_byte_13;
    ChessPiece* target = mk_chess_pdata->board[x].cells[y].piece;
    ChessMovementEvent payload;
    unsigned int occurrence;
    switch (event) {
    case 6:
        memcpy(&payload, &movement->event_data, sizeof(payload));
        payload.piece = g_active_piece;
        mk_chess_piece_event(g_active_piece->movement->event_piece, event, &payload);
        break;
    case 9:
        mk_chess_xfer_to_piece_script(target, x <= 1 || x >= 8 || y <= 1 || y >= 8 ? 36 : 12);
        break;
    case 10: mk_chess_xfer_to_piece_script(target, 21); break;
    case 11: mk_chess_xfer_to_piece_script(target, 24); break;
    case 13:
        mk_chess_xfer_to_piece_script(target, x <= 1 || x >= 8 || y <= 1 || y >= 8 ? 38 : 14);
        break;
    case 20:
        memcpy(&payload, &movement->event_data, sizeof(payload));
        payload.piece = g_active_piece;
        mk_chess_piece_event(g_active_piece, event, &payload);
        break;
    case 16:
        target = mk_chess_find_piece_of_type(g_active_piece->side, 5, 0);
        if (g_active_piece != target) mk_chess_xfer_to_piece_script(target, 28);
        break;
    case 17:
        mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 5, 0), 29);
        break;
    case 21:
        target = mk_chess_find_piece_of_type(g_active_piece->side, 2, 1);
        if (g_active_piece != target) mk_chess_xfer_to_piece_script(target, 28);
        break;
    case 22:
        mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 2, 1), 29);
        break;
    case 18:
        for (occurrence = 0; occurrence < 4; occurrence++) {
            mk_chess_react_idle_ally(1, occurrence);
            mk_chess_react_idle_ally(3, occurrence);
            mk_chess_react_idle_ally(4, occurrence);
            mk_chess_react_idle_ally(2, occurrence);
        }
        break;
    case 19:
        for (occurrence = 0; occurrence < 4; occurrence++) {
            mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 1, occurrence), 29);
            mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 3, occurrence), 29);
            mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 4, occurrence), 29);
            mk_chess_xfer_to_piece_script(mk_chess_find_piece_of_type(g_active_piece->side == 0, 2, occurrence), 29);
        }
        break;
    case 25: mk_chess_random_specials_positive_reaction(); break;
    }
}


/* TODO: [breakthrough needed] 92.32026%; lookup/transfer recovered; redundant retail range-check island and local lowering remain. */
void mk_chess_xfer_to_piece_script(ChessPiece* piece, int event) {
    unsigned int script;
    int stack_kind = 0;

    if (piece != 0 && event < 64) {
        /* The existing event table also carries 32-bit command-script indices. */
        script = (unsigned int)g_board_game_controller.class_definitions[piece->type].event_scripts[event];
        if (script == 0xABABAB00) {
            script = (unsigned int)g_board_game_controller.piece_art_rows[event];
        }
        if (script != 0) {
            if (event < 28) {
                if (event == 0) {
                    stack_kind = 1;
                }
            } else if (event < 34) {
                stack_kind = 1;
            }
            piece->state = 1;
            piece->runtime.fields.event_script = script;
            if (piece->proc == 0) {
                mk_chess_start_transferred_piece(piece, stack_kind);
            } else if ((int)piece->field_64 != stack_kind) {
                xfer_proc(piece->proc, mk_chess_piece_proc_force_dead);
                mk_chess_start_transferred_piece(piece, stack_kind);
            } else {
                xfer_proc(piece->proc, mk_chess_piece_handle_event);
            }
        }
    }
}


/* TODO: [breakthrough needed] 81.71265%; payload extent repaired; verify trailing-byte consumers and remaining lowering. */
void mk_chess_request_piece_move(ChessPiece* piece, unsigned char x, unsigned char y, int end_turn) {
    unsigned int player;
    MkPtr* link;
    /* The event copies sixteen bytes; retail initializes only the coordinates. */
    unsigned char coordinates[sizeof(ChessMovementEvent)];
    unsigned int* restriction;
    ChessCell* previous;

    destroy_mkprocs_pid(0xC020);
    for (player = 10; player <= 15; player++) {
        bgnd_hide_pebbles(player);
    }
    link = piece->effects;
    while (link != 0) {
        ChessPieceEffect* effect = (ChessPieceEffect*)link->hdr;
        if (link->instance != effect->hdr.instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (effect->kind == 1) {
                if (effect->flags.bits.bit7) {
                    fx_pause_emit(effect->emitter);
                }
                if (effect->flags.bits.bit4) {
                    TrackedSound* sound = effect->sound;
                    if (sound != 0 && sound->hdr.instance != effect->sound_instance) {
                        sound = 0;
                    }
                    if (sound != 0) {
                        stop_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
                        if (sound->hdr.instance != 0) {
                            sound->hdr.typed_vtbl->destroy(&sound->hdr);
                        }
                    }
                    effect->sound = 0;
                    effect->sound_instance = 0;
                }
                if (effect->flags.bits.bit6) {
                    if (effect->kind == 2) {
                        effect->expiry_clock = 0xFFFFFFFF;
                    } else {
                        MkObj* object = mk_chess_live_imprison_object(effect);
                        if (object != 0 && object->hdr.instance != 0) {
                            object->hdr.typed_vtbl->destroy(&object->hdr);
                        }
                        if (effect->hdr.instance != 0) {
                            effect->hdr.typed_vtbl->destroy(&effect->hdr);
                        }
                    }
                }
            }
            link = link->next;
        }
    }
    restriction = piece != 0 ? &piece->access_restrictions[2] : 0;
    if (restriction != 0) {
        *restriction = 0;
    }
    mk_chess_set_game_mode(2);
    coordinates[0] = piece->cell_x;
    coordinates[1] = piece->cell_y;
    coordinates[2] = x;
    coordinates[3] = y;
    mk_chess_game_event(4, &piece, 1, coordinates);
    _mkproc_sleep_ticks = 2.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    previous = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    if (previous->piece == piece) {
        previous->piece = 0;
    }
    mk_chess_pdata->board[x].cells[y].piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    if (end_turn != 0) {
        mk_chess_end_of_turn();
    }
}

/* TODO: [breakthrough needed] 88.30282%; cursor motion recovered; float scheduling, normalization stores and latch lowering remain. */
void mk_chess_cursor_tracker_update(ChessSideHudState* hud) {
    Vec direction = {0.0f, 0.0f, 0.0f};
    MkObj* cursor = (MkObj*)mk_chess_pdata->cursor.object;
    ChessCell* cell = &mk_chess_pdata->board[mk_chess_pdata->cursor.cell_x].cells[
        mk_chess_pdata->cursor.cell_y];
    float original_x;
    float dx;
    float dz;
    float squared_distance;
    float step = 0.3f;
    float inverse_length;
    unsigned int arrived = 0;

    if (cursor != 0 && cursor->hdr.instance != mk_chess_pdata->cursor.object_instance) {
        cursor = 0;
    }
    original_x = cursor->pos.value.x;
    dx = cell->position.x - original_x;
    dz = cell->position.z - cursor->pos.value.z;
    squared_distance = dx * dx + dz * dz;
    if (squared_distance > 50.0f) {
        cursor->pos.value.x = cell->position.x;
        cursor->pos.value.z = cell->position.z;
        hud->flags &= ~0x80;
        return;
    }
    if (squared_distance > 14.0f) {
        step *= 6.0f;
    } else if (squared_distance > 9.0f) {
        step *= 3.0f;
    }
    if (dx > step) {
        direction.x = 1.0f;
    } else if (dx < -step) {
        direction.x = -1.0f;
    } else {
        arrived = 1;
    }
    if (dz > step) {
        direction.z = 1.0f;
    } else if (dz < -step) {
        direction.z = -1.0f;
    } else {
        arrived++;
    }
    if (arrived == 2) {
        cursor->pos.value.x = cell->position.x;
        cursor->pos.value.y = cell->position.y;
        cursor->pos.value.z = cell->position.z;
        hud->flags &= ~0x80;
        return;
    }
    inverse_length = mk_chess_inverse_vector_length(direction.x * direction.x + direction.z * direction.z);
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    direction.x *= step;
    cursor->pos.value.x = original_x + direction.x;
    direction.z *= step;
    cursor->pos.value.z += direction.z;
}

static inline MkObj* mk_chess_cursor_live_object(ChessCursor* cursor) {
    MkObj* object = (MkObj*)cursor->object;
    if (object != 0) {
        if (object->hdr.instance == cursor->object_instance) {
            return object;
        }
    }
    return 0;
}

static inline void mk_chess_prepare_reset_cursor(ChessCursor* cursor) {
    MkObj* object = mk_chess_cursor_live_object(cursor);
    MkSobj* sobj;
    obj_create_sobjs(object);
    sobj = obj_first_sobj(object);
    object->light_flags = 13;
    sobj_set_priority(sobj, 9);
    sobj->flags09_bits.bit6 = 1;
    sobj->flags09_bits.bit7 = 1;
    object->flags_08_bits.airborne = 1;
    insert_fgnd_mkobj(object);
    hide_obj(object);
}

static inline void mk_chess_position_reset_cursor(ChessCursor* cursor, unsigned char x, unsigned char y) {
    MkObj* object = mk_chess_cursor_live_object(cursor);
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    cursor->cell_x = x;
    cursor->cell_y = y;
    object->pos.value.x = cell->position.x;
    object->pos.value.y = cell->position.y;
    object->pos.value.z = cell->position.z;
    update_obj_pos(object);
}

/* TODO: [breakthrough needed] 83.14218%; setup and scan recovered; latch branches and address scheduling remain. */
void mk_chess_reset_cursors(void) {
    ChessPiece* piece;
    unsigned char x;
    unsigned char y;

    hide_obj(mk_chess_cursor_live_object(&mk_chess_pdata->cursors[0]));
    hide_obj(mk_chess_cursor_live_object(&mk_chess_pdata->cursors[1]));
    mk_chess_prepare_reset_cursor(&mk_chess_pdata->cursors[2]);
    mk_chess_prepare_reset_cursor(&mk_chess_pdata->cursor);
    piece = mk_chess_find_piece_on_board(mk_chess_pdata->manager.active_side);
    if (piece != 0) {
        y = piece->cell_y;
        x = piece->cell_x;
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[0], x, y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[1], x, y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursor, x, y);
    }
}

/* TODO: [breakthrough needed] 73.333336%; selection and tie-break recovered; absolute-value and cursor-address lowering remain. */
static void mk_chess_move_cursor_to_next_diagnal_piece(unsigned int side, unsigned int direction) {
    ChessPiece* selected = 0;
    unsigned char current_y = mk_chess_pdata->cursors[side].cell_y;
    unsigned char current_x = mk_chess_pdata->cursors[side].cell_x;
    int first_x, first_y, end_x, end_y;
    int x, y;
    unsigned int best_distance = 10000;
    unsigned int best_axis_distance = 10000;

    mk_chess_fetch_quadrant_params(current_x, current_y, direction,
        &first_x, &first_y, &end_x, &end_y, 0);
    for (x = first_x; x < end_x; x++) {
        for (y = first_y; y < end_y; y++) {
            ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
            if (piece != 0 && side == piece->side) {
                int dx = x - current_x;
                int dy = y - current_y;
                unsigned int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                unsigned int axis_distance;
                unsigned int adjusted_direction = direction;
                unsigned int quadrant = mk_chess_pdata->camera.viewing_quadrant;
                if (quadrant == 1 || quadrant == 3) {
                    adjusted_direction += 2;
                    if (adjusted_direction >= 8) {
                        adjusted_direction -= 8;
                    }
                }
                if (adjusted_direction == 0 || adjusted_direction == 4) {
                    axis_distance = dx < 0 ? -dx : dx;
                } else {
                    axis_distance = dy < 0 ? -dy : dy;
                }
                if (distance < best_distance ||
                    (distance == best_distance && axis_distance < best_axis_distance)) {
                    best_axis_distance = axis_distance;
                    best_distance = distance;
                    selected = mk_chess_pdata->board[x].cells[y].piece;
                }
            }
        }
    }
    if (selected != 0) {
        unsigned char selected_x;
        unsigned char selected_y;
        snd_req(0x36A);
        selected_y = selected->cell_y;
        selected_x = selected->cell_x;
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[0], selected_x, selected_y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[1], selected_x, selected_y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursor, selected_x, selected_y);
        mk_chess_hud_set_piece_portrait(mk_chess_pdata->board[selected->cell_x].cells[selected->cell_y].piece);
    }
}

static inline void mk_chess_scan_cursor_candidates(unsigned int side, unsigned int direction,
    int current_x, int current_y, int first_x, int first_y, int end_x, int end_y,
    ChessPiece** selected, unsigned int* best_distance, unsigned int* best_axis_distance) {
    int x, y;
    for (x = first_x; x < end_x; x++) {
        for (y = first_y; y < end_y; y++) {
            ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
            if (piece != 0 && side == piece->side) {
                int dx = x - current_x;
                int dy = y - current_y;
                unsigned int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                unsigned int axis_distance;
                unsigned int adjusted_direction = direction;
                unsigned int quadrant = mk_chess_pdata->camera.viewing_quadrant;
                if (quadrant == 1 || quadrant == 3) {
                    adjusted_direction += 2;
                    if (adjusted_direction >= 8) {
                        adjusted_direction -= 8;
                    }
                }
                if (adjusted_direction == 0 || adjusted_direction == 4) {
                    axis_distance = dx < 0 ? -dx : dx;
                } else {
                    axis_distance = dy < 0 ? -dy : dy;
                }
                if (distance < *best_distance ||
                    (distance == *best_distance && axis_distance < *best_axis_distance)) {
                    *best_axis_distance = axis_distance;
                    *best_distance = distance;
                    *selected = mk_chess_pdata->board[x].cells[y].piece;
                }
            }
        }
    }
}

/* TODO: [breakthrough needed] 72.1443%; directional and two-region search recovered; scan and cursor-address lowering remain. */
static void mk_chess_move_cursor_to_next_piece(unsigned int side, unsigned int direction) {
    ChessCursor* cursor = &mk_chess_pdata->cursors[side];
    int x = cursor->cell_x;
    int y = cursor->cell_y;
    int moving = 1;
    ChessPiece* selected = 0;
    int first_x, first_y, end_x, end_y;
    unsigned int best_distance = 10000;
    unsigned int best_axis_distance = 10000;

    while (moving != 0) {
        ChessPiece* piece;
        moving = move_cursor_based_on_quadrant(side, &x, &y, direction, 0, 10, 0);
        piece = mk_chess_pdata->board[x].cells[y].piece;
        if (piece != 0 && piece->side == side) {
            unsigned char selected_x;
            unsigned char selected_y;
            snd_req(0x36A);
            selected_y = y;
            selected_x = x;
            mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[0], selected_x, selected_y);
            mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[1], selected_x, selected_y);
            mk_chess_position_reset_cursor(&mk_chess_pdata->cursor, selected_x, selected_y);
            mk_chess_hud_set_piece_portrait(mk_chess_pdata->board[x].cells[y].piece);
            return;
        }
    }
    x = cursor->cell_x;
    y = cursor->cell_y;
    mk_chess_fetch_quadrant_params(x, y, direction, &first_x, &first_y, &end_x, &end_y, 0);
    mk_chess_scan_cursor_candidates(side, direction, x, y, first_x, first_y, end_x, end_y,
        &selected, &best_distance, &best_axis_distance);
    mk_chess_fetch_quadrant_params(x, y, direction, &first_x, &first_y, &end_x, &end_y, 1);
    if (selected == 0) {
        best_distance = 10000;
        best_axis_distance = 10000;
    }
    mk_chess_scan_cursor_candidates(side, direction, x, y, first_x, first_y, end_x, end_y,
        &selected, &best_distance, &best_axis_distance);
    if (selected != 0) {
        unsigned char selected_x;
        unsigned char selected_y;
        snd_req(0x36A);
        selected_y = selected->cell_y;
        selected_x = selected->cell_x;
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[0], selected_x, selected_y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursors[1], selected_x, selected_y);
        mk_chess_position_reset_cursor(&mk_chess_pdata->cursor, selected_x, selected_y);
        mk_chess_hud_set_piece_portrait(selected);
    }
}

static inline float mk_chess_table_square_root(float value) {
    union { float f; unsigned int u; } input, estimate;
    if (value <= 0.0f) {
        return 0.0f;
    }
    input.f = value;
    estimate.u = (unsigned int)GXMathSqrtTable[(input.u >> 11) & 0x1FFF] << 8;
    estimate.u |= (((input.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000;
    return 0.5f * (estimate.f * (3.0f - (estimate.f * estimate.f) / value));
}

/* TODO: [breakthrough needed] 81.90625%; offset rounding restored; floating-point scheduling and stack placement remain. */
static int mk_chess_choose_middle_control_points_for_zoom_cam_return(
    int position_class, int return_class, Vec* from, Vec* middle_1, Vec* middle_2, Vec* to) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int side_sign = -1;
    int timing = 12;
    Vec perpendicular = {0.0f, 0.0f, 0.0f};
    float dx, dy, dz;
    float inverse_length;
    if (manager->active_side == 0) {
        side_sign = 1;
    }
    dx = from->x - to->x;
    dz = from->z - to->z;
    dy = from->y - to->y;
    perpendicular.x = dz;
    perpendicular.z = -dx;
    inverse_length = mk_chess_inverse_vector_length(
        perpendicular.x * perpendicular.x + perpendicular.z * perpendicular.z);
    perpendicular.x *= inverse_length;
    perpendicular.z *= inverse_length;
    if (from->x * (float)side_sign < to->x * (float)side_sign) {
        perpendicular.x *= -1.0f;
        perpendicular.z *= -1.0f;
    }
    if (position_class == 1) {
        float distance = mk_chess_table_square_root(dx * dx + dz * dz);
        middle_1->x = 0.6f * dx;
        middle_1->y = 0.6f * dy;
        middle_1->z = 0.6f * dz;
        middle_1->x += to->x;
        middle_1->y += to->y;
        middle_1->z += to->z;
        middle_1->x += (float)(2.0f * perpendicular.x);
        middle_1->z += (float)(2.0f * perpendicular.z);
        middle_2->x = middle_1->x;
        middle_2->y = middle_1->y;
        middle_2->z = middle_1->z;
        if (distance > 26.0f) {
            timing = 11;
        }
    } else if (position_class == 2) {
        timing = 10;
        if (return_class == 4) {
            middle_1->x = 0.7f * dx;
            middle_1->y = 0.7f * dy;
            middle_1->z = 0.7f * dz;
            middle_1->x += to->x;
            middle_1->y += to->y;
            middle_1->z += to->z;
            middle_1->x += (float)(20.0f * perpendicular.x);
            middle_1->z += (float)(20.0f * perpendicular.z);
            middle_2->x = 0.1f * dx;
            middle_2->y = 0.1f * dy;
            middle_2->z = 0.1f * dz;
            middle_2->x += to->x;
            middle_2->y += to->y;
            middle_2->z += to->z;
            middle_2->x += (float)(40.0f * perpendicular.x);
            middle_2->z += (float)(40.0f * perpendicular.z);
        }
    }
    return timing;
}


static int mk_chess_choose_middle_control_points_for_zoom_cam(Vec* camera_position,
    unsigned int position_class, unsigned int height_class, Vec* from,
    Vec* middle_1, Vec* middle_2, Vec* to);

/* TODO: [breakthrough needed] 91.95279%; classification and timing recovered; FP scheduling and stack placement remain. */
static void mk_chess_set_up_zoom_cam(Vec* target) {
    Vec from;
    Vec middle_1;
    Vec middle_2;
    Vec to;
    CameraObj* camera = camera_live_node(&camera_item);
    ChessCameraInfo* info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int side_sign = -1;
    unsigned int position_class = 1;
    unsigned int height_class = 4;
    float target_x, target_z, camera_z;
    float to_x, to_z, from_x, from_z, inverse_length;
    int timing;

    if (manager->active_side == 0) {
        side_sign = 1;
    }
    from.x = camera->pos.x;
    from.y = camera->pos.y;
    from.z = camera->pos.z;
    target_x = target->x;
    target_z = target->z;
    to.x = target_x;
    to.z = target_z;
    to.y = 11.0f;
    to.z = 14.0f * (float)-side_sign + target_z;
    to_x = to.x - target_x;
    to_z = to.z - target_z;
    inverse_length = mk_chess_inverse_vector_length(to_x * to_x + to_z * to_z);
    to_x *= inverse_length;
    to_z *= inverse_length;
    camera_z = camera->pos.z;
    from_z = target_z - camera_z;
    from_x = target_x - camera->pos.x;
    inverse_length = mk_chess_inverse_vector_length(from_x * from_x + from_z * from_z);
    from_z *= inverse_length;
    from_x *= inverse_length;
    if (from_x * to_x + from_z * to_z > 0.75f &&
        ((camera_z < target_z && to.z > target_z) ||
         (camera_z > target_z && to.z < target_z))) {
        position_class = 2;
    }
    if (camera->pos.y > 33.8f) {
        height_class = 5;
    }
    timing = mk_chess_choose_middle_control_points_for_zoom_cam(&camera->pos,
        position_class, height_class, &from, &middle_1, &middle_2, &to);
    info->desired_look_at.x = target->x;
    info->desired_look_at.y = target->y;
    info->desired_look_at.z = target->z;
    info->desired_look_at.y = 2.25f;
    info->look_at_ticks = 40;
    BezierCamera_Init(&g_bezier_cam, 0.0f, &from, &middle_1, &middle_2, &to);
    switch (timing) {
    case 8:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00015f, -0.00005f, 0.7f, 0.72f, 0.0035f, 0.014f);
        break;
    case 9:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00085f, -0.00005f, 0.3f, 0.5f, 0.0035f, 0.025f);
        break;
    case 7:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.0005f, -0.0001f, 0.5f, 0.8f, 0.005f, 0.011f);
        break;
    case 6:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00025f, -0.0001f, 0.5f, 0.8f, 0.0035f, 0.008f);
        break;
    case 12:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 40.0f);
        break;
    case 11:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 70.0f);
        break;
    case 10:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 100.0f);
        break;
    }
}


/* TODO: [breakthrough needed] 88.02445%; geometry branches recovered; FP scheduling and stack placement remain. */
static int mk_chess_choose_middle_control_points_for_zoom_cam(Vec* camera_position,
    unsigned int position_class, unsigned int height_class, Vec* from,
    Vec* middle_1, Vec* middle_2, Vec* to) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int side_sign = -1;
    int timing = 12;
    Vec perpendicular = {0.0f, 0.0f, 0.0f};
    float dx, dy, dz, inverse_length;
    if (manager->active_side == 0) {
        side_sign = 1;
    }
    dx = camera_position->x - to->x;
    dz = camera_position->z - to->z;
    dy = camera_position->y - to->y;
    perpendicular.x = dz;
    perpendicular.z = -dx;
    inverse_length = mk_chess_inverse_vector_length(
        perpendicular.x * perpendicular.x + perpendicular.z * perpendicular.z);
    perpendicular.x *= inverse_length;
    perpendicular.z *= inverse_length;
    if (from->x * (float)side_sign < to->x * (float)side_sign) {
        perpendicular.x *= -1.0f;
        perpendicular.z *= -1.0f;
    }
    if (position_class == 1) {
        float from_dx = from->x - to->x;
        float from_dz = from->z - to->z;
        float distance = mk_chess_table_square_root(from_dx * from_dx + from_dz * from_dz);
        if (height_class == 4) {
            middle_1->x = 0.6f * dx;
            middle_1->y = 0.6f * dy;
            middle_1->z = 0.6f * dz;
            middle_1->x += to->x;
            middle_1->y += to->y;
            middle_1->z += to->z;
            middle_1->x += (float)(2.0f * perpendicular.x);
            middle_1->z += (float)(2.0f * perpendicular.z);
            middle_2->x = middle_1->x;
            middle_2->y = middle_1->y;
            middle_2->z = middle_1->z;
            if (distance > 26.0f) {
                timing = 11;
            }
        } else {
            timing = 10;
            if (distance < 10.0f) {
                middle_1->x = 0.6f * dx;
                middle_1->y = 0.6f * dy;
                middle_1->z = 0.6f * dz;
                middle_1->x += to->x;
                middle_1->y += to->y;
                middle_1->z += to->z;
                middle_1->z = 3.0f * to->z;
                middle_2->x = 0.2f * dx;
                middle_2->y = 0.2f * dy;
                middle_2->z = 0.2f * dz;
                middle_2->x += to->x;
                middle_2->y += to->y;
                middle_2->z += to->z;
                middle_2->z = 2.0f * to->z;
            } else {
                middle_1->x = 1.5f * dx;
                middle_1->y = 1.5f * dy;
                middle_1->z = 1.5f * dz;
                middle_1->x += to->x;
                middle_1->y += to->y;
                middle_1->z += to->z;
                middle_1->x += (float)(20.0f * perpendicular.x);
                middle_1->z += (float)(20.0f * perpendicular.z);
                middle_1->y *= 0.5f;
                middle_2->x = -0.3f * dx;
                middle_2->y = -0.3f * dy;
                middle_2->z = -0.3f * dz;
                middle_2->x += to->x;
                middle_2->y += to->y;
                middle_2->z += to->z;
                middle_2->x += (float)(40.0f * perpendicular.x);
                middle_2->z += (float)(40.0f * perpendicular.z);
            }
        }
    } else if (position_class == 2) {
        timing = 10;
        if (height_class == 4) {
            middle_1->x = 0.7f * dx;
            middle_1->y = 0.7f * dy;
            middle_1->z = 0.7f * dz;
            middle_1->x += to->x;
            middle_1->y += to->y;
            middle_1->z += to->z;
            middle_1->x += (float)(20.0f * perpendicular.x);
            middle_1->z += (float)(20.0f * perpendicular.z);
            middle_2->x = -0.3f * dx;
            middle_2->y = -0.3f * dy;
            middle_2->z = -0.3f * dz;
            middle_2->x += to->x;
            middle_2->y += to->y;
            middle_2->z += to->z;
            middle_2->x += (float)(40.0f * perpendicular.x);
            middle_2->z += (float)(40.0f * perpendicular.z);
        } else {
            middle_1->x = 1.5f * dx;
            middle_1->y = 1.5f * dy;
            middle_1->z = 1.5f * dz;
            middle_1->x += to->x;
            middle_1->y += to->y;
            middle_1->z += to->z;
            middle_1->x += (float)(20.0f * perpendicular.x);
            middle_1->z += (float)(20.0f * perpendicular.z);
            middle_1->y *= 0.5f;
            middle_2->x = -0.3f * dx;
            middle_2->y = -0.3f * dy;
            middle_2->z = -0.3f * dz;
            middle_2->x += to->x;
            middle_2->y += to->y;
            middle_2->z += to->z;
            middle_2->x += (float)(40.0f * perpendicular.x);
            middle_2->z += (float)(40.0f * perpendicular.z);
        }
    }
    return timing;
}


/* TODO: [breakthrough needed] 81.85058%; geometry and timing recovered; retail PSQ blocks runtime validation; inspect FP scheduling. */
static int mk_chess_choose_middle_control_points_for_fight_cam(
    const Vec* normal, const Vec* axis, const Vec* camera_position, const Vec* center,
    unsigned int camera_side, unsigned int camera_type, const Vec* camera_start,
    Vec* middle_0, Vec* middle_1, const Vec* camera_end) {
    Vec normal_unit = {0.0f, 0.0f, 0.0f};
    Vec between = {0.0f, 0.0f, 0.0f};
    Vec axis_unit = {0.0f, 0.0f, 0.0f};
    float dy = camera_start->y - camera_end->y;
    float dx = camera_start->x - camera_end->x;
    float dz = camera_start->z - camera_end->z;
    float camera_distance = mk_chess_table_square_root(dz * dz + (dx * dx + dy * dy));
    float inverse_length;
    float side_sign, height_adjust, first_offset, second_offset, distance_scale;
    int timing = 7;

    inverse_length = mk_chess_inverse_vector_length(normal->x * normal->x + normal->z * normal->z);
    normal_unit.x = normal->x * inverse_length;
    normal_unit.z = normal->z * inverse_length;
    inverse_length = mk_chess_inverse_vector_length(axis->x * axis->x + axis->z * axis->z);
    axis_unit.x = axis->x * inverse_length;
    axis_unit.z = axis->z * inverse_length;
    between.x = camera_position->x - center->x;
    between.y = camera_position->y - center->y;
    between.z = camera_position->z - center->z;
    if (camera_side == 1U) {
        if (camera_type == 5U) {
            side_sign = 1.0f;
            if (((axis_unit.x * camera_position->x) + (float)(axis_unit.z * camera_position->z)) < 0.0f) {
                side_sign = -1.0f;
            }
            timing = 6;
            middle_0->x = 0.5f * between.x;
            first_offset = 45.0f * side_sign;
            second_offset = 10.0f * side_sign;
            middle_0->y = 0.5f * between.y;
            middle_0->z = 0.5f * between.z;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->x = middle_0->x + (float)(35.0f * normal_unit.x);
            middle_0->z = middle_0->z + (float)(35.0f * normal_unit.z);
            middle_0->x = middle_0->x + (float)(axis_unit.x * first_offset);
            middle_0->z = middle_0->z + (float)(axis_unit.z * first_offset);
            middle_1->x = camera_end->x;
            middle_1->y = camera_end->y;
            middle_1->z = camera_end->z;
            middle_1->y = middle_0->y * 0.5f;
            middle_1->x = middle_1->x + (float)(50.0f * normal_unit.x);
            middle_1->z = middle_1->z + (float)(50.0f * normal_unit.z);
            middle_1->x = middle_1->x + (float)(axis_unit.x * second_offset);
            middle_1->z = middle_1->z + (float)(axis_unit.z * second_offset);
        } else {
            middle_0->x = 0.75f * between.x;
            distance_scale = camera_distance / 60.0f;
            middle_0->y = 0.75f * between.y;
            middle_0->z = 0.75f * between.z;
            first_offset = 62.0f * distance_scale;
            second_offset = 34.0f * distance_scale;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->x = middle_0->x + (float)(normal_unit.x * first_offset);
            middle_0->z = middle_0->z + (float)(normal_unit.z * first_offset);
            middle_0->y = middle_0->y * 0.65f;
            middle_1->x = 0.1f * between.x;
            middle_1->y = 0.1f * between.y;
            middle_1->z = 0.1f * between.z;
            middle_1->x = middle_1->x + center->x;
            middle_1->y = middle_1->y + center->y;
            middle_1->z = middle_1->z + center->z;
            middle_1->x = middle_1->x + (float)(normal_unit.x * second_offset);
            middle_1->z = middle_1->z + (float)(normal_unit.z * second_offset);
            if (camera_distance > 50.0f) {
                timing = 6;
            }
        }
    } else if (camera_side == 0U) {
        if (camera_type == 3U) {
            timing = 8;
            middle_0->x = 0.85f * between.x;
            middle_0->y = 0.85f * between.y;
            middle_0->z = 0.85f * between.z;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->y = 20.0f + middle_0->y;
            middle_1->x = 0.7f * between.x;
            middle_1->y = 0.7f * between.y;
            middle_1->z = 0.7f * between.z;
            middle_1->x = middle_1->x + center->x;
            middle_1->y = middle_1->y + center->y;
            middle_1->z = middle_1->z + center->z;
            middle_1->y = 1.5f + camera_end->y;
        } else if (camera_type == 4U) {
            timing = 9;
            middle_0->x = 0.9f * between.x;
            middle_0->y = 0.9f * between.y;
            middle_0->z = 0.9f * between.z;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->y = 1.5f + camera_end->y;
            middle_1->x = 0.7f * between.x;
            middle_1->y = 0.7f * between.y;
            middle_1->z = 0.7f * between.z;
            middle_1->x = middle_1->x + center->x;
            middle_1->y = middle_1->y + center->y;
            middle_1->z = middle_1->z + center->z;
            middle_1->y = camera_end->y;
        } else {
            height_adjust = 0.0f;
            if (camera_position->y > 40.0f) {
                height_adjust = 10.0f;
            }
            middle_0->x = 0.85f * between.x;
            first_offset = 35.0f + height_adjust;
            middle_0->y = 0.85f * between.y;
            middle_0->z = 0.85f * between.z;
            second_offset = 22.0f + height_adjust;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->x = middle_0->x + (float)(normal_unit.x * first_offset);
            middle_0->z = middle_0->z + (float)(normal_unit.z * first_offset);
            middle_1->x = 0.7f * between.x;
            middle_1->y = 0.7f * between.y;
            middle_1->z = 0.7f * between.z;
            middle_1->x = middle_1->x + center->x;
            middle_1->y = middle_1->y + center->y;
            middle_1->z = middle_1->z + center->z;
            middle_1->x = middle_1->x + (float)(normal_unit.x * second_offset);
            middle_1->z = middle_1->z + (float)(normal_unit.z * second_offset);
        }
    } else {
        if (camera_type == 5U) {
            side_sign = 1.0f;
            if (((axis_unit.x * camera_position->x) + (float)(axis_unit.z * camera_position->z)) < 0.0f) {
                side_sign = -1.0f;
            }
            middle_0->x = camera_start->x;
            first_offset = 90.0f * side_sign;
            middle_0->y = camera_start->y;
            second_offset = 25.0f * side_sign;
            middle_0->z = camera_start->z;
            middle_0->y = middle_0->y * 0.9f;
            middle_0->x = middle_0->x + (float)(-35.0f * normal_unit.x);
            middle_0->z = middle_0->z + (float)(-35.0f * normal_unit.z);
            middle_0->x = middle_0->x + (float)(axis_unit.x * first_offset);
            middle_0->z = middle_0->z + (float)(axis_unit.z * first_offset);
            middle_1->x = camera_end->x;
            middle_1->y = camera_end->y;
            middle_1->z = camera_end->z;
            middle_1->y = middle_0->y * 0.5f;
            middle_1->x = middle_1->x + (float)(50.0f * normal_unit.x);
            middle_1->z = middle_1->z + (float)(50.0f * normal_unit.z);
            middle_1->x = middle_1->x + (float)(axis_unit.x * second_offset);
            middle_1->z = middle_1->z + (float)(axis_unit.z * second_offset);
        } else {
            middle_0->x = 0.5f * between.x;
            middle_0->y = 0.5f * between.y;
            middle_0->z = 0.5f * between.z;
            middle_0->x = middle_0->x + center->x;
            middle_0->y = middle_0->y + center->y;
            middle_0->z = middle_0->z + center->z;
            middle_0->x = middle_0->x + (float)(60.0f * axis_unit.x);
            middle_0->z = middle_0->z + (float)(60.0f * axis_unit.z);
            middle_1->x = -1.0f * between.x;
            middle_1->y = -1.0f * between.y;
            middle_1->z = -1.0f * between.z;
            middle_1->x = middle_1->x + center->x;
            middle_1->y = middle_1->y + center->y;
            middle_1->z = middle_1->z + center->z;
            middle_1->y = middle_0->y * 0.5f;
            middle_1->x = middle_1->x + (float)(30.0f * axis_unit.x);
            middle_1->z = middle_1->z + (float)(30.0f * axis_unit.z);
        }
        timing = 6;
    }
    return timing;
}


/* TODO: [breakthrough needed] 93.37304%; camera setup recovered; retail PSQ and FP scheduling need validation. */
static void mk_chess_set_up_fight_cam(Vec* first, Vec* second, Vec* near_point, Vec* far_point) {
    Vec from;
    Vec middle_0;
    Vec middle_1;
    Vec to;
    Vec axis;
    Vec normal = {0.0f, 0.0f, 0.0f};
    Vec center;
    CameraObj* camera = camera_live_node(&camera_item);
    ChessCameraInfo* info = mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    float inverse_length;
    float normal_x, normal_z, approach_x, approach_z, dot;
    unsigned int camera_side = 1;
    unsigned int camera_type = 4;
    int timing;

    axis.x = second->x - first->x;
    axis.y = second->y - first->y;
    axis.z = second->z - first->z;
    center.x = 0.5f * axis.x;
    center.y = 0.5f * axis.y;
    center.z = 0.5f * axis.z;
    center.x += first->x;
    center.y += first->y;
    center.z += first->z;
    normal.x = axis.z;
    normal.z = -axis.x;
    normal.x *= -1.0f;
    normal.z *= -1.0f;
    from.x = camera->pos.x;
    from.y = camera->pos.y;
    from.z = camera->pos.z;
    to.y = 6.0f;
    to.z = 1.5f * (far_point->z - near_point->z);
    to.x = 1.5f * (far_point->x - near_point->x);
    to.z += near_point->z;
    to.x += near_point->x;
    inverse_length = mk_chess_inverse_vector_length((float)(normal.z * normal.z) +
        ((float)(normal.x * normal.x) + (float)(normal.y * normal.y)));
    to.x += (float)(15.0f * (float)(normal.x * inverse_length));
    to.z += (float)(15.0f * (float)(normal.z * inverse_length));
    inverse_length = mk_chess_inverse_vector_length(
        (float)(normal.x * normal.x) + (float)(normal.z * normal.z));
    normal_x = normal.x * inverse_length;
    normal_z = normal.z * inverse_length;
    approach_z = center.z - camera->pos.z;
    approach_x = center.x - camera->pos.x;
    inverse_length = mk_chess_inverse_vector_length(approach_x * approach_x + approach_z * approach_z);
    approach_z *= inverse_length;
    approach_x *= inverse_length;
    dot = approach_x * normal_x + approach_z * normal_z;
    if (dot < -0.91f) {
        camera_side = 0;
    } else if (dot > 0.82f) {
        camera_side = 2;
    }
    if (camera->pos.y < 22.5f) {
        camera_type = 3;
    } else if (camera->pos.y > 34.8f) {
        camera_type = 5;
    }
    timing = mk_chess_choose_middle_control_points_for_fight_cam(&normal, &axis,
        &camera->pos, &center, camera_side, camera_type, &from, &middle_0, &middle_1, &to);
    info->desired_look_at.x = center.x;
    info->desired_look_at.y = center.y;
    info->desired_look_at.z = center.z;
    info->desired_look_at.y = 2.25f;
    info->look_at_ticks = 80;
    info->look_at_completion = mk_chess_fight_cam_target_reached;
    BezierCamera_Init(&g_bezier_cam, 0.0f, &from, &middle_0, &middle_1, &to);
    switch (timing) {
    case 8:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00015f, -0.00005f, 0.7f, 0.72f, 0.0035f, 0.014f);
        break;
    case 9:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00085f, -0.00005f, 0.3f, 0.5f, 0.0035f, 0.025f);
        break;
    case 7:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.0005f, -0.0001f, 0.5f, 0.8f, 0.005f, 0.011f);
        break;
    case 6:
        BezierCamera_LinearAccelerateDeccelerate(&g_bezier_cam,
            0.00025f, -0.0001f, 0.5f, 0.8f, 0.0035f, 0.008f);
        break;
    case 12:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 40.0f);
        break;
    case 11:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 70.0f);
        break;
    case 10:
        BezierCamera_SetOverallCameraTimeInTicks(&g_bezier_cam, 100.0f);
        break;
    }
}


MslSoundHandle random_hit(int group);

/* TODO: [breakthrough needed] 90.966446%; canonical delayed-sound prototype restored; special-character branch lowering remains. */
void mk_chess_snd_request(unsigned int event) {
    int variant = g_board_game_controller.piece_libraries[g_active_piece->library_index].sound_variant;
    switch (event) {
    case 13:
        if (variant != 0) { snd_req(0x38C); } else { snd_req(0x38D); }
        break;
    case 12:
        if (variant != 0) { snd_req(0x38A); } else { snd_req(0x38B); }
        break;
    case 11:
        if (variant != 0) { snd_req(0x388); } else { snd_req(0x389); }
        break;
    case 1:
        if (g_active_piece->field_14 == 1) {
            snd_req(0x38E);
        } else {
            mk_chess_snd_request(8);
        }
        break;
    case 2:
        switch (g_active_piece->field_14) {
        case 1: snd_req(0x38E); break;
        case 0: snd_req(0x38F); break;
        default:
            if (variant != 0) { snd_req(0x37C); } else { snd_req(0x37E); }
            break;
        }
        break;
    case 3:
        if (variant != 0) { snd_req(0x380); } else { snd_req(0x383); }
        break;
    case 4: random_hit(2); break;
    case 5: random_hit(0); break;
    case 10: random_hit(12); break;
    case 7: snd_req(0x3A5); break;
    case 8:
        if (variant != 0) { snd_req(0x384); } else { snd_req(0x385); }
        break;
    case 9:
        if (variant != 0) { snd_req(0x386); } else { snd_req(0x387); }
        break;
    case 6:
        switch ((unsigned short)randu0(4)) {
        case 0: snd_req(0x3A1); break;
        case 1: snd_req(0x3A2); break;
        case 2: snd_req(0x3A3); break;
        case 3: snd_req(0x3A4); break;
        }
        break;
    }
}

typedef struct ChessMoveLinePdata {
    MkHdr hdr;
    int group;
    int pebble;
    float speed;
    float acceleration;
    float length;
    int attack;
    unsigned int target_x, target_y;
    unsigned int start_x, start_y;
    float angle;
    float next_marker;
    float spacing;
} ChessMoveLinePdata; /* 0x3C process allocation. */

static float p_mk_chess_move_line(void);

/* TODO: [breakthrough needed] 70.89349%; marker setup recovered; absolute-value lowering and allocation-output reloads remain. */
void mk_chess_place_move_line_from_to(int group, int pebble, unsigned int start_x,
    unsigned int start_y, unsigned int target_x, unsigned int target_y, int attack) {
    MkHdr* allocation = 0;
    int dx = (int)target_x - (int)start_x;
    int dy = (int)target_y - (int)start_y;
    unsigned int distance = dy < 0 ? -dy : dy;
    int direction = 0;
    ChessCell* cell;
    if (dx == 0) {
        if (dy <= 0) { direction = 4; }
    } else if (dy == 0) {
        distance = dx < 0 ? -dx : dx;
        direction = dx > 0 ? 2 : 6;
    } else {
        unsigned int abs_x = dx < 0 ? -dx : dx;
        if (distance < abs_x) { distance = abs_x; }
        if (dx > 0) { direction = dy > 0 ? 1 : 3; }
        else { direction = dy > 0 ? 7 : 5; }
    }
    cell = &mk_chess_pdata->board[start_x].cells[start_y];
    bgnd_unhide_pebbles(group);
    bgnd_pebble_set_current_pebble(group, pebble);
    bgnd_pebble_set_current_info(15, 4.0f);
    bgnd_pebble_set_current_info(10, 0.0f);
    bgnd_pebble_set_current_info(9, cell->position.x);
    bgnd_pebble_set_current_info(11, cell->position.z);
    bgnd_pebble_set_current_info(14, 0.0f);
    bgnd_pebble_set_current_info(12, 1.0f);
    bgnd_unhide_pebbles(group + 4);
    if (_create_mkproc_generic_tinystack(0xC020, 31, p_mk_chess_move_line,
            sizeof(ChessMoveLinePdata), &allocation) != 0) {
        ChessMoveLinePdata* data = (ChessMoveLinePdata*)allocation;
        float spacing;
        float degrees;
        data->group = group;
        data->pebble = pebble;
        data->speed = 0.01f;
        data->acceleration = 0.02f;
        data->attack = attack;
        spacing = start_x == target_x || start_y == target_y ? 1.0f : 1.414f;
        data->length = (float)distance * spacing - spacing;
        if (attack == 0) { data->length += spacing; }
        else { data->length = spacing * 0.5f + data->length; }
        data->target_x = target_x;
        data->target_y = target_y;
        data->start_x = start_x;
        data->start_y = start_y;
        degrees = 45.0f * (float)direction;
        data->next_marker = spacing;
        data->spacing = spacing;
        data->angle = 0.017453292f * degrees;
        bgnd_pebble_set_current_info(7, degrees);
    }
}


void rotate_xz(Vec* out, const Vec* vector, float angle);

static inline void mk_chess_emit_move_line_marker(ChessMoveLinePdata* data, float distance) {
    Vec offset = {0.0f, 0.0f, 0.0f};
    Vec position = {0.0f, 0.0f, 0.0f};
    ChessCell* cell = &mk_chess_pdata->board[data->start_x].cells[data->start_y];
    unsigned int index = 0;
    int group;
    position.x = cell->position.x;
    position.z = cell->position.z;
    offset.z = 2.0f * distance;
    rotate_xz(&offset, &offset, data->angle);
    position.x += offset.x;
    group = data->group + 4;
    position.z += offset.z;
    for (;;) {
        bgnd_pebble_set_current_pebble(group, index);
        if (bgnd_pebble_fetch_current_info(15) == 0.0f) {
            break;
        }
        index++;
        if (index >= 36) {
            index = 0;
            break;
        }
    }
    bgnd_pebble_set_current_pebble(data->group + 4, index);
    bgnd_pebble_set_current_info(15, 9.0f);
    bgnd_pebble_set_current_info(10, 0.0f);
    bgnd_pebble_set_current_info(9, position.x);
    bgnd_pebble_set_current_info(11, position.z);
    bgnd_pebble_set_current_info(14, 0.8f);
    bgnd_pebble_set_current_info(12, 0.8f);
    bgnd_pebble_set_current_info(4, 3.0f + frand(2.0f));
}

/* TODO: [breakthrough needed] 76.8%; stages recovered; vector initialization and marker-loop scheduling remain. */
static float p_mk_chess_move_line(void) {
    ChessMoveLinePdata* data = (ChessMoveLinePdata*)apdata;
    float current;
    float limit;
    bgnd_pebble_set_current_pebble(data->group, data->pebble);
    current = bgnd_pebble_fetch_current_info(14);
    current += data->speed;
    limit = data->length;
    if (current > limit) {
        current = limit;
    }
    bgnd_pebble_set_current_info(14, current);
    data->speed += data->acceleration;
    if (current >= data->length) {
        if (data->attack != 0) {
            ChessCell* cell = &mk_chess_pdata->board[data->target_x].cells[data->target_y];
            bgnd_unhide_pebbles(data->group + 2);
            bgnd_pebble_set_current_pebble(data->group + 2, data->pebble);
            bgnd_pebble_set_current_info(15, 4.0f);
            bgnd_pebble_set_current_info(10, 0.0f);
            bgnd_pebble_set_current_info(9, cell->position.x);
            bgnd_pebble_set_current_info(11, cell->position.z);
            bgnd_pebble_set_current_info(14, 1.0f);
            bgnd_pebble_set_current_info(12, 1.0f);
            bgnd_pebble_set_current_info(13, 0.0f);
            data->speed = 0.02f;
            data->acceleration = 0.08f;
            data->length = 1.7f;
            ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_attack_burst, 0.0f);
            return 0.0f;
        }
        mk_chess_emit_move_line_marker(data, current);
        return -1.0f;
    }
    if (current > data->next_marker) {
        mk_chess_emit_move_line_marker(data, data->next_marker);
        data->next_marker += data->spacing;
    }
    return 1.0f;
}


float ang_sub_ang(float first, float second);

/* TODO: [breakthrough needed] 88.552%; rotation loops recovered; FP scheduling and coordinate conversion remain. */
void mk_chess_rotate_towards_cell(int track_other, float x, float y, float step, float offset) {
    ChessCell* target_cell = &mk_chess_pdata->board[(unsigned int)x].cells[(unsigned int)y];
    ChessPiece* other = mk_chess_pdata->board[(unsigned char)(int)x].cells[(unsigned char)(int)y].piece;
    ChessCell* start_cell = &mk_chess_pdata->board[g_active_piece->cell_x].cells[g_active_piece->cell_y];
    float desired, difference;
    float other_desired = 0.0f;
    float other_difference = 0.0f;
    MkObj* object;
    ChessAnimPdata* animation;
    object = g_active_piece->object;
    desired = ang_sub_ang(gxMathArcTanYX(target_cell->position.x - object->pos.value.x,
        target_cell->position.z - object->pos.value.z), offset);
    difference = ang_sub_ang(desired, g_active_piece->object->ang.y);
    if (track_other != 0) {
        object = other->object;
        other_desired = ang_sub_ang(gxMathArcTanYX(start_cell->position.x - object->pos.value.x,
            start_cell->position.z - object->pos.value.z), offset);
        other_difference = ang_sub_ang(other_desired, other->object->ang.y);
    }
    while ((difference >= 0.0f ? difference : -difference) > step) {
        if (difference < 0.0f) {
            g_active_piece->object->ang.y -= step;
        } else {
            g_active_piece->object->ang.y += step;
        }
        /* Retail updates this yaw even when target-angle tracking is disabled. */
        if (other_difference < 0.0f) {
            other->object->ang.y -= step;
        } else {
            other->object->ang.y += step;
        }
        animation = g_active_piece->animation;
        advance_anim((AnimPdata*)animation);
        pose_anim((AnimPdata*)animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        object = g_active_piece->object;
        desired = ang_sub_ang(gxMathArcTanYX(target_cell->position.x - object->pos.value.x,
            target_cell->position.z - object->pos.value.z), offset);
        difference = ang_sub_ang(desired, g_active_piece->object->ang.y);
        if (track_other != 0) {
            object = other->object;
            other_desired = ang_sub_ang(gxMathArcTanYX(start_cell->position.x - object->pos.value.x,
                start_cell->position.z - object->pos.value.z), offset);
            other_difference = ang_sub_ang(other_desired, other->object->ang.y);
        }
    }
    g_active_piece->object->ang.y = desired;
    if (track_other != 0) {
        while ((other_difference >= 0.0f ? other_difference : -other_difference) > step) {
            if (other_difference < 0.0f) {
                other->object->ang.y -= step;
            } else {
                other->object->ang.y += step;
            }
            animation = g_active_piece->animation;
            advance_anim((AnimPdata*)animation);
            pose_anim((AnimPdata*)animation, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            object = other->object;
            other_desired = ang_sub_ang(gxMathArcTanYX(start_cell->position.x - object->pos.value.x,
                start_cell->position.z - object->pos.value.z), offset);
            other_difference = ang_sub_ang(other_desired, other->object->ang.y);
        }
    }
    other->object->ang.y = other_desired;
}

/* TODO: [breakthrough needed] 93.140495%; movement phases recovered; coordinate conversion and FP scheduling remain. */
void mk_chess_ani_until_reached_destination(int airborne, float start_x, float start_y,
    float target_x, float target_y, float frames) {
    ChessBoardRow* board = mk_chess_pdata->board;
    ChessCell* target = &board[(unsigned int)target_x].cells[(unsigned int)target_y];
    ChessCell* start = &board[(unsigned int)start_x].cells[(unsigned int)start_y];
    float start_height = g_active_piece->object->pos.value.y;
    unsigned int descending = 0;
    unsigned int bob_ticks = 20;
    ChessAnimPdata* animation;

    g_active_piece->object->pos_vel.z = 0.0f;
    g_active_piece->object->pos_vel.y = 0.0f;
    g_active_piece->object->pos_vel.x = 0.0f;
    g_active_piece->object->flags_08_bits.moving = 0;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 1;
    g_active_piece->object->flags_09_bits.bit6 = 0;
    if (airborne == 1) {
        bob_ticks = 19;
        g_active_piece->object->pos_vel.y = 0.02f;
        do {
            animation = g_active_piece->animation;
            advance_anim((AnimPdata*)animation);
            pose_anim((AnimPdata*)animation, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        } while (bob_ticks-- != 0);
        bob_ticks = 20;
    }
    g_active_piece->object->pos_vel.x = (target->position.x - start->position.x) / frames;
    g_active_piece->object->pos_vel.z = (target->position.z - start->position.z) / frames;
    if (airborne == 1) {
        g_active_piece->object->pos_vel.y = 0.013f;
        while (frames-- > 0.0f) {
            animation = g_active_piece->animation;
            advance_anim((AnimPdata*)animation);
            pose_anim((AnimPdata*)animation, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            if (bob_ticks-- == 0) {
                if (descending == 0) {
                    bob_ticks = 8;
                    descending = 1;
                    g_active_piece->object->pos_vel.y = -0.013f;
                } else {
                    bob_ticks = 8;
                    descending = 0;
                    g_active_piece->object->pos_vel.y = 0.013f;
                }
            }
        }
        g_active_piece->object->pos_vel.z = 0.0f;
        g_active_piece->object->pos_vel.y = 0.0f;
        g_active_piece->object->pos_vel.x = 0.0f;
        g_active_piece->object->pos_vel.y = -0.02f;
        while (g_active_piece->object->pos.value.y > start_height) {
            animation = g_active_piece->animation;
            advance_anim((AnimPdata*)animation);
            pose_anim((AnimPdata*)animation, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        }
        g_active_piece->object->pos.value.y = start_height;
    } else {
        animation = g_active_piece->animation;
        while (frames > 0.0f) {
            advance_anim((AnimPdata*)animation);
            pose_anim((AnimPdata*)animation, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            frames -= 1.0f;
        }
    }
    g_active_piece->object->flags_08_bits.moving = 0;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 0;
    g_active_piece->object->flags_09_bits.bit6 = 1;
}

/* TODO: [breakthrough needed] 95.18033%; spell states recovered; cursor and process setup scheduling remain. */
static float p_mk_chess_cast_spell(void) {
    ChessSpellState* spell = (ChessSpellState*)apdata;
    switch (spell->state) {
    case 20: {
        ChessPiece* caster = spell->caster;
        unsigned char x = caster->cell_x;
        unsigned char y = caster->cell_y;
        ChessCursor* cursor = &mk_chess_pdata->cursors[spell->side];
        ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
        MkObj* object = (MkObj*)cursor->object;
        if (object != 0) {
            if (object->hdr.instance != cursor->object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        cursor->cell_x = x;
        cursor->cell_y = y;
        object->pos.value.x = cell->position.x;
        object->pos.value.y = cell->position.y;
        object->pos.value.z = cell->position.z;
        update_obj_pos(object);
        caster = spell->caster;
        caster->state = 1;
        /* The nonzero availability value is also the spell's event script. */
        caster->runtime.fields.event_script = g_board_game_controller.class_definitions[
            caster->type].spell_definitions[spell->spell_number].enabled;
        if (caster->proc == 0) {
            mk_chess_start_transferred_piece(caster, 0);
        } else if (caster->field_64 != 0) {
            xfer_proc(caster->proc, mk_chess_piece_proc_force_dead);
            mk_chess_start_transferred_piece(caster, 0);
        } else {
            xfer_proc(caster->proc, mk_chess_piece_handle_event);
        }
        spell->state = 21;
        mk_chess_pdata->turn_timeout = 3000;
        break;
    }
    case 22:
        spell->caster->used_spells |= 1U << spell->spell_number;
        break;
    case 23:
        turn_controllers_on();
        spell->caster->used_spells |= 1U << spell->spell_number;
        board_game_save_data.sides[spell->side].forced_fight_count++;
        mk_chess_end_of_turn();
        return -1.0f;
    }
    return 1.0f;
}

static inline void mk_chess_start_piece_initialization(ChessPiece* piece) {
    MkHdr* allocation;
    piece->field_64 = 0;
    piece->proc = _create_mkproc_generic_bigstack(
        0xC01E, 0x1F, p_mk_chess_piece_init, sizeof(ChessPieceProcPdata), &allocation);
    if (piece->proc != 0 && piece->movement != 0 && allocation != 0) {
        ChessPieceProcPdata* data = (ChessPieceProcPdata*)allocation;
        G_BOARD_GAME_BIGSTACK_COUNTER++;
        data->piece = piece;
        piece->proc->pre_destroy = mk_chess_piece_pre_wake;
        piece->proc->destroy_cb = mk_chess_piece_post_sleep;
        set_process_as_scriptable(piece->proc);
    }
}

/* TODO: [near miss] 99.95%; data-value exact; anonymous relocation identity remains. */
static void mk_chess_alive_pieces_initialize(void) {
    unsigned int side;
    unsigned int i;
    for (side = 0; side < 2; side++) {
        if (mk_chess_pdata->sides[side]->live_piece_count >= 7) {
            for (i = 0; i < 6; i++) {
                mk_chess_start_piece_initialization(mk_chess_pdata->sides[side]->pieces[i]);
            }
            for (i = 6; i < mk_chess_pdata->sides[side]->live_piece_count; i++) {
                mk_chess_start_piece_initialization(mk_chess_pdata->sides[side]->pieces[i]);
                while (mk_chess_pdata->sides[side]->pieces[i]->proc != 0) {
                    _mkproc_sleep_ticks = 1.0f;
                    ((ChessProcVtable*)aproc->vtbl)->sleep();
                }
            }
        } else {
            for (i = 0; i < mk_chess_pdata->sides[side]->live_piece_count; i++) {
                mk_chess_start_piece_initialization(mk_chess_pdata->sides[side]->pieces[i]);
                while (mk_chess_pdata->sides[side]->pieces[i]->proc != 0) {
                    _mkproc_sleep_ticks = 1.0f;
                    ((ChessProcVtable*)aproc->vtbl)->sleep();
                }
            }
        }
    }
}

/* TODO: [breakthrough needed] 90.477066%; HUD argument recovered; text/list scheduling remains. */
static void mk_chess_spell_hud_show_my_spells(ChessHudState* hud) {
    int text_x = 118;
    int marker_x = 70;
    int row_offset = 0;
    unsigned int marker = 3;
    unsigned int target;
    MkPtr* link;
    int has_strings = 0;
    MkHdr* allocation;
    if (hud->side == 1) {
        text_x = screen_width - 101;
        marker_x = screen_width - 151;
    }
    hud->strings = 0;
    for (target = 0; target < 2; target++) {
        ChessPiece* caster = mk_chess_find_spellcaster_on_side(hud->side, target);
        if (caster != 0) {
            unsigned int spell;
            for (spell = 0; spell < 4; spell++) {
                StringObj* string;
                if (mk_chess_hud_spell_available(caster, spell) == 0) {
                    ScreenObj* image = mk_chess_latched_screen(&mk_chess_pdata->manager.spell_hud[marker]);
                    image->pfx2d->verts[0].a = 255;
                    image->x = marker_x - image->pfx2d->tex_w / 2;
                    image->y = screen_height / 2 + 82 - row_offset;
                    unhide_screen_obj(image);
                    mk_insert((MkHdr*)image, &hud->images);
                    marker++;
                }
                string = string_center_xy(0xC01C, 8,
                    get_string_by_id(g_board_game_controller.class_definitions[caster->type]
                        .spell_definitions[spell].name_id | 0x20000),
                    text_x, screen_height / 2 + 101 - row_offset, 0x4E);
                if (caster->type == 3) {
                    string->pfx.instance0.native_color.r = 0x13;
                    string->pfx.instance0.native_color.g = 0x8A;
                    string->pfx.instance0.native_color.b = 0xFC;
                } else {
                    string->pfx.instance0.native_color.r = 0xFC;
                    string->pfx.instance0.native_color.g = 0x84;
                    string->pfx.instance0.native_color.b = 0x1A;
                }
                mk_insert((MkHdr*)string, &hud->strings);
                row_offset += 28;
            }
        }
    }
    link = hud->strings;
    while (link != 0) {
        StringObj* string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            string->pfx.instance0.native_color.a = 0;
            has_strings = 1;
            link = link->next;
        }
    }
    if (has_strings && _create_mkproc_generic_tinystack(
            0xC022, 0x1F, p_mk_chess_spell_hud_string_fade,
            sizeof(ChessStringFadePdata), &allocation)) {
        ChessStringFadePdata* fade = (ChessStringFadePdata*)allocation;
        fade->strings = &hud->strings;
        fade->completed = 0;
        fade->owner = hud;
        fade->step = (int)(20.0f * game_speed);
    }
}


/* TODO: [breakthrough needed] 98.69159%; winning-side field corrected; UI and statistics scheduling remain. */
static void mk_chess_game_over(void) {
    unsigned int winner = mk_chess_pdata->manager.winning_side;
    unsigned int elapsed = (unsigned int)board_game_save_data.last_update_tick - (unsigned int)exec_tick_ctr;
    ScreenObj* image = load_named_2d_pfxobj(0xE003E, 0xC01C, "WIN_SQUARE", 0, 0x52);
    StringObj* name;
    StringObj* title;
    image->y = (screen_height - image->pfx2d->tex_h) / 2;
    image->x = (screen_width - image->pfx2d->tex_w) / 2;
    unhide_screen_obj(image);
    snd_req_delay(0x29, 100);
    if (winner == 0) {
        name = string_center_xy(0xC01C, 3, get_string_by_id(0x2004E),
            screen_width / 2, screen_height / 2 - 5, 0x4E);
        name->pfx.instance0.native_color.b = 200;
        name->pfx.instance0.native_color.g = 100;
        name->pfx.instance0.native_color.r = 100;
        snd_req_delay(0x2A, 190);
    } else {
        name = string_center_xy(0xC01C, 3, get_string_by_id(0x2004F),
            screen_width / 2, screen_height / 2 - 5, 0x4E);
        name->pfx.instance0.native_color.b = 100;
        name->pfx.instance0.native_color.g = 100;
        name->pfx.instance0.native_color.r = 200;
        snd_req_delay(0x2B, 190);
    }
    title = string_center_xy(0xC01C, 3, get_string_by_id(0x2004D),
        screen_width / 2, screen_height / 2 - 35, 0x4E);
    if (winner == 0) {
        title->pfx.instance0.native_color.b = 200;
        title->pfx.instance0.native_color.g = 100;
        title->pfx.instance0.native_color.r = 100;
    } else {
        title->pfx.instance0.native_color.b = 100;
        title->pfx.instance0.native_color.g = 100;
        title->pfx.instance0.native_color.r = 230;
    }
    unhide_string_obj(name);
    unhide_string_obj(title);
    _mkproc_sleep_ticks = 400.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    fade_to_black(8, 0);
    if (name->instance != 0) name->typed_vtbl->destroy(name);
    if (title->instance != 0) title->typed_vtbl->destroy(title);
    if (image->instance != 0) image->typed_vtbl->destroy(image);
    board_game_save_data.sides[0].field_704 = mk_chess_pdata->sides[0]->live_piece_count;
    board_game_save_data.sides[1].field_704 = mk_chess_pdata->sides[1]->live_piece_count;
    board_game_save_data.winning_side = winner;
    if ((unsigned int)board_game_save_data.field_16B4 < (unsigned int)mk_chess_pdata->manager.clock) {
        board_game_save_data.field_16B4 = mk_chess_pdata->manager.clock;
    }
    if ((unsigned int)board_game_save_data.profile_stat_ceiling > (unsigned int)mk_chess_pdata->manager.clock) {
        board_game_save_data.profile_stat_ceiling = mk_chess_pdata->manager.clock;
    }
    if ((unsigned int)board_game_save_data.field_16BC < elapsed) {
        board_game_save_data.field_16BC = elapsed;
    }
    if (board_game_save_data.input_flags.input_locked) {
        gamelogic_jump(0, p_attract_mode);
        return;
    }
    if ((g_game_info.field_04 & 0x80) == 0) ck_do_profile_save();
    gamelogic_jump(5, p_mk_chess_game_over);
}

/* TODO: [breakthrough needed] 86.20548%; low-byte rule fixed; captured search and HUD scheduling remain. */
static void mk_chess_request_for_target(ChessHudState* hud) {
    int text_x = 118;
    unsigned int side = hud->side;
    ChessPiece* piece = 0;
    int type;
    if (side == 1) text_x = screen_width - 101;
    if ((unsigned char)hud->target_rules != 8) return;
    for (type = 0; type < 6; type++) {
        ChessSideState* team = mk_chess_pdata->sides[side];
        unsigned int i;
        for (i = 17 - team->captured_piece_count; i < 17; i++) {
            if (team->pieces[i]->type == type) {
                piece = team->pieces[i];
                break;
            }
        }
        if (piece != 0) break;
    }
    if (piece != 0) {
        ScreenObj* image;
        StringObj* string;
        MkPtr* link;
        int has_strings = 0;
        MkHdr* allocation;
        hud->images = 0;
        image = mk_chess_latched_screen(&mk_chess_pdata->manager.spell_hud[11]);
        image->x = text_x - image->pfx2d->tex_w / 2 + 8;
        image->y = 155;
        image->pfx2d->verts[0].a = 255;
        unhide_screen_obj(image);
        mk_insert((MkHdr*)image, &hud->images);
        image = mk_chess_latched_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
        image->pfx2d->verts[0].a = 255;
        hud->image_58 = (MkHdr*)image;
        hud->rescue_piece_type = piece->type;
        hud->state = 10;
        image->pfx2d->verts[0].a = 255;
        image->y = 200;
        if (hud->side == 0) {
            image->x = 52;
            if (image->pfx2d->tex_w > 255) image->x -= 13;
        } else {
            image->x = screen_width - 170;
            if (image->pfx2d->tex_w > 255) image->x -= 113;
        }
        unhide_screen_obj(image);
        turn_controllers_on();
        hud->state = 11;
        string = string_center_xy(0xC01F, 8,
            g_board_game_controller.class_definitions[piece->type].text->rescue_name,
            text_x, 155, 0x4E);
        string->pfx.instance0.native_color.r = 0x66;
        string->pfx.instance0.native_color.g = 0x33;
        string->pfx.instance0.native_color.b = 0;
        mk_insert((MkHdr*)string, &hud->strings);
        link = hud->strings;
        while (link != 0) {
            string = (StringObj*)link->hdr;
            if (link->instance != string->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                string->pfx.instance0.native_color.a = 0;
                has_strings = 1;
                link = link->next;
            }
        }
        if (has_strings && _create_mkproc_generic_tinystack(
                0xC022, 0x1F, p_mk_chess_spell_hud_string_fade,
                sizeof(ChessStringFadePdata), &allocation)) {
            ChessStringFadePdata* fade = (ChessStringFadePdata*)allocation;
            fade->strings = &hud->strings;
            fade->completed = 0;
            fade->owner = hud;
            fade->step = (int)(25.0f * game_speed);
        }
    } else {
        hud->image_58 = 0;
        hud->state = 11;
        turn_controllers_on();
    }
}

typedef struct ChessRingObjectRef {
    MkObj* object;
    unsigned int instance;
} ChessRingObjectRef;

typedef struct ChessMagicRingsPdata {
    MkHdr hdr;
    char pad08[4];
    Vec position;
    int mode;
    float maximum_scale;
    float fade_start_scale;
    float scale_per_tick;
    int alpha_step;
    int fade_ticks;
    float initial_scale;
    Vec scale;
    ChessRingObjectRef rings[2];
    int phase[2];
    float elapsed[2];
    int active[2];
} ChessMagicRingsPdata; /* 0x68 */

static float p_magic_rings_fx(void);

/* TODO: [breakthrough needed] 78.00966%; ring payload recovered; allocation reloads and model scheduling remain. */
static void start_magic_rings_effect(Vec* position, Vec* scale, const char* model, int mode,
    float initial_scale, float maximum_scale, float fade_start_scale, float scale_per_tick) {
    MkObj* objects[2];
    MkHdr* allocation;
    MkProc* process;
    ChessMagicRingsPdata* data;
    unsigned int i;
    objects[0] = load_named_model_from_slot(0xD003C, model, 0xC01E, 0);
    if (objects[0] == 0) return;
    objects[1] = load_named_model_from_slot(0xD003C, model, 0xC01E, 0);
    if (objects[1] == 0) return;
    process = _create_mkproc_generic_tinystack(0xC023, 0x20, p_magic_rings_fx,
        sizeof(ChessMagicRingsPdata), &allocation);
    if (process == 0) return;
    zero_pdata_payload(sizeof(ChessMagicRingsPdata), allocation);
    data = (ChessMagicRingsPdata*)allocation;
    data->maximum_scale = maximum_scale;
    data->fade_start_scale = fade_start_scale;
    data->scale_per_tick = scale_per_tick;
    data->alpha_step = 255 / (int)((data->maximum_scale - data->fade_start_scale) / data->scale_per_tick);
    data->mode = mode;
    data->fade_ticks = 0;
    data->position.x = position->x;
    data->position.y = position->y;
    data->position.z = position->z;
    data->position.y = 0.02f;
    data->initial_scale = initial_scale;
    data->scale.x = scale->x;
    data->scale.y = scale->y;
    data->scale.z = scale->z;
    for (i = 0; i < 2; i++) {
        MkObj* object = objects[i];
        MkSobj* sobj;
        data->phase[i] = 0;
        data->elapsed[i] = 1.0f + initial_scale / data->scale_per_tick;
        data->active[i] = 0;
        data->rings[i].object = 0;
        data->rings[i].instance = 0;
        data->rings[i].object = object;
        data->rings[i].instance = object->hdr.instance;
        mk_insert(&object->hdr, &process->pdata_list_b);
        obj_create_sobjs(object);
        sobj = obj_first_sobj(object);
        object->light_flags = 13;
        sobj_set_priority(sobj, 9);
        sobj->flags09_bits.bit6 = 1;
        sobj->flags09_bits.bit7 = 1;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        object->pos.value.x = data->position.x;
        object->pos.value.y = data->position.y;
        object->pos.value.z = data->position.z;
        object->pos.value.y = 0.0001f;
        object->flags_08_bits.scale_active = 1;
        object->scale.x = scale->x * initial_scale;
        object->scale.y = scale->y * initial_scale;
        object->scale.z = scale->z * initial_scale;
        update_obj_pos(object);
    }
    data->active[0] = 1;
    unhide_obj(objects[0]);
    hide_obj(objects[1]);
}

static inline MkObj* mk_chess_live_ring(ChessRingObjectRef* ref) {
    MkObj* object = ref->object;
    if (object != 0 && object->hdr.instance == ref->instance) return object;
    return 0;
}

static inline void mk_chess_restart_ring(ChessMagicRingsPdata* data, unsigned int index) {
    RwRGBA color = {255, 255, 255, 255};
    MkObj* object = mk_chess_live_ring(&data->rings[index]);
    if (object == 0) {
        data->active[index] = 0;
    } else {
        data->phase[index] = 0;
        data->elapsed[index] = 1.0f + data->initial_scale / data->scale_per_tick;
        data->active[index] = 1;
        object->pos.value.x = data->position.x;
        object->pos.value.y = data->position.y;
        object->pos.value.z = data->position.z;
        object->scale.x = data->scale.x * data->initial_scale;
        object->scale.y = data->scale.y * data->initial_scale;
        object->scale.z = data->scale.z * data->initial_scale;
        unhide_obj(object);
        update_mkobj(object);
        color.alpha = 255;
        obj_set_color_for_all_materials(object, &color);
    }
}

/* TODO: [breakthrough needed] 97.08145%; restart and fade flow recovered; latch and register scheduling remain. */
static float p_magic_rings_fx(void) {
    RwRGBA color = {255, 255, 255, 255};
    ChessMagicRingsPdata* data = (ChessMagicRingsPdata*)apdata;
    unsigned int i;
    if (data->mode > 0) data->mode--;
    for (i = 0; i < 2; i++) {
        if (data->active[i] != 0) {
            MkObj* object = mk_chess_live_ring(&data->rings[i]);
            float scale;
            if (object == 0) data->active[i] = 0;
            scale = data->elapsed[i] * data->scale_per_tick;
            if (scale > data->maximum_scale || data->phase[i] * data->alpha_step > 255) {
                if (data->mode == 0) {
                    data->active[i] = 0;
                } else if (object != 0) {
                    mk_chess_restart_ring(data, i);
                }
            } else {
                if (scale >= data->fade_start_scale) {
                    if (data->fade_ticks == 0) {
                        mk_chess_restart_ring(data, 1);
                        data->fade_ticks = 1;
                    }
                    if (object != 0) {
                        color.alpha = 255 - data->phase[i] * data->alpha_step;
                        obj_set_color_for_all_materials(object, &color);
                        data->phase[i]++;
                    }
                }
                if (object != 0) {
                    object->scale.x = scale;
                    object->scale.z = scale;
                }
                data->elapsed[i] += game_speed;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (data->active[i] != 0) return 1.0f;
    }
    return -1.0f;
}

static inline void mk_chess_choose_trap_region(unsigned int side, unsigned int* x,
    unsigned int* y, unsigned int wide_threshold, unsigned int middle_threshold) {
    unsigned int draw = (unsigned short)randu0(100);
    if (draw < wide_threshold) {
        *x = (unsigned short)randu0(10);
        *y = (unsigned short)randu0(5);
        if (side == 1) *y += 5;
    } else if (draw < middle_threshold) {
        *y = 4 - (unsigned short)randu0(2);
        if (side == 1) *y += 2;
        *x = (unsigned short)randu0(7) + 1;
    } else {
        *x = (unsigned short)randu0(3) + 3;
        *y = (unsigned short)randu0(2);
        if (side == 1) *y += 8;
    }
}

static void mk_chess_drone_select_cell_for_trap(unsigned int side,
    unsigned int* x, unsigned int* y) {
    unsigned int strategy = mk_chess_pdata->sides[side]->strategy;
    do {
        switch (strategy) {
        default:
        case 0:
            mk_chess_choose_trap_region(side, x, y, 20, 60);
            break;
        case 1:
        case 2:
        case 4:
            mk_chess_choose_trap_region(side, x, y, 5, 30);
            break;
        case 3:
            mk_chess_choose_trap_region(side, x, y, 0, 10);
            break;
        case 5:
            *x = 1;
            if ((unsigned short)randu0(100) < 50) *x = 9 - *x;
            *y = (unsigned short)randu0(2);
            if (side == 1) *y = (unsigned short)randu0(2) + 8;
            break;
        }
    } while (mk_chess_pdata->board[*x].cells[*y].square_type != 0);
}

struct BgndPebbleControl;
struct BgndPebbleControl* bgnd_create_pebbles_with_sobj(
    MkSobj* object, unsigned int player, int mode, unsigned int count);

static inline MkSobj* mk_chess_prepare_team_pebbles(MkObj* object,
    unsigned int group, unsigned int count) {
    MkSobj* sobj;
    obj_create_sobjs(object);
    sobj = obj_first_sobj(object);
    bgnd_create_pebbles_with_sobj(sobj, group, 1, count);
    unhide_obj(object);
    hide_sobj(sobj);
    object->light_flags = 1;
    insert_fgnd_mkobj(object);
    if (object != 0 && g_game_info.bgnd_obj != 0) {
        mk_insert(&object->hdr, &g_game_info.bgnd_obj->child_list);
    }
    sobj->flags09_bits.bit7 = 1;
    sobj->flags_08_bits.bit0 = 0;
    sobj->flags_08_bits.bit6 = 1;
    sobj->z_offset = 0.0f;
    sobj_set_priority(sobj, 9);
    return sobj;
}

/* TODO: [breakthrough needed] 85.836365%; team FX setup recovered; model addressing and register scheduling remain. */
static void mk_chess_load_team_fx(void) {
    unsigned int side;
    for (side = 0; side < 2; side++) {
        MkObj* object;
        MkSobj* highlight;
        if (side == 0) object = load_named_model_from_slot(0xD003C, "MKC2_GRADIENT_BLUE", 0xC01E, 0);
        else object = load_named_model_from_slot(0xD003C, "MKC2_GRADIENT_RED", 0xC01E, 0);
        mk_chess_prepare_team_pebbles(object, side + 10, 8);
        if (side == 0) object = load_named_model_from_slot(0xD003C, "MKC2_GRADIENT_HIGHLIGHT_RED", 0xC01E, 0);
        else object = load_named_model_from_slot(0xD003C, "MKC2_GRADIENT_HIGHLIGHT_BLUE", 0xC01E, 0);
        highlight = mk_chess_prepare_team_pebbles(object, side + 12, 8);
        sobj_set_priority(highlight, 19);
        if (side == 0) object = load_named_model_from_slot(0xD003C, "MKC2_MOVEMENT_DOT_BLUE", 0xC01E, 0);
        else object = load_named_model_from_slot(0xD003C, "MKC2_MOVEMENT_DOT_RED", 0xC01E, 0);
        mk_chess_prepare_team_pebbles(object, side + 14, 36);
        if (side == 0) object = load_named_model_from_slot(0xD003C, "MKC2_OUTLINE3_BLUE", 0xC01E, 0);
        else object = load_named_model_from_slot(0xD003C, "MKC2_OUTLINE3_RED", 0xC01E, 0);
        mk_chess_prepare_team_pebbles(object, side + 16, 16);
    }
}

static float p_mk_chess_spell_hud(void);

static inline void mk_chess_suspend_side_selection(ChessSideHudState* hud) {
    if (hud->selected_piece != 0) {
        hud->saved_piece = hud->selected_piece;
        hud->selected_piece = 0;
        hud->flags |= 1;
        hud->flags |= 2;
    } else if ((hud->flags & 2) != 0) {
        hud->flags |= 1;
    }
}

/* TODO: [breakthrough needed] 76.68635%; HUD opening recovered; allocation reloads and image scheduling remain. */
static void mk_chess_show_spell_hud(unsigned int side) {
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessCursor* cursor = &mk_chess_pdata->cursors[manager->active_side];
    MkHdr* allocation;
    ChessHudState* hud;
    ScreenObj* image;
    mk_chess_pdata->manager.active_piece_by_side[side] =
        mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece;
    mk_chess_suspend_side_selection(mk_chess_pdata->sides[0]->hud);
    mk_chess_suspend_side_selection(mk_chess_pdata->sides[1]->hud);
    if (_create_mkproc_generic_bigstack(0xC022, 0x1F, p_mk_chess_spell_hud,
            sizeof(ChessHudState), &allocation) != 0) {
        mk_chess_set_game_mode(9);
        hud = (ChessHudState*)allocation;
        mk_chess_pdata->manager.spell = (ChessSpellState*)hud;
        hud->images = 0;
        snd_req(0x394);
        hud->side = side;
        hud->state = 0;
        hud->countdown = (int)(12.0f * game_speed);
        hud->field_14 = (int)(2.0f * game_speed);
        hud->input_state = 0;
        hud->names_state = 0;
        hud->selected_name = 0;
        hud->cursor_slot = 0;
        hud->field_54 = 0;
        image = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_28);
        if (hud->side == 0) image->x = -255;
        else image->x = screen_width;
        unhide_screen_obj(image);
        image->y = screen_height / 2;
        image = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_30);
        unhide_screen_obj(image);
        image->y = screen_height / 2 - 32;
        if (hud->side == 0) {
            image->x = -255;
        } else {
            image->x = screen_width;
            hud->countdown = (int)(-12.0f * game_speed);
            hud->field_14 = (int)(-2.0f * game_speed);
        }
        image = mk_chess_latched_screen(&mk_chess_pdata->manager.bar_38);
        hide_screen_obj(image);
        image->y = 370;
        if (hud->side == 0) image->x = 0;
        else image->x = screen_width - 255;
        image = mk_chess_latched_screen(&mk_chess_pdata->manager.spell_hud[11]);
        hide_screen_obj(image);
        image->pfx2d->verts[0].a = 255;
        image->y = 341;
        if (hud->side == 0) image->x = 2;
        else image->x = screen_width - 222;
        turn_controllers_off();
    }
}

static inline unsigned int mk_chess_bind_next_smoke_emitter(
    unsigned int emitter, MkPfx* particle, int bone, int index)
{
    emitter = fx_next_emitter(emitter);
    pfx_bind_emitter_num_to_obj_bone(particle, g_active_piece->object, bone, index);
    fx_restart_emit(emitter);
    return emitter;
}

/* TODO: [near miss] 99.20577%; only pooled-string addressing differs. */
static void mk_chess_do_smoke_effect(void)
{
    unsigned int emitter;
    MkPfx* particle;
    MkObj* anchor;

    get_plyr_obj_plyr_num(0);
    emitter = fx_next_emitter(fx_by_owner("hero_transform_smoke", 4));
    particle = pfx_from_emitter(emitter);
    anchor = pfx_bind_emitter_num_to_new_obj(particle, 0x6015, 0);
    get_bone_world_pos(g_active_piece->object, 10, &anchor->pos.value);
    update_mkobj(anchor != 0 ? as_mkhdr(&anchor->hdr) : 0);
    fx_restart_emit(emitter);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 0, 1);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 12, 2);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 13, 3);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 14, 4);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 15, 5);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 4, 6);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 5, 7);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 7, 8);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 8, 9);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 6, 10);

    emitter = fx_next_emitter(fx_by_owner("hero_transform_explode", 4));
    particle = pfx_from_emitter(emitter);
    anchor = pfx_bind_emitter_num_to_new_obj(particle, 0x6015, 0);
    get_bone_world_pos(g_active_piece->object, 10, &anchor->pos.value);
    fx_restart_emit(emitter);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 0, 1);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 12, 2);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 13, 3);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 14, 4);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 15, 5);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 4, 6);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 5, 7);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 7, 8);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 8, 9);
    emitter = mk_chess_bind_next_smoke_emitter(emitter, particle, 6, 10);

}

typedef struct ChessStatsStringLocation {
    unsigned int string_id;
    int font;
    unsigned int color;
    int x;
    int y;
} ChessStatsStringLocation;

extern const ChessStatsStringLocation g_mkc_stats_str_locations[10];

extern MkFileInfo sec_mkchess_game_over;
extern float p_main_menu(void);
static void mk_chess_display_stats(MkPtr** strings, const int* values, int winning_side);

static inline void mk_chess_scale_game_over_message(ScreenObj* image, float step,
    unsigned int delay)
{
    MkHdr* allocation;
    ChessScaleMessagePdata* scale;
    unsigned int width = image->pfx2d->tex_w;
    int center_y = screen_height / 2 + 15;
    int center_x = screen_width / 2;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &allocation);
    scale = (ChessScaleMessagePdata*)allocation;
    scale->object = image;
    hide_screen_obj(image);
    scale->step = step;
    scale->start_scale = 0.1f;
    scale->target_scale = 4.5f;
    scale->initial_delay = delay;
    scale->center_x = center_x;
    scale->center_y = center_y;
    scale->texture_width = width;
    scale->destroy_after_delay = 0;
    scale->final_delay = 0;
    scale->fade_after_midpoint = 1;
    scale->object->scale_x = 0.1f;
    scale->object->scale_y = 0.1f;
    scale->object->x = (int)-((float)(scale->texture_width >> 1) * scale->object->scale_x - (float)center_x);
    scale->object->y = (int)-((float)(scale->object->pfx2d->tex_h / 2) * scale->object->scale_y - (float)center_y);
    scale->object->flag_bits.scaled = 1;
}

/* TODO: [breakthrough needed] 72.67624%; game-over flow recovered; message setup and stack scheduling remain. */
float p_mk_chess_game_over(void)
{
    MkPtr* strings = 0;
    int values[10];
    int remaining = 3600;
    ScreenObj* image;
    if (g_game_info.field_04 & 0x20) push_game_state(3);
    else push_game_state(24);
    board_game_save_data.flags.board_input_seen = 0;
    turn_controllers_off();
    set_section_memory_scheme(8);
    set_mode_of_play(12);
    RwResourcesSetArenaSize(0x48000);
    load_ssf((MkFileEntry*)mkchess_ingame_art_file_table);
    load_art_section_language(0x160069, &sec_mkchess_game_over);
    load_string_bank(0x20000, "boardgame_strings_eng.mko");
    setup_sound_banks(7);
    wait_for_sound_banks_to_load();
    load_font(6);
    load_font(5);
    load_font(2);
    load_font(8);
    values[0] = board_game_save_data.sides[0].field_704;
    values[1] = board_game_save_data.sides[0].fight_stat_714;
    values[2] = board_game_save_data.sides[0].forced_fight_count;
    values[3] = board_game_save_data.sides[0].fight_stat_70C;
    values[4] = board_game_save_data.sides[0].fight_stat_710;
    values[5] = board_game_save_data.sides[1].field_704;
    values[6] = board_game_save_data.sides[1].fight_stat_714;
    values[7] = board_game_save_data.sides[1].forced_fight_count;
    values[8] = board_game_save_data.sides[1].fight_stat_70C;
    values[9] = board_game_save_data.sides[1].fight_stat_710;
    mk_chess_display_stats(&strings, values, board_game_save_data.winning_side);
    if (board_game_save_data.sides[board_game_save_data.winning_side].field_704 == 16) {
        image = load_named_2d_pfxobj(0x160069, 0x2058, "CHESS_FLAWLESS", 0, 46);
        mk_chess_scale_game_over_message(image, 0.03666667f, 100);
        snd_req_delay(0x2D, 120);
    }
    if (board_game_save_data.sides[board_game_save_data.winning_side == 0].field_704 == 0) {
        image = load_named_2d_pfxobj(0x160069, 0x2058, "CHESS_ANNIHILATION", 0, 46);
        mk_chess_scale_game_over_message(image, 0.022f, 250);
        snd_req_delay(0x2E, 280);
    }
    turn_camera_on();
    fade_from_black(8, 1);
    snd_req(0x3B4);
    if (g_game_info.field_04 & 0x80) {
        _mkproc_sleep_ticks = 240.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        remaining = 3360;
    }
    turn_controllers_on();
    board_game_save_data.flags.board_input_seen = 0;
    while (!board_game_save_data.flags.board_input_seen && --remaining > 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    snd_req(0x1AA5);
    fade_to_black(8, 1);
    destroy_list(&strings);
    destroy_mkprocs_pid(0xC023);
    delete_screen_obj_oid(0x2058);
    gamelogic_jump(6, p_main_menu);
    return -1.0f;
}

/* TODO: [breakthrough needed] 94.60833%; text/list flow recovered; stack and string-address lowering remain. */
static void mk_chess_display_stats(MkPtr** strings, const int* values, int winning_side)
{
    char heading[80];
    char number[12];
    int margin;
    unsigned int i;
    StringObj* string;
    const char* second_heading;

    margin = is_widescreen_mode() ? (screen_width - 640) / 2 : 0;
    if (load_named_2d_pfxobj_xy(0x160069, 0x2058, "CHESS_STAT_A", 0,
            margin, -16, 0x51) != 0) {
        load_named_2d_pfxobj_xy(0x160069, 0x2058, "CHESS_STAT_B", 0,
            margin + 512, -16, 0x51);
    }
    if (winning_side == 0) {
        winner = 1;
        string = string_center_xy(0xC01C, g_mkc_stats_str_locations[0].font,
            get_string_by_id(g_mkc_stats_str_locations[0].string_id | 0x20000),
            margin + 160, g_mkc_stats_str_locations[0].y, 0x4E);
        string->pfx.instance0.native_color.r = 100;
        string->pfx.instance0.native_color.g = 100;
        string->pfx.instance0.native_color.b = 200;
    } else {
        winner = 2;
        string = string_center_xy(0xC01C, g_mkc_stats_str_locations[0].font,
            get_string_by_id(g_mkc_stats_str_locations[0].string_id | 0x20000),
            margin + 480, g_mkc_stats_str_locations[0].y, 0x4E);
        string->pfx.instance0.native_color.r = 200;
        string->pfx.instance0.native_color.g = 100;
        string->pfx.instance0.native_color.b = 100;
    }
    mk_insert((MkHdr*)string, strings);
    if (winning_side != 0) {
        string = string_center_xy(0xC01C, g_mkc_stats_str_locations[1].font,
            get_string_by_id(g_mkc_stats_str_locations[1].string_id | 0x20000),
            margin + 160, g_mkc_stats_str_locations[1].y, 0x4E);
        string->pfx.instance0.native_color.r = 100;
        string->pfx.instance0.native_color.g = 100;
        string->pfx.instance0.native_color.b = 200;
    } else {
        string = string_center_xy(0xC01C, g_mkc_stats_str_locations[1].font,
            get_string_by_id(g_mkc_stats_str_locations[1].string_id | 0x20000),
            margin + 480, g_mkc_stats_str_locations[1].y, 0x4E);
        string->pfx.instance0.native_color.r = 200;
        string->pfx.instance0.native_color.g = 100;
        string->pfx.instance0.native_color.b = 100;
    }
    mk_insert((MkHdr*)string, strings);
    second_heading = get_string_by_id(0x2003F);
    sprintf(heading, "%s%s%s", get_string_by_id(0x2003E), "+", second_heading);
    string = string_center_xy(0xC01C, 6, heading, screen_width / 2, 52, 0x4E);
    mk_insert((MkHdr*)string, strings);
    for (i = 2; i < 10; ++i) {
        const ChessStatsStringLocation* location = &g_mkc_stats_str_locations[i];
        if (location->x != 0) {
            string = string_center_xy(0xC01C, location->font,
                get_string_by_id(location->string_id | 0x20000),
                screen_width / 2, location->y, 0x4E);
            mk_insert((MkHdr*)string, strings);
        }
    }
    for (i = 0; i < 10; ++i) {
        sprintf(number, "%d", values[i]);
        if (i < 5) {
            string = string_center_xy(0xC01C, 8, number,
                margin + 160, 280 - i * 42, 0x4E);
            string->pfx.instance0.native_color.b = 200;
            string->pfx.instance0.native_color.g = 100;
            string->pfx.instance0.native_color.r = 100;
        } else {
            string = string_center_xy(0xC01C, 8, number,
                margin + 480, 280 - (i - 5) * 42, 0x4E);
            string->pfx.instance0.native_color.b = 100;
            string->pfx.instance0.native_color.g = 100;
            string->pfx.instance0.native_color.r = 200;
        }
        mk_insert((MkHdr*)string, strings);
    }
}

typedef struct ChessBoardWithMonitor {
    ChessBoardRow rows[10];
    MkProc* monitor;
} ChessBoardWithMonitor;

static float p_monitor_chess_input(void);
static float p_team_monitor(void);

/* TODO: [breakthrough needed] 88.505615%; setup/restore recovered; allocation reloads and helper lowering remain. */
static void mk_chess_game_manager_init(int restoring)
{
    unsigned int side, index;
    ChessBoardWithMonitor* board;

    if (restoring == 0) {
        mk_chess_pdata->manager.active_side = 0;
        mk_chess_pdata->manager.input_state = 0;
        mk_chess_pdata->manager.event_data.other_piece = 0;
        mk_chess_pdata->manager.clock = 1;
        mk_chess_pdata->saved_field_110 = 0;
        mk_chess_pdata->spell_completion_clock = 0;
        mk_chess_pdata->saved_field_118 = 0;
        for (side = 0; side < 2; ++side) mk_chess_pdata->manager.active_piece_by_side[side] = 0;
    }
    mk_chess_pdata->manager.input_state = 13;
    for (side = 0; side < 2; ++side) {
        mk_chess_pdata->sides[side]->input_proc = _create_mkproc_generic_bigstack(
            0xC01D, 31, p_monitor_chess_input, 0x20, &mk_chess_pdata->sides[side]->input_data);
        if (mk_chess_pdata->sides[side]->input_proc != 0) {
            ((ChessInputPdata*)mk_chess_pdata->sides[side]->input_data)->side = side;
            ((ChessInputPdata*)mk_chess_pdata->sides[side]->input_data)->state = 0;
        }
        mk_chess_pdata->sides[side]->team_proc = _create_mkproc_generic_bigstack(
            0xC01D, 31, p_team_monitor, 0x20, &mk_chess_pdata->sides[side]->hud_header);
        if (mk_chess_pdata->sides[side]->team_proc != 0) {
            mk_chess_pdata->sides[side]->hud->saved_piece = 0;
            mk_chess_pdata->sides[side]->hud->selected_piece = 0;
            mk_chess_pdata->sides[side]->hud->flags_word = 0;
            mk_chess_pdata->sides[side]->hud->side = side;
        }
        mk_chess_pdata->sides[side]->drone_proc = _create_mkproc_generic_bigstack(
            0xC01D, 31, p_drone_monitor, 0x10, &mk_chess_pdata->sides[side]->controller_header);
        if (mk_chess_pdata->sides[side]->drone_proc != 0) {
            mk_chess_pdata->sides[side]->controller->flags_word = 0;
            mk_chess_pdata->sides[side]->controller->side = side;
        }
        mk_chess_pdata->sides[side]->controller->flags.drone_controlled =
            g_chess_definition_info[side].field_04;
    }
    board = (ChessBoardWithMonitor*)mk_chess_pdata->board;
    board->monitor = _create_mkproc_generic_bigstack(0xC01D, 31, p_board_monitor, 0, 0);
    mk_chess_reset_cursors();
    if (restoring == 1) {
        mk_chess_pdata->manager.active_side = board_game_save_data.active_side;
        for (side = 0; side < 2; ++side) {
            mk_chess_pdata->manager.active_piece_by_side[side] = mk_chess_pdata->board[
                board_game_save_data.active_x[side]].cells[board_game_save_data.active_y[side]].piece;
        }
        mk_chess_pdata->manager.clock = board_game_save_data.saved_clock;
        mk_chess_pdata->saved_field_110 = board_game_save_data.saved_field_110;
        mk_chess_pdata->spell_completion_clock = board_game_save_data.saved_spell_clock;
        mk_chess_pdata->saved_field_118 = board_game_save_data.saved_field_118;
    }
    destroy_mkprocs_pid(0xC027);
    mk_chess_pdata->turn_timeout = 1800.0f * inverse_game_speed;
    mk_chess_pdata->input_transition_busy = 0;
    for (side = 0; side < 2; ++side) {
        for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; ++index) {
            mk_chess_calc_all_moves_cb(mk_chess_pdata->sides[side]->pieces[index], 0, 0, 0);
        }
    }
    mk_chess_load_team_fx();
    set_player_state(&g_game_info.plyr0, g_chess_definition_info[0].field_04 == 0 ? 2 : 0);
    set_player_state(&g_game_info.plyr1, g_chess_definition_info[1].field_04 == 0 ? 2 : 0);
}

/* TODO: [breakthrough needed] 80.296425%; HUD ownership recovered; string-address and latch/placement lowering remain. */
static void mk_chess_show_select_trap_hud(void)
{
    ChessDirectionState* hud = (ChessDirectionState*)get_mkpdata_generic(sizeof(ChessDirectionState));
    ScreenObj* image;
    StringObj* string;
    MkHdr* allocation;
    MkProc* process;
    unsigned int side;

    mk_chess_pdata->manager.directional_state = hud;
    hud->strings = 0;
    image = load_named_2d_pfxobj(0xD003C, 0xC01C, "BOARD_TITLE_LOGO", 0, 0x57);
    image->x = screen_width / 2 - image->pfx2d->tex_w / 2;
    image->y = screen_height - 130;
    hud->title = image;
    string = string_center_xy(0xC01C, 6, get_string_by_id(0x20034), screen_width / 2, 80, 0x58);
    string->pfx.instance0.native_color.r = 255;
    string->pfx.instance0.native_color.g = 255;
    string->pfx.instance0.native_color.b = 48;
    process = _create_mkproc_generic_tinystack(0xC026, 31, p_mk_chess_blink_string,
        sizeof(ChessBlinkStringPdata), &allocation);
    if (process != 0) {
        ChessBlinkStringPdata* blink = (ChessBlinkStringPdata*)allocation;
        blink->text = 0;
        blink->text_instance = 0;
        blink->text = string;
        blink->text_instance = string->instance;
        mk_insert((MkHdr*)string, &process->pdata_list_b);
        blink->timer = 25;
        blink->interval = 25;
    }
    string = string_left_xy(0xC01C, 5, get_string_by_id(0x20031), screen_width / 2 + (-150), 60, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_left_xy(0xC01C, 6, "=", screen_width / 2 + (-180), 60, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_left_xy(0xC01C, 5, get_string_by_id(0x20033), screen_width / 2 + (95), 50, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_left_xy(0xC01C, 6, "+", screen_width / 2 + (65), 47, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_left_xy(0xC01C, 5, get_string_by_id(0x20032), screen_width / 2 + (-150), 35, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    string = string_left_xy(0xC01C, 6, "{", screen_width / 2 + (-180), 35, 0x58);
    mk_insert((MkHdr*)string, &hud->strings);
    for (side = 0; side < 2; ++side) {
        MkObj* object;
        MkSobj* subobject;
        ChessCursor* cursor;
        ChessBoardRow* board;
        if (side == 0) object = load_model_from_slot(0xD003C, 0x0A430012, 0xA00B);
        else object = load_model_from_slot(0xD003C, 0x0A430011, 0xA00B);
        cursor = &hud->cursors[side];
        cursor->object = &object->hdr;
        cursor->object_instance = object->hdr.instance;
        obj_create_sobjs(object);
        object->light_flags = 16;
        subobject = obj_first_sobj(object);
        subobject->flags09_bits.bit7 = 1;
        sobj_set_priority(subobject, 19);
        object->light_flags = 13;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        unhide_obj(object);
        board = mk_chess_pdata->board;
        object = (MkObj*)cursor->object;
        if (object == 0 || object->hdr.instance != cursor->object_instance) object = 0;
        cursor->cell_x = 5;
        if (side == 0) {
            cursor->cell_y = 4;
            object->pos.value.x = board[5].cells[4].position.x;
            object->pos.value.y = board[5].cells[4].position.y;
            object->pos.value.z = board[5].cells[4].position.z;
        } else {
            cursor->cell_y = 5;
            object->pos.value.x = board[5].cells[5].position.x;
            object->pos.value.y = board[5].cells[5].position.y;
            object->pos.value.z = board[5].cells[5].position.z;
        }
        update_obj_pos(object);
    }
}

static inline void mk_chess_start_cell_emitter(unsigned int emitter, float x, float y, float z)
{
    MkPfx* particle;
    MkObj* object;
    if (emitter == 0) return;
    fx_reset_emit(emitter);
    particle = pfx_from_emitter(emitter);
    if (particle == 0) return;
    object = pfx_bind_emitter_num_to_new_obj(particle, 0x8227, emitter_id_from_handle(emitter));
    if (object == 0) return;
    object->flags_08_bits.airborne = 1;
    object->pos.value.x = x;
    object->pos.value.y = y;
    object->pos.value.z = z;
    update_mkobj(object);
    fx_resume_emit(emitter);
}

/* TODO: [breakthrough needed] 85.16726%; restore flag behavior recovered; branch/register and string lowering remain. */
void mk_chess_place_special_cell_at(unsigned int x, unsigned int y, int type,
    int restored, float px, float py, float pz, float scale)
{
    ChessCell* cell;
    if (x >= 10 || y >= 10 || (unsigned int)type >= 3) return;
    cell = &mk_chess_pdata->board[x].cells[y];
    cell->square_type = type;
    cell->saved_parameters[0] = px;
    cell->saved_parameters[1] = py;
    cell->saved_parameters[2] = pz;
    cell->saved_parameters[3] = scale;
    cell->flags = 0;
    if (type == 1) {
        MkObj* object;
        MkSobj* subobject;
        cell->object = load_named_model_from_slot(0xD003C, "HEALTH_BASE02", 0xC01D, 0);
        object = cell->object;
        obj_create_sobjs(object);
        subobject = obj_first_sobj(object);
        object->light_flags = 13;
        sobj_set_priority(subobject, 9);
        subobject->flags09_bits.bit6 = 1;
        subobject->flags09_bits.bit7 = 1;
        object->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(object);
        cell->object->flags_08_bits.scale_active = 1;
        cell->object->scale.z = 1.0f;
        cell->object->scale.y = 1.0f;
        cell->object->scale.x = 1.0f;
        cell->object->light_flags = 4;
        cell->object->pos.value.x = cell->position.x;
        cell->object->pos.value.y = cell->position.y;
        cell->object->pos.value.z = cell->position.z;
        unhide_obj(cell->object);
        if (g_game_info.bgnd_obj != 0) mk_insert(&cell->object->hdr, &g_game_info.bgnd_obj->child_list);
    }
    cell = &mk_chess_pdata->board[x].cells[y];
    if (cell->square_type != 1) return;
    if (restored != 0) {
        cell->emitter = fx_by_owner("health_effect", 4);
        cell->emitter = fx_next_emitter(cell->emitter);
        mk_chess_start_cell_emitter(cell->emitter, cell->position.x, cell->position.y, cell->position.z);
        cell->flags |= 0x80;
        if (cell->piece != 0) {
            cell->flags |= 0x40;
            cell->second_emitter = fx_by_owner("big_health_effect", 4);
            cell->second_emitter = fx_next_emitter(cell->second_emitter);
            mk_chess_start_cell_emitter(cell->second_emitter, cell->position.x, cell->position.y, cell->position.z);
            cell->saved_parameters[1] = (float)cell->piece->side;
            cell->saved_parameters[2] = (float)cell->piece->id;
        }
    } else {
        cell->emitter = fx_by_owner("health_effect", 4);
        cell->emitter = fx_next_emitter(cell->emitter);
        if (cell->emitter != 0) {
            mk_chess_start_cell_emitter(cell->emitter, cell->position.x, cell->position.y, cell->position.z);
            cell->flags |= 0x80;
        }
    }
}

/* TODO: [breakthrough needed] 90.20649%; launch/landing control flow recovered; FP scheduling and stack layout remain. */
void mk_chess_launch_n_land_ani_with_xz(int animation_id, int turn, unsigned int sound,
    float launch_frame, float initial_speed, float landing_frame, float vertical_speed,
    float gravity, float blend, float start_x, float start_y, float target_x, float target_y)
{
    ChessAnimPdata* animation = g_active_piece->animation;
    ChessBoardRow* board = mk_chess_pdata->board;
    ChessCell* target = &board[(unsigned int)target_x].cells[(unsigned int)target_y];
    ChessCell* start = &board[(unsigned int)start_x].cells[(unsigned int)start_y];
    ChessAnimPdata* current;
    float discriminant, root, flight_time, other_time;
    float rotation = 0.0f;

    animation->flags |= 0x40;
    current = g_active_piece->animation;
    set_root_and_obj_movement_weights(0.0f, 1.0f, (AnimPdata*)current);
    g_active_piece->object->hide_flag_bits.pin_animation = 0;
    current->speed = initial_speed;
    transition_to_anim_script(blend, (AnimPdata*)current, (AniData*)mkc_animations[animation_id], 0x43);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if (launch_frame != 0.0f) {
        ChessAnimPdata* waiting;
        animation->speed = initial_speed;
        waiting = g_active_piece->animation;
        while (waiting->frame <= launch_frame) {
            current = g_active_piece->animation;
            advance_anim((AnimPdata*)current);
            pose_anim((AnimPdata*)current, 1);
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
            if (waiting->speed * game_speed + waiting->frame > launch_frame) break;
        }
        animation->speed = 1.0f;
    }
    if (sound != 0) mk_chess_snd_request(sound);
    g_active_piece->object->pos_vel.y = vertical_speed;
    g_active_piece->object->gravity = gravity;
    g_active_piece->object->flags_08_bits.moving = 1;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 1;
    g_active_piece->object->flags_09_bits.bit6 = 0;
    discriminant = vertical_speed * vertical_speed -
        2.0f * gravity * (g_active_piece->object->pos.value.y - 0.19f);
    if (!(discriminant >= 0.001f)) discriminant = 0.001f;
    root = mk_chess_table_square_root(discriminant);
    flight_time = (root - vertical_speed) / gravity;
    other_time = (-root - vertical_speed) / gravity;
    if (flight_time < 0.0f || (other_time > 0.0f && other_time < flight_time)) flight_time = other_time;
    if (!(flight_time >= 1.0f)) flight_time = 1.0f;
    if (start_x != target_x || start_y != target_y) {
        g_active_piece->object->pos_vel.x = (target->position.x - start->position.x) / flight_time;
        g_active_piece->object->pos_vel.z = (target->position.z - start->position.z) / flight_time;
    }
    animation->speed = (landing_frame - launch_frame) / flight_time;
    if (turn != 0) {
        MkObj* object = g_active_piece->object;
        float desired = gxMathArcTanYX(target->position.x - object->pos.value.x,
            target->position.z - object->pos.value.z);
        rotation = ang_sub_ang(desired, g_active_piece->object->ang.y);
    }
    rotation /= flight_time;
    while (animation->frame <= landing_frame) {
        current = g_active_piece->animation;
        advance_anim((AnimPdata*)current);
        pose_anim((AnimPdata*)current, 1);
        g_active_piece->object->ang.y += rotation;
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        if (animation->speed * game_speed + animation->frame > landing_frame) break;
    }
    g_active_piece->object->flags_08_bits.moving = 0;
    g_active_piece->object->flags_09_bits.launched = 1;
    g_active_piece->object->flags_08_bits.gravity_enabled = 0;
    g_active_piece->object->flags_09_bits.bit6 = 1;
}

extern MkFileInfo sec_mk_chess;
extern MkFileInfo sec_chess_fast_anims;
extern int mkc_anims[89];
extern MkPtr* board_piece_light_list;
int load_effect_bank(char* name);

static inline void mk_chess_load_common_cursor(int cursor_index, unsigned int art_id, int selection)
{
    MkObj* object = load_model_from_slot(0xD003C, art_id, 0xA00B);
    MkSobj* subobject;
    ChessCursor* cursor = cursor_index < 0 ? &mk_chess_pdata->cursor : &mk_chess_pdata->cursors[cursor_index];
    cursor->object = &object->hdr;
    cursor->object_instance = object->hdr.instance;
    obj_create_sobjs(object);
    if (selection) object->light_flags = 16;
    subobject = obj_first_sobj(object);
    if (selection) {
        subobject->flags09_bits.bit7 = 1;
        sobj_set_priority(subobject, 19);
        object->light_flags = 13;
    } else {
        object->light_flags = 13;
        sobj_set_priority(subobject, 9);
        subobject->flags09_bits.bit6 = 1;
        subobject->flags09_bits.bit7 = 1;
    }
    object->flags_08_bits.airborne = 1;
    insert_fgnd_mkobj(object);
}

/* TODO: [breakthrough needed] 92.55064%; cursor owner reload restored; table/register and string lowering remain. */
static void mk_chess_load_common_data(void)
{
    unsigned int row, index, side;
    LoadBgndCtx load_context;
    load_art_section_language(0xD003C, &sec_mk_chess);
    load_string_bank(0x20000, "boardgame_strings_eng.mko");
    g_board_game_controller.command_script = cmdscript_loadfile_by_name(0x10, "board_game_config.mko");
    active_cmdscript->mko = g_board_game_controller.command_script;
    g_board_game_controller.piece_libraries = (ChessLibraryEntry*)get_data_table(g_board_game_controller.command_script, 1);
    g_board_game_controller.piece_art_rows = (AniScript**)get_data_table(g_board_game_controller.command_script, 18);
    get_row_count_for_table_by_pointer(g_board_game_controller.command_script, g_board_game_controller.piece_art_rows);
    for (row = 0; row < 6; ++row) {
        ChessClassText* config = &((ChessClassText*)get_data_table(g_board_game_controller.command_script, 17))[row];
        g_board_game_controller.class_definitions[config->class_index].movement_skill_count = 0;
        g_board_game_controller.class_definitions[config->class_index].flags_word = 0;
        for (index = 0; index < 64; ++index) {
            g_board_game_controller.class_definitions[config->class_index].event_script_words[index] = 0xABABAB00;
        }
        g_board_game_controller.class_definitions[config->class_index].field_16C = config->field_08;
        g_board_game_controller.class_definitions[config->class_index].text = config;
        for (side = 0; side < 2; ++side) {
            ScreenObj* image;
            g_board_game_controller.class_definitions[config->class_index].portraits[side].screen = 0;
            g_board_game_controller.class_definitions[config->class_index].portraits[side].instance = 0;
            image = load_named_2d_pfxobj(0xD003C, 0xC01C, config->portrait_name, 0, 0x4E);
            g_board_game_controller.class_definitions[config->class_index].portraits[side].screen = image;
            g_board_game_controller.class_definitions[config->class_index].portraits[side].instance = image->instance;
            hide_screen_obj(image);
        }
        g_active_class_definition = &g_board_game_controller.class_definitions[config->class_index];
        cmdscript_setup_execution(g_board_game_controller.command_script, config->init_script);
        cmdscript_execute(g_board_game_controller.command_script);
    }
    load_lights((LightDef**)get_data_table(g_board_game_controller.command_script, 16), &board_piece_light_list);
    load_context.bgnd_obj = 0;
    load_context.art_id = 0xD003C;
    load_context.field_08 = 0;
    g_board_game_controller.command_script->load_ctx = &load_context;
    load_effect_bank("boardgame_fx.mko");
    g_board_game_controller.command_script->load_ctx = 0;
    load_font(8);
    load_font(6);
    load_font(5);
    load_font(0);
    load_font(3);
    unload_section_slot(0x12005E);
    add_anim_section_async(0x12005E, &sec_chess_fast_anims, mkc_anims, 0, 1);
    wait_for_slot_load(0x12005E);
    mkc_animations = (AniScript**)mkc_anims;
    mk_chess_load_common_cursor(-1, 0x0A430000, 0);
    mk_chess_load_common_cursor(2, 0x0A430000, 0);
    mk_chess_load_common_cursor(0, 0x0A430012, 1);
    mk_chess_load_common_cursor(1, 0x0A430011, 1);
    mk_chess_load_board_spell_hud();
}

static inline int mk_chess_input_mode_allowed(int camera)
{
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    int mode;
    if (ck_eat_online_switches()) return 0;
    mode = manager->input_state;
    return mode == 0 || mode == 1 || mode == 9 || mode == 10 || (camera && mode == 6);
}

static inline int mk_chess_input_side_allowed(ChessInputPdata* input)
{
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    return manager->input_state == 10 || input->side == manager->active_side;
}

static inline void mk_chess_transfer_direction(ChessInputPdata* input,
    unsigned int direction, int switch_index, float (*handler)(void))
{
    if (input->direction != direction) {
        input->direction_delay = 2;
        input->direction = direction;
    } else if (input->direction_delay == 0) {
        input->analog = 0;
        input->state = 9;
        input->direction = direction;
        input->switch_index = switch_index;
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(handler, 0.0f);
    } else {
        input->direction_delay--;
    }
}

/* TODO: [breakthrough needed] 82.89183%; input permissions/delay recovered; helper and branch lowering remain. */
static float p_monitor_chess_input(void)
{
    ChessInputPdata* input = (ChessInputPdata*)apdata;
    int port = input->side == 1 ? g_game_info.plyr1.pad_index : g_game_info.plyr0.pad_index;
    mk_chess_handle_repeatable_input(port, (ChessRepeatInput*)input);
    if (!board_game_save_data.input_flags.input_locked) {
        if (mk_chess_input_mode_allowed(1) && mk_chess_input_side_allowed(input)) {
            mk_chess_monitor_cam_zoom_scenerios(input);
        }
        if (mk_chess_input_mode_allowed(0) && mk_chess_input_side_allowed(input)) {
            unsigned int left = check_switch(port, 15);
            unsigned int right = check_switch(port, 13);
            unsigned int up = check_switch(port, 12);
            unsigned int down = check_switch(port, 14);
            if (left) {
                if (up) mk_chess_transfer_direction(input, 5, -1, x_chess_left_and_up);
                else if (down) mk_chess_transfer_direction(input, 6, -1, x_chess_left_and_down);
                else mk_chess_transfer_direction(input, 0, 15, x_chess_left);
            } else if (right) {
                if (up) mk_chess_transfer_direction(input, 7, -1, x_chess_right_and_up);
                else if (down) mk_chess_transfer_direction(input, 8, -1, x_chess_right_and_down);
                else mk_chess_transfer_direction(input, 0, 13, x_chess_right);
            } else if (up) {
                mk_chess_transfer_direction(input, 0, 12, x_chess_up);
            } else if (down) {
                mk_chess_transfer_direction(input, 0, 14, x_chess_down);
            }
        }
    }
    return 1.0f;
}

static inline ChessPiece* mk_chess_captured_piece_of_type(unsigned int side, int type)
{
    ChessSideState* team = mk_chess_pdata->sides[side];
    unsigned int index;
    for (index = 17 - team->captured_piece_count; index < 17; ++index) {
        ChessPiece* piece = team->pieces[index];
        if (piece->type == type) return piece;
    }
    return 0;
}

static inline void mk_chess_display_rescue_piece(ChessHudState* hud, ChessPiece* piece)
{
    ChessScreenRef* portrait;
    ScreenObj* image;
    MkPtr* link;
    hide_screen_obj((ScreenObj*)hud->image_58);
    portrait = &mk_chess_pdata->sides[piece->side]->portraits[piece->type];
    image = portrait->screen;
    if (image != 0 && image->instance != portrait->instance) image = 0;
    image->pfx2d->verts[0].a = 255;
    hud->image_58 = (MkHdr*)image;
    hud->rescue_piece_type = piece->type;
    image->pfx2d->verts[0].a = 255;
    image->y = 200;
    if (hud->side == 0) {
        image->x = 52;
        if (image->pfx2d->tex_w > 255) image->x -= 13;
    } else {
        image->x = screen_width - 170;
        if (image->pfx2d->tex_w > 255) image->x -= 113;
    }
    unhide_screen_obj(image);
    link = hud->strings;
    while (link != 0) {
        StringObj* string = (StringObj*)link->hdr;
        if (link->instance != string->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (strcmp(string->text, get_string_by_id(0x2001E)) != 0) {
                update_string_obj(string, 8, g_board_game_controller.class_definitions[piece->type].text->rescue_name);
                string->pfx.instance0.native_color.r = 0x66;
                string->pfx.instance0.native_color.g = 0x33;
                string->pfx.instance0.native_color.b = 0;
            }
            link = link->next;
        }
    }
}

/* TODO: [breakthrough needed] 84.008156%; rescue cycling recovered; search and portrait/list lowering remain. */
static void mk_chess_spell_hud_pick_a_piece_for_rescue(ChessHudState* hud)
{
    ChessPiece* piece;
    int type, selected;
    switch (hud->input_state) {
    case 1:
    case 4:
        if (hud->image_58 != 0) {
            snd_req(0x392);
            selected = hud->rescue_piece_type;
            piece = 0;
            for (type = selected + 1; type < 6; ++type) {
                piece = mk_chess_captured_piece_of_type(hud->side, type);
                if (piece != 0) break;
            }
            if (piece == 0) {
                for (type = 0; type <= selected; ++type) {
                    piece = mk_chess_captured_piece_of_type(hud->side, type);
                    if (piece != 0) break;
                }
            }
            mk_chess_display_rescue_piece(hud, piece);
        }
        break;
    case 2:
    case 3:
        if (hud->image_58 != 0) {
            snd_req(0x392);
            selected = hud->rescue_piece_type;
            piece = 0;
            for (type = selected - 1; type >= 0; --type) {
                piece = mk_chess_captured_piece_of_type(hud->side, type);
                if (piece != 0) break;
            }
            if (piece == 0) {
                for (type = 5; type >= selected; --type) {
                    piece = mk_chess_captured_piece_of_type(hud->side, type);
                    if (piece != 0) break;
                }
            }
            mk_chess_display_rescue_piece(hud, piece);
        }
        break;
    case 9:
        if (hud->image_58 != 0) {
            snd_req(0x393);
            turn_controllers_off();
            mk_chess_spell_hud_retract_all_during_deadpool_select(hud, 1);
        }
        break;
    case 10:
        turn_controllers_off();
        mk_chess_spell_hud_retract_all_during_deadpool_select(hud, 0);
        break;
    }
    hud->input_state = 0;
}

static inline void mk_chess_restore_scroll_alpha(ScreenObj* image)
{
    int vertex;
    for (vertex = 0; vertex < 4; vertex++) image->pfx2d->verts[vertex].a = 255;
}

/* TODO: [near miss] 96.57143%; scroll motion recovered; scheduling/coloring and static relocation remain. */
static void mk_chess_spell_targetting_display_hud(ChessHudState* hud, int hide)
{
    static StringObj* string_obj;
    ScreenObj* top_left_paper;
    ScreenObj* top_right_paper;
    ScreenObj* top_left_roll;
    ScreenObj* top_right_roll;
    ScreenObj* bottom_paper;
    ScreenObj* bottom_left_roll;
    ScreenObj* bottom_right_roll;
    const ChessSpellPageText* page = hud->caster->spells->pages[hud->spell_number][0];
    float closing_speed = 1.0f;
    int finished = 0;
    int vertical_step;

    mk_chess_restore_scroll_alpha(mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[18]));
    mk_chess_restore_scroll_alpha(mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[15]));
    mk_chess_restore_scroll_alpha(mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[14]));
    top_left_paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[14]);
    top_right_paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[15]);
    top_left_roll = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[12]);
    top_right_roll = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[13]);
    bottom_paper = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[18]);
    bottom_left_roll = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[16]);
    bottom_right_roll = mk_chess_live_screen(&mk_chess_pdata->manager.spell_hud[17]);
    if (hide == 0) {
        snd_req(0x394);
        top_left_paper->x = screen_width / 2;
        top_left_paper->y = -top_left_paper->pfx2d->tex_h;
        top_left_paper->flag_bits.scaled = 1;
        top_left_paper->scale_x = -0.1f;
        top_left_paper->scale_y = 1.0f;
        unhide_screen_obj(top_left_paper);
        top_right_paper->x = screen_width / 2;
        top_right_paper->y = -top_right_paper->pfx2d->tex_h;
        top_right_paper->flag_bits.scaled = 1;
        top_right_paper->scale_x = 0.1f;
        top_right_paper->scale_y = 1.0f;
        unhide_screen_obj(top_right_paper);
        top_left_roll->x = screen_width / 2 - 30;
        top_left_roll->y = -top_left_roll->pfx2d->tex_h;
        unhide_screen_obj(top_left_roll);
        top_right_roll->x = screen_width / 2 + 30 - top_right_roll->pfx2d->tex_w;
        top_right_roll->y = -top_right_roll->pfx2d->tex_h;
        unhide_screen_obj(top_right_roll);
        bottom_paper->x = screen_width / 2 - 20;
        bottom_paper->y = screen_height + 20;
        bottom_paper->flag_bits.scaled = 1;
        bottom_paper->scale_x = 0.2f;
        bottom_paper->scale_y = 1.0f;
        unhide_screen_obj(bottom_paper);
        bottom_left_roll->x = screen_width / 2 - 30;
        bottom_left_roll->y = screen_height + 20;
        unhide_screen_obj(bottom_left_roll);
        bottom_right_roll->x = screen_width / 2 + 30 - bottom_right_roll->pfx2d->tex_w;
        bottom_right_roll->y = screen_height + 20;
        unhide_screen_obj(bottom_right_roll);
        vertical_step = (int)(8.0f * game_speed);
        string_obj = string_center_xy(0xC01C, 8, get_string_by_id(page->title_id | 0x20000),
            screen_width / 2, screen_height - 67, 0x4E);
        string_obj->pfx.instance0.native_color.r = 0x66;
        string_obj->pfx.instance0.native_color.g = 0x33;
        string_obj->pfx.instance0.native_color.b = 0;
        hide_string_obj(string_obj);
    } else {
        float scale_step;
        int horizontal_step;
        snd_req(0x396);
        if (string_obj->instance != 0) string_obj->typed_vtbl->destroy(string_obj);
        string_obj = 0;
        if (game_speed > 1.0f) closing_speed = (game_speed - 1.0f) * 0.5f + game_speed;
        scale_step = 0.122f * closing_speed;
        horizontal_step = (int)(16.0f * closing_speed);
        while (top_left_roll->x < screen_width / 2 - 30) {
            top_left_roll->x += horizontal_step;
            top_right_roll->x -= horizontal_step;
            top_left_paper->scale_x += scale_step;
            top_right_paper->scale_x -= scale_step;
            if (bottom_left_roll->x < screen_width / 2 - 30) {
                bottom_left_roll->x += horizontal_step;
                bottom_right_roll->x -= horizontal_step;
                bottom_paper->x = bottom_left_roll->x + (int)(10.0f * closing_speed);
                bottom_paper->scale_x -= 0.25f * closing_speed;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        }
        vertical_step = -8;
    }
    if (hide == 1) snd_req_delay(0x397, 5);
    while (!finished) {
        bottom_paper->y -= vertical_step;
        bottom_left_roll->y -= vertical_step;
        bottom_right_roll->y -= vertical_step;
        top_left_paper->y += vertical_step;
        top_right_paper->y += vertical_step;
        top_left_roll->y += vertical_step;
        top_right_roll->y += vertical_step;
        if ((top_left_roll->y > 0 && vertical_step > 0) ||
            (top_left_roll->y < -64 && vertical_step < 0)) finished = 1;
        if (top_left_roll->y > 0) top_left_roll->y = 0;
        if (top_right_roll->y > 0) top_right_roll->y = 0;
        if (top_right_paper->y > 0) top_right_paper->y = 0;
        if (top_left_paper->y > 0) top_left_paper->y = 0;
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    if (hide == 0) {
        int horizontal_step;
        snd_req_delay(0x395, 5);
        horizontal_step = vertical_step * 2;
        while (top_left_roll->x > 40) {
            top_left_roll->x -= horizontal_step;
            top_right_roll->x += horizontal_step;
            top_left_paper->scale_x = -(0.122f * game_speed - top_left_paper->scale_x);
            top_right_paper->scale_x += 0.122f * game_speed;
            if (bottom_left_roll->x > 220) {
                bottom_left_roll->x -= horizontal_step;
                bottom_right_roll->x += horizontal_step;
                bottom_paper->x = bottom_left_roll->x + 10;
                bottom_paper->scale_x += 0.25f * game_speed;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((ChessProcVtable*)aproc->vtbl)->sleep();
        }
        unhide_string_obj(string_obj);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    } else {
        hide_screen_obj(top_left_paper);
        hide_screen_obj(top_right_paper);
        hide_screen_obj(top_left_roll);
        hide_screen_obj(top_right_roll);
        hide_screen_obj(bottom_paper);
        hide_screen_obj(bottom_left_roll);
        hide_screen_obj(bottom_right_roll);
    }
}

static inline void mk_chess_position_mode_cursor(ChessCursor* owner,
    unsigned char x, unsigned char y)
{
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    MkObj* object = (MkObj*)mk_chess_cursor_live_header(owner);
    owner->cell_x = x;
    owner->cell_y = y;
    object->pos.value.x = cell->position.x;
    object->pos.value.y = cell->position.y;
    object->pos.value.z = cell->position.z;
    update_obj_pos(object);
}

static inline void mk_chess_position_selection_cursors(ChessPiece* piece)
{
    unsigned char x = piece->cell_x;
    unsigned char y = piece->cell_y;
    mk_chess_position_mode_cursor(&mk_chess_pdata->cursors[0], x, y);
    mk_chess_position_mode_cursor(&mk_chess_pdata->cursors[1], x, y);
    mk_chess_position_mode_cursor(&mk_chess_pdata->cursor, x, y);
}

static inline ChessPiece* mk_chess_find_mode_side_piece(unsigned int side)
{
    unsigned char x, y;
    for (x = 0; x < 10; x++) {
        for (y = 0; y < 10; y++) {
            ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;
            if (piece != 0 && piece->side == side) return piece;
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 87.07561%; mode transitions recovered; latch lowering and cursor scheduling remain. */
void mk_chess_set_game_mode(int mode)
{
    switch (mode) {
    case 10:
        mk_chess_show_select_trap_hud();
        mk_chess_pdata->manager.input_state = mode;
        return;
    case 3:
    case 11:
    case 9:
        mk_chess_pdata->manager.input_state = mode;
        return;
    case 0: {
        int other_side = mk_chess_pdata->manager.active_side == 0;
        unsigned int group;
        unsigned int side;
        ChessPiece* piece;
        ChessCursor* cursor;
        ChessManagerInfo* manager;
        destroy_mkprocs_pid(0xC020);
        for (group = 10; group <= 15; group++) bgnd_hide_pebbles(group);
        side = mk_chess_pdata->manager.active_side;
        piece = mk_chess_pdata->manager.active_piece_by_side[side];
        if (piece != 0) {
            mk_chess_position_selection_cursors(piece);
        } else {
            piece = mk_chess_find_mode_side_piece(side);
            if (piece != 0) mk_chess_position_selection_cursors(piece);
        }
        unhide_obj(mk_chess_cursor_live_header(
            &mk_chess_pdata->cursors[mk_chess_pdata->manager.active_side]));
        hide_obj(mk_chess_cursor_live_header(&mk_chess_pdata->cursors[other_side]));
        hide_obj(mk_chess_cursor_live_header(&mk_chess_pdata->cursor));
        manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        cursor = &mk_chess_pdata->cursors[manager->active_side];
        mk_chess_hud_set_piece_portrait(mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece);
        mk_chess_pdata->sides[mk_chess_pdata->manager.active_side]->controller->flags.bit6 = 1;
        mk_chess_pdata->sides[mk_chess_pdata->manager.active_side]->field_104 = 800;
        mk_chess_pdata->manager.input_state = mode;
        return;
    }
    case 1: {
        ChessCursor* cursor;
        ChessPiece* piece;
        ChessPieceMoveMap* moves;
        unsigned int index;
        hide_obj(mk_chess_cursor_live_header(&mk_chess_pdata->cursors[0]));
        hide_obj(mk_chess_cursor_live_header(&mk_chess_pdata->cursors[1]));
        unhide_obj(mk_chess_cursor_live_header(&mk_chess_pdata->cursor));
        mk_chess_pdata->cursor_track = 0;
        cursor = &mk_chess_pdata->cursors[mk_chess_pdata->manager.active_side];
        mk_chess_pdata->manager.event_data.piece = mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece;
        mk_chess_pdata->manager.active_piece_by_side[mk_chess_pdata->manager.active_side] =
            mk_chess_pdata->manager.event_data.piece;
        mk_chess_pdata->manager.input_state = mode;
        piece = mk_chess_pdata->manager.event_data.piece;
        moves = piece->move_map;
        for (index = 0; index < moves->target_count; index++) {
            mk_chess_place_move_line_from_to(piece->side + 10, index,
                piece->cell_x, piece->cell_y, moves->targets[index].cell_x,
                moves->targets[index].cell_y, moves->targets[index].kind);
        }
        return;
    }
    case 6:
        if (mk_chess_pdata->manager.input_state != 6)
            mk_chess_pdata->manager.saved_input_state = mk_chess_pdata->manager.input_state;
        mk_chess_pdata->manager.input_state = mode;
        return;
    case 7:
        mk_chess_pdata->manager.input_state = mk_chess_pdata->manager.saved_input_state;
        return;
    case 8:
        mk_chess_pdata->manager.input_state = mode;
        mk_chess_game_over();
        return;
    }
}

/* TODO: [near miss] 96.863014%; portrait ownership recovered; flags and latch scheduling remain. */
void mk_chess_hud_set_piece_portrait(ChessPiece* piece)
{
    ChessSideHudState* hud = mk_chess_pdata->sides[piece->side]->hud;
    ScreenObj* portrait;
    ScreenObj* class_image;
    hud->flags &= ~1;
    if (hud->flags & 2) {
        if (hud->selected_piece != hud->saved_piece) {
            ChessPiece* previous = hud->saved_piece;
            hide_screen_obj(mk_chess_live_screen(&mk_chess_pdata->sides[previous->side]->portraits[previous->type]));
            previous = hud->saved_piece;
            hide_screen_obj(mk_chess_live_screen(&g_board_game_controller.class_definitions[previous->type].portraits[previous->side]));
        }
    }
    hud->saved_piece = piece;
    if (hud->selected_piece == 0) {
        if (piece->side == 0) {
            portrait = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
            mk_chess_restore_scroll_alpha(portrait);
            portrait->x = -120;
            class_image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
            class_image->x = -98;
            if (hud->selected_piece == 0) {
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[1])->x = -223;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[0])->x = -130;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[2])->x = -92;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[3])->x = -92;
            }
        } else {
            int offset = 0;
            if (g_board_game_controller.piece_libraries[piece->library_index].field_08 != 0) offset = 128;
            portrait = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
            mk_chess_restore_scroll_alpha(portrait);
            portrait->x = screen_width + 1 - offset;
            class_image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
            class_image->x = screen_width + 111 - g_board_game_controller.class_definitions[piece->type].field_16C;
            if (hud->selected_piece == 0) {
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[1])->x = screen_width - 24;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[0])->x = screen_width + 11;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[2])->x = screen_width;
                mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->team_art[3])->x = screen_width + 103;
            }
        }
        hud->flags |= 2;
    } else {
        hud->flags &= ~2;
    }
}

static void mk_chess_bottom_hud_snap_update(ChessSideHudState* hud, int force);

static inline void mk_chess_slide_bottom_image(ChessSideHudState* hud,
    ScreenObj* image, int y, signed char step)
{
    if (image != 0) unhide_screen_obj(image);
    image->y = y;
    if (hud->saved_piece->side == 1) image->x -= step;
    else image->x += step;
}

static inline void mk_chess_hide_bottom_team_image(ChessSideHudState* hud, int index)
{
    ScreenObj* image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[index]);
    if (image != 0) hide_screen_obj(image);
}

/* TODO: [near miss] 98.92534%; slide/completion recovered; equivalent flag tests and stores remain. */
static void mk_chess_bottom_hud_transition_update(ChessSideHudState* hud)
{
    signed char step = (signed char)(6.0f * game_speed);
    ChessPiece* piece;
    ScreenObj* image;
    int x;
    if (hud->flags & 1) step = (signed char)(-6.0f * game_speed);
    piece = hud->saved_piece;
    image = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
    mk_chess_slide_bottom_image(hud, image, 61, step);
    x = image->x;
    if (x > 5 && (x < screen_width - 258 ||
            (x < screen_width - 129 && g_board_game_controller.piece_libraries[hud->saved_piece->library_index].field_08 == 0))) {
        mk_chess_bottom_hud_snap_update(hud, 1);
        hud->selected_piece = hud->saved_piece;
        hud->flags &= ~2;
        return;
    }
    if ((hud->flags & 1) && (x < -130 || x > screen_width + 10)) {
        piece = hud->saved_piece;
        if (piece != 0) {
            image = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
            if (image != 0) hide_screen_obj(image);
            piece = hud->saved_piece;
            image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
            if (image != 0) hide_screen_obj(image);
            mk_chess_hide_bottom_team_image(hud, 1);
            mk_chess_hide_bottom_team_image(hud, 2);
            mk_chess_hide_bottom_team_image(hud, 3);
            mk_chess_hide_bottom_team_image(hud, 0);
        }
        hud->selected_piece = 0;
        hud->saved_piece = 0;
        hud->flags &= ~1;
        return;
    }
    piece = hud->saved_piece;
    image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
    mk_chess_slide_bottom_image(hud, image, 55, step);
    if (hud->selected_piece == 0) {
        image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[1]);
        mk_chess_slide_bottom_image(hud, image, 15, step);
        image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[2]);
        mk_chess_slide_bottom_image(hud, image, 42, step);
        image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[3]);
        mk_chess_slide_bottom_image(hud, image, 42, step);
        image->scale_x = 6.0f * hud->saved_piece->health;
        if (hud->saved_piece->side == 1) image->scale_x *= -1.0f;
        image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[0]);
        mk_chess_slide_bottom_image(hud, image, 59, step);
    }
}

/* TODO: [breakthrough needed] 91.29189%; snap positioning recovered; alpha branches and latch scheduling remain. */
static void mk_chess_bottom_hud_snap_update(ChessSideHudState* hud, int force)
{
    ChessPiece* piece = hud->selected_piece;
    ScreenObj* portrait;
    ScreenObj* image;
    if (piece != 0) {
        image = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
        if (image != 0) hide_screen_obj(image);
        piece = hud->selected_piece;
        image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
        if (image != 0) hide_screen_obj(image);
    }
    piece = hud->saved_piece;
    portrait = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
    unhide_screen_obj(portrait);
    if (force == 0) {
        MkObj* cursor = (MkObj*)mk_chess_cursor_live_header(&mk_chess_pdata->cursors[hud->side]);
        if (cursor != 0) {
            float forward, right, up;
            int obscured = 0;
            cam_calc_right_at_up_offsets(&cursor->pos.value, &forward, &right, &up);
            if (hud->selected_piece->side == 1) {
                if (right < -6.0f && up < -2.4f) obscured = 1;
            } else if (right > 6.0f && up < -2.4f) {
                obscured = 1;
            }
            mk_chess_set_target_paper_alpha(portrait, obscured ? 50 : 255);
        }
    }
    portrait->y = 61;
    piece = hud->saved_piece;
    if (piece->side == 1) {
        if (g_board_game_controller.piece_libraries[piece->library_index].field_08 != 0)
            portrait->x = screen_width - 257;
        else portrait->x = screen_width - 129;
    } else portrait->x = 5;
    piece = hud->saved_piece;
    image = mk_chess_live_screen(&g_board_game_controller.class_definitions[piece->type].portraits[piece->side]);
    if (image != 0) unhide_screen_obj(image);
    image->y = 55;
    piece = hud->saved_piece;
    if (piece->side == 1) image->x = screen_width - 19 - g_board_game_controller.class_definitions[piece->type].field_16C;
    else image->x = 27;
    image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[1]);
    if (image != 0) unhide_screen_obj(image);
    image->y = 15;
    image->x = hud->saved_piece->side == 1 ? screen_width - 154 : -98;
    image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[2]);
    if (image != 0) unhide_screen_obj(image);
    image->y = 42;
    image->x = hud->saved_piece->side == 1 ? screen_width - 130 : 33;
    image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[3]);
    if (image != 0) unhide_screen_obj(image);
    image->y = 42;
    image->x = hud->saved_piece->side == 1 ? screen_width - 27 : 33;
    image->scale_x = 6.0f * hud->saved_piece->health;
    if (hud->saved_piece->side == 1) image->scale_x *= -1.0f;
    image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->saved_piece->side]->team_art[0]);
    if (image != 0) unhide_screen_obj(image);
    image->y = 59;
    image->x = hud->saved_piece->side == 1 ? screen_width - 119 : -5;
    hud->selected_piece = hud->saved_piece;
}

static inline int mk_chess_expire_team_effect(ChessPieceEffect* effect)
{
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    if (!effect->flags.bits.bit5 || effect->expiry_clock >= (unsigned int)manager->clock) return 0;
    if (effect->flags.bits.bit7) fx_pause_emit(effect->emitter);
    if (effect->flags.bits.bit4) {
        TrackedSound* sound = effect->sound;
        if (sound != 0 && sound->hdr.instance != effect->sound_instance) sound = 0;
        if (sound != 0) {
            stop_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
            if (sound->hdr.instance != 0) sound->hdr.typed_vtbl->destroy(&sound->hdr);
        }
        effect->sound = 0;
        effect->sound_instance = 0;
    }
    if (effect->flags.bits.bit6) {
        if (effect->kind == 2) effect->expiry_clock = 0xFFFFFFFF;
        else {
            MkObj* object = mk_chess_live_imprison_object(effect);
            if (object != 0 && object->hdr.instance != 0) object->hdr.typed_vtbl->destroy(&object->hdr);
            if (effect->hdr.instance != 0) effect->hdr.typed_vtbl->destroy(&effect->hdr);
        }
    }
    if (effect->hdr.instance != 0) effect->hdr.typed_vtbl->destroy(&effect->hdr);
    return 1;
}

/* TODO: [breakthrough needed] 85.78966%; team monitoring recovered; flags/latches and effect scheduling remain. */
static float p_team_monitor(void)
{
    ChessSideHudState* hud = (ChessSideHudState*)pdata_of_proc(aproc);
    ChessSideState* team = mk_chess_pdata->sides[hud->side];
    unsigned int mode = mk_chess_pdata->manager.input_state;
    ChessPiece* piece = hud->selected_piece;
    unsigned int index;
    if (piece != hud->saved_piece) {
        if (!(hud->flags & 2)) mk_chess_bottom_hud_snap_update(hud, 0);
        else mk_chess_bottom_hud_transition_update(hud);
    } else if (piece != 0 && !(hud->flags & 2)) {
        ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        ScreenObj* portrait = mk_chess_live_screen(&mk_chess_pdata->sides[piece->side]->portraits[piece->type]);
        if (portrait != 0 && (manager->input_state == 0 || manager->input_state == 1)) {
            ChessCursor* owner = manager->input_state == 0 ? &mk_chess_pdata->cursors[hud->side] : &mk_chess_pdata->cursor;
            MkObj* cursor = (MkObj*)mk_chess_cursor_live_header(owner);
            if (cursor != 0) {
                float forward, right, up;
                int obscured = 0;
                cam_calc_right_at_up_offsets(&cursor->pos.value, &forward, &right, &up);
                if (hud->selected_piece->side == 1) {
                    if (right < -6.0f && up < -2.4f) obscured = 1;
                } else if (right > 6.0f && up < -2.4f) obscured = 1;
                mk_chess_set_target_paper_alpha(portrait, mk_chess_target_paper_alpha(portrait, obscured));
            }
        }
    }
    if (hud->flags_09 & 0x80) {
        int direction = hud->side == 0 ? 1 : -1;
        ScreenObj* image = mk_chess_live_screen(&mk_chess_pdata->sides[hud->side]->team_art[4]);
        if (image != 0) unhide_screen_obj(image);
        if (!(hud->flags_09 & 0x40)) {
            image->x += direction * 10;
            if (hud->side == 0 && image->x > 0) {
                image->x = 0;
                hud->flags_09 &= ~0x80;
            } else if (hud->side == 1 && image->x < screen_width - 128) {
                image->x = screen_width - 128;
                hud->flags_09 &= ~0x80;
            }
        } else {
            image->x -= direction * 10;
            if ((hud->side == 0 && image->x < -128) || (hud->side == 1 && image->x > screen_width)) {
                hide_screen_obj(image);
                hud->flags_09 &= ~0x80;
            }
        }
    }
    if (hud->flags & 0x80) mk_chess_cursor_tracker_update(hud);
    if (hud->flags & 0x40) {
        MkObj* cursor = (MkObj*)mk_chess_cursor_live_header(&mk_chess_pdata->cursor);
        if (!cursor->flags_08_bits.scale_active) {
            if (hud->cursor_scale_step > 0.0f) hud->flags &= ~0x40;
            else {
                cursor->flags_08_bits.scale_active = 1;
                cursor->scale.z = 1.0f;
                cursor->scale.y = 1.0f;
                cursor->scale.x = 1.0f;
                hud->cursor_scale_step = 0.02f * game_speed + 1.0f;
            }
        } else {
            if (cursor->scale.x > 2.5f) hud->cursor_scale_step = 0.98f * (hud->cursor_scale_step - 1.0f) + 1.0f;
            cursor->scale.x *= hud->cursor_scale_step;
            cursor->scale.z *= hud->cursor_scale_step;
        }
    }
    if (hud->flags & 0x20) {
        MkObj* cursor = (MkObj*)mk_chess_cursor_live_header(&mk_chess_pdata->cursor);
        switch ((hud->flags >> 3) & 3) {
        case 0:
            cursor->flags_08_bits.scale_active = 1;
            cursor->scale.z = 1.0f;
            cursor->scale.y = 1.0f;
            cursor->scale.x = 1.0f;
            hud->cursor_scale_step = 1.06f;
            hud->flags = (hud->flags & ~0x18) | 8;
            break;
        case 1:
            cursor->scale.x *= hud->cursor_scale_step;
            cursor->scale.z *= hud->cursor_scale_step;
            if (cursor->scale.x > 1.5f) {
                hud->cursor_scale_step = 0.96f;
                hud->flags = (hud->flags & ~0x18) | 0x10;
            }
            break;
        case 2:
            cursor->scale.x *= hud->cursor_scale_step;
            cursor->scale.z *= hud->cursor_scale_step;
            if (cursor->scale.x < 1.0f) {
                cursor->flags_08_bits.scale_active = 0;
                hud->cursor_scale_step = 0.0f;
                cursor->scale.z = 1.0f;
                cursor->scale.y = 1.0f;
                cursor->scale.x = 1.0f;
                hud->flags &= ~0x20;
                if (hud->flags & 4) hide_obj(cursor);
            }
            break;
        }
    }
    for (index = 0; index < team->live_piece_count; index++) {
        MkPtr* link;
        ChessCell* cell;
        piece = team->pieces[index];
        cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
        if (piece->flags.unknown_bit4) {
            advance_anim((AnimPdata*)piece->animation);
            pose_anim((AnimPdata*)team->pieces[index]->animation, 1);
        }
        link = team->pieces[index]->effects;
        while (link != 0) {
            ChessPieceEffect* effect = (ChessPieceEffect*)link->hdr;
            if (link->instance != effect->hdr.instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (!mk_chess_expire_team_effect(effect)) {
                    MkObj* object = mk_chess_live_imprison_object(effect);
                    if (object != 0) {
                        object->pos.value.x = cell->position.x;
                        object->pos.value.z = cell->position.z;
                    }
                }
                link = link->next;
            }
        }
    }
    if (mk_chess_pdata->manager.active_side == hud->side && team->controller->flags.drone_controlled) {
        if (--team->field_104 == 0 && (mode <= 1 || (mode == 9 && team->field_164 == 0))) {
            team->field_104 = 800;
            if (team->controller->flags.trap_placement_active) team->field_164 = 0;
            else {
                team->controller->flags.bit6 = 1;
                team->field_164 = 0;
                xfer_proc(team->input_proc, x_chess_4);
            }
        }
    }
    if (mk_chess_pdata->manager.active_side == hud->side && (mode <= 1 || mode == 9))
        mk_chess_handle_turn_timeout_scenerios(hud->side);
    return 1.0f;
}

static inline void mk_chess_schedule_bonus_row(ScreenObj* image, int start_x,
    int target_x, int y, int row, unsigned int lifetime, float (*completed)(void))
{
    MkHdr* allocation;
    ChessSlideMessagePdata* slide;
    ChessFadeMessagePdata* fade;
    int duration = (int)(8.0f * inverse_game_speed);
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_slide_display_msg_handler,
        sizeof(ChessSlideMessagePdata), &allocation);
    slide = (ChessSlideMessagePdata*)allocation;
    slide->object = image;
    unhide_screen_obj(image);
    slide->step = (target_x - start_x) / duration;
    slide->initial_delay = (unsigned int)((float)(row * 50) * inverse_game_speed);
    slide->target_x = target_x;
    slide->completed = completed;
    slide->object->x = start_x;
    slide->object->y = y;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_fade_display_msg_handler,
        sizeof(ChessFadeMessagePdata), &allocation);
    fade = (ChessFadeMessagePdata*)allocation;
    fade->object = image;
    fade->step = 6;
    fade->initial_delay = lifetime;
}

/* TODO: [breakthrough needed] 69.88411%; runtime cases agree; payload ownership and scheduling codegen remain nonexact. */
void mk_chess_advantage_hud(void)
{
    int count = 1;
    int row = 0;
    unsigned int lifetime;
    ScreenObj* image;
    int start_x;
    int center_x, center_y;
    unsigned int width;
    MkHdr* allocation;
    ChessScaleMessagePdata* scale;
    ChessFadeMessagePdata* fade;
    if (board_game_save_data.input_flags.p1_power_squares != 0) {
        count = board_game_save_data.input_flags.p1_power_squares;
        if (board_game_save_data.input_flags.pad_bit7) count++;
    } else if (board_game_save_data.input_flags.p2_power_squares != 0) {
        count = board_game_save_data.input_flags.p2_power_squares;
        if (!board_game_save_data.input_flags.pad_bit7) count++;
    }
    lifetime = (count - 1) * 8 + 125;
    _mkproc_sleep_ticks = 40.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if (board_game_save_data.input_flags.pad_bits2_1 & 2) {
        image = load_named_2d_pfxobj(0xE003E, 0xC020, "ON_POWER_CELL", 0, 23);
        mk_chess_schedule_bonus_row(image, -201, screen_width / 2 - 236,
            screen_height - 120, 0, lifetime, mk_chess_on_pwr_cell_1_bonus_slid_into_place_cb);
    } else {
        if (board_game_save_data.input_flags.pad_bit7) {
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_IN_GAME_ATTACKER_BONUS", 0, 23);
            mk_chess_schedule_bonus_row(image, -209, screen_width / 2 - 244,
                screen_height - 120, 0, lifetime, mk_chess_attacker_1_bonus_slid_into_place_cb);
            row = 1;
        }
        if (board_game_save_data.input_flags.p1_power_squares != 0) {
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_POWER_CELL_BONUS_25", 0, 23);
            mk_chess_schedule_bonus_row(image, -238, screen_width / 2 - 273,
                screen_height - 120 - row * 23, row, lifetime, mk_chess_pwr_cell_1_bonus_slid_into_place_cb);
            row++;
        }
        if (board_game_save_data.input_flags.p1_power_squares > 1) {
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_POWER_CELL_BONUS_25", 0, 23);
            mk_chess_schedule_bonus_row(image, -238, screen_width / 2 - 273,
                screen_height - 120 - row * 23, row, lifetime, mk_chess_pwr_cell_1_bonus_slid_into_place_cb);
        }
    }
    row = 0;
    if (board_game_save_data.input_flags.pad_bits2_1 & 1) {
        start_x = screen_width;
        image = load_named_2d_pfxobj(0xE003E, 0xC020, "ON_POWER_CELL", 0, 23);
        mk_chess_schedule_bonus_row(image, start_x, start_x / 2 + 35,
            screen_height - 120, 0, lifetime, mk_chess_on_pwr_cell_2_bonus_slid_into_place_cb);
    } else {
        if (!board_game_save_data.input_flags.pad_bit7) {
            start_x = screen_width;
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_IN_GAME_ATTACKER_BONUS", 0, 23);
            mk_chess_schedule_bonus_row(image, start_x, start_x / 2 + 35,
                screen_height - 120, 0, lifetime, mk_chess_attacker_2_bonus_slid_into_place_cb);
            row = 1;
        }
        if (board_game_save_data.input_flags.p2_power_squares != 0) {
            start_x = screen_width;
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_POWER_CELL_BONUS_25", 0, 23);
            mk_chess_schedule_bonus_row(image, start_x, start_x / 2 + 35,
                screen_height - 120 - row * 23, row, lifetime, mk_chess_pwr_cell_2_bonus_slid_into_place_cb);
            row++;
        }
        if (board_game_save_data.input_flags.p2_power_squares > 1) {
            start_x = screen_width;
            image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_POWER_CELL_BONUS_25", 0, 23);
            mk_chess_schedule_bonus_row(image, start_x, start_x / 2 + 35,
                screen_height - 120 - row * 23, row, lifetime, mk_chess_pwr_cell_2_bonus_slid_into_place_cb);
        }
    }
    _mkproc_sleep_ticks = (float)lifetime;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    image = load_named_2d_pfxobj(0xE003E, 0xC020, "MKC_IN_GAME_FIGHT", 0, 23);
    center_y = screen_height / 2;
    width = image->pfx2d->tex_w;
    center_x = screen_width / 2;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &allocation);
    scale = (ChessScaleMessagePdata*)allocation;
    scale->object = image;
    unhide_screen_obj(image);
    scale->step = 0.093333334f;
    scale->start_scale = 0.1f;
    scale->target_scale = 1.5f;
    scale->initial_delay = 0;
    scale->center_x = center_x;
    scale->center_y = center_y;
    scale->texture_width = width;
    scale->destroy_after_delay = 0;
    scale->final_delay = 0;
    scale->fade_after_midpoint = 0;
    scale->object->scale_x = 0.1f;
    scale->object->scale_y = 0.1f;
    scale->object->x = (int)-((float)(scale->texture_width >> 1) * scale->object->scale_x - (float)center_x);
    scale->object->y = (int)-((float)(scale->object->pfx2d->tex_h / 2) * scale->object->scale_y - (float)center_y);
    scale->object->flag_bits.scaled = 1;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_fade_display_msg_handler,
        sizeof(ChessFadeMessagePdata), &allocation);
    fade = (ChessFadeMessagePdata*)allocation;
    fade->object = image;
    fade->step = 51;
    fade->initial_delay = 60;
    snd_req_delay(0, 10);
    _mkproc_sleep_ticks = 65.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    board_game_save_data.knowledge_health[0] = g_game_info.plyr0.field_0C;
    board_game_save_data.knowledge_health[1] = g_game_info.plyr1.field_0C;
}

static inline int mk_chess_lone_piece_has_no_empty_move(unsigned int side)
{
    unsigned int i;
    unsigned int count;
    if (mk_chess_pdata->sides[side]->live_piece_count != 1) return 0;
    count = mk_chess_pdata->sides[side]->pieces[0]->move_map->target_count;
    for (i = 0; i < count; i++) {
        if (mk_chess_pdata->sides[side]->pieces[0]->move_map->targets[i].kind == 0)
            return 0;
    }
    return 1;
}

static inline void mk_chess_scale_timeout_image(ScreenObj* image, int center_y,
    unsigned int delay)
{
    MkHdr* allocation;
    ChessScaleMessagePdata* scale;
    unsigned int width = image->pfx2d->tex_w;
    int center_x = screen_width / 2;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &allocation);
    scale = (ChessScaleMessagePdata*)allocation;
    scale->object = image;
    unhide_screen_obj(image);
    scale->step = 0.07666667f;
    scale->start_scale = 0.25f;
    scale->target_scale = 1.4f;
    scale->initial_delay = 0;
    scale->center_x = center_x;
    scale->center_y = center_y;
    scale->texture_width = width;
    scale->destroy_after_delay = 1;
    scale->final_delay = delay;
    scale->fade_after_midpoint = 0;
    scale->object->scale_x = 0.25f;
    scale->object->scale_y = 0.25f;
    scale->object->x = (int)-((float)(scale->texture_width >> 1) * scale->object->scale_x - (float)center_x);
    scale->object->y = (int)-((float)(scale->object->pfx2d->tex_h / 2) * scale->object->scale_y - (float)center_y);
    scale->object->flag_bits.scaled = 1;
}

/* TODO: [breakthrough needed] 63.87%; runtime cases agree; payload reloads and scale-store scheduling remain nonexact. */
void mk_chess_timeout_msg(int message, unsigned int side)
{
    char names[6][15] = {
        "FIVE_MSG", "FOUR_MSG", "THREE_MSG", "TWO_MSG", "ONE_MSG", "TURN_LOST_MSG"
    };
    ScreenObj* image;
    MkHdr* allocation;
    ChessTurnExpiredPdata* expired;
    if (message == 5) {
        ChessSideState* team = mk_chess_pdata->sides[side];
        if (mk_chess_lone_piece_has_no_empty_move(side)) {
            team->hud->flags_09 |= 0x20;
            snd_req(0x35);
        } else {
            image = load_named_2d_pfxobj(0xD003C, 0xC01C, "TURN_LOST_MSG", 0, 0x52);
            mk_chess_scale_timeout_image(image, screen_height / 2 + 50, 65);
            snd_req(0x37B);
            snd_req_delay(0x33, 30);
        }
        _create_mkproc_generic_bigstack(0xC027, 31, p_mk_chess_turn_expired,
            sizeof(ChessTurnExpiredPdata), &allocation);
        mk_chess_pdata->input_transition_busy = 1;
        expired = (ChessTurnExpiredPdata*)allocation;
        expired->side = side;
        return;
    }
    if (side < 2 && mk_chess_lone_piece_has_no_empty_move(side)) {
        image = load_named_2d_pfxobj(0xD003C, 0xC01C, "FORFEIT_MSG", 0, 0x52);
        mk_chess_scale_timeout_image(image, screen_height / 2 + 50, 45);
        image = load_named_2d_pfxobj(0xD003C, 0xC01C, names[message], 0, 0x52);
        mk_chess_scale_timeout_image(image, screen_height / 2, 45);
        snd_req(0x37A);
        return;
    }
    image = load_named_2d_pfxobj(0xD003C, 0xC01C, names[message], 0, 0x52);
    mk_chess_scale_timeout_image(image, screen_height / 2 + 50, 45);
    snd_req(0x37A);
}

float p_idle_camera(void);

/* TODO: [breakthrough needed] 79.47738%; full event extent recovered; owner/latch and camera scheduling remain. */
float mk_chess_request_piece_fight(ChessPiece* piece, unsigned char x,
    unsigned char y, int forced)
{
    ChessPiece* defender = mk_chess_pdata->board[x].cells[y].piece;
    ChessModeState* owner = mk_chess_pdata;
    ChessCameraInfo* camera = owner != 0 ? &owner->camera : 0;
    ChessManagerInfo* manager = owner != 0 ? &owner->manager : 0;
    CameraObj* camera_object = camera_live_node(&camera_item);
    CmdScript* script;
    CmdScript* previous_script;
    MkPtr* link;
    unsigned int* restriction;
    unsigned int group;
    ChessPiece* pieces[2];
    /* Retail initializes the four coordinates and piece, then copies all sixteen bytes. */
    ChessMovementEvent event;
    Vec first, second;
    Vec* near_point;
    Vec* far_point;
    ChessManagerInfo* current_manager;
    ChessSideHudState* hud;
    MkObj* cursor;
    MkHdr* allocation = 0;
    ChessSceneFadePdata* fade;
    turn_controllers_off();
    mk_chess_pdata->saved_field_110 = mk_chess_pdata->manager.clock;
    if (camera->field_4C == 0 && camera->field_50 == 0)
        camera->saved_position = camera_object->pos;
    mk_chess_set_game_mode(3);
    if (((ChessTableHeader*)g_game_info.mode_table)->fight_function != 0) {
        script = alloc_cmdscript();
        previous_script = active_cmdscript;
        active_cmdscript = script;
        cmdscript_setup_execution(g_game_info.cmdscript,
            ((ChessTableHeader*)g_game_info.mode_table)->fight_function);
        cmdscript_execute(g_game_info.cmdscript);
        active_cmdscript = previous_script;
        if (script->instance != 0) ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
    }
    link = piece->effects;
    while (link != 0) {
        ChessPieceEffect* effect = (ChessPieceEffect*)link->hdr;
        if (link->instance != effect->hdr.instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (effect->kind == 1) {
                if (effect->flags.bits.bit7) {
                    fx_pause_emit(effect->emitter);
                }
                if (effect->flags.bits.bit4) {
                    TrackedSound* sound = effect->sound;
                    if (sound != 0 && sound->hdr.instance != effect->sound_instance) {
                        sound = 0;
                    }
                    if (sound != 0) {
                        stop_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
                        if (sound->hdr.instance != 0) {
                            sound->hdr.typed_vtbl->destroy(&sound->hdr);
                        }
                    }
                    effect->sound = 0;
                    effect->sound_instance = 0;
                }
                if (effect->flags.bits.bit6) {
                    if (effect->kind == 2) {
                        effect->expiry_clock = 0xFFFFFFFF;
                    } else {
                        MkObj* object = mk_chess_live_imprison_object(effect);
                        if (object != 0 && object->hdr.instance != 0) {
                            object->hdr.typed_vtbl->destroy(&object->hdr);
                        }
                        if (effect->hdr.instance != 0) {
                            effect->hdr.typed_vtbl->destroy(&effect->hdr);
                        }
                    }
                }
            }
            link = link->next;
        }
    }
    restriction = piece != 0 ? &piece->access_restrictions[2] : 0;
    if (restriction != 0) {
        *restriction = 0;
    }

    stop_sound_tracking_process(&mk_chess_pdata->tracked_sounds);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    destroy_mkprocs_pid(0xC020);
    for (group = 10; group <= 15; group++) bgnd_hide_pebbles(group);
    destroy_mkprocs_pid(0xC021);
    bgnd_hide_pebbles(16);
    bgnd_hide_pebbles(17);
    manager->event_data.other_piece = piece;
    manager->event_piece_24 = defender;
    mk_chess_set_vars_for_a_mk_fight();
    hide_obj(mk_chess_cursor_live_object(&mk_chess_pdata->cursors[0]));
    hide_obj(mk_chess_cursor_live_object(&mk_chess_pdata->cursors[1]));
    owner->fight_start_tick = exec_tick_ctr;
    if (forced == 0) {
        camera->zoom_sound_enabled = 1;
        event.coordinates[0] = piece->cell_x;
        event.coordinates[1] = piece->cell_y;
        event.coordinates[2] = x;
        event.coordinates[3] = y;
        event.piece = defender;
        pieces[0] = piece;
        pieces[1] = defender;
        mk_chess_game_event(5, pieces, 2, &event);
        first = piece->object->pos.value;
        second = defender->object->pos.value;
        current_manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
        hud = mk_chess_pdata->sides[current_manager->active_side]->hud;
        cursor = mk_chess_cursor_live_object(&mk_chess_pdata->cursor);
        hud->flags |= 0x40;
        hud->flags &= ~0x20;
        hud->cursor_scale_step = 0.0f;
        cursor->flags_08_bits.scale_active = 0;
        (mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0)->completion = mk_chess_pre_fight_cam_ended;
        near_point = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].position;
        far_point = &mk_chess_pdata->board[defender->cell_x].cells[defender->cell_y].position;
        first = *near_point;
        second = *far_point;
        if (piece->side == 1) {
            first = *far_point;
            second = *near_point;
        }
        mk_chess_set_up_fight_cam(&first, &second, near_point, far_point);
        xfer_camera(p_mk_chess_cam_bezier_controller, 0);
    } else {
        event.coordinates[0] = piece->cell_x;
        event.coordinates[1] = piece->cell_y;
        event.coordinates[2] = x;
        event.coordinates[3] = y;
        event.piece = defender;
        pieces[0] = piece;
        pieces[1] = defender;
        mk_chess_game_event(8, pieces, 2, &event);
        xfer_camera(p_idle_camera, 0);
    }
    snd_stop(mk_chess_pdata->camera_sound.field_00);
    snd_req(0x3B6);
    mk_chess_deactivate_all_special_cells(1);
    if (_create_mkproc_generic_tinystack(0xC01E, 31, p_mk_chess_fade_scene_for_fight,
        sizeof(ChessSceneFadePdata), &allocation) != 0 && allocation != 0) {
        fade = (ChessSceneFadePdata*)allocation;
        fade->first = defender;
        fade->second = piece;
        fade->delay = (unsigned int)(20.0f * inverse_game_speed);
        fade->step = (int)(8.0f * game_speed);
    }
    if (forced == 1) {
        _mkproc_sleep_ticks = 120.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        _create_mkproc_generic_tinystack(0xC01E, 31, mk_chess_continue_pre_fight_chores, 0, 0);
    }
    return 0.0f;
}

extern int round_winner;
int get_chess_coin_award(int coin_index);
int get_chess_leader_won_coin_award(void);
const char* get_rnd_chess_koin_type(void);

/* TODO: [breakthrough needed] 93.26%; result and award traces agree; HUD and award lowering remain nonexact. */
void mk_chess_game_just_ended(void)
{
    PlyrInfo* winner_info = &g_game_info.plyr0;
    int leader_lost;
    ScreenObj* background;
    ScreenObj* name;
    StringObj* message;
    int award_y;
    get_player_proc(g_game_info.plyr0.slot.mirror_a);
    if ((board_game_save_data.ai_settings[1] & 0x40) && g_game_info.plyr1.field_0C == 0.0f)
        leader_lost = 1;
    else if ((board_game_save_data.ai_settings[0] & 0x80) && g_game_info.plyr0.field_0C == 0.0f)
        leader_lost = 1;
    else leader_lost = 0;
    if (round_winner == 3 || round_winner == 0) {
        round_winner = 1;
        winner = 1;
        g_game_info.plyr0.field_0C = 0.03f;
        g_game_info.plyr1.field_0C = 0.0f;
    }
    if (round_winner == 2 && g_game_info.plyr1.field_0C < 0.03f)
        g_game_info.plyr1.field_0C = 0.03f;
    if (round_winner == 1 && g_game_info.plyr0.field_0C < 0.03f)
        g_game_info.plyr0.field_0C = 0.03f;
    background = load_named_2d_pfxobj(0xE003E, 0xC01C, "WIN_SQUARE", 0, 0x52);
    background->y = (screen_height - background->pfx2d->tex_h) / 2;
    background->x = (screen_width - background->pfx2d->tex_w) / 2;
    unhide_screen_obj(background);
    if (round_winner == 1) {
        message = string_center_xy(0xC01C, 3, get_string_by_id(0x20000), screen_width / 2, screen_height / 2 - 5, 0x4E);
        message->pfx.instance0.native_color.b = 200;
        message->pfx.instance0.native_color.g = 100;
        message->pfx.instance0.native_color.r = 100;
        if (leader_lost == 0) snd_req_delay(0x24, 40);
        else { snd_req_delay(0x29, 20); snd_req_delay(0x2A, 120); }
    } else {
        winner_info = &g_game_info.plyr1;
        message = string_center_xy(0xC01C, 3, get_string_by_id(0x20001), screen_width / 2, screen_height / 2 - 5, 0x4E);
        message->pfx.instance0.native_color.b = 100;
        message->pfx.instance0.native_color.g = 100;
        message->pfx.instance0.native_color.r = 200;
        if (leader_lost == 0) snd_req_delay(0x25, 40);
        else { snd_req_delay(0x29, 20); snd_req_delay(0x2B, 120); }
    }
    if (leader_lost != 0)
        message = string_center_xy(0xC01C, 3, get_string_by_id(0x20049), screen_width / 2, screen_height / 2 - 35, 0x4E);
    else
        message = string_center_xy(0xC01C, 3, get_string_by_id(0x2004A), screen_width / 2, screen_height / 2 - 35, 0x4E);
    if (round_winner == 1) {
        message->pfx.instance0.native_color.b = 200;
        message->pfx.instance0.native_color.g = 100;
        message->pfx.instance0.native_color.r = 100;
    } else {
        message->pfx.instance0.native_color.b = 100;
        message->pfx.instance0.native_color.g = 100;
        message->pfx.instance0.native_color.r = 230;
    }
    _mkproc_sleep_ticks = leader_lost != 0 ? 160.0f : 80.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if (winner == 2) get_player_proc(g_game_info.plyr1.slot.mirror_a);
    f_fatality_finished = 1;
    _mkproc_sleep_ticks = 60.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if ((g_game_info.plyr0.player_state == 0 || g_game_info.plyr1.player_state == 0) &&
        ((round_winner == 1 && g_game_info.plyr0.player_state == 2) ||
         (round_winner == 2 && g_game_info.plyr1.player_state == 2)) &&
        ((round_winner == 1 && p1_profile_status == 1) ||
         (round_winner == 2 && p2_profile_status == 1))) {
        if (round_winner == 1) {
            g_game_info.pselect.field_1ec = get_chess_coin_award((board_game_save_data.ai_settings_word >> 14) & 15);
            if (board_game_save_data.ai_settings[0] & 0x80)
                g_game_info.pselect.field_1ec += get_chess_leader_won_coin_award();
            get_rnd_chess_koin_type();
        } else {
            g_game_info.pselect.field_1ec = get_chess_coin_award((board_game_save_data.ai_settings_halves[0] >> 7) & 15);
            if (board_game_save_data.ai_settings[1] & 0x40)
                g_game_info.pselect.field_1ec += get_chess_leader_won_coin_award();
            get_rnd_chess_koin_type();
        }
        name = (ScreenObj*)winner_info->name_latch.object;
        if (name != 0 && name->instance != winner_info->name_latch.instance) name = 0;
        award_y = name != 0 ? 88 : 63;
        show_koin_award(round_winner - 1, g_game_info.pselect.field_1ec, g_game_info.pselect.field_1e4, award_y);
        award_koins_to_player(round_winner - 1, g_game_info.pselect.field_1ec, g_game_info.pselect.field_1e4);
        _mkproc_sleep_ticks = 30.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    _mkproc_sleep_ticks = 105.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
}

static inline void mk_chess_create_protection_blip(ChessPieceEffect* effect,
    ChessCell* cell, const char* name, float height)
{
    float x = cell->position.x;
    float z = cell->position.z;
    unsigned int emitter = fx_by_owner(name, 4);
    MkObj* object = launch_fx_at_pos_with_obj(emitter, x, height, z);
    effect->object = object;
    effect->object_instance = object->hdr.instance;
    effect->emitter = fx_by_owner(name, 4);
}

/* TODO: [breakthrough needed] 81.970276%; effect ownership recovered; emitter inlining and allocation scheduling remain. */
void start_protect_effect(ChessPiece* piece, int duration)
{
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    ChessPieceEffect* effects[3];
    ChessPieceEffect* effect;
    MkObj* object;
    TrackedSound* sound;
    unsigned int i;
    for (i = 0; i < 3; i++)
        effects[i] = (ChessPieceEffect*)get_mkpdata_generic(sizeof(ChessPieceEffect));
    if (piece->side == 0) {
        mk_chess_create_protection_blip(effects[0], cell, "blip_1", 2.0f);
        mk_chess_create_protection_blip(effects[1], cell, "blip_2", 3.0f);
        mk_chess_create_protection_blip(effects[2], cell, "blip_3", 1.0f);
    } else {
        mk_chess_create_protection_blip(effects[0], cell, "blip_4", 2.0f);
        mk_chess_create_protection_blip(effects[1], cell, "blip_5", 3.0f);
        mk_chess_create_protection_blip(effects[2], cell, "blip_6", 1.0f);
    }
    for (i = 0; i < 3; i++) {
        effect = effects[i];
        effect->kind = 1;
        effect->flags.byte = 0;
        effect->flags.bits.bit5 = 1;
        effect->expiry_clock = (unsigned int)duration + (unsigned int)manager->clock;
        effect->flags.bits.bit7 = 1;
        effect->flags.bits.bit6 = 1;
        object = mk_chess_live_imprison_object(effect);
        if (object != 0) {
            object->flags_08_bits.rotation_enabled = 1;
            object->flags_08_bits.angular_velocity_enabled = object->flags_08_bits.rotation_enabled;
            object->ang.z = 0.0f;
            object->ang.y = 0.0f;
            object->ang.x = 0.0f;
            object->ang_vel.z = 0.0f;
            object->ang_vel.y = 0.0f;
            object->ang_vel.x = 0.0f;
            object->ang_vel.y = 0.065f;
            mk_insert(&effect->hdr, &piece->effects);
            effect->sound = 0;
            effect->sound_instance = 0;
        }
    }
    effects[0]->flags.bits.bit4 = 1;
    sound = get_sound_tracker_data();
    effects[0]->sound = sound;
    effects[0]->sound_instance = sound->hdr.instance;
    if (sound != 0) {
        sound->pos = cell->position;
        sound->sound_id = 0x398;
        sound->min_dist = 38.0f;
        sound->max_dist = 60.0f;
        sound->positional_pan = 1;
        sound->tracking_enabled = 1;
        sound->owner_uid = 0;
        make_new_tracked_sound(&mk_chess_pdata->tracked_sounds, sound);
    }
}

static inline void mk_chess_priority_move(unsigned int side, ChessPiece* piece, unsigned int x, unsigned int y)
{
    ChessDroneState* drone = (ChessDroneState*)mk_chess_pdata->sides[side];
    drone->target_0_x = piece->cell_x;
    drone->target_0_y = piece->cell_y;
    drone->target_1_x = x;
    drone->target_1_y = y;
    drone->action_state = 1;
    drone->cooldown = 5;
}

static inline unsigned int mk_chess_priority_attacker(ChessPiece* target, ChessPiece** best)
{
    unsigned int rating = 0;
    unsigned int index;
    unsigned int enemy_side = target->side == 0;
    for (index = 0; index < mk_chess_pdata->sides[enemy_side]->live_piece_count; index++) {
        ChessPiece* attacker = mk_chess_pdata->sides[enemy_side]->pieces[index];
        if (attacker != 0 && ((attacker->move_map->rows[target->cell_y] >> (target->cell_x * 3)) & 7) == 2) {
            unsigned int candidate = mk_chess_drone_calc_fight_favorability_rating(attacker, target);
            if (rating < candidate) { rating = candidate; *best = attacker; }
        }
    }
    return rating;
}

/* TODO: [breakthrough needed] 85.99%; priority branches recovered; selection loops and owner scheduling remain. */
static unsigned int mk_chess_drone_prepare_high_priority_items(unsigned int side)
{
    ChessPiece* own_3 = mk_chess_find_piece_of_type(side, 3, 0);
    ChessPiece* own_4 = mk_chess_find_piece_of_type(side, 4, 0);
    ChessPiece* enemy_3 = mk_chess_find_piece_of_type(side == 0, 3, 0);
    ChessPiece* enemy_4 = mk_chess_find_piece_of_type(side == 0, 4, 0);
    ChessPiece* own_king = mk_chess_find_piece_of_type(side, 5, 0);
    ChessPiece* enemy_king = mk_chess_find_piece_of_type(side == 0, 5, 0);
    ChessPiece* own_attacker = 0;
    ChessPiece* own_setup_attacker = 0;
    ChessPiece* enemy_attacker = 0;
    ChessPiece* enemy_setup_attacker = 0;
    int own_immediate, own_setup, enemy_immediate, enemy_setup;
    unsigned short roll;
    unsigned int x, y, index;
    unsigned int target_x = 0, target_y = 0;
    ChessPiece* selected = 0;
    ChessPiece* occupant;
    int occupied = 0;
    int found = 0;
    unsigned int rating;
    unsigned int chance;
    mk_chess_calc_piece_vulnerability_rating(own_king, &own_immediate, &own_setup, &own_attacker, &own_setup_attacker);
    mk_chess_calc_piece_vulnerability_rating(enemy_king, &enemy_immediate, &enemy_setup, &enemy_attacker, &enemy_setup_attacker);
    roll = (unsigned short)randu0(100);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    if ((unsigned int)own_immediate > 4 && (unsigned int)enemy_immediate < 4 &&
        mk_chess_deal_with_threat_to(side, own_king, own_attacker)) return 1;
    if ((((unsigned int)enemy_immediate > 4 && (roll < 90 || game_settings.arcade_difficulty > 2)) ||
        ((unsigned int)enemy_immediate > 3 && roll < 70)) &&
        mk_chess_drone_move_best_matchup_against_piece(side, enemy_king, 1) == 1) return 1;
    for (x = 0; x < 10 && !found; x++) {
        for (y = 0; y < 10 && !found; y++) {
            ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
            occupant = cell->piece;
            if (cell->square_type == 1 && (occupant == 0 || occupant->side != side)) {
                for (index = 0; index < mk_chess_pdata->sides[side]->live_piece_count; index++) {
                    ChessPiece* candidate = mk_chess_pdata->sides[side]->pieces[index];
                    if ((candidate->move_map->rows[y] >> (x * 3)) & 7) {
                        selected = candidate;
                        target_x = x;
                        target_y = y;
                        occupied = mk_chess_pdata->board[x].cells[y].piece != 0;
                        found = 1;
                        break;
                    }
                }
            }
        }
    }
    if (found) {
        if (!occupied) { mk_chess_priority_move(side, selected, target_x, target_y); return 1; }
        roll = (unsigned short)randu0(100);
        rating = mk_chess_drone_calc_fight_favorability_rating(selected, mk_chess_pdata->board[target_x].cells[target_y].piece);
        if ((((ChessDroneState*)mk_chess_pdata->sides[side])->strategy == 2 && roll > 70) ||
            (rating > 6 && roll < 40) || (rating > 4 && roll < 15) || roll < 10) {
            mk_chess_priority_move(side, selected, target_x, target_y); return 1;
        }
    }
    if ((unsigned int)enemy_setup > 4) {
        chance = 30;
        if ((unsigned int)mk_chess_pdata->manager.clock > 40) chance = 70;
        if (((ChessDroneState*)mk_chess_pdata->sides[side])->strategy == 1) chance = 90;
        if ((unsigned short)randu0(100) < chance && mk_chess_drone_move_best_matchup_against_piece(side, enemy_king, 1) == 1) return 1;
    }
    if (own_3 != 0) {
        rating = mk_chess_priority_attacker(own_3, &selected);
        if (rating >= 4 && (unsigned short)randu0(100) < 50 && mk_chess_deal_with_minor_threat_to(side, own_3, selected)) return 1;
    }
    if (own_4 != 0) {
        rating = mk_chess_priority_attacker(own_4, &selected);
        if (rating >= 4 && (unsigned short)randu0(100) < 50 && mk_chess_deal_with_minor_threat_to(side, own_4, selected)) return 1;
    }
    if (enemy_3 != 0) {
        rating = mk_chess_priority_attacker(enemy_3, &selected);
        if (rating >= 7 && (unsigned short)randu0(100) < 50) {
            mk_chess_priority_move(side, selected, enemy_3->cell_x, enemy_3->cell_y); return 1;
        }
    }
    if (enemy_4 != 0) {
        rating = mk_chess_priority_attacker(enemy_4, &selected);
        if (rating >= 7 && (unsigned short)randu0(100) < 50) {
            mk_chess_priority_move(side, selected, enemy_4->cell_x, enemy_4->cell_y); return 1;
        }
    }
    return 0;
}

extern void remove_fgnd_mkobj(MkObj* object);

static inline void mk_chess_move_trap_cursor(ChessDirectionState* hud,
    unsigned int side, int direction)
{
    ChessCursor* cursor = &hud->cursors[side];
    int x = cursor->cell_x;
    int y = cursor->cell_y;
    ChessCell* cell;
    MkObj* object;
    snd_req(0x36D);
    if (side == 0) move_cursor_based_on_quadrant(side, &x, &y, direction, 0, 5, 0);
    else move_cursor_based_on_quadrant(side, &x, &y, direction, 5, 10, 0);
    cell = &mk_chess_pdata->board[(unsigned char)x].cells[(unsigned char)y];
    object = mk_chess_cursor_live_object(cursor);
    cursor->cell_x = x;
    cursor->cell_y = y;
    object->pos.value = cell->position;
    update_obj_pos(object);
}

static inline void mk_chess_scale_trap_ready(ScreenObj* image, unsigned int side)
{
    MkHdr* allocation;
    ChessScaleMessagePdata* scale;
    unsigned int width = image->pfx2d->tex_w;
    int center_y = screen_height / 2 + 50;
    int center_x = side == 0 ? screen_width / 4 : screen_width * 3 / 4;
    _create_mkproc_generic_tinystack(0xC023, 31, p_mk_chess_scale_display_msg_handler,
        sizeof(ChessScaleMessagePdata), &allocation);
    scale = (ChessScaleMessagePdata*)allocation;
    scale->object = image;
    unhide_screen_obj(image);
    scale->step = 0.044999998f;
    scale->start_scale = 0.1f;
    scale->target_scale = 1.0f;
    scale->initial_delay = 0;
    scale->center_x = center_x;
    scale->center_y = center_y;
    scale->texture_width = width;
    scale->destroy_after_delay = 0;
    scale->final_delay = 0;
    scale->fade_after_midpoint = 0;
    scale->object->scale_x = 0.1f;
    scale->object->scale_y = 0.1f;
    scale->object->x = (int)-((float)(scale->texture_width >> 1) * scale->object->scale_x - (float)center_x);
    scale->object->y = (int)-((float)(scale->object->pfx2d->tex_h / 2) * scale->object->scale_y - (float)center_y);
    scale->object->flag_bits.scaled = 1;
}

/* TODO: [breakthrough needed] 74.97%; trap flow recovered; helper expansion, cursor dispatch and cleanup scheduling remain. */
float p_mk_chess_place_traps(void)
{
    ChessManagerInfo* manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    ChessDirectionState* hud = manager->directional_state;
    int confirmed[2] = {0, 0};
    int selected[2] = {0, 0};
    ScreenObj* ready[2] = {0, 0};
    unsigned int remaining = 180;
    int expired = 0;
    unsigned int side, index, active_side;
    MkPtr* link;
    MkHdr* allocation;
    MkProc* process;
    StringObj* text;
    int group;
    ChessShowSidePdata* display;
    destroy_mkprocs_pid(0xC027);
    mk_chess_pdata->turn_timeout = (unsigned int)(1800.0f * inverse_game_speed);
    mk_chess_pdata->input_transition_busy = 0;
    mk_chess_drone_setup_your_traps((ChessDroneState*)mk_chess_pdata->sides[0]);
    mk_chess_drone_setup_your_traps((ChessDroneState*)mk_chess_pdata->sides[1]);
    while (remaining != 0) {
        if (expired == 0) expired = mk_chess_handle_turn_timeout_scenerios(2);
        if (expired != 0) {
            for (side = 0; side < 2; side++) {
                if (selected[side] == 0) {
                    ChessCursor* cursor = &hud->cursors[side];
                    hud->actions[side] = 1;
                    if (mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].square_type != 0)
                        cursor->cell_x++;
                } else if (confirmed[side] == 0) hud->actions[side] = 3;
            }
            destroy_mkprocs_pid(0xC027);
            mk_chess_pdata->turn_timeout = (unsigned int)(1800.0f * inverse_game_speed);
            mk_chess_pdata->input_transition_busy = 0;
        }
        if (confirmed[0] == 1 && confirmed[1] == 1) remaining--;
        for (side = 0; side < 2; side++) {
            switch (hud->actions[side]) {
            case 4: mk_chess_move_trap_cursor(hud, side, 4); break;
            case 5: mk_chess_move_trap_cursor(hud, side, 0); break;
            case 7: mk_chess_move_trap_cursor(hud, side, 2); break;
            case 9: mk_chess_move_trap_cursor(hud, side, 1); break;
            case 8: mk_chess_move_trap_cursor(hud, side, 3); break;
            case 6: mk_chess_move_trap_cursor(hud, side, 6); break;
            case 11: mk_chess_move_trap_cursor(hud, side, 7); break;
            case 10: mk_chess_move_trap_cursor(hud, side, 5); break;
            case 1: {
                ChessCursor* cursor = &hud->cursors[side];
                if (mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].square_type == 0) {
                    selected[side] = 1;
                    snd_req(0x378);
                    hud->trap_x[side] = cursor->cell_x;
                    hud->trap_y[side] = cursor->cell_y;
                } else snd_req(0x378);
                break;
            }
            case 2: snd_req(0x378); break;
            case 3: {
                int was_selected = selected[side];
                if (was_selected == 1 && confirmed[side] == 0) {
                    ready[side] = load_named_2d_pfxobj(0xD003C, 0xC01C, "READY_MSG", 0, 82);
                    mk_chess_scale_trap_ready(ready[side], side);
                }
                if (was_selected == 1) {
                    confirmed[side] = 1;
                    snd_req(0x375);
                }
                break;
            }
            }
            hud->actions[side] = 0;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
    mk_chess_pdata->manager.flags &= ~0x80;
    turn_controllers_off();
    for (side = 0; side < 2; side++) {
        mk_chess_pdata->sides[side]->desired_x = hud->trap_x[side];
        mk_chess_pdata->sides[side]->desired_y = hud->trap_y[side];
        mk_chess_place_special_cell_at(hud->trap_x[side], hud->trap_y[side], 2, 0,
            (float)side, 0.0f, 0.0f, 0.0f);
    }
    fade_to_black(5, 0);
    link = hud->strings;
    while (link != 0) {
        MkHdr* object = link->hdr;
        if (link->instance != object->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (object->instance != 0) object->typed_vtbl->destroy(object);
            link = link->next;
        }
    }
    if (hud->title->instance != 0) ((MkHdr*)hud->title)->typed_vtbl->destroy((MkHdr*)hud->title);
    destroy_mkprocs_pid(0xC026);
    for (side = 0; side < 2; side++) {
        MkObj* object = mk_chess_cursor_live_object(&hud->cursors[side]);
        remove_fgnd_mkobj(object);
        if (object->hdr.instance != 0) object->hdr.typed_vtbl->destroy(&object->hdr);
    }
    if (ready[0]->instance != 0) ((MkHdr*)ready[0])->typed_vtbl->destroy((MkHdr*)ready[0]);
    if (ready[1]->instance != 0) ((MkHdr*)ready[1])->typed_vtbl->destroy((MkHdr*)ready[1]);
    set_game_switch_maps();
    mk_chess_camera_init();
    mk_chess_set_game_mode(6);
    for (side = 0; side < 2; side++) {
        ChessSideState* team = mk_chess_pdata->sides[side];
        team->controller->flags.trap_placement_active = 0;
        team->field_164 = 0;
    }
    destroy_mkpdata_generic(&hud->hdr);
    manager->directional_state = 0;
    if (board_game_save_data.input_flags.input_locked) {
        text = string_center_xy(0x2010, 0, get_string(1), screen_width / 2, 65, 29);
        process = _create_mkproc_generic_tinystack(0xC026, 31, p_mk_chess_blink_string,
            sizeof(ChessBlinkStringPdata), &allocation);
        if (process != 0) {
            ChessBlinkStringPdata* blink = (ChessBlinkStringPdata*)allocation;
            blink->text = text;
            blink->text_instance = text->instance;
            mk_insert((MkHdr*)text, &process->pdata_list_b);
            blink->timer = 20;
            blink->interval = 20;
        }
    }
    fade_from_black(5, 0);
    active_side = mk_chess_pdata->manager.active_side;
    group = active_side == 1 ? 17 : 16;
    allocation = 0;
    if (_create_mkproc_generic_bigstack(0xC021, 31, p_mk_chess_show_my_side,
        sizeof(ChessShowSidePdata), &allocation) != 0) {
        display = (ChessShowSidePdata*)allocation;
        display->side = mk_chess_pdata->sides[active_side];
        display->pebble_group = group;
        display->amount = 0.01f;
        display->acceleration = 0.02f;
        display->maximum = 1.2f;
        display->current = 0.0f;
        for (index = 0; index < display->side->live_piece_count; index++) {
            ChessPiece* piece = display->side->pieces[index];
            ChessCell* cell = &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
            bgnd_pebble_set_current_pebble(display->pebble_group, index);
            bgnd_pebble_set_current_info(10, 0.0f);
            bgnd_pebble_set_current_info(9, cell->position.x);
            bgnd_pebble_set_current_info(11, cell->position.z);
            bgnd_pebble_set_current_info(14, 0.01f);
            bgnd_pebble_set_current_info(12, 0.01f);
            bgnd_pebble_set_current_info(15, 4.0f);
        }
        bgnd_unhide_pebbles(group);
    }
    turn_controllers_on();
    mk_chess_set_game_mode(0);
    destroy_mkprocs_pid(0xC027);
    mk_chess_pdata->turn_timeout = (unsigned int)(1800.0f * inverse_game_speed);
    mk_chess_pdata->input_transition_busy = 0;
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_mk_chess_loop, 0.0f);
    return 0.0f;
}

static inline void mk_chess_return_selection_cursor(ChessCursor* cursor,
    unsigned char x, unsigned char y)
{
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];
    MkObj* object = mk_chess_cursor_live_object(cursor);
    cursor->cell_x = x;
    cursor->cell_y = y;
    object->pos.value = cell->position;
    update_obj_pos(object);
}

/* TODO: [breakthrough] 61.04%; tested confirm paths agree; shared move/fight branches and cursor lowering remain. */
float x_chess_3(void)
{
    ChessPiece* target;
    unsigned char x, y;
    ChessSideHudState* hud;
    if (mk_chess_pdata == 0) return -1.0f;
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }
    switch (mk_chess_pdata->manager.input_state) {
    case 10:
        mk_chess_pdata->manager.directional_state->actions[((ChessInputPdata*)apdata)->side] = 3;
        break;
    case 9:
        mk_chess_pdata->manager.spell->input_state = 9;
        break;
    case 0: {
        ChessCursor* cursor = &mk_chess_pdata->cursors[mk_chess_pdata->manager.active_side];
        target = mk_chess_pdata->board[cursor->cell_x].cells[cursor->cell_y].piece;
        if (target->move_map->target_count == 0) snd_req(0x36F);
        else {
            if (target->type == 2 || target->type == 1) snd_req(0x36B);
            else snd_req(0x36C);
            mk_chess_set_game_mode(1);
            mk_chess_game_event(1, mk_chess_pdata->manager.event_data.pieces, 1, 0);
        }
        break;
    }
    case 1:
        x = mk_chess_pdata->cursor.cell_x;
        y = mk_chess_pdata->cursor.cell_y;
        target = mk_chess_pdata->board[x].cells[y].piece;
        if (target != 0 && target->side == mk_chess_pdata->manager.active_side) {
            x = mk_chess_pdata->manager.event_data.piece->cell_x;
            y = mk_chess_pdata->manager.event_data.piece->cell_y;
            mk_chess_return_selection_cursor(&mk_chess_pdata->cursors[0], x, y);
            mk_chess_return_selection_cursor(&mk_chess_pdata->cursors[1], x, y);
            mk_chess_return_selection_cursor(&mk_chess_pdata->cursor, x, y);
            break;
        }
        if (((mk_chess_pdata->manager.event_data.piece->move_map->rows[y] >> (x * 3)) & 7) == 0)
            break;
        mk_chess_pdata->turn_timeout = 3000;
        hud = mk_chess_pdata->sides[mk_chess_pdata->manager.active_side]->hud;
        hud->flags |= 0x20;
        hud->cursor_scale_step = 1.0f;
        hud->flags &= ~0x18;
        if (target == 0) hud->flags |= 4;
        else hud->flags &= ~4;
        snd_req(0x36E);
        board_game_save_data.sides[mk_chess_pdata->manager.active_side].fight_stat_710++;
        if (target == 0)
            mk_chess_request_piece_move(mk_chess_pdata->manager.event_data.piece, x, y, 1);
        else
            mk_chess_request_piece_fight(mk_chess_pdata->manager.event_data.piece, x, y, 0);
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}
