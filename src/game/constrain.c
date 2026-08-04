#include "game/bgnd_types.h"
#include "game/collision.h"
#include "game/constrain.h"
#include "game/game_info.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"

typedef struct ConstrainPlayerState {
    Vec position;
    float projection;
} ConstrainPlayerState;

typedef struct ConstrainState {
    ConstrainPlayerState player[2];
    int separated;
} ConstrainState;

typedef struct ConstrainBssLayout {
    ConstrainState state; /* +0x00 */
    Vec perpendicular;    /* +0x24 */
    Vec axis;             /* +0x30 */
} ConstrainBssLayout;

typedef struct ObstacleInfo {
    int type;
    unsigned int first_id;
    unsigned int last_id;
} ObstacleInfo;

typedef struct ConstrainObstacleVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    void (*destroy)(ArenaObstacle*);
} ConstrainObstacleVtable;

int not_mkproc(void);
int not_mkpdata(void);
int not_mksobj(void);
int not_mkmaterial(void);
void vdestroy_obstacle(ArenaObstacle* obstacle);

MkVtable5 vtbl_obstacle = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    (MkVtblFn)vdestroy_obstacle,
};

static ObstacleInfo obstacle_info_table[8] = {
    {0, 0x001, 0x00A},
    {1, 0x00B, 0x014},
    {2, 0x015, 0x01E},
    {3, 0x01F, 0x028},
    {4, 0x029, 0x03C},
    {5, 0x03D, 0x0A0},
    {6, 0x0A1, 0x0FF},
    {7, 0x100, 0x1FF},
};

static unsigned short next_internal_id;
ConstrainInfo constrain_info;
extern ConstrainState constrain_state;
extern Vec tightrope_perp_uv;
extern Vec tightrope_uv;
static int tightrope_set;
static int p1_hit_side_of_arena;
static int p2_hit_side_of_arena;
static int update_tr_due_to_arena_edge;
static float right_wall_player_dist;
static float right_wall;
static float left_wall_player_dist;
static float left_wall;
static float tightrope_dist;
static int tightrope_set_this_tick;

void generate_obstacles(
    unsigned int flags,
    BgndObstacleData* obstacle_data,
    ConstrainInfo* info);
float dist_from_plyr_pos_to_arena_edge(
    const Vec* position, const Vec* direction);
float xz_ray_circle_intersection_dist(
    const Vec* ray_origin, const Vec* ray_direction, float radius);
CollisionObj* get_collision_obj(void);
void collision_obj_set_shape(
    CollisionObj* object, const CollisionShape* shape);
float repel_check_plyrs(void);
int player_is_stationary(PlyrPdata* player);
void bgnd_clear_danger_zone_callback(PlyrPdata* player);
void repel_against_obstacle_list(
    PlyrInfo* player,
    const Vec* previous_position,
    const Vec* movement,
    Vec* position,
    ConstrainInfo* info);
void ground_me(MkObj* object);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);

static float p_constrain_players(void);
static void repel_players(void);
static void keep_players_on_tightrope(void);

static float constrain_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float refined;

    if (!(value > 0.0f)) {
        return 0.0f;
    }

    input.f = value;
    guess.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    guess.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    refined = guess.f * (3.0f - (guess.f * guess.f) / value);
    return 0.5f * refined;
}

static float constrain_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;

    if (!(value > 0.0f)) {
        return 0.0f;
    }

    guess.f = value;
    guess.u = 0x5F375A00U - (guess.u >> 1);
    product = guess.f * (value * guess.f);
    correction = 3.0f - product;
    return 0.0625f * guess.f * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline MkObj* constrain_player_object(PlyrInfo* player) {
    return player->slot.mirror_a;
}

static inline float tightrope_projection(const Vec* position) {
    return position->x * tightrope_uv.x + position->z * tightrope_uv.z;
}

static inline int object_can_be_repelled(const MkObj* object) {
    return object->flags_09_bits.bit4 != 0;
}

static inline int object_ignores_wall_limits(const MkObj* object) {
    return (object->flags_0B & 0x40) != 0;
}

