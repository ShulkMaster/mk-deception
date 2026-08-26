#include "dolphin/ax.h"
#include "dolphin/os.h"

const char* __AXVersion =
    "<< Dolphin SDK - AX\trelease build: Apr  5 2004 04:15:05 (0x2301) >>";

void __AXAllocInit(void);
void __AXVPBInit(void);
void __AXSPBInit(void);
void __AXAuxInit(void);
void __AXClInit(void);
void __AXOutInit(unsigned long outputBufferMode);

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
