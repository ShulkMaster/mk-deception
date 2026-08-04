#include "runtime/mk_obj.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/cam.h"
#include "runtime/sound_tracker.h"
#include "runtime/utils.h"
#include "game/game_info.h"
#include "math/mk_math.h"
#include "platform/display.h"
#include "platform/fog.h"

extern MkObj* his_obj;
extern MkObj* plyr_obj;

typedef struct FatalityDistancePdata {
    MkHdr hdr;
    char pad08[0x28];
    float distance; /* +0x30 */
} FatalityDistancePdata;

typedef union FatalityDistancePdataRef {
    MkHdr* hdr;
    FatalityDistancePdata* distance;
} FatalityDistancePdataRef;

typedef float (*FatalityJumpSleepFn)(MkProcEntryFn entry, float ticks);

typedef struct FatalityProcVtable {
    void* reserved[6];
    void (*sleep)(void);
    void* reserved1C[2];
    FatalityJumpSleepFn jump_sleep;
} FatalityProcVtable;

typedef union FatalityProcVtableRef {
    MkVtableMkproc* base;
    FatalityProcVtable* fatality;
} FatalityProcVtableRef;

typedef struct RaidenLightningBoltPdata {
    MkHdr hdr;
    PlyrPdata* owner; /* +0x08 */
    MkObj* bolt;      /* +0x0C */
    unsigned int bolt_instance; /* +0x10 */
    int frame;        /* +0x14 */
    int bone_id;      /* +0x18 */
} RaidenLightningBoltPdata;

typedef struct FatalityState {
    void* context; /* +0x00 */
    MkObj* attacker_object; /* +0x04 */
    char pad08[0x0C];
    PlyrPdata* player; /* +0x14 */
    char pad18[4];
    MkObj* victim_object; /* +0x1C */
    MkProc* victim_proc; /* +0x20 */
    char pad24[4];
    struct FatalityAnimationView* animation; /* +0x28 */
    char pad2C[4];
    int mirror_camera; /* +0x30 */
    char pad34[0x9C];
} FatalityState;

typedef struct FatalityAnimationView {
    char pad00[0xB4];
    float movement_scale;
} FatalityAnimationView;

typedef struct FatalityObjectLatch {
    int active;
    char pad04[0x10];
    MkHdr* object;
    unsigned int object_instance;
} FatalityObjectLatch;

typedef struct FatalityScalePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float maximum;
    float step;
} FatalityScalePdata;

typedef struct FatalityObjectMatcherPdata {
    MkHdr hdr;
    MkObj* source;
    unsigned int source_instance;
    MkObj* destination;
    unsigned int destination_instance;
    float blend;
} FatalityObjectMatcherPdata;

typedef struct FatalityGroundBouncePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float ground_offset;
    int bounce_count;
    float restitution;
} FatalityGroundBouncePdata;

typedef struct FatalityBodySplatPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float current_scale;
    float target_scale;
    float step;
} FatalityBodySplatPdata;

typedef struct FatalityObjectScalarPdata {
    MkHdr hdr;
    unsigned char flags;
    char pad09[3];
    MkObj* object;
    unsigned int object_instance;
    Vec start;
    Vec target;
    Vec step;
} FatalityObjectScalarPdata;

typedef struct FatalityFaceObjectPdata {
    MkHdr hdr;
    int flags;
    MkObj* source;
    unsigned int source_instance;
    MkObj* target;
    unsigned int target_instance;
    int source_bone;
    int target_bone;
} FatalityFaceObjectPdata;

typedef struct FatalityLightningFlashPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    struct FatalityUvScrollControl* scroll;
    float scroll_step;
    int frame;
    float flash_range;
} FatalityLightningFlashPdata;

typedef struct FatalityUvScrollControl {
    char pad00[0x1C];
    float step;
} FatalityUvScrollControl;

typedef struct FatalityLightningScrollPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    char pad10[4];
    float u_step_a;
    float v_step_a;
    char pad1C[4];
    float u_step_b;
    float v_step_b;
    char pad28[0x10];
    Vec* scaling;
} FatalityLightningScrollPdata;

typedef struct FatalityWeaponSource {
    char pad00[0x58];
    PlyrPdata* owner;
    int weapon_id;
} FatalityWeaponSource;

typedef struct FatalityWeaponReflectionSet {
    char pad00[0x14];
    MkObj* primary;
    unsigned int primary_instance;
    char pad1C[0x10];
    MkObj* secondary;
    unsigned int secondary_instance;
} FatalityWeaponReflectionSet;

typedef struct FatalityRadiusCheck {
    int select_farthest;
    float center_x;
    char pad08[4];
    float center_z;
    float radius;
} FatalityRadiusCheck;

