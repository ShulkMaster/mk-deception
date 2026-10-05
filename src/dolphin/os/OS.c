#include "dolphin/base/PPCArch.h"
#include "dolphin/cache.h"
#include "dolphin/db.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "dolphin/si.h"
#include "runtime/cstring.h"
#include "runtime/asm_sequences.inc"

extern void EnableMetroTRKInterrupts(void);
extern unsigned long __DVDLongFileNameFlag;
extern unsigned long __PADSpec;
unsigned short __OSDeviceCode : 0x800030E6;
volatile unsigned long __DIRegs[] : 0xCC006000;
extern unsigned char __ArenaLo[], __ArenaHi[];
extern char _stack_addr[];

#define OS_EXCEPTION_COUNT 15
#define NOP_INSTRUCTION 0x60000000

const char* __OSVersion =
    "<< Dolphin SDK - OS\trelease build: May 21 2004 09:28:09 (0x2301) >>";
static DVDDriveInfo DriveInfo __attribute__((aligned(32)));
static DVDCommandBlock DriveBlock;
OSExecParams __OSRebootParams;
static OSBootInfo* BootInfo;
static unsigned long* BI2DebugFlag;
static unsigned long BI2DebugFlagHolder;
__declspec(weak) int __OSIsGcam = 0;
static double ZeroF;
static float ZeroPS[2];
static int AreWeInitialized = 0;
static OSExceptionHandler* OSExceptionTable;
OSTime __OSStartTime;
int __OSInIPL;

/* Architectural register initialization is not expressible in portable C. */
asm void __OSFPRInit(void) { SEQ___OSFPRInit(); }

unsigned long OSGetConsoleType(void)
{
    if (BootInfo == 0 || BootInfo->console_type == 0) return OS_CONSOLE_ARTHUR;
    return BootInfo->console_type;
}

static void OSExceptionInit(void);
void OSDefaultExceptionHandler(__OSException exception, OSContext* context);

