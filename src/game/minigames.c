#include "game/minigames.h"
#include "game/controller.h"
#include "game/game_info.h"
#include "game/ladder.h"
#include "game/settings.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/pfx2d.h"
#include "libmkparticle/texture_anim.h"
#include "math/mk_math.h"
#include "msl/msl_types.h"
#include "platform/display.h"
#include "platform/gcutils.h"
#include "platform/io.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "runtime/cam.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
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
#define PUZZLE_CELL_FLAG_MATCHED 0x80
#define PUZZLE_CELL_FLAG_BREAKING 0x40
#define PUZZLE_CELL_FLAG_AI_VISITED 0x20
#define PUZZLE_BLOCK_SUPERBREAKER 9
#define PUZZLE_BLOCK_WILDCARD 0xF000
#define PUZZLE_BOARD_CELLS 112
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

typedef struct PuzzleWagerProfile PuzzleWagerProfile;

void* memcpy(void* destination, const void* source, unsigned long size);
void* memset(void* destination, int value, unsigned long size);
unsigned int pan_snd_req(int sound_id, float pan);
int fx_by_owner(const char* name, int owner);
void* pfx_from_handle(unsigned int handle);
unsigned int snd_req(int sound_id);
void wait_for_a_sound_bank_to_load(int bank);
int random_snd_req(int group);
unsigned short randu0(int limit);
float frand(float max);
float sfrand(float max);
void push_game_state(int state);
void* pfx_get_field(void* pfx, int emitter_index, int field);
int signrand(unsigned short range);
int get_language(void);
void set_string_obj_alpha(void* object, float alpha);
extern int game_save_loop_count;
extern int winner;
extern int p1_profile_status;
extern int p2_profile_status;
extern PuzzleWagerProfile p1_profile;
extern PuzzleWagerProfile p2_profile;
extern GlobalPlayerEntry global_player_data[];
int get_current_wager_koin(void);
void init_wagering(void);
void award_koins_to_player(int player, int amount, int koin_type);
void show_koin_award(int player, int amount, int koin_type, int y);
int pz_fighter_fatality_over(void);
int pz_fighter_is_round_over(void);
void atm_reset_current_page(int page);
const char* get_string(int index);
int advance_ladder_position(void);
void ck_do_profile_save(void);
float do_continue(void);
float p_credits_screen(void);
float p_atm_loop(void);
float p_ladder_select(void);
float p_pz_pselect(void);
static void minigame_puzzlefighter_destroy(void);
void fade_to_black(int frames, int flag);
void pop_game_state(void);

typedef struct PuzzleBgndState {
    char pad00[0x18];
    int y;
} PuzzleBgndState;

typedef struct PuzzleBoardCell {
    union {
        unsigned char flags;
        struct {
            signed char matched : 1;
            signed char effect_bit : 1;
            unsigned char ai_visited : 1;
            unsigned char flag_pad : 5;
        } flag_bits;
        struct {
            unsigned char matched : 1;
            unsigned char breaking : 1;
            unsigned char pad : 6;
        } match_flag_bits;
        struct {
            signed char match_state : 1;
            signed char match_pad : 7;
        } match_bits;
        struct {
            signed char ai_state_pad_high : 2;
            signed char ai_state : 1;
            signed char ai_state_pad_low : 5;
        } ai_bits;
    }; /* +0x00 */
    char pad01[3];
    unsigned int type; /* +0x04 */
    int state; /* +0x08 - zero permits gravity movement */
    int fall_ticks; /* +0x0C */
    union {
        unsigned int visual;
        AniTextureControl* animation;
    }; /* +0x10 */
} PuzzleBoardCell; /* 0x14 */

typedef PuzzleBoardCell PuzzleBoardRow[8];

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

struct PuzzleWagerProfile {
    char pad00[0x40];
    int wager_total[18]; /* +0x40 */
    int wager_wins[6]; /* +0x88 */
    int wager_losses[6]; /* +0xA0 */
    char padB8[0x52C - 0xB8];
    PuzzleProfileStats puzzle_stats; /* +0x52C */
};

typedef struct PuzzleSupermoveBalance {
    float gain_per_block;
    int amount;
} PuzzleSupermoveBalance;

typedef struct PuzzlePlayerState {
    union {
        unsigned char flags;
        struct {
            unsigned char flags_pad_high : 1;
            unsigned char scanning_holes : 1;
            unsigned char flags_pad_mid : 1;
            unsigned char score_applied : 1;
            unsigned char superbomb_cell_phase : 2;
            unsigned char superbomb_active : 1;
            signed char board_moving : 1;
        } flag_bits;
        struct {
            signed char match_mode_pad_high : 4;
            signed char match_mode : 2;
            signed char match_mode_pad_low : 2;
        } match_mode_bits;
        struct {
            signed char low_state_pad_high : 6;
            signed char low_state : 1;
            signed char low_state_pad_low : 1;
        } low_state_bits;
        struct {
            signed char event_pad_high : 1;
            signed char floor_event : 1;
            signed char counter_drops_active : 1;
            signed char event_pad_low : 5;
        } event_bits;
        struct {
            signed char ai_controlled : 1;
            signed char drop_sound_pad_high : 2;
            signed char quick_drop_sound : 1;
            signed char drop_sound_pad_low : 4;
        } drop_control_bits;
        struct {
            signed char ai_player : 1;
            signed char ai_player_pad : 7;
        } ai_player_bits;
        struct {
            signed char profile_ineligible : 1;
            signed char profile_pad : 7;
        } profile_bits;
    }; /* +0x00 */
    union {
        unsigned char flags2;
        struct {
            unsigned char supermove_active : 1;
            unsigned char collapse_pending : 1;
            unsigned char flags2_mid_pad : 2;
            unsigned char new_piece_latch : 1;
            unsigned char board_shift_active : 1;
            unsigned char flags2_low_pad : 2;
        } flags2_bits;
        struct {
            signed char motion_pad_high : 5;
            signed char board_motion_state : 1;
            signed char motion_pad_low : 2;
        } flags2_motion_bits;
        struct {
            signed char rotation_pad_high : 4;
            signed char rotation_kicked : 1;
            signed char rotation_pad_low : 3;
        } flags2_rotation_bits;
        struct {
            signed char chain_resolved : 1;
            signed char collapse_pending : 1;
            signed char event_pad : 6;
        } flags2_event_bits;
        struct {
            signed char mode_pad_high : 2;
            signed char score_applied : 1;
            signed char new_piece : 1;
            signed char mode_pad_mid : 2;
            signed char new_piece_latch : 1;
            signed char counter_active : 1;
        } flags2_mode_bits;
    }; /* +0x01 */
    union {
        unsigned char flags3;
        struct {
            signed char hide_active_piece : 1;
            signed char counter_drops_active : 1;
            unsigned char flags3_pad : 6;
        } flags3_bits;
    }; /* +0x02 */
    char pad03;
    union {
        unsigned char flags4;
        struct {
            unsigned char flags4_pad_high : 1;
            unsigned char drop_active : 1;
            unsigned char flags4_pad_low : 6;
        } flags4_bits;
        struct {
            signed char counter_pad_high : 3;
            signed char force_counter_drops : 1;
            signed char counter_pad_low : 4;
        } flags4_counter_bits;
    }; /* +0x04 */
    char pad05[3];
    PuzzleBoardCell* active_cell; /* +0x08 - falling-pair pivot */
    char pad0C[4];
    union {
        PuzzleBoardCell* board;
        PuzzleBoardRow* board_rows;
    }; /* +0x10 - 8 columns x 14 rows */
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
            signed char supermove_done : 1;
            unsigned char drop_pad_low : 4;
        } drop_bits;
        struct {
            signed char timed_drop_pad_high : 5;
            signed char timed_drop_initialized : 1;
            signed char timed_drop_pad_low : 2;
        } timed_drop_bits;
        struct {
            signed char ai_plan_pad_high : 4;
            signed char ai_match_bonus_pending : 1;
            signed char ai_plan_pad_low : 3;
        } ai_plan_bits;
    }; /* +0x50 */
    char pad51[3];
    int ai_move; /* +0x54 */
    int ai_fallback_move; /* +0x58 */
    int ai_no_pause_band; /* +0x5C - selects row-11 clear window */
    int ai_field_60;
    int ai_horizontal_sequence_index; /* +0x64 */
    int ai_rotation_sequence_index; /* +0x68 */
    int ai_drop_sequence_index; /* +0x6C */
    int ai_pause_ticks; /* +0x70 */
    int ai_pause_base; /* +0x74 */
    char pad78[0x400];
    int ai_target_rotation; /* +0x478 */
    int ai_target_column; /* +0x47C */
    int ai_secondary_rotation; /* +0x480 */
    int ai_secondary_column; /* +0x484 */
    int pfx_slot; /* +0x488 */
    int input_command; /* +0x48C */
    int previous_input_command; /* +0x490 */
    int (*mode_step)(struct PuzzlePlayerState* player,
                     struct PuzzlePlayerState* opponent);
    int (*saved_mode_step)(struct PuzzlePlayerState* player,
                           struct PuzzlePlayerState* opponent);
    int match_delay; /* +0x49C */
    float sound_pan; /* +0x4A0 */
    int event_player; /* +0x4A4 */
    int winner_character_id; /* +0x4A8 */
    PlyrInfo* input_player; /* +0x4AC */
    int field_4B0;
    int largest_counter_drop; /* +0x4B4 */
    int largest_chain; /* +0x4B8 */
    int earned_supermoves; /* +0x4BC */
    float round_end_time; /* +0x4C0 - seconds from gameplay start */
    int character_id; /* +0x4C4 */
    int final_score; /* +0x4C8 */
    int mode_result; /* +0x4CC */
    PuzzleProfileStats* profile_stats; /* +0x4D0 */
    union {
        unsigned int center_event_word;
        struct {
            union {
                unsigned char center_event_flags;
                struct {
                    signed char high : 1;
                    signed char low : 1;
                    signed char split : 1;
                    unsigned char pad : 5;
                } center_event_bits;
            };
            char center_event_pad[3];
        };
    }; /* +0x4D4 */
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
    int equipped_supermove; /* +0x518 */
    int selected_supermove; /* +0x51C - table index while move is executing */
    int supermove_state; /* +0x520 */
    int supermove_amount; /* +0x524 */
    int supermove_phase_ticks; /* +0x528 */
    int supermove_delay_ticks; /* +0x52C */
    void (*supermove_cleanup)(void); /* +0x530 */
    int event_cooldown; /* +0x534 */
    int invisibility_fade; /* +0x538 */
    int invisibility_ticks; /* +0x53C */
    char pad540[4];
    MkProc* block_count_message_proc; /* +0x544 */
    ScreenObj* result_portrait; /* +0x548 */
    int round_result_value; /* +0x54C */
    StringObj* block_count_font_11; /* +0x550 */
    StringObj* block_count_font_10; /* +0x554 */
    int block_count_anim_value; /* +0x558 */
    int block_count_anim_timer; /* +0x55C */
} PuzzlePlayerState;

typedef struct PuzzleAiData {
    PuzzlePlayerState* player;
    int breaker_count; /* +0x04 - board types 4..7 */
    int highest_occupied_row; /* +0x08 */
    int normal_block_count; /* +0x0C - colors/wildcards; retail does not clear */
    union {
        unsigned int word;
        struct {
            signed char has_superbreaker : 1;
            signed char has_breaker : 1;
            unsigned char pad_flags : 6;
            unsigned char pad11[3];
        } bits;
    } flags; /* +0x10 */
} PuzzleAiData;

typedef struct PuzzleAiColorCount {
    unsigned int color;
    int count;
} PuzzleAiColorCount;

typedef struct PuzzleAiPlacement {
    int match_score;
    int row;
} PuzzleAiPlacement;

typedef struct PuzzleAiLayoutColumn {
    int rotation;
    int column;
} PuzzleAiLayoutColumn;

typedef struct PuzzleInvisiblePdata {
    void* vtbl;
    unsigned int instance;
    PuzzlePlayerState* player;
} PuzzleInvisiblePdata;

typedef struct PuzzleMessagePdata {
    MkHdr hdr;
    unsigned char alpha; /* +0x08 */
    unsigned char alpha_step; /* +0x09 */
    char pad0A[2];
    int lifetime_ticks;
    int fade_start_ticks;
    int fade_delay_ticks;
    int velocity_x; /* +0x18 */
    int velocity_y;
    int acceleration; /* +0x20 */
    StringObj* text; /* +0x24 */
    unsigned int text_instance; /* +0x28 */
    ScreenObj* secondary_image; /* +0x2C */
    unsigned int secondary_instance; /* +0x30 */
    ScreenObj* primary_image; /* +0x34 */
    unsigned int primary_instance; /* +0x38 */
    PuzzlePlayerState* player; /* +0x3C */
} PuzzleMessagePdata; /* 0x40 */

typedef struct PuzzleLocalizedImagePlacement {
    int player1_x;
    int player2_x;
    int y;
    int setup_y;
} PuzzleLocalizedImagePlacement;

typedef struct PuzzleLocalizedImageEntry {
    int field_00;
    unsigned int image_id;
    PuzzleLocalizedImagePlacement language[5];
} PuzzleLocalizedImageEntry; /* 0x58 */

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
    char* message_art; /* +0x14 */
    MkFileInfo* art_section; /* +0x18 */
    int message_sound; /* +0x1C */
} PuzzleSuperMoveEntry; /* 0x20 */

typedef struct PuzzleRainDanceData {
    int phase; /* +0x00 */
    int rain_target_y; /* +0x04 */
    AniTextureControl* rain_anim; /* +0x08 */
    AniTextureControl* splash_anim; /* +0x0C */
    ScreenObj* rain_object; /* +0x10 */
    ScreenObj* splash_object; /* +0x14 */
    union {
        unsigned char flags;
        struct {
            signed char holes_filling : 1;
            unsigned char flags_pad : 7;
        } flag_bits;
    }; /* +0x18 */
    char pad19[3];
} PuzzleRainDanceData; /* 0x1C */

typedef struct PuzzleEdgeClearData {
    int phase; /* +0x00 - 1 closing, 2 opening */
    ScreenObj* edge_objects[2]; /* +0x04 */
    int step_count; /* +0x0C */
} PuzzleEdgeClearData; /* 0x10 */

typedef struct PuzzleDrillData {
    AniTextureControl* drill_anim; /* +0x00 */
    ScreenObj* drill_object; /* +0x04 */
    unsigned int sound_handles[2]; /* +0x08 */
} PuzzleDrillData; /* 0x10 */

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
    int* sequence; /* +0x08 */
} PuzzleFeedRandomMessage;

typedef struct PuzzleNetworkArrayEntry {
    int* sequence;
    int field_04;
} PuzzleNetworkArrayEntry;

typedef struct PuzzleArcadeBackground {
    int field_00;
    int background_data;
} PuzzleArcadeBackground;

typedef struct PuzzlePfxView {
    char pad00[0x54];
    int particle_count;
} PuzzlePfxView;

typedef struct PuzzleStormEmitter {
    char pad00[0x1C];
    union {
        unsigned char flags;
        struct {
            unsigned char disabled : 1;
            unsigned char flags_pad : 7;
        } flag_bits;
    };
    char pad1D[0x2CB];
    void* transform; /* +0x2E8 */
} PuzzleStormEmitter;

typedef struct PuzzleStormTransform {
    char pad00[0x30];
    float x; /* +0x30 */
} PuzzleStormTransform;

typedef struct PuzzleBlockOffset {
    float x;
    float y;
} PuzzleBlockOffset;

typedef struct PuzzlePfx2dElements {
    int field_00;
    int field_214;
    int field_A0;
    int pad0C[3];
    float billboard_size;
    float color[4];
    int emitter_lifetime;
    int particle_capacity;
    int field_34;
    unsigned int texture_id;
    int texture_width;
    int frame_width;
    int frame_height;
    int frame_count;
    int field_4C;
    int emitter_enabled;
    int pad54[6];
    int field_70;
    int pad74[3];
} PuzzlePfx2dElements;

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
        struct {
            unsigned char input_pad_high : 3;
            signed char accept_input : 1;
            signed char input_latched : 1;
            unsigned char input_pad_low : 3;
        } input_bits;
        struct {
            signed char pfx_error : 1;
            signed char pfx_secondary_error : 1;
            unsigned char pfx_pad : 6;
        } pfx_bits;
        struct {
            unsigned char motion_pad_high : 5;
            signed char particle_motion_active : 1;
            unsigned char motion_pad_low : 2;
        } motion_bits;
        struct {
            signed char round_pad_high : 1;
            signed char round_started : 1;
            signed char round_pad_low : 6;
        } round_bits;
        struct {
            unsigned char start_pad_high : 7;
            signed char start_initialized : 1;
        } start_bits;
    }; /* +0x00 */
    union {
        unsigned char flags2;
        struct {
            signed char network_sequence_marker : 1;
            unsigned char flags2_pad : 7;
        } flags2_bits;
        struct {
            unsigned char pfx2_pad_high : 1;
            signed char copy_ice_particles : 1;
            unsigned char pfx2_pad_low : 6;
        } pfx_bits2;
    }; /* +0x01 */
    char pad02[2];
    MkProc* mode_proc; /* +0x04 */
    int gameplay_ticks; /* +0x08 */
    int speedup_ticks; /* +0x0C */
    int match_delay; /* +0x10 */
    int supermove_phase_ticks; /* +0x14 */
    int puzzle_music_channel; /* +0x18 */
    float puzzle_music_target_volume; /* +0x1C */
    float puzzle_music_volume; /* +0x20 */
    float fight_message_scale_step; /* +0x24 */
    float fight_message_scale_limit; /* +0x28 */
    int fight_message_x_step; /* +0x2C */
    int fight_message_y_step; /* +0x30 */
    int fight_message_alpha_step; /* +0x34 */
    int particle_effect_ticks; /* +0x38 */
    float particle_effect_distance; /* +0x3C */
    int timed_match_ticks; /* +0x40 */
    char pad44[4];
    MkPtr* fight_message_processes; /* +0x48 */
    PuzzleBoardCell* boards[2]; /* +0x4C */
    PuzzlePlayerState* players[2];
    PuzzleBoardCell* current_pairs[2]; /* +0x5C */
    PuzzleBoardCell* next_pairs[2]; /* +0x64 */
    FighterRuntimeData* winner_runtime_data; /* +0x6C */
    PlyrPdata* losing_fighter; /* +0x70 */
    int winner_character_id; /* +0x74 */
    PuzzlePfxView* puzzle_pfx; /* +0x78 */
    int* puzzle_particle_capacity; /* +0x7C */
    Vec* particle_positions; /* +0x80 */
    float* particle_timers; /* +0x84 */
    int particle_position_stride; /* +0x88 */
    int particle_timer_stride; /* +0x8C */
    PuzzlePfxView* ice_pfx; /* +0x90 */
    int* ice_particle_capacity; /* +0x94 */
    Vec* ice_positions; /* +0x98 */
    float* ice_timers; /* +0x9C */
    int ice_position_stride; /* +0xA0 */
    int ice_timer_stride; /* +0xA4 */
    ScreenObj** ui_objects; /* +0xA8 */
    int ui_object_count; /* +0xAC */
    union {
        unsigned int* block_visuals;
        AniTextureControl** block_animations;
    }; /* +0xB0 */
    union {
        unsigned int* breaker_visuals;
        ScreenObj** breaker_objects;
    }; /* +0xB4 */
    int block_visual_count; /* +0xB8 */
    ScreenObj* round_objects[13]; /* +0xBC */
    ScreenObj* victory_objects[4]; /* +0xF0 */
    union {
        ScreenObj* hud_objects[3];
        struct {
            ScreenObj* super_bar_objects[2];
            ScreenObj* hud_overlay;
        };
    }; /* +0x100 */
    union {
        struct {
            PuzzleBgndState* player1_bgnd;
            PuzzleBgndState* player2_bgnd;
        };
        ScreenObj* supermove_fade_objects[2]; /* +0x10C */
    };
    ScreenObj* fight_message; /* +0x114 */
    StringObj* result_message; /* +0x118 */
    int fight_message_alpha; /* +0x11C */
    PuzzleBlockOffset* block_offsets; /* +0x120 */
    char pad124[4];
    StringObj* score_text[2]; /* +0x128 */
    int player1_score; /* +0x130 */
    int player2_score; /* +0x134 */
    StringObj* result_text[2]; /* +0x138 */
    StringObj* footer_text[2]; /* +0x140 */
    int* piece_sequence; /* +0x148 */
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
    union {
        unsigned char flags;
        struct {
            unsigned char matched_latch : 1;
            unsigned char flags_pad : 7;
        } flag_bits;
        struct {
            signed char match_state : 1;
            signed char match_state_pad : 7;
        } match_bits;
    };
    char pad19[3];
} PuzzleMatchContext;

typedef struct PuzzleArtPlacement {
    unsigned int image_id;
    int x;
    int y;
} PuzzleArtPlacement;

typedef struct PuzzleWiffResource {
    unsigned int image_id;
    float frame_rate;
} PuzzleWiffResource;