static inline int player_ignores_obstacles(const PlyrPdata* player) {
    return (player->state_flags.raw & 0x08) != 0;
}

void set_background_obstacle_disable_flag(
    int obstacle_id, int disabled) {
    MkPtr* link;
    MkPtr* next;
    ArenaObstacle* obstacle;
    unsigned char disable_flag;

    if (&constrain_info != 0) {
        link = constrain_info.obstacles;
        disable_flag = (unsigned char)disabled;
        while (link != 0) {
            obstacle = (ArenaObstacle*)link->hdr;
            if (link->instance != obstacle->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if ((int)obstacle->obstacle_id == obstacle_id) {
                    obstacle->flags.bits.disabled = disable_flag;
                }
                link = link->next;
            }
        }
    }
}

void set_background_obstacle_repel_flag(
    int obstacle_id, int repel_disabled) {
    MkPtr* link;
    MkPtr* next;
    ArenaObstacle* obstacle;

    if (&constrain_info != 0) {
        link = constrain_info.obstacles;
        while (link != 0) {
            obstacle = (ArenaObstacle*)link->hdr;
            if (link->instance != obstacle->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if ((int)obstacle->obstacle_id == obstacle_id) {
                    obstacle->flags.bits.repel = repel_disabled == 0;
                }
                link = link->next;
            }
        }
    }
}

void delete_obstacle_from_background_by_id(int obstacle_id) {
    MkPtr* next;
    MkPtr* link;
    ArenaObstacle* obstacle;

    if (&constrain_info != 0) {
        link = constrain_info.obstacles;
        while (link != 0) {
            obstacle = (ArenaObstacle*)link->hdr;
            if (link->instance != obstacle->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if ((int)obstacle->obstacle_id == obstacle_id &&
                    obstacle->hdr.instance != 0) {
                    ((ConstrainObstacleVtable*)obstacle->hdr.vtbl)
                        ->destroy(obstacle);
                }
                link = link->next;
            }
        }
    }
}

int get_obstacle_type_from_id(unsigned int obstacle_id) {
    int index;

    for (index = 0; index < 8; index++) {
        if (obstacle_info_table[index].first_id <= obstacle_id &&
            obstacle_info_table[index].last_id >= obstacle_id) {
            return obstacle_info_table[index].type;
        }
    }
    return 8;
}

/*
 * Soft ceiling: add_shape_to_background_obstacle_list ~94.52% - typed table
 * selection is exact semantically; the remaining island is loop NV
 * allocation plus the default type value being held across the scan.
 */
ArenaObstacle* add_shape_to_background_obstacle_list(
    const CollisionShape* shape,
    unsigned int obstacle_id) {
    ArenaObstacle* obstacle;
    CollisionObj* collision;
    int type;
    int index;

    obstacle = (ArenaObstacle*)get_mkhdr(
        &vtbl_obstacle, sizeof(ArenaObstacle));
    if (obstacle != 0) {
        obstacle->internal.value = 0;
        obstacle->internal.bits.flag_0 = 0;
        obstacle->internal.bits.flag_1 = 0;
        obstacle->internal.bits.internal_id = next_internal_id++;
        obstacle->obstacle_id = 0;
        obstacle->type = 8;
        obstacle->flags_word = 0;
        obstacle->shapes = 0;
    }
    if (obstacle == 0) {
        return 0;
    }

    mk_insert(&obstacle->hdr, &constrain_info.obstacles);
    obstacle->obstacle_id = obstacle_id;
    type = 8;
    for (index = 0; index < 8; index++) {
        if (obstacle_info_table[index].first_id <= obstacle_id &&
            obstacle_info_table[index].last_id >= obstacle_id) {
            type = obstacle_info_table[index].type;
            break;
        }
    }
    obstacle->type = type;

    collision = get_collision_obj();
    if (collision != 0) {
        collision_obj_set_shape(collision, shape);
        mk_insert(&collision->hdr, &obstacle->shapes);
    } else {
        if (obstacle->hdr.instance != 0) {
            ((ConstrainObstacleVtable*)obstacle->hdr.vtbl)
                ->destroy(obstacle);
        }
        obstacle = 0;
    }
    return obstacle;
}

