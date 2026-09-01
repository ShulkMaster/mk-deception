#include "dolphin/base/PPCArch.h"
#include "dolphin/cache.h"
#include "dolphin/db.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "dolphin/si.h"
#include "runtime/asm_sequences.inc"

extern void* memset(void*, int, unsigned long);
extern void* memcpy(void*, const void*, unsigned long);
extern void EnableMetroTRKInterrupts(void);
extern unsigned long __DVDLongFileNameFlag;
extern unsigned long __PADSpec;
extern unsigned short __OSDeviceCode;
extern volatile unsigned long __DIRegs[];
extern unsigned char __ArenaLo[], __ArenaHi[];
extern char _stack_addr[];

#define OS_EXCEPTION_COUNT 15
#define NOP_INSTRUCTION 0x60000000

const char* __OSVersion =
    "<< Dolphin SDK - OS\trelease build: May 21 2004 09:28:09 (0x2301) >>";
static DVDDriveInfo DriveInfo;
static DVDCommandBlock DriveBlock;
OSExecParams __OSRebootParams;
static OSBootInfo* BootInfo;
static unsigned long* BI2DebugFlag;
static unsigned long BI2DebugFlagHolder;
OSTime __OSStartTime;
int __OSInIPL;
OSExceptionHandler* OSExceptionTable;
static int AreWeInitialized;
static float ZeroPS[2];
static double ZeroF;
int __OSIsGcam;

static unsigned long __OSExceptionLocations[OS_EXCEPTION_COUNT] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800,
    0x900, 0xC00, 0xD00, 0xF00, 0x1300, 0x1400, 0x1700
};

/* Architectural register initialization is not expressible in portable C. */
asm void __OSFPRInit(void) { SEQ___OSFPRInit(); }

void __OSPSInit(void)
{
    PPCMthid2(PPCMfhid2() | 0xA0000000);
    ICFlashInvalidate();
    PPCSync();
}

unsigned long OSGetConsoleType(void)
{
    if (BootInfo == 0 || BootInfo->console_type == 0) return 0x10000002;
    return BootInfo->console_type;
}

static inline void ClearArena(void)
{
    unsigned long lo = (unsigned long)OSGetArenaLo();
    unsigned long hi = (unsigned long)OSGetArenaHi();
    unsigned long start = (unsigned long)__OSRebootParams.region_start;
    unsigned long end = (unsigned long)__OSRebootParams.region_end;

    if ((OSGetResetCode() & 0x80000000) == 0 || start == 0) {
        memset((void*)lo, 0, hi - lo);
        return;
    }
    if (lo < start) {
        if (hi <= start) {
            memset((void*)lo, 0, hi - lo);
            return;
        }
        memset((void*)lo, 0, start - lo);
        if (hi > end) memset((void*)end, 0, hi - end);
    }
}

static void InquiryCallback(long result, DVDCommandBlock* block)
{
    switch (block->state) {
    case 0:
        __OSDeviceCode = 0x8000 | DriveInfo.device_code;
        break;
    default:
        __OSDeviceCode = 1;
        break;
    }
}

static inline void DisableWriteGatherPipe(void)
{
    PPCMthid2(PPCMfhid2() & ~0x40000000);
}

/* Retail implements these symbols as copied exception-vector instruction
 * templates. Empty C leaves preserve their linkage without embedding asm. */
void __OSEVStart(void) {}
void __OSEVSetNumber(void) {}
void __DBVECTOR(void) {}
void __OSEVEnd(void) {}
void __OSDBINTSTART(void) {}
void __OSDBINTEND(void) {}
void __OSDBJUMPSTART(void) {}
void __OSDBJUMPEND(void) {}

void OSDefaultExceptionHandler(__OSException exception, OSContext* context)
{
    __OSUnhandledException(exception, context, 0, 0);
}

static void OSExceptionInit(void)
{
    __OSException exception;
    unsigned char* handler = (unsigned char*)__OSEVStart;
    unsigned long handler_size =
        (unsigned char*)__OSEVEnd - (unsigned char*)__OSEVStart;
    unsigned long* opcode = (unsigned long*)__OSEVSetNumber;
    unsigned long old_opcode = *opcode;
    void* destination = (void*)0x80000060;

    if (*(unsigned long*)destination == 0) {
        unsigned long size =
            (unsigned char*)__OSDBINTEND - (unsigned char*)__OSDBINTSTART;
        DBPrintf("Installing OSDBIntegrator\n");
        memcpy(destination, (void*)__OSDBINTSTART, size);
        DCFlushRangeNoSync(destination, size);
        PPCSync();
        ICInvalidateRange(destination, size);
    }
    for (exception = 0; exception < OS_EXCEPTION_COUNT; exception++) {
        unsigned long* db_vector;
        unsigned long size;
        int offset;
        if (BI2DebugFlag && *BI2DebugFlag >= 2 &&
            __DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
            continue;
        }
        *opcode = old_opcode | exception;
        db_vector = (unsigned long*)__DBVECTOR;
        size = (unsigned char*)__OSDBJUMPEND -
               (unsigned char*)__OSDBJUMPSTART;
        if (__DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
            memcpy(db_vector, (void*)__OSDBJUMPSTART, size);
        } else {
            for (offset = 0; offset < (int)size; offset += 4) {
                *db_vector++ = NOP_INSTRUCTION;
            }
        }
        destination = (void*)(0x80000000 | __OSExceptionLocations[exception]);
        memcpy(destination, handler, handler_size);
        DCFlushRangeNoSync(destination, handler_size);
        PPCSync();
        ICInvalidateRange(destination, handler_size);
    }
    OSExceptionTable = (OSExceptionHandler*)0x80003000;
    for (exception = 0; exception < OS_EXCEPTION_COUNT; exception++) {
        __OSSetExceptionHandler(exception, OSDefaultExceptionHandler);
    }
    *opcode = old_opcode;
    DBPrintf("Exceptions initialized...\n");
}

