#ifndef MKD_PZ_FIGHTERS_H
#define MKD_PZ_FIGHTERS_H

#include "math/gxVect.h"
typedef struct MkObj MkObj;
typedef struct ScreenObj ScreenObj;
typedef struct AniTextureControl AniTextureControl;
typedef struct PuzzlePresentState PuzzlePresentState;
typedef struct MkProc MkProc;

typedef struct PuzzleAttackPolicyFlags {
    unsigned char bit7 : 1;
    unsigned char special_move_4 : 1;
    unsigned char special_move_5 : 1;
    unsigned char bit4 : 1;
    unsigned char continuation_allowed : 1;
    unsigned char continuation_pending : 1;
    unsigned char easy_continuation : 1;
    unsigned char bit0 : 1;
} PuzzleAttackPolicyFlags;

typedef struct PuzzleAttackRuntimeFlags {
    unsigned char enabled : 1;
    unsigned char low_bits : 7;
} PuzzleAttackRuntimeFlags;

typedef struct PuzzleFighterMove {
    int event_type;
    float block_count;
    float chain_count;
    char pad0C[4];
    unsigned int mode;
    unsigned int script_move;
    unsigned int player;
    int distance_class;
    unsigned int active_flags;
    unsigned int has_followup;
    union {
        unsigned int policy_word;
        struct {
            union {
                unsigned char policy_flags;
                PuzzleAttackPolicyFlags policy_bits;
            };
            union {
                unsigned char runtime_flags;
                PuzzleAttackRuntimeFlags runtime_bits;
            };
        };
    };
    unsigned int continuation_move;
    unsigned int continuation_priority;
} PuzzleFighterMove;

typedef struct PuzzleFighterStartFlags {
    signed char enabled : 1;
    signed char player0_started : 1;
    signed char player1_started : 1;
    signed char player0_scored : 1;
    signed char player1_scored : 1;
    unsigned char unused : 3;
} PuzzleFighterStartFlags;

typedef struct PuzzleFightersEngine {
    float balance;
    Vec arena_axis;
    Vec constraint_axis;
    Vec center;
    Vec player1_idle;
    Vec player2_idle;
    Vec fighter_posts[2];
    int round_running;
    int super_move_active;
    int random_fatality_active;
    PuzzleFighterMove fighter_move;
    unsigned int pending_move_count;
    PuzzleFighterMove pending_moves[2];
    int fighters_positioned;
    MkProc* master_proc;
    int positioning_active;
    int fighter_state[2];
    int force_repel;
    int fatality_abort;
    int fatality_active;
    int fatality_ready;
    int fatality_victim;
    int fatality_attacker;
    MkObj* fatality_objects[8];
    MkObj* present_object;
    MkObj* projectile_objects[2];
    int fatality_index;
    unsigned int random_event_cooldown;
    int breakout;
    int y_constraint_enabled[2];
    float y_constraint[2];
    unsigned int fatality_timer;
    float fatality_motion;
    ScreenObj* screen_objects[2];
    AniTextureControl* texture_controls[2];
    unsigned int balance_update_timer;
    float pending_balance;
    PuzzleFighterStartFlags start_flag_bits;
    char pad199[3];
    unsigned int immediate_request_player;
    unsigned int immediate_request_type;
    int immediate_request_timer;
    int immediate_request_active;
    unsigned int event_block_count;
    char pad1B0[4];
    int field_1B4;
    int balance_out_of_range;
    PuzzlePresentState* present;
    int reaction_slots[6];
    int field_1D8;
    int super_move_request_pending;
    int constraint_timer;
    int reactions_disabled;
} PuzzleFightersEngine;

float pz_fighter_check_breakout(void);
void pz_fighter_dont_fudge_desired_distance(void);
int pz_fighter_is_losing_big(unsigned int player);
float pz_fighter_fetch_plyr_to_home_post_distance(int player);
int pz_fighter_should_he_breakout(void);
void pz_fighter_set_y_constrain(MkObj* player_obj, int mode, float value);
int pz_fighter_is_winning_big(unsigned int player);
float pz_fighter_fetch_distance_to_center_pos(void);
void pz_fighter_move_into_fighting_position(void);
extern struct PuzzleFighterMoveTables g_pz_fighter_tables;
void pz_fighter_force_repel_during_attack(void);
extern float p_plyr_pz_fighter_start(void);
#endif