ArenaObstacle* get_obstacle(void) {
    ArenaObstacle* obstacle;

    obstacle = (ArenaObstacle*)get_mkhdr(
        &vtbl_obstacle, sizeof(ArenaObstacle));
    if (obstacle != 0) {
        obstacle->internal.value = 0;
        obstacle->internal.bits.flag_0 = 0;
        obstacle->internal.bits.flag_1 = 0;
        obstacle->internal.bits.internal_id = next_internal_id++;
        obstacle->obstacle_id = 0;
        obstacle->type = 8;
        obstacle->flags_word = 0;
        obstacle->shapes = 0;
    }
    return obstacle;
}

void reset_obstacle_internal_id(void) {
    next_internal_id = 0;
}

int local_collision_allowed_plyr_pdata(void) {
    return 1;
}

int local_collision_allowed(void) {
    return 1;
}

int local_obstacle_callback(ArenaObstacle* obstacle) {
    (void)obstacle;
    return 1;
}

void vdestroy_obstacle(ArenaObstacle* obstacle) {
    destroy_list(&obstacle->shapes);
    obstacle->hdr.instance = 0;
    mkhdr_memfree(&obstacle->hdr);
}

void set_arena_obstacle_callback(ArenaObstacleCallback callback) {
    constrain_info.callback = callback;
}

void terminate_bgnd_collisions(void) {
    constrain_info.callback = 0;
    destroy_list(&constrain_info.obstacles);
}

void initialize_bgnd_collisions(BgndDataTable* background) {
    constrain_info.callback = 0;
    constrain_info.obstacles = 0;

    if (background->obstacle_data != 0) {
        if ((int)mode_of_play == 10) {
            generate_obstacles(0x8003D, background->obstacle_data, &constrain_info);
        } else {
            generate_obstacles(0x2001E, background->obstacle_data, &constrain_info);
        }
    }
}

/*
 * Soft ceiling: dist_behind_me ~96.84% -- validated-latch branch shape and
 * local float-pool relocation labels remain.
 */
float dist_behind_me(void) {
    MkObj* object;
    Vec direction;
    float distance;

    object = plyr_pdata->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance != plyr_pdata->tracked_obj_instance) {
            object = 0;
        } else {
            /* Keep the validated tracked object. */
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        return 0.0f;
    }

    uv_from_angle_y(&direction, object->ang.y + 3.1415927f);
    distance =
        dist_from_plyr_pos_to_arena_edge(&object->pos, &direction);
    if (distance < 0.0f) {
        distance = -1.0f * distance;
    }
    return distance;
}

float dist_from_plyr_pos_to_arena_edge(
    const Vec* position, const Vec* direction) {
    float length;
    float along_ray;
    float radicand;
    float distance;
    float inverse_length;
    float outward_dot;

    length = length_xz(position);
    if (length >= 12.0f) {
        distance = 0.0f;
    } else {
        along_ray =
            direction->x * position->x + direction->z * position->z;
        radicand =
            144.0f - (length * length - along_ray * along_ray);
        distance = constrain_sqrt(radicand) - along_ray;
        if (distance <= 0.0f) {
            distance = 0.0f;
        }
    }

    inverse_length =
        constrain_inv_sqrt(position->x * position->x +
                           position->z * position->z);
    outward_dot =
        direction->x * (position->x * inverse_length) +
        direction->z * (position->z * inverse_length);
    if (1.2f * outward_dot >= 1.0f) {
        distance = -distance;
    }
    return distance;
}

