#ifndef MSL_MATH_H
#define MSL_MATH_H

#ifdef __MWERKS__
#pragma cplusplus on
#endif

extern inline float sqrtf(float x)
{
    static const double _half = 0.5;
    static const double _three = 3.0;
    volatile float result;

    if (x > 0.0f) {
        double guess = __frsqrte((double)x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        result = (float)(x * guess);
        return result;
    }
    return x;
}

#ifdef __MWERKS__
#pragma cplusplus reset
#endif

#endif
