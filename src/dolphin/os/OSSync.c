#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "runtime/asm_sequences.inc"

void __OSSystemCallVectorStart(void);
void __OSSystemCallVectorEnd(void);

static asm void SystemCallVector(void)
{
    SEQ_SystemCallVector();
}

void __OSInitSystemCall(void)
{
    void* address = OSPhysicalToCached(0xC00);

    memcpy(address, __OSSystemCallVectorStart,
           (unsigned long)&__OSSystemCallVectorEnd -
               (unsigned long)&__OSSystemCallVectorStart);
    DCFlushRangeNoSync(address, 0x100);
    __sync();
    ICInvalidateRange(address, 0x100);
}