float xz_ray_circle_intersection_dist(
    const Vec* ray_origin, const Vec* ray_direction, float radius) {
    float length;
    float along_ray;
    float radicand;
    float distance;

    length = length_xz(ray_origin);
    if (length >= radius) {
        return 0.0f;
    }

    along_ray =
        ray_direction->x * ray_origin->x +
        ray_direction->z * ray_origin->z;
    radicand =
        radius * radius -
        (length * length - along_ray * along_ray);
    if (radicand > 0.0f) {
        distance = constrain_sqrt(radicand);
    } else {
        distance = 0.0f;
    }
    distance -= along_ray;
    if (distance > 0.0f) {
        return distance;
    }
    return 0.0f;
}

void uv_to_opponent(Vec* direction) {
    if (plyr_pdata == 0) {
        return;
    }

    if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
        if (constrain_state.player[0].projection <
            constrain_state.player[1].projection) {
            direction->x = tightrope_uv.x;
            direction->y = tightrope_uv.y;
            direction->z = tightrope_uv.z;
            return;
        }
        direction->x = -tightrope_uv.x;
        direction->y = -tightrope_uv.y;
        direction->z = -tightrope_uv.z;
        return;
    }

    if (plyr_pdata == g_game_info.plyr1.slot.pdata) {
        if (constrain_state.player[1].projection <
            constrain_state.player[0].projection) {
            direction->x = tightrope_uv.x;
            direction->y = tightrope_uv.y;
            direction->z = tightrope_uv.z;
            return;
        }
        direction->x = -tightrope_uv.x;
        direction->y = -tightrope_uv.y;
        direction->z = -tightrope_uv.z;
    }
}

void start_constrain_proc(void) {
    ConstrainBssLayout* bss;
    ConstrainState* state;
    Vec* perpendicular;
    Vec* axis;
    int flags;
    int proc_flags;

    bss = (ConstrainBssLayout*)&constrain_state;
    flags = 0;
    if (find_mkproc_pid(0x1003) == 0) {
        proc_flags = flags;
        create_mkproc(
            0x1A, get_mkproc_nostack(&proc_flags), 0x1003,
            p_constrain_players, 0);

        state = &bss->state;
        perpendicular = &bss->perpendicular;
        axis = &bss->axis;

        perpendicular->z = 0.0f;
        perpendicular->y = 0.0f;
        perpendicular->x = 0.0f;
        tightrope_dist = 0.0f;
        state->player[0].projection = 0.0f;
        state->player[1].projection = 0.0f;
        state->separated = 0;
        p1_hit_side_of_arena = 0;
        p2_hit_side_of_arena = 0;
        update_tr_due_to_arena_edge = 0;
        axis->z = 0.0f;
        axis->y = 0.0f;
        axis->x = 0.0f;

        /* Retail repeats this clear after initializing the tightrope axis. */
        perpendicular->z = 0.0f;
        perpendicular->y = 0.0f;
        perpendicular->x = 0.0f;

        state->player[0].position.z = 0.0f;
        state->player[0].position.y = 0.0f;
        state->player[0].position.x = 0.0f;
        state->player[1].position.z = 0.0f;
        state->player[1].position.y = 0.0f;
        state->player[1].position.x = 0.0f;
        tightrope_set = 0;
    }
}

void set_constrain_last_pos(int player, const Vec* position) {
    if (find_mkproc_pid(0x1003) != 0) {
        tightrope_set = 0;
        if (player == 0) {
            constrain_state.player[0].position.x = position->x;
            constrain_state.player[0].position.y = position->y;
            constrain_state.player[0].position.z = position->z;
        } else {
            constrain_state.player[1].position.x = position->x;
            constrain_state.player[1].position.y = position->y;
            constrain_state.player[1].position.z = position->z;
        }
    }
}

