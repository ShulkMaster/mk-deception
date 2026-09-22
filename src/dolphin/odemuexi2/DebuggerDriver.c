#include "dolphin/debugger_driver.h"
#include "dolphin/os.h"

#ifdef __MWERKS__
volatile u32 __EXIRegs[] : 0xCC006800;
#else
static volatile u32 HostEXIRegs[15];
#define __EXIRegs HostEXIRegs
#endif

#define IS_TRUE(value) ((value) != 0)
#define IS_FALSE(value) !IS_TRUE(value)
#define ROUND_UP(value, alignment) (((value) + (alignment)-1) & (-(alignment)))

static EXICallback MTRCallback;
static EXICallback DBGCallback;
static u32 SendMailData;
static s32 RecvDataLeng;
static volatile u8* pEXIInputFlag;
static volatile u8 EXIInputFlag;
static u8 SendCount = 0x80;

static BOOL DBGEXIImm(void* buffer, s32 bytecounter, u32 write);
static BOOL DBGReadMailbox(u32* value);
static BOOL DBGRead(u32 count, u32* buffer, s32 size);
static BOOL DBGWrite(u32 count, void* buffer, s32 size);
static BOOL DBGReadStatus(u32* value);
static void DBGHandler(__OSInterrupt interrupt, OSContext* context);
static void MWCallback(signed long channel, OSContext* context);

inline void DBGEXIInit(void)
{
    __OSMaskInterrupts(0x18000);
    __EXIRegs[10] = 0;
}

inline static u32 DBGEXISelect(u32 device)
{
    u32 state = __EXIRegs[10];

    state &= 0x405;
    state |= 0x80 | (device << 4);
    __EXIRegs[10] = state;
    return 1;
}

inline static BOOL DBGEXIDeselect(void)
{
    __EXIRegs[10] &= 0x405;
    return 1;
}

inline static BOOL DBGEXISync(void)
{
    while (__EXIRegs[13] & 1) {
    }
    return 1;
}

inline static BOOL DBGWriteMailbox(u32 p1)
{
    u32 cmd = 0xC0000000;
    u32 v;
    u32 base = p1;
    BOOL total = 0;

    DBGEXISelect(4);
    v = (base & 0x1FFFFFFF) | cmd;
    total |= IS_FALSE(DBGEXIImm(&v, sizeof(v), 1));
    total |= IS_FALSE(DBGEXISync());
    total |= IS_FALSE(DBGEXIDeselect());
    return IS_FALSE(total);
}

inline static BOOL _DBGReadStatus(u32* p1)
{
    BOOL total = 0;
    u32 v;

    DBGEXISelect(4);
    v = 1 << 30;
    total |= IS_FALSE(DBGEXIImm(&v, 2, 1));
    total |= IS_FALSE(DBGEXISync());
    total |= IS_FALSE(DBGEXIImm(p1, 4, 0));
    total |= IS_FALSE(DBGEXISync());
    total |= IS_FALSE(DBGEXIDeselect());
    return IS_FALSE(total);
}

inline static void CheckMailBox(void)
{
    u32 value;

    DBGReadStatus(&value);
    if (value & 1) {
        DBGReadMailbox(&value);
        value &= 0x1FFFFFFF;
        if ((value & 0x1F000000) == 0x1F000000) {
            SendMailData = value;
            RecvDataLeng = value & 0x7FFF;
            EXIInputFlag = 1;
        }
    }
}

void DBClose(void)
{
}

void DBOpen(void)
{
}

int DBWrite(const void* src, int size)
{
    u32 v;
    u32 busyFlag;
    BOOL interrupts = OSDisableInterrupts();

    do {
        _DBGReadStatus(&busyFlag);
    } while (busyFlag & 2);

    SendCount++;
    v = (SendCount & 1) ? 0x1000 : 0;
    while (!DBGWrite(v | 0x1C000, (void*)src, ROUND_UP(size, 4))) {
    }

    do {
        _DBGReadStatus(&busyFlag);
    } while (busyFlag & 2);

    v = SendCount;
    while (!DBGWriteMailbox((0x1F000000) | v << 0x10 | size)) {
    }

    do {
        while (!_DBGReadStatus(&busyFlag)) {
        }
    } while (busyFlag & 2);

    OSRestoreInterrupts(interrupts);
    return 0;
}

