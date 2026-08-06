#include "game/mk_chess.h"
#include "game/game_info.h"
#include "game/settings.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "platform/io.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "game/pfxscript.h"

typedef int (*ChessProcVtableFn)(void);
typedef int (*ChessProcJumpFn)(MkProcEntryFn entry, float ticks);

typedef struct ChessProcVtable {
    ChessProcVtableFn reserved[6];
    ChessProcVtableFn sleep;
    ChessProcVtableFn stack_ops[2];
    ChessProcJumpFn jump_sleep; /* +0x24 */
} ChessProcVtable;

typedef struct ChessSwitchState {
    int field_00;
    unsigned int player; /* +0x04 */
} ChessSwitchState;

typedef struct ChessSwitchPdata {
    char pad00[8];
    ChessSwitchState* state; /* +0x08 */
} ChessSwitchPdata;

typedef struct ChessPieceEventResult {
    int values[2];
} ChessPieceEventResult;

typedef struct ChessAttackGridPosition {
    unsigned char pad00[2];
    unsigned char cell_x;
    unsigned char cell_y;
} ChessAttackGridPosition;

typedef struct ChessSquareMoveTarget {
    char pad00[0x29];
    unsigned char cell_x;
    unsigned char cell_y;
} ChessSquareMoveTarget;

typedef struct ChessHudState {
    char pad00[0x0C];
    int state;
} ChessHudState;

typedef struct ChessSpellHudPdata {
    MkHdr hdr;
    char pad08[8];
    ChessHudState* selected_spell;
    ChessHudState* target_1;
} ChessSpellHudPdata;

typedef struct ChessPieceProcPdata {
    MkHdr hdr;
    ChessPiece* piece;
} ChessPieceProcPdata;

typedef struct ChessInputPdata {
    MkHdr hdr;
    unsigned int side;
} ChessInputPdata;

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
    ScreenObj* text;
    unsigned int text_instance;
    int timer;
    int interval;
} ChessBlinkStringPdata;

typedef struct ChessBezierCameraState {
    char pad00[0x38];
    float progress;
} ChessBezierCameraState;

typedef struct ChessTableHeader {
    char pad00[8];
    unsigned int init_function;
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
    void (*completed)(ScreenObj*);
    int step;
    int target_x;
    unsigned int initial_delay;
} ChessSlideMessagePdata;

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
    ChessPiece* piece; /* +0x08 */
    char pad0C[0x40];
    unsigned int power; /* +0x4C */
    unsigned int field_50;
    char pad54[4];
    MkProc* proc; /* +0x58 */
    char pad5C[0x0C];
    MkObj* cursor; /* +0x68 */
    char pad6C[0x98];
    int think_delay; /* +0x104 */
    int action_delay; /* +0x108 */
    int cooldown; /* +0x10C */
    char pad110[0x1C];
    int strategy; /* +0x12C */
    char pad130[0x18];
    unsigned int spell; /* +0x148 */
    unsigned int spell_target; /* +0x14C */
    int state; /* +0x150 */
    unsigned int target_0_x; /* +0x154 */
    unsigned int target_1_x; /* +0x158 */
    unsigned int target_0_y; /* +0x15C */
    unsigned int target_1_y; /* +0x160 */
    unsigned int action_state; /* +0x164 */
    char pad168[0x0C];
    unsigned int desired_x; /* +0x174 */
    unsigned int desired_y; /* +0x178 */
} ChessDroneState;

