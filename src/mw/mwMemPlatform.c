#include "mw/mwMemPlatform.h"

#include "mw/mwMemHeap.h"

typedef struct {
    unsigned char gpr;
    unsigned char fpr;
    unsigned char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_start(list, last_arg) __va_start(list, last_arg)

void OSInit(void);
char* OSGetArenaHi(void);
char* OSGetArenaLo(void);
void* OSInitAlloc(void* arena_start, void* arena_end, int max_heaps);
void OSSetArenaLo(void* arena_low);
OSHeapHandle OSCreateHeap(void* heap_start, void* heap_end);
OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap);
int OSCheckHeap(OSHeapHandle heap);
void* OSAllocFromHeap(OSHeapHandle heap, unsigned long size);
int vsprintf(char* buffer, const char* format, __va_list args);

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
