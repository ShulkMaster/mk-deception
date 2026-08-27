#include "libmkparticle/pfxmath.h"
#include "runtime/cstring.h"

void mat_set_identity(PfxMatrix* matrix) {
    int i;
    float (*elements)[4];

    memset(matrix, 0, sizeof(*matrix));
    elements = (float (*)[4])matrix->elements;
    for (i = 0; i < 4; i++) {
        elements[i][i] = 1.0f;
    }
}
