#include "runtime/cfile.h"

extern int errno;
extern void __begin_critical_region(int region);
extern void __end_critical_region(int region);
extern int __flush_buffer(FILE* file, size_t* bytes_flushed);

inline static int file_tell(FILE* file)
{
    int chars_in_undo_buffer = 0;
    int position;
    u8 kind = file->mode.bits.file_kind;

    if ((kind != 1 && kind != 2) || file->state.error) {
        errno = 0x28;
        return -1;
    }
    if (file->state.io_state == 0)
        return file->position;

    position = file->buffer_position + (file->buffer_ptr - file->buffer);
    if (file->state.io_state >= 3) {
        chars_in_undo_buffer = file->state.io_state - 2;
        position -= chars_in_undo_buffer;
    }
    if (!file->mode.bits.binary_io) {
        int count = file->buffer_ptr - file->buffer - chars_in_undo_buffer;
        u8* current = file->buffer;

        while (count-- != 0) {
            if (*current++ == '\n')
                position++;
        }
    }
    return position;
}

int _fseek(FILE* file, u32 offset, int whence);

int fseek(FILE* stream, u32 offset, int whence)
{
    int code;
    __begin_critical_region(2);
    code = _fseek(stream, offset, whence);
    __end_critical_region(2);
    return code;
}

int _fseek(FILE* file, u32 offset, int whence)
{
    u32 position;
    PositionProc function;

    if (file->mode.bits.file_kind != 1 || file->state.error != 0) {
        errno = 0x28;
        return -1;
    }
    if (file->state.io_state == 1) {
        if (__flush_buffer(file, 0) != 0) {
            file->state.error = 1;
            file->buffer_length = 0;
            errno = 0x28;
            return -1;
        }
    }
    if (whence == 1) {
        whence = 0;
        position = file_tell(file);
        offset += position;
    }
    if (whence != 2 && file->mode.bits.io_mode != 3 &&
        (file->state.io_state == 2 || file->state.io_state == 3)) {
        if (offset >= file->position || offset < file->buffer_position) {
            file->state.io_state = 0;
        } else {
            file->buffer_ptr = file->buffer + (offset - file->buffer_position);
            file->buffer_length = file->position - offset;
            file->state.io_state = 2;
        }
    } else {
        file->state.io_state = 0;
    }
    if (file->state.io_state == 0) {
        function = file->position_proc;
        if (function != 0 && function(file->handle, &offset, whence, file->idle_proc) != 0) {
            file->state.error = 1;
            file->buffer_length = 0;
            errno = 0x28;
            return -1;
        } else {
            file->state.eof = 0;
            file->position = offset;
            file->buffer_length = 0;
        }
    }
    return 0;
}

long ftell(FILE* stream)
{
    int result;
    __begin_critical_region(2);
    result = file_tell(stream);
    __end_critical_region(2);
    return result;
}
