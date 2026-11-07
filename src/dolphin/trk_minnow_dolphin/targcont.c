#include "dolphin/trk.h"
#include "dolphin/dolphin_trk_glue.h"
#include "dolphin/targimpl.h"


DSError TRKTargetContinue(void)
{
    TRKTargetSetStopped(0);
    UnreserveEXI2Port();
    TRKSwapAndGo();
    ReserveEXI2Port();
    return 0;
}
