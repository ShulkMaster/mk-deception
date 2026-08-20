#ifndef GAME_JDN_H
#define GAME_JDN_H

#include "math/gxVect.h"

typedef struct MkPfx MkPfx;

MkPfx* start_pfx_glass_shards(
    int art_id, const Vec* position, const Vec* center, int bounce_limit,
    unsigned int spawn_count, unsigned int scale_mode, int motion_mode);
void allow_shard_pfx_now(void);
void kill_shard_pfx_now(void);
float get_soul_sine(int index);
int build_sine_table_for_scripts(void);
void build_sine_table(void);

#endif
