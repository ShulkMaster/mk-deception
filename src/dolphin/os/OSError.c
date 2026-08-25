#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"
#include "runtime/cstdarg.h"

extern int vprintf(const char* format, __va_list arguments);

#define OS_ERROR_COUNT 17
#define OS_ERROR_FLOATING_POINT 16
#define OS_EXCEPTION_PROGRAM 6
#define OS_EXCEPTION_DECREMENTER 8
#define OS_EXCEPTION_DSI 2
#define OS_EXCEPTION_ISI 3
#define OS_EXCEPTION_ALIGNMENT 5
#define OS_EXCEPTION_MEMORY_PROTECTION 15

#define MSR_RI 0x00000002
#define MSR_FP 0x00002000
#define MSR_FE0 0x00000800
#define MSR_FE1 0x00000100
#define FPSCR_ENABLE 0x000000F8
#define FPSCR_NI 0x00000004
#define FPSCR_STATUS_CLEAR_MASK 0x6005F8FF
#define OS_CONTEXT_STATE_FPSAVED 1

#define OS_FPU_CONTEXT (*(volatile OSContext**)0x800000D8)
#define DSP_REGS ((volatile unsigned short*)0xCC005000)
#define DI_REGS ((volatile unsigned long*)0xCC006000)

extern OSThreadQueue __OSActiveThreadQueue;
extern volatile unsigned long __OSLastInterruptSrr0;
extern volatile signed short __OSLastInterrupt;
extern volatile OSTime __OSLastInterruptTime;

OSErrorHandler __OSErrorTable[OS_ERROR_COUNT];
unsigned long __OSFpscrEnableBits = FPSCR_ENABLE;

void OSReport(const char* message, ...)
{
    __va_list arguments;

    va_start(arguments, message);
    vprintf(message, arguments);
    va_end(arguments);
}

void OSPanic(const char* file, int line, const char* message, ...)
{
    __va_list arguments;
    unsigned long count;
    unsigned long* frame;

    OSDisableInterrupts();
    va_start(arguments, message);
    vprintf(message, arguments);
    va_end(arguments);
    OSReport(" in \"%s\" on line %d.\n", file, line);
    OSReport("\nAddress:      Back Chain    LR Save\n");

    for (count = 0, frame = OSGetStackPointer();
         frame && (unsigned long)frame != 0xFFFFFFFF && count++ < 16;
         frame = (unsigned long*)frame[0]) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", frame, frame[0], frame[1]);
    }
    PPCHalt();
}

OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler)
{
    OSErrorHandler previous;
    unsigned long msr;
    unsigned long fpscr;
    OSThread* thread;
    int enabled;
    int index;

    enabled = OSDisableInterrupts();
    previous = __OSErrorTable[error];
    __OSErrorTable[error] = handler;

    if (error == OS_ERROR_FLOATING_POINT) {
        msr = PPCMfmsr();
        PPCMtmsr(msr | MSR_FP);
        fpscr = PPCMffpscr();

        if (handler) {
            for (thread = __OSActiveThreadQueue.head; thread;
                 thread = thread->linkActive.next) {
                thread->context.srr1 |= MSR_FE0 | MSR_FE1;
                if (!(thread->context.state & OS_CONTEXT_STATE_FPSAVED)) {
                    thread->context.state |= OS_CONTEXT_STATE_FPSAVED;
                    for (index = 0; index < 32; index++) {
                        *(unsigned long long*)&thread->context.fpr[index] =
                            0xFFFFFFFFFFFFFFFFULL;
                        *(unsigned long long*)&thread->context.psf[index] =
                            0xFFFFFFFFFFFFFFFFULL;
                    }
                    thread->context.fpscr = FPSCR_NI;
                }
                thread->context.fpscr |= __OSFpscrEnableBits & FPSCR_ENABLE;
                thread->context.fpscr &= FPSCR_STATUS_CLEAR_MASK;
            }
            fpscr |= __OSFpscrEnableBits & FPSCR_ENABLE;
            msr |= MSR_FE0 | MSR_FE1;
        } else {
            for (thread = __OSActiveThreadQueue.head; thread;
                 thread = thread->linkActive.next) {
                thread->context.srr1 &= ~(MSR_FE0 | MSR_FE1);
                thread->context.fpscr &= ~FPSCR_ENABLE;
                thread->context.fpscr &= FPSCR_STATUS_CLEAR_MASK;
            }
            fpscr &= ~FPSCR_ENABLE;
            msr &= ~(MSR_FE0 | MSR_FE1);
        }
        fpscr &= FPSCR_STATUS_CLEAR_MASK;
        PPCMtfpscr(fpscr);
        PPCMtmsr(msr);
    }

    OSRestoreInterrupts(enabled);
    return previous;
}

