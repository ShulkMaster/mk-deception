#include "dolphin/ai.h"
#include "dolphin/os.h"

#define DSP_REGS ((volatile unsigned short*)0xCC005000)
#define AI_REGS ((volatile unsigned long*)0xCC006C00)
#define AI_STREAM_START 1
#define AI_SAMPLERATE_32KHZ 0
#define AI_SAMPLERATE_48KHZ 1

static AISCallback __AIS_Callback;
static AIDCallback __AID_Callback;
static unsigned char* __CallbackStack;
static unsigned char* __OldStack;
static int __AI_init_flag;
static int __AID_Active;
static OSTime bound_32KHz;
static OSTime bound_48KHz;
static OSTime min_wait;
static OSTime max_wait;
static OSTime buffer;

static const char* __AIVersion =
    "<< Dolphin SDK - AI\trelease build: Apr  5 2004 04:15:02 (0x2301) >>";

static void __AI_set_stream_sample_rate(unsigned long rate);
static void __AI_SRC_INIT(void);
static void __AISHandler(__OSInterrupt interrupt, OSContext* context);
static void __AIDHandler(__OSInterrupt interrupt, OSContext* context);

AIDCallback AIRegisterDMACallback(AIDCallback callback)
{
    AIDCallback previous = __AID_Callback;
    int enabled = OSDisableInterrupts();
    __AID_Callback = callback;
    OSRestoreInterrupts(enabled);
    return previous;
}

void AIInitDMA(unsigned long start_address, unsigned long length)
{
    int enabled = OSDisableInterrupts();
    DSP_REGS[24] = (DSP_REGS[24] & 0xFFFFFC00) | (start_address >> 16);
    DSP_REGS[25] = (DSP_REGS[25] & 0xFFFF001F) | (start_address & 0xFFFF);
    DSP_REGS[27] = (DSP_REGS[27] & 0xFFFF8000) |
                   ((length >> 5) & 0xFFFF);
    OSRestoreInterrupts(enabled);
}

void AIStartDMA(void)
{
    DSP_REGS[27] = DSP_REGS[27] | 0x8000;
}

unsigned long AIGetStreamPlayState(void)
{
    return AI_REGS[0] & 1;
}

unsigned long AIGetStreamSampleRate(void)
{
    return (AI_REGS[0] >> 1) & 1;
}

void AISetStreamVolLeft(unsigned char volume)
{
    AI_REGS[1] = (AI_REGS[1] & ~0xFF) | volume;
}

unsigned char AIGetStreamVolLeft(void)
{
    return AI_REGS[1] & 0xFF;
}

void AISetStreamVolRight(unsigned char volume)
{
    AI_REGS[1] = (AI_REGS[1] & ~0xFF00) | (volume << 8);
}

unsigned char AIGetStreamVolRight(void)
{
    return (AI_REGS[1] >> 8) & 0xFF;
}

void AISetStreamPlayState(unsigned long state)
{
    int enabled;
    unsigned char left;
    unsigned char right;

    if (state == AIGetStreamPlayState()) return;
    if (AIGetStreamSampleRate() == 0 && state == AI_STREAM_START) {
        left = AIGetStreamVolRight();
        right = AIGetStreamVolLeft();
        AISetStreamVolRight(0);
        AISetStreamVolLeft(0);
        enabled = OSDisableInterrupts();
        __AI_SRC_INIT();
        AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
        AI_REGS[0] = (AI_REGS[0] & ~1) | AI_STREAM_START;
        OSRestoreInterrupts(enabled);
        AISetStreamVolLeft(left);
        AISetStreamVolRight(right);
        return;
    }
    AI_REGS[0] = (AI_REGS[0] & ~1) | (state & 1);
}

unsigned long AIGetDSPSampleRate(void)
{
    return ((AI_REGS[0] >> 6) & 1) ^ 1;
}

void AISetDSPSampleRate(unsigned long rate)
{
    unsigned long play_state;
    unsigned long stream_rate;
    unsigned char left;
    unsigned char right;
    int enabled;

    if (rate == AIGetDSPSampleRate()) return;
    AI_REGS[0] &= ~0x40;
    if (rate == AI_SAMPLERATE_32KHZ) {
        left = AIGetStreamVolLeft();
        right = AIGetStreamVolRight();
        play_state = AIGetStreamPlayState();
        stream_rate = AIGetStreamSampleRate();
        AISetStreamVolLeft(0);
        AISetStreamVolRight(0);
        enabled = OSDisableInterrupts();
        __AI_SRC_INIT();
        AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
        AI_REGS[0] = (AI_REGS[0] & ~2) | (stream_rate << 1);
        AI_REGS[0] = (AI_REGS[0] & ~1) | play_state;
        AI_REGS[0] |= 0x40;
        OSRestoreInterrupts(enabled);
        AISetStreamVolLeft(left);
        AISetStreamVolRight(right);
    }
}

