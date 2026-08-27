#include "dolphin/base/PPCArch.h"

void PPCDisableSpeculation(void)
{
    PPCMthid0(PPCMfhid0() | 0x200);
}
