#include "runtime/asset.h"
#include "game/game_info.h"
#include "game/moveset.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"

#define YINYANG_LENSFLARE_PID 0x3019
#define FREEZE_TEXTURE_HANDLE 0x10005
#define FREEZE_TEXTURE_OID 0x2003B

#if !defined(TARGET_PC)
#pragma use_lmw_stmw on
#endif

/* Retail TU-local; its body remains in the split assembly. */
void apply_special_fx_to_player(void);

typedef struct FreezeLightPdata {
    MkHdr hdr;
    PlyrMirrorObjLatch* light;
    char pad0C[4];
    PlyrPdata* player;
} FreezeLightPdata;

typedef struct LensflarePdata {
    MkHdr hdr;
    void* sun_position;
    const void* lens_data;
    void* obstructions;
    int courtyard;
} LensflarePdata;

typedef struct FxScreenObjLatch {
    ScreenObj* object;
    unsigned int instance;
} FxScreenObjLatch;

typedef struct FxRayPlane {
    int axis;
    float coordinate;
    Vec minimum;
    Vec maximum;
} FxRayPlane;

extern unsigned char courtyard_sun_pos[];
extern unsigned char courtyard_flare_obstructions[];
extern const unsigned char courtyard_lens_data[];
extern unsigned char yinyang_sun_pos[];
extern unsigned char yinyang_flare_obstructions[];
extern const unsigned char yinyang_lens_data[];
static float lensflare_proc(void);

typedef RpMaterial* (*FxMaterialCallback)(RpMaterial*, unsigned int);

extern CameraObj* camera_obj;
extern int f_p1_showing_fatatality;
extern int screen_width;
extern ScreenObj* player_fstyle_sign[2];
extern FxScreenObjLatch p1_skewer_item;
extern FxScreenObjLatch p1_skewer_tip_item;
extern FxScreenObjLatch p2_skewer_item;
extern FxScreenObjLatch p2_skewer_tip_item;
extern int check_for_winner(void);
extern int get_fatality_available_flag(void);
extern void kill_fstyle_signs_for_plyr(PlyrInfo* player);
extern RpGeometry* RpGeometryForAllMaterials(
    RpGeometry* geometry, FxMaterialCallback callback, unsigned int data);
static RpMaterial* material_set_specular(RpMaterial* material,
                                         unsigned int specular);
static int SKEWER_LEFT_OVERHANG = 0x14;
static int SKEWER_RIGHT_OVERHANG = 0xA;

static int rayintersection(
    const Vec* origin, const Vec* direction, const FxRayPlane* plane) {
    Vec intersection = {0.0f, 0.0f, 0.0f};
    float distance;

    switch (plane->axis) {
    case 0:
        if (direction->x < 0.00001 && direction->x > -0.00001) {
            return 0;
        }
        distance = (plane->coordinate - origin->x) / direction->x;
        intersection.y = distance * direction->y + origin->y;
        intersection.z = distance * direction->z + origin->z;
        return intersection.y > plane->minimum.y &&
               intersection.y < plane->maximum.y &&
               intersection.z > plane->minimum.z &&
               intersection.z < plane->maximum.z;
    case 1:
        if (direction->y < 0.00001 && direction->y > -0.00001) {
            return 0;
        }
        distance = (plane->coordinate - origin->y) / direction->y;
        intersection.x = distance * direction->x + origin->x;
        intersection.z = distance * direction->z + origin->z;
        return intersection.x > plane->minimum.x &&
               intersection.x < plane->maximum.x &&
               intersection.z > plane->minimum.z &&
               intersection.z < plane->maximum.z;
    default:
        if (direction->z < 0.00001 && direction->z > -0.00001) {
            return 0;
        }
        distance = (plane->coordinate - origin->z) / direction->z;
        intersection.x = distance * direction->x + origin->x;
        intersection.y = distance * direction->y + origin->y;
        return intersection.x > plane->minimum.x &&
               intersection.x < plane->maximum.x &&
               intersection.y > plane->minimum.y &&
               intersection.y < plane->maximum.y;
    }
}

static inline ScreenObj* fx_live_screen_obj(FxScreenObjLatch* latch) {
    ScreenObj* object;

    object = latch->object;
    if (object != 0 && (unsigned int)object->instance != latch->instance) {
        object = 0;
    }
    return object;
}

#define FSTYLE_SIGN_SLOT 0x2001E
#define FSTYLE_SIGN_OID 0x3002
#define FSTYLE_SIGN_PRIORITY 0x2A

void yinyang_stop_lensflare(void) {
    destroy_mkprocs_pid(YINYANG_LENSFLARE_PID);
}

