#ifndef MKD_RUNTIME_CMATH_H
#define MKD_RUNTIME_CMATH_H

#ifdef __cplusplus
extern "C" {
#endif

double __fabs(double value);
double floor(double value);
double ceil(double value);
double pow(double x, double y);
double fmod(double numerator, double denominator);
double log10(double value);
double atan2(double y, double x);
float sinf(float value);
float cosf(float value);
float floorf(float value);
float ceilf(float value);
float powf(float x, float y);
float atan2f(float y, float x);

#ifdef VERSION_GQNE5D
#define floorf(value) ((float)floor(value))
#define ceilf(value) ((float)ceil(value))
#define powf(x, y) ((float)pow((x), (y)))
#define atan2f(y, x) ((float)atan2((y), (x)))
#endif

#ifdef __cplusplus
}
#endif

#endif
