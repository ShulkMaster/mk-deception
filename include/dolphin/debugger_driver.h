#ifndef DOLPHIN_DEBUGGER_DRIVER_H
#define DOLPHIN_DEBUGGER_DRIVER_H

#include "dolphin/exi.h"
#include "dolphin/types.h"

void DBClose(void);
void DBOpen(void);
int DBWrite(const void* bytes, int length);
int DBRead(void* bytes, u32 length);
int DBQueryData(void);
void DBInitInterrupts(void);
void DBInitComm(volatile u8** input_pending, EXICallback monitor_callback);

#endif
