#include "game/minigames.h"
#include "game/controller.h"
#include "game/game_info.h"
#include "libmkparticle/particle.h"
#include "msl/msl_types.h"
#include "platform/display.h"
#include "platform/io.h"
#include "platform/main.h"
#include "runtime/cam.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"
#include "runtime/shadow.h"

#define PUZZLE_FLAG_ACCEPT_INPUT 0x10
#define PUZZLE_FLAG_COUNTER_DROPS 0x20
#define PUZZLE_FLAG_FLOOR_EVENT 0x40
#define PUZZLE_FLAG_INPUT_LATCHED 0x08
#define PUZZLE_PLAYER_FLAG_COUNTER_DROP_ACTIVE 0x20
#define PUZZLE_PLAYER_FLAG_BOARD_MOVING 0x01
#define PUZZLE_PLAYER_FLAG_IGNORE_CELL_STATE 0x02
#define PUZZLE_PLAYER_FLAG_SCANNING_HOLES 0x40
#define PUZZLE_PLAYER_FLAG_COLLAPSE_PENDING 0x40
#define PUZZLE_PLAYER_FLAG_CHAIN_RESOLVED 0x80
#define PUZZLE_PLAYER_FLAG3_COUNTER_DROPS 0x40
#define PUZZLE_PLAYER_FLAG4_FORCE_COUNTER_DROPS 0x10
#define PUZZLE_PLAYER_FLAG4_DROP_ACTIVE 0x40
#define PUZZLE_PLAYER_FLAG_QUICK_DROP_SOUND 0x10
#define PUZZLE_CONTROL_FLAG_DROP_HELD 0x08
#define PUZZLE_CELL_FLAG_MATCHED 0x80
#define PUZZLE_CELL_FLAG_BREAKING 0x40
#define PUZZLE_BLOCK_SUPERBREAKER 9
#define PUZZLE_BLOCK_WILDCARD 0xF000
#define PUZZLE_CELL_BREAK_STATE 43
#define PUZZLE_PLAYER_FLAG_ROTATION_KICKED 0x08
#define PUZZLE_PLAYER_FLAG_AI 0x80
#define PUZZLE_PLAYER_FLAG2_COUNTER_ACTIVE 0x01
#define PUZZLE_PLAYER_FLAG2_NEW_PIECE_LATCH 0x02
#define PUZZLE_PLAYER_FLAG2_NEW_PIECE 0x10
#define PUZZLE_PLAYER_FLAG2_SCORE_APPLIED 0x20
#define PUZZLE_CONTROL_FLAG_ROUND_STARTED 0x40
#define PUZZLE_CONTROL_FLAG_EXIT_REQUESTED 0x20
#define PUZZLE_CENTER_EVENT_HIGH 0x80
#define PUZZLE_CENTER_EVENT_LOW 0x40
#define PUZZLE_CENTER_EVENT_SPLIT 0x20

void* memcpy(void* destination, const void* source, unsigned long size);
void* memset(void* destination, int value, unsigned long size);
unsigned int pan_snd_req(int sound_id, float pan);
void pz_fighter_event();
void snd_req(int sound_id);
unsigned short randu0(int limit);
void push_game_state(int state);

typedef struct PuzzleBgndState {
    char pad00[0x18];
    int y;
} PuzzleBgndState;

typedef struct PuzzleBoardCell {
    union {
        unsigned char flags;
        struct {
            unsigned char matched : 1;
            unsigned char effect_bit : 1;
            unsigned char flag_pad : 6;
        } flag_bits;
    }; /* +0x00 */
    char pad01[3];
    unsigned int type; /* +0x04 */
    int state; /* +0x08 - zero permits gravity movement */
    int fall_ticks; /* +0x0C */
    unsigned int visual; /* +0x10 */
} PuzzleBoardCell; /* 0x14 */

typedef struct PuzzleProfileStats {
    unsigned int versus_wins; /* +0x00 */
    unsigned int versus_losses; /* +0x04 */
    unsigned int total_wins; /* +0x08 */
    unsigned int total_losses; /* +0x0C */
    char pad10[8];
    unsigned int best_chain; /* +0x18 */
    char pad1C[4];
    unsigned int best_counter_drop; /* +0x20 */
} PuzzleProfileStats;

typedef struct PuzzlePlayerState {
    union {
        unsigned char flags;
        struct {
            unsigned char flags_pad_high : 4;
            unsigned char superbomb_cell_phase : 2;
            unsigned char superbomb_active : 1;
            unsigned char flags_pad_low : 1;
        } flag_bits;
    }; /* +0x00 */
    union {
        unsigned char flags2;
        struct {
            unsigned char supermove_active : 1;
            unsigned char collapse_pending : 1;
            unsigned char flags2_mid_pad : 3;
            unsigned char lower_down_active : 1;
            unsigned char flags2_low_pad : 2;
        } flags2_bits;
    }; /* +0x01 */
    unsigned char flags3; /* +0x02 */
    char pad03;
    unsigned char flags4; /* +0x04 */
    char pad05[3];
    PuzzleBoardCell* active_cell; /* +0x08 - falling-pair pivot */
    char pad0C[4];
    PuzzleBoardCell* board; /* +0x10 - 8 columns x 14 rows */
    PuzzleBoardCell* board_end; /* +0x14 - inclusive */
    PuzzleBoardCell* current_pair; /* +0x18 - two cells */
    PuzzleBoardCell* next_pair; /* +0x1C - two cells */
    int active_row; /* +0x20 */
    int gravity_ticks; /* +0x24 */
    int counter_drop_delay; /* +0x28 */
    int drop_interval; /* +0x2C */
    int drop_boost_ticks; /* +0x30 */
    int quick_drop_repeat; /* +0x34 */
    int lock_ticks; /* +0x38 */
    int active_column; /* +0x3C */
    int input_repeat_ticks; /* +0x40 */
    int rotation_state; /* +0x44 */
    int next_rotation_state; /* +0x48 */
    float rotation_angle; /* +0x4C */
    union {
        unsigned char drop_flags;
        struct {
            unsigned char drop_pad_high : 3;
            unsigned char supermove_done : 1;
            unsigned char drop_pad_low : 4;
        } drop_bits;
    }; /* +0x50 */
    char pad51[3];
    int ai_move; /* +0x54 */
    int ai_fallback_move; /* +0x58 */
    int ai_no_pause_band; /* +0x5C - selects row-11 clear window */
    int ai_field_60;
    int ai_state[4]; /* +0x64 */
    char pad74[0x408];
    int locked_piece_marker; /* +0x47C */
    char pad480[0xC];
    int input_command; /* +0x48C */
    int previous_input_command; /* +0x490 */
    int (*mode_step)(struct PuzzlePlayerState* player,
                     struct PuzzlePlayerState* opponent);
    int (*saved_mode_step)(struct PuzzlePlayerState* player,
                           struct PuzzlePlayerState* opponent);
    int match_delay; /* +0x49C */
    float sound_pan; /* +0x4A0 */
    int event_player; /* +0x4A4 */
    char pad4A8[4];
    int* input_switch; /* +0x4AC */
    int field_4B0;
    int largest_counter_drop; /* +0x4B4 */
    int largest_chain; /* +0x4B8 */
    int earned_supermoves; /* +0x4BC */
    char pad4C0[0x10];
    PuzzleProfileStats* profile_stats; /* +0x4D0 */
    unsigned char center_event_flags; /* +0x4D4 */
    char pad4D5[3];
    int piece_sequence_index; /* +0x4D8 */
    int counter_sequence_index; /* +0x4DC */
    int counter_field_4E0;
    int counter_drops_remaining; /* +0x4E4 */
    int pieces_since_superbreaker; /* +0x4E8 */
    int cleared_blocks; /* +0x4EC */
    int pending_counter_drops; /* +0x4F0 */
    int chain_count; /* +0x4F4 */
    int best_chain_count; /* +0x4F8 */
    int resolved_chain_count; /* +0x4FC */
    float center_weight; /* +0x500 */
    float super_bar; /* +0x504 */
    float super_gain_per_block; /* +0x508 */
    int super_active; /* +0x50C */
    int super_windup_timer; /* +0x510 */
    int super_start_tick; /* +0x514 */
    int super_windup_value; /* +0x518 */
    int selected_supermove; /* +0x51C - table index while move is executing */
    int supermove_state; /* +0x520 */
    int supermove_amount; /* +0x524 */
    int supermove_phase_ticks; /* +0x528 */
    int supermove_delay_ticks; /* +0x52C */
    char pad530[4];
    int event_cooldown; /* +0x534 */
    int invisibility_fade; /* +0x538 */
    int invisibility_ticks; /* +0x53C */
    char pad540[0xC];
    int round_result_value; /* +0x54C */
} PuzzlePlayerState;

typedef struct PuzzleAiData {
    PuzzlePlayerState* player;
    int breaker_count; /* +0x04 - board types 4..7 */
    int highest_occupied_row; /* +0x08 */
    int normal_block_count; /* +0x0C - colors/wildcards; retail does not clear */
    union {
        unsigned int word;
        struct {
            unsigned char has_superbreaker : 1;
            unsigned char has_breaker : 1;
            unsigned char pad_flags : 6;
            unsigned char pad11[3];
        } bits;
    } flags; /* +0x10 */
} PuzzleAiData;

typedef struct PuzzleInvisiblePdata {
    void* vtbl;
    unsigned int instance;
    PuzzlePlayerState* player;
} PuzzleInvisiblePdata;

typedef struct PuzzleProcVtable {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*fn7)(void);
    int (*fn8)(void);
    float (*transfer)(float (*entry)(void), float delay);
} PuzzleProcVtable;

typedef struct PuzzlePieceLayout {
    int column;
    int row;
} PuzzlePieceLayout;

typedef int (*PuzzleSuperMoveCheck)(PuzzlePlayerState* player,
                                    PuzzlePlayerState* target);

typedef struct PuzzleSuperMoveEntry {
    int field_00;
    int fade_ticks; /* +0x04 */
    int windup_event_tick; /* +0x08 */
    int field_0C;
    int (*update)(PuzzlePlayerState* player,
                  PuzzlePlayerState* opponent); /* +0x10 */
    char pad14[0xC];
} PuzzleSuperMoveEntry; /* 0x20 */

typedef struct PuzzleRainDanceData {
    char pad00[8];
    AniTextureControl* rain_anim; /* +0x08 */
    AniTextureControl* splash_anim; /* +0x0C */
    ScreenObj* rain_object; /* +0x10 */
    ScreenObj* splash_object; /* +0x14 */
    char pad18[4];
} PuzzleRainDanceData; /* 0x1C */

typedef struct PuzzleEdgeClearData {
    int field_00;
    ScreenObj* edge_objects[2]; /* +0x04 */
    int field_0C;
} PuzzleEdgeClearData; /* 0x10 */

typedef struct PuzzleDrillData {
    AniTextureControl* drill_anim; /* +0x00 */
    ScreenObj* drill_object; /* +0x04 */
    unsigned int sound_handles[2]; /* +0x08 */
} PuzzleDrillData; /* 0x10 */

typedef struct PuzzleEvent {
    int player;
    int type;
    float block_count;
    float chain_count;
} PuzzleEvent;

typedef struct PuzzleStartMessage {
    int background; /* +0x00 */
    int background_data; /* +0x04 */
    int warmup; /* +0x08 */
    int player_character; /* +0x0C */
    int network_side; /* +0x10 */
    int player_variant; /* +0x14 */
    int blood_level; /* +0x18 */
} PuzzleStartMessage;

typedef struct PuzzleFeedRandomMessage {
    int array_index; /* +0x00 */
    int sequence_length; /* +0x04 */
    unsigned int* sequence; /* +0x08 */
} PuzzleFeedRandomMessage;

typedef struct PuzzleNetworkArrayEntry {
    unsigned int* sequence;
    int field_04;
} PuzzleNetworkArrayEntry;

typedef struct PuzzleArcadeBackground {
    int field_00;
    int background_data;
} PuzzleArcadeBackground;

typedef struct PuzzleControl {
    union {
        unsigned char flags;
        struct {
            unsigned char control_pad_high : 5;
            unsigned char large_color_clear : 1;
            unsigned char control_pad_low : 2;
        } flag_bits;
        struct {
            unsigned char sequence_pad_high : 6;
            signed char piece_sequence_owned : 1;
            unsigned char sequence_pad_low : 1;
        } sequence_bits;
    }; /* +0x00 */
    union {
        unsigned char flags2;
        struct {
            unsigned char network_sequence_marker : 1;
            unsigned char flags2_pad : 7;
        } flags2_bits;
    }; /* +0x01 */
    char pad02[2];
    MkProc* mode_proc; /* +0x04 */
    char pad08[8];
    int match_delay; /* +0x10 */
    int supermove_phase_ticks; /* +0x14 */
    int puzzle_music_channel; /* +0x18 */
    char pad1C[8];
    float fight_message_scale_step; /* +0x24 */
    float fight_message_scale_limit; /* +0x28 */
    int fight_message_x_step; /* +0x2C */
    int fight_message_y_step; /* +0x30 */
    int fight_message_alpha_step; /* +0x34 */
    char pad38[0x1C];
    PuzzlePlayerState* players[2];
    char pad5C[0x54];
    unsigned int* block_visuals; /* +0xB0 */
    unsigned int* breaker_visuals; /* +0xB4 */
    char padB8[0x54];
    union {
        struct {
            PuzzleBgndState* player1_bgnd;
            PuzzleBgndState* player2_bgnd;
        };
        ScreenObj* supermove_fade_objects[2]; /* +0x10C */
    };
    ScreenObj* fight_message; /* +0x114 */
    char pad118[4];
    int fight_message_alpha; /* +0x11C */
    char pad120[8];
    StringObj* score_text[2]; /* +0x128 */
    int player1_score; /* +0x130 */
    int player2_score; /* +0x134 */
    char pad138[0x10];
    unsigned int* piece_sequence; /* +0x148 */
    int piece_sequence_length; /* +0x14C */
} PuzzleControl;

