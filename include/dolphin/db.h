#ifndef DOLPHIN_DB_H
#define DOLPHIN_DB_H

#include "dolphin/types.h"
#include "dolphin/os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBInterface {
    u32 debugger_present;
    u32 exception_mask;
    void (*exception_destination)(void);
    void* exception_return;
} DBInterface;

extern DBInterface* __DBInterface;
extern int DBVerbose;

void DBInit(void);
void __DBExceptionDestination(void);
void __DBExceptionDestinationAux(void);
int __DBIsExceptionMarked(__OSException exception);
void DBPrintf(char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