typedef struct ChessGameDefinition {
    int background;
    int side_0_type;
    int field_08;
    int side_0_characters[5];
    int side_1_type;
    int field_24;
    int side_1_characters[5];
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
extern ChessSwitchPdata* switch_pdata;
extern CameraObj* camera_obj;
extern ChessBezierCameraState g_bezier_cam;

int fx_by_owner(int effect_type, MkObj* owner);
void fx_set_param_v3(int parameter, float x, float y, float z);
void fx_resume_emit(unsigned int effect);
void fx_reset_emit(unsigned int effect);
int emitter_id_from_handle(unsigned int handle);
MkPfx* pfx_from_handle(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
void mk_chess_cursor_go_to_new_track(
    ChessSquareMoveTarget* cursor, int track, unsigned int direction);
extern void* mk_chess_piece_ground_colls;
extern void* mk_chess_piece_bones;

void* memset(void* destination, int value, unsigned long size);
double __fabs(double value);
double sqrt(double value);
void mk_chess_remove_piece_at_cell_into_deadpool(unsigned int x,
                                                 unsigned int y);
void mk_chess_piece_event(ChessPiece* piece, unsigned int event,
                          ChessPieceEventResult* result);
void mk_chess_game_event(int event, ChessGameEventData* data,
                         int priority, int flags);
void mk_chess_xfer_to_piece_script(
    ChessPiece* piece, unsigned int script);
ScreenObj* mk_chess_create_portrait_from_library(
    int library_id, unsigned int flags);
void mk_chess_move_piece_from_deadpool_to(unsigned int side,
                                          unsigned int deadpool_index,
                                          unsigned int x, unsigned int y);
void mk_chess_request_piece_fight(ChessPiece* piece, unsigned int x,
                                  unsigned int y, int forced);
void mk_chess_place_special_cell_at(unsigned int x, unsigned int y, int type,
                                    int restored, void* saved_cell, float px,
                                    float py, float pz, float scale);
static void mk_chess_spell_hud_show_my_spells(void);
static int mk_chess_place_spell_hud_cursor_at_open_slot(
    ScreenObj* cursor, int side, int slot);
void mk_chess_hud_set_piece_portrait(ChessPiece* piece);
void mk_chess_remove_piece_from_team(ChessPiece* piece, int keep_active);
void mk_chess_activate_piece_properties(ChessPiece* piece);
void fx_set(unsigned int effect, int parameter, float value);
void fx_pause_emit(unsigned int effect);
void advance_anim(AniData* animation);
void pose_anim(AniData* animation, int update_object);
void set_anim_script_frame(AniData* animation, AniScript* script, int flags,
                           float frame);
void transition_to_anim_script_frame(AniData* animation, AniScript* script,
                                     int flags, float blend, float frame);
void set_root_and_obj_movement_weights(AniData* animation, float root_weight,
                                       float object_weight);
int ck_eat_online_switches(void);
int is_plyr_controller_enabled(PlyrInfo* player);
void mk_chess_set_game_mode(int mode);
void mk_chess_timeout_msg(int message, unsigned int side);
void mk_chess_set_viewing_quadrant(CameraObj* camera);
MslSoundHandle snd_req(int sound_id);
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
extern void mk_chess_set_default_chess_demo_game(int side);
void ck_do_profile_save(void);
void fade_to_black(int ticks, int sleep);
void del_string_obj_by_id(int id);
float bgnd_pebble_fetch_current_info(int field);
void bgnd_pebble_set_current_info(int field, float value);
void bgnd_pebble_set_current_pebble(int group, int pebble);
void mk_chess_camera_init(void);
void look_at_target(Vec* target, float distance);
void update_mkobj(void* object);
void bgnd_make_mkobj_transl(MkObj* object);
void obj_create_sobjs(MkObj* object);
void obj_set_z_offsets(
    MkObj* object, MkObj* owner, int enabled, int flags, float offset);
void build_bones_tbl(
    MkObj* object, void* table, unsigned int side, unsigned int flags,
    MkObj* owner, int enabled, float angle, float scale);
void insert_ground_me_mkobj(MkObj* object);
MkObj* load_model_from_slot(int slot, unsigned int model, int heap);
void pbar_force_pb_setting_with_offset(unsigned int player, float offset);
float p_mk_chess_cam_control(void);
float p_mk_chess_cam_chase_cursor(void);
float p_monitor_chess_input(void);
float x_chess_4(void);
float x_chess_3(void);
float x_chess_2(void);
float x_chess_1(void);
float x_chess_r2(void);
float x_chess_l1(void);
float x_chess_down(void);
float x_chess_right(void);
static void mk_chess_drone_cursor_movement_to_target(
    ChessDroneState* drone, void* cursor_position,
    unsigned int target_x, unsigned int target_y,
    unsigned int arrived_state, unsigned int arrived_delay,
    unsigned int moving_state);
static void mk_chess_remove_piece_from_board(ChessPiece* piece);
static int mk_chess_check_input_from_correct_side_no_ai(void);
static int mk_chess_drone_handle_the_big_chill_opening_move(
    ChessDroneState* drone);
static int mk_chess_drone_attempt_to_cast_spell_with_random_targets(
    ChessDroneState* drone, unsigned int target, unsigned int spell);
static int mk_chess_drone_check_spell(
    ChessDroneState* drone, unsigned int target, unsigned int spell);
static void mk_chess_drone_setup_spell_hud_pdata_for(
    ChessDroneState* drone, void* hud, unsigned int target,
    unsigned int spell);
static int mk_chess_drone_validate_target(
    ChessDroneState* drone, void* hud, unsigned int x, unsigned int y,
    unsigned int target);
static int mk_chess_drone_attempt_to_cast_spell(
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
    unsigned int side, int slot, int class_slot, unsigned int x,
    unsigned int y, int character, int active, int team, float health);
static MkObj* mk_chess_create_piece_model_from_library(int library);
unsigned int randu0(unsigned int max);
extern float inverse_game_speed;
extern const int available_chess_chars[26];
int is_char_locked(int character, int player);

static short g_starting_cells[16] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800,
    0x101, 0x201, 0x301, 0x401, 0x501, 0x601, 0x701, 0x801,
};

static void mk_chess_drone_complete_action(ChessDroneState* drone) {
    char* pdata;
    unsigned int side;

    pdata = mk_chess_pdata != 0 ? (char*)mk_chess_pdata + 0x44 : 0;

    if (drone->cooldown != 0) {
        drone->cooldown--;
        if (drone->cooldown != 0) {
            return;
        }
    }

    if ((g_game_info.pause_flags & 2) != 0) {
        return;
    }

    drone->action_delay--;
    if (drone->action_delay == 0) {
        if ((drone->cursor->flags_08 & 0x20) != 0) {
            drone->action_state = 0;
            return;
        }

        drone->cursor->flags_08 |= 0x40;
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_4);
        return;
    }

    switch (drone->action_state) {
    case 0:
        return;
    case 1:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, (char*)mk_chess_pdata + side * 0xC + 0x14,
            drone->target_0_x, drone->target_0_y, 7, 10, 2);
        return;
    case 2:
        xfer_proc(drone->proc, x_chess_3);
        drone->action_state = 3;
        drone->cooldown = 60;
        return;
    case 3:
        mk_chess_drone_cursor_movement_to_target(
            drone, (char*)mk_chess_pdata + 8,
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
        if (pdata != 0) {
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
    case 8:
        mk_chess_drone_cursor_movement_to_target(
            drone, (char*)mk_chess_pdata + 0x2C,
            drone->target_0_x, drone->target_0_y, 5, 10, 9);
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
            drone, (char*)mk_chess_pdata + 0x2C,
            drone->target_1_x, drone->target_1_y, 5, 10, 4);
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
    case 12:
        xfer_proc(drone->proc, x_chess_3);
        drone->action_state = 10;
        drone->cooldown = 120;
        drone->think_delay = 800;
        return;
    case 13:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, (char*)mk_chess_pdata + side * 0xC + 0x10,
            drone->target_0_x, drone->target_0_y, 3, 7, 14);
        return;
    case 14:
        drone->action_state = 0;
        xfer_proc(drone->proc, x_chess_1);
        return;
    case 15:
        side = drone->piece->side;
        mk_chess_drone_cursor_movement_to_target(
            drone, (char*)mk_chess_pdata + side * 0xC + 0x10,
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
    }
}

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
        mk_chess_set_default_chess_demo_game(1);
    }

    vtable = (ChessProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_mk_chess_continue, 0.0f);
}

float mk_chess_fight_cam_target_reached(void) {
    return 0.0f;
}

float mk_chess_piece_proc_force_dead(void) {
    return -1.0f;
}

void mk_chess_request_defender_won(
    unsigned int player,
    unsigned char cell_x,
    unsigned char cell_y) {
    ChessGameEventData event;

    mk_chess_set_game_mode(2);
    event.piece = mk_chess_pdata->board[cell_x].cells[cell_y].piece;
    event.player = player;
    mk_chess_game_event(3, &event, 2, 0);
    mk_chess_remove_piece_at_cell_into_deadpool(cell_x, cell_y);
}

void mk_chess_piece_type_to_piece_script(
    unsigned int side_index,
    int piece_type,
    unsigned int occurrence,
    unsigned int script) {
    ChessPiece* piece;
    ChessSideState* side;
    unsigned int index;

    side = mk_chess_pdata->sides[side_index];
    piece = 0;
    for (index = 0; index < side->deadpool_count; index++) {
        if (side->deadpool[index]->type == piece_type) {
            if (occurrence == 0) {
                piece = side->deadpool[index];
                break;
            }
            occurrence--;
        }
    }

    if (piece != 0 && piece->state == 0) {
        mk_chess_xfer_to_piece_script(piece, script);
    }
}

void mk_chess_register_name_and_portrait_in_team(
    unsigned int side_index,
    unsigned int portrait_index,
    int library_id) {
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
            mk_chess_create_portrait_from_library(library_id, flags);
        side->portraits[portrait_index].screen = portrait;
        side->portraits[portrait_index].instance =
            portrait->instance;
        hide_screen_obj(portrait);
    }
}

/* Soft ceiling: mk_chess_fake_demo_cam ~99.09% - constant-pool label only. */
int mk_chess_fake_demo_cam(float* camera_speed) {
    if (camera_obj->pos_z > 20.3f) {
        return 0;
    }
    *camera_speed = 0.1f;
    return 1;
}

int mk_chess_should_i_fall_down(void) {
    return 0;
}

static void mk_chess_drone_cast_random_spell(ChessDroneState* drone) {
    unsigned int target;

    target = randu0(2) & 0xFFFF;
    mk_chess_drone_attempt_to_cast_spell_with_random_targets(
        drone, target, randu0(4) & 0xFFFF);
}

static int mk_chess_drone_opening_move_based_on_strategy(
    ChessDroneState* drone) {
    if (drone->strategy == 5 &&
        mk_chess_drone_handle_the_big_chill_opening_move(drone) != 0) {
        return 1;
    }
    return 0;
}

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

