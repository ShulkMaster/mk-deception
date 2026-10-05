#include "dolphin/trk.h"
#include "runtime/asm_sequences.inc"

extern struct TRKRestoreFlags gTRKRestoreFlags;

asm void TRKSaveExtended1Block(void)
{
    SEQ_TRKSaveExtended1Block();
}

asm void TRKRestoreExtended1Block(void)
{
    SEQ_TRKRestoreExtended1Block();
}