static float p_constrain_players(void) {
    MkObj* object_1;
    MkObj* object_2;
    MkObj* sidekick;
    PlyrPdata* pdata;
    Vec player_1_bone;
    Vec player_2_bone;
    float delta_x;
    float delta_z;

    tightrope_set_this_tick = 0;

    object_1 = constrain_player_object(&g_game_info.plyr0);
    if (object_1 != 0 && object_1->flags_09_bits.launched) {
        ground_me(object_1);
    }
    object_2 = constrain_player_object(&g_game_info.plyr1);
    if (object_2 != 0 && object_2->flags_09_bits.launched) {
        ground_me(object_2);
    }

    if (object_1 != 0) {
        pdata = g_game_info.plyr0.slot.pdata;
        if (pdata->sidekick_available != 0) {
            sidekick = pdata->sidekick_obj;
            if (sidekick != 0 &&
                sidekick->hdr.instance != pdata->sidekick_instance) {
                sidekick = 0;
            }
            if (sidekick->flags_09_bits.launched) {
                ground_me(sidekick);
            }
        }
    }

    if (object_2 != 0) {
        pdata = g_game_info.plyr1.slot.pdata;
        if (pdata->sidekick_available != 0) {
            sidekick = pdata->sidekick_obj;
            if (sidekick != 0 &&
                sidekick->hdr.instance != pdata->sidekick_instance) {
                sidekick = 0;
            }
            if (sidekick->flags_09_bits.launched) {
                ground_me(sidekick);
            }
        }
    }

    if (object_1 != 0 && object_2 != 0) {
        delta_x = object_2->pos.x - object_1->pos.x;
        delta_z = object_2->pos.z - object_1->pos.z;
        if (object_1->flags_09_bits.face_opponent) {
            object_1->ang.y = gxMathArcTanYX(delta_x, delta_z);
        }
        if (object_2->flags_09_bits.face_opponent) {
            object_2->ang.y = gxMathArcTanYX(-delta_x, -delta_z);
        }
    }

    keep_players_on_tightrope();

    object_1 = constrain_player_object(&g_game_info.plyr0);
    object_2 = constrain_player_object(&g_game_info.plyr1);
    if (object_1 != 0 && object_2 != 0) {
        if (object_1 != 0 && g_game_info.plyr0.field_0C == 0.0f) {
            get_bone_world_pos(object_1, 0x10, &player_1_bone);
        }
        if (object_2 != 0 && g_game_info.plyr1.field_0C == 0.0f) {
            get_bone_world_pos(object_2, 0x10, &player_2_bone);
        }
    }

    repel_players();

    object_1 = constrain_player_object(&g_game_info.plyr0);
    if (object_1 != 0) {
        constrain_state.player[0].position.x = object_1->pos.x;
        constrain_state.player[0].position.y = object_1->pos.y;
        constrain_state.player[0].position.z = object_1->pos.z;
        constrain_state.player[0].projection =
            object_1->pos.x * tightrope_uv.x +
            object_1->pos.z * tightrope_uv.z;
    }

    object_2 = constrain_player_object(&g_game_info.plyr1);
    if (object_2 != 0) {
        constrain_state.player[1].position.x = object_2->pos.x;
        constrain_state.player[1].position.y = object_2->pos.y;
        constrain_state.player[1].position.z = object_2->pos.z;
        constrain_state.player[1].projection =
            object_2->pos.x * tightrope_uv.x +
            object_2->pos.z * tightrope_uv.z;
    }

    return 1.0f;
}

/*
 * Soft ceiling: repel_players ~75.53% - the typed wall, stationarity, and
 * obstacle flow is complete; large FPR/NV scheduling differences
 * remain. Retail intentionally remembers only negative wall penetration.
 */
