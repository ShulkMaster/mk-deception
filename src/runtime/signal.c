#include "runtime/critical_regions.h"

typedef void (*SignalHandler)(int);

extern void exit(int status);

#define SIG_IGN ((SignalHandler)1)

SignalHandler signal_funcs[6];

int raise(int signal)
{
    SignalHandler handler;

    if (signal < 1 || signal > 6)
        return -1;

    __begin_critical_region(4);
    handler = signal_funcs[signal - 1];
    if (handler != SIG_IGN)
        signal_funcs[signal - 1] = 0;
    __end_critical_region(4);

    if (handler == SIG_IGN || (handler == 0 && signal == 1))
        return 0;
    if (handler == 0)
        exit(0);
    handler(signal);
    return 0;
}
