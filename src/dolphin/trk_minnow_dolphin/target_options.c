#include "dolphin/trk.h"

static u8 bUseSerialIO;

void SetUseSerialIO(u8 serial_io)
{
    bUseSerialIO = serial_io;
}

u8 GetUseSerialIO(void)
{
    return bUseSerialIO;
}
