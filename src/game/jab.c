#include "libmkparticle/texture_anim.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "game/specular.h"
#include "math/mk_math.h"
#include "game/game_info.h"
#include "game/plyr.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/image.h"
#include "runtime/light.h"
#include "runtime/utils.h"
#include "platform/main.h"

#define FLASH_SCREEN_PID 0x2098
#define DRAGON_KING_SHAKE_PID 0xA029
#define POINT_LIGHT_TRACKER_PID 0xA02A
#define JADE_BIND_PID 0xA027
#define JADE_CHARACTER_ID 0x15
#define KABAL_DASH_REACTION_STATE 0x421A
#define MKOBJ_FLAG_SCALE_ACTIVE 0x02
#define MKOBJ_FLAG_JADE_ATTACHED 0x08
#define MKOBJ_FLAG_JADE_VISIBLE 0x40
#define MKSOBJ_FLAG_UPDATE_ANGULAR 0x04

extern MkObj* plyr_obj;
extern MkPtr* clone_light_list;
extern MkPtr* point_light_list;
extern float game_speed;

typedef struct JabPfxDefinition {
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
    int field_30;
    int emitter_field_40;
    unsigned int texture_handle;
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
} JabPfxDefinition; /* 0x7C */

const JabPfxDefinition jab_pfx_table[8] = {
    {
        4, 0x232, 3, {0.0f, 0.0f, 0.0f}, 0.5f,
        255.0f, 255.0f, 255.0f, 255.0f,
        1, 100, 0, 0x00020038, 0, 0, 0, 0, 0.0f, 0,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        5, 0x232, 3, {0.0f, 0.0f, 0.0f}, 0.5f,
        255.0f, 255.0f, 255.0f, 255.0f,
        20, 100, 0, 0x08160005, 0x40, 0x10, 0x10, 0x10, 2.0f, 0,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        4, 0x232, 3, {0.0f, 0.0f, 0.0f}, 0.1f,
        255.0f, 255.0f, 255.0f, 255.0f,
        12, 0x168, 0, 0x00020038, 0x10, 8, 8, 4, 3.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        4, 0x32, 3, {0.0f, 0.0f, 0.0f}, 0.1f,
        255.0f, 255.0f, 255.0f, 255.0f,
        8, 0x1EF, 0, 0x00020038, 0x10, 8, 8, 4, 3.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        5, 0x272, 3, {0.0f, 0.0f, 0.0f}, 0.0f,
        255.0f, 255.0f, 255.0f, 255.0f,
        15, 0x12C, 0, 0x013F000E, 0x40, 0x10, 0x10, 0x10, 4.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        5, 0x272, 0x43, {0.0f, 0.0f, 0.0f}, 0.0f,
        255.0f, 255.0f, 255.0f, 255.0f,
        5, 0x12C, 0, 0x00020034, 0x80, 0x20, 0x20, 0x10, 4.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        5, 0x272, 3, {0.0f, 0.0f, 0.0f}, 0.0f,
        255.0f, 255.0f, 255.0f, 255.0f,
        2, 0x82, 0, 0x013F0010, 0x80, 0x20, 0x20, 0x10, 4.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
    {
        5, 0x272, 3, {0.0f, 0.0f, 0.0f}, 0.0f,
        255.0f, 255.0f, 255.0f, 255.0f,
        2, 0x82, 0, 0x013F0013, 0x100, 0x40, 0x55, 0x0C, 4.0f, 1,
        0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0,
    },
};

/* Serialized smoke-emitter preset consumed by the effect-table runtime. */
unsigned char gSmokeTable[0x58] = {
    0x00, 0x00, 0x00, 0x00, 0xC8, 0xC8, 0xC8, 0xFF,
    0xC8, 0xC8, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x08,
    0x3E, 0x19, 0x99, 0x9A, 0x3D, 0xA3, 0xD7, 0x0A,
    0x3B, 0x44, 0x9B, 0xA6, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x24, 0x43, 0xAF, 0x00, 0x00,
    0x42, 0xC8, 0x00, 0x00, 0x3B, 0x44, 0x9B, 0xA6,
    0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x10, 0x40, 0x80, 0x00, 0x00,
};

int skeleton_start_bone_id[2][12] = {
    {24, 7, 7, 4, 10, 15, 17, 8, 8, 5, 11, 17},
    {15, 4, 4, 1, 10, 15, 21, 8, 8, 5, 11, 17},
};

int skeleton_end_bone_id[2][12] = {
    {20, 4, 4, 1, 7, 1, 21, 5, 5, 2, 8, 2},
    {20, 7, 7, 4, 7, 1, 25, 5, 5, 2, 8, 2},
};

float skeleton_radius_table[12] = {
    0.12f, 0.12f, 0.14f, 0.14f, 0.14f, 0.14f,
    0.12f, 0.14f, 0.14f, 0.14f, 0.14f, 0.14f,
};

unsigned int g_kabal_dash_react_pfx_handles[9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static MkPfx* p_grinder_crush_blood_pfx;
static MkPfx* p_grinder_crush_chunks_pfx;
PlyrPdata* g_plyr_pdata;
static float grinder_meat_size = 0.3f;

typedef struct FlashScreenPdata {
    MkHdr hdr;
    int color;
    float intensity;
    float duration;
} FlashScreenPdata;

typedef union FlashScreenPdataRef {
    MkHdr* hdr;
    FlashScreenPdata* flash;
} FlashScreenPdataRef;

typedef void (*JabProcDestroyFn)(MkProc* proc);

typedef struct JabProcVtablePrefix {
    void* reserved[4];
    JabProcDestroyFn destroy;
} JabProcVtablePrefix;

typedef union JabProcVtableRef {
    MkVtableMkproc* base;
    JabProcVtablePrefix* jab;
} JabProcVtableRef;

typedef struct JabProcSleepVtable {
    char pad00[0x18];
    int (*sleep)(void); /* +0x18 */
} JabProcSleepVtable;

typedef struct JabBoneMatcherState {
    MkHdr hdr;
    union {
        unsigned char flags_08;
        struct {
            unsigned char inactive : 1;
            unsigned char copy_bone_matrix : 1;
            unsigned char copy_clone_matrix : 1;
            unsigned char preserve_bone_matrix : 1;
            unsigned char copy_parent_angles : 1;
            unsigned char flip_parent_angle_y : 1;
            unsigned char release_parent_weight : 1;
            unsigned char blend_child_transform : 1;
        } flags_08_bits;
    };
} JabBoneMatcherState;

typedef struct JabSplatterState {
    char pad00[0x40];
    union {
        unsigned char flags;
        struct {
            unsigned char pad_high : 3;
            unsigned char bit4 : 1;
            unsigned char bit3 : 1;
            unsigned char pad_low : 3;
        } flags_bits;
    };
} JabSplatterState;

typedef struct JabObjectRef {
    MkHdr* object;
    unsigned int instance;
} JabObjectRef;

#define RESOLVE_JAB_OBJECT(result, object, expected_instance)              \
    do {                                                                  \
        MkObj* candidate_;                                                \
        candidate_ = (object);                                            \
        if (candidate_ != 0) {                                            \
            if (candidate_->hdr.instance == (expected_instance)) {        \
                (result) = candidate_;                                    \
            } else {                                                      \
                (result) = 0;                                             \
            }                                                             \
        } else {                                                          \
            (result) = 0;                                                 \
        }                                                                 \
    } while (0)

typedef struct JabPointLightPdata {
    MkHdr hdr;
    MkObj* tracked_object;
    unsigned int tracked_object_instance;
    MkObj* light;
    unsigned int light_instance;
    int bone;
} JabPointLightPdata;

typedef struct DragonKingShakePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float distance;
    float speed;
    float base_y;
} DragonKingShakePdata;

typedef struct JadeBindPdata {
    MkHdr hdr;
    MkObj* child;
    unsigned int child_instance;
    MkObj* parent;
    unsigned int parent_instance;
    int parent_bone;
} JadeBindPdata;

typedef union JabFloatBits {
    float f;
    unsigned int u;
} JabFloatBits;

JabBoneMatcherState* start_bone_matcher(
    float blend, MkObj* parent, int parent_bone, MkObj* child,
    int child_bone);
void bone_matcher_parent_set_offset(
    JabBoneMatcherState* matcher, const float* offset);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
unsigned int pfxhandle_bgnd_spawn_at_position(
    const char* effect_name, float x, float y, float z);
static void sh_spawn_grinder_crush_blood(void);
int am_i_flipped(void);
void* load_named_model_for_player(
    char* name, int player, int object_type, int flags);
void fx_reset_emit(unsigned int effect);
int snd_req(int sound_id);
MkPfx* create_pfx(
    int bind_source, int process_id, float (*entry)(void), MkPfx** effect,
    const void* definition, const char* name);
PfxEmitter* pfx_get_emitter(PfxVm* vm, int index);
void* pfx_get_field(PfxVm* vm, int emitter_index, int field);
static float pfx_sh_grinder_crush_blood(void);
static float pfx_sh_grinder_crush_chunks(void);
static float pfx_sh_grinder_meat_spew(void);
static float pfx_kenshi_lift_smoke(void);
static float pfx_react_falling_attach_smoke_to_bones_proc(void);
MkObj* get_mkobj_frame(int type, void* frame);
void pfxhandle_spawn_at_bid(const char* name, MkObj* object, int bone);
/* Retail call sites use both three- and five-argument local declarations. */
void spawn_bld_splat();
unsigned int fx_by_owner(const char* name, int owner);
int pfx_plyr_bankowner(PlyrInfo* player);
unsigned int pfxhandle_spawn_at_bid_next(
    unsigned int effect, MkObj* object, int bone);
MkProc* find_mkproc_pid(int process_id);

static float p_flash_screen(void);
static float p_jab_point_light_tracker(void);
static float p_dk_death_shake(void);
static float p_bind_obj_to_obj_bone(void);
static void jab_kira_projectile_hand_explode(void);
static float p_kabal_dash_react_pfx(void);

void initialize_clone_lights(LightDef** definitions) {
    MkObj* light;

    if (plyr_pdata != 0 && clone_light_list == 0) {
        get_row_count_for_table_by_pointer(plyr_pdata->cmo, definitions);
        if (definitions[0] != 0) {
            light = load_light(definitions[0], &clone_light_list, 0);
            if (light != 0) {
                mk_insert(&light->hdr, &plyr_obj->child_list);
            }
        }
        if (definitions[1] != 0) {
            light = load_light(definitions[1], &clone_light_list, 0);
            if (light != 0) {
                mk_insert(&light->hdr, &plyr_obj->child_list);
            }
        }
    }
}

MkObj* jab_spawn_point_light_at_world_pos(
    LightDef* definition, const Vec* position) {
    MkObj* light;
    MkHdr* light_hdr;

    light = load_light(definition, &point_light_list, 0);
    if (light != 0) {
        light->pos.value.x = position->x;
        light->pos.value.y = position->y;
        light->pos.value.z = position->z;
        if (light != 0) {
            light_hdr = as_mkhdr(&light->hdr);
        } else {
            light_hdr = 0;
        }
        update_mkobj(light_hdr);
        return light;
    }
    return 0;
}

MkObj* jab_attach_point_light_to_obj_bone(
    LightDef* definition, MkObj* object, int bone) {
    JabPointLightPdata* pdata;
    MkObj* light;
    MkProc* proc;

    light = load_light(definition, &point_light_list, 0);
    if (light == 0) {
        return 0;
    }

    proc = _create_mkproc_generic_tinystack(
        POINT_LIGHT_TRACKER_PID, 0x1F, p_jab_point_light_tracker,
        sizeof(JabPointLightPdata), (MkHdr**)&pdata);
    if (proc == 0) {
        if (light->hdr.instance != 0U) {
            ((void (*)(MkHdr*))light->hdr.vtbl->destroy)(&light->hdr);
        }
        return 0;
    }

    zero_pdata_payload(sizeof(JabPointLightPdata), &pdata->hdr);
    pdata->light = light;
    pdata->light_instance = light->hdr.instance;
    pdata->tracked_object = object;
    pdata->tracked_object_instance = object->hdr.instance;
    pdata->bone = bone;
    mk_insert(&light->hdr, &proc->pdata_list);
    return light;
}

float p_jab_point_light_tracker(void) {
    JabPointLightPdata* pdata;
    MkObj* tracked_object;
    MkObj* light;
    Vec position;

    pdata = (JabPointLightPdata*)pdata_of_proc(aproc);
    RESOLVE_JAB_OBJECT(
        tracked_object, pdata->tracked_object, pdata->tracked_object_instance);
    if (tracked_object == 0) {
        return -1.0f;
    }

    RESOLVE_JAB_OBJECT(light, pdata->light, pdata->light_instance);
    if (light == 0) {
        return -1.0f;
    }

    get_bone_world_pos(tracked_object, pdata->bone, &position);
    light->pos.value.x = position.x;
    light->pos.value.y = position.y;
    light->pos.value.z = position.z;
    update_obj_pos(light);
    return 1.0f;
}

void jab_flash_screen(int color, float intensity, float duration) {
    FlashScreenPdataRef pdata;

    if (_create_mkproc_generic_tinystack(
            FLASH_SCREEN_PID, 0x1F, p_flash_screen, sizeof(FlashScreenPdata),
            &pdata.hdr) != 0 &&
        pdata.hdr != 0) {
        zero_pdata_payload(sizeof(FlashScreenPdata), pdata.hdr);
        pdata.flash->color = color;
        pdata.flash->intensity = intensity;
        pdata.flash->duration = duration;
    }
}

float p_flash_screen(void) {
    FlashScreenPdata* pdata;
    ScreenObj* flash;
    float elapsed;
    int visible;
    int flash_count;

    elapsed = 0.0f;
    pdata = (FlashScreenPdata*)pdata_of_proc(aproc);
    visible = 1;
    flash_count = 0;
    flash = load_named_2d_pfxobj(
        0, 0x2056, "WHITE_FADEBOX", 0, 0xD);
    if (flash != 0) {
        flash->x = -50;
        flash->y = -50;
        flash->priority = 0x13;
        flash->draw_flags.on = 1;
        flash->scale_x = 50.0f;
        flash->scale_y = 40.0f;
    }
    snd_req(0x1CA);

    while (flash_count < pdata->color) {
        if (visible != 0) {
            pfx_2d_obj_set_alpha(flash, 0xFF);
            elapsed += get_game_speed();
            if (elapsed >= pdata->duration) {
                elapsed = 0.0f;
                visible = 0;
                flash_count++;
                continue;
            }
        } else {
            pfx_2d_obj_set_alpha(flash, 0);
            elapsed += get_game_speed();
            if (elapsed >= pdata->intensity) {
                elapsed = 0.0f;
                visible = 1;
                if (flash_count % 2 != 0) {
                    snd_req(0x1C9);
                } else {
                    snd_req(0x1CA);
                }
                continue;
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    if (flash->instance != 0) {
        ((void (*)(ScreenObj*))flash->vtbl->destroy)(flash);
    }
    return -1.0f;
}

void jab_shake_dragon_king(float distance, float speed) {
    DragonKingShakePdata* pdata;
    MkObj* object;

    object = get_my_plyr_obj();
    if (object != 0 &&
        _create_mkproc_generic_nostack(
            DRAGON_KING_SHAKE_PID, 0x1F, p_dk_death_shake,
            sizeof(DragonKingShakePdata), (MkHdr**)&pdata) != 0 &&
        pdata != 0) {
        zero_pdata_payload(sizeof(DragonKingShakePdata), &pdata->hdr);
        pdata->object = object;
        pdata->object_instance = object->hdr.instance;
        pdata->base_y = object->pos.value.y;
        pdata->distance = distance;
        pdata->speed = speed;
        object->flags_08_bits.airborne = 1;
        object->flags_09_bits.launched = 0;
        object->flags_09_bits.bit6 = 0;
    }
}

void jab_stop_dragon_king_shake(void) {
    MkProc* proc;
    JabProcVtableRef vtbl;

    proc = find_mkproc_pid(DRAGON_KING_SHAKE_PID);
    if (proc != 0 && proc->instance != 0U) {
        vtbl.base = proc->vtbl;
        vtbl.jab->destroy(proc);
    }
}

float p_dk_death_shake(void) {
    static int moving_up = 1;
    DragonKingShakePdata* pdata;
    MkObj* object;

    pdata = (DragonKingShakePdata*)pdata_of_proc(aproc);
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    if (moving_up != 0) {
        object->pos.value.y += pdata->speed;
        if (object->pos.value.y - pdata->base_y > pdata->distance) {
            moving_up = 0;
        }
    } else {
        object->pos.value.y -= pdata->speed;
        if (object->pos.value.y - pdata->base_y < 0.0f) {
            moving_up = 1;
        }
    }
    update_mkobj(object != 0 ? as_mkhdr(&object->hdr) : 0);
    return 1.0f;
}

void jab_attach_wiff_to_sobj(
    MkObj* object, int sobj_id, const char* wiff_name,
    const char* texture_name, int section, int frame, float rate) {
    AniTextureControl* control;
    MkSobj* sobj;

    sobj = obj_create_sobjs_by_id(object, sobj_id);
    control = replace_sobj_texture_with_named_wiff(
        sobj, section, texture_name, wiff_name);
    if (control != 0) {
        set_ani_texture_framerate(control, rate);
        set_ani_texture_frame(control, frame);
    }
}

void jab_destroy_drink_obj_in_hand(void) {
    if (get_mode_of_play() == 8) {
        one_shot_script_func(g_game_info.plyr1.slot.pdata->cmo, 0x16, 1);
    }
}

void jab_attach_drink_obj_to_hand(
    MkObj* drink, const float* offset, const Vec* angles) {
    MkObj* player;
    JabBoneMatcherState* matcher;

    player = get_my_plyr_obj();
    drink->light_flags = player->light_flags;
    specskin_initialize_clump(drink->clump);
    matcher = start_bone_matcher(0.0f, player, 0x19, drink, 0);
    if (matcher != 0) {
        matcher->flags_08_bits.copy_bone_matrix = 1;
        YXZ_angles_to_MKMATRIX(angles, drink->bones[0]->parent_matrix);
        YXZ_angles_to_quat(angles, &drink->bones[0]->rotation);
        bone_matcher_parent_set_offset(matcher, offset);
    }
}

void jab_face_obj(MkObj* object, const Vec* direction) {
    JabFloatBits inverse;
    RwMatrix* matrix;
    float inverse_length;
    float half_x;
    float length_sq;
    float newton;

    if (object == 0) {
        return;
    }

    matrix = object->field_24;
    matrix->up.z = 0.0f;
    matrix->up.y = 0.0f;
    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->at.x = direction->x;
    matrix->at.y = direction->y;
    matrix->at.z = direction->z;

    length_sq = matrix->at.x * matrix->at.x +
                matrix->at.y * matrix->at.y +
                matrix->at.z * matrix->at.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse.f = length_sq;
        inverse.u = 0x5F375A00U - (inverse.u >> 1);
        half_x = inverse.f * (length_sq * inverse.f);
        newton = 3.0f - half_x;
        inverse_length =
            0.0625f * inverse.f * newton *
            -((newton * (half_x * newton)) - 12.0f);
    }

    matrix->at.x *= inverse_length;
    matrix->at.y *= inverse_length;
    matrix->at.z *= inverse_length;
    matrix->right.x =
        matrix->at.y * matrix->up.z - matrix->at.z * matrix->up.y;
    matrix->right.y =
        matrix->at.z * matrix->up.x - matrix->at.x * matrix->up.z;
    matrix->right.z =
        matrix->at.x * matrix->up.y - matrix->at.y * matrix->up.x;
    matrix->up.x =
        matrix->right.y * matrix->at.z - matrix->right.z * matrix->at.y;
    matrix->up.y =
        matrix->right.z * matrix->at.x - matrix->right.x * matrix->at.z;
    matrix->up.z =
        matrix->right.x * matrix->at.y - matrix->right.y * matrix->at.x;
}

/* Exact size and algorithm; the remaining delta is FPR allocation. */
void obj_scale_over_time(MkObj* object, const Vec* target, float ticks) {
    float ticks_left;
    float delta_z;
    float delta_y;
    float delta_x;
    float scaled_ticks;

    ticks_left = ticks;
    object->flags_08_bits.scale_active = 1;
    scaled_ticks = ticks * game_speed;
    delta_x = (target->x - object->scale.x) / scaled_ticks;
    delta_y = (target->y - object->scale.y) / scaled_ticks;
    delta_z = (target->z - object->scale.z) / scaled_ticks;

    while (ticks_left > 0.0f) {
        object->scale.x += delta_x;
        if (object->scale.x < 0.0f) {
            object->scale.x = 0.0f;
        }
        object->scale.y += delta_y;
        if (object->scale.y < 0.0f) {
            object->scale.y = 0.0f;
        }
        object->scale.z += delta_z;
        if (object->scale.z < 0.0f) {
            object->scale.z = 0.0f;
        }
        _mkproc_sleep_ticks = 1.0f;
        ticks_left -= game_speed;
        ((JabProcSleepVtable*)aproc->vtbl)->sleep();
    }

    object->scale.x = target->x;
    object->scale.y = target->y;
    object->scale.z = target->z;
}

/* Exact size; the remaining delta is register allocation in the two latches. */
void jab_release_jade_boomerang(JabObjectRef* proc_ref) {
    JadeBindPdata* pdata;
    MkObj* boomerang;
    MkProc* proc;
    JabProcVtableRef vtbl;

    proc = (MkProc*)proc_ref->object;
    if (proc != 0) {
        if ((unsigned int)proc->instance == proc_ref->instance) {
            /* The process reference is still live. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (JadeBindPdata*)pdata_of_proc(proc);
        RESOLVE_JAB_OBJECT(
            boomerang, pdata->child, pdata->child_instance);
        if (boomerang != 0) {
            boomerang->flags_08_bits.angular_velocity_enabled = 1;
            boomerang->flags_08_bits.rotation_enabled = 1;
            boomerang->ang_vel.x = 0.3f;
        }
        if (proc->instance != 0) {
            vtbl.base = proc->vtbl;
            vtbl.jab->destroy(proc);
        }
    }
    proc_ref->object = 0;
    proc_ref->instance = 0;
}

/* Soft ceiling: the remaining delta is NV-register coloring, one equivalent
 * character guard branch, and the pooled-string relocation label. */
void jab_start_jade_boomerang_throw(
    JabObjectRef* proc_ref, JabObjectRef* boomerang_ref,
    float unused_parameter) {
    JadeBindPdata* pdata;
    MkObj* player;
    MkObj* boomerang;
    MkProc* live_proc;
    MkProc* proc_candidate;
    MkProc* bind_proc;
    PlyrPdata* jade_data;
    PlyrPdata* player_data;

    player_data = get_my_plyr_pdata();
    if (player_data != 0) {
        if (player_data->character_id == JADE_CHARACTER_ID) {
            jade_data = player_data;
            RESOLVE_JAB_OBJECT(
                player, jade_data->tracked_obj,
                jade_data->tracked_obj_instance);
            if (player != 0) {
                RESOLVE_JAB_OBJECT(
                    boomerang, (MkObj*)boomerang_ref->object,
                    boomerang_ref->instance);
                if (boomerang == 0) {
                    boomerang = load_named_model_for_player(
                        "BRANG",
                        jade_data->plyr_num, 0xD000, 0);
                    if (boomerang != 0) {
                        insert_fgnd_mkobj(boomerang);
                        boomerang_ref->object = &boomerang->hdr;
                        boomerang_ref->instance = boomerang->hdr.instance;
                    }
                }
                if (boomerang != 0) {
                    /* These transform bits drive the Jade boomerang's visible,
                     * attached, and angular-update states respectively. */
                    boomerang->flags_08_bits.airborne = 1;
                    boomerang->flags_08_bits.angular_velocity_enabled = 1;
                    boomerang->flags_08_bits.rotation_enabled = 0;

                    proc_candidate = (MkProc*)proc_ref->object;
                    if (proc_candidate != 0) {
                        if ((unsigned int)proc_candidate->instance ==
                            proc_ref->instance) {
                            live_proc = proc_candidate;
                        } else {
                            live_proc = 0;
                        }
                    } else {
                        live_proc = 0;
                    }
                    bind_proc = live_proc;
                    if (live_proc == 0) {
                        bind_proc = _create_mkproc_generic_nostack(
                            JADE_BIND_PID, 0x1F, p_bind_obj_to_obj_bone,
                            sizeof(JadeBindPdata), (MkHdr**)&pdata);
                    }
                    if (live_proc != 0) {
                        pdata = (JadeBindPdata*)pdata_of_proc(live_proc);
                    }

                    if (bind_proc != 0) {
                        proc_ref->object = (MkHdr*)bind_proc;
                        proc_ref->instance = bind_proc->instance;
                        pdata->child = boomerang;
                        pdata->child_instance = boomerang->hdr.instance;
                        pdata->parent = player;
                        pdata->parent_instance = player->hdr.instance;
                        if (am_i_flipped() != 0) {
                            pdata->parent_bone = 0x1A;
                        } else {
                            pdata->parent_bone = 0x1B;
                        }
                    }
                }
            }
        }
    }
}

float p_bind_obj_to_obj_bone(void) {
    JadeBindPdata* pdata;
    MkObj* parent;
    MkObj* child;

    pdata = (JadeBindPdata*)pdata_of_proc(aproc);
    RESOLVE_JAB_OBJECT(
        parent, pdata->parent, pdata->parent_instance);
    RESOLVE_JAB_OBJECT(
        child, pdata->child, pdata->child_instance);
    if (parent == 0 || child == 0) {
        return -1.0f;
    }

    get_bone_world_pos(parent, pdata->parent_bone, &child->pos.value);
    update_mkobj(child != 0 ? as_mkhdr(&child->hdr) : 0);
    return 1.0f;
}

void jab_setup_kiss_emitter_obj(MkPfx* effect) {
    JabFloatBits inverse;
    RwMatrix* matrix;
    Vec mouth_position;
    Vec head_position;
    float inverse_length;
    float length_sq;
    float half_x;
    float newton;
    int mouth_bone;

    if (plyr_obj == 0 || effect == 0) {
        return;
    }

    matrix = (RwMatrix*)effect->bound_obj;
    mouth_bone = am_i_flipped() != 0 ? 0x19 : 0x18;
    get_bone_world_pos(plyr_obj, mouth_bone, &mouth_position);
    get_bone_world_pos(plyr_obj, 0xD, &head_position);

    matrix->at.x = mouth_position.x - head_position.x;
    matrix->at.y = mouth_position.y - head_position.y;
    matrix->at.z = mouth_position.z - head_position.z;
    length_sq = matrix->at.x * matrix->at.x +
                matrix->at.y * matrix->at.y +
                matrix->at.z * matrix->at.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse.f = length_sq;
        inverse.u = 0x5F375A00U - (inverse.u >> 1);
        half_x = inverse.f * (length_sq * inverse.f);
        newton = 3.0f - half_x;
        inverse_length =
            0.0625f * inverse.f * newton *
            -((newton * (half_x * newton)) - 12.0f);
    }
    matrix->at.x *= inverse_length;
    matrix->at.y *= inverse_length;
    matrix->at.z *= inverse_length;

    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->up.z = 0.0f;
    matrix->right.x =
        matrix->at.y * matrix->up.z - matrix->at.z * matrix->up.y;
    matrix->right.y =
        matrix->at.z * matrix->up.x - matrix->at.x * matrix->up.z;
    matrix->right.z =
        matrix->at.x * matrix->up.y - matrix->at.y * matrix->up.x;

    length_sq = matrix->right.x * matrix->right.x +
                matrix->right.y * matrix->right.y +
                matrix->right.z * matrix->right.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse.f = length_sq;
        inverse.u = 0x5F375A00U - (inverse.u >> 1);
        half_x = inverse.f * (length_sq * inverse.f);
        newton = 3.0f - half_x;
        inverse_length =
            0.0625f * inverse.f * newton *
            -((newton * (half_x * newton)) - 12.0f);
    }
    matrix->right.x *= inverse_length;
    matrix->right.y *= inverse_length;
    matrix->right.z *= inverse_length;

    matrix->up.x =
        matrix->right.y * matrix->at.z - matrix->right.z * matrix->at.y;
    matrix->up.y =
        matrix->right.z * matrix->at.x - matrix->right.x * matrix->at.z;
    matrix->up.z =
        matrix->right.x * matrix->at.y - matrix->right.y * matrix->at.x;
    matrix->pos.x = mouth_position.x;
    matrix->pos.y = mouth_position.y;
    matrix->pos.z = mouth_position.z;
}

void bulvan_function(int command) {
    PlyrPdata* opponent_data;
    MkPfx* effect;
    MkProc* process;
    MkObj* tracked_object;
    unsigned int effect_handle;
    int owner;
    int index;

    switch (command) {
    case 1:
        tracked_object = plyr_obj;
        create_pfx(
            0xA001, 0xA001,
            pfx_react_falling_attach_smoke_to_bones_proc, &effect,
            &jab_pfx_table[3], "C - Smoke Reaction");
        if (effect != 0) {
            set_pfx_texture(
                (PfxVm*)&effect->matrix, (void*)0x10005, (void*)0x2003C);
            pfx_bind_emitter_to_obj_bone(effect, tracked_object, 9);
            pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 8.0f;
            effect->field_90 = 0x1EF;
            effect->depth_bias = -50.0f;
            effect->field_298 = 100.0f;
            effect->field_29C = 80.0f;
            effect->field_2A4 = 1.0f;
            effect->tracked_object = tracked_object;
            effect->tracked_object_instance = tracked_object->hdr.instance;
        }
        break;

    case 0:
        create_pfx(
            0xA00C, 0xA015, pfx_kenshi_lift_smoke, &effect,
            &jab_pfx_table[2], "C - Kenshi Lift Smoke");
        if (effect != 0) {
            set_pfx_texture(
                (PfxVm*)&effect->matrix, (void*)0x10005, (void*)0x20038);
            pfx_texture_animate(
                (PfxVm*)&effect->matrix, 4.0f, 0x40, 0x10, 0x10, 0x10);
            pfx_bind_emitter_to_obj_bone(effect, plyr_obj, 0);
            effect->emitter_enabled = 0;
            pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 24.0f;
            effect->field_90 = 0xA8;
            effect->depth_bias = -50.0f;
            effect->field_298 = 50.0f;
            effect->field_29C = 7.0f;
            effect->field_2A0 = 0.0f;
            effect->effect_state = 0;
            effect->tracked_object = plyr_obj;
            effect->tracked_object_instance = plyr_obj->hdr.instance;
        }
        break;

    case 2:
        if (plyr_obj == 0) {
            break;
        }
        opponent_data = get_his_plyr_pdata();
        if (opponent_data == 0 || opponent_data->character_id != 0x13) {
            break;
        }
        if (_create_mkproc_generic_nostack(
                0xA026, 0x2E, p_kabal_dash_react_pfx, 0, 0) == 0) {
            break;
        }
        g_plyr_pdata = get_my_plyr_pdata();
        owner = pfx_plyr_bankowner(opponent_data->plyr_info);
        effect_handle = fx_by_owner("kabal_dash_react", owner);
        g_kabal_dash_react_pfx_handles[0] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 1);
        g_kabal_dash_react_pfx_handles[1] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 2);

        owner = pfx_plyr_bankowner(opponent_data->plyr_info);
        effect_handle = fx_by_owner("kabal_dash_react_shin", owner);
        g_kabal_dash_react_pfx_handles[2] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 4);
        g_kabal_dash_react_pfx_handles[3] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 5);

        owner = pfx_plyr_bankowner(opponent_data->plyr_info);
        effect_handle = fx_by_owner("kabal_dash_react_lt", owner);
        g_kabal_dash_react_pfx_handles[4] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 0x14);
        g_kabal_dash_react_pfx_handles[5] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 0xF);

        owner = pfx_plyr_bankowner(opponent_data->plyr_info);
        effect_handle = fx_by_owner("kabal_dash_react_rt", owner);
        g_kabal_dash_react_pfx_handles[6] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 0x15);
        g_kabal_dash_react_pfx_handles[7] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 0x11);

        owner = pfx_plyr_bankowner(opponent_data->plyr_info);
        effect_handle = fx_by_owner("kabal_dash_react_head", owner);
        g_kabal_dash_react_pfx_handles[8] =
            pfxhandle_spawn_at_bid_next(effect_handle, plyr_obj, 0x10);
        break;

    case 3:
        process = find_mkproc_pid(0xA026);
        if (process != 0 && process->instance != 0) {
            ((MkHdr*)process)->typed_vtbl->destroy((MkHdr*)process);
        }
        for (index = 0; index < 9; index++) {
            if (g_kabal_dash_react_pfx_handles[index] != 0) {
                fx_reset_emit(g_kabal_dash_react_pfx_handles[index]);
            }
            g_kabal_dash_react_pfx_handles[index] = 0;
        }
        g_plyr_pdata = 0;
        break;

    case 4:
        jab_kira_projectile_hand_explode();
        break;
    }
}

