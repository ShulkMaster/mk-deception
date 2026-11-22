#ifndef DOLPHIN_DSP_H
#define DOLPHIN_DSP_H

#include "dolphin/types.h"
#include "dolphin/os.h"

typedef struct DSPTaskInfo DSPTaskInfo;
typedef void (*DSPCallback)(DSPTaskInfo* task);

struct DSPTaskInfo {
    volatile u32 state;
    volatile u32 priority;
    volatile u32 flags;
    unsigned short* iramMemoryAddress;
    u32 iramLength;
    u32 iramAddress;
    unsigned short* dramMemoryAddress;
    u32 dramLength;
    u32 dramAddress;
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

u32 DSPCheckMailToDSP(void);
u32 DSPCheckMailFromDSP(void);
u32 DSPReadMailFromDSP(void);
void DSPSendMailToDSP(u32 mail);
void DSPAssertInt(void);
void DSPInit(void);
int DSPCheckInit(void);
DSPTaskInfo* DSPAddTask(DSPTaskInfo* task);
DSPTaskInfo* DSPAssertTask(DSPTaskInfo* task);
void __DSP_debug_printf(const char* format, ...);

#endif
