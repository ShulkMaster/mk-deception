#include "dolphin/ax.h"
#include "dolphin/ax_internal.h"
#include "dolphin/os.h"

const char* __AXVersion =
    "<< Dolphin SDK - AX\trelease build: Apr  5 2004 04:15:05 (0x2301) >>";

void AXInitEx(unsigned long outputBufferMode)
{
    OSRegisterVersion(__AXVersion);
    __AXAllocInit();
    __AXVPBInit();
    __AXSPBInit();
    __AXAuxInit();
    __AXClInit();
    __AXOutInit(outputBufferMode);
}
