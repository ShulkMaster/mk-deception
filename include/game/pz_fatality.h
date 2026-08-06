#ifndef MKD_GAME_PZ_FATALITY_H
#define MKD_GAME_PZ_FATALITY_H

typedef float (*PuzzleFatalityProcessFn)(void);

typedef struct PuzzleAttackParameters {
    float field_00;
    float field_04;
    float field_08;
    unsigned int field_0C;
    unsigned int field_10;
    unsigned int field_14;
    float field_18;
    float desired_distance; /* +0x1C */
    float hit_distance; /* +0x20 */
    float end_frame; /* +0x24 */
    float reaction_frame; /* +0x28 */
    unsigned int walk_to_range; /* +0x2C */
    unsigned int ignore_distance; /* +0x30 */
    unsigned int reaction_mode; /* +0x34 */
    unsigned int has_followup; /* +0x38 */
} PuzzleAttackParameters; /* 0x3C */

typedef struct PuzzleFatalityRandomEvent {
    char pad00[0x10];
    int state; /* +0x10 */
    char pad14[4];
    unsigned int side; /* +0x18 */
    int active; /* +0x1C */
    int started; /* +0x20 */
} PuzzleFatalityRandomEvent; /* 0x24 */

float pz_fighter_process_random_fatality_event(
    PuzzleFatalityRandomEvent* event, PuzzleFatalityProcessFn reaction);

#endif
