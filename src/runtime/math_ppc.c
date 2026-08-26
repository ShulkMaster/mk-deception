#include "fdlibm.h"

typedef union FloatRepresentation {
    float value;
    unsigned long bits;
} FloatRepresentation;

extern unsigned long __float_nan[];
extern double __frsqrte(double x);

static inline int classify_float(float value)
{
    FloatRepresentation representation;
    unsigned long bits;

    representation.value = value;
    bits = representation.bits;

    switch (bits & 0x7f800000) {
    case 0x7f800000:
        if ((bits & 0x007fffff) != 0)
            return 1;
        return 2;
    case 0:
        if ((bits & 0x007fffff) != 0)
            return 5;
        return 3;
    }
    return 4;
}

__declspec(weak) float sqrtf(float x)
{
    if (x > 0.0f) {
        double estimate = __frsqrte((double)x);

        estimate = 0.5 * estimate * (3.0 - estimate * estimate * x);
        estimate = 0.5 * estimate * (3.0 - estimate * estimate * x);
        estimate = 0.5 * estimate * (3.0 - estimate * estimate * x);
        return (float)(x * estimate);
    }

    if (x < 0.0)
        return ((FloatRepresentation*)__float_nan)[0].value;

    if (classify_float(x) == 1)
        x = ((FloatRepresentation*)__float_nan)[0].value;
    return x;
}

__declspec(weak) float fmodf(float x, float y)
{
    return (float)fmod((double)x, (double)y);
}

__declspec(weak) float powf(float x, float y)
{
    return (float)pow((double)x, (double)y);
}

__declspec(weak) float sinf(float x)
{
    return (float)sin((double)x);
}

__declspec(weak) float cosf(float x)
{
    return (float)cos((double)x);
}
