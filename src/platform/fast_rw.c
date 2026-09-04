#include "platform/fast_rw.h"
#include "rw/rwengine.h"

int RwRenderStateSet_SRCBLEND_DESTBLEND(int srcBlend, int destBlend) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xa, srcBlend);
    return RwEngineInstance->dOpenDevice.fpRenderStateSet(0xb, destBlend);
}

void RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(int enable) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xc, enable);
}

void RwRenderStateSet_rwRENDERSTATECULLMODE(int mode) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, mode);
}

void RwRenderStateSet_rwRENDERSTATEZTESTENABLE(int enable) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x6, enable);
}

void RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(int enable) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x8, enable);
}

void RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(int filter) {
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x9, filter);
}