static void mk_chess_check_leader_vs_leader_condition(void) {
    ChessScaleMessagePdata* pdata;
    MkHdr* raw_pdata;
    ChessPiece* attacker;
    ChessPiece* defender;
    ScreenObj* message;
    int center_x;
    int center_y;

    if (mk_chess_pdata->sides[0]->deadpool_count != 1 ||
        mk_chess_pdata->sides[1]->deadpool_count != 1) {
        return;
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
    attacker = mk_chess_pdata->sides[0]->deadpool[0];
    defender = mk_chess_pdata->sides[1]->deadpool[0];
    mk_chess_hud_set_piece_portrait(attacker);
    mk_chess_hud_set_piece_portrait(defender);
    mk_chess_request_piece_fight(
        attacker, defender->cell_x, defender->cell_y, 1);
}

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

static void mk_chess_choose_middle_control_points_for_fight_cam(
    const Vec* fighter_0_facing, const Vec* fighter_1_facing,
    const Vec* fighter_0_position, const Vec* fighter_1_position,
    unsigned int camera_side, unsigned int camera_type,
    const Vec* camera_start, Vec* middle_0, Vec* middle_1,
    const Vec* camera_end) {
    Vec between;
    Vec facing_0;
    Vec facing_1;
    float length;
    float camera_distance;
    float side_sign;
    float height_adjust;

    between.x = fighter_0_position->x - fighter_1_position->x;
    between.y = fighter_0_position->y - fighter_1_position->y;
    between.z = fighter_0_position->z - fighter_1_position->z;
    camera_distance =
        (float)sqrt(
            (double)((camera_start->x - camera_end->x) *
                         (camera_start->x - camera_end->x) +
                     (camera_start->y - camera_end->y) *
                         (camera_start->y - camera_end->y) +
                     (camera_start->z - camera_end->z) *
                         (camera_start->z - camera_end->z)));

    length = (float)sqrt(
        (double)(fighter_0_facing->x * fighter_0_facing->x +
                 fighter_0_facing->z * fighter_0_facing->z));
    if (length != 0.0f) {
        facing_0.x = fighter_0_facing->x / length;
        facing_0.z = fighter_0_facing->z / length;
    } else {
        facing_0.x = 0.0f;
        facing_0.z = 0.0f;
    }

    length = (float)sqrt(
        (double)(fighter_1_facing->x * fighter_1_facing->x +
                 fighter_1_facing->z * fighter_1_facing->z));
    if (length != 0.0f) {
        facing_1.x = fighter_1_facing->x / length;
        facing_1.z = fighter_1_facing->z / length;
    } else {
        facing_1.x = 0.0f;
        facing_1.z = 0.0f;
    }

    if (camera_side == 1) {
        if (camera_type == 5) {
            side_sign =
                facing_1.x * fighter_0_position->x +
                            facing_1.z * fighter_0_position->z <
                        0.0f
                    ? -1.0f
                    : 1.0f;
            middle_0->x = fighter_1_position->x + between.x * 0.5f +
                          facing_0.x * 35.0f +
                          facing_1.x * (45.0f * side_sign);
            middle_0->y =
                fighter_1_position->y + between.y * 0.5f;
            middle_0->z = fighter_1_position->z + between.z * 0.5f +
                          facing_0.z * 35.0f +
                          facing_1.z * (45.0f * side_sign);
            *middle_1 = *camera_end;
            middle_1->y = middle_0->y * 0.5f;
            middle_1->x += facing_0.x * 50.0f +
                           facing_1.x * (10.0f * side_sign);
            middle_1->z += facing_0.z * 50.0f +
                           facing_1.z * (10.0f * side_sign);
        } else {
            middle_0->x =
                fighter_1_position->x + between.x * 0.75f;
            middle_0->y =
                (fighter_1_position->y + between.y * 0.75f) * 0.65f;
            middle_0->z =
                fighter_1_position->z + between.z * 0.75f;
            middle_0->x += facing_0.x * (62.0f * camera_distance / 60.0f);
            middle_0->z += facing_0.z * (62.0f * camera_distance / 60.0f);
            middle_1->x =
                fighter_1_position->x + between.x * 0.1f;
            middle_1->y =
                fighter_1_position->y + between.y * 0.1f;
            middle_1->z =
                fighter_1_position->z + between.z * 0.1f;
            middle_1->x += facing_0.x * (34.0f * camera_distance / 60.0f);
            middle_1->z += facing_0.z * (34.0f * camera_distance / 60.0f);
        }
    } else if (camera_side == 0) {
        if (camera_type == 3) {
            middle_0->x =
                fighter_1_position->x + between.x * 0.85f;
            middle_0->y =
                fighter_1_position->y + between.y * 0.85f + 20.0f;
            middle_0->z =
                fighter_1_position->z + between.z * 0.85f;
            middle_1->x =
                fighter_1_position->x + between.x * 0.7f;
            middle_1->y = camera_end->y + 1.5f;
            middle_1->z =
                fighter_1_position->z + between.z * 0.7f;
        } else if (camera_type == 4) {
            middle_0->x =
                fighter_1_position->x + between.x * 0.9f;
            middle_0->y = camera_end->y + 1.5f;
            middle_0->z =
                fighter_1_position->z + between.z * 0.9f;
            middle_1->x =
                fighter_1_position->x + between.x * 0.7f;
            middle_1->y = camera_end->y;
            middle_1->z =
                fighter_1_position->z + between.z * 0.7f;
        } else {
            height_adjust = fighter_0_position->y > 40.0f ? 10.0f : 0.0f;
            middle_0->x =
                fighter_1_position->x + between.x * 0.85f +
                facing_0.x * (35.0f + height_adjust);
            middle_0->y =
                fighter_1_position->y + between.y * 0.85f;
            middle_0->z =
                fighter_1_position->z + between.z * 0.85f +
                facing_0.z * (35.0f + height_adjust);
            middle_1->x =
                fighter_1_position->x + between.x * 0.7f +
                facing_0.x * (22.0f + height_adjust);
            middle_1->y =
                fighter_1_position->y + between.y * 0.7f;
            middle_1->z =
                fighter_1_position->z + between.z * 0.7f +
                facing_0.z * (22.0f + height_adjust);
        }
    } else if (camera_type == 5) {
        side_sign =
            facing_1.x * fighter_0_position->x +
                        facing_1.z * fighter_0_position->z <
                    0.0f
                ? -1.0f
                : 1.0f;
        *middle_0 = *camera_start;
        middle_0->y *= 0.9f;
        middle_0->x += -35.0f * facing_0.x +
                       90.0f * side_sign * facing_1.x;
        middle_0->z += -35.0f * facing_0.z +
                       90.0f * side_sign * facing_1.z;
        *middle_1 = *camera_end;
        middle_1->y = middle_0->y * 0.5f;
        middle_1->x += 50.0f * facing_0.x +
                       25.0f * side_sign * facing_1.x;
        middle_1->z += 50.0f * facing_0.z +
                       25.0f * side_sign * facing_1.z;
    } else {
        middle_0->x =
            fighter_1_position->x + between.x * 0.5f + 60.0f * facing_1.x;
        middle_0->y =
            fighter_1_position->y + between.y * 0.5f;
        middle_0->z =
            fighter_1_position->z + between.z * 0.5f + 60.0f * facing_1.z;
        middle_1->x =
            fighter_1_position->x - between.x + 30.0f * facing_1.x;
        middle_1->y = middle_0->y * 0.5f;
        middle_1->z =
            fighter_1_position->z - between.z + 30.0f * facing_1.z;
    }
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
        roll = randu0(100);
        if (roll < 25) {
            drone->strategy = 3;
        } else if (roll < 50) {
            drone->strategy = 4;
        } else {
            drone->strategy = 1;
        }
    } else if (owned_cells == 1) {
        roll = randu0(100);
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

    if (drone->power < 7 && randu0(100) < 30) {
        drone->strategy = 1;
    }
}

void mk_chess_handle_turn_timeout_scenerios(unsigned int side) {
    unsigned int timeout;
    int message;

    timeout = mk_chess_pdata->turn_timeout;
    if (timeout == 0) {
        return;
    }
    timeout--;
    mk_chess_pdata->turn_timeout = timeout;
    if (timeout == 0) {
        if (mk_chess_pdata->manager.input_state == 9 &&
            mk_chess_pdata->spell != 0 &&
            mk_chess_pdata->spell->state >= 0x13 &&
            mk_chess_pdata->spell->state < 0x18) {
            mk_chess_pdata->turn_timeout = 1;
            return;
        }
        if (side < 2) {
            mk_chess_timeout_msg(5, side);
        }
        return;
    }

    message = -1;
    if (timeout < 620) {
        if (timeout == (unsigned int)(600.0f * inverse_game_speed)) {
            message = 0;
        } else if (
            timeout == (unsigned int)(480.0f * inverse_game_speed)) {
            message = 1;
        } else if (
            timeout == (unsigned int)(360.0f * inverse_game_speed)) {
            message = 2;
        } else if (
            timeout == (unsigned int)(240.0f * inverse_game_speed)) {
            message = 3;
        } else if (
            timeout == (unsigned int)(120.0f * inverse_game_speed)) {
            message = 4;
        }
    }
    if (message >= 0) {
        mk_chess_timeout_msg(message, side);
    }
}

static int mk_chess_fetch_desired_attack_script(
    ChessPiece* piece,
    ChessAttackGridPosition* attack,
    unsigned int* distance) {
    int delta_x;
    int delta_y;
    unsigned int direction;
    unsigned int abs_x;
    unsigned int abs_y;

    delta_x = (int)attack->cell_x - (int)attack->pad00[0];
    delta_y = (int)attack->cell_y - (int)attack->pad00[1];
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

static int mk_chess_drone_validate_spell(
    ChessDroneState* drone,
    unsigned int target,
    unsigned int spell,
    unsigned int target_0_x,
    unsigned int target_0_y,
    unsigned int target_1_x,
    unsigned int target_1_y) {
    unsigned char hud[0x64];

    if (mk_chess_drone_check_spell(drone, target, spell) != 1) {
        return 0;
    }
    drone->state = 0x10;
    drone->spell = spell;
    drone->spell_target = target;
    mk_chess_drone_setup_spell_hud_pdata_for(
        drone, hud, target, spell);
    drone->target_0_x = target_0_x;
    drone->target_0_y = target_0_y;
    drone->target_1_x = target_1_x;
    drone->target_1_y = target_1_y;
    if (mk_chess_drone_validate_target(
            drone, hud, target_0_x, target_0_y, 0) == 0) {
        return 0;
    }
    return mk_chess_drone_validate_target(
               drone, hud, target_1_x, target_1_y, 1) == 1;
}

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

static void mk_chess_deal_with_minor_threat_to(
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
                return;
            }
        } else if (roll < 35) {
            if (mk_chess_drone_attempt_to_cast_spell(
                    drone, 1, 0, attacker->cell_x, attacker->cell_y,
                    0xFF, 0xFF)) {
                return;
            }
        } else if (roll < 70) {
            if (threatened != 0 &&
                mk_chess_drone_move_piece_out_of_danger(side, threatened)) {
                return;
            }
        } else if (roll < 80) {
            if (mk_chess_drone_move_best_matchup_against_piece(
                    side, attacker, 0)) {
                return;
            }
        } else if (roll < 90 && threatened != 0 &&
                   mk_chess_drone_help_piece_by_blocking(
                       side, threatened, attacker)) {
            return;
        }
    }
}

