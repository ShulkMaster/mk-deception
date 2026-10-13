#ifndef GAME_FATALITY_H
#define GAME_FATALITY_H

#include "math/gxVect.h"

struct MkObj;
struct MkHdr;
struct FatalityWeaponAttachment;
struct FatalityRadiusCheck;
struct FatalityEmitterBind;

#ifdef __cplusplus
extern "C" {
#endif

struct FatalityWeaponAttachment* regrab_weapon(
    int secondary, struct MkObj* object, struct MkHdr* bound_object, int bone_id,
    const Vec* angles, const Vec* scale, const Vec* position);
int fat_bgnd_char_setup_radius_check(struct FatalityRadiusCheck* check);
int fatality_check_distance(unsigned int action);
unsigned int fatality_boraicho_light_fart_torch(struct MkObj* torch);
void fire_multi_emitter_pfx_via_tbl(
    const char* name, struct FatalityEmitterBind* table,
    struct MkObj* object, unsigned int* handles);
unsigned int pfxhandle_bgnd_spawn_at_sobj_id(
    const char* name, unsigned int sobj_id);
unsigned int pfxhandle_spawn_at_bid_next_bind_render(
    unsigned int effect, struct MkObj* object, int bone_id);
unsigned int pfxhandle_spawn_at_bid_next(
    unsigned int effect, struct MkObj* object, int bone_id);
unsigned int pfxhandle_spawn_at_bid(
    const char* name, struct MkObj* object, int bone_id);
void start_obj_scalar_proc(struct MkObj* object, Vec* start, Vec* target, Vec* step);

#ifdef __cplusplus
}
#endif

#endif
