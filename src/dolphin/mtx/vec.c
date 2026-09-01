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

asm void PSVECNormalize(const Vec* source, Vec* unit)
{
    SEQ_PSVECNormalize();
}

asm float PSVECMag(const Vec* vector)
{
    SEQ_PSVECMag();
}

float PSVECDotProduct(const Vec* a, const Vec* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

asm void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* product)
{
    SEQ_PSVECCrossProduct();
}
