#ifndef MKD_GAME_COLLISION_H
#define MKD_GAME_COLLISION_H

#include "math/gxVect.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"

typedef struct CollisionPaddedVec {
    Vec value;
    float pad;
} CollisionPaddedVec; /* 0x10 */

typedef struct CollisionShape {
    union {
        char data00[0x80];
        struct {
            Vec sphere_center; /* +0x00 */
            float sphere_radius; /* +0x0C */
            char sphere_pad10[0x70];
        };
        struct {
            Vec cylinder_axis; /* +0x00 */
            char cylinder_pad0C[4];
            Vec cylinder_center; /* +0x10 */
            char cylinder_pad1C[4];
            float cylinder_radius; /* +0x20 */
            float cylinder_height; /* +0x24 */
            char cylinder_pad28[0x58];
        };
        struct {
            Vec box_corner_0;      /* +0x00 */
            float box_pad_0C;
            Vec box_corner_1;      /* +0x10 */
            float box_pad_1C;
            Vec box_corner_2;      /* +0x20 */
            float box_pad_2C;
            Vec box_corner_3;      /* +0x30 */
            float box_pad_3C;
            Vec box_axis_0;        /* +0x40 */
            float box_axis_0_min;  /* +0x4C */
            Vec box_axis_1;        /* +0x50 */
            float box_axis_0_max;  /* +0x5C */
            Vec box_axis_2;        /* +0x60 */
            float box_axis_1_min;  /* +0x6C */
            float box_axis_1_max;  /* +0x70 */
            float box_axis_2_min;  /* +0x74 */
            float box_axis_2_max;  /* +0x78 */
            float box_pad_7C;
        };
        struct {
            Vec quad_vertex_0;     /* +0x00 */
            float quad_pad_0C;
            Vec quad_vertex_1;     /* +0x10 */
            float quad_pad_1C;
            Vec quad_vertex_2;     /* +0x20 */
            float quad_pad_2C;
            Vec quad_vertex_3;     /* +0x30 */
            float quad_pad_3C[16];
        };
        CollisionPaddedVec quad_vertices[8];
    };
    unsigned int type; /* +0x80, low three bits select the shape kind */
    char data84[0x0C];
} CollisionShape; /* 0x90 */

typedef struct CollisionObjList {
    MkHdr hdr;
    MkPtr* objects; /* +0x08 */
} CollisionObjList; /* 0x0C */

typedef struct CollisionObj {
    MkHdr hdr;
    unsigned int flags;       /* +0x08 */
    unsigned int obstacle_id; /* +0x0C */
    CollisionShape shape; /* +0x10 */
} CollisionObj; /* 0xA0 */

/* Runtime-owned node: local and transformed shapes share one bone owner. */
typedef struct PlayerCollisionNode {
    struct MkBone* bone; /* +0x00 */
    unsigned int reserved04[3];
    CollisionShape local_shape; /* +0x10 */
    CollisionShape world_shape; /* +0xA0 */
} PlayerCollisionNode; /* 0x130 */

struct PlayerCollisionData {
    struct MkObj* object; /* +0x0000 */
    unsigned int reserved04[3];
    CollisionShape body_shape; /* +0x0010 */
    PlayerCollisionNode joints[28]; /* +0x00A0 */
    PlayerCollisionNode attacks[36]; /* +0x21E0 */
    PlayerCollisionNode saved_attacks[36]; /* +0x4CA0 */
    CollisionShape recorded_shapes[36]; /* +0x7760 */
    PlayerCollisionNode active_nodes[7]; /* +0x8BA0 */
    unsigned int joint_count; /* +0x93F0 */
    unsigned int field_93F4; /* current attack count */
    union {
        unsigned int field_93F8; /* saved attack count */
        int attack_region_index;
    };
    unsigned int recorded_count; /* +0x93FC */
    unsigned int active_count; /* +0x9400 */
    unsigned int render_recorded; /* +0x9404 */
    float active_scale; /* +0x9408 */
    float attack_radius; /* +0x940C */
};

typedef char PlayerCollisionNodeSizeCheck[sizeof(PlayerCollisionNode) == 0x130 ? 1 : -1];
typedef char PlayerCollisionDataSizeCheck[sizeof(PlayerCollisionData) == 0x9410 ? 1 : -1];

typedef void (*GlobalCollisionCallback)(const unsigned int* obstacle_id);

void build_col_shape_vertical_cylinder(
    CollisionShape* shape, const Vec* center, float radius, float height);
void build_col_shape_vertical_box(
    CollisionShape* shape, const Vec* center, float width, float height,
    float depth, float angle);
CollisionObj* add_shape_to_global_collision_list(
    const CollisionShape* shape, unsigned int flags);
int is_point_inside_shape(const CollisionShape* shape, const Vec* point);
int get_shape_center_for_collision_obstacle(
    CollisionObj* obstacle, Vec* center);
int collide_segment_against_global_collision_list(
    const Vec* start, const Vec* end, Vec* hit_point,
    unsigned int ignored_flags);
int repel_point_against_global_collision_list_toward_target(
    const Vec* target, const Vec* start, Vec* result_point,
    unsigned int ignored_flags);
void update_collision_obj_pos(CollisionObj* object, const Vec* position);
void collision_obj_set_shape(
    CollisionObj* object, const CollisionShape* shape);
void init_player_collision(PlyrInfo* player);

#endif
