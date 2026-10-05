#include "dolphin/base/PPCArch.h"
#include "dolphin/db.h"
#include "runtime/asm_sequences.inc"

int DBVerbose;
DBInterface* __DBInterface;

void DBInit(void)
{
    __DBInterface = OSPhysicalToCached(0x40);
    __DBInterface->exception_destination =
        (void (*)(void))OSCachedToPhysical(__DBExceptionDestination);
    DBVerbose = 1;
}

void __DBExceptionDestinationAux(void)
{
    unsigned long* context_address = (unsigned long*)0xC0;
    OSContext* context = OSPhysicalToCached(*context_address);

    OSReport("DBExceptionDestination\n");
    OSDumpContext(context);
    PPCHalt();
}

asm void __DBExceptionDestination(void)
{
    SEQ___DBExceptionDestination();
}

int __DBIsExceptionMarked(__OSException exception)
{
    unsigned long mask = 1 << exception;

    return __DBInterface->exception_mask & mask;
}

void DBPrintf(char* format, ...)
{
}
