#include "dolphin/os.h"
#include "dolphin/types.h"

extern "C" {

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

}
