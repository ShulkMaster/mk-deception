#ifndef RW_RTQUAT_H
#define RW_RTQUAT_H

#include "math/gxVect.h"
#include "rw/rwplcore.h"

/* RtQuat + RwMatrix layouts from BFBB include/rwsdk (PS2 MKD = API only). */

typedef struct RwMatrixPosition {
    Vec value;
    RwUInt32 pad;
} RwMatrixPosition;

typedef struct RwMatrix {
    RwV3d right;
    RwUInt32 flags;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    union {
        struct {
            RwV3d pos;
            RwUInt32 pad3;
        };
        struct {
            Vec pos_vec;
            RwUInt32 pos_vec_pad;
        };
        RwMatrixPosition pos_row;
    };
} RwMatrix;

typedef struct RtQuat {
    RwV3d imag;
    RwReal real;
} RtQuat;

RwReal _rwSqrt(RwReal num);

RwBool RtQuatConvertFromMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix);

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, int combineOp);
RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, int combineOp);
RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, int combineOp);

#endif