static void repel_players(void) {
    PlyrInfo* player_1;
    PlyrInfo* player_2;
    PlyrPdata* pdata_1;
    PlyrPdata* pdata_2;
    MkObj* object_1;
    MkObj* object_2;
    Vec movement_1;
    Vec movement_2;
    float repel_distance;
    float projection_1;
    float projection_2;
    float distance;
    float midpoint;
    float push_1;
    float push_2;
    float direction_1;
    float direction_2;
    int stationary_1;
    int stationary_2;

    player_1 = &g_game_info.plyr0;
    player_2 = &g_game_info.plyr1;
    pdata_1 = player_1->slot.pdata;
    pdata_2 = player_2->slot.pdata;
    object_1 = constrain_player_object(player_1);
    object_2 = constrain_player_object(player_2);
    if (object_1 == 0 || object_2 == 0) {
        return;
    }

    repel_distance = repel_check_plyrs();
    if (repel_distance != 0.0f) {
        projection_1 = tightrope_projection(&object_1->pos);
        projection_2 = tightrope_projection(&object_2->pos);
        repel_distance = 0.75f * repel_distance * repel_distance;
        if (projection_1 > projection_2) {
            if (object_can_be_repelled(object_1)) {
                xz_x_v_add_xz(
                    &object_1->pos, &tightrope_uv, repel_distance);
            }
            if (object_can_be_repelled(object_2)) {
                xz_x_v_add_xz(
                    &object_2->pos, &tightrope_uv, -repel_distance);
            }
        } else {
            if (object_can_be_repelled(object_1)) {
                xz_x_v_add_xz(
                    &object_1->pos, &tightrope_uv, -repel_distance);
            }
            if (object_can_be_repelled(object_2)) {
                xz_x_v_add_xz(
                    &object_2->pos, &tightrope_uv, repel_distance);
            }
        }
    }

    if (!object_ignores_wall_limits(object_1) &&
        !object_ignores_wall_limits(object_2)) {
        projection_1 = tightrope_projection(&object_1->pos);
        projection_2 = tightrope_projection(&object_2->pos);
        distance = projection_1 - projection_2;
        if (distance < 0.0f) {
            distance = -distance;
        }

        if (distance > 6.5f) {
            if (!constrain_state.separated) {
                constrain_state.separated = 1;
                midpoint = (projection_1 + projection_2) * 0.5f;
                left_wall = midpoint - distance * 0.5f;
                right_wall = midpoint + distance * 0.5f;
            }
        } else {
            constrain_state.separated = 0;
            if (distance < 5.5f) {
                midpoint = (projection_1 + projection_2) * 0.5f;
                left_wall = midpoint - 3.25f;
                right_wall = midpoint + 3.25f;
            } else if (tightrope_set_this_tick &&
                       pdata_1 != 0 && pdata_2 != 0) {
                stationary_1 = player_is_stationary(pdata_1);
                stationary_2 = player_is_stationary(pdata_2);
                if (stationary_1 != stationary_2) {
                    if (stationary_1) {
                        if (projection_1 > projection_2) {
                            right_wall =
                                projection_1 + right_wall_player_dist;
                            left_wall = right_wall - 6.5f;
                        } else {
                            left_wall =
                                projection_1 - left_wall_player_dist;
                            right_wall = left_wall + 6.5f;
                        }
                    } else if (projection_2 > projection_1) {
                        right_wall =
                            projection_2 + right_wall_player_dist;
                        left_wall = right_wall - 6.5f;
                    } else {
                        left_wall =
                            projection_2 - left_wall_player_dist;
                        right_wall = left_wall + 6.5f;
                    }
                }
            }
        }

        if (projection_1 > projection_2) {
            push_1 = projection_1 - right_wall;
            push_2 = left_wall - projection_2;
            direction_1 = -1.0f;
            direction_2 = 1.0f;
        } else {
            push_1 = left_wall - projection_1;
            push_2 = projection_2 - right_wall;
            direction_1 = 1.0f;
            direction_2 = -1.0f;
        }

        left_wall_player_dist = 0.0f;
        if (push_1 < 0.0f) {
            left_wall_player_dist = -push_1;
        }
        right_wall_player_dist = 0.0f;
        if (push_2 < 0.0f) {
            right_wall_player_dist = -push_2;
        }

        if (object_can_be_repelled(object_1) ||
            object_can_be_repelled(object_2)) {
            if (push_1 > 0.0f) {
                xz_x_v_add_xz(
                    &object_1->pos, &tightrope_uv,
                    direction_1 * push_1);
            }
            if (push_2 > 0.0f) {
                xz_x_v_add_xz(
                    &object_2->pos, &tightrope_uv,
                    direction_2 * push_2);
            }
        }
    }

    if (!player_ignores_obstacles(pdata_1) &&
        player_1->collision_data != 0) {
        bgnd_clear_danger_zone_callback(pdata_1);
        movement_1.x =
            object_1->pos.x - constrain_state.player[0].position.x;
        movement_1.y =
            object_1->pos.y - constrain_state.player[0].position.y;
        movement_1.z =
            object_1->pos.z - constrain_state.player[0].position.z;
        repel_against_obstacle_list(
            player_1, &constrain_state.player[0].position,
            &movement_1, &object_1->pos, &constrain_info);
    }

    if (!player_ignores_obstacles(pdata_2) &&
        player_2->collision_data != 0) {
        bgnd_clear_danger_zone_callback(pdata_2);
        movement_2.x =
            object_2->pos.x - constrain_state.player[1].position.x;
        movement_2.y =
            object_2->pos.y - constrain_state.player[1].position.y;
        movement_2.z =
            object_2->pos.z - constrain_state.player[1].position.z;
        repel_against_obstacle_list(
            player_2, &constrain_state.player[1].position,
            &movement_2, &object_2->pos, &constrain_info);
    }
}

