#include "runtime/asset.h"
#include "game/game_info.h"
#include "game/moveset.h"
#include "game/specular.h"
#include "libmkparticle/compile.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "platform/io.h"

#define YINYANG_LENSFLARE_PID 0x3019
#define FREEZE_TEXTURE_HANDLE 0x10005
#define FREEZE_TEXTURE_OID 0x2003B

/* Retail TU-local; its body remains in the split assembly. */
static void apply_special_fx_to_player(void* texture);

typedef struct FreezeLightPdata {
    MkHdr hdr;
    PlyrMirrorObjLatch* light;
    MkObj* player_object;
    PlyrPdata* player;
} FreezeLightPdata;

typedef struct FxRayPlane FxRayPlane;

typedef struct LensflarePdata {
    MkHdr hdr;
    const Vec* sun_position;
    const struct LensFlareDefinition* lens_data;
    const FxRayPlane* obstructions;
    int obstruction_count;
} LensflarePdata;

typedef struct LensFlareDefinition {
    float line_position;
    const char* texture_name;
} LensFlareDefinition;

typedef struct LensFlareEntry {
    ScreenObj* object;
    float line_position;
    float half_height;
    float half_width;
} LensFlareEntry;

typedef struct LensFlareData {
    LensFlareEntry entries[10];
    int inserted;
    int count;
} LensFlareData;

typedef struct FxProcTransferVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkProcDestroyFn destroy;
    MkProcFn dispatch;
    MkProcFn sleep;
    MkProcFn system_stack;
    MkProcFn local_stack;
    void (*transfer_sleep)(MkProcEntryFn entry, MkProc* proc, float ticks);
} FxProcTransferVtable;

typedef union FxScreenLoadFlags {
    int value;
    struct {
        unsigned char bit7 : 1;
        unsigned char bit6 : 1;
        unsigned char reverse : 1; /* bit5 */
        unsigned char bit4 : 1;
        unsigned char alternate : 1; /* bit3 */
        unsigned char low_bits : 3;
        unsigned char padding[3];
    } bits;
} FxScreenLoadFlags;

typedef struct FxScreenObjLatch {
    ScreenObj* object;
    unsigned int instance;
} FxScreenObjLatch;

typedef struct FxHdrLatch {
    MkHdr* object;
    unsigned int instance;
} FxHdrLatch;

struct FxRayPlane {
    int axis;
    float coordinate;
    Vec minimum;
    Vec maximum;
};

static const char fx_string_base[228];

const LensFlareDefinition yinyang_lens_data[] = {
    { -0.65f, &fx_string_base[0] }, { -0.5f, &fx_string_base[11] },
    { -0.35f, &fx_string_base[22] }, { -0.08f, &fx_string_base[33] },
    { 0.19f, &fx_string_base[33] }, { 0.5f, &fx_string_base[45] },
    { 0.75f, &fx_string_base[56] }, { 1.0f, &fx_string_base[67] },
    { 0.0f, 0 },
};
const LensFlareDefinition courtyard_lens_data[] = {
    { -0.65f, &fx_string_base[78] }, { -0.5f, &fx_string_base[89] },
    { -0.35f, &fx_string_base[100] }, { -0.08f, &fx_string_base[111] },
    { 0.19f, &fx_string_base[111] }, { 0.5f, &fx_string_base[123] },
    { 0.75f, &fx_string_base[134] }, { 1.0f, &fx_string_base[145] },
    { 0.0f, 0 },
};
static float lensflare_proc2(void);
static float lensflare_proc(void);
static float fighting_style_sign_proc(void);

typedef struct FightingStyleSignPdata {
    MkHdr hdr;
    GlobalMoveset* moveset;
    int player;
} FightingStyleSignPdata;

typedef struct FxPfxDefinition {
    unsigned int flags;
    int field_04;
    int field_08;
    Vec origin;
    float particle_size;
    float red;
    float green;
    float blue;
    float alpha;
    int emitter_lifetime;
    int field_90;
    int emitter_field_40;
    void* texture;
    int animate_texture;
    int texture_width;
    int texture_height;
    int texture_frame_width;
    float texture_speed;
    int texture_enabled;
    int kill_plane_x;
    int kill_plane_y;
    int kill_plane_z;
    float plane_x;
    float plane_y;
    float plane_z;
    int lifetime_mode;
    int lifetime_minimum;
    int lifetime_maximum;
    PfxInitCb initialize;
} FxPfxDefinition;

typedef struct FxPfxVmView {
    char pad00[0x50];
    int field_50;
    char pad54[0xFC];
    union {
        unsigned char flags_150;
        struct {
            unsigned char enabled : 1; /* bit7 */
            unsigned char sized : 1; /* bit6 */
            unsigned char pad_flags_150 : 6;
        } flag_bits;
    };
    char pad151[0x31];
    short texture_enabled;
    char pad184[0x30];
    PfxColor color;
    float particle_size;
} FxPfxVmView;


extern CameraObj* camera_obj;
extern int screen_width;
extern int screen_height;
ScreenObj* player_fstyle_sign[2] = { 0, 0 };
static LensFlareData flare_data;
PlyrMirrorObjLatch p1_freeze_light_item;
PlyrMirrorObjLatch p2_freeze_light_item;
FxHdrLatch p1_freeze_proc_item;
FxHdrLatch p2_freeze_proc_item;
int small_ground_fx;
int large_ground_fx;
static FxScreenObjLatch p1_skewer_item;
static FxScreenObjLatch p1_skewer_tip_item;
static FxScreenObjLatch p2_skewer_item;
static FxScreenObjLatch p2_skewer_tip_item;
static int f_p1_showing_fatatality;
static int f_p2_showing_fatatality;
static int f_p1_show_fatality_off;
static int f_p2_show_fatality_off;
extern int check_for_winner(void);
extern int get_fatality_available_flag(void);
extern void kill_fstyle_signs_for_plyr(PlyrInfo* player);
extern MkPtr* freeze_light_list;
extern PlyrPdata* his_pdata;
extern int snd_req(int sound_id);
extern PfxEmitter* pfx_get_emitter(PfxVm* vm, int index);
extern void* pfx_get_field(PfxVm* vm, int emitter_index, int field);
extern void pfx_texture_animate(
    PfxVm* vm, int width, int height, int frame_width, float speed);
