#ifndef GAME_MINIGAMES_H
#define GAME_MINIGAMES_H

typedef struct PuzzleFighterEvent {
    unsigned int player;
    int type;
    float block_count;
    float chain_count;
} PuzzleFighterEvent;

void pz_fighter_event(PuzzleFighterEvent* event);
float p_puzzle_switch_4(void);
float p_puzzle_switch_3(void);
float p_puzzle_switch_1(void);
float p_puzzle_switch_drop(void);
float p_puzzle_switch_2(void);
float p_puzzle_switch_right(void);
float p_puzzle_switch_left(void);
float p_puzzle_switch_down(void);
float p_puzzle_switch_up(void);
float p_puzzle_switch_lt_stick(void);
void render_minigame_list(void);
void cleanup_minigame_system(void);
void load_puzzle_champion_screen(void);

extern int __mini_game_display_ctrl;

#endif
