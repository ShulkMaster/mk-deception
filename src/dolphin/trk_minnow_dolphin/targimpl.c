#include "dolphin/trk.h"
#include "runtime/cstring.h"

typedef struct TRKStopInfoPacket {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 pc;
    u32 instruction;
    u32 exception_id;
    u8 field_0x14[0x2C];
} TRKStopInfoPacket;

typedef char TRKStopInfoPacketSizeCheck[
    sizeof(TRKStopInfoPacket) == 0x40 ? 1 : -1];

extern DSError TRKTargetAccessMemory(void* data, u32 address, u32* length,
                                    BOOL write, BOOL use_virtual_address);
extern DSError TRKTargetReadInstruction(u32* instruction, u32 address);

void TRKTargetSetInputPendingPtr(volatile u8* input_pending)
{
    gTRKState.input_pending = input_pending;
}

DSError TRKTargetStop(void)
{
    gTRKState.stopped = 1;
    return 0;
}

void TRKTargetSetStopped(BOOL stopped)
{
    gTRKState.stopped = stopped;
}

BOOL TRKTargetStopped(void)
{
    return gTRKState.stopped;
}

u32 TRKTargetGetPC(void)
{
    return gTRKCPUState.pc;
}

void TRKTargetAddExceptionInfo(MessageBuffer* message)
{
    TRKStopInfoPacket packet;

    memset(&packet, 0, sizeof(packet));
    packet.length = sizeof(packet);
    packet.command = 0x91;
    packet.pc = gTRKExceptionStatus.pc;
    TRKTargetReadInstruction(&packet.instruction, gTRKExceptionStatus.pc);
    packet.exception_id = gTRKExceptionStatus.exception_id;
    TRKAppendBuffer_ui8(message, (u8*)&packet, sizeof(packet));
}

void TRKTargetAddStopInfo(MessageBuffer* message)
{
    TRKStopInfoPacket packet;

    memset(&packet, 0, sizeof(packet));
    packet.length = sizeof(packet);
    packet.command = 0x90;
    packet.pc = gTRKCPUState.pc;
    TRKTargetReadInstruction(&packet.instruction, gTRKCPUState.pc);
    packet.exception_id = gTRKCPUState.exception_id;
    TRKAppendBuffer_ui8(message, (u8*)&packet, sizeof(packet));
}

DSError TRKTargetReadInstruction(u32* instruction, u32 address)
{
    u32 length = sizeof(*instruction);
    DSError error;

    error = TRKTargetAccessMemory(instruction, address, &length, 0, 1);
    if (error == 0 && length != sizeof(*instruction)) {
        error = 0x700;
    }
    return error;
}
