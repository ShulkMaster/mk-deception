#include "dolphin/os.h"
#include "dolphin/si.h"
#include "dolphin/vi.h"

typedef struct SISamplingParameters {
    unsigned short line;
    unsigned char count;
} SISamplingParameters;

static unsigned long SamplingRate;

static SISamplingParameters XYNTSC[12] = {
    {0x00F6, 0x02}, {0x000E, 0x13}, {0x001E, 0x09}, {0x002C, 0x06},
    {0x0034, 0x05}, {0x0041, 0x04}, {0x0057, 0x03}, {0x0057, 0x03},
    {0x0057, 0x03}, {0x0083, 0x02}, {0x0083, 0x02}, {0x0083, 0x02},
};

static SISamplingParameters XYPAL[12] = {
    {0x0128, 0x02}, {0x000F, 0x15}, {0x001D, 0x0B}, {0x002D, 0x07},
    {0x0034, 0x06}, {0x003F, 0x05}, {0x004E, 0x04}, {0x0068, 0x03},
    {0x0068, 0x03}, {0x0068, 0x03}, {0x0068, 0x03}, {0x009C, 0x02},
};

void SISetSamplingRate(unsigned long milliseconds)
{
    SISamplingParameters* parameters;
    int progressive;
    int enabled;

    if (milliseconds > 11) {
        milliseconds = 11;
    }

    enabled = OSDisableInterrupts();
    SamplingRate = milliseconds;

    switch (VIGetTvFormat()) {
    case VI_NTSC:
    case VI_MPAL:
    case VI_EURGB60:
        parameters = XYNTSC;
        break;
    case VI_PAL:
        parameters = XYPAL;
        break;
    default:
        OSReport("SISetSamplingRate: unknown TV format. Use default.");
        milliseconds = 0;
        parameters = XYNTSC;
        break;
    }

    progressive = *(volatile unsigned short*)0xCC00206C & 1;
    SISetXY((progressive ? 2 : 1) * parameters[milliseconds].line,
            parameters[milliseconds].count);
    OSRestoreInterrupts(enabled);
}

void SIRefreshSamplingRate(void)
{
    SISetSamplingRate(SamplingRate);
}
