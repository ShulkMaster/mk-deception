#include "libmkparticle/pfxmath.h"
#include "runtime/cstring.h"

void mat_set_identity(PfxMatrix* matrix) {
    int i;
    float* row;

    memset(matrix, 0, sizeof(*matrix));
    row = matrix->elements;
    for (i = 0; i < 4; i++, row += 4) {
        row[i] = 1.0f;
    }
}