typedef struct PuzzleFightersEngine {
    char pad00[0x198];
    union {
        unsigned char start_flags;
        struct {
            unsigned char startup_enabled : 1; /* bit7 */
            unsigned char pad : 7;
        } start_bits;
    }; /* +0x198 */
    char pad199[0x4F];
} PuzzleFightersEngine; /* 0x1E8 */

typedef struct PuzzleMatchContext {
    PuzzlePlayerState* player;
    PuzzleBoardCell* cell;
    unsigned int breaker_visual;
    unsigned int block_visual;
    unsigned int base_type;
    int matched_count;
    unsigned char flags;
    char pad19[3];
} PuzzleMatchContext;

typedef struct PuzzleStaticArt {
    char pad00[0x40];
    int player1_score_x;
    int player1_score_y;
    char pad48[4];
    int player2_score_x;
    int player2_score_y;
} PuzzleStaticArt;

typedef struct PuzzleSwitchState {
    int player;
    int field_04;
    int event;
    char pad0C[0x4C];
    PlyrPdata* fighter_pdata; /* +0x58 */
} PuzzleSwitchState;

typedef struct PuzzleSwitchPdata {
    char pad00[8];
    PuzzleSwitchState* state; /* +0x08 */
} PuzzleSwitchPdata;

typedef struct PuzzleMainPlyrView {
    char pad00[0x14];
    unsigned char flags_14;
    char pad15[0x3F];
    int character_id;
    void* fighter;
} PuzzleMainPlyrView;

typedef struct PuzzleMainGameView {
    unsigned char flags;
    char pad01[3];
    unsigned char feature_flags;
    char pad05[0x9F];
    PuzzleMainPlyrView players[2];
} PuzzleMainGameView;

typedef struct PuzzleSectionView {
    char pad00[0x48];
    void* background_lights;
    char pad4C[4];
    void* player_lights;
    float shadow_strength;
    char pad58[0x50];
    BgndMisc* misc;
} PuzzleSectionView;

typedef struct PuzzleScriptView {
    char pad00[0x58];
    unsigned int data_table_index;
} PuzzleScriptView;

typedef struct PuzzleStringTable {
    char puzzle_script[sizeof("puzzle_kombat.mko")];
    char pause_menu[sizeof("pause_menu/pause_menu")];
    char score_format[sizeof("%d00")];
    char zero[sizeof("0")];
    char storm_effect[sizeof("pz_sm_storm")];
    char number_format[sizeof("%d")];
    char puzzle_blocks[sizeof("minigames.c-puzzle_blocks")];
    char ice_blocks[sizeof("minigames.c-ice_blocks")];
    char champion_a[sizeof("PZ_CHAMPION_A")];
    char champion_b[sizeof("PZ_CHAMPION_B")];
} PuzzleStringTable;

static PuzzleControl* puzzle_ctrl;
extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleSwitchPdata* switch_pdata;
extern int __mini_game_display_ctrl;
extern int puzzle_mode_net;
extern int is_pz_net_master;
extern int game_tick_ctr;
extern PuzzleEvent pz_event;
extern PuzzleStaticArt art_puzzle_fighter_static_tbl;
extern char temp_80_char[0x50];
extern PuzzlePieceLayout puzzle_piece_layout[4];
extern PuzzleSuperMoveCheck pz_ai_super_move_table[];
extern PuzzleSuperMoveEntry pz_super_move_table[];
extern MslSoundHandle g_puzzle_music;
extern PuzzleRainDanceData pzsm_raindance_data;
extern PuzzleEdgeClearData pzsm_edger_data;
extern PuzzleDrillData pzsm_drill_data;
extern PuzzleFeedRandomMessage __pz_feed_rand_msg;
extern PuzzleStartMessage __pz_start_msg;
extern PuzzleNetworkArrayEntry* puzzle_array_table_local;
extern int pz_map_arcade_bgnd[];
extern unsigned int g_old_warmup;
extern int screen_width;
extern MkFileInfo sec_pz_ending_champion;
extern MkFileEntry gameart_file_table[];
extern MkFileEntry puzzlefighter_file_table[];
extern MkFileInfo sec_fightingart;
extern MkFileInfo sec_pz_plyr_art;
extern void* bgnd_light_list;
extern void* plyr_light_list;
extern int b_game_timer_off;
extern int round_winner;

void puzzle_fighter_display_floor_msg(PuzzlePlayerState* player, int floor);
int puzzle_fighter_match_above_below(PuzzleMatchContext* context);
int puzzle_fighter_match_left_right(PuzzleMatchContext* context);
static int puzzle_fighter_fill_holes(PuzzlePlayerState* player);
static int puzzle_fighter_find_match(PuzzlePlayerState* player);
static int puzzle_fighter_get_new_playpieces(PuzzlePlayerState* player);
static void pz_preinit_world(int network_setup);
static float p_puzzle_fighter_real_one(void);
void minigame_puzzlefighter_setup(void);
void bleed_startup(void);
void setup_sound_banks(int bank);
void display_load_meter(void);
void load_lights(void* lights, void** list);
void init_weapon_trail_light_list(void);
int is_char_locked(int character, int player);
void resolve_alternate_palettes(PlyrInfo* player);
void start_plyrs(void);
void get_pause_menu_ssh(void);
static PuzzleBoardCell*
puzzle_fighter_rotate_drop_pieces(int direction,
                                  PuzzlePlayerState* player);
