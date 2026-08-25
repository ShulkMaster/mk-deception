#include "dolphin/dsp.h"

#define DSP_REGS ((volatile unsigned short*)0xCC005000)

DSPTaskInfo* __DSP_curr_task;
DSPTaskInfo* __DSP_first_task;
DSPTaskInfo* __DSP_last_task;
DSPTaskInfo* __DSP_tmp_task;
DSPTaskInfo* __DSP_rude_task;
int __DSP_rude_task_pending;

void __DSP_exec_task(DSPTaskInfo* current, DSPTaskInfo* next);
void __DSP_remove_task(DSPTaskInfo* task);

void __DSPHandler(__OSInterrupt interrupt, OSContext* context)
{
    OSContext callback_context;
    unsigned short control;
    unsigned long mail;

    control = DSP_REGS[5];
    DSP_REGS[5] = (control & ~0x28) | 0x80;
    OSClearContext(&callback_context);
    OSSetCurrentContext(&callback_context);

    while (!DSPCheckMailFromDSP()) {}
    mail = DSPReadMailFromDSP();
    if ((__DSP_curr_task->flags & 2) && mail + 0x232F0000 == 2) {
        mail = 0xDCD10003;
    }

    switch (mail) {
    case 0xDCD10000:
        __DSP_curr_task->state = 1;
        if (__DSP_curr_task->initCallback) {
            __DSP_curr_task->initCallback(__DSP_curr_task);
        }
        break;
    case 0xDCD10001:
        __DSP_curr_task->state = 1;
        if (__DSP_curr_task->resumeCallback) {
            __DSP_curr_task->resumeCallback(__DSP_curr_task);
        }
        break;
    case 0xDCD10002:
        if (__DSP_rude_task_pending) {
            if (__DSP_curr_task == __DSP_rude_task) {
                DSPSendMailToDSP(0xCDD10003);
                while (DSPCheckMailToDSP()) {}
                __DSP_rude_task = 0;
                __DSP_rude_task_pending = 0;
                if (__DSP_curr_task->resumeCallback) {
                    __DSP_curr_task->resumeCallback(__DSP_curr_task);
                }
            } else {
                DSPSendMailToDSP(0xCDD10001);
                while (DSPCheckMailToDSP()) {}
                __DSP_exec_task(__DSP_curr_task, __DSP_rude_task);
                __DSP_curr_task->state = 2;
                __DSP_curr_task = __DSP_rude_task;
                __DSP_rude_task = 0;
                __DSP_rude_task_pending = 0;
            }
        } else if (!__DSP_curr_task->next) {
            if (__DSP_curr_task == __DSP_first_task) {
                DSPSendMailToDSP(0xCDD10003);
                while (DSPCheckMailToDSP()) {}
                if (__DSP_curr_task->resumeCallback) {
                    __DSP_curr_task->resumeCallback(__DSP_curr_task);
                }
            } else {
                DSPSendMailToDSP(0xCDD10001);
                while (DSPCheckMailToDSP()) {}
                __DSP_exec_task(__DSP_curr_task, __DSP_first_task);
                __DSP_curr_task->state = 2;
                __DSP_curr_task = __DSP_first_task;
            }
        } else {
            DSPSendMailToDSP(0xCDD10001);
            while (DSPCheckMailToDSP()) {}
            __DSP_exec_task(__DSP_curr_task, __DSP_curr_task->next);
            __DSP_curr_task->state = 2;
            __DSP_curr_task = __DSP_curr_task->next;
        }
        break;
    case 0xDCD10003:
        if (__DSP_rude_task_pending) {
            if (__DSP_curr_task->doneCallback) {
                __DSP_curr_task->doneCallback(__DSP_curr_task);
            }
            DSPSendMailToDSP(0xCDD10001);
            while (DSPCheckMailToDSP()) {}
            __DSP_exec_task(0, __DSP_rude_task);
            __DSP_remove_task(__DSP_curr_task);
            __DSP_curr_task = __DSP_rude_task;
            __DSP_rude_task = 0;
            __DSP_rude_task_pending = 0;
        } else if (!__DSP_curr_task->next) {
            if (__DSP_curr_task == __DSP_first_task) {
                if (__DSP_curr_task->doneCallback) {
                    __DSP_curr_task->doneCallback(__DSP_curr_task);
                }
                DSPSendMailToDSP(0xCDD10002);
                while (DSPCheckMailToDSP()) {}
                __DSP_curr_task->state = 3;
                __DSP_remove_task(__DSP_curr_task);
            } else {
                if (__DSP_curr_task->doneCallback) {
                    __DSP_curr_task->doneCallback(__DSP_curr_task);
                }
                DSPSendMailToDSP(0xCDD10001);
                while (DSPCheckMailToDSP()) {}
                __DSP_curr_task->state = 3;
                __DSP_exec_task(0, __DSP_first_task);
                __DSP_curr_task = __DSP_first_task;
                __DSP_remove_task(__DSP_last_task);
            }
        } else {
            if (__DSP_curr_task->doneCallback) {
                __DSP_curr_task->doneCallback(__DSP_curr_task);
            }
            DSPSendMailToDSP(0xCDD10001);
            while (DSPCheckMailToDSP()) {}
            __DSP_curr_task->state = 3;
            __DSP_exec_task(0, __DSP_curr_task->next);
            __DSP_curr_task = __DSP_curr_task->next;
            __DSP_remove_task(__DSP_curr_task->previous);
        }
        break;
    case 0xDCD10004:
        if (__DSP_curr_task->requestCallback) {
            __DSP_curr_task->requestCallback(__DSP_curr_task);
        }
        break;
    }

    OSClearContext(&callback_context);
    OSSetCurrentContext(context);
}

