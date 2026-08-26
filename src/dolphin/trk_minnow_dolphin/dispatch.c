#include "dolphin/trk.h"

extern DSError TRKDoConnect(MessageBuffer* message);
extern DSError TRKDoDisconnect(MessageBuffer* message);
extern DSError TRKDoReset(MessageBuffer* message);
extern DSError TRKDoOverride(MessageBuffer* message);
extern DSError TRKDoVersions(MessageBuffer* message);
extern DSError TRKDoSupportMask(MessageBuffer* message);
extern DSError TRKDoReadMemory(MessageBuffer* message);
extern DSError TRKDoWriteMemory(MessageBuffer* message);
extern DSError TRKDoReadRegisters(MessageBuffer* message);
extern DSError TRKDoWriteRegisters(MessageBuffer* message);
extern DSError TRKDoContinue(MessageBuffer* message);
extern DSError TRKDoStep(MessageBuffer* message);
extern DSError TRKDoStop(MessageBuffer* message);
extern DSError TRKDoSetOption(MessageBuffer* message);

static const char dispatch_command[] = "Dispatch command 0x%08x\n";
static const char dispatch_complete[] = "Dispatch complete err = %ld\n";

DSError TRKInitializeDispatcher(void)
{
    return 0;
}

DSError TRKDispatchMessage(MessageBuffer* message)
{
    DSError error = 0x500;

    TRKSetBufferPosition(message, 0);
    MWTRACE(1, dispatch_command, message->data[4]);
    switch (message->data[4]) {
    case 1:
        error = TRKDoConnect(message);
        break;
    case 2:
        error = TRKDoDisconnect(message);
        break;
    case 3:
        error = TRKDoReset(message);
        break;
    case 7:
        error = TRKDoOverride(message);
        break;
    case 4:
        error = TRKDoVersions(message);
        break;
    case 5:
        error = TRKDoSupportMask(message);
        break;
    case 0x10:
        error = TRKDoReadMemory(message);
        break;
    case 0x11:
        error = TRKDoWriteMemory(message);
        break;
    case 0x12:
        error = TRKDoReadRegisters(message);
        break;
    case 0x13:
        error = TRKDoWriteRegisters(message);
        break;
    case 0x18:
        error = TRKDoContinue(message);
        break;
    case 0x19:
        error = TRKDoStep(message);
        break;
    case 0x1A:
        error = TRKDoStop(message);
        break;
    case 0x17:
        error = TRKDoSetOption(message);
        break;
    }
    MWTRACE(1, dispatch_complete, error);
    return error;
}
