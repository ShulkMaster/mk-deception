#include "math/gxMath.h"

/*
 * Soft ceiling: Wave D NonMatching -- sdata2 first-use pool / FP schedule.
 * GXMathSqrtTable (.data 0x4000) remains on split ASM until Matching.
 * Soft ceiling: gxMathCosSin ~70% -- dual-poly FP interleave vs retail; stop.
 * Soft ceiling: gxMathSin ~97% -- f5 vs f1 after int-float + sdata2 relocs; stop.
 * Soft ceiling: gxMathCos ~99% -- instruction-identical; sdata2 reloc leftover; stop.
 * Soft ceiling: gxMathArcTanYX ~94% / ArcTan ~91% -- atan poly reg/load schedule; stop.
 */

/* Angle scale: 2^20 / (2*pi) and reciprocal 2*pi / 2^20 */
static const float kAngleToIndex = 166886.05f;
static const float kIndexToRad = 0.0000059921126f;
static const float kNegIndexToRad = -0.0000059921126f;

static const float kZero = 0.0f;
static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kHalfPi = 1.5707964f;
static const float kPi = 3.1415927f;
static const float kNegHalfPi = -1.5707964f;

/* sin Taylor extras */
static const float kSinC6 = -0.00019176333f;
static const float kSinC4 = 0.008333334f;
static const float kSinC2 = -0.16666667f;

/* cos Taylor extras */
static const float kCosC6 = -0.0013293402f;
static const float kCosC4 = 0.041666668f;
static const float kCosC2 = -0.5f;

/* atan series odd-reciprocal coeffs (1/3 .. 1/27) */
static const float kAtan3 = 0.33333334f;
static const float kAtan5 = 0.2f;
static const float kAtan7 = 0.14285715f;
static const float kAtan9 = 0.11111111f;
static const float kAtan11 = 0.09090909f;
static const float kAtan13 = 0.07692308f;
static const float kAtan15 = 0.06666667f;
static const float kAtan17 = 0.05882353f;
static const float kAtan19 = 0.05263158f;
static const float kAtan21 = 0.04761905f;
static const float kAtan23 = 0.04347826f;
static const float kAtan25 = 0.04f;
static const float kAtan27 = 0.037037037f;

/* acos piecewise thresholds / linear fits / mid poly */
static const float kAcosLo0 = -0.825f;
static const float kAcosLo1 = -0.911f;
static const float kAcosLo2 = -0.95f;
static const float kAcosLo3 = -0.986f;
static const float kAcosHi0 = 0.825f;
static const float kAcosHi1 = 0.911f;
static const float kAcosHi2 = 0.95f;
static const float kAcosHi3 = 0.986f;

static const float kAcosMidC6 = -0.017352764f;
static const float kAcosMidC5 = -0.022372158f;
static const float kAcosMidC4 = -0.030381944f;
static const float kAcosMidC3 = -0.04464286f;
static const float kAcosMidC2 = -0.075f;

static const float kAcosLinA0 = 0.8574234f;
static const float kAcosLinB0 = -2.0406988f;
static const float kAcosLinA1 = 0.20541239f;
static const float kAcosLinB1 = -2.756408f;
static const float kAcosLinA2 = -1.1369767f;
static const float kAcosLinB2 = -4.1694493f;
static const float kAcosLinA3 = -8.822167f;
static const float kAcosLinB3 = -11.96376f;

static const float kAcosLinA4 = 2.284175f;
static const float kAcosLinB4 = -2.040697f;
static const float kAcosLinA5 = 2.9361913f;
static const float kAcosLinB5 = -2.7564118f;
static const float kAcosLinA6 = 4.278571f;
static const float kAcosLinB6 = -4.169443f;
static const float kAcosLinA7 = 11.964287f;
static const float kAcosLinB7 = -11.964287f;

float gxMathTan(float angle) {
    float cosV;
    float sinV;

    gxMathCosSin(&cosV, &sinV, angle);
    return sinV / cosV;
}

