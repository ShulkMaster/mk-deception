#include "game/collision.h"
#include "game/constrain.h"
#include "game/game_info.h"

#include "runtime/mk_mem.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/asset.h"
#include "runtime/plyr_pdata.h"
#include "platform/display.h"

#include "math/mk_math.h"
#include "math/gxMath.h"
#include "math/gxQuat.h"

typedef union CollisionObjListRef {
    MkHdr* hdr;
    CollisionObjList* list;
} CollisionObjListRef;

typedef union CollisionObjRef {
    MkHdr* hdr;
    CollisionObj* object;
} CollisionObjRef;

typedef struct CdfCollisionGroup {
    int primitive_count;
    unsigned int obstacle_id;
} CdfCollisionGroup;

typedef struct CdfCollisionPrimitive {
    int vertex_count;
    Vec vertices[1];
} CdfCollisionPrimitive;

typedef struct CollisionIm3DVertex {
    Vec position;
    char pad0C[0x0C];
    unsigned int color;
    char pad1C[8];
} CollisionIm3DVertex; /* 0x24 */

typedef struct PlayerCollisionAnimView {
    char pad00[0x1BC];
    MkObj* object;
    unsigned int object_instance;
} PlayerCollisionAnimView;

typedef struct PlayerCollisionRepelView {
    char pad00[0x230];
    int obstacle_contact;
} PlayerCollisionRepelView;

typedef struct CollisionRepelInfo {
    int moving_shape;
    Vec* first_movement;
    Vec* second_movement;
    int preserve_first_contact;
} CollisionRepelInfo;

typedef struct PlayerCollisionRegion {
    CollisionShape shape;
    char pad90[0x10];
    Vec anchor;
    char padAC[0x84];
} PlayerCollisionRegion; /* 0x130 */

typedef struct CollisionNodeDef {
    int node_id;
    unsigned int side;
    float field_08;
    float radius_scale;
    float field_10;
} CollisionNodeDef; /* 0x14 */

typedef struct ObstacleCallbackData {
    unsigned int obstacle_id;
    int obstacle_type;
    Vec* movement;
    PlayerCollisionData* collision_data;
    unsigned char flags;
    char pad19[3];
} ObstacleCallbackData;

typedef struct CollisionObstacleVtable {
    int (*reserved[4])(void);
    void (*destroy)(ArenaObstacle* obstacle, void* vtbl);
} CollisionObstacleVtable;

typedef struct RwEngineInstanceView {
    char pad00[0x24];
    int (*render_state)(int state, void* value, void* engine);
} RwEngineInstanceView;

static const Vec UNITVECT_Y = {0.0f, 1.0f, 0.0f};
static const Vec UNITVECT_Z = {0.0f, 0.0f, 1.0f};
static const CollisionNodeDef collision_node_defs[28] = {
    {0x1001, 1, .35f, .225f, .225f},
    {0x1002, 2, .35f, .225f, .225f},
    {0x1004, 1, 0.0f, .188f, .16f},
    {0x1005, 2, 0.0f, .188f, .16f},
    {0x1006, 3, .35f, .225f, .225f},
    {0x1007, 1, .17f, .188f, .188f},
    {0x1008, 2, .17f, .188f, .188f},
    {0x100A, 1, 0.0f, .125f, .125f},
    {0x100B, 2, 0.0f, .125f, .125f},
    {0x100C, 1, 0.0f, .125f, .125f},
    {0x100E, 2, 0.0f, .125f, .125f},
    {0x100F, 1, 0.0f, .125f, .125f},
    {0x1011, 2, 0.0f, .125f, .125f},
    {0x1010, 3, 0.0f, .125f, .125f},
    {0x1012, 1, 0.0f, .125f, .125f},
    {0x1013, 2, 0.0f, .125f, .125f},
    {0x1014, 1, 0.0f, .125f, .1f},
    {0x1015, 2, 0.0f, .125f, .1f},
    {0x1016, 1, .12f, .125f, .125f},
    {0x1017, 2, .12f, .125f, .125f},
    {0x1018, 1, 0.0f, .125f, 0.0f},
    {0x1019, 2, 0.0f, .125f, 0.0f},
    {0x4044, 1, 0.0f, .125f, .125f},
    {0x4046, 1, 0.0f, .188f, .16f},
    {0x4048, 1, 0.0f, .125f, 0.0f},
    {0x4051, 1, 0.0f, .125f, .125f},
    {0x4053, 1, 0.0f, .188f, .16f},
    {0x4055, 1, 0.0f, .125f, 0.0f}
};
static const int attack_forearm_l[] = {0x14, 0x16, 0x18, 0};
static const int attack_forearm_r[] = {0x15, 0x17, 0x19, 0};
static const int attack_lowleg_l[] = {4, 7, 0};
static const int attack_lowleg_r[] = {5, 8, 0};
static const int attack_lowlegs[] = {5, 8, -1, 4, 7, 0};
static const int attack_arm_l[] = {0xF, 0x12, 0x14, 0x16, 0x18, 0};
static const int attack_arm_r[] = {0x11, 0x13, 0x15, 0x17, 0x19, 0};
static const int attack_leg_l[] = {1, 4, 7, 0xA, 0};
static const int attack_leg_r[] = {2, 5, 8, 0xB, 0};
static const int attack_arm_both[] = {
    0x11, 0x13, 0x15, 0x17, 0x19, -1,
    0xF, 0x12, 0x14, 0x16, 0x18, 0
};
static const int attack_armleg_r[] = {
    0x11, 0x13, 0x15, 0x17, 0x19, -1, 2, 5, 8, 0xB, 0
};
static const int attack_armleg_l[] = {
    0xF, 0x12, 0x14, 0x16, 0x18, -1, 1, 4, 7, 0xA, 0
};
static const int attack_leg_both[] = {
    1, 4, 7, 0xA, -1, 2, 5, 8, 0xB, 0
};
static const int attack_back[] = {1, 2, 0};
static const int attack_goro_arms[] = {
    0x44, 0x46, 0x48, -1, 0x51, 0x53, 0x55, 0
};
static const int* const attack_region_lists[16] = {
    0, attack_forearm_l, attack_forearm_r, attack_lowleg_l,
    attack_lowleg_r, attack_lowlegs, attack_arm_l, attack_arm_r,
    attack_leg_l, attack_leg_r, attack_arm_both, attack_armleg_r,
    attack_armleg_l, attack_leg_both, attack_back, attack_goro_arms
};
#define TEST_RAY_BOX_FACE(axis, plane, other_a, upper_a, lower_a, other_b, upper_b, lower_b) \
    do { \
        denominator = v3_dot_v3((axis), direction); \
        if (denominator != 0.0f) { \
            distance = -(v3_dot_v3((axis), origin) - (plane)) / denominator; \
        } else { \
            distance = -__float_max; \
        } \
        if (distance > 0.0f && \
            (nearest < 0.0f || distance < nearest)) { \
            parametric_ray_to_point(&point, origin, direction, distance); \
            projection = v3_dot_v3((other_a), &point); \
            if (projection < (upper_a) && projection > (lower_a)) { \
                projection = v3_dot_v3((other_b), &point); \
                if (projection < (upper_b) && projection > (lower_b)) { \
                    nearest = distance; \
                } \
            } \
        } \
    } while (0)
#define TEST_QUAD_EDGE(current, next) \
    do { \
        edge.x = (next)->x - (current)->x; \
        edge.y = (next)->y - (current)->y; \
        edge.z = (next)->z - (current)->z; \
        inward.x = edge.y * normal.z - edge.z * normal.y; \
        inward.y = edge.z * normal.x - edge.x * normal.z; \
        inward.z = edge.x * normal.y - edge.y * normal.x; \
        normalize_v3(&inward); \
        point_side = inward.x * point->x + inward.y * point->y + \
                     inward.z * point->z; \
        vertex_side = inward.x * (current)->x + inward.y * (current)->y + \
                      inward.z * (current)->z; \
        if (point_side > vertex_side) { \
            return 0; \
        } \
    } while (0)
static const unsigned short AtomicBBoxIndices[24] = {
    0, 1, 1, 3, 3, 2, 2, 0,
    4, 5, 5, 7, 7, 6, 6, 4,
    0, 4, 1, 5, 2, 6, 3, 7
};
static const unsigned short QuadVertexIndices[8] = {
    0, 1, 1, 2, 2, 3, 3, 0
};
static CollisionShape konquest_hero_collision_shape;
static MKMATRIX inv_cam_rot_mat;
static MkPtr* global_collision_list;
static MkPtr* konquest_shadow_collision_lists;
static GlobalCollisionCallback global_collision_callback;

static void update_player_collision_nodes(PlayerCollisionData* collision);
void render_danger_zone_collision_obj(MkHdr* object);
void render_disabled_collision_obj(MkHdr* object);
void render_collision_obj(MkHdr* object);
void render_bgnd_danger_zone_obstacle(ArenaObstacle* obstacle);
void render_obstacle(ArenaObstacle* obstacle);
void render_hero_collision(void);
static void render_konquest_shadow_objects(MkHdr* object);
static void render_konquest_collision_obj(MkHdr* object);
extern void render_background_danger_areas(void);
extern RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src);
extern RwMatrix* RwMatrixOrthoNormalize(
    RwMatrix* dst, const RwMatrix* src);
extern RwEngineInstanceView* RwEngineInstance;
extern int mode_of_play;
int collide_shape_vs_plyr(
    PlyrInfo* player, const CollisionShape* shape);
CollisionObj* convert_cdf_quad_to_collision_box(
    const Vec* vertices, const Vec* angles, const Vec* position);
CollisionObj* convert_cdf_triangle_to_collision_cylinder(
    const Vec* vertices, const Vec* angles, const Vec* position);
ArenaObstacle* get_obstacle(void);
int get_obstacle_type_from_id(unsigned int obstacle_id);
void insert_collision_on_proper_tile_list(CollisionObj* object);
int collide_sphere_and_box(
    const CollisionShape* sphere, const CollisionShape* box);

static int is_point_inside_quad(
    const CollisionShape* quad, const Vec* point);
static int collide_sphere_and_quad(
    const CollisionShape* sphere, const CollisionShape* quad);
static int test_collision(
    const CollisionShape* shape_a, const CollisionShape* shape_b);
static int test_collision_vs_obstacles(
    PlyrInfo* player, const CollisionShape* shape);
static void xz_unit_vector_to_shape(
    Vec* result, const CollisionShape* shape, const Vec* point);
static void add_plyr_body_attack_nodes(float radius);
static void generate_weapon_collision_nodes(
    PlayerCollisionData* collision, MkObj* weapon, float radius);
extern int local_collision_allowed(PlyrPdata* player);
float ray_intersection_with_shape(
    const CollisionShape* shape, const Vec* origin, const Vec* direction);
static float ray_intersection_with_quad(
    const Vec* origin, const Vec* direction, const CollisionShape* quad);
int repel_shape_against_obstacle_list(
    PlyrInfo* player, CollisionShape* shape, Vec* movement, Vec* position,
    ConstrainInfo* info, Vec* test_position, int step_index);
int repel_a_from_b(
    CollisionShape* shape, const CollisionShape* obstacle, Vec* movement);
static int repel_cylinder_and_box(
    CollisionShape* cylinder, const CollisionShape* box,
    CollisionRepelInfo* info, int side_test);
static int repel_cylinder_and_quad(
    CollisionShape* cylinder, const CollisionShape* quad,
    CollisionRepelInfo* info, int side_test);
static int repel_cylinders(
    CollisionShape* first, CollisionShape* second,
    CollisionRepelInfo* info, int side_test);
int repel_cylinder_against_global_collision_list(
    CollisionShape* cylinder, Vec* movement);
extern ConstrainInfo constrain_info;
extern int local_obstacle_callback(ArenaObstacle* obstacle);
static void reset_player_collision(PlayerCollisionData* collision);
static void render_player_joints(PlayerCollisionData* collision);
void render_col_shape(
    const CollisionShape* shape, const unsigned int* color);
static void build_col_shape_vertical_box_from_corners(
    CollisionShape* shape, const Vec* corner_0, const Vec* corner_1,
    const Vec* corner_2, const Vec* corner_3);
extern void render_col_shape_as_cylinder(
    const CollisionShape* shape, const unsigned int* color);
void render_col_shape_as_box(
    const CollisionShape* shape, const unsigned int* color);
void render_col_shape_as_quad(
    const CollisionShape* shape, const unsigned int* color);
static void set_collision_vertex(
    CollisionIm3DVertex* vertex, const Vec* position,
    unsigned int color);
extern unsigned int rgba_white;
extern unsigned int rgba_yellow;
extern unsigned int rgba_blue;
extern unsigned int rgba_red;
extern unsigned int rgba_green;
extern unsigned int rgba_cyan;
extern void* RwIm3DTransform(
    CollisionIm3DVertex* vertices, unsigned int count, void* matrix,
    unsigned int flags);
extern void RwIm3DRenderPrimitive(int primitive);
extern void RwIm3DRenderIndexedPrimitive(
    int primitive, const unsigned short* indices, int count);
extern void RwIm3DEnd(void);
extern float __float_max;

float repel_check_plyrs(void) {
    PlayerCollisionData* first_data;
    PlayerCollisionData* second_data;
    PlayerCollisionRegion* first_regions;
    PlayerCollisionRegion* second_regions;
    CollisionShape first_shape;
    CollisionShape second_shape;
    Vec difference;
    float first_scale;
    float second_scale;
    float penetration;
    float best;
    unsigned int first_count;
    unsigned int second_count;
    unsigned int first_index;
    unsigned int second_index;

    best = 0.0f;
    first_data = g_game_info.plyr0.collision_data;
    second_data = g_game_info.plyr1.collision_data;
    if (first_data == 0 || second_data == 0) {
        return 0.0f;
    }
    if (*(unsigned int*)((char*)second_data + 0x93F0) == 0) {
        return best;
    }

    first_count = *(unsigned int*)((char*)first_data + 0x9400);
    second_count = *(unsigned int*)((char*)second_data + 0x9400);
    first_scale = *(float*)((char*)first_data + 0x9408);
    second_scale = *(float*)((char*)second_data + 0x9408);
    first_regions =
        (PlayerCollisionRegion*)((char*)first_data + 0x8C40);
    second_regions =
        (PlayerCollisionRegion*)((char*)second_data + 0x8C40);

    for (first_index = 0; first_index < first_count; first_index++) {
        first_shape = first_regions[first_index].shape;
        first_shape.sphere_radius *= first_scale;
        for (second_index = 0;
             second_index < second_count;
             second_index++) {
            second_shape = second_regions[second_index].shape;
            second_shape.sphere_radius *= second_scale;
            if (test_collision(&first_shape, &second_shape) == 1) {
                v3_sub_v3(
                    &difference, &first_shape.sphere_center,
                    &second_shape.sphere_center);
                penetration = first_shape.sphere_radius +
                    second_shape.sphere_radius - length_v3(&difference);
                if (penetration > best) {
                    best = penetration;
                }
            }
        }
    }
    return best;
}

