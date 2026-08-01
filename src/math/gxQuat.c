#include "math/gxQuat.h"
#include "math/gxMat.h"
#include "math/gxMath.h"

/*
 * Soft ceilings (NonMatching -- do not grind Matching on these):
 *   gxQuatInterpQuat ~78% -- slerp weight / FP schedule leftovers; stop.
 *   gxVectV3V3ToQuat ~77.7% -- invsqrt + table-sqrt schedule; stop.
 */

static const float kZero = 0.0f;
static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kSlerpDotThresh = 0.999f;
static const float kSlerpNormDotThresh = 1.001f;
static const float kV3ToQuatParallelDot = 0.9999f;
static const float kV3ToQuatAntiParallelDot = -0.9999f;
static const float kV3ToQuatAxisEpsilon = 0.001f;
static const float kNewtonIter3 = 3.0f;
static const float kInvSqrtScale = 0.0625f;
static const float kNewtonIter12 = 12.0f;
static const float kHalf = 0.5f;

void gxQuatInterpQuat(Quat* out, const Quat* q1, const Quat* q2, float t) {
    float tf;
    float sign;
    float oneMinusT;
    float dot;
    float invSin;
    float theta;

    tf = t;
    if (tf < kZero) {
        tf = kZero;
    }
    if (kOne < tf) {
        tf = kOne;
    }
    oneMinusT = kOne - tf;
    sign = kOne;
    dot = q1->x * q2->x + q1->y * q2->y + q1->z * q2->z + q1->w * q2->w;
    if (dot < kZero) {
        dot = -dot;
        sign = kNegOne;
    }
    if (dot < kSlerpDotThresh) {
        theta = gxMathArcCos(dot);
        invSin = kOne / gxMathSin(theta);
        tf = invSin * gxMathSin(tf * theta);
        oneMinusT = invSin * gxMathSin(oneMinusT * theta);
    }
    oneMinusT *= sign;
    /* Midway: out = sin(t*theta)/sin(theta)*q1 + sin((1-t)*theta)/sin(theta)*sign*q2.
     * Usual libs weight q1 with (1-t) and q2 with t; keep retail order. */
    out->x = tf * q1->x + oneMinusT * q2->x;
    out->y = tf * q1->y + oneMinusT * q2->y;
    out->z = tf * q1->z + oneMinusT * q2->z;
    out->w = tf * q1->w + oneMinusT * q2->w;
    if (kSlerpNormDotThresh < dot) {
        PSQUATNormalize(out, out);
    }
}

void gxVectV3V3ToQuat(Quat* out, const Vec* v1, const Vec* v2) {
    union {
        float f;
        unsigned int u;
    } pun;
    Vec axis;
    Vec scratch;
    float dot;
    float scale;
    float wScale;
    float wValue;
    float invLenSq;
    float t1;
    float t3;
    unsigned int guessBits;
    float guess;
    float crossDot;
    float sqrtGuess;
    float halfAngle;
    float wSqrtArg;

    dot = PSVECDotProduct(v1, v2);
    if (kV3ToQuatParallelDot < dot) {
        out->x = kZero;
        out->y = kZero;
        out->z = kZero;
        out->w = kOne;
        return;
    }
    if (dot < kV3ToQuatAntiParallelDot) {
        /* 180 deg: pick a perpendicular axis and normalize. */
        scratch.x = kZero;
        scratch.y = v1->x;
        scratch.z = -v1->y;
        if (PSVECMag(&scratch) < kV3ToQuatAxisEpsilon) {
            scratch.x = -v1->z;
            scratch.y = kZero;
            scratch.z = v1->x;
        }
        invLenSq = PSVECDotProduct(&scratch, &scratch);
        scale = kZero;
        if (kZero < invLenSq) {
            pun.f = invLenSq;
            guessBits = 0x5F375A00U - (pun.u >> 1);
            pun.u = guessBits;
            guess = pun.f;
            t1 = guess * invLenSq;
            t3 = kNewtonIter3 - t1;
            scale = kInvSqrtScale * guess;
            scale = scale * t3 * (kNewtonIter12 - (t1 * t3 * t3));
        }
        PSVECScale(&scratch, &scratch, scale);
        out->x = scratch.x;
        out->y = scratch.y;
        out->z = scratch.z;
        out->w = kZero;
        return;
    }
    PSVECCrossProduct(v1, v2, &axis);
    crossDot = PSVECDotProduct(&axis, &axis);
    scale = kZero;
    if (kZero < crossDot) {
        pun.f = crossDot;
        guessBits = 0x5F375A00U - (pun.u >> 1);
        pun.u = guessBits;
        guess = pun.f;
        t1 = guess * crossDot;
        t3 = kNewtonIter3 - t1;
        scale = kInvSqrtScale * guess;
        scale = scale * t3 * (kNewtonIter12 - (t1 * t3 * t3));
    }
    halfAngle = kHalf * (kOne - dot);
    wScale = kZero;
    if (kZero < halfAngle) {
        pun.f = halfAngle;
        pun.u = (unsigned int)GXMathSqrtTable[(pun.u >> 10) & 0x3FFE] << 8 |
                ((((pun.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U);
        sqrtGuess = pun.f;
        wScale = kHalf * sqrtGuess * (kNewtonIter3 - (sqrtGuess * sqrtGuess) / halfAngle);
    }
    PSVECScale(&axis, &axis, scale * wScale);
    out->x = axis.x;
    out->y = axis.y;
    out->z = axis.z;
    wSqrtArg = kHalf * (kOne + dot);
    wValue = kZero;
    if (kZero < wSqrtArg) {
        pun.f = wSqrtArg;
        pun.u = (unsigned int)GXMathSqrtTable[(pun.u >> 10) & 0x3FFE] << 8 |
                ((((pun.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U);
        sqrtGuess = pun.f;
        wValue = kHalf * sqrtGuess * (kNewtonIter3 - (sqrtGuess * sqrtGuess) / wSqrtArg);
    }
    out->w = wValue;
}

void gxQuatQuatToMat(Mat33* out, const Quat* q) {
    Mtx m;
    int three;

    PSMTXQuat(m, q);
    out->col0[0] = m[0][0];
    out->col0[1] = m[1][0];
    out->col0[2] = m[2][0];
    out->flags_pad = kZero;
    out->col1[0] = m[0][1];
    out->col1[1] = m[1][1];
    out->col1[2] = m[2][1];
    out->pad1 = kZero;
    out->col2[0] = m[0][2];
    out->col2[1] = m[1][2];
    out->col2[2] = m[2][2];
    out->pad2 = kZero;
    three = 3;
    out->flags = three;
}

void gxQuatNorm(Quat* q) {
    PSQUATNormalize(q, q);
}

void gxQuatMul(Quat* out, const Quat* a, const Quat* b) {
    PSQUATMultiply(a, b, out);
}

void gxQuatCopy(Quat* dst, const Quat* src) {
    /* Word-wise copy matches retail int loads/stores. */
    *dst = *src;
}

void gxQuatSetZero(Quat* q) {
    q->x = kZero;
    q->y = kZero;
    q->z = kZero;
    q->w = kOne;
}
