#include "dolphin/ai.h"
#include "dolphin/ax.h"
#include "dolphin/ax_internal.h"
#include "dolphin/cache.h"
#include "dolphin/dsp.h"
#include "dolphin/os.h"

#define ASSERTLINE(line, condition) ((void)0)

static s16 __AXOutBuffer[3][320];
static s32 __AXOutSBuffer[160];
static u16 __AXDramImage[8192];
static DSPTaskInfo __AXDSPTask;
AXPROFILE __AXLocalProfile;

/* Shared by the AI interrupt callback and DSP task callbacks. */
static volatile u32 __AXOutFrame;
static volatile u32 __AXAiDmaFrame;
static volatile u32 __AXOutDspReady;
static volatile OSTime __AXOsTime;
static AXCallback __AXUserFrameCallback;
static volatile int __AXDSPInitFlag;
static int __AXDSPDoneFlag;

static volatile u32 __AXDebugSteppingMode;
static OSThreadQueue __AXOutThreadQueue;
static u32 __AXOutputBufferMode;

// prototypes
static void __AXDSPInitCallback(DSPTaskInfo* task);
static void __AXDSPResumeCallback(DSPTaskInfo* task);
static void __AXDSPDoneCallback(DSPTaskInfo* task);

void __AXOutNewFrame(u32 lessDspCycles) {
    u32 cl;
    AXPROFILE* profile;
    u8* src;
    u8* dest;
    u32 i;

    __AXLocalProfile.axFrameStart = OSGetTime();
    __AXSyncPBs(lessDspCycles);
    __AXPrintStudio();
    cl = __AXGetCommandListAddress();

    DSPSendMailToDSP(0xBABE0180);
    do {} while (DSPCheckMailToDSP() != 0);

    DSPSendMailToDSP(cl);
    do {} while (DSPCheckMailToDSP() != 0);

    __AXServiceCallbackStack();
    __AXLocalProfile.auxProcessingStart = OSGetTime();
    __AXProcessAux();
    __AXLocalProfile.auxProcessingEnd = OSGetTime();
    __AXLocalProfile.userCallbackStart = OSGetTime();

    if (__AXUserFrameCallback) {
        __AXUserFrameCallback();
    }

    __AXLocalProfile.userCallbackEnd = OSGetTime();
    __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[__AXOutFrame][0]);
    __AXOutFrame += 1;

    if (__AXOutputBufferMode == 1) {
        __AXOutFrame %= 3;
    } else {
        __AXOutFrame &= 1;
        AIInitDMA((u32)&__AXOutBuffer[__AXOutFrame][0], 0x280);
    }

    __AXLocalProfile.axFrameEnd = OSGetTime();
    __AXLocalProfile.axNumVoices = __AXGetNumVoices();
    profile = __AXGetCurrentProfile();

    if (profile) {
        i = 56;
        dest = (u8*)profile;
        src = (u8*)&__AXLocalProfile;

        while (i != 0) {
            *dest = *src;
            dest++;
            src++;
            i--;
        }
    }
}

void __AXOutAiCallback(void) {
    if (__AXOutDspReady == 0) {
        __AXOsTime = OSGetTime();
    }

    if (__AXOutDspReady == 1) {
        __AXOutDspReady = 0;
        __AXOutNewFrame(0);
    } else {
        __AXOutDspReady = 2;
        DSPAssertTask(&__AXDSPTask);
    }

    if (__AXOutputBufferMode == 1) {
        AIInitDMA((u32)__AXOutBuffer[__AXAiDmaFrame], 0x280);
        __AXAiDmaFrame++;
        __AXAiDmaFrame %= 3;
    }
}

static void __AXDSPInitCallback(DSPTaskInfo* task) {
    __AXDSPInitFlag = 1;
}

static void __AXDSPResumeCallback(DSPTaskInfo* task) {
#if DEBUG
    if (__AXDebugSteppingMode != 0) {
        __AXOutDspReady = 1;
        return;
    }
#endif

    if (__AXOutDspReady == 2) {
        __AXOutDspReady = 0;
        __AXOutNewFrame((u32)(OSGetTime() - __AXOsTime) / 4);
        return;
    }
    __AXOutDspReady = 1;
}

