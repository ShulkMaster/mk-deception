#ifndef GX_MAT_H
#define GX_MAT_H

#include "math/gxVect.h"

typedef struct Mat33 {
    float col0[3];
    int flags;
    float col1[3];
    float pad;
    float col2[3];
} Mat33;

void gxMat33Tx31(Vec* out, Vec* v, Mat33* m);
void gxMatScaledByV3(Mat33* out, const Mat33* in, const Vec* scale);
void gxMat33x33_Check(Mat33* out, Mat33* a, Mat33* b);
void gxMatV3MatAddV3_Check(Vec* out, Vec* v, Mat33* m, Vec* add);
void gxMatV3MatAddV3(Vec* out, Vec* v, Mat33* m, Vec* add);
void gxMat33x33(Mat33* out, Mat33* a, Mat33* b);

#endif
