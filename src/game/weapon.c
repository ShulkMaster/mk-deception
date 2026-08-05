#include "game/game_info.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/asset.h"
#include "game/specular.h"
#include "math/mk_math.h"

typedef struct WeaponTrailMap {
    int trail_bone_index;
    int weapon_bone_index;
    int enabled;
    Vec offset;
} WeaponTrailMap;

typedef struct WeaponImpaleData {
    int bone_index;
    Vec position;
    char pad10[4];
    Vec rotation;
    Vec scale;
    Vec gusher_direction;
} WeaponImpaleData; /* 0x38 */

typedef struct WeaponDefinition {
    const char* model_name;       /* +0x00 */
    const int* bone_tags;         /* +0x04 */
    char pad08[0x28];
    const char* secondary_model_name; /* +0x30 */
    const int* trail_bone_tags; /* +0x34 */
    int trail_map_count; /* +0x38 */
    WeaponTrailMap* trail_maps; /* +0x3C */
    int* trail_chain_roots; /* +0x40 */
    const char* reflection_model_name; /* +0x44 */
    char pad48[0x24];
    WeaponImpaleData* impale_data; /* +0x6C */
} WeaponDefinition;

typedef struct ImpaleSecondaryObject {
    MkHdr hdr;
    char pad08[0x40];
    float strength; /* +0x48 */
} ImpaleSecondaryObject;

typedef struct WeaponBoneMatcherFlags08 {
    unsigned char inactive : 1;
    unsigned char copy_bone_matrix : 1;
    unsigned char copy_clone_matrix : 1;
    unsigned char preserve_bone_matrix : 1;
    unsigned char copy_parent_angles : 1;
    unsigned char flip_parent_angle_y : 1;
    unsigned char release_parent_weight : 1;
    unsigned char blend_child_transform : 1;
} WeaponBoneMatcherFlags08;

typedef struct WeaponBoneMatcherFlags09 {
    unsigned char use_unmirrored_parent : 1;
    unsigned char copy_child_flip : 1;
    unsigned char snap_child_transform : 1;
    unsigned char pad : 5;
} WeaponBoneMatcherFlags09;

typedef struct WeaponBoneMatcherState {
    MkHdr hdr;
    union {
        unsigned char flags_08;
        WeaponBoneMatcherFlags08 flags_08_bits;
    };
    union {
        unsigned char flags_09;
        WeaponBoneMatcherFlags09 flags_09_bits;
    };
    char pad0A[2];
    float child_weight;
    MkObj* parent_obj;
    unsigned int parent_instance;
    int parent_bone;
    Vec parent_offset;
    union {
        struct {
            MkObj* child_obj;
            unsigned int child_instance;
        };
        PlyrMirrorObjLatch child_latch;
    }; /* +0x28 */
    char pad30[0x0C];
    Vec child_offset; /* +0x3C */
    char pad48[0x98];
    Vec parent_translation; /* +0xE0 */
    char padEC[0x10];
    Vec mirrored_parent_translation; /* +0xFC */
    char pad108[8];
} WeaponBoneMatcherState; /* 0x110 */

typedef struct GusherStep {
    int blood_type;
    float velocity_scale;
    float interval;
} GusherStep;

typedef struct WeaponCollisionDef {
    float radius;
    Vec offset;
} WeaponCollisionDef;

typedef struct WeaponCollisionInfo {
    char pad00[0x4C];
    WeaponCollisionDef collision;
} WeaponCollisionInfo;

#define WEAPON_COLLISION_INFO(object) \
    ((WeaponCollisionInfo*)(object)->field_5C)

#define RESOLVE_WEAPON_LATCH(result, latch)                               \
    do {                                                                  \
        MkObj* raw_object_;                                               \
        raw_object_ = (latch)->obj;                                       \
        if (raw_object_ != 0) {                                          \
            if (raw_object_->hdr.instance == (latch)->instance) {        \
                (result) = raw_object_;                                   \
            } else {                                                      \
                (result) = 0;                                            \
            }                                                             \
        } else {                                                          \
            (result) = 0;                                                \
        }                                                                 \
    } while (0)

extern MkPtr* weapon_trail_mkobj_list;
extern MkObj* plyr_obj;
extern WeaponDefinition goro_gauntlets_weapon_desc_lr;
extern WeaponDefinition goro_gauntlets_weapon_desc_ll;

int plyr_obj_item_grab(PlyrPdata* player, PlyrMirrorObjLatch* item_latch,
                       PlyrMirrorObjLatch* secondary_latch, MkObj* item,
                       int bone_index, const Vec* position,
                       const Vec* rotation, const Vec* scale,
                       int insert_at_head);
MkObj* plyr_obj_item_release(PlyrPdata* player,
                             PlyrMirrorObjLatch* item_latch,
                             PlyrMirrorObjLatch* secondary_latch);
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots);
void obj_unhide_material_by_id(MkObj* object, int id);
void obj_hide_material_by_id(MkObj* object, int id);
void mkobj_zero_bone_rots(MkObj* object);
int get_player_number(MkObj* object);
int build_bones_tbl(MkObj* object, const int* tags);
void SetupShadowPlayerPipeline(RpClump* clump);
void pull_bone_hierarchy_mkobj(MkObj* object);
void obj_create_sobjs(MkObj* object);
void obj_force_culling_off(MkObj* object);
void start_cloth_bones(MkObj* object);
void start_weapon_trail(MkObj* weapon, MkObj* trail_model);
RpMaterial* obj_find_material_by_id(MkObj* object, int material_id);
RpMaterial* sobj_find_material_by_id(MkSobj* sobj, int material_id);
void sobj_use_material_color(MkSobj* sobj);
void obj_set_material_fade(MkObj* object, unsigned int material_id, int alpha);
void material_set_zbias(RpMaterial* material, float bias);
void RwFrameUpdateObjects(RwFrame* frame);
void* memcpy(void* destination, const void* source, unsigned int size);
MkProc* fade_material(float delta, MkObj* object, unsigned int sobj_id,
                      unsigned int material_id, int frames);
void advance_my_moveset(void);
void update_bone_hierarchy(MkHdr* object);
void* start_gusher(GusherStep* steps, int owner, MkObj* object, int bone,
                   const Vec* position, const Vec* direction);
extern GusherStep heart_beat[];
WeaponBoneMatcherState* start_bone_matcher(
    float blend_ticks, MkObj* parent, int parent_bone, MkObj* child,
    int child_bone);
