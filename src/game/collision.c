#include "game/collision.h"
#include "game/constrain.h"
#include "game/ejb.h"
#include "game/game_info.h"

#include "runtime/cstring.h"
#include "runtime/utils.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/asset.h"
#include "runtime/plyr_pdata.h"
#include "platform/display.h"
#include "platform/main.h"

#include "math/mk_math.h"
#include "math/gxMath.h"
#include "math/gxQuat.h"
#include "rw/rwim3d.h"
#include "rw/rwengine.h"
#include "rw/rtquat.h"

struct CdfCollisionGroup {
    int primitive_count;
    unsigned int field_04;
};

struct CdfCollisionPrimitive {
    int vertex_count;
    Vec vertices[1];
};

struct CollisionRepelInfo {
    int moving_shape;
    Vec* first_movement;
    Vec* second_movement;
    int preserve_first_contact;
};

struct CollisionNodeDef {
    int node_id;
    unsigned int side;
    float active_radius;
    float attack_radius_scale;
    float joint_radius;
};

struct WeaponCollisionDef {
    float radius;
    Vec offset;
};


struct CollisionObstacleVtable {
    int (*reserved[4])(void);
    void (*destroy)(ArenaObstacle* obstacle, void* vtbl);
};

static const struct CollisionNodeDef col_def_list[28] = {
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
static const int forearm_l[] = {0x14, 0x16, 0x18, 0};
static const int forearm_r[] = {0x15, 0x17, 0x19, 0};
static const int lowleg_l[] = {4, 7, 0};
static const int lowleg_r[] = {5, 8, 0};
static const int lowlegs[] = {5, 8, -1, 4, 7, 0};
static const int arm_left[] = {0xF, 0x12, 0x14, 0x16, 0x18, 0};
static const int arm_right[] = {0x11, 0x13, 0x15, 0x17, 0x19, 0};
static const int leg_left[] = {1, 4, 7, 0xA, 0};
static const int leg_right[] = {2, 5, 8, 0xB, 0};
static const int arm_both[] = {
    0x11, 0x13, 0x15, 0x17, 0x19, -1,
    0xF, 0x12, 0x14, 0x16, 0x18, 0
};
static const int armleg_right[] = {
    0x11, 0x13, 0x15, 0x17, 0x19, -1, 2, 5, 8, 0xB, 0
};
static const int armleg_left[] = {
    0xF, 0x12, 0x14, 0x16, 0x18, -1, 1, 4, 7, 0xA, 0
};
static const int leg_both[] = {
    1, 4, 7, 0xA, -1, 2, 5, 8, 0xB, 0
};
static const int back[] = {1, 2, 0};
static const int goro_lower_arm_both[] = {
    0x44, 0x46, 0x48, -1, 0x51, 0x53, 0x55, 0
};
static const int* attack_region_list[16] = {
    0, forearm_l, forearm_r, lowleg_l,
    lowleg_r, lowlegs, arm_left, arm_right,
    leg_left, leg_right, arm_both, armleg_right,
    armleg_left, leg_both, back, goro_lower_arm_both
};
static const Vec UNITVECT_Z = {0.0f, 0.0f, 1.0f};
static const Vec UNITVECT_NEGX = {-1.0f, 0.0f, 0.0f};
static const Vec UNITVECT_Y = {0.0f, 1.0f, 0.0f};
#define TEST_QUAD_EDGE(current, next) \
    do { \
        edge.x = (next)->x - (current)->x; \
        edge.y = (next)->y - (current)->y; \
        edge.z = (next)->z - (current)->z; \
        inward.x = edge.y * normal.z - edge.z * normal.y; \
        inward.y = edge.z * normal.x - edge.x * normal.z; \
        inward.z = edge.x * normal.y - edge.y * normal.x; \
        normalize_v3(&inward); \
        vertex_side = inward.x * (current)->x + inward.y * (current)->y + \
                      inward.z * (current)->z; \
        point_side = inward.x * point->x + inward.y * point->y + \
                     inward.z * point->z; \
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
static GlobalCollisionCallback global_collision_callback;
static MkPtr* konquest_shadow_collision_lists;

static void update_player_collision_nodes(PlayerCollisionData* collision);
static void render_danger_zone_collision_obj(MkHdr* object);
static void render_disabled_collision_obj(MkHdr* object);
static void render_collision_obj(MkHdr* object);
void render_bgnd_danger_zone_obstacle(ArenaObstacle* obstacle);
void render_obstacle(ArenaObstacle* obstacle);
static void render_hero_collision(void);
static void render_konquest_shadow_objects(MkHdr* object);
static void render_konquest_collision_obj(MkHdr* object);
extern void render_background_danger_areas(void);
int collide_shape_vs_plyr(
    PlyrInfo* player, const CollisionShape* shape);
static CollisionObj* convert_cdf_quad_to_collision_box(
    const Vec* vertices, const Vec* angles, const Vec* position);
static CollisionObj* convert_cdf_triangle_to_collision_cylinder(
    const Vec* vertices, const Vec* angles, const Vec* position);
ArenaObstacle* get_obstacle(void);
void insert_collision_on_proper_tile_list(CollisionObj* object);
static int collide_sphere_and_box(
    const CollisionShape* sphere, const CollisionShape* box);
static void get_center_for_shape(const CollisionShape* shape, Vec* center);

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
static void add_plyr_body_attack_nodes(
    int region_id, float radius, float extension);
static void generate_weapon_collision_nodes(
    PlayerCollisionData* collision, MkObj* weapon, float radius);
void get_weapon_collision_def(
    MkObj* weapon, struct WeaponCollisionDef* definition);
static float ray_intersection_with_shape(
    const CollisionShape* shape, const Vec* origin, Vec* direction);
static float ray_intersection_with_quad(
    const Vec* origin, const Vec* direction, const CollisionShape* quad);
int repel_shape_against_obstacle_list(
    PlyrInfo* player, CollisionShape* shape, Vec* movement, Vec* position,
    ConstrainInfo* info, Vec* test_position, int step_index);
static int repel_a_from_b(
    CollisionShape* shape, const CollisionShape* obstacle, Vec* movement);
static int repel_cylinder_and_box(
    CollisionShape* cylinder, const CollisionShape* box,
    struct CollisionRepelInfo* info);
static int repel_cylinder_and_quad(
    CollisionShape* cylinder, const CollisionQuad* quad,
    struct CollisionRepelInfo* info, int side_test);
static int repel_cylinders(
    CollisionShape* first, CollisionShape* second,
    struct CollisionRepelInfo* info);
int repel_cylinder_against_global_collision_list(
    CollisionShape* cylinder, Vec* movement);
void reset_player_collision(PlyrInfo* player);
static void render_players_joints(void);
static void render_player_joints(PlayerCollisionData* collision);
static float p_collision_update(void);
static void update_players_collision_nodes(void);
void render_col_shape(
    CollisionShape* shape, const unsigned int* color);
static void build_col_shape_vertical_box_from_corners(
    CollisionShape* shape, const Vec* corner_0, const Vec* corner_1,
    const Vec* corner_2, const Vec* corner_3);
static void render_col_shape_as_cylinder(
    const CollisionShape* shape, const unsigned int* color);
static void render_col_shape_as_box(
    const CollisionShape* shape, const unsigned int* color);
static void render_col_shape_as_quad(
    const CollisionShape* shape, const unsigned int* color);
static inline void set_collision_vertex(
    RwIm3DVertex* vertex, const Vec* position,
    const unsigned int* color) {
    const unsigned char* channels = (const unsigned char*)color;

    vertex->position.x = position->x;
    vertex->position.y = position->y;
    vertex->position.z = position->z;
    vertex->color_channels.red = channels[0];
    vertex->color_channels.green = channels[1];
    vertex->color_channels.blue = channels[2];
    vertex->color_channels.alpha = channels[3];
}
static inline CollisionObj* allocate_collision_obj(void) {
    CollisionObj* result;

    result = (CollisionObj*)get_mkhdr_generic(sizeof(CollisionObj));
    if (result != 0) {
        result->flags = 0;
    }
    return result;
}
static inline void add_collision_vectors(
    Vec* output, const Vec* first, const Vec* second) {
    output->x = first->x + second->x;
    output->y = first->y + second->y;
    output->z = first->z + second->z;
}
extern float __float_max[];
static inline float collision_dot_vectors(
    const Vec* first, const Vec* second) {
    return first->x * second->x + first->y * second->y +
        first->z * second->z;
}
static inline float collision_ray_to_plane(
    const Vec* origin,
    const Vec* direction,
    const Vec* normal,
    float plane_distance) {
    float origin_distance;
    float denominator;

    origin_distance = normal->x * origin->x + normal->y * origin->y +
        normal->z * origin->z;
    denominator = normal->x * direction->x + normal->y * direction->y +
        normal->z * direction->z;
    if (denominator != 0.0f) {
        return -(origin_distance - plane_distance) / denominator;
    }
    return -__float_max[0];
}
static inline int collision_point_within_face(
    const Vec* point,
    const Vec* axis_a,
    float upper_a,
    float lower_a,
    const Vec* axis_b,
    float upper_b,
    float lower_b) {
    float projection;

    projection = collision_dot_vectors(axis_a, point);
    if (projection >= upper_a) {
        return 0;
    }
    if (projection <= lower_a) {
        return 0;
    }
    projection = collision_dot_vectors(axis_b, point);
    if (projection >= upper_b) {
        return 0;
    }
    if (projection <= lower_b) {
        return 0;
    }
    return 1;
}
static inline int collision_point_inside_shape(
    const CollisionShape* shape, const Vec* point) {
    float radius = 0.0f;
    float projection;

    switch (shape->type & 7) {
    case 2: {
        float collision_radius;
        float distance;
        float dx;
        float dz;

        dz = point->z - shape->cylinder_center.z;
        collision_radius = shape->cylinder_radius + radius;
        dx = point->x - shape->cylinder_center.x;
        distance = dx * dx + dz * dz;
        if (distance < collision_radius * collision_radius) {
            return 1;
        }
        return 0;
    }
    case 3: {
        projection = shape->box_axis_2.x * point->x +
                     shape->box_axis_2.y * point->y +
                     shape->box_axis_2.z * point->z;
        if (projection >= shape->box_axis_2_min + radius) {
            return 0;
        }
        if (projection <= shape->box_axis_2_max - radius) {
            return 0;
        }
        projection = shape->box_axis_1.x * point->x +
                     shape->box_axis_1.y * point->y +
                     shape->box_axis_1.z * point->z;
        if (projection >= shape->box_axis_1_max + radius) {
            return 0;
        }
        if (projection <= shape->box_axis_1_min - radius) {
            return 0;
        }
        projection = shape->box_axis_0.x * point->x +
                     shape->box_axis_0.y * point->y +
                     shape->box_axis_0.z * point->z;
        if (projection >= shape->box_axis_0_min + radius) {
            return 0;
        }
        if (projection <= shape->box_axis_0_max - radius) {
            return 0;
        }
        return 1;
    }
    default:
        return 0;
    }
}

static inline void insert_player_attack_node_unshifted(
    PlayerCollisionData* storage,
    const PlayerCollisionNode* source) {
    CollisionShape* recorded;
    unsigned int index;

    if (storage->joint_count == 0U) {
        return;
    }
    index = storage->field_93F4;
    if (index >= 36U) {
        return;
    }
    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0 &&
        storage->recorded_count >= 36U) {
        return;
    }

    storage->attacks[index] = *source;
    storage->field_93F4++;
    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0) {
        recorded = storage->recorded_shapes;
        recorded[storage->recorded_count] = storage->attacks[index].world_shape;
        storage->recorded_count++;
    }
}

static inline void transform_collision_node(
    PlayerCollisionNode* node) {
    CollisionShape* source;
    CollisionShape* destination;
    MKMATRIX* matrix;
    float translation;

    source = &node->local_shape;
    destination = &node->world_shape;
    matrix = &node->bone->matrix;
    switch (source->type & 7) {
    case 1:
        p3_x_mat(
            &destination->sphere_center, &source->sphere_center,
            matrix);
        break;
    case 2:
        v3_x_mat(
            &destination->cylinder_axis, &source->cylinder_axis,
            matrix);
        p3_x_mat(
            &destination->cylinder_center, &source->cylinder_center,
            matrix);
        break;
    case 3:
        v3_x_mat(
            &destination->box_axis_2, &source->box_axis_2, matrix);
        v3_x_mat(
            &destination->box_axis_1, &source->box_axis_1, matrix);
        v3_x_mat(
            &destination->box_axis_0, &source->box_axis_0, matrix);

        translation =
            destination->box_axis_2.z * matrix->pos.z +
            (destination->box_axis_2.x * matrix->pos.x +
             destination->box_axis_2.y * matrix->pos.y);
        destination->box_axis_2_min =
            source->box_axis_2_min + translation;
        destination->box_axis_2_max =
            source->box_axis_2_max + translation;
        translation =
            destination->box_axis_1.z * matrix->pos.z +
            (destination->box_axis_1.x * matrix->pos.x +
             destination->box_axis_1.y * matrix->pos.y);
        destination->box_axis_1_max =
            source->box_axis_1_max + translation;
        destination->box_axis_1_min =
            source->box_axis_1_min + translation;
        translation =
            destination->box_axis_0.z * matrix->pos.z +
            (destination->box_axis_0.x * matrix->pos.x +
             destination->box_axis_0.y * matrix->pos.y);
        destination->box_axis_0_min =
            source->box_axis_0_min + translation;
        destination->box_axis_0_max =
            source->box_axis_0_max + translation;
        break;
    }
}

static inline PlayerCollisionNode* insert_player_attack_node(
    PlayerCollisionData* storage,
    const PlayerCollisionNode* source, const Vec* movement) {
    PlayerCollisionNode* destination;
    CollisionShape* recorded;
    unsigned int index;

    if (storage->joint_count == 0U) {
        return 0;
    }
    index = storage->field_93F4;
    if (index >= 36U) {
        return 0;
    }
    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0 &&
        storage->recorded_count >= 36U) {
        return 0;
    }

    destination = &storage->attacks[index];
    *destination = *source;
    v3_add_v3(
        &destination->world_shape.sphere_center,
        &destination->world_shape.sphere_center, movement);
    storage->field_93F4++;

    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0) {
        recorded = storage->recorded_shapes;
        recorded[storage->recorded_count] = destination->world_shape;
        storage->recorded_count++;
    }
    return destination;
}

static inline void update_all_collision_flags(
    MkPtr** list, unsigned int flags, int toggle) {
    MkPtr* item;
    MkPtr* next;
    CollisionObj* object;

    if (list == 0) {
        return;
    }
    item = *list;
    while (item != 0) {
        object = (CollisionObj*)item->hdr;
        if (item->instance != object->hdr.instance) {
            next = item->next;
            discard_stale_mkptr(item);
            item = next;
        } else if (toggle != 0) {
            object->flags ^= flags;
            item = item->next;
        } else {
            object->flags |= flags;
            item = item->next;
        }
    }
}

void purge_global_collision_list(void) {
    MkPtr* item;

    item = first_mkptr(&global_collision_list);
    while (item != 0) {
        item = next_mkptr(item);
    }
}

/* TODO: [breakthrough needed] 93.36%; ordered shape guards and outer list CFG recovered;
 * nested result lifetime and cylinder FP staging remain. */
