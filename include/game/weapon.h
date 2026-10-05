#ifndef GAME_WEAPON_H
#define GAME_WEAPON_H

typedef struct PlyrInfo PlyrInfo;
typedef struct MkObj MkObj;
typedef struct WeaponDefinition WeaponDefinition;

void init_weapon_trails(void);

MkObj* load_weapon(WeaponDefinition* definition, MkObj* player_object);
MkObj* load_weapon_reflection(
    WeaponDefinition* definition, MkObj* player_object);

MkObj* clone_my_weapon(
    WeaponDefinition* definition, PlyrInfo* source);
void clone_weapon_to_secondary(
    WeaponDefinition* definition, PlyrInfo* source);

#endif