extern void* pfx_behavior(PfxVm* vm, int emitter_index);
extern void pfxvm_kill_on_intersect_plane_x(void* behavior, float plane);
extern void pfxvm_kill_on_intersect_plane_y(void* behavior, float plane);
extern void pfxvm_kill_on_intersect_plane_z(void* behavior, float plane);
extern void pfxvm_spawn_line_1f(
    PfxEmitter* emitter, int field, float minimum, float maximum);
extern void pfxvm_kill_on_greater(
    void* behavior, int field, float value);
extern double fabs(double value);
static RpMaterial* material_set_specular(RpMaterial* material,
                                         void* data);
int FSTYLE_LFT_START_X;
int FSTYLE_LFT_END_X = 0x14;
int FSTYLE_RGHT_END_X = 0x271;
int FSTYLE_RGHT_START_X = 0x280;
int SKEWER_LEFT_OVERHANG = 0x14;
int SKEWER_RIGHT_OVERHANG = 0xA;

typedef struct FxFreezeLightDefinition {
    int type;
    MkProcEntryFn proc;
    FxScreenLoadFlags flags;
    float color[4];
    float field_1C;
    float field_20;
    float field_24;
} FxFreezeLightDefinition;

FxFreezeLightDefinition plyr_freeze_light = {
    3, 0, 0x20,
    { 1.0f, 1.0f, 1.0f, 1.0f },
    0.0f, 0.0f, 0.0f,
};
Vec sun = { 0.0f, 0.0f, 0.0f };
Vec yinyang_sun_pos = { -97.4000015f, 58.0f, -29.0f };
FxRayPlane yinyang_flare_obstructions[3] = {
    { 0, -15.0f, { -15.0f, 0.0f, -2.8f },
      { -15.0f, 8.0f, 2.8f } },
    { 0, -39.8f, { -39.8f, 0.0f, 0.1f },
      { -39.8f, 12.0f, 9.0f } },
    { 0, -39.8f, { -39.8f, 10.0f, 1.6f },
      { -39.8f, 14.0f, 7.5f } },
};
Vec courtyard_sun_pos = { 105.993f, 79.428f, 259.765f };
FxRayPlane courtyard_flare_obstructions[1] = {
    { 2, 22.25f, { -7.75f, 0.0f, 22.25f },
      { 8.16f, 9.55f, 22.25f } },
};

