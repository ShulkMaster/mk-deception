#include "platform/fast_rw.h"

#include "platform/display.h"

extern RwEngineInstanceType* RwEngineInstance;

void RwRenderStateSet_SRCBLEND_DESTBLEND(int srcBlend, int destBlend) {
    RwEngineInstance->fpRenderStateSet(0xa, srcBlend);
    RwEngineInstance->fpRenderStateSet(0xb, destBlend);
}

void RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(int enable) {
    RwEngineInstance->fpRenderStateSet(0xc, enable);
}

void RwRenderStateSet_rwRENDERSTATECULLMODE(int mode) {
    RwEngineInstance->fpRenderStateSet(0x14, mode);
}

void RwRenderStateSet_rwRENDERSTATEZTESTENABLE(int enable) {
    RwEngineInstance->fpRenderStateSet(0x6, enable);
}

void RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(int enable) {
    RwEngineInstance->fpRenderStateSet(0x8, enable);
}

void RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(int filter) {
    RwEngineInstance->fpRenderStateSet(0x9, filter);
}
