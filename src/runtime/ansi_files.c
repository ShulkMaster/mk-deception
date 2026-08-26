typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long size_t;

typedef unsigned long file_handle;
typedef unsigned long file_position;

enum FileKind {
    FILE_KIND_CLOSED,
    FILE_KIND_DISK,
    FILE_KIND_CONSOLE,
    FILE_KIND_UNAVAILABLE,
};

enum BufferMode {
    BUFFER_MODE_UNBUFFERED,
    BUFFER_MODE_LINE,
    BUFFER_MODE_FULL,
};

enum IOState {
    IO_STATE_NEUTRAL,
    IO_STATE_WRITING,
    IO_STATE_READING,
    IO_STATE_REREADING,
};

typedef struct FileMode {
    u32 open_mode : 2;
    u32 io_mode : 3;
    u32 buffer_mode : 2;
    u32 file_kind : 3;
    u32 binary_io : 1;
} FileMode;

typedef struct FileState {
    u32 io_state : 3;
    u32 free_buffer : 1;
    u8 eof;
    u8 error;
} FileState;

typedef void (*IdleProc)(void);
typedef int (*PositionProc)(file_handle, file_position *, int, IdleProc);
typedef int (*IOProc)(file_handle, u8 *, size_t *, IdleProc);
typedef int (*CloseProc)(file_handle);

typedef struct FILE FILE;

struct FILE {
    file_handle handle;
    FileMode mode;
    FileState state;
    u8 is_dynamically_allocated;
    u8 char_buffer;
    u8 char_buffer_overflow;
    u8 ungetc_buffer[2];
    u16 ungetc_wide_buffer[2];
    u32 position;
    u8 *buffer;
    u32 buffer_size;
    u8 *buffer_ptr;
    u32 buffer_length;
    u32 buffer_alignment;
    u32 saved_buffer_length;
    u32 buffer_position;
    PositionProc position_proc;
    IOProc read_proc;
    IOProc write_proc;
    CloseProc close_proc;
    IdleProc idle_proc;
    FILE *next_file;
};

extern int __position_file(file_handle, file_position *, int, IdleProc);
extern int __read_file(file_handle, u8 *, size_t *, IdleProc);
extern int __write_file(file_handle, u8 *, size_t *, IdleProc);
extern int __close_file(file_handle);
extern int __read_console(file_handle, u8 *, size_t *, IdleProc);
extern int __write_console(file_handle, u8 *, size_t *, IdleProc);
extern int __close_console(file_handle);

extern int fflush(FILE *);
extern int fclose(FILE *);
extern int setvbuf(FILE *, char *, int, size_t);
extern void __begin_critical_region(int);
extern void __end_critical_region(int);
extern void *malloc(size_t);
extern void free(void *);
extern void *memset(void *, int, size_t);

static u8 stdin_buff[0x100];
static u8 stdout_buff[0x100];
static u8 stderr_buff[0x100];

FILE __files[4] = {
    {
        0, 0, 1, BUFFER_MODE_LINE, FILE_KIND_CONSOLE, 0,
        IO_STATE_NEUTRAL, 0, 0, 0,
        0, 0, 0, {0, 0}, {0, 0}, 0,
        stdin_buff, sizeof(stdin_buff), stdin_buff,
        0, 0, 0, 0,
        0, __read_console, __write_console, __close_console, 0,
        &__files[1],
    },
    {
        1, 0, 2, BUFFER_MODE_LINE, FILE_KIND_CONSOLE, 0,
        IO_STATE_NEUTRAL, 0, 0, 0,
        0, 0, 0, {0, 0}, {0, 0}, 0,
        stdout_buff, sizeof(stdout_buff), stdout_buff,
        0, 0, 0, 0,
        0, __read_console, __write_console, __close_console, 0,
        &__files[2],
    },
    {
        2, 0, 2, BUFFER_MODE_UNBUFFERED, FILE_KIND_CONSOLE, 0,
        IO_STATE_NEUTRAL, 0, 0, 0,
        0, 0, 0, {0, 0}, {0, 0}, 0,
        stderr_buff, sizeof(stderr_buff), stderr_buff,
        0, 0, 0, 0,
        0, __read_console, __write_console, __close_console, 0,
        &__files[3],
    },
};

int __flush_line_buffered_output_files(void)
{
    int result = 0;
    FILE *file = &__files[0];

    while (file != 0) {
        if (file->mode.file_kind != FILE_KIND_CLOSED &&
            (file->mode.buffer_mode & BUFFER_MODE_LINE) != 0 &&
            file->state.io_state == IO_STATE_WRITING && fflush(file) != 0) {
            result = -1;
        }
        file = file->next_file;
    }

    return result;
}

int __flush_all(void)
{
    int result = 0;
    FILE *file = &__files[0];

    while (file != 0) {
        if (file->mode.file_kind != FILE_KIND_CLOSED && fflush(file) != 0) {
            result = -1;
        }
        file = file->next_file;
    }

    return result;
}

void __close_all(void)
{
    FILE *file = &__files[0];
    FILE *last_file;

    __begin_critical_region(2);

    while (file != 0) {
        if (file->mode.file_kind != FILE_KIND_CLOSED) {
            fclose(file);
        }

        last_file = file;
        file = file->next_file;
        if (last_file->is_dynamically_allocated) {
            free(last_file);
        } else {
            last_file->mode.file_kind = FILE_KIND_UNAVAILABLE;
            if (file != 0 && file->is_dynamically_allocated) {
                last_file->next_file = 0;
            }
        }
    }

    __end_critical_region(2);
}

void __init_file(FILE *file, const FileMode *mode, char *buffer, size_t buffer_size)
{
    file->handle = 0;
    file->mode = *mode;
    file->state.io_state = IO_STATE_NEUTRAL;
    file->state.free_buffer = 0;
    file->state.eof = 0;
    file->state.error = 0;
    file->position = 0;

    if (buffer_size != 0) {
        setvbuf(file, buffer, BUFFER_MODE_FULL, buffer_size);
    } else {
        setvbuf(file, 0, BUFFER_MODE_UNBUFFERED, 0);
    }

    file->buffer_ptr = file->buffer;
    file->buffer_length = 0;

    if (file->mode.file_kind == FILE_KIND_DISK) {
        file->position_proc = __position_file;
        file->read_proc = __read_file;
        file->write_proc = __write_file;
        file->close_proc = __close_file;
    }

    file->idle_proc = 0;
}

FILE *__find_unopened_file(void)
{
    FILE *file = __files[2].next_file;
    FILE *last_file;

    while (file != 0) {
        if (file->mode.file_kind == FILE_KIND_CLOSED) {
            return file;
        }
        last_file = file;
        file = file->next_file;
    }

    file = malloc(sizeof(FILE));
    if (file != 0) {
        memset(file, 0, sizeof(FILE));
        file->is_dynamically_allocated = 1;
        last_file->next_file = file;
        return file;
    }

    return 0;
}
