#ifndef DOLPHIN_PRIVATE_DSP_H
#define DOLPHIN_PRIVATE_DSP_H

#include "dolphin/dsp.h"

extern DSPTaskInfo* __DSP_rude_task;
extern int __DSP_rude_task_pending;
extern DSPTaskInfo* __DSP_first_task;
extern DSPTaskInfo* __DSP_last_task;
extern DSPTaskInfo* __DSP_curr_task;
extern DSPTaskInfo* __DSP_tmp_task;

void __DSPHandler(__OSInterrupt interrupt, OSContext* context);
void __DSP_insert_task(DSPTaskInfo* task);
void __DSP_boot_task(DSPTaskInfo* task);
void __DSP_exec_task(DSPTaskInfo* current, DSPTaskInfo* next);
void __DSP_remove_task(DSPTaskInfo* task);

#endif