typedef struct FatalityIceChunkPdata {
    MkHdr hdr;
    MkObj* owner;
    unsigned int owner_instance;
    MkObj* model;
    unsigned int model_instance;
    void* chunks[9];
} FatalityIceChunkPdata;

typedef struct FatalityIceChunkMap {
    int subobject_id;
    int pebble_id;
} FatalityIceChunkMap;

typedef struct FatalityIceSubobject {
    MkHdr hdr;
    unsigned char flags08;
    unsigned char flags09;
    char pad0A[0x1E];
    float field28;
    char pad2C[4];
    Vec scale;
} FatalityIceSubobject;

typedef struct FatalityObjectGroupView {
    char pad00[0x2C];
    int group;
} FatalityObjectGroupView;

static FatalityState fatality_state;

float p_3d_distance_handler(void);
MslSoundHandle plyr_snd_req(int sound_offset);
RpMaterial* obj_find_material_by_id(MkObj* object, int id);
extern MslSoundHandle fatality_loop_sound;
extern float fog_distance;
extern float fog_color_real[4];
extern int fog_on;
extern int fog_type;
extern CameraObj* camera_obj;
extern CameraItem camera_item;
extern Vec raiden_scaling_data[];
extern FatalityIceChunkMap sz_hk_icechunk_map[9];
void snd_stop(MslSoundHandle handle);
void snd_req(int sound);
void freeze_player(void);
void start_3d_projectile_iceball(MkProcEntryFn entry);
float subzero_freeze_victim(void);
int is_weapon_style(PlyrFighterDefinition* fighter);
void advance_active_moveset(PlyrPdata* player);
void release_other_player(void);
float p_animate(void);
float p_face_obj(void);
float p_obj_grnd_bounce(void);
float p_obj_pos_matcher(void);
float p_obj_scalar_proc(void);
float p_bodyslam_bodysplat(void);
float p_raiden_lightning_scrolling(void);
float p_raiden_lightning_flash(void);
void set_anim_script(
    void* animation, void* script, int transition);
void set_root_and_obj_movement_weights(
    void* animation, float root_weight, float object_weight);
void create_mkproc_anim(
    int pid, MkProcEntryFn entry, AnimPdata** animation);
MkObj* load_weapon();
MkObj* load_weapon_reflection(int player, int weapon_id);
void obj_create_sobjs(MkObj* object);
void* load_named_model_from_slot(
    int slot, const char* name, int flags, int arg);
void* find_uv_scroll_control_for_obj(MkObj* object);
void update_mkobj_pdata(MkObj* object, MkHdr* pdata);
void camera_set_animation_mirror_plane(int mode);
void* RwCameraSetFarClipPlane(RwCamera* camera, float distance);
float sqrtf(float value);
float p_subzero_ice_chunk(void);
MkObj* load_named_model_for_player(
    const char* name, int player, int flags, int arg);
void* create_pebble_userdata(MkSobj* object, int pebble_id, int arg);

typedef int FatalityEffectHandle;
extern FatalityEffectHandle fx_by_owner();
extern FatalityEffectHandle fx_next_emitter();
extern void fx_resume_emit();
extern MkPfx* pfx_from_emitter(FatalityEffectHandle handle);
extern int emitter_id_from_handle(FatalityEffectHandle handle);
extern MkPfx* find_pfx_by_name();
extern void reset_effect();
extern void resume_effect();

#define FATALITY_SLEEP(ticks)                                                \
    do {                                                                     \
        _mkproc_sleep_ticks = (ticks);                                       \
        ((FatalityProcVtable*)aproc->vtbl)->sleep();                          \
    } while (0)