typedef struct PuzzleStaticArt {
    union {
        PuzzleArtPlacement placements[14];
        struct {
            char pad00[0x40];
            int player1_score_x;
            int player1_score_y;
            char pad48[4];
            int player2_score_x;
            int player2_score_y;
        };
    };
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

typedef struct PuzzleFighterSelectionView {
    char pad00[0xA8];
    int supermove_index; /* +0xA8 */
} PuzzleFighterSelectionView;

typedef struct PuzzleFighterView {
    char pad00[0x6F8];
    PuzzleFighterSelectionView* selection; /* +0x6F8 */
} PuzzleFighterView;

typedef struct PuzzleMainPlyrView {
    char pad00[0x14];
    union {
        unsigned char flags_14;
        struct {
            signed char special_character : 1;
            signed char random_palette_lock : 1;
            signed char flags_14_pad : 6;
        } character_bits;
    };
    char pad15[0x3F];
    int character_id;
    PuzzleFighterView* fighter;
    char pad5C[0x10];
} PuzzleMainPlyrView; /* 0x6C */

typedef struct PuzzleMainGameView {
    union {
        unsigned char flags;
        struct {
            unsigned char startup_wait : 1;
            unsigned char flags_pad_high : 1;
            signed char round_active : 1;
            unsigned char flags_pad_mid : 3;
            signed char first_pass_started : 1;
            unsigned char flags_pad_low : 1;
        } startup_bits;
    };
    char pad01[3];
    unsigned char feature_flags;
    char pad05[0x9F];
    PuzzleMainPlyrView players[2];
} PuzzleMainGameView;

static inline PuzzleMainGameView* puzzle_game_view(void) {
    return (PuzzleMainGameView*)&g_game_info;
}

typedef struct PuzzleScriptView {
    char pad00[0x58];
    unsigned int data_table_index;
} PuzzleScriptView;

#define PUZZLE_STRING_DATA                                                    \
    "puzzle_kombat.mko\0" "pause_menu/pause_menu\0" "%d00\0" "0\0"      \
    "pz_sm_storm\0" "%d\0" "minigames.c-puzzle_blocks\0"                \
    "minigames.c-ice_blocks\0" "PZ_CHAMPION_A\0" "PZ_CHAMPION_B"

enum PuzzleStringOffset {
    PUZZLE_SCRIPT_STRING = 0,
    PUZZLE_PAUSE_MENU_STRING =
        PUZZLE_SCRIPT_STRING + sizeof("puzzle_kombat.mko"),
    PUZZLE_SCORE_FORMAT_STRING =
        PUZZLE_PAUSE_MENU_STRING + sizeof("pause_menu/pause_menu"),
    PUZZLE_ZERO_STRING =
        PUZZLE_SCORE_FORMAT_STRING + sizeof("%d00"),
    PUZZLE_STORM_EFFECT_STRING =
        PUZZLE_ZERO_STRING + sizeof("0"),
    PUZZLE_NUMBER_FORMAT_STRING =
        PUZZLE_STORM_EFFECT_STRING + sizeof("pz_sm_storm"),
    PUZZLE_BLOCKS_STRING =
        PUZZLE_NUMBER_FORMAT_STRING + sizeof("%d"),
    PUZZLE_ICE_BLOCKS_STRING =
        PUZZLE_BLOCKS_STRING + sizeof("minigames.c-puzzle_blocks"),
    PUZZLE_CHAMPION_A_STRING =
        PUZZLE_ICE_BLOCKS_STRING + sizeof("minigames.c-ice_blocks"),
    PUZZLE_CHAMPION_B_STRING =
        PUZZLE_CHAMPION_A_STRING + sizeof("PZ_CHAMPION_A"),
};

extern const char puzzle_strings[];
#define PUZZLE_STRINGS puzzle_strings

int puzzle_mode_net;
static unsigned int g_fatality_sound;
static PuzzleNetworkArrayEntry* puzzle_array_table_local;
static MslSoundHandle g_puzzle_music;
int is_pz_net_master;
int pz_loss_in_a_row;
int __mini_game_display_ctrl;
static PuzzleControl* puzzle_ctrl;
extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleSwitchPdata* switch_pdata;
extern int game_tick_ctr;
extern MkFileInfo sec_pz_bgnd_beetlelair;
extern MkFileInfo sec_pz_bgnd_hellsfoundry;
extern MkFileInfo sec_pz_bgnd_lukangtomb;
extern MkFileInfo sec_pz_bgnd_skytemple;
extern MkFileInfo sec_pz_bgnd_slaughterhouse;
extern MkFileInfo sec_pz_bgnd_yingyang;
extern MkFileInfo sec_pz_plyr_bbuster;
extern MkFileInfo sec_pz_plyr_arrange;
extern MkFileInfo sec_pz_plyr_clearblue;
extern MkFileInfo sec_pz_plyr_cleargreen;
extern MkFileInfo sec_pz_plyr_clearred;
extern MkFileInfo sec_pz_plyr_clearyellow;
extern MkFileInfo sec_pz_plyr_doubleb;
extern MkFileInfo sec_pz_plyr_drill;
extern MkFileInfo sec_pz_plyr_edger;
extern MkFileInfo sec_pz_plyr_levitate;
extern MkFileInfo sec_pz_plyr_freeze;
extern MkFileInfo sec_pz_plyr_invisible;
extern MkFileInfo sec_pz_plyr_jumble;
extern MkFileInfo sec_pz_plyr_kollapse;
extern MkFileInfo sec_pz_plyr_storm;
extern MkFileInfo sec_pz_plyr_stack;
extern MkFileInfo sec_pz_plyr_invincable;
extern int* pz_ai_move_lateral_tbl[6];
extern int* pz_ai_move_rotate_tbl[6];
extern int* pz_ai_move_down_tbl[6];
extern int chain_sound_burst_table[5];
char temp_80_char[0x50];
static PuzzleStartMessage __pz_start_msg;
static PuzzleFeedRandomMessage __pz_feed_rand_msg;
static PuzzleFighterEvent pz_event;
static PuzzleDrillData pzsm_drill_data;
static PuzzleEdgeClearData pzsm_edger_data;
static PuzzleRainDanceData pzsm_raindance_data;
extern unsigned int df_press_start_proc_item[2];
extern unsigned int df_press_start_item[2];
extern PuzzleSuperMoveCheck pz_ai_super_move_table[];
extern PuzzleSuperMoveEntry pz_super_move_table[];
extern int screen_width;
extern int screen_height;
extern MkFileInfo sec_pz_ending_champion;
extern MkFileInfo sec_puzzlefighter;
extern MkFileEntry gameart_file_table[];
extern MkFileEntry puzzlefighter_file_table[];
extern MkFileInfo sec_fightingart;
extern MkFileInfo sec_pz_plyr_art;
extern void* bgnd_light_list;
extern void* plyr_light_list;
extern int b_game_timer_off;
extern int round_winner;
extern PuzzleLocalizedImageEntry pzlang_image_table[];

static void puzzle_fighter_display_floor_msg(PuzzlePlayerState* player,
                                             int fixed_message);
static int puzzle_fighter_match_above_below(PuzzleMatchContext* context);
static int puzzle_fighter_match_left_right(PuzzleMatchContext* context);
static int puzzle_fighter_fill_holes(PuzzlePlayerState* player);
static int puzzle_fighter_find_match(PuzzlePlayerState* player);
static int puzzle_fighter_get_new_playpieces(PuzzlePlayerState* player);
static void pz_preinit_world(int network_setup);
static void pz_update_plyr_profile_status(void);
static float pz_init_network_array(void);
static float p_puzzle_fighter_real_one(void);
static float p_pz_mode_clear(void);
static float p_puzzle_fighter_chain_msg(void);
static void minigame_puzzlefighter_setup(void);
void bleed_startup(void);
void setup_sound_banks(int bank);
void display_load_meter(void);
void load_lights(void* lights, void** list);
void init_weapon_trail_light_list(void);
int is_char_locked(int character, int player);
void resolve_alternate_palettes(PlyrInfo* player);
void start_plyrs(void);
void round_init(void);
float p_puzzle_game_camera_proc(void);
static PuzzleBoardCell*
puzzle_fighter_rotate_drop_pieces(int direction,
                                  PuzzlePlayerState* player);
static int puzzle_fighter_mode_play__drop_sequence(PuzzlePlayerState* player,
                                                   PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__new_piece(PuzzlePlayerState* player,
                                               PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__counter_drops(PuzzlePlayerState* player,
                                                   PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__supermove(PuzzlePlayerState* player,
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
static int puzzle_fighter_mode_play__supermove_sleep(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__collapse_holes(PuzzlePlayerState* player,
                                                    PuzzlePlayerState* opponent);
static int puzzle_fighter_mode_play__breakers(PuzzlePlayerState* player,
                                              PuzzlePlayerState* opponent);
static int pzsm_ai_raise_up(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent);
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
static int puzzle_fighter_match_left_right__ai(PuzzleMatchContext* context);
static int puzzle_fighter_match_above_below__ai(PuzzleMatchContext* context);
static int puzzle_fighter_mode_play__supermove_im_dead(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent);
static int pzsm_invincible(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_kancel(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent);
static void pzsm_rain_dance_cleanup(void);

void minigame_set_bgnd_y_value(int player1_y, int player2_y) {
    if (puzzle_ctrl == 0) {
        return;
    }
    puzzle_ctrl->player1_bgnd->y = player1_y;
    puzzle_ctrl->player2_bgnd->y = player2_y;
}

static int pzsm_lower_down(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_raise_up(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent);
static int pzsm_rain_dance(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static void pzsm_edger_cleanup(void);
static void pzsm_drill_cleanup(void);
static int pzsm_double_bomb(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent);
static int pzsm_invisible(PuzzlePlayerState* player,
                          PuzzlePlayerState* opponent);
static int pzsm_jumble(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent);
static int pzsm_freeze(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent);
static int pzsm_float(PuzzlePlayerState* player,
                      PuzzlePlayerState* opponent);
static int pzsm_edge_clear(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent);
static int pzsm_drill(PuzzlePlayerState* player,
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
static int pzsm_arrange(PuzzlePlayerState* player,
                        PuzzlePlayerState* opponent);
static int pzsm_antibreakers(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent);
static void puzzle_fighter_display_chain_msg(PuzzlePlayerState* player);
static void puzzle_fighter_display_block_count_msg(
    PuzzlePlayerState* player);
static void puzzle_fighter_calc_center_weight(PuzzlePlayerState* player);
static float p_pz_mode_exit(void);
static float p_pz_mode_who_won(void);
static float p_pz_mode_fill(void);
static float p_pz_mode_play(void);
static void pzsm_ai_get_data(PuzzleAiData* data);
static float p_pzsm_invisible(void);
int check_switch(int pad, int switch_id);
static int pz_ai_decide_move(PuzzlePlayerState* player);
static void pz_ai_decide_match(PuzzlePlayerState* player);
static void pz_ai_decide_quick_drop_tower(PuzzlePlayerState* player);
static int pz_ai_decide_superbomb(PuzzlePlayerState* player);

void minigame_get_bgnd_y_value(int* player1_y, int* player2_y) {
    if (puzzle_ctrl == 0) {
        return;
    }
    *player1_y = puzzle_ctrl->player1_bgnd->y;
    *player2_y = puzzle_ctrl->player2_bgnd->y;
}

static int pz_ai_match_precalc(PuzzlePlayerState* player,
                               PuzzleAiPlacement placements[2][8]);
static PuzzleMessagePdata*
pz_display_supermove_msg_sideways(PuzzlePlayerState* player);
void set_snd_vol(MslSoundHandle sound, int channel, float volume);
void reset_effect(const char* effect_name);
void pfx_2d_obj_set_alpha(ScreenObj* object, unsigned char alpha);
void snd_stop(unsigned int sound_handle);
int sprintf(char* destination, const char* format, ...);
void destroy_string_obj(StringObj* string_object);
void pull_string_obj(StringObj* string_object);
StringObj* string_right_xy(int font, int alignment, const char* text, int x,
                           int y, int color);
void cleanup_pz_fatality_stuff(void);
void* get_data_table(ScriptSlot* slot, unsigned int index);
int get_blood_level(void);
int get_puzzle_rounds_to_win(void);
void free_mem(void* memory);
static float p_pz_mode_start(void);
static float p_puzzle_music_fade(void);
static int puzzle_fighter_mode_start(int message);
static int init_pz_pfx_2d(void);
static float p_pzpfx_copy_data(void);
static float p_pzpfx_copy_iceblock_data(void);
static void puzzle_fighter_mode_clear(void);
static void pzpfx_copy_playpieces(PuzzlePlayerState* player);
static void pzpfx_copy_puzzleblocks(PuzzlePlayerState* player);

PuzzleStaticArt art_puzzle_fighter_static_tbl = {{
    {0x08020013, 236, 363}, {0x0802000D, 45, 114},
    {0x0802000D, 385, 114}, {0x08020012, 262, 309},
    {0x08020012, 345, 309}, {0x08020015, 257, 262},
    {0x08020015, 324, 262}, {0x08020014, -1, -1},
    {0x08020014, -2, -2}, {0x08020011, -1, -1},
    {0x08020011, -2, -2}, {0x08020016, 265, 131},
    {0x08020016, 348, 131}, {0, 0, 0},
}};

PuzzleLocalizedImageEntry pzlang_image_table[10] = {
    {0, 0x08020011, {{258,341,346,366},{258,330,346,366},{258,329,346,366},{258,330,346,366},{258,332,346,366}}},
    {1, 0x08020014, {{258,338,267,287},{258,327,267,287},{258,326,267,287},{258,338,267,287},{258,338,267,287}}},
    {2, 0x08020028, {{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139}}},
    {3, 0x0802002B, {{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139}}},
    {4, 0x0802002F, {{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139},{70,410,119,139}}},
    {5, 0x0802000E, {{0}}}, {6, 0x0802000F, {{0}}},
    {7, 0x08020035, {{40,380,275,295},{40,375,275,295},{80,425,275,295},{35,375,275,295},{35,375,275,295}}},
    {8, 0x08020030, {{40,380,275,295},{40,375,275,295},{40,370,275,295},{40,390,275,295},{30,375,275,295}}},
    {0},
};

PuzzleArtPlacement art_puzzle_fighter_super_bar_tbl[4] = {
    {0x08020019,3,11},{0x0802001A,54,11},{0x08020018,0,0},{0,0,0},
};
MkFileInfo* pz_bgnd_ss_tbl[8] = {
    &sec_pz_bgnd_beetlelair,&sec_pz_bgnd_hellsfoundry,
    &sec_pz_bgnd_lukangtomb,&sec_pz_bgnd_skytemple,
    &sec_pz_bgnd_slaughterhouse,&sec_pz_bgnd_yingyang,
    &sec_pz_ending_champion,0,
};
int pz_map_arcade_bgnd[23] = {
    0,3,2,1,3,4,4,5,1,2,0,2,2,1,5,5,3,0,3,4,1,5,-1,
};
unsigned int art_puzzle_fighter_bgnd_tbl[15] = {
    0x08170000,0x08170001,0x08180000,0x08180001,0x08190000,
    0x08190001,0x081B0000,0x081B0001,0x081C0000,0x081C0001,
    0x081D0000,0x081D0001,0x033D0000,0x033D0001,0,
};
PuzzleArtPlacement art_puzzle_fighter_victory_tbl[5] = {
    {0x0802001C,257,434},{0x0802001C,366,434},{0x0802001C,274,434},
    {0x0802001C,350,434},{0,0,0},
};
PuzzleWiffResource art_puzzle_fighter_wiff_tbl[6] = {
    {0x08020004,1.0f},{0x08020003,1.0f},{0x08020002,1.0f},
    {0x08020001,1.0f},{0x08020000,1.0f},{0,0.0f},
};
unsigned int art_puzzle_fighter_pieces_tbl[6] = {
    0x08020036,0x08020010,0x08020017,0x0802000E,0x0802000F,0,
};
unsigned int art_puzzle_fighter_chains_tbl[12] = {
    0x08020029,0x0802002A,0x0802001F,0x08020020,0x08020021,0x08020022,
    0x08020023,0x08020024,0x08020025,0x08020026,0x08020027,0,
};
unsigned int art_puzzle_fighter_messages_tbl[11] = {
    0x08020031,0x08020032,0x08020033,0x08020034,0x0802002C,0x0802001B,
    0x08020035,0x08020030,0x0802002E,0x0802002D,0,
};
PuzzlePieceLayout puzzle_piece_layout[4] = {{0,1},{1,0},{0,-1},{-1,0}};
Vec puzzle_2d_smooth[4] = {
    {0.0f,0.0f,-1.0f},{-1.0f,0.0f,0.0f},
    {0.0f,0.0f,1.0f},{1.0f,0.0f,0.0f},
};
static int ai_state_type[14] = {3,3,3,3,3,1,3,0,0,2,2,3,2,2};
static int demo_ai_state_type[5] = {3,0,3,2,3};
int pz_ai_move_lateral_0[9] = {0,1,0,3,0,1,0,1,4};
int pz_ai_move_lateral_2[5] = {1,0,1,1,4};
int pz_ai_move_lateral_4[7] = {1,1,0,1,1,0,4};
int pz_ai_move_lateral_5[4] = {1,1,1,4};
int pz_ai_move_down_0[3] = {0,1,4};
int pz_ai_move_down_2[6] = {0,1,0,0,2,4};
int pz_ai_move_down_4[5] = {0,0,0,2,4};
int pz_ai_move_down_5[4] = {1,0,2,4};
int* pz_ai_move_lateral_tbl[6] = {
    pz_ai_move_lateral_0,pz_ai_move_lateral_0,
    pz_ai_move_lateral_2,pz_ai_move_lateral_2,
    pz_ai_move_lateral_4,pz_ai_move_lateral_5,
};
int* pz_ai_move_rotate_tbl[6] = {
    pz_ai_move_lateral_0,pz_ai_move_lateral_0,
    pz_ai_move_lateral_2,pz_ai_move_lateral_2,
    pz_ai_move_lateral_5,pz_ai_move_lateral_5,
};
int* pz_ai_move_down_tbl[6] = {
    pz_ai_move_down_0,pz_ai_move_down_0,pz_ai_move_down_2,
    pz_ai_move_down_2,pz_ai_move_down_4,pz_ai_move_down_5,
};
PuzzleSuperMoveCheck pz_ai_super_move_table[19] = {
    pzsm_ai_antibreakers,pzsm_ai_arrange,0,0,0,0,pzsm_ai_double_bomb,
    pzsm_ai_drill,pzsm_ai_edge_clear,pzsm_ai_float,pzsm_ai_freeze,
    pzsm_ai_invisible,pzsm_ai_jumble,pzsm_ai_lower_down,pzsm_ai_rain_dance,
    pzsm_ai_raise_up,0,0,0,
};
PuzzleSuperMoveEntry pz_super_move_table[19] = {
    {0,30,15,0x100,pzsm_antibreakers,(char*)0x08050000,&sec_pz_plyr_bbuster,0x36},
    {1,30,15,0x100,pzsm_arrange,(char*)0x08030000,&sec_pz_plyr_arrange,0x3C},
    {2,30,15,0x100,pzsm_clear_blue,(char*)0x08060000,&sec_pz_plyr_clearblue,0},
    {3,30,15,0x100,pzsm_clear_green,(char*)0x08070000,&sec_pz_plyr_cleargreen,0},
    {4,30,15,0x100,pzsm_clear_red,(char*)0x08080000,&sec_pz_plyr_clearred,0},
    {5,30,15,0x100,pzsm_clear_yellow,(char*)0x08090000,&sec_pz_plyr_clearyellow,0},
    {6,30,15,0x100,pzsm_double_bomb,(char*)0x080A0000,&sec_pz_plyr_doubleb,0x3E},
    {7,30,15,0x100,pzsm_drill,(char*)0x080B0001,&sec_pz_plyr_drill,0x3B},
    {8,30,15,0x100,pzsm_edge_clear,(char*)0x080C0000,&sec_pz_plyr_edger,0x3A},
    {9,30,15,0x100,pzsm_float,(char*)0x08110001,&sec_pz_plyr_levitate,0x39},
    {10,30,15,0x100,pzsm_freeze,(char*)0x080D0000,&sec_pz_plyr_freeze,0x37},
    {11,30,15,0x100,pzsm_invisible,(char*)0x080E0000,&sec_pz_plyr_invisible,0x3D},
    {12,30,15,0x100,pzsm_jumble,(char*)0x080F0000,&sec_pz_plyr_jumble,0x41},
    {13,30,15,0x100,pzsm_lower_down,(char*)0x08100000,&sec_pz_plyr_kollapse,0x40},
    {14,30,15,0x100,pzsm_rain_dance,(char*)0x08130002,&sec_pz_plyr_storm,0x38},
    {15,30,15,0x100,pzsm_raise_up,(char*)0x08120000,&sec_pz_plyr_stack,0x3F},
    {16,30,15,0x200,pzsm_kancel,(char*)0x08150000,&sec_pz_plyr_invincable,0},
    {17,30,15,0x200,pzsm_invincible,(char*)0x08150000,&sec_pz_plyr_invincable,0},
    {0},
};
int chain_sound_burst_table[5] = {0x1B18,0x1B19,0x1B1A,0x1B1B,0x1B1C};
unsigned int g_old_warmup = 10000;
const PuzzlePfx2dElements pfx_2d_elements_tbl = {
    0, 0x202, 3, {0, 0, 0}, 27.0f,
    {255.0f, 255.0f, 255.0f, 255.0f},
    0, 0xD8, 0, 0x08020038, 0x80, 0x1B, 0x1B, 0x0F, 0, 0,
    {0, 0, 0, 0, 0, 0}, 0x0F, {0, 0, 0},
};
static const float puzzle_switch_sleep = -1.0f;
PuzzleAiLayoutColumn ai_layout_column_scheme[4][4] = {
    {{1, 1}, {1, 2}, {1, 2}, {3, 1}},
    {{3, 5}, {1, 5}, {3, 5}, {1, 6}},
    {{3, 3}, {1, 4}, {1, 3}, {3, 4}},
    {{1, 0}, {3, 7}, {0, 0}, {0, 7}},
};
static Vec puzzle_cam_start_pos = {0.0f, 1.9f, 2.85f};
static Vec puzzle_cam_start_ang = {0.1f, 3.1415927f, 0.0f};

/* Recovery in progress: a default player-zero pointer with nested player-one
 * overrides best recovers retail's shared selection island. The seven inlined
 * callbacks remain two instructions larger; their action and return tails
 * match retail. */
static inline PuzzlePlayerState*
puzzle_switch_select_player(PlyrPdata* fighter_pdata) {
    PuzzlePlayerState* player;

    player = puzzle_ctrl->players[0];
    if (fighter_pdata->plyr_num != 0) {
        if (puzzle_mode_net == 0 || is_pz_net_master == 0) {
            player = puzzle_ctrl->players[1];
        }
    } else {
        if (puzzle_mode_net != 0 && is_pz_net_master == 0) {
            player = puzzle_ctrl->players[1];
        }
    }
    return player;
}

static inline float puzzle_switch_set_command(int command) {
    PuzzleSwitchState* switch_state;
    PuzzlePlayerState* player;

    if (__mini_game_display_ctrl != 0) {
        if (puzzle_ctrl->input_bits.accept_input != 0) {
            switch_state = switch_pdata->state;
            if (switch_state != 0 && switch_state->event == 2) {
                player = puzzle_switch_select_player(
                    switch_state->fighter_pdata);

                if (player->input_command < 6) {
                    player->input_command = command;
                }
                player->input_repeat_ticks = 0;
                puzzle_ctrl->input_bits.input_latched = 1;
            }
        }
    }
    return puzzle_switch_sleep;
}

static inline int puzzle_player_check_switch(PuzzlePlayerState* player,
                                             int switch_id) {
    if (player->drop_control_bits.ai_controlled != 0) {
        return 0;
    }
    return check_switch(player->input_player->pad_index, switch_id);
}

static inline void puzzle_profile_update_maxima(
    PuzzleProfileStats* stats, PuzzlePlayerState* player) {
    const unsigned int maximum = 999999;

    if (stats->best_chain < (unsigned int)player->largest_chain) {
        stats->best_chain = player->largest_chain;
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

static inline void puzzle_record_round_winner(PuzzlePlayerState* player) {
    ScreenObj* victory_object;
    int victory_index;
    int visible = player->flags3_bits.hide_active_piece == 0;

    player->active_cell = player->board;
    if (visible != 0) {
        player->round_result_value++;
        puzzle_ctrl->winner_character_id = player->winner_character_id;
        if (player == puzzle_ctrl->players[0]) {
            puzzle_ctrl->winner_runtime_data =
                g_game_info.plyr0.slot.pdata->runtime_data;
            puzzle_ctrl->losing_fighter = g_game_info.plyr1.slot.pdata;
            victory_index = player->round_result_value == 1 ? 0 : 2;
        } else {
            puzzle_ctrl->winner_runtime_data =
                g_game_info.plyr1.slot.pdata->runtime_data;
            puzzle_ctrl->losing_fighter = g_game_info.plyr0.slot.pdata;
            victory_index = player->round_result_value == 1 ? 1 : 3;
        }
        pull_screen_obj(puzzle_ctrl->victory_objects[victory_index]);
        destroy_screen_obj(puzzle_ctrl->victory_objects[victory_index]);
        victory_object = load_2d_pfxobj(
            0x70033, 0x601B,
            (char*)art_puzzle_fighter_messages_tbl[5], 0, 0x3E);
        if (victory_object != 0) {
            victory_object->x =
                art_puzzle_fighter_victory_tbl[victory_index].x;
            victory_object->y =
                art_puzzle_fighter_victory_tbl[victory_index].y;
            pull_screen_obj(victory_object);
            mk_append((MkHdr*)victory_object, &screen_obj_list);
            victory_object->flag_bits.bit2 = 1;
        }
    }
}

static inline int
puzzle_fighter_pair_position_open(PuzzlePlayerState* player,
                                  PuzzleBoardCell* pivot) {
    PuzzleBoardCell* cell;
    PuzzlePieceLayout* layout;
    int check_below;
    int index;

    check_below = 1;
    if (pivot == player->active_cell) {
        return 0;
    }
    layout = &puzzle_piece_layout[player->rotation_state];
    for (;;) {
        for (index = 0; index < 2; index++) {
            cell = pivot;
            if (index != 0) {
                cell += layout->column;
                cell += layout->row * 8;
            }
            if (cell < player->board_end && cell->type != 0) {
                return 0;
            }
        }
        if (check_below != 0 &&
            player->gravity_ticks > player->drop_interval) {
            check_below = 0;
            pivot += 8;
        } else {
            return 1;
        }
    }
}

static inline int
puzzle_fighter_update_invisibility(PuzzlePlayerState* player) {
    PuzzleInvisiblePdata* invisible_pdata;

    if (player->invisibility_ticks > 0 &&
        player->invisibility_ticks < 0x1000) {
        player->invisibility_ticks--;
        if (player->invisibility_ticks == 0) {
            if (_create_mkproc_generic_nostack(
                    0x6010, 0x1F, p_pzsm_invisible, 0,
                    (MkHdr**)&invisible_pdata) != 0) {
                invisible_pdata->player = player;
            } else {
                return 1;
            }
        }
    }

    if (player->invisibility_fade > 0) {
        player->invisibility_fade++;
        if (player->invisibility_fade > 14) {
            player->invisibility_fade = -1;
            return 1;
        }
    }
    return 0;
}

static inline int puzzle_ai_select_best_piece(
    PuzzleAiPlacement placements[2][8], const int best[2]) {
    if (best[0] < 0 || best[1] < 0) {
        return (unsigned int)best[0] >> 31;
    }
    return placements[0][best[0]].match_score <
           placements[1][best[1]].match_score;
}

static inline void puzzle_ai_choose_fallback(
    PuzzlePlayerState* player, int minimum_row,
    unsigned int piece0_type, unsigned int piece1_type) {
    PuzzleAiLayoutColumn* layout;
    PuzzleBoardCell* cell;
    int column;
    int row;

    if (minimum_row > 3 && minimum_row < 14) {
        player->ai_target_column = -1;
        row = 0;
        do {
            for (column = 0; column < 8; column++) {
                cell = &player->board_rows[row][column];
                if (cell->type == 0 || cell->state != 0) {
                    player->ai_target_column = column;
                    if (column > 0 && (cell - 1)->type == 0) {
                        player->ai_target_rotation = 3;
                        break;
                    }
                    if (column < 7 && (cell + 1)->type == 0) {
                        player->ai_target_rotation = 1;
                        break;
                    }
                    player->ai_target_rotation = 0;
                }
            }
            if (player->ai_target_column >= 0) {
                break;
            }
            row++;
        } while (row < 12);
        if (player->ai_target_column < 0) {
            player->ai_target_column = 3;
        }
    } else {
        if (player->event_player != 0) {
            piece0_type = 3 - piece0_type;
            piece1_type = 3 - piece1_type;
        }
        layout = &ai_layout_column_scheme[piece0_type][piece1_type];
        player->ai_target_rotation = layout->rotation;
        player->ai_target_column = layout->column;
        player->ai_secondary_rotation = 1;
        player->ai_secondary_column = 4;
    }
}

static inline int puzzle_ai_find_superbomb_column(
    PuzzlePlayerState* player, const PuzzleAiColorCount colors[4]) {
    PuzzleBoardCell* cell;
    unsigned int type;
    int color_index;
    int row;
    int column;

    for (color_index = 0; color_index < 4; color_index++) {
        for (row = 1; row < 12; row++) {
            for (column = 0; column < 8; column++) {
                cell = &player->board[row * 8 + column];
                if (cell->type != 0) {
                    continue;
                }
                type = (cell - 8)->type;
                if (type >= 4) {
                    if (type != PUZZLE_BLOCK_WILDCARD) {
                        type -= 4;
                    } else {
                        type = 0;
                    }
                }
                if (type == colors[color_index].color) {
                    return column;
                }
            }
        }
    }
    return column;
}

static inline void pzsm_release_rain_dance_resources(void) {
    reset_effect(PUZZLE_STRINGS + PUZZLE_STORM_EFFECT_STRING);
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

static inline void puzzle_rotation_fallback_down(PuzzlePlayerState* player) {
    PuzzleBoardCell* candidate = player->active_cell + 8;
    unsigned int saved_type;

    if (candidate > player->board_end || candidate->type != 0) {
        saved_type = player->current_pair[0].type;
        player->current_pair[0].type = player->current_pair[1].type;
        player->current_pair[1].type = saved_type;
    } else {
        player->rotation_state = 0;
    }
}

static inline void puzzle_rotation_fallback_up(PuzzlePlayerState* player) {
    PuzzleBoardCell* candidate = player->active_cell - 8;
    unsigned int saved_type;

    if (candidate < player->board || candidate->type != 0) {
        saved_type = player->current_pair[0].type;
        player->current_pair[0].type = player->current_pair[1].type;
        player->current_pair[1].type = saved_type;
    } else {
        player->rotation_state = 2;
    }
}

static inline void puzzle_fighter_setup_abort(void) {
    xfer_proc(puzzle_ctrl->mode_proc, p_pz_mode_exit);
}

static inline int puzzle_fighter_setup_ui_and_wiff_objects(void) {
    PuzzleWiffResource* wiff;
    int index;

    for (index = 0; index < puzzle_ctrl->ui_object_count; index++) {
        puzzle_ctrl->ui_objects[index] = load_2d_pfxobj(
            0x70033, 0x601F, (char*)art_puzzle_fighter_pieces_tbl[index], 0,
            0x3B);
        if (puzzle_ctrl->ui_objects[index] == 0) {
            return 0;
        }
        pull_screen_obj(puzzle_ctrl->ui_objects[index]);
    }

    for (index = 0; index < puzzle_ctrl->block_visual_count; index++) {
        wiff = &art_puzzle_fighter_wiff_tbl[index];
        puzzle_ctrl->breaker_objects[index] = load_wiff_screen_pfxobj(
            0x70033, wiff->image_id, 0x601F,
            &puzzle_ctrl->block_animations[index], 0, 0);
        if (puzzle_ctrl->breaker_objects[index] == 0) {
            break;
        }
        set_ani_texture_framerate(puzzle_ctrl->block_animations[index],
                                  wiff->frame_rate);
        pull_screen_obj(puzzle_ctrl->breaker_objects[index]);
        pull_ani_texture_control(puzzle_ctrl->block_animations[index]);
    }
    return index;
}

static inline int puzzle_fighter_setup_background_objects(void) {
    int background = __pz_start_msg.background;
    int index;

    load_art_section(0x70037, pz_bgnd_ss_tbl[background]);
    for (index = 0; index < 2; index++) {
        puzzle_ctrl->supermove_fade_objects[index] = load_2d_pfxobj(
            0x70037, 0x601C,
            (char*)art_puzzle_fighter_bgnd_tbl[background * 2 + index], 0,
            0x3F);
        if (puzzle_ctrl->supermove_fade_objects == 0) {
            break;
        }
        puzzle_ctrl->supermove_fade_objects[index]->flag_bits.bit2 = 1;
        puzzle_ctrl->supermove_fade_objects[index]->x = -0x40;
        if (index != 0) {
            puzzle_ctrl->supermove_fade_objects[index]->x += 0x200;
        }
        puzzle_ctrl->supermove_fade_objects[index]->y = -0x10;
    }
    return index;
}

static inline int puzzle_fighter_setup_victory_and_super_bar_objects(void) {
    PuzzleArtPlacement* placement;
    ScreenObj* object;
    int index;

    for (index = 0; art_puzzle_fighter_victory_tbl[index].image_id != 0;
         index++) {
        placement = &art_puzzle_fighter_victory_tbl[index];
        object = load_2d_pfxobj(0x70033, 0x601B,
                                (char*)placement->image_id, 0, 0x3E);
        if (object == 0) {
            return 0;
        }
        object->flag_bits.bit2 = 1;
        object->x = placement->x;
        object->y = placement->y;
        puzzle_ctrl->victory_objects[index] = object;
        if (get_puzzle_rounds_to_win() == 1 && index >= 1) {
            break;
        }
        if (index >= 4) {
            return 0;
        }
    }

    for (index = 0; art_puzzle_fighter_super_bar_tbl[index].image_id != 0;
         index++) {
        placement = &art_puzzle_fighter_super_bar_tbl[index];
        object = load_2d_pfxobj(0x70033, 0x601E,
                                (char*)placement->image_id, 0, 0x3C);
        if (object == 0) {
            break;
        }
        pull_screen_obj(object);
        puzzle_ctrl->hud_objects[index] = object;
        if (index >= 3) {
            break;
        }
    }
    return index;
}

void minigame_event(void) {
}

float puzzle_fighter_get_super_bar_level(unsigned int player) {
    PuzzlePlayerState* state;
    float level;

    level = 0.0f;
    if (puzzle_ctrl != 0) {
        if (player == 0) {
            state = puzzle_ctrl->players[0];
            level = state->super_bar / 185.0f;
            if (state->super_active > 0) {
                level = 1.0f;
            }
        } else {
            state = puzzle_ctrl->players[1];
            level = state->super_bar / 185.0f;
            if (state->super_active > 0) {
                level = 1.0f;
            }
        }
    }
    return level;
}

/* Emission-only near match (92.81%, retail 0x470/current 0x454). Retail startup
 * behavior, layouts, art loads, random-character retries, and bit writes are
 * recovered. Remaining differences are loop-local register lifetimes and
 * repeated g_game_info address formation around the art/render tail; declaring
 * the randu0 result u16 worsens both size and allocation. */
float p_puzzle_fighter(void) {
    PuzzleMainGameView* game;
    PuzzleScriptView* script;
    BgndDataTable* section;
    int character;
    int attempts;

    set_game_switch_maps();
    set_section_memory_scheme(2);
    game = puzzle_game_view();
    if ((game->feature_flags & 0x20) != 0) {
        push_game_state(3);
    } else {
        push_game_state(0x12);
    }

    load_ssf(gameart_file_table);
    load_art_section(0x10005, &sec_fightingart);
    load_ssf(puzzlefighter_file_table);
    script = (PuzzleScriptView*)cmdscript_loadfile_by_name(
        11, PUZZLE_STRINGS + PUZZLE_SCRIPT_STRING);
    g_game_info.cmdscript = (ScriptSlot*)script;
    section = (BgndDataTable*)get_data_table(
        (ScriptSlot*)script, script->data_table_index);
    g_game_info.section = section;
    g_game_info.misc = section->misc;
    if (section->misc->lights_bgnd != 0) {
        load_lights(section->misc->lights_bgnd, &bgnd_light_list);
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
    UpdateShadowCameraLightSource(g_game_info.misc->shadow_cam_light);
    ShadowStrength = g_game_info.misc->shadow_strength;
    if (g_game_info.misc->lights_plyr != 0) {
        load_lights(g_game_info.misc->lights_plyr, &plyr_light_list);
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
        puzzle_game_view()->players[0].character_id = character;
        puzzle_game_view()
            ->players[0]
            .character_bits.random_palette_lock = 0;
        if (character == 0x15) {
            puzzle_game_view()
                ->players[0]
                .character_bits.special_character = 1;
        } else {
            puzzle_game_view()
                ->players[0]
                .character_bits.special_character = 0;
        }

        attempts = 100;
        do {
            character = randu0(44);
            attempts--;
        } while (is_char_locked(character, 0) != 0 && attempts != 0);
        if (attempts == 0) {
            character = 0;
        }
        puzzle_game_view()->players[1].character_id = character;
        puzzle_game_view()
            ->players[1]
            .character_bits.random_palette_lock = 0;
        if (character == 0x15) {
            puzzle_game_view()
                ->players[1]
                .character_bits.special_character = 1;
        } else {
            puzzle_game_view()
                ->players[1]
                .character_bits.special_character = 0;
        }
        resolve_alternate_palettes(&g_game_info.plyr1);
    }

    if (puzzle_game_view()->players[0].character_id == 0x15) {
        puzzle_game_view()
            ->players[0]
            .character_bits.special_character = 1;
    } else {
        puzzle_game_view()
            ->players[0]
            .character_bits.special_character = 0;
    }
    if (puzzle_game_view()->players[1].character_id == 0x15) {
        puzzle_game_view()
            ->players[1]
            .character_bits.special_character = 1;
    } else {
        puzzle_game_view()
            ->players[1]
            .character_bits.special_character = 0;
    }
    resolve_alternate_palettes(&g_game_info.plyr1);

    start_plyrs();
    bleed_startup();
    minigame_puzzlefighter_setup();
    setup_sound_banks(5);
    load_art_section_language(
        0x70034,
        pz_super_move_table[puzzle_game_view()->players[0]
                                .fighter->selection->supermove_index]
            .art_section);
    load_art_section_language(
        0x70035,
        pz_super_move_table[puzzle_game_view()->players[1]
                                .fighter->selection->supermove_index]
            .art_section);
    load_art_section(0x70038, &sec_pz_plyr_art);
    pz_event.type = 1;
    pz_fighter_event(&pz_event);
    game->startup_bits.first_pass_started = 1;
    start_first_pass_render();
    _mkproc_sleep_ticks = 3.0f;
    ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    end_first_pass_render();
    turn_camera_off();
    while (game->startup_bits.startup_wait != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(
        p_puzzle_fighter_real_one, 0.0f);
    return 0.0f;
}

static float p_puzzle_fighter_real_one(void) {
    CameraObj* camera;
    int pause_slot;

    g_fatality_sound = 0;
    __mini_game_display_ctrl = 1;
    g_game_info.pselect.field_1f4 = 0;
    round_init();

    pz_event.type = 0x14;
    pz_event.block_count = (float)__pz_start_msg.warmup;
    pz_fighter_event(&pz_event);
    pz_event.type = 0x13;
    pz_event.block_count = (float)__pz_start_msg.background_data;
    pz_fighter_event(&pz_event);

    pause_slot = get_pause_menu_ssh();
    unload_section_slot(pause_slot);
    preload_screen_data(PUZZLE_STRINGS + PUZZLE_PAUSE_MENU_STRING, pause_slot);
    turn_camera_on();
    skip_camera_intro();

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    camera->pos.x = puzzle_cam_start_pos.x;
    camera->pos.y = puzzle_cam_start_pos.y;
    camera->pos.z = puzzle_cam_start_pos.z;
    camera->ang.x = puzzle_cam_start_ang.x;
    camera->ang.y = puzzle_cam_start_ang.y;
    camera->ang.z = puzzle_cam_start_ang.z;
    xfer_camera(p_puzzle_game_camera_proc, 1);

    turn_controllers_on();
    pz_event.type = 0xD;
    pz_fighter_event(&pz_event);
    if (puzzle_ctrl->result_text[0] != 0) {
        unhide_string_obj(puzzle_ctrl->result_text[0]);
    }
    if (puzzle_ctrl->result_text[1] != 0) {
        unhide_string_obj(puzzle_ctrl->result_text[1]);
    }
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_clear, 0.0f);
    return 0.0f;
}

/* Near match: xfer_puzzle_exit 98.55%; flag-update register coloring only. */
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

/* Near miss: complete retail exit flow and object ownership are recovered.
 * Remaining differences are one zero rematerialization and float-pool labels. */
static float p_pz_mode_exit(void) {
    PuzzlePlayerState* player0 = puzzle_ctrl->players[0];
    PuzzlePlayerState* player1 = puzzle_ctrl->players[1];
    int result0 = player0->round_result_value;
    int result1 = player1->round_result_value;
    unsigned int event_flags =
        ((unsigned int)puzzle_ctrl->flags << 26) & 0xC0000000;
    int exit_requested = (int)event_flags >> 31;

    pz_event.type = 3;
    pz_fighter_event(&pz_event);

    if (puzzle_ctrl->players[0]->result_portrait != 0) {
        pull_screen_obj(puzzle_ctrl->players[0]->result_portrait);
        destroy_screen_obj(puzzle_ctrl->players[0]->result_portrait);
        puzzle_ctrl->players[0]->result_portrait = 0;
    }
    if (puzzle_ctrl->players[1]->result_portrait != 0) {
        pull_screen_obj(puzzle_ctrl->players[1]->result_portrait);
        destroy_screen_obj(puzzle_ctrl->players[1]->result_portrait);
        puzzle_ctrl->players[1]->result_portrait = 0;
    }
    if (puzzle_ctrl->result_message != 0) {
        if (puzzle_ctrl->result_message->instance != 0) {
            puzzle_ctrl->result_message->typed_vtbl->destroy(
                puzzle_ctrl->result_message);
        }
        puzzle_ctrl->result_message = 0;
    }

    __mini_game_display_ctrl = 0;
    pz_update_plyr_profile_status();
    minigame_puzzlefighter_destroy();
    pfxsystem_widescreen_offset(0, 0);

    if (exit_requested != 0) {
        return 1000.0f;
    }
    if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
        fade_to_black(12, 1);
        pop_game_state();
        gamelogic_jump(0, p_atm_loop);
    }

    pop_game_state();
    if ((g_game_info.plyr0.player_state == 0 && result1 > result0) ||
        (g_game_info.plyr1.player_state == 0 && result0 > result1)) {
        pz_loss_in_a_row = 0;
        if (advance_ladder_position() == 0) {
            if (game_save_loop_count >= 5) {
                ck_do_profile_save();
            }
            gamelogic_jump(6, p_ladder_select);
        }
        g_game_info.pause_flag_bits.ladder_complete = 0;
        fade_to_black(8, 1);
        ck_do_profile_save();
        gamelogic_jump(10, p_credits_screen);
    }

    if (game_save_loop_count >= 5) {
        ck_do_profile_save();
    }
    if (g_game_info.plyr0.player_state == 0 ||
        g_game_info.plyr1.player_state == 0) {
        pz_loss_in_a_row++;
        ((PuzzleProcVtable*)aproc->vtbl)->transfer(do_continue, 0.0f);
        return 0.0f;
    }
    gamelogic_jump(6, p_pz_pselect);
    return 1.0f;
}

/* Recovered retail endgame flow. Remaining bounded work is source shaping for
 * the shared BSS base and large-function register allocation (12-byte delta). */
static float p_pz_mode_endofgame(void) {
    PuzzleLocalizedImagePlacement* winner_placement;
    PuzzleLocalizedImagePlacement* loser_placement;
    PuzzleWagerProfile* winning_profile;
    PuzzleWagerProfile* losing_profile;
    unsigned int winner_index;
    int alpha;
    int index;
    int timeout;

    game_save_loop_count++;
    winner_index =
        puzzle_ctrl->players[0]->round_result_value >
                puzzle_ctrl->players[1]->round_result_value
            ? 0
            : 1;
    if (winner_index == 0) {
        winner = 1;
    } else if (winner_index == 1) {
        winner = 2;
    }

    if (__pz_start_msg.blood_level > 0) {
        if (g_puzzle_music != 0) {
            snd_stop(g_puzzle_music);
        }
        g_puzzle_music = 0;
        g_fatality_sound = snd_req(0x1A9B);
        for (alpha = 0xFF; alpha >= 0x28; alpha -= 8) {
            for (index = 0; index < 2; index++) {
                if (puzzle_ctrl->supermove_fade_objects[index] != 0) {
                    pfx_2d_obj_set_alpha(
                        puzzle_ctrl->supermove_fade_objects[index], alpha);
                }
            }
            for (index = 0; index < 13; index++) {
                if (puzzle_ctrl->round_objects[index] != 0) {
                    pfx_2d_obj_set_alpha(puzzle_ctrl->round_objects[index],
                                         alpha);
                }
            }
            for (index = 0; index < 4; index++) {
                if (puzzle_ctrl->victory_objects[index] != 0) {
                    pfx_2d_obj_set_alpha(
                        puzzle_ctrl->victory_objects[index], alpha);
                }
            }
            if (puzzle_ctrl->score_text[0] != 0) {
                set_string_obj_alpha(puzzle_ctrl->score_text[0],
                                     (float)alpha);
            }
            if (puzzle_ctrl->score_text[1] != 0) {
                set_string_obj_alpha(puzzle_ctrl->score_text[1],
                                     (float)alpha);
            }
            if (puzzle_ctrl->result_text[0] != 0) {
                set_string_obj_alpha(puzzle_ctrl->result_text[0],
                                     (float)alpha);
            }
            if (puzzle_ctrl->result_text[1] != 0) {
                set_string_obj_alpha(puzzle_ctrl->result_text[1],
                                     (float)alpha);
            }
            _mkproc_sleep_ticks = 1.0f;
            ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        }

        if (puzzle_ctrl->score_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[0]);
        }
        puzzle_ctrl->score_text[0] = 0;
        if (puzzle_ctrl->score_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[1]);
        }
        puzzle_ctrl->score_text[1] = 0;
        if (puzzle_ctrl->result_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->result_text[0]);
        }
        puzzle_ctrl->result_text[0] = 0;
        if (puzzle_ctrl->result_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->result_text[1]);
        }
        puzzle_ctrl->result_text[1] = 0;
        pz_event.player = winner_index;
        pz_event.type = 0xA;
        pz_fighter_event(&pz_event);
        timeout = 0x4B0;
        while (--timeout != 0 && pz_fighter_fatality_over() == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        }
        if (g_fatality_sound != 0) {
            snd_stop(g_fatality_sound);
        }
        snd_req(0x1A9C);
    }

    winner_placement =
        &pzlang_image_table[7].language[get_language()];
    loser_placement =
        &pzlang_image_table[8].language[get_language()];
    if (puzzle_ctrl->players[0]->flags3_bits.hide_active_piece == 0) {
        puzzle_ctrl->players[0]->result_portrait = load_2d_pfxobj(
            0x70033, 0x601D,
            (char*)art_puzzle_fighter_messages_tbl[6], 0, 0x39);
        puzzle_ctrl->players[1]->result_portrait = load_2d_pfxobj(
            0x70033, 0x601D,
            (char*)art_puzzle_fighter_messages_tbl[7], 0, 0x39);
        g_game_info.plyr1.field_48 = 0;
        g_game_info.plyr0.field_48++;
        if (puzzle_ctrl->players[0]->result_portrait != 0) {
            puzzle_ctrl->players[0]->result_portrait->x =
                winner_placement->player1_x;
            puzzle_ctrl->players[0]->result_portrait->y =
                winner_placement->y;
        }
        if (puzzle_ctrl->players[1]->result_portrait != 0) {
            puzzle_ctrl->players[1]->result_portrait->x =
                loser_placement->player2_x;
            puzzle_ctrl->players[1]->result_portrait->y = loser_placement->y;
        }
    } else {
        puzzle_ctrl->players[0]->result_portrait = load_2d_pfxobj(
            0x70033, 0x601D,
            (char*)art_puzzle_fighter_messages_tbl[7], 0, 0x39);
        puzzle_ctrl->players[1]->result_portrait = load_2d_pfxobj(
            0x70033, 0x601D,
            (char*)art_puzzle_fighter_messages_tbl[6], 0, 0x39);
        g_game_info.plyr0.field_48 = 0;
        g_game_info.plyr1.field_48++;
        if (puzzle_ctrl->players[0]->result_portrait != 0) {
            puzzle_ctrl->players[0]->result_portrait->x =
                loser_placement->player1_x;
            puzzle_ctrl->players[0]->result_portrait->y = loser_placement->y;
        }
        if (puzzle_ctrl->players[1]->result_portrait != 0) {
            puzzle_ctrl->players[1]->result_portrait->x =
                winner_placement->player2_x;
            puzzle_ctrl->players[1]->result_portrait->y =
                winner_placement->y;
        }
    }

    if (puzzle_ctrl->winner_runtime_data != 0) {
        sprintf(temp_80_char,
                global_player_data[puzzle_ctrl->winner_character_id].name);
        strcat(temp_80_char, get_string(7));
        if (puzzle_ctrl->result_message != 0 &&
            puzzle_ctrl->result_message->instance != 0) {
            puzzle_ctrl->result_message->typed_vtbl->destroy(
                puzzle_ctrl->result_message);
        }
        puzzle_ctrl->result_message = string_center_xy(
            0x2023, 0x11, temp_80_char,
            screen_width / 2 - (screen_width - 0x280) / 2,
            0xA0, 0x3A);
        if (puzzle_ctrl->winner_runtime_data->win_sound_id != 0) {
            snd_req(puzzle_ctrl->winner_runtime_data->win_sound_id);
        }
    }

    puzzle_ctrl->players[0]->active_cell = 0;
    puzzle_ctrl->players[1]->active_cell = 0;
    _mkproc_sleep_ticks = 180.0f;
    ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    if (__pz_start_msg.blood_level > 0) {
        snd_req(0x17);
        load_2d_pfxobj_xy(0x70033, 0x601D, (char*)0x0802001D,
                          0, 0x80, 0xB0, 0x39);
        load_2d_pfxobj_xy(0x70033, 0x601D, (char*)0x0802001E,
                          0, 0x180, 0xB0, 0x39);
    }

    if (((puzzle_ctrl->players[0]->profile_bits.profile_ineligible != 0 &&
          winner_index == 1) ||
         (puzzle_ctrl->players[1]->profile_bits.profile_ineligible != 0 &&
          winner_index == 0)) &&
        ((winner_index == 0 && p1_profile_status == 1) ||
         (winner_index == 1 && p2_profile_status == 1))) {
        _mkproc_sleep_ticks = 180.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        award_koins_to_player(winner_index, g_game_info.pselect.field_1e8,
                              g_game_info.pselect.field_1e4);
        show_koin_award(winner_index, g_game_info.pselect.field_1ec,
                        g_game_info.pselect.field_1e4, 0x3F);
        game_save_loop_count = 5;
        init_wagering();
        _mkproc_sleep_ticks = 60.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }

    if (puzzle_ctrl->players[0]->profile_bits.profile_ineligible == 0 &&
        puzzle_ctrl->players[1]->profile_bits.profile_ineligible == 0 &&
        g_game_info.pselect.field_1d8 > 0) {
        if (winner_index == 0) {
            winning_profile = &p1_profile;
            losing_profile = &p2_profile;
        } else {
            winning_profile = &p2_profile;
            losing_profile = &p1_profile;
        }
        _mkproc_sleep_ticks = 180.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        winning_profile->wager_total[get_current_wager_koin()] +=
            g_game_info.pselect.field_1d8 * 2;
        winning_profile->wager_wins[get_current_wager_koin()] +=
            g_game_info.pselect.field_1d8;
        losing_profile->wager_losses[get_current_wager_koin()] +=
            g_game_info.pselect.field_1d8;
        get_current_wager_koin();
        show_koin_award(winner_index, g_game_info.pselect.field_1d8,
                        get_current_wager_koin(), 0x3F);
        game_save_loop_count = 5;
        init_wagering();
        _mkproc_sleep_ticks = 60.0f;
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }

    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_exit, 120.0f);
    return 120.0f;
}

/* Recovered retail round-fill flow. Remaining bounded work is source shaping
 * around the shared BSS base and duplicated sequence setup (36-byte delta). */
static float p_pz_mode_fill(void) {
    PuzzleBlockOffset* offset;
    int* sequence;
    unsigned int ticks;
    int wait_ticks;
    int row;
    int column;
    int sequence_length;
    int sequence_value;

    if (puzzle_ctrl->players[0]->active_cell != 0) {
        if (puzzle_ctrl->players[0]->flags3_bits.hide_active_piece == 0) {
            puzzle_ctrl->players[0]->active_cell = 0;
        } else if (puzzle_ctrl->particle_effect_ticks == 0 &&
                   puzzle_ctrl->particle_effect_distance >= 90.0f) {
            puzzle_ctrl->players[0]->active_cell = 0;
        }
    }
    if (puzzle_ctrl->players[1]->active_cell != 0) {
        if (puzzle_ctrl->players[1]->flags3_bits.hide_active_piece == 0) {
            puzzle_ctrl->players[1]->active_cell = 0;
        } else if (puzzle_ctrl->particle_effect_ticks == 0 &&
                   puzzle_ctrl->particle_effect_distance >= 90.0f) {
            puzzle_ctrl->players[1]->active_cell = 0;
        }
    }

    if (puzzle_ctrl->particle_effect_distance != 0.0f) {
        offset = puzzle_ctrl->block_offsets;
        for (row = 0; row < 14; row++) {
            for (column = 0; column < 8; column++, offset++) {
                offset->y -= 0.2f;
            }
        }
    }

    if (puzzle_ctrl->players[0]->active_cell == 0 &&
        puzzle_ctrl->players[1]->active_cell == 0) {
        if (puzzle_ctrl->players[0]->round_result_value >=
                get_puzzle_rounds_to_win() ||
            puzzle_ctrl->players[1]->round_result_value >=
                get_puzzle_rounds_to_win() ||
            g_game_info.feature_flags.bits.powerbars_locked != 0) {
            for (ticks = 0; ticks < 60; ticks++) {
                _mkproc_sleep_ticks = 1.0f;
                ((PuzzleProcVtable*)aproc->vtbl)->sleep();
            }
            wait_ticks = 360;
            while (--wait_ticks != 0 &&
                   pz_fighter_is_round_over() == 0) {
                _mkproc_sleep_ticks = 1.0f;
                ((PuzzleProcVtable*)aproc->vtbl)->sleep();
            }
            ((PuzzleProcVtable*)aproc->vtbl)->transfer(
                p_pz_mode_endofgame, 1.0f);
            return 1.0f;
        }

        if (puzzle_ctrl->input_bits.input_latched == 0 &&
            g_game_info.feature_flags.bits.high_bit == 0) {
            if (game_save_loop_count != 0) {
                ck_do_profile_save();
            }
            g_game_info.feature_flags.bits.powerbars_locked = 1;
            atm_reset_current_page(1);
            ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_exit, 0.0f);
            return 0.0f;
        }

        if (puzzle_ctrl->winner_runtime_data != 0) {
            sprintf(temp_80_char,
                    global_player_data[puzzle_ctrl->winner_character_id].name);
            strcat(temp_80_char, get_string(7));
            if (puzzle_ctrl->result_message != 0 &&
                puzzle_ctrl->result_message->instance != 0) {
                puzzle_ctrl->result_message->typed_vtbl->destroy(
                    puzzle_ctrl->result_message);
            }
            puzzle_ctrl->result_message = string_center_xy(
                0x2023, 0x11, temp_80_char,
                screen_width / 2 - (screen_width - 0x280) / 2,
                0xA0, 0x3A);
            if (puzzle_ctrl->winner_runtime_data->win_sound_id != 0) {
                snd_req(puzzle_ctrl->winner_runtime_data->win_sound_id);
            }
        }

        puzzle_ctrl->players[0]->active_cell = 0;
        puzzle_ctrl->players[1]->active_cell = 0;
        if (puzzle_mode_net != 0) {
            if (is_pz_net_master != 0) {
                pz_init_network_array();
            } else {
                do {
                    _mkproc_sleep_ticks = 1.0f;
                    ((PuzzleProcVtable*)aproc->vtbl)->sleep();
                } while (__pz_start_msg.network_side != 0);
                __pz_start_msg.network_side =
                    g_game_info.plyr1.player_index;
            }
        } else {
            __pz_feed_rand_msg.array_index = 0;
            while (puzzle_array_table_local[
                       __pz_feed_rand_msg.array_index]
                       .sequence != 0) {
                __pz_feed_rand_msg.array_index++;
            }
            if (puzzle_array_table_local[
                    __pz_feed_rand_msg.array_index]
                    .sequence == 0) {
                __pz_feed_rand_msg.array_index =
                    randu0((unsigned short)__pz_feed_rand_msg.array_index);
            }
            if (puzzle_ctrl->sequence_bits.piece_sequence_owned != 0 &&
                puzzle_ctrl->piece_sequence != 0) {
                free_mem(puzzle_ctrl->piece_sequence);
            }
            puzzle_ctrl->piece_sequence = 0;
            sequence = puzzle_array_table_local[
                           __pz_feed_rand_msg.array_index]
                           .sequence;
            if (sequence != 0) {
                __pz_feed_rand_msg.sequence = sequence;
                sequence_length = 0;
                while ((sequence_value = sequence[sequence_length]) >= 0) {
                    if (sequence_value == 0) {
                        ((PuzzleProcVtable*)aproc->vtbl)->transfer(
                            p_pz_mode_exit, 0.0f);
                        break;
                    }
                    if (sequence_value > 8) {
                        if (sequence_value == PUZZLE_BLOCK_SUPERBREAKER) {
                            puzzle_ctrl->flags2_bits.network_sequence_marker =
                                1;
                        } else if (sequence_value !=
                                   PUZZLE_BLOCK_WILDCARD) {
                            ((PuzzleProcVtable*)aproc->vtbl)->transfer(
                                p_pz_mode_exit, 0.0f);
                            break;
                        }
                    }
                    sequence_length++;
                }
                if (sequence_value < 0) {
                    __pz_feed_rand_msg.sequence_length = sequence_length;
                    puzzle_ctrl->sequence_bits.piece_sequence_owned = 0;
                }
            }
            puzzle_ctrl->piece_sequence_length =
                __pz_feed_rand_msg.sequence_length;
            puzzle_ctrl->piece_sequence = __pz_feed_rand_msg.sequence;
        }

        for (ticks = 0; ticks < 180; ticks++) {
            _mkproc_sleep_ticks = 1.0f;
            ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        }
        if (puzzle_ctrl->players[0]->result_portrait != 0) {
            pull_screen_obj(puzzle_ctrl->players[0]->result_portrait);
            destroy_screen_obj(puzzle_ctrl->players[0]->result_portrait);
            puzzle_ctrl->players[0]->result_portrait = 0;
        }
        if (puzzle_ctrl->players[1]->result_portrait != 0) {
            pull_screen_obj(puzzle_ctrl->players[1]->result_portrait);
            destroy_screen_obj(puzzle_ctrl->players[1]->result_portrait);
            puzzle_ctrl->players[1]->result_portrait = 0;
        }
        if (puzzle_ctrl->result_message != 0 &&
            puzzle_ctrl->result_message->instance != 0) {
            puzzle_ctrl->result_message->typed_vtbl->destroy(
                puzzle_ctrl->result_message);
        }
        puzzle_ctrl->result_message = 0;
        wait_ticks = 180;
        while (--wait_ticks != 0 && pz_fighter_is_round_over() == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        }
        pz_event.type = 0x12;
        pz_fighter_event(&pz_event);
        ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_clear, 1.0f);
        return 1.0f;
    }

    if (puzzle_ctrl->particle_effect_ticks != 0) {
        puzzle_ctrl->particle_effect_ticks--;
    } else {
        if (puzzle_ctrl->particle_effect_distance == 0.0f) {
            snd_req(0x1B16);
        }
        puzzle_ctrl->particle_effect_distance += 1.0f;
    }
    return 1.0f;
}

/* Near miss: exact retail size, instructions, control flow, and accesses.
 * Objdiff reports only register operands and shared constant relocations. */
static float p_pz_mode_who_won(void) {
    int rounds_to_win;
    int row;
    int column;
    PuzzleBlockOffset* offset;
    float x;

    if (puzzle_mode_net != 0) {
        puzzle_mode_net = 3;
    }
    puzzle_ctrl->input_bits.accept_input = 0;
    puzzle_ctrl->winner_runtime_data = 0;
    puzzle_ctrl->losing_fighter = 0;

    puzzle_record_round_winner(puzzle_ctrl->players[0]);
    puzzle_record_round_winner(puzzle_ctrl->players[1]);

    puzzle_game_view()->startup_bits.round_active = 0;
    pz_event.type = 2;
    pz_fighter_event(&pz_event);

    rounds_to_win = get_puzzle_rounds_to_win();
    if (puzzle_ctrl->players[0]->round_result_value >= rounds_to_win ||
        puzzle_ctrl->players[1]->round_result_value >=
            get_puzzle_rounds_to_win() ||
        g_game_info.feature_flags.bits.powerbars_locked != 0) {
        if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
            df_press_start_proc_item[0] = 0;
            df_press_start_proc_item[1] = 0;
            df_press_start_item[0] = 0;
            df_press_start_item[1] = 0;
        }
        pz_event.type = 0xE;
        if (__pz_start_msg.blood_level > 0) {
            if (am_i_female(puzzle_ctrl->losing_fighter) != 0) {
                puzzle_fighter_mode_start(9);
                snd_req(0x15);
            } else {
                puzzle_fighter_mode_start(8);
                snd_req(0x14);
            }
        }
    } else {
        pz_event.type = 0xB;
    }

    if (puzzle_ctrl->players[0]->flags3_bits.hide_active_piece == 0) {
        pz_event.player = 0;
        pz_fighter_event(&pz_event);
        puzzle_ctrl->players[0]->round_end_time =
            (float)puzzle_ctrl->gameplay_ticks / (float)refresh_rate();
        puzzle_ctrl->players[1]->round_end_time = -1.0f;
    }
    if (puzzle_ctrl->players[1]->flags3_bits.hide_active_piece == 0) {
        pz_event.player = 1;
        pz_fighter_event(&pz_event);
        puzzle_ctrl->players[0]->round_end_time = -1.0f;
        puzzle_ctrl->players[1]->round_end_time =
            (float)puzzle_ctrl->gameplay_ticks / (float)refresh_rate();
    }

    puzzle_ctrl->players[0]->final_score = puzzle_ctrl->player1_score;
    puzzle_ctrl->players[1]->final_score = puzzle_ctrl->player2_score;
    offset = puzzle_ctrl->block_offsets;
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++, offset++) {
            x = 0.5f + frand(0.5f);
            x = (column & 1) != 0 ? x : -x;
            x = (row & 1) != 0 ? x : -x;
            offset->x = x;
            offset->y = sfrand(10.0f);
        }
    }
    snd_req(0x1B09);
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_fill, 1.0f);
    return 1.0f;
}

/* Near miss: exact retail size, control flow, calls, and accesses. The
 * remaining objdiff entries are register operands and pool labels only. */
static float p_pz_mode_play(void) {
    MkHdr* proc_data;
    MkProc* proc;
    int player0_active;
    int player1_active;

    if (puzzle_ctrl->pfx_bits.pfx_error != 0) {
        ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_exit, 1.0f);
        return 1.0f;
    }

    if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
        puzzle_ctrl->timed_match_ticks--;
        if (puzzle_ctrl->timed_match_ticks == 0) {
            ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_exit, 1.0f);
            return 1.0f;
        }
    }

    puzzle_ctrl->gameplay_ticks++;
    if (puzzle_ctrl->speedup_ticks++ > 600) {
        puzzle_ctrl->speedup_ticks = 0;
        if (puzzle_ctrl->players[0]->match_delay > 5) {
            puzzle_ctrl->players[0]->match_delay--;
        }
        if (puzzle_ctrl->players[1]->match_delay > 5) {
            puzzle_ctrl->players[1]->match_delay--;
        }
    }

    player0_active = puzzle_ctrl->players[0]->mode_step(
        puzzle_ctrl->players[0], puzzle_ctrl->players[1]);
    player1_active = puzzle_ctrl->players[1]->mode_step(
        puzzle_ctrl->players[1], puzzle_ctrl->players[0]);
    if (player0_active == 0 || player1_active == 0) {
        puzzle_ctrl->players[0]->super_active = 0;
        puzzle_ctrl->players[1]->super_active = 0;
        puzzle_ctrl->players[0]->flags3_bits.counter_drops_active = 0;
        puzzle_ctrl->players[1]->flags3_bits.counter_drops_active = 0;
        puzzle_ctrl->particle_effect_ticks = 49;
        puzzle_ctrl->particle_effect_distance = 0.0f;

        proc = _create_mkproc_generic_nostack(
            0x6011, 0x1F, p_puzzle_music_fade, 0, &proc_data);
        if (proc == 0) {
            if (g_puzzle_music != 0) {
                snd_stop(g_puzzle_music);
            }
            g_puzzle_music = 0;
        }
        mk_insert(&proc->hdr, &puzzle_ctrl->fight_message_processes);
        puzzle_ctrl->puzzle_music_target_volume = 0.0f;
        puzzle_ctrl->puzzle_music_volume = 1.0f;

        if (player0_active == 0) {
            puzzle_ctrl->players[0]->flags3_bits.hide_active_piece = 1;
        }
        if (player1_active == 0) {
            puzzle_ctrl->players[1]->flags3_bits.hide_active_piece = 1;
        }
        ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_who_won, 1.0f);
        return 1.0f;
    }
    return 1.0f;
}

