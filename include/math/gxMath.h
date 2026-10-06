#ifndef GX_MATH_H
#define GX_MATH_H

extern unsigned short GXMathSqrtTable[];

float gxMathTan(float angle);
float gxMathSin(float angle);
void gxMathCosSin(float* cosOut, float* sinOut, float angle);
float gxMathCos(float angle);
float gxMathArcTanYX(float y, float x);
float gxMathArcTan(float x);
float gxMathArcCos(float x);

static inline float gxMathFastInvSqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    guess.f = value;
    guess.u = 0x5F375A00U - (guess.u >> 1);
    product = guess.f * (value * guess.f);
    correction = 3.0f - product;
    return 0.0625f * guess.f * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline float gxMathFastSqrt(float value) {
    union {
        float f;
        unsigned int u;
    } out;
    float guess;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    out.u = GXMathSqrtTable[(*(unsigned int*)&value >> 11) & 0x1FFF] << 8;
    out.u |= (((*(unsigned int*)&value & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    guess = out.f;
    correction = 3.0f - (guess * guess) / value;
    return 0.5f * (guess * correction);
}

#endif