static void mk_chess_spell_hud_target_1_text_faded_goto_next_target_cb(void) {
    ChessSpellHudPdata* hud;

    hud = (ChessSpellHudPdata*)apdata;
    mk_chess_spell_hud_show_page(hud->target_1, 1);
    hud->target_1->state = 0x12;
}

static inline void mk_chess_start_active_piece_script(void) {
    g_active_piece->flags.snap_into_stance = 1;
    cmdscript_setup_execution(
        g_board_game_controller.command_script,
        (unsigned int)g_active_piece->normal_stance_script);
    cmdscript_execute(g_board_game_controller.command_script);
    g_active_piece->flags.unknown_bit4 = 1;
}

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
    MkObj* object;

    object = g_active_piece->object;
    object->pos_vel.y = vertical_speed;
    object->gravity = gravity;
    object->flags_08 |= 1;
    object->flags_09 |= 0x80;
    object->flags_08 |= 0x20;
    object->flags_09 &= ~0x40;
}

void mk_chess_stop_me(void) {
    MkObj* object;

    object = g_active_piece->object;
    object->flags_08 &= ~1;
    object->flags_09 |= 0x80;
    object->flags_08 &= ~0x20;
    object->pos_vel.x = 0.0f;
    object->pos_vel.y = 0.0f;
    object->pos_vel.z = 0.0f;
    object->flags_09 |= 0x40;
}

void mk_chess_ani_idle(void) {
    mk_chess_stop_me();
    g_active_piece->flags.unknown_bit4 = 0;
}

