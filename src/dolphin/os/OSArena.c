#include "dolphin/os_alloc.h"

#define ROUND(address, alignment) \
    (((unsigned long)(address) + (alignment)-1) & ~((alignment)-1))

static void* __OSArenaHi;
static void* __OSArenaLo = (void*)-1;

void* OSGetArenaHi(void) {
    return __OSArenaHi;
}

void* OSGetArenaLo(void) {
    return __OSArenaLo;
}

void OSSetArenaHi(void* newHi) {
    __OSArenaHi = newHi;
}

void OSSetArenaLo(void* newLo) {
    __OSArenaLo = newLo;
}

void* OSAllocFromArenaLo(unsigned long size, unsigned long align) {
    void* ptr;
    unsigned char* arenaLo;

    ptr = OSGetArenaLo();
    arenaLo = ptr = (void*)ROUND(ptr, align);
    arenaLo += size;
    arenaLo = (unsigned char*)ROUND(arenaLo, align);
    OSSetArenaLo(arenaLo);
    return ptr;
}