int collide_shape_vs_plyr(
    PlyrInfo* player, const CollisionShape* shape) {
    PlayerCollisionData* collision;
    PlayerCollisionRegion* regions;
    CollisionShape* recorded_shapes;
    unsigned int region_count;
    unsigned int region_index;
    int* recorded_index;
    int* reset_recorded;

    collision = player->collision_data;
    recorded_index = (int*)((char*)collision + 0x93FC);
    reset_recorded = (int*)((char*)collision + 0x9404);
    if ((g_game_info.pause_flags & 1) != 0 &&
        (((unsigned char*)&g_game_info)[2] & 0x20) == 0) {
        if (*reset_recorded != 0) {
            *recorded_index = 0;
            *reset_recorded = 0;
        }
        recorded_shapes =
            (CollisionShape*)((char*)collision + 0x7760);
        recorded_shapes[*recorded_index] = *shape;
        (*recorded_index)++;
        *reset_recorded = 0;
    }

    region_count = *(unsigned int*)((char*)collision + 0x93F0);
    regions = (PlayerCollisionRegion*)((char*)collision + 0x140);
    for (region_index = 0; region_index < region_count; region_index++) {
        if (test_collision(shape, &regions[region_index].shape) == 1) {
            return 1;
        }
    }
    return 0;
}
#pragma opt_unroll_loops off
int collide_plyr_vs_plyr(void) {
    PlyrInfo* player;
    PlayerCollisionData* collision;
    PlayerCollisionData* opponent;
    PlayerCollisionRegion* attacks;
    PlayerCollisionRegion* opponent_regions;
    PlayerCollisionRegion* source_regions;
    PlayerCollisionRegion* saved_regions;
    unsigned int attack_count;
    unsigned int opponent_count;
    unsigned int saved_count;
    unsigned int attack_index;
    unsigned int opponent_index;
    unsigned int saved_index;

    player = plyr_pdata->plyr_info;
    collision = player->collision_data;
    if (collision == 0) {
        return 0;
    }
    if (collision == g_game_info.plyr0.collision_data) {
        opponent = g_game_info.plyr1.collision_data;
    } else {
        opponent = g_game_info.plyr0.collision_data;
    }
    if (opponent == 0) {
        return 0;
    }

    opponent_count = *(unsigned int*)((char*)opponent + 0x93F0);
    attack_count = *(unsigned int*)((char*)collision + 0x93F4);
    attacks = (PlayerCollisionRegion*)((char*)collision + 0x2280);
    opponent_regions =
        (PlayerCollisionRegion*)((char*)opponent + 0x140);
    if (opponent_count != 0) {
        for (attack_index = 0;
             attack_index < attack_count;
             attack_index++) {
            test_collision_vs_obstacles(
                player, &attacks[attack_index].shape);
            if (local_collision_allowed(plyr_pdata) != 0) {
                for (opponent_index = 0;
                     opponent_index < opponent_count;
                     opponent_index++) {
                    if (test_collision(
                            &attacks[attack_index].shape,
                            &opponent_regions[opponent_index].shape) == 1) {
                        saved_count = *(unsigned int*)(
                            (char*)collision + 0x93F8);
                        source_regions = (PlayerCollisionRegion*)(
                            (char*)collision + 0x21E0);
                        saved_regions = (PlayerCollisionRegion*)(
                            (char*)collision + 0x4CA0);
                        for (saved_index = 0;
                             saved_index < saved_count;
                             saved_index++) {
                            saved_regions[saved_index] =
                                source_regions[saved_index];
                        }
                        *(unsigned int*)((char*)collision + 0x93F4) = 0;
                        return 1;
                    }
                }
            }
        }
    }

    saved_count = *(unsigned int*)((char*)collision + 0x93F8);
    source_regions =
        (PlayerCollisionRegion*)((char*)collision + 0x21E0);
    saved_regions =
        (PlayerCollisionRegion*)((char*)collision + 0x4CA0);
    for (saved_index = 0; saved_index < saved_count; saved_index++) {
        saved_regions[saved_index] = source_regions[saved_index];
    }
    *(unsigned int*)((char*)collision + 0x93F4) = 0;
    return 0;
}
#pragma opt_unroll_loops reset

static int test_collision_vs_obstacles(
    PlyrInfo* player, const CollisionShape* shape) {
    CollisionObjRef collision_object;
    ObstacleCallbackData callback_data;
    ArenaObstacle* obstacle;
    MkPtr* obstacle_item;
    MkPtr* shape_item;
    MkPtr* next;
    Vec direction;
    int result;

    result = 0;
    obstacle_item = constrain_info.obstacles;
    while (obstacle_item != 0) {
        obstacle = (ArenaObstacle*)obstacle_item->hdr;
        if (obstacle_item->instance != obstacle->hdr.instance) {
            next = obstacle_item->next;
            obstacle_item->hdr = 0;
            destroy_mkptr(obstacle_item);
            obstacle_item = next;
            continue;
        }
        if (!obstacle->flags.bits.disabled && &obstacle->shapes != 0) {
            shape_item = obstacle->shapes;
            while (shape_item != 0) {
                collision_object.hdr = shape_item->hdr;
                if (shape_item->instance !=
                    collision_object.hdr->instance) {
                    next = shape_item->next;
                    shape_item->hdr = 0;
                    destroy_mkptr(shape_item);
                    shape_item = next;
                    continue;
                }
                if (test_collision(
                        shape, &collision_object.object->shape) != 0) {
                    if (constrain_info.callback != 0 &&
                        local_obstacle_callback(obstacle) != 0) {
                        xz_unit_vector_to_shape(
                            &direction, &collision_object.object->shape,
                            (const Vec*)((const char*)player->slot.mirror_a +
                                         0xA0));
                        callback_data.obstacle_id = obstacle->obstacle_id;
                        callback_data.obstacle_type = obstacle->type;
                        callback_data.movement = &direction;
                        callback_data.collision_data = player->collision_data;
                        callback_data.flags = 0;
                        if (((int (*)(ObstacleCallbackData*))
                                 constrain_info.callback)(
                                &callback_data) != 0 &&
                            obstacle->hdr.instance != 0) {
                            ((CollisionObstacleVtable*)
                                 obstacle->hdr.vtbl)->destroy(
                                obstacle, obstacle->hdr.vtbl);
                        }
                    }
                    result = 1;
                    break;
                }
                shape_item = shape_item->next;
            }
        }
        obstacle_item = obstacle_item->next;
    }
    return result;
}

int get_first_shape_center_for_obstacle_id(
    unsigned int obstacle_id, Vec* center) {
    CollisionObjRef collision_object;
    ArenaObstacle* obstacle;
    MkPtr* obstacle_item;
    MkPtr* shape_item;
    MkPtr* next;

    obstacle_item = constrain_info.obstacles;
    while (obstacle_item != 0) {
        obstacle = (ArenaObstacle*)obstacle_item->hdr;
        if (obstacle_item->instance != obstacle->hdr.instance) {
            next = obstacle_item->next;
            obstacle_item->hdr = 0;
            destroy_mkptr(obstacle_item);
            obstacle_item = next;
            continue;
        }
        if (obstacle->obstacle_id == obstacle_id &&
            &obstacle->shapes != 0) {
            shape_item = obstacle->shapes;
            while (shape_item != 0) {
                collision_object.hdr = shape_item->hdr;
                if (shape_item->instance !=
                    collision_object.hdr->instance) {
                    next = shape_item->next;
                    shape_item->hdr = 0;
                    destroy_mkptr(shape_item);
                    shape_item = next;
                    continue;
                }
                get_center_for_shape(
                    &collision_object.object->shape, center);
                return 1;
            }
        }
        obstacle_item = obstacle_item->next;
    }
    return 0;
}

void insert_on_collision_obj_list(
    MkHdr* object, CollisionObjList* list) {
    mk_insert(object, &list->objects);
}

void set_collision_render_state(unsigned char enabled) {
    g_game_info.pause_flags =
        (g_game_info.pause_flags & ~1) | (enabled & 1);
}

void init_player_collision(PlayerCollisionData* collision) {
    collision->nodes = get_mem(0x9410);
    if (collision->nodes != 0) {
        reset_player_collision(collision);
    }
}

static void render_players_joints(void) {
    if (g_game_info.plyr0.collision_data != 0) {
        render_player_joints(g_game_info.plyr0.collision_data);
    }
    if (g_game_info.plyr1.collision_data != 0) {
        render_player_joints(g_game_info.plyr1.collision_data);
    }
}

void render_collision_regions(void) {
    union {
        MkHdr* hdr;
        CollisionObjList* list;
    } shadow_list;
    RwMatrix camera_matrix;
    MkPtr* item;
    MkPtr* next;
    int state_1;
    int state_6;
    int state_8;
    unsigned char render_flags;

    if ((g_game_info.pause_flags & 1) == 0) {
        return;
    }

    RwMatrixInvert(
        &camera_matrix,
        (const RwMatrix*)((const char*)Camera + 0x20));
    RwMatrixOrthoNormalize(&inv_cam_rot_mat, &camera_matrix);
    RwEngineInstance->render_state(1, &state_1, RwEngineInstance);
    RwEngineInstance->render_state(6, &state_6, RwEngineInstance);
    RwEngineInstance->render_state(8, &state_8, RwEngineInstance);
    set_render_state(1, 0);
    set_render_state(6, 0);
    set_render_state(8, 0);

    render_flags = ((unsigned char*)&g_game_info)[2];
    if ((render_flags & 0x10) != 0) {
        if (g_game_info.plyr0.collision_data != 0) {
            *(int*)((char*)g_game_info.plyr0.collision_data + 0x9404) = 1;
        }
        if (g_game_info.plyr1.collision_data != 0) {
            *(int*)((char*)g_game_info.plyr1.collision_data + 0x9404) = 1;
        }
        apply_to_mklist(
            (MkListApplyFn)render_bgnd_danger_zone_obstacle,
            &constrain_info.obstacles);
    } else {
        if ((render_flags & 0x20) == 0) {
            render_players_joints();
        }
        if ((render_flags & 0x80) == 0) {
            render_background_danger_areas();
            apply_to_mklist(
                (MkListApplyFn)render_obstacle,
                &constrain_info.obstacles);
        }
        if (mode_of_play == 7 && (render_flags & 0x40) == 0) {
            render_hero_collision();
            apply_to_mklist(
                render_konquest_collision_obj, &global_collision_list);
            item = konquest_shadow_collision_lists;
            while (item != 0) {
                shadow_list.hdr = item->hdr;
                if (item->instance != shadow_list.hdr->instance) {
                    next = item->next;
                    item->hdr = 0;
                    destroy_mkptr(item);
                    item = next;
                    continue;
                }
                apply_to_mklist(
                    render_konquest_shadow_objects,
                    &shadow_list.list->objects);
                item = item->next;
            }
        }
    }
    set_render_state(1, state_1);
    set_render_state(6, state_6);
    set_render_state(8, state_8);
}

static void render_player_joints(PlayerCollisionData* collision) {
    CollisionIm3DVertex vertices[4][16];
    PlayerCollisionRegion* joint_regions;
    PlayerCollisionRegion* active_regions;
    CollisionShape* recorded_shapes;
    CollisionShape shape;
    unsigned int joint_count;
    unsigned int active_count;
    unsigned int recorded_count;
    unsigned int index;
    int sphere_index;
    Vec radial;
    Vec transformed;
    float scale;
    float angle;

#define RENDER_PLAYER_SHAPE(render_shape, render_color, vertex_set) \
    do { \
        switch ((render_shape)->type & 7) { \
        case 1: \
            for (sphere_index = 0; sphere_index < 16; sphere_index++) { \
                angle = 3.1415927f * ((float)sphere_index / 7.5f); \
                radial.x = (render_shape)->sphere_radius * \
                    (float)gxMathCos(angle); \
                radial.y = (render_shape)->sphere_radius * \
                    gxMathSin(angle); \
                radial.z = 0.0f; \
                v3_x_mat_add_v3( \
                    &transformed, &radial, &inv_cam_rot_mat, \
                    &(render_shape)->sphere_center); \
                set_collision_vertex( \
                    &(vertex_set)[sphere_index], &transformed, \
                    *(render_color)); \
            } \
            if (RwIm3DTransform((vertex_set), 16, 0, 2) != 0) { \
                RwIm3DRenderPrimitive(2); \
                RwIm3DEnd(); \
            } \
            break; \
        case 2: \
            render_col_shape_as_cylinder((render_shape), (render_color)); \
            break; \
        case 3: \
            render_col_shape_as_box((render_shape), (render_color)); \
            break; \
        case 4: \
            render_col_shape_as_quad((render_shape), (render_color)); \
            break; \
        } \
    } while (0)

    joint_count = *(unsigned int*)((char*)collision + 0x93F0);
    joint_regions =
        (PlayerCollisionRegion*)((char*)collision + 0x140);
    for (index = 0; index < joint_count; index++) {
        RENDER_PLAYER_SHAPE(
            &joint_regions[index].shape, &rgba_yellow, vertices[0]);
    }

    active_count = *(unsigned int*)((char*)collision + 0x9400);
    active_regions =
        (PlayerCollisionRegion*)((char*)collision + 0x8C40);
    scale = *(float*)((char*)collision + 0x9408);
    for (index = 0; index < active_count; index++) {
        shape = active_regions[index].shape;
        shape.sphere_radius *= scale;
        RENDER_PLAYER_SHAPE(&shape, &rgba_blue, vertices[1]);
    }

    recorded_count = *(unsigned int*)((char*)collision + 0x93FC);
    if (recorded_count != 0) {
        *(int*)((char*)collision + 0x9404) = 1;
        recorded_shapes =
            (CollisionShape*)((char*)collision + 0x7760);
        for (index = 0; index < recorded_count; index++) {
            RENDER_PLAYER_SHAPE(
                &recorded_shapes[index], &rgba_red, vertices[2]);
        }
    }
    RENDER_PLAYER_SHAPE(
        (const CollisionShape*)((const char*)collision + 0x10),
        &rgba_green, vertices[3]);
#undef RENDER_PLAYER_SHAPE
}

static void render_konquest_shadow_objects(MkHdr* hdr) {
    CollisionIm3DVertex vertices[16];
    CollisionObj* object;
    CollisionShape* shape;
    Vec radial;
    Vec transformed;
    float angle;
    int index;

    object = (CollisionObj*)hdr;
    shape = &object->shape;
    switch (shape->type & 7) {
    case 1:
        for (index = 0; index < 16; index++) {
            angle = 3.1415927f * ((float)index / 7.5f);
            radial.x = shape->sphere_radius * (float)gxMathCos(angle);
            radial.y = shape->sphere_radius * gxMathSin(angle);
            radial.z = 0.0f;
            v3_x_mat_add_v3(
                &transformed, &radial, &inv_cam_rot_mat,
                &shape->sphere_center);
            set_collision_vertex(
                &vertices[index], &transformed, rgba_yellow);
        }
        if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
            RwIm3DRenderPrimitive(2);
            RwIm3DEnd();
        }
        break;
    case 2:
        render_col_shape_as_cylinder(shape, &rgba_yellow);
        break;
    case 3:
        render_col_shape_as_box(shape, &rgba_yellow);
        break;
    case 4:
        render_col_shape_as_quad(shape, &rgba_yellow);
        break;
    }
}

