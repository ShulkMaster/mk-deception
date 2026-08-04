#ifndef MKD_GAME_COLLISION_H
#define MKD_GAME_COLLISION_H

#include "math/gxVect.h"
#include "runtime/mk_struct.h"

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

typedef struct CollisionObstacle {
    Vec center; /* +0x00 - cached by get_shape_center_for_collision_obstacle */
    char pad0C[4];
    CollisionShape shape; /* +0x10 */
} CollisionObstacle;

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

struct PlayerCollisionData {
    char pad00[0x18];
    void* nodes; /* +0x18 */
    char pad1C[0x3C];
    struct PlyrPdata* player; /* +0x58 */
    struct MkObj* object;     /* +0x5C */
    char pad60[0x9398];
    int attack_region_index; /* +0x93F8 */
    char pad93FC[0x10];
    float attack_radius; /* +0x940C */
};

typedef void (*GlobalCollisionCallback)(const unsigned int* obstacle_id);

void build_col_shape_vertical_cylinder(
    CollisionShape* shape, const Vec* center, float radius, float height);
void build_col_shape_vertical_box(
    CollisionShape* shape, const Vec* center, float width, float height,
    float depth, float angle);
int is_point_inside_shape(const CollisionShape* shape, const Vec* point);
int collide_sphere_and_box(
    const CollisionShape* sphere, const CollisionShape* box);
void get_center_for_shape(const CollisionShape* shape, Vec* center);
void update_collision_obj_pos(CollisionObj* object, const Vec* position);

#endif
