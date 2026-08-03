#include "mw/mwMemPlatform.h"

#include "mw/mwMemHeap.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "runtime/cstdarg.h"
#include "runtime/cstdio.h"

static char printBuff[0x200];

/* MWCC emits these small-BSS declarations in reverse order. */
int gap_08_80510ED4_sbss;
OSHeapHandle GameCubeSystemHeap;

int privConsoleMemSystemInit(void) {
    void* arena_high;

    arena_high = OSGetArenaHi();
    OSSetArenaLo(OSInitAlloc(OSGetArenaLo(), arena_high, 1));

    arena_high = OSGetArenaHi();
    GameCubeSystemHeap = OSCreateHeap(OSGetArenaLo(), arena_high);
    OSSetCurrentHeap(GameCubeSystemHeap);
    OSCheckHeap(GameCubeSystemHeap);
    return 1;
}

unsigned long mwMemSystemGetAvailSize(void) {
    char* arena_high;

    OSInit();
    arena_high = OSGetArenaHi();
    return (unsigned long)(arena_high - OSGetArenaLo()) - 0x140;
}

unsigned char* privGetOSMemory(unsigned long size) {
    return OSAllocFromHeap(GameCubeSystemHeap, size);
}

void MEMPRINT(const char* format, ...) {
    __va_list args;

    va_start(args, format);
    vsprintf(printBuff, format, args);
    mwMemUserConfigPrintf(printBuff);
}
