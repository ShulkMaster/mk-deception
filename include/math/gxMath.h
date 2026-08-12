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

#endif
