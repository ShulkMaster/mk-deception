#ifndef RUNTIME_SOUND_BANK_H
#define RUNTIME_SOUND_BANK_H

#ifdef __cplusplus
extern "C" {
#endif

void fxbanks_unload_by_owner(unsigned int owner_flags);
void unload_all_effect_banks(void);

#ifdef __cplusplus
}
#endif

#endif
