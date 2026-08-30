#ifndef LIBMKPARTICLE_SPAWN_H
#define LIBMKPARTICLE_SPAWN_H

#include "libmkparticle/vm.h"

typedef struct PfxSpawnTable {
    int type;
    int count;
    void* values;
} PfxSpawnTable;

int has_spawncode_for(PfxVmEmitter* emitter, unsigned int field);
void pfx_spawn_box(PfxVec3* output, float x, float y, float z,
                   float width, float height, float depth);
void pfxvm_spawn_set_field_from_table(PfxVmEmitter* emitter,
                                      unsigned int field,
                                      PfxSpawnTable* table);
void pfxvm_spawn_box(PfxVmEmitter* emitter, unsigned int field,
                     float x, float y, float z,
                     float width, float height, float depth);
void pfxvm_spawn_cylinder(PfxVmEmitter* emitter, unsigned int field,
                          const PfxVec3* axis, float radial_center,
                          float radial_spread, float axial_center,
                          float axial_spread);
void pfxvm_spawn_disc(PfxVmEmitter* emitter, unsigned int field,
                      const PfxVec3* axis, float minimum_radius,
                      float maximum_radius);
void pfxvm_spawn_roundrobin_mechanism(PfxVmEmitter* emitter,
                                      unsigned int field, int count);
void pfxvm_spawn_line_1i(PfxVmEmitter* emitter, unsigned int field,
                         int minimum, int maximum);
void pfxvm_spawn_line_1f(PfxVmEmitter* emitter, unsigned int field,
                         float minimum, float maximum);
void pfxvm_spawn_point_color(PfxVmEmitter* emitter, unsigned int field,
                             float red, float green, float blue, float alpha);
void pfxvm_spawn_sphere(PfxVmEmitter* emitter, unsigned int field,
                        float x, float y, float z, float minimum_radius,
                        float maximum_radius, int quadratic_radius);
void pfxvm_spawn_from_pos(PfxVmEmitter* emitter, unsigned int field,
                          unsigned int source_field, int clamp_y,
                          float x, float y, float z, float minimum_length,
                          float length_range, float clamped_y);
void pfxvm_spawn_sphere_section(PfxVmEmitter* emitter, unsigned int field,
                                float x, float y, float z, float radius,
                                float radius_spread, float angle,
                                float angle_spread);
void pfxvm_spawn_value(PfxVmEmitter* emitter, unsigned int field, float value);
void pfxvm_spawn_uv(PfxVmEmitter* emitter, unsigned int field, float u, float v);
void __pfxvm_execute_spawn(PfxVm* pfx, PfxVmEmitter* emitter);
void _pfxvm_execute_spawn(PfxVm* pfx, int emitter_index);
void pfxvm_add_transfer(PfxVm* pfx, PfxEmitterTransfer* transfer);
void pfxvm_create_transfer(PfxVm* destination, PfxVm* source);

#endif
