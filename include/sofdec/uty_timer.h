#ifndef MKD_SOFDEC_UTY_TIMER_H
#define MKD_SOFDEC_UTY_TIMER_H

#include "dolphin/types.h"

u64 UTY_GetTmr(void);
u64 UTY_GetTmrUnit(void);
s32 UTY_IsTmrVoid(void);
void UTY_InitTmr(s32 channel);
void UTY_FinishTmr(void);

#endif
