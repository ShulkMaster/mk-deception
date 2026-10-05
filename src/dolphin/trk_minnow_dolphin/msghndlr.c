#include "dolphin/msghndlr.h"
#include "dolphin/targimpl.h"
#include "runtime/cstring.h"

struct TRKReplyPacket {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u8 error;
    u8 field_0x09[0x37];
};

typedef char TRKReplyPacketSizeCheck[sizeof(struct TRKReplyPacket) == 0x40 ? 1 : -1];

extern void __TRK_copy_vectors(void);
extern void __TRK_reset(void);

static BOOL IsTRKConnected;

void OutputData(const u8* data, s32 length)
{
    s32 index;

    for (index = 0; index < length; index++) {
        MWTRACE(8, "%02x ", data[index]);
        if (index % 16 == 15) {
            MWTRACE(8, "\n");
        }
    }
    MWTRACE(8, "\n");
}

u32 GetTRKConnected(void)
{
    return IsTRKConnected;
}

void SetTRKConnected(u32 connected)
{
    IsTRKConnected = connected;
}

DSError TRKSendACK(MessageBuffer* message)
{
    DSError error;

    MWTRACE(1, "SendACK : Calling MessageSend\n");
    error = TRKMessageSend(message);
    MWTRACE(1, "MessageSend err : %ld\n", error);
    return error;
}

DSError TRKStandardACK(MessageBuffer* message, MessageCommandID command,
                       int reply_error)
{
    struct TRKReplyPacket reply;

    memset(&reply, 0, sizeof(reply));
    reply.command = command;
    reply.length = sizeof(reply);
    reply.error = reply_error;
    TRKWriteUARTN(&reply, sizeof(reply));
    return 0;
}

DSError TRKDoConnect(MessageBuffer* message)
{
    IsTRKConnected = 1;
    return TRKStandardACK(message, 0x80, 0);
}

DSError TRKDoDisconnect(MessageBuffer* message)
{
    TRKEvent event;

    IsTRKConnected = 0;
    TRKStandardACK(message, 0x80, 0);
    TRKConstructEvent(&event, 1);
    TRKPostEvent(&event);
    return 0;
}

DSError TRKDoReset(MessageBuffer* message)
{
    TRKStandardACK(message, 0x80, 0);
    __TRK_reset();
    return 0;
}

DSError TRKDoOverride(MessageBuffer* message)
{
    TRKStandardACK(message, 0x80, 0);
    __TRK_copy_vectors();
    return 0;
}

DSError TRKDoVersions(MessageBuffer* message)
{
    return 0;
}

DSError TRKDoSupportMask(MessageBuffer* message)
{
    return 0;
}

DSError TRKDoReadMemory(MessageBuffer* message)
{
    u8 buffer[0x820] __attribute__((aligned(32)));
    u32 transfer_length;
    DSError error;
    int reply_error;
    int options;
    u32 length;
    u32 start;

    start = *(u32*)&message->data[16];
    length = *(u16*)&message->data[12];
    options = message->data[8];

    MWTRACE(1, "ReadMemory (0x%02x) : 0x%08x 0x%08x 0x%08x\n", message->data[4],
            start, length, options);

    if (options & 2) {
        return TRKStandardACK(message, 0x80, 0x12);
    }

    transfer_length = length;
    if (options & 0x40) {
        error = TRKTargetAccessARAM(buffer, start, &transfer_length, 1);
    } else {
        error = TRKTargetAccessMemory(buffer, start, &transfer_length,
                                      (options & 8) ? 0 : 1, 1);
    }

    TRKResetBuffer(message, 0);

    if (error == 0) {
        struct TRKReplyPacket reply;

        memset(&reply, 0, sizeof(reply));
        reply.error = error;
        reply.length = transfer_length + sizeof(reply);
        reply.command = 0x80;
        TRKAppendBuffer(message, &reply, sizeof(reply));

        if (options & 0x40) {
            error = TRKAppendBuffer(message, buffer + (start & 0x1F), transfer_length);
        } else {
            error = TRKAppendBuffer(message, buffer, transfer_length);
        }
    }

    if (error != 0) {
        switch (error) {
        case 0x702:
            reply_error = 0x15;
            break;
        case 0x700:
            reply_error = 0x13;
            break;
        case 0x704:
            reply_error = 0x21;
            break;
        case 0x705:
            reply_error = 0x22;
            break;
        case 0x706:
            reply_error = 0x20;
            break;
        default:
            reply_error = 3;
            break;
        }
        return TRKStandardACK(message, 0x80, reply_error);
    }

    return TRKSendACK(message);
}

