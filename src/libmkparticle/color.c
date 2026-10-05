#include "libmkparticle/color.h"

void pfx_native_set_rgba(PfxColor* color, float r, float g, float b, float a) {
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

void pfx_native_get_rgba(const PfxColor* color, float* r, float* g, float* b,
                         float* a) {
    *r = color->r;
    *g = color->g;
    *b = color->b;
    *a = color->a;
}
