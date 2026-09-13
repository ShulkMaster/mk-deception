#ifndef GAME_GROUND_FX_H
#define GAME_GROUND_FX_H

/* Retail uses these globals as optional no-argument indirect call targets. */
typedef void (*GroundFxCallback)(void);

extern GroundFxCallback small_ground_fx;
extern GroundFxCallback large_ground_fx;

#endif
