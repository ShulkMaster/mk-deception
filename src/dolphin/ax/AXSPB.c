#include "dolphin/ax.h"
#include "dolphin/cache.h"

static AXSPB __AXStudio __attribute__((aligned(32)));
static signed long __AXSpbAL;
static signed long __AXSpbAR;
static signed long __AXSpbAS;
static signed long __AXSpbAAL;
static signed long __AXSpbAAR;
static signed long __AXSpbAAS;
static signed long __AXSpbABL;
static signed long __AXSpbABR;
static signed long __AXSpbABS;

unsigned long __AXGetStudio(void)
{
    return (unsigned long)&__AXStudio;
}

static inline void __AXDepopFade(signed long* host_sum,
                                 signed long* dsp_volume, short* dsp_delta)
{
    int frames = *host_sum / 160;

    if (frames) {
        signed long delta = *host_sum / 160;
        if (delta > 0x14) {
            delta = 0x14;
        }
        if (delta < -0x14) {
            delta = -0x14;
        }
        *dsp_volume = *host_sum;
        *host_sum -= delta * 0xA0;
        *dsp_delta = -delta;
        return;
    }
    *host_sum = 0;
    *dsp_volume = 0;
    *dsp_delta = 0;
}

void __AXPrintStudio(void)
{
    __AXDepopFade(&__AXSpbAL, (void*)&__AXStudio.dpopLHi,
                  &__AXStudio.dpopLDelta);
    __AXDepopFade(&__AXSpbAR, (void*)&__AXStudio.dpopRHi,
                  &__AXStudio.dpopRDelta);
    __AXDepopFade(&__AXSpbAS, (void*)&__AXStudio.dpopSHi,
                  &__AXStudio.dpopSDelta);
    __AXDepopFade(&__AXSpbAAL, (void*)&__AXStudio.dpopALHi,
                  &__AXStudio.dpopALDelta);
    __AXDepopFade(&__AXSpbAAR, (void*)&__AXStudio.dpopARHi,
                  &__AXStudio.dpopARDelta);
    __AXDepopFade(&__AXSpbAAS, (void*)&__AXStudio.dpopASHi,
                  &__AXStudio.dpopASDelta);
    __AXDepopFade(&__AXSpbABL, (void*)&__AXStudio.dpopBLHi,
                  &__AXStudio.dpopBLDelta);
    __AXDepopFade(&__AXSpbABR, (void*)&__AXStudio.dpopBRHi,
                  &__AXStudio.dpopBRDelta);
    __AXDepopFade(&__AXSpbABS, (void*)&__AXStudio.dpopBSHi,
                  &__AXStudio.dpopBSDelta);
    DCFlushRange(&__AXStudio, sizeof(__AXStudio));
}

void __AXSPBInit(void)
{
    __AXSpbAL = __AXSpbAR = __AXSpbAS = __AXSpbAAL = __AXSpbAAR =
        __AXSpbAAS = __AXSpbABL = __AXSpbABR = __AXSpbABS = 0;
}

void __AXDepopVoice(AXPB* pb)
{
    __AXSpbAL += pb->dpop.aL;
    __AXSpbAAL += pb->dpop.aAuxAL;
    __AXSpbABL += pb->dpop.aAuxBL;
    __AXSpbAR += pb->dpop.aR;
    __AXSpbAAR += pb->dpop.aAuxAR;
    __AXSpbABR += pb->dpop.aAuxBR;
    __AXSpbAS += pb->dpop.aS;
    __AXSpbAAS += pb->dpop.aAuxAS;
    __AXSpbABS += pb->dpop.aAuxBS;
}
