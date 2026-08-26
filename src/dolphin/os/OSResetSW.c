#include "dolphin/os.h"

typedef void (*OSResetCallback)(void);

static OSResetCallback ResetCallback;
static int Down;
static int LastState;
static OSTime HoldUp;
static OSTime HoldDown;

volatile unsigned long __PIRegs[] : 0xCC003000;
unsigned char __gUnknown800030E3 : 0x800030E3;
extern OSTime __OSStartTime;

void __OSResetSWInterruptHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSResetCallback callback;
    OSTime debounce_ticks;

    (void)interrupt;
    (void)context;
    HoldDown = __OSGetSystemTime();
    debounce_ticks = OSMicrosecondsToTicks(100);
    while (__OSGetSystemTime() - HoldDown < debounce_ticks &&
           !(__PIRegs[0] & 0x00010000)) {}

    if (!(__PIRegs[0] & 0x00010000)) {
        LastState = Down = 1;
        __OSMaskInterrupts(0x200);
        if (ResetCallback != 0) {
            callback = ResetCallback;
            ResetCallback = 0;
            callback();
        }
    }
    __PIRegs[0] = 2;
}

int OSGetResetButtonState(void)
{
    int enabled;
    int state;
    unsigned long reg;
    OSTime now;

    enabled = OSDisableInterrupts();
    now = __OSGetSystemTime();
    reg = __PIRegs[0];
    if (!(reg & 0x00010000)) {
        if (!Down) {
            Down = 1;
            state = HoldUp ? 1 : 0;
            HoldDown = now;
        } else {
            state = (HoldUp || (OSMicrosecondsToTicks(100) < now - HoldDown)) ? 1 : 0;
        }
    } else if (Down) {
        Down = 0;
        state = LastState;
        if (state) {
            HoldUp = now;
        } else {
            HoldUp = 0;
        }
    } else if (HoldUp && (now - HoldUp < OSMillisecondsToTicks(40))) {
        state = 1;
    } else {
        state = 0;
        HoldUp = 0;
    }

    LastState = state;
    if (__gUnknown800030E3 & 0x1F) {
        unsigned long timer_clock;
        OSTime fire;

        fire = (__gUnknown800030E3 & 0x1F) * 60;
        timer_clock = OS_TIMER_CLOCK;
        fire = __OSStartTime + fire * timer_clock;
        if (fire < now) {
            now -= fire;
            now = (now / timer_clock) / 2;
            if ((now & 1) == 0) {
                state = 1;
            } else {
                state = 0;
            }
        }
    }

    OSRestoreInterrupts(enabled);
    return state;
}