void mk_chess_ani_loop_more_frames(float frames) {
    AniData* animation;

    animation = g_active_piece->animation;
    while (frames > 0.0f) {
        advance_anim(animation);
        pose_anim(animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        frames -= 1.0f;
    }
}

void mk_chess_ani_to_frame_x(float target) {
    AniData* animation;

    animation = g_active_piece->animation;
    while (animation->frame + animation->speed * game_speed <= target) {
        advance_anim(animation);
        pose_anim(animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }
}

void mk_chess_ani_to_end(void) {
    mk_chess_ani_to_frame_x(g_active_piece->animation->end_frame);
}

void mk_chess_ani_to_blend_frame(float blend_frames) {
    mk_chess_ani_to_frame_x(
        g_active_piece->animation->end_frame - blend_frames);
}

void mk_chess_random_specials_positive_reaction(void) {
    unsigned int occurrence;
    unsigned int active_side;
    unsigned int other_side;

    active_side = g_active_piece->side;
    other_side = active_side == 0;
    for (occurrence = 0; occurrence < 2; occurrence++) {
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                other_side, 1, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                other_side, 3, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                other_side, 4, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                other_side, 2, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                active_side, 1, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                active_side, 3, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                active_side, 4, occurrence, 0x1D);
        }
        if ((randu0(100) & 0xFFFF) < 50) {
            mk_chess_piece_type_to_piece_script(
                active_side, 2, occurrence, 0x1D);
        }
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

static float p_mk_chess_piece_proc_entry(void) {
    ChessProcVtable* vtable;

    g_active_piece->state = 0;
    g_active_piece->flags.snap_into_stance = 0;
    g_active_piece->flags.glitch_into_stance = 0;
    g_active_piece->object->flags_08 &= ~0x20;
    g_active_piece->movement->desired_cell_blend = 30.0f;
    vtable = (ChessProcVtable*)aproc->vtbl;
    if (g_active_piece->flags.dont_constrain) {
        g_active_piece->flags.dont_constrain = 0;
        vtable->jump_sleep(p_mk_chess_piece_proc, 0.0f);
    } else {
        vtable->jump_sleep(p_mk_chess_piece_constrain_to_cell, 0.0f);
    }
    return 0.0f;
}

static inline float mk_chess_direction_input(
    unsigned int direction,
    int action,
    int spell_input,
    int diagonal) {
    ChessInputPdata* input;
    unsigned int side;

    if (mk_chess_pdata == 0) {
        return -1.0f;
    }
    if (mk_chess_pdata->input_transition_busy != 0) {
        ((ChessProcVtable*)aproc->vtbl)
            ->jump_sleep(p_monitor_chess_input, 0.0f);
        return 0.0f;
    }

    input = (ChessInputPdata*)apdata;
    side = mk_chess_pdata->manager.active_side;
    switch (mk_chess_pdata->manager.input_state) {
    case 10:
        mk_chess_pdata->directional_actions[input->side + 10] = action;
        break;
    case 9:
        mk_chess_pdata->spell->input_state = spell_input;
        break;
    case 0:
        if (diagonal) {
            mk_chess_move_cursor_to_next_diagnal_piece(side, direction);
        } else {
            mk_chess_move_cursor_to_next_piece(side, direction);
        }
        break;
    case 1:
        mk_chess_move_cursor_to_next_square_track_line(side, direction);
        break;
    }
    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

float x_chess_down(void) {
    return mk_chess_direction_input(0, 5, 2, 0);
}

float x_chess_up(void) {
    return mk_chess_direction_input(4, 4, 1, 0);
}

float x_chess_right(void) {
    return mk_chess_direction_input(2, 7, 4, 0);
}

float x_chess_left(void) {
    return mk_chess_direction_input(6, 6, 3, 0);
}

float x_chess_right_and_down(void) {
    return mk_chess_direction_input(1, 9, 6, 1);
}

float x_chess_right_and_up(void) {
    return mk_chess_direction_input(3, 8, 5, 1);
}

float x_chess_left_and_up(void) {
    return mk_chess_direction_input(5, 10, 7, 1);
}

float x_chess_left_and_down(void) {
    return mk_chess_direction_input(7, 11, 8, 1);
}

void p_board_monitor(void) {
    unsigned int x;
    unsigned int y;

    if (mk_chess_pdata->manager.input_state == 0xB) {
        for (x = 0; x < MK_CHESS_BOARD_COLUMNS; x++) {
            for (y = 0; y < MK_CHESS_BOARD_COLUMNS; y++) {
                mk_chess_cell_monitor(x, y);
            }
        }
    }
}

void p_mk_chess_attack_burst(void) {
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
}

void p_mk_chess_show_my_side(void) {
    ChessShowSidePdata* display;
    unsigned int pebble;

    display = (ChessShowSidePdata*)apdata;
    display->current += display->amount;
    if (display->current > display->maximum) {
        display->current = display->maximum;
    }
    display->amount += display->acceleration;
    for (pebble = 0; pebble < display->side->deadpool_count; pebble++) {
        bgnd_pebble_set_current_pebble(display->pebble_group, pebble);
        bgnd_pebble_set_current_info(0xE, display->current);
        bgnd_pebble_set_current_info(0xC, display->current);
    }
}

void p_mk_chess_blink_string(void) {
    ChessBlinkStringPdata* blink;
    ScreenObj* text;

    blink = (ChessBlinkStringPdata*)apdata;
    text = blink->text;
    if (text != 0 && text->instance != blink->text_instance) {
        text = 0;
    }
    blink->timer--;
    if (blink->timer == 0) {
        text->flags &= ~0x80;
    } else if (blink->timer == -blink->interval) {
        blink->timer = blink->interval;
        text->flags |= 0x80;
    }
}

void mk_chess_wait_until_attack_cam_closes_in(void) {
    int timeout;

    timeout = 0x4AF;
    do {
        advance_anim(g_active_piece->animation);
        pose_anim(g_active_piece->animation, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
        timeout--;
    } while (g_bezier_cam.progress <= 0.8f && timeout > 0);
}

void mk_chess_init_piece(int slot, int character) {
    unsigned int x;
    unsigned int y;

    x = ((unsigned short)g_starting_cells[slot] >> 8) & 0xFF;
    y = (unsigned short)g_starting_cells[slot] & 0xFF;
    mk_chess_create_piece(
        g_active_team, slot, slot, x,
        y + g_active_team * (10 - (y * 2 + 1)),
        character, 1, g_active_team, 1.0f);
    mk_chess_pdata->sides[g_active_team]->deadpool_count++;
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

void mk_chess_camera_init_for_place_traps(void) {
    CameraObj* camera;
    Vec target;

    mk_chess_camera_init();
    camera = camera_item.node;
    if (camera != 0 && camera->instance != camera_item.instance) {
        camera = 0;
    }
    camera->pos_x = -38.45f;
    camera->pos_y = 24.8f;
    camera->pos_z = 0.0f;
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 0.0f;
    look_at_target(&target, 24.8f);
    update_mkobj((MkHdr*)camera);
    mk_chess_pdata->camera.viewing_quadrant = 3;
}

void mk_chess_create_piece_obj(
    ChessPiece* piece,
    unsigned int side,
    int library) {
    MkObj* object;
    unsigned int build_flags;

    object = mk_chess_create_piece_model_from_library(library);
    piece->object = object;
    object->light_flags = 0x200;
    object->flags_09 |= 0x80;
    object->flags_09 |= 0x40;
    object->ground_colls = &mk_chess_piece_ground_colls;
    obj_set_z_offsets(object, object, 1, 0x200, 25.0f);
    obj_create_sobjs(object);
    bgnd_make_mkobj_transl(object);
    build_flags = object->flags_08 | 2;
    object->flags_08 = build_flags;
    object->scale.x = 2.2f;
    object->scale.y = 2.2f;
    object->scale.z = 2.2f;
    object->flags_08 |= 0x40;
    object->ang.y = 3.1415927f * side;
    build_bones_tbl(
        object, &mk_chess_piece_bones, side, build_flags, object,
        1, 3.1415927f, 2.2f);
    insert_fgnd_mkobj(object);
    insert_ground_me_mkobj(object);
}

void p_mk_chess_apply_force(void) {
    ChessForcePdata* force;
    MkObj* object;
    int frame;

    force = (ChessForcePdata*)apdata;
    object = force->object;
    if (object != 0 && object->hdr.instance != force->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return;
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

    index = 0;
    do {
        if (g_board_game_controller.battle_piece_rows[index].character_number ==
            character_number) {
            return index;
        }
        index++;
    } while (index < 26);
    return 0;
}

int mk_chess_fetch_active_defined_teams_class(int class_slot) {
    ChessTeamDefinition* team = &g_chess_definition_info[g_active_team];

    if (class_slot == 2) {
        return team->class_slot_2;
    }
    if (class_slot < 2) {
        if (class_slot > 0) {
            return team->class_slot_1;
        }
    } else {
        if (class_slot == 5) {
            return team->class_slot_5;
        }
        if (class_slot < 5) {
            return team->class_slot_3_4;
        }
    }
    return team[1].next_record_fallback_class;
}

int mk_chess_fetch_active_defined_team(void) {
    return g_active_team;
}

void mk_chess_make_spellcaster(int spellcaster_type) {
    ChessClassDefinition* definition = g_active_class_definition;

    if (definition == 0) {
        return;
    }
    definition->flags.spellcaster = 1;
    definition->spellcaster_type = spellcaster_type;
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
    ChessClassDefinition* definition = g_active_class_definition;
    ChessMovementSkill* skill;

    if (definition == 0) {
        return;
    }
    if (definition->movement_skill_count >= MK_CHESS_MOVEMENT_SKILL_COUNT) {
        return;
    }

    skill = &definition->movement_skills[definition->movement_skill_count];
    skill->move_type = move_type;
    skill->limits = (limit_a << 16) | limit_b;
    skill->flags = flags;
    definition->movement_skill_count++;
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

/*
 * Soft ceiling: mk_chess_spell_hud_target_1_text_faded_exit_cb ~99.17% -
 * constant-pool label only.
 */
float mk_chess_spell_hud_target_1_text_faded_exit_cb(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->target_1->state = 0x13;
    return 0.0f;
}

/*
 * Soft ceiling: p_mk_chess_names_retracted_move_up_selected_spell ~99.17% -
 * constant-pool label only.
 */
float p_mk_chess_names_retracted_move_up_selected_spell(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->selected_spell->state = 9;
    return 0.0f;
}

void mk_chess_spell_hud_spell_text_faded_exit_cb(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->selected_spell->state = 6;
    snd_req(0x396);
}

void p_mk_chess_completed_all_image_retraction_for_targetting(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->selected_spell->state = 0xF;
    snd_req(0x396);
}

void p_mk_chess_completed_all_image_retraction(void) {
    ChessSpellHudPdata* pdata = (ChessSpellHudPdata*)apdata;

    pdata->selected_spell->state = 6;
    snd_req(0x396);
}

void mk_chess_attacker_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 0.1f);
}

void mk_chess_attacker_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 0.1f);
}

void mk_chess_pwr_cell_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 0.25f);
}

void mk_chess_pwr_cell_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 0.25f);
}

