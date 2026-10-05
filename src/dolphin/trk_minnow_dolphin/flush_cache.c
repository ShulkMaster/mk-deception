#include "dolphin/trk.h"
#include "runtime/asm_sequences.inc"

asm void TRK_flush_cache(u32 address, u32 size)
{
    SEQ_TRK_flush_cache();
}
