#ifndef MKD_GAME_COLLISION_H
#define MKD_GAME_COLLISION_H

#include "math/gxVect.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"

typedef struct CollisionPaddedVec {
    Vec value;
    float pad;
} CollisionPaddedVec; /* 0x10 */

typedef struct CollisionQuad {
    CollisionPaddedVec vertices[4];
} CollisionQuad;

typedef struct CollisionSphere {
    Vec center;
    float radius;
} CollisionSphere;

typedef struct CollisionCylinder {
    CollisionPaddedVec axis;
    CollisionPaddedVec center;
    float radius;
    float height;
} __attribute__((aligned(16))) CollisionCylinder;

typedef char CollisionCylinderSizeCheck[
    sizeof(CollisionCylinder) == 0x30 ? 1 : -1];

typedef struct CollisionBox {
    CollisionPaddedVec corners[4];
    Vec axis_0;
    float axis_0_min;
    Vec axis_1;
    float axis_0_max;
    Vec axis_2;
    float axis_1_min;
    float axis_1_max;
    float axis_2_min;
    float axis_2_max;
    float bounding_radius;
} __attribute__((aligned(16))) CollisionBox;

typedef char CollisionBoxSizeCheck[
    sizeof(CollisionBox) == 0x80 ? 1 : -1];

typedef struct CollisionShape {
    union {
        CollisionCylinder cylinder;
        CollisionBox box;
        CollisionSphere sphere;
        CollisionQuad quad;
    };
    unsigned int type;
} __attribute__((aligned(16))) CollisionShape;

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
    CollisionShape local_shape; /* +0x10 */
    CollisionShape world_shape; /* +0xA0 */
} PlayerCollisionNode; /* 0x130 */

struct PlayerCollisionData {
    struct MkObj* object; /* +0x0000 */
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
    int render_recorded; /* +0x9404 */
    float active_scale; /* +0x9408 */
    float attack_radius; /* +0x940C */
};

typedef char CollisionPaddedVecSizeCheck[sizeof(CollisionPaddedVec) == 0x10 ? 1 : -1];
typedef char CollisionSphereSizeCheck[sizeof(CollisionSphere) == 0x10 ? 1 : -1];
typedef char CollisionQuadSizeCheck[sizeof(CollisionQuad) == 0x40 ? 1 : -1];
typedef char CollisionShapeSizeCheck[sizeof(CollisionShape) == 0x90 ? 1 : -1];
typedef char PlayerCollisionNodeSizeCheck[sizeof(PlayerCollisionNode) == 0x130 ? 1 : -1];
typedef char PlayerCollisionDataSizeCheck[sizeof(PlayerCollisionData) == 0x9410 ? 1 : -1];

typedef void (*GlobalCollisionCallback)(const unsigned int* obstacle_id);

struct ConstrainInfo;

void generate_obstacles(int handle, char* name, MkPtr** obstacle_list);
CollisionObj* get_collision_obj(void);
float repel_check_plyrs(void);
int repel_against_global_collision_list(
    Vec* position, Vec* movement, Vec* result_position);
void repel_against_obstacle_list(
    PlyrInfo* player, Vec* previous_position, Vec* movement,
    Vec* position, struct ConstrainInfo* info);

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
int get_first_shape_center_for_obstacle_id(int obstacle_id, Vec* center);
int collide_segment_against_global_collision_list(
    Vec* start, Vec* end, Vec* hit_point,
    unsigned int ignored_flags);
int repel_point_against_global_collision_list_toward_target(
    Vec* target, Vec* start, Vec* result_point,
    unsigned int ignored_flags);
void update_collision_obj_pos(CollisionObj* object, const Vec* position);
void collision_obj_set_shape(
    CollisionObj* object, const CollisionShape* shape);
void init_player_collision(PlyrInfo* player);
int collide_cylinder_vs_plyr(
    PlyrInfo* player, Vec* center, const Vec* angles,
    float radius, float height);

#endif
