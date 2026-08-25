static void (*FatalFunc)(void);

void __DVDPrintFatalMessage(void)
{
    if (FatalFunc != 0) {
        FatalFunc();
    }
}
