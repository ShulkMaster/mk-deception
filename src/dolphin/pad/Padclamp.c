#include "dolphin/pad.h"
#include "math.h"

typedef struct PADClampRegion {
    unsigned char minTrigger;
    unsigned char maxTrigger;
    signed char minStick;
    signed char maxStick;
    signed char xyStick;
    signed char minSubstick;
    signed char maxSubstick;
    signed char xySubstick;
    signed char radStick;
    signed char radSubstick;
} PADClampRegion;

static const PADClampRegion ClampRegion = {
    30, 180, 15, 72, 40, 15, 59, 31, 56, 44,
};

static void ClampCircle(signed char* px, signed char* py, signed char radius, signed char min)
{
    int x = *px;
    int y = *py;
    int squared;
    int length;

    if (-min < x && x < min) {
        x = 0;
    } else if (x > 0) {
        x -= min;
    } else {
        x += min;
    }

    if (-min < y && y < min) {
        y = 0;
    } else if (y > 0) {
        y -= min;
    } else {
        y += min;
    }

    squared = x * x + y * y;
    if (radius * radius < squared) {
        length = sqrtf(squared);
        x = x * radius / length;
        y = y * radius / length;
    }

    *px = x;
    *py = y;
}

static inline void ClampTrigger(unsigned char* trigger, unsigned char min, unsigned char max)
{
    if (*trigger <= min) {
        *trigger = 0;
    } else {
        if (max < *trigger) {
            *trigger = max;
        }
        *trigger -= min;
    }
}

void PADClampCircle(PADStatus* status)
{
    int i;

    for (i = 0; i < 4; ++i, ++status) {
        if (status->err == 0) {
            ClampCircle(&status->stickX, &status->stickY, ClampRegion.radStick, ClampRegion.minStick);
            ClampCircle(&status->substickX, &status->substickY, ClampRegion.radSubstick, ClampRegion.minSubstick);
            ClampTrigger(&status->triggerLeft, ClampRegion.minTrigger, ClampRegion.maxTrigger);
            ClampTrigger(&status->triggerRight, ClampRegion.minTrigger, ClampRegion.maxTrigger);
        }
    }
}
