#include "game/game_info.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"

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

int plyr_obj_item_grab(PlyrPdata* player, PlyrMirrorObjLatch* item_latch,
                       PlyrMirrorObjLatch* secondary_latch, MkObj* item,
                       int bone_index, const Vec* position,
                       const Vec* rotation, const Vec* scale,
                       int insert_at_head);
int plyr_obj_item_release(PlyrPdata* player, PlyrMirrorObjLatch* item_latch,
                          PlyrMirrorObjLatch* secondary_latch);
void plyr_weapon_trail_hide(PlyrMirrorSlots* slots);
void obj_unhide_material_by_id(MkObj* object, int id);
void obj_hide_material_by_id(MkObj* object, int id);

void reload_fan(void) {
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

int plyr_weapon2_release(PlyrPdata* player) {
    PlyrMirrorSlots* slots;

    slots = player->mirror_slots;
    if (slots == 0) {
        return 0;
    }
    return plyr_obj_item_release(player, &slots->weapon[1].primary,
                                 &slots->weapon[1].secondary);
}

int plyr_weapon_release(PlyrPdata* player) {
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
    PlyrWeaponMirrorSlot* slot;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    slot = &slots->weapon[3];
    plyr_obj_item_grab(player, &slot->primary, &slot->secondary, item,
                       attach->bone_index, &attach->position,
                       &attach->rotation, &attach->scale, 1);
}

void plyr_weapon3_grab(PlyrPdata* player, MkObj* item) {
    PlyrMirrorSlots* slots;
    PlyrWeaponMirrorSlot* slot;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    slot = &slots->weapon[2];
    plyr_obj_item_grab(player, &slot->primary, &slot->secondary, item,
                       attach->bone_index, &attach->position,
                       &attach->rotation, &attach->scale, 1);
}

void plyr_weapon2_grab(PlyrPdata* player, MkObj* item) {
    PlyrMirrorSlots* slots;
    PlyrWeaponMirrorSlot* slot;
    MkObjItemAttachData* attach;

    slots = player->mirror_slots;
    if (slots == 0) {
        return;
    }
    attach = item->item_attach_data;
    if (attach == 0) {
        return;
    }
    slot = &slots->weapon[1];
    plyr_obj_item_grab(player, &slot->primary, &slot->secondary, item,
                       attach->bone_index, &attach->position,
                       &attach->rotation, &attach->scale, 1);
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
 * volatile NV coloring and mirror-slot base reloads; stop.
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
