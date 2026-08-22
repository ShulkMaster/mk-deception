#ifndef DOLPHIN_OS_H
#define DOLPHIN_OS_H

#include "platform/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed long long OSTime;
typedef struct OSThread OSThread;
typedef struct OSResetFunctionInfo OSResetFunctionInfo;
typedef struct OSThreadQueue {
    void* head;
    void* tail;
} OSThreadQueue;
typedef signed short __OSInterrupt;
typedef struct OSContext {
    unsigned long gpr[32];
    unsigned long cr, lr, ctr, xer;
    double fpr[32];
    unsigned long fpscr_pad, fpscr, srr0, srr1;
    unsigned short mode, state;
    unsigned long gqr[8];
    unsigned long psf_pad;
    double psf[32];
} OSContext;
typedef void (*__OSInterruptHandler)(__OSInterrupt, OSContext*);
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
unsigned long long OSGetTime(void);
void OSRegisterVersion(const char* version);
void OSRegisterResetFunction(OSResetFunctionInfo* info);
#define OSPhysicalToUncached(address) ((void*)((unsigned long)(address) + 0xC0000000UL))
#define OSPhysicalToCached(address) ((void*)((unsigned long)(address) + 0x80000000UL))
#define OSUncachedToPhysical(address) ((unsigned long)(address) - 0xC0000000UL)
void OSClearContext(OSContext* context);
void OSSetCurrentContext(OSContext* context);
void OSInitThreadQueue(OSThreadQueue* queue);
void OSWakeupThread(OSThreadQueue* queue);
void OSSleepThread(OSThreadQueue* queue);
OSThread* OSGetCurrentThread(void);
int OSSuspendThread(OSThread* thread);
int OSResumeThread(OSThread* thread);
__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt,
                                             __OSInterruptHandler handler);
unsigned long __OSUnmaskInterrupts(unsigned long mask);
unsigned int OSGetResetCode(void);
unsigned int OSGetProgressiveMode(void);
void OSSetProgressiveMode(unsigned int mode);
unsigned char OSGetLanguage(void);
void OSCreateAlarm(OSAlarm* alarm);
void OSCancelAlarm(OSAlarm* alarm);
void OSSetPeriodicAlarm(OSAlarm* alarm, unsigned long long start,
                        unsigned long long period,
                        void (*handler)(OSAlarm*, void*));
int OSGetResetButtonState(void);
void OSResetSystem(int reset, int reset_code, int force_menu);

#ifdef __cplusplus
}
#endif

#endif
