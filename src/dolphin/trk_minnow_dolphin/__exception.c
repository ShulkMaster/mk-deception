#include "runtime/asm_sequences.inc"

void TRKInterruptHandler(void);
void __TRK_reset(void);

#pragma section code_type ".init"

asm void gTRKInterruptVectorTable(void)
{
    SEQ_gTRKInterruptVectorTable();
}