/* Near miss: exact size and instruction sequence; objdiff reports only
 * register operands and shared constant/string relocation labels. */
static float p_pz_mode_start(void) {
    MkHdr* proc_data;
    MkProc* proc;

    wait_for_a_sound_bank_to_load(0x71);
    if (get_puzzle_rounds_to_win() != 1) {
        if (puzzle_ctrl->players[0]->round_result_value == 0 ||
            puzzle_ctrl->players[1]->round_result_value == 0) {
            g_puzzle_music = random_snd_req(0xB3);
        } else {
            g_puzzle_music = random_snd_req(0xB4);
        }
    } else {
        g_puzzle_music = random_snd_req(0xB2);
    }

    puzzle_ctrl->puzzle_music_channel = 0x1C03;
    proc = _create_mkproc_generic_nostack(
        0x6011, 0x1F, p_puzzle_music_fade, 0, &proc_data);
    if (proc == 0 && g_puzzle_music != 0) {
        set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel, 1.0f);
    }
    mk_insert((MkHdr*)proc, &puzzle_ctrl->fight_message_processes);
    puzzle_ctrl->puzzle_music_target_volume = 1.0f;
    puzzle_ctrl->puzzle_music_volume = 0.0f;

    if (puzzle_ctrl->start_bits.start_initialized == 0) {
        puzzle_ctrl->start_bits.start_initialized = 1;
        _mkproc_sleep_ticks = (float)puzzle_fighter_mode_start(0);
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        _mkproc_sleep_ticks = (float)puzzle_fighter_mode_start(
            puzzle_ctrl->players[0]->round_result_value +
            puzzle_ctrl->players[1]->round_result_value + 1);
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
        if (puzzle_ctrl->players[0]->event_player == 0) {
            if (puzzle_ctrl->player1_score != 0) {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                        puzzle_ctrl->player1_score);
            } else {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
            }
            if (puzzle_ctrl->score_text[0] != 0) {
                destroy_string_obj(puzzle_ctrl->score_text[0]);
            }
            puzzle_ctrl->score_text[0] = string_right_xy(
                0x6023, 0x0D, temp_80_char,
                art_puzzle_fighter_static_tbl.placements[5].x + 54,
                art_puzzle_fighter_static_tbl.placements[5].y + 4, 0x3A);
            if (puzzle_ctrl->score_text[0] != 0) {
                pull_string_obj(puzzle_ctrl->score_text[0]);
            }
        } else {
            if (puzzle_ctrl->player2_score != 0) {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                        puzzle_ctrl->player2_score);
            } else {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
            }
            if (puzzle_ctrl->score_text[1] != 0) {
                destroy_string_obj(puzzle_ctrl->score_text[1]);
            }
            puzzle_ctrl->score_text[1] = string_right_xy(
                0x6023, 0x0D, temp_80_char,
                art_puzzle_fighter_static_tbl.placements[6].x + 54,
                art_puzzle_fighter_static_tbl.placements[6].y + 4, 0x3A);
            if (puzzle_ctrl->score_text[1] != 0) {
                pull_string_obj(puzzle_ctrl->score_text[1]);
            }
        }

        if (puzzle_ctrl->players[1]->event_player == 0) {
            if (puzzle_ctrl->player1_score != 0) {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                        puzzle_ctrl->player1_score);
            } else {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
            }
            if (puzzle_ctrl->score_text[0] != 0) {
                destroy_string_obj(puzzle_ctrl->score_text[0]);
            }
            puzzle_ctrl->score_text[0] = string_right_xy(
                0x6023, 0x0D, temp_80_char,
                art_puzzle_fighter_static_tbl.placements[5].x + 54,
                art_puzzle_fighter_static_tbl.placements[5].y + 4, 0x3A);
            if (puzzle_ctrl->score_text[0] != 0) {
                pull_string_obj(puzzle_ctrl->score_text[0]);
            }
        } else {
            if (puzzle_ctrl->player2_score != 0) {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                        puzzle_ctrl->player2_score);
            } else {
                sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
            }
            if (puzzle_ctrl->score_text[1] != 0) {
                destroy_string_obj(puzzle_ctrl->score_text[1]);
            }
            puzzle_ctrl->score_text[1] = string_right_xy(
                0x6023, 0x0D, temp_80_char,
                art_puzzle_fighter_static_tbl.placements[6].x + 54,
                art_puzzle_fighter_static_tbl.placements[6].y + 4, 0x3A);
            if (puzzle_ctrl->score_text[1] != 0) {
                pull_string_obj(puzzle_ctrl->score_text[1]);
            }
        }
    } else {
        _mkproc_sleep_ticks = (float)puzzle_fighter_mode_start(
            puzzle_ctrl->players[0]->round_result_value +
            puzzle_ctrl->players[1]->round_result_value + 1);
        ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    }

    puzzle_fighter_mode_start(4);
    snd_req(0);
    _mkproc_sleep_ticks = 60.0f;
    ((PuzzleProcVtable*)aproc->vtbl)->sleep();
    pz_event.type = 0;
    pz_fighter_event(&pz_event);
    puzzle_ctrl->input_bits.accept_input = 1;
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_play, 1.0f);
    return 1.0f;
}

/* Near match: p_pz_mode_clear 99.57%; the 1.0f pool identity differs only. */
static float p_pz_mode_clear(void) {
    int widescreen_x;

    widescreen_x = (screen_width - 0x280) / 2;
    pfxsystem_widescreen_offset(widescreen_x, 0);
    puzzle_fighter_mode_clear();
    ((PuzzleProcVtable*)aproc->vtbl)->transfer(p_pz_mode_start, 1.0f);
    return 1.0f;
}

static float p_puzzle_music_fade(void) {
    float target_volume;

    target_volume = puzzle_ctrl->puzzle_music_target_volume;
    if (target_volume > puzzle_ctrl->puzzle_music_volume) {
        puzzle_ctrl->puzzle_music_volume += 0.01f;
        target_volume = puzzle_ctrl->puzzle_music_target_volume;
        if (target_volume < puzzle_ctrl->puzzle_music_volume) {
            puzzle_ctrl->puzzle_music_volume = target_volume;
        }
        if (g_puzzle_music != 0) {
            set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel,
                        puzzle_ctrl->puzzle_music_volume);
        }
    } else {
        puzzle_ctrl->puzzle_music_volume -= 0.01f;
        target_volume = puzzle_ctrl->puzzle_music_target_volume;
        if (target_volume > puzzle_ctrl->puzzle_music_volume) {
            puzzle_ctrl->puzzle_music_volume = target_volume;
            if (g_puzzle_music != 0) {
                snd_stop(g_puzzle_music);
            }
            g_puzzle_music = 0;
        } else if (g_puzzle_music != 0) {
            set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel,
                        puzzle_ctrl->puzzle_music_volume);
        }
    }

    if (puzzle_ctrl->puzzle_music_target_volume ==
        puzzle_ctrl->puzzle_music_volume) {
        return -1.0f;
    }
    return 1.0f;
}

/* Near miss: exact size and operations; one player-pointer schedule differs. */
static void pz_update_plyr_profile_status(void) {
    PuzzlePlayerState* player0 = puzzle_ctrl->players[0];
    PuzzlePlayerState* player1 = puzzle_ctrl->players[1];
    PuzzleProfileStats* stats;
    PuzzlePlayerState* player;
    int result0 = player0->round_result_value;
    int result1 = player1->round_result_value;

    if (g_game_info.plyr0.player_state == 0 &&
        player1->profile_stats != 0) {
        stats = player1->profile_stats;
        player = player1;
        if (result1 > result0) {
            stats->versus_wins++;
        } else {
            stats->versus_losses++;
        }
    } else if (g_game_info.plyr1.player_state == 0 &&
               player0->profile_stats != 0) {
        stats = player0->profile_stats;
        player = player0;
        if (result0 > result1) {
            stats->versus_wins++;
        } else {
            stats->versus_losses++;
        }
    } else {
        stats = player0->profile_stats;
        if (stats != 0) {
            if (result0 > result1) {
                stats->total_wins++;
            } else {
                stats->total_losses++;
            }
            puzzle_profile_update_maxima(stats, player0);
        }

        player = puzzle_ctrl->players[1];
        stats = player->profile_stats;
        if (stats != 0) {
            if (result1 > result0) {
                stats->total_wins++;
            } else {
                stats->total_losses++;
            }
        }
    }

    puzzle_profile_update_maxima(stats, player);
}

/* Emission-only near match (93.53%, retail 0x1FC/current 0x204). The network
 * message owns the selected sequence, matching retail's stored-pointer reloads;
 * signed sentinel and invalid-value exits agree. The remaining two-instruction
 * excess is loop-exit scheduling, BSS relocation selection, and register
 * allocation. */
