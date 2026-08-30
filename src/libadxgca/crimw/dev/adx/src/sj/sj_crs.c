#include "dolphin/os.h"

int sjcrs_lvl = 0;
int sjcrs_msk = 0;

/* Soft ceiling: retail reloads sjcrs_lvl after the store; MWCC folds the
 * equivalent decrement-and-zero-test into addic./store. */
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
