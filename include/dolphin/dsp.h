#ifndef DOLPHIN_DSP_H
#define DOLPHIN_DSP_H

#include "dolphin/os.h"

typedef struct DSPTaskInfo DSPTaskInfo;
typedef void (*DSPCallback)(DSPTaskInfo* task);

struct DSPTaskInfo {
    volatile unsigned long state;
    volatile unsigned long priority;
    volatile unsigned long flags;
    unsigned short* iramMemoryAddress;
    unsigned long iramLength;
    unsigned long iramAddress;
    unsigned short* dramMemoryAddress;
    unsigned long dramLength;
    unsigned long dramAddress;
    unsigned short initVector;
    unsigned short resumeVector;
    DSPCallback initCallback;
    DSPCallback resumeCallback;
    DSPCallback doneCallback;
    DSPCallback requestCallback;
    struct DSPTaskInfo* next;
    struct DSPTaskInfo* previous;
    OSTime contextTime;
    OSTime taskTime;
};

unsigned long DSPCheckMailToDSP(void);
unsigned long DSPCheckMailFromDSP(void);
unsigned long DSPReadMailFromDSP(void);
void DSPSendMailToDSP(unsigned long mail);
void DSPAssertInt(void);
void DSPInit(void);
int DSPCheckInit(void);
DSPTaskInfo* DSPAddTask(DSPTaskInfo* task);
DSPTaskInfo* DSPAssertTask(DSPTaskInfo* task);
void __DSP_debug_printf(const char* format, ...);

#endif