void subzero_start_ice_chunks(PlyrPdata* player) {
    FatalityIceChunkPdata* data;
    MkHdr* process;
    MkObj* model;
    FatalityIceSubobject* subobject;
    void* chunk;
    RpMaterialColor color;
    int index;

    data = 0;
    model = 0;
    process = (MkHdr*)_create_mkproc_generic_nostack(
        0x6037, 0x1F, p_subzero_ice_chunk,
        sizeof(FatalityIceChunkPdata), (MkHdr**)&data);
    if (process != 0) {
        zero_pdata_payload(sizeof(FatalityIceChunkPdata), &data->hdr);
        data->owner = fatality_state.attacker_object;
        data->owner_instance =
            fatality_state.attacker_object->hdr.instance;
        model = load_named_model_for_player(
            "ICESUICIDE", player->plyr_num, 0x6014, 1);
        if (model != 0) {
            obj_create_sobjs(model);
            data->model = model;
            data->model_instance = model->hdr.instance;
            insert_fgnd_mkobj(model);
            ((FatalityObjectGroupView*)model)->group =
                ((FatalityObjectGroupView*)plyr_obj)->group;
            mk_insert(&model->hdr, &plyr_obj->child_list);
            color.red = 0xFF;
            color.green = 0xFF;
            color.blue = 0xFF;
            color.alpha = 0xA0;
            obj_set_color_for_material_by_id(model, 0, (int*)&color);

            for (index = 0; index < 9; index++) {
                subobject = (FatalityIceSubobject*)
                    obj_find_sobj_by_id(
                        model, sz_hk_icechunk_map[index].subobject_id);
                if (subobject != 0) {
                    chunk = create_pebble_userdata(
                        (MkSobj*)subobject,
                        sz_hk_icechunk_map[index].pebble_id, 0);
                    if (chunk != 0) {
                        subobject->flags08 =
                            (subobject->flags08 | 0x48) & ~1;
                        subobject->flags09 |= 0x9A;
                        subobject->field28 = -49.0f;
                        sobj_set_priority((MkSobj*)subobject, 0x12);
                        subobject->scale.x = 0.0f;
                        subobject->scale.y = 0.0f;
                        subobject->scale.z = 0.0f;
                        data->chunks[index] = chunk;
                    }
                }
            }
            return;
        }
    }

    if (model != 0 && model->hdr.instance != 0) {
        ((int (*)(MkHdr*))model->hdr.vtbl->destroy)(&model->hdr);
    }
    if (process != 0 && process->instance != 0) {
        ((int (*)(MkHdr*))process->vtbl->destroy)(process);
    }
}

float sz_kill_myself(void) {
    FatalityDistancePdataRef data;
    FatalityProcVtableRef vtable;

    data.hdr = apdata;
    if (data.hdr == 0) {
        return -1.0f;
    }

    data.distance->distance = 13.0f;
    vtable.base = aproc->vtbl;
    vtable.fatality->jump_sleep(p_3d_distance_handler, 0.0f);
    return 0.0f;
}

void sindel_scream_react_sound_start(void) {
    if (plyr_pdata != 0 &&
        plyr_pdata->his_plyr_pdata->character_id == 8) {
        plyr_pdata->scream_sound_handle = plyr_snd_req(0x44);
    }
}

void play_final_fatality_music(void) {
    if (fatality_loop_sound != 0) {
        snd_stop(fatality_loop_sound);
    }
    if (g_game_info.field_200 == 3) {
        snd_req(0x1A9E);
    } else {
        snd_req(0x1A9C);
    }
}

