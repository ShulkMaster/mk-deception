#ifndef DOLPHIN_AI_H
#define DOLPHIN_AI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AIDCallback)(void);
typedef void (*AISCallback)(unsigned long sample_count);

void AIInit(void* callback_stack);
AIDCallback AIRegisterDMACallback(AIDCallback callback);
void AIInitDMA(unsigned long start_address, unsigned long length);
void AIStartDMA(void);
void AISetStreamPlayState(unsigned long state);
unsigned long AIGetStreamPlayState(void);
void AISetDSPSampleRate(unsigned long rate);
unsigned long AIGetDSPSampleRate(void);
unsigned long AIGetStreamSampleRate(void);
void AISetStreamVolLeft(unsigned char volume);
unsigned char AIGetStreamVolLeft(void);
void AISetStreamVolRight(unsigned char volume);
unsigned char AIGetStreamVolRight(void);

#ifdef __cplusplus
}
#endif

#endif
