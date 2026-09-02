#ifndef MKD_SOFDEC_UTY_TIMER_H
#define MKD_SOFDEC_UTY_TIMER_H

unsigned long long UTY_GetTmr(void);
unsigned long long UTY_GetTmrUnit(void);
int UTY_IsTmrVoid(void);
void UTY_InitTmr(int channel);
void UTY_FinishTmr(void);

#endif
