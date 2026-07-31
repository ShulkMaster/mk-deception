#include "libmkparticle/color.h"

void pfx_native_set_rgba(PfxColor* color, float r, float g, float b, float a) {
    color->r = (unsigned char)r;
    color->g = (unsigned char)g;
    color->b = (unsigned char)b;
    color->a = (unsigned char)a;
}

void pfx_native_get_rgba(const PfxColor* color, float* r, float* g, float* b,
                         float* a) {
    *r = color->r;
    *g = color->g;
    *b = color->b;
    *a = color->a;
}