float gxMathSin(float angle) {
    unsigned int bits;
    int folded;
    float scale;
    float x;
    float x2;
    float t;

    bits = (unsigned int)(int)(angle * kAngleToIndex);
    folded = (int)(bits & 0x3FFFFu);
    if ((bits & 0x40000u) != 0) {
        folded = 0x40000 - folded;
    }
    if ((bits & 0x80000u) != 0) {
        scale = kNegIndexToRad;
    } else {
        scale = kIndexToRad;
    }
    x = (float)folded * scale;
    x2 = x * x;
    /* Horner: x * (x2 * ((c6*x2+c4)*x2+c2) + 1) -- fmadds operand order */
    t = kSinC6 * x2 + kSinC4;
    t = t * x2 + kSinC2;
    t = x2 * t + kOne;
    return x * t;
}

void gxMathCosSin(float* cosOut, float* sinOut, float angle) {
    unsigned int bits;
    int folded;
    float scale;
    float x;
    float x2;
    float sinV;
    float cosV;

    bits = (unsigned int)(int)(angle * kAngleToIndex);
    folded = (int)(bits & 0x3FFFFu);
    if ((bits & 0x40000u) != 0) {
        folded = 0x40000 - folded;
    }
    if ((bits & 0x80000u) != 0) {
        scale = kNegIndexToRad;
    } else {
        scale = kIndexToRad;
    }
    x = (float)folded * scale;
    x2 = x * x;
    sinV = x * (kOne + x2 * (kSinC2 + x2 * (kSinC4 + x2 * kSinC6)));
    cosV = kOne + x2 * (kCosC2 + x2 * (kCosC4 + x2 * kCosC6));
    if ((((bits + bits) ^ bits) & 0x80000u) != 0) {
        cosV = -cosV;
    }
    *sinOut = sinV;
    *cosOut = cosV;
}

float gxMathCos(float angle) {
    unsigned int bits;
    int folded;
    float x;
    float x2;
    float t;

    bits = (unsigned int)(int)(angle * kAngleToIndex);
    folded = (int)(bits & 0x3FFFFu);
    if ((bits & 0x40000u) != 0) {
        bits ^= 0x80000u;
        folded = 0x40000 - folded;
    }
    x = (float)folded * kIndexToRad;
    x2 = x * x;
    t = kCosC6 * x2 + kCosC4;
    t = t * x2 + kCosC2;
    t = x2 * t + kOne;
    if ((bits & 0x80000u) != 0) {
        t = -t;
    }
    return t;
}

float gxMathArcTanYX(float y, float x) {
    float ratio;
    float result;
    float x2;
    float t3;
    float t5;
    float t7;
    float t9;
    float t11;
    float t13;
    float t15;
    float t17;
    float t19;
    float t21;
    float t23;
    float t25;
    float t27;
    float inv;

    if (y == kZero) {
        if (x >= kZero) {
            return kZero;
        }
        return kPi;
    }
    if (x == kZero) {
        if (y >= kZero) {
            return kHalfPi;
        }
        return kNegHalfPi;
    }

    ratio = y / x;
    if (ratio <= kOne && ratio >= kNegOne) {
        x2 = ratio * ratio;
        t3 = x2 * ratio;
        t5 = t3 * x2;
        t7 = t5 * x2;
        t9 = t7 * x2;
        t11 = t9 * x2;
        t13 = t11 * x2;
        t15 = t13 * x2;
        t17 = t15 * x2;
        t19 = t17 * x2;
        t21 = t19 * x2;
        t23 = t21 * x2;
        t25 = t23 * x2;
        t27 = t25 * x2;
        result = ratio - kAtan3 * t3;
        result = result + kAtan5 * t5;
        result = result - kAtan7 * t7;
        result = result + kAtan9 * t9;
        result = result - kAtan11 * t11;
        result = result + kAtan13 * t13;
        result = result - kAtan15 * t15;
        result = result + kAtan17 * t17;
        result = result - kAtan19 * t19;
        result = result + kAtan21 * t21;
        result = result - kAtan23 * t23;
        result = result + kAtan25 * t25;
        result = result - kAtan27 * t27;
    } else {
        inv = kOne / ratio;
        x2 = inv * inv;
        t3 = x2 * inv;
        t5 = t3 * x2;
        t7 = t5 * x2;
        t9 = t7 * x2;
        t11 = t9 * x2;
        t13 = t11 * x2;
        t15 = t13 * x2;
        result = -inv + kAtan3 * t3;
        result = result - kAtan5 * t5;
        result = result + kAtan7 * t7;
        result = result - kAtan9 * t9;
        result = result + kAtan11 * t11;
        result = result - kAtan13 * t13;
        result = result + kAtan15 * t15;
        if (ratio > kOne) {
            result = result + kHalfPi;
        } else {
            result = result - kHalfPi;
        }
    }

    if (x < kZero) {
        if (y >= kZero) {
            result = result + kPi;
        } else {
            result = result - kPi;
        }
    }
    return result;
}

