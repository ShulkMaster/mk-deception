#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"

void __OSSystemCallVectorStart(void);
void __OSSystemCallVectorEnd(void);

/*
 * The preceding retail SystemCallVector is an authentic privileged assembly
 * leaf (HID0, sync/isync, and rfi). It remains supplied by the retail object;
 * portable C cannot represent that exception-vector contract honestly.
 */
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