static void ClearArena(void)
{
    if (!((OSGetResetCode() & 0x80000000) ? 1 : 0)) {
        memset(OSGetArenaLo(), 0,
               (unsigned long)OSGetArenaHi() - (unsigned long)OSGetArenaLo());
        return;
    }

    if ((unsigned long)__OSRebootParams.region_start == 0) {
        memset(OSGetArenaLo(), 0,
               (unsigned long)OSGetArenaHi() - (unsigned long)OSGetArenaLo());
        return;
    }

    if ((unsigned long)OSGetArenaLo() < (unsigned long)__OSRebootParams.region_start) {
        if ((unsigned long)OSGetArenaHi() <= (unsigned long)__OSRebootParams.region_start) {
            memset(OSGetArenaLo(), 0,
                   (unsigned long)OSGetArenaHi() - (unsigned long)OSGetArenaLo());
            return;
        }

        memset(OSGetArenaLo(), 0,
               (unsigned long)__OSRebootParams.region_start - (unsigned long)OSGetArenaLo());

        if ((unsigned long)OSGetArenaHi() > (unsigned long)__OSRebootParams.region_end) {
            memset(__OSRebootParams.region_end, 0,
                   (unsigned long)OSGetArenaHi() - (unsigned long)__OSRebootParams.region_end);
        }
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

void OSInit(void)
{
    unsigned long console_type;
    void* bi2;

    if (AreWeInitialized == 0) {
        AreWeInitialized = 1;

        __OSStartTime = __OSGetSystemTime();
        OSDisableInterrupts();

        __OSGetExecParams(&__OSRebootParams);
        PPCMtmmcr0(0);
        PPCMtmmcr1(0);
        PPCMtpmc1(0);
        PPCMtpmc2(0);
        PPCMtpmc3(0);
        PPCMtpmc4(0);
        PPCDisableSpeculation();
        PPCSetFpNonIEEEMode();

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

        OSSetArenaLo(!BootInfo->arena_lo ? __ArenaLo : BootInfo->arena_lo);
        if (!BootInfo->arena_lo && BI2DebugFlag && *BI2DebugFlag < 2) {
            OSSetArenaLo((void*)OSRoundUp32B(_stack_addr));
        }
        OSSetArenaHi(!BootInfo->arena_hi ? __ArenaHi : BootInfo->arena_hi);

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

        if (!__OSInIPL) {
            __OSInitMemoryProtection();
        }

        OSReport("\nDolphin OS\n");
        OSReport("Kernel built : %s %s\n", "May 21 2004", "09:28:09");
        OSReport("Console Type : ");

        console_type = OSGetConsoleType();
        switch (console_type & OS_CONSOLE_MASK) {
        case OS_CONSOLE_RETAIL:
            OSReport("Retail %d\n", console_type);
            break;
        case OS_CONSOLE_DEVELOPMENT:
        case OS_CONSOLE_TDEV:
            switch (console_type & 0x0FFFFFFF) {
            case OS_CONSOLE_EMULATOR:
                OSReport("Mac Emulator\n");
                break;
            case OS_CONSOLE_PC_EMULATOR:
                OSReport("PC Emulator\n");
                break;
            case OS_CONSOLE_ARTHUR:
                OSReport("EPPC Arthur\n");
                break;
            case OS_CONSOLE_MINNOW:
                OSReport("EPPC Minnow\n");
                break;
            default:
                OSReport("Development HW%d (%08x)\n",
                         (console_type & 0x0FFFFFFF) - 3, console_type);
                break;
            }
            break;
        default:
            OSReport("%08x\n", console_type);
            break;
        }

        OSReport("Memory %d MB\n", BootInfo->memory_size >> 20);
        OSReport("Arena : 0x%x - 0x%x\n", OSGetArenaLo(), OSGetArenaHi());
        OSRegisterVersion(__OSVersion);

        if (BI2DebugFlag && *BI2DebugFlag >= 2) {
            EnableMetroTRKInterrupts();
        }

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
}

static unsigned long __OSExceptionLocations[OS_EXCEPTION_COUNT] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800,
    0x900, 0xC00, 0xD00, 0xF00, 0x1300, 0x1400, 0x1700
};

void __OSEVStart(void);
void __OSEVSetNumber(void);
void __DBVECTOR(void);
void __OSEVEnd(void);
void __OSDBINTSTART(void);
void __OSDBINTEND(void);
void __OSDBJUMPSTART(void);
void __OSDBJUMPEND(void);

static void OSExceptionInit(void)
{
    unsigned long* location;
    unsigned long jump_size;
    __OSException exception;
    unsigned long* opcode = (unsigned long*)__OSEVSetNumber;
    unsigned long old_opcode = *opcode;
    unsigned char* handler = (unsigned char*)__OSEVStart;
    unsigned long handler_size =
        (unsigned char*)__OSEVEnd - (unsigned char*)__OSEVStart;
    void* destination = (void*)0x80000060;

    if (*(unsigned long*)destination == 0) {
        unsigned long size;

        DBPrintf("Installing OSDBIntegrator\n");
        size = (unsigned char*)__OSDBJUMPSTART -
               (unsigned char*)__OSDBINTSTART;
        memcpy(destination, (void*)__OSDBINTSTART, size);
        DCFlushRangeNoSync(destination, size);
        __sync();
        ICInvalidateRange(destination, size);
    }
    location = __OSExceptionLocations;
    jump_size = (unsigned char*)__OSDBJUMPEND -
                (unsigned char*)__OSDBJUMPSTART;
    for (exception = 0; exception < OS_EXCEPTION_COUNT; location++, exception++) {
        unsigned long offset;
        if (BI2DebugFlag && *BI2DebugFlag >= 2 &&
            __DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
            continue;
        }
        *opcode = old_opcode | exception;
        if (__DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
            memcpy((void*)__DBVECTOR, (void*)__OSDBJUMPSTART, jump_size);
        } else {
            unsigned long* db_vector = (unsigned long*)__DBVECTOR;
            for (offset = 0; offset < jump_size; offset += sizeof(*db_vector)) {
                *db_vector++ = NOP_INSTRUCTION;
            }
        }
        destination = (void*)(0x80000000 + *location);
        memcpy(destination, handler, handler_size);
        DCFlushRangeNoSync(destination, handler_size);
        __sync();
        ICInvalidateRange(destination, handler_size);
    }
    OSExceptionTable = (OSExceptionHandler*)0x80003000;
    for (exception = 0; exception < OS_EXCEPTION_COUNT; exception++) {
        __OSSetExceptionHandler(exception, OSDefaultExceptionHandler);
    }
    *opcode = old_opcode;
    DBPrintf("Exceptions initialized...\n");
}

static asm void __OSDBIntegrator(void) {
    SEQ___OSDBIntegrator();
}

static asm void __OSDBJump(void) {
    SEQ___OSDBJump();
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

static asm void OSExceptionVector(void) {
    SEQ_OSExceptionVector();
}

asm void OSDefaultExceptionHandler(__OSException exception, OSContext* context)
{
    SEQ_OSDefaultExceptionHandler();
}

asm void __OSPSInit(void)
{
    SEQ___OSPSInit();
}

unsigned long __OSGetDIConfig(void) { return __DIRegs[9] & 0xFF; }
void OSRegisterVersion(const char* version) { OSReport("%s\n", version); }
