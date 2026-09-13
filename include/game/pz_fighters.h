#ifndef MKD_PZ_FIGHTERS_H
#define MKD_PZ_FIGHTERS_H

#include "math/gxVect.h"
typedef struct MkObj MkObj;
typedef struct ScreenObj ScreenObj;
typedef struct AniTextureControl AniTextureControl;
typedef struct PuzzlePresentState PuzzlePresentState;
typedef struct MkProc MkProc;

typedef struct PuzzleFighterMove {
    unsigned int event_type; /* +0x00 */
    float block_count; /* +0x04 */
    float chain_count; /* +0x08 */
    char pad0C[4];
    unsigned int mode; /* +0x10 */
    unsigned int script_move; /* +0x14 */
    unsigned int player; /* +0x18 */
    int distance_class; /* +0x1C */
    unsigned int active_flags; /* +0x20 */
    unsigned int has_followup; /* +0x24 */
    union {
        unsigned int policy_word; /* +0x28 */
        struct {
            unsigned char policy_flags; /* +0x28 */
            unsigned char runtime_flags; /* +0x29 */
            unsigned char policy_pad[2];
        };
    };
    char pad2C[8];
} PuzzleFighterMove; /* 0x34 */

typedef struct PuzzleFighterStartFlags {
    signed char enabled : 1; /* bit7 */
    unsigned char player0_started : 1; /* bit6 */
    unsigned char player1_started : 1; /* bit5 */
    signed char player0_scored : 1; /* bit4 */
    signed char player1_scored : 1; /* bit3 */
    unsigned char unused : 3;
} PuzzleFighterStartFlags;

typedef struct PuzzleFighterStartFlagGroups {
    unsigned char enabled_pad : 1; /* bit7 */
    signed char players_started : 2; /* bits6-5 */
    signed char players_scored : 2; /* bits4-3 */
    signed char unused : 3;
} PuzzleFighterStartFlagGroups;

typedef struct PuzzleAttackPolicyFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char distance_reaction : 1; /* bit3 */
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
} PuzzleAttackPolicyFlags;

typedef struct PuzzleAttackRuntimeFlags {
    unsigned char enabled : 1; /* bit7 */
    unsigned char low_bits : 7;
} PuzzleAttackRuntimeFlags;

typedef struct PuzzleFightersEngine {
    float balance; /* +0x00 */
    Vec arena_axis; /* +0x04 */
    Vec constraint_axis; /* +0x10 */
    float center_x; /* +0x1C */
    float center_y; /* +0x20 */
    float center_z; /* +0x24 */
    float player1_idle_x; /* +0x28 */
    float player1_idle_y; /* +0x2C */
    float player1_idle_z; /* +0x30 */
    float player2_idle_x; /* +0x34 */
    float player2_idle_y; /* +0x38 */
    float player2_idle_z; /* +0x3C */
    Vec fighter_posts[2]; /* +0x40 - home/grinder posts */
    int round_running; /* +0x58 */
    int super_move_active; /* +0x5C */
    int random_fatality_active; /* +0x60 */
    union {
        PuzzleFighterMove fighter_move; /* +0x64 */
        struct {
            char pad_move64[0x20];
            unsigned int distance_flags; /* +0x84 */
            unsigned int attack_has_followup; /* +0x88 */
            union {
                unsigned int attack_policy_word; /* +0x8C */
                struct {
                    union {
                        unsigned char attack_policy_flags;
                        PuzzleAttackPolicyFlags attack_policy_bits;
                    }; /* +0x8C */
                    union {
                        unsigned char attack_runtime_flags;
                        PuzzleAttackRuntimeFlags attack_runtime_bits;
                    }; /* +0x8D */
                    unsigned char attack_policy_pad[2];
                };
            };
            unsigned int continuation_move; /* +0x90 */
            unsigned int continuation_priority; /* +0x94 */
        };
    };
    unsigned int pending_move_count; /* +0x98 */
    PuzzleFighterMove pending_moves[2]; /* +0x9C */
    int fighters_positioned; /* +0x104 */
    MkProc* master_proc; /* +0x108 */
    int positioning_active; /* +0x10C */
    int fighter_state[2]; /* +0x110 */
    int force_repel; /* +0x118 */
    int fatality_abort; /* +0x11C */
    int fatality_active; /* +0x120 */
    int fatality_ready; /* +0x124 */
    int fatality_victim; /* +0x128 */
    int fatality_attacker; /* +0x12C */
    MkObj* fatality_objects[8]; /* +0x130 */
    MkObj* present_object; /* +0x150 */
    MkObj* projectile_objects[2]; /* +0x154 */
    int fatality_index; /* +0x15C */
    unsigned int random_event_cooldown; /* +0x160 */
    int breakout; /* +0x164 */
    int y_constraint_enabled[2]; /* +0x168 */
    float y_constraint[2]; /* +0x170 */
    unsigned int fatality_timer; /* +0x178 */
    float fatality_motion; /* +0x17C */
    ScreenObj* screen_objects[2]; /* +0x180 */
    AniTextureControl* texture_controls[2]; /* +0x188 */
    unsigned int balance_update_timer; /* +0x190 */
    float pending_balance; /* +0x194 */
    union {
        unsigned char start_flags;
        PuzzleFighterStartFlags start_flag_bits;
        PuzzleFighterStartFlagGroups start_flag_groups;
    }; /* +0x198 */
    char pad199[3];
    unsigned int immediate_request_player; /* +0x19C */
    unsigned int immediate_request_type; /* +0x1A0 */
    int immediate_request_timer; /* +0x1A4 */
    int immediate_request_active; /* +0x1A8 */
    unsigned int event_block_count; /* +0x1AC */
    char pad1B0[4];
    int field_1B4;
    int balance_out_of_range; /* +0x1B8 */
    PuzzlePresentState* present; /* +0x1BC */
    int reaction_slots[6]; /* +0x1C0 */
    int field_1D8;
    int super_move_request_pending; /* +0x1DC */
    int constraint_timer; /* +0x1E0 */
    int reactions_disabled; /* +0x1E4 */
} PuzzleFightersEngine;

#endif
