#ifndef DOLPHIN_OS_H
#define DOLPHIN_OS_H

#include "platform/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed long long OSTime;
typedef unsigned long OSTick;
typedef struct OSResetFunctionInfo OSResetFunctionInfo;
typedef struct OSContext OSContext;
typedef unsigned char __OSException;
typedef unsigned short OSError;
typedef unsigned long OSInterruptMask;
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);
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
typedef struct OSThreadQueue {
    void* head;
    void* tail;
} OSThreadQueue;
typedef signed short __OSInterrupt;
struct OSContext {
    unsigned long gpr[32];
    unsigned long cr, lr, ctr, xer;
    double fpr[32];
    unsigned long fpscr_pad, fpscr, srr0, srr1;
    unsigned short mode, state;
    unsigned long gqr[8];
    unsigned long psf_pad;
    double psf[32];
};
typedef void (*__OSInterruptHandler)(__OSInterrupt, OSContext*);
typedef void (*OSErrorHandler)(OSError error, OSContext* context, ...);
typedef int (*OSResetFunction)(int final);
struct OSResetFunctionInfo {
    OSResetFunction func;
    unsigned long priority;
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* prev;
};

#define __OSBusClock (*(volatile unsigned long*)0x800000F8)

void OSReport(const char* format, ...);
void OSInit(void);
void OSPanic(const char* file, int line, const char* format, ...);
int OSDisableInterrupts(void);
int OSRestoreInterrupts(int enabled);
unsigned short OSGetFontEncode(void);
int OSInitFont(void* font_data);
char* OSGetFontWidth(char* string, int* width);
char* OSGetFontTexture(char* string, void** image, int* x, int* y, int* width);
OSTime OSGetTime(void);
OSTick OSGetTick(void);
void OSRegisterVersion(const char* version);
void OSRegisterResetFunction(OSResetFunctionInfo* info);
#define OSPhysicalToUncached(address) ((void*)((unsigned long)(address) + 0xC0000000UL))
#define OSPhysicalToCached(address) ((void*)((unsigned long)(address) + 0x80000000UL))
#define OSUncachedToPhysical(address) ((unsigned long)(address) - 0xC0000000UL)
void OSClearContext(OSContext* context);
void OSDumpContext(OSContext* context);
void OSSetCurrentContext(OSContext* context);
void OSInitThreadQueue(OSThreadQueue* queue);
void OSWakeupThread(OSThreadQueue* queue);
void OSSleepThread(OSThreadQueue* queue);
OSThread* OSGetCurrentThread(void);
int OSSuspendThread(OSThread* thread);
int OSResumeThread(OSThread* thread);
__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt,
                                             __OSInterruptHandler handler);
OSInterruptMask __OSMaskInterrupts(OSInterruptMask mask);
OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask mask);
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler);
void __OSUnhandledException(__OSException exception, OSContext* context,
                            unsigned long cause, unsigned long address);
extern OSErrorHandler __OSErrorTable[17];
unsigned int OSGetResetCode(void);
unsigned int OSGetProgressiveMode(void);
void OSSetProgressiveMode(unsigned int mode);
unsigned char OSGetLanguage(void);
void OSCreateAlarm(OSAlarm* alarm);
void OSCancelAlarm(OSAlarm* alarm);
void OSInitAlarm(void);
void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler);
void OSSetPeriodicAlarm(OSAlarm* alarm, unsigned long long start,
                        unsigned long long period,
                        void (*handler)(OSAlarm*, void*));
int OSGetResetButtonState(void);
void OSResetSystem(int reset, int reset_code, int force_menu);
OSSramEx* __OSLockSramEx(void);
int __OSUnlockSramEx(int commit);
OSSram* __OSLockSram(void);
int __OSUnlockSram(int commit);
#define OSRoundUp32B(value) (((unsigned long)(value) + 31) & ~31)
#define OSCachedToPhysical(address) ((unsigned long)(address) - 0x80000000UL)

#ifdef __cplusplus
}
#endif

#endif