void __OSUnhandledException(__OSException exception, OSContext* context,
                            unsigned long dsisr, unsigned long dar)
{
    OSTime now;
    unsigned long fpscr;
    unsigned long msr;

    now = OSGetTime();
    if (!(context->srr1 & MSR_RI)) {
        OSReport("Non-recoverable Exception %d", exception);
    } else {
        if (exception == OS_EXCEPTION_PROGRAM && (context->srr1 & 0x00100000) &&
            __OSErrorTable[OS_ERROR_FLOATING_POINT]) {
            exception = OS_ERROR_FLOATING_POINT;
            msr = PPCMfmsr();
            PPCMtmsr(msr | MSR_FP);
            if (OS_FPU_CONTEXT) {
                OSSaveFPUContext((OSContext*)OS_FPU_CONTEXT);
            }
            fpscr = PPCMffpscr() & FPSCR_STATUS_CLEAR_MASK;
            PPCMtfpscr(fpscr);
            PPCMtmsr(msr);

            if (OS_FPU_CONTEXT == context) {
                OSDisableScheduler();
                __OSErrorTable[exception](exception, context, dsisr, dar);
                context->srr1 &= ~MSR_FP;
                OS_FPU_CONTEXT = 0;
                context->fpscr &= FPSCR_STATUS_CLEAR_MASK;
                OSEnableScheduler();
                __OSReschedule();
            } else {
                context->srr1 &= ~MSR_FP;
                OS_FPU_CONTEXT = 0;
            }
            OSLoadContext(context);
        }

        if (__OSErrorTable[exception]) {
            OSDisableScheduler();
            __OSErrorTable[exception](exception, context, dsisr, dar);
            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(context);
        }
        if (exception == OS_EXCEPTION_DECREMENTER) {
            OSLoadContext(context);
        }
        OSReport("Unhandled Exception %d", exception);
    }

    OSReport("\n");
    OSDumpContext(context);
    OSReport("\nDSISR = 0x%08x                   DAR  = 0x%08x\n", dsisr, dar);
    OSReport("TB = 0x%016llx\n", now);

    switch (exception) {
    case OS_EXCEPTION_DSI:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
                 "invalid address 0x%x (read from DAR)\n", context->srr0, dar);
        break;
    case OS_EXCEPTION_ISI:
        OSReport("\nAttempted to fetch instruction from invalid address 0x%x "
                 "(read from SRR0)\n", context->srr0);
        break;
    case OS_EXCEPTION_ALIGNMENT:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
                 "unaligned address 0x%x (read from DAR)\n", context->srr0, dar);
        break;
    case OS_EXCEPTION_PROGRAM:
        OSReport("\nProgram exception : Possible illegal instruction/operation "
                 "at or around 0x%x (read from SRR0)\n", context->srr0, dar);
        break;
    case OS_EXCEPTION_MEMORY_PROTECTION:
        OSReport("\n");
        OSReport("AI DMA Address =   0x%04x%04x\n", DSP_REGS[24], DSP_REGS[25]);
        OSReport("ARAM DMA Address = 0x%04x%04x\n", DSP_REGS[16], DSP_REGS[17]);
        OSReport("DI DMA Address =   0x%08x\n", DI_REGS[5]);
        break;
    }

    OSReport("\nLast interrupt (%d): SRR0 = 0x%08x  TB = 0x%016llx\n",
             __OSLastInterrupt, __OSLastInterruptSrr0, __OSLastInterruptTime);
    PPCHalt();
}