OSExceptionHandler __OSSetExceptionHandler(__OSException exception,
                                            OSExceptionHandler handler)
{
    OSExceptionHandler old = OSExceptionTable[exception];
    OSExceptionTable[exception] = handler;
    return old;
}

OSExceptionHandler __OSGetExceptionHandler(__OSException exception)
{
    return OSExceptionTable[exception];
}

void OSInit(void)
{
    unsigned long console_type;
    void* bi2;
    if (AreWeInitialized) return;
    AreWeInitialized = 1;
    __OSStartTime = __OSGetSystemTime();
    OSDisableInterrupts();
    __OSGetExecParams(&__OSRebootParams);

    BootInfo = (OSBootInfo*)0x80000000;
    BI2DebugFlag = 0;
    __DVDLongFileNameFlag = 0;
    bi2 = *(void**)0x800000F4;
    if (bi2) {
        BI2DebugFlag = (unsigned long*)((char*)bi2 + 0xC);
        __PADSpec = ((unsigned long*)bi2)[9];
        *(volatile unsigned char*)0x800030E8 = *BI2DebugFlag;
        *(volatile unsigned char*)0x800030E9 = __PADSpec;
    } else if (BootInfo->arena_hi) {
        BI2DebugFlagHolder = *(volatile unsigned char*)0x800030E8;
        BI2DebugFlag = &BI2DebugFlagHolder;
        __PADSpec = *(volatile unsigned char*)0x800030E9;
    }
    __DVDLongFileNameFlag = 1;
    OSSetArenaLo(BootInfo->arena_lo ? BootInfo->arena_lo : __ArenaLo);
    if (!BootInfo->arena_lo && BI2DebugFlag && *BI2DebugFlag < 2)
        OSSetArenaLo((void*)OSRoundUp32B(_stack_addr));
    OSSetArenaHi(BootInfo->arena_hi ? BootInfo->arena_hi : __ArenaHi);

    OSExceptionInit();
    __OSInitSystemCall();
    OSInitAlarm();
    __OSModuleInit();
    __OSInterruptInit();
    __OSSetInterruptHandler(0x16, __OSResetSWInterruptHandler);
    __OSContextInit();
    __OSCacheInit();
    EXIInit();
    SIInit();
    __OSInitSram();
    __OSThreadInit();
    __OSInitAudioSystem();
    DisableWriteGatherPipe();
    if (!__OSInIPL) __OSInitMemoryProtection();

    OSReport("\nDolphin OS\n");
    OSReport("Kernel built : %s %s\n", "May 21 2004", "09:28:09");
    OSReport("Console Type : ");
    console_type = OSGetConsoleType();
    switch (console_type & 0xF0000000) {
    case 0: OSReport("Retail %d\n", console_type); break;
    case 0x10000000:
    case 0x20000000:
        switch (console_type & 0x0FFFFFFF) {
        case 0: OSReport("Mac Emulator\n"); break;
        case 1: OSReport("PC Emulator\n"); break;
        case 2: OSReport("EPPC Arthur\n"); break;
        case 3: OSReport("EPPC Minnow\n"); break;
        default:
            OSReport("Development HW%d (%08x)\n",
                     (console_type & 0x0FFFFFFF) - 3, console_type);
        }
        break;
    default: OSReport("%08x\n", console_type);
    }
    OSReport("Memory %d MB\n", BootInfo->memory_size >> 20);
    OSReport("Arena : 0x%x - 0x%x\n", OSGetArenaLo(), OSGetArenaHi());
    OSRegisterVersion(__OSVersion);
    if (BI2DebugFlag && *BI2DebugFlag >= 2) EnableMetroTRKInterrupts();
    ClearArena();
    OSEnableInterrupts();

    if (!__OSInIPL) {
        DVDInit();
        if (__OSIsGcam) {
            __OSDeviceCode = 0x9000;
            return;
        }
        DCInvalidateRange(&DriveInfo, sizeof(DriveInfo));
        DVDInquiryAsync(&DriveBlock, &DriveInfo, InquiryCallback);
    }
}

unsigned long __OSGetDIConfig(void) { return __DIRegs[9] & 0xFF; }
void OSRegisterVersion(const char* version) { OSReport("%s\n", version); }