#define DEFINE_COLLISION_OBJECT_RENDERER(function_name, render_color) \
    void function_name(MkHdr* hdr) { \
        CollisionIm3DVertex vertices[16]; \
        CollisionObj* object; \
        CollisionShape* shape; \
        Vec radial; \
        Vec transformed; \
        float angle; \
        int index; \
        object = (CollisionObj*)hdr; \
        shape = &object->shape; \
        switch (shape->type & 7) { \
        case 1: \
            for (index = 0; index < 16; index++) { \
                angle = 3.1415927f * ((float)index / 7.5f); \
                radial.x = shape->sphere_radius * (float)gxMathCos(angle); \
                radial.y = shape->sphere_radius * gxMathSin(angle); \
                radial.z = 0.0f; \
                v3_x_mat_add_v3( \
                    &transformed, &radial, &inv_cam_rot_mat, \
                    &shape->sphere_center); \
                set_collision_vertex( \
                    &vertices[index], &transformed, (render_color)); \
            } \
            if (RwIm3DTransform(vertices, 16, 0, 2) != 0) { \
                RwIm3DRenderPrimitive(2); \
                RwIm3DEnd(); \
            } \
            break; \
        case 2: \
            render_col_shape_as_cylinder(shape, &(render_color)); \
            break; \
        case 3: \
            render_col_shape_as_box(shape, &(render_color)); \
            break; \
        case 4: \
            render_col_shape_as_quad(shape, &(render_color)); \
            break; \
        } \
    }

DEFINE_COLLISION_OBJECT_RENDERER(
    render_danger_zone_collision_obj, rgba_white)
DEFINE_COLLISION_OBJECT_RENDERER(
    render_disabled_collision_obj, rgba_cyan)
DEFINE_COLLISION_OBJECT_RENDERER(render_collision_obj, rgba_blue)
#undef DEFINE_COLLISION_OBJECT_RENDERER

static void render_konquest_collision_obj(MkHdr* hdr) {
    CollisionIm3DVertex vertices[3][16];
    CollisionObj* object;
    CollisionShape* shape;
    Vec radial;
    Vec transformed;
    float angle;
    int index;

    object = (CollisionObj*)hdr;
    shape = &object->shape;

#define RENDER_KONQUEST_SHAPE(render_color, vertex_set) \
    do { \
        switch (shape->type & 7) { \
        case 1: \
            for (index = 0; index < 16; index++) { \
                angle = 3.1415927f * ((float)index / 7.5f); \
                radial.x = shape->sphere_radius * (float)gxMathCos(angle); \
                radial.y = shape->sphere_radius * gxMathSin(angle); \
                radial.z = 0.0f; \
                v3_x_mat_add_v3( \
                    &transformed, &radial, &inv_cam_rot_mat, \
                    &shape->sphere_center); \
                set_collision_vertex( \
                    &(vertex_set)[index], &transformed, (render_color)); \
            } \
            if (RwIm3DTransform((vertex_set), 16, 0, 2) != 0) { \
                RwIm3DRenderPrimitive(2); \
                RwIm3DEnd(); \
            } \
            break; \
        case 2: \
            render_col_shape_as_cylinder(shape, &(render_color)); \
            break; \
        case 3: \
            render_col_shape_as_box(shape, &(render_color)); \
            break; \
        case 4: \
            render_col_shape_as_quad(shape, &(render_color)); \
            break; \
        } \
    } while (0)

    if ((shape->type & 7) == 2) {
        RENDER_KONQUEST_SHAPE(rgba_cyan, vertices[2]);
    } else if ((shape->type & 7) == 4) {
        RENDER_KONQUEST_SHAPE(rgba_green, vertices[1]);
    } else {
        RENDER_KONQUEST_SHAPE(rgba_blue, vertices[0]);
    }
#undef RENDER_KONQUEST_SHAPE
}

#pragma opt_unroll_loops off
void set_plyr_attack_region(int use_body, float radius) {
    PlayerCollisionData* collision;
    PlayerCollisionRegion* attacks;
    PlayerCollisionRegion* saved;
    CollisionShape* recorded;
    PlyrMirrorSlots* mirror_slots;
    MkObj* weapon_0;
    MkObj* weapon_1;
    Vec difference;
    unsigned int attack_count;
    unsigned int saved_count;
    unsigned int index;

    collision = plyr_pdata->plyr_info->collision_data;
    if ((g_game_info.pause_flags & 1) != 0 &&
        (((unsigned char*)&g_game_info)[2] & 0x20) == 0) {
        *(unsigned int*)((char*)collision + 0x9404) = 0;
        *(unsigned int*)((char*)collision + 0x93FC) = 0;
    }

    if (use_body != 0) {
        add_plyr_body_attack_nodes(radius);
    } else {
        mirror_slots = plyr_pdata->mirror_slots;
        if (mirror_slots != 0) {
            weapon_0 = mirror_slots->weapon[0].primary.obj;
            if (weapon_0 != 0 &&
                weapon_0->hdr.instance !=
                    mirror_slots->weapon[0].primary.instance) {
                weapon_0 = 0;
            }
            weapon_1 = mirror_slots->weapon[1].primary.obj;
            if (weapon_1 != 0 &&
                weapon_1->hdr.instance !=
                    mirror_slots->weapon[1].primary.instance) {
                weapon_1 = 0;
            }
            if (weapon_0 != 0) {
                generate_weapon_collision_nodes(
                    collision, weapon_0, radius);
            }
            if (weapon_1 != 0) {
                generate_weapon_collision_nodes(
                    collision, weapon_1, radius);
            }
        }
    }

    attacks = (PlayerCollisionRegion*)((char*)collision + 0x2280);
    saved = (PlayerCollisionRegion*)((char*)collision + 0x4D40);
    attack_count = *(unsigned int*)((char*)collision + 0x93F4);
    saved_count = *(unsigned int*)((char*)collision + 0x93F8);
    if (saved_count == 0) {
        for (index = 0; index < attack_count; index++) {
            saved[index] = attacks[index];
        }
    }
    *(unsigned int*)((char*)collision + 0x93F8) = attack_count;

    for (index = 0; index < attack_count; index++) {
        v3_sub_v3(
            &difference, &attacks[index].shape.sphere_center,
            &saved[index].shape.sphere_center);
        if (length_v3(&difference) >
            2.0f * attacks[index].shape.sphere_radius) {
            unsigned int new_index = *(unsigned int*)(
                (char*)collision + 0x93F4);
            if (new_index < 0x24) {
                PlayerCollisionRegion interpolated = saved[index];
                v3_x_v_add_v3(
                    &interpolated.shape.sphere_center,
                    &difference, 0.5f);
                interpolated.shape.type = 1;
                attacks[new_index] = interpolated;
                *(unsigned int*)((char*)collision + 0x93F4) =
                    new_index + 1;
                if ((g_game_info.pause_flags & 1) != 0 &&
                    (((unsigned char*)&g_game_info)[2] & 0x20) == 0) {
                    unsigned int record_index = *(unsigned int*)(
                        (char*)collision + 0x93FC);
                    if (record_index < 0x24) {
                        recorded = (CollisionShape*)(
                            (char*)collision + 0x7760);
                        recorded[record_index] = interpolated.shape;
                        *(unsigned int*)((char*)collision + 0x93FC) =
                            record_index + 1;
                    }
                }
            }
        }
    }
}
#pragma opt_unroll_loops reset

void purge_global_collision_list(void) {
    MkPtr* item;

    item = first_mkptr(&global_collision_list);
    while (item != 0) {
        item = next_mkptr(item);
    }
}

int is_point_inside_shadow_exclusion_zone(
    const Vec* point, float radius) {
    CollisionObjListRef list_object;
    CollisionObjRef collision_object;
    MkPtr* list_item;
    MkPtr* collision_item;
    MkPtr* next;
    float projection;
    float dx;
    float dz;
    float test_radius;
    int inside;

    if (&konquest_shadow_collision_lists == 0) {
        return 0;
    }

    list_item = konquest_shadow_collision_lists;
    while (list_item != 0) {
        list_object.hdr = list_item->hdr;
        if (list_item->instance != list_object.hdr->instance) {
            next = list_item->next;
            list_item->hdr = 0;
            destroy_mkptr(list_item);
            list_item = next;
            continue;
        }

        inside = 0;
        if (&list_object.list->objects != 0) {
            collision_item = list_object.list->objects;
            while (collision_item != 0) {
                collision_object.hdr = collision_item->hdr;
                if (collision_item->instance !=
                    collision_object.hdr->instance) {
                    next = collision_item->next;
                    collision_item->hdr = 0;
                    destroy_mkptr(collision_item);
                    collision_item = next;
                    continue;
                }

                switch (collision_object.object->shape.type & 7) {
                case 2:
                    dx = point->x -
                        collision_object.object->shape.cylinder_center.x;
                    dz = point->z -
                        collision_object.object->shape.cylinder_center.z;
                    test_radius =
                        collision_object.object->shape.cylinder_radius + radius;
                    inside = dx * dx + dz * dz < test_radius * test_radius;
                    break;
                case 3:
                    projection =
                        collision_object.object->shape.box_axis_2.x * point->x +
                        collision_object.object->shape.box_axis_2.y * point->y +
                        collision_object.object->shape.box_axis_2.z * point->z;
                    if (projection >=
                            collision_object.object->shape.box_axis_2_min + radius ||
                        projection <=
                            collision_object.object->shape.box_axis_2_max - radius) {
                        inside = 0;
                        break;
                    }
                    projection =
                        collision_object.object->shape.box_axis_1.x * point->x +
                        collision_object.object->shape.box_axis_1.y * point->y +
                        collision_object.object->shape.box_axis_1.z * point->z;
                    if (projection >=
                            collision_object.object->shape.box_axis_1_max + radius ||
                        projection <=
                            collision_object.object->shape.box_axis_1_min - radius) {
                        inside = 0;
                        break;
                    }
                    projection =
                        collision_object.object->shape.box_axis_0.x * point->x +
                        collision_object.object->shape.box_axis_0.y * point->y +
                        collision_object.object->shape.box_axis_0.z * point->z;
                    inside =
                        projection <
                            collision_object.object->shape.box_axis_0_min + radius &&
                        projection >
                            collision_object.object->shape.box_axis_0_max - radius;
                    break;
                default:
                    inside = 0;
                    break;
                }
                if (inside != 0) {
                    break;
                }
                collision_item = collision_item->next;
            }
        }
        if (inside != 0) {
            return 1;
        }
        list_item = list_item->next;
    }
    return 0;
}

static int repel_cylinder_and_box(
    CollisionShape* cylinder, const CollisionShape* box,
    CollisionRepelInfo* info, int side_test) {
    CollisionShape side;
    CollisionShape box_copy;
    Vec original_center;
    Vec retained_center;
    Vec top[4];
    Vec point;
    Vec* saved_movement;
    const Vec* corners[4];
    const Vec* axis;
    float extent;
    float center_x;
    float center_z;
    float dx;
    float dz;
    float distance;
    float nearest;
    float projection;
    float adjustment;
    int reverse;
    int retained;
    int result;
    int hit;
    int index;

    if (box->box_pad_7C > 0.0f) {
        center_x =
            0.5f * (box->box_corner_2.x - box->box_corner_1.x) +
            0.5f * (box->box_corner_0.x + box->box_corner_1.x);
        center_z =
            0.5f * (box->box_corner_2.z - box->box_corner_1.z) +
            0.5f * (box->box_corner_0.z + box->box_corner_1.z);
        dx = center_x - cylinder->cylinder_center.x;
        dz = center_z - cylinder->cylinder_center.z;
        distance = cylinder->cylinder_radius + box->box_pad_7C + 0.01f;
        if (dx * dx + dz * dz >= distance * distance) {
            return 0;
        }
    }
    if (info->moving_shape != 2) {
        return 1;
    }

    saved_movement = info->first_movement;
    point = *saved_movement;
    info->first_movement = &point;
    original_center = cylinder->cylinder_center;
    retained_center = original_center;
    retained = 0;
    result = 0;

    corners[0] = &box->box_corner_0;
    corners[1] = &box->box_corner_1;
    corners[2] = &box->box_corner_2;
    corners[3] = &box->box_corner_3;
    extent = box->box_axis_0_max - box->box_axis_0_min;
    for (index = 0; index < 4; index++) {
        top[index].x = corners[index]->x + box->box_axis_0.x * extent;
        top[index].y = corners[index]->y + box->box_axis_0.y * extent;
        top[index].z = corners[index]->z + box->box_axis_0.z * extent;
    }

#define TEST_BOX_SIDE(a, b) \
    do { \
        side.quad_vertex_0 = *corners[(a)]; \
        side.quad_vertex_1 = *corners[(b)]; \
        side.quad_vertex_2 = top[(b)]; \
        side.quad_vertex_3 = top[(a)]; \
        side.type = 4; \
        hit = repel_cylinder_and_quad(cylinder, &side, info, 1); \
        if (hit != 0) { \
            result = 1; \
            if (retained == 0) { \
                retained_center = cylinder->cylinder_center; \
                if (info->preserve_first_contact == 0) { \
                    retained = 1; \
                } \
            } \
            cylinder->cylinder_center = original_center; \
        } \
    } while (0)

    TEST_BOX_SIDE(1, 0);
    TEST_BOX_SIDE(0, 3);
    TEST_BOX_SIDE(3, 2);
    TEST_BOX_SIDE(2, 1);
#undef TEST_BOX_SIDE

    cylinder->cylinder_center = retained_center;
    box_copy = *box;
    box_copy.type = 3;
    point.x = cylinder->cylinder_center.x +
        UNITVECT_Y.x * (0.5f * cylinder->cylinder_height);
    point.y = cylinder->cylinder_center.y +
        UNITVECT_Y.y * (0.5f * cylinder->cylinder_height);
    point.z = cylinder->cylinder_center.z +
        UNITVECT_Y.z * (0.5f * cylinder->cylinder_height);

    if (is_point_inside_shape(&box_copy, &point)) {
        axis = &box_copy.box_axis_2;
        projection = axis->x * point.x + axis->y * point.y + axis->z * point.z;
        nearest = box_copy.box_axis_2_max - projection;
        adjustment = projection - box_copy.box_axis_2_min;
        reverse = 0;
        if (adjustment < nearest) {
            nearest = adjustment;
            reverse = 1;
        }

        projection = box_copy.box_axis_1.x * point.x +
            box_copy.box_axis_1.y * point.y + box_copy.box_axis_1.z * point.z;
        adjustment = box_copy.box_axis_1_max - projection;
        if (adjustment < nearest) {
            nearest = adjustment;
            axis = &box_copy.box_axis_1;
            reverse = 0;
        }
        adjustment = projection - box_copy.box_axis_1_min;
        if (adjustment < nearest) {
            nearest = adjustment;
            axis = &box_copy.box_axis_1;
            reverse = 1;
        }
        if (reverse != 0) {
            nearest = -nearest;
            adjustment = -0.01f;
        } else {
            adjustment = 0.01f;
        }
        cylinder->cylinder_center.x += axis->x * (nearest + adjustment);
        cylinder->cylinder_center.y += axis->y * (nearest + adjustment);
        cylinder->cylinder_center.z += axis->z * (nearest + adjustment);
        cylinder->cylinder_center.y = original_center.y;
        result = 1;
    }
    info->first_movement = saved_movement;
    return result;
}