static int puzzle_fighter_mode_play__drop_sequence(PuzzlePlayerState* player,
                                                   PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__new_piece(PuzzlePlayerState* player,
                                               PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__counter_drops(PuzzlePlayerState* player,
                                                   PuzzlePlayerState* opponent);
int puzzle_fighter_mode_play__supermove(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove_fade_out(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove_wind_up(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove_do(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove_wind_down(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove_done(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
int puzzle_fighter_mode_play__supermove_sleep(PuzzlePlayerState* player,
                                              PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__collapse_holes(PuzzlePlayerState* player,
                                                    PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__breakers(PuzzlePlayerState* player,
                                              PuzzlePlayerState* opponent);
/* Soft ceiling: pzsm_ai_raise_up ~86.88% - typed flag predicate. */
static int pzsm_ai_raise_up(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent);
/* Soft ceiling: pzsm_ai_rain_dance ~86.88% - typed flag predicate. */
static int pzsm_ai_rain_dance(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent);
static int pzsm_ai_lower_down(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent);
static int pzsm_ai_jumble(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent);
static int pzsm_ai_invisible(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent);
static int pzsm_ai_freeze(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent);
static int pzsm_ai_float(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent);
static int pzsm_ai_edge_clear(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent);
static int pzsm_ai_drill(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent);
static int pzsm_ai_double_bomb(PuzzlePlayerState* player,
                               PuzzlePlayerState* opponent);
static int pzsm_ai_arrange(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_ai_antibreakers(PuzzlePlayerState* player,
                                PuzzlePlayerState* opponent);
static int pz_ai_check_no_pause(PuzzlePlayerState* player);
static int puzzle_fighter_mode_play__supermove_im_dead(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int pzsm_invincible(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_kancel(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent);
static void pzsm_rain_dance_cleanup(void);
static int pzsm_lower_down(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static void pzsm_edger_cleanup(void);
static void pzsm_drill_cleanup(void);
static int pzsm_double_bomb(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent);
static int pzsm_invisible(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent);
static int pzsm_freeze(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent);
static int pzsm_clear_yellow(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent);
static int pzsm_clear_red(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent);
static int pzsm_clear_green(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent);
static int pzsm_clear_blue(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_klear_kore(PuzzlePlayerState* player, unsigned int color);
void puzzle_fighter_display_chain_msg(PuzzlePlayerState* player);
void puzzle_fighter_display_block_count_msg(PuzzlePlayerState* player);
static void puzzle_fighter_calc_center_weight(PuzzlePlayerState* player);
float p_pz_mode_exit(void);
static void pzsm_ai_get_data(PuzzleAiData* data);
float p_pzsm_invisible(void);
int check_switch(int pad, int switch_id);
int pz_ai_decide_move(PuzzlePlayerState* player);
void pz_display_supermove_msg_sideways(PuzzlePlayerState* player);
void set_snd_vol(MslSoundHandle sound, int channel, float volume);
void reset_effect(const char* effect_name);
void pfx_2d_obj_set_alpha(ScreenObj* object, unsigned char alpha);
void snd_stop(unsigned int sound_handle);
void* _create_mkproc_generic_nostack(int proc_id, int priority,
                                     float (*entry)(void), int pdata_size,
                                     PuzzleInvisiblePdata** pdata_out);
int sprintf(char* destination, const char* format, ...);
void destroy_string_obj(StringObj* string_object);
void pull_string_obj(StringObj* string_object);
StringObj* string_right_xy(int font, int alignment, const char* text, int x,
                           int y, int color);
void cleanup_pz_fatality_stuff(void);
void* get_data_table(ScriptSlot* slot, unsigned int index);
int get_blood_level(void);
void free_mem(void* memory);
static float p_pz_mode_start(void);
static void puzzle_fighter_mode_clear(void);

static const float puzzle_switch_sleep = -1.0f;
static const PuzzleStringTable puzzle_string_table = {
    "puzzle_kombat.mko",
    "pause_menu/pause_menu",
    "%d00",
    "0",
    "pz_sm_storm",
    "%d",
    "minigames.c-puzzle_blocks",
    "minigames.c-ice_blocks",
    "PZ_CHAMPION_A",
    "PZ_CHAMPION_B",
};

static float puzzle_switch_set_command(int command) {
    PuzzleSwitchState* switch_state;
    PuzzlePlayerState* player;

    if (__mini_game_display_ctrl == 0 ||
        (puzzle_ctrl->flags & PUZZLE_FLAG_ACCEPT_INPUT) == 0) {
        return puzzle_switch_sleep;
    }

    switch_state = switch_pdata->state;
    if (switch_state == 0 || switch_state->event != 2) {
        return puzzle_switch_sleep;
    }

    if ((switch_state->fighter_pdata->plyr_num != 0 && puzzle_mode_net == 0) ||
        (puzzle_mode_net != 0 && is_pz_net_master == 0)) {
        player = puzzle_ctrl->players[1];
    } else {
        player = puzzle_ctrl->players[0];
    }

    if (player->input_command < 6) {
        player->input_command = command;
    }
    player->input_repeat_ticks = 0;
    puzzle_ctrl->flags |= PUZZLE_FLAG_INPUT_LATCHED;
    return puzzle_switch_sleep;
}

void minigame_set_bgnd_y_value(int player1_y, int player2_y) {
    if (puzzle_ctrl == 0) {
        return;
    }
    puzzle_ctrl->player1_bgnd->y = player1_y;
    puzzle_ctrl->player2_bgnd->y = player2_y;
}

void minigame_get_bgnd_y_value(int* player1_y, int* player2_y) {
    if (puzzle_ctrl == 0) {
        return;
    }
    *player1_y = puzzle_ctrl->player1_bgnd->y;
    *player2_y = puzzle_ctrl->player2_bgnd->y;
}

void minigame_event(void) {
}

float puzzle_fighter_get_super_bar_level(int player) {
    PuzzlePlayerState* state;
    float level;

    level = 0.0f;
    if (puzzle_ctrl == 0) {
        return level;
    }
    if (player == 0) {
        state = puzzle_ctrl->players[0];
        level = state->super_bar / 185.0f;
        if (state->super_active <= 0) {
            return level;
        }
        return 1.0f;
    }
    state = puzzle_ctrl->players[1];
    level = state->super_bar / 185.0f;
    if (state->super_active <= 0) {
        return level;
    }
    return 1.0f;
}

float p_puzzle_fighter(void) {
    PuzzleMainGameView* game;
    PuzzleScriptView* script;
    PuzzleSectionView* section;
    int character;
    int attempts;

    game = (PuzzleMainGameView*)&g_game_info;
    set_game_switch_maps();
    set_section_memory_scheme(2);
    if ((game->feature_flags & 0x20) != 0) {
        push_game_state(3);
    } else {
        push_game_state(0x12);
    }

    load_ssf(gameart_file_table);
    load_art_section(0x10005, &sec_fightingart);
    load_ssf(puzzlefighter_file_table);
    script = (PuzzleScriptView*)cmdscript_loadfile_by_name(
        11, "puzzle_kombat.mko");
    g_game_info.cmdscript = (ScriptSlot*)script;
    section = (PuzzleSectionView*)get_data_table(
        (ScriptSlot*)script, script->data_table_index);
    g_game_info.section = (BgndDataTable*)section;
    g_game_info.misc = (BgndMisc*)section->misc;
    if (section->background_lights != 0) {
        load_lights(section->background_lights, &bgnd_light_list);
    }

    set_process_as_scriptable(aproc);
    b_game_timer_off = 1;
    round_winner = 0;
    turn_camera_off();
    turn_controllers_off();
    load_font(0);
    load_font(1);
    load_font(3);
    load_font(0x11);
    __mini_game_display_ctrl = 0;
    turn_camera_on();
    mode_of_play = 6;
    puzzle_mode_net = 0;
    if ((game->feature_flags & 0x80) == 0) {
        pz_preinit_world(is_pz_net_master);
    }

    get_pause_menu_ssh();
    display_load_meter();
    g_game_info.field_08 = 1;
    UpdateShadowCameraLightSource(section->misc->shadow_cam_light);
    ShadowStrength = section->shadow_strength;
    if (section->player_lights != 0) {
        load_lights(section->player_lights, &plyr_light_list);
    }
    init_weapon_trail_light_list();

    if ((game->feature_flags & 0x20) != 0) {
        attempts = 100;
        do {
            character = randu0(44);
            attempts--;
        } while (is_char_locked(character, 0) != 0 && attempts != 0);
        if (attempts == 0) {
            character = 0;
        }
        game->players[0].character_id = character;
        game->players[0].flags_14 &= (unsigned char)~0x40;
        if (character == 0x15) {
            game->players[0].flags_14 |= 0x80;
        } else {
            game->players[0].flags_14 &= (unsigned char)~0x80;
        }

        attempts = 100;
        do {
            character = randu0(44);
            attempts--;
        } while (is_char_locked(character, 0) != 0 && attempts != 0);
        if (attempts == 0) {
            character = 0;
        }
        game->players[1].character_id = character;
        game->players[1].flags_14 &= (unsigned char)~0x40;
        if (character == 0x15) {
            game->players[1].flags_14 |= 0x80;
        } else {
            game->players[1].flags_14 &= (unsigned char)~0x80;
        }
        resolve_alternate_palettes(&g_game_info.plyr1);
    }

    if (game->players[0].character_id == 0x15) {
        game->players[0].flags_14 |= 0x80;
    } else {
        game->players[0].flags_14 &= (unsigned char)~0x80;
    }
    if (game->players[1].character_id == 0x15) {
        game->players[1].flags_14 |= 0x80;
    } else {
        game->players[1].flags_14 &= (unsigned char)~0x80;
    }
    resolve_alternate_palettes(&g_game_info.plyr1);

    start_plyrs();
    bleed_startup();
    minigame_puzzlefighter_setup();
    setup_sound_banks(5);
    pz_event.type = 1;
    pz_fighter_event(&pz_event);
    game->flags |= 0x40;
    start_first_pass_render();
    _mkproc_sleep_ticks = 15.0f;
    ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    end_first_pass_render();
    turn_camera_off();
    while ((game->flags & 0x80) != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(
        p_puzzle_fighter_real_one, 0.0f);
    return 0.0f;
}

static inline void puzzle_profile_update_maxima(
    PuzzleProfileStats* stats, PuzzlePlayerState* player) {
    const unsigned int maximum = 999999;

    if (stats->best_chain < (unsigned int)player->best_chain_count) {
        stats->best_chain = player->best_chain_count;
    }
    if (stats->best_counter_drop <
        (unsigned int)player->largest_counter_drop) {
        stats->best_counter_drop = player->largest_counter_drop;
    }

    if (stats->total_wins > maximum) {
        stats->total_wins = maximum;
    }
    if (stats->total_losses > maximum) {
        stats->total_losses = maximum;
    }
    if (stats->best_chain > maximum) {
        stats->best_chain = maximum;
    }
    if (stats->best_counter_drop > maximum) {
        stats->best_counter_drop = maximum;
    }
}

/* Soft ceiling: p_pz_mode_clear ~99.57% - 1.0f pool identity only. */
static float p_pz_mode_clear(void) {
    int widescreen_x;

    widescreen_x = (screen_width - 0x280) / 2;
    pfxsystem_widescreen_offset(widescreen_x, 0);
    puzzle_fighter_mode_clear();
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_start, 1.0f);
    return 1.0f;
}

/* Soft ceiling: pz_update_plyr_profile_status ~85.66% - broad profile pass. */
static void pz_update_plyr_profile_status(void) {
    PuzzlePlayerState* player0 = puzzle_ctrl->players[0];
    PuzzlePlayerState* player1 = puzzle_ctrl->players[1];
    PuzzleProfileStats* stats;
    PuzzlePlayerState* player;

    if (g_game_info.plyr0.player_state == 0 &&
        player1->profile_stats != 0) {
        stats = player1->profile_stats;
        player = player1;
        if (player1->round_result_value > player0->round_result_value) {
            stats->versus_wins++;
        } else {
            stats->versus_losses++;
        }
    } else if (g_game_info.plyr1.player_state == 0 &&
               player0->profile_stats != 0) {
        stats = player0->profile_stats;
        player = player0;
        if (player0->round_result_value > player1->round_result_value) {
            stats->versus_wins++;
        } else {
            stats->versus_losses++;
        }
    } else {
        stats = player0->profile_stats;
        if (stats != 0) {
            if (player0->round_result_value > player1->round_result_value) {
                stats->total_wins++;
            } else {
                stats->total_losses++;
            }
            puzzle_profile_update_maxima(stats, player0);
        }

        stats = player1->profile_stats;
        player = player1;
        if (stats != 0) {
            if (player1->round_result_value > player0->round_result_value) {
                stats->total_wins++;
            } else {
                stats->total_losses++;
            }
        }
    }

    if (stats != 0) {
        puzzle_profile_update_maxima(stats, player);
    }
}

/* Soft ceiling: pz_init_network_array ~88.83% - broad lifecycle pass. */
static float pz_init_network_array(void) {
    PuzzleNetworkArrayEntry* table;
    unsigned int* sequence;
    int table_count;
    int sequence_length;
    int value;
    int valid;

    table = puzzle_array_table_local;
    __pz_feed_rand_msg.array_index = 0;
    while (table[__pz_feed_rand_msg.array_index].sequence != 0) {
        __pz_feed_rand_msg.array_index++;
    }
    if (table[__pz_feed_rand_msg.array_index].sequence == 0) {
        __pz_feed_rand_msg.array_index =
            randu0(__pz_feed_rand_msg.array_index);
    }

    if (puzzle_ctrl->sequence_bits.piece_sequence_owned != 0 &&
        puzzle_ctrl->piece_sequence != 0) {
        free_mem(puzzle_ctrl->piece_sequence);
    }
    puzzle_ctrl->piece_sequence = 0;

    table_count = __pz_feed_rand_msg.array_index;
    sequence = table[table_count].sequence;
    if (sequence != 0) {
        __pz_feed_rand_msg.sequence = sequence;
        sequence_length = 0;
        valid = 1;

        while ((value = sequence[sequence_length]) >= 0) {
            if (value == 0) {
                ((PuzzleProcVtable*)aproc->vtbl)
                    ->transfer(p_pz_mode_exit, 0.0f);
                valid = 0;
                break;
            }
            if (value > 8) {
                if (value == 9) {
                    puzzle_ctrl->flags2_bits.network_sequence_marker = 1;
                } else if ((unsigned int)value != PUZZLE_BLOCK_WILDCARD) {
                    ((PuzzleProcVtable*)aproc->vtbl)
                        ->transfer(p_pz_mode_exit, 0.0f);
                    valid = 0;
                    break;
                }
            }
            sequence_length++;
        }

        if (valid != 0) {
            __pz_feed_rand_msg.sequence_length = sequence_length;
            puzzle_ctrl->sequence_bits.piece_sequence_owned = 0;
        }
    }

    puzzle_ctrl->piece_sequence_length =
        __pz_feed_rand_msg.sequence_length;
    puzzle_ctrl->piece_sequence = __pz_feed_rand_msg.sequence;

    do {
        _mkproc_sleep_ticks = 1.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    } while (__pz_start_msg.network_side == 0);

    __pz_start_msg.network_side = 0;
    return 0.0f;
}

/* Soft ceiling: pz_preinit_world ~92.04% - frame/scheduling emit island. */
static void pz_preinit_world(int network_setup) {
    PuzzleArcadeBackground* backgrounds;
    unsigned int warmup;
    int attempts;

    puzzle_array_table_local = get_data_table(g_game_info.cmdscript, 0x6F);

    if (puzzle_mode_net != 0 && network_setup == 0) {
        __pz_start_msg.background = -1;
        __pz_start_msg.background_data = -1;
        __pz_start_msg.warmup = -1;
        __pz_start_msg.player_character = g_game_info.plyr1.player_index;
        __pz_start_msg.network_side = 1;
        __pz_start_msg.player_variant = g_game_info.plyr1.field_48;
        __pz_start_msg.blood_level = get_blood_level();
        return;
    }

    if ((g_game_info.field_04 & 0x20) != 0 ||
        g_game_info.bgnd_id >= 0x23) {
        __pz_start_msg.background = randu0(6);
    } else {
        __pz_start_msg.background =
            pz_map_arcade_bgnd[g_game_info.bgnd_id];
    }

    backgrounds = get_data_table(g_game_info.cmdscript, 10);
    __pz_start_msg.background_data =
        backgrounds[__pz_start_msg.background].background_data;

    warmup = randu0(3);
    attempts = 100;
    while (warmup == g_old_warmup && --attempts != 0) {
        warmup = randu0(3);
    }

    __pz_start_msg.warmup = warmup;
    g_old_warmup = warmup;
    __pz_start_msg.player_character = g_game_info.plyr0.player_index;
    __pz_start_msg.network_side = 0;
    __pz_start_msg.player_variant = g_game_info.plyr0.field_48;
    __pz_start_msg.blood_level = get_blood_level();
}

float p_puzzle_switch_lt_stick(void) {
    return 0.0f;
}

float p_puzzle_switch_drop(void) {
    return 0.0f;
}

float p_puzzle_switch_4(void) {
    return puzzle_switch_set_command(4);
}

float p_puzzle_switch_3(void) {
    return puzzle_switch_set_command(5);
}

float p_puzzle_switch_2(void) {
    return 0.0f;
}

float p_puzzle_switch_1(void) {
    PuzzleSwitchState* switch_state;
    PuzzlePlayerState* player;

    switch_state = switch_pdata->state;
    if (switch_state != 0 && __mini_game_display_ctrl != 0 &&
        (puzzle_ctrl->flags & PUZZLE_FLAG_ACCEPT_INPUT) != 0) {
        if ((switch_state->fighter_pdata->plyr_num != 0 &&
             puzzle_mode_net == 0) ||
            (puzzle_mode_net != 0 && is_pz_net_master == 0)) {
            player = puzzle_ctrl->players[1];
        } else {
            player = puzzle_ctrl->players[0];
        }

        if (player->super_active != 0) {
            player->super_active = 0;
            player->selected_supermove = player->super_windup_value;
            player->super_start_tick = game_tick_ctr;
            if (player->mode_step !=
                puzzle_fighter_mode_play__supermove_sleep) {
                player->saved_mode_step = player->mode_step;
                player->mode_step = puzzle_fighter_mode_play__supermove;
            }
        }
    }

    return puzzle_switch_sleep;
}

float p_puzzle_switch_down(void) {
    return puzzle_switch_set_command(3);
}

float p_puzzle_switch_up(void) {
    return puzzle_switch_set_command(9);
}

float p_puzzle_switch_right(void) {
    return puzzle_switch_set_command(2);
}

float p_puzzle_switch_left(void) {
    return puzzle_switch_set_command(1);
}

/* Soft ceiling: xfer_puzzle_exit ~98.55% -- flag-update register coloring. */
void xfer_puzzle_exit(int request_exit) {
    signed char exit_flag;

    if (puzzle_ctrl == 0) {
        return;
    }

    xfer_proc(puzzle_ctrl->mode_proc, p_pz_mode_exit);
    exit_flag = request_exit;
    puzzle_ctrl->flags =
        (puzzle_ctrl->flags & ~PUZZLE_CONTROL_FLAG_EXIT_REQUESTED) |
        ((exit_flag << 5) & PUZZLE_CONTROL_FLAG_EXIT_REQUESTED);

    if (request_exit != 0) {
        _mkproc_sleep_ticks = 3.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }
}

static inline int
puzzle_fighter_pair_blocked(PuzzlePlayerState* player,
                            PuzzleBoardCell* pivot) {
    PuzzleBoardCell* cell;
    PuzzlePieceLayout* layout;
    int index;

    layout = &puzzle_piece_layout[player->rotation_state];
    for (index = 0; index < 2; index++) {
        cell = pivot;
        if (index != 0) {
            cell += layout->column + layout->row * 8;
        }
        if (cell < player->board_end && cell->type != 0) {
            return 1;
        }
    }
    return 0;
}

static int
puzzle_fighter_mode_play__drop_sequence(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent) {
    PuzzleSuperMoveCheck super_check;
    PuzzlePlayerState* super_target;
    PuzzleBoardCell* candidate;
    PuzzleBoardCell* probe;
    PuzzleBoardCell* destination;
    PuzzlePieceLayout* layout;
    int command;
    int saved_column;
    int switch_left;
    int switch_right;
    int switch_drop;
    int old_ticks;
    int moved;
    int rotated;
    int force_lock;
    int skip_input;
    int index;

    (void)opponent;
    command = player->input_command;
    candidate = player->active_cell;
    rotated = 0;
    force_lock = 0;
    skip_input = 0;
    player->flags4 |= PUZZLE_PLAYER_FLAG4_DROP_ACTIVE;

    if (player->input_command == 0 &&
        player->previous_input_command == 3) {
        if ((player->flags & PUZZLE_PLAYER_FLAG_AI) == 0 &&
            check_switch(*player->input_switch, 0x0E) != 0) {
            player->gravity_ticks -= 10;
            skip_input = 1;
        } else {
            player->previous_input_command = 0;
        }
    }

    switch_drop = 0;
    if (!skip_input &&
        (player->flags & PUZZLE_PLAYER_FLAG_AI) == 0) {
        switch_drop = check_switch(*player->input_switch, 0x0C);
        if (switch_drop != 0) {
            puzzle_ctrl->flags |= PUZZLE_CONTROL_FLAG_DROP_HELD;
            if (player->drop_boost_ticks != 0) {
                player->quick_drop_repeat = 0;
            } else if (player->quick_drop_repeat == 0) {
                player->quick_drop_repeat = 4;
            } else if (player->quick_drop_repeat > 1) {
                player->quick_drop_repeat--;
            } else {
                player->input_command = 9;
                command = 9;
            }
        } else {
            player->quick_drop_repeat = 0;
        }
    }

    if (!skip_input && (player->flags & PUZZLE_PLAYER_FLAG_AI) != 0) {
        old_ticks = player->input_repeat_ticks;
        player->input_repeat_ticks--;
        if (old_ticks > 0) {
            skip_input = 1;
        } else {
            player->input_repeat_ticks = 5;
            if (player->ai_state[3] != 0) {
                player->ai_state[3]--;
                command = player->input_command;
            } else {
                moved = 0;
                if (player->super_active != 0) {
                    super_target = puzzle_ctrl->players[0];
                    if (player != super_target) {
                        super_target = puzzle_ctrl->players[1];
                    }
                    super_check =
                        pz_ai_super_move_table[player
                                                   ->super_windup_value];
                    if (super_check == 0 ||
                        super_check(player, super_target) != 0) {
                        player->super_active = 0;
                        player->selected_supermove =
                            player->super_windup_value;
                        player->super_start_tick = game_tick_ctr;
                        if (player->mode_step !=
                            puzzle_fighter_mode_play__supermove_sleep) {
                            player->saved_mode_step =
                                player->mode_step;
                            player->mode_step =
                                puzzle_fighter_mode_play__supermove;
                        }
                        moved = 1;
                    }
                }
                if (moved != 0) {
                    command = 0;
                } else {
                    command = pz_ai_decide_move(player);
                }
            }

            if (command == 0 || command == 3) {
                skip_input = 1;
            } else if (command == 9) {
                player->input_repeat_ticks = 0;
                if ((player->flags &
                     PUZZLE_PLAYER_FLAG_QUICK_DROP_SOUND) == 0) {
                    player->flags |=
                        PUZZLE_PLAYER_FLAG_QUICK_DROP_SOUND;
                    pan_snd_req(0x1B04, player->sound_pan);
                }
                force_lock = 1;
            }
        }
    }

    if (!skip_input && !force_lock &&
        (player->input_command == 0 ||
         player->input_command == 3) &&
        (player->flags & PUZZLE_PLAYER_FLAG_AI) == 0) {
        switch_left = check_switch(*player->input_switch, 0x0F);
        switch_right = 0;
        if (switch_left == 0) {
            switch_right =
                check_switch(*player->input_switch, 0x0D);
        }

        if (switch_left != 0 || switch_right != 0) {
            player->input_command = 0;
            player->quick_drop_repeat = 0;
            old_ticks = player->input_repeat_ticks;
            player->input_repeat_ticks++;
            if (old_ticks < 5) {
                skip_input = 1;
            } else {
                player->input_repeat_ticks = 0;
                command = switch_left != 0 ? 1 : 2;
            }
        } else {
            player->input_repeat_ticks = 0;
        }
    } else if (!skip_input && !force_lock &&
               command == 9 &&
               (player->flags & PUZZLE_PLAYER_FLAG_AI) == 0) {
        switch_left = check_switch(*player->input_switch, 0x0F);
        switch_right = 0;
        if (switch_left == 0) {
            switch_right =
                check_switch(*player->input_switch, 0x0D);
        }
        if (switch_left != 0 || switch_right != 0) {
            player->input_command = 0;
            player->quick_drop_repeat = 0;
            old_ticks = player->input_repeat_ticks;
            player->input_repeat_ticks++;
            if (old_ticks < 5) {
                skip_input = 1;
            } else {
                player->input_repeat_ticks = 0;
                command = switch_left != 0 ? 1 : 2;
            }
        } else {
            if ((player->flags &
                 PUZZLE_PLAYER_FLAG_QUICK_DROP_SOUND) == 0) {
                player->flags |=
                    PUZZLE_PLAYER_FLAG_QUICK_DROP_SOUND;
                pan_snd_req(0x1B04, player->sound_pan);
            }
            force_lock = 1;
        }
    }

    if (!skip_input && !force_lock) {
        saved_column = player->active_column;
        switch (command) {
        case 1:
            if (player->active_column != 0 &&
                (player->active_column > 1 ||
                 player->rotation_state != 3)) {
                candidate--;
                player->active_column--;
            }
            break;
        case 2:
            if (player->active_column < 7 &&
                (player->active_column < 6 ||
                 player->rotation_state != 1)) {
                candidate++;
                player->active_column++;
            }
            break;
        case 4:
            candidate =
                puzzle_fighter_rotate_drop_pieces(4, player);
            saved_column = player->active_column;
            rotated = 1;
            break;
        case 5:
            candidate =
                puzzle_fighter_rotate_drop_pieces(5, player);
            saved_column = player->active_column;
            rotated = 1;
            break;
        }

        moved = candidate != player->active_cell;
        if (moved != 0) {
            if (puzzle_fighter_pair_blocked(player, candidate) != 0) {
                moved = 0;
            } else if (player->gravity_ticks >
                       player->drop_interval) {
                probe = candidate + 8;
                if (puzzle_fighter_pair_blocked(player, probe) != 0) {
                    moved = 0;
                }
            }
        }

        if (moved != 0) {
            player->active_cell = candidate;
        } else {
            player->active_column = saved_column;
            if (rotated != 0) {
                pan_snd_req(0x1B06, player->sound_pan);
            }
        }
    }

    if (!force_lock) {
        if (player->drop_boost_ticks != 0) {
            player->drop_boost_ticks--;
            player->gravity_ticks++;
        }
        if (player->lock_ticks == 0) {
            old_ticks = player->gravity_ticks;
            player->gravity_ticks--;
            if (old_ticks > 0) {
                if ((player->flags & PUZZLE_PLAYER_FLAG_AI) != 0 &&
                    player->input_command != 3) {
                    return 1;
                }
                if (player->input_command != 3) {
                    if (player->input_command != 0) {
                        player->previous_input_command =
                            player->input_command;
                    }
                    player->input_command = 0;
                    return 1;
                }
                player->drop_boost_ticks = 0;
            }
            if (player->input_command != 0) {
                player->previous_input_command =
                    player->input_command;
            }
            player->input_command = 0;
            player->gravity_ticks = player->drop_interval;
        }
    }

    if (player->active_row != 0 &&
        (player->active_row != 1 ||
         player->rotation_state != 2)) {
        candidate = player->active_cell - 8;
        moved = puzzle_fighter_pair_blocked(player, candidate) == 0;
        if (moved != 0 &&
            player->gravity_ticks > player->drop_interval) {
            probe = candidate + 8;
            if (puzzle_fighter_pair_blocked(player, probe) != 0) {
                moved = 0;
            }
        }
        if (moved != 0) {
            player->active_row--;
            player->active_cell = candidate;
            player->lock_ticks = 0;
            player->gravity_ticks = player->drop_interval;
            return 1;
        }
    }

    player->gravity_ticks = 0;
    old_ticks = player->lock_ticks;
    player->lock_ticks++;
    if (command == 9 || old_ticks > 9) {
        layout = &puzzle_piece_layout[player->rotation_state];
        for (index = 0; index < 2; index++) {
            destination = player->active_cell;
            if (index != 0) {
                destination +=
                    layout->column + layout->row * 8;
            }
            destination->type = player->current_pair[index].type;
        }
        pan_snd_req(0x1B08, player->sound_pan);
        player->active_cell = 0;
        player->locked_piece_marker = -1;
        player->previous_input_command = 0;
        player->mode_step =
            puzzle_fighter_mode_play__collapse_holes;
    } else {
        if (player->input_command != 0) {
            player->previous_input_command =
                player->input_command;
        }
        player->input_command = 0;
    }

    return 1;
}

static int
puzzle_fighter_mode_play__new_piece(PuzzlePlayerState* player,
                                    PuzzlePlayerState* opponent) {
    PuzzleAiData ai_data;
    PuzzleInvisiblePdata* invisible_pdata;
    PuzzlePlayerState* other_player;
    PuzzleBoardCell* cell;
    int original_cleared;
    int clear_cell_flags;
    int row;
    int column;
    float balance;

    original_cleared = player->cleared_blocks;
    player->cleared_blocks =
        (int)((0.5f * player->resolved_chain_count + 1.0f) *
              player->cleared_blocks);
    player->pending_counter_drops += player->cleared_blocks;

    other_player = puzzle_ctrl->players[0];
    if (other_player == player) {
        other_player = puzzle_ctrl->players[1];
    }

    if (player->pending_counter_drops > 0) {
        if (player->super_active == 0) {
            player->super_bar +=
                player->super_gain_per_block * original_cleared;
            if (player->super_bar >= 185.0f) {
                player->super_bar = 0.0f;
                player->earned_supermoves++;
                if (player->super_active == 0) {
                    pz_event.type = 0x19;
                    pz_event.player = player->event_player;
                    pz_fighter_event(&pz_event);
                    player->super_active = 0x1B;
                    player->super_windup_timer = 6;
                    pan_snd_req(0x1B0D, player->sound_pan);
                }
            }
        }

        if (puzzle_mode_net == 0) {
            opponent->counter_drops_remaining +=
                player->pending_counter_drops;
            puzzle_fighter_display_block_count_msg(opponent);
        }

        if (player->largest_counter_drop <
            player->pending_counter_drops) {
            player->largest_counter_drop =
                player->pending_counter_drops;
        }
        if (player->largest_chain < player->resolved_chain_count) {
            player->largest_chain = player->resolved_chain_count;
        }
        player->pending_counter_drops = 0;
        player->cleared_blocks = 0;
        player->flags2 |= PUZZLE_PLAYER_FLAG2_SCORE_APPLIED;
    }

    player->flags2 |= PUZZLE_PLAYER_FLAG2_NEW_PIECE;
    player->chain_count = 0;
    if ((player->flags2 & PUZZLE_PLAYER_FLAG2_NEW_PIECE_LATCH) == 0) {
        player->flags2 |= PUZZLE_PLAYER_FLAG2_NEW_PIECE_LATCH;
        return 1;
    }
    player->flags2 &= ~PUZZLE_PLAYER_FLAG2_NEW_PIECE_LATCH;

    if ((player->counter_drops_remaining != 0 ||
         (player->flags4 &
          PUZZLE_PLAYER_FLAG4_FORCE_COUNTER_DROPS) != 0) &&
        (player->flags2 & PUZZLE_PLAYER_FLAG2_COUNTER_ACTIVE) == 0) {
        player->flags3 &= ~PUZZLE_PLAYER_FLAG3_COUNTER_DROPS;
        player->mode_step = puzzle_fighter_mode_play__counter_drops;
        player->flags2 |= PUZZLE_PLAYER_FLAG2_COUNTER_ACTIVE;

        if (player->counter_drops_remaining > 100) {
            snd_req(0x0F);
        } else if (player->counter_drops_remaining > 70) {
            snd_req(0x45);
        } else if (player->counter_drops_remaining > 40) {
            if (randu0(3) == 0) {
                snd_req(0x43);
            } else {
                snd_req(0x44);
            }
        } else if (player->counter_drops_remaining > 20 &&
                   (randu0(3) == 0 ||
                    player->counter_drops_remaining > 30)) {
            snd_req(0x43);
        }
        return 1;
    }

    if (puzzle_fighter_get_new_playpieces(player) == 0) {
        return 0;
    }

    if ((puzzle_ctrl->flags & PUZZLE_CONTROL_FLAG_ROUND_STARTED) == 0) {
        puzzle_ctrl->flags |= PUZZLE_CONTROL_FLAG_ROUND_STARTED;
        if (!((player->flags2 & PUZZLE_PLAYER_FLAG2_NEW_PIECE) != 0 &&
              (other_player->flags2 &
               PUZZLE_PLAYER_FLAG2_NEW_PIECE) != 0 &&
              other_player->mode_step ==
                  puzzle_fighter_mode_play__counter_drops) &&
            (player->flags2 &
             PUZZLE_PLAYER_FLAG2_SCORE_APPLIED) == 0) {
            pz_event.type = 0x18;
            pz_event.player = player->event_player;
            pz_fighter_event(&pz_event);
        }
    } else if ((player->flags2 &
                PUZZLE_PLAYER_FLAG2_SCORE_APPLIED) == 0) {
        pz_event.type = 0x18;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
    }

    puzzle_fighter_calc_center_weight(puzzle_ctrl->players[0]);
    puzzle_fighter_calc_center_weight(puzzle_ctrl->players[1]);
    balance =
        2.2f * (puzzle_ctrl->players[0]->center_weight -
                puzzle_ctrl->players[1]->center_weight);
    if (balance < -0.9f) {
        balance = -0.9f;
    } else if (balance > 0.9f) {
        balance = 0.9f;
    }
    pz_event.type = 4;
    pz_event.block_count = balance;
    pz_event.player = player->event_player;
    pz_fighter_event(&pz_event);

    clear_cell_flags = 0;
    if (player->invisibility_ticks > 0 &&
        player->invisibility_ticks < 0x1000) {
        player->invisibility_ticks--;
        if (player->invisibility_ticks == 0) {
            if (_create_mkproc_generic_nostack(
                    0x6010, 0x1F, p_pzsm_invisible, 0,
                    &invisible_pdata) != 0) {
                invisible_pdata->player = player;
            } else {
                clear_cell_flags = 1;
            }
        }
    }

    if (clear_cell_flags == 0 && player->invisibility_fade > 0) {
        player->invisibility_fade++;
        if (player->invisibility_fade > 14) {
            player->invisibility_fade = -1;
            clear_cell_flags = 1;
        }
    }

    if (clear_cell_flags != 0) {
        for (row = 0; row < 14; row++) {
            for (column = 0; column < 8; column++) {
                cell = &player->board[row * 8 + column];
                cell->flags &=
                    ~(PUZZLE_CELL_FLAG_BREAKING |
                      PUZZLE_CELL_FLAG_MATCHED);
            }
        }
    }

    player->flags2 &= ~PUZZLE_PLAYER_FLAG2_COUNTER_ACTIVE;
    player->flags2 &= ~PUZZLE_PLAYER_FLAG2_SCORE_APPLIED;
    player->mode_step = puzzle_fighter_mode_play__drop_sequence;

    if ((player->flags & PUZZLE_PLAYER_FLAG_AI) != 0) {
        ai_data.player = player;
        pzsm_ai_get_data(&ai_data);
        if (player->ai_move == 3 &&
            (ai_data.highest_occupied_row < 5 ||
             ai_data.normal_block_count < 20)) {
            player->ai_move = player->ai_fallback_move;
        }
        player->ai_state[0] = 0;
        player->ai_state[1] = 0;
        player->ai_state[2] = 0;
        player->ai_state[3] = 0;
        player->drop_flags &= ~0x04;
        player->input_command = 0;
    }

    return 1;
}

static int
puzzle_fighter_mode_play__counter_drops(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* destination;
    unsigned int type;
    int column;
    int row;

    (void)opponent;

    if (player->counter_drops_remaining != 0) {
        if (player->counter_drop_delay > 0) {
            player->counter_drop_delay--;
            return 1;
        }

        if ((player->flags & PUZZLE_PLAYER_FLAG_COUNTER_DROP_ACTIVE) == 0) {
            pan_snd_req(0x1B0B, player->sound_pan);
        }
        player->flags |= PUZZLE_PLAYER_FLAG_COUNTER_DROP_ACTIVE;

        for (row = 0;
             row < 14 && player->counter_drop_delay == 0 &&
             player->counter_drops_remaining != 0;
             row++) {
            for (column = 0; column < 8; column++) {
                cell = &player->board[row * 8 + column];
                if (cell->type != 0 && cell->state == 0) {
                    continue;
                }

                if (player->counter_sequence_index < 0) {
                    player->counter_sequence_index =
                        puzzle_ctrl->piece_sequence_length - 1;
                }
                type = puzzle_ctrl
                           ->piece_sequence[player->counter_sequence_index];
                while (type == PUZZLE_BLOCK_SUPERBREAKER) {
                    player->counter_sequence_index--;
                    if (player->counter_sequence_index < 0) {
                        player->counter_sequence_index =
                            puzzle_ctrl->piece_sequence_length - 1;
                    }
                    type = puzzle_ctrl
                               ->piece_sequence[player
                                                    ->counter_sequence_index];
                }

                if (type >= 4 && type != PUZZLE_BLOCK_WILDCARD) {
                    type -= 4;
                    if (type == 0) {
                        type = PUZZLE_BLOCK_WILDCARD;
                    }
                }

                destination = &player->board[12 * 8 + column];
                player->counter_sequence_index--;
                destination->type = type;
                destination->state = 0;
                player->counter_drop_delay = 2;
                if (player->drop_interval > 20) {
                    player->counter_drop_delay++;
                }

                player->counter_drops_remaining--;
                if (player->counter_drops_remaining == 0) {
                    player->flags3 &=
                        ~PUZZLE_PLAYER_FLAG3_COUNTER_DROPS;
                    break;
                }
            }
        }
    }

    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (player->counter_drops_remaining != 0) {
        return 0;
    }

    player->flags3 &= ~PUZZLE_PLAYER_FLAG3_COUNTER_DROPS;
    player->flags &= ~PUZZLE_PLAYER_FLAG_COUNTER_DROP_ACTIVE;
    pan_snd_req(0x1B0C, player->sound_pan);
    player->mode_step = puzzle_fighter_mode_play__breakers;
    return 1;
}

static int
puzzle_fighter_mode_play__collapse_holes(PuzzlePlayerState* player,
                                         PuzzlePlayerState* opponent) {
    int sound;

    if (puzzle_fighter_fill_holes(player) != 0) {
        player->flags2 &= ~PUZZLE_PLAYER_FLAG_COLLAPSE_PENDING;
        return 1;
    }

    if ((player->flags2 & PUZZLE_PLAYER_FLAG_COLLAPSE_PENDING) == 0) {
        if ((player->flags & PUZZLE_FLAG_COUNTER_DROPS) != 0) {
            player->mode_step = puzzle_fighter_mode_play__counter_drops;
        } else {
            player->mode_step = puzzle_fighter_mode_play__breakers;
        }
        return 1;
    }

    pz_event.block_count = (float)player->cleared_blocks;
    pz_event.player = player->event_player;
    if (player->best_chain_count == 0) {
        pz_event.type = 12;
    } else {
        pz_event.type = 6;
        pz_event.chain_count = (float)player->best_chain_count;
    }
    pz_fighter_event(&pz_event);

    player->flags2 &= ~PUZZLE_PLAYER_FLAG_COLLAPSE_PENDING;
    player->chain_count = 0;
    player->resolved_chain_count = player->best_chain_count;
    player->best_chain_count = 0;

    if (player->resolved_chain_count >= 8) {
        snd_req(0x30);
    } else if (player->resolved_chain_count >= 6) {
        snd_req(7);
    } else if (player->resolved_chain_count >= 4) {
        randu0(5);
    } else if (player->resolved_chain_count >= 2 &&
               (randu0(3) == 0 || player->cleared_blocks > 40)) {
        sound = randu0(4);
        snd_req(sound + 2);
    }

    if ((player->flags & PUZZLE_FLAG_FLOOR_EVENT) != 0 &&
        (player->flags2 & PUZZLE_PLAYER_FLAG_CHAIN_RESOLVED) == 0) {
        pz_event.type = 5;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
        puzzle_fighter_display_floor_msg(player, 0);
    }

    player->flags2 |= PUZZLE_PLAYER_FLAG_CHAIN_RESOLVED;
    if ((player->flags & PUZZLE_PLAYER_FLAG_BOARD_MOVING) != 0) {
        pan_snd_req(0x1B08, player->sound_pan);
    }

    if (player->active_cell != 0) {
        player->mode_step = puzzle_fighter_mode_play__drop_sequence;
        return puzzle_fighter_mode_play__drop_sequence(player, opponent);
    }

    player->mode_step = puzzle_fighter_mode_play__new_piece;
    return puzzle_fighter_mode_play__new_piece(player, opponent);
}

static int
puzzle_fighter_mode_play__breakers(PuzzlePlayerState* player,
                                   PuzzlePlayerState* opponent) {
    if (puzzle_fighter_find_match(player) != 0) {
        if (player->chain_count != 0 &&
            player->chain_count != player->best_chain_count) {
            player->best_chain_count = player->chain_count;
            puzzle_fighter_display_chain_msg(player);
        }
        return 1;
    }

    player->flags2_bits.collapse_pending = 1;
    player->chain_count++;
    player->mode_step = puzzle_fighter_mode_play__collapse_holes;
    return puzzle_fighter_mode_play__collapse_holes(player, opponent);
}

static int pzsm_ai_raise_up(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    (void)player;
    data.player = opponent;
    pzsm_ai_get_data(&data);
    return data.flags.bits.has_superbreaker == 0;
}

static int pzsm_ai_rain_dance(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    (void)player;
    data.player = opponent;
    pzsm_ai_get_data(&data);
    return data.flags.bits.has_superbreaker == 0;
}

/* Soft ceiling: pzsm_ai_lower_down ~88.55% - first typed policy pass. */
static int pzsm_ai_lower_down(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent) {
    PuzzleAiData data;
    int opponent_super;

    data.player = player;
    pzsm_ai_get_data(&data);

    if (data.highest_occupied_row >= 8) {
        return 1;
    }

    if (opponent->super_active != 0) {
        opponent_super = opponent->super_windup_value;
        if (opponent_super == 1 || opponent_super == 14 ||
            opponent_super == 15) {
            return 1;
        }
        if (data.highest_occupied_row > 5 &&
            (opponent_super == 10 || opponent_super == 12)) {
            return 1;
        }
    }

    if (player->counter_drops_remaining > 30 &&
        data.highest_occupied_row > 2) {
        return 1;
    }
    if (data.flags.bits.has_superbreaker != 0) {
        return 1;
    }
    if (player->super_active < 2 && data.normal_block_count > 20) {
        return 1;
    }
    return 0;
}

static int pzsm_ai_jumble(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent) {
    if (opponent->counter_drops_remaining != 0) {
        return 0;
    }
    if (player->super_active < 2) {
        return 1;
    }
    return player->super_active >= opponent->super_active;
}

/* Soft ceiling: pzsm_ai_invisible - broad eligibility pass. */
static int pzsm_ai_invisible(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent) {
    if (opponent->counter_drops_remaining != 0) {
        return 0;
    }
    if (player->super_active < 2) {
        return 1;
    }
    return 1;
}

/* Soft ceiling: pzsm_ai_freeze ~78.69% - first typed policy pass. */
static int pzsm_ai_freeze(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    data.player = opponent;
    pzsm_ai_get_data(&data);

    if (player->super_active < 2) {
        return 1;
    }
    if (data.flags.bits.has_superbreaker != 0) {
        return 0;
    }
    if (opponent->counter_drops_remaining != 0) {
        return 0;
    }
    if (data.flags.bits.has_breaker != 0) {
        return 1;
    }
    return player->super_active >= opponent->super_active;
}

/* Soft ceiling: pzsm_ai_float ~88.55% - first typed policy pass. */
static int pzsm_ai_float(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent) {
    PuzzleAiData data;
    int opponent_super;

    data.player = player;
    pzsm_ai_get_data(&data);

    if (data.highest_occupied_row >= 8) {
        return 1;
    }

    if (opponent->super_active != 0) {
        opponent_super = opponent->super_windup_value;
        if (opponent_super == 1 || opponent_super == 14 ||
            opponent_super == 15) {
            return 1;
        }
        if (data.highest_occupied_row > 5 &&
            (opponent_super == 10 || opponent_super == 12)) {
            return 1;
        }
    }

    if (player->counter_drops_remaining > 30 &&
        data.highest_occupied_row > 2) {
        return 1;
    }
    if (data.flags.bits.has_superbreaker != 0) {
        return 1;
    }
    if (player->super_active < 2 && data.normal_block_count > 20) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: pzsm_ai_edge_clear - 73.46%, exact-size typed column pass. */
static int pzsm_ai_edge_clear(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent) {
    static const int columns[2] = {0, 7};
    PuzzleAiData data;
    int occupied = 0;
    int highest = 0;
    int column_index;
    int row;
    int opponent_super;

    for (column_index = 0; column_index < 2; column_index++) {
        for (row = 0; row < 14; row++) {
            if (player->board[row * 8 + columns[column_index]].type == 0) {
                break;
            }
            occupied++;
            if (row > highest) {
                highest = row;
            }
        }
    }

    if (highest > 8 || occupied > 18) {
        return 1;
    }

    data.player = player;
    pzsm_ai_get_data(&data);
    if (data.highest_occupied_row >= 8) {
        return 1;
    }
    if (opponent->super_active != 0) {
        opponent_super = opponent->super_windup_value;
        if (opponent_super == 1 || opponent_super == 14 ||
            opponent_super == 15) {
            return 1;
        }
        if (data.highest_occupied_row > 5 &&
            (opponent_super == 10 || opponent_super == 12)) {
            return 1;
        }
    }
    if (player->counter_drops_remaining > 30 &&
        data.highest_occupied_row > 2) {
        return 1;
    }
    if (data.flags.bits.has_superbreaker != 0) {
        return 1;
    }
    if (player->super_active < 2 && data.normal_block_count > 20) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: pzsm_ai_drill - 73.46%, exact-size typed column pass. */
static int pzsm_ai_drill(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent) {
    static const int columns[2] = {3, 4};
    PuzzleAiData data;
    int occupied = 0;
    int highest = 0;
    int column_index;
    int row;
    int opponent_super;

    for (column_index = 0; column_index < 2; column_index++) {
        for (row = 0; row < 14; row++) {
            if (player->board[row * 8 + columns[column_index]].type == 0) {
                break;
            }
            occupied++;
            if (row > highest) {
                highest = row;
            }
        }
    }

    if (highest > 8 || occupied > 18) {
        return 1;
    }

    data.player = player;
    pzsm_ai_get_data(&data);
    if (data.highest_occupied_row >= 8) {
        return 1;
    }
    if (opponent->super_active != 0) {
        opponent_super = opponent->super_windup_value;
        if (opponent_super == 1 || opponent_super == 14 ||
            opponent_super == 15) {
            return 1;
        }
        if (data.highest_occupied_row > 5 &&
            (opponent_super == 10 || opponent_super == 12)) {
            return 1;
        }
    }
    if (player->counter_drops_remaining > 30 &&
        data.highest_occupied_row > 2) {
        return 1;
    }
    if (data.flags.bits.has_superbreaker != 0) {
        return 1;
    }
    if (player->super_active < 2 && data.normal_block_count > 20) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: pzsm_ai_double_bomb ~86.71% - first typed policy pass. */
static int pzsm_ai_double_bomb(PuzzlePlayerState* player,
                               PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    (void)opponent;
    data.player = player;
    pzsm_ai_get_data(&data);

    if (data.highest_occupied_row > 5 || player->super_active < 2) {
        return 1;
    }
    if (player->board[1].type == 0 || player->board[6].type == 0) {
        return 1;
    }
    return data.flags.bits.has_superbreaker != 0;
}

/* Soft ceiling: pzsm_ai_arrange ~88.29% - first typed density-policy pass. */
static int pzsm_ai_arrange(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    PuzzleAiData data;
    int occupied_blocks;
    int opponent_super;

    data.player = player;
    pzsm_ai_get_data(&data);

    if (data.flags.bits.has_superbreaker != 0) {
        return 0;
    }

    occupied_blocks = data.breaker_count + data.normal_block_count;
    if (opponent->super_active != 0) {
        opponent_super = opponent->super_windup_value;
        if (opponent_super == 14 || opponent_super == 15) {
            if (data.highest_occupied_row >= 8) {
                return 1;
            }
            if (player->counter_drops_remaining != 0 &&
                occupied_blocks + player->counter_drops_remaining < 56) {
                return 0;
            }
            if (occupied_blocks > 72) {
                return 1;
            }
        } else if (opponent_super == 12 &&
                   opponent->super_active < player->super_active) {
            return 0;
        }
    }

    if (player->counter_drops_remaining != 0 &&
        occupied_blocks + player->counter_drops_remaining < 80) {
        return 0;
    }
    if (occupied_blocks > 64) {
        return 1;
    }
    if (data.flags.bits.has_breaker != 0) {
        return 1;
    }
    return player->super_active < 2;
}

/* Soft ceiling: pzsm_ai_antibreakers ~94.17% - first typed policy pass. */
static int pzsm_ai_antibreakers(PuzzlePlayerState* player,
                                PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    data.player = opponent;
    pzsm_ai_get_data(&data);

    if (data.breaker_count < 1) {
        return 0;
    }
    if (data.flags.bits.has_superbreaker != 0 ||
        player->super_active < 2) {
        return 1;
    }
    if (data.breaker_count < 3 && data.normal_block_count < 45) {
        return 0;
    }
    return 1;
}

/* Soft ceiling: pz_ai_check_no_pause ~40.63% - typed board-window pass. */
static int pz_ai_check_no_pause(PuzzlePlayerState* player) {
    PuzzleBoardCell* row;
    PuzzleBoardCell* cell;
    int count;
    int i;

    row = &player->board[88];
    if (player->ai_no_pause_band < 2) {
        return row[3].type == 0;
    }

    if (player->ai_no_pause_band == 2) {
        cell = &row[2];
        count = 3;
    } else if (player->ai_no_pause_band == 3) {
        cell = &row[1];
        count = 5;
    } else {
        return 0;
    }

    for (i = 0; i < count; i++, cell++) {
        if (cell->type != 0) {
            return 0;
        }
    }
    return 1;
}

/* Soft ceiling: pzsm_ai_get_data ~55.06% - broad typed board-summary pass. */
static void pzsm_ai_get_data(PuzzleAiData* data) {
    PuzzlePlayerState* player = data->player;
    int row;
    int column;

    data->breaker_count = 0;
    data->highest_occupied_row = 0;
    data->flags.word = 0;

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            unsigned int type = player->board[row * 8 + column].type;

            if (type == 0) {
                continue;
            }
            if (type >= 4 && type <= 7) {
                data->breaker_count++;
            } else if (type < 4 || type == PUZZLE_BLOCK_WILDCARD) {
                data->normal_block_count++;
            } else {
                continue;
            }
            if (row > data->highest_occupied_row) {
                data->highest_occupied_row = row;
            }
        }
    }

    if (player->current_pair[0].type == PUZZLE_BLOCK_SUPERBREAKER ||
        player->current_pair[1].type == PUZZLE_BLOCK_SUPERBREAKER ||
        player->next_pair[0].type == PUZZLE_BLOCK_SUPERBREAKER ||
        player->next_pair[1].type == PUZZLE_BLOCK_SUPERBREAKER) {
        data->flags.bits.has_superbreaker = 1;
    }

    if ((player->current_pair[0].type >= 4 &&
         player->current_pair[0].type <= 7) ||
        (player->current_pair[1].type >= 4 &&
         player->current_pair[1].type <= 7) ||
        (player->next_pair[0].type >= 4 &&
         player->next_pair[0].type <= 7) ||
        (player->next_pair[1].type >= 4 &&
         player->next_pair[1].type <= 7)) {
        data->flags.bits.has_breaker = 1;
    }
}

/* Soft ceiling: 99.90%; only the shared 0.5f constant relocation differs. */
int puzzle_fighter_mode_play__supermove(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent) {
    if (opponent->mode_step != puzzle_fighter_mode_play__supermove_sleep) {
        if (opponent->mode_step != puzzle_fighter_mode_play__supermove) {
            opponent->saved_mode_step = opponent->mode_step;
        }
        opponent->mode_step = puzzle_fighter_mode_play__supermove_sleep;
    }

    puzzle_ctrl->supermove_phase_ticks = 40;
    pz_display_supermove_msg_sideways(player);
    if (g_puzzle_music != 0) {
        set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel, 0.5f);
    }
    snd_req(0x1B0A);

    pz_event.type = 7;
    pz_event.player = player->event_player;
    pz_fighter_event();

    player->mode_step = puzzle_fighter_mode_play__supermove_fade_out;
    return 1;
}

/* Soft ceiling: 74.38%; exact-size broad fade loop, defer emit refinement. */
static int puzzle_fighter_mode_play__supermove_fade_out(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    int object_index;

    (void)opponent;
    if (g_puzzle_music != 0) {
        set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel,
                    (float)(puzzle_ctrl->supermove_phase_ticks / 80));
    }

    for (object_index = 0; object_index < 2; object_index++) {
        pfx_2d_obj_set_alpha(
            puzzle_ctrl->supermove_fade_objects[object_index],
            (unsigned char)(puzzle_ctrl->supermove_phase_ticks * 4 + 80));
    }

    puzzle_ctrl->supermove_phase_ticks--;
    if (puzzle_ctrl->supermove_phase_ticks == 1) {
        puzzle_ctrl->supermove_phase_ticks =
            pz_super_move_table[player->selected_supermove].fade_ticks;
        player->mode_step = puzzle_fighter_mode_play__supermove_wind_up;
    }
    return 1;
}

/* Exact: typed bitfield assignment recovers retail's rlwimi sequence. */
static int puzzle_fighter_mode_play__supermove_wind_up(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    (void)opponent;
    if (puzzle_ctrl->supermove_phase_ticks ==
        pz_super_move_table[player->selected_supermove].windup_event_tick) {
        pz_event.type = 0x10;
        pz_event.player = player->event_player;
        pz_fighter_event();
    }

    puzzle_ctrl->supermove_phase_ticks--;
    if (puzzle_ctrl->supermove_phase_ticks == 1) {
        player->mode_step = puzzle_fighter_mode_play__supermove_do;
        player->flags2_bits.supermove_active = 1;
        player->chain_count = 0;
    }
    return 1;
}

/* Exact: table callback and completion event flow. */
static int puzzle_fighter_mode_play__supermove_do(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    if (pz_super_move_table[player->selected_supermove].update(player,
                                                               opponent) ==
        0) {
        player->mode_step = puzzle_fighter_mode_play__supermove_wind_down;
        pz_event.type = 0x11;
        pz_event.player = player->event_player;
        pz_fighter_event();
    }
    return 1;
}

/* Soft ceiling: 77.64%; exact-size broad fade loop, defer emit refinement. */
static int puzzle_fighter_mode_play__supermove_wind_down(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    int object_index;
    int old_ticks;

    (void)opponent;
    old_ticks = puzzle_ctrl->supermove_phase_ticks;
    puzzle_ctrl->supermove_phase_ticks = old_ticks + 1;
    if (old_ticks < 10) {
        if (g_puzzle_music != 0) {
            set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel,
                        (float)(puzzle_ctrl->supermove_phase_ticks / 10));
        }
        for (object_index = 0; object_index < 2; object_index++) {
            pfx_2d_obj_set_alpha(
                puzzle_ctrl->supermove_fade_objects[object_index],
                (unsigned char)(puzzle_ctrl->supermove_phase_ticks * 25));
        }
    } else {
        player->mode_step = puzzle_fighter_mode_play__supermove_done;
    }
    return 1;
}

/* Soft ceiling: 89.33%; body matches broadly, save/restore coloring differs. */
static int puzzle_fighter_mode_play__supermove_done(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    if (g_puzzle_music != 0) {
        set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel, 1.0f);
    }

    player->selected_supermove = -1;
    player->supermove_state = 0;
    puzzle_ctrl->supermove_phase_ticks = 0;
    player->mode_step = player->saved_mode_step;

    if (opponent->selected_supermove == -1) {
        opponent->mode_step = opponent->saved_mode_step;
    } else {
        opponent->mode_step = puzzle_fighter_mode_play__supermove;
        opponent->super_start_tick = 2;
    }

    player->drop_bits.supermove_done = 1;
    opponent->drop_bits.supermove_done = 1;
    return 1;
}

static int puzzle_fighter_mode_play__supermove_im_dead(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    (void)player;
    (void)opponent;
    return 0;
}

int puzzle_fighter_mode_play__supermove_sleep(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    (void)player;
    (void)opponent;
    return 1;
}

static int pzsm_invincible(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    (void)player;
    (void)opponent;
    return 0;
}

static int pzsm_kancel(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent) {
    (void)player;
    (void)opponent;
    return 0;
}

/* Soft ceiling: pzsm_rain_dance_cleanup 90.44%; typed ownership near miss. */
static void pzsm_rain_dance_cleanup(void) {
    reset_effect(puzzle_string_table.storm_effect);

    if (pzsm_raindance_data.rain_anim != 0) {
        destroy_ani_texture_control(pzsm_raindance_data.rain_anim);
    }
    pzsm_raindance_data.rain_anim = 0;

    if (pzsm_raindance_data.rain_object != 0) {
        destroy_screen_obj(pzsm_raindance_data.rain_object);
    }
    pzsm_raindance_data.rain_object = 0;

    if (pzsm_raindance_data.splash_anim != 0) {
        destroy_ani_texture_control(pzsm_raindance_data.splash_anim);
    }
    pzsm_raindance_data.splash_anim = 0;

    if (pzsm_raindance_data.splash_object != 0) {
        destroy_screen_obj(pzsm_raindance_data.splash_object);
    }
    pzsm_raindance_data.splash_object = 0;
}

/* Soft ceiling: pzsm_lower_down 88.07%; exact-size typed row-clear pass. */
static int pzsm_lower_down(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    int removed = 0;
    int column;

    (void)opponent;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = player->supermove_amount;
        player->supermove_delay_ticks = 0;
        player->saved_mode_step =
            puzzle_fighter_mode_play__collapse_holes;
    }

    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (player->supermove_delay_ticks != 0) {
        player->supermove_delay_ticks--;
        return 1;
    }
    if (player->supermove_phase_ticks == 0) {
        return 0;
    }

    player->supermove_delay_ticks = 15;
    for (column = 0; column < 8; column++) {
        PuzzleBoardCell* cell = &player->board[column];

        if (cell->type != 0) {
            removed = 1;
        }
        memset(cell, 0, sizeof(*cell));
    }

    if (removed == 0) {
        return 0;
    }

    player->match_delay = 40;
    player->flags2_bits.lower_down_active = 1;
    pan_snd_req(0x1AFE, player->sound_pan);
    player->supermove_phase_ticks--;
    return 1;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
/* Soft ceiling: p_pzsm_invisible 79.40%; exact-size typed lifetime process. */
float p_pzsm_invisible(void) {
    PuzzleInvisiblePdata* pdata = (PuzzleInvisiblePdata*)apdata;
    PuzzlePlayerState* player;
    int row;
    int column;

    if (pdata == 0) {
        return -1.0f;
    }

    player = pdata->player;
    if (player->invisibility_ticks == 0) {
        player->invisibility_ticks = 0x1000;
        return 1.0f;
    }

    player->invisibility_ticks++;
    if (player->invisibility_ticks < 0x1014) {
        return 1.0f;
    }

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            player->board[row * 8 + column].flag_bits.effect_bit = 0;
        }
    }
    player->invisibility_ticks = 0;
    return -1.0f;
}

/* Soft ceiling: pzsm_invisible 78.86%; exact-size typed flash coordinator. */
static int pzsm_invisible(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent) {
    int row;
    int column;
    signed char effect_visible;

    if (player->supermove_state == 0) {
        player->supermove_phase_ticks = 15;
        player->supermove_state = 19;
        player->counter_drop_delay = 0;
        opponent->invisibility_ticks = 5;
        opponent->saved_mode_step =
            puzzle_fighter_mode_play__collapse_holes;
        snd_req(0x1AFC);
    }

    if (player->counter_drop_delay != 0) {
        player->counter_drop_delay--;
        return 1;
    }

    player->supermove_phase_ticks--;
    effect_visible = (player->supermove_phase_ticks & 1) ^ 1;
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            PuzzleBoardCell* cell = &opponent->board[row * 8 + column];

            if (cell->type != 0) {
                cell->flag_bits.effect_bit = effect_visible;
            }
        }
    }

    if (player->supermove_phase_ticks == 0) {
        return 0;
    }
    player->counter_drop_delay = 3;
    return 1;
}

/* Soft ceiling: pzsm_freeze 86.52%; exact-size typed board/timer pass. */
static int pzsm_freeze(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent) {
    int row;
    int column;

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->counter_drop_delay = 0;
        opponent->invisibility_fade = 14;
        opponent->saved_mode_step =
            puzzle_fighter_mode_play__collapse_holes;
        snd_req(0x1AFB);

        for (row = 0; row < 14; row++) {
            for (column = 0; column < 8; column++) {
                PuzzleBoardCell* cell = &opponent->board[row * 8 + column];

                if (cell->type != 0) {
                    cell->flag_bits.matched = 1;
                }
            }
        }
        return 1;
    }

    if (player->counter_drop_delay != 0) {
        player->counter_drop_delay--;
        return 1;
    }

    opponent->invisibility_fade--;
    if (opponent->invisibility_fade > 10) {
        player->counter_drop_delay = 7;
        return 1;
    }
    return 0;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

/* Soft ceiling: pzsm_edger_cleanup 60.20%; exact-size typed two-object loop. */
static void pzsm_edger_cleanup(void) {
    int object_index;

    for (object_index = 0; object_index < 2; object_index++) {
        ScreenObj** object = &pzsm_edger_data.edge_objects[object_index];

        if (*object != 0) {
            destroy_screen_obj(*object);
        }
        *object = 0;
    }
}

/* Soft ceiling: pzsm_drill_cleanup 88.78%; exact-size resource sequence. */
static void pzsm_drill_cleanup(void) {
    if (pzsm_drill_data.drill_anim != 0) {
        destroy_ani_texture_control(pzsm_drill_data.drill_anim);
    }
    pzsm_drill_data.drill_anim = 0;

    if (pzsm_drill_data.drill_object != 0) {
        destroy_screen_obj(pzsm_drill_data.drill_object);
    }
    pzsm_drill_data.drill_object = 0;

    if (pzsm_drill_data.sound_handles[0] != 0) {
        snd_stop(pzsm_drill_data.sound_handles[0]);
    }
    pzsm_drill_data.sound_handles[0] = 0;

    if (pzsm_drill_data.sound_handles[1] != 0) {
        snd_stop(pzsm_drill_data.sound_handles[1]);
    }
    pzsm_drill_data.sound_handles[1] = 0;
}

/* Soft ceiling: pzsm_double_bomb 97.55%; board base/value coloring remains. */
static int pzsm_double_bomb(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent) {
    (void)opponent;
    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->board[12 * 8 + 1].type = PUZZLE_BLOCK_SUPERBREAKER;
        player->board[12 * 8 + 6].type = PUZZLE_BLOCK_SUPERBREAKER;
        player->event_cooldown = 2;
        player->flag_bits.superbomb_cell_phase = 3;
        player->flag_bits.superbomb_active = 1;
        pan_snd_req(0x1AF7, player->sound_pan);
        player->saved_mode_step = puzzle_fighter_mode_play__new_piece;
        return 1;
    }

    player->flag_bits.superbomb_active = 0;
    return 0;
}

/* Soft ceiling: color wrappers 91.85-92.14%; emit-only near misses. */
static int pzsm_clear_yellow(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent) {
    (void)opponent;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        pzsm_klear_kore(player, PUZZLE_BLOCK_WILDCARD);
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }
    return puzzle_fighter_fill_holes(player) != 0;
}

static int pzsm_clear_red(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent) {
    (void)opponent;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        pzsm_klear_kore(player, 1);
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }
    return puzzle_fighter_fill_holes(player) != 0;
}

static int pzsm_clear_green(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent) {
    (void)opponent;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        pzsm_klear_kore(player, 2);
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }
    return puzzle_fighter_fill_holes(player) != 0;
}

static int pzsm_clear_blue(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    (void)opponent;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        pzsm_klear_kore(player, 3);
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }
    return puzzle_fighter_fill_holes(player) != 0;
}

/* Soft ceiling: pzsm_klear_kore 75.53%; exact-size structured board pass. */
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static int pzsm_klear_kore(PuzzlePlayerState* player, unsigned int color) {
    int cleared = 0;
    int row;
    int column;
    int row_offset = 0;
    unsigned int breaker_color = color + 4;

    player->saved_mode_step = puzzle_fighter_mode_play__new_piece;

    for (row = 0; row < 14; row++, row_offset += 8) {
        for (column = 0; column < 8; column++) {
            PuzzleBoardCell* cell = &player->board[row_offset + column];

            if ((cell->type == color || cell->type == breaker_color) &&
                cell->state == 0) {
                unsigned int visual_index =
                    color == PUZZLE_BLOCK_WILDCARD ? 0 : color;

                cell->state = PUZZLE_CELL_BREAK_STATE;
                cleared++;
                cell->visual = puzzle_ctrl->block_visuals[visual_index];
                cell->type = color;
            }
        }
    }

    if (cleared < 5) {
        pan_snd_req(0x1B0F, player->sound_pan);
    } else if (cleared < 10) {
        pan_snd_req(0x1B10, player->sound_pan);
    } else {
        pan_snd_req(0x1B11, player->sound_pan);
        puzzle_ctrl->match_delay = 40;
        puzzle_ctrl->flag_bits.large_color_clear = 1;
    }
    return cleared;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

static void puzzle_fighter_calc_center_weight(PuzzlePlayerState* player) {
    int total_weight = 0;
    int high_columns = 0;
    int peak_columns = 0;
    int low_or_empty_columns = 0;
    int column;

    for (column = 0; column < 8; column++) {
        int found = 0;
        int row;
        int row_offset;

        for (row = 13, row_offset = 104; row >= 0;
             row--, row_offset -= 8) {
            if (player->board[row_offset + column].type != 0) {
                found = 1;
                total_weight += row + 1;
                if (row > 9) {
                    peak_columns++;
                }
                if (row > 9) {
                    high_columns++;
                } else if (row < 6) {
                    low_or_empty_columns++;
                }
                break;
            }
        }
        if (!found) {
            low_or_empty_columns++;
        }
    }

    player->center_weight = (float)total_weight / 86.399994f;

    if (high_columns > 5) {
        if ((player->center_event_flags & PUZZLE_CENTER_EVENT_HIGH) == 0) {
            player->center_event_flags |= PUZZLE_CENTER_EVENT_HIGH;
            pz_event.type = 8;
            pz_event.player = player->event_player;
            pz_fighter_event();
        }
    } else if (peak_columns == 3 && low_or_empty_columns > 2) {
        if ((player->center_event_flags & PUZZLE_CENTER_EVENT_SPLIT) == 0) {
            player->center_event_flags |= PUZZLE_CENTER_EVENT_SPLIT;
            pz_event.type = 0x17;
            pz_event.player = player->event_player;
            pz_fighter_event();
        }
    }

    if (low_or_empty_columns > 7 &&
        (player->center_event_flags & PUZZLE_CENTER_EVENT_HIGH) != 0 &&
        (player->center_event_flags & PUZZLE_CENTER_EVENT_LOW) == 0) {
        player->center_event_flags |= PUZZLE_CENTER_EVENT_LOW;
        pz_event.type = 9;
        pz_event.player = player->event_player;
        pz_fighter_event();
    }
}

#pragma optimize_for_size on
void puzzle_fighter_get_num_blocks_on_screen(int* player1_blocks,
                                             int* player2_blocks) {
    PuzzlePlayerState* player;
    int column;
    int row;
    int total;

    total = 0;
    player = puzzle_ctrl->players[0];
    for (column = 0; column < 8; column++) {
        row = 13;
        while (player->board[row * 8 + column].type == 0) {
            row--;
        }
        total += row + 1;
    }
    *player1_blocks = total;

    total = 0;
    player = puzzle_ctrl->players[1];
    for (column = 0; column < 8; column++) {
        row = 13;
        while (player->board[row * 8 + column].type == 0) {
            row--;
        }
        total += row + 1;
    }
    *player2_blocks = total;
}
#pragma optimize_for_size reset

static int puzzle_fighter_fill_holes(PuzzlePlayerState* player) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* destination;
    PuzzleBoardCell* source;
    int column;
    int row;
    int destination_row;
    int source_row;
    int board_moving;

    board_moving = 0;
    player->flags |= PUZZLE_PLAYER_FLAG_SCANNING_HOLES;

    for (column = 0; column < 8; column++) {
        for (row = 0; row < 14; row++) {
            cell = &player->board[row * 8 + column];
            if (cell->type != 0 &&
                (cell->state == 0 ||
                 (player->flags & PUZZLE_PLAYER_FLAG_IGNORE_CELL_STATE) != 0)) {
                player->flags &= ~PUZZLE_PLAYER_FLAG_SCANNING_HOLES;
                if (cell->fall_ticks != 0) {
                    board_moving = 1;
                }
                continue;
            }

            destination_row = row;
            for (source_row = row; source_row < 14; source_row++) {
                source = &player->board[source_row * 8 + column];
                if (source->type != 0 && source->state == 0) {
                    for (; source_row < 14; source_row++) {
                        source = &player->board[source_row * 8 + column];
                        if (source->type != 0) {
                            destination =
                                &player->board[destination_row * 8 + column];
                            memcpy(destination, source,
                                   sizeof(PuzzleBoardCell));
                            memset(source, 0, sizeof(PuzzleBoardCell));
                            destination->fall_ticks =
                                (source_row - destination_row) * 25;
                            board_moving = 1;
                            destination_row++;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    player->flags =
        (player->flags & ~PUZZLE_PLAYER_FLAG_BOARD_MOVING) |
        (board_moving & PUZZLE_PLAYER_FLAG_BOARD_MOVING);
    return board_moving;
}

#pragma dont_inline on
#pragma optimize_for_size on
static int puzzle_fighter_find_superbreaker(PuzzlePlayerState* player) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* superbreaker;
    unsigned int below_type;
    unsigned int base_type;
    unsigned int paired_type;
    int visual_index;
    int row;
    int column;
    int marked;

    superbreaker = 0;
    for (row = 0; row < 14 && superbreaker == 0; row++) {
        for (column = 0; column < 8; column++) {
            cell = &player->board[row * 8 + column];
            if (cell->type == PUZZLE_BLOCK_SUPERBREAKER &&
                cell->state == 0) {
                superbreaker = cell;
                break;
            }
        }
    }

    if (superbreaker == 0) {
        return 0;
    }

    superbreaker->state = PUZZLE_CELL_BREAK_STATE;
    if (superbreaker < &player->board[8]) {
        player->super_bar += 24.0f;
        if (player->super_bar > 185.0f) {
            player->super_bar = 185.0f;
        }
        superbreaker->visual = puzzle_ctrl->block_visuals[4];

        if (player->event_cooldown == 0) {
            pz_event.type = 0x16;
            pz_event.player = player->event_player;
            pz_fighter_event();
        } else {
            player->event_cooldown--;
        }

        puzzle_fighter_display_floor_msg(player, 1);
        pan_snd_req(0x1B13, player->sound_pan);
        pan_snd_req(0x1B14, player->sound_pan);
        return 0;
    }

    if (player->event_cooldown == 0) {
        pz_event.type = 0x15;
        pz_event.player = player->event_player;
        pz_fighter_event();
    } else {
        player->event_cooldown--;
    }

    below_type = superbreaker[-8].type;
    for (visual_index = 0; visual_index < 8; visual_index++) {
        if ((visual_index == 0 && below_type == PUZZLE_BLOCK_WILDCARD) ||
            (unsigned int)visual_index == below_type) {
            break;
        }
    }

    if (visual_index >= 4) {
        paired_type = (unsigned int)visual_index;
        base_type = (unsigned int)(visual_index - 4);
        visual_index -= 4;
    } else {
        base_type = (unsigned int)visual_index;
        paired_type = (unsigned int)(visual_index + 4);
    }
    if (base_type == 0) {
        base_type = PUZZLE_BLOCK_WILDCARD;
    }

    marked = 0;
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            cell = &player->board[row * 8 + column];
            if (cell->type == base_type || cell->type == paired_type) {
                cell->flags &=
                    ~(PUZZLE_CELL_FLAG_MATCHED | PUZZLE_CELL_FLAG_BREAKING);
                if (cell->state == 0) {
                    marked++;
                    cell->state = PUZZLE_CELL_BREAK_STATE;
                    cell->visual = puzzle_ctrl->block_visuals[visual_index];
                    cell->type = base_type;
                }
            }
        }
    }

    superbreaker->visual = puzzle_ctrl->block_visuals[visual_index];
    superbreaker->type = base_type;
    return marked;
}
#pragma optimize_for_size reset
#pragma dont_inline reset

int puzzle_fighter_match_left_right(PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* boundary;
    PuzzleBoardCell* neighbor;
    int matched;

    matched = 0;
    next.player = context->player;
    next.breaker_visual = context->breaker_visual;
    next.block_visual = context->block_visual;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.flags = context->flags & PUZZLE_CELL_FLAG_MATCHED;

    for (boundary = context->player->board;
         boundary <= context->player->board_end; boundary += 8) {
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell - 1;
        if (neighbor >= context->player->board &&
            (neighbor->type == context->cell->type ||
             neighbor->type == context->base_type) &&
            (neighbor->flags & PUZZLE_CELL_FLAG_MATCHED) == 0) {
            neighbor->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
            if (neighbor->state == 0) {
                neighbor->state = PUZZLE_CELL_BREAK_STATE;
                neighbor->visual = context->block_visual;
                neighbor->type = context->base_type;
                next.cell = neighbor;
                puzzle_fighter_match_above_below(&next);
                puzzle_fighter_match_left_right(&next);
                next.matched_count++;
            }
            matched = 1;
        }
    }

    for (boundary = context->player->board + 7;
         boundary <= context->player->board_end; boundary += 8) {
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell + 1;
        if (neighbor <= context->player->board_end &&
            (neighbor->type == context->cell->type ||
             neighbor->type == context->base_type) &&
            (neighbor->flags & PUZZLE_CELL_FLAG_MATCHED) == 0) {
            neighbor->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
            if (neighbor->state == 0) {
                neighbor->state = PUZZLE_CELL_BREAK_STATE;
                neighbor->visual = context->block_visual;
                neighbor->type = context->base_type;
                next.cell = neighbor;
                puzzle_fighter_match_above_below(&next);
                puzzle_fighter_match_left_right(&next);
                next.matched_count++;
            }
            matched = 1;
        }
    }

    context->matched_count += next.matched_count;
    return matched;
}

int puzzle_fighter_match_above_below(PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* neighbor;
    int matched;

    matched = 0;
    next.player = context->player;
    next.breaker_visual = context->breaker_visual;
    next.block_visual = context->block_visual;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.flags = context->flags & PUZZLE_CELL_FLAG_MATCHED;

    neighbor = context->cell + 8;
    if (neighbor <= context->player->board_end &&
        (neighbor->type == context->cell->type ||
         neighbor->type == context->base_type) &&
        (neighbor->flags & PUZZLE_CELL_FLAG_MATCHED) == 0) {
        neighbor->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
        if (neighbor->state == 0) {
            neighbor->state = PUZZLE_CELL_BREAK_STATE;
            neighbor->visual = context->block_visual;
            neighbor->type = context->base_type;
            next.cell = neighbor;
            puzzle_fighter_match_above_below(&next);
            puzzle_fighter_match_left_right(&next);
            next.matched_count++;
        }
        matched = 1;
    }

    neighbor = context->cell - 8;
    if (neighbor >= context->player->board &&
        (neighbor->type == context->base_type ||
         neighbor->type == context->cell->type) &&
        (neighbor->flags & PUZZLE_CELL_FLAG_MATCHED) == 0) {
        neighbor->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
        if (neighbor->state == 0) {
            neighbor->state = PUZZLE_CELL_BREAK_STATE;
            neighbor->visual = context->block_visual;
            neighbor->type = context->base_type;
            next.cell = neighbor;
            puzzle_fighter_match_above_below(&next);
            puzzle_fighter_match_left_right(&next);
            next.matched_count++;
        }
        matched = 1;
    }

    context->matched_count += next.matched_count;
    return matched;
}

static int puzzle_fighter_find_match(PuzzlePlayerState* player) {
    PuzzleMatchContext context;
    PuzzleBoardCell* cell;
    unsigned int breaker_type;
    unsigned int match_mode;
    int first_superbreakers;
    int second_superbreakers;
    int superbreakers;
    int any_activity;
    int row;
    int column;
    int score;
    int score_delta;
    int player_index;
    int score_x;
    int score_y;

    first_superbreakers = 0;
    second_superbreakers = 0;
    do {
        first_superbreakers = second_superbreakers;
        second_superbreakers = puzzle_fighter_find_superbreaker(player);
    } while (first_superbreakers == 0 && second_superbreakers != 0);

    if (first_superbreakers != 0 && second_superbreakers != 0) {
        player->flags = (player->flags & ~0x0C) | 0x04;
    }

    superbreakers = first_superbreakers + second_superbreakers;
    if (superbreakers != 0) {
        player->match_delay = 40;
        player->flags2 &= ~0x04;
    }

    any_activity = 0;
    context.matched_count = 0;
    context.flags = 0;

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            cell = &player->board[row * 8 + column];
            if (cell->state != 0) {
                any_activity = 1;
            }
            if (cell->type == 0) {
                continue;
            }

            for (breaker_type = 4; breaker_type < 8; breaker_type++) {
                if (breaker_type == cell->type) {
                    break;
                }
            }
            if (breaker_type >= 8) {
                continue;
            }

            context.base_type =
                breaker_type == 4 ? PUZZLE_BLOCK_WILDCARD : breaker_type - 4;
            context.block_visual =
                puzzle_ctrl->block_visuals[breaker_type - 4];
            context.breaker_visual =
                puzzle_ctrl->breaker_visuals[breaker_type - 4];
            context.player = player;
            context.cell = cell;

            if ((cell->flags & PUZZLE_CELL_FLAG_MATCHED) != 0) {
                continue;
            }

            if (puzzle_fighter_match_above_below(&context) != 0) {
                cell->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
                if (cell->state == 0) {
                    cell->state = PUZZLE_CELL_BREAK_STATE;
                    cell->visual = context.block_visual;
                    cell->type = context.base_type;
                    context.matched_count++;
                }
                any_activity = 1;
            }

            if (puzzle_fighter_match_left_right(&context) != 0) {
                cell->flags &= ~PUZZLE_CELL_FLAG_BREAKING;
                if (cell->state == 0) {
                    cell->state = PUZZLE_CELL_BREAK_STATE;
                    cell->visual = context.block_visual;
                    cell->type = context.base_type;
                    context.matched_count++;
                }
                any_activity = 1;
            }
        }
    }

    if (any_activity == 0) {
        player->input_command = 0;
    } else {
        player->cleared_blocks += context.matched_count;
    }

    if (superbreakers == 0) {
        if (context.matched_count != 0) {
            if (context.matched_count < 5) {
                pan_snd_req(0x1B0F, player->sound_pan);
            } else if (context.matched_count < 10) {
                pan_snd_req(0x1B10, player->sound_pan);
            } else {
                pan_snd_req(0x1B11, player->sound_pan);
                puzzle_ctrl->match_delay = 40;
                puzzle_ctrl->flags |= 0x04;
            }
        }
    } else {
        match_mode = (player->flags >> 2) & 3;
        if (match_mode == 0) {
            pan_snd_req(0x1B14, player->sound_pan);
        } else {
            pan_snd_req(0x1AF6, player->sound_pan);
            if (match_mode == 3) {
                player->flags = (player->flags & ~0x0C) | 0x04;
            } else {
                player->flags &= ~0x0C;
            }
        }
    }

    score_delta =
        superbreakers + context.matched_count * (player->chain_count + 1);
    player_index = player->event_player;
    if (player_index == 0) {
        puzzle_ctrl->player1_score += score_delta;
        score = puzzle_ctrl->player1_score;
        score_x = art_puzzle_fighter_static_tbl.player1_score_x;
        score_y = art_puzzle_fighter_static_tbl.player1_score_y;
    } else {
        puzzle_ctrl->player2_score += score_delta;
        score = puzzle_ctrl->player2_score;
        score_x = art_puzzle_fighter_static_tbl.player2_score_x;
        score_y = art_puzzle_fighter_static_tbl.player2_score_y;
    }

    if (score == 0) {
        sprintf(temp_80_char, "0");
    } else {
        sprintf(temp_80_char, puzzle_string_table.score_format, score);
    }

    if (puzzle_ctrl->score_text[player_index] != 0) {
        destroy_string_obj(puzzle_ctrl->score_text[player_index]);
    }
    puzzle_ctrl->score_text[player_index] =
        string_right_xy(0x6023, 0x0D, temp_80_char, score_x + 0x36,
                        score_y + 4, 0x3A);
    if (puzzle_ctrl->score_text[player_index] != 0) {
        pull_string_obj(puzzle_ctrl->score_text[player_index]);
    }

    return any_activity;
}

static PuzzleBoardCell*
puzzle_fighter_rotate_drop_pieces(int direction, PuzzlePlayerState* player) {
    PuzzleBoardCell* candidate;
    unsigned int saved_type;

    switch (player->rotation_state) {
    case 3:
        if (direction == 5) {
            candidate = player->active_cell - 8;
            if (candidate < player->board || candidate->type != 0) {
                if (player->active_cell >= player->board_end ||
                    player->active_cell->type != 0) {
                    return player->active_cell;
                }
                player->active_cell += 8;
                player->active_row++;
                player->flags2 |= PUZZLE_PLAYER_FLAG_ROTATION_KICKED;
            }
            player->rotation_state = 2;
            player->rotation_angle = 1.5707964f;
        } else {
            player->rotation_state = 0;
            player->rotation_angle = -1.5707964f;
        }
        break;

    case 1:
        if (direction == 5) {
            player->rotation_state = 0;
            player->rotation_angle = 1.5707964f;
        } else {
            candidate = player->active_cell - 8;
            if (candidate < player->board || candidate->type != 0) {
                if (player->active_cell >= player->board_end ||
                    player->active_cell->type != 0) {
                    return player->active_cell;
                }
                player->active_cell += 8;
                player->active_row++;
                player->flags2 |= PUZZLE_PLAYER_FLAG_ROTATION_KICKED;
            }
            player->rotation_state = 2;
            player->rotation_angle = -1.5707964f;
        }
        break;

    case 2:
        if (direction == 4) {
            for (;;) {
                if (player->active_column == 0 ||
                    (player->active_cell - 1)->type != 0) {
                    candidate = player->active_cell + 1;
                    if (player->active_column >= 7 ||
                        candidate > player->board_end ||
                        candidate->type != 0) {
                        candidate = player->active_cell + 8;
                        if (candidate > player->board_end ||
                            candidate->type != 0) {
                            saved_type = player->current_pair[0].type;
                            player->current_pair[0].type =
                                player->current_pair[1].type;
                            player->current_pair[1].type = saved_type;
                        } else {
                            player->rotation_state = 0;
                        }
                        return player->active_cell;
                    }
                    player->active_cell = candidate;
                    player->active_column++;
                }

                if ((player->flags2 &
                     PUZZLE_PLAYER_FLAG_ROTATION_KICKED) != 0) {
                    player->flags2 &=
                        ~PUZZLE_PLAYER_FLAG_ROTATION_KICKED;
                    if (player->active_row != 0 &&
                        player->active_cell >= player->board + 8) {
                        player->active_cell -= 8;
                        player->active_row--;
                        continue;
                    }
                }
                break;
            }
            player->rotation_state = 3;
            player->rotation_angle = -1.5707964f;
        } else {
            for (;;) {
                if (player->active_column >= 7 ||
                    (player->active_cell + 1)->type != 0) {
                    candidate = player->active_cell - 1;
                    if (player->active_column == 0 ||
                        candidate < player->board ||
                        candidate->type != 0) {
                        candidate = player->active_cell + 8;
                        if (candidate > player->board_end ||
                            candidate->type != 0) {
                            saved_type = player->current_pair[0].type;
                            player->current_pair[0].type =
                                player->current_pair[1].type;
                            player->current_pair[1].type = saved_type;
                        } else {
                            player->rotation_state = 0;
                        }
                        return player->active_cell;
                    }
                    player->active_cell = candidate;
                    player->active_column--;
                }

                if ((player->flags2 &
                     PUZZLE_PLAYER_FLAG_ROTATION_KICKED) != 0) {
                    player->flags2 &=
                        ~PUZZLE_PLAYER_FLAG_ROTATION_KICKED;
                    if (player->active_row != 0 &&
                        player->active_cell >= player->board + 8) {
                        player->active_cell -= 8;
                        player->active_row--;
                        continue;
                    }
                }
                break;
            }
            player->rotation_state = 1;
            player->rotation_angle = 1.5707964f;
        }
        break;

    case 0:
        if (direction == 4) {
            candidate = player->active_cell + 1;
            if (player->active_column >= 7 || candidate->type != 0) {
                candidate = player->active_cell - 1;
                if (player->active_column == 0 || candidate->type != 0) {
                    candidate = player->active_cell - 8;
                    if (candidate < player->board ||
                        candidate->type != 0) {
                        saved_type = player->current_pair[0].type;
                        player->current_pair[0].type =
                            player->current_pair[1].type;
                        player->current_pair[1].type = saved_type;
                    } else {
                        player->rotation_state = 2;
                    }
                    return player->active_cell;
                }
                player->active_cell = candidate;
                player->active_column--;
            }
            player->rotation_state = 1;
            player->rotation_angle = -1.5707964f;
        } else {
            candidate = player->active_cell - 1;
            if (player->active_column == 0 || candidate->type != 0) {
                candidate = player->active_cell + 1;
                if (player->active_column >= 7 ||
                    candidate > player->board_end ||
                    candidate->type != 0) {
                    candidate = player->active_cell - 8;
                    if (candidate < player->board ||
                        candidate->type != 0) {
                        saved_type = player->current_pair[0].type;
                        player->current_pair[0].type =
                            player->current_pair[1].type;
                        player->current_pair[1].type = saved_type;
                    } else {
                        player->rotation_state = 2;
                    }
                    return player->active_cell;
                }
                player->active_cell = candidate;
                player->active_column++;
            }
            player->rotation_state = 3;
            player->rotation_angle = 1.5707964f;
        }
        break;
    }

    return player->active_cell;
}

#pragma optimize_for_size on
static int puzzle_fighter_get_new_playpieces(PuzzlePlayerState* player) {
    PuzzleBoardCell* saved_pair;
    unsigned int next_type;
    int blocked;
    int index;

    do {
        saved_pair = player->current_pair;
        player->current_pair = player->next_pair;
        player->next_pair = saved_pair;
        player->rotation_state = player->next_rotation_state;

        for (index = 0; index < 2; index++) {
            next_type =
                puzzle_ctrl->piece_sequence[player->piece_sequence_index++];
            next_type &= 0xFFFF;
            if (player->piece_sequence_index >=
                puzzle_ctrl->piece_sequence_length) {
                player->piece_sequence_index = 0;
            }
            player->next_pair[index].type = next_type;
        }
        player->next_rotation_state = 0;
    } while (player->current_pair[0].type == 0);

    blocked = 0;
    player->active_cell = &player->board[96];
    for (index = 0; index < 8; index++) {
        if (player->active_cell[index].type != 0 &&
            player->active_cell[index].state == 0) {
            blocked = 1;
            break;
        }
    }
    if (blocked == 0) {
        player->active_cell += 3;
        if (player->active_cell->type != 0) {
            blocked = 1;
        }
    }

    player->flags2 &= ~0x80;
    player->flags2 &= ~0x08;
    player->flags &= ~0x10;
    player->active_row = 12;
    player->counter_drop_delay = 0;
    player->gravity_ticks = 0;
    player->drop_boost_ticks = 30;
    player->lock_ticks = 0;
    player->quick_drop_repeat = 0;
    player->active_column = 3;
    player->input_repeat_ticks = 0;
    player->rotation_angle = 0.0f;
    player->drop_flags |= 0x10;
    player->input_command = 0;

    player->pieces_since_superbreaker++;
    if ((puzzle_ctrl->flags2 & 0x80) == 0 &&
        player->pieces_since_superbreaker >= 30) {
        player->next_pair[0].type = PUZZLE_BLOCK_SUPERBREAKER;
        player->pieces_since_superbreaker = 0;
    }

    player->cleared_blocks = 0;
    player->pending_counter_drops = 0;
    player->best_chain_count = 0;
    return blocked == 0;
}
#pragma optimize_for_size reset

/* Soft ceiling: p_puzzle_fighter_fight_msg ~99.76% - float pools only. */
static float p_puzzle_fighter_fight_msg(void) {
    if (puzzle_ctrl->fight_message == 0) {
        return 0.0f;
    }

    puzzle_ctrl->fight_message->scale_x +=
        puzzle_ctrl->fight_message_scale_step;
    puzzle_ctrl->fight_message->scale_y +=
        puzzle_ctrl->fight_message_scale_step;
    puzzle_ctrl->fight_message->x -= puzzle_ctrl->fight_message_x_step;
    puzzle_ctrl->fight_message->y -= puzzle_ctrl->fight_message_y_step;

    if (puzzle_ctrl->fight_message->scale_x >
        puzzle_ctrl->fight_message_scale_limit) {
        puzzle_ctrl->fight_message_alpha -=
            puzzle_ctrl->fight_message_alpha_step;
        if (puzzle_ctrl->fight_message_alpha < 0) {
            pull_screen_obj(puzzle_ctrl->fight_message);
            destroy_screen_obj(puzzle_ctrl->fight_message);
            puzzle_ctrl->fight_message = 0;
            return 0.0f;
        }
        pfx_2d_obj_set_alpha(
            puzzle_ctrl->fight_message,
            (unsigned char)puzzle_ctrl->fight_message_alpha);
    }
    return 1.0f;
}

int puzzle_fighter_plyr_winning_big_based_on_points(void) {
    if (puzzle_ctrl->player1_score > puzzle_ctrl->player2_score + 65) {
        return 0;
    }
    if (puzzle_ctrl->player2_score > puzzle_ctrl->player1_score + 65) {
        return 1;
    }
    return 2;
}

void cleanup_minigame_system(void) {
    g_pz_fighters_engine.start_bits.startup_enabled = 0;
    memset(&g_pz_fighters_engine, 0, sizeof(g_pz_fighters_engine));
    cleanup_pz_fatality_stuff();
    puzzle_ctrl = 0;
    __mini_game_display_ctrl = 0;
}

/* Soft ceiling: load_puzzle_champion_screen ~99.49% - pool identities only. */
void load_puzzle_champion_screen(void) {
    ScreenObj* champion_a;
    ScreenObj* champion_b;
    int alpha;

    snd_req(0x1B03);
    load_art_section(0x2001E, &sec_pz_ending_champion);
    champion_a = load_named_2d_pfxobj_xy(
        0x2001E, 0x4002, puzzle_string_table.champion_a, 0, -0x40, -0x10,
        0x1E);
    champion_b = load_named_2d_pfxobj_xy(
        0x2001E, 0x4003, puzzle_string_table.champion_b, 0, 0x1C0, -0x10,
        0x1E);

    if (champion_a != 0 && champion_b != 0) {
        _mkproc_sleep_ticks = 960.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();

        alpha = 0xFF;
        do {
            pfx_2d_obj_set_alpha(champion_a, alpha);
            pfx_2d_obj_set_alpha(champion_b, alpha);
            _mkproc_sleep_ticks = 1.0f;
            ((PuzzleProcVtable*)aproc->vtbl)->sleep();
            alpha -= 2;
        } while (alpha > 0);
    }

    if (champion_a != 0) {
        destroy_screen_obj(champion_a);
    }
    if (champion_b != 0) {
        destroy_screen_obj(champion_b);
    }
}
