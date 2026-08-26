typedef unsigned char u8;
typedef unsigned long u32;
typedef unsigned long size_t;

typedef struct FileModes {
    unsigned int open_mode : 2;
    unsigned int io_mode : 3;
    unsigned int buffer_mode : 2;
    unsigned int file_kind : 3;
    unsigned int file_orientation : 2;
    unsigned int binary_io : 1;
} FileModes;

typedef union FileModeWord {
    FileModes bits;
    unsigned short half[2];
    unsigned int value;
} FileModeWord;

typedef struct FileStates {
    unsigned int io_state : 3;
    unsigned int free_buffer : 1;
    unsigned char eof;
    unsigned char error;
} FileStates;

typedef void (*IdleProc)(void);
typedef int (*PosProc)(u32 handle, u32* position, int mode, IdleProc idle);

typedef struct FILE {
    u32 handle;
    FileModeWord file_mode;
    FileStates file_state;
    u8 character_state[12];
    u32 position;
    u8* buffer;
    u32 buffer_size;
    u8* buffer_ptr;
    u32 buffer_length;
    u32 buffer_alignment;
    u32 save_buffer_length;
    u32 buffer_position;
    PosProc position_fn;
    void* read_fn;
    void* write_fn;
    void* close_fn;
    IdleProc idle_fn;
} FILE;

extern int errno;
extern void __begin_critical_region(int region);
extern void __end_critical_region(int region);
extern int __flush_buffer(FILE* file, size_t* bytes_flushed);

inline static int file_tell(FILE* file)
{
    int chars_in_undo_buffer = 0;
    int position;
    u8 kind = file->file_mode.bits.file_kind;

    if ((kind != 1 && kind != 2) || file->file_state.error) {
        errno = 0x28;
        return -1;
    }
    if (file->file_state.io_state == 0)
        return file->position;

    position = file->buffer_position + (file->buffer_ptr - file->buffer);
    if (file->file_state.io_state >= 3) {
        chars_in_undo_buffer = file->file_state.io_state - 2;
        position -= chars_in_undo_buffer;
    }
    if (!file->file_mode.bits.binary_io) {
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
    PosProc function;

    if (file->file_mode.bits.file_kind != 1 || file->file_state.error != 0) {
        errno = 0x28;
        return -1;
    }
    if (file->file_state.io_state == 1) {
        if (__flush_buffer(file, 0) != 0) {
            file->file_state.error = 1;
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
    if (whence != 2 && file->file_mode.bits.io_mode != 3 &&
        (file->file_state.io_state == 2 || file->file_state.io_state == 3)) {
        if (offset >= file->position || offset < file->buffer_position) {
            file->file_state.io_state = 0;
        } else {
            file->buffer_ptr = file->buffer + (offset - file->buffer_position);
            file->buffer_length = file->position - offset;
            file->file_state.io_state = 2;
        }
    } else {
        file->file_state.io_state = 0;
    }
    if (file->file_state.io_state == 0) {
        function = file->position_fn;
        if (function != 0 && function(file->handle, &offset, whence, file->idle_fn) != 0) {
            file->file_state.error = 1;
            file->buffer_length = 0;
            errno = 0x28;
            return -1;
        } else {
            file->file_state.eof = 0;
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
