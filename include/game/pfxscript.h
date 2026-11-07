#ifndef MKD_GAME_PFXSCRIPT_H
#define MKD_GAME_PFXSCRIPT_H

#include "game/pfxscript_api.h"

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
void fx_reset_emit(unsigned int effect);
void resume_effect(const char* name);
void spawn_random_size(float* table);

void fx_transfer(unsigned int handle, unsigned int owner);
void fx_hide(unsigned int handle, int hidden);
void update_fade_alpha(int object, int alpha, float start, float end);
void change_on_y_less_than_field(int object, int field);
void enable_profiling(int enable);
void emission_duration(float value);
void initial_divert(int object, float x, float y);
void spawn_color(int a, int b, int c, int d, int e);
void update_mul_scalar(int object, float x, float y, float z);
void emit_in_range(int a, float b, float c);
void emit_color(int a, int b, int c, int d, int e);
void change_on_greater(int object, float value);
void texture_animation(float horizontal_scale, int vertical_frames, float speed);
void emit_uv(int a, float b, float c);
void emit_cuboid(int a, float b, float c, float d);
void emit_value_i(int a, int b);
void emit_from_pos(int a, int b, float c, float d, float e, float f, float g);
void kill_on_y_less_than_field(int object, int field);
void initial_multiply_float(int object, float x, float y);
void set_cycle_emission(int value);
void create_step_effect(struct PfxStepEffectDescription* effect);
void set_drag_coefficient(float value);
void update_bounce(int object, int target, int axis, float value);
void set_rotation(float start, float end);
void create_y_mirror_effect(int effect);
unsigned int fx2(unsigned int bank_handle, const char* name);
void set_bounding_radius(float radius);
void initial_reflect(int object);
void set_growth_coefficient(float value);
void emit_roundrobin_mechanism(int a, int b);
void bind_to_bone(int bone);
void kill_on_greater(int object, float value);
void udpate_roundrobin(int value);
void update_texanim_hold(int object, int texture, float value, int first, int last);
void fxsys_set(int parameter, float value);
void emit_constant_rate(void);
void initial_set_float(int object, float x, float y);
void change_on_less(int object, float value);
void update_wrapbox(int object, float x, float y, float z, float w);
void particle_size(float value);
void emit_disc(int a, float b, float c, float d, float e);
void face_y(void);
void emit_value(int a, float b);
void fx_disable_ztest(unsigned int handle, int disabled);
void kill_at_plane(float value);
void update_add_constant_v3(int object, float x, float y, float z);
void set_vertex_color(const struct PfxVertexColorArgs* color);
void update_copy(int object);
void update_assign(int object, int value);
void update_attract(int object, int target, float value);
void set_aspect_ratio(float x, float y);
void emit_spherical(int a, float b);
void initial_add_v3(int object, int value);
void update_add_constant(int object, float value);
void update_texanim(int object, int texture, float value, int first, int last);
void change_on_y_less(int object, float value);
void set_decal_plane(float* plane);
void z_bias(float value);
void emit_disc2(int a, float b, float c, float d, float e, float f);
void kill_percent(float value);
void kill_roundrobin(int value);
void set_cycle_length(float start, float end);
void set_light(struct PfxLightArgs* light);
void emit_spherical_from_boundary(int a, float b);
void update_add(int object, int value);
void emit_from_point(float a, float b, float c);
void reset_effect_ppfx(struct PfxScriptEffect* effect);
void pfxscript_initialize(void);
#endif