float subzero_rx_freeze(void) {
    FatalityProcVtableRef vtable;

    freeze_player();
    snd_req(0x348);
    vtable.base = aproc->vtbl;
    vtable.fatality->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

void sindel_sonic_sounds(FatalityObjectLatch* sound, int finished) {
    MkHdr* object;

    object = sound->object;
    if (object != 0 &&
        object->instance != sound->object_instance) {
        object = 0;
    }
    if (object != 0 && finished == 0) {
        sound->active = 1;
    }
}

void mks_start_fatality_iceball(int mode) {
    if (mode == 0) {
        start_3d_projectile_iceball(sz_kill_myself);
    } else if (mode == 1) {
        start_3d_projectile_iceball(subzero_freeze_victim);
    }
}

MkHdr* get_fake_bone_matcher_proc(MkObjLatch* matcher) {
    MkHdr* proc;

    if (matcher == 0) {
        return 0;
    }
    proc = matcher->obj;
    if (proc != 0 &&
        proc->instance != matcher->obj_instance) {
        proc = 0;
    }
    return proc;
}

float subzero_his_tinkle_snd(void) {
    FatalityProcVtableRef vtable;

    _mkproc_sleep_ticks = 60.0f;
    vtable.base = aproc->vtbl;
    vtable.fatality->sleep();
    snd_req(0x353);
    vtable.fatality->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

float p_sz2_iceblock_scalar(void) {
    FatalityScalePdata* data;
    MkObj* object;

    data = (FatalityScalePdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object != 0 &&
        object->hdr.instance != data->object_instance) {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }
    object->scale.y += data->step;
    if (object->scale.y > data->maximum) {
        return -1.0f;
    }
    return 1.0f;
}

void advance_to_weapon_style(PlyrPdata* player) {
    if (player->plyr_info->player_index == 0x1B) {
        return;
    }
    while (is_weapon_style(player->fighter_definition) == 0) {
        advance_active_moveset(player);
    }
}

void fatality_release_other_player(void) {
    release_other_player();
    fatality_state.animation->movement_scale = 1.0f;
    his_obj->flags_09 &= ~(0x80 | 0x20 | 0x10 | 0x08 | 0x02);
    plyr_obj->flags_09 &= ~(0x80 | 0x20 | 0x10 | 0x08 | 0x02);
}

void weapon_bm_ignore(int weapon, int ignored) {
    PlyrMirrorObjLatch* latch;
    MkObj* object;

    if (weapon == 0) {
        latch = &plyr_pdata->mirror_slots->weapon[0].secondary;
    } else {
        latch = &plyr_pdata->mirror_slots->weapon[1].secondary;
    }
    object = latch->obj;
    if (object != 0 && object->hdr.instance != latch->instance) {
        object = 0;
    }
    if (object != 0) {
        object->flags_08 =
            (object->flags_08 & ~0x80) | ((ignored << 7) & 0x80);
    }
}

void kill_raiden_summon_lightning_bolt(
    RaidenLightningBoltPdata* lightning) {
    lightning->bone_id = -1;
}

FatalityState* get_fatality_state_ptr(void) {
    return &fatality_state;
}

void obj_unhide_material_by_id(MkObj* object, int id) {
    RpMaterial* material;

    material = obj_find_material_by_id(object, id);
    if (material != 0) {
        show_material(material);
    }
}

void obj_hide_material_by_id(MkObj* object, int id) {
    RpMaterial* material;

    material = obj_find_material_by_id(object, id);
    if (material != 0) {
        hide_material(material);
    }
}

void set_victim_v3_units_away(float x, float z) {
    Vec offset;
    float original_y;

    original_y = his_obj->pos.y;
    offset.x = x;
    offset.y = 0.0f;
    offset.z = z;
    v3_x_mat_add_v3(&his_obj->pos, &offset, plyr_obj->field_24,
                    &plyr_obj->pos);
    his_obj->pos.y = original_y;
}

void fix_axe_angle(const Vec* angles) {
    PlyrMirrorObjLatch* latch;
    MkObj* axe;

    latch = &fatality_state.player->mirror_slots->weapon[0].secondary;
    axe = latch->obj;
    axe = axe != 0
              ? (axe->hdr.instance == latch->instance ? axe : 0)
              : 0;

    if (axe != 0) {
        YXZ_angles_to_quat(angles, &axe->orientation_quat);
        axe->secondary_quat = axe->orientation_quat;
        axe->secondary_quat.y *= -1.0f;
        axe->secondary_quat.z *= -1.0f;
    }
}

void call_fatality_script_function(void) {
    ScriptSlot* script;

    script = fatality_state.player->cmo;
    cmdscript_setup_execution(script, active_cmdscript->unk28);
    cmdscript_execute(script);
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(player_sleep_forever, 0.0f);
}

void clone_my_weapon(FatalityWeaponSource* source) {
    MkObj* weapon;

    weapon = load_weapon(source->weapon_id);
    if (weapon != 0) {
        obj_create_sobjs(weapon);
        sobj_set_priority(obj_first_sobj(weapon), 6);
    }
}

void clone_weapon_to_secondary(
    int player, FatalityWeaponSource* source) {
    PlyrWeaponStyle* style;
    PlyrMirrorObjLatch* weapon_latch;
    PlyrMirrorObjLatch* reflection_latch;
    MkObj* weapon;
    MkObj* reflection;

    style = source->owner->weapon_styles[2];
    weapon_latch = &style->mirror_slots.weapon[1].primary;
    reflection_latch = &style->mirror_slots.weapon[1].mirror;
    weapon = load_weapon(source->weapon_id, source->owner);
    if (weapon == 0) {
        return;
    }
    weapon_latch->obj = weapon;
    weapon_latch->instance = weapon->hdr.instance;
    reflection = load_weapon_reflection(player, source->weapon_id);
    if (reflection != 0) {
        reflection_latch->obj = reflection;
        reflection_latch->instance = reflection->hdr.instance;
        obj_create_sobjs(reflection);
        sobj_set_priority(obj_first_sobj(reflection), 6);
    }
}

void fade_fatality_screen(void) {
    float far_clip;
    float step;

    background_color[0] = 0;
    background_color[1] = 0;
    background_color[2] = 0;
    background_color[3] = 0;
    g_game_info.sky = 0;
    fog_on = 1;
    fog_distance = 0.0f;
    fog_type = 1;
    fog_color_real[0] = 0.0f;
    fog_color_real[1] = 0.0f;
    fog_color_real[2] = 0.0f;
    fog_color_real[3] = 0.0f;

    far_clip = Camera->farPlane;
    step = (far_clip - 10.0f) / 30.0f;
    while (far_clip > 10.0f) {
        far_clip -= step;
        if (far_clip < 10.0f) {
            far_clip = 10.0f;
        }
        RwCameraSetFarClipPlane(Camera, far_clip);
        FATALITY_SLEEP(1.0f);
    }
}

void pfx_spawn_at_bid(
    int effect_handle, MkObj* object, int bone_id) {
    MkPfx* effect;

    effect = find_pfx_by_name();
    if (effect != 0) {
        pfx_bind_emitter_to_obj_bone(effect, object, bone_id);
        reset_effect(effect_handle);
        resume_effect(effect_handle);
    }
}

static void fatality_bind_next_emitter(
    MkObj* object, int bone_id, int bind_render) {
    FatalityEffectHandle emitter;
    MkPfx* effect;
    int emitter_id;

    emitter = fx_next_emitter();
    if (emitter == 0) {
        return;
    }
    fx_resume_emit();
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return;
    }
    emitter_id = emitter_id_from_handle(emitter);
    if (bind_render) {
        if ((unsigned int)(bone_id + 0xC0000000) == 0) {
            pfx_bind_render_to_obj(effect, object, 0);
        } else {
            pfx_bind_render_to_obj_bone(effect, object, bone_id);
        }
    } else if ((unsigned int)(bone_id + 0xC0000000) == 0) {
        pfx_bind_emitter_num_to_obj(effect, object, 0, emitter_id);
    } else {
        pfx_bind_emitter_num_to_obj_bone(
            effect, object, bone_id, emitter_id);
    }
}

void pfxhandle_spawn_at_bid_next_bind_render(
    MkObj* object, int bone_id) {
    fatality_bind_next_emitter(object, bone_id, 1);
}

void pfxhandle_spawn_at_bid_next(MkObj* object, int bone_id) {
    fatality_bind_next_emitter(object, bone_id, 0);
}

void pfxhandle_spawn_at_bid(MkObj* object, int bone_id) {
    if (fx_by_owner(
            1 << fatality_state.player->plyr_info->controller_slot,
            plyr_pdata, 1) != 0) {
        fatality_bind_next_emitter(object, bone_id, 0);
    }
}

void pfxhandle_bgnd_spawn_at_sobj_id(unsigned int sobj_id) {
    FatalityEffectHandle emitter;
    MkPfx* effect;
    MkSobj* sobj;

    if (fx_by_owner(4) == 0) {
        return;
    }
    emitter = fx_next_emitter();
    if (emitter == 0) {
        return;
    }
    fx_resume_emit();
    effect = pfx_from_emitter(emitter);
    if (effect == 0) {
        return;
    }
    sobj = (MkSobj*)obj_find_sobj_by_id(
        g_game_info.bgnd_obj, sobj_id);
    if (sobj != 0) {
        pfx_bind_emitter_num_to_sobj(
            effect, sobj, 0, emitter_id_from_handle(emitter));
    }
}

void fkbm_obj_face_obj(
    int flags, MkObj* source, int source_bone,
    MkObj* target, int target_bone) {
    FatalityFaceObjectPdata* data;

    if (_create_mkproc_generic_nostack(
            0x600D, 0x1E, p_face_obj,
            sizeof(FatalityFaceObjectPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityFaceObjectPdata), &data->hdr);
        data->source = source;
        data->source_instance = source->hdr.instance;
        data->target = target;
        data->target_instance = target->hdr.instance;
        data->source_bone = source_bone;
        data->target_bone = target_bone;
        data->flags = flags;
    }
}

void obj_match_obj_pos(
    MkObj* source, MkObj* destination, int snap, float blend) {
    FatalityObjectMatcherPdata* data;

    if (_create_mkproc_generic_nostack(
            0x6009, 0x1F, p_obj_pos_matcher,
            sizeof(FatalityObjectMatcherPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityObjectMatcherPdata), &data->hdr);
        data->source = source;
        data->source_instance = source->hdr.instance;
        data->destination = destination;
        data->destination_instance = destination->hdr.instance;
        if (snap) {
            destination->pos = source->pos;
        }
        destination->flags_08 |= 0x40;
        data->blend = blend;
    }
}

float p_obj_pos_matcher(void) {
    FatalityObjectMatcherPdata* data;
    MkObj* source;
    MkObj* destination;
    float blend;

    data = (FatalityObjectMatcherPdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    source = data->source;
    if (source == 0 ||
        source->hdr.instance != data->source_instance) {
        return 0.0f;
    }
    destination = data->destination;
    if (destination == 0 ||
        destination->hdr.instance != data->destination_instance) {
        return 0.0f;
    }
    blend = data->blend;
    if (blend == 1.0f) {
        destination->pos = source->pos;
    } else {
        destination->pos.x +=
            (source->pos.x - destination->pos.x) * blend;
        destination->pos.y +=
            (source->pos.y - destination->pos.y) * blend;
        destination->pos.z +=
            (source->pos.z - destination->pos.z) * blend;
    }
    return 1.0f;
}

void obj_grnd_bounce(
    MkObj* object, const Vec* velocity, int bounces,
    float gravity, float ground_offset, float restitution) {
    FatalityGroundBouncePdata* data;

    if (_create_mkproc_generic_nostack(
            0x600B, 0x1F, p_obj_grnd_bounce,
            sizeof(FatalityGroundBouncePdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityGroundBouncePdata), &data->hdr);
        data->object = object;
        data->object_instance = object->hdr.instance;
        if (velocity != 0) {
            object->pos_vel = *velocity;
        }
        object->flags_08 |= 0x20;
        object->gravity = gravity;
        if (gravity != 0.0f) {
            object->flags_08 |= 1;
        }
        data->ground_offset = ground_offset;
        data->bounce_count = bounces;
        data->restitution = restitution;
        update_mkobj(object);
    }
}

float p_obj_grnd_bounce(void) {
    FatalityGroundBouncePdata* data;
    MkObj* object;
    float ground;

    data = (FatalityGroundBouncePdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    object = data->object;
    if (object == 0 ||
        object->hdr.instance != data->object_instance) {
        return 0.0f;
    }
    ground = g_game_info.field_34 + data->ground_offset;
    if (object->pos_vel.y != 0.0f &&
        object->pos.y <= ground) {
        if (data->bounce_count != 0) {
            if (data->bounce_count > 2) {
                snd_req(randu0(2) ? 0xD9A : 0xD95);
            } else {
                snd_req(0xD94);
            }
            object->pos_vel.y =
                (float)data->bounce_count *
                (-object->pos_vel.y * data->restitution);
            data->bounce_count--;
        } else {
            object->flags_08 &= ~(0x20 | 0x04 | 0x01);
            object->flags_09 |= 0xC0;
            object->pos.y = ground;
        }
    }
    return 1.0f;
}

static int fatality_scalar_axis(
    float* value, float* start, float* target,
    float* step, unsigned char flags) {
    float next;
    float swap;

    if (*value == *target) {
        return 0;
    }
    next = (flags & 0x40) != 0
               ? *value * *step : *value + *step;
    if ((*step > 0.0f && next > *target) ||
        (*step < 0.0f && next < *target)) {
        next = *target;
        if ((flags & 0x30) == 0x30) {
            *step = -*step;
            swap = *target;
            *target = *start;
            *start = swap;
        }
    }
    *value = next;
    return 1;
}

float p_obj_scalar_proc(void) {
    FatalityObjectScalarPdata* data;
    MkObj* object;
    int active;
    Vec swap;

    data = (FatalityObjectScalarPdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    object = data->object;
    if (object == 0 ||
        object->hdr.instance != data->object_instance) {
        return 0.0f;
    }
    active = 0;
    active += fatality_scalar_axis(
        &object->scale.x, &data->start.x,
        &data->target.x, &data->step.x, data->flags);
    active += fatality_scalar_axis(
        &object->scale.y, &data->start.y,
        &data->target.y, &data->step.y, data->flags);
    active += fatality_scalar_axis(
        &object->scale.z, &data->start.z,
        &data->target.z, &data->step.z, data->flags);
    if (active != 0 || (data->flags & 0x80) != 0 ||
        (data->flags & 0x20) == 0) {
        return 1.0f;
    }
    data->step.x = -data->step.x;
    data->step.y = -data->step.y;
    data->step.z = -data->step.z;
    swap = data->target;
    data->target = data->start;
    data->start = swap;
    return 1.0f;
}

void start_obj_scalar_proc(
    MkObj* object, const Vec* start,
    const Vec* target, const Vec* step) {
    FatalityObjectScalarPdata* data;

    if (_create_mkproc_generic_nostack(
            0x600A, 0x1F, p_obj_scalar_proc,
            sizeof(FatalityObjectScalarPdata),
            (MkHdr**)&data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityObjectScalarPdata), &data->hdr);
        data->object = object;
        data->object_instance = object->hdr.instance;
        data->start = *start;
        data->target = *target;
        data->step = *step;
        object->flags_08 |= 2;
        object->scale = *start;
        update_mkobj(object);
    }
}

float p_bodyslam_bodysplat(void) {
    FatalityBodySplatPdata* data;
    MkObj* object;
    float next;

    data = (FatalityBodySplatPdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    object = data->object;
    if (object == 0 ||
        object->hdr.instance != data->object_instance) {
        return 0.0f;
    }
    data->current_scale += data->step;
    next = data->current_scale;
    if ((data->step >= 0.0f && next >= data->target_scale) ||
        (data->step < 0.0f && next <= data->target_scale)) {
        return 0.0f;
    }
    object->scale.x = next;
    object->scale.z = next;
    return 1.0f;
}

void start_bodyslam_bodysplat(
    float x, float z, float scale,
    float target_scale, float step) {
    FatalityBodySplatPdata* data;
    MkObj* object;
    int slot;

    if (_create_mkproc_generic_nostack(
            0x601D, 0x1F, p_bodyslam_bodysplat,
            sizeof(FatalityBodySplatPdata),
            (MkHdr**)&data) == 0) {
        return;
    }
    slot = fatality_state.player->plyr_num == 0
               ? 0x3000B : 0x4000B;
    object = (MkObj*)load_named_model_from_slot(
        slot, "BODYSPLAT", 0x6008, 0);
    if (object != 0) {
        data->object = object;
        data->object_instance = object->hdr.instance;
        insert_fgnd_mkobj(object);
        object->pos.x = x;
        object->pos.y = g_game_info.field_34 + 0.001f;
        object->pos.z = z;
        object->flags_08 |= 2;
        object->scale.x = 1.0f;
        object->scale.y = 1.0f;
        object->scale.z = 1.0f;
        data->current_scale = scale;
        data->target_scale = target_scale;
        data->step = step;
    }
}

float subzero_freeze_victim(void) {
    FatalityDistancePdata* data;

    data = (FatalityDistancePdata*)apdata;
    if (data == 0) {
        return 0.0f;
    }
    xfer_proc(fatality_state.victim_proc, subzero_rx_freeze);
    data->distance = 13.0f;
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(p_3d_distance_handler, 0.0f);
    return 0.0f;
}

void weapon_reflection_show_hide(
    PlyrPdata* player, int secondary, int hidden) {
    FatalityWeaponReflectionSet* reflections;
    MkObj* object;
    unsigned int instance;

    reflections = (FatalityWeaponReflectionSet*)
        player->fighter_definition;
    if (secondary == 0) {
        object = reflections->primary;
        instance = reflections->primary_instance;
        if (object != 0 && object->hdr.instance == instance) {
            object->hide_flags =
                (object->hide_flags & ~0x20) |
                ((hidden << 5) & 0x20);
            return;
        }
    }
    object = reflections->secondary;
    instance = reflections->secondary_instance;
    if (object != 0 && object->hdr.instance == instance) {
        object->hide_flags =
            (object->hide_flags & ~0x20) |
            ((hidden << 5) & 0x20);
    }
}

void fat_goro_fold_arms(
    PlyrPdata* player, MkObj* object,
    int transition, float speed) {
    AnimPdata* animation;

    if (player->character_id != 0x1E) {
        return;
    }
    animation = 0;
    create_mkproc_anim(
        0x5002, p_animate, &animation);
    if (animation != 0) {
        animation->obj = object;
        animation->obj_instance = object->hdr.instance;
        set_anim_script(
            animation, player->goro_fold_animation, transition);
        animation->step = speed;
        set_root_and_obj_movement_weights(
            animation, 0.0f, 1.0f);
    }
}

float p_fatality_cam(void) {
    CamVec3 angle;
    CamVec3 position;

    angle.x = 0.0f;
    angle.y = camera_obj->ang_y - 1.5707964f;
    angle.z = 0.0f;
    camera_set_animation_parent_angle(&angle, 0);
    position.x = fatality_state.attacker_object->pos.x;
    position.y = camera_obj->pos_y;
    position.z = fatality_state.attacker_object->pos.z;
    camera_set_animation_parent_position(&position);
    if (fatality_state.mirror_camera != 0) {
        camera_set_animation_mirror_plane(2);
    }
    camera_run_animation(0);
    ((FatalityProcVtable*)aproc->vtbl)
        ->jump_sleep(player_sleep_forever, 0.0f);
    return 0.0f;
}

float p_raiden_lightning_flash(void) {
    FatalityLightningFlashPdata* data;
    MkObj* object;
    float range;

    data = (FatalityLightningFlashPdata*)apdata;
    if (data == 0) {
        return -1.0f;
    }
    object = data->object;
    if (object == 0 ||
        object->hdr.instance != data->object_instance) {
        return -1.0f;
    }
    data->frame++;
    range = data->flash_range;
    if ((data->frame & 1) != 0) {
        range += 5.0f;
        object->hide_flags |= 0x20;
    } else {
        object->hide_flags &= ~0x20;
        if (data->scroll != 0) {
            data->scroll->step =
                randu0(3) == 0
                    ? -data->scroll_step : data->scroll_step;
        }
    }
    return frand(range);
}

void start_raiden_lightning_scroll(
    MkObj* object, int flash, int scaling_index,
    float u_step, float scroll_step) {
    FatalityLightningScrollPdata* scroll_data;
    FatalityLightningFlashPdata* flash_data;
    FatalityUvScrollControl* control;

    scroll_data = 0;
    if (_create_mkproc_generic_nostack(
            0x602C, 0x1F, p_raiden_lightning_scrolling,
            sizeof(FatalityLightningScrollPdata),
            (MkHdr**)&scroll_data) == 0) {
        return;
    }
    zero_pdata_payload(
        sizeof(FatalityLightningScrollPdata), &scroll_data->hdr);
    scroll_data->object = object;
    scroll_data->object_instance = object->hdr.instance;
    scroll_data->u_step_a = u_step;
    scroll_data->v_step_a = 1.0f;
    scroll_data->u_step_b = u_step;
    scroll_data->v_step_b = 1.0f;
    scroll_data->scaling = &raiden_scaling_data[scaling_index];
    control = (FatalityUvScrollControl*)
        find_uv_scroll_control_for_obj(object);
    if (control != 0) {
        control->step = scroll_step;
    }
    if (!flash) {
        return;
    }
    flash_data = 0;
    if (_create_mkproc_generic_nostack(
            0x602C, 0x1F, p_raiden_lightning_flash,
            sizeof(FatalityLightningFlashPdata),
            (MkHdr**)&flash_data) != 0) {
        zero_pdata_payload(
            sizeof(FatalityLightningFlashPdata),
            &flash_data->hdr);
        flash_data->object = object;
        flash_data->object_instance = object->hdr.instance;
        flash_data->scroll = control;
        flash_data->scroll_step = scroll_step;
        flash_data->flash_range =
            scaling_index == 2 ? 20.0f : 5.0f;
    }
}

void fat_bgnd_char_setup_radius_check(
    const FatalityRadiusCheck* check) {
    Vec attacker_delta;
    Vec victim_delta;
    const Vec* selected;
    CameraObj* camera;
    float attacker_distance_sq;
    float victim_distance_sq;
    float selected_distance_sq;
    float radius_sq;
    float correction;
    float x;
    float z;

    attacker_delta.x =
        fatality_state.attacker_object->pos.x - check->center_x;
    attacker_delta.y = 0.0f;
    attacker_delta.z =
        fatality_state.attacker_object->pos.z - check->center_z;
    victim_delta.x =
        fatality_state.victim_object->pos.x - check->center_x;
    victim_delta.y = 0.0f;
    victim_delta.z =
        fatality_state.victim_object->pos.z - check->center_z;
    attacker_distance_sq =
        attacker_delta.x * attacker_delta.x +
        attacker_delta.z * attacker_delta.z;
    victim_distance_sq =
        victim_delta.x * victim_delta.x +
        victim_delta.z * victim_delta.z;
    if (!check->select_farthest) {
        return;
    }
    if (check->radius < 0.0f) {
        selected = attacker_distance_sq < victim_distance_sq
                       ? &attacker_delta : &victim_delta;
        selected_distance_sq =
            attacker_distance_sq < victim_distance_sq
                ? attacker_distance_sq : victim_distance_sq;
    } else {
        selected = attacker_distance_sq > victim_distance_sq
                       ? &attacker_delta : &victim_delta;
        selected_distance_sq =
            attacker_distance_sq > victim_distance_sq
                ? attacker_distance_sq : victim_distance_sq;
    }
    radius_sq = check->radius * check->radius;
    if ((check->radius < 0.0f &&
         selected_distance_sq >= radius_sq) ||
        (check->radius >= 0.0f &&
         selected_distance_sq <= radius_sq)) {
        return;
    }
    if (fatality_state.player->character_id == 0x12 &&
        g_game_info.field_200 == 1 &&
        selected == &victim_delta) {
        return;
    }
    correction =
        0.65f * (check->radius / sqrtf(selected_distance_sq)) -
        1.0f;
    x = selected->x * correction;
    z = selected->z * correction;
    fatality_state.attacker_object->pos.x += x;
    fatality_state.attacker_object->pos.z += z;
    fatality_state.victim_object->pos.x += x;
    fatality_state.victim_object->pos.z += z;
    camera = camera_item.node;
    if (camera != 0 &&
        camera->instance != camera_item.instance) {
        camera = 0;
    }
    if (camera != 0) {
        camera->pos_x += x;
        camera->pos_z += z;
    }
}