static int repel_cylinder_and_quad(
    CollisionShape* cylinder, const CollisionShape* quad,
    CollisionRepelInfo* info, int side_test) {
    union {
        float value;
        unsigned int bits;
    } distance_bits, guess_bits;
    Vec edge_0;
    Vec edge_1;
    Vec normal;
    Vec tangent;
    Vec direction;
    Vec* movement;
    float plane_distance;
    float center_plane;
    float moved_plane;
    float tangent_min;
    float tangent_max;
    float height_min;
    float height_max;
    float projection;
    float plane_delta;
    float edge_delta;
    float distance;
    float penetration;
    float value;
    int index;

    (void)side_test;
    info->preserve_first_contact = 0;
    PSVECSubtract(&quad->quad_vertex_1, &quad->quad_vertex_0, &edge_0);
    PSVECSubtract(&quad->quad_vertex_3, &quad->quad_vertex_0, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    center_plane = normal.x * cylinder->cylinder_center.x +
        normal.y * cylinder->cylinder_center.y +
        normal.z * cylinder->cylinder_center.z;
    plane_distance = normal.x * quad->quad_vertex_0.x +
        normal.y * quad->quad_vertex_0.y +
        normal.z * quad->quad_vertex_0.z;
    if (plane_distance >= center_plane + cylinder->cylinder_radius ||
        plane_distance <= center_plane - cylinder->cylinder_radius) {
        return 0;
    }

    movement = info->first_movement;
    moved_plane = normal.x *
            (cylinder->cylinder_center.x - movement->x) +
        normal.y * (cylinder->cylinder_center.y - movement->y) +
        normal.z * (cylinder->cylinder_center.z - movement->z);
    if (plane_distance >= moved_plane + cylinder->cylinder_radius &&
        normal.x * movement->x + normal.z * movement->z <= 0.0f) {
        return 0;
    }

    PSVECCrossProduct(&normal, &UNITVECT_Y, &tangent);
    PSVECNormalize(&tangent, &tangent);
    tangent_min = tangent_max =
        tangent.x * quad->quad_vertex_0.x +
        tangent.y * quad->quad_vertex_0.y +
        tangent.z * quad->quad_vertex_0.z;
    height_min = height_max = quad->quad_vertex_0.y;
    for (index = 1; index < 4; index++) {
        const Vec* vertex = (const Vec*)(
            (const char*)&quad->quad_vertex_0 + index * 0x10);
        value = tangent.x * vertex->x + tangent.y * vertex->y +
            tangent.z * vertex->z;
        if (value > tangent_max) {
            tangent_max = value;
        }
        if (value < tangent_min) {
            tangent_min = value;
        }
        if (vertex->y > height_max) {
            height_max = vertex->y;
        }
        if (vertex->y < height_min) {
            height_min = vertex->y;
        }
    }
    if (cylinder->cylinder_center.y >= height_max ||
        cylinder->cylinder_center.y + cylinder->cylinder_height <=
            height_min) {
        return 0;
    }

    projection = tangent.x * cylinder->cylinder_center.x +
        tangent.y * cylinder->cylinder_center.y +
        tangent.z * cylinder->cylinder_center.z;
    if (tangent_max <= projection - cylinder->cylinder_radius ||
        tangent_min >= projection + cylinder->cylinder_radius) {
        return 0;
    }

    direction = normal;
    direction.y = 0.0f;
    normalize_xz(&direction);
    if (projection <= tangent_max && projection >= tangent_min) {
        penetration =
            plane_distance + cylinder->cylinder_radius - center_plane;
    } else {
        plane_delta = center_plane - plane_distance;
        edge_delta = projection > tangent_max ?
            projection - tangent_max : tangent_min - projection;
        distance_bits.value =
            plane_delta * plane_delta + edge_delta * edge_delta;
        distance = 0.0f;
        if (distance_bits.value > 0.0f) {
            guess_bits.bits =
                (*(unsigned short*)((char*)GXMathSqrtTable +
                  ((distance_bits.bits >> 10) & 0x3FFE)) << 8) |
                ((((distance_bits.bits & 0x7F800000) + 0x3F800000) >> 1) &
                 0x7F800000);
            distance = 0.5f * guess_bits.value *
                (3.0f - (guess_bits.value * guess_bits.value) /
                 distance_bits.value);
        }
        if (distance >= cylinder->cylinder_radius) {
            return 0;
        }
        penetration = cylinder->cylinder_radius - distance;
    }

    if (direction.x * movement->x + direction.z * movement->z > 0.0f) {
        info->preserve_first_contact = 1;
    }
    xz_x_v_add_xz(
        &cylinder->cylinder_center, &direction, penetration + 0.001f);
    return 1;
}

static int repel_cylinders(
    CollisionShape* first, CollisionShape* second,
    CollisionRepelInfo* info, int side_test) {
    union {
        float value;
        unsigned int bits;
    } distance_bits, guess_bits;
    CollisionShape* fixed;
    CollisionShape* moving;
    Vec* movement;
    Vec direction;
    Vec start;
    float dx;
    float dz;
    float radius;
    float distance;
    float overlap;

    (void)side_test;
    dx = first->cylinder_center.x - second->cylinder_center.x;
    dz = first->cylinder_center.z - second->cylinder_center.z;
    radius = first->cylinder_radius + second->cylinder_radius;
    if (dx * dx + dz * dz >= radius * radius) {
        return 0;
    }
    if (first->cylinder_center.y + first->cylinder_height <=
            second->cylinder_center.y ||
        second->cylinder_center.y + second->cylinder_height <=
            first->cylinder_center.y) {
        return 0;
    }

    switch (info->moving_shape) {
    case 1:
        movement = info->second_movement;
        fixed = first;
        moving = second;
        break;
    case 2:
        movement = info->first_movement;
        fixed = second;
        moving = first;
        break;
    default:
        return 1;
    }

    direction = *movement;
    normalize_xz(&direction);
    start.x = moving->cylinder_center.x - 1.01f * movement->x;
    start.y = moving->cylinder_center.y - 1.01f * movement->y;
    start.z = moving->cylinder_center.z - 1.01f * movement->z;

    dx = moving->cylinder_center.x - fixed->cylinder_center.x;
    dz = moving->cylinder_center.z - fixed->cylinder_center.z;
    distance_bits.value = dx * dx + dz * dz;
    distance = 0.0f;
    if (distance_bits.value > 0.0f) {
        guess_bits.bits =
            (*(unsigned short*)((char*)GXMathSqrtTable +
              ((distance_bits.bits >> 10) & 0x3FFE)) << 8) |
            ((((distance_bits.bits & 0x7F800000) + 0x3F800000) >> 1) &
             0x7F800000);
        distance = 0.5f * guess_bits.value *
            (3.0f - (guess_bits.value * guess_bits.value) /
             distance_bits.value);
    }
    if (distance == 0.0f) {
        dx = -direction.x;
        dz = -direction.z;
        if (dx == 0.0f && dz == 0.0f) {
            dz = radius;
        } else {
            dx *= radius;
            dz *= radius;
        }
    } else {
        overlap = radius - distance;
        dx *= overlap / distance;
        dz *= overlap / distance;
    }
    moving->cylinder_center.x += dx;
    moving->cylinder_center.z += dz;
    return 1;
}

static void update_all_collision_flags(
    MkPtr** list, unsigned int flags, int toggle) {
    MkPtr* item;
    MkPtr* next;
    CollisionObjRef object;

    if (list == 0) {
        return;
    }
    item = *list;
    while (item != 0) {
        next = item->next;
        object.hdr = item->hdr;
        if (object.hdr == 0 || item->instance != object.hdr->instance) {
            item->hdr = 0;
            destroy_mkptr(item);
        } else if (toggle != 0) {
            object.object->flags ^= flags;
        } else {
            object.object->flags |= flags;
        }
        item = next;
    }
}

void set_flag_for_all_collisions(MkPtr** list, unsigned int flags) {
    update_all_collision_flags(list, flags, 0);
}

void exclusive_or_flags_for_all_collisions(
    MkPtr** list, unsigned int flags) {
    update_all_collision_flags(list, flags, 1);
}

void remove_collision_list_from_konquest_shadow_lists(MkHdr* collision_list) {
    mk_pull_discard(collision_list, &konquest_shadow_collision_lists);
}

void insert_collision_list_on_konquest_shadow_lists(MkHdr* collision_list) {
    mk_insert_no_own(collision_list, &konquest_shadow_collision_lists);
}

void set_global_collision_callback(GlobalCollisionCallback callback) {
    global_collision_callback = callback;
}

void collision_obj_set_shape(CollisionObj* object, const CollisionShape* shape) {
    object->shape = *shape;
}

void build_col_shape_vertical_cylinder(
    CollisionShape* shape,
    const Vec* center,
    float radius,
    float height) {
    shape->type = 2;
    shape->cylinder_radius = radius;
    shape->cylinder_axis = UNITVECT_Y;
    shape->cylinder_height = height;

    if (center != 0) {
        shape->cylinder_center = *center;
    } else {
        shape->cylinder_center.x = 0.0f;
        shape->cylinder_center.y = 0.0f;
        shape->cylinder_center.z = 0.0f;
    }
}

void build_col_shape_vertical_box(
    CollisionShape* shape,
    const Vec* center,
    float width,
    float height,
    float depth,
    float angle) {
    float sin_angle;
    float cos_angle;

    if (shape == 0) {
        return;
    }

    sin_angle = gxMathSin(angle);
    cos_angle = (float)gxMathCos(angle);
    shape->type = 3;
    shape->box_pad_7C = 0.0f;
    shape->box_axis_0 = UNITVECT_Y;
    shape->box_axis_1.x = -cos_angle;
    shape->box_axis_1.y = 0.0f;
    shape->box_axis_1.z = sin_angle;
    shape->box_axis_2.x = sin_angle;
    shape->box_axis_2.y = 0.0f;
    shape->box_axis_2.z = cos_angle;
    shape->box_axis_2_min = 0.5f * depth;
    shape->box_axis_2_max = -0.5f * depth;
    shape->box_axis_0_min = height;
    shape->box_axis_0_max = 0.0f;
    shape->box_axis_1_min = -0.5f * width;
    shape->box_axis_1_max = 0.5f * width;

    if (center != 0) {
        float axis_1_center;
        float axis_2_center;

        axis_2_center =
            center->x * shape->box_axis_2.x +
            center->z * shape->box_axis_2.z;
        shape->box_axis_2_min += axis_2_center;
        shape->box_axis_2_max += axis_2_center;
        shape->box_axis_0_min += center->y;
        shape->box_axis_0_max += center->y;
        axis_1_center =
            center->x * shape->box_axis_1.x +
            center->z * shape->box_axis_1.z;
        shape->box_axis_1_min += axis_1_center;
        shape->box_axis_1_max += axis_1_center;
    }

    shape->box_corner_0.x =
        shape->box_axis_2.x * shape->box_axis_2_min +
        shape->box_axis_0.x * shape->box_axis_0_min +
        shape->box_axis_1.x * shape->box_axis_1_max;
    shape->box_corner_0.y =
        shape->box_axis_2.y * shape->box_axis_2_min +
        shape->box_axis_0.y * shape->box_axis_0_min +
        shape->box_axis_1.y * shape->box_axis_1_max;
    shape->box_corner_0.z =
        shape->box_axis_2.z * shape->box_axis_2_min +
        shape->box_axis_0.z * shape->box_axis_0_min +
        shape->box_axis_1.z * shape->box_axis_1_max;
    shape->box_corner_1.x =
        shape->box_axis_2.x * shape->box_axis_2_min +
        shape->box_axis_0.x * shape->box_axis_0_min +
        shape->box_axis_1.x * shape->box_axis_1_min;
    shape->box_corner_1.y =
        shape->box_axis_2.y * shape->box_axis_2_min +
        shape->box_axis_0.y * shape->box_axis_0_min +
        shape->box_axis_1.y * shape->box_axis_1_min;
    shape->box_corner_1.z =
        shape->box_axis_2.z * shape->box_axis_2_min +
        shape->box_axis_0.z * shape->box_axis_0_min +
        shape->box_axis_1.z * shape->box_axis_1_min;
    shape->box_corner_2.x =
        shape->box_axis_2.x * shape->box_axis_2_max +
        shape->box_axis_0.x * shape->box_axis_0_min +
        shape->box_axis_1.x * shape->box_axis_1_min;
    shape->box_corner_2.y =
        shape->box_axis_2.y * shape->box_axis_2_max +
        shape->box_axis_0.y * shape->box_axis_0_min +
        shape->box_axis_1.y * shape->box_axis_1_min;
    shape->box_corner_2.z =
        shape->box_axis_2.z * shape->box_axis_2_max +
        shape->box_axis_0.z * shape->box_axis_0_min +
        shape->box_axis_1.z * shape->box_axis_1_min;
    shape->box_corner_3.x =
        shape->box_axis_2.x * shape->box_axis_2_max +
        shape->box_axis_0.x * shape->box_axis_0_min +
        shape->box_axis_1.x * shape->box_axis_1_max;
    shape->box_corner_3.y =
        shape->box_axis_2.y * shape->box_axis_2_max +
        shape->box_axis_0.y * shape->box_axis_0_min +
        shape->box_axis_1.y * shape->box_axis_1_max;
    shape->box_corner_3.z =
        shape->box_axis_2.z * shape->box_axis_2_max +
        shape->box_axis_0.z * shape->box_axis_0_min +
        shape->box_axis_1.z * shape->box_axis_1_max;
}

int is_point_inside_shape(
    const CollisionShape* shape, const Vec* point) {
    float projection;

    if ((shape->type & 7) == 2) {
        float dx;
        float dz;

        dx = point->x - shape->cylinder_center.x;
        dz = point->z - shape->cylinder_center.z;
        return dx * dx + dz * dz <
               shape->cylinder_radius * shape->cylinder_radius;
    }
    if ((shape->type & 7) != 3) {
        return 0;
    }

    projection = v3_dot_v3(&shape->box_axis_2, point);
    if (projection > shape->box_axis_2_min ||
        projection < shape->box_axis_2_max) {
        return 0;
    }
    projection = v3_dot_v3(&shape->box_axis_1, point);
    if (projection > shape->box_axis_1_max ||
        projection < shape->box_axis_1_min) {
        return 0;
    }
    projection = v3_dot_v3(&shape->box_axis_0, point);
    return projection <= shape->box_axis_0_min &&
           projection >= shape->box_axis_0_max;
}

int collide_sphere_and_box(
    const CollisionShape* sphere, const CollisionShape* box) {
    float projection;
    float radius;

    if ((box->type & 7) == 2) {
        float dx;
        float dz;

        dx = sphere->sphere_center.x - box->cylinder_center.x;
        dz = sphere->sphere_center.z - box->cylinder_center.z;
        return dx * dx + dz * dz <
               box->cylinder_radius * box->cylinder_radius;
    }
    if ((box->type & 7) != 3) {
        return 0;
    }

    radius = sphere->sphere_radius;
    projection = v3_dot_v3(&box->box_axis_2, &sphere->sphere_center);
    if (projection > box->box_axis_2_min + radius ||
        projection < box->box_axis_2_max - radius) {
        return 0;
    }
    projection = v3_dot_v3(&box->box_axis_1, &sphere->sphere_center);
    if (projection > box->box_axis_1_max + radius ||
        projection < box->box_axis_1_min - radius) {
        return 0;
    }
    projection = v3_dot_v3(&box->box_axis_0, &sphere->sphere_center);
    return projection <= box->box_axis_0_min + radius &&
           projection >= box->box_axis_0_max - radius;
}

int collide_sphere_vs_plyr(
    PlyrInfo* player,
    const Vec* center,
    float radius) {
    CollisionShape shape;

    shape.type = 1;
    shape.sphere_radius = radius;
    if (center != 0) {
        shape.sphere_center = *center;
    } else {
        shape.sphere_center.x = 0.0f;
        shape.sphere_center.y = 0.0f;
        shape.sphere_center.z = 0.0f;
    }
    return collide_shape_vs_plyr(player, &shape);
}

int collide_cylinder_vs_plyr(
    PlyrInfo* player, const Vec* center, const Vec* angles,
    float radius, float height) {
    CollisionShape shape;

    shape.type = 2;
    shape.cylinder_radius = radius;
    shape.cylinder_height = height;
    if (center != 0) {
        shape.cylinder_center = *center;
    } else {
        shape.cylinder_center.x = 0.0f;
        shape.cylinder_center.y = 0.0f;
        shape.cylinder_center.z = 0.0f;
    }
    if (angles != 0) {
        uv_from_angles_xy(
            &shape.cylinder_axis, angles->x, angles->y);
    } else {
        shape.cylinder_axis = UNITVECT_Z;
    }
    return collide_shape_vs_plyr(player, &shape);
}

void destroy_konquest_shadow_collision_lists(void) {
    destroy_list(&konquest_shadow_collision_lists);
}

CollisionObjList* get_collision_obj_list(void) {
    CollisionObjListRef result;

    result.hdr = get_mkhdr_generic(sizeof(CollisionObjList));
    if (result.hdr != 0) {
        result.list->objects = 0;
    }
    return result.list;
}

CollisionObj* get_collision_obj(void) {
    CollisionObjRef result;

    result.hdr = get_mkhdr_generic(sizeof(CollisionObj));
    if (result.hdr != 0) {
        result.object->flags = 0;
    }
    return result.object;
}

CollisionObj* add_shape_to_global_collision_list(
    const CollisionShape* shape,
    unsigned int obstacle_id) {
    CollisionObjRef result;

    result.hdr = get_mkhdr_generic(sizeof(CollisionObj));
    if (result.hdr != 0) {
        result.object->flags = 0;
    }

    if (result.hdr != 0) {
        result.object->shape = *shape;
        result.object->obstacle_id = obstacle_id;
        mk_insert(result.hdr, &global_collision_list);
        return result.object;
    }
    return 0;
}

void generate_shadow_collision_objects(int handle, unsigned int art_oid) {
    int* cdf;
    unsigned char* cursor;
    int group_count;
    int group_index;

    cdf = (int*)get_cdf_data(handle, art_oid);
    group_count = *cdf;
    cursor = (unsigned char*)(cdf + 1);

    for (group_index = 0; group_index < group_count; group_index++) {
        CdfCollisionGroup* group;
        int primitive_index;

        group = (CdfCollisionGroup*)cursor;
        cursor += sizeof(*group);
        for (primitive_index = 0;
             primitive_index < group->primitive_count;
             primitive_index++) {
            CdfCollisionPrimitive* primitive;
            CollisionObj* object;

            primitive = (CdfCollisionPrimitive*)cursor;
            cursor = (unsigned char*)primitive->vertices;
            object = 0;
            if (primitive->vertex_count == 4) {
                object = convert_cdf_quad_to_collision_box(
                    primitive->vertices, 0, 0);
            }
            if (object != 0) {
                object->obstacle_id = group->obstacle_id;
                insert_collision_on_proper_tile_list(object);
            }
            cursor = (unsigned char*)&primitive->vertices[
                primitive->vertex_count];
        }
    }
}

int segment_against_obstacle_list(
    const Vec* start, const Vec* end, Vec* hit_point, MkPtr** obstacle_list) {
    CollisionObjRef collision;
    ArenaObstacle* obstacle;
    MkPtr* obstacle_item;
    MkPtr* collision_item;
    MkPtr* next;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max;
    if (start == 0 || end == 0 || hit_point == 0) {
        return 0;
    }

    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (obstacle_list != 0) {
        obstacle_item = *obstacle_list;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                next = obstacle_item->next;
                obstacle_item->hdr = 0;
                destroy_mkptr(obstacle_item);
                obstacle_item = next;
                continue;
            }

            if ((obstacle->flags.value & 0x60) == 0 &&
                &obstacle->shapes != 0) {
                collision_item = obstacle->shapes;
                while (collision_item != 0) {
                    collision.hdr = collision_item->hdr;
                    if (collision_item->instance != collision.hdr->instance) {
                        next = collision_item->next;
                        collision_item->hdr = 0;
                        destroy_mkptr(collision_item);
                        collision_item = next;
                        continue;
                    }
                    if ((collision.object->flags & 0x10000) == 0) {
                        distance = ray_intersection_with_shape(
                            &collision.object->shape, start, &direction);
                        if (distance > 0.0f &&
                            (distance < closest || closest < 0.0f)) {
                            closest = distance;
                        }
                    }
                    collision_item = collision_item->next;
                }
            }
            obstacle_item = obstacle_item->next;
        }
    }

    if (closest > 0.0f && closest < segment_length) {
        parametric_ray_to_point(hit_point, start, &direction, closest);
        return 1;
    }
    return 0;
}

