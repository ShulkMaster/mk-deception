#ifndef MSL_MATH_H
#define MSL_MATH_H

#ifdef __MWERKS__
#pragma cplusplus on
#endif

extern inline float sqrtf(float x)
{
    volatile float result;

    if (x > 0.0f) {
        double guess = __frsqrte((double)x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        result = (float)(x * guess);
        return result;
    }
    return x;
}

#ifdef __MWERKS__
#pragma cplusplus reset
#endif

#endif