void jab_kira_projectile_hand_explode(void) {
    static const Vec ZERO_DIRECTION = {0.0f, 0.0f, 0.0f};
    JabFloatBits inverse;
    PlyrPdata* player_data;
    MkObj* opponent;
    MkObj* effect_object;
    RwMatrix* matrix;
    Vec direction;
    float length_sq;
    float inverse_length;
    float half_x;
    float newton;

    direction = ZERO_DIRECTION;
    if (plyr_obj == 0) {
        return;
    }
    opponent = get_his_plyr_obj();
    if (opponent == 0) {
        return;
    }
    player_data = get_my_plyr_pdata();
    if (player_data == 0 || player_data->character_id != 0x12) {
        return;
    }

    effect_object = get_mkobj_frame(0xA010, 0);
    if (effect_object == 0) {
        return;
    }
    effect_object->flags_08_bits.airborne = 1;
    matrix = effect_object->field_24;
    direction.x = opponent->pos.value.x - plyr_obj->pos.value.x;
    direction.z = opponent->pos.value.z - plyr_obj->pos.value.z;
    length_sq = direction.x * direction.x + direction.z * direction.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse.f = length_sq;
        inverse.u = 0x5F375A00U - (inverse.u >> 1);
        half_x = inverse.f * (length_sq * inverse.f);
        newton = 3.0f - half_x;
        inverse_length =
            0.0625f * inverse.f * newton *
            -((newton * (half_x * newton)) - 12.0f);
    }
    direction.x *= inverse_length;
    direction.z *= inverse_length;

    matrix->at.x = direction.x;
    matrix->at.y = direction.y;
    matrix->at.z = direction.z;
    matrix->up.z = 0.0f;
    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->right.x =
        matrix->at.y * matrix->up.z - matrix->at.z * matrix->up.y;
    matrix->right.y =
        matrix->at.z * matrix->up.x - matrix->at.x * matrix->up.z;
    matrix->right.z =
        matrix->at.x * matrix->up.y - matrix->at.y * matrix->up.x;

    length_sq = matrix->right.x * matrix->right.x +
                matrix->right.y * matrix->right.y +
                matrix->right.z * matrix->right.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse.f = length_sq;
        inverse.u = 0x5F375A00U - (inverse.u >> 1);
        half_x = inverse.f * (length_sq * inverse.f);
        newton = 3.0f - half_x;
        inverse_length =
            0.0625f * inverse.f * newton *
            -((newton * (half_x * newton)) - 12.0f);
    }
    matrix->right.x *= inverse_length;
    matrix->right.y *= inverse_length;
    matrix->right.z *= inverse_length;
    matrix->up.x =
        matrix->right.y * matrix->at.z - matrix->right.z * matrix->at.y;
    matrix->up.y =
        matrix->right.z * matrix->at.x - matrix->right.x * matrix->at.z;
    matrix->up.z =
        matrix->right.x * matrix->at.y - matrix->right.y * matrix->at.x;

    if (am_i_flipped() != 0) {
        get_bone_world_pos(plyr_obj, 0x18, &matrix->pos_vec);
    } else {
        get_bone_world_pos(plyr_obj, 0x19, &matrix->pos_vec);
    }
    pfxhandle_spawn_at_bid("hand_explode", effect_object, 0x40000000);
}

