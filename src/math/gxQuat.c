#include "math/gxQuat.h"
#include "math/gxMat.h"
#include "math/gxMath.h"

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

union GxQuatFloatBits {
    float f;
    unsigned int u;
};

/* TODO: [near miss] 89.40%; oneMinusT/absDot FPRs and scaled-q2-first order match; retail ranks
 * t f28, sign f29, theta f27 (ours f29/f27/f28); declaration order and a t copy do not move it. */
void gxQuatInterpQuat(Quat* out, const Quat* q1, const Quat* q2, float t) {
    float absDot;
    float oneMinusT;
    float sign;
    float theta;
    float invSin;
    float dot;
    Quat scaled;

    if (t < kZero) {
        t = kZero;
    }
    if (t > kOne) {
        t = kOne;
    }
    oneMinusT = kOne - t;
    dot = q1->w * q2->w + (q1->z * q2->z + (q1->x * q2->x + q1->y * q2->y));
    sign = kOne;
    absDot = dot;
    if (dot < kZero) {
        absDot = -dot;
        sign = kNegOne;
    }
    if (absDot < kSlerpDotThresh) {
        theta = gxMathArcCos(absDot);
        invSin = kOne / gxMathSin(theta);
        t = invSin * gxMathSin(t * theta);
        oneMinusT = invSin * gxMathSin(oneMinusT * theta);
    }
    oneMinusT *= sign;
    scaled.x = q2->x * oneMinusT;
    scaled.y = q2->y * oneMinusT;
    scaled.z = q2->z * oneMinusT;
    scaled.w = q2->w * oneMinusT;
    out->x = t * q1->x + scaled.x;
    out->y = t * q1->y + scaled.y;
    out->z = t * q1->z + scaled.z;
    out->w = t * q1->w + scaled.w;
    if (absDot > kSlerpNormDotThresh) {
        PSQUATNormalize(out, out);
    }
}

static inline float gxQuatFloatFromBits(unsigned int value) {
    union GxQuatFloatBits bits;

    bits.u = value;
    return bits.f;
}

static inline float gxQuatInvSqrt(float value) {
    union GxQuatFloatBits in;
    float result;
    float guess;
    float t1;
    float t3;

    if (value <= kZero) {
        return kZero;
    }
    in.f = value;
    guess = gxQuatFloatFromBits(0x5F375A00U - (in.u >> 1));
    t1 = guess * (value * guess);
    t3 = kNewtonIter3 - t1;
    result = kInvSqrtScale * guess;
    result = result * t3 * (kNewtonIter12 - (t1 * t3 * t3));
    return result;
}

static inline float gxQuatHalfSqrt(float value) {
    union GxQuatFloatBits in, out;
    float guess;

    in.f = value;
    if (value <= kZero) {
        return kZero;
    }
    out.u = GXMathSqrtTable[(in.u >> 11) & 0x1FFF] << 8;
    out.u |= (((in.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    guess = out.f;
    return kHalf * (guess * (kNewtonIter3 - (guess * guess) / value));
}

void gxVectV3V3ToQuat(Quat* out, const Vec* v1, const Vec* v2) {
    Vec axis __attribute__((aligned(16)));
    float dot;
    float scale;
    float wScale;

    dot = PSVECDotProduct(v1, v2);
    if (dot > kV3ToQuatParallelDot) {
        out->x = kZero;
        out->y = kZero;
        out->z = kZero;
        out->w = kOne;
        return;
    }
    if (dot < kV3ToQuatAntiParallelDot) {
        float perpendicular_z = -v1->y;

        axis.y = v1->x;
        axis.x = kZero;
        axis.z = perpendicular_z;
        if (PSVECMag(&axis) < kV3ToQuatAxisEpsilon) {
            axis.z = v1->x;
            axis.x = -v1->z;
            axis.y = kZero;
        }
        scale = gxQuatInvSqrt(PSVECDotProduct(&axis, &axis));
        PSVECScale(&axis, &axis, scale);
        out->x = axis.x;
        out->y = axis.y;
        out->z = axis.z;
        out->w = kZero;
        return;
    }
    PSVECCrossProduct(v1, v2, &axis);
    scale = gxQuatInvSqrt(PSVECDotProduct(&axis, &axis));
    wScale = gxQuatHalfSqrt(kHalf * (kOne - dot));
    PSVECScale(&axis, &axis, scale * wScale);
    out->x = axis.x;
    out->y = axis.y;
    out->z = axis.z;
    out->w = gxQuatHalfSqrt(kHalf * (kOne + dot));
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
    *dst = *src;
}

void gxQuatSetZero(Quat* q) {
    q->x = kZero;
    q->y = kZero;
    q->z = kZero;
    q->w = kOne;
}
