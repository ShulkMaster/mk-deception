#ifndef GX_MAT_H
#define GX_MAT_H

#include "math/gxVect.h"

typedef struct Mat33 {
    union {
        Vec col0_vec;
        float col0[3];
    };
    union {
        int flags;
        float flags_pad;
    };
    union {
        Vec col1_vec;
        float col1[3];
    };
    float pad1;
    union {
        Vec col2_vec;
        float col2[3];
    };
    float pad2;
} Mat33;

void gxMat33Tx31(Vec* out, const Vec* v, const Mat33* m);
void gxMatScaledByV3(Mat33* out, const Mat33* in, const Vec* scale);
void gxMat33x33_Check(Mat33* out, const Mat33* a, const Mat33* b);
void gxMatV3MatAddV3_Check(Vec* out, const Vec* v, const Mat33* m, const Vec* add);
void gxMatV3MatAddV3(Vec* out, const Vec* v, const Mat33* m, const Vec* add);
void gxMat33x33(Mat33* out, const Mat33* a, const Mat33* b);

#endif
