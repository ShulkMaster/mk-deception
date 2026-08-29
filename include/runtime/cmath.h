#ifndef MKD_RUNTIME_CMATH_H
#define MKD_RUNTIME_CMATH_H

#ifdef __cplusplus
extern "C" {
#endif

double floor(double value);
double fmod(double numerator, double denominator);
double log10(double value);
double atan2(double y, double x);
float sinf(float value);
float cosf(float value);

#ifdef __cplusplus
}
#endif

#endif
