#ifndef LIBMKPARTICLE_UPDATE_H
#define LIBMKPARTICLE_UPDATE_H

#include "libmkparticle/behavior.h"

extern PfxVm* g_current_effect;

void pfxvm_execute_behavior_update(PfxBehavior* behavior, float frame_time);
void set_vm_field(PfxVmField* field, unsigned int description);
void pfxvm_update_age(PfxBehavior* behavior, unsigned int field);
void pfxvm_update_add(PfxBehavior* behavior, unsigned int destination,
                      unsigned int source);
void pfxvm_update_mul_scalar(PfxBehavior* behavior, unsigned int field,
                             float x, float y, float z);
void pfxvm_update_add_constant_v3(PfxBehavior* behavior, unsigned int field,
                                  float x, float y, float z);
void pfxvm_update_add_constant(PfxBehavior* behavior, unsigned int field,
                               float value);
void pfxvm_update_wrapbox(PfxBehavior* behavior, unsigned int field,
                          float scale, float x, float y, float z);
void pfxvm_update_copy(PfxBehavior* behavior, unsigned int field);
void pfxvm_update_bounce(PfxBehavior* behavior, unsigned int field,
                         unsigned int velocity_field,
                         unsigned int bounce_count_field, float scale);
void pfxvm_update_fade_alpha(PfxBehavior* behavior, unsigned int color_field,
                             unsigned int age_field, int start_alpha,
                             int end_alpha, float start_time, float duration);
void pfxvm_update_lerp_color(PfxBehavior* behavior, unsigned int color_field,
                             unsigned int age_field, int color_count,
                             int first_color, void* colors, float duration);
void pfxvm_update_animate_texture(PfxBehavior* behavior,
                                  unsigned int texture_field,
                                  unsigned int age_field, int frame_count,
                                  int frame_offset, void* frame_source,
                                  int mode, float frame_time);
void pfxvm_update_attract(PfxBehavior* behavior, unsigned int field,
                          unsigned int target_field, float strength);
void pfxvm_update_assign(PfxBehavior* behavior, int destination, int source);
void pfxvm_update_roundrobin(PfxBehavior* behavior, unsigned int field);

#endif
