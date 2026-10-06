#ifndef RUNTIME_SOUND_H
#define RUNTIME_SOUND_H

#include "msl/msl_types.h"

#ifdef __cplusplus
extern "C" {
#endif
extern _mslSystem* msi;
#ifdef __cplusplus
}
#endif

MslSoundHandle snd_req(int sound_id);
MslSoundHandle snd_req_vol(int sound_id, float volume);
MslSoundHandle random_snd_req(int group);
MslSoundHandle pan_vol_pitch_random_snd_req(int group, float pan, float volume, float pitch);
void set_snd_vol(MslSoundHandle handle, int sound_id, float volume);
int init_sounds(void);
void setup_sound_banks(int load_mode);
void wait_for_sound_banks_to_load(void);
void start_tunes(void);
void pause_all_game_sounds(void);
void unpause_all_game_sounds(void);
float snd_get_game_vol(void);
void snd_set_game_vol(float volume);

#endif
