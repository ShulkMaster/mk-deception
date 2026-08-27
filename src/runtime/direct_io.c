#include "runtime/cfile.h"

extern void __begin_critical_region(int region);
extern void __end_critical_region(int region);
extern void __stdio_atexit(void);
extern void __prep_buffer(FILE* file);
extern int __flush_buffer(FILE* file, size_t* bytes_flushed);
extern int __load_buffer(FILE* file, size_t* bytes_loaded, int alignment_mode);
extern int __flush_line_buffered_output_files(void);
extern int fwide(FILE* file, int mode);
extern void* memcpy(void* destination, const void* source, size_t count);
extern void* __memrchr(const void* memory, int character, size_t count);

size_t __fwrite(const void* data, size_t member_size, size_t member_count,
                FILE* file) {
    const u8* current;
    size_t remaining;
    size_t transferred;
    size_t count;
    int buffered;

    if (fwide(file, 0) == 0) {
        fwide(file, -1);
    }
    remaining = member_size * member_count;
    if (remaining == 0 || file->state.error || file->mode.bits.file_kind == 0) {
        return 0;
    }
    if (file->mode.bits.file_kind == 2) {
        __stdio_atexit();
    }
    buffered = !file->mode.bits.binary_io || file->mode.bits.buffer_mode == 2 ||
        file->mode.bits.buffer_mode == 1;
    if (file->state.io_state == 0 && (file->mode.bits.io_mode & 2)) {
        if ((file->mode.bits.io_mode & 4) && fseek(file, 0, 2) != 0) {
            return 0;
        }
        file->state.io_state = 1;
        __prep_buffer(file);
    }
    if (file->state.io_state != 1) {
        file->state.error = 1;
        file->buffer_length = 0;
        return 0;
    }

    current = data;
    transferred = 0;
    if (remaining != 0 && (file->buffer_ptr != file->buffer || buffered)) {
        file->buffer_length = file->buffer_size -
            (file->buffer_ptr - file->buffer);
        do {
            const u8* newline = 0;

            count = file->buffer_length;
            if (count > remaining) {
                count = remaining;
            }
            if (file->mode.bits.buffer_mode == 1 && count != 0) {
                newline = __memrchr(current, '\n', count);
                if (newline != 0) {
                    count = newline + 1 - current;
                }
            }
            if (count != 0) {
                memcpy(file->buffer_ptr, current, count);
                current += count;
                transferred += count;
                remaining -= count;
                file->buffer_ptr += count;
                file->buffer_length -= count;
            }
            if ((file->buffer_length == 0 || newline != 0 ||
                 file->mode.bits.buffer_mode == 0) &&
                __flush_buffer(file, 0) != 0) {
                file->state.error = 1;
                file->buffer_length = 0;
                remaining = 0;
                break;
            }
        } while (remaining != 0 && buffered);
    }
    if (remaining != 0 && !buffered) {
        u8* saved_buffer = file->buffer;
        size_t saved_size = file->buffer_size;

        file->buffer = (u8*)current;
        file->buffer_size = remaining;
        file->buffer_ptr = (u8*)current + remaining;
        if (__flush_buffer(file, &count) != 0) {
            file->state.error = 1;
            file->buffer_length = 0;
        }
        transferred += count;
        file->buffer = saved_buffer;
        file->buffer_size = saved_size;
        __prep_buffer(file);
        file->buffer_length = 0;
    }
    if (file->mode.bits.buffer_mode != 2) {
        file->buffer_length = 0;
    }
    return (transferred + member_size - 1) / member_size;
}

size_t fwrite(const void* data, size_t member_size, size_t member_count,
              FILE* file) {
    size_t result;

    __begin_critical_region(2);
    result = __fwrite(data, member_size, member_count, file);
    __end_critical_region(2);
    return result;
}

size_t __fread(void* data, size_t member_size, size_t member_count, FILE* file) {
    u8* current;
    size_t remaining;
    size_t transferred;
    size_t count;
    int buffered;
    int load_result;

    if (fwide(file, 0) == 0) {
        fwide(file, -1);
    }
    remaining = member_size * member_count;
    if (remaining == 0 || file->state.error || file->mode.bits.file_kind == 0) {
        return 0;
    }
    buffered = 1;
    if (file->mode.bits.binary_io && file->mode.bits.buffer_mode != 2) {
        buffered = 0;
    }
    if (file->state.io_state == 0 && (file->mode.bits.io_mode & 1)) {
        file->state.io_state = 2;
        file->buffer_length = 0;
    }
    if (file->state.io_state < 2) {
        file->state.error = 1;
        file->buffer_length = 0;
        return 0;
    }
    if ((file->mode.bits.buffer_mode & 1) &&
        __flush_line_buffered_output_files() != 0) {
        file->state.error = 1;
        file->buffer_length = 0;
        return 0;
    }

    current = data;
    transferred = 0;
    if (remaining != 0 && file->state.io_state >= 3) {
        do {
            int state = file->state.io_state;

            if (fwide(file, 0) == 1) {
                *(u16*)current = file->ungetc_wide_buffer[state - 3];
                current += 2;
                transferred += 2;
                remaining -= 2;
            } else {
                *current++ = file->ungetc_buffer[state - 3];
                transferred++;
                remaining--;
            }
            file->state.io_state = state - 1;
        } while (remaining != 0 && file->state.io_state >= 3);
    }
    if (file->state.io_state == 2) {
        file->buffer_length = file->saved_buffer_length;
    }

    if (remaining != 0 && (file->buffer_length != 0 || buffered)) {
        do {
            if (file->buffer_length == 0) {
                load_result = __load_buffer(file, 0, 0);
                if (load_result != 0) {
                    if (load_result == 1) {
                        file->state.error = 1;
                        file->buffer_length = 0;
                    } else {
                        file->state.io_state = 0;
                        file->state.eof = 1;
                        file->buffer_length = 0;
                    }
                    remaining = 0;
                    break;
                }
            }
            count = file->buffer_length;
            if (count > remaining) {
                count = remaining;
            }
            memcpy(current, file->buffer_ptr, count);
            remaining -= count;
            current += count;
            transferred += count;
            file->buffer_ptr += count;
            file->buffer_length -= count;
        } while (remaining != 0 && buffered);
    }
    if (remaining != 0 && !buffered) {
        u8* saved_buffer = file->buffer;
        size_t saved_size = file->buffer_size;

        file->buffer = current;
        file->buffer_size = remaining;
        load_result = __load_buffer(file, &count, 1);
        if (load_result != 0) {
            if (load_result == 1) {
                file->state.error = 1;
                file->buffer_length = 0;
            } else {
                file->state.io_state = 0;
                file->state.eof = 1;
                file->buffer_length = 0;
            }
        }
        file->buffer = saved_buffer;
        transferred += count;
        file->buffer_size = saved_size;
        __prep_buffer(file);
        file->buffer_length = 0;
    }
    return transferred / member_size;
}

size_t fread(void* data, size_t member_size, size_t member_count, FILE* file) {
    size_t result;

    __begin_critical_region(2);
    result = __fread(data, member_size, member_count, file);
    __end_critical_region(2);
    return result;
}
