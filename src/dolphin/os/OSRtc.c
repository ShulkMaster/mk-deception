#include "dolphin/cache.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"

#define SRAM_SIZE (sizeof(OSSram) + sizeof(OSSramEx))

typedef struct SramControl {
    unsigned char sram[SRAM_SIZE];
    unsigned long offset;
    int interrupts_enabled;
    int locked;
    int synchronized;
    void (*callback)(void);
} SramControl;

static SramControl Scb __attribute__((aligned(32)));

static int WriteSram(void* buffer, unsigned long offset, unsigned long size);

static void WriteSramCallback(signed long channel, OSContext* context)
{
    Scb.synchronized =
        WriteSram(&Scb.sram[Scb.offset], Scb.offset, SRAM_SIZE - Scb.offset);
    if (Scb.synchronized) Scb.offset = SRAM_SIZE;
}

static int WriteSram(void* buffer, unsigned long offset, unsigned long size)
{
    int error;
    unsigned long command;

    if (!EXILock(0, 1, WriteSramCallback)) return 0;
    if (!EXISelect(0, 1, 3)) {
        EXIUnlock(0);
        return 0;
    }
    command = 0xA0000000 | 0x100 | (offset << 6);
    error = 0;
    error |= !EXIImm(0, &command, 4, EXI_WRITE, 0);
    error |= !EXISync(0);
    error |= !EXIImmEx(0, buffer, size, EXI_WRITE);
    error |= !EXIDeselect(0);
    EXIUnlock(0);
    return !error;
}

void __OSInitSram(void)
{
    int error;
    unsigned long command;

    Scb.locked = 0;
    Scb.interrupts_enabled = 0;
    DCInvalidateRange(&Scb, SRAM_SIZE);
    if (!EXILock(0, 1, 0)) {
        Scb.synchronized = 0;
    } else if (!EXISelect(0, 1, 3)) {
        EXIUnlock(0);
        Scb.synchronized = 0;
    } else {
        command = 0x20000100;
        error = 0;
        error |= !EXIImm(0, &command, 4, EXI_WRITE, 0);
        error |= !EXISync(0);
        error |= !EXIDma(0, &Scb, SRAM_SIZE, EXI_READ, 0);
        error |= !EXISync(0);
        error |= !EXIDeselect(0);
        EXIUnlock(0);
        Scb.synchronized = !error;
    }
    Scb.offset = SRAM_SIZE;
    OSSetGbsMode(OSGetGbsMode());
}

static void* LockSram(unsigned long offset)
{
    int enabled = OSDisableInterrupts();
    if (Scb.locked) {
        OSRestoreInterrupts(enabled);
        return 0;
    }
    Scb.interrupts_enabled = enabled;
    Scb.locked = 1;
    return &Scb.sram[offset];
}

OSSram* __OSLockSram(void) { return (OSSram*)LockSram(0); }
OSSramEx* __OSLockSramEx(void) { return (OSSramEx*)LockSram(sizeof(OSSram)); }

static int UnlockSram(int commit, unsigned long offset)
{
    unsigned short* word;

    if (commit) {
        if (offset == 0) {
            OSSram* sram = (OSSram*)Scb.sram;
            if ((sram->flags & 3) > 2) sram->flags &= ~3;
            sram->checkSum = 0;
            sram->checkSumInv = 0;
            for (word = (unsigned short*)&sram->counterBias;
                 word < (unsigned short*)&Scb.sram[0x14]; word++) {
                sram->checkSum += *word;
                sram->checkSumInv += ~*word;
            }
        }
        if (offset < Scb.offset) Scb.offset = offset;
        if (Scb.offset <= 0x14) {
            OSSramEx* extended = (OSSramEx*)(Scb.sram + sizeof(OSSram));
            if ((extended->gbs & 0x7C00) == 0x5000 ||
                (extended->gbs & 0xC0) == 0xC0)
                extended->gbs = 0;
        }
        Scb.synchronized =
            WriteSram(&Scb.sram[Scb.offset], Scb.offset,
                      SRAM_SIZE - Scb.offset);
        if (Scb.synchronized) Scb.offset = SRAM_SIZE;
    }
    Scb.locked = 0;
    OSRestoreInterrupts(Scb.interrupts_enabled);
    return Scb.synchronized;
}

int __OSUnlockSram(int commit) { return UnlockSram(commit, 0); }
int __OSUnlockSramEx(int commit) { return UnlockSram(commit, sizeof(OSSram)); }
int __OSSyncSram(void) { return Scb.synchronized; }

int __OSReadROM(void* buffer, signed long length, signed long offset)
{
    int error;
    unsigned long command;

    DCInvalidateRange(buffer, length);
    if (!EXILock(0, 1, 0)) return 0;
    if (!EXISelect(0, 1, 3)) {
        EXIUnlock(0);
        return 0;
    }
    command = offset << 6;
    error = 0;
    error |= !EXIImm(0, &command, 4, EXI_WRITE, 0);
    error |= !EXISync(0);
    error |= !EXIDma(0, buffer, length, EXI_READ, 0);
    error |= !EXISync(0);
    error |= !EXIDeselect(0);
    EXIUnlock(0);
    return !error;
}

unsigned int OSGetProgressiveMode(void)
{
    OSSram* sram = __OSLockSram();
    unsigned int enabled = (sram->flags & 0x80) >> 7;
    __OSUnlockSram(0);
    return enabled;
}

void OSSetProgressiveMode(unsigned int enabled)
{
    OSSram* sram;
    enabled = (enabled << 7) & 0x80;
    sram = __OSLockSram();
    if (enabled == (sram->flags & 0x80)) {
        __OSUnlockSram(0);
        return;
    }
    sram->flags = (sram->flags & ~0x80) | enabled;
    __OSUnlockSram(1);
}

unsigned char OSGetLanguage(void)
{
    OSSram* sram = __OSLockSram();
    unsigned char language = sram->language;
    __OSUnlockSram(0);
    return language;
}

unsigned short OSGetWirelessID(signed long channel)
{
    OSSramEx* sram = __OSLockSramEx();
    unsigned short id = sram->wirelessPadID[channel];
    __OSUnlockSramEx(0);
    return id;
}

void OSSetWirelessID(signed long channel, unsigned short id)
{
    OSSramEx* sram = __OSLockSramEx();
    if (sram->wirelessPadID[channel] != id) {
        sram->wirelessPadID[channel] = id;
        __OSUnlockSramEx(1);
        return;
    }
    __OSUnlockSramEx(0);
}

unsigned short OSGetGbsMode(void)
{
    OSSramEx* sram = __OSLockSramEx();
    unsigned short mode = sram->gbs;
    __OSUnlockSramEx(0);
    return mode;
}

void OSSetGbsMode(unsigned short mode)
{
    OSSramEx* sram;
    if ((mode & 0x7C00) == 0x5000 || (mode & 0xC0) == 0xC0) mode = 0;
    sram = __OSLockSramEx();
    if (mode == sram->gbs) {
        __OSUnlockSramEx(0);
        return;
    }
    sram->gbs = mode;
    __OSUnlockSramEx(1);
}
