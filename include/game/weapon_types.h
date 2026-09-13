#ifndef GAME_WEAPON_TYPES_H
#define GAME_WEAPON_TYPES_H

#include "math/gxVect.h"
#include "game/weapon.h"

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

struct WeaponDefinition {
    const char* model_name;       /* +0x00 */
    const int* bone_tags;         /* +0x04 */
    int attachment_bone;
    Vec attachment_position;
    Vec attachment_rotation;
    Vec attachment_scale; /* +0x24 - vector passed to the bone matcher */
    const char* secondary_model_name; /* +0x30 */
    const int* trail_bone_tags; /* +0x34 */
    int trail_map_count; /* +0x38 */
    WeaponTrailMap* trail_maps; /* +0x3C */
    int* trail_chain_roots; /* +0x40 */
    const char* reflection_model_name; /* +0x44 */
    int field_48;
    Vec field_4c;
    float field_58;
    int field_5c; /* +0x5C - material and blood flags read by AI effects */
    Vec field_60;
    WeaponImpaleData* impale_data; /* +0x6C */
};

typedef WeaponDefinition MkObjItemAttachData;
typedef char WeaponDefinitionSizeCheck[sizeof(WeaponDefinition) == 0x70 ? 1 : -1];

#endif
