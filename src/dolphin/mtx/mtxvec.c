#include "dolphin/mtx.h"
#include "math/gxVect.h"
#include "runtime/asm_sequences.inc"

asm void PSMTXMultVec(const Mtx matrix, const Vec* source, Vec* destination)
{
    SEQ_PSMTXMultVec();
}
