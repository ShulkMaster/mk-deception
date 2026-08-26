#include "fdlibm.h"

double log10(double x)
{
    return __ieee754_log10(x);
}
