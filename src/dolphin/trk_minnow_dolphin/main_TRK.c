typedef int DSError;

extern void MWTRACE(int level, const char* format, ...);
extern DSError TRKInitializeNub(void);
extern void TRKNubWelcome(void);
extern void TRKNubMainLoop(void);
extern DSError TRKTerminateNub(void);

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
