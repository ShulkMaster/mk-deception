#include "dolphin/os.h"

#define __OSSystemTime ((OSTime*)0x800030D8)

/*
 * OSGetTime and OSGetTick are authentic no-frame time-base-register leaves in
 * retail.  Their ABI is declared in dolphin/os.h; portable C does not define a
 * truthful portable replacement for the mftbu/mftb instruction sequence.
 */

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
