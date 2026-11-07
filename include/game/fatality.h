#ifndef GAME_FATALITY_H
#define GAME_FATALITY_H

typedef struct Vec Vec;

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
void sindel_scream_react_sound_start(void);
void sindel_sonic_sounds(struct FatalityObjectLatch* sound, int finished);
void set_victim_v3_units_away(float a, float b);
void mks_start_fatality_iceball(int player);
void fatality_release_other_player(void);
void start_bodyslam_bodysplat(float a, float b, float c, float d, float e);
void fade_fatality_screen(void);
float subzero_his_tinkle_snd(void);
void fix_axe_angle(const Vec* angles);
void ft_mileena_start_veil_ripoff(void);
float start_fatality(void);
extern void play_final_fatality_music(void);
float start_suicide(void);
float start_2nd_fatality(void);

#ifdef __cplusplus
}
#endif

#endif