void __DSP_exec_task(DSPTaskInfo* current, DSPTaskInfo* next)
{
    if (current) {
        DSPSendMailToDSP((unsigned long)current->dramMemoryAddress);
        while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(current->dramLength);
        while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(current->dramAddress);
        while (DSPCheckMailToDSP()) {}
    } else {
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
    }

    DSPSendMailToDSP((unsigned long)next->iramMemoryAddress);
    while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(next->iramLength);
    while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(next->iramAddress);
    while (DSPCheckMailToDSP()) {}

    if (!next->state) {
        DSPSendMailToDSP(next->initVector); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
    } else {
        DSPSendMailToDSP(next->resumeVector); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP((unsigned long)next->dramMemoryAddress);
        while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(next->dramLength); while (DSPCheckMailToDSP()) {}
        DSPSendMailToDSP(next->dramAddress); while (DSPCheckMailToDSP()) {}
    }
}

void __DSP_boot_task(DSPTaskInfo* task)
{
    volatile unsigned long mail;

    while (!DSPCheckMailFromDSP()) {}
    mail = DSPReadMailFromDSP();
    DSPSendMailToDSP(0x80F3A001); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP((unsigned long)task->iramMemoryAddress);
    while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(0x80F3C002); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(task->iramAddress & 0xFFFF); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(0x80F3A002); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(task->iramLength); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(0x80F3B002); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(0); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(0x80F3D001); while (DSPCheckMailToDSP()) {}
    DSPSendMailToDSP(task->initVector); while (DSPCheckMailToDSP()) {}

    __DSP_debug_printf("DSP is booting task: 0x%08X\n", task);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM MMEM ADDR: 0x%08X\n",
                       task->iramMemoryAddress);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM DSP ADDR : 0x%08X\n",
                       task->iramAddress);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM LENGTH   : 0x%08X\n",
                       task->iramLength);
    __DSP_debug_printf("__DSP_boot_task()  : DRAM MMEM ADDR: 0x%08X\n",
                       task->dramLength);
    __DSP_debug_printf("__DSP_boot_task()  : Start Vector  : 0x%08X\n",
                       task->initVector);
}

void __DSP_insert_task(DSPTaskInfo* task)
{
    DSPTaskInfo* current;

    if (!__DSP_first_task) {
        __DSP_curr_task = task;
        __DSP_first_task = __DSP_last_task = task;
        task->next = task->previous = 0;
        return;
    }
    for (current = __DSP_first_task; current; current = current->next) {
        if (task->priority < current->priority) {
            task->previous = current->previous;
            current->previous = task;
            task->next = current;
            if (!task->previous) __DSP_first_task = task;
            else task->previous->next = task;
            break;
        }
    }
    if (!current) {
        __DSP_last_task->next = task;
        task->next = 0;
        task->previous = __DSP_last_task;
        __DSP_last_task = task;
    }
}

void __DSP_remove_task(DSPTaskInfo* task)
{
    task->flags = 0;
    task->state = 3;
    if (__DSP_first_task == task) {
        if (task->next) {
            __DSP_first_task = task->next;
            task->next->previous = 0;
        } else {
            __DSP_first_task = __DSP_last_task = __DSP_curr_task = 0;
        }
        return;
    }
    if (__DSP_last_task == task) {
        __DSP_last_task = task->previous;
        task->previous->next = 0;
        __DSP_curr_task = __DSP_first_task;
        return;
    }
    __DSP_curr_task = task->next;
    task->previous->next = task->next;
    task->next->previous = task->previous;
}