int DBRead(void* buffer, u32 count)
{
    u32 interrupts = OSDisableInterrupts();
    u32 v = (SendMailData & 0x10000) ? 0x1000 : 0;

    DBGRead(v + 0x1E000, buffer, ROUND_UP(count, 4));
    RecvDataLeng = 0;
    EXIInputFlag = 0;
    OSRestoreInterrupts(interrupts);
    return 0;
}

int DBQueryData(void)
{
    int interrupts;

    EXIInputFlag = 0;
    if (!RecvDataLeng) {
        interrupts = OSDisableInterrupts();
        CheckMailBox();
        OSRestoreInterrupts(interrupts);
    }
    return RecvDataLeng;
}

void DBInitInterrupts(void)
{
    __OSMaskInterrupts(0x18000);
    __OSMaskInterrupts(0x40);
    DBGCallback = MWCallback;
    __OSSetInterruptHandler(0x19, DBGHandler);
    __OSUnmaskInterrupts(0x40);
}

/* TODO: [near miss] 93.333336%; retail schedules the EXI reset constant before
 * its MMIO address; keep the ABI-correct source and stop at localized lowering. */
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

/* TODO: [near miss] 87.500000%; retail loads DBGCallback before the PI reset
 * store; ABI and callback CFG match, stop at localized scheduling. */
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

static BOOL DBGReadStatus(u32* value)
{
    return _DBGReadStatus(value);
}

static BOOL DBGWrite(u32 count, void* buffer, s32 size)
{
    BOOL failed = 0;
    u32* cursor = (u32*)buffer;
    u32 command;
    u32 value;

    DBGEXISelect(4);
    command = (count & 0x1FFFC) << 8 | 0xA0000000;
    failed |= IS_FALSE(DBGEXIImm(&command, sizeof(command), 1));
    failed |= IS_FALSE(DBGEXISync());
    while (size != 0) {
        value = *cursor++;
        failed |= IS_FALSE(DBGEXIImm(&value, sizeof(value), 1));
        failed |= IS_FALSE(DBGEXISync());
        size -= 4;
        if (size < 0) {
            size = 0;
        }
    }
    failed |= IS_FALSE(DBGEXIDeselect());
    return IS_FALSE(failed);
}

static BOOL DBGRead(u32 count, u32* buffer, s32 size)
{
    BOOL failed = 0;
    u32* cursor = buffer;
    u32 command;
    u32 value;

    DBGEXISelect(4);
    command = (count & 0x1FFFC) << 8 | 0x20000000;
    failed |= IS_FALSE(DBGEXIImm(&command, sizeof(command), 1));
    failed |= IS_FALSE(DBGEXISync());
    while (size != 0) {
        failed |= IS_FALSE(DBGEXIImm(&value, sizeof(value), 0));
        failed |= IS_FALSE(DBGEXISync());
        *cursor++ = value;
        size -= 4;
        if (size < 0) {
            size = 0;
        }
    }
    failed |= IS_FALSE(DBGEXIDeselect());
    return IS_FALSE(failed);
}

static BOOL DBGReadMailbox(u32* value)
{
    BOOL total = 0;
    u32 v;

    DBGEXISelect(4);
    v = 0x60000000;
    total |= IS_FALSE(DBGEXIImm(&v, 2, 1));
    total |= IS_FALSE(DBGEXISync());
    total |= IS_FALSE(DBGEXIImm(value, 4, 0));
    total |= IS_FALSE(DBGEXISync());
    total |= IS_FALSE(DBGEXIDeselect());
    return IS_FALSE(total);
}

static BOOL DBGEXIImm(void* buffer, s32 bytecounter, u32 write)
{
    u8* cursor;
    u32 value;
    int i;

    if (write) {
        value = 0;
        for (i = 0; i < bytecounter; i++) {
            value |= ((u8*)buffer)[i] << ((3 - i) << 3);
        }
        __EXIRegs[14] = value;
    }
    __EXIRegs[13] = 1 | write << 2 | (bytecounter - 1) << 4;
    DBGEXISync();
    if (!write) {
        value = __EXIRegs[14];
        cursor = (u8*)buffer;
        for (i = 0; i < bytecounter; i++) {
            *cursor++ = value >> ((3 - i) << 3);
        }
    }
    return 1;
}
