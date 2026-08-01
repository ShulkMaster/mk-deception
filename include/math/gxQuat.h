#ifndef GX_QUAT_H
#define GX_QUAT_H

#include "math/gxVect.h"

typedef struct Quat {
    float x;
    float y;
    float z;
    float w;
} Quat;

typedef struct Mat33 Mat33;

typedef float Mtx[3][4];

void PSMTXQuat(Mtx m, const Quat* q);
void PSQUATMultiply(const Quat* p, const Quat* q, Quat* dst);
void PSQUATNormalize(const Quat* src, Quat* dst);

void gxQuatInterpQuat(Quat* out, const Quat* q1, const Quat* q2, float t);
void gxVectV3V3ToQuat(Quat* out, const Vec* v1, const Vec* v2);
void gxQuatQuatToMat(Mat33* out, const Quat* q);
void gxQuatNorm(Quat* q);
void gxQuatMul(Quat* out, const Quat* a, const Quat* b);
void gxQuatCopy(Quat* dst, const Quat* src);
void gxQuatSetZero(Quat* q);

#endif