float gxMathArcTan(float x) {
    float result;
    float x2;
    float t3;
    float t5;
    float t7;
    float t9;
    float t11;
    float t13;
    float t15;
    float t17;
    float t19;
    float t21;
    float t23;
    float t25;
    float t27;
    float inv;

    if (x <= kOne && x >= kNegOne) {
        if (x == kZero) {
            return kZero;
        }
        x2 = x * x;
        t3 = x2 * x;
        t5 = t3 * x2;
        t7 = t5 * x2;
        t9 = t7 * x2;
        t11 = t9 * x2;
        t13 = t11 * x2;
        t15 = t13 * x2;
        t17 = t15 * x2;
        t19 = t17 * x2;
        t21 = t19 * x2;
        t23 = t21 * x2;
        t25 = t23 * x2;
        t27 = t25 * x2;
        result = x - kAtan3 * t3;
        result = result + kAtan5 * t5;
        result = result - kAtan7 * t7;
        result = result + kAtan9 * t9;
        result = result - kAtan11 * t11;
        result = result + kAtan13 * t13;
        result = result - kAtan15 * t15;
        result = result + kAtan17 * t17;
        result = result - kAtan19 * t19;
        result = result + kAtan21 * t21;
        result = result - kAtan23 * t23;
        result = result + kAtan25 * t25;
        result = result - kAtan27 * t27;
        return result;
    }

    inv = kOne / x;
    x2 = inv * inv;
    t3 = x2 * inv;
    t5 = t3 * x2;
    t7 = t5 * x2;
    t9 = t7 * x2;
    t11 = t9 * x2;
    t13 = t11 * x2;
    t15 = t13 * x2;
    result = -inv + kAtan3 * t3;
    result = result - kAtan5 * t5;
    result = result + kAtan7 * t7;
    result = result - kAtan9 * t9;
    result = result + kAtan11 * t11;
    result = result - kAtan13 * t13;
    result = result + kAtan15 * t15;
    if (x > kOne) {
        return result + kHalfPi;
    }
    return result - kHalfPi;
}

float gxMathArcCos(float x) {
    float x2;
    float p;
    float t;

    if (x < kAcosLo0) {
        if (x >= kAcosLo1) {
            t = kAcosLinB0 * x;
            return kAcosLinA0 + t;
        }
        if (x >= kAcosLo2) {
            t = kAcosLinB1 * x;
            return kAcosLinA1 + t;
        }
        if (x >= kAcosLo3) {
            t = kAcosLinB2 * x;
            return kAcosLinA2 + t;
        }
        if (x > kNegOne) {
            t = kAcosLinB3 * x;
            return kAcosLinA3 + t;
        }
        return kPi;
    }

    if (x > kAcosHi0) {
        if (x <= kAcosHi1) {
            t = kAcosLinB4 * x;
            return kAcosLinA4 + t;
        }
        if (x <= kAcosHi2) {
            t = kAcosLinB5 * x;
            return kAcosLinA5 + t;
        }
        if (x <= kAcosHi3) {
            t = kAcosLinB6 * x;
            return kAcosLinA6 + t;
        }
        if (x < kOne) {
            t = kAcosLinB7 * x;
            return kAcosLinA7 + t;
        }
        return kZero;
    }

    /* |x| <= 0.825: pi/2 + x * P(x^2). fmadds order: C*x2, p*x2, then x2*p... */
    x2 = x * x;
    p = kAcosMidC6 * x2 + kAcosMidC5;
    p = p * x2 + kAcosMidC4;
    p = x2 * p + kAcosMidC3;
    p = x2 * p + kAcosMidC2;
    p = x2 * p + kSinC2;
    p = x2 * p + kNegOne;
    return x * p + kHalfPi;
}
