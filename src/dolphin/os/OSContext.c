#include "dolphin/db.h"
#include "dolphin/os.h"
#include "runtime/asm_sequences.inc"

volatile OSContext* __OSCurrentContext : 0x800000D4;
volatile OSContext* __OSFPUContext : 0x800000D8;

#define OS_CURRENT_CONTEXT __OSCurrentContext
#define OS_FPU_CONTEXT __OSFPUContext
#define OS_CURRENT_CONTEXT_PHYSICAL (*(volatile unsigned long*)0x800000C0)
#define OS_CONTEXT_STATE_FPSAVED 1u
#define OS_CONTEXT_STATE_EXCEPTION 2

extern char _SDA2_BASE_[];
extern char _SDA_BASE_[];

static asm void __OSLoadFPUContext(unsigned long exception, OSContext* context)
{
    SEQ___OSLoadFPUContext();
}

static asm void __OSSaveFPUContext(unsigned long exception, unsigned long unused,
                                   OSContext* context)
{
    SEQ___OSSaveFPUContext();
}

asm void OSSaveFPUContext(OSContext* context)
{
    SEQ_OSSaveFPUContext();
}

asm void OSSetCurrentContext(OSContext* context)
{
    SEQ_OSSetCurrentContext();
}

OSContext* OSGetCurrentContext(void)
{
    return (OSContext*)OS_CURRENT_CONTEXT;
}

asm unsigned long OSSaveContext(OSContext* context)
{
    SEQ_OSSaveContext();
}

asm void OSLoadContext(OSContext* context)
{
    SEQ_OSLoadContext();
}

asm void* OSGetStackPointer(void)
{
    SEQ_OSGetStackPointer();
}

void OSClearContext(OSContext* context)
{
    context->mode = 0;
    context->state = 0;
    if (context == OS_FPU_CONTEXT) {
        OS_FPU_CONTEXT = 0;
    }
}

asm void OSInitContext(OSContext* context, unsigned long program_counter,
                       unsigned long stack_pointer)
{
    SEQ_OSInitContext();
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

static asm void OSSwitchFPUContext(__OSException exception, OSContext* context)
{
    SEQ_OSSwitchFPUContext();
}

void __OSContextInit(void)
{
    __OSSetExceptionHandler(7, OSSwitchFPUContext);
    OS_FPU_CONTEXT = 0;
    DBPrintf("FPU-unavailable handler installed\n");
}
