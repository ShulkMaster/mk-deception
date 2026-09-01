#include "math/gxQuat.h"
#include "runtime/asm_sequences.inc"

extern float sqrtf(float value);

const float PSQUATNormalizeEpsilon = 0.00001f;
const float PSQUATNormalizeHalf = 0.5f;
const float PSQUATNormalizeThree = 3.0f;

asm void PSQUATMultiply(const Quat* p, const Quat* q, Quat* product)
{
    SEQ_PSQUATMultiply();
}

asm void PSQUATNormalize(const Quat* source, Quat* unit)
{
    SEQ_PSQUATNormalize();
}
