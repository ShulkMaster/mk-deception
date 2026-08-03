#include "runtime/mk_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "game/specular.h"
#include "math/mk_math.h"
#include "game/game_info.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/image.h"
#include "runtime/light.h"
#include "runtime/utils.h"

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
extern PlyrPdata* g_plyr_pdata;
extern unsigned int g_kabal_dash_react_pfx_handles[9];
extern float game_speed;

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
    unsigned char flags_08;
} JabBoneMatcherState;

typedef struct JabSplatterState {
    char pad00[0x40];
    unsigned char flags;
} JabSplatterState;

typedef struct JabObjectRef {
    MkHdr* object;
    unsigned int instance;
} JabObjectRef;

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
    MkObj* parent, int parent_bone, MkObj* child, int child_bone,
    float blend);
void bone_matcher_parent_set_offset(
    JabBoneMatcherState* matcher, const float* offset);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void pfxhandle_bgnd_spawn_at_position(
    const char* effect_name, float x, float y, float z);
void sh_spawn_grinder_crush_blood(void);
int am_i_flipped(void);
void* load_named_model_for_player(
    char* name, int player, int object_type, int flags);
void fx_reset_emit(unsigned int effect);

/* Retail TU-local; its process body remains in the split assembly. */
float p_flash_screen(void);
float p_jab_point_light_tracker(void);
float p_dk_death_shake(void);
float p_bind_obj_to_obj_bone(void);

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

    light = load_light(definition, &point_light_list, 0);
    if (light != 0) {
        light->pos = *position;
        update_mkobj(light != 0 ? as_mkhdr(&light->hdr) : 0);
    }
    return light;
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
    tracked_object = pdata->tracked_object;
    if (tracked_object == 0 ||
        tracked_object->hdr.instance != pdata->tracked_object_instance) {
        return -1.0f;
    }

    light = pdata->light;
    if (light == 0 ||
        light->hdr.instance != pdata->light_instance) {
        return -1.0f;
    }

    get_bone_world_pos(tracked_object, pdata->bone, &position);
    light->pos = position;
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
        pdata->base_y = object->pos_y;
        pdata->distance = distance;
        pdata->speed = speed;
        object->flags_08 |= MKOBJ_FLAG_JADE_VISIBLE;
        object->flags_09 &= ~0xC0;
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
    MkBone* root_bone;

    player = get_my_plyr_obj();
    drink->light_flags = player->light_flags;
    specskin_initialize_clump(drink->clump);
    matcher = start_bone_matcher(player, 0x19, drink, 0, 0.0f);
    if (matcher != 0) {
        matcher->flags_08 |= 0x40;
        root_bone = drink->bones[0];
        YXZ_angles_to_MKMATRIX(angles, root_bone->parent_matrix);
        YXZ_angles_to_quat(angles, &root_bone->rotation);
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
    matrix->up.x = 0.0f;
    matrix->up.y = 0.0f;
    matrix->up.z = 0.0f;
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

void obj_scale_over_time(MkObj* object, const Vec* target, float ticks) {
    float delta_x;
    float delta_y;
    float delta_z;
    float scaled_ticks;
    float ticks_left;

    object->flags_08 |= MKOBJ_FLAG_SCALE_ACTIVE;
    scaled_ticks = ticks * game_speed;
    delta_x = (target->x - object->scale.x) / scaled_ticks;
    delta_y = (target->y - object->scale.y) / scaled_ticks;
    delta_z = (target->z - object->scale.z) / scaled_ticks;

    ticks_left = ticks;
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

    object->scale = *target;
}

void jab_release_jade_boomerang(JabObjectRef* proc_ref) {
    JadeBindPdata* pdata;
    MkObj* boomerang;
    MkProc* proc;
    JabProcVtableRef vtbl;

    proc = (MkProc*)proc_ref->object;
    if (proc != 0 && (unsigned int)proc->instance != proc_ref->instance) {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (JadeBindPdata*)pdata_of_proc(proc);
        boomerang = pdata->child;
        if (boomerang != 0 &&
            boomerang->hdr.instance != pdata->child_instance) {
            boomerang = 0;
        }
        if (boomerang != 0) {
            boomerang->flags_08 |= MKOBJ_FLAG_JADE_ATTACHED;
            boomerang->flags_08 |= MKSOBJ_FLAG_UPDATE_ANGULAR;
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

void jab_start_jade_boomerang_throw(
    JabObjectRef* proc_ref, JabObjectRef* boomerang_ref) {
    JadeBindPdata* pdata;
    MkObj* boomerang;
    MkObj* player;
    MkProc* bind_proc;
    MkProc* live_proc;
    PlyrPdata* player_data;

    player_data = get_my_plyr_pdata();
    if (player_data == 0 || player_data->character_id != JADE_CHARACTER_ID) {
        return;
    }

    player = player_data->tracked_obj;
    if (player != 0 &&
        player->hdr.instance != player_data->tracked_obj_instance) {
        player = 0;
    }
    if (player == 0) {
        return;
    }

    boomerang = (MkObj*)boomerang_ref->object;
    if (boomerang != 0 &&
        boomerang->hdr.instance != boomerang_ref->instance) {
        boomerang = 0;
    }
    if (boomerang == 0) {
        boomerang = load_named_model_for_player(
            "BRANG", player_data->plyr_num, 0xD000, 0);
        if (boomerang != 0) {
            insert_fgnd_mkobj(boomerang);
            boomerang_ref->object = &boomerang->hdr;
            boomerang_ref->instance = boomerang->hdr.instance;
        }
    }
    if (boomerang == 0) {
        return;
    }

    boomerang->flags_08 |= MKOBJ_FLAG_JADE_VISIBLE;
    boomerang->flags_08 |= MKOBJ_FLAG_JADE_ATTACHED;
    boomerang->flags_08 &= ~MKSOBJ_FLAG_UPDATE_ANGULAR;

    live_proc = (MkProc*)proc_ref->object;
    if (live_proc != 0 &&
        (unsigned int)live_proc->instance != proc_ref->instance) {
        live_proc = 0;
    }
    bind_proc = live_proc;
    if (bind_proc == 0) {
        bind_proc = _create_mkproc_generic_nostack(
            JADE_BIND_PID, 0x1F, p_bind_obj_to_obj_bone,
            sizeof(JadeBindPdata), (MkHdr**)&pdata);
    } else {
        pdata = (JadeBindPdata*)pdata_of_proc(bind_proc);
    }

    if (bind_proc != 0) {
        proc_ref->object = (MkHdr*)bind_proc;
        proc_ref->instance = bind_proc->instance;
        pdata->child = boomerang;
        pdata->child_instance = boomerang->hdr.instance;
        pdata->parent = player;
        pdata->parent_instance = player->hdr.instance;
        pdata->parent_bone = am_i_flipped() ? 0x1A : 0x1B;
    }
}

float p_kabal_dash_react_pfx(void) {
    int i;

    if (g_plyr_pdata != 0 &&
        (g_plyr_pdata->state == KABAL_DASH_REACTION_STATE ||
         g_plyr_pdata->previous_state == KABAL_DASH_REACTION_STATE)) {
        return 1.0f;
    }

    for (i = 0; i < 9; i++) {
        if (g_kabal_dash_react_pfx_handles[i] != 0) {
            fx_reset_emit(g_kabal_dash_react_pfx_handles[i]);
        }
        g_kabal_dash_react_pfx_handles[i] = 0;
    }
    g_plyr_pdata = 0;
    return 0.0f;
}

void sh_set_grinder_speed(float speed) {
    MkSobj* grinder;

    grinder = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    grinder->flags_08 |= MKSOBJ_FLAG_UPDATE_ANGULAR;
    grinder->ang_vel.y = speed;

    grinder = obj_find_sobj_by_id(g_game_info.bgnd_obj, 2);
    grinder->flags_08 |= MKSOBJ_FLAG_UPDATE_ANGULAR;
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

static void splatter_init(JabSplatterState* splatter) {
    splatter->flags |= 0x18;
}