void mk_chess_on_pwr_cell_1_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(0, 1.0f);
}

void mk_chess_on_pwr_cell_2_bonus_slid_into_place_cb(void) {
    _mkproc_sleep_ticks = 5.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
    shake_camera(2, 0.01f);
    snd_req(0x3B5);
    pbar_force_pb_setting_with_offset(1, 1.0f);
}

void mk_chess_spell_has_completed_but_wait_for_fight(void) {
    mk_chess_pdata->spell->state = MK_CHESS_SPELL_WAITING_FOR_FIGHT;
}

void mk_chess_spell_has_completed(void) {
    mk_chess_pdata->spell_completion_clock = mk_chess_pdata->clock;
    if (mk_chess_pdata->spell->state !=
        MK_CHESS_SPELL_WAITING_FOR_COMPLETION) {
        return;
    }
    mk_chess_pdata->spell->state = MK_CHESS_SPELL_COMPLETE;
}

void mk_chess_spell_rescue_current_target(void) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    ChessSideState* side = mk_chess_pdata->sides[spell->side];
    unsigned int x = spell->target_x[1];
    unsigned int y = spell->target_y[1];
    unsigned int index;

    for (index = 17 - side->deadpool_count; index < 17; index++) {
        if (side->deadpool[index]->type == spell->rescue_piece_type) {
            mk_chess_move_piece_from_deadpool_to(spell->side, index, x, y);
            spell->target_x[0] = x;
            spell->target_y[0] = y;
            return;
        }
    }
}

void mk_chess_spell_target_add_access_restrictions(unsigned int target,
                                                   unsigned int restriction,
                                                   int duration, int reset) {
    ChessSpellState* spell = mk_chess_pdata->spell;
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

    if ((*expires > (unsigned int)mk_chess_pdata->clock) && (reset == 0)) {
        *expires += duration;
        return;
    }
    *expires = mk_chess_pdata->clock + duration;
}

void mk_chess_spell_force_fight(void) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    unsigned int x = (unsigned char)spell->target_x[0];
    unsigned int y = (unsigned char)spell->target_y[0];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    board_game_save_data.sides[spell->side].forced_fight_count++;
    mk_chess_request_piece_fight(
        piece, (unsigned char)spell->target_x[1],
        (unsigned char)spell->target_y[1], 1);
}

/* Soft ceiling: mk_chess_spell_is_this_a_forced_fight ~90.61% - typed board indexing. */
int mk_chess_spell_is_this_a_forced_fight(void) {
    ChessSpellState* spell = mk_chess_pdata->spell;
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

void mk_chess_spell_kill_target(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;

    mk_chess_remove_piece_at_cell_into_deadpool(
        (unsigned char)spell->target_x[target],
        (unsigned char)spell->target_y[target]);
}

void mk_chess_spell_move_target_from_temp_area_to(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    ChessPiece* piece = spell->temporary_piece;
    ChessCell* old_cell =
        &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];

    if (old_cell->piece == piece) {
        old_cell->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    piece->object->hide_flags &= ~0x02;
    piece->object->pos.x =
        cell->position.x + piece->runtime.fields.cell_offset.x;
    piece->object->pos.z =
        cell->position.z + piece->runtime.fields.cell_offset.z;
    update_obj_pos(piece->object);

    if (piece->side == mk_chess_pdata->manager.active_side) {
        mk_chess_pdata->manager.active_piece_by_side[piece->side] = piece;
    }
}

void mk_chess_spell_move_target_to_temp_area(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    mk_chess_remove_piece_from_board(piece);
    spell->temporary_piece = piece;
}

void mk_chess_spell_move_target_to_target(unsigned int source_target,
                                          unsigned int destination_target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    unsigned int source_x =
        (unsigned char)spell->target_x[source_target];
    unsigned int source_y =
        (unsigned char)spell->target_y[source_target];
    ChessPiece* piece =
        mk_chess_pdata->board[source_x].cells[source_y].piece;
    unsigned int x = (unsigned char)spell->target_x[destination_target];
    unsigned int y = (unsigned char)spell->target_y[destination_target];
    ChessCell* old_cell =
        &mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y];
    ChessCell* cell = &mk_chess_pdata->board[x].cells[y];

    mk_chess_remove_piece_from_board(piece);
    if (old_cell->piece == piece) {
        old_cell->piece = 0;
    }
    cell->piece = piece;
    piece->cell_x = x;
    piece->cell_y = y;
    piece->object->hide_flags &= ~0x02;
    piece->object->pos.x =
        cell->position.x + piece->runtime.fields.cell_offset.x;
    piece->object->pos.z =
        cell->position.z + piece->runtime.fields.cell_offset.z;
    update_obj_pos(piece->object);

    if (piece->side == mk_chess_pdata->manager.active_side) {
        mk_chess_pdata->manager.active_piece_by_side[piece->side] = piece;
    }
}

float mk_chess_spell_get_target_health(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];

    return mk_chess_pdata->board[x].cells[y].piece->health;
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

float mk_chess_spell_get_target_max_health(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
    unsigned int x = (unsigned char)spell->target_x[target];
    unsigned int y = (unsigned char)spell->target_y[target];
    ChessPiece* piece = mk_chess_pdata->board[x].cells[y].piece;

    return g_board_game_controller.class_definitions[piece->type].initial_power;
}

void mk_chess_spell_show_target_portrait(unsigned int target) {
    ChessSpellState* spell = mk_chess_pdata->spell;
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
    if (info != MK_CHESS_PIECE_INFO_Y_ANGLE) {
        return;
    }
    g_active_piece->object->ang.y = value;
}