void generate_obstacles(int handle, char* name, MkPtr** obstacle_list) {
    CdfCollisionGroup* group;
    CdfCollisionPrimitive* primitive;
    ArenaObstacle* obstacle;
    CollisionObj* collision;
    unsigned char* cursor;
    int* cdf;
    int group_count;
    int group_index;
    int primitive_index;
    Vec vertices[4];

    cdf = (int*)load_named_cdf_data_from_slot(handle, name);
    if (cdf == 0) {
        return;
    }

    group_count = *cdf;
    cursor = (unsigned char*)(cdf + 1);
    for (group_index = 0; group_index < group_count; group_index++) {
        group = (CdfCollisionGroup*)cursor;
        cursor += sizeof(*group);
        obstacle = get_obstacle();
        obstacle->obstacle_id = group->obstacle_id;
        mk_insert((MkHdr*)obstacle, obstacle_list);
        obstacle->type = get_obstacle_type_from_id(obstacle->obstacle_id);

        for (primitive_index = 0;
             primitive_index < group->primitive_count;
             primitive_index++) {
            primitive = (CdfCollisionPrimitive*)cursor;
            cursor = (unsigned char*)primitive->vertices;
            switch (obstacle->type) {
            case 1:
            case 3:
            case 4:
                if (primitive->vertex_count == 4) {
                    vertices[0] = primitive->vertices[0];
                    vertices[1] = primitive->vertices[1];
                    vertices[2] = primitive->vertices[2];
                    vertices[3] = primitive->vertices[3];
                    collision = get_collision_obj();
                    if (collision != 0) {
                        collision->shape.type = 4;
                        collision->shape.quad_vertex_0 = vertices[0];
                        collision->shape.quad_vertex_1 = vertices[1];
                        collision->shape.quad_vertex_2 = vertices[2];
                        collision->shape.quad_vertex_3 = vertices[3];
                    }
                    if (collision != 0) {
                        mk_insert((MkHdr*)collision, &obstacle->shapes);
                    }
                }
                break;
            case 5:
            case 6:
                if (primitive->vertex_count == 3) {
                    collision = convert_cdf_triangle_to_collision_cylinder(
                        primitive->vertices, 0, 0);
                    mk_insert((MkHdr*)collision, &obstacle->shapes);
                } else if (primitive->vertex_count == 4) {
                    collision = convert_cdf_quad_to_collision_box(
                        primitive->vertices, 0, 0);
                    mk_insert((MkHdr*)collision, &obstacle->shapes);
                }
                break;
            }
            cursor += primitive->vertex_count * sizeof(Vec);
        }
    }
}

void repel_against_obstacle_list(
    PlyrInfo* player, const Vec* previous_position, const Vec* movement,
    Vec* position, ConstrainInfo* info) {
    union {
        float value;
        unsigned int bits;
    } length_bits, guess_bits;
    PlayerCollisionAnimView* collision_data;
    CollisionShape shape;
    MkObj* object;
    Vec original_position;
    Vec step;
    Vec candidate;
    Vec test_position;
    float length;
    float guess;
    float fraction;
    int step_count;
    int step_index;

    original_position = *position;
    length_bits.value = movement->x * movement->x + movement->z * movement->z;
    length = 0.0f;
    if (length_bits.value > 0.0f) {
        guess_bits.bits =
            (*(unsigned short*)((char*)GXMathSqrtTable +
              ((length_bits.bits >> 10) & 0x3FFE)) << 8) |
            ((((length_bits.bits & 0x7F800000) + 0x3F800000) >> 1) &
             0x7F800000);
        guess = guess_bits.value;
        length = 0.5f * guess *
            (3.0f - (guess * guess) /
             (movement->x * movement->x + movement->z * movement->z));
    }

    step_count = (int)(length / 0.27f) + 1;
    for (step_index = 0; step_index < step_count; step_index++) {
        fraction = (float)(step_index + 1) / (float)step_count;
        step.x = movement->x * fraction;
        step.y = movement->y * fraction;
        step.z = movement->z * fraction;
        candidate.x = previous_position->x + step.x;
        candidate.y = previous_position->y + step.y;
        candidate.z = previous_position->z + step.z;

        shape.type = 2;
        shape.cylinder_radius = 0.3f;
        shape.cylinder_axis = UNITVECT_Y;
        shape.cylinder_height = 2.5f;
        test_position = candidate;
        test_position.y -= 1.0f;
        shape.cylinder_center = test_position;

        if (repel_shape_against_obstacle_list(
                player, &shape, &step, &candidate, info,
                &shape.cylinder_center, step_index) != 0) {
            position->x = candidate.x;
            position->z = candidate.z;
            break;
        }
    }

    collision_data = (PlayerCollisionAnimView*)player->collision_data;
    object = collision_data->object;
    if (object != 0 && object->hdr.instance !=
            collision_data->object_instance) {
        object = 0;
    }
    if (object != 0) {
        object->pos.x += position->x - original_position.x;
        object->pos.y += position->y - original_position.y;
        object->pos.z += position->z - original_position.z;
    }
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int repel_shape_against_obstacle_list(
    PlyrInfo* player, CollisionShape* shape, Vec* movement, Vec* position,
    ConstrainInfo* info, Vec* test_position, int step_index) {
    PlayerCollisionRepelView* collision_data;
    ObstacleCallbackData callback_data;
    ArenaObstacle* obstacle;
    CollisionObjRef collision;
    MkPtr* obstacle_item;
    MkPtr* collision_item;
    MkPtr* next;
    Vec pushes[20];
    Vec original;
    Vec total;
    int collision_count;
    int collided;
    int result;
    int index;

    original = *test_position;
    result = 0;
    collision_count = 0;
    if (info != 0) {
        obstacle_item = info->obstacles;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                next = obstacle_item->next;
                obstacle_item->hdr = 0;
                destroy_mkptr(obstacle_item);
                obstacle_item = next;
                continue;
            }
            if (step_index == 0) {
                obstacle->flags.value &= (unsigned char)~8;
            }
            if ((obstacle->flags.value & 0x40) == 0 &&
                &obstacle->shapes != 0) {
                collision_item = obstacle->shapes;
                while (collision_item != 0) {
                    collision.hdr = collision_item->hdr;
                    if (collision_item->instance != collision.hdr->instance) {
                        next = collision_item->next;
                        collision_item->hdr = 0;
                        destroy_mkptr(collision_item);
                        collision_item = next;
                        continue;
                    }

                    collided = 0;
                    switch (collision.object->shape.type & 7) {
                    case 2:
                    case 3:
                    case 4:
                        if (repel_a_from_b(
                                shape, &collision.object->shape,
                                movement) != 0) {
                            collided = 1;
                        }
                        break;
                    }
                    if ((obstacle->flags.value & 0x10) != 0) {
                        if (collided != 0) {
                            *test_position = original;
                        }
                        collided = collided == 0;
                    }

                    if (collided != 0) {
                        if (constrain_info.callback != 0 &&
                            local_obstacle_callback(obstacle) != 0) {
                            callback_data.obstacle_type = obstacle->type;
                            callback_data.flags = 0x80;
                            callback_data.obstacle_id = obstacle->obstacle_id;
                            callback_data.movement = movement;
                            movement->y = 0.0f;
                            callback_data.collision_data = player->collision_data;
                            if ((obstacle->flags.value & 8) == 0) {
                                if (((int (*)(ObstacleCallbackData*))
                                         constrain_info.callback)(
                                        &callback_data) != 0) {
                                    if (obstacle->hdr.instance != 0) {
                                        ((CollisionObstacleVtable*)
                                             obstacle->hdr.vtbl)->destroy(
                                            obstacle, obstacle->hdr.vtbl);
                                    }
                                    return result;
                                }
                                obstacle->flags.value |= 8;
                            }
                        }
                        if ((obstacle->flags.value & 0x80) == 0) {
                            collision_data = (PlayerCollisionRepelView*)
                                player->collision_data;
                            collision_data->obstacle_contact = 1;
                            if (collision_count < 20) {
                                pushes[collision_count].x =
                                    test_position->x - original.x;
                                pushes[collision_count].y =
                                    test_position->y - original.y;
                                pushes[collision_count].z =
                                    test_position->z - original.z;
                                collision_count++;
                            }
                        }
                        *test_position = original;
                    }
                    collision_item = collision_item->next;
                }
            }
            obstacle_item = obstacle_item->next;
        }
    }

    if (collision_count != 0) {
        *test_position = original;
        total = pushes[0];
        for (index = 1; index < collision_count; index++) {
            total.x += pushes[index].x;
            total.y += pushes[index].y;
            total.z += pushes[index].z;
        }
        result = 1;
        test_position->x += total.x;
        test_position->y += total.y;
        test_position->z += total.z;
    }
    if (result != 0) {
        position->x = test_position->x;
        position->z = test_position->z;
    }
    return result;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
