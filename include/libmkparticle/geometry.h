#ifndef LIBMKPARTICLE_GEOMETRY_H
#define LIBMKPARTICLE_GEOMETRY_H

typedef struct PfxVec3 PfxVec3;
typedef struct PfxVm PfxVm;

void pfx_get_billboard_vector(PfxVm* vm, PfxVec3* axis0,
                              PfxVec3* axis1);

#endif
