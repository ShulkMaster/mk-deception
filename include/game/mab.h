#ifndef GAME_MAB_H
#define GAME_MAB_H

typedef struct Vec Vec;

typedef struct PlyrInfo PlyrInfo;

void do_lightning_strike(PlyrInfo* owner, Vec* position);
void setup_screen_for_fatality(void);

#endif
