#include "dolphin/trk.h"

static DSError TRK_mainError;

DSError TRK_main(void)
{
    MWTRACE(1, "TRK_Main \n");
    TRK_mainError = TRKInitializeNub();
    if (TRK_mainError == 0) {
        TRKNubWelcome();
        TRKNubMainLoop();
    }
    TRK_mainError = TRKTerminateNub();
    return TRK_mainError;
}
