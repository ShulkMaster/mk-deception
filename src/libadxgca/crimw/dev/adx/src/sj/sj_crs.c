#include "dolphin/os.h"

volatile int sjcrs_lvl = 0;
int sjcrs_msk = 0;

void SJCRS_Unlock(void)
{
    sjcrs_lvl--;
    if (sjcrs_lvl == 0) {
        OSRestoreInterrupts(sjcrs_msk);
    }
}

void SJCRS_Lock(void)
{
    if (sjcrs_lvl == 0) {
        sjcrs_msk = OSDisableInterrupts();
    }
    sjcrs_lvl++;
}
