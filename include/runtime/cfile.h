#ifndef MKD_RUNTIME_CFILE_H
#define MKD_RUNTIME_CFILE_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long size_t;

typedef unsigned long file_handle;
typedef unsigned long file_position;

typedef struct FileMode {
    u32 open_mode : 2;
    u32 io_mode : 3;
    u32 buffer_mode : 2;
    u32 file_kind : 3;
    u32 file_orientation : 2;
    u32 binary_io : 1;
} FileMode;

typedef union FileModeWord {
    u32 value;
    FileMode bits;
    u16 half[2];
} FileModeWord;

typedef struct FileState {
    u32 io_state : 3;
    u32 free_buffer : 1;
    u8 eof;
    u8 error;
} FileState;

typedef void (*IdleProc)(void);
typedef int (*PositionProc)(file_handle handle, file_position* position, int mode,
                            IdleProc idle);
typedef int (*IOProc)(file_handle handle, u8* buffer, size_t* count, IdleProc idle);
typedef int (*CloseProc)(file_handle handle);

typedef struct FILE {
    file_handle handle;
    FileModeWord mode;
    FileState state;
    u8 is_dynamically_allocated;
    u8 char_buffer;
    u8 char_buffer_overflow;
    u8 ungetc_buffer[2];
    u16 ungetc_wide_buffer[2];
    u32 position;
    u8* buffer;
    u32 buffer_size;
    u8* buffer_ptr;
    u32 buffer_length;
    u32 buffer_alignment;
    u32 saved_buffer_length;
    u32 buffer_position;
    PositionProc position_proc;
    IOProc read_proc;
    IOProc write_proc;
    CloseProc close_proc;
    IdleProc idle_proc;
    struct FILE* next_file;
} FILE;

typedef char MSLFileSizeCheck[sizeof(FILE) == 0x50 ? 1 : -1];

FILE* fopen(const char* name, const char* mode);
int fclose(FILE* file);
size_t fread(void* address, size_t size, size_t count, FILE* file);
size_t fwrite(const void* address, size_t size, size_t count, FILE* file);
char* fgets(char* buffer, int maxLength, FILE* file);
int fputs(const char* buffer, FILE* file);
int feof(FILE* file);
int fseek(FILE* file, u32 offset, int origin);
int fflush(FILE* file);
long ftell(FILE* file);

#endif
