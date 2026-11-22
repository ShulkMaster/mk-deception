#ifndef DOLPHIN_AI_H
#define DOLPHIN_AI_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AIDCallback)(void);
typedef void (*AISCallback)(u32 sample_count);

void AIInit(void* callback_stack);
AIDCallback AIRegisterDMACallback(AIDCallback callback);
void AIInitDMA(u32 start_address, u32 length);
void AIStartDMA(void);
void AISetStreamPlayState(u32 state);
u32 AIGetStreamPlayState(void);
void AISetDSPSampleRate(u32 rate);
u32 AIGetDSPSampleRate(void);
u32 AIGetStreamSampleRate(void);
void AISetStreamVolLeft(unsigned char volume);
unsigned char AIGetStreamVolLeft(void);
void AISetStreamVolRight(unsigned char volume);
unsigned char AIGetStreamVolRight(void);

#ifdef __cplusplus
}
#endif

#endif
