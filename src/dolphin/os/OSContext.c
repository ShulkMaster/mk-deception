#include "dolphin/db.h"
#include "dolphin/os.h"

volatile OSContext* __OSCurrentContext : 0x800000D4;
volatile OSContext* __OSFPUContext : 0x800000D8;

#define OS_CURRENT_CONTEXT __OSCurrentContext
#define OS_FPU_CONTEXT __OSFPUContext
#define OS_CURRENT_CONTEXT_PHYSICAL (*(volatile unsigned long*)0x800000C0)
#define OS_CONTEXT_STATE_FPSAVED 1
#define OS_CONTEXT_STATE_EXCEPTION 2

static void __OSLoadFPUContext(unsigned long exception, OSContext* context)
{
    /* FPR/PS register restoration is a privileged paired-single leaf. */
    if (!(context->state & OS_CONTEXT_STATE_FPSAVED)) {
        return;
    }
}

static void __OSSaveFPUContext(unsigned long exception, unsigned long unused,
                               OSContext* context)
{
    /* Retail stores the live FPR/FPSCR/PS register banks here. */
    context->state |= OS_CONTEXT_STATE_FPSAVED;
}

void OSSaveFPUContext(OSContext* context)
{
    __OSSaveFPUContext(0, 0, context);
}

void OSSetCurrentContext(OSContext* context)
{
    OS_CURRENT_CONTEXT = context;
    OS_CURRENT_CONTEXT_PHYSICAL = (unsigned long)context & 0x3FFFFFFF;
    if (OS_FPU_CONTEXT == context) {
        context->srr1 |= 0x2000;
    } else {
        context->srr1 &= ~0x2000;
    }
}

OSContext* OSGetCurrentContext(void)
{
    return (OSContext*)OS_CURRENT_CONTEXT;
}

unsigned long OSSaveContext(OSContext* context)
{
    /* Retail saves nonvolatile GPRs and special registers before returning 0. */
    context->gpr[3] = 1;
    return 0;
}

void OSLoadContext(OSContext* context)
{
    /* Retail restores registers and returns from interrupt with rfi. */
    if (context->state & OS_CONTEXT_STATE_EXCEPTION) {
        context->state &= ~OS_CONTEXT_STATE_EXCEPTION;
    }
    OSSetCurrentContext(context);
}

void* OSGetStackPointer(void)
{
    /* The retail leaf returns r1 directly. */
    return 0;
}

void OSClearContext(OSContext* context)
{
    context->mode = 0;
    context->state = 0;
    if (context == OS_FPU_CONTEXT) {
        OS_FPU_CONTEXT = 0;
    }
}

void OSInitContext(OSContext* context, unsigned long program_counter,
                   unsigned long stack_pointer)
{
    unsigned long index;

    context->srr0 = program_counter;
    context->gpr[1] = stack_pointer;
    context->srr1 = 0x00009032;
    context->cr = 0;
    context->xer = 0;
    for (index = 3; index <= 12; index++) {
        context->gpr[index] = 0;
    }
    for (index = 14; index < 32; index++) {
        context->gpr[index] = 0;
    }
    for (index = 0; index < 8; index++) {
        context->gqr[index] = 0;
    }
    OSClearContext(context);
}

void OSDumpContext(OSContext* context)
{
    unsigned long index;
    unsigned long* frame;

    OSReport("------------------------- Context 0x%08x -------------------------\n",
             context);
    for (index = 0; index < 16; index++) {
        OSReport("r%-2d  = 0x%08x (%14d)  r%-2d  = 0x%08x (%14d)\n",
                 index, context->gpr[index], context->gpr[index], index + 16,
                 context->gpr[index + 16], context->gpr[index + 16]);
    }
    OSReport("LR   = 0x%08x                   CR   = 0x%08x\n",
             context->lr, context->cr);
    OSReport("SRR0 = 0x%08x                   SRR1 = 0x%08x\n",
             context->srr0, context->srr1);
    OSReport("\nGQRs----------\n");
    for (index = 0; index < 4; index++) {
        OSReport("gqr%d = 0x%08x \t gqr%d = 0x%08x\n", index,
                 context->gqr[index], index + 4, context->gqr[index + 4]);
    }

    if (context->state & OS_CONTEXT_STATE_FPSAVED) {
        OSContext* current_context;
        OSContext temporary_context;
        int enabled = OSDisableInterrupts();

        current_context = OSGetCurrentContext();
        OSClearContext(&temporary_context);
        OSSetCurrentContext(&temporary_context);
        OSReport("\n\nFPRs----------\n");
        for (index = 0; index < 32; index += 2) {
            OSReport("fr%d \t= %d \t fr%d \t= %d\n", index,
                     (unsigned long)context->fpr[index], index + 1,
                     (unsigned long)context->fpr[index + 1]);
        }
        OSReport("\n\nPSFs----------\n");
        for (index = 0; index < 32; index += 2) {
            OSReport("ps%d \t= 0x%x \t ps%d \t= 0x%x\n", index,
                     (unsigned long)context->psf[index], index + 1,
                     (unsigned long)context->psf[index + 1]);
        }
        OSClearContext(&temporary_context);
        OSSetCurrentContext(current_context);
        OSRestoreInterrupts(enabled);
    }

    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (index = 0, frame = (unsigned long*)context->gpr[1];
         frame && (unsigned long)frame != 0xFFFFFFFF && index++ < 16;
         frame = (unsigned long*)frame[0]) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", frame, frame[0], frame[1]);
    }
}

static void OSSwitchFPUContext(__OSException exception, OSContext* context)
{
    OSContext* previous = (OSContext*)OS_FPU_CONTEXT;

    context->srr1 |= 0x2000;
    OS_FPU_CONTEXT = context;
    if (previous != context) {
        if (previous) {
            __OSSaveFPUContext(exception, 0, previous);
        }
        __OSLoadFPUContext(exception, context);
    }
    context->state &= ~OS_CONTEXT_STATE_EXCEPTION;
}

void __OSContextInit(void)
{
    __OSSetExceptionHandler(7, OSSwitchFPUContext);
    OS_FPU_CONTEXT = 0;
    DBPrintf("FPU-unavailable handler installed\n");
}
