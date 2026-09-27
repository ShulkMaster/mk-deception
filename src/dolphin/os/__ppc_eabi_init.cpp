#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"
#include "runtime/asm_sequences.inc"

__declspec(section ".init") asm void __init_hardware(void)
{
    SEQ___init_hardware();
}

__declspec(section ".init") asm void __flush_cache(void* address, unsigned int size)
{
    SEQ___flush_cache();
}

typedef void (*Ctor)(void);

extern Ctor _ctors[];

static void __init_cpp(void);

void __init_user(void) {
    __init_cpp();
}

static void __init_cpp(void) {
    Ctor* ctor;

    for (ctor = _ctors; *ctor != 0; ctor++) {
        (*ctor)();
    }
}

void _ExitProcess(void)
{
    PPCHalt();
}
