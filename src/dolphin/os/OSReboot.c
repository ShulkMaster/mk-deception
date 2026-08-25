#include "dolphin/os.h"
#include "dolphin/os_alloc.h"

static void* SaveStart;
static void* SaveEnd;

void __OSReboot(unsigned long reset_code, unsigned long boot_dol)
{
    OSContext context;
    char* argv;

    OSDisableInterrupts();
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812f0000);
    OSClearContext(&context);
    OSSetCurrentContext(&context);

    argv = 0;
    __OSBootDol(boot_dol, reset_code | 0x80000000, &argv);
}

void OSGetSaveRegion(void** start, void** end)
{
    *start = SaveStart;
    *end = SaveEnd;
}
