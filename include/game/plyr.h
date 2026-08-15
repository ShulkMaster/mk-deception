#ifndef GAME_PLYR_H
#define GAME_PLYR_H

#include "runtime/plyr_pdata.h"
#include "runtime/mk_proc.h"

typedef struct AnimPdata AnimPdata;

PlyrPdata* get_mkpdata_plyr(void);
void init_mkpdata_plyrs(void);
void init_plyr_info_struct(PlyrInfo* player);
int load_plyr_model_async(int player, int char_id, int* flags);
void set_player_state(PlyrInfo* player, int state);

int plyr_pdata_get_previous_state(PlyrPdata* pdata);
int plyr_pdata_is_alt_costume(PlyrInfo* player);
int plyr_pdata_get_state(PlyrPdata* pdata);
void* plyr_pdata_get_pchr(PlyrPdata* pdata);
ScriptSlot* plyr_pdata_get_cmo(PlyrPdata* pdata);
int plyr_pdata_get_plyr_num(PlyrPdata* pdata);
MkObj* plyr_pdata_get_his_obj(PlyrPdata* pdata);
/* Returns plyr_info->slot.mirror_a (+0x5C), despite the retail name. */
void* plyr_pdata_get_plyr_obj(PlyrPdata* pdata);
PlyrPdata* plyr_pdata_get_his_plyr_pdata(PlyrPdata* pdata);
PlyrInfo* plyr_pdata_get_plyr_info(PlyrPdata* pdata);
PlyrPdata* get_my_plyr_pdata(void);
PlyrPdata* get_his_plyr_pdata(void);
PlyrPdata* get_plyr_pdata_plyr_num(int player);
MkObj* get_plyr_obj_plyr_num(int player);
int get_my_plyr_num(void);
MkObj* get_my_plyr_obj(void);
MkObj* get_his_plyr_obj(void);
PlyrInfo* get_plyr_info(void);
void cleanup_player_globals(void);
int get_my_particle_player_bank_num(void);
int plyr_pdata_sidekick_active(PlyrPdata* pdata);
MkObj* plyr_pdata_get_sidekick_obj(PlyrPdata* pdata);
MkObj* get_my_sidekick_obj(void);
float plyr_anim_get_frame(AnimPdata* pdata);
AnimPdata* get_my_plyr_anim_pdata(void);
int is_sidekick_active(PlyrInfo* player);
float player_sleep_forever(void);
void set_attack_type(int attack_type);

void create_player(int player_index, PlyrInfo* player);
void destroy_mkpdata_plyr(PlyrPdata* pdata);
void load_aux_weapon(WeaponDefinition* definition);
void puzzle_fighter_scale(MkObj* object, float scale);
void release_other_player(void);
MkHdr* plyr_grab_other_flip_states(
    int player_flip, int opponent_flip);
void rnd_plyrs(void);
void start_plyrs(void);
void swap_active_plyr_proc(void);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
int is_special_move_available(PlyrPdata* player, int move_id);

float active_sidekick_swap(PlyrPdata* pdata, int mode);
float active_sidekick_swap_change_style(PlyrPdata* pdata);
float active_sidekick_swap_from_behind(PlyrPdata* pdata);
float active_sidekick_swap_from_sky(PlyrPdata* pdata);

extern ScriptSlot* reactions_cmo;

#endif
