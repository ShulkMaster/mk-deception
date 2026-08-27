#include "dolphin/dsp.h"

volatile unsigned short DSP_REGISTERS[] : 0xCC005000;

const char* __DSPVersion =
    "<< Dolphin SDK - DSP\trelease build: Apr  5 2004 04:15:32 (0x2301) >>";

extern DSPTaskInfo* __DSP_rude_task;
extern int __DSP_rude_task_pending;
extern DSPTaskInfo* __DSP_first_task;
extern DSPTaskInfo* __DSP_last_task;
extern DSPTaskInfo* __DSP_curr_task;
extern DSPTaskInfo* __DSP_tmp_task;

void __DSPHandler(__OSInterrupt interrupt, OSContext* context);
void __DSP_insert_task(DSPTaskInfo* task);
void __DSP_boot_task(DSPTaskInfo* task);

static int __DSP_init_flag;

unsigned long DSPCheckMailToDSP(void)
{
    return (DSP_REGISTERS[0] & 0x8000) >> 15;
}

unsigned long DSPCheckMailFromDSP(void)
{
    return (DSP_REGISTERS[2] & 0x8000) >> 15;
}

unsigned long DSPReadMailFromDSP(void)
{
    return (DSP_REGISTERS[2] << 16) | DSP_REGISTERS[3];
}

void DSPSendMailToDSP(unsigned long mail)
{
    DSP_REGISTERS[0] = mail >> 16;
    DSP_REGISTERS[1] = mail & 0xFFFF;
}

inline void DSPAssertInt(void)
{
    int enabled;
    unsigned short control;

    enabled = OSDisableInterrupts();
    control = DSP_REGISTERS[5];
    control = (control & ~0xA8) | 2;
    DSP_REGISTERS[5] = control;
    OSRestoreInterrupts(enabled);
}

void DSPInit(void)
{
    int enabled;
    unsigned short control;

    __DSP_debug_printf("DSPInit(): Build Date: %s %s\n", "Apr  5 2004",
                       "04:15:32");
    if (__DSP_init_flag == 1) {
        return;
    }

    OSRegisterVersion(__DSPVersion);
    enabled = OSDisableInterrupts();
    __OSSetInterruptHandler(7, __DSPHandler);
    __OSUnmaskInterrupts(0x01000000);

    control = DSP_REGISTERS[5];
    control = (control & ~0xA8) | 0x800;
    DSP_REGISTERS[5] = control;
    control = DSP_REGISTERS[5];
    DSP_REGISTERS[5] = control & ~0xAC;

    __DSP_first_task = __DSP_last_task = __DSP_curr_task = __DSP_tmp_task = 0;
    __DSP_init_flag = 1;
    OSRestoreInterrupts(enabled);
}

int DSPCheckInit(void)
{
    return __DSP_init_flag;
}

DSPTaskInfo* DSPAddTask(DSPTaskInfo* task)
{
    int enabled;

    enabled = OSDisableInterrupts();
    __DSP_insert_task(task);
    task->state = 0;
    task->flags = 1;
    OSRestoreInterrupts(enabled);
    if (task == __DSP_first_task) {
        __DSP_boot_task(task);
    }
    return task;
}

DSPTaskInfo* DSPAssertTask(DSPTaskInfo* task)
{
    int enabled;

    enabled = OSDisableInterrupts();
    if (__DSP_curr_task == task) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = 1;
        OSRestoreInterrupts(enabled);
        return task;
    }

    if (task->priority < __DSP_curr_task->priority) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = 1;
        if (__DSP_curr_task->state == 1) {
            DSPAssertInt();
        }
        OSRestoreInterrupts(enabled);
        return task;
    }

    OSRestoreInterrupts(enabled);
    return 0;
}