static float pz_init_network_array(void) {
    int sequence_length;
    int value;

    __pz_feed_rand_msg.array_index = 0;
    while (puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence !=
           0) {
        __pz_feed_rand_msg.array_index++;
    }
    if (puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence == 0) {
        __pz_feed_rand_msg.array_index =
            randu0((unsigned short)__pz_feed_rand_msg.array_index);
    }

    if (puzzle_ctrl->sequence_bits.piece_sequence_owned != 0 &&
        puzzle_ctrl->piece_sequence != 0) {
        free_mem(puzzle_ctrl->piece_sequence);
    }
    puzzle_ctrl->piece_sequence = 0;

    if (puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence !=
        0) {
        __pz_feed_rand_msg.sequence =
            puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence;
        sequence_length = 0;
        while ((value = __pz_feed_rand_msg.sequence[sequence_length]) >= 0) {
            if (value == 0) {
                ((PuzzleProcVtable*)aproc->vtbl)
                    ->transfer(p_pz_mode_exit, 0.0f);
                break;
            }
            if (value > 8) {
                if (value == 9) {
                    puzzle_ctrl->flags2_bits.network_sequence_marker = 1;
                } else if (value != PUZZLE_BLOCK_WILDCARD) {
                    ((PuzzleProcVtable*)aproc->vtbl)
                        ->transfer(p_pz_mode_exit, 0.0f);
                    break;
                }
            }
            sequence_length++;
        }

        if (value < 0) {
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

/* Near match: pz_preinit_world 99.39%; frame scheduling differs only. */
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
        puzzle_ctrl->input_bits.accept_input != 0) {
        player = puzzle_switch_select_player(switch_state->fighter_pdata);

        if (player->super_active != 0) {
            player->super_active = 0;
            player->selected_supermove = player->equipped_supermove;
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

/* Recovery in progress: retail reevaluates the signed AI-input guard for each
 * switch query, its zero-result path clears quick_drop_repeat for AI players,
 * and both collision tests reuse one two-phase pair scan. Those behaviors are
 * recovered, including the AI quick-drop exit (83.66%, retail 0x868/current
 * 0x87C). The remaining 0x14 is the irreducible shared left/right repeat-tail
 * layout and its register-color cascade; a typed-helper attempt duplicated the
 * tail four times and was rejected at the bounded clean-C limit. This remains
 * structural, not an emission-only near miss. */
static int
puzzle_fighter_mode_play__drop_sequence(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent) {
    PuzzleSuperMoveCheck super_check;
    PuzzlePlayerState* super_target;
    PuzzleBoardCell* candidate;
    PuzzleBoardCell* destination;
    int command;
    int saved_column;
    int switch_direction;
    int switch_drop;
    int old_ticks;
    int rotated;
    int index;

    rotated = 0;
    command = player->input_command;
    candidate = player->active_cell;
    player->flags4_bits.drop_active = 1;

    do {
        if (player->input_command == 0 && player->previous_input_command == 3) {
            switch_drop = puzzle_player_check_switch(player, 0x0E);
            if (switch_drop != 0) {
                player->gravity_ticks -= 10;
                break;
            } else {
                player->previous_input_command = 0;
            }
        }

        switch_drop = puzzle_player_check_switch(player, 0x0C);
        if (switch_drop != 0) {
            puzzle_ctrl->input_bits.input_latched = 1;
            if (player->drop_boost_ticks != 0) {
                player->quick_drop_repeat = 0;
            } else if (player->quick_drop_repeat == 0) {
                player->quick_drop_repeat = 4;
            } else if (player->quick_drop_repeat > 1) {
                player->quick_drop_repeat--;
            } else {
                if (player->input_command != 9) {
                    player->input_command = 9;
                }
                command = 9;
            }
        } else {
            player->quick_drop_repeat = 0;
        }

        if (player->drop_control_bits.ai_controlled != 0) {
            old_ticks = player->input_repeat_ticks;
            player->input_repeat_ticks--;
            if (old_ticks > 0) {
                break;
            } else {
                player->input_repeat_ticks = 5;
                if (player->ai_pause_ticks != 0) {
                    player->ai_pause_ticks--;
                    command = player->input_command;
                } else {
                    int moved;

                    moved = 0;
                    if (player->super_active != 0) {
                        super_target = puzzle_ctrl->players[0];
                        if (player != super_target) {
                            super_target = puzzle_ctrl->players[1];
                        }
                        super_check =
                            pz_ai_super_move_table[player->equipped_supermove];
                        if (super_check == 0 ||
                            super_check(player, super_target) != 0) {
                            player->super_active = 0;
                            player->selected_supermove =
                                player->equipped_supermove;
                            player->super_start_tick = game_tick_ctr;
                            if (player->mode_step !=
                                puzzle_fighter_mode_play__supermove_sleep) {
                                player->saved_mode_step = player->mode_step;
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
                    break;
                } else if (command == 9) {
                    player->input_repeat_ticks = 0;
                    if (player->drop_control_bits.quick_drop_sound == 0) {
                        player->drop_control_bits.quick_drop_sound = 1;
                        pan_snd_req(0x1B04, player->sound_pan);
                    }
                    break;
                }
            }
        }

        do {
            switch_direction = 0;
            if (command != 9 &&
                (player->input_command == 0 || player->input_command == 3)) {
                switch_direction = puzzle_player_check_switch(player, 0x0F);
                if (switch_direction != 0) {
                    switch_direction = 1;
                } else {
                    switch_direction = puzzle_player_check_switch(player, 0x0D);
                    if (switch_direction != 0) {
                        switch_direction = 2;
                    }
                }
                if (switch_direction == 0) {
                    player->input_repeat_ticks = 0;
                }
            } else if (command == 9) {
                switch_direction = puzzle_player_check_switch(player, 0x0F);
                if (switch_direction != 0) {
                    switch_direction = 1;
                } else {
                    switch_direction = puzzle_player_check_switch(player, 0x0D);
                    if (switch_direction != 0) {
                        switch_direction = 2;
                    }
                }
                if (switch_direction != 0) {
                    command = 0;
                } else {
                    if (player->drop_control_bits.quick_drop_sound == 0) {
                        player->drop_control_bits.quick_drop_sound = 1;
                        pan_snd_req(0x1B04, player->sound_pan);
                    }
                }
            }
            if (switch_direction != 0) {
                player->input_command = 0;
                player->quick_drop_repeat = 0;
                old_ticks = player->input_repeat_ticks;
                player->input_repeat_ticks++;
                if (old_ticks < 5) {
                    break;
                } else {
                    player->input_repeat_ticks = 0;
                    command = switch_direction;
                }
            }

            if (command != 9) {
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
                    if (player->active_column <= 6 &&
                        (player->active_column < 6 ||
                         player->rotation_state != 1)) {
                        candidate++;
                        player->active_column++;
                    }
                    break;
                case 4:
                    candidate = puzzle_fighter_rotate_drop_pieces(4, player);
                    saved_column = player->active_column;
                    rotated = 1;
                    break;
                case 5:
                    candidate = puzzle_fighter_rotate_drop_pieces(5, player);
                    saved_column = player->active_column;
                    rotated = 1;
                    break;
                }

                if (puzzle_fighter_pair_position_open(player, candidate) != 0) {
                    player->active_cell = candidate;
                } else {
                    player->active_column = saved_column;
                    if (rotated != 0) {
                        pan_snd_req(0x1B06, player->sound_pan);
                    }
                }
            }
        } while (0);
    } while (0);

    if (command != 9) {
        if (player->drop_boost_ticks != 0) {
            player->drop_boost_ticks--;
            player->gravity_ticks++;
        }
        if (player->lock_ticks == 0) {
            old_ticks = player->gravity_ticks;
            player->gravity_ticks--;
            if (old_ticks > 0) {
                if (player->drop_control_bits.ai_controlled != 0 &&
                    player->input_command != 3) {
                    return 1;
                }
                if (player->input_command != 3) {
                    if (player->input_command != 0) {
                        player->previous_input_command = player->input_command;
                    }
                    player->input_command = 0;
                    return 1;
                }
                player->drop_boost_ticks = 0;
            }
            if (player->input_command != 0) {
                player->previous_input_command = player->input_command;
            }
            player->input_command = 0;
            player->gravity_ticks = player->drop_interval;
        }
    }

    if (player->active_row != 0 &&
        (player->active_row != 1 || player->rotation_state != 2)) {
        candidate = player->active_cell - 8;
        if (puzzle_fighter_pair_position_open(player, candidate) != 0) {
            player->active_row--;
            player->active_cell = candidate;
            player->lock_ticks = 0;
            player->gravity_ticks = player->drop_interval;
            return 1;
        }
    }

    player->gravity_ticks = 0;
    if (player->input_command != 9) {
        old_ticks = player->lock_ticks;
        player->lock_ticks = old_ticks + 1;
        if (old_ticks < 10) {
            if (player->input_command != 0) {
                player->previous_input_command = player->input_command;
            }
            player->input_command = 0;
            return 1;
        }
    }

    for (index = 0; index < 2; index++) {
        destination = player->active_cell;
        if (index != 0) {
            destination +=
                puzzle_piece_layout[player->rotation_state].column;
            destination += puzzle_piece_layout[player->rotation_state].row * 8;
        }
        destination->type = player->current_pair[index].type;
    }
    pan_snd_req(0x1B08, player->sound_pan);
    player->active_cell = 0;
    player->ai_target_column = -1;
    player->previous_input_command = 0;
    player->mode_step = puzzle_fighter_mode_play__collapse_holes;

    return 1;
}

/* Near miss: the recovered signed flag overlays and cleanup stores agree with
 * retail. The remaining 28 bytes materialize the typed invisibility helper's
 * boolean result instead of branching its two exceptional exits directly to
 * the shared cleanup loop. */
static int puzzle_fighter_mode_play__new_piece(PuzzlePlayerState* player,
                                               PuzzlePlayerState* opponent) {
    PuzzleAiData ai_data;
    PuzzlePlayerState* other_player;
    PuzzleBoardCell* cell;
    int original_cleared;
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
        player->flags2_mode_bits.score_applied = 1;
    }

    player->flags2_mode_bits.new_piece = 1;
    player->chain_count = 0;
    if (player->flags2_mode_bits.new_piece_latch == 0) {
        player->flags2_mode_bits.new_piece_latch = 1;
        return 1;
    }
    player->flags2_mode_bits.new_piece_latch = 0;

    if ((player->counter_drops_remaining != 0 ||
         player->flags4_counter_bits.force_counter_drops != 0) &&
        player->flags2_mode_bits.counter_active == 0) {
        player->flags3_bits.counter_drops_active = 0;
        player->mode_step = puzzle_fighter_mode_play__counter_drops;
        player->flags2_mode_bits.counter_active = 1;

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

    if (puzzle_ctrl->round_bits.round_started == 0) {
        puzzle_ctrl->round_bits.round_started = 1;
        if (!((player->flags2_mode_bits.new_piece != 0 &&
               other_player->flags2_mode_bits.new_piece != 0 &&
               other_player->mode_step ==
                   puzzle_fighter_mode_play__counter_drops) &&
              player->flags2_mode_bits.score_applied == 0)) {
            pz_event.type = 0x18;
            pz_event.player = player->event_player;
            pz_fighter_event(&pz_event);
        }
    } else if (player->flags2_mode_bits.score_applied == 0) {
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

    if (puzzle_fighter_update_invisibility(player) != 0) {
        for (row = 0; row < 14; row++) {
            for (column = 0; column < 8; column++) {
                cell = &player->board[row * 8 + column];
                cell->match_flag_bits.breaking = 0;
                cell->match_flag_bits.matched = 0;
            }
        }
    }

    player->flags2_mode_bits.counter_active = 0;
    player->flags2_mode_bits.score_applied = 0;
    player->mode_step = puzzle_fighter_mode_play__drop_sequence;

    if (player->ai_player_bits.ai_player != 0) {
        ai_data.player = player;
        pzsm_ai_get_data(&ai_data);
        if (player->ai_move == 3 &&
            (ai_data.highest_occupied_row < 5 ||
             ai_data.normal_block_count < 20)) {
            player->ai_move = player->ai_fallback_move;
        }
        player->ai_horizontal_sequence_index = 0;
        player->ai_rotation_sequence_index = 0;
        player->ai_drop_sequence_index = 0;
        player->ai_pause_ticks = 0;
        player->drop_flags &= ~0x04;
        player->input_command = 0;
    }

    return 1;
}

/* Near miss: retail traversal is recovered; one addis and register scheduling
 * remain as compiler-emission differences. */
static int
puzzle_fighter_mode_play__counter_drops(PuzzlePlayerState* player,
                                        PuzzlePlayerState* opponent) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* destination;
    PuzzleBoardCell* bottom_row;
    int type;
    int column;
    int column_offset;
    int row;
    int row_offset;


    if (player->counter_drops_remaining != 0) {
        if (player->counter_drop_delay > 0) {
            player->counter_drop_delay--;
            return 1;
        }

        bottom_row = &player->board[12 * 8];
        if (player->event_bits.counter_drops_active == 0) {
            pan_snd_req(0x1B0B, player->sound_pan);
        }
        player->event_bits.counter_drops_active = 1;

        row = 0;
        row_offset = 0;
        do {
            for (column = 0, column_offset = 0; column < 8;
                 column++, column_offset += sizeof(*cell)) {
                cell = (PuzzleBoardCell*)((unsigned char*)player->board +
                                          row_offset + column_offset);
                if (cell->type != 0 && cell->state == 0) {
                    continue;
                }

                for (;;) {
                    if (player->counter_sequence_index < 0) {
                        player->counter_sequence_index =
                            puzzle_ctrl->piece_sequence_length - 1;
                    }
                    type = puzzle_ctrl
                               ->piece_sequence[player
                                                    ->counter_sequence_index];
                    if (type != PUZZLE_BLOCK_SUPERBREAKER) {
                        break;
                    }
                    player->counter_sequence_index--;
                }

                if (type >= 4 &&
                    (unsigned int)type != PUZZLE_BLOCK_WILDCARD) {
                    type -= 4;
                    if (type == 0) {
                        type = PUZZLE_BLOCK_WILDCARD;
                    }
                }

                destination = (PuzzleBoardCell*)((unsigned char*)bottom_row +
                                                  column_offset);
                player->counter_sequence_index--;
                destination->type = type;
                destination->state = 0;
                player->counter_drop_delay = 2;
                if (player->drop_interval > 20) {
                    player->counter_drop_delay++;
                }

                player->counter_drops_remaining--;
                if (player->counter_drops_remaining == 0) {
                    player->flags3_bits.counter_drops_active = 0;
                    break;
                }
            }
            row++;
            row_offset += 8 * sizeof(*cell);
        } while (row < 14 && player->counter_drop_delay == 0 &&
                 player->counter_drops_remaining != 0);
    }

    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (player->counter_drops_remaining != 0) {
        return 0;
    }

    player->flags3_bits.counter_drops_active = 0;
    player->event_bits.counter_drops_active = 0;
    pan_snd_req(0x1B0C, player->sound_pan);
    player->mode_step = puzzle_fighter_mode_play__breakers;
    return 1;
}

/* Near miss: exact code; remaining difference is a constant reloc label. */
static int
puzzle_fighter_mode_play__collapse_holes(PuzzlePlayerState* player,
                                         PuzzlePlayerState* opponent) {
    int sound;

    if (puzzle_fighter_fill_holes(player) != 0) {
        player->flags2_event_bits.collapse_pending = 0;
        return 1;
    }

    if (player->flags2_event_bits.collapse_pending == 0) {
        if (player->event_bits.counter_drops_active != 0) {
            player->mode_step = puzzle_fighter_mode_play__counter_drops;
            return 1;
        } else {
            player->mode_step = puzzle_fighter_mode_play__breakers;
            return 1;
        }
    }

    pz_event.block_count = (float)player->cleared_blocks;
    pz_event.player = player->event_player;
    if (player->best_chain_count != 0) {
        pz_event.type = 6;
        pz_event.chain_count = (float)player->best_chain_count;
    } else {
        pz_event.player = player->event_player;
        pz_event.type = 12;
    }
    pz_fighter_event(&pz_event);

    player->flags2_event_bits.collapse_pending = 0;
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

    if (player->event_bits.floor_event != 0 &&
        player->flags2_event_bits.chain_resolved == 0) {
        pz_event.type = 5;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
        puzzle_fighter_display_floor_msg(player, 0);
    }

    player->flags2_event_bits.chain_resolved = 1;
    if (player->flag_bits.board_moving != 0) {
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

    data.player = opponent;
    pzsm_ai_get_data(&data);
    return data.flags.bits.has_superbreaker == 0;
}

static int pzsm_ai_rain_dance(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    data.player = opponent;
    pzsm_ai_get_data(&data);
    return data.flags.bits.has_superbreaker == 0;
}

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
        opponent_super = opponent->equipped_supermove;
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
        opponent_super = opponent->equipped_supermove;
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

/* Near miss: exact size and operations; remaining differences are registers. */
static int pzsm_ai_edge_clear(PuzzlePlayerState* player,
                              PuzzlePlayerState* opponent) {
    int columns[2] = {0, 7};
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
        opponent_super = opponent->equipped_supermove;
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

/* Near miss: exact size and operations; remaining differences are registers. */
static int pzsm_ai_drill(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent) {
    int columns[2] = {3, 4};
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
        opponent_super = opponent->equipped_supermove;
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

static int pzsm_ai_double_bomb(PuzzlePlayerState* player,
                               PuzzlePlayerState* opponent) {
    PuzzleAiData data;

    data.player = player;
    pzsm_ai_get_data(&data);

    if (data.highest_occupied_row > 5 || player->super_active < 2) {
        return 1;
    }
    if (player->board[1].type == 0) {
        return 1;
    }
    if (player->board[6].type == 0) {
        return 1;
    }
    return data.flags.bits.has_superbreaker != 0;
}

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
        opponent_super = opponent->equipped_supermove;
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
/* Emission-only near match (91.93%, retail 0x47C/current 0x48C). m2c and every
 * state-table access confirm the horizontal, rotation, timed-drop, pause, and
 * sequence-reset algorithms. The four-instruction residue is duplicated
 * command-tail layout plus cascading register allocation; extracting the
 * typed common tail reaches retail size but worsens both expansions. */
static int pz_ai_decide_move(PuzzlePlayerState* player) {
    unsigned short random_direction;
    int sequence_value;
    int command;
    int alternate;
    int* sequence;

    random_direction = randu0(3);
    if (player->drop_bits.supermove_done != 0) {
        pz_ai_decide_match(player);
    }

    sequence = pz_ai_move_lateral_tbl[player->ai_no_pause_band];
    sequence_value = sequence[player->ai_horizontal_sequence_index];
    command = 0;
    if (player->ai_target_column < player->active_column) {
        if (player->active_column != 1 || player->ai_target_column > 0 ||
            player->ai_target_rotation == player->rotation_state) {
            alternate = 0;
            if (sequence_value == 3 && random_direction == 0) {
                alternate = 1;
            }
            command = 1;
            if (alternate != 0) {
                command = 2;
            }
        }
    } else if (player->ai_target_column > player->active_column) {
        if (player->active_column != 6 || player->ai_target_column < 7 ||
            player->ai_target_rotation == player->rotation_state) {
            alternate = 0;
            if (sequence_value == 3 && random_direction == 0) {
                alternate = 1;
            }
            command = 2;
            if (alternate != 0) {
                command = 1;
            }
        }
    }

    if (command != 0) {
        if (pz_ai_check_no_pause(player) == 0) {
            return command;
        }
        if (player->ai_move == 1 || player->ai_move == 2) {
            player->ai_pause_ticks = player->ai_pause_base;
            if (player->ai_pause_ticks < 0) {
                player->ai_pause_ticks = 0;
            }
            return command;
        }
        if (sequence_value == 0) {
            command = 0;
            player->ai_pause_ticks = player->ai_pause_base;
        } else if (sequence_value == 4) {
            command = 0;
            player->ai_horizontal_sequence_index = -1;
            player->ai_pause_ticks = player->ai_pause_base;
        }
        player->ai_horizontal_sequence_index++;
        return command;
    }

    sequence = pz_ai_move_rotate_tbl[player->ai_no_pause_band];
    sequence_value = sequence[player->ai_rotation_sequence_index];
    if (player->ai_target_rotation != player->rotation_state) {
        if (player->ai_target_rotation == 0 || player->rotation_state == 0) {
            if (player->rotation_state != 0) {
                if (sequence_value == 3 && random_direction == 0) {
                    command = 4;
                    if (player->rotation_state > 2) {
                        command = 5;
                    }
                } else {
                    command = 5;
                    if (player->rotation_state > 2) {
                        command = 4;
                    }
                }
            } else if (sequence_value == 3 && random_direction == 0) {
                command = 5;
                if (player->ai_target_rotation > 2) {
                    command = 4;
                }
            } else {
                command = 4;
                if (player->ai_target_rotation > 2) {
                    command = 5;
                }
            }
        } else if (sequence_value == 3 && random_direction == 0) {
            command = 5;
            if (player->ai_target_rotation < player->rotation_state) {
                command = 4;
            }
        } else {
            command = 4;
            if (player->ai_target_rotation < player->rotation_state) {
                command = 5;
            }
        }

        if (pz_ai_check_no_pause(player) == 0) {
            return command;
        }
        if (player->ai_move == 1 || player->ai_move == 2) {
            player->ai_pause_ticks = player->ai_pause_base;
            if (player->ai_pause_ticks < 0) {
                player->ai_pause_ticks = 0;
            }
            return command;
        }
        if (sequence_value == 0) {
            command = 0;
            player->ai_pause_ticks = player->ai_pause_base;
        } else if (sequence_value == 4) {
            command = 0;
            player->ai_rotation_sequence_index = -1;
            player->ai_pause_ticks = player->ai_pause_base;
        }
        player->ai_rotation_sequence_index++;
        return command;
    }

    if (player->rotation_angle != 0.0f &&
        (player->ai_no_pause_band < 3 ||
         g_game_info.feature_flags.bits.powerbars_locked != 0)) {
        return 0;
    }

    if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
        if (player->timed_drop_bits.timed_drop_initialized == 0) {
            player->timed_drop_bits.timed_drop_initialized = 1;
            player->ai_pause_ticks = 5;
            return 0;
        }
        sequence = pz_ai_move_down_tbl[player->ai_no_pause_band + 2];
    } else {
        sequence = pz_ai_move_down_tbl[player->ai_no_pause_band];
    }
    sequence_value = sequence[player->ai_drop_sequence_index];

    if (player->ai_move == 1 || player->ai_move == 2) {
        player->input_command = 3;
        player->ai_pause_ticks = player->ai_pause_base;
        return player->input_command;
    }

    switch (sequence_value) {
    case 0:
        player->input_command = 0;
        player->ai_pause_ticks = player->ai_pause_base;
        break;
    case 4:
        player->input_command = 0;
        player->ai_drop_sequence_index = -1;
        player->ai_pause_ticks = player->ai_pause_base;
        break;
    case 2:
        player->input_command = 9;
        player->ai_pause_ticks = 30;
        break;
    default:
        player->input_command = 3;
        player->ai_pause_ticks = player->ai_pause_base * 3;
        break;
    }
    player->ai_drop_sequence_index++;
    return player->input_command;
}

/* Near miss: exact size, comparisons, scans, and common return. The default
 * result block is emitted on the opposite side of the shared five-cell scan. */
static int pz_ai_check_no_pause(PuzzlePlayerState* player) {
    PuzzleBoardCell* row;
    PuzzleBoardCell* cell;
    int band;
    int result;
    int cell_offset;
    int i;

    band = player->ai_no_pause_band;
    result = 1;
    row = &player->board[88];

    do {
        if (band < 2) {
            if (row[3].type != 0) {
                result = 0;
            }
            break;
        }

        if (band < 4) {
            if (band <= 2) {
                cell_offset = 2 * sizeof(*cell);
                for (i = 0; i < 3;
                     i++, cell_offset += sizeof(*cell)) {
                    cell = (PuzzleBoardCell*)((char*)row + cell_offset);
                    if (cell->type != 0) {
                        result = 0;
                        break;
                    }
                }
                break;
            }
        } else if (band < 6) {
            if (band > 2) {
                result = 0;
                break;
            }
        } else {
            result = 0;
            break;
        }

        cell_offset = sizeof(*cell);
        for (i = 0; i < 5; i++, cell_offset += sizeof(*cell)) {
            cell = (PuzzleBoardCell*)((char*)row + cell_offset);
            if (cell->type != 0) {
                result = 0;
                break;
            }
        }
    } while (0);
    return result;
}

/*
 * Recovery in progress: retail tries both adjacent columns before invalidating
 * an interior candidate. Keeping its four distinct invalidation leaves and
 * early equal-color value restores retail's major block ownership. Direct
 * adjacent matches now feed duplicated score expressions, allowing MWCC to
 * recover retail's backward scoring join without a synthetic boolean. Scoping
 * the initial score scan independently removes one false cross-region
 * lifetime. Retaining the selected piece's placement-row base through all four
 * invalidation leaves removes repeated address reconstruction. Vertical and
 * adjacent bonus matches now converge on retail's single score block. A typed
 * fallback helper plus a single selection loop restores retail's one fallback
 * body and backward unsafe-equal-color join. Selection now occurs after the
 * fallback guard, matching retail block ownership. Dispatch uses the published
 * target column rather than carrying the pre-publication selection through the
 * remaining branches, restoring retail's per-arm reloads and index formation
 * (87.05%, retail 0xB70/current 0xB7C). The stack frame and r21-r31 save range
 * are exact. The remaining 12-byte excess is structural and is not classified
 * as a near miss.
 */
static void pz_ai_decide_match(PuzzlePlayerState* player) {
    enum {
        PUZZLE_AI_SELECTION_FALLBACK = -1,
        PUZZLE_AI_SELECTION_DISPATCH = 0,
        PUZZLE_AI_SELECTION_RETRY = 2
    };
    PuzzleAiPlacement placements[2][8];
    PuzzleAiPlacement* placement;
    PuzzleBoardCell* cell;
    int best[2];
    int lowest[2];
    unsigned int raw_type;
    unsigned int piece0_type;
    unsigned int piece1_type;
    unsigned int piece_type;
    unsigned int other_type;
    unsigned int piece0_breaker;
    unsigned int piece1_breaker;
    unsigned int piece_breaker;
    unsigned int other_breaker;
    unsigned int landing_breaker;
    unsigned int scan_type;
    int minimum_row;
    int breaker_retry;
    int selected_piece;
    int selected_column;
    int selected_row;
    int other_piece;
    int maximum_row;
    int scan_row;
    int rotation;
    int piece;
    int column;
    int row;

    breaker_retry = 0;
    player->drop_bits.supermove_done = 0;
    player->ai_plan_bits.ai_match_bonus_pending = 1;

    if (pz_ai_decide_superbomb(player) != 0) {
        return;
    }
    if (player->ai_move != 3) {
        pz_ai_decide_quick_drop_tower(player);
        if (player->ai_move != 3) {
            return;
        }
    }

    minimum_row = pz_ai_match_precalc(player, placements);
    for (;;) {
        memset(best, -1, sizeof(best));
        memset(lowest, 1000, sizeof(lowest));

        {
            PuzzleAiPlacement* scan_placement;
            PuzzleBoardCell* scan_cell;
            unsigned int scan_raw_type;
            unsigned int scan_piece_breaker;
            unsigned int scan_landing_breaker;
            unsigned int scan_cell_type;
            int scan_best_score;
            int scan_lowest_score;
            int scan_piece;
            int scan_column;

            for (scan_piece = 0; scan_piece < 2; scan_piece++) {
                scan_best_score = -1;
                scan_lowest_score = 1000;
                scan_raw_type = player->current_pair[scan_piece].type;
                scan_piece_breaker =
                    scan_raw_type >= 4 &&
                            scan_raw_type != PUZZLE_BLOCK_WILDCARD
                        ? scan_raw_type
                        : 0;

                for (scan_column = 0; scan_column < 8; scan_column++) {
                    scan_placement = &placements[scan_piece][scan_column];
                    if (scan_placement->match_score < 0) {
                        continue;
                    }
                    scan_cell = &player->board_rows[scan_placement->row]
                                                      [scan_column];
                    scan_cell_type = scan_cell->type;
                    scan_landing_breaker =
                        scan_cell_type >= 4 &&
                                scan_cell_type != PUZZLE_BLOCK_WILDCARD
                            ? scan_cell_type
                            : 0;

                    if (scan_placement->row < 11 ||
                        scan_piece_breaker != 0 ||
                        scan_landing_breaker != 0) {
                        if (breaker_retry != 0 &&
                            (scan_piece_breaker != 0 ||
                             scan_landing_breaker != 0)) {
                            scan_placement->match_score += 200;
                        }
                        if (scan_placement->match_score >= scan_best_score) {
                            if (scan_placement->match_score > scan_best_score ||
                                scan_placement->row <
                                    placements[scan_piece][best[scan_piece]]
                                        .row) {
                                scan_best_score = scan_placement->match_score;
                                best[scan_piece] = scan_column;
                            }
                        }
                    }
                    if (scan_placement->match_score < scan_lowest_score) {
                        scan_lowest_score = scan_placement->match_score;
                        lowest[scan_piece] = scan_column;
                    }
                }
            }
        }

        if (player->ai_no_pause_band >= 2 &&
            (best[0] >= 0 || best[1] >= 0)) {
            if (player->ai_plan_bits.ai_match_bonus_pending == 0) {
                selected_piece = puzzle_ai_select_best_piece(placements, best);
                if (placements[selected_piece][best[selected_piece]].row <= 9) {
                    rotation = 0;
                    player->ai_target_column = best[selected_piece];
                    if (selected_piece != 0) {
                        rotation = 2;
                    }
                    player->ai_target_rotation = rotation;
                    break;
                }
            } else if (minimum_row < 10) {
                for (piece = 0; piece < 2; piece++) {
                    piece_type = player->current_pair[piece].type;
                    piece_breaker =
                        piece_type >= 4 && piece_type != PUZZLE_BLOCK_WILDCARD
                            ? piece_type
                            : 0;
                    other_piece = 1 - piece;
                    other_type = player->current_pair[other_piece].type;
                    other_breaker =
                        other_type >= 4 && other_type != PUZZLE_BLOCK_WILDCARD
                            ? other_type
                            : 0;

                if (piece_type >= 4) {
                    if (piece_type != PUZZLE_BLOCK_WILDCARD) {
                        piece_type -= 4;
                    } else {
                        piece_type = 0;
                    }
                }
                if (other_type >= 4) {
                    if (other_type != PUZZLE_BLOCK_WILDCARD) {
                        other_type -= 4;
                    } else {
                        other_type = 0;
                    }
                }

                scan_type = (unsigned int)-1;
                if (other_breaker == 0) {
                    continue;
                }
                for (column = 0; column < 8; column++) {
                    placement = &placements[piece][column];
                    if (placement->match_score < 0) {
                        continue;
                    }
                    maximum_row = 5;
                    if (piece_breaker != 0) {
                        maximum_row = 8;
                    }
                    if (placement->row > maximum_row) {
                        continue;
                    }

                    scan_row = placement->row;
                    while (scan_row >= 0) {
                        cell = &player->board_rows[scan_row][column];
                        scan_type = cell->type;
                        if (scan_type >= 4) {
                            if (scan_type != PUZZLE_BLOCK_WILDCARD) {
                                scan_type -= 4;
                            } else {
                                scan_type = 0;
                            }
                        }
                        if (scan_type != piece_type) {
                            break;
                        }
                        scan_row--;
                    }

                    if (scan_row >= 0 && scan_type == other_type) {
                        scan_row--;
                        while (scan_row >= 0) {
                            scan_type =
                                player->board_rows[scan_row][column].type;
                            if (scan_type >= 4) {
                                if (scan_type != PUZZLE_BLOCK_WILDCARD) {
                                    scan_type -= 4;
                                } else {
                                    scan_type = 0;
                                }
                            }
                            if (scan_type != other_type) {
                                break;
                            }
                            scan_row--;
                        }
                        if (scan_row < 0 || scan_type == piece_type) {
                            continue;
                        }
                    } else {
                        scan_row++;
                        if (column != 0) {
                            scan_type =
                                player->board_rows[scan_row][column - 1].type;
                            if (scan_type != 0) {
                                if (scan_type >= 4) {
                                    if (scan_type != PUZZLE_BLOCK_WILDCARD) {
                                        scan_type -= 4;
                                    } else {
                                        scan_type = 0;
                                    }
                                }
                            }
                        }
                        if (column == 0 || scan_type != other_type) {
                            if (column >= 7) {
                                continue;
                            }
                            scan_type =
                                player->board_rows[scan_row][column + 1].type;
                            if (scan_type == 0) {
                                continue;
                            }
                            if (scan_type >= 4) {
                                if (scan_type != PUZZLE_BLOCK_WILDCARD) {
                                    scan_type -= 4;
                                } else {
                                    scan_type = 0;
                                }
                            }
                            if (scan_type != other_type) {
                                continue;
                            }
                        }
                    }
                    placement->match_score +=
                        placement->row > maximum_row ? 10 : 300;
                }
                }
                if (player->ai_plan_bits.ai_match_bonus_pending != 0) {
                    player->ai_plan_bits.ai_match_bonus_pending = 0;
                    continue;
                }
            }
        }

        piece0_type = player->current_pair[0].type;
        if (piece0_type >= 4) {
            if (piece0_type != PUZZLE_BLOCK_WILDCARD) {
                piece0_type -= 4;
            } else {
                piece0_type = 0;
            }
        }
        piece1_type = player->current_pair[1].type;
        if (piece1_type >= 4) {
            if (piece1_type != PUZZLE_BLOCK_WILDCARD) {
                piece1_type -= 4;
            } else {
                piece1_type = 0;
            }
        }
        selected_piece = PUZZLE_AI_SELECTION_DISPATCH;
        for (;;) {
            if (selected_piece == PUZZLE_AI_SELECTION_FALLBACK ||
                (best[0] < 0 && best[1] < 0)) {
                puzzle_ai_choose_fallback(
                    player, minimum_row, piece0_type, piece1_type);
                player->ai_secondary_rotation = 0;
                return;
            }

            selected_piece = puzzle_ai_select_best_piece(placements, best);
            placement = placements[selected_piece];
            selected_column = best[selected_piece];
            selected_row = placement[selected_column].row;
            raw_type = player->current_pair[0].type;
            piece0_breaker =
                raw_type >= 4 && raw_type != PUZZLE_BLOCK_WILDCARD
                    ? raw_type
                    : 0;
            raw_type = player->current_pair[1].type;
            piece1_breaker =
                raw_type >= 4 && raw_type != PUZZLE_BLOCK_WILDCARD
                    ? raw_type
                    : 0;
            scan_type = player->board_rows[selected_row][selected_column].type;
            landing_breaker =
                scan_type >= 4 && scan_type != PUZZLE_BLOCK_WILDCARD
                    ? scan_type
                    : 0;

            if (breaker_retry == 0 && selected_row >= 9 &&
                (piece0_breaker != 0 || piece1_breaker != 0)) {
                breaker_retry = 1;
                selected_piece = PUZZLE_AI_SELECTION_RETRY;
                break;
            }

            player->ai_target_column = selected_column;
            player->ai_secondary_column = lowest[0];
            if (piece0_type != piece1_type) {
                break;
            }
            if (placement[player->ai_target_column].row < 10 ||
                piece0_breaker != 0 || piece1_breaker != 0 ||
                landing_breaker != 0) {
                player->ai_target_rotation = 0;
                player->ai_secondary_rotation = 0;
                return;
            }
            selected_piece = PUZZLE_AI_SELECTION_FALLBACK;
        }
        if (selected_piece == PUZZLE_AI_SELECTION_RETRY) {
            continue;
        }

        if (selected_piece != 0) {
            if (player->ai_target_column < 7) {
                    for (row = player->active_row; row >= 0; row--) {
                        if (player->board_rows[row]
                                              [player->ai_target_column + 1]
                                                  .type != 0) {
                            break;
                        }
                    }
                    if (row < 11) {
                        player->ai_target_column++;
                        player->ai_target_rotation = 3;
                        break;
                    }
                    if (piece1_breaker != 0 || landing_breaker != 0) {
                        player->ai_target_rotation = 2;
                        break;
                    }
                    if (player->ai_target_column <= 0) {
                        placement[player->ai_target_column].match_score = -1;
                        placement[player->ai_target_column].row = -1;
                        continue;
                    }
            }
            if (player->ai_target_column > 0) {
                    for (row = player->active_row; row >= 0; row--) {
                        if (player->board_rows[row]
                                              [player->ai_target_column - 1]
                                                  .type != 0) {
                            break;
                        }
                    }
                    if (row < 11) {
                        player->ai_target_column--;
                        player->ai_target_rotation = 1;
                        break;
                    }
                    if (piece1_breaker != 0 || landing_breaker != 0) {
                        player->ai_target_rotation = 2;
                        break;
                    }
                    placement[player->ai_target_column].match_score = -1;
                    placement[player->ai_target_column].row = -1;
                    continue;
            }
        } else {
            if (player->ai_target_column > 0) {
                    for (row = player->active_row; row >= 0; row--) {
                        if (player->board_rows[row]
                                              [player->ai_target_column - 1]
                                                  .type != 0) {
                            break;
                        }
                    }
                    if (row < 11) {
                        player->ai_target_rotation = 3;
                        break;
                    }
                    if (piece0_breaker != 0 || landing_breaker != 0) {
                        player->ai_target_rotation = 0;
                        break;
                    }
                    if (player->ai_target_column >= 7) {
                        placement[player->ai_target_column].match_score = -1;
                        placement[player->ai_target_column].row = -1;
                        continue;
                    }
            }
            if (player->ai_target_column < 7) {
                    for (row = player->active_row; row >= 0; row--) {
                        if (player->board_rows[row]
                                              [player->ai_target_column + 1]
                                                  .type != 0) {
                            break;
                        }
                    }
                    if (row < 11) {
                        player->ai_target_rotation = 1;
                        break;
                    }
                    if (piece0_breaker != 0 || landing_breaker != 0) {
                        player->ai_target_rotation = 0;
                        break;
                    }
                    placement[player->ai_target_column].match_score = -1;
                    placement[player->ai_target_column].row = -1;
                    continue;
            }
        }
    }

    player->ai_secondary_rotation = 0;
}

/* Near miss: placement scores, recursive match calls, row semantics, and
 * obstruction invalidation agree with retail. Moving scoring inside the row
 * scan and using a counted loop recover the exact 0x324-byte retail size; the
 * remaining differences are localized loop scheduling/register allocation. */
static int pz_ai_match_precalc(PuzzlePlayerState* player,
                               PuzzleAiPlacement placements[2][8]) {
    PuzzleMatchContext context;
    PuzzleBoardCell* board_cell;
    PuzzleBoardCell* clear_cell;
    unsigned int type;
    int minimum_row;
    int piece;
    int column;
    int row;

    minimum_row = 14;
    context.player = player;
    context.flag_bits.matched_latch = 1;
    context.breaker_visual = 0;
    context.block_visual = 0;
    memset(placements, -1, sizeof(PuzzleAiPlacement) * 16);

    for (piece = 0; piece < 2; piece++) {
        type = player->current_pair[piece].type;
        if (type >= 4) {
            if (type != PUZZLE_BLOCK_WILDCARD) {
                type -= 4;
            } else {
                type = 0;
            }
            if (type == 0) {
                type = PUZZLE_BLOCK_WILDCARD;
            }
        }
        context.base_type = type;

        for (column = 0; column < 8; column++) {
            for (clear_cell = player->board;
                 clear_cell < player->board_end; clear_cell++) {
                clear_cell->flag_bits.ai_visited = 0;
            }

            for (row = player->active_row; row >= 0; row--) {
                board_cell = &player->board[row * 8 + column];
                if (board_cell->type == 0) {
                    if (row != 0) {
                        continue;
                    }
                    if (!((column != 0 && (board_cell - 1)->type != 0) ||
                          (column < 7 && (board_cell + 1)->type != 0))) {
                        continue;
                    }
                } else {
                    if ((board_cell + 8)->type != 0) {
                        break;
                    }
                    board_cell += 8;
                }

                context.cell = board_cell;
                context.matched_count = 0;
                if (puzzle_fighter_match_above_below__ai(&context) != 0 &&
                    board_cell->ai_bits.ai_state == 0) {
                    board_cell->flag_bits.ai_visited = 1;
                    context.matched_count++;
                }
                if (puzzle_fighter_match_left_right__ai(&context) != 0 &&
                    board_cell->ai_bits.ai_state == 0) {
                    board_cell->flag_bits.ai_visited = 1;
                    context.matched_count++;
                }
                if (context.matched_count == 0) {
                    context.matched_count = -1;
                }
                placements[piece][column].match_score =
                    context.matched_count;
                placements[piece][column].row = row;
                if (row < minimum_row) {
                    minimum_row = row;
                }
                break;
            }
        }
    }

    for (column = player->active_column - 1; column >= 0; column--) {
        if (player->board[player->active_row * 8 + column].type != 0) {
            for (; column >= 0; column--) {
                placements[0][column].match_score = -1;
                placements[1][column].match_score = -1;
            }
            break;
        }
    }

    for (column = player->active_column + 1; column < 8; column++) {
        if (player->board[player->active_row * 8 + column].type != 0) {
            for (; column < 8; column++) {
                placements[0][column].match_score = -1;
                placements[1][column].match_score = -1;
            }
            break;
        }
    }
    return minimum_row;
}

/* Recovery in progress: both recursive neighbor algorithms, visited-state
 * accesses, and match counts agree with retail. Structured boundary-loop
 * exhaustion emits two post-loop pointer tests, accounting for the 40-byte
 * structural CFG residue; this is not an emission-only near miss. */
static int puzzle_fighter_match_left_right__ai(PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* boundary;
    PuzzleBoardCell* neighbor;
    unsigned int boundary_offset;
    int row;
    int matched = 0;

    next.player = context->player;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.flag_bits.matched_latch = context->match_bits.match_state;

    boundary_offset = 0;
    for (row = 0; row < 14; row++, boundary_offset += 8 * sizeof(*boundary)) {
        boundary = (PuzzleBoardCell*)((unsigned char*)context->player->board +
                                      boundary_offset);
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell - 1;
        if (neighbor >= context->player->board &&
            (neighbor->type - 4 == context->base_type ||
             neighbor->type == context->base_type) &&
            neighbor->ai_bits.ai_state == 0) {
            neighbor->flag_bits.ai_visited = 1;
            next.cell = neighbor;
            puzzle_fighter_match_above_below__ai(&next);
            puzzle_fighter_match_left_right__ai(&next);
            matched = 1;
            next.matched_count++;
        }
    }

    for (row = 0; row < 14; row++) {
        boundary = context->player->board + ((row + 1) * 8 - 1);
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell + 1;
        if (neighbor <= context->player->board_end &&
            (neighbor->type - 4 == context->base_type ||
             neighbor->type == context->base_type) &&
            neighbor->ai_bits.ai_state == 0) {
            neighbor->flag_bits.ai_visited = 1;
            next.cell = neighbor;
            puzzle_fighter_match_above_below__ai(&next);
            puzzle_fighter_match_left_right__ai(&next);
            matched = 1;
            next.matched_count++;
        }
    }

    context->matched_count += next.matched_count;
    return matched;
}

/* Exact match: definition order intentionally permits retail's one-level
 * recursive inlining before the left/right sibling. */
static int puzzle_fighter_match_above_below__ai(
    PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* neighbor;
    int matched;

    matched = 0;
    next.player = context->player;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.flag_bits.matched_latch = context->match_bits.match_state;

    neighbor = context->cell + 8;
    if (neighbor <= context->player->board_end &&
        (neighbor->type - 4 == context->base_type ||
         neighbor->type == context->base_type) &&
        neighbor->ai_bits.ai_state == 0) {
        neighbor->flag_bits.ai_visited = 1;
        next.cell = neighbor;
        puzzle_fighter_match_above_below__ai(&next);
        puzzle_fighter_match_left_right__ai(&next);
        matched = 1;
        next.matched_count++;
    }

    neighbor = context->cell - 8;
    if (neighbor >= context->player->board &&
        (neighbor->type - 4 == context->base_type ||
         neighbor->type == context->base_type) &&
        neighbor->ai_bits.ai_state == 0) {
        neighbor->flag_bits.ai_visited = 1;
        next.cell = neighbor;
        puzzle_fighter_match_above_below__ai(&next);
        puzzle_fighter_match_left_right__ai(&next);
        matched = 1;
        next.matched_count++;
    }

    context->matched_count += next.matched_count;
    return matched;
}

/* Emission-only near match (92.38%, retail 0x708/current 0x748). m2c and retail
 * CFG confirm all candidate tables, signed scans, breaker cases, candidate-
 * relative neighbor priorities, rotations, and column updates. Remaining
 * emission is duplicated outcome-tail layout and vertical-scan pointer/register
 * lifetime; a typed outcome helper inlines to the identical object. */
static void pz_ai_decide_quick_drop_tower(PuzzlePlayerState* player) {
    PuzzleAiData data;
    int move0_columns[] = {3, 4, 2, 5, -1};
    int move0_rows[] = {7, 7, 3, 3, -1};
    int move1_columns[] = {3, 6, 1, -1};
    int move1_alternate_columns[] = {0, 7, 4, 2, 5, -1};
    int move1_rows[] = {8, 8, 8, 5, 5, -1};
    int move2_columns[] = {4, 1, 6, -1};
    int move2_alternate_columns[] = {7, 0, 3, 5, 2, -1};
    int move2_rows[] = {6, 6, 6, 4, 4, -1};
    union {
        unsigned int word;
        struct {
            signed char complex_layout : 1;
            signed char piece0_is_color_breaker : 1;
            signed char piece1_is_color_breaker : 1;
            unsigned char pad : 5;
            unsigned char pad_bytes[3];
        } bits;
    } flags;
    PuzzleBoardCell* left_cell = 0;
    PuzzleBoardCell* right_cell = 0;
    int* columns;
    int* alternate_columns;
    int* rows;
    unsigned int below_type;
    unsigned int left_type;
    unsigned int right_type;
    unsigned int type;
    int candidate;
    int column;
    int row;

    flags.word = 0;
    alternate_columns = 0;
    switch (player->ai_move) {
    case 0:
        columns = move0_columns;
        rows = move0_rows;
        break;
    case 1:
        flags.bits.complex_layout = 1;
        columns = move1_columns;
        alternate_columns = move1_alternate_columns;
        rows = move1_rows;
        break;
    case 2:
        flags.bits.complex_layout = 1;
        columns = move2_columns;
        alternate_columns = move2_alternate_columns;
        rows = move2_rows;
        break;
    default:
        player->ai_move = 3;
        return;
    }

    player->ai_target_rotation = 0;
    player->ai_secondary_column = 0;
    if (flags.bits.complex_layout == 0) {
        for (candidate = 0; columns[candidate] >= 0; candidate++) {
            column = columns[candidate];
            if (player->board_rows[rows[candidate]][column].type == 0) {
                player->ai_target_column = column;
                return;
            }
        }
    } else {
    type = player->current_pair[0].type;
    if (type >= 4 && type <= 7) {
        flags.bits.piece0_is_color_breaker = 1;
    }
    type = player->current_pair[1].type;
    if (type >= 4 && type <= 7) {
        flags.bits.piece1_is_color_breaker = 1;
    }

    if (flags.bits.piece0_is_color_breaker == 0 &&
        flags.bits.piece1_is_color_breaker == 0) {
        for (candidate = 0; alternate_columns[candidate] >= 0; candidate++) {
            column = alternate_columns[candidate];
            if (player->board_rows[rows[candidate]][column].type == 0) {
                player->ai_target_column = column;
                return;
            }
        }
    } else {
    data.player = player;
    pzsm_ai_get_data(&data);
    if (data.normal_block_count <= 32) {
    for (candidate = 0; columns[candidate] >= 0; candidate++) {
        PuzzleBoardCell* below_cell;

        column = columns[candidate];
        row = rows[candidate];
        if (player->board_rows[row][column].type != 0) {
            continue;
        }
        player->ai_target_column = column;

        row = rows[candidate];
        for (; row >= 0; row--) {
            below_cell = &player->board_rows[row][column];
            if (below_cell->type != 0) {
                break;
            }
        }
        if (row < 0) {
            below_type = (unsigned int)-1;
        } else {
            below_type = below_cell->type;
            if (below_type >= 4) {
                if (below_type != PUZZLE_BLOCK_WILDCARD) {
                    below_type -= 4;
                } else {
                    below_type = 0;
                }
            }
        }

        for (row = player->active_row; row >= 0; row--) {
            left_cell = &player->board_rows[row][candidate - 1];
            if (left_cell->type != 0) {
                break;
            }
        }
        if (row == player->active_row) {
            left_type = (unsigned int)-16;
        } else if (row < 0) {
            left_type = (unsigned int)-1;
        } else {
            left_type = left_cell->type;
            if (left_type >= 4) {
                if (left_type != PUZZLE_BLOCK_WILDCARD) {
                    left_type -= 4;
                } else {
                    left_type = 0;
                }
            }
        }

        for (row = player->active_row; row >= 0; row--) {
            right_cell = &player->board_rows[row][candidate + 1];
            if (right_cell->type != 0) {
                break;
            }
        }
        if (row == player->active_row) {
            right_type = (unsigned int)-16;
        } else if (row < 0) {
            right_type = (unsigned int)-1;
        } else {
            right_type = right_cell->type;
            if (right_type >= 4) {
                if (right_type != PUZZLE_BLOCK_WILDCARD) {
                    right_type -= 4;
                } else {
                    right_type = 0;
                }
            }
        }

        if (flags.bits.piece0_is_color_breaker != 0 &&
            flags.bits.piece1_is_color_breaker != 0) {
            if (below_type == (unsigned int)-1) {
                return;
            }
            if (player->current_pair[0].type == player->current_pair[1].type) {
                type = player->current_pair[0].type;
                if (type >= 4) {
                    if (type != PUZZLE_BLOCK_WILDCARD) {
                        type -= 4;
                    } else {
                        type = 0;
                    }
                }
                if (below_type == type) {
                    continue;
                }
                if (left_type == (unsigned int)-1 ||
                    right_type == (unsigned int)-16) {
                    player->ai_target_rotation = 3;
                } else if (right_type == (unsigned int)-1 ||
                           left_type == (unsigned int)-16) {
                    player->ai_target_rotation = 1;
                } else {
                    type = player->current_pair[1].type;
                    if (type >= 4) {
                        if (type != PUZZLE_BLOCK_WILDCARD) {
                            type -= 4;
                        } else {
                            type = 0;
                        }
                    }
                    if (left_type == type) {
                        player->ai_target_rotation = 1;
                    } else {
                        player->ai_target_rotation = 3;
                    }
                }
                return;
            }

            type = player->current_pair[0].type;
            if (type >= 4) {
                if (type != PUZZLE_BLOCK_WILDCARD) {
                    type -= 4;
                } else {
                    type = 0;
                }
            }
            if (below_type != type) {
                continue;
            }
            player->ai_target_rotation = 2;
            return;
        }

        if (flags.bits.piece0_is_color_breaker != 0) {
            type = player->current_pair[0].type;
            if (type >= 4) {
                if (type != PUZZLE_BLOCK_WILDCARD) {
                    type -= 4;
                } else {
                    type = 0;
                }
            }
            if (below_type == type) {
                continue;
            }
            type = player->current_pair[1].type;
            if (left_type == type || right_type == (unsigned int)-16) {
                player->ai_target_rotation = 3;
            } else if (right_type == type || left_type == (unsigned int)-16) {
                player->ai_target_rotation = 1;
            } else if (left_type == (unsigned int)-1) {
                player->ai_target_rotation = 3;
            } else {
                player->ai_target_rotation = 1;
            }
            return;
        }

        type = player->current_pair[1].type;
        if (type >= 4) {
            if (type != PUZZLE_BLOCK_WILDCARD) {
                type -= 4;
            } else {
                type = 0;
            }
        }
        if (below_type == type) {
            continue;
        }
        type = player->current_pair[0].type;
        if (left_type == type || right_type == (unsigned int)-16) {
            player->ai_target_rotation = 1;
            player->ai_target_column--;
        } else if (right_type == type ||
                   left_type == (unsigned int)-16) {
            player->ai_target_rotation = 3;
            player->ai_target_column++;
        } else if (left_type == (unsigned int)-1) {
            player->ai_target_rotation = 1;
            player->ai_target_column--;
        } else {
            player->ai_target_rotation = 3;
            player->ai_target_column++;
        }
        return;
    }
    }
    }
    }

    player->ai_move = 3;
}

static int pz_ai_decide_superbomb(PuzzlePlayerState* player) {
    PuzzleAiColorCount colors[4];
    unsigned int type;
    unsigned int swap_color;
    int superbomb_piece;
    int rotation;
    int swap_count;
    int row;
    int column;
    int color_index;
    int index;

    superbomb_piece = 0;
    for (index = 0; index < 2; index++) {
        if (player->current_pair[index].type == PUZZLE_BLOCK_SUPERBREAKER) {
            break;
        }
        superbomb_piece++;
    }
    if (superbomb_piece >= 2) {
        return 0;
    }

    rotation = 0;
    if (superbomb_piece != 0) {
        rotation = 2;
    }
    for (column = 0; column < 8; column++) {
        if (player->board[column].type == 0) {
            player->ai_target_rotation = rotation;
            player->ai_secondary_rotation = rotation;
            player->ai_target_column = column;
            player->ai_secondary_column = column;
            return 1;
        }
    }

    memset(colors, 0, sizeof(colors));
    for (color_index = 0; color_index < 4; color_index++) {
        colors[color_index].color = color_index;
    }

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            type = player->board[row * 8 + column].type;
            if (type != 0) {
                if (type >= 4) {
                    if (type != PUZZLE_BLOCK_WILDCARD) {
                        type -= 4;
                    } else {
                        type = 0;
                    }
                }
                colors[type].count++;
            }
        }
    }

    for (color_index = 1; color_index < 4; color_index++) {
        index = color_index;
        for (;;) {
            if (colors[index - 1].count >= colors[index].count) {
                break;
            }
            swap_color = colors[index - 1].color;
            swap_count = colors[index - 1].count;
            colors[index - 1].color = colors[index].color;
            colors[index - 1].count = colors[index].count;
            colors[index].color = swap_color;
            colors[index].count = swap_count;
            if (index <= 1) {
                break;
            }
            index--;
        }
    }

    column = puzzle_ai_find_superbomb_column(player, colors);

    player->ai_target_rotation = rotation;
    player->ai_secondary_rotation = rotation;
    player->ai_target_column = column;
    player->ai_secondary_column = column;
    return 1;
}

static void pzsm_ai_get_data(PuzzleAiData* data) {
    int row;
    int column;

    data->breaker_count = 0;
    data->highest_occupied_row = 0;
    data->flags.word = 0;

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            unsigned int type =
                data->player->board[row * 8 + column].type;

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

    if (data->player->current_pair[0].type == PUZZLE_BLOCK_SUPERBREAKER ||
        data->player->current_pair[1].type == PUZZLE_BLOCK_SUPERBREAKER ||
        data->player->next_pair[0].type == PUZZLE_BLOCK_SUPERBREAKER ||
        data->player->next_pair[1].type == PUZZLE_BLOCK_SUPERBREAKER) {
        data->flags.bits.has_superbreaker = 1;
    }

    if ((data->player->current_pair[0].type >= 4 &&
         data->player->current_pair[0].type <= 7) ||
        (data->player->current_pair[1].type >= 4 &&
         data->player->current_pair[1].type <= 7) ||
        (data->player->next_pair[0].type >= 4 &&
         data->player->next_pair[0].type <= 7) ||
        (data->player->next_pair[1].type >= 4 &&
         data->player->next_pair[1].type <= 7)) {
        data->flags.bits.has_breaker = 1;
    }
}

/* Near miss: exact code; only the shared 0.5f relocation label differs. */
static int puzzle_fighter_mode_play__supermove(PuzzlePlayerState* player,
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
    pz_fighter_event(&pz_event);

    player->mode_step = puzzle_fighter_mode_play__supermove_fade_out;
    return 1;
}

/* Near miss: exact code; remaining difference is a constant reloc label. */
static int puzzle_fighter_mode_play__supermove_fade_out(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    int object_index;

    if (g_puzzle_music != 0) {
        set_snd_vol(g_puzzle_music, puzzle_ctrl->puzzle_music_channel,
                    (float)(puzzle_ctrl->supermove_phase_ticks / 80));
    }

    for (object_index = 0; object_index < 2; object_index++) {
        pfx_2d_obj_set_alpha(
            puzzle_ctrl->supermove_fade_objects[object_index],
            (unsigned char)((unsigned char)puzzle_ctrl->supermove_phase_ticks *
                                4 +
                            80));
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
    if (puzzle_ctrl->supermove_phase_ticks ==
        pz_super_move_table[player->selected_supermove].windup_event_tick) {
        pz_event.type = 0x10;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
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
        pz_fighter_event(&pz_event);
    }
    return 1;
}

/* Near miss: exact operations; remaining differences are registers/reloc. */
static int puzzle_fighter_mode_play__supermove_wind_down(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    int object_index;
    int old_ticks;

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
                (unsigned char)puzzle_ctrl->supermove_phase_ticks * 25);
        }
    } else {
        player->mode_step = puzzle_fighter_mode_play__supermove_done;
    }
    return 1;
}

/* Near miss: exact code; remaining difference is a constant reloc label. */
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
    return 0;
}

static int puzzle_fighter_mode_play__supermove_sleep(
    PuzzlePlayerState* player, PuzzlePlayerState* opponent) {
    return 1;
}

static int pzsm_invincible(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    return 0;
}

static int pzsm_kancel(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent) {
    return 0;
}
/* Near miss: exact size; address decomposition and register allocation only. */
static int pzsm_raise_up(PuzzlePlayerState* player,
                         PuzzlePlayerState* opponent) {
    int row;
    int column;

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = player->supermove_amount;
        player->supermove_delay_ticks = 0;
    }
    if (player->supermove_delay_ticks != 0) {
        player->supermove_delay_ticks--;
        return 1;
    }
    if (player->supermove_phase_ticks == 0) {
        return 0;
    }

    player->supermove_delay_ticks = 15;
    if (opponent->active_cell != 0) {
        if (opponent->active_row >= 13) {
            if (opponent->board[12 * 8 + opponent->active_column].type != 0) {
                opponent->active_cell = 0;
                opponent->saved_mode_step =
                    puzzle_fighter_mode_play__supermove_im_dead;
            }
        } else {
            opponent->active_row++;
            opponent->active_cell =
                &opponent->board[opponent->active_row * 8] +
                opponent->active_column;
        }
    }

    for (row = 11; row >= 0; row--) {
        for (column = 0; column < 8; column++) {
            PuzzleBoardCell* cell =
                &opponent->board[row * 8 + column];

            memcpy(cell + 8, cell, sizeof(*cell));
        }
    }
    for (column = 0; column < 8; column++) {
        PuzzleBoardCell* cell = &opponent->board[column];
        int sequence_index;
        int type;

        do {
            if (player->counter_sequence_index < 0) {
                player->counter_sequence_index =
                    puzzle_ctrl->piece_sequence_length - 1;
            }
            sequence_index = player->counter_sequence_index;
            type = puzzle_ctrl->piece_sequence[sequence_index];
            player->counter_sequence_index = sequence_index - 1;
        } while (type == PUZZLE_BLOCK_SUPERBREAKER);

        if (type >= 4 && type != PUZZLE_BLOCK_WILDCARD) {
            type -= 4;
            if (type == 0) {
                type = PUZZLE_BLOCK_WILDCARD;
            }
        }
        cell->type = type;
    }
    opponent->match_delay = 40;
    opponent->flags2_bits.board_shift_active = 1;
    pan_snd_req(0x1B00, opponent->sound_pan);
    player->supermove_phase_ticks--;
    return 1;
}

/*
 * Recovery in progress: pzsm_rain_dance 94.16%, retail 0x860/current 0x878.
 * Retail behavior and access widths are recovered, including staging new rain
 * pieces in board row 12. The effect-handle lifetime is nonalgorithmic, but the
 * two-level placement-loop exit remains structural rather than emission-only.
 */
static int pzsm_rain_dance(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    int row;
    int column;
    int placed_count;
    PuzzleBoardCell* drop_row;

    if (player->supermove_state == 0) {
        int storm_handle;

        player->supermove_state = 19;
        player->supermove_phase_ticks = player->supermove_amount;
        player->supermove_delay_ticks = 0;
        player->counter_drop_delay = 0;
        opponent->saved_mode_step =
            puzzle_fighter_mode_play__collapse_holes;
        memset(&pzsm_raindance_data, 0, sizeof(pzsm_raindance_data));

        pzsm_raindance_data.rain_object = load_wiff_screen_pfxobj(
            player->pfx_slot, 0x08130000, 0x6020,
            &pzsm_raindance_data.rain_anim, 0, 0);
        if (pzsm_raindance_data.rain_object == 0) {
            pzsm_release_rain_dance_resources();
            return 0;
        }
        pzsm_raindance_data.splash_object = load_wiff_screen_pfxobj(
            player->pfx_slot, 0x08130001, 0x6020,
            &pzsm_raindance_data.splash_anim, 0, 0);
        if (pzsm_raindance_data.splash_object == 0) {
            pzsm_release_rain_dance_resources();
            return 0;
        }

        pzsm_raindance_data.rain_object->x =
            art_puzzle_fighter_static_tbl
                .placements[opponent->event_player + 1]
                .x;
        pzsm_raindance_data.rain_object->y = screen_height;
        pzsm_raindance_data.rain_object->scale_x = 1.65f;
        pzsm_raindance_data.rain_object->scale_y = 1.35f;
        set_ani_texture_framerate(pzsm_raindance_data.rain_anim, 0.35f);

        pzsm_raindance_data.splash_object->x = -1000;
        pzsm_raindance_data.splash_object->y =
            art_puzzle_fighter_static_tbl
                    .placements[opponent->event_player + 1]
                    .y +
            5;
        set_ani_texture_framerate(pzsm_raindance_data.splash_anim, 0.5f);
        player->supermove_cleanup = pzsm_rain_dance_cleanup;
        pzsm_raindance_data.phase = 1;
        pzsm_raindance_data.rain_target_y =
            art_puzzle_fighter_static_tbl
                    .placements[opponent->event_player + 1]
                    .y +
            300;

        storm_handle = fx_by_owner(
            PUZZLE_STRINGS + PUZZLE_STORM_EFFECT_STRING,
            player->event_player == 0 ? 1 : 2);
        if (storm_handle != 0) {
            void* storm = pfx_from_handle(storm_handle);
            PuzzleStormEmitter* emitter = (PuzzleStormEmitter*)pfx_get_emitter(
                (PfxEmitterTableView*)((unsigned char*)storm + 0x40), 0);
            float storm_x;

            emitter->flag_bits.disabled = 0;
            storm_x = 1.55f * opponent->sound_pan;
            emitter = (PuzzleStormEmitter*)pfx_get_emitter(
                (PfxEmitterTableView*)((unsigned char*)storm + 0x40), 0);
            ((PuzzleStormTransform*)emitter->transform)->x = storm_x;
        }
    }

    if (pzsm_raindance_data.phase == 1) {
        if (pzsm_raindance_data.rain_object->y >
            pzsm_raindance_data.rain_target_y) {
            pzsm_raindance_data.rain_object->y -= 5;
            return 1;
        }
        pzsm_raindance_data.splash_object->y += 50;
        pzsm_raindance_data.phase = 2;
        return 1;
    }

    if (pzsm_raindance_data.phase == 2) {
        int next_tick;

        if ((player->counter_drop_delay & 0x38) != 0) {
            int burst_tick = player->counter_drop_delay & 7;

            if (burst_tick == 0) {
                pzsm_raindance_data.splash_object->x =
                    pzsm_raindance_data.rain_object->x +
                    randu0(6) * 25 + 4;
            } else if (burst_tick > randu0(4) + 2) {
                pzsm_raindance_data.splash_object->x = -1000;
            }
            if (((player->counter_drop_delay & 8) != 0
                     ? player->counter_drop_delay & 0x37
                     : player->counter_drop_delay & 0x1F) == 0) {
                pan_snd_req(0x1B01, opponent->sound_pan);
            }
        }
        next_tick = player->counter_drop_delay + 1;
        player->counter_drop_delay = next_tick;
        if (next_tick < 60) {
            return 1;
        }
        player->counter_drop_delay = 0;
        pzsm_raindance_data.phase = 0;
        pzsm_raindance_data.splash_object->x = -1000;
        pan_snd_req(0x1B02, opponent->sound_pan);
        return 1;
    }

    if (puzzle_fighter_fill_holes(opponent) != 0) {
        pzsm_raindance_data.flag_bits.holes_filling = 1;
    } else {
        if (pzsm_raindance_data.flag_bits.holes_filling != 0) {
            pan_snd_req(0x1B08, opponent->sound_pan);
        }
        pzsm_raindance_data.flag_bits.holes_filling = 0;
    }

    if (player->counter_drop_delay != 0) {
        player->counter_drop_delay--;
        return 1;
    }
    if (player->supermove_phase_ticks == 0) {
        if (pzsm_raindance_data.rain_object->y < screen_height) {
            pzsm_raindance_data.rain_object->y += 2;
            return 1;
        }
        pzsm_release_rain_dance_resources();
        player->supermove_cleanup = 0;
        return 0;
    }

    placed_count = 0;
    drop_row = &opponent->board[12 * 8];
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            PuzzleBoardCell* cell = &opponent->board[row * 8 + column];
            int previous_row;
            int previous_column;
            int distance;
            int type;

            if (cell->type != 0) {
                continue;
            }
            previous_row =
                ((int)(player->supermove_delay_ticks & 0xEFFF0000U)) >> 16;
            if (previous_row == row) {
                previous_column =
                    (unsigned short)player->supermove_delay_ticks;
                distance = previous_column - column;
                if (previous_column < column) {
                    distance = column - previous_column;
                }
                if (distance <= 1) {
                    continue;
                }
            }

            if (row >= opponent->active_row - 1 &&
                opponent->active_cell != 0) {
                if (opponent->active_row >= 13) {
                    if (opponent->board[12 * 8 + opponent->active_column].type !=
                        0) {
                        opponent->active_cell = 0;
                        opponent->saved_mode_step =
                            puzzle_fighter_mode_play__supermove_im_dead;
                    }
                } else {
                    opponent->active_row++;
                    opponent->active_cell =
                        &opponent->board[opponent->active_row * 8 +
                                         opponent->active_column];
                }
            }

            do {
                if (player->counter_sequence_index < 0) {
                    player->counter_sequence_index =
                        puzzle_ctrl->piece_sequence_length - 1;
                }
                type = puzzle_ctrl
                           ->piece_sequence[player->counter_sequence_index];
                if (type == PUZZLE_BLOCK_SUPERBREAKER) {
                    player->counter_sequence_index--;
                }
            } while (type == PUZZLE_BLOCK_SUPERBREAKER);

            if (type >= 4 && type != PUZZLE_BLOCK_WILDCARD) {
                type -= 4;
                if (type == 0) {
                    type = PUZZLE_BLOCK_WILDCARD;
                }
            }
            player->counter_sequence_index--;
            drop_row[column].type = type;
            player->counter_drop_delay = 5;
            player->supermove_phase_ticks--;
            player->supermove_delay_ticks = (row << 16) + column;
            placed_count++;
            if (player->supermove_phase_ticks == 0 || placed_count >= 2) {
                break;
            }
        }
        if (player->supermove_phase_ticks == 0 || placed_count >= 2) {
            break;
        }
    }

    {
        int target_y = art_puzzle_fighter_static_tbl
                           .placements[opponent->event_player + 1]
                           .y +
                       (row + 1) * 25;

        if (pzsm_raindance_data.rain_object->y < target_y) {
            pzsm_raindance_data.rain_object->y += 5;
        }
    }
    if (player->counter_drop_delay == 0) {
        opponent->active_cell = 0;
        pzsm_release_rain_dance_resources();
        return 0;
    }
    return 1;
}