float mk_chess_get_piece_info(int info) {
    if (info == MK_CHESS_PIECE_INFO_Z) {
        return g_active_piece->object->pos.z;
    }
    if (info < MK_CHESS_PIECE_INFO_Z) {
        if (info == MK_CHESS_PIECE_INFO_X) {
            return g_active_piece->object->pos.x;
        }
        if (info >= MK_CHESS_PIECE_INFO_X) {
            return g_active_piece->object->pos.y;
        }
    } else if (info < 4) {
        return g_active_piece->object->ang.y;
    }
    return 0.0f;
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

/*
 * Soft ceiling: p_mk_chess_piece_proc ~99.38% -
 * constant-pool label only.
 */
float p_mk_chess_piece_proc(void) {
    G_BOARD_GAME_BIGSTACK_COUNTER--;
    g_active_piece->proc_state = 0;
    return -1.0f;
}

void mk_chess_ani_1_frame(void) {
    AniData* animation = g_active_piece->animation;

    advance_anim(animation);
    pose_anim(animation, 1);
}

void mk_chess_piece_match_y_ang_to_anim(void) {
    g_active_piece->object->hide_flags |= 0x01;
}

void mk_chess_glitch_to_ani_frame(int animation, int flags, float speed,
                                  float frame) {
    AniData* anim = g_active_piece->animation;

    g_active_piece->flags.glitch_into_stance = 0;
    g_active_piece->object->hide_flags &= ~0x02;
    anim->speed = speed;
    set_anim_script_frame(anim, mkc_animations[animation], flags, frame);
}

void mk_chess_blend_to_ani_frame(int animation, int flags, float blend,
                                 float speed, float frame) {
    AniData* anim = g_active_piece->animation;

    g_active_piece->object->hide_flags &= ~0x02;
    anim->speed = speed;
    transition_to_anim_script_frame(
        anim, mkc_animations[animation], flags, blend, frame);
    _mkproc_sleep_ticks = 1.0f;
    ((ChessProcVtable*)aproc->vtbl)->sleep();
}

/* Soft ceiling: mk_chess_air_move ~77.14% - shared object flag bitfield emit. */
void mk_chess_air_move(void) {
    g_active_piece->object->flags_09 &= ~0x40;
}

void mk_chess_set_ani_speed(float speed) {
    g_active_piece->animation->speed = speed;
}

/* Soft ceiling: mk_chess_set_obj_move_weight ~99.58% - zero-pool label only. */
void mk_chess_set_obj_move_weight(float weight) {
    set_root_and_obj_movement_weights(
        g_active_piece->animation, 0.0f, weight);
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
    g_active_piece->runtime.fields.event_time = mk_chess_pdata->clock + delay;
}

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

/* Soft ceiling: mk_chess_set_cell_offset ~99.78% - constant-pool emit only. */
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

void mk_chess_snap_to_my_cell_now(void) {
    ChessCell* cell;

    g_active_piece->object->hide_flags &= ~0x02;
    cell = &mk_chess_pdata->board[g_active_piece->cell_x]
                .cells[g_active_piece->cell_y];
    g_active_piece->object->pos.x = cell->position.x;
    g_active_piece->object->pos.z = cell->position.z;
    update_obj_pos(g_active_piece->object);
}

/* Soft ceiling: mk_chess_deactivate_my_properties ~99.63% - 1.0f pool identity; stop. */
void mk_chess_deactivate_my_properties(void) {
    ChessPiece* piece = g_active_piece;

    if (piece == 0) {
        return;
    }

    switch (piece->type) {
    case 5:
        fx_set(piece->runtime.timer_slots[4], 0x204, 1.0f);
        return;
    case 3:
        fx_set(piece->runtime.timer_slots[4], 0x204, 1.0f);
        fx_set(piece->runtime.timer_slots[5], 0x204, 1.0f);
        fx_pause_emit(piece->runtime.timer_slots[4]);
        fx_pause_emit(piece->runtime.timer_slots[5]);
        return;
    case 4:
        fx_pause_emit(piece->runtime.timer_slots[4]);
        break;
    }
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

int mk_chess_active_piece_near_edge(void) {
    float x;
    float z;

    x = (float)__fabs(g_active_piece->object->pos.x);
    z = (float)__fabs(g_active_piece->object->pos.z);
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

static void mk_chess_remove_piece_from_board(ChessPiece* piece) {
    ChessPiece** active_piece =
        &mk_chess_pdata->manager.active_piece_by_side[piece->side];
    unsigned char x;
    unsigned char y;

    mk_chess_pdata->board[piece->cell_x].cells[piece->cell_y].piece = 0;

    if (*active_piece == 0 || (*active_piece)->id != piece->id) {
        return;
    }

    for (x = 0; x < MK_CHESS_BOARD_COLUMNS; x++) {
        for (y = 0; y < MK_CHESS_BOARD_COLUMNS; y++) {
            ChessPiece* candidate =
                mk_chess_pdata->board[x].cells[y].piece;

            if (candidate != 0 && candidate->side == piece->side) {
                *active_piece = candidate;
                return;
            }
        }
    }
    *active_piece = 0;
}

/* Soft ceiling: mk_chess_fetch_piece_at_cursor ~90.53% - typed 2D address order. */
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

/* Soft ceiling: mk_chess_allow_setting_of_viewing_quadrant ~91.43% - bit extract emit. */
int mk_chess_allow_setting_of_viewing_quadrant(void) {
    return (g_game_info.field_04 >> 7) == 0;
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

/* Soft ceiling: mk_chess_in_fight_setup ~94.25% - TU string/float pool identities. */
void mk_chess_in_fight_setup(void) {
    load_ssf((MkFileEntry*)mkchess_ingame_art_file_table);
    load_string_bank(0x20000, "boardgame_strings_eng.mko");
    g_game_info.plyr0.field_0C = 1.0f;
    g_game_info.plyr1.field_0C = 1.0f;
}

/*
 * Soft ceilings: p_board_switch_4/3/2/1/r2/l1 ~99.79% - zero-float pool
 * identity; p_board_switch_over_3 ~95.00% - equivalent high-bit test plus
 * zero-float pool identity.
 */
float p_board_switch_4(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_4);
    }
    return 0.0f;
}

float p_board_switch_over_3(void) {
    if ((g_game_info.field_04 >> 7) == 0) {
        board_game_save_data.sides[0].flags.board_input_seen = 1;
    }
    return 0.0f;
}

float p_board_switch_3(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_3);
    }
    return 0.0f;
}

float p_board_switch_2(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_2);
    }
    return 0.0f;
}

float p_board_switch_1(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_1);
    }
    return 0.0f;
}

float p_board_switch_r2(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_r2);
    }
    return 0.0f;
}

float p_board_switch_l1(void) {
    if (mk_chess_input_possibile() != 0 && mk_chess_pdata != 0) {
        xfer_proc(
            mk_chess_pdata->sides[switch_pdata->state->player]->input_proc,
            x_chess_l1);
    }
    return 0.0f;
}

#pragma dont_inline on
/*
 * Soft ceiling: mk_chess_input_possibile ~79.09% - equivalent input-state
 * range CFG remains structurally different.
 */
