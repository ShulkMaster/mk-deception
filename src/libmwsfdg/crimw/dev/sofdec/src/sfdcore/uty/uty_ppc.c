#include "dolphin/types.h"
#include "runtime/asm_sequences.inc"

asm void UTY_PopGqr(u32 saved[8])
{
    SEQ_UTY_PopGqr();
}

asm void UTY_PushGqr(u32 saved[8])
{
    SEQ_UTY_PushGqr();
}