void courtyard_start_lensflare(void) {
    LensflarePdata* pdata;

    _create_mkproc_generic_tinystack(
        YINYANG_LENSFLARE_PID, 0x20, lensflare_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    pdata->sun_position = courtyard_sun_pos;
    pdata->lens_data = courtyard_lens_data;
    pdata->obstructions = courtyard_flare_obstructions;
    pdata->courtyard = 1;
}

void yinyang_start_lensflare(void) {
    LensflarePdata* pdata;

    _create_mkproc_generic_tinystack(
        YINYANG_LENSFLARE_PID, 0x20, lensflare_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    pdata->sun_position = yinyang_sun_pos;
    pdata->lens_data = yinyang_lens_data;
    pdata->obstructions = yinyang_flare_obstructions;
    pdata->courtyard = 0;
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

void kill_all_fstyle_signs(void) {
    int player;
    GlobalMoveset* moveset;
    ScreenObj* sign;

    kill_fstyle_signs_for_plyr(&g_game_info.plyr0);
    kill_fstyle_signs_for_plyr(&g_game_info.plyr1);
    for (player = 0; player < 2; player++) {
        moveset = &global_movesets[player + 6];
        if (moveset != 0) {
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
                if ((unsigned int)sign->instance != 0) {
                    ((int (*)(MkHdr*))sign->vtbl->destroy)((MkHdr*)sign);
                }
                moveset->style_sign = 0;
                moveset->style_sign_instance = 0;
            }
        }
    }
}

/* Soft ceiling: update_skewer_positions ~70.66% -- typed latches/layout; stop. */
static void update_skewer_positions(int player) {
    FxScreenObjLatch* body_latch;
    FxScreenObjLatch* tip_latch;
    ScreenObj* body;
    ScreenObj* tip;
    ScreenObj* sign;
    PlyrPdata* player_data;
    int flags;

    body = 0;
    tip = 0;
    flags = 0;
    if (player == 0) {
        body_latch = &p1_skewer_item;
        tip_latch = &p1_skewer_tip_item;
        body = fx_live_screen_obj(body_latch);
        if (body == 0) {
            ((unsigned char*)&flags)[0] &= ~0x20;
            ((unsigned char*)&flags)[0] |= 8;
            body = load_2d_pfxobj(
                0x10005, 0x2052, (char*)0x2001A, flags, 0x2B);
            if (body != 0) {
                body_latch->object = body;
                body_latch->instance = body->instance;
                tip = load_2d_pfxobj(
                    0x10005, 0x2052, (char*)0x2001B, 0, 0x2B);
                if (tip != 0) {
                    tip_latch->object = tip;
                    tip_latch->instance = tip->instance;
                }
            }
        } else {
            tip = fx_live_screen_obj(tip_latch);
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
        body_latch = &p2_skewer_item;
        tip_latch = &p2_skewer_tip_item;
        body = fx_live_screen_obj(body_latch);
        if (body == 0) {
            ((unsigned char*)&flags)[0] &= ~0x20;
            ((unsigned char*)&flags)[0] |= 8;
            body = load_2d_pfxobj(
                0x10005, 0x2053, (char*)0x2001A, flags, 0x2B);
            if (body != 0) {
                body_latch->object = body;
                body_latch->instance = body->instance;
                ((unsigned char*)&flags)[0] |= 0x20;
                ((unsigned char*)&flags)[0] &= ~8;
                tip = load_2d_pfxobj(
                    0x10005, 0x2053, (char*)0x2001B, flags, 0x2B);
                if (tip != 0) {
                    tip_latch->object = tip;
                    tip_latch->instance = tip->instance;
                }
            }
        } else {
            tip = fx_live_screen_obj(tip_latch);
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

RpAtomic* set_atomic_material_specular(RpAtomic* atomic,
                                       unsigned int specular) {
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(
            atomic->geometry, material_set_specular, specular);
    }
    return atomic;
}

static RpMaterial* material_set_specular(RpMaterial* material,
                                         unsigned int specular) {
    RpSurfaceProperties surface;

    surface.ambient = material->surface.ambient;
    surface.specular =
        ((float)specular / 255.0f) * material->surface.specular;
    surface.diffuse = material->surface.diffuse;
    material->surface = surface;
    return material;
}

static RpMaterial* material_set_alpha(RpMaterial* material,
                                      unsigned int alpha) {
    RpMaterialColor color;

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
            geometry, material_set_alpha, alpha);
    }
    return atomic;
}

static float p_freeze_light(void) {
    FreezeLightPdata* pdata;
    PlyrMirrorObjLatch* item;
    MkObj* light;

    pdata = (FreezeLightPdata*)apdata;
    item = pdata->light;
    light = item->obj;
    if (light != 0) {
        if (light->hdr.instance != item->instance) {
            light = 0;
        }
    } else {
        light = 0;
    }

    if (pdata->player->state_flags.bits.frozen == 0) {
        if (light != 0 && light->hdr.instance != 0) {
            ((int (*)(MkHdr*))light->hdr.vtbl->destroy)(&light->hdr);
        }
        return -1.0f;
    }

    if (camera_obj != 0) {
        light->ang.x = camera_obj->ang_x;
        light->ang.y = camera_obj->ang_y;
    }
    return 1.0f;
}

void turn_into_energy_player(void) {
    if (plyr_pdata->plyr_num == 0) {
        load_named_tga_from_slot(0x3000B, "TELE_ENERGY");
        apply_special_fx_to_player();
        return;
    }
    load_named_tga_from_slot(0x4000B, "TELE_ENERGY");
    apply_special_fx_to_player();
}

void freeze_player(void) {
    load_tga(FREEZE_TEXTURE_HANDLE, FREEZE_TEXTURE_OID);
    apply_special_fx_to_player();
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
