#include "math/gxQuat.h"

extern float sqrtf(float value);

/*
 * Soft ceiling: both retail functions are paired-single assembly leaves.  Keep
 * their typed scalar algorithms here instead of embedding assembly or forcing
 * the compiler's register allocation.
 */
void PSQUATMultiply(const Quat* p, const Quat* q, Quat* product)
{
    Quat result;

    result.x = p->w * q->x + p->x * q->w + p->y * q->z - p->z * q->y;
    result.y = p->w * q->y + p->y * q->w + p->z * q->x - p->x * q->z;
    result.z = p->w * q->z + p->z * q->w + p->x * q->y - p->y * q->x;
    result.w = p->w * q->w - p->x * q->x - p->y * q->y - p->z * q->z;

    *product = result;
}

void PSQUATNormalize(const Quat* source, Quat* unit)
{
    float magnitude_squared;
    float inverse_magnitude;

    magnitude_squared = source->x * source->x + source->y * source->y +
                        source->z * source->z + source->w * source->w;
    if (magnitude_squared >= 0.00001f) {
        inverse_magnitude = 1.0f / sqrtf(magnitude_squared);
    } else {
        inverse_magnitude = 0.0f;
    }

    unit->x = source->x * inverse_magnitude;
    unit->y = source->y * inverse_magnitude;
    unit->z = source->z * inverse_magnitude;
    unit->w = source->w * inverse_magnitude;
}