void mkobj_bones_dest_mat_no_update(MkObj* object);
MkObj* load_weapon(
    WeaponDefinition* definition, MkObj* player_object);
MkObj* load_weapon_reflection(
    WeaponDefinition* definition, MkObj* player_object);

static Vec trail_p_to_c_uv = {1.0f, 0.0f, 0.0f};

static inline MkObj* load_goro_weapon_inline(
    WeaponDefinition* definition, MkObj* player_object) {
    PlyrPdata* player;
    MkObj* weapon;
    MkObj* trail_model;
    RpMaterial* material;
    unsigned int bone_index;
    int player_number;

    trail_model = 0;
    player_number = get_player_number(player_object);
    weapon = (MkObj*)load_named_model_for_player(
        definition->model_name, player_number, 0x1008, 1);
    if (weapon == 0) {
        return 0;
    }
    if (definition->secondary_model_name != 0) {
        trail_model = (MkObj*)load_named_model_for_player(
            definition->secondary_model_name, player_number, 0x5004, 1);
        if (trail_model == 0) {
            return 0;
        }
    }

    player = 0;
    if (player_object->oid == 0x1001) {
        player = (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    } else if (player_object->oid == 0x1002) {
        player = (PlyrPdata*)g_game_info.plyr1.slot.fighter;
    }
    if (player != 0 && player->character_id == 0xA &&
        player->plyr_info->flags_14_bits.alternate_costume) {
        obj_create_sobjs(weapon);
        material = obj_find_material_by_id(weapon, 1);
        if (material != 0) {
            material_set_zbias(material, -0.07f);
        }
    }

    if (definition->bone_tags != 0) {
        build_bones_tbl(weapon, definition->bone_tags);
        for (bone_index = 0; bone_index < weapon->bone_count; bone_index++) {
            MkBone* bone = weapon->bones[bone_index];

            if (bone != 0 && (bone->flags_54 & 0x20) == 0) {
                bone->flags_54 |= 0x10;
            }
        }
        specskin_initialize_clump(weapon->clump);
        specskin_force_clipping_clump(weapon->clump, 1);
    }
    obj_force_culling_off(weapon);
    start_cloth_bones(weapon);
    weapon->light_flags = 0x100C;
    weapon->hide_flag_bits.hidden = 1;
    weapon->hide_flag_bits.weapon_effect = 1;
    weapon->field_5C = definition;
    insert_fgnd_mkobj(weapon);
    start_weapon_trail(weapon, trail_model);
    return weapon;
}

static inline MkObj* load_goro_reflection_inline(
    WeaponDefinition* definition, MkObj* player_object) {
    MkObj* reflection;

    if (definition->reflection_model_name == 0) {
        return 0;
    }
    reflection = (MkObj*)load_named_model_for_player(
        definition->reflection_model_name,
        get_player_number(player_object), 0x5013, 0);
    if (reflection == 0) {
        return 0;
    }
    if (definition->bone_tags == 0) {
        if (reflection->hdr.instance != 0) {
            reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
        }
        return 0;
    }

    SetupShadowPlayerPipeline(reflection->clump);
    if (build_bones_tbl(reflection, definition->bone_tags) == 0) {
        if (reflection->hdr.instance != 0) {
            reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
        }
        return 0;
    }
    pull_bone_hierarchy_mkobj(reflection);
    reflection->light_flags = 4;
    reflection->hide_flag_bits.hidden = 1;
    insert_fgnd_mkobj(reflection);
    obj_create_sobjs(reflection);
    sobj_set_priority(obj_first_sobj(reflection), 6);
    if (g_game_info.field_08 != 0 &&
        (g_game_info.section->flags70 & 8) == 0) {
        hide_obj(reflection);
    }
    return reflection;
}

/*
 * Soft ceiling: 93.27% -- both repeated inline load paths and exact size agree;
 * remaining differences are NV allocation and equivalent branch placement.
 */
void mks_start_goro_xtra_weapons(void) {
    PlyrWeaponStyle* style;
    MkObj* weapon;
    MkObj* reflection;

    style = plyr_pdata->weapon_styles[2];

    weapon = load_goro_weapon_inline(
        &goro_gauntlets_weapon_desc_lr, plyr_obj);
    if (weapon != 0) {
        style->mirror_slots.weapon[2].primary.obj = weapon;
        style->mirror_slots.weapon[2].primary.instance = weapon->hdr.instance;
        mk_insert(&weapon->hdr, &style->object_list);

        reflection = load_goro_reflection_inline(
            &goro_gauntlets_weapon_desc_lr, plyr_obj);
        if (reflection != 0) {
            style->mirror_slots.weapon[2].mirror.obj = reflection;
            style->mirror_slots.weapon[2].mirror.instance =
                reflection->hdr.instance;
            mk_insert(&reflection->hdr, &style->object_list);
            obj_create_sobjs(reflection);
            sobj_set_priority(obj_first_sobj(reflection), 6);
        }
    }

    weapon = load_goro_weapon_inline(
        &goro_gauntlets_weapon_desc_ll, plyr_obj);
    if (weapon != 0) {
        style->mirror_slots.weapon[3].primary.obj = weapon;
        style->mirror_slots.weapon[3].primary.instance = weapon->hdr.instance;
        mk_insert(&weapon->hdr, &style->object_list);

        reflection = load_goro_reflection_inline(
            &goro_gauntlets_weapon_desc_ll, plyr_obj);
        if (reflection != 0) {
            style->mirror_slots.weapon[3].mirror.obj = reflection;
            style->mirror_slots.weapon[3].mirror.instance =
                reflection->hdr.instance;
            mk_insert(&reflection->hdr, &style->object_list);
            obj_create_sobjs(reflection);
            sobj_set_priority(obj_first_sobj(reflection), 6);
        }
    }
}

void reload_fan(PlyrPdata* player) {
}

void get_weapon_collision_def(MkObj* object, WeaponCollisionDef* collision) {
    collision->radius =
        WEAPON_COLLISION_INFO(object)->collision.radius;
    collision->offset.x =
        WEAPON_COLLISION_INFO(object)->collision.offset.x;
    collision->offset.y =
        WEAPON_COLLISION_INFO(object)->collision.offset.y;
    collision->offset.z =
        WEAPON_COLLISION_INFO(object)->collision.offset.z;
}

void plyr_aux_weapon_release(PlyrPdata* player) {
    plyr_obj_item_release(player, &player->aux_weapon_latch, 0);
}

void plyr_aux_weapon_grab(PlyrPdata* player, MkObj* item) {
    MkObjItemAttachData* attach;

    attach = item->item_attach_data;
    plyr_obj_item_grab(player, &player->aux_weapon_latch, 0, item,
                       attach->bone_index, &attach->position,
                       &attach->rotation, &attach->scale, 0);
}

void unimpale_victim(PlyrPdata* victim) {
    PlyrPdata* attacker;
    PlyrMirrorSlots* slots;
    MkObjItemAttachData* attach;
    MkObj* item;

    item = plyr_obj_item_release(
        victim, &victim->impaled_item_a,
        &victim->impaled_item_a_secondary);
    if (item != 0) {
        item->hide_flag_bits.hidden = 1;
        mkobj_zero_bone_rots(item);
        attacker = victim->his_plyr_pdata;
        slots = attacker->mirror_slots;
        if (slots != 0) {
            attach = item->item_attach_data;
            plyr_obj_item_grab(
                attacker, &slots->weapon[0].primary,
                &slots->weapon[0].secondary, item,
                attach->bone_index, &attach->position,
                &attach->rotation, &attach->scale, 1);
        }
    }

    item = plyr_obj_item_release(
        victim, &victim->impaled_item_b,
        &victim->impaled_item_b_secondary);
    if (item != 0) {
        item->hide_flag_bits.hidden = 1;
        mkobj_zero_bone_rots(item);
        attacker = victim->his_plyr_pdata;
        slots = attacker->mirror_slots;
        if (slots != 0) {
            attach = item->item_attach_data;
            if (attach != 0) {
                plyr_obj_item_grab(
                    attacker, &slots->weapon[1].primary,
                    &slots->weapon[1].secondary, item,
                    attach->bone_index, &attach->position,
                    &attach->rotation, &attach->scale, 1);
            }
        }
    }
}

/*
 * Soft ceiling: player_impale ~95.26% - the remaining differences are
 * nonvolatile-register allocation and redundant latch-copy moves.
 */
void player_impale(MkObj* weapon, MkObj* second_weapon) {
    WeaponDefinition* definition;
    WeaponImpaleData* impale;
    PlyrPdata* victim;
    MkObj* victim_object;
    MkObj* secondary;
    ImpaleSecondaryObject* attachment;
    MkHdr* weapon_hdr;

    definition = (WeaponDefinition*)weapon->field_5C;
    if (definition == 0) {
        return;
    }
    impale = definition->impale_data;
    if (impale == 0) {
        return;
    }
    victim = plyr_pdata->his_plyr_pdata;
    if (victim == 0) {
        return;
    }

    plyr_weapon_trail_hide(plyr_pdata->mirror_slots);
    if (impale->bone_index > 0) {
        RESOLVE_WEAPON_LATCH(victim_object, &victim->tracked_obj_latch);
        if (victim_object != 0) {
            mkobj_zero_bone_rots(weapon);
            if (weapon != 0) {
                weapon_hdr = as_mkhdr(&weapon->hdr);
            } else {
                weapon_hdr = 0;
            }
            update_bone_hierarchy(weapon_hdr);
            if (plyr_obj_item_grab(
                    victim, &victim->impaled_item_a,
                    &victim->impaled_item_a_secondary, weapon,
                    impale->bone_index, &impale->position,
                    &impale->rotation, &impale->scale, 0) != 0) {
                RESOLVE_WEAPON_LATCH(
                    secondary, &victim->impaled_item_a_secondary);
                attachment = (ImpaleSecondaryObject*)secondary;
                if (attachment != 0) {
                    attachment->strength = 5.0f;
                }
                start_gusher(
                    heart_beat, (int)victim, victim_object,
                    impale->bone_index, &impale->position,
                    &impale->gusher_direction);
            }
        }
    }

    if (second_weapon != 0) {
        definition = (WeaponDefinition*)second_weapon->field_5C;
        impale = definition->impale_data;
        if (impale->bone_index > 0) {
            RESOLVE_WEAPON_LATCH(victim_object, &victim->tracked_obj_latch);
            if (victim_object != 0) {
                mkobj_zero_bone_rots(second_weapon);
                if (second_weapon != 0) {
                    weapon_hdr = as_mkhdr(&second_weapon->hdr);
                } else {
                    weapon_hdr = 0;
                }
                update_bone_hierarchy(weapon_hdr);
                if (plyr_obj_item_grab(
                        victim, &victim->impaled_item_a,
                        &victim->impaled_item_a_secondary, second_weapon,
                        impale->bone_index, &impale->position,
                        &impale->rotation, &impale->scale, 0) != 0) {
                    RESOLVE_WEAPON_LATCH(
                        secondary, &victim->impaled_item_a_secondary);
                    attachment = (ImpaleSecondaryObject*)secondary;
                    if (attachment != 0) {
                        attachment->strength = 5.0f;
                    }
                    start_gusher(
                        heart_beat, (int)victim, victim_object,
                        impale->bone_index, &impale->position,
                        &impale->gusher_direction);
                }
            }
        }
    }

    advance_my_moveset();
    weapon->hide_flag_bits.hidden = 0;
    weapon->flags_08_bits.gravity_enabled = 0;
    weapon->flags_08_bits.rotation_enabled = 0;
    if (second_weapon != 0) {
        second_weapon->hide_flag_bits.hidden = 0;
        second_weapon->flags_08_bits.gravity_enabled = 0;
        second_weapon->flags_08_bits.rotation_enabled = 0;
    }
}

MkObj* load_weapon_reflection(
    WeaponDefinition* definition, MkObj* player_object) {
    MkObj* reflection;

    if (definition->reflection_model_name == 0) {
        return 0;
    }
    reflection = (MkObj*)load_named_model_for_player(
        definition->reflection_model_name,
        get_player_number(player_object), 0x5013, 0);
    if (reflection == 0) {
        return 0;
    }
    if (definition->bone_tags == 0) {
        if (reflection->hdr.instance != 0) {
            reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
        }
        return 0;
    }

    SetupShadowPlayerPipeline(reflection->clump);
    if (build_bones_tbl(reflection, definition->bone_tags) == 0) {
        if (reflection->hdr.instance != 0) {
            reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
        }
        return 0;
    }
    pull_bone_hierarchy_mkobj(reflection);
    reflection->light_flags = 4;
    reflection->hide_flag_bits.hidden = 1;
    insert_fgnd_mkobj(reflection);
    obj_create_sobjs(reflection);
    sobj_set_priority(obj_first_sobj(reflection), 6);
    if (g_game_info.field_08 != 0 &&
        (g_game_info.section->flags70 & 8) == 0) {
        hide_obj(reflection);
    }
    return reflection;
}

MkObj* load_bgnd_weapon_reflection(WeaponDefinition* definition) {
    MkObj* reflection;

    if (definition->reflection_model_name == 0) {
        return 0;
    }
    reflection = (MkObj*)load_named_model_for_bgnd(
        definition->reflection_model_name, 0x5013, 0);
    if (reflection == 0) {
        return 0;
    }
    if (definition->bone_tags == 0) {
        if (reflection->hdr.instance != 0) {
            reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
        }
        reflection = 0;
    } else {
        SetupShadowPlayerPipeline(reflection->clump);
        if (build_bones_tbl(reflection, definition->bone_tags) == 0) {
            if (reflection->hdr.instance != 0) {
                reflection->hdr.typed_vtbl->destroy((MkHdr*)reflection);
            }
            reflection = 0;
        } else {
            pull_bone_hierarchy_mkobj(reflection);
            reflection->light_flags = 4;
            reflection->hide_flag_bits.hidden = 1;
            insert_fgnd_mkobj(reflection);
            obj_create_sobjs(reflection);
            sobj_set_priority(obj_first_sobj(reflection), 6);
            if (g_game_info.field_08 != 0 &&
                (g_game_info.section->flags70 & 8) == 0) {
                hide_obj(reflection);
            }
        }
    }
    if (reflection != 0 &&
        (g_game_info.section->flags70 & 8) != 0) {
        reflection->hide_flag_bits.hidden = 0;
        return reflection;
    }
    return 0;
}

MkObj* load_weapon_from_slot(WeaponDefinition* definition, int slot) {
    MkObj* weapon;
    MkObj* trail_model;
    unsigned int bone_index;

    trail_model = 0;
    weapon = (MkObj*)load_named_model_from_slot(
        slot, definition->model_name, 0x1008, 1);
    if (weapon == 0 && definition->model_name != 0) {
        return 0;
    }
    if (definition->secondary_model_name != 0) {
        trail_model = (MkObj*)load_named_model_from_slot(
            slot, definition->secondary_model_name, 0x5004, 1);
        if (trail_model == 0) {
            return 0;
        }
    }

    if (definition->bone_tags != 0) {
        build_bones_tbl(weapon, definition->bone_tags);
        for (bone_index = 0; bone_index < weapon->bone_count; bone_index++) {
            MkBone* bone;

            bone = weapon->bones[bone_index];
            if (bone != 0 && (bone->flags_54 & 0x20) == 0) {
                bone->flags_54 |= 0x10;
            }
        }
        specskin_initialize_clump(weapon->clump);
        specskin_force_clipping_clump(weapon->clump, 1);
    }
    obj_force_culling_off(weapon);
    start_cloth_bones(weapon);
    weapon->light_flags = 0x100C;
    weapon->hide_flag_bits.hidden = 1;
    weapon->hide_flag_bits.weapon_effect = 1;
    weapon->field_5C = definition;
    insert_fgnd_mkobj(weapon);
    start_weapon_trail(weapon, trail_model);
    return weapon;
}

MkObj* load_weapon(
    WeaponDefinition* definition, MkObj* player_object) {
    PlyrPdata* player;
    MkObj* weapon;
    MkObj* trail_model;
    RpMaterial* material;
    unsigned int bone_index;
    int player_number;

    trail_model = 0;
    player_number = get_player_number(player_object);
    weapon = (MkObj*)load_named_model_for_player(
        definition->model_name, player_number, 0x1008, 1);
    if (weapon == 0) {
        return 0;
    }
    if (definition->secondary_model_name != 0) {
        trail_model = (MkObj*)load_named_model_for_player(
            definition->secondary_model_name,
            player_number, 0x5004, 1);
        if (trail_model == 0) {
            return 0;
        }
    }

    player = 0;
    if (player_object->oid == 0x1001) {
        player = (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    } else if (player_object->oid == 0x1002) {
        player = (PlyrPdata*)g_game_info.plyr1.slot.fighter;
    }
    if (player != 0 && player->character_id == 0xA &&
        player->plyr_info->flags_14_bits.alternate_costume) {
        obj_create_sobjs(weapon);
        material = obj_find_material_by_id(weapon, 1);
        if (material != 0) {
            material_set_zbias(material, -0.07f);
        }
    }

    if (definition->bone_tags != 0) {
        build_bones_tbl(weapon, definition->bone_tags);
        for (bone_index = 0; bone_index < weapon->bone_count; bone_index++) {
            MkBone* bone;

            bone = weapon->bones[bone_index];
            if (bone != 0 && (bone->flags_54 & 0x20) == 0) {
                bone->flags_54 |= 0x10;
            }
        }
        specskin_initialize_clump(weapon->clump);
        specskin_force_clipping_clump(weapon->clump, 1);
    }
    obj_force_culling_off(weapon);
    start_cloth_bones(weapon);
    weapon->light_flags = 0x100C;
    weapon->hide_flag_bits.hidden = 1;
    weapon->hide_flag_bits.weapon_effect = 1;
    weapon->field_5C = definition;
    insert_fgnd_mkobj(weapon);
    start_weapon_trail(weapon, trail_model);
    return weapon;
}

/*
 * Soft ceiling: the retail stack matrix is 16-byte aligned. Keep the matrix
 * portable instead of forcing the frame with a function-local attribute.
 */
void mkobj_update_weapon_trail(MkObj* trail_model) {
    MkObj* weapon;
    WeaponDefinition* definition;
    RwMatrix* weapon_matrix;
    RwMatrix* trail_matrix;
    WeaponTrailMap* map;
    MkBone* trail_bone;
    MkBone* parent_bone;
    MkBone* child_bone;
    Vec displacement;
    Vec parent_to_child;
    Vec child_direction;
    Quat rotation;
    MKMATRIX rotation_matrix;
    int* chain_root;
    int map_index;

    weapon = (MkObj*)trail_model->parent_hdr;
    if (weapon != 0 && weapon->hdr.instance != trail_model->parent_inst) {
        weapon = 0;
    }
    do {
        if (weapon == 0 || weapon->field_5C == 0 ||
            trail_model->field_5C == 0) {
            break;
        }
        weapon_matrix = &weapon->frame->modelling;
        trail_matrix = &trail_model->frame->modelling;
        v3_sub_v3(&displacement, &weapon_matrix->pos_vec,
                  &trail_matrix->pos_vec);
        trail_matrix->pos_vec = weapon_matrix->pos_vec;

        definition = (WeaponDefinition*)trail_model->field_5C;
        for (map_index = 0;
            map_index < definition->trail_map_count;
             map_index++) {
            map = &definition->trail_maps[map_index];
            if (map->enabled == 0) {
                break;
            }
            trail_bone = trail_model->bones[map->trail_bone_index];
            if (trail_bone == 0) {
                definition = 0;
                break;
            }
            parent_bone = trail_bone->transform_parent;
            v3_x_mat_add_v3(&trail_bone->parent_matrix->pos_vec,
                            &trail_bone->translation,
                            &parent_bone->matrix,
                            &parent_bone->matrix.pos_vec);
            v3_sub_v3(&trail_bone->parent_matrix->pos_vec,
                      &trail_bone->parent_matrix->pos_vec,
                      &trail_matrix->pos_vec);
            memcpy(trail_bone->parent_matrix, &parent_bone->matrix, 0x30);
            definition = (WeaponDefinition*)trail_model->field_5C;
        }
        if (definition == 0) {
            break;
        }

        chain_root = definition->trail_chain_roots;
        if (chain_root != 0) {
            while (*chain_root != 0) {
                trail_bone = trail_model->bones[*chain_root];
                if (trail_bone == 0) {
                    definition = 0;
                    break;
                }
                chain_root++;

                do {
                    child_bone = trail_bone;
                    trail_bone = trail_bone->transform_parent;
                    memcpy(child_bone->parent_matrix,
                           &trail_bone->trail_matrix,
                           sizeof(*child_bone->parent_matrix));
                    child_bone->parent_matrix->pos.x -= displacement.x;
                    child_bone->parent_matrix->pos.y -= displacement.y;
                    child_bone->parent_matrix->pos.z -= displacement.z;
                    memcpy(&trail_bone->trail_matrix,
                           trail_bone->parent_matrix,
                           sizeof(trail_bone->trail_matrix));
                    trail_bone->trail_matrix.pos.x -= displacement.x;
                    trail_bone->trail_matrix.pos.y -= displacement.y;
                    trail_bone->trail_matrix.pos.z -= displacement.z;
                } while (!trail_bone->flags_54_bits.transform_parented);

                parent_bone = trail_bone->transform_parent;
                v3_x_mat_add_v3(&trail_bone->parent_matrix->pos_vec,
                                &trail_bone->translation,
                                &parent_bone->matrix,
                                &parent_bone->matrix.pos_vec);
                v3_sub_v3(&trail_bone->parent_matrix->pos_vec,
                          &trail_bone->parent_matrix->pos_vec,
                          &trail_matrix->pos_vec);
                v3_x_mat(&parent_to_child, &trail_p_to_c_uv,
                         &parent_bone->matrix);
                uv_v3_to_v3(
                            &child_direction,
                            &trail_bone->parent_matrix->pos_vec,
                            &child_bone->parent_matrix->pos_vec);
                v3_v3_to_quat(&rotation, &parent_to_child, &child_direction);
                quat_to_mat(&rotation_matrix, &rotation);
                mat_x_mat(trail_bone->parent_matrix, &parent_bone->matrix,
                          &rotation_matrix);
            }
            if (definition == 0) {
                break;
            }
            RwFrameUpdateObjects(trail_model->frame);
            return;
        }
    } while (0);

    if (trail_model->hdr.instance != 0) {
        trail_model->hdr.typed_vtbl->destroy((MkHdr*)trail_model);
    }
}

/*
 * Soft ceiling: start_weapon_trail ~94.27% - the remaining shared failure
 * edge and nonvolatile-register allocation differ; the recovered operations
 * and object/bone mutations agree with retail.
 */
void start_weapon_trail(MkObj* weapon, MkObj* trail_model) {
    WeaponDefinition* definition;
    MkSobj* sobj;
    RpMaterial* material;
    unsigned int bone_index;
    int map_index;

    if (trail_model == 0) {
        return;
    }

    do {
        definition = (WeaponDefinition*)weapon->field_5C;
        if (definition == 0 || definition->secondary_model_name == 0) {
            break;
        }
        if (definition->trail_bone_tags != 0) {
            if (build_bones_tbl(trail_model, definition->trail_bone_tags) == 0) {
                break;
            }
            pull_bone_hierarchy_mkobj(trail_model);
        }

        obj_create_sobjs(trail_model);
        sobj = obj_first_sobj(trail_model);
        if (sobj != 0) {
            material = sobj_find_material_by_id(sobj, 1);
            if (material != 0) {
                sobj_use_material_color(sobj);
                obj_set_material_fade(trail_model, 1, 0);
                material_set_zbias(material, 0.2f);
            }
            sobj->render_flags = 0x20002;
            sobj->flags09_bits.bit4 = 1;
            sobj->flags_08_bits.bit0 = 0;
            sobj_set_priority(sobj, 0x14);
        }

        mk_insert(&trail_model->hdr, &weapon_trail_mkobj_list);
        trail_model->parent_hdr = &weapon->hdr;
        trail_model->parent_inst = weapon->hdr.instance;
        mk_insert(&trail_model->hdr, &weapon->list_44);
        mk_insert(&trail_model->hdr, &weapon->child_list);
        trail_model->field_5C = weapon->field_5C;
        trail_model->light_flags = 0x10;
        trail_model->hide_flag_bits.hidden = 1;
        insert_fgnd_mkobj(trail_model);

        for (bone_index = 0; bone_index < trail_model->bone_count; bone_index++) {
            MkBone* bone;

            bone = trail_model->bones[bone_index];
            if (bone == 0) {
                break;
            }
            bone->flags_54_bits.calculation_locked = 0;
            bone->flags_54_bits.hierarchy_driven = 1;
        }
        if (bone_index != trail_model->bone_count) {
            break;
        }

        for (map_index = 0; map_index < definition->trail_map_count; map_index++) {
            WeaponTrailMap* map;
            MkBone* trail_bone;
            MkBone* weapon_bone;

            map = &definition->trail_maps[map_index];
            trail_bone = trail_model->bones[map->trail_bone_index];
            if (trail_bone != 0) {
                weapon_bone = weapon->bones[map->weapon_bone_index];
                if (weapon_bone != 0) {
                    weapon_bone->flags_54_bits.calculation_locked = 1;
                    trail_bone->transform_parent = weapon_bone;
                    trail_bone->flags_54_bits.transform_parented = 1;
                    trail_bone->translation = map->offset;
                    continue;
                }
            }
            break;
        }
        if (map_index == definition->trail_map_count) {
            return;
        }
    } while (0);

    if (trail_model->hdr.instance != 0) {
        trail_model->hdr.typed_vtbl->destroy((MkHdr*)trail_model);
    }
}

#define SHOW_WEAPON_TRAIL(latch)                                          \
    do {                                                                  \
        MkObj* weapon_;                                                   \
        MkObj* trail_;                                                    \
        MkObj* parent_;                                                   \
        MkHdr* raw_trail_;                                                \
        MkProc* fade_proc_;                                               \
        weapon_ = (latch)->obj;                                           \
        if (weapon_ != 0) {                                               \
            if (weapon_->hdr.instance == (latch)->instance) {             \
                /* Keep the validated weapon. */                          \
            } else {                                                      \
                weapon_ = 0;                                              \
            }                                                             \
        } else {                                                          \
            weapon_ = 0;                                                  \
        }                                                                 \
        if (weapon_ != 0) {                                               \
            raw_trail_ = first_mkhdr(&weapon_->list_44);                  \
            if (raw_trail_ != 0) {                                       \
                trail_ = (MkObj*)raw_trail_;                              \
                if (trail_ != 0) {                                       \
                    parent_ = (MkObj*)trail_->parent_hdr;                 \
                    if (parent_ != 0) {                                   \
                        if (parent_->hdr.instance ==                      \
                            trail_->parent_inst) {                        \
                            /* Keep the validated parent. */              \
                        } else {                                          \
                            parent_ = 0;                                  \
                        }                                                 \
                    } else {                                              \
                        parent_ = 0;                                      \
                    }                                                     \
                    if (parent_ == 0) {                                   \
                        if (trail_->hdr.instance != 0) {                  \
                            trail_->hdr.typed_vtbl->destroy(              \
                                (MkHdr*)trail_);                           \
                        }                                                 \
                    } else {                                              \
                        if (!trail_->hide_flag_bits.hidden) {             \
                            destroy_mkprocs_pid_from_list(                \
                                0x5010, &trail_->child_list);             \
                        }                                                 \
                        fade_proc_ = fade_material(5.0f, trail_, 0, 1,    \
                                                   0x10);                \
                        if (fade_proc_ != 0) {                            \
                            mk_insert(&fade_proc_->hdr,                   \
                                      &trail_->child_list);               \
                        }                                                 \
                        trail_->hide_flag_bits.hidden = 0;                \
                        RwFrameUpdateObjects(trail_->frame);              \
                    }                                                     \
                }                                                         \
            }                                                             \
        }                                                                 \
    } while (0)

/*
 * Soft ceiling: plyr_weapon_trail_show ~96.58% - the remaining delta is the
 * compiler's redundant null-branch emission in the four expanded blocks.
 */
void plyr_weapon_trail_show(PlyrMirrorSlots* slots) {
    if (slots != 0) {
        SHOW_WEAPON_TRAIL(&slots->weapon[0].primary);
        SHOW_WEAPON_TRAIL(&slots->weapon[1].primary);
        SHOW_WEAPON_TRAIL(&slots->weapon[2].primary);
        SHOW_WEAPON_TRAIL(&slots->weapon[3].primary);
    }
}

#undef SHOW_WEAPON_TRAIL

#define HIDE_WEAPON_TRAIL(latch)                                          \
    do {                                                                  \
        MkObj* weapon_;                                                   \
        MkObj* trail_;                                                    \
        MkHdr* raw_trail_;                                                \
        MkProc* fade_proc_;                                               \
        weapon_ = (latch)->obj;                                           \
        if (weapon_ != 0) {                                               \
            if (weapon_->hdr.instance == (latch)->instance) {             \
                /* Keep the validated weapon. */                          \
            } else {                                                      \
                weapon_ = 0;                                              \
            }                                                             \
        } else {                                                          \
            weapon_ = 0;                                                  \
        }                                                                 \
        if (weapon_ != 0) {                                               \
            raw_trail_ = first_mkhdr(&weapon_->list_44);                  \
            if (raw_trail_ != 0) {                                       \
                trail_ = (MkObj*)raw_trail_;                              \
                if (trail_ != 0 && !trail_->hide_flag_bits.hidden) {      \
                    destroy_mkprocs_pid_from_list(0x5010,                 \
                                                   &trail_->child_list);  \
                    fade_proc_ = fade_material(-10.0f, trail_, 0, 1,      \
                                               0x1A);                    \
                    if (fade_proc_ != 0) {                               \
                        mk_insert(&fade_proc_->hdr, &trail_->child_list); \
                    }                                                     \
                }                                                         \
            }                                                             \
        }                                                                 \
    } while (0)

/*
 * Soft ceiling: plyr_weapon_trail_hide ~97.09% - repeated latch blocks differ
 * only in the compiler's redundant null-branch emission.
 */
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots) {
    if (slots != 0) {
        HIDE_WEAPON_TRAIL(&slots->weapon[0].primary);
        HIDE_WEAPON_TRAIL(&slots->weapon[1].primary);
        HIDE_WEAPON_TRAIL(&slots->weapon[2].primary);
        HIDE_WEAPON_TRAIL(&slots->weapon[3].primary);
    }
}

#undef HIDE_WEAPON_TRAIL

MkObj* plyr_weapon2_release(PlyrPdata* player) {
    PlyrMirrorSlots* slots;

    slots = player->mirror_slots;
    if (slots == 0) {
        return 0;
    }
    return plyr_obj_item_release(player, &slots->weapon[1].primary,
                                 &slots->weapon[1].secondary);
}

MkObj* plyr_weapon_release(PlyrPdata* player) {
    PlyrMirrorSlots* slots;

    slots = player->mirror_slots;
    if (slots == 0) {
        return 0;
    }
    return plyr_obj_item_release(player, &slots->weapon[0].primary,
                                 &slots->weapon[0].secondary);
}

void plyr_weapon4_grab(PlyrPdata* player, MkObj* item) {
    PlyrMirrorSlots* slots;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    {
        MkObj* item_arg;

        item_arg = item;
        plyr_obj_item_grab(player, &slots->weapon[3].primary,
                           &slots->weapon[3].secondary, item_arg,
                           attach->bone_index, &attach->position,
                           &attach->rotation, &attach->scale, 1);
    }
}

void plyr_weapon3_grab(PlyrPdata* player, MkObj* item) {
    PlyrMirrorSlots* slots;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    {
        MkObj* item_arg;

        item_arg = item;
        plyr_obj_item_grab(player, &slots->weapon[2].primary,
                           &slots->weapon[2].secondary, item_arg,
                           attach->bone_index, &attach->position,
                           &attach->rotation, &attach->scale, 1);
    }
}

void plyr_weapon2_grab(PlyrPdata* player, MkObj* item) {
    PlyrMirrorSlots* slots;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    {
        MkObj* item_arg;

        item_arg = item;
        plyr_obj_item_grab(player, &slots->weapon[1].primary,
                           &slots->weapon[1].secondary, item_arg,
                           attach->bone_index, &attach->position,
                           &attach->rotation, &attach->scale, 1);
    }
}

void plyr_weapon_grab(PlyrPdata* player, MkObj* item) {
    PlyrWeaponMirrorSlot* slot;
    MkObjItemAttachData* attach;

    if (player->mirror_slots == 0) {
        return;
    }
    slot = &player->mirror_slots->weapon[0];
    attach = item->item_attach_data;
    plyr_obj_item_grab(player, &slot->primary, &slot->secondary, item,
                       attach->bone_index, &attach->position,
                       &attach->rotation, &attach->scale, 1);
}

/*
 * Soft ceiling: plyr_obj_item_grab ~93.19% - attachment behavior and memory
 * operations match; remaining differences are latch-branch and GPR coloring.
 */
int plyr_obj_item_grab(PlyrPdata* player,
                       PlyrMirrorObjLatch* item_latch,
                       PlyrMirrorObjLatch* secondary_latch, MkObj* item,
                       int bone_index, const Vec* position,
                       const Vec* rotation, const Vec* scale,
                       int insert_at_head) {
    MkObj* player_object;
    MkObj* current_item;
    RwMatrix* bone_matrix;
    MkBone* bone;
    WeaponBoneMatcherState* matcher;
    MkHdr* item_hdr;

    matcher = 0;
    RESOLVE_WEAPON_LATCH(player_object, &player->tracked_obj_latch);
    if (player_object == 0) {
        return 0;
    }

    RESOLVE_WEAPON_LATCH(current_item, item_latch);
    if (current_item == item) {
        plyr_obj_item_release(player, item_latch, secondary_latch);
    } else {
        plyr_obj_item_release(player, item_latch, secondary_latch);
    }

    item->parent_hdr = &player_object->hdr;
    item->parent_inst = player_object->hdr.instance;
    bone = item->bones[item->fallback_bone_index];
    bone_matrix = bone->parent_matrix;
    bone_matrix->pos.z = 0.0f;
    bone_matrix->pos.y = 0.0f;
    bone_matrix->pos.x = 0.0f;
    ZYX_angles_to_MKMATRIX(rotation, bone_matrix);
    RtQuatConvertFromMatrix(&bone->rt_rotation, bone_matrix);

    if (insert_at_head) {
        mk_insert(&item->hdr, &player_object->list_44);
    } else {
        mk_append(&item->hdr, &player_object->list_44);
        if (item != 0) {
            item_hdr = as_mkhdr(&item->hdr);
        } else {
            item_hdr = 0;
        }
        update_bone_hierarchy(item_hdr);
        mkobj_bones_dest_mat_no_update(item);
    }

    if (secondary_latch != 0) {
        matcher = (WeaponBoneMatcherState*)secondary_latch->obj;
        if (matcher != 0) {
            if (matcher->hdr.instance == secondary_latch->instance) {
                /* Keep the validated matcher. */
            } else {
                matcher = 0;
            }
        } else {
            matcher = 0;
        }
    }
    if (matcher == 0) {
        matcher = start_bone_matcher(
            0.0f, player_object, bone_index, item, 0);
    }
    if (matcher != 0) {
        mk_insert(&matcher->hdr, &item->child_list);
        matcher->parent_offset.x = position->x;
        matcher->parent_offset.y = position->y;
        matcher->parent_offset.z = position->z;
        matcher->child_offset.x = scale->x;
        matcher->child_offset.y = scale->y;
        matcher->child_offset.z = scale->z;
        matcher->parent_translation.x = scale->x;
        matcher->parent_translation.y = scale->y;
        matcher->parent_translation.z = scale->z;
        matcher->mirrored_parent_translation.x = scale->x;
        matcher->mirrored_parent_translation.y = scale->y;
        matcher->mirrored_parent_translation.z = scale->z;
        matcher->mirrored_parent_translation.x *= -1.0f;
        if (insert_at_head) {
            matcher->flags_08_bits.inactive = 0;
            matcher->flags_08_bits.preserve_bone_matrix = 1;
            matcher->flags_08_bits.copy_parent_angles = 1;
            matcher->flags_08_bits.blend_child_transform = 1;
            matcher->flags_09_bits.copy_child_flip = 1;
        } else {
            matcher->flags_08_bits.copy_bone_matrix = 1;
        }

        if (secondary_latch != 0) {
            secondary_latch->obj = (MkObj*)matcher;
            secondary_latch->instance = matcher->hdr.instance;
        } else {
            mk_insert(&matcher->hdr, &player->item_links);
        }
        item_latch->obj = item;
        item_latch->instance = item->hdr.instance;
        return 1;
    }
    return 0;
}

MkObj* plyr_obj_item_release(PlyrPdata* player,
                             PlyrMirrorObjLatch* item_latch,
                             PlyrMirrorObjLatch* secondary_latch) {
    MkObj* player_object;
    MkObj* item;
    MkObj* linked_item;
    MkPtr* link_ptr;
    MkPtr* next;

    RESOLVE_WEAPON_LATCH(
        player_object, &player->tracked_obj_latch);
    if (player_object == 0) {
        return 0;
    }

    RESOLVE_WEAPON_LATCH(item, item_latch);
    if (item != 0) {
        MkHdr* object_to_destroy;

        mkobj_zero_bone_rots(item);
        mk_pull_discard(&item->hdr, &player_object->list_44);

        if (secondary_latch != 0) {
            MkObj* secondary_object;

            RESOLVE_WEAPON_LATCH(secondary_object, secondary_latch);
            object_to_destroy = (MkHdr*)secondary_object;
        } else {
            object_to_destroy = 0;
            link_ptr = player->item_links;
            while (link_ptr != 0) {
                WeaponBoneMatcherState* link;

                if (link_ptr->instance != link_ptr->hdr->instance) {
                    next = link_ptr->next;
                    link_ptr->hdr = 0;
                    destroy_mkptr(link_ptr);
                    link_ptr = next;
                    continue;
                }
                link = (WeaponBoneMatcherState*)link_ptr->hdr;
                linked_item = link->child_latch.obj;
                if (linked_item != 0) {
                    if (linked_item->hdr.instance ==
                        link->child_latch.instance) {
                        /* Keep the validated linked item. */
                    } else {
                        linked_item = 0;
                    }
                } else {
                    linked_item = 0;
                }
                if (linked_item == item) {
                    object_to_destroy = &link->hdr;
                    break;
                }
                link_ptr = link_ptr->next;
            }
        }

        if (object_to_destroy != 0 && object_to_destroy->instance != 0) {
            object_to_destroy->typed_vtbl->destroy(object_to_destroy);
        }

        link_ptr = item->child_list;
        while (link_ptr != 0) {
            if (link_ptr->instance != link_ptr->hdr->instance) {
                next = link_ptr->next;
                link_ptr->hdr = 0;
                destroy_mkptr(link_ptr);
                link_ptr = next;
            } else {
                link_ptr = link_ptr->next;
            }
        }
    }
    return item;
}

/*
 * Soft ceiling: plyr_weapon_show ~99.40% -
 * tracked-object latch has one extra move/NV island; stop.
 */
void plyr_weapon_show(PlyrPdata* player, int show_aux,
                      PlyrMirrorSlots* slots) {
    MkObj* object;

    if (slots == 0) {
        return;
    }

    RESOLVE_WEAPON_LATCH(object, &slots->weapon[0].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 0;
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[1].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 0;
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[2].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 0;
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[3].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 0;
    }

    if (player->character_id == 0x12) {
        RESOLVE_WEAPON_LATCH(object, &player->tracked_obj_latch);
        if (object != 0) {
            if (player->plyr_info->flags_14_bits.alternate_costume) {
                obj_hide_material_by_id(object, 0x83);
                obj_hide_material_by_id(object, 0x65);
            } else {
                obj_hide_material_by_id(object, 0x79);
                obj_hide_material_by_id(object, 0x5B);
            }
        }
    }

    if (show_aux) {
        RESOLVE_WEAPON_LATCH(object, &player->aux_weapon_latch);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
        RESOLVE_WEAPON_LATCH(object, &player->mirror_obj);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
    }

    if (g_game_info.section != 0 &&
        (g_game_info.section->flags70 & 8) != 0) {
        RESOLVE_WEAPON_LATCH(object, &slots->weapon[0].mirror);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
        RESOLVE_WEAPON_LATCH(object, &slots->weapon[1].mirror);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
    }
}

/*
 * Soft ceiling: plyr_weapon_hide ~99.40% -
 * tracked-object latch has one extra move/NV island; stop.
 */
void plyr_weapon_hide(PlyrPdata* player, int show_aux,
                      PlyrMirrorSlots* slots) {
    MkObj* object;

    if (slots == 0) {
        return;
    }

    RESOLVE_WEAPON_LATCH(object, &slots->weapon[0].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
        plyr_weapon_trail_hide(slots);
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[1].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
        plyr_weapon_trail_hide(slots);
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[2].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
        plyr_weapon_trail_hide(slots);
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[3].primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
        plyr_weapon_trail_hide(slots);
    }

    if (player->character_id == 0x12) {
        RESOLVE_WEAPON_LATCH(object, &player->tracked_obj_latch);
        if (object != 0) {
            if (player->plyr_info->flags_14_bits.alternate_costume) {
                obj_unhide_material_by_id(object, 0x83);
                obj_unhide_material_by_id(object, 0x65);
            } else {
                obj_unhide_material_by_id(object, 0x79);
                obj_unhide_material_by_id(object, 0x5B);
            }
        }
    }

    if (show_aux) {
        RESOLVE_WEAPON_LATCH(object, &player->aux_weapon_latch);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
        RESOLVE_WEAPON_LATCH(object, &player->mirror_obj);
        if (object != 0) {
            object->hide_flag_bits.hidden = 0;
        }
    }

    RESOLVE_WEAPON_LATCH(object, &slots->weapon[0].mirror);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
    }
    RESOLVE_WEAPON_LATCH(object, &slots->weapon[1].mirror);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
    }
}