DSError TRKDoWriteMemory(MessageBuffer* message)
{
    u8 buffer[0x820] __attribute__((aligned(32)));
    u32 transfer_length;
    int options;
    DSError error;
    int reply_error;
    u32 length;
    u32 start;

    start = *(u32*)&message->data[16];
    length = *(u16*)&message->data[12];
    options = message->data[8];

    MWTRACE(1, "WriteMemory (0x%02x) : 0x%08x 0x%08x 0x%08x\n", (u32)message->data[4],
            start, length, options);

    if (options & 2) {
        return TRKStandardACK(message, 0x80, 0x12);
    }

    transfer_length = length;
    TRKSetBufferPosition(message, 0x40);
    if (options & 0x40) {
        TRKReadBuffer(message, buffer + (start & 0x1F), transfer_length);
        error = TRKTargetAccessARAM(buffer, start, &transfer_length, 0);
    } else {
        TRKReadBuffer(message, buffer, transfer_length);
        error = TRKTargetAccessMemory(buffer, start, &transfer_length,
                                      (options & 8) ? 0 : 1, 0);
    }

    TRKResetBuffer(message, 0);

    if (error == 0) {
        struct TRKReplyPacket reply;

        memset(&reply, 0, sizeof(reply));
        reply.length = sizeof(reply);
        reply.command = 0x80;
        reply.error = error;
        error = TRKAppendBuffer(message, &reply, sizeof(reply));
    }

    if (error != 0) {
        switch (error) {
        case 0x702:
            reply_error = 0x15;
            break;
        case 0x700:
            reply_error = 0x13;
            break;
        case 0x704:
            reply_error = 0x21;
            break;
        case 0x705:
            reply_error = 0x22;
            break;
        case 0x706:
            reply_error = 0x20;
            break;
        default:
            reply_error = 3;
            break;
        }
        return TRKStandardACK(message, 0x80, reply_error);
    }

    return TRKSendACK(message);
}

DSError TRKDoReadRegisters(MessageBuffer* message)
{
    DSError error;
    u8 options;
    u16 first_register;
    u16 last_register;
    u32 registers_length;
    struct TRKReplyPacket reply;

    options = message->data[8];
    first_register = *(u16*)&message->data[12];
    last_register = *(u16*)&message->data[16];

    if (first_register > last_register) {
        return TRKStandardACK(message, 0x80, 0x14);
    }

    reply.command = 0x80;
    reply.length = 0x468;

    TRKResetBuffer(message, 0);
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    TRKAppendBuffer_ui8(message, (u8*)&reply, sizeof(reply));
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    error = TRKTargetAccessDefault(0, 36, message, &registers_length, 1);
    MWTRACE(4, "DoReadRegisters : Error reading  default regs 0x%08x\n", error);
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    if (error == 0) {
        error = TRKTargetAccessFP(0, 33, message, &registers_length, 1);
    }
    MWTRACE(4, "DoReadRegisters : Error FP regs 0x%08x\n", error);
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    if (error == 0) {
        error = TRKTargetAccessExtended1(0, 0x60, message, &registers_length, 1);
    }
    MWTRACE(4, "DoReadRegisters : Error extended1 regs 0x%08x\n", error);
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    if (error == 0) {
        error = TRKTargetAccessExtended2(0, 31, message, &registers_length, 1);
    }
    MWTRACE(4, "DoReadRegisters : Error extended2 regs 0x%08x\n", error);
    MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", message->length);

    if (error != 0) {
        int reply_error;

        switch (error) {
        case 0x703:
            reply_error = 0x12;
            break;
        case 0x701:
            reply_error = 0x14;
            break;
        case 0x702:
            reply_error = 0x15;
            break;
        case 0x704:
            reply_error = 0x21;
            break;
        case 0x705:
            reply_error = 0x22;
            break;
        case 0x706:
            reply_error = 0x20;
            break;
        default:
            reply_error = 3;
        }
        return TRKStandardACK(message, 0x80, reply_error);
    } else {
        return TRKSendACK(message);
    }
}