/*
 * Near match: pzsm_rain_dance_cleanup 99.67%, exact size and instructions;
 * the remaining three argument mismatches are storm-string relocation labels.
 */
static void pzsm_rain_dance_cleanup(void) {
    reset_effect(PUZZLE_STRINGS + PUZZLE_STORM_EFFECT_STRING);

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

/* Near match: pzsm_lower_down 99.53%; exact size, register operands only. */
static int pzsm_lower_down(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    int removed = 0;
    int column;

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
    player->flags2_bits.board_shift_active = 1;
    pan_snd_req(0x1AFE, player->sound_pan);
    player->supermove_phase_ticks--;
    return 1;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
/* Near miss: exact code; remaining differences are constant reloc labels. */
static float p_pzsm_invisible(void) {
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
    if (pdata->player->invisibility_ticks < 0x1014) {
        return 1.0f;
    }

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            pdata->player->board[row * 8 + column].flag_bits.effect_bit = 0;
        }
    }
    pdata->player->invisibility_ticks = 0;
    return -1.0f;
}

/* Near miss: exact size; byte promotion and register allocation only. */
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

/* Near miss: exact size and algorithm; one color-count store is scheduled
 * before the visual clear, with the remaining differences register coloring. */
static int pzsm_jumble(PuzzlePlayerState* player,
                       PuzzlePlayerState* opponent) {
    int color_counts[8];
    int color_rolls[8];
    PuzzleBoardCell* cell;
    unsigned int type;
    unsigned int compared_type;
    int random_value;
    int occupied_count;
    int best_roll;
    int selected_color;
    int row;
    int column;
    int color;

    occupied_count = 0;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = player->supermove_amount;
        player->supermove_delay_ticks = 120;
        opponent->saved_mode_step =
            puzzle_fighter_mode_play__collapse_holes;
    }

    if ((player->supermove_delay_ticks & 3) != 0) {
        player->supermove_delay_ticks--;
        return 1;
    }
    if (player->supermove_delay_ticks == 0) {
        return 0;
    }

    for (color = 0; color < 8; color++) {
        color_counts[color] = 0;
    }
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            type = opponent->board[row * 8 + column].type;
            if (type == 0) {
                continue;
            }
            for (color = 0; color < 8; color++) {
                compared_type = PUZZLE_BLOCK_WILDCARD;
                if (color != 0) {
                    compared_type = color;
                }
                if (type == compared_type) {
                    occupied_count++;
                    color_counts[color]++;
                }
            }
            player->supermove_phase_ticks--;
            if (player->supermove_phase_ticks == 0) {
                break;
            }
        }
        if (player->supermove_phase_ticks == 0) {
            break;
        }
    }

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            cell = &opponent->board[row * 8 + column];
            if (cell->type == 0) {
                continue;
            }

            for (color = 0; color < 8; color++) {
                if (color_counts[color] != 0) {
                    random_value = randu0((unsigned short)(
                        occupied_count + color_counts[color]));
                } else {
                    random_value = -1;
                }
                color_rolls[color] = random_value;
            }
            best_roll = -1;
            selected_color = -1;
            for (color = 0; color < 8; color++) {
                if (color_rolls[color] > best_roll) {
                    best_roll = color_rolls[color];
                    selected_color = color;
                }
            }

            type = PUZZLE_BLOCK_WILDCARD;
            if (selected_color != 0) {
                type = selected_color;
            }
            occupied_count--;
            cell->type = type;
            color_counts[selected_color]--;
            cell->visual = 0;
            if (occupied_count == 0) {
                break;
            }
        }
        if (occupied_count == 0) {
            break;
        }
    }

    pan_snd_req(0x1AFD, opponent->sound_pan);
    player->supermove_phase_ticks = player->supermove_amount;
    player->supermove_delay_ticks--;
    return 1;
}

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

/* Recovery in progress: the reverse board scans, packed marker, selection cap,
 * and completion state match retail. Structured loop exits repeat the two stop
 * comparisons, cascading into induction-register coloring and 12 extra bytes.
 * This remains structural, not an emission-only near miss. */
static int pzsm_float(PuzzlePlayerState* player,
                      PuzzlePlayerState* opponent) {
    PuzzleBoardCell* cell;
    int remaining;
    int selected_count;
    int raised_count;
    int visited_count;
    int previous_row;
    int previous_column;
    int distance;
    int scan_complete;
    int row;
    int column;

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = player->supermove_amount;
        player->supermove_delay_ticks = 0;
        player->counter_drop_delay = 0;
        player->event_bits.counter_drops_active = 0;
        pan_snd_req(0x1AFF, player->sound_pan);
    }

    if (player->counter_drop_delay != 0) {
        player->counter_drop_delay--;
        for (row = 13; row >= 0; row--) {
            for (column = 7; column >= 0; column--) {
                cell = &player->board[row * 8 + column];
                if (cell->type != 0 && cell->fall_ticks != 0) {
                    cell->fall_ticks += 20;
                }
            }
        }
        return 1;
    }

    if (player->supermove_phase_ticks == 0) {
        for (row = 13; row >= 0; row--) {
            for (column = 7; column >= 0; column--) {
                cell = &player->board[row * 8 + column];
                if (cell->type != 0 && cell->fall_ticks != 0) {
                    memset(cell, 0, sizeof(*cell));
                }
            }
        }
        return 0;
    }

    remaining = player->supermove_phase_ticks;
    selected_count = 0;
    raised_count = 0;
    visited_count = 0;
    for (row = 13; row >= 0; row--) {
        for (column = 7; column >= 0; column--) {
            cell = &player->board[row * 8 + column];
            if (cell->type == 0) {
                continue;
            }

            visited_count++;
            previous_row =
                ((int)(player->supermove_delay_ticks & 0xEFFF0000U)) >> 16;
            if (previous_row == row) {
                previous_column =
                    (unsigned short)player->supermove_delay_ticks;
                distance = previous_column - column;
                if (previous_column < column) {
                    distance = column - previous_column;
                }
                if (distance < 3 && cell->fall_ticks == 0 &&
                    player->supermove_phase_ticks - visited_count > 3) {
                    continue;
                }
            }

            remaining--;
            if (cell->fall_ticks == 0) {
                if (selected_count < 3) {
                    selected_count++;
                    cell->fall_ticks = 20;
                    if (player->supermove_phase_ticks - visited_count >= 3 &&
                        player->supermove_delay_ticks >= 0) {
                        player->supermove_delay_ticks =
                            (row << 16) + column;
                    } else {
                        player->supermove_delay_ticks = -1;
                    }
                }
            } else {
                cell->fall_ticks += 20;
            }

            if (cell->fall_ticks > 325) {
                raised_count++;
            }
            if (remaining == 0 || selected_count >= 3) {
                break;
            }
        }
        if (remaining == 0 || selected_count >= 3) {
            break;
        }
    }

    if (raised_count >= player->supermove_phase_ticks) {
        player->supermove_phase_ticks = 0;
    } else {
        scan_complete = 0;
        if (row < 0 && column < 0) {
            scan_complete = 1;
        }
        if (scan_complete != 0) {
            remaining = visited_count;
        }
        player->supermove_phase_ticks = remaining;
        player->counter_drop_delay = 5;
    }
    return 1;
}

/* Near miss: complete typed algorithm and retail resource/phase ownership;
 * one instruction of loop lowering plus register allocation remains. */
static int pzsm_edge_clear(PuzzlePlayerState* player,
                           PuzzlePlayerState* opponent) {
    int changed;
    int object_index;
    int edge_index;

    changed = 0;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = 12;
        player->supermove_delay_ticks = 0;
        pzsm_edger_data.phase = 1;
        pzsm_edger_data.step_count = 0;

        for (object_index = 0; object_index < 2; object_index++) {
            ScreenObj* loaded_object = load_2d_pfxobj(
                player->pfx_slot, 0x6020, (char*)0x080C0001, 0, 0x3A);
            ScreenObj** object =
                &pzsm_edger_data.edge_objects[object_index];

            *object = loaded_object;
            if (*object != 0) {
                (*object)->x = art_puzzle_fighter_static_tbl
                                   .placements[player->event_player + 1]
                                   .x +
                               6;
                if (object_index != 0) {
                    (*object)->x += 175;
                }
                (*object)->y = screen_height - 50;
            }
        }
        player->supermove_cleanup = pzsm_edger_cleanup;
        pan_snd_req(0x1AFA, player->sound_pan);
    }

    if (puzzle_fighter_find_match(player) != 0 &&
        player->supermove_phase_ticks >= 14) {
        return 1;
    }

    if (pzsm_edger_data.phase == 1) {
        if (player->supermove_phase_ticks < 0 &&
            player->supermove_delay_ticks == 0) {
            pzsm_edger_data.phase = 2;
            return 1;
        }

        pzsm_edger_data.edge_objects[0]->y -= 8;
        pzsm_edger_data.edge_objects[1]->y -= 8;
    } else {
        player->supermove_phase_ticks++;
        if (player->supermove_phase_ticks > 12) {
            for (object_index = 0; object_index < 2; object_index++) {
                ScreenObj** object =
                    &pzsm_edger_data.edge_objects[object_index];

                if (*object != 0) {
                    destroy_screen_obj(*object);
                }
                *object = 0;
            }
            player->supermove_cleanup = 0;
            player->saved_mode_step =
                puzzle_fighter_mode_play__collapse_holes;
            return 0;
        }

        pzsm_edger_data.edge_objects[0]->y += 8;
        pzsm_edger_data.edge_objects[1]->y += 8;
        return 1;
    }
    if (player->supermove_delay_ticks != 0) {
        player->supermove_delay_ticks--;
        return 1;
    }

    {
        for (edge_index = 0; edge_index < 2; edge_index++) {
            PuzzleBoardCell* row =
                &player->board[player->supermove_phase_ticks * 8];
            PuzzleBoardCell* cell = &row[edge_index * 7];

            if (cell->type != 0) {
                int color;

                cell->state = 0x2B;
                cell->flag_bits.matched = 0;
                cell->flag_bits.effect_bit = 0;
                for (color = 0; color < 4; color++) {
                    int is_wildcard = color == 0;
                    unsigned int type = is_wildcard
                                            ? PUZZLE_BLOCK_WILDCARD
                                            : (unsigned int)color;

                    if (cell->type == type ||
                        cell->type == (unsigned int)color + 4) {
                        cell->visual = puzzle_ctrl->block_visuals[color];
                        cell->type = is_wildcard ? PUZZLE_BLOCK_WILDCARD
                                                 : (unsigned int)color;
                        changed = 1;
                        break;
                    }
                }
            }
        }
    }

    player->supermove_phase_ticks--;
    pzsm_edger_data.step_count++;
    if (changed != 0 && (pzsm_edger_data.step_count & 1) != 0) {
        pan_snd_req(0x1B0F, player->sound_pan);
    }
    player->supermove_delay_ticks = 2;
    return 1;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

