#ifndef DOLPHIN_DSP_H
#define DOLPHIN_DSP_H

#include "dolphin/card.h"

unsigned long DSPCheckMailToDSP(void);
void DSPSendMailToDSP(unsigned long mail);
void DSPInit(void);
DSPTaskInfo* DSPAddTask(DSPTaskInfo* task);

#endif