float p_kabal_dash_react_pfx(void) {
    int i;

    if (g_plyr_pdata == 0 ||
        (g_plyr_pdata->state != KABAL_DASH_REACTION_STATE &&
         g_plyr_pdata->previous_state != KABAL_DASH_REACTION_STATE)) {
        for (i = 0; i < 9; i++) {
            if (g_kabal_dash_react_pfx_handles[i] != 0) {
                fx_reset_emit(g_kabal_dash_react_pfx_handles[i]);
            }
            g_kabal_dash_react_pfx_handles[i] = 0;
        }
        g_plyr_pdata = 0;
        return -1.0f;
    }
    return 1.0f;
}

void sh_set_grinder_speed(float speed) {
    MkSobj* grinder;

    grinder = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    grinder->flags_08_bits.angular_velocity_enabled = 1;
    grinder->ang_vel.y = speed;

    grinder = obj_find_sobj_by_id(g_game_info.bgnd_obj, 2);
    grinder->flags_08_bits.angular_velocity_enabled = 1;
    grinder->ang_vel.y = -speed;
}

void sh_spawn_grinder_crush_pfx(void) {
    Vec position;
    int burst;

    get_bone_world_pos(plyr_obj, 0, &position);
    pfxhandle_bgnd_spawn_at_position(
        "sh_grinder_crush_chunks", position.x, position.y, position.z);

    for (burst = 0; burst < 5; burst++) {
        sh_spawn_grinder_crush_blood();
        _mkproc_sleep_ticks = 1.0f;
        ((JabProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

float pfx_sh_grinder_crush_chunks(void) {
    MkPfx* effect;
    PfxVm* vm;
    MkHdr* emitter_object;
    Vec* source_positions;
    Vec* destination_positions;
    Vec* source_velocities;
    Vec* destination_velocities;
    Vec* last_position;
    Vec* last_velocity;
    float* source_timers;
    float* destination_timers;
    float* source_scales;
    float* destination_scales;
    float* source_angles;
    float* destination_angles;
    float* last_timer;
    float* last_scale;
    float* last_angle;
    int* source_states;
    int* destination_states;
    int* last_state;
    unsigned char* source_colors;
    unsigned char* destination_colors;
    unsigned char* last_color;
    int field_stride;
    int vector_stride;
    int last_index;
    int index;
    float ground_height;
    float old_velocity_y;
    float delta_x;
    float delta_y;
    float delta_z;

    effect = apfx;
    emitter_object = pfx_get_emitter_obj(effect, 0);
    if (g_game_info.bgnd_obj == 0) {
        if (emitter_object->instance != 0) {
            emitter_object->typed_vtbl->destroy(emitter_object);
        }
        return -1.0f;
    }

    vm = (PfxVm*)&effect->matrix;
    source_velocities = (Vec*)pfx_get_field(vm, -1, 0x300);
    destination_velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
    source_timers = (float*)pfx_get_field(vm, -1, 0x301);
    destination_timers = (float*)pfx_get_field(vm, -2, 0x301);
    source_states = (int*)pfx_get_field(vm, -1, 0x307);
    destination_states = (int*)pfx_get_field(vm, -2, 0x307);
    destination_positions = (Vec*)pfx_get_field(vm, -2, 0x100);
    destination_angles = (float*)pfx_get_field(vm, -2, 0x103);
    source_positions = (Vec*)pfx_get_field(vm, -1, 0x100);
    source_scales = (float*)pfx_get_field(vm, -1, 0x102);
    destination_scales = (float*)pfx_get_field(vm, -2, 0x102);
    destination_colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);
    source_colors = (unsigned char*)pfx_get_field(vm, -1, 0x101);
    source_angles = (float*)pfx_get_field(vm, -1, 0x103);

    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    last_index = vm->particle_cursor - 1;
    last_color = source_colors + field_stride * last_index;
    last_position = (Vec*)((unsigned char*)source_positions +
                           field_stride * last_index);
    last_scale = (float*)((unsigned char*)source_scales +
                          field_stride * last_index);
    last_angle = (float*)((unsigned char*)source_angles +
                          field_stride * last_index);
    last_velocity = (Vec*)((unsigned char*)source_velocities +
                           vector_stride * last_index);
    last_timer = (float*)((unsigned char*)source_timers +
                          vector_stride * last_index);
    last_state = (int*)((unsigned char*)source_states +
                        vector_stride * last_index);

    index = 0;
    while (index < vm->particle_cursor) {
        if ((float)*source_states >= 500.0f) {
            index--;
            source_positions->x = last_position->x;
            source_positions->y = last_position->y;
            source_positions->z = last_position->z;
            source_velocities->x = last_velocity->x;
            source_velocities->y = last_velocity->y;
            source_velocities->z = last_velocity->z;
            source_colors[0] = last_color[0];
            source_colors[1] = last_color[1];
            source_colors[2] = last_color[2];
            source_colors[3] = last_color[3];
            *source_timers = *last_timer;
            *source_scales = *last_scale;
            *source_angles = *last_angle;
            *source_states = *last_state;

            last_position = (Vec*)((unsigned char*)last_position - field_stride);
            last_color -= field_stride;
            last_scale = (float*)((unsigned char*)last_scale - field_stride);
            last_angle = (float*)((unsigned char*)last_angle - field_stride);
            last_velocity =
                (Vec*)((unsigned char*)last_velocity - vector_stride);
            last_timer =
                (float*)((unsigned char*)last_timer - vector_stride);
            last_state = (int*)((unsigned char*)last_state - vector_stride);
            vm->particle_cursor--;
        } else {
            delta_x = source_velocities->x * game_speed;
            delta_y = source_velocities->y * game_speed;
            delta_z = source_velocities->z * game_speed;
            destination_positions->x = source_positions->x + delta_x;
            destination_positions->y = source_positions->y + delta_y;
            destination_positions->z = source_positions->z + delta_z;
            destination_velocities->x = source_velocities->x;
            destination_velocities->y = source_velocities->y;
            destination_velocities->z = source_velocities->z;
            destination_velocities->y -= 0.004f;

            ground_height = 0.1f + g_game_info.field_34;
            if (source_positions->y <= ground_height) {
                if (source_positions->z < 12.2f) {
                    if (*source_states < 2) {
                        source_positions->y = ground_height;
                        *destination_states = *source_states + 1;
                        destination_positions->y = ground_height;
                        old_velocity_y = destination_velocities->y;
                        destination_velocities->y *= -0.4f;
                        destination_velocities->x *= 0.4f;
                        destination_velocities->z *= 0.4f;
                        spawn_bld_splat(
                            "blsplat", effect->decal_owner, source_positions,
                            0.4f, old_velocity_y);
                    } else {
                        destination_velocities->x = 0.0f;
                        destination_velocities->y = 0.0f;
                        destination_velocities->z = 0.0f;
                    }
                } else {
                    *destination_states = 0x190;
                }
            }
            if (*source_states < 2) {
                *destination_timers = *source_timers + game_speed;
            } else {
                *destination_states = *source_states + 1;
            }
            destination_scales[0] = source_scales[0];
            destination_angles[0] = source_angles[0];
            destination_colors[0] = source_colors[0];
            destination_colors[1] = source_colors[1];
            destination_colors[2] = source_colors[2];
            destination_colors[3] = source_colors[3];

            source_velocities =
                (Vec*)((unsigned char*)source_velocities + vector_stride);
            destination_velocities =
                (Vec*)((unsigned char*)destination_velocities + vector_stride);
            source_positions =
                (Vec*)((unsigned char*)source_positions + field_stride);
            destination_positions =
                (Vec*)((unsigned char*)destination_positions + field_stride);
            source_timers =
                (float*)((unsigned char*)source_timers + vector_stride);
            destination_timers =
                (float*)((unsigned char*)destination_timers + vector_stride);
            source_states =
                (int*)((unsigned char*)source_states + vector_stride);
            destination_states =
                (int*)((unsigned char*)destination_states + vector_stride);
            source_scales =
                (float*)((unsigned char*)source_scales + field_stride);
            destination_scales =
                (float*)((unsigned char*)destination_scales + field_stride);
            source_angles =
                (float*)((unsigned char*)source_angles + field_stride);
            destination_angles =
                (float*)((unsigned char*)destination_angles + field_stride);
            source_colors += field_stride;
            destination_colors += field_stride;
        }
        index++;
    }
    return 1.0f;
}

void sh_start_grinder_crush_chunks(const Vec* position, int chunk_type) {
    MkPfx* effect;
    MkObj* emitter_object;

    create_pfx(
        0xA009, 0xA010, pfx_sh_grinder_crush_chunks, &effect,
        &jab_pfx_table[5], "C - Grinder Crush Chunks");
    if (effect != 0) {
        set_pfx_texture(
            (PfxVm*)&effect->matrix, (void*)0x2001E, (void*)0x013F0013);
        pfx_texture_animate(
            (PfxVm*)&effect->matrix, 4.0f, 0x100, 0x40, 0x55, 0xC);
        effect->emitter_enabled = 1;
        pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 5.0f;
        effect->field_90 = 0x12C;
        effect->depth_bias = -50.0f;
        effect->field_2B8 = chunk_type;
        emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
        emitter_object->pos.value.x = position->x;
        emitter_object->pos.value.y = position->y;
        emitter_object->pos.value.z = position->z;
        p_grinder_crush_chunks_pfx = effect;
    }
}

void sh_spawn_grinder_crush_blood(void) {
    JabFloatBits inverse;
    MkPfx* effect;
    PfxVm* vm;
    PfxEmitter* emitter;
    MkObj* emitter_object;
    Vec* positions;
    Vec* velocities;
    float* scales;
    float* angles;
    float* zero_fields;
    unsigned char* colors;
    int field_stride;
    int vector_stride;
    int particle_index;
    int index;
    float length_sq;
    float inverse_length;
    float half_x;
    float newton;
    float speed;

    effect = p_grinder_crush_blood_pfx;
    emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
    vm = (PfxVm*)&effect->matrix;
    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    emitter = pfx_get_emitter(vm, 0);
    if (emitter->lifetime <
        (float)(vm->particle_capacity - vm->particle_cursor + 1)) {
        velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
        positions = (Vec*)pfx_get_field(vm, -2, 0x100);
        zero_fields = (float*)pfx_get_field(vm, -2, 0x301);
        angles = (float*)pfx_get_field(vm, -2, 0x103);
        scales = (float*)pfx_get_field(vm, -2, 0x102);
        colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);

        particle_index = vm->particle_cursor;
        positions = (Vec*)((unsigned char*)positions +
                           field_stride * particle_index);
        scales = (float*)((unsigned char*)scales +
                          field_stride * particle_index);
        angles = (float*)((unsigned char*)angles +
                          field_stride * particle_index);
        colors += field_stride * particle_index;
        velocities = (Vec*)((unsigned char*)velocities +
                            vector_stride * particle_index);
        zero_fields = (float*)((unsigned char*)zero_fields +
                               vector_stride * particle_index);
        vm->particle_cursor += (int)pfx_get_emitter(vm, 0)->lifetime;

        index = 0;
        while ((float)index < pfx_get_emitter(vm, 0)->lifetime) {
            positions->x = emitter_object->pos.value.x + sfrand(0.5f);
            positions->y = emitter_object->pos.value.y + sfrand(0.5f);
            positions->z = emitter_object->pos.value.z + sfrand(0.3f);

            velocities->x = sfrand(1.0f);
            velocities->y = 0.7f + sfrand(0.7f);
            velocities->z = -1.0f - frand(0.5f);
            length_sq = velocities->x * velocities->x +
                        velocities->y * velocities->y +
                        velocities->z * velocities->z;
            inverse_length = 0.0f;
            if (length_sq > 0.0f) {
                inverse.f = length_sq;
                inverse.u = 0x5F375A00U - (inverse.u >> 1);
                half_x = inverse.f * (length_sq * inverse.f);
                newton = 3.0f - half_x;
                inverse_length =
                    0.0625f * inverse.f * newton *
                    -((newton * (half_x * newton)) - 12.0f);
            }
            velocities->x *= inverse_length;
            velocities->y *= inverse_length;
            velocities->z *= inverse_length;
            speed = 0.01f + frand(0.1f);
            velocities->x *= speed;
            velocities->y *= speed;
            velocities->z *= speed;

            *zero_fields = 0.0f;
            *scales = 0.5f + sfrand(0.05f);
            *angles = frand(6.2831855f);
            colors[0] = 0x7D;
            colors[1] = 0x7D;
            colors[2] = 0x7D;
            colors[3] = 0xFF;

            positions = (Vec*)((unsigned char*)positions + field_stride);
            scales = (float*)((unsigned char*)scales + field_stride);
            angles = (float*)((unsigned char*)angles + field_stride);
            colors += field_stride;
            velocities = (Vec*)((unsigned char*)velocities + vector_stride);
            zero_fields =
                (float*)((unsigned char*)zero_fields + vector_stride);
            index++;
        }
    }
}

float pfx_sh_grinder_crush_blood(void) {
    MkPfx* effect;
    PfxVm* vm;
    MkHdr* emitter_object;
    Vec* source_positions;
    Vec* destination_positions;
    Vec* source_velocities;
    Vec* destination_velocities;
    Vec* last_position;
    Vec* last_velocity;
    float* source_zero_fields;
    float* destination_zero_fields;
    float* source_scales;
    float* destination_scales;
    float* source_angles;
    float* destination_angles;
    float* last_zero_field;
    float* last_scale;
    float* last_angle;
    unsigned char* source_colors;
    unsigned char* destination_colors;
    unsigned char* last_color;
    int field_stride;
    int vector_stride;
    int last_index;
    int index;
    float delta_x;
    float delta_y;
    float delta_z;

    effect = apfx;
    emitter_object = pfx_get_emitter_obj(effect, 0);
    if (g_game_info.bgnd_obj == 0) {
        if (emitter_object->instance != 0) {
            emitter_object->typed_vtbl->destroy(emitter_object);
        }
        return -1.0f;
    }

    vm = (PfxVm*)&effect->matrix;
    source_velocities = (Vec*)pfx_get_field(vm, -1, 0x300);
    destination_velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
    source_zero_fields = (float*)pfx_get_field(vm, -1, 0x301);
    destination_zero_fields = (float*)pfx_get_field(vm, -2, 0x301);
    destination_positions = (Vec*)pfx_get_field(vm, -2, 0x100);
    destination_angles = (float*)pfx_get_field(vm, -2, 0x103);
    source_positions = (Vec*)pfx_get_field(vm, -1, 0x100);
    source_scales = (float*)pfx_get_field(vm, -1, 0x102);
    destination_scales = (float*)pfx_get_field(vm, -2, 0x102);
    destination_colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);
    source_colors = (unsigned char*)pfx_get_field(vm, -1, 0x101);
    source_angles = (float*)pfx_get_field(vm, -1, 0x103);

    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    last_index = vm->particle_cursor - 1;
    last_position = (Vec*)((unsigned char*)source_positions +
                           field_stride * last_index);
    last_color = source_colors + field_stride * last_index;
    last_scale = (float*)((unsigned char*)source_scales +
                          field_stride * last_index);
    last_angle = (float*)((unsigned char*)source_angles +
                          field_stride * last_index);
    last_velocity = (Vec*)((unsigned char*)source_velocities +
                           vector_stride * last_index);
    last_zero_field = (float*)((unsigned char*)source_zero_fields +
                               vector_stride * last_index);

    index = 0;
    while (index < vm->particle_cursor) {
        source_colors[3] =
            (unsigned char)(source_colors[3] - (int)(15.0f * game_speed));
        if (source_colors[3] < 0x19) {
            index--;
            source_positions->x = last_position->x;
            source_positions->y = last_position->y;
            source_positions->z = last_position->z;
            source_velocities->x = last_velocity->x;
            source_velocities->y = last_velocity->y;
            source_velocities->z = last_velocity->z;
            source_colors[0] = last_color[0];
            source_colors[1] = last_color[1];
            source_colors[2] = last_color[2];
            source_colors[3] = last_color[3];
            *source_zero_fields = *last_zero_field;
            *source_scales = *last_scale;
            *source_angles = *last_angle;

            last_position = (Vec*)((unsigned char*)last_position - field_stride);
            last_color -= field_stride;
            last_scale = (float*)((unsigned char*)last_scale - field_stride);
            last_angle = (float*)((unsigned char*)last_angle - field_stride);
            last_velocity =
                (Vec*)((unsigned char*)last_velocity - vector_stride);
            last_zero_field =
                (float*)((unsigned char*)last_zero_field - vector_stride);
            vm->particle_cursor--;
        } else {
            delta_x = source_velocities->x * game_speed;
            delta_y = source_velocities->y * game_speed;
            delta_z = source_velocities->z * game_speed;
            destination_positions->x = source_positions->x + delta_x;
            destination_positions->y = source_positions->y + delta_y;
            destination_positions->z = source_positions->z + delta_z;
            destination_velocities->x = source_velocities->x;
            destination_velocities->y = source_velocities->y;
            destination_velocities->z = source_velocities->z;
            destination_velocities->y -= 0.001f;
            *destination_zero_fields = *source_zero_fields + game_speed;
            *destination_scales = *source_scales;
            *destination_angles = *source_angles;
            destination_colors[0] = source_colors[0];
            destination_colors[1] = source_colors[1];
            destination_colors[2] = source_colors[2];
            destination_colors[3] = source_colors[3];

            source_positions =
                (Vec*)((unsigned char*)source_positions + field_stride);
            destination_positions =
                (Vec*)((unsigned char*)destination_positions + field_stride);
            source_velocities =
                (Vec*)((unsigned char*)source_velocities + vector_stride);
            destination_velocities =
                (Vec*)((unsigned char*)destination_velocities + vector_stride);
            source_zero_fields =
                (float*)((unsigned char*)source_zero_fields + vector_stride);
            destination_zero_fields = (float*)((unsigned char*)destination_zero_fields +
                                               vector_stride);
            source_scales =
                (float*)((unsigned char*)source_scales + field_stride);
            destination_scales =
                (float*)((unsigned char*)destination_scales + field_stride);
            source_angles =
                (float*)((unsigned char*)source_angles + field_stride);
            destination_angles =
                (float*)((unsigned char*)destination_angles + field_stride);
            source_colors += field_stride;
            destination_colors += field_stride;
        }
        index++;
    }
    return 1.0f;
}

void sh_start_grinder_crush_blood(const Vec* position) {
    MkPfx* effect;
    MkObj* emitter_object;

    create_pfx(
        0xA008, 0xA00E, pfx_sh_grinder_crush_blood, &effect,
        &jab_pfx_table[4], "C - Grinder Crush Blood");
    if (effect != 0) {
        set_pfx_texture(
            (PfxVm*)&effect->matrix, (void*)0x2001E, (void*)0x013F000E);
        pfx_texture_animate(
            (PfxVm*)&effect->matrix, 1.0f, 0x80, 0x2A, 0x40, 6);
        effect->emitter_enabled = 1;
        pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 15.0f;
        effect->field_90 = 0x12C;
        effect->depth_bias = -50.0f;
        emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
        emitter_object->pos.value.x = position->x;
        emitter_object->pos.value.y = position->y;
        emitter_object->pos.value.z = position->z;
        p_grinder_crush_blood_pfx = effect;
    }
}

void sh_start_grinder_chunk_spew(const Vec* position, int chunk_type) {
    MkPfx* effect;
    MkObj* emitter_object;

    create_pfx(
        0xA009, 0xA010, pfx_sh_grinder_meat_spew, &effect,
        &jab_pfx_table[7], "C - Grinder Chunk Spew");
    if (effect != 0) {
        set_pfx_texture(
            (PfxVm*)&effect->matrix, (void*)0x2001E, (void*)0x013F0013);
        pfx_texture_animate(
            (PfxVm*)&effect->matrix, 4.0f, 0x100, 0x40, 0x55, 0xC);
        effect->emitter_enabled = 1;
        pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 2.0f;
        effect->field_90 = 0x82;
        effect->depth_bias = -50.0f;
        effect->field_298 = 120.0f;
        effect->field_2B8 = chunk_type;
        emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
        emitter_object->pos.value.x = position->x;
        emitter_object->pos.value.y = position->y;
        emitter_object->pos.value.z = position->z;
    }
}

float pfx_sh_grinder_meat_spew(void) {
    JabFloatBits inverse;
    PfxVm* vm;
    PfxEmitter* emitter;
    MkObj* emitter_object;
    Vec* source_positions;
    Vec* destination_positions;
    Vec* source_velocities;
    Vec* destination_velocities;
    Vec* last_position;
    Vec* last_velocity;
    float* source_timers;
    float* destination_timers;
    float* source_scales;
    float* destination_scales;
    float* source_angles;
    float* destination_angles;
    float* last_timer;
    float* last_scale;
    float* last_angle;
    unsigned char* source_colors;
    unsigned char* destination_colors;
    unsigned char* last_color;
    int field_stride;
    int vector_stride;
    int initial_cursor;
    int last_index;
    int index;
    Vec splat_position;
    float length_sq;
    float inverse_length;
    float half_x;
    float newton;
    float speed;
    float delta_x;
    float delta_y;
    float delta_z;
    float result;

    emitter_object = (MkObj*)pfx_get_emitter_obj(apfx, 0);
    if (g_game_info.bgnd_obj == 0) {
        if (emitter_object->hdr.instance != 0) {
            emitter_object->hdr.typed_vtbl->destroy(&emitter_object->hdr);
        }
        return -1.0f;
    }

    vm = (PfxVm*)&apfx->matrix;
    initial_cursor = apfx->field_94;
    source_velocities = (Vec*)pfx_get_field(vm, -1, 0x300);
    destination_velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
    source_timers = (float*)pfx_get_field(vm, -1, 0x301);
    destination_timers = (float*)pfx_get_field(vm, -2, 0x301);
    destination_positions = (Vec*)pfx_get_field(vm, -2, 0x100);
    destination_angles = (float*)pfx_get_field(vm, -2, 0x103);
    source_positions = (Vec*)pfx_get_field(vm, -1, 0x100);
    source_scales = (float*)pfx_get_field(vm, -1, 0x102);
    destination_scales = (float*)pfx_get_field(vm, -2, 0x102);
    destination_colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);
    source_colors = (unsigned char*)pfx_get_field(vm, -1, 0x101);
    source_angles = (float*)pfx_get_field(vm, -1, 0x103);

    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    last_index = vm->particle_cursor - 1;
    last_scale = (float*)((unsigned char*)source_scales +
                          field_stride * last_index);
    last_position = (Vec*)((unsigned char*)source_positions +
                           field_stride * last_index);
    last_angle = (float*)((unsigned char*)source_angles +
                          field_stride * last_index);
    last_color = source_colors + field_stride * last_index;
    last_velocity = (Vec*)((unsigned char*)source_velocities +
                           vector_stride * last_index);
    last_timer = (float*)((unsigned char*)source_timers +
                          vector_stride * last_index);

    index = 0;
    while (index < vm->particle_cursor) {
        if (source_positions->y < g_game_info.field_34 - 1.0f) {
            index--;
            source_positions->x = last_position->x;
            source_positions->y = last_position->y;
            source_positions->z = last_position->z;
            source_velocities->x = last_velocity->x;
            source_velocities->y = last_velocity->y;
            source_velocities->z = last_velocity->z;
            source_colors[0] = last_color[0];
            source_colors[1] = last_color[1];
            source_colors[2] = last_color[2];
            source_colors[3] = last_color[3];
            *source_timers = *last_timer;
            *source_scales = *last_scale;
            *source_angles = *last_angle;

            last_position = (Vec*)((unsigned char*)last_position - field_stride);
            last_color -= field_stride;
            last_scale = (float*)((unsigned char*)last_scale - field_stride);
            last_angle = (float*)((unsigned char*)last_angle - field_stride);
            last_velocity =
                (Vec*)((unsigned char*)last_velocity - vector_stride);
            last_timer = (float*)((unsigned char*)last_timer - vector_stride);
            vm->particle_cursor--;
        } else {
            delta_x = source_velocities->x * game_speed;
            delta_y = source_velocities->y * game_speed;
            delta_z = source_velocities->z * game_speed;
            destination_positions->x = source_positions->x + delta_x;
            destination_positions->y = source_positions->y + delta_y;
            destination_positions->z = source_positions->z + delta_z;
            destination_velocities->x = source_velocities->x;
            destination_velocities->y = source_velocities->y;
            destination_velocities->z = source_velocities->z;

            if (destination_positions->z >= 21.65f) {
                destination_positions->z = 21.65f;
                destination_velocities->z *= -0.2f;
                destination_velocities->x *= 0.4f;
                if (randu0(100) < 15) {
                    splat_position.x = destination_positions->x;
                    splat_position.y = destination_positions->y;
                    splat_position.z = destination_positions->z;
                    spawn_bld_splat("sh_bloodsplat", 0, &splat_position);
                }
            }
            destination_velocities->y -= 0.006f;
            *destination_timers = *source_timers + game_speed;
            *destination_scales = *source_scales;
            *destination_angles = *source_angles;
            destination_colors[0] = source_colors[0];
            destination_colors[1] = source_colors[1];
            destination_colors[2] = source_colors[2];
            destination_colors[3] = source_colors[3];

            source_velocities =
                (Vec*)((unsigned char*)source_velocities + vector_stride);
            destination_velocities =
                (Vec*)((unsigned char*)destination_velocities + vector_stride);
            source_positions =
                (Vec*)((unsigned char*)source_positions + field_stride);
            destination_positions =
                (Vec*)((unsigned char*)destination_positions + field_stride);
            source_timers =
                (float*)((unsigned char*)source_timers + vector_stride);
            destination_timers =
                (float*)((unsigned char*)destination_timers + vector_stride);
            source_scales =
                (float*)((unsigned char*)source_scales + field_stride);
            destination_scales =
                (float*)((unsigned char*)destination_scales + field_stride);
            source_angles =
                (float*)((unsigned char*)source_angles + field_stride);
            destination_angles =
                (float*)((unsigned char*)destination_angles + field_stride);
            source_colors += field_stride;
            destination_colors += field_stride;
        }
        index++;
    }

    if (apfx->field_298 > 0.0f) {
        emitter = pfx_get_emitter(vm, 0);
        if (emitter->lifetime <
            (float)(vm->particle_capacity - initial_cursor + 1)) {
            vm->particle_cursor += (int)pfx_get_emitter(vm, 0)->lifetime;
            index = 0;
            while ((float)index < pfx_get_emitter(vm, 0)->lifetime) {
                destination_positions->x =
                    emitter_object->pos.value.x + sfrand(0.5f);
                destination_positions->y =
                    emitter_object->pos.value.y + sfrand(1.0f);
                destination_positions->z =
                    emitter_object->pos.value.z + sfrand(0.5f);

                destination_velocities->x = 0.0f;
                destination_velocities->y = 0.0f;
                destination_velocities->z = 0.0f;
                destination_velocities->y = 0.6f + sfrand(0.8f);
                destination_velocities->x = sfrand(0.4f);
                destination_velocities->z = 1.0f + frand(0.5f);
                length_sq =
                    destination_velocities->x * destination_velocities->x +
                    destination_velocities->y * destination_velocities->y +
                    destination_velocities->z * destination_velocities->z;
                inverse_length = 0.0f;
                if (length_sq > 0.0f) {
                    inverse.f = length_sq;
                    inverse.u = 0x5F375A00U - (inverse.u >> 1);
                    half_x = inverse.f * (length_sq * inverse.f);
                    newton = 3.0f - half_x;
                    inverse_length =
                        0.0625f * inverse.f * newton *
                        -((newton * (half_x * newton)) - 12.0f);
                }
                destination_velocities->x *= inverse_length;
                destination_velocities->y *= inverse_length;
                destination_velocities->z *= inverse_length;
                speed = 0.2f + frand(0.2f);
                destination_velocities->x *= speed;
                destination_velocities->y *= speed;
                destination_velocities->z *= speed;

                *destination_timers = 0.0f;
                *destination_scales = grinder_meat_size + sfrand(0.2f);
                *destination_angles = frand(6.2831855f);
                destination_colors[0] = 0x96;
                destination_colors[1] = 0x96;
                destination_colors[2] = 0x96;
                destination_colors[3] = 0xFF;

                destination_positions =
                    (Vec*)((unsigned char*)destination_positions + field_stride);
                destination_velocities =
                    (Vec*)((unsigned char*)destination_velocities + vector_stride);
                destination_timers =
                    (float*)((unsigned char*)destination_timers + vector_stride);
                destination_scales =
                    (float*)((unsigned char*)destination_scales + field_stride);
                destination_angles =
                    (float*)((unsigned char*)destination_angles + field_stride);
                destination_colors += field_stride;
                index++;
            }
        }
    }

    result = 1.0f;
    apfx->field_298 -= 1.0f;
    if (vm->particle_cursor == 0) {
        if (emitter_object->hdr.instance != 0) {
            emitter_object->hdr.typed_vtbl->destroy(&emitter_object->hdr);
        }
        result = -1.0f;
    }
    return result;
}

