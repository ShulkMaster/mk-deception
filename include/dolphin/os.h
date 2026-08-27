#ifndef DOLPHIN_OS_H
#define DOLPHIN_OS_H

#include "platform/os_types.h"
#include "dolphin/dvd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed long long OSTime;
typedef unsigned long OSTick;
typedef struct OSResetFunctionInfo OSResetFunctionInfo;
typedef struct OSExecParams {
    int valid;
    unsigned long restart_code;
    unsigned long boot_dol;
    void* region_start;
    void* region_end;
    int args_use_default;
    void* args_address;
} OSExecParams;
typedef struct OSFontHeader {
    unsigned short fontType;
    unsigned short firstChar;
    unsigned short lastChar;
    unsigned short invalChar;
    unsigned short ascent;
    unsigned short descent;
    unsigned short width;
    unsigned short leading;
    unsigned short cellWidth;
    signed short cellHeight;
    unsigned long sheetSize;
    unsigned short sheetFormat;
    unsigned short sheetColumn;
    unsigned short sheetRow;
    unsigned short sheetWidth;
    unsigned short sheetHeight;
    unsigned short widthTable;
    unsigned long sheetImage;
    unsigned long sheetFullSize;
    unsigned char c0, c1, c2, c3;
} OSFontHeader;
typedef char OSExecParamsSizeCheck[sizeof(OSExecParams) == 0x1C ? 1 : -1];
typedef unsigned char __OSException;
typedef unsigned short OSError;
typedef unsigned long OSInterruptMask;
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);
typedef void (*OSSwitchThreadCallback)(OSThread* from, OSThread* to);
typedef struct OSSramEx {
    unsigned char flashID[2][12];
    unsigned long wirelessKeyboardID;
    unsigned short wirelessPadID[4];
    unsigned char dvdErrorCode;
    unsigned char padding0;
    unsigned char flashIDCheckSum[2];
    unsigned short gbs;
    unsigned char padding1[2];
} OSSramEx;
typedef struct OSSram {
    unsigned short checkSum;
    unsigned short checkSumInv;
    unsigned long ead0;
    unsigned long ead1;
    unsigned long counterBias;
    signed char displayOffsetH;
    unsigned char ntd;
    unsigned char language;
    unsigned char flags;
} OSSram;
typedef void* OSMessage;
typedef struct OSMessageQueue {
    OSThreadQueue queueSend;
    OSThreadQueue queueReceive;
    OSMessage* msgArray;
    signed long msgCount;
    signed long firstIndex;
    signed long usedCount;
} OSMessageQueue;
typedef char OSMessageQueueSizeCheck[sizeof(OSMessageQueue) == 0x20 ? 1 : -1];
typedef signed short __OSInterrupt;
typedef void (*__OSInterruptHandler)(__OSInterrupt, OSContext*);
typedef void (*OSExceptionHandler)(__OSException, OSContext*);
typedef void (*OSErrorHandler)(OSError error, OSContext* context, ...);
typedef int (*OSResetFunction)(int final);
struct OSResetFunctionInfo {
    OSResetFunction func;
    unsigned long priority;
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* prev;
};

typedef struct OSBootInfo {
    DVDDiskID disk_id;
    unsigned long magic;
    unsigned long version;
    unsigned long memory_size;
    unsigned long console_type;
    void* arena_lo;
    void* arena_hi;
    void* fst_location;
    unsigned long fst_max_length;
} OSBootInfo;

#define __OSBusClock (*(volatile unsigned long*)0x800000F8)
#define OS_TIMER_CLOCK (__OSBusClock / 4)
#define OSTicksToSeconds(ticks) ((ticks) / OS_TIMER_CLOCK)
#define OSSecondsToTicks(seconds) ((seconds) * OS_TIMER_CLOCK)
#define OSMillisecondsToTicks(milliseconds) \
    ((milliseconds) * (OS_TIMER_CLOCK / 1000))
#define OSMicrosecondsToTicks(microseconds) \
    (((microseconds) * (OS_TIMER_CLOCK / 125000)) / 8)
#define OSNanosecondsToTicks(nanoseconds) \
    (((nanoseconds) * (OS_TIMER_CLOCK / 125000)) / 8000)

void OSReport(const char* format, ...);
void OSInit(void);
unsigned long OSGetConsoleType(void);
void __OSInitSystemCall(void);
void __OSInitMemoryProtection(void);
void __OSModuleInit(void);
void __OSInterruptInit(void);
void __OSContextInit(void);
void __OSCacheInit(void);
void __OSInitSram(void);
void __OSThreadInit(void);
void __OSInitAudioSystem(void);
void __OSStopAudioSystem(void);
unsigned long __OSGetDIConfig(void);
void __OSReboot(unsigned long reset_code, unsigned long boot_dol);
void OSGetSaveRegion(void** start, void** end);
void __OSBootDol(unsigned long boot_dol, unsigned long reset_code, char** argv);
void __OSBootDolSimple(unsigned long boot_dol, unsigned long reset_code,
                       void* region_start, void* region_end,
                       int args_use_default, int argc, char** argv);
