#include "runtime/cfile.h"

extern void __begin_critical_region(int region);
extern void __end_critical_region(int region);
extern void free(void* allocation);
extern void* malloc(size_t size);

int setvbuf(FILE* file, char* buffer, int mode, size_t size)
{
    int io_mode;

    io_mode = (file->mode.half[0] >> 6) & 7;
    if (mode == 0)
        fflush(file);
    if (file->state.io_state != 0 || io_mode == 0)
        return -1;
    if (mode != 0 && mode != 1 && mode != 2)
        return -1;
    if (file->buffer != 0 && file->state.free_buffer)
        free(file->buffer);

    __begin_critical_region(2);
    file->mode.bits.buffer_mode = mode;
    file->state.free_buffer = 0;
    file->buffer = (u8*)&file->char_buffer;
    file->buffer_ptr = (u8*)&file->char_buffer;
    file->buffer_size = 1;
    file->buffer_length = 0;
    file->buffer_alignment = 0;
    if (mode == 0 || size < 1) {
        *file->buffer_ptr = 0;
        __end_critical_region(2);
        return 0;
    }
    if (buffer == 0) {
        buffer = malloc(size);
        if (buffer == 0) {
            __end_critical_region(2);
            return -1;
        }
        file->state.free_buffer = 1;
    }
    file->buffer = (u8*)buffer;
    file->buffer_ptr = file->buffer;
    file->buffer_size = size;
    file->buffer_alignment = 0;
    __end_critical_region(2);
    return 0;
}

int __flush_buffer(FILE* file, size_t* bytes_flushed)
{
    size_t buffer_len;
    int io_result;

    buffer_len = file->buffer_ptr - file->buffer;
    if (buffer_len != 0) {
        file->buffer_length = buffer_len;
        io_result = file->write_proc(file->handle, file->buffer, &file->buffer_length, file->idle_proc);
        if (bytes_flushed != 0)
            *bytes_flushed = file->buffer_length;
        if (io_result != 0)
            return io_result;
        file->position += file->buffer_length;
    }
    file->buffer_ptr = file->buffer;
    file->buffer_length = file->buffer_size;
    file->buffer_length -= file->position & file->buffer_alignment;
    file->buffer_position = file->position;
    return 0;
}

int __load_buffer(FILE* file, size_t* bytes_loaded, int alignment_mode)
{
    int io_result;
    int count;
    u8* current;

    file->buffer_ptr = file->buffer;
    file->buffer_length = file->buffer_size;
    file->buffer_length -= file->position & file->buffer_alignment;
    file->buffer_position = file->position;
    if (alignment_mode == 1)
        file->buffer_length = file->buffer_size;

    io_result = file->read_proc(file->handle, file->buffer, &file->buffer_length, file->idle_proc);
    if (io_result == 2)
        file->buffer_length = 0;
    if (bytes_loaded != 0)
        *bytes_loaded = file->buffer_length;
    if (io_result != 0)
        return io_result;

    file->position += file->buffer_length;
    if (!file->mode.bits.binary_io) {
        count = file->buffer_length;
        current = file->buffer;
        while (count-- != 0) {
            if (*current++ == '\n')
                file->position += 1;
        }
    }
    return 0;
}

void __prep_buffer(FILE* file)
{
    file->buffer_ptr = file->buffer;
    file->buffer_length = file->buffer_size;
    file->buffer_length -= file->position & file->buffer_alignment;
    file->buffer_position = file->position;
}
