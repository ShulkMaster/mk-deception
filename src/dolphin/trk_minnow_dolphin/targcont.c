#include "dolphin/trk.h"

extern void TRKTargetSetStopped(int stopped);
extern void UnreserveEXI2Port(void);
extern void TRKSwapAndGo(void);
extern void ReserveEXI2Port(void);

DSError TRKTargetContinue(void)
{
    TRKTargetSetStopped(0);
    UnreserveEXI2Port();
    TRKSwapAndGo();
    ReserveEXI2Port();
    return 0;
}