void sh_start_grinder_meat_spew(const Vec* position, int chunk_type) {
    MkPfx* effect;
    MkObj* emitter_object;

    create_pfx(
        0xA009, 0xA010, pfx_sh_grinder_meat_spew, &effect,
        &jab_pfx_table[6], "C - Grinder Meat Spew");
    if (effect != 0) {
        set_pfx_texture(
            (PfxVm*)&effect->matrix, (void*)0x2001E, (void*)0x013F0010);
        pfx_texture_animate(
            (PfxVm*)&effect->matrix, 4.0f, 0x80, 0x20, 0x20, 0x10);
        effect->emitter_enabled = 1;
        pfx_get_emitter((PfxVm*)&effect->matrix, 0)->lifetime = 2.0f;
        effect->field_90 = 0x82;
        effect->depth_bias = -50.0f;
        effect->field_298 = 120.0f;
        effect->field_2B8 = chunk_type;
        emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
        emitter_object->pos.value.x = position->x;
        emitter_object->pos.value.y = position->y;
        emitter_object->pos.value.z = position->z;
    }
}

/*
 * Near match: exact size and call/control-flow sequence; remaining differences
 * are particle-pointer register allocation and equivalent float scheduling.
 */
float pfx_react_falling_attach_smoke_to_bones_proc(void) {
    PfxVm* vm;
    PfxEmitter* emitter;
    MkObj* object;
    Vec* source_velocities;
    Vec* destination_velocities;
    Vec* source_positions;
    Vec* destination_positions;
    float* source_timers;
    float* destination_timers;
    float* source_scales;
    float* destination_scales;
    unsigned char* source_colors;
    unsigned char* destination_colors;
    Vec* last_velocity;
    Vec* last_position;
    float* last_timer;
    float* last_scale;
    unsigned char* last_color;
    int field_stride;
    int vector_stride;
    int initial_cursor;
    int last_index;
    int index;

    vm = (PfxVm*)&apfx->matrix;
    initial_cursor = apfx->field_94;
    source_velocities = (Vec*)pfx_get_field(vm, -1, 0x300);
    destination_velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
    source_positions = (Vec*)pfx_get_field(vm, -1, 0x100);
    destination_positions = (Vec*)pfx_get_field(vm, -2, 0x100);
    source_colors = (unsigned char*)pfx_get_field(vm, -1, 0x101);
    destination_colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);
    source_timers = (float*)pfx_get_field(vm, -1, 0x301);
    destination_timers = (float*)pfx_get_field(vm, -2, 0x301);
    source_scales = (float*)pfx_get_field(vm, -1, 0x102);
    destination_scales = (float*)pfx_get_field(vm, -2, 0x102);

    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    last_index = vm->particle_cursor - 1;
    last_position = (Vec*)((unsigned char*)source_positions +
                           field_stride * last_index);
    last_color = source_colors + field_stride * last_index;
    last_scale = (float*)((unsigned char*)source_scales +
                          field_stride * last_index);
    last_velocity = (Vec*)((unsigned char*)source_velocities +
                           vector_stride * last_index);
    last_timer = (float*)((unsigned char*)source_timers +
                          vector_stride * last_index);

    index = 0;
    while (index < vm->particle_cursor) {
        source_colors[3] -= (int)(apfx->field_2A4 * game_speed);
        if ((float)source_colors[3] <= 10.0f + apfx->field_2A4) {
            index--;
            source_positions->x = last_position->x;
            source_positions->y = last_position->y;
            source_positions->z = last_position->z;
            source_velocities->x = last_velocity->x;
            source_velocities->y = last_velocity->y;
            source_velocities->z = last_velocity->z;
            source_colors[0] = last_color[0];
            source_colors[1] = last_color[1];
            source_colors[2] = last_color[2];
            source_colors[3] = last_color[3];
            *source_scales = *last_scale;
            *source_timers = *last_timer;

            last_position = (Vec*)((unsigned char*)last_position -
                                   field_stride);
            last_color -= field_stride;
            last_scale = (float*)((unsigned char*)last_scale -
                                  field_stride);
            last_velocity = (Vec*)((unsigned char*)last_velocity -
                                   vector_stride);
            last_timer = (float*)((unsigned char*)last_timer -
                                  vector_stride);
            vm->particle_cursor--;
        } else {
            destination_colors[0] = source_colors[0];
            destination_colors[1] = source_colors[1];
            destination_colors[2] = source_colors[2];
            destination_colors[3] = source_colors[3];
            *destination_timers = *source_timers + game_speed;
            if (*source_scales < 0.5f) {
                *source_scales += 0.05f * game_speed;
                if (*source_scales > 0.5f) {
                    *source_scales = 0.5f;
                }
            }
            *destination_scales = *source_scales;

            destination_positions->x =
                source_positions->x + source_velocities->x * game_speed;
            destination_positions->y =
                source_positions->y + source_velocities->y * game_speed;
            destination_positions->z =
                source_positions->z + source_velocities->z * game_speed;
            destination_velocities->x = source_velocities->x;
            destination_velocities->y = source_velocities->y;
            destination_velocities->z = source_velocities->z;

            source_colors += field_stride;
            destination_colors += field_stride;
            source_scales = (float*)((unsigned char*)source_scales +
                                     field_stride);
            destination_scales =
                (float*)((unsigned char*)destination_scales + field_stride);
            source_positions = (Vec*)((unsigned char*)source_positions +
                                      field_stride);
            destination_positions =
                (Vec*)((unsigned char*)destination_positions + field_stride);
            source_velocities = (Vec*)((unsigned char*)source_velocities +
                                       vector_stride);
            destination_velocities =
                (Vec*)((unsigned char*)destination_velocities + vector_stride);
            source_timers = (float*)((unsigned char*)source_timers +
                                     vector_stride);
            destination_timers =
                (float*)((unsigned char*)destination_timers + vector_stride);
        }
        index++;
    }

    if (apfx->field_29C > 0.0f) {
        emitter = pfx_get_emitter(vm, 0);
        if (emitter->lifetime <
            (float)(vm->particle_capacity - initial_cursor)) {
            int spawn_count;

            spawn_count = (int)pfx_get_emitter(vm, 0)->lifetime;
            vm->particle_cursor += spawn_count;
            RESOLVE_JAB_OBJECT(
                object, apfx->tracked_object,
                apfx->tracked_object_instance);

            index = 0;
            while ((float)index < pfx_get_emitter(vm, 0)->lifetime) {
                Vec start;
                Vec end;
                Vec offset;
                float interpolation;
                int bone_index;

                bone_index = index % 12;
                get_bone_world_pos(
                    object,
                    skeleton_start_bone_id[apfx->effect_state][bone_index],
                    &start);
                get_bone_world_pos(
                    object,
                    skeleton_end_bone_id[apfx->effect_state][bone_index],
                    &end);
                v3_sub_v3(&offset, &end, &start);
                interpolation = frand(1.0f);
                offset.x *= interpolation;
                offset.y *= interpolation;
                offset.z *= interpolation;
                start.x += offset.x;
                start.y += offset.y;
                start.z += offset.z;

                destination_positions->x =
                    start.x + sfrand(skeleton_radius_table[bone_index]);
                destination_positions->y =
                    start.y + sfrand(skeleton_radius_table[bone_index]);
                destination_positions->z =
                    start.z + sfrand(skeleton_radius_table[bone_index]);
                destination_velocities->x = 0.0f;
                destination_velocities->y = 0.005f;
                destination_velocities->z = 0.0f;
                destination_colors[3] = 0xB4;
                if (apfx->field_29C < 40.0f) {
                    destination_colors[3] =
                        (destination_colors[3] * 2) / 3;
                }
                destination_colors[0] = 0x8E;
                destination_colors[1] = 0x87;
                destination_colors[2] = 0x82;
                *destination_scales = 0.7f;
                *destination_timers = 0.0f;

                destination_positions =
                    (Vec*)((unsigned char*)destination_positions + field_stride);
                destination_velocities =
                    (Vec*)((unsigned char*)destination_velocities + vector_stride);
                destination_colors += field_stride;
                destination_scales =
                    (float*)((unsigned char*)destination_scales + field_stride);
                destination_timers =
                    (float*)((unsigned char*)destination_timers + vector_stride);
                index++;
            }
            apfx->effect_state = apfx->effect_state != 1;
        } else {
            apfx->field_29C += 1.0f;
        }
    }

    if (apfx->field_29C == 53.333332f) {
        float lifetime;

        lifetime = pfx_get_emitter(vm, 0)->lifetime;
        pfx_get_emitter(vm, 0)->lifetime =
            (2.0f * lifetime) / 3.0f;
        apfx->field_2A4 *= 2.0f;
    }
    if (apfx->field_29C == 40.0f) {
        float lifetime;

        lifetime = pfx_get_emitter(vm, 0)->lifetime;
        pfx_get_emitter(vm, 0)->lifetime =
            (2.0f * lifetime) / 3.0f;
        apfx->field_2A4 *= 2.0f;
    }
    if (apfx->field_29C == 20.0f) {
        float lifetime;

        lifetime = pfx_get_emitter(vm, 0)->lifetime;
        pfx_get_emitter(vm, 0)->lifetime =
            (2.0f * lifetime) / 3.0f;
        apfx->field_2A4 *= 1.5f;
    }

    if (apfx->field_29C > 0.0f) {
        apfx->field_29C -= game_speed;
    } else {
        apfx->field_29C = 0.0f;
    }
    if (vm->particle_cursor == 0) {
        return -1.0f;
    }
    if (apfx->field_298 > 0.0f) {
        apfx->field_298 -= game_speed;
        return 1.0f;
    }
    return -1.0f;
}

