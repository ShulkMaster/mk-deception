#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"

static unsigned char DSPInitCode[128] = {
    0x02, 0x9F, 0x00, 0x10, 0x02, 0x9F, 0x00, 0x33, 0x02, 0x9F, 0x00, 0x34, 0x02, 0x9F, 0x00, 0x35,
    0x02, 0x9F, 0x00, 0x36, 0x02, 0x9F, 0x00, 0x37, 0x02, 0x9F, 0x00, 0x38, 0x02, 0x9F, 0x00, 0x39,
    0x12, 0x06, 0x12, 0x03, 0x12, 0x04, 0x12, 0x05, 0x00, 0x80, 0x80, 0x00, 0x00, 0x88, 0xFF, 0xFF,
    0x00, 0x84, 0x10, 0x00, 0x00, 0x64, 0x00, 0x1D, 0x02, 0x18, 0x00, 0x00, 0x81, 0x00, 0x1C, 0x1E,
    0x00, 0x44, 0x1B, 0x1E, 0x00, 0x84, 0x08, 0x00, 0x00, 0x64, 0x00, 0x27, 0x19, 0x1E, 0x00, 0x00,
    0x00, 0xDE, 0xFF, 0xFC, 0x02, 0xA0, 0x80, 0x00, 0x02, 0x9C, 0x00, 0x28, 0x16, 0xFC, 0x00, 0x54,
    0x16, 0xFD, 0x43, 0x48, 0x00, 0x21, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0xFF,
    0x02, 0xFF, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

volatile unsigned short __DSPRegs[] : 0xCC005000;

#define DSP_WORK_BUFFER ((void*)0x81000000)

void __OSInitAudioSystem(void)
{
    unsigned short reg16;
    unsigned long startTick;

    memcpy((unsigned char*)OSGetArenaHi() - sizeof(DSPInitCode), DSP_WORK_BUFFER, sizeof(DSPInitCode));
    memcpy(DSP_WORK_BUFFER, DSPInitCode, sizeof(DSPInitCode));
    DCFlushRange(DSP_WORK_BUFFER, sizeof(DSPInitCode));

    __DSPRegs[9] = 0x43;
    __DSPRegs[5] = 0x8AC;
    __DSPRegs[5] |= 1;
    while (__DSPRegs[5] & 1) {}
    __DSPRegs[0] = 0;
    while (((__DSPRegs[2] << 16) | __DSPRegs[3]) & 0x80000000) {}

    *(volatile unsigned long*)&__DSPRegs[16] = 0x01000000;
    *(volatile unsigned long*)&__DSPRegs[18] = 0;
    *(volatile unsigned long*)&__DSPRegs[20] = 0x20;
    reg16 = __DSPRegs[5];
    while (!(reg16 & 0x20)) {
        reg16 = __DSPRegs[5];
    }
    __DSPRegs[5] = reg16;

    startTick = OSGetTick();
    while ((signed long)(OSGetTick() - startTick) < 0x892) {}

    *(volatile unsigned long*)&__DSPRegs[16] = 0x01000000;
    *(volatile unsigned long*)&__DSPRegs[18] = 0;
    *(volatile unsigned long*)&__DSPRegs[20] = 0x20;
    reg16 = __DSPRegs[5];
    while (!(reg16 & 0x20)) {
        reg16 = __DSPRegs[5];
    }
    __DSPRegs[5] = reg16;

    __DSPRegs[5] &= ~0x800;
    while (__DSPRegs[5] & 0x400) {}
    __DSPRegs[5] &= ~4;

    reg16 = __DSPRegs[2];
    while (!(reg16 & 0x8000)) {
        reg16 = __DSPRegs[2];
    }
    if ((((unsigned long)((reg16 << 16) | __DSPRegs[3])) + 0x7FAC0000UL) != 0x4348) {
        /* The retail assertion body is compiled out in non-debug builds. */
    }

    __DSPRegs[5] |= 4;
    __DSPRegs[5] = 0x8AC;
    __DSPRegs[5] |= 1;
    while (__DSPRegs[5] & 1) {}

    memcpy(DSP_WORK_BUFFER, (unsigned char*)OSGetArenaHi() - sizeof(DSPInitCode), sizeof(DSPInitCode));
}

void __OSStopAudioSystem(void)
{
    unsigned short reg16;
    unsigned long startTick;

    __DSPRegs[5] = 0x804;
    reg16 = __DSPRegs[27];
    __DSPRegs[27] = reg16 & ~0x8000;
    reg16 = __DSPRegs[5];
    while (reg16 & 0x400) {
        reg16 = __DSPRegs[5];
    }
    reg16 = __DSPRegs[5];
    while (reg16 & 0x200) {
        reg16 = __DSPRegs[5];
    }
    __DSPRegs[5] = 0x8AC;
    __DSPRegs[0] = 0;
    while (((__DSPRegs[2] << 16) | __DSPRegs[3]) & 0x80000000) {}

    startTick = OSGetTick();
    while ((signed long)(OSGetTick() - startTick) < 0x2C) {}

    reg16 = __DSPRegs[5];
    __DSPRegs[5] = reg16 | 1;
    reg16 = __DSPRegs[5];
    while (reg16 & 1) {
        reg16 = __DSPRegs[5];
    }
}
