#include "math/gxVect.h"
#include "math/gxMath.h"

static const float _235 = 0.0f;

float gxVectAngleZX(const Vec* v) {
    float lenSq;
    float invLen;
    float angle;
    float z;
    float x;
    float x2;
    float zz;

    x = v->x;
    z = v->z;
    x2 = x * x;
    zz = z * z;
    lenSq = x2 + zz;
    invLen = gxMathFastInvSqrt(lenSq);
    angle = gxMathArcCos(z * invLen);
    if (v->x < _235) {
        angle = -angle;
    }
    return angle;
}

void gxVectUVV3ToV3(Vec* v, const Vec* u, const Vec* w) {
    PSVECSubtract(w, u, v);
    PSVECNormalize(v, v);
}
