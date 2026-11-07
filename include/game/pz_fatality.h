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

float pz_fighter_ani_attack(
    float end_frame, int reaction, float reaction_frame,
    float hit_distance, unsigned int reaction_mode);

float pz_fighter_process_random_fatality_event(
    PuzzleFatalityRandomEvent* event, PuzzleFatalityProcessFn reaction);
void cleanup_pz_fatality_stuff(void);

void pz_fighters_fatality_preround_event(void);
void pz_fighters_fatality_in_progress(void);
float r_pz_fighter_grinding(void);
int pz_fighter_check_fatality_random_event(void);
int pz_fighter_fatality_during_round_stuff_over(void);
void pz_fighters_fatality_prep_chores(void);
float r_pz_fighter_rx_get_to_point(void);
void pz_fighters_fatality_normal_fighting(int enabled);
void pz_fighters_fatality_round_over(void);
void pz_fighters_fatality_unload(void);
#endif
