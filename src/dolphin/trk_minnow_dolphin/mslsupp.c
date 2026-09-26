#include "dolphin/trk.h"
#include "runtime/cfile.h"

extern u8 GetUseSerialIO(void);
extern u8 TRKAccessFile(u8 command, file_handle handle, size_t* count, u8* buffer);
extern u8 TRKOpenFile(u8 command, const char* name, u8 mode, file_handle* handle);
extern u8 TRKCloseFile(u8 command, file_handle handle);
extern u8 TRKPositionFile(u8 command, file_handle handle,
                         file_position* position, u8 mode);

enum {
    TRK_WRITE_FILE = 0xD0,
    TRK_READ_FILE = 0xD1,
    TRK_OPEN_FILE = 0xD2,
    TRK_CLOSE_FILE = 0xD3,
    TRK_POSITION_FILE = 0xD4,
};

typedef enum {
    TRK_FILE_OPEN_READ = 0x01,
    TRK_FILE_OPEN_WRITE = 0x02,
    TRK_FILE_OPEN_APPEND = 0x04,
    TRK_FILE_OPEN_BINARY = 0x08,
    TRK_FILE_OPEN_CREATE = 0x10,
} TRKFileOpenMode;

int __position_file(file_handle handle, file_position* position, int mode,
                    IdleProc idle)
{
    int position_mode = 0;
    u8 result;
    BOOL connected;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    if (mode == 0) {
        position_mode = 0;
    } else if (mode == 1) {
        position_mode = 1;
    } else if (mode == 2) {
        position_mode = 2;
    }
    result = TRKPositionFile(TRK_POSITION_FILE, handle, position, position_mode);
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __close_file(file_handle handle)
{
    u8 result;
    BOOL connected;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    result = TRKCloseFile(TRK_CLOSE_FILE, handle);
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __open_file(const char* name, FileMode* mode, FILE* file)
{
    TRKFileOpenMode trk_mode;
    int open_mode;
    int io_mode;
    unsigned int binary;
    BOOL connected;
    u8 result;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    open_mode = mode->open_mode;
    io_mode = mode->io_mode;
    binary = mode->binary_io;
    trk_mode = 0;
    switch (open_mode) {
    case 0: trk_mode |= 1; break;
    case 2: trk_mode |= 2; break;
    case 1: trk_mode |= 4; break;
    }
    switch (io_mode) {
    case 1: trk_mode |= 1; break;
    case 2: trk_mode |= 2; break;
    case 6: trk_mode |= 4; break;
    case 3: trk_mode |= 0x12; break;
    case 7: trk_mode |= 7; break;
    }
    if (binary == 1) {
        trk_mode |= 8;
    }
    result = TRKOpenFile(TRK_OPEN_FILE, name, trk_mode, &file->handle);
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __write_file(file_handle handle, u8* buffer, size_t* count, IdleProc idle)
{
    size_t transferred;
    u8 result;
    BOOL connected;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    transferred = *count;
    result = TRKAccessFile(TRK_WRITE_FILE, handle, &transferred, buffer);
    *count = transferred;
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __read_file(file_handle handle, u8* buffer, size_t* count, IdleProc idle)
{
    size_t transferred;
    u8 result;
    BOOL connected;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    transferred = *count;
    result = TRKAccessFile(TRK_READ_FILE, handle, &transferred, buffer);
    *count = transferred;
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __close_console(file_handle handle)
{
    u8 result;
    BOOL connected;

    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    result = TRKCloseFile(TRK_CLOSE_FILE, handle);
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __TRK_write_console(file_handle handle, u8* buffer, size_t* count,
                        IdleProc idle)
{
    size_t transferred;
    u8 result;
    BOOL connected;

    if (!GetUseSerialIO()) {
        return 1;
    }
    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    transferred = *count;
    result = TRKAccessFile(TRK_WRITE_FILE, 1, &transferred, buffer);
    *count = transferred;
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}

int __read_console(file_handle handle, u8* buffer, size_t* count, IdleProc idle)
{
    size_t transferred;
    u8 result;
    BOOL connected;

    if (!GetUseSerialIO()) {
        return 1;
    }
    connected = GetTRKConnected();
    if (!connected) {
        return 1;
    }
    transferred = *count;
    result = TRKAccessFile(TRK_READ_FILE, 0, &transferred, buffer);
    *count = transferred;
    switch (result) {
    case 0: return 0;
    case 2: return 2;
    default: return 1;
    }
}
