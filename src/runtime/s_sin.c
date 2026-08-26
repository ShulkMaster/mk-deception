#include "fdlibm.h"

double sin(double x)
{
    double y[2];
    double z = 0.0;
    int n;
    int ix;

    ix = __HI(x) & 0x7fffffff;
    if (ix <= 0x3fe921fb)
        return __kernel_sin(x, z, 0);
    if (ix >= 0x7ff00000)
        return x - x;

    n = __ieee754_rem_pio2(x, y);
    switch (n & 3) {
    case 0:
        return __kernel_sin(y[0], y[1], 1);
    case 1:
        return __kernel_cos(y[0], y[1]);
    case 2:
        return -__kernel_sin(y[0], y[1], 1);
    default:
        return -__kernel_cos(y[0], y[1]);
    }
}