CollisionObj* convert_cdf_quad_to_collision_box(
    const Vec* source, const Vec* angles, const Vec* position) {
    union {
        float value;
        unsigned int bits;
    } distance_bits, guess_bits;
    CollisionObj* collision;
    Vec corners[4];
    Vec input;
    MKMATRIX matrix;
    float center_x;
    float center_z;
    float half_height;
    float dx;
    float dz;
    float distance;
    int index;

    corners[0].x = source[2].x;
    corners[0].y = 0.0f;
    corners[0].z = source[2].z;
    corners[1] = source[1];
    if (corners[1].y <= 0.0f) {
        corners[1].y = 0.001f;
    }
    corners[2] = source[0];
    if (corners[2].y <= 0.0f) {
        corners[2].y = 0.001f;
    }
    corners[3] = source[3];
    if (corners[3].y <= 0.0f) {
        corners[3].y = 0.001f;
    }
    if (corners[1].y < 0.0f || corners[2].y < 0.0f) {
        corners[0].y = corners[1].y - 3.0f;
    }

    if (angles != 0) {
        YXZ_angles_to_MKMATRIX(angles, &matrix);
        for (index = 0; index < 4; index++) {
            input = corners[index];
            v3_x_mat(&corners[index], &input, &matrix);
        }
    }
    if (position != 0) {
        for (index = 0; index < 4; index++) {
            corners[index].x += position->x;
            corners[index].y += position->y;
            corners[index].z += position->z;
        }
    }

    collision = get_collision_obj();
    if (collision != 0) {
        build_col_shape_vertical_box_from_corners(
            &collision->shape, &corners[0], &corners[1],
            &corners[2], &corners[3]);
        if ((int)collision->shape.type == 3) {
            half_height = 0.5f *
                (collision->shape.box_axis_0_max -
                 collision->shape.box_axis_0_min);
            center_x =
                collision->shape.box_axis_0.x * half_height +
                0.5f * (collision->shape.box_corner_2.x -
                        collision->shape.box_corner_1.x) +
                0.5f * (collision->shape.box_corner_0.x +
                        collision->shape.box_corner_1.x);
            center_z =
                collision->shape.box_axis_0.z * half_height +
                0.5f * (collision->shape.box_corner_2.z -
                        collision->shape.box_corner_1.z) +
                0.5f * (collision->shape.box_corner_0.z +
                        collision->shape.box_corner_1.z);
            for (index = 0; index < 4; index++) {
                dx = center_x - corners[index].x;
                dz = center_z - corners[index].z;
                distance_bits.value = dx * dx + dz * dz;
                distance = 0.0f;
                if (distance_bits.value > 0.0f) {
                    guess_bits.bits =
                        (*(unsigned short*)((char*)GXMathSqrtTable +
                          ((distance_bits.bits >> 10) & 0x3FFE)) << 8) |
                        ((((distance_bits.bits & 0x7F800000) +
                            0x3F800000) >> 1) & 0x7F800000);
                    distance = 0.5f * guess_bits.value *
                        (3.0f -
                         (guess_bits.value * guess_bits.value) /
                         distance_bits.value);
                }
                if (distance < collision->shape.box_pad_7C) {
                    collision->shape.box_pad_7C = distance;
                }
            }
        }
    }
    return collision;
}

CollisionObj* convert_cdf_triangle_to_collision_cylinder(
    const Vec* vertices, const Vec* angles, const Vec* position) {
    union {
        float value;
        unsigned int bits;
    } length_bits, guess_bits;
    CollisionObj* collision;
    MKMATRIX matrix;
    Vec input;
    Vec center;
    float dx;
    float dz;
    float radius;
    float height;
    int top_index;
    int next_index;
    int other_index;

    top_index = 0;
    if (vertices[1].y > vertices[0].y) {
        top_index = 1;
    }
    if (vertices[2].y > vertices[top_index].y) {
        top_index = 2;
    }

    next_index = (top_index + 1) % 3;
    other_index = (top_index + 2) % 3;
    dx = vertices[next_index].x - vertices[top_index].x;
    dz = vertices[next_index].z - vertices[top_index].z;
    if (dx * dx + dz * dz >=
        (vertices[other_index].x - vertices[top_index].x) *
            (vertices[other_index].x - vertices[top_index].x) +
        (vertices[other_index].z - vertices[top_index].z) *
            (vertices[other_index].z - vertices[top_index].z)) {
        next_index = other_index;
    }

    dx = vertices[(top_index + 1) % 3].x -
         vertices[(top_index + 2) % 3].x;
    dz = vertices[(top_index + 1) % 3].z -
         vertices[(top_index + 2) % 3].z;
    length_bits.value = dx * dx + dz * dz;
    height = vertices[top_index].y - vertices[(top_index + 1) % 3].y;
    radius = 0.0f;
    if (length_bits.value > 0.0f) {
        guess_bits.bits =
            (*(unsigned short*)((char*)GXMathSqrtTable +
              ((length_bits.bits >> 10) & 0x3FFE)) << 8) |
            ((((length_bits.bits & 0x7F800000) + 0x3F800000) >> 1) &
             0x7F800000);
        radius = 0.5f * guess_bits.value *
            (3.0f - (guess_bits.value * guess_bits.value) /
             length_bits.value);
    }

    center = vertices[next_index];
    if (angles != 0) {
        YXZ_angles_to_MKMATRIX(angles, &matrix);
        input = center;
        v3_x_mat(&center, &input, &matrix);
    }
    if (position != 0) {
        center.x += position->x;
        center.y += position->y;
        center.z += position->z;
    }

    collision = get_collision_obj();
    if (collision != 0) {
        collision->shape.type = 2;
        collision->shape.cylinder_radius = radius;
        collision->shape.cylinder_axis = UNITVECT_Y;
        collision->shape.cylinder_height = height;
        collision->shape.cylinder_center = center;
    }
    return collision;
}

int repel_a_from_b(
    CollisionShape* shape, const CollisionShape* obstacle, Vec* movement) {
    CollisionRepelInfo info;
    int shape_type;
    int obstacle_type;

    obstacle_type = obstacle->type & 7;
    shape_type = shape->type & 7;
    if (obstacle_type == 1) {
        return 0;
    }

    switch (shape_type) {
    case 2:
        info.moving_shape = 2;
        info.first_movement = movement;
        info.second_movement = 0;
        switch (obstacle_type) {
        case 2:
            return repel_cylinders(
                shape, (CollisionShape*)obstacle, &info, 0);
        case 4:
            return repel_cylinder_and_quad(shape, obstacle, &info, 0);
        case 3:
            return repel_cylinder_and_box(shape, obstacle, &info, 0);
        }
        break;
    case 3:
        if (obstacle_type == 2) {
            info.moving_shape = 1;
            info.first_movement = 0;
            info.second_movement = movement;
            return repel_cylinder_and_box(
                (CollisionShape*)obstacle, shape, &info, 0);
        }
        break;
    case 4:
        if (obstacle_type == 2) {
            info.moving_shape = 1;
            info.first_movement = 0;
            info.second_movement = movement;
            return repel_cylinder_and_quad(
                (CollisionShape*)obstacle, shape, &info, 0);
        }
        break;
    }
    return 0;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

float ray_intersection_with_shape(
    const CollisionShape* shape, const Vec* origin, const Vec* direction) {
    union {
        float value;
        unsigned int bits;
    } root_input, root_guess;
    Vec point;
    Vec perpendicular;
    float nearest;
    float denominator;
    float distance;
    float projection;
    float side_distance;
    float along_distance;
    float root;

    switch (shape->type & 7) {
    case 3:
        nearest = -__float_max;
        TEST_RAY_BOX_FACE(
            &shape->box_axis_2, shape->box_axis_2_min,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max);
        TEST_RAY_BOX_FACE(
            &shape->box_axis_2, shape->box_axis_2_max,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max);
        TEST_RAY_BOX_FACE(
            &shape->box_axis_1, shape->box_axis_1_min,
            &shape->box_axis_2, shape->box_axis_2_min,
            shape->box_axis_2_max, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max);
        TEST_RAY_BOX_FACE(
            &shape->box_axis_1, shape->box_axis_1_max,
            &shape->box_axis_2, shape->box_axis_2_min,
            shape->box_axis_2_max, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max);
        TEST_RAY_BOX_FACE(
            &shape->box_axis_0, shape->box_axis_0_min,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_2,
            shape->box_axis_2_min, shape->box_axis_2_max);
        TEST_RAY_BOX_FACE(
            &shape->box_axis_0, shape->box_axis_0_max,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_2,
            shape->box_axis_2_min, shape->box_axis_2_max);
        return nearest;
    case 2:
        perpendicular.x = direction->z;
        perpendicular.y = 0.0f;
        perpendicular.z = -direction->x;
        normalize_xz(&perpendicular);
        side_distance =
            (origin->x - shape->cylinder_center.x) * perpendicular.x +
            (origin->z - shape->cylinder_center.z) * perpendicular.z;
        if (side_distance < 0.0f) {
            projection = -side_distance;
        } else {
            projection = side_distance;
        }
        if (projection >= shape->cylinder_radius) {
            return -__float_max;
        }
        along_distance = -(
            perpendicular.x * (origin->z - shape->cylinder_center.z) +
            perpendicular.z * -(origin->x - shape->cylinder_center.x));
        if (along_distance <= 0.0f) {
            return -__float_max;
        }
        root_input.value =
            shape->cylinder_radius * shape->cylinder_radius -
            side_distance * side_distance;
        root = 0.0f;
        if (root_input.value > 0.0f) {
            root_guess.bits =
                (*(unsigned short*)((char*)GXMathSqrtTable +
                  ((root_input.bits >> 10) & 0x3FFE)) << 8) |
                ((((root_input.bits & 0x7F800000) + 0x3F800000) >> 1) &
                 0x7F800000);
            root = 0.5f * root_guess.value *
                (3.0f - (root_guess.value * root_guess.value) /
                 root_input.value);
        }
        distance = along_distance - root;
        if (distance <= 0.0f) {
            return -__float_max;
        }
        return distance;
    case 4:
        return ray_intersection_with_quad(origin, direction, shape);
    default:
        return -__float_max;
    }
}

int collide_segment_against_global_collision_list_quads(
    const Vec* start, const Vec* end, Vec* hit_point) {
    CollisionObjRef collision;
    MkPtr* item;
    MkPtr* next;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max;
    if (start == 0 || end == 0 || hit_point == 0) {
        return 0;
    }
    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision.hdr = item->hdr;
            if (item->instance != collision.hdr->instance) {
                next = item->next;
                item->hdr = 0;
                destroy_mkptr(item);
                item = next;
                continue;
            }
            if ((collision.object->shape.type & 7) == 4) {
                distance = ray_intersection_with_shape(
                    &collision.object->shape, start, &direction);
                if (distance > 0.0f &&
                    (distance < closest || closest < 0.0f)) {
                    closest = distance;
                }
            }
            item = item->next;
        }
    }
    if (closest > 0.0f && closest < segment_length) {
        parametric_ray_to_point(hit_point, start, &direction, closest);
        return 1;
    }
    return 0;
}

int repel_point_against_global_collision_list_toward_target(
    const Vec* target, const Vec* start, Vec* result_point,
    unsigned int ignored_flags) {
    CollisionObjRef collision;
    MkPtr* item;
    MkPtr* next;
    Vec direction;
    float segment_length;
    float closest;
    float distance;
    float projection;
    float dx;
    float dz;
    int inside;

    closest = -__float_max;
    if (target == 0 || start == 0 || result_point == 0) {
        return 0;
    }
    segment_length = uv_v3_to_v3_dist(&direction, start, target);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision.hdr = item->hdr;
            if (item->instance != collision.hdr->instance) {
                next = item->next;
                item->hdr = 0;
                destroy_mkptr(item);
                item = next;
                continue;
            }
            if ((collision.object->flags & ignored_flags) == 0 &&
                (collision.object->shape.type & 7) != 4) {
                inside = 0;
                switch (collision.object->shape.type & 7) {
                case 2:
                    dx = target->x -
                        collision.object->shape.cylinder_center.x;
                    dz = target->z -
                        collision.object->shape.cylinder_center.z;
                    inside = dx * dx + dz * dz <
                        collision.object->shape.cylinder_radius *
                        collision.object->shape.cylinder_radius;
                    break;
                case 3:
                    projection =
                        collision.object->shape.box_axis_2.x * target->x +
                        collision.object->shape.box_axis_2.y * target->y +
                        collision.object->shape.box_axis_2.z * target->z;
                    if (projection >=
                            collision.object->shape.box_axis_2_min ||
                        projection <=
                            collision.object->shape.box_axis_2_max) {
                        break;
                    }
                    projection =
                        collision.object->shape.box_axis_1.x * target->x +
                        collision.object->shape.box_axis_1.y * target->y +
                        collision.object->shape.box_axis_1.z * target->z;
                    if (projection >=
                            collision.object->shape.box_axis_1_max ||
                        projection <=
                            collision.object->shape.box_axis_1_min) {
                        break;
                    }
                    projection =
                        collision.object->shape.box_axis_0.x * target->x +
                        collision.object->shape.box_axis_0.y * target->y +
                        collision.object->shape.box_axis_0.z * target->z;
                    inside =
                        projection < collision.object->shape.box_axis_0_min &&
                        projection > collision.object->shape.box_axis_0_max;
                    break;
                }
                if (inside != 0) {
                    distance = ray_intersection_with_shape(
                        &collision.object->shape, start, &direction);
                    if (distance > 0.0f &&
                        (distance < closest || closest < 0.0f)) {
                        closest = distance;
                    }
                }
            }
            item = item->next;
        }
    }
    if (closest > 0.0f && closest < segment_length) {
        parametric_ray_to_point(result_point, start, &direction, closest);
        return 1;
    }
    return 0;
}

int collide_segment_against_global_collision_list(
    const Vec* start, const Vec* end, Vec* hit_point,
    unsigned int ignored_flags) {
    CollisionObjRef collision;
    MkPtr* item;
    MkPtr* next;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max;
    if (start == 0 || end == 0 || hit_point == 0) {
        return 0;
    }
    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision.hdr = item->hdr;
            if (item->instance != collision.hdr->instance) {
                next = item->next;
                item->hdr = 0;
                destroy_mkptr(item);
                item = next;
                continue;
            }
            if ((collision.object->shape.type & 7) != 4 &&
                (collision.object->flags & ignored_flags) == 0) {
                distance = ray_intersection_with_shape(
                    &collision.object->shape, start, &direction);
                if (distance > 0.0f &&
                    (distance < closest || closest < 0.0f)) {
                    closest = distance;
                }
            }
            item = item->next;
        }
    }
    if (closest > 0.0f && closest < segment_length) {
        parametric_ray_to_point(hit_point, start, &direction, closest);
        return 1;
    }
    return 0;
}

