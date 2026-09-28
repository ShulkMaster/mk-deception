#ifndef GX_MATH_H
#define GX_MATH_H

/*
 * Midway / GX-style float trig + arccos (gxMath.o).
 * GXMathSqrtTable is .data of this TU (0x4000 bytes, generated
 * gxmath_sqrt_table.inc). Used by quat / cam invsqrt helpers.
 */

extern unsigned short GXMathSqrtTable[];

float gxMathTan(float angle);
float gxMathSin(float angle);
void gxMathCosSin(float* cosOut, float* sinOut, float angle);
float gxMathCos(float angle);
float gxMathArcTanYX(float y, float x);
float gxMathArcTan(float x);
float gxMathArcCos(float x);

static inline float gxMathFastSqrt(float value) {
    union {
        float f;
        unsigned int u;
    } in, out;
    float guess;
    float correction;

    in.f = value;
    if (value <= 0.0f) {
        return 0.0f;
    }
    out.u = (unsigned int)GXMathSqrtTable[(in.u >> 11) & 0x1FFF] << 8;
    out.u |= (((in.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    guess = out.f;
    correction = 3.0f - (guess * guess) / value;
    return 0.5f * (guess * correction);
}

#endif