/*
 * Near match: pzsm_edger_cleanup 98.80%, exact size and opcode sequence;
 * the five remaining argument mismatches are loop-register allocation only.
 */
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

/* Near miss: exact size and instruction sequence; objdiff reports only
 * argument mismatches from register allocation and the 0.5f relocation label. */
static int pzsm_drill(PuzzlePlayerState* player,
                      PuzzlePlayerState* opponent) {
    int changed;

    changed = 0;
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = 12;
        player->supermove_delay_ticks = 0;

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

        pzsm_drill_data.drill_object = load_wiff_screen_pfxobj(
            player->pfx_slot, 0x080B0000, 0x6020,
            &pzsm_drill_data.drill_anim, 0, 0);
        if (pzsm_drill_data.drill_object == 0) {
            return 0;
        }
        pzsm_drill_data.drill_object->x =
            art_puzzle_fighter_static_tbl
                    .placements[player->event_player + 1]
                    .x +
            75;
        pzsm_drill_data.drill_object->y = screen_height;
        set_ani_texture_framerate(pzsm_drill_data.drill_anim, 0.5f);
        player->supermove_cleanup = pzsm_drill_cleanup;
        pzsm_drill_data.sound_handles[0] =
            pan_snd_req(0x1AF8, player->sound_pan);
    }

    if (puzzle_fighter_find_match(player) != 0 &&
        player->supermove_phase_ticks < 0) {
        int target_y = art_puzzle_fighter_static_tbl
                           .placements[player->event_player + 1]
                           .y +
                       5;

        if (pzsm_drill_data.drill_object->y > target_y) {
            pzsm_drill_data.drill_object->y -= 4;
        }
        return 1;
    }
    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }

    if (player->supermove_delay_ticks != 0) {
        if (player->supermove_phase_ticks >= 0) {
            pzsm_drill_data.drill_object->y -= 4;
        } else {
            pzsm_drill_data.drill_object->y += 20;
            if (pzsm_drill_data.sound_handles[1] != 0) {
                snd_stop(pzsm_drill_data.sound_handles[1]);
            }
            pzsm_drill_data.sound_handles[1] = 0;
        }
        player->supermove_delay_ticks--;
        return 1;
    }

    if (player->supermove_phase_ticks < 0) {
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
        player->supermove_cleanup = 0;
        player->saved_mode_step = puzzle_fighter_mode_play__collapse_holes;
        return 0;
    }

    {
        int column;

        for (column = 3; column < 5; column++) {
            PuzzleBoardCell* cell =
                &player->board[player->supermove_phase_ticks * 8 + column];

            if (cell->type != 0) {
                int color;

                cell->state = 0x2B;
                cell->flag_bits.matched = 0;
                cell->flag_bits.effect_bit = 0;
                for (color = 0; color < 4; color++) {
                    int is_wildcard = color == 0;
                    unsigned int type = is_wildcard
                                            ? PUZZLE_BLOCK_WILDCARD
                                            : (unsigned int)color;

                    if (cell->type == type ||
                        cell->type == (unsigned int)color + 4) {
                        cell->visual = puzzle_ctrl->block_visuals[color];
                        cell->type = is_wildcard ? PUZZLE_BLOCK_WILDCARD
                                                 : (unsigned int)color;
                        changed = 1;
                        break;
                    }
                }
            }
        }
    }

    pzsm_drill_data.drill_object->y =
        art_puzzle_fighter_static_tbl
                .placements[player->event_player + 1]
                .y +
        (player->supermove_phase_ticks + 1) * 25;
    player->supermove_phase_ticks--;
    if (changed != 0) {
        pan_snd_req(0x1B0F, player->sound_pan);
        if (pzsm_drill_data.sound_handles[1] == 0) {
            pzsm_drill_data.sound_handles[1] =
                pan_snd_req(0x1AF9, player->sound_pan);
        }
    }
    player->supermove_delay_ticks = 7;
    return 1;
}

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

static int pzsm_double_bomb(PuzzlePlayerState* player,
                            PuzzlePlayerState* opponent) {
    PuzzleBoardCell* board;

    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        board = player->board;
        board[12 * 8 + 1].type = PUZZLE_BLOCK_SUPERBREAKER;
        board[12 * 8 + 6].type = PUZZLE_BLOCK_SUPERBREAKER;
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

/* Near matches: color wrappers differ only in equivalent compiler emission. */
static int pzsm_clear_yellow(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent) {
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
    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        pzsm_klear_kore(player, 3);
    }
    if (puzzle_fighter_find_match(player) != 0) {
        return 1;
    }
    return puzzle_fighter_fill_holes(player) != 0;
}

/* Near miss: exact size and operations; remaining differences are registers. */
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

            if (cell->type == color || cell->type == breaker_color) {
                unsigned int visual_index =
                    color == PUZZLE_BLOCK_WILDCARD ? 0 : color;

                if (cell->state == 0) {
                    cell->state = PUZZLE_CELL_BREAK_STATE;
                    cleared++;
                    cell->visual = puzzle_ctrl->block_visuals[visual_index];
                    cell->type = color;
                }
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

/* Recovery in progress: phase traversal, candidate exchange, and delays agree
 * with retail. The remaining 20 bytes are structured candidate-search
 * postchecks; clean loop and helper forms compile worse. This remains
 * structural, not an emission-only near miss. */
static int pzsm_arrange(PuzzlePlayerState* player,
                        PuzzlePlayerState* opponent) {
    int start_row = 0;
    int start_column = 0;
    int current_type = 0;
    int swaps;
    PuzzleBoardCell* board;

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_phase_ticks = 0;
        player->supermove_delay_ticks = 0;
    }
    if (puzzle_fighter_fill_holes(player) != 0) {
        return 1;
    }
    if (player->supermove_delay_ticks != 0) {
        player->supermove_delay_ticks--;
        return 1;
    }
    if (player->supermove_phase_ticks >= 4) {
        player->saved_mode_step = puzzle_fighter_mode_play__collapse_holes;
        return 0;
    }

    board = player->board;
    if (player->supermove_phase_ticks != 0) {

        while (start_column < 8) {
            start_row = 0;
            while (start_row < 14) {
                unsigned int type =
                    board[start_row * 8 + start_column].type;

                if (type == 0) {
                    break;
                }
                while (current_type !=
                           (type == PUZZLE_BLOCK_WILDCARD ? 0 : (int)type) &&
                       ++current_type < player->supermove_phase_ticks) {
                }
                if (current_type >= player->supermove_phase_ticks) {
                    break;
                }
                start_row++;
            }
            if (current_type >= player->supermove_phase_ticks) {
                break;
            }
            start_column++;
        }
    }

    swaps = 0;
    for (; start_column < 8; start_column++) {
        for (; start_row < 14; start_row++) {
            PuzzleBoardCell* cell =
                &player->board[start_row * 8 + start_column];
            unsigned int type = cell->type;

            if (type == 0 ||
                type == (player->supermove_phase_ticks == 0
                             ? PUZZLE_BLOCK_WILDCARD
                             : (unsigned int)player->supermove_phase_ticks)) {
                continue;
            }
            if (type == (unsigned int)player->supermove_phase_ticks + 4) {
                cell->type = player->supermove_phase_ticks == 0
                                 ? PUZZLE_BLOCK_WILDCARD
                                 : (unsigned int)player->supermove_phase_ticks;
            } else {
                int search_column = start_column;
                int search_row = start_row;
                PuzzleBoardCell* search_cell;

                for (; search_column < 8; search_column++) {
                    for (; search_row < 14; search_row++) {
                        search_cell =
                            &player->board[search_row * 8 + search_column];

                        if (search_cell->type != 0 &&
                            (search_cell->type ==
                                 (player->supermove_phase_ticks == 0
                                      ? PUZZLE_BLOCK_WILDCARD
                                      : (unsigned int)
                                            player->supermove_phase_ticks) ||
                             search_cell->type ==
                                 (unsigned int)player->supermove_phase_ticks +
                                     4)) {
                            break;
                        }
                    }
                    if (search_row < 14) {
                        break;
                    }
                    search_row = 0;
                }
                if (search_column >= 8) {
                    break;
                }
                search_cell->type = type;
                cell->type = player->supermove_phase_ticks == 0
                                 ? PUZZLE_BLOCK_WILDCARD
                                 : (unsigned int)player->supermove_phase_ticks;
                swaps++;
            }
        }
        start_row = 0;
    }

    if (swaps != 0) {
        player->match_delay = 40;
        player->flags2_bits.board_shift_active = 1;
        pan_snd_req(0x1AF3, player->sound_pan);
        player->supermove_delay_ticks = 15;
    }
    player->supermove_phase_ticks++;
    return 1;
}

/* Near miss: exact size and operations; remaining differences are registers. */
static int pzsm_antibreakers(PuzzlePlayerState* player,
                             PuzzlePlayerState* opponent) {
    int changed;
    int column;
    int row;

    if (player->supermove_state == 0) {
        player->supermove_state = 19;
        player->supermove_delay_ticks = 0;
    }
    if (puzzle_fighter_find_match(opponent) != 0) {
        return 1;
    }
    if (puzzle_fighter_fill_holes(opponent) != 0) {
        return 1;
    }
    if (player->supermove_delay_ticks != 0) {
        return 0;
    }

    changed = 0;
    player->supermove_delay_ticks = 1;
    for (column = 0; column < 8; column++) {
        for (row = 0; row < 14; row++) {
            PuzzleBoardCell* cell = &opponent->board[row * 8 + column];
            unsigned int type = cell->type;

            if (type != 0 && cell->state == 0) {
                int breaker_type;

                for (breaker_type = 4; breaker_type < 8; breaker_type++) {
                    if (breaker_type == (int)type) {
                        unsigned int color = breaker_type;

                        color -= 4;
                        cell->state = PUZZLE_CELL_BREAK_STATE;
                        cell->visual = puzzle_ctrl->block_visuals[color];
                        cell->type = breaker_type - 4 != 0
                                         ? breaker_type - 4
                                         : PUZZLE_BLOCK_WILDCARD;
                        changed++;
                        break;
                    }
                }
            }
        }
    }

    if (changed != 0) {
        pan_snd_req(0x1AF4, opponent->sound_pan);
    } else if (randu0(4) == 0) {
        snd_req((int)randu0(2) + 0x12);
    }
    return 1;
}

static PuzzleMessagePdata*
pz_display_supermove_msg_sideways(PuzzlePlayerState* player) {
    PuzzleMessagePdata* pdata;
    ScreenObj* image;
    MkProc* proc;
    int x;
    int y;

    proc = _create_mkproc_generic_nostack(
        0x6010, 0x1F, p_puzzle_fighter_chain_msg,
        sizeof(PuzzleMessagePdata), (MkHdr**)&pdata);
    if (proc == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(PuzzleMessagePdata), &pdata->hdr);
    if (player == puzzle_ctrl->players[0]) {
        pdata->velocity_x = 20;
        pdata->acceleration = -4;
        x = -22 - pz_super_move_table[player->selected_supermove].field_0C;
        if (pz_super_move_table[player->selected_supermove].field_0C >
            0x100) {
            x -= 95;
        }
    } else {
        pdata->velocity_x = -20;
        pdata->acceleration = 4;
        x = screen_width + 22;
        if (pz_super_move_table[player->selected_supermove].field_0C >
            0x100) {
            x += 95;
        }
        if (is_widescreen_mode() != 0) {
            x -= 80;
        }
    }

    pdata->lifetime_ticks =
        pz_super_move_table[player->selected_supermove].field_0C / 20 + 90;
    pdata->fade_start_ticks = 90;
    pdata->fade_delay_ticks = 60;
    pdata->alpha = 0xFF;
    pdata->alpha_step = 3;
    pdata->velocity_y = 0;
    y = art_puzzle_fighter_static_tbl.placements[3].y;
    image = load_2d_pfxobj(
        player->pfx_slot, 0x601D,
        pz_super_move_table[player->selected_supermove].message_art, 0,
        0x3A);
    if (image == 0) {
        if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
        return 0;
    }

    pdata->primary_image = image;
    pdata->primary_instance = image->instance;
    image->x = x;
    image->y = y;
    snd_req(pz_super_move_table[player->selected_supermove].message_sound);
    return pdata;
}

/* Near miss: exact size and operations; registers and reloc labels differ. */
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

    player->center_weight = (float)total_weight;
    player->center_weight = player->center_weight / 86.399994f;

    if (high_columns > 5) {
        if (player->center_event_bits.high == 0) {
            player->center_event_bits.high = 1;
            pz_event.type = 8;
            pz_event.player = player->event_player;
            pz_fighter_event(&pz_event);
        }
    } else if (peak_columns == 3 && low_or_empty_columns > 2) {
        if (player->center_event_bits.split == 0) {
            player->center_event_bits.split = 1;
            pz_event.type = 0x17;
            pz_event.player = player->event_player;
            pz_fighter_event(&pz_event);
        }
    }

    if (low_or_empty_columns > 7 &&
        player->center_event_bits.high != 0 &&
        player->center_event_bits.low == 0) {
        player->center_event_bits.low = 1;
        pz_event.type = 9;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
    }
}

/* Emission-only near match (94.96%, exact retail 0xC4 size). Both 8x14 board
 * scans and output stores agree; only induction-register coloring differs. */
void puzzle_fighter_get_num_blocks_on_screen(unsigned int* player1_blocks,
                                             unsigned int* player2_blocks) {
    PuzzlePlayerState* player;
    int column;
    int row;
    int total;

    total = 0;
    player = puzzle_ctrl->players[0];
    for (column = 0; column < 8; column++) {
        row = 13;
        for (;;) {
            if (player->board[row * 8 + column].type != 0) {
                total += row;
                total++;
                break;
            }
            row--;
        }
    }
    *player1_blocks = total;

    total = 0;
    player = puzzle_ctrl->players[1];
    for (column = 0; column < 8; column++) {
        row = 13;
        for (;;) {
            if (player->board[row * 8 + column].type != 0) {
                total += row;
                total++;
                break;
            }
            row--;
        }
    }
    *player2_blocks = total;
}

static void update_super_bar_verts(PuzzlePlayerState* player) {
    static int super_bar_flash_table[16] = {
        10, 10, 10, 10, 15, 15, 20, 20,
        25, 25, 30, 30, 35, 35, 40, 40,
    };
    ScreenObj* bar;
    float bar_start;
    float bar_end;
    int bar_x = 0x15E;
    int flash_index;

    if (player == puzzle_ctrl->players[0]) {
        bar_x = 0x10C;
    }
    bar_start =
        (float)art_puzzle_fighter_static_tbl.placements[11].y + 3.0f;

    if (player->super_active == 0) {
        flash_index = 0;
        bar_end = bar_start + player->super_bar;
    } else {
        bar_end = bar_start + 185.0f;
        if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
            network_pause_procs == 0) {
            if (player->super_windup_timer != 0) {
                player->super_windup_timer--;
            } else {
                player->super_active--;
                if (player->super_active < 16) {
                    player->super_windup_timer =
                        super_bar_flash_table[player->super_active];
                    pan_snd_req(0x1B0E, player->sound_pan);
                } else if (player->super_active > 17) {
                    player->super_windup_timer = 6;
                } else {
                    player->super_windup_timer = 420;
                }
            }
        }
        if (player->super_active == 16) {
            flash_index = 1;
        } else {
            flash_index = player->super_active & 1;
        }
    }

    bar = puzzle_ctrl->super_bar_objects[flash_index];
    bar->pfx2d->verts[0].y = bar_start;
    bar->pfx2d->verts[1].y = bar_end;
    bar->pfx2d->verts[2].y = bar_end;
    bar->pfx2d->verts[3].y = bar_start;
    bar->pfx2d->x = bar_x;
    bar->pfx2d->mirror = 1;
    pfx2d_begin_render();
    pfx2d_render(bar->pfx2d);
    pfx2d_end_render();
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

/* Near miss: body and layout match; one saved-proc move, validation branch
 * polarity, and string relocation labels remain compiler-emission residue. */
static void puzzle_fighter_display_block_count_msg(
    PuzzlePlayerState* player) {
    PuzzleMessagePdata* pdata;
    StringObj* text;
    MkProc* proc;
    int placement_index;

    if (player->block_count_message_proc != 0) {
        proc = player->block_count_message_proc;
        if (proc != 0) {
            pdata = (PuzzleMessagePdata*)pdata_of_proc(proc);
            text = pdata->text;
            if (text != 0) {
                if (text->instance != pdata->text_instance) {
                    text = 0;
                }
            } else {
                text = 0;
            }
            if (text != 0) {
                destroy_string_obj(text);
            }
            if (proc->instance != 0) {
                proc->hdr.typed_vtbl->destroy(&proc->hdr);
            }
        }
    }

    placement_index = player == puzzle_ctrl->players[0] ? 1 : 2;
    proc = _create_mkproc_generic_nostack(
        0x6010, 0x1F, p_puzzle_fighter_chain_msg,
        sizeof(PuzzleMessagePdata), (MkHdr**)&pdata);
    if (proc != 0) {
        zero_pdata_payload(sizeof(PuzzleMessagePdata), &pdata->hdr);
        pdata->lifetime_ticks = 0x57;
        pdata->fade_start_ticks = 0x3E;
        pdata->fade_delay_ticks = 0;
        pdata->alpha = 0xFF;
        pdata->alpha_step = 4;
        pdata->velocity_x = 0;
        pdata->velocity_y = -5;
        pdata->player = player;
        player->block_count_message_proc = proc;
    }

    sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_NUMBER_FORMAT_STRING,
            player->counter_drops_remaining);
    text = string_center_xy(
        0x601D, 0x0C, temp_80_char,
        art_puzzle_fighter_static_tbl.placements[placement_index].x + 100,
        screen_height, 0x3B);
    if (text == 0) {
        if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
    } else {
        pdata->text = text;
        pdata->text_instance = text->instance;
    }

    if (player->flags3_bits.counter_drops_active == 0) {
        player->flags3_bits.counter_drops_active = 1;
        player->block_count_anim_value = 0;
        player->block_count_anim_timer = 10;
    }
    if (player->block_count_font_11 != 0) {
        destroy_string_obj(player->block_count_font_11);
    }
    if (player->block_count_font_10 != 0) {
        destroy_string_obj(player->block_count_font_10);
    }
    player->block_count_font_11 =
        string_left_xy(0x601D, 0x0B, temp_80_char, 0, 0, 0x3D);
    player->block_count_font_10 =
        string_left_xy(0x601D, 0x0A, temp_80_char, 0, 0, 0x3D);
    if (player->block_count_font_11 != 0) {
        pull_string_obj(player->block_count_font_11);
    }
    if (player->block_count_font_10 != 0) {
        pull_string_obj(player->block_count_font_10);
    }
}

/* Near miss: typed art layout and algorithm match; one art-base copy folds,
 * cascading into nonvolatile-register coloring. */
static void puzzle_fighter_display_chain_msg(PuzzlePlayerState* player) {
    PuzzleArtPlacement* placements;
    PuzzleMessagePdata* pdata;
    ScreenObj* count_image;
    ScreenObj* chain_image;
    MkProc* proc;
    int x;
    int y;
    int sound_index;

    proc = _create_mkproc_generic_nostack(
        0x6010, 0x1F, p_puzzle_fighter_chain_msg,
        sizeof(PuzzleMessagePdata), (MkHdr**)&pdata);
    if (proc == 0) {
        return;
    }
    zero_pdata_payload(sizeof(PuzzleMessagePdata), &pdata->hdr);
    pdata->lifetime_ticks = 0x57;
    pdata->fade_start_ticks = 0x3E;
    pdata->fade_delay_ticks = 0;
    pdata->alpha = 0xFF;
    pdata->alpha_step = 4;
    pdata->velocity_y = 0;

    if (player == puzzle_ctrl->players[0]) {
        x = 0;
        pdata->velocity_x = 3;
        pdata->acceleration = -1;
    } else {
        pdata->velocity_x = -3;
        pdata->acceleration = 1;
        x = screen_width - 140;
        if (is_widescreen_mode() != 0) {
            x -= 80;
        }
    }

    placements = art_puzzle_fighter_static_tbl.placements;
    y = placements[11].y;
    count_image = load_2d_pfxobj(
        0x70033, 0x601D,
        (char*)art_puzzle_fighter_chains_tbl[player->best_chain_count + 1],
        0, 0x3A);
    if (count_image == 0) {
        if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
        return;
    }
    pdata->secondary_image = count_image;
    pdata->secondary_instance = count_image->instance;
    count_image->x = x;
    count_image->y = y;

    chain_image = load_2d_pfxobj(
        0x70033, 0x601D, (char*)art_puzzle_fighter_chains_tbl[1],
        0, 0x3A);
    if (chain_image == 0) {
        if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
        return;
    }
    pdata->primary_image = chain_image;
    pdata->primary_instance = chain_image->instance;
    chain_image->x = x + 25;
    chain_image->y = y;

    sound_index = 4;
    if (player->best_chain_count < 5) {
        sound_index = player->best_chain_count - 1;
    }
    pan_snd_req(chain_sound_burst_table[sound_index], player->sound_pan);
}

/* Near miss: exact size and operations; r30/r31 and reloc labels differ. */
static void puzzle_fighter_display_floor_msg(PuzzlePlayerState* player,
                                             int fixed_message) {
    PuzzleLocalizedImagePlacement* placement = 0;
    PuzzleMessagePdata* pdata;
    ScreenObj* image;
    MkProc* proc;
    unsigned int image_id;
    int entry_index;
    int x;

    proc = _create_mkproc_generic_nostack(
        0x6010, 0x1F, p_puzzle_fighter_chain_msg,
        sizeof(PuzzleMessagePdata), (MkHdr**)&pdata);
    if (proc == 0) {
        return;
    }
    zero_pdata_payload(sizeof(PuzzleMessagePdata), &pdata->hdr);
    pdata->lifetime_ticks = 0x57;
    pdata->fade_start_ticks = 0x3E;
    pdata->fade_delay_ticks = 0;
    pdata->alpha = 0xFF;
    pdata->alpha_step = 4;
    pdata->velocity_y = 0;

    player->super_bar += 24.0f;
    if (player->super_bar > 185.0f) {
        player->super_bar = 185.0f;
    }

    if (fixed_message != 0) {
        image_id = 0x08020028;
    } else if (randu0(2) != 0) {
        image_id = 0x0802002B;
        snd_req(1);
    } else {
        image_id = 0x0802002F;
        snd_req(0x2D);
    }

    entry_index = 0;
    while (pzlang_image_table[entry_index].image_id != 0) {
        if (pzlang_image_table[entry_index].image_id == image_id) {
            placement = pzlang_image_table[entry_index].language;
            placement += get_language();
            break;
        }
        entry_index++;
    }

    image = load_2d_pfxobj(0x70033, 0x601D, (char*)image_id, 0, 0x3A);
    if (image == 0) {
        if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
        return;
    }

    pdata->primary_image = image;
    pdata->primary_instance = image->instance;
    if (player == puzzle_ctrl->players[0]) {
        x = placement->player1_x;
    } else {
        x = placement->player2_x;
    }
    image->x = x;
    image->y = placement->y;
    pdata->velocity_x = 0;
    pdata->velocity_y = 1;
    pdata->acceleration = -1;
}

/* Constrained structural mismatch (92.56%, retail 0x298/current 0x278). The
 * valid callback path, ownership checks, animation, and cleanup agree. Retail
 * preserves three redundant match-side validation branches and dereferences
 * null pdata on the invalid path; portable C merges those branches and returns
 * before the invalid access. Typed validators regress, so this is not labeled
 * an emission-only near miss. */
static float p_puzzle_fighter_chain_msg(void) {
    PuzzleMessagePdata* pdata = (PuzzleMessagePdata*)apdata;
    StringObj* text;
    ScreenObj* secondary;
    ScreenObj* primary;

    if (pdata == 0) {
        return -1.0f;
    }

    text = pdata->text;
    if (text != 0) {
        if (text->instance != pdata->text_instance) {
            text = 0;
        }
    } else {
        text = 0;
    }
    secondary = pdata->secondary_image;
    if (secondary != 0) {
        if (secondary->instance != pdata->secondary_instance) {
            secondary = 0;
        }
    } else {
        secondary = 0;
    }
    primary = pdata->primary_image;
    if (primary != 0) {
        if (primary->instance != pdata->primary_instance) {
            primary = 0;
        }
    } else {
        primary = 0;
    }

    if ((primary != 0 || text != 0) && pdata->lifetime_ticks != 0 &&
        puzzle_ctrl != 0) {
        int countdown = pdata->lifetime_ticks;

        pdata->lifetime_ticks = countdown - 1;
        if (countdown < pdata->fade_start_ticks) {
            int hold;

            if (pdata->velocity_x != 0) {
                pdata->velocity_x += pdata->acceleration;
            }
            if (pdata->velocity_y != 0) {
                pdata->velocity_y += pdata->acceleration;
            }
            hold = pdata->fade_delay_ticks;
            pdata->fade_delay_ticks = hold - 1;
            if (hold > 0) {
                pdata->lifetime_ticks++;
            } else {
                pdata->alpha = pdata->alpha > pdata->alpha_step
                                   ? pdata->alpha - pdata->alpha_step
                                   : 0;
            }
        }

        if (secondary != 0) {
            secondary->x += pdata->velocity_x;
            secondary->y += pdata->velocity_y;
            pfx_2d_obj_set_alpha(secondary, pdata->alpha);
        }
        if (primary != 0) {
            primary->x += pdata->velocity_x;
            primary->y += pdata->velocity_y;
            pfx_2d_obj_set_alpha(primary, pdata->alpha);
        }
        if (text != 0) {
            text->render_x += pdata->velocity_x;
            text->render_y += pdata->velocity_y;
            set_string_obj_alpha(text, (float)pdata->alpha);
        }
        return 1.0f;
    }

    if (secondary != 0) {
        pull_screen_obj(secondary);
        destroy_screen_obj(secondary);
    }
    if (primary != 0) {
        pull_screen_obj(primary);
        destroy_screen_obj(primary);
    }
    if (text != 0) {
        pull_string_obj(text);
        destroy_string_obj(text);
    }
    if (pdata->player != 0) {
        pdata->player->block_count_message_proc = 0;
    }
    return -1.0f;
}

/* Near miss: exact size and compaction operations. Remaining differences are
 * register coloring and equivalent loop pretest/increment scheduling. */
static int puzzle_fighter_fill_holes(PuzzlePlayerState* player) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* destination;
    PuzzleBoardCell* source;
    int column;
    int row;
    int destination_row;
    int board_moving;
    int row_offset;

    board_moving = 0;
    player->flag_bits.scanning_holes = 1;

    for (column = 0; column < 8; column++) {
        row = 0;
        row_offset = 0;
        while (row < 14) {
            cell = &player->board[row_offset + column];
            if (cell->type != 0 &&
                (cell->state == 0 ||
                 player->low_state_bits.low_state != 0)) {
                player->flag_bits.scanning_holes = 0;
                if (cell->fall_ticks != 0) {
                    board_moving = 1;
                }
                row++;
                row_offset += 8;
                continue;
            }

            destination_row = row;
            if (row < 14) {
                do {
                    source = &player->board[row * 8 + column];
                    if (source->type != 0 && source->state == 0) {
                        while (row < 14) {
                            source = &player->board[row * 8 + column];
                            if (source->type != 0) {
                                destination = &player->board[
                                    destination_row * 8 + column];
                                memcpy(destination, source,
                                       sizeof(PuzzleBoardCell));
                                memset(source, 0, sizeof(PuzzleBoardCell));
                                destination->fall_ticks =
                                    (row - destination_row) * 25;
                                board_moving = 1;
                                destination_row++;
                            }
                            row++;
                            row_offset += 8;
                        }
                    }
                    row++;
                    row_offset += 8;
                } while (row < 14);
            }
            break;
        }
    }

    player->flag_bits.board_moving = board_moving;
    return board_moving;
}

/* Near miss: the recovered CFG, signed fields, and visual load/store ordering
 * agree with retail. The 16-byte size gap is two string-pool address sequences;
 * remaining differences are register allocation and moved index setup. */
static int puzzle_fighter_find_match(PuzzlePlayerState* player) {
    PuzzleMatchContext context;
    PuzzleBoardCell* cell;
    unsigned int block_visual;
    unsigned int breaker_visual;
    int breaker_type;
    int base_type;
    int first_superbreakers;
    int superbreakers;
    int any_activity;
    int row;
    int column;
    int score_delta;

    any_activity = 0;
    first_superbreakers = 0;
    superbreakers = 0;
    for (;;) {
        superbreakers = puzzle_fighter_find_superbreaker(player);
        if (first_superbreakers != 0 || superbreakers == 0) {
            break;
        }
        first_superbreakers = superbreakers;
    }

    if (first_superbreakers != 0 && superbreakers != 0) {
        player->flag_bits.superbomb_cell_phase = 1;
    }

    superbreakers += first_superbreakers;
    if (superbreakers != 0) {
        player->match_delay = 40;
        player->flags2_bits.board_shift_active = 0;
    }

    context.matched_count = 0;
    context.flag_bits.matched_latch = 0;

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
                if (breaker_type == (int)cell->type) {
                    break;
                }
            }
            if (breaker_type >= 8) {
                continue;
            }

            base_type = PUZZLE_BLOCK_WILDCARD;
            if (breaker_type - 4 != 0) {
                base_type = breaker_type - 4;
            }
            context.base_type = base_type;
            block_visual = puzzle_ctrl->block_visuals[breaker_type - 4];
            breaker_visual = puzzle_ctrl->breaker_visuals[breaker_type - 4];
            context.breaker_visual = breaker_visual;
            context.block_visual = block_visual;
            context.player = player;
            context.cell = cell;

            if (cell->match_bits.match_state != 0) {
                continue;
            }

            if (puzzle_fighter_match_above_below(&context) != 0) {
                cell->flag_bits.effect_bit = 0;
                if (cell->state == 0) {
                    cell->state = PUZZLE_CELL_BREAK_STATE;
                    cell->visual = context.block_visual;
                    cell->type = context.base_type;
                    context.matched_count++;
                }
                any_activity = 1;
            }

            if (puzzle_fighter_match_left_right(&context) != 0) {
                cell->flag_bits.effect_bit = 0;
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

    if (superbreakers != 0) {
        if (player->match_mode_bits.match_mode != 0) {
            pan_snd_req(0x1AF6, player->sound_pan);
            if (player->match_mode_bits.match_mode == 3) {
                player->flag_bits.superbomb_cell_phase = 1;
            } else {
                player->flag_bits.superbomb_cell_phase = 0;
            }
        } else {
            pan_snd_req(0x1B14, player->sound_pan);
        }
    } else if (context.matched_count != 0) {
        if (context.matched_count < 5) {
            pan_snd_req(0x1B0F, player->sound_pan);
        } else if (context.matched_count < 10) {
            pan_snd_req(0x1B10, player->sound_pan);
        } else {
            pan_snd_req(0x1B11, player->sound_pan);
            puzzle_ctrl->match_delay = 40;
            puzzle_ctrl->flag_bits.large_color_clear = 1;
        }
    }

    score_delta =
        superbreakers + context.matched_count * (player->chain_count + 1);
    if (player->event_player == 0) {
        puzzle_ctrl->player1_score += score_delta;
        if (puzzle_ctrl->player1_score != 0) {
            sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                    puzzle_ctrl->player1_score);
        } else {
            sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
        }
        if (puzzle_ctrl->score_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[0]);
        }
        puzzle_ctrl->score_text[0] = string_right_xy(
            0x6023, 0x0D, temp_80_char,
            art_puzzle_fighter_static_tbl.player1_score_x + 0x36,
            art_puzzle_fighter_static_tbl.player1_score_y + 4, 0x3A);
        if (puzzle_ctrl->score_text[0] != 0) {
            pull_string_obj(puzzle_ctrl->score_text[0]);
        }
    } else {
        puzzle_ctrl->player2_score += score_delta;
        if (puzzle_ctrl->player2_score != 0) {
            sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_SCORE_FORMAT_STRING,
                    puzzle_ctrl->player2_score);
        } else {
            sprintf(temp_80_char, PUZZLE_STRINGS + PUZZLE_ZERO_STRING);
        }
        if (puzzle_ctrl->score_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[1]);
        }
        puzzle_ctrl->score_text[1] = string_right_xy(
            0x6023, 0x0D, temp_80_char,
            art_puzzle_fighter_static_tbl.player2_score_x + 0x36,
            art_puzzle_fighter_static_tbl.player2_score_y + 4, 0x3A);
        if (puzzle_ctrl->score_text[1] != 0) {
            pull_string_obj(puzzle_ctrl->score_text[1]);
        }
    }

    return any_activity;
}

#pragma dont_inline on
#pragma optimize_for_size on
/* Recovery in progress: the search and complete found-cell algorithm agree
 * with retail. Structured nested-loop breaks emit three extra post-search
 * instructions; moving the large found body into the loop changes hot-block
 * placement. This remains structural, not an emission-only near miss. */