int is_point_inside_shadow_exclusion_zone(
    const Vec* point, float radius) {
    CollisionObjList* list_object;
    CollisionObj* collision_object;
    MkPtr* list_item;
    MkPtr* collision_item;
    MkPtr* next;
    float projection;
    float dx;
    float dz;
    float test_radius;
    int inside;

    if (mklist_is_valid(&konquest_shadow_collision_lists)) {
        list_item = konquest_shadow_collision_lists;
        while (list_item != 0) {
            list_object = (CollisionObjList*)list_item->hdr;
            if (list_item->instance != list_object->hdr.instance) {
                next = list_item->next;
                discard_stale_mkptr(list_item);
                list_item = next;
                continue;
            }

            inside = 0;
            if (mklist_is_valid(&list_object->objects)) {
                collision_item = list_object->objects;
                while (collision_item != 0) {
                    collision_object = (CollisionObj*)collision_item->hdr;
                    if (collision_item->instance !=
                        collision_object->hdr.instance) {
                        next = collision_item->next;
                        discard_stale_mkptr(collision_item);
                        collision_item = next;
                        continue;
                    }

                    switch (collision_object->shape.type & 7) {
                    case 2:
                        dx = point->x -
                            collision_object->shape.cylinder_center.x;
                        dz = point->z -
                            collision_object->shape.cylinder_center.z;
                        test_radius =
                            collision_object->shape.cylinder_radius + radius;
                        if (dx * dx + dz * dz < test_radius * test_radius) {
                            inside = 1;
                        } else {
                            inside = 0;
                        }
                        break;
                    case 3:
                        projection =
                            collision_object->shape.box_axis_2.x * point->x +
                            collision_object->shape.box_axis_2.y * point->y +
                            collision_object->shape.box_axis_2.z * point->z;
                        if (projection >=
                                collision_object->shape.box_axis_2_min + radius ||
                            projection <=
                                collision_object->shape.box_axis_2_max - radius) {
                            inside = 0;
                            break;
                        }
                        projection =
                            collision_object->shape.box_axis_1.x * point->x +
                            collision_object->shape.box_axis_1.y * point->y +
                            collision_object->shape.box_axis_1.z * point->z;
                        if (projection >=
                                collision_object->shape.box_axis_1_max + radius ||
                            projection <=
                                collision_object->shape.box_axis_1_min - radius) {
                            inside = 0;
                            break;
                        }
                        projection =
                            collision_object->shape.box_axis_0.x * point->x +
                            collision_object->shape.box_axis_0.y * point->y +
                            collision_object->shape.box_axis_0.z * point->z;
                        if (projection >=
                                collision_object->shape.box_axis_0_min + radius) {
                            inside = 0;
                        } else if (projection <=
                                collision_object->shape.box_axis_0_max - radius) {
                            inside = 0;
                        } else {
                            inside = 1;
                        }
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
    }
    return 0;
}

void remove_collision_list_from_konquest_shadow_lists(MkHdr* collision_list) {
    mk_pull_discard(collision_list, &konquest_shadow_collision_lists);
}

void insert_collision_list_on_konquest_shadow_lists(MkHdr* collision_list) {
    mk_insert_no_own(collision_list, &konquest_shadow_collision_lists);
}

void generate_shadow_collision_objects(int handle, unsigned int art_oid) {
    int primitive_count;
    int primitive_index;
    int group_count;
    int group_index;
    unsigned int group_flags;
    unsigned char* cursor;
    int* cdf;

    cdf = get_cdf_data(handle, art_oid);
    group_count = *cdf;
    cursor = (unsigned char*)(cdf + 1);

    for (group_index = 0; group_index < group_count; group_index++) {
        struct CdfCollisionGroup* group;

        group = (struct CdfCollisionGroup*)cursor;
        primitive_count = group->primitive_count;
        group_flags = group->field_04;
        cursor += sizeof(*group);
        for (primitive_index = 0;
             primitive_index < primitive_count;
             primitive_index++) {
            struct CdfCollisionPrimitive* primitive;
            CollisionObj* object;
            int vertex_count;

            primitive = (struct CdfCollisionPrimitive*)cursor;
            vertex_count = primitive->vertex_count;
            object = 0;
            cursor = (unsigned char*)primitive->vertices;
            if (vertex_count == 4) {
                object = convert_cdf_quad_to_collision_box(
                    (Vec*)cursor, 0, 0);
            }
            if (object != 0) {
                object->flags = group_flags;
                insert_collision_on_proper_tile_list(object);
            }
            cursor += vertex_count * sizeof(Vec);
        }
    }
}


int segment_against_obstacle_list(
    Vec* start, Vec* end, Vec* hit_point, MkPtr** obstacle_list) {
    MkPtr* obstacle_item;
    ArenaObstacle* obstacle;
    MkPtr* collision_item;
    CollisionObj* collision;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max[0];
    if (start == 0) {
        return 0;
    }
    if (end == 0) {
        return 0;
    }
    if (hit_point == 0) {
        return 0;
    }

    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (obstacle_list != 0) {
        obstacle_item = *obstacle_list;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                obstacle_item = discard_stale_mkptr_and_advance(obstacle_item);
                continue;
            }

            if (!obstacle->flags.bits.disabled && !obstacle->flags.bits.danger_zone &&
                &obstacle->shapes != 0) {
                collision_item = obstacle->shapes;
                while (collision_item != 0) {
                    collision = (CollisionObj*)collision_item->hdr;
                    if (collision_item->instance != collision->hdr.instance) {
                        collision_item = discard_stale_mkptr_and_advance(collision_item);
                        continue;
                    }
                    if ((collision->flags & 0x10000) == 0) {
                        distance = ray_intersection_with_shape(
                            &collision->shape, start, &direction);
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

void set_flag_for_all_collisions(MkPtr** list, unsigned int flags) {
    update_all_collision_flags(list, flags, 0);
}

void exclusive_or_flags_for_all_collisions(
    MkPtr** list, unsigned int flags) {
    update_all_collision_flags(list, flags, 1);
}

void set_global_collision_callback(GlobalCollisionCallback callback) {
    global_collision_callback = callback;
}

void collision_obj_set_shape(CollisionObj* object, const CollisionShape* shape) {
    object->shape = *shape;
}

static inline void build_col_shape_quad(
    CollisionShape* shape, const Vec* corner_0, const Vec* corner_1,
    const Vec* corner_2, const Vec* corner_3) {
    if (shape != 0) {
        shape->type = 4;
        shape->quad_vertex_0 = *corner_0;
        shape->quad_vertex_1 = *corner_1;
        shape->quad_vertex_2 = *corner_2;
        shape->quad_vertex_3 = *corner_3;
    }
}

void generate_obstacles(int handle, char* name, MkPtr** obstacle_list) {
    int primitive_count;
    int primitive_index;
    int group_count;
    struct CdfCollisionGroup* group;
    ArenaObstacle* obstacle;
    CollisionObj* collision;
    int* cdf;
    int group_index;
    int* cursor;
    int vertex_index;
    Vec vertices[4];
    CollisionShape* quad_shape;

    cdf = load_named_cdf_data_from_slot(handle, name);
    if (cdf == 0) {
        return;
    }

    group_count = *cdf;
    cursor = cdf + 1;
    for (group_index = 0; group_index < group_count; group_index++) {
        obstacle = get_obstacle();
        group = (struct CdfCollisionGroup*)cursor;
        primitive_count = group->primitive_count;
        obstacle->obstacle_id = group->field_04;
        cursor += sizeof(*group) / sizeof(*cursor);
        mk_insert(&obstacle->hdr, obstacle_list);
        obstacle->type = get_obstacle_type_from_id(obstacle->obstacle_id);

        for (primitive_index = 0;
             primitive_index < primitive_count;
             primitive_index++) {
            int vertex_count;
            Vec* points;

            vertex_count = ((struct CdfCollisionPrimitive*)cursor)->vertex_count;
            cursor++;
            points = (Vec*)cursor;
            if (obstacle->type == 3 || obstacle->type == 1 ||
                obstacle->type == 4) {
                if (vertex_count == 4) {
                    for (vertex_index = 0; vertex_index < 4;
                         vertex_index++) {
                        gxVectCopy(
                            &vertices[vertex_index], &points[vertex_index]);
                    }
                    collision = allocate_collision_obj();
                    if (collision != 0) {
                        quad_shape = &collision->shape;
                        build_col_shape_quad(quad_shape,
                            &vertices[0], &vertices[1],
                            &vertices[2], &vertices[3]);
                    }
                    if (collision != 0) {
                        mk_insert(&collision->hdr, &obstacle->shapes);
                    }
                }
            } else if (obstacle->type == 5 || obstacle->type == 6) {
                if (vertex_count == 3) {
                    collision = convert_cdf_triangle_to_collision_cylinder(
                        points, 0, 0);
                    mk_insert(&collision->hdr, &obstacle->shapes);
                } else if (vertex_count == 4) {
                    collision = convert_cdf_quad_to_collision_box(
                        points, 0, 0);
                    mk_insert(&collision->hdr, &obstacle->shapes);
                }
            }
            cursor += vertex_count * (sizeof(Vec) / sizeof(*cursor));
        }
    }
}

/* TODO: [near miss] 99.75%; latch fixed; final displacement FPR homes remain (scalar/Vec/helper/commuted spellings neutral). */
void repel_against_obstacle_list(
    PlyrInfo* player, Vec* previous_position, Vec* movement,
    Vec* position, ConstrainInfo* info) {
    CollisionShape shape;
    MkObj* object;
    Vec original_position;
    Vec step;
    Vec candidate;
    float length;
    float fraction;
    int step_count;
    int step_index;

    gxVectCopy(&original_position, position);
    length = gxMathFastSqrt(
        movement->x * movement->x + movement->z * movement->z);

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
        gxVectCopy(&shape.cylinder_axis, &UNITVECT_Y);
        shape.cylinder_height = 2.5f;
        gxVectCopy(&shape.cylinder_center, &candidate);
        shape.cylinder_center.y -= 1.0f;

        if (repel_shape_against_obstacle_list(
                player, &shape, &step, &candidate, info,
                &shape.cylinder_center, step_index) != 0) {
            position->x = candidate.x;
            position->z = candidate.z;
            break;
        }
    }

    object = MK_HDR_LIVE(
        player->slot.pdata->held_by_object_latch.obj,
        player->slot.pdata->held_by_object_latch.instance);
    if (object != 0) {
        Vec displacement;

        displacement.x = position->x - original_position.x;
        displacement.y = position->y - original_position.y;
        displacement.z = position->z - original_position.z;
        object->pos.value.x += displacement.x;
        object->pos.value.y += displacement.y;
        object->pos.value.z += displacement.z;
    }
}

/* TODO: [breakthrough] 80.43%; callback now advances the obstacle loop;
 * validated obstacle ownership and push-loop register lifetimes remain. */
int repel_shape_against_obstacle_list(
    PlyrInfo* player, CollisionShape* shape, Vec* movement, Vec* position,
    ConstrainInfo* info, Vec* test_position, int step_index) {
    PlyrPdata* collision_data;
    BgndObstacleEventData callback_data;
    ArenaObstacle* obstacle;
    CollisionObj* collision;
    MkPtr* obstacle_item;
    MkPtr* collision_item;
    MkPtr* next;
    Vec pushes[20];
    float original_x;
    float original_y;
    float original_z;
    Vec total;
    int collision_count;
    int collided;
    int result;
    int index;

    original_x = test_position->x;
    original_y = test_position->y;
    original_z = test_position->z;
    result = 0;
    collision_count = 0;
    if (info != 0) {
        obstacle_item = info->obstacles;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                next = obstacle_item->next;
                discard_stale_mkptr(obstacle_item);
                obstacle_item = next;
                continue;
            }
            if (step_index == 0) {
                obstacle->flags.bits.callback_handled = 0;
            }
            if (!obstacle->flags.bits.disabled &&
                &obstacle->shapes != 0) {
                collision_item = obstacle->shapes;
                while (collision_item != 0) {
                    collision = (CollisionObj*)collision_item->hdr;
                    if (collision_item->instance != collision->hdr.instance) {
                        next = collision_item->next;
                        discard_stale_mkptr(collision_item);
                        collision_item = next;
                        continue;
                    }

                    collided = 0;
                    switch (collision->shape.type & 7) {
                    case 4:
                        if (repel_a_from_b(
                                shape, &collision->shape,
                                movement) != 0) {
                            collided = 1;
                        }
                        break;
                    case 2:
                        if (repel_a_from_b(
                                shape, &collision->shape,
                                movement) != 0) {
                            collided = 1;
                        }
                        break;
                    case 3:
                        if (repel_a_from_b(
                                shape, &collision->shape,
                                movement) != 0) {
                            collided = 1;
                        }
                        break;
                    }
                    if (obstacle->flags.bits.inverted) {
                        if (collided != 0) {
                            test_position->x = original_x;
                            test_position->y = original_y;
                            test_position->z = original_z;
                        }
                        collided = collided == 0;
                    }

                    if (collided != 0) {
                        if (constrain_info.callback != 0 &&
                            local_obstacle_callback(obstacle) != 0) {
                            callback_data.field_04 = obstacle->type;
                            callback_data.flags = 0;
                            callback_data.flag_bits.player_side = 1;
                            callback_data.event_id = obstacle->obstacle_id;
                            callback_data.impact_vector = movement;
                            movement->y = 0.0f;
                            callback_data.player_pdata = player->slot.pdata;
                            if (!obstacle->flags.bits.callback_handled) {
                                if (constrain_info.callback(
                                        &callback_data) != 0) {
                                    if (obstacle->hdr.instance != 0) {
                                        ((struct CollisionObstacleVtable*)
                                             obstacle->hdr.vtbl)->destroy(
                                            obstacle, obstacle->hdr.vtbl);
                                    }
                                    break;
                                }
                                obstacle->flags.bits.callback_handled = 1;
                            }
                        }
                        if (!obstacle->flags.bits.repel) {
                            collision_data = player->slot.pdata;
                            collision_data->f_constrained = 1;
                            if (collision_count < 20) {
                                pushes[collision_count].x =
                                    test_position->x - original_x;
                                pushes[collision_count].y =
                                    test_position->y - original_y;
                                pushes[collision_count].z =
                                    test_position->z - original_z;
                                collision_count++;
                            }
                        }
                        test_position->x = original_x;
                        test_position->y = original_y;
                        test_position->z = original_z;
                    }
                    collision_item = collision_item->next;
                }
            }
            obstacle_item = obstacle_item->next;
        }
    }

    if (collision_count != 0) {
        test_position->x = original_x;
        test_position->y = original_y;
        test_position->z = original_z;
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

void destroy_konquest_shadow_collision_lists(void) {
    destroy_list(&konquest_shadow_collision_lists);
}

int collide_segment_against_global_collision_list_quads(
    Vec* start, Vec* end, Vec* hit_point) {
    MkPtr* next;
    CollisionObj* collision;
    MkPtr* item;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max[0];
    if (start == 0) {
        return 0;
    }
    if (end == 0) {
        return 0;
    }
    if (hit_point == 0) {
        return 0;
    }
    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            int shape_kind;

            collision = (CollisionObj*)item->hdr;
            if (item->instance != collision->hdr.instance) {
                next = item->next;
                discard_stale_mkptr(item);
                item = next;
                continue;
            }
            shape_kind = collision->shape.type & 7;
            if (shape_kind == 4) {
                distance = ray_intersection_with_shape(
                    &collision->shape, start, &direction);
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
    Vec* target, Vec* start, Vec* result_point,
    unsigned int ignored_flags) {
    CollisionObj* collision;
    MkPtr* next;
    MkPtr* item;
    Vec direction;
    float segment_length;
    float closest;
    float distance;
    int inside;

    closest = -__float_max[0];
    if (target == 0) {
        return 0;
    }
    if (start == 0) {
        return 0;
    }
    if (result_point == 0) {
        return 0;
    }
    segment_length = uv_v3_to_v3_dist(&direction, start, target);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision = (CollisionObj*)item->hdr;
            if (item->instance != collision->hdr.instance) {
                next = item->next;
                discard_stale_mkptr(item);
                item = next;
                continue;
            }
            if ((collision->flags & ignored_flags) == 0 &&
                (int)(collision->shape.type & 7) != 4) {
                inside = collision_point_inside_shape(
                    &collision->shape, target);
                if (inside != 0) {
                    distance = ray_intersection_with_shape(
                        &collision->shape, start, &direction);
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
    Vec* start, Vec* end, Vec* hit_point,
    unsigned int ignored_flags) {
    CollisionObj* collision;
    MkPtr* item;
    Vec direction;
    float segment_length;
    float closest;
    float distance;

    closest = -__float_max[0];
    if (start == 0) return 0;
    if (end == 0) return 0;
    if (hit_point == 0) return 0;
    segment_length = uv_v3_to_v3_dist(&direction, start, end);
    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            int shape_kind;

            collision = (CollisionObj*)item->hdr;
            if (item->instance != collision->hdr.instance) {
                item = discard_stale_mkptr_and_advance(item);
                continue;
            }
            shape_kind = collision->shape.type & 7;
            if (shape_kind != 4 &&
                (collision->flags & ignored_flags) == 0) {
                distance = ray_intersection_with_shape(
                    &collision->shape, start, &direction);
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

/* TODO: [breakthrough] 73.34%; signed shape-kind snapshot recovered; cylinder setup and addressed target-vector ownership remain. */
int npc_repel_against_global_collision_list(
    const Vec* position, Vec* movement, Vec* result_position,
    unsigned int ignored_flags) {
    CollisionObj* collision;
    CollisionShape shape;
    MkPtr* item;
    MkPtr* next;
    Vec target_position;
    Vec reverse_direction;
    int shape_kind;
    int collided;
    int result;

    result = 0;
    shape.type = 2;
    shape.cylinder_radius = 0.35f;
    shape.cylinder_axis = UNITVECT_Y;
    shape.cylinder_height = 2.4f;
    target_position.x = position->x + movement->x;
    target_position.y = position->y + movement->y;
    target_position.z = position->z + movement->z;
    shape.cylinder_center.x = target_position.x;
    shape.cylinder_center.y = target_position.y - 1.2f;
    shape.cylinder_center.z = target_position.z;
    result_position->x = target_position.x;
    result_position->y = target_position.y;
    result_position->z = target_position.z;
    reverse_direction.x = -movement->x;
    reverse_direction.y = -movement->y;
    reverse_direction.z = -movement->z;
    normalize_v3(&reverse_direction);

    if (&global_collision_list != 0) {
        item = global_collision_list;
        while (item != 0) {
            collision = (CollisionObj*)item->hdr;
            if (item->instance != collision->hdr.instance) {
                next = item->next;
                discard_stale_mkptr(item);
                item = next;
                continue;
            }
            collided = 0;
            shape_kind = collision->shape.type & 7;
            if (shape_kind != 4 &&
                (collision->flags & ignored_flags) == 0) {
                if (shape_kind == 2) {
                    if (repel_a_from_b(
                            &shape, &collision->shape, movement) != 0) {
                        collided = 1;
                        result_position->x = shape.cylinder_center.x;
                        result_position->z = shape.cylinder_center.z;
                    }
                } else if (shape_kind == 3 &&
                           repel_a_from_b(
                               &shape, &collision->shape,
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

/* TODO: [breakthrough] 81.78%; target snapshot and float axis copies recovered;
 * target-position stack storage, aligned frame and init schedule remain. */
int repel_against_global_collision_list(
    const Vec* position, Vec* movement, Vec* result_position) {
    CollisionShape shape;
    Vec target_position;
    Vec reverse_direction;
    int result;

    result = 0;
    target_position.y = position->y + movement->y;
    target_position.x = position->x + movement->x;
    target_position.z = position->z + movement->z;
    shape.type = 2;
    shape.cylinder_radius = 0.35f;
    shape.cylinder_axis.x = UNITVECT_Y.x;
    shape.cylinder_axis.y = UNITVECT_Y.y;
    shape.cylinder_axis.z = UNITVECT_Y.z;
    shape.cylinder_height = 2.4f;
    shape.cylinder_center.y = target_position.y;
    shape.cylinder_center.x = target_position.x;
    shape.cylinder_center.z = target_position.z;
    shape.cylinder_center.y -= 1.0f;
    result_position->x = target_position.x;
    result_position->y = target_position.y;
    result_position->z = target_position.z;
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

/* TODO: [breakthrough] 82.01%; center snapshots, callback ownership and retry exit recovered;
 * push staging and nonvolatile register lifetimes remain. */
int repel_cylinder_against_global_collision_list(
    CollisionShape* cylinder, Vec* movement) {
    CollisionObj* collision;
    MkPtr* item;
    MkPtr* next;
    unsigned int callback_id;
    Vec pushes[20];
    float original_x;
    float original_y;
    float original_z;
    float pass_x;
    float pass_y;
    float pass_z;
    Vec total;
    int collision_count;
    int shape_type;
    int collided;
    int retry;
    int pass;
    int result;
    int index;

    result = 0;
    retry = 1;
    pass = 0;
    original_x = cylinder->cylinder_center.x - movement->x;
    original_y = cylinder->cylinder_center.y - movement->y;
    original_z = cylinder->cylinder_center.z - movement->z;

    while (retry == 1) {
        pass_x = cylinder->cylinder_center.x;
        pass_y = cylinder->cylinder_center.y;
        pass_z = cylinder->cylinder_center.z;
        collision_count = 0;
        retry = 0;
        pass++;
        if (&global_collision_list != 0) {
            item = global_collision_list;
            while (item != 0) {
                collision = (CollisionObj*)item->hdr;
                if (item->instance != collision->hdr.instance) {
                    next = item->next;
                    discard_stale_mkptr(item);
                    item = next;
                    continue;
                }
                collided = 0;
                shape_type = collision->shape.type & 7;
                if (shape_type == 2) {
                    if (repel_a_from_b(
                            cylinder, &collision->shape,
                            movement) != 0) {
                        collided = 1;
                    }
                } else if (shape_type == 3 &&
                           repel_a_from_b(
                               cylinder, &collision->shape,
                               movement) != 0) {
                    collided = 1;
                }
                if (collided == 1) {
                    if (collision_count < 20) {
                        pushes[collision_count].x =
                            cylinder->cylinder_center.x - pass_x;
                        pushes[collision_count].y =
                            cylinder->cylinder_center.y - pass_y;
                        pushes[collision_count].z =
                            cylinder->cylinder_center.z - pass_z;
                        collision_count++;
                    }
                    cylinder->cylinder_center.x = pass_x;
                    cylinder->cylinder_center.y = pass_y;
                    cylinder->cylinder_center.z = pass_z;
                    if (global_collision_callback != 0) {
                        callback_id = collision->obstacle_id;
                        global_collision_callback(&callback_id);
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
                        cylinder->cylinder_center.x = original_x;
                        cylinder->cylinder_center.y = original_y;
                        cylinder->cylinder_center.z = original_z;
                        total.x = 0.0f;
                        total.y = 0.0f;
                        total.z = 0.0f;
                        break;
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

static inline float collision_ray_box_face(
    const Vec* origin, const Vec* direction, const Vec* normal, float plane,
    const Vec* axis_a, float upper_a, float lower_a,
    const Vec* axis_b, float upper_b, float lower_b, float nearest)
{
    Vec point;
    float distance = collision_ray_to_plane(origin, direction, normal, plane);
    if (distance > 0.0f && (nearest < 0.0f || distance < nearest)) {
        parametric_ray_to_point(&point, origin, direction, distance);
        if (collision_point_within_face(&point, axis_a, upper_a, lower_a,
                axis_b, upper_b, lower_b)) {
            return distance;
        }
    }
    return 0.0f;
}

static inline float collision_ray_cylinder(const CollisionCylinder* shape,
    const Vec* origin, Vec* direction)
{
    Vec perpendicular;
    float projection;
    float side_distance;
    float along_distance;
    float root;
    float distance;
    float delta_z;
    float delta_x;
    perpendicular.x = direction->z;
    perpendicular.z = -direction->x;
    normalize_xz(&perpendicular);
    delta_x = origin->x - shape->center.value.x;
    delta_z = origin->z - shape->center.value.z;
    side_distance = delta_x * perpendicular.x + delta_z * perpendicular.z;
    if (side_distance >= 0.0f) {
        projection = side_distance;
    } else {
        projection = -side_distance;
    }
    if (projection >= shape->radius) {
        return -__float_max[0];
    }
    along_distance = -(
        perpendicular.x * delta_z + perpendicular.z * -delta_x);
    if (along_distance <= 0.0f) {
        return -__float_max[0];
    }
    root = gxMathFastSqrt(
        shape->radius * shape->radius -
        side_distance * side_distance);
    distance = along_distance - root;
    if (distance <= 0.0f) {
        return -__float_max[0];
    }
    return distance;
}

/* TODO: [near miss] 99.95%; cylinder along/radicand FPR homes (f4/f5) remain, shared with repel_cylinders. */
static float ray_intersection_with_shape(
    const CollisionShape* shape, const Vec* origin, Vec* direction) {
    float nearest;
    float distance;

    switch (shape->type & 7) {
    case 3:
        nearest = -__float_max[0];
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_2, shape->box_axis_2_min,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_2, shape->box_axis_2_max,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_1, shape->box_axis_1_min,
            &shape->box_axis_2, shape->box_axis_2_min,
            shape->box_axis_2_max, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_1, shape->box_axis_1_max,
            &shape->box_axis_2, shape->box_axis_2_min,
            shape->box_axis_2_max, &shape->box_axis_0,
            shape->box_axis_0_min, shape->box_axis_0_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_0, shape->box_axis_0_min,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_2,
            shape->box_axis_2_min, shape->box_axis_2_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        distance = collision_ray_box_face(origin, direction,
            &shape->box_axis_0, shape->box_axis_0_max,
            &shape->box_axis_1, shape->box_axis_1_max,
            shape->box_axis_1_min, &shape->box_axis_2,
            shape->box_axis_2_min, shape->box_axis_2_max, nearest);
        if (distance > 0.0f) {
            nearest = distance;
        }
        return nearest;
    case 4:
        return ray_intersection_with_quad(origin, direction, shape);
    case 2:
        return collision_ray_cylinder(&shape->cylinder, origin, direction);
    default:
        return -__float_max[0];
    }
}

/* TODO: [near miss] 99.702377%; aligned frame and ray-plane math agree;
 * plane-result FPR homes remain. */
static float ray_intersection_with_quad(
    const Vec* origin,
    const Vec* direction,
    const CollisionShape* quad) {
    MKVECTOR normal;
    MKVECTOR point;
    MKVECTOR edge_1;
    MKVECTOR edge_0;
    float distance;

    PSVECSubtract(&quad->quad_vertex_1, &quad->quad_vertex_0, &edge_0);
    PSVECSubtract(&quad->quad_vertex_3, &quad->quad_vertex_0, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    distance = collision_ray_to_plane(
        origin, direction, &normal,
        collision_dot_vectors(&normal, &quad->quad_vertex_0));

    if (distance > 0.0f) {
        parametric_ray_to_point(&point, origin, direction, distance);
        if (is_point_inside_quad(quad, &point)) {
            return distance;
        }
    }
    return -__float_max[0];
}

static int is_point_inside_quad(
    const CollisionShape* quad, const Vec* point) {
    MKVECTOR normal;
    MKVECTOR inward;
    MKVECTOR edge_1;
    MKVECTOR edge_0;
    Vec edge;
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

CollisionObj* add_shape_to_global_collision_list(
    const CollisionShape* shape,
    unsigned int flags) {
    CollisionObj* result;

    result = (CollisionObj*)get_mkhdr_generic(sizeof(CollisionObj));
    if (result != 0) {
        result->flags = 0;
    }

    if (result != 0) {
        result->shape = *shape;
        result->flags = flags;
        mk_insert(&result->hdr, &global_collision_list);
        return result;
    }
    return 0;
}

void generate_collision_objects(
    int handle, unsigned int art_oid, Vec* position,
    const Vec* angles, MkPtr** secondary_list) {
    int primitive_count;
    int primitive_index;
    int group_count;
    int group_index;
    int group_flags;
    int* cursor;
    int vertex_count;
    struct CdfCollisionGroup* group;
    Vec* points;
    CollisionObj* collision;
    CollisionShape* quad_shape;
    int* cdf;
    int vertex_index;
    MKMATRIX matrix;
    Vec vertices[4];
    Vec transformed;

    cdf = get_cdf_data(handle, art_oid);
    group_count = *cdf;
    cursor = cdf + 1;
    for (group_index = 0; group_index < group_count; group_index++) {
        group = (struct CdfCollisionGroup*)cursor;
        primitive_count = group->primitive_count;
        group_flags = group->field_04;
        cursor += sizeof(*group) / sizeof(*cursor);
        for (primitive_index = 0;
             primitive_index < primitive_count;
             primitive_index++) {
            vertex_count = ((struct CdfCollisionPrimitive*)cursor)->vertex_count;
            cursor++;
            points = (Vec*)cursor;
            collision = 0;
            if (vertex_count == 3) {
                collision = convert_cdf_triangle_to_collision_cylinder(
                    points, angles, position);
            } else if (vertex_count == 4) {
                if (group_flags == 1) {
                    CollisionObj* quad;

                    for (vertex_index = 0; vertex_index < 4;
                         vertex_index++) {
                        gxVectCopy(
                            &vertices[vertex_index], &points[vertex_index]);
                    }
                    if (angles != 0) {
                        YXZ_angles_to_MKMATRIX(angles, &matrix);
                        for (vertex_index = 0; vertex_index < 4;
                             vertex_index++) {
                            gxVectCopy(
                                &transformed, &vertices[vertex_index]);
                            v3_x_mat(
                                &vertices[vertex_index], &transformed,
                                &matrix);
                        }
                    }
                    if (position != 0) {
                        for (vertex_index = 0; vertex_index < 4;
                             vertex_index++) {
                            add_collision_vectors(
                                &vertices[vertex_index], position,
                                &vertices[vertex_index]);
                        }
                    }
                    quad = allocate_collision_obj();
                    if (quad != 0) {
                        quad_shape = &quad->shape;
                        build_col_shape_quad(quad_shape,
                            &vertices[0], &vertices[1],
                            &vertices[2], &vertices[3]);
                    }
                    collision = quad;
                } else {
                    collision = convert_cdf_quad_to_collision_box(
                        points, angles,
                        position);
                }
            }
            if (collision != 0) {
                collision->flags = group_flags;
                mk_insert(&collision->hdr, &global_collision_list);
                if (secondary_list != 0) {
                    mk_insert(&collision->hdr, secondary_list);
                }
            }
            cursor += vertex_count * (sizeof(Vec) / sizeof(*cursor));
        }
    }
}

/* TODO: [breakthrough] 86.60%; built-corner max radius and corner homes recovered; component scheduling and center ownership remain. */
static CollisionObj* convert_cdf_quad_to_collision_box(
    const Vec* source, const Vec* angles, const Vec* position) {
    CollisionObj* collision;
    MKMATRIX matrix;
    Vec corner_0;
    Vec corner_1;
    Vec corner_2;
    Vec corner_3;
    Vec input;
    float center_x;
    float center_z;
    float half_height;
    float dx;
    float dz;
    float distance;
    int index;

    corner_0.x = source[2].x;
    corner_0.y = 0.0f;
    corner_0.z = source[2].z;
    corner_1.x = source[1].x;
    if (source[1].y <= 0.0f) {
        corner_1.y = 0.001f;
    } else {
        corner_1.y = source[1].y;
    }
    corner_1.z = source[1].z;
    corner_2.x = source[0].x;
    if (source[0].y <= 0.0f) {
        corner_2.y = 0.001f;
    } else {
        corner_2.y = source[0].y;
    }
    corner_2.z = source[0].z;
    corner_3.x = source[3].x;
    if (source[3].y <= 0.0f) {
        corner_3.y = 0.001f;
    } else {
        corner_3.y = source[3].y;
    }
    corner_3.z = source[3].z;
    if (corner_1.y < 0.0f || corner_2.y < 0.0f) {
        corner_0.y = corner_1.y - 3.0f;
    }

    if (angles != 0) {
        YXZ_angles_to_MKMATRIX(angles, &matrix);
        gxVectCopy(&input, &corner_0);
        v3_x_mat(&corner_0, &input, &matrix);
        gxVectCopy(&input, &corner_1);
        v3_x_mat(&corner_1, &input, &matrix);
        gxVectCopy(&input, &corner_2);
        v3_x_mat(&corner_2, &input, &matrix);
        gxVectCopy(&input, &corner_3);
        v3_x_mat(&corner_3, &input, &matrix);
    }
    if (position != 0) {
        add_collision_vectors(&corner_0, &corner_0, position);
        add_collision_vectors(&corner_1, &corner_1, position);
        add_collision_vectors(&corner_2, &corner_2, position);
        add_collision_vectors(&corner_3, &corner_3, position);
    }

    collision = allocate_collision_obj();
    if (collision != 0) {
        build_col_shape_vertical_box_from_corners(
            &collision->shape, &corner_0, &corner_1,
            &corner_2, &corner_3);
        if ((int)collision->shape.type == 3) {
            half_height = 0.5f *
                (collision->shape.box_axis_0_max -
                 collision->shape.box_axis_0_min);
            center_x =
                collision->shape.box_axis_0.x * half_height +
                (0.5f * (collision->shape.box_corner_2.x -
                         collision->shape.box_corner_1.x) +
                 0.5f * (collision->shape.box_corner_0.x +
                         collision->shape.box_corner_1.x));
            center_z =
                collision->shape.box_axis_0.z * half_height +
                (0.5f * (collision->shape.box_corner_2.z -
                         collision->shape.box_corner_1.z) +
                 0.5f * (collision->shape.box_corner_0.z +
                         collision->shape.box_corner_1.z));
            for (index = 0; index < 4; index++) {
                dx = center_x - collision->shape.quad_vertices[index].value.x;
                dz = center_z - collision->shape.quad_vertices[index].value.z;
                distance = gxMathFastSqrt(dx * dx + dz * dz);
                distance = distance >= collision->shape.box_field_0x7C ?
                    distance : collision->shape.box_field_0x7C;
                collision->shape.box_field_0x7C = distance;
            }
        }
    }
    return collision;
}

/* TODO: [breakthrough] 74.46%; fast-sqrt, vertex predicate and component copies recovered;
 * edge-coordinate lifetimes, stack slots and FP scheduling remain. */
static CollisionObj* convert_cdf_triangle_to_collision_cylinder(
    const Vec* vertices, const Vec* angles, const Vec* position) {
    float length_squared;
    CollisionObj* collision;
    MKMATRIX matrix;
    Vec input;
    Vec center;
    float dx;
    float dz;
    float radius;
    float height;
    float top_height;
    int top_index;
    int next_index;
    int other_index;

    top_index = 0;
    top_height = vertices[0].y;
    if (vertices[1].y > top_height) {
        top_height = vertices[1].y;
        top_index = 1;
    }
    if (vertices[2].y > top_height) {
        top_index = 2;
    }

    next_index = (top_index + 1) % 3;
    other_index = (top_index + 2) % 3;
    dx = vertices[next_index].x - vertices[top_index].x;
    dz = vertices[next_index].z - vertices[top_index].z;
    if (!(dx * dx + dz * dz <
        (vertices[other_index].x - vertices[top_index].x) *
            (vertices[other_index].x - vertices[top_index].x) +
        (vertices[other_index].z - vertices[top_index].z) *
            (vertices[other_index].z - vertices[top_index].z))) {
        next_index = other_index;
    }

    dx = vertices[(top_index + 1) % 3].x -
         vertices[(top_index + 2) % 3].x;
    dz = vertices[(top_index + 1) % 3].z -
         vertices[(top_index + 2) % 3].z;
    length_squared = dx * dx + dz * dz;
    height = vertices[top_index].y - vertices[(top_index + 1) % 3].y;
    radius = gxMathFastSqrt(length_squared);

    gxVectCopy(&center, &vertices[next_index]);
    if (angles != 0) {
        YXZ_angles_to_MKMATRIX(angles, &matrix);
        gxVectCopy(&input, &center);
        v3_x_mat(&center, &input, &matrix);
    }
    if (position != 0) {
        center.x += position->x;
        center.y += position->y;
        center.z += position->z;
    }

    collision = allocate_collision_obj();
    if (collision != 0) {
        collision->shape.type = 2;
        collision->shape.cylinder_radius = radius;
        gxVectCopy(&collision->shape.cylinder_axis, &UNITVECT_Y);
        collision->shape.cylinder_height = height;
        gxVectCopy(&collision->shape.cylinder_center, &center);
    }
    return collision;
}

void insert_on_collision_obj_list(
    MkHdr* object, CollisionObjList* list) {
    mk_insert(object, &list->objects);
}

CollisionObjList* get_collision_obj_list(void) {
    CollisionObjList* result;

    result = (CollisionObjList*)get_mkhdr_generic(sizeof(CollisionObjList));
    if (result != 0) {
        result->objects = 0;
    }
    return result;
}

CollisionObj* get_collision_obj(void) {
    return allocate_collision_obj();
}

static int repel_a_from_b(
    CollisionShape* shape, const CollisionShape* obstacle, Vec* movement) {
    struct CollisionRepelInfo info;
    int shape_type;
    int obstacle_type;

    shape_type = shape->type & 7;
    obstacle_type = obstacle->type & 7;
    if (obstacle_type != 1) {
        if (shape_type == 2) {
            if (obstacle_type == 2) {
                info.second_movement = 0;
                info.moving_shape = 2;
                info.first_movement = movement;
                return repel_cylinders(
                    shape, (CollisionShape*)obstacle, &info);
            }
            if (obstacle_type == 4) {
                info.first_movement = movement;
                info.second_movement = 0;
                info.moving_shape = 2;
                return repel_cylinder_and_quad(shape, &obstacle->quad, &info, 0);
            }
            if (obstacle_type == 3) {
                info.second_movement = 0;
                info.moving_shape = 2;
                info.first_movement = movement;
                return repel_cylinder_and_box(shape, obstacle, &info);
            }
        } else if (shape_type == 3) {
            if (obstacle_type == 2) {
                info.first_movement = 0;
                info.second_movement = movement;
                info.moving_shape = 1;
                return repel_cylinder_and_box(
                    shape, obstacle, &info);
            }
        } else if (shape_type == 4) {
            if (obstacle_type == 2) {
                info.first_movement = 0;
                info.second_movement = movement;
                info.moving_shape = 1;
                return repel_cylinder_and_quad(
                    (CollisionShape*)obstacle, &shape->quad, &info, 0);
            }
        }
    }
    return 0;
}

/* TODO: [near miss] 91.77%; operations/CFG recovered; face/corner stack homes and scheduling remain. */
static int repel_cylinder_and_box(
    CollisionShape* cylinder, const CollisionShape* box,
    struct CollisionRepelInfo* info) {
    CollisionShape box_copy;
    CollisionQuad side;
    CollisionPaddedVec bottom_1;
    CollisionPaddedVec bottom_0;
    CollisionPaddedVec top_1;
    CollisionPaddedVec top_0;
    CollisionPaddedVec bottom_2;
    CollisionPaddedVec bottom_3;
    CollisionPaddedVec top_2;
    CollisionPaddedVec top_3;
    float retained_x;
    float retained_y;
    float retained_z;
    MKVECTOR top_offset;
    MKVECTOR movement;
    MKVECTOR point;
    Vec* saved_movement;
    const Vec* axis;
    float original_y;
    float epsilon;
    float extent;
    float center_x;
    float center_z;
    float dx;
    float dz;
    float distance;
    float nearest;
    float projection;
    float adjustment;
    int inside;
    int reverse;
    int retained;
    int result;
    int hit;

    retained = 0;
    if (box->box_field_0x7C > 0.0f) {
        center_x =
            0.5f * (box->box_corner_2.x - box->box_corner_1.x) +
            0.5f * (box->box_corner_0.x + box->box_corner_1.x);
        center_z =
            0.5f * (box->box_corner_2.z - box->box_corner_1.z) +
            0.5f * (box->box_corner_0.z + box->box_corner_1.z);
        dx = center_x - cylinder->cylinder_center.x;
        dz = center_z - cylinder->cylinder_center.z;
        distance = cylinder->cylinder_radius + box->box_field_0x7C + 0.01f;
        if (dx * dx + dz * dz >= distance * distance) {
            return 0;
        }
    }
    switch (info->moving_shape) {
    case 2:
        saved_movement = info->first_movement;
        gxVectCopy(&movement, saved_movement);
        info->first_movement = &movement;
        break;
    default:
        return 1;
    }
    retained_x = cylinder->cylinder_center.x;
    retained_y = cylinder->cylinder_center.y;
    retained_z = cylinder->cylinder_center.z;

    extent = box->box_axis_0_max - box->box_axis_0_min;
    top_offset.x = box->box_axis_0.x * extent;
    top_offset.y = box->box_axis_0.y * extent;
    top_offset.z = box->box_axis_0.z * extent;
    PSVECAdd(&box->box_corner_3, &top_offset, &top_3.value);
    PSVECAdd(&box->box_corner_2, &top_offset, &top_2.value);
    bottom_3 = box->quad_vertices[3];
    bottom_2 = box->quad_vertices[2];
    PSVECAdd(&box->box_corner_0, &top_offset, &top_0.value);
    PSVECAdd(&box->box_corner_1, &top_offset, &top_1.value);
    bottom_0 = box->quad_vertices[0];
    bottom_1 = box->quad_vertices[1];

    side.vertices[0] = bottom_1;
    side.vertices[1] = bottom_0;
    side.vertices[2] = top_0;
    side.vertices[3] = top_1;
    if (repel_cylinder_and_quad(cylinder, &side, info, 1) != 0) {
        retained_x = cylinder->cylinder_center.x;
        retained_y = cylinder->cylinder_center.y;
        retained_z = cylinder->cylinder_center.z;
        if (info->preserve_first_contact == 0) {
            retained = 1;
        }
        hit = 1;
    } else {
        hit = 0;
    }
    result = hit;

    side.vertices[0] = bottom_0;
    side.vertices[1] = bottom_3;
    side.vertices[2] = top_3;
    side.vertices[3] = top_0;
    if (repel_cylinder_and_quad(cylinder, &side, info, 1) != 0) {
        if (retained == 0) {
            retained_x = cylinder->cylinder_center.x;
            retained_y = cylinder->cylinder_center.y;
            retained_z = cylinder->cylinder_center.z;
            if (info->preserve_first_contact == 0) {
                retained = 1;
            }
        }
        hit = 1;
    } else {
        hit = 0;
    }
    result |= hit;

    side.vertices[0] = bottom_3;
    side.vertices[1] = bottom_2;
    side.vertices[2] = top_2;
    side.vertices[3] = top_3;
    if (repel_cylinder_and_quad(cylinder, &side, info, 1) != 0) {
        if (retained == 0) {
            retained_x = cylinder->cylinder_center.x;
            retained_y = cylinder->cylinder_center.y;
            retained_z = cylinder->cylinder_center.z;
            if (info->preserve_first_contact == 0) {
                retained = 1;
            }
        }
        hit = 1;
    } else {
        hit = 0;
    }
    result |= hit;

    side.vertices[0] = bottom_2;
    side.vertices[1] = bottom_1;
    side.vertices[2] = top_1;
    side.vertices[3] = top_2;
    if (repel_cylinder_and_quad(cylinder, &side, info, 1) != 0) {
        if (retained == 0) {
            retained_x = cylinder->cylinder_center.x;
            retained_y = cylinder->cylinder_center.y;
            retained_z = cylinder->cylinder_center.z;
        }
        hit = 1;
    } else {
        hit = 0;
    }
    result |= hit;

    cylinder->cylinder_center.x = retained_x;
    cylinder->cylinder_center.y = retained_y;
    cylinder->cylinder_center.z = retained_z;
    box_copy.box = box->box;
    box_copy.type = 3;
    point.x = cylinder->cylinder_center.x;
    original_y = cylinder->cylinder_center.y;
    point.y = original_y;
    point.z = cylinder->cylinder_center.z;
    v3_x_v_add_v3(
        &point, &UNITVECT_Y, 0.5f * cylinder->cylinder_height);

    inside = collision_point_inside_shape(&box_copy, &point);

    if (inside != 0) {
        epsilon = 0.01f;
        switch (box_copy.type & 7) {
        case 3:
            axis = &box_copy.box_axis_2;
            projection = axis->x * point.x + axis->y * point.y + axis->z * point.z;
            nearest = box_copy.box_axis_2_min - projection;
            adjustment = projection - box_copy.box_axis_2_max;
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
                epsilon = -epsilon;
            }
            parametric_ray_to_point(
                &cylinder->cylinder_center, &point, axis,
                nearest + epsilon);
            break;
        }
        cylinder->cylinder_center.y = original_y;
        result |= 1;
    }
    info->first_movement = saved_movement;
    return result;
}

static inline float collision_plane_penetration(
    float plane_distance, float center_plane, float radius, float scale) {
    float scaled_plane = plane_distance * scale;
    float scaled_center = center_plane * scale;
    return scaled_plane + radius - scaled_center;
}


static int repel_cylinder_and_quad(
    CollisionShape* cylinder, const CollisionQuad* quad,
    struct CollisionRepelInfo* info, int side_test) {
    MKVECTOR normal;
    MKVECTOR tangent;
    MKVECTOR direction;
    MKVECTOR edge_1;
    MKVECTOR edge_0;
    Vec* movement;
    float plane_distance;
    float center_plane;
    float moved_plane;
    float projection;
    float tangent_max;
    float height_max;
    float height_min;
    float tangent_min;
    float plane_delta;
    float edge_delta;
    float distance;
    float penetration;
    float value;
    int index;

    info->preserve_first_contact = 0;
    PSVECSubtract(&quad->vertices[1].value, &quad->vertices[0].value, &edge_0);
    PSVECSubtract(&quad->vertices[3].value, &quad->vertices[0].value, &edge_1);
    PSVECCrossProduct(&edge_0, &edge_1, &normal);
    PSVECNormalize(&normal, &normal);

    plane_distance = normal.x * quad->vertices[0].value.x +
        normal.y * quad->vertices[0].value.y +
        normal.z * quad->vertices[0].value.z;
    center_plane = normal.x * cylinder->cylinder_center.x +
        normal.y * cylinder->cylinder_center.y +
        normal.z * cylinder->cylinder_center.z;
    movement = info->first_movement;
    if (plane_distance >= center_plane + cylinder->cylinder_radius) {
        return 0;
    }
    if (plane_distance <= center_plane - cylinder->cylinder_radius) {
        return 0;
    }

    moved_plane = normal.x *
            (cylinder->cylinder_center.x - movement->x) +
        normal.y * (cylinder->cylinder_center.y - movement->y) +
        normal.z * (cylinder->cylinder_center.z - movement->z);
    if (plane_distance >= moved_plane + cylinder->cylinder_radius &&
        normal.x * movement->x + normal.z * movement->z >= 0.0f) {
        return 0;
    }

    PSVECCrossProduct(&normal, &UNITVECT_Y, &tangent);
    PSVECNormalize(&tangent, &tangent);
    value =
        tangent.x * quad->vertices[0].value.x +
        tangent.y * quad->vertices[0].value.y +
        tangent.z * quad->vertices[0].value.z;
    tangent_max = value;
    tangent_min = value;
    height_min = quad->vertices[0].value.y;
    height_max = quad->vertices[0].value.y;
    for (index = 1; index < 4; index++) {
        const Vec* vertex = &quad->vertices[index].value;
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
    if (cylinder->cylinder_center.y >= height_max) {
        return 0;
    }
    if (cylinder->cylinder_center.y + cylinder->cylinder_height <= height_min) {
        return 0;
    }

    projection = tangent.x * cylinder->cylinder_center.x +
        tangent.y * cylinder->cylinder_center.y +
        tangent.z * cylinder->cylinder_center.z;
    if (tangent_max <= projection - cylinder->cylinder_radius) {
        return 0;
    }
    if (tangent_min >= projection + cylinder->cylinder_radius) {
        return 0;
    }

    gxVectScale(&direction, &normal, 1.0f);
    direction.y = 0.0f;
    normalize_xz(&direction);
    if (tangent_max >= projection && tangent_min <= projection) {
        penetration = collision_plane_penetration(
            plane_distance, center_plane, cylinder->cylinder_radius, 1.0f);
    } else if (projection > tangent_max) {
        plane_delta = center_plane - plane_distance;
        edge_delta = projection - tangent_max;
        if (plane_delta <= 0.0f) {
            return 0;
        }
        distance = gxMathFastSqrt(
            plane_delta * plane_delta + edge_delta * edge_delta);
        if (distance >= cylinder->cylinder_radius) {
            return 0;
        }
        penetration = cylinder->cylinder_radius - distance;
    } else if (projection < tangent_min) {
        plane_delta = center_plane - plane_distance;
        edge_delta = tangent_min - projection;
        if (plane_delta <= 0.0f) {
            return 0;
        }
        distance = gxMathFastSqrt(
            plane_delta * plane_delta + edge_delta * edge_delta);
        if (distance >= cylinder->cylinder_radius) {
            return 0;
        }
        penetration = cylinder->cylinder_radius - distance;
    } else {
        return 1;
    }

    if (direction.x * movement->x + direction.z * movement->z > 0.0f) {
        info->preserve_first_contact = 1;
    } else {
        info->preserve_first_contact = 0;
    }
    xz_x_v_add_xz(
        &cylinder->cylinder_center, &direction, penetration + 0.001f);
    return 1;
}

/* TODO: [near miss] 99.88746%; typed cylinder snapshot and inline boundary agree; along-distance/clearance FPRs differ. */
static int repel_cylinders(
    CollisionShape* first, CollisionShape* second,
    struct CollisionRepelInfo* info) {
    CollisionShape* fixed;
    CollisionShape* moving;
    Vec* movement;
    CollisionCylinder expanded;
    Vec direction;
    Vec start;
    Vec separation;
    Vec offset;
    Vec normal;
    Vec remaining;
    float dx;
    float dz;
    float radius;
    float distance;
    float intersection;
    float remaining_distance;
    float projection;
    float turn_distance;
    float angle;

    dx = first->cylinder_center.x - second->cylinder_center.x;
    dz = first->cylinder_center.z - second->cylinder_center.z;
    radius = first->cylinder_radius + second->cylinder_radius;
    if (dx * dx + dz * dz >= radius * radius) {
        return 0;
    }
    if (first->cylinder_center.y + first->cylinder_height <=
            second->cylinder_center.y) {
        return 0;
    }
    if (second->cylinder_center.y + second->cylinder_height <=
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

    gxVectCopy(&direction, movement);
    normalize_xz(&direction);
    start.x = -1.01f * movement->x;
    start.y = -1.01f * movement->y;
    start.z = -1.01f * movement->z;
    add_collision_vectors(&start, &start, &moving->cylinder_center);

    expanded = fixed->cylinder;
    expanded.radius += moving->cylinder_radius;

    intersection = collision_ray_cylinder(&expanded, &start, &direction);

    if (intersection <= 0.0f) {
        separation.x = moving->cylinder_center.x - fixed->cylinder_center.x;
        separation.z = moving->cylinder_center.z - fixed->cylinder_center.z;
        distance = length_xz(&separation);
        if (distance == 0.0f) {
            separation.x = 0.0f;
            separation.z = radius;
        } else {
            scale_xz(&separation, &separation, radius / distance);
        }
        moving->cylinder_center.x = separation.x + fixed->cylinder_center.x;
        moving->cylinder_center.z = separation.z + fixed->cylinder_center.z;
    } else {
        parametric_ray_to_point(
            &moving->cylinder_center, &start, &direction, intersection);
        scale_v3(
            &remaining, &direction,
            length_v3(movement) - intersection);
        remaining_distance = length_xz(&remaining);

        offset.x = fixed->cylinder_center.x - moving->cylinder_center.x;
        offset.z = fixed->cylinder_center.z - moving->cylinder_center.z;
        normal.x = offset.x;
        normal.z = offset.z;
        normalize_xz(&normal);
        projection = normal.x * remaining.x + normal.z * remaining.z;
        turn_distance = gxMathFastSqrt(
            remaining_distance * remaining_distance -
            projection * projection);
        angle = gxMathArcTan(turn_distance / radius);
        if (offset.z * direction.x - offset.x * direction.z >= 0.0f) {
            rotate_xz(&offset, &offset, -angle);
        } else {
            rotate_xz(&offset, &offset, angle);
        }
        moving->cylinder_center.x = fixed->cylinder_center.x - offset.x;
        moving->cylinder_center.z = fixed->cylinder_center.z - offset.z;
    }
    return 1;
}

/* TODO: [breakthrough] 83.92%; axis construction, publication and half-extent math recovered;
 * retail axis-Y initialization evidence and corner staging remain. */
void build_col_shape_vertical_box(
    CollisionShape* shape,
    const Vec* center,
    float width,
    float height,
    float depth,
    float angle) {
    Vec axis_0_min_part;
    Vec axis_1_min_part;
    Vec axis_1_max_part;
    Vec axis_2_min_part;
    Vec axis_2_max_part;
    Vec axis_1;
    Vec axis_2;

    if (shape == 0) {
        return;
    }

    axis_2.x = gxMathSin(angle);
    axis_2.y = 0.0f;
    axis_2.z = gxMathCos(angle);
    axis_1.x = -axis_2.z;
    axis_1.y = 0.0f;
    axis_1.z = axis_2.x;
    shape->type = 3;
    shape->box_field_0x7C = 0.0f;
    shape->box_axis_2 = axis_2;
    shape->box_axis_1 = axis_1;
    shape->box_axis_0 = UNITVECT_Y;
    shape->box_axis_2_min = 0.5f * depth;
    shape->box_axis_2_max = 0.5f * -depth;
    shape->box_axis_0_min = height;
    shape->box_axis_0_max = 0.0f;
    shape->box_axis_1_min = 0.5f * -width;
    shape->box_axis_1_max = 0.5f * width;

    if (center != 0) {
        float axis_1_center;
        float axis_2_center;

        axis_2_center =
            center->x * axis_2.x + center->z * axis_2.z;
        shape->box_axis_2_min += axis_2_center;
        shape->box_axis_2_max += axis_2_center;
        shape->box_axis_0_min += center->y;
        shape->box_axis_0_max += center->y;
        axis_1_center =
            center->x * axis_1.x + center->z * axis_1.z;
        shape->box_axis_1_min += axis_1_center;
        shape->box_axis_1_max += axis_1_center;
    }

    axis_2_min_part.x = shape->box_axis_2.x * shape->box_axis_2_min;
    axis_2_min_part.y = shape->box_axis_2.y * shape->box_axis_2_min;
    axis_2_min_part.z = shape->box_axis_2.z * shape->box_axis_2_min;
    axis_2_max_part.x = shape->box_axis_2.x * shape->box_axis_2_max;
    axis_2_max_part.y = shape->box_axis_2.y * shape->box_axis_2_max;
    axis_2_max_part.z = shape->box_axis_2.z * shape->box_axis_2_max;
    axis_0_min_part.x = shape->box_axis_0.x * shape->box_axis_0_min;
    axis_0_min_part.y = shape->box_axis_0.y * shape->box_axis_0_min;
    axis_0_min_part.z = shape->box_axis_0.z * shape->box_axis_0_min;
    axis_1_min_part.x = shape->box_axis_1.x * shape->box_axis_1_min;
    axis_1_min_part.y = shape->box_axis_1.y * shape->box_axis_1_min;
    axis_1_min_part.z = shape->box_axis_1.z * shape->box_axis_1_min;
    axis_1_max_part.x = shape->box_axis_1.x * shape->box_axis_1_max;
    axis_1_max_part.y = shape->box_axis_1.y * shape->box_axis_1_max;
    axis_1_max_part.z = shape->box_axis_1.z * shape->box_axis_1_max;

    add_collision_vectors(
        &shape->box_corner_0, &axis_2_min_part, &axis_0_min_part);
    add_collision_vectors(
        &shape->box_corner_0, &shape->box_corner_0, &axis_1_max_part);
    add_collision_vectors(
        &shape->box_corner_1, &axis_2_min_part, &axis_0_min_part);
    add_collision_vectors(
        &shape->box_corner_1, &shape->box_corner_1, &axis_1_min_part);
    add_collision_vectors(
        &shape->box_corner_2, &axis_2_max_part, &axis_0_min_part);
    add_collision_vectors(
        &shape->box_corner_2, &shape->box_corner_2, &axis_1_min_part);
    add_collision_vectors(
        &shape->box_corner_3, &axis_2_max_part, &axis_0_min_part);
    add_collision_vectors(
        &shape->box_corner_3, &shape->box_corner_3, &axis_1_max_part);
}

static void build_col_shape_vertical_box_from_corners(
    CollisionShape* shape,
    const Vec* corner_0,
    const Vec* corner_1,
    const Vec* corner_2,
    const Vec* corner_3) {
    Vec* axis_2;
    Vec* axis_1;
    float delta_x;
    float delta_z;

    if (shape == 0 || corner_0 == 0 || corner_1 == 0 ||
        corner_2 == 0 || corner_3 == 0) {
        return;
    }

    shape->type = 3;
    shape->box_field_0x7C = 0.0f;
    shape->box_axis_0 = UNITVECT_Y;

    axis_2 = &shape->box_axis_2;
    delta_x = corner_2->x - corner_1->x;
    delta_z = corner_2->z - corner_1->z;
    axis_2->x = delta_z;
    axis_2->z = -delta_x;
    axis_2->y = 0.0f;
    normalize_xz(axis_2);
    shape->box_axis_2_min =
        corner_2->x * axis_2->x +
        corner_2->z * axis_2->z;
    shape->box_axis_2_max =
        corner_0->x * axis_2->x +
        corner_0->z * axis_2->z;

    axis_1 = &shape->box_axis_1;
    delta_x = corner_0->x - corner_1->x;
    delta_z = corner_0->z - corner_1->z;
    axis_1->x = delta_z;
    axis_1->z = -delta_x;
    axis_1->y = 0.0f;
    normalize_xz(axis_1);
    shape->box_axis_1_min =
        corner_1->x * axis_1->x +
        corner_1->z * axis_1->z;
    shape->box_axis_1_max =
        corner_2->x * axis_1->x +
        corner_2->z * axis_1->z;

    shape->box_axis_0_min = corner_2->y;
    shape->box_axis_0_max = corner_0->y;
    shape->box_corner_0.x = corner_2->x;
    shape->box_corner_0.y = corner_2->y;
    shape->box_corner_0.z = corner_2->z;
    shape->box_corner_1.x = corner_1->x;
    shape->box_corner_1.y = corner_1->y;
    shape->box_corner_1.z = corner_1->z;
    shape->box_corner_2.x = corner_0->x;
    shape->box_corner_2.z = corner_0->z;
    shape->box_corner_2.y = corner_3->y;
    shape->box_corner_3.x = corner_3->x;
    shape->box_corner_3.y = corner_3->y;
    shape->box_corner_3.z = corner_3->z;
}

void build_col_shape_vertical_cylinder(
    CollisionShape* shape,
    const Vec* center,
    float radius,
    float height) {
    shape->type = 2;
    shape->cylinder_radius = radius;
    shape->cylinder_axis.x = UNITVECT_Y.x;
    shape->cylinder_axis.y = UNITVECT_Y.y;
    shape->cylinder_axis.z = UNITVECT_Y.z;
    shape->cylinder_height = height;

    if (center != 0) {
        shape->cylinder_center.x = center->x;
        shape->cylinder_center.y = center->y;
        shape->cylinder_center.z = center->z;
    } else {
        shape->cylinder_center.x = 0.0f;
        shape->cylinder_center.y = 0.0f;
        shape->cylinder_center.z = 0.0f;
    }
}

/* TODO: [breakthrough] 85.26%; mutable shape aliasing recovered; inspect remaining transform-loop codegen. */
void render_col_shape(
    CollisionShape* shape, const unsigned int* color) {
    RwIm3DVertex vertices[16];
    RwIm3DVertex* vertex;
    const unsigned char* color_channels;
    unsigned char* vertex_colors;
    Vec radial;
    Vec transformed;
    int index;
    int vertex_offset;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    float angle;

    switch (shape->type & 7) {
    case 1:
        index = 0;
        vertex_offset = 0;
        color_channels = (const unsigned char*)color;
        green = color_channels[1];
        blue = color_channels[2];
        alpha = color_channels[3];
        red = color_channels[0];
        do {
            angle = 3.1415927f * ((float)index / 7.5f);
            transformed.x = gxMathCos(angle) * shape->sphere_radius;
            transformed.y = gxMathSin(angle) * shape->sphere_radius;
            transformed.z = 0.0f;
            gxVectCopy(&radial, &transformed);
            v3_x_mat_add_v3(
                &transformed, &radial, &inv_cam_rot_mat,
                &shape->sphere_center);
            index++;
            vertex = (RwIm3DVertex*)((unsigned char*)vertices +
                                            vertex_offset);
            vertex_colors = (unsigned char*)&vertex->color;
            vertex_colors[0] = red;
            vertex_offset += sizeof(RwIm3DVertex);
            vertex_colors[1] = green;
            vertex_colors[2] = blue;
            vertex_colors[3] = alpha;
            vertex->position.x = transformed.x;
            vertex->position.y = transformed.y;
            vertex->position.z = transformed.z;
        } while (index <= 15);
        if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
            RwIm3DRenderPrimitive(2);
            RwIm3DEnd();
        }
        break;
    case 3:
        render_col_shape_as_box(shape, color);
        break;
    case 4:
        render_col_shape_as_quad(shape, color);
        break;
    case 2:
        render_col_shape_as_cylinder(shape, color);
        break;
    }
}

/* TODO: [breakthrough] 59.31%; eight-Vec staging capacity restores frame;
 * indexed loop agrees; color snapshots and load scheduling remain. */
static void render_col_shape_as_quad(
    const CollisionShape* shape, const unsigned int* color) {
    Vec positions[8];
    const Vec* position;
    RwIm3DVertex vertices[8];
    RwIm3DVertex* vertex;
    int index;

    positions[0].x = shape->quad_vertex_0.x;
    positions[0].y = shape->quad_vertex_0.y;
    positions[0].z = shape->quad_vertex_0.z;
    positions[1].x = shape->quad_vertex_1.x;
    positions[1].y = shape->quad_vertex_1.y;
    positions[1].z = shape->quad_vertex_1.z;
    positions[2].x = shape->quad_vertex_2.x;
    positions[2].y = shape->quad_vertex_2.y;
    positions[2].z = shape->quad_vertex_2.z;
    positions[3].x = shape->quad_vertex_3.x;
    positions[3].y = shape->quad_vertex_3.y;
    positions[3].z = shape->quad_vertex_3.z;

    vertex = vertices;
    for (index = 0; index < 4; index++) {
        position = &positions[index];
        set_collision_vertex(vertex, position, color);
        vertex++;
    }
    if (RwIm3DTransform(vertices, 4, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, QuadVertexIndices, 8);
        RwIm3DEnd();
    }
}

/* TODO: [breakthrough] 89.53%; extrusion and vertex traversal recovered; corner staging saves one extra FPR. */
static void render_col_shape_as_box(
    const CollisionShape* shape, const unsigned int* color) {
    RwIm3DVertex vertices[8];
    Vec corners[8];
    float height;
    float extrusion_x;
    float extrusion_y;
    float extrusion_z;
    int index;
    RwIm3DVertex* vertex;

    height = shape->box_axis_0_max - shape->box_axis_0_min;
    extrusion_x = shape->box_axis_0.x * height;
    extrusion_y = shape->box_axis_0.y * height;
    extrusion_z = shape->box_axis_0.z * height;
    corners[0].x = shape->box_corner_3.x + extrusion_x;
    corners[0].y = shape->box_corner_3.y + extrusion_y;
    corners[0].z = shape->box_corner_3.z + extrusion_z;
    corners[1].x = shape->box_corner_2.x + extrusion_x;
    corners[1].y = shape->box_corner_2.y + extrusion_y;
    corners[1].z = shape->box_corner_2.z + extrusion_z;
    corners[2].x = shape->box_corner_3.x;
    corners[2].y = shape->box_corner_3.y;
    corners[2].z = shape->box_corner_3.z;
    corners[3].x = shape->box_corner_2.x;
    corners[3].y = shape->box_corner_2.y;
    corners[3].z = shape->box_corner_2.z;
    corners[4].x = shape->box_corner_0.x + extrusion_x;
    corners[4].y = shape->box_corner_0.y + extrusion_y;
    corners[4].z = shape->box_corner_0.z + extrusion_z;
    corners[5].x = shape->box_corner_1.x + extrusion_x;
    corners[5].y = shape->box_corner_1.y + extrusion_y;
    corners[5].z = shape->box_corner_1.z + extrusion_z;
    corners[6].x = shape->box_corner_0.x;
    corners[6].y = shape->box_corner_0.y;
    corners[6].z = shape->box_corner_0.z;
    corners[7].x = shape->box_corner_1.x;
    corners[7].y = shape->box_corner_1.y;
    corners[7].z = shape->box_corner_1.z;

    vertex = vertices;
    for (index = 0; index < 8; index++) {
        set_collision_vertex(vertex, &corners[index], color);
        vertex++;
    }

    if (RwIm3DTransform(vertices, 8, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, AtomicBBoxIndices, 24);
        RwIm3DEnd();
    }
}

/* TODO: [near miss] 91.92541%; geometry/frame agree; FP coloring, ring radius cache and loop-counter web remain. */
static void render_col_shape_as_cylinder(
    const CollisionShape* shape, const unsigned int* color) {
    RwIm3DVertex wire_vertices[8];
    RwIm3DVertex* wire_vertex;
    RwIm3DVertex vertices[16];
    Vec corners[8];
    Vec radial_0;
    Vec radial_1;
    Vec position;
    Vec axis_min;
    Vec axis_max;
    Vec radial_0_min;
    Vec radial_0_max;
    Vec radial_1_min;
    Vec radial_1_max;
    float length;
    float inverse_length;
    float projection;
    float cosine;
    float sine;
    float angle;
    float cosine_x;
    float cosine_y;
    float cosine_z;
    float sine_x;
    float sine_y;
    float sine_z;
    float cross_x;
    float cross_y;
    float cross_z;
    int index;
    int ring_index;
    int vertex_offset;
    RwIm3DVertex* vertex;
    unsigned char* vertex_colors;
    const unsigned char* channels;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;

    cross_x = shape->cylinder_axis.y * UNITVECT_Y.z -
        shape->cylinder_axis.z * UNITVECT_Y.y;
    cross_y = shape->cylinder_axis.z * UNITVECT_Y.x -
        shape->cylinder_axis.x * UNITVECT_Y.z;
    cross_z = shape->cylinder_axis.x * UNITVECT_Y.y -
        shape->cylinder_axis.y * UNITVECT_Y.x;
    wire_vertex = wire_vertices;
    projection = shape->cylinder_axis.z * shape->cylinder_center.z +
        (shape->cylinder_axis.x * shape->cylinder_center.x +
         shape->cylinder_axis.y * shape->cylinder_center.y);
    axis_min.x = shape->cylinder_axis.x * projection;
    axis_min.y = shape->cylinder_axis.y * projection;
    axis_min.z = shape->cylinder_axis.z * projection;
    projection += shape->cylinder_height;
    axis_max.x = shape->cylinder_axis.x * projection;
    axis_max.y = shape->cylinder_axis.y * projection;
    axis_max.z = shape->cylinder_axis.z * projection;

    length = gxMathFastSqrt(cross_z * cross_z +
        (cross_x * cross_x + cross_y * cross_y));
    inverse_length = length > 0.0f ? 1.0f / length : length;
    radial_0.x = cross_x * inverse_length;
    radial_0.y = cross_y * inverse_length;
    radial_0.z = cross_z * inverse_length;
    if (length == 0.0f) {
        radial_0 = UNITVECT_NEGX;
    }

    radial_1.x = radial_0.y * shape->cylinder_axis.z -
        radial_0.z * shape->cylinder_axis.y;
    radial_1.y = radial_0.z * shape->cylinder_axis.x -
        radial_0.x * shape->cylinder_axis.z;
    radial_1.z = radial_0.x * shape->cylinder_axis.y -
        radial_0.y * shape->cylinder_axis.x;
    projection = radial_0.z * shape->cylinder_center.z +
        (radial_0.x * shape->cylinder_center.x +
         radial_0.y * shape->cylinder_center.y);
    radial_0_max.x = radial_0.x * (projection + shape->cylinder_radius);
    radial_0_max.y = radial_0.y * (projection + shape->cylinder_radius);
    radial_0_max.z = radial_0.z * (projection + shape->cylinder_radius);
    radial_0_min.x = radial_0.x * (projection - shape->cylinder_radius);
    radial_0_min.y = radial_0.y * (projection - shape->cylinder_radius);
    radial_0_min.z = radial_0.z * (projection - shape->cylinder_radius);

    inverse_length = gxMathFastInvSqrt(radial_1.z * radial_1.z +
        (radial_1.x * radial_1.x + radial_1.y * radial_1.y));
    radial_1.x *= inverse_length;
    radial_1.y *= inverse_length;
    radial_1.z *= inverse_length;

    projection = radial_1.z * shape->cylinder_center.z +
        (radial_1.x * shape->cylinder_center.x +
         radial_1.y * shape->cylinder_center.y);
    radial_1_max.x = radial_1.x * (projection + shape->cylinder_radius);
    radial_1_max.y = radial_1.y * (projection + shape->cylinder_radius);
    radial_1_max.z = radial_1.z * (projection + shape->cylinder_radius);
    radial_1_min.x = radial_1.x * (projection - shape->cylinder_radius);
    radial_1_min.y = radial_1.y * (projection - shape->cylinder_radius);
    radial_1_min.z = radial_1.z * (projection - shape->cylinder_radius);

    add_collision_vectors(&corners[0], &axis_min, &radial_1_min);
    add_collision_vectors(&corners[0], &corners[0], &radial_0_max);
    add_collision_vectors(&corners[1], &axis_min, &radial_1_min);
    add_collision_vectors(&corners[1], &corners[1], &radial_0_min);
    add_collision_vectors(&corners[2], &axis_min, &radial_1_max);
    add_collision_vectors(&corners[2], &corners[2], &radial_0_max);
    add_collision_vectors(&corners[3], &axis_min, &radial_1_max);
    add_collision_vectors(&corners[3], &corners[3], &radial_0_min);
    add_collision_vectors(&corners[4], &axis_max, &radial_1_min);
    add_collision_vectors(&corners[4], &corners[4], &radial_0_max);
    add_collision_vectors(&corners[5], &axis_max, &radial_1_min);
    add_collision_vectors(&corners[5], &corners[5], &radial_0_min);
    add_collision_vectors(&corners[6], &axis_max, &radial_1_max);
    add_collision_vectors(&corners[6], &corners[6], &radial_0_max);
    add_collision_vectors(&corners[7], &axis_max, &radial_1_max);
    add_collision_vectors(&corners[7], &corners[7], &radial_0_min);

    for (index = 0; index < 8; index++) {
        set_collision_vertex(wire_vertex, &corners[index], color);
        wire_vertex++;
    }
    if (RwIm3DTransform(wire_vertices, 8, 0, 0) != 0) {
        RwIm3DRenderIndexedPrimitive(1, AtomicBBoxIndices, 24);
        RwIm3DEnd();
    }

    ring_index = 0;
    vertex_offset = 0;
    channels = (const unsigned char*)color;
    green = channels[1];
    blue = channels[2];
    alpha = channels[3];
    red = channels[0];
    do {
        angle = 3.1415927f * ((float)ring_index / 7.5f);
        gxMathCosSin(&cosine, &sine, angle);
        cosine = cosine * shape->cylinder_radius;
        sine = sine * shape->cylinder_radius;
        cosine_x = radial_0.x * cosine;
        cosine_y = radial_0.y * cosine;
        cosine_z = radial_0.z * cosine;
        sine_x = radial_1.x * sine;
        sine_y = radial_1.y * sine;
        sine_z = radial_1.z * sine;
        position.x = cosine_x + sine_x;
        position.y = cosine_y + sine_y;
        position.z = cosine_z + sine_z;
        v3_add_v3(
            &position, &position, &shape->cylinder_center);
        ring_index++;
        vertex = (RwIm3DVertex*)((unsigned char*)vertices + vertex_offset);
        vertex_colors = (unsigned char*)&vertex->color;
        vertex_colors[0] = red;
        vertex_offset += sizeof(RwIm3DVertex);
        vertex_colors[1] = green;
        vertex_colors[2] = blue;
        vertex_colors[3] = alpha;
        vertex->position.x = position.x;
        vertex->position.y = position.y;
        vertex->position.z = position.z;
    } while (ring_index <= 15);
    if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
        RwIm3DRenderPrimitive(2);
        RwIm3DEnd();
    }
}

float repel_check_plyrs(void) {
    unsigned int first_index;
    PlayerCollisionData* first_data;
    PlayerCollisionData* second_data;
    CollisionShape first_shape;
    CollisionShape second_shape;
    Vec difference;
    float penetration;
    float best;
    unsigned int second_index;

    best = 0.0f;
    first_data = g_game_info.plyr0.collision_data;
    second_data = g_game_info.plyr1.collision_data;
    if (first_data == 0 || second_data == 0) {
        return 0.0f;
    }
    if (second_data->joint_count != 0) {
        for (first_index = 0; first_index < first_data->active_count; first_index++) {
            first_shape = first_data->active_nodes[first_index].world_shape;
            first_shape.sphere_radius *= first_data->active_scale;
            for (second_index = 0;
                 second_index < second_data->active_count;
                 second_index++) {
                second_shape = second_data->active_nodes[second_index].world_shape;
                second_shape.sphere_radius *= second_data->active_scale;
                if (test_collision(&first_shape, &second_shape) == 1) {
                    v3_sub_v3(
                        &difference, &first_shape.sphere_center,
                        &second_shape.sphere_center);
                    penetration = first_shape.sphere_radius +
                        second_shape.sphere_radius - length_v3(&difference);
                    best = best >= penetration ? best : penetration;
                }
            }
        }
    }
    return best;
}

int collide_cylinder_vs_plyr(
    PlyrInfo* player, Vec* center, const Vec* angles,
    float radius, float height) {
    CollisionShape shape;

    shape.type = 2;
    shape.cylinder_radius = radius;
    shape.cylinder_height = height;
    if (center != 0) {
        gxVectCopy(&shape.cylinder_center, center);
    } else {
        shape.cylinder_center.x = 0.0f;
        shape.cylinder_center.y = 0.0f;
        shape.cylinder_center.z = 0.0f;
    }
    if (angles != 0) {
        uv_from_angles_xy(
            &shape.cylinder_axis, angles->x, angles->y);
    } else {
        gxVectCopy(&shape.cylinder_axis, &UNITVECT_Z);
    }
    return collide_shape_vs_plyr(player, &shape);
}

/* TODO: [near miss] 85.69%; aligned shape extent and CFG agree; center loads group before stores instead of interleaving. */
int collide_sphere_vs_plyr(
    PlyrInfo* player,
    const Vec* center,
    float radius) {
    CollisionShape shape;

    shape.type = 1;
    shape.sphere_radius = radius;
    if (center != 0) {
        gxVectCopy(&shape.sphere_center, center);
    } else {
        shape.sphere_center.z = 0.0f;
        shape.sphere_center.y = 0.0f;
        shape.sphere_center.x = 0.0f;
    }
    return collide_shape_vs_plyr(player, &shape);
}

int collide_shape_vs_plyr(
    PlyrInfo* player, const CollisionShape* shape) {
    PlayerCollisionData* collision;
    unsigned int region_index;

    collision = player->collision_data;
    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0) {
        if (collision->render_recorded != 0) {
            collision->recorded_count = 0;
            collision->render_recorded = 0;
        }
        collision->recorded_shapes[
            collision->recorded_count] = *shape;
        collision->recorded_count++;
        collision->render_recorded = 0;
    }

    if (collision->joint_count != 0U) {
        for (region_index = 0;
             region_index < collision->joint_count;
             region_index++) {
            if (test_collision(
                    shape,
                    &collision->joints[region_index].world_shape) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

int collide_plyr_vs_plyr(void) {
    unsigned int attack_index;
    unsigned int opponent_index;
    unsigned int saved_index;
    CollisionShape* attack_shape;
    PlayerCollisionData* opponent;
    PlyrInfo* player;
    PlayerCollisionData* collision;

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

    if (opponent->joint_count != 0) {
        for (attack_index = 0;
             attack_index < collision->field_93F4;
             attack_index++) {
            attack_shape = &collision->attacks[attack_index].world_shape;
            test_collision_vs_obstacles(
                player, attack_shape);
            if (local_collision_allowed(plyr_pdata) != 0) {
                for (opponent_index = 0;
                     opponent_index < opponent->joint_count;
                     opponent_index++) {
                    if (test_collision(
                            attack_shape,
                            &opponent->joints[opponent_index].world_shape) == 1) {
                        for (saved_index = 0;
                             saved_index < collision->field_93F8;
                             saved_index++) {
                            collision->saved_attacks[saved_index] =
                                collision->attacks[saved_index];
                        }
                        collision->field_93F4 = 0;
                        return 1;
                    }
                }
            }
        }
    }

    for (saved_index = 0; saved_index < collision->field_93F8; saved_index++) {
        collision->saved_attacks[saved_index] = collision->attacks[saved_index];
    }
    collision->field_93F4 = 0;
    return 0;
}

/* TODO: [near miss] 99.48%; object/shape homes fixed by decl order; result and shape-item GPRs (r24/r23) remain. */
static int test_collision_vs_obstacles(
    PlyrInfo* player, const CollisionShape* shape) {
    BgndObstacleEventData callback_data;
    MKVECTOR direction;
    int result;
    MkObj* object;
    MkPtr* next;
    MkPtr* shape_item;
    CollisionObj* collision_object;
    MkPtr* obstacle_item;
    ArenaObstacle* obstacle;
    MkHdr* obstacle_header;

    result = 0;
    object = player->slot.mirror_a;
    if (&constrain_info != 0) {
        obstacle_item = constrain_info.obstacles;
        while (obstacle_item != 0) {
            obstacle_header = obstacle_item->hdr;
            if (obstacle_item->instance != obstacle_header->instance) {
                next = obstacle_item->next;
                discard_stale_mkptr(obstacle_item);
                obstacle_item = next;
                continue;
            }
            obstacle = (ArenaObstacle*)obstacle_header;
            if (!obstacle->flags.bits.disabled && &obstacle->shapes != 0) {
                shape_item = obstacle->shapes;
                while (shape_item != 0) {
                    collision_object = (CollisionObj*)shape_item->hdr;
                    if (shape_item->instance !=
                        collision_object->hdr.instance) {
                        next = shape_item->next;
                        discard_stale_mkptr(shape_item);
                        shape_item = next;
                        continue;
                    }
                    if (test_collision(
                            shape, &collision_object->shape) != 0) {
                        if (constrain_info.callback != 0 &&
                            local_obstacle_callback(obstacle) != 0) {
                            xz_unit_vector_to_shape(
                                &direction, &collision_object->shape,
                                &object->pos.value);
                            callback_data.field_04 = obstacle->type;
                            callback_data.event_id = obstacle->obstacle_id;
                            callback_data.flags = 0;
                            callback_data.impact_vector = &direction;
                            direction.y = 0.0f;
                            callback_data.player_pdata = player->slot.pdata;
                            if (constrain_info.callback(
                                    &callback_data) != 0 &&
                                obstacle->hdr.instance != 0) {
                                ((struct CollisionObstacleVtable*)
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
    }
    return result;
}

int get_shape_center_for_collision_obstacle(
    CollisionObj* obstacle, Vec* center) {
    if (obstacle == 0) {
        return 0;
    }

    get_center_for_shape(&obstacle->shape, center);
    return 1;
}

int get_first_shape_center_for_obstacle_id(
    int obstacle_id, Vec* center) {
    CollisionObj* collision_object;
    ArenaObstacle* obstacle;
    MkPtr* obstacle_item;
    MkPtr* shape_item;
    MkPtr* shape_next;

    if (mklist_is_valid(&constrain_info.obstacles)) {
        obstacle_item = constrain_info.obstacles;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                obstacle_item = discard_stale_mkptr_and_advance(obstacle_item);
                continue;
            }
            if ((int)obstacle->obstacle_id == obstacle_id &&
                mklist_is_valid(&obstacle->shapes)) {
                shape_item = obstacle->shapes;
                while (shape_item != 0) {
                    collision_object = (CollisionObj*)shape_item->hdr;
                    if (shape_item->instance !=
                        collision_object->hdr.instance) {
                        shape_next = shape_item->next;
                        discard_stale_mkptr(shape_item);
                        shape_item = shape_next;
                        continue;
                    }
                    get_center_for_shape(
                        &collision_object->shape, center);
                    return 1;
                }
            }
            obstacle_item = obstacle_item->next;
        }
    }
    return 0;
}

static inline int collide_spheres(
    const CollisionShape* first, const CollisionShape* second) {
    float dy;
    float dx;
    float dz;
    float radius;

    dx = first->sphere_center.x - second->sphere_center.x;
    dy = first->sphere_center.y - second->sphere_center.y;
    dz = first->sphere_center.z - second->sphere_center.z;
    radius = first->sphere_radius + second->sphere_radius;
    if (dx * dx + dy * dy + dz * dz <= radius * radius) {
        return 1;
    }
    return 0;
}

static inline int collide_sphere_and_cylinder(
    const CollisionShape* sphere, const CollisionShape* cylinder) {
    float cross_x;
    float dz;
    float dy;
    float dx;
    float cross_y;
    float cross_z;
    float radius;
    float axial;

    dx = sphere->sphere_center.x - cylinder->cylinder_center.x;
    dy = sphere->sphere_center.y - cylinder->cylinder_center.y;
    dz = sphere->sphere_center.z - cylinder->cylinder_center.z;
    cross_x = dy * cylinder->cylinder_axis.z -
        dz * cylinder->cylinder_axis.y;
    cross_y = dz * cylinder->cylinder_axis.x -
        dx * cylinder->cylinder_axis.z;
    cross_z = dx * cylinder->cylinder_axis.y -
        dy * cylinder->cylinder_axis.x;
    radius = sphere->sphere_radius + cylinder->cylinder_radius;
    if (cross_x * cross_x + cross_y * cross_y + cross_z * cross_z >
        radius * radius) {
        return 0;
    }
    axial = dx * cylinder->cylinder_axis.x +
        dy * cylinder->cylinder_axis.y +
        dz * cylinder->cylinder_axis.z;
    if (axial < 0.0f) {
        if (-axial > sphere->sphere_radius) {
            return 0;
        }
    } else if (axial > cylinder->cylinder_height + sphere->sphere_radius) {
        return 0;
    }
    return 1;
}

static inline int collide_cylinder_and_box(
    const CollisionShape* cylinder, const CollisionShape* box) {
    return 0;
}

/* TODO: [near miss] 97.5%; MAP-backed inline predicates and ordered pair dispatch agree; two type-load slots and sphere FPR roles remain. */
static int test_collision(
    const CollisionShape* shape_a, const CollisionShape* shape_b) {
    int type_a;
    int type_b;

    type_a = shape_a->type & 7;
    type_b = shape_b->type & 7;
    if (type_a == 1) {
        if (type_b == 1) {
            return collide_spheres(shape_a, shape_b);
        } else if (type_b == 3) {
            return collide_sphere_and_box(shape_a, shape_b);
        } else if (type_b == 2) {
            return collide_sphere_and_cylinder(shape_a, shape_b);
        } else if (type_b == 4) {
            return collide_sphere_and_quad(shape_a, shape_b);
        }
    } else if (type_a == 2) {
        if (type_b == 1) {
            return collide_sphere_and_cylinder(shape_b, shape_a);
        } else if (type_b == 3) {
            return collide_cylinder_and_box(shape_a, shape_b);
        }
    } else if (type_a == 3) {
        if (type_b == 1) {
            return collide_sphere_and_box(shape_b, shape_a);
        } else if (type_b == 2) {
            return collide_cylinder_and_box(shape_b, shape_a);
        }
    } else if (type_a == 4) {
        if (type_b == 1) {
            return collide_sphere_and_quad(shape_b, shape_a);
        }
    }
    return 0;
}

static int collide_sphere_and_box(
    const CollisionShape* sphere, const CollisionShape* box) {
    return collision_point_inside_shape(box, &sphere->sphere_center);
}

int is_point_inside_shape(
    const CollisionShape* shape, const Vec* point) {
    return collision_point_inside_shape(shape, point);
}

/* TODO: [breakthrough] 87.47%; aligned frame and plane predicates recovered; cross-product staging remains. */
static int collide_sphere_and_quad(
    const CollisionShape* sphere, const CollisionShape* quad) {
    Vec normal __attribute__((aligned(16)));
    Vec inward __attribute__((aligned(16)));
    Vec edge_1 __attribute__((aligned(16)));
    Vec edge_0 __attribute__((aligned(16)));
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
    if (quad_plane > sphere_plane + sphere->sphere_radius) {
        return 0;
    }
    if (quad_plane < sphere_plane - sphere->sphere_radius) {
        return 0;
    }

#define TEST_SPHERE_QUAD_EDGE(first, second) \
    do { \
        edge_0.x = (second)->x - (first)->x; \
        edge_0.y = (second)->y - (first)->y; \
        edge_0.z = (second)->z - (first)->z; \
        inward.x = normal.y * edge_0.z - normal.z * edge_0.y; \
        inward.y = normal.z * edge_0.x - normal.x * edge_0.z; \
        inward.z = normal.x * edge_0.y - normal.y * edge_0.x; \
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
#undef TEST_SPHERE_QUAD_EDGE
    edge_0.x = quad->quad_vertex_0.x - quad->quad_vertex_3.x;
    edge_0.y = quad->quad_vertex_0.y - quad->quad_vertex_3.y;
    edge_0.z = quad->quad_vertex_0.z - quad->quad_vertex_3.z;
    inward.x = normal.y * edge_0.z - normal.z * edge_0.y;
    inward.y = normal.z * edge_0.x - normal.x * edge_0.z;
    inward.z = normal.x * edge_0.y - normal.y * edge_0.x;
    normalize_v3(&inward);
    return !(inward.x * sphere->sphere_center.x +
        inward.y * sphere->sphere_center.y +
        inward.z * sphere->sphere_center.z >
        inward.x * quad->quad_vertex_3.x +
        inward.y * quad->quad_vertex_3.y +
        inward.z * quad->quad_vertex_3.z);
}

/* TODO: [near miss] 99.09%; box operations and order agree; stop at localized FPR coloring. */
static void get_center_for_shape(const CollisionShape* shape, Vec* center) {
    int kind;

    kind = shape->type & 7;
    if (kind == 2) {
        center->x = shape->cylinder_center.x;
        center->y = shape->cylinder_center.y;
        center->z = shape->cylinder_center.z;
        v3_x_v_add_v3(
            center, &shape->cylinder_axis, 0.5f * shape->cylinder_height);
    } else if (kind == 3) {
        Vec base_center;
        float half_height;

        base_center.x = shape->box_corner_0.x;
        base_center.x = 0.5f * (base_center.x + shape->box_corner_1.x);
        base_center.y = 0.5f * (shape->box_corner_0.y + shape->box_corner_1.y);
        base_center.z = 0.5f * (shape->box_corner_0.z + shape->box_corner_1.z);
        half_height = shape->box_axis_0_max - shape->box_axis_0_min;
        half_height = 0.5f * half_height;
        base_center.x +=
            0.5f * (shape->box_corner_2.x - shape->box_corner_1.x);
        base_center.y +=
            0.5f * (shape->box_corner_2.y - shape->box_corner_1.y);
        base_center.z +=
            0.5f * (shape->box_corner_2.z - shape->box_corner_1.z);
        center->x = shape->box_axis_0.x * half_height + base_center.x;
        center->y = shape->box_axis_0.y * half_height + base_center.y;
        center->z = shape->box_axis_0.z * half_height + base_center.z;
    } else if (kind == 4) {
        center->x = shape->quad_vertex_0.x;
        center->y = shape->quad_vertex_0.y;
        center->z = shape->quad_vertex_0.z;
        center->x += shape->quad_vertex_1.x;
        center->y += shape->quad_vertex_1.y;
        center->z += shape->quad_vertex_1.z;
        center->x += shape->quad_vertex_2.x;
        center->y += shape->quad_vertex_2.y;
        center->z += shape->quad_vertex_2.z;
        center->x += shape->quad_vertex_3.x;
        center->y += shape->quad_vertex_3.y;
        center->z += shape->quad_vertex_3.z;
        center->x = 0.25f * center->x;
        center->y = 0.25f * center->y;
        center->z = 0.25f * center->z;
    } else if (kind == 1) {
        center->x = shape->sphere_center.x;
        center->y = shape->sphere_center.y;
        center->z = shape->sphere_center.z;
    }
}

/* TODO: [breakthrough needed] 70.80%; edge stack extent and box-center FP staging differ. */
static void xz_unit_vector_to_shape(
    Vec* result, const CollisionShape* shape, const Vec* point) {
    Vec edge_0;
    Vec edge_1;
    float half_height;
    float center_x;
    float center_z;
    int type;

    type = shape->type & 7;
    if (type == 2) {
        result->x = shape->cylinder_center.x - point->x;
        result->z = shape->cylinder_center.z - point->z;
    } else if (type == 3) {
        center_x = 0.5f *
            (shape->box_corner_0.x + shape->box_corner_1.x);
        center_x += 0.5f *
            (shape->box_corner_2.x - shape->box_corner_1.x);
        half_height =
            0.5f * (shape->box_axis_0_max - shape->box_axis_0_min);
        result->x =
            shape->box_axis_0.x * half_height + center_x - point->x;
        center_z = 0.5f *
            (shape->box_corner_0.z + shape->box_corner_1.z);
        center_z += 0.5f *
            (shape->box_corner_2.z - shape->box_corner_1.z);
        result->z =
            shape->box_axis_0.z * half_height + center_z - point->z;
    } else if (type == 4) {
        PSVECSubtract(
            &shape->quad_vertex_1, &shape->quad_vertex_0, &edge_0);
        PSVECSubtract(
            &shape->quad_vertex_3, &shape->quad_vertex_0, &edge_1);
        PSVECCrossProduct(&edge_0, &edge_1, result);
        PSVECNormalize(result, result);
        if (result->x * (shape->quad_vertex_0.x - point->x) +
                result->z * (shape->quad_vertex_0.z - point->z) <
            0.0f) {
            result->x = -result->x;
            result->z = -result->z;
        }
    } else {
        return;
    }

    normalize_xz(result);
    result->y = 0.0f;
}

void render_collision_regions(void) {
    CollisionObjList* shadow_list;
    MKMATRIX camera_matrix;
    MkPtr* item;
    MkPtr* next;
    int state_8;
    int state_6;
    int state_1;
    int saved_state_1;
    int saved_state_6;
    int saved_state_8;

    if ((g_game_info.pause_flags & 1) == 0) {
        return;
    }

    RwMatrixInvert(
        &camera_matrix,
        &Camera->viewMatrix);
    RwMatrixOrthoNormalize(&inv_cam_rot_mat, &camera_matrix);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(1, &state_1);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(6, &state_6);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(8, &state_8);
    set_render_state(1, 0);
    set_render_state(6, 0);
    set_render_state(8, 0);
    saved_state_1 = state_1;
    saved_state_6 = state_6;
    saved_state_8 = state_8;

    if (g_game_info.switch_input_flags.view_danger_zones) {
        if (g_game_info.plyr0.collision_data != 0) {
            g_game_info.plyr0.collision_data->render_recorded = 1;
        }
        if (g_game_info.plyr1.collision_data != 0) {
            g_game_info.plyr1.collision_data->render_recorded = 1;
        }
        apply_to_mklist(
            (MkListApplyFn)render_bgnd_danger_zone_obstacle,
            &constrain_info.obstacles);
    } else {
        if (!g_game_info.switch_input_flags.field_bit5) {
            render_players_joints();
        }
        if (!g_game_info.switch_input_flags.hide_obstacles) {
            render_background_danger_areas();
            apply_to_mklist(
                (MkListApplyFn)render_obstacle,
                &constrain_info.obstacles);
        }
        if (mode_of_play == 7 && !g_game_info.switch_input_flags.hide_konquest_collision) {
            render_hero_collision();
            apply_to_mklist(
                render_konquest_collision_obj, &global_collision_list);
            if (mklist_is_valid(&konquest_shadow_collision_lists)) {
                item = konquest_shadow_collision_lists;
                while (item != 0) {
                    shadow_list = (CollisionObjList*)item->hdr;
                    if (item->instance != shadow_list->hdr.instance) {
                        next = item->next;
                        discard_stale_mkptr(item);
                        item = next;
                        continue;
                    }
                    apply_to_mklist(
                        render_konquest_shadow_objects,
                        &shadow_list->objects);
                    item = item->next;
                }
            }
        }
    }
    set_render_state(1, saved_state_1);
    set_render_state(6, saved_state_6);
    set_render_state(8, saved_state_8);
}

static inline void collision_render_sphere_outline(
    CollisionShape* shape, const unsigned int* color) {
    RwIm3DVertex vertices[16];
    RwIm3DVertex* vertex;
    const unsigned char* color_channels;
    unsigned char* vertex_colors;
    Vec transformed;
    Vec radial;
    int index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    int vertex_offset;
    float angle;
    index = 0;
    vertex_offset = 0;
    color_channels = (const unsigned char*)color;
    red = color_channels[0];
    green = color_channels[1];
    blue = color_channels[2];
    alpha = color_channels[3];
    do {
        angle = 3.1415927f * ((float)index / 7.5f);
        transformed.x = shape->sphere_radius *
            gxMathCos(angle);
        transformed.y = shape->sphere_radius *
            gxMathSin(angle);
        transformed.z = 0.0f;
        gxVectCopy(&radial, &transformed);
        v3_x_mat_add_v3(
            &transformed, &radial, &inv_cam_rot_mat,
            &shape->sphere_center);
        index++;
        vertex = (RwIm3DVertex*)((unsigned char*)vertices +
                                        vertex_offset);
        vertex_colors = (unsigned char*)&vertex->color;
        vertex_colors[0] = red;
        vertex_offset += sizeof(RwIm3DVertex);
        vertex_colors[1] = green;
        vertex_colors[2] = blue;
        vertex_colors[3] = alpha;
        vertex->position.x = transformed.x;
        vertex->position.y = transformed.y;
        vertex->position.z = transformed.z;
    } while (index <= 15);
    if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
        RwIm3DRenderPrimitive(2);
        RwIm3DEnd();
    }
}

/* TODO: [near miss] 87.66393%; sphere operations and Vec homes agree; GPR rotation and preheader scheduling remain. */
static void render_hero_collision(void) {
    switch (konquest_hero_collision_shape.type & 7) {
    case 1:
        collision_render_sphere_outline(&konquest_hero_collision_shape, &rgba_white);
        break;
    case 3:
        render_col_shape_as_box(&konquest_hero_collision_shape, &rgba_white);
        break;
    case 4:
        render_col_shape_as_quad(&konquest_hero_collision_shape, &rgba_white);
        break;
    case 2:
        render_col_shape_as_cylinder(&konquest_hero_collision_shape, &rgba_white);
        break;
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

static inline void render_col_shape_as_sphere(
    CollisionShape* shape, const unsigned int* color) {
    RwIm3DVertex vertices[16];
    Vec transformed;
    Vec radial;
    const unsigned char* channels = (const unsigned char*)color;
    int red;
    int green;
    int blue;
    int alpha;
    int index;
    RwIm3DVertex* vertex;
    float angle;

    index = 0;
    red = channels[0];
    green = channels[1];
    blue = channels[2];
    alpha = channels[3];
    do {
        angle = 3.1415927f * ((float)index / 7.5f);
        transformed.x = shape->sphere_radius * gxMathCos(angle);
        transformed.y = shape->sphere_radius * gxMathSin(angle);
        transformed.z = 0.0f;
        gxVectCopy(&radial, &transformed);
        v3_x_mat_add_v3(
            &transformed, &radial, &inv_cam_rot_mat, &shape->sphere_center);
        vertex = &vertices[index];
        index++;
        vertex->color_channels.red = red;
        vertex->color_channels.green = green;
        vertex->color_channels.blue = blue;
        vertex->color_channels.alpha = alpha;
        vertex->position.x = transformed.x;
        vertex->position.y = transformed.y;
        vertex->position.z = transformed.z;
    } while (index <= 15);
    if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
        RwIm3DRenderPrimitive(2);
        RwIm3DEnd();
    }
}

static inline void render_player_collision_shape(
    CollisionShape* shape, const unsigned int* color) {
    switch (shape->type & 7) {
    case 1:
        render_col_shape_as_sphere(shape, color);
        break;
    case 3:
        render_col_shape_as_box(shape, color);
        break;
    case 4:
        render_col_shape_as_quad(shape, color);
        break;
    case 2:
        render_col_shape_as_cylinder(shape, color);
        break;
    }
}

/* TODO: [near miss] 88.09%; frame, calls and stores agree; palette/helper register ownership remains. */
static void render_player_joints(PlayerCollisionData* collision) {
    CollisionShape shape;
    unsigned int index;


    if (collision->joints != 0) {
        for (index = 0; index < collision->joint_count; index++) {
            render_player_collision_shape(
                &collision->joints[index].world_shape, &rgba_yellow);
        }

        for (index = 0; index < collision->active_count; index++) {
            shape = collision->active_nodes[index].world_shape;
            shape.sphere_radius *= collision->active_scale;
            render_player_collision_shape(&shape, &rgba_blue);
        }

        if (collision->recorded_count != 0) {
            collision->render_recorded = 1;
            for (index = 0; index < collision->recorded_count; index++) {
                render_player_collision_shape(
                    &collision->recorded_shapes[index], &rgba_red);
            }
        }
    }
    render_player_collision_shape(
        &collision->body_shape,
        &rgba_green);
}

/* TODO: [near miss] 87.93%; sphere operations agree; pre-loop scheduling and GPR ownership remain. */
static void render_konquest_shadow_objects(MkHdr* hdr) {
    RwIm3DVertex vertices[16];
    CollisionObj* object;
    Vec radial;
    Vec transformed;
    float angle;
    int index;
    const unsigned char* channels;
    int red;
    int green;
    int blue;
    int alpha;

    object = (CollisionObj*)hdr;
    switch (object->shape.type & 7) {
    case 1:
        channels = (const unsigned char*)&rgba_yellow;
        red = channels[0];
        green = channels[1];
        blue = channels[2];
        alpha = channels[3];
        for (index = 0; index <= 15; index++) {
            angle = 3.1415927f * ((float)index / 7.5f);
            transformed.x = object->shape.sphere_radius * gxMathCos(angle);
            transformed.y = object->shape.sphere_radius * gxMathSin(angle);
            transformed.z = 0.0f;
            radial.x = transformed.x;
            radial.y = transformed.y;
            radial.z = transformed.z;
            v3_x_mat_add_v3(
                &transformed, &radial, &inv_cam_rot_mat,
                &object->shape.sphere_center);
            vertices[index].color_channels.red = red;
            vertices[index].color_channels.green = green;
            vertices[index].color_channels.blue = blue;
            vertices[index].color_channels.alpha = alpha;
            vertices[index].position.x = transformed.x;
            vertices[index].position.y = transformed.y;
            vertices[index].position.z = transformed.z;
        }
        if (RwIm3DTransform(vertices, 16, 0, 2) != 0) {
            RwIm3DRenderPrimitive(2);
            RwIm3DEnd();
        }
        break;
    case 3:
        render_col_shape_as_box(&object->shape, &rgba_yellow);
        break;
    case 4:
        render_col_shape_as_quad(&object->shape, &rgba_yellow);
        break;
    case 2:
        render_col_shape_as_cylinder(&object->shape, &rgba_yellow);
        break;
    }
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

#define DEFINE_COLLISION_OBJECT_RENDERER(function_name, render_color) \
    static void function_name(MkHdr* hdr) { \
        RwIm3DVertex vertices[16]; \
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
                radial.x = shape->sphere_radius * gxMathCos(angle); \
                radial.y = shape->sphere_radius * gxMathSin(angle); \
                radial.z = 0.0f; \
                v3_x_mat_add_v3( \
                    &transformed, &radial, &inv_cam_rot_mat, \
                    &shape->sphere_center); \
                set_collision_vertex( \
                    &vertices[index], &transformed, &(render_color)); \
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

/* TODO: [breakthrough needed] 79.02%; per-pass frame and dispatch recovered; signed channel header adds conversions. */
static void render_konquest_collision_obj(MkHdr* hdr) {
    RwIm3DVertex vertices[3][16];
    CollisionObj* object;
    CollisionShape* shape;
    float angle;
    int index;

    object = (CollisionObj*)hdr;
    shape = &object->shape;

#define RENDER_KONQUEST_SHAPE(render_color, vertex_set) \
    do { \
        switch ((int)(shape->type & 7)) { \
        case 1: { \
            Vec radial; \
            Vec transformed; \
            const unsigned char* channels = (const unsigned char*)&(render_color); \
            unsigned char red = channels[0]; \
            unsigned char green = channels[1]; \
            unsigned char blue = channels[2]; \
            unsigned char alpha = channels[3]; \
            for (index = 0; index < 16; index++) { \
                angle = 3.1415927f * ((float)index / 7.5f); \
                radial.x = shape->sphere_radius * gxMathCos(angle); \
                radial.y = shape->sphere_radius * gxMathSin(angle); \
                radial.z = 0.0f; \
                v3_x_mat_add_v3( \
                    &transformed, &radial, &inv_cam_rot_mat, \
                    &shape->sphere_center); \
                (vertex_set)[index].color_channels.red = red; \
                (vertex_set)[index].color_channels.green = green; \
                (vertex_set)[index].color_channels.blue = blue; \
                (vertex_set)[index].color_channels.alpha = alpha; \
                (vertex_set)[index].position.x = transformed.x; \
                (vertex_set)[index].position.y = transformed.y; \
                (vertex_set)[index].position.z = transformed.z; \
            } \
            if (RwIm3DTransform((vertex_set), 16, 0, 2) != 0) { \
                RwIm3DRenderPrimitive(2); \
                RwIm3DEnd(); \
            } \
            break; \
        } \
        case 3: \
            render_col_shape_as_box(shape, &(render_color)); \
            break; \
        case 4: \
            render_col_shape_as_quad(shape, &(render_color)); \
            break; \
        case 2: \
            render_col_shape_as_cylinder(shape, &(render_color)); \
            break; \
        } \
    } while (0)

    if ((int)(shape->type & 7) == 2) {
        RENDER_KONQUEST_SHAPE(rgba_cyan, vertices[2]);
    } else if ((int)(shape->type & 7) == 4) {
        RENDER_KONQUEST_SHAPE(rgba_green, vertices[1]);
    } else {
        RENDER_KONQUEST_SHAPE(rgba_blue, vertices[0]);
    }
#undef RENDER_KONQUEST_SHAPE
}

DEFINE_COLLISION_OBJECT_RENDERER(render_collision_obj, rgba_blue)
#undef DEFINE_COLLISION_OBJECT_RENDERER

/* TODO: [near miss] 99.65%; collision-owner, scratch-address and loop-index GPR coloring remains; stop without a new structural lead. */
void set_plyr_attack_region(
    int use_body, float radius, float extension) {
    PlyrMirrorSlots* mirror_slots;
    MkObj* weapon_0;
    MkObj* weapon_1;
    int index;
    Vec difference;
    PlayerCollisionNode interpolated;
    PlayerCollisionData* collision;

    collision = plyr_pdata->plyr_info->collision_data;
    if ((g_game_info.pause_flags & 1) != 0 &&
        g_game_info.switch_input_flags.field_bit5 == 0) {
        collision->render_recorded = 0;
        collision->recorded_count = 0;
    }

    if (use_body != 0) {
        add_plyr_body_attack_nodes(use_body, radius, extension);
    } else {
        mirror_slots = plyr_pdata->mirror_slots;
        if (mirror_slots != 0) {
            weapon_0 = MK_HDR_LIVE(
                mirror_slots->weapon[0].primary.obj,
                mirror_slots->weapon[0].primary.instance);
            weapon_1 = MK_HDR_LIVE(
                mirror_slots->weapon[1].primary.obj,
                mirror_slots->weapon[1].primary.instance);
            if (weapon_0 != 0 || weapon_1 != 0) {
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
    }

    if (collision->field_93F8 == 0) {
        for (index = 0; index < (int)collision->field_93F4; index++) {
            collision->saved_attacks[index] = collision->attacks[index];
        }
    }
    collision->field_93F8 = collision->field_93F4;

    for (index = 0; index < (int)collision->field_93F8; index++) {
        CollisionShape* current = &collision->attacks[index].world_shape;
        CollisionShape* previous = &collision->saved_attacks[index].world_shape;
        v3_sub_v3(
            &difference, &current->sphere_center, &previous->sphere_center);
        if (length_v3(&difference) >
            2.0f * current->sphere_radius) {
            interpolated.world_shape.sphere_center =
                previous->sphere_center;
            interpolated.world_shape.sphere_radius =
                previous->sphere_radius;
            interpolated.world_shape.type = 1;
            v3_x_v_add_v3(
                &interpolated.world_shape.sphere_center,
                &difference, 0.5f);
            insert_player_attack_node_unshifted(
                plyr_pdata->plyr_info->collision_data, &interpolated);
        }
    }
}

/* TODO: [breakthrough needed] 85.35%; direct-index insertion restores copy base; transform lifetimes and FP staging remain. */
static void add_plyr_body_attack_nodes(
    int region_id, float radius, float extension) {
    PlayerCollisionData* storage;
    PlayerCollisionNode node;
    CollisionShape local_shape;
    PlayerCollisionNode* inserted[36];
    const struct CollisionNodeDef* definition;
    const int* entries;
    PlyrInfo* player;
    PlyrPdata* fighter;
    MkObj* object;
    Vec movement;
    Vec difference;
    float attack_radius;
    float facing_angle;
    float distance;
    float scale;
    int definition_count;
    int definition_index;
    int inserted_count;
    int node_id;

    entries = attack_region_list[region_id];
    player = plyr_pdata->plyr_info;
    storage = player->collision_data;
    object = storage->object;
    attack_radius = storage->attack_radius;
    facing_angle = object->ang.y;
    movement.x = gxMathSin(facing_angle);
    movement.y = 0.0f;
    movement.z = gxMathCos(facing_angle);
    movement.x *= attack_radius;
    movement.z *= attack_radius;
    player = plyr_pdata->plyr_info;
    if (player == 0) {
        return;
    }
    fighter = player->slot.pdata;
    if (fighter != 0) {
        definition_count = fighter->character_id == 0x1E ? 28 : 22;
    } else {
        definition_count = 0;
    }

    inserted_count = 0;
    while (*entries != 0) {
        node_id = *entries;
        definition_index = 0;
        while (definition_index < definition_count) {
            if ((node_id | 0x1000) ==
                    col_def_list[definition_index].node_id ||
                (node_id | 0x4000) ==
                    col_def_list[definition_index].node_id) {
                break;
            }
            definition_index++;
        }
        if (definition_index == definition_count) {
            return;
        }
        if (am_i_flipped() != 0) {
            if (col_def_list[definition_index].side == 1U) {
                definition_index++;
            } else if (col_def_list[definition_index].side == 2U) {
                definition_index--;
            }
        }
        definition = &col_def_list[definition_index];

        local_shape.type = 1;
        local_shape.sphere_center.x = 0.0f;
        local_shape.sphere_center.y = 0.0f;
        local_shape.sphere_center.z = 0.0f;
        local_shape.sphere_radius =
            radius * definition->attack_radius_scale;
        object = plyr_pdata->plyr_info->collision_data->object;
        if (object != 0) {
            node.bone = object->bones[definition->node_id & 0xFFF];
            node.local_shape = local_shape;
            node.world_shape = local_shape;
            transform_collision_node(&node);
        }

        storage = plyr_pdata->plyr_info->collision_data;
        inserted[inserted_count] =
            insert_player_attack_node(storage, &node, &movement);
        if (inserted[inserted_count] != 0) {
            inserted_count++;
        }

        if (entries[1] <= 0 && extension != 0.0f && inserted_count > 1) {
            PlayerCollisionNode* previous;
            PlayerCollisionNode* current;

            previous = inserted[inserted_count - 2];
            current = inserted[inserted_count - 1];
            v3_sub_v3(
                &difference, &current->world_shape.sphere_center,
                &previous->world_shape.sphere_center);
            distance = length_v3(&difference);
            scale = distance;
            if (distance) {
                scale = (distance + extension) / distance;
            }
            node.world_shape.sphere_center =
                previous->world_shape.sphere_center;
            node.world_shape.sphere_radius =
                radius * definition->attack_radius_scale;
            node.world_shape.type = 1;
            v3_x_v_add_v3(
                &node.world_shape.sphere_center, &difference, scale);
            storage = plyr_pdata->plyr_info->collision_data;
            insert_player_attack_node_unshifted(storage, &node);
        }

        if (entries[1] < 0) {
            inserted_count = 0;
            entries += 2;
        } else {
            entries++;
        }
    }
}

/* TODO: [breakthrough needed] 84.43%; template frame and owner boundaries recovered; offset-copy guard and insertion CFG remain. */
static void generate_weapon_collision_nodes(
    PlayerCollisionData* collision_data, MkObj* weapon, float radius) {
    PlayerCollisionData* storage;
    PlayerCollisionNode node;
    CollisionShape local_shape;
    PlayerCollisionNode* first;
    PlayerCollisionNode* second;
    struct WeaponCollisionDef definition;
    MkBone* bone;
    float attack_radius;
    float facing_angle;
    Vec movement;
    Vec difference;
    unsigned int first_index;
    unsigned int last_pair_index;
    unsigned int index;

    storage = collision_data;
    first_index = storage->field_93F4;
    update_bone_hierarchy(
        weapon != 0 ? as_mkhdr(&weapon->hdr) : 0);
    get_weapon_collision_def(weapon, &definition);

    storage = plyr_pdata->plyr_info->collision_data;
    attack_radius = storage->attack_radius;
    facing_angle = storage->object->ang.y;
    movement.x = gxMathSin(facing_angle);
    movement.y = 0.0f;
    movement.z = gxMathCos(facing_angle);
    movement.x *= attack_radius;
    movement.z *= attack_radius;

    bone = 0;
    for (index = 0; index < weapon->bone_count; index++) {
        local_shape.type = 1;
        local_shape.sphere_center.x = 0.0f;
        local_shape.sphere_center.y = 0.0f;
        local_shape.sphere_center.z = 0.0f;
        local_shape.sphere_radius = radius * definition.radius;
        bone = weapon->bones[index];
        node.bone = bone;
        node.local_shape = local_shape;
        node.world_shape = local_shape;
        transform_collision_node(&node);
        storage = plyr_pdata->plyr_info->collision_data;
        insert_player_attack_node(storage, &node, &movement);
    }

    node.local_shape.type = 1;
    node.local_shape.sphere_radius = radius * definition.radius;
    node.local_shape.sphere_center.x = definition.offset.x;
    node.local_shape.sphere_center.y = definition.offset.y;
    node.local_shape.sphere_center.z = definition.offset.z;
    node.bone = bone;
    transform_collision_node(&node);
    storage = plyr_pdata->plyr_info->collision_data;
        insert_player_attack_node(storage, &node, &movement);

    index = first_index;
    last_pair_index = collision_data->field_93F4 - 2;
    while (index <= last_pair_index) {
        first = &collision_data->attacks[index];
        second = &collision_data->attacks[index + 1];
        v3_sub_v3(
            &difference, &first->world_shape.sphere_center,
            &second->world_shape.sphere_center);
        if (length_v3(&difference) >
            2.0f * first->world_shape.sphere_radius) {
            node.world_shape.sphere_center =
                second->world_shape.sphere_center;
            node.world_shape.sphere_radius =
                second->world_shape.sphere_radius;
            node.world_shape.type = 1;
            v3_x_v_add_v3(
                &node.world_shape.sphere_center, &difference, 0.5f);
            storage = plyr_pdata->plyr_info->collision_data;
            insert_player_attack_node_unshifted(storage, &node);
        }
        index++;
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

/* TODO: [breakthrough] 92.24%; direct node addressing and float copies recovered; zero-center home and constructor store order remain. */
void reset_player_collision(PlyrInfo* player) {
    int definition_count;
    PlayerCollisionData* storage;
    unsigned int region_index;
    CollisionShape sphere;
    Vec center;
    float joint_scale;
    int bone_index;
    int index;

    storage = player->collision_data;
    if (storage == 0) {
        return;
    }

    definition_count = 0;
    storage->object = 0;
    storage->joint_count = 0;
    storage->field_93F4 = 0;
    storage->field_93F8 = 0;
    storage->recorded_count = 0;
    storage->render_recorded = 0;
    storage->active_count = 0;
    storage->active_scale = 1.0f;
    storage->attack_radius = 0.0f;
    storage->body_shape.type = 2;
    storage->body_shape.cylinder_radius = 0.3f;
    gxVectCopy(&storage->body_shape.cylinder_axis, &UNITVECT_Y);
    storage->body_shape.cylinder_height = 2.5f;
    storage->body_shape.cylinder_center.x = 0.0f;
    storage->body_shape.cylinder_center.y = 0.0f;
    storage->body_shape.cylinder_center.z = 0.0f;
    center.x = center.y = center.z = 0.0f;
    storage->object = player->slot.mirror_a;
    if (player->slot.mirror_a == 0) {
        return;
    }

    if (player->slot.pdata != 0) {
        if (player->slot.pdata->character_id == 0x1E) {
            definition_count = 28;
        } else {
            definition_count = 22;
        }
    }
    joint_scale = is_big_boss(player->slot.pdata) ? 1.3f : 1.0f;
    for (index = 0; index < definition_count; index++) {
        const struct CollisionNodeDef* definition = &col_def_list[index];
        bone_index = definition->node_id & 0xFFF;
        player->slot.mirror_a->bones[
            bone_index]->flags_54_bits.calculation_locked = 1;

        if (definition->joint_radius) {
            region_index = storage->joint_count;
            sphere.type = 1;
            gxVectCopy(&sphere.sphere_center, &center);
            sphere.sphere_radius = joint_scale * definition->joint_radius;
            storage->joints[region_index].bone = player->slot.mirror_a->bones[bone_index];
            storage->joints[region_index].local_shape = sphere;
            storage->joints[region_index].world_shape = sphere;
            storage->joint_count++;
        }
        if (definition->active_radius) {
            region_index = storage->active_count;
            sphere.type = 1;
            gxVectCopy(&sphere.sphere_center, &center);
            sphere.sphere_radius = definition->active_radius;
            storage->active_nodes[region_index].bone = player->slot.mirror_a->bones[bone_index];
            storage->active_nodes[region_index].local_shape = sphere;
            storage->active_nodes[region_index].world_shape = sphere;
            storage->active_count++;
        }
    }
}

void init_player_collision(PlyrInfo* player) {
    player->collision_data = get_mem(sizeof(*player->collision_data));
    if (player->collision_data != 0) {
        reset_player_collision(player);
    }
}

void term_collision_system(void) {
    destroy_mkprocs_pid(0x4001);
}

void init_collision_system(void) {
    MkProcInitFlags flags;
    MkProc* proc;

    flags.value = 0;
    proc = get_mkproc_nostack(flags);
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

/* TODO: [breakthrough needed] 72.47%; body cylinder center offset corrected; transform helper reload boundaries remain. */
static void update_player_collision_nodes(PlayerCollisionData* collision) {
    PlayerCollisionData* storage;
    PlayerCollisionNode* node;
    unsigned int index;

    storage = collision;
    if (storage->joint_count != 0U) {
        for (index = 0; index < storage->joint_count; index++) {
            node = &storage->joints[index];
            transform_collision_node(node);
        }
        for (index = 0; index < storage->active_count; index++) {
            node = &storage->active_nodes[index];
            transform_collision_node(node);
        }
    }

    if (storage->render_recorded != 0) {
        storage->recorded_count = 0;
        storage->render_recorded = 0;
    }
    if (storage->object != 0) {
        storage->body_shape.cylinder_center.x = storage->object->pos.value.x;
        storage->body_shape.cylinder_center.y = storage->object->pos.value.y;
        storage->body_shape.cylinder_center.z = storage->object->pos.value.z;
        storage->body_shape.cylinder_center.y -= 1.0f;
    }
}

/* TODO: [breakthrough] 85.14%; shared case exit recovered;
 * redundant dispatch exit and component load/store scheduling remain. */
void update_collision_obj_pos(CollisionObj* object, const Vec* position) {
    switch (object->shape.type & 7) {
    case 1:
        return;
    case 2:
        gxVectCopy(&object->shape.cylinder_center, position);
        break;
    case 3:
        return;
    case 4:
        return;
    default:
        return;
    }
}

void set_collision_render_state(int enabled) {
    g_game_info.pause_flag_bits.paused = enabled;
}
