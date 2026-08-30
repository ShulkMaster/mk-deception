#include "math/gxVect.h"
#include "math/gxMath.h"

static const float _235 = 0.0f;
static const float _464 = 3.0f;
static const float _465 = 0.0625f;
static const float _466 = 12.0f;

float gxVectAngleZX(const Vec* v) {
    union {
        float f;
        unsigned int u;
    } input, estimate;
    float lenSq;
    float invLen;
    float angle;
    unsigned int guessBits;
    float guess;
    float z;
    float x;
    float x2;
    /* Integer bits are written through estimate; read its float view here. */
    float* estimateAsFloat = &estimate.f;
    float zz;
    float t1;
    float t3;

    x = v->x;
    z = v->z;
    x2 = x * x;
    zz = z * z;
    lenSq = x2 + zz;
    if (lenSq <= _235) {
        invLen = _235;
    } else {
        /* Fast inverse sqrt of (x*x + z*z) with one Newton step. */
        input.f = lenSq;
        guessBits = 0x5F375A00U - (input.u >> 1);
        estimate.u = guessBits;
        guess = *estimateAsFloat;
        lenSq = lenSq * guess;
        invLen = guess * lenSq;
        t1 = invLen;
        t3 = _464 - t1;
        invLen = _465 * guess;
        invLen = invLen * t3 * (_466 - (t1 * t3 * t3));
    }
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
