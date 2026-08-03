#ifndef RUNTIME_SOUND_H
#define RUNTIME_SOUND_H

void snd_req(int sound_id);
void set_snd_vol(int handle, int sound_id, float volume);
void init_sounds(void);
void setup_sound_banks(int load_mode);
void pause_all_game_sounds(void);
void unpause_all_game_sounds(void);
float snd_get_game_vol(void);
void snd_set_game_vol(float volume);

#endif
