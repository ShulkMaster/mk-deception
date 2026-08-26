#include "dolphin/ax.h"
#include "dolphin/ax_internal.h"
#include "dolphin/cache.h"

static signed long __AXBufferAuxA[3][480] __attribute__((aligned(32)));
static signed long __AXBufferAuxB[3][480] __attribute__((aligned(32)));
static void (*__AXCallbackAuxA)(void*, void*);
static void (*__AXCallbackAuxB)(void*, void*);
static void* __AXContextAuxA;
static void* __AXContextAuxB;
static signed long* __AXAuxADspWrite;
static signed long* __AXAuxADspRead;
static signed long* __AXAuxBDspWrite;
static signed long* __AXAuxBDspRead;
static unsigned long __AXAuxDspWritePosition;
static unsigned long __AXAuxDspReadPosition;
static unsigned long __AXAuxDspWritePositionDpl2;
static unsigned long __AXAuxDspReadPositionDpl2;
static unsigned long __AXAuxCpuReadWritePosition;

void __AXAuxInit(void)
{
    int i;
    signed long* pA;
    signed long* pB;

    __AXCallbackAuxA = 0;
    __AXCallbackAuxB = 0;
    __AXContextAuxA = 0;
    __AXContextAuxB = 0;
    __AXAuxDspWritePosition = 0;
    __AXAuxDspReadPosition = 1;
    __AXAuxDspWritePositionDpl2 = 0;
    __AXAuxDspReadPositionDpl2 = 1;
    __AXAuxCpuReadWritePosition = 2;

    pA = &__AXBufferAuxA[0][0];
    pB = &__AXBufferAuxB[0][0];
    for (i = 0; i < 480; i++) {
        *pA = 0;
        pA++;
        *pB = 0;
        pB++;
    }
}

void __AXGetAuxAInput(unsigned long* p)
{
    if (__AXCallbackAuxA) {
        *p = (unsigned long)&__AXBufferAuxA[__AXAuxDspWritePosition][0];
    } else {
        *p = 0;
    }
}

void __AXGetAuxAInputDpl2(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspWritePosition][320];
}

void __AXGetAuxAOutput(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxA[__AXAuxDspReadPosition][0];
}

void __AXGetAuxAOutputDpl2R(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxA[__AXAuxDspReadPosition][160];
}

void __AXGetAuxAOutputDpl2Ls(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxA[__AXAuxDspReadPosition][320];
}

void __AXGetAuxAOutputDpl2Rs(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspReadPosition][320];
}

void __AXGetAuxBInput(unsigned long* p)
{
    if (__AXCallbackAuxB) {
        *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspWritePosition][0];
    } else {
        *p = 0;
    }
}

void __AXGetAuxBOutput(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspReadPosition][0];
}

void __AXGetAuxBForDPL2(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspWritePositionDpl2][0];
}

void __AXGetAuxBOutputDPL2(unsigned long* p)
{
    *p = (unsigned long)&__AXBufferAuxB[__AXAuxDspReadPositionDpl2][0];
}

void __AXProcessAux(void)
{
    __AXAuxADspWrite = &__AXBufferAuxA[__AXAuxDspWritePosition][0];
    __AXAuxADspRead = &__AXBufferAuxA[__AXAuxDspReadPosition][0];
    __AXAuxBDspWrite = &__AXBufferAuxB[__AXAuxDspWritePosition][0];
    __AXAuxBDspRead = &__AXBufferAuxB[__AXAuxDspReadPosition][0];

    if (__AXCallbackAuxA) {
        if (__AXClMode == 2) {
            AX_AUX_DATA_DPL2 auxData;
            auxData.l = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][0];
            auxData.r = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][160];
            auxData.ls = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][320];
            auxData.rs = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][320];
            DCInvalidateRange(auxData.l, 0x780);
            DCInvalidateRange(auxData.rs, 0x280);
            __AXCallbackAuxA(&auxData.l, __AXContextAuxA);
            DCFlushRangeNoSync(auxData.l, 0x780);
            DCFlushRangeNoSync(auxData.rs, 0x280);
        } else {
            AX_AUX_DATA auxData;
            auxData.l = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][0];
            auxData.r = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][160];
            auxData.s = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][320];
            DCInvalidateRange(auxData.l, 0x780);
            __AXCallbackAuxA(&auxData.l, __AXContextAuxA);
            DCFlushRangeNoSync(auxData.l, 0x780);
        }
    }

    if (__AXCallbackAuxB && __AXClMode != 2) {
        AX_AUX_DATA auxData;
        auxData.l = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][0];
        auxData.r = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][160];
        auxData.s = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][320];
        DCInvalidateRange(auxData.l, 0x780);
        __AXCallbackAuxB(&auxData.l, __AXContextAuxB);
        DCFlushRangeNoSync(auxData.l, 0x780);
    }

    __AXAuxDspWritePosition += 1;
    __AXAuxDspWritePosition %= 3;
    __AXAuxDspReadPosition += 1;
    __AXAuxDspReadPosition %= 3;
    __AXAuxDspWritePositionDpl2 += 1;
    __AXAuxDspWritePositionDpl2 &= 1;
    __AXAuxDspReadPositionDpl2 += 1;
    __AXAuxDspReadPositionDpl2 &= 1;
    __AXAuxCpuReadWritePosition += 1;
    __AXAuxCpuReadWritePosition %= 3;
}
