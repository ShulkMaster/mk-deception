#include "dolphin/circle_buffer.h"
#include "dolphin/os.h"

void MWInitializeCriticalSection(u32* section)
{
}

void MWEnterCriticalSection(u32* section)
{
    *section = OSDisableInterrupts();
}

void MWExitCriticalSection(u32* section)
{
    OSRestoreInterrupts(*section);
}
