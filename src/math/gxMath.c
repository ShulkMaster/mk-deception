/* TODO: [review] layout stubs: gxMathTan2, gxMathSin2, gxMathCosSin2, gxMathCos2, gxMathArcSin and
 * sinCosTable reproduce linker-stripped retail functions/data only to recreate their pool/data
 * layout; the bodies are NOT recovered source. Replace with genuine bodies if a source turns up. */
#include "math/gxMath.h"

#define kAngleToIndex 166886.05f
#define kIndexToRad 0.0000059921126f
#define kNegIndexToRad -0.0000059921126f

#define kZero 0.0f
#define kOne 1.0f
#define kNegOne -1.0f
#define kHalfPi 1.5707964f
#define kPi 3.1415927f
#define kNegHalfPi -1.5707964f

#define kSinC6 -0.00019176333f
#define kSinC4 0.008333334f
#define kSinC2 -0.16666667f

#define kCosC6 -0.0013293402f
#define kCosC4 0.041666668f
#define kCosC2 -0.5f

#define kAtan3 0.33333334f
#define kAtan5 0.2f
#define kAtan7 0.14285715f
#define kAtan9 0.11111111f
#define kAtan11 0.09090909f
#define kAtan13 0.07692308f
#define kAtan15 0.06666667f
#define kAtan17 0.05882353f
#define kAtan19 0.05263158f
#define kAtan21 0.04761905f
#define kAtan23 0.04347826f
#define kAtan25 0.04f
#define kAtan27 0.037037037f

#define kAcosLo0 -0.825f
#define kAcosLo1 -0.911f
#define kAcosLo2 -0.95f
#define kAcosLo3 -0.986f
#define kAcosHi0 0.825f
#define kAcosHi1 0.911f
#define kAcosHi2 0.95f
#define kAcosHi3 0.986f

#define kAcosMidC6 -0.017352764f
#define kAcosMidC5 -0.022372158f
#define kAcosMidC4 -0.030381944f
#define kAcosMidC3 -0.04464286f
#define kAcosMidC2 -0.075f

#define kAcosLinA0 0.8574234f
#define kAcosLinB0 -2.0406988f
#define kAcosLinA1 0.20541239f
#define kAcosLinB1 -2.756408f
#define kAcosLinA2 -1.1369767f
#define kAcosLinB2 -4.1694493f
#define kAcosLinA3 -8.822167f
#define kAcosLinB3 -11.96376f

#define kAcosLinA4 2.284175f
#define kAcosLinB4 -2.040697f
#define kAcosLinA5 2.9361913f
#define kAcosLinB5 -2.7564118f
#define kAcosLinA6 4.278571f
#define kAcosLinB6 -4.169443f
#define kAcosLinA7 11.964287f
#define kAcosLinB7 -11.964287f

float gxMathTan2(float angle) {
    float x;
    float x2;
    float sinV;
    float cosV;

    if (angle == kZero) {
        return kZero;
    }
    x = angle;
    x2 = x * x;
    cosV = kOne;
    cosV = cosV + kCosC2 * x2;
    cosV = cosV + kCosC4 * x2 * x2;
    sinV = x;
    sinV = sinV + kSinC2 * x2 * x;
    sinV = sinV + kSinC4 * x2 * x2 * x;
    if (x > kHalfPi) {
        sinV = -sinV;
    }
    if (x > kPi) {
        cosV = -cosV;
    }
    if (x < kNegHalfPi) {
        sinV = -sinV;
    }
    return sinV / cosV;
}

float gxMathTan(float angle) {
    float cosV;
    float sinV;

    gxMathCosSin(&cosV, &sinV, angle);
    return sinV / cosV;
}

float gxMathSin2(float angle) {
    return gxMathSin(angle);
}

float gxMathSin(float angle) {
    unsigned int bits;
    int folded;
    float scale;
    float x2;
    float x;
    float t;

    bits = (int)(angle * kAngleToIndex);
    folded = bits & 0x3FFFFu;
    if ((bits & 0x40000u) != 0) {
        folded = 0x40000 - folded;
    }
    x = folded;
    if ((bits & 0x80000u) != 0) {
        scale = kNegIndexToRad;
    } else {
        scale = kIndexToRad;
    }
    x *= scale;
    x2 = x * x;
    t = kSinC6 * x2 + kSinC4;
    t = t * x2 + kSinC2;
    t = x2 * t + kOne;
    return x * t;
}

void gxMathCosSin2(float* cosOut, float* sinOut, float angle) {
    gxMathCosSin(cosOut, sinOut, angle);
}

void gxMathCosSin(float* cosOut, float* sinOut, float angle) {
    unsigned int bits;
    unsigned int doubled;
    int folded;
    float scale;
    float cosV;
    float x2;
    float sinV;
    float x;

    bits = (int)(angle * kAngleToIndex);
    folded = bits & 0x3FFFFu;
    if ((bits & 0x40000u) != 0) {
        folded = 0x40000 - folded;
    }
    doubled = bits + bits;
    x = folded;
    if ((bits & 0x80000u) != 0) {
        scale = kNegIndexToRad;
    } else {
        scale = kIndexToRad;
    }
    x *= scale;
    x2 = x * x;
    sinV = kSinC6 * x2;
    cosV = kCosC6 * x2;
    sinV += kSinC4;
    cosV += kCosC4;
    sinV *= x2;
    cosV *= x2;
    sinV += kSinC2;
    cosV += kCosC2;
    sinV *= x2;
    cosV *= x2;
    sinV += kOne;
    cosV += kOne;
    sinV *= x;
    if (((doubled ^ bits) & 0x80000u) != 0) {
        cosV = -cosV;
    }
    *sinOut = sinV;
    *cosOut = cosV;
}