float pfx_kenshi_lift_smoke(void) {
    JabFloatBits inverse;
    MkPfx* effect;
    PfxVm* vm;
    PfxEmitter* emitter;
    MkObj* emitter_object;
    MkObj* tracked_object;
    Vec* source_velocities;
    Vec* destination_velocities;
    Vec* source_positions;
    Vec* destination_positions;
    Vec* last_velocity;
    Vec* last_position;
    float* source_timers;
    float* destination_timers;
    float* source_scales;
    float* destination_scales;
    float* last_timer;
    float* last_scale;
    unsigned char* source_colors;
    unsigned char* destination_colors;
    unsigned char* last_color;
    int field_stride;
    int vector_stride;
    int last_index;
    int index;
    int bone_index;
    Vec start;
    Vec end;
    Vec offset;
    float along_bone;
    float length_sq;
    float inverse_length;
    float half_x;
    float newton;

    effect = apfx;
    vm = (PfxVm*)&effect->matrix;
    emitter_object = (MkObj*)pfx_get_emitter_obj(effect, 0);
    source_velocities = (Vec*)pfx_get_field(vm, -1, 0x300);
    destination_velocities = (Vec*)pfx_get_field(vm, -2, 0x300);
    destination_positions = (Vec*)pfx_get_field(vm, -2, 0x100);
    source_positions = (Vec*)pfx_get_field(vm, -1, 0x100);
    destination_colors = (unsigned char*)pfx_get_field(vm, -2, 0x101);
    source_colors = (unsigned char*)pfx_get_field(vm, -1, 0x101);
    source_timers = (float*)pfx_get_field(vm, -1, 0x301);
    destination_timers = (float*)pfx_get_field(vm, -2, 0x301);
    destination_scales = (float*)pfx_get_field(vm, -2, 0x102);
    source_scales = (float*)pfx_get_field(vm, -1, 0x102);

    field_stride = vm->transforms[0].particle_field_stride;
    vector_stride = vm->particle_vector_stride;
    last_index = vm->particle_cursor - 1;
    last_position = (Vec*)((unsigned char*)source_positions +
                           field_stride * last_index);
    last_color = source_colors + field_stride * last_index;
    last_scale = (float*)((unsigned char*)source_scales +
                          field_stride * last_index);
    last_velocity = (Vec*)((unsigned char*)source_velocities +
                           vector_stride * last_index);
    last_timer = (float*)((unsigned char*)source_timers +
                          vector_stride * last_index);

    index = 0;
    while (index < vm->particle_cursor) {
        if (effect->field_2A0 <= 0.0f) {
            destination_colors[3] =
                (unsigned char)(source_colors[3] - (int)(5.0f * game_speed));
        } else {
            destination_colors[3] = source_colors[3];
        }

        if (effect->field_2A0 <= 0.0f && destination_colors[3] <= 0xF) {
            index--;
            source_positions->x = last_position->x;
            source_positions->y = last_position->y;
            source_positions->z = last_position->z;
            source_velocities->x = last_velocity->x;
            source_velocities->y = last_velocity->y;
            source_velocities->z = last_velocity->z;
            source_colors[0] = last_color[0];
            source_colors[1] = last_color[1];
            source_colors[2] = last_color[2];
            source_colors[3] = last_color[3];
            *source_scales = *last_scale;
            *source_timers = *last_timer;

            last_position = (Vec*)((unsigned char*)last_position - field_stride);
            last_color -= field_stride;
            last_scale = (float*)((unsigned char*)last_scale - field_stride);
            last_velocity =
                (Vec*)((unsigned char*)last_velocity - vector_stride);
            last_timer = (float*)((unsigned char*)last_timer - vector_stride);
            vm->particle_cursor--;
        } else {
            destination_colors[0] = source_colors[0];
            destination_colors[1] = source_colors[1];
            destination_colors[2] = source_colors[2];
            *destination_timers = *source_timers + game_speed;
            *destination_scales = *source_scales;
            if (*destination_scales < 0.5f) {
                *destination_scales += 0.05f * game_speed;
                if (*destination_scales > 0.5f) {
                    *destination_scales = 0.5f;
                }
            }

            destination_velocities->x = 0.9f * source_velocities->x;
            destination_velocities->z = 0.9f * source_velocities->z;
            destination_positions->x =
                source_positions->x + destination_velocities->x;
            destination_positions->y =
                source_positions->y + destination_velocities->y;
            destination_positions->z =
                source_positions->z + destination_velocities->z;

            source_colors += field_stride;
            destination_colors += field_stride;
            source_timers =
                (float*)((unsigned char*)source_timers + vector_stride);
            destination_timers =
                (float*)((unsigned char*)destination_timers + vector_stride);
            source_scales =
                (float*)((unsigned char*)source_scales + field_stride);
            destination_scales =
                (float*)((unsigned char*)destination_scales + field_stride);
            source_velocities =
                (Vec*)((unsigned char*)source_velocities + vector_stride);
            destination_velocities =
                (Vec*)((unsigned char*)destination_velocities + vector_stride);
            source_positions =
                (Vec*)((unsigned char*)source_positions + field_stride);
            destination_positions =
                (Vec*)((unsigned char*)destination_positions + field_stride);
        }
        index++;
    }

    if (effect->field_29C > 0.0f) {
        emitter = pfx_get_emitter(vm, 0);
        if (emitter->lifetime <
            (float)(vm->particle_capacity - vm->particle_cursor + 1)) {
            vm->particle_cursor += (int)pfx_get_emitter(vm, 0)->lifetime;
            RESOLVE_JAB_OBJECT(
                tracked_object, effect->tracked_object,
                effect->tracked_object_instance);

            index = 0;
            while ((float)index < pfx_get_emitter(vm, 0)->lifetime) {
                bone_index = index % 12;
                get_bone_world_pos(
                    tracked_object,
                    skeleton_start_bone_id[effect->effect_state][bone_index],
                    &start);
                get_bone_world_pos(
                    tracked_object,
                    skeleton_end_bone_id[effect->effect_state][bone_index],
                    &end);
                v3_sub_v3(&offset, &end, &start);
                along_bone = frand(1.0f);
                offset.x *= along_bone;
                offset.y *= along_bone;
                offset.z *= along_bone;
                start.x += offset.x;
                start.y += offset.y;
                start.z += offset.z;

                bone_index = index % 10;
                destination_positions->x =
                    start.x + sfrand(skeleton_radius_table[bone_index]);
                destination_positions->y = frand(0.2f);
                destination_positions->z =
                    start.z + sfrand(skeleton_radius_table[bone_index]);

                destination_velocities->x =
                    destination_positions->x - emitter_object->pos.value.x;
                destination_velocities->y =
                    destination_positions->y - emitter_object->pos.value.y;
                destination_velocities->z =
                    destination_positions->z - emitter_object->pos.value.z;
                length_sq =
                    destination_velocities->x * destination_velocities->x +
                    destination_velocities->y * destination_velocities->y +
                    destination_velocities->z * destination_velocities->z;
                inverse_length = 0.0f;
                if (length_sq > 0.0f) {
                    inverse.f = length_sq;
                    inverse.u = 0x5F375A00U - (inverse.u >> 1);
                    half_x = inverse.f * (length_sq * inverse.f);
                    newton = 3.0f - half_x;
                    inverse_length =
                        0.0625f * inverse.f * newton *
                        -((newton * (half_x * newton)) - 12.0f);
                }
                destination_velocities->x *= inverse_length;
                destination_velocities->y *= inverse_length;
                destination_velocities->z *= inverse_length;
                destination_velocities->x *= 0.05f;
                destination_velocities->y *= 0.05f;
                destination_velocities->z *= 0.05f;
                destination_velocities->y = 0.005f;

                destination_colors[2] = 0x80;
                destination_colors[1] = 0x80;
                destination_colors[0] = 0x80;
                destination_colors[3] = 0xAF;
                *destination_scales = 0.2f;
                *destination_timers = 0.0f;

                destination_positions =
                    (Vec*)((unsigned char*)destination_positions + field_stride);
                destination_velocities =
                    (Vec*)((unsigned char*)destination_velocities + vector_stride);
                destination_colors += field_stride;
                destination_scales =
                    (float*)((unsigned char*)destination_scales + field_stride);
                destination_timers =
                    (float*)((unsigned char*)destination_timers + vector_stride);
                index++;
            }
            effect->effect_state = effect->effect_state != 1;
        }
    }

    if (effect->field_2A0 > 0.0f) {
        effect->field_2A0 -= game_speed;
    } else {
        effect->field_2A0 = 0.0f;
    }
    if (effect->field_29C > 0.0f) {
        effect->field_29C -= game_speed;
    } else {
        effect->field_29C = 0.0f;
    }
    if (vm->particle_cursor == 0) {
        return -1.0f;
    }
    if (effect->field_298 > 0.0f) {
        effect->field_298 -= game_speed;
        return 1.0f;
    }
    return -1.0f;
}

static void splatter_init(JabSplatterState* splatter) {
    splatter->flags_bits.bit4 = 1;
    splatter->flags_bits.bit3 = 1;
}