static void __AI_set_stream_sample_rate(unsigned long rate)
{
    unsigned long play_state;
    unsigned long dsp_state;
    unsigned char left;
    unsigned char right;
    int enabled;

    if (rate == AIGetStreamSampleRate()) return;
    play_state = AIGetStreamPlayState();
    left = AIGetStreamVolLeft();
    right = AIGetStreamVolRight();
    AISetStreamVolRight(0);
    AISetStreamVolLeft(0);
    dsp_state = AI_REGS[0] & 0x40;
    AI_REGS[0] &= ~0x40;
    enabled = OSDisableInterrupts();
    __AI_SRC_INIT();
    AI_REGS[0] |= dsp_state;
    AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
    AI_REGS[0] = (AI_REGS[0] & ~2) | (rate << 1);
    OSRestoreInterrupts(enabled);
    AISetStreamPlayState(play_state);
    AISetStreamVolLeft(left);
    AISetStreamVolRight(right);
}

void AIInit(void* callback_stack)
{
    if (__AI_init_flag) return;
    OSRegisterVersion(__AIVersion);
    bound_32KHz = OSNanosecondsToTicks(31524);
    bound_48KHz = OSNanosecondsToTicks(42024);
    min_wait = OSNanosecondsToTicks(42000);
    max_wait = OSNanosecondsToTicks(63000);
    buffer = OSNanosecondsToTicks(3000);
    AISetStreamVolRight(0);
    AISetStreamVolLeft(0);
    AI_REGS[3] = 0;
    AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
    __AI_set_stream_sample_rate(AI_SAMPLERATE_48KHZ);
    AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);
    __AIS_Callback = 0;
    __AID_Callback = 0;
    __CallbackStack = callback_stack;
    __OSSetInterruptHandler(5, __AIDHandler);
    __OSUnmaskInterrupts(0x04000000);
    __OSSetInterruptHandler(8, __AISHandler);
    __OSUnmaskInterrupts(0x00800000);
    __AI_init_flag = 1;
}

static void __AISHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSContext callback_context;
    AI_REGS[0] |= 8;
    OSClearContext(&callback_context);
    OSSetCurrentContext(&callback_context);
    if (__AIS_Callback) __AIS_Callback(AI_REGS[2]);
    OSClearContext(&callback_context);
    OSSetCurrentContext(context);
}

static void __AICallbackStackSwitch(AIDCallback callback)
{
    /* Retail temporarily swaps r1 to __CallbackStack around this call. */
    callback();
}

static void __AIDHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSContext callback_context;
    unsigned short control = DSP_REGS[5];
    DSP_REGS[5] = (control & ~0xA0) | 8;
    OSClearContext(&callback_context);
    OSSetCurrentContext(&callback_context);
    if (__AID_Callback && !__AID_Active) {
        __AID_Active = 1;
        if (__CallbackStack) __AICallbackStackSwitch(__AID_Callback);
        else __AID_Callback();
        __AID_Active = 0;
    }
    OSClearContext(&callback_context);
    OSSetCurrentContext(context);
}

static void __AI_SRC_INIT(void)
{
    OSTime rising_32khz = 0;
    OSTime rising_48khz = 0;
    OSTime delay = 0;
    OSTime difference;
    unsigned long sample;
    int done = 0;

    while (!done) {
        AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
        AI_REGS[0] &= ~2;
        AI_REGS[0] = (AI_REGS[0] & ~1) | 1;
        sample = AI_REGS[2];
        while (sample == AI_REGS[2]) {}
        rising_32khz = OSGetTime();
        AI_REGS[0] |= 2;
        AI_REGS[0] = (AI_REGS[0] & ~1) | 1;
        sample = AI_REGS[2];
        while (sample == AI_REGS[2]) {}
        rising_48khz = OSGetTime();
        difference = rising_48khz - rising_32khz;
        AI_REGS[0] &= ~2;
        AI_REGS[0] &= ~1;
        if (difference < bound_32KHz - buffer) {
            delay = min_wait;
            done = 1;
        } else if (difference >= bound_32KHz + buffer &&
                   difference < bound_48KHz - buffer) {
            delay = max_wait;
            done = 1;
        }
    }
    while (rising_48khz + delay > OSGetTime()) {}
}