#pragma dont_inline on
static int rayintersection(
    const Vec* origin, const Vec* direction, const FxRayPlane* plane) {
    Vec intersection = { 0.0f, 0.0f, 0.0f };
    float distance;

    switch (plane->axis) {
    case 0:
        if (direction->x < 0.0001 && direction->x > -0.0001) {
            return 0;
        }
        distance = (plane->coordinate - origin->x) / direction->x;
        intersection.y = distance * direction->y + origin->y;
        intersection.z = distance * direction->z + origin->z;
        if (intersection.y > plane->minimum.y &&
            intersection.y < plane->maximum.y &&
            intersection.z > plane->minimum.z &&
            intersection.z < plane->maximum.z) {
            return 1;
        }
        return 0;
    case 1:
        if (direction->y < 0.0001 && direction->y > -0.0001) {
            return 0;
        }
        distance = (plane->coordinate - origin->y) / direction->y;
        intersection.x = distance * direction->x + origin->x;
        intersection.z = distance * direction->z + origin->z;
        if (intersection.x > plane->minimum.x &&
            intersection.x < plane->maximum.x &&
            intersection.z > plane->minimum.z &&
            intersection.z < plane->maximum.z) {
            return 1;
        }
        return 0;
    case 2:
    default:
        if (!(direction->z < 0.0001) ||
            !(direction->z > -0.0001)) {
            distance = (plane->coordinate - origin->z) / direction->z;
            intersection.x = distance * direction->x + origin->x;
            intersection.y = distance * direction->y + origin->y;
            if (intersection.x > plane->minimum.x &&
                intersection.x < plane->maximum.x &&
                intersection.y > plane->minimum.y &&
                intersection.y < plane->maximum.y) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
}
#pragma dont_inline reset

static const char fx_string_base[228] =
    "YY_FLARES3\0YY_FLARES2\0YY_FLARES1\0YY_FLAREDOT\0"
    "YY_FLARES5\0YY_FLARES6\0YY_FLARES7\0"
    "CY_FLARES3\0CY_FLARES2\0CY_FLARES1\0CY_FLAREDOT\0"
    "CY_FLARES5\0CY_FLARES6\0CY_FLARES7\0"
    "Tried to unfreeze a player who is NOT frozen!!\0"
    "TELE_ENERGY\0FX.C-created";

static float lensflare_proc2(void) {
    LensflarePdata* pdata;
    CameraObj* camera;
    LensFlareEntry* flare;
    Vec angles;
    Vec direction;
    Vec obstruction_direction;
    float horizontal;
    float vertical;
    float horizontal_abs;
    float vertical_abs;
    float alpha_value;
    float screen_x;
    int blocked;
    int index;

    pdata = (LensflarePdata*)apdata;
    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    if (camera == 0) {
        mkproc_die();
    }

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    uv_v3_to_v3(&direction, &camera->pos, &sun);
    v3_to_xy_ang(&angles, &direction);
    angles.x -= camera->ang.x;
    angles.y -= camera->ang.y;
    angles.z -= camera->ang.z;
    norm_angles_v3(&angles);
    if (angles.x > 3.1415927f) {
        angles.x -= 6.2831855f;
    }
    if (angles.y > 3.1415927f) {
        angles.y -= 6.2831855f;
    }

    blocked = 0;
    if (fabs(angles.y) < 0.69813f &&
        fabs(angles.x) < 0.69813f) {
        if (pdata->obstruction_count != 0 && pdata->obstructions != 0) {
            camera = camera_item.node;
            if (camera != 0) {
                if (camera->hdr.instance != camera_item.instance) {
                    camera = 0;
                }
            } else {
                camera = 0;
            }
            if (camera != 0) {
                uv_v3_to_v3(
                    &obstruction_direction,
                    &camera->pos, &sun);
                for (index = 0;
                     index < pdata->obstruction_count;
                     index++) {
                    if (rayintersection(
                            &camera->pos,
                            &obstruction_direction,
                            &pdata->obstructions[index])) {
                        blocked = 1;
                        break;
                    }
                }
            }
        }

        if (!blocked) {
            horizontal = angles.y / 0.69813f;
            vertical = angles.x / 0.69813f;
            horizontal_abs = (float)fabs(horizontal);
            vertical_abs = (float)fabs(vertical);
            for (index = 0; index < flare_data.count; index++) {
                flare = &flare_data.entries[index];
                screen_x =
                    ((float)(screen_width / 2) *
                     (horizontal * flare->line_position)) +
                    (float)(screen_width / 2) - flare->half_width;
                if (flare->object != 0) {
                    flare->object->x = (int)screen_x;
                    flare->object->y =
                        (int)(((float)(screen_height / 2) *
                               (vertical * flare->line_position)) +
                              (float)(screen_height / 2) -
                              flare->half_height);
                }
                if (horizontal_abs >= vertical_abs) {
                    if (horizontal >= 0.0f) {
                        alpha_value =
                            255.0f *
                            (0.75f * (1.0f - horizontal) + 0.25f);
                    } else {
                        alpha_value =
                            255.0f *
                            (0.75f * (1.0f + horizontal) + 0.25f);
                    }
                } else if (vertical >= 0.0f) {
                    alpha_value =
                        255.0f *
                        (0.75f * (1.0f - vertical) + 0.25f);
                } else {
                    alpha_value =
                        255.0f *
                        (0.75f * (1.0f + vertical) + 0.25f);
                }
                flare->object->pfx2d->verts[0].a =
                    (unsigned char)alpha_value;
                flare->object->pfx2d->verts[1].a =
                    (unsigned char)alpha_value;
                flare->object->pfx2d->verts[2].a =
                    (unsigned char)alpha_value;
                flare->object->pfx2d->verts[3].a =
                    (unsigned char)alpha_value;
                flare->object->pfx2d->mirror = 1;
                if (flare_data.inserted == 0) {
                    insert_screen_obj(flare->object);
                }
            }
            flare_data.inserted = 1;
            return 1.0f;
        }
    }

    if (flare_data.inserted == 1) {
        for (index = 0; index < flare_data.count; index++) {
            pull_screen_obj(flare_data.entries[index].object);
        }
    }
    flare_data.inserted = 0;
    return 1.0f;
}

static float lensflare_proc(void) {
    LensflarePdata* pdata;
    const LensFlareDefinition* definition;
    LensFlareEntry* flare;
    ScreenObj* object;
    int slot;
    int count;

    if (!g_game_info.flag_bits.lens_flare_enabled) {
        return 1.0f;
    }

    pdata = (LensflarePdata*)apdata;
    sun = *pdata->sun_position;
    definition = pdata->lens_data;
    count = 0;
    while (definition->texture_name != 0 && count < 10) {
        flare = &flare_data.entries[count];
        flare->line_position = definition->line_position;
        if (mode_of_play == 10 || mode_of_play == 9) {
            slot = 0x8003D;
        } else {
            slot = 0x2001E;
        }
        object = load_named_2d_pfxobj(
            slot, 0x301F, definition->texture_name, 0, 0xC);
        if (object != 0) {
            object->x = screen_width / 2 - object->pfx2d->tex_w / 2;
            object->y = screen_height / 2 - object->pfx2d->tex_h / 2;
            object->pfx2d->src_blend = 5;
            object->pfx2d->dst_blend = 2;
            pull_screen_obj(object);
        }
        flare->object = object;
        mk_insert((MkHdr*)flare->object, &aproc->pdata_list);
        flare->half_height = (float)flare->object->pfx2d->tex_h * 0.5f;
        flare->half_width = (float)flare->object->pfx2d->tex_w * 0.5f;
        definition++;
        count++;
    }
    if (count == 0) {
        mkproc_die();
    }
    flare_data.inserted = 0;
    flare_data.count = count;
    ((FxProcTransferVtable*)aproc->vtbl)->transfer_sleep(
        lensflare_proc2, aproc, 1.0f);
    return 1.0f;
}

#define FSTYLE_SIGN_SLOT 0x2001E
#define FSTYLE_SIGN_OID 0x3002
#define FSTYLE_SIGN_PRIORITY 0x2A

void courtyard_start_lensflare(void) {
    LensflarePdata* pdata;

    _create_mkproc_generic_tinystack(
        YINYANG_LENSFLARE_PID, 0x20, lensflare_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    pdata->sun_position = &courtyard_sun_pos;
    pdata->lens_data = courtyard_lens_data;
    pdata->obstructions = courtyard_flare_obstructions;
    pdata->obstruction_count = 1;
}

void yinyang_stop_lensflare(void) {
    destroy_mkprocs_pid(YINYANG_LENSFLARE_PID);
}

void yinyang_start_lensflare(void) {
    LensflarePdata* pdata;

    _create_mkproc_generic_tinystack(
        YINYANG_LENSFLARE_PID, 0x20, lensflare_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    pdata->sun_position = &yinyang_sun_pos;
    pdata->lens_data = yinyang_lens_data;
    pdata->obstructions = yinyang_flare_obstructions;
    pdata->obstruction_count = 0;
}

void show_fighting_style(GlobalMoveset* moveset, int player) {
    FightingStyleSignPdata* pdata;
    ScreenObj* sign;
    MkProc* proc;
    int pid;
    int winner;
    int allow_restart;

    if (moveset == 0) {
        return;
    }
    if ((int)mode_of_play == 6) {
        return;
    }
    if (g_game_info.flag_bits.high_res_path == 1) {
        return;
    }
    if (player < 0) {
        return;
    }
    if (player > 1) {
        return;
    }

    pid = 0x3004;
    if (player == 0) {
        pid = 0x3003;
    }
    sign = moveset->style_sign;
    if (sign != 0) {
        if ((unsigned int)sign->instance ==
            moveset->style_sign_instance) {
        } else {
            sign = 0;
        }
    } else {
        sign = 0;
    }
    if (sign == 0) {
        return;
    }

    if (player_fstyle_sign[player] == sign) {
        if (get_fatality_available_flag() == 0) {
            allow_restart = 0;
        } else if (g_game_info.pause_flag_bits.fatality_window) {
            winner = check_for_winner();
            if (player == 0 && winner == 1) {
                allow_restart = 1;
            } else if (player == 1 && winner == 2) {
                allow_restart = 1;
            } else {
                allow_restart = 0;
            }
        } else {
            allow_restart = 0;
        }
        if (allow_restart == 0) {
            return;
        }
    }

    proc = find_mkproc_pid(pid);
    if (proc != 0) {
        pdata = (FightingStyleSignPdata*)pdata_of_proc(proc);
        pdata->moveset = moveset;
        pdata->player = player;
        xfer_proc(proc, fighting_style_sign_proc);
        return;
    }

    proc = _create_mkproc_generic_bigstack(
        pid, 0x20, fighting_style_sign_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    if (proc != 0) {
        pdata->moveset = moveset;
        pdata->player = player;
    }
}

/* The screen-object latches retain both pointer and instance for validation. */
static void update_skewer_positions(int player) {
    ScreenObj* body;
    ScreenObj* tip;
    ScreenObj* sign;
    PlyrPdata* player_data;
    FxScreenLoadFlags flags;

    body = 0;
    tip = 0;
    flags.value = 0;
    if (player == 0) {
        body = p1_skewer_item.object;
        if (body != 0) {
            if ((unsigned int)body->instance != p1_skewer_item.instance) {
                body = 0;
            }
        } else {
            body = 0;
        }
        if (body == 0) {
            flags.bits.reverse = 0;
            flags.bits.alternate = 1;
            body = load_2d_pfxobj(
                0x10005, 0x2052, (char*)0x2001A, flags.value, 0x2B);
            if (body != 0) {
                p1_skewer_item.object = body;
                p1_skewer_item.instance = body->instance;
                tip = load_2d_pfxobj(
                    0x10005, 0x2052, (char*)0x2001B, 0, 0x2B);
                if (tip != 0) {
                    p1_skewer_tip_item.object = tip;
                    p1_skewer_tip_item.instance = tip->instance;
                }
            }
        } else {
            tip = p1_skewer_tip_item.object;
            if (tip != 0) {
                if ((unsigned int)tip->instance !=
                    p1_skewer_tip_item.instance) {
                    tip = 0;
                }
            } else {
                tip = 0;
            }
        }

        sign = player_fstyle_sign[0];
        if (sign != 0) {
            body->x = sign->x - SKEWER_LEFT_OVERHANG;
            body->y = sign->y +
                (sign->pfx2d->tex_h / 2 - body->pfx2d->tex_h / 2);
            if (f_p1_showing_fatatality != 0) {
                body->scale_x =
                    (float)(SKEWER_LEFT_OVERHANG +
                            SKEWER_RIGHT_OVERHANG + 0x9D) * 0.125f;
            } else {
                player_data = g_game_info.plyr0.slot.pdata;
                if (player_data->active_move_display != 0) {
                    body->scale_x =
                        (float)(player_data->active_move_display->display_width +
                                SKEWER_LEFT_OVERHANG +
                                SKEWER_RIGHT_OVERHANG) * 0.125f;
                }
            }
            tip->x = (int)(8.0f * body->scale_x + (float)body->x);
            tip->y = body->y - 1;
        }
    }

    if (player == 1) {
        body = p2_skewer_item.object;
        if (body != 0) {
            if ((unsigned int)body->instance != p2_skewer_item.instance) {
                body = 0;
            }
        } else {
            body = 0;
        }
        if (body == 0) {
            flags.bits.reverse = 0;
            flags.bits.alternate = 1;
            body = load_2d_pfxobj(
                0x10005, 0x2053, (char*)0x2001A, flags.value, 0x2B);
            if (body != 0) {
                p2_skewer_item.object = body;
                p2_skewer_item.instance = body->instance;
                flags.bits.reverse = 1;
                flags.bits.alternate = 0;
                tip = load_2d_pfxobj(
                    0x10005, 0x2053, (char*)0x2001B,
                    flags.value, 0x2B);
                if (tip != 0) {
                    p2_skewer_tip_item.object = tip;
                    p2_skewer_tip_item.instance = tip->instance;
                }
            }
        } else {
            tip = p2_skewer_tip_item.object;
            if (tip != 0) {
                if ((unsigned int)tip->instance !=
                    p2_skewer_tip_item.instance) {
                    tip = 0;
                }
            } else {
                tip = 0;
            }
        }

        sign = player_fstyle_sign[1];
        if (sign != 0) {
            body->x = sign->x - SKEWER_RIGHT_OVERHANG;
            body->y = sign->y +
                (sign->pfx2d->tex_h / 2 - body->pfx2d->tex_h / 2);
            body->scale_x =
                ((float)screen_width - (float)body->x) * 0.125f;
            tip->x = body->x - tip->pfx2d->tex_w;
            tip->y = body->y - 1;
        }
    }
}

static float fighting_style_sign_proc(void) {
    FightingStyleSignPdata* pdata;
    GlobalMoveset* moveset;
    ScreenObj* sign;
    ScreenObj* skewer;
    int player;
    int fatality;
    int width;

    pdata = (FightingStyleSignPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    moveset = pdata->moveset;
    player = pdata->player;

    if (player == 0) {
        if (f_p1_showing_fatatality != 0) {
            f_p1_show_fatality_off = 1;
            f_p1_showing_fatatality = 0;
        }
        if (player_fstyle_sign[0] != 0) {
            width = g_game_info.plyr0.slot.pdata
                ->active_move_display->display_width;
            while (player_fstyle_sign[0]->x > width) {
                int old_x = player_fstyle_sign[0]->x;

                player_fstyle_sign[0]->x = old_x - 0x1E;
                update_skewer_positions(0);
                _mkproc_sleep_ticks = 1.0f;
                aproc->vtbl->sleep();
                if (player_fstyle_sign[0] == 0) {
                    mkproc_die();
                }
            }
            pull_screen_obj(player_fstyle_sign[0]);
            player_fstyle_sign[0] = 0;

            skewer = p1_skewer_item.object;
            if (skewer != 0) {
                if ((unsigned int)skewer->instance !=
                    p1_skewer_item.instance) {
                    skewer = 0;
                }
            } else {
                skewer = 0;
            }
            if (skewer != 0 && (unsigned int)skewer->instance != 0U) {
                skewer->typed_vtbl->destroy(skewer);
            }
            skewer = p1_skewer_tip_item.object;
            if (skewer != 0) {
                if ((unsigned int)skewer->instance !=
                    p1_skewer_tip_item.instance) {
                    skewer = 0;
                }
            } else {
                skewer = 0;
            }
            if (skewer != 0 && (unsigned int)skewer->instance != 0U) {
                skewer->typed_vtbl->destroy(skewer);
            }
            _mkproc_sleep_ticks = 5.0f;
            aproc->vtbl->sleep();
        }

        sign = moveset->style_sign;
        if (sign != 0) {
            if ((unsigned int)sign->instance !=
                moveset->style_sign_instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        player_fstyle_sign[0] = sign;
        if (sign == 0) {
            mkproc_die();
        }
        insert_screen_obj(player_fstyle_sign[0]);

        fatality = 0;
        if (get_fatality_available_flag() != 0 &&
            g_game_info.pause_flag_bits.fatality_window &&
            check_for_winner() == 1) {
            fatality = 1;
        }
        if (fatality != 0 && f_p1_show_fatality_off == 0) {
            f_p1_showing_fatatality = 1;
            sign = load_2d_pfxobj(
                0x10005, 0x2084, (char*)0x2001C, 0, 0x2A);
            if (sign != 0) {
                pull_screen_obj(player_fstyle_sign[0]);
                player_fstyle_sign[0] = sign;
                sign->x = FSTYLE_LFT_START_X - 0x9D;
                sign->y = 0x20;
            }
        } else {
            f_p1_showing_fatatality = 0;
            player_fstyle_sign[0]->x =
                FSTYLE_LFT_START_X - moveset->definition->style_sign_width;
            player_fstyle_sign[0]->y = 0x20;
        }
        while (player_fstyle_sign[0]->x < FSTYLE_LFT_END_X) {
            player_fstyle_sign[0]->x += 0x1E;
            if (player_fstyle_sign[0]->x > FSTYLE_LFT_END_X) {
                player_fstyle_sign[0]->x = FSTYLE_LFT_END_X;
            }
            update_skewer_positions(0);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            if (player_fstyle_sign[0] == 0) {
                mkproc_die();
            }
        }
        player_fstyle_sign[0]->x = FSTYLE_LFT_END_X;
        update_skewer_positions(0);
        if (f_p1_showing_fatatality == 0) {
            f_p1_show_fatality_off = 0;
            f_p1_showing_fatatality = 0;
        }
    }

    if (player == 1) {
        if (f_p2_showing_fatatality != 0) {
            f_p2_show_fatality_off = 1;
            f_p2_showing_fatatality = 0;
        }
        if (player_fstyle_sign[1] != 0) {
            while (player_fstyle_sign[1]->x < FSTYLE_RGHT_START_X) {
                int old_x = player_fstyle_sign[1]->x;

                player_fstyle_sign[1]->x = old_x + 0x1E;
                update_skewer_positions(1);
                _mkproc_sleep_ticks = 1.0f;
                aproc->vtbl->sleep();
                if (player_fstyle_sign[1] == 0) {
                    mkproc_die();
                }
            }
            pull_screen_obj(player_fstyle_sign[1]);
            player_fstyle_sign[1] = 0;

            skewer = p2_skewer_item.object;
            if (skewer != 0) {
                if ((unsigned int)skewer->instance !=
                    p2_skewer_item.instance) {
                    skewer = 0;
                }
            } else {
                skewer = 0;
            }
            if (skewer != 0 && (unsigned int)skewer->instance != 0U) {
                skewer->typed_vtbl->destroy(skewer);
            }
            skewer = p2_skewer_tip_item.object;
            if (skewer != 0) {
                if ((unsigned int)skewer->instance !=
                    p2_skewer_tip_item.instance) {
                    skewer = 0;
                }
            } else {
                skewer = 0;
            }
            if (skewer != 0 && (unsigned int)skewer->instance != 0U) {
                skewer->typed_vtbl->destroy(skewer);
            }
            _mkproc_sleep_ticks = 5.0f;
            aproc->vtbl->sleep();
        }

        sign = moveset->style_sign;
        if (sign != 0) {
            if ((unsigned int)sign->instance !=
                moveset->style_sign_instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        player_fstyle_sign[1] = sign;
        if (sign == 0) {
            mkproc_die();
        }
        insert_screen_obj(player_fstyle_sign[1]);

        fatality = 0;
        if (get_fatality_available_flag() != 0 &&
            g_game_info.pause_flag_bits.fatality_window &&
            check_for_winner() == 2) {
            fatality = 1;
        }
        if (fatality != 0 && f_p2_show_fatality_off == 0) {
            f_p2_showing_fatatality = 1;
            sign = load_2d_pfxobj(
                0x10005, 0x2084, (char*)0x2001C, 0, 0x2A);
            if (sign != 0) {
                pull_screen_obj(player_fstyle_sign[1]);
                player_fstyle_sign[1] = sign;
            }
        }
        player_fstyle_sign[1]->x = FSTYLE_RGHT_START_X;
        player_fstyle_sign[1]->y = 0x20;
        width = f_p2_showing_fatatality != 0
            ? 0x9D : moveset->definition->style_sign_width;
        while (player_fstyle_sign[1]->x + width > FSTYLE_RGHT_END_X) {
            player_fstyle_sign[1]->x -= 0x1E;
            if (player_fstyle_sign[1]->x + width < FSTYLE_RGHT_END_X) {
                player_fstyle_sign[1]->x = FSTYLE_RGHT_END_X - width;
            }
            update_skewer_positions(1);
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            if (player_fstyle_sign[1] == 0) {
                mkproc_die();
            }
        }
        player_fstyle_sign[1]->x = FSTYLE_RGHT_END_X - width;
        update_skewer_positions(1);
        if (f_p2_showing_fatatality == 0) {
            f_p2_show_fatality_off = 0;
            f_p2_showing_fatatality = 0;
        }
    }
    return -1.0f;
}

void load_bgnd_fstyle_sign(int player) {
    GlobalMoveset* moveset;
    ScreenObj* sign;

    if (player < 2) {
        moveset = &global_movesets[player + 6];
        add_art_section_async(
            FSTYLE_SIGN_SLOT,
            find_section_by_name(moveset->definition->style_section_name));
        wait_for_slot_load(FSTYLE_SIGN_SLOT);
        sign = load_named_2d_pfxobj(
            FSTYLE_SIGN_SLOT, FSTYLE_SIGN_OID,
            moveset->definition->style_sign_name, 0,
            FSTYLE_SIGN_PRIORITY);
        if (sign != 0) {
            moveset->style_sign = sign;
            moveset->style_sign_instance = sign->instance;
            pull_screen_obj(sign);
        }
    }
}

void load_player_fstyle_signs(PlyrPdata* player) {
    GlobalMoveset* moveset;
    ScreenObj* sign;
    int player_index;
    int slot_group;
    int style_index;
    unsigned int style_offset;
    int slot;

    if (player == g_game_info.plyr0.slot.pdata) {
        p1_skewer_item.object = 0;
        p1_skewer_item.instance = 0;
        p1_skewer_tip_item.object = 0;
        p1_skewer_tip_item.instance = 0;
        f_p1_showing_fatatality = 0;
        f_p1_show_fatality_off = 0;
        player_index = 0;
        slot_group = 3;
    } else {
        p2_skewer_item.object = 0;
        p2_skewer_item.instance = 0;
        p2_skewer_tip_item.object = 0;
        p2_skewer_tip_item.instance = 0;
        f_p2_showing_fatatality = 0;
        f_p2_show_fatality_off = 0;
        player_index = 1;
        slot_group = 4;
    }

    style_offset = 0;
    for (style_index = 0; style_index < 3;
         style_index++, style_offset += sizeof(player->weapon_styles[0])) {
        moveset = (GlobalMoveset*)player->weapon_styles[
            style_offset / sizeof(player->weapon_styles[0])];
        slot = (slot_group << 16) |
               (unsigned short)(style_index + 13);
        if (moveset->definition != 0) {
            load_art_section(
                slot,
                find_section_by_name(
                    moveset->definition->style_section_name));
            sign = load_named_2d_pfxobj(
                slot, FSTYLE_SIGN_OID,
                moveset->definition->style_sign_name, 0,
                FSTYLE_SIGN_PRIORITY);
            moveset->style_sign = sign;
            moveset->style_sign_instance = sign->instance;
            pull_screen_obj(sign);
        }
    }
    player_fstyle_sign[player_index] = 0;
}

static inline ScreenObj* moveset_live_style_sign(GlobalMoveset* owner) {
    ScreenObj* object = owner->style_sign;
    if (object != 0) {
        if ((unsigned int)object->instance == owner->style_sign_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

void kill_all_fstyle_signs(void) {
    int player;
    GlobalMoveset* moveset;
    ScreenObj* sign;

    kill_fstyle_signs_for_plyr(&g_game_info.plyr0);
    kill_fstyle_signs_for_plyr(&g_game_info.plyr1);
    for (player = 0; player < 2; player++) {
        moveset = &global_movesets[player + 6];
        if (moveset != 0) {
            sign = moveset_live_style_sign(moveset);

            if (sign != 0) {
                if ((unsigned int)sign->instance != 0) {
                    sign->typed_vtbl->destroy(sign);
                }
                moveset->style_sign = 0;
                moveset->style_sign_instance = 0;
            }
        }
    }
}

void kill_fstyle_signs_for_plyr(PlyrInfo* player) {
    GlobalMoveset* moveset;
    ScreenObj* sign;
    PlyrPdata* player_data;
    int style_index;

    if (player == 0 || player->slot.pdata == 0) {
        return;
    }

    player_data = player->slot.pdata;
    for (style_index = 0; style_index < 3; style_index++) {
        moveset = (GlobalMoveset*)player_data->weapon_styles[style_index];
        sign = moveset->style_sign;
        if (sign != 0) {
            if ((unsigned int)sign->instance !=
                moveset->style_sign_instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        if (sign != 0) {
            if ((unsigned int)sign->instance != 0U) {
                sign->typed_vtbl->destroy(sign);
            }
            moveset->style_sign = 0;
            moveset->style_sign_instance = 0;
        }
    }

    if (player->controller_slot == 0) {
        sign = p1_skewer_item.object;
        if (sign != 0) {
            if ((unsigned int)sign->instance != p1_skewer_item.instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        if (sign != 0 && (unsigned int)sign->instance != 0U) {
            sign->typed_vtbl->destroy(sign);
        }

        sign = p1_skewer_tip_item.object;
        if (sign != 0) {
            if ((unsigned int)sign->instance !=
                p1_skewer_tip_item.instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        if (sign != 0 && (unsigned int)sign->instance != 0U) {
            sign->typed_vtbl->destroy(sign);
        }
    } else {
        sign = p2_skewer_item.object;
        if (sign != 0) {
            if ((unsigned int)sign->instance != p2_skewer_item.instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        if (sign != 0 && (unsigned int)sign->instance != 0U) {
            sign->typed_vtbl->destroy(sign);
        }

        sign = p2_skewer_tip_item.object;
        if (sign != 0) {
            if ((unsigned int)sign->instance !=
                p2_skewer_tip_item.instance) {
                sign = 0;
            }
        } else {
            sign = 0;
        }
        if (sign != 0 && (unsigned int)sign->instance != 0U) {
            sign->typed_vtbl->destroy(sign);
        }
    }
    player_fstyle_sign[player->controller_slot] = 0;
}

RpAtomic* set_atomic_material_specular(RpAtomic* atomic,
                                       unsigned int specular) {
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(
            atomic->geometry, material_set_specular, (void*)specular);
    }
    return atomic;
}

static RpMaterial* material_set_specular(RpMaterial* material,
                                         void* data) {
    unsigned int specular = (unsigned int)data;
    RpSurfaceProperties surface;

    surface.ambient = material->surface.ambient;
    surface.specular =
        ((float)specular / 255.0f) * material->surface.specular;
    surface.diffuse = material->surface.diffuse;
    material->surface = surface;
    return material;
}

static RpMaterial* material_set_alpha(RpMaterial* material,
                                      void* data) {
    unsigned int alpha = (unsigned int)data;
    RwRGBA color;

    color.red = material->color.red;
    color.green = material->color.green;
    color.blue = material->color.blue;
    color.alpha = alpha;
    material->color = color;
    return material;
}

RpAtomic* set_atomic_material_alpha(RpAtomic* atomic, unsigned int alpha) {
    RpGeometry* geometry = atomic->geometry;

    if (geometry != 0) {
        geometry->flags |= 0x40;
        RpGeometryForAllMaterials(
            geometry, material_set_alpha, (void*)alpha);
    }
    return atomic;
}

static inline MkObj* mirror_latch_live_obj(PlyrMirrorObjLatch* owner) {
    MkObj* object = owner->obj;
    if (object != 0) {
        if (object->hdr.instance == owner->instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 99.318184%; register coloring, relocation offsets; one-trial ceiling. */
static float p_freeze_light(void) {
    FreezeLightPdata* pdata;
    PlyrMirrorObjLatch* item;
    MkObj* light;

    pdata = (FreezeLightPdata*)apdata;
    item = pdata->light;
    light = mirror_latch_live_obj(item);


    if (pdata->player->state_flags.bits.frozen == 0) {
        if (light != 0 && light->hdr.instance != 0) {
            light->hdr.typed_vtbl->destroy(&light->hdr);
        }
        return -1.0f;
    }

    if (camera_obj != 0) {
        light->ang.x = camera_obj->ang.x;
        light->ang.y = camera_obj->ang.y;
    }
    return 1.0f;
}

void unfreeze_player(void) {
    PlyrMirrorObjLatch* light_latch;
    FxHdrLatch* proc_latch;
    PlyrPdata* player;
    PlyrMirrorObjLatch* object_latch;
    MkObj* player_object;
    MkObj* object;
    MkHdr* hdr;

    if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
        player_object = g_game_info.plyr0.slot.mirror_a;
        player = g_game_info.plyr0.slot.pdata;
        light_latch = &p1_freeze_light_item;
        proc_latch = &p1_freeze_proc_item;
    } else {
        player_object = g_game_info.plyr1.slot.mirror_a;
        player = g_game_info.plyr1.slot.pdata;
        light_latch = &p2_freeze_light_item;
        proc_latch = &p2_freeze_proc_item;
    }

    if (player_object == 0 || player == 0) {
        return;
    }
    if (!player->state_flags.bits.frozen) {
        debug_print_message(
            &fx_string_base[156]);
        return;
    }

    player_object->flags_0B_bits.special_texture = 0;
    RpClumpForAllAtomics(
        player_object->clump,
        restore_specular_texture_atomic_callback, 0);
    player->state_flags.bits.frozen = 0;

    object_latch = &player->mirror_slots->weapon[0].primary;
    object = object_latch->obj;
    if (object != 0) {
        if (object->hdr.instance != object_latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        object->flags_0B_bits.special_texture = 0;
        RpClumpForAllAtomics(
            object->clump, restore_specular_texture_atomic_callback, 0);
    }

    object_latch = &player->mirror_slots->weapon[1].primary;
    object = object_latch->obj;
    if (object != 0) {
        if (object->hdr.instance != object_latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        object->flags_0B_bits.special_texture = 0;
        RpClumpForAllAtomics(
            object->clump, restore_specular_texture_atomic_callback, 0);
    }

    object_latch = &player->aux_weapon_latch;
    object = object_latch->obj;
    if (object != 0) {
        if (object->hdr.instance != object_latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        object->flags_0B_bits.special_texture = 0;
        RpClumpForAllAtomics(
            object->clump, restore_specular_texture_atomic_callback, 0);
    }

    player_object->light_flags = 0x1004;
    object = light_latch->obj;
    if (object != 0) {
        if (object->hdr.instance != light_latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0 && object->hdr.instance != 0U) {
        object->hdr.typed_vtbl->destroy(&object->hdr);
    }

    hdr = proc_latch->object;
    if (hdr != 0) {
        if (hdr->instance != proc_latch->instance) {
            hdr = 0;
        }
    } else {
        hdr = 0;
    }
    if (hdr != 0 && hdr->instance != 0U) {
        hdr->typed_vtbl->destroy(hdr);
    }
}

void turn_into_energy_player(void) {
    void* texture;

    if (plyr_pdata->plyr_num == 0) {
        texture = load_named_tga_from_slot(
            0x3000B, &fx_string_base[203]);
        apply_special_fx_to_player(texture);
        return;
    }
    texture = load_named_tga_from_slot(
        0x4000B, &fx_string_base[203]);
    apply_special_fx_to_player(texture);
}

void freeze_player(void) {
    void* texture;

    texture = load_tga(FREEZE_TEXTURE_HANDLE, FREEZE_TEXTURE_OID);
    apply_special_fx_to_player(texture);
}

/* TODO: [near miss] 97.919260%; register coloring, relocation offsets; one-trial ceiling. */
static void apply_special_fx_to_player(void* texture) {
    FreezeLightPdata* proc_data;
    PlyrMirrorObjLatch* light_latch;
    FxHdrLatch* proc_latch;
    PlyrMirrorObjLatch* object_latch;
    PlyrPdata* player;
    MkObj* player_object;
    MkObj* object;
    MkObj* light;
    MkProc* proc;
    int proc_id;

    if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
        player_object = g_game_info.plyr0.slot.mirror_a;
        player = g_game_info.plyr0.slot.pdata;
        light_latch = &p1_freeze_light_item;
        proc_latch = &p1_freeze_proc_item;
        proc_id = 0x2039;
    } else {
        player_object = g_game_info.plyr1.slot.mirror_a;
        player = g_game_info.plyr1.slot.pdata;
        light_latch = &p2_freeze_light_item;
        proc_latch = &p2_freeze_proc_item;
        proc_id = 0x203A;
    }

    if (player_object == 0 || plyr_pdata == 0 ||
        plyr_pdata->state_flags.bits.frozen) {
        return;
    }

    player_object->flags_0B_bits.special_texture = 1;
    RpClumpForAllAtomics(
        player_object->clump,
        swap_specular_texture_atomic_callback, texture);
    plyr_pdata->state_flags.bits.frozen = 1;

    object_latch = &plyr_pdata->mirror_slots->weapon[0].primary;
    object = mirror_latch_live_obj(object_latch);

    if (object != 0) {
        object->flags_0B_bits.special_texture = 1;
        RpClumpForAllAtomics(
            object->clump, swap_specular_texture_atomic_callback, texture);
    }

    object_latch = &plyr_pdata->mirror_slots->weapon[1].primary;
    object = mirror_latch_live_obj(object_latch);

    if (object != 0) {
        object->flags_0B_bits.special_texture = 1;
        RpClumpForAllAtomics(
            object->clump, swap_specular_texture_atomic_callback, texture);
    }

    object_latch = &plyr_pdata->aux_weapon_latch;
    object = mirror_latch_live_obj(object_latch);

    if (object != 0) {
        object->flags_0B_bits.special_texture = 1;
        RpClumpForAllAtomics(
            object->clump, swap_specular_texture_atomic_callback, texture);
    }

    light = load_light(
        (LightDef*)&plyr_freeze_light, &freeze_light_list, 0);
    if (light != 0) {
        light_latch->obj = light;
        light_latch->instance = light->hdr.instance;
        proc = _create_mkproc_generic_tinystack(
            proc_id, 0x1F, p_freeze_light, sizeof(*proc_data),
            (MkHdr**)&proc_data);
        if (proc != 0) {
            proc_latch->object = &proc->hdr;
            proc_latch->instance = proc->instance;
            proc_data->light = light_latch;
            proc_data->player_object = player_object;
            proc_data->player = player;
            player_object->light_flags = 0x20;
            if (his_pdata->character_id == 0x19 ||
                his_pdata->character_id == 0x1A) {
                snd_req(0x314);
            } else {
                snd_req(0x348);
            }
        }
    }
}

int can_i_do_fatality_now(int player) {
    int winner;

    if (get_fatality_available_flag() == 0) {
        return 0;
    }
    if (g_game_info.pause_flag_bits.fatality_window != 0) {
        winner = check_for_winner();
    } else {
        return 0;
    }
    if (player == 0 && winner == 1) {
        return 1;
    }
    if (player == 1 && winner == 2) {
        return 1;
    }
    return 0;
}

MkPfx* create_pfx(
    int bind_source, int process_id, MkProcEntryFn entry,
    MkPfx** effect_out, const FxPfxDefinition* definition,
    const char* name) {
    PfxBuildInfo build;
    FxPfxVmView* vm;
    PfxEmitter* emitter;
    void* behavior;
    MkPfx* effect;
    Vec* origin;
    float lifetime;

    if (definition == 0) {
        return 0;
    }

    memset(&build, 0, sizeof(build));
    build.name = (char*)(name != 0 ? name : &fx_string_base[215]);
    if (definition->kill_plane_x != 0 ||
        definition->kill_plane_y != 0 ||
        definition->kill_plane_z != 0 ||
        definition->lifetime_mode != 0) {
        build.behavior_count = 1;
    }
    build.emitter_count = 1;
    effect = (MkPfx*)new_pfx_create_raw_userdata(
        &build, 0, definition->field_90,
        definition->field_04, definition->field_08,
        definition->initialize, process_id, entry,
        (void**)effect_out);
    if (effect == 0) {
        return 0;
    }

    if ((definition->flags & 1) != 0 &&
        pfx_bind_to_new_obj(*effect_out, bind_source) == 0) {
        if (effect->hdr.instance != 0U) {
            effect->hdr.typed_vtbl->destroy(&effect->hdr);
        }
        *effect_out = 0;
        return 0;
    }

    vm = (FxPfxVmView*)&(*effect_out)->matrix;
    origin = (Vec*)pfx_get_field((PfxVm*)vm, 0, 0x200);
    origin->x = definition->origin.x;
    origin->y = definition->origin.y;
    origin->z = definition->origin.z;
    if ((definition->field_04 & 0x20) == 0) {
        vm->flag_bits.sized = 1;
        vm->particle_size = definition->particle_size;
    } else {
        vm->flag_bits.sized = 0;
    }
    vm->flag_bits.enabled = 1;
    pfx_native_set_rgba(
        &vm->color, definition->red, definition->green,
        definition->blue, definition->alpha);

    emitter = pfx_get_emitter((PfxVm*)vm, 0);
    emitter->lifetime = (float)definition->emitter_lifetime;
    vm->field_50 = definition->field_90;
    emitter = pfx_get_emitter((PfxVm*)vm, 0);
    emitter->field_40 = definition->emitter_field_40;

    if ((definition->flags & 4) == 0) {
        if (definition->texture != 0) {
            set_pfx_texture((PfxVm*)vm, (void*)0x10005,
                            definition->texture);
        }
        if (definition->animate_texture != 0) {
            pfx_texture_animate(
                (PfxVm*)vm, definition->texture_width,
                definition->texture_height,
                definition->texture_frame_width,
                definition->texture_speed);
            vm->texture_enabled = definition->texture_enabled;
        }
    }

    if (definition->kill_plane_x != 0) {
        behavior = pfx_behavior((PfxVm*)vm, 0);
        pfxvm_kill_on_intersect_plane_x(behavior, definition->plane_x);
    }
    if (definition->kill_plane_y != 0) {
        behavior = pfx_behavior((PfxVm*)vm, 0);
        pfxvm_kill_on_intersect_plane_y(behavior, definition->plane_y);
    }
    if (definition->kill_plane_z != 0) {
        behavior = pfx_behavior((PfxVm*)vm, 0);
        pfxvm_kill_on_intersect_plane_z(behavior, definition->plane_z);
    }

    if (definition->lifetime_mode != 0) {
        if (definition->lifetime_mode == -1) {
            lifetime = (definition->texture_frame_width *
                        definition->texture_speed) - 1.0f;
        } else {
            lifetime = definition->lifetime_mode;
        }
        emitter = pfx_get_emitter((PfxVm*)vm, 0);
        pfxvm_spawn_line_1f(
            emitter, 0x301, (float)definition->lifetime_minimum,
            (float)definition->lifetime_maximum);
        behavior = pfx_behavior((PfxVm*)vm, 0);
        pfxvm_kill_on_greater(behavior, 0x301, lifetime);
    }
    if ((definition->flags & 2) != 0) {
        pfxvm_compile((PfxVm*)vm);
    }
    (*effect_out)->flags |= 0x10;
    return effect;
}