float gxMathCos2(float angle) {
    return gxMathCos(angle);
}

float gxMathCos(float angle) {
    unsigned int bits;
    int folded;
    float x;
    float x2;
    float t;

    bits = (int)(angle * kAngleToIndex);
    folded = bits & 0x3FFFFu;
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
    float result;
    float ratio;

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
        result = ratio - kAtan3 * t3 + kAtan5 * t5 - kAtan7 * t7 + kAtan9 * t9 -
                 kAtan11 * t11 + kAtan13 * t13 - kAtan15 * t15 + kAtan17 * t17 -
                 kAtan19 * t19 + kAtan21 * t21 - kAtan23 * t23 + kAtan25 * t25 -
                 kAtan27 * t27;
    } else {
        float inv;
        float x2;
        float t3;
        float t5;
        float t7;
        float t9;
        float t11;
        float t13;
        float t15;

        inv = kOne / ratio;
        x2 = inv * inv;
        t3 = x2 * inv;
        t5 = t3 * x2;
        t7 = t5 * x2;
        t9 = t7 * x2;
        t11 = t9 * x2;
        t13 = t11 * x2;
        t15 = t13 * x2;
        result = -inv + kAtan3 * t3 - kAtan5 * t5 + kAtan7 * t7 - kAtan9 * t9 +
                 kAtan11 * t11 - kAtan13 * t13 + kAtan15 * t15;
        if (ratio > kOne) {
            result += kHalfPi;
        } else {
            result -= kHalfPi;
        }
    }

    if (x < kZero) {
        if (y >= kZero) {
            result += kPi;
        } else {
            result -= kPi;
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
    float invSq;

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
        return x - kAtan3 * t3 + kAtan5 * t5 - kAtan7 * t7 + kAtan9 * t9 -
               kAtan11 * t11 + kAtan13 * t13 - kAtan15 * t15 +
               kAtan17 * t17 - kAtan19 * t19 + kAtan21 * t21 -
               kAtan23 * t23 + kAtan25 * t25 - kAtan27 * t27;
    }

    inv = kOne / x;
    invSq = inv * inv;
    t3 = invSq * inv;
    t5 = t3 * invSq;
    t7 = t5 * invSq;
    t9 = t7 * invSq;
    t11 = t9 * invSq;
    t13 = t11 * invSq;
    t15 = t13 * invSq;
    result = -inv + kAtan3 * t3 - kAtan5 * t5 + kAtan7 * t7 - kAtan9 * t9 +
             kAtan11 * t11 - kAtan13 * t13 + kAtan15 * t15;
    if (x > kOne) {
        result += kHalfPi;
    } else {
        result -= kHalfPi;
    }
    return result;
}

float gxMathArcSin(float x) {
    float x2;
    float p;

    if (x < kAcosLo0) {
        if (x >= kAcosLo1 || x >= kAcosLo2 || x >= kAcosLo3) {
            return kHalfPi - gxMathArcCos(x);
        }
        return kNegHalfPi;
    }
    if (x > kAcosHi0) {
        if (x <= kAcosHi1 || x <= kAcosHi2 || x <= kAcosHi3) {
            return kHalfPi - gxMathArcCos(x);
        }
        return kHalfPi;
    }
    x2 = x * x;
    p = kAcosMidC2 + x2 * (kAcosMidC3 + x2 * (kAcosMidC4 + x2 * (kAcosMidC5 + x2 * kAcosMidC6)));
    return -x * (kNegOne + x2 * (kSinC2 + x2 * p));
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

    x2 = x * x;
    p = kAcosMidC6 * x2 + kAcosMidC5;
    p = p * x2 + kAcosMidC4;
    p = x2 * p + kAcosMidC3;
    p = x2 * p + kAcosMidC2;
    p = x2 * p + kSinC2;
    p = x2 * p + kNegOne;
    return x * p + kHalfPi;
}

#include "src/math/gxmath_sqrt_table.inc"

static float sinCosTable[16][2] = {
    { 0.00000000f, 1.00000000f },
    { 0.38268343f, 0.92387953f },
    { 0.70710678f, 0.70710678f },
    { 0.92387953f, 0.38268343f },
    { 1.00000000f, 0.00000000f },
    { 0.92387953f, -0.38268343f },
    { 0.70710678f, -0.70710678f },
    { 0.38268343f, -0.92387953f },
    { 0.00000000f, -1.00000000f },
    { -0.38268343f, -0.92387953f },
    { -0.70710678f, -0.70710678f },
    { -0.92387953f, -0.38268343f },
    { -1.00000000f, -0.00000000f },
    { -0.92387953f, 0.38268343f },
    { -0.70710678f, 0.70710678f },
    { -0.38268343f, 0.92387953f }
};
