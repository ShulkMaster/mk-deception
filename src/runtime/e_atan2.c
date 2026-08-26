#include "fdlibm.h"

extern double __fabs(double value);

static const double tiny = 1.0e-300;
static const double zero = 0.0;
static const double pi_o_4 = 7.8539816339744827900e-01;
static const double pi_o_2 = 1.5707963267948965580e+00;
static const double pi = 3.1415926535897931160e+00;
static const double pi_lo = 1.2246467991473531772e-16;

double __ieee754_atan2(double y, double x)
{
    double z;
    int k;
    int quadrant;
    int hx;
    int hy;
    int ix;
    int iy;
    unsigned int lx;
    unsigned int ly;

    hx = __HI(x);
    ix = hx & 0x7fffffff;
    lx = __LO(x);
    hy = __HI(y);
    iy = hy & 0x7fffffff;
    ly = __LO(y);
    if (((ix | ((lx | -lx) >> 31)) > 0x7ff00000) ||
        ((iy | ((ly | -ly) >> 31)) > 0x7ff00000))
        return x + y;
    if (((hx - 0x3ff00000) | lx) == 0)
        return atan(y);

    quadrant = ((hy >> 31) & 1) | ((hx >> 30) & 2);
    if ((iy | ly) == 0) {
        switch (quadrant) {
        case 0:
        case 1:
            return y;
        case 2:
            return pi + tiny;
        case 3:
            return -pi - tiny;
        }
    }
    if ((ix | lx) == 0)
        return hy < 0 ? -pi_o_2 - tiny : pi_o_2 + tiny;

    if (ix == 0x7ff00000) {
        if (iy == 0x7ff00000) {
            switch (quadrant) {
            case 0:
                return pi_o_4 + tiny;
            case 1:
                return -pi_o_4 - tiny;
            case 2:
                return 3.0 * pi_o_4 + tiny;
            case 3:
                return -3.0 * pi_o_4 - tiny;
            }
        } else {
            switch (quadrant) {
            case 0:
                return zero;
            case 1:
                return -zero;
            case 2:
                return pi + tiny;
            case 3:
                return -pi - tiny;
            }
        }
    }
    if (iy == 0x7ff00000)
        return hy < 0 ? -pi_o_2 - tiny : pi_o_2 + tiny;

    k = (iy - ix) >> 20;
    if (k > 60)
        z = pi_o_2 + 0.5 * pi_lo;
    else if (hx < 0 && k < -60)
        z = 0.0;
    else
        z = atan(__fabs(y / x));

    switch (quadrant) {
    case 0:
        return z;
    case 1:
        __HI(z) ^= 0x80000000;
        return z;
    case 2:
        return pi - (z - pi_lo);
    default:
        return (z - pi_lo) - pi;
    }
}
