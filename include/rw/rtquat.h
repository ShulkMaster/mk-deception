#ifndef RW_RTQUAT_H
#define RW_RTQUAT_H

#include "math/gxVect.h"
#include "rw/rwplcore.h"

typedef struct RwMatrixPosition {
    Vec value;
    unsigned int pad;
} RwMatrixPosition;

typedef struct RwMatrix {
    RwV3d right;
    unsigned int flags;
    RwV3d up;
    unsigned int pad1;
    RwV3d at;
    unsigned int pad2;
    union {
        struct {
            RwV3d pos;
            unsigned int pad3;
        };
        struct {
            Vec pos_vec;
            unsigned int pos_vec_pad;
        };
        RwMatrixPosition pos_row;
    };
} RwMatrix;

typedef struct RtQuat {
    RwV3d imag;
    float real;
} RtQuat;

typedef int RwOpCombineType;

float _rwSqrt(float num);

int RtQuatConvertFromMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix);

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation,
                            RwOpCombineType combineOp);
RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale,
                        RwOpCombineType combineOp);
RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, float angle,
                         RwOpCombineType combineOp);
RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform,
                            RwOpCombineType combineOp);
RwMatrix* RwMatrixOrthoNormalize(RwMatrix* matrixOut,
                                 const RwMatrix* matrixIn);
void RwMatrixUpdate(RwMatrix* matrix);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1,
                           const RwMatrix* matrixIn2);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);

#endif