int npc_repel_against_global_collision_list(
    const Vec* position, Vec* movement, Vec* result_position,
    unsigned int ignored_flags) {
    CollisionObjRef collision;
    CollisionShape shape;
    MkPtr* item;
    MkPtr* next;
    Vec reverse_direction;
    int collided;
    int result;

    result = 0;
    shape.type = 2;
    shape.cylinder_radius = 0.35f;
    shape.cylinder_axis = UNITVECT_Y;
    shape.cylinder_height = 2.4f;
    shape.cylinder_center.x = position->x + movement->x;
    shape.cylinder_center.y = position->y + movement->y - 1.2f;
    shape.cylinder_center.z = position->z + movement->z;
    result_position->x = position->x + movement->x;
    result_position->y = position->y + movement->y;
    result_position->z = position->z + movement->z;
    reverse_direction.x = -movement->x;
    reverse_direction.y = -movement->y;
    reverse_direction.z = -movement->z;
    normalize_v3(&reverse_direction);

    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision.hdr = item->hdr;
            if (item->instance != collision.hdr->instance) {
                next = item->next;
                item->hdr = 0;
                destroy_mkptr(item);
                item = next;
                continue;
            }
            collided = 0;
            if ((collision.object->shape.type & 7) != 4 &&
                (collision.object->flags & ignored_flags) == 0) {
                if ((collision.object->shape.type & 7) == 2) {
                    if (repel_a_from_b(
                            &shape, &collision.object->shape, movement) != 0) {
                        collided = 1;
                        result_position->x = shape.cylinder_center.x;
                        result_position->z = shape.cylinder_center.z;
                    }
                } else if ((collision.object->shape.type & 7) == 3 &&
                           repel_a_from_b(
                               &shape, &collision.object->shape,
                               movement) != 0) {
                    collided = 1;
                    result_position->x = shape.cylinder_center.x;
                    result_position->z = shape.cylinder_center.z;
                }
                if (collided == 1) {
                    result = 1;
                }
            }
            item = item->next;
        }
    }
    return result;
}