void __OSGetExecParams(OSExecParams* params);
void OSPanic(const char* file, int line, const char* format, ...);
int OSDisableInterrupts(void);
int OSEnableInterrupts(void);
int OSRestoreInterrupts(int enabled);
unsigned short OSGetFontEncode(void);
int OSInitFont(OSFontHeader* font_data);
char* OSGetFontWidth(const char* string, int* width);
char* OSGetFontTexture(const char* string, void** image, int* x, int* y,
                       int* width);
OSTime OSGetTime(void);
OSTick OSGetTick(void);
OSTime __OSGetSystemTime(void);
OSTime __OSTimeToSystemTime(OSTime time);
void OSRegisterVersion(const char* version);
void OSRegisterResetFunction(OSResetFunctionInfo* info);
#define OSPhysicalToUncached(address) ((void*)((unsigned long)(address) + 0xC0000000UL))
#define OSPhysicalToCached(address) ((void*)((unsigned long)(address) + 0x80000000UL))
#define OSUncachedToPhysical(address) ((unsigned long)(address) - 0xC0000000UL)
void OSClearContext(OSContext* context);
void OSDumpContext(OSContext* context);
void OSSetCurrentContext(OSContext* context);
OSContext* OSGetCurrentContext(void);
unsigned long OSSaveContext(OSContext* context);
void OSSaveFPUContext(OSContext* context);
void OSLoadContext(OSContext* context);
void* OSGetStackPointer(void);
void OSInitContext(OSContext* context, unsigned long program_counter,
                   unsigned long stack_pointer);
void OSInitThreadQueue(OSThreadQueue* queue);
void OSWakeupThread(OSThreadQueue* queue);
void OSSleepThread(OSThreadQueue* queue);
void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* messages, signed long count);
int OSSendMessage(OSMessageQueue* queue, OSMessage message, signed long flags);
int OSReceiveMessage(OSMessageQueue* queue, OSMessage* message, signed long flags);
int OSJamMessage(OSMessageQueue* queue, OSMessage message, signed long flags);
OSThread* OSGetCurrentThread(void);
int OSSuspendThread(OSThread* thread);
int OSResumeThread(OSThread* thread);
int OSDisableScheduler(void);
int OSEnableScheduler(void);
void __OSReschedule(void);
void OSYieldThread(void);
int OSCreateThread(OSThread* thread, void* (*entry)(void*), void* parameter,
                   void* stack, unsigned long stack_size,
                   signed long priority, unsigned short attributes);
void OSExitThread(void* value);
void OSCancelThread(OSThread* thread);
int OSJoinThread(OSThread* thread, void** value);
int OSSetThreadPriority(OSThread* thread, signed long priority);
signed long OSGetThreadPriority(OSThread* thread);
void OSClearStack(unsigned char value);
void __OSPromoteThread(OSThread* thread, signed long priority);
signed long __OSGetEffectivePriority(OSThread* thread);
void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
void __OSUnlockAllMutex(OSThread* thread);
int OSTryLockMutex(OSMutex* mutex);
void OSInitCond(OSCond* condition);
void OSWaitCond(OSCond* condition, OSMutex* mutex);
void OSSignalCond(OSCond* condition);
__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt,
                                             __OSInterruptHandler handler);
__OSInterruptHandler __OSGetInterruptHandler(__OSInterrupt interrupt);
OSExceptionHandler __OSGetExceptionHandler(__OSException exception);
OSExceptionHandler __OSSetExceptionHandler(__OSException exception,
                                           OSExceptionHandler handler);
OSInterruptMask __OSMaskInterrupts(OSInterruptMask mask);
OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask mask);
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler);
void __OSUnhandledException(__OSException exception, OSContext* context,
                            unsigned long cause, unsigned long address);
extern OSErrorHandler __OSErrorTable[17];
extern unsigned short __OSDeviceCode;
unsigned int OSGetResetCode(void);
unsigned int OSGetProgressiveMode(void);
void OSSetProgressiveMode(unsigned int mode);
unsigned char OSGetLanguage(void);
unsigned short OSGetWirelessID(signed long channel);
void OSSetWirelessID(signed long channel, unsigned short id);
unsigned short OSGetGbsMode(void);
void OSSetGbsMode(unsigned short mode);
int __OSSyncSram(void);
int __OSReadROM(void* buffer, signed long length, signed long offset);
void OSCreateAlarm(OSAlarm* alarm);
void OSCancelAlarm(OSAlarm* alarm);
void OSInitAlarm(void);
void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler);
void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period,
                        OSAlarmHandler handler);
int OSGetResetButtonState(void);
void __OSResetSWInterruptHandler(__OSInterrupt interrupt, OSContext* context);
void OSResetSystem(int reset, unsigned long reset_code, int force_menu);
int __OSCallResetFunctions(int final);
void __OSDoHotReset(unsigned long reset_code);
OSSramEx* __OSLockSramEx(void);
int __OSUnlockSramEx(int commit);
OSSram* __OSLockSram(void);
int __OSUnlockSram(int commit);
#define OSRoundUp32B(value) (((unsigned long)(value) + 31) & ~31)
#define OSRoundDown32B(value) ((unsigned long)(value) & ~31)
#define OSCachedToPhysical(address) ((unsigned long)(address) - 0x80000000UL)

#ifdef __cplusplus
}
#endif

#endif
