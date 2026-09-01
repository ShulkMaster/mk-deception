#include "math/gxVect.h"
#include "runtime/asm_sequences.inc"

extern float sqrtf(float value);

const float PSVECMagHalf = 0.5f;
const float PSVECMagThree = 3.0f;

/*
 * Soft ceiling: retail implements this complete TU with paired-single leaves.
 * These typed scalar forms preserve the public algorithms and alias behavior.
 */
asm void PSVECAdd(const Vec* a, const Vec* b, Vec* sum)
{
    SEQ_PSVECAdd();
}

asm void PSVECSubtract(const Vec* a, const Vec* b, Vec* difference)
{
    SEQ_PSVECSubtract();
}

void PSVECScale(const Vec* source, Vec* scaled, float scale)
{
    scaled->x = source->x * scale;
    scaled->y = source->y * scale;
    scaled->z = source->z * scale;
}

void PSVECNormalize(const Vec* source, Vec* unit)
{
    float magnitude_squared;
    float inverse_magnitude;

    magnitude_squared = source->x * source->x + source->y * source->y +
                        source->z * source->z;
    inverse_magnitude = 1.0f / sqrtf(magnitude_squared);
    unit->x = source->x * inverse_magnitude;
    unit->y = source->y * inverse_magnitude;
    unit->z = source->z * inverse_magnitude;
}

asm float PSVECMag(const Vec* vector)
{
    SEQ_PSVECMag();
}

float PSVECDotProduct(const Vec* a, const Vec* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* product)
{
    Vec result;

    result.x = a->y * b->z - a->z * b->y;
    result.y = a->z * b->x - a->x * b->z;
    result.z = a->x * b->y - a->y * b->x;
    *product = result;
}
