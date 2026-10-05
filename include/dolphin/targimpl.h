#ifndef DOLPHIN_TARGIMPL_H
#define DOLPHIN_TARGIMPL_H

#include "dolphin/trk.h"

void TRKTargetSetInputPendingPtr(volatile u8* input_pending);
DSError TRKTargetStop(void);
void TRKTargetSetStopped(BOOL stopped);
BOOL TRKTargetStopped(void);
u32 TRKTargetGetPC(void);
void TRKTargetAddExceptionInfo(MessageBuffer* message);
void TRKTargetAddStopInfo(MessageBuffer* message);
DSError TRKTargetReadInstruction(u32* instruction, u32 address);
DSError TRKTargetAccessMemory(void* data, u32 start, u32* length,
                              int access_options, BOOL read);
DSError TRKTargetAccessARAM(void* data, u32 start, u32* length, BOOL read);
DSError TRKTargetAccessDefault(u32 first_register, u32 last_register,
                               MessageBuffer* message, u32* registers_length,
                               BOOL read);
DSError TRKTargetAccessFP(u32 first_register, u32 last_register,
                          MessageBuffer* message, u32* registers_length,
                          BOOL read);
DSError TRKTargetAccessExtended1(u32 first_register, u32 last_register,
                                 MessageBuffer* message,
                                 u32* registers_length, BOOL read);
DSError TRKTargetAccessExtended2(u32 first_register, u32 last_register,
                                 MessageBuffer* message,
                                 u32* registers_length, BOOL read);
void TRKSwapAndGo(void);

#endif