DSError TRKDoWriteRegisters(MessageBuffer* message)
{
    DSError error;
    int reply_error;
    u8 options;
    u16 first_register;
    u16 last_register;
    u32 registers_length;

    options = message->data[8];
    first_register = *(u16*)&message->data[12];
    last_register = *(u16*)&message->data[16];

    TRKSetBufferPosition(message, 0);

    if (first_register > last_register) {
        return TRKStandardACK(message, 0x80, 0x14);
    }

    TRKSetBufferPosition(message, 0x40);

    switch (options) {
    case 0:
        error = TRKTargetAccessDefault(first_register, last_register, message,
                                       &registers_length, 0);
        break;
    case 1:
        error = TRKTargetAccessFP(first_register, last_register, message,
                                  &registers_length, 0);
        break;
    case 2:
        error = TRKTargetAccessExtended1(first_register, last_register, message,
                                         &registers_length, 0);
        break;
    case 3:
        error = TRKTargetAccessExtended2(first_register, last_register, message,
                                         &registers_length, 0);
        break;
    default:
        error = 0x703;
        break;
    }

    TRKResetBuffer(message, 0);

    if (error == 0) {
        struct TRKReplyPacket reply;

        memset(&reply, 0, sizeof(reply));
        reply.length = sizeof(reply);
        reply.command = 0x80;
        reply.error = error;
        error = TRKAppendBuffer(message, (u8*)&reply, sizeof(reply));
    }

    if (error != 0) {
        switch (error) {
        case 0x703:
            reply_error = 0x12;
            break;
        case 0x701:
            reply_error = 0x14;
            break;
        case 0x302:
            reply_error = 2;
            break;
        case 0x702:
            reply_error = 0x15;
            break;
        case 0x704:
            reply_error = 0x21;
            break;
        case 0x705:
            reply_error = 0x22;
            break;
        case 0x706:
            reply_error = 0x20;
            break;
        default:
            reply_error = 3;
        }
        return TRKStandardACK(message, 0x80, reply_error);
    } else {
        return TRKSendACK(message);
    }
}

void TRKDoFlushCache(void)
{
    MWTRACE(1, "DoFlushCache unimplemented!!!\n");
}

DSError TRKDoContinue(MessageBuffer* message)
{
    MWTRACE(1, "DoContinue\n");
    if (!TRKTargetStopped()) {
        return TRKStandardACK(message, 0x80, 0x16);
    }

    TRKStandardACK(message, 0x80, 0);
    return TRKTargetContinue();
}

DSError TRKDoStep(MessageBuffer* message)
{
    DSError result;
    u8 mode;
    u8 count;
    u32 range_start;
    u32 range_end;
    u32 pc;

    TRKSetBufferPosition(message, 0);
    mode = message->data[0x08];
    range_start = *(u32*)&message->data[0x10];
    range_end = *(u32*)&message->data[0x14];

    switch (mode) {
    case 0:
    case 0x10:
        count = message->data[0x0C];
        if (count >= 1) {
            break;
        }
        return TRKStandardACK(message, 0x80, 0x11);
    case 1:
    case 0x11:
        pc = TRKTargetGetPC();
        if (pc >= range_start && pc <= range_end) {
            break;
        }
        return TRKStandardACK(message, 0x80, 0x11);
    default:
        return TRKStandardACK(message, 0x80, 0x12);
    }

    if (!TRKTargetStopped()) {
        return TRKStandardACK(message, 0x80, 0x16);
    }

    result = TRKStandardACK(message, 0x80, 0);
    switch (mode) {
    case 0:
    case 0x10:
        result = TRKTargetSingleStep(count, mode == 0x10);
        break;
    case 1:
    case 0x11:
        result = TRKTargetStepOutOfRange(range_start, range_end, mode == 0x11);
        break;
    }
    return result;
}

DSError TRKDoStop(MessageBuffer* message)
{
    int reply_error;

    switch (TRKTargetStop()) {
    case 0:
        reply_error = 0;
        break;
    case 0x704:
        reply_error = 0x21;
        break;
    case 0x705:
        reply_error = 0x22;
        break;
    case 0x706:
        reply_error = 0x20;
        break;
    default:
        reply_error = 1;
        break;
    }

    TRKStandardACK(message, 0x80, reply_error);
    return 0;
}

DSError TRKDoSetOption(MessageBuffer* message)
{
    u8 enable = message->data[0x0C];

    if (message->data[0x08] == 1) {
        usr_puts_serial("\nMetroTRK Option : SerialIO - ");
        if (enable) {
            usr_puts_serial("Enable\n");
        } else {
            usr_puts_serial("Disable\n");
        }
        SetUseSerialIO(enable);
    }

    TRKStandardACK(message, 0x80, 0);
    return 0;
}