static int puzzle_fighter_find_superbreaker(PuzzlePlayerState* player) {
    PuzzleBoardCell* cell;
    PuzzleBoardCell* superbreaker;
    unsigned int base_type;
    unsigned int paired_type;
    int visual_index;
    int row;
    int column;
    int marked;

    marked = 0;
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            superbreaker = &player->board[row * 8 + column];
            if (superbreaker->type != 0 &&
                superbreaker->type == PUZZLE_BLOCK_SUPERBREAKER &&
                superbreaker->state == 0) {
                break;
            }
        }
        if (column < 8) {
            break;
        }
    }

    if (row >= 14) {
        return 0;
    }

    superbreaker->state = PUZZLE_CELL_BREAK_STATE;
    if (superbreaker < &player->board[8]) {
        player->super_bar += 24.0f;
        if (player->super_bar > 185.0f) {
            player->super_bar = 185.0f;
        }
        superbreaker->visual = puzzle_ctrl->block_visuals[4];

        if (player->event_cooldown != 0) {
            player->event_cooldown--;
        } else {
            pz_event.type = 0x16;
            pz_event.player = player->event_player;
            pz_fighter_event(&pz_event);
        }

        puzzle_fighter_display_floor_msg(player, 1);
        pan_snd_req(0x1B13, player->sound_pan);
        pan_snd_req(0x1B14, player->sound_pan);
        return 0;
    }

    if (player->event_cooldown != 0) {
        player->event_cooldown--;
    } else {
        pz_event.type = 0x15;
        pz_event.player = player->event_player;
        pz_fighter_event(&pz_event);
    }

    for (visual_index = 0; visual_index < 8; visual_index++) {
        if ((visual_index == 0 &&
             superbreaker[-8].type == PUZZLE_BLOCK_WILDCARD) ||
            visual_index == (int)superbreaker[-8].type) {
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

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            cell = &player->board[row * 8 + column];
            if (cell->type == base_type || cell->type == paired_type) {
                cell->match_flag_bits.matched = 0;
                cell->match_flag_bits.breaking = 0;
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

/* Recovery in progress: the recursive match algorithm and boundary scans
 * agree with retail. Each structured scan emits one redundant post-break
 * comparison; retail threads the equality branch directly around the neighbor
 * block. This structural CFG residue is not an emission-only near miss. */
static int puzzle_fighter_match_left_right(PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* boundary;
    PuzzleBoardCell* neighbor;
    unsigned int boundary_offset;
    int row;
    int matched;

    matched = 0;
    next.breaker_visual = context->breaker_visual;
    next.block_visual = context->block_visual;
    next.player = context->player;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.match_bits.match_state = context->match_bits.match_state;

    boundary_offset = 0;
    for (row = 0; row < 14;
         row++, boundary_offset += 8 * sizeof(*boundary)) {
        boundary = (PuzzleBoardCell*)((unsigned char*)context->player->board +
                                      boundary_offset);
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell - 1;
        if (neighbor >= context->player->board &&
            (neighbor->type == context->cell->type ||
             neighbor->type == context->base_type) &&
            neighbor->match_bits.match_state == 0) {
            neighbor->match_flag_bits.breaking = 0;
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

    for (row = 0; row < 14; row++) {
        boundary = context->player->board + ((row + 1) * 8 - 1);
        if (context->cell == boundary) {
            break;
        }
    }
    if (context->cell != boundary) {
        neighbor = context->cell + 1;
        if (neighbor <= context->player->board_end &&
            (neighbor->type == context->cell->type ||
             neighbor->type == context->base_type) &&
            neighbor->match_bits.match_state == 0) {
            neighbor->match_flag_bits.breaking = 0;
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

/* Near miss: exact size and operations; visual stores are rescheduled. */
static int puzzle_fighter_match_above_below(PuzzleMatchContext* context) {
    PuzzleMatchContext next;
    PuzzleBoardCell* neighbor;
    int matched;

    matched = 0;
    next.breaker_visual = context->breaker_visual;
    next.block_visual = context->block_visual;
    next.player = context->player;
    next.base_type = context->base_type;
    next.matched_count = 0;
    next.match_bits.match_state = context->match_bits.match_state;

    neighbor = context->cell + 8;
    if (neighbor <= context->player->board_end &&
        (neighbor->type == context->cell->type ||
         neighbor->type == context->base_type) &&
        neighbor->match_bits.match_state == 0) {
        neighbor->match_flag_bits.breaking = 0;
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
        neighbor->match_bits.match_state == 0) {
        neighbor->match_flag_bits.breaking = 0;
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

/* Recovery in progress: all four rotation-state algorithms and shared up/down
 * fallback ownership agree with retail. Falling through from the two case-2
 * collision failures to one shared fallback restores the exact 0x434-byte
 * retail size at 72.36803%. Retail's kicked retry enters after the initial
 * column-bound test; a structured loop repeats that test at its header. A
 * semantic retry state grew the body to 0x458 at 70.45725%, while a clean
 * unroll grew it to 0x588 at 56.74350%. This one irreducible retry entry remains
 * a structural CFG ceiling. */
static PuzzleBoardCell*
puzzle_fighter_rotate_drop_pieces(int direction, PuzzlePlayerState* player) {
    PuzzleBoardCell* candidate;

    switch (player->rotation_state) {
    case 3:
        if (direction == 5) {
            candidate = player->active_cell - 8;
            if (candidate < player->board || candidate->type != 0) {
                if (candidate + 8 >= player->board_end ||
                    (candidate + 8)->type != 0) {
                    break;
                }
                player->active_cell += 8;
                player->active_row++;
                player->flags2_rotation_bits.rotation_kicked = 1;
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
                if (candidate + 8 >= player->board_end ||
                    (candidate + 8)->type != 0) {
                    break;
                }
                player->active_cell += 8;
                player->active_row++;
                player->flags2_rotation_bits.rotation_kicked = 1;
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
                    if (player->active_column >= 7 ||
                        (player->active_cell + 1)->type != 0) {
                        break;
                    }
                    player->active_cell++;
                    player->active_column++;
                }
                if (player->flags2_rotation_bits.rotation_kicked != 0) {
                    player->flags2_rotation_bits.rotation_kicked = 0;
                    if (player->active_row != 0 &&
                        player->active_cell >= player->board + 8) {
                        player->active_cell -= 8;
                        player->active_row--;
                        continue;
                    }
                }
                player->rotation_state = 3;
                player->rotation_angle = -1.5707964f;
                return player->active_cell;
            }
        } else {
            for (;;) {
                if (player->active_column >= 7 ||
                    (player->active_cell + 1)->type != 0) {
                    if (player->active_column == 0 ||
                        player->active_cell - 1 < player->board ||
                        (player->active_cell - 1)->type != 0) {
                        break;
                    }
                    player->active_cell--;
                    player->active_column--;
                }
                if (player->flags2_rotation_bits.rotation_kicked != 0) {
                    player->flags2_rotation_bits.rotation_kicked = 0;
                    if (player->active_row != 0 &&
                        player->active_cell >= player->board + 8) {
                        player->active_cell -= 8;
                        player->active_row--;
                        continue;
                    }
                }
                player->rotation_state = 1;
                player->rotation_angle = 1.5707964f;
                return player->active_cell;
            }
        }
        puzzle_rotation_fallback_down(player);
        break;

    case 0:
        do {
            if (direction == 4) {
                candidate = player->active_cell + 1;
                if (player->active_column >= 7 || candidate->type != 0) {
                    candidate = player->active_cell - 1;
                    if (player->active_column == 0 || candidate->type != 0) {
                        break;
                    }
                    player->active_cell = candidate;
                    player->active_column--;
                }
                player->rotation_state = 1;
                player->rotation_angle = -1.5707964f;
            } else {
                if (player->active_column == 0 ||
                    (player->active_cell - 1)->type != 0) {
                    if (player->active_column >= 7 ||
                        player->active_cell + 1 > player->board_end ||
                        (player->active_cell + 1)->type != 0) {
                        break;
                    }
                    player->active_cell++;
                    player->active_column++;
                }
                player->rotation_state = 3;
                player->rotation_angle = 1.5707964f;
            }
            return player->active_cell;
        } while (0);
        puzzle_rotation_fallback_up(player);
        break;
    }

    return player->active_cell;
}

#pragma optimize_for_size on
/* Emission-only near match (94.87%, retail 0x1C8/current 0x1D0). The pair
 * ownership swap, u16 sequence/result values, spawn scan, resets, and
 * superbreaker injection agree. The eight-byte excess is a post-scan test that
 * retail branch-threads from the loop break; exhaustion guards compile larger. */
static int puzzle_fighter_get_new_playpieces(PuzzlePlayerState* player) {
    PuzzleBoardCell* saved_pair;
    PuzzleBoardCell* cell;
    unsigned short next_type;
    unsigned short blocked;
    int cell_offset;
    int index;

    do {
        saved_pair = player->current_pair;
        player->current_pair = player->next_pair;
        player->next_pair = saved_pair;
        player->rotation_state = player->next_rotation_state;

        for (index = 0; index < 2; index++) {
            next_type =
                puzzle_ctrl->piece_sequence[player->piece_sequence_index++];
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
    for (index = 0, cell_offset = 0; index < 8;
         index++, cell_offset += sizeof(*cell)) {
        cell = (PuzzleBoardCell*)((unsigned char*)player->active_cell +
                                  cell_offset);
        if (cell->type != 0 && cell->state == 0) {
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

    player->flags2_bits.supermove_active = 0;
    player->flags2_bits.new_piece_latch = 0;
    player->flag_bits.score_applied = 0;
    player->active_row = 12;
    player->counter_drop_delay = 0;
    player->gravity_ticks = 0;
    player->drop_boost_ticks = 30;
    player->lock_ticks = 0;
    player->quick_drop_repeat = 0;
    player->active_column = 3;
    player->input_repeat_ticks = 0;
    player->rotation_angle = 0.0f;
    player->drop_bits.supermove_done = 1;
    player->input_command = 0;

    player->pieces_since_superbreaker++;
    if (puzzle_ctrl->flags2_bits.network_sequence_marker == 0 &&
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

/* Near match: p_puzzle_fighter_fight_msg 99.76%; float-pool labels only. */
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

/* Near miss: message selection, object/process ownership, and animation setup
 * agree with retail. The remaining four bytes are placement of the shared
 * 120-tick switch return ahead of the default animation block. */
static int puzzle_fighter_mode_start(int message) {
    MkHdr* proc_data;
    MkProc* proc;

    if (puzzle_ctrl->fight_message != 0) {
        pull_screen_obj(puzzle_ctrl->fight_message);
        destroy_screen_obj(puzzle_ctrl->fight_message);
        puzzle_ctrl->fight_message = 0;
    }

    puzzle_ctrl->fight_message = load_2d_pfxobj(
        0x70033, 0x601D,
        (char*)art_puzzle_fighter_messages_tbl[message], 0, 0x39);
    if (puzzle_ctrl->fight_message != 0) {
        puzzle_ctrl->fight_message->x = 200;
        puzzle_ctrl->fight_message->y = 285;
        puzzle_ctrl->fight_message->flag_bits.scaled = 0;
    }

    switch (message) {
    case 1:
        puzzle_ctrl->players[0]->mode_result = 0;
        puzzle_ctrl->players[1]->mode_result = 0;
        snd_req(0x46);
        break;
    case 2:
        puzzle_ctrl->players[0]->mode_result = 1;
        puzzle_ctrl->players[1]->mode_result = 1;
        snd_req(0x47);
        break;
    case 3:
        puzzle_ctrl->players[0]->mode_result = 2;
        puzzle_ctrl->players[1]->mode_result = 2;
        snd_req(0x48);
        break;
    case 0:
        puzzle_ctrl->fight_message->x = 95;
        puzzle_ctrl->fight_message->y = 275;
        break;
    default:
        puzzle_ctrl->fight_message->x = 290;
        puzzle_ctrl->fight_message->y = 250;
        puzzle_ctrl->fight_message->scale_x = 0.1f;
        puzzle_ctrl->fight_message->scale_y = 0.1f;
        puzzle_ctrl->fight_message->flag_bits.scaled = 1;
        puzzle_ctrl->fight_message_alpha = 255;
        if (message == 4) {
            puzzle_ctrl->fight_message_scale_step = 0.04f;
            puzzle_ctrl->fight_message_scale_limit = 1.0f;
            puzzle_ctrl->fight_message_x_step = 5;
            puzzle_ctrl->fight_message_y_step = 0;
            puzzle_ctrl->fight_message_alpha_step = 10;
        } else {
            puzzle_ctrl->fight_message_scale_step = 0.03f;
            puzzle_ctrl->fight_message_scale_limit = 0.95f;
            puzzle_ctrl->fight_message_x_step = 6;
            puzzle_ctrl->fight_message_y_step = 1;
            puzzle_ctrl->fight_message_alpha_step = 12;
        }

        proc = _create_mkproc_generic_nostack(
            0x6010, 0x1F, p_puzzle_fighter_fight_msg, 0, &proc_data);
        if (proc == 0) {
            pull_screen_obj(puzzle_ctrl->fight_message);
            destroy_screen_obj(puzzle_ctrl->fight_message);
            puzzle_ctrl->fight_message = 0;
        }
        mk_insert((MkHdr*)proc, &puzzle_ctrl->fight_message_processes);
        return 1;
    }
    return 120;
}

/* Near miss: exact retail size and operations. Remaining differences are
 * nonvolatile-register allocation, instruction scheduling, and pool labels. */
static int init_pz_pfx_2d(void) {
    PfxBuildInfo build;
    MkPfx* puzzle_effect = 0;
    MkPfx* ice_effect = 0;
    MkProc* proc;
    PfxEmitter* emitter;
    float emitter_lifetime;
    int ice_count;

    memset(&build, 0, sizeof(build));
    build.emitter_count = 1;
    build.name = (char*)(PUZZLE_STRINGS + PUZZLE_BLOCKS_STRING);
    proc = new_pfx_create_raw_userdata(
        &build, 0, pfx_2d_elements_tbl.particle_capacity,
        pfx_2d_elements_tbl.field_214, pfx_2d_elements_tbl.field_A0, 0,
        0x6012, p_pzpfx_copy_data, (void**)&puzzle_effect);
    if (proc == 0) {
        return 0;
    }
    mk_insert(&proc->hdr, &puzzle_ctrl->fight_message_processes);
    proc->priority = 45;

    ice_count = (g_game_info.plyr0.player_index == 3) +
                (g_game_info.plyr1.player_index == 3);
    if (ice_count != 0) {
        build.name = (char*)(PUZZLE_STRINGS + PUZZLE_ICE_BLOCKS_STRING);
        proc = new_pfx_create_raw_userdata(
            &build, ice_count * 0x68, pfx_2d_elements_tbl.particle_capacity,
            pfx_2d_elements_tbl.field_214, pfx_2d_elements_tbl.field_A0, 0,
            0x6012, p_pzpfx_copy_iceblock_data, (void**)&ice_effect);
        if (proc == 0) {
            return 0;
        }
        mk_insert(&proc->hdr, &puzzle_ctrl->fight_message_processes);
        proc->priority = 45;
    }

    set_pfx_texture((PfxVm*)puzzle_effect->matrix, (void*)0x70033,
                    (void*)pfx_2d_elements_tbl.texture_id);
    pfx_texture_animate(
        (PfxVm*)puzzle_effect->matrix, 1.0f,
        pfx_2d_elements_tbl.texture_width, pfx_2d_elements_tbl.frame_width,
        pfx_2d_elements_tbl.frame_height, pfx_2d_elements_tbl.frame_count);
    puzzle_effect->emitter_enabled =
        (unsigned short)pfx_2d_elements_tbl.emitter_enabled;
    emitter_lifetime = (float)pfx_2d_elements_tbl.emitter_lifetime;
    ((PfxVm*)puzzle_effect->matrix)->flag150_20 = 1;
    ((PfxVm*)puzzle_effect->matrix)->flag150_02 = 1;
    ((PfxVm*)puzzle_effect->matrix)->billboard_size =
        pfx_2d_elements_tbl.billboard_size;
    puzzle_effect->flag_bits.visible = 1;
    emitter = (PfxEmitter*)pfx_get_emitter(
        (PfxEmitterTableView*)puzzle_effect->matrix, 0);
    emitter->lifetime = emitter_lifetime;
    puzzle_effect->depth_bias = 50.0f;
    puzzle_effect->effect_state = 0;
    puzzle_ctrl->puzzle_pfx = (PuzzlePfxView*)puzzle_effect->matrix;
    puzzle_ctrl->puzzle_particle_capacity =
        &((PfxVm*)puzzle_ctrl->puzzle_pfx)->particle_capacity;
    puzzle_ctrl->particle_position_stride =
        pfx_get_struct_size(puzzle_ctrl->puzzle_pfx, 0x100);
    puzzle_ctrl->particle_timer_stride =
        pfx_get_struct_size(puzzle_ctrl->puzzle_pfx, 0x301);

    if (ice_count == 0) {
        return 1;
    }

    set_pfx_texture((PfxVm*)ice_effect->matrix, (void*)0x70033,
                    (void*)pfx_2d_elements_tbl.texture_id);
    pfx_texture_animate(
        (PfxVm*)ice_effect->matrix, 1.0f,
        pfx_2d_elements_tbl.texture_width, pfx_2d_elements_tbl.frame_width,
        pfx_2d_elements_tbl.frame_height, pfx_2d_elements_tbl.frame_count);
    ice_effect->emitter_enabled =
        (unsigned short)pfx_2d_elements_tbl.emitter_enabled;
    emitter_lifetime = (float)pfx_2d_elements_tbl.emitter_lifetime;
    ((PfxVm*)ice_effect->matrix)->flag150_20 = 1;
    ((PfxVm*)ice_effect->matrix)->flag150_02 = 1;
    ((PfxVm*)ice_effect->matrix)->billboard_size =
        pfx_2d_elements_tbl.billboard_size;
    ice_effect->flag_bits.visible = 1;
    emitter = (PfxEmitter*)pfx_get_emitter(
        (PfxEmitterTableView*)ice_effect->matrix, 0);
    emitter->lifetime = emitter_lifetime;
    ice_effect->depth_bias = 40.0f;
    ice_effect->effect_state = 0;
    puzzle_ctrl->ice_pfx = (PuzzlePfxView*)ice_effect->matrix;
    puzzle_ctrl->ice_particle_capacity =
        &((PfxVm*)puzzle_ctrl->ice_pfx)->particle_capacity;
    puzzle_ctrl->ice_position_stride =
        pfx_get_struct_size(puzzle_ctrl->ice_pfx, 0x100);
    puzzle_ctrl->ice_timer_stride =
        pfx_get_struct_size(puzzle_ctrl->ice_pfx, 0x301);
    return 1;
}

/* Structural mismatch (92.37%, retail 0x184/current 0x1A0): field ownership,
 * calls, and copy order agree, but the validity latch costs 28 bytes. Typed
 * acquisition/error helpers inline cleanly yet do not reproduce retail's
 * shared error tail, so this is not classified as an emission-only near miss. */
static float p_pzpfx_copy_iceblock_data(void) {
    int fields_valid = 1;

    if (apfx->effect_state != 0 || __mini_game_display_ctrl == 0) {
        return 1.0f;
    }

    puzzle_ctrl->particle_positions =
        pfx_get_field(puzzle_ctrl->puzzle_pfx, -2, 0x100);
    puzzle_ctrl->particle_timers =
        pfx_get_field(puzzle_ctrl->puzzle_pfx, -2, 0x301);
    if (puzzle_ctrl->ice_pfx != 0) {
        puzzle_ctrl->ice_positions =
            pfx_get_field(puzzle_ctrl->ice_pfx, -2, 0x100);
        puzzle_ctrl->ice_timers =
            pfx_get_field(puzzle_ctrl->ice_pfx, -2, 0x301);
        if (puzzle_ctrl->ice_positions == 0 ||
            puzzle_ctrl->ice_timers == 0) {
            fields_valid = 0;
        } else {
            puzzle_ctrl->ice_pfx->particle_count = 0;
        }
    }

    if (fields_valid == 0 || puzzle_ctrl->particle_positions == 0 ||
        puzzle_ctrl->particle_timers == 0) {
        puzzle_ctrl->pfx_bits.pfx_error = 1;
        return -1.0f;
    }

    puzzle_ctrl->puzzle_pfx->particle_count = 0;
    if (g_fatality_sound == 0) {
        pzpfx_copy_puzzleblocks(puzzle_ctrl->players[0]);
        pzpfx_copy_puzzleblocks(puzzle_ctrl->players[1]);
        pzpfx_copy_playpieces(puzzle_ctrl->players[0]);
        pzpfx_copy_playpieces(puzzle_ctrl->players[1]);
    }
    if (puzzle_ctrl->pfx_bits.pfx_error != 0) {
        return -1.0f;
    }
    return 1.0f;
}

static float p_pzpfx_copy_data(void) {
    if (apfx->effect_state != 0 || __mini_game_display_ctrl == 0) {
        return 1.0f;
    }

    puzzle_ctrl->particle_positions =
        pfx_get_field(puzzle_ctrl->puzzle_pfx, -2, 0x100);
    puzzle_ctrl->particle_timers =
        pfx_get_field(puzzle_ctrl->puzzle_pfx, -2, 0x301);
    if (puzzle_ctrl->particle_positions == 0 ||
        puzzle_ctrl->particle_timers == 0) {
        puzzle_ctrl->pfx_bits.pfx_error = 1;
        return -1.0f;
    }

    puzzle_ctrl->puzzle_pfx->particle_count = 0;
    if (g_fatality_sound == 0) {
        if (puzzle_ctrl->ice_pfx != 0) {
            puzzle_ctrl->pfx_bits2.copy_ice_particles = 1;
        }
        pzpfx_copy_puzzleblocks(puzzle_ctrl->players[0]);
        pzpfx_copy_puzzleblocks(puzzle_ctrl->players[1]);
        pzpfx_copy_playpieces(puzzle_ctrl->players[0]);
        pzpfx_copy_playpieces(puzzle_ctrl->players[1]);
        puzzle_ctrl->pfx_bits2.copy_ice_particles = 0;
    }
    if (puzzle_ctrl->pfx_bits.pfx_error != 0) {
        return -1.0f;
    }
    return 1.0f;
}

/* Near miss: preview/current-pair copy behavior and arithmetic agree with
 * retail. Remaining differences are placement-base formation, integer-to-float
 * scheduling, and the resulting nonvolatile-register allocation. */
static void pzpfx_copy_playpieces(PuzzlePlayerState* player) {
    const PuzzleArtPlacement* preview_placement;
    const PuzzleArtPlacement* board_placement;
    const int* preview_x;
    const int* preview_y;
    const int* board_x;
    const int* board_y;
    int shake_enabled =
        puzzle_ctrl->motion_bits.particle_motion_active != 0 ||
        player->flags2_motion_bits.board_motion_state != 0;
    int movement_active =
        puzzle_ctrl->match_delay != 0 || player->match_delay != 0;
    PuzzleBoardCell* next_piece;
    PuzzleBoardCell* current_piece;
    int piece;
    int preview_y_offset;
    int piece_offset;

    if (shake_enabled != 0 && movement_active != 0) {
        signrand(3);
        signrand(3);
    }

    preview_placement =
        &art_puzzle_fighter_static_tbl
             .placements[player->event_player + 3];
    board_placement =
        &art_puzzle_fighter_static_tbl
             .placements[player->event_player + 1];
    piece = 0;
    preview_y_offset = 0;
    preview_x = &preview_placement->x;
    preview_y = &preview_placement->y;
    piece_offset = 0;
    board_x = &board_placement->x;
    board_y = &board_placement->y;

    for (; piece < 2;
         piece++, piece_offset += sizeof(*next_piece),
         preview_y_offset += 25) {
        unsigned int type;

        if (shake_enabled == 0 && movement_active != 0) {
            signrand(3);
            signrand(3);
        }

        next_piece = (PuzzleBoardCell*)((char*)player->next_pair +
                                        piece_offset);
        puzzle_ctrl->particle_positions->x = (float)(*preview_x + 4);
        {
            int preview_y_position = *preview_y + preview_y_offset;
            preview_y_position += 3;
            puzzle_ctrl->particle_positions->y = (float)preview_y_position;
        }
        puzzle_ctrl->particle_positions->z = 100.0f;
        type = next_piece->type;
        if (type < 15) {
            *puzzle_ctrl->particle_timers = (float)type;
        } else if (type == PUZZLE_BLOCK_WILDCARD) {
            *puzzle_ctrl->particle_timers = 0.0f;
        } else {
            continue;
        }
        puzzle_ctrl->puzzle_pfx->particle_count++;
        puzzle_ctrl->particle_positions =
            (Vec*)((char*)puzzle_ctrl->particle_positions +
                   puzzle_ctrl->particle_position_stride);
        puzzle_ctrl->particle_timers =
            (float*)((char*)puzzle_ctrl->particle_timers +
                     puzzle_ctrl->particle_timer_stride);

        if (player->flags3_bits.hide_active_piece == 0 &&
            player->active_cell != 0) {
            const PuzzlePieceLayout* rotation_steps = puzzle_piece_layout;
            const Vec* rotation_origins;
            int base_x;
            int base_y;
            int gravity_ticks;
            int drop_interval;
            float fall_offset;

            base_x = *board_x + player->active_column * 25 + 4;
            gravity_ticks = player->gravity_ticks;
            drop_interval = player->drop_interval;
            current_piece =
                (PuzzleBoardCell*)((char*)player->current_pair +
                                   piece_offset);
            puzzle_ctrl->particle_positions->x = (float)base_x;
            base_y = *board_y + player->active_row * 25 + 5;
            puzzle_ctrl->particle_positions->y = (float)base_y;
            puzzle_ctrl->particle_positions->x +=
                (float)(piece *
                        (rotation_steps[player->rotation_state]
                             .column *
                         25));
            fall_offset =
                25.0f * ((float)gravity_ticks / (float)drop_interval);
            puzzle_ctrl->particle_positions->y += fall_offset +
                (float)(piece *
                        (rotation_steps[player->rotation_state]
                             .row *
                         25));
            puzzle_ctrl->particle_positions->z = 100.0f;

            if (player->rotation_angle != 0.0f && piece != 0) {
                Vec rotated;

                rotation_origins = puzzle_2d_smooth;
                rotated.x = rotation_origins[player->rotation_state].x;
                rotated.z = rotation_origins[player->rotation_state].z;

                if (puzzle_ctrl->pfx_bits2.copy_ice_particles == 0 &&
                    puzzle_ctrl->supermove_phase_ticks == 0) {
                    if (player->rotation_angle > 0.0f) {
                        player->rotation_angle -= 0.2f;
                        if (player->rotation_angle < 0.0f) {
                            player->rotation_angle = 0.0f;
                        }
                    } else {
                        player->rotation_angle += 0.2f;
                        if (player->rotation_angle > 0.0f) {
                            player->rotation_angle = 0.0f;
                        }
                    }
                }
                if (player->rotation_angle != 0.0f) {
                    rotate_xz(&rotated, &rotated, player->rotation_angle);
                    rotation_origins = puzzle_2d_smooth;
                    puzzle_ctrl->particle_positions->x +=
                        25.0f *
                        (rotation_origins[player->rotation_state].x -
                         rotated.x);
                    puzzle_ctrl->particle_positions->y +=
                        25.0f *
                        (rotation_origins[player->rotation_state].z -
                         rotated.z);
                }
            }

            type = current_piece->type;
            if (type < 15) {
                *puzzle_ctrl->particle_timers = (float)type;
            } else if (type == PUZZLE_BLOCK_WILDCARD) {
                *puzzle_ctrl->particle_timers = 0.0f;
            } else {
                continue;
            }
            {
                puzzle_ctrl->puzzle_pfx->particle_count++;
                puzzle_ctrl->particle_positions =
                    (Vec*)((char*)puzzle_ctrl->particle_positions +
                           puzzle_ctrl->particle_position_stride);
                puzzle_ctrl->particle_timers =
                    (float*)((char*)puzzle_ctrl->particle_timers +
                             puzzle_ctrl->particle_timer_stride);
            }
        }
    }
}

/* Recovery in progress: retail types, placement lifetime, particle algorithm,
 * and common advancement are recovered. The 24-byte excess is one
 * six-instruction condition recheck imposed by the clean structured rejection
 * paths, so this is not an emission-only near miss. */
static void pzpfx_copy_puzzleblocks(PuzzlePlayerState* player) {
    const PuzzleArtPlacement* placement;
    const int* origin_x;
    const int* origin_y;
    int shake_enabled =
        puzzle_ctrl->motion_bits.particle_motion_active != 0 ||
        player->flags2_motion_bits.board_motion_state != 0;
    int movement_active =
        puzzle_ctrl->match_delay != 0 || player->match_delay != 0;
    int jitter_x;
    int jitter_y;
    int fall_step;
    int placement_index;
    int row;
    int column;

    if (shake_enabled != 0 && movement_active != 0) {
        jitter_x = signrand(3);
        jitter_y = signrand(3);
    } else {
        jitter_x = 0;
        jitter_y = 0;
    }

    placement_index = player->event_player + 1;
    if (player->drop_interval <= 10) {
        fall_step = 10;
    } else if (player->drop_interval <= 20) {
        fall_step = 9;
    } else {
        fall_step = 8;
    }

    placement = &art_puzzle_fighter_static_tbl.placements[placement_index];
    origin_x = &placement->x;
    origin_y = &placement->y;

    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            int index = row * 8 + column;
            PuzzleBoardCell* cell = &player->board[index];

            if (cell->type == 0) {
                continue;
            }
            if (shake_enabled == 0 && movement_active != 0) {
                jitter_x = signrand(3);
                jitter_y = signrand(3);
            }
            if (cell->state != 0) {
                continue;
            }

            puzzle_ctrl->particle_positions->x =
                (float)(*origin_x + column * 25 + 4 + jitter_x);
            puzzle_ctrl->particle_positions->y =
                (float)(*origin_y + row * 25 + 5 + jitter_y);
            puzzle_ctrl->particle_positions->z = 100.0f;

            if (cell->fall_ticks != 0 &&
                puzzle_ctrl->pfx_bits2.copy_ice_particles == 0) {
                puzzle_ctrl->particle_positions->y +=
                    (float)cell->fall_ticks;
                cell->fall_ticks -= fall_step;
                if (cell->fall_ticks < 0) {
                    cell->fall_ticks = 0;
                }
            }

            if (player->flags3_bits.hide_active_piece != 0 &&
                puzzle_ctrl->particle_effect_ticks > 0 &&
                (puzzle_ctrl->particle_effect_ticks & 3) == 0) {
                *puzzle_ctrl->particle_timers = 8.0f;
            } else {
                if (player->flags3_bits.hide_active_piece != 0 &&
                    puzzle_ctrl->particle_effect_ticks <= 0) {
                    if (puzzle_ctrl->particle_effect_distance < 90.0f) {
                        PuzzleBlockOffset* offset =
                            &puzzle_ctrl->block_offsets[index];

                        puzzle_ctrl->particle_positions->x +=
                            puzzle_ctrl->particle_effect_distance * offset->x;
                        puzzle_ctrl->particle_positions->y +=
                            puzzle_ctrl->particle_effect_distance * offset->y;
                    } else {
                        puzzle_ctrl->particle_positions->y = -1000.0f;
                    }
                }

                if (cell->flag_bits.effect_bit != 0 &&
                    (player->invisibility_ticks < 0x1000 ||
                     (player->invisibility_ticks & 4) != 0)) {
                    continue;
                }

                if (cell->flag_bits.matched != 0 &&
                    player->invisibility_fade > 0) {
                    puzzle_ctrl->ice_positions->x =
                        puzzle_ctrl->particle_positions->x;
                    puzzle_ctrl->ice_positions->y =
                        puzzle_ctrl->particle_positions->y;
                    puzzle_ctrl->ice_positions->z =
                        puzzle_ctrl->particle_positions->z;
                    *puzzle_ctrl->ice_timers =
                        (float)player->invisibility_fade;
                    puzzle_ctrl->ice_pfx->particle_count++;
                    puzzle_ctrl->ice_positions =
                        (Vec*)((char*)puzzle_ctrl->ice_positions +
                               puzzle_ctrl->ice_position_stride);
                    puzzle_ctrl->ice_timers =
                        (float*)((char*)puzzle_ctrl->ice_timers +
                                 puzzle_ctrl->ice_timer_stride);
                }

                if (cell->type < 15) {
                    *puzzle_ctrl->particle_timers = (float)cell->type;
                } else if (cell->type == PUZZLE_BLOCK_WILDCARD) {
                    *puzzle_ctrl->particle_timers = 0.0f;
                } else {
                    continue;
                }
            }

            puzzle_ctrl->puzzle_pfx->particle_count++;
            puzzle_ctrl->particle_positions =
                (Vec*)((char*)puzzle_ctrl->particle_positions +
                       puzzle_ctrl->particle_position_stride);
            puzzle_ctrl->particle_timers =
                (float*)((char*)puzzle_ctrl->particle_timers +
                         puzzle_ctrl->particle_timer_stride);
        }
    }
}

/* Emission-only near match (95.00%, retail 0x780/current 0x79C). The complete
 * round reset, preserved state, object cleanup, and AI setup agree. Computing
 * the unsigned loss quotient before its threshold matches retail's lifetime;
 * remaining differences are shared AI-move tail layout and register coloring. */
static void puzzle_fighter_mode_clear(void) {
    PuzzlePlayerState* ai_player = 0;
    PuzzlePlayerState* player0 = puzzle_ctrl->players[0];
    PuzzlePlayerState* player1 = puzzle_ctrl->players[1];
    PuzzleSupermoveBalance* supermove_balance;
    float player0_super_bar = player0->super_bar;
    float player1_super_bar = player1->super_bar;
    int player0_result = player0->round_result_value;
    int player1_result = player1->round_result_value;
    void (*player0_cleanup)(void) = player0->supermove_cleanup;
    void (*player1_cleanup)(void) = player1->supermove_cleanup;
    PuzzleProfileStats* player0_stats = player0->profile_stats;
    PuzzleProfileStats* player1_stats = player1->profile_stats;
    int speed;
    unsigned int loss_adjustments;

    if (player0->block_count_font_11 != 0) {
        destroy_string_obj(player0->block_count_font_11);
    }
    if (puzzle_ctrl->players[0]->block_count_font_10 != 0) {
        destroy_string_obj(puzzle_ctrl->players[0]->block_count_font_10);
    }
    if (puzzle_ctrl->players[1]->block_count_font_11 != 0) {
        destroy_string_obj(puzzle_ctrl->players[1]->block_count_font_11);
    }
    if (puzzle_ctrl->players[1]->block_count_font_10 != 0) {
        destroy_string_obj(puzzle_ctrl->players[1]->block_count_font_10);
    }

    puzzle_ctrl->input_bits.input_latched = 0;
    puzzle_ctrl->gameplay_ticks = 0;
    puzzle_ctrl->supermove_phase_ticks = 0;
    memset(puzzle_ctrl->boards[0], 0, 0x8C0);
    memset(puzzle_ctrl->boards[1], 0, 0x8C0);
    memset(puzzle_ctrl->players[0], 0, sizeof(PuzzlePlayerState));
    memset(puzzle_ctrl->players[1], 0, sizeof(PuzzlePlayerState));
    memset(puzzle_ctrl->next_pairs[0], 0, sizeof(PuzzleBoardCell) * 2);
    memset(puzzle_ctrl->next_pairs[1], 0, sizeof(PuzzleBoardCell) * 2);

    puzzle_ctrl->players[0]->board = puzzle_ctrl->boards[0];
    puzzle_ctrl->players[1]->board = puzzle_ctrl->boards[1];
    puzzle_ctrl->players[0]->board_end =
        &puzzle_ctrl->boards[0][PUZZLE_BOARD_CELLS - 1];
    puzzle_ctrl->players[1]->board_end =
        &puzzle_ctrl->boards[1][PUZZLE_BOARD_CELLS - 1];
    puzzle_ctrl->players[0]->current_pair = puzzle_ctrl->current_pairs[0];
    puzzle_ctrl->players[1]->current_pair = puzzle_ctrl->current_pairs[1];
    puzzle_ctrl->players[0]->next_pair = puzzle_ctrl->next_pairs[0];
    puzzle_ctrl->players[1]->next_pair = puzzle_ctrl->next_pairs[1];
    puzzle_ctrl->players[0]->profile_stats = player0_stats;
    puzzle_ctrl->players[1]->profile_stats = player1_stats;
    puzzle_ctrl->players[0]->pfx_slot = 0x70034;
    puzzle_ctrl->players[1]->pfx_slot = 0x70035;
    puzzle_ctrl->players[0]->super_bar = player0_super_bar;
    puzzle_ctrl->players[1]->super_bar = player1_super_bar;
    puzzle_ctrl->players[0]->round_result_value = player0_result;
    puzzle_ctrl->players[1]->round_result_value = player1_result;
    puzzle_ctrl->players[0]->supermove_cleanup = player0_cleanup;
    puzzle_ctrl->players[1]->supermove_cleanup = player1_cleanup;

    puzzle_fighter_get_new_playpieces(puzzle_ctrl->players[0]);
    puzzle_fighter_get_new_playpieces(puzzle_ctrl->players[1]);
    puzzle_ctrl->speedup_ticks = 0;
    supermove_balance =
        (PuzzleSupermoveBalance*)get_data_table(g_game_info.cmdscript, 9);
    puzzle_ctrl->players[0]->equipped_supermove =
        g_game_info.plyr0.slot.pdata->runtime_data->puzzle_supermove_index;
    puzzle_ctrl->players[1]->equipped_supermove =
        g_game_info.plyr1.slot.pdata->runtime_data->puzzle_supermove_index;
    puzzle_ctrl->players[0]->drop_interval = 30;
    puzzle_ctrl->players[1]->drop_interval = 30;
    puzzle_ctrl->players[0]->super_gain_per_block =
        supermove_balance[puzzle_ctrl->players[0]->equipped_supermove]
            .gain_per_block;
    puzzle_ctrl->players[1]->super_gain_per_block =
        supermove_balance[puzzle_ctrl->players[1]->equipped_supermove]
            .gain_per_block;
    puzzle_ctrl->players[0]->supermove_amount =
        supermove_balance[puzzle_ctrl->players[0]->equipped_supermove].amount;
    puzzle_ctrl->players[1]->supermove_amount =
        supermove_balance[puzzle_ctrl->players[1]->equipped_supermove].amount;
    puzzle_ctrl->players[0]->input_command = 0;
    puzzle_ctrl->players[1]->input_command = 0;
    puzzle_ctrl->players[0]->previous_input_command = 0;
    puzzle_ctrl->players[1]->previous_input_command = 0;
    puzzle_ctrl->players[0]->selected_supermove = -1;
    puzzle_ctrl->players[1]->selected_supermove = -1;
    puzzle_ctrl->players[0]->sound_pan = -0.7f;
    puzzle_ctrl->players[1]->sound_pan = 0.7f;
    puzzle_ctrl->players[0]->event_player = 0;
    puzzle_ctrl->players[1]->event_player = 1;
    puzzle_ctrl->players[0]->input_player = &g_game_info.plyr0;
    puzzle_ctrl->players[1]->input_player = &g_game_info.plyr1;
    puzzle_ctrl->players[0]->winner_character_id =
        g_game_info.plyr0.player_index;
    puzzle_ctrl->players[1]->winner_character_id =
        g_game_info.plyr1.player_index;
    puzzle_ctrl->players[0]->character_id = g_game_info.plyr0.player_index;
    puzzle_ctrl->players[1]->character_id = g_game_info.plyr1.player_index;
    puzzle_ctrl->players[0]->mode_step =
        puzzle_fighter_mode_play__drop_sequence;
    puzzle_ctrl->players[1]->mode_step =
        puzzle_fighter_mode_play__drop_sequence;
    puzzle_ctrl->players[0]->center_event_word = 0;
    puzzle_ctrl->players[1]->center_event_word = 0;

    if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
        puzzle_ctrl->players[0]->ai_player_bits.ai_player = 1;
        puzzle_ctrl->players[1]->ai_player_bits.ai_player = 1;
        puzzle_ctrl->players[0]->ai_fallback_move = 3;
        puzzle_ctrl->players[1]->ai_fallback_move = 3;
        puzzle_ctrl->players[0]->ai_no_pause_band = 2;
        puzzle_ctrl->players[1]->ai_no_pause_band = 2;
        if (randu0(3) != 0) {
            puzzle_ctrl->players[0]->ai_pause_base = randu0(3) + 3;
            puzzle_ctrl->players[1]->ai_pause_base = randu0(3) + 3;
            puzzle_ctrl->players[0]->drop_interval = 30;
            puzzle_ctrl->players[1]->drop_interval = 30;
        } else {
            puzzle_ctrl->players[0]->ai_pause_base = 0;
            puzzle_ctrl->players[1]->ai_pause_base = 0;
            puzzle_ctrl->players[0]->drop_interval = 60;
            puzzle_ctrl->players[1]->drop_interval = 60;
        }
        puzzle_ctrl->players[0]->ai_move =
            puzzle_ctrl->players[0]->ai_fallback_move;
        puzzle_ctrl->players[1]->ai_move =
            puzzle_ctrl->players[1]->ai_fallback_move;
    } else {
        if (g_game_info.plyr0.player_state == 2) {
            puzzle_ctrl->players[0]->ai_player_bits.ai_player = 0;
        } else {
            puzzle_ctrl->players[0]->ai_player_bits.ai_player = 1;
            ai_player = puzzle_ctrl->players[0];
        }
        if (g_game_info.plyr1.player_state == 2) {
            puzzle_ctrl->players[1]->ai_player_bits.ai_player = 0;
        } else {
            puzzle_ctrl->players[1]->ai_player_bits.ai_player = 1;
            ai_player = puzzle_ctrl->players[1];
        }
        if (ai_player != 0) {
            ai_player->ai_no_pause_band = get_ladder_position();
            if (game_settings.puzzle_difficulty <= 2) {
                loss_adjustments = pz_loss_in_a_row / 5;
                if (pz_loss_in_a_row >= 5) {
                    do {
                        if (ai_player->ai_no_pause_band != 0) {
                            ai_player->ai_no_pause_band--;
                        }
                        if (ai_player->ai_no_pause_band == 0) {
                            break;
                        }
                    } while (--loss_adjustments != 0);
                }
            }
            if (game_settings.puzzle_difficulty <= 2) {
                ai_player->ai_fallback_move =
                    ai_state_type[(puzzle_ctrl->players[0]
                                       ->round_result_value ^
                                   puzzle_ctrl->players[1]
                                       ->round_result_value) +
                                  ai_player->ai_no_pause_band * 2];
            } else {
                ai_player->ai_fallback_move = 3;
            }
            ai_player->ai_move = ai_player->ai_fallback_move;
            speed = (8 - ai_player->ai_no_pause_band) -
                    game_settings.puzzle_difficulty;
            if (speed <= 0) {
                speed = 1;
            }
            if (speed > 6) {
                speed = 6;
            }
            speed *= 10;
            if (game_settings.puzzle_difficulty > 2) {
                speed /= 2;
            }
            puzzle_ctrl->players[0]->drop_interval = speed;
            puzzle_ctrl->players[1]->drop_interval = speed;
            ai_player->ai_pause_base =
                (8 - ai_player->ai_no_pause_band) -
                game_settings.puzzle_difficulty;
            if (ai_player->ai_pause_base < 0) {
                ai_player->ai_pause_base = 0;
            }
            puzzle_ctrl->players[0]->ai_move =
                puzzle_ctrl->players[0]->ai_fallback_move;
            puzzle_ctrl->players[1]->ai_move =
                puzzle_ctrl->players[1]->ai_fallback_move;
        }
    }
}

void cleanup_minigame_system(void) {
    g_pz_fighters_engine.start_bits.startup_enabled = 0;
    memset(&g_pz_fighters_engine, 0, sizeof(g_pz_fighters_engine));
    cleanup_pz_fatality_stuff();
    puzzle_ctrl = 0;
    __mini_game_display_ctrl = 0;
}

static void render_wiffs(PuzzlePlayerState* player);
static void render_UI(PuzzlePlayerState* player);

/* Near match: load_puzzle_champion_screen 99.49%; pool identities only. */
void load_puzzle_champion_screen(void) {
    ScreenObj* champion_a;
    ScreenObj* champion_b;
    int alpha;

    snd_req(0x1B03);
    load_art_section(0x2001E, &sec_pz_ending_champion);
    champion_a = load_named_2d_pfxobj_xy(
        0x2001E, 0x4002, PUZZLE_STRINGS + PUZZLE_CHAMPION_A_STRING, 0, -0x40, -0x10,
        0x1E);
    champion_b = load_named_2d_pfxobj_xy(
        0x2001E, 0x4003, PUZZLE_STRINGS + PUZZLE_CHAMPION_B_STRING, 0, 0x1C0, -0x10,
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

/* Near miss: exact retail size, operations, ownership, and control flow.
 * Objdiff reports only eleven register-operand differences. */
static void minigame_puzzlefighter_destroy(void) {
    MkPtr* process_item;
    MkPtr* next_item;
    MkPtr** process_list;
    int index;

    if (puzzle_ctrl->fight_message != 0) {
        pull_screen_obj(puzzle_ctrl->fight_message);
        destroy_screen_obj(puzzle_ctrl->fight_message);
        puzzle_ctrl->fight_message = 0;
    }
    if (puzzle_ctrl != 0) {
        process_list = &puzzle_ctrl->fight_message_processes;
        if (process_list != 0) {
            process_item = *process_list;
            while (process_item != 0) {
                if (process_item->hdr->instance != process_item->instance) {
                    next_item = process_item->next;
                    process_item->hdr = 0;
                    destroy_mkptr(process_item);
                    process_item = next_item;
                } else {
                    if (process_item->hdr != 0 &&
                        process_item->hdr->instance != 0) {
                        process_item->hdr->typed_vtbl->destroy(
                            process_item->hdr);
                    }
                    process_item = process_item->next;
                }
            }
        }

        if (puzzle_ctrl->ui_objects != 0) {
            for (index = 0; index < puzzle_ctrl->ui_object_count; index++) {
                destroy_screen_obj(puzzle_ctrl->ui_objects[index]);
                puzzle_ctrl->ui_objects[index] = 0;
            }
            free_mem(puzzle_ctrl->ui_objects);
        }
        if (puzzle_ctrl->block_visuals != 0) {
            for (index = 0; index < puzzle_ctrl->block_visual_count; index++) {
                if (puzzle_ctrl->breaker_objects[index] != 0) {
                    destroy_screen_obj(puzzle_ctrl->breaker_objects[index]);
                }
                puzzle_ctrl->breaker_objects[index] = 0;
                if (puzzle_ctrl->block_animations[index] != 0) {
                    destroy_ani_texture_control(
                        puzzle_ctrl->block_animations[index]);
                }
                puzzle_ctrl->block_animations[index] = 0;
            }
            free_mem(puzzle_ctrl->block_visuals);
        }
        if (puzzle_ctrl->sequence_bits.piece_sequence_owned != 0 &&
            puzzle_ctrl->piece_sequence != 0) {
            free_mem(puzzle_ctrl->piece_sequence);
        }
        puzzle_ctrl->piece_sequence = 0;

        if (puzzle_ctrl->boards[0] != 0) {
            free_mem(puzzle_ctrl->boards[0]);
        }
        puzzle_ctrl->boards[0] = 0;
        if (puzzle_ctrl->boards[1] != 0) {
            free_mem(puzzle_ctrl->boards[1]);
        }
        puzzle_ctrl->boards[1] = 0;

        if (puzzle_ctrl->players[0] != 0) {
            if (puzzle_ctrl->players[0]->result_portrait != 0) {
                pull_screen_obj(puzzle_ctrl->players[0]->result_portrait);
                destroy_screen_obj(
                    puzzle_ctrl->players[0]->result_portrait);
            }
            if (puzzle_ctrl->players[0]->block_count_font_11 != 0) {
                destroy_string_obj(
                    puzzle_ctrl->players[0]->block_count_font_11);
            }
            if (puzzle_ctrl->players[0]->block_count_font_10 != 0) {
                destroy_string_obj(
                    puzzle_ctrl->players[0]->block_count_font_10);
            }
            if (puzzle_ctrl->players[0]->supermove_cleanup != 0) {
                puzzle_ctrl->players[0]->supermove_cleanup();
            }
            free_mem(puzzle_ctrl->players[0]);
            puzzle_ctrl->players[0] = 0;
        }
        if (puzzle_ctrl->players[1] != 0) {
            if (puzzle_ctrl->players[1]->result_portrait != 0) {
                pull_screen_obj(puzzle_ctrl->players[1]->result_portrait);
                destroy_screen_obj(
                    puzzle_ctrl->players[1]->result_portrait);
            }
            if (puzzle_ctrl->players[1]->block_count_font_11 != 0) {
                destroy_string_obj(
                    puzzle_ctrl->players[1]->block_count_font_11);
            }
            if (puzzle_ctrl->players[1]->block_count_font_10 != 0) {
                destroy_string_obj(
                    puzzle_ctrl->players[1]->block_count_font_10);
            }
            if (puzzle_ctrl->players[1]->supermove_cleanup != 0) {
                puzzle_ctrl->players[1]->supermove_cleanup();
            }
            free_mem(puzzle_ctrl->players[1]);
            puzzle_ctrl->players[1] = 0;
        }

        if (puzzle_ctrl->current_pairs[0] != 0) {
            free_mem(puzzle_ctrl->current_pairs[0]);
        }
        puzzle_ctrl->current_pairs[0] = 0;
        if (puzzle_ctrl->current_pairs[1] != 0) {
            free_mem(puzzle_ctrl->current_pairs[1]);
        }
        puzzle_ctrl->current_pairs[1] = 0;
        if (puzzle_ctrl->next_pairs[0] != 0) {
            free_mem(puzzle_ctrl->next_pairs[0]);
        }
        puzzle_ctrl->next_pairs[0] = 0;
        if (puzzle_ctrl->next_pairs[1] != 0) {
            free_mem(puzzle_ctrl->next_pairs[1]);
        }
        puzzle_ctrl->next_pairs[1] = 0;

        for (index = 0; index < 13; index++) {
            if (puzzle_ctrl->round_objects[index] != 0) {
                destroy_screen_obj(puzzle_ctrl->round_objects[index]);
            }
            puzzle_ctrl->round_objects[index] = 0;
        }
        for (index = 0; index < 4; index++) {
            if (puzzle_ctrl->victory_objects[index] != 0) {
                destroy_screen_obj(puzzle_ctrl->victory_objects[index]);
            }
            puzzle_ctrl->victory_objects[index] = 0;
        }
        for (index = 0; index < 3; index++) {
            if (puzzle_ctrl->hud_objects[index] != 0) {
                destroy_screen_obj(puzzle_ctrl->hud_objects[index]);
            }
            puzzle_ctrl->hud_objects[index] = 0;
        }
        for (index = 0; index < 2; index++) {
            if (puzzle_ctrl->supermove_fade_objects[index] != 0) {
                destroy_screen_obj(
                    puzzle_ctrl->supermove_fade_objects[index]);
            }
            puzzle_ctrl->supermove_fade_objects[index] = 0;
        }
        if (puzzle_ctrl->block_offsets != 0) {
            free_mem(puzzle_ctrl->block_offsets);
        }
        puzzle_ctrl->block_offsets = 0;

        if (puzzle_ctrl->score_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[0]);
        }
        puzzle_ctrl->score_text[0] = 0;
        if (puzzle_ctrl->score_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->score_text[1]);
        }
        puzzle_ctrl->score_text[1] = 0;
        if (puzzle_ctrl->result_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->result_text[0]);
        }
        puzzle_ctrl->result_text[0] = 0;
        if (puzzle_ctrl->result_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->result_text[1]);
        }
        puzzle_ctrl->result_text[1] = 0;
        if (puzzle_ctrl->footer_text[0] != 0) {
            destroy_string_obj(puzzle_ctrl->footer_text[0]);
        }
        puzzle_ctrl->footer_text[0] = 0;
        if (puzzle_ctrl->footer_text[1] != 0) {
            destroy_string_obj(puzzle_ctrl->footer_text[1]);
        }
        puzzle_ctrl->footer_text[1] = 0;
        free_mem(puzzle_ctrl);
        puzzle_ctrl = 0;
    }
    puzzle_mode_net = 0;
    unload_font(0xA);
}

/*
 * Recovery in progress: minigame_puzzlefighter_setup 89.91%, retail 0x980/current
 * 0x9F0. Allocation order, layouts, art loads, localized placements, sequence
 * validation, and all consumers agree with retail. The static-art terminator
 * now distinguishes normal completion from a failure break without a status
 * flag, recovering retail's exact r24-r31 save range. Retail branches directly
 * from five nested resource loops to one failure tail. The victory and
 * super-bar phases share one typed index-returning helper, so the one-round
 * victory exit falls naturally into super-bar setup. UI and wiff setup share a
 * second typed count-returning helper; background setup returns its completed
 * induction count into the same failure condition. The remaining helper exits emit
 * repeated status/exit control flow. Bounded
 * UI, super-bar, and victory-loop open-coding attempts either regressed
 * alignment or grew the body; open-coding all four helpers together grew the
 * body by 0x10 and fell to 83.73%, while directly aborting from the static-art
 * loop grew it by 0x14 and fell to 86.85%. The helpers are therefore retained.
 * This is too large and structural to classify as an
 * emission-only near miss.
 */
static void minigame_puzzlefighter_setup(void) {
    PuzzleLocalizedImagePlacement* localized;
    PuzzleArtPlacement* placement;
    ScreenObj* object;
    int* sequence;
    int sequence_length;
    int localized_index;
    int pfx_ok;
    int value;
    int index;

    load_ssf(puzzlefighter_file_table);
    load_art_section_language(0x70033, &sec_puzzlefighter);
    load_font(0xA);
    load_font(0xB);
    load_font(0xD);
    load_font(0xC);

    do {
        puzzle_ctrl = get_mem(sizeof(*puzzle_ctrl));
        if (puzzle_ctrl == 0) {
            break;
        }
    memset(puzzle_ctrl, 0, sizeof(*puzzle_ctrl));
    puzzle_ctrl->mode_proc = aproc;

    puzzle_ctrl->ui_object_count = 0;
    while (art_puzzle_fighter_pieces_tbl[puzzle_ctrl->ui_object_count] != 0) {
        puzzle_ctrl->ui_object_count++;
    }
    puzzle_ctrl->ui_objects =
        get_mem(puzzle_ctrl->ui_object_count * sizeof(*puzzle_ctrl->ui_objects));

    puzzle_ctrl->block_visual_count = 0;
    while (art_puzzle_fighter_wiff_tbl[puzzle_ctrl->block_visual_count]
               .image_id != 0) {
        puzzle_ctrl->block_visual_count++;
    }
    puzzle_ctrl->breaker_objects = get_mem(
        puzzle_ctrl->block_visual_count * sizeof(*puzzle_ctrl->breaker_objects));
    puzzle_ctrl->block_animations = get_mem(
        puzzle_ctrl->block_visual_count * sizeof(*puzzle_ctrl->block_animations));
    puzzle_ctrl->boards[0] =
        get_mem(PUZZLE_BOARD_CELLS * sizeof(*puzzle_ctrl->boards[0]));
    puzzle_ctrl->boards[1] =
        get_mem(PUZZLE_BOARD_CELLS * sizeof(*puzzle_ctrl->boards[1]));
    puzzle_ctrl->players[0] = get_mem(sizeof(*puzzle_ctrl->players[0]));
    puzzle_ctrl->players[1] = get_mem(sizeof(*puzzle_ctrl->players[1]));
    puzzle_ctrl->current_pairs[0] =
        get_mem(2 * sizeof(*puzzle_ctrl->current_pairs[0]));
    puzzle_ctrl->current_pairs[1] =
        get_mem(2 * sizeof(*puzzle_ctrl->current_pairs[1]));
    puzzle_ctrl->next_pairs[0] =
        get_mem(2 * sizeof(*puzzle_ctrl->next_pairs[0]));
    puzzle_ctrl->next_pairs[1] =
        get_mem(2 * sizeof(*puzzle_ctrl->next_pairs[1]));
    puzzle_ctrl->block_offsets =
        get_mem(PUZZLE_BOARD_CELLS * sizeof(*puzzle_ctrl->block_offsets));
    pfx_ok = init_pz_pfx_2d();

    if (puzzle_ctrl->ui_objects == 0 ||
        puzzle_ctrl->breaker_objects == 0 ||
        puzzle_ctrl->block_animations == 0 ||
        puzzle_ctrl->boards[0] == 0 || puzzle_ctrl->boards[1] == 0 ||
        puzzle_ctrl->players[0] == 0 || puzzle_ctrl->players[1] == 0 ||
        puzzle_ctrl->current_pairs[0] == 0 ||
        puzzle_ctrl->current_pairs[1] == 0 ||
        puzzle_ctrl->next_pairs[0] == 0 || puzzle_ctrl->next_pairs[1] == 0 ||
        puzzle_ctrl->block_offsets == 0 || pfx_ok == 0) {
        break;
    }

    puzzle_ctrl->fight_message = 0;
    puzzle_ctrl->result_message = 0;
    __pz_feed_rand_msg.array_index = 0;
    while (puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence !=
           0) {
        __pz_feed_rand_msg.array_index++;
    }
    if (puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence ==
        0) {
        __pz_feed_rand_msg.array_index =
            randu0((unsigned short)__pz_feed_rand_msg.array_index);
    }
    if (puzzle_ctrl->sequence_bits.piece_sequence_owned != 0 &&
        puzzle_ctrl->piece_sequence != 0) {
        free_mem(puzzle_ctrl->piece_sequence);
    }
    puzzle_ctrl->piece_sequence = 0;
    sequence =
        puzzle_array_table_local[__pz_feed_rand_msg.array_index].sequence;
    if (sequence != 0) {
        __pz_feed_rand_msg.sequence = sequence;
        sequence_length = 0;
        while ((value = sequence[sequence_length]) >= 0) {
            if (value == 0) {
                ((PuzzleProcVtable*)aproc->vtbl)
                    ->transfer(p_pz_mode_exit, 0.0f);
                break;
            }
            if (value > 8) {
                if (value == PUZZLE_BLOCK_SUPERBREAKER) {
                    puzzle_ctrl->flags2_bits.network_sequence_marker = 1;
                } else if (value != PUZZLE_BLOCK_WILDCARD) {
                    ((PuzzleProcVtable*)aproc->vtbl)
                        ->transfer(p_pz_mode_exit, 0.0f);
                    break;
                }
            }
            sequence_length++;
        }
        if (value < 0) {
            __pz_feed_rand_msg.sequence_length = sequence_length;
            puzzle_ctrl->sequence_bits.piece_sequence_owned = 0;
        }
    }
    puzzle_ctrl->piece_sequence_length = __pz_feed_rand_msg.sequence_length;
    puzzle_ctrl->piece_sequence = __pz_feed_rand_msg.sequence;

    index = 0;
    while (art_puzzle_fighter_static_tbl.placements[index].image_id != 0) {
        placement = &art_puzzle_fighter_static_tbl.placements[index];
        localized = 0;
        localized_index = -1;
        if (placement->image_id == 0x08020011) {
            localized_index = 0;
        } else if (placement->image_id == 0x08020014) {
            localized_index = 1;
        }
        if (localized_index >= 0) {
            localized = &pzlang_image_table[localized_index]
                             .language[get_language()];
            if (placement->x == -2 && localized->player2_x < 0) {
                index++;
                continue;
            }
        }
        object = load_2d_pfxobj(0x70033, 0x601A,
                                (char*)placement->image_id, 0, 0x3D);
        if (object == 0) {
            break;
        }
        object->flag_bits.bit2 = 1;
        if (localized != 0) {
            if (placement->x == -1) {
                object->x = localized->player1_x;
            } else if (placement->x == -2) {
                object->x = localized->player2_x;
            }
            object->y = localized->setup_y;
        } else {
            object->x = placement->x;
            object->y = placement->y;
        }
        puzzle_ctrl->round_objects[index] = object;
        if (index >= 13) {
            break;
        }
        index++;
    }
    if (art_puzzle_fighter_static_tbl.placements[index].image_id != 0) {
        break;
    }

    if (art_puzzle_fighter_super_bar_tbl[
            puzzle_fighter_setup_victory_and_super_bar_objects()].image_id !=
            0 ||
        puzzle_fighter_setup_background_objects() != 2 ||
        puzzle_fighter_setup_ui_and_wiff_objects() !=
            puzzle_ctrl->block_visual_count) {
        break;
    }

    if (g_game_info.plyr0.field_48 != 0) {
        sprintf(temp_80_char, get_string(5), g_game_info.plyr0.field_48);
        puzzle_ctrl->result_text[0] =
            string_left_xy(0x6022, 0, temp_80_char, 30, 45, 0x3A);
    }
    if (g_game_info.plyr1.field_48 != 0) {
        sprintf(temp_80_char, get_string(5), g_game_info.plyr1.field_48);
        puzzle_ctrl->result_text[1] =
            string_right_xy(0x6022, 0, temp_80_char, 610, 45, 0x3A);
    }
    if (puzzle_ctrl->result_text[0] != 0) {
        pull_string_obj(puzzle_ctrl->result_text[0]);
    }
    if (puzzle_ctrl->result_text[1] != 0) {
        pull_string_obj(puzzle_ctrl->result_text[1]);
    }
    if (g_game_info.feature_flags.bits.powerbars_locked != 0) {
        puzzle_ctrl->timed_match_ticks = 0x708;
    }

    memset(puzzle_ctrl->players[0], 0, sizeof(*puzzle_ctrl->players[0]));
    memset(puzzle_ctrl->players[1], 0, sizeof(*puzzle_ctrl->players[1]));
    memset(puzzle_ctrl->current_pairs[0], 0,
           2 * sizeof(*puzzle_ctrl->current_pairs[0]));
    memset(puzzle_ctrl->current_pairs[1], 0,
           2 * sizeof(*puzzle_ctrl->current_pairs[1]));
    puzzle_fighter_mode_clear();
    puzzle_ctrl->players[0]->profile_stats = &p1_profile.puzzle_stats;
    puzzle_ctrl->players[1]->profile_stats = &p2_profile.puzzle_stats;
        return;
    } while (0);

    puzzle_fighter_setup_abort();
}

void render_minigame_list(void) {
    if (puzzle_ctrl->players[0]->flags3_bits.hide_active_piece == 0 &&
        puzzle_ctrl->players[1]->flags3_bits.hide_active_piece == 0) {
        render_wiffs(puzzle_ctrl->players[0]);
        render_wiffs(puzzle_ctrl->players[1]);
    }
    if (puzzle_ctrl->score_text[0] != 0) {
        pfxfont_begin_render();
        pfxfont_string_render(&puzzle_ctrl->score_text[0]->pfx,
                              (float)puzzle_ctrl->score_text[0]->render_x,
                              (float)puzzle_ctrl->score_text[0]->render_y);
        pfxfont_end_render();
    }
    if (puzzle_ctrl->score_text[1] != 0) {
        pfxfont_begin_render();
        pfxfont_string_render(&puzzle_ctrl->score_text[1]->pfx,
                              (float)puzzle_ctrl->score_text[1]->render_x,
                              (float)puzzle_ctrl->score_text[1]->render_y);
        pfxfont_end_render();
    }
    if (puzzle_ctrl->result_text[0] != 0) {
        pfxfont_begin_render();
        pfxfont_string_render(&puzzle_ctrl->result_text[0]->pfx,
                              (float)puzzle_ctrl->result_text[0]->render_x,
                              (float)puzzle_ctrl->result_text[0]->render_y);
        pfxfont_end_render();
    }
    if (puzzle_ctrl->result_text[1] != 0) {
        pfxfont_begin_render();
        pfxfont_string_render(&puzzle_ctrl->result_text[1]->pfx,
                              (float)puzzle_ctrl->result_text[1]->render_x,
                              (float)puzzle_ctrl->result_text[1]->render_y);
        pfxfont_end_render();
    }

    if (g_fatality_sound == 0) {
        update_super_bar_verts(puzzle_ctrl->players[0]);
        update_super_bar_verts(puzzle_ctrl->players[1]);
        render_UI(puzzle_ctrl->players[0]);
        render_UI(puzzle_ctrl->players[1]);
    }

    if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
        network_pause_procs == 0) {
        if (puzzle_ctrl->match_delay != 0) {
            puzzle_ctrl->match_delay--;
        }
        if (puzzle_ctrl->players[0]->match_delay != 0) {
            puzzle_ctrl->players[0]->match_delay--;
        }
        if (puzzle_ctrl->players[1]->match_delay != 0) {
            puzzle_ctrl->players[1]->match_delay--;
        }
    }
}

__declspec(section ".rodata") const char puzzle_strings[] =
    PUZZLE_STRING_DATA;

/* Recovery in progress: complete typed 14x8 wiff renderer. The structured
 * inner-table exit leaves the current body 16 bytes smaller and changes
 * register allocation; this remains structural, not emission-only. */
static void render_wiffs(PuzzlePlayerState* player) {
    int board_moving = 1;
    int match_active = 1;
    int shake_x;
    int shake_y;
    int row;
    int column;
    PuzzleArtPlacement* placement;

    if (puzzle_ctrl->flag_bits.large_color_clear == 0 &&
        player->flags2_bits.board_shift_active == 0) {
        board_moving = 0;
    }
    if (puzzle_ctrl->match_delay == 0 && player->match_delay == 0) {
        match_active = 0;
    }
    if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
        network_pause_procs == 0 && board_moving != 0 && match_active != 0) {
        shake_x = signrand(3);
        shake_y = signrand(3);
    } else {
        shake_x = 0;
        shake_y = 0;
    }

    placement = &art_puzzle_fighter_static_tbl
                     .placements[player->event_player + 1];
    for (row = 0; row < 14; row++) {
        for (column = 0; column < 8; column++) {
            PuzzleBoardCell* cell = &player->board[row * 8 + column];

            if (cell->type != 0 && cell->state != 0) {
                int color;

                if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
                    network_pause_procs == 0 && board_moving == 0 &&
                    match_active != 0) {
                    shake_x = signrand(3);
                    shake_y = signrand(3);
                }

                for (color = 0;
                     art_puzzle_fighter_wiff_tbl[color].image_id != 0;
                     color++) {
                    if (puzzle_ctrl->block_animations[color] ==
                        cell->animation) {
                        ScreenObj* object = puzzle_ctrl->breaker_objects[color];

                        object->pfx2d->x =
                            placement->x + column * 25 + 4 + shake_x;
                        object->pfx2d->y =
                            placement->y + row * 25 + 5 + shake_y;
                        object->pfx2d->x -= 16;
                        object->pfx2d->y -= 16;
                        object->pfx2d->texture = get_ani_texture_rwtexture(
                            puzzle_ctrl->block_animations[color],
                            0x2B - cell->state);
                        pfx2d_begin_render();
                        pfx2d_render(object->pfx2d);
                        pfx2d_end_render();
                        break;
                    }
                }

                if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
                    network_pause_procs == 0) {
                    cell->state--;
                    if (cell->state == 0) {
                        cell->type = 0;
                        cell->animation = 0;
                    }
                }
            }
        }
    }
}

/* Emission-only near match (93.16%, retail 0x220/current 0x214). The typed UI
 * table, placements, branches, and render calls agree. The three-instruction
 * deficit is placement-base CSE and resulting register coloring; distinct
 * scoped aliases and retail-ordered final assignments compile identically. */
static void render_UI(PuzzlePlayerState* player) {
    int placement_index = player->event_player + 1;
    PuzzleArtPlacement* placement;
    ScreenObj* object;

    if (player->flags3_bits.hide_active_piece != 0) {
        StringObj* block_count;

        if (g_game_info.pause_flag_bits.controller_disable_guard == 0 &&
            network_pause_procs == 0) {
            if (player->block_count_anim_timer != 0) {
                player->block_count_anim_timer--;
            } else {
                player->block_count_anim_value =
                    player->block_count_anim_value == 0;
                player->block_count_anim_timer = 10;
            }
        }

        placement =
            &art_puzzle_fighter_static_tbl.placements[placement_index];
        object = puzzle_ctrl->ui_objects[player->block_count_anim_value + 3];
        object->pfx2d->x = placement->x + 4;
        object->pfx2d->y = placement->y + 5;
        object->pfx2d->y += 305;
        pfx2d_begin_render();
        pfx2d_render(object->pfx2d);
        pfx2d_end_render();

        if (player->block_count_anim_value != 0) {
            block_count = player->block_count_font_11;
        } else {
            block_count = player->block_count_font_10;
        }
        block_count->render_x = object->pfx2d->x + 150;
        block_count->render_y = object->pfx2d->y + 1;
        pfxfont_begin_render();
        pfxfont_string_render(&block_count->pfx,
                              (float)block_count->render_x,
                              (float)block_count->render_y);
        pfxfont_end_render();
    }

    placement =
        &art_puzzle_fighter_static_tbl.placements[placement_index];
    object = puzzle_ctrl->ui_objects[2];
    object->pfx2d->x = placement[10].x + 7;
    object->pfx2d->y = placement[10].y + 24;
    pfx2d_begin_render();
    pfx2d_render(object->pfx2d);
    pfx2d_end_render();

    placement =
        &art_puzzle_fighter_static_tbl.placements[placement_index];
    object = puzzle_ctrl->ui_objects[1];
    object->pfx2d->x = placement->x + 4;
    object->pfx2d->y = placement->y + 5;
    object->pfx2d->y += 300;
    pfx2d_begin_render();
    pfx2d_render(object->pfx2d);
    pfx2d_end_render();
}
