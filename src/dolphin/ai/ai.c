#include "dolphin/ai.h"
#include "dolphin/os.h"

#ifdef __MWERKS__
#define AI_AT_ADDRESS(address) : (address)
volatile unsigned short DSP_REGS[] AI_AT_ADDRESS(0xCC005000);
volatile unsigned long AI_REGS[8] AI_AT_ADDRESS(0xCC006C00);
#else
#define DSP_REGS ((volatile unsigned short*)0xCC005000)
#define AI_REGS ((volatile unsigned long*)0xCC006C00)
#endif
#define AI_STREAM_STOP 0
#define AI_STREAM_START 1
#define AI_SAMPLERATE_32KHZ 0
#define AI_SAMPLERATE_48KHZ 1
#define GET_REG_FIELD(reg, size, shift) \
    (((reg) >> (shift)) & ((1UL << (size)) - 1))
#define SET_REG_FIELD(reg, size, shift, value) \
    ((reg) = ((reg) & ~(((1UL << (size)) - 1) << (shift))) | \
             ((unsigned long)(value) << (shift)))

const char* __AIVersion =
    "<< Dolphin SDK - AI\trelease build: Apr  5 2004 04:15:02 (0x2301) >>";

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

static void __AI_set_stream_sample_rate(unsigned long rate);
static void __AIDHandler(__OSInterrupt interrupt, OSContext* context);
static void __AISHandler(__OSInterrupt interrupt, OSContext* context);
static void __AICallbackStackSwitch(AIDCallback callback);
static void __AI_SRC_INIT(void);

AIDCallback AIRegisterDMACallback(AIDCallback callback)
{
    AIDCallback old_callback;
    int old;

    old_callback = __AID_Callback;
    old = OSDisableInterrupts();
    __AID_Callback = callback;
    OSRestoreInterrupts(old);
    return old_callback;
}

void AIInitDMA(unsigned long start_address, unsigned long length)
{
    int old;

    old = OSDisableInterrupts();
    DSP_REGS[24] =
        (DSP_REGS[24] & 0xFFFFFC00) | (start_address >> 16);
    DSP_REGS[25] =
        (DSP_REGS[25] & 0xFFFF001F) | (start_address & 0xFFFF);
    DSP_REGS[27] =
        (DSP_REGS[27] & 0xFFFF8000) | ((length >> 5) & 0xFFFF);
    OSRestoreInterrupts(old);
}

void AIStartDMA(void)
{
    DSP_REGS[27] = DSP_REGS[27] | 0x8000;
}

void AISetStreamPlayState(unsigned long state)
{
    int old;
    unsigned char vol_left;
    unsigned char vol_right;

    if (state != AIGetStreamPlayState()) {
        if (AIGetStreamSampleRate() == 0 && state == AI_STREAM_START) {
            vol_left = AIGetStreamVolRight();
            vol_right = AIGetStreamVolLeft();
            AISetStreamVolRight(0);
            AISetStreamVolLeft(0);
            old = OSDisableInterrupts();
            __AI_SRC_INIT();
            SET_REG_FIELD(AI_REGS[0], 1, 5, 1);
            SET_REG_FIELD(AI_REGS[0], 1, 0, AI_STREAM_START);
            OSRestoreInterrupts(old);
            AISetStreamVolLeft(vol_left);
            AISetStreamVolRight(vol_right);
            return;
        }
        SET_REG_FIELD(AI_REGS[0], 1, 0, state);
    }
}

unsigned long AIGetStreamPlayState(void)
{
    return AI_REGS[0] & 1;
}

void AISetDSPSampleRate(unsigned long rate)
{
    int old;
    unsigned long play_state;
    unsigned long stream_rate;
    unsigned char vol_left;
    unsigned char vol_right;

    if (rate != AIGetDSPSampleRate()) {
        AI_REGS[0] &= 0xFFFFFFBF;
        if (rate == AI_SAMPLERATE_32KHZ) {
            vol_left = AIGetStreamVolLeft();
            vol_right = AIGetStreamVolRight();
            play_state = AIGetStreamPlayState();
            stream_rate = AIGetStreamSampleRate();
            AISetStreamVolLeft(0);
            AISetStreamVolRight(0);
            old = OSDisableInterrupts();
            __AI_SRC_INIT();
            SET_REG_FIELD(AI_REGS[0], 1, 5, 1);
            SET_REG_FIELD(AI_REGS[0], 1, 1, stream_rate);
            SET_REG_FIELD(AI_REGS[0], 1, 0, play_state);
            AI_REGS[0] |= 0x40;
            OSRestoreInterrupts(old);
            AISetStreamVolLeft(vol_left);
            AISetStreamVolRight(vol_right);
        }
    }
}

unsigned long AIGetDSPSampleRate(void)
{
    return GET_REG_FIELD(AI_REGS[0], 1, 6) ^ 1;
}

