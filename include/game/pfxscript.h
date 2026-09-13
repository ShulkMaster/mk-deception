#ifndef MKD_GAME_PFXSCRIPT_H
#define MKD_GAME_PFXSCRIPT_H

struct PfxStepEffectDescription;
void create_step_fx(struct PfxStepEffectDescription* effect, char* name);
void create_multiemit_step_fx(struct PfxStepEffectDescription* effect,
                              char* name, int emitter_count);

struct PfxParametricEffectDescription;
void create_multiemit_parametric_fx(struct PfxParametricEffectDescription* effect,
                                    char* name, int emitter_count);
void create_parametric_fx(struct PfxParametricEffectDescription* effect, char* name);

struct MkObj;
void fx_bind_render_to_obj_bone(unsigned int handle, struct MkObj* object, int bone_index);
void fx_bind_emitter_to_obj_bone(unsigned int handle, struct MkObj* object, int bone_index);

unsigned int fx(const char* name);
unsigned int fx_by_owner(const char* name, unsigned int owner);
void fx_reset(unsigned int effect);
void resume_effect(const char* name);

#endif

