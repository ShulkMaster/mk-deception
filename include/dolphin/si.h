#ifndef DOLPHIN_SI_H
#define DOLPHIN_SI_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned int SIProbe(int channel);
void SISetSamplingRate(unsigned int rate);

#ifdef __cplusplus
}
#endif

#endif
