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
void TRKSwapAndGo(void);

#endif
