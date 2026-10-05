#include "dolphin/trk.h"
#include "runtime/cfile.h"
#include "runtime/asm_sequences.inc"

asm u8 TRKAccessFile(u8 command, file_handle handle, size_t* count, u8* buffer)
{
    SEQ_TRKAccessFile();
}

asm u8 TRKOpenFile(u8 command, const char* name, u8 mode, file_handle* handle)
{
    SEQ_TRKOpenFile();
}

asm u8 TRKCloseFile(u8 command, file_handle handle)
{
    SEQ_TRKCloseFile();
}

asm u8 TRKPositionFile(u8 command, file_handle handle, file_position* position,
                       u8 mode)
{
    SEQ_TRKPositionFile();
}
