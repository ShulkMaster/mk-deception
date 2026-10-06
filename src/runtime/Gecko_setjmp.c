#include "runtime/asm_sequences.inc"

asm void *__setjmp(void)
{
    SEQ___setjmp();
}

asm void *longjmp(void)
{
    SEQ_longjmp();
}