/*
 * Soft ceiling: keep_players_on_tightrope ~90.37% - remaining differences
 * are the duplicated retail null checks and NV coloring around vector
 * publication and the adjustment calls.
 */
static void keep_players_on_tightrope(void) {
    MkObj* player_2;
    MkObj* player_1;
    Vec direction;
    float offset;

    player_1 = constrain_player_object(&g_game_info.plyr0);
    player_2 = constrain_player_object(&g_game_info.plyr1);
    if (player_1 == 0 || player_2 == 0) {
        return;
    }

    if (!player_1->flags_09_bits.tightrope_restricted ||
        !player_2->flags_09_bits.tightrope_restricted ||
        !tightrope_set || update_tr_due_to_arena_edge) {
        if (xz_unit_vector_recip(
                &direction, &player_1->pos, &player_2->pos) != 0.0f) {
            if (tightrope_set &&
                direction.x * tightrope_uv.x +
                    direction.z * tightrope_uv.z <
                    0.0f) {
                direction.x = -direction.x;
                direction.z = -direction.z;
            }

            tightrope_uv.x = direction.x;
            tightrope_uv.y = direction.y;
            tightrope_uv.z = direction.z;
            tightrope_perp_uv.x = direction.z;
            tightrope_perp_uv.z = -direction.x;
            if (!g_game_info.feature_flags.bits.high_bit) {
                tightrope_dist =
                    player_1->pos.y * tightrope_perp_uv.y +
                    player_1->pos.x * tightrope_perp_uv.x +
                    player_1->pos.z * tightrope_perp_uv.z;
            }
            tightrope_set = 1;
            tightrope_set_this_tick = 1;
        }
        update_tr_due_to_arena_edge = 0;
        return;
    }

    offset =
        xz_dot_xz(&player_1->pos, &tightrope_perp_uv) - tightrope_dist;
    if (offset < -0.1f) {
        xz_x_v_add_xz(
            &player_1->pos, &tightrope_perp_uv, -(0.1f + offset));
    } else if (offset > 0.1f) {
        xz_x_v_add_xz(
            &player_1->pos, &tightrope_perp_uv, 0.1f - offset);
    }

    player_2 = constrain_player_object(&g_game_info.plyr1);
    offset =
        xz_dot_xz(&player_2->pos, &tightrope_perp_uv) - tightrope_dist;
    if (offset < -0.1f) {
        xz_x_v_add_xz(
            &player_2->pos, &tightrope_perp_uv, -(0.1f + offset));
    } else if (offset > 0.1f) {
        xz_x_v_add_xz(
            &player_2->pos, &tightrope_perp_uv, 0.1f - offset);
    }
}

float get_constrain_player_distance(void) {
    float distance =
        constrain_state.player[0].projection -
        constrain_state.player[1].projection;

    if (distance >= 0.0f) {
        return distance;
    }
    return -distance;
}

/* Retail global .bss order: state, perpendicular axis, tightrope axis. */
Vec tightrope_uv;
Vec tightrope_perp_uv;
ConstrainState constrain_state;
