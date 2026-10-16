#include "math/gxVect.h"
#include "math/gxMath.h"

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
    if (v->x < 0.0f) {
        angle = -angle;
    }
    return angle;
}

void gxVectUVV3ToV3(Vec* v, const Vec* u, const Vec* w) {
    PSVECSubtract(w, u, v);
    PSVECNormalize(v, v);
}