int repel_against_global_collision_list(
    const Vec* position, Vec* movement, Vec* result_position) {
    CollisionShape shape;
    Vec reverse_direction;
    int result;

    result = 0;
    shape.type = 2;
    shape.cylinder_radius = 0.35f;
    shape.cylinder_axis = UNITVECT_Y;
    shape.cylinder_height = 2.4f;
    shape.cylinder_center.x = position->x + movement->x;
    shape.cylinder_center.y = position->y + movement->y - 1.0f;
    shape.cylinder_center.z = position->z + movement->z;
    result_position->x = position->x + movement->x;
    result_position->y = position->y + movement->y;
    result_position->z = position->z + movement->z;
    reverse_direction.x = -movement->x;
    reverse_direction.y = -movement->y;
    reverse_direction.z = -movement->z;
    normalize_v3(&reverse_direction);

    if (repel_cylinder_against_global_collision_list(
            &shape, movement) != 0) {
        result = 1;
        result_position->x = shape.cylinder_center.x;
        result_position->z = shape.cylinder_center.z;
    }
    konquest_hero_collision_shape = shape;
    if (result > 1) {
        *result_position = *position;
        return 1;
    }
    return result > 0;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int repel_cylinder_against_global_collision_list(
    CollisionShape* cylinder, Vec* movement) {
    CollisionObjRef collision;
    MkPtr* item;
    MkPtr* next;
    Vec pushes[20];
    Vec original_center;
    Vec pass_center;
    Vec total;
    int collision_count;
    int collided;
    int retry;
    int pass;
    int result;
    int index;

    result = 0;
    retry = 1;
    pass = 0;
    original_center.x = cylinder->cylinder_center.x - movement->x;
    original_center.y = cylinder->cylinder_center.y - movement->y;
    original_center.z = cylinder->cylinder_center.z - movement->z;

    while (retry == 1) {
        pass_center = cylinder->cylinder_center;
        collision_count = 0;
        retry = 0;
        pass++;
        if (&global_collision_list != 0) {
            item = global_collision_list;
            while (item != 0) {
                collision.hdr = item->hdr;
                if (item->instance != collision.hdr->instance) {
                    next = item->next;
                    item->hdr = 0;
                    destroy_mkptr(item);
                    item = next;
                    continue;
                }
                collided = 0;
                if ((collision.object->shape.type & 7) == 2) {
                    if (repel_a_from_b(
                            cylinder, &collision.object->shape,
                            movement) != 0) {
                        collided = 1;
                    }
                } else if ((collision.object->shape.type & 7) == 3 &&
                           repel_a_from_b(
                               cylinder, &collision.object->shape,
                               movement) != 0) {
                    collided = 1;
                }
                if (collided == 1) {
                    if (collision_count < 20) {
                        pushes[collision_count].x =
                            cylinder->cylinder_center.x - pass_center.x;
                        pushes[collision_count].y =
                            cylinder->cylinder_center.y - pass_center.y;
                        pushes[collision_count].z =
                            cylinder->cylinder_center.z - pass_center.z;
                        collision_count++;
                    }
                    cylinder->cylinder_center = pass_center;
                    if (global_collision_callback != 0) {
                        global_collision_callback(
                            &collision.object->obstacle_id);
                    }
                }
                item = item->next;
            }
        }
        if (collision_count != 0) {
            total = pushes[0];
            if (collision_count == 1 && pass < 5) {
                retry = 1;
            } else {
                for (index = 1; index < collision_count; index++) {
                    total.x += pushes[index].x;
                    total.y += pushes[index].y;
                    total.z += pushes[index].z;
                    if (pass < 5) {
                        retry = 1;
                    } else {
                        cylinder->cylinder_center = original_center;
                        total.x = 0.0f;
                        total.y = 0.0f;
                        total.z = 0.0f;
                    }
                }
            }
            result = 1;
            cylinder->cylinder_center.x += total.x;
            cylinder->cylinder_center.y += total.y;
            cylinder->cylinder_center.z += total.z;
        }
    }
    return result != 0;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

int get_shape_center_for_collision_obstacle(CollisionObstacle* obstacle) {
    if (obstacle == 0) {
        return 0;
    }

    get_center_for_shape(&obstacle->shape, &obstacle->center);
    return 1;
}

void render_bgnd_danger_zone_obstacle(ArenaObstacle* obstacle) {
    if (!obstacle->flags.bits.danger_zone) {
        return;
    }

    if (obstacle->flags.bits.disabled) {
        apply_to_mklist(render_disabled_collision_obj, &obstacle->shapes);
    } else {
        apply_to_mklist(render_danger_zone_collision_obj, &obstacle->shapes);
    }
}

void render_obstacle(ArenaObstacle* obstacle) {
    if (obstacle->flags.bits.disabled) {
        apply_to_mklist(render_disabled_collision_obj, &obstacle->shapes);
    } else if (obstacle->flags.bits.danger_zone) {
        apply_to_mklist(render_danger_zone_collision_obj, &obstacle->shapes);
    } else {
        apply_to_mklist(render_collision_obj, &obstacle->shapes);
    }
}

void start_plyr_attack(float radius) {
    PlayerCollisionData* collision;

    collision = plyr_pdata->plyr_info->collision_data;
    collision->attack_radius = radius;
    collision->attack_region_index = 0;
}

void term_player_collision(PlyrInfo* player) {
    PlayerCollisionData* collision;

    collision = player->collision_data;
    if (collision != 0) {
        free_mem(collision);
        player->collision_data = 0;
    }
}

void term_collision_system(void) {
    destroy_mkprocs_pid(0x4001);
}

static float p_collision_update(void);
static void update_players_collision_nodes(void);

void init_collision_system(void) {
    int flags;
    MkProc* proc;

    flags = 0;
    proc = get_mkproc_nostack(&flags);
    global_collision_list = 0;
    konquest_shadow_collision_lists = 0;
    global_collision_callback = 0;
    create_mkproc(0x19, proc, 0x4001, p_collision_update, 0);
}

static float p_collision_update(void) {
    update_players_collision_nodes();
    return 1.0f;
}

static void update_players_collision_nodes(void) {
    if (g_game_info.plyr0.collision_data != 0) {
        update_player_collision_nodes(g_game_info.plyr0.collision_data);
    }
    if (g_game_info.plyr1.collision_data != 0) {
        update_player_collision_nodes(g_game_info.plyr1.collision_data);
    }
}

void update_collision_obj_pos(CollisionObj* object, const Vec* position) {
    if ((object->shape.type & 7) == 1) {
        object->shape.sphere_center = *position;
    }
}

void get_center_for_shape(const CollisionShape* shape, Vec* center) {
    float half_height;

    switch (shape->type & 7) {
    case 1:
        *center = shape->sphere_center;
        break;
    case 2:
        *center = shape->cylinder_center;
        v3_x_v_add_v3(
            center, &shape->cylinder_axis, 0.5f * shape->cylinder_height);
        break;
    case 3:
        half_height =
            0.5f * (shape->box_axis_0_max - shape->box_axis_0_min);
        center->x =
            shape->box_axis_0.x * half_height +
            0.5f * (shape->box_corner_2.x - shape->box_corner_1.x) +
            0.5f * (shape->box_corner_0.x + shape->box_corner_1.x);
        center->y =
            shape->box_axis_0.y * half_height +
            0.5f * (shape->box_corner_2.y - shape->box_corner_1.y) +
            0.5f * (shape->box_corner_0.y + shape->box_corner_1.y);
        center->z =
            shape->box_axis_0.z * half_height +
            0.5f * (shape->box_corner_2.z - shape->box_corner_1.z) +
            0.5f * (shape->box_corner_0.z + shape->box_corner_1.z);
        break;
    case 4:
        center->x =
            0.25f * (shape->quad_vertex_0.x + shape->quad_vertex_1.x +
                      shape->quad_vertex_2.x + shape->quad_vertex_3.x);
        center->y =
            0.25f * (shape->quad_vertex_0.y + shape->quad_vertex_1.y +
                      shape->quad_vertex_2.y + shape->quad_vertex_3.y);
        center->z =
            0.25f * (shape->quad_vertex_0.z + shape->quad_vertex_1.z +
                      shape->quad_vertex_2.z + shape->quad_vertex_3.z);
        break;
    }
}

static void build_col_shape_vertical_box_from_corners(
    CollisionShape* shape,
    const Vec* corner_0,
    const Vec* corner_1,
    const Vec* corner_2,
    const Vec* corner_3) {
    if (shape == 0 || corner_0 == 0 || corner_1 == 0 ||
        corner_2 == 0 || corner_3 == 0) {
        return;
    }

    shape->type = 3;
    shape->box_pad_7C = 0.0f;
    shape->box_axis_0 = UNITVECT_Y;

    shape->box_axis_2.x = corner_2->z - corner_1->z;
    shape->box_axis_2.y = 0.0f;
    shape->box_axis_2.z = -(corner_2->x - corner_1->x);
    normalize_xz(&shape->box_axis_2);
    shape->box_axis_2_min =
        xz_dot_xz(corner_2, &shape->box_axis_2);
    shape->box_axis_2_max =
        xz_dot_xz(corner_0, &shape->box_axis_2);

    shape->box_axis_1.x = corner_0->z - corner_1->z;
    shape->box_axis_1.y = 0.0f;
    shape->box_axis_1.z = -(corner_0->x - corner_1->x);
    normalize_xz(&shape->box_axis_1);
    shape->box_axis_1_min =
        xz_dot_xz(corner_1, &shape->box_axis_1);
    shape->box_axis_1_max =
        xz_dot_xz(corner_2, &shape->box_axis_1);

    shape->box_axis_0_min = corner_2->y;
    shape->box_axis_0_max = corner_0->y;
    shape->box_corner_0 = *corner_2;
    shape->box_corner_1 = *corner_1;
    shape->box_corner_2.x = corner_0->x;
    shape->box_corner_2.y = corner_3->y;
    shape->box_corner_2.z = corner_0->z;
    shape->box_corner_3 = *corner_3;
}

static int is_point_inside_quad(
    const CollisionShape* quad, const Vec* point) {
    Vec edge_0;
    Vec edge_1;
    Vec normal;
    Vec edge;
    Vec inward;
    float point_side;
    float vertex_side;

    PSVECSubtract(
        &quad->quad_vertex_1, &quad->quad_vertex_0, &edge_0);
    PSVECSubtract(
        &quad->quad_vertex_3, &quad->quad_vertex_0, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    TEST_QUAD_EDGE(&quad->quad_vertex_0, &quad->quad_vertex_1);
    TEST_QUAD_EDGE(&quad->quad_vertex_1, &quad->quad_vertex_2);
    TEST_QUAD_EDGE(&quad->quad_vertex_2, &quad->quad_vertex_3);
    TEST_QUAD_EDGE(&quad->quad_vertex_3, &quad->quad_vertex_0);
    return 1;
}

#pragma dont_inline on
static float ray_intersection_with_quad(
    const Vec* origin,
    const Vec* direction,
    const CollisionShape* quad) {
    Vec edge_0;
    Vec edge_1;
    Vec normal;
    Vec point;
    float denominator;
    float distance;

    PSVECSubtract(&quad->quad_vertex_1, &quad->quad_vertex_0, &edge_0);
    PSVECSubtract(&quad->quad_vertex_3, &quad->quad_vertex_0, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    denominator = normal.x * direction->x +
                  normal.y * direction->y +
                  normal.z * direction->z;
    if (denominator == 0.0f) {
        return -3.402823466e38F;
    }

    distance =
        -((normal.x * origin->x + normal.y * origin->y +
           normal.z * origin->z) -
          (normal.x * quad->quad_vertex_0.x +
           normal.y * quad->quad_vertex_0.y +
           normal.z * quad->quad_vertex_0.z)) /
        denominator;
    if (distance <= 0.0f) {
        return -3.402823466e38F;
    }

    parametric_ray_to_point(&point, origin, direction, distance);
    if (!is_point_inside_quad(quad, &point)) {
        return -3.402823466e38F;
    }
    return distance;
}
#pragma dont_inline reset

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void generate_collision_objects(
    int handle, unsigned int art_oid, const Vec* position,
    const Vec* angles, MkPtr** secondary_list) {
    CdfCollisionGroup* group;
    CdfCollisionPrimitive* primitive;
    CollisionObj* collision;
    unsigned char* cursor;
    int* cdf;
    int group_count;
    int group_index;
    int primitive_index;
    int vertex_index;
    Vec vertices[4];
    Vec transformed;
    MKMATRIX matrix;

    cdf = (int*)get_cdf_data(handle, art_oid);
    group_count = *cdf;
    cursor = (unsigned char*)(cdf + 1);
    for (group_index = 0; group_index < group_count; group_index++) {
        group = (CdfCollisionGroup*)cursor;
        cursor += sizeof(*group);
        for (primitive_index = 0;
             primitive_index < group->primitive_count;
             primitive_index++) {
            primitive = (CdfCollisionPrimitive*)cursor;
            cursor = (unsigned char*)primitive->vertices;
            collision = 0;
            if (primitive->vertex_count == 3) {
                collision = convert_cdf_triangle_to_collision_cylinder(
                    primitive->vertices, (void*)angles, (void*)position);
            } else if (primitive->vertex_count == 4) {
                if (group->obstacle_id == 1) {
                    vertices[0] = primitive->vertices[0];
                    vertices[1] = primitive->vertices[1];
                    vertices[2] = primitive->vertices[2];
                    vertices[3] = primitive->vertices[3];
                    if (angles != 0) {
                        YXZ_angles_to_MKMATRIX(angles, &matrix);
                        for (vertex_index = 0; vertex_index < 4;
                             vertex_index++) {
                            transformed = vertices[vertex_index];
                            v3_x_mat(
                                &vertices[vertex_index], &transformed,
                                &matrix);
                        }
                    }
                    if (position != 0) {
                        for (vertex_index = 0; vertex_index < 4;
                             vertex_index++) {
                            vertices[vertex_index].x += position->x;
                            vertices[vertex_index].y += position->y;
                            vertices[vertex_index].z += position->z;
                        }
                    }
                    collision = get_collision_obj();
                    if (collision != 0) {
                        collision->shape.type = 4;
                        collision->shape.quad_vertex_0 = vertices[0];
                        collision->shape.quad_vertex_1 = vertices[1];
                        collision->shape.quad_vertex_2 = vertices[2];
                        collision->shape.quad_vertex_3 = vertices[3];
                    }
                } else {
                    collision = convert_cdf_quad_to_collision_box(
                        primitive->vertices, (void*)angles,
                        (void*)position);
                }
            }
            if (collision != 0) {
                collision->obstacle_id = group->obstacle_id;
                mk_insert((MkHdr*)collision, &global_collision_list);
                if (secondary_list != 0) {
                    mk_insert((MkHdr*)collision, secondary_list);
                }
            }
            cursor += primitive->vertex_count * sizeof(Vec);
        }
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

static int collide_sphere_and_quad(
    const CollisionShape* sphere, const CollisionShape* quad) {
    Vec edge_0;
    Vec edge_1;
    Vec normal;
    Vec inward;
    float sphere_plane;
    float quad_plane;

    PSVECSubtract(
        &quad->quad_vertex_1, &quad->quad_vertex_0, &edge_0);
    PSVECSubtract(
        &quad->quad_vertex_3, &quad->quad_vertex_0, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    sphere_plane = normal.x * sphere->sphere_center.x +
        normal.y * sphere->sphere_center.y +
        normal.z * sphere->sphere_center.z;
    quad_plane = normal.x * quad->quad_vertex_0.x +
        normal.y * quad->quad_vertex_0.y +
        normal.z * quad->quad_vertex_0.z;
    if (sphere_plane > quad_plane + sphere->sphere_radius ||
        sphere_plane < quad_plane - sphere->sphere_radius) {
        return 0;
    }

#define TEST_SPHERE_QUAD_EDGE(first, second) \
    do { \
        PSVECSubtract((second), (first), &edge_0); \
        PSVECCrossProduct(&normal, &edge_0, &inward); \
        normalize_v3(&inward); \
        if (inward.x * sphere->sphere_center.x + \
                inward.y * sphere->sphere_center.y + \
                inward.z * sphere->sphere_center.z > \
            inward.x * (first)->x + inward.y * (first)->y + \
                inward.z * (first)->z) { \
            return 0; \
        } \
    } while (0)

    TEST_SPHERE_QUAD_EDGE(
        &quad->quad_vertex_0, &quad->quad_vertex_1);
    TEST_SPHERE_QUAD_EDGE(
        &quad->quad_vertex_1, &quad->quad_vertex_2);
    TEST_SPHERE_QUAD_EDGE(
        &quad->quad_vertex_2, &quad->quad_vertex_3);
    TEST_SPHERE_QUAD_EDGE(
        &quad->quad_vertex_3, &quad->quad_vertex_0);
#undef TEST_SPHERE_QUAD_EDGE
    return 1;
}

static void set_collision_vertex(
    CollisionIm3DVertex* vertex,
    const Vec* position,
    unsigned int color) {
    vertex->position = *position;
    vertex->color = color;
}

void render_col_shape_as_quad(
    const CollisionShape* shape, const unsigned int* color) {
    CollisionIm3DVertex vertices[4];

    set_collision_vertex(&vertices[0], &shape->quad_vertex_0, *color);
    set_collision_vertex(&vertices[1], &shape->quad_vertex_1, *color);
    set_collision_vertex(&vertices[2], &shape->quad_vertex_2, *color);
    set_collision_vertex(&vertices[3], &shape->quad_vertex_3, *color);
    if (RwIm3DTransform(vertices, 4, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, QuadVertexIndices, 8);
        RwIm3DEnd();
    }
}

void render_col_shape_as_box(
    const CollisionShape* shape, const unsigned int* color) {
    CollisionIm3DVertex vertices[8];
    Vec upper[4];
    float height;
    int index;

    height = shape->box_axis_0_max - shape->box_axis_0_min;
    upper[0].x = shape->box_corner_3.x + shape->box_axis_0.x * height;
    upper[0].y = shape->box_corner_3.y + shape->box_axis_0.y * height;
    upper[0].z = shape->box_corner_3.z + shape->box_axis_0.z * height;
    upper[1].x = shape->box_corner_2.x + shape->box_axis_0.x * height;
    upper[1].y = shape->box_corner_2.y + shape->box_axis_0.y * height;
    upper[1].z = shape->box_corner_2.z + shape->box_axis_0.z * height;
    upper[2] = shape->box_corner_3;
    upper[3] = shape->box_corner_2;

    set_collision_vertex(&vertices[0], &upper[0], *color);
    set_collision_vertex(&vertices[1], &upper[1], *color);
    set_collision_vertex(&vertices[2], &upper[2], *color);
    set_collision_vertex(&vertices[3], &upper[3], *color);

    upper[0].x = shape->box_corner_0.x + shape->box_axis_0.x * height;
    upper[0].y = shape->box_corner_0.y + shape->box_axis_0.y * height;
    upper[0].z = shape->box_corner_0.z + shape->box_axis_0.z * height;
    upper[1].x = shape->box_corner_1.x + shape->box_axis_0.x * height;
    upper[1].y = shape->box_corner_1.y + shape->box_axis_0.y * height;
    upper[1].z = shape->box_corner_1.z + shape->box_axis_0.z * height;
    upper[2] = shape->box_corner_0;
    upper[3] = shape->box_corner_1;
    for (index = 0; index < 4; index++) {
        set_collision_vertex(&vertices[index + 4], &upper[index], *color);
    }

    if (RwIm3DTransform(vertices, 8, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, AtomicBBoxIndices, 24);
        RwIm3DEnd();
    }
}

void render_col_shape_as_cylinder(
    const CollisionShape* shape, const unsigned int* color) {
    CollisionIm3DVertex wire_vertices[8];
    CollisionIm3DVertex vertices[16];
    Vec axis;
    Vec radial_0;
    Vec radial_1;
    Vec position;
    Vec endpoint;
    float cosine;
    float sine;
    float angle;
    int index;

    axis = shape->cylinder_axis;
    normalize_v3(&axis);
    v3_cross_v3(&radial_0, &axis, &UNITVECT_Y);
    if (length_v3(&radial_0) == 0.0f) {
        radial_0.x = 1.0f;
        radial_0.y = 0.0f;
        radial_0.z = 0.0f;
    } else {
        normalize_v3(&radial_0);
    }
    v3_cross_v3(&radial_1, &axis, &radial_0);
    normalize_v3(&radial_1);

    for (index = 0; index < 8; index++) {
        float side_0 = (index & 1) != 0 ?
            shape->cylinder_radius : -shape->cylinder_radius;
        float side_1 = (index & 2) != 0 ?
            shape->cylinder_radius : -shape->cylinder_radius;
        endpoint = shape->cylinder_center;
        if ((index & 4) != 0) {
            endpoint.x += axis.x * shape->cylinder_height;
            endpoint.y += axis.y * shape->cylinder_height;
            endpoint.z += axis.z * shape->cylinder_height;
        }
        position.x = endpoint.x +
            radial_0.x * side_0 + radial_1.x * side_1;
        position.y = endpoint.y +
            radial_0.y * side_0 + radial_1.y * side_1;
        position.z = endpoint.z +
            radial_0.z * side_0 + radial_1.z * side_1;
        set_collision_vertex(&wire_vertices[index], &position, *color);
    }
    if (RwIm3DTransform(wire_vertices, 8, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, AtomicBBoxIndices, 24);
        RwIm3DEnd();
    }

    for (index = 0; index < 16; index++) {
        angle = 3.1415927f * ((float)index / 7.5f);
        gxMathCosSin(&cosine, &sine, angle);
        position.x = shape->cylinder_center.x +
            shape->cylinder_radius *
                (radial_0.x * cosine + radial_1.x * sine);
        position.y = shape->cylinder_center.y +
            shape->cylinder_radius *
                (radial_0.y * cosine + radial_1.y * sine);
        position.z = shape->cylinder_center.z +
            shape->cylinder_radius *
                (radial_0.z * cosine + radial_1.z * sine);
        set_collision_vertex(&vertices[index], &position, *color);
    }
    if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
        RwIm3DRenderPrimitive(2);
        RwIm3DEnd();
    }
}

void render_col_shape(
    const CollisionShape* shape, const unsigned int* color) {
    CollisionIm3DVertex vertices[16];
    Vec radial;
    Vec transformed;
    int index;
    float angle;

    switch (shape->type & 7) {
    case 1:
        for (index = 0; index < 16; index++) {
            angle = 3.1415927f * ((float)index / 7.5f);
            radial.x = shape->sphere_radius * (float)gxMathCos(angle);
            radial.y = shape->sphere_radius * gxMathSin(angle);
            radial.z = 0.0f;
            v3_x_mat_add_v3(
                &transformed, &radial, &inv_cam_rot_mat,
                &shape->sphere_center);
            set_collision_vertex(&vertices[index], &transformed, *color);
        }
        if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
            RwIm3DRenderPrimitive(2);
            RwIm3DEnd();
        }
        break;
    case 2:
        render_col_shape_as_cylinder(shape, color);
        break;
    case 3:
        render_col_shape_as_box(shape, color);
        break;
    case 4:
        render_col_shape_as_quad(shape, color);
        break;
    }
}

void render_hero_collision(void) {
    render_col_shape(&konquest_hero_collision_shape, &rgba_white);
}

#pragma dont_inline on
static int test_collision(
    const CollisionShape* shape_a, const CollisionShape* shape_b) {
    unsigned int type_a;
    unsigned int type_b;
    float dx;
    float dy;
    float dz;
    float cross_x;
    float cross_y;
    float cross_z;
    float radius;
    float axial;

    type_a = shape_a->type & 7;
    type_b = shape_b->type & 7;

#define TEST_SPHERE_CYLINDER(sphere, cylinder) \
    do { \
        dx = (sphere)->sphere_center.x - (cylinder)->cylinder_center.x; \
        dy = (sphere)->sphere_center.y - (cylinder)->cylinder_center.y; \
        dz = (sphere)->sphere_center.z - (cylinder)->cylinder_center.z; \
        cross_x = dy * (cylinder)->cylinder_axis.z - \
            dz * (cylinder)->cylinder_axis.y; \
        cross_y = dz * (cylinder)->cylinder_axis.x - \
            dx * (cylinder)->cylinder_axis.z; \
        cross_z = dx * (cylinder)->cylinder_axis.y - \
            dy * (cylinder)->cylinder_axis.x; \
        radius = (sphere)->sphere_radius + (cylinder)->cylinder_radius; \
        if (cross_x * cross_x + cross_y * cross_y + cross_z * cross_z > \
            radius * radius) { \
            return 0; \
        } \
        axial = dx * (cylinder)->cylinder_axis.x + \
            dy * (cylinder)->cylinder_axis.y + \
            dz * (cylinder)->cylinder_axis.z; \
        if (axial < 0.0f) { \
            if (-axial > (sphere)->sphere_radius) { \
                return 0; \
            } \
        } else if (axial > (cylinder)->cylinder_height + \
                              (sphere)->sphere_radius) { \
            return 0; \
        } \
        return 1; \
    } while (0)

    switch (type_a) {
    case 1:
        switch (type_b) {
        case 1:
            dx = shape_a->sphere_center.x - shape_b->sphere_center.x;
            dy = shape_a->sphere_center.y - shape_b->sphere_center.y;
            dz = shape_a->sphere_center.z - shape_b->sphere_center.z;
            radius = shape_a->sphere_radius + shape_b->sphere_radius;
            return dx * dx + dy * dy + dz * dz <= radius * radius;
        case 3:
            return collide_sphere_and_box(shape_a, shape_b);
        case 2:
            TEST_SPHERE_CYLINDER(shape_a, shape_b);
        case 4:
            return collide_sphere_and_quad(shape_a, shape_b);
        }
        break;
    case 2:
        if (type_b == 1) {
            TEST_SPHERE_CYLINDER(shape_b, shape_a);
        }
        return 0;
    case 3:
        if (type_b == 1) {
            return collide_sphere_and_box(shape_b, shape_a);
        }
        return 0;
    case 4:
        if (type_b == 1) {
            return collide_sphere_and_quad(shape_b, shape_a);
        }
        return 0;
    }
#undef TEST_SPHERE_CYLINDER
    return 0;
}
#pragma dont_inline reset

static void xz_unit_vector_to_shape(
    Vec* result, const CollisionShape* shape, const Vec* point) {
    Vec edge_0;
    Vec edge_1;
    float half_height;

    switch (shape->type & 7) {
    case 2:
        result->x = shape->cylinder_center.x - point->x;
        result->z = shape->cylinder_center.z - point->z;
        break;
    case 3:
        half_height =
            0.5f * (shape->box_axis_0_max - shape->box_axis_0_min);
        result->x =
            shape->box_axis_0.x * half_height +
            0.5f * (shape->box_corner_2.x - shape->box_corner_1.x) +
            0.5f * (shape->box_corner_0.x + shape->box_corner_1.x) -
            point->x;
        result->z =
            shape->box_axis_0.z * half_height +
            0.5f * (shape->box_corner_2.z - shape->box_corner_1.z) +
            0.5f * (shape->box_corner_0.z + shape->box_corner_1.z) -
            point->z;
        break;
    case 4:
        v3_sub_v3(
            &edge_0, &shape->quad_vertex_1, &shape->quad_vertex_0);
        v3_sub_v3(
            &edge_1, &shape->quad_vertex_3, &shape->quad_vertex_0);
        v3_cross_v3(result, &edge_0, &edge_1);
        normalize_v3(result);
        if (result->x * (shape->quad_vertex_0.x - point->x) +
                result->z * (shape->quad_vertex_0.z - point->z) <
            0.0f) {
            result->x = -result->x;
            result->z = -result->z;
        }
        break;
    default:
        return;
    }

    normalize_xz(result);
    result->y = 0.0f;
}
