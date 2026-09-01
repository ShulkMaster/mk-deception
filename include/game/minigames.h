#ifndef GAME_MINIGAMES_H
#define GAME_MINIGAMES_H

typedef struct PuzzleFighterEvent {
    int player;
    int type;
    float block_count;
    float chain_count;
} PuzzleFighterEvent;

void pz_fighter_event(PuzzleFighterEvent* event);
void render_minigame_list(void);
void cleanup_minigame_system(void);

extern int __mini_game_display_ctrl;

#endif
