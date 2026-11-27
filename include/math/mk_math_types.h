#ifndef MK_MATH_TYPES_H
#define MK_MATH_TYPES_H

#include "rw/rtquat.h"

typedef RwMatrix MKMATRIX __attribute__((aligned(16)));

struct _MKV3D {
    float x;
    float y;
    float z;
};

#endif