static void __AI_set_stream_sample_rate(unsigned long rate)
{
    int old;
    unsigned long play_state;
    unsigned char vol_left;
    unsigned char vol_right;
    unsigned long dsp_state;

    if (rate != AIGetStreamSampleRate()) {
        play_state = AIGetStreamPlayState();
        vol_left = AIGetStreamVolLeft();
        vol_right = AIGetStreamVolRight();
        AISetStreamVolRight(0);
        AISetStreamVolLeft(0);
        dsp_state = AI_REGS[0] & 0x40;
        SET_REG_FIELD(AI_REGS[0], 1, 6, 0);
        old = OSDisableInterrupts();
        __AI_SRC_INIT();
        AI_REGS[0] |= dsp_state;
        SET_REG_FIELD(AI_REGS[0], 1, 5, 1);
        SET_REG_FIELD(AI_REGS[0], 1, 1, rate);
        OSRestoreInterrupts(old);
        AISetStreamPlayState(play_state);
        AISetStreamVolLeft(vol_left);
        AISetStreamVolRight(vol_right);
    }
}

unsigned long AIGetStreamSampleRate(void)
{
    return GET_REG_FIELD(AI_REGS[0], 1, 1);
}

void AISetStreamVolLeft(unsigned char volume)
{
    SET_REG_FIELD(AI_REGS[1], 8, 0, volume);
}

unsigned char AIGetStreamVolLeft(void)
{
    return GET_REG_FIELD(AI_REGS[1], 8, 0);
}

void AISetStreamVolRight(unsigned char volume)
{
    SET_REG_FIELD(AI_REGS[1], 8, 8, volume);
}

unsigned char AIGetStreamVolRight(void)
{
    return GET_REG_FIELD(AI_REGS[1], 8, 8);
}

void AIInit(void* callback_stack)
{
    unsigned long timer_scale;

    if (__AI_init_flag != 1) {
        OSRegisterVersion(__AIVersion);
        timer_scale = OS_TIMER_CLOCK / 125000;
        bound_32KHz = (31524 * timer_scale) / 8000;
        bound_48KHz = (42024 * timer_scale) / 8000;
        min_wait = (42000 * timer_scale) / 8000;
        max_wait = (63000 * timer_scale) / 8000;
        buffer = (3000 * timer_scale) / 8000;
        AISetStreamVolRight(0);
        AISetStreamVolLeft(0);
        AI_REGS[3] = 0;
        SET_REG_FIELD(AI_REGS[0], 1, 5, 1);
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
}

static void __AISHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSContext exception_context;

    AI_REGS[0] |= 8;
    OSClearContext(&exception_context);
    OSSetCurrentContext(&exception_context);
    if (__AIS_Callback != 0) {
        __AIS_Callback(AI_REGS[2]);
    }
    OSClearContext(&exception_context);
    OSSetCurrentContext(context);
}

static void __AIDHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSContext exception_context;
    unsigned short control;

    control = DSP_REGS[5];
    control = (control & ~0xA0) | 8;
    DSP_REGS[5] = control;
    OSClearContext(&exception_context);
    OSSetCurrentContext(&exception_context);
    if (__AID_Callback != 0 && __AID_Active == 0) {
        __AID_Active = 1;
        if (__CallbackStack != 0) {
            __AICallbackStackSwitch(__AID_Callback);
        } else {
            __AID_Callback();
        }
        __AID_Active = 0;
    }
    OSClearContext(&exception_context);
    OSSetCurrentContext(context);
}

static void __AICallbackStackSwitch(AIDCallback callback)
{
    /* Retail invokes this callback on __CallbackStack. Portable C cannot
     * express a stack-pointer swap, so this boundary intentionally preserves
     * the callback behavior without importing the SDK's embedded assembly. */
    callback();
}

static void __AI_SRC_INIT(void)
{
    OSTime rising_32khz = 0;
    OSTime rising_48khz = 0;
    OSTime difference = 0;
    OSTime temp;
    unsigned long sample_32khz;
    unsigned long sample_48khz;
    unsigned long done = 0;
    temp = 0;

    while (!done) {
        SET_REG_FIELD(AI_REGS[0], 1, 5, 1);
        SET_REG_FIELD(AI_REGS[0], 1, 1, 0);
        SET_REG_FIELD(AI_REGS[0], 1, 0, AI_STREAM_START);
        sample_32khz = AI_REGS[2];
        while (sample_32khz == AI_REGS[2]) {}
        rising_32khz = OSGetTime();
        SET_REG_FIELD(AI_REGS[0], 1, 1, 1);
        SET_REG_FIELD(AI_REGS[0], 1, 0, AI_STREAM_START);
        sample_48khz = AI_REGS[2];
        while (sample_48khz == AI_REGS[2]) {}
        rising_48khz = OSGetTime();
        difference = rising_48khz - rising_32khz;
        SET_REG_FIELD(AI_REGS[0], 1, 1, 0);
        SET_REG_FIELD(AI_REGS[0], 1, 0, AI_STREAM_STOP);
        if (difference < bound_32KHz - buffer) {
            temp = min_wait;
            done = 1;
        } else if (difference >= bound_32KHz + buffer &&
                   difference < bound_48KHz - buffer) {
            temp = max_wait;
            done = 1;
        } else {
            done = 0;
        }
    }
    while (rising_48khz + temp > OSGetTime()) {}
}
