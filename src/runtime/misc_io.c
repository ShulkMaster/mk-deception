#include "runtime/cfile.h"

typedef struct MSLFileState {
    unsigned char flags;
    unsigned char eof;
    unsigned char error;
} MSLFileState;

struct FILE {
    unsigned long handle;
    unsigned long mode;
    MSLFileState state;
};

extern void (*__stdio_exit)(void);

void __close_all(void);

void __stdio_atexit(void)
{
    __stdio_exit = __close_all;
}

int feof(FILE* file)
{
    return file->state.eof;
}

void clearerr(FILE* file)
{
    file->state.eof = 0;
    file->state.error = 0;
}
