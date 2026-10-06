#ifndef GAME_FATALITY_H
#define GAME_FATALITY_H

#include "math/gxVect.h"

struct MkObj;
struct MkHdr;
struct FatalityWeaponAttachment;
struct FatalityRadiusCheck;

#ifdef __cplusplus
extern "C" {
#endif

struct FatalityWeaponAttachment* regrab_weapon(
    int secondary, struct MkObj* object, struct MkHdr* bound_object, int bone_id,
    const Vec* angles, const Vec* scale, const Vec* position);
int fat_bgnd_char_setup_radius_check(const struct FatalityRadiusCheck* check);
int fatality_check_distance(unsigned int action);
void start_obj_scalar_proc(struct MkObj* object, Vec* start, Vec* target, Vec* step);

#ifdef __cplusplus
}
#endif

#endif
