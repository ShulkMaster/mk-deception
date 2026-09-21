#include "dolphin/base/PPCArch.h"
#include "dolphin/db.h"

DBInterface* __DBInterface;
int DBVerbose;

/* TODO: [near miss] 94.000000%; retail and current DBInit opcodes/data agree; only the local exception-symbol relocation metadata differs, with no honest source-only correction available. */
void DBInit(void)
{
    __DBInterface = (DBInterface*)OSPhysicalToCached(0x40);
    __DBInterface->exception_destination =
        (void (*)(void))OSCachedToPhysical(__DBExceptionDestination);
    DBVerbose = 1;
}

void __DBExceptionDestinationAux(void)
{
    unsigned long* context_address = (unsigned long*)0xC0;
    OSContext* context = (OSContext*)OSPhysicalToCached(*context_address);

    OSReport("DBExceptionDestination\n");
    OSDumpContext(context);
    PPCHalt();
}

/* __DBExceptionDestination is a retail MSR-control assembly boundary. */

int __DBIsExceptionMarked(__OSException exception)
{
    unsigned long mask = 1 << exception;

    return __DBInterface->exception_mask & mask;
}

void DBPrintf(char* format, ...)
{
}
