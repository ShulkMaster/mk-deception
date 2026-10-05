#ifndef DOLPHIN_MSGHNDLR_H
#define DOLPHIN_MSGHNDLR_H

#include "dolphin/trk.h"

DSError TRKDoConnect(MessageBuffer* message);
DSError TRKDoDisconnect(MessageBuffer* message);
DSError TRKDoReset(MessageBuffer* message);
DSError TRKDoOverride(MessageBuffer* message);
DSError TRKDoVersions(MessageBuffer* message);
DSError TRKDoSupportMask(MessageBuffer* message);
DSError TRKDoReadMemory(MessageBuffer* message);
DSError TRKDoWriteMemory(MessageBuffer* message);
DSError TRKDoReadRegisters(MessageBuffer* message);
DSError TRKDoWriteRegisters(MessageBuffer* message);
DSError TRKDoContinue(MessageBuffer* message);
DSError TRKDoStep(MessageBuffer* message);
DSError TRKDoStop(MessageBuffer* message);
DSError TRKDoSetOption(MessageBuffer* message);

#endif
