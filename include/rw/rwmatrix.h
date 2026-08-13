#ifndef RW_RWMATRIX_H
#define RW_RWMATRIX_H

#include "rw/rtquat.h"

typedef struct RwMatrixTolerance {
    float Normal;
    float Orthogonal;
    float Identity;
} RwMatrixTolerance;

void* _rwMatrixOpen(void* instance, int offset, int size);
void* _rwMatrixClose(void* instance, int offset, int size);
int RwEngineSetMatrixTolerances(const RwMatrixTolerance* tolerance);
RwMatrix* RwMatrixOptimize(RwMatrix* matrix,
                           const RwMatrixTolerance* tolerance);
float _rwMatrixDeterminant(const RwMatrix* matrix);
float _rwMatrixOrthogonalError(const RwMatrix* matrix);
float _rwMatrixNormalError(const RwMatrix* matrix);

#endif
