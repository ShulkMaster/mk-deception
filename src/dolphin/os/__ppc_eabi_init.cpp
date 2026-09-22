#include "dolphin/base/PPCArch.h"

typedef void (*Ctor)(void);

extern Ctor _ctors[];

static void __init_cpp(void);

void __init_user(void) {
    __init_cpp();
}

/* TODO: [breakthrough needed] 71.190475%; retail has redundant loop-entry branches that readable loop forms do not recover. */
static void __init_cpp(void) {
    Ctor* ctor;

    for (ctor = _ctors; *ctor != 0; ctor++) {
        (*ctor)();
    }
}

void _ExitProcess(void)
{
    PPCHalt();
}
