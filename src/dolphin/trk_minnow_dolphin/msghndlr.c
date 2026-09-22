#include "dolphin/trk.h"
#include "runtime/cstring.h"

typedef struct TRKReplyPacket {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u8 error;
    u8 field_0x09[0x37];
} TRKReplyPacket;

typedef char TRKReplyPacketSizeCheck[sizeof(TRKReplyPacket) == 0x40 ? 1 : -1];

extern void __TRK_copy_vectors(void);
extern void __TRK_reset(void);

static BOOL IsTRKConnected;

/* TODO: [near miss] 98.333336%; code and diagnostic bytes agree; only grouped string-table relocation identities differ. */
DSError TRKDoSetOption(MessageBuffer* message)
{
    static const struct {
        char label[32];
        char enabled[8];
        char disabled[9];
    } option_text = {
        "\nMetroTRK Option : SerialIO - ",
        "Enable\n",
        "Disable\n"
    };
    TRKReplyPacket reply;
    const char* text;
    u8 value;

    text = option_text.label;
    value = message->data[0x0C];
    if (message->data[0x08] == 1) {
        usr_puts_serial(text);
        if (value != 0) {
            usr_puts_serial(text + 32);
        } else {
            usr_puts_serial(text + 40);
        }
        SetUseSerialIO(value);
    }

    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = 0;
    TRKWriteUARTN(&reply, sizeof(reply));
    return 0;
}

DSError TRKDoStop(MessageBuffer* message)
{
    TRKReplyPacket reply;
    DSError error;
    u8 reply_error;

    error = TRKTargetStop();
    switch (error) {
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

    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = reply_error;
    TRKWriteUARTN(&reply, sizeof(reply));
    return 0;
}

DSError TRKDoStep(MessageBuffer* message)
{
    TRKReplyPacket bad_count_reply;
    TRKReplyPacket bad_range_reply;
    TRKReplyPacket bad_mode_reply;
    TRKReplyPacket running_reply;
    TRKReplyPacket success_reply;
    u8 mode;
    u8 count;
    u32 range_start;
    u32 range_end;
    u32 pc;
    DSError result;

    TRKSetBufferPosition(message, 0);
    mode = message->data[0x08];
    range_start = *(u32*)&message->data[0x10];
    range_end = *(u32*)&message->data[0x14];

    switch (mode) {
    case 0:
    case 0x10:
        count = message->data[0x0C];
        if (count < 1) {
            memset(&bad_count_reply, 0, sizeof(bad_count_reply));
            bad_count_reply.command = 0x80;
            bad_count_reply.length = sizeof(bad_count_reply);
            bad_count_reply.error = 0x11;
            TRKWriteUARTN(&bad_count_reply, sizeof(bad_count_reply));
            return 0;
        }
        break;
    case 1:
    case 0x11:
        pc = TRKTargetGetPC();
        if (pc < range_start || pc > range_end) {
            memset(&bad_range_reply, 0, sizeof(bad_range_reply));
            bad_range_reply.command = 0x80;
            bad_range_reply.length = sizeof(bad_range_reply);
            bad_range_reply.error = 0x11;
            TRKWriteUARTN(&bad_range_reply, sizeof(bad_range_reply));
            return 0;
        }
        break;
    default:
        memset(&bad_mode_reply, 0, sizeof(bad_mode_reply));
        bad_mode_reply.command = 0x80;
        bad_mode_reply.length = sizeof(bad_mode_reply);
        bad_mode_reply.error = 0x12;
        TRKWriteUARTN(&bad_mode_reply, sizeof(bad_mode_reply));
        return 0;
    }

    if (!TRKTargetStopped()) {
        memset(&running_reply, 0, sizeof(running_reply));
        running_reply.command = 0x80;
        running_reply.length = sizeof(running_reply);
        running_reply.error = 0x16;
        TRKWriteUARTN(&running_reply, sizeof(running_reply));
        return 0;
    }

    memset(&success_reply, 0, sizeof(success_reply));
    success_reply.command = 0x80;
    success_reply.length = sizeof(success_reply);
    success_reply.error = 0;
    TRKWriteUARTN(&success_reply, sizeof(success_reply));

    result = 0;
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

DSError TRKDoContinue(MessageBuffer* message)
{
    TRKReplyPacket error_reply;
    TRKReplyPacket success_reply;

    MWTRACE(1, "DoContinue\n");
    if (!TRKTargetStopped()) {
        memset(&error_reply, 0, sizeof(error_reply));
        error_reply.command = 0x80;
        error_reply.length = sizeof(error_reply);
        error_reply.error = 0x16;
        TRKWriteUARTN(&error_reply, sizeof(error_reply));
        return 0;
    }

    memset(&success_reply, 0, sizeof(success_reply));
    success_reply.command = 0x80;
    success_reply.length = sizeof(success_reply);
    success_reply.error = 0;
    TRKWriteUARTN(&success_reply, sizeof(success_reply));
    return TRKTargetContinue();
}

DSError TRKDoSupportMask(MessageBuffer* message)
{
    return 0;
}

DSError TRKDoVersions(MessageBuffer* message)
{
    return 0;
}

DSError TRKDoOverride(MessageBuffer* message)
{
    TRKReplyPacket reply;

    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = 0;
    TRKWriteUARTN(&reply, sizeof(reply));
    __TRK_copy_vectors();
    return 0;
}

DSError TRKDoReset(MessageBuffer* message)
{
    TRKReplyPacket reply;

    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = 0;
    TRKWriteUARTN(&reply, sizeof(reply));
    __TRK_reset();
    return 0;
}

DSError TRKDoDisconnect(MessageBuffer* message)
{
    TRKReplyPacket reply;
    TRKEvent event;

    IsTRKConnected = 0;
    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = 0;
    TRKWriteUARTN(&reply, sizeof(reply));
    TRKConstructEvent(&event, 1);
    TRKPostEvent(&event);
    return 0;
}

DSError TRKDoConnect(MessageBuffer* message)
{
    TRKReplyPacket reply;

    IsTRKConnected = 1;
    memset(&reply, 0, sizeof(reply));
    reply.command = 0x80;
    reply.length = sizeof(reply);
    reply.error = 0;
    TRKWriteUARTN(&reply, sizeof(reply));
    return 0;
}

void SetTRKConnected(u32 connected)
{
    IsTRKConnected = connected;
}

u32 GetTRKConnected(void)
{
    return IsTRKConnected;
}

/* TODO: [near miss] 98.095240%; loop, modulo, calls, and size match; static diagnostic symbols leave only nonvolatile coloring/relocation residue. */
void OutputData(const u8* data, s32 length)
{
    static const char output_format[] = "%02x ";
    static const char output_newline[] = "\n";
    const u8* cursor;
    s32 index;

    cursor = data;
    for (index = 0; index < length; index++, cursor++) {
        MWTRACE(8, output_format, *cursor);
        if (index % 16 == 15) {
            MWTRACE(8, output_newline);
        }
    }
    MWTRACE(8, output_newline);
}
