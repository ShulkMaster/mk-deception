#include "runtime/cfile.h"

enum {
    FILE_KIND_CLOSED,
    FILE_KIND_DISK,
    FILE_KIND_CONSOLE,
};

enum {
    IO_STATE_NEUTRAL,
    IO_STATE_WRITING,
    IO_STATE_READING,
    IO_STATE_REREADING,
};

enum {
    BUFFER_MODE_FULL,
    BUFFER_MODE_LINE,
    BUFFER_MODE_UNBUFFERED,
};

extern void __begin_critical_region(int region);
extern void __end_critical_region(int region);
extern void __stdio_atexit(void);
extern void __prep_buffer(FILE* file);
extern int __flush_buffer(FILE* file, size_t* bytes_flushed);
extern int __load_buffer(FILE* file, size_t* bytes_loaded, int alignment_mode);
extern int fwide(FILE* file, int mode);

int __put_char(int character, FILE* file);

static inline int get_char(FILE* file) {
    u32 available;
    u8* current;
    int state;
    int load_result;

    if (fwide(file, -1) >= 0) {
        return -1;
    }
    available = file->buffer_length;
    file->buffer_length = available - 1;
    if (available != 0) {
        current = file->buffer_ptr++;
        return *current;
    }

    file->buffer_length = 0;
    if (file->state.error || file->mode.bits.file_kind == FILE_KIND_CLOSED) {
        return -1;
    }
    state = file->state.io_state;
    if (state == IO_STATE_WRITING || !(file->mode.bits.io_mode & 1)) {
        file->state.error = 1;
        file->buffer_length = 0;
        return -1;
    }
    if (state >= IO_STATE_REREADING) {
        file->state.io_state = state - 1;
        if (state == IO_STATE_REREADING) {
            file->buffer_length = file->saved_buffer_length;
        }
        return file->ungetc_buffer[state - IO_STATE_REREADING];
    }

    file->state.io_state = IO_STATE_READING;
    if ((load_result = __load_buffer(file, 0, 0)) != 0 ||
        (available = file->buffer_length) == 0) {
        if (load_result == 1) {
            file->state.error = 1;
            file->buffer_length = 0;
        } else {
            file->state.io_state = IO_STATE_NEUTRAL;
            file->state.eof = 1;
            file->buffer_length = 0;
        }
        return -1;
    }
    file->buffer_length = available - 1;
    current = file->buffer_ptr++;
    return *current;
}

int fputs(const char* string, FILE* file) {
    signed char character;
    int written;
    u32 available;
    int result = 0;

    __begin_critical_region(2);
    while ((character = *string++) != 0) {
        if (fwide(file, -1) >= 0) {
            written = -1;
        } else {
            available = file->buffer_length;
            file->buffer_length = available - 1;
            if (available != 0) {
                written = character & 0xFF;
                *file->buffer_ptr++ = (u8)written;
            } else {
                written = __put_char(character, file);
            }
        }
        if (written == -1) {
            result = -1;
            break;
        }
    }
    __end_critical_region(2);
    return result;
}

int __put_char(int character, FILE* file) {
    int file_kind;
    int buffer_mode;

    file->buffer_length = 0;
    file_kind = file->mode.bits.file_kind;
    if (file->state.error || file_kind == FILE_KIND_CLOSED) {
        return -1;
    }
    if (file_kind == FILE_KIND_CONSOLE) {
        __stdio_atexit();
    }
    if (file->state.io_state == IO_STATE_NEUTRAL &&
        (file->mode.bits.io_mode & 2)) {
        if ((file->mode.bits.io_mode & 4) && fseek(file, 0, 2) != 0) {
            return 0;
        }
        file->state.io_state = IO_STATE_WRITING;
        __prep_buffer(file);
    }
    if (file->state.io_state != IO_STATE_WRITING) {
        file->state.error = 1;
        file->buffer_length = 0;
        return -1;
    }

    buffer_mode = file->mode.bits.buffer_mode;
    if ((buffer_mode == BUFFER_MODE_UNBUFFERED ||
         file->buffer_size == (u32)(file->buffer_ptr - file->buffer)) &&
        __flush_buffer(file, 0) != 0) {
        file->state.error = 1;
        file->buffer_length = 0;
        return -1;
    }

    file->buffer_length--;
    *file->buffer_ptr++ = (u8)character;
    if (buffer_mode != BUFFER_MODE_UNBUFFERED) {
        if ((buffer_mode == BUFFER_MODE_FULL || character == '\n') &&
            __flush_buffer(file, 0) != 0) {
            file->state.error = 1;
            file->buffer_length = 0;
            return -1;
        }
        file->buffer_length = 0;
    }
    return (u8)character;
}

char* fgets(char* buffer, int count, FILE* file) {
    int remaining = count - 1;
    char* output = buffer;

    if (remaining < 0) {
        return 0;
    }
    __begin_critical_region(2);
    if (remaining != 0) {
        do {
            int character = get_char(file);

            if (character == -1) {
                if (!file->state.eof || output == buffer) {
                    __end_critical_region(2);
                    return 0;
                }
                break;
            }
            *output++ = (char)character;
            if (character == '\n') {
                break;
            }
        } while (--remaining != 0);
    }
    __end_critical_region(2);
    *output = '\0';
    return buffer;
}
