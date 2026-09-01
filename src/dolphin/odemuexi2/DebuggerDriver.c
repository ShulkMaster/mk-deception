#include "dolphin/debugger_driver.h"
#include "dolphin/os.h"

static EXICallback MTRCallback;
static EXICallback DBGCallback;
static volatile u8* pEXIInputFlag;
static volatile u8 EXIInputFlag;

static void DBGHandler(__OSInterrupt interrupt, OSContext* context);
static void MWCallback(signed long channel, OSContext* context);

void DBClose(void)
{
}

void DBOpen(void)
{
}

void DBInitInterrupts(void)
{
    __OSMaskInterrupts(0x18000);
    __OSMaskInterrupts(0x40);
    DBGCallback = MWCallback;
    __OSSetInterruptHandler(0x19, DBGHandler);
    __OSUnmaskInterrupts(0x40);
}

void DBInitComm(volatile u8** input_pending, EXICallback monitor_callback)
{
    int enabled;

    enabled = OSDisableInterrupts();
    pEXIInputFlag = &EXIInputFlag;
    *input_pending = pEXIInputFlag;
    MTRCallback = monitor_callback;
    __OSMaskInterrupts(0x18000);
    *(volatile u32*)0xCC006828 = 0;
    OSRestoreInterrupts(enabled);
}

static void DBGHandler(__OSInterrupt interrupt, OSContext* context)
{
    *(volatile u32*)0xCC003000 = 0x1000;
    if (DBGCallback != 0) {
        DBGCallback((s16)interrupt, context);
    }
}

static void MWCallback(signed long channel, OSContext* context)
{
    EXIInputFlag = 1;
    if (MTRCallback != 0) {
        MTRCallback(0, context);
    }
}
