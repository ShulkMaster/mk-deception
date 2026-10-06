#ifndef LIBMKPARTICLE_RANDOM_H
#define LIBMKPARTICLE_RANDOM_H

#include "libmkparticle/vm.h"

#ifdef __cplusplus
extern "C" {
#endif

float rnd_between(float minimum, float maximum);
int rnd_int(unsigned int maximum);
void rnd_line_1i(int minimum, int maximum, int* output);
void rnd_sphere(PfxVec3* output, const PfxVec3* origin,
                float minimum_radius, float maximum_radius, int quadratic_radius);
void rnd_point_in_cylinder(PfxVec3* output, const PfxVec3* axis,
                           float radial_center, float radial_spread,
                           float axial_center, float axial_spread);
void rnd_point_in_disc(PfxVec3* output, const PfxVec3* axis,
                       float minimum_radius, float maximum_radius);
void rnd_point_in_sphere_section(PfxVec3* output, const PfxVec3* axis,
                                 float radius, float radius_spread,
                                 float angle, float angle_spread);
void rnd_vector_from_point(PfxVec3* output, const PfxVec3* start,
                           const PfxVec3* end, float minimum_length,
                           float length_range);
void rnd_bend_vector(PfxVec3* vector, float angle, float angle_spread);

#ifdef __cplusplus
}
#endif

#endif
