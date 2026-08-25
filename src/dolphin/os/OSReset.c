#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "dolphin/pad.h"
#include "runtime/cstring.h"

typedef struct OSResetFunctionQueue {
    OSResetFunctionInfo* head;
    OSResetFunctionInfo* tail;
} OSResetFunctionQueue;

typedef struct OSRebootParams {
    int valid;
    unsigned long restartCode;
    unsigned char reserved[0x14];
} OSRebootParams;

static OSResetFunctionQueue ResetFunctionQueue;
static unsigned long bootThisDol;

extern OSThreadQueue __OSActiveThreadQueue;
extern OSRebootParams __OSRebootParams;

volatile unsigned short __VIRegs[] : 0xCC002000;
volatile unsigned long __PIRegs[] : 0xCC003000;

static void Reset(unsigned long resetCode)
{
    /* Retail performs HID0/time-base/PI reset sequencing in privileged code. */
}

void OSRegisterResetFunction(OSResetFunctionInfo* info)
{
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* previous;

    for (next = ResetFunctionQueue.head;
         next != 0 && next->priority <= info->priority;
         next = next->next) {}

    if (next == 0) {
        previous = ResetFunctionQueue.tail;
        if (previous == 0) {
            ResetFunctionQueue.head = info;
        } else {
            previous->next = info;
        }
        info->prev = previous;
        info->next = 0;
        ResetFunctionQueue.tail = info;
    } else {
        info->next = next;
        previous = next->prev;
        next->prev = info;
        info->prev = previous;
        if (previous == 0) {
            ResetFunctionQueue.head = info;
        } else {
            previous->next = info;
        }
    }
}

int __OSCallResetFunctions(int final)
{
    OSResetFunctionInfo* info;
    int error = 0;
    unsigned long priority = 0;

    for (info = ResetFunctionQueue.head; info != 0; info = info->next) {
        if (error != 0 && priority != info->priority) {
            break;
        }
        error |= !info->func(final);
        priority = info->priority;
    }
    error |= !__OSSyncSram();
    return error == 0;
}

static void KillThreads(void)
{
    OSThread* thread;
    OSThread* next;

    for (thread = __OSActiveThreadQueue.head; thread != 0; thread = next) {
        next = thread->linkActive.next;
        if (thread->state == 1 || thread->state == 4) {
            OSCancelThread(thread);
        }
    }
}

void __OSDoHotReset(unsigned long resetCode)
{
    OSDisableInterrupts();
    __VIRegs[1] = 0;
    ICFlashInvalidate();
    Reset(resetCode * 8);
}

static void ShutdownDevices(int recalibrate)
{
    int previousRecalibration = 0;

    __OSStopAudioSystem();
    if (!recalibrate) {
        previousRecalibration = __PADDisableRecalibration(1);
    }
    while (!__OSCallResetFunctions(0)) {}
    while (!__OSSyncSram()) {}
    OSDisableInterrupts();
    __OSCallResetFunctions(1);
    LCDisable();
    if (!recalibrate) {
        __PADDisableRecalibration(previousRecalibration);
    }
    KillThreads();
}

void OSResetSystem(int reset, unsigned long resetCode, int forceMenu)
{
    OSSram* sram;

    OSDisableScheduler();
    if (reset == 1 && forceMenu) {
        sram = __OSLockSram();
        sram->flags |= 0x40;
        __OSUnlockSram(1);
        resetCode = 0;
    }

    if (reset == 2 ||
        (reset == 0 && (bootThisDol != 0 || resetCode + 0x3FFF0000 == 0))) {
        ShutdownDevices(0);
    } else {
        ShutdownDevices(1);
    }

    if (reset == 1) {
        __OSDoHotReset(resetCode);
    } else if (reset == 0) {
        if (forceMenu == 1) {
            OSReport("OSResetSystem(): You can't specify TRUE to forceMenu if you restart. Ignored\n");
        }
        OSEnableScheduler();
        __OSReboot(resetCode, bootThisDol);
    }

    memset(OSPhysicalToCached(0x40), 0, 0x8C);
    memset(OSPhysicalToCached(0xD4), 0, 0x14);
    memset(OSPhysicalToCached(0xF4), 0, 4);
    memset(OSPhysicalToCached(0x3000), 0, 0xC0);
    memset(OSPhysicalToCached(0x30C8), 0, 0x0C);
    memset(OSPhysicalToCached(0x30E2), 0, 1);
}

unsigned int OSGetResetCode(void)
{
    if (__OSRebootParams.valid) {
        return 0x80000000 | __OSRebootParams.restartCode;
    }
    return (__PIRegs[9] & 0xFFFFFFF8) / 8;
}
