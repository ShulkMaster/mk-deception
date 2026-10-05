#ifndef MKD_GAME_PFXSCRIPT_API_H
#define MKD_GAME_PFXSCRIPT_API_H

#ifdef __cplusplus
extern "C" {
#endif

void fx_set(unsigned int handle, int field, float value);
unsigned int fx_next_emitter(unsigned int handle);
int emitter_id_from_handle(unsigned int handle);
void fx_restart_emit(unsigned int handle);
void fx_set_param_v3(unsigned int handle, int parameter,
                     float x, float y, float z);

#ifdef __cplusplus
}
#endif

#endif
