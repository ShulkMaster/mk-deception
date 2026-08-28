#ifndef GAME_WEAPON_H
#define GAME_WEAPON_H

typedef struct FatalityWeaponSource FatalityWeaponSource;
typedef struct MkObj MkObj;
typedef struct WeaponDefinition WeaponDefinition;

MkObj* load_weapon(WeaponDefinition* definition, MkObj* player_object);
MkObj* load_weapon_reflection(
    WeaponDefinition* definition, MkObj* player_object);

MkObj* clone_my_weapon(
    WeaponDefinition* definition, FatalityWeaponSource* source);
void clone_weapon_to_secondary(
    WeaponDefinition* definition, FatalityWeaponSource* source);

#endif
