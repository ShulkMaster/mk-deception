#include "math/gxVect.h"
#include "math/gxMath.h"

static const float _235 = 0.0f;
static const float _464 = 3.0f;
static const float _465 = 0.0625f;
static const float _466 = 12.0f;

/* Soft ceiling: gxVectAngleZX ~77% -- zero-const load vs v load interleave;
 * sdata2 label names. UVV3ToV3 is 100%. */

float gxVectAngleZX(const Vec* v) {
    union {
        float f;
        unsigned int u;
    } pun;
    float lenSq;
    float invLen;
    float angle;
    unsigned int guessBits;
    float guess;
    float z;
    float x;
    float x2;
    float t1;
    float t3;

    x = v->x;
    z = v->z;
    x2 = x * x;
    lenSq = x2 + (z * z);
    invLen = _235;
    if (lenSq > invLen) {
        /* Fast inverse sqrt of (x*x + z*z) with one Newton step. */
        pun.f = lenSq;
        guessBits = 0x5F375A00U - (pun.u >> 1);
        guess = *(float*)&guessBits;
        lenSq = lenSq * guess;
        t1 = guess * lenSq;
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
