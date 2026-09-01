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

extern DSError TRKWriteUARTN(const void* data, u32 length);
extern void __TRK_copy_vectors(void);

static BOOL IsTRKConnected;

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

void SetTRKConnected(BOOL connected)
{
    IsTRKConnected = connected;
}

BOOL GetTRKConnected(void)
{
    return IsTRKConnected;
}
