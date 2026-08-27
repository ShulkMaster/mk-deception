#include "dolphin/base/PPCArch.h"

void PPCMthid0(unsigned long value);

void PPCDisableSpeculation(void)
{
    PPCMthid0(PPCMfhid0() | 0x200);
}