/*
 * Soft ceiling: plyr_match_weapon_flip_to_obj_flip ~93.77% -
 * NV-register coloring and mirror-slot base reloads; stop.
 */
void plyr_match_weapon_flip_to_obj_flip(PlyrPdata* player) {
    MkObj* player_object;
    MkObj* weapon_object;
    PlyrMirrorSlots* slots;

    RESOLVE_WEAPON_LATCH(player_object, &player->tracked_obj_latch);
    if (player_object == 0) {
        return;
    }

    slots = player->mirror_slots;
    RESOLVE_WEAPON_LATCH(weapon_object, &slots->weapon[0].primary);
    if (weapon_object != 0) {
        weapon_object->hide_flag_bits.bit6 =
            player_object->hide_flag_bits.bit6;
    }
    RESOLVE_WEAPON_LATCH(weapon_object, &slots->weapon[1].primary);
    if (weapon_object != 0) {
        weapon_object->hide_flag_bits.bit6 =
            player_object->hide_flag_bits.bit6;
    }
    RESOLVE_WEAPON_LATCH(weapon_object, &slots->weapon[2].primary);
    if (weapon_object != 0) {
        weapon_object->hide_flag_bits.bit6 =
            player_object->hide_flag_bits.bit6;
    }
    RESOLVE_WEAPON_LATCH(weapon_object, &slots->weapon[3].primary);
    if (weapon_object != 0) {
        weapon_object->hide_flag_bits.bit6 =
            player_object->hide_flag_bits.bit6;
    }
}

void init_weapon_trails(void) {
    weapon_trail_mkobj_list = 0;
}
