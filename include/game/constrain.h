#ifndef MKD_GAME_CONSTRAIN_H
#define MKD_GAME_CONSTRAIN_H

#include "math/gxVect.h"
#include "runtime/mk_struct.h"

extern Vec tightrope_perp_uv;

typedef int (*ArenaObstacleCallback)(void);

typedef struct ConstrainInfo {
    MkPtr* obstacles;
    ArenaObstacleCallback callback;
} ConstrainInfo;

struct ArenaObstacleFlagBits {
    unsigned char repel : 1;
    unsigned char disabled : 1;
    unsigned char danger_zone : 1;
    unsigned char inverted : 1;
    unsigned char callback_handled : 1;
    unsigned char pad : 3;
};

typedef union ArenaObstacleFlags {
    unsigned char value;
    struct ArenaObstacleFlagBits bits;
} ArenaObstacleFlags;

struct ArenaObstacleInternalBits {
    unsigned short internal_id : 14;
    unsigned short flag_1 : 1;
    unsigned short flag_0 : 1;
};

typedef union ArenaObstacleInternalFlags {
    unsigned short value;
    struct ArenaObstacleInternalBits bits;
} ArenaObstacleInternalFlags;

typedef struct ArenaObstacle {
    MkHdr hdr;
    unsigned int obstacle_id;
    ArenaObstacleInternalFlags internal;
    unsigned char pad0E[2];
    union {
        unsigned int flags_word;
        ArenaObstacleFlags flags;
    };
    int type;
    MkPtr* shapes;
} ArenaObstacle;

struct CollisionShape;

void set_constrain_last_pos(int player, const Vec* position);
void set_constrain_last_pos_pdata(const Vec* position);
ArenaObstacle* add_shape_to_background_obstacle_list(
    const struct CollisionShape* shape, unsigned int obstacle_id);
void set_background_obstacle_disable_flag(
    int obstacle_id, int disabled);
void set_background_obstacle_repel_flag(
    int obstacle_id, int repel_disabled);
int get_obstacle_type_from_id(unsigned int obstacle_id);
extern ConstrainInfo constrain_info;
int local_obstacle_callback(ArenaObstacle* obstacle);
struct PlyrPdata;
int local_collision_allowed(struct PlyrPdata* player);
void vdestroy_obstacle(ArenaObstacle* obstacle);

#endif
