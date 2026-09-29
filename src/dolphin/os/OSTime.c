#include "dolphin/os.h"
#include "runtime/asm_sequences.inc"

#define __OSSystemTime ((OSTime*)0x800030D8)

asm OSTime OSGetTime(void) {
    SEQ_OSGetTime();
}

asm OSTick OSGetTick(void) {
    SEQ_OSGetTick();
}

OSTime __OSGetSystemTime(void)
{
    int enabled;
    OSTime* time_adjust;
    OSTime result;

    time_adjust = __OSSystemTime;
    enabled = OSDisableInterrupts();
    result = *time_adjust + OSGetTime();
    OSRestoreInterrupts(enabled);
    return result;
}

OSTime __OSTimeToSystemTime(OSTime time)
{
    int enabled;
    OSTime* time_adjust = __OSSystemTime;
    OSTime result;

    enabled = OSDisableInterrupts();
    result = *time_adjust + time;
    OSRestoreInterrupts(enabled);
    return result;
}
