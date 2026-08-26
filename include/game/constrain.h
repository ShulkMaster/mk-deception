#ifndef MKD_GAME_CONSTRAIN_H
#define MKD_GAME_CONSTRAIN_H

#include "runtime/mk_struct.h"

extern Vec tightrope_perp_uv;

typedef int (*ArenaObstacleCallback)(void);

typedef struct ConstrainInfo {
    MkPtr* obstacles;
    ArenaObstacleCallback callback;
} ConstrainInfo;

typedef union ArenaObstacleFlags {
    unsigned char value;
    struct {
        unsigned char repel : 1;
        unsigned char disabled : 1;
        unsigned char danger_zone : 1;
        unsigned char pad : 5;
    } bits;
} ArenaObstacleFlags;

typedef union ArenaObstacleInternalFlags {
    unsigned short value;
    struct {
        unsigned short internal_id : 14;
        unsigned short flag_1 : 1;
        unsigned short flag_0 : 1;
    } bits;
} ArenaObstacleInternalFlags;

typedef struct ArenaObstacle {
    MkHdr hdr;
    unsigned int obstacle_id;            /* +0x08 */
    ArenaObstacleInternalFlags internal; /* +0x0C */
    unsigned char pad0E[2];
    union {
        unsigned int flags_word;
        struct {
            ArenaObstacleFlags flags; /* +0x10 */
            unsigned char flags_pad[3];
        };
    };
    int type;       /* +0x14 */
    MkPtr* shapes; /* +0x18 */
} ArenaObstacle; /* 0x1C */

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

#endif