int mk_chess_input_possibile(void) {
    ChessManagerInfo* manager;
    int state;
    int state_accepts_input;

    if (mk_chess_check_input_from_correct_side_no_ai() == 0) {
        return 0;
    }

    manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    if (ck_eat_online_switches() != 0) {
        state_accepts_input = 0;
    } else {
        state = manager->input_state;
        if (state >= 9) {
            state_accepts_input = state < 11;
        } else if (state >= 2) {
            state_accepts_input = 0;
        } else if (state >= 0) {
            state_accepts_input = 1;
        } else {
            state_accepts_input = 0;
        }
    }

    if (state_accepts_input &&
        board_game_save_data.sides[0].input_flags.input_locked == 0) {
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling: mk_chess_check_input_from_correct_side_no_ai ~79.85% -
 * equivalent side/drone rejection CFG and register allocation differ.
 */
static int mk_chess_check_input_from_correct_side_no_ai(void) {
    ChessManagerInfo* manager;
    unsigned int switch_player;

    if (switch_pdata->state == 0) {
        return 0;
    }

    manager = mk_chess_pdata != 0 ? &mk_chess_pdata->manager : 0;
    switch_player = switch_pdata->state->player;

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

/* Soft ceiling: mk_chess_allow_cam_control ~96.84% - final result diamond emit. */
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
        if (manager->input_state != 10 && manager->input_state != 13) {
            result = 1;
        }
    }
    return result;
}

/*
 * Soft ceiling: mk_chess_zoom_return_completed ~96.36% - validated-camera
 * latch branch shape and zero-float pool identity.
 */
float mk_chess_zoom_return_completed(void) {
    CameraObj* camera = camera_item.node;

    if (camera != 0) {
        if (camera->instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    if (camera != 0) {
        mk_chess_set_viewing_quadrant(camera);
    }

    mk_chess_set_game_mode(7);
    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_mk_chess_cam_control, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: mk_chess_zoom_completed ~95.10% - validated-camera latch
 * coloring/branch shape and zero-float pool identity.
 */
float mk_chess_zoom_completed(void) {
    ChessCameraInfo* camera_info =
        mk_chess_pdata != 0 ? &mk_chess_pdata->camera : 0;
    CameraObj* camera = camera_item.node;

    if (camera != 0) {
        if (camera->instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }

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

float x_chess_2(void) {
    ChessModeState* state;

    state = mk_chess_pdata;
    if (state == 0) {
        return -1.0f;
    }

    switch (state->manager.input_state) {
    case 1:
        snd_req(0x36F);
        mk_chess_game_event(2, &state->manager.event_data, 1, 0);
        mk_chess_set_game_mode(0);
        break;
    case 9:
        state->spell->input_state = 10;
        break;
    }

    ((ChessProcVtable*)aproc->vtbl)
        ->jump_sleep(p_monitor_chess_input, 0.0f);
    return 0.0f;
}

static inline unsigned int mk_chess_save_u16(const unsigned char* bytes) {
    return ((unsigned int)bytes[0] << 8) | bytes[1];
}

static inline unsigned int mk_chess_save_u32(const unsigned char* bytes) {
    return ((unsigned int)bytes[0] << 24) |
           ((unsigned int)bytes[1] << 16) |
           ((unsigned int)bytes[2] << 8) | bytes[3];
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
            (mk_chess_save_u16(board_game_save_data.ai_settings) >> 7) & 0xF;
    } else {
        difficulty =
            (mk_chess_save_u32(board_game_save_data.ai_settings) >> 14) & 0xF;
    }

    if (difficulty == 0 || difficulty == 3 || difficulty == 4) {
        plyr_pdata->breaker_strength = 1;
    } else if (difficulty == 5) {
        plyr_pdata->breaker_strength = 3;
    } else {
        plyr_pdata->breaker_strength = 2;
    }
}

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
                &board_game_save_data.cells[x][y],
                board_game_save_data.cells[x][y].x,
                board_game_save_data.cells[x][y].y,
                board_game_save_data.cells[x][y].z,
                board_game_save_data.cells[x][y].scale);
        }
    }
}

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

static float p_mk_chess_slide_display_msg_handler(void) {
    ChessSlideMessagePdata* pdata;
    int next_x;

    pdata = (ChessSlideMessagePdata*)apdata;
    _mkproc_sleep_ticks = (float)pdata->initial_delay;
    ((ChessProcVtable*)aproc->vtbl)->sleep();

    for (;;) {
        next_x = pdata->object->x + pdata->step;
        if (pdata->step < 0) {
            if (next_x <= pdata->target_x) {
                break;
            }
        } else if (pdata->step > 0) {
            if (next_x >= pdata->target_x) {
                break;
            }
        } else {
            break;
        }
        pdata->object->x = next_x;
        _mkproc_sleep_ticks = 1.0f;
        ((ChessProcVtable*)aproc->vtbl)->sleep();
    }

    pdata->object->x = pdata->target_x;
    pdata->completed(pdata->object);
    return 0.0f;
}

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

static void mk_chess_spell_hud_handle_names_slide_out(
    ChessSpellHudNames* names) {
    ScreenObj* cursor;
    int vertex;

    mk_chess_spell_hud_show_my_spells();
    names->state = 2;
    names->selected_name = 0;
    names->page = 4;
    turn_controllers_on();

    cursor = mk_chess_pdata->hud_cursor;
    if (cursor != 0 &&
        (unsigned int)cursor->instance !=
            mk_chess_pdata->hud_cursor_instance) {
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

void mk_chess_launch_fx_at_active_piece_with_offset(
    float x_offset, float y, float z_offset) {
    MkObj* object = g_active_piece->object;
    float x = object->pos.x + x_offset;
    float z = object->pos.z + z_offset;
    unsigned int effect = fx_by_owner(4, object);

    fx_set_param_v3(0x202, x, y, z);
    fx_reset(effect);
    fx_resume_emit(effect);
}

float p_mk_chess_loop(void) {
    return 1.0f;
}

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

void mk_chess_set_viewing_quadrant(CameraObj* camera) {
    RwMatrix* matrix = camera->matrix;
    float camera_x = matrix->pos.x;
    float camera_z = matrix->pos.z;
    float facing_x = -matrix->up.x;
    float facing_z = -matrix->up.z;
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

int mk_chess_cursor_square_move_next_step(
    int cell_x, int cell_y, ChessSquareMoveTarget* cursor,
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

    (void)movement_state;
    if (current_track == primary_direction) {
        if (distance != 1) {
            return 0;
        }
        if (negative != 0) {
            mk_chess_cursor_go_to_new_track(
                cursor, primary_track, secondary_direction);
        } else {
            mk_chess_cursor_go_to_new_track(
                cursor, secondary_track, tertiary_direction);
        }
        return 1;
    }
    if (current_track == secondary_direction) {
        if (negative != 0 || distance != 1) {
            return 0;
        }
        mk_chess_cursor_go_to_new_track(
            cursor, tertiary_track, primary_direction);
        return 1;
    }
    if (current_track == tertiary_direction && negative != 0) {
        if (distance != 1) {
            return 0;
        }
        mk_chess_cursor_go_to_new_track(
            cursor, fallback_track, primary_direction);
        return 1;
    }
    return 0;
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
        particle, (void*)0x8227, emitter_id_from_handle(effect));
    if (object == 0) {
        return 0;
    }
    object->flags_08 |= 0x40;
    object->pos.x = x;
    object->pos.y = y;
    object->pos.z = z;
    update_mkobj(object);
    fx_resume_emit(effect);
    return object;
}

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
        object = pfx_bind_to_new_obj(particle, (void*)0x8227);
    }
    if (object == 0) {
        return 0;
    }
    object->flags_08 |= 0x40;
    object->pos.x = x;
    object->pos.y = y;
    object->pos.z = z;
    update_mkobj(object);
    fx_resume_emit(effect);
    return object;
}