static void __AXDSPDoneCallback(DSPTaskInfo* task) {
    __AXDSPDoneFlag = 1;
    OSWakeupThread(&__AXOutThreadQueue);
}

#define BUFFER_MEMSET(buffer, size)    \
    {                                  \
        u32* p = (u32*)&buffer;        \
        int i;                         \
        for (i = 0; i < size; i++) {   \
            *p = 0;                    \
            p++;                       \
        }                              \
    }

void __AXOutInitDSP(void) {
    __AXDSPTask.iramMemoryAddress = axDspSlave;
    __AXDSPTask.iramLength = axDspSlaveLength;
    __AXDSPTask.iramAddress = 0;
    __AXDSPTask.dramMemoryAddress = __AXDramImage;
    __AXDSPTask.dramLength = 0x2000;
    __AXDSPTask.dramAddress = 0;
    __AXDSPTask.initVector = 0x10;
    __AXDSPTask.resumeVector = 0x30;
    __AXDSPTask.initCallback = __AXDSPInitCallback;
    __AXDSPTask.resumeCallback = __AXDSPResumeCallback;
    __AXDSPTask.doneCallback = __AXDSPDoneCallback;
    __AXDSPTask.requestCallback = 0;
    __AXDSPTask.priority = 0;
    __AXDSPInitFlag = 0;
    __AXDSPDoneFlag = 0;

    OSInitThreadQueue(&__AXOutThreadQueue);
    if (DSPCheckInit() == 0) {
        DSPInit();
    }

    DSPAddTask(&__AXDSPTask);
    do {} while (__AXDSPInitFlag == 0);
}

void __AXOutInit(u32 outputBufferMode) {
#ifdef DEBUG
    OSReport("Initializing AXOut code module\n");
#endif
    ASSERTLINE(404, ((u32)&__AXOutBuffer[0][0] & 0x1F) == 0);
    ASSERTLINE(405, ((u32)&__AXOutBuffer[1][0] & 0x1F) == 0);
    ASSERTLINE(406, ((u32)&__AXOutBuffer[2][0] & 0x1F) == 0);
    ASSERTLINE(407, ((u32)&__AXOutSBuffer[0] & 0x1F) == 0);

    __AXOutputBufferMode = outputBufferMode;
    __AXOutFrame = 0;
    __AXAiDmaFrame = 0;
    __AXDebugSteppingMode = 0;

    BUFFER_MEMSET(__AXOutBuffer, 0x1E0);
    DCFlushRange(__AXOutBuffer, sizeof(__AXOutBuffer));

    BUFFER_MEMSET(__AXOutSBuffer, 0xA0);
    DCFlushRange(__AXOutSBuffer, sizeof(__AXOutSBuffer));

    __AXOutInitDSP();
    AIRegisterDMACallback(__AXOutAiCallback);

    if (__AXOutputBufferMode == 1) {
        __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[2][0]);
    } else {
        __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[1][0]);
    }

    __AXOutDspReady = 1;
    __AXUserFrameCallback = 0;

    if (__AXOutputBufferMode == 1) {
        AIInitDMA((u32)&__AXOutBuffer[__AXAiDmaFrame][0], sizeof(__AXOutBuffer[0]));
        __AXAiDmaFrame++;
        __AXAiDmaFrame &= 1;
    } else {
        AIInitDMA((u32)&__AXOutBuffer[__AXOutFrame][0], sizeof(__AXOutBuffer[0]));
    }

    AIStartDMA();
}

AXCallback AXRegisterCallback(AXCallback callback) {
    BOOL enabled;
    AXCallback oldCB;

    oldCB = __AXUserFrameCallback;
    enabled = OSDisableInterrupts();
    __AXUserFrameCallback = callback;

    OSRestoreInterrupts(enabled);
    return oldCB;
}
