#include "dolphin/base/PPCArch.h"

typedef void (*Ctor)(void);

extern Ctor _ctors[];

static void __init_cpp(void);

void __init_user(void) {
    __init_cpp();
}

static void __init_cpp(void) {
    Ctor* ctor;

    /* Soft ceiling: __init_cpp ~71.19% - retail keeps redundant loop branches. */
    for (ctor = _ctors; *ctor != 0; ctor++) {
        (*ctor)();
    }
}

void _ExitProcess(void)
{
    PPCHalt();
}
