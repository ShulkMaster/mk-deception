#include "math/gxVect.h"
#include "runtime/asm_sequences.inc"

const float PSVECMagHalf = 0.5f;
const float PSVECMagThree = 3.0f;

asm void PSVECAdd(const Vec* a, const Vec* b, Vec* sum)
{
    SEQ_PSVECAdd();
}

asm void PSVECSubtract(const Vec* a, const Vec* b, Vec* difference)
{
    SEQ_PSVECSubtract();
}

asm void PSVECScale(const Vec* source, Vec* scaled, float scale)
{
    SEQ_PSVECScale();
}

asm void PSVECNormalize(const Vec* source, Vec* unit)
{
    SEQ_PSVECNormalize();
}

asm float PSVECMag(const Vec* vector)
{
    SEQ_PSVECMag();
}

asm float PSVECDotProduct(const Vec* a, const Vec* b)
{
    SEQ_PSVECDotProduct();
}

asm void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* product)
{
    SEQ_PSVECCrossProduct();
}
