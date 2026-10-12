#ifndef LIBMKPARTICLE_PFXMATH_H
#define LIBMKPARTICLE_PFXMATH_H

typedef struct PfxMatrix {
    float elements[4][4];
} PfxMatrix;

void mat_set_identity(PfxMatrix* matrix);

#endif
