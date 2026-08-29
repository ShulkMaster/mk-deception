#include "runtime/cfile.h"
#include "runtime/critical_regions.h"
#include "runtime/cstdlib.h"
#include "ctype.h"

enum {
    FILE_KIND_CLOSED,
    FILE_KIND_DISK,
};

enum {
    IO_MODE_READ = 1,
    IO_MODE_WRITE = 2,
    IO_MODE_READ_WRITE = 3,
    IO_MODE_APPEND = 4,
};

enum {
    IO_STATE_NEUTRAL,
    IO_STATE_WRITING,
    IO_STATE_READING,
    IO_STATE_REREADING,
};

/* These helpers precede fopen so MWCC can inline its nested close/flush path
 * while preserving the retail order of the public symbols below. */
static inline int flush_file(FILE* file)
{
    int position;

    if (file == 0) {
        return __flush_all();
    }
    if (file->state.error || file->mode.bits.file_kind == FILE_KIND_CLOSED) {
        return -1;
    }
    if (file->mode.bits.io_mode == IO_MODE_READ) {
        return 0;
    }
    if (file->state.io_state >= IO_STATE_REREADING) {
        file->state.io_state = IO_STATE_READING;
    }
    if (file->state.io_state == IO_STATE_READING) {
        file->buffer_length = 0;
    }
    if (file->state.io_state != IO_STATE_WRITING) {
        file->state.io_state = IO_STATE_NEUTRAL;
        return 0;
    }
    if (file->mode.bits.file_kind != FILE_KIND_DISK) {
        position = 0;
    } else {
        position = ftell(file);
    }
    if (__flush_buffer(file, 0) != 0) {
        file->state.error = 1;
        file->buffer_length = 0;
        return -1;
    }
    file->state.io_state = IO_STATE_NEUTRAL;
    file->position = position;
    file->buffer_length = 0;
    return 0;
}

static inline int close_file(FILE* file)
{
    int flush_result;
    int close_result;

    if (file == 0) {
        return -1;
    }
    if (file->mode.bits.file_kind == FILE_KIND_CLOSED) {
        return 0;
    }
    flush_result = flush_file(file);
    close_result = file->close_proc(file->handle);
    file->mode.bits.file_kind = FILE_KIND_CLOSED;
    file->handle = 0;
    if (file->state.free_buffer) {
        free(file->buffer);
    }
    return flush_result || close_result ? -1 : 0;
}

int __msl_strnicmp(const char* first, const char* second, int count)
{
    int i;
    char first_char;
    char second_char;

    for (i = 0; i < count; i++) {
        first_char = _tolower(*first++);
        second_char = _tolower(*second++);

        if (first_char < second_char) {
            return -1;
        }
        if (first_char > second_char) {
            return 1;
        }
        if (first_char == '\0') {
            return 0;
        }
    }
    return 0;
}

int __get_file_modes(const char* mode_string, FileMode* mode)
{
    const char* option;
    int mode_key;
    u8 open_mode;
    int io_mode;

    mode->file_kind = FILE_KIND_DISK;
    mode->file_orientation = 0;
    mode->binary_io = 0;
    mode_key = *mode_string++;

    switch (mode_key) {
    case 'r':
        open_mode = 0;
        break;
    case 'w':
        open_mode = 2;
        break;
    case 'a':
        open_mode = 1;
        break;
    default:
        return 0;
    }

    mode->open_mode = open_mode;
    option = mode_string;
    switch (*option++) {
    case 'b':
        mode->binary_io = 1;
        if (*option == '+') {
            mode_key = (mode_key << 8) | '+';
        }
        break;
    case '+':
        mode_key = (mode_key << 8) | '+';
        if (*option == 'b') {
            mode->binary_io = 1;
        }
        break;
    }

    switch (mode_key) {
    case 'r':
        io_mode = IO_MODE_READ;
        break;
    case 'w':
        io_mode = IO_MODE_WRITE;
        break;
    case 'a':
        io_mode = IO_MODE_WRITE | IO_MODE_APPEND;
        break;
    case ('r' << 8) | '+':
        io_mode = IO_MODE_READ_WRITE;
        break;
    case ('w' << 8) | '+':
        io_mode = IO_MODE_READ_WRITE;
        break;
    case ('a' << 8) | '+':
        io_mode = IO_MODE_READ_WRITE | IO_MODE_APPEND;
        break;
    }

    mode->io_mode = io_mode;
    return 1;
}

FILE* fopen(const char* name, const char* mode_string)
{
    FILE* file;
    FileMode open_mode;
    FileMode init_mode;
    FileMode mode;

    __begin_critical_region(2);
    file = __find_unopened_file();
    __stdio_atexit();
    if (file == 0) {
        file = 0;
    } else {
        close_file(file);
        clearerr(file);
        if (!__get_file_modes(mode_string, &mode)) {
            file = 0;
        } else {
            init_mode = mode;
            __init_file(file, &init_mode, 0, 0x400);
            open_mode = mode;
            if (__open_file(name, &open_mode, file) != 0) {
                file->mode.bits.file_kind = FILE_KIND_CLOSED;
                if (file->state.free_buffer) {
                    free(file->buffer);
                }
                file = 0;
            } else if (mode.io_mode & IO_MODE_APPEND) {
                fseek(file, 0, 2);
            }
        }
    }
    __end_critical_region(2);
    return file;
}

int fflush(FILE* file)
{
    int position;

    if (file == 0) {
        return __flush_all();
    }
    if (file->state.error || file->mode.bits.file_kind == FILE_KIND_CLOSED) {
        return -1;
    }
    if (file->mode.bits.io_mode == IO_MODE_READ) {
        return 0;
    }
    if (file->state.io_state >= IO_STATE_REREADING) {
        file->state.io_state = IO_STATE_READING;
    }
    if (file->state.io_state == IO_STATE_READING) {
        file->buffer_length = 0;
    }
    if (file->state.io_state != IO_STATE_WRITING) {
        file->state.io_state = IO_STATE_NEUTRAL;
        return 0;
    }
    if (file->mode.bits.file_kind != FILE_KIND_DISK) {
        position = 0;
    } else {
        position = ftell(file);
    }
    if (__flush_buffer(file, 0) != 0) {
        file->state.error = 1;
        file->buffer_length = 0;
        return -1;
    }
    file->state.io_state = IO_STATE_NEUTRAL;
    file->position = position;
    file->buffer_length = 0;
    return 0;
}

int fclose(FILE* file)
{
    return close_file(file);
}
